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

#include "bobguitogglebutton.h"

#include "bobguiaccessible.h"
#include "bobguibuttonprivate.h"
#include "bobguienums.h"
#include "bobguilabel.h"
#include "bobguimain.h"
#include "bobguimarshalers.h"
#include "bobguiprivate.h"

/**
 * BobguiToggleButton:
 *
 * Shows a button which remains “pressed-in” when clicked.
 *
 * <picture>
 *   <source srcset="toggle-button-dark.png" media="(prefers-color-scheme: dark)">
 *   <img alt="Example BobguiToggleButtons" src="toggle-button.png">
 * </picture>
 *
 * Clicking again will cause the toggle button to return to its normal state.
 *
 * A toggle button is created by calling either [ctor@Bobgui.ToggleButton.new] or
 * [ctor@Bobgui.ToggleButton.new_with_label]. If using the former, it is advisable
 * to pack a widget, (such as a `BobguiLabel` and/or a `BobguiImage`), into the toggle
 * button’s container. (See [class@Bobgui.Button] for more information).
 *
 * The state of a `BobguiToggleButton` can be set specifically using
 * [method@Bobgui.ToggleButton.set_active], and retrieved using
 * [method@Bobgui.ToggleButton.get_active].
 *
 * ## Grouping
 *
 * Toggle buttons can be grouped together, to form mutually exclusive
 * groups - only one of the buttons can be toggled at a time, and toggling
 * another one will switch the currently toggled one off.
 *
 * To add a `BobguiToggleButton` to a group, use [method@Bobgui.ToggleButton.set_group].
 *
 * ## CSS nodes
 *
 * `BobguiToggleButton` has a single CSS node with name button. To differentiate
 * it from a plain `BobguiButton`, it gets the `.toggle` style class.
 *
 * ## Accessibility
 *
 * `BobguiToggleButton` uses the [enum@Bobgui.AccessibleRole.toggle_button] role.
 *
 * ## Creating two `BobguiToggleButton` widgets.
 *
 * ```c
 * static void
 * output_state (BobguiToggleButton *source,
 *               gpointer         user_data)
 * {
 *   g_print ("Toggle button "%s" is active: %s",
 *            bobgui_button_get_label (BOBGUI_BUTTON (source)),
 *            bobgui_toggle_button_get_active (source) ? "Yes" : "No");
 * }
 *
 * static void
 * make_toggles (void)
 * {
 *   BobguiWidget *window, *toggle1, *toggle2;
 *   BobguiWidget *box;
 *   const char *text;
 *
 *   window = bobgui_window_new ();
 *   box = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 12);
 *
 *   text = "Hi, I’m toggle button one";
 *   toggle1 = bobgui_toggle_button_new_with_label (text);
 *
 *   g_signal_connect (toggle1, "toggled",
 *                     G_CALLBACK (output_state),
 *                     NULL);
 *   bobgui_box_append (BOBGUI_BOX (box), toggle1);
 *
 *   text = "Hi, I’m toggle button two";
 *   toggle2 = bobgui_toggle_button_new_with_label (text);
 *   g_signal_connect (toggle2, "toggled",
 *                     G_CALLBACK (output_state),
 *                     NULL);
 *   bobgui_box_append (BOBGUI_BOX (box), toggle2);
 *
 *   bobgui_window_set_child (BOBGUI_WINDOW (window), box);
 *   bobgui_window_present (BOBGUI_WINDOW (window));
 * }
 * ```
 */

typedef struct _BobguiToggleButtonPrivate       BobguiToggleButtonPrivate;
struct _BobguiToggleButtonPrivate
{
  BobguiToggleButton *group_next;
  BobguiToggleButton *group_prev;

  guint active         : 1;
};

enum {
  TOGGLED,
  LAST_SIGNAL
};

enum {
  PROP_0,
  PROP_ACTIVE,
  PROP_GROUP,
  NUM_PROPERTIES
};

static guint toggle_button_signals[LAST_SIGNAL] = { 0 };
static GParamSpec *toggle_button_props[NUM_PROPERTIES] = { NULL, };

G_DEFINE_TYPE_WITH_CODE (BobguiToggleButton, bobgui_toggle_button, BOBGUI_TYPE_BUTTON,
                         G_ADD_PRIVATE (BobguiToggleButton))

