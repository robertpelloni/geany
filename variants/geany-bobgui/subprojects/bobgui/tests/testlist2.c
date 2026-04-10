#include <bobgui/bobgui.h>

static void
row_unrevealed (GObject *revealer, GParamSpec *pspec, gpointer data)
{
  BobguiWidget *row, *list;

  row = bobgui_widget_get_parent (BOBGUI_WIDGET (revealer));
  list = bobgui_widget_get_parent (row);

  bobgui_list_box_remove (BOBGUI_LIST_BOX (list), row);
}

static void
remove_this_row (BobguiButton *button, BobguiWidget *child)
{
  BobguiWidget *row, *revealer;

  row = bobgui_widget_get_parent (child);
  revealer = bobgui_revealer_new ();
  bobgui_revealer_set_reveal_child (BOBGUI_REVEALER (revealer), TRUE);
  g_object_ref (child);
  bobgui_box_remove (BOBGUI_BOX (bobgui_widget_get_parent (child)), child);
  bobgui_revealer_set_child (BOBGUI_REVEALER (revealer), child);
  g_object_unref (child);
  bobgui_box_append (BOBGUI_BOX (row), revealer);
  g_signal_connect (revealer, "notify::child-revealed",
                    G_CALLBACK (row_unrevealed), NULL);
  bobgui_revealer_set_reveal_child (BOBGUI_REVEALER (revealer), FALSE);
}

static BobguiWidget *create_row (const char *label);

static void
row_revealed (GObject *revealer, GParamSpec *pspec, gpointer data)
{
  BobguiWidget *row, *child;

  row = bobgui_widget_get_parent (BOBGUI_WIDGET (revealer));
  child = bobgui_revealer_get_child (BOBGUI_REVEALER (revealer));
  g_object_ref (child);
  bobgui_revealer_set_child (BOBGUI_REVEALER (revealer), NULL);

  bobgui_widget_unparent (BOBGUI_WIDGET (revealer));
  bobgui_box_append (BOBGUI_BOX (row), child);
  g_object_unref (child);
}

static void
add_row_below (BobguiButton *button, BobguiWidget *child)
{
  BobguiWidget *revealer, *row, *list;
  int index;

  row = bobgui_widget_get_parent (child);
  index = bobgui_list_box_row_get_index (BOBGUI_LIST_BOX_ROW (row));
  list = bobgui_widget_get_parent (row);
  row = create_row ("Extra row");
  revealer = bobgui_revealer_new ();
  bobgui_revealer_set_child (BOBGUI_REVEALER (revealer), row);
  g_signal_connect (revealer, "notify::child-revealed",
                    G_CALLBACK (row_revealed), NULL);
  bobgui_list_box_insert (BOBGUI_LIST_BOX (list), revealer, index + 1);
  bobgui_revealer_set_reveal_child (BOBGUI_REVEALER (revealer), TRUE);
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
  BobguiWidget *row, *label, *button;

  row = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 10);
  label = bobgui_label_new (text);
  bobgui_box_append (BOBGUI_BOX (row), label);
  button = bobgui_button_new_with_label ("x");
  bobgui_widget_set_hexpand (button, TRUE);
  bobgui_widget_set_halign (button, BOBGUI_ALIGN_END);
  bobgui_widget_set_valign (button, BOBGUI_ALIGN_CENTER);
  bobgui_box_append (BOBGUI_BOX (row), button);
  g_signal_connect (button, "clicked", G_CALLBACK (remove_this_row), row);
  button = bobgui_button_new_with_label ("+");
  bobgui_widget_set_valign (button, BOBGUI_ALIGN_CENTER);
  bobgui_box_append (BOBGUI_BOX (row), button);
  g_signal_connect (button, "clicked", G_CALLBACK (add_row_below), row);

  return row;
}

static void
quit_cb (BobguiWidget *widget,
         gpointer   data)
{
  gboolean *done = data;

  *done = TRUE;

  g_main_context_wakeup (NULL);
}

int main (int argc, char *argv[])
{
  BobguiWidget *window, *list, *sw, *row;
  int i;
  char *text;
  gboolean done = FALSE;

  bobgui_init ();

  window = bobgui_window_new ();
  bobgui_window_set_default_size (BOBGUI_WINDOW (window), 300, 300);

  list = bobgui_list_box_new ();
  bobgui_list_box_set_selection_mode (BOBGUI_LIST_BOX (list), BOBGUI_SELECTION_NONE);
  bobgui_list_box_set_header_func (BOBGUI_LIST_BOX (list), add_separator, NULL, NULL);
  sw = bobgui_scrolled_window_new ();
  bobgui_window_set_child (BOBGUI_WINDOW (window), sw);
  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (sw), list);

  for (i = 0; i < 20; i++)
    {
      text = g_strdup_printf ("Row %d", i);
      row = create_row (text);
      bobgui_list_box_insert (BOBGUI_LIST_BOX (list), row, -1);
    }

  g_signal_connect (window, "destroy", G_CALLBACK (quit_cb), &done);
  bobgui_window_present (BOBGUI_WINDOW (window));

  while (!done)
    g_main_context_iteration (NULL, TRUE);

  return 0;
}
