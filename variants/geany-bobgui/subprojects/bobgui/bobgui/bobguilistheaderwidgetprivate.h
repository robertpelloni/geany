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

#pragma once

#include "bobguilistheaderbaseprivate.h"

#include "bobguilistitemfactory.h"

G_BEGIN_DECLS

#define BOBGUI_TYPE_LIST_HEADER_WIDGET         (bobgui_list_header_widget_get_type ())
#define BOBGUI_LIST_HEADER_WIDGET(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), BOBGUI_TYPE_LIST_HEADER_WIDGET, BobguiListHeaderWidget))
#define BOBGUI_LIST_HEADER_WIDGET_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), BOBGUI_TYPE_LIST_HEADER_WIDGET, BobguiListHeaderWidgetClass))
#define BOBGUI_IS_LIST_HEADER_WIDGET(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), BOBGUI_TYPE_LIST_HEADER_WIDGET))
#define BOBGUI_IS_LIST_HEADER_WIDGET_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), BOBGUI_TYPE_LIST_HEADER_WIDGET))
#define BOBGUI_LIST_HEADER_WIDGET_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), BOBGUI_TYPE_LIST_HEADER_WIDGET, BobguiListHeaderWidgetClass))

typedef struct _BobguiListHeaderWidget BobguiListHeaderWidget;
typedef struct _BobguiListHeaderWidgetClass BobguiListHeaderWidgetClass;

struct _BobguiListHeaderWidget
{
  BobguiListHeaderBase parent_instance;
};

struct _BobguiListHeaderWidgetClass
{
  BobguiListHeaderBaseClass parent_class;
};

GType                   bobgui_list_header_widget_get_type         (void) G_GNUC_CONST;

BobguiWidget *             bobgui_list_header_widget_new              (BobguiListItemFactory     *factory);

void                    bobgui_list_header_widget_set_factory      (BobguiListHeaderWidget    *self,
                                                                 BobguiListItemFactory     *factory);
BobguiListItemFactory *    bobgui_list_header_widget_get_factory      (BobguiListHeaderWidget    *self);

void                    bobgui_list_header_widget_set_child        (BobguiListHeaderWidget    *self,
                                                                 BobguiWidget              *child);


G_END_DECLS

