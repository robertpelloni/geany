/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2019 Red Hat, Inc.
 *
 * Authors:
 * - MAtthias Clasen <mclasen@redhat.com>
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

#include <bobgui/bobguientry.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_PASSWORD_ENTRY                 (bobgui_password_entry_get_type ())
#define BOBGUI_PASSWORD_ENTRY(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_PASSWORD_ENTRY, BobguiPasswordEntry))
#define BOBGUI_IS_PASSWORD_ENTRY(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_PASSWORD_ENTRY))

typedef struct _BobguiPasswordEntry       BobguiPasswordEntry;
typedef struct _BobguiPasswordEntryClass  BobguiPasswordEntryClass;

GDK_AVAILABLE_IN_ALL
GType           bobgui_password_entry_get_type (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
BobguiWidget *     bobgui_password_entry_new      (void);

GDK_AVAILABLE_IN_ALL
void            bobgui_password_entry_set_show_peek_icon (BobguiPasswordEntry *entry,
                                                       gboolean          show_peek_icon);
GDK_AVAILABLE_IN_ALL
gboolean        bobgui_password_entry_get_show_peek_icon (BobguiPasswordEntry *entry);

GDK_AVAILABLE_IN_ALL
void            bobgui_password_entry_set_extra_menu     (BobguiPasswordEntry *entry,
                                                       GMenuModel       *model);
GDK_AVAILABLE_IN_ALL
GMenuModel *    bobgui_password_entry_get_extra_menu     (BobguiPasswordEntry *entry);

G_END_DECLS

