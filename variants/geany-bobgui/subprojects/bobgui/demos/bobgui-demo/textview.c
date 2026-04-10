/* Text View/Multiple Views
 *
 * The BobguiTextView widget displays a BobguiTextBuffer. One BobguiTextBuffer
 * can be displayed by multiple BobguiTextViews. This demo has two views
 * displaying a single buffer, and shows off the widget's text
 * formatting features.
 *
 */

#include <bobgui/bobgui.h>
#include <stdlib.h> /* for exit() */
#include "paintable.h"


static void easter_egg_callback (BobguiWidget *button, gpointer data);

static void
create_tags (BobguiTextBuffer *buffer)
{
  /* Create a bunch of tags. Note that it's also possible to
   * create tags with bobgui_text_tag_new() then add them to the
   * tag table for the buffer, bobgui_text_buffer_create_tag() is
   * just a convenience function. Also note that you don't have
   * to give tags a name; pass NULL for the name to create an
   * anonymous tag.
   *
   * In any real app, another useful optimization would be to create
   * a BobguiTextTagTable in advance, and reuse the same tag table for
   * all the buffers with the same tag set, instead of creating
   * new copies of the same tags for every buffer.
   *
   * Tags are assigned default priorities in order of addition to the
   * tag table.  That is, tags created later that affect the same text
   * property affected by an earlier tag will override the earlier
   * tag.  You can modify tag priorities with
   * bobgui_text_tag_set_priority().
   */

  bobgui_text_buffer_create_tag (buffer, "heading",
                              "weight", PANGO_WEIGHT_BOLD,
                              "size", 15 * PANGO_SCALE,
                              NULL);

  bobgui_text_buffer_create_tag (buffer, "italic",
                              "style", PANGO_STYLE_ITALIC, NULL);

  bobgui_text_buffer_create_tag (buffer, "bold",
                              "weight", PANGO_WEIGHT_BOLD, NULL);

  bobgui_text_buffer_create_tag (buffer, "big",
                              /* points times the PANGO_SCALE factor */
                              "size", 20 * PANGO_SCALE, NULL);

  bobgui_text_buffer_create_tag (buffer, "xx-small",
                              "scale", PANGO_SCALE_XX_SMALL, NULL);

  bobgui_text_buffer_create_tag (buffer, "x-large",
                              "scale", PANGO_SCALE_X_LARGE, NULL);

  bobgui_text_buffer_create_tag (buffer, "monospace",
                              "family", "monospace", NULL);

  bobgui_text_buffer_create_tag (buffer, "blue_foreground",
                              "foreground", "blue", NULL);

  bobgui_text_buffer_create_tag (buffer, "red_background",
                              "background", "red", NULL);

  bobgui_text_buffer_create_tag (buffer, "big_gap_before_line",
                              "pixels_above_lines", 30, NULL);

  bobgui_text_buffer_create_tag (buffer, "big_gap_after_line",
                              "pixels_below_lines", 30, NULL);

  bobgui_text_buffer_create_tag (buffer, "double_spaced_line",
                              "pixels_inside_wrap", 10, NULL);

  bobgui_text_buffer_create_tag (buffer, "not_editable",
                              "editable", FALSE, NULL);

  bobgui_text_buffer_create_tag (buffer, "word_wrap",
                              "wrap_mode", BOBGUI_WRAP_WORD, NULL);

  bobgui_text_buffer_create_tag (buffer, "char_wrap",
                              "wrap_mode", BOBGUI_WRAP_CHAR, NULL);

  bobgui_text_buffer_create_tag (buffer, "no_wrap",
                              "wrap_mode", BOBGUI_WRAP_NONE, NULL);

  bobgui_text_buffer_create_tag (buffer, "center",
                              "justification", BOBGUI_JUSTIFY_CENTER, NULL);

  bobgui_text_buffer_create_tag (buffer, "right_justify",
                              "justification", BOBGUI_JUSTIFY_RIGHT, NULL);

  bobgui_text_buffer_create_tag (buffer, "wide_margins",
                              "left_margin", 50, "right_margin", 50,
                              NULL);

  bobgui_text_buffer_create_tag (buffer, "strikethrough",
                              "strikethrough", TRUE, NULL);

  bobgui_text_buffer_create_tag (buffer, "underline",
                              "underline", PANGO_UNDERLINE_SINGLE, NULL);

  bobgui_text_buffer_create_tag (buffer, "double_underline",
                              "underline", PANGO_UNDERLINE_DOUBLE, NULL);

  bobgui_text_buffer_create_tag (buffer, "superscript",
                              "rise", 10 * PANGO_SCALE,   /* 10 pixels */
                              "size", 8 * PANGO_SCALE,    /* 8 points */
                              NULL);

  bobgui_text_buffer_create_tag (buffer, "subscript",
                              "rise", -10 * PANGO_SCALE,   /* 10 pixels */
                              "size", 8 * PANGO_SCALE,     /* 8 points */
                              NULL);

  bobgui_text_buffer_create_tag (buffer, "rtl_quote",
                              "wrap_mode", BOBGUI_WRAP_WORD,
                              "direction", BOBGUI_TEXT_DIR_RTL,
                              "indent", 30,
                              "left_margin", 20,
                              "right_margin", 20,
                              NULL);
}

