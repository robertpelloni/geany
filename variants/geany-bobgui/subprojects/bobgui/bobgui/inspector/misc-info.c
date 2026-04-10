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
#include <glib/gi18n-lib.h>

#include "misc-info.h"

#include "measuregraph.h"
#include "window.h"
#include "type-info.h"

#include "bobguitypebuiltins.h"
#include "bobguibuildable.h"
#include "bobguilabel.h"
#include "bobguiframe.h"
#include "bobguibutton.h"
#include "bobguimenubutton.h"
#include "bobguiwidgetprivate.h"
#include "bobguibinlayout.h"
#include "bobguiwidgetprivate.h"
#include "gdk/gdksurfaceprivate.h"

struct _BobguiInspectorMiscInfo
{
  BobguiWidget parent;

  GObject *object;

  BobguiWidget *swin;
  BobguiWidget *address;
  BobguiWidget *type;
  BobguiWidget *type_popover;
  BobguiWidget *refcount_row;
  BobguiWidget *refcount;
  BobguiWidget *state_row;
  BobguiWidget *state;
  BobguiWidget *direction_row;
  BobguiWidget *direction;
  BobguiWidget *buildable_id_row;
  BobguiWidget *buildable_id;
  BobguiWidget *mnemonic_label_row;
  BobguiWidget *mnemonic_label;
  BobguiWidget *request_mode_row;
  BobguiWidget *request_mode;
  BobguiWidget *measure_info_row;
  BobguiWidget *measure_row;
  BobguiWidget *measure_expand_toggle;
  BobguiWidget *measure_picture;
  GdkPaintable *measure_graph;
  BobguiWidget *bounds_row;
  BobguiWidget *bounds;
  BobguiWidget *baseline_row;
  BobguiWidget *baseline;
  BobguiWidget *surface_row;
  BobguiWidget *surface;
  BobguiWidget *surface_button;
  BobguiWidget *renderer_row;
  BobguiWidget *renderer;
  BobguiWidget *renderer_button;
  BobguiWidget *frame_clock_row;
  BobguiWidget *frame_clock;
  BobguiWidget *frame_clock_button;
  BobguiWidget *tick_callback_row;
  BobguiWidget *tick_callback;
  BobguiWidget *framerate_row;
  BobguiWidget *framerate;
  BobguiWidget *scale_row;
  BobguiWidget *scale;
  BobguiWidget *color_state_row;
  BobguiWidget *color_state;
  BobguiWidget *framecount_row;
  BobguiWidget *framecount;
  BobguiWidget *mapped_row;
  BobguiWidget *mapped;
  BobguiWidget *realized_row;
  BobguiWidget *realized;
  BobguiWidget *is_toplevel_row;
  BobguiWidget *is_toplevel;
  BobguiWidget *child_visible_row;
  BobguiWidget *child_visible;
  BobguiWidget *intrinsic_size_row;
  BobguiWidget *intrinsic_size;
  BobguiWidget *aspect_ratio_row;
  BobguiWidget *aspect_ratio;
  BobguiWidget *paintable_flags_row;
  BobguiWidget *paintable_flags;

  guint update_source_id;
  gint64 last_frame;
};

typedef struct _BobguiInspectorMiscInfoClass
{
  BobguiWidgetClass parent_class;
} BobguiInspectorMiscInfoClass;

G_DEFINE_TYPE (BobguiInspectorMiscInfo, bobgui_inspector_misc_info, BOBGUI_TYPE_WIDGET)

static char *
format_state_flags (BobguiStateFlags state)
{
  GFlagsClass *fclass;
  GString *str;
  int i;

  str = g_string_new ("");

  if (state)
    {
      fclass = g_type_class_ref (BOBGUI_TYPE_STATE_FLAGS);
      for (i = 0; i < fclass->n_values; i++)
        {
          if (state & fclass->values[i].value)
            {
              if (str->len)
                g_string_append (str, " | ");
              g_string_append (str, fclass->values[i].value_nick);
            }
        }
      g_type_class_unref (fclass);
    }
  else
    g_string_append (str, "normal");

  return g_string_free (str, FALSE);
}

