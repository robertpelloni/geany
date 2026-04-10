/*
 * Copyright © 2014 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the licence, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Matthias Clasen
 */

#include "config.h"

#include "bobguimodelbuttonprivate.h"

#include "bobguiactionhelperprivate.h"
#include "bobguiboxlayout.h"
#include "bobguigestureclick.h"
#include "bobguiwidgetprivate.h"
#include "bobguimenutrackeritemprivate.h"
#include "bobguiimage.h"
#include "bobguilabel.h"
#include "bobguibox.h"
#include "bobguitypebuiltins.h"
#include "bobguistack.h"
#include "bobguipopovermenuprivate.h"
#include "bobguicssnodeprivate.h"
#include "bobguicsstypesprivate.h"
#include "bobguibuiltiniconprivate.h"
#include "bobguisizegroup.h"
#include "bobguiactionable.h"
#include "bobguieventcontrollermotion.h"
#include "bobguieventcontrollerkey.h"
#include "bobguieventcontrollerfocus.h"
#include "bobguinative.h"
#include "bobguishortcuttrigger.h"
#include "bobguishortcutcontroller.h"
#include "bobguishortcut.h"
#include "bobguiaccessibleprivate.h"
#include "bobguiprivate.h"

/*< private >
 * BobguiModelButton:
 *
 * BobguiModelButton is a button class that can use a GAction as its model.
 * In contrast to BobguiToggleButton or BobguiCheckButton, which can also
 * be backed by a GAction via the BobguiActionable:action-name property,
 * BobguiModelButton will adapt its appearance according to the kind of
 * action it is backed by, and appear either as a plain, check or
 * radio button.
 *
 * Model buttons are used when popovers from a menu model with
 * bobgui_popover_menu_new_from_model(); they can also be used manually in
 * a BobguiPopoverMenu.
 *
 * When the action is specified via the BobguiActionable:action-name
 * and BobguiActionable:action-target properties, the role of the button
 * (i.e. whether it is a plain, check or radio button) is determined by
 * the type of the action and doesn't have to be explicitly specified
 * with the BobguiModelButton:role property.
 *
 * The content of the button is specified by the BobguiModelButton:text
 * and BobguiModelButton:icon properties.
 *
 * The appearance of model buttons can be influenced with the
 * BobguiModelButton:iconic property.
 *
 * Model buttons have built-in support for submenus in BobguiPopoverMenu.
 * To make a BobguiModelButton that opens a submenu when activated, set
 * the BobguiModelButton:menu-name property. To make a button that goes
 * back to the parent menu, you should set the BobguiModelButton:inverted
 * property to place the submenu indicator at the opposite side.
 *
 * # Example
 *
 * |[
 * <object class="BobguiPopoverMenu">
 *   <child>
 *     <object class="BobguiBox">
 *       <property name="visible">True</property>
 *       <property name="margin-start">10</property>
 *       <property name="margin-end">10</property>
 *       <property name="margin-top">10</property>
 *       <property name="margin-bottom">10</property>
 *       <child>
 *         <object class="BobguiModelButton">
 *           <property name="visible">True</property>
 *           <property name="action-name">view.cut</property>
 *           <property name="text" translatable="yes">Cut</property>
 *         </object>
 *       </child>
 *       <child>
 *         <object class="BobguiModelButton">
 *           <property name="visible">True</property>
 *           <property name="action-name">view.copy</property>
 *           <property name="text" translatable="yes">Copy</property>
 *         </object>
 *       </child>
 *       <child>
 *         <object class="BobguiModelButton">
 *           <property name="visible">True</property>
 *           <property name="action-name">view.paste</property>
 *           <property name="text" translatable="yes">Paste</property>
 *         </object>
 *       </child>
 *     </object>
 *   </child>
 * </object>
 * ]|
 *
 * # CSS nodes
 *
 * ```
 * modelbutton
 * ├── <child>
 * ╰── check
 * ```
 *
 * ```
 * modelbutton
 * ├── <child>
 * ╰── radio
 * ```
 *
 * ```
 * modelbutton
 * ├── <child>
 * ╰── arrow
 * ```
 *
 * BobguiModelButton has a main CSS node with name modelbutton, and a subnode,
 * which will have the name check, radio or arrow, depending on the role
 * of the button and whether it has a menu name set.
 *
 * The subnode is positioned before or after the content nodes and gets the
 * .left or .right style class, depending on where it is located.
 *
 * ```
 * button.model
 * ├── <child>
 * ╰── check
 * ```
 *
 * Iconic model buttons (see BobguiModelButton:iconic) change the name of
 * their main node to button and add a .model style class to it. The indicator
 * subnode is invisible in this case.
 */

struct _BobguiModelButton
{
  BobguiWidget parent_instance;

  BobguiWidget *box;
  BobguiWidget *image;
  BobguiWidget *label;
  BobguiWidget *accel_label;
  BobguiWidget *start_box;
  BobguiWidget *start_indicator;
  BobguiWidget *end_indicator;
  BobguiWidget *popover;
  BobguiActionHelper *action_helper;
  char *menu_name;
  BobguiButtonRole role;
  BobguiSizeGroup *indicators;
  char *accel;
  guint open_timeout;
  BobguiEventController *controller;

  guint active : 1;
  guint iconic : 1;
  guint keep_open : 1;
};

typedef struct _BobguiModelButtonClass BobguiModelButtonClass;

struct _BobguiModelButtonClass
{
  BobguiWidgetClass parent_class;

  void (* clicked) (BobguiModelButton *button);
};

static void bobgui_model_button_actionable_iface_init (BobguiActionableInterface *iface);

G_DEFINE_TYPE_WITH_CODE (BobguiModelButton, bobgui_model_button, BOBGUI_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_ACTIONABLE, bobgui_model_button_actionable_iface_init))

GType
bobgui_button_role_get_type (void)
{
  static gsize bobgui_button_role_type;

  if (g_once_init_enter (&bobgui_button_role_type))
    {
      static const GEnumValue values[] = {
        { BOBGUI_BUTTON_ROLE_NORMAL, "BOBGUI_BUTTON_ROLE_NORMAL", "normal" },
        { BOBGUI_BUTTON_ROLE_CHECK, "BOBGUI_BUTTON_ROLE_CHECK", "check" },
        { BOBGUI_BUTTON_ROLE_RADIO, "BOBGUI_BUTTON_ROLE_RADIO", "radio" },
        { BOBGUI_BUTTON_ROLE_TITLE, "BOBGUI_BUTTON_ROLE_RADIO", "title" },
        { 0, NULL, NULL }
      };
      GType type;

      type = g_enum_register_static (I_("BobguiButtonRole"), values);

      g_once_init_leave (&bobgui_button_role_type, type);
    }

  return bobgui_button_role_type;
}

