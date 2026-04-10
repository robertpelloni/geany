/* Builder
 * #Keywords: GMenu, BobguiPopoverMenuBar, BobguiBuilder, BobguiShortcutController, toolbar
 *
 * Demonstrates a traditional interface, loaded from a XML description,
 * and shows how to connect actions to the menu items and toolbar buttons.
 */

#include <bobgui/bobgui.h>

static void
quit_activate (GSimpleAction *action,
               GVariant      *parameter,
               gpointer       user_data)
{
  BobguiWidget *window = user_data;

  bobgui_window_destroy (BOBGUI_WINDOW (window));
}

static void
about_activate (GSimpleAction *action,
                GVariant      *parameter,
                gpointer       user_data)
{
  BobguiWidget *window = user_data;
  BobguiWidget *about_dlg;

  about_dlg = BOBGUI_WIDGET (g_object_get_data (G_OBJECT (window), "about"));
  bobgui_window_present (BOBGUI_WINDOW (about_dlg));
}

static void
remove_timeout (gpointer data)
{
  guint id = GPOINTER_TO_UINT (data);

  g_source_remove (id);
}

static int
pop_message (gpointer data)
{
  BobguiWidget *status = data;

  bobgui_label_set_label (BOBGUI_LABEL (status), "");
  g_object_set_data (G_OBJECT (status), "timeout", GUINT_TO_POINTER (0));

  return G_SOURCE_REMOVE;
}

static void
status_message (BobguiWidget  *status,
                const char *text)
{
  guint id;

  id = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (status), "timeout"));
  if (id)
    g_source_remove (id);

  bobgui_label_set_text (BOBGUI_LABEL (status), text);

  id = g_timeout_add (5000, pop_message, status);

  g_object_set_data_full (G_OBJECT (status), "timeout", GUINT_TO_POINTER (id), remove_timeout);
}

static void
help_activate (GSimpleAction *action,
               GVariant      *parameter,
               gpointer       user_data)
{
  BobguiWidget *status;

  status = BOBGUI_WIDGET (g_object_get_data (G_OBJECT (user_data), "status"));
  status_message (status, "Help not available");
}

static void
not_implemented (GSimpleAction *action,
                 GVariant      *parameter,
                 gpointer       user_data)
{
  BobguiWidget *status;
  char *text;

  text = g_strdup_printf ("Action “%s” not implemented", g_action_get_name (G_ACTION (action)));
  status = BOBGUI_WIDGET (g_object_get_data (G_OBJECT (user_data), "status"));
  status_message (status, text);
  g_free (text);
}

static GActionEntry win_entries[] = {
  { "new", not_implemented, NULL, NULL, NULL },
  { "open", not_implemented, NULL, NULL, NULL },
  { "save", not_implemented, NULL, NULL, NULL },
  { "save-as", not_implemented, NULL, NULL, NULL },
  { "copy", not_implemented, NULL, NULL, NULL },
  { "cut", not_implemented, NULL, NULL, NULL },
  { "paste", not_implemented, NULL, NULL, NULL },
  { "quit", quit_activate, NULL, NULL, NULL },
  { "about", about_activate, NULL, NULL, NULL },
  { "help", help_activate, NULL, NULL, NULL }
};

