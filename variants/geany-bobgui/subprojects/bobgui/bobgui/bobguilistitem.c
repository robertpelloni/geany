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

#include "bobguilistitemprivate.h"

#include "bobguicolumnviewcell.h"
#include "bobguiaccessible.h"

/**
 * BobguiListItem:
 *
 * Used by list widgets to represent items in a [iface@Gio.ListModel].
 *
 * `BobguiListItem` objects are managed by the list widget (with its factory)
 * and cannot be created by applications, but they need to be populated
 * by application code. This is done by calling [method@Bobgui.ListItem.set_child].
 *
 * `BobguiListItem` objects exist in 2 stages:
 *
 * 1. The unbound stage where the listitem is not currently connected to
 *    an item in the list. In that case, the [property@Bobgui.ListItem:item]
 *    property is set to `NULL`.
 *
 * 2. The bound stage where the listitem references an item from the list.
 *    The [property@Bobgui.ListItem:item] property is not `NULL`.
 */

enum
{
  PROP_0,
  PROP_ACCESSIBLE_DESCRIPTION,
  PROP_ACCESSIBLE_LABEL,
  PROP_ACTIVATABLE,
  PROP_CHILD,
  PROP_FOCUSABLE,
  PROP_ITEM,
  PROP_POSITION,
  PROP_SELECTABLE,
  PROP_SELECTED,

  N_PROPS
};

G_DEFINE_TYPE (BobguiListItem, bobgui_list_item, G_TYPE_OBJECT)

static GParamSpec *properties[N_PROPS] = { NULL, };

static void
bobgui_list_item_dispose (GObject *object)
{
  BobguiListItem *self = BOBGUI_LIST_ITEM (object);

  g_assert (self->owner == NULL); /* would hold a reference */
  g_clear_object (&self->child);

  g_clear_pointer (&self->accessible_description, g_free);
  g_clear_pointer (&self->accessible_label, g_free);

  G_OBJECT_CLASS (bobgui_list_item_parent_class)->dispose (object);
}

