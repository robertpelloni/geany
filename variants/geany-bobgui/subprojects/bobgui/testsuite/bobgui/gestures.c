#include <bobgui/bobgui.h>

#define BOBGUI_COMPILATION
#include "gdk/gdkeventsprivate.h"

typedef struct {
  BobguiWidget *widget;
  int x;
  int y;
  guint state;
  guint pressed : 1;
} PointState;

static PointState mouse_state;
static PointState touch_state[10]; /* touchpoint 0 gets pointer emulation,
                                    * use it first in tests for consistency.
                                    */

#define EVENT_SEQUENCE(point) (GdkEventSequence*) ((point) - touch_state + 1)

static void
show_toplevel (BobguiWindow *window)
{
  GdkSurface *surface;

  bobgui_window_present (window);

  surface = bobgui_native_get_surface (BOBGUI_NATIVE (window));

  while (gdk_surface_get_width (surface) <= 1 ||
         gdk_surface_get_height (surface) <= 1)
    g_main_context_iteration (NULL, TRUE);
}

static void
show_popover (BobguiPopover *popover)
{
  GdkSurface *surface;

  bobgui_popover_popup (popover);

  surface = bobgui_native_get_surface (BOBGUI_NATIVE (popover));

  while (gdk_surface_get_width (surface) <= 1 ||
         gdk_surface_get_height (surface) <= 1)
    g_main_context_iteration (NULL, TRUE);
}

static void
inject_event (GdkEvent *event)
{
  gboolean handled;

  g_signal_emit_by_name (event->surface, "event", event, &handled);
}

static void
point_press (PointState *point,
             BobguiWidget  *widget,
             guint       button)
{
  GdkDisplay *display;
  GdkDevice *device;
  GdkSeat *seat;
  GdkSurface *surface;
  GdkEvent *ev;

  display = bobgui_widget_get_display (widget);
  seat = gdk_display_get_default_seat (display);
  device = gdk_seat_get_pointer (seat);
  surface = bobgui_native_get_surface (bobgui_widget_get_native (widget));

  if (point == &mouse_state)
    {
      GdkButtonEvent *button_event;

      button_event = (GdkButtonEvent *) g_type_create_instance (GDK_TYPE_BUTTON_EVENT);
      ev = (GdkEvent *) button_event;

      ev->event_type = GDK_BUTTON_PRESS;
      ev->surface = g_object_ref (surface);
      ev->device = g_object_ref (device);
      ev->time = GDK_CURRENT_TIME;

      button_event->button = button;
      button_event->state = point->state;
      button_event->x = point->x;
      button_event->y = point->y;

      point->state |= GDK_BUTTON1_MASK << (button - 1);
    }
  else
    {
      GdkTouchEvent *touch_event;

      touch_event = (GdkTouchEvent *) g_type_create_instance (GDK_TYPE_TOUCH_EVENT);
      ev = (GdkEvent *) touch_event;

      ev->event_type = GDK_TOUCH_BEGIN;
      ev->surface = g_object_ref (surface);
      ev->device = g_object_ref (device);
      ev->time = GDK_CURRENT_TIME;

      touch_event->sequence = EVENT_SEQUENCE (point);
      touch_event->state = point->state;
      touch_event->x = point->x;
      touch_event->y = point->y;
      touch_event->touch_emulating = (point == &touch_state[0]);
    }

  inject_event (ev);

  gdk_event_unref (ev);

  point->widget = widget;
}

static void
point_update (PointState *point,
              BobguiWidget  *widget,
              double      x,
              double      y)
{
  GdkDisplay *display;
  GdkDevice *device;
  GdkSeat *seat;
  GdkSurface *surface;
  GdkEvent *ev;

  display = bobgui_widget_get_display (widget);
  seat = gdk_display_get_default_seat (display);
  device = gdk_seat_get_pointer (seat);
  surface = bobgui_native_get_surface (bobgui_widget_get_native (widget));

  point->x = x;
  point->y = y;

  if (point == &mouse_state)
    {
      GdkMotionEvent *motion_event;

      motion_event = (GdkMotionEvent *) g_type_create_instance (GDK_TYPE_MOTION_EVENT);
      ev = (GdkEvent *) motion_event;

      ev->event_type = GDK_MOTION_NOTIFY;
      ev->surface = g_object_ref (surface);
      ev->device = g_object_ref (device);
      ev->time = GDK_CURRENT_TIME;

      motion_event->state = point->state;
      motion_event->x = point->x;
      motion_event->y = point->y;
    }
  else
    {
      GdkTouchEvent *touch_event;

      if (!point->widget || widget != point->widget)
        return;

      touch_event = (GdkTouchEvent *) g_type_create_instance (GDK_TYPE_TOUCH_EVENT);
      ev = (GdkEvent *) touch_event;

      ev->event_type = GDK_TOUCH_UPDATE;
      ev->surface = g_object_ref (surface);
      ev->device = g_object_ref (device);
      ev->time = GDK_CURRENT_TIME;

      touch_event->sequence = EVENT_SEQUENCE (point);
      touch_event->state = point->state;
      touch_event->x = point->x;
      touch_event->y = point->y;
      touch_event->touch_emulating = (point == &touch_state[0]);
    }

  inject_event (ev);

  gdk_event_unref (ev);
}

static void
point_release (PointState *point,
               guint       button)
{
  GdkDisplay *display;
  GdkDevice *device;
  GdkSeat *seat;
  GdkSurface *surface;
  GdkEvent *ev;

  if (point->widget == NULL)
    return;

  display = bobgui_widget_get_display (point->widget);
  seat = gdk_display_get_default_seat (display);
  device = gdk_seat_get_pointer (seat);
  surface = bobgui_native_get_surface (bobgui_widget_get_native (point->widget));

  if (!point->widget)
    return;

  if (point == &mouse_state)
    {
      GdkButtonEvent *button_event;

      if ((point->state & (GDK_BUTTON1_MASK << (button - 1))) == 0)
        return;

      button_event = (GdkButtonEvent *) g_type_create_instance (GDK_TYPE_BUTTON_EVENT);
      ev = (GdkEvent *) button_event;

      ev->event_type = GDK_BUTTON_RELEASE;
      ev->surface = g_object_ref (surface);
      ev->device = g_object_ref (device);
      ev->time = GDK_CURRENT_TIME;

      button_event->button = button;
      button_event->state = point->state;
      button_event->x = point->x;
      button_event->y = point->y;

      point->state &= ~(GDK_BUTTON1_MASK << (button - 1));
    }
  else
    {
      GdkTouchEvent *touch_event;

      touch_event = (GdkTouchEvent *) g_type_create_instance (GDK_TYPE_TOUCH_EVENT);
      ev = (GdkEvent *) touch_event;

      ev->event_type = GDK_TOUCH_END;
      ev->surface = g_object_ref (surface);
      ev->device = g_object_ref (device);
      ev->time = GDK_CURRENT_TIME;

      touch_event->sequence = EVENT_SEQUENCE (point);
      touch_event->state = point->state;
      touch_event->x = point->x;
      touch_event->y = point->y;
      touch_event->touch_emulating = (point == &touch_state[0]);
    }

  inject_event (ev);

  gdk_event_unref (ev);
}

static const char *
phase_nick (BobguiPropagationPhase phase)
{
 GTypeClass *class;
 GEnumValue *value;

 class = g_type_class_ref (BOBGUI_TYPE_PROPAGATION_PHASE);
 value = g_enum_get_value ((GEnumClass*)class, phase);
 g_type_class_unref (class);

 return value->value_nick;
}

