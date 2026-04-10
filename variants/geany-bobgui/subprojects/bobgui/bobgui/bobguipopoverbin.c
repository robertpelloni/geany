/* bobguipopoverbin.c: A single-child container with a popover
 *
 * SPDX-FileCopyrightText: 2025  Emmanuele Bassi
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "config.h"

#include "bobguipopoverbin.h"

#include "bobguibinlayout.h"
#include "bobguibuildable.h"
#include "bobguibuilderprivate.h"
#include "bobguipopovermenu.h"
#include "bobguiprivate.h"
#include "bobguiwidgetprivate.h"
#include "bobguigestureclick.h"
#include "bobguigesturelongpress.h"
#include "bobguishortcutcontroller.h"
#include "bobguishortcutaction.h"
#include "bobguishortcuttrigger.h"

/**
 * BobguiPopoverBin:
 *
 * A single child container with a popover.
 *
 * You should use `BobguiPopoverBin` whenever you need to present a [class@Bobgui.Popover]
 * to the user.
 *
 * ## Actions
 *
 * `BobguiPopoverBin` defines the `menu.popup` action, which can be activated
 * to present the popover to the user.
 *
 * ## CSS nodes
 *
 * `BobguiPopoverBin` has a single CSS node with the name `popoverbin`.
 *
 * Since: 4.22
 */

struct _BobguiPopoverBin
{
  BobguiWidget parent_instance;

  BobguiWidget *child;
  BobguiWidget *popover;

  GMenuModel *menu_model;

  gboolean handle_input;
  BobguiEventController *click_gesture;
  BobguiEventController *long_press_gesture;
  BobguiEventController *shortcut_controller;
};

enum
{
  PROP_CHILD = 1,
  PROP_POPOVER,
  PROP_MENU_MODEL,
  PROP_HANDLE_INPUT,
  N_PROPS
};

static GParamSpec *obj_props[N_PROPS];

static BobguiBuildableIface *parent_buildable_iface;

static void bobgui_popover_bin_buildable_iface_init (BobguiBuildableIface *iface);

G_DEFINE_FINAL_TYPE_WITH_CODE (BobguiPopoverBin, bobgui_popover_bin, BOBGUI_TYPE_WIDGET,
                               G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_BUILDABLE,
                                                      bobgui_popover_bin_buildable_iface_init))

static void
bobgui_popover_bin_buildable_add_child (BobguiBuildable *buildable,
                                     BobguiBuilder   *builder,
                                     GObject      *child,
                                     const char   *type)
{
  if (BOBGUI_IS_WIDGET (child))
    {
      if (BOBGUI_IS_POPOVER (child))
        {
          bobgui_buildable_child_deprecation_warning (buildable, builder, NULL, "popover");
          bobgui_popover_bin_set_popover (BOBGUI_POPOVER_BIN (buildable), BOBGUI_WIDGET (child));
        }
      else
        {
          bobgui_buildable_child_deprecation_warning (buildable, builder, NULL, "child");
          bobgui_popover_bin_set_child (BOBGUI_POPOVER_BIN (buildable), BOBGUI_WIDGET (child));
        }
    }
  else
    {
      parent_buildable_iface->add_child (buildable, builder, child, type);
    }
}

static void
bobgui_popover_bin_buildable_iface_init (BobguiBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);

  iface->add_child = bobgui_popover_bin_buildable_add_child;
}

static void
on_popover_destroy (BobguiPopoverBin *self)
{
  bobgui_popover_bin_set_popover (self, NULL);
}

static void
on_popover_map (BobguiPopoverBin *self)
{
  bobgui_widget_add_css_class (self->child, "has-open-popup");
}

static void
on_popover_unmap (BobguiPopoverBin *self)
{
  bobgui_widget_remove_css_class (self->child, "has-open-popup");
}

static void
bobgui_popover_bin_popup_at_position (BobguiPopoverBin   *self,
                                   gdouble          x,
                                   gdouble          y)
{
  GdkRectangle rect;

  if (self->popover == NULL)
    return;

  if (x > -0.5 && y > -0.5) {
    rect.x = x;
    rect.y = y;
  } else {
    if (bobgui_widget_get_direction (BOBGUI_WIDGET (self)) == BOBGUI_TEXT_DIR_RTL)
      rect.x = bobgui_widget_get_width (BOBGUI_WIDGET (self));
    else
      rect.x = 0.0;

    rect.y = bobgui_widget_get_height (BOBGUI_WIDGET (self));
  }

  rect.width = 0.0;
  rect.height = 0.0;

  bobgui_popover_set_pointing_to (BOBGUI_POPOVER (self->popover), &rect);

  bobgui_popover_popup (BOBGUI_POPOVER (self->popover));
}

