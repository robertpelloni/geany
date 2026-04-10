/* BobguiPrinterOptionWidget
 * Copyright (C) 2006 Alexander Larsson  <alexl@redhat.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include <glib/gi18n-lib.h>

#include "bobguiprinteroptionwidgetprivate.h"
#include "bobguistringpairprivate.h"

/* This defines the max file length that the file chooser
 * button should display. The total length will be
 * FILENAME_LENGTH_MAX+3 because the truncated name is prefixed
 * with “...”.
 */
#define FILENAME_LENGTH_MAX 27

static void bobgui_printer_option_widget_finalize (GObject *object);

static void deconstruct_widgets (BobguiPrinterOptionWidget *widget);
static void construct_widgets   (BobguiPrinterOptionWidget *widget);
static void update_widgets      (BobguiPrinterOptionWidget *widget);

static char *trim_long_filename (const char *filename);

struct BobguiPrinterOptionWidgetPrivate
{
  BobguiPrinterOption *source;
  gulong source_changed_handler;
  gulong comboentry_changed_handler_id;

  BobguiWidget *check;
  BobguiWidget *combo;
  BobguiWidget *entry;
  BobguiWidget *image;
  BobguiWidget *label;
  BobguiWidget *info_label;
  BobguiWidget *box;
  BobguiWidget *button;

  /* the last location for save to file, that the user selected */
  GFile *last_location;
};

enum {
  CHANGED,
  LAST_SIGNAL
};

enum {
  PROP_0,
  PROP_SOURCE
};

static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE_WITH_PRIVATE (BobguiPrinterOptionWidget, bobgui_printer_option_widget, BOBGUI_TYPE_BOX)

static void bobgui_printer_option_widget_set_property (GObject      *object,
						    guint         prop_id,
						    const GValue *value,
						    GParamSpec   *pspec);
static void bobgui_printer_option_widget_get_property (GObject      *object,
						    guint         prop_id,
						    GValue       *value,
						    GParamSpec   *pspec);
static gboolean bobgui_printer_option_widget_mnemonic_activate (BobguiWidget *widget,
							     gboolean   group_cycling);

