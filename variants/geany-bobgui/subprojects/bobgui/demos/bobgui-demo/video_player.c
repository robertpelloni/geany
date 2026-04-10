/* Video Player
 * #Keywords: BobguiVideo, BobguiMediaStream, BobguiMediaFile, GdkPaintable
 * #Keywords: BobguiMediaControls
 *
 * This is a simple video player using just BOBGUI widgets.
 */

#include <bobgui/bobgui.h>

static BobguiWidget *window = NULL;

static void
open_dialog_response_cb (GObject *source,
                         GAsyncResult *result,
                         void *user_data)
{
  BobguiFileDialog *dialog = BOBGUI_FILE_DIALOG (source);
  BobguiWidget *video = user_data;
  GFile *file;

  file = bobgui_file_dialog_open_finish (dialog, result, NULL);
  if (file)
    {
      bobgui_video_set_file (BOBGUI_VIDEO (video), file);
      g_object_unref (file);
    }
}

static void
open_clicked_cb (BobguiWidget *button,
                 BobguiWidget *video)
{
  BobguiFileDialog *dialog;
  BobguiFileFilter *filter;
  GListStore *filters;

  dialog = bobgui_file_dialog_new ();
  bobgui_file_dialog_set_title (dialog, "Select a video");

  filters = g_list_store_new (BOBGUI_TYPE_FILE_FILTER);

  filter = bobgui_file_filter_new ();
  bobgui_file_filter_add_pattern (filter, "*");
  bobgui_file_filter_set_name (filter, "All Files");
  g_list_store_append (filters, filter);
  g_object_unref (filter);

  filter = bobgui_file_filter_new ();
  bobgui_file_filter_add_mime_type (filter, "image/*");
  bobgui_file_filter_set_name (filter, "Images");
  g_list_store_append (filters, filter);
  g_object_unref (filter);

  filter = bobgui_file_filter_new ();
  bobgui_file_filter_add_mime_type (filter, "video/*");
  bobgui_file_filter_set_name (filter, "Video");
  g_list_store_append (filters, filter);

  bobgui_file_dialog_set_default_filter (dialog, filter);
  g_object_unref (filter);

  bobgui_file_dialog_set_filters (dialog, G_LIST_MODEL (filters));
  g_object_unref (filters);

  bobgui_file_dialog_open (dialog,
                        BOBGUI_WINDOW (bobgui_widget_get_root (button)),
                        NULL,
                        open_dialog_response_cb, video);
}

static void
logo_clicked_cb (BobguiWidget *button,
                 gpointer   video)
{
  GFile *file;

  file = g_file_new_for_uri ("resource:///images/bobgui-logo.webm");
  bobgui_video_set_file (BOBGUI_VIDEO (video), file);
  g_object_unref (file);
}

static void
bbb_clicked_cb (BobguiWidget *button,
                gpointer   video)
{
  GFile *file;

  file = g_file_new_for_uri ("https://download.blender.org/peach/trailer/trailer_400p.ogg");
  bobgui_video_set_file (BOBGUI_VIDEO (video), file);
  g_object_unref (file);
}

static void
fullscreen_clicked_cb (BobguiWidget *button,
                       gpointer   unused)
{
  BobguiWidget *widget_window = BOBGUI_WIDGET (bobgui_widget_get_root (button));

  bobgui_window_fullscreen (BOBGUI_WINDOW (widget_window));
}

static gboolean
toggle_fullscreen (BobguiWidget *widget,
                   GVariant  *args,
                   gpointer   data)
{
  GdkSurface *surface;
  GdkToplevelState state;

  surface = bobgui_native_get_surface (BOBGUI_NATIVE (widget));
  state = gdk_toplevel_get_state (GDK_TOPLEVEL (surface));

  if (state & GDK_TOPLEVEL_STATE_FULLSCREEN)
    bobgui_window_unfullscreen (BOBGUI_WINDOW (widget));
  else
    bobgui_window_fullscreen (BOBGUI_WINDOW (widget));

  return TRUE;
}

