/* BOBGUI - The Bobgui Framework
 * bobguitextviewchild-private.h Copyright (C) 2019 Red Hat, Inc.
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
#include <bobgui/bobguitextview.h>

#define BOBGUI_TYPE_TEXT_VIEW_CHILD (bobgui_text_view_child_get_type())

G_GNUC_INTERNAL
G_DECLARE_FINAL_TYPE (BobguiTextViewChild, bobgui_text_view_child, BOBGUI, TEXT_VIEW_CHILD, BobguiWidget)

G_GNUC_INTERNAL
BobguiWidget         *bobgui_text_view_child_new             (BobguiTextWindowType  window_type);
G_GNUC_INTERNAL
BobguiTextWindowType  bobgui_text_view_child_get_window_type (BobguiTextViewChild  *self);

G_GNUC_INTERNAL
void               bobgui_text_view_child_add             (BobguiTextViewChild  *self,
                                                        BobguiWidget         *widget);
G_GNUC_INTERNAL
void               bobgui_text_view_child_remove          (BobguiTextViewChild  *self,
                                                        BobguiWidget         *widget);
G_GNUC_INTERNAL
void               bobgui_text_view_child_add_overlay     (BobguiTextViewChild  *self,
                                                        BobguiWidget         *widget,
                                                        int                xpos,
                                                        int                ypos);
G_GNUC_INTERNAL
void               bobgui_text_view_child_move_overlay    (BobguiTextViewChild  *self,
                                                        BobguiWidget         *widget,
                                                        int                xpos,
                                                        int                ypos);
G_GNUC_INTERNAL
void               bobgui_text_view_child_set_offset      (BobguiTextViewChild  *child,
                                                        int                xoffset,
                                                        int                yoffset);

