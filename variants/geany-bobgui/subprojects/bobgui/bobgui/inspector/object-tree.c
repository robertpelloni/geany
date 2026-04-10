/*
 * Copyright (c) 2008-2009  Christian Hammond
 * Copyright (c) 2008-2009  David Trowbridge
 * Copyright (c) 2013 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#include <string.h>

#include "object-tree.h"
#include "prop-list.h"
#include "window.h"

#include "bobguibuildable.h"
#include "bobguibutton.h"
#include "deprecated/bobguicelllayout.h"
#include "bobguicolumnview.h"
#include "deprecated/bobguicomboboxprivate.h"
#include "bobguifilterlistmodel.h"
#include "bobguicustomfilter.h"
#include "bobguiflattenlistmodel.h"
#include "bobguibuiltiniconprivate.h"
#include "deprecated/bobguiiconview.h"
#include "bobguiinscription.h"
#include "bobguilabel.h"
#include "bobguilistitem.h"
#include "bobguipopover.h"
#include "bobguinative.h"
#include "bobguisettings.h"
#include "bobguisingleselection.h"
#include "bobguisignallistitemfactory.h"
#include "bobguitextview.h"
#include "bobguitogglebutton.h"
#include "bobguitreeexpander.h"
#include "bobguitreelistmodel.h"
#include "deprecated/bobguitreeview.h"
#include "deprecated/bobguitreeselection.h"
#include "deprecated/bobguitreemodelsort.h"
#include "deprecated/bobguitreemodelfilter.h"
#include "bobguiwidgetprivate.h"
#include "bobguisearchbar.h"
#include "bobguisearchentry.h"
#include "bobguieventcontrollerkey.h"

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

enum
{
  OBJECT_SELECTED,
  OBJECT_ACTIVATED,
  LAST_SIGNAL
};

struct _BobguiInspectorObjectTreePrivate
{
  BobguiColumnView *list;
  BobguiTreeListModel *tree_model;
  BobguiSingleSelection *selection;
  BobguiWidget *search_bar;
  BobguiWidget *search_entry;
};

typedef struct _ObjectTreeClassFuncs ObjectTreeClassFuncs;
typedef void (* ObjectTreeForallFunc) (GObject    *object,
                                       const char *name,
                                       gpointer    data);

struct _ObjectTreeClassFuncs {
  GType         (* get_type)            (void);
  GObject *     (* get_parent)          (GObject                *object);
  GListModel *  (* get_children)        (GObject                *object);
};

static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE_WITH_PRIVATE (BobguiInspectorObjectTree, bobgui_inspector_object_tree, BOBGUI_TYPE_BOX)

static GObject *
object_tree_get_parent_default (GObject *object)
{
  return g_object_get_data (object, "inspector-object-tree-parent");
}

static GListModel *
object_tree_get_children_default (GObject *object)
{
  return NULL;
}

static GObject *
object_tree_widget_get_parent (GObject *object)
{
  return G_OBJECT (bobgui_widget_get_parent (BOBGUI_WIDGET (object)));
}

static GListModel *
object_tree_widget_get_children (GObject *object)
{
  BobguiWidget *widget = BOBGUI_WIDGET (object);
  GListStore *list;
  GListModel *sublist;

  list = g_list_store_new (G_TYPE_LIST_MODEL);

  sublist = bobgui_widget_observe_children (widget);
  g_list_store_append (list, sublist);
  g_object_unref (sublist);

  sublist = bobgui_widget_observe_controllers (widget);
  g_list_store_append (list, sublist);
  g_object_unref (sublist);

  return G_LIST_MODEL (bobgui_flatten_list_model_new (G_LIST_MODEL (list)));
}

static GObject *
object_tree_window_get_parent (GObject *object)
{
  return NULL;
}

static GListModel *
object_tree_native_get_children (GObject *object)
{
  BobguiNative *native = BOBGUI_NATIVE (object);
  GListStore *list;

  list = g_list_store_new (G_TYPE_OBJECT);

  if (bobgui_native_get_surface (native))
    g_list_store_append (list, bobgui_native_get_surface (native));
  if (bobgui_native_get_renderer (native))
    g_list_store_append (list, bobgui_native_get_renderer (native));

  return G_LIST_MODEL (list);
}

static GListModel *
object_tree_tree_model_sort_get_children (GObject *object)
{
  GListStore *store;

  store = g_list_store_new (G_TYPE_OBJECT);
  g_list_store_append (store, bobgui_tree_model_sort_get_model (BOBGUI_TREE_MODEL_SORT (object)));

  return G_LIST_MODEL (store);
}

static GListModel *
object_tree_tree_model_filter_get_children (GObject *object)
{
  GListStore *store;

  store = g_list_store_new (G_TYPE_OBJECT);
  g_list_store_append (store, bobgui_tree_model_filter_get_model (BOBGUI_TREE_MODEL_FILTER (object)));

  return G_LIST_MODEL (store);
}

static void
update_list_store (GListStore *store,
                   GObject    *object,
                   const char *property)
{
  gpointer value;

  g_object_get (object, property, &value, NULL);
  if (value)
    {
      g_list_store_splice (store,
                           0,
                           g_list_model_get_n_items (G_LIST_MODEL (store)),
                           &value,
                           1);
      g_object_unref (value);
    }
  else
    {
      g_list_store_remove_all (store);
    }
}

static void
list_model_for_property_notify_cb (GObject    *object,
                                   GParamSpec *pspec,
                                   GListStore *store)
{
  update_list_store (store, object, pspec->name);
}

static GListModel *
list_model_for_property (GObject    *object,
                         const char *property)
{
  GListStore *store = g_list_store_new (G_TYPE_OBJECT);

  /* g_signal_connect_object ("notify::property") */
  g_signal_connect_closure_by_id (object,
                                  g_signal_lookup ("notify", G_OBJECT_TYPE (object)),
                                  g_quark_from_static_string (property),
                                  g_cclosure_new_object (G_CALLBACK (list_model_for_property_notify_cb), G_OBJECT (store)),
                                  FALSE);
  update_list_store (store, object, property);

  return G_LIST_MODEL (store);
}

