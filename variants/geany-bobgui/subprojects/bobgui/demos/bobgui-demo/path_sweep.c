/* Path/Sweep
 *
 * This demo shows how path intersections can be used.
 *
 * The world map that is used here is a path with 211 lines and 1569 cubic
 * Bėzier segments in 121 contours.
 */

#include <glib/gi18n.h>
#include <bobgui/bobgui.h>

#define BOBGUI_TYPE_PATH_SWEEP (bobgui_path_sweep_get_type ())
G_DECLARE_FINAL_TYPE (BobguiPathSweep, bobgui_path_sweep, BOBGUI, PATH_SWEEP, BobguiWidget)

#define POINT_SIZE 8

enum {
  PROP_0,
  PROP_PATH,
  N_PROPS
};

struct _BobguiPathSweep
{
  BobguiWidget parent_instance;

  GskPath *path;
  graphene_rect_t bounds;
  float y_pos;
  gboolean in;
};

struct _BobguiPathSweepClass
{
  BobguiWidgetClass parent_class;
};

static GParamSpec *properties[N_PROPS] = { NULL, };

G_DEFINE_TYPE (BobguiPathSweep, bobgui_path_sweep, BOBGUI_TYPE_WIDGET)

static gboolean
intersection_cb (GskPath             *path1,
                 const GskPathPoint  *point1,
                 GskPath             *path2,
                 const GskPathPoint  *point2,
                 GskPathIntersection  kind,
                 gpointer             data)
{
  GskPathBuilder *builder = data;
  graphene_point_t p;

  gsk_path_point_get_position (point1, path1, &p);
  gsk_path_builder_add_circle (builder, &p, 4);

  return TRUE;
}

static GskPath *
get_intersection_path (GskPath *path1,
                       GskPath *path2)
{
  GskPathBuilder *builder = gsk_path_builder_new ();

  gsk_path_foreach_intersection (path1, path2, intersection_cb, builder);

  return gsk_path_builder_free_to_path (builder);
}

static void
bobgui_path_sweep_snapshot (BobguiWidget   *widget,
                        BobguiSnapshot *snapshot)
{
  BobguiPathSweep *self = BOBGUI_PATH_SWEEP (widget);
  GskStroke *stroke;

  if (self->path == NULL)
    return;

  bobgui_snapshot_save (snapshot);

  stroke = gsk_stroke_new (2.0);

  bobgui_snapshot_append_stroke (snapshot, self->path, stroke, &(GdkRGBA) { 0, 0, 0, 1 });

  if (self->in)
    {
      graphene_rect_t bounds;
      GskPathBuilder *builder;
      GskPath *line, *isecs;

      gsk_path_get_stroke_bounds (self->path, stroke, &bounds);

      builder = gsk_path_builder_new ();
      gsk_path_builder_move_to (builder, bounds.origin.x, bounds.origin.y + self->y_pos);
      gsk_path_builder_line_to (builder, bounds.origin.x + bounds.size.width, bounds.origin.y + self->y_pos);
      line = gsk_path_builder_free_to_path (builder);

      bobgui_snapshot_append_stroke (snapshot, line, stroke, &(GdkRGBA) { 0, 0, 0, 1 });

      isecs = get_intersection_path (self->path, line);

      bobgui_snapshot_append_fill (snapshot, isecs, GSK_FILL_RULE_WINDING, &(GdkRGBA) { 1, 0, 0, 1 });
      bobgui_snapshot_append_stroke (snapshot, isecs, stroke, &(GdkRGBA) { 0, 0, 0, 1 });

      gsk_path_unref (isecs);
      gsk_path_unref (line);
    }

  gsk_stroke_free (stroke);

  bobgui_snapshot_restore (snapshot);
}

static void
bobgui_path_sweep_measure (BobguiWidget      *widget,
                        BobguiOrientation  orientation,
                        int             for_size,
                        int            *minimum,
                        int            *natural,
                        int            *minimum_baseline,
                        int            *natural_baseline)
{
  BobguiPathSweep *self = BOBGUI_PATH_SWEEP (widget);

  if (orientation == BOBGUI_ORIENTATION_HORIZONTAL)
    *minimum = *natural = (int) ceilf (self->bounds.size.width);
  else
    *minimum = *natural = (int) ceilf (self->bounds.size.height);
}

