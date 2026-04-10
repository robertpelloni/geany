/*
 * Copyright © 2023 Benjamin Otte
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Benjamin Otte <otte@gnome.org>
 */

#include "config.h"

#include "bobguilistfactorywidgetprivate.h"

#include "bobguibinlayout.h"
#include "bobguieventcontrollermotion.h"
#include "bobguigestureclick.h"
#include "bobguilistitemfactoryprivate.h"
#include "bobguilistbaseprivate.h"
#include "bobguiwidget.h"

typedef struct _BobguiListFactoryWidgetPrivate BobguiListFactoryWidgetPrivate;
struct _BobguiListFactoryWidgetPrivate
{
  BobguiListItemFactory *factory;

  gpointer object;
  gboolean single_click_activate;
  gboolean selectable;
  gboolean activatable;
};

enum {
  PROP_0,
  PROP_ACTIVATABLE,
  PROP_FACTORY,
  PROP_SELECTABLE,
  PROP_SINGLE_CLICK_ACTIVATE,

  N_PROPS
};

enum
{
  ACTIVATE_SIGNAL,
  LAST_SIGNAL
};

G_DEFINE_TYPE_WITH_PRIVATE (BobguiListFactoryWidget, bobgui_list_factory_widget, BOBGUI_TYPE_LIST_ITEM_BASE)

static GParamSpec *properties[N_PROPS] = { NULL, };
static guint signals[LAST_SIGNAL] = { 0 };

static void
bobgui_list_factory_widget_activate_signal (BobguiListFactoryWidget *self)
{
  BobguiListFactoryWidgetPrivate *priv = bobgui_list_factory_widget_get_instance_private (self);

  if (!priv->activatable)
    return;

  bobgui_widget_activate_action (BOBGUI_WIDGET (self),
                              "list.activate-item",
                              "u",
                              bobgui_list_item_base_get_position (BOBGUI_LIST_ITEM_BASE (self)));
}

static gpointer
bobgui_list_factory_widget_default_create_object (BobguiListFactoryWidget *self)
{
  g_assert_not_reached ();
  return NULL;
}

static void
bobgui_list_factory_widget_default_setup_object (BobguiListFactoryWidget *self,
                                              gpointer              object)
{
  BobguiListFactoryWidgetPrivate *priv = bobgui_list_factory_widget_get_instance_private (self);

  priv->object = object;
}

static void
bobgui_list_factory_widget_setup_func (gpointer object,
                                    gpointer data)
{
  BOBGUI_LIST_FACTORY_WIDGET_GET_CLASS (data)->setup_object (data, object);
}

static void
bobgui_list_factory_widget_setup_factory (BobguiListFactoryWidget *self)
{
  BobguiListFactoryWidgetPrivate *priv = bobgui_list_factory_widget_get_instance_private (self);
  gpointer object;

  object = BOBGUI_LIST_FACTORY_WIDGET_GET_CLASS (self)->create_object (self);

  bobgui_list_item_factory_setup (priv->factory,
                               object,
                               bobgui_list_item_base_get_item (BOBGUI_LIST_ITEM_BASE (self)) != NULL,
                               bobgui_list_factory_widget_setup_func,
                               self);

  g_assert (priv->object == object);
}

static void
bobgui_list_factory_widget_default_teardown_object (BobguiListFactoryWidget *self,
                                                 gpointer              object)
{
  BobguiListFactoryWidgetPrivate *priv = bobgui_list_factory_widget_get_instance_private (self);

  priv->object = NULL;
}

static void
bobgui_list_factory_widget_teardown_func (gpointer object,
                                       gpointer data)
{
  BOBGUI_LIST_FACTORY_WIDGET_GET_CLASS (data)->teardown_object (data, object);
}

static void
bobgui_list_factory_widget_teardown_factory (BobguiListFactoryWidget *self)
{
  BobguiListFactoryWidgetPrivate *priv = bobgui_list_factory_widget_get_instance_private (self);
  gpointer item = priv->object;

  bobgui_list_item_factory_teardown (priv->factory,
                                  item,
                                  bobgui_list_item_base_get_item (BOBGUI_LIST_ITEM_BASE (self)) != NULL,
                                  bobgui_list_factory_widget_teardown_func,
                                  self);

  g_assert (priv->object == NULL);
  g_object_unref (item);
}

