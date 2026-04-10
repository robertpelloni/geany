#include <bobgui/bobgui.h>

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

static BobguiWidget *
oriented_test_widget (const char *label, const char *color)
{
  BobguiWidget *box;
  BobguiWidget *widget;
  BobguiCssProvider *provider;
  char *data;

  widget = bobgui_label_new (label);
  box = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
  provider = bobgui_css_provider_new ();
  data = g_strdup_printf ("box { background: %s; }", color);
  bobgui_css_provider_load_from_data (provider, data, -1);
  bobgui_style_context_add_provider (bobgui_widget_get_style_context (box),
                                  BOBGUI_STYLE_PROVIDER (provider),
                                  BOBGUI_STYLE_PROVIDER_PRIORITY_APPLICATION);
  g_free (data);
  g_object_unref (provider);
  bobgui_box_append (BOBGUI_BOX (box), widget);

  return box;
}

static BobguiWidget *
test_widget (const char *label, const char *color)
{
  return oriented_test_widget (label, color);
}

static BobguiOrientation o;

static void
toggle_orientation (BobguiGestureClick *gesture,
                    guint            n_press,
                    double           x,
                    double           y,
                    BobguiGrid         *grid)
{
  o = 1 - o;
  bobgui_orientable_set_orientation (BOBGUI_ORIENTABLE (grid), o);
}

static void
simple_grid (void)
{
  BobguiWidget *window;
  BobguiWidget *grid;
  BobguiWidget *test1, *test2, *test3, *test4, *test5, *test6;
  BobguiGesture *gesture;

  window = bobgui_window_new ();
  bobgui_window_set_title (BOBGUI_WINDOW (window), "Orientation");
  grid = bobgui_grid_new ();
  bobgui_window_set_child (BOBGUI_WINDOW (window), grid);

  gesture = bobgui_gesture_click_new ();
  g_signal_connect (gesture, "pressed", G_CALLBACK (toggle_orientation), grid);
  bobgui_widget_add_controller (window, BOBGUI_EVENT_CONTROLLER (gesture));

  bobgui_grid_set_column_spacing (BOBGUI_GRID (grid), 5);
  bobgui_grid_set_row_spacing (BOBGUI_GRID (grid), 5);
  test1 = test_widget ("1", "red");
  bobgui_grid_attach (BOBGUI_GRID (grid), test1, 0, 0, 1, 1);
  test2 = test_widget ("2", "green");
  bobgui_grid_attach (BOBGUI_GRID (grid), test2, 1, 0, 1, 1);
  test3 = test_widget ("3", "blue");
  bobgui_grid_attach (BOBGUI_GRID (grid), test3, 2, 0, 1, 1);
  test4 = test_widget ("4", "green");
  bobgui_grid_attach (BOBGUI_GRID (grid), test4, 0, 1, 1, 1);
  bobgui_widget_set_vexpand (test4, TRUE);
  test5 = test_widget ("5", "blue");
  bobgui_grid_attach_next_to (BOBGUI_GRID (grid), test5, test4, BOBGUI_POS_RIGHT, 2, 1);
  test6 = test_widget ("6", "yellow");
  bobgui_grid_attach (BOBGUI_GRID (grid), test6, -1, 0, 1, 2);
  bobgui_widget_set_hexpand (test6, TRUE);

  g_assert (bobgui_grid_get_child_at (BOBGUI_GRID (grid), 0, -1) == NULL);
  g_assert (bobgui_grid_get_child_at (BOBGUI_GRID (grid), 0, 0) == test1);
  g_assert (bobgui_grid_get_child_at (BOBGUI_GRID (grid), 1, 0) == test2);
  g_assert (bobgui_grid_get_child_at (BOBGUI_GRID (grid), 0, 1) == test4);
  g_assert (bobgui_grid_get_child_at (BOBGUI_GRID (grid), -1, 0) == test6);
  g_assert (bobgui_grid_get_child_at (BOBGUI_GRID (grid), -1, 1) == test6);
  g_assert (bobgui_grid_get_child_at (BOBGUI_GRID (grid), -1, 2) == NULL);
  bobgui_window_present (BOBGUI_WINDOW (window));
}

