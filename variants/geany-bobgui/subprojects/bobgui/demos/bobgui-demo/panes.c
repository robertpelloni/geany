/* Paned Widgets
 *
 * The BobguiPaned Widget divides its content area into two panes
 * with a divider in between that the user can adjust. A separate
 * child is placed into each pane. BobguiPaned widgets can be split
 * horizontally or vertically. This test contains both a horizontal
 * and a vertical BobguiPaned widget.
 *
 * There are a number of options that can be set for each pane.
 * You can use the Inspector to adjust the options for each side
 * of each widget.
 */

#include <bobgui/bobgui.h>

BobguiWidget *
do_panes (BobguiWidget *do_widget)
{
  static BobguiWidget *window = NULL;
  BobguiWidget *frame;
  BobguiWidget *hpaned;
  BobguiWidget *vpaned;
  BobguiWidget *label;
  BobguiWidget *vbox;

  if (!window)
    {
      window = bobgui_window_new ();
      bobgui_window_set_title (BOBGUI_WINDOW (window), "Paned Widgets");
      bobgui_window_set_default_size (BOBGUI_WINDOW (window), 330, 250);
      bobgui_window_set_resizable (BOBGUI_WINDOW (window), FALSE);
      bobgui_window_set_display (BOBGUI_WINDOW (window),
                              bobgui_widget_get_display (do_widget));
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);

      vbox = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 8);
      bobgui_widget_set_margin_start (vbox, 8);
      bobgui_widget_set_margin_end (vbox, 8);
      bobgui_widget_set_margin_top (vbox, 8);
      bobgui_widget_set_margin_bottom (vbox, 8);
      bobgui_window_set_child (BOBGUI_WINDOW (window), vbox);

      frame = bobgui_frame_new (NULL);
      bobgui_box_append (BOBGUI_BOX (vbox), frame);

      vpaned = bobgui_paned_new (BOBGUI_ORIENTATION_VERTICAL);
      bobgui_frame_set_child (BOBGUI_FRAME (frame), vpaned);

      hpaned = bobgui_paned_new (BOBGUI_ORIENTATION_HORIZONTAL);
      bobgui_paned_set_start_child (BOBGUI_PANED (vpaned), hpaned);
      bobgui_paned_set_shrink_start_child (BOBGUI_PANED (vpaned), FALSE);

      label = bobgui_label_new ("Hi there");
      bobgui_widget_set_margin_start (label, 4);
      bobgui_widget_set_margin_end (label, 4);
      bobgui_widget_set_margin_top (label, 4);
      bobgui_widget_set_margin_bottom (label, 4);
      bobgui_widget_set_hexpand (label, TRUE);
      bobgui_widget_set_vexpand (label, TRUE);
      bobgui_paned_set_start_child (BOBGUI_PANED (hpaned), label);
      bobgui_paned_set_shrink_start_child (BOBGUI_PANED (hpaned), FALSE);

      label = bobgui_label_new ("Hello");
      bobgui_widget_set_margin_start (label, 4);
      bobgui_widget_set_margin_end (label, 4);
      bobgui_widget_set_margin_top (label, 4);
      bobgui_widget_set_margin_bottom (label, 4);
      bobgui_widget_set_hexpand (label, TRUE);
      bobgui_widget_set_vexpand (label, TRUE);
      bobgui_paned_set_end_child (BOBGUI_PANED (hpaned), label);
      bobgui_paned_set_shrink_end_child (BOBGUI_PANED (hpaned), FALSE);

      label = bobgui_label_new ("Goodbye");
      bobgui_widget_set_margin_start (label, 4);
      bobgui_widget_set_margin_end (label, 4);
      bobgui_widget_set_margin_top (label, 4);
      bobgui_widget_set_margin_bottom (label, 4);
      bobgui_widget_set_hexpand (label, TRUE);
      bobgui_widget_set_vexpand (label, TRUE);
      bobgui_paned_set_end_child (BOBGUI_PANED (vpaned), label);
      bobgui_paned_set_shrink_end_child (BOBGUI_PANED (vpaned), FALSE);
    }

  if (!bobgui_widget_get_visible (window))
    bobgui_widget_set_visible (window, TRUE);
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));

  return window;
}
