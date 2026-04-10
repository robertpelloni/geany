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

#include <gio/gio.h>
#include <bobgui/bobguiwidget.h>


G_BEGIN_DECLS

#define BOBGUI_TYPE_IMAGE                  (bobgui_image_get_type ())
#define BOBGUI_IMAGE(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_IMAGE, BobguiImage))
#define BOBGUI_IS_IMAGE(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_IMAGE))

typedef struct _BobguiImage              BobguiImage;

/**
 * BobguiImageType:
 * @BOBGUI_IMAGE_EMPTY: there is no image displayed by the widget
 * @BOBGUI_IMAGE_ICON_NAME: the widget contains a named icon
 * @BOBGUI_IMAGE_GICON: the widget contains a `GIcon`
 * @BOBGUI_IMAGE_PAINTABLE: the widget contains a `GdkPaintable`
 *
 * Describes the image data representation used by a [class@Bobgui.Image].
 *
 * If you want to get the image from the widget, you can only get the
 * currently-stored representation; for instance, if the bobgui_image_get_storage_type()
 * returns %BOBGUI_IMAGE_PAINTABLE, then you can call bobgui_image_get_paintable().
 *
 * For empty images, you can request any storage type (call any of the "get"
 * functions), but they will all return %NULL values.
 */
typedef enum
{
  BOBGUI_IMAGE_EMPTY,
  BOBGUI_IMAGE_ICON_NAME,
  BOBGUI_IMAGE_GICON,
  BOBGUI_IMAGE_PAINTABLE
} BobguiImageType;

GDK_AVAILABLE_IN_ALL
GType      bobgui_image_get_type (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
BobguiWidget* bobgui_image_new                (void);
GDK_AVAILABLE_IN_ALL
BobguiWidget* bobgui_image_new_from_file      (const char      *filename);
GDK_AVAILABLE_IN_ALL
BobguiWidget* bobgui_image_new_from_resource  (const char      *resource_path);
GDK_DEPRECATED_IN_4_12_FOR(bobgui_image_new_from_paintable)
BobguiWidget* bobgui_image_new_from_pixbuf    (GdkPixbuf       *pixbuf);
GDK_AVAILABLE_IN_ALL
BobguiWidget* bobgui_image_new_from_paintable (GdkPaintable    *paintable);
GDK_AVAILABLE_IN_ALL
BobguiWidget* bobgui_image_new_from_icon_name (const char      *icon_name);
GDK_AVAILABLE_IN_ALL
BobguiWidget* bobgui_image_new_from_gicon     (GIcon           *icon);

GDK_AVAILABLE_IN_ALL
void bobgui_image_clear              (BobguiImage        *image);
GDK_AVAILABLE_IN_ALL
void bobgui_image_set_from_file      (BobguiImage        *image,
                                   const char      *filename);
GDK_AVAILABLE_IN_ALL
void bobgui_image_set_from_resource  (BobguiImage        *image,
                                   const char      *resource_path);
GDK_DEPRECATED_IN_4_12_FOR(bobgui_image_set_from_paintable)
void bobgui_image_set_from_pixbuf    (BobguiImage        *image,
                                   GdkPixbuf       *pixbuf);
GDK_AVAILABLE_IN_ALL
void bobgui_image_set_from_paintable (BobguiImage        *image,
                                   GdkPaintable    *paintable);
GDK_AVAILABLE_IN_ALL
void bobgui_image_set_from_icon_name (BobguiImage        *image,
				   const char      *icon_name);
GDK_AVAILABLE_IN_ALL
void bobgui_image_set_from_gicon     (BobguiImage        *image,
				   GIcon           *icon);
GDK_AVAILABLE_IN_ALL
void bobgui_image_set_pixel_size     (BobguiImage        *image,
				   int              pixel_size);
GDK_AVAILABLE_IN_ALL
void bobgui_image_set_icon_size      (BobguiImage        *image,
                                   BobguiIconSize      icon_size);

GDK_AVAILABLE_IN_ALL
BobguiImageType bobgui_image_get_storage_type (BobguiImage   *image);

GDK_AVAILABLE_IN_ALL
GdkPaintable *bobgui_image_get_paintable (BobguiImage       *image);

GDK_AVAILABLE_IN_ALL
const char *bobgui_image_get_icon_name (BobguiImage     *image);
GDK_AVAILABLE_IN_ALL
GIcon *    bobgui_image_get_gicon     (BobguiImage              *image);
GDK_AVAILABLE_IN_ALL
int        bobgui_image_get_pixel_size (BobguiImage             *image);
GDK_AVAILABLE_IN_ALL
BobguiIconSize bobgui_image_get_icon_size (BobguiImage             *image);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiImage, g_object_unref)

G_END_DECLS

