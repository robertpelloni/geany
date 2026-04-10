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

#include "bobguicolumnviewcellprivate.h"

#include "bobguilistitembaseprivate.h"

/**
 * BobguiColumnViewCell:
 *
 * Represents items in a cell in [class@Bobgui.ColumnView].
 *
 * The `BobguiColumnViewCell`s are managed by the [class@Bobgui.ColumnView]
 * widget (with its factory) and cannot be created by applications, but
 * they need to be populated by application code. This is done by calling
 * [method@Bobgui.ColumnViewCell.set_child].
 *
 * `BobguiColumnViewCell`s exist in 2 stages:
 *
 * 1. The unbound stage where the listitem is not currently connected to
 *    an item in the list. In that case, the [property@Bobgui.ColumnViewCell:item]
 *    property is set to %NULL.
 *
 * 2. The bound stage where the listitem references an item from the list.
 *    The [property@Bobgui.ColumnViewCell:item] property is not %NULL.
 *
 * Since: 4.12
 */

struct _BobguiColumnViewCellClass
{
  BobguiListItemClass parent_class;
};

enum
{
  PROP_0,
  PROP_CHILD,
  PROP_FOCUSABLE,
  PROP_ITEM,
  PROP_POSITION,
  PROP_SELECTED,

  N_PROPS
};

G_DEFINE_TYPE (BobguiColumnViewCell, bobgui_column_view_cell, BOBGUI_TYPE_LIST_ITEM)

static GParamSpec *properties[N_PROPS] = { NULL, };

static void
bobgui_column_view_cell_dispose (GObject *object)
{
  BobguiColumnViewCell *self = BOBGUI_COLUMN_VIEW_CELL (object);

  g_assert (self->cell == NULL); /* would hold a reference */
  g_clear_object (&self->child);

  G_OBJECT_CLASS (bobgui_column_view_cell_parent_class)->dispose (object);
}

