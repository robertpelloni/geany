/* bobguiconstraintguideprivate.h: Constraint between two widgets
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

#include "bobguiconstraintguide.h"
#include "bobguiconstraintlayout.h"
#include "bobguiconstrainttypesprivate.h"

G_BEGIN_DECLS

void                   bobgui_constraint_guide_update        (BobguiConstraintGuide     *guide);
void                   bobgui_constraint_guide_detach        (BobguiConstraintGuide     *guide);

BobguiConstraintVariable *bobgui_constraint_guide_get_attribute (BobguiConstraintGuide      *guide,
                                                           BobguiConstraintAttribute  attr);

BobguiConstraintLayout   *bobgui_constraint_guide_get_layout    (BobguiConstraintGuide     *guide);
void                   bobgui_constraint_guide_set_layout    (BobguiConstraintGuide     *guide,
                                                           BobguiConstraintLayout    *layout);

G_END_DECLS
