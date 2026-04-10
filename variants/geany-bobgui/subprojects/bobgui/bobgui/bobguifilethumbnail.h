/* bobguifilethumbnail.h
 *
 * Copyright 2022 Georges Basile Stavracas Neto <georges.stavracas@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of the
 * License, or (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */


#pragma once

#include <gio/gio.h>

#include "bobguifilesystemmodelprivate.h"

G_BEGIN_DECLS

#define BOBGUI_TYPE_FILE_THUMBNAIL    (_bobgui_file_thumbnail_get_type ())
#define BOBGUI_FILE_THUMBNAIL(obj)    (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_FILE_THUMBNAIL, BobguiFileThumbnail))
#define BOBGUI_IS_FILE_THUMBNAIL(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_FILE_THUMBNAIL))

typedef struct _BobguiFileThumbnail      BobguiFileThumbnail;

GType _bobgui_file_thumbnail_get_type (void) G_GNUC_CONST;

GFileInfo *_bobgui_file_thumbnail_get_info (BobguiFileThumbnail *self);
void _bobgui_file_thumbnail_set_info (BobguiFileThumbnail *self,
                                   GFileInfo        *info);

int _bobgui_file_thumbnail_get_icon_size (BobguiFileThumbnail *self);
void _bobgui_file_thumbnail_set_icon_size (BobguiFileThumbnail *self,
                                        int               icon_size);

G_END_DECLS


