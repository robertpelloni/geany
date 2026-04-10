/* Copyright (C) 2019 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */
#include <bobgui/bobgui.h>

#define BOBGUI_TYPE_GIZMO                 (bobgui_gizmo_get_type ())
#define BOBGUI_GIZMO(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_GIZMO, BobguiGizmo))
#define BOBGUI_GIZMO_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), BOBGUI_TYPE_GIZMO, BobguiGizmoClass))
#define BOBGUI_IS_GIZMO(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_GIZMO))
#define BOBGUI_IS_GIZMO_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), BOBGUI_TYPE_GIZMO))
#define BOBGUI_GIZMO_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_GIZMO, BobguiGizmoClass))

typedef struct _BobguiGizmo BobguiGizmo;

struct _BobguiGizmo {
  BobguiWidget parent;

  const char *name;
  int min_width;
  int min_height;
  int nat_width;
  int nat_height;
  int width;
  int height;
};

typedef BobguiWidgetClass BobguiGizmoClass;

G_DEFINE_TYPE (BobguiGizmo, bobgui_gizmo, BOBGUI_TYPE_WIDGET);

static void
bobgui_gizmo_measure (BobguiWidget      *widget,
                   BobguiOrientation  orientation,
                   int             for_size,
                   int            *minimum,
                   int            *natural,
                   int            *minimum_baseline,
                   int            *natural_baseline)
{
  BobguiGizmo *self = BOBGUI_GIZMO (widget);

  if (orientation == BOBGUI_ORIENTATION_HORIZONTAL)
    {
      *minimum = self->min_width;
      *natural = self->nat_width;
    }
  else
    {
      *minimum = self->min_height;
      *natural = self->nat_height;
    }
}

static void
bobgui_gizmo_size_allocate (BobguiWidget *widget,
                         int        width,
                         int        height,
                         int        baseline)
{
  BobguiGizmo *self = BOBGUI_GIZMO (widget);

  self->width = width;
  self->height = height;
}

static void
bobgui_gizmo_class_init (BobguiGizmoClass *klass)
{
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);

  widget_class->measure = bobgui_gizmo_measure;
  widget_class->size_allocate = bobgui_gizmo_size_allocate;
}

static void
bobgui_gizmo_init (BobguiGizmo *self)
{
}

/* Create a grid with three children in row
 *
 * +--------+--------+--------+
 * | child1 | child2 | child3 |
 * +--------+--------+--------+
 *
 * Verify that
 * - the layout has the expected min and nat sizes
 * - the children get their nat width when the layout does
 * - they all get the same height
 */
