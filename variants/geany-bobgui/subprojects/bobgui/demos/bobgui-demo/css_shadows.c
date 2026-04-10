/* Theming/Shadows
 *
 * This demo shows how to use CSS shadows.
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

static BobguiWidget *
create_toolbar (void)
{
  BobguiWidget *toolbar;
  BobguiWidget *item;

  toolbar = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 6);
  bobgui_widget_set_valign (toolbar, BOBGUI_ALIGN_CENTER);

  item = bobgui_button_new_from_icon_name ("go-next");
  bobgui_box_append (BOBGUI_BOX (toolbar), item);

  item = bobgui_button_new_from_icon_name ("go-previous");
  bobgui_box_append (BOBGUI_BOX (toolbar), item);

  item = bobgui_button_new_with_label ("Hello World");
  bobgui_box_append (BOBGUI_BOX (toolbar), item);

  return toolbar;
}

BobguiWidget *
do_css_shadows (BobguiWidget *do_widget)
{
  static BobguiWidget *window = NULL;

  if (!window)
    {
      BobguiWidget *paned, *container, *child;
      BobguiStyleProvider *provider;
      BobguiTextBuffer *text;
      GBytes *bytes;

      window = bobgui_window_new ();
      bobgui_window_set_title (BOBGUI_WINDOW (window), "Shadows");
      bobgui_window_set_transient_for (BOBGUI_WINDOW (window), BOBGUI_WINDOW (do_widget));
      bobgui_window_set_default_size (BOBGUI_WINDOW (window), 400, 300);
      bobgui_widget_add_css_class (window, "demo");
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);

      paned = bobgui_paned_new (BOBGUI_ORIENTATION_VERTICAL);
      bobgui_window_set_child (BOBGUI_WINDOW (window), paned);

      child = create_toolbar ();
      bobgui_paned_set_start_child (BOBGUI_PANED (paned), child);
      bobgui_paned_set_resize_start_child (BOBGUI_PANED (paned), FALSE);

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

      container = bobgui_scrolled_window_new ();
      bobgui_paned_set_end_child (BOBGUI_PANED (paned), container);
      child = bobgui_text_view_new_with_buffer (text);
      bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (container), child);
      g_signal_connect (text, "changed",
                        G_CALLBACK (css_text_changed), provider);

      bytes = g_resources_lookup_data ("/css_shadows/bobgui.css", 0, NULL);
      bobgui_text_buffer_set_text (text, g_bytes_get_data (bytes, NULL), g_bytes_get_size (bytes));
      g_bytes_unref (bytes);

      g_signal_connect (provider,
                        "parsing-error",
                        G_CALLBACK (show_parsing_error),
                        bobgui_text_view_get_buffer (BOBGUI_TEXT_VIEW (child)));

      apply_css (window, provider);
    }

  if (!bobgui_widget_get_visible (window))
    bobgui_window_present (BOBGUI_WINDOW (window));
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));

  return window;
}