static void
insert_text (BobguiTextView *view)
{
  BobguiWidget *widget = BOBGUI_WIDGET (view);
  BobguiTextBuffer *buffer = bobgui_text_view_get_buffer (view);
  BobguiTextIter iter;
  BobguiTextIter start, end;
  BobguiIconTheme *icon_theme;
  BobguiIconPaintable *icon;
  GdkPaintable *nuclear;

  icon_theme = bobgui_icon_theme_get_for_display (bobgui_widget_get_display (widget));
  icon = bobgui_icon_theme_lookup_icon (icon_theme,
                                     "drive-harddisk",
                                     NULL,
                                     32, 1,
                                     bobgui_widget_get_direction (widget),
                                     0);
  nuclear = bobgui_nuclear_animation_new (TRUE);

  /* get start of buffer; each insertion will revalidate the
   * iterator to point to just after the inserted text.
   */
  bobgui_text_buffer_get_iter_at_offset (buffer, &iter, 0);

  bobgui_text_buffer_begin_irreversible_action (buffer);
  bobgui_text_buffer_insert (buffer, &iter,
      "The text widget can display text with all kinds of nifty attributes. "
      "It also supports multiple views of the same buffer; this demo is "
      "showing the same buffer in two places.\n\n", -1);

  bobgui_text_buffer_insert_with_tags_by_name (buffer, &iter,
                                            "Font styles. ", -1,
                                            "heading", NULL);

  bobgui_text_buffer_insert (buffer, &iter, "For example, you can have ", -1);
  bobgui_text_buffer_insert_with_tags_by_name (buffer, &iter,
                                            "italic", -1,
                                            "italic", NULL);
  bobgui_text_buffer_insert (buffer, &iter, ", ", -1);
  bobgui_text_buffer_insert_with_tags_by_name (buffer, &iter,
                                            "bold", -1,
                                            "bold", NULL);
  bobgui_text_buffer_insert (buffer, &iter, ", or ", -1);
  bobgui_text_buffer_insert_with_tags_by_name (buffer, &iter,
                                            "monospace (typewriter)", -1,
                                            "monospace", NULL);
  bobgui_text_buffer_insert (buffer, &iter, ", or ", -1);
  bobgui_text_buffer_insert_with_tags_by_name (buffer, &iter,
                                            "big", -1,
                                            "big", NULL);
  bobgui_text_buffer_insert (buffer, &iter, " text. ", -1);
  bobgui_text_buffer_insert (buffer, &iter,
      "It's best not to hardcode specific text sizes; you can use relative "
      "sizes as with CSS, such as ", -1);
  bobgui_text_buffer_insert_with_tags_by_name (buffer, &iter,
                                            "xx-small", -1,
                                            "xx-small", NULL);
  bobgui_text_buffer_insert (buffer, &iter, " or ", -1);
  bobgui_text_buffer_insert_with_tags_by_name (buffer, &iter,
                                            "x-large", -1,
                                            "x-large", NULL);
  bobgui_text_buffer_insert (buffer, &iter,
      " to ensure that your program properly adapts if the user changes the "
      "default font size.\n\n", -1);

  bobgui_text_buffer_insert_with_tags_by_name (buffer, &iter, "Colors. ", -1,
                                            "heading", NULL);

  bobgui_text_buffer_insert (buffer, &iter, "Colors such as ", -1);
  bobgui_text_buffer_insert_with_tags_by_name (buffer, &iter,
                                            "a blue foreground", -1,
                                            "blue_foreground", NULL);
  bobgui_text_buffer_insert (buffer, &iter, " or ", -1);
  bobgui_text_buffer_insert_with_tags_by_name (buffer, &iter,
                                            "a red background", -1,
                                            "red_background", NULL);
  bobgui_text_buffer_insert (buffer, &iter, " or even ", -1);
  bobgui_text_buffer_insert_with_tags_by_name (buffer, &iter,
                                            "a blue foreground on red background", -1,
                                            "blue_foreground",
                                            "red_background",
                                            NULL);
  bobgui_text_buffer_insert (buffer, &iter, " (select that to read it) can be used.\n\n", -1);

  bobgui_text_buffer_insert_with_tags_by_name (buffer, &iter,
                                            "Underline, strikethrough, and rise. ", -1,
                                            "heading", NULL);

  bobgui_text_buffer_insert_with_tags_by_name (buffer, &iter,
                                            "Strikethrough", -1,
                                            "strikethrough", NULL);
  bobgui_text_buffer_insert (buffer, &iter, ", ", -1);
  bobgui_text_buffer_insert_with_tags_by_name (buffer, &iter,
                                            "underline", -1,
                                            "underline", NULL);
  bobgui_text_buffer_insert (buffer, &iter, ", ", -1);
  bobgui_text_buffer_insert_with_tags_by_name (buffer, &iter,
                                            "double underline", -1,
                                            "double_underline", NULL);
  bobgui_text_buffer_insert (buffer, &iter, ", ", -1);
  bobgui_text_buffer_insert_with_tags_by_name (buffer, &iter,
                                            "superscript", -1,
                                            "superscript", NULL);
  bobgui_text_buffer_insert (buffer, &iter, ", and ", -1);
  bobgui_text_buffer_insert_with_tags_by_name (buffer, &iter,
                                            "subscript", -1,
                                            "subscript", NULL);
  bobgui_text_buffer_insert (buffer, &iter, " are all supported.\n\n", -1);

  bobgui_text_buffer_insert_with_tags_by_name (buffer, &iter, "Images. ", -1,
                                            "heading", NULL);

  bobgui_text_buffer_insert (buffer, &iter, "The buffer can have images in it: ", -1);
  bobgui_text_buffer_insert_paintable (buffer, &iter, GDK_PAINTABLE (icon));
  bobgui_text_buffer_insert_paintable (buffer, &iter, nuclear);

  bobgui_text_buffer_insert (buffer, &iter, " for example.\n\n", -1);

  bobgui_text_buffer_insert_with_tags_by_name (buffer, &iter, "Spacing. ", -1,
                                            "heading", NULL);

  bobgui_text_buffer_insert (buffer, &iter,
      "You can adjust the amount of space before each line.\n", -1);

  bobgui_text_buffer_insert_with_tags_by_name (buffer, &iter,
      "This line has a whole lot of space before it.\n", -1,
      "big_gap_before_line", "wide_margins", NULL);
  bobgui_text_buffer_insert_with_tags_by_name (buffer, &iter,
      "You can also adjust the amount of space after each line; "
      "this line has a whole lot of space after it.\n", -1,
      "big_gap_after_line", "wide_margins", NULL);

  bobgui_text_buffer_insert_with_tags_by_name (buffer, &iter,
      "You can also adjust the amount of space between wrapped lines; "
      "this line has extra space between each wrapped line in the same "
      "paragraph. To show off wrapping, some filler text: the quick "
      "brown fox jumped over the lazy dog. Blah blah blah blah blah "
      "blah blah blah blah.\n", -1,
      "double_spaced_line", "wide_margins", NULL);

  bobgui_text_buffer_insert (buffer, &iter,
      "Also note that those lines have extra-wide margins.\n\n", -1);

  bobgui_text_buffer_insert_with_tags_by_name (buffer, &iter,
                                            "Editability. ", -1,
                                            "heading", NULL);

  bobgui_text_buffer_insert_with_tags_by_name (buffer, &iter,
      "This line is 'locked down' and can't be edited by the user - just "
      "try it! You can't delete this line.\n\n", -1,
      "not_editable", NULL);

  bobgui_text_buffer_insert_with_tags_by_name (buffer, &iter,
                                            "Wrapping. ", -1,
                                            "heading", NULL);

  bobgui_text_buffer_insert (buffer, &iter,
      "This line (and most of the others in this buffer) is word-wrapped, "
      "using the proper Unicode algorithm. Word wrap should work in all "
      "scripts and languages that BOBGUI supports. Let's make this a long "
      "paragraph to demonstrate: blah blah blah blah blah blah blah blah "
      "blah blah blah blah blah blah blah blah blah blah blah\n\n", -1);

  bobgui_text_buffer_insert_with_tags_by_name (buffer, &iter,
      "This line has character-based wrapping, and can wrap between any two "
      "character glyphs. Let's make this a long paragraph to demonstrate: "
      "blah blah blah blah blah blah blah blah blah blah blah blah blah blah "
      "blah blah blah blah blah\n\n", -1, "char_wrap", NULL);

  bobgui_text_buffer_insert_with_tags_by_name (buffer, &iter,
      "This line has all wrapping turned off, so it makes the horizontal "
      "scrollbar appear.\n\n\n", -1, "no_wrap", NULL);

  bobgui_text_buffer_insert_with_tags_by_name (buffer, &iter,
                                            "Justification. ", -1,
                                            "heading", NULL);

  bobgui_text_buffer_insert_with_tags_by_name (buffer, &iter,
                                            "\nThis line has center justification.\n", -1,
                                            "center", NULL);

  bobgui_text_buffer_insert_with_tags_by_name (buffer, &iter,
                                            "This line has right justification.\n", -1,
                                            "right_justify", NULL);

  bobgui_text_buffer_insert_with_tags_by_name (buffer, &iter,
      "\nThis line has big wide margins. Text text text text text text text "
      "text text text text text text text text text text text text text text "
      "text text text text text text text text text text text text text text "
      "text.\n", -1, "wide_margins", NULL);

  bobgui_text_buffer_insert_with_tags_by_name (buffer, &iter,
                                            "Internationalization. ", -1,
                                            "heading", NULL);

  bobgui_text_buffer_insert (buffer, &iter,
      "You can put all sorts of Unicode text in the buffer.\n\nGerman "
      "(Deutsch S\303\274d) Gr\303\274\303\237 Gott\nGreek "
      "(\316\225\316\273\316\273\316\267\316\275\316\271\316\272\316\254) "
      "\316\223\316\265\316\271\316\254 \317\203\316\261\317\202\nHebrew      "
      "\327\251\327\234\327\225\327\235\nJapanese "
      "(\346\227\245\346\234\254\350\252\236)\n\nThe widget properly handles "
      "bidirectional text, word wrapping, DOS/UNIX/Unicode paragraph separators, "
      "grapheme boundaries, and so on using the Pango internationalization "
      "framework.\n", -1);

  bobgui_text_buffer_insert (buffer, &iter,
      "Here's a word-wrapped quote in a right-to-left language:\n", -1);
  bobgui_text_buffer_insert_with_tags_by_name (buffer, &iter,
      "\331\210\331\202\330\257 \330\250\330\257\330\243 "
      "\330\253\331\204\330\247\330\253 \331\205\331\206 "
      "\330\243\331\203\330\253\330\261 \330\247\331\204\331\205\330\244\330\263\330\263\330\247\330\252 "
      "\330\252\331\202\330\257\331\205\330\247 \331\201\331\212 "
      "\330\264\330\250\331\203\330\251 \330\247\331\203\330\263\331\212\331\210\331\206 "
      "\330\250\330\261\330\247\331\205\330\254\331\207\330\247 "
      "\331\203\331\205\331\206\330\270\331\205\330\247\330\252 "
      "\331\204\330\247 \330\252\330\263\330\271\331\211 \331\204\331\204\330\261\330\250\330\255\330\214 "
      "\330\253\331\205 \330\252\330\255\331\210\331\204\330\252 "
      "\331\201\331\212 \330\247\331\204\330\263\331\206\331\210\330\247\330\252 "
      "\330\247\331\204\330\256\331\205\330\263 \330\247\331\204\331\205\330\247\330\266\331\212\330\251 "
      "\330\245\331\204\331\211 \331\205\330\244\330\263\330\263\330\247\330\252 "
      "\331\205\330\247\331\204\331\212\330\251 \331\205\331\206\330\270\331\205\330\251\330\214 "
      "\331\210\330\250\330\247\330\252\330\252 \330\254\330\262\330\241\330\247 "
      "\331\205\331\206 \330\247\331\204\331\206\330\270\330\247\331\205 "
      "\330\247\331\204\331\205\330\247\331\204\331\212 \331\201\331\212 "
      "\330\250\331\204\330\257\330\247\331\206\331\207\330\247\330\214 "
      "\331\210\331\204\331\203\331\206\331\207\330\247 \330\252\330\252\330\256\330\265\330\265 "
      "\331\201\331\212 \330\256\330\257\331\205\330\251 \331\202\330\267\330\247\330\271 "
      "\330\247\331\204\331\205\330\264\330\261\331\210\330\271\330\247\330\252 "
      "\330\247\331\204\330\265\330\272\331\212\330\261\330\251. \331\210\330\243\330\255\330\257 "
      "\330\243\331\203\330\253\330\261 \331\207\330\260\331\207 "
      "\330\247\331\204\331\205\330\244\330\263\330\263\330\247\330\252 "
      "\331\206\330\254\330\247\330\255\330\247 \331\207\331\210 "
      "\302\273\330\250\330\247\331\206\331\203\331\210\330\263\331\210\331\204\302\253 "
      "\331\201\331\212 \330\250\331\210\331\204\331\212\331\201\331\212\330\247.\n\n", -1,
                                                "rtl_quote", NULL);

  bobgui_text_buffer_insert (buffer, &iter,
                          "You can put widgets in the buffer: Here's a button: ", -1);
  bobgui_text_buffer_create_child_anchor (buffer, &iter);
  bobgui_text_buffer_insert (buffer, &iter, " and a menu: ", -1);
  bobgui_text_buffer_create_child_anchor (buffer, &iter);
  bobgui_text_buffer_insert (buffer, &iter, " and a scale: ", -1);
  bobgui_text_buffer_create_child_anchor (buffer, &iter);
  bobgui_text_buffer_insert (buffer, &iter, " finally a text entry: ", -1);
  bobgui_text_buffer_create_child_anchor (buffer, &iter);
  bobgui_text_buffer_insert (buffer, &iter, ".\n", -1);

  bobgui_text_buffer_insert (buffer, &iter,
      "\n\nThis demo doesn't demonstrate all the BobguiTextBuffer features; "
      "it leaves out, for example: invisible/hidden text, tab stops, "
      "application-drawn areas on the sides of the widget for displaying "
      "breakpoints and such...", -1);

  /* Apply word_wrap tag to whole buffer */
  bobgui_text_buffer_get_bounds (buffer, &start, &end);
  bobgui_text_buffer_apply_tag_by_name (buffer, "word_wrap", &start, &end);

  bobgui_text_buffer_end_irreversible_action (buffer);

  g_object_unref (icon);
  g_object_unref (nuclear);
}

