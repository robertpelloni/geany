/* Path/Maze
 *
 * This demo shows how to use a GskPath to create a maze and use
 * gsk_path_get_closest_point() to check the mouse stays
 * on the path.
 *
 * It also shows off the performance of GskPath (or not) as this
 * is a rather complex path.
 */

#include "config.h"
#include <glib/gi18n.h>
#include <bobgui/bobgui.h>

#include "paintable.h"

#define MAZE_GRID_SIZE 20
#define MAZE_STROKE_SIZE_ACTIVE (MAZE_GRID_SIZE - 4)
#define MAZE_STROKE_SIZE_INACTIVE (MAZE_GRID_SIZE - 12)
#define MAZE_WIDTH 31
#define MAZE_HEIGHT 21

#define BOBGUI_TYPE_MAZE (bobgui_maze_get_type ())
G_DECLARE_FINAL_TYPE (BobguiMaze, bobgui_maze, BOBGUI, MAZE, BobguiWidget)

struct _BobguiMaze
{
  BobguiWidget parent_instance;

  int width;
  int height;
  GskPath *path;
  GskPathMeasure *measure;
  GdkPaintable *background;

  gboolean active;
};

struct _BobguiMazeClass
{
  BobguiWidgetClass parent_class;
};

G_DEFINE_TYPE (BobguiMaze, bobgui_maze, BOBGUI_TYPE_WIDGET)

static void
bobgui_maze_measure (BobguiWidget      *widget,
                  BobguiOrientation  orientation,
                  int             for_size,
                  int            *minimum,
                  int            *natural,
                  int            *minimum_baseline,
                  int            *natural_baseline)
{
  BobguiMaze *self = BOBGUI_MAZE (widget);

  if (orientation == BOBGUI_ORIENTATION_HORIZONTAL)
    *minimum = *natural = self->width;
  else
    *minimum = *natural = self->height;
}

static void
bobgui_maze_snapshot (BobguiWidget   *widget,
                   GdkSnapshot *snapshot)
{
  BobguiMaze *self = BOBGUI_MAZE (widget);
  double width = bobgui_widget_get_width (widget);
  double height = bobgui_widget_get_height (widget);
  GskStroke *stroke;

  stroke = gsk_stroke_new (MAZE_STROKE_SIZE_INACTIVE);
  if (self->active)
    gsk_stroke_set_line_width (stroke, MAZE_STROKE_SIZE_ACTIVE);
  gsk_stroke_set_line_join (stroke, GSK_LINE_JOIN_ROUND);
  gsk_stroke_set_line_cap (stroke, GSK_LINE_CAP_ROUND);
  bobgui_snapshot_push_stroke (snapshot, self->path, stroke);
  gsk_stroke_free (stroke);

  if (self->background)
    {
      gdk_paintable_snapshot (self->background, snapshot, width, height);
    }
  else
    {
      bobgui_snapshot_append_linear_gradient (snapshot,
                                           &GRAPHENE_RECT_INIT (0, 0, width, height),
                                           &GRAPHENE_POINT_INIT (0, 0),
                                           &GRAPHENE_POINT_INIT (width, height),
                                           (GskColorStop[8]) {
                                             { 0.0, { 1.0, 0.0, 0.0, 1.0 } },
                                             { 0.2, { 1.0, 0.0, 0.0, 1.0 } },
                                             { 0.3, { 1.0, 1.0, 0.0, 1.0 } },
                                             { 0.4, { 0.0, 1.0, 0.0, 1.0 } },
                                             { 0.6, { 0.0, 1.0, 1.0, 1.0 } },
                                             { 0.7, { 0.0, 0.0, 1.0, 1.0 } },
                                             { 0.8, { 1.0, 0.0, 1.0, 1.0 } },
                                             { 1.0, { 1.0, 0.0, 1.0, 1.0 } }
                                           },
                                           8);
    }

  bobgui_snapshot_pop (snapshot);

}

static void
bobgui_maze_dispose (GObject *object)
{
  BobguiMaze *self = BOBGUI_MAZE (object);

  g_clear_pointer (&self->path, gsk_path_unref);
  g_clear_pointer (&self->measure, gsk_path_measure_unref);
  if (self->background)
    {
      g_signal_handlers_disconnect_matched (self->background, G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, self);
      g_clear_object (&self->background);
    }

  G_OBJECT_CLASS (bobgui_maze_parent_class)->dispose (object);
}

