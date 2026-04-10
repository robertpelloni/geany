#include <bobgui/bobgui.h>
#include <glib/gstdio.h>

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

static void
activate (GSimpleAction *action,
          GVariant      *parameter,
          gpointer       user_data)
{
  g_print ("%s activated\n", g_action_get_name (G_ACTION (action)));
}

static GActionEntry entries[] = {
  { "cut", activate, NULL, NULL, NULL },
  { "copy", activate, NULL, NULL, NULL },
  { "paste", activate, NULL, NULL, NULL },
  { "bold", NULL, NULL, "false", NULL },
  { "italic", NULL, NULL, "false", NULL },
  { "strikethrough", NULL, NULL, "false", NULL },
  { "underline", NULL, NULL, "false", NULL },
  { "set-view", NULL, "s", "'list'", NULL },
  { "action1", activate, NULL, NULL, NULL },
  { "action2", NULL, NULL, "true", NULL },
  { "action2a", NULL, NULL, "false", NULL },
  { "action3", NULL, "s", "'three'", NULL },
  { "action4", activate, NULL, NULL, NULL },
  { "action5", activate, NULL, NULL, NULL },
  { "action6", activate, NULL, NULL, NULL },
  { "action7", activate, NULL, NULL, NULL },
  { "action8", activate, NULL, NULL, NULL },
  { "action9", activate, NULL, NULL, NULL },
  { "action10", activate, NULL, NULL, NULL }
};

static void
quit_cb (BobguiWidget *widget,
         gpointer   data)
{
  gboolean *done = data;

  *done = TRUE;

  g_main_context_wakeup (NULL);
}

