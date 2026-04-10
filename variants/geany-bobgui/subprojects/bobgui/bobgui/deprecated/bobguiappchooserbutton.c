/* bobguiappchooserbutton.c: an app-chooser button
 *
 * Copyright (C) 2010 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Cosimo Cecchi <ccecchi@redhat.com>
 */

/**
 * BobguiAppChooserButton:
 *
 * The `BobguiAppChooserButton` lets the user select an application.
 *
 * <picture>
 *   <source srcset="appchooserbutton-dark.png" media="(prefers-color-scheme: dark)">
 *   <img alt="An example BobguiAppChooserButton" src="appchooserbutton.png">
 * </picture>
 *
 * Initially, a `BobguiAppChooserButton` selects the first application
 * in its list, which will either be the most-recently used application
 * or, if [property@Bobgui.AppChooserButton:show-default-item] is %TRUE, the
 * default application.
 *
 * The list of applications shown in a `BobguiAppChooserButton` includes
 * the recommended applications for the given content type. When
 * [property@Bobgui.AppChooserButton:show-default-item] is set, the default
 * application is also included. To let the user chooser other applications,
 * you can set the [property@Bobgui.AppChooserButton:show-dialog-item] property,
 * which allows to open a full [class@Bobgui.AppChooserDialog].
 *
 * It is possible to add custom items to the list, using
 * [method@Bobgui.AppChooserButton.append_custom_item]. These items cause
 * the [signal@Bobgui.AppChooserButton::custom-item-activated] signal to be
 * emitted when they are selected.
 *
 * To track changes in the selected application, use the
 * [signal@Bobgui.AppChooserButton::changed] signal.
 *
 * ## CSS nodes
 *
 * `BobguiAppChooserButton` has a single CSS node with the name “appchooserbutton”.
 *
 * Deprecated: 4.10: The application selection widgets should be
 *   implemented according to the design of each platform and/or
 *   application requiring them.
 */
#include "config.h"

#include "bobguiappchooserbutton.h"

#include "bobguiappchooser.h"
#include "bobguiappchooserdialog.h"
#include "bobguiappchooserprivate.h"
#include "bobguicelllayout.h"
#include "bobguicellrendererpixbuf.h"
#include "bobguicellrenderertext.h"
#include "bobguicombobox.h"
#include "bobguiwidgetprivate.h"
#include "bobguidialog.h"
#include <glib/gi18n-lib.h>
#include "bobguimarshalers.h"
#include "bobguiliststore.h"
#include "bobguiprivate.h"

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

enum {
  PROP_SHOW_DIALOG_ITEM = 1,
  PROP_SHOW_DEFAULT_ITEM,
  PROP_HEADING,
  PROP_MODAL,
  NUM_PROPERTIES,

  PROP_CONTENT_TYPE = NUM_PROPERTIES
};

enum {
  SIGNAL_CHANGED,
  SIGNAL_CUSTOM_ITEM_ACTIVATED,
  ACTIVATE,
  NUM_SIGNALS
};

enum {
  COLUMN_APP_INFO,
  COLUMN_NAME,
  COLUMN_LABEL,
  COLUMN_ICON,
  COLUMN_CUSTOM,
  COLUMN_SEPARATOR,
  NUM_COLUMNS,
};

#define CUSTOM_ITEM_OTHER_APP "bobgui-internal-item-other-app"

static void app_chooser_iface_init  (BobguiAppChooserIface *iface);

static void real_insert_custom_item (BobguiAppChooserButton *self,
                                     const char          *name,
                                     const char          *label,
                                     GIcon               *icon,
                                     gboolean             custom,
                                     BobguiTreeIter         *iter);

static void real_insert_separator   (BobguiAppChooserButton *self,
                                     gboolean             custom,
                                     BobguiTreeIter         *iter);

static guint signals[NUM_SIGNALS] = { 0, };
static GParamSpec *properties[NUM_PROPERTIES];

typedef struct _BobguiAppChooserButtonClass   BobguiAppChooserButtonClass;

struct _BobguiAppChooserButton {
  BobguiWidget parent_instance;

  BobguiWidget *combobox;
  BobguiListStore *store;

  char *content_type;
  char *heading;
  int last_active;
  gboolean show_dialog_item;
  gboolean show_default_item;
  gboolean modal;

  GHashTable *custom_item_names;
};

struct _BobguiAppChooserButtonClass {
  BobguiWidgetClass parent_class;

  void (* changed)               (BobguiAppChooserButton *self);
  void (* custom_item_activated) (BobguiAppChooserButton *self,
                                  const char *item_name);
  void (* activate)              (BobguiAppChooserButton *self);
};