static GListModel *
list_model_for_properties (GObject     *object,
                           const char **props)
{
  GListStore *concat;
  guint i;

  if (props[1] == NULL)
    return list_model_for_property (object, props[0]);

  concat = g_list_store_new (G_TYPE_LIST_MODEL);
  for (i = 0; props[i]; i++)
    {
      GListModel *tmp = list_model_for_property (object, props[i]);
      g_list_store_append (concat, tmp);
      g_object_unref (tmp);
    }

  return G_LIST_MODEL (bobgui_flatten_list_model_new (G_LIST_MODEL (concat)));
}

static GListModel *
object_tree_combo_box_get_children (GObject *object)
{
  return list_model_for_properties (object, (const char *[2]) { "model", NULL });
}

static void
treeview_columns_changed (BobguiTreeView *treeview,
                          GListModel  *store)
{
  BobguiTreeViewColumn *column, *item;
  guint i, n_columns, n_items;

  n_columns = bobgui_tree_view_get_n_columns (treeview);
  n_items = g_list_model_get_n_items (store);

  for (i = 0; i < MAX (n_columns, n_items); i++)
    {
      column = bobgui_tree_view_get_column (treeview, i);
      item = g_list_model_get_item (store, i);
      g_object_unref (item);

      if (column == item)
        continue;

      if (n_columns < n_items)
        {
          /* column removed */
          g_assert (n_columns + 1 == n_items);
          g_list_store_remove (G_LIST_STORE (store), i);
          return;
        }
      else if (n_columns > n_items)
        {
          /* column added */
          g_assert (n_columns - 1 == n_items);
          g_list_store_insert (G_LIST_STORE (store), i, column);
          return;
        }
      else
        {
          guint j;
          /* column reordered */
          for (j = n_columns - 1; j > i; j--)
            {
              column = bobgui_tree_view_get_column (treeview, j);
              item = g_list_model_get_item (store, j);
              g_object_unref (item);

              if (column != item)
                break;
            }
          g_assert (j > i);
          column = bobgui_tree_view_get_column (treeview, i);
          item = g_list_model_get_item (store, j);
          g_object_unref (item);

          if (item == column)
            {
              /* column was removed from position j and put into position i */
              g_list_store_remove (G_LIST_STORE (store), j);
              g_list_store_insert (G_LIST_STORE (store), i, column);
            }
          else
            {
              /* column was removed from position i and put into position j */
              column = bobgui_tree_view_get_column (treeview, j);
              g_list_store_remove (G_LIST_STORE (store), i);
              g_list_store_insert (G_LIST_STORE (store), j, column);
            }
        }
    }
}

static GListModel *
object_tree_tree_view_get_children (GObject *object)
{
  BobguiTreeView *treeview = BOBGUI_TREE_VIEW (object);
  GListStore *columns, *selection, *result_list;
  GListModel *props;
  guint i;

  props = list_model_for_properties (object, (const char *[2]) { "model", NULL });

  columns = g_list_store_new (BOBGUI_TYPE_TREE_VIEW_COLUMN);
  g_signal_connect_object (treeview, "columns-changed", G_CALLBACK (treeview_columns_changed), columns, 0);
  for (i = 0; i < bobgui_tree_view_get_n_columns (treeview); i++)
    g_list_store_append (columns, bobgui_tree_view_get_column (treeview, i));

  selection = g_list_store_new (BOBGUI_TYPE_TREE_SELECTION);
  g_list_store_append (selection, bobgui_tree_view_get_selection (treeview));

  result_list = g_list_store_new (G_TYPE_LIST_MODEL);
  g_list_store_append (result_list, props);
  g_object_unref (props);
  g_list_store_append (result_list, selection);
  g_object_unref (selection);
  g_list_store_append (result_list, columns);
  g_object_unref (columns);

  return G_LIST_MODEL (bobgui_flatten_list_model_new (G_LIST_MODEL (result_list)));
}

static GListModel *
object_tree_column_view_get_children (GObject *object)
{
  BobguiColumnView *view = BOBGUI_COLUMN_VIEW (object);
  GListStore *result_list;
  GListModel *columns, *sublist;

  result_list = g_list_store_new (G_TYPE_LIST_MODEL);

  columns = bobgui_column_view_get_columns (view);
  g_list_store_append (result_list, columns);

  sublist = object_tree_widget_get_children (object);
  g_list_store_append (result_list, sublist);
  g_object_unref (sublist);

  return G_LIST_MODEL (bobgui_flatten_list_model_new (G_LIST_MODEL (result_list)));
}

static GListModel *
object_tree_icon_view_get_children (GObject *object)
{
  return list_model_for_properties (object, (const char *[2]) { "model", NULL });
}