static void
text_grid (void)
{
  BobguiWidget *window;
  BobguiWidget *grid;
  BobguiWidget *paned1;
  BobguiWidget *box;
  BobguiWidget *label;

  window = bobgui_window_new ();
  bobgui_window_set_title (BOBGUI_WINDOW (window), "Height-for-Width");
  paned1 = bobgui_paned_new (BOBGUI_ORIENTATION_HORIZONTAL);
  bobgui_window_set_child (BOBGUI_WINDOW (window), paned1);

  box = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
  bobgui_paned_set_start_child (BOBGUI_PANED (paned1), box);
  bobgui_paned_set_resize_start_child (BOBGUI_PANED (paned1), TRUE);
  bobgui_paned_set_shrink_start_child (BOBGUI_PANED (paned1), FALSE);
  bobgui_paned_set_end_child (BOBGUI_PANED (paned1), bobgui_label_new ("Space"));
  bobgui_paned_set_resize_end_child (BOBGUI_PANED (paned1), TRUE);
  bobgui_paned_set_shrink_end_child (BOBGUI_PANED (paned1), FALSE);

  grid = bobgui_grid_new ();
  bobgui_orientable_set_orientation (BOBGUI_ORIENTABLE (grid), BOBGUI_ORIENTATION_VERTICAL);
  bobgui_box_append (BOBGUI_BOX (box), bobgui_label_new ("Above"));
  bobgui_box_append (BOBGUI_BOX (box), bobgui_separator_new (BOBGUI_ORIENTATION_HORIZONTAL));
  bobgui_box_append (BOBGUI_BOX (box), grid);
  bobgui_box_append (BOBGUI_BOX (box), bobgui_separator_new (BOBGUI_ORIENTATION_HORIZONTAL));
  bobgui_box_append (BOBGUI_BOX (box), bobgui_label_new ("Below"));

  label = bobgui_label_new ("Some text that may wrap if it has to");
  bobgui_label_set_width_chars (BOBGUI_LABEL (label), 10);
  bobgui_label_set_wrap (BOBGUI_LABEL (label), TRUE);
  bobgui_grid_attach (BOBGUI_GRID (grid), label, 0, 0, 1, 1);

  bobgui_grid_attach (BOBGUI_GRID (grid), test_widget ("1", "red"), 1, 0, 1, 1);
  bobgui_grid_attach (BOBGUI_GRID (grid), test_widget ("2", "blue"), 0, 1, 1, 1);

  label = bobgui_label_new ("Some text that may wrap if it has to");
  bobgui_label_set_ellipsize (BOBGUI_LABEL (label), PANGO_ELLIPSIZE_END);
  bobgui_label_set_width_chars (BOBGUI_LABEL (label), 10);
  bobgui_grid_attach (BOBGUI_GRID (grid), label, 1, 1, 1, 1);

  bobgui_window_present (BOBGUI_WINDOW (window));
}

