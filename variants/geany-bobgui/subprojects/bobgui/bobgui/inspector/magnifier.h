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

#include <bobgui/bobguibox.h>

#define BOBGUI_TYPE_INSPECTOR_MAGNIFIER            (bobgui_inspector_magnifier_get_type())
#define BOBGUI_INSPECTOR_MAGNIFIER(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), BOBGUI_TYPE_INSPECTOR_MAGNIFIER, BobguiInspectorMagnifier))
#define BOBGUI_INSPECTOR_MAGNIFIER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), BOBGUI_TYPE_INSPECTOR_MAGNIFIER, BobguiInspectorMagnifierClass))
#define BOBGUI_INSPECTOR_IS_MAGNIFIER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), BOBGUI_TYPE_INSPECTOR_MAGNIFIER))
#define BOBGUI_INSPECTOR_IS_MAGNIFIER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), BOBGUI_TYPE_INSPECTOR_MAGNIFIER))
#define BOBGUI_INSPECTOR_MAGNIFIER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), BOBGUI_TYPE_INSPECTOR_MAGNIFIER, BobguiInspectorMagnifierClass))


typedef struct _BobguiInspectorMagnifierPrivate BobguiInspectorMagnifierPrivate;

typedef struct _BobguiInspectorMagnifier
{
  BobguiBox parent;
  BobguiInspectorMagnifierPrivate *priv;
} BobguiInspectorMagnifier;

typedef struct _BobguiInspectorMagnifierClass
{
  BobguiBoxClass parent;
} BobguiInspectorMagnifierClass;

G_BEGIN_DECLS

GType      bobgui_inspector_magnifier_get_type   (void);
void       bobgui_inspector_magnifier_set_object (BobguiInspectorMagnifier *sl,
                                               GObject               *object);

G_END_DECLS


// vim: set et sw=2 ts=2:
