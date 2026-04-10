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

#include "updatesoverlay.h"

#include "bobguiwidget.h"
#include "bobguinative.h"

#include "gsk/gskrendernodeprivate.h"

/* duration before we start fading in us */
#define GDK_DRAW_REGION_MIN_DURATION 50 * 1000
/* duration when fade is finished in us */
#define GDK_DRAW_REGION_MAX_DURATION 200 * 1000

typedef struct {
  gint64 timestamp;
  cairo_region_t *region;
} BobguiUpdate;

typedef struct {
  GQueue *updates;
  GskRenderNode *last;
  BobguiWidget *widget;
  guint tick_callback;
  gulong unmap_callback;
} BobguiWidgetUpdates;

struct _BobguiUpdatesOverlay
{
  BobguiInspectorOverlay parent_instance;

  GHashTable *toplevels; /* widget => BobguiWidgetUpdates */
};

struct _BobguiUpdatesOverlayClass
{
  BobguiInspectorOverlayClass parent_class;
};

G_DEFINE_TYPE (BobguiUpdatesOverlay, bobgui_updates_overlay, BOBGUI_TYPE_INSPECTOR_OVERLAY)

static void
bobgui_update_free (gpointer data)
{
  BobguiUpdate *region = data;

  cairo_region_destroy (region->region);
  g_free (region);
}

static void
bobgui_widget_updates_free (gpointer data)
{
  BobguiWidgetUpdates *updates = data;

  g_queue_free_full (updates->updates, bobgui_update_free);
  g_clear_pointer (&updates->last, gsk_render_node_unref);
  g_signal_handler_disconnect (updates->widget, updates->unmap_callback);
  if (updates->tick_callback)
    bobgui_widget_remove_tick_callback (updates->widget, updates->tick_callback);
  updates->tick_callback = 0;

  g_free (updates);
}

static void
bobgui_widget_updates_unmap_widget (BobguiWidget         *widget,
                                 BobguiUpdatesOverlay *self)
{
  g_hash_table_remove (self->toplevels, widget);
}

static gboolean
bobgui_widget_updates_tick (BobguiWidget     *widget,
                         GdkFrameClock *clock,
                         gpointer       data)
{
  BobguiWidgetUpdates *updates = data;
  BobguiUpdate *draw;
  gint64 now;

  now = gdk_frame_clock_get_frame_time (clock);

  for (draw = g_queue_pop_tail (updates->updates);
       draw != NULL && (now - draw->timestamp >= GDK_DRAW_REGION_MAX_DURATION);
       draw = g_queue_pop_tail (updates->updates))
    {
      bobgui_update_free (draw);
    }

  gdk_surface_queue_render (bobgui_native_get_surface (bobgui_widget_get_native (widget)));
  if (draw)
    {
      g_queue_push_tail (updates->updates, draw);
      return G_SOURCE_CONTINUE;
    }
  else
    {
      updates->tick_callback = 0;
      return G_SOURCE_REMOVE;
    }
}

static BobguiWidgetUpdates *
bobgui_update_overlay_lookup_for_widget (BobguiUpdatesOverlay *self,
                                      BobguiWidget         *widget,
                                      gboolean           create)
{
  BobguiWidgetUpdates *updates = g_hash_table_lookup (self->toplevels, widget);

  if (updates || !create)
    return updates;

  updates = g_new0 (BobguiWidgetUpdates, 1);
  updates->updates = g_queue_new ();
  updates->widget = widget;
  updates->unmap_callback = g_signal_connect (widget, "unmap", G_CALLBACK (bobgui_widget_updates_unmap_widget), self);

  g_hash_table_insert (self->toplevels, g_object_ref (widget), updates);
  return updates;
}

static void
bobgui_widget_updates_add (BobguiWidgetUpdates *updates,
                        gint64            timestamp,
                        cairo_region_t   *region)
{
  BobguiUpdate *update;
  GList *l;

  update = g_new0 (BobguiUpdate, 1);
  update->timestamp = timestamp;
  update->region = region;
  for (l = g_queue_peek_head_link (updates->updates); l != NULL; l = l->next)
    {
      BobguiUpdate *u = l->data;
      cairo_region_subtract (u->region, region);
    }
  g_queue_push_head (updates->updates, update);
  if (updates->tick_callback == 0)
    updates->tick_callback = bobgui_widget_add_tick_callback (updates->widget, bobgui_widget_updates_tick, updates, NULL);
}

static void
bobgui_updates_overlay_snapshot (BobguiInspectorOverlay *overlay,
                              BobguiSnapshot         *snapshot,
                              GskRenderNode       *node,
                              BobguiWidget           *widget)
{
  BobguiUpdatesOverlay *self = BOBGUI_UPDATES_OVERLAY (overlay);
  BobguiWidgetUpdates *updates;
  BobguiUpdate *draw;
  gint64 now;
  GList *l;

  if (!BOBGUI_IS_NATIVE (widget))
    return;

  updates = bobgui_update_overlay_lookup_for_widget (self, widget, TRUE);
  now = gdk_frame_clock_get_frame_time (bobgui_widget_get_frame_clock (widget));

  if (updates->last)
    {
      cairo_region_t *diff;

      diff = cairo_region_create ();
      gsk_render_node_diff (updates->last, node, &(GskDiffData) { diff, NULL });
      if (cairo_region_is_empty (diff))
        cairo_region_destroy (diff);
      else
        bobgui_widget_updates_add (updates, now, diff);
    }
  else
    {
      cairo_region_t *region;
      graphene_rect_t bounds;

      gsk_render_node_get_bounds (node, &bounds);
      region = cairo_region_create_rectangle (&(cairo_rectangle_int_t) {
                                                  floor (bounds.origin.x),
                                                  floor (bounds.origin.y),
                                                  ceil (bounds.origin.x + bounds.size.width) - floor (bounds.origin.x),
                                                  ceil (bounds.origin.y + bounds.size.height) - floor (bounds.origin.y)
                                              });
      bobgui_widget_updates_add (updates, now, region);
    }
  g_clear_pointer (&updates->last, gsk_render_node_unref);
  updates->last = gsk_render_node_ref (node);

  for (l = g_queue_peek_head_link (updates->updates); l != NULL; l = l->next)
    {
      double progress;
      guint i;

      draw = l->data;

      if (now - draw->timestamp < GDK_DRAW_REGION_MIN_DURATION)
        progress = 0.0;
      else if (now - draw->timestamp < GDK_DRAW_REGION_MAX_DURATION)
        progress = (double) (now - draw->timestamp - GDK_DRAW_REGION_MIN_DURATION)
                   / (GDK_DRAW_REGION_MAX_DURATION - GDK_DRAW_REGION_MIN_DURATION);
      else
        break;

      for (i = 0; i < cairo_region_num_rectangles (draw->region); i++)
        {
          GdkRectangle rect;

          cairo_region_get_rectangle (draw->region, i, &rect);
          bobgui_snapshot_append_color (snapshot,
                                     &(GdkRGBA) { 1, 0, 0, 0.4 * (1 - progress) },
                                     &GRAPHENE_RECT_INIT(rect.x, rect.y,
                                                         rect.width, rect.height));
        }
    }
}

static void
bobgui_updates_overlay_queue_draw (BobguiInspectorOverlay *overlay)
{
  BobguiUpdatesOverlay *self = BOBGUI_UPDATES_OVERLAY (overlay);
  GHashTableIter iter;
  gpointer widget;

  g_hash_table_iter_init (&iter, self->toplevels);
  while (g_hash_table_iter_next (&iter, &widget, NULL))
    gdk_surface_queue_render (bobgui_native_get_surface (bobgui_widget_get_native (widget)));
}

static void
bobgui_updates_overlay_dispose (GObject *object)
{
  BobguiUpdatesOverlay *self = BOBGUI_UPDATES_OVERLAY (object);

  g_hash_table_unref (self->toplevels);

  G_OBJECT_CLASS (bobgui_updates_overlay_parent_class)->dispose (object);
}

static void
bobgui_updates_overlay_class_init (BobguiUpdatesOverlayClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  BobguiInspectorOverlayClass *overlay_class = BOBGUI_INSPECTOR_OVERLAY_CLASS (klass);

  overlay_class->snapshot = bobgui_updates_overlay_snapshot;
  overlay_class->queue_draw = bobgui_updates_overlay_queue_draw;

  gobject_class->dispose = bobgui_updates_overlay_dispose;
}

static void
bobgui_updates_overlay_init (BobguiUpdatesOverlay *self)
{
  self->toplevels = g_hash_table_new_full (g_direct_hash, g_direct_equal, g_object_unref, bobgui_widget_updates_free);
}

BobguiInspectorOverlay *
bobgui_updates_overlay_new (void)
{
  return g_object_new (BOBGUI_TYPE_UPDATES_OVERLAY, NULL);
}