static void
state_flags_changed (BobguiWidget *w, BobguiStateFlags old_flags, BobguiInspectorMiscInfo *sl)
{
  char *s;

  s = format_state_flags (bobgui_widget_get_state_flags (w));
  bobgui_label_set_label (BOBGUI_LABEL (sl->state), s);
  g_free (s);
}

static void
update_measure_picture (BobguiPicture      *picture,
                        BobguiToggleButton *toggle)
{
  GdkPaintable *paintable = bobgui_picture_get_paintable (picture);

  if (bobgui_toggle_button_get_active (toggle) ||
      (gdk_paintable_get_intrinsic_width (paintable) <= 200 &&
       gdk_paintable_get_intrinsic_height (paintable) <= 100))
    {
      bobgui_picture_set_can_shrink (picture, FALSE);
      bobgui_widget_set_size_request (BOBGUI_WIDGET (picture), -1, -1);
    }
  else
    {
      bobgui_picture_set_can_shrink (picture, TRUE);
      bobgui_widget_set_size_request (BOBGUI_WIDGET (picture),
                                   -1,
                                   MIN (100, 200 / gdk_paintable_get_intrinsic_aspect_ratio (paintable)));
    }
}

static void
measure_graph_measure (BobguiInspectorMiscInfo *sl)
{
  if (bobgui_widget_get_visible (sl->measure_row))
    bobgui_inspector_measure_graph_measure (BOBGUI_INSPECTOR_MEASURE_GRAPH (sl->measure_graph), BOBGUI_WIDGET (sl->object));

  update_measure_picture (BOBGUI_PICTURE (sl->measure_picture), BOBGUI_TOGGLE_BUTTON (sl->measure_expand_toggle));
}

static void
update_allocation (BobguiWidget            *w,
                   BobguiInspectorMiscInfo *sl)
{
  graphene_rect_t bounds;
  char *size_label;
  GEnumClass *class;
  GEnumValue *value;
  BobguiWidget *target;

  target = bobgui_widget_get_parent (w);
  if (target == NULL)
    target = w;

  if (!bobgui_widget_compute_bounds (w, target, &bounds))
    graphene_rect_init (&bounds, 0, 0, 0, 0);

  size_label = g_strdup_printf ("%g × %g +%g +%g",
                                bounds.size.width, bounds.size.height,
                                bounds.origin.x, bounds.origin.y);

  bobgui_label_set_label (BOBGUI_LABEL (sl->bounds), size_label);
  g_free (size_label);

  size_label = g_strdup_printf ("%d", bobgui_widget_get_baseline (w));
  bobgui_label_set_label (BOBGUI_LABEL (sl->baseline), size_label);
  g_free (size_label);

  class = G_ENUM_CLASS (g_type_class_ref (BOBGUI_TYPE_SIZE_REQUEST_MODE));
  value = g_enum_get_value (class, bobgui_widget_get_request_mode (w));
  bobgui_label_set_label (BOBGUI_LABEL (sl->request_mode), value->value_nick);
  g_type_class_unref (class);

  measure_graph_measure (sl);
}

static void
disconnect_each_other (gpointer  still_alive,
                       GObject  *for_science)
{
  if (BOBGUI_INSPECTOR_IS_MISC_INFO (still_alive))
    {
      BobguiInspectorMiscInfo *self = BOBGUI_INSPECTOR_MISC_INFO (still_alive);
      self->object = NULL;
    }

  g_signal_handlers_disconnect_matched (still_alive, G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, for_science);
  g_object_weak_unref (still_alive, disconnect_each_other, for_science);
}

static void
show_mnemonic_label (BobguiWidget *button, BobguiInspectorMiscInfo *sl)
{
  BobguiInspectorWindow *iw;
  BobguiWidget *widget;

  iw = BOBGUI_INSPECTOR_WINDOW (bobgui_widget_get_ancestor (BOBGUI_WIDGET (sl), BOBGUI_TYPE_INSPECTOR_WINDOW));

  widget = g_object_get_data (G_OBJECT (button), "mnemonic-label");
  if (widget)
    bobgui_inspector_window_push_object (iw, G_OBJECT (widget), CHILD_KIND_OTHER, 0);
}

