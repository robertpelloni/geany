/* -*- Mode: C; c-file-style: "gnu"; tab-width: 8 -*- */
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

#define BOBGUI_TYPE_DRAG_SOURCE         (bobgui_drag_source_get_type ())
#define BOBGUI_DRAG_SOURCE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), BOBGUI_TYPE_DRAG_SOURCE, BobguiDragSource))
#define BOBGUI_DRAG_SOURCE_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), BOBGUI_TYPE_DRAG_SOURCE, BobguiDragSourceClass))
#define BOBGUI_IS_DRAG_SOURCE(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), BOBGUI_TYPE_DRAG_SOURCE))
#define BOBGUI_IS_DRAG_SOURCE_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), BOBGUI_TYPE_DRAG_SOURCE))
#define BOBGUI_DRAG_SOURCE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), BOBGUI_TYPE_DRAG_SOURCE, BobguiDragSourceClass))

typedef struct _BobguiDragSource BobguiDragSource;
typedef struct _BobguiDragSourceClass BobguiDragSourceClass;

GDK_AVAILABLE_IN_ALL
GType              bobgui_drag_source_get_type  (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
BobguiDragSource      *bobgui_drag_source_new        (void);

GDK_AVAILABLE_IN_ALL
void                bobgui_drag_source_set_content (BobguiDragSource     *source,
                                                 GdkContentProvider *content);
GDK_AVAILABLE_IN_ALL
GdkContentProvider *bobgui_drag_source_get_content (BobguiDragSource     *source);

GDK_AVAILABLE_IN_ALL
void               bobgui_drag_source_set_actions (BobguiDragSource     *source,
                                                GdkDragAction      actions);
GDK_AVAILABLE_IN_ALL
GdkDragAction      bobgui_drag_source_get_actions (BobguiDragSource     *source);

GDK_AVAILABLE_IN_ALL
void               bobgui_drag_source_set_icon    (BobguiDragSource     *source,
                                                GdkPaintable      *paintable,
                                                int                hot_x,
                                                int                hot_y);
GDK_AVAILABLE_IN_ALL
void               bobgui_drag_source_drag_cancel (BobguiDragSource     *source);

GDK_AVAILABLE_IN_ALL
GdkDrag *          bobgui_drag_source_get_drag    (BobguiDragSource     *source);

GDK_AVAILABLE_IN_ALL
gboolean           bobgui_drag_check_threshold    (BobguiWidget         *widget,
                                                int                start_x,
                                                int                start_y,
                                                int                current_x,
                                                int                current_y);


G_END_DECLS

