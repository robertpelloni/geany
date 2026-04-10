/* Entry/Password Entry
 *
 * BobguiPasswordEntry provides common functionality of
 * entries that are used to enter passwords and other
 * secrets.
 *
 * It will display a warning if CapsLock is on, and it
 * can optionally provide a way to see the text.
 */

#include <glib/gi18n.h>
#include <bobgui/bobgui.h>


static BobguiWidget *entry;
static BobguiWidget *entry2;
static BobguiWidget *button;

static void
update_button (GObject    *object,
               GParamSpec *pspec,
               gpointer    data)
{
  const char *text = bobgui_editable_get_text (BOBGUI_EDITABLE (entry));
  const char *text2 = bobgui_editable_get_text (BOBGUI_EDITABLE (entry2));

  bobgui_widget_set_sensitive (button,
                            text[0] != '\0' && g_str_equal (text, text2));
}

static void
button_pressed (BobguiButton *widget,
                BobguiWidget *window)
{
  bobgui_window_destroy (BOBGUI_WINDOW (window));
}

BobguiWidget *
do_password_entry (BobguiWidget *do_widget)
{
  static BobguiWidget *window = NULL;
  BobguiWidget *box;
  BobguiWidget *header;

  if (!window)
    {
      window = bobgui_window_new ();
      bobgui_window_set_display (BOBGUI_WINDOW (window),
                              bobgui_widget_get_display (do_widget));
      header = bobgui_header_bar_new ();
      bobgui_header_bar_set_show_title_buttons (BOBGUI_HEADER_BAR (header), FALSE);
      bobgui_window_set_titlebar (BOBGUI_WINDOW (window), header);
      bobgui_window_set_title (BOBGUI_WINDOW (window), "Choose a Password");
      bobgui_window_set_resizable (BOBGUI_WINDOW (window), FALSE);
      bobgui_window_set_deletable (BOBGUI_WINDOW (window), FALSE);
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);

      box = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 6);
      bobgui_widget_set_margin_start (box, 18);
      bobgui_widget_set_margin_end (box, 18);
      bobgui_widget_set_margin_top (box, 18);
      bobgui_widget_set_margin_bottom (box, 18);
      bobgui_window_set_child (BOBGUI_WINDOW (window), box);

      entry = bobgui_password_entry_new ();
      bobgui_password_entry_set_show_peek_icon (BOBGUI_PASSWORD_ENTRY (entry), TRUE);
      g_object_set (entry,
                    "placeholder-text", "Password",
                    "activates-default", TRUE,
                    NULL);
      bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (entry),
                                      BOBGUI_ACCESSIBLE_PROPERTY_LABEL, "Password",
                                      -1);
      g_signal_connect (entry, "notify::text", G_CALLBACK (update_button), NULL);
      bobgui_box_append (BOBGUI_BOX (box), entry);

      entry2 = bobgui_password_entry_new ();
      bobgui_password_entry_set_show_peek_icon (BOBGUI_PASSWORD_ENTRY (entry2), TRUE);
      g_object_set (entry2,
                    "placeholder-text", "Confirm",
                    "activates-default", TRUE,
                    NULL);
      bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (entry2),
                                      BOBGUI_ACCESSIBLE_PROPERTY_LABEL, "Confirm",
                                      -1);
      g_signal_connect (entry2, "notify::text", G_CALLBACK (update_button), NULL);
      bobgui_box_append (BOBGUI_BOX (box), entry2);

      button = bobgui_button_new_with_mnemonic ("_Done");
      bobgui_widget_add_css_class (button, "suggested-action");
      g_signal_connect (button, "clicked", G_CALLBACK (button_pressed), window);
      bobgui_widget_set_sensitive (button, FALSE);
      bobgui_header_bar_pack_end (BOBGUI_HEADER_BAR (header), button);

      bobgui_window_set_default_widget (BOBGUI_WINDOW (window), button);
    }

  if (!bobgui_widget_get_visible (window))
    bobgui_widget_set_visible (window, TRUE);
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));

  return window;
}
