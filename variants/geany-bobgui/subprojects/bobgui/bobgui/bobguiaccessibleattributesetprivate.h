/* bobguiaccessibleattributesetprivate.h: Accessible attribute container
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
#include "bobguiaccessiblevalueprivate.h"

G_BEGIN_DECLS

typedef struct _BobguiAccessibleAttributeSet       BobguiAccessibleAttributeSet;

typedef const char *(* BobguiAccessibleAttributeNameFunc) (int attribute);
typedef BobguiAccessibleValue *(* BobguiAccessibleAttributeDefaultFunc) (int attribute);

BobguiAccessibleAttributeSet *     bobgui_accessible_attribute_set_new                (gsize                             n_attributes,
                                                                                 BobguiAccessibleAttributeNameFunc    name_func,
                                                                                 BobguiAccessibleAttributeDefaultFunc default_func);
BobguiAccessibleAttributeSet *     bobgui_accessible_attribute_set_ref                (BobguiAccessibleAttributeSet  *self);
void                            bobgui_accessible_attribute_set_unref              (BobguiAccessibleAttributeSet  *self);

gsize                           bobgui_accessible_attribute_set_get_length         (BobguiAccessibleAttributeSet  *self);

gboolean                        bobgui_accessible_attribute_set_add                (BobguiAccessibleAttributeSet  *self,
                                                                                 int                         attribute,
                                                                                 BobguiAccessibleValue         *value);
gboolean                        bobgui_accessible_attribute_set_remove             (BobguiAccessibleAttributeSet  *self,
                                                                                 int                         state);
gboolean                        bobgui_accessible_attribute_set_contains           (BobguiAccessibleAttributeSet  *self,
                                                                                 int                         state);
BobguiAccessibleValue *            bobgui_accessible_attribute_set_get_value          (BobguiAccessibleAttributeSet  *self,
                                                                                 int                         state);

guint                           bobgui_accessible_attribute_set_get_changed        (BobguiAccessibleAttributeSet   *self);

void                            bobgui_accessible_attribute_set_print              (BobguiAccessibleAttributeSet  *self,
                                                                                 gboolean                    only_set,
                                                                                 GString                    *string);
char *                          bobgui_accessible_attribute_set_to_string          (BobguiAccessibleAttributeSet  *self);

G_END_DECLS
