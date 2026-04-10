/*
 * Copyright © 2018 Benjamin Otte
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

#include "bobguilistitemwidgetprivate.h"

#include "bobguibinlayout.h"
#include "bobguieventcontrollermotion.h"
#include "bobguigestureclick.h"
#include "bobguilistitemfactoryprivate.h"
#include "bobguilistitemprivate.h"
#include "bobguilistbaseprivate.h"
#include "bobguiwidget.h"
#include "bobguiwidgetprivate.h"

G_DEFINE_TYPE (BobguiListItemWidget, bobgui_list_item_widget, BOBGUI_TYPE_LIST_FACTORY_WIDGET)

static gboolean
bobgui_list_item_widget_focus (BobguiWidget        *widget,
                            BobguiDirectionType  direction)
{
  BobguiWidget *child = bobgui_widget_get_first_child (widget);
  BobguiWidget *focus_child = bobgui_widget_get_focus_child (widget);

  if (focus_child)
    {
      /* focus is in the child */
      if (bobgui_widget_child_focus (focus_child, direction))
        return TRUE;
      else if (direction == BOBGUI_DIR_TAB_BACKWARD)
        return bobgui_widget_grab_focus_self (widget);
      else
        return FALSE;
    }
  else if (bobgui_widget_is_focus (widget))
    {
      /* The widget has focus */
      if (direction == BOBGUI_DIR_TAB_FORWARD)
        {
          if (child)
            return bobgui_widget_child_focus (child, direction);
        }

      return FALSE;
    }
  else
    {
      /* focus coming in from the outside */
      if (direction == BOBGUI_DIR_TAB_BACKWARD)
        {
          if (child &&
              bobgui_widget_child_focus (child, direction))
            return TRUE;

          return bobgui_widget_grab_focus_self (widget);
        }
      else
        {
          if (bobgui_widget_grab_focus_self (widget))
            return TRUE;

          if (child &&
              bobgui_widget_child_focus (child, direction))
            return TRUE;

          return FALSE;
        }
    }
}

static gboolean
bobgui_list_item_widget_grab_focus (BobguiWidget *widget)
{
  BobguiWidget *child;

  if (BOBGUI_WIDGET_CLASS (bobgui_list_item_widget_parent_class)->grab_focus (widget))
    return TRUE;

  child = bobgui_widget_get_first_child (widget);
  if (child && bobgui_widget_grab_focus (child))
    return TRUE;

  return FALSE;
}

static gpointer
bobgui_list_item_widget_create_object (BobguiListFactoryWidget *fw)
{
  return bobgui_list_item_new ();
}

static void
bobgui_list_item_widget_setup_object (BobguiListFactoryWidget *fw,
                                   gpointer              object)
{
  BobguiListItemWidget *self = BOBGUI_LIST_ITEM_WIDGET (fw);
  BobguiListItem *list_item = object;

  BOBGUI_LIST_FACTORY_WIDGET_CLASS (bobgui_list_item_widget_parent_class)->setup_object (fw, object);

  list_item->owner = self;

  bobgui_list_item_widget_set_child (self, list_item->child);

  bobgui_list_factory_widget_set_activatable (fw, list_item->activatable);
  bobgui_list_factory_widget_set_selectable (fw, list_item->selectable);
  bobgui_widget_set_focusable (BOBGUI_WIDGET (self), list_item->focusable);

  bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (self),
                                  BOBGUI_ACCESSIBLE_PROPERTY_LABEL, list_item->accessible_label,
                                  BOBGUI_ACCESSIBLE_PROPERTY_DESCRIPTION, list_item->accessible_description,
                                  -1);

  bobgui_list_item_do_notify (list_item,
                           bobgui_list_item_base_get_item (BOBGUI_LIST_ITEM_BASE (self)) != NULL,
                           bobgui_list_item_base_get_position (BOBGUI_LIST_ITEM_BASE (self)) != BOBGUI_INVALID_LIST_POSITION,
                           bobgui_list_item_base_get_selected (BOBGUI_LIST_ITEM_BASE (self)));
}

