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

#include "bobguilistheaderwidgetprivate.h"

#include "bobguibinlayout.h"
#include "bobguilistheaderprivate.h"
#include "bobguilistitemfactoryprivate.h"
#include "bobguilistbaseprivate.h"
#include "bobguiwidget.h"

typedef struct _BobguiListHeaderWidgetPrivate BobguiListHeaderWidgetPrivate;
struct _BobguiListHeaderWidgetPrivate
{
  BobguiListItemFactory *factory;

  BobguiListHeader *header;
};

enum {
  PROP_0,
  PROP_FACTORY,

  N_PROPS
};

G_DEFINE_TYPE_WITH_PRIVATE (BobguiListHeaderWidget, bobgui_list_header_widget, BOBGUI_TYPE_LIST_HEADER_BASE)

static GParamSpec *properties[N_PROPS] = { NULL, };

static void
bobgui_list_header_widget_setup_func (gpointer object,
                                   gpointer data)
{
  BobguiListHeaderWidget *self = BOBGUI_LIST_HEADER_WIDGET (data);
  BobguiListHeaderWidgetPrivate *priv = bobgui_list_header_widget_get_instance_private (self);
  BobguiListHeader *header = object;

  priv->header = header;
  header->owner = self;

  bobgui_list_header_widget_set_child (self, header->child);

  bobgui_list_header_do_notify (header,
                             bobgui_list_header_base_get_item (BOBGUI_LIST_HEADER_BASE (self)) != NULL,
                             bobgui_list_header_base_get_start (BOBGUI_LIST_HEADER_BASE (self)) != BOBGUI_INVALID_LIST_POSITION,
                             bobgui_list_header_base_get_end (BOBGUI_LIST_HEADER_BASE (self)) != BOBGUI_INVALID_LIST_POSITION,
                             bobgui_list_header_base_get_start (BOBGUI_LIST_HEADER_BASE (self)) != bobgui_list_header_base_get_end (BOBGUI_LIST_HEADER_BASE (self)));
}

static void
bobgui_list_header_widget_setup_factory (BobguiListHeaderWidget *self)
{
  BobguiListHeaderWidgetPrivate *priv = bobgui_list_header_widget_get_instance_private (self);
  BobguiListHeader *header;

  header = bobgui_list_header_new ();

  bobgui_list_item_factory_setup (priv->factory,
                               G_OBJECT (header),
                               bobgui_list_header_base_get_item (BOBGUI_LIST_HEADER_BASE (self)) != NULL,
                               bobgui_list_header_widget_setup_func,
                               self);

  g_assert (priv->header == header);
}

static void
bobgui_list_header_widget_teardown_func (gpointer object,
                                      gpointer data)
{
  BobguiListHeaderWidget *self = BOBGUI_LIST_HEADER_WIDGET (data);
  BobguiListHeaderWidgetPrivate *priv = bobgui_list_header_widget_get_instance_private (self);
  BobguiListHeader *header = object;

  header->owner = NULL;
  priv->header = NULL;

  bobgui_list_header_widget_set_child (self, NULL);

  bobgui_list_header_do_notify (header,
                             bobgui_list_header_base_get_item (BOBGUI_LIST_HEADER_BASE (self)) != NULL,
                             bobgui_list_header_base_get_start (BOBGUI_LIST_HEADER_BASE (self)) != BOBGUI_INVALID_LIST_POSITION,
                             bobgui_list_header_base_get_end (BOBGUI_LIST_HEADER_BASE (self)) != BOBGUI_INVALID_LIST_POSITION,
                             bobgui_list_header_base_get_start (BOBGUI_LIST_HEADER_BASE (self)) != bobgui_list_header_base_get_end (BOBGUI_LIST_HEADER_BASE (self)));
}

static void
bobgui_list_header_widget_teardown_factory (BobguiListHeaderWidget *self)
{
  BobguiListHeaderWidgetPrivate *priv = bobgui_list_header_widget_get_instance_private (self);
  gpointer header = priv->header;

  bobgui_list_item_factory_teardown (priv->factory,
                                  header,
                                  bobgui_list_header_base_get_item (BOBGUI_LIST_HEADER_BASE (self)) != NULL,
                                  bobgui_list_header_widget_teardown_func,
                                  self);

  g_assert (priv->header == NULL);
  g_object_unref (header);
}

typedef struct {
  BobguiListHeaderWidget *widget;
  gpointer item;
  guint start;
  guint end;
} BobguiListHeaderWidgetUpdate;

