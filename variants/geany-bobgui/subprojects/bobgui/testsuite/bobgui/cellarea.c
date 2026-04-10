/*
 * Copyright (C) 2011 Red Hat, Inc.
 * Author: Matthias Clasen
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
#include <gdk/gdkkeysyms.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

/* tests related to handling of the cell-area property in
 * BobguiCellLayout implementations
 */

/* test that we have a cell area after new() */
static void
test_iconview_new (void)
{
  BobguiWidget *view;
  BobguiCellArea *area;

  view = bobgui_icon_view_new ();

  area = bobgui_cell_layout_get_area (BOBGUI_CELL_LAYOUT (view));
  g_assert_true (BOBGUI_IS_CELL_AREA_BOX (area));
  g_assert_true (bobgui_orientable_get_orientation (BOBGUI_ORIENTABLE (area)) == bobgui_icon_view_get_item_orientation (BOBGUI_ICON_VIEW (view)));

  g_object_ref_sink (view);
  g_object_unref (view);
}

/* test that new_with_area() keeps the provided area */
static void
test_iconview_new_with_area (void)
{
  BobguiWidget *view;
  BobguiCellArea *area;

  area = bobgui_cell_area_box_new ();
  view = bobgui_icon_view_new_with_area (area);
  g_assert_true (bobgui_cell_layout_get_area (BOBGUI_CELL_LAYOUT (view)) == area);

  g_object_ref_sink (view);
  g_object_unref (view);
}

/* test that g_object_new keeps the provided area */
static void
test_iconview_object_new (void)
{
  BobguiWidget *view;
  BobguiCellArea *area;

  area = bobgui_cell_area_box_new ();
  bobgui_orientable_set_orientation (BOBGUI_ORIENTABLE (area), BOBGUI_ORIENTATION_HORIZONTAL);
  view = g_object_new (BOBGUI_TYPE_ICON_VIEW, "cell-area", area, NULL);
  g_assert_true (bobgui_cell_layout_get_area (BOBGUI_CELL_LAYOUT (view)) == area);
  g_assert_true (bobgui_orientable_get_orientation (BOBGUI_ORIENTABLE (area)) == bobgui_icon_view_get_item_orientation (BOBGUI_ICON_VIEW (view)));

  g_object_ref_sink (view);
  g_object_unref (view);
}

/* test that we have a cell area after new() */
static void
test_combobox_new (void)
{
  BobguiWidget *view;
  BobguiCellArea *area;

  view = bobgui_combo_box_new ();

  area = bobgui_cell_layout_get_area (BOBGUI_CELL_LAYOUT (view));
  g_assert_true (BOBGUI_IS_CELL_AREA_BOX (area));

  g_object_ref_sink (view);
  g_object_unref (view);
}

static int subclass_init;

typedef BobguiComboBox MyComboBox;
typedef BobguiComboBoxClass MyComboBoxClass;

GType my_combo_box_get_type (void);

G_DEFINE_TYPE (MyComboBox, my_combo_box, BOBGUI_TYPE_COMBO_BOX)

static void
my_combo_box_class_init (MyComboBoxClass *klass)
{
}

static void
my_combo_box_init (MyComboBox *view)
{
  BobguiCellArea *area;

  if (subclass_init == 0)
    {
      /* do nothing to area */
    }
  else if (subclass_init == 1)
    {
      area = bobgui_cell_layout_get_area (BOBGUI_CELL_LAYOUT (view));
      g_assert_true (BOBGUI_IS_CELL_AREA_BOX (area));
      g_assert_cmpint (bobgui_orientable_get_orientation (BOBGUI_ORIENTABLE (area)), ==, BOBGUI_ORIENTATION_HORIZONTAL);
      bobgui_orientable_set_orientation (BOBGUI_ORIENTABLE (area), BOBGUI_ORIENTATION_VERTICAL);
    }
}

/* test that a combobox subclass has an area */
static void
test_combobox_subclass0 (void)
{
  BobguiWidget *view;
  BobguiCellArea *area;

  subclass_init = 0;

  view = g_object_new (my_combo_box_get_type (), NULL);
  area = bobgui_cell_layout_get_area (BOBGUI_CELL_LAYOUT (view));
  g_assert_true (BOBGUI_IS_CELL_AREA_BOX (area));
  g_assert_cmpint (bobgui_orientable_get_orientation (BOBGUI_ORIENTABLE (area)), ==, BOBGUI_ORIENTATION_HORIZONTAL);

  g_object_ref_sink (view);
  g_object_unref (view);
}

/* test we can access the area in subclass init */
static void
test_combobox_subclass2 (void)
{
  BobguiWidget *view;
  BobguiCellArea *area;

  subclass_init = 1;

  view = g_object_new (my_combo_box_get_type (), NULL);
  area = bobgui_cell_layout_get_area (BOBGUI_CELL_LAYOUT (view));
  g_assert_true (BOBGUI_IS_CELL_AREA_BOX (area));
  g_assert_cmpint (bobgui_orientable_get_orientation (BOBGUI_ORIENTABLE (area)), ==, BOBGUI_ORIENTATION_VERTICAL);

  g_object_ref_sink (view);
  g_object_unref (view);
}

/* test that we have a cell area after new() */
static void
test_cellview_new (void)
{
  BobguiWidget *view;
  BobguiCellArea *area;

  view = bobgui_cell_view_new ();

  area = bobgui_cell_layout_get_area (BOBGUI_CELL_LAYOUT (view));
  g_assert_true (BOBGUI_IS_CELL_AREA_BOX (area));

  g_object_ref_sink (view);
  g_object_unref (view);
}

