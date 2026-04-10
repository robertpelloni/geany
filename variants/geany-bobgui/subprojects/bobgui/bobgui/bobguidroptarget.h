/*
 * Copyright © 2020 Benjamin Otte
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Benjamin Otte <otte@gnome.org>
 */

#pragma once


#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <bobgui/bobguitypes.h>


G_BEGIN_DECLS

typedef struct _BobguiDropTarget BobguiDropTarget;


#define BOBGUI_TYPE_DROP_TARGET         (bobgui_drop_target_get_type ())
#define BOBGUI_DROP_TARGET(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), BOBGUI_TYPE_DROP_TARGET, BobguiDropTarget))
#define BOBGUI_DROP_TARGET_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), BOBGUI_TYPE_DROP_TARGET, BobguiDropTargetClass))
#define BOBGUI_IS_DROP_TARGET(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), BOBGUI_TYPE_DROP_TARGET))
#define BOBGUI_IS_DROP_TARGET_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), BOBGUI_TYPE_DROP_TARGET))
#define BOBGUI_DROP_TARGET_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), BOBGUI_TYPE_DROP_TARGET, BobguiDropTargetClass))

typedef struct _BobguiDropTargetClass BobguiDropTargetClass;

GDK_AVAILABLE_IN_ALL
GType                   bobgui_drop_target_get_type         (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
BobguiDropTarget *         bobgui_drop_target_new              (GType                  type,
                                                          GdkDragAction          actions);

GDK_AVAILABLE_IN_ALL
void                    bobgui_drop_target_set_gtypes       (BobguiDropTarget         *self,
                                                          GType                 *types,
                                                          gsize                  n_types);
GDK_AVAILABLE_IN_ALL
const GType *           bobgui_drop_target_get_gtypes       (BobguiDropTarget         *self,
                                                          gsize                 *n_types);
GDK_AVAILABLE_IN_ALL
GdkContentFormats *     bobgui_drop_target_get_formats      (BobguiDropTarget         *self);

GDK_AVAILABLE_IN_ALL
void                    bobgui_drop_target_set_actions      (BobguiDropTarget         *self,
                                                          GdkDragAction          actions);
GDK_AVAILABLE_IN_ALL
GdkDragAction           bobgui_drop_target_get_actions      (BobguiDropTarget         *self);

GDK_AVAILABLE_IN_ALL
void                    bobgui_drop_target_set_preload      (BobguiDropTarget         *self,
                                                          gboolean               preload);
GDK_AVAILABLE_IN_ALL
gboolean                bobgui_drop_target_get_preload      (BobguiDropTarget         *self);

GDK_DEPRECATED_IN_4_4_FOR(bobgui_drop_target_get_current_drop)
GdkDrop *               bobgui_drop_target_get_drop         (BobguiDropTarget         *self);

GDK_AVAILABLE_IN_4_4
GdkDrop *               bobgui_drop_target_get_current_drop (BobguiDropTarget         *self);

GDK_AVAILABLE_IN_ALL
const GValue *          bobgui_drop_target_get_value        (BobguiDropTarget         *self);

GDK_AVAILABLE_IN_ALL
void                    bobgui_drop_target_reject           (BobguiDropTarget         *self);


G_END_DECLS

