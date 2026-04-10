/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2012 Red Hat, Inc.
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
 * Modified by the BOBGUI Team and others 2012.  See the AUTHORS
 * file for a list of people on the BOBGUI Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BOBGUI at ftp://ftp.bobgui.org/pub/bobgui/.
 */

#pragma once

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <bobgui/bobguientry.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_SEARCH_ENTRY                 (bobgui_search_entry_get_type ())
#define BOBGUI_SEARCH_ENTRY(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_SEARCH_ENTRY, BobguiSearchEntry))
#define BOBGUI_IS_SEARCH_ENTRY(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_SEARCH_ENTRY))

typedef struct _BobguiSearchEntry       BobguiSearchEntry;

GDK_AVAILABLE_IN_ALL
GType           bobgui_search_entry_get_type       (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
BobguiWidget*      bobgui_search_entry_new            (void);

GDK_AVAILABLE_IN_ALL
void            bobgui_search_entry_set_key_capture_widget (BobguiSearchEntry *entry,
                                                         BobguiWidget      *widget);
GDK_AVAILABLE_IN_ALL
BobguiWidget*      bobgui_search_entry_get_key_capture_widget (BobguiSearchEntry *entry);

GDK_AVAILABLE_IN_4_8
void bobgui_search_entry_set_search_delay (BobguiSearchEntry *entry,
                                        guint delay);
GDK_AVAILABLE_IN_4_8
guint bobgui_search_entry_get_search_delay (BobguiSearchEntry *entry);

GDK_AVAILABLE_IN_4_10
void            bobgui_search_entry_set_placeholder_text (BobguiSearchEntry *entry,
                                                       const char     *text);
GDK_AVAILABLE_IN_4_10
const char *    bobgui_search_entry_get_placeholder_text (BobguiSearchEntry *entry);

GDK_AVAILABLE_IN_4_14
void            bobgui_search_entry_set_input_purpose (BobguiSearchEntry  *entry,
                                                    BobguiInputPurpose  purpose);
GDK_AVAILABLE_IN_4_14
BobguiInputPurpose bobgui_search_entry_get_input_purpose (BobguiSearchEntry *entry);

GDK_AVAILABLE_IN_4_14
void            bobgui_search_entry_set_input_hints (BobguiSearchEntry *entry,
                                                  BobguiInputHints   hints);
GDK_AVAILABLE_IN_4_14
BobguiInputHints   bobgui_search_entry_get_input_hints (BobguiSearchEntry *entry);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiSearchEntry, g_object_unref)

G_END_DECLS