static void
show_surface (BobguiWidget *button, BobguiInspectorMiscInfo *sl)
{
  BobguiInspectorWindow *iw;
  GObject *surface;

  iw = BOBGUI_INSPECTOR_WINDOW (bobgui_widget_get_ancestor (BOBGUI_WIDGET (sl), BOBGUI_TYPE_INSPECTOR_WINDOW));

  surface = (GObject *)bobgui_native_get_surface (BOBGUI_NATIVE (sl->object));
  if (surface)
    bobgui_inspector_window_push_object (iw, surface, CHILD_KIND_OTHER, 0);
}

static void
show_renderer (BobguiWidget *button, BobguiInspectorMiscInfo *sl)
{
  BobguiInspectorWindow *iw;
  GObject *renderer;

  iw = BOBGUI_INSPECTOR_WINDOW (bobgui_widget_get_ancestor (BOBGUI_WIDGET (sl), BOBGUI_TYPE_INSPECTOR_WINDOW));

  renderer = (GObject *)bobgui_native_get_renderer (BOBGUI_NATIVE (sl->object));
  if (renderer)
    bobgui_inspector_window_push_object (iw, renderer, CHILD_KIND_OTHER, 0);
}

static void
show_frame_clock (BobguiWidget *button, BobguiInspectorMiscInfo *sl)
{
  BobguiInspectorWindow *iw;
  GObject *clock;

  iw = BOBGUI_INSPECTOR_WINDOW (bobgui_widget_get_ancestor (BOBGUI_WIDGET (sl), BOBGUI_TYPE_INSPECTOR_WINDOW));

  clock = (GObject *)bobgui_widget_get_frame_clock (BOBGUI_WIDGET (sl->object));
  if (clock)
    bobgui_inspector_window_push_object (iw, clock, CHILD_KIND_OTHER, 0);
}

static void
update_surface (BobguiInspectorMiscInfo *sl)
{
  bobgui_widget_set_visible (sl->surface_row, BOBGUI_IS_NATIVE (sl->object));
  if (BOBGUI_IS_NATIVE (sl->object))
    {
      GObject *obj;
      char *tmp;

      obj = (GObject *)bobgui_native_get_surface (BOBGUI_NATIVE (sl->object));
      tmp = g_strdup_printf ("%p", obj);
      bobgui_label_set_label (BOBGUI_LABEL (sl->surface), tmp);
      g_free (tmp);
    }
}

static void
update_renderer (BobguiInspectorMiscInfo *sl)
{
  bobgui_widget_set_visible (sl->renderer_row, BOBGUI_IS_NATIVE (sl->object));
  if (BOBGUI_IS_NATIVE (sl->object))
    {
      GObject *obj;
      char *tmp;

      obj = (GObject *)bobgui_native_get_surface (BOBGUI_NATIVE (sl->object));
      tmp = g_strdup_printf ("%p", obj);
      bobgui_label_set_label (BOBGUI_LABEL (sl->renderer), tmp);
      g_free (tmp);
    }
}

static void
update_frame_clock (BobguiInspectorMiscInfo *sl)
{
  bobgui_widget_set_visible (sl->frame_clock_row, BOBGUI_IS_ROOT (sl->object));
  if (BOBGUI_IS_ROOT (sl->object))
    {
      GObject *clock;

      clock = (GObject *)bobgui_widget_get_frame_clock (BOBGUI_WIDGET (sl->object));
      bobgui_widget_set_sensitive (sl->frame_clock_button, clock != NULL);
      if (clock)
        {
          char *tmp = g_strdup_printf ("%p", clock);
          bobgui_label_set_label (BOBGUI_LABEL (sl->frame_clock), tmp);
          g_free (tmp);
        }
      else
        {
          bobgui_label_set_label (BOBGUI_LABEL (sl->frame_clock), "NULL");
        }
    }
}