static void
test_simple_row (void)
{
  BobguiWidget *window;
  BobguiWidget *parent;
  BobguiLayoutManager *layout;
  BobguiGizmo *child1;
  BobguiGizmo *child2;
  BobguiGizmo *child3;
  BobguiLayoutChild *lc;
  int minimum, natural;

  window = bobgui_window_new ();
  parent = g_object_new (BOBGUI_TYPE_GIZMO, NULL);
  bobgui_window_set_child (BOBGUI_WINDOW (window), parent);

  layout = bobgui_grid_layout_new ();
  bobgui_widget_set_layout_manager (parent, layout);

  child1 = g_object_new (BOBGUI_TYPE_GIZMO, NULL);
  child2 = g_object_new (BOBGUI_TYPE_GIZMO, NULL);
  child3 = g_object_new (BOBGUI_TYPE_GIZMO, NULL);

  child1->name = "child1";
  child1->min_width = 10;
  child1->min_height = 10;
  child1->nat_width = 20;
  child1->nat_height = 20;
  child2->name = "child2";
  child2->min_width = 20;
  child2->min_height = 20;
  child2->nat_width = 30;
  child2->nat_height = 30;
  child3->name = "child3";
  child3->min_width = 30;
  child3->min_height = 30;
  child3->nat_width = 40;
  child3->nat_height = 40;

  bobgui_widget_set_parent (BOBGUI_WIDGET (child1), parent);
  bobgui_widget_set_parent (BOBGUI_WIDGET (child2), parent);
  bobgui_widget_set_parent (BOBGUI_WIDGET (child3), parent);

  lc = bobgui_layout_manager_get_layout_child (layout, BOBGUI_WIDGET (child1));
  bobgui_grid_layout_child_set_column (BOBGUI_GRID_LAYOUT_CHILD (lc), 0);
  lc = bobgui_layout_manager_get_layout_child (layout, BOBGUI_WIDGET (child2));
  bobgui_grid_layout_child_set_column (BOBGUI_GRID_LAYOUT_CHILD (lc), 1);
  lc = bobgui_layout_manager_get_layout_child (layout, BOBGUI_WIDGET (child3));
  bobgui_grid_layout_child_set_column (BOBGUI_GRID_LAYOUT_CHILD (lc), 2);

  bobgui_layout_manager_measure (layout,
                              parent,
                              BOBGUI_ORIENTATION_HORIZONTAL,
                              -1,
                              &minimum,
                              &natural,
                              NULL,
                              NULL);

  g_assert_cmpint (minimum, ==, 10 + 20 + 30);
  g_assert_cmpint (natural, ==, 20 + 30 + 40);

  bobgui_layout_manager_measure (layout,
                              parent,
                              BOBGUI_ORIENTATION_VERTICAL,
                              -1,
                              &minimum,
                              &natural,
                              NULL,
                              NULL);

  g_assert_cmpint (minimum, ==, 30);
  g_assert_cmpint (natural, ==, 40);

  bobgui_layout_manager_allocate (layout, parent, 90, 40, 0);

  g_assert_cmpint (child1->width, ==, 20);
  g_assert_cmpint (child2->width, ==, 30);
  g_assert_cmpint (child3->width, ==, 40);

  g_assert_cmpint (child1->height, ==, 40);
  g_assert_cmpint (child2->height, ==, 40);
  g_assert_cmpint (child3->height, ==, 40);

  bobgui_widget_unparent (BOBGUI_WIDGET (child1));
  bobgui_widget_unparent (BOBGUI_WIDGET (child2));
  bobgui_widget_unparent (BOBGUI_WIDGET (child3));

  bobgui_window_destroy (BOBGUI_WINDOW (window));
}

/* same as the previous test, with a column
 */
static void
test_simple_column (void)
{
  BobguiWidget *window;
  BobguiWidget *parent;
  BobguiLayoutManager *layout;
  BobguiGizmo *child1;
  BobguiGizmo *child2;
  BobguiGizmo *child3;
  BobguiLayoutChild *lc;
  int minimum, natural;

  window = bobgui_window_new ();
  parent = g_object_new (BOBGUI_TYPE_GIZMO, NULL);
  bobgui_window_set_child (BOBGUI_WINDOW (window), parent);

  layout = bobgui_grid_layout_new ();
  bobgui_widget_set_layout_manager (parent, layout);

  child1 = g_object_new (BOBGUI_TYPE_GIZMO, NULL);
  child2 = g_object_new (BOBGUI_TYPE_GIZMO, NULL);
  child3 = g_object_new (BOBGUI_TYPE_GIZMO, NULL);

  child1->name = "child1";
  child1->min_width = 10;
  child1->min_height = 10;
  child1->nat_width = 20;
  child1->nat_height = 20;
  child2->name = "child2";
  child2->min_width = 20;
  child2->min_height = 20;
  child2->nat_width = 30;
  child2->nat_height = 30;
  child3->name = "child3";
  child3->min_width = 30;
  child3->min_height = 30;
  child3->nat_width = 40;
  child3->nat_height = 40;

  bobgui_widget_set_parent (BOBGUI_WIDGET (child1), parent);
  bobgui_widget_set_parent (BOBGUI_WIDGET (child2), parent);
  bobgui_widget_set_parent (BOBGUI_WIDGET (child3), parent);

  lc = bobgui_layout_manager_get_layout_child (layout, BOBGUI_WIDGET (child1));
  bobgui_grid_layout_child_set_row (BOBGUI_GRID_LAYOUT_CHILD (lc), 0);
  lc = bobgui_layout_manager_get_layout_child (layout, BOBGUI_WIDGET (child2));
  bobgui_grid_layout_child_set_row (BOBGUI_GRID_LAYOUT_CHILD (lc), 1);
  lc = bobgui_layout_manager_get_layout_child (layout, BOBGUI_WIDGET (child3));
  bobgui_grid_layout_child_set_row (BOBGUI_GRID_LAYOUT_CHILD (lc), 2);

  bobgui_layout_manager_measure (layout,
                              parent,
                              BOBGUI_ORIENTATION_HORIZONTAL,
                              -1,
                              &minimum,
                              &natural,
                              NULL,
                              NULL);

  g_assert_cmpint (minimum, ==, 30);
  g_assert_cmpint (natural, ==, 40);

  bobgui_layout_manager_measure (layout,
                              parent,
                              BOBGUI_ORIENTATION_VERTICAL,
                              -1,
                              &minimum,
                              &natural,
                              NULL,
                              NULL);

  g_assert_cmpint (minimum, ==, 10 + 20 + 30);
  g_assert_cmpint (natural, ==, 20 + 30 + 40);

  bobgui_layout_manager_allocate (layout, parent, 40, 90, 0);

  g_assert_cmpint (child1->width, ==, 40);
  g_assert_cmpint (child2->width, ==, 40);
  g_assert_cmpint (child3->width, ==, 40);

  g_assert_cmpint (child1->height, ==, 20);
  g_assert_cmpint (child2->height, ==, 30);
  g_assert_cmpint (child3->height, ==, 40);

  bobgui_widget_unparent (BOBGUI_WIDGET (child1));
  bobgui_widget_unparent (BOBGUI_WIDGET (child2));
  bobgui_widget_unparent (BOBGUI_WIDGET (child3));

  bobgui_window_destroy (BOBGUI_WINDOW (window));
}

