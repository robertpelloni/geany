/* Sliding Puzzle
 * #Keywords: GdkPaintable, GdkGesture, BobguiShortcutController, game
 *
 * This demo demonstrates how to use gestures and paintables to create a
 * small sliding puzzle game.
 */

#include "config.h"
#include <bobgui/bobgui.h>

/* Include the header for the puzzle piece */
#include "puzzlepiece.h"
#include "paintable.h"


static BobguiWidget *window = NULL;
static BobguiWidget *frame = NULL;
static BobguiWidget *choices = NULL;
static BobguiWidget *size_spin = NULL;
static GdkPaintable *puzzle = NULL;

static gboolean solved = TRUE;
static guint width = 3;
static guint height = 3;
static guint pos_x;
static guint pos_y;

static void
ended (GObject *object)
{
  g_object_unref (object);
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

  g_signal_connect (stream, "notify::ended", G_CALLBACK (ended), NULL);
  g_free (path);
}

static gboolean
move_puzzle (BobguiWidget *grid,
             int        dx,
             int        dy)
{
  BobguiWidget *pos, *next;
  GdkPaintable *piece;
  guint next_x, next_y;

  /* We don't move anything if the puzzle is solved */
  if (solved)
    return FALSE;

  /* Return FALSE if we can't move to where the call 
   * wants us to move.
   */
  if ((dx < 0 && pos_x < -dx) ||
      dx + pos_x >= width ||
      (dy < 0 && pos_y < -dy) ||
      dy + pos_y >= height)
    return FALSE;

  /* Compute the new position */
  next_x = pos_x + dx;
  next_y = pos_y + dy;

  /* Get the current and next image */
  pos = bobgui_grid_get_child_at (BOBGUI_GRID (grid), pos_x, pos_y);
  next = bobgui_grid_get_child_at (BOBGUI_GRID (grid), next_x, next_y);

  /* Move the displayed piece. */
  piece = bobgui_picture_get_paintable (BOBGUI_PICTURE (next));
  bobgui_picture_set_paintable (BOBGUI_PICTURE (pos), piece);
  bobgui_picture_set_paintable (BOBGUI_PICTURE (next), NULL);

  /* Update the current position */
  pos_x = next_x;
  pos_y = next_y;

  /* Return TRUE because we successfully moved the piece */
  return TRUE;
}

static void
shuffle_puzzle (BobguiWidget *grid)
{
  guint i, n_steps;

  /* Do this many random moves */
  n_steps = width * height * 50;

  for (i = 0; i < n_steps; i++)
    {
      /* Get a random number for the direction to move in */
      switch (g_random_int_range (0, 4))
        {
        case 0:
          /* left */
          move_puzzle (grid, -1, 0);
          break;

        case 1:
          /* up */
          move_puzzle (grid, 0, -1);
          break;

        case 2:
          /* right */
          move_puzzle (grid, 1, 0);
          break;

        case 3:
          /* down */
          move_puzzle (grid, 0, 1);
          break;

        default:
          g_assert_not_reached ();
          continue;
        }
    }
}

static gboolean
check_solved (BobguiWidget *grid)
{
  BobguiWidget *picture;
  GdkPaintable *piece;
  guint x, y;

  /* Nothing to check if the puzzle is already solved */
  if (solved)
    return TRUE;

  /* If the empty cell isn't in the bottom right,
   * the puzzle is obviously not solved */
  if (pos_x != width - 1 ||
      pos_y != height - 1)
    return FALSE;

  /* Check that all pieces are in the right position */
  for (y = 0; y < height; y++)
    {
      for (x = 0; x < width; x++)
        {
          picture = bobgui_grid_get_child_at (BOBGUI_GRID (grid), x, y);
          piece = bobgui_picture_get_paintable (BOBGUI_PICTURE (picture));

          /* empty cell */
          if (piece == NULL)
            continue;

          if (bobgui_puzzle_piece_get_x (BOBGUI_PUZZLE_PIECE (piece)) != x ||
              bobgui_puzzle_piece_get_y (BOBGUI_PUZZLE_PIECE (piece)) != y)
            return FALSE;
        }
    }

  /* We solved the puzzle!
   */
  solved = TRUE;

  /* Fill the empty cell to show that we're done.
   */
  picture = bobgui_grid_get_child_at (BOBGUI_GRID (grid), 0, 0);
  piece = bobgui_picture_get_paintable (BOBGUI_PICTURE (picture));

  piece = bobgui_puzzle_piece_new (bobgui_puzzle_piece_get_puzzle (BOBGUI_PUZZLE_PIECE (piece)),
                                pos_x, pos_y,
                                width, height);
  picture = bobgui_grid_get_child_at (BOBGUI_GRID (grid), pos_x, pos_y);
  bobgui_picture_set_paintable (BOBGUI_PICTURE (picture), piece);

  celebrate (TRUE);

  return TRUE;
}