static const char *
state_nick (BobguiEventSequenceState state)
{
 GTypeClass *class;
 GEnumValue *value;

 class = g_type_class_ref (BOBGUI_TYPE_EVENT_SEQUENCE_STATE);
 value = g_enum_get_value ((GEnumClass*)class, state);
 g_type_class_unref (class);

 return value->value_nick;
}

typedef struct {
  BobguiEventController *controller;
  GString *str;
  gboolean exit;
} LegacyData;

static gboolean
legacy_cb (BobguiEventControllerLegacy *c, GdkEvent *button, gpointer data)
{
  if (gdk_event_get_event_type (button) == GDK_BUTTON_PRESS)
    {
      LegacyData *ld = data;
      BobguiWidget *w;

      w = bobgui_event_controller_get_widget (ld->controller);

      if (ld->str->len > 0)
        g_string_append (ld->str, ", ");
      g_string_append_printf (ld->str, "legacy %s", bobgui_widget_get_name (w));

      return ld->exit;
    }

  return GDK_EVENT_PROPAGATE;
}

typedef struct {
  GString *str;
  BobguiEventSequenceState state;
} GestureData;

static void
press_cb (BobguiGesture *g, int n_press, double x, double y, gpointer data)
{
  BobguiEventController *c = BOBGUI_EVENT_CONTROLLER (g);
  GdkEventSequence *sequence;
  BobguiPropagationPhase phase;
  GestureData *gd = data;
  const char *name;

  name = g_object_get_data (G_OBJECT (g), "name");
  phase = bobgui_event_controller_get_propagation_phase (c);

  if (gd->str->len > 0)
    g_string_append (gd->str, ", ");
  g_string_append_printf (gd->str, "%s %s", phase_nick (phase), name);

  sequence = bobgui_gesture_get_last_updated_sequence (g);

  if (sequence)
    g_string_append_printf (gd->str, " (%x)", GPOINTER_TO_UINT (sequence));

  if (gd->state != BOBGUI_EVENT_SEQUENCE_NONE)
    bobgui_gesture_set_state (g, gd->state);
}

static void
cancel_cb (BobguiGesture *g, GdkEventSequence *sequence, gpointer data)
{
  GestureData *gd = data;
  const char *name;

  name = g_object_get_data (G_OBJECT (g), "name");

  if (gd->str->len > 0)
    g_string_append (gd->str, ", ");
  g_string_append_printf (gd->str, "%s cancelled", name);
}

static void
begin_cb (BobguiGesture *g, GdkEventSequence *sequence, gpointer data)
{
  GestureData *gd = data;
  const char *name;

  name = g_object_get_data (G_OBJECT (g), "name");

  if (gd->str->len > 0)
    g_string_append (gd->str, ", ");
  g_string_append_printf (gd->str, "%s began", name);

  if (gd->state != BOBGUI_EVENT_SEQUENCE_NONE)
    bobgui_gesture_set_state (g, gd->state);
}

static void
end_cb (BobguiGesture *g, GdkEventSequence *sequence, gpointer data)
{
  GestureData *gd = data;
  const char *name;

  name = g_object_get_data (G_OBJECT (g), "name");

  if (gd->str->len > 0)
    g_string_append (gd->str, ", ");
  g_string_append_printf (gd->str, "%s ended", name);
}

static void
update_cb (BobguiGesture *g, GdkEventSequence *sequence, gpointer data)
{
  GestureData *gd = data;
  const char *name;

  name = g_object_get_data (G_OBJECT (g), "name");

  if (gd->str->len > 0)
    g_string_append (gd->str, ", ");
  g_string_append_printf (gd->str, "%s updated", name);
}

static void
state_changed_cb (BobguiGesture *g, GdkEventSequence *sequence, BobguiEventSequenceState state, gpointer data)
{
  GestureData *gd = data;
  const char *name;

  name = g_object_get_data (G_OBJECT (g), "name");

  if (gd->str->len > 0)
    g_string_append (gd->str, ", ");
  g_string_append_printf (gd->str, "%s state %s", name, state_nick (state));

  if (sequence != NULL)
    g_string_append_printf (gd->str, " (%x)", GPOINTER_TO_UINT (sequence));
}


static BobguiGesture *
add_gesture (BobguiWidget *w, const char *name, BobguiPropagationPhase phase, GString *str, BobguiEventSequenceState state)
{
  BobguiGesture *g;
  GestureData *data;
  GList *gestures;

  data = g_new (GestureData, 1);
  data->str = str;
  data->state = state;

  g = bobgui_gesture_click_new ();
  bobgui_gesture_single_set_touch_only (BOBGUI_GESTURE_SINGLE (g), FALSE);
  bobgui_gesture_single_set_button (BOBGUI_GESTURE_SINGLE (g), 1);
  bobgui_event_controller_set_propagation_phase (BOBGUI_EVENT_CONTROLLER (g), phase);
  bobgui_widget_add_controller (w, BOBGUI_EVENT_CONTROLLER (g));

  g_object_set_data (G_OBJECT (g), "name", (gpointer)name);

  gestures = g_object_steal_data (G_OBJECT (w), "gestures");
  gestures = g_list_prepend (gestures, g);
  g_object_set_data_full (G_OBJECT (w), "gestures", (gpointer) gestures,
                          (GDestroyNotify) g_list_free);

  g_signal_connect (g, "pressed", G_CALLBACK (press_cb), data);
  g_signal_connect (g, "cancel", G_CALLBACK (cancel_cb), data);
  g_signal_connect (g, "update", G_CALLBACK (update_cb), data);
  g_signal_connect (g, "sequence-state-changed", G_CALLBACK (state_changed_cb), data);

  return g;
}

static BobguiGesture *
add_mt_gesture (BobguiWidget *w, const char *name, BobguiPropagationPhase phase, GString *str, BobguiEventSequenceState state)
{
  BobguiGesture *g;
  GestureData *data;
  GList *gestures;

  data = g_new (GestureData, 1);
  data->str = str;
  data->state = state;

  g = bobgui_gesture_rotate_new ();
  bobgui_event_controller_set_propagation_phase (BOBGUI_EVENT_CONTROLLER (g), phase);
  bobgui_widget_add_controller (w, BOBGUI_EVENT_CONTROLLER (g));

  g_object_set_data (G_OBJECT (g), "name", (gpointer)name);

  gestures = g_object_get_data (G_OBJECT (w), "gestures");
  gestures = g_list_prepend (gestures, g);
  g_object_set_data_full (G_OBJECT (w), "gestures", (gpointer) gestures,
                          (GDestroyNotify) g_list_free);

  g_signal_connect (g, "begin", G_CALLBACK (begin_cb), data);
  g_signal_connect (g, "update", G_CALLBACK (update_cb), data);
  g_signal_connect (g, "end", G_CALLBACK (end_cb), data);
  g_signal_connect (g, "sequence-state-changed", G_CALLBACK (state_changed_cb), data);

  return g;
}

static void
add_legacy (BobguiWidget *w, GString *str, gboolean exit)
{
  LegacyData *data;

  data = g_new (LegacyData, 1);
  data->controller = bobgui_event_controller_legacy_new ();
  data->str = str;
  data->exit = exit;

  bobgui_event_controller_set_propagation_phase (data->controller, BOBGUI_PHASE_BUBBLE);
  bobgui_widget_add_controller (w, data->controller);
  g_signal_connect (data->controller, "event", G_CALLBACK (legacy_cb), data);
}