/* Create a grid with spanning children
 *
 * +--------+-----------------+
 * | child1 |      child2     |
 * +--------+--------+--------+
 * |      child3     | child4 |
 * +-----------------+--------+
 *
 * Verify that
 * - the layout has the expected min and nat sizes
 * - the children get their nat width when the layout does
 */
static void
test_spans (void)
{
  BobguiWidget *window;
  BobguiWidget *parent;
  BobguiLayoutManager *layout;
  BobguiGizmo *child1;
  BobguiGizmo *child2;
  BobguiGizmo *child3;
  BobguiGizmo *child4;
  BobguiLayoutChild *lc;
  int minimum, natural;

  window = bobgui_window_new ();
  parent = g_object_new (BOBGUI_TYPE_GIZMO, NULL);
  bobgui_window_set_child (BOBGUI_WINDOW (window), parent);

  layout = bobgui_grid_layout_new ();
  bobgui_widget_set_layout_manager (parent, layout);

  child1 = g_object_new (BOBGUI_TYPE_GIZMO, NULL);
  child2 = g_object_new (BOBGUI_TYPE_GIZMO, NULL);
  child3 = g_object_new (BOBGUI_TYPE_GIZMO, NULL);
  child4 = g_object_new (BOBGUI_TYPE_GIZMO, NULL);

  child1->name = "child1";
  child1->min_width = 10;
  child1->min_height = 10;
  child1->nat_width = 20;
  child1->nat_height = 20;
  child2->name = "child2";
  child2->min_width = 20;
  child2->min_height = 20;
  child2->nat_width = 30;
  child2->nat_height = 30;
  child3->name = "child3";
  child3->min_width = 30;
  child3->min_height = 30;
  child3->nat_width = 40;
  child3->nat_height = 40;
  child4->name = "child4";
  child4->min_width = 30;
  child4->min_height = 30;
  child4->nat_width = 40;
  child4->nat_height = 40;

  bobgui_widget_set_parent (BOBGUI_WIDGET (child1), parent);
  bobgui_widget_set_parent (BOBGUI_WIDGET (child2), parent);
  bobgui_widget_set_parent (BOBGUI_WIDGET (child3), parent);
  bobgui_widget_set_parent (BOBGUI_WIDGET (child4), parent);

  lc = bobgui_layout_manager_get_layout_child (layout, BOBGUI_WIDGET (child1));
  bobgui_grid_layout_child_set_row (BOBGUI_GRID_LAYOUT_CHILD (lc), 0);
  bobgui_grid_layout_child_set_column (BOBGUI_GRID_LAYOUT_CHILD (lc), 0);

  lc = bobgui_layout_manager_get_layout_child (layout, BOBGUI_WIDGET (child2));
  bobgui_grid_layout_child_set_row (BOBGUI_GRID_LAYOUT_CHILD (lc), 0);
  bobgui_grid_layout_child_set_column (BOBGUI_GRID_LAYOUT_CHILD (lc), 1);
  bobgui_grid_layout_child_set_column_span (BOBGUI_GRID_LAYOUT_CHILD (lc), 2);

  lc = bobgui_layout_manager_get_layout_child (layout, BOBGUI_WIDGET (child3));
  bobgui_grid_layout_child_set_row (BOBGUI_GRID_LAYOUT_CHILD (lc), 1);
  bobgui_grid_layout_child_set_column (BOBGUI_GRID_LAYOUT_CHILD (lc), 0);
  bobgui_grid_layout_child_set_column_span (BOBGUI_GRID_LAYOUT_CHILD (lc), 2);

  lc = bobgui_layout_manager_get_layout_child (layout, BOBGUI_WIDGET (child4));
  bobgui_grid_layout_child_set_row (BOBGUI_GRID_LAYOUT_CHILD (lc), 1);
  bobgui_grid_layout_child_set_column (BOBGUI_GRID_LAYOUT_CHILD (lc), 2);

  bobgui_layout_manager_measure (layout,
                              parent,
                              BOBGUI_ORIENTATION_HORIZONTAL,
                              -1,
                              &minimum,
                              &natural,
                              NULL,
                              NULL);

  g_assert_cmpint (minimum, ==, 60);
  g_assert_cmpint (natural, ==, 80);

  bobgui_layout_manager_measure (layout,
                              parent,
                              BOBGUI_ORIENTATION_VERTICAL,
                              -1,
                              &minimum,
                              &natural,
                              NULL,
                              NULL);

  g_assert_cmpint (minimum, ==, 50);
  g_assert_cmpint (natural, ==, 70);

  bobgui_layout_manager_allocate (layout, parent, 80, 70, 0);

  g_assert_cmpint (child1->width, ==, 30);
  g_assert_cmpint (child2->width, ==, 50);
  g_assert_cmpint (child3->width, ==, 40);
  g_assert_cmpint (child4->width, ==, 40);

  g_assert_cmpint (child1->height, ==, 30);
  g_assert_cmpint (child2->height, ==, 30);
  g_assert_cmpint (child3->height, ==, 40);
  g_assert_cmpint (child4->height, ==, 40);

  bobgui_widget_unparent (BOBGUI_WIDGET (child1));
  bobgui_widget_unparent (BOBGUI_WIDGET (child2));
  bobgui_widget_unparent (BOBGUI_WIDGET (child3));
  bobgui_widget_unparent (BOBGUI_WIDGET (child4));

  bobgui_window_destroy (BOBGUI_WINDOW (window));
}

