#include <math.h>
#include "demo4widget.h"
#include "hsla.h"

enum
{
  PROP_0,
  PROP_PROGRESS,
};

struct _Demo4Widget
{
  BobguiWidget parent_instance;
  PangoLayout *layout;
  GskColorStop stops[8];
  gsize n_stops;
  double progress;

  guint tick;
};

struct _Demo4WidgetClass
{
  BobguiWidgetClass parent_class;
};

G_DEFINE_TYPE (Demo4Widget, demo4_widget, BOBGUI_TYPE_WIDGET)

static void
rotate_color (GdkRGBA *rgba)
{
  GdkHSLA hsla;

  _gdk_hsla_init_from_rgba (&hsla, rgba);
  hsla.hue -= 1;
  _gdk_rgba_init_from_hsla (rgba, &hsla);
}

static gboolean
rotate_colors (BobguiWidget     *widget,
               GdkFrameClock *clock,
               gpointer       user_data)
{
  Demo4Widget *self = DEMO4_WIDGET (widget);

  for (unsigned int i = 0; i < self->n_stops; i++)
    rotate_color (&self->stops[i].color);

  bobgui_widget_queue_draw (widget);

  return G_SOURCE_CONTINUE;
}

static void
demo4_widget_init (Demo4Widget *self)
{
  PangoContext *context;
  PangoFontDescription *desc;

  self->progress = 0.5;

  self->n_stops = 8;
  self->stops[0].offset = 0;
  self->stops[0].color = (GdkRGBA) { 1, 0, 0, 1 };

  for (unsigned int i = 1; i < self->n_stops; i++)
    {
      GdkHSLA hsla;

      self->stops[i].offset = i / (double)(self->n_stops - 1);
      _gdk_hsla_init_from_rgba (&hsla, &self->stops[i - 1].color);
      hsla.hue += 360.0 / (double)(self->n_stops - 1);
      _gdk_rgba_init_from_hsla (&self->stops[i].color, &hsla);
    }

  self->layout = bobgui_widget_create_pango_layout (BOBGUI_WIDGET (self), "123");
  context = pango_layout_get_context (self->layout);
  desc = pango_font_description_copy (pango_context_get_font_description (context));
  pango_font_description_set_weight (desc, PANGO_WEIGHT_BOLD);
  pango_font_description_set_size (desc, 210 * PANGO_SCALE);
  pango_layout_set_font_description (self->layout, desc);
  pango_font_description_free (desc);

  self->tick = bobgui_widget_add_tick_callback (BOBGUI_WIDGET (self), rotate_colors, NULL, NULL);
}

static void
demo4_widget_dispose (GObject *object)
{
  Demo4Widget *self = DEMO4_WIDGET (object);

  g_clear_object (&self->layout);
  bobgui_widget_remove_tick_callback (BOBGUI_WIDGET (self), self->tick);

  G_OBJECT_CLASS (demo4_widget_parent_class)->dispose (object);
}

static void
demo4_widget_snapshot_content (BobguiWidget   *widget,
                               BobguiSnapshot *snapshot,
                               GskMaskMode  mode)
{
  Demo4Widget *self = DEMO4_WIDGET (widget);
  int width, height, layout_width, layout_height;
  double scale;

  width = bobgui_widget_get_width (widget);
  height = bobgui_widget_get_height (widget);

  bobgui_snapshot_push_mask (snapshot, mode);
  pango_layout_get_pixel_size (self->layout, &layout_width, &layout_height);
  scale = MIN ((double) width / layout_width, (double) height / layout_height);
  bobgui_snapshot_translate (snapshot,
                          &GRAPHENE_POINT_INIT ((width - scale * layout_width) / 2,
                                                (height - scale * layout_height) / 2));
  bobgui_snapshot_scale (snapshot, scale, scale);
  bobgui_snapshot_append_layout (snapshot, self->layout, &(GdkRGBA) { 0, 0, 0, 1 });
  bobgui_snapshot_pop (snapshot);

  bobgui_snapshot_append_linear_gradient (snapshot,
                                       &GRAPHENE_RECT_INIT (0, 0, width, height),
                                       &GRAPHENE_POINT_INIT (0, 0),
                                       &GRAPHENE_POINT_INIT (width, height),
                                       self->stops,
                                       self->n_stops);
  bobgui_snapshot_pop (snapshot);
}

static void
demo4_widget_snapshot (BobguiWidget   *widget,
                       BobguiSnapshot *snapshot)
{
  Demo4Widget *self = DEMO4_WIDGET (widget);
  int width, height;

  width = bobgui_widget_get_width (widget);
  height = bobgui_widget_get_height (widget);

  bobgui_snapshot_push_mask (snapshot, GSK_MASK_MODE_INVERTED_LUMINANCE);
  bobgui_snapshot_append_linear_gradient (snapshot,
                                       &GRAPHENE_RECT_INIT (0, 0, width, height),
                                       &GRAPHENE_POINT_INIT (0, 0),
                                       &GRAPHENE_POINT_INIT (width, 0),
                                       (GskColorStop[2]) {
                                         { MAX (0.0, self->progress - 5.0 / width), { 1, 1, 1, 1 } },
                                         { MIN (1.0, self->progress + 5.0 / width), { 0, 0, 0, 1 } }
                                       }, 2);
  bobgui_snapshot_pop (snapshot);
  demo4_widget_snapshot_content (widget, snapshot, GSK_MASK_MODE_INVERTED_ALPHA);
  bobgui_snapshot_pop (snapshot);

  bobgui_snapshot_push_mask (snapshot, GSK_MASK_MODE_LUMINANCE);
  bobgui_snapshot_append_linear_gradient (snapshot,
                                       &GRAPHENE_RECT_INIT (0, 0, width, height),
                                       &GRAPHENE_POINT_INIT (0, 0),
                                       &GRAPHENE_POINT_INIT (width, 0),
                                       (GskColorStop[2]) {
                                         { MAX (0.0, self->progress - 5.0 / width), { 1, 1, 1, 1 } },
                                         { MIN (1.0, self->progress + 5.0 / width), { 0, 0, 0, 1 } }
                                       }, 2);
  bobgui_snapshot_pop (snapshot);
  demo4_widget_snapshot_content (widget, snapshot, GSK_MASK_MODE_ALPHA);
  bobgui_snapshot_pop (snapshot);
}

static void
demo4_widget_set_property (GObject      *object,
                           guint         prop_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  Demo4Widget *self = DEMO4_WIDGET (object);

  switch (prop_id)
    {
    case PROP_PROGRESS:
      self->progress = g_value_get_double (value);
      bobgui_widget_queue_draw (BOBGUI_WIDGET (object));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
demo4_widget_get_property (GObject     *object,
                           guint        prop_id,
                           GValue      *value,
                           GParamSpec  *pspec)
{
  Demo4Widget *self = DEMO4_WIDGET (object);

  switch (prop_id)
    {
    case PROP_PROGRESS:
      g_value_set_double (value, self->progress);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
demo4_widget_class_init (Demo4WidgetClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (class);

  object_class->dispose = demo4_widget_dispose;
  object_class->get_property = demo4_widget_get_property;
  object_class->set_property = demo4_widget_set_property;

  widget_class->snapshot = demo4_widget_snapshot;

  g_object_class_install_property (object_class, PROP_PROGRESS,
      g_param_spec_double ("progress", NULL, NULL,
                           0.0, 1.0, 0.5,
                           G_PARAM_READWRITE));
}

BobguiWidget *
demo4_widget_new (void)
{
  return g_object_new (DEMO4_TYPE_WIDGET, NULL);
}

