/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2010 Red Hat, Inc.
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

#include "config.h"

#include <string.h>

#include "bobguigrid.h"

#include "bobguibuildable.h"
#include "bobguicsspositionvalueprivate.h"
#include "bobguigridlayout.h"
#include "bobguiorientable.h"
#include "bobguiprivate.h"
#include "bobguisizerequest.h"
#include "bobguicssnodeprivate.h"
#include "bobguiwidgetprivate.h"


/**
 * BobguiGrid:
 *
 * Arranges its child widgets in rows and columns.
 *
 * <picture>
 *   <source srcset="grid-dark.png" media="(prefers-color-scheme: dark)">
 *   <img alt="An example BobguiGrid" src="grid.png">
 * </picture>
 *
 * It supports arbitrary positions and horizontal/vertical spans.
 *
 * Children are added using [method@Bobgui.Grid.attach]. They can span multiple
 * rows or columns. It is also possible to add a child next to an existing
 * child, using [method@Bobgui.Grid.attach_next_to]. To remove a child from the
 * grid, use [method@Bobgui.Grid.remove].
 *
 * The behaviour of `BobguiGrid` when several children occupy the same grid
 * cell is undefined.
 *
 * # BobguiGrid as BobguiBuildable
 *
 * Every child in a `BobguiGrid` has access to a custom [iface@Bobgui.Buildable]
 * element, called `<layout>`. It can by used to specify a position in the
 * grid and optionally spans. All properties that can be used in the `<layout>`
 * element are implemented by [class@Bobgui.GridLayoutChild].
 *
 * It is implemented by `BobguiWidget` using [class@Bobgui.LayoutManager].
 *
 * To showcase it, here is a simple example:
 *
 * ```xml
 * <object class="BobguiGrid" id="my_grid">
 *   <child>
 *     <object class="BobguiButton" id="button1">
 *       <property name="label">Button 1</property>
 *       <layout>
 *         <property name="column">0</property>
 *         <property name="row">0</property>
 *       </layout>
 *     </object>
 *   </child>
 *   <child>
 *     <object class="BobguiButton" id="button2">
 *       <property name="label">Button 2</property>
 *       <layout>
 *         <property name="column">1</property>
 *         <property name="row">0</property>
 *       </layout>
 *     </object>
 *   </child>
 *   <child>
 *     <object class="BobguiButton" id="button3">
 *       <property name="label">Button 3</property>
 *       <layout>
 *         <property name="column">2</property>
 *         <property name="row">0</property>
 *         <property name="row-span">2</property>
 *       </layout>
 *     </object>
 *   </child>
 *   <child>
 *     <object class="BobguiButton" id="button4">
 *       <property name="label">Button 4</property>
 *       <layout>
 *         <property name="column">0</property>
 *         <property name="row">1</property>
 *         <property name="column-span">2</property>
 *       </layout>
 *     </object>
 *   </child>
 * </object>
 * ```
 *
 * It organizes the first two buttons side-by-side in one cell each.
 * The third button is in the last column but spans across two rows.
 * This is defined by the `row-span` property. The last button is
 * located in the second row and spans across two columns, which is
 * defined by the `column-span` property.
 *
 * # CSS nodes
 *
 * `BobguiGrid` uses a single CSS node with name `grid`.
 *
 * # Accessibility
 *
 * Until BOBGUI 4.10, `BobguiGrid` used the [enum@Bobgui.AccessibleRole.group] role.
 *
 * Starting from BOBGUI 4.12, `BobguiGrid` uses the [enum@Bobgui.AccessibleRole.generic] role.
 */

typedef struct
{
  BobguiLayoutManager *layout_manager;

  BobguiOrientation orientation;
} BobguiGridPrivate;

enum
{
  PROP_0,
  PROP_ROW_SPACING,
  PROP_COLUMN_SPACING,
  PROP_ROW_HOMOGENEOUS,
  PROP_COLUMN_HOMOGENEOUS,
  PROP_BASELINE_ROW,
  N_PROPERTIES,

  /* BobguiOrientable */
  PROP_ORIENTATION
};

static void bobgui_grid_buildable_iface_init (BobguiBuildableIface *iface);

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

G_DEFINE_TYPE_WITH_CODE (BobguiGrid, bobgui_grid, BOBGUI_TYPE_WIDGET,
                         G_ADD_PRIVATE (BobguiGrid)
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_ORIENTABLE, NULL)
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_BUILDABLE,
                                                bobgui_grid_buildable_iface_init))


