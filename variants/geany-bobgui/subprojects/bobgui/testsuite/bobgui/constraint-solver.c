#include <locale.h>

#include <bobgui/bobgui.h>
#include "../../bobgui/bobguiconstrainttypesprivate.h"
#include "../../bobgui/bobguiconstraintsolverprivate.h"
#include "../../bobgui/bobguiconstraintexpressionprivate.h"

static void
constraint_solver_simple (void)
{
  BobguiConstraintSolver *solver = bobgui_constraint_solver_new ();

  BobguiConstraintVariable *x = bobgui_constraint_solver_create_variable (solver, NULL, "x", 167.0);
  BobguiConstraintVariable *y = bobgui_constraint_solver_create_variable (solver, NULL, "y", 2.0);

  BobguiConstraintExpression *e = bobgui_constraint_expression_new_from_variable (y);

  bobgui_constraint_solver_add_constraint (solver,
                                        x, BOBGUI_CONSTRAINT_RELATION_EQ, e,
                                        BOBGUI_CONSTRAINT_STRENGTH_REQUIRED);

  double x_value = bobgui_constraint_variable_get_value (x);
  double y_value = bobgui_constraint_variable_get_value (y);

  g_assert_cmpfloat_with_epsilon (x_value, y_value, 0.001);
  g_assert_cmpfloat_with_epsilon (x_value, 0.0, 0.001);
  g_assert_cmpfloat_with_epsilon (y_value, 0.0, 0.001);

  bobgui_constraint_variable_unref (y);
  bobgui_constraint_variable_unref (x);

  g_object_unref (solver);
}

static void
constraint_solver_stay (void)
{
  BobguiConstraintSolver *solver = bobgui_constraint_solver_new ();

  BobguiConstraintVariable *x = bobgui_constraint_solver_create_variable (solver, NULL, "x", 5.0);
  BobguiConstraintVariable *y = bobgui_constraint_solver_create_variable (solver, NULL, "y", 10.0);

  bobgui_constraint_solver_add_stay_variable (solver, x, BOBGUI_CONSTRAINT_STRENGTH_WEAK);
  bobgui_constraint_solver_add_stay_variable (solver, y, BOBGUI_CONSTRAINT_STRENGTH_WEAK);

  double x_value = bobgui_constraint_variable_get_value (x);
  double y_value = bobgui_constraint_variable_get_value (y);

  g_assert_cmpfloat_with_epsilon (x_value, 5.0, 0.001);
  g_assert_cmpfloat_with_epsilon (y_value, 10.0, 0.001);

  bobgui_constraint_variable_unref (x);
  bobgui_constraint_variable_unref (y);

  g_object_unref (solver);
}

static void
constraint_solver_variable_geq_constant (void)
{
  BobguiConstraintSolver *solver = bobgui_constraint_solver_new ();

  BobguiConstraintVariable *x = bobgui_constraint_solver_create_variable (solver, NULL, "x", 10.0);
  BobguiConstraintExpression *e = bobgui_constraint_expression_new (100.0);

  bobgui_constraint_solver_add_constraint (solver,
                                        x, BOBGUI_CONSTRAINT_RELATION_GE, e,
                                        BOBGUI_CONSTRAINT_STRENGTH_REQUIRED);

  double x_value = bobgui_constraint_variable_get_value (x);

  g_assert_cmpfloat (x_value, >=, 100.0);

  bobgui_constraint_variable_unref (x);

  g_object_unref (solver);
}

static void
constraint_solver_variable_leq_constant (void)
{
  BobguiConstraintSolver *solver = bobgui_constraint_solver_new ();

  BobguiConstraintVariable *x = bobgui_constraint_solver_create_variable (solver, NULL, "x", 100.0);
  BobguiConstraintExpression *e = bobgui_constraint_expression_new (10.0);

  bobgui_constraint_solver_add_constraint (solver,
                                        x, BOBGUI_CONSTRAINT_RELATION_LE, e,
                                        BOBGUI_CONSTRAINT_STRENGTH_REQUIRED);

  double x_value = bobgui_constraint_variable_get_value (x);

  g_assert_cmpfloat (x_value, <=, 10.0);

  bobgui_constraint_variable_unref (x);

  g_object_unref (solver);
}

static void
constraint_solver_variable_eq_constant (void)
{
  BobguiConstraintSolver *solver = bobgui_constraint_solver_new ();

  BobguiConstraintVariable *x = bobgui_constraint_solver_create_variable (solver, NULL, "x", 10.0);
  BobguiConstraintExpression *e = bobgui_constraint_expression_new (100.0);

  bobgui_constraint_solver_add_constraint (solver,
                                        x, BOBGUI_CONSTRAINT_RELATION_EQ, e,
                                        BOBGUI_CONSTRAINT_STRENGTH_REQUIRED);

  double x_value = bobgui_constraint_variable_get_value (x);

  g_assert_cmpfloat_with_epsilon (x_value, 100.0, 0.001);

  bobgui_constraint_variable_unref (x);

  g_object_unref (solver);
}

