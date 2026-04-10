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

#include <gio/gio.h>
#include <bobgui/bobguiwidget.h>


G_BEGIN_DECLS

#define BOBGUI_TYPE_PICTURE (bobgui_picture_get_type ())

GDK_AVAILABLE_IN_ALL
G_DECLARE_FINAL_TYPE (BobguiPicture, bobgui_picture, BOBGUI, PICTURE, BobguiWidget)

GDK_AVAILABLE_IN_ALL
BobguiWidget*      bobgui_picture_new                         (void);
GDK_AVAILABLE_IN_ALL
BobguiWidget*      bobgui_picture_new_for_paintable           (GdkPaintable           *paintable);
GDK_DEPRECATED_IN_4_12_FOR(bobgui_pixbuf_new_for_paintable)
BobguiWidget*      bobgui_picture_new_for_pixbuf              (GdkPixbuf              *pixbuf);
GDK_AVAILABLE_IN_ALL
BobguiWidget*      bobgui_picture_new_for_file                (GFile                  *file);
GDK_AVAILABLE_IN_ALL
BobguiWidget*      bobgui_picture_new_for_filename            (const char             *filename);
GDK_AVAILABLE_IN_ALL
BobguiWidget*      bobgui_picture_new_for_resource            (const char             *resource_path);

GDK_AVAILABLE_IN_ALL
void            bobgui_picture_set_paintable               (BobguiPicture             *self,
                                                         GdkPaintable           *paintable);
GDK_AVAILABLE_IN_ALL
GdkPaintable *  bobgui_picture_get_paintable               (BobguiPicture             *self);
GDK_AVAILABLE_IN_ALL
void            bobgui_picture_set_file                    (BobguiPicture             *self,
                                                         GFile                  *file);
GDK_AVAILABLE_IN_ALL
GFile *         bobgui_picture_get_file                    (BobguiPicture             *self);
GDK_AVAILABLE_IN_ALL
void            bobgui_picture_set_filename                (BobguiPicture             *self,
                                                         const char             *filename);
GDK_AVAILABLE_IN_ALL
void            bobgui_picture_set_resource                (BobguiPicture             *self,
                                                         const char             *resource_path);
GDK_DEPRECATED_IN_4_12_FOR(bobgui_picture_set_paintable)
void            bobgui_picture_set_pixbuf                  (BobguiPicture             *self,
                                                         GdkPixbuf              *pixbuf);

GDK_DEPRECATED_IN_4_8_FOR(bobgui_picture_set_content_fit)
void            bobgui_picture_set_keep_aspect_ratio       (BobguiPicture             *self,
                                                         gboolean                keep_aspect_ratio);
GDK_DEPRECATED_IN_4_8_FOR(bobgui_picture_get_content_fit)
gboolean        bobgui_picture_get_keep_aspect_ratio       (BobguiPicture             *self);
GDK_AVAILABLE_IN_ALL
void            bobgui_picture_set_can_shrink              (BobguiPicture             *self,
                                                         gboolean                can_shrink);
GDK_AVAILABLE_IN_ALL
gboolean        bobgui_picture_get_can_shrink              (BobguiPicture             *self);

GDK_AVAILABLE_IN_4_8
void            bobgui_picture_set_content_fit             (BobguiPicture             *self,
                                                         BobguiContentFit           content_fit);
GDK_AVAILABLE_IN_4_8
BobguiContentFit   bobgui_picture_get_content_fit             (BobguiPicture             *self);

GDK_AVAILABLE_IN_4_22
void            bobgui_picture_set_isolate_contents        (BobguiPicture             *self,
                                                         gboolean                isolate_contents);
GDK_AVAILABLE_IN_4_22
gboolean        bobgui_picture_get_isolate_contents        (BobguiPicture             *self);

GDK_AVAILABLE_IN_ALL
void            bobgui_picture_set_alternative_text        (BobguiPicture             *self,
                                                         const char             *alternative_text);
GDK_AVAILABLE_IN_ALL
const char *    bobgui_picture_get_alternative_text        (BobguiPicture             *self);


G_END_DECLS