static void
box_comparison (void)
{
  BobguiWidget *window;
  BobguiWidget *vbox;
  BobguiWidget *box;
  BobguiWidget *label;
  BobguiWidget *grid;

  window = bobgui_window_new ();
  bobgui_window_set_title (BOBGUI_WINDOW (window), "Grid vs. Box");
  vbox = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 5);
  bobgui_window_set_child (BOBGUI_WINDOW (window), vbox);

  bobgui_box_append (BOBGUI_BOX (vbox), bobgui_label_new ("Above"));
  bobgui_box_append (BOBGUI_BOX (vbox), bobgui_separator_new (BOBGUI_ORIENTATION_HORIZONTAL));

  box = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
  bobgui_box_append (BOBGUI_BOX (vbox), box);

  bobgui_box_append (BOBGUI_BOX (box), test_widget ("1", "white"));

  label = bobgui_label_new ("Some ellipsizing text");
  bobgui_label_set_ellipsize (BOBGUI_LABEL (label), PANGO_ELLIPSIZE_END);
  bobgui_label_set_width_chars (BOBGUI_LABEL (label), 10);
  bobgui_box_append (BOBGUI_BOX (box), label);

  bobgui_box_append (BOBGUI_BOX (box), test_widget ("2", "green"));

  label = bobgui_label_new ("Some text that may wrap if needed");
  bobgui_label_set_wrap (BOBGUI_LABEL (label), TRUE);
  bobgui_label_set_width_chars (BOBGUI_LABEL (label), 10);
  bobgui_box_append (BOBGUI_BOX (box), label);

  bobgui_box_append (BOBGUI_BOX (box), test_widget ("3", "red"));

  grid = bobgui_grid_new ();
  bobgui_orientable_set_orientation (BOBGUI_ORIENTABLE (grid), BOBGUI_ORIENTATION_VERTICAL);
  bobgui_box_append (BOBGUI_BOX (vbox), bobgui_separator_new (BOBGUI_ORIENTATION_HORIZONTAL));
  bobgui_box_append (BOBGUI_BOX (vbox), grid);

  bobgui_grid_attach (BOBGUI_GRID (grid), test_widget ("1", "white"), 0, 0, 1, 1);

  label = bobgui_label_new ("Some ellipsizing text");
  bobgui_label_set_ellipsize (BOBGUI_LABEL (label), PANGO_ELLIPSIZE_END);
  bobgui_label_set_width_chars (BOBGUI_LABEL (label), 10);
  bobgui_grid_attach (BOBGUI_GRID (grid), label, 1, 0, 1, 1);
  bobgui_widget_set_hexpand (label, TRUE);

  bobgui_grid_attach (BOBGUI_GRID (grid), test_widget ("2", "green"), 2, 0, 1, 1);

  label = bobgui_label_new ("Some text that may wrap if needed");
  bobgui_label_set_wrap (BOBGUI_LABEL (label), TRUE);
  bobgui_label_set_width_chars (BOBGUI_LABEL (label), 10);
  bobgui_grid_attach (BOBGUI_GRID (grid), label, 3, 0, 1, 1);
  bobgui_widget_set_hexpand (label, TRUE);

  bobgui_grid_attach (BOBGUI_GRID (grid), test_widget ("3", "red"), 4, 0, 1, 1);

  bobgui_box_append (BOBGUI_BOX (vbox), bobgui_separator_new (BOBGUI_ORIENTATION_HORIZONTAL));
  bobgui_box_append (BOBGUI_BOX (vbox), bobgui_label_new ("Below"));

  bobgui_window_present (BOBGUI_WINDOW (window));
}

static void
empty_line (void)
{
  BobguiWidget *window;
  BobguiWidget *grid;
  BobguiWidget *child;

  window = bobgui_window_new ();
  bobgui_window_set_title (BOBGUI_WINDOW (window), "Empty row");
  grid = bobgui_grid_new ();
  bobgui_window_set_child (BOBGUI_WINDOW (window), grid);

  bobgui_grid_set_row_spacing (BOBGUI_GRID (grid), 10);
  bobgui_grid_set_column_spacing (BOBGUI_GRID (grid), 10);

  child = test_widget ("(0, 0)", "red");
  bobgui_grid_attach (BOBGUI_GRID (grid), child, 0, 0, 1, 1);
  bobgui_widget_set_hexpand (child, TRUE);
  bobgui_widget_set_vexpand (child, TRUE);

  bobgui_grid_attach (BOBGUI_GRID (grid), test_widget ("(0, 1)", "blue"), 0, 1, 1, 1);

  bobgui_grid_attach (BOBGUI_GRID (grid), test_widget ("(10, 0)", "green"), 10, 0, 1, 1);
  bobgui_grid_attach (BOBGUI_GRID (grid), test_widget ("(10, 1)", "magenta"), 10, 1, 1, 1);

  bobgui_window_present (BOBGUI_WINDOW (window));
}

