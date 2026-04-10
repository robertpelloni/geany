/* BOBGUI - The Bobgui Framework
 *
 * Copyright (C) 2010  Intel Corporation
 * Copyright (C) 2010  RedHat, Inc.
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
 *
 * Author:
 *      Emmanuele Bassi <ebassi@linux.intel.com>
 *      Matthias Clasen <mclasen@redhat.com>
 *
 * Based on similar code from Mx.
 */

/**
 * BobguiSwitch:
 *
 * Shows a "light switch" that has two states: on or off.
 *
 * <picture>
 *   <source srcset="switch-dark.png" media="(prefers-color-scheme: dark)">
 *   <img alt="An example BobguiSwitch" src="switch.png">
 * </picture>
 *
 * The user can control which state should be active by clicking the
 * empty area, or by dragging the slider.
 *
 * `BobguiSwitch` can also express situations where the underlying state changes
 * with a delay. In this case, the slider position indicates the user's recent
 * change (represented by the [property@Bobgui.Switch:active] property), while the
 * trough color indicates the present underlying state (represented by the
 * [property@Bobgui.Switch:state] property).
 *
 * <picture>
 *   <source srcset="switch-state-dark.png" media="(prefers-color-scheme: dark)">
 *   <img alt="BobguiSwitch with delayed state change" src="switch-state.png">
 * </picture>
 *
 * See [signal@Bobgui.Switch::state-set] for details.
 *
 * # Shortcuts and Gestures
 *
 * `BobguiSwitch` supports pan and drag gestures to move the slider.
 *
 * # CSS nodes
 *
 * ```
 * switch
 * ├── image
 * ├── image
 * ╰── slider
 * ```
 *
 * `BobguiSwitch` has four css nodes, the main node with the name switch and
 * subnodes for the slider and the on and off images. Neither of them is
 * using any style classes.
 *
 * # Accessibility
 *
 * `BobguiSwitch` uses the [enum@Bobgui.AccessibleRole.switch] role.
 */

#include "config.h"

#include "bobguiswitch.h"

#include "bobguiactionable.h"
#include "bobguiactionhelperprivate.h"
#include "bobguigestureclick.h"
#include "bobguigesturepan.h"
#include "bobguigesturesingle.h"
#include "bobguigizmoprivate.h"
#include "bobguiimage.h"
#include "bobguicustomlayout.h"
#include "bobguimarshalers.h"
#include "bobguiprivate.h"
#include "bobguiprogresstrackerprivate.h"
#include "bobguisettingsprivate.h"
#include "bobguiwidgetprivate.h"

typedef struct _BobguiSwitchClass   BobguiSwitchClass;

struct _BobguiSwitch
{
  BobguiWidget parent_instance;

  BobguiActionHelper *action_helper;

  BobguiGesture *pan_gesture;
  BobguiGesture *click_gesture;

  double handle_pos;
  guint tick_id;

  guint state                 : 1;
  guint is_active             : 1;

  BobguiProgressTracker tracker;

  BobguiWidget *on_image;
  BobguiWidget *off_image;
  BobguiWidget *slider;
};

struct _BobguiSwitchClass
{
  BobguiWidgetClass parent_class;

  void (* activate) (BobguiSwitch *self);

  gboolean (* state_set) (BobguiSwitch *self,
                          gboolean   state);
};

enum
{
  PROP_0,
  PROP_ACTIVE,
  PROP_STATE,
  LAST_PROP,
  PROP_ACTION_NAME,
  PROP_ACTION_TARGET
};

enum
{
  ACTIVATE,
  STATE_SET,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

static GParamSpec *switch_props[LAST_PROP] = { NULL, };

static void bobgui_switch_actionable_iface_init (BobguiActionableInterface *iface);

G_DEFINE_TYPE_WITH_CODE (BobguiSwitch, bobgui_switch, BOBGUI_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_ACTIONABLE,
                                                bobgui_switch_actionable_iface_init))

static gboolean
is_right_side (BobguiWidget *widget,
               gboolean   active)
{
  if (_bobgui_widget_get_direction (widget) == BOBGUI_TEXT_DIR_LTR)
    return active;
  else
    return !active;
}

static void
bobgui_switch_end_toggle_animation (BobguiSwitch *self)
{
  if (self->tick_id != 0)
    {
      bobgui_widget_remove_tick_callback (BOBGUI_WIDGET (self), self->tick_id);
      self->tick_id = 0;
    }
}

static gboolean
bobgui_switch_on_frame_clock_update (BobguiWidget     *widget,
                                  GdkFrameClock *clock,
                                  gpointer       user_data)
{
  BobguiSwitch *self = BOBGUI_SWITCH (widget);
  double progress;

  bobgui_progress_tracker_advance_frame (&self->tracker,
                                      gdk_frame_clock_get_frame_time (clock));

  if (bobgui_progress_tracker_get_state (&self->tracker) != BOBGUI_PROGRESS_STATE_AFTER)
    {
      progress = bobgui_progress_tracker_get_ease_out_cubic (&self->tracker, FALSE);
      if (is_right_side (widget, self->is_active))
        self->handle_pos = 1.0 - progress;
      else
        self->handle_pos = progress;
    }
  else
    {
      bobgui_switch_set_active (self, !self->is_active);
    }

  bobgui_widget_queue_allocate (BOBGUI_WIDGET (self));

  return G_SOURCE_CONTINUE;
}

#define ANIMATION_DURATION 100

static void
bobgui_switch_begin_toggle_animation (BobguiSwitch *self)
{
  if (bobgui_settings_get_enable_animations (bobgui_widget_get_settings (BOBGUI_WIDGET (self))))
    {
      bobgui_progress_tracker_start (&self->tracker, 1000 * ANIMATION_DURATION, 0, 1.0);
      if (self->tick_id == 0)
        self->tick_id = bobgui_widget_add_tick_callback (BOBGUI_WIDGET (self),
                                                      bobgui_switch_on_frame_clock_update,
                                                      NULL, NULL);
    }
  else
    {
      bobgui_switch_set_active (self, !self->is_active);
    }
}

static void
bobgui_switch_click_gesture_pressed (BobguiGestureClick *gesture,
                                  int              n_press,
                                  double           x,
                                  double           y,
                                  BobguiSwitch       *self)
{
  graphene_rect_t switch_bounds;

  if (!bobgui_widget_compute_bounds (BOBGUI_WIDGET (self), BOBGUI_WIDGET (self), &switch_bounds))
    return;

  /* If the press didn't happen in the draggable handle,
   * cancel the pan gesture right away
   */
  if ((self->is_active && x <= switch_bounds.size.width / 2.0) ||
      (!self->is_active && x > switch_bounds.size.width / 2.0))
    bobgui_gesture_set_state (self->pan_gesture, BOBGUI_EVENT_SEQUENCE_DENIED);
}

static void
bobgui_switch_click_gesture_released (BobguiGestureClick *gesture,
                                   int              n_press,
                                   double           x,
                                   double           y,
                                   BobguiSwitch       *self)
{
  GdkEventSequence *sequence;

  sequence = bobgui_gesture_single_get_current_sequence (BOBGUI_GESTURE_SINGLE (gesture));

  if (bobgui_widget_contains (BOBGUI_WIDGET (self), x, y) &&
      bobgui_gesture_handles_sequence (BOBGUI_GESTURE (gesture), sequence))
    {
      bobgui_gesture_set_state (BOBGUI_GESTURE (gesture), BOBGUI_EVENT_SEQUENCE_CLAIMED);
      bobgui_switch_begin_toggle_animation (self);
    }
}

static void
bobgui_switch_pan_gesture_pan (BobguiGesturePan   *gesture,
                            BobguiPanDirection  direction,
                            double           offset,
                            BobguiSwitch       *self)
{
  BobguiWidget *widget = BOBGUI_WIDGET (self);
  int width;

  width = bobgui_widget_get_width (widget);

  if (direction == BOBGUI_PAN_DIRECTION_LEFT)
    offset = -offset;

  bobgui_gesture_set_state (BOBGUI_GESTURE (gesture), BOBGUI_EVENT_SEQUENCE_CLAIMED);

  if (is_right_side (widget, self->is_active))
    offset += width / 2;

  offset /= width / 2;
  /* constrain the handle within the trough width */
  self->handle_pos = CLAMP (offset, 0, 1.0);

  /* we need to redraw the handle */
  bobgui_widget_queue_allocate (widget);
}

