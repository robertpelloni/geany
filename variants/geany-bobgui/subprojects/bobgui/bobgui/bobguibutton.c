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
 * Modified by the BOBGUI Team and others 1997-2001.  See the AUTHORS
 * file for a list of people on the BOBGUI Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BOBGUI at ftp://ftp.bobgui.org/pub/bobgui/.
 */

/**
 * BobguiButton:
 *
 * Calls a callback function when the button is clicked.
 *
 * <picture>
 *   <source srcset="button-dark.png" media="(prefers-color-scheme: dark)">
 *   <img alt="An example BobguiButton" src="button.png">
 * </picture>
 *
 * The `BobguiButton` widget can hold any valid child widget. That is, it can hold
 * almost any other standard `BobguiWidget`. The most commonly used child is the
 * `BobguiLabel`.
 *
 * # Shortcuts and Gestures
 *
 * The following signals have default keybindings:
 *
 * - [signal@Bobgui.Button::activate]
 *
 * # CSS nodes
 *
 * `BobguiButton` has a single CSS node with name button. The node will get the
 * style classes .image-button or .text-button, if the content is just an
 * image or label, respectively. It may also receive the .flat style class.
 * When activating a button via the keyboard, the button will temporarily
 * gain the .keyboard-activating style class.
 *
 * Other style classes that are commonly used with `BobguiButton` include
 * .suggested-action and .destructive-action. In special cases, buttons
 * can be made round by adding the .circular style class.
 *
 * Button-like widgets like [class@Bobgui.ToggleButton], [class@Bobgui.MenuButton],
 * [class@Bobgui.VolumeButton], [class@Bobgui.LockButton], [class@Bobgui.ColorButton]
 * or [class@Bobgui.FontButton] use style classes such as .toggle, .popup, .scale,
 * .lock, .color on the button node to differentiate themselves from a plain
 * `BobguiButton`.
 *
 * # Accessibility
 *
 * `BobguiButton` uses the [enum@Bobgui.AccessibleRole.button] role.
 */

#include "config.h"

#include "bobguibuttonprivate.h"

#include "bobguiactionhelperprivate.h"
#include "bobguibuildable.h"
#include "bobguigestureclick.h"
#include "bobguieventcontrollerkey.h"
#include "bobguibinlayout.h"
#include "bobguiimage.h"
#include "bobguilabel.h"
#include "bobguimarshalers.h"
#include "bobguiprivate.h"
#include "bobguitypebuiltins.h"
#include "bobguiwidgetprivate.h"
#include "bobguishortcuttrigger.h"
#include "bobguibuilderprivate.h"

#include <string.h>

/* Time out before giving up on getting a key release when animating
 * the close button.
 */
#define ACTIVATE_TIMEOUT 250

struct _BobguiButtonPrivate
{
  BobguiWidget             *child;

  BobguiActionHelper       *action_helper;

  BobguiGesture            *gesture;

  guint                  activate_timeout;

  guint          button_down           : 1;
  guint          use_underline         : 1;
  guint          child_type            : 2;
  guint          can_shrink            : 1;
};

enum {
  CLICKED,
  ACTIVATE,
  LAST_SIGNAL
};

enum {
  PROP_0,
  PROP_LABEL,
  PROP_HAS_FRAME,
  PROP_USE_UNDERLINE,
  PROP_ICON_NAME,
  PROP_CHILD,
  PROP_CAN_SHRINK,

  /* actionable properties */
  PROP_ACTION_NAME,
  PROP_ACTION_TARGET,
  LAST_PROP = PROP_ACTION_NAME
};

enum {
  LABEL_CHILD,
  ICON_CHILD,
  WIDGET_CHILD
};


static void bobgui_button_dispose        (GObject            *object);
static void bobgui_button_set_property   (GObject            *object,
                                       guint               prop_id,
                                       const GValue       *value,
                                       GParamSpec         *pspec);
static void bobgui_button_get_property   (GObject            *object,
                                       guint               prop_id,
                                       GValue             *value,
                                       GParamSpec         *pspec);
static void bobgui_button_unrealize (BobguiWidget * widget);
static void bobgui_real_button_clicked (BobguiButton * button);
static void bobgui_real_button_activate  (BobguiButton          *button);
static void bobgui_button_finish_activate (BobguiButton         *button,
                                        gboolean           do_it);

static void bobgui_button_state_flags_changed (BobguiWidget     *widget,
                                            BobguiStateFlags  previous_state);
static void bobgui_button_do_release      (BobguiButton             *button,
                                        gboolean               emit_clicked);
static void bobgui_button_set_child_type (BobguiButton *button, guint child_type);

static void bobgui_button_buildable_iface_init      (BobguiBuildableIface *iface);
static void bobgui_button_actionable_iface_init     (BobguiActionableInterface *iface);

