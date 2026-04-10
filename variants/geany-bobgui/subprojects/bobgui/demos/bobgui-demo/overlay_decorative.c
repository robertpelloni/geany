/* Overlay/Decorative Overlay
 * #Keywords: BobguiOverlay
 *
 * Another example of an overlay with some decorative
 * and some interactive controls.
 */

#include <bobgui/bobgui.h>

static BobguiTextTag *tag;

static void
margin_changed (BobguiAdjustment *adjustment,
                BobguiTextView   *text)
{
  int value;

  value = (int)bobgui_adjustment_get_value (adjustment);
  bobgui_text_view_set_left_margin (BOBGUI_TEXT_VIEW (text), value);
  g_object_set (tag, "pixels-above-lines", value, NULL);
}

BobguiWidget *
do_overlay_decorative (BobguiWidget *do_widget)
{
  static BobguiWidget *window = NULL;

  if (!window)
    {
      BobguiWidget *overlay;
      BobguiWidget *sw;
      BobguiWidget *text;
      BobguiWidget *image;
      BobguiWidget *scale;
      BobguiTextBuffer *buffer;
      BobguiTextIter start, end;
      BobguiAdjustment *adjustment;

      window = bobgui_window_new ();
      bobgui_window_set_default_size (BOBGUI_WINDOW (window), 500, 510);
      bobgui_window_set_title (BOBGUI_WINDOW (window), "Decorative Overlay");

      overlay = bobgui_overlay_new ();
      sw = bobgui_scrolled_window_new ();
      bobgui_scrolled_window_set_policy (BOBGUI_SCROLLED_WINDOW (sw),
                                      BOBGUI_POLICY_AUTOMATIC,
                                      BOBGUI_POLICY_AUTOMATIC);
      text = bobgui_text_view_new ();
      buffer = bobgui_text_view_get_buffer (BOBGUI_TEXT_VIEW (text));

      bobgui_text_buffer_set_text (buffer, "Dear diary...", -1);

      tag = bobgui_text_buffer_create_tag (buffer, "top-margin",
                                        "pixels-above-lines", 0,
                                        NULL);
      bobgui_text_buffer_get_start_iter (buffer, &start);
      end = start;
      bobgui_text_iter_forward_word_end (&end);
      bobgui_text_buffer_apply_tag (buffer, tag, &start, &end);

      bobgui_window_set_child (BOBGUI_WINDOW (window), overlay);
      bobgui_overlay_set_child (BOBGUI_OVERLAY (overlay), sw);
      bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (sw), text);
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);

      image = bobgui_picture_new_for_resource ("/overlay2/decor1.png");
      bobgui_overlay_add_overlay (BOBGUI_OVERLAY (overlay), image);
      bobgui_widget_set_can_target (image, FALSE);
      bobgui_widget_set_halign (image, BOBGUI_ALIGN_START);
      bobgui_widget_set_valign (image, BOBGUI_ALIGN_START);

      image = bobgui_picture_new_for_resource ("/overlay2/decor2.png");
      bobgui_overlay_add_overlay (BOBGUI_OVERLAY (overlay), image);
      bobgui_widget_set_can_target (image, FALSE);
      bobgui_widget_set_halign (image, BOBGUI_ALIGN_END);
      bobgui_widget_set_valign (image, BOBGUI_ALIGN_END);

      adjustment = bobgui_adjustment_new (0, 0, 100, 1, 1, 0);
      g_signal_connect (adjustment, "value-changed",
                        G_CALLBACK (margin_changed), text);

      scale = bobgui_scale_new (BOBGUI_ORIENTATION_HORIZONTAL, adjustment);
      bobgui_scale_set_draw_value (BOBGUI_SCALE (scale), FALSE);
      bobgui_widget_set_size_request (scale, 120, -1);
      bobgui_widget_set_margin_start (scale, 20);
      bobgui_widget_set_margin_end (scale, 20);
      bobgui_widget_set_margin_bottom (scale, 20);
      bobgui_overlay_add_overlay (BOBGUI_OVERLAY (overlay), scale);
      bobgui_widget_set_halign (scale, BOBGUI_ALIGN_START);
      bobgui_widget_set_valign (scale, BOBGUI_ALIGN_END);
      bobgui_widget_set_tooltip_text (scale, "Margin");

      bobgui_adjustment_set_value (adjustment, 100);
    }

  if (!bobgui_widget_get_visible (window))
    bobgui_widget_set_visible (window, TRUE);
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));

  return window;
}
