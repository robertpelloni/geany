#include "demoimage.h"
#include <glib/gi18n.h>

struct _DemoImage {
  BobguiWidget parent_instance;
 
  BobguiWidget *image;
  BobguiWidget *popover;
};

enum {
  PROP_ICON_NAME = 1
};

G_DEFINE_TYPE(DemoImage, demo_image, BOBGUI_TYPE_WIDGET)

static GdkPaintable *
get_image_paintable (BobguiImage *image)
{
  const char *icon_name;
  BobguiIconTheme *icon_theme;
  BobguiIconPaintable *icon;

  switch (bobgui_image_get_storage_type (image))
    {
    case BOBGUI_IMAGE_PAINTABLE:
      return g_object_ref (bobgui_image_get_paintable (image));
    case BOBGUI_IMAGE_ICON_NAME:
      icon_name = bobgui_image_get_icon_name (image);
      icon_theme = bobgui_icon_theme_get_for_display (bobgui_widget_get_display (BOBGUI_WIDGET (image)));
      icon = bobgui_icon_theme_lookup_icon (icon_theme,
                                         icon_name,
                                         NULL,
                                         48, 1,
                                         bobgui_widget_get_direction (BOBGUI_WIDGET (image)),
                                         0);
      if (icon == NULL)
        return NULL;
      return GDK_PAINTABLE (icon);

    case BOBGUI_IMAGE_EMPTY:
    case BOBGUI_IMAGE_GICON:
    default:
      g_warning ("Image storage type %d not handled",
                 bobgui_image_get_storage_type (image));
      return NULL;
    }
}

static void
update_drag_icon (DemoImage   *demo,
                  BobguiDragIcon *icon)
{
  const char *icon_name;
  GdkPaintable *paintable;
  BobguiWidget *image;

  switch (bobgui_image_get_storage_type (BOBGUI_IMAGE (demo->image)))
    {
    case BOBGUI_IMAGE_PAINTABLE:
      paintable = bobgui_image_get_paintable (BOBGUI_IMAGE (demo->image));
      image = bobgui_image_new_from_paintable (paintable);
      break;
    case BOBGUI_IMAGE_ICON_NAME:
      icon_name = bobgui_image_get_icon_name (BOBGUI_IMAGE (demo->image));
      image = bobgui_image_new_from_icon_name (icon_name);
      break;
    case BOBGUI_IMAGE_EMPTY:
    case BOBGUI_IMAGE_GICON:
    default:
      g_warning ("Image storage type %d not handled",
                 bobgui_image_get_storage_type (BOBGUI_IMAGE (demo->image)));
      return;
    }

  bobgui_image_set_pixel_size (BOBGUI_IMAGE (image),
                            bobgui_image_get_pixel_size (BOBGUI_IMAGE (demo->image)));

  bobgui_drag_icon_set_child (icon, image);
}

static void
drag_begin (BobguiDragSource *source,
            GdkDrag       *drag,
            gpointer       data)
{
  BobguiWidget *widget = bobgui_event_controller_get_widget (BOBGUI_EVENT_CONTROLLER (source));
  DemoImage *demo = DEMO_IMAGE (widget);

  update_drag_icon (demo, BOBGUI_DRAG_ICON (bobgui_drag_icon_get_for_drag (drag)));
}

static GdkContentProvider *
prepare_drag (BobguiDragSource *source,
              double         x,
              double         y,
              gpointer       data)
{
  BobguiWidget *widget = bobgui_event_controller_get_widget (BOBGUI_EVENT_CONTROLLER (source));
  DemoImage *demo = DEMO_IMAGE (widget);
  GdkPaintable *paintable = get_image_paintable (BOBGUI_IMAGE (demo->image));

  /* Textures can be serialized, paintables can't, so special case the textures */
  if (GDK_IS_TEXTURE (paintable))
    return gdk_content_provider_new_typed (GDK_TYPE_TEXTURE, paintable);
  else
    return gdk_content_provider_new_typed (GDK_TYPE_PAINTABLE, paintable);
}

static gboolean
drag_drop (BobguiDropTarget *dest,
           const GValue  *value,
           double         x,
           double         y,
           gpointer       data)
{
  BobguiWidget *widget = bobgui_event_controller_get_widget (BOBGUI_EVENT_CONTROLLER (dest));
  DemoImage *demo = DEMO_IMAGE (widget);
  GdkPaintable *paintable = g_value_get_object (value);

  bobgui_image_set_from_paintable (BOBGUI_IMAGE (demo->image), paintable);

  return TRUE;
}

static void
copy_image (BobguiWidget *widget,
            const char *action_name,
            GVariant *parameter)
{
  GdkClipboard *clipboard = bobgui_widget_get_clipboard (widget);
  DemoImage *demo = DEMO_IMAGE (widget);
  GdkPaintable *paintable = get_image_paintable (BOBGUI_IMAGE (demo->image));
  GValue value = G_VALUE_INIT;

  /* Textures can be serialized, paintables can't, so special case the textures */
  if (GDK_IS_TEXTURE (paintable))
    g_value_init (&value, GDK_TYPE_TEXTURE);
  else
    g_value_init (&value, GDK_TYPE_PAINTABLE);
  g_value_set_object (&value, paintable);
  gdk_clipboard_set_value (clipboard, &value);
  g_value_unset (&value);

  if (paintable)
    g_object_unref (paintable);
}

