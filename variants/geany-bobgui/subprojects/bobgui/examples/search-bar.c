#include <bobgui/bobgui.h>

static void
activate_cb (BobguiApplication *app,
             gpointer        user_data)
{
  BobguiWidget *window;
  BobguiWidget *search_bar;
  BobguiWidget *box;
  BobguiWidget *entry;
  BobguiWidget *menu_button;

  window = bobgui_application_window_new (app);
  bobgui_window_present (BOBGUI_WINDOW (window));

  search_bar = bobgui_search_bar_new ();
  bobgui_widget_set_valign (search_bar, BOBGUI_ALIGN_START);
  bobgui_window_set_child (BOBGUI_WINDOW (window), search_bar);

  box = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 6);
  bobgui_search_bar_set_child (BOBGUI_SEARCH_BAR (search_bar), box);

  entry = bobgui_search_entry_new ();
  bobgui_widget_set_hexpand (entry, TRUE);
  bobgui_box_append (BOBGUI_BOX (box), entry);

  menu_button = bobgui_menu_button_new ();
  bobgui_box_append (BOBGUI_BOX (box), menu_button);

  bobgui_search_bar_connect_entry (BOBGUI_SEARCH_BAR (search_bar), BOBGUI_EDITABLE (entry));
  bobgui_search_bar_set_key_capture_widget (BOBGUI_SEARCH_BAR (search_bar), window);
}

int
main (int argc,
    char *argv[])
{
  BobguiApplication *app;

  app = bobgui_application_new ("org.bobgui.Example.BobguiSearchBar", G_APPLICATION_DEFAULT_FLAGS);
  g_signal_connect (app, "activate", G_CALLBACK (activate_cb), NULL);

  return g_application_run (G_APPLICATION (app), argc, argv);
}