enum
{
  PROP_0,
  PROP_ROLE,
  PROP_ICON,
  PROP_TEXT,
  PROP_USE_MARKUP,
  PROP_ACTIVE,
  PROP_MENU_NAME,
  PROP_POPOVER,
  PROP_ICONIC,
  PROP_ACCEL,
  PROP_INDICATOR_SIZE_GROUP,

  /* actionable properties */
  PROP_ACTION_NAME,
  PROP_ACTION_TARGET,
  LAST_PROP = PROP_ACTION_NAME
};

enum
{
  SIGNAL_CLICKED,
  LAST_SIGNAL
};

static GParamSpec *properties[LAST_PROP] = { NULL, };
static guint signals[LAST_SIGNAL] = { 0 };

static void
bobgui_model_button_set_action_name (BobguiActionable *actionable,
                                  const char    *action_name)
{
  BobguiModelButton *self = BOBGUI_MODEL_BUTTON (actionable);

  if (!self->action_helper)
    self->action_helper = bobgui_action_helper_new (actionable);

  bobgui_action_helper_set_action_name (self->action_helper, action_name);
}

static void
bobgui_model_button_set_action_target_value (BobguiActionable *actionable,
                                          GVariant      *action_target)
{
  BobguiModelButton *self = BOBGUI_MODEL_BUTTON (actionable);

  if (!self->action_helper)
    self->action_helper = bobgui_action_helper_new (actionable);

  bobgui_action_helper_set_action_target_value (self->action_helper, action_target);
}

static const char *
bobgui_model_button_get_action_name (BobguiActionable *actionable)
{
  BobguiModelButton *self = BOBGUI_MODEL_BUTTON (actionable);

  return bobgui_action_helper_get_action_name (self->action_helper);
}

static GVariant *
bobgui_model_button_get_action_target_value (BobguiActionable *actionable)
{
  BobguiModelButton *self = BOBGUI_MODEL_BUTTON (actionable);

  return bobgui_action_helper_get_action_target_value (self->action_helper);
}

static void
bobgui_model_button_actionable_iface_init (BobguiActionableInterface *iface)
{
  iface->get_action_name = bobgui_model_button_get_action_name;
  iface->set_action_name = bobgui_model_button_set_action_name;
  iface->get_action_target_value = bobgui_model_button_get_action_target_value;
  iface->set_action_target_value = bobgui_model_button_set_action_target_value;
}

static void
update_at_context (BobguiModelButton *button)
{
  BobguiAccessibleRole role;
  BobguiATContext *context;
  gboolean was_realized;

  context = bobgui_accessible_get_at_context (BOBGUI_ACCESSIBLE (button));
  if (context == NULL)
    return;

  was_realized = bobgui_at_context_is_realized (context);

  bobgui_at_context_unrealize (context);

  switch (button->role)
    {
    default:
    case BOBGUI_BUTTON_ROLE_NORMAL:
    case BOBGUI_BUTTON_ROLE_TITLE:
      role = BOBGUI_ACCESSIBLE_ROLE_MENU_ITEM;
      break;
    case BOBGUI_BUTTON_ROLE_CHECK:
      role = BOBGUI_ACCESSIBLE_ROLE_MENU_ITEM_CHECKBOX;
      break;
    case BOBGUI_BUTTON_ROLE_RADIO:
      role = BOBGUI_ACCESSIBLE_ROLE_MENU_ITEM_RADIO;
      break;
    }

  bobgui_at_context_set_accessible_role (context, role);

  if (was_realized)
    bobgui_at_context_realize (context);

  g_object_unref (context);
}

static void
update_node_ordering (BobguiModelButton *button)
{
  BobguiWidget *child;

  if (bobgui_widget_get_direction (BOBGUI_WIDGET (button)) == BOBGUI_TEXT_DIR_LTR)
    {
      if (button->start_indicator)
        {
          bobgui_widget_add_css_class (button->start_indicator, "left");
          bobgui_widget_remove_css_class (button->start_indicator, "right");
        }

      if (button->end_indicator)
        {
          bobgui_widget_add_css_class (button->end_indicator, "right");
          bobgui_widget_remove_css_class (button->end_indicator, "left");
        }

      child = bobgui_widget_get_first_child (BOBGUI_WIDGET (button));
     if (button->start_indicator && child != button->start_box)
        bobgui_widget_insert_before (button->start_box, BOBGUI_WIDGET (button), child);

      child = bobgui_widget_get_last_child (BOBGUI_WIDGET (button));
      if (button->end_indicator && child != button->end_indicator)
        bobgui_widget_insert_after (button->end_indicator, BOBGUI_WIDGET (button), child);
    }
  else
    {
      if (button->start_indicator)
        {
          bobgui_widget_add_css_class (button->start_indicator, "right");
          bobgui_widget_remove_css_class (button->start_indicator, "left");
        }

      if (button->end_indicator)
        {
          bobgui_widget_add_css_class (button->end_indicator, "left");
          bobgui_widget_remove_css_class (button->end_indicator, "right");

        }

      child = bobgui_widget_get_first_child (BOBGUI_WIDGET (button));
      if (button->end_indicator && child != button->end_indicator)
        bobgui_widget_insert_before (button->end_indicator, BOBGUI_WIDGET (button), child);

      child = bobgui_widget_get_last_child (BOBGUI_WIDGET (button));
      if (button->end_indicator && child != button->end_indicator)
        bobgui_widget_insert_after (button->end_indicator, BOBGUI_WIDGET (button), child);
    }
}

static void
update_end_indicator (BobguiModelButton *self)
{
  const gboolean is_ltr = bobgui_widget_get_direction (BOBGUI_WIDGET (self)) == BOBGUI_TEXT_DIR_LTR;

  if (!self->end_indicator)
    return;

  if (is_ltr)
    {
      bobgui_widget_add_css_class (self->end_indicator, "right");
      bobgui_widget_remove_css_class (self->end_indicator, "left");
    }
  else
    {
      bobgui_widget_add_css_class (self->end_indicator, "left");
      bobgui_widget_remove_css_class (self->end_indicator, "right");
    }
}

static BobguiStateFlags
get_start_indicator_state (BobguiModelButton *self)
{
  BobguiStateFlags state = bobgui_widget_get_state_flags (BOBGUI_WIDGET (self));

  if (self->role == BOBGUI_BUTTON_ROLE_CHECK ||
      self->role == BOBGUI_BUTTON_ROLE_RADIO)
    {
      if (self->active)
        state |= BOBGUI_STATE_FLAG_CHECKED;
      else
        state &= ~BOBGUI_STATE_FLAG_CHECKED;
    }

  return state;
}

