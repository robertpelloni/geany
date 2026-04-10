/* bobguiaccessibleprivate.h: Accessible interface
 *
 * Copyright 2020  GNOME Foundation
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

#include "bobguiaccessible.h"

G_BEGIN_DECLS

/* < private >
 * BobguiAccessiblePlatformChange:
 * @BOBGUI_ACCESSIBLE_PLATFORM_CHANGE_FOCUSABLE: whether the accessible has changed
 *   its focusable state
 * @BOBGUI_ACCESSIBLE_PLATFORM_CHANGE_FOCUSED: whether the accessible has changed its
 *   focused state
 * @BOBGUI_ACCESSIBLE_PLATFORM_CHANGE_ACTIVE: whether the accessible has changed its
 *   active state
 *
 * Represents the various platform changes which can occur and are communicated
 * using [method@Bobgui.Accessible.platform_changed].
 */
typedef enum {
  BOBGUI_ACCESSIBLE_PLATFORM_CHANGE_FOCUSABLE = 1 << BOBGUI_ACCESSIBLE_PLATFORM_STATE_FOCUSABLE,
  BOBGUI_ACCESSIBLE_PLATFORM_CHANGE_FOCUSED   = 1 << BOBGUI_ACCESSIBLE_PLATFORM_STATE_FOCUSED,
  BOBGUI_ACCESSIBLE_PLATFORM_CHANGE_ACTIVE    = 1 << BOBGUI_ACCESSIBLE_PLATFORM_STATE_ACTIVE,
} BobguiAccessiblePlatformChange;

typedef enum {
  BOBGUI_ACCESSIBLE_CHILD_STATE_ADDED,
  BOBGUI_ACCESSIBLE_CHILD_STATE_REMOVED
} BobguiAccessibleChildState;

typedef enum {
  BOBGUI_ACCESSIBLE_CHILD_CHANGE_ADDED   = 1 << BOBGUI_ACCESSIBLE_CHILD_STATE_ADDED,
  BOBGUI_ACCESSIBLE_CHILD_CHANGE_REMOVED = 1 << BOBGUI_ACCESSIBLE_CHILD_STATE_REMOVED
} BobguiAccessibleChildChange;

const char *    bobgui_accessible_role_to_name     (BobguiAccessibleRole  role,
                                                 const char        *domain);

gboolean        bobgui_accessible_role_is_range_subclass (BobguiAccessibleRole role);

gboolean        bobgui_accessible_role_is_subclass       (BobguiAccessibleRole role,
                                                       BobguiAccessibleRole superclass);

gboolean        bobgui_accessible_role_is_abstract       (BobguiAccessibleRole role);

/* < private >
 * BobguiAccessibleNaming:
 * @BOBGUI_ACCESSIBLE_NAME_ALLOWED:
 *   The role allows an accessible name and description
 * @BOBGUI_ACCESSIBLE_NAME_PROHIBITED:
 *   The role does not allow an accessible name and description
 * @BOBGUI_ACCESSIBLE_NAME_REQUIRED:
 *   The role requires an accessible name and description
 * @BOBGUI_ACCESSIBLE_NAME_RECOMMENDED:
 *   It is recommended to set the label property or labelled-by relation
 *   for this role
 * @BOBGUI_ACCESSIBLE_NAME_NOT_RECOMMENDED:
 *   It is recommended not to set the label property or labelled-by relation
 *   for this role
 *
 * Information about naming requirements for accessible roles.
 */
typedef enum {
  BOBGUI_ACCESSIBLE_NAME_ALLOWED,
  BOBGUI_ACCESSIBLE_NAME_PROHIBITED,
  BOBGUI_ACCESSIBLE_NAME_REQUIRED,
  BOBGUI_ACCESSIBLE_NAME_RECOMMENDED,
  BOBGUI_ACCESSIBLE_NAME_NOT_RECOMMENDED,
} BobguiAccessibleNaming;

gboolean bobgui_accessible_role_supports_name_from_author  (BobguiAccessibleRole role) G_GNUC_CONST;
gboolean bobgui_accessible_role_supports_name_from_content (BobguiAccessibleRole role) G_GNUC_CONST;
BobguiAccessibleNaming bobgui_accessible_role_get_naming      (BobguiAccessibleRole role) G_GNUC_CONST;

gboolean        bobgui_accessible_should_present   (BobguiAccessible     *self);

void            bobgui_accessible_update_children  (BobguiAccessible           *self,
                                                 BobguiAccessible           *child,
                                                 BobguiAccessibleChildState  state);

void            bobgui_accessible_bounds_changed   (BobguiAccessible *self);

gboolean        bobgui_accessible_is_password_text (BobguiAccessible *accessible);

G_END_DECLS
