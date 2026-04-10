/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2000 Red Hat, Inc.
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

#include <gdk/gdk.h>
#include <bobgui/bobguitypes.h>

G_BEGIN_DECLS


/* -- type macros --- */
#define BOBGUI_TYPE_SETTINGS             (bobgui_settings_get_type ())
#define BOBGUI_SETTINGS(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_SETTINGS, BobguiSettings))
#define BOBGUI_IS_SETTINGS(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_SETTINGS))

/* --- functions --- */
GDK_AVAILABLE_IN_ALL
GType           bobgui_settings_get_type                (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
BobguiSettings*    bobgui_settings_get_default             (void);
GDK_AVAILABLE_IN_ALL
BobguiSettings*    bobgui_settings_get_for_display         (GdkDisplay *display);

GDK_AVAILABLE_IN_ALL
void     bobgui_settings_reset_property       (BobguiSettings            *settings,
                                            const char             *name);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiSettings, g_object_unref)

G_END_DECLS

