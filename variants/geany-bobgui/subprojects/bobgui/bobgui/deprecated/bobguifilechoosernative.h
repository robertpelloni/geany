/* BOBGUI - The Bobgui Framework
 * bobguifilechoosernative.h: Native File selector dialog
 * Copyright (C) 2015, Red Hat, Inc.
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

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <bobgui/deprecated/bobguifilechooser.h>
#include <bobgui/bobguinativedialog.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_FILE_CHOOSER_NATIVE             (bobgui_file_chooser_native_get_type ())

GDK_AVAILABLE_IN_ALL
G_DECLARE_FINAL_TYPE (BobguiFileChooserNative, bobgui_file_chooser_native, BOBGUI, FILE_CHOOSER_NATIVE, BobguiNativeDialog)

GDK_DEPRECATED_IN_4_10
BobguiFileChooserNative *bobgui_file_chooser_native_new (const char           *title,
                                                   BobguiWindow            *parent,
                                                   BobguiFileChooserAction  action,
                                                   const char           *accept_label,
                                                   const char           *cancel_label);

GDK_DEPRECATED_IN_4_10
const char *bobgui_file_chooser_native_get_accept_label (BobguiFileChooserNative *self);
GDK_DEPRECATED_IN_4_10
void        bobgui_file_chooser_native_set_accept_label (BobguiFileChooserNative *self,
                                                      const char           *accept_label);
GDK_DEPRECATED_IN_4_10
const char *bobgui_file_chooser_native_get_cancel_label (BobguiFileChooserNative *self);
GDK_DEPRECATED_IN_4_10
void        bobgui_file_chooser_native_set_cancel_label (BobguiFileChooserNative *self,
                                                      const char           *cancel_label);

G_END_DECLS

