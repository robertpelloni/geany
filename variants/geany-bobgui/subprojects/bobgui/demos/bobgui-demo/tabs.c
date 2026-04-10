/* Text View/Tabs
 *
 * BobguiTextView can position text at fixed positions, using tabs.
 * Tabs can specify alignment, and also allow aligning numbers
 * on the decimal point.
 *
 * The example here has three tabs, with left, numeric and right
 * alignment.
 */

#include <bobgui/bobgui.h>
#include <gdk/gdkkeysyms.h>

BobguiWidget *
do_tabs (BobguiWidget *do_widget)
{
  static BobguiWidget *window = NULL;

  if (!window)
    {
      BobguiWidget *view;
      BobguiWidget *sw;
      BobguiTextBuffer *buffer;
      PangoTabArray *tabs;

      window = bobgui_window_new ();
      bobgui_window_set_title (BOBGUI_WINDOW (window), "Tabs");
      bobgui_window_set_display (BOBGUI_WINDOW (window),
                              bobgui_widget_get_display (do_widget));
      bobgui_window_set_default_size (BOBGUI_WINDOW (window), 330, 130);
      bobgui_window_set_resizable (BOBGUI_WINDOW (window), FALSE);
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);

      view = bobgui_text_view_new ();
      bobgui_text_view_set_wrap_mode (BOBGUI_TEXT_VIEW (view), BOBGUI_WRAP_WORD);
      bobgui_text_view_set_top_margin (BOBGUI_TEXT_VIEW (view), 20);
      bobgui_text_view_set_bottom_margin (BOBGUI_TEXT_VIEW (view), 20);
      bobgui_text_view_set_left_margin (BOBGUI_TEXT_VIEW (view), 20);
      bobgui_text_view_set_right_margin (BOBGUI_TEXT_VIEW (view), 20);

      tabs = pango_tab_array_new (3, TRUE);
      pango_tab_array_set_tab (tabs, 0, PANGO_TAB_LEFT, 0);
      pango_tab_array_set_tab (tabs, 1, PANGO_TAB_DECIMAL, 150);
      pango_tab_array_set_decimal_point (tabs, 1, '.');
      pango_tab_array_set_tab (tabs, 2, PANGO_TAB_RIGHT, 290);
      bobgui_text_view_set_tabs (BOBGUI_TEXT_VIEW (view), tabs);
      pango_tab_array_free (tabs);

      buffer = bobgui_text_view_get_buffer (BOBGUI_TEXT_VIEW (view));
      bobgui_text_buffer_set_text (buffer, "one\t2.0\tthree\nfour\t5.555\tsix\nseven\t88.88\tnine", -1);

      sw = bobgui_scrolled_window_new ();
      bobgui_scrolled_window_set_policy (BOBGUI_SCROLLED_WINDOW (sw),
                                      BOBGUI_POLICY_NEVER,
                                      BOBGUI_POLICY_AUTOMATIC);
      bobgui_window_set_child (BOBGUI_WINDOW (window), sw);
      bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (sw), view);
    }

  if (!bobgui_widget_get_visible (window))
    bobgui_widget_set_visible (window, TRUE);
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));

  return window;
}
