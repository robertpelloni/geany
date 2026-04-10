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

typedef struct _BobguiDropTargetAsync BobguiDropTargetAsync;
typedef struct _BobguiDropTargetAsyncClass BobguiDropTargetAsyncClass;


#define BOBGUI_TYPE_DROP_TARGET_ASYNC         (bobgui_drop_target_async_get_type ())
#define BOBGUI_DROP_TARGET_ASYNC(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), BOBGUI_TYPE_DROP_TARGET_ASYNC, BobguiDropTargetAsync))
#define BOBGUI_DROP_TARGET_ASYNC_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), BOBGUI_TYPE_DROP_TARGET_ASYNC, BobguiDropTargetAsyncClass))
#define BOBGUI_IS_DROP_TARGET_ASYNC(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), BOBGUI_TYPE_DROP_TARGET_ASYNC))
#define BOBGUI_IS_DROP_TARGET_ASYNC_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), BOBGUI_TYPE_DROP_TARGET_ASYNC))
#define BOBGUI_DROP_TARGET_ASYNC_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), BOBGUI_TYPE_DROP_TARGET_ASYNC, BobguiDropTargetAsyncClass))


GDK_AVAILABLE_IN_ALL
GType                   bobgui_drop_target_async_get_type          (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
BobguiDropTargetAsync *    bobgui_drop_target_async_new               (GdkContentFormats      *formats,
                                                                 GdkDragAction           actions);

GDK_AVAILABLE_IN_ALL
void                    bobgui_drop_target_async_set_formats       (BobguiDropTargetAsync     *self,
                                                                 GdkContentFormats      *formats);
GDK_AVAILABLE_IN_ALL
GdkContentFormats *     bobgui_drop_target_async_get_formats       (BobguiDropTargetAsync     *self);

GDK_AVAILABLE_IN_ALL
void                    bobgui_drop_target_async_set_actions       (BobguiDropTargetAsync     *self,
                                                                 GdkDragAction           actions);
GDK_AVAILABLE_IN_ALL
GdkDragAction           bobgui_drop_target_async_get_actions       (BobguiDropTargetAsync     *self);

GDK_AVAILABLE_IN_ALL
void                    bobgui_drop_target_async_reject_drop       (BobguiDropTargetAsync     *self,
                                                                 GdkDrop                *drop);


G_END_DECLS

