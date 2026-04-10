/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2013 Red Hat, Inc.
 *
 * Authors:
 * - Bastien Nocera <bnocera@redhat.com>
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
 * Modified by the BOBGUI Team and others 2013.  See the AUTHORS
 * file for a list of people on the BOBGUI Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BOBGUI at ftp://ftp.bobgui.org/pub/bobgui/.
 */

#pragma once

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <bobgui/bobguiwidget.h>
#include <bobgui/bobguieditable.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_SEARCH_BAR                 (bobgui_search_bar_get_type ())
#define BOBGUI_SEARCH_BAR(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_SEARCH_BAR, BobguiSearchBar))
#define BOBGUI_IS_SEARCH_BAR(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_SEARCH_BAR))

typedef struct _BobguiSearchBar        BobguiSearchBar;

GDK_AVAILABLE_IN_ALL
GType       bobgui_search_bar_get_type        (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
BobguiWidget*  bobgui_search_bar_new             (void);

GDK_AVAILABLE_IN_ALL
void        bobgui_search_bar_connect_entry   (BobguiSearchBar *bar,
                                            BobguiEditable  *entry);

GDK_AVAILABLE_IN_ALL
gboolean    bobgui_search_bar_get_search_mode (BobguiSearchBar *bar);
GDK_AVAILABLE_IN_ALL
void        bobgui_search_bar_set_search_mode (BobguiSearchBar *bar,
                                            gboolean      search_mode);

GDK_AVAILABLE_IN_ALL
gboolean    bobgui_search_bar_get_show_close_button (BobguiSearchBar *bar);
GDK_AVAILABLE_IN_ALL
void        bobgui_search_bar_set_show_close_button (BobguiSearchBar *bar,
                                                  gboolean      visible);

GDK_AVAILABLE_IN_ALL
void        bobgui_search_bar_set_key_capture_widget (BobguiSearchBar *bar,
                                                   BobguiWidget    *widget);
GDK_AVAILABLE_IN_ALL
BobguiWidget * bobgui_search_bar_get_key_capture_widget (BobguiSearchBar *bar);

GDK_AVAILABLE_IN_ALL
void        bobgui_search_bar_set_child          (BobguiSearchBar *bar,
                                               BobguiWidget    *child);
GDK_AVAILABLE_IN_ALL
BobguiWidget * bobgui_search_bar_get_child          (BobguiSearchBar *bar);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiSearchBar, g_object_unref)

G_END_DECLS

