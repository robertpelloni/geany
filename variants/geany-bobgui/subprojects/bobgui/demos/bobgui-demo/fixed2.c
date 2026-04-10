/* Fixed Layout / Transformations
 * #Keywords: BobguiLayoutManager
 *
 * BobguiFixed is a container that allows placing and transforming
 * widgets manually.
 *
 * This demo shows how to rotate and scale a child widget using
 * a transform.
 */

#include <bobgui/bobgui.h>

static BobguiWidget *demo_window = NULL;
static gint64 start_time = 0;

static gboolean
tick_cb (BobguiWidget     *widget,
         GdkFrameClock *frame_clock,
         gpointer       user_data)
{
  BobguiFixed *fixed = BOBGUI_FIXED (widget);
  BobguiWidget *child = user_data;
  gint64 now = g_get_monotonic_time ();
  double duration;
  double angle;
  double width, height;
  double child_width, child_height;
  GskTransform *transform;
  double scale;

  duration = (now - start_time) / (double) G_TIME_SPAN_SECOND;

  width = bobgui_widget_get_width (widget);
  height = bobgui_widget_get_height (widget);

  child_width = bobgui_widget_get_width (child);
  child_height = bobgui_widget_get_height (child);

  angle = duration * 90;
  scale = 2 + sin (duration * M_PI);

  transform = gsk_transform_translate (
      gsk_transform_scale (
          gsk_transform_rotate (
              gsk_transform_translate (NULL,
                  &GRAPHENE_POINT_INIT (width / 2, height / 2)),
              angle),
          scale, scale),
      &GRAPHENE_POINT_INIT (- child_width / 2,  - child_height / 2));

  bobgui_fixed_set_child_transform (fixed, child, transform);

  gsk_transform_unref (transform);

  return G_SOURCE_CONTINUE;
}

static BobguiWidget *
create_demo_window (BobguiWidget *do_widget)
{
  BobguiWidget *window, *sw, *fixed, *child;

  window = bobgui_window_new ();
  bobgui_window_set_display (BOBGUI_WINDOW (window),  bobgui_widget_get_display (do_widget));
  bobgui_window_set_title (BOBGUI_WINDOW (window), "Fixed Layout ‐ Transformations");
  bobgui_window_set_default_size (BOBGUI_WINDOW (window), 400, 300);

  sw = bobgui_scrolled_window_new ();
  bobgui_window_set_child (BOBGUI_WINDOW (window), sw);

  fixed = bobgui_fixed_new ();
  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (sw), fixed);

  child = bobgui_label_new ("All fixed?");
  bobgui_fixed_put (BOBGUI_FIXED (fixed), child, 0, 0);
  bobgui_widget_set_overflow (fixed, BOBGUI_OVERFLOW_VISIBLE);

  bobgui_widget_add_tick_callback (fixed, tick_cb, child, NULL);

  return window;
}

BobguiWidget*
do_fixed2 (BobguiWidget *do_widget)
{
  if (demo_window == NULL)
    demo_window = create_demo_window (do_widget);

  start_time = g_get_monotonic_time ();

  if (!bobgui_widget_get_visible (demo_window))
    bobgui_widget_set_visible (demo_window, TRUE);
  else
    bobgui_window_destroy (BOBGUI_WINDOW (demo_window));

  return demo_window;
}
