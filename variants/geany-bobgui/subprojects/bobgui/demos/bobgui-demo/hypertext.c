/* Text View/Hypertext
 * #Keywords: BobguiTextView, BobguiTextBuffer
 *
 * Usually, tags modify the appearance of text in the view, e.g. making it
 * bold or colored or underlined. But tags are not restricted to appearance.
 * They can also affect the behavior of mouse and key presses, as this demo
 * shows.
 *
 * We also demonstrate adding other things to a text view, such as
 * clickable icons and widgets which can also replace a character
 * (try copying the ghost text).
 */

#include <bobgui/bobgui.h>
#include <gdk/gdkkeysyms.h>

/* Inserts a piece of text into the buffer, giving it the usual
 * appearance of a hyperlink in a web browser: blue and underlined.
 * Additionally, attaches some data on the tag, to make it recognizable
 * as a link.
 */
static void
insert_link (BobguiTextBuffer *buffer,
             BobguiTextIter   *iter,
             const char    *text,
             int            page)
{
  BobguiTextTag *tag;

  tag = bobgui_text_buffer_create_tag (buffer, NULL,
                                    "foreground", "blue",
                                    "underline", PANGO_UNDERLINE_SINGLE,
                                    NULL);
  g_object_set_data (G_OBJECT (tag), "page", GINT_TO_POINTER (page));
  bobgui_text_buffer_insert_with_tags (buffer, iter, text, -1, tag, NULL);
}

/* Quick-and-dirty text-to-speech for a single word. If you don't hear
 * anything, you are missing espeak-ng on your system.
 */
static void
say_word (BobguiGestureClick *gesture,
          guint            n_press,
          double           x,
          double           y,
          const char      *word)
{
  const char *argv[3];

  argv[0] = "espeak-ng";
  argv[1] = word;
  argv[2] = NULL;

  g_spawn_async (NULL, (char **)argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, NULL);
}

/* Fills the buffer with text and interspersed links. In any real
 * hypertext app, this method would parse a file to identify the links.
 */