static void
bobgui_maze_class_init (BobguiMazeClass *klass)
{
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = bobgui_maze_dispose;

  widget_class->measure = bobgui_maze_measure;
  widget_class->snapshot = bobgui_maze_snapshot;
}

static void
celebrate (gboolean win)
{
  char *path;
  BobguiMediaStream *stream;

  if (win)
    path = g_build_filename (BOBGUI_DATADIR, "sounds", "freedesktop", "stereo", "complete.oga", NULL);
  else
    path = g_build_filename (BOBGUI_DATADIR, "sounds", "freedesktop", "stereo", "suspend-error.oga", NULL);
  stream = bobgui_media_file_new_for_filename (path);
  bobgui_media_stream_set_volume (stream, 1.0);
  bobgui_media_stream_play (stream);

  g_signal_connect (stream, "notify::ended", G_CALLBACK (g_object_unref), NULL);
  g_free (path);
}

static void
pointer_motion (BobguiEventControllerMotion *controller,
                double                    x,
                double                    y,
                BobguiMaze                  *self)
{
  GskPathPoint point;
  float distance;

  if (!self->active)
    return;

  if (gsk_path_get_closest_point (self->path,
                                  &GRAPHENE_POINT_INIT (x, y),
                                  INFINITY,
                                  &point,
                                  &distance))
    {
      if (distance < MAZE_STROKE_SIZE_ACTIVE / 2.f)
        return;
    }

  celebrate (FALSE);

  self->active = FALSE;
  bobgui_widget_queue_draw (BOBGUI_WIDGET (self));
}

static void
pointer_leave (BobguiEventControllerMotion *controller,
               BobguiMaze                  *self)
{
  if (!self->active)
    {
      self->active = TRUE;
      bobgui_widget_queue_draw (BOBGUI_WIDGET (self));
    }
}

static void
bobgui_maze_init (BobguiMaze *self)
{
  BobguiEventController *controller;

  controller = BOBGUI_EVENT_CONTROLLER (bobgui_event_controller_motion_new ());
  g_signal_connect (controller, "motion", G_CALLBACK (pointer_motion), self);
  g_signal_connect (controller, "leave", G_CALLBACK (pointer_leave), self);
  bobgui_widget_add_controller (BOBGUI_WIDGET (self), controller);

  self->active = TRUE;
}

static void
bobgui_maze_set_path (BobguiMaze *self,
                   GskPath *path)
{
  g_clear_pointer (&self->path, gsk_path_unref);
  g_clear_pointer (&self->measure, gsk_path_measure_unref);
  self->path = gsk_path_ref (path);
  self->measure = gsk_path_measure_new (path);

  bobgui_widget_queue_draw (BOBGUI_WIDGET (self));
}

BobguiWidget *
bobgui_maze_new (GskPath      *path,
              GdkPaintable *background,
              int           width,
              int           height)
{
  BobguiMaze *self;

  self = g_object_new (BOBGUI_TYPE_MAZE, NULL);

  bobgui_maze_set_path (self, path);
  gsk_path_unref (path);
  self->background = background;
  if (self->background)
    {
      g_signal_connect_swapped (self->background, "invalidate-contents", G_CALLBACK (bobgui_widget_queue_draw), self);
      g_signal_connect_swapped (self->background, "invalidate-size", G_CALLBACK (bobgui_widget_queue_resize), self);
    }
  self->width = width;
  self->height = height;

  return BOBGUI_WIDGET (self);
}

static void
add_point_to_maze (BobguiBitset      *maze,
                   GskPathBuilder *builder,
                   guint           x,
                   guint           y)
{
  gboolean set[4] = { FALSE, FALSE, FALSE, FALSE };
  guint dir;

  bobgui_bitset_add (maze, y * MAZE_WIDTH + x);

  while (TRUE)
    {
      set[0] = set[0] || x == 0 || bobgui_bitset_contains (maze, y * MAZE_WIDTH + x - 1);
      set[1] = set[1] || y == 0 || bobgui_bitset_contains (maze, (y - 1) * MAZE_WIDTH + x);
      set[2] = set[2] || x + 1 == MAZE_WIDTH || bobgui_bitset_contains (maze, y * MAZE_WIDTH + x + 1);
      set[3] = set[3] || y + 1 == MAZE_HEIGHT || bobgui_bitset_contains (maze, (y + 1) * MAZE_WIDTH + x);

      if (set[0] && set[1] && set[2] && set[3])
        return;
      
      do
        {
          dir = g_random_int_range (0, 4);
        }
      while (set[dir]);

      switch (dir)
        {
          case 0:
            gsk_path_builder_move_to (builder, (x + 0.5) * MAZE_GRID_SIZE, (y + 0.5) * MAZE_GRID_SIZE);
            gsk_path_builder_line_to (builder, (x - 0.5) * MAZE_GRID_SIZE, (y + 0.5) * MAZE_GRID_SIZE);
            add_point_to_maze (maze, builder, x - 1, y);
            break;

          case 1:
            gsk_path_builder_move_to (builder, (x + 0.5) * MAZE_GRID_SIZE, (y + 0.5) * MAZE_GRID_SIZE);
            gsk_path_builder_line_to (builder, (x + 0.5) * MAZE_GRID_SIZE, (y - 0.5) * MAZE_GRID_SIZE);
            add_point_to_maze (maze, builder, x, y - 1);
            break;

          case 2:
            gsk_path_builder_move_to (builder, (x + 0.5) * MAZE_GRID_SIZE, (y + 0.5) * MAZE_GRID_SIZE);
            gsk_path_builder_line_to (builder, (x + 1.5) * MAZE_GRID_SIZE, (y + 0.5) * MAZE_GRID_SIZE);
            add_point_to_maze (maze, builder, x + 1, y);
            break;

          case 3:
            gsk_path_builder_move_to (builder, (x + 0.5) * MAZE_GRID_SIZE, (y + 0.5) * MAZE_GRID_SIZE);
            gsk_path_builder_line_to (builder, (x + 0.5) * MAZE_GRID_SIZE, (y + 1.5) * MAZE_GRID_SIZE);
            add_point_to_maze (maze, builder, x, y + 1);
            break;

          default:
            g_assert_not_reached ();
            break;
        }
    }
}

static GskPath *
create_path_for_maze (BobguiWidget *widget)
{
  GskPathBuilder *builder;
  BobguiBitset *maze;

  builder = gsk_path_builder_new ();
  maze = bobgui_bitset_new_empty ();
  /* make sure the outer lines are unreachable:
   * Set the full range, then remove the center again. */
  bobgui_bitset_add_range (maze, 0, MAZE_WIDTH * MAZE_HEIGHT);
  bobgui_bitset_remove_rectangle (maze, MAZE_WIDTH + 1, MAZE_WIDTH - 2, MAZE_HEIGHT - 2, MAZE_WIDTH);

  /* Fill the maze */
  add_point_to_maze (maze, builder, MAZE_WIDTH / 2, MAZE_HEIGHT / 2);

  /* Add start and stop lines */
  gsk_path_builder_move_to (builder, 1.5 * MAZE_GRID_SIZE, -0.5 * MAZE_GRID_SIZE);
  gsk_path_builder_line_to (builder, 1.5 * MAZE_GRID_SIZE, 1.5 * MAZE_GRID_SIZE);
  gsk_path_builder_move_to (builder, (MAZE_WIDTH - 1.5) * MAZE_GRID_SIZE, (MAZE_HEIGHT - 1.5) * MAZE_GRID_SIZE);
  gsk_path_builder_line_to (builder, (MAZE_WIDTH - 1.5) * MAZE_GRID_SIZE, (MAZE_HEIGHT + 0.5) * MAZE_GRID_SIZE);


  bobgui_bitset_unref (maze);

  return gsk_path_builder_free_to_path (builder);
}

BobguiWidget *
do_path_maze (BobguiWidget *do_widget)
{
  static BobguiWidget *window = NULL;

  if (!window)
    {
      BobguiWidget *maze;
      BobguiMediaStream *stream;
      GskPath *path;

      window = bobgui_window_new ();
      bobgui_window_set_resizable (BOBGUI_WINDOW (window), FALSE);
      bobgui_window_set_title (BOBGUI_WINDOW (window), "Follow the maze with the mouse");
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);

#if 0
      stream = bobgui_media_file_new_for_resource ("/images/bobgui-logo.webm");
#else
      stream = bobgui_nuclear_media_stream_new ();
#endif
      bobgui_media_stream_play (stream);
      bobgui_media_stream_set_loop (stream, TRUE);

      path = create_path_for_maze (window);

      maze = bobgui_maze_new (path,
                           GDK_PAINTABLE (stream),
                           MAZE_WIDTH * MAZE_GRID_SIZE,
                           MAZE_HEIGHT * MAZE_GRID_SIZE);

      bobgui_window_set_child (BOBGUI_WINDOW (window), maze);
    }

  if (!bobgui_widget_get_visible (window))
    bobgui_window_present (BOBGUI_WINDOW (window));
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));

  return window;
}