G_DEFINE_TYPE_WITH_CODE (BobguiAppChooserButton, bobgui_app_chooser_button, BOBGUI_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_APP_CHOOSER,
                                                app_chooser_iface_init));

static gboolean
row_separator_func (BobguiTreeModel *model,
                    BobguiTreeIter  *iter,
                    gpointer      user_data)
{
  gboolean separator;

  bobgui_tree_model_get (model, iter,
                      COLUMN_SEPARATOR, &separator,
                      -1);

  return separator;
}

static void
get_first_iter (BobguiListStore *store,
                BobguiTreeIter  *iter)
{
  BobguiTreeIter iter2;

  if (!bobgui_tree_model_get_iter_first (BOBGUI_TREE_MODEL (store), iter))
    {
      /* the model is empty, append */
      bobgui_list_store_append (store, iter);
    }
  else
    {
      bobgui_list_store_insert_before (store, &iter2, iter);
      *iter = iter2;
    }
}

typedef struct {
  BobguiAppChooserButton *self;
  GAppInfo *info;
  int active_index;
} SelectAppData;

static void
select_app_data_free (SelectAppData *data)
{
  g_clear_object (&data->self);
  g_clear_object (&data->info);

  g_slice_free (SelectAppData, data);
}

static gboolean
select_application_func_cb (BobguiTreeModel *model,
                            BobguiTreePath  *path,
                            BobguiTreeIter  *iter,
                            gpointer      user_data)
{
  SelectAppData *data = user_data;
  BobguiAppChooserButton *self = data->self;
  GAppInfo *app_to_match = data->info;
  GAppInfo *app = NULL;
  gboolean custom;
  gboolean result;

  bobgui_tree_model_get (model, iter,
                      COLUMN_APP_INFO, &app,
                      COLUMN_CUSTOM, &custom,
                      -1);

  /* custom items are always after GAppInfos, so iterating further here
   * is just useless.
   */
  if (custom)
    result = TRUE;
  else if (g_app_info_equal (app, app_to_match))
    {
      bobgui_combo_box_set_active_iter (BOBGUI_COMBO_BOX (self->combobox), iter);
      result = TRUE;
    }
  else
    result = FALSE;

  g_object_unref (app);

  return result;
}

static void
bobgui_app_chooser_button_select_application (BobguiAppChooserButton *self,
                                           GAppInfo            *info)
{
  SelectAppData *data;

  data = g_slice_new0 (SelectAppData);
  data->self = g_object_ref (self);
  data->info = g_object_ref (info);

  bobgui_tree_model_foreach (BOBGUI_TREE_MODEL (self->store),
                          select_application_func_cb, data);

  select_app_data_free (data);
}

static void
other_application_dialog_response_cb (BobguiDialog *dialog,
                                      int        response_id,
                                      gpointer   user_data)
{
  BobguiAppChooserButton *self = user_data;
  GAppInfo *info;

  if (response_id != BOBGUI_RESPONSE_OK)
    {
      /* reset the active item, otherwise we are stuck on
       * 'Other application…'
       */
      bobgui_combo_box_set_active (BOBGUI_COMBO_BOX (self->combobox), self->last_active);
      bobgui_window_destroy (BOBGUI_WINDOW (dialog));
      return;
    }

  info = bobgui_app_chooser_get_app_info (BOBGUI_APP_CHOOSER (dialog));

  bobgui_window_destroy (BOBGUI_WINDOW (dialog));

  /* refresh the combobox to get the new application */
  bobgui_app_chooser_refresh (BOBGUI_APP_CHOOSER (self));
  bobgui_app_chooser_button_select_application (self, info);

  g_object_unref (info);
}

static void
other_application_item_activated_cb (BobguiAppChooserButton *self)
{
  BobguiWidget *dialog, *widget;
  BobguiRoot *root;

  root = bobgui_widget_get_root (BOBGUI_WIDGET (self));
  dialog = bobgui_app_chooser_dialog_new_for_content_type (BOBGUI_WINDOW (root),
                                                        BOBGUI_DIALOG_DESTROY_WITH_PARENT,
                                                        self->content_type);
  bobgui_window_set_modal (BOBGUI_WINDOW (dialog), self->modal | bobgui_window_get_modal (BOBGUI_WINDOW (root)));
  bobgui_app_chooser_dialog_set_heading (BOBGUI_APP_CHOOSER_DIALOG (dialog), self->heading);

  widget = bobgui_app_chooser_dialog_get_widget (BOBGUI_APP_CHOOSER_DIALOG (dialog));
  g_object_set (widget,
                "show-fallback", TRUE,
                "show-other", TRUE,
                NULL);
  bobgui_widget_show (dialog);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (other_application_dialog_response_cb), self);
}