static void
bobgui_list_header_widget_update_func (gpointer object,
                                    gpointer data)
{
  BobguiListHeaderWidgetUpdate *update = data;
  BobguiListHeaderWidget *self = update->widget;
  BobguiListHeaderBase *base = BOBGUI_LIST_HEADER_BASE (self);
  /* Track notify manually instead of freeze/thaw_notify for performance reasons. */
  gboolean notify_item, notify_start, notify_end, notify_n_items;

  /* FIXME: It's kinda evil to notify external objects from here... */
  notify_item = bobgui_list_header_base_get_item (base) != update->item;
  notify_start = bobgui_list_header_base_get_start (base) != update->start;
  notify_end = bobgui_list_header_base_get_end (base) != update->end;
  notify_n_items = bobgui_list_header_base_get_end (base) - bobgui_list_header_base_get_start (base) != update->end - update->start;

  BOBGUI_LIST_HEADER_BASE_CLASS (bobgui_list_header_widget_parent_class)->update (base,
                                                                            update->item,
                                                                            update->start,
                                                                            update->end);

  if (object)
    bobgui_list_header_do_notify (object, notify_item, notify_start, notify_end, notify_n_items);
}

static void
bobgui_list_header_widget_update (BobguiListHeaderBase *base,
                               gpointer           item,
                               guint              start,
                               guint              end)
{
  BobguiListHeaderWidget *self = BOBGUI_LIST_HEADER_WIDGET (base);
  BobguiListHeaderWidgetPrivate *priv = bobgui_list_header_widget_get_instance_private (self);
  BobguiListHeaderWidgetUpdate update = { self, item, start, end };

  if (priv->header)
    {
      bobgui_list_item_factory_update (priv->factory,
                                    G_OBJECT (priv->header),
                                    bobgui_list_header_base_get_item (BOBGUI_LIST_HEADER_BASE (self)) != NULL,
                                    item != NULL,
                                    bobgui_list_header_widget_update_func,
                                    &update);
    }
  else
    {
      bobgui_list_header_widget_update_func (NULL, &update);
    }
}

static void
bobgui_list_header_widget_set_property (GObject      *object,
                                     guint         property_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  BobguiListHeaderWidget *self = BOBGUI_LIST_HEADER_WIDGET (object);

  switch (property_id)
    {
    case PROP_FACTORY:
      bobgui_list_header_widget_set_factory (self, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
bobgui_list_header_widget_clear_factory (BobguiListHeaderWidget *self)
{
  BobguiListHeaderWidgetPrivate *priv = bobgui_list_header_widget_get_instance_private (self);

  if (priv->factory == NULL)
    return;

  if (priv->header)
    bobgui_list_header_widget_teardown_factory (self);

  g_clear_object (&priv->factory);
}

static void
bobgui_list_header_widget_dispose (GObject *object)
{
  BobguiListHeaderWidget *self = BOBGUI_LIST_HEADER_WIDGET (object);

  bobgui_list_header_widget_clear_factory (self);

  G_OBJECT_CLASS (bobgui_list_header_widget_parent_class)->dispose (object);
}

static void
bobgui_list_header_widget_class_init (BobguiListHeaderWidgetClass *klass)
{
  BobguiListHeaderBaseClass *base_class = BOBGUI_LIST_HEADER_BASE_CLASS (klass);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  base_class->update = bobgui_list_header_widget_update;

  gobject_class->set_property = bobgui_list_header_widget_set_property;
  gobject_class->dispose = bobgui_list_header_widget_dispose;

  properties[PROP_FACTORY] =
    g_param_spec_object ("factory", NULL, NULL,
                         BOBGUI_TYPE_LIST_ITEM_FACTORY,
                         G_PARAM_WRITABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (gobject_class, N_PROPS, properties);

  bobgui_widget_class_set_css_name (widget_class, I_("header"));
  bobgui_widget_class_set_accessible_role (widget_class, BOBGUI_ACCESSIBLE_ROLE_ROW_HEADER);
  bobgui_widget_class_set_layout_manager_type (widget_class, BOBGUI_TYPE_BIN_LAYOUT);
}

static void
bobgui_list_header_widget_init (BobguiListHeaderWidget *self)
{
}

void
bobgui_list_header_widget_set_factory (BobguiListHeaderWidget *self,
                                    BobguiListItemFactory  *factory)
{
  BobguiListHeaderWidgetPrivate *priv = bobgui_list_header_widget_get_instance_private (self);

  if (priv->factory == factory)
    return;

  bobgui_list_header_widget_clear_factory (self);

  if (factory)
    {
      priv->factory = g_object_ref (factory);

      bobgui_list_header_widget_setup_factory (self);
    }

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_FACTORY]);
}

BobguiWidget *
bobgui_list_header_widget_new (BobguiListItemFactory *factory)
{
  return g_object_new (BOBGUI_TYPE_LIST_HEADER_WIDGET,
                       "factory", factory,
                       NULL);
}

void
bobgui_list_header_widget_set_child (BobguiListHeaderWidget *self,
                                  BobguiWidget           *child)
{
  BobguiWidget *cur_child = bobgui_widget_get_first_child (BOBGUI_WIDGET (self));

  if (cur_child == child)
    return;

  g_clear_pointer (&cur_child, bobgui_widget_unparent);

  if (child)
    bobgui_widget_set_parent (child, BOBGUI_WIDGET (self));
}

