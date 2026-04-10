#include <bobgui/bobgui.h>

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

BobguiWidget *stack;
BobguiWidget *switcher;
BobguiWidget *sidebar;
BobguiWidget *w1;

static void
set_visible_child (BobguiWidget *button, gpointer data)
{
  bobgui_stack_set_visible_child (BOBGUI_STACK (stack), BOBGUI_WIDGET (data));
}

static void
set_visible_child_name (BobguiWidget *button, gpointer data)
{
  bobgui_stack_set_visible_child_name (BOBGUI_STACK (stack), (const char *)data);
}

static void
toggle_hhomogeneous (BobguiWidget *button, gpointer data)
{
  gboolean active = bobgui_check_button_get_active (BOBGUI_CHECK_BUTTON (button));
  bobgui_stack_set_hhomogeneous (BOBGUI_STACK (stack), active);
}

static void
toggle_vhomogeneous (BobguiWidget *button, gpointer data)
{
  gboolean active = bobgui_check_button_get_active (BOBGUI_CHECK_BUTTON (button));
  bobgui_stack_set_vhomogeneous (BOBGUI_STACK (stack), active);
}

static void
toggle_icon_name (BobguiWidget *button, gpointer data)
{
  gboolean active = bobgui_toggle_button_get_active (BOBGUI_TOGGLE_BUTTON (button));
  BobguiStackPage *page;

  page = bobgui_stack_get_page (BOBGUI_STACK (stack), w1);
  g_object_set (page, "icon-name", active ? "edit-find-symbolic" : NULL, NULL);
}

static void
toggle_transitions (BobguiWidget *combo, gpointer data)
{
  int id = bobgui_combo_box_get_active (BOBGUI_COMBO_BOX (combo));
  bobgui_stack_set_transition_type (BOBGUI_STACK (stack), id);
}

static void
on_back_button_clicked (BobguiButton *button, gpointer user_data)
{
  const char *seq[] = { "1", "2", "3" };
  const char *vis;
  int i;

  vis = bobgui_stack_get_visible_child_name (BOBGUI_STACK (stack));

  for (i = 1; i < G_N_ELEMENTS (seq); i++)
    {
      if (g_strcmp0 (vis, seq[i]) == 0)
        {
          bobgui_stack_set_visible_child_full (BOBGUI_STACK (stack), seq[i - 1], BOBGUI_STACK_TRANSITION_TYPE_SLIDE_RIGHT);
          break;
        }
    }
}

static void
on_forward_button_clicked (BobguiButton *button, gpointer user_data)
{
  const char *seq[] = { "1", "2", "3" };
  const char *vis;
  int i;

  vis = bobgui_stack_get_visible_child_name (BOBGUI_STACK (stack));

  for (i = 0; i < G_N_ELEMENTS (seq) - 1; i++)
    {
      if (g_strcmp0 (vis, seq[i]) == 0)
        {
          bobgui_stack_set_visible_child_full (BOBGUI_STACK (stack), seq[i + 1], BOBGUI_STACK_TRANSITION_TYPE_SLIDE_LEFT);
          break;
        }
    }
}

static void
update_back_button_sensitivity (BobguiStack *_stack, GParamSpec *pspec, BobguiWidget *button)
{
  const char *vis;

  vis = bobgui_stack_get_visible_child_name (BOBGUI_STACK (stack));
  bobgui_widget_set_sensitive (button, g_strcmp0 (vis, "1") != 0);
}

static void
update_forward_button_sensitivity (BobguiStack *_stack, GParamSpec *pspec, BobguiWidget *button)
{
  const char *vis;

  vis = bobgui_stack_get_visible_child_name (BOBGUI_STACK (stack));
  bobgui_widget_set_sensitive (button, g_strcmp0 (vis, "3") != 0);
}