static void
bobgui_printer_option_widget_class_init (BobguiPrinterOptionWidgetClass *class)
{
  GObjectClass *object_class;
  BobguiWidgetClass *widget_class;

  object_class = (GObjectClass *) class;
  widget_class = (BobguiWidgetClass *) class;

  object_class->finalize = bobgui_printer_option_widget_finalize;
  object_class->set_property = bobgui_printer_option_widget_set_property;
  object_class->get_property = bobgui_printer_option_widget_get_property;

  widget_class->mnemonic_activate = bobgui_printer_option_widget_mnemonic_activate;

  signals[CHANGED] =
    g_signal_new ("changed",
		  G_TYPE_FROM_CLASS (class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BobguiPrinterOptionWidgetClass, changed),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 0);

  g_object_class_install_property (object_class,
                                   PROP_SOURCE,
                                   g_param_spec_object ("source", NULL, NULL,
							BOBGUI_TYPE_PRINTER_OPTION,
							G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

}

static void
bobgui_printer_option_widget_init (BobguiPrinterOptionWidget *widget)
{
  widget->priv = bobgui_printer_option_widget_get_instance_private (widget);

  bobgui_box_set_spacing (BOBGUI_BOX (widget), 12);
}

static void
bobgui_printer_option_widget_finalize (GObject *object)
{
  BobguiPrinterOptionWidget *widget = BOBGUI_PRINTER_OPTION_WIDGET (object);
  BobguiPrinterOptionWidgetPrivate *priv = widget->priv;

  if (priv->source)
    {
      g_signal_handler_disconnect (priv->source,
				   priv->source_changed_handler);
      g_object_unref (priv->source);
      priv->source = NULL;
    }

  G_OBJECT_CLASS (bobgui_printer_option_widget_parent_class)->finalize (object);
}

static void
bobgui_printer_option_widget_set_property (GObject         *object,
					guint            prop_id,
					const GValue    *value,
					GParamSpec      *pspec)
{
  BobguiPrinterOptionWidget *widget;

  widget = BOBGUI_PRINTER_OPTION_WIDGET (object);

  switch (prop_id)
    {
    case PROP_SOURCE:
      bobgui_printer_option_widget_set_source (widget, g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_printer_option_widget_get_property (GObject    *object,
					guint       prop_id,
					GValue     *value,
					GParamSpec *pspec)
{
  BobguiPrinterOptionWidget *widget = BOBGUI_PRINTER_OPTION_WIDGET (object);
  BobguiPrinterOptionWidgetPrivate *priv = widget->priv;

  switch (prop_id)
    {
    case PROP_SOURCE:
      g_value_set_object (value, priv->source);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static gboolean
bobgui_printer_option_widget_mnemonic_activate (BobguiWidget *widget,
					     gboolean   group_cycling)
{
  BobguiPrinterOptionWidget *powidget = BOBGUI_PRINTER_OPTION_WIDGET (widget);
  BobguiPrinterOptionWidgetPrivate *priv = powidget->priv;

  if (priv->check)
    return bobgui_widget_mnemonic_activate (priv->check, group_cycling);
  if (priv->combo)
    return bobgui_widget_mnemonic_activate (priv->combo, group_cycling);
  if (priv->entry)
    return bobgui_widget_mnemonic_activate (priv->entry, group_cycling);
  if (priv->button)
    return bobgui_widget_mnemonic_activate (priv->button, group_cycling);

  return FALSE;
}

static void
emit_changed (BobguiPrinterOptionWidget *widget)
{
  g_signal_emit (widget, signals[CHANGED], 0);
}

BobguiWidget *
bobgui_printer_option_widget_new (BobguiPrinterOption *source)
{
  return g_object_new (BOBGUI_TYPE_PRINTER_OPTION_WIDGET, "source", source, NULL);
}

static void
source_changed_cb (BobguiPrinterOption *source,
		   BobguiPrinterOptionWidget  *widget)
{
  update_widgets (widget);
  emit_changed (widget);
}

void
bobgui_printer_option_widget_set_source (BobguiPrinterOptionWidget *widget,
				      BobguiPrinterOption       *source)
{
  BobguiPrinterOptionWidgetPrivate *priv = widget->priv;

  if (source)
    g_object_ref (source);

  if (priv->source)
    {
      g_signal_handler_disconnect (priv->source,
				   priv->source_changed_handler);
      g_object_unref (priv->source);
    }

  priv->source = source;

  if (source)
    priv->source_changed_handler =
      g_signal_connect (source, "changed", G_CALLBACK (source_changed_cb), widget);

  construct_widgets (widget);
  update_widgets (widget);

  g_object_notify (G_OBJECT (widget), "source");
}

static void
combo_box_set_model (BobguiWidget *combo_box)
{
  GListStore *store;

  store = g_list_store_new (BOBGUI_TYPE_STRING_PAIR);
  bobgui_drop_down_set_model (BOBGUI_DROP_DOWN (combo_box), G_LIST_MODEL (store));
  g_object_unref (store);
}

static void
setup_no_item (BobguiSignalListItemFactory *factory,
               BobguiListItem              *item)
{
}

static void
setup_list_item (BobguiSignalListItemFactory *factory,
                 BobguiListItem              *item)
{
  BobguiWidget *label;

  label = bobgui_label_new ("");
  bobgui_widget_set_halign (label, BOBGUI_ALIGN_START);
  bobgui_list_item_set_child (item, label);
}

static void
bind_list_item (BobguiSignalListItemFactory *factory,
                BobguiListItem              *item)
{
  BobguiStringPair *pair;
  BobguiWidget *label;

  pair = bobgui_list_item_get_item (item);
  label = bobgui_list_item_get_child (item);

  bobgui_label_set_text (BOBGUI_LABEL (label), bobgui_string_pair_get_string (pair));
}

static void
combo_box_set_view (BobguiWidget *combo_box)
{
  BobguiListItemFactory *factory;

  factory = bobgui_signal_list_item_factory_new ();
  g_signal_connect (factory, "setup", G_CALLBACK (setup_list_item), NULL);
  g_signal_connect (factory, "bind", G_CALLBACK (bind_list_item), NULL);
  bobgui_drop_down_set_factory (BOBGUI_DROP_DOWN (combo_box), factory);
  g_object_unref (factory);
}

static void
selected_changed (BobguiDropDown *dropdown,
                  GParamSpec *pspec,
                  gpointer data)
{
  GListModel *model;
  guint selected;
  BobguiStringPair *pair;
  BobguiWidget *entry = data;

  model = bobgui_drop_down_get_model (dropdown);
  selected = bobgui_drop_down_get_selected (dropdown);

  pair = g_list_model_get_item (model, selected);
  if (pair)
    {
      bobgui_editable_set_text (BOBGUI_EDITABLE (entry), bobgui_string_pair_get_string (pair));
      g_object_unref (pair);
    }
  else
    bobgui_editable_set_text (BOBGUI_EDITABLE (entry), "");

}

static BobguiWidget *
combo_box_entry_new (void)
{
  BobguiWidget *hbox, *entry, *button;
  BobguiListItemFactory *factory;

  hbox = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
  bobgui_widget_add_css_class (hbox, "linked");

  entry = bobgui_entry_new ();
  button = bobgui_drop_down_new (NULL, NULL);
  combo_box_set_model (button);

  factory = bobgui_signal_list_item_factory_new ();
  g_signal_connect (factory, "setup", G_CALLBACK (setup_no_item), NULL);
  bobgui_drop_down_set_factory (BOBGUI_DROP_DOWN (button), factory);
  g_object_unref (factory);

  factory = bobgui_signal_list_item_factory_new ();
  g_signal_connect (factory, "setup", G_CALLBACK (setup_list_item), NULL);
  g_signal_connect (factory, "bind", G_CALLBACK (bind_list_item), NULL);
  bobgui_drop_down_set_list_factory (BOBGUI_DROP_DOWN (button), factory);
  g_object_unref (factory);

  g_signal_connect (button, "notify::selected", G_CALLBACK (selected_changed), entry);

  bobgui_box_append (BOBGUI_BOX (hbox), entry);
  bobgui_box_append (BOBGUI_BOX (hbox), button);

  return hbox;
}

static BobguiWidget *
combo_box_new (void)
{
  BobguiWidget *combo_box;

  combo_box = bobgui_drop_down_new (NULL, NULL);

  combo_box_set_model (combo_box);
  combo_box_set_view (combo_box);

  return combo_box;
}

static void
combo_box_append (BobguiWidget   *combo,
                  const char *display_text,
                  const char *value)
{
  BobguiWidget *dropdown;
  GListModel *model;
  BobguiStringPair *object;

  if (BOBGUI_IS_DROP_DOWN (combo))
    dropdown = combo;
  else
    dropdown = bobgui_widget_get_last_child (combo);

  model = bobgui_drop_down_get_model (BOBGUI_DROP_DOWN (dropdown));

  object = bobgui_string_pair_new (value, display_text);
  g_list_store_append (G_LIST_STORE (model), object);
  g_object_unref (object);
}

static void
combo_box_set (BobguiWidget   *combo,
               const char *value)
{
  BobguiWidget *dropdown;
  GListModel *model;
  guint i;

  if (BOBGUI_IS_DROP_DOWN (combo))
    dropdown = combo;
  else
    dropdown = bobgui_widget_get_last_child (combo);

  model = bobgui_drop_down_get_model (BOBGUI_DROP_DOWN (dropdown));

  for (i = 0; i < g_list_model_get_n_items (model); i++)
    {
      BobguiStringPair *item = g_list_model_get_item (model, i);
      if (strcmp (value, bobgui_string_pair_get_id (item)) == 0)
        {
          bobgui_drop_down_set_selected (BOBGUI_DROP_DOWN (dropdown), i);
          g_object_unref (item);
          break;
        }
      g_object_unref (item);
    }
}

static char *
combo_box_get (BobguiWidget *combo, gboolean *custom)
{
  BobguiWidget *dropdown;
  GListModel *model;
  guint selected;
  gpointer item;
  const char *id;
  const char *string;

  if (BOBGUI_IS_DROP_DOWN (combo))
    dropdown = combo;
  else
    dropdown = bobgui_widget_get_last_child (combo);

  model = bobgui_drop_down_get_model (BOBGUI_DROP_DOWN (dropdown));
  selected = bobgui_drop_down_get_selected (BOBGUI_DROP_DOWN (dropdown));
  item = g_list_model_get_item (model, selected);
  if (item)
    {
      id = bobgui_string_pair_get_id (item);
      string = bobgui_string_pair_get_string (item);
      g_object_unref (item);
    }
  else
    {
      id = "";
      string = NULL;
    }

  if (dropdown == combo) // no entry
    {
      *custom = FALSE;
      return g_strdup (id);
    }
  else
    {
      const char *text;

      text = bobgui_editable_get_text (BOBGUI_EDITABLE (bobgui_widget_get_first_child (combo)));
      if (g_strcmp0 (text, string) == 0)
        {
          *custom = FALSE;
          return g_strdup (id);
        }
      else
        {
          *custom = TRUE;
          return g_strdup (text);
        }
    }
}

static void
deconstruct_widgets (BobguiPrinterOptionWidget *widget)
{
  BobguiPrinterOptionWidgetPrivate *priv = widget->priv;

  g_clear_pointer (&priv->check, bobgui_widget_unparent);
  g_clear_pointer (&priv->combo, bobgui_widget_unparent);
  g_clear_pointer (&priv->entry, bobgui_widget_unparent);
  g_clear_pointer (&priv->image, bobgui_widget_unparent);
  g_clear_pointer (&priv->label, bobgui_widget_unparent);
  g_clear_pointer (&priv->info_label, bobgui_widget_unparent);
}

static void
check_toggled_cb (BobguiCheckButton         *check_button,
		  BobguiPrinterOptionWidget *widget)
{
  BobguiPrinterOptionWidgetPrivate *priv = widget->priv;

  g_signal_handler_block (priv->source, priv->source_changed_handler);
  bobgui_printer_option_set_boolean (priv->source,
                                  bobgui_check_button_get_active (check_button));
  g_signal_handler_unblock (priv->source, priv->source_changed_handler);
  emit_changed (widget);
}

static void
dialog_response_callback (GObject *source,
                          GAsyncResult *result,
                          gpointer data)
{
  BobguiFileDialog *dialog = BOBGUI_FILE_DIALOG (source);
  BobguiPrinterOptionWidget *widget = data;
  BobguiPrinterOptionWidgetPrivate *priv = widget->priv;
  GFile *new_location = NULL;
  char *uri = NULL;

  new_location = bobgui_file_dialog_save_finish (dialog, result, NULL);
  if (new_location)
    {
      GFileInfo *info;

      info = g_file_query_info (new_location,
                                "standard::display-name",
                                0,
                                NULL,
                                NULL);
      if (info != NULL)
        {
          const char *filename_utf8 = g_file_info_get_display_name (info);

          char *filename_short = trim_long_filename (filename_utf8);
          bobgui_button_set_label (BOBGUI_BUTTON (priv->button), filename_short);

          g_free (filename_short);
          g_object_unref (info);
        }
      else
        {
          const char *path = g_file_peek_path (new_location);
          char *filename_utf8 = g_utf8_make_valid (path, -1);

          char *filename_short = trim_long_filename (filename_utf8);
          bobgui_button_set_label (BOBGUI_BUTTON (priv->button), filename_short);

          g_free (filename_short);
          g_free (filename_utf8);
        }
    }

  if (new_location)
    uri = g_file_get_uri (new_location);
  else
    uri = g_file_get_uri (priv->last_location);

  if (uri != NULL)
    {
      bobgui_printer_option_set (priv->source, uri);
      emit_changed (widget);
      g_free (uri);
    }

  g_clear_object (&new_location);
  g_clear_object (&priv->last_location);

  /* unblock the handler which was blocked in the filesave_choose_cb function */
  g_signal_handler_unblock (priv->source, priv->source_changed_handler);
}

static void
filesave_choose_cb (BobguiWidget              *button,
                    BobguiPrinterOptionWidget *widget)
{
  BobguiPrinterOptionWidgetPrivate *priv = widget->priv;
  BobguiFileDialog *dialog;

  /* this will be unblocked in the dialog_response_callback function */
  g_signal_handler_block (priv->source, priv->source_changed_handler);

  dialog = bobgui_file_dialog_new ();
  bobgui_file_dialog_set_title (dialog, _("Select a filename"));

  /* select the current filename in the dialog */
  if (priv->source != NULL && priv->source->value != NULL)
    {
      priv->last_location = g_file_new_for_uri (priv->source->value);
      if (priv->last_location)
        {
          if (g_file_query_file_type (priv->last_location, 0, NULL) == G_FILE_TYPE_DIRECTORY)
            bobgui_file_dialog_set_initial_folder (dialog, priv->last_location);
          else
            bobgui_file_dialog_set_initial_file (dialog, priv->last_location);
        }
    }

  bobgui_file_dialog_save (dialog,
                        BOBGUI_WINDOW (bobgui_widget_get_root (BOBGUI_WIDGET (widget))),
                        NULL,
                        dialog_response_callback, widget);
}

static char *
filter_numeric (const char *val,
                gboolean     allow_neg,
		gboolean     allow_dec,
                gboolean    *changed_out)
{
  char *filtered_val;
  int i, j;
  int len = strlen (val);
  gboolean dec_set = FALSE;

  filtered_val = g_malloc (len + 1);

  for (i = 0, j = 0; i < len; i++)
    {
      if (isdigit (val[i]))
        {
          filtered_val[j] = val[i];
	  j++;
	}
      else if (allow_dec && !dec_set &&
               (val[i] == '.' || val[i] == ','))
        {
	  /* allow one period or comma
	   * we should be checking locals
	   * but this is good enough for now
	   */
          filtered_val[j] = val[i];
	  dec_set = TRUE;
	  j++;
	}
      else if (allow_neg && i == 0 && val[0] == '-')
        {
          filtered_val[0] = val[0];
	  j++;
	}
    }

  filtered_val[j] = '\0';
  *changed_out = !(i == j);

  return filtered_val;
}

static void
handle_combo_entry_change (BobguiPrinterOptionWidget *widget)
{
  BobguiPrinterOptionWidgetPrivate *priv = widget->priv;
  char *value;
  char *filtered_val = NULL;
  gboolean changed;
  gboolean custom = TRUE;

  g_signal_handler_block (priv->source, priv->source_changed_handler);

  value = combo_box_get (priv->combo, &custom);

  /* Handle constraints if the user entered a custom value. */
  if (custom)
    {
      switch (priv->source->type)
        {
        case BOBGUI_PRINTER_OPTION_TYPE_PICKONE_PASSCODE:
          filtered_val = filter_numeric (value, FALSE, FALSE, &changed);
          break;
        case BOBGUI_PRINTER_OPTION_TYPE_PICKONE_INT:
          filtered_val = filter_numeric (value, TRUE, FALSE, &changed);
          break;
        case BOBGUI_PRINTER_OPTION_TYPE_PICKONE_REAL:
          filtered_val = filter_numeric (value, TRUE, TRUE, &changed);
          break;
        case BOBGUI_PRINTER_OPTION_TYPE_BOOLEAN:
        case BOBGUI_PRINTER_OPTION_TYPE_PICKONE:
        case BOBGUI_PRINTER_OPTION_TYPE_PICKONE_PASSWORD:
        case BOBGUI_PRINTER_OPTION_TYPE_PICKONE_STRING:
        case BOBGUI_PRINTER_OPTION_TYPE_ALTERNATIVE:
        case BOBGUI_PRINTER_OPTION_TYPE_STRING:
        case BOBGUI_PRINTER_OPTION_TYPE_FILESAVE:
        case BOBGUI_PRINTER_OPTION_TYPE_INFO:
        default:
          break;
        }
    }

  if (filtered_val)
    {
      g_free (value);

      if (changed)
        {
          BobguiWidget *entry = bobgui_widget_get_first_child (priv->combo);
          gssize     buffer_length, filtered_buffer_length;
          gint       position;

          position = bobgui_editable_get_position (BOBGUI_EDITABLE (entry));
          buffer_length = bobgui_entry_buffer_get_length (bobgui_entry_get_buffer (BOBGUI_ENTRY (entry)));

          g_signal_handler_block (entry, priv->comboentry_changed_handler_id);
          bobgui_editable_set_text (BOBGUI_EDITABLE (entry), filtered_val);
          g_signal_handler_unblock (entry, priv->comboentry_changed_handler_id);

          filtered_buffer_length = bobgui_entry_buffer_get_length (bobgui_entry_get_buffer (BOBGUI_ENTRY (entry)));

          /* Maintain position of the cursor with respect to the end of the buffer. */
          if (position > 0 && filtered_buffer_length < buffer_length)
            bobgui_editable_set_position (BOBGUI_EDITABLE (entry), position - (buffer_length - filtered_buffer_length));
        }
      value = filtered_val;
    }

  if (value)
    bobgui_printer_option_set (priv->source, value);
  g_free (value);
  g_signal_handler_unblock (priv->source, priv->source_changed_handler);
  emit_changed (widget);
}

static void
combo_changed_cb (BobguiWidget              *combo,
                  GParamSpec             *pspec,
                  BobguiPrinterOptionWidget *widget)
{
  handle_combo_entry_change (widget);
}

static void
comboentry_changed_cb (BobguiEditable            *editable,
                       BobguiPrinterOptionWidget *widget)
{
  handle_combo_entry_change (widget);
}

static void
entry_changed_cb (BobguiWidget              *entry,
		  BobguiPrinterOptionWidget *widget)
{
  BobguiPrinterOptionWidgetPrivate *priv = widget->priv;
  const char *value;

  g_signal_handler_block (priv->source, priv->source_changed_handler);
  value = bobgui_editable_get_text (BOBGUI_EDITABLE (entry));
  if (value)
    bobgui_printer_option_set (priv->source, value);
  g_signal_handler_unblock (priv->source, priv->source_changed_handler);
  emit_changed (widget);
}


static void
radio_changed_cb (BobguiWidget              *button,
		  BobguiPrinterOptionWidget *widget)
{
  BobguiPrinterOptionWidgetPrivate *priv = widget->priv;
  char *value;

  g_signal_handler_block (priv->source, priv->source_changed_handler);
  value = g_object_get_data (G_OBJECT (button), "value");
  if (value)
    bobgui_printer_option_set (priv->source, value);
  g_signal_handler_unblock (priv->source, priv->source_changed_handler);
  emit_changed (widget);
}

static void
alternative_set (BobguiWidget   *box,
                 const char *value)
{
  BobguiWidget *child;

  for (child = bobgui_widget_get_first_child (box);
       child != NULL;
       child = bobgui_widget_get_next_sibling (child))
    {
      char *v = g_object_get_data (G_OBJECT (child), "value");

      if (strcmp (value, v) == 0)
        {
          bobgui_check_button_set_active (BOBGUI_CHECK_BUTTON (child), TRUE);
          break;
        }
    }
}

static void
alternative_append (BobguiWidget              *box,
		    const char             *label,
                    const char             *value,
		    BobguiPrinterOptionWidget *widget,
		    BobguiWidget              **group)
{
  BobguiWidget *button;

  button = bobgui_check_button_new_with_label (label);
  if (*group)
    bobgui_check_button_set_group (BOBGUI_CHECK_BUTTON (button), BOBGUI_CHECK_BUTTON (*group));
  else
    *group = button;

  bobgui_widget_set_valign (button, BOBGUI_ALIGN_BASELINE_FILL);
  bobgui_box_append (BOBGUI_BOX (box), button);

  g_object_set_data (G_OBJECT (button), "value", (gpointer)value);
  g_signal_connect (button, "toggled", G_CALLBACK (radio_changed_cb), widget);
}

static void
construct_widgets (BobguiPrinterOptionWidget *widget)
{
  BobguiPrinterOptionWidgetPrivate *priv = widget->priv;
  BobguiPrinterOption *source;
  char *text;
  int i;
  BobguiWidget *group;

  source = priv->source;

  deconstruct_widgets (widget);

  bobgui_widget_set_sensitive (BOBGUI_WIDGET (widget), TRUE);

  if (source == NULL)
    {
      const char * strings[2];
      strings[0] = _("Not available");
      strings[1] = NULL;
      priv->combo = bobgui_drop_down_new_from_strings (strings);
      bobgui_drop_down_set_selected (BOBGUI_DROP_DOWN (priv->combo), 0);
      bobgui_widget_set_sensitive (BOBGUI_WIDGET (widget), FALSE);
      bobgui_box_append (BOBGUI_BOX (widget), priv->combo);
    }
  else switch (source->type)
    {
    case BOBGUI_PRINTER_OPTION_TYPE_BOOLEAN:
      priv->check = bobgui_check_button_new_with_mnemonic (source->display_text);
      g_signal_connect (priv->check, "toggled", G_CALLBACK (check_toggled_cb), widget);
      bobgui_box_append (BOBGUI_BOX (widget), priv->check);
      break;
    case BOBGUI_PRINTER_OPTION_TYPE_PICKONE:
    case BOBGUI_PRINTER_OPTION_TYPE_PICKONE_PASSWORD:
    case BOBGUI_PRINTER_OPTION_TYPE_PICKONE_PASSCODE:
    case BOBGUI_PRINTER_OPTION_TYPE_PICKONE_REAL:
    case BOBGUI_PRINTER_OPTION_TYPE_PICKONE_INT:
    case BOBGUI_PRINTER_OPTION_TYPE_PICKONE_STRING:
      if (source->type == BOBGUI_PRINTER_OPTION_TYPE_PICKONE)
        {
          priv->combo = combo_box_new ();
        }
      else
        {
          priv->combo = combo_box_entry_new ();

          if (source->type == BOBGUI_PRINTER_OPTION_TYPE_PICKONE_PASSWORD ||
              source->type == BOBGUI_PRINTER_OPTION_TYPE_PICKONE_PASSCODE)
            {
              BobguiWidget *entry = bobgui_widget_get_first_child (priv->combo);
              bobgui_entry_set_visibility (BOBGUI_ENTRY (entry), FALSE);
              bobgui_entry_set_input_purpose (BOBGUI_ENTRY (entry), 
                                          source->type == BOBGUI_PRINTER_OPTION_TYPE_PICKONE_PASSWORD ?
                                          BOBGUI_INPUT_PURPOSE_PASSWORD : BOBGUI_INPUT_PURPOSE_PIN);
            }
        }

      for (i = 0; i < source->num_choices; i++)
        combo_box_append (priv->combo,
                          source->choices_display[i],
                          source->choices[i]);
      bobgui_box_append (BOBGUI_BOX (widget), priv->combo);
      if (BOBGUI_IS_DROP_DOWN (priv->combo))
        {
          g_signal_connect (priv->combo, "notify::selected", G_CALLBACK (combo_changed_cb),widget);
        }
      else
        {
          g_signal_connect (bobgui_widget_get_last_child (priv->combo), "notify::selected", G_CALLBACK (combo_changed_cb), widget);
          priv->comboentry_changed_handler_id = g_signal_connect (bobgui_widget_get_first_child (priv->combo), "changed", G_CALLBACK (comboentry_changed_cb), widget);
        }


      text = g_strdup_printf ("%s:", source->display_text);
      priv->label = bobgui_label_new_with_mnemonic (text);
      g_free (text);
      break;

    case BOBGUI_PRINTER_OPTION_TYPE_ALTERNATIVE:
      group = NULL;
      priv->box = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 12);
      bobgui_widget_set_valign (priv->box, BOBGUI_ALIGN_BASELINE_FILL);
      bobgui_box_append (BOBGUI_BOX (widget), priv->box);
      for (i = 0; i < source->num_choices; i++)
        {
          alternative_append (priv->box,
                              source->choices_display[i],
                              source->choices[i],
                              widget,
                              &group);
          /* for mnemonic activation */
          if (i == 0)
            priv->button = group;
        }

      if (source->display_text)
	{
	  text = g_strdup_printf ("%s:", source->display_text);
	  priv->label = bobgui_label_new_with_mnemonic (text);
          bobgui_widget_set_valign (priv->label, BOBGUI_ALIGN_BASELINE_FILL);
	  g_free (text);
	}
      break;

    case BOBGUI_PRINTER_OPTION_TYPE_STRING:
      priv->entry = bobgui_entry_new ();
      bobgui_entry_set_activates_default (BOBGUI_ENTRY (priv->entry),
                                       bobgui_printer_option_get_activates_default (source));
      bobgui_box_append (BOBGUI_BOX (widget), priv->entry);
      g_signal_connect (priv->entry, "changed", G_CALLBACK (entry_changed_cb), widget);

      text = g_strdup_printf ("%s:", source->display_text);
      priv->label = bobgui_label_new_with_mnemonic (text);
      g_free (text);

      break;

    case BOBGUI_PRINTER_OPTION_TYPE_FILESAVE:
      priv->button = bobgui_button_new ();
      bobgui_box_append (BOBGUI_BOX (widget), priv->button);
      g_signal_connect (priv->button, "clicked", G_CALLBACK (filesave_choose_cb), widget);

      text = g_strdup_printf ("%s:", source->display_text);
      priv->label = bobgui_label_new_with_mnemonic (text);
      g_free (text);

      break;

    case BOBGUI_PRINTER_OPTION_TYPE_INFO:
      priv->info_label = bobgui_label_new (NULL);
      bobgui_label_set_selectable (BOBGUI_LABEL (priv->info_label), TRUE);
      bobgui_box_append (BOBGUI_BOX (widget), priv->info_label);

      text = g_strdup_printf ("%s:", source->display_text);
      priv->label = bobgui_label_new_with_mnemonic (text);
      g_free (text);

      break;

    default:
      break;
    }

  priv->image = bobgui_image_new_from_icon_name ("dialog-warning");
  bobgui_box_append (BOBGUI_BOX (widget), priv->image);
}

/*
 * If the filename exceeds FILENAME_LENGTH_MAX, then trim it and replace
 * the first three letters with three dots.
 */
static char *
trim_long_filename (const char *filename)
{
  const char *home;
  int len, offset;
  char *result;

  home = g_get_home_dir ();
  if (g_str_has_prefix (filename, home))
    {
      char *homeless_filename;

      offset = g_utf8_strlen (home, -1);
      len = g_utf8_strlen (filename, -1);
      homeless_filename = g_utf8_substring (filename, offset, len);
      result = g_strconcat ("~", homeless_filename, NULL);
      g_free (homeless_filename);
    }
  else
    result = g_strdup (filename);

  len = g_utf8_strlen (result, -1);
  if (len > FILENAME_LENGTH_MAX)
    {
      char *suffix;

      suffix = g_utf8_substring (result, len - FILENAME_LENGTH_MAX, len);
      g_free (result);
      result = g_strconcat ("...", suffix, NULL);
      g_free (suffix);
    }

  return result;
}

static void
update_widgets (BobguiPrinterOptionWidget *widget)
{
  BobguiPrinterOptionWidgetPrivate *priv = widget->priv;
  BobguiPrinterOption *source;

  source = priv->source;

  if (source == NULL)
    {
      bobgui_widget_set_visible (priv->image, FALSE);
      return;
    }

  switch (source->type)
    {
    case BOBGUI_PRINTER_OPTION_TYPE_BOOLEAN:
      if (g_ascii_strcasecmp (source->value, "True") == 0)
	bobgui_check_button_set_active (BOBGUI_CHECK_BUTTON (priv->check), TRUE);
      else
	bobgui_check_button_set_active (BOBGUI_CHECK_BUTTON (priv->check), FALSE);
      break;
    case BOBGUI_PRINTER_OPTION_TYPE_PICKONE:
      combo_box_set (priv->combo, source->value);
      break;
    case BOBGUI_PRINTER_OPTION_TYPE_ALTERNATIVE:
      alternative_set (priv->box, source->value);
      break;
    case BOBGUI_PRINTER_OPTION_TYPE_STRING:
      bobgui_editable_set_text (BOBGUI_EDITABLE (priv->entry), source->value);
      break;
    case BOBGUI_PRINTER_OPTION_TYPE_PICKONE_PASSWORD:
    case BOBGUI_PRINTER_OPTION_TYPE_PICKONE_PASSCODE:
    case BOBGUI_PRINTER_OPTION_TYPE_PICKONE_REAL:
    case BOBGUI_PRINTER_OPTION_TYPE_PICKONE_INT:
    case BOBGUI_PRINTER_OPTION_TYPE_PICKONE_STRING:
      {
        BobguiWidget *entry = bobgui_widget_get_first_child (priv->combo);
        if (bobgui_printer_option_has_choice (source, source->value))
          combo_box_set (priv->combo, source->value);
        else
          bobgui_editable_set_text (BOBGUI_EDITABLE (entry), source->value);

        break;
      }
    case BOBGUI_PRINTER_OPTION_TYPE_FILESAVE:
      {
        char *text;
        char *filename;

        filename = g_filename_from_uri (source->value, NULL, NULL);
        if (filename != NULL)
          {
            text = g_filename_to_utf8 (filename, -1, NULL, NULL, NULL);
            if (text != NULL)
              {
                char *short_filename;

                short_filename = trim_long_filename (text);
                bobgui_button_set_label (BOBGUI_BUTTON (priv->button), short_filename);
                g_free (short_filename);
              }

            g_free (text);
            g_free (filename);
          }
        else
          bobgui_button_set_label (BOBGUI_BUTTON (priv->button), source->value);
        break;
      }
    case BOBGUI_PRINTER_OPTION_TYPE_INFO:
      bobgui_label_set_text (BOBGUI_LABEL (priv->info_label), source->value);
      break;
    default:
      break;
    }

  bobgui_widget_set_visible (priv->image, source->has_conflict);
}

gboolean
bobgui_printer_option_widget_has_external_label (BobguiPrinterOptionWidget *widget)
{
  return widget->priv->label != NULL;
}

BobguiWidget *
bobgui_printer_option_widget_get_external_label (BobguiPrinterOptionWidget  *widget)
{
  return widget->priv->label;
}

const char *
bobgui_printer_option_widget_get_value (BobguiPrinterOptionWidget *widget)
{
  BobguiPrinterOptionWidgetPrivate *priv = widget->priv;

  if (priv->source)
    return priv->source->value;

  return "";
}