static void
empty_grid (void)
{
  BobguiWidget *window;
  BobguiWidget *grid;
  BobguiWidget *child;

  window = bobgui_window_new ();
  bobgui_window_set_title (BOBGUI_WINDOW (window), "Empty grid");
  grid = bobgui_grid_new ();
  bobgui_window_set_child (BOBGUI_WINDOW (window), grid);

  bobgui_grid_set_row_spacing (BOBGUI_GRID (grid), 10);
  bobgui_grid_set_column_spacing (BOBGUI_GRID (grid), 10);
  bobgui_grid_set_row_homogeneous (BOBGUI_GRID (grid), TRUE);

  child = test_widget ("(0, 0)", "red");
  bobgui_grid_attach (BOBGUI_GRID (grid), child, 0, 0, 1, 1);
  bobgui_widget_set_hexpand (child, TRUE);
  bobgui_widget_set_vexpand (child, TRUE);

  bobgui_window_present (BOBGUI_WINDOW (window));
  bobgui_widget_hide (child);
}

static void
scrolling (void)
{
  BobguiWidget *window;
  BobguiWidget *sw;
  BobguiWidget *viewport;
  BobguiWidget *grid;
  BobguiWidget *child;
  int i;

  window = bobgui_window_new ();
  bobgui_window_set_title (BOBGUI_WINDOW (window), "Scrolling");
  sw = bobgui_scrolled_window_new ();
  viewport = bobgui_viewport_new (NULL, NULL);
  grid = bobgui_grid_new ();

  bobgui_window_set_child (BOBGUI_WINDOW (window), sw);
  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (sw), viewport);
  bobgui_viewport_set_child (BOBGUI_VIEWPORT (viewport), grid);

  child = oriented_test_widget ("#800080", "#800080");
  bobgui_grid_attach (BOBGUI_GRID (grid), child, 0, 0, 1, 1);
  bobgui_widget_set_hexpand (child, TRUE);
  bobgui_widget_set_vexpand (child, TRUE);

  for (i = 1; i < 16; i++)
    {
      char *color;
      color = g_strdup_printf ("#%02x00%02x", 128 + 8*i, 128 - 8*i);
      child = test_widget (color, color);
      bobgui_grid_attach (BOBGUI_GRID (grid), child, 0, i, i + 1, 1);
      bobgui_widget_set_hexpand (child, TRUE);
      g_free (color);
    }

  for (i = 1; i < 16; i++)
    {
      char *color;
      color = g_strdup_printf ("#%02x00%02x", 128 - 8*i, 128 + 8*i);
      child = oriented_test_widget (color, color);
      bobgui_grid_attach (BOBGUI_GRID (grid), child, i, 0, 1, i);
      bobgui_widget_set_vexpand (child, TRUE);
      g_free (color);
    }

  bobgui_window_present (BOBGUI_WINDOW (window));
}

