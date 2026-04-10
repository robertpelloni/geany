#include <bobgui/bobgui.h>
#include <glib/gstdio.h>

typedef struct {
  BobguiListBoxRow parent;
  BobguiWidget *box;
  BobguiWidget *revealer;
  BobguiWidget *check;
} SelectableRow;

typedef struct {
  BobguiListBoxRowClass parent_class;
} SelectableRowClass;

static GType selectable_row_get_type (void);
G_DEFINE_TYPE (SelectableRow, selectable_row, BOBGUI_TYPE_LIST_BOX_ROW)

static void
selectable_row_init (SelectableRow *row)
{
  row->box = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 10);  
  row->revealer = bobgui_revealer_new ();
  bobgui_revealer_set_transition_type (BOBGUI_REVEALER (row->revealer), BOBGUI_REVEALER_TRANSITION_TYPE_SLIDE_RIGHT);
  row->check = bobgui_check_button_new ();
  bobgui_widget_set_margin_start (row->check, 10);
  bobgui_widget_set_margin_end (row->check, 10);
  bobgui_widget_set_margin_top (row->check, 10);
  bobgui_widget_set_margin_bottom (row->check, 10);

  bobgui_list_box_row_set_child (BOBGUI_LIST_BOX_ROW (row), row->box);
  bobgui_box_append (BOBGUI_BOX (row->box), row->revealer);
  bobgui_revealer_set_child (BOBGUI_REVEALER (row->revealer), row->check);
}

static void
selectable_row_add (SelectableRow *row, BobguiWidget *child)
{
  bobgui_box_append (BOBGUI_BOX (row->box), child);
}

static void
update_selectable (BobguiWidget *widget)
{
  SelectableRow *row = (SelectableRow *)widget;
  BobguiListBox *list;

  list = BOBGUI_LIST_BOX (bobgui_widget_get_parent (widget));

  if (bobgui_list_box_get_selection_mode (list) != BOBGUI_SELECTION_NONE)
    bobgui_revealer_set_reveal_child (BOBGUI_REVEALER (row->revealer), TRUE);
  else
    bobgui_revealer_set_reveal_child (BOBGUI_REVEALER (row->revealer), FALSE);
}

static void
update_selected (BobguiWidget *widget)
{
  SelectableRow *row = (SelectableRow *)widget;

  if (bobgui_list_box_row_is_selected (BOBGUI_LIST_BOX_ROW (row)))
    {
      bobgui_check_button_set_active (BOBGUI_CHECK_BUTTON (row->check), TRUE);
      bobgui_widget_unset_state_flags (widget, BOBGUI_STATE_FLAG_SELECTED);
    }
  else
    bobgui_check_button_set_active (BOBGUI_CHECK_BUTTON (row->check), FALSE);
}

static void
selectable_row_class_init (SelectableRowClass *class)
{
}

static BobguiWidget *
selectable_row_new (void)
{
  return BOBGUI_WIDGET (g_object_new (selectable_row_get_type (), NULL));
}

static void
add_row (BobguiWidget *list, int i)
{
  BobguiWidget *row;
  BobguiWidget *label;
  char *text;

  row = selectable_row_new ();
  text = g_strdup_printf ("Docker %d", i);
  label = bobgui_label_new (text);
  bobgui_widget_set_halign (label, BOBGUI_ALIGN_START);
  selectable_row_add ((SelectableRow*)row, label);
  g_free (text);

  bobgui_list_box_insert (BOBGUI_LIST_BOX (list), row, -1);
}

static void
selection_mode_enter (BobguiButton *button, BobguiBuilder *builder)
{
  BobguiWidget *header;
  BobguiWidget *list;
  BobguiWidget *headerbutton;
  BobguiWidget *cancelbutton;
  BobguiWidget *selectbutton;
  BobguiWidget *titlestack;
  BobguiWidget *child;

  header = BOBGUI_WIDGET (bobgui_builder_get_object (builder, "header"));
  list = BOBGUI_WIDGET (bobgui_builder_get_object (builder, "list"));
  headerbutton = BOBGUI_WIDGET (bobgui_builder_get_object (builder, "headerbutton"));
  cancelbutton = BOBGUI_WIDGET (bobgui_builder_get_object (builder, "cancel-button"));
  selectbutton = BOBGUI_WIDGET (bobgui_builder_get_object (builder, "select-button"));
  titlestack = BOBGUI_WIDGET (bobgui_builder_get_object (builder, "titlestack"));
 
  bobgui_widget_add_css_class (header, "selection-mode");
  bobgui_header_bar_set_show_title_buttons (BOBGUI_HEADER_BAR (header), FALSE);
  bobgui_widget_set_visible (headerbutton, FALSE);
  bobgui_widget_set_visible (selectbutton, FALSE);
  bobgui_widget_set_visible (cancelbutton, TRUE);
  bobgui_stack_set_visible_child_name (BOBGUI_STACK (titlestack), "selection");

  bobgui_list_box_set_activate_on_single_click (BOBGUI_LIST_BOX (list), FALSE);
  bobgui_list_box_set_selection_mode (BOBGUI_LIST_BOX (list), BOBGUI_SELECTION_MULTIPLE);
  for (child = bobgui_widget_get_first_child (list);
       child != NULL;
       child = bobgui_widget_get_next_sibling (child))
    update_selectable (child);
}

