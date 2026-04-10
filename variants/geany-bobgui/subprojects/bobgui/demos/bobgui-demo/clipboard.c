/* Clipboard
 * #Keywords: drag-and-drop, dnd
 *
 * GdkClipboard is used for clipboard handling. This demo shows how to
 * copy and paste text, images, colors or files to and from the clipboard.
 *
 * You can also use Drag-And-Drop to copy the data from the source to the
 * target.
 */

#include <glib/gi18n.h>
#include <bobgui/bobgui.h>
#include <string.h>
#include "demoimage.h"

static void
copy_button_clicked (BobguiStack *source_stack,
                     gpointer  user_data)
{
  GdkClipboard *clipboard;
  const char *visible_child_name;
  BobguiWidget *visible_child;

  clipboard = bobgui_widget_get_clipboard (BOBGUI_WIDGET (source_stack));

  visible_child = bobgui_stack_get_visible_child (source_stack);
  visible_child_name = bobgui_stack_get_visible_child_name (source_stack);

  if (strcmp (visible_child_name, "Text") == 0)
    {
      gdk_clipboard_set_text (clipboard, bobgui_editable_get_text (BOBGUI_EDITABLE (visible_child)));
    }
  else if (strcmp (visible_child_name, "Image") == 0)
    {
      BobguiWidget *child;

      for (child = bobgui_widget_get_first_child (visible_child); child; child = bobgui_widget_get_next_sibling (child))
        {
          if (bobgui_toggle_button_get_active (BOBGUI_TOGGLE_BUTTON (child)))
            {
              BobguiWidget *image = bobgui_widget_get_first_child (child);
              GdkPaintable *paintable = bobgui_image_get_paintable (BOBGUI_IMAGE (image));

              if (GDK_IS_TEXTURE (paintable))
                gdk_clipboard_set (clipboard, GDK_TYPE_TEXTURE, paintable);
              else
                gdk_clipboard_set (clipboard, GDK_TYPE_PAINTABLE, paintable);
              break;
            }
        }
    }
  else if (strcmp (visible_child_name, "Color") == 0)
    {
      const GdkRGBA *color;

      color = bobgui_color_dialog_button_get_rgba (BOBGUI_COLOR_DIALOG_BUTTON (visible_child));
      gdk_clipboard_set (clipboard, GDK_TYPE_RGBA, color);
    }
  else if (strcmp (visible_child_name, "File") == 0)
    {
      gdk_clipboard_set (clipboard, G_TYPE_FILE, g_object_get_data (G_OBJECT (visible_child), "file"), NULL);
    }
  else
    {
      g_print ("TODO");
    }
}

static void
present_value (BobguiStack     *dest_stack,
               const GValue *value)
{
  BobguiWidget *child;

  if (G_VALUE_HOLDS (value, G_TYPE_FILE))
    {
      GFile *file;

      bobgui_stack_set_visible_child_name (dest_stack, "File");
      child = bobgui_stack_get_visible_child (dest_stack);

      file = g_value_get_object (value);
      g_object_set (child, "label", g_file_peek_path (file), NULL);
    }
  else if (G_VALUE_HOLDS (value, GDK_TYPE_RGBA))
    {
      GdkRGBA *color;

      bobgui_stack_set_visible_child_name (dest_stack, "Color");
      child = bobgui_widget_get_first_child (bobgui_stack_get_visible_child (dest_stack));

      color = g_value_get_boxed (value);
      g_object_set (child, "rgba", color, NULL);
    }
  else if (G_VALUE_HOLDS (value, GDK_TYPE_TEXTURE) ||
           G_VALUE_HOLDS (value, GDK_TYPE_PAINTABLE))
    {
      GdkPaintable *paintable;

      bobgui_stack_set_visible_child_name (dest_stack, "Image");
      child = bobgui_stack_get_visible_child (dest_stack);

      paintable = g_value_get_object (value);
      g_object_set (child, "paintable", paintable, NULL);
    }
  else if (G_VALUE_HOLDS (value, G_TYPE_STRING))
    {
      bobgui_stack_set_visible_child_name (dest_stack, "Text");
      child = bobgui_stack_get_visible_child (dest_stack);

      bobgui_label_set_label (BOBGUI_LABEL (child), g_value_get_string (value));
    }
}

static void
paste_received (GObject      *source_object,
                GAsyncResult *result,
                gpointer      user_data)
{
  BobguiStack *dest_stack = user_data;
  GdkClipboard *clipboard;
  const GValue *value;
  GError *error = NULL;

  clipboard = GDK_CLIPBOARD (source_object);

  value = gdk_clipboard_read_value_finish (clipboard, result, &error);
  if (value)
    {
      present_value (dest_stack, value);
    }
  else
    {
      g_print ("%s\n", error->message);
      g_error_free (error);
    }
}

