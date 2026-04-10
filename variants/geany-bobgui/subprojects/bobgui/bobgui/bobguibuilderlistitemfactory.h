/*
 * Copyright © 2019 Benjamin Otte
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

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <bobgui/bobguilistitemfactory.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_BUILDER_LIST_ITEM_FACTORY         (bobgui_builder_list_item_factory_get_type ())
#define BOBGUI_BUILDER_LIST_ITEM_FACTORY(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), BOBGUI_TYPE_BUILDER_LIST_ITEM_FACTORY, BobguiBuilderListItemFactory))
#define BOBGUI_BUILDER_LIST_ITEM_FACTORY_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), BOBGUI_TYPE_BUILDER_LIST_ITEM_FACTORY, BobguiBuilderListItemFactoryClass))
#define BOBGUI_IS_BUILDER_LIST_ITEM_FACTORY(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), BOBGUI_TYPE_BUILDER_LIST_ITEM_FACTORY))
#define BOBGUI_IS_BUILDER_LIST_ITEM_FACTORY_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), BOBGUI_TYPE_BUILDER_LIST_ITEM_FACTORY))
#define BOBGUI_BUILDER_LIST_ITEM_FACTORY_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), BOBGUI_TYPE_BUILDER_LIST_ITEM_FACTORY, BobguiBuilderListItemFactoryClass))

typedef struct _BobguiBuilderListItemFactory BobguiBuilderListItemFactory;
typedef struct _BobguiBuilderListItemFactoryClass BobguiBuilderListItemFactoryClass;

GDK_AVAILABLE_IN_ALL
GType                   bobgui_builder_list_item_factory_get_type          (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
BobguiListItemFactory *    bobgui_builder_list_item_factory_new_from_bytes    (BobguiBuilderScope                *scope,
                                                                         GBytes                         *bytes);
GDK_AVAILABLE_IN_ALL
BobguiListItemFactory *    bobgui_builder_list_item_factory_new_from_resource (BobguiBuilderScope                *scope,
                                                                         const char                     *resource_path);

GDK_AVAILABLE_IN_ALL
GBytes *                bobgui_builder_list_item_factory_get_bytes         (BobguiBuilderListItemFactory      *self) G_GNUC_PURE;
GDK_AVAILABLE_IN_ALL
const char *            bobgui_builder_list_item_factory_get_resource      (BobguiBuilderListItemFactory      *self) G_GNUC_PURE;
GDK_AVAILABLE_IN_ALL
BobguiBuilderScope *       bobgui_builder_list_item_factory_get_scope         (BobguiBuilderListItemFactory      *self) G_GNUC_PURE;

G_END_DECLS

