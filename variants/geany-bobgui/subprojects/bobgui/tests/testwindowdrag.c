#include <bobgui/bobgui.h>

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

static void
start_resize (BobguiGestureClick *gesture,
              int n_press,
              double x,
              double y,
              gpointer   data)
{
  BobguiWidget *widget = bobgui_event_controller_get_widget (BOBGUI_EVENT_CONTROLLER (gesture));
  GdkSurfaceEdge edge = GPOINTER_TO_INT (data);
  GdkSurface *surface;
  GdkEvent *event;
  guint button;
  guint32 timestamp;

  bobgui_gesture_set_state (BOBGUI_GESTURE (gesture), BOBGUI_EVENT_SEQUENCE_CLAIMED);

  surface = bobgui_native_get_surface (bobgui_widget_get_native (widget));
  event = bobgui_event_controller_get_current_event (BOBGUI_EVENT_CONTROLLER (gesture));
  if (gdk_event_get_event_type (event) == GDK_BUTTON_PRESS)
    button = gdk_button_event_get_button (event);
  else
    button = 0;
  timestamp = gdk_event_get_time (event);

  bobgui_widget_translate_coordinates (widget, BOBGUI_WIDGET (bobgui_widget_get_root (widget)),
                                    x, y, &x, &y);
  gdk_toplevel_begin_resize (GDK_TOPLEVEL (surface), edge, gdk_event_get_device (event), button, x, y, timestamp);

  bobgui_event_controller_reset (BOBGUI_EVENT_CONTROLLER (gesture));
}

static BobguiWidget *
resize_button (GdkSurfaceEdge edge)
{
  BobguiWidget *button;
  BobguiGesture *gesture;

  button = bobgui_image_new_from_icon_name ("view-fullscreen-symbolic");
  bobgui_widget_set_hexpand (button, TRUE);
  bobgui_widget_set_vexpand (button, TRUE);
  gesture = bobgui_gesture_click_new ();
  g_signal_connect (gesture, "pressed", G_CALLBACK (start_resize), GINT_TO_POINTER (edge));
  bobgui_widget_add_controller (button, BOBGUI_EVENT_CONTROLLER (gesture));

  return button;
}

static void
start_move (BobguiGestureClick *gesture,
            int n_press,
            double x,
            double y,
            gpointer   data)
{
  BobguiWidget *widget = bobgui_event_controller_get_widget (BOBGUI_EVENT_CONTROLLER (gesture));
  GdkSurface *surface;
  GdkEvent *event;
  guint button;
  guint32 timestamp;

  bobgui_gesture_set_state (BOBGUI_GESTURE (gesture), BOBGUI_EVENT_SEQUENCE_CLAIMED);

  surface = bobgui_native_get_surface (bobgui_widget_get_native (widget));
  event = bobgui_event_controller_get_current_event (BOBGUI_EVENT_CONTROLLER (gesture));
  if (gdk_event_get_event_type (event) == GDK_BUTTON_PRESS)
    button = gdk_button_event_get_button (event);
  else
    button = 0;
  timestamp = gdk_event_get_time (event);

  bobgui_widget_translate_coordinates (widget, BOBGUI_WIDGET (bobgui_widget_get_root (widget)),
                                    x, y, &x, &y);
  gdk_toplevel_begin_move (GDK_TOPLEVEL (surface), gdk_event_get_device (event), button, x, y, timestamp);
  bobgui_event_controller_reset (BOBGUI_EVENT_CONTROLLER (gesture));
}

static BobguiWidget *
move_button (void)
{
  BobguiWidget *button;
  BobguiGesture *gesture;

  button = bobgui_image_new_from_icon_name ("view-grid-symbolic");
  bobgui_widget_set_hexpand (button, TRUE);
  bobgui_widget_set_vexpand (button, TRUE);
  gesture = bobgui_gesture_click_new ();
  g_signal_connect (gesture, "pressed", G_CALLBACK (start_move), NULL);
  bobgui_widget_add_controller (button, BOBGUI_EVENT_CONTROLLER (gesture));

  return button;
}

int
main (int argc, char *argv[])
{
  BobguiWidget *window;
  BobguiWidget *grid;

  bobgui_init ();

  window = bobgui_window_new ();
  bobgui_window_set_titlebar (BOBGUI_WINDOW (window), bobgui_header_bar_new ());

  grid = bobgui_grid_new ();
  bobgui_window_set_child (BOBGUI_WINDOW (window), grid);

  bobgui_grid_attach (BOBGUI_GRID (grid),
                   resize_button (GDK_SURFACE_EDGE_NORTH_WEST), 
                   0, 0, 1, 1);
  bobgui_grid_attach (BOBGUI_GRID (grid),
                   resize_button (GDK_SURFACE_EDGE_NORTH), 
                   1, 0, 1, 1);
  bobgui_grid_attach (BOBGUI_GRID (grid),
                   resize_button (GDK_SURFACE_EDGE_NORTH_EAST), 
                   2, 0, 1, 1);
  bobgui_grid_attach (BOBGUI_GRID (grid),
                   resize_button (GDK_SURFACE_EDGE_WEST), 
                   0, 1, 1, 1);
  bobgui_grid_attach (BOBGUI_GRID (grid),
                   move_button (),
                   1, 1, 1, 1);
  bobgui_grid_attach (BOBGUI_GRID (grid),
                   resize_button (GDK_SURFACE_EDGE_EAST), 
                   2, 1, 1, 1);
  bobgui_grid_attach (BOBGUI_GRID (grid),
                   resize_button (GDK_SURFACE_EDGE_SOUTH_WEST), 
                   0, 2, 1, 1);
  bobgui_grid_attach (BOBGUI_GRID (grid),
                   resize_button (GDK_SURFACE_EDGE_SOUTH), 
                   1, 2, 1, 1);
  bobgui_grid_attach (BOBGUI_GRID (grid),
                   resize_button (GDK_SURFACE_EDGE_SOUTH_EAST), 
                   2, 2, 1, 1);

  bobgui_window_present (BOBGUI_WINDOW (window));

  while (TRUE)
    g_main_context_iteration (NULL, TRUE);

  return 0;
}
