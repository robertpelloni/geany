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


#define BOBGUI_TYPE_VIEWPORT            (bobgui_viewport_get_type ())
#define BOBGUI_VIEWPORT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_VIEWPORT, BobguiViewport))
#define BOBGUI_IS_VIEWPORT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_VIEWPORT))


typedef struct _BobguiViewport              BobguiViewport;


GDK_AVAILABLE_IN_ALL
GType          bobgui_viewport_get_type        (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
BobguiWidget*     bobgui_viewport_new             (BobguiAdjustment *hadjustment,
                                             BobguiAdjustment *vadjustment);

GDK_AVAILABLE_IN_ALL
gboolean       bobgui_viewport_get_scroll_to_focus (BobguiViewport *viewport);
GDK_AVAILABLE_IN_ALL
void           bobgui_viewport_set_scroll_to_focus (BobguiViewport *viewport,
                                                 gboolean     scroll_to_focus);

GDK_AVAILABLE_IN_ALL
void           bobgui_viewport_set_child           (BobguiViewport *viewport,
                                                 BobguiWidget   *child);
GDK_AVAILABLE_IN_ALL
BobguiWidget *    bobgui_viewport_get_child           (BobguiViewport *viewport);

GDK_AVAILABLE_IN_4_12
void           bobgui_viewport_scroll_to           (BobguiViewport   *viewport,
                                                 BobguiWidget     *descendant,
                                                 BobguiScrollInfo *scroll);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiViewport, g_object_unref)

G_END_DECLS


