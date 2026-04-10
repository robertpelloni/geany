/*
 * bobguioverlay.h
 * This file is part of bobgui
 *
 * Copyright (C) 2011 - Ignacio Casal Quinteiro
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

#include <bobgui/bobguiwidget.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_OVERLAY             (bobgui_overlay_get_type ())
#define BOBGUI_OVERLAY(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_OVERLAY, BobguiOverlay))
#define BOBGUI_IS_OVERLAY(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_OVERLAY))

typedef struct _BobguiOverlay         BobguiOverlay;

GDK_AVAILABLE_IN_ALL
GType      bobgui_overlay_get_type    (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
BobguiWidget *bobgui_overlay_new         (void);
GDK_AVAILABLE_IN_ALL
void       bobgui_overlay_add_overlay (BobguiOverlay *overlay,
                                    BobguiWidget  *widget);
GDK_AVAILABLE_IN_ALL
void       bobgui_overlay_remove_overlay (BobguiOverlay *overlay,
                                       BobguiWidget  *widget);

GDK_AVAILABLE_IN_ALL
void                  bobgui_overlay_set_child (BobguiOverlay *overlay,
                                             BobguiWidget  *child);
GDK_AVAILABLE_IN_ALL
BobguiWidget *           bobgui_overlay_get_child (BobguiOverlay *overlay);

GDK_AVAILABLE_IN_ALL
gboolean   bobgui_overlay_get_measure_overlay (BobguiOverlay *overlay,
                                            BobguiWidget  *widget);
GDK_AVAILABLE_IN_ALL
void       bobgui_overlay_set_measure_overlay (BobguiOverlay *overlay,
                                            BobguiWidget  *widget,
                                            gboolean    measure);
GDK_AVAILABLE_IN_ALL
gboolean   bobgui_overlay_get_clip_overlay    (BobguiOverlay *overlay,
                                            BobguiWidget  *widget);
GDK_AVAILABLE_IN_ALL
void       bobgui_overlay_set_clip_overlay    (BobguiOverlay *overlay,
                                            BobguiWidget  *widget,
                                            gboolean    clip_overlay);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiOverlay, g_object_unref)

G_END_DECLS

