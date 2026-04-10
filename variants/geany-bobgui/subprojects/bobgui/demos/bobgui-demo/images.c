/* Images
 * #Keywords: GdkPaintable, BobguiWidgetPaintable
 *
 * BobguiImage and BobguiPicture are used to display an image; the image can be
 * in a number of formats.
 *
 * BobguiImage is the widget used to display icons or images that should be
 * sized and styled like an icon, while BobguiPicture is used for images
 * that should be displayed as-is.
 *
 * This demo code shows some of the more obscure cases, in the simple
 * case a call to bobgui_picture_new_for_file() or
 * bobgui_image_new_from_icon_name() is all you need.
 */

#include <bobgui/bobgui.h>
#include <glib/gstdio.h>
#include <stdio.h>
#include <errno.h>
#include "pixbufpaintable.h"

static BobguiWidget *window = NULL;

static void
toggle_sensitivity_callback (BobguiWidget *togglebutton,
                             gpointer   user_data)
{
  BobguiWidget *child;

  for (child = bobgui_widget_get_first_child (BOBGUI_WIDGET (user_data));
       child != NULL;
       child = bobgui_widget_get_next_sibling (child))
    {
      /* don't disable our toggle */
      if (child != togglebutton)
        bobgui_widget_set_sensitive (child, !bobgui_toggle_button_get_active (BOBGUI_TOGGLE_BUTTON (togglebutton)));
    }
}


