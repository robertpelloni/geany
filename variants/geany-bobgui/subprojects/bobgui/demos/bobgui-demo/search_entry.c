/* Entry/Search Entry
 *
 * BobguiSearchEntry provides an entry that is ready for search.
 *
 * Search entries have their "search-changed" signal delayed and
 * should be used when the search operation is slow, such as big
 * datasets to search, or online searches.
 *
 * BobguiSearchBar allows have a hidden search entry that 'springs
 * into action' upon keyboard input.
 */

#include <bobgui/bobgui.h>

static void
search_changed_cb (BobguiSearchEntry *entry,
                   BobguiLabel       *result_label)
{
  const char *text;
  text = bobgui_editable_get_text (BOBGUI_EDITABLE (entry));
  bobgui_label_set_text (result_label, text ? text : "");
}

BobguiWidget *
do_search_entry (BobguiWidget *do_widget)
{
  static BobguiWidget *window = NULL;
  BobguiWidget *vbox;
  BobguiWidget *hbox;
  BobguiWidget *box;
  BobguiWidget *label;
  BobguiWidget *entry;
  BobguiWidget *searchbar;
  BobguiWidget *button;
  BobguiWidget *header;

  if (!window)
    {
      window = bobgui_window_new ();
      bobgui_window_set_title (BOBGUI_WINDOW (window), "Type to Search");
      bobgui_window_set_transient_for (BOBGUI_WINDOW (window), BOBGUI_WINDOW (do_widget));
      bobgui_window_set_resizable (BOBGUI_WINDOW (window), FALSE);
      bobgui_widget_set_size_request (window, 200, -1);
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);

      header = bobgui_header_bar_new ();
      bobgui_window_set_titlebar (BOBGUI_WINDOW (window), header);

      vbox = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
      bobgui_window_set_child (BOBGUI_WINDOW (window), vbox);

      entry = bobgui_search_entry_new ();
      bobgui_widget_set_halign (entry, BOBGUI_ALIGN_CENTER);
      searchbar = bobgui_search_bar_new ();
      bobgui_search_bar_connect_entry (BOBGUI_SEARCH_BAR (searchbar), BOBGUI_EDITABLE (entry));
      bobgui_search_bar_set_show_close_button (BOBGUI_SEARCH_BAR (searchbar), FALSE);
      bobgui_search_bar_set_child (BOBGUI_SEARCH_BAR (searchbar), entry);
      bobgui_box_append (BOBGUI_BOX (vbox), searchbar);

      /* Hook the search bar to key presses */
      bobgui_search_bar_set_key_capture_widget (BOBGUI_SEARCH_BAR (searchbar), window);

      box = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 18);
      bobgui_widget_set_margin_start (box, 18);
      bobgui_widget_set_margin_end (box, 18);
      bobgui_widget_set_margin_top (box, 18);
      bobgui_widget_set_margin_bottom (box, 18);
      bobgui_box_append (BOBGUI_BOX (vbox), box);

      /* Toggle button */
      button = bobgui_toggle_button_new ();
      bobgui_button_set_icon_name (BOBGUI_BUTTON (button), "system-search-symbolic");
      g_object_bind_property (button, "active",
                              searchbar, "search-mode-enabled",
                              G_BINDING_BIDIRECTIONAL);
      bobgui_header_bar_pack_end (BOBGUI_HEADER_BAR (header), button);

      /* Result */
      hbox = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 10);
      bobgui_box_append (BOBGUI_BOX (box), hbox);

      label = bobgui_label_new ("Searching for:");
      bobgui_label_set_xalign (BOBGUI_LABEL (label), 0.0);
      bobgui_box_append (BOBGUI_BOX (hbox), label);

      label = bobgui_label_new ("");
      bobgui_box_append (BOBGUI_BOX (hbox), label);

      g_signal_connect (entry, "search-changed",
                        G_CALLBACK (search_changed_cb), label);
    }

  if (!bobgui_widget_get_visible (window))
    bobgui_widget_set_visible (window, TRUE);
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));

  return window;
}
