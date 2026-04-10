#include <bobgui/bobgui.h>

#include "bobgui/bobguiaccessibletextprivate.h"

static void
label_role (void)
{
  BobguiWidget *label = bobgui_label_new ("a");

  g_object_ref_sink (label);

  bobgui_test_accessible_assert_role (BOBGUI_ACCESSIBLE (label), BOBGUI_ACCESSIBLE_ROLE_LABEL);

  g_object_unref (label);
}

static void
label_relations (void)
{
  BobguiWidget *label = bobgui_label_new ("a");
  BobguiWidget *label2 = bobgui_label_new ("b");
  BobguiWidget *entry = bobgui_entry_new ();

  g_object_ref_sink (label);
  g_object_ref_sink (label2);
  g_object_ref_sink (entry);

  bobgui_test_accessible_assert_relation (BOBGUI_ACCESSIBLE (entry), BOBGUI_ACCESSIBLE_RELATION_LABELLED_BY, NULL);

  bobgui_widget_add_mnemonic_label (entry, label);

  bobgui_test_accessible_assert_relation (BOBGUI_ACCESSIBLE (entry), BOBGUI_ACCESSIBLE_RELATION_LABELLED_BY, label, NULL);

  bobgui_widget_add_mnemonic_label (entry, label2);

  bobgui_test_accessible_assert_relation (BOBGUI_ACCESSIBLE (entry), BOBGUI_ACCESSIBLE_RELATION_LABELLED_BY, label, label2, NULL);

  g_object_unref (entry);
  g_object_unref (label);
  g_object_unref (label2);
}

static void
label_properties (void)
{
  BobguiWidget *label = bobgui_label_new ("a");

  g_object_ref_sink (label);

  bobgui_label_set_selectable (BOBGUI_LABEL (label), TRUE);

  bobgui_test_accessible_assert_property (BOBGUI_ACCESSIBLE (label), BOBGUI_ACCESSIBLE_PROPERTY_HAS_POPUP, TRUE);

  bobgui_label_set_selectable (BOBGUI_LABEL (label), FALSE);

  g_assert_false (bobgui_test_accessible_has_property (BOBGUI_ACCESSIBLE (label), BOBGUI_ACCESSIBLE_PROPERTY_HAS_POPUP));

  g_object_unref (label);
}

static void
label_text_interface (void)
{
  BobguiWidget *label = bobgui_label_new ("");
  GBytes *bytes;
  gsize len;
  gboolean res;
  gsize n_ranges;
  BobguiAccessibleTextRange *ranges = NULL;
  char **attr_names, **attr_values;
  const char *string;
  unsigned int start, end;

  g_object_ref_sink (label);

  bobgui_label_set_markup (BOBGUI_LABEL (label), "<markup>a<span underline='single'>b</span>c def</markup>");
  bobgui_label_set_selectable (BOBGUI_LABEL (label), TRUE);
  bobgui_label_select_region (BOBGUI_LABEL (label), 1, 2);

  bytes = bobgui_accessible_text_get_contents (BOBGUI_ACCESSIBLE_TEXT (label), 0, G_MAXINT);
  string = g_bytes_get_data (bytes, &len);
  g_assert_cmpint (len, ==, 8);
  g_assert_cmpstr (string, ==, "abc def");
  g_bytes_unref (bytes);

  bytes = bobgui_accessible_text_get_contents_at (BOBGUI_ACCESSIBLE_TEXT (label), 1, BOBGUI_ACCESSIBLE_TEXT_GRANULARITY_WORD, &start, &end);
  string = g_bytes_get_data (bytes, &len);
  g_assert_cmpint (len, ==, 5);
  g_assert_cmpint (start, ==, 0);
  g_assert_cmpint (end, ==, 4);
  g_assert_cmpstr (string, ==, "abc ");
  g_bytes_unref (bytes);

  g_assert_cmpint (bobgui_accessible_text_get_caret_position (BOBGUI_ACCESSIBLE_TEXT (label)), ==, 2);

  res = bobgui_accessible_text_get_selection (BOBGUI_ACCESSIBLE_TEXT (label), &n_ranges, &ranges);
  g_assert_true (res);
  g_assert_cmpint (n_ranges, ==, 1);
  g_assert_cmpuint (ranges[0].start, ==, 1);
  g_assert_cmpuint (ranges[0].length, ==, 1);
  g_free (ranges);

  // Waiting for the attribute api to be fixed
  res = bobgui_accessible_text_get_attributes (BOBGUI_ACCESSIBLE_TEXT (label), 1, &n_ranges, &ranges, &attr_names, &attr_values);
  for (int i = 0; i < n_ranges; i++)
    g_print ("%s = %s\n", attr_names[i], attr_values[i]);
  g_assert_true (res);
  g_assert_cmpint (n_ranges, ==, 1);
  g_assert_cmpuint (ranges[0].start, ==, 1);
  g_assert_cmpuint (ranges[0].length, ==, 1);
  g_assert_cmpstr (attr_names[0], ==, BOBGUI_ACCESSIBLE_ATTRIBUTE_UNDERLINE);
  g_assert_cmpstr (attr_values[0], ==, BOBGUI_ACCESSIBLE_ATTRIBUTE_UNDERLINE_SINGLE);
  g_free (ranges);
  g_strfreev (attr_names);
  g_strfreev (attr_values);

  g_object_unref (label);
}