static gboolean
object_tree_cell_area_add_child (BobguiCellRenderer  *renderer,
                                 gpointer          store)
{
  gpointer cell_layout;

  cell_layout = g_object_get_data (store, "bobgui-inspector-cell-layout");
  g_object_set_data (G_OBJECT (renderer), "bobgui-inspector-cell-layout", cell_layout);

  g_list_store_append (store, renderer);

  return FALSE;
}

static GListModel *
object_tree_cell_area_get_children (GObject *object)
{
  GListStore *store;
  gpointer cell_layout;

  cell_layout = g_object_get_data (object, "bobgui-inspector-cell-layout");
  store = g_list_store_new (BOBGUI_TYPE_CELL_RENDERER);
  g_object_set_data (G_OBJECT (store), "bobgui-inspector-cell-layout", cell_layout);
  /* XXX: no change notification for cell areas */
  bobgui_cell_area_foreach (BOBGUI_CELL_AREA (object), object_tree_cell_area_add_child, store);

  return G_LIST_MODEL (store);
}

static GListModel *
object_tree_cell_layout_get_children (GObject *object)
{
  GListStore *store;
  BobguiCellArea *area;

  /* cell areas handle their own stuff */
  if (BOBGUI_IS_CELL_AREA (object))
    return NULL;

  area = bobgui_cell_layout_get_area (BOBGUI_CELL_LAYOUT (object));
  if (!area)
    return NULL;

  g_object_set_data (G_OBJECT (area), "bobgui-inspector-cell-layout", object);
  /* XXX: are cell areas immutable? */
  store = g_list_store_new (G_TYPE_OBJECT);
  g_list_store_append (store, area);
  return G_LIST_MODEL (store);
}

static GListModel *
object_tree_text_view_get_children (GObject *object)
{
  return list_model_for_properties (object, (const char *[2]) { "buffer", NULL });
}

static GListModel *
object_tree_text_buffer_get_children (GObject *object)
{
  return list_model_for_properties (object, (const char *[2]) { "tag-table", NULL });
}

static void
text_tag_added (BobguiTextTagTable *table,
                BobguiTextTag      *tag,
                GListStore      *store)
{
  g_list_store_append (store, tag);
}

static void
text_tag_removed (BobguiTextTagTable *table,
                  BobguiTextTag      *tag,
                  GListStore      *store)
{
  guint i;

  for (i = 0; i < g_list_model_get_n_items (G_LIST_MODEL (store)); i++)
    {
      gpointer item = g_list_model_get_item (G_LIST_MODEL (store), i);
      g_object_unref (item);

      if (tag == item)
        {
          g_list_store_remove (store, i);
          return;
        }
    }
}

static void
text_tag_foreach (BobguiTextTag *tag,
                  gpointer    store)
{
  g_list_store_append (store, tag);
}

static GListModel *
object_tree_text_tag_table_get_children (GObject *object)
{
  GListStore *store = g_list_store_new (BOBGUI_TYPE_TEXT_TAG);

  g_signal_connect_object (object, "tag-added", G_CALLBACK (text_tag_added), store, 0);
  g_signal_connect_object (object, "tag-removed", G_CALLBACK (text_tag_removed), store, 0);
  bobgui_text_tag_table_foreach (BOBGUI_TEXT_TAG_TABLE (object), text_tag_foreach, store);

  return NULL;
}

static GListModel *
object_tree_application_get_children (GObject *object)
{
  return list_model_for_properties (object, (const char *[2]) { "menubar", NULL });
}

static GObject *
object_tree_event_controller_get_parent (GObject *object)
{
  return G_OBJECT (bobgui_event_controller_get_widget (BOBGUI_EVENT_CONTROLLER (object)));
}

/* Note:
 * This tree should be sorted with the most specific types first.
 * We iterate over it top to bottom and append the children to the
 * list if g_type_is_a () matches.
 */
static const ObjectTreeClassFuncs object_tree_class_funcs[] = {
  {
    bobgui_application_get_type,
    object_tree_get_parent_default,
    object_tree_application_get_children
  },
  {
    bobgui_text_tag_table_get_type,
    object_tree_get_parent_default,
    object_tree_text_tag_table_get_children
  },
  {
    bobgui_text_buffer_get_type,
    object_tree_get_parent_default,
    object_tree_text_buffer_get_children
  },
  {
    bobgui_text_view_get_type,
    object_tree_widget_get_parent,
    object_tree_text_view_get_children
  },
  {
    bobgui_icon_view_get_type,
    object_tree_widget_get_parent,
    object_tree_icon_view_get_children
  },
  {
    bobgui_tree_view_get_type,
    object_tree_widget_get_parent,
    object_tree_tree_view_get_children
  },
  {
    bobgui_column_view_get_type,
    object_tree_widget_get_parent,
    object_tree_column_view_get_children
  },
  {
    bobgui_combo_box_get_type,
    object_tree_widget_get_parent,
    object_tree_combo_box_get_children
  },
  {
    bobgui_window_get_type,
    object_tree_window_get_parent,
    object_tree_native_get_children,
  },
  {
    bobgui_popover_get_type,
    object_tree_widget_get_parent,
    object_tree_native_get_children,
  },
  {
    bobgui_widget_get_type,
    object_tree_widget_get_parent,
    object_tree_widget_get_children
  },
  {
    bobgui_tree_model_filter_get_type,
    object_tree_get_parent_default,
    object_tree_tree_model_filter_get_children
  },
  {
    bobgui_tree_model_sort_get_type,
    object_tree_get_parent_default,
    object_tree_tree_model_sort_get_children
  },
  {
    bobgui_cell_area_get_type,
    object_tree_get_parent_default,
    object_tree_cell_area_get_children
  },
  {
    bobgui_cell_layout_get_type,
    object_tree_get_parent_default,
    object_tree_cell_layout_get_children
  },
  {
    bobgui_event_controller_get_type,
    object_tree_event_controller_get_parent,
    object_tree_get_children_default
  },
  {
    g_object_get_type,
    object_tree_get_parent_default,
    object_tree_get_children_default
  },
};

