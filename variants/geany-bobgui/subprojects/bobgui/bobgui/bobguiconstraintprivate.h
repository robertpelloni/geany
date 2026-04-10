/* bobguiconstraintprivate.h: Constraint between two widgets
 * Copyright 2019  GNOME Foundation
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
 * Author: Emmanuele Bassi
 */

#pragma once

#include "bobguiconstraint.h"
#include "bobguiconstrainttypesprivate.h"

G_BEGIN_DECLS

struct _BobguiConstraint
{
  GObject parent_instance;

  BobguiConstraintAttribute target_attribute;
  BobguiConstraintAttribute source_attribute;

  BobguiConstraintTarget *target;
  BobguiConstraintTarget *source;

  BobguiConstraintRelation relation;

  double multiplier;
  double constant;

  int strength;

  /* A reference to the real constraint inside the
   * BobguiConstraintSolver, so we can remove it when
   * finalizing the BobguiConstraint instance
   */
  BobguiConstraintRef *constraint_ref;

  BobguiConstraintSolver *solver;

  guint active : 1;
};

void    bobgui_constraint_attach           (BobguiConstraint       *constraint,
                                         BobguiConstraintSolver *solver,
                                         BobguiConstraintRef    *ref);
void    bobgui_constraint_detach           (BobguiConstraint       *constraint);

G_END_DECLS
