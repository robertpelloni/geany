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

#include "bobguilistheaderprivate.h"

/**
 * BobguiListHeader:
 *
 * Used by list widgets to represent the headers they display.
 *
 * `BobguiListHeader` objects are managed just like [class@Bobgui.ListItem]
 * objects via their factory, but provide a different set of properties suitable
 * for managing the header instead of individual items.
 *
 * Since: 4.12
 */

enum
{
  PROP_0,
  PROP_CHILD,
  PROP_END,
  PROP_ITEM,
  PROP_N_ITEMS,
  PROP_START,

  N_PROPS
};

G_DEFINE_TYPE (BobguiListHeader, bobgui_list_header, G_TYPE_OBJECT)

static GParamSpec *properties[N_PROPS] = { NULL, };

static void
bobgui_list_header_dispose (GObject *object)
{
  BobguiListHeader *self = BOBGUI_LIST_HEADER (object);

  g_assert (self->owner == NULL); /* would hold a reference */
  g_clear_object (&self->child);

  G_OBJECT_CLASS (bobgui_list_header_parent_class)->dispose (object);
}

static void
bobgui_list_header_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  BobguiListHeader *self = BOBGUI_LIST_HEADER (object);

  switch (property_id)
    {
    case PROP_CHILD:
      g_value_set_object (value, self->child);
      break;

    case PROP_END:
      if (self->owner)
        g_value_set_uint (value, bobgui_list_header_base_get_end (BOBGUI_LIST_HEADER_BASE (self->owner)));
      else
        g_value_set_uint (value, BOBGUI_INVALID_LIST_POSITION);
      break;

    case PROP_ITEM:
      if (self->owner)
        g_value_set_object (value, bobgui_list_header_base_get_item (BOBGUI_LIST_HEADER_BASE (self->owner)));
      break;

    case PROP_N_ITEMS:
      g_value_set_uint (value, bobgui_list_header_get_n_items (self));
      break;

    case PROP_START:
      if (self->owner)
        g_value_set_uint (value, bobgui_list_header_base_get_start (BOBGUI_LIST_HEADER_BASE (self->owner)));
      else
        g_value_set_uint (value, BOBGUI_INVALID_LIST_POSITION);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
bobgui_list_header_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BobguiListHeader *self = BOBGUI_LIST_HEADER (object);

  switch (property_id)
    {
    case PROP_CHILD:
      bobgui_list_header_set_child (self, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
bobgui_list_header_class_init (BobguiListHeaderClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->dispose = bobgui_list_header_dispose;
  gobject_class->get_property = bobgui_list_header_get_property;
  gobject_class->set_property = bobgui_list_header_set_property;

  /**
   * BobguiListHeader:child:
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
   * BobguiListHeader:end:
   *
   * The first position no longer part of this section.
   *
   * Since: 4.12
   */
  properties[PROP_END] =
    g_param_spec_uint ("end", NULL, NULL,
                       0, G_MAXUINT, BOBGUI_INVALID_LIST_POSITION,
                       G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * BobguiListHeader:item:
   *
   * The item at the start of the section.
   *
   * Since: 4.12
   */
  properties[PROP_ITEM] =
    g_param_spec_object ("item", NULL, NULL,
                         G_TYPE_OBJECT,
                         G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * BobguiListHeader:n-items:
   *
   * Number of items in this section.
   *
   * Since: 4.12
   */
  properties[PROP_N_ITEMS] =
    g_param_spec_uint ("n-items", NULL, NULL,
                       0, G_MAXUINT, 0,
                       G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * BobguiListHeader:start:
   *
   * First position of items in this section.
   *
   * Since: 4.12
   */
  properties[PROP_START] =
    g_param_spec_uint ("start", NULL, NULL,
                       0, G_MAXUINT, BOBGUI_INVALID_LIST_POSITION,
                       G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (gobject_class, N_PROPS, properties);
}

static void
bobgui_list_header_init (BobguiListHeader *self)
{
}

BobguiListHeader *
bobgui_list_header_new (void)
{
  return g_object_new (BOBGUI_TYPE_LIST_HEADER, NULL);
}

void
bobgui_list_header_do_notify (BobguiListHeader *list_header,
                           gboolean notify_item,
                           gboolean notify_start,
                           gboolean notify_end,
                           gboolean notify_n_items)
{
  GObject *object = G_OBJECT (list_header);

  if (notify_item)
    g_object_notify_by_pspec (object, properties[PROP_ITEM]);
  if (notify_start)
    g_object_notify_by_pspec (object, properties[PROP_START]);
  if (notify_end)
    g_object_notify_by_pspec (object, properties[PROP_END]);
  if (notify_n_items)
    g_object_notify_by_pspec (object, properties[PROP_N_ITEMS]);
}

/**
 * bobgui_list_header_get_item:
 * @self: a `BobguiListHeader`
 *
 * Gets the model item at the start of the section.
 * This is the item that occupies the list model at position
 * [property@Bobgui.ListHeader:start].
 *
 * If @self is unbound, this function returns %NULL.
 *
 * Returns: (nullable) (transfer none) (type GObject): The item displayed
 *
 * Since: 4.12
 **/
gpointer
bobgui_list_header_get_item (BobguiListHeader *self)
{
  g_return_val_if_fail (BOBGUI_IS_LIST_HEADER (self), NULL);

  if (self->owner)
    return bobgui_list_header_base_get_item (BOBGUI_LIST_HEADER_BASE (self->owner));
  else
    return NULL;
}

/**
 * bobgui_list_header_get_child:
 * @self: a `BobguiListHeader`
 *
 * Gets the child previously set via bobgui_list_header_set_child() or
 * %NULL if none was set.
 *
 * Returns: (transfer none) (nullable): The child
 *
 * Since: 4.12
 */
BobguiWidget *
bobgui_list_header_get_child (BobguiListHeader *self)
{
  g_return_val_if_fail (BOBGUI_IS_LIST_HEADER (self), NULL);

  return self->child;
}

/**
 * bobgui_list_header_set_child:
 * @self: a `BobguiListHeader`
 * @child: (nullable): The list item's child or %NULL to unset
 *
 * Sets the child to be used for this listitem.
 *
 * This function is typically called by applications when
 * setting up a header so that the widget can be reused when
 * binding it multiple times.
 *
 * Since: 4.12
 */
void
bobgui_list_header_set_child (BobguiListHeader *self,
                           BobguiWidget   *child)
{
  g_return_if_fail (BOBGUI_IS_LIST_HEADER (self));
  g_return_if_fail (child == NULL || bobgui_widget_get_parent (child) == NULL);

  if (self->child == child)
    return;

  g_clear_object (&self->child);

  if (child)
    {
      g_object_ref_sink (child);
      self->child = child;
    }

  if (self->owner)
    bobgui_list_header_widget_set_child (self->owner, child);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_CHILD]);
}

/**
 * bobgui_list_header_get_start:
 * @self: a `BobguiListHeader`
 *
 * Gets the start position in the model of the section that @self is
 * currently the header for.
 *
 * If @self is unbound, %BOBGUI_INVALID_LIST_POSITION is returned.
 *
 * Returns: The start position of the section
 *
 * Since: 4.12
 */
guint
bobgui_list_header_get_start (BobguiListHeader *self)
{
  g_return_val_if_fail (BOBGUI_IS_LIST_HEADER (self), BOBGUI_INVALID_LIST_POSITION);

  if (self->owner)
    return bobgui_list_header_base_get_start (BOBGUI_LIST_HEADER_BASE (self->owner));
  else
    return BOBGUI_INVALID_LIST_POSITION;
}

/**
 * bobgui_list_header_get_end:
 * @self: a `BobguiListHeader`
 *
 * Gets the end position in the model of the section that @self is
 * currently the header for.
 *
 * If @self is unbound, %BOBGUI_INVALID_LIST_POSITION is returned.
 *
 * Returns: The end position of the section
 *
 * Since: 4.12
 */
guint
bobgui_list_header_get_end (BobguiListHeader *self)
{
  g_return_val_if_fail (BOBGUI_IS_LIST_HEADER (self), BOBGUI_INVALID_LIST_POSITION);

  if (self->owner)
    return bobgui_list_header_base_get_end (BOBGUI_LIST_HEADER_BASE (self->owner));
  else
    return BOBGUI_INVALID_LIST_POSITION;
}

/**
 * bobgui_list_header_get_n_items:
 * @self: a `BobguiListHeader`
 *
 * Gets the the number of items in the section.
 *
 * If @self is unbound, 0 is returned.
 *
 * Returns: The number of items in the section
 *
 * Since: 4.12
 */
guint
bobgui_list_header_get_n_items (BobguiListHeader *self)
{
  g_return_val_if_fail (BOBGUI_IS_LIST_HEADER (self), BOBGUI_INVALID_LIST_POSITION);

  if (self->owner)
    return bobgui_list_header_base_get_end (BOBGUI_LIST_HEADER_BASE (self->owner)) -
           bobgui_list_header_base_get_start (BOBGUI_LIST_HEADER_BASE (self->owner));
  else
    return 0;
}

