/* Text View/Markup
 * #Keywords: BobguiTextView
 *
 * BobguiTextBuffer lets you define your own tags that can influence
 * text formatting in a variety of ways. In this example, we show
 * that BobguiTextBuffer can load Pango markup and automatically generate
 * suitable tags.
 */

#include <bobgui/bobgui.h>

static BobguiWidget *stack;
static BobguiWidget *view;
static BobguiWidget *view2;

static void
source_toggled (BobguiCheckButton *button)
{
  if (bobgui_check_button_get_active (button))
    bobgui_stack_set_visible_child_name (BOBGUI_STACK (stack), "source");
  else
    {
      BobguiTextBuffer *buffer;
      BobguiTextIter start, end;
      char *markup;

      buffer = bobgui_text_view_get_buffer (BOBGUI_TEXT_VIEW (view2));
      bobgui_text_buffer_get_bounds (buffer, &start, &end);
      markup = bobgui_text_buffer_get_text (buffer, &start, &end, FALSE);

      buffer = bobgui_text_view_get_buffer (BOBGUI_TEXT_VIEW (view));
      bobgui_text_buffer_get_bounds (buffer, &start, &end);
      bobgui_text_buffer_begin_irreversible_action (buffer);
      bobgui_text_buffer_delete (buffer, &start, &end);
      bobgui_text_buffer_insert_markup (buffer, &start, markup, -1);
      bobgui_text_buffer_end_irreversible_action (buffer);
      g_free (markup);

      bobgui_stack_set_visible_child_name (BOBGUI_STACK (stack), "formatted");
    }
}

BobguiWidget *
do_markup (BobguiWidget *do_widget)
{
  static BobguiWidget *window = NULL;

  if (!window)
    {
      BobguiWidget *sw;
      BobguiTextBuffer *buffer;
      BobguiTextIter iter;
      GBytes *bytes;
      const char *markup;
      BobguiWidget *header;
      BobguiWidget *show_source;

      window = bobgui_window_new ();
      bobgui_window_set_display (BOBGUI_WINDOW (window),
                              bobgui_widget_get_display (do_widget));
      bobgui_window_set_default_size (BOBGUI_WINDOW (window), 600, 680);
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);

      stack = bobgui_stack_new ();
      bobgui_window_set_child (BOBGUI_WINDOW (window), stack);

      show_source = bobgui_check_button_new_with_label ("Source");
      bobgui_widget_set_valign (show_source, BOBGUI_ALIGN_CENTER);
      g_signal_connect (show_source, "toggled", G_CALLBACK (source_toggled), stack);

      header = bobgui_header_bar_new ();
      bobgui_header_bar_pack_start (BOBGUI_HEADER_BAR (header), show_source);
      bobgui_window_set_titlebar (BOBGUI_WINDOW (window), header);

      bobgui_window_set_title (BOBGUI_WINDOW (window), "Markup");

      view = bobgui_text_view_new ();
      bobgui_text_view_set_editable (BOBGUI_TEXT_VIEW (view), FALSE);
      bobgui_text_view_set_wrap_mode (BOBGUI_TEXT_VIEW (view), BOBGUI_WRAP_WORD_CHAR);
      bobgui_text_view_set_left_margin (BOBGUI_TEXT_VIEW (view), 10);
      bobgui_text_view_set_right_margin (BOBGUI_TEXT_VIEW (view), 10);

      sw = bobgui_scrolled_window_new ();
      bobgui_scrolled_window_set_policy (BOBGUI_SCROLLED_WINDOW (sw),
                                      BOBGUI_POLICY_AUTOMATIC,
                                      BOBGUI_POLICY_AUTOMATIC);
      bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (sw), view);

      bobgui_stack_add_named (BOBGUI_STACK (stack), sw, "formatted");

      view2 = bobgui_text_view_new ();
      bobgui_text_view_set_wrap_mode (BOBGUI_TEXT_VIEW (view2), BOBGUI_WRAP_WORD);
      bobgui_text_view_set_left_margin (BOBGUI_TEXT_VIEW (view2), 10);
      bobgui_text_view_set_right_margin (BOBGUI_TEXT_VIEW (view2), 10);

      sw = bobgui_scrolled_window_new ();
      bobgui_scrolled_window_set_policy (BOBGUI_SCROLLED_WINDOW (sw),
                                      BOBGUI_POLICY_AUTOMATIC,
                                      BOBGUI_POLICY_AUTOMATIC);
      bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (sw), view2);

      bobgui_stack_add_named (BOBGUI_STACK (stack), sw, "source");

      bytes = g_resources_lookup_data ("/markup/markup.txt", 0, NULL);
      markup = (const char *)g_bytes_get_data (bytes, NULL);

      buffer = bobgui_text_view_get_buffer (BOBGUI_TEXT_VIEW (view));
      bobgui_text_buffer_get_start_iter (buffer, &iter);
      bobgui_text_buffer_begin_irreversible_action (buffer);
      bobgui_text_buffer_insert_markup (buffer, &iter, markup, -1);
      bobgui_text_buffer_end_irreversible_action (buffer);

      buffer = bobgui_text_view_get_buffer (BOBGUI_TEXT_VIEW (view2));
      bobgui_text_buffer_get_start_iter (buffer, &iter);
      bobgui_text_buffer_begin_irreversible_action (buffer);
      bobgui_text_buffer_insert (buffer, &iter, markup, -1);
      bobgui_text_buffer_end_irreversible_action (buffer);

      g_bytes_unref (bytes);
    }

  if (!bobgui_widget_get_visible (window))
    bobgui_widget_set_visible (window, TRUE);
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));

  return window;
}
