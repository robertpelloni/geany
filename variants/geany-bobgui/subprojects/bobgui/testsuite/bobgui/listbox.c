#include <bobgui/bobgui.h>

static int
sort_list (BobguiListBoxRow *row1,
           BobguiListBoxRow *row2,
           gpointer       data)
{
  BobguiWidget *label1, *label2;
  int n1, n2;
  int *count = data;

  (*count)++;

  label1 = bobgui_list_box_row_get_child (row1);
  n1 = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (label1), "data"));

  label2 = bobgui_list_box_row_get_child (row2);
  n2 = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (label2), "data"));

  return (n1 - n2);
}

static void
check_sorted (BobguiListBox *list)
{
  BobguiWidget *row, *label;
  int res[100];
  int index, value;
  int n_rows = 0;
  int i;

  for (row = bobgui_widget_get_first_child (BOBGUI_WIDGET (list));
       row != NULL;
       row = bobgui_widget_get_next_sibling (row))
    {
      if (!BOBGUI_IS_LIST_BOX_ROW (row))
        continue;

      index = bobgui_list_box_row_get_index (BOBGUI_LIST_BOX_ROW (row));
      label = bobgui_list_box_row_get_child (BOBGUI_LIST_BOX_ROW (row));
      value = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (label), "data"));
      res[index] = value;
      n_rows++;
    }

  for (i = 1; i < n_rows; i++)
    g_assert_cmpint (res[i - 1], <=, res[i]);
}

static void
test_sort (void)
{
  BobguiListBox *list;
  BobguiListBoxRow *row;
  BobguiWidget *label;
  int i, r;
  char *s;
  int count;

  list = BOBGUI_LIST_BOX (bobgui_list_box_new ());
  g_object_ref_sink (list);

  for (i = 0; i < 100; i++)
    {
      r = g_test_rand_int_range (0, 1000);
      s = g_strdup_printf ("%d: %d", i, r);
      label = bobgui_label_new (s);
      g_object_set_data (G_OBJECT (label), "data", GINT_TO_POINTER (r));
      g_free (s);
      bobgui_list_box_insert (BOBGUI_LIST_BOX (list), label, -1);
    }

  count = 0;
  bobgui_list_box_set_sort_func (list, sort_list, &count, NULL);
  g_assert_cmpint (count, >, 0);

  check_sorted (list);

  count = 0;
  bobgui_list_box_invalidate_sort (list);
  g_assert_cmpint (count, >, 0);

  count = 0;
  row = bobgui_list_box_get_row_at_index (list, 0);
  bobgui_list_box_row_changed (row);
  g_assert_cmpint (count, >, 0);

  g_object_unref (list);
}

static BobguiListBoxRow *callback_row;

static void
on_row_selected (BobguiListBox    *list_box,
                 BobguiListBoxRow *row,
                 gpointer       data)
{
  int *i = data;

  (*i)++;

  callback_row = row;
}

static void
test_selection (void)
{
  BobguiListBox *list;
  BobguiListBoxRow *row, *row2;
  BobguiWidget *label;
  int i;
  char *s;
  int count;
  int index;

  list = BOBGUI_LIST_BOX (bobgui_list_box_new ());
  g_object_ref_sink (list);

  g_assert_cmpint (bobgui_list_box_get_selection_mode (list), ==, BOBGUI_SELECTION_SINGLE);
  g_assert_null (bobgui_list_box_get_selected_row (list));

  for (i = 0; i < 100; i++)
    {
      s = g_strdup_printf ("%d", i);
      label = bobgui_label_new (s);
      g_object_set_data (G_OBJECT (label), "data", GINT_TO_POINTER (i));
      g_free (s);
      bobgui_list_box_insert (BOBGUI_LIST_BOX (list), label, -1);
    }

  count = 0;
  g_signal_connect (list, "row-selected",
                    G_CALLBACK (on_row_selected),
                    &count);

  row = bobgui_list_box_get_row_at_index (list, 20);
  g_assert_false (bobgui_list_box_row_is_selected (row));
  bobgui_list_box_select_row (list, row);
  g_assert_true (bobgui_list_box_row_is_selected (row));
  g_assert_true (callback_row == row);
  g_assert_cmpint (count, ==, 1);
  row2 = bobgui_list_box_get_selected_row (list);
  g_assert_true (row2 == row);
  bobgui_list_box_unselect_all (list);
  row2 = bobgui_list_box_get_selected_row (list);
  g_assert_null (row2);
  bobgui_list_box_select_row (list, row);
  row2 = bobgui_list_box_get_selected_row (list);
  g_assert_true (row2 == row);

  bobgui_list_box_set_selection_mode (list, BOBGUI_SELECTION_BROWSE);
  bobgui_list_box_remove (BOBGUI_LIST_BOX (list), BOBGUI_WIDGET (row));
  g_assert_null (callback_row);
  g_assert_cmpint (count, ==, 4);
  row2 = bobgui_list_box_get_selected_row (list);
  g_assert_null (row2);

  row = bobgui_list_box_get_row_at_index (list, 20);
  bobgui_list_box_select_row (list, row);
  g_assert_true (bobgui_list_box_row_is_selected (row));
  g_assert_true (callback_row == row);
  g_assert_cmpint (count, ==, 5);

  bobgui_list_box_set_selection_mode (list, BOBGUI_SELECTION_NONE);
  g_assert_false (bobgui_list_box_row_is_selected (row));
  g_assert_null (callback_row);
  g_assert_cmpint (count, ==, 6);
  row2 = bobgui_list_box_get_selected_row (list);
  g_assert_null (row2);

  row = bobgui_list_box_get_row_at_index (list, 20);
  index = bobgui_list_box_row_get_index (row);
  g_assert_cmpint (index, ==, 20);

  row = BOBGUI_LIST_BOX_ROW (bobgui_list_box_row_new ());
  g_object_ref_sink (row);
  index = bobgui_list_box_row_get_index (row);
  g_assert_cmpint (index, ==, -1);
  g_object_unref (row);

  g_object_unref (list);
}