static const ObjectTreeClassFuncs *
find_class_funcs (GObject *object)
{
  GType object_type;
  guint i;

  object_type = G_OBJECT_TYPE (object);

  for (i = 0; i < G_N_ELEMENTS (object_tree_class_funcs); i++)
    {
      if (g_type_is_a (object_type, object_tree_class_funcs[i].get_type ()))
        return &object_tree_class_funcs[i];
    }

  g_assert_not_reached ();

  return NULL;
}

static GObject *
object_get_parent (GObject *object)
{
  const ObjectTreeClassFuncs *funcs;

  funcs = find_class_funcs (object);
  
  return funcs->get_parent (object);
}

static GListModel *
object_get_children (GObject *object)
{
  GType object_type;
  GListModel *children;
  GListStore *result_list;
  guint i;

  object_type = G_OBJECT_TYPE (object);
  result_list = NULL;

  for (i = 0; i < G_N_ELEMENTS (object_tree_class_funcs); i++)
    {
      if (!g_type_is_a (object_type, object_tree_class_funcs[i].get_type ()))
        continue;

      children = object_tree_class_funcs[i].get_children (object);
      if (children == NULL)
        continue;

      if (!result_list)
        result_list = g_list_store_new (G_TYPE_LIST_MODEL);

      g_list_store_append (result_list, children);
      g_object_unref (children);
    }

  if (result_list)
    return G_LIST_MODEL (bobgui_flatten_list_model_new (G_LIST_MODEL (result_list)));
  else
    return NULL;
}

static const char *
bobgui_inspector_get_object_name (GObject *object)
{
  if (BOBGUI_IS_WIDGET (object))
    {
      const char *id;

      id = bobgui_widget_get_name (BOBGUI_WIDGET (object));
      if (id != NULL && g_strcmp0 (id, G_OBJECT_TYPE_NAME (object)) != 0)
        return id;
    }

  if (BOBGUI_IS_BUILDABLE (object))
    {
      const char *id;

      id = bobgui_buildable_get_buildable_id (BOBGUI_BUILDABLE (object));
      if (id != NULL && !g_str_has_prefix (id, "___object_"))
        return id;
    }

  if (BOBGUI_IS_EVENT_CONTROLLER (object))
    {
      return bobgui_event_controller_get_name (BOBGUI_EVENT_CONTROLLER (object));
    }

  return NULL;
}

char *
bobgui_inspector_get_object_title (GObject *object)
{
  const char *name = bobgui_inspector_get_object_name (object);

  if (name == NULL)
    return g_strdup (G_OBJECT_TYPE_NAME (object));
  else
    return g_strconcat (G_OBJECT_TYPE_NAME (object), " — ", name, NULL);
}

void
bobgui_inspector_object_tree_activate_object (BobguiInspectorObjectTree *wt,
                                           GObject                *object)
{
  bobgui_inspector_object_tree_select_object (wt, object);
  g_signal_emit (wt, signals[OBJECT_ACTIVATED], 0, object);
}

static void
on_row_activated (BobguiColumnView          *view,
                  guint                   pos,
                  BobguiInspectorObjectTree *wt)
{
  BobguiTreeListRow *item;
  GObject *object;

  item = g_list_model_get_item (G_LIST_MODEL (wt->priv->tree_model), pos);
  object = bobgui_tree_list_row_get_item (item);

  bobgui_inspector_object_tree_activate_object (wt, object);

  g_object_unref (item);
  g_object_unref (object);
}

GObject *
bobgui_inspector_object_tree_get_selected (BobguiInspectorObjectTree *wt)
{
  BobguiTreeListRow *selected_item;
  GObject *object;

  selected_item = bobgui_single_selection_get_selected_item (wt->priv->selection);
  if (selected_item == NULL)
    return NULL;

  object = bobgui_tree_list_row_get_item (selected_item);

  g_object_unref (object); /* ahem */
  return object;
}

static void
widget_mapped (BobguiWidget *widget,
               BobguiWidget *label)
{
  bobgui_widget_remove_css_class (label, "dim-label");
}

static void
widget_unmapped (BobguiWidget *widget,
                 BobguiWidget *label)
{
  bobgui_widget_add_css_class (label, "dim-label");
}

static gboolean
search (BobguiInspectorObjectTree *wt,
        gboolean                forward,
        gboolean                force_progress);