static void
bobgui_path_sweep_set_path (BobguiPathSweep *self,
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
    }

  bobgui_widget_queue_resize (BOBGUI_WIDGET (self));

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_PATH]);
}

static void
bobgui_path_sweep_set_property (GObject      *object,
                            guint         prop_id,
                            const GValue *value,
                            GParamSpec   *pspec)

{
  BobguiPathSweep *self = BOBGUI_PATH_SWEEP (object);

  switch (prop_id)
    {
    case PROP_PATH:
      bobgui_path_sweep_set_path (self, g_value_get_boxed (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_path_sweep_get_property (GObject    *object,
                            guint       prop_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  BobguiPathSweep *self = BOBGUI_PATH_SWEEP (object);

  switch (prop_id)
    {
    case PROP_PATH:
      g_value_set_boxed (value, self->path);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_path_sweep_dispose (GObject *object)
{
  BobguiPathSweep *self = BOBGUI_PATH_SWEEP (object);

  g_clear_pointer (&self->path, gsk_path_unref);

  G_OBJECT_CLASS (bobgui_path_sweep_parent_class)->dispose (object);
}

static void
bobgui_path_sweep_class_init (BobguiPathSweepClass *klass)
{
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = bobgui_path_sweep_dispose;
  object_class->set_property = bobgui_path_sweep_set_property;
  object_class->get_property = bobgui_path_sweep_get_property;

  widget_class->snapshot = bobgui_path_sweep_snapshot;
  widget_class->measure = bobgui_path_sweep_measure;

  properties[PROP_PATH] =
    g_param_spec_boxed ("path", NULL, NULL,
                        GSK_TYPE_PATH,
                        G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
motion_cb (BobguiEventControllerMotion *controller,
           double                    x,
           double                    y,
           gpointer                  data)
{
  BobguiPathSweep *self = data;

  self->y_pos = y;
  bobgui_widget_queue_draw (BOBGUI_WIDGET (self));
}

static void
enter_cb (BobguiEventControllerMotion *controller,
          double                    x,
          double                    y,
          gpointer                  data)
{
  BobguiPathSweep *self = data;

  self->in = TRUE;
  self->y_pos = y;
  bobgui_widget_queue_draw (BOBGUI_WIDGET (self));
}

static void
leave_cb (BobguiEventControllerMotion *controller,
          gpointer                  data)
{
  BobguiPathSweep *self = data;

  self->in = FALSE;
  bobgui_widget_queue_draw (BOBGUI_WIDGET (self));
}

static void
bobgui_path_sweep_init (BobguiPathSweep *self)
{
  BobguiEventController *controller;

  /* Data taken from
   * https://commons.wikimedia.org/wiki/Maps_of_the_world#/media/File:Simplified_blank_world_map_without_Antartica_(no_borders).svg
   */
  GBytes *data = g_resources_lookup_data ("/path_sweep/path_world.txt", 0, NULL);
  GskPath *path = gsk_path_parse (g_bytes_get_data (data, NULL));
  g_bytes_unref (data);
  bobgui_path_sweep_set_path (self, path);
  gsk_path_unref (path);

  controller = bobgui_event_controller_motion_new ();
  g_signal_connect (controller, "motion", G_CALLBACK (motion_cb), self);
  g_signal_connect (controller, "enter", G_CALLBACK (enter_cb), self);
  g_signal_connect (controller, "leave", G_CALLBACK (leave_cb), self);
  bobgui_widget_add_controller (BOBGUI_WIDGET (self), controller);
}

BobguiWidget *
bobgui_path_sweep_new (void)
{
  BobguiPathSweep *self;

  self = g_object_new (BOBGUI_TYPE_PATH_SWEEP, NULL);

  return BOBGUI_WIDGET (self);
}

BobguiWidget *
do_path_sweep (BobguiWidget *do_widget)
{
  static BobguiWidget *window = NULL;

  if (!window)
    {
      BobguiBuilder *builder;

      g_type_ensure (BOBGUI_TYPE_PATH_SWEEP);

      builder = bobgui_builder_new_from_resource ("/path_sweep/path_sweep.ui");
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