static void
bobgui_app_chooser_button_ensure_dialog_item (BobguiAppChooserButton *self,
                                           BobguiTreeIter         *prev_iter)
{
  BobguiTreeIter iter, iter2;

  if (!self->show_dialog_item || !self->content_type)
    return;

  if (prev_iter == NULL)
    bobgui_list_store_append (self->store, &iter);
  else
    bobgui_list_store_insert_after (self->store, &iter, prev_iter);

  real_insert_separator (self, FALSE, &iter);
  iter2 = iter;

  bobgui_list_store_insert_after (self->store, &iter, &iter2);
  real_insert_custom_item (self, CUSTOM_ITEM_OTHER_APP,
                           _("Other app…"), NULL,
                           FALSE, &iter);
}

static void
insert_one_application (BobguiAppChooserButton *self,
                        GAppInfo            *app,
                        BobguiTreeIter         *iter)
{
  GIcon *icon;

  icon = g_app_info_get_icon (app);

  if (icon == NULL)
    icon = g_themed_icon_new ("application-x-executable");
  else
    g_object_ref (icon);

  bobgui_list_store_set (self->store, iter,
                      COLUMN_APP_INFO, app,
                      COLUMN_LABEL, g_app_info_get_name (app),
                      COLUMN_ICON, icon,
                      COLUMN_CUSTOM, FALSE,
                      -1);

  g_object_unref (icon);
}

static void
bobgui_app_chooser_button_populate (BobguiAppChooserButton *self)
{
  GList *recommended_apps = NULL, *l;
  GAppInfo *app, *default_app = NULL;
  BobguiTreeIter iter, iter2;
  gboolean cycled_recommended;

#ifndef G_OS_WIN32
  if (self->content_type)
    recommended_apps = g_app_info_get_recommended_for_type (self->content_type);
#endif
  cycled_recommended = FALSE;

  if (self->show_default_item)
    {
      if (self->content_type)
        default_app = g_app_info_get_default_for_type (self->content_type, FALSE);

      if (default_app != NULL)
        {
          get_first_iter (self->store, &iter);
          cycled_recommended = TRUE;

          insert_one_application (self, default_app, &iter);

          g_object_unref (default_app);
        }
    }

  for (l = recommended_apps; l != NULL; l = l->next)
    {
      app = l->data;

      if (default_app != NULL && g_app_info_equal (app, default_app))
        continue;

      if (cycled_recommended)
        {
          bobgui_list_store_insert_after (self->store, &iter2, &iter);
          iter = iter2;
        }
      else
        {
          get_first_iter (self->store, &iter);
          cycled_recommended = TRUE;
        }

      insert_one_application (self, app, &iter);
    }

  if (recommended_apps != NULL)
    g_list_free_full (recommended_apps, g_object_unref);

  if (!cycled_recommended)
    bobgui_app_chooser_button_ensure_dialog_item (self, NULL);
  else
    bobgui_app_chooser_button_ensure_dialog_item (self, &iter);

  bobgui_combo_box_set_active (BOBGUI_COMBO_BOX (self->combobox), 0);
}

static void
bobgui_app_chooser_button_build_ui (BobguiAppChooserButton *self)
{
  BobguiCellRenderer *cell;
  BobguiCellArea *area;

  bobgui_combo_box_set_model (BOBGUI_COMBO_BOX (self->combobox),
                           BOBGUI_TREE_MODEL (self->store));

  area = bobgui_cell_layout_get_area (BOBGUI_CELL_LAYOUT (self->combobox));

  bobgui_combo_box_set_row_separator_func (BOBGUI_COMBO_BOX (self->combobox),
                                        row_separator_func, NULL, NULL);

  cell = bobgui_cell_renderer_pixbuf_new ();
  bobgui_cell_area_add_with_properties (area, cell,
                                     "align", FALSE,
                                     "expand", FALSE,
                                     "fixed-size", FALSE,
                                     NULL);
  bobgui_cell_layout_set_attributes (BOBGUI_CELL_LAYOUT (self->combobox), cell,
                                  "gicon", COLUMN_ICON,
                                  NULL);

  cell = bobgui_cell_renderer_text_new ();
  bobgui_cell_area_add_with_properties (area, cell,
                                     "align", FALSE,
                                     "expand", TRUE,
                                     NULL);
  bobgui_cell_layout_set_attributes (BOBGUI_CELL_LAYOUT (self->combobox), cell,
                                  "text", COLUMN_LABEL,
                                  NULL);

  bobgui_app_chooser_button_populate (self);
}

