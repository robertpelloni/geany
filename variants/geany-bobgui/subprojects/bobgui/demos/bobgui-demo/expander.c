/* Expander
 *
 * BobguiExpander allows to provide additional content that is initially hidden.
 * This is also known as "disclosure triangle".
 *
 * This example also shows how to make the window resizable only if the expander
 * is expanded.
 */

#include <glib/gi18n.h>
#include <bobgui/bobgui.h>

static BobguiWidget *window = NULL;

static gboolean
close_request_cb (BobguiWidget *win, gpointer user_data)
{
  g_assert (window == win);
  bobgui_window_destroy ((BobguiWindow *)window);
  window = NULL;
  return TRUE;
}

static void
expander_cb (BobguiExpander *expander, GParamSpec *pspec, BobguiWindow *dialog)
{
  bobgui_window_set_resizable (dialog, bobgui_expander_get_expanded (expander));
}

BobguiWidget *
do_expander (BobguiWidget *do_widget)
{
  BobguiWidget *toplevel;
  BobguiWidget *area;
  BobguiWidget *expander;
  BobguiWidget *label;
  BobguiWidget *sw;
  BobguiWidget *tv;
  BobguiTextBuffer *buffer;
  BobguiTextIter start;
  BobguiTextIter end;
  BobguiTextTag *tag;
  GdkPaintable *paintable;

  if (!window)
    {
      toplevel = BOBGUI_WIDGET (bobgui_widget_get_root (do_widget));
      window = bobgui_window_new ();
      bobgui_window_set_title (BOBGUI_WINDOW (window), "Expander");
      bobgui_window_set_transient_for (BOBGUI_WINDOW (window), BOBGUI_WINDOW (toplevel));
      area = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 10);
      bobgui_widget_set_margin_start (area, 10);
      bobgui_widget_set_margin_end (area, 10);
      bobgui_widget_set_margin_top (area, 10);
      bobgui_widget_set_margin_bottom (area, 10);
      bobgui_window_set_child (BOBGUI_WINDOW (window), area);
      label = bobgui_label_new ("<big><b>Something went wrong</b></big>");
      bobgui_label_set_use_markup (BOBGUI_LABEL (label), TRUE);
      bobgui_box_append (BOBGUI_BOX (area), label);
      label = bobgui_label_new ("Here are some more details but not the full story");
      bobgui_label_set_wrap (BOBGUI_LABEL (label), FALSE);
      bobgui_widget_set_vexpand (label, FALSE);
      bobgui_box_append (BOBGUI_BOX (area), label);

      expander = bobgui_expander_new ("Details:");
      bobgui_widget_set_vexpand (expander, TRUE);
      sw = bobgui_scrolled_window_new ();
      bobgui_scrolled_window_set_min_content_height (BOBGUI_SCROLLED_WINDOW (sw), 100);
      bobgui_scrolled_window_set_has_frame (BOBGUI_SCROLLED_WINDOW (sw), TRUE);
      bobgui_scrolled_window_set_policy (BOBGUI_SCROLLED_WINDOW (sw),
                                      BOBGUI_POLICY_NEVER,
                                      BOBGUI_POLICY_AUTOMATIC);
      bobgui_scrolled_window_set_propagate_natural_height (BOBGUI_SCROLLED_WINDOW (sw), TRUE);
      bobgui_widget_set_vexpand (sw, TRUE);

      tv = bobgui_text_view_new ();

      g_object_set (tv,
                    "left-margin", 10,
                    "right-margin", 10,
                    "top-margin", 10,
                    "bottom-margin", 10,
                    NULL);

      buffer = bobgui_text_view_get_buffer (BOBGUI_TEXT_VIEW (tv));
      bobgui_text_view_set_editable (BOBGUI_TEXT_VIEW (tv), FALSE);
      bobgui_text_view_set_cursor_visible (BOBGUI_TEXT_VIEW (tv), FALSE);
      bobgui_text_view_set_wrap_mode (BOBGUI_TEXT_VIEW (tv), BOBGUI_WRAP_WORD);
      bobgui_text_view_set_pixels_above_lines (BOBGUI_TEXT_VIEW (tv), 2);
      bobgui_text_view_set_pixels_below_lines (BOBGUI_TEXT_VIEW (tv), 2);

      bobgui_text_buffer_set_text (buffer,
                                "Finally, the full story with all details. "
                                "And all the inside information, including "
                                "error codes, etc etc. Pages of information, "
                                "you might have to scroll down to read it all, "
                                "or even resize the window - it works !\n"
                                "A second paragraph will contain even more "
                                "innuendo, just to make you scroll down or "
                                "resize the window.\n"
                                "Do it already!\n", -1);

      bobgui_text_buffer_get_end_iter (buffer, &start);
      paintable = GDK_PAINTABLE (gdk_texture_new_from_resource ("/cursors/images/bobgui_logo_cursor.png"));
      bobgui_text_buffer_insert_paintable (buffer, &start, paintable);
      g_object_unref (paintable);
      bobgui_text_iter_backward_char (&start);

      bobgui_text_buffer_get_end_iter (buffer, &end);
      tag = bobgui_text_buffer_create_tag (buffer, NULL,
                                        "pixels-above-lines", 200,
                                        "justification", BOBGUI_JUSTIFY_RIGHT,
                                        NULL);
      bobgui_text_buffer_apply_tag (buffer, tag, &start, &end);

      bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (sw), tv);
      bobgui_expander_set_child (BOBGUI_EXPANDER (expander), sw);
      bobgui_box_append (BOBGUI_BOX (area), expander);
      g_signal_connect (expander, "notify::expanded",
                        G_CALLBACK (expander_cb), window);

      g_signal_connect (window, "close-request", G_CALLBACK (close_request_cb), NULL);
  }

  if (!bobgui_widget_get_visible (window))
    bobgui_window_present (BOBGUI_WINDOW (window));
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));

  return window;
}