static void
insert_cb (BobguiButton *button, BobguiWidget *window)
{
  BobguiGrid *g, *g1, *g2, *g3, *g4;
  BobguiWidget *child;
  gboolean inserted;

  g = BOBGUI_GRID (bobgui_window_get_child (BOBGUI_WINDOW (window)));
  g1 = BOBGUI_GRID (bobgui_grid_get_child_at (g, 0, 0));
  g2 = BOBGUI_GRID (bobgui_grid_get_child_at (g, 1, 0));
  g3 = BOBGUI_GRID (bobgui_grid_get_child_at (g, 0, 1));
  g4 = BOBGUI_GRID (bobgui_grid_get_child_at (g, 1, 1));

  inserted = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (button), "inserted"));

  if (inserted)
    {
      bobgui_grid_remove_row (g1, 1);
      bobgui_grid_remove_column (g2, 1);
      bobgui_grid_remove_row (g3, 1);
      bobgui_grid_remove_column (g4, 1);
    }
  else
    {
      bobgui_grid_insert_row (g1, 1);
      bobgui_grid_attach (g1, test_widget ("(0, 1)", "red"), 0, 1, 1, 1);
      bobgui_grid_attach (g1, test_widget ("(2, 1)", "red"), 2, 1, 1, 1);

      bobgui_grid_insert_column (g2, 1);
      bobgui_grid_attach (g2, test_widget ("(1, 0)", "red"), 1, 0, 1, 1);
      bobgui_grid_attach (g2, test_widget ("(1, 2)", "red"), 1, 2, 1, 1);

      child = bobgui_grid_get_child_at (g3, 0, 0);
      bobgui_grid_insert_next_to (g3, child, BOBGUI_POS_BOTTOM);
      bobgui_grid_attach (g3, test_widget ("(0, 1)", "red"), 0, 1, 1, 1);
      bobgui_grid_attach (g3, test_widget ("(2, 1)", "red"), 2, 1, 1, 1);

      child = bobgui_grid_get_child_at (g4, 0, 0);
      bobgui_grid_insert_next_to (g4, child, BOBGUI_POS_RIGHT);
      bobgui_grid_attach (g4, test_widget ("(1, 0)", "red"), 1, 0, 1, 1);
      bobgui_grid_attach (g4, test_widget ("(1, 2)", "red"), 1, 2, 1, 1);
    }

  bobgui_button_set_label (button, inserted ? "Insert" : "Remove");
  g_object_set_data (G_OBJECT (button), "inserted", GINT_TO_POINTER (!inserted));
}

static void
insert (void)
{
  BobguiWidget *window;
  BobguiWidget *g;
  BobguiWidget *grid;
  BobguiWidget *child;
  BobguiWidget *button;

  window = bobgui_window_new ();
  bobgui_window_set_title (BOBGUI_WINDOW (window), "Insertion / Removal");

  g = bobgui_grid_new ();
  bobgui_grid_set_row_spacing (BOBGUI_GRID (g), 10);
  bobgui_grid_set_column_spacing (BOBGUI_GRID (g), 10);
  bobgui_window_set_child (BOBGUI_WINDOW (window), g);

  grid = bobgui_grid_new ();
  bobgui_grid_attach (BOBGUI_GRID (g), grid, 0, 0, 1, 1);

  bobgui_grid_attach (BOBGUI_GRID (grid), test_widget ("(0, 0)", "blue"), 0, 0, 1, 1);
  bobgui_grid_attach (BOBGUI_GRID (grid), test_widget ("(0, 1)", "blue"), 0, 1, 1, 1);
  bobgui_grid_attach (BOBGUI_GRID (grid), test_widget ("(1, 0)", "green"), 1, 0, 1, 2);
  bobgui_grid_attach (BOBGUI_GRID (grid), test_widget ("(2, 0)", "yellow"), 2, 0, 1, 1);
  bobgui_grid_attach (BOBGUI_GRID (grid), test_widget ("(2, 1)", "yellow"), 2, 1, 1, 1);

  grid = bobgui_grid_new ();
  bobgui_grid_attach (BOBGUI_GRID (g), grid, 1, 0, 1, 1);

  bobgui_grid_attach (BOBGUI_GRID (grid), test_widget ("(0, 0)", "blue"), 0, 0, 1, 1);
  bobgui_grid_attach (BOBGUI_GRID (grid), test_widget ("(1, 0)", "blue"), 1, 0, 1, 1);
  bobgui_grid_attach (BOBGUI_GRID (grid), test_widget ("(0, 1)", "green"), 0, 1, 2, 1);
  bobgui_grid_attach (BOBGUI_GRID (grid), test_widget ("(0, 2)", "yellow"), 0, 2, 1, 1);
  bobgui_grid_attach (BOBGUI_GRID (grid), test_widget ("(1, 2)", "yellow"), 1, 2, 1, 1);

  grid = bobgui_grid_new ();
  bobgui_grid_attach (BOBGUI_GRID (g), grid, 0, 1, 1, 1);

  child = test_widget ("(0, 0)", "blue");
  bobgui_grid_attach (BOBGUI_GRID (grid), child, 0, 0, 1, 1);
  bobgui_grid_attach (BOBGUI_GRID (grid), test_widget ("(0, 1)", "blue"), 0, 1, 1, 1);
  bobgui_grid_attach (BOBGUI_GRID (grid), test_widget ("(1, 0)", "green"), 1, 0, 1, 2);
  bobgui_grid_attach (BOBGUI_GRID (grid), test_widget ("(2, 0)", "yellow"), 2, 0, 1, 1);
  bobgui_grid_attach (BOBGUI_GRID (grid), test_widget ("(2, 1)", "yellow"), 2, 1, 1, 1);

  grid = bobgui_grid_new ();
  bobgui_grid_attach (BOBGUI_GRID (g), grid, 1, 1, 1, 1);

  child = test_widget ("(0, 0)", "blue");
  bobgui_grid_attach (BOBGUI_GRID (grid), child, 0, 0, 1, 1);
  bobgui_grid_attach (BOBGUI_GRID (grid), test_widget ("(1, 0)", "blue"), 1, 0, 1, 1);
  bobgui_grid_attach (BOBGUI_GRID (grid), test_widget ("(0, 1)", "green"), 0, 1, 2, 1);
  bobgui_grid_attach (BOBGUI_GRID (grid), test_widget ("(0, 2)", "yellow"), 0, 2, 1, 1);
  bobgui_grid_attach (BOBGUI_GRID (grid), test_widget ("(1, 2)", "yellow"), 1, 2, 1, 1);

  button = bobgui_button_new_with_label ("Insert");
  g_signal_connect (button, "clicked", G_CALLBACK (insert_cb), window);
  bobgui_grid_attach (BOBGUI_GRID (g), button, 0, 2, 2, 1);

  bobgui_window_present (BOBGUI_WINDOW (window));
}

static void
spanning_grid (void)
{
  BobguiWidget *window;
  BobguiWidget *g;
  BobguiWidget *c;

  /* inspired by bug 698660
   * the row/column that are empty except for the spanning
   * child need to stay collapsed
   */

  window = bobgui_window_new ();
  bobgui_window_set_title (BOBGUI_WINDOW (window), "Spanning");

  g = bobgui_grid_new ();
  bobgui_window_set_child (BOBGUI_WINDOW (window), g);

  c = test_widget ("0", "blue");
  bobgui_widget_set_hexpand (c, TRUE);
  bobgui_grid_attach (BOBGUI_GRID (g), c, 0, 4, 4, 1);

  c = test_widget ("1", "green");
  bobgui_widget_set_vexpand (c, TRUE);
  bobgui_grid_attach (BOBGUI_GRID (g), c, 4, 0, 1, 4);

  c = test_widget ("2", "red");
  bobgui_widget_set_hexpand (c, TRUE);
  bobgui_widget_set_vexpand (c, TRUE);
  bobgui_grid_attach (BOBGUI_GRID (g), c, 3, 3, 1, 1);

  c = test_widget ("3", "yellow");
  bobgui_grid_attach (BOBGUI_GRID (g), c, 0, 3, 2, 1);

  c = test_widget ("4", "orange");
  bobgui_grid_attach (BOBGUI_GRID (g), c, 3, 0, 1, 2);

  c = test_widget ("5", "purple");
  bobgui_grid_attach (BOBGUI_GRID (g), c, 1, 1, 1, 1);

  c = test_widget ("6", "white");
  bobgui_grid_attach (BOBGUI_GRID (g), c, 0, 1, 1, 1);

  c = test_widget ("7", "cyan");
  bobgui_grid_attach (BOBGUI_GRID (g), c, 1, 0, 1, 1);

  bobgui_window_present (BOBGUI_WINDOW (window));
}

int
main (int argc, char *argv[])
{
  bobgui_init ();

  if (g_getenv ("RTL"))
    bobgui_widget_set_default_direction (BOBGUI_TEXT_DIR_RTL);

  simple_grid ();
  text_grid ();
  box_comparison ();
  empty_line ();
  scrolling ();
  insert ();
  empty_grid ();
  spanning_grid ();

  while (TRUE)
    g_main_context_iteration (NULL, TRUE);

  return 0;
}