static void
bobgui_app_chooser_button_remove_non_custom (BobguiAppChooserButton *self)
{
  BobguiTreeModel *model;
  BobguiTreeIter iter;
  gboolean custom, res;

  model = BOBGUI_TREE_MODEL (self->store);

  if (!bobgui_tree_model_get_iter_first (model, &iter))
    return;

  do {
    bobgui_tree_model_get (model, &iter,
                        COLUMN_CUSTOM, &custom,
                        -1);
    if (custom)
      res = bobgui_tree_model_iter_next (model, &iter);
    else
      res = bobgui_list_store_remove (BOBGUI_LIST_STORE (model), &iter);
  } while (res);
}

static void
bobgui_app_chooser_button_changed (BobguiComboBox *object,
                                gpointer     user_data)
{
  BobguiAppChooserButton *self = user_data;
  BobguiTreeIter iter;
  char *name = NULL;
  gboolean custom;
  GQuark name_quark;

  if (!bobgui_combo_box_get_active_iter (object, &iter))
    return;

  bobgui_tree_model_get (BOBGUI_TREE_MODEL (self->store), &iter,
                      COLUMN_NAME, &name,
                      COLUMN_CUSTOM, &custom,
                      -1);

  if (name != NULL)
    {
      if (custom)
        {
          name_quark = g_quark_from_string (name);
          g_signal_emit (self, signals[SIGNAL_CUSTOM_ITEM_ACTIVATED], name_quark, name);
          self->last_active = bobgui_combo_box_get_active (object);
        }
      else
        {
          /* trigger the dialog internally */
          other_application_item_activated_cb (self);
        }

      g_free (name);
    }
  else
    self->last_active = bobgui_combo_box_get_active (object);

  g_signal_emit (self, signals[SIGNAL_CHANGED], 0);
}

static void
bobgui_app_chooser_button_refresh (BobguiAppChooser *object)
{
  BobguiAppChooserButton *self = BOBGUI_APP_CHOOSER_BUTTON (object);

  bobgui_app_chooser_button_remove_non_custom (self);
  bobgui_app_chooser_button_populate (self);
}

static GAppInfo *
bobgui_app_chooser_button_get_app_info (BobguiAppChooser *object)
{
  BobguiAppChooserButton *self = BOBGUI_APP_CHOOSER_BUTTON (object);
  BobguiTreeIter iter;
  GAppInfo *info;

  if (!bobgui_combo_box_get_active_iter (BOBGUI_COMBO_BOX (self->combobox), &iter))
    return NULL;

  bobgui_tree_model_get (BOBGUI_TREE_MODEL (self->store), &iter,
                      COLUMN_APP_INFO, &info,
                      -1);

  return info;
}

static void
bobgui_app_chooser_button_constructed (GObject *obj)
{
  BobguiAppChooserButton *self = BOBGUI_APP_CHOOSER_BUTTON (obj);

  if (G_OBJECT_CLASS (bobgui_app_chooser_button_parent_class)->constructed != NULL)
    G_OBJECT_CLASS (bobgui_app_chooser_button_parent_class)->constructed (obj);

  bobgui_app_chooser_button_build_ui (self);
}

static void
bobgui_app_chooser_button_set_property (GObject      *obj,
                                     guint         property_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  BobguiAppChooserButton *self = BOBGUI_APP_CHOOSER_BUTTON (obj);

  switch (property_id)
    {
    case PROP_CONTENT_TYPE:
      self->content_type = g_value_dup_string (value);
      break;
    case PROP_SHOW_DIALOG_ITEM:
      bobgui_app_chooser_button_set_show_dialog_item (self, g_value_get_boolean (value));
      break;
    case PROP_SHOW_DEFAULT_ITEM:
      bobgui_app_chooser_button_set_show_default_item (self, g_value_get_boolean (value));
      break;
    case PROP_HEADING:
      bobgui_app_chooser_button_set_heading (self, g_value_get_string (value));
      break;
    case PROP_MODAL:
      bobgui_app_chooser_button_set_modal (self, g_value_get_boolean (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, property_id, pspec);
      break;
    }
}

static void
bobgui_app_chooser_button_get_property (GObject    *obj,
                                     guint       property_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
  BobguiAppChooserButton *self = BOBGUI_APP_CHOOSER_BUTTON (obj);

  switch (property_id)
    {
    case PROP_CONTENT_TYPE:
      g_value_set_string (value, self->content_type);
      break;
    case PROP_SHOW_DIALOG_ITEM:
      g_value_set_boolean (value, self->show_dialog_item);
      break;
    case PROP_SHOW_DEFAULT_ITEM:
      g_value_set_boolean (value, self->show_default_item);
      break;
    case PROP_HEADING:
      g_value_set_string (value, self->heading);
      break;
    case PROP_MODAL:
      g_value_set_boolean (value, self->modal);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, property_id, pspec);
      break;
    }
}

static void
bobgui_app_chooser_button_finalize (GObject *obj)
{
  BobguiAppChooserButton *self = BOBGUI_APP_CHOOSER_BUTTON (obj);

  g_hash_table_destroy (self->custom_item_names);
  g_free (self->content_type);
  g_free (self->heading);
  g_object_unref (self->store);
  bobgui_widget_unparent (self->combobox);

  G_OBJECT_CLASS (bobgui_app_chooser_button_parent_class)->finalize (obj);
}

static void
bobgui_app_chooser_button_measure (BobguiWidget       *widget,
                                BobguiOrientation  orientation,
                                int             for_size,
                                int            *minimum,
                                int            *natural,
                                int            *minimum_baseline,
                                int            *natural_baseline)
{
  BobguiAppChooserButton *self = BOBGUI_APP_CHOOSER_BUTTON (widget);

  bobgui_widget_measure (self->combobox, orientation, for_size,
                      minimum, natural,
                      minimum_baseline, natural_baseline);
}

static void
bobgui_app_chooser_button_size_allocate (BobguiWidget *widget,
                                      int        width,
                                      int        height,
                                      int        baseline)
{
  BobguiAppChooserButton *self = BOBGUI_APP_CHOOSER_BUTTON (widget);

  bobgui_widget_size_allocate (self->combobox, &(BobguiAllocation){0, 0, width, height}, baseline);
}

static void
app_chooser_iface_init (BobguiAppChooserIface *iface)
{
  iface->get_app_info = bobgui_app_chooser_button_get_app_info;
  iface->refresh = bobgui_app_chooser_button_refresh;
}

static void
bobgui_app_chooser_button_activate (BobguiAppChooserButton *self)
{
  bobgui_widget_activate (self->combobox);
}

