#include <bobgui/bobgui.h>

typedef struct
{
  BobguiWidget parent_instance;
  BobguiWidget *child;
  float scale;
  float angle;
} BobguiZoom;

typedef struct
{
  BobguiWidgetClass parent_class;
} BobguiZoomClass;

enum {
  PROP_0,
  PROP_CHILD,
  PROP_SCALE,
  PROP_ANGLE,
  NUM_PROPERTIES
};

static GParamSpec *props[NUM_PROPERTIES] = { NULL, };

GType bobgui_zoom_get_type (void);

G_DEFINE_TYPE (BobguiZoom, bobgui_zoom, BOBGUI_TYPE_WIDGET)

static void
bobgui_zoom_init (BobguiZoom *zoom)
{
  zoom->child = NULL;
  zoom->scale = 1.0;
  zoom->angle = 0.0;
}

static void
bobgui_zoom_dispose (GObject *object)
{
  BobguiZoom *zoom = (BobguiZoom *)object;

  g_clear_pointer (&zoom->child, bobgui_widget_unparent);

  G_OBJECT_CLASS (bobgui_zoom_parent_class)->dispose (object);
}

static void
update_transform (BobguiZoom *zoom)
{
  BobguiLayoutManager *manager;
  BobguiLayoutChild *child;
  GskTransform *transform;
  graphene_rect_t bounds;
  int w, h;
  int x, y;

  manager = bobgui_widget_get_layout_manager (BOBGUI_WIDGET (zoom));
  child = bobgui_layout_manager_get_layout_child (manager, zoom->child);

  w = bobgui_widget_get_width (BOBGUI_WIDGET (zoom));
  h = bobgui_widget_get_height (BOBGUI_WIDGET (zoom));

  if (!bobgui_widget_compute_bounds (BOBGUI_WIDGET (zoom->child), BOBGUI_WIDGET (zoom->child), &bounds))
    return;

  x = bounds.size.width;
  y = bounds.size.height;

  transform = NULL;
  transform = gsk_transform_translate (transform, &GRAPHENE_POINT_INIT (w/2, h/2));
  transform = gsk_transform_scale (transform, zoom->scale, zoom->scale);
  transform = gsk_transform_rotate (transform, zoom->angle);
  transform = gsk_transform_translate (transform, &GRAPHENE_POINT_INIT (-x/2, -y/2));
  bobgui_fixed_layout_child_set_transform (BOBGUI_FIXED_LAYOUT_CHILD (child), transform);
  gsk_transform_unref (transform);
}

static void
bobgui_zoom_set_scale (BobguiZoom *zoom,
                    float    scale)
{

  if (zoom->scale == scale)
    return;

  zoom->scale = scale;

  update_transform (zoom);

  g_object_notify_by_pspec (G_OBJECT (zoom), props[PROP_SCALE]);

  bobgui_widget_queue_resize (BOBGUI_WIDGET (zoom));
}

static void
bobgui_zoom_set_angle (BobguiZoom *zoom,
                    float    angle)
{

  if (zoom->angle == angle)
    return;

  zoom->angle = angle;

  update_transform (zoom);

  g_object_notify_by_pspec (G_OBJECT (zoom), props[PROP_ANGLE]);

  bobgui_widget_queue_resize (BOBGUI_WIDGET (zoom));
}

static void
bobgui_zoom_set_child (BobguiZoom   *zoom,
                    BobguiWidget *child)
{
  g_clear_pointer (&zoom->child, bobgui_widget_unparent);

  zoom->child = child;

  if (zoom->child)
    bobgui_widget_set_parent (zoom->child, BOBGUI_WIDGET (zoom));

  update_transform (zoom);

  g_object_notify_by_pspec (G_OBJECT (zoom), props[PROP_CHILD]);
}