static void
show_page (BobguiTextView *text_view,
           int          page)
{
  BobguiTextBuffer *buffer;
  BobguiTextIter iter, start;
  BobguiTextMark *mark;
  BobguiWidget *child;
  BobguiTextChildAnchor *anchor;
  BobguiEventController *controller;
  BobguiTextTag *bold, *mono, *nobreaks;

  buffer = bobgui_text_view_get_buffer (text_view);

  bold = bobgui_text_buffer_create_tag (buffer, NULL,
                                     "weight", PANGO_WEIGHT_BOLD,
                                     "scale", PANGO_SCALE_X_LARGE,
                                     NULL);
  mono = bobgui_text_buffer_create_tag (buffer, NULL,
                                     "family", "monospace",
                                     NULL);
  nobreaks = bobgui_text_buffer_create_tag (buffer, NULL,
                                         "allow-breaks", FALSE,
                                         NULL);

  bobgui_text_buffer_set_text (buffer, "", 0);
  bobgui_text_buffer_get_iter_at_offset (buffer, &iter, 0);
  bobgui_text_buffer_begin_irreversible_action (buffer);
  if (page == 1)
    {
      BobguiIconPaintable *icon;
      BobguiIconTheme *theme;

      bobgui_text_buffer_insert (buffer, &iter, "Some text to show that simple ", -1);
      insert_link (buffer, &iter, "hypertext", 3);
      bobgui_text_buffer_insert (buffer, &iter, " can easily be realized with ", -1);
      insert_link (buffer, &iter, "tags", 2);
      bobgui_text_buffer_insert (buffer, &iter, ".\n", -1);
      bobgui_text_buffer_insert (buffer, &iter, "Of course you can also embed Emoji 😋, ", -1);
      bobgui_text_buffer_insert (buffer, &iter, "icons ", -1);

      theme = bobgui_icon_theme_get_for_display (bobgui_widget_get_display (BOBGUI_WIDGET (text_view)));
      icon = bobgui_icon_theme_lookup_icon (theme,
                                         "view-conceal-symbolic",
                                         NULL,
                                         16,
                                         1,
                                         BOBGUI_TEXT_DIR_LTR,
                                         0);
      bobgui_text_buffer_insert_paintable (buffer, &iter, GDK_PAINTABLE (icon));
      g_object_unref (icon);
      bobgui_text_buffer_insert (buffer, &iter, ", or even widgets ", -1);
      anchor = bobgui_text_buffer_create_child_anchor (buffer, &iter);
      child = bobgui_level_bar_new_for_interval (0, 100);
      bobgui_level_bar_set_value (BOBGUI_LEVEL_BAR (child), 50);
      bobgui_widget_set_size_request (child, 100, -1);
      bobgui_text_view_add_child_at_anchor (text_view, child, anchor);
      bobgui_text_buffer_insert (buffer, &iter, " and labels with ", -1);
      anchor = bobgui_text_child_anchor_new_with_replacement ("👻");
      bobgui_text_buffer_insert_child_anchor (buffer, &iter, anchor);
      child = bobgui_label_new ("ghost");
      bobgui_text_view_add_child_at_anchor (text_view, child, anchor);
      bobgui_text_buffer_insert (buffer, &iter, " text.", -1);
    }
  else if (page == 2)
    {
      mark = bobgui_text_buffer_create_mark (buffer, "mark", &iter, TRUE);

      bobgui_text_buffer_insert_with_tags (buffer, &iter, "tag", -1, bold, NULL);
      bobgui_text_buffer_insert (buffer, &iter, " /", -1);

      bobgui_text_buffer_get_iter_at_mark (buffer, &start, mark);
      bobgui_text_buffer_apply_tag (buffer, nobreaks, &start, &iter);
      bobgui_text_buffer_insert (buffer, &iter, " ", -1);

      bobgui_text_buffer_move_mark (buffer, mark, &iter);
      bobgui_text_buffer_insert_with_tags (buffer, &iter, "tag", -1, mono, NULL);
      bobgui_text_buffer_insert (buffer, &iter, " /", -1);

      bobgui_text_buffer_get_iter_at_mark (buffer, &start, mark);
      bobgui_text_buffer_apply_tag (buffer, nobreaks, &start, &iter);
      bobgui_text_buffer_insert (buffer, &iter, " ", -1);

      anchor = bobgui_text_buffer_create_child_anchor (buffer, &iter);
      child = bobgui_image_new_from_icon_name ("audio-volume-high-symbolic");
      bobgui_widget_set_cursor_from_name (child, "pointer");
      controller = BOBGUI_EVENT_CONTROLLER (bobgui_gesture_click_new ());
      g_signal_connect (controller, "pressed", G_CALLBACK (say_word), (gpointer)"tag");
      bobgui_widget_add_controller (child, controller);
      bobgui_text_view_add_child_at_anchor (text_view, child, anchor);

      bobgui_text_buffer_insert (buffer, &iter, "\n"
          "An attribute that can be applied to some range of text. For example, "
          "a tag might be called “bold” and make the text inside the tag bold.\n"
          "However, the tag concept is more general than that; "
          "tags don't have to affect appearance. They can instead affect the "
          "behavior of mouse and key presses, “lock” a range of text so the "
          "user can't edit it, or countless other things.\n", -1);
      insert_link (buffer, &iter, "Go back", 1);

      bobgui_text_buffer_delete_mark (buffer, mark);
    }
  else if (page == 3)
    {
      mark = bobgui_text_buffer_create_mark (buffer, "mark", &iter, TRUE);

      bobgui_text_buffer_insert_with_tags (buffer, &iter, "hypertext", -1, bold, NULL);
      bobgui_text_buffer_insert (buffer, &iter, " /", -1);

      bobgui_text_buffer_get_iter_at_mark (buffer, &start, mark);
      bobgui_text_buffer_apply_tag (buffer, nobreaks, &start, &iter);
      bobgui_text_buffer_insert (buffer, &iter, " ", -1);

      bobgui_text_buffer_move_mark (buffer, mark, &iter);
      bobgui_text_buffer_insert_with_tags (buffer, &iter, "ˈhaɪ pərˌtɛkst", -1, mono, NULL);
      bobgui_text_buffer_insert (buffer, &iter, " /", -1);
      bobgui_text_buffer_get_iter_at_mark (buffer, &start, mark);
      bobgui_text_buffer_apply_tag (buffer, nobreaks, &start, &iter);
      bobgui_text_buffer_insert (buffer, &iter, " ", -1);

      anchor = bobgui_text_buffer_create_child_anchor (buffer, &iter);
      child = bobgui_image_new_from_icon_name ("audio-volume-high-symbolic");
      bobgui_widget_set_cursor_from_name (child, "pointer");
      controller = BOBGUI_EVENT_CONTROLLER (bobgui_gesture_click_new ());
      g_signal_connect (controller, "pressed", G_CALLBACK (say_word), (gpointer)"hypertext");
      bobgui_widget_add_controller (child, controller);
      bobgui_text_view_add_child_at_anchor (text_view, child, anchor);

      bobgui_text_buffer_insert (buffer, &iter, "\n"
          "Machine-readable text that is not sequential but is organized "
          "so that related items of information are connected.\n", -1);
      insert_link (buffer, &iter, "Go back", 1);

      bobgui_text_buffer_delete_mark (buffer, mark);
    }
  bobgui_text_buffer_end_irreversible_action (buffer);
}

