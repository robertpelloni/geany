/* Links
 *
 * BobguiLabel can show hyperlinks. The default action is to call
 * bobgui_show_uri() on their URI, but it is possible to override
 * this with a custom handler.
 */

#include <bobgui/bobgui.h>

static gboolean
activate_link (BobguiWidget  *label,
               const char *uri,
               gpointer    data)
{
  if (g_strcmp0 (uri, "keynav") == 0)
    {
      BobguiAlertDialog *dialog;

      dialog = bobgui_alert_dialog_new ("Keyboard navigation");
      bobgui_alert_dialog_set_detail (dialog,
                                   "The term ‘keynav’ is a shorthand for "
                                   "keyboard navigation and refers to the process of using "
                                   "a program (exclusively) via keyboard input.");
      bobgui_alert_dialog_show (dialog, BOBGUI_WINDOW (bobgui_widget_get_root (label)));
      g_object_unref (dialog);

      return TRUE;
    }

  return FALSE;
}

BobguiWidget *
do_links (BobguiWidget *do_widget)
{
  static BobguiWidget *window = NULL;
  BobguiWidget *label;

  if (!window)
    {
      window = bobgui_window_new ();
      bobgui_window_set_display (BOBGUI_WINDOW (window),
                              bobgui_widget_get_display (do_widget));
      bobgui_window_set_title (BOBGUI_WINDOW (window), "Links");
      bobgui_window_set_resizable (BOBGUI_WINDOW (window), FALSE);
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);

      label = bobgui_label_new ("Some <a href=\"http://en.wikipedia.org/wiki/Text\""
                             "title=\"plain text\">text</a> may be marked up "
                             "as hyperlinks, which can be clicked "
                             "or activated via <a href=\"keynav\">keynav</a> "
                             "and they work fine with other markup, like when "
                             "linking to <a href=\"http://www.flathub.org/\"><b>"
                             "<span letter_spacing=\"1024\" underline=\"none\" color=\"pink\" background=\"darkslategray\">Flathub</span>"
                             "</b></a>.");
      bobgui_label_set_use_markup (BOBGUI_LABEL (label), TRUE);
      bobgui_label_set_max_width_chars (BOBGUI_LABEL (label), 40);
      bobgui_label_set_wrap (BOBGUI_LABEL (label), TRUE);
      bobgui_label_set_wrap_mode (BOBGUI_LABEL (label), PANGO_WRAP_WORD);
      g_signal_connect (label, "activate-link", G_CALLBACK (activate_link), NULL);
      bobgui_widget_set_margin_start (label, 20);
      bobgui_widget_set_margin_end (label, 20);
      bobgui_widget_set_margin_top (label, 20);
      bobgui_widget_set_margin_bottom (label, 20);
      bobgui_window_set_child (BOBGUI_WINDOW (window), label);
    }

  if (!bobgui_widget_get_visible (window))
    bobgui_widget_set_visible (window, TRUE);
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));

  return window;
}