static void
constraint_solver_eq_with_stay (void)
{
  BobguiConstraintSolver *solver = bobgui_constraint_solver_new ();

  BobguiConstraintVariable *x = bobgui_constraint_solver_create_variable (solver, NULL, "x", 10.0);
  BobguiConstraintVariable *width = bobgui_constraint_solver_create_variable (solver, NULL, "width", 10.0);
  BobguiConstraintVariable *right_min = bobgui_constraint_solver_create_variable (solver, NULL, "rightMin", 100.0);

  BobguiConstraintExpressionBuilder builder;
  bobgui_constraint_expression_builder_init (&builder, solver);
  bobgui_constraint_expression_builder_term (&builder, x);
  bobgui_constraint_expression_builder_plus (&builder);
  bobgui_constraint_expression_builder_term (&builder, width);
  BobguiConstraintExpression *right = bobgui_constraint_expression_builder_finish (&builder);

  bobgui_constraint_solver_add_stay_variable (solver, width, BOBGUI_CONSTRAINT_STRENGTH_WEAK);
  bobgui_constraint_solver_add_stay_variable (solver, right_min, BOBGUI_CONSTRAINT_STRENGTH_WEAK);
  bobgui_constraint_solver_add_constraint (solver,
                                        right_min, BOBGUI_CONSTRAINT_RELATION_EQ, right,
                                        BOBGUI_CONSTRAINT_STRENGTH_REQUIRED);

  double x_value = bobgui_constraint_variable_get_value (x);
  double width_value = bobgui_constraint_variable_get_value (width);

  g_assert_cmpfloat_with_epsilon (x_value, 90.0, 0.001);
  g_assert_cmpfloat_with_epsilon (width_value, 10.0, 0.001);

  bobgui_constraint_variable_unref (right_min);
  bobgui_constraint_variable_unref (width);
  bobgui_constraint_variable_unref (x);

  g_object_unref (solver);
}

static void
constraint_solver_cassowary (void)
{
  BobguiConstraintSolver *solver = bobgui_constraint_solver_new ();

  BobguiConstraintVariable *x = bobgui_constraint_solver_create_variable (solver, NULL, "x", 0.0);
  BobguiConstraintVariable *y = bobgui_constraint_solver_create_variable (solver, NULL, "y", 0.0);

  BobguiConstraintExpression *e;

  e = bobgui_constraint_expression_new_from_variable (y);
  bobgui_constraint_solver_add_constraint (solver,
                                        x, BOBGUI_CONSTRAINT_RELATION_LE, e,
                                        BOBGUI_CONSTRAINT_STRENGTH_REQUIRED);

  e = bobgui_constraint_expression_plus_constant (bobgui_constraint_expression_new_from_variable (x), 3.0);
  bobgui_constraint_solver_add_constraint (solver,
                                        y, BOBGUI_CONSTRAINT_RELATION_EQ, e,
                                        BOBGUI_CONSTRAINT_STRENGTH_REQUIRED);

  e = bobgui_constraint_expression_new (10.0);
  bobgui_constraint_solver_add_constraint (solver,
                                        x, BOBGUI_CONSTRAINT_RELATION_EQ, e,
                                        BOBGUI_CONSTRAINT_STRENGTH_WEAK);

  e = bobgui_constraint_expression_new (10.0);
  bobgui_constraint_solver_add_constraint (solver,
                                        y, BOBGUI_CONSTRAINT_RELATION_EQ, e,
                                        BOBGUI_CONSTRAINT_STRENGTH_WEAK);

  double x_val = bobgui_constraint_variable_get_value (x);
  double y_val = bobgui_constraint_variable_get_value (y);

  g_test_message ("x = %g, y = %g", x_val, y_val);

  /* The system is unstable and has two possible solutions we need to test */
  g_assert_true ((G_APPROX_VALUE (x_val, 10.0, 1e-8) &&
                  G_APPROX_VALUE (y_val, 13.0, 1e-8)) ||
                 (G_APPROX_VALUE (x_val,  7.0, 1e-8) &&
                  G_APPROX_VALUE (y_val, 10.0, 1e-8)));

  bobgui_constraint_variable_unref (x);
  bobgui_constraint_variable_unref (y);

  g_object_unref (solver);
}

