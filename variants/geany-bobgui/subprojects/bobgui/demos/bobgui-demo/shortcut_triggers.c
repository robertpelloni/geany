/* Shortcuts
 * #Keywords: BobguiShortcutController
 *
 * BobguiShortcut is the abstraction used by BOBGUI to handle shortcuts from
 * keyboard or other input devices.
 *
 * Shortcut triggers can be used to weave complex sequences of key
 * presses into sophisticated mechanisms to activate shortcuts.
 *
 * This demo code shows creative ways to do that.
 */

#include <bobgui/bobgui.h>

static BobguiWidget *window = NULL;

static gboolean
shortcut_activated (BobguiWidget *widget,
                    GVariant  *unused,
                    gpointer   row)
{
  g_print ("activated %s\n", bobgui_label_get_label (row));
  return TRUE;
}

static BobguiShortcutTrigger *
create_ctrl_g (void)
{
  return bobgui_keyval_trigger_new (GDK_KEY_g, GDK_CONTROL_MASK);
}

static BobguiShortcutTrigger *
create_x (void)
{
  return bobgui_keyval_trigger_new (GDK_KEY_x, 0);
}

struct {
  const char *description;
  BobguiShortcutTrigger * (* create_trigger_func) (void);
} shortcuts[] = {
  { "Press Ctrl-G", create_ctrl_g },
  { "Press X", create_x },
};

BobguiWidget *
do_shortcut_triggers (BobguiWidget *do_widget)
{
  guint i;

  if (!window)
    {
      BobguiWidget *list;
      BobguiEventController *controller;

      window = bobgui_window_new ();
      bobgui_window_set_display (BOBGUI_WINDOW (window),
                              bobgui_widget_get_display (do_widget));
      bobgui_window_set_title (BOBGUI_WINDOW (window), "Shortcuts");
      bobgui_window_set_default_size (BOBGUI_WINDOW (window), 200, -1);
      bobgui_window_set_resizable (BOBGUI_WINDOW (window), FALSE);
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);

      list = bobgui_list_box_new ();
      bobgui_widget_set_margin_top (list, 6);
      bobgui_widget_set_margin_bottom (list, 6);
      bobgui_widget_set_margin_start (list, 6);
      bobgui_widget_set_margin_end (list, 6);
      bobgui_window_set_child (BOBGUI_WINDOW (window), list);

      for (i = 0; i < G_N_ELEMENTS (shortcuts); i++)
        {
          BobguiShortcut *shortcut;
          BobguiWidget *row;

          row = bobgui_label_new (shortcuts[i].description);
          bobgui_list_box_insert (BOBGUI_LIST_BOX (list), row, -1);

          controller = bobgui_shortcut_controller_new ();
          bobgui_shortcut_controller_set_scope (BOBGUI_SHORTCUT_CONTROLLER (controller), BOBGUI_SHORTCUT_SCOPE_GLOBAL);
          bobgui_widget_add_controller (row, controller);

          shortcut = bobgui_shortcut_new (shortcuts[i].create_trigger_func(),
                                       bobgui_callback_action_new (shortcut_activated, row, NULL));
          bobgui_shortcut_controller_add_shortcut (BOBGUI_SHORTCUT_CONTROLLER (controller), shortcut);
        }
    }

  if (!bobgui_widget_get_visible (window))
    bobgui_widget_set_visible (window, TRUE);
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));

  return window;
}
