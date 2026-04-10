/*
 * Copyright © 2019 Benjamin Otte
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Benjamin Otte <otte@gnome.org>
 */

#include "config.h"

#include "bobguimultifilter.h"

#include "bobguifilterprivate.h"
#include "bobguibuildable.h"
#include "bobguitypebuiltins.h"

#define GDK_ARRAY_TYPE_NAME BobguiFilters
#define GDK_ARRAY_NAME bobgui_filters
#define GDK_ARRAY_ELEMENT_TYPE BobguiFilter *
#define GDK_ARRAY_FREE_FUNC g_object_unref

#include "gdk/gdkarrayimpl.c"

/*** MULTI FILTER ***/

/**
 * BobguiMultiFilter:
 *
 * Base class for filters that combine multiple filters.
 */

/**
 * BobguiAnyFilter:
 *
 * Matches an item when at least one of its filters matches.
 *
 * To add filters to a `BobguiAnyFilter`, use [method@Bobgui.MultiFilter.append].
 */

/**
 * BobguiEveryFilter:
 *
 * Matches an item when each of its filters matches.
 *
 * To add filters to a `BobguiEveryFilter`, use [method@Bobgui.MultiFilter.append].
 */

struct _BobguiMultiFilter
{
  BobguiFilter parent_instance;

  BobguiFilters filters;
};

struct _BobguiMultiFilterClass
{
  BobguiFilterClass parent_class;

  BobguiFilterChange addition_change;
  BobguiFilterChange removal_change;
};

enum {
  PROP_0,
  PROP_ITEM_TYPE,
  PROP_N_ITEMS,

  N_PROPS
};

static GParamSpec *properties[N_PROPS] = { NULL, };

static GType
bobgui_multi_filter_get_item_type (GListModel *list)
{
  return BOBGUI_TYPE_FILTER;
}

static guint
bobgui_multi_filter_get_n_items (GListModel *list)
{
  BobguiMultiFilter *self = BOBGUI_MULTI_FILTER (list);

  return bobgui_filters_get_size (&self->filters);
}

static gpointer
bobgui_multi_filter_get_item (GListModel *list,
                           guint       position)
{
  BobguiMultiFilter *self = BOBGUI_MULTI_FILTER (list);

  if (position < bobgui_filters_get_size (&self->filters))
    return g_object_ref (bobgui_filters_get (&self->filters, position));
  else
    return NULL;
}

static void
bobgui_multi_filter_list_model_init (GListModelInterface *iface)
{
  iface->get_item_type = bobgui_multi_filter_get_item_type;
  iface->get_n_items = bobgui_multi_filter_get_n_items;
  iface->get_item = bobgui_multi_filter_get_item;
}

static BobguiBuildableIface *parent_buildable_iface;

static void
bobgui_multi_filter_buildable_add_child (BobguiBuildable *buildable,
                                      BobguiBuilder   *builder,
                                      GObject      *child,
                                      const char   *type)
{
  if (BOBGUI_IS_FILTER (child))
    bobgui_multi_filter_append (BOBGUI_MULTI_FILTER (buildable), g_object_ref (BOBGUI_FILTER (child)));
  else
    parent_buildable_iface->add_child (buildable, builder, child, type);
}

static void
bobgui_multi_filter_buildable_init (BobguiBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);

  iface->add_child = bobgui_multi_filter_buildable_add_child;
}

G_DEFINE_ABSTRACT_TYPE_WITH_CODE (BobguiMultiFilter, bobgui_multi_filter, BOBGUI_TYPE_FILTER,
                                  G_IMPLEMENT_INTERFACE (G_TYPE_LIST_MODEL, bobgui_multi_filter_list_model_init)
                                  G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_BUILDABLE, bobgui_multi_filter_buildable_init))

static void
bobgui_multi_filter_changed_cb (BobguiFilter       *filter,
                             BobguiFilterChange  change,
                             BobguiMultiFilter  *self)
{
  bobgui_filter_changed (BOBGUI_FILTER (self), change);
}

