/*
 * Copyright © 2019 Benjamin Otte
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
 * Authors: Benjamin Otte <otte@gnome.org>
 */

#include "config.h"

#include "bobguicolumnviewprivate.h"

#include "bobguiadjustment.h"
#include "bobguiboxlayout.h"
#include "bobguibuildable.h"
#include "bobguicolumnviewcolumnprivate.h"
#include "bobguicolumnviewsorterprivate.h"
#include "bobguicssnodeprivate.h"
#include "bobguidragsourceprivate.h"
#include "bobguidropcontrollermotion.h"
#include "bobguieventcontrollerkey.h"
#include "bobguieventcontrollermotion.h"
#include "bobguigestureclick.h"
#include "bobguigesturedrag.h"
#include "bobguilistviewprivate.h"
#include "bobguiscrollable.h"
#include "bobguiscrollinfoprivate.h"
#include "bobguisizerequest.h"
#include "bobguitypebuiltins.h"
#include "bobguiwidgetprivate.h"

/**
 * BobguiColumnView:
 *
 * Presents a large dynamic list of items using multiple columns with headers.
 *
 * `BobguiColumnView` uses the factories of its columns to generate a cell widget for
 * each column, for each visible item and displays them together as the row for
 * this item.
 *
 * The [property@Bobgui.ColumnView:show-row-separators] and
 * [property@Bobgui.ColumnView:show-column-separators] properties offer a simple way
 * to display separators between the rows or columns.
 *
 * `BobguiColumnView` allows the user to select items according to the selection
 * characteristics of the model. For models that allow multiple selected items,
 * it is possible to turn on *rubberband selection*, using
 * [property@Bobgui.ColumnView:enable-rubberband].
 *
 * The column view supports sorting that can be customized by the user by
 * clicking on column headers. To set this up, the `BobguiSorter` returned by
 * [method@Bobgui.ColumnView.get_sorter] must be attached to a sort model for the
 * data that the view is showing, and the columns must have sorters attached to
 * them by calling [method@Bobgui.ColumnViewColumn.set_sorter]. The initial sort
 * order can be set with [method@Bobgui.ColumnView.sort_by_column].
 *
 * The column view also supports interactive resizing and reordering of
 * columns, via Drag-and-Drop of the column headers. This can be enabled or
 * disabled with the [property@Bobgui.ColumnView:reorderable] and
 * [property@Bobgui.ColumnViewColumn:resizable] properties.
 *
 * To learn more about the list widget framework, see the
 * [overview](section-list-widget.html).
 *
 * # CSS nodes
 *
 * ```
 * columnview[.column-separators][.rich-list][.navigation-sidebar][.data-table]
 * ├── header
 * │   ├── <column header>
 * ┊   ┊
 * │   ╰── <column header>
 * │
 * ├── listview
 * │
 * ┊
 * ╰── [rubberband]
 * ```
 *
 * `BobguiColumnView` uses a single CSS node named columnview. It may carry the
 * .column-separators style class, when [property@Bobgui.ColumnView:show-column-separators]
 * property is set. Header widgets appear below a node with name header.
 * The rows are contained in a `BobguiListView` widget, so there is a listview
 * node with the same structure as for a standalone `BobguiListView` widget.
 * If [property@Bobgui.ColumnView:show-row-separators] is set, it will be passed
 * on to the list view, causing its CSS node to carry the .separators style class.
 * For rubberband selection, a node with name rubberband is used.
 *
 * The main columnview node may also carry style classes to select
 * the style of [list presentation](section-list-widget.html#list-styles):
 * .rich-list, .navigation-sidebar or .data-table.
 *
 * # Accessibility
 *
 * `BobguiColumnView` uses the [enum@Bobgui.AccessibleRole.tree_grid] role, header title
 * widgets are using the [enum@Bobgui.AccessibleRole.column_header] role. The row widgets
 * are using the [enum@Bobgui.AccessibleRole.row] role, and individual cells are using
 * the [enum@Bobgui.AccessibleRole.grid_cell] role
 */

/* We create a subclass of BobguiListView for the sole purpose of overriding
 * some parameters for item creation.
 */

struct _BobguiColumnView
{
  BobguiWidget parent_instance;

  GListStore *columns;

  BobguiColumnViewColumn *focus_column;

  BobguiWidget *header;

  BobguiListView *listview;

  BobguiSorter *sorter;

  BobguiAdjustment *hadjustment;

  guint reorderable : 1;
  guint show_column_separators : 1;
  guint in_column_resize : 1;
  guint in_column_reorder : 1;

  int drag_pos;
  int drag_x;
  int drag_offset;
  int drag_column_x;

  guint autoscroll_id;
  double autoscroll_x;
  double autoscroll_delta;

  BobguiGesture *drag_gesture;
};

struct _BobguiColumnViewClass
{
  BobguiWidgetClass parent_class;
};


#define BOBGUI_TYPE_COLUMN_LIST_VIEW (bobgui_column_list_view_get_type ())
G_DECLARE_FINAL_TYPE (BobguiColumnListView, bobgui_column_list_view, BOBGUI, COLUMN_LIST_VIEW, BobguiListView)

struct _BobguiColumnListView
{
  BobguiListView parent_instance;
};

struct _BobguiColumnListViewClass
{
  BobguiListViewClass parent_class;
};

G_DEFINE_TYPE (BobguiColumnListView, bobgui_column_list_view, BOBGUI_TYPE_LIST_VIEW)

static void
bobgui_column_list_view_init (BobguiColumnListView *view)
{
}

static BobguiListItemBase *
bobgui_column_list_view_create_list_widget (BobguiListBase *base)
{
  BobguiColumnView *self = BOBGUI_COLUMN_VIEW (bobgui_widget_get_parent (BOBGUI_WIDGET (base)));
  BobguiWidget *result;
  guint i;

  result = bobgui_column_view_row_widget_new (bobgui_list_view_get_factory (self->listview), FALSE);

  bobgui_list_factory_widget_set_single_click_activate (BOBGUI_LIST_FACTORY_WIDGET (result), BOBGUI_LIST_VIEW (base)->single_click_activate);

  for (i = 0; i < g_list_model_get_n_items (G_LIST_MODEL (self->columns)); i++)
    {
      BobguiColumnViewColumn *column = g_list_model_get_item (G_LIST_MODEL (self->columns), i);

      if (bobgui_column_view_column_get_visible (column))
        {
          BobguiWidget *cell;

          cell = bobgui_column_view_cell_widget_new (column, bobgui_column_view_is_inert (self));
          bobgui_column_view_row_widget_add_child (BOBGUI_COLUMN_VIEW_ROW_WIDGET (result), cell);
        }

      g_object_unref (column);
    }

  return BOBGUI_LIST_ITEM_BASE (result);
}

static void
bobgui_column_list_view_class_init (BobguiColumnListViewClass *klass)
{
  BobguiListBaseClass *list_base_class = BOBGUI_LIST_BASE_CLASS (klass);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);

  list_base_class->create_list_widget = bobgui_column_list_view_create_list_widget;

  bobgui_widget_class_set_css_name (widget_class, I_("listview"));
  bobgui_widget_class_set_accessible_role (widget_class, BOBGUI_ACCESSIBLE_ROLE_LIST);
}


enum
{
  PROP_0,
  PROP_COLUMNS,
  PROP_ENABLE_RUBBERBAND,
  PROP_HADJUSTMENT,
  PROP_HEADER_FACTORY,
  PROP_HSCROLL_POLICY,
  PROP_MODEL,
  PROP_REORDERABLE,
  PROP_ROW_FACTORY,
  PROP_SHOW_ROW_SEPARATORS,
  PROP_SHOW_COLUMN_SEPARATORS,
  PROP_SINGLE_CLICK_ACTIVATE,
  PROP_SORTER,
  PROP_TAB_BEHAVIOR,
  PROP_VADJUSTMENT,
  PROP_VSCROLL_POLICY,

  N_PROPS
};

enum {
  ACTIVATE,
  LAST_SIGNAL
};

static BobguiBuildableIface *parent_buildable_iface;

static void
bobgui_column_view_buildable_add_child (BobguiBuildable  *buildable,
                                     BobguiBuilder    *builder,
                                     GObject       *child,
                                     const char    *type)
{
  if (BOBGUI_IS_COLUMN_VIEW_COLUMN (child))
    {
      if (type != NULL)
        {
          BOBGUI_BUILDER_WARN_INVALID_CHILD_TYPE (buildable, type);
        }
      else
        {
          bobgui_column_view_append_column (BOBGUI_COLUMN_VIEW (buildable),
                                         BOBGUI_COLUMN_VIEW_COLUMN (child));
        }
    }
  else
    {
      parent_buildable_iface->add_child (buildable, builder, child, type);
    }
}

static void
bobgui_column_view_buildable_interface_init (BobguiBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);

  iface->add_child = bobgui_column_view_buildable_add_child;
}

