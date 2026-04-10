/*
 * Copyright (c) 2014 Red Hat, Inc.
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

#include "statistics.h"

#include "graphdata.h"
#include "graphrenderer.h"

#include "bobguilabel.h"
#include "bobguisearchbar.h"
#include "bobguistack.h"
#include "bobguitogglebutton.h"
#include "bobguimain.h"
#include "bobguicolumnview.h"
#include "bobguicolumnviewcolumn.h"
#include "bobguisingleselection.h"
#include "bobguisignallistitemfactory.h"
#include "bobguilistitem.h"
#include "bobguistringsorter.h"
#include "bobguinumericsorter.h"
#include "bobguisortlistmodel.h"
#include "bobguisearchentry.h"

#include <glib/gi18n-lib.h>

/* {{{ TypeData object */

typedef struct _TypeData TypeData;

G_DECLARE_FINAL_TYPE (TypeData, type_data, TYPE, DATA, GObject);

struct _TypeData {
  GObject parent;

  GType type;
  GraphData *self;
  GraphData *cumulative;
};

enum {
  TYPE_DATA_PROP_NAME = 1,
  TYPE_DATA_PROP_SELF1,
  TYPE_DATA_PROP_CUMULATIVE1,
  TYPE_DATA_PROP_SELF2,
  TYPE_DATA_PROP_CUMULATIVE2,
  TYPE_DATA_PROP_SELF,
  TYPE_DATA_PROP_CUMULATIVE,
};

G_DEFINE_TYPE (TypeData, type_data, G_TYPE_OBJECT);

static void
type_data_init (TypeData *self)
{
}

static void
type_data_finalize (GObject *object)
{
  TypeData *self = TYPE_DATA (object);

  g_object_unref (self->self);
  g_object_unref (self->cumulative);

  G_OBJECT_CLASS (type_data_parent_class)->finalize (object);
}

