/*
 * Copyright (c) 2021 Benjamin Otte
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

#include <bobgui/bobguiwidget.h>

#define BOBGUI_TYPE_INSPECTOR_CLIPBOARD            (bobgui_inspector_clipboard_get_type())
#define BOBGUI_INSPECTOR_CLIPBOARD(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), BOBGUI_TYPE_INSPECTOR_CLIPBOARD, BobguiInspectorClipboard))
#define BOBGUI_INSPECTOR_IS_CLIPBOARD(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), BOBGUI_TYPE_INSPECTOR_CLIPBOARD))

typedef struct _BobguiInspectorClipboard BobguiInspectorClipboard;

G_BEGIN_DECLS

GType           bobgui_inspector_clipboard_get_type                (void);

void            bobgui_inspector_clipboard_set_display             (BobguiInspectorClipboard  *self,
                                                                 GdkDisplay             *display);

G_END_DECLS