static void
bobgui_app_chooser_button_class_init (BobguiAppChooserButtonClass *klass)
{
  GObjectClass *oclass = G_OBJECT_CLASS (klass);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);

  oclass->set_property = bobgui_app_chooser_button_set_property;
  oclass->get_property = bobgui_app_chooser_button_get_property;
  oclass->finalize = bobgui_app_chooser_button_finalize;
  oclass->constructed = bobgui_app_chooser_button_constructed;

  widget_class->measure = bobgui_app_chooser_button_measure;
  widget_class->size_allocate = bobgui_app_chooser_button_size_allocate;
  widget_class->grab_focus = bobgui_widget_grab_focus_child;
  widget_class->focus = bobgui_widget_focus_child;

  klass->activate = bobgui_app_chooser_button_activate;

  g_object_class_override_property (oclass, PROP_CONTENT_TYPE, "content-type");

  /**
   * BobguiAppChooserButton:show-dialog-item:
   *
   * Determines whether the dropdown menu shows an item to open
   * a `BobguiAppChooserDialog`.
   */
  properties[PROP_SHOW_DIALOG_ITEM] =
    g_param_spec_boolean ("show-dialog-item", NULL, NULL,
                          FALSE,
                          G_PARAM_READWRITE|G_PARAM_CONSTRUCT|G_PARAM_STATIC_STRINGS|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiAppChooserButton:show-default-item:
   *
   * Determines whether the dropdown menu shows the default application
   * on top for the provided content type.
   */
  properties[PROP_SHOW_DEFAULT_ITEM] =
    g_param_spec_boolean ("show-default-item", NULL, NULL,
                          FALSE,
                          G_PARAM_READWRITE|G_PARAM_CONSTRUCT|G_PARAM_STATIC_STRINGS|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiAppChooserButton:heading:
   *
   * The text to show at the top of the dialog that can be
   * opened from the button.
   *
   * The string may contain Pango markup.
   */
  properties[PROP_HEADING] =
    g_param_spec_string ("heading", NULL, NULL,
                         NULL,
                         G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiAppChooserButton:modal:
   *
   * Whether the app chooser dialog should be modal.
   */
  properties[PROP_MODAL] =
    g_param_spec_boolean ("modal", NULL, NULL,
                          TRUE,
                          G_PARAM_READWRITE|G_PARAM_CONSTRUCT|G_PARAM_STATIC_STRINGS|G_PARAM_EXPLICIT_NOTIFY);
  g_object_class_install_properties (oclass, NUM_PROPERTIES, properties);

  /**
   * BobguiAppChooserButton::changed:
   * @self: the object which received the signal
   *
   * Emitted when the active application changes.
   */
  signals[SIGNAL_CHANGED] =
    g_signal_new (I_("changed"),
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BobguiAppChooserButtonClass, changed),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);


  /**
   * BobguiAppChooserButton::custom-item-activated:
   * @self: the object which received the signal
   * @item_name: the name of the activated item
   *
   * Emitted when a custom item is activated.
   *
   * Use [method@Bobgui.AppChooserButton.append_custom_item],
   * to add custom items.
   */
  signals[SIGNAL_CUSTOM_ITEM_ACTIVATED] =
    g_signal_new (I_("custom-item-activated"),
                  BOBGUI_TYPE_APP_CHOOSER_BUTTON,
                  G_SIGNAL_RUN_FIRST | G_SIGNAL_DETAILED,
                  G_STRUCT_OFFSET (BobguiAppChooserButtonClass, custom_item_activated),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE,
                  1, G_TYPE_STRING);

  /**
   * BobguiAppChooserButton::activate:
   * @widget: the object which received the signal.
   *
   * Emitted to when the button is activated.
   *
   * The `::activate` signal on `BobguiAppChooserButton` is an action signal and
   * emitting it causes the button to pop up its dialog.
   *
   * Since: 4.4
   */
  signals[ACTIVATE] =
      g_signal_new (I_ ("activate"),
                    G_OBJECT_CLASS_TYPE (oclass),
                    G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                    G_STRUCT_OFFSET (BobguiAppChooserButtonClass, activate),
                    NULL, NULL,
                    NULL,
                    G_TYPE_NONE, 0);

  bobgui_widget_class_set_activate_signal (widget_class, signals[ACTIVATE]);


  bobgui_widget_class_set_css_name (widget_class, I_("appchooserbutton"));
}

static void
bobgui_app_chooser_button_init (BobguiAppChooserButton *self)
{
  self->modal = TRUE;

  self->custom_item_names = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
  self->store = bobgui_list_store_new (NUM_COLUMNS,
                                    G_TYPE_APP_INFO,
                                    G_TYPE_STRING, /* name */
                                    G_TYPE_STRING, /* label */
                                    G_TYPE_ICON,
                                    G_TYPE_BOOLEAN, /* separator */
                                    G_TYPE_BOOLEAN); /* custom */
  self->combobox = bobgui_combo_box_new_with_model (BOBGUI_TREE_MODEL (self->store));
  bobgui_widget_set_parent (self->combobox, BOBGUI_WIDGET (self));

  g_signal_connect (self->combobox, "changed",
                    G_CALLBACK (bobgui_app_chooser_button_changed), self);
}

static gboolean
app_chooser_button_iter_from_custom_name (BobguiAppChooserButton *self,
                                          const char          *name,
                                          BobguiTreeIter         *set_me)
{
  BobguiTreeIter iter;
  char *custom_name = NULL;

  if (!bobgui_tree_model_get_iter_first (BOBGUI_TREE_MODEL (self->store), &iter))
    return FALSE;

  do {
    bobgui_tree_model_get (BOBGUI_TREE_MODEL (self->store), &iter,
                        COLUMN_NAME, &custom_name,
                        -1);

    if (g_strcmp0 (custom_name, name) == 0)
      {
        g_free (custom_name);
        *set_me = iter;

        return TRUE;
      }

    g_free (custom_name);
  } while (bobgui_tree_model_iter_next (BOBGUI_TREE_MODEL (self->store), &iter));

  return FALSE;
}

static void
real_insert_custom_item (BobguiAppChooserButton *self,
                         const char          *name,
                         const char          *label,
                         GIcon               *icon,
                         gboolean             custom,
                         BobguiTreeIter         *iter)
{
  if (custom)
    {
      if (g_hash_table_lookup (self->custom_item_names, name) != NULL)
        {
          g_warning ("Attempting to add custom item %s to BobguiAppChooserButton, "
                     "when there's already an item with the same name", name);
          return;
        }

      g_hash_table_insert (self->custom_item_names,
                           g_strdup (name), GINT_TO_POINTER (1));
    }

  bobgui_list_store_set (self->store, iter,
                      COLUMN_NAME, name,
                      COLUMN_LABEL, label,
                      COLUMN_ICON, icon,
                      COLUMN_CUSTOM, custom,
                      COLUMN_SEPARATOR, FALSE,
                      -1);
}

static void
real_insert_separator (BobguiAppChooserButton *self,
                       gboolean             custom,
                       BobguiTreeIter         *iter)
{
  bobgui_list_store_set (self->store, iter,
                      COLUMN_CUSTOM, custom,
                      COLUMN_SEPARATOR, TRUE,
                      -1);
}

/**
 * bobgui_app_chooser_button_new:
 * @content_type: the content type to show applications for
 *
 * Creates a new `BobguiAppChooserButton` for applications
 * that can handle content of the given type.
 *
 * Returns: a newly created `BobguiAppChooserButton`
 *
 * Deprecated: 4.10: This widget will be removed in BOBGUI 5
 */
BobguiWidget *
bobgui_app_chooser_button_new (const char *content_type)
{
  g_return_val_if_fail (content_type != NULL, NULL);

  return g_object_new (BOBGUI_TYPE_APP_CHOOSER_BUTTON,
                       "content-type", content_type,
                       NULL);
}

/**
 * bobgui_app_chooser_button_append_separator:
 * @self: a `BobguiAppChooserButton`
 *
 * Appends a separator to the list of applications that is shown
 * in the popup.
 *
 * Deprecated: 4.10: This widget will be removed in BOBGUI 5
 */
void
bobgui_app_chooser_button_append_separator (BobguiAppChooserButton *self)
{
  BobguiTreeIter iter;

  g_return_if_fail (BOBGUI_IS_APP_CHOOSER_BUTTON (self));

  bobgui_list_store_append (self->store, &iter);
  real_insert_separator (self, TRUE, &iter);
}

/**
 * bobgui_app_chooser_button_append_custom_item:
 * @self: a `BobguiAppChooserButton`
 * @name: the name of the custom item
 * @label: the label for the custom item
 * @icon: the icon for the custom item
 *
 * Appends a custom item to the list of applications that is shown
 * in the popup.
 *
 * The item name must be unique per-widget. Clients can use the
 * provided name as a detail for the
 * [signal@Bobgui.AppChooserButton::custom-item-activated] signal, to add a
 * callback for the activation of a particular custom item in the list.
 *
 * See also [method@Bobgui.AppChooserButton.append_separator].
 *
 * Deprecated: 4.10: This widget will be removed in BOBGUI 5
 */
void
bobgui_app_chooser_button_append_custom_item (BobguiAppChooserButton *self,
                                           const char          *name,
                                           const char          *label,
                                           GIcon               *icon)
{
  BobguiTreeIter iter;

  g_return_if_fail (BOBGUI_IS_APP_CHOOSER_BUTTON (self));
  g_return_if_fail (name != NULL);

  bobgui_list_store_append (self->store, &iter);
  real_insert_custom_item (self, name, label, icon, TRUE, &iter);
}

/**
 * bobgui_app_chooser_button_set_active_custom_item:
 * @self: a `BobguiAppChooserButton`
 * @name: the name of the custom item
 *
 * Selects a custom item.
 *
 * See [method@Bobgui.AppChooserButton.append_custom_item].
 *
 * Use [method@Bobgui.AppChooser.refresh] to bring the selection
 * to its initial state.
 *
 * Deprecated: 4.10: This widget will be removed in BOBGUI 5
 */
void
bobgui_app_chooser_button_set_active_custom_item (BobguiAppChooserButton *self,
                                               const char          *name)
{
  BobguiTreeIter iter;

  g_return_if_fail (BOBGUI_IS_APP_CHOOSER_BUTTON (self));
  g_return_if_fail (name != NULL);

  if (!g_hash_table_contains (self->custom_item_names, name) ||
      !app_chooser_button_iter_from_custom_name (self, name, &iter))
    {
      g_warning ("Can't find the item named %s in the app chooser.", name);
      return;
    }

  bobgui_combo_box_set_active_iter (BOBGUI_COMBO_BOX (self->combobox), &iter);
}

/**
 * bobgui_app_chooser_button_get_show_dialog_item:
 * @self: a `BobguiAppChooserButton`
 *
 * Returns whether the dropdown menu shows an item
 * for a `BobguiAppChooserDialog`.
 *
 * Returns: the value of [property@Bobgui.AppChooserButton:show-dialog-item]
 *
 * Deprecated: 4.10: This widget will be removed in BOBGUI 5
 */
gboolean
bobgui_app_chooser_button_get_show_dialog_item (BobguiAppChooserButton *self)
{
  g_return_val_if_fail (BOBGUI_IS_APP_CHOOSER_BUTTON (self), FALSE);

  return self->show_dialog_item;
}

/**
 * bobgui_app_chooser_button_set_show_dialog_item:
 * @self: a `BobguiAppChooserButton`
 * @setting: the new value for [property@Bobgui.AppChooserButton:show-dialog-item]
 *
 * Sets whether the dropdown menu of this button should show an
 * entry to trigger a `BobguiAppChooserDialog`.
 *
 * Deprecated: 4.10: This widget will be removed in BOBGUI 5
 */
void
bobgui_app_chooser_button_set_show_dialog_item (BobguiAppChooserButton *self,
                                             gboolean             setting)
{
  if (self->show_dialog_item != setting)
    {
      self->show_dialog_item = setting;

      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_SHOW_DIALOG_ITEM]);

      bobgui_app_chooser_refresh (BOBGUI_APP_CHOOSER (self));
    }
}