static void
update_direction (BobguiInspectorMiscInfo *sl)
{
  BobguiWidget *widget = BOBGUI_WIDGET (sl->object);

  switch (widget->priv->direction)
    {
    case BOBGUI_TEXT_DIR_LTR:
      bobgui_label_set_label (BOBGUI_LABEL (sl->direction), "Left-to-Right");
      break;
    case BOBGUI_TEXT_DIR_RTL:
      bobgui_label_set_label (BOBGUI_LABEL (sl->direction), "Right-to-Left");
      break;
    case BOBGUI_TEXT_DIR_NONE:
      if (bobgui_widget_get_direction (widget) == BOBGUI_TEXT_DIR_LTR)
        bobgui_label_set_label (BOBGUI_LABEL (sl->direction), "Left-to-Right (inherited)");
      else
        bobgui_label_set_label (BOBGUI_LABEL (sl->direction), "Right-to-Left (inherited)");
      break;
    default:
      g_assert_not_reached ();
    }
}

static gboolean
update_info (gpointer data)
{
  BobguiInspectorMiscInfo *sl = data;
  char *tmp;
  GType gtype;

  tmp = g_strdup_printf ("%p", sl->object);
  bobgui_label_set_text (BOBGUI_LABEL (sl->address), tmp);
  g_free (tmp);

  gtype = G_TYPE_FROM_INSTANCE (sl->object);

  bobgui_menu_button_set_label (BOBGUI_MENU_BUTTON (sl->type), g_type_name (gtype));
  bobgui_inspector_type_popover_set_gtype (BOBGUI_INSPECTOR_TYPE_POPOVER (sl->type_popover),
                                        gtype);

  if (G_IS_OBJECT (sl->object))
    {
      tmp = g_strdup_printf ("%d", sl->object->ref_count);
      bobgui_label_set_text (BOBGUI_LABEL (sl->refcount), tmp);
      g_free (tmp);
    }

  if (BOBGUI_IS_WIDGET (sl->object))
    {
      BobguiWidget *child;
      GList *list, *l;

      update_direction (sl);

      while ((child = bobgui_widget_get_first_child (sl->mnemonic_label)))
        bobgui_box_remove (BOBGUI_BOX (sl->mnemonic_label), child);

      list = bobgui_widget_list_mnemonic_labels (BOBGUI_WIDGET (sl->object));
      for (l = list; l; l = l->next)
        {
          BobguiWidget *button;

          tmp = g_strdup_printf ("%p (%s)", l->data, g_type_name_from_instance ((GTypeInstance*)l->data));
          button = bobgui_button_new_with_label (tmp);
          g_free (tmp);
          bobgui_box_append (BOBGUI_BOX (sl->mnemonic_label), button);
          g_object_set_data (G_OBJECT (button), "mnemonic-label", l->data);
          g_signal_connect (button, "clicked", G_CALLBACK (show_mnemonic_label), sl);
        }
      g_list_free (list);

      bobgui_widget_set_visible (sl->tick_callback, bobgui_widget_has_tick_callback (BOBGUI_WIDGET (sl->object)));
      bobgui_widget_set_visible (sl->realized, bobgui_widget_get_realized (BOBGUI_WIDGET (sl->object)));
      bobgui_widget_set_visible (sl->mapped, bobgui_widget_get_mapped (BOBGUI_WIDGET (sl->object)));
      bobgui_widget_set_visible (sl->is_toplevel, BOBGUI_IS_NATIVE (sl->object));
      bobgui_widget_set_visible (sl->child_visible, _bobgui_widget_get_child_visible (BOBGUI_WIDGET (sl->object)));
    }

  update_surface (sl);
  update_renderer (sl);
  update_frame_clock (sl);

  if (BOBGUI_IS_BUILDABLE (sl->object))
    {
      bobgui_label_set_text (BOBGUI_LABEL (sl->buildable_id),
                          bobgui_buildable_get_buildable_id (BOBGUI_BUILDABLE (sl->object)));
    }

  if (GDK_IS_FRAME_CLOCK (sl->object))
    {
      GdkFrameClock *clock;
      gint64 frame;
      gint64 frame_time;
      gint64 history_start;
      gint64 history_len;
      gint64 previous_frame_time;
      GdkFrameTimings *previous_timings;

      clock = GDK_FRAME_CLOCK (sl->object);
      frame = gdk_frame_clock_get_frame_counter (clock);
      frame_time = gdk_frame_clock_get_frame_time (clock);

      tmp = g_strdup_printf ("%"G_GINT64_FORMAT, frame);
      bobgui_label_set_label (BOBGUI_LABEL (sl->framecount), tmp);
      g_free (tmp);

      history_start = gdk_frame_clock_get_history_start (clock);
      history_len = frame - history_start;

      if (history_len > 0 && sl->last_frame != frame)
        {
          previous_timings = gdk_frame_clock_get_timings (clock, history_start);
          previous_frame_time = gdk_frame_timings_get_frame_time (previous_timings);
          tmp = g_strdup_printf ("%4.1f ⁄ s", (G_USEC_PER_SEC * history_len) / (double) (frame_time - previous_frame_time));
          bobgui_label_set_label (BOBGUI_LABEL (sl->framerate), tmp);
          g_free (tmp);
        }
      else
        {
          bobgui_label_set_label (BOBGUI_LABEL (sl->framerate), "—");
        }

      sl->last_frame = frame;
    }

  if (GDK_IS_SURFACE (sl->object))
    {
      char buf[64];

      g_snprintf (buf, sizeof (buf), "%g", gdk_surface_get_scale (GDK_SURFACE (sl->object)));

      bobgui_label_set_label (BOBGUI_LABEL (sl->scale), buf);

      bobgui_label_set_label (BOBGUI_LABEL (sl->color_state), gdk_color_state_get_name (gdk_surface_get_color_state (GDK_SURFACE (sl->object))));
    }

  if (GDK_IS_PAINTABLE (sl->object))
    {
      char buf[64];
      char *value;

      g_snprintf (buf, sizeof (buf), "%d x %d",
                  gdk_paintable_get_intrinsic_width (GDK_PAINTABLE (sl->object)),
                  gdk_paintable_get_intrinsic_height (GDK_PAINTABLE (sl->object)));
      bobgui_label_set_label (BOBGUI_LABEL (sl->intrinsic_size), buf);

      g_snprintf (buf, sizeof (buf), "%.2g",
                  gdk_paintable_get_intrinsic_aspect_ratio (GDK_PAINTABLE (sl->object)));
      bobgui_label_set_label (BOBGUI_LABEL (sl->aspect_ratio), buf);

      value = g_flags_to_string (GDK_TYPE_PAINTABLE_FLAGS,
                                 gdk_paintable_get_flags (GDK_PAINTABLE (sl->object)));
      bobgui_label_set_label (BOBGUI_LABEL (sl->paintable_flags), value);
      g_free (value);
    }

  return G_SOURCE_CONTINUE;
}