static void
bobgui_zoom_set_property (GObject      *object,
                       guint         prop_id,
                       const GValue *value,
                       GParamSpec   *pspec)
{
  BobguiZoom *zoom = (BobguiZoom *)object;

  switch (prop_id)
    {
    case PROP_SCALE:
      bobgui_zoom_set_scale (zoom, g_value_get_float (value));
      break;

    case PROP_ANGLE:
      bobgui_zoom_set_angle (zoom, g_value_get_float (value));
      break;

    case PROP_CHILD:
      bobgui_zoom_set_child (zoom, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_zoom_get_property (GObject    *object,
                       guint       prop_id,
                       GValue     *value,
                       GParamSpec *pspec)
{
  BobguiZoom *zoom = (BobguiZoom *)object;

  switch (prop_id)
    {
    case PROP_SCALE:
      g_value_set_float (value, zoom->scale);
      break;

    case PROP_ANGLE:
      g_value_set_float (value, zoom->angle);
      break;

    case PROP_CHILD:
      g_value_set_object (value, zoom->child);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_zoom_class_init (BobguiZoomClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (class);

  object_class->dispose = bobgui_zoom_dispose;
  object_class->set_property = bobgui_zoom_set_property;
  object_class->get_property = bobgui_zoom_get_property;

  props[PROP_SCALE] = g_param_spec_float ("scale", "", "",
                                          0.0, 100.0, 1.0,
                                          G_PARAM_READWRITE);

  props[PROP_ANGLE] = g_param_spec_float ("angle", "", "",
                                          0.0, 360.0, 1.0,
                                          G_PARAM_READWRITE);

  props[PROP_CHILD] = g_param_spec_object ("child", "", "",
                                           BOBGUI_TYPE_WIDGET,
                                           G_PARAM_READWRITE);

  g_object_class_install_properties (object_class, NUM_PROPERTIES, props);

  bobgui_widget_class_set_layout_manager_type (widget_class, BOBGUI_TYPE_FIXED_LAYOUT);
}

static BobguiWidget *
bobgui_zoom_new (void)
{
  return g_object_new (bobgui_zoom_get_type (), NULL);
}

static gboolean
update_transform_once (BobguiWidget     *widget,
                       GdkFrameClock *frame_clock,
                       gpointer       data)
{
  static int count = 0;

  update_transform ((BobguiZoom *)widget);
  count++;

  if (count == 2)
    return G_SOURCE_REMOVE;

  return G_SOURCE_CONTINUE;
}

int
main (int argc, char *argv[])
{
  BobguiWindow *window;
  BobguiWidget *zoom;
  BobguiWidget *box;
  BobguiWidget *grid;
  BobguiWidget *scale;
  BobguiWidget *angle;
  BobguiWidget *child;
  BobguiAdjustment *adjustment;

  bobgui_init ();

  window = BOBGUI_WINDOW (bobgui_window_new ());
  bobgui_window_set_default_size (BOBGUI_WINDOW (window), 600, 400);
  box = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
  bobgui_window_set_child (window, box);

  grid = bobgui_grid_new ();
  bobgui_grid_set_column_spacing (BOBGUI_GRID (grid), 10);

  scale = bobgui_scale_new_with_range (BOBGUI_ORIENTATION_HORIZONTAL, 1.0, 10.0, 1.0);
  bobgui_widget_set_hexpand (scale, TRUE);
  bobgui_grid_attach (BOBGUI_GRID (grid), bobgui_label_new ("Scale"), 0, 0, 1, 1);
  bobgui_grid_attach (BOBGUI_GRID (grid), scale, 1, 0, 1, 1);

  angle = bobgui_scale_new_with_range (BOBGUI_ORIENTATION_HORIZONTAL, 0.0, 360.0, 1.0);
  bobgui_scale_add_mark (BOBGUI_SCALE (angle),  90.0, BOBGUI_POS_BOTTOM, NULL);
  bobgui_scale_add_mark (BOBGUI_SCALE (angle), 180.0, BOBGUI_POS_BOTTOM, NULL);
  bobgui_scale_add_mark (BOBGUI_SCALE (angle), 270.0, BOBGUI_POS_BOTTOM, NULL);
  bobgui_widget_set_hexpand (angle, TRUE);
  bobgui_grid_attach (BOBGUI_GRID (grid), bobgui_label_new ("Angle"), 0, 1, 1, 1);
  bobgui_grid_attach (BOBGUI_GRID (grid), angle, 1, 1, 1, 1);
  bobgui_box_append (BOBGUI_BOX (box), grid);

  zoom = bobgui_zoom_new ();
  bobgui_widget_set_hexpand (zoom, TRUE);
  bobgui_widget_set_vexpand (zoom, TRUE);
  bobgui_box_append (BOBGUI_BOX (box), zoom);

  adjustment = bobgui_range_get_adjustment (BOBGUI_RANGE (scale));
  g_object_bind_property (adjustment, "value",
                          zoom, "scale",
                          G_BINDING_DEFAULT);

  adjustment = bobgui_range_get_adjustment (BOBGUI_RANGE (angle));
  g_object_bind_property (adjustment, "value",
                          zoom, "angle",
                          G_BINDING_DEFAULT);

  if (argc > 1)
    {
      BobguiBuilder *builder = bobgui_builder_new ();
      bobgui_builder_add_from_file (builder, argv[1], NULL);
      child = BOBGUI_WIDGET (bobgui_builder_get_object (builder, "child"));
      bobgui_zoom_set_child ((BobguiZoom *)zoom, child);
      g_object_unref (builder);
    }
  else
    bobgui_zoom_set_child ((BobguiZoom *)zoom, bobgui_button_new_with_label ("Click me!"));

  bobgui_window_present (window);

  /* HACK to get the transform initally updated */
  bobgui_widget_add_tick_callback (zoom, update_transform_once, NULL, NULL);

  while (g_list_model_get_n_items (bobgui_window_get_toplevels ()) > 0)
    g_main_context_iteration (NULL, TRUE);

  return 0;
}
