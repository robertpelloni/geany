/* Path/Walk
 *
 * This demo draws a world map and shows how to animate objects along a GskPath.
 *
 * The world map that is used here is a path with 211 lines and 1569 cubic
 * Bėzier segments in 121 contours.
 */

#include <glib/gi18n.h>
#include <bobgui/bobgui.h>

#define BOBGUI_TYPE_PATH_WALK (bobgui_path_walk_get_type ())
G_DECLARE_FINAL_TYPE (BobguiPathWalk, bobgui_path_walk, BOBGUI, PATH_WALK, BobguiWidget)

#define POINT_SIZE 8

enum {
  PROP_0,
  PROP_N_POINTS,
  PROP_PATH,
  N_PROPS
};

struct _BobguiPathWalk
{
  BobguiWidget parent_instance;

  GskPath *path;
  GskPathMeasure *measure;
  graphene_rect_t bounds;
  GskPath *arrow_path;
  guint n_points;
};

struct _BobguiPathWalkClass
{
  BobguiWidgetClass parent_class;
};

static GParamSpec *properties[N_PROPS] = { NULL, };

G_DEFINE_TYPE (BobguiPathWalk, bobgui_path_walk, BOBGUI_TYPE_WIDGET)

static void
rgba_init_from_hsla (GdkRGBA *rgba,
                     float    hue,
                     float    saturation,
                     float    lightness,
                     float    alpha)
{
  float m1, m2;

  if (lightness <= 0.5)
    m2 = lightness * (1 + saturation);
  else
    m2 = lightness + saturation - lightness * saturation;
  m1 = 2 * lightness - m2;

  rgba->alpha = alpha;

  if (saturation == 0)
    {
      rgba->red = lightness;
      rgba->green = lightness;
      rgba->blue = lightness;
    }
  else
    {
      hue = hue + 120;
      while (hue > 360)
        hue -= 360;
      while (hue < 0)
        hue += 360;

      if (hue < 60)
        rgba->red = m1 + (m2 - m1) * hue / 60;
      else if (hue < 180)
        rgba->red = m2;
      else if (hue < 240)
        rgba->red = m1 + (m2 - m1) * (240 - hue) / 60;
      else
        rgba->red = m1;

      hue -= 120;
      if (hue < 0)
        hue += 360;

      if (hue < 60)
        rgba->green = m1 + (m2 - m1) * hue / 60;
      else if (hue < 180)
        rgba->green = m2;
      else if (hue < 240)
        rgba->green = m1 + (m2 - m1) * (240 - hue) / 60;
      else
        rgba->green = m1;

      hue -= 120;
      if (hue < 0)
        hue += 360;

      if (hue < 60)
        rgba->blue = m1 + (m2 - m1) * hue / 60;
      else if (hue < 180)
        rgba->blue = m2;
      else if (hue < 240)
        rgba->blue = m1 + (m2 - m1) * (240 - hue) / 60;
      else
        rgba->blue = m1;
    }
}

static void
bobgui_path_walk_snapshot (BobguiWidget   *widget,
                        BobguiSnapshot *snapshot)
{
  BobguiPathWalk *self = BOBGUI_PATH_WALK (widget);
  double width = bobgui_widget_get_width (widget);
  double height = bobgui_widget_get_height (widget);
  float length, progress;
  GskStroke *stroke;
  guint i;

  if (self->path == NULL)
    return;

  bobgui_snapshot_save (snapshot);

  stroke = gsk_stroke_new (2.0);
  bobgui_snapshot_push_stroke (snapshot, self->path, stroke);
  bobgui_snapshot_append_color (snapshot, &(GdkRGBA) { 0, 0, 0, 1 }, &GRAPHENE_RECT_INIT (0, 0, width, height));
  bobgui_snapshot_pop (snapshot);
  gsk_stroke_free (stroke);

  length = gsk_path_measure_get_length (self->measure);
  progress = 25.f * gdk_frame_clock_get_frame_time (bobgui_widget_get_frame_clock (widget)) / G_USEC_PER_SEC;

  stroke = gsk_stroke_new (1.0);
  for (i = 0; i < self->n_points; i++)
    {
      GskPathPoint point;
      graphene_point_t position;
      float angle;
      GdkRGBA color;
      float distance;

      distance = i * length / self->n_points;
      distance = fmod (distance + progress, length);

      gsk_path_measure_get_point (self->measure, distance, &point);
      gsk_path_point_get_position (&point, self->path, &position);
      angle = gsk_path_point_get_rotation (&point, self->path, GSK_PATH_FROM_START);
      rgba_init_from_hsla (&color, 360.f * i / self->n_points, 1, 0.5, 1);

      bobgui_snapshot_save (snapshot);
      bobgui_snapshot_translate (snapshot, &position);
      bobgui_snapshot_rotate (snapshot, angle);
      bobgui_snapshot_append_fill (snapshot, self->arrow_path, GSK_FILL_RULE_EVEN_ODD, &color);
      bobgui_snapshot_append_stroke (snapshot, self->arrow_path, stroke, &(GdkRGBA) { 0, 0, 0, 1 });
      bobgui_snapshot_restore (snapshot);
    }

  gsk_stroke_free (stroke);
  bobgui_snapshot_restore (snapshot);
}