/* Looks at all tags covering the position of iter in the text view,
 * and if one of them is a link, follow it by showing the page identified
 * by the data attached to it.
 */
static void
follow_if_link (BobguiWidget   *text_view,
                BobguiTextIter *iter)
{
  GSList *tags = NULL, *tagp = NULL;

  tags = bobgui_text_iter_get_tags (iter);
  for (tagp = tags;  tagp != NULL;  tagp = tagp->next)
    {
      BobguiTextTag *tag = tagp->data;
      int page = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (tag), "page"));

      if (page != 0)
        {
          show_page (BOBGUI_TEXT_VIEW (text_view), page);
          break;
        }
    }

  if (tags)
    g_slist_free (tags);
}

/* Links can be activated by pressing Enter.
 */
static gboolean
key_pressed (BobguiEventController *controller,
             guint               keyval,
             guint               keycode,
             GdkModifierType     modifiers,
             BobguiWidget          *text_view)
{
  BobguiTextIter iter;
  BobguiTextBuffer *buffer;

  switch (keyval)
    {
      case GDK_KEY_Return:
      case GDK_KEY_KP_Enter:
        buffer = bobgui_text_view_get_buffer (BOBGUI_TEXT_VIEW (text_view));
        bobgui_text_buffer_get_iter_at_mark (buffer, &iter,
                                          bobgui_text_buffer_get_insert (buffer));
        follow_if_link (text_view, &iter);
        break;

      default:
        break;
    }

  return GDK_EVENT_PROPAGATE;
}

static void set_cursor_if_appropriate (BobguiTextView *text_view,
                                       int          x,
                                       int          y);

static void
released_cb (BobguiGestureClick *gesture,
             guint            n_press,
             double           x,
             double           y,
             BobguiWidget       *text_view)
{
  BobguiTextIter start, end, iter;
  BobguiTextBuffer *buffer;
  int tx, ty;

  if (bobgui_gesture_single_get_button (BOBGUI_GESTURE_SINGLE (gesture)) > 1)
    return;

  bobgui_text_view_window_to_buffer_coords (BOBGUI_TEXT_VIEW (text_view),
                                         BOBGUI_TEXT_WINDOW_WIDGET,
                                         x, y, &tx, &ty);

  buffer = bobgui_text_view_get_buffer (BOBGUI_TEXT_VIEW (text_view));

  /* we shouldn't follow a link if the user has selected something */
  bobgui_text_buffer_get_selection_bounds (buffer, &start, &end);
  if (bobgui_text_iter_get_offset (&start) != bobgui_text_iter_get_offset (&end))
    return;

  if (bobgui_text_view_get_iter_at_location (BOBGUI_TEXT_VIEW (text_view), &iter, tx, ty))
    follow_if_link (text_view, &iter);
}

static void
motion_cb (BobguiEventControllerMotion *controller,
           double                    x,
           double                    y,
           BobguiTextView              *text_view)
{
  int tx, ty;

  bobgui_text_view_window_to_buffer_coords (text_view,
                                         BOBGUI_TEXT_WINDOW_WIDGET,
                                         x, y, &tx, &ty);

  set_cursor_if_appropriate (text_view, tx, ty);
}

