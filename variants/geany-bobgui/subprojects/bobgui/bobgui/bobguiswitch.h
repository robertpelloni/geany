/* BOBGUI - The Bobgui Framework
 *
 * Copyright (C) 2010  Intel Corporation
 * Copyright (C) 2010  RedHat, Inc.
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
 *
 * Author:
 *      Emmanuele Bassi <ebassi@linux.intel.com>
 *      Matthias Clasen <mclasen@redhat.com>
 *
 * Based on similar code from Mx.
 */

#pragma once

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <bobgui/bobguiwidget.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_SWITCH                 (bobgui_switch_get_type ())
#define BOBGUI_SWITCH(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_SWITCH, BobguiSwitch))
#define BOBGUI_IS_SWITCH(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_SWITCH))

typedef struct _BobguiSwitch               BobguiSwitch;


GDK_AVAILABLE_IN_ALL
GType bobgui_switch_get_type (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
BobguiWidget *     bobgui_switch_new          (void);

GDK_AVAILABLE_IN_ALL
void            bobgui_switch_set_active   (BobguiSwitch *self,
                                         gboolean   is_active);
GDK_AVAILABLE_IN_ALL
gboolean        bobgui_switch_get_active   (BobguiSwitch *self);

GDK_AVAILABLE_IN_ALL
void            bobgui_switch_set_state   (BobguiSwitch *self,
                                        gboolean   state);
GDK_AVAILABLE_IN_ALL
gboolean        bobgui_switch_get_state   (BobguiSwitch *self);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiSwitch, g_object_unref)

G_END_DECLS

