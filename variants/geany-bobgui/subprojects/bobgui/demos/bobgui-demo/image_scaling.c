/* Image Scaling
 * #Keywords: zoom, scale, filter, action, menu
 *
 * The custom widget we create here is similar to a BobguiPicture,
 * but allows setting a zoom level and filtering mode for the
 * displayed paintable.
 *
 * It also demonstrates how to add a context menu to a custom
 * widget and connect it with widget actions.
 *
 * The context menu has items to change the zoom level.
 */

#include <bobgui/bobgui.h>
#include "imageview.h"


static BobguiWidget *window = NULL;
static GCancellable *cancellable = NULL;

static void
load_texture (GTask        *task,
              gpointer      source_object,
              gpointer      task_data,
              GCancellable *cable)
{
  GFile *file = task_data;
  GdkTexture *texture;
  GError *error = NULL;

  texture = gdk_texture_new_from_file (file, &error);

  if (texture)
    g_task_return_pointer (task, texture, g_object_unref);
  else
    g_task_return_error (task, error);
}

static void
set_wait_cursor (BobguiWidget *widget)
{
  bobgui_widget_set_cursor_from_name (BOBGUI_WIDGET (bobgui_widget_get_root (widget)), "wait");
}

static void
unset_wait_cursor (BobguiWidget *widget)
{
  bobgui_widget_set_cursor (BOBGUI_WIDGET (bobgui_widget_get_root (widget)), NULL);
}

static void
texture_loaded (GObject      *source,
                GAsyncResult *result,
                gpointer      data)
{
  GdkTexture *texture;
  GError *error = NULL;

  texture = g_task_propagate_pointer (G_TASK (result), &error);

  if (!texture)
    {
      g_print ("%s\n", error->message);
      g_error_free (error);
      return;
    }

  if (!window)
    {
      g_object_unref (texture);
      return;
    }

  unset_wait_cursor (BOBGUI_WIDGET (data));

  g_object_set (G_OBJECT (data), "texture", texture, NULL);
}

static void
open_file_async (GFile     *file,
                 BobguiWidget *demo)
{
  GTask *task;

  set_wait_cursor (demo);

  task = g_task_new (demo, cancellable, texture_loaded, demo);
  g_task_set_task_data (task, g_object_ref (file), g_object_unref);
  g_task_run_in_thread (task, load_texture);
  g_object_unref (task);
}

static void
open_portland_rose (BobguiWidget *button,
                    BobguiWidget *demo)
{
  GFile *file;

  file = g_file_new_for_uri ("resource:///transparent/portland-rose.jpg");
  open_file_async (file, demo);
  g_object_unref (file);
}

static void
open_large_image (BobguiWidget *button,
                  BobguiWidget *demo)
{
  GFile *file;

  file = g_file_new_for_uri ("resource:///org/bobgui/Demo4/large-image.png");
  open_file_async (file, demo);
  g_object_unref (file);
}

static void
file_opened (GObject      *source,
             GAsyncResult *result,
             void         *data)
{
  GFile *file;
  GError *error = NULL;

  file = bobgui_file_dialog_open_finish (BOBGUI_FILE_DIALOG (source), result, &error);

  if (!file)
    {
      g_print ("%s\n", error->message);
      g_error_free (error);
      return;
    }

  open_file_async (file, data);

  g_object_unref (file);
}

static void
open_file (BobguiWidget *picker,
           BobguiWidget *demo)
{
  BobguiWindow *parent = BOBGUI_WINDOW (bobgui_widget_get_root (picker));
  BobguiFileDialog *dialog;
  BobguiFileFilter *filter;
  GListStore *filters;

  dialog = bobgui_file_dialog_new ();

  filter = bobgui_file_filter_new ();
  bobgui_file_filter_set_name (filter, "Images");
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  bobgui_file_filter_add_pixbuf_formats (filter);
G_GNUC_END_IGNORE_DEPRECATIONS
  filters = g_list_store_new (BOBGUI_TYPE_FILE_FILTER);
  g_list_store_append (filters, filter);
  g_object_unref (filter);

  bobgui_file_dialog_set_filters (dialog, G_LIST_MODEL (filters));
  g_object_unref (filters);

  bobgui_file_dialog_open (dialog, parent, NULL, file_opened, demo);

  g_object_unref (dialog);
}

static void
rotate (BobguiWidget *button,
        BobguiWidget *demo)
{
  float angle;

  g_object_get (demo, "angle", &angle, NULL);

  angle = fmodf (angle + 90.f, 360.f);

  g_object_set (demo, "angle", angle, NULL);
}

static gboolean
transform_to (GBinding     *binding,
              const GValue *src,
              GValue       *dest,
              gpointer      user_data)
{
  double from;
  float to;

  from = g_value_get_double (src);
  to = (float) pow (2., from);
  g_value_set_float (dest, to);

  return TRUE;
}

static gboolean
transform_from (GBinding     *binding,
                const GValue *src,
                GValue       *dest,
                gpointer      user_data)
{
  float to;
  double from;

  to = g_value_get_float (src);
  from = log2 (to);
  g_value_set_double (dest, from);

  return TRUE;
}

static void
free_cancellable (gpointer data)
{
  g_cancellable_cancel (cancellable);
  g_clear_object (&cancellable);
}

static gboolean
cancel_load (BobguiWidget *widget,
             GVariant  *args,
             gpointer   data)
{
  unset_wait_cursor (widget);
  g_cancellable_cancel (G_CANCELLABLE (data));
  return TRUE;
}

