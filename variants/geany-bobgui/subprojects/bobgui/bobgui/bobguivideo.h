/*
 * Copyright © 2018 Benjamin Otte
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

#include <bobgui/bobguimediastream.h>
#include <bobgui/bobguiwidget.h>
#include <bobgui/bobguigraphicsoffload.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_VIDEO         (bobgui_video_get_type ())

GDK_AVAILABLE_IN_ALL
G_DECLARE_FINAL_TYPE (BobguiVideo, bobgui_video, BOBGUI, VIDEO, BobguiWidget)

GDK_AVAILABLE_IN_ALL
BobguiWidget *     bobgui_video_new                           (void);
GDK_AVAILABLE_IN_ALL
BobguiWidget *     bobgui_video_new_for_media_stream          (BobguiMediaStream         *stream);
GDK_AVAILABLE_IN_ALL
BobguiWidget *     bobgui_video_new_for_file                  (GFile                  *file);
GDK_AVAILABLE_IN_ALL
BobguiWidget *     bobgui_video_new_for_filename              (const char             *filename);
GDK_AVAILABLE_IN_ALL
BobguiWidget *     bobgui_video_new_for_resource              (const char             *resource_path);

GDK_AVAILABLE_IN_ALL
BobguiMediaStream *bobgui_video_get_media_stream              (BobguiVideo               *self);
GDK_AVAILABLE_IN_ALL
void            bobgui_video_set_media_stream              (BobguiVideo               *self,
                                                         BobguiMediaStream         *stream);
GDK_AVAILABLE_IN_ALL
GFile *         bobgui_video_get_file                      (BobguiVideo               *self);
GDK_AVAILABLE_IN_ALL
void            bobgui_video_set_file                      (BobguiVideo               *self,
                                                         GFile                  *file);
GDK_AVAILABLE_IN_ALL
void            bobgui_video_set_filename                  (BobguiVideo               *self,
                                                         const char             *filename);
GDK_AVAILABLE_IN_ALL
void            bobgui_video_set_resource                  (BobguiVideo               *self,
                                                         const char             *resource_path);
GDK_AVAILABLE_IN_ALL
gboolean        bobgui_video_get_autoplay                  (BobguiVideo               *self);
GDK_AVAILABLE_IN_ALL
void            bobgui_video_set_autoplay                  (BobguiVideo               *self,
                                                         gboolean                autoplay);
GDK_AVAILABLE_IN_ALL
gboolean        bobgui_video_get_loop                      (BobguiVideo               *self);
GDK_AVAILABLE_IN_ALL
void            bobgui_video_set_loop                      (BobguiVideo               *self,
                                                         gboolean                loop);

GDK_AVAILABLE_IN_4_14
BobguiGraphicsOffloadEnabled
                bobgui_video_get_graphics_offload          (BobguiVideo               *self);
GDK_AVAILABLE_IN_4_14
void            bobgui_video_set_graphics_offload          (BobguiVideo               *self,
                                                         BobguiGraphicsOffloadEnabled enabled);

G_END_DECLS

