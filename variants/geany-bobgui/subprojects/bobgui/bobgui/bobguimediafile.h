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

G_BEGIN_DECLS

/**
 * BOBGUI_MEDIA_FILE_EXTENSION_POINT_NAME:
 *
 * The default extension point name for media file.
 */
#define BOBGUI_MEDIA_FILE_EXTENSION_POINT_NAME "bobgui-media-file"

#define BOBGUI_TYPE_MEDIA_FILE             (bobgui_media_file_get_type ())

GDK_AVAILABLE_IN_ALL
G_DECLARE_DERIVABLE_TYPE (BobguiMediaFile, bobgui_media_file, BOBGUI, MEDIA_FILE, BobguiMediaStream)

struct _BobguiMediaFileClass
{
  BobguiMediaStreamClass parent_class;

  void                  (* open)                                (BobguiMediaFile *self);
  void                  (* close)                               (BobguiMediaFile *self);

  /* Padding for future expansion */
  void (*_bobgui_reserved1) (void);
  void (*_bobgui_reserved2) (void);
  void (*_bobgui_reserved3) (void);
  void (*_bobgui_reserved4) (void);
};

GDK_AVAILABLE_IN_ALL
BobguiMediaStream *        bobgui_media_file_new                      (void);
GDK_AVAILABLE_IN_ALL
BobguiMediaStream *        bobgui_media_file_new_for_filename         (const char     *filename);
GDK_AVAILABLE_IN_ALL
BobguiMediaStream *        bobgui_media_file_new_for_resource         (const char     *resource_path);
GDK_AVAILABLE_IN_ALL
BobguiMediaStream *        bobgui_media_file_new_for_file             (GFile          *file);
GDK_AVAILABLE_IN_ALL
BobguiMediaStream *        bobgui_media_file_new_for_input_stream     (GInputStream   *stream);

GDK_AVAILABLE_IN_ALL
void                    bobgui_media_file_clear                    (BobguiMediaFile   *self);

GDK_AVAILABLE_IN_ALL
void                    bobgui_media_file_set_filename             (BobguiMediaFile   *self,
                                                                 const char     *filename);
GDK_AVAILABLE_IN_ALL
void                    bobgui_media_file_set_resource             (BobguiMediaFile   *self,
                                                                 const char     *resource_path);
GDK_AVAILABLE_IN_ALL
void                    bobgui_media_file_set_file                 (BobguiMediaFile   *self,
                                                                 GFile          *file);
GDK_AVAILABLE_IN_ALL
GFile *                 bobgui_media_file_get_file                 (BobguiMediaFile   *self);
GDK_AVAILABLE_IN_ALL
void                    bobgui_media_file_set_input_stream         (BobguiMediaFile   *self,
                                                                 GInputStream   *stream);
GDK_AVAILABLE_IN_ALL
GInputStream *          bobgui_media_file_get_input_stream         (BobguiMediaFile   *self);


G_END_DECLS

