/* bobguitreeviewcolumn.c
 * Copyright (C) 2000  Red Hat, Inc.,  Jonathan Blandford <jrb@redhat.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "bobguitreeviewcolumn.h"

#include "bobguibox.h"
#include "bobguibutton.h"
#include "bobguicellareabox.h"
#include "bobguicellareacontext.h"
#include "bobguicelllayout.h"
#include "bobguidragsourceprivate.h"
#include "bobguiframe.h"
#include "bobguiimage.h"
#include "bobguilabel.h"
#include "bobguimarshalers.h"
#include "bobguiprivate.h"
#include "bobguitreeprivate.h"
#include "deprecated/bobguitreeview.h"
#include "bobguitypebuiltins.h"
#include "bobguiwidgetprivate.h"
#include "bobguigesturedrag.h"
#include "bobguieventcontrollerfocus.h"
#include "bobguieventcontrollerkey.h"
#include "bobguibuiltiniconprivate.h"

#include <string.h>

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

/**
 * BobguiTreeViewColumn:
 *
 * A visible column in a [class@Bobgui.TreeView] widget
 *
 * The `BobguiTreeViewColumn` object represents a visible column in a `BobguiTreeView` widget.
 * It allows to set properties of the column header, and functions as a holding pen
 * for the cell renderers which determine how the data in the column is displayed.
 *
 * Please refer to the [tree widget conceptual overview](section-tree-widget.html)
 * for an overview of all the objects and data types related to the tree widget and
 * how they work together, and to the [class@Bobgui.TreeView] documentation for specifics
 * about the CSS node structure for treeviews and their headers.
 *
 * Deprecated: 4.10: Use [class@Bobgui.ColumnView] and [class@Bobgui.ColumnViewColumn]
 *   instead of [class@Bobgui.TreeView] to show a tabular list
 */


/* Type methods */
static void bobgui_tree_view_column_cell_layout_init              (BobguiCellLayoutIface      *iface);

/* GObject methods */
static void bobgui_tree_view_column_set_property                  (GObject                 *object,
								guint                    prop_id,
								const GValue            *value,
								GParamSpec              *pspec);
static void bobgui_tree_view_column_get_property                  (GObject                 *object,
								guint                    prop_id,
								GValue                  *value,
								GParamSpec              *pspec);
static void bobgui_tree_view_column_finalize                      (GObject                 *object);
static void bobgui_tree_view_column_dispose                       (GObject                 *object);
static void bobgui_tree_view_column_constructed                   (GObject                 *object);

/* BobguiCellLayout implementation */
static void       bobgui_tree_view_column_ensure_cell_area        (BobguiTreeViewColumn      *column,
                                                                BobguiCellArea            *cell_area);

static BobguiCellArea *bobgui_tree_view_column_cell_layout_get_area  (BobguiCellLayout           *cell_layout);

/* Button handling code */
static void bobgui_tree_view_column_create_button                 (BobguiTreeViewColumn       *tree_column);
static void bobgui_tree_view_column_update_button                 (BobguiTreeViewColumn       *tree_column);

/* Button signal handlers */
static void column_button_drag_begin  (BobguiGestureDrag    *gesture,
                                       double             x,
                                       double             y,
                                       BobguiTreeViewColumn *column);
static void column_button_drag_update (BobguiGestureDrag    *gesture,
                                       double             offset_x,
                                       double             offset_y,
                                       BobguiTreeViewColumn *column);

static void bobgui_tree_view_column_button_clicked                (BobguiWidget               *widget,
								gpointer                 data);
static gboolean bobgui_tree_view_column_mnemonic_activate         (BobguiWidget *widget,
					                        gboolean   group_cycling,
								gpointer   data);

/* Property handlers */
static void bobgui_tree_view_model_sort_column_changed            (BobguiTreeSortable         *sortable,
								BobguiTreeViewColumn       *tree_column);

/* BobguiCellArea/BobguiCellAreaContext callbacks */
static void bobgui_tree_view_column_context_changed               (BobguiCellAreaContext      *context,
								GParamSpec              *pspec,
								BobguiTreeViewColumn       *tree_column);
static void bobgui_tree_view_column_add_editable_callback         (BobguiCellArea             *area,
								BobguiCellRenderer         *renderer,
								BobguiCellEditable         *edit_widget,
								GdkRectangle            *cell_area,
								const char              *path_string,
								gpointer                 user_data);
static void bobgui_tree_view_column_remove_editable_callback      (BobguiCellArea             *area,
								BobguiCellRenderer         *renderer,
								BobguiCellEditable         *edit_widget,
								gpointer                 user_data);

/* Internal functions */
static void bobgui_tree_view_column_sort                          (BobguiTreeViewColumn       *tree_column,
								gpointer                 data);
static void bobgui_tree_view_column_setup_sort_column_id_callback (BobguiTreeViewColumn       *tree_column);
static void bobgui_tree_view_column_set_attributesv               (BobguiTreeViewColumn       *tree_column,
								BobguiCellRenderer         *cell_renderer,
								va_list                  args);

/* BobguiBuildable implementation */
static void bobgui_tree_view_column_buildable_init                 (BobguiBuildableIface     *iface);

typedef struct _BobguiTreeViewColumnClass   BobguiTreeViewColumnClass;
typedef struct _BobguiTreeViewColumnPrivate BobguiTreeViewColumnPrivate;

struct _BobguiTreeViewColumn
{
  GInitiallyUnowned parent_instance;

  BobguiTreeViewColumnPrivate *priv;
};

struct _BobguiTreeViewColumnClass
{
  GInitiallyUnownedClass parent_class;

  void (*clicked) (BobguiTreeViewColumn *tree_column);
};


struct _BobguiTreeViewColumnPrivate
{
  BobguiWidget *tree_view;
  BobguiWidget *button;
  BobguiWidget *child;
  BobguiWidget *arrow;
  BobguiWidget *frame;
  gulong property_changed_signal;
  float xalign;

  /* Sizing fields */
  /* see bobgui+/doc/tree-column-sizing.txt for more information on them */
  BobguiTreeViewColumnSizing column_type;
  int padding;
  int x_offset;
  int width;
  int fixed_width;
  int min_width;
  int max_width;

  /* dragging columns */
  int drag_x;
  int drag_y;

  char *title;

  /* Sorting */
  gulong      sort_clicked_signal;
  gulong      sort_column_changed_signal;
  int         sort_column_id;
  BobguiSortType sort_order;

  /* Cell area */
  BobguiCellArea        *cell_area;
  BobguiCellAreaContext *cell_area_context;
  gulong              add_editable_signal;
  gulong              remove_editable_signal;
  gulong              context_changed_signal;

  /* Flags */
  guint visible             : 1;
  guint resizable           : 1;
  guint clickable           : 1;
  guint dirty               : 1;
  guint show_sort_indicator : 1;
  guint maybe_reordered     : 1;
  guint reorderable         : 1;
  guint expand              : 1;
};

enum
{
  PROP_0,
  PROP_VISIBLE,
  PROP_RESIZABLE,
  PROP_X_OFFSET,
  PROP_WIDTH,
  PROP_SPACING,
  PROP_SIZING,
  PROP_FIXED_WIDTH,
  PROP_MIN_WIDTH,
  PROP_MAX_WIDTH,
  PROP_TITLE,
  PROP_EXPAND,
  PROP_CLICKABLE,
  PROP_WIDGET,
  PROP_ALIGNMENT,
  PROP_REORDERABLE,
  PROP_SORT_INDICATOR,
  PROP_SORT_ORDER,
  PROP_SORT_COLUMN_ID,
  PROP_CELL_AREA,
  LAST_PROP
};

enum
{
  CLICKED,
  LAST_SIGNAL
};

static guint tree_column_signals[LAST_SIGNAL] = { 0 };
static GParamSpec *tree_column_props[LAST_PROP] = { NULL, };

G_DEFINE_TYPE_WITH_CODE (BobguiTreeViewColumn, bobgui_tree_view_column, G_TYPE_INITIALLY_UNOWNED,
                         G_ADD_PRIVATE (BobguiTreeViewColumn)
			 G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_CELL_LAYOUT,
						bobgui_tree_view_column_cell_layout_init)
			 G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_BUILDABLE,
						bobgui_tree_view_column_buildable_init))


