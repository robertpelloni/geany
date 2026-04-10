/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2012 Red Hat, Inc.
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

#include <bobgui/bobguibox.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_COLOR_EDITOR            (bobgui_color_editor_get_type ())
#define BOBGUI_COLOR_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_COLOR_EDITOR, BobguiColorEditor))
#define BOBGUI_COLOR_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BOBGUI_TYPE_COLOR_EDITOR, BobguiColorEditorClass))
#define BOBGUI_IS_COLOR_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_COLOR_EDITOR))
#define BOBGUI_IS_COLOR_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BOBGUI_TYPE_COLOR_EDITOR))
#define BOBGUI_COLOR_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_COLOR_EDITOR, BobguiColorEditorClass))


typedef struct _BobguiColorEditor         BobguiColorEditor;

GType       bobgui_color_editor_get_type (void) G_GNUC_CONST;
BobguiWidget * bobgui_color_editor_new      (void);

G_END_DECLS

