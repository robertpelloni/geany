#include <bobgui/bobgui.h>

#if 0
#define bobgui_event_controller_motion_new bobgui_drop_controller_motion_new
#define bobgui_event_controller_motion_contains_pointer bobgui_drop_controller_motion_contains_pointer
#define bobgui_event_controller_motion_is_pointer bobgui_drop_controller_motion_is_pointer
#undef BOBGUI_EVENT_CONTROLLER_MOTION
#define BOBGUI_EVENT_CONTROLLER_MOTION BOBGUI_DROP_CONTROLLER_MOTION
#endif

static void
quit_cb (BobguiWidget *widget,
         gpointer   unused)
{
  g_main_context_wakeup (NULL);
}

static void
enter_annoy_cb (BobguiEventController *controller,
                double              x,
                double              y)
{
  BobguiWidget *widget = bobgui_event_controller_get_widget (controller);
  BobguiWindow *window = BOBGUI_WINDOW (bobgui_widget_get_root (widget));

  g_print ("%15s ENTER %s %g, %g\n",
           bobgui_window_get_title (window),
           bobgui_event_controller_motion_contains_pointer (BOBGUI_EVENT_CONTROLLER_MOTION (controller))
           ? bobgui_event_controller_motion_is_pointer (BOBGUI_EVENT_CONTROLLER_MOTION (controller))
             ? "IS     "
             : "CONTAIN"
             : "       ",
           x, y);
}

static void
motion_annoy_cb (BobguiEventController *controller,
                 double              x,
                 double              y)
{
  BobguiWidget *widget = bobgui_event_controller_get_widget (controller);
  BobguiWindow *window = BOBGUI_WINDOW (bobgui_widget_get_root (widget));

  g_print ("%15s MOVE  %s %g, %g\n",
           bobgui_window_get_title (window),
           bobgui_event_controller_motion_contains_pointer (BOBGUI_EVENT_CONTROLLER_MOTION (controller))
           ? bobgui_event_controller_motion_is_pointer (BOBGUI_EVENT_CONTROLLER_MOTION (controller))
             ? "IS     "
             : "CONTAIN"
             : "       ",
           x, y);
}

static void
leave_annoy_cb (BobguiEventController *controller)
{
  BobguiWidget *widget = bobgui_event_controller_get_widget (controller);
  BobguiWindow *window = BOBGUI_WINDOW (bobgui_widget_get_root (widget));

  g_print ("%15s LEAVE %s\n",
           bobgui_window_get_title (window),
           bobgui_event_controller_motion_contains_pointer (BOBGUI_EVENT_CONTROLLER_MOTION (controller))
           ? bobgui_event_controller_motion_is_pointer (BOBGUI_EVENT_CONTROLLER_MOTION (controller))
             ? "IS     "
             : "CONTAIN"
             : "       ");
}

static BobguiEventController *
annoying_event_controller_motion_new (void)
{
  BobguiEventController *controller = bobgui_event_controller_motion_new ();

  g_signal_connect (controller, "enter", G_CALLBACK (enter_annoy_cb), NULL);
  g_signal_connect (controller, "motion", G_CALLBACK (motion_annoy_cb), NULL);
  g_signal_connect (controller, "leave", G_CALLBACK (leave_annoy_cb), NULL);

  return controller;
}

/*** TEST 1: remove()/add() ***/

static void
enter1_cb (BobguiEventController *controller)
{
  BobguiWidget *box = bobgui_event_controller_get_widget (controller);

  bobgui_box_remove (BOBGUI_BOX (box), bobgui_widget_get_first_child (box));
  bobgui_box_append (BOBGUI_BOX (box), bobgui_label_new ("HOVER!"));
}

static void
leave1_cb (BobguiEventController *controller)
{
  BobguiWidget *box = bobgui_event_controller_get_widget (controller);

  bobgui_box_remove (BOBGUI_BOX (box), bobgui_widget_get_first_child (box));
  bobgui_box_append (BOBGUI_BOX (box), bobgui_image_new_from_icon_name ("start-here"));
}

static void
test1 (void)
{
  BobguiWidget *win;
  BobguiWidget *box;
  BobguiEventController *controller;
  win = bobgui_window_new ();
  bobgui_window_set_default_size (BOBGUI_WINDOW (win), 400, 300);
  bobgui_window_set_title (BOBGUI_WINDOW (win), "add/remove");

  box = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, FALSE);
  bobgui_window_set_child (BOBGUI_WINDOW (win), box);
  controller = annoying_event_controller_motion_new ();
  g_signal_connect (controller, "enter", G_CALLBACK (enter1_cb), NULL);
  g_signal_connect (controller, "leave", G_CALLBACK (leave1_cb), NULL);
  bobgui_widget_add_controller (box, controller);

  bobgui_box_append (BOBGUI_BOX (box), bobgui_image_new_from_icon_name ("start-here"));

  bobgui_window_present (BOBGUI_WINDOW (win));

  g_signal_connect (win, "destroy", G_CALLBACK (quit_cb), NULL);
}

/*** TEST 2: hide()/show() ***/

static void
enter2_cb (BobguiEventController *controller)
{
  BobguiWidget *box = bobgui_event_controller_get_widget (controller);

  bobgui_widget_set_visible (bobgui_widget_get_first_child (box), FALSE);
  bobgui_widget_set_visible (bobgui_widget_get_last_child (box), TRUE);
}

static void
leave2_cb (BobguiEventController *controller)
{
  BobguiWidget *box = bobgui_event_controller_get_widget (controller);

  bobgui_widget_set_visible (bobgui_widget_get_first_child (box), TRUE);
  bobgui_widget_set_visible (bobgui_widget_get_last_child (box), FALSE);
}