/**
 * bobgui_app_chooser_button_get_show_default_item:
 * @self: a `BobguiAppChooserButton`
 *
 * Returns whether the dropdown menu should show the default
 * application at the top.
 *
 * Returns: the value of [property@Bobgui.AppChooserButton:show-default-item]
 *
 * Deprecated: 4.10: This widget will be removed in BOBGUI 5
 */
gboolean
bobgui_app_chooser_button_get_show_default_item (BobguiAppChooserButton *self)
{
  g_return_val_if_fail (BOBGUI_IS_APP_CHOOSER_BUTTON (self), FALSE);

  return self->show_default_item;
}

/**
 * bobgui_app_chooser_button_set_show_default_item:
 * @self: a `BobguiAppChooserButton`
 * @setting: the new value for [property@Bobgui.AppChooserButton:show-default-item]
 *
 * Sets whether the dropdown menu of this button should show the
 * default application for the given content type at top.
 *
 * Deprecated: 4.10: This widget will be removed in BOBGUI 5
 */
void
bobgui_app_chooser_button_set_show_default_item (BobguiAppChooserButton *self,
                                              gboolean             setting)
{
  g_return_if_fail (BOBGUI_IS_APP_CHOOSER_BUTTON (self));

  if (self->show_default_item != setting)
    {
      self->show_default_item = setting;

      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_SHOW_DEFAULT_ITEM]);

      bobgui_app_chooser_refresh (BOBGUI_APP_CHOOSER (self));
    }
}