BobguiWidget *
do_builder (BobguiWidget *do_widget)
{
  static BobguiWidget *window = NULL;
  GActionGroup *actions;

  if (!window)
    {
      BobguiBuilder *builder;
      BobguiWidget *about;
      BobguiWidget *status;
      BobguiEventController *controller;

      builder = bobgui_builder_new_from_resource ("/builder/demo.ui");

      window = BOBGUI_WIDGET (bobgui_builder_get_object (builder, "window1"));
      bobgui_window_set_display (BOBGUI_WINDOW (window),
                              bobgui_widget_get_display (do_widget));
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);
      actions = (GActionGroup*)g_simple_action_group_new ();
      g_action_map_add_action_entries (G_ACTION_MAP (actions),
                                       win_entries, G_N_ELEMENTS (win_entries),
                                       window);
      bobgui_widget_insert_action_group (window, "win", actions);

      controller = bobgui_shortcut_controller_new ();
      bobgui_shortcut_controller_set_scope (BOBGUI_SHORTCUT_CONTROLLER (controller),
                                         BOBGUI_SHORTCUT_SCOPE_GLOBAL);
      bobgui_widget_add_controller (window, controller);
      bobgui_shortcut_controller_add_shortcut (BOBGUI_SHORTCUT_CONTROLLER (controller),
           bobgui_shortcut_new (bobgui_keyval_trigger_new (GDK_KEY_n, GDK_CONTROL_MASK),
                             bobgui_named_action_new ("win.new")));
      bobgui_shortcut_controller_add_shortcut (BOBGUI_SHORTCUT_CONTROLLER (controller),
           bobgui_shortcut_new (bobgui_keyval_trigger_new (GDK_KEY_o, GDK_CONTROL_MASK),
                             bobgui_named_action_new ("win.open")));
      bobgui_shortcut_controller_add_shortcut (BOBGUI_SHORTCUT_CONTROLLER (controller),
           bobgui_shortcut_new (bobgui_keyval_trigger_new (GDK_KEY_s, GDK_CONTROL_MASK),
                             bobgui_named_action_new ("win.save")));
      bobgui_shortcut_controller_add_shortcut (BOBGUI_SHORTCUT_CONTROLLER (controller),
           bobgui_shortcut_new (bobgui_keyval_trigger_new (GDK_KEY_s, GDK_CONTROL_MASK|GDK_SHIFT_MASK),
                             bobgui_named_action_new ("win.save-as")));
      bobgui_shortcut_controller_add_shortcut (BOBGUI_SHORTCUT_CONTROLLER (controller),
           bobgui_shortcut_new (bobgui_keyval_trigger_new (GDK_KEY_q, GDK_CONTROL_MASK),
                             bobgui_named_action_new ("win.quit")));
      bobgui_shortcut_controller_add_shortcut (BOBGUI_SHORTCUT_CONTROLLER (controller),
           bobgui_shortcut_new (bobgui_keyval_trigger_new (GDK_KEY_c, GDK_CONTROL_MASK),
                             bobgui_named_action_new ("win.copy")));
      bobgui_shortcut_controller_add_shortcut (BOBGUI_SHORTCUT_CONTROLLER (controller),
           bobgui_shortcut_new (bobgui_keyval_trigger_new (GDK_KEY_x, GDK_CONTROL_MASK),
                             bobgui_named_action_new ("win.cut")));
      bobgui_shortcut_controller_add_shortcut (BOBGUI_SHORTCUT_CONTROLLER (controller),
           bobgui_shortcut_new (bobgui_keyval_trigger_new (GDK_KEY_v, GDK_CONTROL_MASK),
                             bobgui_named_action_new ("win.paste")));
      bobgui_shortcut_controller_add_shortcut (BOBGUI_SHORTCUT_CONTROLLER (controller),
           bobgui_shortcut_new (bobgui_keyval_trigger_new (GDK_KEY_F1, 0),
                             bobgui_named_action_new ("win.help")));
      bobgui_shortcut_controller_add_shortcut (BOBGUI_SHORTCUT_CONTROLLER (controller),
           bobgui_shortcut_new (bobgui_keyval_trigger_new (GDK_KEY_F7, 0),
                             bobgui_named_action_new ("win.about")));

      about = BOBGUI_WIDGET (bobgui_builder_get_object (builder, "aboutdialog1"));
      bobgui_window_set_transient_for (BOBGUI_WINDOW (about), BOBGUI_WINDOW (window));
      bobgui_window_set_hide_on_close (BOBGUI_WINDOW (about), TRUE);
      g_object_set_data_full (G_OBJECT (window), "about",
                              about, (GDestroyNotify)bobgui_window_destroy);

      status = BOBGUI_WIDGET (bobgui_builder_get_object (builder, "statusbar1"));
      g_object_set_data (G_OBJECT (window), "status", status);

      g_object_unref (builder);
    }

  if (!bobgui_widget_get_visible (window))
    bobgui_widget_set_visible (window, TRUE);
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));

  return window;
}