/* Create a 2x2 homogeneous grid and verify
 * all children get the same size.
 */
static void
test_homogeneous (void)
{
  BobguiWidget *window;
  BobguiWidget *parent;
  BobguiLayoutManager *layout;
  BobguiGizmo *child1;
  BobguiGizmo *child2;
  BobguiGizmo *child3;
  BobguiGizmo *child4;
  BobguiLayoutChild *lc;
  int minimum, natural;

  window = bobgui_window_new ();
  parent = g_object_new (BOBGUI_TYPE_GIZMO, NULL);
  bobgui_window_set_child (BOBGUI_WINDOW (window), parent);

  layout = bobgui_grid_layout_new ();
  bobgui_grid_layout_set_row_homogeneous (BOBGUI_GRID_LAYOUT (layout), TRUE);
  bobgui_grid_layout_set_column_homogeneous (BOBGUI_GRID_LAYOUT (layout), TRUE);
  bobgui_widget_set_layout_manager (parent, layout);

  child1 = g_object_new (BOBGUI_TYPE_GIZMO, NULL);
  child2 = g_object_new (BOBGUI_TYPE_GIZMO, NULL);
  child3 = g_object_new (BOBGUI_TYPE_GIZMO, NULL);
  child4 = g_object_new (BOBGUI_TYPE_GIZMO, NULL);

  child1->name = "child1";
  child1->min_width = 10;
  child1->min_height = 10;
  child1->nat_width = 20;
  child1->nat_height = 20;
  child2->name = "child2";
  child2->min_width = 20;
  child2->min_height = 20;
  child2->nat_width = 30;
  child2->nat_height = 30;
  child3->name = "child3";
  child3->min_width = 30;
  child3->min_height = 30;
  child3->nat_width = 40;
  child3->nat_height = 40;
  child4->name = "child4";
  child4->min_width = 30;
  child4->min_height = 30;
  child4->nat_width = 40;
  child4->nat_height = 40;

  bobgui_widget_set_parent (BOBGUI_WIDGET (child1), parent);
  bobgui_widget_set_parent (BOBGUI_WIDGET (child2), parent);
  bobgui_widget_set_parent (BOBGUI_WIDGET (child3), parent);
  bobgui_widget_set_parent (BOBGUI_WIDGET (child4), parent);

  lc = bobgui_layout_manager_get_layout_child (layout, BOBGUI_WIDGET (child1));
  bobgui_grid_layout_child_set_row (BOBGUI_GRID_LAYOUT_CHILD (lc), 0);
  bobgui_grid_layout_child_set_column (BOBGUI_GRID_LAYOUT_CHILD (lc), 0);

  lc = bobgui_layout_manager_get_layout_child (layout, BOBGUI_WIDGET (child2));
  bobgui_grid_layout_child_set_row (BOBGUI_GRID_LAYOUT_CHILD (lc), 0);
  bobgui_grid_layout_child_set_column (BOBGUI_GRID_LAYOUT_CHILD (lc), 1);

  lc = bobgui_layout_manager_get_layout_child (layout, BOBGUI_WIDGET (child3));
  bobgui_grid_layout_child_set_row (BOBGUI_GRID_LAYOUT_CHILD (lc), 1);
  bobgui_grid_layout_child_set_column (BOBGUI_GRID_LAYOUT_CHILD (lc), 0);

  lc = bobgui_layout_manager_get_layout_child (layout, BOBGUI_WIDGET (child4));
  bobgui_grid_layout_child_set_row (BOBGUI_GRID_LAYOUT_CHILD (lc), 1);
  bobgui_grid_layout_child_set_column (BOBGUI_GRID_LAYOUT_CHILD (lc), 1);

  bobgui_layout_manager_measure (layout,
                              parent,
                              BOBGUI_ORIENTATION_HORIZONTAL,
                              -1,
                              &minimum,
                              &natural,
                              NULL,
                              NULL);

  g_assert_cmpint (minimum, ==, 60);
  g_assert_cmpint (natural, ==, 80);

  bobgui_layout_manager_measure (layout,
                              parent,
                              BOBGUI_ORIENTATION_VERTICAL,
                              -1,
                              &minimum,
                              &natural,
                              NULL,
                              NULL);

  g_assert_cmpint (minimum, ==, 60);
  g_assert_cmpint (natural, ==, 80);

  bobgui_layout_manager_allocate (layout, parent, 80, 80, 0);

  g_assert_cmpint (child1->width, ==, 40);
  g_assert_cmpint (child2->width, ==, 40);
  g_assert_cmpint (child3->width, ==, 40);
  g_assert_cmpint (child4->width, ==, 40);

  g_assert_cmpint (child1->height, ==, 40);
  g_assert_cmpint (child2->height, ==, 40);
  g_assert_cmpint (child3->height, ==, 40);
  g_assert_cmpint (child4->height, ==, 40);

  bobgui_widget_unparent (BOBGUI_WIDGET (child1));
  bobgui_widget_unparent (BOBGUI_WIDGET (child2));
  bobgui_widget_unparent (BOBGUI_WIDGET (child3));
  bobgui_widget_unparent (BOBGUI_WIDGET (child4));

  bobgui_window_destroy (BOBGUI_WINDOW (window));
}