static gboolean
find_anchor (BobguiTextIter *iter)
{
  while (bobgui_text_iter_forward_char (iter))
    {
      if (bobgui_text_iter_get_child_anchor (iter))
        return TRUE;
    }
  return FALSE;
}

static void
attach_widgets (BobguiTextView *text_view)
{
  BobguiTextIter iter;
  BobguiTextBuffer *buffer;
  int i;

  buffer = bobgui_text_view_get_buffer (text_view);

  bobgui_text_buffer_get_start_iter (buffer, &iter);

  i = 0;
  while (find_anchor (&iter))
    {
      BobguiTextChildAnchor *anchor;
      BobguiWidget *widget;

      anchor = bobgui_text_iter_get_child_anchor (&iter);

      if (i == 0)
        {
          widget = bobgui_button_new_with_label ("Click Me");

          g_signal_connect (widget, "clicked",
                            G_CALLBACK (easter_egg_callback),
                            NULL);
        }
      else if (i == 1)
        {
          const char *options[] = {
            "Option 1", "Option 2", "Option 3", NULL
          };

          widget = bobgui_drop_down_new_from_strings (options);
        }
      else if (i == 2)
        {
          widget = bobgui_scale_new (BOBGUI_ORIENTATION_HORIZONTAL, NULL);
          bobgui_range_set_range (BOBGUI_RANGE (widget), 0, 100);
          bobgui_widget_set_size_request (widget, 100, -1);
        }
      else if (i == 3)
        {
          widget = bobgui_entry_new ();
          bobgui_editable_set_width_chars (BOBGUI_EDITABLE (widget), 10);
        }
      else
        {
          widget = NULL; /* avoids a compiler warning */
          g_assert_not_reached ();
        }

      bobgui_text_view_add_child_at_anchor (text_view,
                                         widget,
                                         anchor);

      ++i;
    }
}