/* test that new_with_context() keeps the provided area */
static void
test_cellview_new_with_context (void)
{
  BobguiWidget *view;
  BobguiCellArea *area;
  BobguiCellAreaContext *context;

  area = bobgui_cell_area_box_new ();
  context = bobgui_cell_area_create_context (area);
  view = bobgui_cell_view_new_with_context (area, context);
  g_assert_true (bobgui_cell_layout_get_area (BOBGUI_CELL_LAYOUT (view)) == area);

  g_object_ref_sink (view);
  g_object_unref (view);
}

/* test that g_object_new keeps the provided area */
static void
test_cellview_object_new (void)
{
  BobguiWidget *view;
  BobguiCellArea *area;

  area = bobgui_cell_area_box_new ();
  bobgui_orientable_set_orientation (BOBGUI_ORIENTABLE (area), BOBGUI_ORIENTATION_HORIZONTAL);
  view = g_object_new (BOBGUI_TYPE_CELL_VIEW, "cell-area", area, NULL);
  g_assert_true (bobgui_cell_layout_get_area (BOBGUI_CELL_LAYOUT (view)) == area);

  g_object_ref_sink (view);
  g_object_unref (view);
}

/* test that we have a cell area after new() */
static void
test_column_new (void)
{
  BobguiTreeViewColumn *col;
  BobguiCellArea *area;

  col = bobgui_tree_view_column_new ();

  area = bobgui_cell_layout_get_area (BOBGUI_CELL_LAYOUT (col));
  g_assert_true (BOBGUI_IS_CELL_AREA_BOX (area));

  g_object_ref_sink (col);
  g_object_unref (col);
}

/* test that new_with_area() keeps the provided area */
static void
test_column_new_with_area (void)
{
  BobguiTreeViewColumn *col;
  BobguiCellArea *area;

  area = bobgui_cell_area_box_new ();
  col = bobgui_tree_view_column_new_with_area (area);
  g_assert_true (bobgui_cell_layout_get_area (BOBGUI_CELL_LAYOUT (col)) == area);

  g_object_ref_sink (col);
  g_object_unref (col);
}

/* test that g_object_new keeps the provided area */
static void
test_column_object_new (void)
{
  BobguiTreeViewColumn *col;
  BobguiCellArea *area;

  area = bobgui_cell_area_box_new ();
  bobgui_orientable_set_orientation (BOBGUI_ORIENTABLE (area), BOBGUI_ORIENTATION_HORIZONTAL);
  col = g_object_new (BOBGUI_TYPE_TREE_VIEW_COLUMN, "cell-area", area, NULL);
  g_assert_true (bobgui_cell_layout_get_area (BOBGUI_CELL_LAYOUT (col)) == area);

  g_object_ref_sink (col);
  g_object_unref (col);
}

/* test that we have a cell area after new() */
static void
test_completion_new (void)
{
  BobguiEntryCompletion *c;
  BobguiCellArea *area;

  c = bobgui_entry_completion_new ();

  area = bobgui_cell_layout_get_area (BOBGUI_CELL_LAYOUT (c));
  g_assert_true (BOBGUI_IS_CELL_AREA_BOX (area));

  g_object_ref_sink (c);
  g_object_unref (c);
}

/* test that new_with_area() keeps the provided area */
static void
test_completion_new_with_area (void)
{
  BobguiEntryCompletion *c;
  BobguiCellArea *area;

  area = bobgui_cell_area_box_new ();
  c = bobgui_entry_completion_new_with_area (area);
  g_assert_true (bobgui_cell_layout_get_area (BOBGUI_CELL_LAYOUT (c)) == area);

  g_object_ref_sink (c);
  g_object_unref (c);
}

/* test that g_object_new keeps the provided area */
static void
test_completion_object_new (void)
{
  BobguiEntryCompletion *c;
  BobguiCellArea *area;

  area = bobgui_cell_area_box_new ();
  bobgui_orientable_set_orientation (BOBGUI_ORIENTABLE (area), BOBGUI_ORIENTATION_HORIZONTAL);
  c = g_object_new (BOBGUI_TYPE_ENTRY_COMPLETION, "cell-area", area, NULL);
  g_assert_true (bobgui_cell_layout_get_area (BOBGUI_CELL_LAYOUT (c)) == area);

  g_object_ref_sink (c);
  g_object_unref (c);
}

int
main (int argc, char *argv[])
{
  bobgui_test_init (&argc, &argv);
  bobgui_test_register_all_types();

  g_test_add_func ("/tests/iconview-new", test_iconview_new);
  g_test_add_func ("/tests/iconview-new-with-area", test_iconview_new_with_area);
  g_test_add_func ("/tests/iconview-object-new", test_iconview_object_new);

  g_test_add_func ("/tests/combobox-new", test_combobox_new);
  g_test_add_func ("/tests/combobox-subclass0", test_combobox_subclass0);
  g_test_add_func ("/tests/combobox-subclass2", test_combobox_subclass2);

  g_test_add_func ("/tests/cellview-new", test_cellview_new);
  g_test_add_func ("/tests/cellview-new-with-context", test_cellview_new_with_context);
  g_test_add_func ("/tests/cellview-object-new", test_cellview_object_new);

  g_test_add_func ("/tests/column-new", test_column_new);
  g_test_add_func ("/tests/column-new-with-area", test_column_new_with_area);
  g_test_add_func ("/tests/column-object-new", test_column_object_new);

  g_test_add_func ("/tests/completion-new", test_completion_new);
  g_test_add_func ("/tests/completion-new-with-area", test_completion_new_with_area);
  g_test_add_func ("/tests/completion-object-new", test_completion_object_new);

  return g_test_run();
}
