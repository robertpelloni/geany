#include <bobgui/bobgui.h>

const int KEEP_HEIGHT = 1 << 0;
const int KEEP_WIDTH  = 1 << 1;

static void
keep_size (int      direction,
           guint    transition_type,
           gboolean animations)
{
  gboolean animations_before;
  int min_height, min_width;
  int min_child_width, min_child_height;
  BobguiRevealer *revealer = BOBGUI_REVEALER (bobgui_revealer_new ());
  BobguiWidget   *child    = bobgui_button_new_with_label ("Some Text!");
  BobguiSettings *settings = bobgui_settings_get_default ();

  g_object_get (settings, "bobgui-enable-animations", &animations_before, NULL);
  g_object_set (settings, "bobgui-enable-animations", animations, NULL);

  bobgui_revealer_set_child (BOBGUI_REVEALER (revealer), child);

  bobgui_revealer_set_transition_type (revealer, transition_type);

  bobgui_revealer_set_reveal_child (revealer, TRUE);

  bobgui_widget_measure (child, BOBGUI_ORIENTATION_HORIZONTAL, -1,
                      &min_child_width, NULL, NULL, NULL);
  bobgui_widget_measure (child, BOBGUI_ORIENTATION_VERTICAL, -1,
                      &min_child_height, NULL, NULL, NULL);

  bobgui_widget_measure (BOBGUI_WIDGET (revealer), BOBGUI_ORIENTATION_HORIZONTAL, -1,
                      &min_width, NULL, NULL, NULL);
  bobgui_widget_measure (BOBGUI_WIDGET (revealer), BOBGUI_ORIENTATION_VERTICAL, -1,
                      &min_height, NULL, NULL, NULL);

  g_assert_cmpint (min_width, ==, min_child_width);
  g_assert_cmpint (min_height, ==, min_child_height);


  bobgui_revealer_set_reveal_child (revealer, FALSE);
  bobgui_widget_measure (BOBGUI_WIDGET (revealer), BOBGUI_ORIENTATION_HORIZONTAL, -1,
                      &min_width, NULL, NULL, NULL);
  bobgui_widget_measure (BOBGUI_WIDGET (revealer), BOBGUI_ORIENTATION_VERTICAL, -1,
                      &min_height, NULL, NULL, NULL);

  if (direction & KEEP_WIDTH)
    g_assert_cmpint (min_width, ==, min_child_width);
  else
    g_assert_cmpint (min_width, ==, 0);

  if (direction & KEEP_HEIGHT)
    g_assert_cmpint (min_height, ==, min_child_height);
  else
    g_assert_cmpint (min_height, ==, 0);

  g_object_set (settings, "bobgui-enable-animations", animations_before, NULL);
}


static void
slide_right_animations (void)
{
  keep_size (KEEP_HEIGHT, BOBGUI_REVEALER_TRANSITION_TYPE_SLIDE_RIGHT, TRUE);
}

static void
slide_right_no_animations (void)
{
  keep_size (KEEP_HEIGHT, BOBGUI_REVEALER_TRANSITION_TYPE_SLIDE_RIGHT, FALSE);
}

static void
slide_left_animations (void)
{
  keep_size (KEEP_HEIGHT, BOBGUI_REVEALER_TRANSITION_TYPE_SLIDE_LEFT, TRUE);
}

static void
slide_left_no_animations (void)
{
  keep_size (KEEP_HEIGHT, BOBGUI_REVEALER_TRANSITION_TYPE_SLIDE_LEFT, FALSE);
}

static void
none_animations (void)
{
  keep_size (KEEP_WIDTH | KEEP_HEIGHT, BOBGUI_REVEALER_TRANSITION_TYPE_NONE, TRUE);
}

static void
none_no_animations (void)
{
  keep_size (KEEP_WIDTH | KEEP_HEIGHT, BOBGUI_REVEALER_TRANSITION_TYPE_NONE, FALSE);
}

static void
crossfade_animations (void)
{
  keep_size (KEEP_WIDTH | KEEP_HEIGHT, BOBGUI_REVEALER_TRANSITION_TYPE_CROSSFADE, TRUE);
}

static void
crossfade_no_animations (void)
{
  keep_size (KEEP_WIDTH | KEEP_HEIGHT, BOBGUI_REVEALER_TRANSITION_TYPE_CROSSFADE, FALSE);
}

static void
slide_down_animations (void)
{
  keep_size (KEEP_WIDTH, BOBGUI_REVEALER_TRANSITION_TYPE_SLIDE_DOWN, TRUE);
}

static void
slide_down_no_animations (void)
{
  keep_size (KEEP_WIDTH, BOBGUI_REVEALER_TRANSITION_TYPE_SLIDE_DOWN, FALSE);
}

static void
slide_up_animations (void)
{
  keep_size (KEEP_WIDTH, BOBGUI_REVEALER_TRANSITION_TYPE_SLIDE_UP, TRUE);
}

static void
slide_up_no_animations (void)
{
  keep_size (KEEP_WIDTH, BOBGUI_REVEALER_TRANSITION_TYPE_SLIDE_UP, FALSE);
}

int
main (int argc, char **argv)
{
  bobgui_init ();
  (g_test_init) (&argc, &argv, NULL);

  g_test_add_func ("/sizing/revealer/slide_right_animations", slide_right_animations);
  g_test_add_func ("/sizing/revealer/slide_right_no_animations", slide_right_no_animations);

  g_test_add_func ("/sizing/revealer/slide_left_animations", slide_left_animations);
  g_test_add_func ("/sizing/revealer/slide_left_no_animations", slide_left_no_animations);

  g_test_add_func ("/sizing/revealer/none_animations", none_animations);
  g_test_add_func ("/sizing/revealer/none_no_animations", none_no_animations);

  g_test_add_func ("/sizing/revealer/crossfade_animations", crossfade_animations);
  g_test_add_func ("/sizing/revealer/crossfade_no_animations", crossfade_no_animations);

  g_test_add_func ("/sizing/revealer/slide_down_animations", slide_down_animations);
  g_test_add_func ("/sizing/revealer/slide_down_no_animations", slide_down_no_animations);

  g_test_add_func ("/sizing/revealer/slide_up_animations", slide_up_animations);
  g_test_add_func ("/sizing/revealer/slide_up_no_animations", slide_up_no_animations);

  return g_test_run ();
}