static void
paste_button_clicked (BobguiStack *dest_stack,
                      gpointer  user_data)
{
  GdkClipboard *clipboard;
  GdkContentFormats *formats;

  clipboard = bobgui_widget_get_clipboard (BOBGUI_WIDGET (dest_stack));
  formats = gdk_clipboard_get_formats (clipboard);

  if (gdk_content_formats_contain_gtype (formats, GDK_TYPE_TEXTURE))
    gdk_clipboard_read_value_async (clipboard, GDK_TYPE_TEXTURE, 0, NULL, paste_received, dest_stack);
  else if (gdk_content_formats_contain_gtype (formats, GDK_TYPE_PAINTABLE))
    gdk_clipboard_read_value_async (clipboard, GDK_TYPE_PAINTABLE, 0, NULL, paste_received, dest_stack);
  else if (gdk_content_formats_contain_gtype (formats, GDK_TYPE_RGBA))
    gdk_clipboard_read_value_async (clipboard, GDK_TYPE_RGBA, 0, NULL, paste_received, dest_stack);
  else if (gdk_content_formats_contain_gtype (formats, G_TYPE_FILE))
    gdk_clipboard_read_value_async (clipboard, G_TYPE_FILE, 0, NULL, paste_received, dest_stack);
  else if (gdk_content_formats_contain_gtype (formats, G_TYPE_STRING))
    gdk_clipboard_read_value_async (clipboard, G_TYPE_STRING, 0, NULL, paste_received, dest_stack);
}

static void
update_copy_button_sensitivity (BobguiWidget *source_stack)
{
  BobguiButton *copy_button;
  const char *visible_child_name;
  BobguiWidget *visible_child;
  gboolean sensitive;

  copy_button = BOBGUI_BUTTON (g_object_get_data (G_OBJECT (source_stack), "copy-button"));

  visible_child = bobgui_stack_get_visible_child (BOBGUI_STACK (source_stack));
  visible_child_name = bobgui_stack_get_visible_child_name (BOBGUI_STACK (source_stack));
  if (strcmp (visible_child_name, "Text") == 0)
    {
      sensitive = strlen (bobgui_editable_get_text (BOBGUI_EDITABLE (visible_child))) > 0;
    }
  else if (strcmp (visible_child_name, "Color") == 0 ||
           strcmp (visible_child_name, "Image") == 0)
    {
      sensitive = TRUE;
    }
  else if (strcmp (visible_child_name, "File") == 0)
    {
      sensitive = g_object_get_data (G_OBJECT (visible_child), "file") != NULL;
    }
  else
    {
      sensitive = FALSE;
    }

  bobgui_widget_set_sensitive (BOBGUI_WIDGET (copy_button), sensitive);
}

static void
source_changed_cb (BobguiButton  *copy_button,
                   GParamSpec *pspec,
                   BobguiWidget  *source_stack)
{
  update_copy_button_sensitivity (source_stack);
}

static void
text_changed_cb (BobguiButton  *copy_button,
                 GParamSpec *pspec,
                 BobguiWidget  *entry)
{
  update_copy_button_sensitivity (bobgui_widget_get_ancestor (entry, BOBGUI_TYPE_STACK));
}

static void
file_button_set_file (BobguiButton *button,
                      GFile     *file)
{
  bobgui_label_set_label (BOBGUI_LABEL (bobgui_button_get_child (button)), g_file_peek_path (file));
  g_object_set_data_full (G_OBJECT (button), "file", g_object_ref (file), g_object_unref);
}

static void
file_chooser_response (GObject *source,
                       GAsyncResult *result,
                       gpointer user_data)
{
  BobguiFileDialog *dialog = BOBGUI_FILE_DIALOG (source);
  BobguiButton *button = BOBGUI_BUTTON (user_data);
  GFile *file;

  file = bobgui_file_dialog_open_finish (dialog, result, NULL);
  if (file)
    {
      file_button_set_file (button, file);
      g_object_unref (file);

      update_copy_button_sensitivity (bobgui_widget_get_ancestor (BOBGUI_WIDGET (button), BOBGUI_TYPE_STACK));
    }
}

static void
open_file_cb (BobguiWidget *button)
{
  BobguiFileDialog *dialog;

  dialog = bobgui_file_dialog_new ();

  bobgui_file_dialog_open (dialog,
                        BOBGUI_WINDOW (bobgui_widget_get_ancestor (button, BOBGUI_TYPE_WINDOW)),
                        NULL,
                        file_chooser_response, button);

  g_object_unref (dialog);
}

static void
folder_chooser_response (GObject *source,
                         GAsyncResult *result,
                         gpointer user_data)
{
  BobguiFileDialog *dialog = BOBGUI_FILE_DIALOG (source);
  BobguiButton *button = BOBGUI_BUTTON (user_data);
  GFile *file;

  file = bobgui_file_dialog_select_folder_finish (dialog, result, NULL);
  if (file)
    {
      file_button_set_file (button, file);
      g_object_unref (file);

      update_copy_button_sensitivity (bobgui_widget_get_ancestor (BOBGUI_WIDGET (button), BOBGUI_TYPE_STACK));
    }
}