static gboolean
bobgui_column_view_scrollable_get_border (BobguiScrollable *scrollable,
                                       BobguiBorder     *border)
{
  BobguiColumnView *self = BOBGUI_COLUMN_VIEW (scrollable);
  int min, nat;

  bobgui_widget_measure (self->header, BOBGUI_ORIENTATION_VERTICAL, -1, &min, &nat, NULL, NULL);
  if (bobgui_scrollable_get_vscroll_policy (BOBGUI_SCROLLABLE (self->listview)) == BOBGUI_SCROLL_MINIMUM)
    border->top = min;
  else
    border->top = nat;

  return TRUE;
}

static void
bobgui_column_view_scrollable_interface_init (BobguiScrollableInterface *iface)
{
  iface->get_border = bobgui_column_view_scrollable_get_border;
}

G_DEFINE_TYPE_WITH_CODE (BobguiColumnView, bobgui_column_view, BOBGUI_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_BUILDABLE, bobgui_column_view_buildable_interface_init)
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_SCROLLABLE, bobgui_column_view_scrollable_interface_init))

static GParamSpec *properties[N_PROPS] = { NULL, };
static guint signals[LAST_SIGNAL] = { 0 };

gboolean
bobgui_column_view_is_inert (BobguiColumnView *self)
{
  BobguiWidget *widget = BOBGUI_WIDGET (self);

  return !bobgui_widget_get_visible (widget) ||
         bobgui_widget_get_root (widget) == NULL;
}

static void
bobgui_column_view_update_cell_factories (BobguiColumnView *self,
                                       gboolean       inert)
{
  guint i, n;

  n = g_list_model_get_n_items (G_LIST_MODEL (self->columns));

  for (i = 0; i < n; i++)
    {
      BobguiColumnViewColumn *column = g_list_model_get_item (G_LIST_MODEL (self->columns), i);

      bobgui_column_view_column_update_factory (column, inert);
      g_object_unref (column);
    }
}

static void
bobgui_column_view_measure (BobguiWidget      *widget,
                         BobguiOrientation  orientation,
                         int             for_size,
                         int            *minimum,
                         int            *natural,
                         int            *minimum_baseline,
                         int            *natural_baseline)
{
  BobguiColumnView *self = BOBGUI_COLUMN_VIEW (widget);

  if (orientation == BOBGUI_ORIENTATION_HORIZONTAL)
    {
      bobgui_column_view_measure_across (self, minimum, natural);
    }
  else
    {
      int header_min, header_nat, list_min, list_nat;

      bobgui_widget_measure (BOBGUI_WIDGET (self->header),
                          orientation, for_size,
                          &header_min, &header_nat,
                          NULL, NULL);
      bobgui_widget_measure (BOBGUI_WIDGET (self->listview),
                          orientation, for_size,
                          &list_min, &list_nat,
                          NULL, NULL);
      *minimum = header_min + list_min;
      *natural = header_nat + list_nat;
    }
}

void
bobgui_column_view_distribute_width (BobguiColumnView    *self,
                                  int               width,
                                  BobguiRequestedSize *sizes)
{
  BobguiScrollablePolicy scroll_policy;
  int col_min, col_nat, extra, col_size;
  int n, n_expand, expand_size, n_extra;
  guint i;

  n = g_list_model_get_n_items (G_LIST_MODEL (self->columns));
  n_expand = 0;

  for (i = 0; i < n; i++)
    {
      BobguiColumnViewColumn *column;

      column = g_list_model_get_item (G_LIST_MODEL (self->columns), i);
      if (bobgui_column_view_column_get_visible (column))
        {
          bobgui_column_view_column_measure (column, &sizes[i].minimum_size, &sizes[i].natural_size);
          if (bobgui_column_view_column_get_expand (column))
            n_expand++;
        }
      else
        sizes[i].minimum_size = sizes[i].natural_size = 0;
      g_object_unref (column);
    }

  bobgui_column_view_measure_across (self, &col_min, &col_nat);

  scroll_policy = bobgui_scrollable_get_hscroll_policy (BOBGUI_SCROLLABLE (self->listview));
  if (scroll_policy == BOBGUI_SCROLL_MINIMUM)
    extra = MAX (width - col_min, 0);
  else
    extra = MAX (width - col_min, col_nat - col_min);

  extra = bobgui_distribute_natural_allocation (extra, n, sizes);
  if (n_expand > 0)
    {
      expand_size = extra / n_expand;
      n_extra = extra % n_expand;
    }
  else
    expand_size = n_extra = 0;

  for (i = 0; i < n; i++)
    {
      BobguiColumnViewColumn *column;

      column = g_list_model_get_item (G_LIST_MODEL (self->columns), i);
      if (bobgui_column_view_column_get_visible (column))
        {
          col_size = sizes[i].minimum_size;
          if (bobgui_column_view_column_get_expand (column))
            {
              col_size += expand_size;
              if (n_extra > 0)
                {
                  col_size++;
                  n_extra--;
                }
            }
          sizes[i].minimum_size = col_size;
        }

      g_object_unref (column);
    }
}

static int
bobgui_column_view_allocate_columns (BobguiColumnView *self,
                                  int            width)
{
  gboolean rtl;
  guint i, n;
  int total_width, x;
  BobguiRequestedSize *sizes;

  rtl = bobgui_widget_get_direction (BOBGUI_WIDGET (self)) == BOBGUI_TEXT_DIR_RTL;

  n = g_list_model_get_n_items (G_LIST_MODEL (self->columns));

  sizes = g_newa (BobguiRequestedSize, n);

  bobgui_column_view_distribute_width (self, width, sizes);

  total_width = 0;
  for (i = 0; i < n; i++)
    total_width += sizes[i].minimum_size;

  x = rtl ? total_width : 0;
  for (i = 0; i < n; i++)
    {
      BobguiColumnViewColumn *column;
      int col_size;

      column = g_list_model_get_item (G_LIST_MODEL (self->columns), i);
      if (bobgui_column_view_column_get_visible (column))
        {
          col_size = sizes[i].minimum_size;

          if (rtl)
            x -= col_size;

          bobgui_column_view_column_allocate (column, x, col_size);
          if (self->in_column_reorder && i == self->drag_pos)
            bobgui_column_view_column_set_header_position (column, self->drag_x);

          if (!rtl)
            x += col_size;
        }

      g_object_unref (column);
    }

  return total_width;
}

static void
bobgui_column_view_allocate (BobguiWidget *widget,
                          int        width,
                          int        height,
                          int        baseline)
{
  BobguiColumnView *self = BOBGUI_COLUMN_VIEW (widget);
  int full_width, header_height, min, nat, x, dx;

  x = bobgui_adjustment_get_value (self->hadjustment);
  full_width = bobgui_column_view_allocate_columns (self, width);

  bobgui_widget_measure (self->header, BOBGUI_ORIENTATION_VERTICAL, full_width, &min, &nat, NULL, NULL);
  if (bobgui_scrollable_get_vscroll_policy (BOBGUI_SCROLLABLE (self->listview)) == BOBGUI_SCROLL_MINIMUM)
    header_height = min;
  else
    header_height = nat;

  dx = (_bobgui_widget_get_direction (widget) != BOBGUI_TEXT_DIR_RTL) ? -x : width - full_width + x;

  bobgui_widget_allocate (self->header, full_width, header_height, -1,
                       gsk_transform_translate (NULL, &GRAPHENE_POINT_INIT (dx, 0)));

  bobgui_widget_allocate (BOBGUI_WIDGET (self->listview),
                       full_width, height - header_height, -1,
                       gsk_transform_translate (NULL, &GRAPHENE_POINT_INIT (dx, header_height)));

  bobgui_adjustment_configure (self->hadjustment, x, 0, MAX (full_width, width),
                            width * 0.1, width * 0.9, width);
}

static void
bobgui_column_view_root (BobguiWidget *widget)
{
  BobguiColumnView *self = BOBGUI_COLUMN_VIEW (widget);

  BOBGUI_WIDGET_CLASS (bobgui_column_view_parent_class)->root (widget);

  if (!bobgui_column_view_is_inert (self))
    bobgui_column_view_update_cell_factories (self, FALSE);
}

static void
bobgui_column_view_unroot (BobguiWidget *widget)
{
  BobguiColumnView *self = BOBGUI_COLUMN_VIEW (widget);

  if (!bobgui_column_view_is_inert (self))
    bobgui_column_view_update_cell_factories (self, TRUE);

  BOBGUI_WIDGET_CLASS (bobgui_column_view_parent_class)->unroot (widget);
}

static void
bobgui_column_view_show (BobguiWidget *widget)
{
  BobguiColumnView *self = BOBGUI_COLUMN_VIEW (widget);

  BOBGUI_WIDGET_CLASS (bobgui_column_view_parent_class)->show (widget);

  if (!bobgui_column_view_is_inert (self))
    bobgui_column_view_update_cell_factories (self, FALSE);
}

static void
bobgui_column_view_hide (BobguiWidget *widget)
{
  BobguiColumnView *self = BOBGUI_COLUMN_VIEW (widget);

  if (!bobgui_column_view_is_inert (self))
    bobgui_column_view_update_cell_factories (self, TRUE);

  BOBGUI_WIDGET_CLASS (bobgui_column_view_parent_class)->hide (widget);
}

