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

#include <bobgui/bobguiwidget.h>


G_BEGIN_DECLS

#define BOBGUI_TYPE_SCROLLBAR            (bobgui_scrollbar_get_type ())
#define BOBGUI_SCROLLBAR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_SCROLLBAR, BobguiScrollbar))
#define BOBGUI_IS_SCROLLBAR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_SCROLLBAR))


typedef struct _BobguiScrollbar        BobguiScrollbar;


GDK_AVAILABLE_IN_ALL
GType       bobgui_scrollbar_get_type (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
BobguiWidget * bobgui_scrollbar_new      (BobguiOrientation  orientation,
                                    BobguiAdjustment  *adjustment);
GDK_AVAILABLE_IN_ALL
void           bobgui_scrollbar_set_adjustment (BobguiScrollbar  *self,
                                             BobguiAdjustment *adjustment);
GDK_AVAILABLE_IN_ALL
BobguiAdjustment *bobgui_scrollbar_get_adjustment (BobguiScrollbar  *self);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiScrollbar, g_object_unref)

G_END_DECLS