BobguiWidget *
do_textview (BobguiWidget *do_widget)
{
  static BobguiWidget *window = NULL;

  if (!window)
    {
      BobguiWidget *vpaned;
      BobguiWidget *view1;
      BobguiWidget *view2;
      BobguiWidget *sw;
      BobguiTextBuffer *buffer;

      window = bobgui_window_new ();
      bobgui_window_set_display (BOBGUI_WINDOW (window),
                              bobgui_widget_get_display (do_widget));
      bobgui_window_set_default_size (BOBGUI_WINDOW (window), 450, 450);
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);

      bobgui_window_set_title (BOBGUI_WINDOW (window), "Multiple Views");

      vpaned = bobgui_paned_new (BOBGUI_ORIENTATION_VERTICAL);
      bobgui_window_set_child (BOBGUI_WINDOW (window), vpaned);

      /* For convenience, we just use the autocreated buffer from
       * the first text view; you could also create the buffer
       * by itself with bobgui_text_buffer_new(), then later create
       * a view widget.
       */
      view1 = bobgui_text_view_new ();
      buffer = bobgui_text_view_get_buffer (BOBGUI_TEXT_VIEW (view1));
      view2 = bobgui_text_view_new_with_buffer (buffer);

      sw = bobgui_scrolled_window_new ();
      bobgui_scrolled_window_set_policy (BOBGUI_SCROLLED_WINDOW (sw),
                                      BOBGUI_POLICY_AUTOMATIC,
                                      BOBGUI_POLICY_AUTOMATIC);
      bobgui_paned_set_start_child (BOBGUI_PANED (vpaned), sw);
      bobgui_paned_set_resize_start_child (BOBGUI_PANED (vpaned), FALSE);
      bobgui_paned_set_shrink_start_child (BOBGUI_PANED (vpaned), TRUE);

      bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (sw), view1);

      sw = bobgui_scrolled_window_new ();
      bobgui_scrolled_window_set_policy (BOBGUI_SCROLLED_WINDOW (sw),
                                      BOBGUI_POLICY_AUTOMATIC,
                                      BOBGUI_POLICY_AUTOMATIC);
      bobgui_paned_set_end_child (BOBGUI_PANED (vpaned), sw);
      bobgui_paned_set_resize_end_child (BOBGUI_PANED (vpaned), TRUE);
      bobgui_paned_set_shrink_end_child (BOBGUI_PANED (vpaned), TRUE);

      bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (sw), view2);

      create_tags (buffer);
      insert_text (BOBGUI_TEXT_VIEW (view1));

      attach_widgets (BOBGUI_TEXT_VIEW (view1));
      attach_widgets (BOBGUI_TEXT_VIEW (view2));
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

