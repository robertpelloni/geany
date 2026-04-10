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

#include "focusoverlay.h"

#include "bobguiwidget.h"
#include "bobguiroot.h"
#include "bobguinative.h"

struct _BobguiFocusOverlay
{
  BobguiInspectorOverlay parent_instance;

  GdkRGBA color;
};

struct _BobguiFocusOverlayClass
{
  BobguiInspectorOverlayClass parent_class;
};

G_DEFINE_TYPE (BobguiFocusOverlay, bobgui_focus_overlay, BOBGUI_TYPE_INSPECTOR_OVERLAY)

static void
bobgui_focus_overlay_snapshot (BobguiInspectorOverlay *overlay,
                            BobguiSnapshot         *snapshot,
                            GskRenderNode       *node,
                            BobguiWidget           *widget)
{
  BobguiFocusOverlay *self = BOBGUI_FOCUS_OVERLAY (overlay);
  BobguiWidget *focus;
  graphene_rect_t bounds;

  if (!BOBGUI_IS_NATIVE (widget))
    return;

  focus = bobgui_root_get_focus (BOBGUI_ROOT (bobgui_widget_get_root (widget)));
  if (!focus)
    return;

  if (!bobgui_widget_is_ancestor (focus, widget))
    return;

  if (BOBGUI_WIDGET (bobgui_widget_get_native (focus)) != widget)
    return;

  if (!bobgui_widget_compute_bounds (focus, widget, &bounds))
    return;

  bobgui_snapshot_append_color (snapshot, &self->color, &bounds);
}

static void
bobgui_focus_overlay_queue_draw (BobguiInspectorOverlay *overlay)
{
}

static void
bobgui_focus_overlay_class_init (BobguiFocusOverlayClass *klass)
{
  BobguiInspectorOverlayClass *overlay_class = BOBGUI_INSPECTOR_OVERLAY_CLASS (klass);

  overlay_class->snapshot = bobgui_focus_overlay_snapshot;
  overlay_class->queue_draw = bobgui_focus_overlay_queue_draw;
}

static void
bobgui_focus_overlay_init (BobguiFocusOverlay *self)
{
  self->color = (GdkRGBA) { 0.5, 0.0, 1.0, 0.2 };
}

BobguiInspectorOverlay *
bobgui_focus_overlay_new (void)
{
  BobguiFocusOverlay *self;

  self = g_object_new (BOBGUI_TYPE_FOCUS_OVERLAY, NULL);

  return BOBGUI_INSPECTOR_OVERLAY (self);
}

void
bobgui_focus_overlay_set_color (BobguiFocusOverlay *self,
                             const GdkRGBA   *color)
{
  if (gdk_rgba_equal (&self->color, color))
    return;

  self->color = *color;
  bobgui_inspector_overlay_queue_draw (BOBGUI_INSPECTOR_OVERLAY (self));
}
