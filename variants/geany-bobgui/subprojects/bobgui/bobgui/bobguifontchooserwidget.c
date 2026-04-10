/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2011 Alberto Ruiz <aruiz@gnome.org>
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
#include <glib/gprintf.h>
#include <string.h>

#include "deprecated/bobguifontchooserwidget.h"
#include "bobguifontchooserwidgetprivate.h"
#include "bobguifontfilterprivate.h"

#include "bobguiadjustment.h"
#include "bobguibuildable.h"
#include "bobguibox.h"
#include "bobguibinlayout.h"
#include "bobguicheckbutton.h"
#include "bobguicustomfilter.h"
#include "bobguientry.h"
#include "bobguifilter.h"
#include "bobguiframe.h"
#include "bobguigrid.h"
#include "deprecated/bobguifontchooser.h"
#include "bobguifontchooserutils.h"
#include <glib/gi18n-lib.h>
#include "bobguilabel.h"
#include "bobguisingleselection.h"
#include "bobguistack.h"
#include "bobguiprivate.h"
#include "bobguiscale.h"
#include "bobguiscrolledwindow.h"
#include "bobguisearchentry.h"
#include "bobguispinbutton.h"
#include "bobguitextview.h"
#include "bobguiwidgetprivate.h"
#include "bobguisettings.h"
#include "deprecated/bobguidialog.h"
#include "bobguigestureclick.h"
#include "bobguieventcontrollerscroll.h"
#include "bobguiroot.h"
#include "bobguifilterlistmodel.h"
#include "bobguiflattenlistmodel.h"
#include "bobguislicelistmodel.h"
#include "bobguimaplistmodel.h"
#include "bobguilistitem.h"
#include "bobguisignallistitemfactory.h"
#include "bobguistringlist.h"
#include "bobguilistview.h"
#include "bobguisortlistmodel.h"
#include "bobguistringsorter.h"
#include "bobguidropdown.h"
#include "bobguimultifilter.h"

#include <hb-ot.h>

#include "language-names.h"
#include "open-type-layout.h"

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

/**
 * BobguiFontChooserWidget:
 *
 * The `BobguiFontChooserWidget` widget lets the user select a font.
 *
 * It is used in the `BobguiFontChooserDialog` widget to provide a
 * dialog for selecting fonts.
 *
 * To set the font which is initially selected, use
 * [method@Bobgui.FontChooser.set_font] or [method@Bobgui.FontChooser.set_font_desc].
 *
 * To get the selected font use [method@Bobgui.FontChooser.get_font] or
 * [method@Bobgui.FontChooser.get_font_desc].
 *
 * To change the text which is shown in the preview area, use
 * [method@Bobgui.FontChooser.set_preview_text].
 *
 * # CSS nodes
 *
 * `BobguiFontChooserWidget` has a single CSS node with name fontchooser.
 *
 * Deprecated: 4.10: Direct use of `BobguiFontChooserWidget` is deprecated.
 */

typedef struct _BobguiFontChooserWidgetClass         BobguiFontChooserWidgetClass;

struct _BobguiFontChooserWidget
{
  BobguiWidget parent_instance;

  BobguiWidget    *stack;
  BobguiWidget    *grid;
  BobguiWidget    *search_entry;
  BobguiWidget    *family_face_list;
  BobguiWidget    *list_stack;
  BobguiSingleSelection *selection;
  BobguiCustomFilter    *custom_filter;
  BobguiFontFilter      *user_filter;
  BobguiCustomFilter    *multi_filter;
  BobguiFilterListModel *filter_model;

  BobguiWidget       *preview;
  BobguiWidget       *preview2;
  BobguiWidget       *font_name_label;
  char            *preview_text;
  gboolean         show_preview_entry;
  gboolean         preview_text_set;

  BobguiWidget *size_label;
  BobguiWidget *size_spin;
  BobguiWidget *size_slider;
  BobguiWidget *size_label2;
  BobguiWidget *size_spin2;
  BobguiWidget *size_slider2;

  BobguiWidget       *axis_grid;
  BobguiWidget       *feature_box;
  
  BobguiCheckButton    *language_button;
  BobguiSearchEntry   *language_search_entry;
  BobguiStringFilter *language_code_filter;
  BobguiStringFilter *language_name_filter;
  BobguiFilterListModel *language_filtered_model;
  BobguiSingleSelection *language_selection;
  BobguiFilter        *language_filter;
  BobguiFrame          *language_frame;
  BobguiWidget         *language_list;
  BobguiStringList     *languages;
  GHashTable        *language_table;

  PangoFontMap         *font_map;

  PangoFontDescription *font_desc;
  char                 *font_features;
  PangoLanguage        *language;

  BobguiFontFilterFunc filter_func;
  gpointer          filter_data;
  GDestroyNotify    filter_data_destroy;

  BobguiFilter *filter;

  guint last_fontconfig_timestamp;

  BobguiFontChooserLevel level;

  GHashTable *axes;
  gboolean updating_variations;

  GList *feature_items;

  GAction *tweak_action;

  hb_map_t *glyphmap;
};

struct _BobguiFontChooserWidgetClass
{
  BobguiWidgetClass parent_class;
};

enum {
  PROP_ZERO,
  PROP_TWEAK_ACTION
};

static void bobgui_font_chooser_widget_set_property         (GObject         *object,
                                                          guint            prop_id,
                                                          const GValue    *value,
                                                          GParamSpec      *pspec);
static void bobgui_font_chooser_widget_get_property         (GObject         *object,
                                                          guint            prop_id,
                                                          GValue          *value,
                                                          GParamSpec      *pspec);
static void bobgui_font_chooser_widget_finalize             (GObject         *object);

static char    *bobgui_font_chooser_widget_get_font         (BobguiFontChooserWidget *fontchooser);
static void     bobgui_font_chooser_widget_set_font         (BobguiFontChooserWidget *fontchooser,
                                                          const char           *fontname);

static PangoFontDescription *bobgui_font_chooser_widget_get_font_desc  (BobguiFontChooserWidget *fontchooser);
static void                  bobgui_font_chooser_widget_merge_font_desc(BobguiFontChooserWidget       *fontchooser,
                                                                     const PangoFontDescription *font_desc);
static void                  bobgui_font_chooser_widget_take_font_desc (BobguiFontChooserWidget *fontchooser,
                                                                     PangoFontDescription *font_desc);


static const char *bobgui_font_chooser_widget_get_preview_text (BobguiFontChooserWidget *fontchooser);
static void         bobgui_font_chooser_widget_set_preview_text (BobguiFontChooserWidget *fontchooser,
                                                              const char           *text);

static gboolean bobgui_font_chooser_widget_get_show_preview_entry (BobguiFontChooserWidget *fontchooser);
static void     bobgui_font_chooser_widget_set_show_preview_entry (BobguiFontChooserWidget *fontchooser,
                                                                gboolean              show_preview_entry);

static void     bobgui_font_chooser_widget_populate_features      (BobguiFontChooserWidget *fontchooser);
static void                bobgui_font_chooser_widget_set_level (BobguiFontChooserWidget *fontchooser,
                                                              BobguiFontChooserLevel   level);
static BobguiFontChooserLevel bobgui_font_chooser_widget_get_level (BobguiFontChooserWidget *fontchooser);
static void                bobgui_font_chooser_widget_set_language (BobguiFontChooserWidget *fontchooser,
                                                                 const char           *language);
static void update_font_features (BobguiFontChooserWidget *fontchooser);

static void bobgui_font_chooser_widget_ensure_matching_selection (BobguiFontChooserWidget *self);

static void bobgui_font_chooser_widget_iface_init (BobguiFontChooserIface *iface);

G_DEFINE_TYPE_WITH_CODE (BobguiFontChooserWidget, bobgui_font_chooser_widget, BOBGUI_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_FONT_CHOOSER,
                                                bobgui_font_chooser_widget_iface_init))