static void
bobgui_grid_get_property (GObject    *object,
                       guint       prop_id,
                       GValue     *value,
                       GParamSpec *pspec)
{
  BobguiGrid *grid = BOBGUI_GRID (object);
  BobguiGridPrivate *priv = bobgui_grid_get_instance_private (grid);

  switch (prop_id)
    {
    case PROP_ORIENTATION:
      g_value_set_enum (value, priv->orientation);
      break;

    case PROP_ROW_SPACING:
      g_value_set_int (value, bobgui_grid_layout_get_row_spacing (BOBGUI_GRID_LAYOUT (priv->layout_manager)));
      break;

    case PROP_COLUMN_SPACING:
      g_value_set_int (value, bobgui_grid_layout_get_column_spacing (BOBGUI_GRID_LAYOUT (priv->layout_manager)));
      break;

    case PROP_ROW_HOMOGENEOUS:
      g_value_set_boolean (value, bobgui_grid_layout_get_row_homogeneous (BOBGUI_GRID_LAYOUT (priv->layout_manager)));
      break;

    case PROP_COLUMN_HOMOGENEOUS:
      g_value_set_boolean (value, bobgui_grid_layout_get_column_homogeneous (BOBGUI_GRID_LAYOUT (priv->layout_manager)));
      break;

    case PROP_BASELINE_ROW:
      g_value_set_int (value, bobgui_grid_layout_get_baseline_row (BOBGUI_GRID_LAYOUT (priv->layout_manager)));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_grid_set_orientation (BobguiGrid        *grid,
                          BobguiOrientation  orientation)
{
  BobguiGridPrivate *priv = bobgui_grid_get_instance_private (grid);

  if (priv->orientation != orientation)
    {
      priv->orientation = orientation;

      bobgui_widget_update_orientation (BOBGUI_WIDGET (grid), priv->orientation);

      g_object_notify (G_OBJECT (grid), "orientation");
    }
}

static void
bobgui_grid_set_property (GObject      *object,
                       guint         prop_id,
                       const GValue *value,
                       GParamSpec   *pspec)
{
  BobguiGrid *grid = BOBGUI_GRID (object);

  switch (prop_id)
    {
    case PROP_ORIENTATION:
      bobgui_grid_set_orientation (grid, g_value_get_enum (value));
      break;

    case PROP_ROW_SPACING:
      bobgui_grid_set_row_spacing (grid, g_value_get_int (value));
      break;

    case PROP_COLUMN_SPACING:
      bobgui_grid_set_column_spacing (grid, g_value_get_int (value));
      break;

    case PROP_ROW_HOMOGENEOUS:
      bobgui_grid_set_row_homogeneous (grid, g_value_get_boolean (value));
      break;

    case PROP_COLUMN_HOMOGENEOUS:
      bobgui_grid_set_column_homogeneous (grid, g_value_get_boolean (value));
      break;

    case PROP_BASELINE_ROW:
      bobgui_grid_set_baseline_row (grid, g_value_get_int (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
grid_attach (BobguiGrid   *grid,
             BobguiWidget *widget,
             int        column,
             int        row,
             int        width,
             int        height)
{
  BobguiGridPrivate *priv = bobgui_grid_get_instance_private (grid);
  BobguiGridLayoutChild *grid_child;

  bobgui_widget_set_parent (widget, BOBGUI_WIDGET (grid));

  grid_child = BOBGUI_GRID_LAYOUT_CHILD (bobgui_layout_manager_get_layout_child (priv->layout_manager, widget));
  bobgui_grid_layout_child_set_column (grid_child, column);
  bobgui_grid_layout_child_set_row (grid_child, row);
  bobgui_grid_layout_child_set_column_span (grid_child, width);
  bobgui_grid_layout_child_set_row_span (grid_child, height);
}

/* Find the position 'touching' existing
 * children. @orientation and @max determine
 * from which direction to approach (horizontal
 * + max = right, vertical + !max = top, etc).
 * @op_pos, @op_span determine the rows/columns
 * in which the touching has to happen.
 */
static int
find_attach_position (BobguiGrid         *grid,
                      BobguiOrientation   orientation,
                      int              op_pos,
                      int              op_span,
                      gboolean         max)
{
  BobguiGridPrivate *priv = bobgui_grid_get_instance_private (grid);
  BobguiWidget *child;
  gboolean hit;
  int pos;

  if (max)
    pos = -G_MAXINT;
  else
    pos = G_MAXINT;

  hit = FALSE;

  for (child = bobgui_widget_get_first_child (BOBGUI_WIDGET (grid));
       child != NULL;
       child = bobgui_widget_get_next_sibling (child))
    {
      BobguiGridLayoutChild *grid_child;
      int attach_pos = 0, attach_span = 0;
      int opposite_pos = 0, opposite_span = 0;

      grid_child = BOBGUI_GRID_LAYOUT_CHILD (bobgui_layout_manager_get_layout_child (priv->layout_manager, child));

      switch (orientation)
        {
        case BOBGUI_ORIENTATION_HORIZONTAL:
          attach_pos = bobgui_grid_layout_child_get_column (grid_child);
          attach_span = bobgui_grid_layout_child_get_column_span (grid_child);
          opposite_pos = bobgui_grid_layout_child_get_row (grid_child);
          opposite_span = bobgui_grid_layout_child_get_row_span (grid_child);
          break;

        case BOBGUI_ORIENTATION_VERTICAL:
          attach_pos = bobgui_grid_layout_child_get_row (grid_child);
          attach_span = bobgui_grid_layout_child_get_row_span (grid_child);
          opposite_pos = bobgui_grid_layout_child_get_column (grid_child);
          opposite_span = bobgui_grid_layout_child_get_column_span (grid_child);
          break;

        default:
          break;
        }

      /* check if the ranges overlap */
      if (opposite_pos <= op_pos + op_span && op_pos <= opposite_pos + opposite_span)
        {
          hit = TRUE;

          if (max)
            pos = MAX (pos, attach_pos + attach_span);
          else
            pos = MIN (pos, attach_pos);
        }
     }

  if (!hit)
    pos = 0;

  return pos;
}

static void
bobgui_grid_compute_expand (BobguiWidget *widget,
                         gboolean  *hexpand_p,
                         gboolean  *vexpand_p)
{
  BobguiWidget *w;
  gboolean hexpand = FALSE;
  gboolean vexpand = FALSE;

  for (w = bobgui_widget_get_first_child (widget);
       w != NULL;
       w = bobgui_widget_get_next_sibling (w))
    {
      hexpand = hexpand || bobgui_widget_compute_expand (w, BOBGUI_ORIENTATION_HORIZONTAL);
      vexpand = vexpand || bobgui_widget_compute_expand (w, BOBGUI_ORIENTATION_VERTICAL);
    }

  *hexpand_p = hexpand;
  *vexpand_p = vexpand;
}

static BobguiSizeRequestMode
bobgui_grid_get_request_mode (BobguiWidget *widget)
{
  BobguiWidget *w;
  int wfh = 0, hfw = 0;

  for (w = bobgui_widget_get_first_child (widget);
       w != NULL;
       w = bobgui_widget_get_next_sibling (w))
    {
      BobguiSizeRequestMode mode = bobgui_widget_get_request_mode (w);

      switch (mode)
        {
        case BOBGUI_SIZE_REQUEST_HEIGHT_FOR_WIDTH:
          hfw ++;
          break;
        case BOBGUI_SIZE_REQUEST_WIDTH_FOR_HEIGHT:
          wfh ++;
          break;
        case BOBGUI_SIZE_REQUEST_CONSTANT_SIZE:
        default:
          break;
        }
    }

  if (hfw == 0 && wfh == 0)
    return BOBGUI_SIZE_REQUEST_CONSTANT_SIZE;
  else
    return wfh > hfw ?
        BOBGUI_SIZE_REQUEST_WIDTH_FOR_HEIGHT :
        BOBGUI_SIZE_REQUEST_HEIGHT_FOR_WIDTH;
}

static void
bobgui_grid_dispose (GObject *object)
{
  BobguiWidget *child;

  while ((child = bobgui_widget_get_first_child (BOBGUI_WIDGET (object))))
    bobgui_grid_remove (BOBGUI_GRID (object), child);

  G_OBJECT_CLASS (bobgui_grid_parent_class)->dispose (object);
}

static void
bobgui_grid_class_init (BobguiGridClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (class);

  object_class->dispose = bobgui_grid_dispose;
  object_class->get_property = bobgui_grid_get_property;
  object_class->set_property = bobgui_grid_set_property;

  widget_class->compute_expand = bobgui_grid_compute_expand;
  widget_class->get_request_mode = bobgui_grid_get_request_mode;

  g_object_class_override_property (object_class, PROP_ORIENTATION, "orientation");

  /**
   * BobguiGrid:row-spacing:
   *
   * The amount of space between two consecutive rows.
   */
  obj_properties[PROP_ROW_SPACING] =
    g_param_spec_int ("row-spacing", NULL, NULL,
                      0, G_MAXINT16, 0,
                      BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiGrid:column-spacing:
   *
   * The amount of space between two consecutive columns.
   */
  obj_properties[PROP_COLUMN_SPACING] =
    g_param_spec_int ("column-spacing", NULL, NULL,
                      0, G_MAXINT16, 0,
                      BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiGrid:row-homogeneous:
   *
   * If %TRUE, the rows are all the same height.
   */
  obj_properties[PROP_ROW_HOMOGENEOUS] =
    g_param_spec_boolean ("row-homogeneous", NULL, NULL,
                          FALSE,
                          BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiGrid:column-homogeneous:
   *
   * If %TRUE, the columns are all the same width.
   */
  obj_properties[PROP_COLUMN_HOMOGENEOUS] =
    g_param_spec_boolean ("column-homogeneous", NULL, NULL,
                          FALSE,
                          BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiGrid:baseline-row:
   *
   * The row to align to the baseline when valign is using baseline alignment.
   */
  obj_properties[PROP_BASELINE_ROW] =
    g_param_spec_int ("baseline-row", NULL, NULL,
                      0, G_MAXINT, 0,
                      BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, N_PROPERTIES, obj_properties);

  bobgui_widget_class_set_css_name (widget_class, I_("grid"));
  bobgui_widget_class_set_layout_manager_type (widget_class, BOBGUI_TYPE_GRID_LAYOUT);
  bobgui_widget_class_set_accessible_role (widget_class, BOBGUI_ACCESSIBLE_ROLE_GENERIC);
}

static BobguiBuildableIface *parent_buildable_iface;

static void
bobgui_grid_buildable_add_child (BobguiBuildable *buildable,
                              BobguiBuilder   *builder,
                              GObject      *child,
                              const char   *type)
{
  if (BOBGUI_IS_WIDGET (child))
    {
      BobguiGrid *grid = BOBGUI_GRID ( buildable);
      BobguiGridPrivate *priv = bobgui_grid_get_instance_private (grid);
      int pos[2] = { 0, 0 };

      pos[priv->orientation] = find_attach_position (grid, priv->orientation, 0, 1, TRUE);
      grid_attach (grid, BOBGUI_WIDGET (child), pos[0], pos[1], 1, 1);
    }
  else
    parent_buildable_iface->add_child (buildable, builder, child, type);
}

static void
bobgui_grid_buildable_iface_init (BobguiBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);

  iface->add_child = bobgui_grid_buildable_add_child;
}

static void
bobgui_grid_init (BobguiGrid *grid)
{
  BobguiGridPrivate *priv = bobgui_grid_get_instance_private (grid);

  priv->layout_manager = bobgui_widget_get_layout_manager (BOBGUI_WIDGET (grid));

  priv->orientation = BOBGUI_ORIENTATION_HORIZONTAL;
  bobgui_widget_update_orientation (BOBGUI_WIDGET (grid), priv->orientation);
}

/**
 * bobgui_grid_new:
 *
 * Creates a new grid widget.
 *
 * Returns: the new `BobguiGrid`
 */
BobguiWidget *
bobgui_grid_new (void)
{
  return g_object_new (BOBGUI_TYPE_GRID, NULL);
}

/**
 * bobgui_grid_attach:
 * @grid: a `BobguiGrid`
 * @child: the widget to add
 * @column: the column number to attach the left side of @child to
 * @row: the row number to attach the top side of @child to
 * @width: the number of columns that @child will span
 * @height: the number of rows that @child will span
 *
 * Adds a widget to the grid.
 *
 * The position of @child is determined by @column and @row.
 * The number of “cells” that @child will occupy is determined
 * by @width and @height.
 */
void
bobgui_grid_attach (BobguiGrid   *grid,
                 BobguiWidget *child,
                 int        column,
                 int        row,
                 int        width,
                 int        height)
{
  g_return_if_fail (BOBGUI_IS_GRID (grid));
  g_return_if_fail (BOBGUI_IS_WIDGET (child));
  g_return_if_fail (_bobgui_widget_get_parent (child) == NULL);
  g_return_if_fail (width > 0);
  g_return_if_fail (height > 0);

  grid_attach (grid, child, column, row, width, height);
}

/**
 * bobgui_grid_attach_next_to:
 * @grid: a `BobguiGrid`
 * @child: the widget to add
 * @sibling: (nullable): the child of @grid that @child will be placed
 *   next to, or %NULL to place @child at the beginning or end
 * @side: the side of @sibling that @child is positioned next to
 * @width: the number of columns that @child will span
 * @height: the number of rows that @child will span
 *
 * Adds a widget to the grid.
 *
 * The widget is placed next to @sibling, on the side determined by
 * @side. When @sibling is %NULL, the widget is placed in row (for
 * left or right placement) or column 0 (for top or bottom placement),
 * at the end indicated by @side.
 *
 * Attaching widgets labeled `[1]`, `[2]`, `[3]` with `@sibling == %NULL` and
 * `@side == %BOBGUI_POS_LEFT` yields a layout of `[3][2][1]`.
 */
void
bobgui_grid_attach_next_to (BobguiGrid         *grid,
                         BobguiWidget       *child,
                         BobguiWidget       *sibling,
                         BobguiPositionType  side,
                         int              width,
                         int              height)
{
  BobguiGridPrivate *priv = bobgui_grid_get_instance_private (grid);
  BobguiGridLayoutChild *grid_sibling;
  int left, top;

  g_return_if_fail (BOBGUI_IS_GRID (grid));
  g_return_if_fail (BOBGUI_IS_WIDGET (child));
  g_return_if_fail (_bobgui_widget_get_parent (child) == NULL);
  g_return_if_fail (sibling == NULL || _bobgui_widget_get_parent (sibling) == (BobguiWidget*)grid);
  g_return_if_fail (width > 0);
  g_return_if_fail (height > 0);

  if (sibling != NULL)
    {
      grid_sibling = BOBGUI_GRID_LAYOUT_CHILD (bobgui_layout_manager_get_layout_child (priv->layout_manager, sibling));

      switch (side)
        {
        case BOBGUI_POS_LEFT:
          left = bobgui_grid_layout_child_get_column (grid_sibling) - width;
          top = bobgui_grid_layout_child_get_row (grid_sibling);
          break;
        case BOBGUI_POS_RIGHT:
          left = bobgui_grid_layout_child_get_column (grid_sibling) +
                 bobgui_grid_layout_child_get_column_span (grid_sibling);
          top = bobgui_grid_layout_child_get_row (grid_sibling);
          break;
        case BOBGUI_POS_TOP:
          left = bobgui_grid_layout_child_get_column (grid_sibling);
          top = bobgui_grid_layout_child_get_row (grid_sibling) - height;
          break;
        case BOBGUI_POS_BOTTOM:
          left = bobgui_grid_layout_child_get_column (grid_sibling);
          top = bobgui_grid_layout_child_get_row (grid_sibling) +
                bobgui_grid_layout_child_get_row_span (grid_sibling);
          break;
        default:
          g_assert_not_reached ();
        }
    }
  else
    {
      switch (side)
        {
        case BOBGUI_POS_LEFT:
          left = find_attach_position (grid, BOBGUI_ORIENTATION_HORIZONTAL, 0, height, FALSE);
          left -= width;
          top = 0;
          break;
        case BOBGUI_POS_RIGHT:
          left = find_attach_position (grid, BOBGUI_ORIENTATION_HORIZONTAL, 0, height, TRUE);
          top = 0;
          break;
        case BOBGUI_POS_TOP:
          left = 0;
          top = find_attach_position (grid, BOBGUI_ORIENTATION_VERTICAL, 0, width, FALSE);
          top -= height;
          break;
        case BOBGUI_POS_BOTTOM:
          left = 0;
          top = find_attach_position (grid, BOBGUI_ORIENTATION_VERTICAL, 0, width, TRUE);
          break;
        default:
          g_assert_not_reached ();
        }
    }

  grid_attach (grid, child, left, top, width, height);
}

/**
 * bobgui_grid_get_child_at:
 * @grid: a `BobguiGrid`
 * @column: the left edge of the cell
 * @row: the top edge of the cell
 *
 * Gets the child of @grid whose area covers the grid
 * cell at @column, @row.
 *
 * Returns: (transfer none) (nullable): the child at the given position
 */
BobguiWidget *
bobgui_grid_get_child_at (BobguiGrid *grid,
                       int      column,
                       int      row)
{
  BobguiGridPrivate *priv = bobgui_grid_get_instance_private (grid);
  BobguiWidget *child;

  g_return_val_if_fail (BOBGUI_IS_GRID (grid), NULL);

  for (child = bobgui_widget_get_first_child (BOBGUI_WIDGET (grid));
       child != NULL;
       child = bobgui_widget_get_next_sibling (child))
    {
      BobguiGridLayoutChild *grid_child;
      int child_column, child_row, child_width, child_height;

      grid_child = BOBGUI_GRID_LAYOUT_CHILD (bobgui_layout_manager_get_layout_child (priv->layout_manager, child));
      child_column = bobgui_grid_layout_child_get_column (grid_child);
      child_row = bobgui_grid_layout_child_get_row (grid_child);
      child_width = bobgui_grid_layout_child_get_column_span (grid_child);
      child_height = bobgui_grid_layout_child_get_row_span (grid_child);

      if (child_column <= column && child_column + child_width > column &&
          child_row <= row && child_row + child_height > row)
        return child;
    }

  return NULL;
}

/**
 * bobgui_grid_remove:
 * @grid: a `BobguiGrid`
 * @child: the child widget to remove
 *
 * Removes a child from @grid.
 *
 * The child must have been added with
 * [method@Bobgui.Grid.attach] or [method@Bobgui.Grid.attach_next_to].
 */
void
bobgui_grid_remove (BobguiGrid   *grid,
                 BobguiWidget *child)
{
  g_return_if_fail (BOBGUI_IS_GRID (grid));
  g_return_if_fail (BOBGUI_IS_WIDGET (child));
  g_return_if_fail (bobgui_widget_get_parent (child) == BOBGUI_WIDGET (grid));

  bobgui_widget_unparent (child);
}

/**
 * bobgui_grid_insert_row:
 * @grid: a `BobguiGrid`
 * @position: the position to insert the row at
 *
 * Inserts a row at the specified position.
 *
 * Children which are attached at or below this position
 * are moved one row down. Children which span across this
 * position are grown to span the new row.
 */
void
bobgui_grid_insert_row (BobguiGrid *grid,
                     int      position)
{
  BobguiGridPrivate *priv = bobgui_grid_get_instance_private (grid);
  BobguiWidget *child;
  int top, height;

  g_return_if_fail (BOBGUI_IS_GRID (grid));

  for (child = bobgui_widget_get_first_child (BOBGUI_WIDGET (grid));
       child != NULL;
       child = bobgui_widget_get_next_sibling (child))
    {
      BobguiGridLayoutChild *grid_child;
      
      grid_child = BOBGUI_GRID_LAYOUT_CHILD (bobgui_layout_manager_get_layout_child (priv->layout_manager, child));
      top = bobgui_grid_layout_child_get_row (grid_child);
      height = bobgui_grid_layout_child_get_row_span (grid_child);

      if (top >= position)
        bobgui_grid_layout_child_set_row (grid_child, top + 1);
      else if (top + height > position)
        bobgui_grid_layout_child_set_row_span (grid_child, height + 1);
    }
}

/**
 * bobgui_grid_remove_row:
 * @grid: a `BobguiGrid`
 * @position: the position of the row to remove
 *
 * Removes a row from the grid.
 *
 * Children that are placed in this row are removed,
 * spanning children that overlap this row have their
 * height reduced by one, and children below the row
 * are moved up.
 */
void
bobgui_grid_remove_row (BobguiGrid *grid,
                     int      position)
{
  BobguiGridPrivate *priv = bobgui_grid_get_instance_private (grid);
  BobguiWidget *child;

  g_return_if_fail (BOBGUI_IS_GRID (grid));

  child = bobgui_widget_get_first_child (BOBGUI_WIDGET (grid));
  while (child)
    {
      BobguiWidget *next = bobgui_widget_get_next_sibling (child);
      BobguiGridLayoutChild *grid_child;
      int top, height;

      grid_child = BOBGUI_GRID_LAYOUT_CHILD (bobgui_layout_manager_get_layout_child (priv->layout_manager, child));
      top = bobgui_grid_layout_child_get_row (grid_child);
      height = bobgui_grid_layout_child_get_row_span (grid_child);

      if (top <= position && top + height > position)
        height--;
      if (top > position)
        top--;

      if (height <= 0)
        {
          bobgui_grid_remove (grid, child);
        }
      else
        {
          bobgui_grid_layout_child_set_row_span (grid_child, height);
          bobgui_grid_layout_child_set_row (grid_child, top);
        }

      child = next;
    }
}

/**
 * bobgui_grid_insert_column:
 * @grid: a `BobguiGrid`
 * @position: the position to insert the column at
 *
 * Inserts a column at the specified position.
 *
 * Children which are attached at or to the right of this position
 * are moved one column to the right. Children which span across this
 * position are grown to span the new column.
 */
void
bobgui_grid_insert_column (BobguiGrid *grid,
                        int      position)
{
  BobguiGridPrivate *priv = bobgui_grid_get_instance_private (grid);
  BobguiWidget *child;

  g_return_if_fail (BOBGUI_IS_GRID (grid));

  for (child = bobgui_widget_get_first_child (BOBGUI_WIDGET (grid));
       child != NULL;
       child = bobgui_widget_get_next_sibling (child))
    {
      BobguiGridLayoutChild *grid_child;
      int left, width;

      grid_child = BOBGUI_GRID_LAYOUT_CHILD (bobgui_layout_manager_get_layout_child (priv->layout_manager, child));
      left = bobgui_grid_layout_child_get_column (grid_child);
      width = bobgui_grid_layout_child_get_column_span (grid_child);

      if (left >= position)
        bobgui_grid_layout_child_set_column (grid_child, left + 1);
      else if (left + width > position)
        bobgui_grid_layout_child_set_column_span (grid_child, width + 1);
    }
}

/**
 * bobgui_grid_remove_column:
 * @grid: a `BobguiGrid`
 * @position: the position of the column to remove
 *
 * Removes a column from the grid.
 *
 * Children that are placed in this column are removed,
 * spanning children that overlap this column have their
 * width reduced by one, and children after the column
 * are moved to the left.
 */
void
bobgui_grid_remove_column (BobguiGrid *grid,
                        int      position)
{
  BobguiGridPrivate *priv = bobgui_grid_get_instance_private (grid);
  BobguiWidget *child;

  g_return_if_fail (BOBGUI_IS_GRID (grid));

  child = bobgui_widget_get_first_child (BOBGUI_WIDGET (grid));
  while (child)
    {
      BobguiWidget *next = bobgui_widget_get_next_sibling (child);
      BobguiGridLayoutChild *grid_child;
      int left, width;

      grid_child = BOBGUI_GRID_LAYOUT_CHILD (bobgui_layout_manager_get_layout_child (priv->layout_manager, child));

      left = bobgui_grid_layout_child_get_column (grid_child);
      width = bobgui_grid_layout_child_get_column_span (grid_child);

      if (left <= position && left + width > position)
        width--;
      if (left > position)
        left--;

      if (width <= 0)
        {
          bobgui_grid_remove (grid, child);
        }
      else
        {
          bobgui_grid_layout_child_set_column_span (grid_child, width);
          bobgui_grid_layout_child_set_column (grid_child, left);
        }

      child = next;
    }
}

/**
 * bobgui_grid_insert_next_to:
 * @grid: a `BobguiGrid`
 * @sibling: the child of @grid that the new row or column will be
 *   placed next to
 * @side: the side of @sibling that @child is positioned next to
 *
 * Inserts a row or column at the specified position.
 *
 * The new row or column is placed next to @sibling, on the side
 * determined by @side. If @side is %BOBGUI_POS_TOP or %BOBGUI_POS_BOTTOM,
 * a row is inserted. If @side is %BOBGUI_POS_LEFT of %BOBGUI_POS_RIGHT,
 * a column is inserted.
 */
void
bobgui_grid_insert_next_to (BobguiGrid         *grid,
                         BobguiWidget       *sibling,
                         BobguiPositionType  side)
{
  BobguiGridPrivate *priv = bobgui_grid_get_instance_private (grid);
  BobguiGridLayoutChild *child;

  g_return_if_fail (BOBGUI_IS_GRID (grid));
  g_return_if_fail (BOBGUI_IS_WIDGET (sibling));
  g_return_if_fail (_bobgui_widget_get_parent (sibling) == (BobguiWidget*)grid);

  child = BOBGUI_GRID_LAYOUT_CHILD (bobgui_layout_manager_get_layout_child (priv->layout_manager, sibling));

  switch (side)
    {
    case BOBGUI_POS_LEFT:
      bobgui_grid_insert_column (grid, bobgui_grid_layout_child_get_column (child));
      break;
    case BOBGUI_POS_RIGHT:
      {
        int col = bobgui_grid_layout_child_get_column (child) +
                  bobgui_grid_layout_child_get_column_span (child);
        bobgui_grid_insert_column (grid, col);
      }
      break;
    case BOBGUI_POS_TOP:
      bobgui_grid_insert_row (grid, bobgui_grid_layout_child_get_row (child));
      break;
    case BOBGUI_POS_BOTTOM:
      {
        int row = bobgui_grid_layout_child_get_row (child) +
                  bobgui_grid_layout_child_get_row_span (child);
        bobgui_grid_insert_row (grid, row);
      }
      break;
    default:
      g_assert_not_reached ();
    }
}

/**
 * bobgui_grid_set_row_homogeneous:
 * @grid: a `BobguiGrid`
 * @homogeneous: %TRUE to make rows homogeneous
 *
 * Sets whether all rows of @grid will have the same height.
 */
void
bobgui_grid_set_row_homogeneous (BobguiGrid  *grid,
                              gboolean  homogeneous)
{
  BobguiGridPrivate *priv = bobgui_grid_get_instance_private (grid);
  gboolean old_val;

  g_return_if_fail (BOBGUI_IS_GRID (grid));

  old_val = bobgui_grid_layout_get_row_homogeneous (BOBGUI_GRID_LAYOUT (priv->layout_manager));
  if (old_val != !!homogeneous)
    {
      bobgui_grid_layout_set_row_homogeneous (BOBGUI_GRID_LAYOUT (priv->layout_manager), homogeneous);
      g_object_notify_by_pspec (G_OBJECT (grid), obj_properties [PROP_ROW_HOMOGENEOUS]);
    }
}

/**
 * bobgui_grid_get_row_homogeneous:
 * @grid: a `BobguiGrid`
 *
 * Returns whether all rows of @grid have the same height.
 *
 * Returns: whether all rows of @grid have the same height.
 */
gboolean
bobgui_grid_get_row_homogeneous (BobguiGrid *grid)
{
  BobguiGridPrivate *priv = bobgui_grid_get_instance_private (grid);

  g_return_val_if_fail (BOBGUI_IS_GRID (grid), FALSE);

  return bobgui_grid_layout_get_row_homogeneous (BOBGUI_GRID_LAYOUT (priv->layout_manager));
}

/**
 * bobgui_grid_set_column_homogeneous:
 * @grid: a `BobguiGrid`
 * @homogeneous: %TRUE to make columns homogeneous
 *
 * Sets whether all columns of @grid will have the same width.
 */
void
bobgui_grid_set_column_homogeneous (BobguiGrid  *grid,
                                 gboolean  homogeneous)
{
  BobguiGridPrivate *priv = bobgui_grid_get_instance_private (grid);
  gboolean old_val;

  g_return_if_fail (BOBGUI_IS_GRID (grid));

  old_val = bobgui_grid_layout_get_column_homogeneous (BOBGUI_GRID_LAYOUT (priv->layout_manager));
  if (old_val != !!homogeneous)
    {
      bobgui_grid_layout_set_column_homogeneous (BOBGUI_GRID_LAYOUT (priv->layout_manager), homogeneous);
      g_object_notify_by_pspec (G_OBJECT (grid), obj_properties [PROP_COLUMN_HOMOGENEOUS]);
    }
}

/**
 * bobgui_grid_get_column_homogeneous:
 * @grid: a `BobguiGrid`
 *
 * Returns whether all columns of @grid have the same width.
 *
 * Returns: whether all columns of @grid have the same width.
 */
gboolean
bobgui_grid_get_column_homogeneous (BobguiGrid *grid)
{
  BobguiGridPrivate *priv = bobgui_grid_get_instance_private (grid);

  g_return_val_if_fail (BOBGUI_IS_GRID (grid), FALSE);

  return bobgui_grid_layout_get_column_homogeneous (BOBGUI_GRID_LAYOUT (priv->layout_manager));
}

/**
 * bobgui_grid_set_row_spacing:
 * @grid: a `BobguiGrid`
 * @spacing: the amount of space to insert between rows
 *
 * Sets the amount of space between rows of @grid.
 */
void
bobgui_grid_set_row_spacing (BobguiGrid *grid,
                          guint    spacing)
{
  BobguiGridPrivate *priv = bobgui_grid_get_instance_private (grid);
  guint old_spacing;

  g_return_if_fail (BOBGUI_IS_GRID (grid));
  g_return_if_fail (spacing <= G_MAXINT16);

  old_spacing = bobgui_grid_layout_get_row_spacing (BOBGUI_GRID_LAYOUT (priv->layout_manager));
  if (old_spacing != spacing)
    {
      bobgui_grid_layout_set_row_spacing (BOBGUI_GRID_LAYOUT (priv->layout_manager), spacing);
      g_object_notify_by_pspec (G_OBJECT (grid), obj_properties [PROP_ROW_SPACING]);
    }
}

/**
 * bobgui_grid_get_row_spacing:
 * @grid: a `BobguiGrid`
 *
 * Returns the amount of space between the rows of @grid.
 *
 * Returns: the row spacing of @grid
 */
guint
bobgui_grid_get_row_spacing (BobguiGrid *grid)
{
  BobguiGridPrivate *priv = bobgui_grid_get_instance_private (grid);

  g_return_val_if_fail (BOBGUI_IS_GRID (grid), 0);

  return bobgui_grid_layout_get_row_spacing (BOBGUI_GRID_LAYOUT (priv->layout_manager));
}

/**
 * bobgui_grid_set_column_spacing:
 * @grid: a `BobguiGrid`
 * @spacing: the amount of space to insert between columns
 *
 * Sets the amount of space between columns of @grid.
 */
void
bobgui_grid_set_column_spacing (BobguiGrid *grid,
                             guint    spacing)
{
  BobguiGridPrivate *priv = bobgui_grid_get_instance_private (grid);
  guint old_spacing;

  g_return_if_fail (BOBGUI_IS_GRID (grid));
  g_return_if_fail (spacing <= G_MAXINT16);

  old_spacing = bobgui_grid_layout_get_column_spacing (BOBGUI_GRID_LAYOUT (priv->layout_manager));
  if (old_spacing != spacing)
    {
      bobgui_grid_layout_set_column_spacing (BOBGUI_GRID_LAYOUT (priv->layout_manager), spacing);
      g_object_notify_by_pspec (G_OBJECT (grid), obj_properties [PROP_COLUMN_SPACING]);
    }
}

/**
 * bobgui_grid_get_column_spacing:
 * @grid: a `BobguiGrid`
 *
 * Returns the amount of space between the columns of @grid.
 *
 * Returns: the column spacing of @grid
 */
guint
bobgui_grid_get_column_spacing (BobguiGrid *grid)
{
  BobguiGridPrivate *priv = bobgui_grid_get_instance_private (grid);

  g_return_val_if_fail (BOBGUI_IS_GRID (grid), 0);

  return bobgui_grid_layout_get_column_spacing (BOBGUI_GRID_LAYOUT (priv->layout_manager));
}

/**
 * bobgui_grid_set_row_baseline_position:
 * @grid: a `BobguiGrid`
 * @row: a row index
 * @pos: a `BobguiBaselinePosition`
 *
 * Sets how the baseline should be positioned on @row of the
 * grid, in case that row is assigned more space than is requested.
 *
 * The default baseline position is %BOBGUI_BASELINE_POSITION_CENTER.
 */
void
bobgui_grid_set_row_baseline_position (BobguiGrid            *grid,
				    int                 row,
				    BobguiBaselinePosition pos)
{
  BobguiGridPrivate *priv = bobgui_grid_get_instance_private (grid);

  g_return_if_fail (BOBGUI_IS_GRID (grid));

  bobgui_grid_layout_set_row_baseline_position (BOBGUI_GRID_LAYOUT (priv->layout_manager),
                                             row,
                                             pos);
}

/**
 * bobgui_grid_get_row_baseline_position:
 * @grid: a `BobguiGrid`
 * @row: a row index
 *
 * Returns the baseline position of @row.
 *
 * See [method@Bobgui.Grid.set_row_baseline_position].
 *
 * Returns: the baseline position of @row
 */
BobguiBaselinePosition
bobgui_grid_get_row_baseline_position (BobguiGrid      *grid,
				    int           row)
{
  BobguiGridPrivate *priv = bobgui_grid_get_instance_private (grid);

  g_return_val_if_fail (BOBGUI_IS_GRID (grid), BOBGUI_BASELINE_POSITION_CENTER);

  return bobgui_grid_layout_get_row_baseline_position (BOBGUI_GRID_LAYOUT (priv->layout_manager), row);
}

/**
 * bobgui_grid_set_baseline_row:
 * @grid: a `BobguiGrid`
 * @row: the row index
 *
 * Sets which row defines the global baseline for the entire grid.
 *
 * Each row in the grid can have its own local baseline, but only
 * one of those is global, meaning it will be the baseline in the
 * parent of the @grid.
 */
void
bobgui_grid_set_baseline_row (BobguiGrid *grid,
			   int      row)
{
  BobguiGridPrivate *priv = bobgui_grid_get_instance_private (grid);
  int old_row;

  g_return_if_fail (BOBGUI_IS_GRID (grid));

  old_row = bobgui_grid_layout_get_baseline_row (BOBGUI_GRID_LAYOUT (priv->layout_manager));
  if (old_row != row)
    {
      bobgui_grid_layout_set_baseline_row (BOBGUI_GRID_LAYOUT (priv->layout_manager), row);
      g_object_notify (G_OBJECT (grid), "baseline-row");
    }
}

/**
 * bobgui_grid_get_baseline_row:
 * @grid: a `BobguiGrid`
 *
 * Returns which row defines the global baseline of @grid.
 *
 * Returns: the row index defining the global baseline
 */
int
bobgui_grid_get_baseline_row (BobguiGrid *grid)
{
  BobguiGridPrivate *priv = bobgui_grid_get_instance_private (grid);

  g_return_val_if_fail (BOBGUI_IS_GRID (grid), 0);

  return bobgui_grid_layout_get_baseline_row (BOBGUI_GRID_LAYOUT (priv->layout_manager));
}

/**
 * bobgui_grid_query_child:
 * @grid: a `BobguiGrid`
 * @child: a `BobguiWidget` child of @grid
 * @column: (out) (optional): the column used to attach the left side of @child
 * @row: (out) (optional): the row used to attach the top side of @child
 * @width: (out) (optional): the number of columns @child spans
 * @height: (out) (optional): the number of rows @child spans
 *
 * Queries the attach points and spans of @child inside the given `BobguiGrid`.
 */
void
bobgui_grid_query_child (BobguiGrid   *grid,
                      BobguiWidget *child,
                      int       *column,
                      int       *row,
                      int       *width,
                      int       *height)
{
  BobguiGridPrivate *priv = bobgui_grid_get_instance_private (grid);
  BobguiGridLayoutChild *grid_child;

  g_return_if_fail (BOBGUI_IS_GRID (grid));
  g_return_if_fail (BOBGUI_IS_WIDGET (child));
  g_return_if_fail (_bobgui_widget_get_parent (child) == (BobguiWidget *) grid);

  grid_child = BOBGUI_GRID_LAYOUT_CHILD (bobgui_layout_manager_get_layout_child (priv->layout_manager, child));

  if (column != NULL)
    *column = bobgui_grid_layout_child_get_column (grid_child);
  if (row != NULL)
    *row = bobgui_grid_layout_child_get_row (grid_child);
  if (width != NULL)
    *width = bobgui_grid_layout_child_get_column_span (grid_child);
  if (height != NULL)
    *height = bobgui_grid_layout_child_get_row_span (grid_child);
}