static gboolean
key_pressed (BobguiEventController     *controller,
             guint                   keyval,
             guint                   keycode,
             GdkModifierType         state,
             BobguiInspectorObjectTree *wt)
{
  if (bobgui_widget_get_mapped (BOBGUI_WIDGET (wt)))
    {
      GdkModifierType default_accel;
      gboolean search_started;

      search_started = bobgui_search_bar_get_search_mode (BOBGUI_SEARCH_BAR (wt->priv->search_bar));
      default_accel = GDK_CONTROL_MASK;

      if (search_started &&
          (keyval == GDK_KEY_Return ||
           keyval == GDK_KEY_ISO_Enter ||
           keyval == GDK_KEY_KP_Enter))
        {
          bobgui_widget_activate (BOBGUI_WIDGET (wt->priv->list));
          return GDK_EVENT_PROPAGATE;
        }
      else if (search_started &&
               (keyval == GDK_KEY_Escape))
        {
          bobgui_search_bar_set_search_mode (BOBGUI_SEARCH_BAR (wt->priv->search_bar), FALSE);
          return GDK_EVENT_STOP;
        }
      else if (search_started &&
               ((state & (default_accel | GDK_SHIFT_MASK)) == (default_accel | GDK_SHIFT_MASK)) &&
               (keyval == GDK_KEY_g || keyval == GDK_KEY_G))
        {
          if (!search (wt, TRUE, TRUE))
            bobgui_widget_error_bell (BOBGUI_WIDGET (wt));
          return GDK_EVENT_STOP;
        }
      else if (search_started &&
               ((state & (default_accel | GDK_SHIFT_MASK)) == default_accel) &&
               (keyval == GDK_KEY_g || keyval == GDK_KEY_G))
        {
          if (!search (wt, TRUE, TRUE))
            bobgui_widget_error_bell (BOBGUI_WIDGET (wt));
          return GDK_EVENT_STOP;
        }
    }

  return GDK_EVENT_PROPAGATE;
}

static void
destroy_controller (BobguiEventController *controller)
{
  bobgui_widget_remove_controller (bobgui_event_controller_get_widget (controller), controller);
}

static gboolean toplevel_filter_func (gpointer item,
                                      gpointer data);

static void
map (BobguiWidget *widget)
{
  BobguiInspectorObjectTree *wt = BOBGUI_INSPECTOR_OBJECT_TREE (widget);
  BobguiEventController *controller;
  BobguiWidget *toplevel;

  BOBGUI_WIDGET_CLASS (bobgui_inspector_object_tree_parent_class)->map (widget);

  toplevel = BOBGUI_WIDGET (bobgui_widget_get_root (widget));

  controller = bobgui_event_controller_key_new ();
  g_object_set_data_full (G_OBJECT (toplevel), "object-controller", controller, (GDestroyNotify)destroy_controller);
  g_signal_connect (controller, "key-pressed", G_CALLBACK (key_pressed), widget);
  bobgui_widget_add_controller (toplevel, controller);

  bobgui_search_bar_set_key_capture_widget (BOBGUI_SEARCH_BAR (wt->priv->search_bar), toplevel);
}

static void
unmap (BobguiWidget *widget)
{
  BobguiWidget *toplevel;

  toplevel = BOBGUI_WIDGET (bobgui_widget_get_root (widget));
  g_object_set_data (G_OBJECT (toplevel), "object-controller", NULL);

  BOBGUI_WIDGET_CLASS (bobgui_inspector_object_tree_parent_class)->unmap (widget);
}

static gboolean
match_string (const char *string,
              const char *text)
{
  char *lower;
  gboolean match = FALSE;

  if (string)
    {
      lower = g_ascii_strdown (string, -1);
      match = g_str_has_prefix (lower, text);
      g_free (lower);
    }

  return match;
}

static gboolean
match_object (GObject    *object,
              const char *text)
{
  char *address;
  gboolean ret = FALSE;

  if (match_string (G_OBJECT_TYPE_NAME (object), text) ||
      match_string (bobgui_inspector_get_object_name (object), text))
    return TRUE;

  if (BOBGUI_IS_LABEL (object))
    return match_string (bobgui_label_get_label (BOBGUI_LABEL (object)), text);
  if (BOBGUI_IS_INSCRIPTION (object))
    return match_string (bobgui_inscription_get_text (BOBGUI_INSCRIPTION (object)), text);
  else if (BOBGUI_IS_BUTTON (object))
    return match_string (bobgui_button_get_label (BOBGUI_BUTTON (object)), text);
  else if (BOBGUI_IS_WINDOW (object))
    return match_string (bobgui_window_get_title (BOBGUI_WINDOW (object)), text);
  else if (BOBGUI_IS_TREE_VIEW_COLUMN (object))
    return match_string (bobgui_tree_view_column_get_title (BOBGUI_TREE_VIEW_COLUMN (object)), text);
  else if (BOBGUI_IS_EDITABLE (object))
    return match_string (bobgui_editable_get_text (BOBGUI_EDITABLE (object)), text);

  address = g_strdup_printf ("%p", object);
  ret = match_string (address, text);
  g_free (address);

  return ret;
}

static GObject *
search_children (GObject    *object,
                 const char *text,
                 gboolean    forward)
{
  GListModel *children;
  GObject *child, *result;
  guint i, n;

  children = object_get_children (object);
  if (children == NULL)
    return NULL;

  n = g_list_model_get_n_items (children);
  for (i = 0; i < n; i++)
    {
      child = g_list_model_get_item (children, forward ? i : n - i - 1);
      if (match_object (child, text))
        return child;

      result = search_children (child, text, forward);
      g_object_unref (child);
      if (result)
        return result;
    }

  return NULL;
}

