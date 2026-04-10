#include <bobgui/bobgui.h>

BobguiWidget *label;


static void
change_label_button (GSimpleAction *action,
                     GVariant      *parameter,
                     gpointer       user_data)
{
  bobgui_label_set_label (BOBGUI_LABEL (label), "Text set from button");
}

static void
normal_menu_item (GSimpleAction *action,
                  GVariant      *parameter,
                  gpointer       user_data)
{
  bobgui_label_set_label (BOBGUI_LABEL (label), "Text set from normal menu item");
}

static void
toggle_menu_item (GSimpleAction *action,
                  GVariant      *parameter,
                  gpointer       user_data)
{
  GVariant *state = g_action_get_state (G_ACTION (action));

  bobgui_label_set_label (BOBGUI_LABEL (label), "Text set from toggle menu item");

  g_simple_action_set_state (action, g_variant_new_boolean (!g_variant_get_boolean (state)));

  g_variant_unref (state);
}

static void
submenu_item (GSimpleAction *action,
              GVariant      *parameter,
              gpointer       user_data)
{
  bobgui_label_set_label (BOBGUI_LABEL (label), "Text set from submenu item");
}

static void
radio (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  char *str;

  str = g_strdup_printf ("From Radio menu item %s",
                         g_variant_get_string (parameter, NULL));

  bobgui_label_set_label (BOBGUI_LABEL (label), str);

  g_free (str);

  g_simple_action_set_state (action, parameter);
}



static const GActionEntry win_actions[] = {
  { "change-label-button", change_label_button, NULL, NULL, NULL },
  { "normal-menu-item",    normal_menu_item,    NULL, NULL, NULL },
  { "toggle-menu-item",    toggle_menu_item,    NULL, "true", NULL },
  { "submenu-item",        submenu_item,        NULL, NULL, NULL },
  { "radio",               radio,               "s", "'1'", NULL },
};


static const char *menu_data =
  "<interface>"
  "  <menu id=\"menu_model\">"
  "    <section>"
  "      <item>"
  "        <attribute name=\"label\">Normal Menu Item</attribute>"
  "        <attribute name=\"action\">win.normal-menu-item</attribute>"
  "      </item>"
  "      <submenu>"
  "        <attribute name=\"label\">Submenu</attribute>"
  "        <item>"
  "          <attribute name=\"label\">Submenu Item</attribute>"
  "          <attribute name=\"action\">win.submenu-item</attribute>"
  "        </item>"
  "      </submenu>"
  "      <item>"
  "        <attribute name=\"label\">Toggle Menu Item</attribute>"
  "        <attribute name=\"action\">win.toggle-menu-item</attribute>"
  "      </item>"
  "    </section>"
  "    <section>"
  "      <item>"
  "        <attribute name=\"label\">Radio 1</attribute>"
  "        <attribute name=\"action\">win.radio</attribute>"
  "        <attribute name=\"target\">1</attribute>"
  "      </item>"
  "      <item>"
  "        <attribute name=\"label\">Radio 2</attribute>"
  "        <attribute name=\"action\">win.radio</attribute>"
  "        <attribute name=\"target\">2</attribute>"
  "      </item>"
  "      <item>"
  "        <attribute name=\"label\">Radio 3</attribute>"
  "        <attribute name=\"action\">win.radio</attribute>"
  "        <attribute name=\"target\">3</attribute>"
  "      </item>"
  "    </section>"
  "  </menu>"
  "</interface>"
;


