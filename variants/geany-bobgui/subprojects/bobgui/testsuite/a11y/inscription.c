#include <bobgui/bobgui.h>

#include "bobgui/bobguiaccessibletextprivate.h"

static void
inscription_text_interface (void)
{
  BobguiWidget *inscription = bobgui_inscription_new ("");
  GBytes *bytes;
  gsize len;
  gboolean res;
  gsize n_ranges;
  BobguiAccessibleTextRange *ranges = NULL;
  char **attr_names, **attr_values;
  const char *string;
  unsigned int start, end;

  g_object_ref_sink (inscription);

  bobgui_inscription_set_markup (BOBGUI_INSCRIPTION (inscription), "<markup>a<span overline='single'>b</span>c</markup> def");

  bytes = bobgui_accessible_text_get_contents (BOBGUI_ACCESSIBLE_TEXT (inscription), 0, G_MAXINT);
  string = g_bytes_get_data (bytes, &len);
  g_assert_cmpint (len, ==, 8);
  g_assert_cmpstr (string, ==, "abc def");
  g_bytes_unref (bytes);

  bytes = bobgui_accessible_text_get_contents_at (BOBGUI_ACCESSIBLE_TEXT (inscription), 1, BOBGUI_ACCESSIBLE_TEXT_GRANULARITY_WORD, &start, &end);
  string = g_bytes_get_data (bytes, &len);
  g_assert_cmpint (len, ==, 5);
  g_assert_cmpint (start, ==, 0);
  g_assert_cmpint (end, ==, 4);
  g_assert_cmpstr (string, ==, "abc ");
  g_bytes_unref (bytes);

  g_assert_cmpint (bobgui_accessible_text_get_caret_position (BOBGUI_ACCESSIBLE_TEXT (inscription)), ==, 0);

  res = bobgui_accessible_text_get_selection (BOBGUI_ACCESSIBLE_TEXT (inscription), &n_ranges, &ranges);
  g_assert_false (res);

  res = bobgui_accessible_text_get_attributes (BOBGUI_ACCESSIBLE_TEXT (inscription), 1, &n_ranges, &ranges, &attr_names, &attr_values);
  g_assert_true (res);
  g_assert_cmpint (n_ranges, ==, 1);
  g_assert_cmpuint (ranges[0].start, ==, 1);
  g_assert_cmpuint (ranges[0].length, ==, 1);
  g_assert_cmpstr (attr_names[0], ==, BOBGUI_ACCESSIBLE_ATTRIBUTE_OVERLINE);
  g_assert_cmpstr (attr_values[0], ==, BOBGUI_ACCESSIBLE_ATTRIBUTE_OVERLINE_SINGLE);
  g_free (ranges);
  g_strfreev (attr_names);
  g_strfreev (attr_values);

  g_object_unref (inscription);
}

/* Some of the text interface functions require an allocated widget */
static void
more_inscription_text_interface (void)
{
  BobguiWidget *window, *inscription;
  int width, height;
  gboolean res;
  unsigned int offset;

  window = bobgui_window_new ();
  inscription = bobgui_inscription_new ("AAA");
  bobgui_widget_set_halign (inscription, BOBGUI_ALIGN_CENTER);
  bobgui_widget_set_valign (inscription, BOBGUI_ALIGN_CENTER);
  bobgui_window_set_child (BOBGUI_WINDOW (window), inscription);

  bobgui_window_present (BOBGUI_WINDOW (window));

  while (bobgui_widget_get_width (inscription) == 0)
    g_main_context_iteration (NULL, TRUE);

  width = bobgui_widget_get_width (inscription);
  height = bobgui_widget_get_height (inscription);
  g_assert_true (width > 0);
  g_assert_true (height > 0);

  res = bobgui_accessible_text_get_offset (BOBGUI_ACCESSIBLE_TEXT (inscription),
                                        &GRAPHENE_POINT_INIT (width / 12, height / 2),
                                        &offset);
  g_assert_true (res);
  g_assert_cmpuint (offset, ==, 0);

  res = bobgui_accessible_text_get_offset (BOBGUI_ACCESSIBLE_TEXT (inscription),
                                        &GRAPHENE_POINT_INIT (width / 2, height / 2),
                                        &offset);
  g_assert_true (res);
  g_assert_cmpuint (offset, ==, 1);

  res = bobgui_accessible_text_get_offset (BOBGUI_ACCESSIBLE_TEXT (inscription),
                                        &GRAPHENE_POINT_INIT (width - width / 4, height / 2),
                                        &offset);
  g_assert_true (res);
  g_assert_cmpuint (offset, ==, 2);

  res = bobgui_accessible_text_get_offset (BOBGUI_ACCESSIBLE_TEXT (inscription),
                                        &GRAPHENE_POINT_INIT (width, height / 2),
                                        &offset);
  g_assert_true (res);
  g_assert_cmpuint (offset, ==, 3);

  bobgui_window_destroy (BOBGUI_WINDOW (window));
}

int
main (int argc, char *argv[])
{
  bobgui_test_init (&argc, &argv, NULL);

  g_test_add_func ("/a11y/inscription/text-interface", inscription_text_interface);
  g_test_add_func ("/a11y/inscription/more-text-interface", more_inscription_text_interface);

  return g_test_run ();
}
