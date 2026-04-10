/*
 * Copyright © 2023 Benjamin Otte
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

#include "bobguilistheaderbaseprivate.h"

typedef struct _BobguiListHeaderBasePrivate BobguiListHeaderBasePrivate;
struct _BobguiListHeaderBasePrivate
{
  GObject *item;
  guint start;
  guint end;
};

G_DEFINE_TYPE_WITH_PRIVATE (BobguiListHeaderBase, bobgui_list_header_base, BOBGUI_TYPE_WIDGET)

static void
bobgui_list_header_base_default_update (BobguiListHeaderBase *self,
                                     gpointer           item,
                                     guint              start,
                                     guint              end)
{
  BobguiListHeaderBasePrivate *priv = bobgui_list_header_base_get_instance_private (self);

  g_set_object (&priv->item, item);
  priv->start = start;
  priv->end = end;
}

static void
bobgui_list_header_base_dispose (GObject *object)
{
  BobguiListHeaderBase *self = BOBGUI_LIST_HEADER_BASE (object);
  BobguiListHeaderBasePrivate *priv = bobgui_list_header_base_get_instance_private (self);

  g_clear_object (&priv->item);

  G_OBJECT_CLASS (bobgui_list_header_base_parent_class)->dispose (object);
}

static void
bobgui_list_header_base_class_init (BobguiListHeaderBaseClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  klass->update = bobgui_list_header_base_default_update;

  gobject_class->dispose = bobgui_list_header_base_dispose;
}

static void
bobgui_list_header_base_init (BobguiListHeaderBase *self)
{
}

void
bobgui_list_header_base_update (BobguiListHeaderBase *self,
                             gpointer           item,
                             guint              start,
                             guint              end)
{
  BobguiListHeaderBasePrivate *priv = bobgui_list_header_base_get_instance_private (self);

  if (priv->item == item &&
      priv->start == start && 
      priv->end == end)
    return;

  BOBGUI_LIST_HEADER_BASE_GET_CLASS (self)->update (self, item, start, end);
}

guint
bobgui_list_header_base_get_start (BobguiListHeaderBase *self)
{
  BobguiListHeaderBasePrivate *priv = bobgui_list_header_base_get_instance_private (self);

  return priv->start;
}

guint
bobgui_list_header_base_get_end (BobguiListHeaderBase *self)
{
  BobguiListHeaderBasePrivate *priv = bobgui_list_header_base_get_instance_private (self);

  return priv->end;
}

gpointer
bobgui_list_header_base_get_item (BobguiListHeaderBase *self)
{
  BobguiListHeaderBasePrivate *priv = bobgui_list_header_base_get_instance_private (self);

  return priv->item;
}