static void
bobgui_column_view_activate_cb (BobguiListView   *listview,
                             guint          pos,
                             BobguiColumnView *self)
{
  g_signal_emit (self, signals[ACTIVATE], 0, pos);
}

static void
adjustment_value_changed_cb (BobguiAdjustment *adjustment,
                             BobguiColumnView *self)
{
  bobgui_widget_queue_allocate (BOBGUI_WIDGET (self));
}

static void
clear_adjustment (BobguiColumnView *self)
{
  if (self->hadjustment == NULL)
    return;

  g_signal_handlers_disconnect_by_func (self->hadjustment, adjustment_value_changed_cb, self);
  g_clear_object (&self->hadjustment);
}

static void
bobgui_column_view_dispose (GObject *object)
{
  BobguiColumnView *self = BOBGUI_COLUMN_VIEW (object);

  bobgui_column_view_sorter_clear (BOBGUI_COLUMN_VIEW_SORTER (self->sorter));

  while (g_list_model_get_n_items (G_LIST_MODEL (self->columns)) > 0)
    {
      BobguiColumnViewColumn *column = g_list_model_get_item (G_LIST_MODEL (self->columns), 0);
      bobgui_column_view_remove_column (self, column);
      g_object_unref (column);
    }

  g_assert (self->focus_column == NULL);

  g_clear_pointer (&self->header, bobgui_widget_unparent);

  g_clear_pointer ((BobguiWidget **) &self->listview, bobgui_widget_unparent);

  g_clear_object (&self->sorter);
  clear_adjustment (self);

  G_OBJECT_CLASS (bobgui_column_view_parent_class)->dispose (object);
}

static void
bobgui_column_view_finalize (GObject *object)
{
  BobguiColumnView *self = BOBGUI_COLUMN_VIEW (object);

  g_object_unref (self->columns);

  G_OBJECT_CLASS (bobgui_column_view_parent_class)->finalize (object);
}

