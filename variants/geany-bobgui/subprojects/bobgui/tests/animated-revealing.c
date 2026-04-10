/* -*- mode: C; c-basic-offset: 2; indent-tabs-mode: nil; -*- */

#include <bobgui/bobgui.h>

#include "frame-stats.h"

double reveal_time = 5;

static GOptionEntry options[] = {
  { "time", 't', 0, G_OPTION_ARG_DOUBLE, &reveal_time, "Reveal time", "SECONDS" },
  { NULL }
};

static void
toggle_reveal (BobguiRevealer *revealer)
{
  bobgui_revealer_set_reveal_child (revealer, !bobgui_revealer_get_reveal_child (revealer));
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
main(int argc, char **argv)
{
  BobguiWidget *window, *revealer, *grid, *widget;
  BobguiCssProvider *cssprovider;
  GError *error = NULL;
  guint x, y;
  gboolean done = FALSE;

  GOptionContext *context = g_option_context_new (NULL);
  g_option_context_add_main_entries (context, options, NULL);
  frame_stats_add_options (g_option_context_get_main_group (context));

  if (!g_option_context_parse (context, &argc, &argv, &error))
    {
      g_printerr ("Option parsing failed: %s\n", error->message);
      return 1;
    }

  bobgui_init ();

  window = bobgui_window_new ();
  g_signal_connect (window, "destroy", G_CALLBACK (quit_cb), &done);
  frame_stats_ensure (BOBGUI_WINDOW (window));

  revealer = bobgui_revealer_new ();
  bobgui_widget_set_valign (revealer, BOBGUI_ALIGN_START);
  bobgui_revealer_set_transition_type (BOBGUI_REVEALER (revealer), BOBGUI_REVEALER_TRANSITION_TYPE_SLIDE_DOWN);
  bobgui_revealer_set_transition_duration (BOBGUI_REVEALER (revealer), reveal_time * 1000);
  bobgui_revealer_set_reveal_child (BOBGUI_REVEALER (revealer), TRUE);
  g_signal_connect_after (revealer, "map", G_CALLBACK (toggle_reveal), NULL);
  g_signal_connect_after (revealer, "notify::child-revealed", G_CALLBACK (toggle_reveal), NULL);
  bobgui_window_set_child (BOBGUI_WINDOW (window), revealer);

  grid = bobgui_grid_new ();
  bobgui_revealer_set_child (BOBGUI_REVEALER (revealer), grid);

  cssprovider = bobgui_css_provider_new ();
  bobgui_css_provider_load_from_string (cssprovider, "* { padding: 2px; text-shadow: 5px 5px 2px grey; }");
  bobgui_style_context_add_provider_for_display (gdk_display_get_default (),
                                              BOBGUI_STYLE_PROVIDER (cssprovider),
                                              BOBGUI_STYLE_PROVIDER_PRIORITY_APPLICATION);

  for (x = 0; x < 10; x++)
    {
      for (y = 0; y < 20; y++)
        {
          widget = bobgui_label_new ("Hello World");
          bobgui_grid_attach (BOBGUI_GRID (grid), widget, x, y, 1, 1);
        }
    }

  bobgui_window_present (BOBGUI_WINDOW (window));

  while (!done)
    g_main_context_iteration (NULL, TRUE);

  return 0;
}