static void
bobgui_font_chooser_widget_set_property (GObject         *object,
                                      guint            prop_id,
                                      const GValue    *value,
                                      GParamSpec      *pspec)
{
  BobguiFontChooserWidget *fontchooser = BOBGUI_FONT_CHOOSER_WIDGET (object);

  switch (prop_id)
    {
    case BOBGUI_FONT_CHOOSER_PROP_FONT:
      bobgui_font_chooser_widget_set_font (fontchooser, g_value_get_string (value));
      break;
    case BOBGUI_FONT_CHOOSER_PROP_FONT_DESC:
      bobgui_font_chooser_widget_take_font_desc (fontchooser, g_value_dup_boxed (value));
      break;
    case BOBGUI_FONT_CHOOSER_PROP_PREVIEW_TEXT:
      bobgui_font_chooser_widget_set_preview_text (fontchooser, g_value_get_string (value));
      fontchooser->preview_text_set = TRUE;
      break;
    case BOBGUI_FONT_CHOOSER_PROP_SHOW_PREVIEW_ENTRY:
      bobgui_font_chooser_widget_set_show_preview_entry (fontchooser, g_value_get_boolean (value));
      break;
    case BOBGUI_FONT_CHOOSER_PROP_LEVEL:
      bobgui_font_chooser_widget_set_level (fontchooser, g_value_get_flags (value));
      break;
    case BOBGUI_FONT_CHOOSER_PROP_LANGUAGE:
      bobgui_font_chooser_widget_set_language (fontchooser, g_value_get_string (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_font_chooser_widget_get_property (GObject         *object,
                                      guint            prop_id,
                                      GValue          *value,
                                      GParamSpec      *pspec)
{
  BobguiFontChooserWidget *fontchooser = BOBGUI_FONT_CHOOSER_WIDGET (object);

  switch (prop_id)
    {
    case PROP_TWEAK_ACTION:
      g_value_set_object (value, G_OBJECT (fontchooser->tweak_action));
      break;
    case BOBGUI_FONT_CHOOSER_PROP_FONT:
      g_value_take_string (value, bobgui_font_chooser_widget_get_font (fontchooser));
      break;
    case BOBGUI_FONT_CHOOSER_PROP_FONT_DESC:
      g_value_set_boxed (value, bobgui_font_chooser_widget_get_font_desc (fontchooser));
      break;
    case BOBGUI_FONT_CHOOSER_PROP_PREVIEW_TEXT:
      g_value_set_string (value, bobgui_font_chooser_widget_get_preview_text (fontchooser));
      break;
    case BOBGUI_FONT_CHOOSER_PROP_SHOW_PREVIEW_ENTRY:
      g_value_set_boolean (value, bobgui_font_chooser_widget_get_show_preview_entry (fontchooser));
      break;
    case BOBGUI_FONT_CHOOSER_PROP_LEVEL:
      g_value_set_flags (value, bobgui_font_chooser_widget_get_level (fontchooser));
      break;
    case BOBGUI_FONT_CHOOSER_PROP_FONT_FEATURES:
      g_value_set_string (value, fontchooser->font_features);
      break;
    case BOBGUI_FONT_CHOOSER_PROP_LANGUAGE:
      g_value_set_string (value, pango_language_to_string (fontchooser->language));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
stop_search_cb (BobguiSearchEntry       *entry,
                BobguiFontChooserWidget *fc)
{
  if (bobgui_editable_get_text (BOBGUI_EDITABLE (entry))[0] != 0)
    bobgui_editable_set_text (BOBGUI_EDITABLE (entry), "");
  else
    {
      BobguiWidget *dialog;
      BobguiWidget *button = NULL;

      dialog = bobgui_widget_get_ancestor (BOBGUI_WIDGET (fc), BOBGUI_TYPE_DIALOG);
      if (dialog)
        button = bobgui_dialog_get_widget_for_response (BOBGUI_DIALOG (dialog), BOBGUI_RESPONSE_CANCEL);

      if (button)
        bobgui_widget_activate (button);
    }
}

static void
size_change_cb (BobguiAdjustment *adjustment,
                gpointer       user_data)
{
  BobguiFontChooserWidget *fontchooser = user_data;
  PangoFontDescription *font_desc;
  double size = bobgui_adjustment_get_value (adjustment);

  font_desc = pango_font_description_new ();
  if (pango_font_description_get_size_is_absolute (fontchooser->font_desc))
    pango_font_description_set_absolute_size (font_desc, size * PANGO_SCALE);
  else
    pango_font_description_set_size (font_desc, size * PANGO_SCALE);

  bobgui_font_chooser_widget_take_font_desc (fontchooser, font_desc);
}

static gboolean
output_cb (BobguiSpinButton *spin,
           gpointer       data)
{
  BobguiAdjustment *adjustment;
  char *text;
  double value;

  adjustment = bobgui_spin_button_get_adjustment (spin);
  value = bobgui_adjustment_get_value (adjustment);
  text = g_strdup_printf ("%2.4g", value);
  bobgui_editable_set_text (BOBGUI_EDITABLE (spin), text);
  g_free (text);

  return TRUE;
}

static gboolean
update_filter_language_idle (gpointer user_data)
{
  BobguiFontChooserWidget *self = user_data;
  gboolean should_filter_language;
   
  should_filter_language = bobgui_check_button_get_active (self->language_button);
  if (!should_filter_language)
    {
      _bobgui_font_filter_set_language (self->user_filter, NULL);
    }
  else
    {
      gpointer item = bobgui_single_selection_get_selected_item (self->language_selection);
      if (item)
        {
          const char *lang_str = bobgui_string_object_get_string (BOBGUI_STRING_OBJECT (item));
          bobgui_font_chooser_widget_set_language (self, lang_str);
        }
      else
        {
          _bobgui_font_filter_set_language (self->user_filter, NULL);
        }
    }

  g_object_unref (self);

  return G_SOURCE_REMOVE;
}

static void
update_filter_language (BobguiFontChooserWidget *self)
{
  g_idle_add (update_filter_language_idle, g_object_ref (self));
}

static void
language_check_changed (BobguiCheckButton       *check,
                        GParamSpec           *pspec,
                        BobguiFontChooserWidget *self)
{
  update_filter_language (self);
}


static void
bobgui_font_chooser_widget_update_marks (BobguiFontChooserWidget *self)
{
  BobguiAdjustment *adj, *spin_adj;
  const int *sizes;
  int *font_sizes;
  int i, n_sizes;
  double value, spin_value;
  gpointer item;

  item = bobgui_single_selection_get_selected_item (self->selection);

  if (item)
    {
      PangoFontFace *face;

      if (PANGO_IS_FONT_FAMILY (item))
        face = pango_font_family_get_face (item, NULL);
      else
        face = item;

      pango_font_face_list_sizes (face, &font_sizes, &n_sizes);

      /* It seems not many fonts actually have a sane set of sizes */
      for (i = 0; i < n_sizes; i++)
        font_sizes[i] = font_sizes[i] / PANGO_SCALE;
    }
  else
    {
      font_sizes = NULL;
      n_sizes = 0;
    }

  if (n_sizes < 2)
    {
      static const int fallback_sizes[] = {
        6, 8, 9, 10, 11, 12, 13, 14, 16, 20, 24, 36, 48, 72
      };

      sizes = fallback_sizes;
      n_sizes = G_N_ELEMENTS (fallback_sizes);
    }
  else
    {
      sizes = font_sizes;
    }

  bobgui_scale_clear_marks (BOBGUI_SCALE (self->size_slider));
  bobgui_scale_clear_marks (BOBGUI_SCALE (self->size_slider2));

  adj        = bobgui_range_get_adjustment (BOBGUI_RANGE (self->size_slider));
  spin_adj   = bobgui_spin_button_get_adjustment (BOBGUI_SPIN_BUTTON (self->size_spin));
  spin_value = bobgui_adjustment_get_value (spin_adj);

  if (spin_value < sizes[0])
    value = (double) sizes[0];
  else if (spin_value > sizes[n_sizes - 1])
    value = (double)sizes[n_sizes - 1];
  else
    value = (double)spin_value;

  /* ensure clamping doesn't callback into font resizing code */
  g_signal_handlers_block_by_func (adj, size_change_cb, self);
  bobgui_adjustment_configure (adj,
                            value,
                            sizes[0],
                            sizes[n_sizes - 1],
                            bobgui_adjustment_get_step_increment (adj),
                            bobgui_adjustment_get_page_increment (adj),
                            bobgui_adjustment_get_page_size (adj));
  g_signal_handlers_unblock_by_func (adj, size_change_cb, self);

  for (i = 0; i < n_sizes; i++)
    {
      bobgui_scale_add_mark (BOBGUI_SCALE (self->size_slider),
                          sizes[i],
                          BOBGUI_POS_BOTTOM, NULL);
      bobgui_scale_add_mark (BOBGUI_SCALE (self->size_slider2),
                          sizes[i],
                          BOBGUI_POS_BOTTOM, NULL);
    }

  g_free (font_sizes);
}

static void
row_activated_cb (BobguiWidget *view,
                  guint      pos,
                  gpointer   user_data)
{
  BobguiFontChooserWidget *fontchooser = user_data;
  char *fontname;

  fontname = bobgui_font_chooser_widget_get_font (fontchooser);
  _bobgui_font_chooser_font_activated (BOBGUI_FONT_CHOOSER (fontchooser), fontname);
  g_free (fontname);
}

static void
resize_by_scroll_cb (BobguiEventControllerScroll *controller,
                     double                    dx,
                     double                    dy,
                     gpointer                  user_data)
{
  BobguiFontChooserWidget *self = user_data;
  BobguiAdjustment *adj = bobgui_spin_button_get_adjustment (BOBGUI_SPIN_BUTTON (self->size_spin));

  bobgui_adjustment_set_value (adj,
                            bobgui_adjustment_get_value (adj) +
                            bobgui_adjustment_get_step_increment (adj) * dx);
}

static void
maybe_update_preview_text (BobguiFontChooserWidget *self,
                           PangoFontFace        *face,
                           PangoFontDescription *desc)
{
  PangoContext *context;
  PangoFont *font;
  PangoLanguage *filter_lang;
  const char *sample;
  PangoLanguage **languages;
  GHashTable *langs = NULL;
  PangoLanguage *default_lang;
  PangoLanguage *alt_default = NULL;
  PangoLanguage *lang = NULL;
  int i;
  const char *p;

  /* If the user has typed text into the entry, we don't touch it */
  if (self->preview_text_set)
    return;

  filter_lang = _bobgui_font_filter_get_language (self->user_filter);
  if (filter_lang != NULL)
    {
      sample = pango_language_get_sample_string (filter_lang);
      bobgui_font_chooser_widget_set_preview_text (self, sample);
      return;
    }

  /* We do the work only once, and cache the result on the PangoFontFace */
  sample = (const char *)g_object_get_data (G_OBJECT (face), "bobgui-sample-text");
  if (sample)
    {
      bobgui_font_chooser_widget_set_preview_text (self, sample);
      return;
    }

  context = bobgui_widget_get_pango_context (BOBGUI_WIDGET (self));
  font = pango_context_load_font (context, desc);

  default_lang = pango_language_get_default ();
  p = pango_language_to_string (default_lang);

  /* The default language tends to be of the form en-us.
   * Since fontconfig languages just have the language part,
   * and we want to use direct pointer comparisons, we need
   * an PangoLanguage for the shortened default language.
   */
  if (strchr (p, '-'))
    {
      char q[10];
      for (i = 0; p[i] != '-' && i < 9; i++)
        q[i] = p[i];
      q[i] = '\0';
      alt_default = pango_language_from_string (q);
    }

  languages = pango_font_get_languages (font);

  /* If the font supports the default language, just use it. */
  if (languages)
    for (i = 0; languages[i]; i++)
      {
        if (languages[i] == default_lang || languages[i] == alt_default)
          {
            lang = default_lang;
            goto found;
          }
      }

  /* Otherwise, we make a list of representative languages */
  langs = g_hash_table_new (NULL, NULL);

  if (languages)
    for (i = 0; languages[i]; i++)
      {
        const PangoScript *scripts;
        int num, j;

        scripts = pango_language_get_scripts (languages[i], &num);
        for (j = 0; j < num; j++)
          {
            lang = pango_script_get_sample_language (scripts[j]);
            if (lang)
              g_hash_table_add (langs, lang);
          }
      }

  /* ... and compare it to the users default and preferred languages */
  if (g_hash_table_contains (langs, default_lang) ||
      g_hash_table_contains (langs, alt_default))
    {
      lang = default_lang;
    }
  else
    {
      PangoLanguage **preferred;

      preferred = pango_language_get_preferred ();
      if (preferred)
        {
          for (i = 0; preferred[i]; i++)
            {
              if (g_hash_table_contains (langs, preferred[i]))
                {
                  lang = preferred[i];
                  break;
                }
            }
        }

      g_hash_table_unref (langs);

found:
      sample = pango_language_get_sample_string (lang);
      bobgui_font_chooser_widget_set_preview_text (self, sample);
      g_object_set_data (G_OBJECT (face), "bobgui-sample-text", (gpointer)sample);
    }

  g_object_unref (font);
}


static void
selection_changed_cb (BobguiSingleSelection   *selection,
                      GParamSpec           *pspec,
                      BobguiFontChooserWidget *self)
{
  gpointer item;

  item = bobgui_single_selection_get_selected_item (selection);
  if (item)
    {
      PangoFontFace *face;
      PangoFontDescription *desc;

      if (PANGO_IS_FONT_FAMILY (item))
        face = pango_font_family_get_face (item, NULL);
      else
        face = item;
      desc = pango_font_face_describe (face);
      pango_font_description_set_variations (self->font_desc, NULL);
      bobgui_font_chooser_widget_merge_font_desc (self, desc);
      g_simple_action_set_enabled (G_SIMPLE_ACTION (self->tweak_action), TRUE);

      maybe_update_preview_text (self, face, desc);

      pango_font_description_free (desc);
    }
  else
    {
      g_simple_action_set_state (G_SIMPLE_ACTION (self->tweak_action), g_variant_new_boolean (FALSE));
      g_simple_action_set_enabled (G_SIMPLE_ACTION (self->tweak_action), FALSE);
    }

  g_object_notify (G_OBJECT (self), "font");
  g_object_notify (G_OBJECT (self), "font-desc");
}

static char *
get_font_name (GObject  *ignore,
               gpointer  item)
{
  if (item == NULL)
    return NULL;

  if (PANGO_IS_FONT_FACE (item))
    {
      return g_strconcat (pango_font_family_get_name (pango_font_face_get_family (item)),
                          " ",
                          pango_font_face_get_face_name (item),
                          NULL);
    }
  else
    {
      return g_strdup (pango_font_family_get_name (item));
    }
}

static PangoAttrList *
get_font_attributes (GObject  *ignore,
                     gpointer  item)
{
  PangoAttribute *attribute;
  PangoAttrList *attrs;

  attrs = pango_attr_list_new ();

  if (item)
    {
      PangoFontFace *face;
      PangoFontDescription *font_desc;

      if (PANGO_IS_FONT_FAMILY (item))
        face = pango_font_family_get_face (item, NULL);
      else
        face = item;
      if (face)
        {
          font_desc = pango_font_face_describe (face);
          attribute = pango_attr_font_desc_new (font_desc);
          pango_attr_list_insert (attrs, attribute);
          pango_font_description_free (font_desc);
        }
    }

  return attrs;
}

static void
bobgui_font_chooser_widget_update_preview_attributes (BobguiFontChooserWidget *fontchooser)
{
  PangoAttrList *attrs;

  attrs = pango_attr_list_new ();

  /* Prevent font fallback */
  pango_attr_list_insert (attrs, pango_attr_fallback_new (FALSE));

  /* Force current font and features */
  pango_attr_list_insert (attrs, pango_attr_font_desc_new (fontchooser->font_desc));
  if (fontchooser->font_features)
    pango_attr_list_insert (attrs, pango_attr_font_features_new (fontchooser->font_features));
  if (fontchooser->language)
    pango_attr_list_insert (attrs, pango_attr_language_new (fontchooser->language));

  bobgui_entry_set_attributes (BOBGUI_ENTRY (fontchooser->preview), attrs);

  pango_attr_list_unref (attrs);
}

static void
rows_changed_cb (BobguiFontChooserWidget *self)
{
  const char *page;
  if (bobgui_single_selection_get_selected (self->selection) == BOBGUI_INVALID_LIST_POSITION)
      bobgui_font_chooser_widget_ensure_matching_selection (self);
  if (g_list_model_get_n_items (G_LIST_MODEL (self->selection)) == 0 &&
      bobgui_filter_list_model_get_pending (BOBGUI_FILTER_LIST_MODEL (self->filter_model)) == 0)
    page = "empty";
  else
    page = "list";

  if (strcmp (bobgui_stack_get_visible_child_name (BOBGUI_STACK (self->list_stack)), page) != 0)
    bobgui_stack_set_visible_child_name (BOBGUI_STACK (self->list_stack), page);
}

static void
update_key_capture (BobguiWidget *chooser)
{
  BobguiFontChooserWidget *fontchooser = BOBGUI_FONT_CHOOSER_WIDGET (chooser);
  BobguiWidget *capture_widget;

  if (bobgui_widget_get_mapped (chooser) &&
      g_str_equal (bobgui_stack_get_visible_child_name (BOBGUI_STACK (fontchooser->stack)), "list"))
    {
      BobguiWidget *toplevel;
      BobguiWidget *focus;

      toplevel = BOBGUI_WIDGET (bobgui_widget_get_root (chooser));
      focus = bobgui_root_get_focus (BOBGUI_ROOT (toplevel));

      if (BOBGUI_IS_EDITABLE (focus) && focus != fontchooser->search_entry)
        capture_widget = NULL;
      else
        capture_widget = chooser;
    }
  else
    capture_widget = NULL;

  bobgui_search_entry_set_key_capture_widget (BOBGUI_SEARCH_ENTRY (fontchooser->search_entry),
                                           capture_widget);
}

static void
bobgui_font_chooser_widget_map (BobguiWidget *widget)
{
  BobguiFontChooserWidget *fontchooser = BOBGUI_FONT_CHOOSER_WIDGET (widget);   
  bobgui_widget_set_sensitive (BOBGUI_WIDGET (fontchooser->language_frame), FALSE);
  bobgui_check_button_set_active (fontchooser->language_button, FALSE);
  bobgui_single_selection_set_selected (fontchooser->language_selection, BOBGUI_INVALID_LIST_POSITION);
  bobgui_editable_set_text (BOBGUI_EDITABLE (fontchooser->search_entry), "");
  bobgui_stack_set_visible_child_name (BOBGUI_STACK (fontchooser->stack), "list");
  g_simple_action_set_state (G_SIMPLE_ACTION (fontchooser->tweak_action), g_variant_new_boolean (FALSE));
  BOBGUI_WIDGET_CLASS (bobgui_font_chooser_widget_parent_class)->map (widget);
  update_key_capture (widget);
}

static void
bobgui_font_chooser_widget_unmap (BobguiWidget *widget)
{
  update_key_capture (widget);

  BOBGUI_WIDGET_CLASS (bobgui_font_chooser_widget_parent_class)->unmap (widget);
}

static void
bobgui_font_chooser_widget_root (BobguiWidget *widget)
{
  BobguiFontChooserWidget *self = BOBGUI_FONT_CHOOSER_WIDGET (widget);

  BOBGUI_WIDGET_CLASS (bobgui_font_chooser_widget_parent_class)->root (widget);

  g_signal_connect_swapped (bobgui_widget_get_root (widget), "notify::focus-widget",
                            G_CALLBACK (update_key_capture), widget);
  _bobgui_font_filter_set_pango_context (self->user_filter,
                                      bobgui_widget_get_pango_context (widget));
}

static void
bobgui_font_chooser_widget_unroot (BobguiWidget *widget)
{
  BobguiFontChooserWidget *self = BOBGUI_FONT_CHOOSER_WIDGET (widget);

  _bobgui_font_filter_set_pango_context (self->user_filter,
                                      bobgui_widget_get_pango_context (widget));
  g_signal_handlers_disconnect_by_func (bobgui_widget_get_root (widget),
                                        update_key_capture, widget);

  BOBGUI_WIDGET_CLASS (bobgui_font_chooser_widget_parent_class)->unroot (widget);
}

static void
bobgui_font_chooser_widget_dispose (GObject *object)
{
  BobguiFontChooserWidget *self = BOBGUI_FONT_CHOOSER_WIDGET (object);

  g_signal_handlers_disconnect_by_func (self->selection, rows_changed_cb, self);
  g_signal_handlers_disconnect_by_func (self->filter_model, rows_changed_cb, self);

  self->filter_func = NULL;
  g_clear_pointer (&self->filter_data, self->filter_data_destroy);

  g_clear_pointer (&self->stack, bobgui_widget_unparent);
  g_clear_object (&self->language_selection); 
  g_clear_pointer (&self->language_table, g_hash_table_unref);

  G_OBJECT_CLASS (bobgui_font_chooser_widget_parent_class)->dispose (object);
}

static void
bobgui_font_chooser_widget_class_init (BobguiFontChooserWidgetClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);
  GParamSpec *pspec;

  g_type_ensure (G_TYPE_THEMED_ICON);
  g_type_ensure (BOBGUI_TYPE_FONT_FILTER);

  widget_class->root = bobgui_font_chooser_widget_root;
  widget_class->unroot = bobgui_font_chooser_widget_unroot;
  widget_class->map = bobgui_font_chooser_widget_map;
  widget_class->unmap = bobgui_font_chooser_widget_unmap;

  gobject_class->finalize = bobgui_font_chooser_widget_finalize;
  gobject_class->dispose = bobgui_font_chooser_widget_dispose;
  gobject_class->set_property = bobgui_font_chooser_widget_set_property;
  gobject_class->get_property = bobgui_font_chooser_widget_get_property;

  /**
   * BobguiFontChooserWidget:tweak-action:
   *
   * A toggle action that can be used to switch to the tweak page
   * of the font chooser widget, which lets the user tweak the
   * OpenType features and variation axes of the selected font.
   *
   * The action will be enabled or disabled depending on whether
   * the selected font has any features or axes.
   */
  pspec = g_param_spec_object ("tweak-action", NULL, NULL,
                               G_TYPE_ACTION,
                               G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (gobject_class, PROP_TWEAK_ACTION, pspec);

  _bobgui_font_chooser_install_properties (gobject_class);

  /* Bind class to template */
  bobgui_widget_class_set_template_from_resource (widget_class,
					       "/org/bobgui/libbobgui/ui/bobguifontchooserwidget.ui");

  bobgui_widget_class_bind_template_child (widget_class, BobguiFontChooserWidget, search_entry);
  bobgui_widget_class_bind_template_child (widget_class, BobguiFontChooserWidget, family_face_list);
  bobgui_widget_class_bind_template_child (widget_class, BobguiFontChooserWidget, list_stack);
  bobgui_widget_class_bind_template_child (widget_class, BobguiFontChooserWidget, filter_model);
  bobgui_widget_class_bind_template_child (widget_class, BobguiFontChooserWidget, selection);
  bobgui_widget_class_bind_template_child (widget_class, BobguiFontChooserWidget, custom_filter);
  bobgui_widget_class_bind_template_child (widget_class, BobguiFontChooserWidget, user_filter);
  bobgui_widget_class_bind_template_child (widget_class, BobguiFontChooserWidget, multi_filter);
  bobgui_widget_class_bind_template_child (widget_class, BobguiFontChooserWidget, preview);
  bobgui_widget_class_bind_template_child (widget_class, BobguiFontChooserWidget, preview2);
  bobgui_widget_class_bind_template_child (widget_class, BobguiFontChooserWidget, size_label);
  bobgui_widget_class_bind_template_child (widget_class, BobguiFontChooserWidget, size_spin);
  bobgui_widget_class_bind_template_child (widget_class, BobguiFontChooserWidget, size_slider);
  bobgui_widget_class_bind_template_child (widget_class, BobguiFontChooserWidget, size_label2);
  bobgui_widget_class_bind_template_child (widget_class, BobguiFontChooserWidget, size_spin2);
  bobgui_widget_class_bind_template_child (widget_class, BobguiFontChooserWidget, size_slider2);
  bobgui_widget_class_bind_template_child (widget_class, BobguiFontChooserWidget, stack);
  bobgui_widget_class_bind_template_child (widget_class, BobguiFontChooserWidget, grid);
  bobgui_widget_class_bind_template_child (widget_class, BobguiFontChooserWidget, font_name_label);
  bobgui_widget_class_bind_template_child (widget_class, BobguiFontChooserWidget, feature_box);
  bobgui_widget_class_bind_template_child (widget_class, BobguiFontChooserWidget, axis_grid);
  bobgui_widget_class_bind_template_child (widget_class, BobguiFontChooserWidget, language_button);
  bobgui_widget_class_bind_template_child (widget_class, BobguiFontChooserWidget, language_frame);
  bobgui_widget_class_bind_template_child (widget_class, BobguiFontChooserWidget, language_list);
  bobgui_widget_class_bind_template_callback (widget_class, get_font_name);
  bobgui_widget_class_bind_template_callback (widget_class, get_font_attributes);
  bobgui_widget_class_bind_template_callback (widget_class, stop_search_cb);
  bobgui_widget_class_bind_template_callback (widget_class, row_activated_cb);
  bobgui_widget_class_bind_template_callback (widget_class, rows_changed_cb);
  bobgui_widget_class_bind_template_callback (widget_class, size_change_cb);
  bobgui_widget_class_bind_template_callback (widget_class, output_cb);
  bobgui_widget_class_bind_template_callback (widget_class, selection_changed_cb);
  bobgui_widget_class_bind_template_callback (widget_class, resize_by_scroll_cb);
  bobgui_widget_class_bind_template_callback (widget_class, language_check_changed);
  bobgui_widget_class_bind_template_child (widget_class, BobguiFontChooserWidget, language_search_entry);

  bobgui_widget_class_set_layout_manager_type (widget_class, BOBGUI_TYPE_BIN_LAYOUT);
  bobgui_widget_class_set_css_name (widget_class, I_("fontchooser"));
}

static void
change_tweak (GSimpleAction *action,
              GVariant      *state,
              gpointer       data)
{
  BobguiFontChooserWidget *fontchooser = data;
  gboolean tweak = g_variant_get_boolean (state);

  if (tweak)
    {
      bobgui_entry_grab_focus_without_selecting (BOBGUI_ENTRY (fontchooser->preview2));
      bobgui_stack_set_visible_child_name (BOBGUI_STACK (fontchooser->stack), "tweaks");
    }
  else
    {
      bobgui_widget_grab_focus (fontchooser->search_entry);
      bobgui_stack_set_visible_child_name (BOBGUI_STACK (fontchooser->stack), "list");
    }

  g_simple_action_set_state (action, state);
}

typedef struct {
  guint32 tag;
  float default_value;
  BobguiAdjustment *adjustment;
  BobguiWidget *label;
  BobguiWidget *scale;
  BobguiWidget *spin;
  BobguiWidget *fontchooser;
} Axis;

static guint
axis_hash (gconstpointer v)
{
  const Axis *a = v;

  return a->tag;
}

static gboolean
axis_equal (gconstpointer v1, gconstpointer v2)
{
  const Axis *a1 = v1;
  const Axis *a2 = v2;

  return a1->tag == a2->tag;
}

static void
axis_remove (gpointer key,
             gpointer value,
             gpointer data)
{
  BobguiFontChooserWidget *fontchooser = data;
  Axis *a = value;

  bobgui_grid_remove (BOBGUI_GRID (fontchooser->axis_grid), a->label);
  bobgui_grid_remove (BOBGUI_GRID (fontchooser->axis_grid), a->scale);
  bobgui_grid_remove (BOBGUI_GRID (fontchooser->axis_grid), a->spin);
}

static void
axis_free (gpointer v)
{
  Axis *a = v;

  g_free (a);
}

static void
select_added (GListModel *model,
              guint       position,
              guint       removed,
              guint       added,
              gpointer    data)
{
  BobguiSingleSelection *selection = BOBGUI_SINGLE_SELECTION (model);

  g_assert (removed == 0);
  g_assert (added == 1);

  bobgui_single_selection_set_selected (selection, position);
}

static void
add_languages_from_font (BobguiFontChooserWidget *self,
                         gpointer              item)
{
  PangoFontFace *face;
  PangoFontDescription *desc;
  PangoFont *font;
  PangoContext *context;
  BobguiSelectionModel *model = bobgui_list_view_get_model (BOBGUI_LIST_VIEW (self->language_list));
  PangoLanguage *default_lang = pango_language_get_default ();
  PangoLanguage **langs;
  int i;

  if (PANGO_IS_FONT_FAMILY (item))
    face = pango_font_family_get_face (PANGO_FONT_FAMILY (item), NULL);
  else
    face = PANGO_FONT_FACE (item);

  if (!face)
    return;

  desc = pango_font_face_describe (face);
  pango_font_description_set_size (desc, 20);

  context = bobgui_widget_get_pango_context (BOBGUI_WIDGET (self));
  font = pango_context_load_font (context, desc);

  langs = pango_font_get_languages (font);
  if (langs)
    {
      for (i = 0; langs[i]; i++)
        {
          if (!g_hash_table_contains (self->language_table, langs[i]))
            {
              g_hash_table_add (self->language_table, langs[i]);
              if (get_language_name (langs[i]))
                {
                  const char *l = pango_language_to_string (langs[i]);
                  gulong id = 0;

                  /* Pre-select the default language */
                  if (pango_language_matches (default_lang, l))
                    id = g_signal_connect (model, "items-changed", G_CALLBACK (select_added), NULL);

                  bobgui_string_list_append (self->languages, l);

                  if (id)
                    g_signal_handler_disconnect (model, id);
                }
            }
        }
    }

  g_object_unref (font);
  pango_font_description_free (desc);
}

/* We incrementally populate our fontlist to prevent blocking
 * the font chooser for a long time with expensive FcFontSort
 * calls in pango for every row in the list).
 */
static gboolean
add_to_fontlist (BobguiWidget     *widget,
                 GdkFrameClock *clock,
                 gpointer       user_data)
{
  BobguiFontChooserWidget *self = BOBGUI_FONT_CHOOSER_WIDGET (widget);
  BobguiSliceListModel *model = user_data;
  GListModel *child_model;
  guint i G_GNUC_UNUSED;
  guint n G_GNUC_UNUSED;

  if (bobgui_filter_list_model_get_model (self->filter_model) != G_LIST_MODEL (model))
    return G_SOURCE_REMOVE;

  child_model = bobgui_slice_list_model_get_model (model);

  n = bobgui_slice_list_model_get_size (model);

  for (i = n; i < n + 10; i++)
    {
      if (bobgui_filter_list_model_get_pending (BOBGUI_FILTER_LIST_MODEL (self->filter_model)) != 0)
        {
          break;
        }
      gpointer item = g_list_model_get_item (child_model, i);
      if (!item)
        break;
      add_languages_from_font (self, item);
      g_object_unref (item);
    }

  n += 10;

  if (n >= g_list_model_get_n_items (child_model))
    n = G_MAXUINT;

  bobgui_slice_list_model_set_size (model, n);

  if (n == G_MAXUINT)
    {
      if (bobgui_single_selection_get_selected (BOBGUI_SINGLE_SELECTION (self->selection)) == BOBGUI_INVALID_LIST_POSITION)
        bobgui_font_chooser_widget_ensure_matching_selection (self);

      return G_SOURCE_REMOVE;
    }

  return G_SOURCE_CONTINUE;
}


static void
update_fontlist (BobguiFontChooserWidget *self)
{
  PangoFontMap *fontmap;
  GListModel *model;
  guint i, n_items;

  fontmap = self->font_map;
  if (!fontmap)
    fontmap = pango_cairo_font_map_get_default ();

  if ((self->level & BOBGUI_FONT_CHOOSER_LEVEL_STYLE) == 0)
    model = g_object_ref (G_LIST_MODEL (fontmap));
  else
    model = G_LIST_MODEL (bobgui_flatten_list_model_new (G_LIST_MODEL (g_object_ref (fontmap))));

  model = G_LIST_MODEL (bobgui_slice_list_model_new (model, 0, 20));
  n_items = g_list_model_get_n_items (model);
  for (i = 0; i < n_items; i++)
    {
      gpointer item = g_list_model_get_item (model, i);
      if (item)
        {
          add_languages_from_font (self, item);
          g_object_unref (item);
        }
    }

  bobgui_widget_add_tick_callback (BOBGUI_WIDGET (self), add_to_fontlist, g_object_ref (model), g_object_unref);

  bobgui_filter_list_model_set_model (self->filter_model, model);
  g_object_unref (model);
}

static void
setup_lang_item (BobguiSignalListItemFactory *factory,
                 gpointer                  item,
                 gpointer                  data)
{
  BobguiWidget *label;

  label = bobgui_label_new (NULL);
  bobgui_label_set_xalign (BOBGUI_LABEL (label), 0);
  bobgui_list_item_set_child (BOBGUI_LIST_ITEM (item), label);
}

static void
bind_lang_item (BobguiSignalListItemFactory *factory,
                gpointer                  item,
                gpointer                  data)
{
  BobguiWidget *label;
  gpointer obj;
  const char *str;
  PangoLanguage *language;
  const char *name;

  obj = bobgui_list_item_get_item (BOBGUI_LIST_ITEM (item));
  if (!obj)
    return;
  str = bobgui_string_object_get_string (BOBGUI_STRING_OBJECT (obj));

  language = pango_language_from_string (str);
  name = get_language_name (language);

  label = bobgui_list_item_get_child (BOBGUI_LIST_ITEM (item));
  bobgui_label_set_label (BOBGUI_LABEL (label), name);
}

static char *
lang_code_to_name (BobguiStringObject *obj)
{
  if (!obj)
    return g_strdup ("");
  const char *code = bobgui_string_object_get_string (obj);
  PangoLanguage *lang = pango_language_from_string (code);
  const char *name = get_language_name (lang);

  if (name)
    return g_strdup (name);

  return g_strdup ("");
}

static gboolean
update_language_filters_idle (gpointer user_data)
{
  BobguiFontChooserWidget *self = user_data;
  const char *text;

  text = bobgui_editable_get_text (BOBGUI_EDITABLE (self->language_search_entry));
  bobgui_string_filter_set_search (self->language_name_filter, text);
  bobgui_string_filter_set_search (self->language_code_filter, text);

  g_object_unref (self);

  return G_SOURCE_REMOVE;
}

static void
on_language_search_changed (BobguiSearchEntry *entry, gpointer user_data)
{
  BobguiFontChooserWidget *self = user_data;
  g_idle_add (update_language_filters_idle, g_object_ref (self));
}

static void
on_language_selection_changed (BobguiSingleSelection *selection,
                               GParamSpec         *pspec,
                               gpointer            user_data)
{
  BobguiFontChooserWidget *self = user_data;
  update_filter_language (self);
}


static void
setup_language_list (BobguiFontChooserWidget *self)
{
  BobguiListItemFactory *factory;
  BobguiExpression *name_expression;
  BobguiExpression *code_expression;
  BobguiSingleSelection *selection;
  BobguiSortListModel *sorter;
  BobguiStringSorter *string_sorter;
  BobguiAnyFilter *any_filter;
  GListModel *model;

  self->languages = bobgui_string_list_new (NULL);
  self->language_table = g_hash_table_new (NULL, NULL);

  name_expression = bobgui_cclosure_expression_new (G_TYPE_STRING, NULL, 0, NULL,
                                                 (GCallback) lang_code_to_name,
                                                 NULL, NULL);

  code_expression = bobgui_property_expression_new (BOBGUI_TYPE_STRING_OBJECT, NULL, "string");

  string_sorter = bobgui_string_sorter_new (bobgui_expression_ref (name_expression));
  sorter = bobgui_sort_list_model_new (G_LIST_MODEL (g_object_ref (self->languages)), BOBGUI_SORTER (string_sorter));
  model = G_LIST_MODEL (sorter);

  self->language_name_filter = bobgui_string_filter_new (name_expression);
  bobgui_string_filter_set_ignore_case (self->language_name_filter, TRUE);
  bobgui_string_filter_set_match_mode (self->language_name_filter, BOBGUI_STRING_FILTER_MATCH_MODE_SUBSTRING);

  self->language_code_filter = bobgui_string_filter_new (code_expression);
  bobgui_string_filter_set_ignore_case (self->language_code_filter, TRUE);
  bobgui_string_filter_set_match_mode (self->language_code_filter, BOBGUI_STRING_FILTER_MATCH_MODE_SUBSTRING);

  any_filter = bobgui_any_filter_new ();
  bobgui_multi_filter_append (BOBGUI_MULTI_FILTER (any_filter), g_object_ref (BOBGUI_FILTER (self->language_name_filter)));
  bobgui_multi_filter_append (BOBGUI_MULTI_FILTER (any_filter), g_object_ref (BOBGUI_FILTER (self->language_code_filter)));

  self->language_filtered_model = bobgui_filter_list_model_new (model, BOBGUI_FILTER (any_filter));
  model = G_LIST_MODEL (self->language_filtered_model);

  selection = bobgui_single_selection_new (model);
  bobgui_single_selection_set_autoselect (selection, FALSE);
  self->language_selection = g_object_ref (selection);
  bobgui_list_view_set_model (BOBGUI_LIST_VIEW (self->language_list), BOBGUI_SELECTION_MODEL (selection));
  g_object_unref (selection);

  g_signal_connect (self->language_selection, "notify::selected-item",
                    G_CALLBACK (on_language_selection_changed), self);

  g_signal_connect (self->language_search_entry, "search-changed",
                    G_CALLBACK (on_language_search_changed), self);

  factory = bobgui_signal_list_item_factory_new ();
  g_signal_connect (factory, "setup", G_CALLBACK (setup_lang_item), self);
  g_signal_connect (factory, "bind", G_CALLBACK (bind_lang_item), self);
  bobgui_list_view_set_factory (BOBGUI_LIST_VIEW (self->language_list), factory);
  g_object_unref (factory);
}

static void
bobgui_font_chooser_widget_init (BobguiFontChooserWidget *self)
{
  bobgui_widget_init_template (BOBGUI_WIDGET (self));

  self->axes = g_hash_table_new_full (axis_hash, axis_equal, NULL, axis_free);

  /* Default preview string  */
  self->preview_text = g_strdup (pango_language_get_sample_string (NULL));
  self->show_preview_entry = TRUE;
  self->font_desc = pango_font_description_new ();
  self->level = BOBGUI_FONT_CHOOSER_LEVEL_FAMILY |
                BOBGUI_FONT_CHOOSER_LEVEL_STYLE |
                BOBGUI_FONT_CHOOSER_LEVEL_SIZE;
  self->language = pango_language_get_default ();

  /* Set default preview text */
  bobgui_editable_set_text (BOBGUI_EDITABLE (self->preview), self->preview_text);

  bobgui_font_chooser_widget_update_preview_attributes (self);

  /* Set the upper values of the spin/scale with G_MAXINT / PANGO_SCALE */
  bobgui_spin_button_set_range (BOBGUI_SPIN_BUTTON (self->size_spin),
			     1.0, (double)(G_MAXINT / PANGO_SCALE));
  bobgui_adjustment_set_upper (bobgui_range_get_adjustment (BOBGUI_RANGE (self->size_slider)),
			    (double)(G_MAXINT / PANGO_SCALE));

  self->tweak_action = G_ACTION (g_simple_action_new_stateful ("tweak", NULL, g_variant_new_boolean (FALSE)));
  g_signal_connect (self->tweak_action, "change-state", G_CALLBACK (change_tweak), self);

  g_object_set (self->language_search_entry, "placeholder-text", C_("Search placeholder for font language", "Language…"), NULL);
  
  setup_language_list (self);
  update_fontlist (self);

  /* Load data and set initial style-dependent parameters */
  bobgui_font_chooser_widget_populate_features (self);

  bobgui_font_chooser_widget_take_font_desc (self, NULL);
}

/**
 * bobgui_font_chooser_widget_new:
 *
 * Creates a new `BobguiFontChooserWidget`.
 *
 * Returns: a new `BobguiFontChooserWidget`
 *
 * Deprecated: 4.10: Direct use of `BobguiFontChooserWidget` is deprecated.
 */
BobguiWidget *
bobgui_font_chooser_widget_new (void)
{
  return g_object_new (BOBGUI_TYPE_FONT_CHOOSER_WIDGET, NULL);
}

static void
bobgui_font_chooser_widget_finalize (GObject *object)
{
  BobguiFontChooserWidget *fontchooser = BOBGUI_FONT_CHOOSER_WIDGET (object);

  if (fontchooser->font_desc)
    pango_font_description_free (fontchooser->font_desc);

  if (fontchooser->filter_data_destroy)
    fontchooser->filter_data_destroy (fontchooser->filter_data);

  g_free (fontchooser->preview_text);

  g_clear_object (&fontchooser->font_map);

  g_object_unref (fontchooser->tweak_action);

  g_list_free_full (fontchooser->feature_items, g_free);

  g_hash_table_unref (fontchooser->axes);

  g_free (fontchooser->font_features);

  g_clear_object (&fontchooser->filter);

  G_OBJECT_CLASS (bobgui_font_chooser_widget_parent_class)->finalize (object);
}

static gboolean
my_pango_font_family_equal (const char *familya,
                            const char *familyb)
{
  return g_ascii_strcasecmp (familya, familyb) == 0;
}

static void
bobgui_font_chooser_widget_ensure_matching_selection (BobguiFontChooserWidget *self)
{
  const char *desc_family;
  guint i, n;
  gpointer matched_item = NULL;
  
  desc_family = pango_font_description_get_family (self->font_desc);
  if (desc_family == NULL)
    {
      bobgui_single_selection_set_selected (self->selection, BOBGUI_INVALID_LIST_POSITION);
      return;
    }
  
  n = g_list_model_get_n_items (G_LIST_MODEL (self->selection));
  for (i = 0; i < n; i++)
    {
      gpointer item;
      PangoFontFace *face;
      PangoFontFamily *family;
      PangoFontDescription *merged;

      item = g_list_model_get_item (G_LIST_MODEL (self->selection), i);
      if (!item)
        continue;
      
      if (PANGO_IS_FONT_FAMILY (item))
        {
          family = item;
          face = pango_font_family_get_face (family, NULL);
        }
      else
        {
          face = item;
          family = pango_font_face_get_family (face);
        }
      
      if (!face || !my_pango_font_family_equal (desc_family, pango_font_family_get_name (family)))
        {
          g_object_unref (item);
          continue;
        }
      
      merged = pango_font_face_describe (face);
      pango_font_description_merge_static (merged, self->font_desc, FALSE);
      
      if (pango_font_description_equal (merged, self->font_desc))
        {
          pango_font_description_free (merged);
          matched_item = item;
          break;
        }
      
      pango_font_description_free (merged);
      g_object_unref (item);
    }
  
  if (matched_item)
    {
      bobgui_list_view_scroll_to (BOBGUI_LIST_VIEW (self->family_face_list), i,
                               BOBGUI_LIST_SCROLL_SELECT | BOBGUI_LIST_SCROLL_FOCUS, NULL);
      g_object_unref (matched_item);
    }
}


static PangoFontFace *
bobgui_font_chooser_widget_get_face (BobguiFontChooser *chooser)
{
  BobguiFontChooserWidget *self = BOBGUI_FONT_CHOOSER_WIDGET (chooser);
  gpointer item;

  item = bobgui_single_selection_get_selected_item (self->selection);
  if (PANGO_IS_FONT_FAMILY (item))
    return pango_font_family_get_face (item, NULL);
  else
    return item;
}

static PangoFontFamily *
bobgui_font_chooser_widget_get_family (BobguiFontChooser *chooser)
{
  BobguiFontChooserWidget *self = BOBGUI_FONT_CHOOSER_WIDGET (chooser);
  gpointer item;

  item = bobgui_single_selection_get_selected_item (self->selection);
  if (item == NULL)
    return NULL;

  if (PANGO_IS_FONT_FAMILY (item))
    return item;
  else
    return pango_font_face_get_family (item);
}

static int
bobgui_font_chooser_widget_get_size (BobguiFontChooser *chooser)
{
  BobguiFontChooserWidget *fontchooser = BOBGUI_FONT_CHOOSER_WIDGET (chooser);
  PangoFontDescription *desc = bobgui_font_chooser_widget_get_font_desc (fontchooser);

  if (desc)
    return pango_font_description_get_size (desc);

  return -1;
}

static char *
bobgui_font_chooser_widget_get_font (BobguiFontChooserWidget *fontchooser)
{
  PangoFontDescription *desc = bobgui_font_chooser_widget_get_font_desc (fontchooser);

  if (desc)
    return pango_font_description_to_string (desc);

  return NULL;
}

static PangoFontDescription *
bobgui_font_chooser_widget_get_font_desc (BobguiFontChooserWidget *self)
{
  if (bobgui_single_selection_get_selected_item (self->selection))
    return self->font_desc;

  return NULL;
}

static void
bobgui_font_chooser_widget_set_font (BobguiFontChooserWidget *fontchooser,
                                  const char           *fontname)
{
  PangoFontDescription *font_desc;

  font_desc = pango_font_description_from_string (fontname);
  bobgui_font_chooser_widget_take_font_desc (fontchooser, font_desc);
}

/* OpenType variations */

static void
add_font_variations (BobguiFontChooserWidget *fontchooser,
                     GString              *s)
{
  GHashTableIter iter;
  Axis *axis;

  g_hash_table_iter_init (&iter, fontchooser->axes);
  while (g_hash_table_iter_next (&iter, (gpointer *)NULL, (gpointer *)&axis))
    {
      double value;
      char buf[128];

      value = bobgui_adjustment_get_value (axis->adjustment);

      if (value == axis->default_value)
        continue;

      hb_variation_to_string (&(hb_variation_t) { axis->tag, value }, buf, sizeof (buf));

      if (s->len > 0)
        g_string_append_c (s, ',');
      g_string_append (s, buf);
    }
}

static void
adjustment_changed (BobguiAdjustment *adjustment,
                    Axis          *axis)
{
  BobguiFontChooserWidget *fontchooser = BOBGUI_FONT_CHOOSER_WIDGET (axis->fontchooser);
  PangoFontDescription *font_desc;
  GString *s;

  fontchooser->updating_variations = TRUE;

  s = g_string_new ("");
  add_font_variations (fontchooser, s);

  font_desc = pango_font_description_new ();
  pango_font_description_set_variations (font_desc, s->str);
  bobgui_font_chooser_widget_take_font_desc (fontchooser, font_desc);

  g_string_free (s, TRUE);

  fontchooser->updating_variations = FALSE;
}

static gboolean
should_show_axis (hb_ot_var_axis_info_t *ax)
{
  return (ax->flags & HB_OT_VAR_AXIS_FLAG_HIDDEN) == 0;
}

static gboolean
is_named_instance (hb_font_t *font)
{
  /* FIXME */
  return FALSE;
}

static struct {
  guint32 tag;
  const char *name;
} axis_names[] = {
  { HB_OT_TAG_VAR_AXIS_WIDTH,        NC_("Font variation axis", "Width") },
  { HB_OT_TAG_VAR_AXIS_WEIGHT,       NC_("Font variation axis", "Weight") },
  { HB_OT_TAG_VAR_AXIS_ITALIC,       NC_("Font variation axis", "Italic") },
  { HB_OT_TAG_VAR_AXIS_SLANT,        NC_("Font variation axis", "Slant") },
  { HB_OT_TAG_VAR_AXIS_OPTICAL_SIZE, NC_("Font variation axis", "Optical Size") },
};

static gboolean
add_axis (BobguiFontChooserWidget  *fontchooser,
          hb_font_t             *hb_font,
          hb_ot_var_axis_info_t *ax,
          int                    value,
          int                    row)
{
  hb_face_t *hb_face;
  Axis *axis;
  const char *name;
  char buffer[20];
  unsigned int buffer_len = 20;
  int i;

  hb_face = hb_font_get_face (hb_font);

  axis = g_new (Axis, 1);
  axis->tag = ax->tag;
  axis->default_value = ax->default_value;
  axis->fontchooser = BOBGUI_WIDGET (fontchooser);

  hb_ot_name_get_utf8 (hb_face, ax->name_id, HB_LANGUAGE_INVALID, &buffer_len, buffer);
  name = buffer;

  for (i = 0; i < G_N_ELEMENTS (axis_names); i++)
    {
      if (axis_names[i].tag == ax->tag)
        {
          name = g_dpgettext2 (NULL, "Font variation axis", axis_names[i].name);
          break;
        }
    }

  axis->label = bobgui_label_new (name);

  bobgui_widget_set_halign (axis->label, BOBGUI_ALIGN_START);
  bobgui_widget_set_valign (axis->label, BOBGUI_ALIGN_BASELINE_FILL);
  bobgui_grid_attach (BOBGUI_GRID (fontchooser->axis_grid), axis->label, 0, row, 1, 1);
  axis->adjustment = bobgui_adjustment_new ((double)value,
                                         (double)ax->min_value,
                                         (double)ax->max_value,
                                         1.0, 10.0, 0.0);
  axis->scale = bobgui_scale_new (BOBGUI_ORIENTATION_HORIZONTAL, axis->adjustment);
  bobgui_accessible_update_relation (BOBGUI_ACCESSIBLE (axis->scale),
                                  BOBGUI_ACCESSIBLE_RELATION_LABELLED_BY, axis->label, NULL,
                                  -1);
  bobgui_scale_add_mark (BOBGUI_SCALE (axis->scale), (double)ax->default_value, BOBGUI_POS_TOP, NULL);
  bobgui_widget_set_valign (axis->scale, BOBGUI_ALIGN_BASELINE_FILL);
  bobgui_widget_set_hexpand (axis->scale, TRUE);
  bobgui_widget_set_size_request (axis->scale, 100, -1);
  bobgui_scale_set_draw_value (BOBGUI_SCALE (axis->scale), FALSE);
  bobgui_grid_attach (BOBGUI_GRID (fontchooser->axis_grid), axis->scale, 1, row, 1, 1);
  axis->spin = bobgui_spin_button_new (axis->adjustment, 0, 0);
  bobgui_accessible_update_relation (BOBGUI_ACCESSIBLE (axis->spin),
                                  BOBGUI_ACCESSIBLE_RELATION_LABELLED_BY, axis->label, NULL,
                                  -1);
  g_signal_connect (axis->spin, "output", G_CALLBACK (output_cb), fontchooser);
  bobgui_widget_set_valign (axis->spin, BOBGUI_ALIGN_BASELINE_FILL);
  bobgui_grid_attach (BOBGUI_GRID (fontchooser->axis_grid), axis->spin, 2, row, 1, 1);

  g_hash_table_add (fontchooser->axes, axis);

  adjustment_changed (axis->adjustment, axis);
  g_signal_connect (axis->adjustment, "value-changed", G_CALLBACK (adjustment_changed), axis);
  if (is_named_instance (hb_font) || !should_show_axis (ax))
    {
      bobgui_widget_set_visible (axis->label, FALSE);
      bobgui_widget_set_visible (axis->scale, FALSE);
      bobgui_widget_set_visible (axis->spin, FALSE);

      return FALSE;
    }

  return TRUE;
}

#if HB_VERSION_ATLEAST (3, 3, 0)
static void
get_axes_and_values (hb_font_t             *font,
                     unsigned int           n_axes,
                     hb_ot_var_axis_info_t *axes,
                     float                 *coords)
{
  const float *dcoords;
  unsigned int length = n_axes;

  hb_ot_var_get_axis_infos (hb_font_get_face (font), 0, &length, axes);

  dcoords = hb_font_get_var_coords_design (font, &length);
  if (dcoords)
    memcpy (coords, dcoords, sizeof (float) * length);
  else
    {
      for (int i = 0; i < n_axes; i++)
        {
          hb_ot_var_axis_info_t *axis = &axes[i];
          coords[axis->axis_index] = axis->default_value;
        }
    }
}

#else

/* FIXME: This doesn't work if the font has an avar table */
static float
denorm_coord (hb_ot_var_axis_info_t *axis, int coord)
{
  float r = coord / 16384.0;

  if (coord < 0)
    return axis->default_value + r * (axis->default_value - axis->min_value);
  else
    return axis->default_value + r * (axis->max_value - axis->default_value);
}

static void
get_axes_and_values (hb_font_t             *font,
                     unsigned int           n_axes,
                     hb_ot_var_axis_info_t *axes,
                     float                 *coords)
{
  const int *ncoords;
  unsigned int length = n_axes;

  hb_ot_var_get_axis_infos (hb_font_get_face (font), 0, &length, axes);

  ncoords = hb_font_get_var_coords_normalized (font, &length);

  for (int i = 0; i < n_axes; i++)
    {
      hb_ot_var_axis_info_t *axis = &axes[i];
      int idx = axis->axis_index;
      if (ncoords)
        coords[idx] = denorm_coord (axis, ncoords[idx]);
      else
        coords[idx] = axis->default_value;
    }
}
#endif

static gboolean
bobgui_font_chooser_widget_update_font_variations (BobguiFontChooserWidget *fontchooser)
{
  PangoFont *pango_font;
  hb_font_t *hb_font;
  hb_face_t *hb_face;
  unsigned int n_axes;
  hb_ot_var_axis_info_t *axes;
  float *coords;
  gboolean has_axis = FALSE;
  int i;

  if (fontchooser->updating_variations)
    return FALSE;

  g_hash_table_foreach (fontchooser->axes, axis_remove, fontchooser);
  g_hash_table_remove_all (fontchooser->axes);

  if ((fontchooser->level & BOBGUI_FONT_CHOOSER_LEVEL_VARIATIONS) == 0)
    return FALSE;

  pango_font = pango_context_load_font (bobgui_widget_get_pango_context (BOBGUI_WIDGET (fontchooser)),
                                        fontchooser->font_desc);
  hb_font = pango_font_get_hb_font (pango_font);
  hb_face = hb_font_get_face (hb_font);

  if (!hb_ot_var_has_data (hb_face))
    return FALSE;

  n_axes = hb_ot_var_get_axis_count (hb_face);
  axes = g_alloca (sizeof (hb_ot_var_axis_info_t) * n_axes);
  coords = g_alloca (sizeof (float) * n_axes);
  get_axes_and_values (hb_font, n_axes, axes, coords);

  for (i = 0; i < n_axes; i++)
    {
      if (add_axis (fontchooser, hb_font, &axes[i], coords[axes[i].axis_index], i + 4))
        has_axis = TRUE;
    }

  g_object_unref (pango_font);

  return has_axis;
}

/* OpenType features */

/* look for a lang / script combination that matches the
 * language property and is supported by the hb_face. If
 * none is found, return the default lang / script tags.
 */
static void
find_language_and_script (BobguiFontChooserWidget *fontchooser,
                          hb_face_t            *hb_face,
                          hb_tag_t             *lang_tag,
                          hb_tag_t             *script_tag)
{
  int i, j, k;
  hb_tag_t scripts[80];
  unsigned int n_scripts;
  unsigned int count;
  hb_tag_t table[2] = { HB_OT_TAG_GSUB, HB_OT_TAG_GPOS };
  hb_language_t lang;
  const char *langname, *p;

  langname = pango_language_to_string (fontchooser->language);
  p = strchr (langname, '-');
  lang = hb_language_from_string (langname, p ? p - langname : -1);

  n_scripts = 0;
  for (i = 0; i < 2; i++)
    {
      count = G_N_ELEMENTS (scripts);
      hb_ot_layout_table_get_script_tags (hb_face, table[i], n_scripts, &count, scripts);
      n_scripts += count;
    }

  for (j = 0; j < n_scripts; j++)
    {
      hb_tag_t languages[80];
      unsigned int n_languages;

      n_languages = 0;
      for (i = 0; i < 2; i++)
        {
          count = G_N_ELEMENTS (languages);
          hb_ot_layout_script_get_language_tags (hb_face, table[i], j, n_languages, &count, languages);
          n_languages += count;
        }

      for (k = 0; k < n_languages; k++)
        {
          if (lang == hb_ot_tag_to_language (languages[k]))
            {
              *script_tag = scripts[j];
              *lang_tag = languages[k];
              return;
            }
        }
    }

  *lang_tag = HB_OT_TAG_DEFAULT_LANGUAGE;
  *script_tag = HB_OT_TAG_DEFAULT_SCRIPT;
}

typedef struct {
  hb_tag_t tag;
  const char *name;
  BobguiWidget *top;
  BobguiWidget *feat;
  BobguiWidget *example;
} FeatureItem;

static char *
get_feature_display_name (hb_tag_t tag)
{
  int i;
  char buf[5] = { 0, };

  hb_tag_to_string (tag, buf);

  if (buf[0] == 's' && buf[1] == 's' && g_ascii_isdigit (buf[2]) && g_ascii_isdigit (buf[3]))
    {
      int num = (buf[2] - '0') * 10 + (buf[3] - '0');
      return g_strdup_printf (g_dpgettext2 (NULL, "OpenType layout", "Stylistic Set %d"), num);
    }

  if (buf[0] == 'c' && buf[1] == 'v' && g_ascii_isdigit (buf[2]) && g_ascii_isdigit (buf[3]))
    {
      int num = (buf[2] - '0') * 10 + (buf[3] - '0');
      return g_strdup_printf (g_dpgettext2 (NULL, "OpenType layout", "Character Variant %d"), num);
    }

  for (i = 0; i < G_N_ELEMENTS (open_type_layout_features); i++)
    {
      if (tag == open_type_layout_features[i].tag)
        return g_strdup (g_dpgettext2 (NULL, "OpenType layout", open_type_layout_features[i].name));
    }

  return NULL;
}

static void
set_inconsistent (BobguiCheckButton *button,
                  gboolean        inconsistent)
{
  bobgui_check_button_set_inconsistent (BOBGUI_CHECK_BUTTON (button), inconsistent);
  bobgui_widget_set_opacity (bobgui_widget_get_first_child (BOBGUI_WIDGET (button)), inconsistent ? 0.0 : 1.0);
}

static void
feat_pressed (BobguiGestureClick *gesture,
              int              n_press,
              double           x,
              double           y,
              BobguiWidget       *feat)
{
  const guint button = bobgui_gesture_single_get_current_button (BOBGUI_GESTURE_SINGLE (gesture));

  if (button == GDK_BUTTON_PRIMARY)
    {
      g_signal_handlers_block_by_func (feat, feat_pressed, NULL);

      if (bobgui_check_button_get_inconsistent (BOBGUI_CHECK_BUTTON (feat)))
        {
          set_inconsistent (BOBGUI_CHECK_BUTTON (feat), FALSE);
          bobgui_check_button_set_active (BOBGUI_CHECK_BUTTON (feat), TRUE);
        }

      g_signal_handlers_unblock_by_func (feat, feat_pressed, NULL);
    }
  else if (button == GDK_BUTTON_SECONDARY)
    {
      gboolean inconsistent = bobgui_check_button_get_inconsistent (BOBGUI_CHECK_BUTTON (feat));
      set_inconsistent (BOBGUI_CHECK_BUTTON (feat), !inconsistent);
    }
}

static char *
find_affected_text (BobguiFontChooserWidget *fontchooser,
                    hb_tag_t    feature_tag,
                    hb_font_t  *hb_font,
                    hb_tag_t    script_tag,
                    hb_tag_t    lang_tag,
                    int         max_chars)
{
  hb_face_t *hb_face;
  unsigned int script_index = 0;
  unsigned int lang_index = 0;
  unsigned int feature_index = 0;
  GString *chars;

  hb_face = hb_font_get_face (hb_font);

  chars = g_string_new ("");

  hb_ot_layout_table_find_script (hb_face, HB_OT_TAG_GSUB, script_tag, &script_index);

  hb_ot_layout_script_select_language (hb_face, HB_OT_TAG_GSUB, script_index, 1, &lang_tag, &lang_index);

  if (hb_ot_layout_language_find_feature (hb_face,
                                          HB_OT_TAG_GSUB,
                                          script_index,
                                          lang_index,
                                          feature_tag,
                                          &feature_index))
    {
      unsigned int lookup_indexes[32];
      unsigned int lookup_count = 32;
      unsigned int count;
      int n_chars = 0;

      count = hb_ot_layout_feature_get_characters (hb_face,
                                                   HB_OT_TAG_GSUB,
                                                   feature_index,
                                                   0,
                                                   NULL, NULL);
      if (count > 0)
        {
          hb_codepoint_t *ch;

          ch = g_alloca (sizeof (hb_codepoint_t) * count);
          hb_ot_layout_feature_get_characters (hb_face,
                                               HB_OT_TAG_GSUB,
                                               feature_index,
                                               0,
                                               &count,
                                               ch);

          for (unsigned int i = 0; i < MIN (count, max_chars); i++)
            g_string_append_unichar (chars, ch[i]);
        }
      else
        {
          count  = hb_ot_layout_feature_get_lookups (hb_face,
                                                     HB_OT_TAG_GSUB,
                                                     feature_index,
                                                     0,
                                                     &lookup_count,
                                                     lookup_indexes);
          if (count > 0)
            {
              hb_set_t *glyphs_before = NULL;
              hb_set_t *glyphs_after = NULL;
              hb_set_t *glyphs_output = NULL;
              hb_set_t *glyphs_input;
              hb_codepoint_t gid;
              char buf[5] = { 0, };

              hb_tag_to_string (feature_tag, buf);

              glyphs_input = hb_set_create ();

              for (int i = 0; i < count; i++)
                {
                  hb_ot_layout_lookup_collect_glyphs (hb_face,
                                                      HB_OT_TAG_GSUB,
                                                      lookup_indexes[i],
                                                      glyphs_before,
                                                      glyphs_input,
                                                      glyphs_after,
                                                      glyphs_output);
                }

              if (!fontchooser->glyphmap)
                {
                  fontchooser->glyphmap = hb_map_create ();
                  for (hb_codepoint_t ch = 0; ch < 0xffff; ch++)
                    {
                      hb_codepoint_t glyph = 0;
                      if (hb_font_get_nominal_glyph (hb_font, ch, &glyph) &&
                         !hb_map_has (fontchooser->glyphmap, glyph))
                        hb_map_set (fontchooser->glyphmap, glyph, ch);
                    }
                }

              gid = HB_SET_VALUE_INVALID;
              while (hb_set_next (glyphs_input, &gid))
                {
                  hb_codepoint_t ch;

                  if (n_chars == max_chars)
                    {
                      g_string_append (chars, "…");
                      break;
                    }
                  ch = hb_map_get (fontchooser->glyphmap, gid);
                  if (ch != HB_MAP_VALUE_INVALID)
                    {
                      g_string_append_unichar (chars, (gunichar)ch);
                      n_chars++;
                    }
                }

              hb_set_destroy (glyphs_input);
            }
        }
    }

  return g_string_free (chars, FALSE);
}

static char *
get_name (hb_face_t       *hb_face,
          hb_ot_name_id_t  id)
{
  unsigned int len;
  char *name;

  len = hb_ot_name_get_utf8 (hb_face, id, HB_LANGUAGE_INVALID, NULL, NULL);
  len++;
  name = g_new (char, len);
  hb_ot_name_get_utf8 (hb_face, id, HB_LANGUAGE_INVALID, &len, name);

  return name;
}

static void
update_feature_label (BobguiFontChooserWidget *fontchooser,
                      FeatureItem          *item,
                      hb_font_t            *hb_font,
                      hb_tag_t              script_tag,
                      hb_tag_t              lang_tag)
{
  hb_face_t *hb_face;
  unsigned int script_index, lang_index, feature_index;
  hb_ot_name_id_t label_id, first_param_id;
  unsigned int num_params;
  const char *feat[] = { "salt", "swsh", "nalt", NULL };

  hb_face = hb_font_get_face (hb_font);

  if (!g_strv_contains (feat, item->name) &&
      (!(g_str_has_prefix (item->name, "ss") || g_str_has_prefix (item->name, "cv")) ||
       !g_ascii_isdigit (item->name[2]) || !g_ascii_isdigit (item->name[3])))
    return;

  hb_ot_layout_table_find_script (hb_face, HB_OT_TAG_GSUB, script_tag, &script_index);

  hb_ot_layout_script_select_language (hb_face, HB_OT_TAG_GSUB, script_index, 1, &lang_tag, &lang_index);

  if (hb_ot_layout_language_find_feature (hb_face, HB_OT_TAG_GSUB, script_index, lang_index, item->tag, &feature_index))
    {
      char *label;

      if (!hb_ot_layout_feature_get_name_ids (hb_face, HB_OT_TAG_GSUB, feature_index, &label_id, NULL, NULL, &num_params, &first_param_id))
        {
          label_id = HB_OT_NAME_ID_INVALID;
          num_params = 0;
        }

      if (label_id != HB_OT_NAME_ID_INVALID)
        label = get_name (hb_face, label_id);
      else
        label = get_feature_display_name (item->tag);

      if (BOBGUI_IS_CHECK_BUTTON (item->feat))
        bobgui_check_button_set_label (BOBGUI_CHECK_BUTTON (item->feat), label);
      else
        {
          BobguiWidget *l = bobgui_widget_get_prev_sibling (item->feat);
          bobgui_label_set_label (BOBGUI_LABEL (l), label);
        }

      g_free (label);

      if (BOBGUI_IS_DROP_DOWN (item->feat))
        {
          unsigned int n_lookups;
          BobguiStringList *strings;
          unsigned int *lookups;
          unsigned int n_alternates;

          n_lookups = hb_ot_layout_feature_get_lookups (hb_face,
                                                        HB_OT_TAG_GSUB,
                                                        feature_index,
                                                        0,
                                                        NULL, NULL);

          lookups = g_alloca (sizeof (unsigned int) * n_lookups);
          hb_ot_layout_feature_get_lookups (hb_face,
                                            HB_OT_TAG_GSUB,
                                            feature_index,
                                            0,
                                            &n_lookups,
                                            lookups);

          n_alternates = 0;
          for (unsigned int l = 0; l < n_lookups; l++)
            {
              hb_set_t *glyphs;
              hb_codepoint_t glyph_index;
              unsigned int lookup = lookups[l];

              glyphs = hb_set_create ();

              hb_ot_layout_lookup_collect_glyphs (hb_face,
                                                  HB_OT_TAG_GSUB,
                                                  lookup,
                                                  NULL,
                                                  glyphs,
                                                  NULL, NULL);

              glyph_index = HB_SET_VALUE_INVALID;
              while (hb_set_next (glyphs, &glyph_index))
                n_alternates = MAX (n_alternates,
                                    hb_ot_layout_lookup_get_glyph_alternates (hb_face,
                                                                              lookup,
                                                                              glyph_index,
                                                                              0, NULL, NULL));

              hb_set_destroy (glyphs);
            }

          strings = bobgui_string_list_new (NULL);
          bobgui_string_list_append (strings, C_("Font feature value", "Default"));
          for (unsigned int i = 0; i < num_params; i++)
            {
              char *name;
              name = get_name (hb_face, first_param_id + i);
              bobgui_string_list_append (strings, name);
              g_free (name);
            }

          for (int i = num_params; i < n_alternates; i++)
            {
              char buf[64];
              g_snprintf (buf, sizeof (buf), "%d", i + 1);
              bobgui_string_list_append (strings, buf);
            }

          if (g_list_model_get_n_items (G_LIST_MODEL (strings)) == 1)
            bobgui_string_list_append (strings, C_("Font feature value", "Enable"));

          bobgui_drop_down_set_model (BOBGUI_DROP_DOWN (item->feat), G_LIST_MODEL (strings));
          g_object_unref (strings);
        }
    }
  else
    {
      char *label = get_feature_display_name (item->tag);
      bobgui_check_button_set_label (BOBGUI_CHECK_BUTTON (item->feat), label);
      g_free (label);
    }
}

static void
update_feature_example (BobguiFontChooserWidget *fontchooser,
                        FeatureItem          *item,
                        hb_font_t            *hb_font,
                        hb_tag_t              script_tag,
                        hb_tag_t              lang_tag,
                        PangoFontDescription *font_desc)
{
  const char *letter_case[] = { "smcp", "c2sc", "pcap", "c2pc", "unic", "cpsp", "case", NULL };
  const char *number_case[] = { "xxnc", "lnum", "onum", NULL };
  const char *number_spacing[] = { "xxns", "pnum", "tnum", NULL };
  const char *fraction[] = { "xxnf", "frac", "afrc", NULL };
  const char *char_variants[] = {
    "zero", "sinf", "nalt",
    "swsh", "cswh", "calt", "falt", "hist", "salt", "jalt", "titl", "rand",
    "ss01", "ss02", "ss03", "ss04", "ss05", "ss06", "ss07", "ss08", "ss09", "ss10",
    "ss11", "ss12", "ss13", "ss14", "ss15", "ss16", "ss17", "ss18", "ss19", "ss20",
    "cv01", "cv02", "cv03", "cv04", "cv05", "cv06", "cv07", "cv08", "cv09", "cv10",
    "cv11", "cv12", "cv13", "cv14", "cv15", "cv16", "cv17", "cv18", "cv19", "cv20",
    "cv21", "cv22", "cv23", "cv24", "cv25", "cv26", "cv27", "cv28", "cv29", "cv30",
    "cv31", "cv32", "cv33", "cv34", "cv35", "cv36", "cv37", "cv38", "cv39", "cv40",
    "cv41", "cv42", "cv43", "cv44", "cv45", "cv46", "cv47", "cv48", "cv49", "cv50",
    "cv51", "cv52", "cv53", "cv54", "cv55", "cv56", "cv57", "cv58", "cv59", "cv60",
    "cv61", "cv62", "cv63", "cv64", "cv65", "cv66", "cv67", "cv68", "cv69", "cv70",
    "cv71", "cv72", "cv73", "cv74", "cv75", "cv76", "cv77", "cv78", "cv79", "cv80",
    "cv81", "cv82", "cv83", "cv84", "cv85", "cv86", "cv87", "cv88", "cv89", "cv90",
    "cv91", "cv92", "cv93", "cv94", "cv95", "cv96", "cv97", "cv98", "cv99",
    NULL };

  if (g_strv_contains (number_case, item->name) ||
      g_strv_contains (number_spacing, item->name) ||
      g_strv_contains (fraction, item->name))
    {
      PangoAttrList *attrs;
      PangoAttribute *attr;
      PangoFontDescription *desc;
      char *str;

      attrs = pango_attr_list_new ();

      desc = pango_font_description_copy (font_desc);
      pango_font_description_unset_fields (desc, PANGO_FONT_MASK_SIZE);
      pango_attr_list_insert (attrs, pango_attr_font_desc_new (desc));
      pango_font_description_free (desc);
      str = g_strconcat (item->name, " 1", NULL);
      attr = pango_attr_font_features_new (str);
      pango_attr_list_insert (attrs, attr);

      if (g_strv_contains (fraction, item->name))
        bobgui_label_set_text (BOBGUI_LABEL (item->example), "1/2 2/3 7/8");
      else
        bobgui_label_set_text (BOBGUI_LABEL (item->example), "0123456789");
      bobgui_label_set_attributes (BOBGUI_LABEL (item->example), attrs);

      pango_attr_list_unref (attrs);
    }
  else if (g_strv_contains (letter_case, item->name) ||
           g_strv_contains (char_variants, item->name))
    {
      char *input = NULL;
      char *text;

      if (strcmp (item->name, "case") == 0)
        input = g_strdup ("A-B[Cq]");
      else if (g_strv_contains (letter_case, item->name))
        input = g_strdup ("AaBbCc…");
      else if (strcmp (item->name, "zero") == 0)
        input = g_strdup ("0");
      else if (strcmp (item->name, "sinf") == 0)
        input = g_strdup ("H2O");
      else
        input = find_affected_text (fontchooser, item->tag, hb_font, script_tag, lang_tag, 10);

      if (input[0] != '\0')
        {
          PangoAttrList *attrs;
          PangoAttribute *attr;
          PangoFontDescription *desc;
          char *str;

          text = g_strconcat (input, " → ", input, NULL);

          attrs = pango_attr_list_new ();

          desc = pango_font_description_copy (font_desc);
          pango_font_description_unset_fields (desc, PANGO_FONT_MASK_SIZE);
          pango_attr_list_insert (attrs, pango_attr_font_desc_new (desc));
          pango_font_description_free (desc);
          str = g_strconcat (item->name, " 0", NULL);
          attr = pango_attr_font_features_new (str);
          attr->start_index = 0;
          attr->end_index = strlen (input);
          pango_attr_list_insert (attrs, attr);
          attr = pango_attr_fallback_new (FALSE);
          attr->start_index = 0;
          attr->end_index = strlen (input);
          pango_attr_list_insert (attrs, attr);
          g_free (str);
          str = g_strconcat (item->name, " 1", NULL);
          attr = pango_attr_font_features_new (str);
          attr->start_index = strlen (input) + strlen (" → ");
          attr->end_index = attr->start_index + strlen (input);
          pango_attr_list_insert (attrs, attr);
          attr = pango_attr_fallback_new (FALSE);
          attr->start_index = strlen (input) + strlen (" → ");
          attr->end_index = attr->start_index + strlen (input);
          pango_attr_list_insert (attrs, attr);
          g_free (str);

          bobgui_label_set_text (BOBGUI_LABEL (item->example), text);
          bobgui_label_set_attributes (BOBGUI_LABEL (item->example), attrs);
          bobgui_label_set_ellipsize (BOBGUI_LABEL (item->example), PANGO_ELLIPSIZE_END);

          g_free (text);
          pango_attr_list_unref (attrs);
        }
      else
        bobgui_label_set_markup (BOBGUI_LABEL (item->example), "");
      g_free (input);
    }
}

static void
font_feature_toggled_cb (BobguiCheckButton *check_button,
                         gpointer        user_data)
{
  BobguiFontChooserWidget *fontchooser = user_data;

  set_inconsistent (check_button, FALSE);
  update_font_features (fontchooser);
}

static void
add_check_group (BobguiFontChooserWidget  *fontchooser,
                 const char            *title,
                 const char           **tags,
                 unsigned int           n_tags)
{
  BobguiWidget *label;
  BobguiWidget *group;
  PangoAttrList *attrs;
  int i;

  group = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
  bobgui_widget_set_halign (group, BOBGUI_ALIGN_FILL);

  label = bobgui_label_new (title);
  bobgui_label_set_xalign (BOBGUI_LABEL (label), 0.0);
  bobgui_widget_set_halign (label, BOBGUI_ALIGN_START);
  g_object_set (label, "margin-top", 10, "margin-bottom", 10, NULL);
  attrs = pango_attr_list_new ();
  pango_attr_list_insert (attrs, pango_attr_weight_new (PANGO_WEIGHT_BOLD));
  bobgui_label_set_attributes (BOBGUI_LABEL (label), attrs);
  pango_attr_list_unref (attrs);
  bobgui_box_append (BOBGUI_BOX (group), label);

  for (i = 0; i < n_tags; i++)
    {
      hb_tag_t tag;
      BobguiWidget *feat;
      FeatureItem *item;
      BobguiGesture *gesture;
      BobguiWidget *box;
      BobguiWidget *example;
      char *name;

      tag = hb_tag_from_string (tags[i], -1);

      name = get_feature_display_name (tag);
      feat = bobgui_check_button_new_with_label (name);
      g_free (name);
      set_inconsistent (BOBGUI_CHECK_BUTTON (feat), TRUE);
      g_signal_connect (feat, "toggled", G_CALLBACK (font_feature_toggled_cb), fontchooser);
      g_signal_connect_swapped (feat, "notify::inconsistent", G_CALLBACK (update_font_features), fontchooser);

      gesture = bobgui_gesture_click_new ();
      bobgui_gesture_single_set_button (BOBGUI_GESTURE_SINGLE (gesture), GDK_BUTTON_SECONDARY);
      g_signal_connect (gesture, "pressed", G_CALLBACK (feat_pressed), feat);
      bobgui_widget_add_controller (feat, BOBGUI_EVENT_CONTROLLER (gesture));

      example = bobgui_label_new ("");
      bobgui_label_set_selectable (BOBGUI_LABEL (example), TRUE);
      bobgui_widget_set_halign (example, BOBGUI_ALIGN_START);

      box = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 10);
      bobgui_box_set_homogeneous (BOBGUI_BOX (box), TRUE);
      bobgui_box_append (BOBGUI_BOX (box), feat);
      bobgui_box_append (BOBGUI_BOX (box), example);
      bobgui_box_append (BOBGUI_BOX (group), box);

      item = g_new (FeatureItem, 1);
      item->name = tags[i];
      item->tag = tag;
      item->top = box;
      item->feat = feat;
      item->example = example;

      fontchooser->feature_items = g_list_prepend (fontchooser->feature_items, item);
    }

  bobgui_box_append (BOBGUI_BOX (fontchooser->feature_box), group);
}

static void
font_enum_feature_changed_cb (GObject    *object,
                              GParamSpec *pspec,
                              gpointer    data)
{
  BobguiFontChooserWidget *fontchooser = data;

  update_font_features (fontchooser);
}

static void
add_enum_group (BobguiFontChooserWidget  *fontchooser,
                const char            *title,
                const char           **tags,
                unsigned int           n_tags)
{
  BobguiWidget *label = NULL;
  BobguiWidget *group;
  PangoAttrList *attrs;
  int i;

  group = bobgui_grid_new ();
  bobgui_grid_set_row_spacing (BOBGUI_GRID (group), 6);
  bobgui_grid_set_column_spacing (BOBGUI_GRID (group), 12);

  if (title)
    {
      label = bobgui_label_new (title);
      bobgui_label_set_xalign (BOBGUI_LABEL (label), 0.0);
      bobgui_widget_set_halign (label, BOBGUI_ALIGN_START);
      g_object_set (label, "margin-top", 10, "margin-bottom", 10, NULL);
      attrs = pango_attr_list_new ();
      pango_attr_list_insert (attrs, pango_attr_weight_new (PANGO_WEIGHT_BOLD));
      bobgui_label_set_attributes (BOBGUI_LABEL (label), attrs);
      pango_attr_list_unref (attrs);
      bobgui_grid_attach (BOBGUI_GRID (group), label, 0, -1, 3, 1);
    }

  for (i = 0; i < n_tags; i++)
    {
      hb_tag_t tag;
      BobguiWidget *feat;
      FeatureItem *item;
      BobguiWidget *example;
      char *name;

      tag = hb_tag_from_string (tags[i], -1);

      name = get_feature_display_name (tag);
      label = bobgui_label_new (name);
      bobgui_label_set_xalign (BOBGUI_LABEL (label), 0);
      bobgui_grid_attach (BOBGUI_GRID (group), label, 0, i, 1, 1);
      g_free (name);

      feat = bobgui_drop_down_new (NULL, NULL);
      bobgui_grid_attach (BOBGUI_GRID (group), feat, 1, i, 1, 1);

      bobgui_label_set_mnemonic_widget (BOBGUI_LABEL (label), feat);

      g_signal_connect (feat, "notify::selected", G_CALLBACK (font_enum_feature_changed_cb), fontchooser);

      example = bobgui_label_new ("");
      bobgui_label_set_selectable (BOBGUI_LABEL (example), TRUE);
      bobgui_widget_set_halign (example, BOBGUI_ALIGN_START);

      bobgui_grid_attach (BOBGUI_GRID (group), example, 2, i, 1, 1);

      item = g_new (FeatureItem, 1);
      item->name = tags[i];
      item->tag = tag;
      item->top = NULL;
      item->feat = feat;
      item->example = example;

      fontchooser->feature_items = g_list_prepend (fontchooser->feature_items, item);
    }

  bobgui_box_append (BOBGUI_BOX (fontchooser->feature_box), group);
}

static void
add_radio_group (BobguiFontChooserWidget  *fontchooser,
                 const char            *title,
                 const char           **tags,
                 unsigned int           n_tags)
{
  BobguiWidget *label;
  BobguiWidget *group;
  int i;
  BobguiWidget *group_button = NULL;
  PangoAttrList *attrs;

  group = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
  bobgui_widget_set_halign (group, BOBGUI_ALIGN_FILL);

  label = bobgui_label_new (title);
  bobgui_label_set_xalign (BOBGUI_LABEL (label), 0.0);
  bobgui_widget_set_halign (label, BOBGUI_ALIGN_START);
  g_object_set (label, "margin-top", 10, "margin-bottom", 10, NULL);
  attrs = pango_attr_list_new ();
  pango_attr_list_insert (attrs, pango_attr_weight_new (PANGO_WEIGHT_BOLD));
  bobgui_label_set_attributes (BOBGUI_LABEL (label), attrs);
  pango_attr_list_unref (attrs);
  bobgui_box_append (BOBGUI_BOX (group), label);
  bobgui_accessible_update_relation (BOBGUI_ACCESSIBLE (group),
                                  BOBGUI_ACCESSIBLE_RELATION_LABELLED_BY, label, NULL,
                                  -1);

  for (i = 0; i < n_tags; i++)
    {
      hb_tag_t tag;
      BobguiWidget *feat;
      FeatureItem *item;
      char *name;
      BobguiWidget *box;
      BobguiWidget *example;

      tag = hb_tag_from_string (tags[i], -1);
      name = get_feature_display_name (tag);
      feat = bobgui_check_button_new_with_label (name ? name : _("Default"));
      g_free (name);
      if (group_button == NULL)
        group_button = feat;
      else
        bobgui_check_button_set_group (BOBGUI_CHECK_BUTTON (feat), BOBGUI_CHECK_BUTTON (group_button));

      g_signal_connect_swapped (feat, "notify::active", G_CALLBACK (update_font_features), fontchooser);
      g_object_set_data (G_OBJECT (feat), "default", group_button);

      example = bobgui_label_new ("");
      bobgui_label_set_selectable (BOBGUI_LABEL (example), TRUE);
      bobgui_widget_set_halign (example, BOBGUI_ALIGN_START);

      box = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 10);
      bobgui_box_set_homogeneous (BOBGUI_BOX (box), TRUE);
      bobgui_box_append (BOBGUI_BOX (box), feat);
      bobgui_box_append (BOBGUI_BOX (box), example);
      bobgui_box_append (BOBGUI_BOX (group), box);

      item = g_new (FeatureItem, 1);
      item->name = tags[i];
      item->tag = tag;
      item->top = box;
      item->feat = feat;
      item->example = example;

      fontchooser->feature_items = g_list_prepend (fontchooser->feature_items, item);
    }

  bobgui_box_append (BOBGUI_BOX (fontchooser->feature_box), group);
}

static void
bobgui_font_chooser_widget_populate_features (BobguiFontChooserWidget *fontchooser)
{
  const char *ligatures[] = { "liga", "dlig", "hlig", "clig" };
  const char *letter_case[] = { "smcp", "c2sc", "pcap", "c2pc", "unic", "cpsp", "case" };
  const char *number_case[] = { "xxnc", "lnum", "onum" };
  const char *number_spacing[] = { "xxns", "pnum", "tnum" };
  const char *fractions[] = { "xxnf", "frac", "afrc" };
  const char *numeric_extras[] = { "zero", "sinf" };
  const char *style_variants[] = {
    "cswh", "calt", "falt", "hist", "jalt", "titl", "rand",
    "ss01", "ss02", "ss03", "ss04", "ss05", "ss06", "ss07", "ss08", "ss09", "ss10",
    "ss11", "ss12", "ss13", "ss14", "ss15", "ss16", "ss17", "ss18", "ss19", "ss20",
  };
  const char *style_variants2[] = {
    "swsh", "salt", "nalt"
  };
  const char *char_variants[] = {
    "cv01", "cv02", "cv03", "cv04", "cv05", "cv06", "cv07", "cv08", "cv09", "cv10",
    "cv11", "cv12", "cv13", "cv14", "cv15", "cv16", "cv17", "cv18", "cv19", "cv20",
    "cv21", "cv22", "cv23", "cv24", "cv25", "cv26", "cv27", "cv28", "cv29", "cv30",
    "cv31", "cv32", "cv33", "cv34", "cv35", "cv36", "cv37", "cv38", "cv39", "cv40",
    "cv41", "cv42", "cv43", "cv44", "cv45", "cv46", "cv47", "cv48", "cv49", "cv50",
    "cv51", "cv52", "cv53", "cv54", "cv55", "cv56", "cv57", "cv58", "cv59", "cv60",
    "cv61", "cv62", "cv63", "cv64", "cv65", "cv66", "cv67", "cv68", "cv69", "cv70",
    "cv71", "cv72", "cv73", "cv74", "cv75", "cv76", "cv77", "cv78", "cv79", "cv80",
    "cv81", "cv82", "cv83", "cv84", "cv85", "cv86", "cv87", "cv88", "cv89", "cv90",
    "cv91", "cv92", "cv93", "cv94", "cv95", "cv96", "cv97", "cv98", "cv99",
  };

  add_check_group (fontchooser, _("Ligatures"), ligatures, G_N_ELEMENTS (ligatures));
  add_check_group (fontchooser, _("Letter Case"), letter_case, G_N_ELEMENTS (letter_case));
  add_radio_group (fontchooser, _("Number Case"), number_case, G_N_ELEMENTS (number_case));
  add_radio_group (fontchooser, _("Number Spacing"), number_spacing, G_N_ELEMENTS (number_spacing));
  add_check_group (fontchooser, _("Numeric Extras"), numeric_extras, G_N_ELEMENTS (numeric_extras));
  add_radio_group (fontchooser, _("Fractions"), fractions, G_N_ELEMENTS (fractions));
  add_check_group (fontchooser, _("Style Variations"), style_variants, G_N_ELEMENTS (style_variants));
  add_enum_group (fontchooser, NULL, style_variants2, G_N_ELEMENTS (style_variants2));
  add_enum_group (fontchooser, _("Character Variations"), char_variants, G_N_ELEMENTS (char_variants));

  update_font_features (fontchooser);
}

static gboolean
bobgui_font_chooser_widget_update_font_features (BobguiFontChooserWidget *fontchooser)
{
  PangoFont *pango_font;
  hb_font_t *hb_font;
  hb_tag_t script_tag;
  hb_tag_t lang_tag;
  guint script_index = 0;
  guint lang_index = 0;
  int i, j;
  GList *l;
  gboolean has_feature = FALSE;

  for (l = fontchooser->feature_items; l; l = l->next)
    {
      FeatureItem *item = l->data;
      if (item->top)
        {
          bobgui_widget_set_visible (item->top, FALSE);
          bobgui_widget_set_visible (bobgui_widget_get_parent (item->top), FALSE);
        }
      else
        {
          bobgui_widget_set_visible (bobgui_widget_get_parent (item->feat), FALSE);
          bobgui_widget_set_visible (item->feat, FALSE);
          bobgui_widget_set_visible (bobgui_widget_get_prev_sibling (item->feat), FALSE);
          bobgui_widget_set_visible (item->example, FALSE);
        }
    }

  if ((fontchooser->level & BOBGUI_FONT_CHOOSER_LEVEL_FEATURES) == 0)
    return FALSE;

  pango_font = pango_context_load_font (bobgui_widget_get_pango_context (BOBGUI_WIDGET (fontchooser)),
                                        fontchooser->font_desc);
  hb_font = pango_font_get_hb_font (pango_font);

  if (hb_font)
    {
      hb_tag_t table[2] = { HB_OT_TAG_GSUB, HB_OT_TAG_GPOS };
      hb_face_t *hb_face;
      hb_tag_t features[80];
      unsigned int count;
      unsigned int n_features;
      hb_tag_t *feat;

      hb_face = hb_font_get_face (hb_font);

      find_language_and_script (fontchooser, hb_face, &lang_tag, &script_tag);

      n_features = 0;
      for (i = 0; i < G_N_ELEMENTS (table); i++)
        {
          hb_ot_layout_table_find_script (hb_face, table[i], script_tag, &script_index);

          hb_ot_layout_script_select_language (hb_face, table[i], script_index, 1, &lang_tag, &lang_index);

          feat = features + n_features;
          count = G_N_ELEMENTS (features) - n_features;
          hb_ot_layout_language_get_feature_tags (hb_face,
                                                  table[i],
                                                  script_index,
                                                  lang_index,
                                                  n_features,
                                                  &count,
                                                  feat);
          n_features += count;
        }

      for (j = 0; j < n_features; j++)
        {
          for (l = fontchooser->feature_items; l; l = l->next)
            {
              FeatureItem *item = l->data;

              if (strncmp (item->name, "xx", 2) == 0)
                {
                  update_feature_example (fontchooser, item, hb_font, script_tag, lang_tag, fontchooser->font_desc);
                  continue;
                }

              if (item->tag != features[j])
                continue;

              has_feature = TRUE;
              if (item->top)
                {
                  bobgui_widget_set_visible (item->top, TRUE);
                  bobgui_widget_set_visible (bobgui_widget_get_parent (item->top), TRUE);
                }
              else
                {
                  bobgui_widget_set_visible (bobgui_widget_get_parent (item->feat), TRUE);
                  bobgui_widget_set_visible (item->feat, TRUE);
                  bobgui_widget_set_visible (bobgui_widget_get_prev_sibling (item->feat), TRUE);
                  bobgui_widget_set_visible (item->example, TRUE);
                }

              update_feature_label (fontchooser, item, hb_font, script_tag, lang_tag);
              update_feature_example (fontchooser, item, hb_font, script_tag, lang_tag, fontchooser->font_desc);

              if (BOBGUI_IS_CHECK_BUTTON (item->feat))
                {
                  BobguiWidget *def = BOBGUI_WIDGET (g_object_get_data (G_OBJECT (item->feat), "default"));
                  if (def)
                    {
                      bobgui_widget_set_visible (def, TRUE);
                      bobgui_widget_set_visible (bobgui_widget_get_parent (def), TRUE);
                      bobgui_check_button_set_active (BOBGUI_CHECK_BUTTON (def), TRUE);
                    }
                  else
                    set_inconsistent (BOBGUI_CHECK_BUTTON (item->feat), TRUE);
                }
            }
        }

      if (fontchooser->glyphmap)
        {
          hb_map_destroy (fontchooser->glyphmap);
          fontchooser->glyphmap = NULL;
        }
    }

  g_object_unref (pango_font);

  return has_feature;
}

static void
update_font_features (BobguiFontChooserWidget *fontchooser)
{
  GString *s;
  GList *l;
  char buf[128];

  s = g_string_new ("");

  for (l = fontchooser->feature_items; l; l = l->next)
    {
      FeatureItem *item = l->data;

      if (!bobgui_widget_is_sensitive (item->feat))
        continue;

      if (BOBGUI_IS_CHECK_BUTTON (item->feat) && g_object_get_data (G_OBJECT (item->feat), "default"))
        {
          if (bobgui_check_button_get_active (BOBGUI_CHECK_BUTTON (item->feat)) &&
              strncmp (item->name, "xx", 2) != 0)
            {
              hb_feature_to_string (&(hb_feature_t) { item->tag, 1, 0, -1 }, buf, sizeof (buf));
              if (s->len > 0)
                g_string_append_c (s, ',');
              g_string_append (s, buf);
            }
        }
      else if (BOBGUI_IS_CHECK_BUTTON (item->feat))
        {
          guint32 value;

          if (bobgui_check_button_get_inconsistent (BOBGUI_CHECK_BUTTON (item->feat)))
            continue;

          value = bobgui_check_button_get_active (BOBGUI_CHECK_BUTTON (item->feat));
          hb_feature_to_string (&(hb_feature_t) { item->tag, value, 0, -1 }, buf, sizeof (buf));
          if (s->len > 0)
            g_string_append_c (s, ',');
          g_string_append (s, buf);
        }
      else if (BOBGUI_IS_DROP_DOWN (item->feat))
        {
          guint value;

          value = bobgui_drop_down_get_selected (BOBGUI_DROP_DOWN (item->feat));
          if (value == 0 || value == BOBGUI_INVALID_LIST_POSITION)
            continue;

          hb_feature_to_string (&(hb_feature_t) { item->tag, value, 0, -1 }, buf, sizeof (buf));
          if (s->len > 0)
            g_string_append_c (s, ',');
          g_string_append (s, buf);
        }
    }

  if (g_strcmp0 (fontchooser->font_features, s->str) != 0)
    {
      g_free (fontchooser->font_features);
      fontchooser->font_features = g_string_free (s, FALSE);
      g_object_notify (G_OBJECT (fontchooser), "font-features");
    }
  else
    g_string_free (s, TRUE);

  bobgui_font_chooser_widget_update_preview_attributes (fontchooser);
}

static void
bobgui_font_chooser_widget_merge_font_desc (BobguiFontChooserWidget       *fontchooser,
                                         const PangoFontDescription *font_desc)
{
  PangoFontMask mask;

  g_assert (font_desc != NULL);
  /* iter may be NULL if the font doesn't exist on the list */

  mask = pango_font_description_get_set_fields (font_desc);

  /* sucky test, because we can't restrict the comparison to
   * only the parts that actually do get merged */
  if (pango_font_description_equal (font_desc, fontchooser->font_desc))
    return;

  pango_font_description_merge (fontchooser->font_desc, font_desc, TRUE);

  if (mask & PANGO_FONT_MASK_SIZE)
    {
      double font_size = (double) pango_font_description_get_size (fontchooser->font_desc) / PANGO_SCALE;
      /* XXX: This clamps, which can cause it to reloop into here, do we need
       * to block its signal handler?
       */
      bobgui_range_set_value (BOBGUI_RANGE (fontchooser->size_slider), font_size);
      bobgui_spin_button_set_value (BOBGUI_SPIN_BUTTON (fontchooser->size_spin), font_size);
    }
  if (mask & (PANGO_FONT_MASK_FAMILY | PANGO_FONT_MASK_STYLE | PANGO_FONT_MASK_VARIANT |
              PANGO_FONT_MASK_WEIGHT | PANGO_FONT_MASK_STRETCH))
    {
      gboolean has_tweak = FALSE;

      bobgui_font_chooser_widget_update_marks (fontchooser);

      if (bobgui_font_chooser_widget_update_font_features (fontchooser))
        has_tweak = TRUE;
      if (bobgui_font_chooser_widget_update_font_variations (fontchooser))
        has_tweak = TRUE;

      g_simple_action_set_enabled (G_SIMPLE_ACTION (fontchooser->tweak_action), has_tweak);
    }

  if (mask & PANGO_FONT_MASK_VARIATIONS)
    {
      if (pango_font_description_get_variations (fontchooser->font_desc)[0] == '\0')
        pango_font_description_unset_fields (fontchooser->font_desc, PANGO_FONT_MASK_VARIANT);
    }

  bobgui_font_chooser_widget_update_preview_attributes (fontchooser);

  g_object_notify (G_OBJECT (fontchooser), "font");
  g_object_notify (G_OBJECT (fontchooser), "font-desc");
}

static void
bobgui_font_chooser_widget_take_font_desc (BobguiFontChooserWidget *fontchooser,
                                        PangoFontDescription *font_desc)
{
  PangoFontMask mask;

  if (font_desc == NULL)
    font_desc = pango_font_description_from_string (BOBGUI_FONT_CHOOSER_DEFAULT_FONT_NAME);

  mask = pango_font_description_get_set_fields (font_desc);
  bobgui_font_chooser_widget_merge_font_desc (fontchooser, font_desc);

  if (mask & (PANGO_FONT_MASK_FAMILY | PANGO_FONT_MASK_STYLE | PANGO_FONT_MASK_VARIANT |
              PANGO_FONT_MASK_WEIGHT | PANGO_FONT_MASK_STRETCH))
    {
      bobgui_single_selection_set_selected (fontchooser->selection, BOBGUI_INVALID_LIST_POSITION);
      bobgui_font_chooser_widget_ensure_matching_selection (fontchooser);
    }

  pango_font_description_free (font_desc);
}

static const char *
bobgui_font_chooser_widget_get_preview_text (BobguiFontChooserWidget *fontchooser)
{

  return fontchooser->preview_text;
}

static void
bobgui_font_chooser_widget_set_preview_text (BobguiFontChooserWidget *fontchooser,
                                          const char           *text)
{
  if (fontchooser->preview_text == text)
    return;

  g_free (fontchooser->preview_text);
  fontchooser->preview_text = g_strdup (text);

  bobgui_editable_set_text (BOBGUI_EDITABLE (fontchooser->preview), text);

  g_object_notify (G_OBJECT (fontchooser), "preview-text");
}

static gboolean
bobgui_font_chooser_widget_get_show_preview_entry (BobguiFontChooserWidget *fontchooser)
{
  return fontchooser->show_preview_entry;
}

static void
bobgui_font_chooser_widget_set_show_preview_entry (BobguiFontChooserWidget *fontchooser,
                                                gboolean              show_preview_entry)
{
  if (fontchooser->show_preview_entry != show_preview_entry)
    {
      fontchooser->show_preview_entry = show_preview_entry;

      bobgui_widget_set_visible (fontchooser->preview, show_preview_entry);

      g_object_notify (G_OBJECT (fontchooser), "show-preview-entry");
    }
}

static void
bobgui_font_chooser_widget_set_font_map (BobguiFontChooser *chooser,
                                      PangoFontMap   *fontmap)
{
  BobguiFontChooserWidget *fontchooser = BOBGUI_FONT_CHOOSER_WIDGET (chooser);

  if (g_set_object (&fontchooser->font_map, fontmap))
    {
      PangoContext *context;

      if (!fontmap)
        fontmap = pango_cairo_font_map_get_default ();

      context = bobgui_widget_get_pango_context (fontchooser->family_face_list);
      pango_context_set_font_map (context, fontmap);

      context = bobgui_widget_get_pango_context (fontchooser->preview);
      pango_context_set_font_map (context, fontmap);

      update_fontlist (fontchooser);
    }
}

static PangoFontMap *
bobgui_font_chooser_widget_get_font_map (BobguiFontChooser *chooser)
{
  BobguiFontChooserWidget *fontchooser = BOBGUI_FONT_CHOOSER_WIDGET (chooser);

  return fontchooser->font_map;
}

static gboolean
bobgui_font_chooser_widget_filter_cb (gpointer item,
                                   gpointer data)
{
  BobguiFontChooserWidget *self = BOBGUI_FONT_CHOOSER_WIDGET (data);
  PangoFontFamily *family;
  PangoFontFace *face;

  if (PANGO_IS_FONT_FAMILY (item))
    {
      family = item;
      face = pango_font_family_get_face (family, NULL);
    }
  else
    {
      face = item;
      family = pango_font_face_get_family (face);
    }

  return self->filter_func (family, face, self->filter_data);
}

static void
bobgui_font_chooser_widget_set_filter_func (BobguiFontChooser   *chooser,
                                         BobguiFontFilterFunc filter,
                                         gpointer          data,
                                         GDestroyNotify    destroy)
{
  BobguiFontChooserWidget *self = BOBGUI_FONT_CHOOSER_WIDGET (chooser);

  if (self->filter_data_destroy)
    self->filter_data_destroy (self->filter_data);

  self->filter_func = filter;
  self->filter_data = data;
  self->filter_data_destroy = destroy;

  if (filter)
    {
      bobgui_custom_filter_set_filter_func (self->custom_filter,
                                         bobgui_font_chooser_widget_filter_cb,
                                         self,
                                         NULL);
    }
  else
    {
      bobgui_custom_filter_set_filter_func (self->custom_filter, NULL, NULL, NULL);
    }
}

static void
bobgui_font_chooser_widget_set_level (BobguiFontChooserWidget *fontchooser,
                                   BobguiFontChooserLevel   level)
{
  gboolean show_size;

  if (fontchooser->level == level)
    return;

  fontchooser->level = level;

  show_size = (level & BOBGUI_FONT_CHOOSER_LEVEL_SIZE) != 0;

  bobgui_widget_set_visible (fontchooser->size_label, show_size);
  bobgui_widget_set_visible (fontchooser->size_slider, show_size);
  bobgui_widget_set_visible (fontchooser->size_spin, show_size);
  bobgui_widget_set_visible (fontchooser->size_label2, show_size);
  bobgui_widget_set_visible (fontchooser->size_slider2, show_size);
  bobgui_widget_set_visible (fontchooser->size_spin2, show_size);

  update_fontlist (fontchooser);

  g_object_notify (G_OBJECT (fontchooser), "level");
}

static BobguiFontChooserLevel
bobgui_font_chooser_widget_get_level (BobguiFontChooserWidget *fontchooser)
{
  return fontchooser->level;
}

static void
bobgui_font_chooser_widget_set_language (BobguiFontChooserWidget *fontchooser,
                                      const char           *language)
{
  PangoLanguage *lang;

  lang = pango_language_from_string (language);
  if (fontchooser->language == lang)
    return;

  fontchooser->language = lang;

  _bobgui_font_filter_set_language (fontchooser->user_filter, lang);

  g_object_notify (G_OBJECT (fontchooser), "language");

  bobgui_font_chooser_widget_update_preview_attributes (fontchooser);
}

static void
bobgui_font_chooser_widget_iface_init (BobguiFontChooserIface *iface)
{
  iface->get_font_family = bobgui_font_chooser_widget_get_family;
  iface->get_font_face = bobgui_font_chooser_widget_get_face;
  iface->get_font_size = bobgui_font_chooser_widget_get_size;
  iface->set_filter_func = bobgui_font_chooser_widget_set_filter_func;
  iface->set_font_map = bobgui_font_chooser_widget_set_font_map;
  iface->get_font_map = bobgui_font_chooser_widget_get_font_map;
}

GAction *
bobgui_font_chooser_widget_get_tweak_action (BobguiWidget *widget)
{
  BobguiFontChooserWidget *fontchooser = BOBGUI_FONT_CHOOSER_WIDGET (widget);

  return fontchooser->tweak_action;
}

void
bobgui_font_chooser_widget_set_filter (BobguiFontChooserWidget *widget,
                                    BobguiFilter            *filter)
{
  if (widget->filter)
    bobgui_multi_filter_remove (BOBGUI_MULTI_FILTER (widget->multi_filter), 3);
  g_set_object (&widget->filter, filter);
  if (widget->filter)
    bobgui_multi_filter_append (BOBGUI_MULTI_FILTER (widget->multi_filter), g_object_ref (filter));
}
