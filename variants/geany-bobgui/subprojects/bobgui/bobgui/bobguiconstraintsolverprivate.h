/* bobguiconstraintsolverprivate.h: Constraint solver based on the Cassowary method
 * Copyright 2019  GNOME Foundation
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
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Emmanuele Bassi
 */

#pragma once

#include "bobguiconstrainttypesprivate.h"

G_BEGIN_DECLS

#define BOBGUI_TYPE_CONSTRAINT_SOLVER (bobgui_constraint_solver_get_type())

G_DECLARE_FINAL_TYPE (BobguiConstraintSolver, bobgui_constraint_solver, BOBGUI, CONSTRAINT_SOLVER, GObject)

BobguiConstraintSolver *
bobgui_constraint_solver_new (void);

void
bobgui_constraint_solver_freeze (BobguiConstraintSolver *solver);

void
bobgui_constraint_solver_thaw (BobguiConstraintSolver *solver);

void
bobgui_constraint_solver_resolve (BobguiConstraintSolver *solver);

BobguiConstraintVariable *
bobgui_constraint_solver_create_variable (BobguiConstraintSolver *solver,
                                       const char          *prefix,
                                       const char          *name,
                                       double               value);

BobguiConstraintRef *
bobgui_constraint_solver_add_constraint (BobguiConstraintSolver     *solver,
                                      BobguiConstraintVariable   *variable,
                                      BobguiConstraintRelation    relation,
                                      BobguiConstraintExpression *expression,
                                      int                      strength);

void
bobgui_constraint_solver_remove_constraint (BobguiConstraintSolver *solver,
                                         BobguiConstraintRef    *reference);

BobguiConstraintRef *
bobgui_constraint_solver_add_stay_variable (BobguiConstraintSolver   *solver,
                                         BobguiConstraintVariable *variable,
                                         int                    strength);

void
bobgui_constraint_solver_remove_stay_variable (BobguiConstraintSolver   *solver,
                                            BobguiConstraintVariable *variable);

gboolean
bobgui_constraint_solver_has_stay_variable (BobguiConstraintSolver   *solver,
                                         BobguiConstraintVariable *variable);

BobguiConstraintRef *
bobgui_constraint_solver_add_edit_variable (BobguiConstraintSolver   *solver,
                                         BobguiConstraintVariable *variable,
                                         int                    strength);

void
bobgui_constraint_solver_remove_edit_variable (BobguiConstraintSolver   *solver,
                                            BobguiConstraintVariable *variable);

gboolean
bobgui_constraint_solver_has_edit_variable (BobguiConstraintSolver   *solver,
                                         BobguiConstraintVariable *variable);

void
bobgui_constraint_solver_suggest_value (BobguiConstraintSolver   *solver,
                                     BobguiConstraintVariable *variable,
                                     double                 value);

void
bobgui_constraint_solver_begin_edit (BobguiConstraintSolver *solver);

void
bobgui_constraint_solver_end_edit (BobguiConstraintSolver *solver);

void
bobgui_constraint_solver_note_added_variable (BobguiConstraintSolver *self,
                                           BobguiConstraintVariable *variable,
                                           BobguiConstraintVariable *subject);

void
bobgui_constraint_solver_note_removed_variable (BobguiConstraintSolver *self,
                                             BobguiConstraintVariable *variable,
                                             BobguiConstraintVariable *subject);

void
bobgui_constraint_solver_clear (BobguiConstraintSolver *solver);

char *
bobgui_constraint_solver_to_string (BobguiConstraintSolver *solver);

char *
bobgui_constraint_solver_statistics (BobguiConstraintSolver *solver);

G_END_DECLS