static void
assert_gesture_state (BobguiWidget *w, const char *name, GdkEventSequence *seq, BobguiEventSequenceState state)
{
  GList *gestures, *l;

  gestures = g_object_get_data (G_OBJECT (w), "gestures");

  for (l = gestures; l; l = l->next)
    {
      const char *n;

      n = g_object_get_data (l->data, "name");
      if (g_strcmp0 (n, name) != 0)
        continue;

      g_assert_cmpint (bobgui_gesture_get_sequence_state (l->data, seq), ==, state);
      return;
    }

  g_assert_not_reached ();
}

static void
test_phases (void)
{
  BobguiWidget *A, *B, *C;
  GdkDisplay *display;
  GString *str;
  graphene_rect_t allocation;

  A = bobgui_window_new ();
  display = bobgui_widget_get_display (A);
  if (gdk_display_get_default_seat (display) == NULL)
    {
      g_test_skip ("Display has no seat");
      return;
    }

  bobgui_widget_set_name (A, "A");
  B = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
  bobgui_widget_set_name (B, "B");
  C = bobgui_image_new ();
  bobgui_widget_set_hexpand (C, TRUE);
  bobgui_widget_set_vexpand (C, TRUE);
  bobgui_widget_set_name (C, "C");

  bobgui_window_set_default_size (BOBGUI_WINDOW (A), 400, 400);
  bobgui_window_set_child (BOBGUI_WINDOW (A), B);
  bobgui_box_append (BOBGUI_BOX (B), C);

  show_toplevel (BOBGUI_WINDOW (A));

  str = g_string_new ("");

  add_gesture (A, "a1", BOBGUI_PHASE_CAPTURE, str, BOBGUI_EVENT_SEQUENCE_NONE);
  add_gesture (B, "b1", BOBGUI_PHASE_CAPTURE, str, BOBGUI_EVENT_SEQUENCE_NONE);
  add_gesture (C, "c1", BOBGUI_PHASE_CAPTURE, str, BOBGUI_EVENT_SEQUENCE_NONE);
  add_gesture (A, "a2", BOBGUI_PHASE_TARGET, str, BOBGUI_EVENT_SEQUENCE_NONE);
  add_gesture (B, "b2", BOBGUI_PHASE_TARGET, str, BOBGUI_EVENT_SEQUENCE_NONE);
  add_gesture (C, "c2", BOBGUI_PHASE_TARGET, str, BOBGUI_EVENT_SEQUENCE_NONE);
  add_gesture (A, "a3", BOBGUI_PHASE_BUBBLE, str, BOBGUI_EVENT_SEQUENCE_NONE);
  add_gesture (B, "b3", BOBGUI_PHASE_BUBBLE, str, BOBGUI_EVENT_SEQUENCE_NONE);
  add_gesture (C, "c3", BOBGUI_PHASE_BUBBLE, str, BOBGUI_EVENT_SEQUENCE_NONE);

  g_assert_true (bobgui_widget_compute_bounds (B, B, &allocation));

  point_update (&mouse_state, A,
                allocation.origin.x + allocation.size.width / 2 ,
                allocation.origin.y + allocation.size.height / 2);
  point_press (&mouse_state, A, 1);

  g_assert_cmpstr (str->str, ==,
                   "capture a1, "
                   "capture b1, "
                   "capture c1, "
                   "target c2, "
                   "bubble c3, "
                   "bubble b3, "
                   "bubble a3");

  bobgui_window_destroy (BOBGUI_WINDOW (A));

  g_string_free (str, TRUE);
}

static void
test_mixed (void)
{
  BobguiWidget *A, *B, *C;
  GdkDisplay *display;
  GString *str;
  graphene_rect_t allocation;

  A = bobgui_window_new ();
  display = bobgui_widget_get_display (A);
  if (gdk_display_get_default_seat (display) == NULL)
    {
      g_test_skip ("Display has no seat");
      return;
    }

  bobgui_widget_set_name (A, "A");
  B = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
  bobgui_widget_set_name (B, "B");
  C = bobgui_image_new ();
  bobgui_widget_set_hexpand (C, TRUE);
  bobgui_widget_set_vexpand (C, TRUE);
  bobgui_widget_set_name (C, "C");

  bobgui_window_set_default_size (BOBGUI_WINDOW (A), 400, 400);
  bobgui_window_set_child (BOBGUI_WINDOW (A), B);
  bobgui_box_append (BOBGUI_BOX (B), C);

  show_toplevel (BOBGUI_WINDOW (A));

  str = g_string_new ("");

  add_legacy (A, str, GDK_EVENT_PROPAGATE);
  add_legacy (B, str, GDK_EVENT_PROPAGATE);
  add_legacy (C, str, GDK_EVENT_PROPAGATE);

  add_gesture (A, "a1", BOBGUI_PHASE_CAPTURE, str, BOBGUI_EVENT_SEQUENCE_NONE);
  add_gesture (B, "b1", BOBGUI_PHASE_CAPTURE, str, BOBGUI_EVENT_SEQUENCE_NONE);
  add_gesture (C, "c1", BOBGUI_PHASE_CAPTURE, str, BOBGUI_EVENT_SEQUENCE_NONE);
  add_gesture (A, "a2", BOBGUI_PHASE_TARGET, str, BOBGUI_EVENT_SEQUENCE_NONE);
  add_gesture (B, "b2", BOBGUI_PHASE_TARGET, str, BOBGUI_EVENT_SEQUENCE_NONE);
  add_gesture (C, "c2", BOBGUI_PHASE_TARGET, str, BOBGUI_EVENT_SEQUENCE_NONE);
  add_gesture (A, "a3", BOBGUI_PHASE_BUBBLE, str, BOBGUI_EVENT_SEQUENCE_NONE);
  add_gesture (B, "b3", BOBGUI_PHASE_BUBBLE, str, BOBGUI_EVENT_SEQUENCE_NONE);
  add_gesture (C, "c3", BOBGUI_PHASE_BUBBLE, str, BOBGUI_EVENT_SEQUENCE_NONE);

  g_assert_true (bobgui_widget_compute_bounds (B, B, &allocation));

  point_update (&mouse_state, A,
                allocation.origin.x + allocation.size.width / 2 ,
                allocation.origin.y + allocation.size.height / 2);
  point_press (&mouse_state, A, 1);

  g_assert_cmpstr (str->str, ==,
                   "capture a1, "
                   "capture b1, "
                   "capture c1, "
                   "target c2, "
                   "bubble c3, "
                   "legacy C, "
                   "bubble b3, "
                   "legacy B, "
                   "bubble a3, "
                   "legacy A");

  bobgui_window_destroy (BOBGUI_WINDOW (A));

  g_string_free (str, TRUE);
}

