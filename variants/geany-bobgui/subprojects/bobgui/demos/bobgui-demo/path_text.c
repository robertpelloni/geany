/* Path/Text
 *
 * This demo shows how to use GskPath to transform a path along another path.
 *
 * It also demonstrates that paths can be filled with more interesting
 * content than just plain colors.
 */

#include <glib/gi18n.h>
#include <bobgui/bobgui.h>

#define BOBGUI_TYPE_PATH_WIDGET (bobgui_path_widget_get_type ())
G_DECLARE_FINAL_TYPE (BobguiPathWidget, bobgui_path_widget, BOBGUI, PATH_WIDGET, BobguiWidget)

#define POINT_SIZE 8

enum {
  PROP_0,
  PROP_TEXT,
  PROP_EDITABLE,
  N_PROPS
};

struct _BobguiPathWidget
{
  BobguiWidget parent_instance;

  char *text;
  gboolean editable;

  graphene_point_t points[4];

  guint active_point;

  GskPath *line_path;
  GskPath *text_path;

  GdkPaintable *background;
};

struct _BobguiPathWidgetClass
{
  BobguiWidgetClass parent_class;
};

static GParamSpec *properties[N_PROPS] = { NULL, };

G_DEFINE_TYPE (BobguiPathWidget, bobgui_path_widget, BOBGUI_TYPE_WIDGET)

static GskPath *
create_path_from_text (BobguiWidget  *widget,
                       const char *text,
                       graphene_point_t *out_offset)
{
  PangoLayout *layout;
  PangoFontDescription *desc;
  GskPathBuilder *builder;
  GskPath *result;

  layout = bobgui_widget_create_pango_layout (widget, text);
  desc = pango_font_description_from_string ("sans bold 36");
  pango_layout_set_font_description (layout, desc);
  pango_font_description_free (desc);

  builder = gsk_path_builder_new ();
  gsk_path_builder_add_layout (builder, layout);
  result = gsk_path_builder_free_to_path (builder);

  if (out_offset)
    graphene_point_init (out_offset, 0, - pango_layout_get_baseline (layout) / (double) PANGO_SCALE);
  g_object_unref (layout);

  return result;
}

typedef struct
{
  GskPathMeasure *measure;
  GskPathBuilder *builder;
  graphene_point_t offset;
  double scale;
} BobguiPathTransform;

static void
bobgui_path_transform_point (GskPathMeasure         *measure,
                          const graphene_point_t *pt,
                          const graphene_point_t *offset,
                          float                   scale,
                          graphene_point_t       *res)
{
  graphene_vec2_t tangent;
  GskPathPoint point;

  if (gsk_path_measure_get_point (measure, (pt->x + offset->x) * scale, &point))
    {
      GskPath *path = gsk_path_measure_get_path (measure);

      gsk_path_point_get_position (&point, path, res);
      gsk_path_point_get_tangent (&point, path, GSK_PATH_TO_END, &tangent);

      res->x -= (pt->y + offset->y) * scale * graphene_vec2_get_y (&tangent);
      res->y += (pt->y + offset->y) * scale * graphene_vec2_get_x (&tangent);
    }
}

static gboolean
bobgui_path_transform_op (GskPathOperation        op,
                       const graphene_point_t *pts,
                       gsize                   n_pts,
                       float                   weight,
                       gpointer                data)
{
  BobguiPathTransform *transform = data;

  switch (op)
  {
    case GSK_PATH_MOVE:
      {
        graphene_point_t res;
        bobgui_path_transform_point (transform->measure, &pts[0], &transform->offset, transform->scale, &res);
        gsk_path_builder_move_to (transform->builder, res.x, res.y);
      }
      break;

    case GSK_PATH_LINE:
      {
        graphene_point_t res;
        bobgui_path_transform_point (transform->measure, &pts[1], &transform->offset, transform->scale, &res);
        gsk_path_builder_line_to (transform->builder, res.x, res.y);
      }
      break;

    case GSK_PATH_QUAD:
      {
        graphene_point_t res[2];
        bobgui_path_transform_point (transform->measure, &pts[1], &transform->offset, transform->scale, &res[0]);
        bobgui_path_transform_point (transform->measure, &pts[2], &transform->offset, transform->scale, &res[1]);
        gsk_path_builder_quad_to (transform->builder, res[0].x, res[0].y, res[1].x, res[1].y);
      }
      break;

    case GSK_PATH_CUBIC:
      {
        graphene_point_t res[3];
        bobgui_path_transform_point (transform->measure, &pts[1], &transform->offset, transform->scale, &res[0]);
        bobgui_path_transform_point (transform->measure, &pts[2], &transform->offset, transform->scale, &res[1]);
        bobgui_path_transform_point (transform->measure, &pts[3], &transform->offset, transform->scale, &res[2]);
        gsk_path_builder_cubic_to (transform->builder, res[0].x, res[0].y, res[1].x, res[1].y, res[2].x, res[2].y);
      }
      break;

    case GSK_PATH_CONIC:
      {
        graphene_point_t res[2];
        bobgui_path_transform_point (transform->measure, &pts[1], &transform->offset, transform->scale, &res[0]);
        bobgui_path_transform_point (transform->measure, &pts[3], &transform->offset, transform->scale, &res[1]);
        gsk_path_builder_conic_to (transform->builder, res[0].x, res[0].y, res[1].x, res[1].y, weight);
      }
      break;

    case GSK_PATH_CLOSE:
      gsk_path_builder_close (transform->builder);
      break;

    default:
      g_assert_not_reached();
      return FALSE;
  }

  return TRUE;
}

