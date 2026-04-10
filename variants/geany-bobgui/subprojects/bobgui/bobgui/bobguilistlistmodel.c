/*
 * Copyright © 2018 Benjamin Otte
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


/*
 * BobguiListListModel:
 *
 * A list model that takes a list API and provides it as a `GListModel`.
 */

#include "config.h"

#include "bobguilistlistmodelprivate.h"

struct _BobguiListListModel
{
  GObject parent_instance;

  guint n_items;
  gpointer (* get_first) (gpointer);
  gpointer (* get_next) (gpointer, gpointer);
  gpointer (* get_previous) (gpointer, gpointer);
  gpointer (* get_last) (gpointer);
  gpointer (* get_item) (gpointer, gpointer);
  gpointer data;
  GDestroyNotify notify;

  guint cache_pos;
  gpointer cache_item;
};

struct _BobguiListListModelClass
{
  GObjectClass parent_class;
};

enum {
  PROP_0,
  PROP_ITEM_TYPE,
  PROP_N_ITEMS,

  N_PROPS
};

static GParamSpec *properties[N_PROPS] = { NULL, };

static GType
bobgui_list_list_model_get_item_type (GListModel *list)
{
  return G_TYPE_OBJECT;
}

static guint
bobgui_list_list_model_get_n_items (GListModel *list)
{
  BobguiListListModel *self = BOBGUI_LIST_LIST_MODEL (list);

  return self->n_items;
}

static gboolean
bobgui_list_list_model_cache_is_valid (BobguiListListModel *self)
{
  return self->cache_item != NULL;
}

static void
bobgui_list_list_model_invalidate_cache (BobguiListListModel *self)
{
  self->cache_item = NULL;
}

static gpointer
bobgui_list_list_model_get_item (GListModel *list,
                              guint       position)
{
  BobguiListListModel *self = BOBGUI_LIST_LIST_MODEL (list);
  gpointer result;
  guint i;
  guint start, end;

  if (position >= self->n_items)
    return NULL;

  start = 0;
  end = self->n_items;
  if (bobgui_list_list_model_cache_is_valid (self))
    {
      if (self->cache_pos <= position)
        start = self->cache_pos;
      else
        end = self->cache_pos;
    }

  if (self->get_last &&
      position > (start + end) / 2)
    {
      if (end == self->cache_pos && bobgui_list_list_model_cache_is_valid (self))
        result = self->get_previous (self->cache_item, self->data);
      else
        result = self->get_last (self->data);

      for (i = end - 1; i > position; i--)
        {
          result = self->get_previous (result, self->data);
        }
    }
  else
    {
      if (start == self->cache_pos && bobgui_list_list_model_cache_is_valid (self))
        result = self->cache_item;
      else
        result = self->get_first (self->data);

      for (i = start; i < position; i++)
        {
          result = self->get_next (result, self->data);
        }
    }

  self->cache_item = result;
  self->cache_pos = position;

  return self->get_item (result, self->data);
}

static void
bobgui_list_list_model_list_model_init (GListModelInterface *iface)
{
  iface->get_item_type = bobgui_list_list_model_get_item_type;
  iface->get_n_items = bobgui_list_list_model_get_n_items;
  iface->get_item = bobgui_list_list_model_get_item;
}

G_DEFINE_TYPE_WITH_CODE (BobguiListListModel, bobgui_list_list_model,
                         G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (G_TYPE_LIST_MODEL, bobgui_list_list_model_list_model_init))