static gboolean
search (BobguiInspectorObjectTree *wt,
        gboolean                forward,
        gboolean                force_progress)
{
  BobguiInspectorObjectTreePrivate *priv = wt->priv;
  GListModel *model = G_LIST_MODEL (priv->tree_model);
  BobguiTreeListRow *row_item;
  GObject *child, *result;
  guint i, selected, n, row;
  const char *text;

  text = bobgui_editable_get_text (BOBGUI_EDITABLE (priv->search_entry));
  selected = bobgui_single_selection_get_selected (priv->selection);
  n = g_list_model_get_n_items (model);
  if (selected >= n)
    selected = 0;

  for (i = 0; i < n; i++)
    {
      row = (selected + (forward ? i : n - i - 1)) % n;
      row_item = g_list_model_get_item (model, row);
      child = bobgui_tree_list_row_get_item (row_item);
      if (i > 0 || !force_progress)
        {
          if (match_object (child, text))
            {
              bobgui_column_view_scroll_to (BOBGUI_COLUMN_VIEW (wt->priv->list),
                                         row,
                                         NULL,
                                         BOBGUI_LIST_SCROLL_SELECT | BOBGUI_LIST_SCROLL_FOCUS,
                                         NULL);
              g_object_unref (child);
              g_object_unref (row_item);
              return TRUE;
            }
        }

      if (!bobgui_tree_list_row_get_expanded (row_item))
        {
          result = search_children (child, text, forward);
          if (result)
            {
              bobgui_inspector_object_tree_select_object (wt, result);
              g_object_unref (result);
              g_object_unref (child);
              g_object_unref (row_item);
              return TRUE;
            }
        }
      g_object_unref (child);
      g_object_unref (row_item);
    }

  return FALSE;
}

static void
on_search_changed (BobguiSearchEntry         *entry,
                   BobguiInspectorObjectTree *wt)
{
  if (!search (wt, TRUE, FALSE))
    bobgui_widget_error_bell (BOBGUI_WIDGET (wt));
}

static void
next_match (BobguiButton              *button,
            BobguiInspectorObjectTree *wt)
{
  if (bobgui_search_bar_get_search_mode (BOBGUI_SEARCH_BAR (wt->priv->search_bar)))
    {
      if (!search (wt, TRUE, TRUE))
        bobgui_widget_error_bell (BOBGUI_WIDGET (wt));
    }
}

static void
previous_match (BobguiButton              *button,
                BobguiInspectorObjectTree *wt)
{
  if (bobgui_search_bar_get_search_mode (BOBGUI_SEARCH_BAR (wt->priv->search_bar)))
    {
      if (!search (wt, FALSE, TRUE))
        bobgui_widget_error_bell (BOBGUI_WIDGET (wt));
    }
}

static void
stop_search (BobguiWidget              *entry,
             BobguiInspectorObjectTree *wt)
{
  bobgui_editable_set_text (BOBGUI_EDITABLE (wt->priv->search_entry), "");
  bobgui_search_bar_set_search_mode (BOBGUI_SEARCH_BAR (wt->priv->search_bar), FALSE);
}

static void
setup_type_cb (BobguiSignalListItemFactory *factory,
               BobguiListItem              *list_item)
{
  BobguiWidget *expander, *inscription;

  /* expander */
  expander = bobgui_tree_expander_new ();
  bobgui_list_item_set_child (list_item, expander);

  /* label */
  inscription = bobgui_inscription_new (NULL);
  bobgui_inscription_set_nat_chars (BOBGUI_INSCRIPTION (inscription), 30);
  bobgui_tree_expander_set_child (BOBGUI_TREE_EXPANDER (expander), inscription);
}

static void
bind_type_cb (BobguiSignalListItemFactory *factory,
              BobguiListItem              *list_item)
{
  BobguiWidget *expander, *inscription;
  BobguiTreeListRow *list_row;
  gpointer item;

  list_row = bobgui_list_item_get_item (list_item);
  expander = bobgui_list_item_get_child (list_item);
  bobgui_tree_expander_set_list_row (BOBGUI_TREE_EXPANDER (expander), list_row);
  item = bobgui_tree_list_row_get_item (list_row);
  inscription = bobgui_tree_expander_get_child (BOBGUI_TREE_EXPANDER (expander));

  bobgui_inscription_set_text (BOBGUI_INSCRIPTION (inscription), G_OBJECT_TYPE_NAME (item));

  if (BOBGUI_IS_WIDGET (item))
    {
      g_signal_connect (item, "map", G_CALLBACK (widget_mapped), inscription);
      g_signal_connect (item, "unmap", G_CALLBACK (widget_unmapped), inscription);
      if (!bobgui_widget_get_mapped (item))
        widget_unmapped (item, inscription);
      g_object_set_data (G_OBJECT (inscription), "binding", g_object_ref (item));
    }

  g_object_unref (item);
}

static void
unbind_type_cb (BobguiSignalListItemFactory *factory,
                BobguiListItem              *list_item)
{
  BobguiWidget *expander, *label;
  gpointer item;

  expander = bobgui_list_item_get_child (list_item);
  label = bobgui_tree_expander_get_child (BOBGUI_TREE_EXPANDER (expander));
  item = g_object_steal_data (G_OBJECT (label), "binding");
  if (item)
    {
      g_signal_handlers_disconnect_by_func (item, widget_mapped, label);
      g_signal_handlers_disconnect_by_func (item, widget_unmapped, label);

      g_object_unref (item);
    }
}

static void
setup_name_cb (BobguiSignalListItemFactory *factory,
               BobguiListItem              *list_item)
{
  BobguiWidget *inscription;

  inscription = bobgui_inscription_new (NULL);
  bobgui_inscription_set_nat_chars (BOBGUI_INSCRIPTION (inscription), 15);
  bobgui_list_item_set_child (list_item, inscription);
}