static void
open_folder_cb (BobguiWidget *button)
{
  BobguiFileDialog *dialog;

  dialog = bobgui_file_dialog_new ();

  bobgui_file_dialog_select_folder (dialog,
                                 BOBGUI_WINDOW (bobgui_widget_get_ancestor (button, BOBGUI_TYPE_WINDOW)),
                                 NULL,
                                 folder_chooser_response, button);

  g_object_unref (dialog);
}

static void
update_paste_button_sensitivity (GdkClipboard *clipboard,
                                 BobguiWidget    *paste_button)
{
  GdkContentFormats *formats;
  gboolean sensitive = FALSE;

  formats = gdk_clipboard_get_formats (clipboard);

  if (gdk_content_formats_contain_gtype (formats, G_TYPE_FILE) ||
      gdk_content_formats_contain_gtype (formats, GDK_TYPE_RGBA) ||
      gdk_content_formats_contain_gtype (formats, GDK_TYPE_TEXTURE) ||
      gdk_content_formats_contain_gtype (formats, GDK_TYPE_PAINTABLE) ||
      gdk_content_formats_contain_gtype (formats, G_TYPE_STRING))
    sensitive = TRUE;

  bobgui_widget_set_sensitive (paste_button, sensitive);
}

static void
unset_clipboard_handler (gpointer data)
{
  GdkClipboard *clipboard = bobgui_widget_get_clipboard (BOBGUI_WIDGET (data));

  g_signal_handlers_disconnect_by_func (clipboard, update_paste_button_sensitivity, data);
}

static gboolean
on_drop (BobguiStack      *dest_stack,
         const GValue  *value,
         double         x,
         double         y,
         gpointer       data)
{
  present_value (dest_stack, value);
  return TRUE;
}

static GdkContentProvider *
drag_prepare (BobguiDragSource *drag_source,
              double         x,
              double         y,
              gpointer       data)
{
  BobguiWidget *button;
  GValue value = G_VALUE_INIT;

  button = bobgui_event_controller_get_widget (BOBGUI_EVENT_CONTROLLER (drag_source));

  if (BOBGUI_IS_TOGGLE_BUTTON (button))
    {
      BobguiWidget *image = bobgui_widget_get_first_child (button);
      GdkPaintable *paintable = bobgui_image_get_paintable (BOBGUI_IMAGE (image));

      if (GDK_IS_TEXTURE (paintable))
        {
          g_value_init (&value, GDK_TYPE_TEXTURE);
          g_value_set_object (&value, paintable);
        }
      else
        {
          g_value_init (&value, GDK_TYPE_PAINTABLE);
          g_value_set_object (&value, paintable);
        }
    }
  else
    {
      GFile *file = g_object_get_data (G_OBJECT (button), "file");

      if (file)
        {
          g_value_init (&value, G_TYPE_FILE);
          g_value_set_object (&value, file);
        }
      else
        return NULL;
    }

  return gdk_content_provider_new_for_value (&value);
}

BobguiWidget *
do_clipboard (BobguiWidget *do_widget)
{
  static BobguiWidget *window = NULL;

  if (!window)
    {
      BobguiBuilderScope *scope;
      BobguiBuilder *builder;
      BobguiWidget *button;

      scope = bobgui_builder_cscope_new ();
      bobgui_builder_cscope_add_callback (scope, copy_button_clicked);
      bobgui_builder_cscope_add_callback (scope, paste_button_clicked);
      bobgui_builder_cscope_add_callback (scope, source_changed_cb);
      bobgui_builder_cscope_add_callback (scope, text_changed_cb);
      bobgui_builder_cscope_add_callback (scope, open_file_cb);
      bobgui_builder_cscope_add_callback (scope, open_folder_cb);
      bobgui_builder_cscope_add_callback (scope, on_drop);
      bobgui_builder_cscope_add_callback (scope, drag_prepare);
      builder = bobgui_builder_new ();
      bobgui_builder_set_scope (builder, scope);
      bobgui_builder_add_from_resource (builder, "/clipboard/clipboard.ui", NULL);

      window = BOBGUI_WIDGET (bobgui_builder_get_object (builder, "window"));
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);
      bobgui_window_set_display (BOBGUI_WINDOW (window), bobgui_widget_get_display (do_widget));

      button = BOBGUI_WIDGET (bobgui_builder_get_object (builder, "copy_button"));
      g_object_set_data (bobgui_builder_get_object (builder, "source_stack"), "copy-button", button);

      button = BOBGUI_WIDGET (bobgui_builder_get_object (builder, "paste_button"));
      g_signal_connect (bobgui_widget_get_clipboard (button), "changed",
                        G_CALLBACK (update_paste_button_sensitivity), button);
      g_object_set_data_full (G_OBJECT (button), "clipboard-handler", button, unset_clipboard_handler);

      g_object_unref (builder);
      g_object_unref (scope);
    }

  if (!bobgui_widget_get_visible (window))
    bobgui_widget_set_visible (window, TRUE);
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));

  return window;
}
