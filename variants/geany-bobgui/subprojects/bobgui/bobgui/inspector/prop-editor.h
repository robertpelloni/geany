/*
 * Copyright (c) 2014 Red Hat, Inc.
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


#include <bobgui/bobgui.h>
#include <bobgui/bobguisizegroup.h>

G_BEGIN_DECLS


#define BOBGUI_TYPE_INSPECTOR_PROP_EDITOR            (bobgui_inspector_prop_editor_get_type())

G_DECLARE_FINAL_TYPE (BobguiInspectorPropEditor, bobgui_inspector_prop_editor, BOBGUI, INSPECTOR_PROP_EDITOR, BobguiBox)

BobguiWidget *bobgui_inspector_prop_editor_new      (GObject      *object,
                                               const char   *name,
                                               BobguiSizeGroup *values);

gboolean   bobgui_inspector_prop_editor_should_expand (BobguiInspectorPropEditor *editor);

G_END_DECLS



// vim: set et:
