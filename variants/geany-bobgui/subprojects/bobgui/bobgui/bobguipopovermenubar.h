/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2019 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once


#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <bobgui/bobguipopover.h>


G_BEGIN_DECLS


#define	BOBGUI_TYPE_POPOVER_MENU_BAR               (bobgui_popover_menu_bar_get_type ())
#define BOBGUI_POPOVER_MENU_BAR(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_POPOVER_MENU_BAR, BobguiPopoverMenuBar))
#define BOBGUI_IS_POPOVER_MENU_BAR(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_POPOVER_MENU_BAR))

typedef struct _BobguiPopoverMenuBar BobguiPopoverMenuBar;

GDK_AVAILABLE_IN_ALL
GType        bobgui_popover_menu_bar_get_type       (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
BobguiWidget *  bobgui_popover_menu_bar_new_from_model (GMenuModel        *model);

GDK_AVAILABLE_IN_ALL
void         bobgui_popover_menu_bar_set_menu_model (BobguiPopoverMenuBar *bar,
                                                  GMenuModel        *model);
GDK_AVAILABLE_IN_ALL
GMenuModel * bobgui_popover_menu_bar_get_menu_model (BobguiPopoverMenuBar *bar);

GDK_AVAILABLE_IN_ALL
gboolean     bobgui_popover_menu_bar_add_child      (BobguiPopoverMenuBar *bar,
                                                  BobguiWidget         *child,
                                                  const char        *id);

GDK_AVAILABLE_IN_ALL
gboolean     bobgui_popover_menu_bar_remove_child   (BobguiPopoverMenuBar *bar,
                                                  BobguiWidget         *child);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiPopoverMenuBar, g_object_unref)

G_END_DECLS


