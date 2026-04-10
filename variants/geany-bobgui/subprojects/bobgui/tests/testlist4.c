#include <bobgui/bobgui.h>

typedef struct
{
  BobguiApplication parent_instance;
} TestApp;

typedef BobguiApplicationClass TestAppClass;

static GType test_app_get_type (void);
G_DEFINE_TYPE (TestApp, test_app, BOBGUI_TYPE_APPLICATION)

static BobguiWidget *create_row (const char *label);

static void
activate_first_row (GSimpleAction *simple,
                    GVariant      *parameter,
                    gpointer       user_data)
{
  const char *text = "First row activated (no parameter action)";

  g_print ("%s\n", text);
  bobgui_label_set_label (BOBGUI_LABEL (user_data), text);
}

static void
activate_print_string (GSimpleAction *simple,
                       GVariant      *parameter,
                       gpointer       user_data)
{
  const char *text = g_variant_get_string (parameter, NULL);

  g_print ("%s\n", text);
  bobgui_label_set_label (BOBGUI_LABEL (user_data), text);
}

static void
activate_print_int (GSimpleAction *simple,
                    GVariant      *parameter,
                    gpointer       user_data)
{
  const int value = g_variant_get_int32 (parameter);
  char *text;

  text = g_strdup_printf ("Row %d activated (int action)", value);

  g_print ("%s\n", text);
  bobgui_label_set_label (BOBGUI_LABEL (user_data), text);
}

static void
row_without_gaction_activated_cb (BobguiListBox    *list,
                                  BobguiListBoxRow *row,
                                  gpointer       user_data)
{
  int index = bobgui_list_box_row_get_index (row);
  char *text;

  text = g_strdup_printf ("Row %d activated (signal based)", index);

  g_print ("%s\n", text);
  bobgui_label_set_label (BOBGUI_LABEL (user_data), text);
}

static void
add_separator (BobguiListBoxRow *row, BobguiListBoxRow *before, gpointer data)
{
  if (!before)
    return;

  bobgui_list_box_row_set_header (row, bobgui_separator_new (BOBGUI_ORIENTATION_HORIZONTAL));
}

static BobguiWidget *
create_row (const char *text)
{
  BobguiWidget *row_content, *label;

  row_content = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 10);

  label = bobgui_label_new (text);
  bobgui_box_append (BOBGUI_BOX (row_content), label);

  return row_content;
}

static void
new_window (GApplication *app)
{
  BobguiWidget *window, *grid, *sw, *list, *label;
  GSimpleAction *action;

  BobguiWidget *row_content;
  BobguiListBoxRow *row;

  int i;
  char *text, *text2;

  window = bobgui_application_window_new (BOBGUI_APPLICATION (app));
  bobgui_window_set_default_size (BOBGUI_WINDOW (window), 300, 300);

  /* widget creation */
  grid = bobgui_grid_new ();
  bobgui_window_set_child (BOBGUI_WINDOW (window), grid);
  sw = bobgui_scrolled_window_new ();
  bobgui_widget_set_hexpand (BOBGUI_WIDGET (sw), true);
  bobgui_widget_set_vexpand (BOBGUI_WIDGET (sw), true);
  bobgui_grid_attach (BOBGUI_GRID (grid), sw, 0, 0, 1, 1);

  list = bobgui_list_box_new ();
  bobgui_list_box_set_selection_mode (BOBGUI_LIST_BOX (list), BOBGUI_SELECTION_NONE);
  bobgui_list_box_set_header_func (BOBGUI_LIST_BOX (list), add_separator, NULL, NULL);
  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (sw), list);

  label = bobgui_label_new ("No row activated");
  bobgui_grid_attach (BOBGUI_GRID (grid), label, 0, 1, 1, 1);

  /* no parameter action row */
  action = g_simple_action_new ("first-row-action", NULL);
  g_action_map_add_action (G_ACTION_MAP (window), G_ACTION (action));

  row_content = create_row ("First row (no parameter action)");
  bobgui_list_box_insert (BOBGUI_LIST_BOX (list), row_content, -1);

  row = bobgui_list_box_get_row_at_index (BOBGUI_LIST_BOX (list), 0);
  bobgui_actionable_set_action_name (BOBGUI_ACTIONABLE (row), "win.first-row-action");

  g_signal_connect (action, "activate", (GCallback) activate_first_row, label);

  /* string action rows */
  action = g_simple_action_new ("print-string", G_VARIANT_TYPE_STRING);
  g_action_map_add_action (G_ACTION_MAP (window), G_ACTION (action));

  for (i = 1; i < 3; i++)
    {
      text = g_strdup_printf ("Row %d (string action)", i);
      row_content = create_row (text);
      bobgui_list_box_insert (BOBGUI_LIST_BOX (list), row_content, -1);

      row = bobgui_list_box_get_row_at_index (BOBGUI_LIST_BOX (list), i);
      text2 = g_strdup_printf ("Row %d activated (string action)", i);
      bobgui_actionable_set_action_target (BOBGUI_ACTIONABLE (row), "s", text2);
      bobgui_actionable_set_action_name (BOBGUI_ACTIONABLE (row), "win.print-string");
    }

  g_signal_connect (action, "activate", (GCallback) activate_print_string, label);

  /* int action rows */
  action = g_simple_action_new ("print-int", G_VARIANT_TYPE_INT32);
  g_action_map_add_action (G_ACTION_MAP (window), G_ACTION (action));

  for (i = 3; i < 5; i++)
    {
      text = g_strdup_printf ("Row %d (int action)", i);
      row_content = create_row (text);
      bobgui_list_box_insert (BOBGUI_LIST_BOX (list), row_content, -1);

      row = bobgui_list_box_get_row_at_index (BOBGUI_LIST_BOX (list), i);
      bobgui_actionable_set_action_target (BOBGUI_ACTIONABLE (row), "i", i);
      bobgui_actionable_set_action_name (BOBGUI_ACTIONABLE (row), "win.print-int");
    }

  g_signal_connect (action, "activate", (GCallback) activate_print_int, label);

  /* signal based row */
  for (i = 5; i < 7; i++)
    {
      text = g_strdup_printf ("Row %d (signal based)", i);
      row_content = create_row (text);
      bobgui_list_box_insert (BOBGUI_LIST_BOX (list), row_content, -1);
    }

  g_signal_connect (list, "row-activated",
                    G_CALLBACK (row_without_gaction_activated_cb), label);

  /* let the show begin */
  bobgui_window_present (BOBGUI_WINDOW (window));
}

static void
test_app_activate (GApplication *application)
{
  new_window (application);
}

static void
test_app_init (TestApp *app)
{
}

static void
test_app_class_init (TestAppClass *class)
{
  G_APPLICATION_CLASS (class)->activate = test_app_activate;
}

static TestApp *
test_app_new (void)
{
  TestApp *test_app;

  g_set_application_name ("Test List 4");

  test_app = g_object_new (test_app_get_type (),
                           "application-id", "org.bobgui.testlist4",
                           "flags", G_APPLICATION_DEFAULT_FLAGS,
                           NULL);

  return test_app;
}

int
main (int argc, char **argv)
{
  TestApp *test_app;
  int status;

  test_app = test_app_new ();
  status = g_application_run (G_APPLICATION (test_app), argc, argv);

  g_object_unref (test_app);
  return status;
}
