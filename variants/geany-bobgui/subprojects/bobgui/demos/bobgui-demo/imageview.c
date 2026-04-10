#include <math.h>
#include "imageview.h"


enum
{
  PROP_TEXTURE = 1,
  PROP_FILTER,
  PROP_SCALE,
  PROP_ANGLE,
};

struct _ImageView
{
  BobguiWidget parent_instance;

  GdkTexture *texture;
  float scale;
  float angle;
  GskScalingFilter filter;

  BobguiWidget *menu;
  float start_scale;
  float start_angle;
};

struct _ImageViewClass
{
  BobguiWidgetClass parent_class;
};

G_DEFINE_TYPE (ImageView, image_view, BOBGUI_TYPE_WIDGET)

static gboolean
query_tooltip (BobguiWidget  *widget,
               int         x,
               int         y,
               gboolean    keyboard_mode,
               BobguiTooltip *tooltip,
               gpointer    data)
{
  ImageView *self = IMAGE_VIEW (widget);
  BobguiWidget *grid;
  BobguiWidget *label;
  char *s, *s2;
  const char *filter[] = { "Linear", "Nearest", "Trilinear" };
  int precision, l;

  grid = bobgui_grid_new ();
  bobgui_grid_set_column_spacing (BOBGUI_GRID (grid), 12);
  bobgui_grid_set_row_spacing (BOBGUI_GRID (grid), 6);
  label = bobgui_label_new ("Texture");
  bobgui_label_set_xalign (BOBGUI_LABEL (label), 0);
  bobgui_grid_attach (BOBGUI_GRID (grid), label, 0, 0, 1, 1);
  s = g_strdup_printf ("%d\342\200\206\303\227\342\200\206%d",
                       gdk_texture_get_width (self->texture),
                       gdk_texture_get_height (self->texture));
  label = bobgui_label_new (s);
  g_free (s);
  bobgui_label_set_xalign (BOBGUI_LABEL (label), 1);
  bobgui_grid_attach (BOBGUI_GRID (grid), label, 1, 0, 1, 1);

  label = bobgui_label_new ("Rotation");
  bobgui_label_set_xalign (BOBGUI_LABEL (label), 0);
  bobgui_grid_attach (BOBGUI_GRID (grid), label, 0, 1, 1, 1);
  s = g_strdup_printf ("%.1f", self->angle);
  if (g_str_has_suffix (s, ".0"))
    s[strlen (s) - 2] = '\0';
  s2 = g_strconcat (s, "\302\260", NULL);
  label = bobgui_label_new (s2);
  g_free (s2);
  g_free (s);
  bobgui_label_set_xalign (BOBGUI_LABEL (label), 1);
  bobgui_grid_attach (BOBGUI_GRID (grid), label, 1, 1, 1, 1);

  label = bobgui_label_new ("Scale");
  bobgui_label_set_xalign (BOBGUI_LABEL (label), 0);
  bobgui_grid_attach (BOBGUI_GRID (grid), label, 0, 2, 1, 1);

  precision = 1;
  s = NULL;
  do {
    g_free (s);
    s = g_strdup_printf ("%.*f", precision, self->scale);
    l = strlen (s) - 1;
    while (s[l] == '0')
      l--;
    if (s[l] == '.')
      s[l] = '\0';
    precision++;
  } while (strcmp (s, "0") == 0);

  label = bobgui_label_new (s);
  g_free (s);
  bobgui_label_set_xalign (BOBGUI_LABEL (label), 1);
  bobgui_grid_attach (BOBGUI_GRID (grid), label, 1, 2, 1, 1);

  label = bobgui_label_new ("Filter");
  bobgui_label_set_xalign (BOBGUI_LABEL (label), 0);
  bobgui_grid_attach (BOBGUI_GRID (grid), label, 0, 3, 1, 1);
  label = bobgui_label_new (filter[self->filter]);
  bobgui_label_set_xalign (BOBGUI_LABEL (label), 1);
  bobgui_grid_attach (BOBGUI_GRID (grid), label, 1, 3, 1, 1);

  bobgui_tooltip_set_custom (tooltip, grid);

  return TRUE;
}

static void
image_view_init (ImageView *self)
{
  self->scale = 1.f;
  self->angle = 0.f;
  self->filter = GSK_SCALING_FILTER_LINEAR;
  bobgui_widget_init_template (BOBGUI_WIDGET (self));
}

static void
image_view_dispose (GObject *object)
{
  ImageView *self = IMAGE_VIEW (object);

  g_clear_object (&self->texture);

  bobgui_widget_dispose_template (BOBGUI_WIDGET (self), IMAGE_TYPE_VIEW);

  G_OBJECT_CLASS (image_view_parent_class)->dispose (object);
}

static void
image_view_snapshot (BobguiWidget   *widget,
                     BobguiSnapshot *snapshot)
{
  ImageView *self = IMAGE_VIEW (widget);
  int x, y, width, height;
  double w, h, w2, h2;

  width = bobgui_widget_get_width (widget);
  height = bobgui_widget_get_height (widget);

  w2 = w = self->scale * gdk_texture_get_width (self->texture);
  h2 = h = self->scale * gdk_texture_get_height (self->texture);

  if (G_APPROX_VALUE (self->angle, 90.f, FLT_EPSILON) ||
      G_APPROX_VALUE (self->angle, 270.f, FLT_EPSILON))
    {
      double s = w2;
      w2 = h2;
      h2 = s;
    }

  x = (width - ceil (w2)) / 2;
  y = (height - ceil (h2)) / 2;

  bobgui_snapshot_push_clip (snapshot, &GRAPHENE_RECT_INIT (0, 0, width, height));
  bobgui_snapshot_save (snapshot);
  bobgui_snapshot_translate (snapshot, &GRAPHENE_POINT_INIT (x, y));
  bobgui_snapshot_translate (snapshot, &GRAPHENE_POINT_INIT (w2 / 2, h2 / 2));
  bobgui_snapshot_rotate (snapshot, self->angle);
  bobgui_snapshot_translate (snapshot, &GRAPHENE_POINT_INIT (- w / 2, - h / 2));
  bobgui_snapshot_append_scaled_texture (snapshot,
                                      self->texture,
                                      self->filter,
                                      &GRAPHENE_RECT_INIT (0, 0, w, h));
  bobgui_snapshot_restore (snapshot);
  bobgui_snapshot_pop (snapshot);
}

static void
image_view_measure (BobguiWidget      *widget,
                    BobguiOrientation  orientation,
                    int             for_size,
                    int            *minimum,
                    int            *natural,
                    int            *minimum_baseline,
                    int            *natural_baseline)
{
  ImageView *self = IMAGE_VIEW (widget);
  int width, height;
  int size;

  width = gdk_texture_get_width (self->texture);
  height = gdk_texture_get_height (self->texture);

  if (G_APPROX_VALUE (self->angle, 90.f, FLT_EPSILON) ||
      G_APPROX_VALUE (self->angle, 270.f, FLT_EPSILON))
    {
      int s = width;
      width = height;
      height = s;
    }

  if (orientation == BOBGUI_ORIENTATION_HORIZONTAL)
    size = width;
  else
    size = height;

  *minimum = *natural = (int) ceil (self->scale * size);
}

static void
image_view_size_allocate (BobguiWidget *widget,
                          int        width,
                          int        height,
                          int        baseline)
{
  ImageView *self = IMAGE_VIEW (widget);

  /* Since we are not using a layout manager (who would do this
   * for us), we need to allocate a size for our menu by calling
   * bobgui_popover_present().
   */
  bobgui_popover_present (BOBGUI_POPOVER (self->menu));
}

static void update_actions (ImageView *self);

static void
image_view_set_property (GObject      *object,
                         guint         prop_id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
  ImageView *self = IMAGE_VIEW (object);

  switch (prop_id)
    {
    case PROP_TEXTURE:
      g_clear_object (&self->texture);
      self->texture = g_value_dup_object (value);
      self->scale = 1.f;
      self->angle = 0.f;
      self->filter = GSK_SCALING_FILTER_LINEAR;
      update_actions (self);
      bobgui_widget_queue_resize (BOBGUI_WIDGET (object));
      g_object_notify (object, "scale");
      g_object_notify (object, "angle");
      g_object_notify (object, "filter");
      break;

    case PROP_SCALE:
      self->scale = g_value_get_float (value);
      update_actions (self);
      bobgui_widget_queue_resize (BOBGUI_WIDGET (object));
      break;

    case PROP_ANGLE:
      self->angle = fmodf (g_value_get_float (value), 360.f);
      bobgui_widget_queue_resize (BOBGUI_WIDGET (object));
      break;

    case PROP_FILTER:
      self->filter = g_value_get_enum (value);
      bobgui_widget_queue_resize (BOBGUI_WIDGET (object));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
image_view_get_property (GObject     *object,
                         guint        prop_id,
                         GValue      *value,
                         GParamSpec  *pspec)
{
  ImageView *self = IMAGE_VIEW (object);

  switch (prop_id)
    {
    case PROP_TEXTURE:
      g_value_set_object (value, self->texture);
      break;

    case PROP_SCALE:
      g_value_set_float (value, self->scale);
      break;

    case PROP_ANGLE:
      g_value_set_float (value, self->angle);
      break;

    case PROP_FILTER:
      g_value_set_enum (value, self->filter);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
pressed_cb (BobguiGestureClick *gesture,
            guint            n_press,
            double           x,
            double           y,
            ImageView     *self)
{
  /* We are placing our menu at the point where
   * the click happened, before popping it up.
   */
  bobgui_popover_set_pointing_to (BOBGUI_POPOVER (self->menu),
                               &(const GdkRectangle){ x, y, 1, 1 });
  bobgui_popover_popup (BOBGUI_POPOVER (self->menu));
}

static void
update_actions (ImageView *self)
{
  bobgui_widget_action_set_enabled (BOBGUI_WIDGET (self), "zoom.in", self->scale < 1024.);
  bobgui_widget_action_set_enabled (BOBGUI_WIDGET (self), "zoom.out", self->scale > 1./1024.);
  bobgui_widget_action_set_enabled (BOBGUI_WIDGET (self), "zoom.reset", self->scale != 1.);
}

static void
zoom_cb (BobguiWidget  *widget,
         const char *action_name,
         GVariant   *parameter)
{
  ImageView *self = IMAGE_VIEW (widget);
  float scale;

  if (g_str_equal (action_name, "zoom.in"))
    scale = MIN (1024., self->scale * M_SQRT2);
  else if (g_str_equal (action_name, "zoom.out"))
    scale = MAX (1./1024., self->scale / M_SQRT2);
  else if (g_str_equal (action_name, "zoom.reset"))
    scale = 1.0;
  else
    g_assert_not_reached ();

  g_object_set (widget, "scale", scale, NULL);
}

static void
rotate_cb (BobguiWidget  *widget,
           const char *action_name,
           GVariant   *parameter)
{
  ImageView *self = IMAGE_VIEW (widget);
  int angle;

  g_variant_get (parameter, "i", &angle);

  g_object_set (widget, "angle", fmodf (self->angle + angle, 360.f), NULL);
}

static void
scale_begin_cb (BobguiGesture       *gesture,
                GdkEventSequence *sequence,
	        ImageView        *self)
{
  self->start_scale = self->scale;
}

static void
scale_changed_cb (BobguiGestureZoom *gesture,
		  double          scale,
		  ImageView      *self)
{
  g_object_set (self, "scale", self->start_scale*scale, NULL);
}

#define RAD_TO_DEG(a) (180.0 * (a) / M_PI)

static void
rotate_begin_cb (BobguiGesture       *gesture,
	         GdkEventSequence *sequence,
	         ImageView        *self)
{
  self->start_angle = self->angle;
}

static void
angle_changed_cb (BobguiGestureRotate *gesture,
		  double            angle,
		  double            delta,
		  ImageView        *self)
{
  double a;

  a = 90 * round (RAD_TO_DEG (delta) / 10);

  g_object_set (self, "angle", fmodf (self->start_angle + a, 360.f), NULL);
}

static void
image_view_class_init (ImageViewClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (class);

  object_class->dispose = image_view_dispose;
  object_class->set_property = image_view_set_property;
  object_class->get_property = image_view_get_property;

  widget_class->snapshot = image_view_snapshot;
  widget_class->measure = image_view_measure;
  widget_class->size_allocate = image_view_size_allocate;

  g_object_class_install_property (object_class, PROP_TEXTURE,
      g_param_spec_object ("texture", NULL, NULL,
                           GDK_TYPE_TEXTURE,
                           G_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_SCALE,
      g_param_spec_float ("scale", NULL, NULL,
                          1./1024., 1024., 1.0,
                          G_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_ANGLE,
      g_param_spec_float ("angle", NULL, NULL,
                          0.0, 360.0, 0.0,
                          G_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_FILTER,
      g_param_spec_enum ("filter", NULL, NULL,
                         GSK_TYPE_SCALING_FILTER, GSK_SCALING_FILTER_LINEAR,
                         G_PARAM_READWRITE));

  /* These are the actions that we are using in the menu */
  bobgui_widget_class_install_action (widget_class, "zoom.in", NULL, zoom_cb);
  bobgui_widget_class_install_action (widget_class, "zoom.out", NULL, zoom_cb);
  bobgui_widget_class_install_action (widget_class, "zoom.reset", NULL, zoom_cb);
  bobgui_widget_class_install_action (widget_class, "rotate", "i", rotate_cb);

  bobgui_widget_class_set_template_from_resource (widget_class, "/menu/imageview.ui");
  bobgui_widget_class_bind_template_child (widget_class, ImageView, menu);
  bobgui_widget_class_bind_template_callback (widget_class, pressed_cb);
  bobgui_widget_class_bind_template_callback (widget_class, scale_changed_cb);
  bobgui_widget_class_bind_template_callback (widget_class, scale_begin_cb);
  bobgui_widget_class_bind_template_callback (widget_class, angle_changed_cb);
  bobgui_widget_class_bind_template_callback (widget_class, rotate_begin_cb);
  bobgui_widget_class_bind_template_callback (widget_class, query_tooltip);

  bobgui_widget_class_set_accessible_role (widget_class, BOBGUI_ACCESSIBLE_ROLE_IMG);
}

BobguiWidget *
image_view_new (const char *resource)
{
  ImageView *self;
  GdkTexture *texture;

  texture = gdk_texture_new_from_resource (resource);

  self = g_object_new (IMAGE_TYPE_VIEW,
                       "texture", texture,
                       NULL);

  g_object_unref (texture);

  return BOBGUI_WIDGET (self);
}
