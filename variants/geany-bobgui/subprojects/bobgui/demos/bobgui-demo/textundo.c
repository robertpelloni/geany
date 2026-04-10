/* Text View/Undo and Redo
 *
 * The BobguiTextView supports undo and redo through the use of a
 * BobguiTextBuffer. You can enable or disable undo support using
 * bobgui_text_buffer_set_enable_undo().
 *
 * Use Control+z to undo and Control+Shift+z or Control+y to
 * redo previously undone operations.
 */

#include <bobgui/bobgui.h>
#include <stdlib.h> /* for exit() */

BobguiWidget *
do_textundo (BobguiWidget *do_widget)
{
  static BobguiWidget *window = NULL;

  if (!window)
    {
      BobguiWidget *view;
      BobguiWidget *sw;
      BobguiTextBuffer *buffer;
      BobguiTextIter iter;

      window = bobgui_window_new ();
      bobgui_window_set_display (BOBGUI_WINDOW (window),
                              bobgui_widget_get_display (do_widget));
      bobgui_window_set_default_size (BOBGUI_WINDOW (window), 330, 330);
      bobgui_window_set_resizable (BOBGUI_WINDOW (window), FALSE);
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);

      bobgui_window_set_title (BOBGUI_WINDOW (window), "Undo and Redo");

      view = bobgui_text_view_new ();
      bobgui_text_view_set_wrap_mode (BOBGUI_TEXT_VIEW (view), BOBGUI_WRAP_WORD);
      bobgui_text_view_set_pixels_below_lines (BOBGUI_TEXT_VIEW (view), 10);
      bobgui_text_view_set_left_margin (BOBGUI_TEXT_VIEW (view), 20);
      bobgui_text_view_set_right_margin (BOBGUI_TEXT_VIEW (view), 20);
      bobgui_text_view_set_top_margin (BOBGUI_TEXT_VIEW (view), 20);
      bobgui_text_view_set_bottom_margin (BOBGUI_TEXT_VIEW (view), 20);

      buffer = bobgui_text_view_get_buffer (BOBGUI_TEXT_VIEW (view));
      bobgui_text_buffer_set_enable_undo (buffer, TRUE);

      /* this text cannot be undone */
      bobgui_text_buffer_begin_irreversible_action (buffer);
      bobgui_text_buffer_get_start_iter (buffer, &iter);
      bobgui_text_buffer_insert (buffer, &iter,
          "The BobguiTextView supports undo and redo through the use of a "
          "BobguiTextBuffer. You can enable or disable undo support using "
          "bobgui_text_buffer_set_enable_undo().\n"
          "Type to add more text.\n"
          "Use Control+z to undo and Control+Shift+z or Control+y to "
          "redo previously undone operations.",
          -1);
      bobgui_text_buffer_end_irreversible_action (buffer);

      sw = bobgui_scrolled_window_new ();
      bobgui_scrolled_window_set_policy (BOBGUI_SCROLLED_WINDOW (sw),
                                      BOBGUI_POLICY_AUTOMATIC,
                                      BOBGUI_POLICY_AUTOMATIC);
      bobgui_window_set_child (BOBGUI_WINDOW (window), sw);
      bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (sw), view);
    }

  if (!bobgui_widget_get_visible (window))
    {
      bobgui_widget_set_visible (window, TRUE);
    }
  else
    {
      bobgui_window_destroy (BOBGUI_WINDOW (window));
      window = NULL;
    }

  return window;
}
