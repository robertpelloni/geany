/* Benchmark/Frames
 *
 * This demo is intentionally as simple as possible, to see what
 * framerate the windowing system can deliver on its own.
 *
 * It does nothing but change the drawn color, for every frame.
 */

#include <bobgui/bobgui.h>

typedef struct
{
  BobguiWidget parent_instance;

  GdkRGBA color1;
  GdkRGBA color2;
  guint64 time2;
  float t;

  guint tick_cb;
} ColorWidget;

typedef struct
{
  BobguiWidgetClass parent_class;
} ColorWidgetClass;

G_DEFINE_TYPE (ColorWidget, color_widget, BOBGUI_TYPE_WIDGET)

#define TIME_SPAN (3.0 * G_TIME_SPAN_SECOND)

static gboolean
change_color (BobguiWidget     *widget,
              GdkFrameClock *frame_clock,
              gpointer       data)
{
  ColorWidget *color = (ColorWidget *)widget;
  gint64 time;

  time = gdk_frame_clock_get_frame_time (frame_clock);

  if (time >= color->time2)
    {
      color->time2 = time + TIME_SPAN;

      color->color1 = color->color2;
      color->color2.red = g_random_double_range (0, 1);
      color->color2.green = g_random_double_range (0, 1);
      color->color2.blue = g_random_double_range (0, 1);
      color->color2.alpha = 1;
    }

  color->t = 1 - (color->time2 - time) / TIME_SPAN;

  bobgui_widget_queue_draw (widget);

  return G_SOURCE_CONTINUE;
}

static void
color_widget_snapshot (BobguiWidget   *widget,
                       BobguiSnapshot *snapshot)
{
  ColorWidget *color = (ColorWidget *)widget;
  float w, h;
  GdkRGBA c;

  w = bobgui_widget_get_width (widget);
  h = bobgui_widget_get_height (widget);

  c.red = (1 - color->t) * color->color1.red + color->t * color->color2.red;
  c.green = (1 - color->t) * color->color1.green + color->t * color->color2.green;
  c.blue = (1 - color->t) * color->color1.blue + color->t * color->color2.blue;
  c.alpha = 1;

  bobgui_snapshot_append_color (snapshot, &c, &GRAPHENE_RECT_INIT (0, 0, w, h));
}

static void
color_widget_init (ColorWidget *color)
{
  bobgui_widget_add_tick_callback (BOBGUI_WIDGET (color), change_color, NULL, NULL);
  bobgui_widget_set_hexpand (BOBGUI_WIDGET (color), TRUE);
  bobgui_widget_set_vexpand (BOBGUI_WIDGET (color), TRUE);
}

static void
color_widget_class_init (ColorWidgetClass *class)
{
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (class);

  widget_class->snapshot = color_widget_snapshot;
}

BobguiWidget *
color_widget_new (void)
{
  return g_object_new (color_widget_get_type (), NULL);
}

static gboolean
update_fps_label (gpointer data)
{
  BobguiWidget *label = BOBGUI_WIDGET (data);
  GdkFrameClock *frame_clock;

  frame_clock = bobgui_widget_get_frame_clock (label);

  if (frame_clock)
    {
      char *fps;

      fps = g_strdup_printf ("%.2f fps", gdk_frame_clock_get_fps (frame_clock));
      bobgui_label_set_label (BOBGUI_LABEL (label), fps);
      g_free (fps);
    }
  else
    bobgui_label_set_label (BOBGUI_LABEL (label), "");

  return G_SOURCE_CONTINUE;
}

static void
remove_id (gpointer data)
{
  guint id = GPOINTER_TO_UINT (data);

  g_source_remove (id);
}

BobguiWidget *
do_frames (BobguiWidget *do_widget)
{
  static BobguiWidget *window = NULL;

  if (!window)
    {
      BobguiBuilder *builder;
      BobguiWidget *box;
      BobguiWidget *label;
      guint id;

      builder = bobgui_builder_new_from_resource ("/frames/frames.ui");
      window = BOBGUI_WIDGET (bobgui_builder_get_object (builder, "window"));
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);
      bobgui_window_set_display (BOBGUI_WINDOW (window),
                              bobgui_widget_get_display (do_widget));

      label = BOBGUI_WIDGET (bobgui_builder_get_object (builder, "fps"));
      box = BOBGUI_WIDGET (bobgui_builder_get_object (builder, "box"));

      bobgui_box_append (BOBGUI_BOX (box), color_widget_new ());

      id = g_timeout_add (500, update_fps_label, label);
      g_object_set_data_full (G_OBJECT (label), "tick_cb",
                              GUINT_TO_POINTER (id), remove_id);

      g_object_unref (builder);
    }

  if (!bobgui_widget_get_visible (window))
    bobgui_widget_set_visible (window, TRUE);
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));

  return window;
}