static gboolean
puzzle_key_pressed (BobguiWidget *grid,
                    GVariant  *args,
                    gpointer   unused)
{
  int dx, dy;

  g_variant_get (args, "(ii)", &dx, &dy);

  if (!move_puzzle (grid, dx, dy))
    {
      /* Make the error sound and then return TRUE.
       * We handled this key, even though we didn't
       * do anything to the puzzle.
       */
      bobgui_widget_error_bell (grid);
      return TRUE;
    }

  check_solved (grid);

  return TRUE;
}

static void
puzzle_button_pressed (BobguiGestureClick *gesture,
                       int              n_press,
                       double           x,
                       double           y,
                       BobguiWidget       *grid)
{
  BobguiWidget *child;
  int l, t, i;
  int pos;

  child = bobgui_widget_pick (grid, x, y, BOBGUI_PICK_DEFAULT);

  if (!child)
    {
      bobgui_widget_error_bell (grid);
      return;
    }

  bobgui_grid_query_child (BOBGUI_GRID (grid), child, &l, &t, NULL, NULL);

  if (l == pos_x && t == pos_y)
    {
      bobgui_widget_error_bell (grid);
    }
  else if (l == pos_x)
    {
      pos = pos_y;
      for (i = t; i < pos; i++)
        {
          if (!move_puzzle (grid, 0, -1))
            bobgui_widget_error_bell (grid);
        }
      for (i = pos; i < t; i++)
        {
          if (!move_puzzle (grid, 0, 1))
            bobgui_widget_error_bell (grid);
        }
    }
  else if (t == pos_y)
    {
      pos = pos_x;
      for (i = l; i < pos; i++)
        {
          if (!move_puzzle (grid, -1, 0))
            bobgui_widget_error_bell (grid);
        }
      for (i = pos; i < l; i++)
        {
          if (!move_puzzle (grid, 1, 0))
            bobgui_widget_error_bell (grid);
        }
    }
  else
    {
      bobgui_widget_error_bell (grid);
    }

  check_solved (grid);
}

static void
add_move_binding (BobguiShortcutController *controller,
                  guint                  keyval,
                  guint                  kp_keyval,
                  int                    dx,
                  int                    dy)
{
  BobguiShortcut *shortcut;

  shortcut = bobgui_shortcut_new_with_arguments (
                 bobgui_alternative_trigger_new (bobgui_keyval_trigger_new (keyval, 0),
                                              bobgui_keyval_trigger_new (kp_keyval, 0)),
                 bobgui_callback_action_new (puzzle_key_pressed, NULL, NULL),
                 "(ii)", dx, dy);
  bobgui_shortcut_controller_add_shortcut (controller, shortcut);
}

