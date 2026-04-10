/* bobguiscrollable.h
 * Copyright (C) 2008 Tadej Borovšak <tadeboro@gmail.com>
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

#include <gdk/gdk.h>
#include <bobgui/bobguienums.h>
#include <bobgui/bobguitypes.h>
#include <bobgui/bobguiborder.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_SCROLLABLE            (bobgui_scrollable_get_type ())
#define BOBGUI_SCROLLABLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj),     BOBGUI_TYPE_SCROLLABLE, BobguiScrollable))
#define BOBGUI_IS_SCROLLABLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj),     BOBGUI_TYPE_SCROLLABLE))
#define BOBGUI_SCROLLABLE_GET_IFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE ((inst), BOBGUI_TYPE_SCROLLABLE, BobguiScrollableInterface))

typedef struct _BobguiScrollable          BobguiScrollable; /* Dummy */
typedef struct _BobguiScrollableInterface BobguiScrollableInterface;

struct _BobguiScrollableInterface
{
  GTypeInterface base_iface;

  gboolean (* get_border) (BobguiScrollable *scrollable,
                           BobguiBorder     *border);
};

/* Public API */
GDK_AVAILABLE_IN_ALL
GType                bobgui_scrollable_get_type               (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
BobguiAdjustment       *bobgui_scrollable_get_hadjustment        (BobguiScrollable       *scrollable);
GDK_AVAILABLE_IN_ALL
void                 bobgui_scrollable_set_hadjustment        (BobguiScrollable       *scrollable,
							    BobguiAdjustment       *hadjustment);
GDK_AVAILABLE_IN_ALL
BobguiAdjustment       *bobgui_scrollable_get_vadjustment        (BobguiScrollable       *scrollable);
GDK_AVAILABLE_IN_ALL
void                 bobgui_scrollable_set_vadjustment        (BobguiScrollable       *scrollable,
							    BobguiAdjustment       *vadjustment);
GDK_AVAILABLE_IN_ALL
BobguiScrollablePolicy  bobgui_scrollable_get_hscroll_policy     (BobguiScrollable       *scrollable);
GDK_AVAILABLE_IN_ALL
void                 bobgui_scrollable_set_hscroll_policy     (BobguiScrollable       *scrollable,
							    BobguiScrollablePolicy  policy);
GDK_AVAILABLE_IN_ALL
BobguiScrollablePolicy  bobgui_scrollable_get_vscroll_policy     (BobguiScrollable       *scrollable);
GDK_AVAILABLE_IN_ALL
void                 bobgui_scrollable_set_vscroll_policy     (BobguiScrollable       *scrollable,
							    BobguiScrollablePolicy  policy);

GDK_AVAILABLE_IN_ALL
gboolean             bobgui_scrollable_get_border             (BobguiScrollable       *scrollable,
                                                            BobguiBorder           *border);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiScrollable, g_object_unref)

G_END_DECLS

