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

#include "bobguicheckbutton.h"

#include "bobguiactionhelperprivate.h"
#include "bobguiboxlayout.h"
#include "bobguibuiltiniconprivate.h"
#include "bobguigestureclick.h"
#include <glib/gi18n-lib.h>
#include "bobguilabel.h"
#include "bobguiprivate.h"
#include "bobguishortcuttrigger.h"
#include "bobguicssnodeprivate.h"
#include "bobguiwidgetprivate.h"

/**
 * BobguiCheckButton:
 *
 * Places a label next to an indicator.
 *
 * <picture>
 *   <source srcset="check-button-dark.png" media="(prefers-color-scheme: dark)">
 *   <img alt="Example BobguiCheckButtons" src="check-button.png">
 * </picture>
 *
 * A `BobguiCheckButton` is created by calling either [ctor@Bobgui.CheckButton.new]
 * or [ctor@Bobgui.CheckButton.new_with_label].
 *
 * The state of a `BobguiCheckButton` can be set specifically using
 * [method@Bobgui.CheckButton.set_active], and retrieved using
 * [method@Bobgui.CheckButton.get_active].
 *
 * # Inconsistent state
 *
 * In addition to "on" and "off", check buttons can be an
 * "in between" state that is neither on nor off. This can be used
 * e.g. when the user has selected a range of elements (such as some
 * text or spreadsheet cells) that are affected by a check button,
 * and the current values in that range are inconsistent.
 *
 * To set a `BobguiCheckButton` to inconsistent state, use
 * [method@Bobgui.CheckButton.set_inconsistent].
 *
 * # Grouping
 *
 * Check buttons can be grouped together, to form mutually exclusive
 * groups - only one of the buttons can be toggled at a time, and toggling
 * another one will switch the currently toggled one off.
 *
 * Grouped check buttons use a different indicator, and are commonly referred
 * to as *radio buttons*.
 *
 * <picture>
 *   <source srcset="radio-button-dark.png" media="(prefers-color-scheme: dark)">
 *   <img alt="Example BobguiRadioButtons" src="radio-button.png">
 * </picture>
 *
 * To add a `BobguiCheckButton` to a group, use [method@Bobgui.CheckButton.set_group].
 *
 * When the code must keep track of the state of a group of radio buttons, it
 * is recommended to keep track of such state through a stateful
 * `GAction` with a target for each button. Using the `toggled` signals to keep
 * track of the group changes and state is discouraged.
 *
 * # Shortcuts and Gestures
 *
 * `BobguiCheckButton` supports the following keyboard shortcuts:
 *
 * - <kbd>␣</kbd> or <kbd>Enter</kbd> activates the button.
 *
 * # CSS nodes
 *
 * ```
 * checkbutton[.text-button][.grouped]
 * ├── check
 * ╰── [label]
 * ```
 *
 * A `BobguiCheckButton` has a main node with name checkbutton. If the
 * [property@Bobgui.CheckButton:label] or [property@Bobgui.CheckButton:child]
 * properties are set, it contains a child widget. The indicator node
 * is named check when no group is set, and radio if the checkbutton
 * is grouped together with other checkbuttons.
 *
 * # Accessibility
 *
 * `BobguiCheckButton` uses the [enum@Bobgui.AccessibleRole.checkbox] role.
 */

typedef struct {
  BobguiWidget *indicator_widget;
  BobguiWidget *child;

  guint inconsistent:  1;
  guint active:        1;
  guint use_underline: 1;
  guint child_type:    1;

  BobguiCheckButton *group_next;
  BobguiCheckButton *group_prev;

  BobguiActionHelper *action_helper;
} BobguiCheckButtonPrivate;

enum {
  PROP_0,
  PROP_ACTIVE,
  PROP_GROUP,
  PROP_LABEL,
  PROP_INCONSISTENT,
  PROP_USE_UNDERLINE,
  PROP_CHILD,

  /* actionable properties */
  PROP_ACTION_NAME,
  PROP_ACTION_TARGET,
  LAST_PROP = PROP_ACTION_NAME
};

enum {
  TOGGLED,
  ACTIVATE,
  LAST_SIGNAL
};

enum {
  LABEL_CHILD,
  WIDGET_CHILD
};

static void bobgui_check_button_actionable_iface_init (BobguiActionableInterface *iface);

static guint signals[LAST_SIGNAL] = { 0 };
static GParamSpec *props[LAST_PROP] = { NULL, };

G_DEFINE_TYPE_WITH_CODE (BobguiCheckButton, bobgui_check_button, BOBGUI_TYPE_WIDGET,
                         G_ADD_PRIVATE (BobguiCheckButton)
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_ACTIONABLE, bobgui_check_button_actionable_iface_init))

static void
bobgui_check_button_dispose (GObject *object)
{
  BobguiCheckButtonPrivate *priv = bobgui_check_button_get_instance_private (BOBGUI_CHECK_BUTTON (object));

  g_clear_object (&priv->action_helper);

  g_clear_pointer (&priv->indicator_widget, bobgui_widget_unparent);
  g_clear_pointer (&priv->child, bobgui_widget_unparent);

  bobgui_check_button_set_group (BOBGUI_CHECK_BUTTON (object), NULL);

  G_OBJECT_CLASS (bobgui_check_button_parent_class)->dispose (object);
}

