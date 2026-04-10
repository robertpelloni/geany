/* BOBGUI - The Bobgui Framework
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Modified by the BOBGUI Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the BOBGUI Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BOBGUI at ftp://ftp.bobgui.org/pub/bobgui/.
 */

#include "config.h"

#include "bobguiprogressbar.h"

#include "bobguiaccessiblerange.h"
#include "bobguiboxlayout.h"
#include "bobguicssboxesprivate.h"
#include "bobguigizmoprivate.h"
#include <glib/gi18n-lib.h>
#include "bobguilabel.h"
#include "bobguiorientable.h"
#include "bobguiprogresstrackerprivate.h"
#include "bobguiprivate.h"
#include "bobguisnapshot.h"
#include "bobguiwidgetprivate.h"

#include <string.h>

/**
 * BobguiProgressBar:
 *
 * Displays the progress of a long-running operation.
 *
 * `BobguiProgressBar` provides a visual clue that processing is underway.
 * It can be used in two different modes: percentage mode and activity mode.
 *
 * <picture>
 *   <source srcset="progressbar-dark.png" media="(prefers-color-scheme: dark)">
 *   <img alt="An example BobguiProgressBar" src="progressbar.png">
 * </picture>
 *
 * When an application can determine how much work needs to take place
 * (e.g. read a fixed number of bytes from a file) and can monitor its
 * progress, it can use the `BobguiProgressBar` in percentage mode and the
 * user sees a growing bar indicating the percentage of the work that
 * has been completed. In this mode, the application is required to call
 * [method@Bobgui.ProgressBar.set_fraction] periodically to update the progress bar.
 *
 * When an application has no accurate way of knowing the amount of work
 * to do, it can use the `BobguiProgressBar` in activity mode, which shows
 * activity by a block moving back and forth within the progress area. In
 * this mode, the application is required to call [method@Bobgui.ProgressBar.pulse]
 * periodically to update the progress bar.
 *
 * There is quite a bit of flexibility provided to control the appearance
 * of the `BobguiProgressBar`. Functions are provided to control the orientation
 * of the bar, optional text can be displayed along with the bar, and the
 * step size used in activity mode can be set.
 *
 * # CSS nodes
 *
 * ```
 * progressbar[.osd]
 * ├── [text]
 * ╰── trough[.empty][.full]
 *     ╰── progress[.pulse]
 * ```
 *
 * `BobguiProgressBar` has a main CSS node with name progressbar and subnodes with
 * names text and trough, of which the latter has a subnode named progress. The
 * text subnode is only present if text is shown. The progress subnode has the
 * style class .pulse when in activity mode. It gets the style classes .left,
 * .right, .top or .bottom added when the progress 'touches' the corresponding
 * end of the BobguiProgressBar. The .osd class on the progressbar node is for use
 * in overlays like the one Epiphany has for page loading progress.
 *
 * # Accessibility
 *
 * `BobguiProgressBar` uses the [enum@Bobgui.AccessibleRole.progress_bar] role
 * and sets the [enum@Bobgui.AccessibleProperty.value_min], [enum@Bobgui.AccessibleProperty.value_max] and [enum@Bobgui.AccessibleProperty.value_now] properties to reflect
 * the progress.
 */

typedef struct _BobguiProgressBarClass         BobguiProgressBarClass;

struct _BobguiProgressBar
{
  BobguiWidget parent_instance;

  char          *text;

  BobguiWidget     *label;
  BobguiWidget     *trough_widget;
  BobguiWidget     *progress_widget;

  double         fraction;
  double         pulse_fraction;

  double         activity_pos;
  guint          activity_blocks;

  BobguiOrientation orientation;

  guint              tick_id;
  BobguiProgressTracker tracker;
  gint64             pulse1;
  gint64             pulse2;
  double             last_iteration;

  guint          activity_dir  : 1;
  guint          activity_mode : 1;
  guint          ellipsize     : 3;
  guint          show_text     : 1;
  guint          inverted      : 1;
};

struct _BobguiProgressBarClass
{
  BobguiWidgetClass parent_class;
};

enum {
  PROP_0,
  PROP_FRACTION,
  PROP_PULSE_STEP,
  PROP_INVERTED,
  PROP_TEXT,
  PROP_SHOW_TEXT,
  PROP_ELLIPSIZE,
  PROP_ORIENTATION,
  NUM_PROPERTIES = PROP_ORIENTATION
};

static GParamSpec *progress_props[NUM_PROPERTIES] = { NULL, };

static void bobgui_progress_bar_set_property         (GObject        *object,
                                                   guint           prop_id,
                                                   const GValue   *value,
                                                   GParamSpec     *pspec);
static void bobgui_progress_bar_get_property         (GObject        *object,
                                                   guint           prop_id,
                                                   GValue         *value,
                                                   GParamSpec     *pspec);
