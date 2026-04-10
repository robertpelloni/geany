/* Aspect Frame
 *
 * BobguiAspectFrame makes sure its child gets a specific aspect ratio.
 */

#include <bobgui/bobgui.h>

static void
setup_ui (BobguiWidget *window)
{
  BobguiWidget *box, *vbox;
  BobguiWidget *aspect_frame;
  BobguiWidget *label;
  BobguiWidget *scale;

  scale = bobgui_scale_new_with_range (BOBGUI_ORIENTATION_HORIZONTAL, 0.2, 5.0, 0.1);
  bobgui_scale_set_draw_value (BOBGUI_SCALE (scale), TRUE);
  bobgui_scale_set_digits (BOBGUI_SCALE (scale), 2);
  bobgui_range_set_value (BOBGUI_RANGE (scale), 1.5);

  box = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
  bobgui_widget_set_vexpand (box, TRUE);

  label = bobgui_label_new ("This wrapping label is always given a specific aspect ratio by the aspect frame. The specific aspect ratio can be controlled by dragging the slider. The picture is always given its natural aspect ratio. Try resizing the window to see how the two aspect frames react to different available sizes, and how the box distributes space between them.");
  bobgui_label_set_wrap (BOBGUI_LABEL (label), TRUE);
  bobgui_label_set_max_width_chars (BOBGUI_LABEL (label), 50);
  aspect_frame = bobgui_aspect_frame_new (0.5, 0.5, 1.5, FALSE);
  g_object_bind_property (bobgui_range_get_adjustment (BOBGUI_RANGE (scale)), "value",
                          aspect_frame, "ratio",
                          0);
  bobgui_aspect_frame_set_child (BOBGUI_ASPECT_FRAME (aspect_frame), label);
  bobgui_widget_set_hexpand (aspect_frame, TRUE);
  bobgui_box_append (BOBGUI_BOX (box), aspect_frame);

  aspect_frame = bobgui_aspect_frame_new (0.5, 0.5, 0, TRUE);
  bobgui_aspect_frame_set_child (BOBGUI_ASPECT_FRAME (aspect_frame),
                              bobgui_picture_new_for_resource ("/aspect_frame/ducky.png"));
  bobgui_box_append (BOBGUI_BOX (box), aspect_frame);

  vbox = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 12);
  bobgui_box_append (BOBGUI_BOX (vbox), scale);
  bobgui_box_append (BOBGUI_BOX (vbox), box);

  bobgui_window_set_child (BOBGUI_WINDOW (window), vbox);
}

BobguiWidget*
do_aspect_frame (BobguiWidget *do_widget)
{
  static BobguiWidget *window = NULL;

  if (!window)
    {
      BobguiCssProvider *provider;

      window = bobgui_window_new ();
      bobgui_window_set_display (BOBGUI_WINDOW (window),
                              bobgui_widget_get_display (do_widget));

      bobgui_window_set_title (BOBGUI_WINDOW (window), "Aspect Frame");
      bobgui_widget_add_css_class (window, "aspect-frame-demo");
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *) &window);

      provider = bobgui_css_provider_new ();
      bobgui_css_provider_load_from_resource (provider, "/aspect_frame/aspect_frame.css");
      bobgui_style_context_add_provider_for_display (bobgui_widget_get_display (do_widget),
                                                  BOBGUI_STYLE_PROVIDER (provider),
                                                  800);
      g_object_unref (provider);

      setup_ui (window);
    }

  if (!bobgui_widget_get_visible (window))
    bobgui_widget_set_visible (window, TRUE);
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));

  return window;
}
