/* bobguiconstraint.h: Constraint between two widgets
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

#include <bobgui/bobguitypes.h>
#include <bobgui/bobguienums.h>

G_BEGIN_DECLS

typedef struct _BobguiConstraintTarget BobguiConstraintTarget;

#define BOBGUI_TYPE_CONSTRAINT_TARGET (bobgui_constraint_target_get_type ())

/**
 * BobguiConstraintTarget:
 *
 * Makes it possible to use an object as source or target in a
 * [class@Bobgui.Constraint].
 *
 * Besides `BobguiWidget`, it is also implemented by `BobguiConstraintGuide`.
 */

GDK_AVAILABLE_IN_ALL
G_DECLARE_INTERFACE (BobguiConstraintTarget, bobgui_constraint_target, BOBGUI, CONSTRAINT_TARGET, GObject)

#define BOBGUI_TYPE_CONSTRAINT (bobgui_constraint_get_type ())

GDK_AVAILABLE_IN_ALL
G_DECLARE_FINAL_TYPE (BobguiConstraint, bobgui_constraint, BOBGUI, CONSTRAINT, GObject)

GDK_AVAILABLE_IN_ALL
BobguiConstraint *         bobgui_constraint_new                      (gpointer                target,
                                                                 BobguiConstraintAttribute  target_attribute,
                                                                 BobguiConstraintRelation   relation,
                                                                 gpointer                source,
                                                                 BobguiConstraintAttribute  source_attribute,
                                                                 double                  multiplier,
                                                                 double                  constant,
                                                                 int                     strength);
GDK_AVAILABLE_IN_ALL
BobguiConstraint *         bobgui_constraint_new_constant             (gpointer                target,
                                                                 BobguiConstraintAttribute  target_attribute,
                                                                 BobguiConstraintRelation   relation,
                                                                 double                  constant,
                                                                 int                     strength);

GDK_AVAILABLE_IN_ALL
BobguiConstraintTarget *   bobgui_constraint_get_target               (BobguiConstraint          *constraint);
GDK_AVAILABLE_IN_ALL
BobguiConstraintAttribute  bobgui_constraint_get_target_attribute     (BobguiConstraint          *constraint);
GDK_AVAILABLE_IN_ALL
BobguiConstraintTarget *   bobgui_constraint_get_source               (BobguiConstraint          *constraint);
GDK_AVAILABLE_IN_ALL
BobguiConstraintAttribute  bobgui_constraint_get_source_attribute     (BobguiConstraint          *constraint);
GDK_AVAILABLE_IN_ALL
BobguiConstraintRelation   bobgui_constraint_get_relation             (BobguiConstraint          *constraint);
GDK_AVAILABLE_IN_ALL
double                  bobgui_constraint_get_multiplier           (BobguiConstraint          *constraint);
GDK_AVAILABLE_IN_ALL
double                  bobgui_constraint_get_constant             (BobguiConstraint          *constraint);
GDK_AVAILABLE_IN_ALL
int                     bobgui_constraint_get_strength             (BobguiConstraint          *constraint);

GDK_AVAILABLE_IN_ALL
gboolean                bobgui_constraint_is_required              (BobguiConstraint          *constraint);
GDK_AVAILABLE_IN_ALL
gboolean                bobgui_constraint_is_attached              (BobguiConstraint          *constraint);
GDK_AVAILABLE_IN_ALL
gboolean                bobgui_constraint_is_constant              (BobguiConstraint          *constraint);

G_END_DECLS