static void
bobgui_list_list_model_get_property (GObject    *object,
                                  guint       prop_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  BobguiListListModel *self = BOBGUI_LIST_LIST_MODEL (object);

  switch (prop_id)
    {
    case PROP_ITEM_TYPE:
      g_value_set_gtype (value, G_TYPE_OBJECT);
      break;

    case PROP_N_ITEMS:
      g_value_set_uint (value, self->n_items);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_list_list_model_dispose (GObject *object)
{
  BobguiListListModel *self = BOBGUI_LIST_LIST_MODEL (object);

  if (self->notify)
    self->notify (self->data);

  self->n_items = 0;
  self->notify = NULL;

  G_OBJECT_CLASS (bobgui_list_list_model_parent_class)->dispose (object);
}

static void
bobgui_list_list_model_class_init (BobguiListListModelClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = bobgui_list_list_model_get_property;
  object_class->dispose = bobgui_list_list_model_dispose;

  properties[PROP_ITEM_TYPE] =
    g_param_spec_gtype ("item-type", NULL, NULL,
                        G_TYPE_OBJECT,
                        G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  properties[PROP_N_ITEMS] =
    g_param_spec_uint ("n-items", NULL, NULL,
                       0, G_MAXUINT, 0,
                       G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
bobgui_list_list_model_init (BobguiListListModel *self)
{
}

BobguiListListModel *
bobgui_list_list_model_new (gpointer       (* get_first) (gpointer),
                         gpointer       (* get_next) (gpointer, gpointer),
                         gpointer       (* get_previous) (gpointer, gpointer),
                         gpointer       (* get_last) (gpointer),
                         gpointer       (* get_item) (gpointer, gpointer),
                         gpointer       data,
                         GDestroyNotify notify)
{
  guint n_items;
  gpointer item;

  n_items = 0;
  for (item = get_first (data);
       item != NULL;
       item = get_next (item, data))
    n_items++;

  return bobgui_list_list_model_new_with_size (n_items,
                                            get_first,
                                            get_next,
                                            get_previous,
                                            get_last,
                                            get_item,
                                            data,
                                            notify);
}

BobguiListListModel *
bobgui_list_list_model_new_with_size (guint          n_items,
                                   gpointer       (* get_first) (gpointer),
                                   gpointer       (* get_next) (gpointer, gpointer),
                                   gpointer       (* get_previous) (gpointer, gpointer),
                                   gpointer       (* get_last) (gpointer),
                                   gpointer       (* get_item) (gpointer, gpointer),
                                   gpointer       data,
                                   GDestroyNotify notify)
{
  BobguiListListModel *result;

  g_return_val_if_fail (get_first != NULL, NULL);
  g_return_val_if_fail (get_next != NULL, NULL);
  g_return_val_if_fail (get_previous != NULL, NULL);
  g_return_val_if_fail (get_item != NULL, NULL);

  result = g_object_new (BOBGUI_TYPE_LIST_LIST_MODEL, NULL);

  result->n_items = n_items;
  result->get_first = get_first;
  result->get_next = get_next;
  result->get_previous = get_previous;
  result->get_last = get_last;
  result->get_item = get_item;
  result->data = data;
  result->notify = notify;

  return result;
}

static guint
bobgui_list_list_model_find (BobguiListListModel *self,
                          gpointer          item)
{
  guint position;
  gpointer x;

  position = 0;
  for (x = self->get_first (self->data);
       x != item;
       x = self->get_next (x, self->data))
    position++;

  return position;
}

void
bobgui_list_list_model_item_added (BobguiListListModel *self,
                                gpointer          item)
{
  g_return_if_fail (BOBGUI_IS_LIST_LIST_MODEL (self));
  g_return_if_fail (item != NULL);

  bobgui_list_list_model_item_added_at (self, bobgui_list_list_model_find (self, item));
}

void
bobgui_list_list_model_item_added_at (BobguiListListModel *self,
                                   guint             position)
{
  g_return_if_fail (BOBGUI_IS_LIST_LIST_MODEL (self));
  g_return_if_fail (position <= self->n_items);

  self->n_items++;
  if (position <= self->cache_pos)
    self->cache_pos++;

  g_list_model_items_changed (G_LIST_MODEL (self), position, 0, 1);
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_N_ITEMS]);
}

void
bobgui_list_list_model_item_removed (BobguiListListModel *self,
                                  gpointer          previous)
{
  guint position;

  g_return_if_fail (BOBGUI_IS_LIST_LIST_MODEL (self));

  if (previous == NULL)
    position = 0;
  else
    position = 1 + bobgui_list_list_model_find (self, previous);

  bobgui_list_list_model_item_removed_at (self, position);
}

void
bobgui_list_list_model_item_moved (BobguiListListModel *self,
                                gpointer          item,
                                gpointer          previous_previous)
{
  guint position, previous_position;
  guint min, max;

  g_return_if_fail (BOBGUI_IS_LIST_LIST_MODEL (self));
  g_return_if_fail (item != previous_previous);

  position = bobgui_list_list_model_find (self, item);

  if (previous_previous == NULL)
    {
      previous_position = 0;
    }
  else
    {
      previous_position = bobgui_list_list_model_find (self, previous_previous);
      if (position > previous_position)
        previous_position++;
    }

  /* item didn't move */
  if (position == previous_position)
    return;

  min = MIN (position, previous_position);
  max = MAX (position, previous_position) + 1;

  if (self->cache_item == item)
    self->cache_pos = position;
  else if (self->cache_pos >= min && self->cache_pos < max)
    self->cache_pos += (self->cache_pos > position ? 1 : -1);

  g_list_model_items_changed (G_LIST_MODEL (self), min, max - min, max - min);
}

void
bobgui_list_list_model_item_removed_at (BobguiListListModel *self,
                                     guint             position)
{
  g_return_if_fail (BOBGUI_IS_LIST_LIST_MODEL (self));
  g_return_if_fail (position < self->n_items);

  self->n_items -= 1;
  if (position == self->cache_pos)
    bobgui_list_list_model_invalidate_cache (self);
  else if (position < self->cache_pos)
    self->cache_pos--;

  g_list_model_items_changed (G_LIST_MODEL (self), position, 1, 0);
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_N_ITEMS]);
}

void
bobgui_list_list_model_clear (BobguiListListModel *self)
{
  guint n_items;

  g_return_if_fail (BOBGUI_IS_LIST_LIST_MODEL (self));

  n_items = self->n_items;
  
  if (self->notify)
    self->notify (self->data);

  self->n_items = 0;
  self->notify = NULL;

  bobgui_list_list_model_invalidate_cache (self);

  if (n_items > 0)
    {
      g_list_model_items_changed (G_LIST_MODEL (self), 0, n_items, 0);
      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_N_ITEMS]);
    }
}