static void
test_early_exit (void)
{
  BobguiWidget *A, *B, *C;
  GdkDisplay *display;
  GString *str;
  graphene_rect_t allocation;

  A = bobgui_window_new ();
  display = bobgui_widget_get_display (A);
  if (gdk_display_get_default_seat (display) == NULL)
    {
      g_test_skip ("Display has no seat");
      return;
    }

  bobgui_widget_set_name (A, "A");
  B = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
  bobgui_widget_set_name (B, "B");
  C = bobgui_image_new ();
  bobgui_widget_set_hexpand (C, TRUE);
  bobgui_widget_set_vexpand (C, TRUE);
  bobgui_widget_set_name (C, "C");

  bobgui_window_set_default_size (BOBGUI_WINDOW (A), 400, 400);
  bobgui_window_set_child (BOBGUI_WINDOW (A), B);
  bobgui_box_append (BOBGUI_BOX (B), C);

  show_toplevel (BOBGUI_WINDOW (A));

  str = g_string_new ("");

  add_legacy (A, str, GDK_EVENT_PROPAGATE);
  add_legacy (B, str, GDK_EVENT_STOP);
  add_legacy (C, str, GDK_EVENT_PROPAGATE);

  add_gesture (A, "a1", BOBGUI_PHASE_CAPTURE, str, BOBGUI_EVENT_SEQUENCE_NONE);
  add_gesture (B, "b1", BOBGUI_PHASE_CAPTURE, str, BOBGUI_EVENT_SEQUENCE_NONE);
  add_gesture (C, "c1", BOBGUI_PHASE_CAPTURE, str, BOBGUI_EVENT_SEQUENCE_NONE);
  add_gesture (C, "c2", BOBGUI_PHASE_TARGET, str, BOBGUI_EVENT_SEQUENCE_NONE);
  add_gesture (A, "a3", BOBGUI_PHASE_BUBBLE, str, BOBGUI_EVENT_SEQUENCE_NONE);
  add_gesture (B, "b3", BOBGUI_PHASE_BUBBLE, str, BOBGUI_EVENT_SEQUENCE_NONE);
  add_gesture (C, "c3", BOBGUI_PHASE_BUBBLE, str, BOBGUI_EVENT_SEQUENCE_NONE);

  g_assert_true (bobgui_widget_compute_bounds (B, B, &allocation));

  point_update (&mouse_state, A,
                allocation.origin.x + allocation.size.width / 2 ,
                allocation.origin.y + allocation.size.height / 2);
  point_press (&mouse_state, A, 1);

  g_assert_cmpstr (str->str, ==,
                   "capture a1, "
                   "capture b1, "
                   "capture c1, "
                   "target c2, "
                   "bubble c3, "
                   "legacy C, "
                   "bubble b3, "
                   "legacy B");

  bobgui_window_destroy (BOBGUI_WINDOW (A));

  g_string_free (str, TRUE);
}

static void
test_claim_capture (void)
{
  BobguiWidget *A, *B, *C;
  GdkDisplay *display;
  GString *str;
  graphene_rect_t allocation;

  A = bobgui_window_new ();
  display = bobgui_widget_get_display (A);
  if (gdk_display_get_default_seat (display) == NULL)
    {
      g_test_skip ("Display has no seat");
      return;
    }

  bobgui_widget_set_name (A, "A");
  B = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
  bobgui_widget_set_name (B, "B");
  C = bobgui_image_new ();
  bobgui_widget_set_hexpand (C, TRUE);
  bobgui_widget_set_vexpand (C, TRUE);
  bobgui_widget_set_name (C, "C");

  bobgui_window_set_default_size (BOBGUI_WINDOW (A), 400, 400);
  bobgui_window_set_child (BOBGUI_WINDOW (A), B);
  bobgui_box_append (BOBGUI_BOX (B), C);

  show_toplevel (BOBGUI_WINDOW (A));

  str = g_string_new ("");

  add_gesture (A, "a1", BOBGUI_PHASE_CAPTURE, str, BOBGUI_EVENT_SEQUENCE_NONE);
  add_gesture (B, "b1", BOBGUI_PHASE_CAPTURE, str, BOBGUI_EVENT_SEQUENCE_NONE);
  add_gesture (C, "c1", BOBGUI_PHASE_CAPTURE, str, BOBGUI_EVENT_SEQUENCE_CLAIMED);
  add_gesture (C, "c2", BOBGUI_PHASE_TARGET, str, BOBGUI_EVENT_SEQUENCE_NONE);
  add_gesture (A, "a3", BOBGUI_PHASE_BUBBLE, str, BOBGUI_EVENT_SEQUENCE_NONE);
  add_gesture (B, "b3", BOBGUI_PHASE_BUBBLE, str, BOBGUI_EVENT_SEQUENCE_NONE);
  add_gesture (C, "c3", BOBGUI_PHASE_BUBBLE, str, BOBGUI_EVENT_SEQUENCE_NONE);

  g_assert_true (bobgui_widget_compute_bounds (B, B, &allocation));

  point_update (&mouse_state, A,
                allocation.origin.x + allocation.size.width / 2 ,
                allocation.origin.y + allocation.size.height / 2);
  point_press (&mouse_state, A, 1);

  g_assert_cmpstr (str->str, ==,
                   "capture a1, "
                   "capture b1, "
                   "capture c1, "
                   "b1 cancelled, "
                   "a1 cancelled, "
                   "c1 state claimed");

  assert_gesture_state (A, "a1", NULL, BOBGUI_EVENT_SEQUENCE_NONE);
  assert_gesture_state (B, "b1", NULL, BOBGUI_EVENT_SEQUENCE_NONE);
  assert_gesture_state (C, "c1", NULL, BOBGUI_EVENT_SEQUENCE_CLAIMED);
  assert_gesture_state (C, "c2", NULL, BOBGUI_EVENT_SEQUENCE_NONE);
  assert_gesture_state (C, "c3", NULL, BOBGUI_EVENT_SEQUENCE_NONE);
  assert_gesture_state (B, "b3", NULL, BOBGUI_EVENT_SEQUENCE_NONE);
  assert_gesture_state (A, "a3", NULL, BOBGUI_EVENT_SEQUENCE_NONE);

  bobgui_window_destroy (BOBGUI_WINDOW (A));

  g_string_free (str, TRUE);
}

static void
test_claim_target (void)
{
  BobguiWidget *A, *B, *C;
  GdkDisplay *display;
  GString *str;
  graphene_rect_t allocation;

  A = bobgui_window_new ();
  display = bobgui_widget_get_display (A);
  if (gdk_display_get_default_seat (display) == NULL)
    {
      g_test_skip ("Display has no seat");
      return;
    }

  bobgui_widget_set_name (A, "A");
  B = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
  bobgui_widget_set_name (B, "B");
  C = bobgui_image_new ();
  bobgui_widget_set_hexpand (C, TRUE);
  bobgui_widget_set_vexpand (C, TRUE);
  bobgui_widget_set_name (C, "C");

  bobgui_window_set_default_size (BOBGUI_WINDOW (A), 400, 400);
  bobgui_window_set_child (BOBGUI_WINDOW (A), B);
  bobgui_box_append (BOBGUI_BOX (B), C);

  show_toplevel (BOBGUI_WINDOW (A));

  str = g_string_new ("");

  add_gesture (A, "a1", BOBGUI_PHASE_CAPTURE, str, BOBGUI_EVENT_SEQUENCE_NONE);
  add_gesture (B, "b1", BOBGUI_PHASE_CAPTURE, str, BOBGUI_EVENT_SEQUENCE_NONE);
  add_gesture (C, "c1", BOBGUI_PHASE_CAPTURE, str, BOBGUI_EVENT_SEQUENCE_NONE);
  add_gesture (C, "c2", BOBGUI_PHASE_TARGET, str, BOBGUI_EVENT_SEQUENCE_CLAIMED);
  add_gesture (A, "a3", BOBGUI_PHASE_BUBBLE, str, BOBGUI_EVENT_SEQUENCE_NONE);
  add_gesture (B, "b3", BOBGUI_PHASE_BUBBLE, str, BOBGUI_EVENT_SEQUENCE_NONE);
  add_gesture (C, "c3", BOBGUI_PHASE_BUBBLE, str, BOBGUI_EVENT_SEQUENCE_NONE);

  g_assert_true (bobgui_widget_compute_bounds (B, B, &allocation));
  point_update (&mouse_state, A,
                allocation.origin.x + allocation.size.width / 2 ,
                allocation.origin.y + allocation.size.height / 2);
  point_press (&mouse_state, A, 1);

  g_assert_cmpstr (str->str, ==,
                   "capture a1, "
                   "capture b1, "
                   "capture c1, "
                   "target c2, "
                   "c1 state denied, "
                   "b1 state denied, "
                   "a1 state denied, "
                   "c2 state claimed");

  assert_gesture_state (A, "a1", NULL, BOBGUI_EVENT_SEQUENCE_DENIED);
  assert_gesture_state (B, "b1", NULL, BOBGUI_EVENT_SEQUENCE_DENIED);
  assert_gesture_state (C, "c1", NULL, BOBGUI_EVENT_SEQUENCE_DENIED);
  assert_gesture_state (C, "c2", NULL, BOBGUI_EVENT_SEQUENCE_CLAIMED);
  assert_gesture_state (C, "c3", NULL, BOBGUI_EVENT_SEQUENCE_NONE);
  assert_gesture_state (B, "b3", NULL, BOBGUI_EVENT_SEQUENCE_NONE);
  assert_gesture_state (A, "a3", NULL, BOBGUI_EVENT_SEQUENCE_NONE);

  bobgui_window_destroy (BOBGUI_WINDOW (A));

  g_string_free (str, TRUE);
}

