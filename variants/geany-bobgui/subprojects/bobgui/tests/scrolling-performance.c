/* -*- mode: C; c-basic-offset: 2; indent-tabs-mode: nil; -*- */

#include <bobgui/bobgui.h>
#include <math.h>

#include "frame-stats.h"


/* Stub definition of MyTextView which is used in the
 * widget-factory.ui file. We just need this so the
 * test keeps working
 */
typedef struct
{
  BobguiTextView tv;
} MyTextView;

typedef BobguiTextViewClass MyTextViewClass;

static GType my_text_view_get_type (void);
G_DEFINE_TYPE (MyTextView, my_text_view, BOBGUI_TYPE_TEXT_VIEW)

static void
my_text_view_init (MyTextView *tv) {}

static void
my_text_view_class_init (MyTextViewClass *tv_class) {}



static BobguiWidget *
create_widget_factory_content (void)
{
  GError *error = NULL;
  BobguiBuilder *builder;
  BobguiWidget *result;

  g_type_ensure (my_text_view_get_type ());
  builder = bobgui_builder_new ();
  bobgui_builder_add_from_file (builder, "./testsuite/bobgui/focus-chain/widget-factory.ui", &error);
  if (error != NULL)
    g_error ("Failed to create widgets: %s", error->message);

  result = BOBGUI_WIDGET (bobgui_builder_get_object (builder, "box1"));
  g_object_ref (result);
  bobgui_window_set_child (BOBGUI_WINDOW (bobgui_widget_get_parent (result)), NULL);
  g_object_unref (builder);

  return result;
}

static void
set_adjustment_to_fraction (BobguiAdjustment *adjustment,
                            double         fraction)
{
  double upper = bobgui_adjustment_get_upper (adjustment);
  double lower = bobgui_adjustment_get_lower (adjustment);
  double page_size = bobgui_adjustment_get_page_size (adjustment);

  bobgui_adjustment_set_value (adjustment,
                            (1 - fraction) * lower +
                            fraction * (upper - page_size));
}

static gboolean
scroll_viewport (BobguiWidget     *viewport,
                 GdkFrameClock *frame_clock,
                 gpointer       user_data)
{
  static gint64 start_time;
  gint64 now = gdk_frame_clock_get_frame_time (frame_clock);
  double elapsed;
  BobguiAdjustment *hadjustment, *vadjustment;

  if (start_time == 0)
    start_time = now;

  elapsed = (now - start_time) / 1000000.;

  hadjustment = bobgui_scrollable_get_hadjustment (BOBGUI_SCROLLABLE (viewport));
  vadjustment = bobgui_scrollable_get_vadjustment (BOBGUI_SCROLLABLE (viewport));

  set_adjustment_to_fraction (hadjustment, 0.5 + 0.5 * sin (elapsed));
  set_adjustment_to_fraction (vadjustment, 0.5 + 0.5 * cos (elapsed));

  return TRUE;
}

static GOptionEntry options[] = {
  { NULL }
};

static void
quit_cb (BobguiWidget *widget,
         gpointer   data)
{
  gboolean *done = data;

  *done = TRUE;

  g_main_context_wakeup (NULL);
}

int
main (int argc, char **argv)
{
  BobguiWidget *window;
  BobguiWidget *scrolled_window;
  BobguiWidget *viewport;
  BobguiWidget *grid;
  GError *error = NULL;
  int i;
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
  frame_stats_ensure (BOBGUI_WINDOW (window));
  bobgui_window_set_default_size (BOBGUI_WINDOW (window), 800, 600);

  scrolled_window = bobgui_scrolled_window_new ();
  bobgui_window_set_child (BOBGUI_WINDOW (window), scrolled_window);

  viewport = bobgui_viewport_new (NULL, NULL);
  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (scrolled_window), viewport);

  grid = bobgui_grid_new ();
  bobgui_viewport_set_child (BOBGUI_VIEWPORT (viewport), grid);

  for (i = 0; i < 4; i++)
    {
      BobguiWidget *content = create_widget_factory_content ();
      bobgui_grid_attach (BOBGUI_GRID (grid), content,
                       i % 2, i / 2, 1, 1);
      g_object_unref (content);
    }

  bobgui_widget_add_tick_callback (viewport,
                                scroll_viewport,
                                NULL,
                                NULL);

  bobgui_window_present (BOBGUI_WINDOW (window));
  g_signal_connect (window, "destroy",
                    G_CALLBACK (quit_cb), &done);

  while (!done)
    g_main_context_iteration (NULL, TRUE);

  return 0;
}