static void
bobgui_path_walk_measure (BobguiWidget      *widget,
                       BobguiOrientation  orientation,
                       int             for_size,
                       int            *minimum,
                       int            *natural,
                       int            *minimum_baseline,
                       int            *natural_baseline)
{
  BobguiPathWalk *self = BOBGUI_PATH_WALK (widget);

  if (orientation == BOBGUI_ORIENTATION_HORIZONTAL)
    *minimum = *natural = (int) ceilf (self->bounds.size.width);
  else
    *minimum = *natural = (int) ceilf (self->bounds.size.height);
}

static void
bobgui_path_walk_set_n_points (BobguiPathWalk *self,
                            gsize        n_points)
{
  if (self->n_points == n_points)
    return;

  self->n_points = n_points;

  bobgui_widget_queue_draw (BOBGUI_WIDGET (self));

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_N_POINTS]);
}

static void
bobgui_path_walk_set_path (BobguiPathWalk *self,
                        GskPath     *path)
{
  if (self->path == path)
    return;

  g_clear_pointer (&self->path, gsk_path_unref);
  graphene_rect_init (&self->bounds, 0, 0, 0, 0);
  if (path)
    {
      GskStroke *stroke;

      self->path = gsk_path_ref (path);
      stroke = gsk_stroke_new (2.0);
      gsk_path_get_stroke_bounds (path, stroke, &self->bounds);
      gsk_stroke_free (stroke);
      self->measure = gsk_path_measure_new (self->path);
    }

  bobgui_widget_queue_resize (BOBGUI_WIDGET (self));

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_PATH]);
}

static void
bobgui_path_walk_set_property (GObject      *object,
                            guint         prop_id,
                            const GValue *value,
                            GParamSpec   *pspec)

{
  BobguiPathWalk *self = BOBGUI_PATH_WALK (object);

  switch (prop_id)
    {
    case PROP_N_POINTS:
      bobgui_path_walk_set_n_points (self, g_value_get_uint (value));
      break;

    case PROP_PATH:
      bobgui_path_walk_set_path (self, g_value_get_boxed (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_path_walk_get_property (GObject    *object,
                            guint       prop_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  BobguiPathWalk *self = BOBGUI_PATH_WALK (object);

  switch (prop_id)
    {
    case PROP_N_POINTS:
      g_value_set_uint (value, self->n_points);
      break;

    case PROP_PATH:
      g_value_set_boxed (value, self->path);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_path_walk_dispose (GObject *object)
{
  BobguiPathWalk *self = BOBGUI_PATH_WALK (object);

  g_clear_pointer (&self->path, gsk_path_unref);
  g_clear_pointer (&self->measure, gsk_path_measure_unref);
  g_clear_pointer (&self->arrow_path, gsk_path_unref);

  G_OBJECT_CLASS (bobgui_path_walk_parent_class)->dispose (object);
}

static void
bobgui_path_walk_class_init (BobguiPathWalkClass *klass)
{
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = bobgui_path_walk_dispose;
  object_class->set_property = bobgui_path_walk_set_property;
  object_class->get_property = bobgui_path_walk_get_property;

  widget_class->snapshot = bobgui_path_walk_snapshot;
  widget_class->measure = bobgui_path_walk_measure;

  properties[PROP_N_POINTS] =
    g_param_spec_uint ("n-points",
                       NULL, NULL,
                       1, G_MAXUINT,
                       500,
                       G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  properties[PROP_PATH] =
    g_param_spec_boxed ("path",
                        NULL, NULL,
                        GSK_TYPE_PATH,
                        G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPS, properties);
}

static gboolean
tick_tick_tick (BobguiWidget     *self,
                GdkFrameClock *frame_clock,
                gpointer       unused)
{
  bobgui_widget_queue_draw (BOBGUI_WIDGET (self));

  return G_SOURCE_CONTINUE;
}

static void
bobgui_path_walk_init (BobguiPathWalk *self)
{
  /* Data taken from
   * https://commons.wikimedia.org/wiki/Maps_of_the_world#/media/File:Simplified_blank_world_map_without_Antartica_(no_borders).svg
   */
  GBytes *data = g_resources_lookup_data ("/path_walk/path_world.txt", 0, NULL);
  GskPath *path = gsk_path_parse (g_bytes_get_data (data, NULL));
  g_bytes_unref (data);
  bobgui_path_walk_set_path (self, path);
  gsk_path_unref (path);
  self->arrow_path = gsk_path_parse ("M 5 0 L 0 -5. 0 -2, -5 -2, -5 2, 0 2, 0 5 Z");
  self->n_points = 500;
  bobgui_widget_add_tick_callback (BOBGUI_WIDGET (self), tick_tick_tick, NULL, NULL);
}

BobguiWidget *
bobgui_path_walk_new (void)
{
  BobguiPathWalk *self;

  self = g_object_new (BOBGUI_TYPE_PATH_WALK, NULL);

  return BOBGUI_WIDGET (self);
}

BobguiWidget *
do_path_walk (BobguiWidget *do_widget)
{
  static BobguiWidget *window = NULL;

  if (!window)
    {
      BobguiBuilder *builder;

      g_type_ensure (BOBGUI_TYPE_PATH_WALK);

      builder = bobgui_builder_new_from_resource ("/path_walk/path_walk.ui");
      window = BOBGUI_WIDGET (bobgui_builder_get_object (builder, "window"));
      bobgui_window_set_display (BOBGUI_WINDOW (window),
                              bobgui_widget_get_display (do_widget));
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *) &window);
      g_object_unref (builder);
    }

  if (!bobgui_widget_get_visible (window))
    bobgui_window_present (BOBGUI_WINDOW (window));
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));

  return window;
}
