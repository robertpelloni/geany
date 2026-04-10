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

#include "config.h"

#include "bobguirendernodepaintableprivate.h"

#include "bobguisnapshot.h"

struct _BobguiRenderNodePaintable
{
  GObject parent_instance;

  GskRenderNode *node;
  graphene_rect_t bounds;
};

struct _BobguiRenderNodePaintableClass
{
  GObjectClass parent_class;
};

static void
bobgui_render_node_paintable_paintable_snapshot (GdkPaintable *paintable,
                                              GdkSnapshot  *snapshot,
                                              double        width,
                                              double        height)
{
  BobguiRenderNodePaintable *self = BOBGUI_RENDER_NODE_PAINTABLE (paintable);

  if (self->bounds.size.width <= 0 ||
      self->bounds.size.height <= 0 ||
      self->node == NULL)
    return;

  bobgui_snapshot_save (snapshot);

  bobgui_snapshot_scale (snapshot,
                      width / ceilf (self->bounds.size.width),
                      height / ceilf (self->bounds.size.height));
  bobgui_snapshot_translate (snapshot, &GRAPHENE_POINT_INIT (-self->bounds.origin.x, -self->bounds.origin.y));

  bobgui_snapshot_push_clip (snapshot, &self->bounds);

  bobgui_snapshot_append_node (snapshot, self->node);
  //bobgui_snapshot_append_color (snapshot, &(GdkRGBA) { 1, 0, 0, 1 }, &self->bounds);

  bobgui_snapshot_pop (snapshot);

  bobgui_snapshot_restore (snapshot);
}

static GdkPaintableFlags
bobgui_render_node_paintable_paintable_get_flags (GdkPaintable *paintable)
{
  return GDK_PAINTABLE_STATIC_CONTENTS | GDK_PAINTABLE_STATIC_SIZE;
}

static int
bobgui_render_node_paintable_paintable_get_intrinsic_width (GdkPaintable *paintable)
{
  BobguiRenderNodePaintable *self = BOBGUI_RENDER_NODE_PAINTABLE (paintable);

  return ceilf (self->bounds.size.width);
}

static int
bobgui_render_node_paintable_paintable_get_intrinsic_height (GdkPaintable *paintable)
{
  BobguiRenderNodePaintable *self = BOBGUI_RENDER_NODE_PAINTABLE (paintable);

  return ceilf (self->bounds.size.height);
}

static void
bobgui_render_node_paintable_paintable_init (GdkPaintableInterface *iface)
{
  iface->snapshot = bobgui_render_node_paintable_paintable_snapshot;
  iface->get_flags = bobgui_render_node_paintable_paintable_get_flags;
  iface->get_intrinsic_width = bobgui_render_node_paintable_paintable_get_intrinsic_width;
  iface->get_intrinsic_height = bobgui_render_node_paintable_paintable_get_intrinsic_height;
}

G_DEFINE_TYPE_EXTENDED (BobguiRenderNodePaintable, bobgui_render_node_paintable, G_TYPE_OBJECT, 0,
                        G_IMPLEMENT_INTERFACE (GDK_TYPE_PAINTABLE,
                                               bobgui_render_node_paintable_paintable_init))

static void
bobgui_render_node_paintable_dispose (GObject *object)
{
  BobguiRenderNodePaintable *self = BOBGUI_RENDER_NODE_PAINTABLE (object);

  g_clear_pointer (&self->node, gsk_render_node_unref);

  G_OBJECT_CLASS (bobgui_render_node_paintable_parent_class)->dispose (object);
}

static void
bobgui_render_node_paintable_class_init (BobguiRenderNodePaintableClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->dispose = bobgui_render_node_paintable_dispose;
}

static void
bobgui_render_node_paintable_init (BobguiRenderNodePaintable *self)
{
}

GdkPaintable *
bobgui_render_node_paintable_new (GskRenderNode         *node,
                               const graphene_rect_t *bounds)
{
  BobguiRenderNodePaintable *self;

  g_return_val_if_fail (node == NULL || GSK_IS_RENDER_NODE (node), NULL);
  g_return_val_if_fail (bounds != NULL, NULL);

  self = g_object_new (BOBGUI_TYPE_RENDER_NODE_PAINTABLE, NULL);

  self->node = node ? gsk_render_node_ref (node) : NULL;
  self->bounds = *bounds;

  return GDK_PAINTABLE (self);
}

GskRenderNode *
bobgui_render_node_paintable_get_render_node (BobguiRenderNodePaintable *self)
{
  g_return_val_if_fail (BOBGUI_IS_RENDER_NODE_PAINTABLE (self), NULL);

  return self->node;
}