/**
 * bobgui_app_chooser_button_set_heading:
 * @self: a `BobguiAppChooserButton`
 * @heading: a string containing Pango markup
 *
 * Sets the text to display at the top of the dialog.
 *
 * If the heading is not set, the dialog displays a default text.
 *
 * Deprecated: 4.10: This widget will be removed in BOBGUI 5
 */
void
bobgui_app_chooser_button_set_heading (BobguiAppChooserButton *self,
                                    const char          *heading)
{
  g_return_if_fail (BOBGUI_IS_APP_CHOOSER_BUTTON (self));

  g_free (self->heading);
  self->heading = g_strdup (heading);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_HEADING]);
}

/**
 * bobgui_app_chooser_button_get_heading:
 * @self: a `BobguiAppChooserButton`
 *
 * Returns the text to display at the top of the dialog.
 *
 * Returns: (nullable): the text to display at the top of the dialog,
 *   or %NULL, in which case a default text is displayed
 *
 * Deprecated: 4.10: This widget will be removed in BOBGUI 5
 */
const char *
bobgui_app_chooser_button_get_heading (BobguiAppChooserButton *self)
{
  g_return_val_if_fail (BOBGUI_IS_APP_CHOOSER_BUTTON (self), NULL);

  return self->heading;
}

/**
 * bobgui_app_chooser_button_set_modal:
 * @self: a `BobguiAppChooserButton`
 * @modal: %TRUE to make the dialog modal
 *
 * Sets whether the dialog should be modal.
 *
 * Deprecated: 4.10: This widget will be removed in BOBGUI 5
 */
void
bobgui_app_chooser_button_set_modal (BobguiAppChooserButton *self,
                                  gboolean             modal)
{
  g_return_if_fail (BOBGUI_IS_APP_CHOOSER_BUTTON (self));

  if (self->modal == modal)
    return;

  self->modal = modal;

  g_object_notify (G_OBJECT (self), "modal");
}

/**
 * bobgui_app_chooser_button_get_modal:
 * @self: a `BobguiAppChooserButton`
 *
 * Gets whether the dialog is modal.
 *
 * Returns: %TRUE if the dialog is modal
 *
 * Deprecated: 4.10: This widget will be removed in BOBGUI 5
 */
gboolean
bobgui_app_chooser_button_get_modal (BobguiAppChooserButton *self)
{
  g_return_val_if_fail (BOBGUI_IS_APP_CHOOSER_BUTTON (self), FALSE);

  return self->modal;
}