static void     bobgui_progress_bar_act_mode_enter   (BobguiProgressBar *progress);
static void     bobgui_progress_bar_act_mode_leave   (BobguiProgressBar *progress);
static void     bobgui_progress_bar_finalize         (GObject        *object);
static void     bobgui_progress_bar_dispose          (GObject        *object);
static void     bobgui_progress_bar_set_orientation  (BobguiProgressBar *progress,
                                                   BobguiOrientation  orientation);
static void     bobgui_progress_bar_direction_changed (BobguiWidget        *widget,
                                                    BobguiTextDirection  previous_dir);

G_DEFINE_TYPE_WITH_CODE (BobguiProgressBar, bobgui_progress_bar, BOBGUI_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_ORIENTABLE, NULL)
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_ACCESSIBLE_RANGE, NULL))

static void
bobgui_progress_bar_class_init (BobguiProgressBarClass *class)
{
  GObjectClass *gobject_class;
  BobguiWidgetClass *widget_class;

  gobject_class = G_OBJECT_CLASS (class);
  widget_class = (BobguiWidgetClass *) class;

  gobject_class->set_property = bobgui_progress_bar_set_property;
  gobject_class->get_property = bobgui_progress_bar_get_property;
  gobject_class->dispose = bobgui_progress_bar_dispose;
  gobject_class->finalize = bobgui_progress_bar_finalize;

  widget_class->direction_changed = bobgui_progress_bar_direction_changed;

  g_object_class_override_property (gobject_class, PROP_ORIENTATION, "orientation");

  /**
   * BobguiProgressBar:inverted:
   *
   * Invert the direction in which the progress bar grows.
   */
  progress_props[PROP_INVERTED] =
      g_param_spec_boolean ("inverted", NULL, NULL,
                            FALSE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiProgressBar:fraction:
   *
   * The fraction of total work that has been completed.
   */
  progress_props[PROP_FRACTION] =
      g_param_spec_double ("fraction", NULL, NULL,
                           0.0, 1.0,
                           0.0,
                           BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiProgressBar:pulse-step:
   *
   * The fraction of total progress to move the bounding block when pulsed.
   */
  progress_props[PROP_PULSE_STEP] =
      g_param_spec_double ("pulse-step", NULL, NULL,
                           0.0, 1.0,
                           0.1,
                           BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiProgressBar:text:
   *
   * Text to be displayed in the progress bar.
   */
  progress_props[PROP_TEXT] =
      g_param_spec_string ("text", NULL, NULL,
                           NULL,
                           BOBGUI_PARAM_READWRITE);

  /**
   * BobguiProgressBar:show-text:
   *
   * Sets whether the progress bar will show a text in addition
   * to the bar itself.
   *
   * The shown text is either the value of the [property@Bobgui.ProgressBar:text]
   * property or, if that is %NULL, the [property@Bobgui.ProgressBar:fraction]
   * value, as a percentage.
   *
   * To make a progress bar that is styled and sized suitably for showing text
   * (even if the actual text is blank), set [property@Bobgui.ProgressBar:show-text]
   * to %TRUE and [property@Bobgui.ProgressBar:text] to the empty string (not %NULL).
   */
  progress_props[PROP_SHOW_TEXT] =
      g_param_spec_boolean ("show-text", NULL, NULL,
                            FALSE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiProgressBar:ellipsize:
   *
   * The preferred place to ellipsize the string.
   *
   * The text will be ellipsized if the progress bar does not have enough room
   * to display the entire string, specified as a `PangoEllipsizeMode`.
   *
   * Note that setting this property to a value other than
   * %PANGO_ELLIPSIZE_NONE has the side-effect that the progress bar requests
   * only enough space to display the ellipsis ("..."). Another means to set a
   * progress bar's width is [method@Bobgui.Widget.set_size_request].
   */
  progress_props[PROP_ELLIPSIZE] =
      g_param_spec_enum ("ellipsize", NULL, NULL,
                         PANGO_TYPE_ELLIPSIZE_MODE,
                         PANGO_ELLIPSIZE_NONE,
                         BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (gobject_class, NUM_PROPERTIES, progress_props);

  bobgui_widget_class_set_css_name (widget_class, I_("progressbar"));
  bobgui_widget_class_set_layout_manager_type (widget_class, BOBGUI_TYPE_BOX_LAYOUT);
  bobgui_widget_class_set_accessible_role (widget_class, BOBGUI_ACCESSIBLE_ROLE_PROGRESS_BAR);
}

static void
update_fraction_classes (BobguiProgressBar *pbar)
{
  gboolean empty = FALSE;
  gboolean full = FALSE;

  /* Here we set classes based on fill-level unless we're in activity-mode.
   */

  if (!pbar->activity_mode)
    {
      if (pbar->fraction <= 0.0)
        empty = TRUE;
      else if (pbar->fraction >= 1.0)
        full = TRUE;
    }

  if (empty)
    bobgui_widget_add_css_class (pbar->trough_widget, "empty");
  else
    bobgui_widget_remove_css_class (pbar->trough_widget, "empty");

  if (full)
    bobgui_widget_add_css_class (pbar->trough_widget, "full");
  else
    bobgui_widget_remove_css_class (pbar->trough_widget, "full");
}

static void
update_node_classes (BobguiProgressBar *pbar)
{
  gboolean left = FALSE;
  gboolean right = FALSE;
  gboolean top = FALSE;
  gboolean bottom = FALSE;

  /* Here we set positional classes, depending on which end of the
   * progressbar the progress touches.
   */

  if (pbar->activity_mode)
    {
      if (pbar->orientation == BOBGUI_ORIENTATION_HORIZONTAL)
        {
          left = pbar->activity_pos <= 0.0;
          right = pbar->activity_pos >= 1.0;
        }
      else
        {
          top = pbar->activity_pos <= 0.0;
          bottom = pbar->activity_pos >= 1.0;
        }
    }
  else /* continuous */
    {
      gboolean inverted;

      inverted = pbar->inverted;
      if (bobgui_widget_get_direction (BOBGUI_WIDGET (pbar)) == BOBGUI_TEXT_DIR_RTL)
        {
          if (pbar->orientation == BOBGUI_ORIENTATION_HORIZONTAL)
            inverted = !inverted;
        }

      if (pbar->orientation == BOBGUI_ORIENTATION_HORIZONTAL)
        {
          left = !inverted || pbar->fraction >= 1.0;
          right = inverted || pbar->fraction >= 1.0;
        }
      else
        {
          top = !inverted || pbar->fraction >= 1.0;
          bottom = inverted || pbar->fraction >= 1.0;
        }
    }

  if (left)
    bobgui_widget_add_css_class (pbar->progress_widget, "left");
  else
    bobgui_widget_remove_css_class (pbar->progress_widget, "left");

  if (right)
    bobgui_widget_add_css_class (pbar->progress_widget, "right");
  else
    bobgui_widget_remove_css_class (pbar->progress_widget, "right");

  if (top)
    bobgui_widget_add_css_class (pbar->progress_widget, "top");
  else
    bobgui_widget_remove_css_class (pbar->progress_widget, "top");

  if (bottom)
    bobgui_widget_add_css_class (pbar->progress_widget, "bottom");
  else
    bobgui_widget_remove_css_class (pbar->progress_widget, "bottom");

  update_fraction_classes (pbar);
}

static void
allocate_trough (BobguiGizmo *gizmo,
                 int       width,
                 int       height,
                 int       baseline)

{
  BobguiProgressBar *pbar = BOBGUI_PROGRESS_BAR (bobgui_widget_get_parent (BOBGUI_WIDGET (gizmo)));
  BobguiAllocation alloc;
  int progress_width, progress_height;
  gboolean inverted;

  inverted = pbar->inverted;
  if (bobgui_widget_get_direction (BOBGUI_WIDGET (pbar)) == BOBGUI_TEXT_DIR_RTL)
    {
      if (pbar->orientation == BOBGUI_ORIENTATION_HORIZONTAL)
        inverted = !inverted;
    }

  bobgui_widget_measure (pbar->progress_widget, BOBGUI_ORIENTATION_VERTICAL, -1,
                      &progress_height, NULL,
                      NULL, NULL);

  bobgui_widget_measure (pbar->progress_widget, BOBGUI_ORIENTATION_HORIZONTAL, -1,
                      &progress_width, NULL,
                      NULL, NULL);

  if (pbar->activity_mode)
    {
      if (pbar->orientation == BOBGUI_ORIENTATION_HORIZONTAL)
        {
          alloc.width = progress_width + (width - progress_width) / pbar->activity_blocks;
          alloc.x = pbar->activity_pos * (width - alloc.width);
          alloc.y = (height - progress_height) / 2;
          alloc.height = progress_height;
        }
      else
        {

          alloc.height = progress_height + (height - progress_height) / pbar->activity_blocks;
          alloc.y = pbar->activity_pos * (height - alloc.height);
          alloc.x = (width - progress_width) / 2;
          alloc.width = progress_width;
        }
    }
  else
    {
      if (pbar->orientation == BOBGUI_ORIENTATION_HORIZONTAL)
        {
          alloc.width = progress_width + (width - progress_width) * pbar->fraction;
          alloc.height = progress_height;
          alloc.y = (height - progress_height) / 2;

          if (!inverted)
            alloc.x = 0;
          else
            alloc.x = width - alloc.width;
        }
      else
        {
          alloc.width = progress_width;
          alloc.height = progress_height + (height - progress_height) * pbar->fraction;
          alloc.x = (width - progress_width) / 2;

          if (!inverted)
            alloc.y = 0;
          else
            alloc.y = height - alloc.height;
        }
    }

  bobgui_widget_size_allocate (pbar->progress_widget, &alloc, -1);
}

static void
snapshot_trough (BobguiGizmo    *gizmo,
                 BobguiSnapshot *snapshot)

{
  BobguiWidget *widget = bobgui_widget_get_parent (BOBGUI_WIDGET (gizmo));
  BobguiProgressBar *pbar = BOBGUI_PROGRESS_BAR (widget);

  if (pbar->progress_widget)
    {
      BobguiCssBoxes boxes;
      bobgui_css_boxes_init (&boxes, BOBGUI_WIDGET (gizmo));
      bobgui_snapshot_push_rounded_clip (snapshot, bobgui_css_boxes_get_border_box (&boxes));
      bobgui_widget_snapshot_child (BOBGUI_WIDGET (gizmo), pbar->progress_widget, snapshot);
      bobgui_snapshot_pop (snapshot);
    }
}

static void
bobgui_progress_bar_init (BobguiProgressBar *pbar)
{
  pbar->inverted = FALSE;
  pbar->pulse_fraction = 0.1;
  pbar->activity_pos = 0;
  pbar->activity_dir = 1;
  pbar->activity_blocks = 5;
  pbar->ellipsize = PANGO_ELLIPSIZE_NONE;
  pbar->show_text = FALSE;

  pbar->text = NULL;
  pbar->fraction = 0.0;

  pbar->trough_widget = bobgui_gizmo_new_with_role ("trough",
                                                 BOBGUI_ACCESSIBLE_ROLE_NONE,
                                                 NULL,
                                                 allocate_trough,
                                                 snapshot_trough,
                                                 NULL,
                                                 NULL, NULL);
  bobgui_widget_set_parent (pbar->trough_widget, BOBGUI_WIDGET (pbar));

  pbar->progress_widget = bobgui_gizmo_new_with_role ("progress",
                                                   BOBGUI_ACCESSIBLE_ROLE_NONE,
                                                   NULL, NULL, NULL, NULL, NULL, NULL);
  bobgui_widget_set_parent (pbar->progress_widget, pbar->trough_widget);

  /* horizontal is default */
  pbar->orientation = BOBGUI_ORIENTATION_VERTICAL; /* Just to force an update... */
  bobgui_progress_bar_set_orientation (pbar, BOBGUI_ORIENTATION_HORIZONTAL);
  bobgui_widget_update_orientation (BOBGUI_WIDGET (pbar), pbar->orientation);

  bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (pbar),
                                  BOBGUI_ACCESSIBLE_PROPERTY_VALUE_MAX, 1.0,
                                  BOBGUI_ACCESSIBLE_PROPERTY_VALUE_MIN, 0.0,
                                  BOBGUI_ACCESSIBLE_PROPERTY_VALUE_NOW, 0.0,
                                  -1);
}

static void
bobgui_progress_bar_set_property (GObject      *object,
                               guint         prop_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  BobguiProgressBar *pbar;

  pbar = BOBGUI_PROGRESS_BAR (object);

  switch (prop_id)
    {
    case PROP_ORIENTATION:
      bobgui_progress_bar_set_orientation (pbar, g_value_get_enum (value));
      break;
    case PROP_INVERTED:
      bobgui_progress_bar_set_inverted (pbar, g_value_get_boolean (value));
      break;
    case PROP_FRACTION:
      bobgui_progress_bar_set_fraction (pbar, g_value_get_double (value));
      break;
    case PROP_PULSE_STEP:
      bobgui_progress_bar_set_pulse_step (pbar, g_value_get_double (value));
      break;
    case PROP_TEXT:
      bobgui_progress_bar_set_text (pbar, g_value_get_string (value));
      break;
    case PROP_SHOW_TEXT:
      bobgui_progress_bar_set_show_text (pbar, g_value_get_boolean (value));
      break;
    case PROP_ELLIPSIZE:
      bobgui_progress_bar_set_ellipsize (pbar, g_value_get_enum (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_progress_bar_get_property (GObject      *object,
                               guint         prop_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  BobguiProgressBar *pbar = BOBGUI_PROGRESS_BAR (object);

  switch (prop_id)
    {
    case PROP_ORIENTATION:
      g_value_set_enum (value, pbar->orientation);
      break;
    case PROP_INVERTED:
      g_value_set_boolean (value, pbar->inverted);
      break;
    case PROP_FRACTION:
      g_value_set_double (value, pbar->fraction);
      break;
    case PROP_PULSE_STEP:
      g_value_set_double (value, pbar->pulse_fraction);
      break;
    case PROP_TEXT:
      g_value_set_string (value, pbar->text);
      break;
    case PROP_SHOW_TEXT:
      g_value_set_boolean (value, pbar->show_text);
      break;
    case PROP_ELLIPSIZE:
      g_value_set_enum (value, pbar->ellipsize);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

/**
 * bobgui_progress_bar_new:
 *
 * Creates a new `BobguiProgressBar`.
 *
 * Returns: a `BobguiProgressBar`.
 */
BobguiWidget*
bobgui_progress_bar_new (void)
{
  BobguiWidget *pbar;

  pbar = g_object_new (BOBGUI_TYPE_PROGRESS_BAR, NULL);

  return pbar;
}

static void
bobgui_progress_bar_dispose (GObject *object)
{
  BobguiProgressBar *pbar = BOBGUI_PROGRESS_BAR (object);

  if (pbar->activity_mode)
    bobgui_progress_bar_act_mode_leave (pbar);

  g_clear_pointer (&pbar->label, bobgui_widget_unparent);
  g_clear_pointer (&pbar->progress_widget, bobgui_widget_unparent);
  g_clear_pointer (&pbar->trough_widget, bobgui_widget_unparent);

  G_OBJECT_CLASS (bobgui_progress_bar_parent_class)->dispose (object);
}

static void
bobgui_progress_bar_finalize (GObject *object)
{
  BobguiProgressBar *pbar = BOBGUI_PROGRESS_BAR (object);

  g_free (pbar->text);

  G_OBJECT_CLASS (bobgui_progress_bar_parent_class)->finalize (object);
}

static char *
get_current_text (BobguiProgressBar *pbar)
{
  if (pbar->text)
    return g_strdup (pbar->text);
  else
    return g_strdup_printf (C_("progress bar label", "%.0f %%"), pbar->fraction * 100.0);
}

static gboolean
tick_cb (BobguiWidget     *widget,
         GdkFrameClock *frame_clock,
         gpointer       user_data)
{
  BobguiProgressBar *pbar = BOBGUI_PROGRESS_BAR (widget);
  gint64 frame_time;
  double iteration, pulse_iterations, current_iterations, fraction;

  if (pbar->pulse2 == 0 && pbar->pulse1 == 0)
    return G_SOURCE_CONTINUE;

  frame_time = gdk_frame_clock_get_frame_time (frame_clock);
  bobgui_progress_tracker_advance_frame (&pbar->tracker, frame_time);

  g_assert (pbar->pulse2 > pbar->pulse1);

  pulse_iterations = (pbar->pulse2 - pbar->pulse1) / (double) G_USEC_PER_SEC;
  current_iterations = (frame_time - pbar->pulse1) / (double) G_USEC_PER_SEC;

  iteration = bobgui_progress_tracker_get_iteration (&pbar->tracker);
  /* Determine the fraction to move the block from one frame
   * to the next when pulse_fraction is how far the block should
   * move between two calls to bobgui_progress_bar_pulse().
   */
  fraction = pbar->pulse_fraction * (iteration - pbar->last_iteration) / MAX (pulse_iterations, current_iterations);
  pbar->last_iteration = iteration;

  if (current_iterations > 3 * pulse_iterations)
    return G_SOURCE_CONTINUE;

  /* advance the block */
  if (pbar->activity_dir == 0)
    {
      pbar->activity_pos += fraction;
      if (pbar->activity_pos > 1.0)
        {
          pbar->activity_pos = 1.0;
          pbar->activity_dir = 1;
        }
    }
  else
    {
      pbar->activity_pos -= fraction;
      if (pbar->activity_pos <= 0)
        {
          pbar->activity_pos = 0;
          pbar->activity_dir = 0;
        }
    }

  update_node_classes (pbar);

  bobgui_widget_queue_allocate (pbar->trough_widget);

  return G_SOURCE_CONTINUE;
}

static void
bobgui_progress_bar_act_mode_enter (BobguiProgressBar *pbar)
{
  BobguiWidget *widget = BOBGUI_WIDGET (pbar);
  gboolean inverted;

  bobgui_widget_add_css_class (pbar->progress_widget, "pulse");
  bobgui_accessible_update_state (BOBGUI_ACCESSIBLE (pbar),
                               BOBGUI_ACCESSIBLE_STATE_BUSY, TRUE,
                               -1);

  inverted = pbar->inverted;
  if (bobgui_widget_get_direction (widget) == BOBGUI_TEXT_DIR_RTL)
    {
      if (pbar->orientation == BOBGUI_ORIENTATION_HORIZONTAL)
        inverted = !inverted;
    }

  /* calculate start pos */
  if (!inverted)
    {
      pbar->activity_pos = 0.0;
      pbar->activity_dir = 0;
    }
  else
    {
      pbar->activity_pos = 1.0;
      pbar->activity_dir = 1;
    }

  update_node_classes (pbar);
  /* No fixed schedule for pulses, will adapt after calls to update_pulse. Just
   * start the tracker to repeat forever with iterations every second.*/
  bobgui_progress_tracker_start (&pbar->tracker, G_USEC_PER_SEC, 0, INFINITY);
  pbar->tick_id = bobgui_widget_add_tick_callback (widget, tick_cb, NULL, NULL);
  pbar->pulse2 = 0;
  pbar->pulse1 = 0;
  pbar->last_iteration = 0;
}

static void
bobgui_progress_bar_act_mode_leave (BobguiProgressBar *pbar)
{
  if (pbar->tick_id)
    bobgui_widget_remove_tick_callback (BOBGUI_WIDGET (pbar), pbar->tick_id);
  pbar->tick_id = 0;

  bobgui_accessible_update_state (BOBGUI_ACCESSIBLE (pbar),
                               BOBGUI_ACCESSIBLE_STATE_BUSY, FALSE,
                               -1);
  bobgui_widget_remove_css_class (pbar->progress_widget, "pulse");
  update_node_classes (pbar);
}

static void
bobgui_progress_bar_set_activity_mode (BobguiProgressBar *pbar,
                                    gboolean        activity_mode)
{
  if (pbar->activity_mode == activity_mode)
    return;

  pbar->activity_mode = activity_mode;

  if (pbar->activity_mode)
    bobgui_progress_bar_act_mode_enter (pbar);
  else
    bobgui_progress_bar_act_mode_leave (pbar);

  bobgui_widget_queue_resize (BOBGUI_WIDGET (pbar));
}

/**
 * bobgui_progress_bar_set_fraction:
 * @pbar: a `BobguiProgressBar`
 * @fraction: fraction of the task that’s been completed
 *
 * Causes the progress bar to “fill in” the given fraction
 * of the bar.
 *
 * The fraction should be between 0.0 and 1.0, inclusive.
 */
void
bobgui_progress_bar_set_fraction (BobguiProgressBar *pbar,
                               double          fraction)
{
  char *text = NULL;

  g_return_if_fail (BOBGUI_IS_PROGRESS_BAR (pbar));

  pbar->fraction = CLAMP (fraction, 0.0, 1.0);

  if (pbar->label)
    {
      text = get_current_text (pbar);
      bobgui_label_set_label (BOBGUI_LABEL (pbar->label), text);
    }

  bobgui_progress_bar_set_activity_mode (pbar, FALSE);
  bobgui_widget_queue_allocate (pbar->trough_widget);
  update_fraction_classes (pbar);

  bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (pbar),
                                  BOBGUI_ACCESSIBLE_PROPERTY_VALUE_MAX, 1.0,
                                  BOBGUI_ACCESSIBLE_PROPERTY_VALUE_MIN, 0.0,
                                  BOBGUI_ACCESSIBLE_PROPERTY_VALUE_NOW, fraction,
                                  -1);

  if (text != NULL)
    {
      bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (pbar),
                                      BOBGUI_ACCESSIBLE_PROPERTY_VALUE_TEXT, text,
                                      -1);
    }
  else
    {
      bobgui_accessible_reset_property (BOBGUI_ACCESSIBLE (pbar), BOBGUI_ACCESSIBLE_PROPERTY_VALUE_TEXT);
    }

  g_free (text);

  g_object_notify_by_pspec (G_OBJECT (pbar), progress_props[PROP_FRACTION]);
}

static void
bobgui_progress_bar_update_pulse (BobguiProgressBar *pbar)
{
  gint64 pulse_time = g_get_monotonic_time ();

  if (pbar->pulse2 == pulse_time)
    return;

  pbar->pulse1 = pbar->pulse2;
  pbar->pulse2 = pulse_time;
}

/**
 * bobgui_progress_bar_pulse:
 * @pbar: a `BobguiProgressBar`
 *
 * Indicates that some progress has been made, but you don’t know how much.
 *
 * Causes the progress bar to enter “activity mode,” where a block
 * bounces back and forth. Each call to [method@Bobgui.ProgressBar.pulse]
 * causes the block to move by a little bit (the amount of movement
 * per pulse is determined by [method@Bobgui.ProgressBar.set_pulse_step]).
 */
void
bobgui_progress_bar_pulse (BobguiProgressBar *pbar)
{
  g_return_if_fail (BOBGUI_IS_PROGRESS_BAR (pbar));

  bobgui_progress_bar_set_activity_mode (pbar, TRUE);
  bobgui_progress_bar_update_pulse (pbar);
}

/**
 * bobgui_progress_bar_set_text:
 * @pbar: a `BobguiProgressBar`
 * @text: (nullable): a UTF-8 string
 *
 * Causes the given @text to appear next to the progress bar.
 *
 * If @text is %NULL and [property@Bobgui.ProgressBar:show-text] is %TRUE,
 * the current value of [property@Bobgui.ProgressBar:fraction] will be displayed
 * as a percentage.
 *
 * If @text is non-%NULL and [property@Bobgui.ProgressBar:show-text] is %TRUE,
 * the text will be displayed. In this case, it will not display the progress
 * percentage. If @text is the empty string, the progress bar will still
 * be styled and sized suitably for containing text, as long as
 * [property@Bobgui.ProgressBar:show-text] is %TRUE.
 */
void
bobgui_progress_bar_set_text (BobguiProgressBar *pbar,
                           const char     *text)
{
  g_return_if_fail (BOBGUI_IS_PROGRESS_BAR (pbar));

  /* Don't notify again if nothing's changed. */
  if (g_strcmp0 (pbar->text, text) == 0)
    return;

  g_free (pbar->text);
  pbar->text = g_strdup (text);

  if (pbar->label)
    bobgui_label_set_label (BOBGUI_LABEL (pbar->label), text);

  g_object_notify_by_pspec (G_OBJECT (pbar), progress_props[PROP_TEXT]);
}

/**
 * bobgui_progress_bar_set_show_text:
 * @pbar: a `BobguiProgressBar`
 * @show_text: whether to show text
 *
 * Sets whether the progress bar will show text next to the bar.
 *
 * The shown text is either the value of the [property@Bobgui.ProgressBar:text]
 * property or, if that is %NULL, the [property@Bobgui.ProgressBar:fraction] value,
 * as a percentage.
 *
 * To make a progress bar that is styled and sized suitably for containing
 * text (even if the actual text is blank), set [property@Bobgui.ProgressBar:show-text]
 * to %TRUE and [property@Bobgui.ProgressBar:text] to the empty string (not %NULL).
 */
void
bobgui_progress_bar_set_show_text (BobguiProgressBar *pbar,
                                gboolean        show_text)
{
  g_return_if_fail (BOBGUI_IS_PROGRESS_BAR (pbar));

  show_text = !!show_text;

  if (pbar->show_text == show_text)
    return;

  pbar->show_text = show_text;

  if (show_text)
    {
      char *text = get_current_text (pbar);

      pbar->label = g_object_new (BOBGUI_TYPE_LABEL,
                                  "accessible-role", BOBGUI_ACCESSIBLE_ROLE_NONE,
                                  "css-name", "text",
                                  "label", text,
                                  "ellipsize", pbar->ellipsize,
                                  NULL);
      bobgui_widget_insert_after (pbar->label, BOBGUI_WIDGET (pbar), NULL);

      g_free (text);
    }
  else
    {
      g_clear_pointer (&pbar->label, bobgui_widget_unparent);
    }

  g_object_notify_by_pspec (G_OBJECT (pbar), progress_props[PROP_SHOW_TEXT]);
}

/**
 * bobgui_progress_bar_get_show_text:
 * @pbar: a `BobguiProgressBar`
 *
 * Returns whether the `BobguiProgressBar` shows text.
 *
 * See [method@Bobgui.ProgressBar.set_show_text].
 *
 * Returns: %TRUE if text is shown in the progress bar
 */
gboolean
bobgui_progress_bar_get_show_text (BobguiProgressBar *pbar)
{
  g_return_val_if_fail (BOBGUI_IS_PROGRESS_BAR (pbar), FALSE);

  return pbar->show_text;
}

/**
 * bobgui_progress_bar_set_pulse_step:
 * @pbar: a `BobguiProgressBar`
 * @fraction: fraction between 0.0 and 1.0
 *
 * Sets the fraction of total progress bar length to move the
 * bouncing block.
 *
 * The bouncing block is moved when [method@Bobgui.ProgressBar.pulse]
 * is called.
 */
void
bobgui_progress_bar_set_pulse_step (BobguiProgressBar *pbar,
                                 double          fraction)
{
  g_return_if_fail (BOBGUI_IS_PROGRESS_BAR (pbar));

  pbar->pulse_fraction = fraction;

  g_object_notify_by_pspec (G_OBJECT (pbar), progress_props[PROP_PULSE_STEP]);
}

static void
bobgui_progress_bar_direction_changed (BobguiWidget        *widget,
                                    BobguiTextDirection  previous_dir)
{
  BobguiProgressBar *pbar = BOBGUI_PROGRESS_BAR (widget);

  update_node_classes (pbar);

  BOBGUI_WIDGET_CLASS (bobgui_progress_bar_parent_class)->direction_changed (widget, previous_dir);
}

static void
bobgui_progress_bar_set_orientation (BobguiProgressBar *pbar,
                                  BobguiOrientation  orientation)
{
  BobguiBoxLayout *layout;

  if (pbar->orientation == orientation)
    return;

  pbar->orientation = orientation;

  if (orientation == BOBGUI_ORIENTATION_HORIZONTAL)
    {
      bobgui_widget_set_vexpand (pbar->trough_widget, FALSE);
      bobgui_widget_set_hexpand (pbar->trough_widget, TRUE);
      bobgui_widget_set_halign (pbar->trough_widget, BOBGUI_ALIGN_FILL);
      bobgui_widget_set_valign (pbar->trough_widget, BOBGUI_ALIGN_CENTER);
    }
  else
    {
      bobgui_widget_set_vexpand (pbar->trough_widget, TRUE);
      bobgui_widget_set_hexpand (pbar->trough_widget, FALSE);
      bobgui_widget_set_halign (pbar->trough_widget, BOBGUI_ALIGN_CENTER);
      bobgui_widget_set_valign (pbar->trough_widget, BOBGUI_ALIGN_FILL);
    }

  bobgui_widget_update_orientation (BOBGUI_WIDGET (pbar), pbar->orientation);
  update_node_classes (pbar);

  layout = BOBGUI_BOX_LAYOUT (bobgui_widget_get_layout_manager (BOBGUI_WIDGET (pbar)));
  bobgui_orientable_set_orientation (BOBGUI_ORIENTABLE (layout), BOBGUI_ORIENTATION_VERTICAL);

  g_object_notify (G_OBJECT (pbar), "orientation");
}

/**
 * bobgui_progress_bar_set_inverted:
 * @pbar: a `BobguiProgressBar`
 * @inverted: %TRUE to invert the progress bar
 *
 * Sets whether the progress bar is inverted.
 *
 * Progress bars normally grow from top to bottom or left to right.
 * Inverted progress bars grow in the opposite direction.
 */
void
bobgui_progress_bar_set_inverted (BobguiProgressBar *pbar,
                               gboolean        inverted)
{
  g_return_if_fail (BOBGUI_IS_PROGRESS_BAR (pbar));

  if (pbar->inverted == inverted)
    return;

  pbar->inverted = inverted;

  bobgui_widget_queue_allocate (pbar->trough_widget);
  update_node_classes (pbar);

  g_object_notify_by_pspec (G_OBJECT (pbar), progress_props[PROP_INVERTED]);
}

/**
 * bobgui_progress_bar_get_text:
 * @pbar: a `BobguiProgressBar`
 *
 * Retrieves the text that is displayed with the progress bar.
 *
 * The return value is a reference to the text, not a copy of it,
 * so will become invalid if you change the text in the progress bar.
 *
 * Returns: (nullable): the text
 */
const char *
bobgui_progress_bar_get_text (BobguiProgressBar *pbar)
{
  g_return_val_if_fail (BOBGUI_IS_PROGRESS_BAR (pbar), NULL);

  return pbar->text;
}

/**
 * bobgui_progress_bar_get_fraction:
 * @pbar: a `BobguiProgressBar`
 *
 * Returns the current fraction of the task that’s been completed.
 *
 * Returns: a fraction from 0.0 to 1.0
 */
double
bobgui_progress_bar_get_fraction (BobguiProgressBar *pbar)
{
  g_return_val_if_fail (BOBGUI_IS_PROGRESS_BAR (pbar), 0);

  return pbar->fraction;
}

/**
 * bobgui_progress_bar_get_pulse_step:
 * @pbar: a `BobguiProgressBar`
 *
 * Retrieves the pulse step.
 *
 * See [method@Bobgui.ProgressBar.set_pulse_step].
 *
 * Returns: a fraction from 0.0 to 1.0
 */
double
bobgui_progress_bar_get_pulse_step (BobguiProgressBar *pbar)
{
  g_return_val_if_fail (BOBGUI_IS_PROGRESS_BAR (pbar), 0);

  return pbar->pulse_fraction;
}

/**
 * bobgui_progress_bar_get_inverted:
 * @pbar: a `BobguiProgressBar`
 *
 * Returns whether the progress bar is inverted.
 *
 * Returns: %TRUE if the progress bar is inverted
 */
gboolean
bobgui_progress_bar_get_inverted (BobguiProgressBar *pbar)
{
  g_return_val_if_fail (BOBGUI_IS_PROGRESS_BAR (pbar), FALSE);

  return pbar->inverted;
}

/**
 * bobgui_progress_bar_set_ellipsize:
 * @pbar: a `BobguiProgressBar`
 * @mode: a `PangoEllipsizeMode`
 *
 * Sets the mode used to ellipsize the text.
 *
 * The text is ellipsized if there is not enough space
 * to render the entire string.
 */
void
bobgui_progress_bar_set_ellipsize (BobguiProgressBar     *pbar,
                                PangoEllipsizeMode  mode)
{
  g_return_if_fail (BOBGUI_IS_PROGRESS_BAR (pbar));
  g_return_if_fail (mode >= PANGO_ELLIPSIZE_NONE &&
                    mode <= PANGO_ELLIPSIZE_END);

  if ((PangoEllipsizeMode)pbar->ellipsize == mode)
    return;

  pbar->ellipsize = mode;

  if (pbar->label)
    bobgui_label_set_ellipsize (BOBGUI_LABEL (pbar->label), mode);

  g_object_notify_by_pspec (G_OBJECT (pbar), progress_props[PROP_ELLIPSIZE]);
}

/**
 * bobgui_progress_bar_get_ellipsize:
 * @pbar: a `BobguiProgressBar`
 *
 * Returns the ellipsizing position of the progress bar.
 *
 * See [method@Bobgui.ProgressBar.set_ellipsize].
 *
 * Returns: `PangoEllipsizeMode`
 */
PangoEllipsizeMode
bobgui_progress_bar_get_ellipsize (BobguiProgressBar *pbar)
{
  g_return_val_if_fail (BOBGUI_IS_PROGRESS_BAR (pbar), PANGO_ELLIPSIZE_NONE);

  return pbar->ellipsize;
}