static void
on_selected_rows_changed (BobguiListBox *box, gpointer data)
{
  int *i = data;

  (*i)++;
}

static void
test_multi_selection (void)
{
  BobguiListBox *list;
  GList *l;
  BobguiListBoxRow *row, *row2;
  BobguiWidget *label;
  int i;
  char *s;
  int count;

  list = BOBGUI_LIST_BOX (bobgui_list_box_new ());
  g_object_ref_sink (list);

  g_assert_cmpint (bobgui_list_box_get_selection_mode (list), ==, BOBGUI_SELECTION_SINGLE);
  g_assert_null (bobgui_list_box_get_selected_rows (list));

  bobgui_list_box_set_selection_mode (list, BOBGUI_SELECTION_MULTIPLE);

  for (i = 0; i < 100; i++)
    {
      s = g_strdup_printf ("%d", i);
      label = bobgui_label_new (s);
      g_object_set_data (G_OBJECT (label), "data", GINT_TO_POINTER (i));
      g_free (s);
      bobgui_list_box_insert (BOBGUI_LIST_BOX (list), label, -1);
    }

  count = 0;
  g_signal_connect (list, "selected-rows-changed",
                    G_CALLBACK (on_selected_rows_changed),
                    &count);

  row = bobgui_list_box_get_row_at_index (list, 20);

  bobgui_list_box_select_all (list);
  g_assert_cmpint (count, ==, 1);
  l = bobgui_list_box_get_selected_rows (list);
  g_assert_cmpint (g_list_length (l), ==, 100);
  g_list_free (l);
  g_assert_true (bobgui_list_box_row_is_selected (row));

  bobgui_list_box_unselect_all (list);
  g_assert_cmpint (count, ==, 2);
  l = bobgui_list_box_get_selected_rows (list);
  g_assert_null (l);
  g_assert_false (bobgui_list_box_row_is_selected (row));

  bobgui_list_box_select_row (list, row);
  g_assert_true (bobgui_list_box_row_is_selected (row));
  g_assert_cmpint (count, ==, 3);
  l = bobgui_list_box_get_selected_rows (list);
  g_assert_cmpint (g_list_length (l), ==, 1);
  g_assert_true (l->data == row);
  g_list_free (l);

  row2 = bobgui_list_box_get_row_at_index (list, 40);
  g_assert_false (bobgui_list_box_row_is_selected (row2));
  bobgui_list_box_select_row (list, row2);
  g_assert_true (bobgui_list_box_row_is_selected (row2));
  g_assert_cmpint (count, ==, 4);
  l = bobgui_list_box_get_selected_rows (list);
  g_assert_cmpint (g_list_length (l), ==, 2);
  g_assert_true (l->data == row);
  g_assert_true (l->next->data == row2);
  g_list_free (l);

  bobgui_list_box_unselect_row (list, row);
  g_assert_false (bobgui_list_box_row_is_selected (row));
  g_assert_cmpint (count, ==, 5);
  l = bobgui_list_box_get_selected_rows (list);
  g_assert_cmpint (g_list_length (l), ==, 1);
  g_assert_true (l->data == row2);
  g_list_free (l);

  g_object_unref (list);
}

static gboolean
filter_func (BobguiListBoxRow *row,
             gpointer       data)
{
  int *count = data;
  BobguiWidget *child;
  int i;

  (*count)++;

  child = bobgui_list_box_row_get_child (row);
  i = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (child), "data"));

  return (i % 2) == 0;
}

static void
check_filtered (BobguiListBox *list)
{
  int count;
  BobguiWidget *row;

  count = 0;
  for (row = bobgui_widget_get_first_child (BOBGUI_WIDGET (list));
       row != NULL;
       row = bobgui_widget_get_next_sibling (row))
    {
      if (!BOBGUI_IS_LIST_BOX_ROW (row))
        continue;

      if (bobgui_widget_get_child_visible (row))
        count++;
    }
  g_assert_cmpint (count, ==, 50);
}

static void
test_filter (void)
{
  BobguiListBox *list;
  BobguiListBoxRow *row;
  int i;
  char *s;
  BobguiWidget *label;
  int count;

  list = BOBGUI_LIST_BOX (bobgui_list_box_new ());
  g_object_ref_sink (list);

  g_assert_cmpint (bobgui_list_box_get_selection_mode (list), ==, BOBGUI_SELECTION_SINGLE);
  g_assert_null (bobgui_list_box_get_selected_row (list));

  for (i = 0; i < 100; i++)
    {
      s = g_strdup_printf ("%d", i);
      label = bobgui_label_new (s);
      g_object_set_data (G_OBJECT (label), "data", GINT_TO_POINTER (i));
      g_free (s);
      bobgui_list_box_insert (BOBGUI_LIST_BOX (list), label, -1);
    }

  count = 0;
  bobgui_list_box_set_filter_func (list, filter_func, &count, NULL);
  g_assert_cmpint (count, >, 0);

  check_filtered (list);

  count = 0;
  bobgui_list_box_invalidate_filter (list);
  g_assert_cmpint (count, >, 0);

  count = 0;
  row = bobgui_list_box_get_row_at_index (list, 0);
  bobgui_list_box_row_changed (row);
  g_assert_cmpint (count, >, 0);

  g_object_unref (list);
}

static void
header_func (BobguiListBoxRow *row,
             BobguiListBoxRow *before,
             gpointer       data)
{
  BobguiWidget *child;
  int i;
  int *count = data;
  BobguiWidget *header;
  char *s;

  (*count)++;

  child = bobgui_list_box_row_get_child (row);
  i = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (child), "data"));

  if (i % 2 == 0)
    {
      s = g_strdup_printf ("Header %d", i);
      header = bobgui_label_new (s);
      g_free (s);
    }
  else
    header = NULL;

  bobgui_list_box_row_set_header (row, header);
}

static void
check_headers (BobguiListBox *list)
{
  int count;
  BobguiWidget *row;

  count = 0;
  for (row = bobgui_widget_get_first_child (BOBGUI_WIDGET (list));
       row != NULL;
       row = bobgui_widget_get_next_sibling (row))
    {
      if (!BOBGUI_IS_LIST_BOX_ROW (row))
        continue;

      if (bobgui_list_box_row_get_header (BOBGUI_LIST_BOX_ROW (row)) != NULL)
        count++;
    }
  g_assert_cmpint (count, ==, 50);
}

static void
test_header (void)
{
  BobguiListBox *list;
  BobguiListBoxRow *row;
  int i;
  char *s;
  BobguiWidget *label;
  int count;

  list = BOBGUI_LIST_BOX (bobgui_list_box_new ());
  g_object_ref_sink (list);

  g_assert_cmpint (bobgui_list_box_get_selection_mode (list), ==, BOBGUI_SELECTION_SINGLE);
  g_assert_null (bobgui_list_box_get_selected_row (list));

  for (i = 0; i < 100; i++)
    {
      s = g_strdup_printf ("%d", i);
      label = bobgui_label_new (s);
      g_object_set_data (G_OBJECT (label), "data", GINT_TO_POINTER (i));
      g_free (s);
      bobgui_list_box_insert (BOBGUI_LIST_BOX (list), label, -1);
    }

  count = 0;
  bobgui_list_box_set_header_func (list, header_func, &count, NULL);
  g_assert_cmpint (count, >, 0);

  check_headers (list);

  count = 0;
  bobgui_list_box_invalidate_headers (list);
  g_assert_cmpint (count, >, 0);

  count = 0;
  row = bobgui_list_box_get_row_at_index (list, 0);
  bobgui_list_box_row_changed (row);
  g_assert_cmpint (count, >, 0);

  g_object_unref (list);
}

int
main (int argc, char *argv[])
{
  bobgui_test_init (&argc, &argv);

  g_test_add_func ("/listbox/sort", test_sort);
  g_test_add_func ("/listbox/selection", test_selection);
  g_test_add_func ("/listbox/multi-selection", test_multi_selection);
  g_test_add_func ("/listbox/filter", test_filter);
  g_test_add_func ("/listbox/header", test_header);

  return g_test_run ();
}