static void
selection_mode_leave (BobguiButton *button, BobguiBuilder *builder)
{
  BobguiWidget *header;
  BobguiWidget *list;
  BobguiWidget *headerbutton;
  BobguiWidget *cancelbutton;
  BobguiWidget *selectbutton;
  BobguiWidget *titlestack;
  BobguiWidget *child;

  header = BOBGUI_WIDGET (bobgui_builder_get_object (builder, "header"));
  list = BOBGUI_WIDGET (bobgui_builder_get_object (builder, "list"));
  headerbutton = BOBGUI_WIDGET (bobgui_builder_get_object (builder, "headerbutton"));
  cancelbutton = BOBGUI_WIDGET (bobgui_builder_get_object (builder, "cancel-button"));
  selectbutton = BOBGUI_WIDGET (bobgui_builder_get_object (builder, "select-button"));
  titlestack = BOBGUI_WIDGET (bobgui_builder_get_object (builder, "titlestack"));

  bobgui_widget_remove_css_class (header, "selection-mode");
  bobgui_header_bar_set_show_title_buttons (BOBGUI_HEADER_BAR (header), TRUE);
  bobgui_widget_set_visible (headerbutton, TRUE);
  bobgui_widget_set_visible (selectbutton, TRUE);
  bobgui_widget_set_visible (cancelbutton, FALSE);
  bobgui_stack_set_visible_child_name (BOBGUI_STACK (titlestack), "title");

  bobgui_list_box_set_activate_on_single_click (BOBGUI_LIST_BOX (list), TRUE);
  bobgui_list_box_set_selection_mode (BOBGUI_LIST_BOX (list), BOBGUI_SELECTION_NONE);
  for (child = bobgui_widget_get_first_child (list);
       child != NULL;
       child = bobgui_widget_get_next_sibling (child))
    update_selectable (child);
}

static void
select_all (GAction *action, GVariant *param, BobguiWidget *list)
{
  bobgui_list_box_select_all (BOBGUI_LIST_BOX (list));
}

static void
select_none (GAction *action, GVariant *param, BobguiWidget *list)
{
  bobgui_list_box_unselect_all (BOBGUI_LIST_BOX (list));
}

static void
selected_rows_changed (BobguiListBox *list)
{
  BobguiWidget *child;

  for (child = bobgui_widget_get_first_child (BOBGUI_WIDGET (list));
       child != NULL;
       child = bobgui_widget_get_next_sibling (child))
    update_selected (child);
}

int
main (int argc, char *argv[])
{
  BobguiBuilder *builder;
  BobguiWidget *window;
  BobguiWidget *list;
  BobguiWidget *button;
  int i;
  GSimpleActionGroup *group;
  GSimpleAction *action;

#ifdef BOBGUI_SRCDIR
  g_chdir (BOBGUI_SRCDIR);
#endif

  bobgui_init ();

  builder = bobgui_builder_new_from_file ("selectionmode.ui");
  window = BOBGUI_WIDGET (bobgui_builder_get_object (builder, "window"));
  list = BOBGUI_WIDGET (bobgui_builder_get_object (builder, "list"));

  group = g_simple_action_group_new ();
  action = g_simple_action_new ("select-all", NULL);
  g_signal_connect (action, "activate", G_CALLBACK (select_all), list);
  g_action_map_add_action (G_ACTION_MAP (group), G_ACTION (action));

  action = g_simple_action_new ("select-none", NULL);
  g_signal_connect (action, "activate", G_CALLBACK (select_none), list);
  g_action_map_add_action (G_ACTION_MAP (group), G_ACTION (action));

  bobgui_widget_insert_action_group (window, "win", G_ACTION_GROUP (group));

  for (i = 0; i < 10; i++)
    add_row (list, i);

  button = BOBGUI_WIDGET (bobgui_builder_get_object (builder, "select-button"));
  g_signal_connect (button, "clicked", G_CALLBACK (selection_mode_enter), builder);
  button = BOBGUI_WIDGET (bobgui_builder_get_object (builder, "cancel-button"));
  g_signal_connect (button, "clicked", G_CALLBACK (selection_mode_leave), builder);

  g_signal_connect (list, "selected-rows-changed", G_CALLBACK (selected_rows_changed), NULL);

  bobgui_window_present (BOBGUI_WINDOW (window));
 
  while (TRUE)
    g_main_context_iteration (NULL, TRUE);

  return 0;
}