static GskPath *
bobgui_path_transform (GskPath                *line_path,
                    GskPath                *path,
                    const graphene_point_t *offset)
{
  BobguiPathTransform transform;
  graphene_rect_t bounds;

  if (!gsk_path_get_bounds (path, &bounds))
    return NULL;

  transform.measure = gsk_path_measure_new (line_path);
  transform.builder = gsk_path_builder_new ();
  transform.offset = *offset;

  if (bounds.origin.x + bounds.size.width > 0)
    transform.scale = gsk_path_measure_get_length (transform.measure) / (bounds.origin.x + bounds.size.width);
  else
    transform.scale = 1.0f;

  gsk_path_foreach (path, -1, bobgui_path_transform_op, &transform);

  gsk_path_measure_unref (transform.measure);

  return gsk_path_builder_free_to_path (transform.builder);
}

static void
bobgui_path_widget_clear_text_path (BobguiPathWidget *self)
{
  g_clear_pointer (&self->text_path, gsk_path_unref);
}

static void
bobgui_path_widget_clear_paths (BobguiPathWidget *self)
{
  bobgui_path_widget_clear_text_path (self);

  g_clear_pointer (&self->line_path, gsk_path_unref);
}

static void
bobgui_path_widget_create_text_path (BobguiPathWidget *self)
{
  GskPath *path;
  graphene_point_t offset;

  bobgui_path_widget_clear_text_path (self);

  path = create_path_from_text (BOBGUI_WIDGET (self), self->text, &offset);
  self->text_path = bobgui_path_transform (self->line_path, path, &offset);

  gsk_path_unref (path);
}

static void
bobgui_path_widget_create_paths (BobguiPathWidget *self)
{
  double width = bobgui_widget_get_width (BOBGUI_WIDGET (self));
  double height = bobgui_widget_get_height (BOBGUI_WIDGET (self));
  GskPathBuilder *builder;

  bobgui_path_widget_clear_paths (self);

  if (width <= 0 || height <= 0)
    return;

  builder = gsk_path_builder_new ();
  gsk_path_builder_move_to (builder,
                            self->points[0].x * width, self->points[0].y * height);
  gsk_path_builder_cubic_to (builder,
                             self->points[1].x * width, self->points[1].y * height,
                             self->points[2].x * width, self->points[2].y * height,
                             self->points[3].x * width, self->points[3].y * height);
  self->line_path = gsk_path_builder_free_to_path (builder);

  bobgui_path_widget_create_text_path (self);
}

static void
bobgui_path_widget_allocate (BobguiWidget *widget,
                          int        width,
                          int        height,
                          int        baseline)
{
  BobguiPathWidget *self = BOBGUI_PATH_WIDGET (widget);

  BOBGUI_WIDGET_CLASS (bobgui_path_widget_parent_class)->size_allocate (widget, width, height, baseline);

  bobgui_path_widget_create_paths (self);
}