static void
bobgui_column_view_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  BobguiColumnView *self = BOBGUI_COLUMN_VIEW (object);

  switch (property_id)
    {
    case PROP_COLUMNS:
      g_value_set_object (value, self->columns);
      break;

    case PROP_ENABLE_RUBBERBAND:
      g_value_set_boolean (value, bobgui_column_view_get_enable_rubberband (self));
      break;

    case PROP_HADJUSTMENT:
      g_value_set_object (value, self->hadjustment);
      break;

    case PROP_HEADER_FACTORY:
      g_value_set_object (value, bobgui_column_view_get_header_factory (self));
      break;

    case PROP_HSCROLL_POLICY:
      g_value_set_enum (value, bobgui_scrollable_get_hscroll_policy (BOBGUI_SCROLLABLE (self->listview)));
      break;

    case PROP_MODEL:
      g_value_set_object (value, bobgui_list_view_get_model (self->listview));
      break;

    case PROP_REORDERABLE:
      g_value_set_boolean (value, bobgui_column_view_get_reorderable (self));
      break;

    case PROP_ROW_FACTORY:
      g_value_set_object (value, bobgui_column_view_get_row_factory (self));
      break;

    case PROP_SHOW_ROW_SEPARATORS:
      g_value_set_boolean (value, bobgui_list_view_get_show_separators (self->listview));
      break;

    case PROP_SHOW_COLUMN_SEPARATORS:
      g_value_set_boolean (value, self->show_column_separators);
      break;

    case PROP_VADJUSTMENT:
      g_value_set_object (value, bobgui_scrollable_get_vadjustment (BOBGUI_SCROLLABLE (self->listview)));
      break;

    case PROP_VSCROLL_POLICY:
      g_value_set_enum (value, bobgui_scrollable_get_vscroll_policy (BOBGUI_SCROLLABLE (self->listview)));
      break;

    case PROP_SORTER:
      g_value_set_object (value, self->sorter);
      break;

    case PROP_SINGLE_CLICK_ACTIVATE:
      g_value_set_boolean (value, bobgui_column_view_get_single_click_activate (self));
      break;

    case PROP_TAB_BEHAVIOR:
      g_value_set_enum (value, bobgui_list_view_get_tab_behavior (self->listview));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
bobgui_column_view_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BobguiColumnView *self = BOBGUI_COLUMN_VIEW (object);
  BobguiAdjustment *adjustment;

  switch (property_id)
    {
    case PROP_ENABLE_RUBBERBAND:
      bobgui_column_view_set_enable_rubberband (self, g_value_get_boolean (value));
      break;

    case PROP_HADJUSTMENT:
      adjustment = g_value_get_object (value);
      if (adjustment == NULL)
        adjustment = bobgui_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
      g_object_ref_sink (adjustment);

      if (self->hadjustment != adjustment)
        {
          clear_adjustment (self);

          self->hadjustment = adjustment;

          g_signal_connect (adjustment, "value-changed", G_CALLBACK (adjustment_value_changed_cb), self);

          g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_HADJUSTMENT]);
        }
      break;

    case PROP_HEADER_FACTORY:
      bobgui_column_view_set_header_factory (self, g_value_get_object (value));
      break;

    case PROP_HSCROLL_POLICY:
      if (bobgui_scrollable_get_hscroll_policy (BOBGUI_SCROLLABLE (self->listview)) != g_value_get_enum (value))
        {
          bobgui_scrollable_set_hscroll_policy (BOBGUI_SCROLLABLE (self->listview), g_value_get_enum (value));
          g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_HSCROLL_POLICY]);
        }
      break;

    case PROP_MODEL:
      bobgui_column_view_set_model (self, g_value_get_object (value));
      break;

    case PROP_REORDERABLE:
      bobgui_column_view_set_reorderable (self, g_value_get_boolean (value));
      break;

    case PROP_ROW_FACTORY:
      bobgui_column_view_set_row_factory (self, g_value_get_object (value));
      break;

    case PROP_SHOW_ROW_SEPARATORS:
      bobgui_column_view_set_show_row_separators (self, g_value_get_boolean (value));
      break;

    case PROP_SHOW_COLUMN_SEPARATORS:
      bobgui_column_view_set_show_column_separators (self, g_value_get_boolean (value));
      break;

    case PROP_VADJUSTMENT:
      if (bobgui_scrollable_get_vadjustment (BOBGUI_SCROLLABLE (self->listview)) != g_value_get_object (value))
        {
          bobgui_scrollable_set_vadjustment (BOBGUI_SCROLLABLE (self->listview), g_value_get_object (value));
          g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_VADJUSTMENT]);
        }
      break;

    case PROP_VSCROLL_POLICY:
      if (bobgui_scrollable_get_vscroll_policy (BOBGUI_SCROLLABLE (self->listview)) != g_value_get_enum (value))
        {
          bobgui_scrollable_set_vscroll_policy (BOBGUI_SCROLLABLE (self->listview), g_value_get_enum (value));
          g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_VSCROLL_POLICY]);
        }
      break;

    case PROP_SINGLE_CLICK_ACTIVATE:
      bobgui_column_view_set_single_click_activate (self, g_value_get_boolean (value));
      break;

    case PROP_TAB_BEHAVIOR:
      bobgui_column_view_set_tab_behavior (self, g_value_get_enum (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
bobgui_column_view_class_init (BobguiColumnViewClass *klass)
{
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  gpointer iface;

  widget_class->focus = bobgui_widget_focus_child;
  widget_class->grab_focus = bobgui_widget_grab_focus_child;
  widget_class->measure = bobgui_column_view_measure;
  widget_class->size_allocate = bobgui_column_view_allocate;
  widget_class->root = bobgui_column_view_root;
  widget_class->unroot = bobgui_column_view_unroot;
  widget_class->show = bobgui_column_view_show;
  widget_class->hide = bobgui_column_view_hide;

  gobject_class->dispose = bobgui_column_view_dispose;
  gobject_class->finalize = bobgui_column_view_finalize;
  gobject_class->get_property = bobgui_column_view_get_property;
  gobject_class->set_property = bobgui_column_view_set_property;

  /* BobguiScrollable implementation */
  iface = g_type_default_interface_peek (BOBGUI_TYPE_SCROLLABLE);
  properties[PROP_HADJUSTMENT] =
      g_param_spec_override ("hadjustment",
                             g_object_interface_find_property (iface, "hadjustment"));
  properties[PROP_HSCROLL_POLICY] =
      g_param_spec_override ("hscroll-policy",
                             g_object_interface_find_property (iface, "hscroll-policy"));
  properties[PROP_VADJUSTMENT] =
      g_param_spec_override ("vadjustment",
                             g_object_interface_find_property (iface, "vadjustment"));
  properties[PROP_VSCROLL_POLICY] =
      g_param_spec_override ("vscroll-policy",
                             g_object_interface_find_property (iface, "vscroll-policy"));

  /**
   * BobguiColumnView:columns:
   *
   * The list of columns.
   */
  properties[PROP_COLUMNS] =
    g_param_spec_object ("columns", NULL, NULL,
                         G_TYPE_LIST_MODEL,
                         G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * BobguiColumnView:enable-rubberband:
   *
   * Allow rubberband selection.
   */
  properties[PROP_ENABLE_RUBBERBAND] =
    g_param_spec_boolean ("enable-rubberband", NULL, NULL,
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiColumnView:model:
   *
   * Model for the items displayed.
   */
  properties[PROP_MODEL] =
    g_param_spec_object ("model", NULL, NULL,
                         BOBGUI_TYPE_SELECTION_MODEL,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * BobguiColumnView:reorderable:
   *
   * Whether columns are reorderable.
   */
  properties[PROP_REORDERABLE] =
    g_param_spec_boolean ("reorderable", NULL, NULL,
                          TRUE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * BobguiColumnView:row-factory:
   *
   * The factory used for configuring rows.
   *
   * The factory must be for configuring [class@Bobgui.ColumnViewRow] objects.
   *
   * Since: 4.12
   */
  properties[PROP_ROW_FACTORY] =
    g_param_spec_object ("row-factory", NULL, NULL,
                         BOBGUI_TYPE_LIST_ITEM_FACTORY,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * BobguiColumnView:show-row-separators:
   *
   * Show separators between rows.
   */
  properties[PROP_SHOW_ROW_SEPARATORS] =
    g_param_spec_boolean ("show-row-separators", NULL, NULL,
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiColumnView:show-column-separators:
   *
   * Show separators between columns.
   */
  properties[PROP_SHOW_COLUMN_SEPARATORS] =
    g_param_spec_boolean ("show-column-separators", NULL, NULL,
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiColumnView:sorter:
   *
   * Sorter with the sorting choices of the user.
   */
  properties[PROP_SORTER] =
    g_param_spec_object ("sorter", NULL, NULL,
                         BOBGUI_TYPE_SORTER,
                         G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * BobguiColumnView:single-click-activate:
   *
   * Activate rows on single click and select them on hover.
   */
  properties[PROP_SINGLE_CLICK_ACTIVATE] =
    g_param_spec_boolean ("single-click-activate", NULL, NULL,
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiColumnView:tab-behavior:
   *
   * Behavior of the <kbd>Tab</kbd> key
   *
   * Since: 4.12
   */
  properties[PROP_TAB_BEHAVIOR] =
    g_param_spec_enum ("tab-behavior", NULL, NULL,
                       BOBGUI_TYPE_LIST_TAB_BEHAVIOR,
                       BOBGUI_LIST_TAB_ALL,
                       G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiColumnView:header-factory:
   *
   * Factory for creating header widgets.
   *
   * The factory must be for configuring [class@Bobgui.ListHeader] objects.
   *
   * Since: 4.12
   */
  properties[PROP_HEADER_FACTORY] =
    g_param_spec_object ("header-factory", NULL, NULL,
                         BOBGUI_TYPE_LIST_ITEM_FACTORY,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (gobject_class, N_PROPS, properties);

  /**
   * BobguiColumnView::activate:
   * @self: The columnview
   * @position: position of item to activate
   *
   * Emitted when a row has been activated by the user, usually via activating
   * the BobguiListBase|list.activate-item action.
   *
   * This allows for a convenient way to handle activation in a columnview.
   * See [method@Bobgui.ListItem.set_activatable] for details on how to use this
   * signal.
   */
  signals[ACTIVATE] =
    g_signal_new (I_("activate"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__UINT,
                  G_TYPE_NONE, 1,
                  G_TYPE_UINT);
  g_signal_set_va_marshaller (signals[ACTIVATE],
                              G_TYPE_FROM_CLASS (gobject_class),
                              g_cclosure_marshal_VOID__UINTv);

  bobgui_widget_class_set_css_name (widget_class, I_("columnview"));
  bobgui_widget_class_set_accessible_role (widget_class, BOBGUI_ACCESSIBLE_ROLE_TREE_GRID);
}

static void update_column_resize  (BobguiColumnView *self,
                                   double         x);
static void update_column_reorder (BobguiColumnView *self,
                                   double         x);

static gboolean
autoscroll_cb (BobguiWidget     *widget,
               GdkFrameClock *frame_clock,
               gpointer       data)
{
  BobguiColumnView *self = data;

  bobgui_adjustment_set_value (self->hadjustment,
                            bobgui_adjustment_get_value (self->hadjustment) + self->autoscroll_delta);

  self->autoscroll_x += self->autoscroll_delta;

  if (self->in_column_resize)
    update_column_resize (self, self->autoscroll_x);
  else if (self->in_column_reorder)
    update_column_reorder (self, self->autoscroll_x);

  return G_SOURCE_CONTINUE;
}

static void
add_autoscroll (BobguiColumnView *self,
                double         x,
                double         delta)
{
  self->autoscroll_x = x;
  self->autoscroll_delta = delta;

  if (self->autoscroll_id == 0)
    self->autoscroll_id = bobgui_widget_add_tick_callback (BOBGUI_WIDGET (self), autoscroll_cb, self, NULL);
}

static void
remove_autoscroll (BobguiColumnView *self)
{
  if (self->autoscroll_id != 0)
    {
      bobgui_widget_remove_tick_callback (BOBGUI_WIDGET (self), self->autoscroll_id);
      self->autoscroll_id = 0;
    }
}

#define SCROLL_EDGE_SIZE 30

static void
update_autoscroll (BobguiColumnView *self,
                   double         x)
{
  double width;
  double delta;
  graphene_point_t v;

  /* x is in header coordinates */
  if (!bobgui_widget_compute_point (self->header, BOBGUI_WIDGET (self),
                                 &GRAPHENE_POINT_INIT (x, 0),
                                 &v))
    graphene_point_init (&v, 0, 0);
  width = bobgui_widget_get_width (BOBGUI_WIDGET (self));

  if (v.x < SCROLL_EDGE_SIZE)
    delta = - (SCROLL_EDGE_SIZE - v.x)/3.0;
  else if (width - v.x < SCROLL_EDGE_SIZE)
    delta = (SCROLL_EDGE_SIZE - (width - v.x))/3.0;
  else
    delta = 0;

  if (bobgui_widget_get_direction (BOBGUI_WIDGET (self)) == BOBGUI_TEXT_DIR_RTL)
    delta = - delta;

  if (delta != 0)
    add_autoscroll (self, x, delta);
  else
    remove_autoscroll (self);
}


#define DRAG_WIDTH 8

static gboolean
bobgui_column_view_in_resize_rect (BobguiColumnView       *self,
                                BobguiColumnViewColumn *column,
                                double               x,
                                double               y)
{
  BobguiWidget *header;
  graphene_rect_t rect;
  int width;

  header = bobgui_column_view_column_get_header (column);

  if (!bobgui_widget_compute_bounds (header, self->header, &rect))
    return FALSE;

  bobgui_column_view_column_get_allocation (column, NULL, &width);
  rect.size.width = width;

  if (_bobgui_widget_get_direction (BOBGUI_WIDGET (self)) != BOBGUI_TEXT_DIR_RTL)
    rect.origin.x += rect.size.width;
  rect.origin.x -= DRAG_WIDTH / 2;
  rect.size.width = DRAG_WIDTH;

  return graphene_rect_contains_point (&rect, &(graphene_point_t) { x, y});
}

static gboolean
bobgui_column_view_in_header (BobguiColumnView       *self,
                           BobguiColumnViewColumn *column,
                           double               x,
                           double               y)
{
  BobguiWidget *header;
  graphene_rect_t rect;

  header = bobgui_column_view_column_get_header (column);

  if (!bobgui_widget_compute_bounds (header, self->header, &rect))
    return FALSE;

  return graphene_rect_contains_point (&rect, &(graphene_point_t) { x, y});
}

static void
set_resize_cursor (BobguiColumnView *self,
                   gboolean       set)
{
  int i, n;

  n = g_list_model_get_n_items (G_LIST_MODEL (self->columns));
  for (i = 0; i < n; i++)
    {
      BobguiColumnViewColumn *column = g_list_model_get_item (G_LIST_MODEL (self->columns), i);
      BobguiWidget *header = bobgui_column_view_column_get_header (column);
      if (set)
        bobgui_widget_set_cursor_from_name (header, "col-resize");
      else
        bobgui_widget_set_cursor (header, NULL);
      g_object_unref (column);
    }
}

static void
header_drag_begin (BobguiGestureDrag *gesture,
                   double          start_x,
                   double          start_y,
                   BobguiColumnView  *self)
{
  int i, n;

  self->drag_pos = -1;

  n = g_list_model_get_n_items (G_LIST_MODEL (self->columns));
  for (i = n - 1; !self->in_column_resize && i >= 0; i--)
    {
      BobguiColumnViewColumn *column = g_list_model_get_item (G_LIST_MODEL (self->columns), i);

      if (!bobgui_column_view_column_get_visible (column))
        {
          g_object_unref (column);
          continue;
        }

      if (i + 1 < n &&
          bobgui_column_view_column_get_resizable (column) &&
          bobgui_column_view_in_resize_rect (self, column, start_x, start_y))
        {
          int size;

          bobgui_gesture_set_state (BOBGUI_GESTURE (gesture), BOBGUI_EVENT_SEQUENCE_CLAIMED);
          if (!bobgui_widget_has_focus (BOBGUI_WIDGET (self)))
            bobgui_widget_grab_focus (BOBGUI_WIDGET (self));

          bobgui_column_view_column_get_allocation (column, NULL, &size);
          bobgui_column_view_column_set_fixed_width (column, size);

          self->drag_pos = i;
          if (_bobgui_widget_get_direction (BOBGUI_WIDGET (self)) != BOBGUI_TEXT_DIR_RTL)
            self->drag_x = start_x - size;
          else
            {
              int width = bobgui_widget_get_width (BOBGUI_WIDGET (self->header));

              self->drag_x = width - (start_x + size);
            }
          self->in_column_resize = TRUE;

          set_resize_cursor (self, TRUE);

          g_object_unref (column);

          break;
        }

      g_object_unref (column);
    }

  for (i = 0; !self->in_column_resize && i < n; i++)
    {
      BobguiColumnViewColumn *column = g_list_model_get_item (G_LIST_MODEL (self->columns), i);

      if (!bobgui_column_view_column_get_visible (column))
        {
          g_object_unref (column);
          continue;
        }

      if (bobgui_column_view_get_reorderable (self) &&
          bobgui_column_view_in_header (self, column, start_x, start_y))
        {
          int pos;

          bobgui_column_view_column_get_allocation (column, &pos, NULL);

          self->drag_pos = i;
          self->drag_offset = start_x - pos;

          g_object_unref (column);
          break;
        }

      g_object_unref (column);
    }
}

static void
header_drag_end (BobguiGestureDrag *gesture,
                 double          offset_x,
                 double          offset_y,
                 BobguiColumnView  *self)
{
  double start_x, x;

  bobgui_gesture_drag_get_start_point (gesture, &start_x, NULL);
  x = start_x + offset_x;

  remove_autoscroll (self);

  if (self->in_column_resize)
    {
      set_resize_cursor (self, FALSE);
      self->in_column_resize = FALSE;
    }
  else if (self->in_column_reorder)
    {
      GdkEventSequence *sequence;
      BobguiColumnViewColumn *column;
      BobguiWidget *header;
      int i;

      self->in_column_reorder = FALSE;

      if (self->drag_pos == -1)
        return;

      column = g_list_model_get_item (G_LIST_MODEL (self->columns), self->drag_pos);
      header = bobgui_column_view_column_get_header (column);
      bobgui_widget_remove_css_class (header, "dnd");

      sequence = bobgui_gesture_single_get_current_sequence (BOBGUI_GESTURE_SINGLE (gesture));
      if (!bobgui_gesture_handles_sequence (BOBGUI_GESTURE (gesture), sequence))
        {
          g_object_unref (column);
          return;
        }

      for (i = 0; i < g_list_model_get_n_items (G_LIST_MODEL (self->columns)); i++)
        {
          BobguiColumnViewColumn *col = g_list_model_get_item (G_LIST_MODEL (self->columns), i);

          if (bobgui_column_view_column_get_visible (col))
            {
              int pos, size;

              bobgui_column_view_column_get_allocation (col, &pos, &size);
              if (pos <= x && x <= pos + size)
                {
                  bobgui_column_view_insert_column (self, i, column);
                  g_object_unref (col);
                  break;
                }
            }

          g_object_unref (col);
        }

      g_object_unref (column);
    }
}

static void
update_column_resize (BobguiColumnView *self,
                      double         x)
{
  BobguiColumnViewColumn *column;

  if (_bobgui_widget_get_direction (BOBGUI_WIDGET (self)) == BOBGUI_TEXT_DIR_RTL)
    {
      int width = bobgui_widget_get_width (BOBGUI_WIDGET (self->header));

      x = width - x;
    }

  column = g_list_model_get_item (G_LIST_MODEL (self->columns), self->drag_pos);
  bobgui_column_view_column_set_fixed_width (column, MAX (x - self->drag_x, 0));
  g_object_unref (column);
}

static void
update_column_reorder (BobguiColumnView *self,
                       double         x)
{
  BobguiColumnViewColumn *column;
  int width;
  int size;

  column = g_list_model_get_item (G_LIST_MODEL (self->columns), self->drag_pos);
  width = bobgui_widget_get_width (BOBGUI_WIDGET (self->header));
  bobgui_column_view_column_get_allocation (column, NULL, &size);

  self->drag_x = CLAMP (x - self->drag_offset, 0, width - size);

  bobgui_widget_queue_allocate (BOBGUI_WIDGET (self));
  bobgui_column_view_column_queue_resize (column);
  g_object_unref (column);
}

static void
header_drag_update (BobguiGestureDrag *gesture,
                    double          offset_x,
                    double          offset_y,
                    BobguiColumnView  *self)
{
  GdkEventSequence *sequence;
  double start_x, x;

  sequence = bobgui_gesture_single_get_current_sequence (BOBGUI_GESTURE_SINGLE (gesture));
  if (!bobgui_gesture_handles_sequence (BOBGUI_GESTURE (gesture), sequence))
    return;

  if (self->drag_pos == -1)
    return;

  if (!self->in_column_resize && !self->in_column_reorder)
    {
      if (bobgui_drag_check_threshold_double (BOBGUI_WIDGET (self), 0, 0, offset_x, 0))
        {
          BobguiColumnViewColumn *column;
          BobguiWidget *header;

          column = g_list_model_get_item (G_LIST_MODEL (self->columns), self->drag_pos);
          header = bobgui_column_view_column_get_header (column);

          bobgui_widget_insert_after (header, self->header, bobgui_widget_get_last_child (self->header));
          bobgui_widget_add_css_class (header, "dnd");

          bobgui_gesture_set_state (BOBGUI_GESTURE (gesture), BOBGUI_EVENT_SEQUENCE_CLAIMED);
          if (!bobgui_widget_has_focus (BOBGUI_WIDGET (self)))
            bobgui_widget_grab_focus (BOBGUI_WIDGET (self));

          self->in_column_reorder = TRUE;

          g_object_unref (column);
        }
    }

  bobgui_gesture_drag_get_start_point (gesture, &start_x, NULL);
  x = start_x + offset_x;

  if (self->in_column_resize)
    update_column_resize (self, x);
  else if (self->in_column_reorder)
    update_column_reorder (self, x);

  if (self->in_column_resize || self->in_column_reorder)
    update_autoscroll (self, x);
}

static void
header_motion (BobguiEventControllerMotion *controller,
               double                    x,
               double                    y,
               BobguiColumnView            *self)
{
  gboolean cursor_set = FALSE;
  int i, n;

  if (self->in_column_resize)
    return;

  n = g_list_model_get_n_items (G_LIST_MODEL (self->columns));
  for (i = 0; i < n; i++)
    {
      BobguiColumnViewColumn *column = g_list_model_get_item (G_LIST_MODEL (self->columns), i);

      if (!bobgui_column_view_column_get_visible (column))
        {
          g_object_unref (column);
          continue;
        }

      if (i + 1 < n &&
          bobgui_column_view_column_get_resizable (column) &&
          bobgui_column_view_in_resize_rect (self, column, x, y))
        {
          bobgui_widget_set_cursor_from_name (self->header, "col-resize");
          cursor_set = TRUE;
        }

      g_object_unref (column);
    }

  if (!cursor_set)
    bobgui_widget_set_cursor (self->header, NULL);
}

static gboolean
header_key_pressed (BobguiEventControllerKey *controller,
                    guint                  keyval,
                    guint                  keycode,
                    GdkModifierType        modifiers,
                    BobguiColumnView         *self)
{
  if (self->in_column_reorder)
    {
      if (keyval == GDK_KEY_Escape)
        bobgui_gesture_set_state (self->drag_gesture, BOBGUI_EVENT_SEQUENCE_DENIED);
      return TRUE;
    }

  return FALSE;
}

static void
header_pressed (BobguiGestureClick *gesture,
                int              n_press,
                double           x,
                double           y,
                BobguiColumnView   *self)
{
  int i, n;

  if (n_press != 2)
    return;

  n = g_list_model_get_n_items (G_LIST_MODEL (self->columns));
  for (i = n - 1; i >= 0; i--)
    {
      BobguiColumnViewColumn *column = g_list_model_get_item (G_LIST_MODEL (self->columns), i);

      g_object_unref (column);

      if (i + 1 < n &&
          bobgui_column_view_column_get_resizable (column) &&
          bobgui_column_view_in_resize_rect (self, column, x, y))
        {
          bobgui_gesture_set_state (self->drag_gesture, BOBGUI_EVENT_SEQUENCE_DENIED);
          bobgui_column_view_column_set_fixed_width (column, -1);
          break;
        }
    }
}

static void
bobgui_column_view_drag_motion (BobguiDropControllerMotion *motion,
                             double                   x,
                             double                   y,
                             gpointer                 unused)
{
  BobguiWidget *widget = bobgui_event_controller_get_widget (BOBGUI_EVENT_CONTROLLER (motion));
  BobguiColumnView *self = BOBGUI_COLUMN_VIEW (widget);
  graphene_point_t h;

  if (!bobgui_widget_compute_point (widget, self->header,
                                 &GRAPHENE_POINT_INIT (x, 0), &h))
    graphene_point_init (&h, 0, 0);
  update_autoscroll (BOBGUI_COLUMN_VIEW (widget), h.x);
}

static void
bobgui_column_view_drag_leave (BobguiDropControllerMotion *motion,
                            gpointer                 unused)
{
  BobguiWidget *widget = bobgui_event_controller_get_widget (BOBGUI_EVENT_CONTROLLER (motion));

  remove_autoscroll (BOBGUI_COLUMN_VIEW (widget));
}

static void
bobgui_column_view_init (BobguiColumnView *self)
{
  BobguiEventController *controller;

  self->columns = g_list_store_new (BOBGUI_TYPE_COLUMN_VIEW_COLUMN);

  self->header = bobgui_column_view_row_widget_new (NULL, TRUE);
  bobgui_widget_set_can_focus (self->header, FALSE);
  bobgui_widget_set_parent (self->header, BOBGUI_WIDGET (self));

  controller = BOBGUI_EVENT_CONTROLLER (bobgui_gesture_click_new ());
  g_signal_connect (controller, "pressed", G_CALLBACK (header_pressed), self);
  bobgui_event_controller_set_propagation_phase (controller, BOBGUI_PHASE_CAPTURE);
  bobgui_widget_add_controller (self->header, controller);

  controller = BOBGUI_EVENT_CONTROLLER (bobgui_gesture_drag_new ());
  g_signal_connect (controller, "drag-begin", G_CALLBACK (header_drag_begin), self);
  g_signal_connect (controller, "drag-update", G_CALLBACK (header_drag_update), self);
  g_signal_connect (controller, "drag-end", G_CALLBACK (header_drag_end), self);
  bobgui_event_controller_set_propagation_phase (controller, BOBGUI_PHASE_CAPTURE);
  bobgui_widget_add_controller (self->header, controller);
  self->drag_gesture = BOBGUI_GESTURE (controller);

  controller = bobgui_event_controller_motion_new ();
  g_signal_connect (controller, "motion", G_CALLBACK (header_motion), self);
  bobgui_widget_add_controller (self->header, controller);

  controller = bobgui_event_controller_key_new ();
  g_signal_connect (controller, "key-pressed", G_CALLBACK (header_key_pressed), self);
  bobgui_widget_add_controller (BOBGUI_WIDGET (self), controller);

  controller = bobgui_drop_controller_motion_new ();
  g_signal_connect (controller, "motion", G_CALLBACK (bobgui_column_view_drag_motion), NULL);
  g_signal_connect (controller, "leave", G_CALLBACK (bobgui_column_view_drag_leave), NULL);
  bobgui_widget_add_controller (BOBGUI_WIDGET (self), controller);

  self->sorter = BOBGUI_SORTER (bobgui_column_view_sorter_new ());
  self->listview = BOBGUI_LIST_VIEW (g_object_new (BOBGUI_TYPE_COLUMN_LIST_VIEW, NULL));
  bobgui_widget_set_hexpand (BOBGUI_WIDGET (self->listview), TRUE);
  bobgui_widget_set_vexpand (BOBGUI_WIDGET (self->listview), TRUE);
  g_signal_connect (self->listview, "activate", G_CALLBACK (bobgui_column_view_activate_cb), self);
  bobgui_widget_set_parent (BOBGUI_WIDGET (self->listview), BOBGUI_WIDGET (self));

  bobgui_css_node_add_class (bobgui_widget_get_css_node (BOBGUI_WIDGET (self)),
                          g_quark_from_static_string (I_("view")));

  bobgui_widget_set_overflow (BOBGUI_WIDGET (self), BOBGUI_OVERFLOW_HIDDEN);

  self->reorderable = TRUE;
}

/**
 * bobgui_column_view_new:
 * @model: (nullable) (transfer full): the list model to use
 *
 * Creates a new `BobguiColumnView`.
 *
 * You most likely want to call [method@Bobgui.ColumnView.append_column]
 * to add columns next.
 *
 * Returns: a new `BobguiColumnView`
 **/
BobguiWidget *
bobgui_column_view_new (BobguiSelectionModel *model)
{
  BobguiWidget *result;

  g_return_val_if_fail (model == NULL || BOBGUI_IS_SELECTION_MODEL (model), NULL);

  result = g_object_new (BOBGUI_TYPE_COLUMN_VIEW,
                         "model", model,
                         NULL);

  /* consume the reference */
  g_clear_object (&model);

  return result;
}

/**
 * bobgui_column_view_get_model:
 * @self: a columnview
 *
 * Gets the model that's currently used to read the items displayed.
 *
 * Returns: (nullable) (transfer none): The model in use
 */
BobguiSelectionModel *
bobgui_column_view_get_model (BobguiColumnView *self)
{
  g_return_val_if_fail (BOBGUI_IS_COLUMN_VIEW (self), NULL);

  return bobgui_list_view_get_model (self->listview);
}

/**
 * bobgui_column_view_set_model:
 * @self: a columnview
 * @model: (nullable) (transfer none): the model to use
 *
 * Sets the model to use.
 *
 * This must be a [iface@Bobgui.SelectionModel].
 */
void
bobgui_column_view_set_model (BobguiColumnView     *self,
                           BobguiSelectionModel *model)
{
  g_return_if_fail (BOBGUI_IS_COLUMN_VIEW (self));
  g_return_if_fail (model == NULL || BOBGUI_IS_SELECTION_MODEL (model));

  if (bobgui_list_view_get_model (self->listview) == model)
    return;

  bobgui_list_view_set_model (self->listview, model);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_MODEL]);
}

/**
 * bobgui_column_view_get_columns:
 * @self: a columnview
 *
 * Gets the list of columns in this column view.
 *
 * This list is constant over the lifetime of @self and can be used to
 * monitor changes to the columns of @self by connecting to the
 * [signal@Gio.ListModel::items-changed] signal.
 *
 * Returns: (transfer none): The list managing the columns
 */
GListModel *
bobgui_column_view_get_columns (BobguiColumnView *self)
{
  g_return_val_if_fail (BOBGUI_IS_COLUMN_VIEW (self), NULL);

  return G_LIST_MODEL (self->columns);
}

/**
 * bobgui_column_view_set_show_row_separators:
 * @self: a columnview
 * @show_row_separators: whether to show row separators
 *
 * Sets whether the list should show separators between rows.
 */
void
bobgui_column_view_set_show_row_separators (BobguiColumnView *self,
                                         gboolean       show_row_separators)
{
  g_return_if_fail (BOBGUI_IS_COLUMN_VIEW (self));

  if (bobgui_list_view_get_show_separators (self->listview) == show_row_separators)
    return;

  bobgui_list_view_set_show_separators (self->listview, show_row_separators);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_SHOW_ROW_SEPARATORS]);
}

/**
 * bobgui_column_view_get_show_row_separators:
 * @self: a columnview
 *
 * Returns whether the list should show separators between rows.
 *
 * Returns: true if the list shows separators
 */
gboolean
bobgui_column_view_get_show_row_separators (BobguiColumnView *self)
{
  g_return_val_if_fail (BOBGUI_IS_COLUMN_VIEW (self), FALSE);

  return bobgui_list_view_get_show_separators (self->listview);
}

/**
 * bobgui_column_view_set_show_column_separators:
 * @self: a columnview
 * @show_column_separators: whether to show column separators
 *
 * Sets whether the list should show separators between columns.
 */
void
bobgui_column_view_set_show_column_separators (BobguiColumnView *self,
                                            gboolean       show_column_separators)
{
  g_return_if_fail (BOBGUI_IS_COLUMN_VIEW (self));

  if (self->show_column_separators == show_column_separators)
    return;

  self->show_column_separators = show_column_separators;

  if (show_column_separators)
    bobgui_widget_add_css_class (BOBGUI_WIDGET (self), "column-separators");
  else
    bobgui_widget_remove_css_class (BOBGUI_WIDGET (self), "column-separators");

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_SHOW_COLUMN_SEPARATORS]);
}

/**
 * bobgui_column_view_get_show_column_separators:
 * @self: a columnview
 *
 * Returns whether the list should show separators between columns.
 *
 * Returns: true if the list shows column separators
 */
gboolean
bobgui_column_view_get_show_column_separators (BobguiColumnView *self)
{
  g_return_val_if_fail (BOBGUI_IS_COLUMN_VIEW (self), FALSE);

  return self->show_column_separators;
}

/**
 * bobgui_column_view_append_column:
 * @self: a columnview
 * @column: a column that hasn't been added to a `BobguiColumnView` yet
 *
 * Appends the @column to the end of the columns in @self.
 */
void
bobgui_column_view_append_column (BobguiColumnView       *self,
                               BobguiColumnViewColumn *column)
{
  g_return_if_fail (BOBGUI_IS_COLUMN_VIEW (self));
  g_return_if_fail (BOBGUI_IS_COLUMN_VIEW_COLUMN (column));
  g_return_if_fail (bobgui_column_view_column_get_column_view (column) == NULL);

  bobgui_column_view_column_set_column_view (column, self);
  g_list_store_append (self->columns, column);
}

/**
 * bobgui_column_view_remove_column:
 * @self: a columnview
 * @column: a column that's part of @self
 *
 * Removes the @column from the list of columns of @self.
 */
void
bobgui_column_view_remove_column (BobguiColumnView       *self,
                               BobguiColumnViewColumn *column)
{
  guint i;

  g_return_if_fail (BOBGUI_IS_COLUMN_VIEW (self));
  g_return_if_fail (BOBGUI_IS_COLUMN_VIEW_COLUMN (column));
  g_return_if_fail (bobgui_column_view_column_get_column_view (column) == self);

  for (i = 0; i < g_list_model_get_n_items (G_LIST_MODEL (self->columns)); i++)
    {
      BobguiColumnViewColumn *item = g_list_model_get_item (G_LIST_MODEL (self->columns), i);

      g_object_unref (item);
      if (item == column)
        break;
    }

  bobgui_column_view_sorter_remove_column (BOBGUI_COLUMN_VIEW_SORTER (self->sorter), column);
  bobgui_column_view_column_set_column_view (column, NULL);
  g_list_store_remove (self->columns, i);

  if (self->focus_column == column)
    {
      BobguiColumnViewColumn *item;

      if (i < g_list_model_get_n_items (G_LIST_MODEL (self->columns)))
        item = g_list_model_get_item (G_LIST_MODEL (self->columns), i);
      else if (i > 0)
        item = g_list_model_get_item (G_LIST_MODEL (self->columns), i - 1);
      else
        item = NULL;

      bobgui_column_view_set_focus_column (self, item, TRUE);
    }
}

/**
 * bobgui_column_view_insert_column:
 * @self: a columnview
 * @position: the position to insert @column at
 * @column: the column to insert
 *
 * Inserts a column at the given position in the columns of @self.
 *
 * If @column is already a column of @self, it will be repositioned.
 */
void
bobgui_column_view_insert_column (BobguiColumnView       *self,
                               guint                position,
                               BobguiColumnViewColumn *column)
{
  g_return_if_fail (BOBGUI_IS_COLUMN_VIEW (self));
  g_return_if_fail (BOBGUI_IS_COLUMN_VIEW_COLUMN (column));
  g_return_if_fail (bobgui_column_view_column_get_column_view (column) == NULL ||
                    bobgui_column_view_column_get_column_view (column) == self);
  g_return_if_fail (position <= g_list_model_get_n_items (G_LIST_MODEL (self->columns)));
  int old_position = -1;

  g_object_ref (column);

  if (bobgui_column_view_column_get_column_view (column) == self)
    {
      guint i;

      for (i = 0; i < g_list_model_get_n_items (G_LIST_MODEL (self->columns)); i++)
        {
          BobguiColumnViewColumn *item = g_list_model_get_item (G_LIST_MODEL (self->columns), i);

          g_object_unref (item);
          if (item == column)
            {
              old_position = i;
              g_list_store_remove (self->columns, i);
              break;
            }
        }
    }

  g_list_store_insert (self->columns, position, column);

  bobgui_column_view_column_set_column_view (column, self);

  if (old_position != -1 && position != old_position)
    bobgui_column_view_column_set_position (column, position);

  bobgui_column_view_column_queue_resize (column);

  g_object_unref (column);
}

static void
bobgui_column_view_scroll_to_column (BobguiColumnView       *self,
                                  BobguiColumnViewColumn *column,
                                  BobguiScrollInfo       *scroll_info)
{
  int col_x, col_width, new_value;

  bobgui_column_view_column_get_header_allocation (column, &col_x, &col_width);

  new_value = bobgui_scroll_info_compute_for_orientation (scroll_info,
                                                       BOBGUI_ORIENTATION_HORIZONTAL,
                                                       col_x,
                                                       col_width,
                                                       bobgui_adjustment_get_value (self->hadjustment),
                                                       bobgui_adjustment_get_page_size (self->hadjustment));

  bobgui_adjustment_set_value (self->hadjustment, new_value);

  g_clear_pointer (&scroll_info, bobgui_scroll_info_unref);
}

void
bobgui_column_view_set_focus_column (BobguiColumnView       *self,
                                  BobguiColumnViewColumn *column,
                                  gboolean             scroll)
{
  g_assert (column == NULL || bobgui_column_view_column_get_column_view (column) == self);

  if (self->focus_column == column)
    return;

  self->focus_column = column;

  if (column && scroll)
    bobgui_column_view_scroll_to_column (self, column, NULL);
}

BobguiColumnViewColumn *
bobgui_column_view_get_focus_column (BobguiColumnView *self)
{
  return self->focus_column;
}

void
bobgui_column_view_measure_across (BobguiColumnView *self,
                                int           *minimum,
                                int           *natural)
{
  guint i;
  int min, nat;

  min = 0;
  nat = 0;

  for (i = 0; i < g_list_model_get_n_items (G_LIST_MODEL (self->columns)); i++)
    {
      BobguiColumnViewColumn *column;
      int col_min, col_nat;

      column = g_list_model_get_item (G_LIST_MODEL (self->columns), i);

      if (bobgui_column_view_column_get_visible (column))
        {
          bobgui_column_view_column_measure (column, &col_min, &col_nat);
          min += col_min;
          nat += col_nat;
        }

      g_object_unref (column);
    }

  *minimum = min;
  *natural = nat;
}

BobguiColumnViewRowWidget *
bobgui_column_view_get_header_widget (BobguiColumnView *self)
{
  return BOBGUI_COLUMN_VIEW_ROW_WIDGET (self->header);
}

BobguiListView *
bobgui_column_view_get_list_view (BobguiColumnView *self)
{
  return BOBGUI_LIST_VIEW (self->listview);
}

/**
 * bobgui_column_view_get_sorter:
 * @self: a columnview
 *
 * Returns a special sorter that reflects the users sorting
 * choices in the column view.
 *
 * To allow users to customizable sorting by clicking on column
 * headers, this sorter needs to be set on the sort model underneath
 * the model that is displayed by the view.
 *
 * See [method@Bobgui.ColumnViewColumn.set_sorter] for setting up
 * per-column sorting.
 *
 * Here is an example:
 * ```c
 * bobgui_column_view_column_set_sorter (column, sorter);
 * bobgui_column_view_append_column (view, column);
 * sorter = g_object_ref (bobgui_column_view_get_sorter (view)));
 * model = bobgui_sort_list_model_new (store, sorter);
 * selection = bobgui_no_selection_new (model);
 * bobgui_column_view_set_model (view, selection);
 * ```
 *
 * Returns: (nullable) (transfer none): the `BobguiSorter` of @self
 */
BobguiSorter *
bobgui_column_view_get_sorter (BobguiColumnView *self)
{
  g_return_val_if_fail (BOBGUI_IS_COLUMN_VIEW (self), NULL);

  return self->sorter;
}

/**
 * bobgui_column_view_sort_by_column:
 * @self: a columnview
 * @column: (nullable): the column to sort by
 * @direction: the direction to sort in
 *
 * Sets the sorting of the view.
 *
 * This function should be used to set up the initial sorting.
 * At runtime, users can change the sorting of a column view
 * by clicking on the list headers.
 *
 * This call only has an effect if the sorter returned by
 * [method@Bobgui.ColumnView.get_sorter] is set on a sort model,
 * and [method@Bobgui.ColumnViewColumn.set_sorter] has been called
 * on @column to associate a sorter with the column.
 *
 * If @column is unset, the view will be unsorted.
 */
void
bobgui_column_view_sort_by_column (BobguiColumnView       *self,
                                BobguiColumnViewColumn *column,
                                BobguiSortType          direction)
{
  g_return_if_fail (BOBGUI_IS_COLUMN_VIEW (self));
  g_return_if_fail (column == NULL || BOBGUI_IS_COLUMN_VIEW_COLUMN (column));
  g_return_if_fail (column == NULL || bobgui_column_view_column_get_column_view (column) == self);

  if (column == NULL)
    bobgui_column_view_sorter_clear (BOBGUI_COLUMN_VIEW_SORTER (self->sorter));
  else
    bobgui_column_view_sorter_set_column (BOBGUI_COLUMN_VIEW_SORTER (self->sorter),
                                       column,
                                       direction == BOBGUI_SORT_DESCENDING);
}

/**
 * bobgui_column_view_set_single_click_activate:
 * @self: a columnview
 * @single_click_activate: whether to activate items on single click
 *
 * Sets whether rows should be activated on single click and
 * selected on hover.
 */
void
bobgui_column_view_set_single_click_activate (BobguiColumnView *self,
                                           gboolean       single_click_activate)
{
  g_return_if_fail (BOBGUI_IS_COLUMN_VIEW (self));

  if (single_click_activate == bobgui_list_view_get_single_click_activate (self->listview))
    return;

  bobgui_list_view_set_single_click_activate (self->listview, single_click_activate);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_SINGLE_CLICK_ACTIVATE]);
}

/**
 * bobgui_column_view_get_single_click_activate:
 * @self: a columnview
 *
 * Returns whether rows will be activated on single click and
 * selected on hover.
 *
 * Returns: true if rows are activated on single click
 */
gboolean
bobgui_column_view_get_single_click_activate (BobguiColumnView *self)
{
  g_return_val_if_fail (BOBGUI_IS_COLUMN_VIEW (self), FALSE);

  return bobgui_list_view_get_single_click_activate (self->listview);
}

/**
 * bobgui_column_view_set_reorderable:
 * @self: a columnview
 * @reorderable: whether columns should be reorderable
 *
 * Sets whether columns should be reorderable by dragging.
 */
void
bobgui_column_view_set_reorderable (BobguiColumnView *self,
                                 gboolean       reorderable)
{
  g_return_if_fail (BOBGUI_IS_COLUMN_VIEW (self));

  if (self->reorderable == reorderable)
    return;

  self->reorderable = reorderable;

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_REORDERABLE]);
}

/**
 * bobgui_column_view_get_reorderable:
 * @self: a columnview
 *
 * Returns whether columns are reorderable.
 *
 * Returns: true if columns are reorderable
 */
gboolean
bobgui_column_view_get_reorderable (BobguiColumnView *self)
{
  g_return_val_if_fail (BOBGUI_IS_COLUMN_VIEW (self), TRUE);

  return self->reorderable;
}

/**
 * bobgui_column_view_set_enable_rubberband:
 * @self: a columnview
 * @enable_rubberband: whether to enable rubberband selection
 *
 * Sets whether selections can be changed by dragging with the mouse.
 */
void
bobgui_column_view_set_enable_rubberband (BobguiColumnView *self,
                                       gboolean       enable_rubberband)
{
  g_return_if_fail (BOBGUI_IS_COLUMN_VIEW (self));

  if (enable_rubberband == bobgui_list_view_get_enable_rubberband (self->listview))
    return;

  bobgui_list_view_set_enable_rubberband (self->listview, enable_rubberband);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_ENABLE_RUBBERBAND]);
}

/**
 * bobgui_column_view_get_enable_rubberband:
 * @self: a columnview
 *
 * Returns whether rows can be selected by dragging with the mouse.
 *
 * Returns: true if rubberband selection is enabled
 */
gboolean
bobgui_column_view_get_enable_rubberband (BobguiColumnView *self)
{
  g_return_val_if_fail (BOBGUI_IS_COLUMN_VIEW (self), FALSE);

  return bobgui_list_view_get_enable_rubberband (self->listview);
}

/**
 * bobgui_column_view_set_row_factory:
 * @self: a columnview
 * @factory: (nullable): The row factory
 *
 * Sets the factory used for configuring rows.
 *
 * The factory must be for configuring [class@Bobgui.ColumnViewRow] objects.
 *
 * If this factory is not set - which is the default - then the defaults
 * will be used.
 *
 * This factory is not used to set the widgets displayed in the individual
 * cells. For that see [method@BobguiColumnViewColumn.set_factory] and
 * [class@BobguiColumnViewCell].
 *
 * Since: 4.12
 */
void
bobgui_column_view_set_row_factory (BobguiColumnView      *self,
                                 BobguiListItemFactory *factory)
{
  g_return_if_fail (BOBGUI_IS_COLUMN_VIEW (self));

  if (factory == bobgui_list_view_get_factory (self->listview))
    return;

  bobgui_list_view_set_factory (self->listview, factory);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_ROW_FACTORY]);
}