static void
update_start_indicator (BobguiModelButton *self)
{
  const gboolean is_ltr = bobgui_widget_get_direction (BOBGUI_WIDGET (self)) == BOBGUI_TEXT_DIR_LTR;

  if (!self->start_indicator)
    return;

  bobgui_widget_set_state_flags (self->start_indicator, get_start_indicator_state (self), TRUE);

  if (is_ltr)
    {
      bobgui_widget_add_css_class (self->start_indicator, "left");
      bobgui_widget_remove_css_class (self->start_indicator, "right");
    }
  else
    {
      bobgui_widget_add_css_class (self->start_indicator, "right");
      bobgui_widget_remove_css_class (self->start_indicator, "left");
    }

}

static void
bobgui_model_button_update_state (BobguiModelButton *self)
{
  BobguiStateFlags indicator_state;

  update_start_indicator (self);
  update_end_indicator (self);

  indicator_state = get_start_indicator_state (self);
  if (self->iconic)
    bobgui_widget_set_state_flags (BOBGUI_WIDGET (self), indicator_state, TRUE);
}

static void
bobgui_model_button_state_flags_changed (BobguiWidget     *widget,
                                      BobguiStateFlags  previous_flags)
{
  bobgui_model_button_update_state (BOBGUI_MODEL_BUTTON (widget));

  BOBGUI_WIDGET_CLASS (bobgui_model_button_parent_class)->state_flags_changed (widget, previous_flags);
}

static void
bobgui_model_button_direction_changed (BobguiWidget        *widget,
                                    BobguiTextDirection  previous_dir)
{
  BobguiModelButton *button = BOBGUI_MODEL_BUTTON (widget);

  bobgui_model_button_update_state (button);
  update_node_ordering (button);

  BOBGUI_WIDGET_CLASS (bobgui_model_button_parent_class)->direction_changed (widget, previous_dir);
}

static void
update_node_name (BobguiModelButton *self)
{
  const char *start_name;
  const char *end_name;

  switch (self->role)
    {
    case BOBGUI_BUTTON_ROLE_TITLE:
      start_name = "arrow";
      end_name = "";
      break;
    case BOBGUI_BUTTON_ROLE_NORMAL:
      start_name = NULL;
      if (self->menu_name || self->popover)
        end_name = "arrow";
      else
        end_name = NULL;
      break;

    case BOBGUI_BUTTON_ROLE_CHECK:
      start_name = "check";
      end_name = NULL;
      break;

    case BOBGUI_BUTTON_ROLE_RADIO:
      start_name = "radio";
      end_name = NULL;
      break;

    default:
      g_assert_not_reached ();
    }

  if (self->iconic)
    {
      start_name = NULL;
      end_name = NULL;
    }

  if (start_name && !self->start_indicator)
    {
      self->start_indicator = bobgui_builtin_icon_new (start_name);
      bobgui_widget_set_halign (self->start_indicator, BOBGUI_ALIGN_CENTER);
      bobgui_widget_set_valign (self->start_indicator, BOBGUI_ALIGN_CENTER);
      update_start_indicator (self);

      bobgui_box_append (BOBGUI_BOX (self->start_box), self->start_indicator);
    }
  else if (start_name)
    {
      bobgui_css_node_set_name (bobgui_widget_get_css_node (self->start_indicator), g_quark_from_static_string (start_name));
    }
  else if (self->start_indicator)
    {
      bobgui_box_remove (BOBGUI_BOX (self->start_box), self->start_indicator);
      self->start_indicator = NULL;
    }

  if (end_name && !self->end_indicator)
    {
      self->end_indicator = bobgui_builtin_icon_new (end_name);
      bobgui_widget_set_halign (self->end_indicator, BOBGUI_ALIGN_CENTER);
      bobgui_widget_set_valign (self->end_indicator, BOBGUI_ALIGN_CENTER);
      bobgui_widget_set_parent (self->end_indicator, BOBGUI_WIDGET (self));
      update_end_indicator (self);
    }
  else if (end_name)
    {
      bobgui_css_node_set_name (bobgui_widget_get_css_node (self->end_indicator), g_quark_from_static_string (end_name));
    }
  else
    {
      g_clear_pointer (&self->end_indicator, bobgui_widget_unparent);
    }
}

static void
update_accessible_properties (BobguiModelButton *button)
{
  if (button->menu_name || button->popover)
    {
      bobgui_accessible_update_state (BOBGUI_ACCESSIBLE (button),
                                  BOBGUI_ACCESSIBLE_STATE_EXPANDED, FALSE,
                                  -1);
      bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (button),
                                      BOBGUI_ACCESSIBLE_PROPERTY_HAS_POPUP, TRUE,
                                      -1);
    }
  else
    {
      bobgui_accessible_reset_property (BOBGUI_ACCESSIBLE (button),
                                     BOBGUI_ACCESSIBLE_PROPERTY_HAS_POPUP);
      bobgui_accessible_reset_state (BOBGUI_ACCESSIBLE (button),
                                  BOBGUI_ACCESSIBLE_STATE_EXPANDED);
    }

  bobgui_accessible_reset_relation (BOBGUI_ACCESSIBLE (button), BOBGUI_ACCESSIBLE_RELATION_CONTROLS);
  if (button->popover)
    bobgui_accessible_update_relation (BOBGUI_ACCESSIBLE (button),
                                    BOBGUI_ACCESSIBLE_RELATION_CONTROLS, button->popover, NULL,
                                    -1);

  if (button->role == BOBGUI_BUTTON_ROLE_CHECK ||
      button->role == BOBGUI_BUTTON_ROLE_RADIO)
    bobgui_accessible_update_state (BOBGUI_ACCESSIBLE (button),
                                 BOBGUI_ACCESSIBLE_STATE_CHECKED, button->active,
                                 -1);
  else
    bobgui_accessible_reset_state (BOBGUI_ACCESSIBLE (button),
                                BOBGUI_ACCESSIBLE_STATE_CHECKED);

  bobgui_accessible_reset_relation (BOBGUI_ACCESSIBLE (button), BOBGUI_ACCESSIBLE_RELATION_LABELLED_BY);
  bobgui_accessible_update_relation (BOBGUI_ACCESSIBLE (button),
                                  BOBGUI_ACCESSIBLE_RELATION_LABELLED_BY, button->label, NULL,
                                  -1);

  if (button->accel)
    {
      guint key;
      GdkModifierType mods;
      char *text;

      bobgui_accelerator_parse (button->accel, &key, &mods);
      text = bobgui_accelerator_get_accessible_label (key, mods);
      bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (button),
                                      BOBGUI_ACCESSIBLE_PROPERTY_KEY_SHORTCUTS, text,
                                      -1);
      g_free (text);
    }
  else
    bobgui_accessible_reset_property (BOBGUI_ACCESSIBLE (button),
                                   BOBGUI_ACCESSIBLE_PROPERTY_KEY_SHORTCUTS);
}

