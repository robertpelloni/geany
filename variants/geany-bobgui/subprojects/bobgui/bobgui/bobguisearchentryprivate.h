/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2014 Red Hat, Inc.
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

#include <bobgui/bobguisearchentry.h>
#include <bobgui/bobguitext.h>

G_BEGIN_DECLS

gboolean bobgui_search_entry_is_keynav (guint           keyval,
                                     GdkModifierType state);

BobguiText *bobgui_search_entry_get_text_widget (BobguiSearchEntry *entry);
BobguiEventController * bobgui_search_entry_get_key_controller (BobguiSearchEntry *entry);

G_END_DECLS