static void
constraint_solver_edit_var_required (void)
{
  BobguiConstraintSolver *solver = bobgui_constraint_solver_new ();

  BobguiConstraintVariable *a = bobgui_constraint_solver_create_variable (solver, NULL, "a", 0.0);
  bobgui_constraint_solver_add_stay_variable (solver, a, BOBGUI_CONSTRAINT_STRENGTH_STRONG);

  g_assert_cmpfloat_with_epsilon (bobgui_constraint_variable_get_value (a), 0.0, 0.001);

  bobgui_constraint_solver_add_edit_variable (solver, a, BOBGUI_CONSTRAINT_STRENGTH_REQUIRED);
  bobgui_constraint_solver_begin_edit (solver);
  bobgui_constraint_solver_suggest_value (solver, a, 2.0);
  bobgui_constraint_solver_resolve (solver);

  g_assert_cmpfloat_with_epsilon (bobgui_constraint_variable_get_value (a), 2.0, 0.001);

  bobgui_constraint_solver_suggest_value (solver, a, 10.0);
  bobgui_constraint_solver_resolve (solver);

  g_assert_cmpfloat_with_epsilon (bobgui_constraint_variable_get_value (a), 10.0, 0.001);

  bobgui_constraint_solver_end_edit (solver);

  bobgui_constraint_variable_unref (a);

  g_object_unref (solver);
}

static void
constraint_solver_edit_var_suggest (void)
{
  BobguiConstraintSolver *solver = bobgui_constraint_solver_new ();

  BobguiConstraintVariable *a = bobgui_constraint_solver_create_variable (solver, NULL, "a", 0.0);
  BobguiConstraintVariable *b = bobgui_constraint_solver_create_variable (solver, NULL, "b", 0.0);

  bobgui_constraint_solver_add_stay_variable (solver, a, BOBGUI_CONSTRAINT_STRENGTH_STRONG);

  BobguiConstraintExpression *e = bobgui_constraint_expression_new_from_variable (b);
  bobgui_constraint_solver_add_constraint (solver,
                                        a, BOBGUI_CONSTRAINT_RELATION_EQ, e,
                                        BOBGUI_CONSTRAINT_STRENGTH_REQUIRED);

  bobgui_constraint_solver_resolve (solver);

  g_assert_cmpfloat_with_epsilon (bobgui_constraint_variable_get_value (a), 0.0, 0.001);
  g_assert_cmpfloat_with_epsilon (bobgui_constraint_variable_get_value (b), 0.0, 0.001);

  bobgui_constraint_solver_add_edit_variable (solver, a, BOBGUI_CONSTRAINT_STRENGTH_REQUIRED);
  bobgui_constraint_solver_begin_edit (solver);

  bobgui_constraint_solver_suggest_value (solver, a, 2.0);
  bobgui_constraint_solver_resolve (solver);

  g_test_message ("Check values after first edit");

  g_assert_cmpfloat_with_epsilon (bobgui_constraint_variable_get_value (a), 2.0, 0.001);
  g_assert_cmpfloat_with_epsilon (bobgui_constraint_variable_get_value (b), 2.0, 0.001);

  bobgui_constraint_solver_suggest_value (solver, a, 10.0);
  bobgui_constraint_solver_resolve (solver);

  g_test_message ("Check values after second edit");

  g_assert_cmpfloat_with_epsilon (bobgui_constraint_variable_get_value (a), 10.0, 0.001);
  g_assert_cmpfloat_with_epsilon (bobgui_constraint_variable_get_value (b), 10.0, 0.001);

  bobgui_constraint_solver_suggest_value (solver, a, 12.0);
  bobgui_constraint_solver_resolve (solver);

  g_test_message ("Check values after third edit");

  g_assert_cmpfloat_with_epsilon (bobgui_constraint_variable_get_value (a), 12.0, 0.001);
  g_assert_cmpfloat_with_epsilon (bobgui_constraint_variable_get_value (b), 12.0, 0.001);

  bobgui_constraint_variable_unref (a);
  bobgui_constraint_variable_unref (b);

  g_object_unref (solver);
}

