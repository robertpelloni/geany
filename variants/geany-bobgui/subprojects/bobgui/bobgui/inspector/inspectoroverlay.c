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

#include "inspectoroverlay.h"


G_DEFINE_ABSTRACT_TYPE (BobguiInspectorOverlay, bobgui_inspector_overlay, G_TYPE_OBJECT)

static void
bobgui_inspector_overlay_default_snapshot (BobguiInspectorOverlay *self,
                                        BobguiSnapshot         *snapshot,
                                        GskRenderNode       *node,
                                        BobguiWidget           *widget)
{
}

static void
bobgui_inspector_overlay_default_queue_draw (BobguiInspectorOverlay *self)
{
}

static void
bobgui_inspector_overlay_class_init (BobguiInspectorOverlayClass *class)
{
  class->snapshot = bobgui_inspector_overlay_default_snapshot;
  class->queue_draw = bobgui_inspector_overlay_default_queue_draw;
}

static void
bobgui_inspector_overlay_init (BobguiInspectorOverlay *self)
{
}

void
bobgui_inspector_overlay_snapshot (BobguiInspectorOverlay *self,
                                BobguiSnapshot         *snapshot,
                                GskRenderNode       *node,
                                BobguiWidget           *widget)
{
  bobgui_snapshot_push_debug (snapshot, "%s %p", G_OBJECT_TYPE_NAME (self), self);

  BOBGUI_INSPECTOR_OVERLAY_GET_CLASS (self)->snapshot (self, snapshot, node, widget);

  bobgui_snapshot_pop (snapshot);
}

void
bobgui_inspector_overlay_queue_draw (BobguiInspectorOverlay *self)
{
  BOBGUI_INSPECTOR_OVERLAY_GET_CLASS (self)->queue_draw (self);
}