static void
bobgui_model_button_set_role (BobguiModelButton *self,
                           BobguiButtonRole   role)
{
  if (role == self->role)
    return;

  self->role = role;

  if (role == BOBGUI_BUTTON_ROLE_TITLE)
    {
      bobgui_widget_add_css_class (BOBGUI_WIDGET (self), "title");
      bobgui_widget_set_halign (self->label, BOBGUI_ALIGN_CENTER);
    }
  else
    {
      bobgui_widget_remove_css_class (BOBGUI_WIDGET (self), "title");
      bobgui_widget_set_halign (self->label, BOBGUI_ALIGN_START);
    }

  update_node_name (self);
  bobgui_model_button_update_state (self);

  update_at_context (self);
  update_accessible_properties (self);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_ROLE]);
}

static void
update_visibility (BobguiModelButton *self)
{
  gboolean has_icon;
  gboolean has_text;

  has_icon = self->image && bobgui_image_get_storage_type (BOBGUI_IMAGE (self->image)) != BOBGUI_IMAGE_EMPTY;
  has_text = bobgui_label_get_text (BOBGUI_LABEL (self->label))[0] != '\0';

  bobgui_widget_set_visible (self->label, has_text && (!self->iconic || !has_icon));
  bobgui_widget_set_hexpand (self->label,
                          bobgui_widget_get_visible (self->label) && !has_icon);

  if (self->accel_label)
    bobgui_widget_set_visible (self->accel_label, has_text && (!self->iconic || !has_icon));

  if (self->image)
    {
      bobgui_widget_set_visible (self->image, has_icon && (self->iconic || !has_text));
      bobgui_widget_set_hexpand (self->image,
                              has_icon && (!has_text || !bobgui_widget_get_visible (self->label)));
    }
}

static void
update_tooltip (BobguiModelButton *self)
{
  if (self->iconic)
    {
      if (bobgui_label_get_use_markup (BOBGUI_LABEL (self->label)))
        bobgui_widget_set_tooltip_markup (BOBGUI_WIDGET (self), bobgui_label_get_text (BOBGUI_LABEL (self->label)));
      else
        bobgui_widget_set_tooltip_text (BOBGUI_WIDGET (self), bobgui_label_get_text (BOBGUI_LABEL (self->label)));
    }
  else
    {
      bobgui_widget_set_tooltip_text (BOBGUI_WIDGET (self), NULL);
    }
}

static void
bobgui_model_button_set_icon (BobguiModelButton *self,
                           GIcon          *icon)
{
  if (!self->image && icon)
    {
      self->image = g_object_new (BOBGUI_TYPE_IMAGE,
                                  "accessible-role", BOBGUI_ACCESSIBLE_ROLE_PRESENTATION,
                                  "gicon", icon,
                                  NULL);
      bobgui_widget_insert_before (self->image, BOBGUI_WIDGET (self), self->label);
    }
  else if (self->image && !icon)
    {
      g_clear_pointer (&self->image, bobgui_widget_unparent);
    }
  else if (icon)
    {
      bobgui_image_set_from_gicon (BOBGUI_IMAGE (self->image), icon);
    }

  update_visibility (self);
  update_tooltip (self);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_ICON]);
}

static void
bobgui_model_button_set_text (BobguiModelButton *button,
                           const char     *text)
{
  bobgui_label_set_text_with_mnemonic (BOBGUI_LABEL (button->label), text ? text : "");
  update_visibility (button);
  update_tooltip (button);

  bobgui_accessible_reset_relation (BOBGUI_ACCESSIBLE (button), BOBGUI_ACCESSIBLE_RELATION_LABELLED_BY);
  bobgui_accessible_update_relation (BOBGUI_ACCESSIBLE (button),
                                  BOBGUI_ACCESSIBLE_RELATION_LABELLED_BY, button->label, NULL,
                                  -1);

  g_object_notify_by_pspec (G_OBJECT (button), properties[PROP_TEXT]);
}

static void
bobgui_model_button_set_use_markup (BobguiModelButton *button,
                                 gboolean        use_markup)
{
  use_markup = !!use_markup;
  if (bobgui_label_get_use_markup (BOBGUI_LABEL (button->label)) == use_markup)
    return;

  bobgui_label_set_use_markup (BOBGUI_LABEL (button->label), use_markup);
  update_visibility (button);
  update_tooltip (button);

  g_object_notify_by_pspec (G_OBJECT (button), properties[PROP_USE_MARKUP]);
}

static void
bobgui_model_button_set_active (BobguiModelButton *button,
                             gboolean        active)
{
  active = !!active;
  if (button->active == active)
    return;

  button->active = active;

  update_accessible_properties (button);

  bobgui_model_button_update_state (button);
  bobgui_widget_queue_draw (BOBGUI_WIDGET (button));
  g_object_notify_by_pspec (G_OBJECT (button), properties[PROP_ACTIVE]);
}

static void
bobgui_model_button_set_menu_name (BobguiModelButton *button,
                                const char     *menu_name)
{
  g_free (button->menu_name);
  button->menu_name = g_strdup (menu_name);

  update_node_name (button);
  bobgui_model_button_update_state (button);

  update_accessible_properties (button);

  bobgui_widget_queue_resize (BOBGUI_WIDGET (button));
  g_object_notify_by_pspec (G_OBJECT (button), properties[PROP_MENU_NAME]);
}

static void
bobgui_model_button_set_iconic (BobguiModelButton *self,
                             gboolean        iconic)
{
  BobguiWidget *widget = BOBGUI_WIDGET (self);
  BobguiCssNode *widget_node;

  iconic = !!iconic;
  if (self->iconic == iconic)
    return;

  self->iconic = iconic;

  widget_node = bobgui_widget_get_css_node (widget);
  bobgui_widget_set_visible (self->start_box, !iconic);
  if (iconic)
    {
      bobgui_css_node_set_name (widget_node, g_quark_from_static_string ("button"));
      bobgui_widget_add_css_class (widget, "model");
      bobgui_widget_add_css_class (widget, "image-button");
      bobgui_widget_remove_css_class (widget, "flat");
    }
  else
    {
      bobgui_css_node_set_name (widget_node, g_quark_from_static_string ("modelbutton"));
      bobgui_widget_remove_css_class (widget, "model");
      bobgui_widget_remove_css_class (widget, "image-button");
      bobgui_widget_add_css_class (widget, "flat");
    }

  if (!iconic)
    {
      if (self->start_indicator)
        {
          bobgui_box_remove (BOBGUI_BOX (self->start_box), self->start_indicator);
          self->start_indicator = NULL;
        }
      g_clear_pointer (&self->end_indicator, bobgui_widget_unparent);
    }

  update_node_name (self);
  update_visibility (self);
  update_tooltip (self);
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_ICONIC]);
}

