/* Overlay/Transparency
 * #Keywords: BobguiOverlay, BobguiSnapshot, blur, backdrop-filter
 *
 * Blur the background behind an overlay.
 */

#include <bobgui/bobgui.h>

BobguiWidget *
do_transparent (BobguiWidget *do_widget)
{
  static BobguiWidget *window = NULL;
  static BobguiCssProvider *css_provider = NULL;

  if (!css_provider)
    {
      css_provider = bobgui_css_provider_new ();
      bobgui_css_provider_load_from_resource (css_provider, "/transparent/transparent.css");

      bobgui_style_context_add_provider_for_display (gdk_display_get_default (),
                                                  BOBGUI_STYLE_PROVIDER (css_provider),
                                                  BOBGUI_STYLE_PROVIDER_PRIORITY_APPLICATION);
    }

  if (!window)
    {
      BobguiWidget *overlay;
      BobguiWidget *button;
      BobguiWidget *box;
      BobguiWidget *picture;

      window = bobgui_window_new ();
      bobgui_window_set_display (BOBGUI_WINDOW (window),
                              bobgui_widget_get_display (do_widget));
      bobgui_window_set_default_size (BOBGUI_WINDOW (window), 450, 450);
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);

      bobgui_window_set_title (BOBGUI_WINDOW (window), "Transparency");

      overlay = bobgui_overlay_new ();
      bobgui_window_set_child (BOBGUI_WINDOW (window), overlay);

      box = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
      bobgui_widget_set_hexpand (box, TRUE);
      bobgui_box_set_homogeneous (BOBGUI_BOX (box), TRUE);
      bobgui_widget_add_css_class (box, "floating-controls");

      bobgui_widget_set_halign (box, BOBGUI_ALIGN_FILL);
      bobgui_widget_set_valign (box, BOBGUI_ALIGN_END);

      bobgui_overlay_add_overlay (BOBGUI_OVERLAY (overlay), box);

      button = bobgui_button_new_with_label ("Don't click this button!");
      bobgui_widget_add_css_class (button, "blur-overlay");

      bobgui_box_append (BOBGUI_BOX (box), button);

      button = bobgui_button_new_with_label ("Maybe this one?");
      bobgui_widget_add_css_class (button, "blur-overlay");

      bobgui_box_append (BOBGUI_BOX (box), button);

      picture = bobgui_picture_new_for_resource ("/transparent/portland-rose.jpg");
      bobgui_picture_set_content_fit (BOBGUI_PICTURE (picture), BOBGUI_CONTENT_FIT_COVER);
      bobgui_overlay_set_child (BOBGUI_OVERLAY (overlay), picture);
    }

  if (!bobgui_widget_get_visible (window))
    bobgui_widget_set_visible (window, TRUE);
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));

  return window;
}
