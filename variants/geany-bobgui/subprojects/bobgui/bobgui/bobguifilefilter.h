/* BOBGUI - The Bobgui Framework
 * bobguifilefilter.h: Filters for selecting a file subset
 * Copyright (C) 2003, Red Hat, Inc.
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

#include <glib-object.h>
#include <gdk/gdk.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_FILE_FILTER              (bobgui_file_filter_get_type ())
#define BOBGUI_FILE_FILTER(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_FILE_FILTER, BobguiFileFilter))
#define BOBGUI_IS_FILE_FILTER(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_FILE_FILTER))

typedef struct _BobguiFileFilter     BobguiFileFilter;

GDK_AVAILABLE_IN_ALL
GType           bobgui_file_filter_get_type           (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
BobguiFileFilter * bobgui_file_filter_new                (void);
GDK_AVAILABLE_IN_ALL
void            bobgui_file_filter_set_name           (BobguiFileFilter *filter,
                                                    const char    *name);
GDK_AVAILABLE_IN_ALL
const char *    bobgui_file_filter_get_name           (BobguiFileFilter *filter);

GDK_AVAILABLE_IN_ALL
void            bobgui_file_filter_add_mime_type      (BobguiFileFilter *filter,
                                                    const char    *mime_type);

GDK_AVAILABLE_IN_4_22
void            bobgui_file_filter_add_mime_types (BobguiFileFilter  *filter,
                                                const char    **mime_types);

GDK_AVAILABLE_IN_ALL
void            bobgui_file_filter_add_pattern        (BobguiFileFilter *filter,
                                                    const char    *pattern);

GDK_AVAILABLE_IN_4_4
void            bobgui_file_filter_add_suffix         (BobguiFileFilter *filter,
                                                    const char    *suffix);

GDK_DEPRECATED_IN_4_20
void            bobgui_file_filter_add_pixbuf_formats (BobguiFileFilter *filter);

GDK_AVAILABLE_IN_ALL
const char **   bobgui_file_filter_get_attributes     (BobguiFileFilter *filter);

GDK_AVAILABLE_IN_ALL
GVariant      * bobgui_file_filter_to_gvariant        (BobguiFileFilter *filter);
GDK_AVAILABLE_IN_ALL
BobguiFileFilter * bobgui_file_filter_new_from_gvariant  (GVariant      *variant);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiFileFilter, g_object_unref)

G_END_DECLS

