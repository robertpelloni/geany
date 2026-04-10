/* bobguipathbarprivate.h
 * Copyright (C) 2004  Red Hat, Inc.,  Jonathan Blandford <jrb@gnome.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "bobguiwidget.h"

G_BEGIN_DECLS

#define BOBGUI_TYPE_PATH_BAR    (bobgui_path_bar_get_type ())
#define BOBGUI_PATH_BAR(obj)    (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_PATH_BAR, BobguiPathBar))
#define BOBGUI_IS_PATH_BAR(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_PATH_BAR))

typedef struct _BobguiPathBar BobguiPathBar;

GDK_AVAILABLE_IN_ALL
GType    bobgui_path_bar_get_type (void) G_GNUC_CONST;
void     _bobgui_path_bar_set_file        (BobguiPathBar         *path_bar,
                                        GFile              *file,
                                        gboolean            keep_trail);
void     _bobgui_path_bar_up              (BobguiPathBar *path_bar);
void     _bobgui_path_bar_down            (BobguiPathBar *path_bar);

G_END_DECLS