static gboolean hovering_over_link = FALSE;

/* Looks at all tags covering the position (x, y) in the text view,
 * and if one of them is a link, change the cursor to the "hands" cursor
 * typically used by web browsers.
 */
static void
set_cursor_if_appropriate (BobguiTextView *text_view,
                           int          x,
                           int          y)
{
  GSList *tags = NULL, *tagp = NULL;
  BobguiTextIter iter;
  gboolean hovering = FALSE;

  if (bobgui_text_view_get_iter_at_location (text_view, &iter, x, y))
    {
      tags = bobgui_text_iter_get_tags (&iter);
      for (tagp = tags;  tagp != NULL;  tagp = tagp->next)
        {
          BobguiTextTag *tag = tagp->data;
          int page = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (tag), "page"));

          if (page != 0)
            {
              hovering = TRUE;
              break;
            }
        }
    }

  if (hovering != hovering_over_link)
    {
      hovering_over_link = hovering;

      if (hovering_over_link)
        bobgui_widget_set_cursor_from_name (BOBGUI_WIDGET (text_view), "pointer");
      else
        bobgui_widget_set_cursor_from_name (BOBGUI_WIDGET (text_view), "text");
    }

  if (tags)
    g_slist_free (tags);
}

BobguiWidget *
do_hypertext (BobguiWidget *do_widget)
{
  static BobguiWidget *window = NULL;

  if (!window)
    {
      BobguiWidget *view;
      BobguiWidget *sw;
      BobguiTextBuffer *buffer;
      BobguiEventController *controller;

      window = bobgui_window_new ();
      bobgui_window_set_title (BOBGUI_WINDOW (window), "Hypertext");
      bobgui_window_set_display (BOBGUI_WINDOW (window),
                              bobgui_widget_get_display (do_widget));
      bobgui_window_set_default_size (BOBGUI_WINDOW (window), 330, 330);
      bobgui_window_set_resizable (BOBGUI_WINDOW (window), FALSE);
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);

      view = bobgui_text_view_new ();
      bobgui_text_view_set_wrap_mode (BOBGUI_TEXT_VIEW (view), BOBGUI_WRAP_WORD);
      bobgui_text_view_set_top_margin (BOBGUI_TEXT_VIEW (view), 20);
      bobgui_text_view_set_bottom_margin (BOBGUI_TEXT_VIEW (view), 20);
      bobgui_text_view_set_left_margin (BOBGUI_TEXT_VIEW (view), 20);
      bobgui_text_view_set_right_margin (BOBGUI_TEXT_VIEW (view), 20);
      bobgui_text_view_set_pixels_below_lines (BOBGUI_TEXT_VIEW (view), 10);
      controller = bobgui_event_controller_key_new ();
      g_signal_connect (controller, "key-pressed", G_CALLBACK (key_pressed), view);
      bobgui_widget_add_controller (view, controller);

      controller = BOBGUI_EVENT_CONTROLLER (bobgui_gesture_click_new ());
      g_signal_connect (controller, "released",
                        G_CALLBACK (released_cb), view);
      bobgui_widget_add_controller (view, controller);

      controller = bobgui_event_controller_motion_new ();
      g_signal_connect (controller, "motion",
                        G_CALLBACK (motion_cb), view);
      bobgui_widget_add_controller (view, controller);

      buffer = bobgui_text_view_get_buffer (BOBGUI_TEXT_VIEW (view));
      bobgui_text_buffer_set_enable_undo (buffer, TRUE);

      sw = bobgui_scrolled_window_new ();
      bobgui_scrolled_window_set_policy (BOBGUI_SCROLLED_WINDOW (sw),
                                      BOBGUI_POLICY_NEVER,
                                      BOBGUI_POLICY_AUTOMATIC);
      bobgui_window_set_child (BOBGUI_WINDOW (window), sw);
      bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (sw), view);

      show_page (BOBGUI_TEXT_VIEW (view), 1);
    }

  if (!bobgui_widget_get_visible (window))
    bobgui_widget_set_visible (window, TRUE);
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));

  return window;
}