static void
bobgui_list_item_widget_teardown_object (BobguiListFactoryWidget *fw,
                                      gpointer              object)
{
  BobguiListItemWidget *self = BOBGUI_LIST_ITEM_WIDGET (fw);
  BobguiListItem *list_item = object;

  BOBGUI_LIST_FACTORY_WIDGET_CLASS (bobgui_list_item_widget_parent_class)->teardown_object (fw, object);

  list_item->owner = NULL;

  bobgui_list_item_widget_set_child (self, NULL);

  bobgui_list_factory_widget_set_activatable (fw, FALSE);
  bobgui_list_factory_widget_set_selectable (fw, FALSE);
  bobgui_widget_set_focusable (BOBGUI_WIDGET (self), TRUE);

  bobgui_accessible_reset_property (BOBGUI_ACCESSIBLE (self), BOBGUI_ACCESSIBLE_PROPERTY_LABEL);
  bobgui_accessible_reset_property (BOBGUI_ACCESSIBLE (self), BOBGUI_ACCESSIBLE_PROPERTY_DESCRIPTION);

  bobgui_list_item_do_notify (list_item,
                           bobgui_list_item_base_get_item (BOBGUI_LIST_ITEM_BASE (self)) != NULL,
                           bobgui_list_item_base_get_position (BOBGUI_LIST_ITEM_BASE (self)) != BOBGUI_INVALID_LIST_POSITION,
                           bobgui_list_item_base_get_selected (BOBGUI_LIST_ITEM_BASE (self)));

  /* FIXME: This is technically not correct, the child is user code, isn't it? */
  bobgui_list_item_set_child (list_item, NULL);
}

static void
bobgui_list_item_widget_update_object (BobguiListFactoryWidget *fw,
                                    gpointer              object,
                                    guint                 position,
                                    gpointer              item,
                                    gboolean              selected)
{
  BobguiListItemWidget *self = BOBGUI_LIST_ITEM_WIDGET (fw);
  BobguiListItemBase *base = BOBGUI_LIST_ITEM_BASE (self);
  BobguiListItem *list_item = object;
  /* Track notify manually instead of freeze/thaw_notify for performance reasons. */
  gboolean notify_item = FALSE, notify_position = FALSE, notify_selected = FALSE;

  /* FIXME: It's kinda evil to notify external objects from here... */
  notify_item = bobgui_list_item_base_get_item (base) != item;
  notify_position = bobgui_list_item_base_get_position (base) != position;
  notify_selected = bobgui_list_item_base_get_selected (base) != selected;

  BOBGUI_LIST_FACTORY_WIDGET_CLASS (bobgui_list_item_widget_parent_class)->update_object (fw,
                                                                                    object,
                                                                                    position,
                                                                                    item,
                                                                                    selected);

  if (list_item)
    bobgui_list_item_do_notify (list_item, notify_item, notify_position, notify_selected);
}

static void
bobgui_list_item_widget_class_init (BobguiListItemWidgetClass *klass)
{
  BobguiListFactoryWidgetClass *factory_class = BOBGUI_LIST_FACTORY_WIDGET_CLASS (klass);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);

  factory_class->create_object = bobgui_list_item_widget_create_object;
  factory_class->setup_object = bobgui_list_item_widget_setup_object;
  factory_class->update_object = bobgui_list_item_widget_update_object;
  factory_class->teardown_object = bobgui_list_item_widget_teardown_object;

  widget_class->focus = bobgui_list_item_widget_focus;
  widget_class->grab_focus = bobgui_list_item_widget_grab_focus;

  /* This gets overwritten by bobgui_list_item_widget_new() but better safe than sorry */
  bobgui_widget_class_set_css_name (widget_class, I_("row"));
  bobgui_widget_class_set_layout_manager_type (widget_class, BOBGUI_TYPE_BIN_LAYOUT);
}

static void
bobgui_list_item_widget_init (BobguiListItemWidget *self)
{
  bobgui_widget_set_focusable (BOBGUI_WIDGET (self), TRUE);
}

BobguiWidget *
bobgui_list_item_widget_new (BobguiListItemFactory *factory,
                          const char         *css_name,
                          BobguiAccessibleRole   role)
{
  g_return_val_if_fail (css_name != NULL, NULL);

  return g_object_new (BOBGUI_TYPE_LIST_ITEM_WIDGET,
                       "css-name", css_name,
                       "accessible-role", role,
                       "factory", factory,
                       NULL);
}

void
bobgui_list_item_widget_set_child (BobguiListItemWidget *self,
                                BobguiWidget         *child)
{
  BobguiWidget *cur_child = bobgui_widget_get_first_child (BOBGUI_WIDGET (self));

  if (cur_child == child)
    return;

  g_clear_pointer (&cur_child, bobgui_widget_unparent);

  if (child)
    bobgui_widget_set_parent (child, BOBGUI_WIDGET (self));
}

