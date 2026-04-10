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

#pragma once

#include "bobguilistfactorywidgetprivate.h"

G_BEGIN_DECLS

#define BOBGUI_TYPE_LIST_ITEM_WIDGET         (bobgui_list_item_widget_get_type ())
#define BOBGUI_LIST_ITEM_WIDGET(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), BOBGUI_TYPE_LIST_ITEM_WIDGET, BobguiListItemWidget))
#define BOBGUI_LIST_ITEM_WIDGET_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), BOBGUI_TYPE_LIST_ITEM_WIDGET, BobguiListItemWidgetClass))
#define BOBGUI_IS_LIST_ITEM_WIDGET(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), BOBGUI_TYPE_LIST_ITEM_WIDGET))
#define BOBGUI_IS_LIST_ITEM_WIDGET_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), BOBGUI_TYPE_LIST_ITEM_WIDGET))
#define BOBGUI_LIST_ITEM_WIDGET_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), BOBGUI_TYPE_LIST_ITEM_WIDGET, BobguiListItemWidgetClass))

typedef struct _BobguiListItemWidget BobguiListItemWidget;
typedef struct _BobguiListItemWidgetClass BobguiListItemWidgetClass;

struct _BobguiListItemWidget
{
  BobguiListFactoryWidget parent_instance;
};

struct _BobguiListItemWidgetClass
{
  BobguiListFactoryWidgetClass parent_class;
};

GType                   bobgui_list_item_widget_get_type           (void) G_GNUC_CONST;

BobguiWidget *             bobgui_list_item_widget_new                (BobguiListItemFactory     *factory,
                                                                 const char             *css_name,
                                                                 BobguiAccessibleRole       role);

void                    bobgui_list_item_widget_set_child          (BobguiListItemWidget      *self,
                                                                 BobguiWidget              *child);

G_END_DECLS

