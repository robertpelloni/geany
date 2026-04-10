/* OpenGL/Gears
 *
 * This is a classic OpenGL demo, running in a BobguiGLArea.
 */


#include <stdlib.h>
#include <bobgui/bobgui.h>

#include "bobguigears.h"

/************************************************************************
 *                 DEMO CODE                                            *
 ************************************************************************/

static void
on_axis_value_change (BobguiAdjustment *adjustment,
                      gpointer       data)
{
  BobguiGears *gears = BOBGUI_GEARS (data);
  int axis = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (adjustment), "axis"));

  bobgui_gears_set_axis (gears, axis, bobgui_adjustment_get_value (adjustment));
}


static BobguiWidget *
create_axis_slider (BobguiGears *gears,
                    int axis)
{
  BobguiWidget *box, *label, *slider;
  BobguiAdjustment *adj;
  const char *text;

  box = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, FALSE);

  switch (axis)
    {
    case BOBGUI_GEARS_X_AXIS:
      text = "X";
      break;

    case BOBGUI_GEARS_Y_AXIS:
      text = "Y";
      break;

    case BOBGUI_GEARS_Z_AXIS:
      text = "Z";
      break;

    default:
      g_assert_not_reached ();
    }

  label = bobgui_label_new (text);
  bobgui_box_append (BOBGUI_BOX (box), label);

  adj = bobgui_adjustment_new (bobgui_gears_get_axis (gears, axis), 0.0, 360.0, 1.0, 12.0, 0.0);
  g_object_set_data (G_OBJECT (adj), "axis", GINT_TO_POINTER (axis));
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (on_axis_value_change),
                    gears);
  slider = bobgui_scale_new (BOBGUI_ORIENTATION_VERTICAL, adj);
  bobgui_scale_set_draw_value (BOBGUI_SCALE (slider), FALSE);
  bobgui_box_append (BOBGUI_BOX (box), slider);
  bobgui_widget_set_vexpand (slider, TRUE);

  return box;
}

BobguiWidget *
do_gears (BobguiWidget *do_widget)
{
  static BobguiWidget *window = NULL;
  BobguiWidget *box, *hbox, *fps_label, *gears, *overlay, *frame;
  int i;

  if (!window)
    {
      window = bobgui_window_new ();
      bobgui_window_set_title (BOBGUI_WINDOW (window), "Gears");
      bobgui_window_set_resizable (BOBGUI_WINDOW (window), TRUE);
      bobgui_window_set_default_size (BOBGUI_WINDOW (window), 640, 640);
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);

      overlay = bobgui_overlay_new ();
      bobgui_widget_set_margin_start (overlay, 12);
      bobgui_widget_set_margin_end (overlay, 12);
      bobgui_widget_set_margin_top (overlay, 12);
      bobgui_widget_set_margin_bottom (overlay, 12);

      bobgui_window_set_child (BOBGUI_WINDOW (window), overlay);

      frame = bobgui_frame_new (NULL);
      bobgui_widget_set_halign (frame, BOBGUI_ALIGN_START);
      bobgui_widget_set_valign (frame, BOBGUI_ALIGN_START);
      bobgui_widget_add_css_class (frame, "app-notification");
      bobgui_overlay_add_overlay (BOBGUI_OVERLAY (overlay), frame);

      fps_label = bobgui_label_new ("");
      bobgui_widget_set_halign (fps_label, BOBGUI_ALIGN_START);
      bobgui_frame_set_child (BOBGUI_FRAME (frame), fps_label);

      box = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, FALSE);
      bobgui_box_set_spacing (BOBGUI_BOX (box), 6);
      bobgui_overlay_set_child (BOBGUI_OVERLAY (overlay), box);

      hbox = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, FALSE);
      bobgui_box_set_spacing (BOBGUI_BOX (box), 6);
      bobgui_box_append (BOBGUI_BOX (box), hbox);

      gears = bobgui_gears_new ();
      bobgui_widget_set_hexpand (gears, TRUE);
      bobgui_widget_set_vexpand (gears, TRUE);
      bobgui_box_append (BOBGUI_BOX (hbox), gears);

      for (i = 0; i < BOBGUI_GEARS_N_AXIS; i++)
        bobgui_box_append (BOBGUI_BOX (hbox), create_axis_slider (BOBGUI_GEARS (gears), i));

      hbox = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, FALSE);
      bobgui_box_set_spacing (BOBGUI_BOX (hbox), 6);
      bobgui_box_append (BOBGUI_BOX (box), hbox);

      bobgui_gears_set_fps_label (BOBGUI_GEARS (gears), BOBGUI_LABEL (fps_label));
    }

  if (!bobgui_widget_get_visible (window))
    bobgui_widget_set_visible (window, TRUE);
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));

  return window;
}