static void
pressed_cb (BobguiPopoverBin *self,
            int            n_press,
            double         x,
            double         y,
            BobguiGesture    *gesture)
{
  GdkEventSequence *current = bobgui_gesture_single_get_current_sequence (BOBGUI_GESTURE_SINGLE (gesture));
  GdkEvent *event = bobgui_gesture_get_last_event (gesture, current);

  if (gdk_event_triggers_context_menu (event))
    {
      bobgui_popover_bin_popup_at_position (self, x, y);
      bobgui_gesture_set_state (gesture, BOBGUI_EVENT_SEQUENCE_CLAIMED);
      bobgui_event_controller_reset (BOBGUI_EVENT_CONTROLLER (gesture));

      return;
    }

  bobgui_gesture_set_state (gesture, BOBGUI_EVENT_SEQUENCE_DENIED);
}

static void
long_pressed_cb (BobguiPopoverBin *self,
                 double         x,
                 double         y,
                 BobguiGesture    *gesture)
{
  bobgui_gesture_set_state (gesture, BOBGUI_EVENT_SEQUENCE_CLAIMED);
  bobgui_popover_bin_popup_at_position (self, x, y);
}

static void
popup_action (BobguiWidget  *widget,
              const char *action_name,
              GVariant   *parameters)
{
  BobguiPopoverBin *self = BOBGUI_POPOVER_BIN (widget);

  bobgui_popover_bin_popup (self);
}

static void
bobgui_popover_bin_dispose (GObject *gobject)
{
  BobguiPopoverBin *self = BOBGUI_POPOVER_BIN (gobject);

  if (self->popover != NULL)
    {
      g_signal_handlers_disconnect_by_func (self->popover,
                                            on_popover_destroy,
                                            self);
      g_signal_handlers_disconnect_by_func (self->popover,
                                            on_popover_map,
                                            self);
      g_signal_handlers_disconnect_by_func (self->popover,
                                            on_popover_unmap,
                                            self);
    }

  g_clear_pointer (&self->popover, bobgui_widget_unparent);
  g_clear_pointer (&self->child, bobgui_widget_unparent);

  g_clear_object (&self->menu_model);

  G_OBJECT_CLASS (bobgui_popover_bin_parent_class)->dispose (gobject);
}

static void
bobgui_popover_bin_set_property (GObject *gobject,
                              unsigned int prop_id,
                              const GValue *value,
                              GParamSpec *pspec)
{
  BobguiPopoverBin *self = BOBGUI_POPOVER_BIN (gobject);

  switch (prop_id)
    {
      case PROP_MENU_MODEL:
        bobgui_popover_bin_set_menu_model (self, g_value_get_object (value));
        break;

      case PROP_POPOVER:
        bobgui_popover_bin_set_popover (self, g_value_get_object (value));
        break;

      case PROP_CHILD:
        bobgui_popover_bin_set_child (self, g_value_get_object (value));
        break;

      case PROP_HANDLE_INPUT:
        bobgui_popover_bin_set_handle_input (self, g_value_get_boolean (value));
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
    }
}

static void
bobgui_popover_bin_get_property (GObject *gobject,
                              unsigned int prop_id,
                              GValue *value,
                              GParamSpec *pspec)
{
  BobguiPopoverBin *self = BOBGUI_POPOVER_BIN (gobject);

  switch (prop_id)
    {
      case PROP_MENU_MODEL:
        g_value_set_object (value, self->menu_model);
        break;

      case PROP_POPOVER:
        g_value_set_object (value, self->popover);
        break;

      case PROP_CHILD:
        g_value_set_object (value, self->child);
        break;

      case PROP_HANDLE_INPUT:
        g_value_set_boolean (value, self->handle_input);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
    }
}

static void
bobgui_popover_bin_compute_expand (BobguiWidget *widget,
                                gboolean *hexpand_p,
                                gboolean *vexpand_p)
{
  BobguiPopoverBin *self = BOBGUI_POPOVER_BIN (widget);
  gboolean hexpand = FALSE, vexpand = FALSE;

  if (self->child != NULL)
    {
      hexpand = bobgui_widget_compute_expand (self->child, BOBGUI_ORIENTATION_HORIZONTAL);
      vexpand = bobgui_widget_compute_expand (self->child, BOBGUI_ORIENTATION_VERTICAL);
    }

  if (hexpand_p != NULL)
    *hexpand_p = hexpand;
  if (vexpand_p != NULL)
    *vexpand_p = vexpand;
}

