/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2020 Red Hat, Inc.
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

#include <bobgui/bobguiwidget.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_EDITABLE_LABEL (bobgui_editable_label_get_type ())

GDK_AVAILABLE_IN_ALL
G_DECLARE_FINAL_TYPE (BobguiEditableLabel, bobgui_editable_label, BOBGUI, EDITABLE_LABEL, BobguiWidget)

GDK_AVAILABLE_IN_ALL
BobguiWidget *           bobgui_editable_label_new            (const char       *str);

GDK_AVAILABLE_IN_ALL
gboolean              bobgui_editable_label_get_editing    (BobguiEditableLabel *self);

GDK_AVAILABLE_IN_ALL
void                  bobgui_editable_label_start_editing  (BobguiEditableLabel *self);

GDK_AVAILABLE_IN_ALL
void                  bobgui_editable_label_stop_editing   (BobguiEditableLabel *self,
                                                         gboolean          commit);

G_END_DECLS

