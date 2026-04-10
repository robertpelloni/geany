/* Theming/Multiple Backgrounds
 *
 * BOBGUI themes are written using CSS. Every widget is build of multiple items
 * that you can style very similarly to a regular website.
 */

#include <bobgui/bobgui.h>

static void
show_parsing_error (BobguiCssProvider *provider,
                    BobguiCssSection  *section,
                    const GError   *error,
                    BobguiTextBuffer  *buffer)
{
  const BobguiCssLocation *start_location, *end_location;
  BobguiTextIter start, end;
  const char *tag_name;

  start_location = bobgui_css_section_get_start_location (section);
  bobgui_text_buffer_get_iter_at_line_index (buffer,
                                          &start,
                                          start_location->lines,
                                          start_location->line_bytes);
  end_location = bobgui_css_section_get_end_location (section);
  bobgui_text_buffer_get_iter_at_line_index (buffer,
                                          &end,
                                          end_location->lines,
                                          end_location->line_bytes);


  if (error->domain == BOBGUI_CSS_PARSER_WARNING)
    tag_name = "warning";
  else
    tag_name = "error";

  bobgui_text_buffer_apply_tag_by_name (buffer, tag_name, &start, &end);
}

static void
css_text_changed (BobguiTextBuffer  *buffer,
                  BobguiCssProvider *provider)
{
  BobguiTextIter start, end;
  char *text;

  bobgui_text_buffer_get_start_iter (buffer, &start);
  bobgui_text_buffer_get_end_iter (buffer, &end);
  bobgui_text_buffer_remove_all_tags (buffer, &start, &end);

  text = bobgui_text_buffer_get_text (buffer, &start, &end, FALSE);
  bobgui_css_provider_load_from_string (provider, text);
  g_free (text);
}

static void
clear_provider (gpointer data)
{
  BobguiStyleProvider *provider = data;

  bobgui_style_context_remove_provider_for_display (gdk_display_get_default (), provider);
}

static void
apply_css (BobguiWidget *widget, BobguiStyleProvider *provider)
{
  bobgui_style_context_add_provider_for_display (gdk_display_get_default (), provider, G_MAXUINT);
  g_object_set_data_full (G_OBJECT (widget), "provider", provider, clear_provider);
}

BobguiWidget *
do_css_multiplebgs (BobguiWidget *do_widget)
{
  static BobguiWidget *window = NULL;

  if (!window)
    {
      BobguiWidget *paned, *overlay, *child, *sw;
      BobguiStyleProvider *provider;
      BobguiTextBuffer *text;
      GBytes *bytes;

      window = bobgui_window_new ();
      bobgui_window_set_title (BOBGUI_WINDOW (window), "Multiple Backgrounds");
      bobgui_window_set_transient_for (BOBGUI_WINDOW (window), BOBGUI_WINDOW (do_widget));
      bobgui_window_set_default_size (BOBGUI_WINDOW (window), 400, 300);
      bobgui_widget_add_css_class (window, "demo");
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);

      overlay = bobgui_overlay_new ();
      bobgui_window_set_child (BOBGUI_WINDOW (window), overlay);

      child = bobgui_drawing_area_new ();
      /* Don't set a draw_func, since we are only interested in CSS drawing,
       * which happens automatically.
       */
      bobgui_widget_set_name (child, "canvas");
      bobgui_overlay_set_child (BOBGUI_OVERLAY (overlay), child);

      child = bobgui_button_new ();
      bobgui_overlay_add_overlay (BOBGUI_OVERLAY (overlay), child);
      bobgui_widget_set_name (child, "bricks-button");
      bobgui_widget_set_halign (child, BOBGUI_ALIGN_CENTER);
      bobgui_widget_set_valign (child, BOBGUI_ALIGN_CENTER);
      bobgui_widget_set_size_request (child, 250, 84);

      paned = bobgui_paned_new (BOBGUI_ORIENTATION_VERTICAL);
      bobgui_overlay_add_overlay (BOBGUI_OVERLAY (overlay), paned);

      /* Need a filler so we get a handle */
      child = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
      bobgui_paned_set_start_child (BOBGUI_PANED (paned), child);

      text = bobgui_text_buffer_new (NULL);
      bobgui_text_buffer_create_tag (text,
                                  "warning",
                                  "underline", PANGO_UNDERLINE_SINGLE,
                                  NULL);
      bobgui_text_buffer_create_tag (text,
                                  "error",
                                  "underline", PANGO_UNDERLINE_ERROR,
                                  NULL);

      provider = BOBGUI_STYLE_PROVIDER (bobgui_css_provider_new ());

      sw = bobgui_scrolled_window_new ();
      bobgui_paned_set_end_child (BOBGUI_PANED (paned), sw);
      child = bobgui_text_view_new_with_buffer (text);
      bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (sw), child);
      g_signal_connect (text,
                        "changed",
                        G_CALLBACK (css_text_changed),
                        provider);

      bytes = g_resources_lookup_data ("/css_multiplebgs/css_multiplebgs.css", 0, NULL);
      bobgui_text_buffer_set_text (text, g_bytes_get_data (bytes, NULL), g_bytes_get_size (bytes));
      g_bytes_unref (bytes);

      g_signal_connect (provider,
                        "parsing-error",
                        G_CALLBACK (show_parsing_error),
                        bobgui_text_view_get_buffer (BOBGUI_TEXT_VIEW (child)));

      apply_css (window, provider);
    }

  if (!bobgui_widget_get_visible (window))
    bobgui_widget_set_visible (window, TRUE);
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));

  return window;
}