static void
bobgui_switch_pan_gesture_drag_end (BobguiGestureDrag *gesture,
                                 double          x,
                                 double          y,
                                 BobguiSwitch      *self)
{
  BobguiWidget *widget = BOBGUI_WIDGET (self);
  GdkEventSequence *sequence;
  gboolean active;

  sequence = bobgui_gesture_single_get_current_sequence (BOBGUI_GESTURE_SINGLE (gesture));

  if (bobgui_gesture_get_sequence_state (BOBGUI_GESTURE (gesture), sequence) == BOBGUI_EVENT_SEQUENCE_CLAIMED)
    {
      /* if half the handle passed the middle of the switch, then we
       * consider it to be on
       */
      if (_bobgui_widget_get_direction (widget) == BOBGUI_TEXT_DIR_LTR)
        active = self->handle_pos >= 0.5;
      else
        active = self->handle_pos <= 0.5;
    }
  else if (!bobgui_gesture_handles_sequence (self->click_gesture, sequence))
    active = self->is_active;
  else
    return;

  self->handle_pos = is_right_side (widget, active) ? 1.0 : 0.0;
  bobgui_switch_set_active (self, active);
  bobgui_widget_queue_allocate (widget);
}

static void
bobgui_switch_activate (BobguiSwitch *self)
{
  bobgui_switch_begin_toggle_animation (self);
}

static void
bobgui_switch_measure (BobguiWidget      *widget,
                    BobguiOrientation  orientation,
                    int             for_size,
                    int            *minimum,
                    int            *natural,
                    int            *minimum_baseline,
                    int            *natural_baseline)
{
  BobguiSwitch *self = BOBGUI_SWITCH (widget);
  int slider_minimum, slider_natural;
  int on_nat, off_nat;
  int on_baseline, off_baseline;

  bobgui_widget_measure (self->slider, orientation, -1,
                      &slider_minimum, &slider_natural,
                      NULL, NULL);

  bobgui_widget_measure (self->on_image, orientation, for_size, NULL, &on_nat, NULL, &on_baseline);
  bobgui_widget_measure (self->off_image, orientation, for_size, NULL, &off_nat, NULL, &off_baseline);

  if (orientation == BOBGUI_ORIENTATION_HORIZONTAL)
    {
      int text_width = MAX (on_nat, off_nat);
      *minimum = 2 * MAX (slider_minimum, text_width);
      *natural = 2 * MAX (slider_natural, text_width);
    }
  else
    {
      int text_height = MAX (on_nat, off_nat);

      *minimum = MAX (slider_minimum, text_height);
      *natural = MAX (slider_natural, text_height);

      *minimum_baseline = MAX (on_baseline, off_baseline) + MAX ((slider_minimum - text_height) / 2, 0);
      *natural_baseline = MAX (on_baseline, off_baseline) + MAX ((slider_natural - text_height) / 2, 0);
    }
}

static void
bobgui_switch_allocate (BobguiWidget *widget,
                     int        width,
                     int        height,
                     int        baseline)
{
  BobguiSwitch *self = BOBGUI_SWITCH (widget);
  BobguiAllocation child_alloc;
  int min;

  bobgui_widget_size_allocate (self->slider,
                            &(BobguiAllocation) {
                              round (self->handle_pos * (width / 2)), 0,
                              width / 2, height
                            }, -1);

  /* Center ON icon in left half */
  bobgui_widget_measure (self->on_image, BOBGUI_ORIENTATION_HORIZONTAL, -1, &min, NULL, NULL, NULL);
  child_alloc.x = ((width / 2) - min) / 2;
  if (is_right_side (widget, FALSE))
    child_alloc.x += width / 2;
  child_alloc.width = min;
  bobgui_widget_measure (self->on_image, BOBGUI_ORIENTATION_VERTICAL, min, &min, NULL, NULL, NULL);
  child_alloc.y = (height - min) / 2;
  child_alloc.height = min;
  bobgui_widget_size_allocate (self->on_image, &child_alloc, -1);

  /* Center OFF icon in right half */
  bobgui_widget_measure (self->off_image, BOBGUI_ORIENTATION_HORIZONTAL, -1, &min, NULL, NULL, NULL);
  child_alloc.x = ((width / 2) - min) / 2;
  if (is_right_side (widget, TRUE))
    child_alloc.x += width / 2;
  child_alloc.width = min;
  bobgui_widget_measure (self->off_image, BOBGUI_ORIENTATION_VERTICAL, min, &min, NULL, NULL, NULL);
  child_alloc.y = (height - min) / 2;
  child_alloc.height = min;
  bobgui_widget_size_allocate (self->off_image, &child_alloc, -1);
}