int
main (int argc, char *argv[])
{
  BobguiWidget *win;
  BobguiWidget *box;
  BobguiWidget *button;
  BobguiWidget *button1;
  BobguiWidget *button2;
  BobguiBuilder *builder;
  GMenuModel *model;
  GSimpleActionGroup *actions;
  BobguiWidget *overlay;
  BobguiWidget *grid;
  BobguiWidget *popover;
  BobguiWidget *popover1;
  BobguiWidget *popover2;
  BobguiWidget *label;
  BobguiWidget *check;
  BobguiWidget *combo;
  BobguiWidget *header_bar;
  gboolean done = FALSE;

#ifdef BOBGUI_SRCDIR
  g_chdir (BOBGUI_SRCDIR);
#endif

  bobgui_init ();

  win = bobgui_window_new ();
  bobgui_window_set_default_size (BOBGUI_WINDOW (win), 400, 600);
  header_bar = bobgui_header_bar_new ();
  bobgui_window_set_titlebar (BOBGUI_WINDOW (win), header_bar);
  bobgui_window_set_title (BOBGUI_WINDOW (win), "Test BobguiPopover");
  actions = g_simple_action_group_new ();
  g_action_map_add_action_entries (G_ACTION_MAP (actions), entries, G_N_ELEMENTS (entries), NULL);

  bobgui_widget_insert_action_group (win, "top", G_ACTION_GROUP (actions));

  overlay = bobgui_overlay_new ();
  bobgui_window_set_child (BOBGUI_WINDOW (win), overlay);

  grid = bobgui_grid_new ();
  bobgui_widget_set_halign (grid, BOBGUI_ALIGN_FILL);
  bobgui_widget_set_valign (grid, BOBGUI_ALIGN_FILL);
  bobgui_grid_set_row_spacing (BOBGUI_GRID (grid), 10);
  bobgui_grid_set_column_spacing (BOBGUI_GRID (grid), 10);
  bobgui_overlay_set_child (BOBGUI_OVERLAY (overlay), grid);

  label = bobgui_label_new ("");
  bobgui_widget_set_hexpand (label, TRUE);
  bobgui_widget_set_vexpand (label, TRUE);
  bobgui_grid_attach (BOBGUI_GRID (grid), label, 0, 0, 1, 1);

  label = bobgui_label_new ("");
  bobgui_widget_set_hexpand (label, TRUE);
  bobgui_widget_set_vexpand (label, TRUE);
  bobgui_grid_attach (BOBGUI_GRID (grid), label, 3, 7, 1, 1);

  builder = bobgui_builder_new_from_file ("popover.ui");
  model = (GMenuModel *)bobgui_builder_get_object (builder, "menu");

  box = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 6);
  button = bobgui_menu_button_new ();
  bobgui_box_append (BOBGUI_BOX (box), button);
  button1 = bobgui_menu_button_new ();
  bobgui_box_append (BOBGUI_BOX (box), button1);
  button2 = bobgui_menu_button_new ();
  bobgui_box_append (BOBGUI_BOX (box), button2);

  bobgui_menu_button_set_menu_model (BOBGUI_MENU_BUTTON (button), model);
  popover = BOBGUI_WIDGET (bobgui_menu_button_get_popover (BOBGUI_MENU_BUTTON (button)));

  popover1 = bobgui_popover_menu_new_from_model_full (model, BOBGUI_POPOVER_MENU_NESTED);
  bobgui_menu_button_set_popover (BOBGUI_MENU_BUTTON (button1), popover1);

  g_object_unref (builder);
  builder = bobgui_builder_new_from_file ("popover2.ui");
  popover2 = (BobguiWidget *)bobgui_builder_get_object (builder, "popover");
  bobgui_menu_button_set_popover (BOBGUI_MENU_BUTTON (button2), popover2);

  bobgui_widget_set_margin_start (box, 10);
  bobgui_widget_set_margin_end (box, 10);
  bobgui_widget_set_margin_top (box, 10);
  bobgui_widget_set_margin_bottom (box, 10);
  bobgui_overlay_add_overlay (BOBGUI_OVERLAY (overlay), box);

  label = bobgui_label_new ("Popover hexpand");
  check = bobgui_check_button_new ();
  g_object_bind_property (check, "active", popover, "hexpand", G_BINDING_SYNC_CREATE);
  g_object_bind_property (check, "active", popover1, "hexpand", G_BINDING_SYNC_CREATE);
  g_object_bind_property (check, "active", popover2, "hexpand", G_BINDING_SYNC_CREATE);
  bobgui_grid_attach (BOBGUI_GRID (grid), label , 1, 1, 1, 1);
  bobgui_grid_attach (BOBGUI_GRID (grid), check, 2, 1, 1, 1);

  label = bobgui_label_new ("Popover vexpand");
  check = bobgui_check_button_new ();
  g_object_bind_property (check, "active", popover, "vexpand", G_BINDING_SYNC_CREATE);
  g_object_bind_property (check, "active", popover1, "vexpand", G_BINDING_SYNC_CREATE);
  g_object_bind_property (check, "active", popover2, "vexpand", G_BINDING_SYNC_CREATE);
  bobgui_grid_attach (BOBGUI_GRID (grid), label , 1, 2, 1, 1);
  bobgui_grid_attach (BOBGUI_GRID (grid), check, 2, 2, 1, 1);


  label = bobgui_label_new ("Autohide");
  check = bobgui_check_button_new ();
  g_object_bind_property (check, "active", popover, "autohide", G_BINDING_SYNC_CREATE);
  g_object_bind_property (check, "active", popover1, "autohide", G_BINDING_SYNC_CREATE);
  g_object_bind_property (check, "active", popover2, "autohide", G_BINDING_SYNC_CREATE);
  bobgui_check_button_set_active (BOBGUI_CHECK_BUTTON (check), TRUE);
  bobgui_grid_attach (BOBGUI_GRID (grid), label , 1, 3, 1, 1);
  bobgui_grid_attach (BOBGUI_GRID (grid), check, 2, 3, 1, 1);

  label = bobgui_label_new ("Button direction");
  combo = bobgui_combo_box_text_new ();
  bobgui_combo_box_text_append (BOBGUI_COMBO_BOX_TEXT (combo), "up", "Up");
  bobgui_combo_box_text_append (BOBGUI_COMBO_BOX_TEXT (combo), "down", "Down");
  bobgui_combo_box_text_append (BOBGUI_COMBO_BOX_TEXT (combo), "left", "Left");
  bobgui_combo_box_text_append (BOBGUI_COMBO_BOX_TEXT (combo), "right", "Right");
  bobgui_combo_box_set_active (BOBGUI_COMBO_BOX (combo), 1);
  g_object_bind_property (combo, "active", button, "direction", G_BINDING_SYNC_CREATE);
  g_object_bind_property (combo, "active", button1, "direction", G_BINDING_SYNC_CREATE);
  g_object_bind_property (combo, "active", button2, "direction", G_BINDING_SYNC_CREATE);
  bobgui_grid_attach (BOBGUI_GRID (grid), label , 1, 4, 1, 1);
  bobgui_grid_attach (BOBGUI_GRID (grid), combo, 2, 4, 1, 1);

  label = bobgui_label_new ("Button halign");
  combo = bobgui_combo_box_text_new ();
  bobgui_combo_box_text_append (BOBGUI_COMBO_BOX_TEXT (combo), "fill", "Fill");
  bobgui_combo_box_text_append (BOBGUI_COMBO_BOX_TEXT (combo), "start", "Start");
  bobgui_combo_box_text_append (BOBGUI_COMBO_BOX_TEXT (combo), "end", "End");
  bobgui_combo_box_text_append (BOBGUI_COMBO_BOX_TEXT (combo), "center", "Center");
  bobgui_combo_box_set_active (BOBGUI_COMBO_BOX (combo), 2);
  g_object_bind_property (combo, "active", box, "halign", G_BINDING_SYNC_CREATE);
  bobgui_grid_attach (BOBGUI_GRID (grid), label , 1, 5, 1, 1);
  bobgui_grid_attach (BOBGUI_GRID (grid), combo, 2, 5, 1, 1);

  label = bobgui_label_new ("Button valign");
  combo = bobgui_combo_box_text_new ();
  bobgui_combo_box_text_append (BOBGUI_COMBO_BOX_TEXT (combo), "fill", "Fill");
  bobgui_combo_box_text_append (BOBGUI_COMBO_BOX_TEXT (combo), "start", "Start");
  bobgui_combo_box_text_append (BOBGUI_COMBO_BOX_TEXT (combo), "end", "End");
  bobgui_combo_box_text_append (BOBGUI_COMBO_BOX_TEXT (combo), "center", "Center");
  bobgui_combo_box_set_active (BOBGUI_COMBO_BOX (combo), 1);
  g_object_bind_property (combo, "active", box, "valign", G_BINDING_SYNC_CREATE);
  bobgui_grid_attach (BOBGUI_GRID (grid), label , 1, 6, 1, 1);
  bobgui_grid_attach (BOBGUI_GRID (grid), combo, 2, 6, 1, 1);
  g_object_unref (builder);


  g_signal_connect (win, "destroy", G_CALLBACK (quit_cb), &done);
  bobgui_window_present (BOBGUI_WINDOW (win));

  while (!done)
    g_main_context_iteration (NULL, TRUE);

  return 0;
}
