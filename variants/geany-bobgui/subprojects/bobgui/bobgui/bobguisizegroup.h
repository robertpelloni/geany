/* BOBGUI - The Bobgui Framework
 * bobguisizegroup.h:
 * Copyright (C) 2000 Red Hat Software
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

#define BOBGUI_TYPE_SIZE_GROUP            (bobgui_size_group_get_type ())
#define BOBGUI_SIZE_GROUP(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_SIZE_GROUP, BobguiSizeGroup))
#define BOBGUI_IS_SIZE_GROUP(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_SIZE_GROUP))

typedef struct _BobguiSizeGroup              BobguiSizeGroup;

struct _BobguiSizeGroup
{
  GObject parent_instance;
};

GDK_AVAILABLE_IN_ALL
GType            bobgui_size_group_get_type      (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
BobguiSizeGroup *   bobgui_size_group_new           (BobguiSizeGroupMode  mode);
GDK_AVAILABLE_IN_ALL
void             bobgui_size_group_set_mode      (BobguiSizeGroup     *size_group,
					       BobguiSizeGroupMode  mode);
GDK_AVAILABLE_IN_ALL
BobguiSizeGroupMode bobgui_size_group_get_mode      (BobguiSizeGroup     *size_group);
GDK_AVAILABLE_IN_ALL
void             bobgui_size_group_add_widget    (BobguiSizeGroup     *size_group,
					       BobguiWidget        *widget);
GDK_AVAILABLE_IN_ALL
void             bobgui_size_group_remove_widget (BobguiSizeGroup     *size_group,
					       BobguiWidget        *widget);
GDK_AVAILABLE_IN_ALL
GSList *         bobgui_size_group_get_widgets   (BobguiSizeGroup     *size_group);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiSizeGroup, g_object_unref)

G_END_DECLS