/* Some of the text interface functions require an allocated widget */
static void
more_label_text_interface (void)
{
  BobguiWidget *window, *label;
  int width, height;
  gboolean res;
  unsigned int offset;

  window = bobgui_window_new ();
  label = bobgui_label_new ("AAA");
  bobgui_widget_set_halign (label, BOBGUI_ALIGN_CENTER);
  bobgui_widget_set_valign (label, BOBGUI_ALIGN_CENTER);
  bobgui_window_set_child (BOBGUI_WINDOW (window), label);

  bobgui_window_present (BOBGUI_WINDOW (window));

  while (bobgui_widget_get_width (label) == 0)
    g_main_context_iteration (NULL, TRUE);

  width = bobgui_widget_get_width (label);
  height = bobgui_widget_get_height (label);
  g_assert_true (width > 0);
  g_assert_true (height > 0);

  res = bobgui_accessible_text_get_offset (BOBGUI_ACCESSIBLE_TEXT (label),
                                        &GRAPHENE_POINT_INIT (0, height / 2),
                                        &offset);
  g_assert_true (res);
  g_assert_cmpuint (offset, ==, 0);

  res = bobgui_accessible_text_get_offset (BOBGUI_ACCESSIBLE_TEXT (label),
                                        &GRAPHENE_POINT_INIT (width / 2 - 1, height / 2),
                                        &offset);
  g_assert_true (res);
  g_assert_cmpuint (offset, ==, 1);

  res = bobgui_accessible_text_get_offset (BOBGUI_ACCESSIBLE_TEXT (label),
                                        &GRAPHENE_POINT_INIT (width - width / 4, height / 2),
                                        &offset);
  g_assert_true (res);
  g_assert_cmpuint (offset, ==, 2);

  res = bobgui_accessible_text_get_offset (BOBGUI_ACCESSIBLE_TEXT (label),
                                        &GRAPHENE_POINT_INIT (width - width / 12, height / 2),
                                        &offset);
  g_assert_true (res);
  g_assert_cmpuint (offset, ==, 3);

  bobgui_window_destroy (BOBGUI_WINDOW (window));
}

int
main (int argc, char *argv[])
{
  bobgui_test_init (&argc, &argv, NULL);

  g_test_add_func ("/a11y/label/role", label_role);
  g_test_add_func ("/a11y/label/relations", label_relations);
  g_test_add_func ("/a11y/label/properties", label_properties);
  g_test_add_func ("/a11y/label/text-interface", label_text_interface);
  g_test_add_func ("/a11y/label/more-text-interface", more_label_text_interface);

  return g_test_run ();
}