static void
bobgui_switch_direction_changed (BobguiWidget       *widget,
                              BobguiTextDirection previous_dir)
{
  BobguiSwitch *self = BOBGUI_SWITCH (widget);

  self->handle_pos = 1.0 - self->handle_pos;
  bobgui_widget_queue_allocate (widget);

  BOBGUI_WIDGET_CLASS (bobgui_switch_parent_class)->direction_changed (widget, previous_dir);
}

static void
bobgui_switch_set_action_name (BobguiActionable *actionable,
                            const char    *action_name)
{
  BobguiSwitch *self = BOBGUI_SWITCH (actionable);

  if (!self->action_helper)
    self->action_helper = bobgui_action_helper_new (actionable);

  bobgui_action_helper_set_action_name (self->action_helper, action_name);
}

static void
bobgui_switch_set_action_target_value (BobguiActionable *actionable,
                                    GVariant      *action_target)
{
  BobguiSwitch *self = BOBGUI_SWITCH (actionable);

  if (!self->action_helper)
    self->action_helper = bobgui_action_helper_new (actionable);

  bobgui_action_helper_set_action_target_value (self->action_helper, action_target);
}

static const char *
bobgui_switch_get_action_name (BobguiActionable *actionable)
{
  BobguiSwitch *self = BOBGUI_SWITCH (actionable);

  return bobgui_action_helper_get_action_name (self->action_helper);
}

static GVariant *
bobgui_switch_get_action_target_value (BobguiActionable *actionable)
{
  BobguiSwitch *self = BOBGUI_SWITCH (actionable);

  return bobgui_action_helper_get_action_target_value (self->action_helper);
}

static void
bobgui_switch_actionable_iface_init (BobguiActionableInterface *iface)
{
  iface->get_action_name = bobgui_switch_get_action_name;
  iface->set_action_name = bobgui_switch_set_action_name;
  iface->get_action_target_value = bobgui_switch_get_action_target_value;
  iface->set_action_target_value = bobgui_switch_set_action_target_value;
}

