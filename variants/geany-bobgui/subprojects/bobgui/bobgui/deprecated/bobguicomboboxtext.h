/* BOBGUI - The Bobgui Framework
 *
 * Copyright (C) 2010 Christian Dywan
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
 */

#pragma once

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <bobgui/deprecated/bobguicombobox.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_COMBO_BOX_TEXT                 (bobgui_combo_box_text_get_type ())
#define BOBGUI_COMBO_BOX_TEXT(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_COMBO_BOX_TEXT, BobguiComboBoxText))
#define BOBGUI_IS_COMBO_BOX_TEXT(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_COMBO_BOX_TEXT))

typedef struct _BobguiComboBoxText BobguiComboBoxText;

GDK_AVAILABLE_IN_ALL
GType         bobgui_combo_box_text_get_type        (void) G_GNUC_CONST;
GDK_DEPRECATED_IN_4_10_FOR(BobguiDropDown and BobguiStringList)
BobguiWidget*    bobgui_combo_box_text_new             (void);
GDK_DEPRECATED_IN_4_10_FOR(BobguiDropDown and BobguiStringList)
BobguiWidget*    bobgui_combo_box_text_new_with_entry  (void);
GDK_DEPRECATED_IN_4_10_FOR(BobguiDropDown and BobguiStringList)
void          bobgui_combo_box_text_append_text     (BobguiComboBoxText     *combo_box,
                                                  const char          *text);
GDK_DEPRECATED_IN_4_10_FOR(BobguiDropDown and BobguiStringList)
void          bobgui_combo_box_text_insert_text     (BobguiComboBoxText     *combo_box,
                                                  int                  position,
                                                  const char          *text);
GDK_DEPRECATED_IN_4_10_FOR(BobguiDropDown and BobguiStringList)
void          bobgui_combo_box_text_prepend_text    (BobguiComboBoxText     *combo_box,
                                                  const char          *text);
GDK_DEPRECATED_IN_4_10_FOR(BobguiDropDown and BobguiStringList)
void          bobgui_combo_box_text_remove          (BobguiComboBoxText     *combo_box,
                                                  int                  position);
GDK_DEPRECATED_IN_4_10_FOR(BobguiDropDown and BobguiStringList)
void          bobgui_combo_box_text_remove_all      (BobguiComboBoxText     *combo_box);
GDK_DEPRECATED_IN_4_10_FOR(BobguiDropDown and BobguiStringList)
char         *bobgui_combo_box_text_get_active_text (BobguiComboBoxText     *combo_box);

GDK_DEPRECATED_IN_4_10_FOR(BobguiDropDown and BobguiStringList)
void          bobgui_combo_box_text_insert          (BobguiComboBoxText     *combo_box,
                                                  int                  position,
                                                  const char          *id,
                                                  const char          *text);
GDK_DEPRECATED_IN_4_10_FOR(BobguiDropDown and BobguiStringList)
void          bobgui_combo_box_text_append          (BobguiComboBoxText     *combo_box,
                                                  const char          *id,
                                                  const char          *text);
GDK_DEPRECATED_IN_4_10_FOR(BobguiDropDown and BobguiStringList)
void          bobgui_combo_box_text_prepend         (BobguiComboBoxText     *combo_box,
                                                  const char          *id,
                                                  const char          *text);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiComboBoxText, g_object_unref)

G_END_DECLS