static void
bobgui_multi_filter_get_property (GObject    *object,
                               guint       prop_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  BobguiMultiFilter *self = BOBGUI_MULTI_FILTER (object);

  switch (prop_id)
    {
    case PROP_ITEM_TYPE:
      g_value_set_gtype (value, BOBGUI_TYPE_FILTER);
      break;

    case PROP_N_ITEMS:
      g_value_set_uint (value, bobgui_filters_get_size (&self->filters));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_multi_filter_dispose (GObject *object)
{
  BobguiMultiFilter *self = BOBGUI_MULTI_FILTER (object);
  guint i;

  for (i = 0; i < bobgui_filters_get_size (&self->filters); i++)
    {
      BobguiFilter *filter = bobgui_filters_get (&self->filters, i);
      g_signal_handlers_disconnect_by_func (filter, bobgui_multi_filter_changed_cb, self);
    }

  bobgui_filters_clear (&self->filters);

  G_OBJECT_CLASS (bobgui_multi_filter_parent_class)->dispose (object);
}

typedef struct _MultiFilterWatchData {
  GHashTable *filter_to_watch;
  BobguiFilterWatchCallback callback;

  gpointer user_data;
  GDestroyNotify destroy;
} MultiFilterWatchData;

static void
multi_filter_watch_cb (gpointer item,
                       gpointer user_data)
{
  MultiFilterWatchData *data = (MultiFilterWatchData *) user_data;
  data->callback (item, data->user_data);
}

static gpointer
bobgui_multi_filter_watch (BobguiFilter              *filter,
                        gpointer                item,
                        BobguiFilterWatchCallback  callback,
                        gpointer                user_data,
                        GDestroyNotify          destroy)
{
  MultiFilterWatchData *data;
  BobguiMultiFilter *self;

  self = BOBGUI_MULTI_FILTER (filter);

  data = g_new0 (MultiFilterWatchData, 1);
  data->callback = callback;
  data->user_data = user_data;
  data->destroy = destroy;

  data->filter_to_watch = g_hash_table_new (g_direct_hash, g_direct_equal);
  for (size_t i = 0; i < bobgui_filters_get_size (&self->filters); i++)
    {
      BobguiFilter *child = bobgui_filters_get (&self->filters, i);

      g_hash_table_insert (data->filter_to_watch,
                           child,
                           bobgui_filter_watch (child, item,
                                             multi_filter_watch_cb,
                                             data,
                                             NULL));
    }

  return g_steal_pointer (&data);
}

static void
bobgui_multi_filter_unwatch (BobguiFilter *filter,
                          gpointer   watch)
{
  MultiFilterWatchData *data = (MultiFilterWatchData *) watch;
  GHashTableIter iter;
  gpointer child_filter;
  gpointer child_watch;

  g_assert (data->filter_to_watch != NULL);

  g_hash_table_iter_init (&iter, data->filter_to_watch);
  while (g_hash_table_iter_next (&iter, &child_filter, &child_watch) && child_watch)
    bobgui_filter_unwatch (child_filter, child_watch);

  g_clear_pointer (&data->filter_to_watch, g_hash_table_destroy);
  g_free (data);
}

static void
bobgui_multi_filter_class_init (BobguiMultiFilterClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  BobguiFilterClassPrivate *filter_class_priv = G_TYPE_CLASS_GET_PRIVATE (class, BOBGUI_TYPE_FILTER, BobguiFilterClassPrivate);

  object_class->get_property = bobgui_multi_filter_get_property;
  object_class->dispose = bobgui_multi_filter_dispose;

  filter_class_priv->watch = bobgui_multi_filter_watch;
  filter_class_priv->unwatch = bobgui_multi_filter_unwatch;

  /**
   * BobguiMultiFilter:item-type:
   *
   * The type of items.
   *
   * See [method@Gio.ListModel.get_item_type].
   *
   * Since: 4.8
   */
  properties[PROP_ITEM_TYPE] =
    g_param_spec_gtype ("item-type", NULL, NULL,
                        BOBGUI_TYPE_FILTER,
                        G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  /**
   * BobguiMultiFilter:n-items:
   *
   * The number of items.
   *
   * See [method@Gio.ListModel.get_n_items].
   *
   * Since: 4.8
   */
  properties[PROP_N_ITEMS] =
    g_param_spec_uint ("n-items", NULL, NULL,
                       0, G_MAXUINT, 0,
                       G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
bobgui_multi_filter_init (BobguiMultiFilter *self)
{
  bobgui_filters_init (&self->filters);
}

/**
 * bobgui_multi_filter_append:
 * @self: a multi filter
 * @filter: (transfer full): a filter to add
 *
 * Adds a filter.
 */
void
bobgui_multi_filter_append (BobguiMultiFilter *self,
                         BobguiFilter    *filter)
{
  g_return_if_fail (BOBGUI_IS_MULTI_FILTER (self));
  g_return_if_fail (BOBGUI_IS_FILTER (filter));

  g_signal_connect (filter, "changed", G_CALLBACK (bobgui_multi_filter_changed_cb), self);
  bobgui_filters_append (&self->filters, filter);
  g_list_model_items_changed (G_LIST_MODEL (self), bobgui_filters_get_size (&self->filters) - 1, 0, 1);
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_N_ITEMS]);

  bobgui_filter_changed (BOBGUI_FILTER (self),
                      BOBGUI_MULTI_FILTER_GET_CLASS (self)->addition_change);
}

/**
 * bobgui_multi_filter_remove:
 * @self: a multi filter
 * @position: position of filter to remove
 *
 * Removes a filter.
 *
 * If @position is larger than the number of filters,
 * nothing happens.
 **/
void
bobgui_multi_filter_remove (BobguiMultiFilter *self,
                         guint           position)
{
  guint length;
  BobguiFilter *filter;

  length = bobgui_filters_get_size (&self->filters);
  if (position >= length)
    return;

  filter = bobgui_filters_get (&self->filters, position);
  g_signal_handlers_disconnect_by_func (filter, bobgui_multi_filter_changed_cb, self);
  bobgui_filters_splice (&self->filters, position, 1, FALSE, NULL, 0);
  g_list_model_items_changed (G_LIST_MODEL (self), position, 1, 0);
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_N_ITEMS]);

  bobgui_filter_changed (BOBGUI_FILTER (self),
                      BOBGUI_MULTI_FILTER_GET_CLASS (self)->removal_change);
}

/*** ANY FILTER ***/

struct _BobguiAnyFilter
{
  BobguiMultiFilter parent_instance;
};

struct _BobguiAnyFilterClass
{
  BobguiMultiFilterClass parent_class;
};

G_DEFINE_TYPE (BobguiAnyFilter, bobgui_any_filter, BOBGUI_TYPE_MULTI_FILTER)

static gboolean
bobgui_any_filter_match (BobguiFilter *filter,
                      gpointer   item)
{
  BobguiMultiFilter *self = BOBGUI_MULTI_FILTER (filter);
  guint i;

  for (i = 0; i < bobgui_filters_get_size (&self->filters); i++)
    {
      BobguiFilter *child = bobgui_filters_get (&self->filters, i);

      if (bobgui_filter_match (child, item))
        return TRUE;
    }

  return FALSE;
}

static BobguiFilterMatch
bobgui_any_filter_get_strictness (BobguiFilter *filter)
{
  BobguiMultiFilter *self = BOBGUI_MULTI_FILTER (filter);
  guint i;
  BobguiFilterMatch result = BOBGUI_FILTER_MATCH_NONE;

  for (i = 0; i < bobgui_filters_get_size (&self->filters); i++)
    {
      BobguiFilter *child = bobgui_filters_get (&self->filters, i);

      switch (bobgui_filter_get_strictness (child))
      {
        case BOBGUI_FILTER_MATCH_SOME:
          result = BOBGUI_FILTER_MATCH_SOME;
          break;
        case BOBGUI_FILTER_MATCH_NONE:
          break;
        case BOBGUI_FILTER_MATCH_ALL:
          return BOBGUI_FILTER_MATCH_ALL;
        default:
          g_return_val_if_reached (BOBGUI_FILTER_MATCH_NONE);
          break;
      }
    }

  return result;
}

static void
bobgui_any_filter_class_init (BobguiAnyFilterClass *class)
{
  BobguiMultiFilterClass *multi_filter_class = BOBGUI_MULTI_FILTER_CLASS (class);
  BobguiFilterClass *filter_class = BOBGUI_FILTER_CLASS (class);

  multi_filter_class->addition_change = BOBGUI_FILTER_CHANGE_LESS_STRICT_REWATCH;
  multi_filter_class->removal_change = BOBGUI_FILTER_CHANGE_MORE_STRICT_REWATCH;

  filter_class->match = bobgui_any_filter_match;
  filter_class->get_strictness = bobgui_any_filter_get_strictness;
}

static void
bobgui_any_filter_init (BobguiAnyFilter *self)
{
}

/**
 * bobgui_any_filter_new:
 *
 * Creates a new empty "any" filter.
 *
 * Use [method@Bobgui.MultiFilter.append] to add filters to it.
 *
 * This filter matches an item if any of the filters added to it
 * matches the item. In particular, this means that if no filter
 * has been added to it, the filter matches no item.
 *
 * Returns: a new `BobguiAnyFilter`
 */
BobguiAnyFilter *
bobgui_any_filter_new (void)
{
  return g_object_new (BOBGUI_TYPE_ANY_FILTER, NULL);
}

/*** EVERY FILTER ***/

struct _BobguiEveryFilter
{
  BobguiMultiFilter parent_instance;
};

struct _BobguiEveryFilterClass
{
  BobguiMultiFilterClass parent_class;
};

G_DEFINE_TYPE (BobguiEveryFilter, bobgui_every_filter, BOBGUI_TYPE_MULTI_FILTER)

static gboolean
bobgui_every_filter_match (BobguiFilter *filter,
                        gpointer   item)
{
  BobguiMultiFilter *self = BOBGUI_MULTI_FILTER (filter);
  guint i;

  for (i = 0; i < bobgui_filters_get_size (&self->filters); i++)
    {
      BobguiFilter *child = bobgui_filters_get (&self->filters, i);

      if (!bobgui_filter_match (child, item))
        return FALSE;
    }

  return TRUE;
}

static BobguiFilterMatch
bobgui_every_filter_get_strictness (BobguiFilter *filter)
{
  BobguiMultiFilter *self = BOBGUI_MULTI_FILTER (filter);
  guint i;
  BobguiFilterMatch result = BOBGUI_FILTER_MATCH_ALL;

  for (i = 0; i < bobgui_filters_get_size (&self->filters); i++)
    {
      BobguiFilter *child = bobgui_filters_get (&self->filters, i);

      switch (bobgui_filter_get_strictness (child))
      {
        case BOBGUI_FILTER_MATCH_SOME:
          result = BOBGUI_FILTER_MATCH_SOME;
          break;
        case BOBGUI_FILTER_MATCH_NONE:
          return BOBGUI_FILTER_MATCH_NONE;
        case BOBGUI_FILTER_MATCH_ALL:
          break;
        default:
          g_return_val_if_reached (BOBGUI_FILTER_MATCH_NONE);
          break;
      }
    }

  return result;
}

static void
bobgui_every_filter_class_init (BobguiEveryFilterClass *class)
{
  BobguiMultiFilterClass *multi_filter_class = BOBGUI_MULTI_FILTER_CLASS (class);
  BobguiFilterClass *filter_class = BOBGUI_FILTER_CLASS (class);

  multi_filter_class->addition_change = BOBGUI_FILTER_CHANGE_MORE_STRICT_REWATCH;
  multi_filter_class->removal_change = BOBGUI_FILTER_CHANGE_LESS_STRICT_REWATCH;

  filter_class->match = bobgui_every_filter_match;
  filter_class->get_strictness = bobgui_every_filter_get_strictness;
}

static void
bobgui_every_filter_init (BobguiEveryFilter *self)
{
}

/**
 * bobgui_every_filter_new:
 *
 * Creates a new empty "every" filter.
 *
 * Use [method@Bobgui.MultiFilter.append] to add filters to it.
 *
 * This filter matches an item if each of the filters added to it
 * matches the item. In particular, this means that if no filter
 * has been added to it, the filter matches every item.
 *
 * Returns: a new `BobguiEveryFilter`
 */
BobguiEveryFilter *
bobgui_every_filter_new (void)
{
  return g_object_new (BOBGUI_TYPE_EVERY_FILTER, NULL);
}

