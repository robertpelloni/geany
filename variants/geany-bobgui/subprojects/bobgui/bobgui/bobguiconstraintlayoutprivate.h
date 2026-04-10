/*
 * Copyright 2019 Red Hat, Inc.
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
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Matthias Clasen
 */

#pragma once

#include "bobguiconstraintlayout.h"
#include "bobguiconstraintsolverprivate.h"

G_BEGIN_DECLS

BobguiConstraintSolver *
bobgui_constraint_layout_get_solver (BobguiConstraintLayout *layout);

BobguiConstraintVariable *
bobgui_constraint_layout_get_attribute (BobguiConstraintLayout    *layout,
                                     BobguiConstraintAttribute  attr,
                                     const char             *prefix,
                                     BobguiWidget              *widget,
                                     GHashTable             *bound_attributes);

G_END_DECLS
