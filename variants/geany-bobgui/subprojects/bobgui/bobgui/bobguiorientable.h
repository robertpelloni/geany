/* BOBGUI - The Bobgui Framework
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * bobguiorientable.h
 * Copyright (C) 2008 Imendio AB
 * Contact: Michael Natterer <mitch@imendio.com>
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

#define BOBGUI_TYPE_ORIENTABLE             (bobgui_orientable_get_type ())
#define BOBGUI_ORIENTABLE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_ORIENTABLE, BobguiOrientable))
#define BOBGUI_IS_ORIENTABLE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_ORIENTABLE))
#define BOBGUI_ORIENTABLE_GET_IFACE(inst)  (G_TYPE_INSTANCE_GET_INTERFACE ((inst), BOBGUI_TYPE_ORIENTABLE, BobguiOrientableIface))

typedef struct _BobguiOrientable       BobguiOrientable;         /* Dummy typedef */
typedef struct _BobguiOrientableIface  BobguiOrientableIface;

struct _BobguiOrientableIface
{
  GTypeInterface base_iface;
};


GDK_AVAILABLE_IN_ALL
GType          bobgui_orientable_get_type        (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
void           bobgui_orientable_set_orientation (BobguiOrientable  *orientable,
                                               BobguiOrientation  orientation);
GDK_AVAILABLE_IN_ALL
BobguiOrientation bobgui_orientable_get_orientation (BobguiOrientable  *orientable);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiOrientable, g_object_unref)

G_END_DECLS

