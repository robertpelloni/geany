/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2010 Red Hat, Inc.
 * Author: Matthias Clasen
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <bobgui/bobguibutton.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_LOCK_BUTTON         (bobgui_lock_button_get_type ())
#define BOBGUI_LOCK_BUTTON(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), BOBGUI_TYPE_LOCK_BUTTON, BobguiLockButton))
#define BOBGUI_IS_LOCK_BUTTON(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), BOBGUI_TYPE_LOCK_BUTTON))

typedef struct _BobguiLockButton        BobguiLockButton;

GDK_AVAILABLE_IN_ALL
GType        bobgui_lock_button_get_type       (void) G_GNUC_CONST;
GDK_DEPRECATED_IN_4_10
BobguiWidget   *bobgui_lock_button_new            (GPermission   *permission);
GDK_DEPRECATED_IN_4_10
GPermission *bobgui_lock_button_get_permission (BobguiLockButton *button);
GDK_DEPRECATED_IN_4_10
void         bobgui_lock_button_set_permission (BobguiLockButton *button,
                                             GPermission   *permission);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiLockButton, g_object_unref)

G_END_DECLS