static void
bobgui_toggle_button_set_property (GObject      *object,
                                guint         prop_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  BobguiToggleButton *tb = BOBGUI_TOGGLE_BUTTON (object);

  switch (prop_id)
    {
    case PROP_ACTIVE:
      bobgui_toggle_button_set_active (tb, g_value_get_boolean (value));
      break;
    case PROP_GROUP:
      bobgui_toggle_button_set_group (BOBGUI_TOGGLE_BUTTON (object), g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_toggle_button_get_property (GObject      *object,
                                guint         prop_id,
                                GValue       *value,
                                GParamSpec   *pspec)
{
  BobguiToggleButton *tb = BOBGUI_TOGGLE_BUTTON (object);
  BobguiToggleButtonPrivate *priv = bobgui_toggle_button_get_instance_private (tb);

  switch (prop_id)
    {
    case PROP_ACTIVE:
      g_value_set_boolean (value, priv->active);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static gboolean
bobgui_toggle_button_mnemonic_activate (BobguiWidget *widget,
                                     gboolean   group_cycling)
{
  /*
   * We override the standard implementation in
   * bobgui_widget_real_mnemonic_activate() in order to focus the widget even
   * if there is no mnemonic conflict.
   */
  if (bobgui_widget_get_focusable (widget))
    bobgui_widget_grab_focus (widget);

  if (!group_cycling)
    bobgui_widget_activate (widget);

  return TRUE;
}

static void
bobgui_toggle_button_clicked (BobguiButton *button)
{
  BobguiToggleButton *toggle_button = BOBGUI_TOGGLE_BUTTON (button);
  BobguiToggleButtonPrivate *priv = bobgui_toggle_button_get_instance_private (toggle_button);

  if (priv->active && (priv->group_prev || priv->group_next))
    return;

  if (bobgui_button_get_action_helper (button))
    return;

  bobgui_toggle_button_set_active (toggle_button, !priv->active);
}

static void
bobgui_toggle_button_dispose (GObject *object)
{
  BobguiToggleButton *toggle_button = BOBGUI_TOGGLE_BUTTON (object);

  bobgui_toggle_button_set_group (toggle_button, NULL);

  G_OBJECT_CLASS (bobgui_toggle_button_parent_class)->dispose (object);
}

static BobguiToggleButton *
get_group_next (BobguiToggleButton *self)
{
  return ((BobguiToggleButtonPrivate *)bobgui_toggle_button_get_instance_private (self))->group_next;
}

static BobguiToggleButton *
get_group_prev (BobguiToggleButton *self)
{
  return ((BobguiToggleButtonPrivate *)bobgui_toggle_button_get_instance_private (self))->group_prev;
}

static BobguiToggleButton *
get_group_first (BobguiToggleButton *self)
{
  BobguiToggleButton *group_first = NULL;
  BobguiToggleButton *iter;

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

static void
bobgui_toggle_button_realize (BobguiWidget *widget)
{
  BobguiToggleButton *self = BOBGUI_TOGGLE_BUTTON (widget);
  BobguiToggleButtonPrivate *priv = bobgui_toggle_button_get_instance_private (self);

  BOBGUI_WIDGET_CLASS (bobgui_toggle_button_parent_class)->realize (widget);

  bobgui_accessible_update_state (BOBGUI_ACCESSIBLE (widget),
                               BOBGUI_ACCESSIBLE_STATE_PRESSED, priv->active,
                               -1);
}

static void
bobgui_toggle_button_class_init (BobguiToggleButtonClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (class);
  BobguiButtonClass *button_class = BOBGUI_BUTTON_CLASS (class);

  gobject_class->dispose = bobgui_toggle_button_dispose;
  gobject_class->set_property = bobgui_toggle_button_set_property;
  gobject_class->get_property = bobgui_toggle_button_get_property;

  widget_class->mnemonic_activate = bobgui_toggle_button_mnemonic_activate;
  widget_class->realize = bobgui_toggle_button_realize;

  button_class->clicked = bobgui_toggle_button_clicked;

  class->toggled = NULL;

  /**
   * BobguiToggleButton:active:
   *
   * If the toggle button should be pressed in.
   */
  toggle_button_props[PROP_ACTIVE] =
      g_param_spec_boolean ("active", NULL, NULL,
                            FALSE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiToggleButton:group:
   *
   * The toggle button whose group this widget belongs to.
   */
  toggle_button_props[PROP_GROUP] =
      g_param_spec_object ("group", NULL, NULL,
                           BOBGUI_TYPE_TOGGLE_BUTTON,
                           BOBGUI_PARAM_WRITABLE);

  g_object_class_install_properties (gobject_class, NUM_PROPERTIES, toggle_button_props);

  /**
   * BobguiToggleButton::toggled:
   * @togglebutton: the object which received the signal.
   *
   * Emitted whenever the `BobguiToggleButton`'s state is changed.
   */
  toggle_button_signals[TOGGLED] =
    g_signal_new (I_("toggled"),
                  G_OBJECT_CLASS_TYPE (gobject_class),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (BobguiToggleButtonClass, toggled),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);

  bobgui_widget_class_set_css_name (widget_class, I_("button"));

  bobgui_widget_class_set_accessible_role (widget_class, BOBGUI_ACCESSIBLE_ROLE_TOGGLE_BUTTON);
}

static void
bobgui_toggle_button_init (BobguiToggleButton *toggle_button)
{
  BobguiToggleButtonPrivate *priv = bobgui_toggle_button_get_instance_private (toggle_button);

  priv->active = FALSE;

  bobgui_widget_add_css_class (BOBGUI_WIDGET (toggle_button), "toggle");
}


/**
 * bobgui_toggle_button_new:
 *
 * Creates a new toggle button.
 *
 * A widget should be packed into the button, as in [ctor@Bobgui.Button.new].
 *
 * Returns: a new toggle button.
 */
BobguiWidget *
bobgui_toggle_button_new (void)
{
  return g_object_new (BOBGUI_TYPE_TOGGLE_BUTTON, NULL);
}

/**
 * bobgui_toggle_button_new_with_label:
 * @label: a string containing the message to be placed in the toggle button.
 *
 * Creates a new toggle button with a text label.
 *
 * Returns: a new toggle button.
 */
BobguiWidget *
bobgui_toggle_button_new_with_label (const char *label)
{
  return g_object_new (BOBGUI_TYPE_TOGGLE_BUTTON, "label", label, NULL);
}

/**
 * bobgui_toggle_button_new_with_mnemonic:
 * @label: the text of the button, with an underscore in front of the
 *   mnemonic character
 *
 * Creates a new `BobguiToggleButton` containing a label.
 *
 * The label will be created using [ctor@Bobgui.Label.new_with_mnemonic],
 * so underscores in @label indicate the mnemonic for the button.
 *
 * Returns: a new `BobguiToggleButton`
 */
BobguiWidget *
bobgui_toggle_button_new_with_mnemonic (const char *label)
{
  return g_object_new (BOBGUI_TYPE_TOGGLE_BUTTON,
                       "label", label,
                       "use-underline", TRUE,
                       NULL);
}

/**
 * bobgui_toggle_button_set_active:
 * @toggle_button: a `BobguiToggleButton`.
 * @is_active: %TRUE or %FALSE.
 *
 * Sets the status of the toggle button.
 *
 * Set to %TRUE if you want the `BobguiToggleButton` to be “pressed in”,
 * and %FALSE to raise it.
 *
 * If the status of the button changes, this action causes the
 * [signal@Bobgui.ToggleButton::toggled] signal to be emitted.
 */
void
bobgui_toggle_button_set_active (BobguiToggleButton *toggle_button,
                              gboolean         is_active)
{
  BobguiToggleButtonPrivate *priv = bobgui_toggle_button_get_instance_private (toggle_button);

  g_return_if_fail (BOBGUI_IS_TOGGLE_BUTTON (toggle_button));

  is_active = is_active != FALSE;

  if (priv->active == is_active)
    return;

  if (is_active && (priv->group_prev || priv->group_next))
    {
      BobguiToggleButton *group_first = NULL;
      BobguiToggleButton *iter;

      group_first = get_group_first (toggle_button);
      g_assert (group_first);

      /* Set all buttons in group to !active */
      for (iter = group_first; iter; iter = get_group_next (iter))
        bobgui_toggle_button_set_active (iter, FALSE);

      /* ... and the next code block will set this one to active */
    }

  priv->active = is_active;

  if (is_active)
    bobgui_widget_set_state_flags (BOBGUI_WIDGET (toggle_button), BOBGUI_STATE_FLAG_CHECKED, FALSE);
  else
    bobgui_widget_unset_state_flags (BOBGUI_WIDGET (toggle_button), BOBGUI_STATE_FLAG_CHECKED);

  bobgui_accessible_update_state (BOBGUI_ACCESSIBLE (toggle_button),
                               BOBGUI_ACCESSIBLE_STATE_PRESSED, is_active,
                               -1);

  g_signal_emit (toggle_button, toggle_button_signals[TOGGLED], 0);

  g_object_notify_by_pspec (G_OBJECT (toggle_button), toggle_button_props[PROP_ACTIVE]);
}

/**
 * bobgui_toggle_button_get_active:
 * @toggle_button: a `BobguiToggleButton`.
 *
 * Queries a `BobguiToggleButton` and returns its current state.
 *
 * Returns %TRUE if the toggle button is pressed in and %FALSE
 * if it is raised.
 *
 * Returns: whether the button is pressed
 */
gboolean
bobgui_toggle_button_get_active (BobguiToggleButton *toggle_button)
{
  BobguiToggleButtonPrivate *priv = bobgui_toggle_button_get_instance_private (toggle_button);

  g_return_val_if_fail (BOBGUI_IS_TOGGLE_BUTTON (toggle_button), FALSE);

  return priv->active;
}

/**
 * bobgui_toggle_button_toggled:
 * @toggle_button: a `BobguiToggleButton`.
 *
 * Emits the ::toggled signal on the `BobguiToggleButton`.
 *
 * Deprecated: 4.10: There is no good reason for an application ever to call this function.
 */
void
bobgui_toggle_button_toggled (BobguiToggleButton *toggle_button)
{
  g_return_if_fail (BOBGUI_IS_TOGGLE_BUTTON (toggle_button));

  g_signal_emit (toggle_button, toggle_button_signals[TOGGLED], 0);
}

/**
 * bobgui_toggle_button_set_group:
 * @toggle_button: a `BobguiToggleButton`
 * @group: (nullable) (transfer none): another `BobguiToggleButton` to
 *   form a group with
 *
 * Adds @self to the group of @group.
 *
 * In a group of multiple toggle buttons, only one button can be active
 * at a time.
 *
 * Setting up groups in a cycle leads to undefined behavior.
 *
 * Note that the same effect can be achieved via the [iface@Bobgui.Actionable]
 * API, by using the same action with parameter type and state type 's'
 * for all buttons in the group, and giving each button its own target
 * value.
 */
void
bobgui_toggle_button_set_group (BobguiToggleButton *toggle_button,
                             BobguiToggleButton *group)
{
  BobguiToggleButtonPrivate *priv = bobgui_toggle_button_get_instance_private (toggle_button);
  BobguiToggleButtonPrivate *group_priv;

  g_return_if_fail (BOBGUI_IS_TOGGLE_BUTTON (toggle_button));
  g_return_if_fail (toggle_button != group);

  if (!group)
    {
      if (priv->group_prev)
        {
          BobguiToggleButtonPrivate *p = bobgui_toggle_button_get_instance_private (priv->group_prev);
          p->group_next = priv->group_next;
        }
      if (priv->group_next)
        {
          BobguiToggleButtonPrivate *p = bobgui_toggle_button_get_instance_private (priv->group_next);
          p->group_prev = priv->group_prev;
        }

      priv->group_next = NULL;
      priv->group_prev = NULL;
      g_object_notify_by_pspec (G_OBJECT (toggle_button), toggle_button_props[PROP_GROUP]);
      return;
    }

  if (priv->group_next == group)
    return;

  group_priv = bobgui_toggle_button_get_instance_private (group);

  priv->group_prev = NULL;
  if (group_priv->group_prev)
    {
      BobguiToggleButtonPrivate *prev = bobgui_toggle_button_get_instance_private (group_priv->group_prev);

      prev->group_next = toggle_button;
      priv->group_prev = group_priv->group_prev;
    }

  group_priv->group_prev = toggle_button;
  priv->group_next = group;

  g_object_notify_by_pspec (G_OBJECT (toggle_button), toggle_button_props[PROP_GROUP]);
}