static void
bobgui_list_factory_widget_default_update_object (BobguiListFactoryWidget *self,
                                               gpointer              object,
                                               guint                 position,
                                               gpointer              item,
                                               gboolean              selected)
{
  BOBGUI_LIST_ITEM_BASE_CLASS (bobgui_list_factory_widget_parent_class)->update (BOBGUI_LIST_ITEM_BASE (self),
                                                                           position,
                                                                           item,
                                                                           selected);
}

typedef struct {
  BobguiListFactoryWidget *widget;
  guint position;
  gpointer item;
  gboolean selected;
} BobguiListFactoryWidgetUpdate;

static void
bobgui_list_factory_widget_update_func (gpointer object,
                                     gpointer data)
{
  BobguiListFactoryWidgetUpdate *update = data;

  BOBGUI_LIST_FACTORY_WIDGET_GET_CLASS (update->widget)->update_object (update->widget,
                                                                     object,
                                                                     update->position,
                                                                     update->item,
                                                                     update->selected);
}

static void
bobgui_list_factory_widget_update (BobguiListItemBase *base,
                                guint            position,
                                gpointer         item,
                                gboolean         selected)
{
  BobguiListFactoryWidget *self = BOBGUI_LIST_FACTORY_WIDGET (base);
  BobguiListFactoryWidgetPrivate *priv = bobgui_list_factory_widget_get_instance_private (self);
  BobguiListFactoryWidgetUpdate update = { self, position, item, selected };

  if (priv->object)
    {
      gpointer old_item = bobgui_list_item_base_get_item (BOBGUI_LIST_ITEM_BASE (self));

      bobgui_list_item_factory_update (priv->factory,
                                    priv->object,
                                    item != old_item && old_item != NULL,
                                    item != old_item && item != NULL,
                                    bobgui_list_factory_widget_update_func,
                                    &update);
    }
  else
    {
      bobgui_list_factory_widget_update_func (NULL, &update);
    }
}