BobguiWidget *
do_images (BobguiWidget *do_widget)
{
  BobguiWidget *video;
  BobguiWidget *frame;
  BobguiWidget *vbox;
  BobguiWidget *hbox;
  BobguiWidget *base_vbox;
  BobguiWidget *image;
  BobguiWidget *picture;
  BobguiWidget *label;
  BobguiWidget *button;
  GdkPaintable *paintable;
  BobguiWidget *state;
  GIcon *gicon;

  if (!window)
    {
      window = bobgui_window_new ();
      bobgui_window_set_display (BOBGUI_WINDOW (window),
                              bobgui_widget_get_display (do_widget));
      bobgui_window_set_title (BOBGUI_WINDOW (window), "Images");
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer*)&window);

      base_vbox = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 8);
      bobgui_widget_set_margin_start (base_vbox, 16);
      bobgui_widget_set_margin_end (base_vbox, 16);
      bobgui_widget_set_margin_top (base_vbox, 16);
      bobgui_widget_set_margin_bottom (base_vbox, 16);
      bobgui_window_set_child (BOBGUI_WINDOW (window), base_vbox);

      hbox = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 16);
      bobgui_box_append (BOBGUI_BOX (base_vbox), hbox);

      vbox = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 8);
      bobgui_box_append (BOBGUI_BOX (hbox), vbox);

      label = bobgui_label_new ("Image from a resource");
      bobgui_widget_add_css_class (label, "heading");
      bobgui_box_append (BOBGUI_BOX (vbox), label);

      frame = bobgui_frame_new (NULL);
      bobgui_widget_set_halign (frame, BOBGUI_ALIGN_CENTER);
      bobgui_widget_set_valign (frame, BOBGUI_ALIGN_CENTER);
      bobgui_box_append (BOBGUI_BOX (vbox), frame);

      image = bobgui_image_new_from_resource ("/images/org.bobgui.Demo4.svg");
      bobgui_image_set_icon_size (BOBGUI_IMAGE (image), BOBGUI_ICON_SIZE_LARGE);

      bobgui_frame_set_child (BOBGUI_FRAME (frame), image);


      /* Animation */

      label = bobgui_label_new ("Animation from a resource");
      bobgui_widget_add_css_class (label, "heading");
      bobgui_box_append (BOBGUI_BOX (vbox), label);

      frame = bobgui_frame_new (NULL);
      bobgui_widget_set_halign (frame, BOBGUI_ALIGN_CENTER);
      bobgui_widget_set_valign (frame, BOBGUI_ALIGN_CENTER);
      bobgui_box_append (BOBGUI_BOX (vbox), frame);

      paintable = pixbuf_paintable_new_from_resource ("/images/floppybuddy.gif");
      picture = bobgui_picture_new_for_paintable (paintable);
      g_object_unref (paintable);

      bobgui_frame_set_child (BOBGUI_FRAME (frame), picture);

      /* Symbolic icon */

      label = bobgui_label_new ("Symbolic themed icon");
      bobgui_widget_add_css_class (label, "heading");
      bobgui_box_append (BOBGUI_BOX (vbox), label);

      frame = bobgui_frame_new (NULL);
      bobgui_widget_set_halign (frame, BOBGUI_ALIGN_CENTER);
      bobgui_widget_set_valign (frame, BOBGUI_ALIGN_CENTER);
      bobgui_box_append (BOBGUI_BOX (vbox), frame);

      gicon = g_themed_icon_new_with_default_fallbacks ("battery-level-10-charging-symbolic");
      image = bobgui_image_new_from_gicon (gicon);
      bobgui_image_set_icon_size (BOBGUI_IMAGE (image), BOBGUI_ICON_SIZE_LARGE);
      g_object_unref (gicon);

      bobgui_frame_set_child (BOBGUI_FRAME (frame), image);


      /* Stateful */

      vbox = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 8);
      bobgui_box_append (BOBGUI_BOX (hbox), vbox);

      label = bobgui_label_new ("Stateful icon");
      bobgui_widget_add_css_class (label, "heading");
      bobgui_box_append (BOBGUI_BOX (vbox), label);

      frame = bobgui_frame_new (NULL);
      bobgui_widget_set_halign (frame, BOBGUI_ALIGN_CENTER);
      bobgui_widget_set_valign (frame, BOBGUI_ALIGN_CENTER);
      bobgui_box_append (BOBGUI_BOX (vbox), frame);

      paintable = GDK_PAINTABLE (bobgui_svg_new_from_resource ("/images/stateful.gpa"));
      bobgui_svg_play (BOBGUI_SVG (paintable));
      bobgui_svg_set_state (BOBGUI_SVG (paintable), 0);
      image = bobgui_image_new_from_paintable (paintable);
      bobgui_image_set_pixel_size (BOBGUI_IMAGE (image), 128);

      bobgui_frame_set_child (BOBGUI_FRAME (frame), image);

      state = bobgui_switch_new ();
      bobgui_widget_set_halign (state, BOBGUI_ALIGN_START);
      g_object_bind_property (state, "active",
                              paintable, "state",
                              G_BINDING_DEFAULT);
      bobgui_box_append (BOBGUI_BOX (vbox), state);
      g_object_unref (paintable);


      /* Animations */

      label = bobgui_label_new ("Path animation");
      bobgui_widget_add_css_class (label, "heading");
      bobgui_box_append (BOBGUI_BOX (vbox), label);

      frame = bobgui_frame_new (NULL);
      bobgui_widget_set_halign (frame, BOBGUI_ALIGN_CENTER);
      bobgui_widget_set_valign (frame, BOBGUI_ALIGN_CENTER);
      bobgui_box_append (BOBGUI_BOX (vbox), frame);

      paintable = GDK_PAINTABLE (bobgui_svg_new_from_resource ("/images/animated.gpa"));
      bobgui_svg_play (BOBGUI_SVG (paintable));
      bobgui_svg_set_state (BOBGUI_SVG (paintable), 0);
      image = bobgui_image_new_from_paintable (paintable);
      bobgui_image_set_pixel_size (BOBGUI_IMAGE (image), 128);

      bobgui_frame_set_child (BOBGUI_FRAME (frame), image);
      g_object_unref (paintable);

      /* Video */

      vbox = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 8);
      bobgui_box_append (BOBGUI_BOX (hbox), vbox);

      label = bobgui_label_new ("Displaying video");
      bobgui_widget_add_css_class (label, "heading");
      bobgui_box_append (BOBGUI_BOX (vbox), label);

      frame = bobgui_frame_new (NULL);
      bobgui_widget_set_halign (frame, BOBGUI_ALIGN_CENTER);
      bobgui_widget_set_valign (frame, BOBGUI_ALIGN_CENTER);
      bobgui_box_append (BOBGUI_BOX (vbox), frame);

      video = bobgui_video_new_for_resource ("/images/bobgui-logo.webm");
      bobgui_media_stream_set_loop (bobgui_video_get_media_stream (BOBGUI_VIDEO (video)), TRUE);
      bobgui_frame_set_child (BOBGUI_FRAME (frame), video);

      /* Widget paintables */
      vbox = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 8);
      bobgui_box_append (BOBGUI_BOX (hbox), vbox);

      label = bobgui_label_new ("BobguiWidgetPaintable");
      bobgui_widget_add_css_class (label, "heading");
      bobgui_box_append (BOBGUI_BOX (vbox), label);

      paintable = bobgui_widget_paintable_new (do_widget);
      picture = bobgui_picture_new_for_paintable (paintable);
      bobgui_widget_set_size_request (picture, 100, 100);
      bobgui_widget_set_valign (picture, BOBGUI_ALIGN_START);
      bobgui_box_append (BOBGUI_BOX (vbox), picture);

      /* Sensitivity control */
      button = bobgui_toggle_button_new_with_mnemonic ("_Insensitive");
      bobgui_widget_set_halign (button, BOBGUI_ALIGN_END);
      bobgui_widget_set_valign (button, BOBGUI_ALIGN_END);
      bobgui_widget_set_vexpand (button, TRUE);
      bobgui_box_append (BOBGUI_BOX (base_vbox), button);

      g_signal_connect (button, "toggled",
                        G_CALLBACK (toggle_sensitivity_callback),
                        base_vbox);
    }

  if (!bobgui_widget_get_visible (window))
    bobgui_widget_set_visible (window, TRUE);
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));

  return window;
}