/* Create a layout with three children
 *
 * +--------+--------+
 * | child1 | child2 |
 * +--------+--------+
 * |      child3     |
 * +-----------------+
 *
 * This is a layout that we also reproduce with
 * constraints, for comparison. Among the constraints:
 * - child1.width == child2.width
 * - child1.height == child2.height == child3.height
 */
static void
test_simple_layout (void)
{
  BobguiWidget *window;
  BobguiWidget *parent;
  BobguiLayoutManager *layout;
  BobguiLayoutChild *lc;
  BobguiGizmo *child1;
  BobguiGizmo *child2;
  BobguiGizmo *child3;
  int minimum, natural;

  window = bobgui_window_new ();
  parent = g_object_new (BOBGUI_TYPE_GIZMO, NULL);
  bobgui_window_set_child (BOBGUI_WINDOW (window), parent);

  layout = bobgui_grid_layout_new ();
  bobgui_grid_layout_set_row_homogeneous (BOBGUI_GRID_LAYOUT (layout), TRUE);
  bobgui_grid_layout_set_column_homogeneous (BOBGUI_GRID_LAYOUT (layout), TRUE);
  bobgui_widget_set_layout_manager (parent, layout);

  child1 = g_object_new (BOBGUI_TYPE_GIZMO, NULL);
  child2 = g_object_new (BOBGUI_TYPE_GIZMO, NULL);
  child3 = g_object_new (BOBGUI_TYPE_GIZMO, NULL);

  child1->name = "child1";
  child1->min_width = 10;
  child1->min_height = 10;
  child1->nat_width = 50;
  child1->nat_height = 50;
  child2->name = "child2";
  child2->min_width = 20;
  child2->min_height = 20;
  child2->nat_width = 50;
  child2->nat_height = 50;
  child3->name = "child3";
  child3->min_width = 50;
  child3->min_height = 10;
  child3->nat_width = 50;
  child3->nat_height = 50;

  bobgui_widget_set_parent (BOBGUI_WIDGET (child1), parent);
  bobgui_widget_set_parent (BOBGUI_WIDGET (child2), parent);
  bobgui_widget_set_parent (BOBGUI_WIDGET (child3), parent);

  lc = bobgui_layout_manager_get_layout_child (layout, BOBGUI_WIDGET (child1));
  bobgui_grid_layout_child_set_row (BOBGUI_GRID_LAYOUT_CHILD (lc), 0);
  bobgui_grid_layout_child_set_column (BOBGUI_GRID_LAYOUT_CHILD (lc), 0);

  lc = bobgui_layout_manager_get_layout_child (layout, BOBGUI_WIDGET (child2));
  bobgui_grid_layout_child_set_row (BOBGUI_GRID_LAYOUT_CHILD (lc), 0);
  bobgui_grid_layout_child_set_column (BOBGUI_GRID_LAYOUT_CHILD (lc), 1);

  lc = bobgui_layout_manager_get_layout_child (layout, BOBGUI_WIDGET (child3));
  bobgui_grid_layout_child_set_row (BOBGUI_GRID_LAYOUT_CHILD (lc), 1);
  bobgui_grid_layout_child_set_column (BOBGUI_GRID_LAYOUT_CHILD (lc), 0);
  bobgui_grid_layout_child_set_column_span (BOBGUI_GRID_LAYOUT_CHILD (lc), 2);

  bobgui_layout_manager_measure (layout,
                              parent,
                              BOBGUI_ORIENTATION_HORIZONTAL,
                              -1,
                              &minimum,
                              &natural,
                              NULL,
                              NULL);

  g_assert_cmpint (minimum, ==, 50);
  g_assert_cmpint (natural, ==, 100);

  bobgui_layout_manager_measure (layout,
                              parent,
                              BOBGUI_ORIENTATION_VERTICAL,
                              -1,
                              &minimum,
                              &natural,
                              NULL,
                              NULL);

  g_assert_cmpint (minimum, ==, 40);
  g_assert_cmpint (natural, ==, 100);

  bobgui_layout_manager_allocate (layout, parent, 100, 100, 0);

  g_assert_cmpint (child1->width, ==, 50);
  g_assert_cmpint (child2->width, ==, 50);
  g_assert_cmpint (child3->width, ==, 100);

  g_assert_cmpint (child1->height, ==, 50);
  g_assert_cmpint (child2->height, ==, 50);
  g_assert_cmpint (child3->height, ==, 50);

  bobgui_widget_unparent (BOBGUI_WIDGET (child1));
  bobgui_widget_unparent (BOBGUI_WIDGET (child2));
  bobgui_widget_unparent (BOBGUI_WIDGET (child3));

  bobgui_window_destroy (BOBGUI_WINDOW (window));
}

int
main (int   argc,
      char *argv[])
{
  bobgui_test_init (&argc, &argv);

  g_test_add_func ("/grid-layout/row", test_simple_row);
  g_test_add_func ("/grid-layout/column", test_simple_column);
  g_test_add_func ("/grid-layout/span", test_spans);
  g_test_add_func ("/grid-layout/homogeneous", test_homogeneous);
  g_test_add_func ("/grid-layout/simple", test_simple_layout);

  return g_test_run();
}
