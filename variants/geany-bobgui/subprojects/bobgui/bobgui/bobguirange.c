/* BOBGUI - The Bobgui Framework
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 * Copyright (C) 2001 Red Hat, Inc.
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
 * Modified by the BOBGUI Team and others 1997-2004.  See the AUTHORS
 * file for a list of people on the BOBGUI Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BOBGUI at ftp://ftp.bobgui.org/pub/bobgui/. 
 */

#include "config.h"

#include "bobguirangeprivate.h"

#include "bobguiaccessible.h"
#include "bobguiaccessiblerange.h"
#include "bobguiadjustmentprivate.h"
#include "bobguicolorscaleprivate.h"
#include "bobguicssboxesprivate.h"
#include "bobguienums.h"
#include "bobguieventcontrollerkey.h"
#include "bobguieventcontrollerscroll.h"
#include "bobguigesturedrag.h"
#include "bobguigesturelongpressprivate.h"
#include "bobguigestureclick.h"
#include "bobguigizmoprivate.h"
#include "bobguimarshalers.h"
#include "bobguiorientable.h"
#include "bobguiprivate.h"
#include "bobguiscale.h"
#include "bobguisnapshot.h"
#include "bobguitypebuiltins.h"
#include "bobguiwidgetprivate.h"

#include <stdio.h>
#include <math.h>

/**
 * BobguiRange:
 *
 * Base class for widgets which visualize an adjustment.
 *
 * Widgets that are derived from `BobguiRange` include
 * [class@Bobgui.Scale] and [class@Bobgui.Scrollbar].
 *
 * Apart from signals for monitoring the parameters of the adjustment,
 * `BobguiRange` provides properties and methods for setting a
 * “fill level” on range widgets. See [method@Bobgui.Range.set_fill_level].
 *
 * # Shortcuts and Gestures
 *
 * The `BobguiRange` slider is draggable. Holding the <kbd>Shift</kbd> key while
 * dragging, or initiating the drag with a long-press will enable the
 * fine-tuning mode.
 */


#define TIMEOUT_INITIAL     500
#define TIMEOUT_REPEAT      250
#define AUTOSCROLL_FACTOR   20
#define SCROLL_EDGE_SIZE    15
#define MARK_SNAP_LENGTH    12

typedef struct _BobguiRangeStepTimer BobguiRangeStepTimer;

typedef struct _BobguiRangePrivate       BobguiRangePrivate;
struct _BobguiRangePrivate
{
  BobguiWidget *grab_location;   /* "grabbed" mouse location, NULL for no grab */

  BobguiRangeStepTimer *timer;

  BobguiAdjustment     *adjustment;

  int slider_x;
  int slider_y;

  BobguiWidget    *trough_widget;
  BobguiWidget    *fill_widget;
  BobguiWidget    *highlight_widget;
  BobguiWidget    *slider_widget;

  BobguiGesture *drag_gesture;

  double   fill_level;
  double *marks;
  double initial_scroll_value;
  double scroll_delta_accum;

  int *mark_pos;
  int   n_marks;
  int   round_digits;                /* Round off value to this many digits, -1 for no rounding */
  int   slide_initial_slider_position;
  int   slide_initial_coordinate_delta;

  guint flippable              : 1;
  guint inverted               : 1;
  guint slider_size_fixed      : 1;
  guint trough_click_forward   : 1;  /* trough click was on the forward side of slider */

  /* Whether we're doing fine adjustment */
  guint zoom                   : 1;

  /* Fill level */
  guint show_fill_level        : 1;
  guint restrict_to_fill_level : 1;

  /* Whether dragging is ongoing */
  guint in_drag                : 1;
  /* Whether scroll is ongoing */
  guint in_scroll              : 1;

  BobguiOrientation     orientation;

  BobguiScrollType autoscroll_mode;
  guint autoscroll_id;
};


enum {
  PROP_0,
  PROP_ADJUSTMENT,
  PROP_INVERTED,
  PROP_SHOW_FILL_LEVEL,
  PROP_RESTRICT_TO_FILL_LEVEL,
  PROP_FILL_LEVEL,
  PROP_ROUND_DIGITS,
  PROP_ORIENTATION,
  LAST_PROP = PROP_ORIENTATION
};

enum {
  VALUE_CHANGED,
  ADJUST_BOUNDS,
  MOVE_SLIDER,
  CHANGE_VALUE,
  LAST_SIGNAL
};

static void bobgui_range_set_property   (GObject          *object,
                                      guint             prop_id,
                                      const GValue     *value,
                                      GParamSpec       *pspec);
static void bobgui_range_get_property   (GObject          *object,
                                      guint             prop_id,
                                      GValue           *value,
                                      GParamSpec       *pspec);
static void bobgui_range_constructed    (GObject          *object);
static void bobgui_range_finalize       (GObject          *object);
static void bobgui_range_dispose        (GObject          *object);
static void bobgui_range_measure        (BobguiWidget      *widget,
                                      BobguiOrientation  orientation,
                                      int             for_size,
                                      int            *minimum,
                                      int            *natural,
                                      int            *minimum_baseline,
                                      int            *natural_baseline);
static void bobgui_range_size_allocate  (BobguiWidget      *widget,
                                      int             width,
                                      int             height,
                                      int             baseline);
static void bobgui_range_unmap          (BobguiWidget        *widget);

static void bobgui_range_click_gesture_pressed  (BobguiGestureClick *gesture,
                                                   guint                 n_press,
                                                   double                x,
                                                   double                y,
                                                   BobguiRange             *range);
static void bobgui_range_drag_gesture_begin          (BobguiGestureDrag       *gesture,
                                                   double                offset_x,
                                                   double                offset_y,
                                                   BobguiRange             *range);
static void bobgui_range_drag_gesture_update         (BobguiGestureDrag       *gesture,
                                                   double                offset_x,
                                                   double                offset_y,
                                                   BobguiRange             *range);
static void bobgui_range_drag_gesture_end            (BobguiGestureDrag       *gesture,
                                                   double                offset_x,
                                                   double                offset_y,
                                                   BobguiRange             *range);
static void bobgui_range_long_press_gesture_pressed  (BobguiGestureLongPress  *gesture,
                                                   double                x,
                                                   double                y,
                                                   BobguiRange             *range);


static void update_slider_position   (BobguiRange	       *range,
                                      int               mouse_x,
                                      int               mouse_y);
static void stop_scrolling           (BobguiRange         *range);
static void add_autoscroll           (BobguiRange         *range);
static void remove_autoscroll        (BobguiRange         *range);

/* Range methods */

static void bobgui_range_move_slider              (BobguiRange         *range,
                                                BobguiScrollType     scroll);

/* Internals */
static void          bobgui_range_compute_slider_position  (BobguiRange      *range,
                                                         double         adjustment_value,
                                                         GdkRectangle  *slider_rect);
static gboolean      bobgui_range_scroll                   (BobguiRange      *range,
                                                         BobguiScrollType  scroll);
static void          bobgui_range_calc_marks               (BobguiRange      *range);
static void          bobgui_range_adjustment_value_changed (BobguiAdjustment *adjustment,
                                                         gpointer       data);
static void          bobgui_range_adjustment_changed       (BobguiAdjustment *adjustment,
                                                         gpointer       data);
static void          bobgui_range_add_step_timer           (BobguiRange      *range,
                                                         BobguiScrollType  step);
static void          bobgui_range_remove_step_timer        (BobguiRange      *range);
static gboolean      bobgui_range_real_change_value        (BobguiRange      *range,
                                                         BobguiScrollType  scroll,
                                                         double         value);
static gboolean      bobgui_range_key_controller_key_pressed (BobguiEventControllerKey *controller,
                                                           guint                  keyval,
                                                           guint                  keycode,
                                                           GdkModifierType        state,
                                                           BobguiWidget             *widget);
static void          bobgui_range_direction_changed        (BobguiWidget     *widget,
                                                         BobguiTextDirection  previous_direction);
static void          bobgui_range_measure_trough           (BobguiGizmo       *gizmo,
                                                         BobguiOrientation  orientation,
                                                         int             for_size,
                                                         int            *minimum,
                                                         int            *natural,
                                                         int            *minimum_baseline,
                                                         int            *natural_baseline);
static void          bobgui_range_allocate_trough          (BobguiGizmo            *gizmo,
                                                         int                  width,
                                                         int                  height,
                                                         int                  baseline);
static void          bobgui_range_render_trough            (BobguiGizmo     *gizmo,
                                                         BobguiSnapshot  *snapshot);

static void          bobgui_range_scroll_controller_scroll_begin (BobguiEventControllerScroll *scroll,
                                                               BobguiRange                 *range);
static void          bobgui_range_scroll_controller_scroll_end (BobguiEventControllerScroll *scroll,
                                                             BobguiRange                 *range);
static gboolean      bobgui_range_scroll_controller_scroll (BobguiEventControllerScroll *scroll,
                                                         double                    dx,
                                                         double                    dy,
                                                         BobguiRange                 *range);

static void          bobgui_range_set_orientation          (BobguiRange       *range,
                                                         BobguiOrientation  orientation);

static void bobgui_range_accessible_range_init (BobguiAccessibleRangeInterface *iface);

G_DEFINE_TYPE_WITH_CODE (BobguiRange, bobgui_range, BOBGUI_TYPE_WIDGET,
                         G_ADD_PRIVATE (BobguiRange)
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_ACCESSIBLE_RANGE,
                                                bobgui_range_accessible_range_init)
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_ORIENTABLE,
                                                NULL))

static guint signals[LAST_SIGNAL];
static GParamSpec *properties[LAST_PROP];

