/* Paintable/SVG
 *
 * This demo shows using BobguiSvg to display an SVG image in
 * a BobguiPicture that can be scaled by resizing the window.
 */

#include <bobgui/bobgui.h>

static void
image_clicked (BobguiGestureClick *click,
               int n_press,
               double x,
               double y,
               BobguiImage *image)
{
  BobguiSvg *paintable = BOBGUI_SVG (bobgui_image_get_paintable (image));
  guint state = bobgui_svg_get_state (paintable);

  if (state < 63)
    bobgui_svg_set_state (paintable, state + 1);
  else
    bobgui_svg_set_state (paintable, 0);
}

static void
open_response_cb (GObject      *source,
                  GAsyncResult *result,
                  void         *data)
{
  BobguiFileDialog *dialog = BOBGUI_FILE_DIALOG (source);
  BobguiWidget *window = data;
  GFile *file;

  file = bobgui_file_dialog_open_finish (dialog, result, NULL);
  if (file)
    {
      GBytes *bytes;
      GdkPaintable *paintable;
      BobguiWidget *image;
      BobguiEventController *controller;

      bytes = g_file_load_bytes (file, NULL, NULL, NULL);
      paintable = GDK_PAINTABLE (bobgui_svg_new_from_bytes (bytes));
      g_bytes_unref (bytes);

      image = bobgui_picture_new ();
      bobgui_window_set_child (BOBGUI_WINDOW (window), image);

      controller = BOBGUI_EVENT_CONTROLLER (bobgui_gesture_click_new ());
      g_signal_connect (controller, "pressed",
                        G_CALLBACK (image_clicked), image);
      bobgui_widget_add_controller (image, controller);

      bobgui_picture_set_paintable (BOBGUI_PICTURE (image), paintable);

      g_object_unref (paintable);
      g_object_unref (file);
    }
}

static void
show_file_open (BobguiWidget *button,
                BobguiWidget *window)
{
  BobguiFileFilter *filter;
  BobguiFileDialog *dialog;
  GListStore *filters;

  dialog = bobgui_file_dialog_new ();
  bobgui_file_dialog_set_title (dialog, "Open svg image");

  filter = bobgui_file_filter_new ();
  bobgui_file_filter_add_mime_type (filter, "image/svg+xml");
  bobgui_file_filter_add_mime_type (filter, "image/x-bobgui-path-animation");
  bobgui_file_filter_add_pattern (filter, "*.gpa");
  filters = g_list_store_new (BOBGUI_TYPE_FILE_FILTER);
  g_list_store_append (filters, filter);
  g_object_unref (filter);
  bobgui_file_dialog_set_filters (dialog, G_LIST_MODEL (filters));
  g_object_unref (filters);

  bobgui_file_dialog_open (dialog,
                        BOBGUI_WINDOW (bobgui_widget_get_root (button)),
                        NULL,
                        open_response_cb, window);
}

static BobguiWidget *window;

BobguiWidget *
do_paintable_svg (BobguiWidget *do_widget)
{
  BobguiWidget *header;
  BobguiWidget *image;
  BobguiWidget *button;
  GdkPaintable *paintable;

  if (!window)
    {
      window = bobgui_window_new ();
      header = bobgui_header_bar_new ();
      bobgui_window_set_titlebar (BOBGUI_WINDOW (window), header);
      bobgui_window_set_default_size (BOBGUI_WINDOW (window), 330, 330);
      bobgui_window_set_title (BOBGUI_WINDOW (window), "Paintable — SVG");
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);

      button = bobgui_button_new_with_mnemonic ("_Open");
      bobgui_header_bar_pack_start (BOBGUI_HEADER_BAR (header), button);

      image = bobgui_picture_new ();
      bobgui_widget_set_size_request (image, 16, 16);

      g_signal_connect (button, "clicked", G_CALLBACK (show_file_open), window);

      bobgui_window_set_child (BOBGUI_WINDOW (window), image);

      paintable = GDK_PAINTABLE (bobgui_svg_new_from_resource ("/paintable_svg/org.bobgui.bobgui4.NodeEditor.Devel.svg"));
      bobgui_picture_set_paintable (BOBGUI_PICTURE (image), paintable);
      g_object_unref (paintable);
    }

  if (!bobgui_widget_get_visible (window))
    bobgui_window_present (BOBGUI_WINDOW (window));
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));

  return window;
}
