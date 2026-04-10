/* Header Bar
 * #Keywords: BobguiWindowHandle, BobguiWindowControls
 *
 * BobguiHeaderBar is a container that is suitable for implementing
 * window titlebars. One of its features is that it can position
 * a title centered with regard to the full width, regardless of
 * variable-width content at the left or right.
 *
 * It is commonly used with bobgui_window_set_titlebar()
 */

#include <bobgui/bobgui.h>

BobguiWidget *
do_headerbar (BobguiWidget *do_widget)
{
  static BobguiWidget *window = NULL;
  BobguiWidget *header;
  BobguiWidget *button;
  BobguiWidget *box;
  BobguiWidget *content;

  if (!window)
    {
      window = bobgui_window_new ();
      bobgui_window_set_display (BOBGUI_WINDOW (window),  bobgui_widget_get_display (do_widget));
      bobgui_window_set_title (BOBGUI_WINDOW (window), "Welcome to the Hotel California");
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);

      bobgui_window_set_default_size (BOBGUI_WINDOW (window), 600, 400);

      header = bobgui_header_bar_new ();

      button = bobgui_button_new_from_icon_name ("mail-send-receive-symbolic");
      bobgui_widget_set_tooltip_text (button, "Check out");
      bobgui_header_bar_pack_end (BOBGUI_HEADER_BAR (header), button);

      box = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
      bobgui_widget_add_css_class (box, "linked");
      button = bobgui_button_new_from_icon_name ("go-previous-symbolic");
      bobgui_widget_set_tooltip_text (button, "Back");
      bobgui_box_append (BOBGUI_BOX (box), button);
      button = bobgui_button_new_from_icon_name ("go-next-symbolic");
      bobgui_widget_set_tooltip_text (button, "Forward");
      bobgui_box_append (BOBGUI_BOX (box), button);

      bobgui_header_bar_pack_start (BOBGUI_HEADER_BAR (header), box);
      button = bobgui_switch_new ();
      bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (button),
                                      BOBGUI_ACCESSIBLE_PROPERTY_LABEL, "Change something",
                                      -1);
      bobgui_header_bar_pack_start (BOBGUI_HEADER_BAR (header), button);

      bobgui_window_set_titlebar (BOBGUI_WINDOW (window), header);

      content = bobgui_text_view_new ();
      bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (content),
                                      BOBGUI_ACCESSIBLE_PROPERTY_LABEL, "Content",
                                      -1);
      bobgui_window_set_child (BOBGUI_WINDOW (window), content);
    }

  if (!bobgui_widget_get_visible (window))
    bobgui_widget_set_visible (window, TRUE);
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));

  return window;
}
