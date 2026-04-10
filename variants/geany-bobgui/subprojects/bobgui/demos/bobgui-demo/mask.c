/* Masking
 *
 * Demonstrates mask nodes.
 *
 * This demo uses a text node as mask for
 * an animated linear gradient.
 */

#include <bobgui/bobgui.h>
#include "demo4widget.h"


BobguiWidget *
do_mask (BobguiWidget *do_widget)
{
  static BobguiWidget *window = NULL;

  if (!window)
    {
      BobguiWidget *box;
      BobguiWidget *demo;
      BobguiWidget *scale;

      window = bobgui_window_new ();
      bobgui_window_set_title (BOBGUI_WINDOW (window), "Mask Nodes");
      bobgui_window_set_default_size (BOBGUI_WINDOW (window), 600, 400);
      bobgui_window_set_display (BOBGUI_WINDOW (window),
                              bobgui_widget_get_display (do_widget));
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);

      box = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
      bobgui_window_set_child (BOBGUI_WINDOW (window), box);

      demo = demo4_widget_new ();
      bobgui_widget_set_hexpand (demo, TRUE);
      bobgui_widget_set_vexpand (demo, TRUE);

      bobgui_box_append (BOBGUI_BOX (box), demo);

      scale = bobgui_scale_new_with_range (BOBGUI_ORIENTATION_HORIZONTAL, 0, 1, 0.1);
      bobgui_range_set_value (BOBGUI_RANGE (scale), 0.5);
      g_object_bind_property (bobgui_range_get_adjustment (BOBGUI_RANGE (scale)), "value", demo, "progress", 0);

      bobgui_box_append (BOBGUI_BOX (box), scale);
    }

  if (!bobgui_widget_get_visible (window))
    bobgui_widget_set_visible (window, TRUE);
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));

  return window;
}
