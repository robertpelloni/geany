/* bobguiconstraintequationprivate.h: Constraint expressions and variables
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

BobguiConstraintVariable *
bobgui_constraint_variable_new (const char *prefix,
                             const char *name);

BobguiConstraintVariable *
bobgui_constraint_variable_new_dummy (const char *name);

BobguiConstraintVariable *
bobgui_constraint_variable_new_objective (const char *name);

BobguiConstraintVariable *
bobgui_constraint_variable_new_slack (const char *name);

BobguiConstraintVariable *
bobgui_constraint_variable_ref (BobguiConstraintVariable *variable);

void
bobgui_constraint_variable_unref (BobguiConstraintVariable *variable);

void
bobgui_constraint_variable_set_value (BobguiConstraintVariable *variable,
                                   double value);

double
bobgui_constraint_variable_get_value (const BobguiConstraintVariable *variable);

char *
bobgui_constraint_variable_to_string (const BobguiConstraintVariable *variable);

gboolean
bobgui_constraint_variable_is_external (const BobguiConstraintVariable *variable);

gboolean
bobgui_constraint_variable_is_pivotable (const BobguiConstraintVariable *variable);

gboolean
bobgui_constraint_variable_is_restricted (const BobguiConstraintVariable *variable);

gboolean
bobgui_constraint_variable_is_dummy (const BobguiConstraintVariable *variable);

typedef struct {
  BobguiConstraintVariable *first;
  BobguiConstraintVariable *second;
} BobguiConstraintVariablePair;

BobguiConstraintVariablePair *
bobgui_constraint_variable_pair_new (BobguiConstraintVariable *first,
                                  BobguiConstraintVariable *second);

void
bobgui_constraint_variable_pair_free (BobguiConstraintVariablePair *pair);

typedef struct _BobguiConstraintVariableSet        BobguiConstraintVariableSet;

BobguiConstraintVariableSet *
bobgui_constraint_variable_set_new (void);

void
bobgui_constraint_variable_set_free (BobguiConstraintVariableSet *set);

gboolean
bobgui_constraint_variable_set_add (BobguiConstraintVariableSet *set,
                                 BobguiConstraintVariable *variable);

gboolean
bobgui_constraint_variable_set_remove (BobguiConstraintVariableSet *set,
                                    BobguiConstraintVariable *variable);

gboolean
bobgui_constraint_variable_set_is_empty (BobguiConstraintVariableSet *set);

gboolean
bobgui_constraint_variable_set_is_singleton (BobguiConstraintVariableSet *set);

int
bobgui_constraint_variable_set_size (BobguiConstraintVariableSet *set);

typedef struct {
  /*< private >*/
  gpointer dummy1;
  gpointer dummy2;
  gint64 dummy3;
} BobguiConstraintVariableSetIter;

void
bobgui_constraint_variable_set_iter_init (BobguiConstraintVariableSetIter *iter,
                                       BobguiConstraintVariableSet *set);

gboolean
bobgui_constraint_variable_set_iter_next (BobguiConstraintVariableSetIter *iter,
                                       BobguiConstraintVariable **variable_p);

BobguiConstraintExpression *
bobgui_constraint_expression_new (double constant);

BobguiConstraintExpression *
bobgui_constraint_expression_new_from_variable (BobguiConstraintVariable *variable);

BobguiConstraintExpression *
bobgui_constraint_expression_ref (BobguiConstraintExpression *expression);

void
bobgui_constraint_expression_unref (BobguiConstraintExpression *expression);

BobguiConstraintExpression *
bobgui_constraint_expression_clone (BobguiConstraintExpression *expression);

void
bobgui_constraint_expression_set_constant (BobguiConstraintExpression *expression,
                                        double constant);
double
bobgui_constraint_expression_get_constant (const BobguiConstraintExpression *expression);

gboolean
bobgui_constraint_expression_is_constant (const BobguiConstraintExpression *expression);

void
bobgui_constraint_expression_add_expression (BobguiConstraintExpression *a_expr,
                                          BobguiConstraintExpression *b_expr,
                                          double n,
                                          BobguiConstraintVariable *subject,
                                          BobguiConstraintSolver *solver);

void
bobgui_constraint_expression_add_variable (BobguiConstraintExpression *expression,
                                        BobguiConstraintVariable *variable,
                                        double coefficient,
                                        BobguiConstraintVariable *subject,
                                        BobguiConstraintSolver *solver);