static void
bobgui_list_factory_widget_set_property (GObject      *object,
                                      guint         property_id,
                                      const GValue *value,
                                      GParamSpec   *pspec)
{
  BobguiListFactoryWidget *self = BOBGUI_LIST_FACTORY_WIDGET (object);

  switch (property_id)
    {
    case PROP_ACTIVATABLE:
      bobgui_list_factory_widget_set_activatable (self, g_value_get_boolean (value));
      break;

    case PROP_FACTORY:
      bobgui_list_factory_widget_set_factory (self, g_value_get_object (value));
      break;

    case PROP_SELECTABLE:
      bobgui_list_factory_widget_set_selectable (self, g_value_get_boolean (value));
      break;

    case PROP_SINGLE_CLICK_ACTIVATE:
      bobgui_list_factory_widget_set_single_click_activate (self, g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
bobgui_list_factory_widget_clear_factory (BobguiListFactoryWidget *self)
{
  BobguiListFactoryWidgetPrivate *priv = bobgui_list_factory_widget_get_instance_private (self);

  if (priv->factory == NULL)
    return;

  if (priv->object)
    bobgui_list_factory_widget_teardown_factory (self);

  g_clear_object (&priv->factory);
}
static void
bobgui_list_factory_widget_dispose (GObject *object)
{
  BobguiListFactoryWidget *self = BOBGUI_LIST_FACTORY_WIDGET (object);

  bobgui_list_factory_widget_clear_factory (self);

  G_OBJECT_CLASS (bobgui_list_factory_widget_parent_class)->dispose (object);
}

static void
bobgui_list_factory_widget_select_action (BobguiWidget  *widget,
                                       const char *action_name,
                                       GVariant   *parameter)
{
  BobguiListFactoryWidget *self = BOBGUI_LIST_FACTORY_WIDGET (widget);
  BobguiListFactoryWidgetPrivate *priv = bobgui_list_factory_widget_get_instance_private (self);
  gboolean modify, extend;

  if (!priv->selectable)
    return;

  g_variant_get (parameter, "(bb)", &modify, &extend);

  bobgui_widget_activate_action (BOBGUI_WIDGET (self),
                              "list.select-item",
                              "(ubb)",
                              bobgui_list_item_base_get_position (BOBGUI_LIST_ITEM_BASE (self)), modify, extend);
}

static void
bobgui_list_factory_widget_scroll_to_action (BobguiWidget  *widget,
                                          const char *action_name,
                                          GVariant   *parameter)
{
  bobgui_widget_activate_action (widget,
                              "list.scroll-to-item",
                              "u",
                              bobgui_list_item_base_get_position (BOBGUI_LIST_ITEM_BASE (widget)));
}

static void
bobgui_list_factory_widget_class_init (BobguiListFactoryWidgetClass *klass)
{
  BobguiListItemBaseClass *base_class = BOBGUI_LIST_ITEM_BASE_CLASS (klass);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  klass->activate_signal = bobgui_list_factory_widget_activate_signal;
  klass->create_object = bobgui_list_factory_widget_default_create_object;
  klass->setup_object = bobgui_list_factory_widget_default_setup_object;
  klass->update_object = bobgui_list_factory_widget_default_update_object;
  klass->teardown_object = bobgui_list_factory_widget_default_teardown_object;

  base_class->update = bobgui_list_factory_widget_update;

  gobject_class->set_property = bobgui_list_factory_widget_set_property;
  gobject_class->dispose = bobgui_list_factory_widget_dispose;

  properties[PROP_ACTIVATABLE] =
    g_param_spec_boolean ("activatable", NULL, NULL,
                          FALSE,
                          G_PARAM_WRITABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  properties[PROP_FACTORY] =
    g_param_spec_object ("factory", NULL, NULL,
                         BOBGUI_TYPE_LIST_ITEM_FACTORY,
                         G_PARAM_WRITABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  properties[PROP_SELECTABLE] =
    g_param_spec_boolean ("selectable", NULL, NULL,
                          FALSE,
                          G_PARAM_WRITABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  properties[PROP_SINGLE_CLICK_ACTIVATE] =
    g_param_spec_boolean ("single-click-activate", NULL, NULL,
                          FALSE,
                          G_PARAM_WRITABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (gobject_class, N_PROPS, properties);

  signals[ACTIVATE_SIGNAL] =
    g_signal_new (I_("activate-keybinding"),
                  G_OBJECT_CLASS_TYPE (gobject_class),
                  G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (BobguiListFactoryWidgetClass, activate_signal),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);

  bobgui_widget_class_set_activate_signal (widget_class, signals[ACTIVATE_SIGNAL]);

  /**
   * BobguiListItem|listitem.select:
   * @modify: %TRUE to toggle the existing selection, %FALSE to select
   * @extend: %TRUE to extend the selection
   *
   * Changes selection if the item is selectable.
   * If the item is not selectable, nothing happens.
   *
   * This function will emit the list.select-item action and the resulting
   * behavior, in particular the interpretation of @modify and @extend
   * depends on the view containing this listitem. See for example
   * BobguiListView|list.select-item or BobguiGridView|list.select-item.
   */
  bobgui_widget_class_install_action (widget_class,
                                   "listitem.select",
                                   "(bb)",
                                   bobgui_list_factory_widget_select_action);

  /**
   * BobguiListItem|listitem.scroll-to:
   *
   * Moves the visible area of the list to this item with the minimum amount
   * of scrolling required. If the item is already visible, nothing happens.
   */
  bobgui_widget_class_install_action (widget_class,
                                   "listitem.scroll-to",
                                   NULL,
                                   bobgui_list_factory_widget_scroll_to_action);

  bobgui_widget_class_add_binding_signal (widget_class, GDK_KEY_Return, 0,
                                       "activate-keybinding", 0);
  bobgui_widget_class_add_binding_signal (widget_class, GDK_KEY_ISO_Enter, 0,
                                       "activate-keybinding", 0);
  bobgui_widget_class_add_binding_signal (widget_class, GDK_KEY_KP_Enter, 0,
                                       "activate-keybinding", 0);

  /* note that some of these may get overwritten by child widgets,
   * such as BobguiTreeExpander */
  bobgui_widget_class_add_binding_action (widget_class, GDK_KEY_space, 0,
                                       "listitem.select", "(bb)", TRUE, FALSE);
  bobgui_widget_class_add_binding_action (widget_class, GDK_KEY_space, GDK_CONTROL_MASK,
                                       "listitem.select", "(bb)", TRUE, FALSE);
  bobgui_widget_class_add_binding_action (widget_class, GDK_KEY_space, GDK_SHIFT_MASK,
                                       "listitem.select", "(bb)", TRUE, FALSE);
  bobgui_widget_class_add_binding_action (widget_class, GDK_KEY_space, GDK_CONTROL_MASK | GDK_SHIFT_MASK,
                                       "listitem.select", "(bb)", TRUE, FALSE);
  bobgui_widget_class_add_binding_action (widget_class, GDK_KEY_KP_Space, 0,
                                       "listitem.select", "(bb)", TRUE, FALSE);
  bobgui_widget_class_add_binding_action (widget_class, GDK_KEY_KP_Space, GDK_CONTROL_MASK,
                                       "listitem.select", "(bb)", TRUE, FALSE);
  bobgui_widget_class_add_binding_action (widget_class, GDK_KEY_KP_Space, GDK_SHIFT_MASK,
                                       "listitem.select", "(bb)", TRUE, FALSE);
  bobgui_widget_class_add_binding_action (widget_class, GDK_KEY_KP_Space, GDK_CONTROL_MASK | GDK_SHIFT_MASK,
                                       "listitem.select", "(bb)", TRUE, FALSE);

  /* This gets overwritten by bobgui_list_factory_widget_new() but better safe than sorry */
  bobgui_widget_class_set_css_name (widget_class, I_("row"));
}

static void
bobgui_list_factory_widget_click_gesture_pressed (BobguiGestureClick      *gesture,
                                               int                   n_press,
                                               double                x,
                                               double                y,
                                               BobguiListFactoryWidget *self)
{
  BobguiListFactoryWidgetPrivate *priv = bobgui_list_factory_widget_get_instance_private (self);
  BobguiWidget *widget = BOBGUI_WIDGET (self);

  if (!priv->selectable && !priv->activatable)
    {
      bobgui_gesture_set_state (BOBGUI_GESTURE (gesture), BOBGUI_EVENT_SEQUENCE_DENIED);
      return;
    }

  if (priv->activatable)
    {
      if (n_press == 2 && !priv->single_click_activate)
        {
          bobgui_gesture_set_state (BOBGUI_GESTURE (gesture), BOBGUI_EVENT_SEQUENCE_CLAIMED);
          bobgui_widget_activate_action (BOBGUI_WIDGET (self),
                                      "list.activate-item",
                                      "u",
                                      bobgui_list_item_base_get_position (BOBGUI_LIST_ITEM_BASE (self)));
        }
    }

  if (bobgui_widget_get_focus_on_click (widget))
    bobgui_widget_grab_focus (widget);
}

static void
bobgui_list_factory_widget_click_gesture_released (BobguiGestureClick      *gesture,
                                                int                   n_press,
                                                double                x,
                                                double                y,
                                                BobguiListFactoryWidget *self)
{
  BobguiListFactoryWidgetPrivate *priv = bobgui_list_factory_widget_get_instance_private (self);

  if (priv->selectable)
    {
      GdkModifierType state;
      GdkEvent *event;
      gboolean extend, modify;

      event = bobgui_gesture_get_last_event (BOBGUI_GESTURE (gesture),
                                          bobgui_gesture_single_get_current_sequence (BOBGUI_GESTURE_SINGLE (gesture)));
      state = gdk_event_get_modifier_state (event);
      extend = (state & GDK_SHIFT_MASK) != 0;
      modify = (state & GDK_CONTROL_MASK) != 0;
#ifdef __APPLE__
      modify = modify | ((state & GDK_META_MASK) != 0);
#endif

      bobgui_widget_activate_action (BOBGUI_WIDGET (self),
                                  "list.select-item",
                                  "(ubb)",
                                  bobgui_list_item_base_get_position (BOBGUI_LIST_ITEM_BASE (self)), modify, extend);
    }

  if (priv->activatable)
    {
      if (n_press == 1 && priv->single_click_activate)
        {
          bobgui_gesture_set_state (BOBGUI_GESTURE (gesture), BOBGUI_EVENT_SEQUENCE_CLAIMED);
          bobgui_widget_activate_action (BOBGUI_WIDGET (self),
                                      "list.activate-item",
                                      "u",
                                      bobgui_list_item_base_get_position (BOBGUI_LIST_ITEM_BASE (self)));
        }
    }

}

static void
bobgui_list_factory_widget_hover_cb (BobguiEventControllerMotion *controller,
                                  double                    x,
                                  double                    y,
                                  BobguiListFactoryWidget     *self)
{
  BobguiListFactoryWidgetPrivate *priv = bobgui_list_factory_widget_get_instance_private (self);

  if (!priv->single_click_activate)
    return;

  if (priv->selectable)
    {
      bobgui_widget_activate_action (BOBGUI_WIDGET (self),
                                  "list.select-item",
                                  "(ubb)",
                                  bobgui_list_item_base_get_position (BOBGUI_LIST_ITEM_BASE (self)), FALSE, FALSE);
    }
}

static void
bobgui_list_factory_widget_init (BobguiListFactoryWidget *self)
{
  BobguiEventController *controller;
  BobguiGesture *gesture;

  bobgui_widget_set_focusable (BOBGUI_WIDGET (self), TRUE);

  gesture = bobgui_gesture_click_new ();
  bobgui_event_controller_set_propagation_phase (BOBGUI_EVENT_CONTROLLER (gesture),
                                              BOBGUI_PHASE_BUBBLE);
  bobgui_gesture_single_set_touch_only (BOBGUI_GESTURE_SINGLE (gesture),
                                     FALSE);
  bobgui_gesture_single_set_button (BOBGUI_GESTURE_SINGLE (gesture),
                                 GDK_BUTTON_PRIMARY);
  g_signal_connect (gesture, "pressed",
                    G_CALLBACK (bobgui_list_factory_widget_click_gesture_pressed), self);
  g_signal_connect (gesture, "released",
                    G_CALLBACK (bobgui_list_factory_widget_click_gesture_released), self);
  bobgui_widget_add_controller (BOBGUI_WIDGET (self), BOBGUI_EVENT_CONTROLLER (gesture));

  controller = bobgui_event_controller_motion_new ();
  g_signal_connect (controller, "enter", G_CALLBACK (bobgui_list_factory_widget_hover_cb), self);
  bobgui_widget_add_controller (BOBGUI_WIDGET (self), controller);
}

gpointer
bobgui_list_factory_widget_get_object (BobguiListFactoryWidget *self)
{
  BobguiListFactoryWidgetPrivate *priv = bobgui_list_factory_widget_get_instance_private (self);

  return priv->object;
}

void
bobgui_list_factory_widget_set_factory (BobguiListFactoryWidget *self,
                                     BobguiListItemFactory   *factory)
{
  BobguiListFactoryWidgetPrivate *priv = bobgui_list_factory_widget_get_instance_private (self);

  if (priv->factory == factory)
    return;

  bobgui_list_factory_widget_clear_factory (self);

  if (factory)
    {
      priv->factory = g_object_ref (factory);

      bobgui_list_factory_widget_setup_factory (self);
    }

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_FACTORY]);
}

void
bobgui_list_factory_widget_set_single_click_activate (BobguiListFactoryWidget *self,
                                                   gboolean              single_click_activate)
{
  BobguiListFactoryWidgetPrivate *priv = bobgui_list_factory_widget_get_instance_private (self);

  if (priv->single_click_activate == single_click_activate)
    return;

  priv->single_click_activate = single_click_activate;

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_SINGLE_CLICK_ACTIVATE]);
}

gboolean
bobgui_list_factory_widget_get_single_click_activate (BobguiListFactoryWidget *self)
{
  BobguiListFactoryWidgetPrivate *priv = bobgui_list_factory_widget_get_instance_private (self);

  return priv->single_click_activate;
}

void
bobgui_list_factory_widget_set_activatable (BobguiListFactoryWidget *self,
                                         gboolean              activatable)
{
  BobguiListFactoryWidgetPrivate *priv = bobgui_list_factory_widget_get_instance_private (self);

  if (priv->activatable == activatable)
    return;

  priv->activatable = activatable;

  if (activatable)
    bobgui_widget_add_css_class (BOBGUI_WIDGET (self), "activatable");
  else
    bobgui_widget_remove_css_class (BOBGUI_WIDGET (self), "activatable");

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_ACTIVATABLE]);
}

gboolean
bobgui_list_factory_widget_get_activatable (BobguiListFactoryWidget *self)
{
  BobguiListFactoryWidgetPrivate *priv = bobgui_list_factory_widget_get_instance_private (self);

  return priv->activatable;
}

void
bobgui_list_factory_widget_set_selectable (BobguiListFactoryWidget *self,
                                        gboolean              selectable)
{
  BobguiListFactoryWidgetPrivate *priv = bobgui_list_factory_widget_get_instance_private (self);

  if (priv->selectable == selectable)
    return;

  priv->selectable = selectable;

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_SELECTABLE]);
}

gboolean
bobgui_list_factory_widget_get_selectable (BobguiListFactoryWidget *self)
{
  BobguiListFactoryWidgetPrivate *priv = bobgui_list_factory_widget_get_instance_private (self);

  return priv->selectable;
}
