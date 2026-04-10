#include <bobgui/bobgui.h>

#define DEMO_TYPE_IMAGE (demo_image_get_type ())

G_DECLARE_FINAL_TYPE (DemoImage, demo_image, DEMO, IMAGE, BobguiWidget)

struct _DemoImage {
  BobguiWidget parent_instance;

  GdkTexture *texture;
};

G_DEFINE_TYPE (DemoImage, demo_image, BOBGUI_TYPE_WIDGET)

static void
demo_image_init (DemoImage *demo)
{
}

static void
demo_image_dispose (GObject *object)
{
  DemoImage *demo = DEMO_IMAGE (object);

  g_clear_object (&demo->texture);

  G_OBJECT_CLASS (demo_image_parent_class)->dispose (object);
}

static void
demo_image_measure (BobguiWidget      *widget,
                    BobguiOrientation  orientation,
                    int             for_size,
                    int            *minimum,
                    int            *natural,
                    int            *minimum_baseline,
                    int            *natural_baseline)
{
  DemoImage *demo = DEMO_IMAGE (widget);
  double scale;

  g_print ("measure\n");
  scale = gdk_surface_get_scale (bobgui_native_get_surface (bobgui_widget_get_native (widget)));

  if (orientation == BOBGUI_ORIENTATION_VERTICAL)
    {
      *minimum = *natural = (int) ceil (gdk_texture_get_height (demo->texture) / scale);
      g_print ("requesting height: %d\n", *minimum);
    }
  else
    {
      *minimum = *natural = (int) ceil (gdk_texture_get_width (demo->texture) / scale);
      g_print ("requesting width: %d\n", *minimum);
    }
}

static void
demo_image_snapshot (BobguiWidget   *widget,
                     BobguiSnapshot *snapshot)
{
  DemoImage *demo = DEMO_IMAGE (widget);
  BobguiNative *native = bobgui_widget_get_native (widget);
  graphene_point_t point;
  double scale;
  double ox, oy;
  double x, y, width, height;

  g_print ("snapshot\n");

  scale = gdk_surface_get_scale (bobgui_native_get_surface (native));

  g_print ("scale %f\n", scale);

  /* width and height that give us 1-1 mapping to device pixels */
  width = gdk_texture_get_width (demo->texture) / scale;
  height = gdk_texture_get_height (demo->texture) / scale;

  bobgui_native_get_surface_transform (native, &ox, &oy);

  g_print ("surface transform %f %f\n", ox, oy);

  x = (bobgui_widget_get_width (widget) - width) / 2;
  y = (bobgui_widget_get_height (widget) - height) / 2;

  g_print ("texture origin in widget coordinates: %f %f\n", x, y);

  if (!bobgui_widget_compute_point (widget, BOBGUI_WIDGET (native), &GRAPHENE_POINT_INIT (x, y), &point))
    return;

  x = point.x;
  y = point.y;

  g_print ("in window (app) coordinates: %f %f\n", x, y);

  x += ox;
  y += oy;

  g_print ("in surface (app) coordinates: %f %f\n", x, y);

  x *= scale;
  y *= scale;

  g_print ("in surface (device) coordinates: %f %f\n", x, y);

  /* Now x, y are the surface (device) coordinates of the widget's origin */

  /* Round up to the next full device pixel */

  x = ceil (x);
  y = ceil (y);

  g_print ("rounded up: %f %f\n", x, y);

  /* And back to widget coordinates */

  x /= scale;
  y /= scale;

  x -= ox;
  y -= oy;

  if (!bobgui_widget_compute_point (widget, BOBGUI_WIDGET (native), &GRAPHENE_POINT_INIT (0, 0), &point))
    return;

  x -= point.x;
  y -= point.y;

  g_print ("bounds: %f %f %f %f\n", x, y, width, height);

  bobgui_snapshot_append_texture (snapshot, demo->texture,
                               &GRAPHENE_RECT_INIT (x, y, width, height));
}

static void
notify_scale (GObject *object,
              GParamSpec *pspec,
              BobguiWidget *widget)
{
  g_print ("scale change!\n");

  bobgui_widget_queue_resize (widget);
}