void
bobgui_constraint_expression_remove_variable (BobguiConstraintExpression *expression,
                                           BobguiConstraintVariable *variable);

void
bobgui_constraint_expression_set_variable (BobguiConstraintExpression *expression,
                                        BobguiConstraintVariable *variable,
                                        double coefficient);

double
bobgui_constraint_expression_get_coefficient (BobguiConstraintExpression *expression,
                                           BobguiConstraintVariable *variable);

char *
bobgui_constraint_expression_to_string (const BobguiConstraintExpression *expression);

double
bobgui_constraint_expression_new_subject (BobguiConstraintExpression *expression,
                                       BobguiConstraintVariable *subject);

void
bobgui_constraint_expression_change_subject (BobguiConstraintExpression *expression,
                                          BobguiConstraintVariable *old_subject,
                                          BobguiConstraintVariable *new_subject);

void
bobgui_constraint_expression_substitute_out (BobguiConstraintExpression *expression,
                                          BobguiConstraintVariable *out_var,
                                          BobguiConstraintExpression *expr,
                                          BobguiConstraintVariable *subject,
                                          BobguiConstraintSolver *solver);

BobguiConstraintVariable *
bobgui_constraint_expression_get_pivotable_variable (BobguiConstraintExpression *expression);

BobguiConstraintExpression *
bobgui_constraint_expression_plus_constant (BobguiConstraintExpression *expression,
                                         double constant);

BobguiConstraintExpression *
bobgui_constraint_expression_minus_constant (BobguiConstraintExpression *expression,
                                          double constant);

BobguiConstraintExpression *
bobgui_constraint_expression_plus_variable (BobguiConstraintExpression *expression,
                                         BobguiConstraintVariable *variable);

BobguiConstraintExpression *
bobgui_constraint_expression_minus_variable (BobguiConstraintExpression *expression,
                                          BobguiConstraintVariable *variable);

BobguiConstraintExpression *
bobgui_constraint_expression_multiply_by (BobguiConstraintExpression *expression,
                                       double factor);

BobguiConstraintExpression *
bobgui_constraint_expression_divide_by (BobguiConstraintExpression *expression,
                                     double factor);

struct _BobguiConstraintExpressionBuilder
{
  /*< private >*/
  gpointer dummy1;
  gpointer dummy2;
  int dummy3;
};

void
bobgui_constraint_expression_builder_init (BobguiConstraintExpressionBuilder *builder,
                                        BobguiConstraintSolver *solver);

void
bobgui_constraint_expression_builder_term (BobguiConstraintExpressionBuilder *builder,
                                        BobguiConstraintVariable *term);

void
bobgui_constraint_expression_builder_plus (BobguiConstraintExpressionBuilder *builder);

void
bobgui_constraint_expression_builder_minus (BobguiConstraintExpressionBuilder *builder);

void
bobgui_constraint_expression_builder_divide_by (BobguiConstraintExpressionBuilder *builder);

void
bobgui_constraint_expression_builder_multiply_by (BobguiConstraintExpressionBuilder *builder);

void
bobgui_constraint_expression_builder_constant (BobguiConstraintExpressionBuilder *builder,
                                            double value);

BobguiConstraintExpression *
bobgui_constraint_expression_builder_finish (BobguiConstraintExpressionBuilder *builder) G_GNUC_WARN_UNUSED_RESULT;

/*< private >
 * BobguiConstraintExpressionIter:
 *
 * An iterator object for terms inside a `BobguiConstraintExpression`.
 */
typedef struct {
  /*< private >*/
  gpointer dummy1;
  gpointer dummy2;
  gint64 dummy3;
} BobguiConstraintExpressionIter;

void
bobgui_constraint_expression_iter_init (BobguiConstraintExpressionIter *iter,
                                     BobguiConstraintExpression *equation);

gboolean
bobgui_constraint_expression_iter_next (BobguiConstraintExpressionIter *iter,
                                     BobguiConstraintVariable **variable,
                                     double *coefficient);

gboolean
bobgui_constraint_expression_iter_prev (BobguiConstraintExpressionIter *iter,
                                     BobguiConstraintVariable **variable,
                                     double *coefficient);

G_END_DECLS