static void
type_data_get_property (GObject    *object,
                        guint       property_id,
                        GValue     *value,
                        GParamSpec *pspec)
{
  TypeData *self = TYPE_DATA (object);

  switch (property_id)
    {
    case TYPE_DATA_PROP_NAME:
      g_value_set_string (value, g_type_name (self->type));
      break;

    case TYPE_DATA_PROP_SELF1:
      g_value_set_int (value, (int) graph_data_get_value (self->self, 1));
      break;

    case TYPE_DATA_PROP_CUMULATIVE1:
      g_value_set_int (value, (int) graph_data_get_value (self->cumulative, 1));
      break;

    case TYPE_DATA_PROP_SELF2:
      g_value_set_int (value, (int) graph_data_get_value (self->self, 0));
      break;

    case TYPE_DATA_PROP_CUMULATIVE2:
      g_value_set_int (value, (int) graph_data_get_value (self->cumulative, 0));
      break;

    case TYPE_DATA_PROP_SELF:
      g_value_set_object (value, self->self);
      break;

    case TYPE_DATA_PROP_CUMULATIVE:
      g_value_set_object (value, self->cumulative);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
type_data_class_init (TypeDataClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->finalize = type_data_finalize;
  object_class->get_property = type_data_get_property;

  g_object_class_install_property (object_class,
                                   TYPE_DATA_PROP_NAME,
                                   g_param_spec_string ("name", NULL, NULL,
                                                        NULL,
                                                        G_PARAM_READABLE |
                                                        G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class,
                                   TYPE_DATA_PROP_SELF1,
                                   g_param_spec_int ("self1", NULL, NULL,
                                                     0, G_MAXINT, 0,
                                                     G_PARAM_READABLE |
                                                     G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class,
                                   TYPE_DATA_PROP_CUMULATIVE1,
                                   g_param_spec_int ("cumulative1", NULL, NULL,
                                                     0, G_MAXINT, 0,
                                                     G_PARAM_READABLE |
                                                     G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class,
                                   TYPE_DATA_PROP_SELF2,
                                   g_param_spec_int ("self2", NULL, NULL,
                                                     0, G_MAXINT, 0,
                                                     G_PARAM_READABLE |
                                                     G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class,
                                   TYPE_DATA_PROP_CUMULATIVE2,
                                   g_param_spec_int ("cumulative2", NULL, NULL,
                                                     0, G_MAXINT, 0,
                                                     G_PARAM_READABLE |
                                                     G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class,
                                   TYPE_DATA_PROP_SELF,
                                   g_param_spec_object ("self", NULL, NULL,
                                                        graph_data_get_type (),
                                                        G_PARAM_READABLE |
                                                        G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class,
                                   TYPE_DATA_PROP_CUMULATIVE,
                                   g_param_spec_object ("cumulative", NULL, NULL,
                                                        graph_data_get_type (),
                                                        G_PARAM_READABLE |
                                                        G_PARAM_STATIC_STRINGS));
}

static TypeData *
type_data_new (GType type)
{
  TypeData *self;

  self = g_object_new (type_data_get_type (), NULL);

  self->type = type;
  self->self = graph_data_new (60);
  self->cumulative = graph_data_new (60);

  return self;
}

static void
type_data_update (TypeData *data,
                  int       self,
                  int       cumulative)
{
  int value;

  g_object_freeze_notify (G_OBJECT (data));

  value = graph_data_get_value (data->self, 0);
  if (value != self)
    g_object_notify (G_OBJECT (data), "self2");
  if (value != graph_data_get_value (data->self, 1))
    g_object_notify (G_OBJECT (data), "self1");

  g_object_notify (G_OBJECT (data), "self");
  graph_data_prepend_value (data->self, self);

  value = graph_data_get_value (data->cumulative, 0);
  if (value != cumulative)
    g_object_notify (G_OBJECT (data), "cumulative2");
  if (value != graph_data_get_value (data->cumulative, 1))
    g_object_notify (G_OBJECT (data), "cumulative1");

  g_object_notify (G_OBJECT (data), "cumulative");
  graph_data_prepend_value (data->cumulative, cumulative);

  g_object_thaw_notify (G_OBJECT (data));
}

/* }}} */

enum
{
  PROP_0,
  PROP_BUTTON
};

struct _BobguiInspectorStatisticsPrivate
{
  BobguiWidget *stack;
  BobguiWidget *excuse;
  BobguiWidget *view;
  BobguiWidget *button;
  GListStore *data;
  BobguiSingleSelection *selection;
  GHashTable *types;
  guint update_source_id;
  BobguiWidget *search_entry;
  BobguiWidget *search_bar;
};

G_DEFINE_TYPE_WITH_PRIVATE (BobguiInspectorStatistics, bobgui_inspector_statistics, BOBGUI_TYPE_BOX)

static int
add_type_count (BobguiInspectorStatistics *sl, GType type)
{
  int cumulative;
  int self;
  GType *children;
  guint n_children;
  int i;
  guint idx;
  TypeData *data;

  cumulative = 0;

  children = g_type_children (type, &n_children);
  for (i = 0; i < n_children; i++)
    cumulative += add_type_count (sl, children[i]);

  idx = GPOINTER_TO_UINT (g_hash_table_lookup (sl->priv->types, GSIZE_TO_POINTER (type)));
  if (idx == 0)
    {
      g_list_store_append (sl->priv->data, type_data_new (type));
      idx = g_list_model_get_n_items (G_LIST_MODEL (sl->priv->data));
      g_hash_table_insert (sl->priv->types, GSIZE_TO_POINTER (type), GUINT_TO_POINTER (idx));
    }

  data = g_list_model_get_item (G_LIST_MODEL (sl->priv->data), idx - 1);

  g_assert (data->type == type);

  self = g_type_get_instance_count (type);
  cumulative += self;

  type_data_update (data, self, cumulative);

  g_object_unref (data);

  return cumulative;
}

static gboolean
update_type_counts (gpointer data)
{
  BobguiInspectorStatistics *sl = data;
  GType type;

  for (type = G_TYPE_INTERFACE; type <= G_TYPE_FUNDAMENTAL_MAX; type += (1 << G_TYPE_FUNDAMENTAL_SHIFT))
    {
      if (!G_TYPE_IS_INSTANTIATABLE (type))
        continue;

      add_type_count (sl, type);
    }

  return TRUE;
}

static void
toggle_record (BobguiToggleButton        *button,
               BobguiInspectorStatistics *sl)
{
  if (bobgui_toggle_button_get_active (button) == (sl->priv->update_source_id != 0))
    return;

  if (bobgui_toggle_button_get_active (button))
    {
      sl->priv->update_source_id = g_timeout_add_seconds (1, update_type_counts, sl);
      update_type_counts (sl);
    }
  else
    {
      g_source_remove (sl->priv->update_source_id);
      sl->priv->update_source_id = 0;
    }
}

static gboolean
has_instance_counts (void)
{
  return g_type_get_instance_count (BOBGUI_TYPE_LABEL) > 0;
}

static gboolean
instance_counts_enabled (void)
{
  const char *string;
  guint flags = 0;

  string = g_getenv ("GOBJECT_DEBUG");
  if (string != NULL)
    {
      GDebugKey debug_keys[] = {
        { "objects", 1 },
        { "instance-count", 2 },
        { "signals", 4 }
      };

     flags = g_parse_debug_string (string, debug_keys, G_N_ELEMENTS (debug_keys));
    }

  return (flags & 2) != 0;
}

static void
search_changed (BobguiSearchEntry         *entry,
                BobguiInspectorStatistics *sl)
{
  const char *text;
  GListModel *model;

  text = bobgui_editable_get_text (BOBGUI_EDITABLE (entry));
  model = bobgui_single_selection_get_model (sl->priv->selection);

  for (guint i = 0; i < g_list_model_get_n_items (model); i++)
    {
      TypeData *data = g_list_model_get_item (model, i);
      char *string;

      g_object_unref (data);

      string = g_ascii_strdown (g_type_name (data->type), -1);
      if (g_str_has_prefix (string, text))
        {
          g_free (string);
          bobgui_single_selection_set_selected (sl->priv->selection, i);
          return;
        }

       g_free (string);
    }

  bobgui_single_selection_set_selected (sl->priv->selection, BOBGUI_INVALID_LIST_POSITION);
}

static void
root (BobguiWidget *widget)
{
  BobguiInspectorStatistics *sl = BOBGUI_INSPECTOR_STATISTICS (widget);
  BobguiWidget *toplevel;

  BOBGUI_WIDGET_CLASS (bobgui_inspector_statistics_parent_class)->root (widget);

  toplevel = BOBGUI_WIDGET (bobgui_widget_get_root (widget));

  bobgui_search_bar_set_key_capture_widget (BOBGUI_SEARCH_BAR (sl->priv->search_bar), toplevel);
}

static void
unroot (BobguiWidget *widget)
{
  BOBGUI_WIDGET_CLASS (bobgui_inspector_statistics_parent_class)->unroot (widget);
}

static void
setup_label (BobguiSignalListItemFactory *factory,
             BobguiListItem              *list_item)
{
  BobguiWidget *label;

  label = bobgui_label_new (NULL);
  bobgui_label_set_xalign (BOBGUI_LABEL (label), 0.);
  bobgui_list_item_set_child (list_item, label);
}

static void
bind_name (BobguiSignalListItemFactory *factory,
           BobguiListItem              *list_item)
{
  BobguiWidget *label;
  TypeData *data;

  data = bobgui_list_item_get_item (list_item);
  label = bobgui_list_item_get_child (list_item);
  bobgui_label_set_text (BOBGUI_LABEL (label), g_type_name (data->type));
}

static void
set_self1 (TypeData   *data,
           GParamSpec *pspec,
           BobguiWidget  *label)
{
  int count;
  char *text;

  g_object_get (data, "self1", &count, NULL);
  text = g_strdup_printf ("%d", count);
  bobgui_label_set_text (BOBGUI_LABEL (label), text);
  g_free (text);
}

static void
bind_self1 (BobguiSignalListItemFactory *factory,
            BobguiListItem              *list_item)
{
  BobguiWidget *label;
  TypeData *data;

  label = bobgui_list_item_get_child (list_item);
  data = bobgui_list_item_get_item (list_item);

  set_self1 (data, NULL, label);
  g_signal_connect (data, "notify::self1", G_CALLBACK (set_self1), label);
}

static void
unbind_self1 (BobguiSignalListItemFactory *factory,
              BobguiListItem              *list_item)
{
  BobguiWidget *label;
  TypeData *data;

  label = bobgui_list_item_get_child (list_item);
  data = bobgui_list_item_get_item (list_item);

  g_signal_handlers_disconnect_by_func (data, G_CALLBACK (set_self1), label);
}

static void
set_cumulative1 (TypeData   *data,
                 GParamSpec *pspec,
                 BobguiWidget  *label)
{
  int count;
  char *text;

  g_object_get (data, "cumulative1", &count, NULL);
  text = g_strdup_printf ("%d", count);
  bobgui_label_set_text (BOBGUI_LABEL (label), text);
  g_free (text);
}

static void
bind_cumulative1 (BobguiSignalListItemFactory *factory,
                  BobguiListItem              *list_item)
{
  BobguiWidget *label;
  TypeData *data;

  label = bobgui_list_item_get_child (list_item);
  data = bobgui_list_item_get_item (list_item);

  set_cumulative1 (data, NULL, label);
  g_signal_connect (data, "notify::cumulative1", G_CALLBACK (set_cumulative1), label);
}

static void
unbind_cumulative1 (BobguiSignalListItemFactory *factory,
                    BobguiListItem              *list_item)
{
  BobguiWidget *label;
  TypeData *data;

  label = bobgui_list_item_get_child (list_item);
  data = bobgui_list_item_get_item (list_item);

  g_signal_handlers_disconnect_by_func (data, G_CALLBACK (set_cumulative1), label);
}

static void
set_self2 (TypeData   *data,
           GParamSpec *pspec,
           BobguiWidget  *label)
{
  int count1;
  int count2;
  char *text;

  g_object_get (data, "self1", &count1, NULL);
  g_object_get (data, "self2", &count2, NULL);
  if (count2 > count1)
    text = g_strdup_printf ("%d (↗ %d)", count2, count2 - count1);
  else if (count2 < count1)
    text = g_strdup_printf ("%d (↘ %d)", count2, count1 - count2);
  else
    text = g_strdup_printf ("%d", count2);
  bobgui_label_set_text (BOBGUI_LABEL (label), text);
  g_free (text);
}

static void
bind_self2 (BobguiSignalListItemFactory *factory,
            BobguiListItem              *list_item)
{
  BobguiWidget *label;
  TypeData *data;

  label = bobgui_list_item_get_child (list_item);
  data = bobgui_list_item_get_item (list_item);

  set_self2 (data, NULL, label);
  g_signal_connect (data, "notify::self1", G_CALLBACK (set_self2), label);
  g_signal_connect (data, "notify::self2", G_CALLBACK (set_self2), label);
}

static void
unbind_self2 (BobguiSignalListItemFactory *factory,
              BobguiListItem              *list_item)
{
  BobguiWidget *label;
  TypeData *data;

  label = bobgui_list_item_get_child (list_item);
  data = bobgui_list_item_get_item (list_item);

  g_signal_handlers_disconnect_by_func (data, G_CALLBACK (set_self2), label);
}

static void
set_cumulative2 (TypeData   *data,
                 GParamSpec *pspec,
                 BobguiWidget  *label)
{
  int count1;
  int count2;
  char *text;

  g_object_get (data, "cumulative1", &count1, NULL);
  g_object_get (data, "cumulative2", &count2, NULL);
  if (count2 > count1)
    text = g_strdup_printf ("%d (↗ %d)", count2, count2 - count1);
  else if (count2 < count1)
    text = g_strdup_printf ("%d (↘ %d)", count2, count1 - count2);
  else
    text = g_strdup_printf ("%d", count2);
  bobgui_label_set_text (BOBGUI_LABEL (label), text);
  g_free (text);
}

static void
bind_cumulative2 (BobguiSignalListItemFactory *factory,
                  BobguiListItem              *list_item)
{
  BobguiWidget *label;
  TypeData *data;

  label = bobgui_list_item_get_child (list_item);
  data = bobgui_list_item_get_item (list_item);

  set_cumulative2 (data, NULL, label);
  g_signal_connect (data, "notify::cumulative1", G_CALLBACK (set_cumulative2), label);
  g_signal_connect (data, "notify::cumulative2", G_CALLBACK (set_cumulative2), label);
}

static void
unbind_cumulative2 (BobguiSignalListItemFactory *factory,
                    BobguiListItem              *list_item)
{
  BobguiWidget *label;
  TypeData *data;

  label = bobgui_list_item_get_child (list_item);
  data = bobgui_list_item_get_item (list_item);

  g_signal_handlers_disconnect_by_func (data, G_CALLBACK (set_cumulative2), label);
}

static void
setup_graph (BobguiSignalListItemFactory *factory,
             BobguiListItem              *list_item)
{
  bobgui_list_item_set_child (list_item, BOBGUI_WIDGET (graph_renderer_new ()));
}

static void
set_graph_self (TypeData   *data,
                GParamSpec *pspec,
                BobguiWidget  *graph)
{
  graph_renderer_set_data (GRAPH_RENDERER (graph), data->self);
}

static void
bind_graph_self (BobguiSignalListItemFactory *factory,
                 BobguiListItem              *list_item)
{
  BobguiWidget *graph;
  TypeData *data;

  data = bobgui_list_item_get_item (list_item);
  graph = bobgui_list_item_get_child (list_item);

  set_graph_self (data, NULL, graph);
  g_signal_connect (data, "notify::self", G_CALLBACK (set_graph_self), graph);
}

static void
unbind_graph_self (BobguiSignalListItemFactory *factory,
                   BobguiListItem              *list_item)
{
  BobguiWidget *graph;
  TypeData *data;

  data = bobgui_list_item_get_item (list_item);
  graph = bobgui_list_item_get_child (list_item);

  g_signal_handlers_disconnect_by_func (data, G_CALLBACK (set_graph_self), graph);
}

static void
set_graph_cumulative (TypeData   *data,
                      GParamSpec *pspec,
                      BobguiWidget  *graph)
{
  graph_renderer_set_data (GRAPH_RENDERER (graph), data->cumulative);
}

static void
bind_graph_cumulative (BobguiSignalListItemFactory *factory,
                       BobguiListItem              *list_item)
{
  BobguiWidget *graph;
  TypeData *data;

  data = bobgui_list_item_get_item (list_item);
  graph = bobgui_list_item_get_child (list_item);

  set_graph_cumulative (data, NULL, graph);
  g_signal_connect (data, "notify::cumulative", G_CALLBACK (set_graph_cumulative), graph);
}

static void
unbind_graph_cumulative (BobguiSignalListItemFactory *factory,
                         BobguiListItem              *list_item)
{
  BobguiWidget *graph;
  TypeData *data;

  data = bobgui_list_item_get_item (list_item);
  graph = bobgui_list_item_get_child (list_item);

  g_signal_handlers_disconnect_by_func (data, G_CALLBACK (set_graph_cumulative), graph);
}

static void
bobgui_inspector_statistics_init (BobguiInspectorStatistics *sl)
{
  BobguiColumnViewColumn *column;
  BobguiListItemFactory *factory;
  BobguiSorter *sorter;
  BobguiSortListModel *sort_model;

  sl->priv = bobgui_inspector_statistics_get_instance_private (sl);
  bobgui_widget_init_template (BOBGUI_WIDGET (sl));
  sl->priv->types = g_hash_table_new (NULL, NULL);

  sl->priv->data = g_list_store_new (type_data_get_type ());

  sort_model = bobgui_sort_list_model_new (G_LIST_MODEL (sl->priv->data),
                                        g_object_ref (bobgui_column_view_get_sorter (BOBGUI_COLUMN_VIEW (sl->priv->view))));

  sl->priv->selection = bobgui_single_selection_new (G_LIST_MODEL (sort_model));
  bobgui_single_selection_set_can_unselect (sl->priv->selection, TRUE);

  bobgui_column_view_set_model (BOBGUI_COLUMN_VIEW (sl->priv->view), BOBGUI_SELECTION_MODEL (sl->priv->selection));

  g_object_unref (sl->priv->selection);

  column = g_list_model_get_item (bobgui_column_view_get_columns (BOBGUI_COLUMN_VIEW (sl->priv->view)), 0);

  factory = bobgui_signal_list_item_factory_new ();
  g_signal_connect (factory, "setup", G_CALLBACK (setup_label), NULL);
  g_signal_connect (factory, "bind", G_CALLBACK (bind_name), NULL);

  bobgui_column_view_column_set_factory (column, factory);
  sorter = BOBGUI_SORTER (bobgui_string_sorter_new (bobgui_property_expression_new (type_data_get_type (), NULL, "name")));
  bobgui_column_view_column_set_sorter (column, sorter);
  g_object_unref (sorter);
  g_object_unref (factory);
  g_object_unref (column);

  column = g_list_model_get_item (bobgui_column_view_get_columns (BOBGUI_COLUMN_VIEW (sl->priv->view)), 1);

  factory = bobgui_signal_list_item_factory_new ();
  g_signal_connect (factory, "setup", G_CALLBACK (setup_label), NULL);
  g_signal_connect (factory, "bind", G_CALLBACK (bind_self1), NULL);
  g_signal_connect (factory, "unbind", G_CALLBACK (unbind_self1), NULL);

  bobgui_column_view_column_set_factory (column, factory);
  sorter = BOBGUI_SORTER (bobgui_numeric_sorter_new (bobgui_property_expression_new (type_data_get_type (), NULL, "self1")));
  bobgui_column_view_column_set_sorter (column, sorter);
  g_object_unref (sorter);
  g_object_unref (factory);
  g_object_unref (column);

  column = g_list_model_get_item (bobgui_column_view_get_columns (BOBGUI_COLUMN_VIEW (sl->priv->view)), 2);

  factory = bobgui_signal_list_item_factory_new ();
  g_signal_connect (factory, "setup", G_CALLBACK (setup_label), NULL);
  g_signal_connect (factory, "bind", G_CALLBACK (bind_cumulative1), NULL);
  g_signal_connect (factory, "unbind", G_CALLBACK (unbind_cumulative1), NULL);

  bobgui_column_view_column_set_factory (column, factory);
  sorter = BOBGUI_SORTER (bobgui_numeric_sorter_new (bobgui_property_expression_new (type_data_get_type (), NULL, "cumulative1")));
  bobgui_column_view_column_set_sorter (column, sorter);
  g_object_unref (sorter);
  g_object_unref (factory);
  g_object_unref (column);

  column = g_list_model_get_item (bobgui_column_view_get_columns (BOBGUI_COLUMN_VIEW (sl->priv->view)), 3);

  factory = bobgui_signal_list_item_factory_new ();
  g_signal_connect (factory, "setup", G_CALLBACK (setup_label), NULL);
  g_signal_connect (factory, "bind", G_CALLBACK (bind_self2), NULL);
  g_signal_connect (factory, "unbind", G_CALLBACK (unbind_self2), NULL);

  bobgui_column_view_column_set_factory (column, factory);
  sorter = BOBGUI_SORTER (bobgui_numeric_sorter_new (bobgui_property_expression_new (type_data_get_type (), NULL, "self2")));
  bobgui_column_view_column_set_sorter (column, sorter);
  g_object_unref (sorter);
  g_object_unref (factory);
  g_object_unref (column);

  column = g_list_model_get_item (bobgui_column_view_get_columns (BOBGUI_COLUMN_VIEW (sl->priv->view)), 4);

  factory = bobgui_signal_list_item_factory_new ();
  g_signal_connect (factory, "setup", G_CALLBACK (setup_label), NULL);
  g_signal_connect (factory, "bind", G_CALLBACK (bind_cumulative2), NULL);
  g_signal_connect (factory, "unbind", G_CALLBACK (unbind_cumulative2), NULL);

  bobgui_column_view_column_set_factory (column, factory);
  sorter = BOBGUI_SORTER (bobgui_numeric_sorter_new (bobgui_property_expression_new (type_data_get_type (), NULL, "cumulative2")));
  bobgui_column_view_column_set_sorter (column, sorter);
  g_object_unref (sorter);
  g_object_unref (factory);
  g_object_unref (column);

  column = g_list_model_get_item (bobgui_column_view_get_columns (BOBGUI_COLUMN_VIEW (sl->priv->view)), 5);

  factory = bobgui_signal_list_item_factory_new ();
  g_signal_connect (factory, "setup", G_CALLBACK (setup_graph), NULL);
  g_signal_connect (factory, "bind", G_CALLBACK (bind_graph_self), NULL);
  g_signal_connect (factory, "unbind", G_CALLBACK (unbind_graph_self), NULL);

  bobgui_column_view_column_set_factory (column, factory);
  g_object_unref (factory);
  g_object_unref (column);

  column = g_list_model_get_item (bobgui_column_view_get_columns (BOBGUI_COLUMN_VIEW (sl->priv->view)), 6);

  factory = bobgui_signal_list_item_factory_new ();
  g_signal_connect (factory, "setup", G_CALLBACK (setup_graph), NULL);
  g_signal_connect (factory, "bind", G_CALLBACK (bind_graph_cumulative), NULL);
  g_signal_connect (factory, "unbind", G_CALLBACK (unbind_graph_cumulative), NULL);

  bobgui_column_view_column_set_factory (column, factory);
  g_object_unref (factory);
  g_object_unref (column);
}

static void
constructed (GObject *object)
{
  BobguiInspectorStatistics *sl = BOBGUI_INSPECTOR_STATISTICS (object);

  g_signal_connect (sl->priv->button, "toggled", G_CALLBACK (toggle_record), sl);

  if (has_instance_counts ())
    update_type_counts (sl);
  else
    {
      if (instance_counts_enabled ())
        bobgui_label_set_text (BOBGUI_LABEL (sl->priv->excuse), _("GLib must be configured with -Dbuildtype=debug"));
      bobgui_stack_set_visible_child_name (BOBGUI_STACK (sl->priv->stack), "excuse");
      bobgui_widget_set_sensitive (sl->priv->button, FALSE);
    }
}

static void
finalize (GObject *object)
{
  BobguiInspectorStatistics *sl = BOBGUI_INSPECTOR_STATISTICS (object);

  if (sl->priv->update_source_id)
    g_source_remove (sl->priv->update_source_id);

  g_hash_table_unref (sl->priv->types);

  G_OBJECT_CLASS (bobgui_inspector_statistics_parent_class)->finalize (object);
}

static void
get_property (GObject    *object,
              guint       param_id,
              GValue     *value,
              GParamSpec *pspec)
{
  BobguiInspectorStatistics *sl = BOBGUI_INSPECTOR_STATISTICS (object);

  switch (param_id)
    {
    case PROP_BUTTON:
      g_value_take_object (value, sl->priv->button);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

static void
set_property (GObject      *object,
              guint         param_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  BobguiInspectorStatistics *sl = BOBGUI_INSPECTOR_STATISTICS (object);

  switch (param_id)
    {
    case PROP_BUTTON:
      sl->priv->button = g_value_get_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

static void
bobgui_inspector_statistics_class_init (BobguiInspectorStatisticsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);

  object_class->get_property = get_property;
  object_class->set_property = set_property;
  object_class->constructed = constructed;
  object_class->finalize = finalize;

  widget_class->root = root;
  widget_class->unroot = unroot;

  g_object_class_install_property (object_class, PROP_BUTTON,
      g_param_spec_object ("button", NULL, NULL,
                           BOBGUI_TYPE_WIDGET, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

  bobgui_widget_class_set_template_from_resource (widget_class, "/org/bobgui/libbobgui/inspector/statistics.ui");
  bobgui_widget_class_bind_template_child_private (widget_class, BobguiInspectorStatistics, view);
  bobgui_widget_class_bind_template_child_private (widget_class, BobguiInspectorStatistics, stack);
  bobgui_widget_class_bind_template_child_private (widget_class, BobguiInspectorStatistics, search_entry);
  bobgui_widget_class_bind_template_child_private (widget_class, BobguiInspectorStatistics, search_bar);
  bobgui_widget_class_bind_template_child_private (widget_class, BobguiInspectorStatistics, excuse);
  bobgui_widget_class_bind_template_callback (widget_class, search_changed);
}

/* vim:set foldmethod=marker: */
