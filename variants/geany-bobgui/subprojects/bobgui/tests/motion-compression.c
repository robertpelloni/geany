#include <bobgui/bobgui.h>
#include <math.h>

BobguiAdjustment *adjustment;
int cursor_x, cursor_y;

static void
motion_cb (BobguiEventControllerMotion *motion,
           double                    x,
           double                    y,
           BobguiWidget                *widget)
{
  float processing_ms = bobgui_adjustment_get_value (adjustment);
  g_usleep (processing_ms * 1000);

  cursor_x = x;
  cursor_y = y;
  bobgui_widget_queue_draw (widget);
}

static void
on_draw (BobguiDrawingArea *da,
         cairo_t        *cr,
         int             width,
         int             height,
         gpointer        data)
{
  cairo_set_source_rgb (cr, 1, 1, 1);
  cairo_paint (cr);

  cairo_set_source_rgb (cr, 0, 0.5, 0.5);

  cairo_arc (cr, cursor_x, cursor_y, 10, 0, 2 * M_PI);
  cairo_stroke (cr);
}

static void
quit_cb (BobguiWidget *widget,
         gpointer   data)
{
  gboolean *done = data;

  *done = TRUE;

  g_main_context_wakeup (NULL);
}

int
main (int argc, char **argv)
{
  BobguiWidget *window;
  BobguiWidget *vbox;
  BobguiWidget *label;
  BobguiWidget *scale;
  BobguiWidget *da;
  BobguiEventController *controller;
  gboolean done = FALSE;

  bobgui_init ();

  window = bobgui_window_new ();
  bobgui_window_set_default_size (BOBGUI_WINDOW (window), 300, 300);

  vbox = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
  bobgui_window_set_child (BOBGUI_WINDOW (window), vbox);

  da = bobgui_drawing_area_new ();
  bobgui_drawing_area_set_draw_func (BOBGUI_DRAWING_AREA (da), on_draw, NULL, NULL);
  bobgui_widget_set_vexpand (da, TRUE);
  bobgui_box_append (BOBGUI_BOX (vbox), da);

  label = bobgui_label_new ("Event processing time (ms):");
  bobgui_widget_set_halign (label, BOBGUI_ALIGN_CENTER);
  bobgui_box_append (BOBGUI_BOX (vbox), label);

  adjustment = bobgui_adjustment_new (20, 0, 200, 1, 10, 0);
  scale = bobgui_scale_new (BOBGUI_ORIENTATION_HORIZONTAL, adjustment);
  bobgui_box_append (BOBGUI_BOX (vbox), scale);

  controller = bobgui_event_controller_motion_new ();
  g_signal_connect (controller, "motion",
                    G_CALLBACK (motion_cb), da);
  bobgui_widget_add_controller (da, controller);

  g_signal_connect (window, "destroy",
                    G_CALLBACK (quit_cb), &done);

  bobgui_window_present (BOBGUI_WINDOW (window));
  while (!done)
    g_main_context_iteration (NULL, TRUE); 

  return 0;
}
