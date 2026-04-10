#include <bobgui/bobgui.h>

#include "bobgui/bobguiatcontextprivate.h"
#include "bobgui/bobguiaccessibletextprivate.h"

static void
textview_role (void)
{
  BobguiWidget *widget = bobgui_text_view_new ();
  g_object_ref_sink (widget);

  bobgui_test_accessible_assert_role (widget, BOBGUI_ACCESSIBLE_ROLE_TEXT_BOX);

  g_object_unref (widget);
}

static void
textview_properties (void)
{
  BobguiWidget *widget = bobgui_text_view_new ();
  g_object_ref_sink (widget);

  bobgui_test_accessible_assert_property (widget, BOBGUI_ACCESSIBLE_PROPERTY_MULTI_LINE, TRUE);
  bobgui_test_accessible_assert_property (widget, BOBGUI_ACCESSIBLE_PROPERTY_READ_ONLY, FALSE);

  bobgui_text_view_set_editable (BOBGUI_TEXT_VIEW (widget), FALSE);

  bobgui_test_accessible_assert_property (widget, BOBGUI_ACCESSIBLE_PROPERTY_READ_ONLY, TRUE);

  g_object_unref (widget);
}

typedef struct
{
  guint update_caret_pos_count;
  guint caret_pos;

  guint update_selection_bound_count;

  guint update_text_contents_count;
  BobguiAccessibleTextContentChange change;
  guint start;
  guint end;
  GBytes *contents;
} TestData;

static void
test_data_clear (TestData *td)
{
  td->update_caret_pos_count = 0;
  td->caret_pos = G_MAXUINT;

  td->update_selection_bound_count = 0;

  td->update_text_contents_count = 0;
  td->change = 0;
  td->start = G_MAXUINT;
  td->end = G_MAXUINT;
  g_clear_pointer (&td->contents, g_bytes_unref);
}

static void
update_caret_pos (BobguiATContext *context,
                  guint         caret_pos,
                  gpointer      data)
{
  TestData *td = data;

  td->update_caret_pos_count++;
  td->caret_pos = caret_pos;
}

static void
update_selection_bound (BobguiATContext *context,
                        gpointer      data)
{
  TestData *td = data;

  td->update_selection_bound_count++;
}

static void
update_text_contents (BobguiATContext                   *context,
                      BobguiAccessibleTextContentChange  change,
                      guint                           start,
                      guint                           end,
                      GBytes                         *contents,
                      gpointer                        data)
{
  TestData *td = data;

  td->update_text_contents_count++;

  td->change = change;
  td->start = start;
  td->end = end;
  g_clear_pointer (&td->contents, g_bytes_unref);
  td->contents = g_bytes_ref (contents);
}