static void
test_claim_bubble (void)
{
  BobguiWidget *A, *B, *C;
  GdkDisplay *display;
  GString *str;
  graphene_rect_t allocation;

  A = bobgui_window_new ();
  display = bobgui_widget_get_display (A);
  if (gdk_display_get_default_seat (display) == NULL)
    {
      g_test_skip ("Display has no seat");
      return;
    }

  bobgui_widget_set_name (A, "A");
  B = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
  bobgui_widget_set_name (B, "B");
  C = bobgui_image_new ();
  bobgui_widget_set_hexpand (C, TRUE);
  bobgui_widget_set_vexpand (C, TRUE);
  bobgui_widget_set_name (C, "C");

  bobgui_window_set_default_size (BOBGUI_WINDOW (A), 400, 400);
  bobgui_window_set_child (BOBGUI_WINDOW (A), B);
  bobgui_box_append (BOBGUI_BOX (B), C);

  show_toplevel (BOBGUI_WINDOW (A));

  str = g_string_new ("");

  add_gesture (A, "a1", BOBGUI_PHASE_CAPTURE, str, BOBGUI_EVENT_SEQUENCE_NONE);
  add_gesture (B, "b1", BOBGUI_PHASE_CAPTURE, str, BOBGUI_EVENT_SEQUENCE_NONE);
  add_gesture (C, "c1", BOBGUI_PHASE_CAPTURE, str, BOBGUI_EVENT_SEQUENCE_NONE);
  add_gesture (C, "c2", BOBGUI_PHASE_TARGET, str, BOBGUI_EVENT_SEQUENCE_NONE);
  add_gesture (A, "a3", BOBGUI_PHASE_BUBBLE, str, BOBGUI_EVENT_SEQUENCE_NONE);
  add_gesture (B, "b3", BOBGUI_PHASE_BUBBLE, str, BOBGUI_EVENT_SEQUENCE_CLAIMED);
  add_gesture (C, "c3", BOBGUI_PHASE_BUBBLE, str, BOBGUI_EVENT_SEQUENCE_NONE);

  g_assert_true (bobgui_widget_compute_bounds (B, B, &allocation));

  point_update (&mouse_state, A,
                allocation.origin.x + allocation.size.width / 2 ,
                allocation.origin.y + allocation.size.height / 2);
  point_press (&mouse_state, A, 1);

  g_assert_cmpstr (str->str, ==,
                   "capture a1, "
                   "capture b1, "
                   "capture c1, "
                   "target c2, "
                   "bubble c3, "
                   "bubble b3, "
                   "b1 state denied, "
                   "c3 state denied, "
                   "c2 state denied, "
                   "c1 state denied, "
                   "a1 state denied, "
                   "b3 state claimed");

  assert_gesture_state (A, "a1", NULL, BOBGUI_EVENT_SEQUENCE_DENIED);
  assert_gesture_state (B, "b1", NULL, BOBGUI_EVENT_SEQUENCE_DENIED);
  assert_gesture_state (C, "c1", NULL, BOBGUI_EVENT_SEQUENCE_DENIED);
  assert_gesture_state (C, "c2", NULL, BOBGUI_EVENT_SEQUENCE_DENIED);
  assert_gesture_state (C, "c3", NULL, BOBGUI_EVENT_SEQUENCE_DENIED);
  assert_gesture_state (B, "b3", NULL, BOBGUI_EVENT_SEQUENCE_CLAIMED);
  assert_gesture_state (A, "a3", NULL, BOBGUI_EVENT_SEQUENCE_NONE);

  bobgui_window_destroy (BOBGUI_WINDOW (A));

  g_string_free (str, TRUE);
}

static void
test_early_claim_capture (void)
{
  BobguiWidget *A, *B, *C;
  GdkDisplay *display;
  BobguiGesture *g;
  GString *str;
  graphene_rect_t allocation;

  A = bobgui_window_new ();
  display = bobgui_widget_get_display (A);
  if (gdk_display_get_default_seat (display) == NULL)
    {
      g_test_skip ("Display has no seat");
      return;
    }

  bobgui_widget_set_name (A, "A");
  B = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
  bobgui_widget_set_name (B, "B");
  C = bobgui_image_new ();
  bobgui_widget_set_hexpand (C, TRUE);
  bobgui_widget_set_vexpand (C, TRUE);
  bobgui_widget_set_name (C, "C");

  bobgui_window_set_default_size (BOBGUI_WINDOW (A), 400, 400);
  bobgui_window_set_child (BOBGUI_WINDOW (A), B);
  bobgui_box_append (BOBGUI_BOX (B), C);

  show_toplevel (BOBGUI_WINDOW (A));

  str = g_string_new ("");

  add_gesture (A, "a1", BOBGUI_PHASE_CAPTURE, str, BOBGUI_EVENT_SEQUENCE_NONE);
  g = add_gesture (B, "b1", BOBGUI_PHASE_CAPTURE, str, BOBGUI_EVENT_SEQUENCE_CLAIMED);
  add_gesture (C, "c1", BOBGUI_PHASE_CAPTURE, str, BOBGUI_EVENT_SEQUENCE_CLAIMED);
  add_gesture (C, "c2", BOBGUI_PHASE_TARGET, str, BOBGUI_EVENT_SEQUENCE_NONE);
  add_gesture (A, "a3", BOBGUI_PHASE_BUBBLE, str, BOBGUI_EVENT_SEQUENCE_NONE);
  add_gesture (B, "b3", BOBGUI_PHASE_BUBBLE, str, BOBGUI_EVENT_SEQUENCE_NONE);
  add_gesture (C, "c3", BOBGUI_PHASE_BUBBLE, str, BOBGUI_EVENT_SEQUENCE_NONE);

  g_assert_true (bobgui_widget_compute_bounds (B, B, &allocation));

  point_update (&mouse_state, A,
                allocation.origin.x + allocation.size.width / 2 ,
                allocation.origin.y + allocation.size.height / 2);
  point_press (&mouse_state, A, 1);

  g_assert_cmpstr (str->str, ==,
                   "capture a1, "
                   "capture b1, "
                   "a1 cancelled, "
                   "b1 state claimed");

  /* Reset the string */
  g_string_erase (str, 0, str->len);

  bobgui_gesture_set_state (g, BOBGUI_EVENT_SEQUENCE_DENIED);

  g_assert_cmpstr (str->str, ==,
                   "capture a1, "
                   "capture c1, "
                   "b1 cancelled, "
                   "a1 cancelled, "
                   "c1 state claimed, "
                   "b1 state denied");

  point_release (&mouse_state, 1);
  bobgui_window_destroy (BOBGUI_WINDOW (A));

  g_string_free (str, TRUE);
}