static void
bobgui_switch_set_property (GObject      *gobject,
                         guint         prop_id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
  BobguiSwitch *self = BOBGUI_SWITCH (gobject);

  switch (prop_id)
    {
    case PROP_ACTIVE:
      bobgui_switch_set_active (self, g_value_get_boolean (value));
      break;

    case PROP_STATE:
      bobgui_switch_set_state (self, g_value_get_boolean (value));
      break;

    case PROP_ACTION_NAME:
      bobgui_switch_set_action_name (BOBGUI_ACTIONABLE (self), g_value_get_string (value));
      break;

    case PROP_ACTION_TARGET:
      bobgui_switch_set_action_target_value (BOBGUI_ACTIONABLE (self), g_value_get_variant (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
    }
}

static void
bobgui_switch_get_property (GObject    *gobject,
                         guint       prop_id,
                         GValue     *value,
                         GParamSpec *pspec)
{
  BobguiSwitch *self = BOBGUI_SWITCH (gobject);

  switch (prop_id)
    {
    case PROP_ACTIVE:
      g_value_set_boolean (value, self->is_active);
      break;

    case PROP_STATE:
      g_value_set_boolean (value, self->state);
      break;

    case PROP_ACTION_NAME:
      g_value_set_string (value, bobgui_action_helper_get_action_name (self->action_helper));
      break;

    case PROP_ACTION_TARGET:
      g_value_set_variant (value, bobgui_action_helper_get_action_target_value (self->action_helper));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
    }
}

static void
bobgui_switch_dispose (GObject *object)
{
  BobguiSwitch *self = BOBGUI_SWITCH (object);

  g_clear_object (&self->action_helper);

  G_OBJECT_CLASS (bobgui_switch_parent_class)->dispose (object);
}

static void
bobgui_switch_finalize (GObject *object)
{
  BobguiSwitch *self = BOBGUI_SWITCH (object);

  bobgui_switch_end_toggle_animation (self);

  bobgui_widget_unparent (self->on_image);
  bobgui_widget_unparent (self->off_image);
  bobgui_widget_unparent (self->slider);

  G_OBJECT_CLASS (bobgui_switch_parent_class)->finalize (object);
}

static gboolean
state_set (BobguiSwitch *self,
           gboolean   state)
{
  if (self->action_helper)
    bobgui_action_helper_activate (self->action_helper);

  bobgui_switch_set_state (self, state);

  return TRUE;
}

static gboolean
translate_switch_shapes_to_opacity (GBinding     *binding,
                                    const GValue *from_value,
                                    GValue       *to_value,
                                    gpointer      user_data)
{
  gboolean visible = g_value_get_boolean (from_value);
  g_value_set_double (to_value, visible ? 1.0 : 0.0);

  return TRUE;
}

static void
bobgui_switch_class_init (BobguiSwitchClass *klass)
{
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  /**
   * BobguiSwitch:active:
   *
   * Whether the `BobguiSwitch` widget is in its on or off state.
   */
  switch_props[PROP_ACTIVE] =
    g_param_spec_boolean ("active", NULL, NULL,
                          FALSE,
                          BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiSwitch:state:
   *
   * The backend state that is controlled by the switch.
   *
   * Applications should usually set the [property@Bobgui.Switch:active] property,
   * except when indicating a change to the backend state which occurs
   * separately from the user's interaction.
   *
   * See [signal@Bobgui.Switch::state-set] for details.
   */
  switch_props[PROP_STATE] =
    g_param_spec_boolean ("state", NULL, NULL,
                          FALSE,
                          BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  gobject_class->set_property = bobgui_switch_set_property;
  gobject_class->get_property = bobgui_switch_get_property;
  gobject_class->dispose = bobgui_switch_dispose;
  gobject_class->finalize = bobgui_switch_finalize;

  g_object_class_install_properties (gobject_class, LAST_PROP, switch_props);

  widget_class->direction_changed = bobgui_switch_direction_changed;

  klass->activate = bobgui_switch_activate;
  klass->state_set = state_set;

  /**
   * BobguiSwitch::activate:
   * @widget: the object which received the signal
   *
   * Emitted to animate the switch.
   *
   * Applications should never connect to this signal,
   * but use the [property@Bobgui.Switch:active] property.
   */
  signals[ACTIVATE] =
    g_signal_new (I_("activate"),
                  G_OBJECT_CLASS_TYPE (gobject_class),
                  G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (BobguiSwitchClass, activate),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);

  bobgui_widget_class_set_activate_signal (widget_class, signals[ACTIVATE]);

  /**
   * BobguiSwitch::state-set:
   * @widget: the object on which the signal was emitted
   * @state: the new state of the switch
   *
   * Emitted to change the underlying state.
   *
   * The ::state-set signal is emitted when the user changes the switch
   * position. The default handler calls [method@Bobgui.Switch.set_state] with the
   * value of @state.
   *
   * To implement delayed state change, applications can connect to this
   * signal, initiate the change of the underlying state, and call
   * [method@Bobgui.Switch.set_state] when the underlying state change is
   * complete. The signal handler should return %TRUE to prevent the
   * default handler from running.
   *
   * Returns: %TRUE to stop the signal emission
   */
  signals[STATE_SET] =
    g_signal_new (I_("state-set"),
                  G_OBJECT_CLASS_TYPE (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BobguiSwitchClass, state_set),
                  _bobgui_boolean_handled_accumulator, NULL,
                  _bobgui_marshal_BOOLEAN__BOOLEAN,
                  G_TYPE_BOOLEAN, 1,
                  G_TYPE_BOOLEAN);
  g_signal_set_va_marshaller (signals[STATE_SET],
                              G_TYPE_FROM_CLASS (gobject_class),
                              _bobgui_marshal_BOOLEAN__BOOLEANv);

  g_object_class_override_property (gobject_class, PROP_ACTION_NAME, "action-name");
  g_object_class_override_property (gobject_class, PROP_ACTION_TARGET, "action-target");

  bobgui_widget_class_set_css_name (widget_class, I_("switch"));

  bobgui_widget_class_set_accessible_role (widget_class, BOBGUI_ACCESSIBLE_ROLE_SWITCH);
}

static void
bobgui_switch_init (BobguiSwitch *self)
{
  BobguiLayoutManager *layout;
  BobguiGesture *gesture;
  BobguiSettings *bobgui_settings;

  bobgui_widget_set_focusable (BOBGUI_WIDGET (self), TRUE);

  gesture = bobgui_gesture_click_new ();
  bobgui_gesture_single_set_touch_only (BOBGUI_GESTURE_SINGLE (gesture), FALSE);
  bobgui_gesture_single_set_exclusive (BOBGUI_GESTURE_SINGLE (gesture), TRUE);
  g_signal_connect (gesture, "pressed",
                    G_CALLBACK (bobgui_switch_click_gesture_pressed), self);
  g_signal_connect (gesture, "released",
                    G_CALLBACK (bobgui_switch_click_gesture_released), self);
  bobgui_event_controller_set_propagation_phase (BOBGUI_EVENT_CONTROLLER (gesture),
                                              BOBGUI_PHASE_BUBBLE);
  bobgui_widget_add_controller (BOBGUI_WIDGET (self), BOBGUI_EVENT_CONTROLLER (gesture));
  self->click_gesture = gesture;

  gesture = bobgui_gesture_pan_new (BOBGUI_ORIENTATION_HORIZONTAL);
  bobgui_gesture_single_set_touch_only (BOBGUI_GESTURE_SINGLE (gesture), FALSE);
  bobgui_gesture_single_set_exclusive (BOBGUI_GESTURE_SINGLE (gesture), TRUE);
  g_signal_connect (gesture, "pan",
                    G_CALLBACK (bobgui_switch_pan_gesture_pan), self);
  g_signal_connect (gesture, "drag-end",
                    G_CALLBACK (bobgui_switch_pan_gesture_drag_end), self);
  bobgui_event_controller_set_propagation_phase (BOBGUI_EVENT_CONTROLLER (gesture),
                                              BOBGUI_PHASE_CAPTURE);
  bobgui_widget_add_controller (BOBGUI_WIDGET (self), BOBGUI_EVENT_CONTROLLER (gesture));
  self->pan_gesture = gesture;

  layout = bobgui_custom_layout_new (NULL,
                                  bobgui_switch_measure,
                                  bobgui_switch_allocate);
  bobgui_widget_set_layout_manager (BOBGUI_WIDGET (self), layout);

  bobgui_settings = bobgui_settings_get_default ();

  self->on_image = g_object_new (BOBGUI_TYPE_IMAGE,
                                 "accessible-role", BOBGUI_ACCESSIBLE_ROLE_NONE,
                                 "icon-name", "switch-on-symbolic",
                                 NULL);
  bobgui_widget_set_parent (self->on_image, BOBGUI_WIDGET (self));

  g_object_bind_property_full (bobgui_settings, "bobgui-show-status-shapes",
                               self->on_image, "opacity",
                               G_BINDING_SYNC_CREATE,
                               translate_switch_shapes_to_opacity,
                               NULL, NULL, NULL);

  self->off_image = g_object_new (BOBGUI_TYPE_IMAGE,
                                  "accessible-role", BOBGUI_ACCESSIBLE_ROLE_NONE,
                                  "icon-name", "switch-off-symbolic",
                                  NULL);
  bobgui_widget_set_parent (self->off_image, BOBGUI_WIDGET (self));

  g_object_bind_property_full (bobgui_settings, "bobgui-show-status-shapes",
                               self->off_image, "opacity",
                               G_BINDING_SYNC_CREATE,
                               translate_switch_shapes_to_opacity,
                               NULL, NULL, NULL);

  self->slider = bobgui_gizmo_new_with_role ("slider",
                                          BOBGUI_ACCESSIBLE_ROLE_NONE,
                                          NULL, NULL, NULL, NULL, NULL, NULL);
  bobgui_widget_set_parent (self->slider, BOBGUI_WIDGET (self));

  bobgui_accessible_update_state (BOBGUI_ACCESSIBLE (self),
                               BOBGUI_ACCESSIBLE_STATE_CHECKED, FALSE,
                               -1);
  if (is_right_side (BOBGUI_WIDGET (self), FALSE))
    self->handle_pos = 1.0;
  else
    self->handle_pos = 0.0;
}

/**
 * bobgui_switch_new:
 *
 * Creates a new `BobguiSwitch` widget.
 *
 * Returns: the newly created `BobguiSwitch` instance
 */
BobguiWidget *
bobgui_switch_new (void)
{
  return g_object_new (BOBGUI_TYPE_SWITCH, NULL);
}

/**
 * bobgui_switch_set_active:
 * @self: a `BobguiSwitch`
 * @is_active: %TRUE if @self should be active, and %FALSE otherwise
 *
 * Changes the state of @self to the desired one.
 */
void
bobgui_switch_set_active (BobguiSwitch *self,
                       gboolean   is_active)
{
  g_return_if_fail (BOBGUI_IS_SWITCH (self));

  bobgui_switch_end_toggle_animation (self);

  is_active = !!is_active;

  if (self->is_active != is_active)
    {
      gboolean handled;

      self->is_active = is_active;

      if (is_right_side (BOBGUI_WIDGET (self), self->is_active))
        self->handle_pos = 1.0;
      else
        self->handle_pos = 0.0;

      g_signal_emit (self, signals[STATE_SET], 0, is_active, &handled);

      g_object_notify_by_pspec (G_OBJECT (self), switch_props[PROP_ACTIVE]);

      bobgui_accessible_update_state (BOBGUI_ACCESSIBLE (self),
                                   BOBGUI_ACCESSIBLE_STATE_CHECKED, is_active,
                                   -1);

      bobgui_widget_queue_allocate (BOBGUI_WIDGET (self));
    }
}

/**
 * bobgui_switch_get_active:
 * @self: a `BobguiSwitch`
 *
 * Gets whether the `BobguiSwitch` is in its “on” or “off” state.
 *
 * Returns: %TRUE if the `BobguiSwitch` is active, and %FALSE otherwise
 */
gboolean
bobgui_switch_get_active (BobguiSwitch *self)
{
  g_return_val_if_fail (BOBGUI_IS_SWITCH (self), FALSE);

  return self->is_active;
}

/**
 * bobgui_switch_set_state:
 * @self: a `BobguiSwitch`
 * @state: the new state
 *
 * Sets the underlying state of the `BobguiSwitch`.
 *
 * This function is typically called from a [signal@Bobgui.Switch::state-set]
 * signal handler in order to set up delayed state changes.
 *
 * See [signal@Bobgui.Switch::state-set] for details.
 */
void
bobgui_switch_set_state (BobguiSwitch *self,
                      gboolean   state)
{
  g_return_if_fail (BOBGUI_IS_SWITCH (self));

  state = state != FALSE;

  if (self->state == state)
    return;

  self->state = state;

  if (state)
    bobgui_widget_set_state_flags (BOBGUI_WIDGET (self), BOBGUI_STATE_FLAG_CHECKED, FALSE);
  else
    bobgui_widget_unset_state_flags (BOBGUI_WIDGET (self), BOBGUI_STATE_FLAG_CHECKED);

  g_object_notify_by_pspec (G_OBJECT (self), switch_props[PROP_STATE]);
}

/**
 * bobgui_switch_get_state:
 * @self: a `BobguiSwitch`
 *
 * Gets the underlying state of the `BobguiSwitch`.
 *
 * Returns: the underlying state
 */
gboolean
bobgui_switch_get_state (BobguiSwitch *self)
{
  g_return_val_if_fail (BOBGUI_IS_SWITCH (self), FALSE);

  return self->state;
}