static void
bobgui_range_class_init (BobguiRangeClass *class)
{
  GObjectClass   *gobject_class;
  BobguiWidgetClass *widget_class;

  gobject_class = G_OBJECT_CLASS (class);
  widget_class = (BobguiWidgetClass*) class;

  gobject_class->set_property = bobgui_range_set_property;
  gobject_class->get_property = bobgui_range_get_property;
  gobject_class->constructed = bobgui_range_constructed;
  gobject_class->finalize = bobgui_range_finalize;
  gobject_class->dispose = bobgui_range_dispose;

  widget_class->measure = bobgui_range_measure;
  widget_class->size_allocate = bobgui_range_size_allocate;
  widget_class->unmap = bobgui_range_unmap;
  widget_class->direction_changed = bobgui_range_direction_changed;

  class->move_slider = bobgui_range_move_slider;
  class->change_value = bobgui_range_real_change_value;

  /**
   * BobguiRange::value-changed:
   * @range: the `BobguiRange` that received the signal
   *
   * Emitted when the range value changes.
   */
  signals[VALUE_CHANGED] =
    g_signal_new (I_("value-changed"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BobguiRangeClass, value_changed),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);

  /**
   * BobguiRange::adjust-bounds:
   * @range: the `BobguiRange` that received the signal
   * @value: the value before we clamp
   *
   * Emitted before clamping a value, to give the application a
   * chance to adjust the bounds.
   */
  signals[ADJUST_BOUNDS] =
    g_signal_new (I_("adjust-bounds"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BobguiRangeClass, adjust_bounds),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_DOUBLE);

  /**
   * BobguiRange::move-slider:
   * @range: the `BobguiRange` that received the signal
   * @step: how to move the slider
   *
   * Virtual function that moves the slider.
   *
   * Used for keybindings.
   */
  signals[MOVE_SLIDER] =
    g_signal_new (I_("move-slider"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (BobguiRangeClass, move_slider),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 1,
                  BOBGUI_TYPE_SCROLL_TYPE);

  /**
   * BobguiRange::change-value:
   * @range: the `BobguiRange` that received the signal
   * @scroll: the type of scroll action that was performed
   * @value: the new value resulting from the scroll action
   *
   * Emitted when a scroll action is performed on a range.
   *
   * It allows an application to determine the type of scroll event
   * that occurred and the resultant new value. The application can
   * handle the event itself and return %TRUE to prevent further
   * processing. Or, by returning %FALSE, it can pass the event to
   * other handlers until the default BOBGUI handler is reached.
   *
   * The value parameter is unrounded. An application that overrides
   * the ::change-value signal is responsible for clamping the value
   * to the desired number of decimal digits; the default BOBGUI
   * handler clamps the value based on [property@Bobgui.Range:round-digits].
   *
   * Returns: %TRUE to prevent other handlers from being invoked for
   *     the signal, %FALSE to propagate the signal further
   */
  signals[CHANGE_VALUE] =
    g_signal_new (I_("change-value"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BobguiRangeClass, change_value),
                  _bobgui_boolean_handled_accumulator, NULL,
                  _bobgui_marshal_BOOLEAN__ENUM_DOUBLE,
                  G_TYPE_BOOLEAN, 2,
                  BOBGUI_TYPE_SCROLL_TYPE,
                  G_TYPE_DOUBLE);
  g_signal_set_va_marshaller (signals[CHANGE_VALUE],
                              G_TYPE_FROM_CLASS (gobject_class),
                              _bobgui_marshal_BOOLEAN__ENUM_DOUBLEv);

  g_object_class_override_property (gobject_class, PROP_ORIENTATION, "orientation");

  /**
   * BobguiRange:adjustment:
   *
   * The adjustment that is controlled by the range.
   */
  properties[PROP_ADJUSTMENT] =
      g_param_spec_object ("adjustment", NULL, NULL,
                           BOBGUI_TYPE_ADJUSTMENT,
                           BOBGUI_PARAM_READWRITE|G_PARAM_CONSTRUCT|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiRange:inverted:
   *
   * If %TRUE, the direction in which the slider moves is inverted.
   */
  properties[PROP_INVERTED] =
      g_param_spec_boolean ("inverted", NULL, NULL,
                            FALSE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiRange:show-fill-level:
   *
   * Controls whether fill level indicator graphics are displayed
   * on the trough.
   */
  properties[PROP_SHOW_FILL_LEVEL] =
      g_param_spec_boolean ("show-fill-level", NULL, NULL,
                            FALSE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiRange:restrict-to-fill-level:
   *
   * Controls whether slider movement is restricted to an
   * upper boundary set by the fill level.
   */
  properties[PROP_RESTRICT_TO_FILL_LEVEL] =
      g_param_spec_boolean ("restrict-to-fill-level", NULL, NULL,
                            TRUE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiRange:fill-level:
   *
   * The fill level (e.g. prebuffering of a network stream).
   */
  properties[PROP_FILL_LEVEL] =
      g_param_spec_double ("fill-level", NULL, NULL,
                           -G_MAXDOUBLE, G_MAXDOUBLE,
                           G_MAXDOUBLE,
                           BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiRange:round-digits:
   *
   * The number of digits to round the value to when
   * it changes.
   *
   * See [signal@Bobgui.Range::change-value].
   */
  properties[PROP_ROUND_DIGITS] =
      g_param_spec_int ("round-digits", NULL, NULL,
                        -1, G_MAXINT,
                        -1,
                        BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (gobject_class, LAST_PROP, properties);

  bobgui_widget_class_set_css_name (widget_class, I_("range"));
  bobgui_widget_class_set_accessible_role (widget_class, BOBGUI_ACCESSIBLE_ROLE_GENERIC);
}

static gboolean
accessible_range_set_current_value (BobguiAccessibleRange *accessible_range,
                                    double              value)
{
  bobgui_range_set_value (BOBGUI_RANGE (accessible_range), value);

  return TRUE;
}

static void
bobgui_range_accessible_range_init (BobguiAccessibleRangeInterface *iface)
{
  iface->set_current_value = accessible_range_set_current_value;
}

static void
bobgui_range_set_property (GObject      *object,
			guint         prop_id,
			const GValue *value,
			GParamSpec   *pspec)
{
  BobguiRange *range = BOBGUI_RANGE (object);

  switch (prop_id)
    {
    case PROP_ORIENTATION:
      bobgui_range_set_orientation (range, g_value_get_enum (value));
      break;
    case PROP_ADJUSTMENT:
      bobgui_range_set_adjustment (range, g_value_get_object (value));
      break;
    case PROP_INVERTED:
      bobgui_range_set_inverted (range, g_value_get_boolean (value));
      break;
    case PROP_SHOW_FILL_LEVEL:
      bobgui_range_set_show_fill_level (range, g_value_get_boolean (value));
      break;
    case PROP_RESTRICT_TO_FILL_LEVEL:
      bobgui_range_set_restrict_to_fill_level (range, g_value_get_boolean (value));
      break;
    case PROP_FILL_LEVEL:
      bobgui_range_set_fill_level (range, g_value_get_double (value));
      break;
    case PROP_ROUND_DIGITS:
      bobgui_range_set_round_digits (range, g_value_get_int (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_range_get_property (GObject      *object,
			guint         prop_id,
			GValue       *value,
			GParamSpec   *pspec)
{
  BobguiRange *range = BOBGUI_RANGE (object);
  BobguiRangePrivate *priv = bobgui_range_get_instance_private (range);

  switch (prop_id)
    {
    case PROP_ORIENTATION:
      g_value_set_enum (value, priv->orientation);
      break;
    case PROP_ADJUSTMENT:
      g_value_set_object (value, priv->adjustment);
      break;
    case PROP_INVERTED:
      g_value_set_boolean (value, priv->inverted);
      break;
    case PROP_SHOW_FILL_LEVEL:
      g_value_set_boolean (value, bobgui_range_get_show_fill_level (range));
      break;
    case PROP_RESTRICT_TO_FILL_LEVEL:
      g_value_set_boolean (value, bobgui_range_get_restrict_to_fill_level (range));
      break;
    case PROP_FILL_LEVEL:
      g_value_set_double (value, bobgui_range_get_fill_level (range));
      break;
    case PROP_ROUND_DIGITS:
      g_value_set_int (value, bobgui_range_get_round_digits (range));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_range_init (BobguiRange *range)
{
  BobguiRangePrivate *priv = bobgui_range_get_instance_private (range);
  BobguiGesture *gesture;
  BobguiEventController *controller;

  priv->orientation = BOBGUI_ORIENTATION_HORIZONTAL;
  priv->adjustment = NULL;
  priv->inverted = FALSE;
  priv->flippable = FALSE;
  priv->round_digits = -1;
  priv->show_fill_level = FALSE;
  priv->restrict_to_fill_level = TRUE;
  priv->fill_level = G_MAXDOUBLE;
  priv->timer = NULL;

  bobgui_widget_update_orientation (BOBGUI_WIDGET (range), priv->orientation);

  priv->trough_widget = bobgui_gizmo_new_with_role ("trough",
                                                 BOBGUI_ACCESSIBLE_ROLE_NONE,
                                                 bobgui_range_measure_trough,
                                                 bobgui_range_allocate_trough,
                                                 bobgui_range_render_trough,
                                                 NULL,
                                                 NULL, NULL);

  bobgui_widget_set_parent (priv->trough_widget, BOBGUI_WIDGET (range));

  priv->slider_widget = bobgui_gizmo_new ("slider", NULL, NULL, NULL, NULL, NULL, NULL);
  bobgui_widget_set_parent (priv->slider_widget, priv->trough_widget);

  /* Note: Order is important here.
   * The ::drag-begin handler relies on the state set up by the
   * click ::pressed handler. Gestures are handling events
   * in the opposite order in which they are added to their
   * widget.
   */
  priv->drag_gesture = bobgui_gesture_drag_new ();
  bobgui_gesture_single_set_button (BOBGUI_GESTURE_SINGLE (priv->drag_gesture), 0);
  g_signal_connect (priv->drag_gesture, "drag-begin",
                    G_CALLBACK (bobgui_range_drag_gesture_begin), range);
  g_signal_connect (priv->drag_gesture, "drag-update",
                    G_CALLBACK (bobgui_range_drag_gesture_update), range);
  g_signal_connect (priv->drag_gesture, "drag-end",
                    G_CALLBACK (bobgui_range_drag_gesture_end), range);
  bobgui_widget_add_controller (BOBGUI_WIDGET (range), BOBGUI_EVENT_CONTROLLER (priv->drag_gesture));

  gesture = bobgui_gesture_click_new ();
  bobgui_gesture_single_set_button (BOBGUI_GESTURE_SINGLE (gesture), 0);
  g_signal_connect (gesture, "pressed",
                    G_CALLBACK (bobgui_range_click_gesture_pressed), range);
  bobgui_widget_add_controller (BOBGUI_WIDGET (range), BOBGUI_EVENT_CONTROLLER (gesture));
  bobgui_gesture_group (gesture, priv->drag_gesture);

  gesture = bobgui_gesture_long_press_new ();
  bobgui_gesture_long_press_set_delay_factor (BOBGUI_GESTURE_LONG_PRESS (gesture), 2.0);
  g_signal_connect (gesture, "pressed",
                    G_CALLBACK (bobgui_range_long_press_gesture_pressed), range);
  bobgui_widget_add_controller (BOBGUI_WIDGET (range), BOBGUI_EVENT_CONTROLLER (gesture));
  bobgui_gesture_group (gesture, priv->drag_gesture);

  controller = bobgui_event_controller_key_new ();
  g_signal_connect (controller, "key-pressed",
                    G_CALLBACK (bobgui_range_key_controller_key_pressed), range);
  bobgui_widget_add_controller (BOBGUI_WIDGET (range), controller);
}

static void
bobgui_range_set_orientation (BobguiRange       *range,
                           BobguiOrientation  orientation)
{
  BobguiRangePrivate *priv = bobgui_range_get_instance_private (range);

  if (priv->orientation != orientation)
    {
      priv->orientation = orientation;

      bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (range),
                                      BOBGUI_ACCESSIBLE_PROPERTY_ORIENTATION, priv->orientation,
                                      -1);

      bobgui_widget_update_orientation (BOBGUI_WIDGET (range), priv->orientation);
      bobgui_widget_queue_resize (BOBGUI_WIDGET (range));

      g_object_notify (G_OBJECT (range), "orientation");
    }
}

/**
 * bobgui_range_get_adjustment:
 * @range: a `BobguiRange`
 *
 * Get the adjustment which is the “model” object for `BobguiRange`.
 *
 * Returns: (transfer none): a `BobguiAdjustment`
 **/
BobguiAdjustment*
bobgui_range_get_adjustment (BobguiRange *range)
{
  BobguiRangePrivate *priv = bobgui_range_get_instance_private (range);

  g_return_val_if_fail (BOBGUI_IS_RANGE (range), NULL);

  if (!priv->adjustment)
    bobgui_range_set_adjustment (range, NULL);

  return priv->adjustment;
}

/**
 * bobgui_range_set_adjustment:
 * @range: a `BobguiRange`
 * @adjustment: a `BobguiAdjustment`
 *
 * Sets the adjustment to be used as the “model” object for the `BobguiRange`
 *
 * The adjustment indicates the current range value, the minimum and
 * maximum range values, the step/page increments used for keybindings
 * and scrolling, and the page size.
 *
 * The page size is normally 0 for `BobguiScale` and nonzero for `BobguiScrollbar`,
 * and indicates the size of the visible area of the widget being scrolled.
 * The page size affects the size of the scrollbar slider.
 */
void
bobgui_range_set_adjustment (BobguiRange      *range,
			  BobguiAdjustment *adjustment)
{
  BobguiRangePrivate *priv = bobgui_range_get_instance_private (range);

  g_return_if_fail (BOBGUI_IS_RANGE (range));

  if (!adjustment)
    adjustment = bobgui_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
  else
    g_return_if_fail (BOBGUI_IS_ADJUSTMENT (adjustment));

  if (priv->adjustment != adjustment)
    {
      double accessible_max;

      if (priv->adjustment)
	{
	  g_signal_handlers_disconnect_by_func (priv->adjustment,
						bobgui_range_adjustment_changed,
						range);
	  g_signal_handlers_disconnect_by_func (priv->adjustment,
						bobgui_range_adjustment_value_changed,
						range);
	  g_object_unref (priv->adjustment);
	}

      priv->adjustment = adjustment;
      g_object_ref_sink (adjustment);
      
      g_signal_connect (adjustment, "changed",
			G_CALLBACK (bobgui_range_adjustment_changed),
			range);
      g_signal_connect (adjustment, "value-changed",
			G_CALLBACK (bobgui_range_adjustment_value_changed),
			range);

      accessible_max = bobgui_adjustment_get_upper (adjustment) -
                       bobgui_adjustment_get_page_size (adjustment);

      if (priv->restrict_to_fill_level)
        accessible_max = MIN (accessible_max, priv->fill_level);

      bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (range),
                                      BOBGUI_ACCESSIBLE_PROPERTY_VALUE_MAX, accessible_max,
                                      BOBGUI_ACCESSIBLE_PROPERTY_VALUE_MIN, bobgui_adjustment_get_lower (adjustment),
                                      BOBGUI_ACCESSIBLE_PROPERTY_VALUE_NOW, bobgui_adjustment_get_value (adjustment),
                                      -1);

      bobgui_range_adjustment_changed (adjustment, range);
      bobgui_range_adjustment_value_changed (adjustment, range);

      g_object_notify_by_pspec (G_OBJECT (range), properties[PROP_ADJUSTMENT]);
    }
}

static void
update_accessible_range (BobguiRange *range)
{
  BobguiRangePrivate *priv = bobgui_range_get_instance_private (range);
  double upper = bobgui_adjustment_get_upper (priv->adjustment);
  double lower = bobgui_adjustment_get_lower (priv->adjustment);
  double page_size = bobgui_adjustment_get_page_size (priv->adjustment);
  double accessible_max = upper - page_size;

  if (priv->restrict_to_fill_level)
    accessible_max = MIN (accessible_max, priv->fill_level);

  bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (range),
                                  BOBGUI_ACCESSIBLE_PROPERTY_VALUE_MAX, accessible_max,
                                  BOBGUI_ACCESSIBLE_PROPERTY_VALUE_MIN, lower,
                                  -1);
}

static gboolean
should_invert (BobguiRange *range)
{
  BobguiRangePrivate *priv = bobgui_range_get_instance_private (range);

  if (priv->orientation == BOBGUI_ORIENTATION_HORIZONTAL)
    return
      (priv->inverted && !priv->flippable) ||
      (priv->inverted && priv->flippable && bobgui_widget_get_direction (BOBGUI_WIDGET (range)) == BOBGUI_TEXT_DIR_LTR) ||
      (!priv->inverted && priv->flippable && bobgui_widget_get_direction (BOBGUI_WIDGET (range)) == BOBGUI_TEXT_DIR_RTL);
  else
    return priv->inverted;
}

static gboolean
should_invert_move (BobguiRange       *range,
                    BobguiOrientation  move_orientation)
{
  BobguiRangePrivate *priv = bobgui_range_get_instance_private (range);

  /* If the move is parallel to the range, use general check for inversion */
  if (move_orientation == priv->orientation)
    return should_invert (range);

  /* H scale/V move: Always invert, so down/up always dec/increase the value */
  if (priv->orientation == BOBGUI_ORIENTATION_HORIZONTAL && BOBGUI_IS_SCALE (range))
    return TRUE;

  /* V range/H move: Left/right always dec/increase the value */
  return FALSE;
}

static void
update_highlight_position (BobguiRange *range)
{
  BobguiRangePrivate *priv = bobgui_range_get_instance_private (range);

  if (!priv->highlight_widget)
    return;

  if (should_invert (range))
    {
      bobgui_widget_add_css_class (priv->highlight_widget, "bottom");
      bobgui_widget_remove_css_class (priv->highlight_widget, "top");
    }
  else
    {
      bobgui_widget_add_css_class (priv->highlight_widget, "top");
      bobgui_widget_remove_css_class (priv->highlight_widget, "bottom");
    }
}

static void
update_fill_position (BobguiRange *range)
{
  BobguiRangePrivate *priv = bobgui_range_get_instance_private (range);

  if (!priv->fill_widget)
    return;

  if (should_invert (range))
    {
      bobgui_widget_add_css_class (priv->fill_widget, "bottom");
      bobgui_widget_remove_css_class (priv->fill_widget, "top");
    }
  else
    {
      bobgui_widget_add_css_class (priv->fill_widget, "top");
      bobgui_widget_remove_css_class (priv->fill_widget, "bottom");
    }
}

/**
 * bobgui_range_set_inverted:
 * @range: a `BobguiRange`
 * @setting: %TRUE to invert the range
 *
 * Sets whether to invert the range.
 *
 * Ranges normally move from lower to higher values as the
 * slider moves from top to bottom or left to right. Inverted
 * ranges have higher values at the top or on the right rather
 * than on the bottom or left.
 */
void
bobgui_range_set_inverted (BobguiRange *range,
                        gboolean  setting)
{
  BobguiRangePrivate *priv = bobgui_range_get_instance_private (range);

  g_return_if_fail (BOBGUI_IS_RANGE (range));

  setting = setting != FALSE;

  if (setting != priv->inverted)
    {
      priv->inverted = setting;

      update_fill_position (range);
      update_highlight_position (range);

      bobgui_widget_queue_resize (priv->trough_widget);

      g_object_notify_by_pspec (G_OBJECT (range), properties[PROP_INVERTED]);
    }
}

/**
 * bobgui_range_get_inverted:
 * @range: a `BobguiRange`
 *
 * Gets whether the range is inverted.
 *
 * See [method@Bobgui.Range.set_inverted].
 *
 * Returns: %TRUE if the range is inverted
 */
gboolean
bobgui_range_get_inverted (BobguiRange *range)
{
  BobguiRangePrivate *priv = bobgui_range_get_instance_private (range);

  g_return_val_if_fail (BOBGUI_IS_RANGE (range), FALSE);

  return priv->inverted;
}

/**
 * bobgui_range_set_flippable:
 * @range: a `BobguiRange`
 * @flippable: %TRUE to make the range flippable
 *
 * Sets whether the `BobguiRange` respects text direction.
 *
 * If a range is flippable, it will switch its direction
 * if it is horizontal and its direction is %BOBGUI_TEXT_DIR_RTL.
 *
 * See [method@Bobgui.Widget.get_direction].
 */
void
bobgui_range_set_flippable (BobguiRange *range,
                         gboolean  flippable)
{
  BobguiRangePrivate *priv = bobgui_range_get_instance_private (range);

  g_return_if_fail (BOBGUI_IS_RANGE (range));

  flippable = flippable ? TRUE : FALSE;

  if (flippable != priv->flippable)
    {
      priv->flippable = flippable;
      update_fill_position (range);
      update_highlight_position (range);

      bobgui_widget_queue_allocate (BOBGUI_WIDGET (range));
    }
}

/**
 * bobgui_range_get_flippable:
 * @range: a `BobguiRange`
 *
 * Gets whether the `BobguiRange` respects text direction.
 *
 * See [method@Bobgui.Range.set_flippable].
 *
 * Returns: %TRUE if the range is flippable
 */
gboolean
bobgui_range_get_flippable (BobguiRange *range)
{
  BobguiRangePrivate *priv = bobgui_range_get_instance_private (range);

  g_return_val_if_fail (BOBGUI_IS_RANGE (range), FALSE);

  return priv->flippable;
}

/**
 * bobgui_range_set_slider_size_fixed:
 * @range: a `BobguiRange`
 * @size_fixed: %TRUE to make the slider size constant
 *
 * Sets whether the range’s slider has a fixed size, or a size that
 * depends on its adjustment’s page size.
 *
 * This function is useful mainly for `BobguiRange` subclasses.
 */
void
bobgui_range_set_slider_size_fixed (BobguiRange *range,
                                 gboolean  size_fixed)
{
  BobguiRangePrivate *priv = bobgui_range_get_instance_private (range);

  g_return_if_fail (BOBGUI_IS_RANGE (range));

  if (size_fixed != priv->slider_size_fixed)
    {
      priv->slider_size_fixed = size_fixed ? TRUE : FALSE;

      if (priv->adjustment && bobgui_widget_get_mapped (BOBGUI_WIDGET (range)))
        bobgui_widget_queue_allocate (priv->trough_widget);
    }
}

/**
 * bobgui_range_get_slider_size_fixed:
 * @range: a `BobguiRange`
 *
 * This function is useful mainly for `BobguiRange` subclasses.
 *
 * See [method@Bobgui.Range.set_slider_size_fixed].
 *
 * Returns: whether the range’s slider has a fixed size.
 */
gboolean
bobgui_range_get_slider_size_fixed (BobguiRange *range)
{
  BobguiRangePrivate *priv = bobgui_range_get_instance_private (range);

  g_return_val_if_fail (BOBGUI_IS_RANGE (range), FALSE);

  return priv->slider_size_fixed;
}

/**
 * bobgui_range_get_range_rect:
 * @range: a `BobguiRange`
 * @range_rect: (out): return location for the range rectangle
 *
 * This function returns the area that contains the range’s trough,
 * in coordinates relative to @range's origin.
 *
 * This function is useful mainly for `BobguiRange` subclasses.
 */
void
bobgui_range_get_range_rect (BobguiRange     *range,
                          GdkRectangle *range_rect)
{
  BobguiRangePrivate *priv = bobgui_range_get_instance_private (range);
  graphene_rect_t r;

  g_return_if_fail (BOBGUI_IS_RANGE (range));
  g_return_if_fail (range_rect != NULL);

  if (!bobgui_widget_compute_bounds (priv->trough_widget, BOBGUI_WIDGET (range), &r))
    {
      *range_rect = (GdkRectangle) { 0, 0, 0, 0 };
    }
  else
    {
      *range_rect = (GdkRectangle) {
        floorf (r.origin.x),
        floorf (r.origin.y),
        ceilf (r.size.width),
        ceilf (r.size.height),
      };
    }
}

/**
 * bobgui_range_get_slider_range:
 * @range: a `BobguiRange`
 * @slider_start: (out) (optional): return location for the slider's start
 * @slider_end: (out) (optional): return location for the slider's end
 *
 * This function returns sliders range along the long dimension,
 * in widget->window coordinates.
 *
 * This function is useful mainly for `BobguiRange` subclasses.
 */
void
bobgui_range_get_slider_range (BobguiRange *range,
                            int      *slider_start,
                            int      *slider_end)
{
  BobguiRangePrivate *priv = bobgui_range_get_instance_private (range);
  graphene_rect_t slider_bounds;

  g_return_if_fail (BOBGUI_IS_RANGE (range));

  if (!bobgui_widget_compute_bounds (priv->slider_widget, BOBGUI_WIDGET (range), &slider_bounds))
    {
      if (slider_start)
        *slider_start = 0;
      if (slider_end)
        *slider_end = 0;
      return;
    }

  if (priv->orientation == BOBGUI_ORIENTATION_VERTICAL)
    {
      if (slider_start)
        *slider_start = slider_bounds.origin.y;
      if (slider_end)
        *slider_end = slider_bounds.origin.y + slider_bounds.size.height;
    }
  else
    {
      if (slider_start)
        *slider_start = slider_bounds.origin.x;
      if (slider_end)
        *slider_end = slider_bounds.origin.x + slider_bounds.size.width;
    }
}

/**
 * bobgui_range_set_increments:
 * @range: a `BobguiRange`
 * @step: step size
 * @page: page size
 *
 * Sets the step and page sizes for the range.
 *
 * The step size is used when the user clicks the `BobguiScrollbar`
 * arrows or moves a `BobguiScale` via arrow keys. The page size
 * is used for example when moving via Page Up or Page Down keys.
 */
void
bobgui_range_set_increments (BobguiRange *range,
                          double    step,
                          double    page)
{
  BobguiRangePrivate *priv = bobgui_range_get_instance_private (range);
  BobguiAdjustment *adjustment;

  g_return_if_fail (BOBGUI_IS_RANGE (range));

  adjustment = priv->adjustment;

  bobgui_adjustment_configure (adjustment,
                            bobgui_adjustment_get_value (adjustment),
                            bobgui_adjustment_get_lower (adjustment),
                            bobgui_adjustment_get_upper (adjustment),
                            step,
                            page,
                            bobgui_adjustment_get_page_size (adjustment));
}

/**
 * bobgui_range_set_range:
 * @range: a `BobguiRange`
 * @min: minimum range value
 * @max: maximum range value
 *
 * Sets the allowable values in the `BobguiRange`.
 *
 * The range value is clamped to be between @min and @max.
 * (If the range has a non-zero page size, it is clamped
 * between @min and @max - page-size.)
 */
void
bobgui_range_set_range (BobguiRange *range,
                     double    min,
                     double    max)
{
  BobguiRangePrivate *priv = bobgui_range_get_instance_private (range);
  BobguiAdjustment *adjustment;
  double value;
  
  g_return_if_fail (BOBGUI_IS_RANGE (range));
  g_return_if_fail (min <= max);

  adjustment = priv->adjustment;

  value = bobgui_adjustment_get_value (adjustment);
  if (priv->restrict_to_fill_level)
    value = MIN (value, MAX (bobgui_adjustment_get_lower (adjustment),
                             priv->fill_level));

  bobgui_adjustment_configure (adjustment,
                            value,
                            min,
                            max,
                            bobgui_adjustment_get_step_increment (adjustment),
                            bobgui_adjustment_get_page_increment (adjustment),
                            bobgui_adjustment_get_page_size (adjustment));
}

/**
 * bobgui_range_set_value:
 * @range: a `BobguiRange`
 * @value: new value of the range
 *
 * Sets the current value of the range.
 *
 * If the value is outside the minimum or maximum range values,
 * it will be clamped to fit inside them. The range emits the
 * [signal@Bobgui.Range::value-changed] signal if the value changes.
 */
void
bobgui_range_set_value (BobguiRange *range,
                     double    value)
{
  BobguiRangePrivate *priv = bobgui_range_get_instance_private (range);

  g_return_if_fail (BOBGUI_IS_RANGE (range));

  if (priv->restrict_to_fill_level)
    value = MIN (value, MAX (bobgui_adjustment_get_lower (priv->adjustment),
                             priv->fill_level));

  bobgui_adjustment_set_value (priv->adjustment, value);
}

/**
 * bobgui_range_get_value:
 * @range: a `BobguiRange`
 *
 * Gets the current value of the range.
 *
 * Returns: current value of the range.
 */
double
bobgui_range_get_value (BobguiRange *range)
{
  BobguiRangePrivate *priv = bobgui_range_get_instance_private (range);

  g_return_val_if_fail (BOBGUI_IS_RANGE (range), 0.0);

  return bobgui_adjustment_get_value (priv->adjustment);
}

/**
 * bobgui_range_set_show_fill_level:
 * @range: A `BobguiRange`
 * @show_fill_level: Whether a fill level indicator graphics is shown.
 *
 * Sets whether a graphical fill level is show on the trough.
 *
 * See [method@Bobgui.Range.set_fill_level] for a general description
 * of the fill level concept.
 */
void
bobgui_range_set_show_fill_level (BobguiRange *range,
                               gboolean  show_fill_level)
{
  BobguiRangePrivate *priv = bobgui_range_get_instance_private (range);

  g_return_if_fail (BOBGUI_IS_RANGE (range));

  show_fill_level = show_fill_level ? TRUE : FALSE;

  if (show_fill_level == priv->show_fill_level)
    return;

  priv->show_fill_level = show_fill_level;

  if (show_fill_level)
    {
      priv->fill_widget = bobgui_gizmo_new ("fill", NULL, NULL, NULL, NULL, NULL, NULL);
      bobgui_widget_insert_after (priv->fill_widget, priv->trough_widget, NULL);
      update_fill_position (range);
    }
  else
    {
      g_clear_pointer (&priv->fill_widget, bobgui_widget_unparent);
    }

  g_object_notify_by_pspec (G_OBJECT (range), properties[PROP_SHOW_FILL_LEVEL]);
  bobgui_widget_queue_allocate (BOBGUI_WIDGET (range));
}

/**
 * bobgui_range_get_show_fill_level:
 * @range: A `BobguiRange`
 *
 * Gets whether the range displays the fill level graphically.
 *
 * Returns: %TRUE if @range shows the fill level.
 */
gboolean
bobgui_range_get_show_fill_level (BobguiRange *range)
{
  BobguiRangePrivate *priv = bobgui_range_get_instance_private (range);

  g_return_val_if_fail (BOBGUI_IS_RANGE (range), FALSE);

  return priv->show_fill_level;
}

/**
 * bobgui_range_set_restrict_to_fill_level:
 * @range: A `BobguiRange`
 * @restrict_to_fill_level: Whether the fill level restricts slider movement.
 *
 * Sets whether the slider is restricted to the fill level.
 *
 * See [method@Bobgui.Range.set_fill_level] for a general description
 * of the fill level concept.
 */
void
bobgui_range_set_restrict_to_fill_level (BobguiRange *range,
                                      gboolean  restrict_to_fill_level)
{
  BobguiRangePrivate *priv = bobgui_range_get_instance_private (range);

  g_return_if_fail (BOBGUI_IS_RANGE (range));

  restrict_to_fill_level = restrict_to_fill_level ? TRUE : FALSE;

  if (restrict_to_fill_level != priv->restrict_to_fill_level)
    {
      priv->restrict_to_fill_level = restrict_to_fill_level;
      g_object_notify_by_pspec (G_OBJECT (range), properties[PROP_RESTRICT_TO_FILL_LEVEL]);

      bobgui_range_set_value (range, bobgui_range_get_value (range));

      update_accessible_range (range);
    }
}

/**
 * bobgui_range_get_restrict_to_fill_level:
 * @range: A `BobguiRange`
 *
 * Gets whether the range is restricted to the fill level.
 *
 * Returns: %TRUE if @range is restricted to the fill level.
 **/
gboolean
bobgui_range_get_restrict_to_fill_level (BobguiRange *range)
{
  BobguiRangePrivate *priv = bobgui_range_get_instance_private (range);

  g_return_val_if_fail (BOBGUI_IS_RANGE (range), FALSE);

  return priv->restrict_to_fill_level;
}

/**
 * bobgui_range_set_fill_level:
 * @range: a `BobguiRange`
 * @fill_level: the new position of the fill level indicator
 *
 * Set the new position of the fill level indicator.
 *
 * The “fill level” is probably best described by its most prominent
 * use case, which is an indicator for the amount of pre-buffering in
 * a streaming media player. In that use case, the value of the range
 * would indicate the current play position, and the fill level would
 * be the position up to which the file/stream has been downloaded.
 *
 * This amount of prebuffering can be displayed on the range’s trough
 * and is themeable separately from the trough. To enable fill level
 * display, use [method@Bobgui.Range.set_show_fill_level]. The range defaults
 * to not showing the fill level.
 *
 * Additionally, it’s possible to restrict the range’s slider position
 * to values which are smaller than the fill level. This is controlled
 * by [method@Bobgui.Range.set_restrict_to_fill_level] and is by default
 * enabled.
 */
void
bobgui_range_set_fill_level (BobguiRange *range,
                          double    fill_level)
{
  BobguiRangePrivate *priv = bobgui_range_get_instance_private (range);

  g_return_if_fail (BOBGUI_IS_RANGE (range));

  if (fill_level != priv->fill_level)
    {
      priv->fill_level = fill_level;
      g_object_notify_by_pspec (G_OBJECT (range), properties[PROP_FILL_LEVEL]);

      if (priv->show_fill_level)
        bobgui_widget_queue_allocate (BOBGUI_WIDGET (range));

      if (priv->restrict_to_fill_level)
        {
          bobgui_range_set_value (range, bobgui_range_get_value (range));
          update_accessible_range (range);
        }
    }
}

/**
 * bobgui_range_get_fill_level:
 * @range: A `BobguiRange`
 *
 * Gets the current position of the fill level indicator.
 *
 * Returns: The current fill level
 */
double
bobgui_range_get_fill_level (BobguiRange *range)
{
  BobguiRangePrivate *priv = bobgui_range_get_instance_private (range);

  g_return_val_if_fail (BOBGUI_IS_RANGE (range), 0.0);

  return priv->fill_level;
}

static void
bobgui_range_dispose (GObject *object)
{
  BobguiRange *range = BOBGUI_RANGE (object);
  BobguiRangePrivate *priv = bobgui_range_get_instance_private (range);

  bobgui_range_remove_step_timer (range);

  if (priv->adjustment)
    {
      g_signal_handlers_disconnect_by_func (priv->adjustment,
					    bobgui_range_adjustment_changed,
					    range);
      g_signal_handlers_disconnect_by_func (priv->adjustment,
					    bobgui_range_adjustment_value_changed,
					    range);
      g_object_unref (priv->adjustment);
      priv->adjustment = NULL;
    }

  if (priv->n_marks)
    {
      g_free (priv->marks);
      priv->marks = NULL;
      g_free (priv->mark_pos);
      priv->mark_pos = NULL;
      priv->n_marks = 0;
    }

  G_OBJECT_CLASS (bobgui_range_parent_class)->dispose (object);
}

static void
bobgui_range_constructed (GObject *object)
{
  BobguiRange *range = BOBGUI_RANGE (object);
  BobguiEventController *controller;
  BobguiEventControllerScrollFlags flags;

  flags = BOBGUI_EVENT_CONTROLLER_SCROLL_BOTH_AXES;

  if (BOBGUI_IS_SCALE (object))
    flags |= BOBGUI_EVENT_CONTROLLER_SCROLL_PHYSICAL_DIRECTION;

  controller = bobgui_event_controller_scroll_new (flags);
  g_signal_connect (controller, "scroll-begin",
                    G_CALLBACK (bobgui_range_scroll_controller_scroll_begin), range);
  g_signal_connect (controller, "scroll",
                    G_CALLBACK (bobgui_range_scroll_controller_scroll), range);
  g_signal_connect (controller, "scroll-end",
                    G_CALLBACK (bobgui_range_scroll_controller_scroll_end), range);
  bobgui_widget_add_controller (BOBGUI_WIDGET (range), controller);

  G_OBJECT_CLASS (bobgui_range_parent_class)->constructed (object);
}

static void
bobgui_range_finalize (GObject *object)
{
  BobguiRange *range = BOBGUI_RANGE (object);
  BobguiRangePrivate *priv = bobgui_range_get_instance_private (range);

  g_clear_pointer (&priv->slider_widget, bobgui_widget_unparent);
  g_clear_pointer (&priv->fill_widget, bobgui_widget_unparent);
  g_clear_pointer (&priv->highlight_widget, bobgui_widget_unparent);
  g_clear_pointer (&priv->trough_widget, bobgui_widget_unparent);

  G_OBJECT_CLASS (bobgui_range_parent_class)->finalize (object);
}

static void
bobgui_range_measure_trough (BobguiGizmo       *gizmo,
                          BobguiOrientation  orientation,
                          int             for_size,
                          int            *minimum,
                          int            *natural,
                          int            *minimum_baseline,
                          int            *natural_baseline)
{
  BobguiWidget *widget = bobgui_widget_get_parent (BOBGUI_WIDGET (gizmo));
  BobguiRange *range = BOBGUI_RANGE (widget);
  BobguiRangePrivate *priv = bobgui_range_get_instance_private (range);
  int min, nat;

  bobgui_widget_measure (priv->slider_widget,
                      orientation, -1,
                      minimum, natural,
                      NULL, NULL);

  if (priv->fill_widget)
    {
      bobgui_widget_measure (priv->fill_widget,
                          orientation, for_size,
                          &min, &nat,
                          NULL, NULL);
      *minimum = MAX (*minimum, min);
      *natural = MAX (*natural, nat);
    }

  if (priv->highlight_widget)
    {
      bobgui_widget_measure (priv->highlight_widget,
                          orientation, for_size,
                          &min, &nat,
                          NULL, NULL);
      *minimum = MAX (*minimum, min);
      *natural = MAX (*natural, nat);
    }
}

static void
bobgui_range_measure (BobguiWidget      *widget,
                   BobguiOrientation  orientation,
                   int             for_size,
                   int            *minimum,
                   int            *natural,
                   int            *minimum_baseline,
                   int            *natural_baseline)
{
  BobguiRange *range = BOBGUI_RANGE (widget);
  BobguiRangePrivate *priv = bobgui_range_get_instance_private (range);
  BobguiBorder border = { 0 };

  /* Measure the main box */
  bobgui_widget_measure (priv->trough_widget,
                      orientation,
                      -1,
                      minimum, natural,
                      NULL, NULL);

  if (BOBGUI_RANGE_GET_CLASS (range)->get_range_border)
    BOBGUI_RANGE_GET_CLASS (range)->get_range_border (range, &border);

  /* Add the border */
  if (orientation == BOBGUI_ORIENTATION_HORIZONTAL)
    {
      *minimum += border.left + border.right;
      *natural += border.left + border.right;
    }
  else
    {
      *minimum += border.top + border.bottom;
      *natural += border.top + border.bottom;
    }
}

static void
bobgui_range_allocate_trough (BobguiGizmo *gizmo,
                           int       width,
                           int       height,
                           int       baseline)
{
  BobguiWidget *widget = bobgui_widget_get_parent (BOBGUI_WIDGET (gizmo));
  BobguiRange *range = BOBGUI_RANGE (widget);
  BobguiRangePrivate *priv = bobgui_range_get_instance_private (range);
  BobguiAllocation slider_alloc;
  const double lower = bobgui_adjustment_get_lower (priv->adjustment);
  const double upper = bobgui_adjustment_get_upper (priv->adjustment);
  const double page_size = bobgui_adjustment_get_page_size (priv->adjustment);
  double value;

  /* Slider */
  bobgui_range_calc_marks (range);

  bobgui_range_compute_slider_position (range,
                                     bobgui_adjustment_get_value (priv->adjustment),
                                     &slider_alloc);

  bobgui_widget_size_allocate (priv->slider_widget, &slider_alloc, -1);
  priv->slider_x = slider_alloc.x;
  priv->slider_y = slider_alloc.y;

  if (lower == upper)
    value = 0;
  else
    value = (bobgui_adjustment_get_value (priv->adjustment) - lower) /
            (upper - page_size - lower);

  if (priv->show_fill_level &&
      upper - page_size - lower != 0)
    {
      double level, fill;
      BobguiAllocation fill_alloc;
      fill_alloc = (BobguiAllocation) {0, 0, width, height};

      level = CLAMP (priv->fill_level, lower, upper - page_size);
      fill = (level - lower) / (upper - lower - page_size);

      if (priv->orientation == BOBGUI_ORIENTATION_HORIZONTAL)
        {
          fill_alloc.width *= fill;

          if (should_invert (range))
            fill_alloc.x += width - fill_alloc.width;
        }
      else
        {
          fill_alloc.height *= fill;

          if (should_invert (range))
            fill_alloc.y += height - fill_alloc.height;
        }

      bobgui_widget_size_allocate (priv->fill_widget, &fill_alloc, -1);
    }

  if (priv->highlight_widget)
    {
      BobguiAllocation highlight_alloc;
      int min, nat;

      bobgui_widget_measure (priv->highlight_widget,
                          priv->orientation, -1,
                          &min, &nat,
                          NULL, NULL);

      if (priv->orientation == BOBGUI_ORIENTATION_HORIZONTAL)
        {
          highlight_alloc.y = 0;
          highlight_alloc.width = MAX (min, value * width);
          highlight_alloc.height = height;

          if (!should_invert (range))
            highlight_alloc.x = 0;
          else
            highlight_alloc.x = width - highlight_alloc.width;
        }
      else
        {
          highlight_alloc.x = 0;
          highlight_alloc.width = width;
          highlight_alloc.height = MAX (min, height * value);

          if (!should_invert (range))
            highlight_alloc.y = 0;
          else
            highlight_alloc.y = height - highlight_alloc.height;
        }

      bobgui_widget_size_allocate (priv->highlight_widget, &highlight_alloc, -1);
    }
}

/* Clamp dimensions and border inside allocation, such that we prefer
 * to take space from border not dimensions in all directions, and prefer to
 * give space to border over dimensions in one direction.
 */
static void
clamp_dimensions (int        range_width,
                  int        range_height,
                  int       *width,
                  int       *height,
                  BobguiBorder *border,
                  gboolean   border_expands_horizontally)
{
  int extra, shortage;

  /* Width */
  extra = range_width - border->left - border->right - *width;
  if (extra > 0)
    {
      if (border_expands_horizontally)
        {
          border->left += extra / 2;
          border->right += extra / 2 + extra % 2;
        }
      else
        {
          *width += extra;
        }
    }
  
  /* See if we can fit rect, if not kill the border */
  shortage = *width - range_width;
  if (shortage > 0)
    {
      *width = range_width;
      /* lose the border */
      border->left = 0;
      border->right = 0;
    }
  else
    {
      /* See if we can fit rect with borders */
      shortage = *width + border->left + border->right - range_width;
      if (shortage > 0)
        {
          /* Shrink borders */
          border->left -= shortage / 2;
          border->right -= shortage / 2 + shortage % 2;
        }
    }

  /* Height */
  extra = range_height - border->top - border->bottom - *height;
  if (extra > 0)
    {
      if (border_expands_horizontally)
        {
          /* don't expand border vertically */
          *height += extra;
        }
      else
        {
          border->top += extra / 2;
          border->bottom += extra / 2 + extra % 2;
        }
    }
  
  /* See if we can fit rect, if not kill the border */
  shortage = *height - range_height;
  if (shortage > 0)
    {
      *height = range_height;
      /* lose the border */
      border->top = 0;
      border->bottom = 0;
    }
  else
    {
      /* See if we can fit rect with borders */
      shortage = *height + border->top + border->bottom - range_height;
      if (shortage > 0)
        {
          /* Shrink borders */
          border->top -= shortage / 2;
          border->bottom -= shortage / 2 + shortage % 2;
        }
    }
}

static void
bobgui_range_size_allocate (BobguiWidget *widget,
                         int        width,
                         int        height,
                         int        baseline)
{
  BobguiRange *range = BOBGUI_RANGE (widget);
  BobguiRangePrivate *priv = bobgui_range_get_instance_private (range);
  BobguiBorder border = { 0 };
  BobguiAllocation box_alloc;
  int box_min_width, box_min_height;

  if (BOBGUI_RANGE_GET_CLASS (range)->get_range_border)
    BOBGUI_RANGE_GET_CLASS (range)->get_range_border (range, &border);

  bobgui_widget_measure (priv->trough_widget,
                      BOBGUI_ORIENTATION_HORIZONTAL, -1,
                      &box_min_width, NULL,
                      NULL, NULL);
  bobgui_widget_measure (priv->trough_widget,
                      BOBGUI_ORIENTATION_VERTICAL, -1,
                      &box_min_height, NULL,
                      NULL, NULL);

  if (priv->orientation == BOBGUI_ORIENTATION_VERTICAL)
    clamp_dimensions (width, height, &box_min_width, &box_min_height, &border, TRUE);
  else
    clamp_dimensions (width, height, &box_min_width, &box_min_height, &border, FALSE);

  box_alloc.x = border.left;
  box_alloc.y = border.top;
  box_alloc.width = box_min_width;
  box_alloc.height = box_min_height;

  bobgui_widget_size_allocate (priv->trough_widget, &box_alloc, -1);
}

static void
bobgui_range_unmap (BobguiWidget *widget)
{
  BobguiRange *range = BOBGUI_RANGE (widget);

  stop_scrolling (range);

  BOBGUI_WIDGET_CLASS (bobgui_range_parent_class)->unmap (widget);
}

static void
bobgui_range_direction_changed (BobguiWidget        *widget,
                             BobguiTextDirection  previous_direction)
{
  BobguiRange *range = BOBGUI_RANGE (widget);

  update_fill_position (range);
  update_highlight_position (range);

  BOBGUI_WIDGET_CLASS (bobgui_range_parent_class)->direction_changed (widget, previous_direction);
}

static void
bobgui_range_render_trough (BobguiGizmo    *gizmo,
                         BobguiSnapshot *snapshot)
{
  BobguiWidget *widget = bobgui_widget_get_parent (BOBGUI_WIDGET (gizmo));
  BobguiRange *range = BOBGUI_RANGE (widget);
  BobguiRangePrivate *priv = bobgui_range_get_instance_private (range);

  /* HACK: BobguiColorScale wants to draw its own trough
   * so we let it...
   */
  if (BOBGUI_IS_COLOR_SCALE (widget))
    {
      BobguiCssBoxes boxes;
      bobgui_css_boxes_init (&boxes, BOBGUI_WIDGET (gizmo));
      bobgui_snapshot_push_rounded_clip (snapshot, bobgui_css_boxes_get_padding_box (&boxes));
      bobgui_color_scale_snapshot_trough (BOBGUI_COLOR_SCALE (widget), snapshot,
                                       bobgui_widget_get_width (BOBGUI_WIDGET (gizmo)),
                                       bobgui_widget_get_height (BOBGUI_WIDGET (gizmo)));
      bobgui_snapshot_pop (snapshot);
    }

  if (priv->show_fill_level &&
      bobgui_adjustment_get_upper (priv->adjustment) - bobgui_adjustment_get_page_size (priv->adjustment) -
      bobgui_adjustment_get_lower (priv->adjustment) != 0)
    bobgui_widget_snapshot_child (BOBGUI_WIDGET (gizmo), priv->fill_widget, snapshot);

  if (priv->highlight_widget)
    {
      BobguiCssBoxes boxes;
      bobgui_css_boxes_init (&boxes, BOBGUI_WIDGET (gizmo));
      bobgui_snapshot_push_rounded_clip (snapshot, bobgui_css_boxes_get_border_box (&boxes));
      bobgui_widget_snapshot_child (BOBGUI_WIDGET (gizmo), priv->highlight_widget, snapshot);
      bobgui_snapshot_pop (snapshot);
    }

  bobgui_widget_snapshot_child (BOBGUI_WIDGET (gizmo), priv->slider_widget, snapshot);
}

static void
range_grab_add (BobguiRange  *range,
                BobguiWidget *location)
{
  BobguiRangePrivate *priv = bobgui_range_get_instance_private (range);

  /* Don't perform any GDK/BOBGUI grab here. Since a button
   * is down, there's an ongoing implicit grab on
   * the widget, which pretty much guarantees this
   * is the only widget receiving the pointer events.
   */
  priv->grab_location = location;

  bobgui_widget_add_css_class (BOBGUI_WIDGET (range), "dragging");
}

static void
update_zoom_state (BobguiRange *range,
                   gboolean  enabled)
{
  BobguiRangePrivate *priv = bobgui_range_get_instance_private (range);

  if (enabled)
    bobgui_widget_add_css_class (BOBGUI_WIDGET (range), "fine-tune");
  else
    bobgui_widget_remove_css_class (BOBGUI_WIDGET (range), "fine-tune");

  priv->zoom = enabled;
}

static void
range_grab_remove (BobguiRange *range)
{
  BobguiRangePrivate *priv = bobgui_range_get_instance_private (range);

  if (!priv->grab_location)
    return;

  priv->grab_location = NULL;

  update_zoom_state (range, FALSE);

  bobgui_widget_remove_css_class (BOBGUI_WIDGET (range), "dragging");
}

static BobguiScrollType
range_get_scroll_for_grab (BobguiRange *range)
{
  BobguiRangePrivate *priv = bobgui_range_get_instance_private (range);

  if (!priv->grab_location)
    return BOBGUI_SCROLL_NONE;

  /* In the trough */
  if (priv->grab_location == priv->trough_widget)
    {
      if (priv->trough_click_forward)
        return BOBGUI_SCROLL_PAGE_FORWARD;
      else
        return BOBGUI_SCROLL_PAGE_BACKWARD;
    }

  return BOBGUI_SCROLL_NONE;
}

static double
coord_to_value (BobguiRange *range,
                double    coord)
{
  BobguiRangePrivate *priv = bobgui_range_get_instance_private (range);
  double frac;
  double value;
  int     trough_length;
  int     slider_length;
  graphene_rect_t slider_bounds;

  if (!bobgui_widget_compute_bounds (priv->slider_widget, priv->slider_widget, &slider_bounds))
    graphene_rect_init (&slider_bounds, 0, 0, bobgui_widget_get_width (priv->trough_widget), bobgui_widget_get_height (priv->trough_widget));

  if (priv->orientation == BOBGUI_ORIENTATION_HORIZONTAL)
    {
      trough_length = bobgui_widget_get_width (priv->trough_widget);
      slider_length = slider_bounds.size.width;
    }
  else
    {
      trough_length = bobgui_widget_get_height (priv->trough_widget);
      slider_length = slider_bounds.size.height;
    }

  if (trough_length == slider_length)
    {
      frac = 1.0;
    }
  else
    {
      if (priv->slider_size_fixed)
        frac = CLAMP (coord / (double) trough_length, 0, 1);
      else
        frac = CLAMP (coord / (double) (trough_length - slider_length), 0, 1);
    }

  if (should_invert (range))
    frac = 1.0 - frac;

  value = bobgui_adjustment_get_lower (priv->adjustment) +
          frac * (bobgui_adjustment_get_upper (priv->adjustment) -
                  bobgui_adjustment_get_lower (priv->adjustment) -
                  bobgui_adjustment_get_page_size (priv->adjustment));
  return value;
}

static double
scroll_delta_to_value (BobguiRange      *range,
                       GdkScrollUnit  scroll_unit,
                       double         delta)
{
  BobguiRangePrivate *priv = bobgui_range_get_instance_private (range);

  if (scroll_unit == GDK_SCROLL_UNIT_WHEEL)
    return delta * bobgui_adjustment_get_page_increment (priv->adjustment);
  else if (scroll_unit == GDK_SCROLL_UNIT_SURFACE && BOBGUI_IS_SCALE (range))
    {
      double frac;
      int trough_length, slider_length;
      graphene_rect_t slider_bounds;

      if (!bobgui_widget_compute_bounds (priv->slider_widget, priv->slider_widget, &slider_bounds))
        {
          graphene_rect_init (&slider_bounds, 0, 0,
                              bobgui_widget_get_width (priv->trough_widget),
                              bobgui_widget_get_height (priv->trough_widget));
        }

      if (priv->orientation == BOBGUI_ORIENTATION_HORIZONTAL)
        {
          trough_length = bobgui_widget_get_width (priv->trough_widget);
          slider_length = slider_bounds.size.width;
        }
      else
        {
          trough_length = bobgui_widget_get_height (priv->trough_widget);
          slider_length = slider_bounds.size.height;
        }

      if (trough_length == slider_length)
        frac = 1.0;
      else if (priv->slider_size_fixed)
        frac = delta / (double) trough_length;
      else
        frac = delta / (double) (trough_length - slider_length);

      return frac * (bobgui_adjustment_get_upper (priv->adjustment) -
                     bobgui_adjustment_get_lower (priv->adjustment) -
                     bobgui_adjustment_get_page_size (priv->adjustment));
    }

  return delta;
}

static gboolean
bobgui_range_key_controller_key_pressed (BobguiEventControllerKey *controller,
                                      guint                  keyval,
                                      guint                  keycode,
                                      GdkModifierType        state,
                                      BobguiWidget             *widget)
{
  BobguiRange *range = BOBGUI_RANGE (widget);
  BobguiRangePrivate *priv = bobgui_range_get_instance_private (range);

  if (bobgui_gesture_is_active (priv->drag_gesture) &&
      keyval == GDK_KEY_Escape &&
      priv->grab_location != NULL)
    {
      stop_scrolling (range);

      return GDK_EVENT_STOP;
    }
  else if (priv->in_drag &&
           (keyval == GDK_KEY_Shift_L ||
            keyval == GDK_KEY_Shift_R))
    {
      graphene_rect_t slider_bounds;

      if (!bobgui_widget_compute_bounds (priv->slider_widget, priv->trough_widget, &slider_bounds))
        return GDK_EVENT_STOP;

      if (priv->orientation == BOBGUI_ORIENTATION_VERTICAL)
        priv->slide_initial_slider_position = slider_bounds.origin.y;
      else
        priv->slide_initial_slider_position = slider_bounds.origin.x;
      update_zoom_state (range, !priv->zoom);

      return GDK_EVENT_STOP;
    }

  return GDK_EVENT_PROPAGATE;
}

static void
update_initial_slider_position (BobguiRange *range,
                                double    x,
                                double    y)
{
  BobguiRangePrivate *priv = bobgui_range_get_instance_private (range);
  graphene_point_t p;

  if (!bobgui_widget_compute_point (BOBGUI_WIDGET (range), priv->trough_widget,
                                 &GRAPHENE_POINT_INIT (x, y), &p))
    graphene_point_init (&p, x, y);

  if (priv->orientation == BOBGUI_ORIENTATION_HORIZONTAL)
    {
      priv->slide_initial_slider_position = MAX (0, priv->slider_x);
      priv->slide_initial_coordinate_delta = p.x - priv->slide_initial_slider_position;
    }
  else
    {
      priv->slide_initial_slider_position = MAX (0, priv->slider_y);
      priv->slide_initial_coordinate_delta = p.y - priv->slide_initial_slider_position;
    }
}

static void
bobgui_range_long_press_gesture_pressed (BobguiGestureLongPress *gesture,
                                      double               x,
                                      double               y,
                                      BobguiRange            *range)
{
  BobguiRangePrivate *priv = bobgui_range_get_instance_private (range);
  BobguiWidget *mouse_location;

  mouse_location = bobgui_widget_pick (BOBGUI_WIDGET (range), x, y, BOBGUI_PICK_DEFAULT);

  if (mouse_location == priv->slider_widget && !priv->zoom)
    {
      update_initial_slider_position (range, x, y);
      update_zoom_state (range, TRUE);
    }
}

static void
bobgui_range_click_gesture_pressed (BobguiGestureClick *gesture,
                                 guint            n_press,
                                 double           x,
                                 double           y,
                                 BobguiRange        *range)
{
  BobguiWidget *widget = BOBGUI_WIDGET (range);
  BobguiRangePrivate *priv = bobgui_range_get_instance_private (range);
  GdkDevice *source_device;
  GdkEventSequence *sequence;
  GdkEvent *event;
  GdkInputSource source;
  gboolean primary_warps;
  gboolean shift_pressed;
  guint button;
  GdkModifierType state_mask;
  BobguiWidget *mouse_location;

  if (!bobgui_widget_has_focus (widget))
    bobgui_widget_grab_focus (widget);

  sequence = bobgui_gesture_single_get_current_sequence (BOBGUI_GESTURE_SINGLE (gesture));
  button = bobgui_gesture_single_get_current_button (BOBGUI_GESTURE_SINGLE (gesture));
  event = bobgui_gesture_get_last_event (BOBGUI_GESTURE (gesture), sequence);
  state_mask = gdk_event_get_modifier_state (event);
  shift_pressed = (state_mask & GDK_SHIFT_MASK) != 0;

  source_device = gdk_event_get_device ((GdkEvent *) event);
  source = gdk_device_get_source (source_device);

  g_object_get (bobgui_widget_get_settings (widget),
                "bobgui-primary-button-warps-slider", &primary_warps,
                NULL);

  mouse_location = bobgui_widget_pick (widget, x, y, 0);

  /* For the purposes of this function, we treat anything outside
   * the slider like a click on the trough
   */
  if (mouse_location != priv->slider_widget)
    mouse_location = priv->trough_widget;

  if (mouse_location == priv->slider_widget)
    {
      /* Shift-click in the slider = fine adjustment */
      if (shift_pressed)
        update_zoom_state (range, TRUE);

      update_initial_slider_position (range, x, y);
      range_grab_add (range, priv->slider_widget);
    }
  else if (mouse_location == priv->trough_widget &&
           (source == GDK_SOURCE_TOUCHSCREEN ||
            (primary_warps && !shift_pressed && button == GDK_BUTTON_PRIMARY) ||
            (!primary_warps && shift_pressed && button == GDK_BUTTON_PRIMARY) ||
            (!primary_warps && button == GDK_BUTTON_MIDDLE)))
    {
      graphene_point_t p;
      graphene_rect_t slider_bounds;

      if (!bobgui_widget_compute_point (priv->trough_widget, widget,
                                     &GRAPHENE_POINT_INIT (priv->slider_x, priv->slider_y),
                                     &p))
        graphene_point_init (&p, priv->slider_x, priv->slider_y);

      /* If we aren't fixed, center on the slider. I.e. if this is not a scale... */
      if (!priv->slider_size_fixed &&
          bobgui_widget_compute_bounds (priv->slider_widget, priv->slider_widget, &slider_bounds))
        {
          p.x += (slider_bounds.size.width  / 2);
          p.y += (slider_bounds.size.height / 2);
        }

      update_initial_slider_position (range, p.x, p.y);

      range_grab_add (range, priv->slider_widget);

      update_slider_position (range, x, y);
    }
  else if (mouse_location == priv->trough_widget &&
           ((primary_warps && shift_pressed && button == GDK_BUTTON_PRIMARY) ||
            (!primary_warps && !shift_pressed && button == GDK_BUTTON_PRIMARY) ||
            (primary_warps && button == GDK_BUTTON_MIDDLE)))
    {
      /* jump by pages */
      BobguiScrollType scroll;
      double click_value;

      click_value = coord_to_value (range,
                                    priv->orientation == BOBGUI_ORIENTATION_VERTICAL ?
                                    y : x);

      priv->trough_click_forward = click_value > bobgui_adjustment_get_value (priv->adjustment);
      range_grab_add (range, priv->trough_widget);

      scroll = range_get_scroll_for_grab (range);
      bobgui_range_add_step_timer (range, scroll);
    }
  else if (mouse_location == priv->trough_widget &&
           button == GDK_BUTTON_SECONDARY)
    {
      /* autoscroll */
      double click_value;

      click_value = coord_to_value (range,
                                    priv->orientation == BOBGUI_ORIENTATION_VERTICAL ?
                                    y : x);

      priv->trough_click_forward = click_value > bobgui_adjustment_get_value (priv->adjustment);
      range_grab_add (range, priv->trough_widget);

      remove_autoscroll (range);
      priv->autoscroll_mode = priv->trough_click_forward ? BOBGUI_SCROLL_END : BOBGUI_SCROLL_START;
      add_autoscroll (range);
    }

  if (priv->grab_location == priv->slider_widget);
    /* leave it to ::drag-begin to claim the sequence */
  else if (priv->grab_location != NULL)
    bobgui_gesture_set_state (BOBGUI_GESTURE (gesture), BOBGUI_EVENT_SEQUENCE_CLAIMED);
}

/* During a slide, move the slider as required given new mouse position */
static void
update_slider_position (BobguiRange *range,
                        int       mouse_x,
                        int       mouse_y)
{
  BobguiRangePrivate *priv = bobgui_range_get_instance_private (range);
  graphene_rect_t trough_bounds;
  double delta;
  double c;
  double new_value;
  gboolean handled;
  double next_value;
  double mark_value;
  double mark_delta;
  double zoom;
  int i;
  graphene_point_t p;

  if (!bobgui_widget_compute_point(BOBGUI_WIDGET (range), priv->trough_widget,
                                &GRAPHENE_POINT_INIT (mouse_x, mouse_y), &p))
    graphene_point_init (&p, mouse_x, mouse_y);

  if (priv->zoom &&
      bobgui_widget_compute_bounds (priv->trough_widget, priv->trough_widget, &trough_bounds))
    {
      zoom = MIN(1.0, (priv->orientation == BOBGUI_ORIENTATION_VERTICAL ?
                       trough_bounds.size.height : trough_bounds.size.width) /
                       (bobgui_adjustment_get_upper (priv->adjustment) -
                        bobgui_adjustment_get_lower (priv->adjustment) -
                        bobgui_adjustment_get_page_size (priv->adjustment)));

      /* the above is ineffective for scales, so just set a zoom factor */
      if (zoom == 1.0)
        zoom = 0.25;
    }
  else
    zoom = 1.0;

  /* recalculate the initial position from the current position */
  if (priv->slide_initial_slider_position == -1)
    {
      graphene_rect_t slider_bounds;
      double zoom_divisor;

      if (!bobgui_widget_compute_bounds (priv->slider_widget, BOBGUI_WIDGET (range), &slider_bounds))
        graphene_rect_init (&slider_bounds, 0, 0, 0, 0);

      if (zoom == 1.0)
        zoom_divisor = 1.0;
      else
        zoom_divisor = zoom - 1.0;

      if (priv->orientation == BOBGUI_ORIENTATION_VERTICAL)
        priv->slide_initial_slider_position = (zoom * (p.y - priv->slide_initial_coordinate_delta) - slider_bounds.origin.y) / zoom_divisor;
      else
        priv->slide_initial_slider_position = (zoom * (p.x - priv->slide_initial_coordinate_delta) - slider_bounds.origin.x) / zoom_divisor;
    }

  if (priv->orientation == BOBGUI_ORIENTATION_VERTICAL)
    delta = p.y - (priv->slide_initial_coordinate_delta + priv->slide_initial_slider_position);
  else
    delta = p.x - (priv->slide_initial_coordinate_delta + priv->slide_initial_slider_position);

  c = priv->slide_initial_slider_position + zoom * delta;

  new_value = coord_to_value (range, c);
  next_value = coord_to_value (range, c + 1);
  mark_delta = fabs (next_value - new_value);

  for (i = 0; i < priv->n_marks; i++)
    {
      mark_value = priv->marks[i];

      if (fabs (bobgui_adjustment_get_value (priv->adjustment) - mark_value) < 3 * mark_delta)
        {
          if (fabs (new_value - mark_value) < MARK_SNAP_LENGTH * mark_delta)
            {
              new_value = mark_value;
              break;
            }
        }
    }

  g_signal_emit (range, signals[CHANGE_VALUE], 0, BOBGUI_SCROLL_JUMP, new_value, &handled);
}

static void
remove_autoscroll (BobguiRange *range)
{
  BobguiRangePrivate *priv = bobgui_range_get_instance_private (range);

  if (priv->autoscroll_id)
    {
      bobgui_widget_remove_tick_callback (BOBGUI_WIDGET (range),
                                       priv->autoscroll_id);
      priv->autoscroll_id = 0;
    }

  /* unset initial position so it can be calculated */
  priv->slide_initial_slider_position = -1;

  priv->autoscroll_mode = BOBGUI_SCROLL_NONE;
}

static gboolean
autoscroll_cb (BobguiWidget     *widget,
               GdkFrameClock *frame_clock,
               gpointer       data)
{
  BobguiRange *range = BOBGUI_RANGE (data);
  BobguiRangePrivate *priv = bobgui_range_get_instance_private (range);
  BobguiAdjustment *adj = priv->adjustment;
  double increment;
  double value;
  gboolean handled;
  double step, page;

  step = bobgui_adjustment_get_step_increment (adj);
  page = bobgui_adjustment_get_page_increment (adj);

  switch ((guint) priv->autoscroll_mode)
    {
    case BOBGUI_SCROLL_STEP_FORWARD:
      increment = step / AUTOSCROLL_FACTOR;
      break;
    case BOBGUI_SCROLL_PAGE_FORWARD:
      increment = page / AUTOSCROLL_FACTOR;
      break;
    case BOBGUI_SCROLL_STEP_BACKWARD:
      increment = - step / AUTOSCROLL_FACTOR;
      break;
    case BOBGUI_SCROLL_PAGE_BACKWARD:
      increment = - page / AUTOSCROLL_FACTOR;
      break;
    case BOBGUI_SCROLL_START:
    case BOBGUI_SCROLL_END:
      {
        double x, y;
        double distance, t;

        /* Vary scrolling speed from slow (ie step) to fast (2 * page),
         * based on the distance of the pointer from the widget. We start
         * speeding up if the pointer moves at least 20 pixels away, and
         * we reach maximum speed when it is 220 pixels away.
         */
        if (!bobgui_gesture_drag_get_offset (BOBGUI_GESTURE_DRAG (priv->drag_gesture), &x, &y))
          {
            x = 0.0;
            y = 0.0;
          }
        if (bobgui_orientable_get_orientation (BOBGUI_ORIENTABLE (range)) == BOBGUI_ORIENTATION_HORIZONTAL)
          distance = fabs (y);
        else
          distance = fabs (x);
        distance = CLAMP (distance - 20, 0.0, 200);
        t = distance / 100.0;
        step = (1 - t) * step + t * page;
        if (priv->autoscroll_mode == BOBGUI_SCROLL_END)
          increment = step / AUTOSCROLL_FACTOR;
        else
          increment = - step / AUTOSCROLL_FACTOR;
      }
      break;
    default:
      g_assert_not_reached ();
      break;
    }

  value = bobgui_adjustment_get_value (adj);
  value += increment;

  g_signal_emit (range, signals[CHANGE_VALUE], 0, BOBGUI_SCROLL_JUMP, value, &handled);

  return G_SOURCE_CONTINUE;
}

static void
add_autoscroll (BobguiRange *range)
{
  BobguiRangePrivate *priv = bobgui_range_get_instance_private (range);

  if (priv->autoscroll_id != 0 ||
      priv->autoscroll_mode == BOBGUI_SCROLL_NONE)
    return;

  priv->autoscroll_id = bobgui_widget_add_tick_callback (BOBGUI_WIDGET (range),
                                                      autoscroll_cb, range, NULL);
}

static void
stop_scrolling (BobguiRange *range)
{
  range_grab_remove (range);
  bobgui_range_remove_step_timer (range);
  remove_autoscroll (range);
}

static void
bobgui_range_scroll_controller_scroll_begin (BobguiEventControllerScroll *scroll,
                                          BobguiRange                 *range)
{
  BobguiRangePrivate *priv = bobgui_range_get_instance_private (range);

  priv->in_scroll = TRUE;
  priv->initial_scroll_value = bobgui_adjustment_get_value (priv->adjustment);
  priv->scroll_delta_accum = 0;
}

static void
bobgui_range_scroll_controller_scroll_end (BobguiEventControllerScroll *scroll,
                                        BobguiRange                 *range)
{
  BobguiRangePrivate *priv = bobgui_range_get_instance_private (range);

  priv->in_scroll = FALSE;
}

static gboolean
bobgui_range_scroll_controller_scroll (BobguiEventControllerScroll *scroll,
                                    double                    dx,
                                    double                    dy,
                                    BobguiRange                 *range)
{
  BobguiRangePrivate *priv = bobgui_range_get_instance_private (range);
  double delta, value;
  gboolean handled;
  BobguiOrientation move_orientation;
  GdkScrollUnit scroll_unit;

  scroll_unit = bobgui_event_controller_scroll_get_unit (scroll);

  if (priv->orientation == BOBGUI_ORIENTATION_HORIZONTAL &&
      scroll_unit == GDK_SCROLL_UNIT_SURFACE)
    {
      move_orientation = BOBGUI_ORIENTATION_HORIZONTAL;
      delta = dx;
    }
  else
    {
      move_orientation = BOBGUI_ORIENTATION_VERTICAL;
      delta = dy;
    }

  delta = scroll_delta_to_value (range, scroll_unit, delta);

  if (delta != 0 && should_invert_move (range, move_orientation))
    delta = - delta;

  if (priv->in_scroll)
    {
      priv->scroll_delta_accum += delta;
      value = priv->initial_scroll_value + priv->scroll_delta_accum;
    }
  else
    value = bobgui_adjustment_get_value (priv->adjustment) + delta;

  g_signal_emit (range, signals[CHANGE_VALUE], 0,
                 BOBGUI_SCROLL_JUMP, value,
                 &handled);

  return GDK_EVENT_STOP;
}

static void
update_autoscroll_mode (BobguiRange *range,
                        int       mouse_x,
                        int       mouse_y)
{
  BobguiRangePrivate *priv = bobgui_range_get_instance_private (range);
  BobguiScrollType mode = BOBGUI_SCROLL_NONE;

  if (priv->zoom)
    {
      int width, height;
      int size, pos;

      width = bobgui_widget_get_width (BOBGUI_WIDGET (range));
      height = bobgui_widget_get_height (BOBGUI_WIDGET (range));

      if (priv->orientation == BOBGUI_ORIENTATION_VERTICAL)
        {
          size = height;
          pos = mouse_y;
        }
      else
        {
          size = width;
          pos = mouse_x;
        }

      if (pos < SCROLL_EDGE_SIZE)
        mode = should_invert (range) ? BOBGUI_SCROLL_STEP_FORWARD : BOBGUI_SCROLL_STEP_BACKWARD;
      else if (pos > (size - SCROLL_EDGE_SIZE))
        mode = should_invert (range) ? BOBGUI_SCROLL_STEP_BACKWARD : BOBGUI_SCROLL_STEP_FORWARD;
    }

  if (mode != priv->autoscroll_mode)
    {
      remove_autoscroll (range);
      priv->autoscroll_mode = mode;
      add_autoscroll (range);
    }
}

static void
bobgui_range_drag_gesture_update (BobguiGestureDrag *gesture,
                               double          offset_x,
                               double          offset_y,
                               BobguiRange       *range)
{
  BobguiRangePrivate *priv = bobgui_range_get_instance_private (range);
  double start_x, start_y;

  if (priv->grab_location == priv->slider_widget)
    {
      int mouse_x, mouse_y;

      bobgui_gesture_drag_get_start_point (gesture, &start_x, &start_y);
      mouse_x = start_x + offset_x;
      mouse_y = start_y + offset_y;
      priv->in_drag = TRUE;
      update_autoscroll_mode (range, mouse_x, mouse_y);

      if (priv->autoscroll_mode == BOBGUI_SCROLL_NONE)
        update_slider_position (range, mouse_x, mouse_y);
    }
}

static void
bobgui_range_drag_gesture_begin (BobguiGestureDrag *gesture,
                              double          offset_x,
                              double          offset_y,
                              BobguiRange       *range)
{
  BobguiRangePrivate *priv = bobgui_range_get_instance_private (range);

  if (priv->grab_location == priv->slider_widget)
    bobgui_gesture_set_state (priv->drag_gesture, BOBGUI_EVENT_SEQUENCE_CLAIMED);
}

static void
bobgui_range_drag_gesture_end (BobguiGestureDrag *gesture,
                            double          offset_x,
                            double          offset_y,
                            BobguiRange       *range)
{
  BobguiRangePrivate *priv = bobgui_range_get_instance_private (range);

  priv->in_drag = FALSE;
  stop_scrolling (range);
}

static void
bobgui_range_adjustment_changed (BobguiAdjustment *adjustment,
                              gpointer       data)
{
  BobguiRange *range = BOBGUI_RANGE (data);
  BobguiRangePrivate *priv = bobgui_range_get_instance_private (range);
  double upper = bobgui_adjustment_get_upper (priv->adjustment);
  double lower = bobgui_adjustment_get_lower (priv->adjustment);

  bobgui_widget_set_visible (priv->slider_widget, upper != lower || !BOBGUI_IS_SCALE (range));

  bobgui_widget_queue_allocate (priv->trough_widget);

  update_accessible_range (range);

  /* Note that we don't round off to priv->round_digits here.
   * that's because it's really broken to change a value
   * in response to a change signal on that value; round_digits
   * is therefore defined to be a filter on what the BobguiRange
   * can input into the adjustment, not a filter that the BobguiRange
   * will enforce on the adjustment.
   */
}

static void
bobgui_range_adjustment_value_changed (BobguiAdjustment *adjustment,
				    gpointer       data)
{
  BobguiRange *range = BOBGUI_RANGE (data);
  BobguiRangePrivate *priv = bobgui_range_get_instance_private (range);

  /* Note that we don't round off to priv->round_digits here.
   * that's because it's really broken to change a value
   * in response to a change signal on that value; round_digits
   * is therefore defined to be a filter on what the BobguiRange
   * can input into the adjustment, not a filter that the BobguiRange
   * will enforce on the adjustment.
   */

  g_signal_emit (range, signals[VALUE_CHANGED], 0);

  bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (range),
                                  BOBGUI_ACCESSIBLE_PROPERTY_VALUE_NOW, bobgui_adjustment_get_value (adjustment),
                                  -1);

  bobgui_widget_queue_allocate (priv->trough_widget);
}

static void
apply_marks (BobguiRange *range, 
             double    oldval,
             double   *newval)
{
  BobguiRangePrivate *priv = bobgui_range_get_instance_private (range);
  int i;
  double mark;

  for (i = 0; i < priv->n_marks; i++)
    {
      mark = priv->marks[i];
      if ((oldval < mark && mark < *newval) ||
          (oldval > mark && mark > *newval))
        {
          *newval = mark;
          return;
        }
    }
}

static void
step_back (BobguiRange *range)
{
  BobguiRangePrivate *priv = bobgui_range_get_instance_private (range);
  double newval;
  gboolean handled;

  newval = bobgui_adjustment_get_value (priv->adjustment) - bobgui_adjustment_get_step_increment (priv->adjustment);
  apply_marks (range, bobgui_adjustment_get_value (priv->adjustment), &newval);
  g_signal_emit (range, signals[CHANGE_VALUE], 0,
                 BOBGUI_SCROLL_STEP_BACKWARD, newval, &handled);
}

static void
step_forward (BobguiRange *range)
{
  BobguiRangePrivate *priv = bobgui_range_get_instance_private (range);
  double newval;
  gboolean handled;

  newval = bobgui_adjustment_get_value (priv->adjustment) + bobgui_adjustment_get_step_increment (priv->adjustment);
  apply_marks (range, bobgui_adjustment_get_value (priv->adjustment), &newval);
  g_signal_emit (range, signals[CHANGE_VALUE], 0,
                 BOBGUI_SCROLL_STEP_FORWARD, newval, &handled);
}


static void
page_back (BobguiRange *range)
{
  BobguiRangePrivate *priv = bobgui_range_get_instance_private (range);
  double newval;
  gboolean handled;

  newval = bobgui_adjustment_get_value (priv->adjustment) - bobgui_adjustment_get_page_increment (priv->adjustment);
  apply_marks (range, bobgui_adjustment_get_value (priv->adjustment), &newval);
  g_signal_emit (range, signals[CHANGE_VALUE], 0,
                 BOBGUI_SCROLL_PAGE_BACKWARD, newval, &handled);
}

static void
page_forward (BobguiRange *range)
{
  BobguiRangePrivate *priv = bobgui_range_get_instance_private (range);
  double newval;
  gboolean handled;

  newval = bobgui_adjustment_get_value (priv->adjustment) + bobgui_adjustment_get_page_increment (priv->adjustment);
  apply_marks (range, bobgui_adjustment_get_value (priv->adjustment), &newval);
  g_signal_emit (range, signals[CHANGE_VALUE], 0,
                 BOBGUI_SCROLL_PAGE_FORWARD, newval, &handled);
}

static void
scroll_begin (BobguiRange *range)
{
  BobguiRangePrivate *priv = bobgui_range_get_instance_private (range);
  gboolean handled;

  g_signal_emit (range, signals[CHANGE_VALUE], 0,
                 BOBGUI_SCROLL_START, bobgui_adjustment_get_lower (priv->adjustment),
                 &handled);
}

static void
scroll_end (BobguiRange *range)
{
  BobguiRangePrivate *priv = bobgui_range_get_instance_private (range);
  double newval;
  gboolean handled;

  newval = bobgui_adjustment_get_upper (priv->adjustment) - bobgui_adjustment_get_page_size (priv->adjustment);
  g_signal_emit (range, signals[CHANGE_VALUE], 0, BOBGUI_SCROLL_END, newval,
                 &handled);
}

static gboolean
bobgui_range_scroll (BobguiRange     *range,
                  BobguiScrollType scroll)
{
  BobguiRangePrivate *priv = bobgui_range_get_instance_private (range);
  double old_value = bobgui_adjustment_get_value (priv->adjustment);

  switch (scroll)
    {
    case BOBGUI_SCROLL_STEP_LEFT:
      if (should_invert_move (range, BOBGUI_ORIENTATION_HORIZONTAL))
        step_forward (range);
      else
        step_back (range);
      break;
                    
    case BOBGUI_SCROLL_STEP_UP:
      if (should_invert_move (range, BOBGUI_ORIENTATION_VERTICAL))
        step_forward (range);
      else
        step_back (range);
      break;

    case BOBGUI_SCROLL_STEP_RIGHT:
      if (should_invert_move (range, BOBGUI_ORIENTATION_HORIZONTAL))
        step_back (range);
      else
        step_forward (range);
      break;
                    
    case BOBGUI_SCROLL_STEP_DOWN:
      if (should_invert_move (range, BOBGUI_ORIENTATION_VERTICAL))
        step_back (range);
      else
        step_forward (range);
      break;
                  
    case BOBGUI_SCROLL_STEP_BACKWARD:
      step_back (range);
      break;
                  
    case BOBGUI_SCROLL_STEP_FORWARD:
      step_forward (range);
      break;

    case BOBGUI_SCROLL_PAGE_LEFT:
      if (should_invert_move (range, BOBGUI_ORIENTATION_HORIZONTAL))
        page_forward (range);
      else
        page_back (range);
      break;
                    
    case BOBGUI_SCROLL_PAGE_UP:
      if (should_invert_move (range, BOBGUI_ORIENTATION_VERTICAL))
        page_forward (range);
      else
        page_back (range);
      break;

    case BOBGUI_SCROLL_PAGE_RIGHT:
      if (should_invert_move (range, BOBGUI_ORIENTATION_HORIZONTAL))
        page_back (range);
      else
        page_forward (range);
      break;
                    
    case BOBGUI_SCROLL_PAGE_DOWN:
      if (should_invert_move (range, BOBGUI_ORIENTATION_VERTICAL))
        page_back (range);
      else
        page_forward (range);
      break;
                  
    case BOBGUI_SCROLL_PAGE_BACKWARD:
      page_back (range);
      break;
                  
    case BOBGUI_SCROLL_PAGE_FORWARD:
      page_forward (range);
      break;

    case BOBGUI_SCROLL_START:
      scroll_begin (range);
      break;

    case BOBGUI_SCROLL_END:
      scroll_end (range);
      break;

    case BOBGUI_SCROLL_JUMP:
    case BOBGUI_SCROLL_NONE:
    default:
      break;
    }

  return bobgui_adjustment_get_value (priv->adjustment) != old_value;
}

static void
bobgui_range_move_slider (BobguiRange     *range,
                       BobguiScrollType scroll)
{
  if (! bobgui_range_scroll (range, scroll))
    bobgui_widget_error_bell (BOBGUI_WIDGET (range));
}

static void
bobgui_range_compute_slider_position (BobguiRange     *range,
                                   double        adjustment_value,
                                   GdkRectangle *slider_rect)
{
  BobguiRangePrivate *priv = bobgui_range_get_instance_private (range);
  const double upper = bobgui_adjustment_get_upper (priv->adjustment);
  const double lower = bobgui_adjustment_get_lower (priv->adjustment);
  const double page_size = bobgui_adjustment_get_page_size (priv->adjustment);
  int trough_width, trough_height;
  int slider_width, slider_height;

  bobgui_widget_measure (priv->slider_widget,
                      BOBGUI_ORIENTATION_HORIZONTAL, -1,
                      &slider_width, NULL,
                      NULL, NULL);
  bobgui_widget_measure (priv->slider_widget,
                      BOBGUI_ORIENTATION_VERTICAL, slider_width,
                      &slider_height, NULL,
                      NULL, NULL);

  trough_width = bobgui_widget_get_width (priv->trough_widget);
  trough_height = bobgui_widget_get_height (priv->trough_widget);

  if (priv->orientation == BOBGUI_ORIENTATION_VERTICAL)
    {
      int y, height;

      slider_rect->x = (int) floor ((trough_width - slider_width) / 2);
      slider_rect->width = slider_width;

      /* slider height is the fraction (page_size /
       * total_adjustment_range) times the trough height in pixels
       */

      if (upper - lower != 0)
        height = trough_height * (page_size / (upper - lower));
      else
        height = slider_height;

      if (height < slider_height ||
          priv->slider_size_fixed)
        height = slider_height;

      height = MIN (height, trough_height);

      if (upper - lower - page_size != 0)
        y = (trough_height - height) * ((adjustment_value - lower)  / (upper - lower - page_size));
      else
        y = 0;

      y = CLAMP (y, 0, trough_height);

      if (should_invert (range))
        y = trough_height - y - height;

      slider_rect->y = y;
      slider_rect->height = height;
    }
  else
    {
      int x, width;

      slider_rect->y = (int) floor ((trough_height - slider_height) / 2);
      slider_rect->height = slider_height;

      /* slider width is the fraction (page_size /
       * total_adjustment_range) times the trough width in pixels
       */

      if (upper - lower != 0)
        width = trough_width * (page_size / (upper - lower));
      else
        width = slider_width;

      if (width < slider_width ||
          priv->slider_size_fixed)
        width = slider_width;

      width = MIN (width, trough_width);

      if (upper - lower - page_size != 0)
        x = (trough_width - width) * ((adjustment_value - lower) / (upper - lower - page_size));
      else
        x = 0;

      x = CLAMP (x, 0, trough_width);

      if (should_invert (range))
        x = trough_width - x - width;

      slider_rect->x = x;
      slider_rect->width = width;
    }
}

static void
bobgui_range_calc_marks (BobguiRange *range)
{
  BobguiRangePrivate *priv = bobgui_range_get_instance_private (range);
  GdkRectangle slider;
  graphene_point_t p;
  int i;

  for (i = 0; i < priv->n_marks; i++)
    {
      bobgui_range_compute_slider_position (range, priv->marks[i], &slider);
      if (!bobgui_widget_compute_point (priv->trough_widget, BOBGUI_WIDGET (range),
                                     &GRAPHENE_POINT_INIT (slider.x, slider.y), &p))
        graphene_point_init (&p, slider.x, slider.y);

      if (priv->orientation == BOBGUI_ORIENTATION_HORIZONTAL)
        priv->mark_pos[i] = p.x + slider.width / 2;
      else
        priv->mark_pos[i] = p.y + slider.height / 2;
    }
}

static gboolean
bobgui_range_real_change_value (BobguiRange      *range,
                             BobguiScrollType  scroll,
                             double         value)
{
  BobguiRangePrivate *priv = bobgui_range_get_instance_private (range);

  /* potentially adjust the bounds _before_ we clamp */
  g_signal_emit (range, signals[ADJUST_BOUNDS], 0, value);

  if (priv->restrict_to_fill_level)
    value = MIN (value, MAX (bobgui_adjustment_get_lower (priv->adjustment),
                             priv->fill_level));

  value = CLAMP (value, bobgui_adjustment_get_lower (priv->adjustment),
                 (bobgui_adjustment_get_upper (priv->adjustment) - bobgui_adjustment_get_page_size (priv->adjustment)));

  if (priv->round_digits >= 0)
    {
      double power;
      int i;

      i = priv->round_digits;
      power = 1;
      while (i--)
        power *= 10;

      value = floor ((value * power) + 0.5) / power;
    }

  if (priv->in_drag || priv->autoscroll_id)
    bobgui_adjustment_set_value (priv->adjustment, value);
  else
    bobgui_adjustment_animate_to_value (priv->adjustment, value);

  return FALSE;
}

struct _BobguiRangeStepTimer
{
  guint timeout_id;
  BobguiScrollType step;
};

static gboolean
second_timeout (gpointer data)
{
  BobguiRange *range = BOBGUI_RANGE (data);
  BobguiRangePrivate *priv = bobgui_range_get_instance_private (range);

  bobgui_range_scroll (range, priv->timer->step);

  return G_SOURCE_CONTINUE;
}

static gboolean
initial_timeout (gpointer data)
{
  BobguiRange *range = BOBGUI_RANGE (data);
  BobguiRangePrivate *priv = bobgui_range_get_instance_private (range);

  priv->timer->timeout_id = g_timeout_add (TIMEOUT_REPEAT, second_timeout, range);
  gdk_source_set_static_name_by_id (priv->timer->timeout_id, "[bobgui] second_timeout");
  return G_SOURCE_REMOVE;
}

static void
bobgui_range_add_step_timer (BobguiRange      *range,
                          BobguiScrollType  step)
{
  BobguiRangePrivate *priv = bobgui_range_get_instance_private (range);

  g_return_if_fail (priv->timer == NULL);
  g_return_if_fail (step != BOBGUI_SCROLL_NONE);

  priv->timer = g_new (BobguiRangeStepTimer, 1);

  priv->timer->timeout_id = g_timeout_add (TIMEOUT_INITIAL, initial_timeout, range);
  gdk_source_set_static_name_by_id (priv->timer->timeout_id, "[bobgui] initial_timeout");
  priv->timer->step = step;

  bobgui_range_scroll (range, priv->timer->step);
}

static void
bobgui_range_remove_step_timer (BobguiRange *range)
{
  BobguiRangePrivate *priv = bobgui_range_get_instance_private (range);

  if (priv->timer)
    {
      if (priv->timer->timeout_id != 0)
        g_source_remove (priv->timer->timeout_id);

      g_free (priv->timer);

      priv->timer = NULL;
    }
}

void
_bobgui_range_set_has_origin (BobguiRange *range,
                           gboolean  has_origin)
{
  BobguiRangePrivate *priv = bobgui_range_get_instance_private (range);

  if (has_origin)
    {
      priv->highlight_widget = bobgui_gizmo_new ("highlight", NULL, NULL, NULL, NULL, NULL, NULL);
      bobgui_widget_insert_before (priv->highlight_widget, priv->trough_widget, priv->slider_widget);

      update_highlight_position (range);
    }
  else
    {
      g_clear_pointer (&priv->highlight_widget, bobgui_widget_unparent);
    }
}

gboolean
_bobgui_range_get_has_origin (BobguiRange *range)
{
  BobguiRangePrivate *priv = bobgui_range_get_instance_private (range);

  return priv->highlight_widget != NULL;
}

void
_bobgui_range_set_stop_values (BobguiRange *range,
                            double   *values,
                            int       n_values)
{
  BobguiRangePrivate *priv = bobgui_range_get_instance_private (range);
  int i;

  g_free (priv->marks);
  priv->marks = g_new (double, n_values);

  g_free (priv->mark_pos);
  priv->mark_pos = g_new (int, n_values);

  priv->n_marks = n_values;

  for (i = 0; i < n_values; i++) 
    priv->marks[i] = values[i];

  bobgui_range_calc_marks (range);
}

int
_bobgui_range_get_stop_positions (BobguiRange  *range,
                               int      **values)
{
  BobguiRangePrivate *priv = bobgui_range_get_instance_private (range);

  bobgui_range_calc_marks (range);

  if (values)
    *values = g_memdup2 (priv->mark_pos, priv->n_marks * sizeof (int));

  return priv->n_marks;
}

/**
 * bobgui_range_set_round_digits:
 * @range: a `BobguiRange`
 * @round_digits: the precision in digits, or -1
 *
 * Sets the number of digits to round the value to when
 * it changes.
 *
 * See [signal@Bobgui.Range::change-value].
 */
void
bobgui_range_set_round_digits (BobguiRange *range,
                            int       round_digits)
{
  BobguiRangePrivate *priv = bobgui_range_get_instance_private (range);

  g_return_if_fail (BOBGUI_IS_RANGE (range));
  g_return_if_fail (round_digits >= -1);

  if (priv->round_digits != round_digits)
    {
      priv->round_digits = round_digits;
      g_object_notify_by_pspec (G_OBJECT (range), properties[PROP_ROUND_DIGITS]);
    }
}

/**
 * bobgui_range_get_round_digits:
 * @range: a `BobguiRange`
 *
 * Gets the number of digits to round the value to when
 * it changes.
 *
 * See [signal@Bobgui.Range::change-value].
 *
 * Returns: the number of digits to round to
 */
int
bobgui_range_get_round_digits (BobguiRange *range)
{
  BobguiRangePrivate *priv = bobgui_range_get_instance_private (range);

  g_return_val_if_fail (BOBGUI_IS_RANGE (range), -1);

  return priv->round_digits;
}

BobguiWidget *
bobgui_range_get_slider_widget (BobguiRange *range)
{
  BobguiRangePrivate *priv = bobgui_range_get_instance_private (range);

  return priv->slider_widget;
}

BobguiWidget *
bobgui_range_get_trough_widget (BobguiRange *range)
{
  BobguiRangePrivate *priv = bobgui_range_get_instance_private (range);

  return priv->trough_widget;
}

void
bobgui_range_start_autoscroll (BobguiRange      *range,
                            BobguiScrollType  scroll_type)
{
  BobguiRangePrivate *priv = bobgui_range_get_instance_private (range);

  remove_autoscroll (range);
  priv->autoscroll_mode = scroll_type;
  add_autoscroll (range);
}

void
bobgui_range_stop_autoscroll (BobguiRange *range)
{
  remove_autoscroll (range);
}