static void
paste_image_cb (GObject      *source,
                GAsyncResult *result,
                gpointer      data)
{
  GdkClipboard *clipboard = GDK_CLIPBOARD (source);
  DemoImage *demo = DEMO_IMAGE (data);
  const GValue *value;

  value = gdk_clipboard_read_value_finish (clipboard, result, NULL);
  if (value == NULL)
    {
      bobgui_widget_error_bell (BOBGUI_WIDGET (demo));
      g_object_unref (demo);
      return;
    }

  bobgui_image_set_from_paintable (BOBGUI_IMAGE (demo->image), g_value_get_object (value));
  g_object_unref (demo);
}

static void
paste_image (BobguiWidget *widget,
             const char *action_name,
             GVariant *parameter)
{
  GdkClipboard *clipboard = bobgui_widget_get_clipboard (widget);
  GType type;

  if (gdk_content_formats_contain_gtype (gdk_clipboard_get_formats (clipboard), GDK_TYPE_TEXTURE))
    type = GDK_TYPE_TEXTURE;
  else
    type = GDK_TYPE_PAINTABLE;

  gdk_clipboard_read_value_async (clipboard, 
                                  type,
                                  G_PRIORITY_DEFAULT,
                                  NULL,
                                  paste_image_cb,
                                  g_object_ref (widget));
}

static void
pressed_cb (BobguiGesture *gesture,
            int         n_press,
            double      x,
            double      y,
            gpointer    data)
{
  DemoImage *demo = DEMO_IMAGE (bobgui_event_controller_get_widget (BOBGUI_EVENT_CONTROLLER (gesture)));

  bobgui_popover_popup (BOBGUI_POPOVER (demo->popover));
}

static void
demo_image_init (DemoImage *demo)
{
  GMenu *menu;
  GMenuItem *item;
  BobguiDragSource *source;
  BobguiDropTarget *dest;
  BobguiGesture *gesture;

  demo->image = bobgui_image_new ();
  bobgui_image_set_pixel_size (BOBGUI_IMAGE (demo->image), 48);
  bobgui_widget_set_parent (demo->image, BOBGUI_WIDGET (demo));

  menu = g_menu_new (); 
  item = g_menu_item_new (_("_Copy"), "clipboard.copy");
  g_menu_append_item (menu, item);

  item = g_menu_item_new (_("_Paste"), "clipboard.paste");
  g_menu_append_item (menu, item);

  demo->popover = bobgui_popover_menu_new_from_model (G_MENU_MODEL (menu));
  bobgui_widget_set_parent (demo->popover, BOBGUI_WIDGET (demo));

  source = bobgui_drag_source_new ();
  g_signal_connect (source, "prepare", G_CALLBACK (prepare_drag), NULL);
  g_signal_connect (source, "drag-begin", G_CALLBACK (drag_begin), NULL);
  bobgui_widget_add_controller (BOBGUI_WIDGET (demo), BOBGUI_EVENT_CONTROLLER (source));

  dest = bobgui_drop_target_new (GDK_TYPE_PAINTABLE, GDK_ACTION_COPY);
  g_signal_connect (dest, "drop", G_CALLBACK (drag_drop), NULL);
  bobgui_widget_add_controller (BOBGUI_WIDGET (demo), BOBGUI_EVENT_CONTROLLER (dest));

  gesture = bobgui_gesture_click_new ();
  bobgui_gesture_single_set_button (BOBGUI_GESTURE_SINGLE (gesture), GDK_BUTTON_SECONDARY);
  g_signal_connect (gesture, "pressed", G_CALLBACK (pressed_cb), NULL);
  bobgui_widget_add_controller (BOBGUI_WIDGET (demo), BOBGUI_EVENT_CONTROLLER (gesture));
}

static void
demo_image_dispose (GObject *object)
{
  DemoImage *demo = DEMO_IMAGE (object);

  g_clear_pointer (&demo->image, bobgui_widget_unparent);
  g_clear_pointer (&demo->popover, bobgui_widget_unparent);

  G_OBJECT_CLASS (demo_image_parent_class)->dispose (object);
}

static void
demo_image_get_property (GObject    *object,
                         guint       prop_id,
                         GValue     *value,
                         GParamSpec *pspec)
{
  DemoImage *demo = DEMO_IMAGE (object);

  switch (prop_id)
    {
    case PROP_ICON_NAME:
      g_value_set_string (value, bobgui_image_get_icon_name (BOBGUI_IMAGE (demo->image)));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
demo_image_set_property (GObject      *object,
                         guint         prop_id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
  DemoImage *demo = DEMO_IMAGE (object);

  switch (prop_id)
    {
    case PROP_ICON_NAME:
      bobgui_image_set_from_icon_name (BOBGUI_IMAGE (demo->image),
                                    g_value_get_string (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
demo_image_class_init (DemoImageClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (class);

  object_class->dispose = demo_image_dispose;
  object_class->get_property = demo_image_get_property;
  object_class->set_property = demo_image_set_property;

  g_object_class_install_property (object_class, PROP_ICON_NAME,
      g_param_spec_string ("icon-name", "Icon name", "Icon name",
                           NULL, G_PARAM_READWRITE));
                       
  bobgui_widget_class_set_layout_manager_type (widget_class, BOBGUI_TYPE_BIN_LAYOUT);

  bobgui_widget_class_install_action (widget_class, "clipboard.copy", NULL, copy_image);
  bobgui_widget_class_install_action (widget_class, "clipboard.paste", NULL, paste_image);
}

BobguiWidget *
demo_image_new (const char *icon_name)
{
  return g_object_new (DEMO_TYPE_IMAGE, "icon-name", icon_name, NULL);
}
