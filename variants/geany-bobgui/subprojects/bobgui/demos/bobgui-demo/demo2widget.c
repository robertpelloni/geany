#include "demo2widget.h"
#include "demo2layout.h"

struct _Demo2Widget
{
  BobguiWidget parent_instance;

  gint64 start_time;
  gint64 end_time;
  float start_position;
  float end_position;
  float start_offset;
  float end_offset;
  gboolean animating;
};

struct _Demo2WidgetClass
{
  BobguiWidgetClass parent_class;
};

G_DEFINE_TYPE (Demo2Widget, demo2_widget, BOBGUI_TYPE_WIDGET)

static void
demo2_widget_init (Demo2Widget *self)
{
  bobgui_widget_set_focusable (BOBGUI_WIDGET (self), TRUE);
}

static void
demo2_widget_dispose (GObject *object)
{
  BobguiWidget *child;

  while ((child = bobgui_widget_get_first_child (BOBGUI_WIDGET (object))))
    bobgui_widget_unparent (child);

  G_OBJECT_CLASS (demo2_widget_parent_class)->dispose (object);
}

/* From clutter-easing.c, based on Robert Penner's
 * infamous easing equations, MIT license.
 */
static double
ease_out_cubic (double t)
{
  double p = t - 1;

  return p * p * p + 1;
}

static gboolean
update_position (BobguiWidget     *widget,
                 GdkFrameClock *clock,
                 gpointer       data)
{
  Demo2Widget *self = DEMO2_WIDGET (widget);
  Demo2Layout *layout = DEMO2_LAYOUT (bobgui_widget_get_layout_manager (widget));
  gint64 now;
  double t;

  now = gdk_frame_clock_get_frame_time (clock);

  if (now >= self->end_time)
    {
      self->animating = FALSE;

      return G_SOURCE_REMOVE;
    }

  t = (now - self->start_time) / (double) (self->end_time - self->start_time);

  t = ease_out_cubic (t);

  demo2_layout_set_position (layout, self->start_position + t * (self->end_position - self->start_position));
  demo2_layout_set_offset (layout, self->start_offset + t * (self->end_offset - self->start_offset));
  bobgui_widget_queue_allocate (widget);

  return G_SOURCE_CONTINUE;
}

static void
rotate_sphere (BobguiWidget  *widget,
               const char *action,
               GVariant   *parameters)
{
  Demo2Widget *self = DEMO2_WIDGET (widget);
  Demo2Layout *layout = DEMO2_LAYOUT (bobgui_widget_get_layout_manager (widget));
  BobguiOrientation orientation;
  int direction;

  g_variant_get (parameters, "(ii)", &orientation, &direction);

  self->end_position = self->start_position = demo2_layout_get_position (layout);
  self->end_offset = self->start_offset = demo2_layout_get_offset (layout);
  if (orientation == BOBGUI_ORIENTATION_HORIZONTAL)
    self->end_position += 10 * direction;
  else
    self->end_offset += 10 * direction;
  self->start_time = g_get_monotonic_time ();
  self->end_time = self->start_time + 0.5 * G_TIME_SPAN_SECOND;

  if (!self->animating)
    {
      bobgui_widget_add_tick_callback (widget, update_position, NULL, NULL);
      self->animating = TRUE;
    }
}

static void
demo2_widget_snapshot (BobguiWidget   *widget,
                       BobguiSnapshot *snapshot)
{
  BobguiWidget *child;

  for (child = bobgui_widget_get_first_child (widget);
       child != NULL;
       child = bobgui_widget_get_next_sibling (child))
    {
      /* our layout manager sets this for children that are out of view */
      if (!bobgui_widget_get_child_visible (child))
        continue;

      bobgui_widget_snapshot_child (widget, child, snapshot);
    }
}

static void
demo2_widget_class_init (Demo2WidgetClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (class);

  object_class->dispose = demo2_widget_dispose;

  widget_class->snapshot = demo2_widget_snapshot;

  bobgui_widget_class_install_action (widget_class, "rotate", "(ii)", rotate_sphere);

  bobgui_widget_class_add_binding_action (widget_class,
                                       GDK_KEY_Left, 0,
                                       "rotate",
                                       "(ii)", BOBGUI_ORIENTATION_HORIZONTAL, -1);
  bobgui_widget_class_add_binding_action (widget_class,
                                       GDK_KEY_Right, 0,
                                       "rotate",
                                       "(ii)", BOBGUI_ORIENTATION_HORIZONTAL, 1);
  bobgui_widget_class_add_binding_action (widget_class,
                                       GDK_KEY_Up, 0,
                                       "rotate",
                                       "(ii)", BOBGUI_ORIENTATION_VERTICAL, 1);
  bobgui_widget_class_add_binding_action (widget_class,
                                       GDK_KEY_Down, 0,
                                       "rotate",
                                       "(ii)", BOBGUI_ORIENTATION_VERTICAL, -1);

  /* here is where we use our custom layout manager */
  bobgui_widget_class_set_layout_manager_type (widget_class, DEMO2_TYPE_LAYOUT);
}

BobguiWidget *
demo2_widget_new (void)
{
  return g_object_new (DEMO2_TYPE_WIDGET, NULL);
}

void
demo2_widget_add_child (Demo2Widget *self,
                        BobguiWidget   *child)
{
  bobgui_widget_set_parent (child, BOBGUI_WIDGET (self));
}