static void
force_set_accessible_role (BobguiCheckButton *button, BobguiAccessibleRole role)
{
  gboolean was_realized;
  BobguiWidget *widget = BOBGUI_WIDGET (button);
  BobguiATContext *context = bobgui_accessible_get_at_context (BOBGUI_ACCESSIBLE (widget));
  if (context)
    was_realized = bobgui_at_context_is_realized (context);
  else
    was_realized = FALSE;
  if (was_realized)
    bobgui_at_context_unrealize (context);
  bobgui_widget_set_accessible_role (widget, role);
  if (was_realized)
    bobgui_at_context_realize (context);
  if (context)
    g_object_unref (context);
}

static void
update_button_role (BobguiCheckButton *self,
                    BobguiButtonRole   role)
{
  BobguiCheckButtonPrivate *priv = bobgui_check_button_get_instance_private (self);

  if (priv->indicator_widget == NULL)
    return;

  if (role == BOBGUI_BUTTON_ROLE_RADIO)
    {
      bobgui_css_node_set_name (bobgui_widget_get_css_node (priv->indicator_widget),
                             g_quark_from_static_string ("radio"));

      bobgui_widget_add_css_class (BOBGUI_WIDGET (self), "grouped");

      force_set_accessible_role (self, BOBGUI_ACCESSIBLE_ROLE_RADIO);
    }
  else
    {
      bobgui_css_node_set_name (bobgui_widget_get_css_node (priv->indicator_widget),
                             g_quark_from_static_string ("check"));

      bobgui_widget_remove_css_class (BOBGUI_WIDGET (self), "grouped");

      force_set_accessible_role (self, BOBGUI_ACCESSIBLE_ROLE_CHECKBOX);
    }
}

static void
button_role_changed (BobguiCheckButton *self)
{
  BobguiCheckButtonPrivate *priv = bobgui_check_button_get_instance_private (self);

  update_button_role (self, bobgui_action_helper_get_role (priv->action_helper));
}

static void
ensure_action_helper (BobguiCheckButton *self)
{
  BobguiCheckButtonPrivate *priv = bobgui_check_button_get_instance_private (self);

  if (priv->action_helper)
    return;

  priv->action_helper = bobgui_action_helper_new (BOBGUI_ACTIONABLE (self));
  g_signal_connect_swapped (priv->action_helper, "notify::role",
                            G_CALLBACK (button_role_changed), self);
}

static void
bobgui_check_button_set_action_name (BobguiActionable *actionable,
                                  const char    *action_name)
{
  BobguiCheckButton *self = BOBGUI_CHECK_BUTTON (actionable);
  BobguiCheckButtonPrivate *priv = bobgui_check_button_get_instance_private (self);

  ensure_action_helper (self);

  bobgui_action_helper_set_action_name (priv->action_helper, action_name);
}

static void
bobgui_check_button_set_action_target_value (BobguiActionable *actionable,
                                          GVariant      *action_target)
{
  BobguiCheckButton *self = BOBGUI_CHECK_BUTTON (actionable);
  BobguiCheckButtonPrivate *priv = bobgui_check_button_get_instance_private (self);

  ensure_action_helper (self);

  bobgui_action_helper_set_action_target_value (priv->action_helper, action_target);
}

static void
bobgui_check_button_set_property (GObject      *object,
                               guint         prop_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  switch (prop_id)
    {
    case PROP_ACTIVE:
      bobgui_check_button_set_active (BOBGUI_CHECK_BUTTON (object), g_value_get_boolean (value));
      break;
    case PROP_GROUP:
      bobgui_check_button_set_group (BOBGUI_CHECK_BUTTON (object), g_value_get_object (value));
      break;
    case PROP_LABEL:
      bobgui_check_button_set_label (BOBGUI_CHECK_BUTTON (object), g_value_get_string (value));
      break;
    case PROP_INCONSISTENT:
      bobgui_check_button_set_inconsistent (BOBGUI_CHECK_BUTTON (object), g_value_get_boolean (value));
      break;
    case PROP_USE_UNDERLINE:
      bobgui_check_button_set_use_underline (BOBGUI_CHECK_BUTTON (object), g_value_get_boolean (value));
      break;
    case PROP_CHILD:
      bobgui_check_button_set_child (BOBGUI_CHECK_BUTTON (object), g_value_get_object (value));
      break;
    case PROP_ACTION_NAME:
      bobgui_check_button_set_action_name (BOBGUI_ACTIONABLE (object), g_value_get_string (value));
      break;
    case PROP_ACTION_TARGET:
      bobgui_check_button_set_action_target_value (BOBGUI_ACTIONABLE (object), g_value_get_variant (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_check_button_get_property (GObject    *object,
                               guint       prop_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  BobguiCheckButtonPrivate *priv = bobgui_check_button_get_instance_private (BOBGUI_CHECK_BUTTON (object));

  switch (prop_id)
    {
    case PROP_ACTIVE:
      g_value_set_boolean (value, bobgui_check_button_get_active (BOBGUI_CHECK_BUTTON (object)));
      break;
    case PROP_LABEL:
      g_value_set_string (value, bobgui_check_button_get_label (BOBGUI_CHECK_BUTTON (object)));
      break;
    case PROP_INCONSISTENT:
      g_value_set_boolean (value, bobgui_check_button_get_inconsistent (BOBGUI_CHECK_BUTTON (object)));
      break;
    case PROP_USE_UNDERLINE:
      g_value_set_boolean (value, bobgui_check_button_get_use_underline (BOBGUI_CHECK_BUTTON (object)));
      break;
    case PROP_CHILD:
      g_value_set_object (value, bobgui_check_button_get_child (BOBGUI_CHECK_BUTTON (object)));
      break;
    case PROP_ACTION_NAME:
      g_value_set_string (value, bobgui_action_helper_get_action_name (priv->action_helper));
      break;
    case PROP_ACTION_TARGET:
      g_value_set_variant (value, bobgui_action_helper_get_action_target_value (priv->action_helper));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static const char *
bobgui_check_button_get_action_name (BobguiActionable *actionable)
{
  BobguiCheckButton *self = BOBGUI_CHECK_BUTTON (actionable);
  BobguiCheckButtonPrivate *priv = bobgui_check_button_get_instance_private (self);

  return bobgui_action_helper_get_action_name (priv->action_helper);
}

static GVariant *
bobgui_check_button_get_action_target_value (BobguiActionable *actionable)
{
  BobguiCheckButton *self = BOBGUI_CHECK_BUTTON (actionable);
  BobguiCheckButtonPrivate *priv = bobgui_check_button_get_instance_private (self);

  return bobgui_action_helper_get_action_target_value (priv->action_helper);
}

static void
bobgui_check_button_actionable_iface_init (BobguiActionableInterface *iface)
{
  iface->get_action_name = bobgui_check_button_get_action_name;
  iface->set_action_name = bobgui_check_button_set_action_name;
  iface->get_action_target_value = bobgui_check_button_get_action_target_value;
  iface->set_action_target_value = bobgui_check_button_set_action_target_value;
}

static void
click_pressed_cb (BobguiGestureClick *gesture,
                  guint            n_press,
                  double           x,
                  double           y,
                  BobguiWidget       *widget)
{
  if (bobgui_widget_get_focus_on_click (widget) && !bobgui_widget_has_focus (widget))
    bobgui_widget_grab_focus (widget);
}

static void
click_released_cb (BobguiGestureClick *gesture,
                   guint            n_press,
                   double           x,
                   double           y,
                   BobguiWidget       *widget)
{
  BobguiCheckButton *self = BOBGUI_CHECK_BUTTON (widget);
  BobguiCheckButtonPrivate *priv = bobgui_check_button_get_instance_private (self);

  if (priv->active && (priv->group_prev || priv->group_next))
    return;

  bobgui_gesture_set_state (BOBGUI_GESTURE (gesture), BOBGUI_EVENT_SEQUENCE_CLAIMED);
  if  (bobgui_widget_is_sensitive (widget) &&
       bobgui_widget_contains (widget, x, y))
    {
      if (priv->action_helper)
        bobgui_action_helper_activate (priv->action_helper);
      else
        bobgui_check_button_set_active (self, !priv->active);
    }
}

static void
update_accessible_state (BobguiCheckButton *check_button)
{
  BobguiCheckButtonPrivate *priv = bobgui_check_button_get_instance_private (check_button);

  BobguiAccessibleTristate checked_state;

  if (priv->inconsistent)
    checked_state = BOBGUI_ACCESSIBLE_TRISTATE_MIXED;
  else if (priv->active)
    checked_state = BOBGUI_ACCESSIBLE_TRISTATE_TRUE;
  else
    checked_state = BOBGUI_ACCESSIBLE_TRISTATE_FALSE;

  bobgui_accessible_update_state (BOBGUI_ACCESSIBLE (check_button),
                               BOBGUI_ACCESSIBLE_STATE_CHECKED, checked_state,
                               -1);
}


static BobguiCheckButton *
get_group_next (BobguiCheckButton *self)
{
  return ((BobguiCheckButtonPrivate *)bobgui_check_button_get_instance_private (self))->group_next;
}

static BobguiCheckButton *
get_group_prev (BobguiCheckButton *self)
{
  return ((BobguiCheckButtonPrivate *)bobgui_check_button_get_instance_private (self))->group_prev;
}

static BobguiCheckButton *
get_group_first (BobguiCheckButton *self)
{
  BobguiCheckButton *group_first = NULL;
  BobguiCheckButton *iter;

  /* Find first in group */
  iter = self;
  while (iter)
    {
      group_first = iter;

      iter = get_group_prev (iter);
      if (!iter)
        break;
    }

  g_assert (group_first);

  return group_first;
}

static BobguiCheckButton *
get_group_active_button (BobguiCheckButton *self)
{
  BobguiCheckButton *iter;

  for (iter = get_group_first (self); iter; iter = get_group_next (iter))
    {
      if (bobgui_check_button_get_active (iter))
        return iter;
    }

  return NULL;
}

static void
bobgui_check_button_state_flags_changed (BobguiWidget     *widget,
                                      BobguiStateFlags  previous_flags)
{
  BobguiCheckButton *self = BOBGUI_CHECK_BUTTON (widget);
  BobguiCheckButtonPrivate *priv = bobgui_check_button_get_instance_private (self);
  BobguiStateFlags state = bobgui_widget_get_state_flags (BOBGUI_WIDGET (self));

  bobgui_widget_set_state_flags (priv->indicator_widget, state, TRUE);

  BOBGUI_WIDGET_CLASS (bobgui_check_button_parent_class)->state_flags_changed (widget, previous_flags);
}

static gboolean
bobgui_check_button_focus (BobguiWidget         *widget,
                        BobguiDirectionType   direction)
{
  BobguiCheckButton *self = BOBGUI_CHECK_BUTTON (widget);

  if (bobgui_widget_is_focus (widget))
    {
      BobguiCheckButton *iter;
      GPtrArray *child_array;
      BobguiWidget *new_focus = NULL;
      guint index;
      gboolean found;
      guint i;

      if (direction == BOBGUI_DIR_TAB_FORWARD ||
          direction == BOBGUI_DIR_TAB_BACKWARD)
        return FALSE;

      child_array = g_ptr_array_new ();
      for (iter = get_group_first (self); iter; iter = get_group_next (iter))
        g_ptr_array_add (child_array, iter);

      bobgui_widget_focus_sort (widget, direction, child_array);
      found = g_ptr_array_find (child_array, widget, &index);

      if (found)
        {
          /* Start at the *next* widget in the list */
          if (index < child_array->len - 1)
            index ++;
        }
      else
        {
          /* Search from the start of the list */
          index = 0;
        }

      for (i = index; i < child_array->len; i ++)
        {
          BobguiWidget *child = g_ptr_array_index (child_array, i);

          if (bobgui_widget_get_mapped (child) && bobgui_widget_is_sensitive (child))
            {
              new_focus = child;
              break;
            }
        }

      g_ptr_array_free (child_array, TRUE);

      if (new_focus && new_focus != widget)
        {
          bobgui_widget_grab_focus (new_focus);
          bobgui_widget_activate (new_focus);
          return TRUE;
        }
      return FALSE;
    }
  else
    {
      BobguiCheckButton *active_button;

      active_button = get_group_active_button (self);
      if (active_button && active_button != self)
        return FALSE;

      bobgui_widget_grab_focus (widget);
      return TRUE;
    }
}

static void
bobgui_check_button_real_set_child (BobguiCheckButton *self,
                                 BobguiWidget      *child,
                                 guint           child_type)
{
  BobguiCheckButtonPrivate *priv = bobgui_check_button_get_instance_private (self);

  g_return_if_fail (BOBGUI_IS_CHECK_BUTTON (self));

  g_clear_pointer (&priv->child, bobgui_widget_unparent);

  priv->child = child;

  if (priv->child)
    {
      bobgui_widget_set_parent (priv->child, BOBGUI_WIDGET (self));
      bobgui_widget_insert_after (priv->child, BOBGUI_WIDGET (self), priv->indicator_widget);
    }

  if (child_type == priv->child_type)
    return;

  priv->child_type = child_type;
  if (child_type != LABEL_CHILD)
    g_object_notify_by_pspec (G_OBJECT (self), props[PROP_LABEL]);
  else
    g_object_notify_by_pspec (G_OBJECT (self), props[PROP_CHILD]);

}

static void
bobgui_check_button_real_activate (BobguiCheckButton *self)
{
  BobguiCheckButtonPrivate *priv = bobgui_check_button_get_instance_private (self);

  if (priv->active && (priv->group_prev || priv->group_next))
    return;

  if (priv->action_helper)
    bobgui_action_helper_activate (priv->action_helper);
  else
    bobgui_check_button_set_active (self, !bobgui_check_button_get_active (self));
}

static void
bobgui_check_button_class_init (BobguiCheckButtonClass *class)
{
  const guint activate_keyvals[] = {
    GDK_KEY_space,
    GDK_KEY_KP_Space,
    GDK_KEY_Return,
    GDK_KEY_ISO_Enter,
    GDK_KEY_KP_Enter
  };
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (class);
  BobguiShortcutAction *activate_action;

  object_class->dispose = bobgui_check_button_dispose;
  object_class->set_property = bobgui_check_button_set_property;
  object_class->get_property = bobgui_check_button_get_property;

  widget_class->state_flags_changed = bobgui_check_button_state_flags_changed;
  widget_class->focus = bobgui_check_button_focus;

  class->activate = bobgui_check_button_real_activate;

  /**
   * BobguiCheckButton:active:
   *
   * If the check button is active.
   *
   * Setting `active` to %TRUE will add the `:checked:` state to both
   * the check button and the indicator CSS node.
   */
  props[PROP_ACTIVE] =
      g_param_spec_boolean ("active", NULL, NULL,
                            FALSE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiCheckButton:group:
   *
   * The check button whose group this widget belongs to.
   */
  props[PROP_GROUP] =
      g_param_spec_object ("group", NULL, NULL,
                           BOBGUI_TYPE_CHECK_BUTTON,
                           BOBGUI_PARAM_WRITABLE);

  /**
   * BobguiCheckButton:label:
   *
   * Text of the label inside the check button, if it contains a label widget.
   */
  props[PROP_LABEL] =
    g_param_spec_string ("label", NULL, NULL,
                         NULL,
                         BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiCheckButton:inconsistent:
   *
   * If the check button is in an “in between” state.
   *
   * The inconsistent state only affects visual appearance,
   * not the semantics of the button.
   */
  props[PROP_INCONSISTENT] =
      g_param_spec_boolean ("inconsistent", NULL, NULL,
                            FALSE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiCheckButton:use-underline:
   *
   * If set, an underline in the text indicates that the following
   * character is to be used as mnemonic.
   */
  props[PROP_USE_UNDERLINE] =
      g_param_spec_boolean ("use-underline", NULL, NULL,
                            FALSE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiCheckButton:child:
   *
   * The child widget.
   *
   * Since: 4.8
   */
  props[PROP_CHILD] =
      g_param_spec_object ("child", NULL, NULL,
                           BOBGUI_TYPE_WIDGET,
                           BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, LAST_PROP, props);

  g_object_class_override_property (object_class, PROP_ACTION_NAME, "action-name");
  g_object_class_override_property (object_class, PROP_ACTION_TARGET, "action-target");

  /**
   * BobguiCheckButton::toggled:
   *
   * Emitted when the buttons's [property@Bobgui.CheckButton:active]
   * property changes.
   */
  signals[TOGGLED] =
    g_signal_new (I_("toggled"),
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (BobguiCheckButtonClass, toggled),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);

  /**
   * BobguiCheckButton::activate:
   * @widget: the object which received the signal.
   *
   * Emitted to when the check button is activated.
   *
   * The `::activate` signal on `BobguiCheckButton` is an action signal and
   * emitting it causes the button to animate press then release.
   *
   * Applications should never connect to this signal, but use the
   * [signal@Bobgui.CheckButton::toggled] signal.
   *
   * The default bindings for this signal are all forms of the
   * <kbd>␣</kbd> and <kbd>Enter</kbd> keys.
   *
   * Since: 4.2
   */
  signals[ACTIVATE] =
      g_signal_new (I_ ("activate"),
                    G_OBJECT_CLASS_TYPE (object_class),
                    G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                    G_STRUCT_OFFSET (BobguiCheckButtonClass, activate),
                    NULL, NULL,
                    NULL,
                    G_TYPE_NONE, 0);

  bobgui_widget_class_set_activate_signal (widget_class, signals[ACTIVATE]);

  activate_action = bobgui_signal_action_new ("activate");
  for (guint i = 0; i < G_N_ELEMENTS (activate_keyvals); i++)
    {
      BobguiShortcut *activate_shortcut = bobgui_shortcut_new (bobgui_keyval_trigger_new (activate_keyvals[i], 0),
                                                         g_object_ref (activate_action));

      bobgui_widget_class_add_shortcut (widget_class, activate_shortcut);
      g_object_unref (activate_shortcut);
    }
  g_object_unref (activate_action);

  bobgui_widget_class_set_layout_manager_type (widget_class, BOBGUI_TYPE_BOX_LAYOUT);
  bobgui_widget_class_set_css_name (widget_class, I_("checkbutton"));
  bobgui_widget_class_set_accessible_role (widget_class, BOBGUI_ACCESSIBLE_ROLE_CHECKBOX);
}

static void
bobgui_check_button_init (BobguiCheckButton *self)
{
  BobguiCheckButtonPrivate *priv = bobgui_check_button_get_instance_private (self);
  BobguiGesture *gesture;

  bobgui_widget_set_receives_default (BOBGUI_WIDGET (self), FALSE);
  priv->indicator_widget = bobgui_builtin_icon_new ("check");
  bobgui_widget_set_halign (priv->indicator_widget, BOBGUI_ALIGN_CENTER);
  bobgui_widget_set_valign (priv->indicator_widget, BOBGUI_ALIGN_CENTER);
  bobgui_widget_set_parent (priv->indicator_widget, BOBGUI_WIDGET (self));

  update_accessible_state (self);

  gesture = bobgui_gesture_click_new ();
  bobgui_gesture_single_set_touch_only (BOBGUI_GESTURE_SINGLE (gesture), FALSE);
  bobgui_gesture_single_set_exclusive (BOBGUI_GESTURE_SINGLE (gesture), TRUE);
  bobgui_gesture_single_set_button (BOBGUI_GESTURE_SINGLE (gesture), GDK_BUTTON_PRIMARY);
  g_signal_connect (gesture, "pressed", G_CALLBACK (click_pressed_cb), self);
  g_signal_connect (gesture, "released", G_CALLBACK (click_released_cb), self);
  bobgui_event_controller_set_propagation_phase (BOBGUI_EVENT_CONTROLLER (gesture), BOBGUI_PHASE_CAPTURE);
  bobgui_widget_add_controller (BOBGUI_WIDGET (self), BOBGUI_EVENT_CONTROLLER (gesture));

  bobgui_widget_set_focusable (BOBGUI_WIDGET (self), TRUE);
}

/**
 * bobgui_check_button_new:
 *
 * Creates a new `BobguiCheckButton`.
 *
 * Returns: a new `BobguiCheckButton`
 */
BobguiWidget *
bobgui_check_button_new (void)
{
  return g_object_new (BOBGUI_TYPE_CHECK_BUTTON, NULL);
}

/**
 * bobgui_check_button_new_with_label:
 * @label: (nullable): the text for the check button.
 *
 * Creates a new `BobguiCheckButton` with the given text.
 *
 * Returns: a new `BobguiCheckButton`
 */
BobguiWidget*
bobgui_check_button_new_with_label (const char *label)
{
  return g_object_new (BOBGUI_TYPE_CHECK_BUTTON, "label", label, NULL);
}

/**
 * bobgui_check_button_new_with_mnemonic:
 * @label: (nullable): The text of the button, with an underscore
 *   in front of the mnemonic character
 *
 * Creates a new `BobguiCheckButton` with the given text and a mnemonic.
 *
 * Returns: a new `BobguiCheckButton`
 */
BobguiWidget*
bobgui_check_button_new_with_mnemonic (const char *label)
{
  return g_object_new (BOBGUI_TYPE_CHECK_BUTTON,
                       "label", label,
                       "use-underline", TRUE,
                       NULL);
}

/**
 * bobgui_check_button_set_inconsistent:
 * @check_button: a `BobguiCheckButton`
 * @inconsistent: %TRUE if state is inconsistent
 *
 * Sets the `BobguiCheckButton` to inconsistent state.
 *
 * You should turn off the inconsistent state again if the user checks
 * the check button. This has to be done manually.
 */
void
bobgui_check_button_set_inconsistent (BobguiCheckButton *check_button,
                                   gboolean        inconsistent)
{
  BobguiCheckButtonPrivate *priv = bobgui_check_button_get_instance_private (check_button);

  g_return_if_fail (BOBGUI_IS_CHECK_BUTTON (check_button));

  inconsistent = !!inconsistent;
  if (priv->inconsistent != inconsistent)
    {
      priv->inconsistent = inconsistent;

      if (inconsistent)
        {
          bobgui_widget_set_state_flags (BOBGUI_WIDGET (check_button), BOBGUI_STATE_FLAG_INCONSISTENT, FALSE);
          bobgui_widget_set_state_flags (priv->indicator_widget, BOBGUI_STATE_FLAG_INCONSISTENT, FALSE);
        }
      else
        {
          bobgui_widget_unset_state_flags (BOBGUI_WIDGET (check_button), BOBGUI_STATE_FLAG_INCONSISTENT);
          bobgui_widget_unset_state_flags (priv->indicator_widget, BOBGUI_STATE_FLAG_INCONSISTENT);
        }

      update_accessible_state (check_button);

      g_object_notify_by_pspec (G_OBJECT (check_button), props[PROP_INCONSISTENT]);
    }
}

/**
 * bobgui_check_button_get_inconsistent:
 * @check_button: a `BobguiCheckButton`
 *
 * Returns whether the check button is in an inconsistent state.
 *
 * Returns: %TRUE if @check_button is currently in an inconsistent state
 */
gboolean
bobgui_check_button_get_inconsistent (BobguiCheckButton *check_button)
{
  BobguiCheckButtonPrivate *priv = bobgui_check_button_get_instance_private (check_button);

  g_return_val_if_fail (BOBGUI_IS_CHECK_BUTTON (check_button), FALSE);

  return priv->inconsistent;
}

/**
 * bobgui_check_button_get_active:
 * @self: a `BobguiCheckButton`
 *
 * Returns whether the check button is active.
 *
 * Returns: whether the check button is active
 */
gboolean
bobgui_check_button_get_active (BobguiCheckButton *self)
{
  BobguiCheckButtonPrivate *priv = bobgui_check_button_get_instance_private (self);

  g_return_val_if_fail (BOBGUI_IS_CHECK_BUTTON (self), FALSE);

  return priv->active;
}

/**
 * bobgui_check_button_set_active:
 * @self: a `BobguiCheckButton`
 * @setting: the new value to set
 *
 * Changes the check buttons active state.
 */
void
bobgui_check_button_set_active (BobguiCheckButton *self,
                             gboolean       setting)
{
  BobguiCheckButtonPrivate *priv = bobgui_check_button_get_instance_private (self);

  g_return_if_fail (BOBGUI_IS_CHECK_BUTTON (self));

  setting = !!setting;

  if (setting == priv->active)
    return;

  if (setting)
    {
      bobgui_widget_set_state_flags (BOBGUI_WIDGET (self), BOBGUI_STATE_FLAG_CHECKED, FALSE);
      bobgui_widget_set_state_flags (priv->indicator_widget, BOBGUI_STATE_FLAG_CHECKED, FALSE);
    }
  else
    {
      bobgui_widget_unset_state_flags (BOBGUI_WIDGET (self), BOBGUI_STATE_FLAG_CHECKED);
      bobgui_widget_unset_state_flags (priv->indicator_widget, BOBGUI_STATE_FLAG_CHECKED);
    }

  if (setting && (priv->group_prev || priv->group_next))
    {
      BobguiCheckButton *group_first = NULL;
      BobguiCheckButton *iter;

      group_first = get_group_first (self);
      g_assert (group_first);

      /* Set all buttons in group to !active */
      for (iter = group_first; iter; iter = get_group_next (iter))
        bobgui_check_button_set_active (iter, FALSE);

      /* ... and the next code block will set this one to active */
    }

  priv->active = setting;
  update_accessible_state (self);
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_ACTIVE]);
  g_signal_emit (self, signals[TOGGLED], 0);
}

/**
 * bobgui_check_button_get_label:
 * @self: a `BobguiCheckButton`
 *
 * Returns the label of the check button or `NULL` if [property@CheckButton:child] is set.
 *
 * Returns: (nullable) (transfer none): The label @self shows next
 *   to the indicator. If no label is shown, %NULL will be returned.
 */
const char *
bobgui_check_button_get_label (BobguiCheckButton *self)
{
  BobguiCheckButtonPrivate *priv = bobgui_check_button_get_instance_private (self);

  g_return_val_if_fail (BOBGUI_IS_CHECK_BUTTON (self), "");

  if (priv->child_type == LABEL_CHILD && priv->child != NULL)
    return bobgui_label_get_label (BOBGUI_LABEL (priv->child));

  return NULL;
}

/**
 * bobgui_check_button_set_label:
 * @self: a `BobguiCheckButton`
 * @label: (nullable): The text shown next to the indicator, or %NULL
 *   to show no text
 *
 * Sets the text of @self.
 *
 * If [property@Bobgui.CheckButton:use-underline] is %TRUE, an underscore
 * in @label is interpreted as mnemonic indicator, see
 * [method@Bobgui.CheckButton.set_use_underline] for details on this behavior.
 */
void
bobgui_check_button_set_label (BobguiCheckButton *self,
                            const char     *label)
{
  BobguiCheckButtonPrivate *priv = bobgui_check_button_get_instance_private (self);
  BobguiWidget *child;

  g_return_if_fail (BOBGUI_IS_CHECK_BUTTON (self));

  g_object_freeze_notify (G_OBJECT (self));

  if (label == NULL || label[0] == '\0')
    {
      bobgui_check_button_real_set_child (self, NULL, LABEL_CHILD);
      bobgui_widget_remove_css_class (BOBGUI_WIDGET (self), "text-button");
    }
  else
    {
      if (priv->child_type != LABEL_CHILD || priv->child == NULL)
        {
          child = bobgui_label_new (NULL);
          bobgui_widget_set_hexpand (child, TRUE);
          bobgui_label_set_xalign (BOBGUI_LABEL (child), 0.0f);
          if (priv->use_underline)
            bobgui_label_set_use_underline (BOBGUI_LABEL (child), priv->use_underline);
          bobgui_check_button_real_set_child (self, BOBGUI_WIDGET (child), LABEL_CHILD);
        }

      bobgui_widget_add_css_class (BOBGUI_WIDGET (self), "text-button");
      bobgui_label_set_label (BOBGUI_LABEL (priv->child), label);
    }

  /* Use the processed text from the label widget for accessibility,
   * which strips mnemonics when use_underline is enabled */
  if (priv->child_type == LABEL_CHILD && priv->child != NULL)
    {
      const char *text = bobgui_label_get_text (BOBGUI_LABEL (priv->child));
      bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (self),
                                      BOBGUI_ACCESSIBLE_PROPERTY_LABEL, text,
                                      -1);
    }
  else
    {
      bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (self),
                                      BOBGUI_ACCESSIBLE_PROPERTY_LABEL, label,
                                      -1);
    }

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_LABEL]);

  g_object_thaw_notify (G_OBJECT (self));
}

/**
 * bobgui_check_button_set_group:
 * @self: a `BobguiCheckButton`
 * @group: (nullable) (transfer none): another `BobguiCheckButton` to
 *   form a group with
 *
 * Adds @self to the group of @group.
 *
 * In a group of multiple check buttons, only one button can be active
 * at a time. The behavior of a checkbutton in a group is also commonly
 * known as a *radio button*.
 *
 * Setting the group of a check button also changes the css name of the
 * indicator widget's CSS node to 'radio'.
 *
 * Setting up groups in a cycle leads to undefined behavior.
 *
 * Note that the same effect can be achieved via the [iface@Bobgui.Actionable]
 * API, by using the same action with parameter type and state type 's'
 * for all buttons in the group, and giving each button its own target
 * value.
 */
void
bobgui_check_button_set_group (BobguiCheckButton *self,
                            BobguiCheckButton *group)
{
  BobguiCheckButtonPrivate *priv = bobgui_check_button_get_instance_private (self);
  BobguiCheckButtonPrivate *group_priv;

  g_return_if_fail (BOBGUI_IS_CHECK_BUTTON (self));
  g_return_if_fail (self != group);

  if (!group)
    {
      if (priv->group_prev)
        {
          BobguiCheckButtonPrivate *p = bobgui_check_button_get_instance_private (priv->group_prev);
          p->group_next = priv->group_next;
        }
      if (priv->group_next)
        {
          BobguiCheckButtonPrivate *p = bobgui_check_button_get_instance_private (priv->group_next);
          p->group_prev = priv->group_prev;
        }

      priv->group_next = NULL;
      priv->group_prev = NULL;

      update_button_role (self, BOBGUI_BUTTON_ROLE_CHECK);

      force_set_accessible_role (self, BOBGUI_ACCESSIBLE_ROLE_CHECKBOX);

      return;
    }

  if (priv->group_next == group)
    return;

  group_priv = bobgui_check_button_get_instance_private (group);

  priv->group_prev = NULL;
  if (group_priv->group_prev)
    {
      BobguiCheckButtonPrivate *prev = bobgui_check_button_get_instance_private (group_priv->group_prev);

      prev->group_next = self;
      priv->group_prev = group_priv->group_prev;
    }

  group_priv->group_prev = self;
  priv->group_next = group;

  update_button_role (self, BOBGUI_BUTTON_ROLE_RADIO);
  update_button_role (group, BOBGUI_BUTTON_ROLE_RADIO);

  force_set_accessible_role (self, BOBGUI_ACCESSIBLE_ROLE_RADIO);
  force_set_accessible_role (group, BOBGUI_ACCESSIBLE_ROLE_RADIO);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_GROUP]);
}

/**
 * bobgui_check_button_get_use_underline:
 * @self: a `BobguiCheckButton`
 *
 * Returns whether underlines in the label indicate mnemonics.
 *
 * Returns: The value of the [property@Bobgui.CheckButton:use-underline] property.
 *   See [method@Bobgui.CheckButton.set_use_underline] for details on how to set
 *   a new value.
 */
gboolean
bobgui_check_button_get_use_underline (BobguiCheckButton *self)
{
  BobguiCheckButtonPrivate *priv = bobgui_check_button_get_instance_private (self);

  g_return_val_if_fail (BOBGUI_IS_CHECK_BUTTON (self), FALSE);

  return priv->use_underline;
}

/**
 * bobgui_check_button_set_use_underline:
 * @self: a `BobguiCheckButton`
 * @setting: the new value to set
 *
 * Sets whether underlines in the label indicate mnemonics.
 *
 * If @setting is %TRUE, an underscore character in @self's label
 * indicates a mnemonic accelerator key. This behavior is similar
 * to [property@Bobgui.Label:use-underline].
 */
void
bobgui_check_button_set_use_underline (BobguiCheckButton *self,
                                    gboolean       setting)
{
  BobguiCheckButtonPrivate *priv = bobgui_check_button_get_instance_private (self);

  g_return_if_fail (BOBGUI_IS_CHECK_BUTTON (self));

  setting = !!setting;

  if (setting == priv->use_underline)
    return;

  priv->use_underline = setting;
  if (priv->child_type == LABEL_CHILD && priv->child != NULL)
    {
      bobgui_label_set_use_underline (BOBGUI_LABEL (priv->child), priv->use_underline);

      const char *text = bobgui_label_get_text (BOBGUI_LABEL (priv->child));
      bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (self),
                                      BOBGUI_ACCESSIBLE_PROPERTY_LABEL, text,
                                      -1);
    }

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_USE_UNDERLINE]);
}

