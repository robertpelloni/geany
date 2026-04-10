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

typedef struct _BobguiListItemFactoryClass BobguiListItemFactoryClass;

#include <gdk/gdk.h>
#include <bobgui/bobguitypes.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_LIST_ITEM_FACTORY         (bobgui_list_item_factory_get_type ())
#define BOBGUI_LIST_ITEM_FACTORY(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), BOBGUI_TYPE_LIST_ITEM_FACTORY, BobguiListItemFactory))
#define BOBGUI_LIST_ITEM_FACTORY_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), BOBGUI_TYPE_LIST_ITEM_FACTORY, BobguiListItemFactoryClass))
#define BOBGUI_IS_LIST_ITEM_FACTORY(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), BOBGUI_TYPE_LIST_ITEM_FACTORY))
#define BOBGUI_IS_LIST_ITEM_FACTORY_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), BOBGUI_TYPE_LIST_ITEM_FACTORY))
#define BOBGUI_LIST_ITEM_FACTORY_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), BOBGUI_TYPE_LIST_ITEM_FACTORY, BobguiListItemFactoryClass))


GDK_AVAILABLE_IN_ALL
GType        bobgui_list_item_factory_get_type       (void) G_GNUC_CONST;

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiListItemFactory, g_object_unref)

G_END_DECLS