static void
demo_image_realize (BobguiWidget *widget)
{
  BobguiNative *native;
  GdkSurface *surface;

  BOBGUI_WIDGET_CLASS (demo_image_parent_class)->realize (widget);

  g_print ("realize\n");

  native = bobgui_widget_get_native (widget);
  surface = bobgui_native_get_surface (native);
  g_signal_connect (surface, "notify::scale",
                    G_CALLBACK (notify_scale), widget);
}

static void
demo_image_unrealize (BobguiWidget *widget)
{
  BobguiNative *native;
  GdkSurface *surface;

  native = bobgui_widget_get_native (widget);
  surface = bobgui_native_get_surface (native);
  g_signal_handlers_disconnect_by_func (surface, notify_scale, widget);

  BOBGUI_WIDGET_CLASS (demo_image_parent_class)->unrealize (widget);
}

static void
demo_image_class_init (DemoImageClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (class);

  object_class->dispose = demo_image_dispose;

  widget_class->realize = demo_image_realize;
  widget_class->unrealize = demo_image_unrealize;

  widget_class->measure = demo_image_measure;
  widget_class->snapshot = demo_image_snapshot;

  bobgui_widget_class_set_layout_manager_type (widget_class, BOBGUI_TYPE_BIN_LAYOUT);
}

static BobguiWidget *
demo_image_new (GdkTexture *texture)
{
  DemoImage *demo;

  demo = g_object_new (DEMO_TYPE_IMAGE, NULL);

  demo->texture = g_object_ref (texture);

  g_print ("texture size %dx%d\n",
           gdk_texture_get_width (texture),
           gdk_texture_get_height (texture));

  return BOBGUI_WIDGET (demo);
}

static GdkTexture *
make_checkerboard_texture (int width, int height)
{
  guint32 *data, *row;
  GBytes *bytes;
  GdkTexture *texture;

  data = (guint32 *) g_new (guint32, width * height);

  for (int y = 0; y < height; y++)
    {
      row = data + y * width;
      for (int x = 0; x < width; x++)
        {
          if ((x + y) % 2)
            row[x] = 0xffffffff;
          else
            row[x] = 0xff000000;
        }
    }

  bytes = g_bytes_new_take (data, height * width * 4);

  texture = gdk_memory_texture_new (width, height, GDK_MEMORY_DEFAULT, bytes, width * 4);

  g_bytes_unref (bytes);

  return texture;
}

static gboolean
toggle_fullscreen (BobguiWidget *widget,
                   GVariant  *args,
                   gpointer   data)
{
  BobguiWindow *window = BOBGUI_WINDOW (widget);

  if (bobgui_window_is_fullscreen (window))
    bobgui_window_unfullscreen (window);
  else
    bobgui_window_fullscreen (window);

  return TRUE;
}

int
main (int argc, char *argv[])
{
  BobguiWidget *window;
  GdkTexture *texture;
  GError *error = NULL;
  BobguiEventController *controller;
  BobguiShortcutTrigger *trigger;
  BobguiShortcutAction *action;
  BobguiShortcut *shortcut;

  bobgui_init ();

  if (argc > 1)
    {
      texture = gdk_texture_new_from_filename (argv[1], &error);
      if (!texture)
        g_error ("%s", error->message);
    }
  else
    {
      texture = make_checkerboard_texture (100, 100);
    }

  window = bobgui_window_new ();

  controller = bobgui_shortcut_controller_new ();
  trigger = bobgui_keyval_trigger_new (GDK_KEY_F11, GDK_NO_MODIFIER_MASK);
  action = bobgui_callback_action_new (toggle_fullscreen, NULL, NULL);
  shortcut = bobgui_shortcut_new (trigger, action);
  bobgui_shortcut_controller_add_shortcut (BOBGUI_SHORTCUT_CONTROLLER (controller), shortcut);
  bobgui_widget_add_controller (window, controller);

  bobgui_window_set_child (BOBGUI_WINDOW (window), demo_image_new (texture));

  g_object_unref (texture);

  bobgui_window_present (BOBGUI_WINDOW (window));

  while (g_list_model_get_n_items (bobgui_window_get_toplevels ()) > 0)
    g_main_context_iteration (NULL, TRUE);

  return 0;
}
