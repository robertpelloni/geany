#include <bobgui/bobgui.h>
#include "bobgui/bobguiatcontextprivate.h"
#include "bobgui/bobguiwidgetprivate.h"

static void
test_name_content (void)
{
  BobguiWidget *window, *label1, *label2, *box, *button;
  BobguiATContext *context;
  char *name;

  label1 = bobgui_label_new ("a");
  label2 = bobgui_label_new ("b");
  box = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
  button = bobgui_button_new ();

  bobgui_box_append (BOBGUI_BOX (box), label1);
  bobgui_box_append (BOBGUI_BOX (box), label2);
  bobgui_button_set_child (BOBGUI_BUTTON (button), box);

  window = bobgui_window_new ();
  bobgui_window_set_child (BOBGUI_WINDOW (window), button);
  bobgui_window_present (BOBGUI_WINDOW (window));

  context = bobgui_accessible_get_at_context (BOBGUI_ACCESSIBLE (label1));
  name = bobgui_at_context_get_name (context);
  g_assert_cmpstr (name, ==, "a");
  g_free (name);
  g_object_unref (context);

  /* this is because generic doesn't allow naming */
  context = bobgui_accessible_get_at_context (BOBGUI_ACCESSIBLE (box));
  name = bobgui_at_context_get_name (context);
  g_assert_cmpstr (name, ==, "");
  g_free (name);
  g_object_unref (context);

  context = bobgui_accessible_get_at_context (BOBGUI_ACCESSIBLE (button));
  name = bobgui_at_context_get_name (context);
  g_assert_cmpstr (name, ==, "a b");
  g_free (name);
  g_object_unref (context);

  bobgui_widget_set_visible (label2, FALSE);

  context = bobgui_accessible_get_at_context (BOBGUI_ACCESSIBLE (button));
  name = bobgui_at_context_get_name (context);
  g_assert_cmpstr (name, ==, "a");
  g_free (name);
  g_object_unref (context);

  bobgui_window_destroy (BOBGUI_WINDOW (window));
}

static void
test_name_tooltip (void)
{
  BobguiWidget *window, *image;
  BobguiATContext *context;
  char *name;

  image = bobgui_image_new ();

  window = bobgui_window_new ();
  bobgui_window_set_child (BOBGUI_WINDOW (window), image);
  bobgui_window_present (BOBGUI_WINDOW (window));

  bobgui_widget_set_tooltip_text (image, "tooltip");

  context = bobgui_accessible_get_at_context (BOBGUI_ACCESSIBLE (image));

  name = bobgui_at_context_get_name (context);
  g_assert_cmpstr (name, ==, "tooltip");
  g_free (name);

  g_object_unref (context);

  bobgui_window_destroy (BOBGUI_WINDOW (window));
}

static void
test_name_menubutton (void)
{
  BobguiWidget *window, *widget;
  BobguiATContext *context;
  char *name;

  widget = bobgui_menu_button_new ();
  bobgui_menu_button_set_popover (BOBGUI_MENU_BUTTON (widget), bobgui_popover_new ());

  window = bobgui_window_new ();
  bobgui_window_set_child (BOBGUI_WINDOW (window), widget);
  bobgui_window_present (BOBGUI_WINDOW (window));

  bobgui_widget_set_tooltip_text (widget, "tooltip");

  context = bobgui_accessible_get_at_context (BOBGUI_ACCESSIBLE (widget));

  name = bobgui_at_context_get_name (context);
  g_assert_cmpstr (name, ==, "tooltip");
  g_free (name);

  g_object_unref (context);

  bobgui_window_destroy (BOBGUI_WINDOW (window));
}

static void
test_name_label (void)
{
  BobguiWidget *window, *image;
  BobguiATContext *context;
  char *name;
  char *desc;

  image = bobgui_image_new ();

  window = bobgui_window_new ();
  bobgui_window_set_child (BOBGUI_WINDOW (window), image);
  bobgui_window_present (BOBGUI_WINDOW (window));

  g_object_ref_sink (image);
  bobgui_widget_realize_at_context (image);

  bobgui_widget_set_tooltip_text (image, "tooltip");

  bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (image),
                                  BOBGUI_ACCESSIBLE_PROPERTY_LABEL, "label",
                                  -1);

  context = bobgui_accessible_get_at_context (BOBGUI_ACCESSIBLE (image));

  name = bobgui_at_context_get_name (context);
  desc = bobgui_at_context_get_description (context);

  g_assert_cmpstr (name, ==, "label");
  g_assert_cmpstr (desc, ==, "tooltip");

  g_free (name);
  g_free (desc);

  g_object_unref (context);

  bobgui_window_destroy (BOBGUI_WINDOW (window));
}

static void
test_name_prohibited (void)
{
  BobguiWidget *window, *widget;
  BobguiATContext *context;
  char *name;
  char *desc;

  widget = g_object_new (BOBGUI_TYPE_BUTTON,
                         "accessible-role", BOBGUI_ACCESSIBLE_ROLE_TIME,
                         "label", "too late",
                         NULL);

  window = bobgui_window_new ();
  bobgui_window_set_child (BOBGUI_WINDOW (window), widget);
  bobgui_window_present (BOBGUI_WINDOW (window));

  context = bobgui_accessible_get_at_context (BOBGUI_ACCESSIBLE (widget));

  name = bobgui_at_context_get_name (context);
  desc = bobgui_at_context_get_description (context);

  g_assert_cmpstr (name, ==, "too late");
  g_assert_cmpstr (desc, ==, "");

  g_free (name);
  g_free (desc);

  g_object_unref (context);

  bobgui_window_destroy (BOBGUI_WINDOW (window));
}

static void
test_name_range (void)
{
  BobguiWidget *window, *scale;
  BobguiATContext *context;
  char *name;

  scale = bobgui_scale_new_with_range (BOBGUI_ORIENTATION_HORIZONTAL, 0, 100, 10);

  window = bobgui_window_new ();
  bobgui_window_set_child (BOBGUI_WINDOW (window), scale);
  bobgui_window_present (BOBGUI_WINDOW (window));

  context = bobgui_accessible_get_at_context (BOBGUI_ACCESSIBLE (scale));

  g_assert_true (bobgui_accessible_get_accessible_role (BOBGUI_ACCESSIBLE (scale)) == BOBGUI_ACCESSIBLE_ROLE_SLIDER);
  g_assert_true (bobgui_at_context_get_accessible_role (context) == BOBGUI_ACCESSIBLE_ROLE_SLIDER);

  bobgui_range_set_value (BOBGUI_RANGE (scale), 50);

  name = bobgui_at_context_get_name (context);
  g_assert_cmpstr (name, ==, "");

  g_free (name);

  g_object_unref (context);

  bobgui_window_destroy (BOBGUI_WINDOW (window));
}

int
main (int argc, char *argv[])
{
  bobgui_test_init (&argc, &argv, NULL);

  g_test_add_func ("/a11y/name/content", test_name_content);
  g_test_add_func ("/a11y/name/tooltip", test_name_tooltip);
  g_test_add_func ("/a11y/name/menubutton", test_name_menubutton);
  g_test_add_func ("/a11y/name/label", test_name_label);
  g_test_add_func ("/a11y/name/prohibited", test_name_prohibited);
  g_test_add_func ("/a11y/name/range", test_name_range);

  return g_test_run ();
}
