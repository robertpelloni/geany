#include <bobgui/bobgui.h>

static BobguiWidget *win;
static BobguiWidget *inhibit_entry;
static BobguiWidget *inhibit_logout;
static BobguiWidget *inhibit_switch;
static BobguiWidget *inhibit_suspend;
static BobguiWidget *inhibit_idle;
static BobguiWidget *inhibit_label;

static void
inhibitor_toggled (BobguiToggleButton *button, BobguiApplication *app)
{
  gboolean active;
  const char *reason;
  BobguiApplicationInhibitFlags flags;
  BobguiWidget *toplevel;
  guint cookie;

  active = bobgui_toggle_button_get_active (button);
  reason = bobgui_editable_get_text (BOBGUI_EDITABLE (inhibit_entry));

  flags = 0;
  if (bobgui_check_button_get_active (BOBGUI_CHECK_BUTTON (inhibit_logout)))
    flags |= BOBGUI_APPLICATION_INHIBIT_LOGOUT;
  if (bobgui_check_button_get_active (BOBGUI_CHECK_BUTTON (inhibit_switch)))
    flags |= BOBGUI_APPLICATION_INHIBIT_SWITCH;
  if (bobgui_check_button_get_active (BOBGUI_CHECK_BUTTON (inhibit_suspend)))
    flags |= BOBGUI_APPLICATION_INHIBIT_SUSPEND;
  if (bobgui_check_button_get_active (BOBGUI_CHECK_BUTTON (inhibit_idle)))
    flags |= BOBGUI_APPLICATION_INHIBIT_IDLE;

  toplevel = BOBGUI_WIDGET (bobgui_widget_get_root (BOBGUI_WIDGET (button)));

  if (active)
    {
      char *text;

      g_print ("Calling bobgui_application_inhibit: %d, '%s'\n", flags, reason);

      cookie = bobgui_application_inhibit (app, BOBGUI_WINDOW (toplevel), flags, reason);
      g_object_set_data (G_OBJECT (button), "cookie", GUINT_TO_POINTER (cookie));
      if (cookie == 0)
        {
          g_signal_handlers_block_by_func (button, inhibitor_toggled, app);
          bobgui_toggle_button_set_active (BOBGUI_TOGGLE_BUTTON (button), FALSE);
          g_signal_handlers_unblock_by_func (button, inhibitor_toggled, app);
          active = FALSE;
        }
      else
        {
          text = g_strdup_printf ("%#x", cookie);
          bobgui_label_set_label (BOBGUI_LABEL (inhibit_label), text);
          g_free (text);
        }
    }
  else
    {
      cookie = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (button), "cookie"));
      g_print ("Calling bobgui_application_uninhibit: %#x\n", cookie);
      bobgui_application_uninhibit (app, cookie);
      bobgui_label_set_label (BOBGUI_LABEL (inhibit_label), "");
    }

  bobgui_widget_set_sensitive (inhibit_entry, !active);
  bobgui_widget_set_sensitive (inhibit_logout, !active);
  bobgui_widget_set_sensitive (inhibit_switch, !active);
  bobgui_widget_set_sensitive (inhibit_suspend, !active);
  bobgui_widget_set_sensitive (inhibit_idle, !active);
}

static void
activate (BobguiApplication *app,
          gpointer        data)
{
  BobguiWidget *box;
  BobguiWidget *separator;
  BobguiWidget *grid;
  BobguiWidget *button;
  BobguiWidget *label;

  win = bobgui_window_new ();

  box = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 12);
  bobgui_widget_set_margin_start (box, 12);
  bobgui_widget_set_margin_end (box, 12);
  bobgui_widget_set_margin_top (box, 12);
  bobgui_widget_set_margin_bottom (box, 12);
  bobgui_window_set_child (BOBGUI_WINDOW (win), box);

  grid = bobgui_grid_new ();
  bobgui_grid_set_row_spacing (BOBGUI_GRID (grid), 6);
  bobgui_grid_set_column_spacing (BOBGUI_GRID (grid), 6);

  bobgui_box_append (BOBGUI_BOX (box), grid);

  label = bobgui_label_new ("Inhibitor");
  bobgui_grid_attach (BOBGUI_GRID (grid), label, 0, 0, 1, 1);

  inhibit_label = bobgui_label_new ("");
  bobgui_grid_attach (BOBGUI_GRID (grid), inhibit_label, 1, 0, 1, 1);

  inhibit_logout = bobgui_check_button_new_with_label ("Logout");
  bobgui_grid_attach (BOBGUI_GRID (grid), inhibit_logout, 1, 1, 1, 1);

  inhibit_switch = bobgui_check_button_new_with_label ("User switching");
  bobgui_grid_attach (BOBGUI_GRID (grid), inhibit_switch, 1, 2, 1, 1);

  inhibit_suspend = bobgui_check_button_new_with_label ("Suspend");
  bobgui_grid_attach (BOBGUI_GRID (grid), inhibit_suspend, 1, 4, 1, 1);

  inhibit_idle = bobgui_check_button_new_with_label ("Idle");
  bobgui_grid_attach (BOBGUI_GRID (grid), inhibit_idle, 1, 5, 1, 1);

  inhibit_entry = bobgui_entry_new ();
  bobgui_grid_attach (BOBGUI_GRID (grid), inhibit_entry, 1, 6, 1, 1);

  button = bobgui_toggle_button_new_with_label ("Inhibit");
  g_signal_connect (button, "toggled",
                    G_CALLBACK (inhibitor_toggled), app);
  bobgui_grid_attach (BOBGUI_GRID (grid), button, 2, 6, 1, 1);

  separator = bobgui_separator_new (BOBGUI_ORIENTATION_HORIZONTAL);
  bobgui_box_append (BOBGUI_BOX (box), separator);

  grid = bobgui_grid_new ();
  bobgui_grid_set_row_spacing (BOBGUI_GRID (grid), 6);
  bobgui_grid_set_column_spacing (BOBGUI_GRID (grid), 6);

  bobgui_window_present (BOBGUI_WINDOW (win));

  bobgui_application_add_window (app, BOBGUI_WINDOW (win));
}

int
main (int argc, char *argv[])
{
  BobguiApplication *app;

  app = bobgui_application_new ("org.bobgui.Test.session", 0);
  g_object_set (app, "register-session", TRUE, NULL);

  g_signal_connect (app, "activate",
                    G_CALLBACK (activate), NULL);

  g_application_run (G_APPLICATION (app), argc, argv);

  return 0;
}