static void
test_late_claim_capture (void)
{
  BobguiWidget *A, *B, *C;
  GdkDisplay *display;
  BobguiGesture *g;
  GString *str;
  graphene_rect_t allocation;

  A = bobgui_window_new ();
  display = bobgui_widget_get_display (A);
  if (gdk_display_get_default_seat (display) == NULL)
    {
      g_test_skip ("Display has no seat");
      return;
    }

  bobgui_widget_set_name (A, "A");
  B = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
  bobgui_widget_set_name (B, "B");
  C = bobgui_image_new ();
  bobgui_widget_set_hexpand (C, TRUE);
  bobgui_widget_set_vexpand (C, TRUE);
  bobgui_widget_set_name (C, "C");

  bobgui_window_set_default_size (BOBGUI_WINDOW (A), 400, 400);
  bobgui_window_set_child (BOBGUI_WINDOW (A), B);
  bobgui_box_append (BOBGUI_BOX (B), C);

  show_toplevel (BOBGUI_WINDOW (A));

  str = g_string_new ("");

  add_gesture (A, "a1", BOBGUI_PHASE_CAPTURE, str, BOBGUI_EVENT_SEQUENCE_NONE);
  g = add_gesture (B, "b1", BOBGUI_PHASE_CAPTURE, str, BOBGUI_EVENT_SEQUENCE_NONE);
  add_gesture (C, "c1", BOBGUI_PHASE_CAPTURE, str, BOBGUI_EVENT_SEQUENCE_NONE);
  add_gesture (C, "c2", BOBGUI_PHASE_TARGET, str, BOBGUI_EVENT_SEQUENCE_CLAIMED);
  add_gesture (A, "a3", BOBGUI_PHASE_BUBBLE, str, BOBGUI_EVENT_SEQUENCE_NONE);
  add_gesture (B, "b3", BOBGUI_PHASE_BUBBLE, str, BOBGUI_EVENT_SEQUENCE_NONE);
  add_gesture (C, "c3", BOBGUI_PHASE_BUBBLE, str, BOBGUI_EVENT_SEQUENCE_NONE);

  g_assert_true (bobgui_widget_compute_bounds (B, B, &allocation));

  point_update (&mouse_state, A,
                allocation.origin.x + allocation.size.width / 2 ,
                allocation.origin.y + allocation.size.height / 2);
  point_press (&mouse_state, A, 1);

  g_assert_cmpstr (str->str, ==,
                   "capture a1, "
                   "capture b1, "
                   "capture c1, "
                   "target c2, "
                   "c1 state denied, "
                   "b1 state denied, "
                   "a1 state denied, "
                   "c2 state claimed");

  /* Reset the string */
  g_string_erase (str, 0, str->len);

  bobgui_gesture_set_state (g, BOBGUI_EVENT_SEQUENCE_CLAIMED);

  g_assert_cmpstr (str->str, ==, "");

  point_release (&mouse_state, 1);
  bobgui_window_destroy (BOBGUI_WINDOW (A));

  g_string_free (str, TRUE);
}

static void
test_group (void)
{
  BobguiWidget *A, *B, *C;
  GdkDisplay *display;
  GString *str;
  BobguiGesture *g1, *g2;
  graphene_rect_t allocation;

  A = bobgui_window_new ();
  display = bobgui_widget_get_display (A);
  if (gdk_display_get_default_seat (display) == NULL)
    {
      g_test_skip ("Display has no seat");
      return;
    }

  bobgui_widget_set_name (A, "A");
  B = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
  bobgui_widget_set_name (B, "B");
  C = bobgui_image_new ();
  bobgui_widget_set_hexpand (C, TRUE);
  bobgui_widget_set_vexpand (C, TRUE);
  bobgui_widget_set_name (C, "C");

  bobgui_window_set_default_size (BOBGUI_WINDOW (A), 400, 400);
  bobgui_window_set_child (BOBGUI_WINDOW (A), B);
  bobgui_box_append (BOBGUI_BOX (B), C);

  show_toplevel (BOBGUI_WINDOW (A));

  str = g_string_new ("");

  add_gesture (A, "a1", BOBGUI_PHASE_CAPTURE, str, BOBGUI_EVENT_SEQUENCE_NONE);
  add_gesture (B, "b1", BOBGUI_PHASE_CAPTURE, str, BOBGUI_EVENT_SEQUENCE_NONE);
  add_gesture (C, "c1", BOBGUI_PHASE_CAPTURE, str, BOBGUI_EVENT_SEQUENCE_NONE);
  g1 = add_gesture (C, "c2", BOBGUI_PHASE_TARGET, str, BOBGUI_EVENT_SEQUENCE_NONE);
  g2 = add_gesture (C, "c3", BOBGUI_PHASE_TARGET, str, BOBGUI_EVENT_SEQUENCE_CLAIMED);
  bobgui_gesture_group (g1, g2);
  add_gesture (A, "a3", BOBGUI_PHASE_BUBBLE, str, BOBGUI_EVENT_SEQUENCE_NONE);
  add_gesture (B, "b3", BOBGUI_PHASE_BUBBLE, str, BOBGUI_EVENT_SEQUENCE_NONE);
  add_gesture (C, "c4", BOBGUI_PHASE_BUBBLE, str, BOBGUI_EVENT_SEQUENCE_NONE);

  g_assert_true (bobgui_widget_compute_bounds (B, B, &allocation));

  point_update (&mouse_state, A,
                allocation.origin.x + allocation.size.width / 2 ,
                allocation.origin.y + allocation.size.height / 2);
  point_press (&mouse_state, A, 1);

  g_assert_cmpstr (str->str, ==,
                   "capture a1, "
                   "capture b1, "
                   "capture c1, "
                   "target c3, "
                   "c1 state denied, "
                   "b1 state denied, "
                   "a1 state denied, "
                   "c3 state claimed, "
                   "c2 state claimed, "
                   "target c2");

  bobgui_window_destroy (BOBGUI_WINDOW (A));

  g_string_free (str, TRUE);
}