static void
bind_name_cb (BobguiSignalListItemFactory *factory,
              BobguiListItem              *list_item)
{
  BobguiWidget *inscription;
  gpointer item;

  item = bobgui_tree_list_row_get_item (bobgui_list_item_get_item (list_item));
  inscription = bobgui_list_item_get_child (list_item);

  bobgui_inscription_set_text (BOBGUI_INSCRIPTION (inscription), bobgui_inspector_get_object_name (item));

  g_object_unref (item);
}

static void
setup_label_cb (BobguiSignalListItemFactory *factory,
                BobguiListItem              *list_item)
{
  BobguiWidget *inscription;

  inscription = bobgui_inscription_new (NULL);
  bobgui_inscription_set_nat_chars (BOBGUI_INSCRIPTION (inscription), 25);
  bobgui_list_item_set_child (list_item, inscription);
}

static void
bind_label_cb (BobguiSignalListItemFactory *factory,
               BobguiListItem              *list_item)
{
  BobguiWidget *inscription;
  gpointer item;
  GBinding *binding = NULL;

  item = bobgui_tree_list_row_get_item (bobgui_list_item_get_item (list_item));
  inscription = bobgui_list_item_get_child (list_item);

  if (BOBGUI_IS_LABEL (item))
    binding = g_object_bind_property (item, "label", inscription, "text", G_BINDING_SYNC_CREATE);
  if (BOBGUI_IS_INSCRIPTION (item))
    binding = g_object_bind_property (item, "text", inscription, "text", G_BINDING_SYNC_CREATE);
  else if (BOBGUI_IS_BUTTON (item))
    binding = g_object_bind_property (item, "label", inscription, "text", G_BINDING_SYNC_CREATE);
  else if (BOBGUI_IS_WINDOW (item))
    binding = g_object_bind_property (item, "title", inscription, "text", G_BINDING_SYNC_CREATE);
  else if (BOBGUI_IS_TREE_VIEW_COLUMN (item))
    binding = g_object_bind_property (item, "title", inscription, "text", G_BINDING_SYNC_CREATE);
  else if (BOBGUI_IS_EDITABLE (item))
    binding = g_object_bind_property (item, "text", inscription, "text", G_BINDING_SYNC_CREATE);
  else
    bobgui_inscription_set_text (BOBGUI_INSCRIPTION (inscription), NULL);

  g_object_unref (item);

  if (binding)
    g_object_set_data (G_OBJECT (inscription), "binding", binding);
}

static void
unbind_label_cb (BobguiSignalListItemFactory *factory,
                 BobguiListItem              *list_item)
{
  BobguiWidget *inscription;
  GBinding *binding;

  inscription = bobgui_list_item_get_child (list_item);
  binding = g_object_steal_data (G_OBJECT (inscription), "binding");
  if (binding)
    g_binding_unbind (binding);
}

static GListModel *
create_model_for_object (gpointer object,
                         gpointer user_data)
{
  return object_get_children (object);
}

static gboolean
toplevel_filter_func (gpointer item,
                      gpointer data)
{
  GdkDisplay *display = data;
  gpointer iw;

  if (!BOBGUI_IS_WINDOW (item))
    return FALSE;

  if (bobgui_widget_get_display (item) != display)
    return FALSE;

  iw = g_object_get_data (G_OBJECT (display), "-bobgui-inspector");
  if (iw == item)
    return FALSE;

  return TRUE;
}

static GListModel *
create_root_model (GdkDisplay *display)
{
  BobguiFilter *filter;
  BobguiFilterListModel *filter_model;
  GListModel *model;
  GListStore *list, *special;
  gpointer item;

  list = g_list_store_new (G_TYPE_LIST_MODEL);

  special = g_list_store_new (G_TYPE_OBJECT);
  item = g_application_get_default ();
  if (item)
    g_list_store_append (special, item);
  g_list_store_append (special, bobgui_settings_get_for_display (display));
  g_list_store_append (list, special);
  g_object_unref (special);

  filter = BOBGUI_FILTER (bobgui_custom_filter_new (toplevel_filter_func, display, NULL));
  model = bobgui_window_get_toplevels ();
  filter_model = bobgui_filter_list_model_new (g_object_ref (model), filter);
  g_list_store_append (list, filter_model);
  g_object_unref (filter_model);

  return G_LIST_MODEL (bobgui_flatten_list_model_new (G_LIST_MODEL (list)));
}

static void
bobgui_inspector_object_tree_init (BobguiInspectorObjectTree *wt)
{
  wt->priv = bobgui_inspector_object_tree_get_instance_private (wt);
  bobgui_widget_init_template (BOBGUI_WIDGET (wt));

  bobgui_search_bar_connect_entry (BOBGUI_SEARCH_BAR (wt->priv->search_bar),
                                BOBGUI_EDITABLE (wt->priv->search_entry));
}

static void
bobgui_inspector_object_tree_dispose (GObject *object)
{
  BobguiInspectorObjectTree *wt = BOBGUI_INSPECTOR_OBJECT_TREE (object);

  g_clear_object (&wt->priv->tree_model);
  g_clear_object (&wt->priv->selection);

  G_OBJECT_CLASS (bobgui_inspector_object_tree_parent_class)->dispose (object);
}