static void
constraint_solver_paper (void)
{
  BobguiConstraintSolver *solver = bobgui_constraint_solver_new ();

  BobguiConstraintVariable *left = bobgui_constraint_solver_create_variable (solver, NULL, "left", 0.0);
  BobguiConstraintVariable *middle = bobgui_constraint_solver_create_variable (solver, NULL, "middle", 0.0);
  BobguiConstraintVariable *right = bobgui_constraint_solver_create_variable (solver, NULL, "right", 0.0);

  BobguiConstraintExpressionBuilder builder;
  BobguiConstraintExpression *expr;

  bobgui_constraint_expression_builder_init (&builder, solver);
  bobgui_constraint_expression_builder_term (&builder, left);
  bobgui_constraint_expression_builder_plus (&builder);
  bobgui_constraint_expression_builder_term (&builder, right);
  bobgui_constraint_expression_builder_divide_by (&builder);
  bobgui_constraint_expression_builder_constant (&builder, 2.0);
  expr = bobgui_constraint_expression_builder_finish (&builder);
  bobgui_constraint_solver_add_constraint (solver,
                                        middle, BOBGUI_CONSTRAINT_RELATION_EQ, expr,
                                        BOBGUI_CONSTRAINT_STRENGTH_REQUIRED);

  bobgui_constraint_expression_builder_init (&builder, solver);
  bobgui_constraint_expression_builder_term (&builder, left);
  bobgui_constraint_expression_builder_plus (&builder);
  bobgui_constraint_expression_builder_constant (&builder, 10.0);
  expr = bobgui_constraint_expression_builder_finish (&builder);
  bobgui_constraint_solver_add_constraint (solver,
                                        right, BOBGUI_CONSTRAINT_RELATION_EQ, expr,
                                        BOBGUI_CONSTRAINT_STRENGTH_REQUIRED);

  expr = bobgui_constraint_expression_new (100.0);
  bobgui_constraint_solver_add_constraint (solver,
                                        right, BOBGUI_CONSTRAINT_RELATION_LE, expr,
                                        BOBGUI_CONSTRAINT_STRENGTH_REQUIRED);

  expr = bobgui_constraint_expression_new (0.0);
  bobgui_constraint_solver_add_constraint (solver,
                                        left, BOBGUI_CONSTRAINT_RELATION_GE, expr,
                                        BOBGUI_CONSTRAINT_STRENGTH_REQUIRED);

  g_test_message ("Check constraints hold");

  g_assert_cmpfloat_with_epsilon (bobgui_constraint_variable_get_value (middle),
                                  (bobgui_constraint_variable_get_value (left) + bobgui_constraint_variable_get_value (right)) / 2.0,
                                  0.001);
  g_assert_cmpfloat_with_epsilon (bobgui_constraint_variable_get_value (right),
                                  bobgui_constraint_variable_get_value (left) + 10.0,
                                  0.001);
  g_assert_cmpfloat (bobgui_constraint_variable_get_value (right), <=, 100.0);
  g_assert_cmpfloat (bobgui_constraint_variable_get_value (left), >=, 0.0);

  bobgui_constraint_variable_set_value (middle, 45.0);
  bobgui_constraint_solver_add_stay_variable (solver, middle, BOBGUI_CONSTRAINT_STRENGTH_WEAK);

  g_test_message ("Check constraints hold after setting middle");

  g_assert_cmpfloat_with_epsilon (bobgui_constraint_variable_get_value (middle),
                                  (bobgui_constraint_variable_get_value (left) + bobgui_constraint_variable_get_value (right)) / 2.0,
                                  0.001);
  g_assert_cmpfloat_with_epsilon (bobgui_constraint_variable_get_value (right),
                                  bobgui_constraint_variable_get_value (left) + 10.0,
                                  0.001);
  g_assert_cmpfloat (bobgui_constraint_variable_get_value (right), <=, 100.0);
  g_assert_cmpfloat (bobgui_constraint_variable_get_value (left), >=, 0.0);

  g_assert_cmpfloat_with_epsilon (bobgui_constraint_variable_get_value (left), 40.0, 0.001);
  g_assert_cmpfloat_with_epsilon (bobgui_constraint_variable_get_value (middle), 45.0, 0.001);
  g_assert_cmpfloat_with_epsilon (bobgui_constraint_variable_get_value (right), 50.0, 0.001);

  bobgui_constraint_variable_unref (left);
  bobgui_constraint_variable_unref (middle);
  bobgui_constraint_variable_unref (right);

  g_object_unref (solver);
}

int
main (int argc, char *argv[])
{
  (g_test_init) (&argc, &argv, NULL);
  setlocale (LC_ALL, "C");

  g_test_add_func ("/constraint-solver/paper", constraint_solver_paper);
  g_test_add_func ("/constraint-solver/simple", constraint_solver_simple);
  g_test_add_func ("/constraint-solver/constant/eq", constraint_solver_variable_eq_constant);
  g_test_add_func ("/constraint-solver/constant/ge", constraint_solver_variable_geq_constant);
  g_test_add_func ("/constraint-solver/constant/le", constraint_solver_variable_leq_constant);
  g_test_add_func ("/constraint-solver/stay/simple", constraint_solver_stay);
  g_test_add_func ("/constraint-solver/stay/eq", constraint_solver_eq_with_stay);
  g_test_add_func ("/constraint-solver/cassowary", constraint_solver_cassowary);
  g_test_add_func ("/constraint-solver/edit/required", constraint_solver_edit_var_required);
  g_test_add_func ("/constraint-solver/edit/suggest", constraint_solver_edit_var_suggest);

  return g_test_run ();
}