G_GNUC_UNUSED static void
test_gestures_outside_grab (void)
{
  BobguiWidget *A, *B, *C, *D, *E;
  GdkDisplay *display;
  GString *str;
  graphene_rect_t allocation;

  A = bobgui_window_new ();
  display = bobgui_widget_get_display (A);
  if (gdk_display_get_default_seat (display) == NULL)
    {
      g_test_skip ("Display has no seat");
      return;
    }

  bobgui_widget_set_name (A, "A");
  B = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
  bobgui_widget_set_name (B, "B");
  C = bobgui_image_new ();
  bobgui_widget_set_hexpand (C, TRUE);
  bobgui_widget_set_vexpand (C, TRUE);
  bobgui_widget_set_name (C, "C");

  bobgui_window_set_default_size (BOBGUI_WINDOW (A), 400, 400);
  bobgui_window_set_child (BOBGUI_WINDOW (A), B);
  bobgui_box_append (BOBGUI_BOX (B), C);

  show_toplevel (BOBGUI_WINDOW (A));

  D = bobgui_popover_new ();
  bobgui_widget_set_name (D, "D");
  bobgui_widget_set_parent (D, C);

  E = bobgui_image_new ();
  bobgui_widget_set_name (E, "E");
  bobgui_popover_set_child (BOBGUI_POPOVER (D), E);

  str = g_string_new ("");

  add_gesture (A, "a1", BOBGUI_PHASE_CAPTURE, str, BOBGUI_EVENT_SEQUENCE_NONE);
  add_gesture (B, "b1", BOBGUI_PHASE_CAPTURE, str, BOBGUI_EVENT_SEQUENCE_NONE);
  add_gesture (C, "c1", BOBGUI_PHASE_CAPTURE, str, BOBGUI_EVENT_SEQUENCE_NONE);
  add_gesture (C, "c2", BOBGUI_PHASE_TARGET, str, BOBGUI_EVENT_SEQUENCE_CLAIMED);
  add_gesture (B, "b2", BOBGUI_PHASE_BUBBLE, str, BOBGUI_EVENT_SEQUENCE_NONE);
  add_gesture (A, "a2", BOBGUI_PHASE_BUBBLE, str, BOBGUI_EVENT_SEQUENCE_NONE);

  g_assert_true (bobgui_widget_compute_bounds (B, B, &allocation));

  point_update (&mouse_state, A,
                allocation.origin.x + allocation.size.width / 2 ,
                allocation.origin.y + allocation.size.height / 2);
  point_press (&mouse_state, A, 1);

  g_assert_cmpstr (str->str, ==,
                   "capture a1, "
                   "capture b1, "
                   "capture c1, "
                   "target c2, "
                   "c1 state denied, "
                   "c2 state claimed");

  /* Set a grab on another window */
  g_string_erase (str, 0, str->len);
  show_popover (BOBGUI_POPOVER (D));

  g_assert_cmpstr (str->str, ==,
                   "c1 cancelled, "
                   "c2 cancelled, "
                   "b1 cancelled, "
                   "a1 cancelled");

  bobgui_window_destroy (BOBGUI_WINDOW (A));

  g_string_free (str, TRUE);
}

G_GNUC_UNUSED static void
test_gestures_inside_grab (void)
{
  BobguiWidget *A, *B, *C;
  GdkDisplay *display;
  GString *str;
  graphene_rect_t allocation;

  A = bobgui_window_new ();
  display = bobgui_widget_get_display (A);
  if (gdk_display_get_default_seat (display) == NULL)
    {
      g_test_skip ("Display has no seat");
      return;
    }

  bobgui_widget_set_name (A, "A");
  B = bobgui_popover_new ();
  bobgui_widget_set_name (B, "B");
  bobgui_widget_set_parent (B, A);
  C = bobgui_image_new ();
  bobgui_widget_set_hexpand (C, TRUE);
  bobgui_widget_set_vexpand (C, TRUE);
  bobgui_widget_set_name (C, "C");
  bobgui_popover_set_child (BOBGUI_POPOVER (B), C);

  bobgui_window_set_default_size (BOBGUI_WINDOW (A), 400, 400);

  show_toplevel (BOBGUI_WINDOW (A));
  show_popover (BOBGUI_POPOVER (B));

  str = g_string_new ("");

  add_gesture (A, "a1", BOBGUI_PHASE_CAPTURE, str, BOBGUI_EVENT_SEQUENCE_NONE);
  add_gesture (B, "b1", BOBGUI_PHASE_CAPTURE, str, BOBGUI_EVENT_SEQUENCE_NONE);
  add_gesture (C, "c1", BOBGUI_PHASE_CAPTURE, str, BOBGUI_EVENT_SEQUENCE_NONE);
  add_gesture (C, "c2", BOBGUI_PHASE_TARGET, str, BOBGUI_EVENT_SEQUENCE_CLAIMED);
  add_gesture (B, "b2", BOBGUI_PHASE_BUBBLE, str, BOBGUI_EVENT_SEQUENCE_NONE);
  add_gesture (A, "a2", BOBGUI_PHASE_BUBBLE, str, BOBGUI_EVENT_SEQUENCE_NONE);

  g_assert_true (bobgui_widget_compute_bounds (B, B, &allocation));

  point_update (&mouse_state, A,
                allocation.origin.x + allocation.size.width / 2 ,
                allocation.origin.y + allocation.size.height / 2);
  point_press (&mouse_state, A, 1);

  g_assert_cmpstr (str->str, ==,
                   "capture a1, "
                   "capture b1, "
                   "capture c1, "
                   "target c2, "
                   "c2 state claimed");

  g_string_erase (str, 0, str->len);
  g_assert_cmpstr (str->str, ==,
                   "a1 cancelled");

  /* Update with the grab under effect */
  g_string_erase (str, 0, str->len);
  point_update (&mouse_state, A, allocation.origin.x, allocation.origin.y);
  g_assert_cmpstr (str->str, ==,
                   "b1 updated, "
                   "c1 updated, "
                   "c2 updated");

  bobgui_window_destroy (BOBGUI_WINDOW (A));

  g_string_free (str, TRUE);
}

G_GNUC_UNUSED static void
test_multitouch_on_single (void)
{
  BobguiWidget *A, *B, *C;
  GdkDisplay *display;
  GString *str;
  graphene_rect_t allocation;

  A = bobgui_window_new ();
  display = bobgui_widget_get_display (A);
  if (gdk_display_get_default_seat (display) == NULL)
    {
      g_test_skip ("Display has no seat");
      return;
    }

  bobgui_widget_set_name (A, "A");
  B = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
  bobgui_widget_set_name (B, "B");
  C = bobgui_image_new ();
  bobgui_widget_set_hexpand (C, TRUE);
  bobgui_widget_set_vexpand (C, TRUE);
  bobgui_widget_set_name (C, "C");

  bobgui_window_set_default_size (BOBGUI_WINDOW (A), 400, 400);
  bobgui_window_set_child (BOBGUI_WINDOW (A), B);
  bobgui_box_append (BOBGUI_BOX (B), C);

  show_toplevel (BOBGUI_WINDOW (A));

  str = g_string_new ("");

  add_gesture (A, "a1", BOBGUI_PHASE_CAPTURE, str, BOBGUI_EVENT_SEQUENCE_NONE);
  add_gesture (B, "b1", BOBGUI_PHASE_CAPTURE, str, BOBGUI_EVENT_SEQUENCE_CLAIMED);

  g_assert_true (bobgui_widget_compute_bounds (B, B, &allocation));

  /* First touch down */
  point_update (&touch_state[0], A,
                allocation.origin.x + allocation.size.width / 2 ,
                allocation.origin.y + allocation.size.height / 2);
  point_press (&touch_state[0], A, 1);

  g_assert_cmpstr (str->str, ==,
                   "capture a1 (1), "
                   "capture b1 (1), "
                   "a1 cancelled, "
                   "b1 state claimed (1)");

  /* Second touch down */
  g_string_erase (str, 0, str->len);
  point_update (&touch_state[1], A,
                allocation.origin.x + allocation.size.width / 2 ,
                allocation.origin.y + allocation.size.height / 2);
  point_press (&touch_state[1], A, 1);

  g_assert_cmpstr (str->str, ==,
                   "capture a1 (2), "
                   "b1 state denied (2)");

  bobgui_window_destroy (BOBGUI_WINDOW (A));

  g_string_free (str, TRUE);
}