static void
recursive_attach_view (int                 depth,
                       BobguiTextView        *view,
                       BobguiTextChildAnchor *anchor)
{
  BobguiWidget *child_view, *frame;

  if (depth > 4)
    return;

  child_view = bobgui_text_view_new_with_buffer (bobgui_text_view_get_buffer (view));
  bobgui_widget_set_size_request (child_view, 260 - 20 * depth, -1);

  /* Frame is to add a black border around each child view */
  frame = bobgui_frame_new (NULL);
  bobgui_frame_set_child (BOBGUI_FRAME (frame), child_view);

  bobgui_text_view_add_child_at_anchor (view, frame, anchor);

  recursive_attach_view (depth + 1, BOBGUI_TEXT_VIEW (child_view), anchor);
}

static void
easter_egg_callback (BobguiWidget *button,
                     gpointer   data)
{
  static BobguiWidget *window = NULL;
  gpointer window_ptr;
  BobguiTextBuffer *buffer;
  BobguiWidget     *view;
  BobguiTextIter    iter;
  BobguiTextChildAnchor *anchor;
  BobguiWidget *sw;

  if (window)
    {
      bobgui_window_present (BOBGUI_WINDOW (window));
      return;
    }

  buffer = bobgui_text_buffer_new (NULL);

  bobgui_text_buffer_get_start_iter (buffer, &iter);

  bobgui_text_buffer_insert (buffer, &iter,
      "This buffer is shared by a set of nested text views.\n Nested view:\n", -1);
  anchor = bobgui_text_buffer_create_child_anchor (buffer, &iter);
  bobgui_text_buffer_insert (buffer, &iter,
                          "\nDon't do this in real applications, please.\n", -1);

  view = bobgui_text_view_new_with_buffer (buffer);

  recursive_attach_view (0, BOBGUI_TEXT_VIEW (view), anchor);

  g_object_unref (buffer);

  window = bobgui_window_new ();
  bobgui_window_set_transient_for (BOBGUI_WINDOW (window), BOBGUI_WINDOW (bobgui_widget_get_root (button)));
  bobgui_window_set_modal (BOBGUI_WINDOW (window), TRUE);
  sw = bobgui_scrolled_window_new ();
  bobgui_scrolled_window_set_policy (BOBGUI_SCROLLED_WINDOW (sw),
                                  BOBGUI_POLICY_AUTOMATIC,
                                  BOBGUI_POLICY_AUTOMATIC);

  bobgui_window_set_child (BOBGUI_WINDOW (window), sw);
  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (sw), view);

  window_ptr = &window;
  g_object_add_weak_pointer (G_OBJECT (window), window_ptr);

  bobgui_window_set_default_size (BOBGUI_WINDOW (window), 300, 400);

  bobgui_window_present (BOBGUI_WINDOW (window));
}