static void
bobgui_column_view_cell_get_property (GObject    *object,
                                   guint       property_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  BobguiColumnViewCell *self = BOBGUI_COLUMN_VIEW_CELL (object);

  switch (property_id)
    {
    case PROP_CHILD:
      g_value_set_object (value, self->child);
      break;

    case PROP_FOCUSABLE:
      g_value_set_boolean (value, self->focusable);
      break;

    case PROP_ITEM:
      if (self->cell)
        g_value_set_object (value, bobgui_list_item_base_get_item (BOBGUI_LIST_ITEM_BASE (self->cell)));
      break;

    case PROP_POSITION:
      if (self->cell)
        g_value_set_uint (value, bobgui_list_item_base_get_position (BOBGUI_LIST_ITEM_BASE (self->cell)));
      else
        g_value_set_uint (value, BOBGUI_INVALID_LIST_POSITION);
      break;

    case PROP_SELECTED:
      if (self->cell)
        g_value_set_boolean (value, bobgui_list_item_base_get_selected (BOBGUI_LIST_ITEM_BASE (self->cell)));
      else
        g_value_set_boolean (value, FALSE);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
bobgui_column_view_cell_set_property (GObject      *object,
                            guint         property_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  BobguiColumnViewCell *self = BOBGUI_COLUMN_VIEW_CELL (object);

  switch (property_id)
    {
    case PROP_CHILD:
      bobgui_column_view_cell_set_child (self, g_value_get_object (value));
      break;

    case PROP_FOCUSABLE:
      bobgui_column_view_cell_set_focusable (self, g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
bobgui_column_view_cell_class_init (BobguiColumnViewCellClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->dispose = bobgui_column_view_cell_dispose;
  gobject_class->get_property = bobgui_column_view_cell_get_property;
  gobject_class->set_property = bobgui_column_view_cell_set_property;

  /**
   * BobguiColumnViewCell:child:
   *
   * Widget used for display.
   *
   * Since: 4.12
   */
  properties[PROP_CHILD] =
    g_param_spec_object ("child", NULL, NULL,
                         BOBGUI_TYPE_WIDGET,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * BobguiColumnViewCell:focusable:
   *
   * If the item can be focused with the keyboard.
   *
   * Since: 4.12
   */
  properties[PROP_FOCUSABLE] =
    g_param_spec_boolean ("focusable", NULL, NULL,
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * BobguiColumnViewCell:item:
   *
   * Displayed item.
   *
   * Since: 4.12
   */
  properties[PROP_ITEM] =
    g_param_spec_object ("item", NULL, NULL,
                         G_TYPE_OBJECT,
                         G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * BobguiColumnViewCell:position:
   *
   * Position of the item.
   *
   * Since: 4.12
   */
  properties[PROP_POSITION] =
    g_param_spec_uint ("position", NULL, NULL,
                       0, G_MAXUINT, BOBGUI_INVALID_LIST_POSITION,
                       G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * BobguiColumnViewCell:selected:
   *
   * If the item is currently selected.
   *
   * Since: 4.12
   */
  properties[PROP_SELECTED] =
    g_param_spec_boolean ("selected", NULL, NULL,
                          FALSE,
                          G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (gobject_class, N_PROPS, properties);
}

static void
bobgui_column_view_cell_init (BobguiColumnViewCell *self)
{
  self->focusable = FALSE;
}

BobguiColumnViewCell *
bobgui_column_view_cell_new (void)
{
  return g_object_new (BOBGUI_TYPE_COLUMN_VIEW_CELL, NULL);
}

void
bobgui_column_view_cell_do_notify (BobguiColumnViewCell *column_view_cell,
                                gboolean notify_item,
                                gboolean notify_position,
                                gboolean notify_selected)
{
  GObject *object = G_OBJECT (column_view_cell);

  if (notify_item)
    g_object_notify_by_pspec (object, properties[PROP_ITEM]);
  if (notify_position)
    g_object_notify_by_pspec (object, properties[PROP_POSITION]);
  if (notify_selected)
    g_object_notify_by_pspec (object, properties[PROP_SELECTED]);
}

/**
 * bobgui_column_view_cell_get_item:
 * @self: a `BobguiColumnViewCell`
 *
 * Gets the model item that associated with @self.
 *
 * If @self is unbound, this function returns %NULL.
 *
 * Returns: (nullable) (transfer none) (type GObject): The item displayed
 *
 * Since: 4.12
 **/
gpointer
bobgui_column_view_cell_get_item (BobguiColumnViewCell *self)
{
  g_return_val_if_fail (BOBGUI_IS_COLUMN_VIEW_CELL (self), NULL);

  if (self->cell == NULL)
    return NULL;

  return bobgui_list_item_base_get_item (BOBGUI_LIST_ITEM_BASE (self->cell));
}

/**
 * bobgui_column_view_cell_get_child:
 * @self: a `BobguiColumnViewCell`
 *
 * Gets the child previously set via bobgui_column_view_cell_set_child() or
 * %NULL if none was set.
 *
 * Returns: (transfer none) (nullable): The child
 *
 * Since: 4.12
 */
BobguiWidget *
bobgui_column_view_cell_get_child (BobguiColumnViewCell *self)
{
  g_return_val_if_fail (BOBGUI_IS_COLUMN_VIEW_CELL (self), NULL);

  return self->child;
}

/**
 * bobgui_column_view_cell_set_child:
 * @self: a `BobguiColumnViewCell`
 * @child: (nullable): The list item's child or %NULL to unset
 *
 * Sets the child to be used for this listitem.
 *
 * This function is typically called by applications when
 * setting up a listitem so that the widget can be reused when
 * binding it multiple times.
 *
 * Since: 4.12
 */
void
bobgui_column_view_cell_set_child (BobguiColumnViewCell *self,
                                BobguiWidget   *child)
{
  g_return_if_fail (BOBGUI_IS_COLUMN_VIEW_CELL (self));
  g_return_if_fail (child == NULL || BOBGUI_IS_WIDGET (child));

  if (self->child == child)
    return;

  g_clear_object (&self->child);

  if (child)
    {
      g_object_ref_sink (child);
      self->child = child;
    }

  if (self->cell)
    bobgui_column_view_cell_widget_set_child (self->cell, child);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_CHILD]);
}

/**
 * bobgui_column_view_cell_get_position:
 * @self: a `BobguiColumnViewCell`
 *
 * Gets the position in the model that @self currently displays.
 *
 * If @self is unbound, %BOBGUI_INVALID_LIST_POSITION is returned.
 *
 * Returns: The position of this item
 *
 * Since: 4.12
 */
guint
bobgui_column_view_cell_get_position (BobguiColumnViewCell *self)
{
  g_return_val_if_fail (BOBGUI_IS_COLUMN_VIEW_CELL (self), BOBGUI_INVALID_LIST_POSITION);

  if (self->cell == NULL)
    return BOBGUI_INVALID_LIST_POSITION;

  return bobgui_list_item_base_get_position (BOBGUI_LIST_ITEM_BASE (self->cell));
}

/**
 * bobgui_column_view_cell_get_selected:
 * @self: a `BobguiColumnViewCell`
 *
 * Checks if the item is displayed as selected.
 *
 * The selected state is maintained by the list widget and its model
 * and cannot be set otherwise.
 *
 * Returns: %TRUE if the item is selected.
 *
 * Since: 4.12
 */
gboolean
bobgui_column_view_cell_get_selected (BobguiColumnViewCell *self)
{
  g_return_val_if_fail (BOBGUI_IS_COLUMN_VIEW_CELL (self), FALSE);

  if (self->cell == NULL)
    return FALSE;

  return bobgui_list_item_base_get_selected (BOBGUI_LIST_ITEM_BASE (self->cell));
}

/**
 * bobgui_column_view_cell_get_focusable:
 * @self: a `BobguiColumnViewCell`
 *
 * Checks if a list item has been set to be focusable via
 * bobgui_column_view_cell_set_focusable().
 *
 * Returns: %TRUE if the item is focusable
 *
 * Since: 4.12
 */
gboolean
bobgui_column_view_cell_get_focusable (BobguiColumnViewCell *self)
{
  g_return_val_if_fail (BOBGUI_IS_COLUMN_VIEW_CELL (self), FALSE);

  return self->focusable;
}

/**
 * bobgui_column_view_cell_set_focusable:
 * @self: a `BobguiColumnViewCell`
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
 * By default, list items are focusable.
 *
 * Since: 4.12
 */
void
bobgui_column_view_cell_set_focusable (BobguiColumnViewCell *self,
                                    gboolean     focusable)
{
  g_return_if_fail (BOBGUI_IS_COLUMN_VIEW_CELL (self));

  if (self->focusable == focusable)
    return;

  self->focusable = focusable;

  if (self->cell)
    bobgui_widget_set_focusable (BOBGUI_WIDGET (self->cell), focusable);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_FOCUSABLE]);
}
