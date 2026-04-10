/* BOBGUI - The Bobgui Framework
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 * BobguiStatusbar Copyright (C) 1998 Shawn T. Amundson
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

#include <bobgui/bobguiwidget.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_STATUSBAR            (bobgui_statusbar_get_type ())
#define BOBGUI_STATUSBAR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_STATUSBAR, BobguiStatusbar))
#define BOBGUI_IS_STATUSBAR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_STATUSBAR))

typedef struct _BobguiStatusbar BobguiStatusbar;

GDK_AVAILABLE_IN_ALL
GType      bobgui_statusbar_get_type     	(void) G_GNUC_CONST;
GDK_DEPRECATED_IN_4_10
BobguiWidget* bobgui_statusbar_new          	(void);
GDK_DEPRECATED_IN_4_10
guint	   bobgui_statusbar_get_context_id	(BobguiStatusbar *statusbar,
					 const char   *context_description);
GDK_DEPRECATED_IN_4_10
guint      bobgui_statusbar_push          	(BobguiStatusbar *statusbar,
					 guint	       context_id,
					 const char   *text);
GDK_DEPRECATED_IN_4_10
void       bobgui_statusbar_pop          	(BobguiStatusbar *statusbar,
					 guint	       context_id);
GDK_DEPRECATED_IN_4_10
void       bobgui_statusbar_remove        	(BobguiStatusbar *statusbar,
					 guint	       context_id,
					 guint         message_id);
GDK_DEPRECATED_IN_4_10
void       bobgui_statusbar_remove_all    	(BobguiStatusbar *statusbar,
					 guint	       context_id);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiStatusbar, g_object_unref)

G_END_DECLS

