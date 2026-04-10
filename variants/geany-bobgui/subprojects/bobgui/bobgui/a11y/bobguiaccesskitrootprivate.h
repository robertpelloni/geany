/* bobguiaccesskitrootprivate.h: AccessKit root object
 *
 * Copyright 2024  GNOME Foundation
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "bobguiaccesskitcontextprivate.h"
#include "bobguiroot.h"

#include <accesskit.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_ACCESSKIT_ROOT (bobgui_accesskit_root_get_type())

G_DECLARE_FINAL_TYPE (BobguiAccessKitRoot, bobgui_accesskit_root, BOBGUI, ACCESSKIT_ROOT, GObject)

BobguiAccessKitRoot *
bobgui_accesskit_root_new (BobguiRoot *root_widget);

guint32
bobgui_accesskit_root_new_id (BobguiAccessKitRoot *self);

guint32
bobgui_accesskit_root_add_context (BobguiAccessKitRoot    *self,
                                BobguiAccessKitContext *context);

void
bobgui_accesskit_root_remove_context (BobguiAccessKitRoot *self, guint32 id);

void
bobgui_accesskit_root_queue_update (BobguiAccessKitRoot *self,
                                 guint32           id,
                                 gboolean          force_to_end);

void
bobgui_accesskit_root_update_tree (BobguiAccessKitRoot *self);

void
bobgui_accesskit_root_update_window_focus_state (BobguiAccessKitRoot *self,
                                              gboolean          focused);

G_END_DECLS
