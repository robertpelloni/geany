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


#include <bobgui/bobguiwidget.h>
#include <bobgui/bobguisizegroup.h>


#define BOBGUI_TYPE_INSPECTOR_VARIANT_EDITOR            (bobgui_inspector_variant_editor_get_type())
#define BOBGUI_INSPECTOR_VARIANT_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), BOBGUI_TYPE_INSPECTOR_VARIANT_EDITOR, BobguiInspectorVariantEditor))
#define BOBGUI_INSPECTOR_IS_VARIANT_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), BOBGUI_TYPE_INSPECTOR_VARIANT_EDITOR))


typedef struct _BobguiInspectorVariantEditor BobguiInspectorVariantEditor;

G_BEGIN_DECLS

typedef void (* BobguiInspectorVariantEditorChanged) (BobguiWidget *editor,
                                                   gpointer   data);

GType      bobgui_inspector_variant_editor_get_type (void);
BobguiWidget *bobgui_inspector_variant_editor_new      (const GVariantType               *type,
                                                  BobguiInspectorVariantEditorChanged  callback,
                                                  gpointer                          data);
void       bobgui_inspector_variant_editor_set_type (BobguiWidget                        *editor,
                                                  const GVariantType               *type);
void       bobgui_inspector_variant_editor_set_value (BobguiWidget                       *editor,
                                                   GVariant                        *value);
GVariant * bobgui_inspector_variant_editor_get_value (BobguiWidget                       *editor);


G_END_DECLS



// vim: set et:
