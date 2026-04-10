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

#include "highlightoverlay.h"

#include "bobguiwidget.h"

struct _BobguiHighlightOverlay
{
  BobguiInspectorOverlay parent_instance;

  BobguiWidget *widget;
  GdkRGBA color;
};

struct _BobguiHighlightOverlayClass
{
  BobguiInspectorOverlayClass parent_class;
};

G_DEFINE_TYPE (BobguiHighlightOverlay, bobgui_highlight_overlay, BOBGUI_TYPE_INSPECTOR_OVERLAY)

static void
bobgui_highlight_overlay_snapshot (BobguiInspectorOverlay *overlay,
                                BobguiSnapshot         *snapshot,
                                GskRenderNode       *node,
                                BobguiWidget           *widget)
{
  BobguiHighlightOverlay *self = BOBGUI_HIGHLIGHT_OVERLAY (overlay);
  graphene_rect_t bounds;

  if (!bobgui_widget_compute_bounds (self->widget, widget, &bounds))
    return;

  bobgui_snapshot_append_color (snapshot,
                             &self->color,
                             &bounds);
}

static void
bobgui_highlight_overlay_queue_draw (BobguiInspectorOverlay *overlay)
{
  BobguiHighlightOverlay *self = BOBGUI_HIGHLIGHT_OVERLAY (overlay);

  bobgui_widget_queue_draw (self->widget);
}

static void
bobgui_highlight_overlay_dispose (GObject *object)
{
  BobguiHighlightOverlay *self = BOBGUI_HIGHLIGHT_OVERLAY (object);

  g_clear_object (&self->widget);

  G_OBJECT_CLASS (bobgui_highlight_overlay_parent_class)->dispose (object);
}

static void
bobgui_highlight_overlay_class_init (BobguiHighlightOverlayClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  BobguiInspectorOverlayClass *overlay_class = BOBGUI_INSPECTOR_OVERLAY_CLASS (klass);

  overlay_class->snapshot = bobgui_highlight_overlay_snapshot;
  overlay_class->queue_draw = bobgui_highlight_overlay_queue_draw;

  gobject_class->dispose = bobgui_highlight_overlay_dispose;
}

static void
bobgui_highlight_overlay_init (BobguiHighlightOverlay *self)
{
  self->color = (GdkRGBA) { 0.0, 0.0, 1.0, 0.2 };
}

BobguiInspectorOverlay *
bobgui_highlight_overlay_new (BobguiWidget *widget)
{
  BobguiHighlightOverlay *self;

  self = g_object_new (BOBGUI_TYPE_HIGHLIGHT_OVERLAY, NULL);

  self->widget = g_object_ref (widget);

  return BOBGUI_INSPECTOR_OVERLAY (self);
}

BobguiWidget *
bobgui_highlight_overlay_get_widget (BobguiHighlightOverlay *self)
{
  return self->widget;
}

void
bobgui_highlight_overlay_set_color (BobguiHighlightOverlay *self,
                                 const GdkRGBA       *color)
{
  if (gdk_rgba_equal (&self->color, color))
    return;

  self->color = *color;
  bobgui_inspector_overlay_queue_draw (BOBGUI_INSPECTOR_OVERLAY (self));
}
