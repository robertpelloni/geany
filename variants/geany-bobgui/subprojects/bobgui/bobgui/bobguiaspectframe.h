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
 * Modified by the BOBGUI Team and others 1997-2001.  See the AUTHORS
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

#define BOBGUI_TYPE_ASPECT_FRAME            (bobgui_aspect_frame_get_type ())
#define BOBGUI_ASPECT_FRAME(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_ASPECT_FRAME, BobguiAspectFrame))
#define BOBGUI_IS_ASPECT_FRAME(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_ASPECT_FRAME))

typedef struct _BobguiAspectFrame      BobguiAspectFrame;

GDK_AVAILABLE_IN_ALL
GType      bobgui_aspect_frame_get_type   (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
BobguiWidget* bobgui_aspect_frame_new        (float            xalign,
                                        float            yalign,
                                        float            ratio,
                                        gboolean         obey_child);

GDK_AVAILABLE_IN_ALL
void       bobgui_aspect_frame_set_xalign (BobguiAspectFrame *self,
                                        float           xalign);
GDK_AVAILABLE_IN_ALL
float      bobgui_aspect_frame_get_xalign (BobguiAspectFrame *self);

GDK_AVAILABLE_IN_ALL
void       bobgui_aspect_frame_set_yalign (BobguiAspectFrame *self,
                                        float           yalign);
GDK_AVAILABLE_IN_ALL
float      bobgui_aspect_frame_get_yalign (BobguiAspectFrame *self);

GDK_AVAILABLE_IN_ALL
void       bobgui_aspect_frame_set_ratio  (BobguiAspectFrame *self,
                                        float           ratio);
GDK_AVAILABLE_IN_ALL
float      bobgui_aspect_frame_get_ratio  (BobguiAspectFrame *self);

GDK_AVAILABLE_IN_ALL
void       bobgui_aspect_frame_set_obey_child (BobguiAspectFrame *self,
                                            gboolean        obey_child);
GDK_AVAILABLE_IN_ALL
gboolean   bobgui_aspect_frame_get_obey_child (BobguiAspectFrame *self);

GDK_AVAILABLE_IN_ALL
void       bobgui_aspect_frame_set_child  (BobguiAspectFrame *self,
                                        BobguiWidget      *child);
GDK_AVAILABLE_IN_ALL
BobguiWidget *bobgui_aspect_frame_get_child  (BobguiAspectFrame *self);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiAspectFrame, g_object_unref)

G_END_DECLS