/**
 * bobgui_column_view_get_row_factory:
 * @self: a columnview
 *
 * Gets the factory set via [method@Bobgui.ColumnView.set_row_factory].
 *
 * Returns: (nullable) (transfer none): The factory
 *
 * Since: 4.12
 */
BobguiListItemFactory *
bobgui_column_view_get_row_factory (BobguiColumnView *self)
{
  g_return_val_if_fail (BOBGUI_IS_COLUMN_VIEW (self), FALSE);

  return bobgui_list_view_get_factory (self->listview);
}

/**
 * bobgui_column_view_set_tab_behavior:
 * @self: a columnview
 * @tab_behavior: The desired tab behavior
 *
 * Sets the <kbd>Tab</kbd> key behavior.
 *
 * This influences how the <kbd>Tab</kbd> and
 * <kbd>Shift</kbd>+<kbd>Tab</kbd> keys move the
 * focus in the columnview.
 *
 * Since: 4.12
 */
void
bobgui_column_view_set_tab_behavior (BobguiColumnView      *self,
                                  BobguiListTabBehavior  tab_behavior)
{
  g_return_if_fail (BOBGUI_IS_COLUMN_VIEW (self));

  if (tab_behavior == bobgui_list_view_get_tab_behavior (self->listview))
    return;

  bobgui_list_view_set_tab_behavior (self->listview, tab_behavior);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_TAB_BEHAVIOR]);
}