static GParamSpec *props[LAST_PROP] = { NULL, };
static guint button_signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE_WITH_CODE (BobguiButton, bobgui_button, BOBGUI_TYPE_WIDGET,
                         G_ADD_PRIVATE (BobguiButton)
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_BUILDABLE, bobgui_button_buildable_iface_init)
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_ACTIONABLE, bobgui_button_actionable_iface_init))

static void
bobgui_button_compute_expand (BobguiWidget *widget,
                           gboolean  *hexpand,
                           gboolean  *vexpand)
{
  BobguiButtonPrivate *priv = bobgui_button_get_instance_private (BOBGUI_BUTTON (widget));

  if (priv->child)
    {
      *hexpand = bobgui_widget_compute_expand (priv->child, BOBGUI_ORIENTATION_HORIZONTAL);
      *vexpand = bobgui_widget_compute_expand (priv->child, BOBGUI_ORIENTATION_VERTICAL);
    }
  else
    {
      *hexpand = FALSE;
      *vexpand = FALSE;
    }
}

static BobguiSizeRequestMode
bobgui_button_get_request_mode (BobguiWidget *widget)
{
  BobguiButtonPrivate *priv = bobgui_button_get_instance_private (BOBGUI_BUTTON (widget));

  if (priv->child)
    return bobgui_widget_get_request_mode (priv->child);
  else
    return BOBGUI_SIZE_REQUEST_CONSTANT_SIZE;
}

