#include <bobgui/bobgui.h>

/* Surface to store current scribbles */
static cairo_surface_t *surface = NULL;

static void
clear_surface (void)
{
  cairo_t *cr;

  cr = cairo_create (surface);

  cairo_set_source_rgb (cr, 1, 1, 1);
  cairo_paint (cr);

  cairo_destroy (cr);
}

/* Create a new surface of the appropriate size to store our scribbles */
static void
resize_cb (BobguiWidget *widget,
           int        width,
           int        height,
           gpointer   data)
{
  if (surface)
    {
      cairo_surface_destroy (surface);
      surface = NULL;
    }

  if (bobgui_native_get_surface (bobgui_widget_get_native (widget)))
    {
      surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
                                            bobgui_widget_get_width (widget),
                                            bobgui_widget_get_height (widget));

      /* Initialize the surface to white */
      clear_surface ();
    }
}

/* Redraw the screen from the surface. Note that the draw
 * callback receives a ready-to-be-used cairo_t that is already
 * clipped to only draw the exposed areas of the widget
 */
static void
draw_cb (BobguiDrawingArea *drawing_area,
         cairo_t        *cr,
         int             width,
         int             height,
         gpointer        data)
{
  cairo_set_source_surface (cr, surface, 0, 0);
  cairo_paint (cr);
}

/* Draw a rectangle on the surface at the given position */
static void
draw_brush (BobguiWidget *widget,
            double     x,
            double     y)
{
  cairo_t *cr;

  /* Paint to the surface, where we store our state */
  cr = cairo_create (surface);

  cairo_rectangle (cr, x - 3, y - 3, 6, 6);
  cairo_fill (cr);

  cairo_destroy (cr);

  /* Now invalidate the drawing area. */
  bobgui_widget_queue_draw (widget);
}

static double start_x;
static double start_y;

static void
drag_begin (BobguiGestureDrag *gesture,
            double          x,
            double          y,
            BobguiWidget      *area)
{
  start_x = x;
  start_y = y;

  draw_brush (area, x, y);
}

static void
drag_update (BobguiGestureDrag *gesture,
             double          x,
             double          y,
             BobguiWidget      *area)
{
  draw_brush (area, start_x + x, start_y + y);
}

static void
drag_end (BobguiGestureDrag *gesture,
          double          x,
          double          y,
          BobguiWidget      *area)
{
  draw_brush (area, start_x + x, start_y + y);
}

static void
pressed (BobguiGestureClick *gesture,
         int              n_press,
         double           x,
         double           y,
         BobguiWidget       *area)
{
  clear_surface ();
  bobgui_widget_queue_draw (area);
}

static void
close_window (void)
{
  if (surface)
    cairo_surface_destroy (surface);
}

static void
activate (BobguiApplication *app,
          gpointer        user_data)
{
  BobguiWidget *window;
  BobguiWidget *drawing_area;
  BobguiGesture *drag;
  BobguiGesture *press;

  window = bobgui_application_window_new (app);
  bobgui_window_set_title (BOBGUI_WINDOW (window), "Drawing Area");

  g_signal_connect (window, "destroy", G_CALLBACK (close_window), NULL);

  drawing_area = bobgui_drawing_area_new ();
  /* set a minimum size */
  bobgui_widget_set_size_request (drawing_area, 100, 100);

  bobgui_window_set_child (BOBGUI_WINDOW (window), drawing_area);

  bobgui_drawing_area_set_draw_func (BOBGUI_DRAWING_AREA (drawing_area), draw_cb, NULL, NULL);

  g_signal_connect_after (drawing_area, "resize", G_CALLBACK (resize_cb), NULL);

  drag = bobgui_gesture_drag_new ();
  bobgui_gesture_single_set_button (BOBGUI_GESTURE_SINGLE (drag), GDK_BUTTON_PRIMARY);
  bobgui_widget_add_controller (drawing_area, BOBGUI_EVENT_CONTROLLER (drag));
  g_signal_connect (drag, "drag-begin", G_CALLBACK (drag_begin), drawing_area);
  g_signal_connect (drag, "drag-update", G_CALLBACK (drag_update), drawing_area);
  g_signal_connect (drag, "drag-end", G_CALLBACK (drag_end), drawing_area);

  press = bobgui_gesture_click_new ();
  bobgui_gesture_single_set_button (BOBGUI_GESTURE_SINGLE (press), GDK_BUTTON_SECONDARY);
  bobgui_widget_add_controller (drawing_area, BOBGUI_EVENT_CONTROLLER (press));

  g_signal_connect (press, "pressed", G_CALLBACK (pressed), drawing_area);

  bobgui_window_present (BOBGUI_WINDOW (window));
}

int
main (int    argc,
      char **argv)
{
  BobguiApplication *app;
  int status;

  app = bobgui_application_new ("org.bobgui.example", G_APPLICATION_DEFAULT_FLAGS);
  g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
  status = g_application_run (G_APPLICATION (app), argc, argv);
  g_object_unref (app);

  return status;
}