static void
start_puzzle (GdkPaintable *paintable)
{
  BobguiWidget *picture, *grid;
  BobguiEventController *controller;
  guint x, y;
  float aspect_ratio;

  /* Create a new grid */
  grid = bobgui_grid_new ();
  bobgui_widget_set_focusable (grid, TRUE);
  bobgui_aspect_frame_set_child (BOBGUI_ASPECT_FRAME (frame), grid);
  aspect_ratio = gdk_paintable_get_intrinsic_aspect_ratio (paintable);
  if (aspect_ratio == 0.0)
    aspect_ratio = 1.0;
  bobgui_aspect_frame_set_ratio (BOBGUI_ASPECT_FRAME (frame), aspect_ratio);
  bobgui_aspect_frame_set_obey_child (BOBGUI_ASPECT_FRAME (frame), FALSE);

  /* Add shortcuts so people can use the arrow
   * keys to move the puzzle
   */
  controller = bobgui_shortcut_controller_new ();
  bobgui_shortcut_controller_set_scope (BOBGUI_SHORTCUT_CONTROLLER (controller),
                                     BOBGUI_SHORTCUT_SCOPE_LOCAL);
  add_move_binding (BOBGUI_SHORTCUT_CONTROLLER (controller),
                    GDK_KEY_Left, GDK_KEY_KP_Left,
                    -1, 0);
  add_move_binding (BOBGUI_SHORTCUT_CONTROLLER (controller),
                    GDK_KEY_Right, GDK_KEY_KP_Right,
                    1, 0);
  add_move_binding (BOBGUI_SHORTCUT_CONTROLLER (controller),
                    GDK_KEY_Up, GDK_KEY_KP_Up,
                    0, -1);
  add_move_binding (BOBGUI_SHORTCUT_CONTROLLER (controller),
                    GDK_KEY_Down, GDK_KEY_KP_Down,
                    0, 1);
  bobgui_widget_add_controller (BOBGUI_WIDGET (grid), controller);

  controller = BOBGUI_EVENT_CONTROLLER (bobgui_gesture_click_new ());
  g_signal_connect (controller, "pressed",
                    G_CALLBACK (puzzle_button_pressed),
                    grid);
  bobgui_widget_add_controller (BOBGUI_WIDGET (grid), controller);

  /* Make sure the cells have equal size */
  bobgui_grid_set_row_homogeneous (BOBGUI_GRID (grid), TRUE);
  bobgui_grid_set_column_homogeneous (BOBGUI_GRID (grid), TRUE);

  /* Reset the variables */
  solved = FALSE;
  pos_x = width - 1;
  pos_y = height - 1;

  /* add a picture for every cell */
  for (y = 0; y < height; y++)
    {
      for (x = 0; x < width; x++)
        {
          GdkPaintable *piece;

          /* Don't paint anything for the lsiding part of the video */
          if (x == pos_x && y == pos_y)
            piece = NULL;
          else
            piece = bobgui_puzzle_piece_new (paintable,
                                          x, y,
                                          width, height);
          picture = bobgui_picture_new_for_paintable (piece);
          bobgui_picture_set_content_fit (BOBGUI_PICTURE (picture), BOBGUI_CONTENT_FIT_FILL);
          bobgui_grid_attach (BOBGUI_GRID (grid),
                           picture,
                           x, y,
                           1, 1);
        }
    }

  shuffle_puzzle (grid);
}

static void
reshuffle (void)
{
  BobguiWidget *grid;

  if (solved)
    {
      start_puzzle (puzzle);
      grid = bobgui_aspect_frame_get_child (BOBGUI_ASPECT_FRAME (frame));
    }
  else
    {
      grid = bobgui_aspect_frame_get_child (BOBGUI_ASPECT_FRAME (frame));
      shuffle_puzzle (grid);
    }
  bobgui_widget_grab_focus (grid);
}

static void
reconfigure (void)
{
  BobguiWidget *popover;
  BobguiWidget *grid;
  BobguiWidget *child;
  BobguiWidget *image;
  GList *selected;

  width = height = bobgui_spin_button_get_value_as_int (BOBGUI_SPIN_BUTTON (size_spin));

  selected = bobgui_flow_box_get_selected_children (BOBGUI_FLOW_BOX (choices));
  if (selected == NULL)
    child = bobgui_widget_get_first_child (choices);
  else
    {
      child = selected->data;
      g_list_free (selected);
    }

  image = bobgui_flow_box_child_get_child (BOBGUI_FLOW_BOX_CHILD (child));
  puzzle = bobgui_image_get_paintable (BOBGUI_IMAGE (image));

  start_puzzle (puzzle);
  popover = bobgui_widget_get_ancestor (size_spin, BOBGUI_TYPE_POPOVER);
  bobgui_popover_popdown (BOBGUI_POPOVER (popover));
  grid = bobgui_aspect_frame_get_child (BOBGUI_ASPECT_FRAME (frame));
  bobgui_widget_grab_focus (grid);
}

static void
add_choice (BobguiWidget    *container,
            GdkPaintable *paintable)
{
  BobguiWidget *icon;

  icon = bobgui_image_new_from_paintable (paintable);
  bobgui_image_set_icon_size (BOBGUI_IMAGE (icon), BOBGUI_ICON_SIZE_LARGE);

  bobgui_flow_box_insert (BOBGUI_FLOW_BOX (container), icon, -1);
}

static void
widget_destroyed (gpointer data,
                  GObject *widget)
{
  if (data)
    *(gpointer *) data = NULL;
}