static void
test2 (void)
{
  BobguiWidget *win;
  BobguiWidget *box;
  BobguiEventController *controller;
  win = bobgui_window_new ();
  bobgui_window_set_default_size (BOBGUI_WINDOW (win), 400, 300);
  bobgui_window_set_title (BOBGUI_WINDOW (win), "show/hide");

  box = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, FALSE);
  bobgui_window_set_child (BOBGUI_WINDOW (win), box);
  controller = annoying_event_controller_motion_new ();
  g_signal_connect (controller, "enter", G_CALLBACK (enter2_cb), NULL);
  g_signal_connect (controller, "leave", G_CALLBACK (leave2_cb), NULL);
  bobgui_widget_add_controller (box, controller);

  bobgui_box_append (BOBGUI_BOX (box), bobgui_image_new_from_icon_name ("start-here"));
  bobgui_box_append (BOBGUI_BOX (box), bobgui_label_new ("HOVER!"));
  bobgui_widget_set_visible (bobgui_widget_get_last_child (box), FALSE);

  bobgui_window_present (BOBGUI_WINDOW (win));

  g_signal_connect (win, "destroy", G_CALLBACK (quit_cb), NULL);
}

/*** TEST 3: set_child_visible() ***/

static void
enter3_cb (BobguiEventController *controller)
{
  BobguiWidget *stack = bobgui_event_controller_get_widget (controller);

  bobgui_stack_set_visible_child_name (BOBGUI_STACK (stack), "enter");
}

static void
leave3_cb (BobguiEventController *controller)
{
  BobguiWidget *stack = bobgui_event_controller_get_widget (controller);

  bobgui_stack_set_visible_child_name (BOBGUI_STACK (stack), "leave");
}

static void
test3 (void)
{
  BobguiWidget *win;
  BobguiWidget *stack;
  BobguiEventController *controller;
  win = bobgui_window_new ();
  bobgui_window_set_default_size (BOBGUI_WINDOW (win), 400, 300);
  bobgui_window_set_title (BOBGUI_WINDOW (win), "child-visible");

  stack = bobgui_stack_new ();
  bobgui_window_set_child (BOBGUI_WINDOW (win), stack);
  controller = annoying_event_controller_motion_new ();
  g_signal_connect (controller, "enter", G_CALLBACK (enter3_cb), NULL);
  g_signal_connect (controller, "leave", G_CALLBACK (leave3_cb), NULL);
  bobgui_widget_add_controller (stack, controller);

  bobgui_stack_add_named (BOBGUI_STACK (stack), bobgui_image_new_from_icon_name ("start-here"), "leave");
  bobgui_stack_add_named (BOBGUI_STACK (stack), bobgui_label_new ("HOVER!"), "enter");
  bobgui_stack_set_visible_child_name (BOBGUI_STACK (stack), "leave");

  bobgui_window_present (BOBGUI_WINDOW (win));

  g_signal_connect (win, "destroy", G_CALLBACK (quit_cb), NULL);
}

/*** TEST 4: move ***/

static void
enter4_cb (BobguiEventController *controller)
{
  BobguiWidget *fixed = bobgui_event_controller_get_widget (controller);

  bobgui_fixed_move (BOBGUI_FIXED (fixed), bobgui_widget_get_first_child (fixed), -1000, -1000);
  bobgui_fixed_move (BOBGUI_FIXED (fixed), bobgui_widget_get_last_child (fixed), 0, 0);
}

static void
leave4_cb (BobguiEventController *controller)
{
  BobguiWidget *fixed = bobgui_event_controller_get_widget (controller);

  bobgui_fixed_move (BOBGUI_FIXED (fixed), bobgui_widget_get_first_child (fixed), 0, 0);
  bobgui_fixed_move (BOBGUI_FIXED (fixed), bobgui_widget_get_last_child (fixed), -1000, -1000);
}

static void
test4 (void)
{
  BobguiWidget *win;
  BobguiWidget *fixed;
  BobguiEventController *controller;
  win = bobgui_window_new ();
  bobgui_window_set_default_size (BOBGUI_WINDOW (win), 400, 300);
  bobgui_window_set_title (BOBGUI_WINDOW (win), "move");

  fixed = bobgui_fixed_new ();
  bobgui_window_set_child (BOBGUI_WINDOW (win), fixed);
  controller = annoying_event_controller_motion_new ();
  g_signal_connect (controller, "enter", G_CALLBACK (enter4_cb), NULL);
  g_signal_connect (controller, "leave", G_CALLBACK (leave4_cb), NULL);
  bobgui_widget_add_controller (fixed, controller);

  bobgui_fixed_put (BOBGUI_FIXED (fixed), bobgui_image_new_from_icon_name ("start-here"), 0, 0);
  bobgui_fixed_put (BOBGUI_FIXED (fixed), bobgui_label_new ("HOVER!"), -1000, -1000);

  bobgui_window_present (BOBGUI_WINDOW (win));

  g_signal_connect (win, "destroy", G_CALLBACK (quit_cb), NULL);
}

int
main (int argc, char *argv[])
{
  BobguiCssProvider *provider;

  bobgui_init ();

  provider = bobgui_css_provider_new ();
  bobgui_css_provider_load_from_string (provider,
      ":hover {"
      "   box-shadow: inset 0px 0px 0px 1px red;"
      " }"
      " window :not(.title):hover {"
      "   background: yellow;"
      " }"
      " window :not(.title):hover * {"
      "   background: goldenrod;"
      " }");
  bobgui_style_context_add_provider_for_display (gdk_display_get_default (), BOBGUI_STYLE_PROVIDER (provider), 800);
  g_object_unref (provider);

  test1();
  test2();
  test3();
  test4();

  while (bobgui_window_list_toplevels ())
    g_main_context_iteration (NULL, TRUE);

  return 0;
}
