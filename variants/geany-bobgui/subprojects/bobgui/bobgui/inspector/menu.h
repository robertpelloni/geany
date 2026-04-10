/*
 * Copyright (c) 2014 Red Hat, Inc.
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

#include <bobgui/bobguibox.h>

#define BOBGUI_TYPE_INSPECTOR_MENU            (bobgui_inspector_menu_get_type())
#define BOBGUI_INSPECTOR_MENU(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), BOBGUI_TYPE_INSPECTOR_MENU, BobguiInspectorMenu))
#define BOBGUI_INSPECTOR_MENU_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), BOBGUI_TYPE_INSPECTOR_MENU, BobguiInspectorMenuClass))
#define BOBGUI_INSPECTOR_IS_MENU(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), BOBGUI_TYPE_INSPECTOR_MENU))
#define BOBGUI_INSPECTOR_IS_MENU_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), BOBGUI_TYPE_INSPECTOR_MENU))
#define BOBGUI_INSPECTOR_MENU_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), BOBGUI_TYPE_INSPECTOR_MENU, BobguiInspectorMenuClass))


typedef struct _BobguiInspectorMenuPrivate BobguiInspectorMenuPrivate;

typedef struct _BobguiInspectorMenu
{
  BobguiBox parent;
  BobguiInspectorMenuPrivate *priv;
} BobguiInspectorMenu;

typedef struct _BobguiInspectorMenuClass
{
  BobguiBoxClass parent;
} BobguiInspectorMenuClass;

G_BEGIN_DECLS

GType      bobgui_inspector_menu_get_type   (void);
void       bobgui_inspector_menu_set_object (BobguiInspectorMenu *sl,
                                          GObject          *object);

G_END_DECLS


// vim: set et sw=2 ts=2:
