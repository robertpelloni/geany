/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2011      Alberto Ruiz <aruiz@gnome.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <bobgui/deprecated/bobguidialog.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_FONT_CHOOSER_DIALOG              (bobgui_font_chooser_dialog_get_type ())
#define BOBGUI_FONT_CHOOSER_DIALOG(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_FONT_CHOOSER_DIALOG, BobguiFontChooserDialog))
#define BOBGUI_IS_FONT_CHOOSER_DIALOG(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_FONT_CHOOSER_DIALOG))

typedef struct _BobguiFontChooserDialog              BobguiFontChooserDialog;

GDK_AVAILABLE_IN_ALL
GType      bobgui_font_chooser_dialog_get_type         (void) G_GNUC_CONST;
GDK_DEPRECATED_IN_4_10
BobguiWidget* bobgui_font_chooser_dialog_new              (const char           *title,
                                                     BobguiWindow            *parent);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiFontChooserDialog, g_object_unref)

G_END_DECLS