static GdkContentProvider *
measure_picture_drag_prepare (BobguiDragSource *source,
                              double         x,
                              double         y,
                              gpointer       unused)
{
  BobguiWidget *picture;
  GdkPaintable *measure_graph;
  GdkTexture *texture;

  picture = bobgui_event_controller_get_widget (BOBGUI_EVENT_CONTROLLER (source));
  measure_graph = bobgui_picture_get_paintable (BOBGUI_PICTURE (picture));
  if (!BOBGUI_IS_INSPECTOR_MEASURE_GRAPH (measure_graph))
    return NULL;

  texture = bobgui_inspector_measure_graph_get_texture (BOBGUI_INSPECTOR_MEASURE_GRAPH (measure_graph));
  if (texture == NULL)
    return NULL;

  return gdk_content_provider_new_typed (GDK_TYPE_TEXTURE, texture);
}

void
bobgui_inspector_misc_info_set_object (BobguiInspectorMiscInfo *sl,
                                    GObject              *object)
{
  if (sl->object)
    {
      g_signal_handlers_disconnect_by_func (sl->object, state_flags_changed, sl);
      disconnect_each_other (sl->object, G_OBJECT (sl));
      disconnect_each_other (sl, sl->object);
      sl->object = NULL;
    }

  bobgui_widget_set_visible (BOBGUI_WIDGET (sl), TRUE);

  sl->object = object;
  g_object_weak_ref (G_OBJECT (sl), disconnect_each_other, object);
  g_object_weak_ref (object, disconnect_each_other, sl);

  bobgui_widget_set_visible (sl->refcount_row, G_IS_OBJECT (object));
  bobgui_widget_set_visible (sl->state_row, BOBGUI_IS_WIDGET (object));
  bobgui_widget_set_visible (sl->direction_row, BOBGUI_IS_WIDGET (object));
  bobgui_widget_set_visible (sl->request_mode_row, BOBGUI_IS_WIDGET (object));
  bobgui_widget_set_visible (sl->bounds_row, BOBGUI_IS_WIDGET (object));
  bobgui_widget_set_visible (sl->baseline_row, BOBGUI_IS_WIDGET (object));
  /* Don't autoshow, it may be slow, we have a button for this */
  if (!BOBGUI_IS_WIDGET (object))
    bobgui_widget_set_visible (sl->measure_row, FALSE);
  bobgui_widget_set_visible (sl->measure_info_row, BOBGUI_IS_WIDGET (object));
  bobgui_widget_set_visible (sl->mnemonic_label_row, BOBGUI_IS_WIDGET (object));
  bobgui_widget_set_visible (sl->tick_callback_row, BOBGUI_IS_WIDGET (object));
  bobgui_widget_set_visible (sl->mapped_row, BOBGUI_IS_WIDGET (object));
  bobgui_widget_set_visible (sl->realized_row, BOBGUI_IS_WIDGET (object));
  bobgui_widget_set_visible (sl->is_toplevel_row, BOBGUI_IS_WIDGET (object));
  bobgui_widget_set_visible (sl->child_visible_row, BOBGUI_IS_WIDGET (object));
  bobgui_widget_set_visible (sl->frame_clock_row, BOBGUI_IS_WIDGET (object));
  bobgui_widget_set_visible (sl->buildable_id_row, BOBGUI_IS_BUILDABLE (object));
  bobgui_widget_set_visible (sl->framecount_row, GDK_IS_FRAME_CLOCK (object));
  bobgui_widget_set_visible (sl->framerate_row, GDK_IS_FRAME_CLOCK (object));
  bobgui_widget_set_visible (sl->scale_row, GDK_IS_SURFACE (object));
  bobgui_widget_set_visible (sl->color_state_row, GDK_IS_SURFACE (object));
  bobgui_widget_set_visible (sl->intrinsic_size_row, GDK_IS_PAINTABLE (object));
  bobgui_widget_set_visible (sl->aspect_ratio_row, GDK_IS_PAINTABLE (object));
  bobgui_widget_set_visible (sl->paintable_flags_row, GDK_IS_PAINTABLE (object));

  if (BOBGUI_IS_WIDGET (object))
    {
      g_signal_connect_object (object, "state-flags-changed", G_CALLBACK (state_flags_changed), sl, 0);
      state_flags_changed (BOBGUI_WIDGET (sl->object), 0, sl);

      update_allocation (BOBGUI_WIDGET (sl->object), sl);
      update_measure_picture (BOBGUI_PICTURE (sl->measure_picture), BOBGUI_TOGGLE_BUTTON (sl->measure_expand_toggle));
    }
  else
    {
      bobgui_inspector_measure_graph_clear (BOBGUI_INSPECTOR_MEASURE_GRAPH (sl->measure_graph));
    }

  update_info (sl);
}