int main (int argc, char **argv)
{
  bobgui_init ();
  BobguiWidget *window = bobgui_window_new ();
  BobguiWidget *box = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 12);
  BobguiWidget *menubutton = bobgui_menu_button_new ();
  BobguiWidget *button1 = bobgui_button_new_with_label ("Change Label Text");
  BobguiWidget *menu;
  GSimpleActionGroup *action_group;
  BobguiWidget *box1;


  action_group = g_simple_action_group_new ();
  g_action_map_add_action_entries (G_ACTION_MAP (action_group),
                                   win_actions,
                                   G_N_ELEMENTS (win_actions),
                                   NULL);

  bobgui_widget_insert_action_group (window, "win", G_ACTION_GROUP (action_group));


  label = bobgui_label_new ("Initial Text");
  bobgui_widget_set_margin_top (label, 12);
  bobgui_widget_set_margin_bottom (label, 12);
  bobgui_box_append (BOBGUI_BOX (box), label);
  bobgui_widget_set_halign (menubutton, BOBGUI_ALIGN_CENTER);
  {
    GMenuModel *menu_model;
    BobguiBuilder *builder = bobgui_builder_new_from_string (menu_data, -1);
    menu_model = G_MENU_MODEL (bobgui_builder_get_object (builder, "menu_model"));

    menu = bobgui_popover_menu_new_from_model (menu_model);

  }
  bobgui_menu_button_set_popover (BOBGUI_MENU_BUTTON (menubutton), menu);
  bobgui_box_append (BOBGUI_BOX (box), menubutton);
  bobgui_widget_set_halign (button1, BOBGUI_ALIGN_CENTER);
  bobgui_actionable_set_action_name (BOBGUI_ACTIONABLE (button1), "win.change-label-button");
  bobgui_box_append (BOBGUI_BOX (box), button1);

  box1 = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 10);

  button1 = bobgui_toggle_button_new_with_label ("Toggle");
  bobgui_actionable_set_action_name (BOBGUI_ACTIONABLE (button1), "win.toggle-menu-item");
  bobgui_box_append (BOBGUI_BOX (box1), button1);

  button1 = bobgui_check_button_new_with_label ("Check");
  bobgui_actionable_set_action_name (BOBGUI_ACTIONABLE (button1), "win.toggle-menu-item");
  bobgui_box_append (BOBGUI_BOX (box1), button1);

  bobgui_box_append (BOBGUI_BOX (box), box1);

  box1 = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 10);

  button1 = bobgui_toggle_button_new_with_label ("Radio 1");
  bobgui_actionable_set_detailed_action_name (BOBGUI_ACTIONABLE (button1), "win.radio::1");
  bobgui_box_append (BOBGUI_BOX (box1), button1);

  button1 = bobgui_toggle_button_new_with_label ("Radio 2");
  bobgui_actionable_set_detailed_action_name (BOBGUI_ACTIONABLE (button1), "win.radio::2");
  bobgui_box_append (BOBGUI_BOX (box1), button1);

  button1 = bobgui_toggle_button_new_with_label ("Radio 3");
  bobgui_actionable_set_detailed_action_name (BOBGUI_ACTIONABLE (button1), "win.radio::3");
  bobgui_box_append (BOBGUI_BOX (box1), button1);

  bobgui_box_append (BOBGUI_BOX (box), box1);

  box1 = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 10);

  button1 = bobgui_check_button_new_with_label ("Radio 1");
  bobgui_actionable_set_detailed_action_name (BOBGUI_ACTIONABLE (button1), "win.radio::1");
  bobgui_box_append (BOBGUI_BOX (box1), button1);

  button1 = bobgui_check_button_new_with_label ("Radio 2");
  bobgui_actionable_set_detailed_action_name (BOBGUI_ACTIONABLE (button1), "win.radio::2");
  bobgui_box_append (BOBGUI_BOX (box1), button1);

  button1 = bobgui_check_button_new_with_label ("Radio 3");
  bobgui_actionable_set_detailed_action_name (BOBGUI_ACTIONABLE (button1), "win.radio::3");
  bobgui_box_append (BOBGUI_BOX (box1), button1);

  bobgui_box_append (BOBGUI_BOX (box), box1);

  bobgui_window_set_child (BOBGUI_WINDOW (window), box);

  bobgui_window_present (BOBGUI_WINDOW (window));
  while (TRUE)
    g_main_context_iteration (NULL, TRUE);
  return 0;
}