/**
 * bobgui_check_button_set_child:
 * @button: a `BobguiCheckButton`
 * @child: (nullable): the child widget
 *
 * Sets the child widget of @button.
 *
 * Note that by using this API, you take full responsibility for setting
 * up the proper accessibility label and description information for @button.
 * Most likely, you'll either set the accessibility label or description
 * for @button explicitly, or you'll set a labelled-by or described-by
 * relations from @child to @button.
 *
 * Since: 4.8
 */
void
bobgui_check_button_set_child (BobguiCheckButton *button,
                            BobguiWidget      *child)
{
  BobguiCheckButtonPrivate *priv = bobgui_check_button_get_instance_private (button);

  g_return_if_fail (BOBGUI_IS_CHECK_BUTTON (button));
  g_return_if_fail (child == NULL || priv->child == child || bobgui_widget_get_parent (child) == NULL);

  if (priv->child == child)
    return;

  g_object_freeze_notify (G_OBJECT (button));

  bobgui_widget_remove_css_class (BOBGUI_WIDGET (button), "text-button");

  bobgui_check_button_real_set_child (button, child, WIDGET_CHILD);

  g_object_notify_by_pspec (G_OBJECT (button), props[PROP_CHILD]);

  g_object_thaw_notify (G_OBJECT (button));
}

/**
 * bobgui_check_button_get_child:
 * @button: a `BobguiCheckButton`
 *
 * Gets the child widget of @button or `NULL` if [property@CheckButton:label] is set.
 *
 * Returns: (nullable) (transfer none): the child widget of @button
 *
 * Since: 4.8
 */
BobguiWidget *
bobgui_check_button_get_child (BobguiCheckButton *button)
{
  BobguiCheckButtonPrivate *priv = bobgui_check_button_get_instance_private (button);

  g_return_val_if_fail (BOBGUI_IS_CHECK_BUTTON (button), NULL);

  if (priv->child_type == WIDGET_CHILD)
    return priv->child;

  return NULL;
}
