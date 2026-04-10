/* Written by Florian Muellner
 * https://bugzilla.gnome.org/show_bug.cgi?id=761760
 */

#include <bobgui/bobgui.h>

static void
on_activate (GApplication *app,
             gpointer      data)
{
  static BobguiWidget *window = NULL;

  if (window == NULL)
    {
      BobguiWidget *header, *sidebar_toggle, *animation_switch;
      BobguiWidget *hbox, *revealer, *sidebar, *img;

      window = bobgui_application_window_new (BOBGUI_APPLICATION (app));
      bobgui_window_set_default_size (BOBGUI_WINDOW (window), 400, 300);

      /* titlebar */
      header = bobgui_header_bar_new ();
      bobgui_window_set_titlebar (BOBGUI_WINDOW (window), header);

      sidebar_toggle = bobgui_toggle_button_new_with_label ("Show Sidebar");
      bobgui_header_bar_pack_start (BOBGUI_HEADER_BAR (header), sidebar_toggle);

      animation_switch = bobgui_switch_new ();
      bobgui_widget_set_valign (animation_switch, BOBGUI_ALIGN_CENTER);
      bobgui_header_bar_pack_end (BOBGUI_HEADER_BAR (header), animation_switch);
      bobgui_header_bar_pack_end (BOBGUI_HEADER_BAR (header),
                               bobgui_label_new ("Animations"));

      /* content */
      hbox = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
      bobgui_window_set_child (BOBGUI_WINDOW (window), hbox);

      revealer = bobgui_revealer_new ();
      bobgui_revealer_set_transition_type (BOBGUI_REVEALER (revealer),
                                        BOBGUI_REVEALER_TRANSITION_TYPE_SLIDE_LEFT);
      bobgui_box_append (BOBGUI_BOX (hbox), revealer);

      sidebar = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
      bobgui_widget_set_size_request (sidebar, 150, -1);
      bobgui_widget_add_css_class (sidebar, "sidebar");
      bobgui_revealer_set_child (BOBGUI_REVEALER (revealer), sidebar);

      img = bobgui_image_new ();
      g_object_set (img, "icon-name", "face-smile-symbolic",
                         "pixel-size", 128,
                         "hexpand", TRUE,
                         "halign", BOBGUI_ALIGN_CENTER,
                         "valign", BOBGUI_ALIGN_CENTER,
                         NULL);
      bobgui_box_append (BOBGUI_BOX (hbox), img);

      g_object_bind_property (sidebar_toggle, "active",
                              revealer, "reveal-child",
                              G_BINDING_SYNC_CREATE);
      g_object_bind_property (bobgui_settings_get_default(), "bobgui-enable-animations",
                              animation_switch, "active",
                              G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);
                              
    }
  bobgui_window_present (BOBGUI_WINDOW (window));
}

int
main (int argc, char *argv[])
{
  BobguiApplication *app = bobgui_application_new ("org.bobgui.fmuellner.Revealer", 0);

  g_signal_connect (app, "activate", G_CALLBACK (on_activate), NULL);

  return g_application_run (G_APPLICATION (app), argc, argv);
}