static void
bobgui_button_class_init (BobguiButtonClass *klass)
{
  const guint activate_keyvals[] = { GDK_KEY_space, GDK_KEY_KP_Space, GDK_KEY_Return,
                                     GDK_KEY_ISO_Enter, GDK_KEY_KP_Enter };
  GObjectClass *gobject_class;
  BobguiWidgetClass *widget_class;
  BobguiShortcutAction *activate_action;

  gobject_class = G_OBJECT_CLASS (klass);
  widget_class = (BobguiWidgetClass*) klass;

  gobject_class->dispose      = bobgui_button_dispose;
  gobject_class->set_property = bobgui_button_set_property;
  gobject_class->get_property = bobgui_button_get_property;

  widget_class->unrealize = bobgui_button_unrealize;
  widget_class->state_flags_changed = bobgui_button_state_flags_changed;
  widget_class->compute_expand = bobgui_button_compute_expand;
  widget_class->get_request_mode = bobgui_button_get_request_mode;

  klass->clicked = NULL;
  klass->activate = bobgui_real_button_activate;

  /**
   * BobguiButton:label:
   *
   * Text of the label inside the button, if the button contains a label widget.
   */
  props[PROP_LABEL] =
    g_param_spec_string ("label", NULL, NULL,
                         NULL,
                         BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiButton:use-underline:
   *
   * If set, an underline in the text indicates that the following character is
   * to be used as mnemonic.
   */
  props[PROP_USE_UNDERLINE] =
    g_param_spec_boolean ("use-underline", NULL, NULL,
                          FALSE,
                          BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiButton:has-frame:
   *
   * Whether the button has a frame.
   */
  props[PROP_HAS_FRAME] =
    g_param_spec_boolean ("has-frame", NULL, NULL,
                          TRUE,
                          BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiButton:icon-name:
   *
   * The name of the icon used to automatically populate the button.
   */
  props[PROP_ICON_NAME] =
    g_param_spec_string ("icon-name", NULL, NULL,
                         NULL,
                         BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiButton:child:
   *
   * The child widget.
   */
  props[PROP_CHILD] =
    g_param_spec_object ("child", NULL, NULL,
                         BOBGUI_TYPE_WIDGET,
                         BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiButton:can-shrink:
   *
   * Whether the size of the button can be made smaller than the natural
   * size of its contents.
   *
   * For text buttons, setting this property will allow ellipsizing the label.
   *
   * If the contents of a button are an icon or a custom widget, setting this
   * property has no effect.
   *
   * Since: 4.12
   */
  props[PROP_CAN_SHRINK] =
    g_param_spec_boolean ("can-shrink", NULL, NULL,
                          FALSE,
                          BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (gobject_class, LAST_PROP, props);

  g_object_class_override_property (gobject_class, PROP_ACTION_NAME, "action-name");
  g_object_class_override_property (gobject_class, PROP_ACTION_TARGET, "action-target");

  /**
   * BobguiButton::clicked:
   * @button: the object that received the signal
   *
   * Emitted when the button has been activated (pressed and released).
   */
  button_signals[CLICKED] =
    g_signal_new (I_("clicked"),
                  G_OBJECT_CLASS_TYPE (gobject_class),
                  G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (BobguiButtonClass, clicked),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);

  /**
   * BobguiButton::activate:
   * @widget: the object which received the signal.
   *
   * Emitted to animate press then release.
   *
   * This is an action signal. Applications should never connect
   * to this signal, but use the [signal@Bobgui.Button::clicked] signal.
   *
   * The default bindings for this signal are all forms of the
   * <kbd>␣</kbd> and <kbd>Enter</kbd> keys.
   */
  button_signals[ACTIVATE] =
    g_signal_new (I_("activate"),
                  G_OBJECT_CLASS_TYPE (gobject_class),
                  G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (BobguiButtonClass, activate),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);

  bobgui_widget_class_set_activate_signal (widget_class, button_signals[ACTIVATE]);

  activate_action = bobgui_signal_action_new ("activate");
  for (guint i = 0; i < G_N_ELEMENTS (activate_keyvals); i++)
    {
      BobguiShortcut *activate_shortcut = bobgui_shortcut_new (bobgui_keyval_trigger_new (activate_keyvals[i], 0),
                                                         g_object_ref (activate_action));

      bobgui_widget_class_add_shortcut (widget_class, activate_shortcut);
      g_object_unref (activate_shortcut);
    }
  g_object_unref (activate_action);

  bobgui_widget_class_set_accessible_role (widget_class, BOBGUI_ACCESSIBLE_ROLE_BUTTON);
  bobgui_widget_class_set_layout_manager_type (widget_class, BOBGUI_TYPE_BIN_LAYOUT);
  bobgui_widget_class_set_css_name (widget_class, I_("button"));
}

static void
click_pressed_cb (BobguiGestureClick *gesture,
                  guint            n_press,
                  double           x,
                  double           y,
                  BobguiWidget       *widget)
{
  BobguiButton *button = BOBGUI_BUTTON (widget);
  BobguiButtonPrivate *priv = bobgui_button_get_instance_private (button);

  if (bobgui_widget_get_focus_on_click (widget) && !bobgui_widget_has_focus (widget))
    bobgui_widget_grab_focus (widget);

  if (!priv->activate_timeout)
    priv->button_down = TRUE;
}

static void
click_released_cb (BobguiGestureClick *gesture,
                   guint            n_press,
                   double           x,
                   double           y,
                   BobguiWidget       *widget)
{
  BobguiButton *button = BOBGUI_BUTTON (widget);

  bobgui_gesture_set_state (BOBGUI_GESTURE (gesture), BOBGUI_EVENT_SEQUENCE_CLAIMED);
  bobgui_button_do_release (button,
                         bobgui_widget_is_sensitive (BOBGUI_WIDGET (button)) &&
                         bobgui_widget_contains (widget, x, y));
}

static void
click_gesture_cancel_cb (BobguiGesture       *gesture,
                         GdkEventSequence *sequence,
                         BobguiButton        *button)
{
  BobguiButtonPrivate *priv = bobgui_button_get_instance_private (button);

  if (priv->activate_timeout)
    bobgui_button_finish_activate (button, FALSE);

  bobgui_button_do_release (button, FALSE);
}

static gboolean
key_controller_key_pressed_cb (BobguiEventControllerKey *controller,
                               guint                  keyval,
                               guint                  keycode,
                               guint                  modifiers,
                               BobguiButton             *button)
{
  BobguiButtonPrivate *priv = bobgui_button_get_instance_private (button);

  return priv->activate_timeout != 0;
}

static void
key_controller_key_released_cb (BobguiEventControllerKey *controller,
                                guint                  keyval,
                                guint                  keycode,
                                guint                  modifiers,
                                BobguiButton             *button)
{
  BobguiButtonPrivate *priv = bobgui_button_get_instance_private (button);

  if (priv->activate_timeout)
    bobgui_button_finish_activate (button, TRUE);
}

static inline void
add_or_remove_class (BobguiWidget  *widget,
                     gboolean    add,
                     const char *class)
{
  if (add)
    bobgui_widget_add_css_class (widget, class);
  else
    bobgui_widget_remove_css_class (widget, class);
}

static void
update_style_classes_from_child_type (BobguiButton *button,
                                      guint      child_type)
{
  add_or_remove_class (BOBGUI_WIDGET (button), child_type == LABEL_CHILD, "text-button");
  add_or_remove_class (BOBGUI_WIDGET (button), child_type == ICON_CHILD, "image-button");
}

static void
bobgui_button_set_child_type (BobguiButton *button,
                           guint      child_type)
{
  BobguiButtonPrivate *priv = bobgui_button_get_instance_private (button);

  if (priv->child_type == child_type)
    return;

  update_style_classes_from_child_type (button, child_type);

  if (child_type != LABEL_CHILD)
    g_object_notify_by_pspec (G_OBJECT (button), props[PROP_LABEL]);
  else if (child_type != ICON_CHILD)
    g_object_notify_by_pspec (G_OBJECT (button), props[PROP_ICON_NAME]);

  priv->child_type = child_type;
}

static void
bobgui_button_init (BobguiButton *button)
{
  BobguiButtonPrivate *priv = bobgui_button_get_instance_private (button);
  BobguiEventController *key_controller;

  bobgui_widget_set_focusable (BOBGUI_WIDGET (button), TRUE);
  bobgui_widget_set_receives_default (BOBGUI_WIDGET (button), TRUE);

  priv->button_down = FALSE;
  priv->use_underline = FALSE;
  priv->child_type = WIDGET_CHILD;

  priv->gesture = bobgui_gesture_click_new ();
  bobgui_gesture_single_set_touch_only (BOBGUI_GESTURE_SINGLE (priv->gesture), FALSE);
  bobgui_gesture_single_set_button (BOBGUI_GESTURE_SINGLE (priv->gesture), GDK_BUTTON_PRIMARY);
  g_signal_connect (priv->gesture, "pressed", G_CALLBACK (click_pressed_cb), button);
  g_signal_connect (priv->gesture, "released", G_CALLBACK (click_released_cb), button);
  g_signal_connect (priv->gesture, "cancel", G_CALLBACK (click_gesture_cancel_cb), button);
  bobgui_event_controller_set_propagation_phase (BOBGUI_EVENT_CONTROLLER (priv->gesture), BOBGUI_PHASE_CAPTURE);
  bobgui_widget_add_controller (BOBGUI_WIDGET (button), BOBGUI_EVENT_CONTROLLER (priv->gesture));

  key_controller = bobgui_event_controller_key_new ();
  g_signal_connect (key_controller, "key-pressed", G_CALLBACK (key_controller_key_pressed_cb), button);
  g_signal_connect (key_controller, "key-released", G_CALLBACK (key_controller_key_released_cb), button);
  bobgui_widget_add_controller (BOBGUI_WIDGET (button), key_controller);
}

static void
bobgui_button_dispose (GObject *object)
{
  BobguiButton *button = BOBGUI_BUTTON (object);
  BobguiButtonPrivate *priv = bobgui_button_get_instance_private (button);

  g_clear_pointer (&priv->child, bobgui_widget_unparent);
  g_clear_object (&priv->action_helper);

  G_OBJECT_CLASS (bobgui_button_parent_class)->dispose (object);
}

static void
bobgui_button_set_action_name (BobguiActionable *actionable,
                            const char    *action_name)
{
  BobguiButton *button = BOBGUI_BUTTON (actionable);
  BobguiButtonPrivate *priv = bobgui_button_get_instance_private (button);

  if (!priv->action_helper)
    priv->action_helper = bobgui_action_helper_new (actionable);

  g_signal_handlers_disconnect_by_func (button, bobgui_real_button_clicked, NULL);
  if (action_name)
    g_signal_connect_after (button, "clicked", G_CALLBACK (bobgui_real_button_clicked), NULL);

  bobgui_action_helper_set_action_name (priv->action_helper, action_name);
}

static void
bobgui_button_set_action_target_value (BobguiActionable *actionable,
                                    GVariant      *action_target)
{
  BobguiButton *button = BOBGUI_BUTTON (actionable);
  BobguiButtonPrivate *priv = bobgui_button_get_instance_private (button);

  if (!priv->action_helper)
    priv->action_helper = bobgui_action_helper_new (actionable);

  bobgui_action_helper_set_action_target_value (priv->action_helper, action_target);
}

static void
bobgui_button_set_property (GObject         *object,
                         guint            prop_id,
                         const GValue    *value,
                         GParamSpec      *pspec)
{
  BobguiButton *button = BOBGUI_BUTTON (object);

  switch (prop_id)
    {
    case PROP_LABEL:
      bobgui_button_set_label (button, g_value_get_string (value));
      break;
    case PROP_HAS_FRAME:
      bobgui_button_set_has_frame (button, g_value_get_boolean (value));
      break;
    case PROP_USE_UNDERLINE:
      bobgui_button_set_use_underline (button, g_value_get_boolean (value));
      break;
    case PROP_ICON_NAME:
      bobgui_button_set_icon_name (button, g_value_get_string (value));
      break;
    case PROP_CHILD:
      bobgui_button_set_child (button, g_value_get_object (value));
      break;
    case PROP_CAN_SHRINK:
      bobgui_button_set_can_shrink (button, g_value_get_boolean (value));
      break;
    case PROP_ACTION_NAME:
      bobgui_button_set_action_name (BOBGUI_ACTIONABLE (button), g_value_get_string (value));
      break;
    case PROP_ACTION_TARGET:
      bobgui_button_set_action_target_value (BOBGUI_ACTIONABLE (button), g_value_get_variant (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_button_get_property (GObject         *object,
                         guint            prop_id,
                         GValue          *value,
                         GParamSpec      *pspec)
{
  BobguiButton *button = BOBGUI_BUTTON (object);
  BobguiButtonPrivate *priv = bobgui_button_get_instance_private (button);

  switch (prop_id)
    {
    case PROP_LABEL:
      g_value_set_string (value, bobgui_button_get_label (button));
      break;
    case PROP_HAS_FRAME:
      g_value_set_boolean (value, bobgui_button_get_has_frame (button));
      break;
    case PROP_USE_UNDERLINE:
      g_value_set_boolean (value, priv->use_underline);
      break;
    case PROP_ICON_NAME:
      g_value_set_string (value, bobgui_button_get_icon_name (button));
      break;
    case PROP_CHILD:
      g_value_set_object (value, priv->child);
      break;
    case PROP_CAN_SHRINK:
      g_value_set_boolean (value, priv->can_shrink);
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
bobgui_button_get_action_name (BobguiActionable *actionable)
{
  BobguiButton *button = BOBGUI_BUTTON (actionable);
  BobguiButtonPrivate *priv = bobgui_button_get_instance_private (button);

  return bobgui_action_helper_get_action_name (priv->action_helper);
}

static GVariant *
bobgui_button_get_action_target_value (BobguiActionable *actionable)
{
  BobguiButton *button = BOBGUI_BUTTON (actionable);
  BobguiButtonPrivate *priv = bobgui_button_get_instance_private (button);

  return bobgui_action_helper_get_action_target_value (priv->action_helper);
}

static void
bobgui_button_actionable_iface_init (BobguiActionableInterface *iface)
{
  iface->get_action_name = bobgui_button_get_action_name;
  iface->set_action_name = bobgui_button_set_action_name;
  iface->get_action_target_value = bobgui_button_get_action_target_value;
  iface->set_action_target_value = bobgui_button_set_action_target_value;
}

static BobguiBuildableIface *parent_buildable_iface;

static void
bobgui_button_buildable_add_child (BobguiBuildable *buildable,
                                BobguiBuilder   *builder,
                                GObject      *child,
                                const char   *type)
{
  if (BOBGUI_IS_WIDGET (child))
    {
      bobgui_buildable_child_deprecation_warning (buildable, builder, NULL, "child");
      bobgui_button_set_child (BOBGUI_BUTTON (buildable), BOBGUI_WIDGET (child));
    }
  else
    {
      parent_buildable_iface->add_child (buildable, builder, child, type);
    }
}

static void
bobgui_button_buildable_iface_init (BobguiBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);

  iface->add_child = bobgui_button_buildable_add_child;
}

/**
 * bobgui_button_new:
 *
 * Creates a new `BobguiButton` widget.
 *
 * To add a child widget to the button, use [method@Bobgui.Button.set_child].
 *
 * Returns: The newly created `BobguiButton` widget.
 */
BobguiWidget*
bobgui_button_new (void)
{
  return g_object_new (BOBGUI_TYPE_BUTTON, NULL);
}

/**
 * bobgui_button_new_with_label:
 * @label: The text you want the `BobguiLabel` to hold
 *
 * Creates a `BobguiButton` widget with a `BobguiLabel` child.
 *
 * Returns: The newly created `BobguiButton` widget
 */
BobguiWidget*
bobgui_button_new_with_label (const char *label)
{
  return g_object_new (BOBGUI_TYPE_BUTTON, "label", label, NULL);
}

/**
 * bobgui_button_new_from_icon_name:
 * @icon_name: an icon name
 *
 * Creates a new button containing an icon from the current icon theme.
 *
 * If the icon name isn’t known, a “broken image” icon will be
 * displayed instead. If the current icon theme is changed, the icon
 * will be updated appropriately.
 *
 * Returns: a new `BobguiButton` displaying the themed icon
 */
BobguiWidget*
bobgui_button_new_from_icon_name (const char *icon_name)
{
  BobguiWidget *button;

  button = g_object_new (BOBGUI_TYPE_BUTTON,
                         "icon-name", icon_name,
                         NULL);

  return button;
}

/**
 * bobgui_button_new_with_mnemonic:
 * @label: The text of the button, with an underscore in front of the
 *   mnemonic character
 *
 * Creates a new `BobguiButton` containing a label.
 *
 * If characters in @label are preceded by an underscore, they are underlined.
 * If you need a literal underscore character in a label, use “__” (two
 * underscores). The first underlined character represents a keyboard
 * accelerator called a mnemonic. Pressing <kbd>Alt</kbd> and that key
 * activates the button.
 *
 * Returns: a new `BobguiButton`
 */
BobguiWidget*
bobgui_button_new_with_mnemonic (const char *label)
{
  return g_object_new (BOBGUI_TYPE_BUTTON, "label", label, "use-underline", TRUE,  NULL);
}

/**
 * bobgui_button_set_has_frame:
 * @button: a `BobguiButton`
 * @has_frame: whether the button should have a visible frame
 *
 * Sets the style of the button.
 *
 * Buttons can have a flat appearance or have a frame drawn around them.
 */
void
bobgui_button_set_has_frame (BobguiButton *button,
                          gboolean   has_frame)
{

  g_return_if_fail (BOBGUI_IS_BUTTON (button));

  if (bobgui_button_get_has_frame (button) == has_frame)
    return;

  if (has_frame)
    bobgui_widget_remove_css_class (BOBGUI_WIDGET (button), "flat");
  else
    bobgui_widget_add_css_class (BOBGUI_WIDGET (button), "flat");

  g_object_notify_by_pspec (G_OBJECT (button), props[PROP_HAS_FRAME]);
}

/**
 * bobgui_button_get_has_frame:
 * @button: a `BobguiButton`
 *
 * Returns whether the button has a frame.
 *
 * Returns: %TRUE if the button has a frame
 */
gboolean
bobgui_button_get_has_frame (BobguiButton *button)
{
  g_return_val_if_fail (BOBGUI_IS_BUTTON (button), TRUE);

  return !bobgui_widget_has_css_class (BOBGUI_WIDGET (button), "flat");
}

static void
bobgui_button_unrealize (BobguiWidget *widget)
{
  BobguiButton *button = BOBGUI_BUTTON (widget);
  BobguiButtonPrivate *priv = bobgui_button_get_instance_private (button);

  if (priv->activate_timeout)
    bobgui_button_finish_activate (button, FALSE);

  BOBGUI_WIDGET_CLASS (bobgui_button_parent_class)->unrealize (widget);
}

static void
bobgui_button_do_release (BobguiButton *button,
                       gboolean   emit_clicked)
{
  BobguiButtonPrivate *priv = bobgui_button_get_instance_private (button);

  if (priv->button_down)
    {
      priv->button_down = FALSE;

      if (priv->activate_timeout)
        return;

      if (emit_clicked)
        g_signal_emit (button, button_signals[CLICKED], 0);
    }
}

static void
bobgui_real_button_clicked (BobguiButton *button)
{
  BobguiButtonPrivate *priv = bobgui_button_get_instance_private (button);

  if (priv->action_helper)
    bobgui_action_helper_activate (priv->action_helper);
}

static gboolean
button_activate_timeout (gpointer data)
{
  bobgui_button_finish_activate (data, TRUE);

  return FALSE;
}

static void
bobgui_real_button_activate (BobguiButton *button)
{
  BobguiWidget *widget = BOBGUI_WIDGET (button);
  BobguiButtonPrivate *priv = bobgui_button_get_instance_private (button);

  if (bobgui_widget_get_realized (widget) && !priv->activate_timeout)
    {
      priv->activate_timeout = g_timeout_add (ACTIVATE_TIMEOUT, button_activate_timeout, button);
      gdk_source_set_static_name_by_id (priv->activate_timeout, "[bobgui] button_activate_timeout");

      bobgui_widget_add_css_class (BOBGUI_WIDGET (button), "keyboard-activating");
      priv->button_down = TRUE;
    }
}

static void
bobgui_button_finish_activate (BobguiButton *button,
                            gboolean   do_it)
{
  BobguiButtonPrivate *priv = bobgui_button_get_instance_private (button);

  bobgui_widget_remove_css_class (BOBGUI_WIDGET (button), "keyboard-activating");

  g_source_remove (priv->activate_timeout);
  priv->activate_timeout = 0;

  priv->button_down = FALSE;

  if (do_it)
    g_signal_emit (button, button_signals[CLICKED], 0);
}

/**
 * bobgui_button_set_label:
 * @button: a `BobguiButton`
 * @label: a string
 *
 * Sets the text of the label of the button to @label.
 *
 * This will also clear any previously set labels.
 */
void
bobgui_button_set_label (BobguiButton  *button,
                      const char *label)
{
  BobguiButtonPrivate *priv = bobgui_button_get_instance_private (button);
  BobguiWidget *child;

  g_return_if_fail (BOBGUI_IS_BUTTON (button));

  if (priv->child_type != LABEL_CHILD || priv->child == NULL)
    {
      child = bobgui_label_new (NULL);
      bobgui_button_set_child (button,  child);
      if (priv->use_underline)
        {
          bobgui_label_set_use_underline (BOBGUI_LABEL (child), priv->use_underline);
          bobgui_label_set_mnemonic_widget (BOBGUI_LABEL (child), BOBGUI_WIDGET (button));
        }
      else
        {
          bobgui_accessible_update_relation (BOBGUI_ACCESSIBLE (button),
                                          BOBGUI_ACCESSIBLE_RELATION_LABELLED_BY, child, NULL,
                                          -1);
        }
    }

  bobgui_label_set_label (BOBGUI_LABEL (priv->child), label);
  bobgui_label_set_ellipsize (BOBGUI_LABEL (priv->child),
                           priv->can_shrink ? PANGO_ELLIPSIZE_END
                                            : PANGO_ELLIPSIZE_NONE);

  bobgui_button_set_child_type (button, LABEL_CHILD);

  g_object_notify_by_pspec (G_OBJECT (button), props[PROP_LABEL]);
}

/**
 * bobgui_button_get_label:
 * @button: a `BobguiButton`
 *
 * Fetches the text from the label of the button.
 *
 * If the label text has not been set with [method@Bobgui.Button.set_label]
 * the return value will be %NULL. This will be the case if you create
 * an empty button with [ctor@Bobgui.Button.new] to use as a container.
 *
 * Returns: (nullable): The text of the label widget. This string is owned
 * by the widget and must not be modified or freed.
 */
const char *
bobgui_button_get_label (BobguiButton *button)
{
  BobguiButtonPrivate *priv = bobgui_button_get_instance_private (button);

  g_return_val_if_fail (BOBGUI_IS_BUTTON (button), NULL);

  if (priv->child_type == LABEL_CHILD)
    return bobgui_label_get_label (BOBGUI_LABEL (priv->child));

  return NULL;
}

/**
 * bobgui_button_set_use_underline:
 * @button: a `BobguiButton`
 * @use_underline: %TRUE if underlines in the text indicate mnemonics
 *
 * Sets whether to use underlines as mnemonics.
 *
 * If true, an underline in the text of the button label indicates
 * the next character should be used for the mnemonic accelerator key.
 */
void
bobgui_button_set_use_underline (BobguiButton *button,
                              gboolean   use_underline)
{
  BobguiButtonPrivate *priv = bobgui_button_get_instance_private (button);

  g_return_if_fail (BOBGUI_IS_BUTTON (button));

  use_underline = use_underline != FALSE;

  if (use_underline != priv->use_underline)
    {
      if (priv->child_type == LABEL_CHILD)
        {
          bobgui_label_set_use_underline (BOBGUI_LABEL (priv->child), use_underline);
          bobgui_label_set_mnemonic_widget (BOBGUI_LABEL (priv->child), BOBGUI_WIDGET (button));
        }

      priv->use_underline = use_underline;
      g_object_notify_by_pspec (G_OBJECT (button), props[PROP_USE_UNDERLINE]);
    }
}

/**
 * bobgui_button_get_use_underline:
 * @button: a `BobguiButton`
 *
 * gets whether underlines are interpreted as mnemonics.
 *
 * See [method@Bobgui.Button.set_use_underline].
 *
 * Returns: %TRUE if an embedded underline in the button label
 *   indicates the mnemonic accelerator keys.
 */
gboolean
bobgui_button_get_use_underline (BobguiButton *button)
{
  BobguiButtonPrivate *priv = bobgui_button_get_instance_private (button);

  g_return_val_if_fail (BOBGUI_IS_BUTTON (button), FALSE);

  return priv->use_underline;
}

static void
bobgui_button_state_flags_changed (BobguiWidget     *widget,
                                BobguiStateFlags  previous_state)
{
  BobguiButton *button = BOBGUI_BUTTON (widget);

  if (!bobgui_widget_is_sensitive (widget))
    bobgui_button_do_release (button, FALSE);
}

/**
 * bobgui_button_set_icon_name:
 * @button: A `BobguiButton`
 * @icon_name: An icon name
 *
 * Adds a `BobguiImage` with the given icon name as a child.
 *
 * If @button already contains a child widget, that child widget will
 * be removed and replaced with the image.
 */
void
bobgui_button_set_icon_name (BobguiButton  *button,
                          const char *icon_name)
{
  BobguiButtonPrivate *priv = bobgui_button_get_instance_private (button);

  g_return_if_fail (BOBGUI_IS_BUTTON (button));
  g_return_if_fail (icon_name != NULL);

  if (priv->child_type != ICON_CHILD || priv->child == NULL)
    {
      BobguiWidget *child = g_object_new (BOBGUI_TYPE_IMAGE,
                                       "accessible-role", BOBGUI_ACCESSIBLE_ROLE_PRESENTATION,
                                       "icon-name", icon_name,
                                       NULL);
      bobgui_button_set_child (BOBGUI_BUTTON (button), child);
      bobgui_widget_set_valign (child, BOBGUI_ALIGN_CENTER);
    }
  else
    {
      bobgui_image_set_from_icon_name (BOBGUI_IMAGE (priv->child), icon_name);
    }

  bobgui_button_set_child_type (button, ICON_CHILD);
  g_object_notify_by_pspec (G_OBJECT (button), props[PROP_ICON_NAME]);
}

/**
 * bobgui_button_get_icon_name:
 * @button: A `BobguiButton`
 *
 * Returns the icon name of the button.
 *
 * If the icon name has not been set with [method@Bobgui.Button.set_icon_name]
 * the return value will be %NULL. This will be the case if you create
 * an empty button with [ctor@Bobgui.Button.new] to use as a container.
 *
 * Returns: (nullable): The icon name set via [method@Bobgui.Button.set_icon_name]
 */
const char *
bobgui_button_get_icon_name (BobguiButton *button)
{
  BobguiButtonPrivate *priv = bobgui_button_get_instance_private (button);

  g_return_val_if_fail (BOBGUI_IS_BUTTON (button), NULL);

  if (priv->child_type == ICON_CHILD)
    return bobgui_image_get_icon_name (BOBGUI_IMAGE (priv->child));

  return NULL;
}

BobguiGesture *
bobgui_button_get_gesture (BobguiButton *button)
{
  BobguiButtonPrivate *priv = bobgui_button_get_instance_private (button);

  return priv->gesture;
}

BobguiActionHelper *
bobgui_button_get_action_helper (BobguiButton *button)
{
  BobguiButtonPrivate *priv = bobgui_button_get_instance_private (button);

  return priv->action_helper;
}

/**
 * bobgui_button_set_child:
 * @button: a `BobguiButton`
 * @child: (nullable): the child widget
 *
 * Sets the child widget of @button.
 *
 * Note that by using this API, you take full responsibility for setting
 * up the proper accessibility label and description information for @button.
 * Most likely, you'll either set the accessibility label or description
 * for @button explicitly, or you'll set a labelled-by or described-by
 * relations from @child to @button.
 */
void
bobgui_button_set_child (BobguiButton *button,
                      BobguiWidget *child)
{
  BobguiButtonPrivate *priv = bobgui_button_get_instance_private (button);

  g_return_if_fail (BOBGUI_IS_BUTTON (button));
  g_return_if_fail (child == NULL || priv->child == child || bobgui_widget_get_parent (child) == NULL);

  if (priv->child == child)
    return;

  g_clear_pointer (&priv->child, bobgui_widget_unparent);

  priv->child = child;

  if (priv->child)
    bobgui_widget_set_parent (priv->child, BOBGUI_WIDGET (button));

  bobgui_button_set_child_type (button, WIDGET_CHILD);

  if (BOBGUI_IS_IMAGE (child))
    update_style_classes_from_child_type (button, ICON_CHILD);
  else if (BOBGUI_IS_LABEL (child))
    update_style_classes_from_child_type (button, LABEL_CHILD);

  g_object_notify_by_pspec (G_OBJECT (button), props[PROP_CHILD]);
}

/**
 * bobgui_button_get_child:
 * @button: a `BobguiButton`
 *
 * Gets the child widget of @button.
 *
 * Returns: (nullable) (transfer none): the child widget of @button
 */
BobguiWidget *
bobgui_button_get_child (BobguiButton *button)
{
  BobguiButtonPrivate *priv = bobgui_button_get_instance_private (button);

  g_return_val_if_fail (BOBGUI_IS_BUTTON (button), NULL);

  return priv->child;
}

/**
 * bobgui_button_set_can_shrink:
 * @button: a button
 * @can_shrink: whether the button can shrink
 *
 * Sets whether the button size can be smaller than the natural size of
 * its contents.
 *
 * For text buttons, setting @can_shrink to true will ellipsize the label.
 *
 * For icons and custom children, this function has no effect.
 *
 * Since: 4.12
 */
void
bobgui_button_set_can_shrink (BobguiButton *button,
                           gboolean   can_shrink)
{
  BobguiButtonPrivate *priv = bobgui_button_get_instance_private (button);

  g_return_if_fail (BOBGUI_IS_BUTTON (button));

  can_shrink = !!can_shrink;

  if (priv->can_shrink != can_shrink)
    {
      priv->can_shrink = can_shrink;

      switch (priv->child_type)
        {
        case LABEL_CHILD:
          bobgui_label_set_ellipsize (BOBGUI_LABEL (priv->child),
                                   priv->can_shrink ? PANGO_ELLIPSIZE_END
                                                    : PANGO_ELLIPSIZE_NONE);
          break;

        case ICON_CHILD:
        case WIDGET_CHILD:
          break;

        default:
          g_assert_not_reached ();
          break;
        }

      g_object_notify_by_pspec (G_OBJECT (button), props[PROP_CAN_SHRINK]);
    }
}

/**
 * bobgui_button_get_can_shrink:
 * @button: a button
 *
 * Retrieves whether the button can be smaller than the natural
 * size of its contents.
 *
 * Returns: true if the button can shrink, and false otherwise
 *
 * Since: 4.12
 */
gboolean
bobgui_button_get_can_shrink (BobguiButton *button)
{
  BobguiButtonPrivate *priv = bobgui_button_get_instance_private (button);

  g_return_val_if_fail (BOBGUI_IS_BUTTON (button), FALSE);

  return priv->can_shrink;
}