BobguiWidget *
do_image_scaling (BobguiWidget *do_widget)
{
  if (!window)
    {
      BobguiWidget *box;
      BobguiWidget *box2;
      BobguiWidget *sw;
      BobguiWidget *vp;
      BobguiWidget *widget;
      BobguiWidget *scale;
      BobguiWidget *dropdown;
      BobguiWidget *button;
      BobguiEventController *controller;

      window = bobgui_window_new ();
      bobgui_window_set_title (BOBGUI_WINDOW (window), "Image Scaling");
      bobgui_window_set_default_size (BOBGUI_WINDOW (window), 600, 400);
      bobgui_window_set_display (BOBGUI_WINDOW (window),
                              bobgui_widget_get_display (do_widget));
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);

      cancellable = g_cancellable_new ();
      g_object_set_data_full (G_OBJECT (window), "cancellable",
                              cancellable, free_cancellable);

      controller = bobgui_shortcut_controller_new ();
      bobgui_shortcut_controller_add_shortcut (BOBGUI_SHORTCUT_CONTROLLER (controller),
                                            bobgui_shortcut_new (
                                                bobgui_keyval_trigger_new (GDK_KEY_Escape, 0),
                                                bobgui_callback_action_new (cancel_load, cancellable, NULL)
                                                ));
      bobgui_shortcut_controller_set_scope (BOBGUI_SHORTCUT_CONTROLLER (controller),
                                         BOBGUI_SHORTCUT_SCOPE_GLOBAL);
      bobgui_widget_add_controller (window, controller);

      box = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
      bobgui_window_set_child (BOBGUI_WINDOW (window), box);

      sw = bobgui_scrolled_window_new ();
      bobgui_widget_set_vexpand (sw, TRUE);
      bobgui_box_append (BOBGUI_BOX (box), sw);

      widget = image_view_new ("/transparent/portland-rose.jpg");
      vp = bobgui_viewport_new (NULL, NULL);
      bobgui_viewport_set_scroll_to_focus (BOBGUI_VIEWPORT (vp), FALSE);

      bobgui_viewport_set_child (BOBGUI_VIEWPORT (vp), widget);
      bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (sw), vp);

      box2 = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
      bobgui_box_append (BOBGUI_BOX (box), box2);

      button = bobgui_button_new_from_icon_name ("document-open-symbolic");
      bobgui_widget_set_tooltip_text (button, "Open File");
      g_signal_connect (button, "clicked", G_CALLBACK (open_file), widget);
      bobgui_box_append (BOBGUI_BOX (box2), button);

      button = bobgui_button_new ();
      bobgui_button_set_child (BOBGUI_BUTTON (button),
                            bobgui_image_new_from_resource ("/org/bobgui/Demo4/portland-rose-thumbnail.png"));
      bobgui_widget_add_css_class (button, "image-button");
      bobgui_widget_set_tooltip_text (button, "Portland Rose");
      g_signal_connect (button, "clicked", G_CALLBACK (open_portland_rose), widget);
      bobgui_box_append (BOBGUI_BOX (box2), button);

      button = bobgui_button_new ();
      bobgui_button_set_child (BOBGUI_BUTTON (button),
                            bobgui_image_new_from_resource ("/org/bobgui/Demo4/large-image-thumbnail.png"));
      bobgui_widget_add_css_class (button, "image-button");
      bobgui_widget_set_tooltip_text (button, "Large image");
      g_signal_connect (button, "clicked", G_CALLBACK (open_large_image), widget);
      bobgui_box_append (BOBGUI_BOX (box2), button);

      button = bobgui_button_new_from_icon_name ("object-rotate-right-symbolic");
      bobgui_widget_set_tooltip_text (button, "Rotate");
      g_signal_connect (button, "clicked", G_CALLBACK (rotate), widget);
      bobgui_box_append (BOBGUI_BOX (box2), button);

      scale = bobgui_scale_new_with_range (BOBGUI_ORIENTATION_HORIZONTAL, -10., 10., 0.1);
      bobgui_scale_add_mark (BOBGUI_SCALE (scale), 0., BOBGUI_POS_TOP, NULL);
      bobgui_widget_set_tooltip_text (scale, "Zoom");
      bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (scale),
                                      BOBGUI_ACCESSIBLE_PROPERTY_LABEL, "Zoom",
                                      -1);
      bobgui_range_set_value (BOBGUI_RANGE (scale), 0.);
      bobgui_widget_set_hexpand (scale, TRUE);
      bobgui_box_append (BOBGUI_BOX (box2), scale);

      dropdown = bobgui_drop_down_new (G_LIST_MODEL (bobgui_string_list_new ((const char *[]){ "Linear", "Nearest", "Trilinear", NULL })), NULL);
      bobgui_widget_set_tooltip_text (dropdown, "Filter");
      bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (dropdown),
                                      BOBGUI_ACCESSIBLE_PROPERTY_LABEL, "Filter",
                                      -1);
      bobgui_box_append (BOBGUI_BOX (box2), dropdown);

      g_object_bind_property (dropdown, "selected", widget, "filter", G_BINDING_DEFAULT);

      g_object_bind_property_full (bobgui_range_get_adjustment (BOBGUI_RANGE (scale)), "value",
                                   widget, "scale",
                                   G_BINDING_BIDIRECTIONAL,
                                   transform_to,
                                   transform_from,
                                   NULL, NULL);
    }

  if (!bobgui_widget_get_visible (window))
    bobgui_widget_set_visible (window, TRUE);
  else
    {
      bobgui_window_destroy (BOBGUI_WINDOW (window));
    }

  return window;
}
