/*
 * Copyright (c) 2015 Red Hat, Inc.
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


#define BOBGUI_TYPE_INSPECTOR_STRV_EDITOR            (bobgui_inspector_strv_editor_get_type())
#define BOBGUI_INSPECTOR_STRV_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), BOBGUI_TYPE_INSPECTOR_STRV_EDITOR, BobguiInspectorStrvEditor))
#define BOBGUI_INSPECTOR_STRV_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), BOBGUI_TYPE_INSPECTOR_STRV_EDITOR, BobguiInspectorStrvEditorClass))
#define BOBGUI_INSPECTOR_IS_STRV_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), BOBGUI_TYPE_INSPECTOR_STRV_EDITOR))
#define BOBGUI_INSPECTOR_IS_STRV_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), BOBGUI_TYPE_INSPECTOR_STRV_EDITOR))
#define BOBGUI_INSPECTOR_STRV_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), BOBGUI_TYPE_INSPECTOR_STRV_EDITOR, BobguiInspectorStrvEditorClass))


typedef struct
{
  BobguiBox parent;

  BobguiWidget *box;
  BobguiWidget *button;

  gboolean blocked;
} BobguiInspectorStrvEditor;

typedef struct
{
  BobguiBoxClass parent;

  void (* changed) (BobguiInspectorStrvEditor *editor);

} BobguiInspectorStrvEditorClass;


G_BEGIN_DECLS


GType bobgui_inspector_strv_editor_get_type (void);

void    bobgui_inspector_strv_editor_set_strv (BobguiInspectorStrvEditor  *editor,
                                            char                   **strv);

char **bobgui_inspector_strv_editor_get_strv (BobguiInspectorStrvEditor  *editor);

G_END_DECLS



// vim: set et:
