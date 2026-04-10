#include <bobgui/bobgui.h>

static BobguiWidget *
create_level_bar (void)
{
  BobguiWidget *bar;

  bar = bobgui_level_bar_new ();
  bobgui_level_bar_set_min_value (BOBGUI_LEVEL_BAR (bar), 0.0);
  bobgui_level_bar_set_max_value (BOBGUI_LEVEL_BAR (bar), 10.0);

  bobgui_level_bar_add_offset_value (BOBGUI_LEVEL_BAR (bar),
                                  BOBGUI_LEVEL_BAR_OFFSET_LOW, 1.0);

  bobgui_level_bar_add_offset_value (BOBGUI_LEVEL_BAR (bar),
                                  BOBGUI_LEVEL_BAR_OFFSET_HIGH, 9.0);

  bobgui_level_bar_add_offset_value (BOBGUI_LEVEL_BAR (bar),
                                  "full", 10.0);

  bobgui_level_bar_add_offset_value (BOBGUI_LEVEL_BAR (bar),
                                  "my-offset", 5.0);

  return bar;
}

static void
add_custom_css (void)
{
  BobguiCssProvider *provider;
  const char data[] =
  "levelbar block.my-offset {"
  "   background: magenta;"
  "}";

  provider = bobgui_css_provider_new ();
  bobgui_css_provider_load_from_string (provider, data);
  bobgui_style_context_add_provider_for_display (gdk_display_get_default (),
                                              BOBGUI_STYLE_PROVIDER (provider),
                                              BOBGUI_STYLE_PROVIDER_PRIORITY_APPLICATION);
}

static gboolean
increase_level (gpointer data)
{
  BobguiLevelBar *bar = data;
  double value;

  value = bobgui_level_bar_get_value (bar);
  value += 0.1;
  if (value >= bobgui_level_bar_get_max_value (bar))
    value = bobgui_level_bar_get_min_value (bar);
  bobgui_level_bar_set_value (bar, value);

  return G_SOURCE_CONTINUE;
}

static void
toggle (BobguiSwitch *sw, GParamSpec *pspec, BobguiLevelBar *bar)
{
  if (bobgui_switch_get_active (sw))
    bobgui_level_bar_set_mode (bar, BOBGUI_LEVEL_BAR_MODE_DISCRETE);
  else
    bobgui_level_bar_set_mode (bar, BOBGUI_LEVEL_BAR_MODE_CONTINUOUS);
}

static void
quit_cb (BobguiWidget *widget,
         gpointer   data)
{
  gboolean *done = data;

  *done = TRUE;

  g_main_context_wakeup (NULL);
}

int
main (int argc, char *argv[])
{
  BobguiWidget *window;
  BobguiWidget *box;
  BobguiWidget *bar;
  BobguiWidget *box2;
  BobguiWidget *sw;
  gboolean done = FALSE;

  bobgui_init ();

  add_custom_css ();

  window = bobgui_window_new ();
  bobgui_window_set_default_size (BOBGUI_WINDOW (window), 500, 100);
  box = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 10);
  bobgui_widget_set_margin_start (box, 20);
  bobgui_widget_set_margin_end (box, 20);
  bobgui_widget_set_margin_top (box, 20);
  bobgui_widget_set_margin_bottom (box, 20);
  bar = create_level_bar ();
  bobgui_window_set_child (BOBGUI_WINDOW (window), box);
  bobgui_box_append (BOBGUI_BOX (box), bar);
  box2 = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 10);
  bobgui_box_append (BOBGUI_BOX (box), box2);
  bobgui_box_append (BOBGUI_BOX (box2), bobgui_label_new ("Discrete"));
  sw = bobgui_switch_new ();
  bobgui_box_append (BOBGUI_BOX (box2), sw);
  g_signal_connect (sw, "notify::active", G_CALLBACK (toggle), bar);

  bobgui_window_present (BOBGUI_WINDOW (window));

  g_signal_connect (window, "destroy", G_CALLBACK (quit_cb), &done);

  g_timeout_add (100, increase_level, bar);
  while (!done)
    g_main_context_iteration (NULL, TRUE);

  return 0;
}