static void
bobgui_model_button_set_popover (BobguiModelButton *button,
                              BobguiWidget      *popover)
{
  if (button->popover)
    bobgui_widget_unparent (button->popover);

  button->popover = popover;

  if (button->popover)
    {
      bobgui_widget_set_parent (button->popover, BOBGUI_WIDGET (button));
      bobgui_popover_set_position (BOBGUI_POPOVER (button->popover), BOBGUI_POS_RIGHT);
    }

  update_accessible_properties (button);

  update_node_name (button);
  bobgui_model_button_update_state (button);

  bobgui_widget_queue_resize (BOBGUI_WIDGET (button));
  g_object_notify_by_pspec (G_OBJECT (button), properties[PROP_POPOVER]);
}

static void
update_accel (BobguiModelButton *self,
              const char     *accel)
{
  if (accel)
    {
      guint key;
      GdkModifierType mods;
      char *str;

      if (!self->accel_label)
        {
          self->accel_label = g_object_new (BOBGUI_TYPE_LABEL,
                                            "accessible-role", BOBGUI_ACCESSIBLE_ROLE_PRESENTATION,
                                            "css-name", "accelerator",
                                            NULL);
          bobgui_widget_insert_before (self->accel_label, BOBGUI_WIDGET (self), NULL);
          bobgui_widget_set_hexpand (self->accel_label, TRUE),
          bobgui_widget_set_halign (self->accel_label, BOBGUI_ALIGN_END);
        }

      bobgui_accelerator_parse (accel, &key, &mods);
      str = bobgui_accelerator_get_label (key, mods);
      bobgui_label_set_label (BOBGUI_LABEL (self->accel_label), str);
      g_free (str);

      if (BOBGUI_IS_POPOVER (bobgui_widget_get_native (BOBGUI_WIDGET (self))))
        {
          BobguiShortcutTrigger *trigger;
          BobguiShortcutAction *action;

          if (self->controller)
            {
              while (g_list_model_get_n_items (G_LIST_MODEL (self->controller)) > 0)
                {
                  BobguiShortcut *shortcut = g_list_model_get_item (G_LIST_MODEL (self->controller), 0);
                  bobgui_shortcut_controller_remove_shortcut (BOBGUI_SHORTCUT_CONTROLLER (self->controller),
                                                           shortcut);
                  g_object_unref (shortcut);
                }
            }
          else
            {
              self->controller = bobgui_shortcut_controller_new ();
              bobgui_shortcut_controller_set_scope (BOBGUI_SHORTCUT_CONTROLLER (self->controller), BOBGUI_SHORTCUT_SCOPE_MANAGED);
              bobgui_widget_add_controller (BOBGUI_WIDGET (self), self->controller);
            }

          trigger = bobgui_keyval_trigger_new (key, mods);
          action = bobgui_signal_action_new ("clicked");
          bobgui_shortcut_controller_add_shortcut (BOBGUI_SHORTCUT_CONTROLLER (self->controller),
                                                bobgui_shortcut_new (trigger, action));
        }
    }
  else
    {
      g_clear_pointer (&self->accel_label, bobgui_widget_unparent);
      if (self->controller)
        {
          bobgui_widget_remove_controller (BOBGUI_WIDGET (self), BOBGUI_EVENT_CONTROLLER (self->controller));
          self->controller = NULL;
        }
    }

  update_accessible_properties (self);
}

static void
bobgui_model_button_set_accel (BobguiModelButton *button,
                            const char     *accel)
{
  g_free (button->accel);
  button->accel = g_strdup (accel);
  update_accel (button, button->accel);
  update_visibility (button);

  g_object_notify_by_pspec (G_OBJECT (button), properties[PROP_ACCEL]);
}

