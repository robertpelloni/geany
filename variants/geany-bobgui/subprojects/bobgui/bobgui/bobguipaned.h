/* BOBGUI - The Bobgui Framework
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
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

/*
 * Modified by the BOBGUI Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the BOBGUI Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BOBGUI at ftp://ftp.bobgui.org/pub/bobgui/.
 */

#pragma once

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <bobgui/bobguiwidget.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_PANED                  (bobgui_paned_get_type ())
#define BOBGUI_PANED(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_PANED, BobguiPaned))
#define BOBGUI_IS_PANED(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_PANED))

typedef struct _BobguiPaned BobguiPaned;

GDK_AVAILABLE_IN_ALL
GType       bobgui_paned_get_type     (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
BobguiWidget * bobgui_paned_new          (BobguiOrientation orientation);
GDK_AVAILABLE_IN_ALL
void        bobgui_paned_set_start_child (BobguiPaned       *paned,
                                       BobguiWidget      *child);
GDK_AVAILABLE_IN_ALL
BobguiWidget * bobgui_paned_get_start_child (BobguiPaned       *paned);
GDK_AVAILABLE_IN_ALL
void        bobgui_paned_set_resize_start_child (BobguiPaned *paned,
                                              gboolean  resize);
GDK_AVAILABLE_IN_ALL
gboolean    bobgui_paned_get_resize_start_child (BobguiPaned *paned);

GDK_AVAILABLE_IN_ALL
void        bobgui_paned_set_end_child   (BobguiPaned       *paned,
                                       BobguiWidget      *child);
GDK_AVAILABLE_IN_ALL
BobguiWidget * bobgui_paned_get_end_child   (BobguiPaned       *paned);

GDK_AVAILABLE_IN_ALL
void        bobgui_paned_set_shrink_start_child (BobguiPaned *paned,
                                              gboolean  resize);
GDK_AVAILABLE_IN_ALL
gboolean    bobgui_paned_get_shrink_start_child (BobguiPaned *paned);

GDK_AVAILABLE_IN_ALL
void        bobgui_paned_set_resize_end_child (BobguiPaned *paned,
                                              gboolean  resize);
GDK_AVAILABLE_IN_ALL
gboolean    bobgui_paned_get_resize_end_child (BobguiPaned *paned);

GDK_AVAILABLE_IN_ALL
void        bobgui_paned_set_shrink_end_child (BobguiPaned *paned,
                                              gboolean  resize);
GDK_AVAILABLE_IN_ALL
gboolean    bobgui_paned_get_shrink_end_child (BobguiPaned *paned);

GDK_AVAILABLE_IN_ALL
int         bobgui_paned_get_position (BobguiPaned       *paned);
GDK_AVAILABLE_IN_ALL
void        bobgui_paned_set_position (BobguiPaned       *paned,
                                    int             position);

GDK_AVAILABLE_IN_ALL
void        bobgui_paned_set_wide_handle (BobguiPaned    *paned,
                                       gboolean     wide);
GDK_AVAILABLE_IN_ALL
gboolean    bobgui_paned_get_wide_handle (BobguiPaned    *paned);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiPaned, g_object_unref)

G_END_DECLS