G_GNUC_UNUSED static void
test_multitouch_activation (void)
{
  BobguiWidget *A, *B, *C;
  GdkDisplay *display;
  GString *str;
  graphene_rect_t allocation;

  A = bobgui_window_new ();
  display = bobgui_widget_get_display (A);
  if (gdk_display_get_default_seat (display) == NULL)
    {
      g_test_skip ("Display has no seat");
      return;
    }

  bobgui_widget_set_name (A, "A");
  B = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
  bobgui_widget_set_name (B, "B");
  C = bobgui_image_new ();
  bobgui_widget_set_hexpand (C, TRUE);
  bobgui_widget_set_vexpand (C, TRUE);
  bobgui_widget_set_name (C, "C");

  bobgui_window_set_default_size (BOBGUI_WINDOW (A), 400, 400);
  bobgui_window_set_child (BOBGUI_WINDOW (A), B);
  bobgui_box_append (BOBGUI_BOX (B), C);

  show_toplevel (BOBGUI_WINDOW (A));

  str = g_string_new ("");

  add_mt_gesture (C, "c1", BOBGUI_PHASE_BUBBLE, str, BOBGUI_EVENT_SEQUENCE_CLAIMED);
  g_assert_true (bobgui_widget_compute_bounds (B, B, &allocation));

  /* First touch down */
  point_update (&touch_state[0], A,
                allocation.origin.x + allocation.size.width / 2 ,
                allocation.origin.y + allocation.size.height / 2);
  point_press (&touch_state[0], A, 1);

  g_assert_cmpstr (str->str, ==, "");

  /* Second touch down */
  point_update (&touch_state[1], A,
                allocation.origin.x + allocation.size.width / 2 ,
                allocation.origin.y + allocation.size.height / 2);
  point_press (&touch_state[1], A, 1);

  g_assert_cmpstr (str->str, ==,
                   "c1 began, "
                   "c1 state claimed (2), "
                   "c1 state claimed (1)");

  /* First touch up */
  g_string_erase (str, 0, str->len);
  point_release (&touch_state[0], 1);

  g_assert_cmpstr (str->str, ==,
                   "c1 ended");

  /* A third touch down triggering again action */
  g_string_erase (str, 0, str->len);
  point_update (&touch_state[2], A,
                allocation.origin.x + allocation.size.width / 2 ,
                allocation.origin.y + allocation.size.height / 2);
  point_press (&touch_state[2], A, 1);

  g_assert_cmpstr (str->str, ==,
                   "c1 began, "
                   "c1 state claimed (3)");

  /* One touch up, gesture is finished again */
  g_string_erase (str, 0, str->len);
  point_release (&touch_state[2], 1);

  g_assert_cmpstr (str->str, ==,
                   "c1 ended");

  /* Another touch up, gesture remains inactive */
  g_string_erase (str, 0, str->len);
  point_release (&touch_state[1], 1);

  g_assert_cmpstr (str->str, ==, "");

  bobgui_window_destroy (BOBGUI_WINDOW (A));

  g_string_free (str, TRUE);
}

G_GNUC_UNUSED static void
test_multitouch_interaction (void)
{
  BobguiWidget *A, *B, *C;
  GdkDisplay *display;
  BobguiGesture *g;
  GString *str;
  graphene_rect_t allocation;

  A = bobgui_window_new ();
  display = bobgui_widget_get_display (A);
  if (gdk_display_get_default_seat (display) == NULL)
    {
      g_test_skip ("Display has no seat");
      return;
    }

  bobgui_widget_set_name (A, "A");
  B = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
  bobgui_widget_set_name (B, "B");
  C = bobgui_image_new ();
  bobgui_widget_set_hexpand (C, TRUE);
  bobgui_widget_set_vexpand (C, TRUE);
  bobgui_widget_set_name (C, "C");

  bobgui_window_set_default_size (BOBGUI_WINDOW (A), 400, 400);
  bobgui_window_set_child (BOBGUI_WINDOW (A), B);
  bobgui_box_append (BOBGUI_BOX (B), C);

  show_toplevel (BOBGUI_WINDOW (A));

  str = g_string_new ("");

  g = add_gesture (A, "a1", BOBGUI_PHASE_CAPTURE, str, BOBGUI_EVENT_SEQUENCE_CLAIMED);
  add_mt_gesture (C, "c1", BOBGUI_PHASE_BUBBLE, str, BOBGUI_EVENT_SEQUENCE_CLAIMED);
  g_assert_true (bobgui_widget_compute_bounds (B, B, &allocation));

  /* First touch down, a1 claims the sequence */
  point_update (&touch_state[0], A,
                allocation.origin.x + allocation.size.width / 2 ,
                allocation.origin.y + allocation.size.height / 2);
  point_press (&touch_state[0], A, 1);

  g_assert_cmpstr (str->str, ==,
                   "capture a1 (1), "
                   "a1 state claimed (1)");

  /* Second touch down, a1 denies and c1 takes over */
  g_string_erase (str, 0, str->len);
  point_update (&touch_state[1], A,
                allocation.origin.x + allocation.size.width / 2 ,
                allocation.origin.y + allocation.size.height / 2);
  point_press (&touch_state[1], A, 1);

  /* Denying sequences in touch-excess situation is a responsibility of the caller */
  bobgui_gesture_set_state (g, BOBGUI_EVENT_SEQUENCE_DENIED);

  g_assert_cmpstr (str->str, ==,
                   "a1 state denied (2), "
                   "c1 began, "
                   "c1 state claimed (1), "
                   "c1 state claimed (2), "
                   "a1 state denied (1)");

  /* Move first point, only c1 should update */
  g_string_erase (str, 0, str->len);
  point_update (&touch_state[0], A, allocation.origin.x, allocation.origin.y);

  g_assert_cmpstr (str->str, ==,
                   "c1 updated");

  /* First touch up */
  g_string_erase (str, 0, str->len);
  point_release (&touch_state[0], 1);

  g_assert_cmpstr (str->str, ==,
                   "c1 ended");

  /* A third touch down triggering again action on c1 */
  g_string_erase (str, 0, str->len);
  point_update (&touch_state[2], A,
                allocation.origin.x + allocation.size.width / 2 ,
                allocation.origin.y + allocation.size.height / 2);
  point_press (&touch_state[2], A, 1);

  g_assert_cmpstr (str->str, ==,
                   "a1 state denied (3), "
                   "c1 began, "
                   "c1 state claimed (3)");

  /* One touch up, gesture is finished again */
  g_string_erase (str, 0, str->len);
  point_release (&touch_state[2], 1);

  g_assert_cmpstr (str->str, ==,
                   "c1 ended");

  /* Another touch up, gesture remains inactive */
  g_string_erase (str, 0, str->len);
  point_release (&touch_state[1], 1);

  g_assert_cmpstr (str->str, ==, "");

  bobgui_window_destroy (BOBGUI_WINDOW (A));

  g_string_free (str, TRUE);
}

int
main (int argc, char *argv[])
{
  bobgui_test_init (&argc, &argv);

  g_test_add_func ("/gestures/propagation/phases", test_phases);
  g_test_add_func ("/gestures/propagation/mixed", test_mixed);
  g_test_add_func ("/gestures/propagation/early-exit", test_early_exit);
  g_test_add_func ("/gestures/claim/capture", test_claim_capture);
  g_test_add_func ("/gestures/claim/target", test_claim_target);
  g_test_add_func ("/gestures/claim/bubble", test_claim_bubble);
  g_test_add_func ("/gestures/claim/early-capture", test_early_claim_capture);
  g_test_add_func ("/gestures/claim/late-capture", test_late_claim_capture);
  g_test_add_func ("/gestures/group", test_group);
#if 0
  g_test_add_func ("/gestures/grabs/gestures-outside-grab", test_gestures_outside_grab);
  g_test_add_func ("/gestures/grabs/gestures-inside-grab", test_gestures_inside_grab);
  g_test_add_func ("/gestures/multitouch/gesture-single", test_multitouch_on_single);
  g_test_add_func ("/gestures/multitouch/multitouch-activation", test_multitouch_activation);
  g_test_add_func ("/gestures/multitouch/interaction", test_multitouch_interaction);
#endif

  return g_test_run ();
}
