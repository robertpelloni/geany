/* Spinner
 *
 * BobguiSpinner allows to show that background activity is on-going.
 */

#include <glib/gi18n.h>
#include <bobgui/bobgui.h>

static BobguiWidget *spinner_sensitive = NULL;
static BobguiWidget *spinner_unsensitive = NULL;

static void
on_play_clicked (BobguiButton *button, gpointer user_data)
{
  bobgui_spinner_start (BOBGUI_SPINNER (spinner_sensitive));
  bobgui_spinner_start (BOBGUI_SPINNER (spinner_unsensitive));
}

static void
on_stop_clicked (BobguiButton *button, gpointer user_data)
{
  bobgui_spinner_stop (BOBGUI_SPINNER (spinner_sensitive));
  bobgui_spinner_stop (BOBGUI_SPINNER (spinner_unsensitive));
}

BobguiWidget *
do_spinner (BobguiWidget *do_widget)
{
  static BobguiWidget *window = NULL;
  BobguiWidget *vbox;
  BobguiWidget *hbox;
  BobguiWidget *button;
  BobguiWidget *spinner;

  if (!window)
  {
    window = bobgui_window_new ();
    bobgui_window_set_transient_for (BOBGUI_WINDOW (window), BOBGUI_WINDOW (do_widget));
    bobgui_window_set_title (BOBGUI_WINDOW (window), "Spinner");
    bobgui_window_set_resizable (BOBGUI_WINDOW (window), FALSE);
    g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);

    vbox = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 10);
    bobgui_widget_set_margin_top (vbox, 5);
    bobgui_widget_set_margin_bottom (vbox, 5);
    bobgui_widget_set_margin_start (vbox, 5);
    bobgui_widget_set_margin_end (vbox, 5);

    bobgui_window_set_child (BOBGUI_WINDOW (window), vbox);

    /* Sensitive */
    hbox = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 5);
    spinner = bobgui_spinner_new ();
    bobgui_box_append (BOBGUI_BOX (hbox), spinner);
    bobgui_box_append (BOBGUI_BOX (hbox), bobgui_entry_new ());
    bobgui_box_append (BOBGUI_BOX (vbox), hbox);
    spinner_sensitive = spinner;

    /* Disabled */
    hbox = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 5);
    spinner = bobgui_spinner_new ();
    bobgui_box_append (BOBGUI_BOX (hbox), spinner);
    bobgui_box_append (BOBGUI_BOX (hbox), bobgui_entry_new ());
    bobgui_box_append (BOBGUI_BOX (vbox), hbox);
    spinner_unsensitive = spinner;
    bobgui_widget_set_sensitive (hbox, FALSE);

    button = bobgui_button_new_with_label (_("Play"));
    g_signal_connect (G_OBJECT (button), "clicked",
                      G_CALLBACK (on_play_clicked), spinner);
    bobgui_box_append (BOBGUI_BOX (vbox), button);

    button = bobgui_button_new_with_label (_("Stop"));
    g_signal_connect (G_OBJECT (button), "clicked",
                      G_CALLBACK (on_stop_clicked), spinner);
    bobgui_box_append (BOBGUI_BOX (vbox), button);

    /* Start by default to test for:
     * https://bugzilla.gnome.org/show_bug.cgi?id=598496 */
    on_play_clicked (NULL, NULL);
  }

  if (!bobgui_widget_get_visible (window))
    bobgui_widget_set_visible (window, TRUE);
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));

  return window;
}
