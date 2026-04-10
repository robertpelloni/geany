/*
 * Copyright (c) 2013 Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#pragma once

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <bobgui/bobguiwidget.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_HEADER_BAR            (bobgui_header_bar_get_type ())
#define BOBGUI_HEADER_BAR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_HEADER_BAR, BobguiHeaderBar))
#define BOBGUI_IS_HEADER_BAR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_HEADER_BAR))

typedef struct _BobguiHeaderBar              BobguiHeaderBar;

GDK_AVAILABLE_IN_ALL
GType        bobgui_header_bar_get_type          (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
BobguiWidget   *bobgui_header_bar_new               (void);

GDK_AVAILABLE_IN_ALL
void         bobgui_header_bar_set_title_widget  (BobguiHeaderBar *bar,
                                               BobguiWidget    *title_widget);
GDK_AVAILABLE_IN_ALL
BobguiWidget   *bobgui_header_bar_get_title_widget  (BobguiHeaderBar *bar);
GDK_AVAILABLE_IN_ALL
void         bobgui_header_bar_pack_start        (BobguiHeaderBar *bar,
                                               BobguiWidget    *child);
GDK_AVAILABLE_IN_ALL
void         bobgui_header_bar_pack_end          (BobguiHeaderBar *bar,
                                               BobguiWidget    *child);
GDK_AVAILABLE_IN_ALL
void         bobgui_header_bar_remove            (BobguiHeaderBar *bar,
                                               BobguiWidget    *child);

GDK_AVAILABLE_IN_ALL
gboolean     bobgui_header_bar_get_show_title_buttons (BobguiHeaderBar *bar);

GDK_AVAILABLE_IN_ALL
void         bobgui_header_bar_set_show_title_buttons (BobguiHeaderBar *bar,
                                                    gboolean      setting);

GDK_AVAILABLE_IN_ALL
void         bobgui_header_bar_set_decoration_layout (BobguiHeaderBar *bar,
                                                   const char   *layout);
GDK_AVAILABLE_IN_ALL
const char *bobgui_header_bar_get_decoration_layout (BobguiHeaderBar *bar);

GDK_AVAILABLE_IN_4_18
gboolean     bobgui_header_bar_get_use_native_controls (BobguiHeaderBar *bar);

GDK_AVAILABLE_IN_4_18
void         bobgui_header_bar_set_use_native_controls (BobguiHeaderBar *bar,
                                                     gboolean      setting);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiHeaderBar, g_object_unref)

G_END_DECLS