BobguiWidget *
do_sliding_puzzle (BobguiWidget *do_widget)
{
  if (!window)
    {
      BobguiWidget *header;
      BobguiWidget *restart;
      BobguiWidget *tweak;
      BobguiWidget *popover;
      BobguiWidget *tweaks;
      BobguiWidget *apply;
      BobguiWidget *label;
      BobguiWidget *sw;
      BobguiMediaStream *media;

      puzzle = GDK_PAINTABLE (gdk_texture_new_from_resource ("/sliding_puzzle/portland-rose.jpg"));

      tweaks = bobgui_grid_new ();
      bobgui_grid_set_row_spacing (BOBGUI_GRID (tweaks), 10);
      bobgui_grid_set_column_spacing (BOBGUI_GRID (tweaks), 10);
      bobgui_widget_set_margin_start (tweaks, 10);
      bobgui_widget_set_margin_end (tweaks, 10);
      bobgui_widget_set_margin_top (tweaks, 10);
      bobgui_widget_set_margin_bottom (tweaks, 10);

      choices = bobgui_flow_box_new ();
      bobgui_widget_add_css_class (choices, "view");
      add_choice (choices, puzzle);
      add_choice (choices, bobgui_nuclear_animation_new (TRUE));
      media = bobgui_media_file_new_for_resource ("/images/bobgui-logo.webm");
      bobgui_media_stream_set_loop (media, TRUE);
      bobgui_media_stream_set_muted (media, TRUE);
      bobgui_media_stream_play (media);
      add_choice (choices, GDK_PAINTABLE (media));
      sw = bobgui_scrolled_window_new ();
      bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (sw), choices);
      bobgui_grid_attach (BOBGUI_GRID (tweaks), sw, 0, 0, 2, 1);

      label = bobgui_label_new ("Size");
      bobgui_label_set_xalign (BOBGUI_LABEL (label), 0.0);
      bobgui_grid_attach (BOBGUI_GRID (tweaks), label, 0, 1, 1, 1);
      size_spin = bobgui_spin_button_new_with_range (2, 10, 1);
      bobgui_spin_button_set_value (BOBGUI_SPIN_BUTTON (size_spin), width);
      bobgui_grid_attach (BOBGUI_GRID (tweaks), size_spin, 1, 1, 1, 1);

      apply = bobgui_button_new_with_label ("Apply");
      bobgui_widget_set_halign (apply, BOBGUI_ALIGN_END);
      bobgui_grid_attach (BOBGUI_GRID (tweaks), apply, 1, 2, 1, 1);
      g_signal_connect (apply, "clicked", G_CALLBACK (reconfigure), NULL);

      popover = bobgui_popover_new ();
      bobgui_popover_set_child (BOBGUI_POPOVER (popover), tweaks);

      tweak = bobgui_menu_button_new ();
      bobgui_menu_button_set_popover (BOBGUI_MENU_BUTTON (tweak), popover);
      bobgui_menu_button_set_icon_name (BOBGUI_MENU_BUTTON (tweak), "open-menu-symbolic");

      restart = bobgui_button_new_from_icon_name ("view-refresh-symbolic");
      g_signal_connect (restart, "clicked", G_CALLBACK (reshuffle), NULL);

      header = bobgui_header_bar_new ();
      bobgui_header_bar_pack_start (BOBGUI_HEADER_BAR (header), restart);
      bobgui_header_bar_pack_end (BOBGUI_HEADER_BAR (header), tweak);
      window = bobgui_window_new ();
      bobgui_window_set_display (BOBGUI_WINDOW (window),
                              bobgui_widget_get_display (do_widget));
      bobgui_window_set_title (BOBGUI_WINDOW (window), "Sliding Puzzle");
      bobgui_window_set_titlebar (BOBGUI_WINDOW (window), header);
      bobgui_window_set_default_size (BOBGUI_WINDOW (window), 400, 300);
      g_object_weak_ref (G_OBJECT (window), widget_destroyed, &window);

      frame = bobgui_aspect_frame_new (0.5, 0.5, (float) gdk_paintable_get_intrinsic_aspect_ratio (puzzle), FALSE);
      bobgui_window_set_child (BOBGUI_WINDOW (window), frame);

      start_puzzle (puzzle);
    }

  if (!bobgui_widget_get_visible (window))
    bobgui_widget_set_visible (window, TRUE);
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));

  return window;
}
