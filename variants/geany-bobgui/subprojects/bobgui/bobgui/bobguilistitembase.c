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

#include "bobguilistitembaseprivate.h"

#include "bobguiaccessible.h"

typedef struct _BobguiListItemBasePrivate BobguiListItemBasePrivate;
struct _BobguiListItemBasePrivate
{
  GObject *item;
  guint position;
  gboolean selected;
};

G_DEFINE_TYPE_WITH_PRIVATE (BobguiListItemBase, bobgui_list_item_base, BOBGUI_TYPE_WIDGET)

static void
bobgui_list_item_base_default_update (BobguiListItemBase *self,
                                   guint            position,
                                   gpointer         item,
                                   gboolean         selected)
{
  BobguiListItemBasePrivate *priv = bobgui_list_item_base_get_instance_private (self);

  g_set_object (&priv->item, item);
  priv->position = position;
  priv->selected = selected;
}

static void
bobgui_list_item_base_dispose (GObject *object)
{
  BobguiListItemBase *self = BOBGUI_LIST_ITEM_BASE (object);
  BobguiListItemBasePrivate *priv = bobgui_list_item_base_get_instance_private (self);

  g_clear_object (&priv->item);

  G_OBJECT_CLASS (bobgui_list_item_base_parent_class)->dispose (object);
}

static void
bobgui_list_item_base_class_init (BobguiListItemBaseClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  klass->update = bobgui_list_item_base_default_update;

  gobject_class->dispose = bobgui_list_item_base_dispose;
}

static void
bobgui_list_item_base_init (BobguiListItemBase *self)
{
}

void
bobgui_list_item_base_update (BobguiListItemBase *self,
                           guint            position,
                           gpointer         item,
                           gboolean         selected)
{
  BobguiListItemBasePrivate *priv = bobgui_list_item_base_get_instance_private (self);
  gboolean was_selected;

  if (priv->position == position &&
      priv->item == item &&
      priv->selected == selected)
    return;

  was_selected = priv->selected;

  BOBGUI_LIST_ITEM_BASE_GET_CLASS (self)->update (self, position, item, selected);

  /* don't look at selected variable, it's not reentrancy safe */
  if (was_selected != priv->selected)
    {
      if (priv->selected)
        bobgui_widget_set_state_flags (BOBGUI_WIDGET (self), BOBGUI_STATE_FLAG_SELECTED, FALSE);
      else
        bobgui_widget_unset_state_flags (BOBGUI_WIDGET (self), BOBGUI_STATE_FLAG_SELECTED);
    }

  /* We're updating the a11y state at least once because of the side-effects, subsequent same state filtering is done on the a11y layer. */
  bobgui_accessible_update_state (BOBGUI_ACCESSIBLE (self),
                               BOBGUI_ACCESSIBLE_STATE_SELECTED, priv->selected,
                               -1);
}

guint
bobgui_list_item_base_get_position (BobguiListItemBase *self)
{
  BobguiListItemBasePrivate *priv = bobgui_list_item_base_get_instance_private (self);

  return priv->position;
}

gpointer
bobgui_list_item_base_get_item (BobguiListItemBase *self)
{
  BobguiListItemBasePrivate *priv = bobgui_list_item_base_get_instance_private (self);

  return priv->item;
}

gboolean
bobgui_list_item_base_get_selected (BobguiListItemBase *self)
{
  BobguiListItemBasePrivate *priv = bobgui_list_item_base_get_instance_private (self);

  return priv->selected;
}

