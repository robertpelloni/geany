#include <bobgui/bobgui.h>

#define DEMO_TYPE_WIDGET (demo_widget_get_type ())
G_DECLARE_FINAL_TYPE (DemoWidget, demo_widget, DEMO, WIDGET, BobguiWidget)

struct _DemoWidget
{
  BobguiWidget parent_instance;

  guint tick_cb;
  guint64 start_time;
  guint64 stop_time;

  float angle;
  float scale;

  PangoLayout *layout;
};

struct _DemoWidgetClass
{
  BobguiWidgetClass parent_class;
};

G_DEFINE_TYPE (DemoWidget, demo_widget, BOBGUI_TYPE_WIDGET)

static gboolean
tick_cb (BobguiWidget     *widget,
         GdkFrameClock *frame_clock,
         gpointer       user_data)
{
  DemoWidget *self = DEMO_WIDGET (widget);
  guint64 now;

  now = gdk_frame_clock_get_frame_time (frame_clock);

  if (self->start_time == 0)
    self->start_time = now;

  self->angle = 360 * (now - self->start_time) / (double)(G_TIME_SPAN_SECOND * 10);
  self->scale = 0.5 + 20 * (1 + sin (2 * G_PI * (now - self->start_time) / (double)(G_TIME_SPAN_SECOND * 5)));

  bobgui_widget_queue_draw (widget);

  return G_SOURCE_CONTINUE;
}

static gboolean
pressed_cb (BobguiEventController *controller,
            guint               keyval,
            guint               keycode,
            GdkModifierType     state,
            gpointer            data)
{
  DemoWidget *self = (DemoWidget *)bobgui_event_controller_get_widget (controller);

  if (keyval == GDK_KEY_space)
    {
      GdkFrameClock *frame_clock;
      guint64 now;

      frame_clock = bobgui_widget_get_frame_clock (BOBGUI_WIDGET (self));
      now = gdk_frame_clock_get_frame_time (frame_clock);

      if (self->tick_cb)
        {
          bobgui_widget_remove_tick_callback (BOBGUI_WIDGET (self), self->tick_cb);
          self->tick_cb = 0;
          self->stop_time = now;
        }
      else
        {
          self->start_time += now - self->stop_time;
          self->tick_cb = bobgui_widget_add_tick_callback (BOBGUI_WIDGET (self), tick_cb, NULL, NULL);
        }
      return TRUE;
    }

  return FALSE;
}

static void
demo_widget_set_text (DemoWidget *self,
                      const char *text,
                      size_t      length)
{
  pango_layout_set_text (self->layout, text, length);
}

static void
demo_widget_init (DemoWidget *self)
{
  BobguiEventController *controller;
  PangoFontDescription *desc;

  self->start_time = 0;
  self->tick_cb = bobgui_widget_add_tick_callback (BOBGUI_WIDGET (self), tick_cb, NULL, NULL);

  controller = bobgui_event_controller_key_new ();
  g_signal_connect (controller, "key-pressed", G_CALLBACK (pressed_cb), NULL);
  bobgui_widget_add_controller (BOBGUI_WIDGET (self), controller);
  bobgui_widget_set_focusable (BOBGUI_WIDGET (self), TRUE);

  desc = pango_font_description_new ();
  pango_font_description_set_family (desc, "Cantarell");
  pango_font_description_set_weight (desc, 520);
  pango_font_description_set_size (desc, 11 * PANGO_SCALE);

  self->layout = bobgui_widget_create_pango_layout (BOBGUI_WIDGET (self), "");
  pango_layout_set_font_description (self->layout, desc);

  pango_font_description_free (desc);
}

static void
demo_widget_snapshot (BobguiWidget   *widget,
                      BobguiSnapshot *snapshot)
{
  DemoWidget *self = DEMO_WIDGET (widget);
  int width, height;
  int pwidth, pheight;
  GdkRGBA color;

  width = bobgui_widget_get_width (widget);
  height = bobgui_widget_get_height (widget);

  bobgui_widget_get_color (widget, &color);

  pango_layout_get_pixel_size (self->layout, &pwidth, &pheight);

  bobgui_snapshot_save (snapshot);

  bobgui_snapshot_translate (snapshot, &GRAPHENE_POINT_INIT (0.5 * width, 0.5 * height));
  bobgui_snapshot_rotate (snapshot, self->angle);
  bobgui_snapshot_scale (snapshot, self->scale, self->scale);
  bobgui_snapshot_translate (snapshot, &GRAPHENE_POINT_INIT (- 0.5 * pwidth, - 0.5 * pheight));

  bobgui_snapshot_append_layout (snapshot, self->layout, &color);

  bobgui_snapshot_restore (snapshot);
}

static void
demo_widget_dispose (GObject *object)
{
  DemoWidget *self = DEMO_WIDGET (object);

  g_clear_object (&self->layout);

  G_OBJECT_CLASS (demo_widget_parent_class)->dispose (object);
}

static void
demo_widget_class_init (DemoWidgetClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (class);

  object_class->dispose = demo_widget_dispose;

  widget_class->snapshot = demo_widget_snapshot;
}

static BobguiWidget *
demo_widget_new (void)
{
  DemoWidget *demo;

  demo = g_object_new (DEMO_TYPE_WIDGET, NULL);

  return BOBGUI_WIDGET (demo);
}

int
main (int argc, char *argv[])
{
  BobguiWidget *window;
  BobguiWidget *demo;
  char *text = NULL;
  size_t length;

  bobgui_init ();

  window = bobgui_window_new ();
  bobgui_window_set_default_size (BOBGUI_WINDOW (window), 1024, 768);

  if (argc > 1)
    {
      GError *error = NULL;

      if (!g_file_get_contents (argv[1], &text, &length, &error))
        {
          g_warning ("%s", error->message);
          g_error_free (error);
          text = NULL;
        }
    }

  if (!text)
    {
      text = g_strdup ("Best Aa");
      length = strlen (text);
    }

  demo = demo_widget_new ();
  demo_widget_set_text (DEMO_WIDGET (demo), text, length);
  bobgui_window_set_child (BOBGUI_WINDOW (window), demo);

  g_free (text);

  bobgui_window_present (BOBGUI_WINDOW (window));

  bobgui_widget_grab_focus (demo);

  while (g_list_model_get_n_items (bobgui_window_get_toplevels ()) > 0)
    g_main_context_iteration (NULL, TRUE);

  return 0;
}