static void
bobgui_list_item_get_property (GObject    *object,
                            guint       property_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  BobguiListItem *self = BOBGUI_LIST_ITEM (object);

  switch (property_id)
    {
    case PROP_ACCESSIBLE_DESCRIPTION:
      g_value_set_string (value, self->accessible_description);
      break;

    case PROP_ACCESSIBLE_LABEL:
      g_value_set_string (value, self->accessible_label);
      break;

    case PROP_ACTIVATABLE:
      g_value_set_boolean (value, self->activatable);
      break;

    case PROP_CHILD:
      g_value_set_object (value, self->child);
      break;

    case PROP_FOCUSABLE:
      g_value_set_boolean (value, self->focusable);
      break;

    case PROP_ITEM:
      if (self->owner)
        g_value_set_object (value, bobgui_list_item_base_get_item (BOBGUI_LIST_ITEM_BASE (self->owner)));
      break;

    case PROP_POSITION:
      if (self->owner)
        g_value_set_uint (value, bobgui_list_item_base_get_position (BOBGUI_LIST_ITEM_BASE (self->owner)));
      else
        g_value_set_uint (value, BOBGUI_INVALID_LIST_POSITION);
      break;

    case PROP_SELECTABLE:
      g_value_set_boolean (value, self->selectable);
      break;

    case PROP_SELECTED:
      if (self->owner)
        g_value_set_boolean (value, bobgui_list_item_base_get_selected (BOBGUI_LIST_ITEM_BASE (self->owner)));
      else
        g_value_set_boolean (value, FALSE);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
bobgui_list_item_set_property (GObject      *object,
                            guint         property_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  BobguiListItem *self = BOBGUI_LIST_ITEM (object);

  switch (property_id)
    {
    case PROP_ACCESSIBLE_DESCRIPTION:
      bobgui_list_item_set_accessible_description (self, g_value_get_string (value));
      break;

    case PROP_ACCESSIBLE_LABEL:
      bobgui_list_item_set_accessible_label (self, g_value_get_string (value));
      break;

    case PROP_ACTIVATABLE:
      bobgui_list_item_set_activatable (self, g_value_get_boolean (value));
      break;

    case PROP_CHILD:
      bobgui_list_item_set_child (self, g_value_get_object (value));
      break;

    case PROP_FOCUSABLE:
      bobgui_list_item_set_focusable (self, g_value_get_boolean (value));
      break;

    case PROP_SELECTABLE:
      bobgui_list_item_set_selectable (self, g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
bobgui_list_item_class_init (BobguiListItemClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->dispose = bobgui_list_item_dispose;
  gobject_class->get_property = bobgui_list_item_get_property;
  gobject_class->set_property = bobgui_list_item_set_property;

  /**
   * BobguiListItem:accessible-description:
   *
   * The accessible description to set on the listitem.
   *
   * Since: 4.12
   */
  properties[PROP_ACCESSIBLE_DESCRIPTION] =
    g_param_spec_string ("accessible-description", NULL, NULL,
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * BobguiListItem:accessible-label:
   *
   * The accessible label to set on the listitem.
   *
   * Since: 4.12
   */
  properties[PROP_ACCESSIBLE_LABEL] =
    g_param_spec_string ("accessible-label", NULL, NULL,
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * BobguiListItem:activatable:
   *
   * If the item can be activated by the user.
   */
  properties[PROP_ACTIVATABLE] =
    g_param_spec_boolean ("activatable", NULL, NULL,
                          TRUE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * BobguiListItem:child:
   *
   * Widget used for display.
   */
  properties[PROP_CHILD] =
    g_param_spec_object ("child", NULL, NULL,
                         BOBGUI_TYPE_WIDGET,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * BobguiListItem:focusable:
   *
   * If the item can be focused with the keyboard.
   *
   * Since: 4.12
   */
  properties[PROP_FOCUSABLE] =
    g_param_spec_boolean ("focusable", NULL, NULL,
                          TRUE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * BobguiListItem:item:
   *
   * Displayed item.
   */
  properties[PROP_ITEM] =
    g_param_spec_object ("item", NULL, NULL,
                         G_TYPE_OBJECT,
                         G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * BobguiListItem:position:
   *
   * Position of the item.
   */
  properties[PROP_POSITION] =
    g_param_spec_uint ("position", NULL, NULL,
                       0, G_MAXUINT, BOBGUI_INVALID_LIST_POSITION,
                       G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * BobguiListItem:selectable:
   *
   * If the item can be selected by the user.
   */
  properties[PROP_SELECTABLE] =
    g_param_spec_boolean ("selectable", NULL, NULL,
                          TRUE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * BobguiListItem:selected:
   *
   * If the item is currently selected.
   */
  properties[PROP_SELECTED] =
    g_param_spec_boolean ("selected", NULL, NULL,
                          FALSE,
                          G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (gobject_class, N_PROPS, properties);
}

static void
bobgui_list_item_init (BobguiListItem *self)
{
  self->selectable = TRUE;
  self->activatable = TRUE;
  self->focusable = TRUE;
}

BobguiListItem *
bobgui_list_item_new (void)
{
  return g_object_new (BOBGUI_TYPE_LIST_ITEM, NULL);
}

void
bobgui_list_item_do_notify (BobguiListItem *list_item,
                         gboolean notify_item,
                         gboolean notify_position,
                         gboolean notify_selected)
{
  GObject *object = G_OBJECT (list_item);

  if (notify_item)
    g_object_notify_by_pspec (object, properties[PROP_ITEM]);
  if (notify_position)
    g_object_notify_by_pspec (object, properties[PROP_POSITION]);
  if (notify_selected)
    g_object_notify_by_pspec (object, properties[PROP_SELECTED]);
}

/**
 * bobgui_list_item_get_item:
 * @self: a listitem
 *
 * Gets the model item that associated with @self.
 *
 * If @self is unbound, this function returns `NULL`.
 *
 * Returns: (nullable) (transfer none) (type GObject): The item displayed
 **/
gpointer
bobgui_list_item_get_item (BobguiListItem *self)
{
  g_return_val_if_fail (BOBGUI_IS_LIST_ITEM (self), NULL);

  if (self->owner)
    return bobgui_list_item_base_get_item (BOBGUI_LIST_ITEM_BASE (self->owner));
  else if (BOBGUI_IS_COLUMN_VIEW_CELL (self))
    return bobgui_column_view_cell_get_item (BOBGUI_COLUMN_VIEW_CELL (self));
  else
    return NULL;
}

/**
 * bobgui_list_item_get_child:
 * @self: a listitem
 *
 * Gets the child previously set via [method@Bobgui.ListItem.set_child]
 * or `NULL` if none was set.
 *
 * Returns: (transfer none) (nullable): The child
 */
BobguiWidget *
bobgui_list_item_get_child (BobguiListItem *self)
{
  g_return_val_if_fail (BOBGUI_IS_LIST_ITEM (self), NULL);

  if (BOBGUI_IS_COLUMN_VIEW_CELL (self))
    return bobgui_column_view_cell_get_child (BOBGUI_COLUMN_VIEW_CELL (self));

  return self->child;
}

/**
 * bobgui_list_item_set_child:
 * @self: a listitem
 * @child: (nullable): The listitem's child or `NULL` to unset
 *
 * Sets the child to be used for this listitem.
 *
 * This function is typically called by applications when
 * setting up a listitem so that the widget can be reused when
 * binding it multiple times.
 */
void
bobgui_list_item_set_child (BobguiListItem *self,
                         BobguiWidget   *child)
{
  g_return_if_fail (BOBGUI_IS_LIST_ITEM (self));
  g_return_if_fail (child == NULL || bobgui_widget_get_parent (child) == NULL);

  if (BOBGUI_IS_COLUMN_VIEW_CELL (self))
    {
      bobgui_column_view_cell_set_child (BOBGUI_COLUMN_VIEW_CELL (self), child);
      return;
    }

  if (self->child == child)
    return;

  g_clear_object (&self->child);

  if (child)
    {
      g_object_ref_sink (child);
      self->child = child;

      /* Workaround that hopefully achieves good enough backwards
       * compatibility with people using expanders.
       */
      if (!self->focusable_set)
        bobgui_list_item_set_focusable (self, !bobgui_widget_get_focusable (child));
    }

  if (self->owner)
    bobgui_list_item_widget_set_child (self->owner, child);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_CHILD]);
}

/**
 * bobgui_list_item_get_position:
 * @self: a listitem
 *
 * Gets the position in the model that @self currently displays.
 *
 * If @self is unbound, `BOBGUI_INVALID_LIST_POSITION` is returned.
 *
 * Returns: The position of this item
 */
guint
bobgui_list_item_get_position (BobguiListItem *self)
{
  g_return_val_if_fail (BOBGUI_IS_LIST_ITEM (self), BOBGUI_INVALID_LIST_POSITION);

  if (self->owner)
    return bobgui_list_item_base_get_position (BOBGUI_LIST_ITEM_BASE (self->owner));
  else if (BOBGUI_IS_COLUMN_VIEW_CELL (self))
    return bobgui_column_view_cell_get_position (BOBGUI_COLUMN_VIEW_CELL (self));
  else
    return BOBGUI_INVALID_LIST_POSITION;
}

/**
 * bobgui_list_item_get_selected:
 * @self: a listitem
 *
 * Checks if the item is displayed as selected.
 *
 * The selected state is maintained by the list widget and its model
 * and cannot be set otherwise.
 *
 * Returns: true if the item is selected.
 */
gboolean
bobgui_list_item_get_selected (BobguiListItem *self)
{
  g_return_val_if_fail (BOBGUI_IS_LIST_ITEM (self), FALSE);

  if (self->owner)
    return bobgui_list_item_base_get_selected (BOBGUI_LIST_ITEM_BASE (self->owner));
  else if (BOBGUI_IS_COLUMN_VIEW_CELL (self))
    return bobgui_column_view_cell_get_selected (BOBGUI_COLUMN_VIEW_CELL (self));
  else
    return FALSE;
}

/**
 * bobgui_list_item_get_selectable:
 * @self: a listitem
 *
 * Checks if a listitem has been set to be selectable via
 * [method@Bobgui.ListItem.set_selectable].
 *
 * Do not confuse this function with [method@Bobgui.ListItem.get_selected].
 *
 * Returns: true if the item is selectable
 */
gboolean
bobgui_list_item_get_selectable (BobguiListItem *self)
{
  g_return_val_if_fail (BOBGUI_IS_LIST_ITEM (self), FALSE);

  return self->selectable;
}

/**
 * bobgui_list_item_set_selectable:
 * @self: a listitem
 * @selectable: if the item should be selectable
 *
 * Sets @self to be selectable.
 *
 * If an item is selectable, clicking on the item or using the keyboard
 * will try to select or unselect the item. If this succeeds is up to
 * the model to determine, as it is managing the selected state.
 *
 * Note that this means that making an item non-selectable has no
 * influence on the selected state at all. A non-selectable item
 * may still be selected.
 *
 * By default, listitems are selectable. When rebinding them to
 * a new item, they will also be reset to be selectable by BOBGUI.
 */
void
bobgui_list_item_set_selectable (BobguiListItem *self,
                              gboolean     selectable)
{
  g_return_if_fail (BOBGUI_IS_LIST_ITEM (self));

  if (self->selectable == selectable)
    return;

  self->selectable = selectable;

  if (self->owner)
    bobgui_list_factory_widget_set_selectable (BOBGUI_LIST_FACTORY_WIDGET (self->owner), selectable);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_SELECTABLE]);
}

/**
 * bobgui_list_item_get_activatable:
 * @self: a listitem
 *
 * Checks if a listitem has been set to be activatable via
 * [method@Bobgui.ListItem.set_activatable].
 *
 * Returns: true if the item is activatable
 */
gboolean
bobgui_list_item_get_activatable (BobguiListItem *self)
{
  g_return_val_if_fail (BOBGUI_IS_LIST_ITEM (self), FALSE);

  return self->activatable;
}

/**
 * bobgui_list_item_set_activatable:
 * @self: a listitem
 * @activatable: if the item should be activatable
 *
 * Sets @self to be activatable.
 *
 * If an item is activatable, double-clicking on the item, using
 * the Return key or calling [method@Bobgui.Widget.activate] will activate
 * the item. Activating instructs the containing view to handle
 * activation. `BobguiListView` for example will be emitting the
 * [signal@Bobgui.ListView::activate] signal.
 *
 * By default, listitems are activatable.
 */
void
bobgui_list_item_set_activatable (BobguiListItem *self,
                               gboolean     activatable)
{
  g_return_if_fail (BOBGUI_IS_LIST_ITEM (self));

  if (self->activatable == activatable)
    return;

  self->activatable = activatable;

  if (self->owner)
    bobgui_list_factory_widget_set_activatable (BOBGUI_LIST_FACTORY_WIDGET (self->owner), activatable);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_ACTIVATABLE]);
}

/**
 * bobgui_list_item_get_focusable:
 * @self: a lits item
 *
 * Checks if a listitem has been set to be focusable via
 * [method@Bobgui.ListItem.set_focusable].
 *
 * Returns: true if the item is focusable
 *
 * Since: 4.12
 */
gboolean
bobgui_list_item_get_focusable (BobguiListItem *self)
{
  g_return_val_if_fail (BOBGUI_IS_LIST_ITEM (self), FALSE);

  return self->focusable;
}

/**
 * bobgui_list_item_set_focusable:
 * @self: a listitem
 * @focusable: if the item should be focusable
 *
 * Sets @self to be focusable.
 *
 * If an item is focusable, it can be focused using the keyboard.
 * This works similar to [method@Bobgui.Widget.set_focusable].
 *
 * Note that if items are not focusable, the keyboard cannot be used to activate
 * them and selecting only works if one of the listitem's children is focusable.
 *
 * By default, listitems are focusable.
 *
 * Since: 4.12
 */
void
bobgui_list_item_set_focusable (BobguiListItem *self,
                             gboolean     focusable)
{
  g_return_if_fail (BOBGUI_IS_LIST_ITEM (self));

  self->focusable_set = TRUE;

  if (self->focusable == focusable)
    return;

  self->focusable = focusable;

  if (self->owner)
    bobgui_widget_set_focusable (BOBGUI_WIDGET (self->owner), focusable);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_FOCUSABLE]);
}

/**
 * bobgui_list_item_set_accessible_description:
 * @self: a listitem
 * @description: the description
 *
 * Sets the accessible description for the listitem.
 *
 * The accessible description may be used by e.g. screen readers.
 *
 * Since: 4.12
 */
void
bobgui_list_item_set_accessible_description (BobguiListItem *self,
                                          const char  *description)
{
  g_return_if_fail (BOBGUI_IS_LIST_ITEM (self));

  if (!g_set_str (&self->accessible_description, description))
    return;

  if (self->owner)
    bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (self->owner),
                                    BOBGUI_ACCESSIBLE_PROPERTY_DESCRIPTION, self->accessible_description,
                                    -1);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_ACCESSIBLE_DESCRIPTION]);
}

/**
 * bobgui_list_item_get_accessible_description:
 * @self: a listitem
 *
 * Gets the accessible description of @self.
 *
 * Returns: the accessible description
 *
 * Since: 4.12
 */
const char *
bobgui_list_item_get_accessible_description (BobguiListItem *self)
{
  g_return_val_if_fail (BOBGUI_IS_LIST_ITEM (self), NULL);

  return self->accessible_description;
}

/**
 * bobgui_list_item_set_accessible_label:
 * @self: a listitem
 * @label: the label
 *
 * Sets the accessible label for the listitem.
 *
 * The accessible label may be used by e.g. screen readers.
 *
 * Since: 4.12
 */
void
bobgui_list_item_set_accessible_label (BobguiListItem *self,
                                    const char  *label)
{
  g_return_if_fail (BOBGUI_IS_LIST_ITEM (self));

  if (!g_set_str (&self->accessible_label, label))
    return;

  if (self->owner)
    bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (self->owner),
                                    BOBGUI_ACCESSIBLE_PROPERTY_LABEL, self->accessible_label,
                                    -1);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_ACCESSIBLE_LABEL]);
}

/**
 * bobgui_list_item_get_accessible_label:
 * @self: a listitem
 *
 * Gets the accessible label of @self.
 *
 * Returns: the accessible label
 *
 * Since: 4.12
 */
const char *
bobgui_list_item_get_accessible_label (BobguiListItem *self)
{
  g_return_val_if_fail (BOBGUI_IS_LIST_ITEM (self), NULL);

  return self->accessible_label;
}