static void
bobgui_path_widget_snapshot (BobguiWidget   *widget,
                          BobguiSnapshot *snapshot)
{
  BobguiPathWidget *self = BOBGUI_PATH_WIDGET (widget);
  double width = bobgui_widget_get_width (widget);
  double height = bobgui_widget_get_height (widget);
  GskPath *path;
  GskStroke *stroke;
  gsize i;

  /* frosted glass the background */
  bobgui_snapshot_push_blur (snapshot, 100);
  gdk_paintable_snapshot (self->background, snapshot, width, height);
  bobgui_snapshot_append_color (snapshot, &(GdkRGBA) { 1, 1, 1, 0.6 }, &GRAPHENE_RECT_INIT (0, 0, width, height));
  bobgui_snapshot_pop (snapshot);

  /* draw the text */
  if (self->text_path)
    {
      bobgui_snapshot_push_fill (snapshot, self->text_path, GSK_FILL_RULE_WINDING);
      gdk_paintable_snapshot (self->background, snapshot, width, height);

      /* ... with an emboss effect */
      stroke = gsk_stroke_new (2.0);
      bobgui_snapshot_translate (snapshot, &GRAPHENE_POINT_INIT(1, 1));
      bobgui_snapshot_push_stroke (snapshot, self->text_path, stroke);
      bobgui_snapshot_append_color (snapshot, &(GdkRGBA) { 0, 0, 0, 0.2 }, &GRAPHENE_RECT_INIT (0, 0, width, height));
      gsk_stroke_free (stroke);
      bobgui_snapshot_pop (snapshot);

      bobgui_snapshot_pop (snapshot);
    }

  if (self->editable && self->line_path)
    {
      GskPathBuilder *builder;

      /* draw the control line */
      stroke = gsk_stroke_new (1.0);
      bobgui_snapshot_push_stroke (snapshot, self->line_path, stroke);
      gsk_stroke_free (stroke);
      bobgui_snapshot_append_color (snapshot, &(GdkRGBA) { 0, 0, 0, 1 }, &GRAPHENE_RECT_INIT (0, 0, width, height));
      bobgui_snapshot_pop (snapshot);

      /* draw the points */
      builder = gsk_path_builder_new ();
      for (i = 0; i < 4; i++)
        {
          gsk_path_builder_add_circle (builder, &GRAPHENE_POINT_INIT (self->points[i].x * width, self->points[i].y * height), POINT_SIZE);
        }
      path = gsk_path_builder_free_to_path (builder);

      bobgui_snapshot_push_fill (snapshot, path, GSK_FILL_RULE_WINDING);
      bobgui_snapshot_append_color (snapshot, &(GdkRGBA) { 1, 1, 1, 1 }, &GRAPHENE_RECT_INIT (0, 0, width, height));
      bobgui_snapshot_pop (snapshot);

      stroke = gsk_stroke_new (1.0);
      bobgui_snapshot_push_stroke (snapshot, path, stroke);
      gsk_stroke_free (stroke);
      bobgui_snapshot_append_color (snapshot, &(GdkRGBA) { 0, 0, 0, 1 }, &GRAPHENE_RECT_INIT (0, 0, width, height));
      bobgui_snapshot_pop (snapshot);

      gsk_path_unref (path);
    }
}

static void
bobgui_path_widget_set_text (BobguiPathWidget *self,
                          const char    *text)
{
  if (g_strcmp0 (self->text, text) == 0)
    return;

  g_free (self->text);
  self->text = g_strdup (text);

  bobgui_path_widget_create_paths (self);

  bobgui_widget_queue_draw (BOBGUI_WIDGET (self));

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_TEXT]);
}

static void
bobgui_path_widget_set_editable (BobguiPathWidget *self,
                              gboolean       editable)
{
  if (self->editable == editable)
    return;

  self->editable = editable;

  bobgui_widget_queue_draw (BOBGUI_WIDGET (self));

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_EDITABLE]);
}

static void
bobgui_path_widget_set_property (GObject      *object,
                              guint         prop_id,
                              const GValue *value,
                              GParamSpec   *pspec)

{
  BobguiPathWidget *self = BOBGUI_PATH_WIDGET (object);

  switch (prop_id)
    {
    case PROP_TEXT:
      bobgui_path_widget_set_text (self, g_value_get_string (value));
      break;

    case PROP_EDITABLE:
      bobgui_path_widget_set_editable (self, g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_path_widget_get_property (GObject    *object,
                              guint       prop_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  BobguiPathWidget *self = BOBGUI_PATH_WIDGET (object);

  switch (prop_id)
    {
    case PROP_TEXT:
      g_value_set_string (value, self->text);
      break;

    case PROP_EDITABLE:
      g_value_set_boolean (value, self->editable);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_path_widget_dispose (GObject *object)
{
  BobguiPathWidget *self = BOBGUI_PATH_WIDGET (object);

  bobgui_path_widget_clear_paths (self);

  G_OBJECT_CLASS (bobgui_path_widget_parent_class)->dispose (object);
}

static void
bobgui_path_widget_class_init (BobguiPathWidgetClass *klass)
{
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = bobgui_path_widget_dispose;
  object_class->set_property = bobgui_path_widget_set_property;
  object_class->get_property = bobgui_path_widget_get_property;

  widget_class->size_allocate = bobgui_path_widget_allocate;
  widget_class->snapshot = bobgui_path_widget_snapshot;

  properties[PROP_TEXT] =
    g_param_spec_string ("text",
                         "text",
                         "Text transformed along a path",
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  properties[PROP_EDITABLE] =
    g_param_spec_boolean ("editable",
                          "editable",
                          "If the path can be edited by the user",
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
drag_begin (BobguiGestureDrag *gesture,
            double          x,
            double          y,
            BobguiPathWidget  *self)
{
  graphene_point_t mouse = GRAPHENE_POINT_INIT (x, y);
  double width = bobgui_widget_get_width (BOBGUI_WIDGET (self));
  double height = bobgui_widget_get_height (BOBGUI_WIDGET (self));
  gsize i;

  for (i = 0; i < 4; i++)
    {
      if (graphene_point_distance (&GRAPHENE_POINT_INIT (self->points[i].x * width, self->points[i].y * height), &mouse, NULL, NULL) <= POINT_SIZE)
        {
          self->active_point = i;
          break;
        }
    }
  if (i == 4)
    {
      bobgui_gesture_set_state (BOBGUI_GESTURE (gesture), BOBGUI_EVENT_SEQUENCE_DENIED);
      return;
    }

  bobgui_widget_queue_draw (BOBGUI_WIDGET (self));
}

static void
drag_update (BobguiGestureDrag *drag,
             double          offset_x,
             double          offset_y,
             BobguiPathWidget  *self)
{
  double width = bobgui_widget_get_width (BOBGUI_WIDGET (self));
  double height = bobgui_widget_get_height (BOBGUI_WIDGET (self));
  double start_x, start_y;

  bobgui_gesture_drag_get_start_point (drag, &start_x, &start_y);

  self->points[self->active_point] = GRAPHENE_POINT_INIT ((start_x + offset_x) / width,
                                                          (start_y + offset_y) / height);
  self->points[self->active_point].x = CLAMP (self->points[self->active_point].x, 0, 1);
  self->points[self->active_point].y = CLAMP (self->points[self->active_point].y, 0, 1);

  bobgui_path_widget_create_paths (self);

  bobgui_widget_queue_draw (BOBGUI_WIDGET (self));
}

static void
pointer_motion (BobguiEventControllerMotion *controller,
                double                    x,
                double                    y,
                BobguiPathWidget            *self)
{
  GskPathPoint point;

  if (gsk_path_get_closest_point (self->line_path,
                                  &GRAPHENE_POINT_INIT (x, y),
                                  INFINITY,
                                  &point,
                                  NULL))
    {
      bobgui_widget_queue_draw (BOBGUI_WIDGET (self));
    }
}

static void
pointer_leave (BobguiEventControllerMotion *controller,
               BobguiPathWidget            *self)
{
  bobgui_widget_queue_draw (BOBGUI_WIDGET (self));
}

static void
bobgui_path_widget_init (BobguiPathWidget *self)
{
  BobguiEventController *controller;

  controller = BOBGUI_EVENT_CONTROLLER (bobgui_gesture_drag_new ());
  g_signal_connect (controller, "drag-begin", G_CALLBACK (drag_begin), self);
  g_signal_connect (controller, "drag-update", G_CALLBACK (drag_update), self);
  g_signal_connect (controller, "drag-end", G_CALLBACK (drag_update), self);
  bobgui_widget_add_controller (BOBGUI_WIDGET (self), controller);

  controller = BOBGUI_EVENT_CONTROLLER (bobgui_event_controller_motion_new ());
  g_signal_connect (controller, "enter", G_CALLBACK (pointer_motion), self);
  g_signal_connect (controller, "motion", G_CALLBACK (pointer_motion), self);
  g_signal_connect (controller, "leave", G_CALLBACK (pointer_leave), self);
  bobgui_widget_add_controller (BOBGUI_WIDGET (self), controller);

  self->points[0] = GRAPHENE_POINT_INIT (0.1, 0.9);
  self->points[1] = GRAPHENE_POINT_INIT (0.3, 0.1);
  self->points[2] = GRAPHENE_POINT_INIT (0.7, 0.1);
  self->points[3] = GRAPHENE_POINT_INIT (0.9, 0.9);

  self->background = GDK_PAINTABLE (gdk_texture_new_from_resource ("/sliding_puzzle/portland-rose.jpg"));

  bobgui_path_widget_set_text (self, "It's almost working");
}

BobguiWidget *
bobgui_path_widget_new (void)
{
  BobguiPathWidget *self;

  self = g_object_new (BOBGUI_TYPE_PATH_WIDGET, NULL);

  return BOBGUI_WIDGET (self);
}

BobguiWidget *
do_path_text (BobguiWidget *do_widget)
{
  static BobguiWidget *window = NULL;

  if (!window)
    {
      BobguiBuilder *builder;

      g_type_ensure (BOBGUI_TYPE_PATH_WIDGET);

      builder = bobgui_builder_new_from_resource ("/path_text/path_text.ui");
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
