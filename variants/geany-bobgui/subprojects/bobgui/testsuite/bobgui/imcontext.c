#include <bobgui/bobgui.h>
#include <locale.h>

#include "../bobgui/bobguitextprivate.h"
#include "../bobgui/bobguitextviewprivate.h"

static void
test_text_surrounding (void)
{
  BobguiWidget *widget;
  BobguiEventController *controller;
  BobguiIMContext *context;
  gboolean ret;
  char *text;
  int cursor_pos, selection_bound;

  widget = bobgui_text_new ();
  controller = bobgui_text_get_key_controller (BOBGUI_TEXT (widget));
  context = bobgui_event_controller_key_get_im_context (BOBGUI_EVENT_CONTROLLER_KEY (controller));

  bobgui_editable_set_text (BOBGUI_EDITABLE (widget), "abcd");
  bobgui_editable_set_position (BOBGUI_EDITABLE (widget), 2);

  ret = bobgui_im_context_get_surrounding_with_selection (context,
                                                       &text,
                                                       &cursor_pos,
                                                       &selection_bound);

  g_assert_true (ret);
  g_assert_cmpstr (text, ==, "abcd");
  g_assert_cmpint (cursor_pos, ==, 2);
  g_assert_cmpint (selection_bound, ==, 2);

  g_free (text);

  ret = bobgui_im_context_delete_surrounding (context, -1, 1);
  g_assert_true (ret);

  g_assert_cmpstr (bobgui_editable_get_text (BOBGUI_EDITABLE (widget)), ==, "acd");
  g_assert_cmpint (bobgui_editable_get_position (BOBGUI_EDITABLE (widget)), ==, 1);

  ret = bobgui_im_context_delete_surrounding (context, 1, 1);
  g_assert_true (ret);

  g_assert_cmpstr (bobgui_editable_get_text (BOBGUI_EDITABLE (widget)), ==, "ac");
  g_assert_cmpint (bobgui_editable_get_position (BOBGUI_EDITABLE (widget)), ==, 1);

  bobgui_editable_set_text (BOBGUI_EDITABLE (widget), "abcd");
  bobgui_editable_select_region (BOBGUI_EDITABLE (widget), 4, 2);

  ret = bobgui_im_context_get_surrounding_with_selection (context,
                                                       &text,
                                                       &cursor_pos,
                                                       &selection_bound);

  g_assert_true (ret);
  g_assert_cmpstr (text, ==, "abcd");
  g_assert_cmpint (cursor_pos, ==, 2);
  g_assert_cmpint (selection_bound, ==, 4);

  g_free (text);

  g_object_ref_sink (widget);
  g_object_unref (widget);
}

static void
test_textview_surrounding (void)
{
  BobguiWidget *widget;
  BobguiEventController *controller;
  BobguiIMContext *context;
  BobguiTextBuffer *buffer;
  BobguiTextIter iter;
  BobguiTextIter start, end;
  BobguiTextIter bound, insert;
  gboolean ret;
  char *text;
  int anchor_pos, cursor_pos;

  widget = bobgui_text_view_new ();
  controller = bobgui_text_view_get_key_controller (BOBGUI_TEXT_VIEW (widget));
  context = bobgui_event_controller_key_get_im_context (BOBGUI_EVENT_CONTROLLER_KEY (controller));

  buffer = bobgui_text_view_get_buffer (BOBGUI_TEXT_VIEW (widget));
  bobgui_text_buffer_set_text (buffer, "abcd\nefgh\nijkl", -1);
  bobgui_text_buffer_get_iter_at_line_offset (buffer, &iter, 1, 2);
  bobgui_text_buffer_place_cursor (buffer, &iter);

  ret = bobgui_im_context_get_surrounding_with_selection (context,
                                                       &text,
                                                       &cursor_pos,
                                                       &anchor_pos);

  g_assert_true (ret);
  g_assert_cmpstr (text, ==, "abcd\nefgh\nijkl");
  g_assert_cmpint (cursor_pos, ==, 7);
  g_assert_cmpint (anchor_pos, ==, 7);

  g_free (text);

  ret = bobgui_im_context_delete_surrounding (context, -1, 1);
  g_assert_true (ret);

  bobgui_text_buffer_get_bounds (buffer, &start, &end);
  text = bobgui_text_buffer_get_text (buffer, &start, &end, FALSE);
  g_assert_cmpstr (text, ==, "abcd\negh\nijkl");
  g_free (text);
  bobgui_text_buffer_get_selection_bounds (buffer, &start, &end);
  g_assert_cmpint (bobgui_text_iter_get_line (&start), ==, 1);
  g_assert_cmpint (bobgui_text_iter_get_line_offset (&start), ==, 1);

  ret = bobgui_im_context_delete_surrounding (context, 1, 1);
  g_assert_true (ret);

  bobgui_text_buffer_get_bounds (buffer, &start, &end);
  text = bobgui_text_buffer_get_text (buffer, &start, &end, FALSE);
  g_assert_cmpstr (text, ==, "abcd\neg\nijkl");
  g_free (text);
  bobgui_text_buffer_get_selection_bounds (buffer, &start, &end);
  g_assert_cmpint (bobgui_text_iter_get_line (&start), ==, 1);
  g_assert_cmpint (bobgui_text_iter_get_line_offset (&start), ==, 1);

  buffer = bobgui_text_view_get_buffer (BOBGUI_TEXT_VIEW (widget));
  bobgui_text_buffer_set_text (buffer, "ab cd\nef gh\nijkl", -1);
  bobgui_text_buffer_get_iter_at_line_offset (buffer, &bound, 1, 4);
  bobgui_text_buffer_get_iter_at_line_offset (buffer, &insert, 2, 2);
  bobgui_text_buffer_select_range (buffer, &insert, &bound);

  ret = bobgui_im_context_get_surrounding_with_selection (context,
                                                       &text,
                                                       &cursor_pos,
                                                       &anchor_pos);

  g_assert_true (ret);
  g_assert_cmpstr (text, ==, "cd\nef gh\nijkl");
  g_assert_cmpint (anchor_pos, ==, 7);
  g_assert_cmpint (cursor_pos, ==, 11);

  g_free (text);

  g_object_ref_sink (widget);
  g_object_unref (widget);
}

int
main (int argc, char *argv[])
{
  bobgui_test_init (&argc, &argv, NULL);

  g_object_set (bobgui_settings_get_default (), "bobgui-im-module", "bobgui-im-context-simple", NULL);

  g_test_add_func ("/im-context/text-surrounding", test_text_surrounding);
  g_test_add_func ("/im-context/textview-surrounding", test_textview_surrounding);

  return g_test_run ();
}
