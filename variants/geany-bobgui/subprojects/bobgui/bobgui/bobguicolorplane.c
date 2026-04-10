/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2012 Red Hat, Inc.
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

#include "bobguicolorplaneprivate.h"

#include "bobguiadjustment.h"
#include "bobguicolorutils.h"
#include "bobguigesturedrag.h"
#include "bobguigesturelongpress.h"
#include "bobguisnapshot.h"
#include "bobguiprivate.h"
#include "bobguieventcontrollerkey.h"
#include "bobguishortcutcontroller.h"
#include "bobguishortcuttrigger.h"
#include "bobguishortcutaction.h"
#include "bobguishortcut.h"

struct _BobguiColorPlane
{
  BobguiWidget parent_instance;

  BobguiAdjustment *h_adj;
  BobguiAdjustment *s_adj;
  BobguiAdjustment *v_adj;

  GdkTexture *texture;
};

typedef struct
{
  BobguiWidgetClass parent_class;
} BobguiColorPlaneClass;

enum {
  PROP_0,
  PROP_H_ADJUSTMENT,
  PROP_S_ADJUSTMENT,
  PROP_V_ADJUSTMENT
};

G_DEFINE_TYPE (BobguiColorPlane, bobgui_color_plane, BOBGUI_TYPE_WIDGET)

static void
sv_to_xy (BobguiColorPlane *plane,
          int           *x,
          int           *y)
{
  double s, v;
  int width, height;

  width = bobgui_widget_get_width (BOBGUI_WIDGET (plane));
  height = bobgui_widget_get_height (BOBGUI_WIDGET (plane));

  s = bobgui_adjustment_get_value (plane->s_adj);
  v = bobgui_adjustment_get_value (plane->v_adj);

  *x = CLAMP (width * v, 0, width - 1);
  *y = CLAMP (height * (1 - s), 0, height - 1);
}

static void
plane_snapshot (BobguiWidget   *widget,
                BobguiSnapshot *snapshot)
{
  BobguiColorPlane *plane = BOBGUI_COLOR_PLANE (widget);
  int x, y;
  int width, height;

  sv_to_xy (plane, &x, &y);
  width = bobgui_widget_get_width (widget);
  height = bobgui_widget_get_height (widget);

  bobgui_snapshot_append_texture (snapshot,
                               plane->texture,
                               &GRAPHENE_RECT_INIT (0, 0, width, height));
  if (bobgui_widget_has_visible_focus (widget))
    {
      const GdkRGBA c1 = { 1.0, 1.0, 1.0, 0.6 };
      const GdkRGBA c2 = { 0.0, 0.0, 0.0, 0.8 };

      /* Crosshair border */
      bobgui_snapshot_append_color (snapshot, &c1,
                                 &GRAPHENE_RECT_INIT (0, y - 1.5, width, 3));
      bobgui_snapshot_append_color (snapshot, &c1,
                                 &GRAPHENE_RECT_INIT (x - 1.5, 0, 3, height));

      /* Actual crosshair */
      bobgui_snapshot_append_color (snapshot, &c2,
                                 &GRAPHENE_RECT_INIT (0, y - 0.5, width, 1));
      bobgui_snapshot_append_color (snapshot, &c2,
                                 &GRAPHENE_RECT_INIT (x - 0.5, 0, 1, height));
    }
  else
    {
      const GdkRGBA c = { 0.8, 0.8, 0.8, 0.8 };

      /* Horizontal */
      bobgui_snapshot_append_color (snapshot, &c,
                                 &GRAPHENE_RECT_INIT (0, y - 0.5, width, 1));
      /* Vertical */
      bobgui_snapshot_append_color (snapshot, &c,
                                 &GRAPHENE_RECT_INIT (x - 0.5, 0, 1, height));
    }
}