/**
 * bobgui_column_view_get_tab_behavior:
 * @self: a columnview
 *
 * Gets the behavior set for the <kbd>Tab</kbd> key.
 *
 * Returns: The behavior of the <kbd>Tab</kbd> key
 *
 * Since: 4.12
 */
BobguiListTabBehavior
bobgui_column_view_get_tab_behavior (BobguiColumnView *self)
{
  g_return_val_if_fail (BOBGUI_IS_COLUMN_VIEW (self), FALSE);

  return bobgui_list_view_get_tab_behavior (self->listview);
}

/**
 * bobgui_column_view_get_header_factory:
 * @self: a columnview
 *
 * Gets the factory that's currently used to populate section headers.
 *
 * Returns: (nullable) (transfer none): The factory in use
 *
 * Since: 4.12
 */
BobguiListItemFactory *
bobgui_column_view_get_header_factory (BobguiColumnView *self)
{
  g_return_val_if_fail (BOBGUI_IS_COLUMN_VIEW (self), NULL);

  return bobgui_list_view_get_header_factory (self->listview);
}

/**
 * bobgui_column_view_set_header_factory:
 * @self: a columnview
 * @factory: (nullable) (transfer none): the factory to use
 *
 * Sets the factory to use for populating the
 * [class@Bobgui.ListHeader] objects used in section headers.
 *
 * If this factory is set to `NULL`, the list will not show
 * section headers.
 *
 * Since: 4.12
 */
void
bobgui_column_view_set_header_factory (BobguiColumnView      *self,
                                    BobguiListItemFactory *factory)
{
  g_return_if_fail (BOBGUI_IS_COLUMN_VIEW (self));
  g_return_if_fail (factory == NULL || BOBGUI_IS_LIST_ITEM_FACTORY (factory));

  if (factory == bobgui_list_view_get_header_factory (self->listview))
    return;

  bobgui_list_view_set_header_factory (self->listview, factory);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_HEADER_FACTORY]);
}

/**
 * bobgui_column_view_scroll_to:
 * @self: The columnview
 * @pos: position of the item. Must be less than the number of
 *   items in the view.
 * @column: (nullable) (transfer none): The column to scroll to
 *   or `NULL` to not scroll columns
 * @flags: actions to perform
 * @scroll: (nullable) (transfer full): details of how to perform
 *   the scroll operation or %NULL to scroll into view
 *
 * Scroll to the row at the given position - or cell if a column is
 * given - and performs the actions specified in @flags.
 *
 * This function works no matter if the columnview is shown or focused.
 * If it isn't, then the changes will take effect once that happens.
 *
 * Since: 4.12
 */
void
bobgui_column_view_scroll_to (BobguiColumnView       *self,
                           guint                pos,
                           BobguiColumnViewColumn *column,
                           BobguiListScrollFlags   flags,
                           BobguiScrollInfo       *scroll)
{
  g_return_if_fail (BOBGUI_IS_COLUMN_VIEW (self));
  g_return_if_fail (pos < bobgui_list_base_get_n_items (BOBGUI_LIST_BASE (self->listview)));
  g_return_if_fail (column == NULL || BOBGUI_IS_COLUMN_VIEW_COLUMN (column));
  if (column)
    {
      g_return_if_fail (bobgui_column_view_column_get_column_view (column) == self);
    }

  if (column && (flags & BOBGUI_LIST_SCROLL_FOCUS))
    bobgui_column_view_set_focus_column (self, column, FALSE);

  bobgui_list_view_scroll_to (self->listview,
                           pos,
                           flags,
                           scroll ? bobgui_scroll_info_ref (scroll) : NULL);

  if (column)
    bobgui_column_view_scroll_to_column (self, column, scroll);
  else
    g_clear_pointer (&scroll, bobgui_scroll_info_unref);
}