int
main (int argc,
      char ** argv)
{
  BobguiWidget *window, *box, *button, *hbox, *combo, *layout;
  BobguiWidget *w2, *w3;
  BobguiListStore* store;
  BobguiWidget *tree_view;
  BobguiTreeViewColumn *column;
  BobguiCellRenderer *renderer;
  BobguiWidget *scrolled_win;
  int i;
  BobguiTreeIter iter;
  GEnumClass *class;
  BobguiStackPage *page;

  bobgui_init ();

  window = bobgui_window_new ();
  bobgui_widget_set_size_request (window, 300, 300);

  box = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
  bobgui_window_set_child (BOBGUI_WINDOW (window), box);

  switcher = bobgui_stack_switcher_new ();
  bobgui_box_append (BOBGUI_BOX (box), switcher);

  stack = bobgui_stack_new ();

  /* Make transitions longer so we can see that they work */
  bobgui_stack_set_transition_duration (BOBGUI_STACK (stack), 1500);

  bobgui_widget_set_halign (stack, BOBGUI_ALIGN_START);
  bobgui_widget_set_vexpand (stack, TRUE);

  /* Add sidebar before stack */
  sidebar = bobgui_stack_sidebar_new ();
  bobgui_stack_sidebar_set_stack (BOBGUI_STACK_SIDEBAR (sidebar), BOBGUI_STACK (stack));
  layout = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
  bobgui_box_append (BOBGUI_BOX (layout), sidebar);
  bobgui_widget_set_hexpand (stack, TRUE);
  bobgui_box_append (BOBGUI_BOX (layout), stack);

  bobgui_box_append (BOBGUI_BOX (box), layout);

  bobgui_stack_switcher_set_stack (BOBGUI_STACK_SWITCHER (switcher), BOBGUI_STACK (stack));

  w1 = bobgui_text_view_new ();
  bobgui_text_buffer_set_text (bobgui_text_view_get_buffer (BOBGUI_TEXT_VIEW (w1)),
			    "This is a\nTest\nBalh!", -1);

  bobgui_stack_add_titled (BOBGUI_STACK (stack), w1, "1", "1");

  w2 = bobgui_button_new_with_label ("Gazoooooooooooooooonk");
  bobgui_stack_add_titled (BOBGUI_STACK (stack), w2, "2", "2");
  page = bobgui_stack_get_page (BOBGUI_STACK (stack), w2);
  g_object_set (page, "needs-attention", TRUE, NULL);


  scrolled_win = bobgui_scrolled_window_new ();
  bobgui_scrolled_window_set_policy (BOBGUI_SCROLLED_WINDOW (scrolled_win),
				  BOBGUI_POLICY_AUTOMATIC,
				  BOBGUI_POLICY_AUTOMATIC);
  bobgui_widget_set_size_request (scrolled_win, 100, 200);


  store = bobgui_list_store_new (1, G_TYPE_STRING);

  for (i = 0; i < 40; i++)
    bobgui_list_store_insert_with_values (store, &iter, i, 0,  "Testvalule", -1);

  tree_view = bobgui_tree_view_new_with_model (BOBGUI_TREE_MODEL (store));

  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (scrolled_win), tree_view);
  w3 = scrolled_win;

  renderer = bobgui_cell_renderer_text_new ();
  column = bobgui_tree_view_column_new_with_attributes ("Target", renderer,
						     "text", 0, NULL);
  bobgui_tree_view_append_column (BOBGUI_TREE_VIEW (tree_view), column);

  bobgui_stack_add_titled (BOBGUI_STACK (stack), w3, "3", "3");

  hbox = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
  bobgui_box_append (BOBGUI_BOX (box), hbox);

  button = bobgui_button_new_with_label ("1");
  bobgui_box_append (BOBGUI_BOX (hbox), button);
  g_signal_connect (button, "clicked", (GCallback) set_visible_child, w1);

  button = bobgui_button_new_with_label ("2");
  bobgui_box_append (BOBGUI_BOX (hbox), button);
  g_signal_connect (button, "clicked", (GCallback) set_visible_child, w2);

  button = bobgui_button_new_with_label ("3");
  bobgui_box_append (BOBGUI_BOX (hbox), button);
  g_signal_connect (button, "clicked", (GCallback) set_visible_child, w3);

  button = bobgui_button_new_with_label ("1");
  bobgui_box_append (BOBGUI_BOX (hbox), button);
  g_signal_connect (button, "clicked", (GCallback) set_visible_child_name, (gpointer) "1");

  button = bobgui_button_new_with_label ("2");
  bobgui_box_append (BOBGUI_BOX (hbox), button);
  g_signal_connect (button, "clicked", (GCallback) set_visible_child_name, (gpointer) "2");

  button = bobgui_button_new_with_label ("3");
  bobgui_box_append (BOBGUI_BOX (hbox), button);
  g_signal_connect (button, "clicked", (GCallback) set_visible_child_name, (gpointer) "3");

  button = bobgui_check_button_new ();
  bobgui_check_button_set_active (BOBGUI_CHECK_BUTTON (button),
                               bobgui_stack_get_hhomogeneous (BOBGUI_STACK (stack)));
  bobgui_box_append (BOBGUI_BOX (hbox), button);
  g_signal_connect (button, "clicked", (GCallback) toggle_hhomogeneous, NULL);

  button = bobgui_check_button_new_with_label ("homogeneous");
  bobgui_check_button_set_active (BOBGUI_CHECK_BUTTON (button),
                               bobgui_stack_get_vhomogeneous (BOBGUI_STACK (stack)));
  bobgui_box_append (BOBGUI_BOX (hbox), button);
  g_signal_connect (button, "clicked", (GCallback) toggle_vhomogeneous, NULL);

  button = bobgui_toggle_button_new_with_label ("Add icon");
  g_signal_connect (button, "toggled", (GCallback) toggle_icon_name, NULL);
  bobgui_box_append (BOBGUI_BOX (hbox), button);

  combo = bobgui_combo_box_text_new ();
  class = g_type_class_ref (BOBGUI_TYPE_STACK_TRANSITION_TYPE);
  for (i = 0; i < class->n_values; i++)
    bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (combo), class->values[i].value_nick);
  g_type_class_unref (class);

  bobgui_box_append (BOBGUI_BOX (hbox), combo);
  g_signal_connect (combo, "changed", (GCallback) toggle_transitions, NULL);
  bobgui_combo_box_set_active (BOBGUI_COMBO_BOX (combo), 0);

  hbox = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
  bobgui_box_append (BOBGUI_BOX (box), hbox);

  button = bobgui_button_new_with_label ("<");
  g_signal_connect (button, "clicked", (GCallback) on_back_button_clicked, stack);
  g_signal_connect (stack, "notify::visible-child-name",
                    (GCallback)update_back_button_sensitivity, button);
  bobgui_box_append (BOBGUI_BOX (hbox), button);

  button = bobgui_button_new_with_label (">");
  bobgui_box_append (BOBGUI_BOX (hbox), button);
  g_signal_connect (button, "clicked", (GCallback) on_forward_button_clicked, stack);
  g_signal_connect (stack, "notify::visible-child-name",
                    (GCallback)update_forward_button_sensitivity, button);


  bobgui_window_present (BOBGUI_WINDOW (window));
  while (TRUE)
    g_main_context_iteration (NULL, TRUE);

  bobgui_window_destroy (BOBGUI_WINDOW (window));

  return 0;
}