static void
create_texture (BobguiWidget *widget)
{
  const int width = bobgui_widget_get_width (widget);
  const int height = bobgui_widget_get_height (widget);
  const int stride = width * 3;
  BobguiColorPlane *plane = BOBGUI_COLOR_PLANE (widget);
  GBytes *bytes;
  guint8 *data;

  if (!bobgui_widget_get_mapped (widget))
    return;

  if (width == 0 || height == 0)
    return;

  g_clear_object (&plane->texture);

  data = g_malloc (height * stride);
  if (width > 1 && height > 1)
    {
      const float h = bobgui_adjustment_get_value (plane->h_adj);
      int x, y;

      for (y = 0; y < height; y++)
        {
          const float s = 1.0 - (float)y / (height - 1);

          for (x = 0; x < width; x++)
            {
              const float v = (float)x / (width - 1);
              float r, g, b;

              bobgui_hsv_to_rgb (h, s, v, &r, &g, &b);

              data[(y * stride) + (x * 3) + 0] = r * 255;
              data[(y * stride) + (x * 3) + 1] = g * 255;
              data[(y * stride) + (x * 3) + 2] = b * 255;
            }
        }
    }
  else
    {
      memset (data, 0, height * stride);
    }

  bytes = g_bytes_new_take (data, height * stride);
  plane->texture = gdk_memory_texture_new (width, height,
                                           GDK_MEMORY_R8G8B8,
                                           bytes,
                                           stride);
  g_bytes_unref (bytes);
}

static void
plane_size_allocate (BobguiWidget *widget,
                     int        width,
                     int        height,
                     int        baseline)
{
  create_texture (widget);
}

static void
set_cross_cursor (BobguiWidget *widget,
                  gboolean   enabled)
{
  if (enabled)
    bobgui_widget_set_cursor_from_name (widget, "crosshair");
  else
    bobgui_widget_set_cursor (widget, NULL);
}

static void
h_changed (BobguiWidget *plane)
{
  create_texture (plane);
  bobgui_widget_queue_draw (plane);
}

static void
sv_changed (BobguiColorPlane *plane)
{
  bobgui_widget_queue_draw (BOBGUI_WIDGET (plane));
}

static void
update_color (BobguiColorPlane *plane,
              int            x,
              int            y)
{
  BobguiWidget *widget = BOBGUI_WIDGET (plane);
  double s, v;

  s = CLAMP (1 - y * (1.0 / bobgui_widget_get_height (widget)), 0, 1);
  v = CLAMP (x * (1.0 / bobgui_widget_get_width (widget)), 0, 1);
  bobgui_adjustment_set_value (plane->s_adj, s);
  bobgui_adjustment_set_value (plane->v_adj, v);

  bobgui_widget_queue_draw (widget);
}

static void
hold_action (BobguiGestureLongPress *gesture,
             double               x,
             double               y,
             BobguiWidget           *plane)
{
  bobgui_widget_activate_action (plane,
                              "color.edit",
                              "s", bobgui_widget_get_name (plane));
}

static void
sv_move (BobguiColorPlane *plane,
         double         ds,
         double         dv)
{
  double s, v;

  s = bobgui_adjustment_get_value (plane->s_adj);
  v = bobgui_adjustment_get_value (plane->v_adj);

  if (s + ds > 1)
    {
      if (s < 1)
        s = 1;
      else
        goto error;
    }
  else if (s + ds < 0)
    {
      if (s > 0)
        s = 0;
      else
        goto error;
    }
  else
    {
      s += ds;
    }

  if (v + dv > 1)
    {
      if (v < 1)
        v = 1;
      else
        goto error;
    }
  else if (v + dv < 0)
    {
      if (v > 0)
        v = 0;
      else
        goto error;
    }
  else
    {
      v += dv;
    }

  bobgui_adjustment_set_value (plane->s_adj, s);
  bobgui_adjustment_set_value (plane->v_adj, v);
  return;

error:
  bobgui_widget_error_bell (BOBGUI_WIDGET (plane));
}

static gboolean
key_controller_key_pressed (BobguiEventControllerKey *controller,
                            guint                  keyval,
                            guint                  keycode,
                            GdkModifierType        state,
                            BobguiWidget             *widget)
{
  BobguiColorPlane *plane = BOBGUI_COLOR_PLANE (widget);
  double step;

  if ((state & GDK_ALT_MASK) != 0)
    step = 0.1;
  else
    step = 0.01;

  if (keyval == GDK_KEY_Up ||
      keyval == GDK_KEY_KP_Up)
    sv_move (plane, step, 0);
  else if (keyval == GDK_KEY_Down ||
           keyval == GDK_KEY_KP_Down)
    sv_move (plane, -step, 0);
  else if (keyval == GDK_KEY_Left ||
           keyval == GDK_KEY_KP_Left)
    sv_move (plane, 0, -step);
  else if (keyval == GDK_KEY_Right ||
           keyval == GDK_KEY_KP_Right)
    sv_move (plane, 0, step);
  else
    return FALSE;

  return TRUE;
}

static void
plane_drag_gesture_begin (BobguiGestureDrag *gesture,
                          double          start_x,
                          double          start_y,
                          BobguiWidget      *plane)
{
  guint button;

  button = bobgui_gesture_single_get_current_button (BOBGUI_GESTURE_SINGLE (gesture));

  if (button == GDK_BUTTON_SECONDARY)
    {
      bobgui_widget_activate_action (plane,
                                  "color.edit",
                                  "s", bobgui_widget_get_name (plane));
    }

  if (button != GDK_BUTTON_PRIMARY)
    {
      bobgui_gesture_set_state (BOBGUI_GESTURE (gesture), BOBGUI_EVENT_SEQUENCE_DENIED);
      return;
    }

  set_cross_cursor (plane, TRUE);
  update_color (BOBGUI_COLOR_PLANE (plane), start_x, start_y);
  bobgui_widget_grab_focus (plane);
  bobgui_gesture_set_state (BOBGUI_GESTURE (gesture), BOBGUI_EVENT_SEQUENCE_CLAIMED);
}

static void
plane_drag_gesture_update (BobguiGestureDrag *gesture,
                           double          offset_x,
                           double          offset_y,
                           BobguiColorPlane  *plane)
{
  double start_x, start_y;

  bobgui_gesture_drag_get_start_point (BOBGUI_GESTURE_DRAG (gesture),
                                    &start_x, &start_y);
  update_color (plane, start_x + offset_x, start_y + offset_y);
}

static void
plane_drag_gesture_end (BobguiGestureDrag *gesture,
                        double          offset_x,
                        double          offset_y,
                        BobguiColorPlane  *plane)
{
  set_cross_cursor (BOBGUI_WIDGET (plane), FALSE);
}

static void
bobgui_color_plane_init (BobguiColorPlane *plane)
{
  BobguiEventController *controller;
  BobguiGesture *gesture;
  BobguiShortcutTrigger *trigger;
  BobguiShortcutAction *action;
  BobguiShortcut *shortcut;

  bobgui_widget_set_focusable (BOBGUI_WIDGET (plane), TRUE);

  gesture = bobgui_gesture_drag_new ();
  g_signal_connect (gesture, "drag-begin",
                    G_CALLBACK (plane_drag_gesture_begin), plane);
  g_signal_connect (gesture, "drag-update",
                    G_CALLBACK (plane_drag_gesture_update), plane);
  g_signal_connect (gesture, "drag-end",
                    G_CALLBACK (plane_drag_gesture_end), plane);
  bobgui_gesture_single_set_button (BOBGUI_GESTURE_SINGLE (gesture), 0);
  bobgui_widget_add_controller (BOBGUI_WIDGET (plane), BOBGUI_EVENT_CONTROLLER (gesture));

  gesture = bobgui_gesture_long_press_new ();
  g_signal_connect (gesture, "pressed",
                    G_CALLBACK (hold_action), plane);
  bobgui_gesture_single_set_touch_only (BOBGUI_GESTURE_SINGLE (gesture),
                                     TRUE);
  bobgui_widget_add_controller (BOBGUI_WIDGET (plane), BOBGUI_EVENT_CONTROLLER (gesture));

  controller = bobgui_event_controller_key_new ();
  g_signal_connect (controller, "key-pressed",
                    G_CALLBACK (key_controller_key_pressed), plane);
  bobgui_widget_add_controller (BOBGUI_WIDGET (plane), controller);

  controller = bobgui_shortcut_controller_new ();
  trigger = bobgui_alternative_trigger_new (bobgui_keyval_trigger_new (GDK_KEY_F10, GDK_SHIFT_MASK),
                                         bobgui_keyval_trigger_new (GDK_KEY_Menu, 0));
  action = bobgui_named_action_new ("color.edit");
  shortcut = bobgui_shortcut_new_with_arguments (trigger, action, "s", "sv");
  bobgui_shortcut_controller_add_shortcut (BOBGUI_SHORTCUT_CONTROLLER (controller), shortcut);
  bobgui_widget_add_controller (BOBGUI_WIDGET (plane), controller);
}