BobguiWidget *
do_video_player (BobguiWidget *do_widget)
{
  BobguiWidget *title;
  BobguiWidget *video;
  BobguiWidget *button;
  BobguiWidget *image;
  BobguiWidget *fullscreen_button;
  BobguiEventController *controller;

  if (!window)
    {
      window = bobgui_window_new ();
      bobgui_window_set_display (BOBGUI_WINDOW (window),
                              bobgui_widget_get_display (do_widget));
      bobgui_window_set_title (BOBGUI_WINDOW (window), "Video Player");
      bobgui_window_set_default_size (BOBGUI_WINDOW (window), 600, 400);
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);

      video = bobgui_video_new ();
      bobgui_video_set_autoplay (BOBGUI_VIDEO (video), TRUE);
      bobgui_video_set_graphics_offload (BOBGUI_VIDEO (video), BOBGUI_GRAPHICS_OFFLOAD_ENABLED);
      bobgui_window_set_child (BOBGUI_WINDOW (window), video);

      title = bobgui_header_bar_new ();
      bobgui_window_set_titlebar (BOBGUI_WINDOW (window), title);

      button = bobgui_button_new_with_mnemonic ("_Open");
      g_signal_connect (button, "clicked", G_CALLBACK (open_clicked_cb), video);
      bobgui_header_bar_pack_start (BOBGUI_HEADER_BAR (title), button);

      button = bobgui_button_new ();
      image = bobgui_image_new_from_resource ("/cursors/images/bobgui_logo_cursor.png");
      bobgui_accessible_update_relation (BOBGUI_ACCESSIBLE (image),
                                      BOBGUI_ACCESSIBLE_RELATION_LABELLED_BY, button, NULL,
                                      -1);
      bobgui_image_set_pixel_size (BOBGUI_IMAGE (image), 24);
      bobgui_button_set_child (BOBGUI_BUTTON (button), image);
      g_signal_connect (button, "clicked", G_CALLBACK (logo_clicked_cb), video);
      bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (button),
                                      BOBGUI_ACCESSIBLE_PROPERTY_LABEL, "BOBGUI Logo",
                                      -1);
      bobgui_header_bar_pack_start (BOBGUI_HEADER_BAR (title), button);

      button = bobgui_button_new ();
      image = bobgui_image_new_from_resource ("/video-player/bbb.png");
      bobgui_accessible_update_relation (BOBGUI_ACCESSIBLE (image),
                                      BOBGUI_ACCESSIBLE_RELATION_LABELLED_BY, button, NULL,
                                      -1);
      bobgui_image_set_pixel_size (BOBGUI_IMAGE (image), 24);
      bobgui_button_set_child (BOBGUI_BUTTON (button), image);
      g_signal_connect (button, "clicked", G_CALLBACK (bbb_clicked_cb), video);
      bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (button),
                                      BOBGUI_ACCESSIBLE_PROPERTY_LABEL, "Big Buck Bunny",
                                      -1);
      bobgui_header_bar_pack_start (BOBGUI_HEADER_BAR (title), button);

      fullscreen_button = bobgui_button_new_from_icon_name ("view-fullscreen-symbolic");
      g_signal_connect (fullscreen_button, "clicked", G_CALLBACK (fullscreen_clicked_cb), NULL);
      bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (fullscreen_button),
                                      BOBGUI_ACCESSIBLE_PROPERTY_LABEL, "Fullscreen",
                                      -1);

      bobgui_header_bar_pack_end (BOBGUI_HEADER_BAR (title), fullscreen_button);

      controller = bobgui_shortcut_controller_new ();
      bobgui_shortcut_controller_set_scope (BOBGUI_SHORTCUT_CONTROLLER (controller),
                                         BOBGUI_SHORTCUT_SCOPE_GLOBAL);
      bobgui_widget_add_controller (window, controller);
      bobgui_shortcut_controller_add_shortcut (BOBGUI_SHORTCUT_CONTROLLER (controller),
           bobgui_shortcut_new (bobgui_keyval_trigger_new (GDK_KEY_F11, 0),
                             bobgui_callback_action_new (toggle_fullscreen, NULL, NULL)));
    }

  if (!bobgui_widget_get_visible (window))
    bobgui_widget_set_visible (window, TRUE);
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));

  return window;
}
