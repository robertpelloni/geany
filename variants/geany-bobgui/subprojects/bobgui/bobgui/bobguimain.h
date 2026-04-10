/* BOBGUI - The Bobgui Framework
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
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

/*
 * Modified by the BOBGUI Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the BOBGUI Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BOBGUI at ftp://ftp.bobgui.org/pub/bobgui/.
 */

#pragma once


#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <gdk/gdk.h>
#include <bobgui/bobguiwidget.h>
#ifdef G_PLATFORM_WIN32
#include <bobgui/bobguibox.h>
#include <bobgui/bobguiwindow.h>
#endif

G_BEGIN_DECLS

/**
 * BOBGUI_PRIORITY_RESIZE: (value 110)
 *
 * Use this priority for functionality related to size allocation.
 *
 * It is used internally by BOBGUI to compute the sizes of widgets.
 * This priority is higher than %GDK_PRIORITY_REDRAW to avoid
 * resizing a widget which was just redrawn.
 */
#define BOBGUI_PRIORITY_RESIZE (G_PRIORITY_HIGH_IDLE + 10)

/* Initialization, exit, mainloop and miscellaneous routines
 */

GDK_AVAILABLE_IN_ALL
void     bobgui_init                 (void);

GDK_AVAILABLE_IN_ALL
gboolean bobgui_init_check           (void);

GDK_AVAILABLE_IN_ALL
gboolean bobgui_is_initialized       (void);

#ifdef G_OS_WIN32

/* Variants that are used to check for correct struct packing
 * when building BOBGUI-using code.
 */
GDK_AVAILABLE_IN_ALL
void     bobgui_init_abi_check       (int     num_checks,
                                   size_t  sizeof_BobguiWindow,
                                   size_t  sizeof_BobguiBox);
GDK_AVAILABLE_IN_ALL
gboolean bobgui_init_check_abi_check (int     num_checks,
                                   size_t  sizeof_BobguiWindow,
                                   size_t  sizeof_BobguiBox);

#define bobgui_init() bobgui_init_abi_check (2, sizeof (BobguiWindow), sizeof (BobguiBox))
#define bobgui_init_check() bobgui_init_check_abi_check (2, sizeof (BobguiWindow), sizeof (BobguiBox))

#endif

GDK_AVAILABLE_IN_ALL
void              bobgui_disable_setlocale    (void);

GDK_AVAILABLE_IN_4_18
void              bobgui_disable_portals      (void);

GDK_AVAILABLE_IN_4_22
void              bobgui_disable_portal_interfaces (const char **portal_interfaces);

GDK_AVAILABLE_IN_ALL
PangoLanguage *   bobgui_get_default_language (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
BobguiTextDirection  bobgui_get_locale_direction (void);


G_END_DECLS