static void
plane_finalize (GObject *object)
{
  BobguiColorPlane *plane = BOBGUI_COLOR_PLANE (object);

  g_clear_object (&plane->texture);

  g_clear_object (&plane->h_adj);
  g_clear_object (&plane->s_adj);
  g_clear_object (&plane->v_adj);

  G_OBJECT_CLASS (bobgui_color_plane_parent_class)->finalize (object);
}

static void
plane_set_property (GObject      *object,
                    guint         prop_id,
                    const GValue *value,
                    GParamSpec   *pspec)
{
  BobguiColorPlane *plane = BOBGUI_COLOR_PLANE (object);
  BobguiAdjustment *adjustment;

  /* Construct only properties can only be set once, these are created
   * only in order to be properly buildable from bobguicoloreditor.ui
   */
  switch (prop_id)
    {
    case PROP_H_ADJUSTMENT:
      adjustment = g_value_get_object (value);
      if (adjustment)
        {
          plane->h_adj = g_object_ref_sink (adjustment);
          g_signal_connect_swapped (adjustment, "value-changed", G_CALLBACK (h_changed), plane);
        }
      break;
    case PROP_S_ADJUSTMENT:
      adjustment = g_value_get_object (value);
      if (adjustment)
        {
          plane->s_adj = g_object_ref_sink (adjustment);
          g_signal_connect_swapped (adjustment, "value-changed", G_CALLBACK (sv_changed), plane);
        }
      break;
    case PROP_V_ADJUSTMENT:
      adjustment = g_value_get_object (value);
      if (adjustment)
        {
          plane->v_adj = g_object_ref_sink (adjustment);
          g_signal_connect_swapped (adjustment, "value-changed", G_CALLBACK (sv_changed), plane);
        }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_color_plane_class_init (BobguiColorPlaneClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (class);

  object_class->finalize = plane_finalize;
  object_class->set_property = plane_set_property;

  widget_class->snapshot = plane_snapshot;
  widget_class->size_allocate = plane_size_allocate;

  g_object_class_install_property (object_class,
                                   PROP_H_ADJUSTMENT,
                                   g_param_spec_object ("h-adjustment", NULL, NULL,
                                                        BOBGUI_TYPE_ADJUSTMENT,
                                                        BOBGUI_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class,
                                   PROP_S_ADJUSTMENT,
                                   g_param_spec_object ("s-adjustment", NULL, NULL,
                                                        BOBGUI_TYPE_ADJUSTMENT,
                                                        BOBGUI_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class,
                                   PROP_V_ADJUSTMENT,
                                   g_param_spec_object ("v-adjustment", NULL, NULL,
                                                        BOBGUI_TYPE_ADJUSTMENT,
                                                        BOBGUI_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  bobgui_widget_class_set_css_name (widget_class, "plane");
  bobgui_widget_class_set_accessible_role (widget_class, BOBGUI_ACCESSIBLE_ROLE_SLIDER);
}

BobguiWidget *
bobgui_color_plane_new (BobguiAdjustment *h_adj,
                     BobguiAdjustment *s_adj,
                     BobguiAdjustment *v_adj)
{
  return g_object_new (BOBGUI_TYPE_COLOR_PLANE,
                       "h-adjustment", h_adj,
                       "s-adjustment", s_adj,
                       "v-adjustment", v_adj,
                       NULL);
}