static void
bobgui_model_button_get_property (GObject    *object,
                               guint       prop_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  BobguiModelButton *self = BOBGUI_MODEL_BUTTON (object);

  switch (prop_id)
    {
    case PROP_ROLE:
      g_value_set_enum (value, self->role);
      break;

    case PROP_ICON:
      g_value_set_object (value, self->image ? bobgui_image_get_gicon (BOBGUI_IMAGE (self->image)) : NULL);
      break;

    case PROP_TEXT:
      g_value_set_string (value, bobgui_label_get_text (BOBGUI_LABEL (self->label)));
      break;

    case PROP_USE_MARKUP:
      g_value_set_boolean (value, bobgui_label_get_use_markup (BOBGUI_LABEL (self->label)));
      break;

    case PROP_ACTIVE:
      g_value_set_boolean (value, self->active);
      break;

    case PROP_MENU_NAME:
      g_value_set_string (value, self->menu_name);
      break;

    case PROP_POPOVER:
      g_value_set_object (value, self->popover);
      break;

    case PROP_ICONIC:
      g_value_set_boolean (value, self->iconic);
      break;

    case PROP_ACCEL:
      g_value_set_string (value, self->accel);
      break;

    case PROP_INDICATOR_SIZE_GROUP:
      g_value_set_object (value, self->indicators);
      break;

    case PROP_ACTION_NAME:
      g_value_set_string (value, bobgui_action_helper_get_action_name (self->action_helper));
      break;

    case PROP_ACTION_TARGET:
      g_value_set_variant (value, bobgui_action_helper_get_action_target_value (self->action_helper));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_model_button_set_property (GObject      *object,
                               guint         prop_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  BobguiModelButton *button = BOBGUI_MODEL_BUTTON (object);

  switch (prop_id)
    {
    case PROP_ROLE:
      bobgui_model_button_set_role (button, g_value_get_enum (value));
      break;

    case PROP_ICON:
      bobgui_model_button_set_icon (button, g_value_get_object (value));
      break;

    case PROP_TEXT:
      bobgui_model_button_set_text (button, g_value_get_string (value));
      break;

    case PROP_USE_MARKUP:
      bobgui_model_button_set_use_markup (button, g_value_get_boolean (value));
      break;

    case PROP_ACTIVE:
      bobgui_model_button_set_active (button, g_value_get_boolean (value));
      break;

    case PROP_MENU_NAME:
      bobgui_model_button_set_menu_name (button, g_value_get_string (value));
      break;

    case PROP_POPOVER:
      bobgui_model_button_set_popover (button, (BobguiWidget *)g_value_get_object (value));
      break;

    case PROP_ICONIC:
      bobgui_model_button_set_iconic (button, g_value_get_boolean (value));
      break;

    case PROP_ACCEL:
      bobgui_model_button_set_accel (button, g_value_get_string (value));
      break;

    case PROP_INDICATOR_SIZE_GROUP:
      if (button->indicators)
        bobgui_size_group_remove_widget (button->indicators, button->start_box);
      button->indicators = BOBGUI_SIZE_GROUP (g_value_get_object (value));
      if (button->indicators)
        bobgui_size_group_add_widget (button->indicators, button->start_box);
      break;

    case PROP_ACTION_NAME:
      bobgui_model_button_set_action_name (BOBGUI_ACTIONABLE (button), g_value_get_string (value));
      break;

    case PROP_ACTION_TARGET:
      bobgui_model_button_set_action_target_value (BOBGUI_ACTIONABLE (button), g_value_get_variant (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_model_button_dispose (GObject *object)
{
  BobguiModelButton *model_button = BOBGUI_MODEL_BUTTON (object);

  g_clear_pointer (&model_button->menu_name, g_free);

  G_OBJECT_CLASS (bobgui_model_button_parent_class)->dispose (object);
}

static void
switch_menu (BobguiModelButton *button)
{
  BobguiWidget *stack;

  stack = bobgui_widget_get_ancestor (BOBGUI_WIDGET (button), BOBGUI_TYPE_STACK);
  if (stack != NULL)
    {
      if (button->role == BOBGUI_BUTTON_ROLE_NORMAL)
        {
          BobguiWidget *title_button = bobgui_widget_get_first_child (bobgui_stack_get_child_by_name (BOBGUI_STACK (stack), button->menu_name));
          bobgui_accessible_update_state (BOBGUI_ACCESSIBLE (button),
                                       BOBGUI_ACCESSIBLE_STATE_EXPANDED, TRUE,
                                       -1);
          bobgui_accessible_update_state (BOBGUI_ACCESSIBLE (title_button),
                                       BOBGUI_ACCESSIBLE_STATE_EXPANDED, TRUE,
                                       -1);
          g_object_set_data (G_OBJECT (title_button), "-bobgui-model-button-parent", button);
        }
      else if (button->role == BOBGUI_BUTTON_ROLE_TITLE)
        {
          BobguiWidget *parent_button = g_object_get_data (G_OBJECT (button), "-bobgui-model-button-parent");
          bobgui_accessible_update_state (BOBGUI_ACCESSIBLE (parent_button),
                                        BOBGUI_ACCESSIBLE_STATE_EXPANDED, FALSE,
                                        -1);
          bobgui_accessible_update_state (BOBGUI_ACCESSIBLE (button),
                                        BOBGUI_ACCESSIBLE_STATE_EXPANDED, FALSE,
                                        -1);
          g_object_set_data (G_OBJECT (button), "-bobgui-model-button-parent", NULL);
        }
      bobgui_stack_set_visible_child_name (BOBGUI_STACK (stack), button->menu_name);
    }
}

static void
bobgui_model_button_clicked (BobguiModelButton *self)
{
  if (self->menu_name != NULL)
    {
      switch_menu (self);
    }
  else if (self->popover != NULL)
    {
      BobguiPopoverMenu *menu;
      BobguiWidget *submenu;

      menu = (BobguiPopoverMenu *)bobgui_widget_get_ancestor (BOBGUI_WIDGET (self), BOBGUI_TYPE_POPOVER_MENU);
      submenu = self->popover;
      bobgui_popover_popup (BOBGUI_POPOVER (submenu));
      bobgui_popover_menu_set_open_submenu (menu, submenu);
      bobgui_popover_menu_set_parent_menu (BOBGUI_POPOVER_MENU (submenu), BOBGUI_WIDGET (menu));

      bobgui_accessible_update_state (BOBGUI_ACCESSIBLE (self),
                                   BOBGUI_ACCESSIBLE_STATE_EXPANDED, TRUE,
                                   -1);
    }
  else if (!self->keep_open)
    {
      BobguiWidget *popover;

      popover = bobgui_widget_get_ancestor (BOBGUI_WIDGET (self), BOBGUI_TYPE_POPOVER);
      if (popover)
        bobgui_popover_popdown (BOBGUI_POPOVER (popover));

      bobgui_accessible_update_state (BOBGUI_ACCESSIBLE (self),
                                   BOBGUI_ACCESSIBLE_STATE_EXPANDED, FALSE,
                                   -1);
    }

  if (self->action_helper)
    bobgui_action_helper_activate (self->action_helper);
}

static gboolean
toggle_cb (BobguiWidget *widget,
           GVariant  *args,
           gpointer   user_data)
{
  BobguiModelButton *self = BOBGUI_MODEL_BUTTON (widget);

  self->keep_open = self->role != BOBGUI_BUTTON_ROLE_NORMAL;
  g_signal_emit (widget, signals[SIGNAL_CLICKED], 0);
  self->keep_open = FALSE;

  return TRUE;
}

static void
bobgui_model_button_finalize (GObject *object)
{
  BobguiModelButton *button = BOBGUI_MODEL_BUTTON (object);

  g_clear_pointer (&button->image, bobgui_widget_unparent);
  g_clear_pointer (&button->label, bobgui_widget_unparent);
  g_clear_pointer (&button->start_box, bobgui_widget_unparent);
  g_clear_pointer (&button->accel_label, bobgui_widget_unparent);
  g_clear_pointer (&button->end_indicator, bobgui_widget_unparent);
  g_clear_object (&button->action_helper);
  g_free (button->accel);
  g_clear_pointer (&button->popover, bobgui_widget_unparent);

  if (button->open_timeout)
    g_source_remove (button->open_timeout);

  G_OBJECT_CLASS (bobgui_model_button_parent_class)->finalize (object);
}

static gboolean
bobgui_model_button_focus (BobguiWidget        *widget,
                        BobguiDirectionType  direction)
{
  BobguiModelButton *button = BOBGUI_MODEL_BUTTON (widget);

  if (bobgui_widget_is_focus (widget))
    {
      if (direction == BOBGUI_DIR_LEFT &&
          button->role == BOBGUI_BUTTON_ROLE_TITLE &&
          button->menu_name != NULL)
        {
          switch_menu (button);
          return TRUE;
        }
      else if (direction == BOBGUI_DIR_RIGHT &&
               button->role == BOBGUI_BUTTON_ROLE_NORMAL &&
               button->menu_name != NULL)
        {
          switch_menu (button);
          return TRUE;
        }
      else if (direction == BOBGUI_DIR_RIGHT &&
               button->role == BOBGUI_BUTTON_ROLE_NORMAL &&
               button->popover != NULL)
        {
          BobguiPopoverMenu *menu;
          BobguiWidget *submenu;

          menu = BOBGUI_POPOVER_MENU (bobgui_widget_get_ancestor (BOBGUI_WIDGET (button), BOBGUI_TYPE_POPOVER_MENU));
          submenu = button->popover;
          bobgui_popover_popup (BOBGUI_POPOVER (submenu));
          bobgui_popover_menu_set_open_submenu (menu, submenu);
          bobgui_popover_menu_set_parent_menu (BOBGUI_POPOVER_MENU (submenu), BOBGUI_WIDGET (menu));
          return TRUE;
        }
    }
  else
    {
      bobgui_widget_grab_focus (widget);
      return TRUE;
    }

  return FALSE;
}

static void
bobgui_model_button_class_init (BobguiModelButtonClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (class);
  BobguiShortcutAction *action;
  guint activate_keyvals[] = {
    GDK_KEY_Return, GDK_KEY_ISO_Enter, GDK_KEY_KP_Enter
  };
  guint toggle_keyvals[] = {
    GDK_KEY_space, GDK_KEY_KP_Space
  };
  int i;

  object_class->dispose = bobgui_model_button_dispose;
  object_class->finalize = bobgui_model_button_finalize;
  object_class->get_property = bobgui_model_button_get_property;
  object_class->set_property = bobgui_model_button_set_property;

  widget_class->state_flags_changed = bobgui_model_button_state_flags_changed;
  widget_class->direction_changed = bobgui_model_button_direction_changed;
  widget_class->focus = bobgui_model_button_focus;

  class->clicked = bobgui_model_button_clicked;

  /**
   * BobguiModelButton:role:
   *
   * Specifies whether the button is a plain, check or radio button.
   * When BobguiActionable:action-name is set, the role will be determined
   * from the action and does not have to be set explicitly.
   */
  properties[PROP_ROLE] =
    g_param_spec_enum ("role", NULL, NULL,
                       BOBGUI_TYPE_BUTTON_ROLE,
                       BOBGUI_BUTTON_ROLE_NORMAL,
                       G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * BobguiModelButton:icon:
   *
   * A GIcon that will be used if iconic appearance for the button is
   * desired.
   */
  properties[PROP_ICON] =
    g_param_spec_object ("icon", NULL, NULL,
                         G_TYPE_ICON,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * BobguiModelButton:text:
   *
   * The label for the button.
   */
  properties[PROP_TEXT] =
    g_param_spec_string ("text", NULL, NULL,
                         "",
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * BobguiModelButton:use-markup:
   *
   * If %TRUE, XML tags in the text of the button are interpreted as by
   * pango_parse_markup() to format the enclosed spans of text. If %FALSE, the
   * text will be displayed verbatim.
   */
  properties[PROP_USE_MARKUP] =
    g_param_spec_boolean ("use-markup", NULL, NULL,
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * BobguiModelButton:active:
   *
   * The state of the button. This is reflecting the state of the associated
   * GAction.
   */
  properties[PROP_ACTIVE] =
    g_param_spec_boolean ("active", NULL, NULL,
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * BobguiModelButton:menu-name:
   *
   * The name of a submenu to open when the button is activated.  * If this is set, the button should not have an action associated with it.
   */
  properties[PROP_MENU_NAME] =
    g_param_spec_string ("menu-name", NULL, NULL,
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

 properties[PROP_POPOVER] =
   g_param_spec_object ("popover", NULL, NULL,
                        BOBGUI_TYPE_POPOVER,
                        G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * BobguiModelButton:iconic:
   *
   * If this property is set, the button will show an icon if one is set.
   * If no icon is set, the text will be used. This is typically used for
   * horizontal sections of linked buttons.
   */
  properties[PROP_ICONIC] =
    g_param_spec_boolean ("iconic", NULL, NULL,
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * BobguiModelButton:indicator-size-group:
   *
   * Containers like BobguiPopoverMenu can provide a size group
   * in this property to align the checks and radios of all
   * the model buttons in a menu.
   */
  properties[PROP_INDICATOR_SIZE_GROUP] =
    g_param_spec_object ("indicator-size-group", NULL, NULL,
                          BOBGUI_TYPE_SIZE_GROUP,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);
  properties[PROP_ACCEL] =
    g_param_spec_string ("accel", NULL, NULL,
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);
  g_object_class_install_properties (object_class, LAST_PROP, properties);

  g_object_class_override_property (object_class, PROP_ACTION_NAME, "action-name");
  g_object_class_override_property (object_class, PROP_ACTION_TARGET, "action-target");

  signals[SIGNAL_CLICKED] = g_signal_new (I_("clicked"),
                                          G_OBJECT_CLASS_TYPE (object_class),
                                          G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                                          G_STRUCT_OFFSET (BobguiModelButtonClass, clicked),
                                          NULL, NULL,
                                          NULL,
                                          G_TYPE_NONE, 0);

  bobgui_widget_class_set_activate_signal (widget_class, signals[SIGNAL_CLICKED]);
  bobgui_widget_class_set_layout_manager_type (widget_class, BOBGUI_TYPE_BOX_LAYOUT);
  bobgui_widget_class_set_css_name (widget_class, I_("modelbutton"));
  bobgui_widget_class_set_accessible_role (widget_class, BOBGUI_ACCESSIBLE_ROLE_MENU_ITEM);

  action = bobgui_signal_action_new ("clicked");
  for (i = 0; i < G_N_ELEMENTS (activate_keyvals); i++)
    {
      BobguiShortcut *shortcut;

      shortcut = bobgui_shortcut_new (bobgui_keyval_trigger_new (activate_keyvals[i], 0),
                                   g_object_ref (action));
      bobgui_widget_class_add_shortcut (widget_class, shortcut);
      g_object_unref (shortcut);
    }
  g_object_unref (action);

  action = bobgui_callback_action_new (toggle_cb, NULL, NULL);
  for (i = 0; i < G_N_ELEMENTS (toggle_keyvals); i++)
    {
      BobguiShortcut *shortcut;

      shortcut = bobgui_shortcut_new (bobgui_keyval_trigger_new (toggle_keyvals[i], 0),
                                   g_object_ref (action));
      bobgui_widget_class_add_shortcut (widget_class, shortcut);
      g_object_unref (shortcut);
    }
  g_object_unref (action);
}

static gboolean
open_submenu (gpointer data)
{
  BobguiModelButton *button = data;
  BobguiPopover *popover;

  popover = (BobguiPopover*)bobgui_widget_get_ancestor (BOBGUI_WIDGET (button), BOBGUI_TYPE_POPOVER);

  if (BOBGUI_IS_POPOVER_MENU (popover))
    {
      bobgui_popover_menu_set_active_item (BOBGUI_POPOVER_MENU (popover), BOBGUI_WIDGET (button));

      if (button->popover)
        {
          BobguiWidget *submenu = button->popover;

          if (bobgui_popover_menu_get_open_submenu (BOBGUI_POPOVER_MENU (popover)) != submenu)
            bobgui_popover_menu_close_submenus (BOBGUI_POPOVER_MENU (popover));

          bobgui_popover_popup (BOBGUI_POPOVER (submenu));
          bobgui_popover_menu_set_open_submenu (BOBGUI_POPOVER_MENU (popover), submenu);
          bobgui_popover_menu_set_parent_menu (BOBGUI_POPOVER_MENU (submenu), BOBGUI_WIDGET (popover));
        }
      else
        {
          bobgui_popover_menu_close_submenus (BOBGUI_POPOVER_MENU (popover));
        }
    }

  button->open_timeout = 0;

  return G_SOURCE_REMOVE;
}

#define OPEN_TIMEOUT 80

static void
start_open (BobguiModelButton *button)
{
  if (button->open_timeout)
    g_source_remove (button->open_timeout);

  if (button->popover &&
      bobgui_widget_get_visible (button->popover))
    return;

  button->open_timeout = g_timeout_add (OPEN_TIMEOUT, open_submenu, button);
  gdk_source_set_static_name_by_id (button->open_timeout, "[bobgui] open_submenu");
}

static void
stop_open (BobguiModelButton *button)
{
  if (button->open_timeout)
    {
      g_source_remove (button->open_timeout);
      button->open_timeout = 0;
    }
}

static void
pointer_cb (GObject    *object,
            GParamSpec *pspec,
            gpointer    data)
{
  BobguiWidget *target = BOBGUI_WIDGET (data);
  BobguiWidget *popover;
  gboolean contains;

  contains = bobgui_event_controller_motion_contains_pointer (BOBGUI_EVENT_CONTROLLER_MOTION (object));

  popover = bobgui_widget_get_ancestor (target, BOBGUI_TYPE_POPOVER_MENU);

  if (contains)
    {
      if (popover)
        {
          if (bobgui_popover_menu_get_open_submenu (BOBGUI_POPOVER_MENU (popover)) != NULL)
            start_open (BOBGUI_MODEL_BUTTON (target));
          else
            open_submenu (target);
        }
    }
  else
    {
      BobguiModelButton *button = data;

      stop_open (button);
      if (popover)
        bobgui_popover_menu_set_active_item (BOBGUI_POPOVER_MENU (popover), NULL);
    }
}

static void
motion_cb (BobguiEventController *controller,
           double              x,
           double              y,
           gpointer            data)
{
  start_open (BOBGUI_MODEL_BUTTON (data));
}

static void
focus_in_cb (BobguiEventController   *controller,
             gpointer              data)
{
  BobguiWidget *target;
  BobguiWidget *popover;

  target = bobgui_event_controller_get_widget (controller);
  popover = bobgui_widget_get_ancestor (target, BOBGUI_TYPE_POPOVER_MENU);

  if (popover)
    bobgui_popover_menu_set_active_item (BOBGUI_POPOVER_MENU (popover), target);
}

static void
gesture_pressed (BobguiGestureClick *gesture,
                 guint            n_press,
                 double           x,
                 double           y,
                 BobguiWidget       *widget)
{
  if (bobgui_widget_get_focus_on_click (widget) && !bobgui_widget_has_focus (widget))
    bobgui_widget_grab_focus (widget);

  bobgui_gesture_set_state (BOBGUI_GESTURE (gesture), BOBGUI_EVENT_SEQUENCE_CLAIMED);
}

static void
gesture_released (BobguiGestureClick *gesture,
                  guint            n_press,
                  double           x,
                  double           y,
                  BobguiWidget       *widget)
{
  if (bobgui_widget_contains (widget, x, y))
    g_signal_emit (widget, signals[SIGNAL_CLICKED], 0);
}

static void
gesture_unpaired_release (BobguiGestureClick  *gesture,
                          double            x,
                          double            y,
                          guint             button,
                          GdkEventSequence *sequence,
                          BobguiWidget        *widget)
{
  if (bobgui_widget_contains (widget, x, y))
    g_signal_emit (widget, signals[SIGNAL_CLICKED], 0);
}

static void
bobgui_model_button_init (BobguiModelButton *self)
{
  BobguiEventController *controller;
  BobguiGesture *gesture;

  bobgui_widget_set_focusable (BOBGUI_WIDGET (self), TRUE);

  self->role = BOBGUI_BUTTON_ROLE_NORMAL;
  self->label = g_object_new (BOBGUI_TYPE_LABEL, "accessible-role", BOBGUI_ACCESSIBLE_ROLE_PRESENTATION, NULL);
  bobgui_widget_set_halign (self->label, BOBGUI_ALIGN_START);
  bobgui_widget_set_parent (self->label, BOBGUI_WIDGET (self));

  self->start_box = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
  bobgui_widget_insert_after (self->start_box, BOBGUI_WIDGET (self), NULL);
  update_node_ordering (self);

  bobgui_widget_add_css_class (BOBGUI_WIDGET (self), "flat");

  controller = bobgui_event_controller_motion_new ();
  g_signal_connect (controller, "notify::contains-pointer", G_CALLBACK (pointer_cb), self);
  g_signal_connect (controller, "motion", G_CALLBACK (motion_cb), self);
  bobgui_widget_add_controller (BOBGUI_WIDGET (self), controller);

  controller = bobgui_event_controller_focus_new ();
  bobgui_event_controller_set_propagation_limit (controller, BOBGUI_LIMIT_NONE);
  g_signal_connect (controller, "enter", G_CALLBACK (focus_in_cb), NULL);
  bobgui_widget_add_controller (BOBGUI_WIDGET (self), controller);

  gesture = bobgui_gesture_click_new ();
  bobgui_gesture_single_set_touch_only (BOBGUI_GESTURE_SINGLE (gesture), FALSE);
  bobgui_gesture_single_set_exclusive (BOBGUI_GESTURE_SINGLE (gesture), TRUE);
  bobgui_gesture_single_set_button (BOBGUI_GESTURE_SINGLE (gesture), GDK_BUTTON_PRIMARY);
  g_signal_connect (gesture, "pressed", G_CALLBACK (gesture_pressed), self);
  g_signal_connect (gesture, "released", G_CALLBACK (gesture_released), self);
  g_signal_connect (gesture, "unpaired-release", G_CALLBACK (gesture_unpaired_release), self);
  bobgui_event_controller_set_propagation_phase (BOBGUI_EVENT_CONTROLLER (gesture), BOBGUI_PHASE_CAPTURE);
  bobgui_widget_add_controller (BOBGUI_WIDGET (self), BOBGUI_EVENT_CONTROLLER (gesture));
}

/**
 * bobgui_model_button_new:
 *
 * Creates a new BobguiModelButton.
 *
 * Returns: the newly created BobguiModelButton widget
 */
BobguiWidget *
bobgui_model_button_new (void)
{
  return g_object_new (BOBGUI_TYPE_MODEL_BUTTON, NULL);
}
