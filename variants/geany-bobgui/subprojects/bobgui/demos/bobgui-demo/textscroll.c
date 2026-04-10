/* Text View/Automatic Scrolling
 * #Keywords: BobguiTextView, BobguiScrolledWindow
 *
 * This example demonstrates how to use the gravity of
 * BobguiTextMarks to keep a text view scrolled to the bottom
 * while appending text.
 */

#include <bobgui/bobgui.h>

/* Scroll to the end of the buffer.
 */
static gboolean
scroll_to_end (BobguiTextView *textview)
{
  BobguiTextBuffer *buffer;
  BobguiTextIter iter;
  BobguiTextMark *mark;
  char *spaces;
  char *text;
  static int count;

  buffer = bobgui_text_view_get_buffer (textview);

  /* Get "end" mark. It's located at the end of buffer because
   * of right gravity
   */
  mark = bobgui_text_buffer_get_mark (buffer, "end");
  bobgui_text_buffer_get_iter_at_mark (buffer, &iter, mark);

  /* and insert some text at its position, the iter will be
   * revalidated after insertion to point to the end of inserted text
   */
  spaces = g_strnfill (count++, ' ');
  bobgui_text_buffer_insert (buffer, &iter, "\n", -1);
  bobgui_text_buffer_insert (buffer, &iter, spaces, -1);
  text = g_strdup_printf ("Scroll to end scroll to end scroll "
                          "to end scroll to end %d", count);
  bobgui_text_buffer_insert (buffer, &iter, text, -1);
  g_free (spaces);
  g_free (text);

  /* Now scroll the end mark onscreen.
   */
  bobgui_text_view_scroll_mark_onscreen (textview, mark);

  /* Emulate typewriter behavior, shift to the left if we
   * are far enough to the right.
   */
  if (count > 150)
    count = 0;

  return G_SOURCE_CONTINUE;
}

/* Scroll to the bottom of the buffer.
 */
static gboolean
scroll_to_bottom (BobguiTextView *textview)
{
  BobguiTextBuffer *buffer;
  BobguiTextIter iter;
  BobguiTextMark *mark;
  char *spaces;
  char *text;
  static int count;

  buffer = bobgui_text_view_get_buffer (textview);

  /* Get end iterator */
  bobgui_text_buffer_get_end_iter (buffer, &iter);

  /* and insert some text at it, the iter will be revalidated
   * after insertion to point to the end of inserted text
   */
  spaces = g_strnfill (count++, ' ');
  bobgui_text_buffer_insert (buffer, &iter, "\n", -1);
  bobgui_text_buffer_insert (buffer, &iter, spaces, -1);
  text = g_strdup_printf ("Scroll to bottom scroll to bottom scroll "
                          "to bottom scroll to bottom %d", count);
  bobgui_text_buffer_insert (buffer, &iter, text, -1);
  g_free (spaces);
  g_free (text);

  /* Move the iterator to the beginning of line, so we don't scroll
   * in horizontal direction
   */
  bobgui_text_iter_set_line_offset (&iter, 0);

  /* and place the mark at iter. the mark will stay there after we
   * insert some text at the end because it has left gravity.
   */
  mark = bobgui_text_buffer_get_mark (buffer, "scroll");
  bobgui_text_buffer_move_mark (buffer, mark, &iter);

  /* Scroll the mark onscreen.
   */
  bobgui_text_view_scroll_mark_onscreen (textview, mark);

  /* Shift text back if we got enough to the right.
   */
  if (count > 40)
    count = 0;

  return G_SOURCE_CONTINUE;
}

static guint
setup_scroll (BobguiTextView *textview,
              gboolean     to_end)
{
  BobguiTextBuffer *buffer;
  BobguiTextIter iter;

  buffer = bobgui_text_view_get_buffer (textview);
  bobgui_text_buffer_get_end_iter (buffer, &iter);

  if (to_end)
    {
      /* If we want to scroll to the end, including horizontal scrolling,
       * then we just create a mark with right gravity at the end of the
       * buffer. It will stay at the end unless explicitly moved with
       * bobgui_text_buffer_move_mark.
       */
      bobgui_text_buffer_create_mark (buffer, "end", &iter, FALSE);

      /* Add scrolling timeout. */
      return g_timeout_add (50, (GSourceFunc) scroll_to_end, textview);
    }
  else
    {
      /* If we want to scroll to the bottom, but not scroll horizontally,
       * then an end mark won't do the job. Just create a mark so we can
       * use it with bobgui_text_view_scroll_mark_onscreen, we'll position it
       * explicitly when needed. Use left gravity so the mark stays where
       * we put it after inserting new text.
       */
      bobgui_text_buffer_create_mark (buffer, "scroll", &iter, TRUE);

      /* Add scrolling timeout. */
      return g_timeout_add (100, (GSourceFunc) scroll_to_bottom, textview);
    }
}

static void
remove_timeout (BobguiWidget *window,
                gpointer   timeout)
{
  g_source_remove (GPOINTER_TO_UINT (timeout));
}

static void
create_text_view (BobguiWidget *hbox,
                  gboolean   to_end)
{
  BobguiWidget *swindow;
  BobguiWidget *textview;
  guint timeout;

  swindow = bobgui_scrolled_window_new ();
  bobgui_box_append (BOBGUI_BOX (hbox), swindow);
  textview = bobgui_text_view_new ();
  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (swindow), textview);

  timeout = setup_scroll (BOBGUI_TEXT_VIEW (textview), to_end);

  /* Remove the timeout in destroy handler, so we don't try to
   * scroll destroyed widget.
   */
  g_signal_connect (textview, "destroy",
                    G_CALLBACK (remove_timeout),
                    GUINT_TO_POINTER (timeout));
}

BobguiWidget *
do_textscroll (BobguiWidget *do_widget)
{
  static BobguiWidget *window = NULL;

  if (!window)
    {
      BobguiWidget *hbox;

      window = bobgui_window_new ();
      bobgui_window_set_title (BOBGUI_WINDOW (window), "Automatic Scrolling");
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);
      bobgui_window_set_default_size (BOBGUI_WINDOW (window), 600, 400);

      hbox = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 6);
      bobgui_box_set_homogeneous (BOBGUI_BOX (hbox), TRUE);
      bobgui_window_set_child (BOBGUI_WINDOW (window), hbox);

      create_text_view (hbox, TRUE);
      create_text_view (hbox, FALSE);
    }

  if (!bobgui_widget_get_visible (window))
    bobgui_widget_set_visible (window, TRUE);
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));

  return window;
}