static void
bobgui_inspector_object_tree_class_init (BobguiInspectorObjectTreeClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);

  object_class->dispose = bobgui_inspector_object_tree_dispose;

  widget_class->map = map;
  widget_class->unmap = unmap;

  signals[OBJECT_ACTIVATED] =
      g_signal_new ("object-activated",
                    G_OBJECT_CLASS_TYPE (klass),
                    G_SIGNAL_RUN_FIRST | G_SIGNAL_NO_RECURSE,
                    G_STRUCT_OFFSET (BobguiInspectorObjectTreeClass, object_activated),
                    NULL, NULL,
                    NULL,
                    G_TYPE_NONE, 1, G_TYPE_OBJECT);

  signals[OBJECT_SELECTED] =
      g_signal_new ("object-selected",
                    G_OBJECT_CLASS_TYPE (klass),
                    G_SIGNAL_RUN_FIRST | G_SIGNAL_NO_RECURSE,
                    G_STRUCT_OFFSET (BobguiInspectorObjectTreeClass, object_selected),
                    NULL, NULL,
                    NULL,
                    G_TYPE_NONE, 1, G_TYPE_OBJECT);

  bobgui_widget_class_set_template_from_resource (widget_class, "/org/bobgui/libbobgui/inspector/object-tree.ui");
  bobgui_widget_class_bind_template_child_private (widget_class, BobguiInspectorObjectTree, list);
  bobgui_widget_class_bind_template_child_private (widget_class, BobguiInspectorObjectTree, search_bar);
  bobgui_widget_class_bind_template_child_private (widget_class, BobguiInspectorObjectTree, search_entry);
  bobgui_widget_class_bind_template_callback (widget_class, on_search_changed);
  bobgui_widget_class_bind_template_callback (widget_class, on_row_activated);
  bobgui_widget_class_bind_template_callback (widget_class, next_match);
  bobgui_widget_class_bind_template_callback (widget_class, previous_match);
  bobgui_widget_class_bind_template_callback (widget_class, stop_search);
  bobgui_widget_class_bind_template_callback (widget_class, setup_type_cb);
  bobgui_widget_class_bind_template_callback (widget_class, bind_type_cb);
  bobgui_widget_class_bind_template_callback (widget_class, unbind_type_cb);
  bobgui_widget_class_bind_template_callback (widget_class, setup_name_cb);
  bobgui_widget_class_bind_template_callback (widget_class, bind_name_cb);
  bobgui_widget_class_bind_template_callback (widget_class, setup_label_cb);
  bobgui_widget_class_bind_template_callback (widget_class, bind_label_cb);
  bobgui_widget_class_bind_template_callback (widget_class, unbind_label_cb);
}

static guint
model_get_item_index (GListModel *model,
                      gpointer    item)
{
  gpointer cmp;
  guint i;

  for (i = 0; (cmp = g_list_model_get_item (model, i)); i++)
    {
      if (cmp == item)
        {
          g_object_unref (cmp);
          return i;
        }
      g_object_unref (cmp);
    }

  return G_MAXUINT;
}

static BobguiTreeListRow *
find_and_expand_object (BobguiTreeListModel *model,
                        GObject          *object)
{
  BobguiTreeListRow *result;
  GObject *parent;
  guint pos;

  parent = object_get_parent (object);
  if (parent)
    {
      BobguiTreeListRow *parent_row = find_and_expand_object (model, parent);
      if (parent_row == NULL)
        return NULL;

      bobgui_tree_list_row_set_expanded (parent_row, TRUE);
      pos = model_get_item_index (bobgui_tree_list_row_get_children (parent_row), object);
      result = bobgui_tree_list_row_get_child_row (parent_row, pos);
      g_object_unref (parent_row);
    }
  else
    {
      pos = model_get_item_index (bobgui_tree_list_model_get_model (model), object);
      result = bobgui_tree_list_model_get_child_row (model, pos);
    }
  
  return result;
}

void
bobgui_inspector_object_tree_select_object (BobguiInspectorObjectTree *wt,
                                         GObject                *object)
{
  BobguiTreeListRow *row_item;

  row_item = find_and_expand_object (wt->priv->tree_model, object);
  if (row_item == NULL)
    return;

  bobgui_column_view_scroll_to (BOBGUI_COLUMN_VIEW (wt->priv->list),
                             bobgui_tree_list_row_get_position (row_item),
                             NULL,
                             BOBGUI_LIST_SCROLL_SELECT | BOBGUI_LIST_SCROLL_FOCUS,
                             NULL);

  g_signal_emit (wt, signals[OBJECT_SELECTED], 0, object);
  g_object_unref (row_item);
}

static void
on_selected_item (BobguiSingleSelection     *selection,
                  GParamSpec             *pspec,
                  BobguiInspectorObjectTree *wt)
{
  GObject *selected = bobgui_single_selection_get_selected_item (selection);
  BobguiTreeListRow *row = BOBGUI_TREE_LIST_ROW (selected);
  GObject *object = bobgui_tree_list_row_get_item (row);
  g_signal_emit (wt, signals[OBJECT_SELECTED], 0, object);
  g_object_unref (object);
}

void
bobgui_inspector_object_tree_set_display (BobguiInspectorObjectTree *wt,
                                       GdkDisplay *display)
{
  wt->priv->tree_model = bobgui_tree_list_model_new (create_root_model (display),
                                                  FALSE,
                                                  FALSE,
                                                  create_model_for_object,
                                                  NULL,
                                                  NULL);
  wt->priv->selection = bobgui_single_selection_new (g_object_ref (G_LIST_MODEL (wt->priv->tree_model)));
  bobgui_column_view_set_model (BOBGUI_COLUMN_VIEW (wt->priv->list),
                             BOBGUI_SELECTION_MODEL (wt->priv->selection));
  g_signal_connect (wt->priv->selection, "notify::selected-item", G_CALLBACK (on_selected_item), wt);
}