static void
bobgui_inspector_misc_info_init (BobguiInspectorMiscInfo *sl)
{
  bobgui_widget_init_template (BOBGUI_WIDGET (sl));

  sl->type_popover = g_object_new (BOBGUI_TYPE_INSPECTOR_TYPE_POPOVER, NULL);
  bobgui_menu_button_set_popover (BOBGUI_MENU_BUTTON (sl->type),
                               sl->type_popover);

}

static void
map (BobguiWidget *widget)
{
  BobguiInspectorMiscInfo *sl = BOBGUI_INSPECTOR_MISC_INFO (widget);

  BOBGUI_WIDGET_CLASS (bobgui_inspector_misc_info_parent_class)->map (widget);

  sl->update_source_id = g_timeout_add_seconds (1, update_info, sl);
  update_info (sl);
}

static void
unmap (BobguiWidget *widget)
{
  BobguiInspectorMiscInfo *sl = BOBGUI_INSPECTOR_MISC_INFO (widget);

  g_source_remove (sl->update_source_id);
  sl->update_source_id = 0;

  BOBGUI_WIDGET_CLASS (bobgui_inspector_misc_info_parent_class)->unmap (widget);
}

static void
dispose (GObject *o)
{
  BobguiInspectorMiscInfo *sl = BOBGUI_INSPECTOR_MISC_INFO (o);

  bobgui_widget_dispose_template (BOBGUI_WIDGET (sl), BOBGUI_TYPE_INSPECTOR_MISC_INFO);

  G_OBJECT_CLASS (bobgui_inspector_misc_info_parent_class)->dispose (o);
}

