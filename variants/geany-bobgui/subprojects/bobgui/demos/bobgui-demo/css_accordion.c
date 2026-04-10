/* Theming/CSS Accordion
 *
 * A simple accordion demo written using CSS transitions and multiple backgrounds
 *
 */

#include <bobgui/bobgui.h>

static void
destroy_provider (BobguiWidget        *window,
                  BobguiStyleProvider *provider)
{
  bobgui_style_context_remove_provider_for_display (bobgui_widget_get_display (window), provider);
}

BobguiWidget *
do_css_accordion (BobguiWidget *do_widget)
{
  static BobguiWidget *window = NULL;

  if (!window)
    {
      BobguiWidget *container, *styled_box, *child;
      BobguiCssProvider *provider;

      window = bobgui_window_new ();
      bobgui_window_set_title (BOBGUI_WINDOW (window), "CSS Accordion");
      bobgui_window_set_transient_for (BOBGUI_WINDOW (window), BOBGUI_WINDOW (do_widget));
      bobgui_window_set_default_size (BOBGUI_WINDOW (window), 600, 300);
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);

      styled_box = bobgui_frame_new (NULL);
      bobgui_window_set_child (BOBGUI_WINDOW (window), styled_box);
      bobgui_widget_add_css_class (styled_box, "accordion");
      container = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
      bobgui_widget_set_halign (container, BOBGUI_ALIGN_CENTER);
      bobgui_widget_set_valign (container, BOBGUI_ALIGN_CENTER);
      bobgui_frame_set_child (BOBGUI_FRAME (styled_box), container);

      child = bobgui_button_new_with_label ("This");
      bobgui_box_append (BOBGUI_BOX (container), child);

      child = bobgui_button_new_with_label ("Is");
      bobgui_box_append (BOBGUI_BOX (container), child);

      child = bobgui_button_new_with_label ("A");
      bobgui_box_append (BOBGUI_BOX (container), child);

      child = bobgui_button_new_with_label ("CSS");
      bobgui_box_append (BOBGUI_BOX (container), child);

      child = bobgui_button_new_with_label ("Accordion");
      bobgui_box_append (BOBGUI_BOX (container), child);

      child = bobgui_button_new_with_label (":-)");
      bobgui_box_append (BOBGUI_BOX (container), child);

      provider = bobgui_css_provider_new ();
      bobgui_css_provider_load_from_resource (provider, "/css_accordion/css_accordion.css");

      bobgui_style_context_add_provider_for_display (bobgui_widget_get_display (window),
                                                  BOBGUI_STYLE_PROVIDER (provider),
                                                  BOBGUI_STYLE_PROVIDER_PRIORITY_APPLICATION);

      g_signal_connect (window, "destroy",
                        G_CALLBACK (destroy_provider), provider);
      g_object_unref (provider);
    }

  if (!bobgui_widget_get_visible (window))
    bobgui_widget_set_visible (window, TRUE);
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));

  return window;
}