static void
textview_accessible_text (void)
{
  BobguiWidget *widget = bobgui_text_view_new ();
  GBytes *bytes;
  gsize len;
  gboolean res;
  gsize n_ranges;
  BobguiAccessibleTextRange *ranges = NULL;
  char **attr_names, **attr_values;
  const char *string;
  BobguiATContext *context;
  TestData td = { 0, };
  BobguiTextBuffer *buffer;
  BobguiTextTag *tag;
  BobguiTextIter start, end;
  unsigned int start_index, end_index;

  g_object_ref_sink (widget);

  buffer = bobgui_text_view_get_buffer (BOBGUI_TEXT_VIEW (widget));

  context = bobgui_accessible_get_at_context (BOBGUI_ACCESSIBLE (widget));
  bobgui_at_context_realize (context);

  g_signal_connect (context, "update-caret-position", G_CALLBACK (update_caret_pos), &td);
  g_signal_connect (context, "update-selection-bound", G_CALLBACK (update_selection_bound), &td);
  g_signal_connect (context, "update-text-contents", G_CALLBACK (update_text_contents), &td);

  test_data_clear (&td);

  bobgui_text_buffer_set_text (buffer, "abc def", -1);

  g_assert_cmpuint (td.update_text_contents_count, ==, 1);
  g_assert_cmpuint (td.change, ==, BOBGUI_ACCESSIBLE_TEXT_CONTENT_CHANGE_INSERT);
  g_assert_cmpuint (td.start, ==, 0);
  g_assert_cmpuint (td.end, ==, 7);
  g_assert_cmpstr ("abc def", ==, g_bytes_get_data (td.contents, NULL));

  tag = bobgui_text_tag_new ("uline");
  g_object_set (tag, "underline", PANGO_UNDERLINE_DOUBLE, NULL);
  bobgui_text_tag_table_add (bobgui_text_buffer_get_tag_table (buffer), tag);
  g_object_unref (tag);

  bobgui_text_buffer_get_iter_at_offset (buffer, &start, 1);
  bobgui_text_buffer_get_iter_at_offset (buffer, &end, 2);
  bobgui_text_buffer_apply_tag_by_name (buffer, "uline", &start, &end);

  test_data_clear (&td);

  bobgui_text_buffer_select_range (buffer, &end, &start);

  g_assert_cmpuint (td.update_caret_pos_count, ==, 1);
  g_assert_cmpuint (td.caret_pos, ==, 2);

  g_assert_cmpint (bobgui_accessible_text_get_caret_position (BOBGUI_ACCESSIBLE_TEXT (widget)), ==, 2);

  bytes = bobgui_accessible_text_get_contents (BOBGUI_ACCESSIBLE_TEXT (widget), 0, G_MAXINT);
  string = g_bytes_get_data (bytes, &len);
  g_assert_cmpint (len, ==, 8);
  g_assert_cmpstr (string, ==, "abc def");
  g_bytes_unref (bytes);

  bytes = bobgui_accessible_text_get_contents_at (BOBGUI_ACCESSIBLE_TEXT (widget), 1, BOBGUI_ACCESSIBLE_TEXT_GRANULARITY_WORD, &start_index, &end_index);
  string = g_bytes_get_data (bytes, &len);
  g_assert_cmpint (len, ==, 5);
  g_assert_cmpint (start_index, ==, 0);
  g_assert_cmpint (end_index, ==, 4);
  g_assert_cmpstr (string, ==, "abc ");
  g_bytes_unref (bytes);

  res = bobgui_accessible_text_get_selection (BOBGUI_ACCESSIBLE_TEXT (widget), &n_ranges, &ranges);
  g_assert_true (res);
  g_assert_cmpint (n_ranges, ==, 1);
  g_assert_cmpuint (ranges[0].start, ==, 1);
  g_assert_cmpuint (ranges[0].length, ==, 1);
  g_free (ranges);

  res = bobgui_accessible_text_get_attributes (BOBGUI_ACCESSIBLE_TEXT (widget), 1, &n_ranges, &ranges, &attr_names, &attr_values);
  g_assert_true (res);
  g_assert_cmpint (n_ranges, ==, 1);
  g_assert_cmpuint (ranges[0].start, ==, 1);
  g_assert_cmpuint (ranges[0].length, ==, 1);
  g_assert_cmpstr (attr_names[0], ==, BOBGUI_ACCESSIBLE_ATTRIBUTE_UNDERLINE);
  g_assert_cmpstr (attr_values[0], ==, BOBGUI_ACCESSIBLE_ATTRIBUTE_UNDERLINE_DOUBLE);
  g_free (ranges);
  g_strfreev (attr_names);
  g_strfreev (attr_values);

  test_data_clear (&td);

  bobgui_text_buffer_delete (buffer, &start, &end);

  g_assert_cmpuint (td.update_text_contents_count, ==, 1);
  g_assert_cmpuint (td.change, ==, BOBGUI_ACCESSIBLE_TEXT_CONTENT_CHANGE_REMOVE);
  g_assert_cmpuint (td.start, ==, 1);
  g_assert_cmpuint (td.end, ==, 2);
  g_assert_cmpstr ("b", ==, g_bytes_get_data (td.contents, NULL));

  test_data_clear (&td);

  bobgui_at_context_unrealize (context);

  g_object_unref (widget);
}

int
main (int argc, char *argv[])
{
  bobgui_test_init (&argc, &argv, NULL);

  g_test_add_func ("/a11y/textview/role", textview_role);
  g_test_add_func ("/a11y/textview/properties", textview_properties);
  g_test_add_func ("/a11y/textview/text-accessible", textview_accessible_text);

  return g_test_run ();
}