static void
bobgui_inspector_misc_info_class_init (BobguiInspectorMiscInfoClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);

  object_class->dispose = dispose;

  widget_class->map = map;
  widget_class->unmap = unmap;

  bobgui_widget_class_set_template_from_resource (widget_class, "/org/bobgui/libbobgui/inspector/misc-info.ui");
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorMiscInfo, swin);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorMiscInfo, address);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorMiscInfo, type);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorMiscInfo, refcount_row);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorMiscInfo, refcount);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorMiscInfo, state_row);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorMiscInfo, state);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorMiscInfo, direction_row);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorMiscInfo, direction);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorMiscInfo, buildable_id_row);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorMiscInfo, buildable_id);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorMiscInfo, mnemonic_label_row);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorMiscInfo, mnemonic_label);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorMiscInfo, request_mode_row);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorMiscInfo, request_mode);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorMiscInfo, measure_row);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorMiscInfo, measure_info_row);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorMiscInfo, measure_expand_toggle);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorMiscInfo, measure_picture);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorMiscInfo, measure_graph);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorMiscInfo, bounds_row);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorMiscInfo, bounds);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorMiscInfo, baseline_row);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorMiscInfo, baseline);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorMiscInfo, surface_row);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorMiscInfo, surface);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorMiscInfo, surface_button);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorMiscInfo, renderer_row);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorMiscInfo, renderer);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorMiscInfo, renderer_button);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorMiscInfo, frame_clock_row);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorMiscInfo, frame_clock);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorMiscInfo, frame_clock_button);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorMiscInfo, tick_callback_row);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorMiscInfo, tick_callback);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorMiscInfo, framecount_row);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorMiscInfo, framecount);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorMiscInfo, framerate_row);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorMiscInfo, framerate);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorMiscInfo, scale_row);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorMiscInfo, scale);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorMiscInfo, color_state_row);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorMiscInfo, color_state);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorMiscInfo, mapped_row);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorMiscInfo, mapped);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorMiscInfo, realized_row);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorMiscInfo, realized);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorMiscInfo, is_toplevel_row);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorMiscInfo, is_toplevel);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorMiscInfo, child_visible_row);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorMiscInfo, child_visible);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorMiscInfo, intrinsic_size_row);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorMiscInfo, intrinsic_size);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorMiscInfo, aspect_ratio_row);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorMiscInfo, aspect_ratio);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorMiscInfo, paintable_flags_row);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorMiscInfo, paintable_flags);

  bobgui_widget_class_bind_template_callback (widget_class, update_measure_picture);
  bobgui_widget_class_bind_template_callback (widget_class, measure_picture_drag_prepare);
  bobgui_widget_class_bind_template_callback (widget_class, measure_graph_measure);
  bobgui_widget_class_bind_template_callback (widget_class, show_surface);
  bobgui_widget_class_bind_template_callback (widget_class, show_renderer);
  bobgui_widget_class_bind_template_callback (widget_class, show_frame_clock);

  bobgui_widget_class_set_layout_manager_type (widget_class, BOBGUI_TYPE_BIN_LAYOUT);
}

// vim: set et sw=2 ts=2:
