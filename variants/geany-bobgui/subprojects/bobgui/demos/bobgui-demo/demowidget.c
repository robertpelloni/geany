#include "demowidget.h"
#include "demolayout.h"

/* parent widget */

struct _DemoWidget
{
  BobguiWidget parent_instance;

  gboolean backward; /* whether we go 0 -> 1 or 1 -> 0 */
  gint64 start_time; /* time the transition started */
  guint tick_id;     /* our tick cb */
};

struct _DemoWidgetClass
{
  BobguiWidgetClass parent_class;
};

G_DEFINE_TYPE (DemoWidget, demo_widget, BOBGUI_TYPE_WIDGET)

/* The widget is controlling the transition by calling
 * demo_layout_set_position() in a tick callback.
 *
 * We take half a second to go from one layout to the other.
 */

#define DURATION (0.5 * G_TIME_SPAN_SECOND)

static gboolean
transition (BobguiWidget     *widget,
            GdkFrameClock *frame_clock,
            gpointer       data)
{
  DemoWidget *self = DEMO_WIDGET (widget);
  DemoLayout *demo_layout = DEMO_LAYOUT (bobgui_widget_get_layout_manager (widget));
  gint64 now = gdk_frame_clock_get_frame_time (frame_clock);

  bobgui_widget_queue_allocate (widget);

  if (self->backward)
    demo_layout_set_position (demo_layout, 1.0 - (now - self->start_time) / DURATION);
  else
    demo_layout_set_position (demo_layout, (now - self->start_time) / DURATION);

  if (now - self->start_time >= DURATION)
    {
      self->backward = !self->backward;
      demo_layout_set_position (demo_layout, self->backward ? 1.0 : 0.0);
      /* keep things interesting by shuffling the positions */
      if (!self->backward)
        demo_layout_shuffle (demo_layout);
      self->tick_id = 0;

      return G_SOURCE_REMOVE;
    }

  return G_SOURCE_CONTINUE;
}

static void
clicked (BobguiGestureClick *gesture,
         guint            n_press,
         double           x,
         double           y,
         gpointer         data)
{
  DemoWidget *self = data;
  GdkFrameClock *frame_clock;

  if (self->tick_id != 0)
    return;

  frame_clock = bobgui_widget_get_frame_clock (BOBGUI_WIDGET (self));
  self->start_time = gdk_frame_clock_get_frame_time (frame_clock);
  self->tick_id = bobgui_widget_add_tick_callback (BOBGUI_WIDGET (self), transition, NULL, NULL);
}

static void
demo_widget_init (DemoWidget *self)
{
  BobguiGesture *gesture;

  gesture = bobgui_gesture_click_new ();
  g_signal_connect (gesture, "pressed", G_CALLBACK (clicked), self);
  bobgui_widget_add_controller (BOBGUI_WIDGET (self), BOBGUI_EVENT_CONTROLLER (gesture));
}

static void
demo_widget_dispose (GObject *object)
{
  BobguiWidget *child;

  while ((child = bobgui_widget_get_first_child (BOBGUI_WIDGET (object))))
    bobgui_widget_unparent (child);

  G_OBJECT_CLASS (demo_widget_parent_class)->dispose (object);
}

static void
demo_widget_class_init (DemoWidgetClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (class);

  object_class->dispose = demo_widget_dispose;

  /* here is where we use our custom layout manager */
  bobgui_widget_class_set_layout_manager_type (widget_class, DEMO_TYPE_LAYOUT);
}

BobguiWidget *
demo_widget_new (void)
{
  return g_object_new (DEMO_TYPE_WIDGET, NULL);
}

void
demo_widget_add_child (DemoWidget *self,
                       BobguiWidget  *child)
{
  bobgui_widget_set_parent (child, BOBGUI_WIDGET (self));
}