static void
bobgui_tree_view_column_class_init (BobguiTreeViewColumnClass *class)
{
  GObjectClass *object_class;

  object_class = (GObjectClass*) class;

  class->clicked = NULL;

  object_class->constructed = bobgui_tree_view_column_constructed;
  object_class->finalize = bobgui_tree_view_column_finalize;
  object_class->dispose = bobgui_tree_view_column_dispose;
  object_class->set_property = bobgui_tree_view_column_set_property;
  object_class->get_property = bobgui_tree_view_column_get_property;

  /**
   * BobguiTreeViewColumn::clicked:
   * @column: the `BobguiTreeViewColumn` that emitted the signal
   *
   * Emitted when the column's header has been clicked.
   */
  tree_column_signals[CLICKED] =
    g_signal_new (I_("clicked"),
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BobguiTreeViewColumnClass, clicked),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);

  tree_column_props[PROP_VISIBLE] =
      g_param_spec_boolean ("visible", NULL, NULL,
                            TRUE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  tree_column_props[PROP_RESIZABLE] =
      g_param_spec_boolean ("resizable", NULL, NULL,
                            FALSE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  tree_column_props[PROP_X_OFFSET] =
      g_param_spec_int ("x-offset", NULL, NULL,
                        -G_MAXINT, G_MAXINT,
                        0,
                        BOBGUI_PARAM_READABLE);

  tree_column_props[PROP_WIDTH] =
      g_param_spec_int ("width", NULL, NULL,
                        0, G_MAXINT,
                        0,
                        BOBGUI_PARAM_READABLE);

  tree_column_props[PROP_SPACING] =
      g_param_spec_int ("spacing", NULL, NULL,
                        0, G_MAXINT,
                        0,
                        BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  tree_column_props[PROP_SIZING] =
      g_param_spec_enum ("sizing", NULL, NULL,
                         BOBGUI_TYPE_TREE_VIEW_COLUMN_SIZING,
                         BOBGUI_TREE_VIEW_COLUMN_GROW_ONLY,
                         BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  tree_column_props[PROP_FIXED_WIDTH] =
      g_param_spec_int ("fixed-width", NULL, NULL,
                         -1, G_MAXINT,
                         -1,
                         BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  tree_column_props[PROP_MIN_WIDTH] =
      g_param_spec_int ("min-width", NULL, NULL,
                        -1, G_MAXINT,
                        -1,
                        BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  tree_column_props[PROP_MAX_WIDTH] =
      g_param_spec_int ("max-width", NULL, NULL,
                        -1, G_MAXINT,
                        -1,
                        BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  tree_column_props[PROP_TITLE] =
      g_param_spec_string ("title", NULL, NULL,
                           "",
                           BOBGUI_PARAM_READWRITE);

  tree_column_props[PROP_EXPAND] =
      g_param_spec_boolean ("expand", NULL, NULL,
                            FALSE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  tree_column_props[PROP_CLICKABLE] =
      g_param_spec_boolean ("clickable", NULL, NULL,
                            FALSE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  tree_column_props[PROP_WIDGET] =
      g_param_spec_object ("widget", NULL, NULL,
                           BOBGUI_TYPE_WIDGET,
                           BOBGUI_PARAM_READWRITE);

  tree_column_props[PROP_ALIGNMENT] =
      g_param_spec_float ("alignment", NULL, NULL,
                          0.0, 1.0, 0.0,
                          BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  tree_column_props[PROP_REORDERABLE] =
      g_param_spec_boolean ("reorderable", NULL, NULL,
                            FALSE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  tree_column_props[PROP_SORT_INDICATOR] =
      g_param_spec_boolean ("sort-indicator", NULL, NULL,
                            FALSE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  tree_column_props[PROP_SORT_ORDER] =
      g_param_spec_enum ("sort-order", NULL, NULL,
                         BOBGUI_TYPE_SORT_TYPE,
                         BOBGUI_SORT_ASCENDING,
                         BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiTreeViewColumn:sort-column-id:
   *
   * Logical sort column ID this column sorts on when selected for sorting. Setting the sort column ID makes the column header
   * clickable. Set to -1 to make the column unsortable.
   **/
  tree_column_props[PROP_SORT_COLUMN_ID] =
      g_param_spec_int ("sort-column-id", NULL, NULL,
                        -1, G_MAXINT,
                        -1,
                        BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiTreeViewColumn:cell-area:
   *
   * The `BobguiCellArea` used to layout cell renderers for this column.
   *
   * If no area is specified when creating the tree view column with bobgui_tree_view_column_new_with_area()
   * a horizontally oriented `BobguiCellAreaBox` will be used.
   */
  tree_column_props[PROP_CELL_AREA] =
      g_param_spec_object ("cell-area", NULL, NULL,
                           BOBGUI_TYPE_CELL_AREA,
                           BOBGUI_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (object_class, LAST_PROP, tree_column_props);
}

static void
bobgui_tree_view_column_custom_tag_end (BobguiBuildable *buildable,
				     BobguiBuilder   *builder,
				     GObject      *child,
				     const char   *tagname,
				     gpointer      data)
{
  /* Just ignore the boolean return from here */
  _bobgui_cell_layout_buildable_custom_tag_end (buildable, builder, child, tagname, data);
}

static void
bobgui_tree_view_column_buildable_init (BobguiBuildableIface *iface)
{
  iface->add_child = _bobgui_cell_layout_buildable_add_child;
  iface->custom_tag_start = _bobgui_cell_layout_buildable_custom_tag_start;
  iface->custom_tag_end = bobgui_tree_view_column_custom_tag_end;
}

static void
bobgui_tree_view_column_cell_layout_init (BobguiCellLayoutIface *iface)
{
  iface->get_area = bobgui_tree_view_column_cell_layout_get_area;
}

static void
bobgui_tree_view_column_init (BobguiTreeViewColumn *tree_column)
{
  BobguiTreeViewColumnPrivate *priv;

  tree_column->priv = bobgui_tree_view_column_get_instance_private (tree_column);
  priv = tree_column->priv;

  priv->button = NULL;
  priv->xalign = 0.0;
  priv->width = 0;
  priv->padding = 0;
  priv->min_width = -1;
  priv->max_width = -1;
  priv->column_type = BOBGUI_TREE_VIEW_COLUMN_GROW_ONLY;
  priv->visible = TRUE;
  priv->resizable = FALSE;
  priv->expand = FALSE;
  priv->clickable = FALSE;
  priv->dirty = TRUE;
  priv->sort_order = BOBGUI_SORT_ASCENDING;
  priv->show_sort_indicator = FALSE;
  priv->property_changed_signal = 0;
  priv->sort_clicked_signal = 0;
  priv->sort_column_changed_signal = 0;
  priv->sort_column_id = -1;
  priv->reorderable = FALSE;
  priv->maybe_reordered = FALSE;
  priv->fixed_width = -1;
  priv->title = g_strdup ("");

  bobgui_tree_view_column_create_button (tree_column);
}

static void
bobgui_tree_view_column_constructed (GObject *object)
{
  BobguiTreeViewColumn *tree_column = BOBGUI_TREE_VIEW_COLUMN (object);

  G_OBJECT_CLASS (bobgui_tree_view_column_parent_class)->constructed (object);

  bobgui_tree_view_column_ensure_cell_area (tree_column, NULL);
}

static void
bobgui_tree_view_column_dispose (GObject *object)
{
  BobguiTreeViewColumn        *tree_column = (BobguiTreeViewColumn *) object;
  BobguiTreeViewColumnPrivate *priv        = tree_column->priv;

  /* Remove this column from its treeview,
   * in case this column is destroyed before its treeview.
   */
  if (priv->tree_view)
    bobgui_tree_view_remove_column (BOBGUI_TREE_VIEW (priv->tree_view), tree_column);

  if (priv->cell_area_context)
    {
      g_signal_handler_disconnect (priv->cell_area_context,
				   priv->context_changed_signal);

      g_object_unref (priv->cell_area_context);

      priv->cell_area_context = NULL;
      priv->context_changed_signal = 0;
    }

  if (priv->cell_area)
    {
      g_signal_handler_disconnect (priv->cell_area,
				   priv->add_editable_signal);
      g_signal_handler_disconnect (priv->cell_area,
				   priv->remove_editable_signal);

      g_object_unref (priv->cell_area);
      priv->cell_area = NULL;
      priv->add_editable_signal = 0;
      priv->remove_editable_signal = 0;
    }

  if (priv->child)
    {
      g_object_unref (priv->child);
      priv->child = NULL;
    }

  g_clear_object (&priv->button);

  G_OBJECT_CLASS (bobgui_tree_view_column_parent_class)->dispose (object);
}

static void
bobgui_tree_view_column_finalize (GObject *object)
{
  BobguiTreeViewColumn        *tree_column = (BobguiTreeViewColumn *) object;
  BobguiTreeViewColumnPrivate *priv        = tree_column->priv;

  g_free (priv->title);

  G_OBJECT_CLASS (bobgui_tree_view_column_parent_class)->finalize (object);
}

static void
bobgui_tree_view_column_set_property (GObject         *object,
                                   guint            prop_id,
                                   const GValue    *value,
                                   GParamSpec      *pspec)
{
  BobguiTreeViewColumn *tree_column;
  BobguiCellArea       *area;

  tree_column = BOBGUI_TREE_VIEW_COLUMN (object);

  switch (prop_id)
    {
    case PROP_VISIBLE:
      bobgui_tree_view_column_set_visible (tree_column,
                                        g_value_get_boolean (value));
      break;

    case PROP_RESIZABLE:
      bobgui_tree_view_column_set_resizable (tree_column,
					  g_value_get_boolean (value));
      break;

    case PROP_SIZING:
      bobgui_tree_view_column_set_sizing (tree_column,
                                       g_value_get_enum (value));
      break;

    case PROP_FIXED_WIDTH:
      bobgui_tree_view_column_set_fixed_width (tree_column,
					    g_value_get_int (value));
      break;

    case PROP_MIN_WIDTH:
      bobgui_tree_view_column_set_min_width (tree_column,
                                          g_value_get_int (value));
      break;

    case PROP_MAX_WIDTH:
      bobgui_tree_view_column_set_max_width (tree_column,
                                          g_value_get_int (value));
      break;

    case PROP_SPACING:
      bobgui_tree_view_column_set_spacing (tree_column,
					g_value_get_int (value));
      break;

    case PROP_TITLE:
      bobgui_tree_view_column_set_title (tree_column,
                                      g_value_get_string (value));
      break;

    case PROP_EXPAND:
      bobgui_tree_view_column_set_expand (tree_column,
				       g_value_get_boolean (value));
      break;

    case PROP_CLICKABLE:
      bobgui_tree_view_column_set_clickable (tree_column,
                                          g_value_get_boolean (value));
      break;

    case PROP_WIDGET:
      bobgui_tree_view_column_set_widget (tree_column,
                                       (BobguiWidget*) g_value_get_object (value));
      break;

    case PROP_ALIGNMENT:
      bobgui_tree_view_column_set_alignment (tree_column,
                                          g_value_get_float (value));
      break;

    case PROP_REORDERABLE:
      bobgui_tree_view_column_set_reorderable (tree_column,
					    g_value_get_boolean (value));
      break;

    case PROP_SORT_INDICATOR:
      bobgui_tree_view_column_set_sort_indicator (tree_column,
                                               g_value_get_boolean (value));
      break;

    case PROP_SORT_ORDER:
      bobgui_tree_view_column_set_sort_order (tree_column,
                                           g_value_get_enum (value));
      break;

    case PROP_SORT_COLUMN_ID:
      bobgui_tree_view_column_set_sort_column_id (tree_column,
                                               g_value_get_int (value));
      break;

    case PROP_CELL_AREA:
      /* Construct-only, can only be assigned once */
      area = g_value_get_object (value);

      if (area)
        {
          if (tree_column->priv->cell_area != NULL)
            {
              g_warning ("cell-area has already been set, ignoring construct property");
              g_object_ref_sink (area);
              g_object_unref (area);
            }
          else
            bobgui_tree_view_column_ensure_cell_area (tree_column, area);
        }
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_tree_view_column_get_property (GObject         *object,
                                   guint            prop_id,
                                   GValue          *value,
                                   GParamSpec      *pspec)
{
  BobguiTreeViewColumn *tree_column;

  tree_column = BOBGUI_TREE_VIEW_COLUMN (object);

  switch (prop_id)
    {
    case PROP_VISIBLE:
      g_value_set_boolean (value,
                           bobgui_tree_view_column_get_visible (tree_column));
      break;

    case PROP_RESIZABLE:
      g_value_set_boolean (value,
                           bobgui_tree_view_column_get_resizable (tree_column));
      break;

    case PROP_X_OFFSET:
      g_value_set_int (value,
                       bobgui_tree_view_column_get_x_offset (tree_column));
      break;

    case PROP_WIDTH:
      g_value_set_int (value,
                       bobgui_tree_view_column_get_width (tree_column));
      break;

    case PROP_SPACING:
      g_value_set_int (value,
                       bobgui_tree_view_column_get_spacing (tree_column));
      break;

    case PROP_SIZING:
      g_value_set_enum (value,
                        bobgui_tree_view_column_get_sizing (tree_column));
      break;

    case PROP_FIXED_WIDTH:
      g_value_set_int (value,
                       bobgui_tree_view_column_get_fixed_width (tree_column));
      break;

    case PROP_MIN_WIDTH:
      g_value_set_int (value,
                       bobgui_tree_view_column_get_min_width (tree_column));
      break;

    case PROP_MAX_WIDTH:
      g_value_set_int (value,
                       bobgui_tree_view_column_get_max_width (tree_column));
      break;

    case PROP_TITLE:
      g_value_set_string (value,
                          bobgui_tree_view_column_get_title (tree_column));
      break;

    case PROP_EXPAND:
      g_value_set_boolean (value,
                          bobgui_tree_view_column_get_expand (tree_column));
      break;

    case PROP_CLICKABLE:
      g_value_set_boolean (value,
                           bobgui_tree_view_column_get_clickable (tree_column));
      break;

    case PROP_WIDGET:
      g_value_set_object (value,
                          (GObject*) bobgui_tree_view_column_get_widget (tree_column));
      break;

    case PROP_ALIGNMENT:
      g_value_set_float (value,
                         bobgui_tree_view_column_get_alignment (tree_column));
      break;

    case PROP_REORDERABLE:
      g_value_set_boolean (value,
			   bobgui_tree_view_column_get_reorderable (tree_column));
      break;

    case PROP_SORT_INDICATOR:
      g_value_set_boolean (value,
                           bobgui_tree_view_column_get_sort_indicator (tree_column));
      break;

    case PROP_SORT_ORDER:
      g_value_set_enum (value,
                        bobgui_tree_view_column_get_sort_order (tree_column));
      break;

    case PROP_SORT_COLUMN_ID:
      g_value_set_int (value,
                       bobgui_tree_view_column_get_sort_column_id (tree_column));
      break;

    case PROP_CELL_AREA:
      g_value_set_object (value, tree_column->priv->cell_area);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

/* Implementation of BobguiCellLayout interface
 */

static void
bobgui_tree_view_column_ensure_cell_area (BobguiTreeViewColumn *column,
                                       BobguiCellArea       *cell_area)
{
  BobguiTreeViewColumnPrivate *priv = column->priv;

  if (priv->cell_area)
    return;

  if (cell_area)
    priv->cell_area = cell_area;
  else
    priv->cell_area = bobgui_cell_area_box_new ();

  g_object_ref_sink (priv->cell_area);

  priv->add_editable_signal =
    g_signal_connect (priv->cell_area, "add-editable",
                      G_CALLBACK (bobgui_tree_view_column_add_editable_callback),
                      column);
  priv->remove_editable_signal =
    g_signal_connect (priv->cell_area, "remove-editable",
                      G_CALLBACK (bobgui_tree_view_column_remove_editable_callback),
                      column);

  priv->cell_area_context = bobgui_cell_area_create_context (priv->cell_area);

  priv->context_changed_signal =
    g_signal_connect (priv->cell_area_context, "notify",
                      G_CALLBACK (bobgui_tree_view_column_context_changed),
                      column);
}

static BobguiCellArea *
bobgui_tree_view_column_cell_layout_get_area (BobguiCellLayout   *cell_layout)
{
  BobguiTreeViewColumn        *column = BOBGUI_TREE_VIEW_COLUMN (cell_layout);
  BobguiTreeViewColumnPrivate *priv   = column->priv;

  if (G_UNLIKELY (!priv->cell_area))
    bobgui_tree_view_column_ensure_cell_area (column, NULL);

  return priv->cell_area;
}

static void
focus_in (BobguiEventControllerKey *controller,
          BobguiTreeViewColumn     *column)
{
  _bobgui_tree_view_set_focus_column (BOBGUI_TREE_VIEW (column->priv->tree_view), column);
}

/* Button handling code
 */
static void
bobgui_tree_view_column_create_button (BobguiTreeViewColumn *tree_column)
{
  BobguiTreeViewColumnPrivate *priv = tree_column->priv;
  BobguiEventController *controller;
  BobguiWidget *child;
  BobguiWidget *hbox;

  g_return_if_fail (priv->button == NULL);

  priv->button = bobgui_button_new ();
  g_object_ref_sink (priv->button);
  bobgui_widget_set_focus_on_click (priv->button, FALSE);
  bobgui_widget_set_overflow (priv->button, BOBGUI_OVERFLOW_HIDDEN);

  g_signal_connect (priv->button, "clicked",
		    G_CALLBACK (bobgui_tree_view_column_button_clicked),
		    tree_column);

  controller = BOBGUI_EVENT_CONTROLLER (bobgui_gesture_drag_new ());
  g_signal_connect (controller, "drag-begin",
                    G_CALLBACK (column_button_drag_begin), tree_column);
  g_signal_connect (controller, "drag-update",
                    G_CALLBACK (column_button_drag_update), tree_column);
  bobgui_event_controller_set_propagation_phase (controller, BOBGUI_PHASE_CAPTURE);
  bobgui_widget_add_controller (priv->button, controller);

  controller = bobgui_event_controller_focus_new ();
  g_signal_connect (controller, "enter", G_CALLBACK (focus_in), tree_column);
  bobgui_widget_add_controller (priv->button, controller);

  priv->frame = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
  bobgui_widget_set_hexpand (priv->frame, TRUE);
  bobgui_widget_set_halign (priv->frame, BOBGUI_ALIGN_START);

  hbox = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 2);
  priv->arrow = bobgui_builtin_icon_new ("sort-indicator");

  if (priv->child)
    child = priv->child;
  else
    child = bobgui_label_new (priv->title);

  g_signal_connect (child, "mnemonic-activate",
		    G_CALLBACK (bobgui_tree_view_column_mnemonic_activate),
		    tree_column);

  if (priv->xalign <= 0.5)
    {
      bobgui_box_append (BOBGUI_BOX (hbox), priv->frame);
      bobgui_box_append (BOBGUI_BOX (hbox), priv->arrow);
    }
  else
    {
      bobgui_box_append (BOBGUI_BOX (hbox), priv->arrow);
      bobgui_box_append (BOBGUI_BOX (hbox), priv->frame);
    }

  bobgui_box_append (BOBGUI_BOX (priv->frame), child);
  bobgui_button_set_child (BOBGUI_BUTTON (priv->button), hbox);
}

static void
bobgui_tree_view_column_update_button (BobguiTreeViewColumn *tree_column)
{
  BobguiTreeViewColumnPrivate *priv = tree_column->priv;
  int sort_column_id = -1;
  BobguiWidget *hbox;
  BobguiWidget *frame;
  BobguiWidget *arrow;
  BobguiWidget *current_child;
  BobguiTreeModel *model;

  if (priv->tree_view)
    model = bobgui_tree_view_get_model (BOBGUI_TREE_VIEW (priv->tree_view));
  else
    model = NULL;

  hbox = bobgui_button_get_child (BOBGUI_BUTTON (priv->button));
  frame = priv->frame;
  arrow = priv->arrow;
  current_child = bobgui_widget_get_first_child (frame);

  /* Set up the actual button */
  if (priv->child)
    {
      if (current_child != priv->child)
        {
          bobgui_box_remove (BOBGUI_BOX (frame), current_child);
          bobgui_box_append (BOBGUI_BOX (frame), priv->child);
        }
    }
  else
    {
      if (current_child == NULL)
        {
          current_child = bobgui_label_new (NULL);
          bobgui_widget_show (current_child);
          bobgui_box_append (BOBGUI_BOX (frame), current_child);
        }

      g_return_if_fail (BOBGUI_IS_LABEL (current_child));

      if (priv->title)
        bobgui_label_set_text_with_mnemonic (BOBGUI_LABEL (current_child),
                                          priv->title);
      else
        bobgui_label_set_text_with_mnemonic (BOBGUI_LABEL (current_child),
                                          "");
    }

  if (BOBGUI_IS_TREE_SORTABLE (model))
    bobgui_tree_sortable_get_sort_column_id (BOBGUI_TREE_SORTABLE (model),
                                          &sort_column_id,
                                          NULL);

  if (priv->show_sort_indicator)
    {
      gboolean alternative;

      if (priv->tree_view)
        g_object_get (bobgui_widget_get_settings (priv->tree_view),
                      "bobgui-alternative-sort-arrows", &alternative,
                      NULL);
      else
        alternative = FALSE;

      if ((!alternative && priv->sort_order == BOBGUI_SORT_ASCENDING) ||
          (alternative && priv->sort_order == BOBGUI_SORT_DESCENDING))
        {
          bobgui_widget_remove_css_class (arrow, "ascending");
          bobgui_widget_add_css_class (arrow, "descending");
        }
      else
        {
          bobgui_widget_remove_css_class (arrow, "descending");
          bobgui_widget_add_css_class (arrow, "ascending");
        }
    }

  /* Put arrow on the right if the text is left-or-center justified, and on the
   * left otherwise; do this by packing boxes, so flipping text direction will
   * reverse things
   */
  if (priv->xalign <= 0.5)
    bobgui_box_reorder_child_after (BOBGUI_BOX (hbox), arrow, bobgui_widget_get_last_child (hbox));
  else
    bobgui_box_reorder_child_after (BOBGUI_BOX (hbox), arrow, NULL);

  if (priv->show_sort_indicator
      || (BOBGUI_IS_TREE_SORTABLE (model) && priv->sort_column_id >= 0))
    bobgui_widget_show (arrow);
  else
    bobgui_widget_hide (arrow);

  if (priv->show_sort_indicator)
    bobgui_widget_set_opacity (arrow, 1.0);
  else
    bobgui_widget_set_opacity (arrow, 0.0);

  /* It's always safe to hide the button.  It isn't always safe to show it, as
   * if you show it before it's realized, it'll get the wrong window. */
  if (priv->tree_view != NULL &&
      bobgui_widget_get_realized (priv->tree_view))
    {
      if (priv->visible &&
          bobgui_tree_view_get_headers_visible (BOBGUI_TREE_VIEW (priv->tree_view)))
	{
          bobgui_widget_show (priv->button);
	}
      else
	{
	  bobgui_widget_hide (priv->button);
	}
    }

  if (priv->reorderable || priv->clickable)
    {
      bobgui_widget_set_focusable (priv->button, TRUE);
    }
  else
    {
      bobgui_widget_set_focusable (priv->button, FALSE);
      if (bobgui_widget_has_focus (priv->button))
	{
          BobguiRoot *root = bobgui_widget_get_root (priv->tree_view);
	  bobgui_root_set_focus (root, NULL);
	}
    }
  /* Queue a resize on the assumption that we always want to catch all changes
   * and columns don't change all that often.
   */
  if (priv->tree_view && bobgui_widget_get_realized (priv->tree_view))
     bobgui_widget_queue_resize (priv->tree_view);
}

/* Button signal handlers
 */

static void
column_button_drag_begin (BobguiGestureDrag    *gesture,
                          double             x,
                          double             y,
                          BobguiTreeViewColumn *column)
{
  BobguiTreeViewColumnPrivate *priv = column->priv;

  if (!priv->reorderable)
    {
      bobgui_gesture_set_state (BOBGUI_GESTURE (gesture),
                             BOBGUI_EVENT_SEQUENCE_DENIED);
      return;
    }

  priv->drag_x = x;
  priv->drag_y = y;
  bobgui_widget_grab_focus (priv->button);
}

static void
column_button_drag_update (BobguiGestureDrag    *gesture,
                           double             offset_x,
                           double             offset_y,
                           BobguiTreeViewColumn *column)
{
  BobguiTreeViewColumnPrivate *priv = column->priv;

  if (bobgui_drag_check_threshold_double (priv->button, 0, 0, offset_x, offset_y))
    {
      bobgui_gesture_set_state (BOBGUI_GESTURE (gesture), BOBGUI_EVENT_SEQUENCE_CLAIMED);
      _bobgui_tree_view_column_start_drag (BOBGUI_TREE_VIEW (priv->tree_view), column,
                                        bobgui_gesture_get_device (BOBGUI_GESTURE (gesture)));
    }
}

static void
bobgui_tree_view_column_button_clicked (BobguiWidget *widget, gpointer data)
{
  g_signal_emit_by_name (data, "clicked");
}

static gboolean
bobgui_tree_view_column_mnemonic_activate (BobguiWidget *widget,
					gboolean   group_cycling,
					gpointer   data)
{
  BobguiTreeViewColumn        *column = (BobguiTreeViewColumn *)data;
  BobguiTreeViewColumnPrivate *priv   = column->priv;

  g_return_val_if_fail (BOBGUI_IS_TREE_VIEW_COLUMN (column), FALSE);

  _bobgui_tree_view_set_focus_column (BOBGUI_TREE_VIEW (priv->tree_view), column);

  if (priv->clickable)
     g_signal_emit_by_name (priv->button, "clicked");
  else if (bobgui_widget_get_focusable (priv->button))
    bobgui_widget_grab_focus (priv->button);
  else
    bobgui_widget_grab_focus (priv->tree_view);

  return TRUE;
}

static void
bobgui_tree_view_model_sort_column_changed (BobguiTreeSortable   *sortable,
					 BobguiTreeViewColumn *column)
{
  BobguiTreeViewColumnPrivate *priv = column->priv;
  int sort_column_id;
  BobguiSortType order;

  if (bobgui_tree_sortable_get_sort_column_id (sortable,
					    &sort_column_id,
					    &order))
    {
      if (sort_column_id == priv->sort_column_id)
	{
	  bobgui_tree_view_column_set_sort_indicator (column, TRUE);
	  bobgui_tree_view_column_set_sort_order (column, order);
	}
      else
	{
	  bobgui_tree_view_column_set_sort_indicator (column, FALSE);
	}
    }
  else
    {
      bobgui_tree_view_column_set_sort_indicator (column, FALSE);
    }
}

static void
bobgui_tree_view_column_sort (BobguiTreeViewColumn *tree_column,
			   gpointer           data)
{
  BobguiTreeViewColumnPrivate *priv = tree_column->priv;
  BobguiTreeModel *model;
  BobguiTreeSortable *sortable;
  int sort_column_id;
  BobguiSortType order;
  gboolean has_sort_column;
  gboolean has_default_sort_func;

  g_return_if_fail (priv->tree_view != NULL);

  model    = bobgui_tree_view_get_model (BOBGUI_TREE_VIEW (priv->tree_view));
  sortable = BOBGUI_TREE_SORTABLE (model);

  has_sort_column =
    bobgui_tree_sortable_get_sort_column_id (sortable,
					  &sort_column_id,
					  &order);
  has_default_sort_func =
    bobgui_tree_sortable_has_default_sort_func (sortable);

  if (has_sort_column &&
      sort_column_id == priv->sort_column_id)
    {
      if (order == BOBGUI_SORT_ASCENDING)
	bobgui_tree_sortable_set_sort_column_id (sortable,
					      priv->sort_column_id,
					      BOBGUI_SORT_DESCENDING);
      else if (order == BOBGUI_SORT_DESCENDING && has_default_sort_func)
	bobgui_tree_sortable_set_sort_column_id (sortable,
					      BOBGUI_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID,
					      BOBGUI_SORT_ASCENDING);
      else
	bobgui_tree_sortable_set_sort_column_id (sortable,
					      priv->sort_column_id,
					      BOBGUI_SORT_ASCENDING);
    }
  else
    {
      bobgui_tree_sortable_set_sort_column_id (sortable,
					    priv->sort_column_id,
					    BOBGUI_SORT_ASCENDING);
    }
}

static void
bobgui_tree_view_column_setup_sort_column_id_callback (BobguiTreeViewColumn *tree_column)
{
  BobguiTreeViewColumnPrivate *priv = tree_column->priv;
  BobguiTreeModel *model;

  if (priv->tree_view == NULL)
    return;

  model = bobgui_tree_view_get_model (BOBGUI_TREE_VIEW (priv->tree_view));

  if (model == NULL)
    return;

  if (BOBGUI_IS_TREE_SORTABLE (model) &&
      priv->sort_column_id != -1)
    {
      int real_sort_column_id;
      BobguiSortType real_order;

      if (priv->sort_column_changed_signal == 0)
        priv->sort_column_changed_signal =
	  g_signal_connect (model, "sort-column-changed",
			    G_CALLBACK (bobgui_tree_view_model_sort_column_changed),
			    tree_column);

      if (bobgui_tree_sortable_get_sort_column_id (BOBGUI_TREE_SORTABLE (model),
						&real_sort_column_id,
						&real_order) &&
	  (real_sort_column_id == priv->sort_column_id))
	{
	  bobgui_tree_view_column_set_sort_indicator (tree_column, TRUE);
	  bobgui_tree_view_column_set_sort_order (tree_column, real_order);
 	}
      else
	{
	  bobgui_tree_view_column_set_sort_indicator (tree_column, FALSE);
	}
   }
}

static void
bobgui_tree_view_column_context_changed  (BobguiCellAreaContext      *context,
				       GParamSpec              *pspec,
				       BobguiTreeViewColumn       *tree_column)
{
  /* Here we want the column re-requested if the underlying context was
   * actually reset for any reason, this can happen if the underlying
   * area/cell configuration changes (i.e. cell packing properties
   * or cell spacing and the like)
   *
   * Note that we block this handler while requesting for sizes
   * so there is no need to check for the new context size being -1,
   * we also block the handler when explicitly resetting the context
   * so as to avoid some infinite stack recursion.
   */
  if (!strcmp (pspec->name, "minimum-width") ||
      !strcmp (pspec->name, "natural-width") ||
      !strcmp (pspec->name, "minimum-height") ||
      !strcmp (pspec->name, "natural-height"))
    _bobgui_tree_view_column_cell_set_dirty (tree_column, TRUE);
}

static void
bobgui_tree_view_column_add_editable_callback (BobguiCellArea       *area,
                                            BobguiCellRenderer   *renderer,
                                            BobguiCellEditable   *edit_widget,
                                            GdkRectangle      *cell_area,
                                            const char        *path_string,
                                            gpointer           user_data)
{
  BobguiTreeViewColumn        *column = user_data;
  BobguiTreeViewColumnPrivate *priv   = column->priv;
  BobguiTreePath              *path;

  if (priv->tree_view)
    {
      path = bobgui_tree_path_new_from_string (path_string);

      _bobgui_tree_view_add_editable (BOBGUI_TREE_VIEW (priv->tree_view),
				   column,
				   path,
				   edit_widget,
				   cell_area);

      bobgui_tree_path_free (path);
    }
}

static void
bobgui_tree_view_column_remove_editable_callback (BobguiCellArea     *area,
                                               BobguiCellRenderer *renderer,
                                               BobguiCellEditable *edit_widget,
                                               gpointer         user_data)
{
  BobguiTreeViewColumn        *column = user_data;
  BobguiTreeViewColumnPrivate *priv   = column->priv;

  if (priv->tree_view)
    _bobgui_tree_view_remove_editable (BOBGUI_TREE_VIEW (priv->tree_view),
				    column,
				    edit_widget);
}

/* Exported Private Functions.
 * These should only be called by bobguitreeview.c or bobguitreeviewcolumn.c
 */
void
_bobgui_tree_view_column_realize_button (BobguiTreeViewColumn *column)
{
  g_return_if_fail (BOBGUI_IS_TREE_VIEW (column->priv->tree_view));
  g_return_if_fail (bobgui_widget_get_realized (column->priv->tree_view));
  g_return_if_fail (column->priv->button != NULL);

  bobgui_tree_view_column_update_button (column);
}

void
_bobgui_tree_view_column_unset_model (BobguiTreeViewColumn *column,
				   BobguiTreeModel      *old_model)
{
  BobguiTreeViewColumnPrivate *priv = column->priv;

  if (priv->sort_column_changed_signal)
    {
      g_signal_handler_disconnect (old_model,
				   priv->sort_column_changed_signal);
      priv->sort_column_changed_signal = 0;
    }
  bobgui_tree_view_column_set_sort_indicator (column, FALSE);
}

void
_bobgui_tree_view_column_set_tree_view (BobguiTreeViewColumn *column,
				     BobguiTreeView       *tree_view)
{
  BobguiTreeViewColumnPrivate *priv = column->priv;

  g_assert (priv->tree_view == NULL);

  priv->tree_view = BOBGUI_WIDGET (tree_view);

  /* avoid a warning with our messed up CSS nodes */
  bobgui_widget_insert_after (priv->button, BOBGUI_WIDGET (tree_view), NULL);

  priv->property_changed_signal =
    g_signal_connect_swapped (tree_view,
			      "notify::model",
			      G_CALLBACK (bobgui_tree_view_column_setup_sort_column_id_callback),
			      column);

  bobgui_tree_view_column_setup_sort_column_id_callback (column);
}

void
_bobgui_tree_view_column_unset_tree_view (BobguiTreeViewColumn *column)
{
  BobguiTreeViewColumnPrivate *priv = column->priv;

  if (priv->tree_view == NULL)
    return;

  bobgui_widget_unparent (priv->button);

  if (priv->property_changed_signal)
    {
      g_signal_handler_disconnect (priv->tree_view, priv->property_changed_signal);
      priv->property_changed_signal = 0;
    }

  if (priv->sort_column_changed_signal)
    {
      g_signal_handler_disconnect (bobgui_tree_view_get_model (BOBGUI_TREE_VIEW (priv->tree_view)),
                                   priv->sort_column_changed_signal);
      priv->sort_column_changed_signal = 0;
    }

  priv->tree_view = NULL;
}

gboolean
_bobgui_tree_view_column_has_editable_cell (BobguiTreeViewColumn *column)
{
  BobguiTreeViewColumnPrivate *priv = column->priv;
  gboolean ret = FALSE;
  GList *list, *cells;

  cells = bobgui_cell_layout_get_cells (BOBGUI_CELL_LAYOUT (priv->cell_area));

  for (list = cells; list; list = list->next)
    {
      BobguiCellRenderer *cell = list->data;
      BobguiCellRendererMode mode;

      g_object_get (cell, "mode", &mode, NULL);

      if (mode == BOBGUI_CELL_RENDERER_MODE_EDITABLE)
        {
          ret = TRUE;
          break;
        }
    }

  g_list_free (cells);

  return ret;
}

/* gets cell being edited */
BobguiCellRenderer *
_bobgui_tree_view_column_get_edited_cell (BobguiTreeViewColumn *column)
{
  BobguiTreeViewColumnPrivate *priv = column->priv;

  return bobgui_cell_area_get_edited_cell (priv->cell_area);
}

BobguiCellRenderer *
_bobgui_tree_view_column_get_cell_at_pos (BobguiTreeViewColumn *column,
                                       GdkRectangle      *cell_area,
                                       GdkRectangle      *background_area,
                                       int                x,
                                       int                y)
{
  BobguiCellRenderer *match = NULL;
  BobguiTreeViewColumnPrivate *priv = column->priv;

  /* If (x, y) is outside of the background area, immediately return */
  if (x < background_area->x ||
      x > background_area->x + background_area->width ||
      y < background_area->y ||
      y > background_area->y + background_area->height)
    return NULL;

  /* If (x, y) is inside the background area, clamp it to the cell_area
   * so that a cell is still returned.  The main reason for doing this
   * (on the x axis) is for handling clicks in the indentation area
   * (either at the left or right depending on RTL setting).  Another
   * reason is for handling clicks on the area where the focus rectangle
   * is drawn (this is outside of cell area), this manifests itself
   * mainly when a large setting is used for focus-line-width.
   */
  if (x < cell_area->x)
    x = cell_area->x;
  else if (x > cell_area->x + cell_area->width)
    x = cell_area->x + cell_area->width;

  if (y < cell_area->y)
    y = cell_area->y;
  else if (y > cell_area->y + cell_area->height)
    y = cell_area->y + cell_area->height;

  match = bobgui_cell_area_get_cell_at_position (priv->cell_area,
                                              priv->cell_area_context,
                                              priv->tree_view,
                                              cell_area,
                                              x, y,
                                              NULL);

  return match;
}

gboolean
_bobgui_tree_view_column_is_blank_at_pos (BobguiTreeViewColumn *column,
                                       GdkRectangle      *cell_area,
                                       GdkRectangle      *background_area,
                                       int                x,
                                       int                y)
{
  BobguiCellRenderer *match;
  GdkRectangle cell_alloc, aligned_area, inner_area;
  BobguiTreeViewColumnPrivate *priv = column->priv;

  match = _bobgui_tree_view_column_get_cell_at_pos (column,
                                                 cell_area,
                                                 background_area,
                                                 x, y);
  if (!match)
    return FALSE;

  bobgui_cell_area_get_cell_allocation (priv->cell_area,
                                     priv->cell_area_context,
                                     priv->tree_view,
                                     match,
                                     cell_area,
                                     &cell_alloc);

  bobgui_cell_area_inner_cell_area (priv->cell_area, priv->tree_view,
                                 &cell_alloc, &inner_area);
  bobgui_cell_renderer_get_aligned_area (match, priv->tree_view, 0,
                                      &inner_area, &aligned_area);

  if (x < aligned_area.x ||
      x > aligned_area.x + aligned_area.width ||
      y < aligned_area.y ||
      y > aligned_area.y + aligned_area.height)
    return TRUE;

  return FALSE;
}

/* Public Functions */


/**
 * bobgui_tree_view_column_new:
 *
 * Creates a new `BobguiTreeViewColumn`.
 *
 * Returns: A newly created `BobguiTreeViewColumn`.
 *
 * Deprecated: 4.10: Use BobguiColumnView instead
 **/
BobguiTreeViewColumn *
bobgui_tree_view_column_new (void)
{
  BobguiTreeViewColumn *tree_column;

  tree_column = g_object_new (BOBGUI_TYPE_TREE_VIEW_COLUMN, NULL);

  return tree_column;
}

/**
 * bobgui_tree_view_column_new_with_area:
 * @area: the `BobguiCellArea` that the newly created column should use to layout cells.
 *
 * Creates a new `BobguiTreeViewColumn` using @area to render its cells.
 *
 * Returns: A newly created `BobguiTreeViewColumn`.
 *
 * Deprecated: 4.10: Use BobguiColumnView instead
 */
BobguiTreeViewColumn *
bobgui_tree_view_column_new_with_area (BobguiCellArea *area)
{
  BobguiTreeViewColumn *tree_column;

  tree_column = g_object_new (BOBGUI_TYPE_TREE_VIEW_COLUMN, "cell-area", area, NULL);

  return tree_column;
}


/**
 * bobgui_tree_view_column_new_with_attributes:
 * @title: The title to set the header to
 * @cell: The `BobguiCellRenderer`
 * @...: A %NULL-terminated list of attributes
 *
 * Creates a new `BobguiTreeViewColumn` with a number of default values.
 * This is equivalent to calling bobgui_tree_view_column_set_title(),
 * bobgui_tree_view_column_pack_start(), and
 * bobgui_tree_view_column_set_attributes() on the newly created `BobguiTreeViewColumn`.
 *
 * Here’s a simple example:
 *
 * ```c
 *  enum { TEXT_COLUMN, COLOR_COLUMN, N_COLUMNS };
 *  // ...
 *  {
 *    BobguiTreeViewColumn *column;
 *    BobguiCellRenderer   *renderer = bobgui_cell_renderer_text_new ();
 *
 *    column = bobgui_tree_view_column_new_with_attributes ("Title",
 *                                                       renderer,
 *                                                       "text", TEXT_COLUMN,
 *                                                       "foreground", COLOR_COLUMN,
 *                                                       NULL);
 *  }
 * ```
 *
 * Returns: A newly created `BobguiTreeViewColumn`.
 *
 * Deprecated: 4.10: Use BobguiColumnView instead
 **/
BobguiTreeViewColumn *
bobgui_tree_view_column_new_with_attributes (const char      *title,
					  BobguiCellRenderer *cell,
					  ...)
{
  BobguiTreeViewColumn *retval;
  va_list args;

  retval = bobgui_tree_view_column_new ();

  bobgui_tree_view_column_set_title (retval, title);
  bobgui_tree_view_column_pack_start (retval, cell, TRUE);

  va_start (args, cell);
  bobgui_tree_view_column_set_attributesv (retval, cell, args);
  va_end (args);

  return retval;
}

/**
 * bobgui_tree_view_column_pack_start:
 * @tree_column: A `BobguiTreeViewColumn`.
 * @cell: The `BobguiCellRenderer`
 * @expand: %TRUE if @cell is to be given extra space allocated to @tree_column.
 *
 * Packs the @cell into the beginning of the column. If @expand is %FALSE, then
 * the @cell is allocated no more space than it needs. Any unused space is divided
 * evenly between cells for which @expand is %TRUE.
 *
 * Deprecated: 4.10: Use BobguiColumnView instead
 **/
void
bobgui_tree_view_column_pack_start (BobguiTreeViewColumn *tree_column,
				 BobguiCellRenderer   *cell,
				 gboolean           expand)
{
  bobgui_cell_layout_pack_start (BOBGUI_CELL_LAYOUT (tree_column), cell, expand);
}

/**
 * bobgui_tree_view_column_pack_end:
 * @tree_column: A `BobguiTreeViewColumn`.
 * @cell: The `BobguiCellRenderer`
 * @expand: %TRUE if @cell is to be given extra space allocated to @tree_column.
 *
 * Adds the @cell to end of the column. If @expand is %FALSE, then the @cell
 * is allocated no more space than it needs. Any unused space is divided
 * evenly between cells for which @expand is %TRUE.
 *
 * Deprecated: 4.10: Use BobguiColumnView instead
 **/
void
bobgui_tree_view_column_pack_end (BobguiTreeViewColumn  *tree_column,
			       BobguiCellRenderer    *cell,
			       gboolean            expand)
{
  bobgui_cell_layout_pack_end (BOBGUI_CELL_LAYOUT (tree_column), cell, expand);
}

/**
 * bobgui_tree_view_column_clear:
 * @tree_column: A `BobguiTreeViewColumn`
 *
 * Unsets all the mappings on all renderers on the @tree_column.
 *
 * Deprecated: 4.10: Use BobguiColumnView instead
 **/
void
bobgui_tree_view_column_clear (BobguiTreeViewColumn *tree_column)
{
  bobgui_cell_layout_clear (BOBGUI_CELL_LAYOUT (tree_column));
}

/**
 * bobgui_tree_view_column_add_attribute:
 * @tree_column: A `BobguiTreeViewColumn`
 * @cell_renderer: the `BobguiCellRenderer` to set attributes on
 * @attribute: An attribute on the renderer
 * @column: The column position on the model to get the attribute from.
 *
 * Adds an attribute mapping to the list in @tree_column.
 *
 * The @column is the
 * column of the model to get a value from, and the @attribute is the
 * parameter on @cell_renderer to be set from the value. So for example
 * if column 2 of the model contains strings, you could have the
 * “text” attribute of a `BobguiCellRendererText` get its values from
 * column 2.
 *
 * Deprecated: 4.10: Use BobguiColumnView instead
 **/
void
bobgui_tree_view_column_add_attribute (BobguiTreeViewColumn *tree_column,
				    BobguiCellRenderer   *cell_renderer,
				    const char        *attribute,
				    int                column)
{
  bobgui_cell_layout_add_attribute (BOBGUI_CELL_LAYOUT (tree_column),
                                 cell_renderer, attribute, column);
}

static void
bobgui_tree_view_column_set_attributesv (BobguiTreeViewColumn *tree_column,
				      BobguiCellRenderer   *cell_renderer,
				      va_list            args)
{
  BobguiTreeViewColumnPrivate *priv = tree_column->priv;
  char *attribute;
  int column;

  attribute = va_arg (args, char *);

  bobgui_cell_layout_clear_attributes (BOBGUI_CELL_LAYOUT (priv->cell_area),
                                    cell_renderer);

  while (attribute != NULL)
    {
      column = va_arg (args, int);
      bobgui_cell_layout_add_attribute (BOBGUI_CELL_LAYOUT (priv->cell_area),
                                     cell_renderer, attribute, column);
      attribute = va_arg (args, char *);
    }
}

/**
 * bobgui_tree_view_column_set_attributes:
 * @tree_column: A `BobguiTreeViewColumn`
 * @cell_renderer: the `BobguiCellRenderer` we’re setting the attributes of
 * @...: A %NULL-terminated list of attributes
 *
 * Sets the attributes in the list as the attributes of @tree_column.
 *
 * The attributes should be in attribute/column order, as in
 * bobgui_tree_view_column_add_attribute(). All existing attributes
 * are removed, and replaced with the new attributes.
 *
 * Deprecated: 4.10: Use BobguiColumnView instead
 */
void
bobgui_tree_view_column_set_attributes (BobguiTreeViewColumn *tree_column,
				     BobguiCellRenderer   *cell_renderer,
				     ...)
{
  va_list args;

  g_return_if_fail (BOBGUI_IS_TREE_VIEW_COLUMN (tree_column));
  g_return_if_fail (BOBGUI_IS_CELL_RENDERER (cell_renderer));

  va_start (args, cell_renderer);
  bobgui_tree_view_column_set_attributesv (tree_column, cell_renderer, args);
  va_end (args);
}


/**
 * bobgui_tree_view_column_set_cell_data_func:
 * @tree_column: A `BobguiTreeViewColumn`
 * @cell_renderer: A `BobguiCellRenderer`
 * @func: (nullable) (scope notified) (closure func_data) (destroy destroy): The `BobguiTreeCellDataFunc` to use.
 * @func_data: The user data for @func.
 * @destroy: The destroy notification for @func_data
 *
 * Sets the `BobguiTreeCellDataFunc` to use for the column.
 *
 * This
 * function is used instead of the standard attributes mapping for
 * setting the column value, and should set the value of @tree_column's
 * cell renderer as appropriate.  @func may be %NULL to remove an
 * older one.
 *
 * Deprecated: 4.10: Use BobguiColumnView instead
 **/
void
bobgui_tree_view_column_set_cell_data_func (BobguiTreeViewColumn   *tree_column,
					 BobguiCellRenderer     *cell_renderer,
					 BobguiTreeCellDataFunc  func,
					 gpointer             func_data,
					 GDestroyNotify       destroy)
{
  bobgui_cell_layout_set_cell_data_func (BOBGUI_CELL_LAYOUT (tree_column),
                                      cell_renderer,
                                      (BobguiCellLayoutDataFunc)func,
                                      func_data, destroy);
}


/**
 * bobgui_tree_view_column_clear_attributes:
 * @tree_column: a `BobguiTreeViewColumn`
 * @cell_renderer: a `BobguiCellRenderer` to clear the attribute mapping on.
 *
 * Clears all existing attributes previously set with
 * bobgui_tree_view_column_set_attributes().
 *
 * Deprecated: 4.10: Use BobguiColumnView instead
 **/
void
bobgui_tree_view_column_clear_attributes (BobguiTreeViewColumn *tree_column,
				       BobguiCellRenderer   *cell_renderer)
{
  bobgui_cell_layout_clear_attributes (BOBGUI_CELL_LAYOUT (tree_column),
                                    cell_renderer);
}

/**
 * bobgui_tree_view_column_set_spacing:
 * @tree_column: A `BobguiTreeViewColumn`.
 * @spacing: distance between cell renderers in pixels.
 *
 * Sets the spacing field of @tree_column, which is the number of pixels to
 * place between cell renderers packed into it.
 *
 * Deprecated: 4.10: Use BobguiColumnView instead
 **/
void
bobgui_tree_view_column_set_spacing (BobguiTreeViewColumn *tree_column,
				  int                spacing)
{
  BobguiTreeViewColumnPrivate *priv;

  g_return_if_fail (BOBGUI_IS_TREE_VIEW_COLUMN (tree_column));
  g_return_if_fail (spacing >= 0);

  priv = tree_column->priv;

  if (bobgui_cell_area_box_get_spacing (BOBGUI_CELL_AREA_BOX (priv->cell_area)) != spacing)
    {
      bobgui_cell_area_box_set_spacing (BOBGUI_CELL_AREA_BOX (priv->cell_area), spacing);
      if (priv->tree_view)
        _bobgui_tree_view_column_cell_set_dirty (tree_column, TRUE);
      g_object_notify_by_pspec (G_OBJECT (tree_column), tree_column_props[PROP_SPACING]);
    }
}

/**
 * bobgui_tree_view_column_get_spacing:
 * @tree_column: A `BobguiTreeViewColumn`.
 *
 * Returns the spacing of @tree_column.
 *
 * Returns: the spacing of @tree_column.
 *
 * Deprecated: 4.10: Use BobguiColumnView instead
 **/
int
bobgui_tree_view_column_get_spacing (BobguiTreeViewColumn *tree_column)
{
  BobguiTreeViewColumnPrivate *priv;

  g_return_val_if_fail (BOBGUI_IS_TREE_VIEW_COLUMN (tree_column), 0);

  priv = tree_column->priv;

  return bobgui_cell_area_box_get_spacing (BOBGUI_CELL_AREA_BOX (priv->cell_area));
}

/* Options for manipulating the columns */

/**
 * bobgui_tree_view_column_set_visible:
 * @tree_column: A `BobguiTreeViewColumn`.
 * @visible: %TRUE if the @tree_column is visible.
 *
 * Sets the visibility of @tree_column.
 *
 * Deprecated: 4.10: Use BobguiColumnView instead
 */
void
bobgui_tree_view_column_set_visible (BobguiTreeViewColumn *tree_column,
                                  gboolean           visible)
{
  BobguiTreeViewColumnPrivate *priv;

  g_return_if_fail (BOBGUI_IS_TREE_VIEW_COLUMN (tree_column));

  priv    = tree_column->priv;
  visible = !! visible;

  if (priv->visible == visible)
    return;

  priv->visible = visible;

  bobgui_widget_set_visible (priv->button, visible);

  if (priv->visible)
    _bobgui_tree_view_column_cell_set_dirty (tree_column, TRUE);

  bobgui_tree_view_column_update_button (tree_column);
  g_object_notify_by_pspec (G_OBJECT (tree_column), tree_column_props[PROP_VISIBLE]);
}

/**
 * bobgui_tree_view_column_get_visible:
 * @tree_column: A `BobguiTreeViewColumn`.
 *
 * Returns %TRUE if @tree_column is visible.
 *
 * Returns: whether the column is visible or not.  If it is visible, then
 * the tree will show the column.
 *
 * Deprecated: 4.10: Use BobguiColumnView instead
 **/
gboolean
bobgui_tree_view_column_get_visible (BobguiTreeViewColumn *tree_column)
{
  g_return_val_if_fail (BOBGUI_IS_TREE_VIEW_COLUMN (tree_column), FALSE);

  return tree_column->priv->visible;
}

/**
 * bobgui_tree_view_column_set_resizable:
 * @tree_column: A `BobguiTreeViewColumn`
 * @resizable: %TRUE, if the column can be resized
 *
 * If @resizable is %TRUE, then the user can explicitly resize the column by
 * grabbing the outer edge of the column button.
 *
 * If resizable is %TRUE and
 * sizing mode of the column is %BOBGUI_TREE_VIEW_COLUMN_AUTOSIZE, then the sizing
 * mode is changed to %BOBGUI_TREE_VIEW_COLUMN_GROW_ONLY.
 *
 * Deprecated: 4.10: Use BobguiColumnView instead
 **/
void
bobgui_tree_view_column_set_resizable (BobguiTreeViewColumn *tree_column,
				    gboolean           resizable)
{
  BobguiTreeViewColumnPrivate *priv;

  g_return_if_fail (BOBGUI_IS_TREE_VIEW_COLUMN (tree_column));

  priv      = tree_column->priv;
  resizable = !! resizable;

  if (priv->resizable == resizable)
    return;

  priv->resizable = resizable;

  if (resizable && priv->column_type == BOBGUI_TREE_VIEW_COLUMN_AUTOSIZE)
    bobgui_tree_view_column_set_sizing (tree_column, BOBGUI_TREE_VIEW_COLUMN_GROW_ONLY);

  bobgui_tree_view_column_update_button (tree_column);

  g_object_notify_by_pspec (G_OBJECT (tree_column), tree_column_props[PROP_RESIZABLE]);
}

/**
 * bobgui_tree_view_column_get_resizable:
 * @tree_column: A `BobguiTreeViewColumn`
 *
 * Returns %TRUE if the @tree_column can be resized by the end user.
 *
 * Returns: %TRUE, if the @tree_column can be resized.
 *
 * Deprecated: 4.10: Use BobguiColumnView instead
 **/
gboolean
bobgui_tree_view_column_get_resizable (BobguiTreeViewColumn *tree_column)
{
  g_return_val_if_fail (BOBGUI_IS_TREE_VIEW_COLUMN (tree_column), FALSE);

  return tree_column->priv->resizable;
}


/**
 * bobgui_tree_view_column_set_sizing:
 * @tree_column: A `BobguiTreeViewColumn`.
 * @type: The `BobguiTreeViewColumn`Sizing.
 *
 * Sets the growth behavior of @tree_column to @type.
 *
 * Deprecated: 4.10: Use BobguiColumnView instead
 **/
void
bobgui_tree_view_column_set_sizing (BobguiTreeViewColumn       *tree_column,
                                 BobguiTreeViewColumnSizing  type)
{
  BobguiTreeViewColumnPrivate *priv;

  g_return_if_fail (BOBGUI_IS_TREE_VIEW_COLUMN (tree_column));

  priv      = tree_column->priv;

  if (type == priv->column_type)
    return;

  if (type == BOBGUI_TREE_VIEW_COLUMN_AUTOSIZE)
    bobgui_tree_view_column_set_resizable (tree_column, FALSE);

  priv->column_type = type;

  bobgui_tree_view_column_update_button (tree_column);

  g_object_notify_by_pspec (G_OBJECT (tree_column), tree_column_props[PROP_SIZING]);
}

/**
 * bobgui_tree_view_column_get_sizing:
 * @tree_column: A `BobguiTreeViewColumn`.
 *
 * Returns the current type of @tree_column.
 *
 * Returns: The type of @tree_column.
 *
 * Deprecated: 4.10: Use BobguiColumnView instead
 **/
BobguiTreeViewColumnSizing
bobgui_tree_view_column_get_sizing (BobguiTreeViewColumn *tree_column)
{
  g_return_val_if_fail (BOBGUI_IS_TREE_VIEW_COLUMN (tree_column), 0);

  return tree_column->priv->column_type;
}

/**
 * bobgui_tree_view_column_get_width:
 * @tree_column: A `BobguiTreeViewColumn`.
 *
 * Returns the current size of @tree_column in pixels.
 *
 * Returns: The current width of @tree_column.
 *
 * Deprecated: 4.10: Use BobguiColumnView instead
 **/
int
bobgui_tree_view_column_get_width (BobguiTreeViewColumn *tree_column)
{
  g_return_val_if_fail (BOBGUI_IS_TREE_VIEW_COLUMN (tree_column), 0);

  return tree_column->priv->width;
}

/**
 * bobgui_tree_view_column_get_x_offset:
 * @tree_column: A `BobguiTreeViewColumn`.
 *
 * Returns the current X offset of @tree_column in pixels.
 *
 * Returns: The current X offset of @tree_column.
 *
 * Deprecated: 4.10: Use BobguiColumnView instead
 */
int
bobgui_tree_view_column_get_x_offset (BobguiTreeViewColumn *tree_column)
{
  g_return_val_if_fail (BOBGUI_IS_TREE_VIEW_COLUMN (tree_column), 0);

  return tree_column->priv->x_offset;
}

int
_bobgui_tree_view_column_request_width (BobguiTreeViewColumn *tree_column)
{
  BobguiTreeViewColumnPrivate *priv;
  int real_requested_width;

  priv = tree_column->priv;

  if (priv->fixed_width != -1)
    {
      real_requested_width = priv->fixed_width;
    }
  else if (bobgui_tree_view_get_headers_visible (BOBGUI_TREE_VIEW (priv->tree_view)))
    {
      int button_request;
      int requested_width;

      bobgui_cell_area_context_get_preferred_width (priv->cell_area_context, &requested_width, NULL);
      requested_width += priv->padding;

      bobgui_widget_measure (priv->button, BOBGUI_ORIENTATION_HORIZONTAL, -1,
                          &button_request, NULL, NULL, NULL);
      real_requested_width = MAX (requested_width, button_request);
    }
  else
    {
      int requested_width;

      bobgui_cell_area_context_get_preferred_width (priv->cell_area_context, &requested_width, NULL);
      requested_width += priv->padding;

      real_requested_width = requested_width;
      if (real_requested_width < 0)
        real_requested_width = 0;
    }

  if (priv->min_width != -1)
    real_requested_width = MAX (real_requested_width, priv->min_width);

  if (priv->max_width != -1)
    real_requested_width = MIN (real_requested_width, priv->max_width);

  return real_requested_width;
}

void
_bobgui_tree_view_column_allocate (BobguiTreeViewColumn *tree_column,
				int                x_offset,
				int                width,
                                int                height)
{
  BobguiTreeViewColumnPrivate *priv;
  BobguiAllocation             allocation = { 0, 0, 0, 0 };

  g_return_if_fail (BOBGUI_IS_TREE_VIEW_COLUMN (tree_column));

  priv = tree_column->priv;

  if (priv->width != width)
    bobgui_widget_queue_draw (priv->tree_view);

  priv->x_offset = x_offset;
  priv->width = width;

  bobgui_cell_area_context_allocate (priv->cell_area_context, priv->width - priv->padding, -1);

  if (bobgui_tree_view_get_headers_visible (BOBGUI_TREE_VIEW (priv->tree_view)))
    {
      /* TODO: Underallocates the button horizontally, but
       * https://bugzilla.gnome.org/show_bug.cgi?id=770388
       */
      allocation.x      = x_offset;
      allocation.y      = 0;
      allocation.width  = width;
      allocation.height = height;

      bobgui_widget_size_allocate (priv->button, &allocation, -1);
    }

  g_object_notify_by_pspec (G_OBJECT (tree_column), tree_column_props[PROP_X_OFFSET]);
  g_object_notify_by_pspec (G_OBJECT (tree_column), tree_column_props[PROP_WIDTH]);
}

/**
 * bobgui_tree_view_column_set_fixed_width:
 * @tree_column: A `BobguiTreeViewColumn`.
 * @fixed_width: The new fixed width, in pixels, or -1.
 *
 * If @fixed_width is not -1, sets the fixed width of @tree_column; otherwise
 * unsets it.  The effective value of @fixed_width is clamped between the
 * minimum and maximum width of the column; however, the value stored in the
 * “fixed-width” property is not clamped.  If the column sizing is
 * %BOBGUI_TREE_VIEW_COLUMN_GROW_ONLY or %BOBGUI_TREE_VIEW_COLUMN_AUTOSIZE, setting
 * a fixed width overrides the automatically calculated width.  Note that
 * @fixed_width is only a hint to BOBGUI; the width actually allocated to the
 * column may be greater or less than requested.
 *
 * Along with “expand”, the “fixed-width” property changes when the column is
 * resized by the user.
 *
 * Deprecated: 4.10: Use BobguiColumnView instead
 **/
void
bobgui_tree_view_column_set_fixed_width (BobguiTreeViewColumn *tree_column,
				      int                fixed_width)
{
  BobguiTreeViewColumnPrivate *priv;

  g_return_if_fail (BOBGUI_IS_TREE_VIEW_COLUMN (tree_column));
  g_return_if_fail (fixed_width >= -1);

  priv = tree_column->priv;

  if (priv->fixed_width != fixed_width)
    {
      priv->fixed_width = fixed_width;
      if (priv->visible &&
          priv->tree_view != NULL &&
          bobgui_widget_get_realized (priv->tree_view))
        bobgui_widget_queue_resize (priv->tree_view);

      g_object_notify_by_pspec (G_OBJECT (tree_column), tree_column_props[PROP_FIXED_WIDTH]);
    }
}

/**
 * bobgui_tree_view_column_get_fixed_width:
 * @tree_column: A `BobguiTreeViewColumn`.
 *
 * Gets the fixed width of the column.  This may not be the actual displayed
 * width of the column; for that, use bobgui_tree_view_column_get_width().
 *
 * Returns: The fixed width of the column.
 *
 * Deprecated: 4.10: Use BobguiColumnView instead
 **/
int
bobgui_tree_view_column_get_fixed_width (BobguiTreeViewColumn *tree_column)
{
  g_return_val_if_fail (BOBGUI_IS_TREE_VIEW_COLUMN (tree_column), 0);

  return tree_column->priv->fixed_width;
}

/**
 * bobgui_tree_view_column_set_min_width:
 * @tree_column: A `BobguiTreeViewColumn`.
 * @min_width: The minimum width of the column in pixels, or -1.
 *
 * Sets the minimum width of the @tree_column.  If @min_width is -1, then the
 * minimum width is unset.
 *
 * Deprecated: 4.10: Use BobguiColumnView instead
 **/
void
bobgui_tree_view_column_set_min_width (BobguiTreeViewColumn *tree_column,
				    int                min_width)
{
  BobguiTreeViewColumnPrivate *priv;

  g_return_if_fail (BOBGUI_IS_TREE_VIEW_COLUMN (tree_column));
  g_return_if_fail (min_width >= -1);

  priv = tree_column->priv;

  if (min_width == priv->min_width)
    return;

  if (priv->visible &&
      priv->tree_view != NULL &&
      bobgui_widget_get_realized (priv->tree_view))
    {
      if (min_width > priv->width)
	bobgui_widget_queue_resize (priv->tree_view);
    }

  priv->min_width = min_width;
  g_object_freeze_notify (G_OBJECT (tree_column));
  if (priv->max_width != -1 && priv->max_width < min_width)
    {
      priv->max_width = min_width;
      g_object_notify_by_pspec (G_OBJECT (tree_column), tree_column_props[PROP_MAX_WIDTH]);
    }
  g_object_notify_by_pspec (G_OBJECT (tree_column), tree_column_props[PROP_MIN_WIDTH]);
  g_object_thaw_notify (G_OBJECT (tree_column));

  if (priv->column_type == BOBGUI_TREE_VIEW_COLUMN_AUTOSIZE && priv->tree_view)
    _bobgui_tree_view_column_autosize (BOBGUI_TREE_VIEW (priv->tree_view),
				    tree_column);
}

/**
 * bobgui_tree_view_column_get_min_width:
 * @tree_column: A `BobguiTreeViewColumn`.
 *
 * Returns the minimum width in pixels of the @tree_column, or -1 if no minimum
 * width is set.
 *
 * Returns: The minimum width of the @tree_column.
 *
 * Deprecated: 4.10: Use BobguiColumnView instead
 **/
int
bobgui_tree_view_column_get_min_width (BobguiTreeViewColumn *tree_column)
{
  g_return_val_if_fail (BOBGUI_IS_TREE_VIEW_COLUMN (tree_column), -1);

  return tree_column->priv->min_width;
}

/**
 * bobgui_tree_view_column_set_max_width:
 * @tree_column: A `BobguiTreeViewColumn`.
 * @max_width: The maximum width of the column in pixels, or -1.
 *
 * Sets the maximum width of the @tree_column.  If @max_width is -1, then the
 * maximum width is unset.  Note, the column can actually be wider than max
 * width if it’s the last column in a view.  In this case, the column expands to
 * fill any extra space.
 *
 * Deprecated: 4.10: Use BobguiColumnView instead
 **/
void
bobgui_tree_view_column_set_max_width (BobguiTreeViewColumn *tree_column,
				    int                max_width)
{
  BobguiTreeViewColumnPrivate *priv;

  g_return_if_fail (BOBGUI_IS_TREE_VIEW_COLUMN (tree_column));
  g_return_if_fail (max_width >= -1);

  priv = tree_column->priv;

  if (max_width == priv->max_width)
    return;

  if (priv->visible &&
      priv->tree_view != NULL &&
      bobgui_widget_get_realized (priv->tree_view))
    {
      if (max_width != -1 && max_width < priv->width)
	bobgui_widget_queue_resize (priv->tree_view);
    }

  priv->max_width = max_width;
  g_object_freeze_notify (G_OBJECT (tree_column));
  if (max_width != -1 && max_width < priv->min_width)
    {
      priv->min_width = max_width;
      g_object_notify_by_pspec (G_OBJECT (tree_column), tree_column_props[PROP_MIN_WIDTH]);
    }
  g_object_notify_by_pspec (G_OBJECT (tree_column), tree_column_props[PROP_MAX_WIDTH]);
  g_object_thaw_notify (G_OBJECT (tree_column));

  if (priv->column_type == BOBGUI_TREE_VIEW_COLUMN_AUTOSIZE && priv->tree_view)
    _bobgui_tree_view_column_autosize (BOBGUI_TREE_VIEW (priv->tree_view),
				    tree_column);
}

/**
 * bobgui_tree_view_column_get_max_width:
 * @tree_column: A `BobguiTreeViewColumn`.
 *
 * Returns the maximum width in pixels of the @tree_column, or -1 if no maximum
 * width is set.
 *
 * Returns: The maximum width of the @tree_column.
 *
 * Deprecated: 4.10: Use BobguiColumnView instead
 **/
int
bobgui_tree_view_column_get_max_width (BobguiTreeViewColumn *tree_column)
{
  g_return_val_if_fail (BOBGUI_IS_TREE_VIEW_COLUMN (tree_column), -1);

  return tree_column->priv->max_width;
}

/**
 * bobgui_tree_view_column_clicked:
 * @tree_column: a `BobguiTreeViewColumn`
 *
 * Emits the “clicked” signal on the column.  This function will only work if
 * @tree_column is clickable.
 *
 * Deprecated: 4.10: Use BobguiColumnView instead
 **/
void
bobgui_tree_view_column_clicked (BobguiTreeViewColumn *tree_column)
{
  BobguiTreeViewColumnPrivate *priv;

  g_return_if_fail (BOBGUI_IS_TREE_VIEW_COLUMN (tree_column));

  priv = tree_column->priv;

  if (priv->visible && priv->clickable)
    g_signal_emit_by_name (priv->button, "clicked");
}

/**
 * bobgui_tree_view_column_set_title:
 * @tree_column: A `BobguiTreeViewColumn`.
 * @title: The title of the @tree_column.
 *
 * Sets the title of the @tree_column.  If a custom widget has been set, then
 * this value is ignored.
 *
 * Deprecated: 4.10: Use BobguiColumnView instead
 **/
void
bobgui_tree_view_column_set_title (BobguiTreeViewColumn *tree_column,
				const char        *title)
{
  BobguiTreeViewColumnPrivate *priv;
  char                     *new_title;

  g_return_if_fail (BOBGUI_IS_TREE_VIEW_COLUMN (tree_column));

  priv = tree_column->priv;

  new_title = g_strdup (title);
  g_free (priv->title);
  priv->title = new_title;

  bobgui_tree_view_column_update_button (tree_column);
  g_object_notify_by_pspec (G_OBJECT (tree_column), tree_column_props[PROP_TITLE]);
}

/**
 * bobgui_tree_view_column_get_title:
 * @tree_column: A `BobguiTreeViewColumn`.
 *
 * Returns the title of the widget.
 *
 * Returns: the title of the column. This string should not be
 * modified or freed.
 *
 * Deprecated: 4.10: Use BobguiColumnView instead
 **/
const char *
bobgui_tree_view_column_get_title (BobguiTreeViewColumn *tree_column)
{
  g_return_val_if_fail (BOBGUI_IS_TREE_VIEW_COLUMN (tree_column), NULL);

  return tree_column->priv->title;
}

/**
 * bobgui_tree_view_column_set_expand:
 * @tree_column: A `BobguiTreeViewColumn`.
 * @expand: %TRUE if the column should expand to fill available space.
 *
 * Sets the column to take available extra space.  This space is shared equally
 * amongst all columns that have the expand set to %TRUE.  If no column has this
 * option set, then the last column gets all extra space.  By default, every
 * column is created with this %FALSE.
 *
 * Along with “fixed-width”, the “expand” property changes when the column is
 * resized by the user.
 *
 * Deprecated: 4.10: Use BobguiColumnView instead
 **/
void
bobgui_tree_view_column_set_expand (BobguiTreeViewColumn *tree_column,
				 gboolean           expand)
{
  BobguiTreeViewColumnPrivate *priv;

  g_return_if_fail (BOBGUI_IS_TREE_VIEW_COLUMN (tree_column));

  priv = tree_column->priv;

  expand = expand?TRUE:FALSE;
  if (priv->expand == expand)
    return;
  priv->expand = expand;

  if (priv->visible &&
      priv->tree_view != NULL &&
      bobgui_widget_get_realized (priv->tree_view))
    {
      bobgui_widget_queue_resize (priv->tree_view);
    }

  g_object_notify_by_pspec (G_OBJECT (tree_column), tree_column_props[PROP_EXPAND]);
}

/**
 * bobgui_tree_view_column_get_expand:
 * @tree_column: A `BobguiTreeViewColumn`.
 *
 * Returns %TRUE if the column expands to fill available space.
 *
 * Returns: %TRUE if the column expands to fill available space.
 *
 * Deprecated: 4.10: Use BobguiColumnView instead
 **/
gboolean
bobgui_tree_view_column_get_expand (BobguiTreeViewColumn *tree_column)
{
  g_return_val_if_fail (BOBGUI_IS_TREE_VIEW_COLUMN (tree_column), FALSE);

  return tree_column->priv->expand;
}

/**
 * bobgui_tree_view_column_set_clickable:
 * @tree_column: A `BobguiTreeViewColumn`.
 * @clickable: %TRUE if the header is active.
 *
 * Sets the header to be active if @clickable is %TRUE.  When the header is
 * active, then it can take keyboard focus, and can be clicked.
 *
 * Deprecated: 4.10: Use BobguiColumnView instead
 **/
void
bobgui_tree_view_column_set_clickable (BobguiTreeViewColumn *tree_column,
                                    gboolean           clickable)
{
  BobguiTreeViewColumnPrivate *priv;

  g_return_if_fail (BOBGUI_IS_TREE_VIEW_COLUMN (tree_column));

  priv = tree_column->priv;

  clickable = !! clickable;
  if (priv->clickable == clickable)
    return;

  priv->clickable = clickable;
  bobgui_tree_view_column_update_button (tree_column);
  g_object_notify_by_pspec (G_OBJECT (tree_column), tree_column_props[PROP_CLICKABLE]);
}

/**
 * bobgui_tree_view_column_get_clickable:
 * @tree_column: a `BobguiTreeViewColumn`
 *
 * Returns %TRUE if the user can click on the header for the column.
 *
 * Returns: %TRUE if user can click the column header.
 *
 * Deprecated: 4.10: Use BobguiColumnView instead
 **/
gboolean
bobgui_tree_view_column_get_clickable (BobguiTreeViewColumn *tree_column)
{
  g_return_val_if_fail (BOBGUI_IS_TREE_VIEW_COLUMN (tree_column), FALSE);

  return tree_column->priv->clickable;
}

/**
 * bobgui_tree_view_column_set_widget:
 * @tree_column: A `BobguiTreeViewColumn`.
 * @widget: (nullable): A child `BobguiWidget`
 *
 * Sets the widget in the header to be @widget.  If widget is %NULL, then the
 * header button is set with a `BobguiLabel` set to the title of @tree_column.
 *
 * Deprecated: 4.10: Use BobguiColumnView instead
 **/
void
bobgui_tree_view_column_set_widget (BobguiTreeViewColumn *tree_column,
				 BobguiWidget         *widget)
{
  BobguiTreeViewColumnPrivate *priv;

  g_return_if_fail (BOBGUI_IS_TREE_VIEW_COLUMN (tree_column));
  g_return_if_fail (widget == NULL || BOBGUI_IS_WIDGET (widget));

  priv = tree_column->priv;

  if (widget)
    g_object_ref_sink (widget);

  if (priv->child)
    g_object_unref (priv->child);

  priv->child = widget;
  bobgui_tree_view_column_update_button (tree_column);
  g_object_notify_by_pspec (G_OBJECT (tree_column), tree_column_props[PROP_WIDGET]);
}

/**
 * bobgui_tree_view_column_get_widget:
 * @tree_column: A `BobguiTreeViewColumn`
 *
 * Returns the `BobguiWidget` in the button on the column header.
 *
 * If a custom widget has not been set then %NULL is returned.
 *
 * Returns: (nullable) (transfer none): The `BobguiWidget` in the column header
 *
 * Deprecated: 4.10: Use BobguiColumnView instead
 */
BobguiWidget *
bobgui_tree_view_column_get_widget (BobguiTreeViewColumn *tree_column)
{
  g_return_val_if_fail (BOBGUI_IS_TREE_VIEW_COLUMN (tree_column), NULL);

  return tree_column->priv->child;
}

/**
 * bobgui_tree_view_column_set_alignment:
 * @tree_column: A `BobguiTreeViewColumn`.
 * @xalign: The alignment, which is between [0.0 and 1.0] inclusive.
 *
 * Sets the alignment of the title or custom widget inside the column header.
 * The alignment determines its location inside the button -- 0.0 for left, 0.5
 * for center, 1.0 for right.
 *
 * Deprecated: 4.10: Use BobguiColumnView instead
 **/
void
bobgui_tree_view_column_set_alignment (BobguiTreeViewColumn *tree_column,
                                    float              xalign)
{
  BobguiTreeViewColumnPrivate *priv;

  g_return_if_fail (BOBGUI_IS_TREE_VIEW_COLUMN (tree_column));

  priv = tree_column->priv;

  xalign = CLAMP (xalign, 0.0, 1.0);

  if (priv->xalign == xalign)
    return;

  priv->xalign = xalign;
  bobgui_tree_view_column_update_button (tree_column);
  g_object_notify_by_pspec (G_OBJECT (tree_column), tree_column_props[PROP_ALIGNMENT]);
}

/**
 * bobgui_tree_view_column_get_alignment:
 * @tree_column: A `BobguiTreeViewColumn`.
 *
 * Returns the current x alignment of @tree_column.  This value can range
 * between 0.0 and 1.0.
 *
 * Returns: The current alignent of @tree_column.
 *
 * Deprecated: 4.10: Use BobguiColumnView instead
 **/
float
bobgui_tree_view_column_get_alignment (BobguiTreeViewColumn *tree_column)
{
  g_return_val_if_fail (BOBGUI_IS_TREE_VIEW_COLUMN (tree_column), 0.5);

  return tree_column->priv->xalign;
}

/**
 * bobgui_tree_view_column_set_reorderable:
 * @tree_column: A `BobguiTreeViewColumn`
 * @reorderable: %TRUE, if the column can be reordered.
 *
 * If @reorderable is %TRUE, then the column can be reordered by the end user
 * dragging the header.
 *
 * Deprecated: 4.10: Use BobguiColumnView instead
 **/
void
bobgui_tree_view_column_set_reorderable (BobguiTreeViewColumn *tree_column,
				      gboolean           reorderable)
{
  BobguiTreeViewColumnPrivate *priv;

  g_return_if_fail (BOBGUI_IS_TREE_VIEW_COLUMN (tree_column));

  priv = tree_column->priv;

  /*  if (reorderable)
      bobgui_tree_view_column_set_clickable (tree_column, TRUE);*/

  if (priv->reorderable == (reorderable?TRUE:FALSE))
    return;

  priv->reorderable = (reorderable?TRUE:FALSE);
  bobgui_tree_view_column_update_button (tree_column);
  g_object_notify_by_pspec (G_OBJECT (tree_column), tree_column_props[PROP_REORDERABLE]);
}

/**
 * bobgui_tree_view_column_get_reorderable:
 * @tree_column: A `BobguiTreeViewColumn`
 *
 * Returns %TRUE if the @tree_column can be reordered by the user.
 *
 * Returns: %TRUE if the @tree_column can be reordered by the user.
 *
 * Deprecated: 4.10: Use BobguiColumnView instead
 **/
gboolean
bobgui_tree_view_column_get_reorderable (BobguiTreeViewColumn *tree_column)
{
  g_return_val_if_fail (BOBGUI_IS_TREE_VIEW_COLUMN (tree_column), FALSE);

  return tree_column->priv->reorderable;
}


/**
 * bobgui_tree_view_column_set_sort_column_id:
 * @tree_column: a `BobguiTreeViewColumn`
 * @sort_column_id: The @sort_column_id of the model to sort on.
 *
 * Sets the logical @sort_column_id that this column sorts on when this column
 * is selected for sorting.  Doing so makes the column header clickable.
 *
 * Deprecated: 4.10: Use BobguiColumnView instead
 **/
void
bobgui_tree_view_column_set_sort_column_id (BobguiTreeViewColumn *tree_column,
					 int                sort_column_id)
{
  BobguiTreeViewColumnPrivate *priv;

  g_return_if_fail (BOBGUI_IS_TREE_VIEW_COLUMN (tree_column));
  g_return_if_fail (sort_column_id >= -1);

  priv = tree_column->priv;

  if (priv->sort_column_id == sort_column_id)
    return;

  priv->sort_column_id = sort_column_id;

  /* Handle unsetting the id */
  if (sort_column_id == -1)
    {
      BobguiTreeModel *model = bobgui_tree_view_get_model (BOBGUI_TREE_VIEW (priv->tree_view));

      if (priv->sort_clicked_signal)
	{
	  g_signal_handler_disconnect (tree_column, priv->sort_clicked_signal);
	  priv->sort_clicked_signal = 0;
	}

      if (priv->sort_column_changed_signal)
	{
	  g_signal_handler_disconnect (model, priv->sort_column_changed_signal);
	  priv->sort_column_changed_signal = 0;
	}

      bobgui_tree_view_column_set_sort_order (tree_column, BOBGUI_SORT_ASCENDING);
      bobgui_tree_view_column_set_sort_indicator (tree_column, FALSE);
      bobgui_tree_view_column_set_clickable (tree_column, FALSE);
      g_object_notify_by_pspec (G_OBJECT (tree_column), tree_column_props[PROP_SORT_COLUMN_ID]);
      return;
    }

  bobgui_tree_view_column_set_clickable (tree_column, TRUE);

  if (! priv->sort_clicked_signal)
    priv->sort_clicked_signal = g_signal_connect (tree_column,
						  "clicked",
						  G_CALLBACK (bobgui_tree_view_column_sort),
						  NULL);

  bobgui_tree_view_column_setup_sort_column_id_callback (tree_column);
  g_object_notify_by_pspec (G_OBJECT (tree_column), tree_column_props[PROP_SORT_COLUMN_ID]);
}

/**
 * bobgui_tree_view_column_get_sort_column_id:
 * @tree_column: a `BobguiTreeViewColumn`
 *
 * Gets the logical @sort_column_id that the model sorts on
 * when this column is selected for sorting.
 *
 * See [method@Bobgui.TreeViewColumn.set_sort_column_id].
 *
 * Returns: the current @sort_column_id for this column, or -1 if
 *   this column can’t be used for sorting
 *
 * Deprecated: 4.10: Use BobguiColumnView instead
 */
int
bobgui_tree_view_column_get_sort_column_id (BobguiTreeViewColumn *tree_column)
{
  g_return_val_if_fail (BOBGUI_IS_TREE_VIEW_COLUMN (tree_column), 0);

  return tree_column->priv->sort_column_id;
}

/**
 * bobgui_tree_view_column_set_sort_indicator:
 * @tree_column: a `BobguiTreeViewColumn`
 * @setting: %TRUE to display an indicator that the column is sorted
 *
 * Call this function with a @setting of %TRUE to display an arrow in
 * the header button indicating the column is sorted. Call
 * bobgui_tree_view_column_set_sort_order() to change the direction of
 * the arrow.
 *
 * Deprecated: 4.10: Use BobguiColumnView instead
 **/
void
bobgui_tree_view_column_set_sort_indicator (BobguiTreeViewColumn     *tree_column,
                                         gboolean               setting)
{
  g_return_if_fail (BOBGUI_IS_TREE_VIEW_COLUMN (tree_column));

  setting = setting != FALSE;

  if (setting == tree_column->priv->show_sort_indicator)
    return;

  tree_column->priv->show_sort_indicator = setting;
  bobgui_tree_view_column_update_button (tree_column);
  g_object_notify_by_pspec (G_OBJECT (tree_column), tree_column_props[PROP_SORT_INDICATOR]);
}

/**
 * bobgui_tree_view_column_get_sort_indicator:
 * @tree_column: a `BobguiTreeViewColumn`
 *
 * Gets the value set by bobgui_tree_view_column_set_sort_indicator().
 *
 * Returns: whether the sort indicator arrow is displayed
 *
 * Deprecated: 4.10: Use BobguiColumnView instead
 **/
gboolean
bobgui_tree_view_column_get_sort_indicator  (BobguiTreeViewColumn     *tree_column)
{
  g_return_val_if_fail (BOBGUI_IS_TREE_VIEW_COLUMN (tree_column), FALSE);

  return tree_column->priv->show_sort_indicator;
}

/**
 * bobgui_tree_view_column_set_sort_order:
 * @tree_column: a `BobguiTreeViewColumn`
 * @order: sort order that the sort indicator should indicate
 *
 * Changes the appearance of the sort indicator.
 *
 * This does not actually sort the model.  Use
 * bobgui_tree_view_column_set_sort_column_id() if you want automatic sorting
 * support.  This function is primarily for custom sorting behavior, and should
 * be used in conjunction with bobgui_tree_sortable_set_sort_column_id() to do
 * that. For custom models, the mechanism will vary.
 *
 * The sort indicator changes direction to indicate normal sort or reverse sort.
 * Note that you must have the sort indicator enabled to see anything when
 * calling this function; see bobgui_tree_view_column_set_sort_indicator().
 *
 * Deprecated: 4.10: Use BobguiColumnView instead
 **/
void
bobgui_tree_view_column_set_sort_order      (BobguiTreeViewColumn     *tree_column,
                                          BobguiSortType            order)
{
  g_return_if_fail (BOBGUI_IS_TREE_VIEW_COLUMN (tree_column));

  if (order == tree_column->priv->sort_order)
    return;

  tree_column->priv->sort_order = order;
  bobgui_tree_view_column_update_button (tree_column);
  g_object_notify_by_pspec (G_OBJECT (tree_column), tree_column_props[PROP_SORT_ORDER]);
}

/**
 * bobgui_tree_view_column_get_sort_order:
 * @tree_column: a `BobguiTreeViewColumn`
 *
 * Gets the value set by bobgui_tree_view_column_set_sort_order().
 *
 * Returns: the sort order the sort indicator is indicating
 *
 * Deprecated: 4.10: Use BobguiColumnView instead
 **/
BobguiSortType
bobgui_tree_view_column_get_sort_order      (BobguiTreeViewColumn     *tree_column)
{
  g_return_val_if_fail (BOBGUI_IS_TREE_VIEW_COLUMN (tree_column), 0);

  return tree_column->priv->sort_order;
}

/**
 * bobgui_tree_view_column_cell_set_cell_data:
 * @tree_column: A `BobguiTreeViewColumn`.
 * @tree_model: The `BobguiTreeModel` to get the cell renderers attributes from.
 * @iter: The `BobguiTreeIter` to get the cell renderer’s attributes from.
 * @is_expander: %TRUE, if the row has children
 * @is_expanded: %TRUE, if the row has visible children
 *
 * Sets the cell renderer based on the @tree_model and @iter.  That is, for
 * every attribute mapping in @tree_column, it will get a value from the set
 * column on the @iter, and use that value to set the attribute on the cell
 * renderer.  This is used primarily by the `BobguiTreeView`.
 *
 * Deprecated: 4.10: Use BobguiColumnView instead
 **/
void
bobgui_tree_view_column_cell_set_cell_data (BobguiTreeViewColumn *tree_column,
					 BobguiTreeModel      *tree_model,
					 BobguiTreeIter       *iter,
					 gboolean           is_expander,
					 gboolean           is_expanded)
{
  g_return_if_fail (BOBGUI_IS_TREE_VIEW_COLUMN (tree_column));

  if (tree_model == NULL)
    return;

  bobgui_cell_area_apply_attributes (tree_column->priv->cell_area, tree_model, iter,
                                  is_expander, is_expanded);
}

/**
 * bobgui_tree_view_column_cell_get_size:
 * @tree_column: A `BobguiTreeViewColumn`.
 * @x_offset: (out) (optional): location to return x offset of a cell relative to @cell_area
 * @y_offset: (out) (optional): location to return y offset of a cell relative to @cell_area
 * @width: (out) (optional): location to return width needed to render a cell
 * @height: (out) (optional): location to return height needed to render a cell
 *
 * Obtains the width and height needed to render the column.  This is used
 * primarily by the `BobguiTreeView`.
 *
 * Deprecated: 4.10: Use BobguiColumnView instead
 **/
void
bobgui_tree_view_column_cell_get_size (BobguiTreeViewColumn  *tree_column,
                                    int                *x_offset,
                                    int                *y_offset,
                                    int                *width,
                                    int                *height)
{
  BobguiTreeViewColumnPrivate *priv;
  int min_width = 0, min_height = 0;

  g_return_if_fail (BOBGUI_IS_TREE_VIEW_COLUMN (tree_column));

  priv = tree_column->priv;

  g_signal_handler_block (priv->cell_area_context,
			  priv->context_changed_signal);

  bobgui_cell_area_get_preferred_width (priv->cell_area,
                                     priv->cell_area_context,
                                     priv->tree_view,
                                     NULL, NULL);

  bobgui_cell_area_context_get_preferred_width (priv->cell_area_context, &min_width, NULL);

  bobgui_cell_area_get_preferred_height_for_width (priv->cell_area,
                                                priv->cell_area_context,
                                                priv->tree_view,
                                                min_width,
                                                &min_height,
                                                NULL);

  g_signal_handler_unblock (priv->cell_area_context,
			    priv->context_changed_signal);


  if (height)
    * height = min_height;
  if (width)
    * width = min_width;

}

/**
 * bobgui_tree_view_column_cell_snapshot:
 * @tree_column: A `BobguiTreeViewColumn`.
 * @snapshot: `BobguiSnapshot` to draw to
 * @background_area: entire cell area (including tree expanders and maybe padding on the sides)
 * @cell_area: area normally rendered by a cell renderer
 * @flags: flags that affect rendering
 *
 * Renders the cell contained by #tree_column. This is used primarily by the
 * `BobguiTreeView`.
 *
 * Deprecated: 4.10: Use BobguiColumnView instead
 **/
void
bobgui_tree_view_column_cell_snapshot (BobguiTreeViewColumn  *tree_column,
				    BobguiSnapshot        *snapshot,
				    const GdkRectangle *background_area,
				    const GdkRectangle *cell_area,
				    guint               flags,
                                    gboolean            draw_focus)
{
  BobguiTreeViewColumnPrivate *priv;

  g_return_if_fail (BOBGUI_IS_TREE_VIEW_COLUMN (tree_column));
  g_return_if_fail (snapshot != NULL);
  g_return_if_fail (background_area != NULL);
  g_return_if_fail (cell_area != NULL);

  priv = tree_column->priv;

  bobgui_cell_area_snapshot (priv->cell_area, priv->cell_area_context,
                          priv->tree_view, snapshot,
                          background_area, cell_area, flags,
                          draw_focus);
}

gboolean
_bobgui_tree_view_column_cell_event (BobguiTreeViewColumn  *tree_column,
				  GdkEvent           *event,
				  const GdkRectangle *cell_area,
				  guint               flags)
{
  BobguiTreeViewColumnPrivate *priv;

  g_return_val_if_fail (BOBGUI_IS_TREE_VIEW_COLUMN (tree_column), FALSE);

  priv = tree_column->priv;

  return bobgui_cell_area_event (priv->cell_area,
                              priv->cell_area_context,
                              priv->tree_view,
                              event,
                              cell_area,
                              flags);
}

/**
 * bobgui_tree_view_column_cell_is_visible:
 * @tree_column: A `BobguiTreeViewColumn`
 *
 * Returns %TRUE if any of the cells packed into the @tree_column are visible.
 * For this to be meaningful, you must first initialize the cells with
 * bobgui_tree_view_column_cell_set_cell_data()
 *
 * Returns: %TRUE, if any of the cells packed into the @tree_column are currently visible
 *
 * Deprecated: 4.10: Use BobguiColumnView instead
 **/
gboolean
bobgui_tree_view_column_cell_is_visible (BobguiTreeViewColumn *tree_column)
{
  GList *list;
  GList *cells;
  BobguiTreeViewColumnPrivate *priv;

  g_return_val_if_fail (BOBGUI_IS_TREE_VIEW_COLUMN (tree_column), FALSE);

  priv = tree_column->priv;

  cells = bobgui_cell_layout_get_cells (BOBGUI_CELL_LAYOUT (priv->cell_area));
  for (list = cells; list; list = list->next)
    {
      if (bobgui_cell_renderer_get_visible (list->data))
        {
          g_list_free (cells);
          return TRUE;
        }
    }

  g_list_free (cells);

  return FALSE;
}

/**
 * bobgui_tree_view_column_focus_cell:
 * @tree_column: A `BobguiTreeViewColumn`
 * @cell: A `BobguiCellRenderer`
 *
 * Sets the current keyboard focus to be at @cell, if the column contains
 * 2 or more editable and activatable cells.
 *
 * Deprecated: 4.10: Use BobguiColumnView instead
 **/
void
bobgui_tree_view_column_focus_cell (BobguiTreeViewColumn *tree_column,
				 BobguiCellRenderer   *cell)
{
  g_return_if_fail (BOBGUI_IS_TREE_VIEW_COLUMN (tree_column));
  g_return_if_fail (BOBGUI_IS_CELL_RENDERER (cell));

  bobgui_cell_area_set_focus_cell (tree_column->priv->cell_area, cell);
}

void
_bobgui_tree_view_column_cell_set_dirty (BobguiTreeViewColumn *tree_column,
				      gboolean           install_handler)
{
  BobguiTreeViewColumnPrivate *priv = tree_column->priv;

  priv->dirty = TRUE;
  priv->padding = 0;
  priv->width = 0;

  /* Issue a manual reset on the context to have all
   * sizes re-requested for the context.
   */
  g_signal_handler_block (priv->cell_area_context,
			  priv->context_changed_signal);
  bobgui_cell_area_context_reset (priv->cell_area_context);
  g_signal_handler_unblock (priv->cell_area_context,
			    priv->context_changed_signal);

  if (priv->tree_view &&
      bobgui_widget_get_realized (priv->tree_view))
    {
      _bobgui_tree_view_install_mark_rows_col_dirty (BOBGUI_TREE_VIEW (priv->tree_view), install_handler);
      bobgui_widget_queue_resize (priv->tree_view);
    }
}

gboolean
_bobgui_tree_view_column_cell_get_dirty (BobguiTreeViewColumn  *tree_column)
{
  return tree_column->priv->dirty;
}

/**
 * bobgui_tree_view_column_cell_get_position:
 * @tree_column: a `BobguiTreeViewColumn`
 * @cell_renderer: a `BobguiCellRenderer`
 * @x_offset: (out) (optional): return location for the horizontal
 *   position of @cell within @tree_column
 * @width: (out) (optional): return location for the width of @cell
 *
 * Obtains the horizontal position and size of a cell in a column.
 *
 * If the  cell is not found in the column, @start_pos and @width
 * are not changed and %FALSE is returned.
 *
 * Returns: %TRUE if @cell belongs to @tree_column
 *
 * Deprecated: 4.10: Use BobguiColumnView instead
 */
gboolean
bobgui_tree_view_column_cell_get_position (BobguiTreeViewColumn *tree_column,
					BobguiCellRenderer   *cell_renderer,
					int               *x_offset,
					int               *width)
{
  BobguiTreeViewColumnPrivate *priv;
  GdkRectangle cell_area;
  GdkRectangle allocation;

  g_return_val_if_fail (BOBGUI_IS_TREE_VIEW_COLUMN (tree_column), FALSE);
  g_return_val_if_fail (BOBGUI_IS_CELL_RENDERER (cell_renderer), FALSE);

  priv = tree_column->priv;

  if (! bobgui_cell_area_has_renderer (priv->cell_area, cell_renderer))
    return FALSE;

  bobgui_tree_view_get_background_area (BOBGUI_TREE_VIEW (priv->tree_view),
                                     NULL, tree_column, &cell_area);

  bobgui_cell_area_get_cell_allocation (priv->cell_area,
                                     priv->cell_area_context,
                                     priv->tree_view,
                                     cell_renderer,
                                     &cell_area,
                                     &allocation);

  if (x_offset)
    *x_offset = allocation.x - cell_area.x;

  if (width)
    *width = allocation.width;

  return TRUE;
}

/**
 * bobgui_tree_view_column_queue_resize:
 * @tree_column: A `BobguiTreeViewColumn`
 *
 * Flags the column, and the cell renderers added to this column, to have
 * their sizes renegotiated.
 *
 * Deprecated: 4.10: Use BobguiColumnView instead
 **/
void
bobgui_tree_view_column_queue_resize (BobguiTreeViewColumn *tree_column)
{
  g_return_if_fail (BOBGUI_IS_TREE_VIEW_COLUMN (tree_column));

  if (tree_column->priv->tree_view)
    _bobgui_tree_view_column_cell_set_dirty (tree_column, TRUE);
}

/**
 * bobgui_tree_view_column_get_tree_view:
 * @tree_column: A `BobguiTreeViewColumn`
 *
 * Returns the `BobguiTreeView` wherein @tree_column has been inserted.
 * If @column is currently not inserted in any tree view, %NULL is
 * returned.
 *
 * Returns: (nullable) (transfer none): The tree view wherein @column
 *   has been inserted
 *
 * Deprecated: 4.10: Use BobguiColumnView instead
 */
BobguiWidget *
bobgui_tree_view_column_get_tree_view (BobguiTreeViewColumn *tree_column)
{
  g_return_val_if_fail (BOBGUI_IS_TREE_VIEW_COLUMN (tree_column), NULL);

  return tree_column->priv->tree_view;
}

/**
 * bobgui_tree_view_column_get_button:
 * @tree_column: A `BobguiTreeViewColumn`
 *
 * Returns the button used in the treeview column header
 *
 * Returns: (transfer none): The button for the column header.
 *
 * Deprecated: 4.10: Use BobguiColumnView instead
 */
BobguiWidget *
bobgui_tree_view_column_get_button (BobguiTreeViewColumn *tree_column)
{
  g_return_val_if_fail (BOBGUI_IS_TREE_VIEW_COLUMN (tree_column), NULL);

  return tree_column->priv->button;
}

void
_bobgui_tree_view_column_push_padding (BobguiTreeViewColumn  *column,
				    int                 padding)
{
  column->priv->padding = MAX (column->priv->padding, padding);
}

int
_bobgui_tree_view_column_get_requested_width (BobguiTreeViewColumn  *column)
{
  int requested_width;

  bobgui_cell_area_context_get_preferred_width (column->priv->cell_area_context, &requested_width, NULL);

  return requested_width + column->priv->padding;
}

int
_bobgui_tree_view_column_get_drag_x (BobguiTreeViewColumn  *column)
{
  return column->priv->drag_x;
}

BobguiCellAreaContext *
_bobgui_tree_view_column_get_context (BobguiTreeViewColumn  *column)
{
  return column->priv->cell_area_context;
}

gboolean
_bobgui_tree_view_column_coords_in_resize_rect (BobguiTreeViewColumn *column,
                                             double             x,
                                             double             y)
{
  BobguiTreeViewColumnPrivate *priv = column->priv;
  graphene_rect_t button_bounds;

  /* x and y are in treeview coordinates. */

  if (!bobgui_widget_get_realized (priv->button) ||
      !priv->resizable ||
      !priv->visible)
    return FALSE;

  if (!bobgui_widget_compute_bounds (priv->button, priv->tree_view, &button_bounds))
    return FALSE;

  if (bobgui_widget_get_direction (priv->tree_view) == BOBGUI_TEXT_DIR_LTR)
    button_bounds.origin.x += button_bounds.size.width - TREE_VIEW_DRAG_WIDTH;

  button_bounds.size.width = TREE_VIEW_DRAG_WIDTH;

  return graphene_rect_contains_point (&button_bounds,
                                       &(graphene_point_t){x, y});
}