static void
bobgui_popover_bin_class_init (BobguiPopoverBinClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);

  gobject_class->set_property = bobgui_popover_bin_set_property;
  gobject_class->get_property = bobgui_popover_bin_get_property;
  gobject_class->dispose = bobgui_popover_bin_dispose;

  widget_class->focus = bobgui_widget_focus_child;
  widget_class->grab_focus = bobgui_widget_grab_focus_child;
  widget_class->compute_expand = bobgui_popover_bin_compute_expand;

  /**
   * BobguiPopoverBin:menu-model:
   *
   * The `GMenuModel` from which the popup will be created.
   *
   * See [method@Bobgui.PopoverBin.set_menu_model] for the interaction
   * with the [property@Bobgui.PopoverBin:popover] property.
   *
   * Since: 4.22
   */
  obj_props[PROP_MENU_MODEL] =
      g_param_spec_object ("menu-model", NULL, NULL,
                           G_TYPE_MENU_MODEL,
                           BOBGUI_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiPopoverBin:popover:
   *
   * The `BobguiPopover` that will be popped up when calling
   * [method@Bobgui.PopoverBin.popup].
   *
   * Since: 4.22
   */
  obj_props[PROP_POPOVER] =
      g_param_spec_object ("popover", NULL, NULL,
                           BOBGUI_TYPE_POPOVER,
                           BOBGUI_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiPopoverBin:child:
   *
   * The child widget of the popover bin.
   *
   * Since: 4.22
   */
  obj_props[PROP_CHILD] =
      g_param_spec_object ("child", NULL, NULL,
                           BOBGUI_TYPE_WIDGET,
                           BOBGUI_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiPopoverBin:handle-input:
   *
   * Whether the popover bin will handle input
   * to trigger the popup.
   *
   * Since: 4.22
   */
  obj_props[PROP_HANDLE_INPUT] =
      g_param_spec_boolean ("handle-input", NULL, NULL,
                            FALSE,
                            BOBGUI_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (gobject_class, N_PROPS, obj_props);

  bobgui_widget_class_install_action (widget_class, "menu.popup", NULL, popup_action);

  bobgui_widget_class_set_layout_manager_type (widget_class, BOBGUI_TYPE_BIN_LAYOUT);

  bobgui_widget_class_set_css_name (widget_class, "popoverbin");
}

static void
bobgui_popover_bin_init (BobguiPopoverBin *self)
{
}

/**
 * bobgui_popover_bin_new:
 *
 * Creates a new popover bin widget.
 *
 * Returns: (transfer floating): the newly created popover bin
 *
 * Since: 4.22
 */
BobguiWidget *
bobgui_popover_bin_new (void)
{
  return g_object_new (BOBGUI_TYPE_POPOVER_BIN, NULL);
}

/**
 * bobgui_popover_bin_set_child:
 * @self: a popover bin
 * @child: (nullable): the child of the popover bin
 *
 * Sets the child of the popover bin.
 *
 * Since: 4.22
 */
void
bobgui_popover_bin_set_child (BobguiPopoverBin *self,
                           BobguiWidget     *child)
{
  g_return_if_fail (BOBGUI_IS_POPOVER_BIN (self));
  g_return_if_fail (child == NULL || BOBGUI_IS_WIDGET (child));

  if (self->child == child)
    return;

  g_clear_pointer (&self->child, bobgui_widget_unparent);

  if (child != NULL)
    {
      bobgui_widget_set_parent (child, BOBGUI_WIDGET (self));
      self->child = child;
    }

  g_object_notify_by_pspec (G_OBJECT (self), obj_props[PROP_CHILD]);
}

/**
 * bobgui_popover_bin_get_child:
 * @self: a popover bin
 *
 * Retrieves the child widget of the popover bin.
 *
 * Returns: (transfer none) (nullable): the child widget
 *
 * Since: 4.22
 */
BobguiWidget *
bobgui_popover_bin_get_child (BobguiPopoverBin *self)
{
  g_return_val_if_fail (BOBGUI_IS_POPOVER_BIN (self), NULL);

  return self->child;
}

/**
 * bobgui_popover_bin_set_menu_model:
 * @self: a popover bin
 * @model: (nullable): a menu model
 *
 * Sets the menu model used to create the popover that will be
 * presented when calling [method@Bobgui.PopoverBin.popup].
 *
 * If @model is `NULL`, the popover will be unset.
 *
 * A [class@Bobgui.Popover] will be created from the menu model with
 * [ctor@Bobgui.PopoverMenu.new_from_model]. Actions will be connected
 * as documented for this function.
 *
 * If [property@Bobgui.PopoverBin:popover] is already set, it will be
 * dissociated from the popover bin, and the property is set to `NULL`.
 *
 * See: [method@Bobgui.PopoverBin.set_popover]
 *
 * Since: 4.22
 */
void
bobgui_popover_bin_set_menu_model (BobguiPopoverBin *self,
                                GMenuModel    *model)
{
  g_return_if_fail (BOBGUI_IS_POPOVER_BIN (self));
  g_return_if_fail (model == NULL || G_IS_MENU_MODEL (model));

  if (self->menu_model == model)
    return;

  g_object_freeze_notify (G_OBJECT (self));

  if (model != NULL)
    g_object_ref (model);

  if (model != NULL)
    {
      BobguiWidget *popover;

      popover = bobgui_popover_menu_new_from_model (model);
      bobgui_popover_bin_set_popover (self, popover);
    }
  else
    {
      bobgui_popover_bin_set_popover (self, NULL);
    }

  self->menu_model = model;
  g_object_notify_by_pspec (G_OBJECT (self), obj_props[PROP_MENU_MODEL]);

  g_object_thaw_notify (G_OBJECT (self));
}

/**
 * bobgui_popover_bin_get_menu_model:
 * @self: a popover bin
 *
 * Retrieves the menu model set using [method@Bobgui.PopoverBin.set_menu_model].
 *
 * Returns: (transfer none) (nullable): the menu model for the popover
 *
 * Since: 4.22
 */
GMenuModel *
bobgui_popover_bin_get_menu_model (BobguiPopoverBin *self)
{
  g_return_val_if_fail (BOBGUI_IS_POPOVER_BIN (self), NULL);

  return self->menu_model;
}

/**
 * bobgui_popover_bin_set_popover:
 * @self: a popover bin
 * @popover: (nullable) (type Bobgui.Popover): a `BobguiPopover`
 *
 * Sets the `BobguiPopover` that will be presented when calling
 * [method@Bobgui.PopoverBin.popup].
 *
 * If @popover is `NULL`, the popover will be unset.
 *
 * If [property@Bobgui.PopoverBin:menu-model] is set before calling
 * this function, then the menu model property will be unset.
 *
 * See: [method@Bobgui.PopoverBin.set_menu_model]
 *
 * Since: 4.22
 */
void
bobgui_popover_bin_set_popover (BobguiPopoverBin *self,
                             BobguiWidget     *popover)
{
  g_return_if_fail (BOBGUI_IS_POPOVER_BIN (self));
  g_return_if_fail (popover == NULL || BOBGUI_IS_POPOVER (popover));

  g_object_freeze_notify (G_OBJECT (self));

  g_clear_object (&self->menu_model);

  if (self->popover != NULL)
    {
      bobgui_widget_set_visible (self->popover, FALSE);

      g_signal_handlers_disconnect_by_func (self->popover,
                                            on_popover_destroy,
                                            self);
      g_signal_handlers_disconnect_by_func (self->popover,
                                            on_popover_map,
                                            self);
      g_signal_handlers_disconnect_by_func (self->popover,
                                            on_popover_unmap,
                                            self);

      bobgui_widget_unparent (self->popover);
    }

  self->popover = popover;

  if (popover != NULL)
    {
      bobgui_widget_set_parent (self->popover, BOBGUI_WIDGET (self));
      g_signal_connect_swapped (self->popover, "destroy",
                                G_CALLBACK (on_popover_destroy),
                                self);
      g_signal_connect_swapped (self->popover, "map",
                                G_CALLBACK (on_popover_map),
                                self);
      g_signal_connect_swapped (self->popover, "unmap",
                                G_CALLBACK (on_popover_unmap),
                                self);
    }

  g_object_notify_by_pspec (G_OBJECT (self), obj_props[PROP_POPOVER]);
  g_object_notify_by_pspec (G_OBJECT (self), obj_props[PROP_MENU_MODEL]);

  g_object_thaw_notify (G_OBJECT (self));
}

/**
 * bobgui_popover_bin_get_popover:
 * @self: a popover bin
 *
 * Retrieves the `BobguiPopover` set using [method@Bobgui.PopoverBin.set_popover].
 *
 * Returns: (transfer none) (nullable) (type Bobgui.Popover): a popover widget
 *
 * Since: 4.22
 */
BobguiWidget *
bobgui_popover_bin_get_popover (BobguiPopoverBin *self)
{
  g_return_val_if_fail (BOBGUI_IS_POPOVER_BIN (self), NULL);

  return self->popover;
}

/**
 * bobgui_popover_bin_set_handle_input:
 * @self: a popover bin
 * @handle_input: whether to handle input
 *
 * Enables or disables input handling.
 *
 * If enabled, the popover bin will pop up the
 * popover on right-click or long press, as expected
 * for a context menu.
 *
 * Since: 4.22
 */
void
bobgui_popover_bin_set_handle_input (BobguiPopoverBin *self,
                                  gboolean       handle_input)
{
  g_return_if_fail (BOBGUI_IS_POPOVER_BIN (self));

  if (self->handle_input == handle_input)
    return;

  self->handle_input = handle_input;

  if (handle_input)
    {
      BobguiShortcutTrigger *trigger;
      BobguiShortcutAction *action;

      self->click_gesture = BOBGUI_EVENT_CONTROLLER (bobgui_gesture_click_new ());
      bobgui_gesture_single_set_button (BOBGUI_GESTURE_SINGLE (self->click_gesture), 0);
      bobgui_gesture_single_set_exclusive (BOBGUI_GESTURE_SINGLE (self->click_gesture), TRUE);
      g_signal_connect_swapped (self->click_gesture, "pressed",
                                G_CALLBACK (pressed_cb), self);
      bobgui_widget_add_controller (BOBGUI_WIDGET (self), self->click_gesture);

      self->long_press_gesture = BOBGUI_EVENT_CONTROLLER (bobgui_gesture_long_press_new ());
      bobgui_gesture_single_set_exclusive (BOBGUI_GESTURE_SINGLE (self->long_press_gesture), TRUE);
      bobgui_gesture_single_set_touch_only (BOBGUI_GESTURE_SINGLE (self->long_press_gesture), TRUE);
      g_signal_connect_swapped (self->long_press_gesture, "pressed", G_CALLBACK (long_pressed_cb), self);
      bobgui_widget_add_controller (BOBGUI_WIDGET (self), self->long_press_gesture);

      self->shortcut_controller = BOBGUI_EVENT_CONTROLLER (bobgui_shortcut_controller_new ());
      trigger = bobgui_alternative_trigger_new (
                  bobgui_keyval_trigger_new (GDK_KEY_Menu, GDK_NO_MODIFIER_MASK),
                  bobgui_keyval_trigger_new (GDK_KEY_F10, GDK_SHIFT_MASK));
      action = bobgui_named_action_new ("menu.popup");
      bobgui_shortcut_controller_add_shortcut (BOBGUI_SHORTCUT_CONTROLLER (self->shortcut_controller),
                                            bobgui_shortcut_new (trigger, action));

      bobgui_widget_add_controller (BOBGUI_WIDGET (self), self->shortcut_controller);
    }
  else
    {
      bobgui_widget_remove_controller (BOBGUI_WIDGET (self), self->click_gesture);
      self->click_gesture = NULL;
      bobgui_widget_remove_controller (BOBGUI_WIDGET (self), self->long_press_gesture);
      self->long_press_gesture = NULL;
      bobgui_widget_remove_controller (BOBGUI_WIDGET (self), self->shortcut_controller);
      self->shortcut_controller = NULL;
    }

  g_object_notify_by_pspec (G_OBJECT (self), obj_props[PROP_HANDLE_INPUT]);
}

gboolean
bobgui_popover_bin_get_handle_input (BobguiPopoverBin *self)
{
  g_return_val_if_fail (BOBGUI_IS_POPOVER_BIN (self), FALSE);

  return self->handle_input;
}

/**
 * bobgui_popover_bin_popup:
 * @self: a popover bin
 *
 * Presents the popover to the user.
 *
 * Use [method@Bobgui.PopoverBin.set_popover] or
 * [method@Bobgui.PopoverBin.set_menu_model] to define the popover.
 *
 * See: [method@Bobgui.PopoverBin.popdown]
 *
 * Since: 4.22
 */
void
bobgui_popover_bin_popup (BobguiPopoverBin *self)
{
  g_return_if_fail (BOBGUI_IS_POPOVER_BIN (self));

  bobgui_popover_bin_popup_at_position (self, -1.0, -1.0);
}

/**
 * bobgui_popover_bin_popdown:
 * @self: a popover bin
 *
 * Hides the popover from the user.
 *
 * See: [method@Bobgui.PopoverBin.popup]
 *
 * Since: 4.22
 */
void
bobgui_popover_bin_popdown (BobguiPopoverBin *self)
{
  g_return_if_fail (BOBGUI_IS_POPOVER_BIN (self));

  if (self->popover != NULL)
    bobgui_popover_popdown (BOBGUI_POPOVER (self->popover));
}
