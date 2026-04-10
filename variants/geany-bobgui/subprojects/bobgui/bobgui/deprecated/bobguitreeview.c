/* bobguitreeview.c
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

#include "bobguitreeview.h"

#include "bobguiadjustmentprivate.h"
#include "bobguibox.h"
#include "bobguibuildable.h"
#include "bobguibutton.h"
#include "bobguicelllayout.h"
#include "bobguicellrenderer.h"
#include "bobguicssnumbervalueprivate.h"
#include "bobguicsscolorvalueprivate.h"
#include "bobguidragsourceprivate.h"
#include "bobguidragicon.h"
#include "bobguidroptargetasync.h"
#include "bobguientryprivate.h"
#include "bobguisearchentryprivate.h"
#include "bobguieventcontrollerkey.h"
#include "bobguieventcontrollerfocus.h"
#include "bobguieventcontrollermotion.h"
#include "bobguieventcontrollerscroll.h"
#include "bobguiframe.h"
#include "bobguigesturedrag.h"
#include "bobguigestureclick.h"
#include "bobguigesturesingle.h"
#include "bobguilabel.h"
#include "bobguimain.h"
#include "bobguimarshalers.h"
#include "bobguiprivate.h"
#include "bobguitext.h"
#include "bobguitreerbtreeprivate.h"
#include "bobguirendericonprivate.h"
#include "bobguiscrollable.h"
#include "bobguisettingsprivate.h"
#include "bobguishortcutcontroller.h"
#include "deprecated/bobguirender.h"
#include "bobguistylecontextprivate.h"
#include "bobguitooltip.h"
#include "bobguitreednd.h"
#include "bobguitreemodelsort.h"
#include "bobguitreeprivate.h"
#include "bobguitypebuiltins.h"
#include "bobguiwidgetprivate.h"
#include "bobguiwindowgroup.h"
#include "bobguinative.h"
#include "bobguipopover.h"

#include "gdk/gdkeventsprivate.h"
#include "gdk/gdktextureprivate.h"

#include <math.h>
#include <string.h>

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

/**
 * BobguiTreeView:
 *
 * A widget for displaying both trees and lists
 *
 * <picture>
 *   <source srcset="list-and-tree-dark.png" media="(prefers-color-scheme: dark)">
 *   <img alt="An example BobguiTreeView" src="list-and-tree.png">
 * </picture>
 *
 * Widget that displays any object that implements the [iface@Bobgui.TreeModel] interface.
 *
 * Please refer to the [tree widget conceptual overview](section-tree-widget.html)
 * for an overview of all the objects and data types related to the tree
 * widget and how they work together.
 *
 * ## Coordinate systems in BobguiTreeView API
 *
 * Several different coordinate systems are exposed in the `BobguiTreeView` API.
 * These are:
 *
 * ![](tree-view-coordinates.png)
 *
 * - Widget coordinates: Coordinates relative to the widget (usually `widget->window`).
 *
 * - Bin window coordinates: Coordinates relative to the window that BobguiTreeView renders to.
 *
 * - Tree coordinates: Coordinates relative to the entire scrollable area of BobguiTreeView. These
 *   coordinates start at (0, 0) for row 0 of the tree.
 *
 * Several functions are available for converting between the different
 * coordinate systems.  The most common translations are between widget and bin
 * window coordinates and between bin window and tree coordinates. For the
 * former you can use [method@Bobgui.TreeView.convert_widget_to_bin_window_coords]
 * (and vice versa), for the latter [method@Bobgui.TreeView.convert_bin_window_to_tree_coords]
 * (and vice versa).
 *
 * ## `BobguiTreeView` as `BobguiBuildable`
 *
 * The `BobguiTreeView` implementation of the `BobguiBuildable` interface accepts
 * [class@Bobgui.TreeViewColumn] objects as `<child>` elements and exposes the
 * internal [class@Bobgui.TreeSelection] in UI definitions.
 *
 * An example of a UI definition fragment with `BobguiTreeView`:
 *
 * ```xml
 * <object class="BobguiTreeView" id="treeview">
 *   <property name="model">liststore1</property>
 *   <child>
 *     <object class="BobguiTreeViewColumn" id="test-column">
 *       <property name="title">Test</property>
 *       <child>
 *         <object class="BobguiCellRendererText" id="test-renderer"/>
 *         <attributes>
 *           <attribute name="text">1</attribute>
 *         </attributes>
 *       </child>
 *     </object>
 *   </child>
 *   <child internal-child="selection">
 *     <object class="BobguiTreeSelection" id="selection">
 *       <signal name="changed" handler="on_treeview_selection_changed"/>
 *     </object>
 *   </child>
 * </object>
 * ```
 *
 * ## CSS nodes
 *
 * ```
 * treeview.view
 * ├── header
 * │   ├── button
 * │   │   ╰── [sort-indicator]
 * ┊   ┊
 * │   ╰── button
 * │       ╰── [sort-indicator]
 * │
 * ├── [rubberband]
 * ╰── [dndtarget]
 * ```
 *
 * `BobguiTreeView` has a main CSS node with name `treeview` and style class `.view`.
 * It has a subnode with name `header`, which is the parent for all the column
 * header widgets' CSS nodes.
 *
 * Each column header consists of a `button`, which among other content, has a
 * child with name `sort-indicator`, which carries the `.ascending` or `.descending`
 * style classes when the column header should show a sort indicator. The CSS
 * is expected to provide a suitable image using the `-bobgui-icon-source` property.
 *
 * For rubberband selection, a subnode with name `rubberband` is used.
 *
 * For the drop target location during DND, a subnode with name `dndtarget` is used.
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] for lists, and [class@Bobgui.ColumnView]
 *   for tabular lists
 */

enum
{
  DRAG_COLUMN_WINDOW_STATE_UNSET = 0,
  DRAG_COLUMN_WINDOW_STATE_ORIGINAL = 1,
  DRAG_COLUMN_WINDOW_STATE_ARROW = 2,
  DRAG_COLUMN_WINDOW_STATE_ARROW_LEFT = 3,
  DRAG_COLUMN_WINDOW_STATE_ARROW_RIGHT = 4
};

enum
{
  RUBBER_BAND_OFF = 0,
  RUBBER_BAND_MAYBE_START = 1,
  RUBBER_BAND_ACTIVE = 2
};

typedef enum {
  CLEAR_AND_SELECT = (1 << 0),
  CLAMP_NODE       = (1 << 1),
  CURSOR_INVALID   = (1 << 2)
} SetCursorFlags;

 /* This lovely little value is used to determine how far away from the title bar
  * you can move the mouse and still have a column drag work.
  */
#define TREE_VIEW_COLUMN_DRAG_DEAD_MULTIPLIER(tree_view) (10*bobgui_tree_view_get_effective_header_height(tree_view))

#ifdef __GNUC__

#define TREE_VIEW_INTERNAL_ASSERT(expr, ret)     G_STMT_START{          \
     if (!(expr))                                                       \
       {                                                                \
         g_log (G_LOG_DOMAIN,                                           \
                G_LOG_LEVEL_CRITICAL,                                   \
		"%s (%s): assertion `%s' failed.\n"                     \
	        "There is a disparity between the internal view of the BobguiTreeView,\n"    \
		"and the BobguiTreeModel.  This generally means that the model has changed\n"\
		"without letting the view know.  Any display from now on is likely to\n"  \
		"be incorrect.\n",                                                        \
                G_STRLOC,                                               \
                G_STRFUNC,                                              \
                #expr);                                                 \
         return ret;                                                    \
       };                               }G_STMT_END

#define TREE_VIEW_INTERNAL_ASSERT_VOID(expr)     G_STMT_START{          \
     if (!(expr))                                                       \
       {                                                                \
         g_log (G_LOG_DOMAIN,                                           \
                G_LOG_LEVEL_CRITICAL,                                   \
		"%s (%s): assertion `%s' failed.\n"                     \
	        "There is a disparity between the internal view of the BobguiTreeView,\n"    \
		"and the BobguiTreeModel.  This generally means that the model has changed\n"\
		"without letting the view know.  Any display from now on is likely to\n"  \
		"be incorrect.\n",                                                        \
                G_STRLOC,                                               \
                G_STRFUNC,                                              \
                #expr);                                                 \
         return;                                                        \
       };                               }G_STMT_END

#else

#define TREE_VIEW_INTERNAL_ASSERT(expr, ret)     G_STMT_START{          \
     if (!(expr))                                                       \
       {                                                                \
         g_log (G_LOG_DOMAIN,                                           \
                G_LOG_LEVEL_CRITICAL,                                   \
		"file %s: line %d: assertion `%s' failed.\n"       \
	        "There is a disparity between the internal view of the BobguiTreeView,\n"    \
		"and the BobguiTreeModel.  This generally means that the model has changed\n"\
		"without letting the view know.  Any display from now on is likely to\n"  \
		"be incorrect.\n",                                                        \
                __FILE__,                                               \
                __LINE__,                                               \
                #expr);                                                 \
         return ret;                                                    \
       };                               }G_STMT_END

#define TREE_VIEW_INTERNAL_ASSERT_VOID(expr)     G_STMT_START{          \
     if (!(expr))                                                       \
       {                                                                \
         g_log (G_LOG_DOMAIN,                                           \
                G_LOG_LEVEL_CRITICAL,                                   \
		"file %s: line %d: assertion '%s' failed.\n"            \
	        "There is a disparity between the internal view of the BobguiTreeView,\n"    \
		"and the BobguiTreeModel.  This generally means that the model has changed\n"\
		"without letting the view know.  Any display from now on is likely to\n"  \
		"be incorrect.\n",                                                        \
                __FILE__,                                               \
                __LINE__,                                               \
                #expr);                                                 \
         return;                                                        \
       };                               }G_STMT_END
#endif

#define BOBGUI_TREE_VIEW_PRIORITY_VALIDATE (GDK_PRIORITY_REDRAW + 5)
#define BOBGUI_TREE_VIEW_PRIORITY_SCROLL_SYNC (BOBGUI_TREE_VIEW_PRIORITY_VALIDATE + 2)
/* 3/5 of gdkframeclockidle.c's FRAME_INTERVAL (16667 microsecs) */
#define BOBGUI_TREE_VIEW_TIME_MS_PER_IDLE 10
#define SCROLL_EDGE_SIZE 15
#define BOBGUI_TREE_VIEW_SEARCH_DIALOG_TIMEOUT 5000
#define AUTO_EXPAND_TIMEOUT 500

/* Translate from bin_window coordinates to rbtree (tree coordinates) and
 * vice versa.
 */
#define TREE_WINDOW_Y_TO_RBTREE_Y(tree_view,y) ((y) + tree_view->dy)
#define RBTREE_Y_TO_TREE_WINDOW_Y(tree_view,y) ((y) - tree_view->dy)

/* Vertical separator width. Must be an even number. */
#define _TREE_VIEW_VERTICAL_SEPARATOR 2
/* Horizontal separator width. Must be an even number.  */
#define _TREE_VIEW_HORIZONTAL_SEPARATOR 4
/* Tree view grid line width, in pixels */
#define _TREE_VIEW_GRID_LINE_WIDTH 1
/* Tree view line width, in pixels */
#define _TREE_VIEW_TREE_LINE_WIDTH 1

typedef struct _BobguiTreeViewColumnReorder BobguiTreeViewColumnReorder;
struct _BobguiTreeViewColumnReorder
{
  int left_align;
  int right_align;
  BobguiTreeViewColumn *left_column;
  BobguiTreeViewColumn *right_column;
};

typedef struct _BobguiTreeViewChild BobguiTreeViewChild;
struct _BobguiTreeViewChild
{
  BobguiWidget *widget;
  BobguiTreeRBNode *node;
  BobguiTreeRBTree *tree;
  BobguiTreeViewColumn *column;
  BobguiBorder border;
};


typedef struct _TreeViewDragInfo TreeViewDragInfo;
struct _TreeViewDragInfo
{
  GdkContentFormats *source_formats;
  GdkDragAction source_actions;
  GdkDrag *drag;
  BobguiTreeRowReference *source_item;

  BobguiCssNode *cssnode;
  BobguiDropTargetAsync *dest;
  GdkModifierType start_button_mask;

  guint source_set : 1;
  guint dest_set : 1;
};


typedef struct
{
  BobguiTreeModel *model;

  /* tree information */
  BobguiTreeRBTree *tree;

  /* Container info */
  GList *children;
  int width;

  guint presize_handler_tick_cb;

  /* Adjustments */
  BobguiAdjustment *hadjustment;
  BobguiAdjustment *vadjustment;
  int            min_display_width;
  int            min_display_height;

  /* CSS nodes */
  BobguiCssNode *header_node;

  /* Scroll position state keeping */
  BobguiTreeRowReference *top_row;
  int top_row_dy;
  /* dy == y pos of top_row + top_row_dy */
  /* we cache it for simplicity of the code */
  int dy;

  guint validate_rows_timer;
  guint scroll_sync_timer;

  /* Indentation and expander layout */
  BobguiTreeViewColumn *expander_column;

  int level_indentation;

  /* Key navigation (focus), selection */
  int cursor_offset;

  BobguiTreeRowReference *anchor;
  BobguiTreeRBNode *cursor_node;
  BobguiTreeRBTree *cursor_tree;

  BobguiTreeViewColumn *focus_column;

  /* Current pressed node, previously pressed, prelight */
  BobguiTreeRBNode *button_pressed_node;
  BobguiTreeRBTree *button_pressed_tree;

  int press_start_x;
  int press_start_y;

  int event_last_x;
  int event_last_y;

  BobguiTreeRBNode *prelight_node;
  BobguiTreeRBTree *prelight_tree;

  /* Cell Editing */
  BobguiTreeViewColumn *edited_column;

  /* Auto expand/collapse timeout in hover mode */
  guint auto_expand_timeout;

  /* Selection information */
  BobguiTreeSelection *selection;

  /* Header information */
  int header_height;
  int n_columns;
  GList *columns;

  BobguiTreeViewColumnDropFunc column_drop_func;
  gpointer column_drop_func_data;
  GDestroyNotify column_drop_func_data_destroy;
  GList *column_drag_info;
  BobguiTreeViewColumnReorder *cur_reorder;

  int prev_width_before_expander;

  /* Scroll timeout (e.g. during dnd, rubber banding) */
  guint scroll_timeout;

  /* Interactive Header reordering */
  BobguiTreeViewColumn *drag_column;
  int drag_column_x;
  int drag_column_y;

  /* Interactive Header Resizing */
  int drag_pos;
  int x_drag;

  /* Row drag-and-drop */
  BobguiTreeRowReference *drag_dest_row;
  BobguiTreeViewDropPosition drag_dest_pos;
  guint open_dest_timeout;

  /* Rubber banding */
  int rubber_band_status;
  int rubber_band_x;
  int rubber_band_y;
  int rubber_band_extend;
  int rubber_band_modify;

  /* fixed height */
  int fixed_height;

  BobguiTreeRBNode *rubber_band_start_node;
  BobguiTreeRBTree *rubber_band_start_tree;

  BobguiTreeRBNode *rubber_band_end_node;
  BobguiTreeRBTree *rubber_band_end_tree;
  BobguiCssNode *rubber_band_cssnode;

  /* Scroll-to functionality when unrealized */
  BobguiTreeRowReference *scroll_to_path;
  BobguiTreeViewColumn *scroll_to_column;
  float scroll_to_row_align;
  float scroll_to_col_align;

  /* Interactive search */
  int selected_iter;
  int search_column;
  BobguiTreeViewSearchEqualFunc search_equal_func;
  gpointer search_user_data;
  GDestroyNotify search_destroy;
  gpointer search_position_user_data;
  GDestroyNotify search_position_destroy;
  BobguiWidget *search_popover;
  BobguiWidget *search_entry;
  gulong search_entry_changed_id;
  guint typeselect_flush_timeout;

  /* Grid and tree lines */
  BobguiTreeViewGridLines grid_lines;
  gboolean tree_lines_enabled;

  /* Row separators */
  BobguiTreeViewRowSeparatorFunc row_separator_func;
  gpointer row_separator_data;
  GDestroyNotify row_separator_destroy;

  /* Gestures */
  BobguiGesture *click_gesture;
  BobguiGesture *drag_gesture; /* Rubberbanding, row DnD */
  BobguiGesture *column_drag_gesture; /* Column reordering, resizing */

  /* Tooltip support */
  int tooltip_column;

  int expander_size;

  GdkRGBA grid_line_color; /* Color used in the textures */
  GdkTexture *horizontal_grid_line_texture;
  GdkTexture *vertical_grid_line_texture;

  GdkRGBA tree_line_color; /* Color used in the textures */
  GdkTexture *horizontal_tree_line_texture;
  GdkTexture *vertical_tree_line_texture;

  /* Here comes the bitfield */
  guint scroll_to_use_align : 1;

  guint fixed_height_mode : 1;
  guint fixed_height_check : 1;

  guint activate_on_single_click : 1;
  guint reorderable : 1;
  guint header_has_focus : 1;
  guint drag_column_surface_state : 3;
  guint mark_rows_col_dirty : 1;

  /* for DnD */
  guint empty_view_drop : 1;

  guint modify_selection_pressed : 1;
  guint extend_selection_pressed : 1;

  guint in_top_row_to_dy : 1;

  /* interactive search */
  guint enable_search : 1;
  guint disable_popdown : 1;
  guint search_custom_entry_set : 1;

  guint hover_selection : 1;
  guint hover_expand : 1;
  guint imcontext_changed : 1;

  guint in_scroll : 1;

  guint rubber_banding_enable : 1;

  guint in_grab : 1;

  /* Whether our key press handler is to avoid sending an unhandled binding to the search entry */
  guint search_entry_avoid_unhandled_binding : 1;

  /* BobguiScrollablePolicy needs to be checked when
   * driving the scrollable adjustment values */
  guint hscroll_policy : 1;
  guint vscroll_policy : 1;

  /* BobguiTreeView flags */
  guint is_list : 1;
  guint show_expanders : 1;
  guint in_column_resize : 1;
  guint arrow_prelit : 1;
  guint headers_visible : 1;
  guint draw_keyfocus : 1;
  guint model_setup : 1;
  guint in_column_drag : 1;
} BobguiTreeViewPrivate;


/* Signals */
enum
{
  ROW_ACTIVATED,
  TEST_EXPAND_ROW,
  TEST_COLLAPSE_ROW,
  ROW_EXPANDED,
  ROW_COLLAPSED,
  COLUMNS_CHANGED,
  CURSOR_CHANGED,
  MOVE_CURSOR,
  SELECT_ALL,
  UNSELECT_ALL,
  SELECT_CURSOR_ROW,
  TOGGLE_CURSOR_ROW,
  EXPAND_COLLAPSE_CURSOR_ROW,
  SELECT_CURSOR_PARENT,
  START_INTERACTIVE_SEARCH,
  LAST_SIGNAL
};

/* Properties */
enum {
  PROP_0,
  PROP_MODEL,
  PROP_HEADERS_VISIBLE,
  PROP_HEADERS_CLICKABLE,
  PROP_EXPANDER_COLUMN,
  PROP_REORDERABLE,
  PROP_ENABLE_SEARCH,
  PROP_SEARCH_COLUMN,
  PROP_FIXED_HEIGHT_MODE,
  PROP_HOVER_SELECTION,
  PROP_HOVER_EXPAND,
  PROP_SHOW_EXPANDERS,
  PROP_LEVEL_INDENTATION,
  PROP_RUBBER_BANDING,
  PROP_ENABLE_GRID_LINES,
  PROP_ENABLE_TREE_LINES,
  PROP_TOOLTIP_COLUMN,
  PROP_ACTIVATE_ON_SINGLE_CLICK,
  LAST_PROP,
  /* overridden */
  PROP_HADJUSTMENT = LAST_PROP,
  PROP_VADJUSTMENT,
  PROP_HSCROLL_POLICY,
  PROP_VSCROLL_POLICY,
};

/* object signals */
static void     bobgui_tree_view_finalize             (GObject          *object);
static void     bobgui_tree_view_dispose              (GObject          *object);
static void     bobgui_tree_view_set_property         (GObject         *object,
						    guint            prop_id,
						    const GValue    *value,
						    GParamSpec      *pspec);
static void     bobgui_tree_view_get_property         (GObject         *object,
						    guint            prop_id,
						    GValue          *value,
						    GParamSpec      *pspec);

/* bobguiwidget signals */
static void     bobgui_tree_view_realize              (BobguiWidget        *widget);
static void     bobgui_tree_view_unrealize            (BobguiWidget        *widget);
static void     bobgui_tree_view_unroot               (BobguiWidget        *widget);
static void     bobgui_tree_view_map                  (BobguiWidget        *widget);
static void     bobgui_tree_view_measure              (BobguiWidget        *widget,
                                                    BobguiOrientation  orientation,
                                                    int             for_size,
                                                    int            *minimum,
                                                    int            *natural,
                                                    int            *minimum_baseline,
                                                    int            *natural_baseline);
static void     bobgui_tree_view_size_allocate        (BobguiWidget      *widget,
                                                    int             width,
                                                    int             height,
                                                    int             baseline);
static void     bobgui_tree_view_snapshot             (BobguiWidget        *widget,
                                                    BobguiSnapshot      *snapshot);

static gboolean bobgui_tree_view_forward_controller_key_pressed  (BobguiEventControllerKey *key,
                                                               guint                  keyval,
                                                               guint                  keycode,
                                                               GdkModifierType        state,
                                                               BobguiTreeView           *tree_view);
static gboolean bobgui_tree_view_key_controller_key_pressed  (BobguiEventControllerKey *key,
                                                           guint                  keyval,
                                                           guint                  keycode,
                                                           GdkModifierType        state,
                                                           BobguiTreeView           *tree_view);
static void     bobgui_tree_view_key_controller_key_released (BobguiEventControllerKey *key,
                                                           guint                  keyval,
                                                           guint                  keycode,
                                                           GdkModifierType        state,
                                                           BobguiTreeView           *tree_view);
static void     bobgui_tree_view_focus_controller_focus_out  (BobguiEventController    *focus,
                                                           BobguiTreeView           *tree_view);

static int      bobgui_tree_view_focus                (BobguiWidget        *widget,
						    BobguiDirectionType  direction);
static gboolean bobgui_tree_view_grab_focus           (BobguiWidget        *widget);
static void     bobgui_tree_view_css_changed          (BobguiWidget        *widget,
                                                    BobguiCssStyleChange *change);

static void     bobgui_tree_view_remove               (BobguiTreeView      *tree_view,
                                                    BobguiWidget        *widget);

/* Source side drag signals */
static void bobgui_tree_view_dnd_finished_cb  (GdkDrag          *drag,
                                            BobguiWidget        *widget);
static GdkContentProvider * bobgui_tree_view_drag_data_get   (BobguiTreeView           *tree_view,
                                                           BobguiTreePath           *source_row);

/* Target side drag signals */
static void     bobgui_tree_view_drag_leave                  (BobguiDropTargetAsync    *dest,
                                                           GdkDrop               *drop,
                                                           BobguiTreeView           *tree_view);
static GdkDragAction bobgui_tree_view_drag_motion            (BobguiDropTargetAsync    *dest,
                                                           GdkDrop               *drop,
                                                           double                 x,
                                                           double                 y,
                                                           BobguiTreeView           *tree_view);
static gboolean bobgui_tree_view_drag_drop                   (BobguiDropTargetAsync    *dest,
                                                           GdkDrop               *drop,
                                                           double                 x,
                                                           double                 y,
                                                           BobguiTreeView           *tree_view);
static void     bobgui_tree_view_drag_data_received          (GObject               *source,
                                                           GAsyncResult          *result,
                                                           gpointer               data);

/* tree_model signals */
static gboolean bobgui_tree_view_real_move_cursor            (BobguiTreeView     *tree_view,
							   BobguiMovementStep  step,
							   int              count,
                                                           gboolean         extend,
                                                           gboolean         modify);
static gboolean bobgui_tree_view_real_select_all             (BobguiTreeView     *tree_view);
static gboolean bobgui_tree_view_real_unselect_all           (BobguiTreeView     *tree_view);
static gboolean bobgui_tree_view_real_select_cursor_row      (BobguiTreeView     *tree_view,
							   gboolean         start_editing);
static gboolean bobgui_tree_view_real_toggle_cursor_row      (BobguiTreeView     *tree_view);
static gboolean bobgui_tree_view_real_expand_collapse_cursor_row (BobguiTreeView     *tree_view,
							       gboolean         logical,
							       gboolean         expand,
							       gboolean         open_all);
static gboolean bobgui_tree_view_real_select_cursor_parent   (BobguiTreeView     *tree_view);
static void bobgui_tree_view_row_changed                     (BobguiTreeModel    *model,
							   BobguiTreePath     *path,
							   BobguiTreeIter     *iter,
							   gpointer         data);
static void bobgui_tree_view_row_inserted                    (BobguiTreeModel    *model,
							   BobguiTreePath     *path,
							   BobguiTreeIter     *iter,
							   gpointer         data);
static void bobgui_tree_view_row_has_child_toggled           (BobguiTreeModel    *model,
							   BobguiTreePath     *path,
							   BobguiTreeIter     *iter,
							   gpointer         data);
static void bobgui_tree_view_row_deleted                     (BobguiTreeModel    *model,
							   BobguiTreePath     *path,
							   gpointer         data);
static void bobgui_tree_view_rows_reordered                  (BobguiTreeModel    *model,
							   BobguiTreePath     *parent,
							   BobguiTreeIter     *iter,
							   int             *new_order,
							   gpointer         data);

/* Incremental reflow */
static gboolean validate_row                              (BobguiTreeView     *tree_view,
                                                           BobguiTreeRBTree   *tree,
                                                           BobguiTreeRBNode   *node,
                                                           BobguiTreeIter     *iter,
                                                           BobguiTreePath     *path);
static void     validate_visible_area    (BobguiTreeView *tree_view);
static gboolean do_validate_rows         (BobguiTreeView *tree_view,
					  gboolean     queue_resize);
static gboolean validate_rows            (BobguiTreeView *tree_view);
static void     install_presize_handler  (BobguiTreeView *tree_view);
static void     install_scroll_sync_handler (BobguiTreeView *tree_view);
static void     bobgui_tree_view_set_top_row   (BobguiTreeView *tree_view,
					     BobguiTreePath *path,
					     int          offset);
static void	bobgui_tree_view_dy_to_top_row (BobguiTreeView *tree_view);
static void     bobgui_tree_view_top_row_to_dy (BobguiTreeView *tree_view);
static void     invalidate_empty_focus      (BobguiTreeView *tree_view);

/* Internal functions */
static gboolean bobgui_tree_view_is_expander_column             (BobguiTreeView        *tree_view,
							      BobguiTreeViewColumn  *column);
static inline gboolean bobgui_tree_view_draw_expanders          (BobguiTreeView        *tree_view);
static void     bobgui_tree_view_add_move_binding               (BobguiWidgetClass     *widget_class,
							      guint               keyval,
							      guint               modmask,
							      gboolean            add_shifted_binding,
							      BobguiMovementStep     step,
							      int                 count);
static int      bobgui_tree_view_unref_and_check_selection_tree (BobguiTreeView        *tree_view,
							      BobguiTreeRBTree      *tree);
static void     bobgui_tree_view_snapshot_arrow                 (BobguiTreeView        *tree_view,
                                                              BobguiSnapshot        *snapshot,
							      BobguiTreeRBTree      *tree,
							      BobguiTreeRBNode      *node);
static void     bobgui_tree_view_get_arrow_xrange               (BobguiTreeView        *tree_view,
							      BobguiTreeRBTree      *tree,
							      int                *x1,
							      int                *x2);
static void     bobgui_tree_view_adjustment_changed             (BobguiAdjustment      *adjustment,
							      BobguiTreeView        *tree_view);
static void     bobgui_tree_view_build_tree                     (BobguiTreeView        *tree_view,
							      BobguiTreeRBTree          *tree,
							      BobguiTreeIter        *iter,
							      int                 depth,
							      gboolean            recurse);
static void     bobgui_tree_view_clamp_node_visible             (BobguiTreeView        *tree_view,
							      BobguiTreeRBTree      *tree,
							      BobguiTreeRBNode      *node);
static void     bobgui_tree_view_clamp_column_visible           (BobguiTreeView        *tree_view,
							      BobguiTreeViewColumn  *column,
							      gboolean            focus_to_cell);
static gboolean bobgui_tree_view_maybe_begin_dragging_row       (BobguiTreeView        *tree_view);
static void     bobgui_tree_view_focus_to_cursor                (BobguiTreeView        *tree_view);
static void     bobgui_tree_view_move_cursor_up_down            (BobguiTreeView        *tree_view,
							      int                 count);
static void     bobgui_tree_view_move_cursor_page_up_down       (BobguiTreeView        *tree_view,
							      int                 count);
static void     bobgui_tree_view_move_cursor_left_right         (BobguiTreeView        *tree_view,
							      int                 count);
static void     bobgui_tree_view_move_cursor_start_end          (BobguiTreeView        *tree_view,
							      int                 count);
static gboolean bobgui_tree_view_real_collapse_row              (BobguiTreeView        *tree_view,
							      BobguiTreePath        *path,
							      BobguiTreeRBTree      *tree,
							      BobguiTreeRBNode      *node);
static gboolean bobgui_tree_view_real_expand_row                (BobguiTreeView        *tree_view,
							      BobguiTreePath        *path,
							      BobguiTreeRBTree      *tree,
							      BobguiTreeRBNode      *node,
							      gboolean            open_all);
static void     bobgui_tree_view_real_set_cursor                (BobguiTreeView        *tree_view,
							      BobguiTreePath        *path,
                                                              SetCursorFlags      flags);
static gboolean bobgui_tree_view_has_can_focus_cell             (BobguiTreeView        *tree_view);
static void     column_sizing_notify                         (GObject            *object,
                                                              GParamSpec         *pspec,
                                                              gpointer            data);
static void     bobgui_tree_view_stop_rubber_band               (BobguiTreeView        *tree_view);
static void     ensure_unprelighted                          (BobguiTreeView        *tree_view);
static void     update_prelight                              (BobguiTreeView        *tree_view,
                                                              int                 x,
                                                              int                 y);

static inline int bobgui_tree_view_get_effective_header_height (BobguiTreeView        *tree_view);

static inline int bobgui_tree_view_get_cell_area_y_offset      (BobguiTreeView        *tree_view,
                                                              BobguiTreeRBTree      *tree,
                                                              BobguiTreeRBNode      *node);
static inline int bobgui_tree_view_get_cell_area_height        (BobguiTreeView        *tree_view,
                                                              BobguiTreeRBNode      *node);

static inline int bobgui_tree_view_get_row_y_offset            (BobguiTreeView        *tree_view,
                                                              BobguiTreeRBTree      *tree,
                                                              BobguiTreeRBNode      *node);
static inline int bobgui_tree_view_get_row_height              (BobguiTreeView        *tree_view,
                                                              BobguiTreeRBNode      *node);
static TreeViewDragInfo* get_info (BobguiTreeView *tree_view);

/* interactive search */
static void     bobgui_tree_view_ensure_interactive_directory (BobguiTreeView *tree_view);
static void     bobgui_tree_view_search_popover_hide       (BobguiWidget        *search_popover,
                                                         BobguiTreeView      *tree_view);
static void     bobgui_tree_view_search_preedit_changed    (BobguiText          *text,
                                                         const char       *preedit,
							 BobguiTreeView      *tree_view);
static void     bobgui_tree_view_search_changed            (BobguiEditable      *editable,
                                                         BobguiTreeView      *tree_view);
static void     bobgui_tree_view_search_activate           (BobguiEntry         *entry,
							 BobguiTreeView      *tree_view);
static void     bobgui_tree_view_search_pressed_cb         (BobguiGesture       *gesture,
                                                         int               n_press,
                                                         double            x,
                                                         double            y,
							 BobguiTreeView      *tree_view);
static gboolean bobgui_tree_view_search_scroll_event       (BobguiWidget        *entry,
							 double            dx,
                                                         double            dy,
							 BobguiTreeView      *tree_view);
static gboolean bobgui_tree_view_search_key_pressed        (BobguiEventControllerKey *key,
                                                         guint                  keyval,
                                                         guint                  keycode,
                                                         GdkModifierType        state,
                                                         BobguiTreeView           *tree_view);
static gboolean bobgui_tree_view_search_move               (BobguiWidget        *window,
							 BobguiTreeView      *tree_view,
							 gboolean          up);
static gboolean bobgui_tree_view_search_equal_func         (BobguiTreeModel     *model,
							 int               column,
							 const char       *key,
							 BobguiTreeIter      *iter,
							 gpointer          search_data);
static gboolean bobgui_tree_view_search_iter               (BobguiTreeModel     *model,
							 BobguiTreeSelection *selection,
							 BobguiTreeIter      *iter,
							 const char       *text,
							 int              *count,
							 int               n);
static void     bobgui_tree_view_search_init               (BobguiWidget        *entry,
							 BobguiTreeView      *tree_view);
static void     bobgui_tree_view_put                       (BobguiTreeView      *tree_view,
							 BobguiWidget        *child_widget,
                                                         BobguiTreePath      *path,
                                                         BobguiTreeViewColumn*column,
                                                         const BobguiBorder  *border);
static gboolean bobgui_tree_view_start_editing             (BobguiTreeView      *tree_view,
							 BobguiTreePath      *cursor_path,
							 gboolean          edit_only);
static void bobgui_tree_view_stop_editing                  (BobguiTreeView *tree_view,
							 gboolean     cancel_editing);
static gboolean bobgui_tree_view_real_start_interactive_search (BobguiTreeView *tree_view,
							     gboolean     keybinding);
static gboolean bobgui_tree_view_start_interactive_search      (BobguiTreeView *tree_view);
static BobguiTreeViewColumn *bobgui_tree_view_get_drop_column (BobguiTreeView       *tree_view,
							 BobguiTreeViewColumn *column,
							 int                drop_position);

/* BobguiBuildable */
static void     bobgui_tree_view_buildable_add_child          (BobguiBuildable      *tree_view,
							    BobguiBuilder        *builder,
							    GObject           *child,
							    const char        *type);
static GObject *bobgui_tree_view_buildable_get_internal_child (BobguiBuildable      *buildable,
							    BobguiBuilder        *builder,
							    const char        *childname);
static void     bobgui_tree_view_buildable_init               (BobguiBuildableIface *iface);

/* BobguiScrollable */
static void     bobgui_tree_view_scrollable_init              (BobguiScrollableInterface *iface);

static void           bobgui_tree_view_do_set_hadjustment (BobguiTreeView   *tree_view,
                                                        BobguiAdjustment *adjustment);
static void           bobgui_tree_view_do_set_vadjustment (BobguiTreeView   *tree_view,
                                                        BobguiAdjustment *adjustment);

static gboolean scroll_row_timeout                   (gpointer     data);
static void     add_scroll_timeout                   (BobguiTreeView *tree_view);
static void     remove_scroll_timeout                (BobguiTreeView *tree_view);

static void     grab_focus_and_unset_draw_keyfocus   (BobguiTreeView *tree_view);

/* Gestures */
static void bobgui_tree_view_column_click_gesture_pressed (BobguiGestureClick *gesture,
                                                             int                   n_press,
                                                             double                x,
                                                             double                y,
                                                             BobguiTreeView          *tree_view);

static void bobgui_tree_view_click_gesture_pressed        (BobguiGestureClick *gesture,
                                                             int                   n_press,
                                                             double                x,
                                                             double                y,
                                                             BobguiTreeView          *tree_view);
static void bobgui_tree_view_click_gesture_released       (BobguiGestureClick *gesture,
                                                             int                   n_press,
                                                             double                x,
                                                             double                y,
                                                             BobguiTreeView          *tree_view);

static void bobgui_tree_view_column_drag_gesture_begin         (BobguiGestureDrag *gesture,
                                                             double          start_x,
                                                             double          start_y,
                                                             BobguiTreeView    *tree_view);
static void bobgui_tree_view_column_drag_gesture_update        (BobguiGestureDrag *gesture,
                                                             double          offset_x,
                                                             double          offset_y,
                                                             BobguiTreeView    *tree_view);
static void bobgui_tree_view_column_drag_gesture_end           (BobguiGestureDrag *gesture,
                                                             double          offset_x,
                                                             double          offset_y,
                                                             BobguiTreeView    *tree_view);

static void bobgui_tree_view_drag_gesture_begin                (BobguiGestureDrag *gesture,
                                                             double          start_x,
                                                             double          start_y,
                                                             BobguiTreeView    *tree_view);
static void bobgui_tree_view_drag_gesture_update               (BobguiGestureDrag *gesture,
                                                             double          offset_x,
                                                             double          offset_y,
                                                             BobguiTreeView    *tree_view);
static void bobgui_tree_view_drag_gesture_end                  (BobguiGestureDrag *gesture,
                                                             double          offset_x,
                                                             double          offset_y,
                                                             BobguiTreeView    *tree_view);
static void bobgui_tree_view_motion_controller_enter           (BobguiEventControllerMotion *controller,
                                                             double                    x,
                                                             double                    y,
                                                             BobguiTreeView              *tree_view);
static void bobgui_tree_view_motion_controller_leave           (BobguiEventControllerMotion *controller,
                                                             BobguiTreeView              *tree_view);
static void bobgui_tree_view_motion_controller_motion          (BobguiEventControllerMotion *controller,
                                                             double                    x,
                                                             double                    y,
                                                             BobguiTreeView              *tree_view);

static guint tree_view_signals [LAST_SIGNAL] = { 0 };
static GParamSpec *tree_view_props [LAST_PROP] = { NULL };



/* GType Methods
 */

G_DEFINE_TYPE_WITH_CODE (BobguiTreeView, bobgui_tree_view, BOBGUI_TYPE_WIDGET,
                         G_ADD_PRIVATE (BobguiTreeView)
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_BUILDABLE,
                                                bobgui_tree_view_buildable_init)
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_SCROLLABLE,
                                                bobgui_tree_view_scrollable_init))

static void
bobgui_tree_view_class_init (BobguiTreeViewClass *class)
{
  GObjectClass *o_class = G_OBJECT_CLASS (class);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (class);

  /* GObject signals */
  o_class->set_property = bobgui_tree_view_set_property;
  o_class->get_property = bobgui_tree_view_get_property;
  o_class->finalize = bobgui_tree_view_finalize;
  o_class->dispose = bobgui_tree_view_dispose;

  /* BobguiWidget signals */
  widget_class->map = bobgui_tree_view_map;
  widget_class->realize = bobgui_tree_view_realize;
  widget_class->unrealize = bobgui_tree_view_unrealize;
  widget_class->unroot = bobgui_tree_view_unroot;
  widget_class->measure = bobgui_tree_view_measure;
  widget_class->size_allocate = bobgui_tree_view_size_allocate;
  widget_class->snapshot = bobgui_tree_view_snapshot;
  widget_class->focus = bobgui_tree_view_focus;
  widget_class->grab_focus = bobgui_tree_view_grab_focus;
  widget_class->css_changed = bobgui_tree_view_css_changed;

  class->move_cursor = bobgui_tree_view_real_move_cursor;
  class->select_all = bobgui_tree_view_real_select_all;
  class->unselect_all = bobgui_tree_view_real_unselect_all;
  class->select_cursor_row = bobgui_tree_view_real_select_cursor_row;
  class->toggle_cursor_row = bobgui_tree_view_real_toggle_cursor_row;
  class->expand_collapse_cursor_row = bobgui_tree_view_real_expand_collapse_cursor_row;
  class->select_cursor_parent = bobgui_tree_view_real_select_cursor_parent;
  class->start_interactive_search = bobgui_tree_view_start_interactive_search;

  /* Properties */

  g_object_class_override_property (o_class, PROP_HADJUSTMENT,    "hadjustment");
  g_object_class_override_property (o_class, PROP_VADJUSTMENT,    "vadjustment");
  g_object_class_override_property (o_class, PROP_HSCROLL_POLICY, "hscroll-policy");
  g_object_class_override_property (o_class, PROP_VSCROLL_POLICY, "vscroll-policy");

  tree_view_props[PROP_MODEL] =
      g_param_spec_object ("model", NULL, NULL,
                           BOBGUI_TYPE_TREE_MODEL,
                           BOBGUI_PARAM_READWRITE);

  tree_view_props[PROP_HEADERS_VISIBLE] =
      g_param_spec_boolean ("headers-visible", NULL, NULL,
                            TRUE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  tree_view_props[PROP_HEADERS_CLICKABLE] =
      g_param_spec_boolean ("headers-clickable", NULL, NULL,
                            TRUE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  tree_view_props[PROP_EXPANDER_COLUMN] =
      g_param_spec_object ("expander-column", NULL, NULL,
                           BOBGUI_TYPE_TREE_VIEW_COLUMN,
                           BOBGUI_PARAM_READWRITE);

  tree_view_props[PROP_REORDERABLE] =
      g_param_spec_boolean ("reorderable", NULL, NULL,
                            FALSE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  tree_view_props[PROP_ENABLE_SEARCH] =
      g_param_spec_boolean ("enable-search", NULL, NULL,
                            TRUE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  tree_view_props[PROP_SEARCH_COLUMN] =
      g_param_spec_int ("search-column", NULL, NULL,
                        -1, G_MAXINT,
                        -1,
                        BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiTreeView:fixed-height-mode:
   *
   * Setting the ::fixed-height-mode property to %TRUE speeds up
   * `BobguiTreeView` by assuming that all rows have the same height.
   * Only enable this option if all rows are the same height.
   * Please see bobgui_tree_view_set_fixed_height_mode() for more
   * information on this option.
   */
  tree_view_props[PROP_FIXED_HEIGHT_MODE] =
      g_param_spec_boolean ("fixed-height-mode", NULL, NULL,
                            FALSE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiTreeView:hover-selection:
   *
   * Enables or disables the hover selection mode of @tree_view.
   * Hover selection makes the selected row follow the pointer.
   * Currently, this works only for the selection modes
   * %BOBGUI_SELECTION_SINGLE and %BOBGUI_SELECTION_BROWSE.
   *
   * This mode is primarily intended for treeviews in popups, e.g.
   * in `BobguiComboBox` or `BobguiEntryCompletion`.
   */
  tree_view_props[PROP_HOVER_SELECTION] =
      g_param_spec_boolean ("hover-selection", NULL, NULL,
                            FALSE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiTreeView:hover-expand:
   *
   * Enables or disables the hover expansion mode of @tree_view.
   * Hover expansion makes rows expand or collapse if the pointer moves
   * over them.
   *
   * This mode is primarily intended for treeviews in popups, e.g.
   * in `BobguiComboBox` or `BobguiEntryCompletion`.
   */
  tree_view_props[PROP_HOVER_EXPAND] =
      g_param_spec_boolean ("hover-expand", NULL, NULL,
                            FALSE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiTreeView:show-expanders:
   *
   * %TRUE if the view has expanders.
   */
  tree_view_props[PROP_SHOW_EXPANDERS] =
      g_param_spec_boolean ("show-expanders", NULL, NULL,
                            TRUE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiTreeView:level-indentation:
   *
   * Extra indentation for each level.
   */
  tree_view_props[PROP_LEVEL_INDENTATION] =
      g_param_spec_int ("level-indentation", NULL, NULL,
                        0, G_MAXINT,
                        0,
                        BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  tree_view_props[PROP_RUBBER_BANDING] =
      g_param_spec_boolean ("rubber-banding", NULL, NULL,
                            FALSE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  tree_view_props[PROP_ENABLE_GRID_LINES] =
      g_param_spec_enum ("enable-grid-lines", NULL, NULL,
                         BOBGUI_TYPE_TREE_VIEW_GRID_LINES,
                         BOBGUI_TREE_VIEW_GRID_LINES_NONE,
                         BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  tree_view_props[PROP_ENABLE_TREE_LINES] =
      g_param_spec_boolean ("enable-tree-lines", NULL, NULL,
                            FALSE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  tree_view_props[PROP_TOOLTIP_COLUMN] =
      g_param_spec_int ("tooltip-column", NULL, NULL,
                        -1, G_MAXINT,
                        -1,
                        BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiTreeView:activate-on-single-click:
   *
   * The activate-on-single-click property specifies whether the "row-activated" signal
   * will be emitted after a single click.
   */
  tree_view_props[PROP_ACTIVATE_ON_SINGLE_CLICK] =
      g_param_spec_boolean ("activate-on-single-click", NULL, NULL,
                            FALSE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (o_class, LAST_PROP, tree_view_props);

  /* Signals */
  /**
   * BobguiTreeView::row-activated:
   * @tree_view: the object on which the signal is emitted
   * @path: the `BobguiTreePath` for the activated row
   * @column: (nullable): the `BobguiTreeViewColumn` in which the activation occurred
   *
   * The "row-activated" signal is emitted when the method
   * [method@Bobgui.TreeView.row_activated] is called.
   *
   * This signal is emitted when the user double-clicks a treeview row with the
   * [property@Bobgui.TreeView:activate-on-single-click] property set to %FALSE,
   * or when the user single-clicks a row when that property set to %TRUE.
   *
   * This signal is also emitted when a non-editable row is selected and one
   * of the keys: <kbd>Space</kbd>, <kbd>Shift</kbd>+<kbd>Space</kbd>,
   * <kbd>Return</kbd> or <kbd>Enter</kbd> is pressed.
   *
   * For selection handling refer to the
   * [tree widget conceptual overview](section-tree-widget.html)
   * as well as `BobguiTreeSelection`.
   */
  tree_view_signals[ROW_ACTIVATED] =
    g_signal_new (I_("row-activated"),
		  G_TYPE_FROM_CLASS (o_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (BobguiTreeViewClass, row_activated),
		  NULL, NULL,
		  _bobgui_marshal_VOID__BOXED_OBJECT,
		  G_TYPE_NONE, 2,
		  BOBGUI_TYPE_TREE_PATH,
		  BOBGUI_TYPE_TREE_VIEW_COLUMN);
  g_signal_set_va_marshaller (tree_view_signals[ROW_ACTIVATED],
                              G_TYPE_FROM_CLASS (o_class),
                              _bobgui_marshal_VOID__BOXED_OBJECTv);

  /**
   * BobguiTreeView::test-expand-row:
   * @tree_view: the object on which the signal is emitted
   * @iter: the tree iter of the row to expand
   * @path: a tree path that points to the row
   *
   * The given row is about to be expanded (show its children nodes). Use this
   * signal if you need to control the expandability of individual rows.
   *
   * Returns: %FALSE to allow expansion, %TRUE to reject
   */
  tree_view_signals[TEST_EXPAND_ROW] =
    g_signal_new (I_("test-expand-row"),
		  G_TYPE_FROM_CLASS (o_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BobguiTreeViewClass, test_expand_row),
		  _bobgui_boolean_handled_accumulator, NULL,
		  _bobgui_marshal_BOOLEAN__BOXED_BOXED,
		  G_TYPE_BOOLEAN, 2,
		  BOBGUI_TYPE_TREE_ITER,
		  BOBGUI_TYPE_TREE_PATH);
  g_signal_set_va_marshaller (tree_view_signals[TEST_EXPAND_ROW],
                              G_TYPE_FROM_CLASS (o_class),
                              _bobgui_marshal_BOOLEAN__BOXED_BOXEDv);

  /**
   * BobguiTreeView::test-collapse-row:
   * @tree_view: the object on which the signal is emitted
   * @iter: the tree iter of the row to collapse
   * @path: a tree path that points to the row
   *
   * The given row is about to be collapsed (hide its children nodes). Use this
   * signal if you need to control the collapsibility of individual rows.
   *
   * Returns: %FALSE to allow collapsing, %TRUE to reject
   */
  tree_view_signals[TEST_COLLAPSE_ROW] =
    g_signal_new (I_("test-collapse-row"),
		  G_TYPE_FROM_CLASS (o_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BobguiTreeViewClass, test_collapse_row),
		  _bobgui_boolean_handled_accumulator, NULL,
		  _bobgui_marshal_BOOLEAN__BOXED_BOXED,
		  G_TYPE_BOOLEAN, 2,
		  BOBGUI_TYPE_TREE_ITER,
		  BOBGUI_TYPE_TREE_PATH);
  g_signal_set_va_marshaller (tree_view_signals[TEST_COLLAPSE_ROW],
                              G_TYPE_FROM_CLASS (o_class),
                              _bobgui_marshal_BOOLEAN__BOXED_BOXEDv);

  /**
   * BobguiTreeView::row-expanded:
   * @tree_view: the object on which the signal is emitted
   * @iter: the tree iter of the expanded row
   * @path: a tree path that points to the row
   *
   * The given row has been expanded (child nodes are shown).
   */
  tree_view_signals[ROW_EXPANDED] =
    g_signal_new (I_("row-expanded"),
		  G_TYPE_FROM_CLASS (o_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BobguiTreeViewClass, row_expanded),
		  NULL, NULL,
		  _bobgui_marshal_VOID__BOXED_BOXED,
		  G_TYPE_NONE, 2,
		  BOBGUI_TYPE_TREE_ITER,
		  BOBGUI_TYPE_TREE_PATH);
  g_signal_set_va_marshaller (tree_view_signals[ROW_EXPANDED],
                              G_TYPE_FROM_CLASS (o_class),
                              _bobgui_marshal_VOID__BOXED_BOXEDv);

  /**
   * BobguiTreeView::row-collapsed:
   * @tree_view: the object on which the signal is emitted
   * @iter: the tree iter of the collapsed row
   * @path: a tree path that points to the row
   *
   * The given row has been collapsed (child nodes are hidden).
   */
  tree_view_signals[ROW_COLLAPSED] =
    g_signal_new (I_("row-collapsed"),
		  G_TYPE_FROM_CLASS (o_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BobguiTreeViewClass, row_collapsed),
		  NULL, NULL,
		  _bobgui_marshal_VOID__BOXED_BOXED,
		  G_TYPE_NONE, 2,
		  BOBGUI_TYPE_TREE_ITER,
		  BOBGUI_TYPE_TREE_PATH);
  g_signal_set_va_marshaller (tree_view_signals[ROW_COLLAPSED],
                              G_TYPE_FROM_CLASS (o_class),
                              _bobgui_marshal_VOID__BOXED_BOXEDv);

  /**
   * BobguiTreeView::columns-changed:
   * @tree_view: the object on which the signal is emitted
   *
   * The number of columns of the treeview has changed.
   */
  tree_view_signals[COLUMNS_CHANGED] =
    g_signal_new (I_("columns-changed"),
		  G_TYPE_FROM_CLASS (o_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BobguiTreeViewClass, columns_changed),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 0);

  /**
   * BobguiTreeView::cursor-changed:
   * @tree_view: the object on which the signal is emitted
   *
   * The position of the cursor (focused cell) has changed.
   */
  tree_view_signals[CURSOR_CHANGED] =
    g_signal_new (I_("cursor-changed"),
		  G_TYPE_FROM_CLASS (o_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BobguiTreeViewClass, cursor_changed),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 0);

  /**
   * BobguiTreeView::move-cursor:
   * @tree_view: the object on which the signal is emitted.
   * @step: the granularity of the move, as a `BobguiMovementStep`.
   *     %BOBGUI_MOVEMENT_LOGICAL_POSITIONS, %BOBGUI_MOVEMENT_VISUAL_POSITIONS,
   *     %BOBGUI_MOVEMENT_DISPLAY_LINES, %BOBGUI_MOVEMENT_PAGES and
   *     %BOBGUI_MOVEMENT_BUFFER_ENDS are supported.
   *     %BOBGUI_MOVEMENT_LOGICAL_POSITIONS and %BOBGUI_MOVEMENT_VISUAL_POSITIONS
   *     are treated identically.
   * @direction: the direction to move: +1 to move forwards; -1 to move
   *     backwards. The resulting movement is undefined for all other values.
   * @extend: whether to extend the selection
   * @modify: whether to modify the selection
   *
   * The `BobguiTreeView`::move-cursor signal is a [keybinding
   * signal][class@Bobgui.SignalAction] which gets emitted when the user
   * presses one of the cursor keys.
   *
   * Applications should not connect to it, but may emit it with
   * g_signal_emit_by_name() if they need to control the cursor
   * programmatically. In contrast to bobgui_tree_view_set_cursor() and
   * bobgui_tree_view_set_cursor_on_cell() when moving horizontally
   * `BobguiTreeView`::move-cursor does not reset the current selection.
   *
   * Returns: %TRUE if @step is supported, %FALSE otherwise.
   */
  tree_view_signals[MOVE_CURSOR] =
    g_signal_new (I_("move-cursor"),
		  G_TYPE_FROM_CLASS (o_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (BobguiTreeViewClass, move_cursor),
		  NULL, NULL,
		  _bobgui_marshal_BOOLEAN__ENUM_INT_BOOLEAN_BOOLEAN,
		  G_TYPE_BOOLEAN, 4,
		  BOBGUI_TYPE_MOVEMENT_STEP,
		  G_TYPE_INT,
                  G_TYPE_BOOLEAN,
                  G_TYPE_BOOLEAN);
  g_signal_set_va_marshaller (tree_view_signals[MOVE_CURSOR],
                              G_TYPE_FROM_CLASS (o_class),
                              _bobgui_marshal_BOOLEAN__ENUM_INT_BOOLEAN_BOOLEANv);

  tree_view_signals[SELECT_ALL] =
    g_signal_new (I_("select-all"),
		  G_TYPE_FROM_CLASS (o_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (BobguiTreeViewClass, select_all),
		  NULL, NULL,
		  _bobgui_marshal_BOOLEAN__VOID,
		  G_TYPE_BOOLEAN, 0);
  g_signal_set_va_marshaller (tree_view_signals[SELECT_ALL],
                              G_TYPE_FROM_CLASS (o_class),
                              _bobgui_marshal_BOOLEAN__VOIDv);

  tree_view_signals[UNSELECT_ALL] =
    g_signal_new (I_("unselect-all"),
		  G_TYPE_FROM_CLASS (o_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (BobguiTreeViewClass, unselect_all),
		  NULL, NULL,
		  _bobgui_marshal_BOOLEAN__VOID,
		  G_TYPE_BOOLEAN, 0);
  g_signal_set_va_marshaller (tree_view_signals[UNSELECT_ALL],
                              G_TYPE_FROM_CLASS (o_class),
                              _bobgui_marshal_BOOLEAN__VOIDv);

  tree_view_signals[SELECT_CURSOR_ROW] =
    g_signal_new (I_("select-cursor-row"),
		  G_TYPE_FROM_CLASS (o_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (BobguiTreeViewClass, select_cursor_row),
		  NULL, NULL,
		  _bobgui_marshal_BOOLEAN__BOOLEAN,
		  G_TYPE_BOOLEAN, 1,
		  G_TYPE_BOOLEAN);
  g_signal_set_va_marshaller (tree_view_signals[SELECT_CURSOR_ROW],
                              G_TYPE_FROM_CLASS (o_class),
                              _bobgui_marshal_BOOLEAN__BOOLEANv);

  tree_view_signals[TOGGLE_CURSOR_ROW] =
    g_signal_new (I_("toggle-cursor-row"),
		  G_TYPE_FROM_CLASS (o_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (BobguiTreeViewClass, toggle_cursor_row),
		  NULL, NULL,
		  _bobgui_marshal_BOOLEAN__VOID,
		  G_TYPE_BOOLEAN, 0);
  g_signal_set_va_marshaller (tree_view_signals[TOGGLE_CURSOR_ROW],
                              G_TYPE_FROM_CLASS (o_class),
                              _bobgui_marshal_BOOLEAN__VOIDv);

  tree_view_signals[EXPAND_COLLAPSE_CURSOR_ROW] =
    g_signal_new (I_("expand-collapse-cursor-row"),
		  G_TYPE_FROM_CLASS (o_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (BobguiTreeViewClass, expand_collapse_cursor_row),
		  NULL, NULL,
		  _bobgui_marshal_BOOLEAN__BOOLEAN_BOOLEAN_BOOLEAN,
		  G_TYPE_BOOLEAN, 3,
		  G_TYPE_BOOLEAN,
		  G_TYPE_BOOLEAN,
		  G_TYPE_BOOLEAN);
  g_signal_set_va_marshaller (tree_view_signals[EXPAND_COLLAPSE_CURSOR_ROW],
                              G_TYPE_FROM_CLASS (o_class),
                              _bobgui_marshal_BOOLEAN__BOOLEAN_BOOLEAN_BOOLEANv);

  tree_view_signals[SELECT_CURSOR_PARENT] =
    g_signal_new (I_("select-cursor-parent"),
		  G_TYPE_FROM_CLASS (o_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (BobguiTreeViewClass, select_cursor_parent),
		  NULL, NULL,
		  _bobgui_marshal_BOOLEAN__VOID,
		  G_TYPE_BOOLEAN, 0);
  g_signal_set_va_marshaller (tree_view_signals[SELECT_CURSOR_PARENT],
                              G_TYPE_FROM_CLASS (o_class),
                              _bobgui_marshal_BOOLEAN__VOIDv);

  tree_view_signals[START_INTERACTIVE_SEARCH] =
    g_signal_new (I_("start-interactive-search"),
		  G_TYPE_FROM_CLASS (o_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (BobguiTreeViewClass, start_interactive_search),
		  NULL, NULL,
		  _bobgui_marshal_BOOLEAN__VOID,
		  G_TYPE_BOOLEAN, 0);
  g_signal_set_va_marshaller (tree_view_signals[START_INTERACTIVE_SEARCH],
                              G_TYPE_FROM_CLASS (o_class),
                              _bobgui_marshal_BOOLEAN__VOIDv);

  /* Key bindings */
  bobgui_tree_view_add_move_binding (widget_class, GDK_KEY_Up, 0, TRUE,
				  BOBGUI_MOVEMENT_DISPLAY_LINES, -1);
  bobgui_tree_view_add_move_binding (widget_class, GDK_KEY_KP_Up, 0, TRUE,
				  BOBGUI_MOVEMENT_DISPLAY_LINES, -1);

  bobgui_tree_view_add_move_binding (widget_class, GDK_KEY_Down, 0, TRUE,
				  BOBGUI_MOVEMENT_DISPLAY_LINES, 1);
  bobgui_tree_view_add_move_binding (widget_class, GDK_KEY_KP_Down, 0, TRUE,
				  BOBGUI_MOVEMENT_DISPLAY_LINES, 1);

  bobgui_tree_view_add_move_binding (widget_class, GDK_KEY_p, GDK_CONTROL_MASK, FALSE,
				  BOBGUI_MOVEMENT_DISPLAY_LINES, -1);

  bobgui_tree_view_add_move_binding (widget_class, GDK_KEY_n, GDK_CONTROL_MASK, FALSE,
				  BOBGUI_MOVEMENT_DISPLAY_LINES, 1);

  bobgui_tree_view_add_move_binding (widget_class, GDK_KEY_Home, 0, TRUE,
				  BOBGUI_MOVEMENT_BUFFER_ENDS, -1);
  bobgui_tree_view_add_move_binding (widget_class, GDK_KEY_KP_Home, 0, TRUE,
				  BOBGUI_MOVEMENT_BUFFER_ENDS, -1);

  bobgui_tree_view_add_move_binding (widget_class, GDK_KEY_End, 0, TRUE,
				  BOBGUI_MOVEMENT_BUFFER_ENDS, 1);
  bobgui_tree_view_add_move_binding (widget_class, GDK_KEY_KP_End, 0, TRUE,
				  BOBGUI_MOVEMENT_BUFFER_ENDS, 1);

  bobgui_tree_view_add_move_binding (widget_class, GDK_KEY_Page_Up, 0, TRUE,
				  BOBGUI_MOVEMENT_PAGES, -1);
  bobgui_tree_view_add_move_binding (widget_class, GDK_KEY_KP_Page_Up, 0, TRUE,
				  BOBGUI_MOVEMENT_PAGES, -1);

  bobgui_tree_view_add_move_binding (widget_class, GDK_KEY_Page_Down, 0, TRUE,
				  BOBGUI_MOVEMENT_PAGES, 1);
  bobgui_tree_view_add_move_binding (widget_class, GDK_KEY_KP_Page_Down, 0, TRUE,
				  BOBGUI_MOVEMENT_PAGES, 1);

  bobgui_tree_view_add_move_binding (widget_class, GDK_KEY_Right, 0, FALSE,
                                  BOBGUI_MOVEMENT_VISUAL_POSITIONS, 1);
  bobgui_tree_view_add_move_binding (widget_class, GDK_KEY_Left, 0, FALSE,
                                  BOBGUI_MOVEMENT_VISUAL_POSITIONS, -1);

  bobgui_tree_view_add_move_binding (widget_class, GDK_KEY_KP_Right, 0, FALSE,
                                  BOBGUI_MOVEMENT_VISUAL_POSITIONS, 1);
  bobgui_tree_view_add_move_binding (widget_class, GDK_KEY_KP_Left, 0, FALSE,
                                  BOBGUI_MOVEMENT_VISUAL_POSITIONS, -1);

  bobgui_widget_class_add_binding_signal (widget_class, GDK_KEY_space, GDK_CONTROL_MASK, "toggle-cursor-row", NULL);
  bobgui_widget_class_add_binding_signal (widget_class, GDK_KEY_KP_Space, GDK_CONTROL_MASK, "toggle-cursor-row", NULL);


#ifdef __APPLE__
  bobgui_widget_class_add_binding_signal (widget_class, GDK_KEY_a, GDK_META_MASK, "select-all", NULL);
  bobgui_widget_class_add_binding_signal (widget_class, GDK_KEY_A, GDK_META_MASK | GDK_SHIFT_MASK, "unselect-all", NULL);
#else
  bobgui_widget_class_add_binding_signal (widget_class, GDK_KEY_a, GDK_CONTROL_MASK, "select-all", NULL);
  bobgui_widget_class_add_binding_signal (widget_class, GDK_KEY_slash, GDK_CONTROL_MASK, "select-all", NULL);

  bobgui_widget_class_add_binding_signal (widget_class, GDK_KEY_A, GDK_CONTROL_MASK | GDK_SHIFT_MASK, "unselect-all", NULL);
  bobgui_widget_class_add_binding_signal (widget_class, GDK_KEY_backslash, GDK_CONTROL_MASK, "unselect-all", NULL);
#endif

  bobgui_widget_class_add_binding_signal (widget_class, GDK_KEY_space, GDK_SHIFT_MASK, "select-cursor-row", "(b)", TRUE);
  bobgui_widget_class_add_binding_signal (widget_class, GDK_KEY_KP_Space, GDK_SHIFT_MASK, "select-cursor-row", "(b)", TRUE);

  bobgui_widget_class_add_binding_signal (widget_class, GDK_KEY_space, 0, "select-cursor-row", "(b)", TRUE);
  bobgui_widget_class_add_binding_signal (widget_class, GDK_KEY_KP_Space, 0, "select-cursor-row", "(b)", TRUE);
  bobgui_widget_class_add_binding_signal (widget_class, GDK_KEY_Return, 0, "select-cursor-row", "(b)", TRUE);
  bobgui_widget_class_add_binding_signal (widget_class, GDK_KEY_ISO_Enter, 0, "select-cursor-row", "(b)", TRUE);
  bobgui_widget_class_add_binding_signal (widget_class, GDK_KEY_KP_Enter, 0, "select-cursor-row", "(b)", TRUE);

  /* expand and collapse rows */
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_plus, 0,
                                       "expand-collapse-cursor-row",
                                       "(bbb)", TRUE, TRUE, FALSE);

  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_asterisk, 0,
                                       "expand-collapse-cursor-row",
                                       "(bbb)", TRUE, TRUE, TRUE);
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_KP_Multiply, 0,
                                       "expand-collapse-cursor-row",
                                       "(bbb)", TRUE, TRUE, TRUE);

  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_slash, 0,
                                       "expand-collapse-cursor-row",
                                       "(bbb)", TRUE, FALSE, FALSE);
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_KP_Divide, 0,
                                       "expand-collapse-cursor-row",
                                       "(bbb)", TRUE, FALSE, FALSE);

  /* Not doable on US keyboards */
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_plus, GDK_SHIFT_MASK,
                                       "expand-collapse-cursor-row",
                                       "(bbb)", TRUE, TRUE, TRUE);
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_KP_Add, 0,
                                       "expand-collapse-cursor-row",
                                       "(bbb)", TRUE, TRUE, FALSE);
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_KP_Add, GDK_SHIFT_MASK,
                                       "expand-collapse-cursor-row",
                                       "(bbb)", TRUE, TRUE, TRUE);
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_KP_Add, GDK_SHIFT_MASK,
                                       "expand-collapse-cursor-row",
                                       "(bbb)", TRUE, TRUE, TRUE);
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_Right, GDK_SHIFT_MASK,
                                       "expand-collapse-cursor-row",
                                       "(bbb)", FALSE, TRUE, TRUE);
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_KP_Right, GDK_SHIFT_MASK,
                                       "expand-collapse-cursor-row",
                                       "(bbb)", FALSE, TRUE, TRUE);
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_Right, GDK_CONTROL_MASK | GDK_SHIFT_MASK,
                                       "expand-collapse-cursor-row",
                                       "(bbb)", FALSE, TRUE, TRUE);
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_KP_Right, GDK_CONTROL_MASK | GDK_SHIFT_MASK,
                                       "expand-collapse-cursor-row",
                                       "(bbb)", FALSE, TRUE, TRUE);

  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_minus, 0,
                                       "expand-collapse-cursor-row",
                                       "(bbb)", TRUE, FALSE, FALSE);
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_minus, GDK_SHIFT_MASK,
                                       "expand-collapse-cursor-row",
                                       "(bbb)", TRUE, FALSE, TRUE);
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_KP_Subtract, 0,
                                       "expand-collapse-cursor-row",
                                       "(bbb)", TRUE, FALSE, FALSE);
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_KP_Subtract, GDK_SHIFT_MASK,
                                       "expand-collapse-cursor-row",
                                       "(bbb)", TRUE, FALSE, TRUE);
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_Left, GDK_SHIFT_MASK,
                                       "expand-collapse-cursor-row",
                                       "(bbb)", FALSE, FALSE, TRUE);
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_KP_Left, GDK_SHIFT_MASK,
                                       "expand-collapse-cursor-row",
                                       "(bbb)", FALSE, FALSE, TRUE);
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_Left, GDK_CONTROL_MASK | GDK_SHIFT_MASK,
                                       "expand-collapse-cursor-row",
                                       "(bbb)", FALSE, FALSE, TRUE);
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_KP_Left, GDK_CONTROL_MASK | GDK_SHIFT_MASK,
                                       "expand-collapse-cursor-row",
                                       "(bbb)", FALSE, FALSE, TRUE);

  bobgui_widget_class_add_binding_signal (widget_class, GDK_KEY_BackSpace, 0, "select-cursor-parent", NULL);
  bobgui_widget_class_add_binding_signal (widget_class, GDK_KEY_BackSpace, GDK_CONTROL_MASK, "select-cursor-parent", NULL);

  bobgui_widget_class_add_binding_signal (widget_class, GDK_KEY_f, GDK_CONTROL_MASK, "start-interactive-search", NULL);

  bobgui_widget_class_add_binding_signal (widget_class, GDK_KEY_F, GDK_CONTROL_MASK, "start-interactive-search", NULL);

  bobgui_widget_class_set_css_name (widget_class, I_("treeview"));
}

static void
bobgui_tree_view_init (BobguiTreeView *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  BobguiCssNode *widget_node;
  BobguiGesture *gesture;
  BobguiEventController *controller;
  BobguiEventController **controllers;
  guint n_controllers, i;

  bobgui_widget_set_overflow (BOBGUI_WIDGET (tree_view), BOBGUI_OVERFLOW_HIDDEN);
  bobgui_widget_set_focusable (BOBGUI_WIDGET (tree_view), TRUE);

  priv->show_expanders = TRUE;
  priv->draw_keyfocus = TRUE;
  priv->headers_visible = TRUE;
  priv->activate_on_single_click = FALSE;

  /* We need some padding */
  priv->dy = 0;
  priv->cursor_offset = 0;
  priv->n_columns = 0;
  priv->header_height = 1;
  priv->x_drag = 0;
  priv->drag_pos = -1;
  priv->header_has_focus = FALSE;
  priv->press_start_x = -1;
  priv->press_start_y = -1;
  priv->reorderable = FALSE;
  priv->presize_handler_tick_cb = 0;
  priv->scroll_sync_timer = 0;
  priv->fixed_height = -1;
  priv->fixed_height_mode = FALSE;
  priv->fixed_height_check = 0;
  priv->selection = _bobgui_tree_selection_new_with_tree_view (tree_view);
  priv->enable_search = TRUE;
  priv->search_column = -1;
  priv->search_equal_func = bobgui_tree_view_search_equal_func;
  priv->search_custom_entry_set = FALSE;
  priv->typeselect_flush_timeout = 0;
  priv->width = 0;
  priv->expander_size = -1;

  priv->hover_selection = FALSE;
  priv->hover_expand = FALSE;

  priv->level_indentation = 0;

  priv->rubber_banding_enable = FALSE;

  priv->grid_lines = BOBGUI_TREE_VIEW_GRID_LINES_NONE;
  priv->tree_lines_enabled = FALSE;

  priv->tooltip_column = -1;

  priv->event_last_x = -10000;
  priv->event_last_y = -10000;

  bobgui_tree_view_do_set_vadjustment (tree_view, NULL);
  bobgui_tree_view_do_set_hadjustment (tree_view, NULL);

  bobgui_widget_add_css_class (BOBGUI_WIDGET (tree_view), "view");

  widget_node = bobgui_widget_get_css_node (BOBGUI_WIDGET (tree_view));
  priv->header_node = bobgui_css_node_new ();
  bobgui_css_node_set_name (priv->header_node, g_quark_from_static_string ("header"));
  bobgui_css_node_set_parent (priv->header_node, widget_node);
  bobgui_css_node_set_state (priv->header_node, bobgui_css_node_get_state (widget_node));
  g_object_unref (priv->header_node);

  controller = bobgui_event_controller_key_new ();
  g_signal_connect (controller, "key-pressed",
                    G_CALLBACK (bobgui_tree_view_forward_controller_key_pressed), tree_view);
  bobgui_widget_add_controller (BOBGUI_WIDGET (tree_view), controller);

  controllers = bobgui_widget_list_controllers (BOBGUI_WIDGET (tree_view), BOBGUI_PHASE_BUBBLE, &n_controllers);
  for (i = 0; i < n_controllers; i ++)
    {
      controller = controllers[i];
      if (BOBGUI_IS_SHORTCUT_CONTROLLER (controller))
        {
          g_object_ref (controller);
          bobgui_widget_remove_controller (BOBGUI_WIDGET (tree_view), controller);
          bobgui_widget_add_controller (BOBGUI_WIDGET (tree_view), controller);
          break;
        }
    }
  g_free (controllers);

  priv->click_gesture = bobgui_gesture_click_new ();
  bobgui_gesture_single_set_button (BOBGUI_GESTURE_SINGLE (priv->click_gesture), 0);
  g_signal_connect (priv->click_gesture, "pressed",
                    G_CALLBACK (bobgui_tree_view_click_gesture_pressed), tree_view);
  g_signal_connect (priv->click_gesture, "released",
                    G_CALLBACK (bobgui_tree_view_click_gesture_released), tree_view);
  bobgui_widget_add_controller (BOBGUI_WIDGET (tree_view), BOBGUI_EVENT_CONTROLLER (priv->click_gesture));

  gesture = bobgui_gesture_click_new ();
  g_signal_connect (gesture, "pressed",
                    G_CALLBACK (bobgui_tree_view_column_click_gesture_pressed), tree_view);
  bobgui_event_controller_set_propagation_phase (BOBGUI_EVENT_CONTROLLER (gesture),
                                              BOBGUI_PHASE_CAPTURE);
  bobgui_widget_add_controller (BOBGUI_WIDGET (tree_view), BOBGUI_EVENT_CONTROLLER (gesture));

  priv->drag_gesture = bobgui_gesture_drag_new ();
  g_signal_connect (priv->drag_gesture, "drag-begin",
                    G_CALLBACK (bobgui_tree_view_drag_gesture_begin), tree_view);
  g_signal_connect (priv->drag_gesture, "drag-update",
                    G_CALLBACK (bobgui_tree_view_drag_gesture_update), tree_view);
  g_signal_connect (priv->drag_gesture, "drag-end",
                    G_CALLBACK (bobgui_tree_view_drag_gesture_end), tree_view);
  bobgui_widget_add_controller (BOBGUI_WIDGET (tree_view), BOBGUI_EVENT_CONTROLLER (priv->drag_gesture));

  priv->column_drag_gesture = bobgui_gesture_drag_new ();
  g_signal_connect (priv->column_drag_gesture, "drag-begin",
                    G_CALLBACK (bobgui_tree_view_column_drag_gesture_begin), tree_view);
  g_signal_connect (priv->column_drag_gesture, "drag-update",
                    G_CALLBACK (bobgui_tree_view_column_drag_gesture_update), tree_view);
  g_signal_connect (priv->column_drag_gesture, "drag-end",
                    G_CALLBACK (bobgui_tree_view_column_drag_gesture_end), tree_view);
  bobgui_event_controller_set_propagation_phase (BOBGUI_EVENT_CONTROLLER (priv->column_drag_gesture),
                                              BOBGUI_PHASE_CAPTURE);
  bobgui_widget_add_controller (BOBGUI_WIDGET (tree_view), BOBGUI_EVENT_CONTROLLER (priv->column_drag_gesture));

  controller = bobgui_event_controller_motion_new ();
  g_signal_connect (controller, "enter",
                    G_CALLBACK (bobgui_tree_view_motion_controller_enter), tree_view);
  g_signal_connect (controller, "leave",
                    G_CALLBACK (bobgui_tree_view_motion_controller_leave), tree_view);
  g_signal_connect (controller, "motion",
                    G_CALLBACK (bobgui_tree_view_motion_controller_motion), tree_view);
  bobgui_widget_add_controller (BOBGUI_WIDGET (tree_view), controller);

  controller = bobgui_event_controller_key_new ();
  g_signal_connect (controller, "key-pressed",
                    G_CALLBACK (bobgui_tree_view_key_controller_key_pressed), tree_view);
  g_signal_connect (controller, "key-released",
                    G_CALLBACK (bobgui_tree_view_key_controller_key_released), tree_view);
  bobgui_widget_add_controller (BOBGUI_WIDGET (tree_view), controller);

  controller = bobgui_event_controller_focus_new ();
  g_signal_connect (controller, "leave",
                    G_CALLBACK (bobgui_tree_view_focus_controller_focus_out), tree_view);
  bobgui_widget_add_controller (BOBGUI_WIDGET (tree_view), controller);
}



/* GObject Methods
 */

static void
bobgui_tree_view_set_property (GObject         *object,
			    guint            prop_id,
			    const GValue    *value,
			    GParamSpec      *pspec)
{
  BobguiTreeView *tree_view = BOBGUI_TREE_VIEW (object);
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  switch (prop_id)
    {
    case PROP_MODEL:
      bobgui_tree_view_set_model (tree_view, g_value_get_object (value));
      break;
    case PROP_HADJUSTMENT:
      bobgui_tree_view_do_set_hadjustment (tree_view, g_value_get_object (value));
      break;
    case PROP_VADJUSTMENT:
      bobgui_tree_view_do_set_vadjustment (tree_view, g_value_get_object (value));
      break;
    case PROP_HSCROLL_POLICY:
      if (priv->hscroll_policy != g_value_get_enum (value))
        {
          priv->hscroll_policy = g_value_get_enum (value);
          bobgui_widget_queue_resize (BOBGUI_WIDGET (tree_view));
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    case PROP_VSCROLL_POLICY:
      if (priv->vscroll_policy != g_value_get_enum (value))
        {
          priv->vscroll_policy = g_value_get_enum (value);
          bobgui_widget_queue_resize (BOBGUI_WIDGET (tree_view));
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    case PROP_HEADERS_VISIBLE:
      bobgui_tree_view_set_headers_visible (tree_view, g_value_get_boolean (value));
      break;
    case PROP_HEADERS_CLICKABLE:
      bobgui_tree_view_set_headers_clickable (tree_view, g_value_get_boolean (value));
      break;
    case PROP_EXPANDER_COLUMN:
      bobgui_tree_view_set_expander_column (tree_view, g_value_get_object (value));
      break;
    case PROP_REORDERABLE:
      bobgui_tree_view_set_reorderable (tree_view, g_value_get_boolean (value));
      break;
    case PROP_ENABLE_SEARCH:
      bobgui_tree_view_set_enable_search (tree_view, g_value_get_boolean (value));
      break;
    case PROP_SEARCH_COLUMN:
      bobgui_tree_view_set_search_column (tree_view, g_value_get_int (value));
      break;
    case PROP_FIXED_HEIGHT_MODE:
      bobgui_tree_view_set_fixed_height_mode (tree_view, g_value_get_boolean (value));
      break;
    case PROP_HOVER_SELECTION:
      if (priv->hover_selection != g_value_get_boolean (value))
        {
          priv->hover_selection = g_value_get_boolean (value);
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    case PROP_HOVER_EXPAND:
      if (priv->hover_expand != g_value_get_boolean (value))
        {
          priv->hover_expand = g_value_get_boolean (value);
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    case PROP_SHOW_EXPANDERS:
      bobgui_tree_view_set_show_expanders (tree_view, g_value_get_boolean (value));
      break;
    case PROP_LEVEL_INDENTATION:
      if (priv->level_indentation != g_value_get_int (value))
        {
          priv->level_indentation = g_value_get_int (value);
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    case PROP_RUBBER_BANDING:
      if (priv->rubber_banding_enable != g_value_get_boolean (value))
        {
          priv->rubber_banding_enable = g_value_get_boolean (value);
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    case PROP_ENABLE_GRID_LINES:
      bobgui_tree_view_set_grid_lines (tree_view, g_value_get_enum (value));
      break;
    case PROP_ENABLE_TREE_LINES:
      bobgui_tree_view_set_enable_tree_lines (tree_view, g_value_get_boolean (value));
      break;
    case PROP_TOOLTIP_COLUMN:
      bobgui_tree_view_set_tooltip_column (tree_view, g_value_get_int (value));
      break;
    case PROP_ACTIVATE_ON_SINGLE_CLICK:
      bobgui_tree_view_set_activate_on_single_click (tree_view, g_value_get_boolean (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_tree_view_get_property (GObject    *object,
			    guint       prop_id,
			    GValue     *value,
			    GParamSpec *pspec)
{
  BobguiTreeView *tree_view = BOBGUI_TREE_VIEW (object);
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  switch (prop_id)
    {
    case PROP_MODEL:
      g_value_set_object (value, priv->model);
      break;
    case PROP_HADJUSTMENT:
      g_value_set_object (value, priv->hadjustment);
      break;
    case PROP_VADJUSTMENT:
      g_value_set_object (value, priv->vadjustment);
      break;
    case PROP_HSCROLL_POLICY:
      g_value_set_enum (value, priv->hscroll_policy);
      break;
    case PROP_VSCROLL_POLICY:
      g_value_set_enum (value, priv->vscroll_policy);
      break;
    case PROP_HEADERS_VISIBLE:
      g_value_set_boolean (value, bobgui_tree_view_get_headers_visible (tree_view));
      break;
    case PROP_HEADERS_CLICKABLE:
      g_value_set_boolean (value, bobgui_tree_view_get_headers_clickable (tree_view));
      break;
    case PROP_EXPANDER_COLUMN:
      g_value_set_object (value, priv->expander_column);
      break;
    case PROP_REORDERABLE:
      g_value_set_boolean (value, priv->reorderable);
      break;
    case PROP_ENABLE_SEARCH:
      g_value_set_boolean (value, priv->enable_search);
      break;
    case PROP_SEARCH_COLUMN:
      g_value_set_int (value, priv->search_column);
      break;
    case PROP_FIXED_HEIGHT_MODE:
      g_value_set_boolean (value, priv->fixed_height_mode);
      break;
    case PROP_HOVER_SELECTION:
      g_value_set_boolean (value, priv->hover_selection);
      break;
    case PROP_HOVER_EXPAND:
      g_value_set_boolean (value, priv->hover_expand);
      break;
    case PROP_SHOW_EXPANDERS:
      g_value_set_boolean (value, priv->show_expanders);
      break;
    case PROP_LEVEL_INDENTATION:
      g_value_set_int (value, priv->level_indentation);
      break;
    case PROP_RUBBER_BANDING:
      g_value_set_boolean (value, priv->rubber_banding_enable);
      break;
    case PROP_ENABLE_GRID_LINES:
      g_value_set_enum (value, priv->grid_lines);
      break;
    case PROP_ENABLE_TREE_LINES:
      g_value_set_boolean (value, priv->tree_lines_enabled);
      break;
    case PROP_TOOLTIP_COLUMN:
      g_value_set_int (value, priv->tooltip_column);
      break;
    case PROP_ACTIVATE_ON_SINGLE_CLICK:
      g_value_set_boolean (value, priv->activate_on_single_click);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_tree_view_finalize (GObject *object)
{
  G_OBJECT_CLASS (bobgui_tree_view_parent_class)->finalize (object);
}


static BobguiBuildableIface *parent_buildable_iface;

static void
bobgui_tree_view_buildable_init (BobguiBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);
  iface->add_child = bobgui_tree_view_buildable_add_child;
  iface->get_internal_child = bobgui_tree_view_buildable_get_internal_child;
}

static void
bobgui_tree_view_buildable_add_child (BobguiBuildable *tree_view,
				   BobguiBuilder  *builder,
				   GObject     *child,
				   const char *type)
{
  if (BOBGUI_IS_TREE_VIEW_COLUMN (child))
    bobgui_tree_view_append_column (BOBGUI_TREE_VIEW (tree_view), BOBGUI_TREE_VIEW_COLUMN (child));
  else
    parent_buildable_iface->add_child (tree_view, builder, child, type);
}

static GObject *
bobgui_tree_view_buildable_get_internal_child (BobguiBuildable      *buildable,
					    BobguiBuilder        *builder,
					    const char        *childname)
{
  BobguiTreeView *tree_view = BOBGUI_TREE_VIEW (buildable);
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  if (strcmp (childname, "selection") == 0)
    return G_OBJECT (priv->selection);

  return parent_buildable_iface->get_internal_child (buildable,
                                                     builder,
                                                     childname);
}

/* BobguiWidget Methods
 */

static void
bobgui_tree_view_free_rbtree (BobguiTreeView *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  bobgui_tree_rbtree_free (priv->tree);

  priv->tree = NULL;
  priv->button_pressed_node = NULL;
  priv->button_pressed_tree = NULL;
  priv->prelight_tree = NULL;
  priv->prelight_node = NULL;
}

static void
bobgui_tree_view_destroy_search_popover (BobguiTreeView *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  bobgui_widget_unparent (priv->search_popover);

  priv->search_popover = NULL;
  priv->search_entry = NULL;
  priv->search_entry_changed_id = 0;
}

static void
bobgui_tree_view_dispose (GObject *object)
{
  BobguiTreeView *tree_view = BOBGUI_TREE_VIEW (object);
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  GList *list;

  bobgui_tree_view_stop_editing (tree_view, TRUE);
  bobgui_tree_view_stop_rubber_band (tree_view);

  if (priv->columns != NULL)
    {
      list = priv->columns;
      while (list)
	{
	  BobguiTreeViewColumn *column;
	  column = BOBGUI_TREE_VIEW_COLUMN (list->data);
	  list = list->next;
	  bobgui_tree_view_remove_column (tree_view, column);
	}
      priv->columns = NULL;
    }

  if (priv->tree != NULL)
    {
      bobgui_tree_view_unref_and_check_selection_tree (tree_view, priv->tree);

      bobgui_tree_view_free_rbtree (tree_view);
    }

  if (priv->selection != NULL)
    {
      _bobgui_tree_selection_set_tree_view (priv->selection, NULL);
      g_object_unref (priv->selection);
      priv->selection = NULL;
    }

  g_clear_pointer (&priv->scroll_to_path, bobgui_tree_row_reference_free);
  g_clear_pointer (&priv->drag_dest_row, bobgui_tree_row_reference_free);
  g_clear_pointer (&priv->top_row, bobgui_tree_row_reference_free);

  if (priv->column_drop_func_data &&
      priv->column_drop_func_data_destroy)
    {
      priv->column_drop_func_data_destroy (priv->column_drop_func_data);
      priv->column_drop_func_data = NULL;
    }

  bobgui_tree_row_reference_free (priv->anchor);
  priv->anchor = NULL;

  /* destroy interactive search dialog */
  if (priv->search_popover)
    {
      bobgui_tree_view_destroy_search_popover (tree_view);
      if (priv->typeselect_flush_timeout)
	{
	  g_source_remove (priv->typeselect_flush_timeout);
	  priv->typeselect_flush_timeout = 0;
	}
    }

  if (priv->search_custom_entry_set)
    {
      BobguiEventController *controller;

      g_signal_handlers_disconnect_by_func (priv->search_entry,
                                            G_CALLBACK (bobgui_tree_view_search_init),
                                            tree_view);

      if (BOBGUI_IS_ENTRY (priv->search_entry))
        controller = bobgui_entry_get_key_controller (BOBGUI_ENTRY (priv->search_entry));
      else
        controller = bobgui_search_entry_get_key_controller (BOBGUI_SEARCH_ENTRY (priv->search_entry));
      g_signal_handlers_disconnect_by_func (controller,
                                            G_CALLBACK (bobgui_tree_view_search_key_pressed),
                                            tree_view);

      g_object_unref (priv->search_entry);

      priv->search_entry = NULL;
      priv->search_custom_entry_set = FALSE;
    }

  if (priv->search_destroy && priv->search_user_data)
    {
      priv->search_destroy (priv->search_user_data);
      priv->search_user_data = NULL;
    }

  if (priv->search_position_destroy && priv->search_position_user_data)
    {
      priv->search_position_destroy (priv->search_position_user_data);
      priv->search_position_user_data = NULL;
    }

  if (priv->row_separator_destroy && priv->row_separator_data)
    {
      priv->row_separator_destroy (priv->row_separator_data);
      priv->row_separator_data = NULL;
    }

  bobgui_tree_view_set_model (tree_view, NULL);

  g_clear_object (&priv->hadjustment);
  g_clear_object (&priv->vadjustment);
  g_clear_object (&priv->horizontal_grid_line_texture);
  g_clear_object (&priv->vertical_grid_line_texture);
  g_clear_object (&priv->horizontal_tree_line_texture);
  g_clear_object (&priv->vertical_tree_line_texture);

  G_OBJECT_CLASS (bobgui_tree_view_parent_class)->dispose (object);
}

/* BobguiWidget::map helper */
static void
bobgui_tree_view_map_buttons (BobguiTreeView *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  GList *list;

  g_return_if_fail (bobgui_widget_get_mapped (BOBGUI_WIDGET (tree_view)));

  if (priv->headers_visible)
    {
      BobguiTreeViewColumn *column;
      BobguiWidget         *button;

      for (list = priv->columns; list; list = list->next)
	{
	  column = list->data;
	  button = bobgui_tree_view_column_get_button (column);

          if (bobgui_tree_view_column_get_visible (column) && button)
            bobgui_widget_show (button);

          if (bobgui_widget_get_visible (button) &&
              !bobgui_widget_get_mapped (button))
            bobgui_widget_map (button);
	}
    }
}

static void
bobgui_tree_view_map (BobguiWidget *widget)
{
  BobguiTreeView *tree_view = BOBGUI_TREE_VIEW (widget);
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  GList *tmp_list;

  BOBGUI_WIDGET_CLASS (bobgui_tree_view_parent_class)->map (widget);

  tmp_list = priv->children;
  while (tmp_list)
    {
      BobguiTreeViewChild *child = tmp_list->data;
      tmp_list = tmp_list->next;

      if (bobgui_widget_get_visible (child->widget))
	{
	  if (!bobgui_widget_get_mapped (child->widget))
	    bobgui_widget_map (child->widget);
	}
    }

  bobgui_tree_view_map_buttons (tree_view);
}

static void
bobgui_tree_view_realize (BobguiWidget *widget)
{
  BobguiTreeView *tree_view = BOBGUI_TREE_VIEW (widget);
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  GList *tmp_list;

  BOBGUI_WIDGET_CLASS (bobgui_tree_view_parent_class)->realize (widget);

  for (tmp_list = priv->columns; tmp_list; tmp_list = tmp_list->next)
    _bobgui_tree_view_column_realize_button (BOBGUI_TREE_VIEW_COLUMN (tmp_list->data));

  /* Need to call those here, since they create GCs */
  bobgui_tree_view_set_grid_lines (tree_view, priv->grid_lines);
  bobgui_tree_view_set_enable_tree_lines (tree_view, priv->tree_lines_enabled);

  install_presize_handler (tree_view);
}

static void
bobgui_tree_view_unrealize (BobguiWidget *widget)
{
  BobguiTreeView *tree_view = BOBGUI_TREE_VIEW (widget);
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  if (priv->scroll_timeout != 0)
    {
      g_source_remove (priv->scroll_timeout);
      priv->scroll_timeout = 0;
    }

  if (priv->auto_expand_timeout != 0)
    {
      g_source_remove (priv->auto_expand_timeout);
      priv->auto_expand_timeout = 0;
    }

  if (priv->open_dest_timeout != 0)
    {
      g_source_remove (priv->open_dest_timeout);
      priv->open_dest_timeout = 0;
    }

  if (priv->presize_handler_tick_cb != 0)
    {
      bobgui_widget_remove_tick_callback (widget, priv->presize_handler_tick_cb);
      priv->presize_handler_tick_cb = 0;
    }

  if (priv->validate_rows_timer != 0)
    {
      g_source_remove (priv->validate_rows_timer);
      priv->validate_rows_timer = 0;
    }

  if (priv->scroll_sync_timer != 0)
    {
      g_source_remove (priv->scroll_sync_timer);
      priv->scroll_sync_timer = 0;
    }

  if (priv->typeselect_flush_timeout)
    {
      g_source_remove (priv->typeselect_flush_timeout);
      priv->typeselect_flush_timeout = 0;
    }

  BOBGUI_WIDGET_CLASS (bobgui_tree_view_parent_class)->unrealize (widget);
}

static void
bobgui_tree_view_unroot (BobguiWidget *widget)
{
  BobguiTreeView *tree_view = BOBGUI_TREE_VIEW (widget);
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  /* break ref cycles */
  g_clear_pointer (&priv->scroll_to_path, bobgui_tree_row_reference_free);
  g_clear_pointer (&priv->drag_dest_row, bobgui_tree_row_reference_free);
  g_clear_pointer (&priv->top_row, bobgui_tree_row_reference_free);

  BOBGUI_WIDGET_CLASS (bobgui_tree_view_parent_class)->unroot (widget);
}

/* BobguiWidget::get_preferred_height helper */
static void
bobgui_tree_view_update_height (BobguiTreeView *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  GList *list;

  priv->header_height = 0;

  for (list = priv->columns; list; list = list->next)
    {
      BobguiRequisition     requisition;
      BobguiTreeViewColumn *column = list->data;
      BobguiWidget         *button = bobgui_tree_view_column_get_button (column);

      if (button == NULL)
        continue;

      bobgui_widget_get_preferred_size (button, &requisition, NULL);
      priv->header_height = MAX (priv->header_height, requisition.height);
    }
}

static int
bobgui_tree_view_get_height (BobguiTreeView *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  if (priv->tree == NULL)
    return 0;
  else
    return priv->tree->root->offset;
}

static void
bobgui_tree_view_measure (BobguiWidget        *widget,
                       BobguiOrientation  orientation,
                       int             for_size,
                       int            *minimum,
                       int            *natural,
                       int            *minimum_baseline,
                       int            *natural_baseline)
{
  BobguiTreeView *tree_view = BOBGUI_TREE_VIEW (widget);
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  if (orientation == BOBGUI_ORIENTATION_HORIZONTAL)
    {
      GList *list;
      BobguiTreeViewColumn *column;
      int width = 0;

      /* we validate some rows initially just to make sure we have some size.
       * In practice, with a lot of static lists, this should get a good width.
       */
      do_validate_rows (tree_view, FALSE);

      /* keep this in sync with size_allocate below */
      for (list = priv->columns; list; list = list->next)
        {
          column = list->data;
          if (!bobgui_tree_view_column_get_visible (column) || column == priv->drag_column)
            continue;

          width += _bobgui_tree_view_column_request_width (column);
        }

      *minimum = *natural = width;
    }
  else /* VERTICAL */
    {
      int height;

      bobgui_tree_view_update_height (tree_view);
      height = bobgui_tree_view_get_height (tree_view) + bobgui_tree_view_get_effective_header_height (tree_view);

      *minimum = *natural = height;
    }
}

static int
bobgui_tree_view_calculate_width_before_expander (BobguiTreeView *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  int width = 0;
  GList *list;
  gboolean rtl;

  rtl = (_bobgui_widget_get_direction (BOBGUI_WIDGET (tree_view)) == BOBGUI_TEXT_DIR_RTL);
  for (list = (rtl ? g_list_last (priv->columns) : g_list_first (priv->columns));
       list->data != priv->expander_column;
       list = (rtl ? list->prev : list->next))
    {
      BobguiTreeViewColumn *column = list->data;

      width += bobgui_tree_view_column_get_width (column);
    }

  return width;
}

/* BobguiWidget::size_allocate helper */
static void
bobgui_tree_view_size_allocate_columns (BobguiWidget *widget)
{
  BobguiTreeView *tree_view = BOBGUI_TREE_VIEW (widget);
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  const int x_offset = - bobgui_adjustment_get_value (priv->hadjustment);
  GList *list, *first_column, *last_column;
  BobguiTreeViewColumn *column;
  int widget_width, width = 0;
  int extra, extra_per_column;
  int full_requested_width = 0;
  int number_of_expand_columns = 0;
  gboolean rtl;

  for (last_column = g_list_last (priv->columns);
       last_column &&
       !(bobgui_tree_view_column_get_visible (BOBGUI_TREE_VIEW_COLUMN (last_column->data)));
       last_column = last_column->prev)
    ;
  if (last_column == NULL)
    return;

  for (first_column = g_list_first (priv->columns);
       first_column &&
       !(bobgui_tree_view_column_get_visible (BOBGUI_TREE_VIEW_COLUMN (first_column->data)));
       first_column = first_column->next)
    ;

  if (first_column == NULL)
    return;

  rtl = (_bobgui_widget_get_direction (widget) == BOBGUI_TEXT_DIR_RTL);

  /* find out how many extra space and expandable columns we have */
  for (list = priv->columns; list != last_column->next; list = list->next)
    {
      column = (BobguiTreeViewColumn *)list->data;

      if (!bobgui_tree_view_column_get_visible (column) || column == priv->drag_column)
	continue;

      full_requested_width += _bobgui_tree_view_column_request_width (column);

      if (bobgui_tree_view_column_get_expand (column))
	number_of_expand_columns++;
    }

  widget_width = bobgui_widget_get_width (widget);
  extra = MAX (widget_width - full_requested_width, 0);

  if (number_of_expand_columns > 0)
    extra_per_column = extra/number_of_expand_columns;
  else
    extra_per_column = 0;

  for (list = first_column;
       list != last_column->next;
       list = list->next)
    {
      int column_width;

      column = list->data;
      column_width = _bobgui_tree_view_column_request_width (column);

      if (!bobgui_tree_view_column_get_visible (column))
	continue;

      if (column == priv->drag_column)
        goto next;

      if (bobgui_tree_view_column_get_expand (column))
	{
	  if (number_of_expand_columns == 1)
	    {
	      /* We add the remainder to the last column as
	       * */
	      column_width += extra;
	    }
	  else
	    {
	      column_width += extra_per_column;
	      extra -= extra_per_column;
	      number_of_expand_columns --;
	    }
	}
      else if (number_of_expand_columns == 0 &&
	       list == last_column)
	{
	  column_width += extra;
	}

      if (rtl)
        _bobgui_tree_view_column_allocate (column, widget_width - width - column_width + x_offset,
                                        column_width, priv->header_height);
      else
        _bobgui_tree_view_column_allocate (column, width + x_offset,
                                        column_width, priv->header_height);
  next:
      width += column_width;
    }

  /* We change the width here.  The user might have been resizing columns,
   * which changes the total width of the tree view.  This is of
   * importance for getting the horizontal scroll bar right.
   */
  priv->width = width;
}

/* BobguiWidget::size_allocate helper */
static void
bobgui_tree_view_size_allocate_drag_column (BobguiWidget *widget)
{
  BobguiTreeView *tree_view = BOBGUI_TREE_VIEW (widget);
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  BobguiAllocation allocation;
  int baseline;
  BobguiWidget *button;

  if (priv->drag_column == NULL)
    return;

  button = bobgui_tree_view_column_get_button (priv->drag_column);

  allocation.x = priv->drag_column_x;
  allocation.y = priv->drag_column_y;
  allocation.width = bobgui_widget_get_allocated_width (button);
  allocation.height = bobgui_widget_get_allocated_height (button);
  baseline = bobgui_widget_get_allocated_baseline (button);

  bobgui_widget_size_allocate (button, &allocation, baseline);
}

static void
bobgui_tree_view_size_allocate (BobguiWidget *widget,
                             int        width,
                             int        height,
                             int        baseline)
{
  BobguiTreeView *tree_view = BOBGUI_TREE_VIEW (widget);
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  GList *tmp_list;
  double page_size;

  /* We allocate the columns first because the width of the
   * tree view (used in updating the adjustments below) might change.
   */
  bobgui_tree_view_size_allocate_columns (widget);
  bobgui_tree_view_size_allocate_drag_column (widget);

  page_size = bobgui_adjustment_get_page_size (priv->vadjustment);
  bobgui_adjustment_configure (priv->hadjustment,
                            bobgui_adjustment_get_value (priv->hadjustment) +
                            (_bobgui_widget_get_direction(widget) == BOBGUI_TEXT_DIR_RTL ? width - page_size : 0),
                            0,
                            MAX (width, priv->width),
                            width * 0.1,
                            width * 0.9,
                            width);

  page_size = height - bobgui_tree_view_get_effective_header_height (tree_view);
  bobgui_adjustment_configure (priv->vadjustment,
                            bobgui_adjustment_get_value (priv->vadjustment),
                            0,
                            MAX (page_size, bobgui_tree_view_get_height (tree_view)),
                            page_size * 0.1,
                            page_size * 0.9,
                            page_size);

  /* now the adjustments and window sizes are in sync, we can sync toprow/dy again */
  if (bobgui_tree_row_reference_valid (priv->top_row))
    bobgui_tree_view_top_row_to_dy (tree_view);
  else
    bobgui_tree_view_dy_to_top_row (tree_view);

  if (bobgui_widget_get_realized (widget))
    {
      if (priv->tree == NULL)
        invalidate_empty_focus (tree_view);

      if (priv->expander_column)
        {
          /* Might seem awkward, but is the best heuristic I could come up
           * with.  Only if the width of the columns before the expander
           * changes, we will update the prelight status.  It is this
           * width that makes the expander move vertically.  Always updating
           * prelight status causes trouble with hover selections.
           */
          int width_before_expander;

          width_before_expander = bobgui_tree_view_calculate_width_before_expander (tree_view);

          if (priv->prev_width_before_expander
              != width_before_expander)
              update_prelight (tree_view,
                               priv->event_last_x,
                               priv->event_last_y);

          priv->prev_width_before_expander = width_before_expander;
        }
    }

  for (tmp_list = priv->children; tmp_list; tmp_list = tmp_list->next)
    {
      BobguiTreeViewChild *child = tmp_list->data;
      BobguiTreePath *path;
      GdkRectangle child_rect;
      int min_x, max_x, min_y, max_y;
      int size;
      BobguiTextDirection direction;

      direction = _bobgui_widget_get_direction (child->widget);
      path = _bobgui_tree_path_new_from_rbtree (child->tree, child->node);
      bobgui_tree_view_get_cell_area (tree_view, path, child->column, &child_rect);
      child_rect.x += child->border.left;
      child_rect.y += child->border.top;
      child_rect.width -= child->border.left + child->border.right;
      child_rect.height -= child->border.top + child->border.bottom;

      bobgui_widget_measure (BOBGUI_WIDGET (child->widget), BOBGUI_ORIENTATION_HORIZONTAL, -1,
                          &size, NULL, NULL, NULL);

      if (size > child_rect.width)
        {
          /* Enlarge the child, extending it to the left (RTL) */
          if (direction == BOBGUI_TEXT_DIR_RTL)
            child_rect.x -= (size - child_rect.width);
          /* or to the right (LTR) */
          else
            child_rect.x += 0;

          child_rect.width = size;
        }

      bobgui_widget_measure (BOBGUI_WIDGET (child->widget), BOBGUI_ORIENTATION_VERTICAL,
                          child_rect.width,
                          &size, NULL,
                          NULL, NULL);
      if (size > child_rect.height)
        {
          /* Enlarge the child, extending in both directions equally */
          child_rect.y -= (size - child_rect.height) / 2;
          child_rect.height = size;
        }

      /* push the rect back in the visible area if needed,
       * preferring the top left corner (for RTL)
       * or top right corner (for LTR)
       */
      min_x = 0;
      max_x = min_x + width - child_rect.width;
      min_y = 0;
      max_y = min_y + height - bobgui_tree_view_get_effective_header_height (tree_view) - child_rect.height;

      if (direction == BOBGUI_TEXT_DIR_LTR)
        /* Ensure that child's right edge is not sticking to the right
         * (if (child_rect.x > max_x) child_rect.x = max_x),
         * then ensure that child's left edge is visible and is not sticking to the left
         * (if (child_rect.x < min_x) child_rect.x = min_x).
         */
        child_rect.x = MAX (min_x, MIN (max_x, child_rect.x));
      else
        /* Ensure that child's left edge is not sticking to the left
         * (if (child_rect.x < min_x) child_rect.x = min_x),
         * then ensure that child's right edge is visible and is not sticking to the right
         * (if (child_rect.x > max_x) child_rect.x = max_x).
         */
        child_rect.x = MIN (max_x, MAX (min_x, child_rect.x));

      child_rect.y = MAX (min_y, MIN (max_y, child_rect.y));

      bobgui_tree_path_free (path);
      bobgui_widget_size_allocate (child->widget, &child_rect, -1);
    }

  if (priv->search_popover)
    bobgui_popover_present (BOBGUI_POPOVER (priv->search_popover));
}

/* Grabs the focus and unsets the BOBGUI_TREE_VIEW_DRAW_KEYFOCUS flag */
static void
grab_focus_and_unset_draw_keyfocus (BobguiTreeView *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  BobguiWidget *widget = BOBGUI_WIDGET (tree_view);

  if (bobgui_widget_get_focusable (widget) &&
      !bobgui_widget_has_focus (widget))
    bobgui_widget_grab_focus (widget);

  priv->draw_keyfocus = 0;
}

static inline gboolean
row_is_separator (BobguiTreeView *tree_view,
		  BobguiTreeIter *iter,
		  BobguiTreePath *path)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  gboolean is_separator = FALSE;

  if (priv->row_separator_func)
    {
      BobguiTreeIter tmpiter;

      if (iter)
        tmpiter = *iter;
      else
        {
          if (!bobgui_tree_model_get_iter (priv->model, &tmpiter, path))
            return FALSE;
        }

      is_separator = priv->row_separator_func (priv->model,
                                               &tmpiter,
                                               priv->row_separator_data);
    }

  return is_separator;
}

static int
bobgui_tree_view_get_expander_size (BobguiTreeView *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  BobguiStyleContext *context;
  BobguiCssStyle *style;
  int min_width;
  int min_height;
  int expander_size;

  if (priv->expander_size != -1)
    return priv->expander_size;

  context = bobgui_widget_get_style_context (BOBGUI_WIDGET (tree_view));
  bobgui_style_context_save (context);
  bobgui_style_context_add_class (context, "expander");

  style = bobgui_style_context_lookup_style (context);
  min_width = bobgui_css_number_value_get (style->size->min_width, 100);
  min_height = bobgui_css_number_value_get (style->size->min_height, 100);

  bobgui_style_context_restore (context);

  expander_size = MAX (min_width, min_height);

  priv->expander_size = expander_size + (_TREE_VIEW_HORIZONTAL_SEPARATOR / 2);

  return priv->expander_size;
}

static void
get_current_selection_modifiers (BobguiEventController *controller,
                                 gboolean           *modify,
                                 gboolean           *extend)
{
  GdkModifierType state;

  state = bobgui_event_controller_get_current_event_state (controller);
  *modify = (state & GDK_CONTROL_MASK) != 0;
#ifdef __APPLE__
  *modify = *modify | ((state & GDK_META_MASK) != 0);
#endif
  *extend = (state & GDK_SHIFT_MASK) != 0;
}

static void
bobgui_tree_view_click_gesture_pressed (BobguiGestureClick *gesture,
                                     int              n_press,
                                     double           x,
                                     double           y,
                                     BobguiTreeView     *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  BobguiWidget *widget = BOBGUI_WIDGET (tree_view);
  GdkRectangle background_area, cell_area;
  BobguiTreeViewColumn *column = NULL;
  GdkEventSequence *sequence;
  GdkModifierType modifiers;
  GdkEvent *event;
  int new_y, y_offset;
  int bin_x, bin_y;
  BobguiTreePath *path;
  BobguiTreeRBNode *node;
  BobguiTreeRBTree *tree;
  int depth;
  guint button;
  GList *list;
  gboolean rtl;
  BobguiWidget *target;

  bobgui_tree_view_convert_widget_to_bin_window_coords (tree_view, x, y,
                                                     &bin_x, &bin_y);

  /* Are we clicking a column header? */
  if (bin_y < 0)
    return;

  /* check if this is a click in a child widget */
  target = bobgui_event_controller_get_target (BOBGUI_EVENT_CONTROLLER (gesture));
  if (bobgui_widget_is_ancestor (target, widget))
    return;

  bobgui_tree_view_stop_editing (tree_view, FALSE);
  button = bobgui_gesture_single_get_current_button (BOBGUI_GESTURE_SINGLE (gesture));
  sequence = bobgui_gesture_single_get_current_sequence (BOBGUI_GESTURE_SINGLE (gesture));

  if (button > 3)
    {
      bobgui_gesture_set_state (BOBGUI_GESTURE (gesture), BOBGUI_EVENT_SEQUENCE_DENIED);
      return;
    }

  if (n_press > 1)
    bobgui_gesture_set_state (priv->drag_gesture,
                           BOBGUI_EVENT_SEQUENCE_DENIED);

  /* Empty tree? */
  if (priv->tree == NULL)
    {
      grab_focus_and_unset_draw_keyfocus (tree_view);
      return;
    }

  if (sequence)
    update_prelight (tree_view, x, y);

  /* are we in an arrow? */
  if (priv->prelight_node &&
      priv->arrow_prelit &&
      bobgui_tree_view_draw_expanders (tree_view))
    {
      if (button == GDK_BUTTON_PRIMARY)
        {
          priv->button_pressed_node = priv->prelight_node;
          priv->button_pressed_tree = priv->prelight_tree;
          bobgui_widget_queue_draw (widget);
        }

      grab_focus_and_unset_draw_keyfocus (tree_view);
      bobgui_gesture_set_state (BOBGUI_GESTURE (gesture), BOBGUI_EVENT_SEQUENCE_CLAIMED);
      return;
    }

  /* find the node that was clicked */
  new_y = TREE_WINDOW_Y_TO_RBTREE_Y(priv, bin_y);
  if (new_y < 0)
    new_y = 0;
  y_offset = -bobgui_tree_rbtree_find_offset (priv->tree, new_y, &tree, &node);

  if (node == NULL)
    {
      /* We clicked in dead space */
      grab_focus_and_unset_draw_keyfocus (tree_view);
      return;
    }

  /* Get the path and the node */
  path = _bobgui_tree_path_new_from_rbtree (tree, node);

  if (row_is_separator (tree_view, NULL, path))
    {
      bobgui_tree_path_free (path);
      grab_focus_and_unset_draw_keyfocus (tree_view);
      return;
    }

  depth = bobgui_tree_path_get_depth (path);
  background_area.y = y_offset + bin_y;
  background_area.height = bobgui_tree_view_get_row_height (tree_view, node);
  background_area.x = 0;

  bobgui_tree_view_convert_bin_window_to_widget_coords (tree_view,
                                                     background_area.x,
                                                     background_area.y,
                                                     &background_area.x,
                                                     &background_area.y);

  /* Let the column have a chance at selecting it. */
  rtl = (_bobgui_widget_get_direction (widget) == BOBGUI_TEXT_DIR_RTL);
  for (list = (rtl ? g_list_last (priv->columns) : g_list_first (priv->columns));
       list; list = (rtl ? list->prev : list->next))
    {
      BobguiTreeViewColumn *candidate = list->data;

      if (!bobgui_tree_view_column_get_visible (candidate))
        continue;

      background_area.width = bobgui_tree_view_column_get_width (candidate);
      if ((background_area.x > x) ||
          (background_area.x + background_area.width <= x))
        {
          background_area.x += background_area.width;
          continue;
        }

      /* we found the focus column */
      column = candidate;
      cell_area = background_area;
      cell_area.width -= _TREE_VIEW_HORIZONTAL_SEPARATOR;
      cell_area.x += _TREE_VIEW_HORIZONTAL_SEPARATOR / 2;
      if (bobgui_tree_view_is_expander_column (tree_view, column))
        {
          if (!rtl)
            cell_area.x += (depth - 1) * priv->level_indentation;
          cell_area.width -= (depth - 1) * priv->level_indentation;

          if (bobgui_tree_view_draw_expanders (tree_view))
            {
              int expander_size = bobgui_tree_view_get_expander_size (tree_view);
              if (!rtl)
                cell_area.x += depth * expander_size;
              cell_area.width -= depth * expander_size;
            }
        }
      break;
    }

  if (column == NULL)
    {
      bobgui_tree_path_free (path);
      grab_focus_and_unset_draw_keyfocus (tree_view);
      bobgui_gesture_set_state (BOBGUI_GESTURE (gesture), BOBGUI_EVENT_SEQUENCE_DENIED);
      return;
    }

  _bobgui_tree_view_set_focus_column (tree_view, column);

  event = bobgui_gesture_get_last_event (BOBGUI_GESTURE (gesture), sequence);
  modifiers = gdk_event_get_modifier_state (event);

  /* decide if we edit */
  if (button == GDK_BUTTON_PRIMARY &&
      !(modifiers & bobgui_accelerator_get_default_mod_mask ()))
    {
      BobguiTreePath *anchor;
      BobguiTreeIter iter;

      bobgui_tree_model_get_iter (priv->model, &iter, path);
      bobgui_tree_view_column_cell_set_cell_data (column,
                                               priv->model,
                                               &iter,
                                               BOBGUI_TREE_RBNODE_FLAG_SET (node, BOBGUI_TREE_RBNODE_IS_PARENT),
                                               node->children?TRUE:FALSE);

      if (priv->anchor)
        anchor = bobgui_tree_row_reference_get_path (priv->anchor);
      else
        anchor = NULL;

      if ((anchor && !bobgui_tree_path_compare (anchor, path))
          || !_bobgui_tree_view_column_has_editable_cell (column))
        {
          BobguiCellEditable *cell_editable = NULL;

          /* FIXME: get the right flags */
          guint flags = 0;

          if (_bobgui_tree_view_column_cell_event (column,
                                                (GdkEvent *)event,
                                                &cell_area, flags))
            {
              BobguiCellArea *area = bobgui_cell_layout_get_area (BOBGUI_CELL_LAYOUT (column));
              cell_editable = bobgui_cell_area_get_edit_widget (area);
              bobgui_gesture_set_state (BOBGUI_GESTURE (gesture), BOBGUI_EVENT_SEQUENCE_CLAIMED);

              if (cell_editable != NULL)
                {
                  bobgui_tree_path_free (path);
                  bobgui_tree_path_free (anchor);
                  return;
                }
            }
        }
      if (anchor)
        bobgui_tree_path_free (anchor);
    }

  /* we only handle selection modifications on the first button press
   */
  if (n_press == 1)
    {
      BobguiCellRenderer *focus_cell;
      gboolean modify, extend;

      get_current_selection_modifiers (BOBGUI_EVENT_CONTROLLER (gesture), &modify, &extend);
      priv->modify_selection_pressed = modify;
      priv->extend_selection_pressed = extend;

      /* We update the focus cell here, this is also needed if the
       * column does not contain an editable cell.  In this case,
       * BobguiCellArea did not receive the event for processing (and
       * could not update the focus cell).
       */
      focus_cell = _bobgui_tree_view_column_get_cell_at_pos (column,
                                                          &cell_area,
                                                          &background_area,
                                                          x, y);

      if (focus_cell)
        bobgui_tree_view_column_focus_cell (column, focus_cell);

      if (modify)
        {
          bobgui_tree_view_real_set_cursor (tree_view, path, CLAMP_NODE);
          bobgui_tree_view_real_toggle_cursor_row (tree_view);
        }
      else if (extend)
        {
          bobgui_tree_view_real_set_cursor (tree_view, path, CLAMP_NODE);
          bobgui_tree_view_real_select_cursor_row (tree_view, FALSE);
        }
      else
        {
          bobgui_tree_view_real_set_cursor (tree_view, path, CLEAR_AND_SELECT | CLAMP_NODE);
        }

      priv->modify_selection_pressed = FALSE;
      priv->extend_selection_pressed = FALSE;
    }

  if (button == GDK_BUTTON_PRIMARY && n_press == 2)
    {
      bobgui_tree_view_row_activated (tree_view, path, column);
      bobgui_gesture_set_state (BOBGUI_GESTURE (gesture), BOBGUI_EVENT_SEQUENCE_CLAIMED);
    }
  else
    {
      if (n_press == 1)
        {
          priv->button_pressed_node = priv->prelight_node;
          priv->button_pressed_tree = priv->prelight_tree;
        }

      grab_focus_and_unset_draw_keyfocus (tree_view);
    }

  bobgui_tree_path_free (path);

  if (n_press >= 2)
    bobgui_event_controller_reset (BOBGUI_EVENT_CONTROLLER (gesture));
}

static void
bobgui_tree_view_drag_gesture_begin (BobguiGestureDrag *gesture,
                                  double          start_x,
                                  double          start_y,
                                  BobguiTreeView    *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  int bin_x, bin_y;
  BobguiTreeRBTree *tree;
  BobguiTreeRBNode *node;

  if (priv->tree == NULL)
    {
      bobgui_gesture_set_state (BOBGUI_GESTURE (gesture), BOBGUI_EVENT_SEQUENCE_DENIED);
      return;
    }

  bobgui_tree_view_convert_widget_to_bin_window_coords (tree_view, start_x, start_y,
                                                     &bin_x, &bin_y);

  /* Are we dragging a column header? */
  if (bin_y < 0)
    return;

  priv->press_start_x = priv->rubber_band_x = bin_x;
  priv->press_start_y = priv->rubber_band_y = bin_y;
  bobgui_tree_rbtree_find_offset (priv->tree, bin_y + priv->dy,
                           &tree, &node);

  if (priv->rubber_banding_enable
      && !BOBGUI_TREE_RBNODE_FLAG_SET (node, BOBGUI_TREE_RBNODE_IS_SELECTED)
      && bobgui_tree_selection_get_mode (priv->selection) == BOBGUI_SELECTION_MULTIPLE)
    {
      gboolean modify, extend;

      priv->press_start_y += priv->dy;
      priv->rubber_band_y += priv->dy;
      priv->rubber_band_status = RUBBER_BAND_MAYBE_START;

      get_current_selection_modifiers (BOBGUI_EVENT_CONTROLLER (gesture), &modify, &extend);
      priv->rubber_band_modify = modify;
      priv->rubber_band_extend = extend;
    }
}

static void
bobgui_tree_view_column_click_gesture_pressed (BobguiGestureClick *gesture,
                                            int              n_press,
                                            double           x,
                                            double           y,
                                            BobguiTreeView     *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  BobguiTreeViewColumn *column;
  GList *list;

  if (n_press != 2)
    return;

  for (list = priv->columns; list; list = list->next)
    {
      column = list->data;

      if (!_bobgui_tree_view_column_coords_in_resize_rect (column, x, y) ||
          !bobgui_tree_view_column_get_resizable (column))
        continue;

      if (bobgui_tree_view_column_get_sizing (column) != BOBGUI_TREE_VIEW_COLUMN_AUTOSIZE)
        {
          bobgui_tree_view_column_set_fixed_width (column, -1);
          bobgui_tree_view_column_set_expand (column, FALSE);
          _bobgui_tree_view_column_autosize (tree_view, column);
        }

      bobgui_gesture_set_state (BOBGUI_GESTURE (gesture), BOBGUI_EVENT_SEQUENCE_CLAIMED);
      break;
    }
}

static void
bobgui_tree_view_column_drag_gesture_begin (BobguiGestureDrag *gesture,
                                         double          start_x,
                                         double          start_y,
                                         BobguiTreeView    *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  BobguiTreeViewColumn *column;
  gboolean rtl;
  GList *list;
  int i;

  rtl = (bobgui_widget_get_direction (BOBGUI_WIDGET (tree_view)) == BOBGUI_TEXT_DIR_RTL);

  for (i = 0, list = priv->columns; list; list = list->next, i++)
    {
      gpointer drag_data;
      int column_width;

      column = list->data;

      if (!_bobgui_tree_view_column_coords_in_resize_rect (column, start_x, start_y))
        continue;

      if (!bobgui_tree_view_column_get_resizable (column))
        break;

      priv->in_column_resize = TRUE;

      /* block attached dnd signal handler */
      drag_data = g_object_get_data (G_OBJECT (tree_view), "bobgui-site-data");
      if (drag_data)
        g_signal_handlers_block_matched (tree_view,
                                         G_SIGNAL_MATCH_DATA,
                                         0, 0, NULL, NULL,
                                         drag_data);

      column_width = bobgui_tree_view_column_get_width (column);
      bobgui_tree_view_column_set_fixed_width (column, column_width);
      bobgui_tree_view_column_set_expand (column, FALSE);

      priv->drag_pos = i;
      priv->x_drag = start_x + (rtl ? column_width : -column_width);

      if (!bobgui_widget_has_focus (BOBGUI_WIDGET (tree_view)))
        bobgui_widget_grab_focus (BOBGUI_WIDGET (tree_view));

      bobgui_gesture_set_state (BOBGUI_GESTURE (gesture), BOBGUI_EVENT_SEQUENCE_CLAIMED);
      return;
    }
}

static void
bobgui_tree_view_update_button_position (BobguiTreeView       *tree_view,
                                      BobguiTreeViewColumn *column)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  GList *column_el;

  column_el = g_list_find (priv->columns, column);
  g_return_if_fail (column_el != NULL);

  bobgui_css_node_insert_after (priv->header_node,
                             bobgui_widget_get_css_node (bobgui_tree_view_column_get_button (column)),
                             column_el->prev ? bobgui_widget_get_css_node (
                                bobgui_tree_view_column_get_button (column_el->prev->data)) : NULL);
}

/* column drag gesture helper */
static gboolean
bobgui_tree_view_button_release_drag_column (BobguiTreeView *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  BobguiWidget *button, *widget = BOBGUI_WIDGET (tree_view);
  GList *l;
  gboolean rtl;
  BobguiStyleContext *context;

  rtl = (bobgui_widget_get_direction (widget) == BOBGUI_TEXT_DIR_RTL);

  /* Move the button back */
  button = bobgui_tree_view_column_get_button (priv->drag_column);

  context = bobgui_widget_get_style_context (button);
  bobgui_style_context_remove_class (context, "dnd");

  bobgui_tree_view_update_button_position (tree_view, priv->drag_column);
  bobgui_widget_queue_allocate (widget);

  bobgui_widget_grab_focus (button);

  if (rtl)
    {
      if (priv->cur_reorder &&
          priv->cur_reorder->right_column != priv->drag_column)
        bobgui_tree_view_move_column_after (tree_view, priv->drag_column,
                                         priv->cur_reorder->right_column);
    }
  else
    {
      if (priv->cur_reorder &&
          priv->cur_reorder->left_column != priv->drag_column)
        bobgui_tree_view_move_column_after (tree_view, priv->drag_column,
                                         priv->cur_reorder->left_column);
    }
  priv->drag_column = NULL;

  for (l = priv->column_drag_info; l != NULL; l = l->next)
    g_slice_free (BobguiTreeViewColumnReorder, l->data);
  g_list_free (priv->column_drag_info);
  priv->column_drag_info = NULL;
  priv->cur_reorder = NULL;

  /* Reset our flags */
  priv->drag_column_surface_state = DRAG_COLUMN_WINDOW_STATE_UNSET;
  priv->in_column_drag = FALSE;

  return TRUE;
}

/* column drag gesture helper */
static gboolean
bobgui_tree_view_button_release_column_resize (BobguiTreeView *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  gpointer drag_data;

  priv->drag_pos = -1;

  /* unblock attached dnd signal handler */
  drag_data = g_object_get_data (G_OBJECT (tree_view), "bobgui-site-data");
  if (drag_data)
    g_signal_handlers_unblock_matched (tree_view,
				       G_SIGNAL_MATCH_DATA,
				       0, 0, NULL, NULL,
				       drag_data);

  priv->in_column_resize = FALSE;
  return TRUE;
}

static void
bobgui_tree_view_column_drag_gesture_end (BobguiGestureDrag *gesture,
                                       double          offset_x,
                                       double          offset_y,
                                       BobguiTreeView    *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  GdkEventSequence *sequence;

  sequence = bobgui_gesture_single_get_current_sequence (BOBGUI_GESTURE_SINGLE (gesture));

  /* Cancel reorder if the drag got cancelled */
  if (!bobgui_gesture_handles_sequence (BOBGUI_GESTURE (gesture), sequence))
    priv->cur_reorder = NULL;

  if (priv->in_column_drag)
    bobgui_tree_view_button_release_drag_column (tree_view);
  else if (priv->in_column_resize)
    bobgui_tree_view_button_release_column_resize (tree_view);
}

static void
bobgui_tree_view_drag_gesture_end (BobguiGestureDrag *gesture,
                                double          offset_x,
                                double          offset_y,
                                BobguiTreeView    *tree_view)
{
  bobgui_tree_view_stop_rubber_band (tree_view);
}

static void
bobgui_tree_view_click_gesture_released (BobguiGestureClick *gesture,
                                      int              n_press,
                                      double           x,
                                      double           y,
                                      BobguiTreeView     *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  GdkEventSequence *sequence;
  gboolean modify, extend;
  guint button;

  button = bobgui_gesture_single_get_current_button (BOBGUI_GESTURE_SINGLE (gesture));
  sequence = bobgui_gesture_single_get_current_sequence (BOBGUI_GESTURE_SINGLE (gesture));

  if (button != GDK_BUTTON_PRIMARY ||
      priv->button_pressed_node == NULL ||
      priv->button_pressed_node != priv->prelight_node)
    return;

  get_current_selection_modifiers (BOBGUI_EVENT_CONTROLLER (gesture), &modify, &extend);

  if (priv->arrow_prelit)
    {
      BobguiTreePath *path = NULL;

      path = _bobgui_tree_path_new_from_rbtree (priv->button_pressed_tree,
                                             priv->button_pressed_node);
      /* Actually activate the node */
      if (priv->button_pressed_node->children == NULL)
        bobgui_tree_view_real_expand_row (tree_view, path,
                                       priv->button_pressed_tree,
                                       priv->button_pressed_node,
                                       FALSE);
      else
        bobgui_tree_view_real_collapse_row (tree_view, path,
                                         priv->button_pressed_tree,
                                         priv->button_pressed_node);
      bobgui_tree_path_free (path);
    }
  else if (priv->activate_on_single_click && !modify && !extend)
    {
      BobguiTreePath *path = NULL;

      path = _bobgui_tree_path_new_from_rbtree (priv->button_pressed_tree,
                                             priv->button_pressed_node);
      bobgui_tree_view_row_activated (tree_view, path, priv->focus_column);
      bobgui_tree_path_free (path);
    }

  priv->button_pressed_tree = NULL;
  priv->button_pressed_node = NULL;

  if (sequence)
    ensure_unprelighted (tree_view);
}

/* BobguiWidget::motion_event function set.
 */

static gboolean
coords_are_over_arrow (BobguiTreeView   *tree_view,
                       BobguiTreeRBTree *tree,
                       BobguiTreeRBNode *node,
                       /* these are in bin window coords */
                       int            x,
                       int            y)
{
  GdkRectangle arrow;
  int x2;

  if (!bobgui_widget_get_realized (BOBGUI_WIDGET (tree_view)))
    return FALSE;

  if ((node->flags & BOBGUI_TREE_RBNODE_IS_PARENT) == 0)
    return FALSE;

  arrow.y = bobgui_tree_view_get_row_y_offset (tree_view, tree, node);
  arrow.height = bobgui_tree_view_get_row_height (tree_view, node);

  bobgui_tree_view_get_arrow_xrange (tree_view, tree, &arrow.x, &x2);

  arrow.width = x2 - arrow.x;

  return (x >= arrow.x &&
          x < (arrow.x + arrow.width) &&
	  y >= arrow.y &&
	  y < (arrow.y + arrow.height));
}

static gboolean
auto_expand_timeout (gpointer data)
{
  BobguiTreeView *tree_view = BOBGUI_TREE_VIEW (data);
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  BobguiTreePath *path;

  if (priv->prelight_node)
    {
      path = _bobgui_tree_path_new_from_rbtree (priv->prelight_tree,
				             priv->prelight_node);

      if (priv->prelight_node->children)
        bobgui_tree_view_collapse_row (tree_view, path);
      else
        bobgui_tree_view_expand_row (tree_view, path, FALSE);

      bobgui_tree_path_free (path);
    }

  priv->auto_expand_timeout = 0;

  return FALSE;
}

static void
remove_auto_expand_timeout (BobguiTreeView *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  g_clear_handle_id (&priv->auto_expand_timeout, g_source_remove);
}

static void
do_prelight (BobguiTreeView   *tree_view,
             BobguiTreeRBTree *tree,
             BobguiTreeRBNode *node,
	     /* these are in bin_window coords */
             int            x,
             int            y)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  if (priv->prelight_tree == tree &&
      priv->prelight_node == node)
    {
      /*  We are still on the same node,
	  but we might need to take care of the arrow  */

      if (tree && node && bobgui_tree_view_draw_expanders (tree_view))
	{
	  gboolean over_arrow;

	  over_arrow = coords_are_over_arrow (tree_view, tree, node, x, y);

	  if (over_arrow != priv->arrow_prelit)
	    {
	      if (over_arrow)
          priv->arrow_prelit = TRUE;
        else
          priv->arrow_prelit = FALSE;

	      bobgui_widget_queue_draw (BOBGUI_WIDGET (tree_view));
	    }
	}

      return;
    }

  if (priv->prelight_tree && priv->prelight_node)
    {
      /*  Unprelight the old node and arrow  */

      BOBGUI_TREE_RBNODE_UNSET_FLAG (priv->prelight_node,
                                  BOBGUI_TREE_RBNODE_IS_PRELIT);

      if (priv->arrow_prelit
          && bobgui_tree_view_draw_expanders (tree_view))
        {
          priv->arrow_prelit = FALSE;

          bobgui_widget_queue_draw (BOBGUI_WIDGET (tree_view));
        }

      bobgui_widget_queue_draw (BOBGUI_WIDGET (tree_view));
    }


  if (priv->hover_expand)
    remove_auto_expand_timeout (tree_view);

  /*  Set the new prelight values  */
  priv->prelight_node = node;
  priv->prelight_tree = tree;

  if (!node || !tree)
    return;

  /*  Prelight the new node and arrow  */

  if (bobgui_tree_view_draw_expanders (tree_view)
      && coords_are_over_arrow (tree_view, tree, node, x, y))
    {
      priv->arrow_prelit = TRUE;

      bobgui_widget_queue_draw (BOBGUI_WIDGET (tree_view));
    }

  BOBGUI_TREE_RBNODE_SET_FLAG (node, BOBGUI_TREE_RBNODE_IS_PRELIT);

  bobgui_widget_queue_draw (BOBGUI_WIDGET (tree_view));

  if (priv->hover_expand)
    {
      priv->auto_expand_timeout =
        g_timeout_add (AUTO_EXPAND_TIMEOUT, auto_expand_timeout, tree_view);
      gdk_source_set_static_name_by_id (priv->auto_expand_timeout, "[bobgui] auto_expand_timeout");
    }
}

static void
prelight_or_select (BobguiTreeView   *tree_view,
                    BobguiTreeRBTree *tree,
                    BobguiTreeRBNode *node,
                    /* these are in bin_window coords */
                    int            x,
                    int            y)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  BobguiSelectionMode mode = bobgui_tree_selection_get_mode (priv->selection);

  if (priv->hover_selection &&
      (mode == BOBGUI_SELECTION_SINGLE || mode == BOBGUI_SELECTION_BROWSE) &&
      !(priv->edited_column &&
        bobgui_cell_area_get_edit_widget
        (bobgui_cell_layout_get_area (BOBGUI_CELL_LAYOUT (priv->edited_column)))))
    {
      if (node)
        {
          if (!BOBGUI_TREE_RBNODE_FLAG_SET (node, BOBGUI_TREE_RBNODE_IS_SELECTED))
            {
              BobguiTreePath *path;

              path = _bobgui_tree_path_new_from_rbtree (tree, node);
              bobgui_tree_selection_select_path (priv->selection, path);
              if (BOBGUI_TREE_RBNODE_FLAG_SET (node, BOBGUI_TREE_RBNODE_IS_SELECTED))
                {
                  priv->draw_keyfocus = FALSE;
                  bobgui_tree_view_real_set_cursor (tree_view, path, 0);
                }
              bobgui_tree_path_free (path);
            }
        }

      else if (mode == BOBGUI_SELECTION_SINGLE)
        bobgui_tree_selection_unselect_all (priv->selection);
    }

  do_prelight (tree_view, tree, node, x, y);
}

static void
ensure_unprelighted (BobguiTreeView *tree_view)
{
  BobguiTreeViewPrivate *priv G_GNUC_UNUSED = bobgui_tree_view_get_instance_private (tree_view);

  do_prelight (tree_view,
	       NULL, NULL,
	       -1000, -1000); /* coords not possibly over an arrow */

  g_assert (priv->prelight_node == NULL);
}

static void
update_prelight (BobguiTreeView *tree_view,
                 int          x,
                 int          y)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  int new_y;
  BobguiTreeRBTree *tree;
  BobguiTreeRBNode *node;

  if (priv->tree == NULL)
    return;

  if (x == -10000)
    {
      ensure_unprelighted (tree_view);
      return;
    }

  new_y = TREE_WINDOW_Y_TO_RBTREE_Y (priv, y);
  if (new_y < 0)
    new_y = 0;

  bobgui_tree_rbtree_find_offset (priv->tree,
                               new_y, &tree, &node);

  if (node)
    prelight_or_select (tree_view, tree, node, x, y);
}

static gboolean
bobgui_tree_view_motion_resize_column (BobguiTreeView *tree_view,
                                    double       x,
                                    double       y)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  int new_width;
  BobguiTreeViewColumn *column;

  column = bobgui_tree_view_get_column (tree_view, priv->drag_pos);

  if (bobgui_widget_get_direction (BOBGUI_WIDGET (tree_view)) == BOBGUI_TEXT_DIR_RTL)
    new_width = MAX (priv->x_drag - x, 0);
  else
    new_width = MAX (x - priv->x_drag, 0);

  if (new_width != bobgui_tree_view_column_get_fixed_width (column))
    bobgui_tree_view_column_set_fixed_width (column, new_width);

  return FALSE;
}

static void
bobgui_tree_view_update_current_reorder (BobguiTreeView *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  BobguiTreeViewColumnReorder *reorder = NULL;
  GdkEventSequence *sequence;
  GList *list;
  double x;

  sequence = bobgui_gesture_single_get_current_sequence
    (BOBGUI_GESTURE_SINGLE (priv->column_drag_gesture));
  bobgui_gesture_get_point (priv->column_drag_gesture,
                         sequence, &x, NULL);
  x += bobgui_adjustment_get_value (priv->hadjustment);

  for (list = priv->column_drag_info; list; list = list->next)
    {
      reorder = (BobguiTreeViewColumnReorder *) list->data;
      if (x >= reorder->left_align && x < reorder->right_align)
	break;
      reorder = NULL;
    }

  priv->cur_reorder = reorder;
}

static void
bobgui_tree_view_vertical_autoscroll (BobguiTreeView *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  GdkRectangle visible_rect;
  int y;
  int offset;

  if (bobgui_gesture_is_recognized (priv->drag_gesture))
    {
      GdkEventSequence *sequence;
      double py;

      sequence = bobgui_gesture_single_get_current_sequence (BOBGUI_GESTURE_SINGLE (priv->drag_gesture));
      bobgui_gesture_get_point (priv->drag_gesture, sequence, NULL, &py);
      bobgui_tree_view_convert_widget_to_bin_window_coords (tree_view, 0, py,
                                                         NULL, &y);
    }
  else
    {
      y = priv->event_last_y;
      bobgui_tree_view_convert_widget_to_bin_window_coords (tree_view, 0, y, NULL, &y);
    }

  y += priv->dy;
  bobgui_tree_view_get_visible_rect (tree_view, &visible_rect);

  /* see if we are near the edge. */
  offset = y - (visible_rect.y + 2 * SCROLL_EDGE_SIZE);
  if (offset > 0)
    {
      offset = y - (visible_rect.y + visible_rect.height - 2 * SCROLL_EDGE_SIZE);
      if (offset < 0)
	return;
    }

  bobgui_adjustment_set_value (priv->vadjustment,
                            MAX (bobgui_adjustment_get_value (priv->vadjustment) + offset, 0.0));
}

static void
bobgui_tree_view_horizontal_autoscroll (BobguiTreeView *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  GdkEventSequence *sequence;
  GdkRectangle visible_rect;
  double x;
  int offset;

  sequence = bobgui_gesture_single_get_current_sequence
    (BOBGUI_GESTURE_SINGLE (priv->column_drag_gesture));
  bobgui_gesture_get_point (priv->column_drag_gesture,
                         sequence, &x, NULL);
  bobgui_tree_view_get_visible_rect (tree_view, &visible_rect);

  x += bobgui_adjustment_get_value (priv->hadjustment);

  /* See if we are near the edge. */
  offset = x - (visible_rect.x + SCROLL_EDGE_SIZE);
  if (offset > 0)
    {
      offset = x - (visible_rect.x + visible_rect.width - SCROLL_EDGE_SIZE);
      if (offset < 0)
	return;
    }
  offset = offset/3;

  bobgui_adjustment_set_value (priv->hadjustment,
                            MAX (bobgui_adjustment_get_value (priv->hadjustment) + offset, 0.0));
}

static void
bobgui_tree_view_motion_drag_column (BobguiTreeView *tree_view,
                                  double       x,
                                  double       y)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  BobguiTreeViewColumn *column = priv->drag_column;
  BobguiWidget *button;
  int width, button_width;

  button = bobgui_tree_view_column_get_button (column);
  x += bobgui_adjustment_get_value (priv->hadjustment);

  /* Handle moving the header */
  width = bobgui_widget_get_allocated_width (BOBGUI_WIDGET (tree_view));
  button_width = bobgui_widget_get_allocated_width (button);
  priv->drag_column_x = CLAMP (x - _bobgui_tree_view_column_get_drag_x (column), 0,
                               MAX (priv->width, width) - button_width);

  /* autoscroll, if needed */
  bobgui_tree_view_horizontal_autoscroll (tree_view);
  /* Update the current reorder position and arrow; */
  bobgui_tree_view_update_current_reorder (tree_view);
  bobgui_widget_queue_allocate (BOBGUI_WIDGET (tree_view));
}

static void
bobgui_tree_view_stop_rubber_band (BobguiTreeView *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  remove_scroll_timeout (tree_view);

  if (priv->rubber_band_status == RUBBER_BAND_ACTIVE)
    {
      BobguiTreePath *tmp_path;

      bobgui_widget_queue_draw (BOBGUI_WIDGET (tree_view));

      /* The anchor path should be set to the start path */
      if (priv->rubber_band_start_node)
        {
          tmp_path = _bobgui_tree_path_new_from_rbtree (priv->rubber_band_start_tree,
                                                     priv->rubber_band_start_node);

          if (priv->anchor)
            bobgui_tree_row_reference_free (priv->anchor);

          priv->anchor = bobgui_tree_row_reference_new (priv->model, tmp_path);

          bobgui_tree_path_free (tmp_path);
        }

      /* ... and the cursor to the end path */
      if (priv->rubber_band_end_node)
        {
          tmp_path = _bobgui_tree_path_new_from_rbtree (priv->rubber_band_end_tree,
                                                     priv->rubber_band_end_node);
          bobgui_tree_view_real_set_cursor (BOBGUI_TREE_VIEW (tree_view), tmp_path, 0);
          bobgui_tree_path_free (tmp_path);
        }

      _bobgui_tree_selection_emit_changed (priv->selection);

      bobgui_css_node_set_parent (priv->rubber_band_cssnode, NULL);
      priv->rubber_band_cssnode = NULL;
    }

  /* Clear status variables */
  priv->rubber_band_status = RUBBER_BAND_OFF;
  priv->rubber_band_extend = FALSE;
  priv->rubber_band_modify = FALSE;

  priv->rubber_band_start_node = NULL;
  priv->rubber_band_start_tree = NULL;
  priv->rubber_band_end_node = NULL;
  priv->rubber_band_end_tree = NULL;
}

static void
bobgui_tree_view_update_rubber_band_selection_range (BobguiTreeView  *tree_view,
						 BobguiTreeRBTree *start_tree,
						 BobguiTreeRBNode *start_node,
						 BobguiTreeRBTree *end_tree,
						 BobguiTreeRBNode *end_node,
						 gboolean       select,
						 gboolean       skip_start,
						 gboolean       skip_end)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  if (start_node == end_node)
    return;

  /* We skip the first node and jump inside the loop */
  if (skip_start)
    goto skip_first;

  do
    {
      /* Small optimization by assuming insensitive nodes are never
       * selected.
       */
      if (!BOBGUI_TREE_RBNODE_FLAG_SET (start_node, BOBGUI_TREE_RBNODE_IS_SELECTED))
        {
          BobguiTreePath *path;
          gboolean selectable;

          path = _bobgui_tree_path_new_from_rbtree (start_tree, start_node);
          selectable = _bobgui_tree_selection_row_is_selectable (priv->selection, start_node, path);
          bobgui_tree_path_free (path);

          if (!selectable)
            goto node_not_selectable;
        }

      if (select)
        {
          if (priv->rubber_band_extend)
            BOBGUI_TREE_RBNODE_SET_FLAG (start_node, BOBGUI_TREE_RBNODE_IS_SELECTED);
          else if (priv->rubber_band_modify)
            {
              /* Toggle the selection state */
              if (BOBGUI_TREE_RBNODE_FLAG_SET (start_node, BOBGUI_TREE_RBNODE_IS_SELECTED))
                BOBGUI_TREE_RBNODE_UNSET_FLAG (start_node, BOBGUI_TREE_RBNODE_IS_SELECTED);
              else
                BOBGUI_TREE_RBNODE_SET_FLAG (start_node, BOBGUI_TREE_RBNODE_IS_SELECTED);
            }
          else
            BOBGUI_TREE_RBNODE_SET_FLAG (start_node, BOBGUI_TREE_RBNODE_IS_SELECTED);
        }
      else
        {
          /* Mirror the above */
          if (priv->rubber_band_extend)
            BOBGUI_TREE_RBNODE_UNSET_FLAG (start_node, BOBGUI_TREE_RBNODE_IS_SELECTED);
          else if (priv->rubber_band_modify)
            {
              /* Toggle the selection state */
              if (BOBGUI_TREE_RBNODE_FLAG_SET (start_node, BOBGUI_TREE_RBNODE_IS_SELECTED))
                BOBGUI_TREE_RBNODE_UNSET_FLAG (start_node, BOBGUI_TREE_RBNODE_IS_SELECTED);
              else
                BOBGUI_TREE_RBNODE_SET_FLAG (start_node, BOBGUI_TREE_RBNODE_IS_SELECTED);
            }
          else
            BOBGUI_TREE_RBNODE_UNSET_FLAG (start_node, BOBGUI_TREE_RBNODE_IS_SELECTED);
        }

      bobgui_widget_queue_draw (BOBGUI_WIDGET (tree_view));

node_not_selectable:
      if (start_node == end_node)
	break;

skip_first:

      if (start_node->children)
        {
	  start_tree = start_node->children;
          start_node = bobgui_tree_rbtree_first (start_tree);
	}
      else
        {
	  bobgui_tree_rbtree_next_full (start_tree, start_node, &start_tree, &start_node);

	  if (!start_tree)
	    /* Ran out of tree */
	    break;
	}

      if (skip_end && start_node == end_node)
	break;
    }
  while (TRUE);
}

static void
bobgui_tree_view_update_rubber_band_selection (BobguiTreeView *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  BobguiTreeRBTree *start_tree, *end_tree;
  BobguiTreeRBNode *start_node, *end_node;
  double start_y, offset_y;
  int bin_y;

  if (!bobgui_gesture_is_active (priv->drag_gesture))
    return;

  bobgui_gesture_drag_get_offset (BOBGUI_GESTURE_DRAG (priv->drag_gesture),
                               NULL, &offset_y);
  bobgui_gesture_drag_get_start_point (BOBGUI_GESTURE_DRAG (priv->drag_gesture),
                                    NULL, &start_y);
  bobgui_tree_view_convert_widget_to_bin_window_coords (tree_view, 0, start_y,
						      NULL, &bin_y);
  bin_y = MAX (0, bin_y + offset_y + priv->dy);

  bobgui_tree_rbtree_find_offset (priv->tree, MIN (priv->press_start_y, bin_y), &start_tree, &start_node);
  bobgui_tree_rbtree_find_offset (priv->tree, MAX (priv->press_start_y, bin_y), &end_tree, &end_node);

  /* Handle the start area first */
  if (!start_node && !end_node)
    {
      if (priv->rubber_band_start_node)
        {
          BobguiTreeRBNode *node = priv->rubber_band_start_node;

	  if (priv->rubber_band_modify)
	    {
	      /* Toggle the selection state */
	      if (BOBGUI_TREE_RBNODE_FLAG_SET (node, BOBGUI_TREE_RBNODE_IS_SELECTED))
		BOBGUI_TREE_RBNODE_UNSET_FLAG (node, BOBGUI_TREE_RBNODE_IS_SELECTED);
	      else
		BOBGUI_TREE_RBNODE_SET_FLAG (node, BOBGUI_TREE_RBNODE_IS_SELECTED);
	    }
          else
            BOBGUI_TREE_RBNODE_UNSET_FLAG (node, BOBGUI_TREE_RBNODE_IS_SELECTED);

          bobgui_widget_queue_draw (BOBGUI_WIDGET (tree_view));
        }
    }
  if (!priv->rubber_band_start_node || !start_node)
    {
      bobgui_tree_view_update_rubber_band_selection_range (tree_view,
						       start_tree,
						       start_node,
						       end_tree,
						       end_node,
						       TRUE,
						       FALSE,
						       FALSE);
    }
  else if (bobgui_tree_rbtree_node_find_offset (start_tree, start_node) <
           bobgui_tree_rbtree_node_find_offset (priv->rubber_band_start_tree, priv->rubber_band_start_node))
    {
      /* New node is above the old one; selection became bigger */
      bobgui_tree_view_update_rubber_band_selection_range (tree_view,
						       start_tree,
						       start_node,
						       priv->rubber_band_start_tree,
						       priv->rubber_band_start_node,
						       TRUE,
						       FALSE,
						       TRUE);
    }
  else if (bobgui_tree_rbtree_node_find_offset (start_tree, start_node) >
           bobgui_tree_rbtree_node_find_offset (priv->rubber_band_start_tree, priv->rubber_band_start_node))
    {
      /* New node is below the old one; selection became smaller */
      bobgui_tree_view_update_rubber_band_selection_range (tree_view,
						       priv->rubber_band_start_tree,
						       priv->rubber_band_start_node,
						       start_tree,
						       start_node,
						       FALSE,
						       FALSE,
						       TRUE);
    }

  priv->rubber_band_start_tree = start_tree;
  priv->rubber_band_start_node = start_node;

  /* Next, handle the end area */
  if (!priv->rubber_band_end_node)
    {
      /* In the event this happens, start_node was also NULL; this case is
       * handled above.
       */
    }
  else if (!end_node)
    {
      /* Find the last node in the tree */
      bobgui_tree_rbtree_find_offset (priv->tree, bobgui_tree_view_get_height (tree_view) - 1,
			       &end_tree, &end_node);

      /* Selection reached end of the tree */
      bobgui_tree_view_update_rubber_band_selection_range (tree_view,
                                                        priv->rubber_band_end_tree,
                                                        priv->rubber_band_end_node,
                                                        end_tree,
                                                        end_node,
                                                        TRUE,
                                                        TRUE,
                                                        FALSE);
    }
  else if (bobgui_tree_rbtree_node_find_offset (end_tree, end_node) >
           bobgui_tree_rbtree_node_find_offset (priv->rubber_band_end_tree, priv->rubber_band_end_node))
    {
      /* New node is below the old one; selection became bigger */
      bobgui_tree_view_update_rubber_band_selection_range (tree_view,
                                                        priv->rubber_band_end_tree,
                                                        priv->rubber_band_end_node,
                                                        end_tree,
                                                        end_node,
                                                        TRUE,
                                                        TRUE,
                                                        FALSE);
    }
  else if (bobgui_tree_rbtree_node_find_offset (end_tree, end_node) <
           bobgui_tree_rbtree_node_find_offset (priv->rubber_band_end_tree, priv->rubber_band_end_node))
    {
      /* New node is above the old one; selection became smaller */
      bobgui_tree_view_update_rubber_band_selection_range (tree_view,
						       end_tree,
						       end_node,
						       priv->rubber_band_end_tree,
						       priv->rubber_band_end_node,
						       FALSE,
						       TRUE,
						       FALSE);
    }

  priv->rubber_band_end_tree = end_tree;
  priv->rubber_band_end_node = end_node;
}

static void
bobgui_tree_view_update_rubber_band (BobguiTreeView *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  double start_x, start_y, offset_x, offset_y, x, y;
  int bin_x, bin_y;

  if (!bobgui_gesture_is_recognized (priv->drag_gesture))
    return;

  bobgui_gesture_drag_get_offset (BOBGUI_GESTURE_DRAG (priv->drag_gesture),
                               &offset_x, &offset_y);
  bobgui_gesture_drag_get_start_point (BOBGUI_GESTURE_DRAG (priv->drag_gesture),
                                    &start_x, &start_y);
  bobgui_tree_view_convert_widget_to_bin_window_coords (tree_view, start_x, start_y,
                                                     &bin_x, &bin_y);
  bin_y += priv->dy;

  x = MAX (bin_x + offset_x, 0);
  y = MAX (bin_y + offset_y, 0);

  bobgui_widget_queue_draw (BOBGUI_WIDGET (tree_view));

  priv->rubber_band_x = x;
  priv->rubber_band_y = y;

  bobgui_tree_view_update_rubber_band_selection (tree_view);
}

static void
bobgui_tree_view_snapshot_rubber_band (BobguiTreeView *tree_view,
                                    BobguiSnapshot *snapshot)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  double start_x, start_y, offset_x, offset_y;
  GdkRectangle rect;
  BobguiStyleContext *context;
  int bin_x, bin_y;

  if (!bobgui_gesture_is_recognized (priv->drag_gesture))
    return;

  bobgui_gesture_drag_get_offset (BOBGUI_GESTURE_DRAG (priv->drag_gesture),
                               &offset_x, &offset_y);
  bobgui_gesture_drag_get_start_point (BOBGUI_GESTURE_DRAG (priv->drag_gesture),
                                    &start_x, &start_y);
  bobgui_tree_view_convert_widget_to_bin_window_coords (tree_view, start_x, start_y,
                                                     &bin_x, &bin_y);
  bin_x = MAX (0, bin_x + offset_x);
  bin_y = MAX (0, bin_y + offset_y + priv->dy);

  context = bobgui_widget_get_style_context (BOBGUI_WIDGET (tree_view));

  bobgui_style_context_save_to_node (context, priv->rubber_band_cssnode);

  rect.x = MIN (priv->press_start_x, bin_x);
  rect.y = MIN (priv->press_start_y, bin_y) - priv->dy;
  rect.width = ABS (priv->press_start_x - bin_x) + 1;
  rect.height = ABS (priv->press_start_y - bin_y) + 1;

  bobgui_snapshot_render_background (snapshot, context,
                                  rect.x, rect.y,
                                  rect.width, rect.height);
  bobgui_snapshot_render_frame (snapshot, context,
                             rect.x, rect.y,
                             rect.width, rect.height);

  bobgui_style_context_restore (context);
}

static void
bobgui_tree_view_column_drag_gesture_update (BobguiGestureDrag *gesture,
                                          double          offset_x,
                                          double          offset_y,
                                          BobguiTreeView    *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  double start_x, start_y, x, y;
  GdkEventSequence *sequence;

  sequence = bobgui_gesture_single_get_current_sequence (BOBGUI_GESTURE_SINGLE (gesture));

  if (bobgui_gesture_get_sequence_state (BOBGUI_GESTURE (gesture), sequence) != BOBGUI_EVENT_SEQUENCE_CLAIMED)
    return;

  bobgui_gesture_drag_get_start_point (gesture, &start_x, &start_y);
  x = start_x + offset_x;
  y = start_y + offset_y;

  if (priv->in_column_resize)
    bobgui_tree_view_motion_resize_column (tree_view, x, y);
  else if (priv->in_column_drag)
    bobgui_tree_view_motion_drag_column (tree_view, x, y);
}

static void
bobgui_tree_view_drag_gesture_update (BobguiGestureDrag *gesture,
                                   double          offset_x,
                                   double          offset_y,
                                   BobguiTreeView    *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  if (priv->tree == NULL)
    {
      bobgui_gesture_set_state (BOBGUI_GESTURE (gesture), BOBGUI_EVENT_SEQUENCE_DENIED);
      return;
    }

  if (priv->rubber_band_status == RUBBER_BAND_MAYBE_START)
    {
      BobguiCssNode *widget_node;

      widget_node = bobgui_widget_get_css_node (BOBGUI_WIDGET (tree_view));
      priv->rubber_band_cssnode = bobgui_css_node_new ();
      bobgui_css_node_set_name (priv->rubber_band_cssnode, g_quark_from_static_string ("rubberband"));
      bobgui_css_node_set_parent (priv->rubber_band_cssnode, widget_node);
      bobgui_css_node_set_state (priv->rubber_band_cssnode, bobgui_css_node_get_state (widget_node));
      g_object_unref (priv->rubber_band_cssnode);

      bobgui_tree_view_update_rubber_band (tree_view);

      priv->rubber_band_status = RUBBER_BAND_ACTIVE;
      bobgui_gesture_set_state (BOBGUI_GESTURE (gesture), BOBGUI_EVENT_SEQUENCE_CLAIMED);
    }
  else if (priv->rubber_band_status == RUBBER_BAND_ACTIVE)
    {
      bobgui_tree_view_update_rubber_band (tree_view);

      add_scroll_timeout (tree_view);
    }
  else if (!priv->rubber_band_status)
    {
      if (bobgui_tree_view_maybe_begin_dragging_row (tree_view))
        bobgui_gesture_set_state (BOBGUI_GESTURE (gesture), BOBGUI_EVENT_SEQUENCE_DENIED);
    }
}

static void
bobgui_tree_view_motion_controller_motion (BobguiEventControllerMotion *controller,
                                        double                    x,
                                        double                    y,
                                        BobguiTreeView              *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  BobguiTreeRBTree *tree;
  BobguiTreeRBNode *node;
  int new_y;
  GList *list;
  gboolean cursor_set = FALSE;

  if (priv->tree)
    {
      int bin_x, bin_y;

      /* If we are currently pressing down a button, we don't want to prelight anything else. */
      if (bobgui_gesture_is_active (priv->drag_gesture) ||
          bobgui_gesture_is_active (priv->click_gesture))
        node = NULL;

      bobgui_tree_view_convert_widget_to_bin_window_coords (tree_view, x, y,
							  &bin_x, &bin_y);
      new_y = MAX (0, TREE_WINDOW_Y_TO_RBTREE_Y (priv, bin_y));

      bobgui_tree_rbtree_find_offset (priv->tree, new_y, &tree, &node);

      priv->event_last_x = bin_x;
      priv->event_last_y = bin_y;
      prelight_or_select (tree_view, tree, node, bin_x, bin_y);
    }

  for (list = priv->columns; list; list = list->next)
    {
      BobguiTreeViewColumn *column = list->data;

      if (_bobgui_tree_view_column_coords_in_resize_rect (column, x, y))
        {
          bobgui_widget_set_cursor_from_name (BOBGUI_WIDGET (tree_view), "col-resize");
          cursor_set = TRUE;
          break;
        }
    }

  if (!cursor_set)
    bobgui_widget_set_cursor (BOBGUI_WIDGET (tree_view), NULL);
}

/* Invalidate the focus rectangle near the edge of the bin_window; used when
 * the tree is empty.
 */
static void
invalidate_empty_focus (BobguiTreeView *tree_view)
{
  if (!bobgui_widget_has_focus (BOBGUI_WIDGET (tree_view)))
    return;

  bobgui_widget_queue_draw (BOBGUI_WIDGET (tree_view));
}

static void
bobgui_tree_view_snapshot_grid_line (BobguiTreeView            *tree_view,
                                  BobguiSnapshot            *snapshot,
                                  BobguiOrientation          orientation,
                                  const graphene_point_t *start,
                                  float                   size)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  BobguiStyleContext *context;
  const GdkRGBA *grid_line_color;

  context = bobgui_widget_get_style_context (BOBGUI_WIDGET (tree_view));
  grid_line_color = bobgui_css_color_value_get_rgba (_bobgui_style_context_peek_property (context,
                                                                                    BOBGUI_CSS_PROPERTY_BORDER_TOP_COLOR));

  if (!gdk_rgba_equal (grid_line_color, &priv->grid_line_color) ||
      (orientation == BOBGUI_ORIENTATION_HORIZONTAL && !priv->horizontal_grid_line_texture) ||
      (orientation == BOBGUI_ORIENTATION_VERTICAL && !priv->vertical_grid_line_texture))
    {
      cairo_surface_t *surface;
      unsigned char *data;

      g_clear_object (&priv->horizontal_grid_line_texture);
      g_clear_object (&priv->vertical_grid_line_texture);
      priv->grid_line_color = *grid_line_color;

      surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 2, 1);
      data = cairo_image_surface_get_data (surface);
      /* just color the first pixel... */
      data[0] = round (CLAMP (grid_line_color->blue,  0, 1)  * 255);
      data[1] = round (CLAMP (grid_line_color->green, 0, 1) * 255);
      data[2] = round (CLAMP (grid_line_color->red,   0, 1)   * 255);
      data[3] = round (CLAMP (grid_line_color->alpha, 0, 1) * 255);

      priv->horizontal_grid_line_texture = gdk_texture_new_for_surface (surface);
      cairo_surface_destroy (surface);

      surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 1, 2);
      data = cairo_image_surface_get_data (surface);
      data[0] = round (CLAMP (grid_line_color->blue,  0, 1)  * 255);
      data[1] = round (CLAMP (grid_line_color->green, 0, 1) * 255);
      data[2] = round (CLAMP (grid_line_color->red,   0, 1)   * 255);
      data[3] = round (CLAMP (grid_line_color->alpha, 0, 1) * 255);

      priv->vertical_grid_line_texture = gdk_texture_new_for_surface (surface);
      cairo_surface_destroy (surface);
    }

  g_assert (priv->horizontal_grid_line_texture);
  g_assert (priv->vertical_grid_line_texture);

  if (orientation == BOBGUI_ORIENTATION_HORIZONTAL)
    {
      bobgui_snapshot_push_repeat (snapshot,
                                &GRAPHENE_RECT_INIT (
                                  start->x, start->y,
                                  size, 1),
                                NULL);
      bobgui_snapshot_append_texture (snapshot, priv->horizontal_grid_line_texture,
                                   &GRAPHENE_RECT_INIT (0, 0, 2, 1));
      bobgui_snapshot_pop (snapshot);
    }
  else /* VERTICAL */
    {
      bobgui_snapshot_push_repeat (snapshot,
                                &GRAPHENE_RECT_INIT (
                                  start->x, start->y,
                                  1, size),
                                NULL);
      bobgui_snapshot_append_texture (snapshot, priv->vertical_grid_line_texture,
                                   &GRAPHENE_RECT_INIT (0, 0, 1, 2));
      bobgui_snapshot_pop (snapshot);
    }
}

static void
bobgui_tree_view_snapshot_tree_line (BobguiTreeView            *tree_view,
                                  BobguiSnapshot            *snapshot,
                                  BobguiOrientation          orientation,
                                  const graphene_point_t *start,
                                  float                   size)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  BobguiStyleContext *context;
  const GdkRGBA *tree_line_color;

  context = bobgui_widget_get_style_context (BOBGUI_WIDGET (tree_view));
  tree_line_color = bobgui_css_color_value_get_rgba (_bobgui_style_context_peek_property (context,
                                                                                    BOBGUI_CSS_PROPERTY_BORDER_LEFT_COLOR));

  if (!gdk_rgba_equal (tree_line_color, &priv->tree_line_color) ||
      (orientation == BOBGUI_ORIENTATION_HORIZONTAL && !priv->horizontal_tree_line_texture) ||
      (orientation == BOBGUI_ORIENTATION_VERTICAL && !priv->vertical_tree_line_texture))
    {
      cairo_surface_t *surface;
      unsigned char *data;

      g_clear_object (&priv->horizontal_tree_line_texture);
      g_clear_object (&priv->vertical_tree_line_texture);
      priv->tree_line_color = *tree_line_color;

      surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 2, 1);
      data = cairo_image_surface_get_data (surface);
      /* just color the first pixel... */
      data[0] = round (CLAMP (tree_line_color->blue,  0, 1) * 255);
      data[1] = round (CLAMP (tree_line_color->green, 0, 1) * 255);
      data[2] = round (CLAMP (tree_line_color->red,   0, 1) * 255);
      data[3] = round (CLAMP (tree_line_color->alpha, 0, 1) * 255);

      priv->horizontal_tree_line_texture = gdk_texture_new_for_surface (surface);
      cairo_surface_destroy (surface);

      surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 1, 2);
      data = cairo_image_surface_get_data (surface);
      data[0] = round (CLAMP (tree_line_color->blue,  0, 1) * 255);
      data[1] = round (CLAMP (tree_line_color->green, 0, 1) * 255);
      data[2] = round (CLAMP (tree_line_color->red,   0, 1) * 255);
      data[3] = round (CLAMP (tree_line_color->alpha, 0, 1) * 255);

      priv->vertical_tree_line_texture = gdk_texture_new_for_surface (surface);
      cairo_surface_destroy (surface);
    }

  g_assert (priv->horizontal_tree_line_texture);
  g_assert (priv->vertical_tree_line_texture);

  if (orientation == BOBGUI_ORIENTATION_HORIZONTAL)
    {
      bobgui_snapshot_push_repeat (snapshot,
                                &GRAPHENE_RECT_INIT (
                                  start->x, start->y,
                                  size, 1),
                                NULL);
      bobgui_snapshot_append_texture (snapshot, priv->horizontal_tree_line_texture,
                                   &GRAPHENE_RECT_INIT (0, 0, 2, 1));
      bobgui_snapshot_pop (snapshot);
    }
  else /* VERTICAL */
    {
      bobgui_snapshot_push_repeat (snapshot,
                                &GRAPHENE_RECT_INIT (
                                  start->x, start->y,
                                  1, size),
                                NULL);
      bobgui_snapshot_append_texture (snapshot, priv->vertical_tree_line_texture,
                                   &GRAPHENE_RECT_INIT (0, 0, 1, 2));
      bobgui_snapshot_pop (snapshot);
    }
}

static void
bobgui_tree_view_snapshot_grid_lines (BobguiTreeView *tree_view,
                                   BobguiSnapshot *snapshot)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  GList *list, *first, *last;
  gboolean rtl;
  int current_x = 0;
  int tree_view_height;

  if (priv->grid_lines != BOBGUI_TREE_VIEW_GRID_LINES_VERTICAL &&
      priv->grid_lines != BOBGUI_TREE_VIEW_GRID_LINES_BOTH)
    return;

  rtl = (_bobgui_widget_get_direction (BOBGUI_WIDGET (tree_view)) == BOBGUI_TEXT_DIR_RTL);

  first = g_list_first (priv->columns);
  last = g_list_last (priv->columns);
  tree_view_height = bobgui_tree_view_get_height (tree_view);

  for (list = (rtl ? last : first);
       list;
       list = (rtl ? list->prev : list->next))
    {
      BobguiTreeViewColumn *column = list->data;

      /* We don't want a line for the last column */
      if (column == (rtl ? first->data : last->data))
        break;

      if (!bobgui_tree_view_column_get_visible (column))
        continue;

      current_x += bobgui_tree_view_column_get_width (column);

      bobgui_tree_view_snapshot_grid_line (tree_view,
                                        snapshot,
                                        BOBGUI_ORIENTATION_VERTICAL,
                                        &(graphene_point_t) { current_x - 1, 0 },
                                        tree_view_height);
    }
}

/* Warning: Very scary function.
 * Modify at your own risk
 *
 * KEEP IN SYNC WITH bobgui_tree_view_create_row_drag_icon()!
 * FIXME: It’s not...
 */
static void
bobgui_tree_view_bin_snapshot (BobguiWidget   *widget,
			    BobguiSnapshot *snapshot)
{
  BobguiTreeView *tree_view = BOBGUI_TREE_VIEW (widget);
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  const int x_scroll_offset = - bobgui_adjustment_get_value (priv->hadjustment);
  BobguiTreePath *path;
  BobguiTreeRBTree *tree;
  GList *list;
  BobguiTreeRBNode *node;
  BobguiTreeRBNode *drag_highlight = NULL;
  BobguiTreeRBTree *drag_highlight_tree = NULL;
  BobguiTreeIter iter;
  int new_y;
  int y_offset, cell_offset;
  int max_height;
  int depth;
  GdkRectangle background_area;
  GdkRectangle cell_area;
  GdkRectangle clip;
  guint flags;
  int bin_window_width;
  int bin_window_height;
  BobguiTreePath *drag_dest_path;
  GList *first_column, *last_column;
  gboolean has_can_focus_cell;
  gboolean rtl;
  int expander_size;
  gboolean draw_vgrid_lines, draw_hgrid_lines;
  BobguiStyleContext *context;
  gboolean parity;

  rtl = (_bobgui_widget_get_direction (widget) == BOBGUI_TEXT_DIR_RTL);
  context = bobgui_widget_get_style_context (widget);

  if (priv->tree == NULL)
    return;

  bin_window_width = bobgui_widget_get_width (BOBGUI_WIDGET (tree_view));
  bin_window_height = bobgui_widget_get_height(BOBGUI_WIDGET (tree_view));

  clip = (GdkRectangle) { 0, 0, bin_window_width, bin_window_height };
  new_y = TREE_WINDOW_Y_TO_RBTREE_Y (priv, clip.y);
  y_offset = -bobgui_tree_rbtree_find_offset (priv->tree, new_y, &tree, &node);

  if (bobgui_tree_view_get_height (tree_view) < bin_window_height)
    {
      BobguiStateFlags state;

      bobgui_style_context_save (context);
      bobgui_style_context_add_class (context, "cell");

      state = bobgui_style_context_get_state (context);
      state &= ~(BOBGUI_STATE_FLAG_FOCUSED | BOBGUI_STATE_FLAG_PRELIGHT |
                 BOBGUI_STATE_FLAG_SELECTED | BOBGUI_STATE_FLAG_DROP_ACTIVE);
      bobgui_style_context_set_state (context, state);

      bobgui_snapshot_render_background (snapshot, context,
                                      0, bobgui_tree_view_get_height (tree_view),
                                      bin_window_width,
                                      bin_window_height - bobgui_tree_view_get_height (tree_view));

      bobgui_style_context_restore (context);
    }

  if (node == NULL)
    return;

  /* find the path for the node */
  path = _bobgui_tree_path_new_from_rbtree (tree, node);
  bobgui_tree_model_get_iter (priv->model,
			   &iter,
			   path);
  depth = bobgui_tree_path_get_depth (path);
  bobgui_tree_path_free (path);

  drag_dest_path = NULL;

  if (priv->drag_dest_row)
    drag_dest_path = bobgui_tree_row_reference_get_path (priv->drag_dest_row);

  if (drag_dest_path)
    _bobgui_tree_view_find_node (tree_view, drag_dest_path,
                              &drag_highlight_tree, &drag_highlight);

  draw_vgrid_lines =
    priv->grid_lines == BOBGUI_TREE_VIEW_GRID_LINES_VERTICAL
    || priv->grid_lines == BOBGUI_TREE_VIEW_GRID_LINES_BOTH;
  draw_hgrid_lines =
    priv->grid_lines == BOBGUI_TREE_VIEW_GRID_LINES_HORIZONTAL
    || priv->grid_lines == BOBGUI_TREE_VIEW_GRID_LINES_BOTH;
  expander_size = bobgui_tree_view_get_expander_size (tree_view);

  /* Find the last column */
  for (last_column = g_list_last (priv->columns);
       last_column &&
       !(bobgui_tree_view_column_get_visible (BOBGUI_TREE_VIEW_COLUMN (last_column->data)));
       last_column = last_column->prev)
    ;

  /* and the first */
  for (first_column = g_list_first (priv->columns);
       first_column &&
       !(bobgui_tree_view_column_get_visible (BOBGUI_TREE_VIEW_COLUMN (first_column->data)));
       first_column = first_column->next)
    ;

  /* Actually process the expose event.  To do this, we want to
   * start at the first node of the event, and walk the tree in
   * order, drawing each successive node.
   */

  parity = !(bobgui_tree_rbtree_node_get_index (tree, node) % 2);

  do
    {
      gboolean is_separator = FALSE;

      parity = !parity;
      is_separator = row_is_separator (tree_view, &iter, NULL);

      max_height = bobgui_tree_view_get_row_height (tree_view, node);

      cell_offset = x_scroll_offset;

      background_area.y = y_offset + clip.y;
      background_area.height = max_height;

      flags = 0;

      if (BOBGUI_TREE_RBNODE_FLAG_SET (node, BOBGUI_TREE_RBNODE_IS_PRELIT))
	flags |= BOBGUI_CELL_RENDERER_PRELIT;

      if (BOBGUI_TREE_RBNODE_FLAG_SET (node, BOBGUI_TREE_RBNODE_IS_SELECTED))
        flags |= BOBGUI_CELL_RENDERER_SELECTED;

      /* we *need* to set cell data on all cells before the call
       * to _has_can_focus_cell, else _has_can_focus_cell() does not
       * return a correct value.
       */
      for (list = (rtl ? g_list_last (priv->columns) : g_list_first (priv->columns));
	   list;
	   list = (rtl ? list->prev : list->next))
        {
	  BobguiTreeViewColumn *column = list->data;
	  bobgui_tree_view_column_cell_set_cell_data (column,
						   priv->model,
						   &iter,
						   BOBGUI_TREE_RBNODE_FLAG_SET (node, BOBGUI_TREE_RBNODE_IS_PARENT),
						   node->children?TRUE:FALSE);
        }

      has_can_focus_cell = bobgui_tree_view_has_can_focus_cell (tree_view);

      for (list = (rtl ? g_list_last (priv->columns) : g_list_first (priv->columns));
	   list;
	   list = (rtl ? list->prev : list->next))
	{
	  BobguiTreeViewColumn *column = list->data;
	  BobguiStateFlags state = 0;
          int width;
          gboolean draw_focus;

	  if (!bobgui_tree_view_column_get_visible (column))
            continue;

          width = bobgui_tree_view_column_get_width (column);

	  if (cell_offset > clip.x + clip.width ||
	      cell_offset + width < clip.x)
	    {
	      cell_offset += width;
	      continue;
	    }

          if (bobgui_tree_view_column_get_sort_indicator (column))
	    flags |= BOBGUI_CELL_RENDERER_SORTED;
          else
            flags &= ~BOBGUI_CELL_RENDERER_SORTED;

	  if (priv->cursor_node == node)
            flags |= BOBGUI_CELL_RENDERER_FOCUSED;
          else
            flags &= ~BOBGUI_CELL_RENDERER_FOCUSED;

          if (BOBGUI_TREE_RBNODE_FLAG_SET (node, BOBGUI_TREE_RBNODE_IS_PARENT))
            flags |= BOBGUI_CELL_RENDERER_EXPANDABLE;
          else
            flags &= ~BOBGUI_CELL_RENDERER_EXPANDABLE;

          if (node->children)
            flags |= BOBGUI_CELL_RENDERER_EXPANDED;
          else
            flags &= ~BOBGUI_CELL_RENDERER_EXPANDED;

          background_area.x = cell_offset;
	  background_area.width = width;

          cell_area = background_area;
          cell_area.x += _TREE_VIEW_HORIZONTAL_SEPARATOR /2;
          cell_area.width -= _TREE_VIEW_HORIZONTAL_SEPARATOR;

	  if (draw_vgrid_lines)
	    {
	      if (list == first_column)
	        {
		  cell_area.width -= _TREE_VIEW_GRID_LINE_WIDTH / 2;
		}
	      else if (list == last_column)
	        {
		  cell_area.x += _TREE_VIEW_GRID_LINE_WIDTH / 2;
		  cell_area.width -= _TREE_VIEW_GRID_LINE_WIDTH / 2;
		}
	      else
	        {
	          cell_area.x += _TREE_VIEW_GRID_LINE_WIDTH / 2;
	          cell_area.width -= _TREE_VIEW_GRID_LINE_WIDTH;
		}
	    }

	  if (draw_hgrid_lines)
	    {
	      cell_area.y += _TREE_VIEW_GRID_LINE_WIDTH / 2;
	      cell_area.height -= _TREE_VIEW_GRID_LINE_WIDTH;
	    }

	  if (!gdk_rectangle_intersect (&clip, &background_area, NULL))
	    {
	      cell_offset += bobgui_tree_view_column_get_width (column);
	      continue;
	    }

          background_area.x -= x_scroll_offset;
          cell_area.x -= x_scroll_offset;

	  bobgui_tree_view_column_cell_set_cell_data (column,
						   priv->model,
						   &iter,
						   BOBGUI_TREE_RBNODE_FLAG_SET (node, BOBGUI_TREE_RBNODE_IS_PARENT),
						   node->children?TRUE:FALSE);

          bobgui_style_context_save (context);

          state = bobgui_cell_renderer_get_state (NULL, widget, flags);
          bobgui_style_context_set_state (context, state);

          bobgui_style_context_add_class (context, "cell");

	  if (node == priv->cursor_node && has_can_focus_cell
              && ((column == priv->focus_column
                   && priv->draw_keyfocus &&
                   bobgui_widget_has_visible_focus (widget))
                  || (column == priv->edited_column)))
            draw_focus = TRUE;
          else
            draw_focus = FALSE;

	  /* Draw background */
          bobgui_snapshot_render_background (snapshot, context,
                                          background_area.x,
                                          background_area.y,
                                          background_area.width,
                                          background_area.height);

          /* Draw frame */
          bobgui_snapshot_render_frame (snapshot, context,
                                     background_area.x,
                                     background_area.y,
                                     background_area.width,
                                     background_area.height);

	  if (bobgui_tree_view_is_expander_column (tree_view, column))
	    {
	      if (!rtl)
		cell_area.x += (depth - 1) * priv->level_indentation;
	      cell_area.width -= (depth - 1) * priv->level_indentation;

              if (bobgui_tree_view_draw_expanders (tree_view))
	        {
	          if (!rtl)
		    cell_area.x += depth * expander_size;
		  cell_area.width -= depth * expander_size;
		}

	      if (is_separator)
                {
                  GdkRGBA color;

                  bobgui_style_context_save (context);
                  bobgui_style_context_add_class (context, "separator");

                  bobgui_style_context_get_color (context, &color);
                  bobgui_snapshot_append_color (snapshot,
                                             &color,
                                             &GRAPHENE_RECT_INIT(
                                                 cell_area.x,
                                                 cell_area.y + cell_area.height / 2,
                                                 cell_area.x + cell_area.width,
                                                 1
                                             ));

                  bobgui_style_context_restore (context);
                }
	      else
                {
                  bobgui_tree_view_column_cell_snapshot (column,
                                                      snapshot,
                                                      &background_area,
                                                      &cell_area,
                                                      flags,
                                                      draw_focus);
                }

	      if (bobgui_tree_view_draw_expanders (tree_view)
		  && (node->flags & BOBGUI_TREE_RBNODE_IS_PARENT) == BOBGUI_TREE_RBNODE_IS_PARENT)
		{
		  bobgui_tree_view_snapshot_arrow (BOBGUI_TREE_VIEW (widget),
                                                snapshot,
                                                tree,
                                                node);
		}
	    }
	  else
	    {
	      if (is_separator)
                {
                  GdkRGBA color;

                  bobgui_style_context_save (context);
                  bobgui_style_context_add_class (context, "separator");

                  bobgui_style_context_get_color (context, &color);
                  bobgui_snapshot_append_color (snapshot,
                                             &color,
                                             &GRAPHENE_RECT_INIT(
                                                 cell_area.x,
                                                 cell_area.y + cell_area.height / 2,
                                                 cell_area.x + cell_area.width,
                                                 1
                                             ));

                  bobgui_style_context_restore (context);
                }
	      else
		bobgui_tree_view_column_cell_snapshot (column,
                                                    snapshot,
                                                    &background_area,
                                                    &cell_area,
                                                    flags,
                                                    draw_focus);
	    }

          if (draw_hgrid_lines)
            {
              if (background_area.y >= clip.y)
                bobgui_tree_view_snapshot_grid_line (tree_view,
                                                  snapshot,
                                                  BOBGUI_ORIENTATION_HORIZONTAL,
                                                  &(graphene_point_t) {
                                                    background_area.x, background_area.y
                                                  },
                                                  background_area.width);

              if (background_area.y + max_height < clip.y + clip.height)
                bobgui_tree_view_snapshot_grid_line (tree_view,
                                                  snapshot,
                                                  BOBGUI_ORIENTATION_HORIZONTAL,
                                                  &(graphene_point_t) {
                                                    background_area.x, background_area.y + max_height
                                                  },
                                                  background_area.width);
            }

	  if (bobgui_tree_view_is_expander_column (tree_view, column) &&
	      priv->tree_lines_enabled)
	    {
	      int x = background_area.x;
	      int mult = rtl ? -1 : 1;
	      int y0 = background_area.y;
	      int y1 = background_area.y + background_area.height/2;
	      int y2 = background_area.y + background_area.height;

              if (rtl)
                x += background_area.width - 1;

              if ((node->flags & BOBGUI_TREE_RBNODE_IS_PARENT) == BOBGUI_TREE_RBNODE_IS_PARENT &&
                  depth > 1)
                {
                  bobgui_tree_view_snapshot_tree_line (tree_view,
                                                    snapshot,
                                                    BOBGUI_ORIENTATION_HORIZONTAL,
                                                    &(graphene_point_t) {
                                                      x + expander_size * (depth - 1.5) * mult,
                                                      y1
                                                    },
                                                    mult * expander_size * 0.4);

                }
              else if (depth > 1)
                {
                  bobgui_tree_view_snapshot_tree_line (tree_view,
                                                    snapshot,
                                                    BOBGUI_ORIENTATION_HORIZONTAL,
                                                    &(graphene_point_t) {
                                                      x + expander_size * (depth - 1.5) * mult,
                                                      y1
                                                    },
                                                    mult * expander_size);
                }

              if (depth > 1)
                {
                  int i;
                  BobguiTreeRBNode *tmp_node;
                  BobguiTreeRBTree *tmp_tree;

                  if (!bobgui_tree_rbtree_next (tree, node))
                    bobgui_tree_view_snapshot_tree_line (tree_view,
                                                      snapshot,
                                                      BOBGUI_ORIENTATION_VERTICAL,
                                                      &(graphene_point_t) {
                                                        x + expander_size * (depth - 1.5) * mult,
                                                        y0
                                                      },
                                                      y1 - y0);
                  else
                    bobgui_tree_view_snapshot_tree_line (tree_view,
                                                      snapshot,
                                                      BOBGUI_ORIENTATION_VERTICAL,
                                                      &(graphene_point_t) {
                                                        x + expander_size * (depth - 1.5) * mult,
                                                        y0
                                                      },
                                                      y2 - y0);

                  tmp_node = tree->parent_node;
                  tmp_tree = tree->parent_tree;

                  for (i = depth - 2; i > 0; i--)
                    {
                      if (bobgui_tree_rbtree_next (tmp_tree, tmp_node))
                        bobgui_tree_view_snapshot_tree_line (tree_view,
                                                          snapshot,
                                                          BOBGUI_ORIENTATION_VERTICAL,
                                                          &(graphene_point_t) {
                                                            x + expander_size * (i - 0.5) * mult,
                                                            y0
                                                          },
                                                          y2 - y0);
                      tmp_node = tmp_tree->parent_node;
                      tmp_tree = tmp_tree->parent_tree;
                    }
                }
	    }

          bobgui_style_context_restore (context);
	  cell_offset += bobgui_tree_view_column_get_width (column);
	}

      if (node == drag_highlight)
        {
	  BobguiTreeRBTree *drag_tree = NULL;
	  BobguiTreeRBNode *drag_node = NULL;

          _bobgui_tree_view_find_node (tree_view, drag_dest_path, &drag_tree, &drag_node);
          if (drag_tree != NULL)
            {
              TreeViewDragInfo *di;

              di = get_info (tree_view);
              /* Draw indicator for the drop
               */

              switch (priv->drag_dest_pos)
                {
                case BOBGUI_TREE_VIEW_DROP_BEFORE:
                  bobgui_css_node_set_classes (di->cssnode, (const char *[]){"before", NULL});
                  break;

                case BOBGUI_TREE_VIEW_DROP_AFTER:
                  bobgui_css_node_set_classes (di->cssnode, (const char *[]){"after", NULL});
                  break;

                case BOBGUI_TREE_VIEW_DROP_INTO_OR_BEFORE:
                case BOBGUI_TREE_VIEW_DROP_INTO_OR_AFTER:
                  bobgui_css_node_set_classes (di->cssnode, (const char *[]){"into", NULL});
                  break;

                default:
                 break;
                }

             bobgui_style_context_save_to_node (context, di->cssnode);
             bobgui_style_context_set_state (context, bobgui_style_context_get_state (context) | BOBGUI_STATE_FLAG_DROP_ACTIVE);

             bobgui_snapshot_render_frame (snapshot, context,
                                        0, bobgui_tree_view_get_row_y_offset (tree_view, drag_tree, drag_node),
                                        bin_window_width,
                                        bobgui_tree_view_get_row_height (tree_view, drag_node));

             bobgui_style_context_restore (context);
          }
        }

      /* draw the big row-spanning focus rectangle, if needed */
      if (!has_can_focus_cell && node == priv->cursor_node &&
          priv->draw_keyfocus &&
	  bobgui_widget_has_visible_focus (widget))
        {
	  int tmp_y, tmp_height;
	  BobguiStateFlags focus_rect_state = 0;

          bobgui_style_context_save (context);

          focus_rect_state = bobgui_cell_renderer_get_state (NULL, widget, flags);
          bobgui_style_context_set_state (context, focus_rect_state);

	  if (draw_hgrid_lines)
	    {
	      tmp_y = bobgui_tree_view_get_row_y_offset (tree_view, tree, node) + _TREE_VIEW_GRID_LINE_WIDTH / 2;
              tmp_height = bobgui_tree_view_get_row_height (tree_view, node) - _TREE_VIEW_GRID_LINE_WIDTH;
	    }
	  else
	    {
	      tmp_y = bobgui_tree_view_get_row_y_offset (tree_view, tree, node);
              tmp_height = bobgui_tree_view_get_row_height (tree_view, node);
	    }

          bobgui_snapshot_render_focus (snapshot, context,
                                     0, tmp_y,
                                     bin_window_width,
                                     tmp_height);

          bobgui_style_context_restore (context);
        }

      y_offset += max_height;
      if (node->children)
	{
	  BobguiTreeIter parent = iter;
	  gboolean has_child;

	  tree = node->children;
          node = bobgui_tree_rbtree_first (tree);

	  has_child = bobgui_tree_model_iter_children (priv->model,
						    &iter,
						    &parent);
	  depth++;

	  /* Sanity Check! */
	  TREE_VIEW_INTERNAL_ASSERT_VOID (has_child);
	}
      else
	{
	  gboolean done = FALSE;

	  do
	    {
	      node = bobgui_tree_rbtree_next (tree, node);
	      if (node != NULL)
		{
		  gboolean has_next = bobgui_tree_model_iter_next (priv->model, &iter);
		  done = TRUE;

		  /* Sanity Check! */
		  TREE_VIEW_INTERNAL_ASSERT_VOID (has_next);
		}
	      else
		{
		  BobguiTreeIter parent_iter = iter;
		  gboolean has_parent;

		  node = tree->parent_node;
		  tree = tree->parent_tree;
		  if (tree == NULL)
		    /* we should go to done to free some memory */
		    goto done;
		  has_parent = bobgui_tree_model_iter_parent (priv->model,
							   &iter,
							   &parent_iter);
		  depth--;

		  /* Sanity check */
		  TREE_VIEW_INTERNAL_ASSERT_VOID (has_parent);
		}
	    }
	  while (!done);
	}
    }
  while (y_offset < clip.height);

done:
  bobgui_tree_view_snapshot_grid_lines (tree_view, snapshot);

  if (priv->rubber_band_status == RUBBER_BAND_ACTIVE)
    bobgui_tree_view_snapshot_rubber_band (tree_view, snapshot);

  if (drag_dest_path)
    bobgui_tree_path_free (drag_dest_path);
}

static void
bobgui_tree_view_snapshot (BobguiWidget   *widget,
                        BobguiSnapshot *snapshot)
{
  BobguiTreeView *tree_view = BOBGUI_TREE_VIEW (widget);
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  BobguiWidget *button;
  BobguiStyleContext *context;
  GList *list;
  int width, height;

  context = bobgui_widget_get_style_context (widget);
  width = bobgui_widget_get_width (widget);
  height = bobgui_widget_get_height (widget);

  bobgui_snapshot_push_clip (snapshot,
                          &GRAPHENE_RECT_INIT(
                              0, bobgui_tree_view_get_effective_header_height (tree_view),
                              width,
                              height - bobgui_tree_view_get_effective_header_height (tree_view)
                          ));

  bobgui_snapshot_save (snapshot);
  bobgui_snapshot_translate (snapshot, &GRAPHENE_POINT_INIT (
                          - (int) bobgui_adjustment_get_value (priv->hadjustment),
                          bobgui_tree_view_get_effective_header_height (tree_view)));
  bobgui_tree_view_bin_snapshot (widget, snapshot);
  bobgui_snapshot_restore (snapshot);

  /* We can't just chain up to Container::draw as it will try to send the
   * event to the headers, so we handle propagating it to our children
   * (eg. widgets being edited) ourselves.
   */
  for (list = priv->children; list; list = list->next)
    {
      BobguiTreeViewChild *child = list->data;

      bobgui_widget_snapshot_child (widget, child->widget, snapshot);
    }

  bobgui_snapshot_pop (snapshot);

  bobgui_snapshot_push_clip (snapshot,
                          &GRAPHENE_RECT_INIT(
                              0, 0,
                              width,
                              bobgui_tree_view_get_effective_header_height (tree_view)
                          ));

  bobgui_style_context_save (context);
  bobgui_style_context_remove_class (context, "view");

  for (list = priv->columns; list != NULL; list = list->next)
    {
      BobguiTreeViewColumn *column = list->data;

      if (column == priv->drag_column)
        continue;

      if (bobgui_tree_view_column_get_visible (column))
        {
          button = bobgui_tree_view_column_get_button (column);
          bobgui_widget_snapshot_child (widget, button, snapshot);
        }
    }

  if (priv->drag_column)
    {
      button = bobgui_tree_view_column_get_button (priv->drag_column);
      bobgui_widget_snapshot_child (widget, button, snapshot);
    }

  bobgui_style_context_restore (context);

  bobgui_snapshot_pop (snapshot);
}

enum
{
  DROP_HOME,
  DROP_RIGHT,
  DROP_LEFT,
  DROP_END
};

/* returns 0x1 when no column has been found -- yes it's hackish */
static BobguiTreeViewColumn *
bobgui_tree_view_get_drop_column (BobguiTreeView       *tree_view,
			       BobguiTreeViewColumn *column,
			       int                drop_position)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  BobguiTreeViewColumn *left_column = NULL;
  BobguiTreeViewColumn *cur_column = NULL;
  GList *tmp_list;

  if (!bobgui_tree_view_column_get_reorderable (column))
    return (BobguiTreeViewColumn *)0x1;

  switch (drop_position)
    {
      case DROP_HOME:
	/* find first column where we can drop */
	tmp_list = priv->columns;
	if (column == BOBGUI_TREE_VIEW_COLUMN (tmp_list->data))
	  return (BobguiTreeViewColumn *)0x1;

	while (tmp_list)
	  {
	    g_assert (tmp_list);

	    cur_column = BOBGUI_TREE_VIEW_COLUMN (tmp_list->data);
	    tmp_list = tmp_list->next;

	    if (left_column &&
                bobgui_tree_view_column_get_visible (left_column) == FALSE)
	      continue;

	    if (!priv->column_drop_func)
	      return left_column;

	    if (!priv->column_drop_func (tree_view, column, left_column, cur_column, priv->column_drop_func_data))
	      {
		left_column = cur_column;
		continue;
	      }

	    return left_column;
	  }

	if (!priv->column_drop_func)
	  return left_column;

	if (priv->column_drop_func (tree_view, column, left_column, NULL, priv->column_drop_func_data))
	  return left_column;
	else
	  return (BobguiTreeViewColumn *)0x1;
	break;

      case DROP_RIGHT:
	/* find first column after column where we can drop */
	tmp_list = priv->columns;

	for (; tmp_list; tmp_list = tmp_list->next)
	  if (BOBGUI_TREE_VIEW_COLUMN (tmp_list->data) == column)
	    break;

	if (!tmp_list || !tmp_list->next)
	  return (BobguiTreeViewColumn *)0x1;

	tmp_list = tmp_list->next;
	left_column = BOBGUI_TREE_VIEW_COLUMN (tmp_list->data);
	tmp_list = tmp_list->next;

	while (tmp_list)
	  {
	    g_assert (tmp_list);

	    cur_column = BOBGUI_TREE_VIEW_COLUMN (tmp_list->data);
	    tmp_list = tmp_list->next;

	    if (left_column &&
                bobgui_tree_view_column_get_visible (left_column) == FALSE)
	      {
		left_column = cur_column;
		if (tmp_list)
		  tmp_list = tmp_list->next;
	        continue;
	      }

	    if (!priv->column_drop_func)
	      return left_column;

	    if (!priv->column_drop_func (tree_view, column, left_column, cur_column, priv->column_drop_func_data))
	      {
		left_column = cur_column;
		continue;
	      }

	    return left_column;
	  }

	if (!priv->column_drop_func)
	  return left_column;

	if (priv->column_drop_func (tree_view, column, left_column, NULL, priv->column_drop_func_data))
	  return left_column;
	else
	  return (BobguiTreeViewColumn *)0x1;
	break;

      case DROP_LEFT:
	/* find first column before column where we can drop */
	tmp_list = priv->columns;

	for (; tmp_list; tmp_list = tmp_list->next)
	  if (BOBGUI_TREE_VIEW_COLUMN (tmp_list->data) == column)
	    break;

	if (!tmp_list || !tmp_list->prev)
	  return (BobguiTreeViewColumn *)0x1;

	tmp_list = tmp_list->prev;
	cur_column = BOBGUI_TREE_VIEW_COLUMN (tmp_list->data);
	tmp_list = tmp_list->prev;

	while (tmp_list)
	  {
	    g_assert (tmp_list);

	    left_column = BOBGUI_TREE_VIEW_COLUMN (tmp_list->data);

	    if (left_column &&
                bobgui_tree_view_column_get_visible (left_column) == FALSE)
	      {
		/*if (!tmp_list->prev)
		  return (BobguiTreeViewColumn *)0x1;
		  */
/*
		cur_column = BOBGUI_TREE_VIEW_COLUMN (tmp_list->prev->data);
		tmp_list = tmp_list->prev->prev;
		continue;*/

		cur_column = left_column;
		if (tmp_list)
		  tmp_list = tmp_list->prev;
		continue;
	      }

	    if (!priv->column_drop_func)
	      return left_column;

	    if (priv->column_drop_func (tree_view, column, left_column, cur_column, priv->column_drop_func_data))
	      return left_column;

	    cur_column = left_column;
	    tmp_list = tmp_list->prev;
	  }

	if (!priv->column_drop_func)
	  return NULL;

	if (priv->column_drop_func (tree_view, column, NULL, cur_column, priv->column_drop_func_data))
	  return NULL;
	else
	  return (BobguiTreeViewColumn *)0x1;
	break;

      case DROP_END:
	/* same as DROP_HOME case, but doing it backwards */
	tmp_list = g_list_last (priv->columns);
	cur_column = NULL;

	if (column == BOBGUI_TREE_VIEW_COLUMN (tmp_list->data))
	  return (BobguiTreeViewColumn *)0x1;

	while (tmp_list)
	  {
	    g_assert (tmp_list);

	    left_column = BOBGUI_TREE_VIEW_COLUMN (tmp_list->data);

	    if (left_column &&
                bobgui_tree_view_column_get_visible (left_column) == FALSE)
	      {
		cur_column = left_column;
		tmp_list = tmp_list->prev;
	      }

	    if (!priv->column_drop_func)
	      return left_column;

	    if (priv->column_drop_func (tree_view, column, left_column, cur_column, priv->column_drop_func_data))
	      return left_column;

	    cur_column = left_column;
	    tmp_list = tmp_list->prev;
	  }

	if (!priv->column_drop_func)
	  return NULL;

	if (priv->column_drop_func (tree_view, column, NULL, cur_column, priv->column_drop_func_data))
	  return NULL;
	else
	  return (BobguiTreeViewColumn *)0x1;
	break;

      default:
        return (BobguiTreeViewColumn *)0x1;
        break;
    }
}

static gboolean
bobgui_tree_view_search_key_cancels_search (guint keyval)
{
  return keyval == GDK_KEY_Escape
      || keyval == GDK_KEY_Tab
      || keyval == GDK_KEY_KP_Tab
      || keyval == GDK_KEY_ISO_Left_Tab;
}

static gboolean
bobgui_tree_view_key_controller_key_pressed (BobguiEventControllerKey *key,
                                          guint                  keyval,
                                          guint                  keycode,
                                          GdkModifierType        state,
                                          BobguiTreeView           *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  BobguiWidget *widget = BOBGUI_WIDGET (tree_view);
  BobguiWidget *button;

  if (priv->rubber_band_status)
    {
      if (keyval == GDK_KEY_Escape)
	bobgui_tree_view_stop_rubber_band (tree_view);

      return TRUE;
    }

  if (priv->in_column_drag)
    {
      if (keyval == GDK_KEY_Escape)
        bobgui_gesture_set_state (BOBGUI_GESTURE (priv->column_drag_gesture),
                               BOBGUI_EVENT_SEQUENCE_DENIED);
      return TRUE;
    }

  if (priv->headers_visible)
    {
      GList *focus_column;
      gboolean rtl;

      rtl = (bobgui_widget_get_direction (BOBGUI_WIDGET (tree_view)) == BOBGUI_TEXT_DIR_RTL);

      for (focus_column = priv->columns;
           focus_column;
           focus_column = focus_column->next)
        {
          BobguiTreeViewColumn *column = BOBGUI_TREE_VIEW_COLUMN (focus_column->data);

	  button = bobgui_tree_view_column_get_button (column);
          if (bobgui_widget_has_focus (button))
            break;
        }

      if (focus_column &&
          (state & GDK_SHIFT_MASK) && (state & GDK_ALT_MASK) &&
          (keyval == GDK_KEY_Left || keyval == GDK_KEY_KP_Left
           || keyval == GDK_KEY_Right || keyval == GDK_KEY_KP_Right))
        {
          BobguiTreeViewColumn *column = BOBGUI_TREE_VIEW_COLUMN (focus_column->data);
          int column_width;

          if (!bobgui_tree_view_column_get_resizable (column))
            {
              bobgui_widget_error_bell (widget);
              return TRUE;
            }

	  column_width = bobgui_tree_view_column_get_width (column);

          if (keyval == (rtl ? GDK_KEY_Right : GDK_KEY_Left)
              || keyval == (rtl ? GDK_KEY_KP_Right : GDK_KEY_KP_Left))
            {
	      column_width = MAX (column_width - 2, 0);
            }
          else if (keyval == (rtl ? GDK_KEY_Left : GDK_KEY_Right)
                   || keyval == (rtl ? GDK_KEY_KP_Left : GDK_KEY_KP_Right))
            {
	      column_width = column_width + 2;
            }

	  bobgui_tree_view_column_set_fixed_width (column, column_width);
	  bobgui_tree_view_column_set_expand (column, FALSE);
          return TRUE;
        }

      if (focus_column &&
          (state & GDK_ALT_MASK) &&
          (keyval == GDK_KEY_Left || keyval == GDK_KEY_KP_Left
           || keyval == GDK_KEY_Right || keyval == GDK_KEY_KP_Right
           || keyval == GDK_KEY_Home || keyval == GDK_KEY_KP_Home
           || keyval == GDK_KEY_End || keyval == GDK_KEY_KP_End))
        {
          BobguiTreeViewColumn *column = BOBGUI_TREE_VIEW_COLUMN (focus_column->data);

          if (keyval == (rtl ? GDK_KEY_Right : GDK_KEY_Left)
              || keyval == (rtl ? GDK_KEY_KP_Right : GDK_KEY_KP_Left))
            {
              BobguiTreeViewColumn *col;
              col = bobgui_tree_view_get_drop_column (tree_view, column, DROP_LEFT);
              if (col != (BobguiTreeViewColumn *)0x1)
                bobgui_tree_view_move_column_after (tree_view, column, col);
              else
                bobgui_widget_error_bell (widget);
            }
          else if (keyval == (rtl ? GDK_KEY_Left : GDK_KEY_Right)
                   || keyval == (rtl ? GDK_KEY_KP_Left : GDK_KEY_KP_Right))
            {
              BobguiTreeViewColumn *col;
              col = bobgui_tree_view_get_drop_column (tree_view, column, DROP_RIGHT);
              if (col != (BobguiTreeViewColumn *)0x1)
                bobgui_tree_view_move_column_after (tree_view, column, col);
              else
                bobgui_widget_error_bell (widget);
            }
          else if (keyval == GDK_KEY_Home || keyval == GDK_KEY_KP_Home)
            {
              BobguiTreeViewColumn *col;
              col = bobgui_tree_view_get_drop_column (tree_view, column, DROP_HOME);
              if (col != (BobguiTreeViewColumn *)0x1)
                bobgui_tree_view_move_column_after (tree_view, column, col);
              else
                bobgui_widget_error_bell (widget);
            }
          else if (keyval == GDK_KEY_End || keyval == GDK_KEY_KP_End)
            {
              BobguiTreeViewColumn *col;
              col = bobgui_tree_view_get_drop_column (tree_view, column, DROP_END);
              if (col != (BobguiTreeViewColumn *)0x1)
                bobgui_tree_view_move_column_after (tree_view, column, col);
              else
                bobgui_widget_error_bell (widget);
            }

          return TRUE;
        }
    }

  return FALSE;
}

static gboolean
bobgui_tree_view_forward_controller_key_pressed (BobguiEventControllerKey *key,
                                              guint                  keyval,
                                              guint                  keycode,
                                              GdkModifierType        state,
                                              BobguiTreeView           *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  if (priv->search_entry_avoid_unhandled_binding)
    {
      priv->search_entry_avoid_unhandled_binding = FALSE;
      return FALSE;
    }

  /* Initially, before the search window is visible, we pass the event to the
   * IM context of the search entry box. If it triggers a commit or a preedit,
   * we then show the search window without losing tree view focus.
   * If the search window is already visible, we forward the events to it,
   * keeping the focus on the tree view.
   */
  if (bobgui_widget_has_focus (BOBGUI_WIDGET (tree_view))
      && priv->enable_search
      && !priv->search_custom_entry_set
      && !bobgui_tree_view_search_key_cancels_search (keyval))
    {
      bobgui_tree_view_ensure_interactive_directory (tree_view);

      if (!bobgui_widget_is_visible (priv->search_popover))
        {
          priv->imcontext_changed = FALSE;

          bobgui_event_controller_key_forward (key, priv->search_entry);

          if (priv->imcontext_changed)
            return bobgui_tree_view_real_start_interactive_search (tree_view, FALSE);
        }
    }

  return FALSE;
}

static void
bobgui_tree_view_key_controller_key_released (BobguiEventControllerKey *key,
                                           guint                  keyval,
                                           guint                  keycode,
                                           GdkModifierType        state,
                                           BobguiTreeView           *tree_view)
{
}

static void
bobgui_tree_view_motion_controller_enter (BobguiEventControllerMotion *controller,
                                       double                    x,
                                       double                    y,
                                       BobguiTreeView              *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  BobguiTreeRBTree *tree;
  BobguiTreeRBNode *node;
  int new_y;

  if (priv->tree == NULL)
    return;

  /* find the node internally */
  new_y = TREE_WINDOW_Y_TO_RBTREE_Y(priv, y);
  if (new_y < 0)
    new_y = 0;
  bobgui_tree_rbtree_find_offset (priv->tree, new_y, &tree, &node);

  priv->event_last_x = x;
  priv->event_last_y = y;

  if ((priv->button_pressed_node == NULL) ||
      (priv->button_pressed_node == node))
    prelight_or_select (tree_view, tree, node, x, y);
}

static void
bobgui_tree_view_motion_controller_leave (BobguiEventControllerMotion *controller,
                                       BobguiTreeView              *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  if (priv->prelight_node)
    bobgui_widget_queue_draw (BOBGUI_WIDGET (tree_view));

  priv->event_last_x = -10000;
  priv->event_last_y = -10000;

  prelight_or_select (tree_view, NULL, NULL, -1000, -1000); /* not possibly over an arrow */
}

static void
bobgui_tree_view_focus_controller_focus_out (BobguiEventController   *focus,
                                          BobguiTreeView          *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  bobgui_widget_queue_draw (BOBGUI_WIDGET (tree_view));

  if (priv->search_popover &&
      !bobgui_event_controller_focus_contains_focus (BOBGUI_EVENT_CONTROLLER_FOCUS (focus)))
    bobgui_tree_view_search_popover_hide (priv->search_popover, tree_view);
}

/* Incremental Reflow
 */

static int
get_separator_height (BobguiTreeView *tree_view)
{
  BobguiStyleContext *context;
  BobguiCssStyle *style;
  double d;
  int min_size;

  context = bobgui_widget_get_style_context (BOBGUI_WIDGET (tree_view));
  bobgui_style_context_save (context);
  bobgui_style_context_add_class (context, "separator");

  style = bobgui_style_context_lookup_style (context);
  d = bobgui_css_number_value_get (style->size->min_height, 100);

  if (d < 1)
    min_size = ceil (d);
  else
    min_size = floor (d);

  bobgui_style_context_restore (context);

  return min_size;
}

/* Returns TRUE if it updated the size
 */
static gboolean
validate_row (BobguiTreeView   *tree_view,
	      BobguiTreeRBTree *tree,
	      BobguiTreeRBNode *node,
	      BobguiTreeIter   *iter,
	      BobguiTreePath *path)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  BobguiTreeViewColumn *column;
  BobguiStyleContext *context;
  GList *list, *first_column, *last_column;
  int height = 0;
  int depth = bobgui_tree_path_get_depth (path);
  gboolean retval = FALSE;
  gboolean is_separator = FALSE;
  gboolean draw_vgrid_lines, draw_hgrid_lines;
  int expander_size;
  int separator_height;

  /* double check the row needs validating */
  if (! BOBGUI_TREE_RBNODE_FLAG_SET (node, BOBGUI_TREE_RBNODE_INVALID) &&
      ! BOBGUI_TREE_RBNODE_FLAG_SET (node, BOBGUI_TREE_RBNODE_COLUMN_INVALID))
    return FALSE;

  is_separator = row_is_separator (tree_view, iter, NULL);

  draw_vgrid_lines =
    priv->grid_lines == BOBGUI_TREE_VIEW_GRID_LINES_VERTICAL
    || priv->grid_lines == BOBGUI_TREE_VIEW_GRID_LINES_BOTH;
  draw_hgrid_lines =
    priv->grid_lines == BOBGUI_TREE_VIEW_GRID_LINES_HORIZONTAL
    || priv->grid_lines == BOBGUI_TREE_VIEW_GRID_LINES_BOTH;
  expander_size = bobgui_tree_view_get_expander_size (tree_view);

  for (last_column = g_list_last (priv->columns);
       last_column &&
       !(bobgui_tree_view_column_get_visible (BOBGUI_TREE_VIEW_COLUMN (last_column->data)));
       last_column = last_column->prev)
    ;

  for (first_column = g_list_first (priv->columns);
       first_column &&
       !(bobgui_tree_view_column_get_visible (BOBGUI_TREE_VIEW_COLUMN (first_column->data)));
       first_column = first_column->next)
    ;

  separator_height = get_separator_height (tree_view);

  context = bobgui_widget_get_style_context (BOBGUI_WIDGET (tree_view));
  bobgui_style_context_save (context);
  bobgui_style_context_add_class (context, "cell");

  for (list = priv->columns; list; list = list->next)
    {
      int padding = 0;
      int original_width;
      int new_width;
      int row_height;

      column = list->data;

      if (!bobgui_tree_view_column_get_visible (column))
	continue;

      if (BOBGUI_TREE_RBNODE_FLAG_SET (node, BOBGUI_TREE_RBNODE_COLUMN_INVALID) &&
	  !_bobgui_tree_view_column_cell_get_dirty (column))
	continue;

      original_width = _bobgui_tree_view_column_get_requested_width (column);

      bobgui_tree_view_column_cell_set_cell_data (column, priv->model, iter,
					       BOBGUI_TREE_RBNODE_FLAG_SET (node, BOBGUI_TREE_RBNODE_IS_PARENT),
					       node->children?TRUE:FALSE);
      bobgui_tree_view_column_cell_get_size (column,
                                          NULL, NULL,
                                          NULL, &row_height);

      if (is_separator)
        {
          height = separator_height;
          /* bobgui_tree_view_get_row_height() assumes separator nodes are > 0 */
          height = MAX (height, 1);
        }
      else
        {
          height = MAX (height, row_height);
          height = MAX (height, expander_size);
        }

      if (bobgui_tree_view_is_expander_column (tree_view, column))
        {
	  padding += _TREE_VIEW_HORIZONTAL_SEPARATOR + (depth - 1) * priv->level_indentation;

	  if (bobgui_tree_view_draw_expanders (tree_view))
	    padding += depth * expander_size;
	}
      else
        padding += _TREE_VIEW_HORIZONTAL_SEPARATOR;

      if (draw_vgrid_lines)
        {
	  if (list->data == first_column || list->data == last_column)
	    padding += _TREE_VIEW_GRID_LINE_WIDTH / 2.0;
	  else
	    padding += _TREE_VIEW_GRID_LINE_WIDTH;
	}

      /* Update the padding for the column */
      _bobgui_tree_view_column_push_padding (column, padding);
      new_width = _bobgui_tree_view_column_get_requested_width (column);

      if (new_width > original_width)
	retval = TRUE;
    }

  bobgui_style_context_restore (context);

  if (draw_hgrid_lines)
    height += _TREE_VIEW_GRID_LINE_WIDTH;

  if (height != BOBGUI_TREE_RBNODE_GET_HEIGHT (node))
    {
      retval = TRUE;
      bobgui_tree_rbtree_node_set_height (tree, node, height);
    }
  bobgui_tree_rbtree_node_mark_valid (tree, node);

  return retval;
}


static void
validate_visible_area (BobguiTreeView *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  BobguiTreePath *path = NULL;
  BobguiTreePath *above_path = NULL;
  BobguiTreeIter iter = { 0, };
  BobguiTreeRBTree *tree = NULL;
  BobguiTreeRBNode *node = NULL;
  gboolean need_redraw = FALSE;
  gboolean size_changed = FALSE;
  int total_height;
  int area_above = 0;
  int area_below = 0;

  if (priv->tree == NULL)
    return;

  if (! BOBGUI_TREE_RBNODE_FLAG_SET (priv->tree->root, BOBGUI_TREE_RBNODE_DESCENDANTS_INVALID) &&
      priv->scroll_to_path == NULL)
    return;

  total_height = bobgui_widget_get_height (BOBGUI_WIDGET (tree_view))
                 - bobgui_tree_view_get_effective_header_height (tree_view);

  if (total_height == 0)
    return;

  /* First, we check to see if we need to scroll anywhere
   */
  if (priv->scroll_to_path)
    {
      path = bobgui_tree_row_reference_get_path (priv->scroll_to_path);
      if (path && !_bobgui_tree_view_find_node (tree_view, path, &tree, &node))
	{
          /* we are going to scroll, and will update dy */
	  bobgui_tree_model_get_iter (priv->model, &iter, path);
	  if (BOBGUI_TREE_RBNODE_FLAG_SET (node, BOBGUI_TREE_RBNODE_INVALID) ||
	      BOBGUI_TREE_RBNODE_FLAG_SET (node, BOBGUI_TREE_RBNODE_COLUMN_INVALID))
	    {
              bobgui_widget_queue_draw (BOBGUI_WIDGET (tree_view));
	      if (validate_row (tree_view, tree, node, &iter, path))
		size_changed = TRUE;
	    }

	  if (priv->scroll_to_use_align)
	    {
	      int height = bobgui_tree_view_get_row_height (tree_view, node);
	      area_above = (total_height - height) *
		priv->scroll_to_row_align;
	      area_below = total_height - area_above - height;
	      area_above = MAX (area_above, 0);
	      area_below = MAX (area_below, 0);
	    }
	  else
	    {
	      /* two cases:
	       * 1) row not visible
	       * 2) row visible
	       */
	      int dy;
	      int height = bobgui_tree_view_get_row_height (tree_view, node);

	      dy = bobgui_tree_rbtree_node_find_offset (tree, node);

	      if (dy >= bobgui_adjustment_get_value (priv->vadjustment) &&
		  dy + height <= (bobgui_adjustment_get_value (priv->vadjustment)
		                  + bobgui_adjustment_get_page_size (priv->vadjustment)))
	        {
		  /* row visible: keep the row at the same position */
		  area_above = dy - bobgui_adjustment_get_value (priv->vadjustment);
		  area_below = (bobgui_adjustment_get_value (priv->vadjustment) +
		                bobgui_adjustment_get_page_size (priv->vadjustment))
		               - dy - height;
		}
	      else
	        {
		  /* row not visible */
		  if (dy >= 0
		      && dy + height <= bobgui_adjustment_get_page_size (priv->vadjustment))
		    {
		      /* row at the beginning -- fixed */
		      area_above = dy;
		      area_below = bobgui_adjustment_get_page_size (priv->vadjustment)
				   - area_above - height;
		    }
		  else if (dy >= (bobgui_adjustment_get_upper (priv->vadjustment) -
			          bobgui_adjustment_get_page_size (priv->vadjustment)))
		    {
		      /* row at the end -- fixed */
		      area_above = dy - (bobgui_adjustment_get_upper (priv->vadjustment) -
			           bobgui_adjustment_get_page_size (priv->vadjustment));
                      area_below = bobgui_adjustment_get_page_size (priv->vadjustment) -
                                   area_above - height;

                      if (area_below < 0)
                        {
			  area_above = bobgui_adjustment_get_page_size (priv->vadjustment) - height;
                          area_below = 0;
                        }
		    }
		  else
		    {
		      /* row somewhere in the middle, bring it to the top
		       * of the view
		       */
		      area_above = 0;
		      area_below = total_height - height;
		    }
		}
	    }
	}
      else
	/* the scroll to isn't valid; ignore it.
	 */
	{
	  if (priv->scroll_to_path && !path)
	    {
	      bobgui_tree_row_reference_free (priv->scroll_to_path);
	      priv->scroll_to_path = NULL;
	    }
	  if (path)
	    bobgui_tree_path_free (path);
	  path = NULL;
	}
    }

  /* We didn't have a scroll_to set, so we just handle things normally
   */
  if (path == NULL)
    {
      int offset;

      offset = bobgui_tree_rbtree_find_offset (priv->tree,
                                            TREE_WINDOW_Y_TO_RBTREE_Y (priv, 0),
                                            &tree, &node);
      if (node == NULL)
	{
	  /* In this case, nothing has been validated */
	  path = bobgui_tree_path_new_first ();
	  _bobgui_tree_view_find_node (tree_view, path, &tree, &node);
	}
      else
	{
	  path = _bobgui_tree_path_new_from_rbtree (tree, node);
	  total_height += offset;
	}

      bobgui_tree_model_get_iter (priv->model, &iter, path);

      if (BOBGUI_TREE_RBNODE_FLAG_SET (node, BOBGUI_TREE_RBNODE_INVALID) ||
	  BOBGUI_TREE_RBNODE_FLAG_SET (node, BOBGUI_TREE_RBNODE_COLUMN_INVALID))
	{
          bobgui_widget_queue_draw (BOBGUI_WIDGET (tree_view));
	  if (validate_row (tree_view, tree, node, &iter, path))
	    size_changed = TRUE;
	}
      area_above = 0;
      area_below = total_height - bobgui_tree_view_get_row_height (tree_view, node);
    }

  above_path = bobgui_tree_path_copy (path);

  /* if we do not validate any row above the new top_row, we will make sure
   * that the row immediately above top_row has been validated. (if we do not
   * do this, bobgui_tree_rbtree_find_offset will find the row above top_row, because
   * when invalidated that row's height will be zero. and this will mess up
   * scrolling).
   */
  if (area_above == 0)
    {
      BobguiTreeRBTree *tmptree;
      BobguiTreeRBNode *tmpnode;

      _bobgui_tree_view_find_node (tree_view, above_path, &tmptree, &tmpnode);
      bobgui_tree_rbtree_prev_full (tmptree, tmpnode, &tmptree, &tmpnode);

      if (tmpnode)
        {
	  BobguiTreePath *tmppath;
	  BobguiTreeIter tmpiter;

	  tmppath = _bobgui_tree_path_new_from_rbtree (tmptree, tmpnode);
	  bobgui_tree_model_get_iter (priv->model, &tmpiter, tmppath);

	  if (BOBGUI_TREE_RBNODE_FLAG_SET (tmpnode, BOBGUI_TREE_RBNODE_INVALID) ||
	      BOBGUI_TREE_RBNODE_FLAG_SET (tmpnode, BOBGUI_TREE_RBNODE_COLUMN_INVALID))
	    {
              bobgui_widget_queue_draw (BOBGUI_WIDGET (tree_view));
	      if (validate_row (tree_view, tmptree, tmpnode, &tmpiter, tmppath))
		size_changed = TRUE;
	    }

	  bobgui_tree_path_free (tmppath);
	}
    }

  /* Now, we walk forwards and backwards, measuring rows. Unfortunately,
   * backwards is much slower then forward, as there is no iter_prev function.
   * We go forwards first in case we run out of tree.  Then we go backwards to
   * fill out the top.
   */
  while (node && area_below > 0)
    {
      if (node->children)
	{
	  BobguiTreeIter parent = iter;
	  gboolean has_child;

	  tree = node->children;
          node = bobgui_tree_rbtree_first (tree);

	  has_child = bobgui_tree_model_iter_children (priv->model,
						    &iter,
						    &parent);
	  TREE_VIEW_INTERNAL_ASSERT_VOID (has_child);
	  bobgui_tree_path_down (path);
	}
      else
	{
	  gboolean done = FALSE;
	  do
	    {
	      node = bobgui_tree_rbtree_next (tree, node);
	      if (node != NULL)
		{
		  gboolean has_next = bobgui_tree_model_iter_next (priv->model, &iter);
		  done = TRUE;
		  bobgui_tree_path_next (path);

		  /* Sanity Check! */
		  TREE_VIEW_INTERNAL_ASSERT_VOID (has_next);
		}
	      else
		{
		  BobguiTreeIter parent_iter = iter;
		  gboolean has_parent;

		  node = tree->parent_node;
		  tree = tree->parent_tree;
		  if (tree == NULL)
		    break;
		  has_parent = bobgui_tree_model_iter_parent (priv->model,
							   &iter,
							   &parent_iter);
		  bobgui_tree_path_up (path);

		  /* Sanity check */
		  TREE_VIEW_INTERNAL_ASSERT_VOID (has_parent);
		}
	    }
	  while (!done);
	}

      if (!node)
        break;

      if (BOBGUI_TREE_RBNODE_FLAG_SET (node, BOBGUI_TREE_RBNODE_INVALID) ||
	  BOBGUI_TREE_RBNODE_FLAG_SET (node, BOBGUI_TREE_RBNODE_COLUMN_INVALID))
	{
          bobgui_widget_queue_draw (BOBGUI_WIDGET (tree_view));
	  if (validate_row (tree_view, tree, node, &iter, path))
	      size_changed = TRUE;
	}

      area_below -= bobgui_tree_view_get_row_height (tree_view, node);
    }
  bobgui_tree_path_free (path);

  /* If we ran out of tree, and have extra area_below left, we need to add it
   * to area_above */
  if (area_below > 0)
    area_above += area_below;

  _bobgui_tree_view_find_node (tree_view, above_path, &tree, &node);

  /* We walk backwards */
  while (area_above > 0)
    {
      bobgui_tree_rbtree_prev_full (tree, node, &tree, &node);

      /* Always find the new path in the tree.  We cannot just assume
       * a bobgui_tree_path_prev() is enough here, as there might be children
       * in between this node and the previous sibling node.  If this
       * appears to be a performance hotspot in profiles, we can look into
       * intrigate logic for keeping path, node and iter in sync like
       * we do for forward walks.  (Which will be hard because of the lacking
       * iter_prev).
       */

      if (node == NULL)
	break;

      bobgui_tree_path_free (above_path);
      above_path = _bobgui_tree_path_new_from_rbtree (tree, node);

      bobgui_tree_model_get_iter (priv->model, &iter, above_path);

      if (BOBGUI_TREE_RBNODE_FLAG_SET (node, BOBGUI_TREE_RBNODE_INVALID) ||
	  BOBGUI_TREE_RBNODE_FLAG_SET (node, BOBGUI_TREE_RBNODE_COLUMN_INVALID))
	{
          bobgui_widget_queue_draw (BOBGUI_WIDGET (tree_view));
	  if (validate_row (tree_view, tree, node, &iter, above_path))
	    size_changed = TRUE;
	}
      area_above -= bobgui_tree_view_get_row_height (tree_view, node);
    }

  /* if we scrolled to a path, we need to set the dy here,
   * and sync the top row accordingly
   */
  if (priv->scroll_to_path)
    {
      bobgui_tree_view_set_top_row (tree_view, above_path, -area_above);
      bobgui_tree_view_top_row_to_dy (tree_view);

      need_redraw = TRUE;
    }
  else if (bobgui_tree_view_get_height (tree_view) <= bobgui_adjustment_get_page_size (priv->vadjustment))
    {
      /* when we are not scrolling, we should never set dy to something
       * else than zero. we update top_row to be in sync with dy = 0.
       */
      bobgui_adjustment_set_value (BOBGUI_ADJUSTMENT (priv->vadjustment), 0);
      bobgui_tree_view_dy_to_top_row (tree_view);
    }
  else if (bobgui_adjustment_get_value (priv->vadjustment) + bobgui_adjustment_get_page_size (priv->vadjustment) > bobgui_tree_view_get_height (tree_view))
    {
      bobgui_adjustment_set_value (BOBGUI_ADJUSTMENT (priv->vadjustment), bobgui_tree_view_get_height (tree_view) - bobgui_adjustment_get_page_size (priv->vadjustment));
      bobgui_tree_view_dy_to_top_row (tree_view);
    }
  else
    bobgui_tree_view_top_row_to_dy (tree_view);

  /* update width/height and queue a resize */
  if (size_changed)
    {
      BobguiRequisition requisition;

      /* We temporarily guess a size, under the assumption that it will be the
       * same when we get our next size_allocate.  If we don't do this, we'll be
       * in an inconsistent state if we call top_row_to_dy. */

      bobgui_widget_get_preferred_size (BOBGUI_WIDGET (tree_view),
                                     &requisition, NULL);
      bobgui_adjustment_set_upper (priv->hadjustment,
                                MAX (bobgui_adjustment_get_upper (priv->hadjustment), requisition.width));
      bobgui_adjustment_set_upper (priv->vadjustment,
                                MAX (bobgui_adjustment_get_upper (priv->vadjustment), requisition.height));
      bobgui_widget_queue_resize (BOBGUI_WIDGET (tree_view));
    }

  if (priv->scroll_to_path)
    {
      bobgui_tree_row_reference_free (priv->scroll_to_path);
      priv->scroll_to_path = NULL;
    }

  if (above_path)
    bobgui_tree_path_free (above_path);

  if (priv->scroll_to_column)
    {
      priv->scroll_to_column = NULL;
    }
  if (need_redraw)
    bobgui_widget_queue_draw (BOBGUI_WIDGET (tree_view));
}

static void
initialize_fixed_height_mode (BobguiTreeView *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  if (!priv->tree)
    return;

  if (priv->fixed_height < 0)
    {
      BobguiTreeIter iter;
      BobguiTreePath *path;

      BobguiTreeRBTree *tree = NULL;
      BobguiTreeRBNode *node = NULL;

      tree = priv->tree;
      node = tree->root;

      path = _bobgui_tree_path_new_from_rbtree (tree, node);
      bobgui_tree_model_get_iter (priv->model, &iter, path);

      validate_row (tree_view, tree, node, &iter, path);

      bobgui_tree_path_free (path);

      priv->fixed_height = bobgui_tree_view_get_row_height (tree_view, node);
    }

   bobgui_tree_rbtree_set_fixed_height (priv->tree,
                                 priv->fixed_height, TRUE);
}

/* Our strategy for finding nodes to validate is a little convoluted.  We find
 * the left-most uninvalidated node.  We then try walking right, validating
 * nodes.  Once we find a valid node, we repeat the previous process of finding
 * the first invalid node.
 */

static gboolean
do_validate_rows (BobguiTreeView *tree_view, gboolean queue_resize)
{
  static gboolean prevent_recursion_hack = FALSE;

  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  BobguiTreeRBTree *tree = NULL;
  BobguiTreeRBNode *node = NULL;
  gboolean validated_area = FALSE;
  int retval = TRUE;
  BobguiTreePath *path = NULL;
  BobguiTreeIter iter;
  GTimer *timer;

  int y = -1;
  int prev_height = -1;
  gboolean fixed_height = TRUE;

  g_assert (tree_view);

  /* prevent infinite recursion via get_preferred_width() */
  if (prevent_recursion_hack)
    return FALSE;

  if (priv->tree == NULL)
      return FALSE;

  if (priv->fixed_height_mode)
    {
      if (priv->fixed_height < 0)
        initialize_fixed_height_mode (tree_view);

      return FALSE;
    }

  timer = g_timer_new ();
  g_timer_start (timer);

  do
    {
      gboolean changed = FALSE;

      if (! BOBGUI_TREE_RBNODE_FLAG_SET (priv->tree->root, BOBGUI_TREE_RBNODE_DESCENDANTS_INVALID))
	{
	  retval = FALSE;
	  goto done;
	}

      if (path != NULL)
	{
	  node = bobgui_tree_rbtree_next (tree, node);
	  if (node != NULL)
	    {
	      TREE_VIEW_INTERNAL_ASSERT (bobgui_tree_model_iter_next (priv->model, &iter), FALSE);
	      bobgui_tree_path_next (path);
	    }
	  else
	    {
	      bobgui_tree_path_free (path);
	      path = NULL;
	    }
	}

      if (path == NULL)
	{
	  tree = priv->tree;
	  node = priv->tree->root;

	  g_assert (BOBGUI_TREE_RBNODE_FLAG_SET (node, BOBGUI_TREE_RBNODE_DESCENDANTS_INVALID));

	  do
	    {
	      if (!bobgui_tree_rbtree_is_nil (node->left) &&
		  BOBGUI_TREE_RBNODE_FLAG_SET (node->left, BOBGUI_TREE_RBNODE_DESCENDANTS_INVALID))
		{
		  node = node->left;
		}
              else if (!bobgui_tree_rbtree_is_nil (node->right) &&
		       BOBGUI_TREE_RBNODE_FLAG_SET (node->right, BOBGUI_TREE_RBNODE_DESCENDANTS_INVALID))
		{
		  node = node->right;
		}
	      else if (BOBGUI_TREE_RBNODE_FLAG_SET (node, BOBGUI_TREE_RBNODE_INVALID) ||
		       BOBGUI_TREE_RBNODE_FLAG_SET (node, BOBGUI_TREE_RBNODE_COLUMN_INVALID))
		{
		  break;
		}
	      else if (node->children != NULL)
		{
		  tree = node->children;
		  node = tree->root;
		}
	      else
		/* RBTree corruption!  All bad */
		g_assert_not_reached ();
	    }
	  while (TRUE);
	  path = _bobgui_tree_path_new_from_rbtree (tree, node);
	  bobgui_tree_model_get_iter (priv->model, &iter, path);
	}

      changed = validate_row (tree_view, tree, node, &iter, path);
      validated_area = changed || validated_area;

      if (changed)
        {
          int offset = bobgui_tree_view_get_row_y_offset (tree_view, tree, node);

          if (y == -1 || y > offset)
            y = offset;
        }

      if (!priv->fixed_height_check)
        {
	  int height;

	  height = bobgui_tree_view_get_row_height (tree_view, node);
	  if (prev_height < 0)
	    prev_height = height;
	  else if (prev_height != height)
	    fixed_height = FALSE;
	}
    }
  while (g_timer_elapsed (timer, NULL) < BOBGUI_TREE_VIEW_TIME_MS_PER_IDLE / 1000.);

  if (!priv->fixed_height_check)
   {
     if (fixed_height)
       bobgui_tree_rbtree_set_fixed_height (priv->tree, prev_height, FALSE);

     priv->fixed_height_check = 1;
   }

 done:
  if (validated_area)
    {
      BobguiRequisition requisition;
      int dummy;

      /* We temporarily guess a size, under the assumption that it will be the
       * same when we get our next size_allocate.  If we don't do this, we'll be
       * in an inconsistent state when we call top_row_to_dy. */

      /* FIXME: This is called from size_request, for some reason it is not infinitely
       * recursing, we cannot call bobgui_widget_get_preferred_size() here because that's
       * not allowed (from inside ->get_preferred_width/height() implementations, one
       * should call the vfuncs directly). However what is desired here is the full
       * size including any margins and limited by any alignment (i.e. after
       * BobguiWidget:adjust_size_request() is called).
       *
       * Currently bypassing this but the real solution is to not update the scroll adjustments
       * until we've received an allocation (never update scroll adjustments from size-requests).
       */
      prevent_recursion_hack = TRUE;
      bobgui_tree_view_measure (BOBGUI_WIDGET (tree_view),
                             BOBGUI_ORIENTATION_HORIZONTAL,
                             -1,
                             &requisition.width, &dummy,
                             NULL, NULL);
      bobgui_tree_view_measure (BOBGUI_WIDGET (tree_view),
                             BOBGUI_ORIENTATION_VERTICAL,
                             -1,
                             &requisition.height, &dummy,
                             NULL, NULL);
      prevent_recursion_hack = FALSE;

      /* If rows above the current position have changed height, this has
       * affected the current view and thus needs a redraw.
       */
      if (y != -1 && y < bobgui_adjustment_get_value (priv->vadjustment))
        bobgui_widget_queue_draw (BOBGUI_WIDGET (tree_view));

      bobgui_adjustment_set_upper (priv->hadjustment,
                                MAX (bobgui_adjustment_get_upper (priv->hadjustment), requisition.width));
      bobgui_adjustment_set_upper (priv->vadjustment,
                                MAX (bobgui_adjustment_get_upper (priv->vadjustment), requisition.height));

      if (queue_resize)
        bobgui_widget_queue_resize (BOBGUI_WIDGET (tree_view));
    }

  if (path) bobgui_tree_path_free (path);
  g_timer_destroy (timer);

  if (!retval && bobgui_widget_get_mapped (BOBGUI_WIDGET (tree_view)))
    update_prelight (tree_view,
                     priv->event_last_x,
                     priv->event_last_y);

  return retval;
}

static void
disable_adjustment_animation (BobguiTreeView *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  bobgui_adjustment_enable_animation (priv->vadjustment,
                                   NULL,
                                   bobgui_adjustment_get_animation_duration (priv->vadjustment));
}

static void
maybe_reenable_adjustment_animation (BobguiTreeView *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  if (priv->presize_handler_tick_cb != 0 ||
      priv->validate_rows_timer != 0)
    return;

  bobgui_adjustment_enable_animation (priv->vadjustment,
                                   bobgui_widget_get_frame_clock (BOBGUI_WIDGET (tree_view)),
                                   bobgui_adjustment_get_animation_duration (priv->vadjustment));
}

static gboolean
do_presize_handler (BobguiTreeView *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  if (priv->mark_rows_col_dirty)
   {
      if (priv->tree)
	bobgui_tree_rbtree_column_invalid (priv->tree);
      priv->mark_rows_col_dirty = FALSE;
    }
  validate_visible_area (tree_view);
  if (priv->presize_handler_tick_cb != 0)
    {
      bobgui_widget_remove_tick_callback (BOBGUI_WIDGET (tree_view), priv->presize_handler_tick_cb);
      priv->presize_handler_tick_cb = 0;
    }

  if (priv->fixed_height_mode)
    {
      BobguiRequisition requisition;

      bobgui_widget_get_preferred_size (BOBGUI_WIDGET (tree_view),
                                     &requisition, NULL);

      bobgui_adjustment_set_upper (priv->hadjustment,
                                MAX (bobgui_adjustment_get_upper (priv->hadjustment), requisition.width));
      bobgui_adjustment_set_upper (priv->vadjustment,
                                MAX (bobgui_adjustment_get_upper (priv->vadjustment), requisition.height));
      bobgui_widget_queue_resize (BOBGUI_WIDGET (tree_view));
    }

  maybe_reenable_adjustment_animation (tree_view);

  return FALSE;
}

static gboolean
presize_handler_callback (BobguiWidget     *widget,
                          GdkFrameClock *clock,
                          gpointer       unused)
{
  do_presize_handler (BOBGUI_TREE_VIEW (widget));

  return G_SOURCE_REMOVE;
}

static gboolean
validate_rows (BobguiTreeView *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  gboolean retval;

  if (priv->presize_handler_tick_cb)
    {
      do_presize_handler (tree_view);
      return G_SOURCE_CONTINUE;
    }

  retval = do_validate_rows (tree_view, TRUE);

  if (! retval && priv->validate_rows_timer)
    {
      g_source_remove (priv->validate_rows_timer);
      priv->validate_rows_timer = 0;
      maybe_reenable_adjustment_animation (tree_view);
    }

  return retval;
}

static void
install_presize_handler (BobguiTreeView *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  if (! bobgui_widget_get_realized (BOBGUI_WIDGET (tree_view)))
    return;

  disable_adjustment_animation (tree_view);

  if (! priv->presize_handler_tick_cb)
    {
      priv->presize_handler_tick_cb =
	bobgui_widget_add_tick_callback (BOBGUI_WIDGET (tree_view), presize_handler_callback, NULL, NULL);
    }
  if (! priv->validate_rows_timer)
    {
      priv->validate_rows_timer =
	g_idle_add_full (BOBGUI_TREE_VIEW_PRIORITY_VALIDATE, (GSourceFunc) validate_rows, tree_view, NULL);
      gdk_source_set_static_name_by_id (priv->validate_rows_timer, "[bobgui] validate_rows");
    }
}

static gboolean
scroll_sync_handler (BobguiTreeView *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  if (bobgui_tree_view_get_height (tree_view) <= bobgui_adjustment_get_page_size (priv->vadjustment))
    bobgui_adjustment_set_value (BOBGUI_ADJUSTMENT (priv->vadjustment), 0);
  else if (bobgui_tree_row_reference_valid (priv->top_row))
    bobgui_tree_view_top_row_to_dy (tree_view);
  else
    bobgui_tree_view_dy_to_top_row (tree_view);

  priv->scroll_sync_timer = 0;

  return FALSE;
}

static void
install_scroll_sync_handler (BobguiTreeView *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  if (!bobgui_widget_get_realized (BOBGUI_WIDGET (tree_view)))
    return;

  if (!priv->scroll_sync_timer)
    {
      priv->scroll_sync_timer =
	g_idle_add_full (BOBGUI_TREE_VIEW_PRIORITY_SCROLL_SYNC, (GSourceFunc) scroll_sync_handler, tree_view, NULL);
      gdk_source_set_static_name_by_id (priv->scroll_sync_timer, "[bobgui] scroll_sync_handler");
    }
}

static void
bobgui_tree_view_set_top_row (BobguiTreeView *tree_view,
			   BobguiTreePath *path,
			   int          offset)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  bobgui_tree_row_reference_free (priv->top_row);

  if (!path)
    {
      priv->top_row = NULL;
      priv->top_row_dy = 0;
    }
  else
    {
      priv->top_row = bobgui_tree_row_reference_new_proxy (G_OBJECT (tree_view), priv->model, path);
      priv->top_row_dy = offset;
    }
}

/* Always call this iff dy is in the visible range.  If the tree is empty, then
 * it’s set to be NULL, and top_row_dy is 0;
 */
static void
bobgui_tree_view_dy_to_top_row (BobguiTreeView *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  int offset;
  BobguiTreePath *path;
  BobguiTreeRBTree *tree;
  BobguiTreeRBNode *node;

  if (priv->tree == NULL)
    {
      bobgui_tree_view_set_top_row (tree_view, NULL, 0);
    }
  else
    {
      offset = bobgui_tree_rbtree_find_offset (priv->tree,
					priv->dy,
					&tree, &node);

      if (tree == NULL)
        {
	  bobgui_tree_view_set_top_row (tree_view, NULL, 0);
	}
      else
        {
	  path = _bobgui_tree_path_new_from_rbtree (tree, node);
	  bobgui_tree_view_set_top_row (tree_view, path, offset);
	  bobgui_tree_path_free (path);
	}
    }
}

static void
bobgui_tree_view_top_row_to_dy (BobguiTreeView *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  BobguiTreePath *path;
  BobguiTreeRBTree *tree;
  BobguiTreeRBNode *node;
  int new_dy;

  /* Avoid recursive calls */
  if (priv->in_top_row_to_dy)
    return;

  if (bobgui_adjustment_is_animating (priv->vadjustment))
    return;

  if (priv->top_row)
    path = bobgui_tree_row_reference_get_path (priv->top_row);
  else
    path = NULL;

  if (!path)
    {
      tree = NULL;
      node = NULL;
    }
  else
    _bobgui_tree_view_find_node (tree_view, path, &tree, &node);

  if (path)
    bobgui_tree_path_free (path);

  if (tree == NULL)
    {
      /* keep dy and set new toprow */
      bobgui_tree_row_reference_free (priv->top_row);
      priv->top_row = NULL;
      priv->top_row_dy = 0;
      /* DO NOT install the idle handler */
      bobgui_tree_view_dy_to_top_row (tree_view);
      return;
    }

  if (bobgui_tree_view_get_row_height (tree_view, node)
      < priv->top_row_dy)
    {
      /* new top row -- do NOT install the idle handler */
      bobgui_tree_view_dy_to_top_row (tree_view);
      return;
    }

  new_dy = bobgui_tree_rbtree_node_find_offset (tree, node);
  new_dy += priv->top_row_dy;

  if (new_dy + bobgui_adjustment_get_page_size (priv->vadjustment) > bobgui_tree_view_get_height (tree_view))
    new_dy = bobgui_tree_view_get_height (tree_view) - bobgui_adjustment_get_page_size (priv->vadjustment);

  new_dy = MAX (0, new_dy);

  priv->in_top_row_to_dy = TRUE;
  bobgui_adjustment_set_value (priv->vadjustment, (double)new_dy);
  priv->in_top_row_to_dy = FALSE;
}


void
_bobgui_tree_view_install_mark_rows_col_dirty (BobguiTreeView *tree_view,
					    gboolean     install_handler)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  priv->mark_rows_col_dirty = TRUE;

  if (install_handler)
    install_presize_handler (tree_view);
}

/*
 * This function works synchronously (due to the while (validate_rows...)
 * loop).
 *
 * There was a check for column_type != BOBGUI_TREE_VIEW_COLUMN_AUTOSIZE
 * here. You now need to check that yourself.
 */
void
_bobgui_tree_view_column_autosize (BobguiTreeView *tree_view,
			        BobguiTreeViewColumn *column)
{
  g_return_if_fail (BOBGUI_IS_TREE_VIEW (tree_view));
  g_return_if_fail (BOBGUI_IS_TREE_VIEW_COLUMN (column));

  _bobgui_tree_view_column_cell_set_dirty (column, FALSE);

  do_presize_handler (tree_view);
  while (validate_rows (tree_view));

  bobgui_widget_queue_resize (BOBGUI_WIDGET (tree_view));
}

/* Drag-and-drop */

typedef struct
{
  BobguiTreeRowReference *dest_row;
  guint                path_down_mode   : 1;
  guint                empty_view_drop  : 1;
  guint                drop_append_mode : 1;
}
DestRow;

static void
dest_row_free (gpointer data)
{
  DestRow *dr = (DestRow *)data;

  bobgui_tree_row_reference_free (dr->dest_row);
  g_slice_free (DestRow, dr);
}

static void
set_dest_row (GdkDrop      *drop,
              BobguiTreeModel *model,
              BobguiTreePath  *dest_row,
              gboolean      path_down_mode,
              gboolean      empty_view_drop,
              gboolean      drop_append_mode)
{
  DestRow *dr;

  if (!dest_row)
    {
      g_object_set_data_full (G_OBJECT (drop), I_("bobgui-tree-view-dest-row"),
                              NULL, NULL);
      return;
    }

  dr = g_slice_new (DestRow);

  dr->dest_row = bobgui_tree_row_reference_new (model, dest_row);
  dr->path_down_mode = path_down_mode != FALSE;
  dr->empty_view_drop = empty_view_drop != FALSE;
  dr->drop_append_mode = drop_append_mode != FALSE;

  g_object_set_data_full (G_OBJECT (drop), I_("bobgui-tree-view-dest-row"),
                          dr, (GDestroyNotify) dest_row_free);
}

static BobguiTreePath*
get_dest_row (GdkDrop  *drop,
              gboolean *path_down_mode)
{
  DestRow *dr =
    g_object_get_data (G_OBJECT (drop), "bobgui-tree-view-dest-row");

  if (dr)
    {
      BobguiTreePath *path = NULL;

      if (path_down_mode)
        *path_down_mode = dr->path_down_mode;

      if (dr->dest_row)
        path = bobgui_tree_row_reference_get_path (dr->dest_row);
      else if (dr->empty_view_drop)
        path = bobgui_tree_path_new_from_indices (0, -1);
      else
        path = NULL;

      if (path && dr->drop_append_mode)
        bobgui_tree_path_next (path);

      return path;
    }
  else
    return NULL;
}

/* Get/set whether drag_motion requested the drag data and
 * drag_data_received should thus not actually insert the data,
 * since the data doesn’t result from a drop.
 */
static void
set_status_pending (GdkDrop       *drop,
                    GdkDragAction  suggested_action)
{
  g_object_set_data (G_OBJECT (drop),
                     I_("bobgui-tree-view-status-pending"),
                     GINT_TO_POINTER (suggested_action));
}

static GdkDragAction
get_status_pending (GdkDrop *drop)
{
  return GPOINTER_TO_INT (g_object_get_data (G_OBJECT (drop),
                                             "bobgui-tree-view-status-pending"));
}

static TreeViewDragInfo*
get_info (BobguiTreeView *tree_view)
{
  return g_object_get_data (G_OBJECT (tree_view), "bobgui-tree-view-drag-info");
}

static void
destroy_info (TreeViewDragInfo *di)
{
  g_clear_pointer (&di->source_formats, gdk_content_formats_unref);
  g_clear_pointer (&di->source_item, bobgui_tree_row_reference_free);
  g_clear_object (&di->dest);

  g_slice_free (TreeViewDragInfo, di);
}

static TreeViewDragInfo*
ensure_info (BobguiTreeView *tree_view)
{
  TreeViewDragInfo *di;

  di = get_info (tree_view);

  if (di == NULL)
    {
      di = g_slice_new0 (TreeViewDragInfo);

      g_object_set_data_full (G_OBJECT (tree_view),
                              I_("bobgui-tree-view-drag-info"),
                              di,
                              (GDestroyNotify) destroy_info);
    }

  return di;
}

static void
remove_info (BobguiTreeView *tree_view)
{
  TreeViewDragInfo *di;

  di = get_info (tree_view);
  if (di && di->dest)
    bobgui_widget_remove_controller (BOBGUI_WIDGET (tree_view), BOBGUI_EVENT_CONTROLLER (di->dest));
  g_object_set_data (G_OBJECT (tree_view), I_("bobgui-tree-view-drag-info"), NULL);
}

static void
add_scroll_timeout (BobguiTreeView *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  if (priv->scroll_timeout == 0)
    {
      priv->scroll_timeout = g_timeout_add (150, scroll_row_timeout, tree_view);
      gdk_source_set_static_name_by_id (priv->scroll_timeout, "[bobgui] scroll_row_timeout");
    }
}

static void
remove_scroll_timeout (BobguiTreeView *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  g_clear_handle_id (&priv->scroll_timeout, g_source_remove);
}

static gboolean
check_model_dnd (BobguiTreeModel *model,
                 GType         required_iface,
                 const char   *signal)
{
  if (model == NULL || !G_TYPE_CHECK_INSTANCE_TYPE ((model), required_iface))
    {
      g_warning ("You must override the default '%s' handler "
                 "on BobguiTreeView when using models that don't support "
                 "the %s interface and enabling drag-and-drop. The simplest way to do this "
                 "is to connect to '%s' and call "
                 "g_signal_stop_emission_by_name() in your signal handler to prevent "
                 "the default handler from running. Look at the source code "
                 "for the default handler in bobguitreeview.c to get an idea what "
                 "your handler should do. (bobguitreeview.c is in the BOBGUI source "
                 "code.) If you're using BOBGUI from a language other than C, "
                 "there may be a more natural way to override default handlers, e.g. via derivation.",
                 signal, g_type_name (required_iface), signal);
      return FALSE;
    }
  else
    return TRUE;
}

static void
remove_open_timeout (BobguiTreeView *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  g_clear_handle_id (&priv->open_dest_timeout, g_source_remove);
}


static int
open_row_timeout (gpointer data)
{
  BobguiTreeView *tree_view = data;
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  BobguiTreePath *dest_path = NULL;
  BobguiTreeViewDropPosition pos;
  gboolean result = FALSE;

  bobgui_tree_view_get_drag_dest_row (tree_view,
                                   &dest_path,
                                   &pos);

  if (dest_path &&
      (pos == BOBGUI_TREE_VIEW_DROP_INTO_OR_AFTER ||
       pos == BOBGUI_TREE_VIEW_DROP_INTO_OR_BEFORE))
    {
      bobgui_tree_view_expand_row (tree_view, dest_path, FALSE);
      priv->open_dest_timeout = 0;

      bobgui_tree_path_free (dest_path);
    }
  else
    {
      if (dest_path)
        bobgui_tree_path_free (dest_path);

      result = TRUE;
    }

  return result;
}

static gboolean
scroll_row_timeout (gpointer data)
{
  BobguiTreeView *tree_view = data;
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  bobgui_tree_view_vertical_autoscroll (tree_view);

  if (priv->rubber_band_status == RUBBER_BAND_ACTIVE)
    bobgui_tree_view_update_rubber_band (tree_view);

  return TRUE;
}

static GdkDragAction
bobgui_tree_view_get_action (BobguiWidget *widget,
                          GdkDrop   *drop)
{
  BobguiTreeView *tree_view = BOBGUI_TREE_VIEW (widget);
  TreeViewDragInfo *di;
  GdkDrag *drag = gdk_drop_get_drag (drop);
  GdkDragAction actions;

  di = get_info (tree_view);

  actions = gdk_drop_get_actions (drop);

  if (di && di->drag == drag &&
      actions & GDK_ACTION_MOVE)
    return GDK_ACTION_MOVE;

  if (actions & GDK_ACTION_COPY)
    return GDK_ACTION_COPY;

  if (actions & GDK_ACTION_MOVE)
    return GDK_ACTION_MOVE;

  return 0;
}

/* Returns TRUE if event should not be propagated to parent widgets */
static gboolean
set_destination_row (BobguiTreeView         *tree_view,
                     GdkDrop             *drop,
                     BobguiDropTargetAsync  *dest,
                     /* coordinates relative to the widget */
                     int                  x,
                     int                  y,
                     GdkDragAction       *suggested_action,
                     GType               *target)
{
  BobguiTreePath *path = NULL;
  BobguiTreeViewDropPosition pos;
  BobguiTreeViewDropPosition old_pos;
  TreeViewDragInfo *di;
  BobguiWidget *widget;
  BobguiTreePath *old_dest_path = NULL;
  gboolean can_drop = FALSE;
  GdkContentFormats *formats;

  *suggested_action = 0;
  *target = G_TYPE_INVALID;

  widget = BOBGUI_WIDGET (tree_view);

  di = get_info (tree_view);

  if (di == NULL || y - bobgui_tree_view_get_effective_header_height (tree_view) < 0)
    {
      /* someone unset us as a drag dest, note that if
       * we return FALSE drag_leave isn't called
       */

      bobgui_tree_view_set_drag_dest_row (tree_view,
                                       NULL,
                                       BOBGUI_TREE_VIEW_DROP_BEFORE);

      remove_scroll_timeout (BOBGUI_TREE_VIEW (widget));
      remove_open_timeout (BOBGUI_TREE_VIEW (widget));

      return FALSE; /* no longer a drop site */
    }

  formats = bobgui_drop_target_async_get_formats (dest);
  *target = gdk_content_formats_match_gtype (formats, formats);
  if (*target == G_TYPE_INVALID)
    return FALSE;

  if (!bobgui_tree_view_get_dest_row_at_pos (tree_view,
                                          x, y,
                                          &path,
                                          &pos))
    {
      int n_children;
      BobguiTreeModel *model;

      remove_open_timeout (tree_view);

      /* the row got dropped on empty space, let's setup a special case
       */

      if (path)
	bobgui_tree_path_free (path);

      model = bobgui_tree_view_get_model (tree_view);

      n_children = bobgui_tree_model_iter_n_children (model, NULL);
      if (n_children)
        {
          pos = BOBGUI_TREE_VIEW_DROP_AFTER;
          path = bobgui_tree_path_new_from_indices (n_children - 1, -1);
        }
      else
        {
          pos = BOBGUI_TREE_VIEW_DROP_BEFORE;
          path = bobgui_tree_path_new_from_indices (0, -1);
        }

      can_drop = TRUE;

      goto out;
    }

  g_assert (path);

  /* If we left the current row's "open" zone, unset the timeout for
   * opening the row
   */
  bobgui_tree_view_get_drag_dest_row (tree_view,
                                   &old_dest_path,
                                   &old_pos);

  if (old_dest_path &&
      (bobgui_tree_path_compare (path, old_dest_path) != 0 ||
       !(pos == BOBGUI_TREE_VIEW_DROP_INTO_OR_AFTER ||
         pos == BOBGUI_TREE_VIEW_DROP_INTO_OR_BEFORE)))
    remove_open_timeout (tree_view);

  if (old_dest_path)
    bobgui_tree_path_free (old_dest_path);

  if (TRUE /* FIXME if the location droppable predicate */)
    {
      can_drop = TRUE;
    }

out:
  if (can_drop)
    {
      *suggested_action = bobgui_tree_view_get_action (widget, drop);

      bobgui_tree_view_set_drag_dest_row (BOBGUI_TREE_VIEW (widget),
                                       path, pos);
    }
  else
    {
      /* can't drop here */
      remove_open_timeout (tree_view);

      bobgui_tree_view_set_drag_dest_row (BOBGUI_TREE_VIEW (widget),
                                       NULL,
                                       BOBGUI_TREE_VIEW_DROP_BEFORE);
    }

  if (path)
    bobgui_tree_path_free (path);

  return TRUE;
}

static BobguiTreePath*
get_logical_dest_row (BobguiTreeView *tree_view,
                      gboolean    *path_down_mode,
                      gboolean    *drop_append_mode)
{
  /* adjust path to point to the row the drop goes in front of */
  BobguiTreePath *path = NULL;
  BobguiTreeViewDropPosition pos;

  g_return_val_if_fail (path_down_mode != NULL, NULL);
  g_return_val_if_fail (drop_append_mode != NULL, NULL);

  *path_down_mode = FALSE;
  *drop_append_mode = 0;

  bobgui_tree_view_get_drag_dest_row (tree_view, &path, &pos);

  if (path == NULL)
    return NULL;

  if (pos == BOBGUI_TREE_VIEW_DROP_BEFORE)
    ; /* do nothing */
  else if (pos == BOBGUI_TREE_VIEW_DROP_INTO_OR_BEFORE ||
           pos == BOBGUI_TREE_VIEW_DROP_INTO_OR_AFTER)
    *path_down_mode = TRUE;
  else
    {
      BobguiTreeIter iter;
      BobguiTreeModel *model = bobgui_tree_view_get_model (tree_view);

      g_assert (pos == BOBGUI_TREE_VIEW_DROP_AFTER);

      if (!bobgui_tree_model_get_iter (model, &iter, path) ||
          !bobgui_tree_model_iter_next (model, &iter))
        *drop_append_mode = 1;
      else
        {
          *drop_append_mode = 0;
          bobgui_tree_path_next (path);
        }
    }

  return path;
}

static gboolean
bobgui_tree_view_maybe_begin_dragging_row (BobguiTreeView *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  BobguiWidget *widget = BOBGUI_WIDGET (tree_view);
  double start_x, start_y, offset_x, offset_y;
  TreeViewDragInfo *di;
  BobguiTreePath *path = NULL;
  int button;
  BobguiTreeModel *model;
  gboolean retval = FALSE;
  int bin_x, bin_y;
  GdkSurface *surface;
  GdkDevice *device;
  GdkContentProvider *content;
  GdkDrag *drag;
  GdkPaintable *icon;

  di = get_info (tree_view);

  if (di == NULL || !di->source_set)
    goto out;

  if (!bobgui_gesture_is_recognized (priv->drag_gesture))
    goto out;

  bobgui_gesture_drag_get_start_point (BOBGUI_GESTURE_DRAG (priv->drag_gesture),
                                    &start_x, &start_y);
  bobgui_gesture_drag_get_offset (BOBGUI_GESTURE_DRAG (priv->drag_gesture),
                               &offset_x, &offset_y);

  if (!bobgui_drag_check_threshold_double (widget, 0, 0, offset_x, offset_y))
    goto out;

  model = bobgui_tree_view_get_model (tree_view);

  if (model == NULL)
    goto out;

  button = bobgui_gesture_single_get_current_button (BOBGUI_GESTURE_SINGLE (priv->drag_gesture));

  /* Deny the click gesture */
  bobgui_gesture_set_state (BOBGUI_GESTURE (priv->click_gesture),
                         BOBGUI_EVENT_SEQUENCE_DENIED);

  bobgui_tree_view_convert_widget_to_bin_window_coords (tree_view, start_x, start_y,
						      &bin_x, &bin_y);
  bobgui_tree_view_get_path_at_pos (tree_view, bin_x, bin_y, &path,
                                 NULL, NULL, NULL);

  if (path == NULL)
    goto out;

  if (!BOBGUI_IS_TREE_DRAG_SOURCE (model) ||
      !bobgui_tree_drag_source_row_draggable (BOBGUI_TREE_DRAG_SOURCE (model),
					   path))
    goto out;

  if (!(GDK_BUTTON1_MASK << (button - 1) & di->start_button_mask))
    goto out;

  /* Now we can begin the drag */
  bobgui_gesture_set_state (BOBGUI_GESTURE (priv->drag_gesture),
                         BOBGUI_EVENT_SEQUENCE_CLAIMED);

  surface = bobgui_native_get_surface (bobgui_widget_get_native (BOBGUI_WIDGET (tree_view)));
  device = bobgui_gesture_get_device (BOBGUI_GESTURE (priv->drag_gesture)),
  content = bobgui_tree_view_drag_data_get (tree_view, path);
  if (content == NULL)
    goto out;

  retval = TRUE;

  drag = gdk_drag_begin (surface, device, content, di->source_actions, start_x, start_y);

  g_object_unref (content);

  g_signal_connect (drag, "dnd-finished", G_CALLBACK (bobgui_tree_view_dnd_finished_cb), tree_view);

  icon = bobgui_tree_view_create_row_drag_icon (tree_view, path);
  bobgui_drag_icon_set_from_paintable (drag, icon, priv->press_start_x + 1, 1);
  g_object_unref (icon);

  di->drag = drag;

  g_object_unref (drag);

  di->source_item = bobgui_tree_row_reference_new (model, path);

 out:
  if (path)
    bobgui_tree_path_free (path);

  return retval;
}

static void
bobgui_tree_view_dnd_finished_cb (GdkDrag   *drag,
                               BobguiWidget *widget)
{
  BobguiTreeView *tree_view = BOBGUI_TREE_VIEW (widget);
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  TreeViewDragInfo *di;
  BobguiTreeModel *model;
  BobguiTreePath *source_row;

  priv->event_last_x = -10000;
  priv->event_last_y = -10000;

  if (gdk_drag_get_selected_action (drag) != GDK_ACTION_MOVE)
    return;

  tree_view = BOBGUI_TREE_VIEW (widget);
  model = bobgui_tree_view_get_model (tree_view);

  if (!check_model_dnd (model, BOBGUI_TYPE_TREE_DRAG_SOURCE, "drag_data_delete"))
    return;

  di = get_info (tree_view);

  if (di == NULL || di->source_item == NULL)
    return;

  source_row = bobgui_tree_row_reference_get_path (di->source_item);

  if (source_row == NULL)
    return;

  bobgui_tree_drag_source_drag_data_delete (BOBGUI_TREE_DRAG_SOURCE (model), source_row);

  bobgui_tree_path_free (source_row);

  g_clear_pointer (&di->source_item, bobgui_tree_row_reference_free);
}

/* Default signal implementations for the drag signals */
static GdkContentProvider *
bobgui_tree_view_drag_data_get (BobguiTreeView *tree_view,
                             BobguiTreePath *source_row)
{
  BobguiTreeModel *model;
  GdkContentProvider *content;

  model = bobgui_tree_view_get_model (tree_view);

  if (model == NULL)
    return NULL;

  /* We can implement the BOBGUI_TREE_MODEL_ROW target generically for
   * any model; for DragSource models there are some other targets
   * we also support.
   */

  if (BOBGUI_IS_TREE_DRAG_SOURCE (model))
    content = bobgui_tree_drag_source_drag_data_get (BOBGUI_TREE_DRAG_SOURCE (model), source_row);
  else
    content = NULL;

  /* If drag_data_get does nothing, try providing row data. */
  if (!content)
    content = bobgui_tree_create_row_drag_content (model, source_row);

  return content;
}

static void
bobgui_tree_view_drag_leave (BobguiDropTargetAsync *dest,
                          GdkDrop            *drop,
                          BobguiTreeView        *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  /* unset any highlight row */
  bobgui_tree_view_set_drag_dest_row (tree_view,
                                   NULL,
                                   BOBGUI_TREE_VIEW_DROP_BEFORE);

  remove_scroll_timeout (tree_view);
  remove_open_timeout (tree_view);

  priv->event_last_x = -10000;
  priv->event_last_y = -10000;
}


static GdkDragAction
bobgui_tree_view_drag_motion (BobguiDropTargetAsync *dest,
                           GdkDrop            *drop,
                           double              x,
                           double              y,
                           BobguiTreeView        *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  gboolean empty;
  BobguiTreePath *path = NULL;
  BobguiTreeViewDropPosition pos;
  GdkDragAction suggested_action = 0;
  GType target;

  if (!set_destination_row (tree_view, drop, dest, x, y, &suggested_action, &target))
    return 0;

  priv->event_last_x = x;
  priv->event_last_y = y;

  bobgui_tree_view_get_drag_dest_row (tree_view, &path, &pos);

  /* we only know this *after* set_desination_row */
  empty = priv->empty_view_drop;

  if (path == NULL && !empty)
    {
      suggested_action = 0;
    }
  else
    {
      if (priv->open_dest_timeout == 0 &&
          (pos == BOBGUI_TREE_VIEW_DROP_INTO_OR_AFTER ||
           pos == BOBGUI_TREE_VIEW_DROP_INTO_OR_BEFORE))
        {
          priv->open_dest_timeout =
            g_timeout_add (AUTO_EXPAND_TIMEOUT, open_row_timeout, tree_view);
          gdk_source_set_static_name_by_id (priv->open_dest_timeout, "[bobgui] open_row_timeout");
        }
      else
        {
	  add_scroll_timeout (tree_view);
	}

      if (target == BOBGUI_TYPE_TREE_ROW_DATA)
        {
          /* Request data so we can use the source row when
           * determining whether to accept the drop
           */
          set_status_pending (drop, suggested_action);
          gdk_drop_read_value_async (drop, BOBGUI_TYPE_TREE_ROW_DATA, G_PRIORITY_DEFAULT, NULL, bobgui_tree_view_drag_data_received, tree_view);
        }
      else
        {
          set_status_pending (drop, 0);
        }
    }

  if (path)
    bobgui_tree_path_free (path);

  return suggested_action;
}


static gboolean
bobgui_tree_view_drag_drop (BobguiDropTargetAsync *dest,
                         GdkDrop            *drop,
                         double              x,
                         double              y,
                         BobguiTreeView        *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  BobguiTreePath *path;
  GdkDragAction suggested_action = 0;
  GType target = G_TYPE_INVALID;
  TreeViewDragInfo *di;
  BobguiTreeModel *model;
  gboolean path_down_mode;
  gboolean drop_append_mode;

  model = bobgui_tree_view_get_model (tree_view);

  remove_scroll_timeout (tree_view);
  remove_open_timeout (tree_view);

  di = get_info (tree_view);

  if (di == NULL)
    return FALSE;

  if (!check_model_dnd (model, BOBGUI_TYPE_TREE_DRAG_DEST, "drag_drop"))
    return FALSE;

  if (!set_destination_row (tree_view, drop, dest, x, y, &suggested_action, &target))
    return FALSE;

  path = get_logical_dest_row (tree_view, &path_down_mode, &drop_append_mode);

  if (target != G_TYPE_INVALID && path != NULL)
    {
      /* in case a motion had requested drag data, change things so we
       * treat drag data receives as a drop.
       */
      set_status_pending (drop, 0);
      set_dest_row (drop, model, path,
                    path_down_mode, priv->empty_view_drop,
                    drop_append_mode);
    }

  if (path)
    bobgui_tree_path_free (path);

  /* Unset this thing */
  bobgui_tree_view_set_drag_dest_row (tree_view,
                                   NULL,
                                   BOBGUI_TREE_VIEW_DROP_BEFORE);

  if (target != G_TYPE_INVALID)
    {
      gdk_drop_read_value_async (drop, BOBGUI_TYPE_TREE_ROW_DATA, G_PRIORITY_DEFAULT, NULL, bobgui_tree_view_drag_data_received, tree_view);
      return TRUE;
    }
  else
    return FALSE;
}

static void
bobgui_tree_view_drag_data_received (GObject      *source,
                                  GAsyncResult *result,
                                  gpointer      data)
{
  BobguiTreeView *tree_view = BOBGUI_TREE_VIEW (data);
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  GdkDrop *drop = GDK_DROP (source);
  BobguiTreePath *path;
  TreeViewDragInfo *di;
  BobguiTreeModel *model;
  BobguiTreePath *dest_row;
  GdkDragAction suggested_action;
  gboolean path_down_mode;
  gboolean drop_append_mode;
  const GValue *value;

  value = gdk_drop_read_value_finish (drop, result, NULL);
  if (value == NULL)
    return;

  model = bobgui_tree_view_get_model (tree_view);

  if (!check_model_dnd (model, BOBGUI_TYPE_TREE_DRAG_DEST, "drag_data_received"))
    return;

  di = get_info (tree_view);

  if (di == NULL)
    return;

  suggested_action = get_status_pending (drop);

  if (suggested_action)
    {
      /* We are getting this data due to a request in drag_motion,
       * rather than due to a request in drag_drop, so we are just
       * supposed to call drag_status, not actually paste in the
       * data.
       */
      path = get_logical_dest_row (tree_view, &path_down_mode,
                                   &drop_append_mode);

      if (path == NULL)
        suggested_action = 0;
      else if (path_down_mode)
        bobgui_tree_path_down (path);

      if (suggested_action)
        {
	  if (!bobgui_tree_drag_dest_row_drop_possible (BOBGUI_TREE_DRAG_DEST (model),
						     path,
						     value))
            {
              if (path_down_mode)
                {
                  path_down_mode = FALSE;
                  bobgui_tree_path_up (path);

                  if (!bobgui_tree_drag_dest_row_drop_possible (BOBGUI_TREE_DRAG_DEST (model),
                                                             path,
                                                             value))
                    suggested_action = 0;
                }
              else
	        suggested_action = 0;
            }
        }

      if (path)
        bobgui_tree_path_free (path);

      /* If you can't drop, remove user drop indicator until the next motion */
      if (suggested_action == 0)
        bobgui_tree_view_set_drag_dest_row (tree_view,
                                         NULL,
                                         BOBGUI_TREE_VIEW_DROP_BEFORE);

      return;
    }

  dest_row = get_dest_row (drop, &path_down_mode);

  if (dest_row == NULL)
    return;

  if (path_down_mode)
    {
      bobgui_tree_path_down (dest_row);
      if (!bobgui_tree_drag_dest_row_drop_possible (BOBGUI_TREE_DRAG_DEST (model),
                                                 dest_row, value))
        bobgui_tree_path_up (dest_row);
    }

  suggested_action = bobgui_tree_view_get_action (BOBGUI_WIDGET (tree_view), drop);

  if (suggested_action &&
      !bobgui_tree_drag_dest_drag_data_received (BOBGUI_TREE_DRAG_DEST (model),
                                              dest_row,
                                              value))
    suggested_action = 0;

  gdk_drop_finish (drop, suggested_action);

  if (bobgui_tree_path_get_depth (dest_row) == 1 &&
      bobgui_tree_path_get_indices (dest_row)[0] == 0 &&
      bobgui_tree_model_iter_n_children (priv->model, NULL) != 0)
    {
      /* special case drag to "0", scroll to first item */
      if (!priv->scroll_to_path)
        bobgui_tree_view_scroll_to_cell (tree_view, dest_row, NULL, FALSE, 0.0, 0.0);
    }

  bobgui_tree_path_free (dest_row);

  /* drop dest_row */
  set_dest_row (drop, NULL, NULL, FALSE, FALSE, FALSE);
}

static void
bobgui_tree_view_remove (BobguiTreeView  *tree_view,
                      BobguiWidget    *widget)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  BobguiTreeViewChild *child = NULL;
  GList *tmp_list;

  tmp_list = priv->children;
  while (tmp_list)
    {
      child = tmp_list->data;
      if (child->widget == widget)
	{
	  bobgui_widget_unparent (widget);

	  priv->children = g_list_remove_link (priv->children, tmp_list);
	  g_list_free_1 (tmp_list);
	  g_slice_free (BobguiTreeViewChild, child);
	  return;
	}

      tmp_list = tmp_list->next;
    }

  tmp_list = priv->columns;

  while (tmp_list)
    {
      BobguiTreeViewColumn *column;
      BobguiWidget         *button;

      column = tmp_list->data;
      button = bobgui_tree_view_column_get_button (column);

      if (button == widget)
	{
	  bobgui_widget_unparent (widget);
	  return;
	}
      tmp_list = tmp_list->next;
    }
}

/* Returns TRUE is any of the columns contains a cell that can-focus.
 * If this is not the case, a column-spanning focus rectangle will be
 * drawn.
 */
static gboolean
bobgui_tree_view_has_can_focus_cell (BobguiTreeView *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  GList *list;

  for (list = priv->columns; list; list = list->next)
    {
      BobguiTreeViewColumn *column = list->data;

      if (!bobgui_tree_view_column_get_visible (column))
	continue;
      if (bobgui_cell_area_is_activatable (bobgui_cell_layout_get_area (BOBGUI_CELL_LAYOUT (column))))
	return TRUE;
    }

  return FALSE;
}

static void
column_sizing_notify (GObject    *object,
                      GParamSpec *pspec,
                      gpointer    data)
{
  BobguiTreeViewColumn *c = BOBGUI_TREE_VIEW_COLUMN (object);

  if (bobgui_tree_view_column_get_sizing (c) != BOBGUI_TREE_VIEW_COLUMN_FIXED)
    /* disable fixed height mode */
    g_object_set (data, "fixed-height-mode", FALSE, NULL);
}

/**
 * bobgui_tree_view_set_fixed_height_mode:
 * @tree_view: a `BobguiTreeView`
 * @enable: %TRUE to enable fixed height mode
 *
 * Enables or disables the fixed height mode of @tree_view.
 * Fixed height mode speeds up `BobguiTreeView` by assuming that all
 * rows have the same height.
 * Only enable this option if all rows are the same height and all
 * columns are of type %BOBGUI_TREE_VIEW_COLUMN_FIXED.
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 **/
void
bobgui_tree_view_set_fixed_height_mode (BobguiTreeView *tree_view,
                                     gboolean     enable)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  GList *l;

  enable = enable != FALSE;

  if (enable == priv->fixed_height_mode)
    return;

  if (!enable)
    {
      priv->fixed_height_mode = 0;
      priv->fixed_height = -1;
    }
  else
    {
      /* make sure all columns are of type FIXED */
      for (l = priv->columns; l; l = l->next)
	{
	  g_return_if_fail (bobgui_tree_view_column_get_sizing (l->data) == BOBGUI_TREE_VIEW_COLUMN_FIXED);
	}

      /* yes, we really have to do this is in a separate loop */
      for (l = priv->columns; l; l = l->next)
	g_signal_connect (l->data, "notify::sizing",
			  G_CALLBACK (column_sizing_notify), tree_view);

      priv->fixed_height_mode = 1;
      priv->fixed_height = -1;
    }

  /* force a revalidation */
  install_presize_handler (tree_view);

  g_object_notify_by_pspec (G_OBJECT (tree_view), tree_view_props[PROP_FIXED_HEIGHT_MODE]);
}

/**
 * bobgui_tree_view_get_fixed_height_mode:
 * @tree_view: a `BobguiTreeView`
 *
 * Returns whether fixed height mode is turned on for @tree_view.
 *
 * Returns: %TRUE if @tree_view is in fixed height mode
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 **/
gboolean
bobgui_tree_view_get_fixed_height_mode (BobguiTreeView *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  return priv->fixed_height_mode;
}

/* Returns TRUE if the focus is within the headers, after the focus operation is
 * done
 */
static gboolean
bobgui_tree_view_header_focus (BobguiTreeView      *tree_view,
			    BobguiDirectionType  dir,
			    gboolean          clamp_column_visible)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  BobguiTreeViewColumn *column;
  BobguiWidget *button;
  BobguiWidget *focus_child;
  GList *last_column, *first_column;
  GList *tmp_list;
  gboolean rtl;

  if (! priv->headers_visible)
    return FALSE;

  focus_child = bobgui_widget_get_focus_child (BOBGUI_WIDGET (tree_view));

  first_column = priv->columns;
  while (first_column)
    {
      column = BOBGUI_TREE_VIEW_COLUMN (first_column->data);
      button = bobgui_tree_view_column_get_button (column);

      if (bobgui_widget_get_focusable (button) &&
          bobgui_tree_view_column_get_visible (column) &&
          (bobgui_tree_view_column_get_clickable (column) ||
           bobgui_tree_view_column_get_reorderable (column)))
	break;
      first_column = first_column->next;
    }

  /* No headers are visible, or are focusable.  We can't focus in or out.
   */
  if (first_column == NULL)
    return FALSE;

  last_column = g_list_last (priv->columns);
  while (last_column)
    {
      column = BOBGUI_TREE_VIEW_COLUMN (last_column->data);
      button = bobgui_tree_view_column_get_button (column);

      if (bobgui_widget_get_focusable (button) &&
          bobgui_tree_view_column_get_visible (column) &&
          (bobgui_tree_view_column_get_clickable (column) ||
           bobgui_tree_view_column_get_reorderable (column)))
	break;
      last_column = last_column->prev;
    }


  rtl = (bobgui_widget_get_direction (BOBGUI_WIDGET (tree_view)) == BOBGUI_TEXT_DIR_RTL);

  switch (dir)
    {
    case BOBGUI_DIR_TAB_BACKWARD:
    case BOBGUI_DIR_TAB_FORWARD:
    case BOBGUI_DIR_UP:
    case BOBGUI_DIR_DOWN:
      if (focus_child == NULL)
	{
	  if (priv->focus_column != NULL)
	    button = bobgui_tree_view_column_get_button (priv->focus_column);
	  else
	    button = NULL;

	  if (button && bobgui_widget_get_focusable (button))
	    focus_child = button;
	  else
	    focus_child = bobgui_tree_view_column_get_button (BOBGUI_TREE_VIEW_COLUMN (first_column->data));

	  bobgui_widget_grab_focus (focus_child);
	  break;
	}
      return FALSE;

    case BOBGUI_DIR_LEFT:
    case BOBGUI_DIR_RIGHT:
      if (focus_child == NULL)
	{
	  if (priv->focus_column != NULL)
	    focus_child = bobgui_tree_view_column_get_button (priv->focus_column);
	  else if (dir == BOBGUI_DIR_LEFT)
	    focus_child = bobgui_tree_view_column_get_button (BOBGUI_TREE_VIEW_COLUMN (last_column->data));
	  else
	    focus_child = bobgui_tree_view_column_get_button (BOBGUI_TREE_VIEW_COLUMN (first_column->data));

	  bobgui_widget_grab_focus (focus_child);
	  break;
	}

      if (bobgui_widget_child_focus (focus_child, dir))
	{
	  /* The focus moves inside the button. */
	  /* This is probably a great example of bad UI */
	  break;
	}

      /* We need to move the focus among the row of buttons. */
      for (tmp_list = priv->columns; tmp_list; tmp_list = tmp_list->next)
	if (bobgui_tree_view_column_get_button (BOBGUI_TREE_VIEW_COLUMN (tmp_list->data)) == focus_child)
	  break;

      if ((tmp_list == first_column && dir == (rtl ? BOBGUI_DIR_RIGHT : BOBGUI_DIR_LEFT))
	  || (tmp_list == last_column && dir == (rtl ? BOBGUI_DIR_LEFT : BOBGUI_DIR_RIGHT)))
        {
	  bobgui_widget_error_bell (BOBGUI_WIDGET (tree_view));
	  break;
	}

      while (tmp_list)
	{
	  if (dir == (rtl ? BOBGUI_DIR_LEFT : BOBGUI_DIR_RIGHT))
	    tmp_list = tmp_list->next;
	  else
	    tmp_list = tmp_list->prev;

	  if (tmp_list == NULL)
	    {
	      g_warning ("Internal button not found");
	      break;
	    }
	  column = tmp_list->data;
	  button = bobgui_tree_view_column_get_button (column);
	  if (button &&
	      bobgui_tree_view_column_get_visible (column) &&
	      bobgui_widget_get_focusable (button))
	    {
	      focus_child = button;
	      bobgui_widget_grab_focus (button);
	      break;
	    }
	}
      break;
    default:
      g_assert_not_reached ();
      break;
    }

  /* if focus child is non-null, we assume it's been set to the current focus child
   */
  if (focus_child)
    {
      for (tmp_list = priv->columns; tmp_list; tmp_list = tmp_list->next)
	if (bobgui_tree_view_column_get_button (BOBGUI_TREE_VIEW_COLUMN (tmp_list->data)) == focus_child)
	  {
            _bobgui_tree_view_set_focus_column (tree_view, BOBGUI_TREE_VIEW_COLUMN (tmp_list->data));
	    break;
	  }

      if (clamp_column_visible)
        {
	  bobgui_tree_view_clamp_column_visible (tree_view,
					      priv->focus_column,
					      FALSE);
	}
    }

  return (focus_child != NULL);
}

/* This function returns in 'path' the first focusable path, if the given path
 * is already focusable, it’s the returned one.
 */
static gboolean
search_first_focusable_path (BobguiTreeView    *tree_view,
			     BobguiTreePath   **path,
			     gboolean        search_forward,
			     BobguiTreeRBTree **new_tree,
			     BobguiTreeRBNode **new_node)
{
  BobguiTreeRBTree *tree = NULL;
  BobguiTreeRBNode *node = NULL;

  if (!path || !*path)
    return FALSE;

  _bobgui_tree_view_find_node (tree_view, *path, &tree, &node);

  if (!tree || !node)
    return FALSE;

  while (node && row_is_separator (tree_view, NULL, *path))
    {
      if (search_forward)
	bobgui_tree_rbtree_next_full (tree, node, &tree, &node);
      else
	bobgui_tree_rbtree_prev_full (tree, node, &tree, &node);

      if (*path)
	bobgui_tree_path_free (*path);

      if (node)
	*path = _bobgui_tree_path_new_from_rbtree (tree, node);
      else
	*path = NULL;
    }

  if (new_tree)
    *new_tree = tree;

  if (new_node)
    *new_node = node;

  return (*path != NULL);
}

static int
bobgui_tree_view_focus (BobguiWidget        *widget,
		     BobguiDirectionType  direction)
{
  BobguiTreeView *tree_view = BOBGUI_TREE_VIEW (widget);
  BobguiWidget *focus_child;

  focus_child = bobgui_widget_get_focus_child (widget);

  bobgui_tree_view_stop_editing (BOBGUI_TREE_VIEW (widget), FALSE);
  /* Case 1.  Headers currently have focus. */
  if (focus_child)
    {
      switch (direction)
	{
	case BOBGUI_DIR_LEFT:
	case BOBGUI_DIR_RIGHT:
	  bobgui_tree_view_header_focus (tree_view, direction, TRUE);
	  return TRUE;
	case BOBGUI_DIR_TAB_BACKWARD:
	case BOBGUI_DIR_UP:
	  return FALSE;
	case BOBGUI_DIR_TAB_FORWARD:
	case BOBGUI_DIR_DOWN:
	  return bobgui_widget_grab_focus (widget);
	default:
	  g_assert_not_reached ();
	  return FALSE;
	}
    }

  /* Case 2. We don't have focus at all. */
  if (!bobgui_widget_has_focus (widget))
    {
      return bobgui_widget_grab_focus (widget);
    }

  /* Case 3. We have focus already. */
  if (direction == BOBGUI_DIR_TAB_BACKWARD)
    return (bobgui_tree_view_header_focus (tree_view, direction, FALSE));
  else if (direction == BOBGUI_DIR_TAB_FORWARD)
    return FALSE;

  /* Other directions caught by the keybindings */
  return bobgui_widget_grab_focus (widget);
}

static gboolean
bobgui_tree_view_grab_focus (BobguiWidget *widget)
{
  if (!bobgui_widget_grab_focus_self (widget))
    return FALSE;

  bobgui_tree_view_focus_to_cursor (BOBGUI_TREE_VIEW (widget));
  return TRUE;
}

static void
bobgui_tree_view_css_changed (BobguiWidget         *widget,
                           BobguiCssStyleChange *change)
{
  BobguiTreeView *tree_view = BOBGUI_TREE_VIEW (widget);
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  GList *list;
  BobguiTreeViewColumn *column;

  BOBGUI_WIDGET_CLASS (bobgui_tree_view_parent_class)->css_changed (widget, change);

  if (bobgui_widget_get_realized (widget))
    {
      bobgui_tree_view_set_grid_lines (tree_view, priv->grid_lines);
      bobgui_tree_view_set_enable_tree_lines (tree_view, priv->tree_lines_enabled);
    }

  if (change == NULL || bobgui_css_style_change_affects (change, BOBGUI_CSS_AFFECTS_SIZE))
    {
      for (list = priv->columns; list; list = list->next)
	{
	  column = list->data;
	  _bobgui_tree_view_column_cell_set_dirty (column, TRUE);
	}

      priv->fixed_height = -1;
      bobgui_tree_rbtree_mark_invalid (priv->tree);
    }

  /* Invalidate expander size */
  priv->expander_size = -1;
}

static gboolean
bobgui_tree_view_real_move_cursor (BobguiTreeView       *tree_view,
				BobguiMovementStep    step,
				int                count,
                                gboolean           extend,
                                gboolean           modify)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  g_return_val_if_fail (BOBGUI_IS_TREE_VIEW (tree_view), FALSE);
  g_return_val_if_fail (step == BOBGUI_MOVEMENT_LOGICAL_POSITIONS ||
			step == BOBGUI_MOVEMENT_VISUAL_POSITIONS ||
			step == BOBGUI_MOVEMENT_DISPLAY_LINES ||
			step == BOBGUI_MOVEMENT_PAGES ||
			step == BOBGUI_MOVEMENT_BUFFER_ENDS, FALSE);

  if (priv->tree == NULL)
    return FALSE;
  if (!bobgui_widget_has_focus (BOBGUI_WIDGET (tree_view)))
    return FALSE;

  bobgui_tree_view_stop_editing (tree_view, FALSE);
  priv->draw_keyfocus = TRUE;
  bobgui_widget_grab_focus (BOBGUI_WIDGET (tree_view));

  priv->modify_selection_pressed = modify;
  priv->extend_selection_pressed = extend;

  switch (step)
    {
      /* currently we make no distinction.  When we go bi-di, we need to */
    case BOBGUI_MOVEMENT_LOGICAL_POSITIONS:
    case BOBGUI_MOVEMENT_VISUAL_POSITIONS:
      bobgui_tree_view_move_cursor_left_right (tree_view, count);
      break;
    case BOBGUI_MOVEMENT_DISPLAY_LINES:
      bobgui_tree_view_move_cursor_up_down (tree_view, count);
      break;
    case BOBGUI_MOVEMENT_PAGES:
      bobgui_tree_view_move_cursor_page_up_down (tree_view, count);
      break;
    case BOBGUI_MOVEMENT_BUFFER_ENDS:
      bobgui_tree_view_move_cursor_start_end (tree_view, count);
      break;
    case BOBGUI_MOVEMENT_WORDS:
    case BOBGUI_MOVEMENT_DISPLAY_LINE_ENDS:
    case BOBGUI_MOVEMENT_PARAGRAPHS:
    case BOBGUI_MOVEMENT_PARAGRAPH_ENDS:
    case BOBGUI_MOVEMENT_HORIZONTAL_PAGES:
    default:
      g_assert_not_reached ();
    }

  priv->modify_selection_pressed = FALSE;
  priv->extend_selection_pressed = FALSE;

  return TRUE;
}

static void
bobgui_tree_view_put (BobguiTreeView       *tree_view,
		   BobguiWidget         *child_widget,
                   BobguiTreePath       *path,
                   BobguiTreeViewColumn *column,
                   const BobguiBorder   *border)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  BobguiTreeViewChild *child;

  g_return_if_fail (BOBGUI_IS_TREE_VIEW (tree_view));
  g_return_if_fail (BOBGUI_IS_WIDGET (child_widget));

  child = g_slice_new (BobguiTreeViewChild);

  child->widget = child_widget;
  if (_bobgui_tree_view_find_node (tree_view,
				path,
				&child->tree,
				&child->node))
    {
      g_assert_not_reached ();
    }
  child->column = column;
  child->border = *border;

  priv->children = g_list_append (priv->children, child);

  bobgui_css_node_insert_after (bobgui_widget_get_css_node (BOBGUI_WIDGET (tree_view)),
                             bobgui_widget_get_css_node (child_widget),
                             priv->header_node);
  bobgui_widget_set_parent (child_widget, BOBGUI_WIDGET (tree_view));
}

/* TreeModel Callbacks
 */

static void
bobgui_tree_view_row_changed (BobguiTreeModel *model,
			   BobguiTreePath  *path,
			   BobguiTreeIter  *iter,
			   gpointer      data)
{
  BobguiTreeView *tree_view = (BobguiTreeView *)data;
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  BobguiTreeRBTree *tree;
  BobguiTreeRBNode *node;
  gboolean free_path = FALSE;
  GList *list;
  BobguiTreePath *cursor_path;

  g_return_if_fail (path != NULL || iter != NULL);

  if (priv->cursor_node != NULL)
    cursor_path = _bobgui_tree_path_new_from_rbtree (priv->cursor_tree,
                                                  priv->cursor_node);
  else
    cursor_path = NULL;

  if (priv->edited_column &&
      (cursor_path == NULL || bobgui_tree_path_compare (cursor_path, path) == 0))
    bobgui_tree_view_stop_editing (tree_view, TRUE);

  if (cursor_path != NULL)
    bobgui_tree_path_free (cursor_path);

  if (path == NULL)
    {
      path = bobgui_tree_model_get_path (model, iter);
      free_path = TRUE;
    }
  else if (iter == NULL)
    bobgui_tree_model_get_iter (model, iter, path);

  if (_bobgui_tree_view_find_node (tree_view,
				path,
				&tree,
				&node))
    /* We aren't actually showing the node */
    goto done;

  if (tree == NULL)
    goto done;

  if (priv->fixed_height_mode
      && priv->fixed_height >= 0)
    {
      bobgui_tree_rbtree_node_set_height (tree, node, priv->fixed_height);
      bobgui_widget_queue_draw (BOBGUI_WIDGET (tree_view));
    }
  else
    {
      bobgui_tree_rbtree_node_mark_invalid (tree, node);
      for (list = priv->columns; list; list = list->next)
        {
          BobguiTreeViewColumn *column;

          column = list->data;
          if (!bobgui_tree_view_column_get_visible (column))
            continue;

          if (bobgui_tree_view_column_get_sizing (column) == BOBGUI_TREE_VIEW_COLUMN_AUTOSIZE)
            {
              _bobgui_tree_view_column_cell_set_dirty (column, TRUE);
            }
        }
    }

 done:
  if (!priv->fixed_height_mode &&
      bobgui_widget_get_realized (BOBGUI_WIDGET (tree_view)))
    install_presize_handler (tree_view);
  if (free_path)
    bobgui_tree_path_free (path);
}

static void
bobgui_tree_view_row_inserted (BobguiTreeModel *model,
			    BobguiTreePath  *path,
			    BobguiTreeIter  *iter,
			    gpointer      data)
{
  BobguiTreeView *tree_view = (BobguiTreeView *) data;
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  int *indices;
  BobguiTreeRBTree *tree;
  BobguiTreeRBNode *tmpnode = NULL;
  int depth;
  int i = 0;
  int height;
  gboolean free_path = FALSE;

  g_return_if_fail (path != NULL || iter != NULL);

  if (priv->fixed_height_mode
      && priv->fixed_height >= 0)
    height = priv->fixed_height;
  else
    height = 0;

  if (path == NULL)
    {
      path = bobgui_tree_model_get_path (model, iter);
      free_path = TRUE;
    }
  else if (iter == NULL)
    bobgui_tree_model_get_iter (model, iter, path);

  if (priv->tree == NULL)
    priv->tree = bobgui_tree_rbtree_new ();

  tree = priv->tree;

  /* Update all row-references */
  bobgui_tree_row_reference_inserted (G_OBJECT (data), path);
  depth = bobgui_tree_path_get_depth (path);
  indices = bobgui_tree_path_get_indices (path);

  /* First, find the parent tree */
  while (i < depth - 1)
    {
      if (tree == NULL)
	{
	  /* We aren't showing the node */
          goto done;
	}

      tmpnode = bobgui_tree_rbtree_find_count (tree, indices[i] + 1);
      if (tmpnode == NULL)
	{
	  g_warning ("A node was inserted with a parent that's not in the tree.\n" \
		     "This possibly means that a BobguiTreeModel inserted a child node\n" \
		     "before the parent was inserted.");
          goto done;
	}
      else if (!BOBGUI_TREE_RBNODE_FLAG_SET (tmpnode, BOBGUI_TREE_RBNODE_IS_PARENT))
	{
          /* FIXME enforce correct behavior on model, probably */
	  /* In theory, the model should have emitted has_child_toggled here.  We
	   * try to catch it anyway, just to be safe, in case the model hasn't.
	   */
	  BobguiTreePath *tmppath = _bobgui_tree_path_new_from_rbtree (tree, tmpnode);
	  bobgui_tree_view_row_has_child_toggled (model, tmppath, NULL, data);
	  bobgui_tree_path_free (tmppath);
          goto done;
	}

      tree = tmpnode->children;
      i++;
    }

  if (tree == NULL)
    {
      goto done;
    }

  /* ref the node */
  bobgui_tree_model_ref_node (priv->model, iter);
  if (indices[depth - 1] == 0)
    {
      tmpnode = bobgui_tree_rbtree_find_count (tree, 1);
      tmpnode = bobgui_tree_rbtree_insert_before (tree, tmpnode, height, FALSE);
    }
  else
    {
      tmpnode = bobgui_tree_rbtree_find_count (tree, indices[depth - 1]);
      tmpnode = bobgui_tree_rbtree_insert_after (tree, tmpnode, height, FALSE);
    }

 done:
  if (height > 0)
    {
      if (tree)
        bobgui_tree_rbtree_node_mark_valid (tree, tmpnode);

      bobgui_widget_queue_resize (BOBGUI_WIDGET (tree_view));
    }
  else
    install_presize_handler (tree_view);
  if (free_path)
    bobgui_tree_path_free (path);
}

static void
bobgui_tree_view_row_has_child_toggled (BobguiTreeModel *model,
				     BobguiTreePath  *path,
				     BobguiTreeIter  *iter,
				     gpointer      data)
{
  BobguiTreeView *tree_view = (BobguiTreeView *)data;
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  BobguiTreeIter real_iter;
  gboolean has_child;
  BobguiTreeRBTree *tree;
  BobguiTreeRBNode *node;
  gboolean free_path = FALSE;

  g_return_if_fail (path != NULL || iter != NULL);

  if (iter)
    real_iter = *iter;

  if (path == NULL)
    {
      path = bobgui_tree_model_get_path (model, iter);
      free_path = TRUE;
    }
  else if (iter == NULL)
    bobgui_tree_model_get_iter (model, &real_iter, path);

  if (_bobgui_tree_view_find_node (tree_view,
				path,
				&tree,
				&node))
    /* We aren't actually showing the node */
    goto done;

  if (tree == NULL)
    goto done;

  has_child = bobgui_tree_model_iter_has_child (model, &real_iter);
  /* Sanity check.
   */
  if (BOBGUI_TREE_RBNODE_FLAG_SET (node, BOBGUI_TREE_RBNODE_IS_PARENT) == has_child)
    goto done;

  if (has_child)
    {
      BOBGUI_TREE_RBNODE_SET_FLAG (node, BOBGUI_TREE_RBNODE_IS_PARENT);
    }
  else
    {
      BOBGUI_TREE_RBNODE_UNSET_FLAG (node, BOBGUI_TREE_RBNODE_IS_PARENT);
    }

  if (has_child && priv->is_list)
    {
      priv->is_list = FALSE;
      if (priv->show_expanders)
	{
	  GList *list;

	  for (list = priv->columns; list; list = list->next)
	    if (bobgui_tree_view_column_get_visible (BOBGUI_TREE_VIEW_COLUMN (list->data)))
	      {
		_bobgui_tree_view_column_cell_set_dirty (BOBGUI_TREE_VIEW_COLUMN (list->data), TRUE);
		break;
	      }
	}
      bobgui_widget_queue_resize (BOBGUI_WIDGET (tree_view));
    }
  else
    {
      bobgui_widget_queue_draw (BOBGUI_WIDGET (tree_view));
    }

 done:
  if (free_path)
    bobgui_tree_path_free (path);
}

static void
check_selection_helper (BobguiTreeRBTree *tree,
                        BobguiTreeRBNode *node,
                        gpointer       data)
{
  int *value = (int *)data;

  *value |= BOBGUI_TREE_RBNODE_FLAG_SET (node, BOBGUI_TREE_RBNODE_IS_SELECTED);

  if (node->children && !*value)
    bobgui_tree_rbtree_traverse (node->children, node->children->root, G_POST_ORDER, check_selection_helper, data);
}

static void
bobgui_tree_view_row_deleted (BobguiTreeModel *model,
			   BobguiTreePath  *path,
			   gpointer      data)
{
  BobguiTreeView *tree_view = (BobguiTreeView *)data;
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  BobguiTreeRBTree *tree;
  BobguiTreeRBNode *node;
  GList *list;
  gboolean selection_changed = FALSE, cursor_changed = FALSE;
  BobguiTreeRBTree *cursor_tree = NULL;
  BobguiTreeRBNode *cursor_node = NULL;

  g_return_if_fail (path != NULL);

  bobgui_tree_row_reference_deleted (G_OBJECT (data), path);

  if (_bobgui_tree_view_find_node (tree_view, path, &tree, &node))
    return;

  if (tree == NULL)
    return;

  /* check if the selection has been changed */
  bobgui_tree_rbtree_traverse (tree, node, G_POST_ORDER,
                        check_selection_helper, &selection_changed);

  for (list = priv->columns; list; list = list->next)
    if (bobgui_tree_view_column_get_visible (BOBGUI_TREE_VIEW_COLUMN (list->data)) &&
	bobgui_tree_view_column_get_sizing (BOBGUI_TREE_VIEW_COLUMN (list->data)) == BOBGUI_TREE_VIEW_COLUMN_AUTOSIZE)
      _bobgui_tree_view_column_cell_set_dirty ((BobguiTreeViewColumn *)list->data, TRUE);

  /* Ensure we don't have a dangling pointer to a dead node */
  ensure_unprelighted (tree_view);

  /* Cancel editing if we've started */
  bobgui_tree_view_stop_editing (tree_view, TRUE);

  /* If the cursor row got deleted, move the cursor to the next row */
  if (priv->cursor_node &&
      (priv->cursor_node == node ||
       (node->children && (priv->cursor_tree == node->children ||
                           bobgui_tree_rbtree_contains (node->children, priv->cursor_tree)))))
    {
      BobguiTreePath *cursor_path;

      cursor_tree = tree;
      cursor_node = bobgui_tree_rbtree_next (tree, node);
      /* find the first node that is not going to be deleted */
      while (cursor_node == NULL && cursor_tree->parent_tree)
        {
          cursor_node = bobgui_tree_rbtree_next (cursor_tree->parent_tree,
                                          cursor_tree->parent_node);
          cursor_tree = cursor_tree->parent_tree;
        }

      if (cursor_node != NULL)
        cursor_path = _bobgui_tree_path_new_from_rbtree (cursor_tree, cursor_node);
      else
        cursor_path = NULL;

      if (cursor_path == NULL ||
          ! search_first_focusable_path (tree_view, &cursor_path, TRUE,
                                         &cursor_tree, &cursor_node))
        {
          /* It looks like we reached the end of the view without finding
           * a focusable row.  We will step backwards to find the last
           * focusable row.
           */
          bobgui_tree_rbtree_prev_full (tree, node, &cursor_tree, &cursor_node);
          if (cursor_node)
            {
              cursor_path = _bobgui_tree_path_new_from_rbtree (cursor_tree, cursor_node);
              if (! search_first_focusable_path (tree_view, &cursor_path, FALSE,
                                                 &cursor_tree, &cursor_node))
                cursor_node = NULL;
              bobgui_tree_path_free (cursor_path);
            }
        }
      else if (cursor_path)
        bobgui_tree_path_free (cursor_path);

      cursor_changed = TRUE;
    }

  if (tree->root->count == 1)
    {
      if (priv->tree == tree)
	priv->tree = NULL;

      bobgui_tree_rbtree_remove (tree);
    }
  else
    {
      bobgui_tree_rbtree_remove_node (tree, node);
    }

  if (! bobgui_tree_row_reference_valid (priv->top_row))
    {
      bobgui_tree_row_reference_free (priv->top_row);
      priv->top_row = NULL;
    }

  install_scroll_sync_handler (tree_view);

  bobgui_widget_queue_resize (BOBGUI_WIDGET (tree_view));

  if (cursor_changed)
    {
      if (cursor_node)
        {
          BobguiTreePath *cursor_path = _bobgui_tree_path_new_from_rbtree (cursor_tree, cursor_node);
          bobgui_tree_view_real_set_cursor (tree_view, cursor_path, CLEAR_AND_SELECT | CURSOR_INVALID);
          bobgui_tree_path_free (cursor_path);
        }
      else
        bobgui_tree_view_real_set_cursor (tree_view, NULL, CLEAR_AND_SELECT | CURSOR_INVALID);
    }
  if (selection_changed)
    g_signal_emit_by_name (priv->selection, "changed");
}

static void
bobgui_tree_view_rows_reordered (BobguiTreeModel *model,
			      BobguiTreePath  *parent,
			      BobguiTreeIter  *iter,
			      int          *new_order,
			      gpointer      data)
{
  BobguiTreeView *tree_view = BOBGUI_TREE_VIEW (data);
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  BobguiTreeRBTree *tree;
  BobguiTreeRBNode *node;
  int len;

  len = bobgui_tree_model_iter_n_children (model, iter);

  if (len < 2)
    return;

  bobgui_tree_row_reference_reordered (G_OBJECT (data),
				    parent,
				    iter,
				    new_order);

  if (_bobgui_tree_view_find_node (tree_view,
				parent,
				&tree,
				&node))
    return;

  /* We need to special case the parent path */
  if (tree == NULL)
    tree = priv->tree;
  else
    tree = node->children;

  if (tree == NULL)
    return;

  if (priv->edited_column)
    bobgui_tree_view_stop_editing (tree_view, TRUE);

  /* we need to be unprelighted */
  ensure_unprelighted (tree_view);

  bobgui_tree_rbtree_reorder (tree, new_order, len);

  bobgui_widget_queue_draw (BOBGUI_WIDGET (tree_view));

  bobgui_tree_view_dy_to_top_row (tree_view);
}


/* Internal tree functions
 */


static void
bobgui_tree_view_get_background_xrange (BobguiTreeView       *tree_view,
                                     BobguiTreeRBTree     *tree,
                                     BobguiTreeViewColumn *column,
                                     int               *x1,
                                     int               *x2)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  BobguiTreeViewColumn *tmp_column = NULL;
  int total_width;
  GList *list;
  gboolean rtl;

  if (x1)
    *x1 = 0;

  if (x2)
    *x2 = 0;

  rtl = (_bobgui_widget_get_direction (BOBGUI_WIDGET (tree_view)) == BOBGUI_TEXT_DIR_RTL);

  total_width = 0;
  for (list = (rtl ? g_list_last (priv->columns) : g_list_first (priv->columns));
       list;
       list = (rtl ? list->prev : list->next))
    {
      tmp_column = list->data;

      if (tmp_column == column)
        break;

      if (bobgui_tree_view_column_get_visible (tmp_column))
        total_width += bobgui_tree_view_column_get_width (tmp_column);
    }

  if (tmp_column != column)
    {
      g_warning (G_STRLOC": passed-in column isn't in the tree");
      return;
    }

  if (x1)
    *x1 = total_width;

  if (x2)
    {
      if (bobgui_tree_view_column_get_visible (column))
        *x2 = total_width + bobgui_tree_view_column_get_width (column);
      else
        *x2 = total_width; /* width of 0 */
    }
}

static void
bobgui_tree_view_get_arrow_xrange (BobguiTreeView   *tree_view,
				BobguiTreeRBTree *tree,
                                int           *x1,
                                int           *x2)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  int x_offset = 0;
  GList *list;
  BobguiTreeViewColumn *tmp_column = NULL;
  int total_width;
  int expander_size, expander_render_size;
  gboolean rtl;

  rtl = (_bobgui_widget_get_direction (BOBGUI_WIDGET (tree_view)) == BOBGUI_TEXT_DIR_RTL);
  expander_size = bobgui_tree_view_get_expander_size (tree_view);
  expander_render_size = expander_size - (_TREE_VIEW_HORIZONTAL_SEPARATOR / 2);

  total_width = 0;
  for (list = (rtl ? g_list_last (priv->columns) : g_list_first (priv->columns));
       list;
       list = (rtl ? list->prev : list->next))
    {
      tmp_column = list->data;

      if (bobgui_tree_view_is_expander_column (tree_view, tmp_column))
        {
	  if (rtl)
	    x_offset = total_width + bobgui_tree_view_column_get_width (tmp_column) - expander_size;
	  else
	    x_offset = total_width;
          break;
        }

      if (bobgui_tree_view_column_get_visible (tmp_column))
        total_width += bobgui_tree_view_column_get_width (tmp_column);
    }

  x_offset += (expander_size - expander_render_size);

  if (rtl)
    x_offset -= expander_size * bobgui_tree_rbtree_get_depth (tree);
  else
    x_offset += expander_size * bobgui_tree_rbtree_get_depth (tree);

  *x1 = x_offset;

  if (tmp_column &&
      bobgui_tree_view_column_get_visible (tmp_column))
    /* +1 because x2 isn't included in the range. */
    *x2 = *x1 + expander_render_size + 1;
  else
    *x2 = *x1;
}

static void
bobgui_tree_view_build_tree (BobguiTreeView   *tree_view,
			  BobguiTreeRBTree *tree,
			  BobguiTreeIter   *iter,
			  int            depth,
			  gboolean       recurse)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  BobguiTreeRBNode *temp = NULL;
  BobguiTreePath *path = NULL;

  do
    {
      bobgui_tree_model_ref_node (priv->model, iter);
      temp = bobgui_tree_rbtree_insert_after (tree, temp, 0, FALSE);

      if (priv->fixed_height > 0)
        {
          if (BOBGUI_TREE_RBNODE_FLAG_SET (temp, BOBGUI_TREE_RBNODE_INVALID))
	    {
              bobgui_tree_rbtree_node_set_height (tree, temp, priv->fixed_height);
	      bobgui_tree_rbtree_node_mark_valid (tree, temp);
	    }
        }

      if (priv->is_list)
        continue;

      if (recurse)
	{
	  BobguiTreeIter child;

	  if (!path)
	    path = bobgui_tree_model_get_path (priv->model, iter);
	  else
	    bobgui_tree_path_next (path);

	  if (bobgui_tree_model_iter_has_child (priv->model, iter))
	    {
	      gboolean expand;

	      g_signal_emit (tree_view, tree_view_signals[TEST_EXPAND_ROW], 0, iter, path, &expand);

	      if (bobgui_tree_model_iter_children (priv->model, &child, iter)
		  && !expand)
	        {
	          temp->children = bobgui_tree_rbtree_new ();
	          temp->children->parent_tree = tree;
	          temp->children->parent_node = temp;
	          bobgui_tree_view_build_tree (tree_view, temp->children, &child, depth + 1, recurse);
		}
	    }
	}

      if (bobgui_tree_model_iter_has_child (priv->model, iter))
	{
	  if ((temp->flags&BOBGUI_TREE_RBNODE_IS_PARENT) != BOBGUI_TREE_RBNODE_IS_PARENT)
	    temp->flags ^= BOBGUI_TREE_RBNODE_IS_PARENT;
	}
    }
  while (bobgui_tree_model_iter_next (priv->model, iter));

  if (path)
    bobgui_tree_path_free (path);
}

/* Make sure the node is visible vertically */
static void
bobgui_tree_view_clamp_node_visible (BobguiTreeView   *tree_view,
				  BobguiTreeRBTree *tree,
				  BobguiTreeRBNode *node)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  int node_dy, height;
  BobguiTreePath *path = NULL;

  if (!bobgui_widget_get_realized (BOBGUI_WIDGET (tree_view)))
    return;

  /* just return if the node is visible, avoiding a costly expose */
  node_dy = bobgui_tree_rbtree_node_find_offset (tree, node);
  height = bobgui_tree_view_get_row_height (tree_view, node);
  if (! BOBGUI_TREE_RBNODE_FLAG_SET (node, BOBGUI_TREE_RBNODE_INVALID)
      && node_dy >= bobgui_adjustment_get_value (priv->vadjustment)
      && node_dy + height <= (bobgui_adjustment_get_value (priv->vadjustment)
                              + bobgui_adjustment_get_page_size (priv->vadjustment)))
    return;

  path = _bobgui_tree_path_new_from_rbtree (tree, node);
  if (path)
    {
      bobgui_tree_view_scroll_to_cell (tree_view, path, NULL, FALSE, 0.0, 0.0);
      bobgui_tree_path_free (path);
    }
}

static void
bobgui_tree_view_clamp_column_visible (BobguiTreeView       *tree_view,
				    BobguiTreeViewColumn *column,
				    gboolean           focus_to_cell)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  BobguiAllocation allocation;
  int x, width;

  if (column == NULL)
    return;

  bobgui_widget_get_allocation (bobgui_tree_view_column_get_button (column), &allocation);
  x = allocation.x;
  width = allocation.width;

  if (width > bobgui_adjustment_get_page_size (priv->hadjustment))
    {
      /* The column is larger than the horizontal page size.  If the
       * column has cells which can be focused individually, then we make
       * sure the cell which gets focus is fully visible (if even the
       * focus cell is bigger than the page size, we make sure the
       * left-hand side of the cell is visible).
       *
       * If the column does not have an activatable cell, we
       * make sure the left-hand side of the column is visible.
       */

      if (focus_to_cell && bobgui_tree_view_has_can_focus_cell (tree_view))
        {
          BobguiCellArea *cell_area;
          BobguiCellRenderer *focus_cell;

          cell_area = bobgui_cell_layout_get_area (BOBGUI_CELL_LAYOUT (column));
          focus_cell = bobgui_cell_area_get_focus_cell (cell_area);

          if (bobgui_tree_view_column_cell_get_position (column, focus_cell,
                                                      &x, &width))
            {
              if (width < bobgui_adjustment_get_page_size (priv->hadjustment))
                {
                  if (bobgui_adjustment_get_value (priv->hadjustment) + bobgui_adjustment_get_page_size (priv->hadjustment) < x + width)
                    bobgui_adjustment_set_value (priv->hadjustment,
                                              x + width - bobgui_adjustment_get_page_size (priv->hadjustment));
                  else if (bobgui_adjustment_get_value (priv->hadjustment) > x)
                    bobgui_adjustment_set_value (priv->hadjustment, x);
                }
            }
        }

      bobgui_adjustment_set_value (priv->hadjustment, x);
    }
  else
    {
      if ((bobgui_adjustment_get_value (priv->hadjustment) + bobgui_adjustment_get_page_size (priv->hadjustment)) < (x + width))
	  bobgui_adjustment_set_value (priv->hadjustment,
				    x + width - bobgui_adjustment_get_page_size (priv->hadjustment));
      else if (bobgui_adjustment_get_value (priv->hadjustment) > x)
	bobgui_adjustment_set_value (priv->hadjustment, x);
  }
}

/* This function could be more efficient.  I'll optimize it if profiling seems
 * to imply that it is important */
BobguiTreePath *
_bobgui_tree_path_new_from_rbtree (BobguiTreeRBTree *tree,
			        BobguiTreeRBNode *node)
{
  BobguiTreePath *path;
  BobguiTreeRBTree *tmp_tree;
  BobguiTreeRBNode *tmp_node, *last;
  int count;

  path = bobgui_tree_path_new ();

  g_return_val_if_fail (node != NULL, path);

  count = 1 + node->left->count;

  last = node;
  tmp_node = node->parent;
  tmp_tree = tree;
  while (tmp_tree)
    {
      while (!bobgui_tree_rbtree_is_nil (tmp_node))
	{
	  if (tmp_node->right == last)
	    count += 1 + tmp_node->left->count;
	  last = tmp_node;
	  tmp_node = tmp_node->parent;
	}
      bobgui_tree_path_prepend_index (path, count - 1);
      last = tmp_tree->parent_node;
      tmp_tree = tmp_tree->parent_tree;
      if (last)
	{
	  count = 1 + last->left->count;
	  tmp_node = last->parent;
	}
    }
  return path;
}

/* Returns TRUE if we ran out of tree before finding the path.  If the path is
 * invalid (ie. points to a node that’s not in the tree), *tree and *node are
 * both set to NULL.
 */
gboolean
_bobgui_tree_view_find_node (BobguiTreeView    *tree_view,
			  BobguiTreePath    *path,
			  BobguiTreeRBTree **tree,
			  BobguiTreeRBNode **node)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  BobguiTreeRBNode *tmpnode = NULL;
  BobguiTreeRBTree *tmptree = priv->tree;
  int *indices = bobgui_tree_path_get_indices (path);
  int depth = bobgui_tree_path_get_depth (path);
  int i = 0;

  *node = NULL;
  *tree = NULL;

  if (depth == 0 || tmptree == NULL)
    return FALSE;
  do
    {
      tmpnode = bobgui_tree_rbtree_find_count (tmptree, indices[i] + 1);
      ++i;
      if (tmpnode == NULL)
	{
	  *tree = NULL;
	  *node = NULL;
	  return FALSE;
	}
      if (i >= depth)
	{
	  *tree = tmptree;
	  *node = tmpnode;
	  return FALSE;
	}
      *tree = tmptree;
      *node = tmpnode;
      tmptree = tmpnode->children;
      if (tmptree == NULL)
	return TRUE;
    }
  while (1);
}

static gboolean
bobgui_tree_view_is_expander_column (BobguiTreeView       *tree_view,
				  BobguiTreeViewColumn *column)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  GList *list;

  if (priv->is_list)
    return FALSE;

  if (priv->expander_column != NULL)
    {
      if (priv->expander_column == column)
	return TRUE;
      return FALSE;
    }
  else
    {
      for (list = priv->columns;
	   list;
	   list = list->next)
	if (bobgui_tree_view_column_get_visible (BOBGUI_TREE_VIEW_COLUMN (list->data)))
	  break;
      if (list && list->data == column)
	return TRUE;
    }
  return FALSE;
}

static inline gboolean
bobgui_tree_view_draw_expanders (BobguiTreeView *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  if (!priv->is_list && priv->show_expanders)
    return TRUE;
  /* else */
  return FALSE;
}

static void
bobgui_tree_view_add_move_binding (BobguiWidgetClass *widget_class,
				guint           keyval,
				guint           modmask,
				gboolean        add_shifted_binding,
				BobguiMovementStep step,
				int             count)
{
  bobgui_widget_class_add_binding_signal (widget_class,
                                       keyval, modmask,
                                       "move-cursor",
                                       "(iibb)", step, count, FALSE, FALSE);

  if (add_shifted_binding)
    bobgui_widget_class_add_binding_signal (widget_class,
                                         keyval, GDK_SHIFT_MASK,
                                         "move-cursor",
                                         "(iibb)", step, count, TRUE, FALSE);

  if ((modmask & GDK_CONTROL_MASK) == GDK_CONTROL_MASK)
   return;

  bobgui_widget_class_add_binding_signal (widget_class,
                                       keyval, GDK_CONTROL_MASK,
                                       "move-cursor",
                                       "(iibb)", step, count, FALSE, TRUE);

  if (add_shifted_binding)
    bobgui_widget_class_add_binding_signal (widget_class, keyval,
                                         GDK_CONTROL_MASK | GDK_SHIFT_MASK,
                                         "move-cursor",
                                         "(iibb)", step, count, TRUE, TRUE);
}

static int
bobgui_tree_view_unref_tree_helper (BobguiTreeModel  *model,
				 BobguiTreeIter   *iter,
				 BobguiTreeRBTree *tree,
				 BobguiTreeRBNode *node)
{
  int retval = FALSE;
  do
    {
      g_return_val_if_fail (node != NULL, FALSE);

      if (node->children)
	{
	  BobguiTreeIter child;
	  BobguiTreeRBTree *new_tree;
	  BobguiTreeRBNode *new_node;

	  new_tree = node->children;
          new_node = bobgui_tree_rbtree_first (new_tree);

	  if (!bobgui_tree_model_iter_children (model, &child, iter))
	    return FALSE;

	  retval = bobgui_tree_view_unref_tree_helper (model, &child, new_tree, new_node) | retval;
	}

      if (BOBGUI_TREE_RBNODE_FLAG_SET (node, BOBGUI_TREE_RBNODE_IS_SELECTED))
	retval = TRUE;
      bobgui_tree_model_unref_node (model, iter);
      node = bobgui_tree_rbtree_next (tree, node);
    }
  while (bobgui_tree_model_iter_next (model, iter));

  return retval;
}

static int
bobgui_tree_view_unref_and_check_selection_tree (BobguiTreeView   *tree_view,
					      BobguiTreeRBTree *tree)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  BobguiTreeIter iter;
  BobguiTreePath *path;
  BobguiTreeRBNode *node;
  int retval;

  if (!tree)
    return FALSE;

  node = bobgui_tree_rbtree_first (tree);

  g_return_val_if_fail (node != NULL, FALSE);
  path = _bobgui_tree_path_new_from_rbtree (tree, node);
  bobgui_tree_model_get_iter (BOBGUI_TREE_MODEL (priv->model),
			   &iter, path);
  retval = bobgui_tree_view_unref_tree_helper (BOBGUI_TREE_MODEL (priv->model), &iter, tree, node);
  bobgui_tree_path_free (path);

  return retval;
}

static void
bobgui_tree_view_set_column_drag_info (BobguiTreeView       *tree_view,
				    BobguiTreeViewColumn *column)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  BobguiTreeViewColumn *left_column;
  BobguiTreeViewColumn *cur_column = NULL;
  BobguiTreeViewColumnReorder *reorder;
  gboolean rtl;
  GList *tmp_list;
  int left;

  /* We want to precalculate the motion list such that we know what column slots
   * are available.
   */
  left_column = NULL;
  rtl = (bobgui_widget_get_direction (BOBGUI_WIDGET (tree_view)) == BOBGUI_TEXT_DIR_RTL);

  /* First, identify all possible drop spots */
  if (rtl)
    tmp_list = g_list_last (priv->columns);
  else
    tmp_list = g_list_first (priv->columns);

  while (tmp_list)
    {
      cur_column = BOBGUI_TREE_VIEW_COLUMN (tmp_list->data);
      tmp_list = rtl ? tmp_list->prev : tmp_list->next;

      if (bobgui_tree_view_column_get_visible (cur_column) == FALSE)
	continue;

      /* If it's not the column moving and func tells us to skip over the column, we continue. */
      if (left_column != column && cur_column != column &&
	  priv->column_drop_func &&
	  ! priv->column_drop_func (tree_view, column, left_column, cur_column, priv->column_drop_func_data))
	{
	  left_column = cur_column;
	  continue;
	}
      reorder = g_slice_new0 (BobguiTreeViewColumnReorder);
      reorder->left_column = left_column;
      left_column = reorder->right_column = cur_column;

      priv->column_drag_info = g_list_append (priv->column_drag_info, reorder);
    }

  /* Add the last one */
  if (priv->column_drop_func == NULL ||
      ((left_column != column) &&
       priv->column_drop_func (tree_view, column, left_column, NULL, priv->column_drop_func_data)))
    {
      reorder = g_slice_new0 (BobguiTreeViewColumnReorder);
      reorder->left_column = left_column;
      reorder->right_column = NULL;
      priv->column_drag_info = g_list_append (priv->column_drag_info, reorder);
    }

  /* We quickly check to see if it even makes sense to reorder columns. */
  /* If there is nothing that can be moved, then we return */

  if (priv->column_drag_info == NULL)
    return;

  /* We know there are always 2 slots possible, as you can always return column. */
  /* If that's all there is, return */
  if (priv->column_drag_info->next == NULL ||
      (priv->column_drag_info->next->next == NULL &&
       ((BobguiTreeViewColumnReorder *)priv->column_drag_info->data)->right_column == column &&
       ((BobguiTreeViewColumnReorder *)priv->column_drag_info->next->data)->left_column == column))
    {
      for (tmp_list = priv->column_drag_info; tmp_list; tmp_list = tmp_list->next)
	g_slice_free (BobguiTreeViewColumnReorder, tmp_list->data);
      g_list_free (priv->column_drag_info);
      priv->column_drag_info = NULL;
      return;
    }
  /* We fill in the ranges for the columns, now that we've isolated them */
  left = - TREE_VIEW_COLUMN_DRAG_DEAD_MULTIPLIER (tree_view);

  for (tmp_list = priv->column_drag_info; tmp_list; tmp_list = tmp_list->next)
    {
      reorder = (BobguiTreeViewColumnReorder *) tmp_list->data;

      reorder->left_align = left;
      if (tmp_list->next != NULL)
	{
          BobguiAllocation right_allocation, left_allocation;
	  BobguiWidget    *left_button, *right_button;

	  g_assert (tmp_list->next->data);

	  right_button = bobgui_tree_view_column_get_button (reorder->right_column);
	  left_button  = bobgui_tree_view_column_get_button
	    (((BobguiTreeViewColumnReorder *)tmp_list->next->data)->left_column);

          bobgui_widget_get_allocation (right_button, &right_allocation);
          bobgui_widget_get_allocation (left_button, &left_allocation);
	  left = reorder->right_align = (right_allocation.x + right_allocation.width + left_allocation.x) / 2;
	}
      else
	{
	  reorder->right_align = bobgui_widget_get_allocated_width (BOBGUI_WIDGET (tree_view))
                                 + TREE_VIEW_COLUMN_DRAG_DEAD_MULTIPLIER (tree_view);
	}
    }
}

void
_bobgui_tree_view_column_start_drag (BobguiTreeView       *tree_view,
				  BobguiTreeViewColumn *column,
                                  GdkDevice         *device)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  BobguiAllocation button_allocation;
  BobguiWidget *button;
  BobguiStyleContext *context;

  g_return_if_fail (priv->column_drag_info == NULL);
  g_return_if_fail (priv->cur_reorder == NULL);

  bobgui_tree_view_set_column_drag_info (tree_view, column);

  if (priv->column_drag_info == NULL)
    return;

  button = bobgui_tree_view_column_get_button (column);

  context = bobgui_widget_get_style_context (button);
  bobgui_style_context_add_class (context, "dnd");

  bobgui_widget_get_allocation (button, &button_allocation);
  priv->drag_column_x = button_allocation.x;
  priv->drag_column_y = button_allocation.y;

  priv->drag_column = column;

  bobgui_widget_grab_focus (BOBGUI_WIDGET (tree_view));

  priv->in_column_drag = TRUE;

  bobgui_gesture_set_state (priv->column_drag_gesture,
                         BOBGUI_EVENT_SEQUENCE_CLAIMED);
}

static inline int
bobgui_tree_view_get_effective_header_height (BobguiTreeView *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  if (priv->headers_visible)
    return priv->header_height;
  else
    return 0;
}

void
_bobgui_tree_view_get_row_separator_func (BobguiTreeView                 *tree_view,
				       BobguiTreeViewRowSeparatorFunc *func,
				       gpointer                    *data)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  *func = priv->row_separator_func;
  *data = priv->row_separator_data;
}

BobguiTreePath *
_bobgui_tree_view_get_anchor_path (BobguiTreeView *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  if (priv->anchor)
    return bobgui_tree_row_reference_get_path (priv->anchor);

  return NULL;
}

void
_bobgui_tree_view_set_anchor_path (BobguiTreeView *tree_view,
				BobguiTreePath *anchor_path)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  if (priv->anchor)
    {
      bobgui_tree_row_reference_free (priv->anchor);
      priv->anchor = NULL;
    }

  if (anchor_path && priv->model)
    priv->anchor =
      bobgui_tree_row_reference_new (priv->model, anchor_path);
}

BobguiTreeRBTree *
_bobgui_tree_view_get_rbtree (BobguiTreeView *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  return priv->tree;
}

gboolean
_bobgui_tree_view_get_cursor_node (BobguiTreeView    *tree_view,
                                BobguiTreeRBTree **tree,
                                BobguiTreeRBNode **node)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  if (priv->cursor_node == NULL)
    return FALSE;

  *tree = priv->cursor_tree;
  *node = priv->cursor_node;

  return TRUE;
}

BobguiTreeViewColumn *
_bobgui_tree_view_get_focus_column (BobguiTreeView *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  return priv->focus_column;
}

void
_bobgui_tree_view_set_focus_column (BobguiTreeView       *tree_view,
				 BobguiTreeViewColumn *column)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  BobguiTreeViewColumn *old_column = priv->focus_column;

  if (old_column == column)
    return;

  priv->focus_column = column;
}

/* x and y are the mouse position
 */
static void
bobgui_tree_view_snapshot_arrow (BobguiTreeView   *tree_view,
                              BobguiSnapshot   *snapshot,
                              BobguiTreeRBTree *tree,
                              BobguiTreeRBNode *node)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  GdkRectangle area;
  BobguiStateFlags state = 0;
  BobguiStyleContext *context;
  BobguiWidget *widget;
  int x_offset = 0;
  int x2;
  BobguiCellRendererState flags = 0;

  widget = BOBGUI_WIDGET (tree_view);
  context = bobgui_widget_get_style_context (widget);

  if (! BOBGUI_TREE_RBNODE_FLAG_SET (node, BOBGUI_TREE_RBNODE_IS_PARENT))
    return;

  bobgui_tree_view_get_arrow_xrange (tree_view, tree, &x_offset, &x2);

  area.x = x_offset;
  area.y = bobgui_tree_view_get_cell_area_y_offset (tree_view, tree, node);
  area.width = x2 - x_offset;
  area.height = bobgui_tree_view_get_cell_area_height (tree_view, node);

  if (BOBGUI_TREE_RBNODE_FLAG_SET (node, BOBGUI_TREE_RBNODE_IS_SELECTED))
    flags |= BOBGUI_CELL_RENDERER_SELECTED;

  if (node == priv->prelight_node &&
      priv->arrow_prelit)
    flags |= BOBGUI_CELL_RENDERER_PRELIT;

  state = bobgui_cell_renderer_get_state (NULL, widget, flags);

  if (node->children != NULL)
    state |= BOBGUI_STATE_FLAG_CHECKED;
  else
    state &= ~(BOBGUI_STATE_FLAG_CHECKED);

  bobgui_style_context_save (context);

  bobgui_style_context_set_state (context, state);
  bobgui_style_context_add_class (context, "expander");

  bobgui_snapshot_save (snapshot);
  bobgui_snapshot_translate (snapshot, &GRAPHENE_POINT_INIT (area.x, area.y));
  bobgui_css_style_snapshot_icon (bobgui_style_context_lookup_style (context), snapshot,
                               area.width, area.height);
  bobgui_snapshot_restore (snapshot);

  bobgui_style_context_restore (context);
}

static void
bobgui_tree_view_focus_to_cursor (BobguiTreeView *tree_view)

{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  BobguiTreePath *cursor_path;

  if ((priv->tree == NULL) ||
      (! bobgui_widget_get_realized (BOBGUI_WIDGET (tree_view))))
    return;

  cursor_path = NULL;
  if (priv->cursor_node)
    cursor_path = _bobgui_tree_path_new_from_rbtree (priv->cursor_tree,
                                                  priv->cursor_node);

  if (cursor_path == NULL)
    {
      /* Consult the selection before defaulting to the
       * first focusable element
       */
      GList *selected_rows;
      BobguiTreeModel *model;
      BobguiTreeSelection *selection;

      selection = bobgui_tree_view_get_selection (tree_view);
      selected_rows = bobgui_tree_selection_get_selected_rows (selection, &model);

      if (selected_rows)
	{
          cursor_path = bobgui_tree_path_copy((const BobguiTreePath *)(selected_rows->data));
	  g_list_free_full (selected_rows, (GDestroyNotify) bobgui_tree_path_free);
        }
      else
	{
	  cursor_path = bobgui_tree_path_new_first ();
	  search_first_focusable_path (tree_view, &cursor_path,
				       TRUE, NULL, NULL);
	}

      if (cursor_path)
	{
	  if (bobgui_tree_selection_get_mode (priv->selection) == BOBGUI_SELECTION_MULTIPLE)
	    bobgui_tree_view_real_set_cursor (tree_view, cursor_path, 0);
	  else
	    bobgui_tree_view_real_set_cursor (tree_view, cursor_path, CLEAR_AND_SELECT);
	}
    }

  if (cursor_path)
    {
      priv->draw_keyfocus = TRUE;

      bobgui_widget_queue_draw (BOBGUI_WIDGET (tree_view));
      bobgui_tree_path_free (cursor_path);

      if (priv->focus_column == NULL)
	{
	  GList *list;
	  for (list = priv->columns; list; list = list->next)
	    {
	      if (bobgui_tree_view_column_get_visible (BOBGUI_TREE_VIEW_COLUMN (list->data)))
		{
		  BobguiCellArea *cell_area;

                  _bobgui_tree_view_set_focus_column (tree_view, BOBGUI_TREE_VIEW_COLUMN (list->data));

		  /* This happens when the treeview initially grabs focus and there
		   * is no column in focus, here we explicitly focus into the first cell */
		  cell_area = bobgui_cell_layout_get_area (BOBGUI_CELL_LAYOUT (priv->focus_column));
		  if (!bobgui_cell_area_get_focus_cell (cell_area))
                    {
                      gboolean rtl;

                      rtl = (_bobgui_widget_get_direction (BOBGUI_WIDGET (tree_view)) == BOBGUI_TEXT_DIR_RTL);
                      bobgui_cell_area_focus (cell_area,
                                           rtl ? BOBGUI_DIR_LEFT : BOBGUI_DIR_RIGHT);
                    }

		  break;
		}
	    }
	}
    }
}

static void
bobgui_tree_view_move_cursor_up_down (BobguiTreeView *tree_view,
				   int          count)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  int selection_count;
  BobguiTreeRBTree *new_cursor_tree = NULL;
  BobguiTreeRBNode *new_cursor_node = NULL;
  BobguiTreePath *cursor_path = NULL;
  gboolean selectable;
  BobguiDirectionType direction;
  BobguiCellArea *cell_area = NULL;
  BobguiCellRenderer *last_focus_cell = NULL;
  BobguiTreeIter iter;

  if (priv->cursor_node == NULL)
    return;

  cursor_path = _bobgui_tree_path_new_from_rbtree (priv->cursor_tree,
                                                priv->cursor_node);

  direction = count < 0 ? BOBGUI_DIR_UP : BOBGUI_DIR_DOWN;

  if (priv->focus_column)
    cell_area = bobgui_cell_layout_get_area (BOBGUI_CELL_LAYOUT (priv->focus_column));

  /* If focus stays in the area for this row, then just return for this round */
  if (cell_area && (count == -1 || count == 1) &&
      bobgui_tree_model_get_iter (priv->model, &iter, cursor_path))
    {
      bobgui_tree_view_column_cell_set_cell_data (priv->focus_column,
					       priv->model,
                                               &iter,
                                               BOBGUI_TREE_RBNODE_FLAG_SET (priv->cursor_node, BOBGUI_TREE_RBNODE_IS_PARENT),
					       priv->cursor_node->children ? TRUE : FALSE);

      /* Save the last cell that had focus, if we hit the end of the view we'll give
       * focus back to it. */
      last_focus_cell = bobgui_cell_area_get_focus_cell (cell_area);

      /* If focus stays in the area, no need to change the cursor row */
      if (bobgui_cell_area_focus (cell_area, direction))
	return;
    }

  selection_count = bobgui_tree_selection_count_selected_rows (priv->selection);
  selectable = _bobgui_tree_selection_row_is_selectable (priv->selection,
						      priv->cursor_node,
						      cursor_path);

  if (selection_count == 0
      && bobgui_tree_selection_get_mode (priv->selection) != BOBGUI_SELECTION_NONE
      && !priv->modify_selection_pressed
      && selectable)
    {
      /* Don't move the cursor, but just select the current node */
      new_cursor_tree = priv->cursor_tree;
      new_cursor_node = priv->cursor_node;
    }
  else
    {
      if (count == -1)
	bobgui_tree_rbtree_prev_full (priv->cursor_tree, priv->cursor_node,
			       &new_cursor_tree, &new_cursor_node);
      else
	bobgui_tree_rbtree_next_full (priv->cursor_tree, priv->cursor_node,
			       &new_cursor_tree, &new_cursor_node);
    }

  bobgui_tree_path_free (cursor_path);

  if (new_cursor_node)
    {
      cursor_path = _bobgui_tree_path_new_from_rbtree (new_cursor_tree, new_cursor_node);

      search_first_focusable_path (tree_view, &cursor_path,
				   (count != -1),
				   &new_cursor_tree,
				   &new_cursor_node);

      if (cursor_path)
	bobgui_tree_path_free (cursor_path);
    }

  /*
   * If the list has only one item and multi-selection is set then select
   * the row (if not yet selected).
   */
  if (bobgui_tree_selection_get_mode (priv->selection) == BOBGUI_SELECTION_MULTIPLE &&
      new_cursor_node == NULL)
    {
      if (count == -1)
        bobgui_tree_rbtree_next_full (priv->cursor_tree, priv->cursor_node,
    			       &new_cursor_tree, &new_cursor_node);
      else
        bobgui_tree_rbtree_prev_full (priv->cursor_tree, priv->cursor_node,
			       &new_cursor_tree, &new_cursor_node);

      if (new_cursor_node == NULL
	  && !BOBGUI_TREE_RBNODE_FLAG_SET (priv->cursor_node, BOBGUI_TREE_RBNODE_IS_SELECTED))
        {
          new_cursor_node = priv->cursor_node;
          new_cursor_tree = priv->cursor_tree;
        }
      else
        {
          new_cursor_tree = NULL;
          new_cursor_node = NULL;
        }
    }

  if (new_cursor_node)
    {
      cursor_path = _bobgui_tree_path_new_from_rbtree (new_cursor_tree, new_cursor_node);
      bobgui_tree_view_real_set_cursor (tree_view, cursor_path, CLEAR_AND_SELECT | CLAMP_NODE);
      bobgui_tree_path_free (cursor_path);

      /* Give focus to the area in the new row */
      if (cell_area)
	bobgui_cell_area_focus (cell_area, direction);
    }
  else
    {
      bobgui_tree_view_clamp_node_visible (tree_view,
                                        priv->cursor_tree,
                                        priv->cursor_node);

      if (!priv->extend_selection_pressed)
        {
          if (! bobgui_widget_keynav_failed (BOBGUI_WIDGET (tree_view),
                                          count < 0 ?
                                          BOBGUI_DIR_UP : BOBGUI_DIR_DOWN))
            {
              BobguiWidget *toplevel = BOBGUI_WIDGET (bobgui_widget_get_root (BOBGUI_WIDGET (tree_view)));

              if (toplevel)
                bobgui_widget_child_focus (toplevel,
                                        count < 0 ?
                                        BOBGUI_DIR_TAB_BACKWARD :
                                        BOBGUI_DIR_TAB_FORWARD);
            }
        }
      else
        {
          bobgui_widget_error_bell (BOBGUI_WIDGET (tree_view));
        }

      if (cell_area)
	bobgui_cell_area_set_focus_cell (cell_area, last_focus_cell);
    }
}

static void
bobgui_tree_view_move_cursor_page_up_down (BobguiTreeView *tree_view,
					int          count)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  BobguiTreePath *old_cursor_path = NULL;
  BobguiTreePath *cursor_path = NULL;
  BobguiTreeRBTree *start_cursor_tree = NULL;
  BobguiTreeRBNode *start_cursor_node = NULL;
  BobguiTreeRBTree *cursor_tree;
  BobguiTreeRBNode *cursor_node;
  int y;
  int window_y;

  if (!bobgui_widget_has_focus (BOBGUI_WIDGET (tree_view)))
    return;

  if (priv->cursor_node == NULL)
    return;

  old_cursor_path = _bobgui_tree_path_new_from_rbtree (priv->cursor_tree,
                                                    priv->cursor_node);

  y = bobgui_tree_rbtree_node_find_offset (priv->cursor_tree, priv->cursor_node);
  window_y = RBTREE_Y_TO_TREE_WINDOW_Y (priv, y);
  y += priv->cursor_offset;
  y += count * (int)bobgui_adjustment_get_page_increment (priv->vadjustment);
  y = CLAMP (y, (int)bobgui_adjustment_get_lower (priv->vadjustment),  (int)bobgui_adjustment_get_upper (priv->vadjustment));

  if (y >= bobgui_tree_view_get_height (tree_view))
    y = bobgui_tree_view_get_height (tree_view) - 1;

  priv->cursor_offset =
    bobgui_tree_rbtree_find_offset (priv->tree, y,
			     &cursor_tree, &cursor_node);

  if (cursor_tree == NULL)
    {
      /* FIXME: we lost the cursor.  Should we try to get one? */
      bobgui_tree_path_free (old_cursor_path);
      return;
    }

  if (priv->cursor_offset
      > bobgui_tree_view_get_row_height (tree_view, cursor_node))
    {
      bobgui_tree_rbtree_next_full (cursor_tree, cursor_node,
			     &cursor_tree, &cursor_node);
      priv->cursor_offset -= bobgui_tree_view_get_row_height (tree_view, cursor_node);
    }

  y -= priv->cursor_offset;
  cursor_path = _bobgui_tree_path_new_from_rbtree (cursor_tree, cursor_node);

  start_cursor_tree = cursor_tree;
  start_cursor_node = cursor_node;

  if (! search_first_focusable_path (tree_view, &cursor_path,
				     (count != -1),
				     &cursor_tree, &cursor_node))
    {
      /* It looks like we reached the end of the view without finding
       * a focusable row.  We will step backwards to find the last
       * focusable row.
       */
      cursor_tree = start_cursor_tree;
      cursor_node = start_cursor_node;
      cursor_path = _bobgui_tree_path_new_from_rbtree (cursor_tree, cursor_node);

      search_first_focusable_path (tree_view, &cursor_path,
				   (count == -1),
				   &cursor_tree, &cursor_node);
    }

  if (!cursor_path)
    goto cleanup;

  /* update y */
  y = bobgui_tree_rbtree_node_find_offset (cursor_tree, cursor_node);

  bobgui_tree_view_real_set_cursor (tree_view, cursor_path, CLEAR_AND_SELECT);

  y -= window_y;
  bobgui_tree_view_scroll_to_point (tree_view, -1, y);
  bobgui_tree_view_clamp_node_visible (tree_view, cursor_tree, cursor_node);
  bobgui_widget_queue_draw (BOBGUI_WIDGET (tree_view));

  if (!bobgui_tree_path_compare (old_cursor_path, cursor_path))
    bobgui_widget_error_bell (BOBGUI_WIDGET (tree_view));

  bobgui_widget_grab_focus (BOBGUI_WIDGET (tree_view));

cleanup:
  bobgui_tree_path_free (old_cursor_path);
  bobgui_tree_path_free (cursor_path);
}

static void
bobgui_tree_view_move_cursor_left_right (BobguiTreeView *tree_view,
				      int          count)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  BobguiTreePath *cursor_path = NULL;
  BobguiTreeViewColumn *column;
  BobguiTreeIter iter;
  GList *list;
  gboolean found_column = FALSE;
  gboolean rtl;
  BobguiDirectionType direction;
  BobguiCellArea     *cell_area;
  BobguiCellRenderer *last_focus_cell = NULL;
  BobguiCellArea     *last_focus_area = NULL;

  rtl = (bobgui_widget_get_direction (BOBGUI_WIDGET (tree_view)) == BOBGUI_TEXT_DIR_RTL);

  if (!bobgui_widget_has_focus (BOBGUI_WIDGET (tree_view)))
    return;

  if (priv->cursor_node == NULL)
    return;

  cursor_path = _bobgui_tree_path_new_from_rbtree (priv->cursor_tree,
                                                priv->cursor_node);

  if (bobgui_tree_model_get_iter (priv->model, &iter, cursor_path) == FALSE)
    {
      bobgui_tree_path_free (cursor_path);
      return;
    }
  bobgui_tree_path_free (cursor_path);

  list = rtl ? g_list_last (priv->columns) : g_list_first (priv->columns);
  if (priv->focus_column)
    {
      /* Save the cell/area we are moving focus from, if moving the cursor
       * by one step hits the end we'll set focus back here */
      last_focus_area = bobgui_cell_layout_get_area (BOBGUI_CELL_LAYOUT (priv->focus_column));
      last_focus_cell = bobgui_cell_area_get_focus_cell (last_focus_area);

      for (; list; list = (rtl ? list->prev : list->next))
	{
	  if (list->data == priv->focus_column)
	    break;
	}
    }

  direction = count > 0 ? BOBGUI_DIR_RIGHT : BOBGUI_DIR_LEFT;

  while (list)
    {
      column = list->data;
      if (bobgui_tree_view_column_get_visible (column) == FALSE)
	goto loop_end;

      bobgui_tree_view_column_cell_set_cell_data (column,
					       priv->model,
					       &iter,
					       BOBGUI_TREE_RBNODE_FLAG_SET (priv->cursor_node, BOBGUI_TREE_RBNODE_IS_PARENT),
					       priv->cursor_node->children ? TRUE : FALSE);

      cell_area = bobgui_cell_layout_get_area (BOBGUI_CELL_LAYOUT (column));
      if (bobgui_cell_area_focus (cell_area, direction))
	{
          _bobgui_tree_view_set_focus_column (tree_view, column);
	  found_column = TRUE;
	  break;
	}

    loop_end:
      if (count == 1)
	list = rtl ? list->prev : list->next;
      else
	list = rtl ? list->next : list->prev;
    }

  if (found_column)
    {
      if (!bobgui_tree_view_has_can_focus_cell (tree_view))
        bobgui_widget_queue_draw (BOBGUI_WIDGET (tree_view));
      g_signal_emit (tree_view, tree_view_signals[CURSOR_CHANGED], 0);
      bobgui_widget_grab_focus (BOBGUI_WIDGET (tree_view));
    }
  else
    {
      bobgui_widget_error_bell (BOBGUI_WIDGET (tree_view));

      if (last_focus_area)
	bobgui_cell_area_set_focus_cell (last_focus_area, last_focus_cell);
    }

  bobgui_tree_view_clamp_column_visible (tree_view,
				      priv->focus_column, TRUE);
}

static void
bobgui_tree_view_move_cursor_start_end (BobguiTreeView *tree_view,
				     int          count)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  BobguiTreeRBTree *cursor_tree;
  BobguiTreeRBNode *cursor_node;
  BobguiTreePath *path;
  BobguiTreePath *old_path;

  if (!bobgui_widget_has_focus (BOBGUI_WIDGET (tree_view)))
    return;

  g_return_if_fail (priv->tree != NULL);

  bobgui_tree_view_get_cursor (tree_view, &old_path, NULL);

  cursor_tree = priv->tree;

  if (count == -1)
    {
      cursor_node = bobgui_tree_rbtree_first (cursor_tree);

      /* Now go forward to find the first focusable row. */
      path = _bobgui_tree_path_new_from_rbtree (cursor_tree, cursor_node);
      search_first_focusable_path (tree_view, &path,
				   TRUE, &cursor_tree, &cursor_node);
    }
  else
    {
      cursor_node = cursor_tree->root;

      do
	{
	  while (cursor_node && !bobgui_tree_rbtree_is_nil (cursor_node->right))
	    cursor_node = cursor_node->right;
	  if (cursor_node->children == NULL)
	    break;

	  cursor_tree = cursor_node->children;
	  cursor_node = cursor_tree->root;
	}
      while (1);

      /* Now go backwards to find last focusable row. */
      path = _bobgui_tree_path_new_from_rbtree (cursor_tree, cursor_node);
      search_first_focusable_path (tree_view, &path,
				   FALSE, &cursor_tree, &cursor_node);
    }

  if (!path)
    goto cleanup;

  if (bobgui_tree_path_compare (old_path, path))
    {
      bobgui_tree_view_real_set_cursor (tree_view, path, CLEAR_AND_SELECT | CLAMP_NODE);
      bobgui_widget_grab_focus (BOBGUI_WIDGET (tree_view));
    }
  else
    {
      bobgui_widget_error_bell (BOBGUI_WIDGET (tree_view));
    }

cleanup:
  bobgui_tree_path_free (old_path);
  bobgui_tree_path_free (path);
}

static gboolean
bobgui_tree_view_real_select_all (BobguiTreeView *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  if (!bobgui_widget_has_focus (BOBGUI_WIDGET (tree_view)))
    return FALSE;

  if (bobgui_tree_selection_get_mode (priv->selection) != BOBGUI_SELECTION_MULTIPLE)
    return FALSE;

  bobgui_tree_selection_select_all (priv->selection);

  return TRUE;
}

static gboolean
bobgui_tree_view_real_unselect_all (BobguiTreeView *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  if (!bobgui_widget_has_focus (BOBGUI_WIDGET (tree_view)))
    return FALSE;

  if (bobgui_tree_selection_get_mode (priv->selection) != BOBGUI_SELECTION_MULTIPLE)
    return FALSE;

  bobgui_tree_selection_unselect_all (priv->selection);

  return TRUE;
}

static gboolean
bobgui_tree_view_real_select_cursor_row (BobguiTreeView *tree_view,
				      gboolean     start_editing)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  BobguiTreeRBTree *new_tree = NULL;
  BobguiTreeRBNode *new_node = NULL;
  BobguiTreeRBTree *cursor_tree = NULL;
  BobguiTreeRBNode *cursor_node = NULL;
  BobguiTreePath *cursor_path = NULL;
  BobguiTreeSelectMode mode = 0;

  if (!bobgui_widget_has_focus (BOBGUI_WIDGET (tree_view)))
    return FALSE;

  if (priv->cursor_node == NULL)
    return FALSE;

  cursor_path = _bobgui_tree_path_new_from_rbtree (priv->cursor_tree,
                                                priv->cursor_node);

  _bobgui_tree_view_find_node (tree_view, cursor_path,
			    &cursor_tree, &cursor_node);

  if (cursor_tree == NULL)
    {
      bobgui_tree_path_free (cursor_path);
      return FALSE;
    }

  if (!priv->extend_selection_pressed && start_editing &&
      priv->focus_column)
    {
      if (bobgui_tree_view_start_editing (tree_view, cursor_path, FALSE))
	{
	  bobgui_tree_path_free (cursor_path);
	  return TRUE;
	}
    }

  if (priv->modify_selection_pressed)
    mode |= BOBGUI_TREE_SELECT_MODE_TOGGLE;
  if (priv->extend_selection_pressed)
    mode |= BOBGUI_TREE_SELECT_MODE_EXTEND;

  _bobgui_tree_selection_internal_select_node (priv->selection,
					    cursor_node,
					    cursor_tree,
					    cursor_path,
                                            mode,
					    FALSE);

  /* We bail out if the original (tree, node) don't exist anymore after
   * handling the selection-changed callback.  We do return TRUE because
   * the key press has been handled at this point.
   */
  _bobgui_tree_view_find_node (tree_view, cursor_path, &new_tree, &new_node);

  if (cursor_tree != new_tree || cursor_node != new_node)
    return FALSE;

  bobgui_tree_view_clamp_node_visible (tree_view, cursor_tree, cursor_node);

  bobgui_widget_grab_focus (BOBGUI_WIDGET (tree_view));
  bobgui_widget_queue_draw (BOBGUI_WIDGET (tree_view));

  if (!priv->extend_selection_pressed)
    bobgui_tree_view_row_activated (tree_view, cursor_path,
                                 priv->focus_column);

  bobgui_tree_path_free (cursor_path);

  return TRUE;
}

static gboolean
bobgui_tree_view_real_toggle_cursor_row (BobguiTreeView *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  BobguiTreeRBTree *new_tree = NULL;
  BobguiTreeRBNode *new_node = NULL;
  BobguiTreePath *cursor_path = NULL;

  if (!bobgui_widget_has_focus (BOBGUI_WIDGET (tree_view)))
    return FALSE;

  if (priv->cursor_node == NULL)
    return FALSE;

  cursor_path = _bobgui_tree_path_new_from_rbtree (priv->cursor_tree,
                                                priv->cursor_node);

  _bobgui_tree_selection_internal_select_node (priv->selection,
					    priv->cursor_node,
					    priv->cursor_tree,
					    cursor_path,
                                            BOBGUI_TREE_SELECT_MODE_TOGGLE,
					    FALSE);

  /* We bail out if the original (tree, node) don't exist anymore after
   * handling the selection-changed callback.  We do return TRUE because
   * the key press has been handled at this point.
   */
  _bobgui_tree_view_find_node (tree_view, cursor_path, &new_tree, &new_node);

  if (priv->cursor_node != new_node)
    return FALSE;

  bobgui_tree_view_clamp_node_visible (tree_view,
                                    priv->cursor_tree,
                                    priv->cursor_node);

  bobgui_widget_grab_focus (BOBGUI_WIDGET (tree_view));
  bobgui_widget_queue_draw (BOBGUI_WIDGET (tree_view));
  bobgui_tree_path_free (cursor_path);

  return TRUE;
}

static gboolean
bobgui_tree_view_real_expand_collapse_cursor_row (BobguiTreeView *tree_view,
					       gboolean     logical,
					       gboolean     expand,
					       gboolean     open_all)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  BobguiTreePath *cursor_path = NULL;

  if (!bobgui_widget_has_focus (BOBGUI_WIDGET (tree_view)))
    return FALSE;

  if (priv->cursor_node == NULL)
    return FALSE;

  cursor_path = _bobgui_tree_path_new_from_rbtree (priv->cursor_tree,
                                                priv->cursor_node);

  /* Don't handle the event if we aren't an expander */
  if (!BOBGUI_TREE_RBNODE_FLAG_SET (priv->cursor_node, BOBGUI_TREE_RBNODE_IS_PARENT))
    return FALSE;

  if (!logical
      && bobgui_widget_get_direction (BOBGUI_WIDGET (tree_view)) == BOBGUI_TEXT_DIR_RTL)
    expand = !expand;

  if (expand)
    bobgui_tree_view_real_expand_row (tree_view,
                                   cursor_path,
                                   priv->cursor_tree,
                                   priv->cursor_node,
                                   open_all);
  else
    bobgui_tree_view_real_collapse_row (tree_view,
                                     cursor_path,
                                     priv->cursor_tree,
                                     priv->cursor_node);

  bobgui_tree_path_free (cursor_path);

  return TRUE;
}

static gboolean
bobgui_tree_view_real_select_cursor_parent (BobguiTreeView *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  BobguiTreePath *cursor_path = NULL;

  if (!bobgui_widget_has_focus (BOBGUI_WIDGET (tree_view)))
    goto out;

  if (priv->cursor_node == NULL)
    goto out;

  cursor_path = _bobgui_tree_path_new_from_rbtree (priv->cursor_tree,
                                                priv->cursor_node);

  if (priv->cursor_tree->parent_node)
    {
      bobgui_widget_queue_draw (BOBGUI_WIDGET (tree_view));

      bobgui_tree_path_up (cursor_path);

      bobgui_tree_view_real_set_cursor (tree_view, cursor_path, CLEAR_AND_SELECT | CLAMP_NODE);
      bobgui_tree_path_free (cursor_path);

      bobgui_widget_grab_focus (BOBGUI_WIDGET (tree_view));

      return TRUE;
    }

 out:

  priv->search_entry_avoid_unhandled_binding = TRUE;
  return FALSE;
}

static gboolean
bobgui_tree_view_search_entry_flush_timeout (BobguiTreeView *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  bobgui_tree_view_search_popover_hide (priv->search_popover, tree_view);
  priv->typeselect_flush_timeout = 0;

  return FALSE;
}

static void
bobgui_tree_view_ensure_interactive_directory (BobguiTreeView *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  BobguiEventController *controller;
  BobguiGesture *gesture;

  if (priv->search_custom_entry_set)
    return;

  if (priv->search_popover)
    return;

  priv->search_popover = bobgui_popover_new ();
  bobgui_css_node_insert_after (bobgui_widget_get_css_node (BOBGUI_WIDGET (tree_view)),
                             bobgui_widget_get_css_node (priv->search_popover),
                             priv->header_node);
  bobgui_widget_set_parent (priv->search_popover, BOBGUI_WIDGET (tree_view));
  bobgui_popover_set_autohide (BOBGUI_POPOVER (priv->search_popover), FALSE);

  controller = bobgui_event_controller_key_new ();
  g_signal_connect (controller, "key-pressed",
		    G_CALLBACK (bobgui_tree_view_search_key_pressed),
		    tree_view);
  bobgui_widget_add_controller (priv->search_popover, controller);

  gesture = bobgui_gesture_click_new ();
  g_signal_connect (gesture, "pressed",
                    G_CALLBACK (bobgui_tree_view_search_pressed_cb), tree_view);
  bobgui_widget_add_controller (priv->search_popover, BOBGUI_EVENT_CONTROLLER (gesture));

  controller = bobgui_event_controller_scroll_new (BOBGUI_EVENT_CONTROLLER_SCROLL_VERTICAL);
  g_signal_connect (controller, "scroll",
		    G_CALLBACK (bobgui_tree_view_search_scroll_event),
		    tree_view);
  bobgui_widget_add_controller (priv->search_popover, controller);

  priv->search_entry = bobgui_text_new ();

  controller = bobgui_text_get_key_controller (BOBGUI_TEXT (priv->search_entry));
  bobgui_event_controller_set_propagation_limit (controller, BOBGUI_LIMIT_NONE);

  g_signal_connect (priv->search_entry, "activate",
                    G_CALLBACK (bobgui_tree_view_search_activate), tree_view);
  g_signal_connect (priv->search_entry, "preedit-changed",
		    G_CALLBACK (bobgui_tree_view_search_preedit_changed), tree_view);
  g_signal_connect (priv->search_entry, "changed",
		    G_CALLBACK (bobgui_tree_view_search_changed), tree_view);

  bobgui_popover_set_child (BOBGUI_POPOVER (priv->search_popover), priv->search_entry);

  bobgui_widget_realize (priv->search_entry);
}

/* Pops up the interactive search entry.  If keybinding is TRUE then the user
 * started this by typing the start_interactive_search keybinding.  Otherwise, it came from
 */
static gboolean
bobgui_tree_view_real_start_interactive_search (BobguiTreeView *tree_view,
					     gboolean     keybinding)
{
  /* We only start interactive search if we have focus or the columns
   * have focus.  If one of our children have focus, we don't want to
   * start the search.
   */
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  GList *list;
  gboolean found_focus = FALSE;

  if (!priv->enable_search && !keybinding)
    return FALSE;

  if (priv->search_custom_entry_set)
    return FALSE;

  if (priv->search_popover &&
      bobgui_widget_get_visible (priv->search_popover))
    return TRUE;

  for (list = priv->columns; list; list = list->next)
    {
      BobguiTreeViewColumn *column;
      BobguiWidget         *button;

      column = list->data;
      if (!bobgui_tree_view_column_get_visible (column))
	continue;

      button = bobgui_tree_view_column_get_button (column);
      if (bobgui_widget_has_focus (button))
	{
	  found_focus = TRUE;
	  break;
	}
    }

  if (bobgui_widget_has_focus (BOBGUI_WIDGET (tree_view)))
    found_focus = TRUE;

  if (!found_focus)
    return FALSE;

  if (priv->search_column < 0)
    return FALSE;

  bobgui_tree_view_ensure_interactive_directory (tree_view);

  if (keybinding)
    bobgui_editable_set_text (BOBGUI_EDITABLE (priv->search_entry), "");

  /* Grab focus without selecting all the text. */
  bobgui_text_grab_focus_without_selecting (BOBGUI_TEXT (priv->search_entry));

  bobgui_popover_popup (BOBGUI_POPOVER (priv->search_popover));
  if (priv->search_entry_changed_id == 0)
    {
      priv->search_entry_changed_id =
	g_signal_connect (priv->search_entry, "changed",
			  G_CALLBACK (bobgui_tree_view_search_init),
			  tree_view);
    }

  priv->typeselect_flush_timeout =
    g_timeout_add (BOBGUI_TREE_VIEW_SEARCH_DIALOG_TIMEOUT,
                   (GSourceFunc) bobgui_tree_view_search_entry_flush_timeout,
                   tree_view);
  gdk_source_set_static_name_by_id (priv->typeselect_flush_timeout, "[bobgui] bobgui_tree_view_search_entry_flush_timeout");

  /* search first matching iter */
  bobgui_tree_view_search_init (priv->search_entry, tree_view);

  return TRUE;
}

static gboolean
bobgui_tree_view_start_interactive_search (BobguiTreeView *tree_view)
{
  return bobgui_tree_view_real_start_interactive_search (tree_view, TRUE);
}

/* Callbacks */
static void
bobgui_tree_view_adjustment_changed (BobguiAdjustment *adjustment,
				  BobguiTreeView   *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  if (bobgui_widget_get_realized (BOBGUI_WIDGET (tree_view)))
    {
      BobguiAllocation allocation;
      int dy;

      bobgui_widget_get_allocation (BOBGUI_WIDGET (tree_view), &allocation);
      dy = priv->dy - (int) bobgui_adjustment_get_value (priv->vadjustment);

      if (dy != 0)
        {
          /* update our dy and top_row */
          priv->dy = (int) bobgui_adjustment_get_value (priv->vadjustment);

          update_prelight (tree_view,
                           priv->event_last_x,
                           priv->event_last_y);

          if (!priv->in_top_row_to_dy)
            bobgui_tree_view_dy_to_top_row (tree_view);

        }
    }

  bobgui_widget_queue_allocate (BOBGUI_WIDGET (tree_view));
}



/* Public methods
 */

/**
 * bobgui_tree_view_new:
 *
 * Creates a new `BobguiTreeView` widget.
 *
 * Returns: A newly created `BobguiTreeView` widget.
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 **/
BobguiWidget *
bobgui_tree_view_new (void)
{
  return g_object_new (BOBGUI_TYPE_TREE_VIEW, NULL);
}

/**
 * bobgui_tree_view_new_with_model:
 * @model: the model.
 *
 * Creates a new `BobguiTreeView` widget with the model initialized to @model.
 *
 * Returns: A newly created `BobguiTreeView` widget.
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 **/
BobguiWidget *
bobgui_tree_view_new_with_model (BobguiTreeModel *model)
{
  return g_object_new (BOBGUI_TYPE_TREE_VIEW, "model", model, NULL);
}

/* Public Accessors
 */

/**
 * bobgui_tree_view_get_model:
 * @tree_view: a `BobguiTreeView`
 *
 * Returns the model the `BobguiTreeView` is based on.  Returns %NULL if the
 * model is unset.
 *
 * Returns: (transfer none) (nullable): A `BobguiTreeModel`
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 **/
BobguiTreeModel *
bobgui_tree_view_get_model (BobguiTreeView *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  g_return_val_if_fail (BOBGUI_IS_TREE_VIEW (tree_view), NULL);

  return priv->model;
}

/**
 * bobgui_tree_view_set_model:
 * @tree_view: A `BobguiTreeView`.
 * @model: (nullable): The model.
 *
 * Sets the model for a `BobguiTreeView`.  If the @tree_view already has a model
 * set, it will remove it before setting the new model.  If @model is %NULL,
 * then it will unset the old model.
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 **/
void
bobgui_tree_view_set_model (BobguiTreeView  *tree_view,
			 BobguiTreeModel *model)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  g_return_if_fail (BOBGUI_IS_TREE_VIEW (tree_view));
  g_return_if_fail (model == NULL || BOBGUI_IS_TREE_MODEL (model));

  if (model == priv->model)
    return;

  if (priv->scroll_to_path)
    {
      bobgui_tree_row_reference_free (priv->scroll_to_path);
      priv->scroll_to_path = NULL;
    }

  if (priv->rubber_band_status)
    bobgui_tree_view_stop_rubber_band (tree_view);

  if (priv->model)
    {
      GList *tmplist = priv->columns;

      bobgui_tree_view_unref_and_check_selection_tree (tree_view, priv->tree);
      bobgui_tree_view_stop_editing (tree_view, TRUE);

      g_signal_handlers_disconnect_by_func (priv->model,
					    bobgui_tree_view_row_changed,
					    tree_view);
      g_signal_handlers_disconnect_by_func (priv->model,
					    bobgui_tree_view_row_inserted,
					    tree_view);
      g_signal_handlers_disconnect_by_func (priv->model,
					    bobgui_tree_view_row_has_child_toggled,
					    tree_view);
      g_signal_handlers_disconnect_by_func (priv->model,
					    bobgui_tree_view_row_deleted,
					    tree_view);
      g_signal_handlers_disconnect_by_func (priv->model,
					    bobgui_tree_view_rows_reordered,
					    tree_view);

      for (; tmplist; tmplist = tmplist->next)
	_bobgui_tree_view_column_unset_model (tmplist->data,
					   priv->model);

      if (priv->tree)
	bobgui_tree_view_free_rbtree (tree_view);

      bobgui_tree_row_reference_free (priv->drag_dest_row);
      priv->drag_dest_row = NULL;
      bobgui_tree_row_reference_free (priv->anchor);
      priv->anchor = NULL;
      bobgui_tree_row_reference_free (priv->top_row);
      priv->top_row = NULL;
      bobgui_tree_row_reference_free (priv->scroll_to_path);
      priv->scroll_to_path = NULL;

      priv->scroll_to_column = NULL;

      g_object_unref (priv->model);

      priv->search_column = -1;
      priv->fixed_height_check = 0;
      priv->fixed_height = -1;
      priv->dy = priv->top_row_dy = 0;
    }

  priv->model = model;

  if (priv->model)
    {
      int i;
      BobguiTreePath *path;
      BobguiTreeIter iter;
      BobguiTreeModelFlags flags;

      if (priv->search_column == -1)
	{
	  for (i = 0; i < bobgui_tree_model_get_n_columns (model); i++)
	    {
	      GType type = bobgui_tree_model_get_column_type (model, i);

	      if (g_value_type_transformable (type, G_TYPE_STRING))
		{
		  priv->search_column = i;
		  break;
		}
	    }
	}

      g_object_ref (priv->model);
      g_signal_connect (priv->model,
			"row-changed",
			G_CALLBACK (bobgui_tree_view_row_changed),
			tree_view);
      g_signal_connect (priv->model,
			"row-inserted",
			G_CALLBACK (bobgui_tree_view_row_inserted),
			tree_view);
      g_signal_connect (priv->model,
			"row-has-child-toggled",
			G_CALLBACK (bobgui_tree_view_row_has_child_toggled),
			tree_view);
      g_signal_connect (priv->model,
			"row-deleted",
			G_CALLBACK (bobgui_tree_view_row_deleted),
			tree_view);
      g_signal_connect (priv->model,
			"rows-reordered",
			G_CALLBACK (bobgui_tree_view_rows_reordered),
			tree_view);

      flags = bobgui_tree_model_get_flags (priv->model);
      if ((flags & BOBGUI_TREE_MODEL_LIST_ONLY) == BOBGUI_TREE_MODEL_LIST_ONLY)
        priv->is_list = TRUE;
      else
        priv->is_list = FALSE;

      path = bobgui_tree_path_new_first ();
      if (bobgui_tree_model_get_iter (priv->model, &iter, path))
	{
	  priv->tree = bobgui_tree_rbtree_new ();
	  bobgui_tree_view_build_tree (tree_view, priv->tree, &iter, 1, FALSE);
	}
      bobgui_tree_path_free (path);

      /*  FIXME: do I need to do this? bobgui_tree_view_create_buttons (tree_view); */
      install_presize_handler (tree_view);
    }

  bobgui_tree_view_real_set_cursor (tree_view, NULL, CURSOR_INVALID);

  g_object_notify_by_pspec (G_OBJECT (tree_view), tree_view_props[PROP_MODEL]);

  if (priv->selection)
    _bobgui_tree_selection_emit_changed (priv->selection);

  if (bobgui_widget_get_realized (BOBGUI_WIDGET (tree_view)))
    bobgui_widget_queue_resize (BOBGUI_WIDGET (tree_view));
}

/**
 * bobgui_tree_view_get_selection:
 * @tree_view: A `BobguiTreeView`.
 *
 * Gets the `BobguiTreeSelection` associated with @tree_view.
 *
 * Returns: (transfer none): A `BobguiTreeSelection` object.
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 **/
BobguiTreeSelection *
bobgui_tree_view_get_selection (BobguiTreeView *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  g_return_val_if_fail (BOBGUI_IS_TREE_VIEW (tree_view), NULL);

  return priv->selection;
}

static void
bobgui_tree_view_do_set_hadjustment (BobguiTreeView   *tree_view,
                                  BobguiAdjustment *adjustment)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  if (adjustment && priv->hadjustment == adjustment)
    return;

  if (priv->hadjustment != NULL)
    {
      g_signal_handlers_disconnect_by_func (priv->hadjustment,
                                            bobgui_tree_view_adjustment_changed,
                                            tree_view);
      g_object_unref (priv->hadjustment);
    }

  if (adjustment == NULL)
    adjustment = bobgui_adjustment_new (0.0, 0.0, 0.0,
                                     0.0, 0.0, 0.0);

  g_signal_connect (adjustment, "value-changed",
                    G_CALLBACK (bobgui_tree_view_adjustment_changed), tree_view);
  priv->hadjustment = g_object_ref_sink (adjustment);
  /* FIXME: Adjustment should probably be populated here with fresh values, but
   * internal details are too complicated for me to decipher right now.
   */
  bobgui_tree_view_adjustment_changed (NULL, tree_view);

  g_object_notify (G_OBJECT (tree_view), "hadjustment");
}

static void
bobgui_tree_view_do_set_vadjustment (BobguiTreeView   *tree_view,
                                  BobguiAdjustment *adjustment)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  if (adjustment && priv->vadjustment == adjustment)
    return;

  if (priv->vadjustment != NULL)
    {
      g_signal_handlers_disconnect_by_func (priv->vadjustment,
                                            bobgui_tree_view_adjustment_changed,
                                            tree_view);
      g_object_unref (priv->vadjustment);
    }

  if (adjustment == NULL)
    adjustment = bobgui_adjustment_new (0.0, 0.0, 0.0,
                                     0.0, 0.0, 0.0);

  g_signal_connect (adjustment, "value-changed",
                    G_CALLBACK (bobgui_tree_view_adjustment_changed), tree_view);
  priv->vadjustment = g_object_ref_sink (adjustment);
  /* FIXME: Adjustment should probably be populated here with fresh values, but
   * internal details are too complicated for me to decipher right now.
   */
  bobgui_tree_view_adjustment_changed (NULL, tree_view);
  g_object_notify (G_OBJECT (tree_view), "vadjustment");
}

/* Column and header operations */

/**
 * bobgui_tree_view_get_headers_visible:
 * @tree_view: A `BobguiTreeView`.
 *
 * Returns %TRUE if the headers on the @tree_view are visible.
 *
 * Returns: Whether the headers are visible or not.
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 **/
gboolean
bobgui_tree_view_get_headers_visible (BobguiTreeView *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  g_return_val_if_fail (BOBGUI_IS_TREE_VIEW (tree_view), FALSE);

  return priv->headers_visible;
}

/**
 * bobgui_tree_view_set_headers_visible:
 * @tree_view: A `BobguiTreeView`.
 * @headers_visible: %TRUE if the headers are visible
 *
 * Sets the visibility state of the headers.
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 **/
void
bobgui_tree_view_set_headers_visible (BobguiTreeView *tree_view,
				   gboolean     headers_visible)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  GList *list;
  BobguiTreeViewColumn *column;
  BobguiWidget *button;

  g_return_if_fail (BOBGUI_IS_TREE_VIEW (tree_view));

  headers_visible = !! headers_visible;

  if (priv->headers_visible == headers_visible)
    return;

  priv->headers_visible = headers_visible == TRUE;

  if (bobgui_widget_get_realized (BOBGUI_WIDGET (tree_view)))
    {
      if (headers_visible)
	{
          if (bobgui_widget_get_mapped (BOBGUI_WIDGET (tree_view)))
            bobgui_tree_view_map_buttons (tree_view);
 	}
      else
	{

	  for (list = priv->columns; list; list = list->next)
	    {
	      column = list->data;
	      button = bobgui_tree_view_column_get_button (column);

              bobgui_widget_hide (button);
	      bobgui_widget_unmap (button);
	    }
	}
    }

  bobgui_widget_queue_resize (BOBGUI_WIDGET (tree_view));

  g_object_notify_by_pspec (G_OBJECT (tree_view), tree_view_props[PROP_HEADERS_VISIBLE]);
}

/**
 * bobgui_tree_view_columns_autosize:
 * @tree_view: A `BobguiTreeView`.
 *
 * Resizes all columns to their optimal width. Only works after the
 * treeview has been realized.
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 **/
void
bobgui_tree_view_columns_autosize (BobguiTreeView *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  gboolean dirty = FALSE;
  GList *list;
  BobguiTreeViewColumn *column;

  g_return_if_fail (BOBGUI_IS_TREE_VIEW (tree_view));

  for (list = priv->columns; list; list = list->next)
    {
      column = list->data;
      if (bobgui_tree_view_column_get_sizing (column) == BOBGUI_TREE_VIEW_COLUMN_AUTOSIZE)
	continue;
      _bobgui_tree_view_column_cell_set_dirty (column, TRUE);
      dirty = TRUE;
    }

  if (dirty)
    bobgui_widget_queue_resize (BOBGUI_WIDGET (tree_view));
}

/**
 * bobgui_tree_view_set_headers_clickable:
 * @tree_view: A `BobguiTreeView`.
 * @setting: %TRUE if the columns are clickable.
 *
 * Allow the column title buttons to be clicked.
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 **/
void
bobgui_tree_view_set_headers_clickable (BobguiTreeView *tree_view,
				     gboolean   setting)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  GList *list;
  gboolean changed = FALSE;

  g_return_if_fail (BOBGUI_IS_TREE_VIEW (tree_view));

  for (list = priv->columns; list; list = list->next)
    {
      if (bobgui_tree_view_column_get_clickable (BOBGUI_TREE_VIEW_COLUMN (list->data)) != setting)
        {
          bobgui_tree_view_column_set_clickable (BOBGUI_TREE_VIEW_COLUMN (list->data), setting);
          changed = TRUE;
        }
    }

  if (changed)
    g_object_notify_by_pspec (G_OBJECT (tree_view), tree_view_props[PROP_HEADERS_CLICKABLE]);
}


/**
 * bobgui_tree_view_get_headers_clickable:
 * @tree_view: A `BobguiTreeView`.
 *
 * Returns whether all header columns are clickable.
 *
 * Returns: %TRUE if all header columns are clickable, otherwise %FALSE
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 **/
gboolean
bobgui_tree_view_get_headers_clickable (BobguiTreeView *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  GList *list;

  g_return_val_if_fail (BOBGUI_IS_TREE_VIEW (tree_view), FALSE);

  for (list = priv->columns; list; list = list->next)
    if (!bobgui_tree_view_column_get_clickable (BOBGUI_TREE_VIEW_COLUMN (list->data)))
      return FALSE;

  return TRUE;
}

/**
 * bobgui_tree_view_set_activate_on_single_click:
 * @tree_view: a `BobguiTreeView`
 * @single: %TRUE to emit row-activated on a single click
 *
 * Cause the `BobguiTreeView`::row-activated signal to be emitted
 * on a single click instead of a double click.
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 **/
void
bobgui_tree_view_set_activate_on_single_click (BobguiTreeView *tree_view,
                                            gboolean     single)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  g_return_if_fail (BOBGUI_IS_TREE_VIEW (tree_view));

  single = single != FALSE;

  if (priv->activate_on_single_click == single)
    return;

  priv->activate_on_single_click = single;
  g_object_notify_by_pspec (G_OBJECT (tree_view), tree_view_props[PROP_ACTIVATE_ON_SINGLE_CLICK]);
}

/**
 * bobgui_tree_view_get_activate_on_single_click:
 * @tree_view: a `BobguiTreeView`
 *
 * Gets the setting set by bobgui_tree_view_set_activate_on_single_click().
 *
 * Returns: %TRUE if row-activated will be emitted on a single click
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 **/
gboolean
bobgui_tree_view_get_activate_on_single_click (BobguiTreeView *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  g_return_val_if_fail (BOBGUI_IS_TREE_VIEW (tree_view), FALSE);

  return priv->activate_on_single_click;
}

/* Public Column functions
 */

/**
 * bobgui_tree_view_append_column:
 * @tree_view: A `BobguiTreeView`.
 * @column: The `BobguiTreeViewColumn` to add.
 *
 * Appends @column to the list of columns. If @tree_view has “fixed_height”
 * mode enabled, then @column must have its “sizing” property set to be
 * BOBGUI_TREE_VIEW_COLUMN_FIXED.
 *
 * Returns: The number of columns in @tree_view after appending.
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 **/
int
bobgui_tree_view_append_column (BobguiTreeView       *tree_view,
			     BobguiTreeViewColumn *column)
{
  g_return_val_if_fail (BOBGUI_IS_TREE_VIEW (tree_view), -1);
  g_return_val_if_fail (BOBGUI_IS_TREE_VIEW_COLUMN (column), -1);
  g_return_val_if_fail (bobgui_tree_view_column_get_tree_view (column) == NULL, -1);

  return bobgui_tree_view_insert_column (tree_view, column, -1);
}

/**
 * bobgui_tree_view_remove_column:
 * @tree_view: A `BobguiTreeView`.
 * @column: The `BobguiTreeViewColumn` to remove.
 *
 * Removes @column from @tree_view.
 *
 * Returns: The number of columns in @tree_view after removing.
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 **/
int
bobgui_tree_view_remove_column (BobguiTreeView       *tree_view,
                             BobguiTreeViewColumn *column)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  g_return_val_if_fail (BOBGUI_IS_TREE_VIEW (tree_view), -1);
  g_return_val_if_fail (BOBGUI_IS_TREE_VIEW_COLUMN (column), -1);
  g_return_val_if_fail (bobgui_tree_view_column_get_tree_view (column) == BOBGUI_WIDGET (tree_view), -1);

  if (priv->focus_column == column)
    _bobgui_tree_view_set_focus_column (tree_view, NULL);

  if (priv->edited_column == column)
    {
      bobgui_tree_view_stop_editing (tree_view, TRUE);

      /* no need to, but just to be sure ... */
      priv->edited_column = NULL;
    }

  if (priv->expander_column == column)
    priv->expander_column = NULL;

  g_signal_handlers_disconnect_by_func (column,
                                        G_CALLBACK (column_sizing_notify),
                                        tree_view);

  _bobgui_tree_view_column_unset_tree_view (column);

  priv->columns = g_list_remove (priv->columns, column);
  priv->n_columns--;

  if (bobgui_widget_get_realized (BOBGUI_WIDGET (tree_view)))
    {
      GList *list;

      for (list = priv->columns; list; list = list->next)
	{
	  BobguiTreeViewColumn *tmp_column;

	  tmp_column = BOBGUI_TREE_VIEW_COLUMN (list->data);
	  if (bobgui_tree_view_column_get_visible (tmp_column))
            _bobgui_tree_view_column_cell_set_dirty (tmp_column, TRUE);
	}

      bobgui_widget_queue_resize (BOBGUI_WIDGET (tree_view));
    }

  g_object_unref (column);
  g_signal_emit (tree_view, tree_view_signals[COLUMNS_CHANGED], 0);

  return priv->n_columns;
}

/**
 * bobgui_tree_view_insert_column:
 * @tree_view: A `BobguiTreeView`.
 * @column: The `BobguiTreeViewColumn` to be inserted.
 * @position: The position to insert @column in.
 *
 * This inserts the @column into the @tree_view at @position.  If @position is
 * -1, then the column is inserted at the end. If @tree_view has
 * “fixed_height” mode enabled, then @column must have its “sizing” property
 * set to be BOBGUI_TREE_VIEW_COLUMN_FIXED.
 *
 * Returns: The number of columns in @tree_view after insertion.
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 **/
int
bobgui_tree_view_insert_column (BobguiTreeView       *tree_view,
                             BobguiTreeViewColumn *column,
                             int                position)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  g_return_val_if_fail (BOBGUI_IS_TREE_VIEW (tree_view), -1);
  g_return_val_if_fail (BOBGUI_IS_TREE_VIEW_COLUMN (column), -1);
  g_return_val_if_fail (bobgui_tree_view_column_get_tree_view (column) == NULL, -1);

  if (priv->fixed_height_mode)
    g_return_val_if_fail (bobgui_tree_view_column_get_sizing (column)
                          == BOBGUI_TREE_VIEW_COLUMN_FIXED, -1);

  if (position < 0 || position > priv->n_columns)
    position = priv->n_columns;

  g_object_ref_sink (column);

  g_signal_connect (column, "notify::sizing",
                    G_CALLBACK (column_sizing_notify), tree_view);

  priv->columns = g_list_insert (priv->columns,
					    column, position);
  priv->n_columns++;

  _bobgui_tree_view_column_set_tree_view (column, tree_view);

  /* XXX: We need to reparent the node into the header, somebody make that a real widget */
  bobgui_css_node_set_parent (bobgui_widget_get_css_node (bobgui_tree_view_column_get_button (column)), NULL);
  bobgui_tree_view_update_button_position (tree_view, column);

  if (bobgui_widget_get_realized (BOBGUI_WIDGET (tree_view)))
    {
      GList *list;

      _bobgui_tree_view_column_realize_button (column);

      for (list = priv->columns; list; list = list->next)
	{
	  column = BOBGUI_TREE_VIEW_COLUMN (list->data);
	  if (bobgui_tree_view_column_get_visible (column))
            _bobgui_tree_view_column_cell_set_dirty (column, TRUE);
	}
      bobgui_widget_queue_resize (BOBGUI_WIDGET (tree_view));
    }

  g_signal_emit (tree_view, tree_view_signals[COLUMNS_CHANGED], 0);

  return priv->n_columns;
}

/**
 * bobgui_tree_view_insert_column_with_attributes:
 * @tree_view: A `BobguiTreeView`
 * @position: The position to insert the new column in
 * @title: The title to set the header to
 * @cell: The `BobguiCellRenderer`
 * @...: A %NULL-terminated list of attributes
 *
 * Creates a new `BobguiTreeViewColumn` and inserts it into the @tree_view at
 * @position.  If @position is -1, then the newly created column is inserted at
 * the end.  The column is initialized with the attributes given. If @tree_view
 * has “fixed_height” mode enabled, then the new column will have its sizing
 * property set to be BOBGUI_TREE_VIEW_COLUMN_FIXED.
 *
 * Returns: The number of columns in @tree_view after insertion.
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 **/
int
bobgui_tree_view_insert_column_with_attributes (BobguiTreeView     *tree_view,
					     int              position,
					     const char      *title,
					     BobguiCellRenderer *cell,
					     ...)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  BobguiTreeViewColumn *column;
  char *attribute;
  va_list args;
  int column_id;

  g_return_val_if_fail (BOBGUI_IS_TREE_VIEW (tree_view), -1);

  column = bobgui_tree_view_column_new ();
  if (priv->fixed_height_mode)
    bobgui_tree_view_column_set_sizing (column, BOBGUI_TREE_VIEW_COLUMN_FIXED);

  bobgui_tree_view_column_set_title (column, title);
  bobgui_tree_view_column_pack_start (column, cell, TRUE);

  va_start (args, cell);

  attribute = va_arg (args, char *);

  while (attribute != NULL)
    {
      column_id = va_arg (args, int);
      bobgui_tree_view_column_add_attribute (column, cell, attribute, column_id);
      attribute = va_arg (args, char *);
    }

  va_end (args);

  return bobgui_tree_view_insert_column (tree_view, column, position);
}

/**
 * bobgui_tree_view_insert_column_with_data_func:
 * @tree_view: a `BobguiTreeView`
 * @position: Position to insert, -1 for append
 * @title: column title
 * @cell: cell renderer for column
 * @func: function to set attributes of cell renderer
 * @data: data for @func
 * @dnotify: destroy notifier for @data
 *
 * Convenience function that inserts a new column into the `BobguiTreeView`
 * with the given cell renderer and a `BobguiTreeCellDataFunc` to set cell renderer
 * attributes (normally using data from the model). See also
 * bobgui_tree_view_column_set_cell_data_func(), bobgui_tree_view_column_pack_start().
 * If @tree_view has “fixed_height” mode enabled, then the new column will have its
 * “sizing” property set to be BOBGUI_TREE_VIEW_COLUMN_FIXED.
 *
 * Returns: number of columns in the tree view post-insert
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 **/
int
bobgui_tree_view_insert_column_with_data_func  (BobguiTreeView               *tree_view,
                                             int                        position,
                                             const char                *title,
                                             BobguiCellRenderer           *cell,
                                             BobguiTreeCellDataFunc        func,
                                             gpointer                   data,
                                             GDestroyNotify             dnotify)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  BobguiTreeViewColumn *column;

  g_return_val_if_fail (BOBGUI_IS_TREE_VIEW (tree_view), -1);

  column = bobgui_tree_view_column_new ();
  if (priv->fixed_height_mode)
    bobgui_tree_view_column_set_sizing (column, BOBGUI_TREE_VIEW_COLUMN_FIXED);

  bobgui_tree_view_column_set_title (column, title);
  bobgui_tree_view_column_pack_start (column, cell, TRUE);
  bobgui_tree_view_column_set_cell_data_func (column, cell, func, data, dnotify);

  return bobgui_tree_view_insert_column (tree_view, column, position);
}

/**
 * bobgui_tree_view_get_n_columns:
 * @tree_view: a `BobguiTreeView`
 *
 * Queries the number of columns in the given @tree_view.
 *
 * Returns: The number of columns in the @tree_view
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 **/
guint
bobgui_tree_view_get_n_columns (BobguiTreeView *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  g_return_val_if_fail (BOBGUI_IS_TREE_VIEW (tree_view), 0);

  return priv->n_columns;
}

/**
 * bobgui_tree_view_get_column:
 * @tree_view: A `BobguiTreeView`.
 * @n: The position of the column, counting from 0.
 *
 * Gets the `BobguiTreeViewColumn` at the given position in the #tree_view.
 *
 * Returns: (nullable) (transfer none): The `BobguiTreeViewColumn`, or %NULL if the
 * position is outside the range of columns.
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 **/
BobguiTreeViewColumn *
bobgui_tree_view_get_column (BobguiTreeView *tree_view,
			  int          n)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  g_return_val_if_fail (BOBGUI_IS_TREE_VIEW (tree_view), NULL);

  if (n < 0 || n >= priv->n_columns)
    return NULL;

  if (priv->columns == NULL)
    return NULL;

  return BOBGUI_TREE_VIEW_COLUMN (g_list_nth (priv->columns, n)->data);
}

/**
 * bobgui_tree_view_get_columns:
 * @tree_view: A `BobguiTreeView`
 *
 * Returns a `GList` of all the `BobguiTreeViewColumn`s currently in @tree_view.
 * The returned list must be freed with g_list_free ().
 *
 * Returns: (element-type BobguiTreeViewColumn) (transfer container): A list of `BobguiTreeViewColumn`s
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 **/
GList *
bobgui_tree_view_get_columns (BobguiTreeView *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  g_return_val_if_fail (BOBGUI_IS_TREE_VIEW (tree_view), NULL);

  return g_list_copy (priv->columns);
}

/**
 * bobgui_tree_view_move_column_after:
 * @tree_view: A `BobguiTreeView`
 * @column: The `BobguiTreeViewColumn` to be moved.
 * @base_column: (nullable): The `BobguiTreeViewColumn` to be moved relative to
 *
 * Moves @column to be after to @base_column.  If @base_column is %NULL, then
 * @column is placed in the first position.
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 **/
void
bobgui_tree_view_move_column_after (BobguiTreeView       *tree_view,
				 BobguiTreeViewColumn *column,
				 BobguiTreeViewColumn *base_column)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  GList *column_list_el, *base_el = NULL;

  g_return_if_fail (BOBGUI_IS_TREE_VIEW (tree_view));

  column_list_el = g_list_find (priv->columns, column);
  g_return_if_fail (column_list_el != NULL);

  if (base_column)
    {
      base_el = g_list_find (priv->columns, base_column);
      g_return_if_fail (base_el != NULL);
    }

  if (column_list_el->prev == base_el)
    return;

  priv->columns = g_list_remove_link (priv->columns, column_list_el);
  if (base_el == NULL)
    {
      column_list_el->prev = NULL;
      column_list_el->next = priv->columns;
      if (column_list_el->next)
	column_list_el->next->prev = column_list_el;
      priv->columns = column_list_el;
    }
  else
    {
      column_list_el->prev = base_el;
      column_list_el->next = base_el->next;
      if (column_list_el->next)
	column_list_el->next->prev = column_list_el;
      base_el->next = column_list_el;
    }

  bobgui_tree_view_update_button_position (tree_view, column);

  bobgui_widget_queue_resize (BOBGUI_WIDGET (tree_view));

  g_signal_emit (tree_view, tree_view_signals[COLUMNS_CHANGED], 0);
}

/**
 * bobgui_tree_view_set_expander_column:
 * @tree_view: A `BobguiTreeView`
 * @column: (nullable): %NULL, or the column to draw the expander arrow at.
 *
 * Sets the column to draw the expander arrow at. It must be in @tree_view.
 * If @column is %NULL, then the expander arrow is always at the first
 * visible column.
 *
 * If you do not want expander arrow to appear in your tree, set the
 * expander column to a hidden column.
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 **/
void
bobgui_tree_view_set_expander_column (BobguiTreeView       *tree_view,
                                   BobguiTreeViewColumn *column)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  g_return_if_fail (BOBGUI_IS_TREE_VIEW (tree_view));
  g_return_if_fail (column == NULL || BOBGUI_IS_TREE_VIEW_COLUMN (column));
  g_return_if_fail (column == NULL || bobgui_tree_view_column_get_tree_view (column) == BOBGUI_WIDGET (tree_view));

  if (priv->expander_column != column)
    {
      priv->expander_column = column;
      g_object_notify_by_pspec (G_OBJECT (tree_view), tree_view_props[PROP_EXPANDER_COLUMN]);
    }
}

/**
 * bobgui_tree_view_get_expander_column:
 * @tree_view: A `BobguiTreeView`
 *
 * Returns the column that is the current expander column,
 * or %NULL if none has been set.
 * This column has the expander arrow drawn next to it.
 *
 * Returns: (transfer none) (nullable): The expander column.
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 **/
BobguiTreeViewColumn *
bobgui_tree_view_get_expander_column (BobguiTreeView *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  GList *list;

  g_return_val_if_fail (BOBGUI_IS_TREE_VIEW (tree_view), NULL);

  for (list = priv->columns; list; list = list->next)
    if (bobgui_tree_view_is_expander_column (tree_view, BOBGUI_TREE_VIEW_COLUMN (list->data)))
      return (BobguiTreeViewColumn *) list->data;
  return NULL;
}


/**
 * bobgui_tree_view_set_column_drag_function:
 * @tree_view: A `BobguiTreeView`.
 * @func: (nullable) (scope notified) (closure user_data) (destroy destroy): A function to determine which columns are reorderable
 * @user_data: User data to be passed to @func
 * @destroy: (nullable): Destroy notifier for @user_data
 *
 * Sets a user function for determining where a column may be dropped when
 * dragged.  This function is called on every column pair in turn at the
 * beginning of a column drag to determine where a drop can take place.  The
 * arguments passed to @func are: the @tree_view, the `BobguiTreeViewColumn` being
 * dragged, the two `BobguiTreeViewColumn`s determining the drop spot, and
 * @user_data.  If either of the `BobguiTreeViewColumn` arguments for the drop spot
 * are %NULL, then they indicate an edge.  If @func is set to be %NULL, then
 * @tree_view reverts to the default behavior of allowing all columns to be
 * dropped everywhere.
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 **/
void
bobgui_tree_view_set_column_drag_function (BobguiTreeView               *tree_view,
					BobguiTreeViewColumnDropFunc  func,
					gpointer                   user_data,
					GDestroyNotify             destroy)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  g_return_if_fail (BOBGUI_IS_TREE_VIEW (tree_view));

  if (priv->column_drop_func_data_destroy)
    priv->column_drop_func_data_destroy (priv->column_drop_func_data);

  priv->column_drop_func = func;
  priv->column_drop_func_data = user_data;
  priv->column_drop_func_data_destroy = destroy;
}

/**
 * bobgui_tree_view_scroll_to_point:
 * @tree_view: a `BobguiTreeView`
 * @tree_x: X coordinate of new top-left pixel of visible area, or -1
 * @tree_y: Y coordinate of new top-left pixel of visible area, or -1
 *
 * Scrolls the tree view such that the top-left corner of the visible
 * area is @tree_x, @tree_y, where @tree_x and @tree_y are specified
 * in tree coordinates.  The @tree_view must be realized before
 * this function is called.  If it isn't, you probably want to be
 * using bobgui_tree_view_scroll_to_cell().
 *
 * If either @tree_x or @tree_y are -1, then that direction isn’t scrolled.
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 **/
void
bobgui_tree_view_scroll_to_point (BobguiTreeView *tree_view,
                               int          tree_x,
                               int          tree_y)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  BobguiAdjustment *hadj;
  BobguiAdjustment *vadj;

  g_return_if_fail (BOBGUI_IS_TREE_VIEW (tree_view));
  g_return_if_fail (bobgui_widget_get_realized (BOBGUI_WIDGET (tree_view)));

  hadj = priv->hadjustment;
  vadj = priv->vadjustment;

  if (tree_x != -1)
    bobgui_adjustment_animate_to_value (hadj, tree_x);
  if (tree_y != -1)
    bobgui_adjustment_animate_to_value (vadj, tree_y);
}

/**
 * bobgui_tree_view_scroll_to_cell:
 * @tree_view: A `BobguiTreeView`.
 * @path: (nullable): The path of the row to move to
 * @column: (nullable): The `BobguiTreeViewColumn` to move horizontally to
 * @use_align: whether to use alignment arguments, or %FALSE.
 * @row_align: The vertical alignment of the row specified by @path.
 * @col_align: The horizontal alignment of the column specified by @column.
 *
 * Moves the alignments of @tree_view to the position specified by @column and
 * @path.  If @column is %NULL, then no horizontal scrolling occurs.  Likewise,
 * if @path is %NULL no vertical scrolling occurs.  At a minimum, one of @column
 * or @path need to be non-%NULL.  @row_align determines where the row is
 * placed, and @col_align determines where @column is placed.  Both are expected
 * to be between 0.0 and 1.0. 0.0 means left/top alignment, 1.0 means
 * right/bottom alignment, 0.5 means center.
 *
 * If @use_align is %FALSE, then the alignment arguments are ignored, and the
 * tree does the minimum amount of work to scroll the cell onto the screen.
 * This means that the cell will be scrolled to the edge closest to its current
 * position.  If the cell is currently visible on the screen, nothing is done.
 *
 * This function only works if the model is set, and @path is a valid row on the
 * model.  If the model changes before the @tree_view is realized, the centered
 * path will be modified to reflect this change.
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 **/
void
bobgui_tree_view_scroll_to_cell (BobguiTreeView       *tree_view,
                              BobguiTreePath       *path,
                              BobguiTreeViewColumn *column,
			      gboolean           use_align,
                              float              row_align,
                              float              col_align)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  g_return_if_fail (BOBGUI_IS_TREE_VIEW (tree_view));
  g_return_if_fail (priv->model != NULL);
  g_return_if_fail (priv->tree != NULL);
  g_return_if_fail (row_align >= 0.0 && row_align <= 1.0);
  g_return_if_fail (col_align >= 0.0 && col_align <= 1.0);
  g_return_if_fail (path != NULL || column != NULL);

  row_align = CLAMP (row_align, 0.0, 1.0);
  col_align = CLAMP (col_align, 0.0, 1.0);


  /* Note: Despite the benefits that come from having one code path for the
   * scrolling code, we short-circuit validate_visible_area's immplementation as
   * it is much slower than just going to the point.
   */
  if (!bobgui_widget_get_visible (BOBGUI_WIDGET (tree_view)) ||
      !bobgui_widget_get_realized (BOBGUI_WIDGET (tree_view)) ||
      _bobgui_widget_get_alloc_needed (BOBGUI_WIDGET (tree_view)) ||
      BOBGUI_TREE_RBNODE_FLAG_SET (priv->tree->root, BOBGUI_TREE_RBNODE_DESCENDANTS_INVALID))
    {
      if (priv->scroll_to_path)
	bobgui_tree_row_reference_free (priv->scroll_to_path);

      priv->scroll_to_path = NULL;
      priv->scroll_to_column = NULL;

      if (path)
	priv->scroll_to_path = bobgui_tree_row_reference_new_proxy (G_OBJECT (tree_view), priv->model, path);
      if (column)
	priv->scroll_to_column = column;
      priv->scroll_to_use_align = use_align;
      priv->scroll_to_row_align = row_align;
      priv->scroll_to_col_align = col_align;

      install_presize_handler (tree_view);
    }
  else
    {
      GdkRectangle cell_rect;
      GdkRectangle vis_rect;
      int dest_x, dest_y;

      bobgui_tree_view_get_background_area (tree_view, path, column, &cell_rect);
      bobgui_tree_view_get_visible_rect (tree_view, &vis_rect);

      cell_rect.y = TREE_WINDOW_Y_TO_RBTREE_Y (priv, cell_rect.y);

      dest_x = vis_rect.x;
      dest_y = vis_rect.y;

      if (column)
	{
	  if (use_align)
	    {
	      dest_x = cell_rect.x - ((vis_rect.width - cell_rect.width) * col_align);
	    }
	  else
	    {
	      if (cell_rect.x < vis_rect.x)
		dest_x = cell_rect.x;
	      if (cell_rect.x + cell_rect.width > vis_rect.x + vis_rect.width)
		dest_x = cell_rect.x + cell_rect.width - vis_rect.width;
	    }
	}

      if (path)
	{
	  if (use_align)
	    {
	      dest_y = cell_rect.y - ((vis_rect.height - cell_rect.height) * row_align);
	      dest_y = MAX (dest_y, 0);
	    }
	  else
	    {
	      if (cell_rect.y < vis_rect.y)
		dest_y = cell_rect.y;
	      if (cell_rect.y + cell_rect.height > vis_rect.y + vis_rect.height)
		dest_y = cell_rect.y + cell_rect.height - vis_rect.height;
	    }
	}

      bobgui_tree_view_scroll_to_point (tree_view, dest_x, dest_y);
    }
}

/**
 * bobgui_tree_view_row_activated:
 * @tree_view: A `BobguiTreeView`
 * @path: The `BobguiTreePath` to be activated.
 * @column: (nullable): The `BobguiTreeViewColumn` to be activated.
 *
 * Activates the cell determined by @path and @column.
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 **/
void
bobgui_tree_view_row_activated (BobguiTreeView       *tree_view,
			     BobguiTreePath       *path,
			     BobguiTreeViewColumn *column)
{
  g_return_if_fail (BOBGUI_IS_TREE_VIEW (tree_view));

  g_signal_emit (tree_view, tree_view_signals[ROW_ACTIVATED], 0, path, column);
}


static void
bobgui_tree_view_expand_all_emission_helper (BobguiTreeRBTree *tree,
                                          BobguiTreeRBNode *node,
                                          gpointer       data)
{
  BobguiTreeView *tree_view = data;
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  if ((node->flags & BOBGUI_TREE_RBNODE_IS_PARENT) == BOBGUI_TREE_RBNODE_IS_PARENT &&
      node->children)
    {
      BobguiTreePath *path;
      BobguiTreeIter iter;

      path = _bobgui_tree_path_new_from_rbtree (tree, node);
      bobgui_tree_model_get_iter (priv->model, &iter, path);

      g_signal_emit (tree_view, tree_view_signals[ROW_EXPANDED], 0, &iter, path);

      bobgui_tree_path_free (path);
    }

  if (node->children)
    bobgui_tree_rbtree_traverse (node->children,
                          node->children->root,
                          G_PRE_ORDER,
                          bobgui_tree_view_expand_all_emission_helper,
                          tree_view);
}

/**
 * bobgui_tree_view_expand_all:
 * @tree_view: A `BobguiTreeView`.
 *
 * Recursively expands all nodes in the @tree_view.
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 **/
void
bobgui_tree_view_expand_all (BobguiTreeView *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  BobguiTreePath *path;
  BobguiTreeRBTree *tree;
  BobguiTreeRBNode *node;

  g_return_if_fail (BOBGUI_IS_TREE_VIEW (tree_view));

  if (priv->tree == NULL)
    return;

  path = bobgui_tree_path_new_first ();
  _bobgui_tree_view_find_node (tree_view, path, &tree, &node);

  while (node)
    {
      bobgui_tree_view_real_expand_row (tree_view, path, tree, node, TRUE);
      node = bobgui_tree_rbtree_next (tree, node);
      bobgui_tree_path_next (path);
  }

  bobgui_tree_path_free (path);
}

/**
 * bobgui_tree_view_collapse_all:
 * @tree_view: A `BobguiTreeView`.
 *
 * Recursively collapses all visible, expanded nodes in @tree_view.
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 **/
void
bobgui_tree_view_collapse_all (BobguiTreeView *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  BobguiTreeRBTree *tree;
  BobguiTreeRBNode *node;
  BobguiTreePath *path;
  int *indices;

  g_return_if_fail (BOBGUI_IS_TREE_VIEW (tree_view));

  if (priv->tree == NULL)
    return;

  path = bobgui_tree_path_new ();
  bobgui_tree_path_down (path);
  indices = bobgui_tree_path_get_indices (path);

  tree = priv->tree;
  node = bobgui_tree_rbtree_first (tree);

  while (node)
    {
      if (node->children)
	bobgui_tree_view_real_collapse_row (tree_view, path, tree, node);
      indices[0]++;
      node = bobgui_tree_rbtree_next (tree, node);
    }

  bobgui_tree_path_free (path);
}

/**
 * bobgui_tree_view_expand_to_path:
 * @tree_view: A `BobguiTreeView`.
 * @path: path to a row.
 *
 * Expands the row at @path. This will also expand all parent rows of
 * @path as necessary.
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 **/
void
bobgui_tree_view_expand_to_path (BobguiTreeView *tree_view,
			      BobguiTreePath *path)
{
  int i, depth;
  int *indices;
  BobguiTreePath *tmp;

  g_return_if_fail (BOBGUI_IS_TREE_VIEW (tree_view));
  g_return_if_fail (path != NULL);

  depth = bobgui_tree_path_get_depth (path);
  indices = bobgui_tree_path_get_indices (path);

  tmp = bobgui_tree_path_new ();
  g_return_if_fail (tmp != NULL);

  for (i = 0; i < depth; i++)
    {
      bobgui_tree_path_append_index (tmp, indices[i]);
      bobgui_tree_view_expand_row (tree_view, tmp, FALSE);
    }

  bobgui_tree_path_free (tmp);
}

/* FIXME the bool return values for expand_row and collapse_row are
 * not analogous; they should be TRUE if the row had children and
 * was not already in the requested state.
 */


static gboolean
bobgui_tree_view_real_expand_row (BobguiTreeView   *tree_view,
			       BobguiTreePath   *path,
			       BobguiTreeRBTree *tree,
			       BobguiTreeRBNode *node,
			       gboolean       open_all)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  BobguiTreeIter iter;
  BobguiTreeIter temp;
  gboolean expand;

  remove_auto_expand_timeout (tree_view);

  if (node->children && !open_all)
    return FALSE;

  if (! BOBGUI_TREE_RBNODE_FLAG_SET (node, BOBGUI_TREE_RBNODE_IS_PARENT))
    return FALSE;

  bobgui_tree_model_get_iter (priv->model, &iter, path);
  if (! bobgui_tree_model_iter_has_child (priv->model, &iter))
    return FALSE;


   if (node->children && open_all)
    {
      gboolean retval = FALSE;
      BobguiTreePath *tmp_path = bobgui_tree_path_copy (path);

      bobgui_tree_path_append_index (tmp_path, 0);
      tree = node->children;
      node = bobgui_tree_rbtree_first (tree);
      /* try to expand the children */
      do
        {
         gboolean t;
	 t = bobgui_tree_view_real_expand_row (tree_view, tmp_path, tree, node,
                                            TRUE);
         if (t)
           retval = TRUE;

         bobgui_tree_path_next (tmp_path);
	 node = bobgui_tree_rbtree_next (tree, node);
       }
      while (node != NULL);

      bobgui_tree_path_free (tmp_path);

      return retval;
    }

  g_signal_emit (tree_view, tree_view_signals[TEST_EXPAND_ROW], 0, &iter, path, &expand);

  if (!bobgui_tree_model_iter_has_child (priv->model, &iter))
    return FALSE;

  if (expand)
    return FALSE;

  node->children = bobgui_tree_rbtree_new ();
  node->children->parent_tree = tree;
  node->children->parent_node = node;

  bobgui_tree_model_iter_children (priv->model, &temp, &iter);

  bobgui_tree_view_build_tree (tree_view,
			    node->children,
			    &temp,
			    bobgui_tree_path_get_depth (path) + 1,
			    open_all);

  install_presize_handler (tree_view);

  g_signal_emit (tree_view, tree_view_signals[ROW_EXPANDED], 0, &iter, path);
  if (open_all && node->children)
    {
      bobgui_tree_rbtree_traverse (node->children,
                            node->children->root,
                            G_PRE_ORDER,
                            bobgui_tree_view_expand_all_emission_helper,
                            tree_view);
    }
  return TRUE;
}


/**
 * bobgui_tree_view_expand_row:
 * @tree_view: a `BobguiTreeView`
 * @path: path to a row
 * @open_all: whether to recursively expand, or just expand immediate children
 *
 * Opens the row so its children are visible.
 *
 * Returns: %TRUE if the row existed and had children
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 **/
gboolean
bobgui_tree_view_expand_row (BobguiTreeView *tree_view,
			  BobguiTreePath *path,
			  gboolean     open_all)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  BobguiTreeRBTree *tree;
  BobguiTreeRBNode *node;

  g_return_val_if_fail (BOBGUI_IS_TREE_VIEW (tree_view), FALSE);
  g_return_val_if_fail (priv->model != NULL, FALSE);
  g_return_val_if_fail (path != NULL, FALSE);

  if (_bobgui_tree_view_find_node (tree_view,
				path,
				&tree,
				&node))
    return FALSE;

  if (tree != NULL)
    return bobgui_tree_view_real_expand_row (tree_view, path, tree, node, open_all);
  else
    return FALSE;
}

static gboolean
bobgui_tree_view_real_collapse_row (BobguiTreeView   *tree_view,
				 BobguiTreePath   *path,
				 BobguiTreeRBTree *tree,
				 BobguiTreeRBNode *node)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  BobguiTreeIter iter;
  BobguiTreeIter children;
  gboolean collapse;
  GList *list;
  gboolean selection_changed, cursor_changed;

  remove_auto_expand_timeout (tree_view);

  if (node->children == NULL)
    return FALSE;
  bobgui_tree_model_get_iter (priv->model, &iter, path);

  g_signal_emit (tree_view, tree_view_signals[TEST_COLLAPSE_ROW], 0, &iter, path, &collapse);

  if (collapse)
    return FALSE;

  /* if the prelighted node is a child of us, we want to unprelight it.  We have
   * a chance to prelight the correct node below */

  if (priv->prelight_tree)
    {
      BobguiTreeRBTree *parent_tree;
      BobguiTreeRBNode *parent_node;

      parent_tree = priv->prelight_tree->parent_tree;
      parent_node = priv->prelight_tree->parent_node;
      while (parent_tree)
	{
	  if (parent_tree == tree && parent_node == node)
	    {
	      ensure_unprelighted (tree_view);
	      break;
	    }
	  parent_node = parent_tree->parent_node;
	  parent_tree = parent_tree->parent_tree;
	}
    }

  TREE_VIEW_INTERNAL_ASSERT (bobgui_tree_model_iter_children (priv->model, &children, &iter), FALSE);

  for (list = priv->columns; list; list = list->next)
    {
      BobguiTreeViewColumn *column = list->data;

      if (bobgui_tree_view_column_get_visible (column) == FALSE)
	continue;
      if (bobgui_tree_view_column_get_sizing (column) == BOBGUI_TREE_VIEW_COLUMN_AUTOSIZE)
	_bobgui_tree_view_column_cell_set_dirty (column, TRUE);
    }

  if (priv->cursor_node)
    {
      cursor_changed = (node->children == priv->cursor_tree)
                       || bobgui_tree_rbtree_contains (node->children, priv->cursor_tree);
    }
  else
    cursor_changed = FALSE;

  if (bobgui_tree_row_reference_valid (priv->anchor))
    {
      BobguiTreePath *anchor_path = bobgui_tree_row_reference_get_path (priv->anchor);
      if (bobgui_tree_path_is_ancestor (path, anchor_path))
	{
	  bobgui_tree_row_reference_free (priv->anchor);
	  priv->anchor = NULL;
	}
      bobgui_tree_path_free (anchor_path);
    }

  selection_changed = bobgui_tree_view_unref_and_check_selection_tree (tree_view, node->children);

  /* Stop a pending double click */
  bobgui_event_controller_reset (BOBGUI_EVENT_CONTROLLER (priv->click_gesture));

  bobgui_tree_rbtree_remove (node->children);

  if (cursor_changed)
    bobgui_tree_view_real_set_cursor (tree_view, path, CLEAR_AND_SELECT | CURSOR_INVALID);
  if (selection_changed)
    g_signal_emit_by_name (priv->selection, "changed");

  if (bobgui_widget_get_mapped (BOBGUI_WIDGET (tree_view)))
    {
      bobgui_widget_queue_resize (BOBGUI_WIDGET (tree_view));
    }

  g_signal_emit (tree_view, tree_view_signals[ROW_COLLAPSED], 0, &iter, path);

  if (bobgui_widget_get_mapped (BOBGUI_WIDGET (tree_view)))
    update_prelight (tree_view,
                     priv->event_last_x,
                     priv->event_last_y);

  return TRUE;
}

/**
 * bobgui_tree_view_collapse_row:
 * @tree_view: a `BobguiTreeView`
 * @path: path to a row in the @tree_view
 *
 * Collapses a row (hides its child rows, if they exist).
 *
 * Returns: %TRUE if the row was collapsed.
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 **/
gboolean
bobgui_tree_view_collapse_row (BobguiTreeView *tree_view,
			    BobguiTreePath *path)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  BobguiTreeRBTree *tree;
  BobguiTreeRBNode *node;

  g_return_val_if_fail (BOBGUI_IS_TREE_VIEW (tree_view), FALSE);
  g_return_val_if_fail (priv->tree != NULL, FALSE);
  g_return_val_if_fail (path != NULL, FALSE);

  if (_bobgui_tree_view_find_node (tree_view,
				path,
				&tree,
				&node))
    return FALSE;

  if (tree == NULL || node->children == NULL)
    return FALSE;

  return bobgui_tree_view_real_collapse_row (tree_view, path, tree, node);
}

static void
bobgui_tree_view_map_expanded_rows_helper (BobguiTreeView            *tree_view,
					BobguiTreeRBTree          *tree,
					BobguiTreePath            *path,
					BobguiTreeViewMappingFunc  func,
					gpointer                user_data)
{
  BobguiTreeRBNode *node;

  if (tree == NULL || tree->root == NULL)
    return;

  node = bobgui_tree_rbtree_first (tree);

  while (node)
    {
      if (node->children)
	{
	  (* func) (tree_view, path, user_data);
	  bobgui_tree_path_down (path);
	  bobgui_tree_view_map_expanded_rows_helper (tree_view, node->children, path, func, user_data);
	  bobgui_tree_path_up (path);
	}
      bobgui_tree_path_next (path);
      node = bobgui_tree_rbtree_next (tree, node);
    }
}

/**
 * bobgui_tree_view_map_expanded_rows:
 * @tree_view: A `BobguiTreeView`
 * @func: (scope call): A function to be called
 * @data: User data to be passed to the function.
 *
 * Calls @func on all expanded rows.
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 **/
void
bobgui_tree_view_map_expanded_rows (BobguiTreeView            *tree_view,
				 BobguiTreeViewMappingFunc  func,
				 gpointer                user_data)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  BobguiTreePath *path;

  g_return_if_fail (BOBGUI_IS_TREE_VIEW (tree_view));
  g_return_if_fail (func != NULL);

  path = bobgui_tree_path_new_first ();

  bobgui_tree_view_map_expanded_rows_helper (tree_view,
					  priv->tree,
					  path, func, user_data);

  bobgui_tree_path_free (path);
}

/**
 * bobgui_tree_view_row_expanded:
 * @tree_view: A `BobguiTreeView`.
 * @path: A `BobguiTreePath` to test expansion state.
 *
 * Returns %TRUE if the node pointed to by @path is expanded in @tree_view.
 *
 * Returns: %TRUE if #path is expanded.
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 **/
gboolean
bobgui_tree_view_row_expanded (BobguiTreeView *tree_view,
			    BobguiTreePath *path)
{
  BobguiTreeRBTree *tree;
  BobguiTreeRBNode *node;

  g_return_val_if_fail (BOBGUI_IS_TREE_VIEW (tree_view), FALSE);
  g_return_val_if_fail (path != NULL, FALSE);

  _bobgui_tree_view_find_node (tree_view, path, &tree, &node);

  if (node == NULL)
    return FALSE;

  return (node->children != NULL);
}

/**
 * bobgui_tree_view_get_reorderable:
 * @tree_view: a `BobguiTreeView`
 *
 * Retrieves whether the user can reorder the tree via drag-and-drop. See
 * bobgui_tree_view_set_reorderable().
 *
 * Returns: %TRUE if the tree can be reordered.
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 **/
gboolean
bobgui_tree_view_get_reorderable (BobguiTreeView *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  g_return_val_if_fail (BOBGUI_IS_TREE_VIEW (tree_view), FALSE);

  return priv->reorderable;
}

/**
 * bobgui_tree_view_set_reorderable:
 * @tree_view: A `BobguiTreeView`.
 * @reorderable: %TRUE, if the tree can be reordered.
 *
 * This function is a convenience function to allow you to reorder
 * models that support the `BobguiTreeDragSourceIface` and the
 * `BobguiTreeDragDestIface`.  Both `BobguiTreeStore` and `BobguiListStore` support
 * these.  If @reorderable is %TRUE, then the user can reorder the
 * model by dragging and dropping rows. The developer can listen to
 * these changes by connecting to the model’s `BobguiTreeModel::row-inserted`
 * and `BobguiTreeModel::row-deleted` signals. The reordering is implemented
 * by setting up the tree view as a drag source and destination.
 * Therefore, drag and drop can not be used in a reorderable view for any
 * other purpose.
 *
 * This function does not give you any degree of control over the order -- any
 * reordering is allowed.  If more control is needed, you should probably
 * handle drag and drop manually.
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 **/
void
bobgui_tree_view_set_reorderable (BobguiTreeView *tree_view,
			       gboolean     reorderable)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  g_return_if_fail (BOBGUI_IS_TREE_VIEW (tree_view));

  reorderable = reorderable != FALSE;

  if (priv->reorderable == reorderable)
    return;

  if (reorderable)
    {
      GdkContentFormats *formats;

      formats = gdk_content_formats_new_for_gtype (BOBGUI_TYPE_TREE_ROW_DATA);

      bobgui_tree_view_enable_model_drag_source (tree_view,
					      GDK_BUTTON1_MASK,
					      formats,
					      GDK_ACTION_MOVE);
      bobgui_tree_view_enable_model_drag_dest (tree_view,
					    formats,
					    GDK_ACTION_MOVE);
      gdk_content_formats_unref (formats);
    }
  else
    {
      bobgui_tree_view_unset_rows_drag_source (tree_view);
      bobgui_tree_view_unset_rows_drag_dest (tree_view);
    }

  priv->reorderable = reorderable;

  g_object_notify_by_pspec (G_OBJECT (tree_view), tree_view_props[PROP_REORDERABLE]);
}

static void
bobgui_tree_view_real_set_cursor (BobguiTreeView     *tree_view,
			       BobguiTreePath     *path,
                               SetCursorFlags   flags)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  if (!(flags & CURSOR_INVALID) && priv->cursor_node)
    bobgui_widget_queue_draw (BOBGUI_WIDGET (tree_view));

  /* One cannot set the cursor on a separator.   Also, if
   * _bobgui_tree_view_find_node returns TRUE, it ran out of tree
   * before finding the tree and node belonging to path.  The
   * path maps to a non-existing path and we will silently bail out.
   * We unset tree and node to avoid further processing.
   */
  if (path == NULL ||
      row_is_separator (tree_view, NULL, path)
      || _bobgui_tree_view_find_node (tree_view,
                                   path,
                                   &priv->cursor_tree,
                                   &priv->cursor_node))
    {
      priv->cursor_tree = NULL;
      priv->cursor_node = NULL;
    }

  if (priv->cursor_node != NULL)
    {
      BobguiTreeRBTree *new_tree = NULL;
      BobguiTreeRBNode *new_node = NULL;

      if ((flags & CLEAR_AND_SELECT) && !priv->modify_selection_pressed)
        {
          BobguiTreeSelectMode mode = 0;

          if (priv->extend_selection_pressed)
            mode |= BOBGUI_TREE_SELECT_MODE_EXTEND;

          _bobgui_tree_selection_internal_select_node (priv->selection,
                                                    priv->cursor_node,
                                                    priv->cursor_tree,
                                                    path,
                                                    mode,
                                                    FALSE);
        }

      /* We have to re-find tree and node here again, somebody might have
       * cleared the node or the whole tree in the BobguiTreeSelection::changed
       * callback. If the nodes differ we bail out here.
       */
      _bobgui_tree_view_find_node (tree_view, path, &new_tree, &new_node);

      if (priv->cursor_node == NULL ||
          priv->cursor_node != new_node)
        return;

      if (flags & CLAMP_NODE)
        {
	  bobgui_tree_view_clamp_node_visible (tree_view,
                                            priv->cursor_tree,
                                            priv->cursor_node);
          bobgui_widget_queue_draw (BOBGUI_WIDGET (tree_view));
	}
    }

  if (!bobgui_widget_in_destruction (BOBGUI_WIDGET (tree_view)))
    g_signal_emit (tree_view, tree_view_signals[CURSOR_CHANGED], 0);
}

/**
 * bobgui_tree_view_get_cursor:
 * @tree_view: A `BobguiTreeView`
 * @path: (out) (transfer full) (optional) (nullable): A pointer to be
 *   filled with the current cursor path
 * @focus_column: (out) (transfer none) (optional) (nullable): A
 *   pointer to be filled with the current focus column
 *
 * Fills in @path and @focus_column with the current path and focus column.  If
 * the cursor isn’t currently set, then *@path will be %NULL.  If no column
 * currently has focus, then *@focus_column will be %NULL.
 *
 * The returned `BobguiTreePath` must be freed with bobgui_tree_path_free() when
 * you are done with it.
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 **/
void
bobgui_tree_view_get_cursor (BobguiTreeView        *tree_view,
			  BobguiTreePath       **path,
			  BobguiTreeViewColumn **focus_column)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  g_return_if_fail (BOBGUI_IS_TREE_VIEW (tree_view));

  if (path)
    {
      if (priv->cursor_node)
        *path = _bobgui_tree_path_new_from_rbtree (priv->cursor_tree,
                                                priv->cursor_node);
      else
	*path = NULL;
    }

  if (focus_column)
    {
      *focus_column = priv->focus_column;
    }
}

/**
 * bobgui_tree_view_set_cursor:
 * @tree_view: A `BobguiTreeView`
 * @path: A `BobguiTreePath`
 * @focus_column: (nullable): A `BobguiTreeViewColumn`
 * @start_editing: %TRUE if the specified cell should start being edited.
 *
 * Sets the current keyboard focus to be at @path, and selects it.  This is
 * useful when you want to focus the user’s attention on a particular row.  If
 * @focus_column is not %NULL, then focus is given to the column specified by
 * it. Additionally, if @focus_column is specified, and @start_editing is
 * %TRUE, then editing should be started in the specified cell.
 * This function is often followed by @bobgui_widget_grab_focus (@tree_view)
 * in order to give keyboard focus to the widget.  Please note that editing
 * can only happen when the widget is realized.
 *
 * If @path is invalid for @model, the current cursor (if any) will be unset
 * and the function will return without failing.
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 **/
void
bobgui_tree_view_set_cursor (BobguiTreeView       *tree_view,
			  BobguiTreePath       *path,
			  BobguiTreeViewColumn *focus_column,
			  gboolean           start_editing)
{
  bobgui_tree_view_set_cursor_on_cell (tree_view, path, focus_column,
				    NULL, start_editing);
}

/**
 * bobgui_tree_view_set_cursor_on_cell:
 * @tree_view: A `BobguiTreeView`
 * @path: A `BobguiTreePath`
 * @focus_column: (nullable): A `BobguiTreeViewColumn`
 * @focus_cell: (nullable): A `BobguiCellRenderer`
 * @start_editing: %TRUE if the specified cell should start being edited.
 *
 * Sets the current keyboard focus to be at @path, and selects it.  This is
 * useful when you want to focus the user’s attention on a particular row.  If
 * @focus_column is not %NULL, then focus is given to the column specified by
 * it. If @focus_column and @focus_cell are not %NULL, and @focus_column
 * contains 2 or more editable or activatable cells, then focus is given to
 * the cell specified by @focus_cell. Additionally, if @focus_column is
 * specified, and @start_editing is %TRUE, then editing should be started in
 * the specified cell.  This function is often followed by
 * @bobgui_widget_grab_focus (@tree_view) in order to give keyboard focus to the
 * widget.  Please note that editing can only happen when the widget is
 * realized.
 *
 * If @path is invalid for @model, the current cursor (if any) will be unset
 * and the function will return without failing.
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 **/
void
bobgui_tree_view_set_cursor_on_cell (BobguiTreeView       *tree_view,
				  BobguiTreePath       *path,
				  BobguiTreeViewColumn *focus_column,
				  BobguiCellRenderer   *focus_cell,
				  gboolean           start_editing)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  g_return_if_fail (BOBGUI_IS_TREE_VIEW (tree_view));
  g_return_if_fail (path != NULL);
  g_return_if_fail (focus_column == NULL || BOBGUI_IS_TREE_VIEW_COLUMN (focus_column));

  if (!priv->model)
    return;

  if (focus_cell)
    {
      g_return_if_fail (focus_column);
      g_return_if_fail (BOBGUI_IS_CELL_RENDERER (focus_cell));
    }

  /* cancel the current editing, if it exists */
  if (priv->edited_column &&
      bobgui_cell_area_get_edit_widget
      (bobgui_cell_layout_get_area (BOBGUI_CELL_LAYOUT (priv->edited_column))))
    bobgui_tree_view_stop_editing (tree_view, TRUE);

  bobgui_tree_view_real_set_cursor (tree_view, path, CLEAR_AND_SELECT | CLAMP_NODE);

  if (focus_column &&
      bobgui_tree_view_column_get_visible (focus_column))
    {
#ifndef G_DISABLE_CHECKS
      GList *list;
      gboolean column_in_tree = FALSE;

      for (list = priv->columns; list; list = list->next)
	if (list->data == focus_column)
	  {
	    column_in_tree = TRUE;
	    break;
	  }
      g_return_if_fail (column_in_tree);
#endif
      _bobgui_tree_view_set_focus_column (tree_view, focus_column);
      if (focus_cell)
	bobgui_tree_view_column_focus_cell (focus_column, focus_cell);
      if (start_editing)
	bobgui_tree_view_start_editing (tree_view, path, TRUE);
    }
}

/**
 * bobgui_tree_view_get_path_at_pos:
 * @tree_view: A `BobguiTreeView`.
 * @x: The x position to be identified (relative to bin_window).
 * @y: The y position to be identified (relative to bin_window).
 * @path: (out) (optional) (nullable): A pointer to a `BobguiTreePath`
 *   pointer to be filled in
 * @column: (out) (transfer none) (optional) (nullable): A pointer to
 *   a `BobguiTreeViewColumn` pointer to be filled in
 * @cell_x: (out) (optional): A pointer where the X coordinate
 *   relative to the cell can be placed
 * @cell_y: (out) (optional): A pointer where the Y coordinate
 *   relative to the cell can be placed
 *
 * Finds the path at the point (@x, @y), relative to bin_window coordinates.
 * That is, @x and @y are relative to an events coordinates. Widget-relative
 * coordinates must be converted using
 * bobgui_tree_view_convert_widget_to_bin_window_coords(). It is primarily for
 * things like popup menus. If @path is non-%NULL, then it will be filled
 * with the `BobguiTreePath` at that point.  This path should be freed with
 * bobgui_tree_path_free().  If @column is non-%NULL, then it will be filled
 * with the column at that point.  @cell_x and @cell_y return the coordinates
 * relative to the cell background (i.e. the @background_area passed to
 * bobgui_cell_renderer_render()).  This function is only meaningful if
 * @tree_view is realized.  Therefore this function will always return %FALSE
 * if @tree_view is not realized or does not have a model.
 *
 * For converting widget coordinates (eg. the ones you get from
 * BobguiWidget::query-tooltip), please see
 * bobgui_tree_view_convert_widget_to_bin_window_coords().
 *
 * Returns: %TRUE if a row exists at that coordinate.
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 **/
gboolean
bobgui_tree_view_get_path_at_pos (BobguiTreeView        *tree_view,
			       int                 x,
			       int                 y,
			       BobguiTreePath       **path,
			       BobguiTreeViewColumn **column,
                               int                *cell_x,
                               int                *cell_y)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  BobguiTreeRBTree *tree;
  BobguiTreeRBNode *node;
  int y_offset;

  g_return_val_if_fail (tree_view != NULL, FALSE);

  if (path)
    *path = NULL;
  if (column)
    *column = NULL;

  if (priv->tree == NULL)
    return FALSE;

  if (x > bobgui_adjustment_get_upper (priv->hadjustment))
    return FALSE;

  if (x < 0 || y < 0)
    return FALSE;

  if (column || cell_x)
    {
      BobguiTreeViewColumn *tmp_column;
      BobguiTreeViewColumn *last_column = NULL;
      GList *list;
      int remaining_x = x;
      gboolean found = FALSE;
      gboolean rtl;
      int width;

      rtl = (_bobgui_widget_get_direction (BOBGUI_WIDGET (tree_view)) == BOBGUI_TEXT_DIR_RTL);
      for (list = (rtl ? g_list_last (priv->columns) : g_list_first (priv->columns));
	   list;
	   list = (rtl ? list->prev : list->next))
	{
	  tmp_column = list->data;

	  if (bobgui_tree_view_column_get_visible (tmp_column) == FALSE)
	    continue;

	  last_column = tmp_column;
          width = bobgui_tree_view_column_get_width (tmp_column);
	  if (remaining_x < width)
	    {
              found = TRUE;

              if (column)
                *column = tmp_column;

              if (cell_x)
                *cell_x = remaining_x;

	      break;
	    }
	  remaining_x -= width;
	}

      /* If found is FALSE and there is a last_column, then it the remainder
       * space is in that area
       */
      if (!found)
        {
	  if (last_column)
	    {
	      if (column)
		*column = last_column;

	      if (cell_x)
		*cell_x = bobgui_tree_view_column_get_width (last_column) + remaining_x;
	    }
	  else
	    {
	      return FALSE;
	    }
	}
    }

  y_offset = bobgui_tree_rbtree_find_offset (priv->tree,
				      TREE_WINDOW_Y_TO_RBTREE_Y (priv, y),
				      &tree, &node);

  if (tree == NULL)
    return FALSE;

  if (cell_y)
    *cell_y = y_offset;

  if (path)
    *path = _bobgui_tree_path_new_from_rbtree (tree, node);

  return TRUE;
}


static inline int
bobgui_tree_view_get_cell_area_height (BobguiTreeView   *tree_view,
                                    BobguiTreeRBNode *node)
{
  int expander_size = bobgui_tree_view_get_expander_size (tree_view);
  int height;

  /* The "cell" areas are the cell_area passed in to bobgui_cell_renderer_render(),
   * i.e. just the cells, no spacing.
   *
   * The cell area height is at least expander_size - vertical_separator.
   * For regular nodes, the height is then at least expander_size. We should
   * be able to enforce the expander_size minimum here, because this
   * function will not be called for irregular (e.g. separator) rows.
   */
  height = bobgui_tree_view_get_row_height (tree_view, node);
  if (height < expander_size)
    height = expander_size;

  return height;
}

static inline int
bobgui_tree_view_get_cell_area_y_offset (BobguiTreeView   *tree_view,
                                      BobguiTreeRBTree *tree,
                                      BobguiTreeRBNode *node)
{
  int offset;

  offset = bobgui_tree_view_get_row_y_offset (tree_view, tree, node);

  return offset;
}

/**
 * bobgui_tree_view_get_cell_area:
 * @tree_view: a `BobguiTreeView`
 * @path: (nullable): a `BobguiTreePath` for the row, or %NULL to get only horizontal coordinates
 * @column: (nullable): a `BobguiTreeViewColumn` for the column, or %NULL to get only vertical coordinates
 * @rect: (out): rectangle to fill with cell rect
 *
 * Fills the bounding rectangle in bin_window coordinates for the cell at the
 * row specified by @path and the column specified by @column.  If @path is
 * %NULL, or points to a path not currently displayed, the @y and @height fields
 * of the rectangle will be filled with 0. If @column is %NULL, the @x and @width
 * fields will be filled with 0.  The sum of all cell rects does not cover the
 * entire tree; there are extra pixels in between rows, for example. The
 * returned rectangle is equivalent to the @cell_area passed to
 * bobgui_cell_renderer_render().  This function is only valid if @tree_view is
 * realized.
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 **/
void
bobgui_tree_view_get_cell_area (BobguiTreeView        *tree_view,
                             BobguiTreePath        *path,
                             BobguiTreeViewColumn  *column,
                             GdkRectangle       *rect)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  BobguiTreeRBTree *tree = NULL;
  BobguiTreeRBNode *node = NULL;

  g_return_if_fail (BOBGUI_IS_TREE_VIEW (tree_view));
  g_return_if_fail (column == NULL || BOBGUI_IS_TREE_VIEW_COLUMN (column));
  g_return_if_fail (rect != NULL);
  g_return_if_fail (!column || bobgui_tree_view_column_get_tree_view (column) == (BobguiWidget *) tree_view);
  g_return_if_fail (bobgui_widget_get_realized (BOBGUI_WIDGET (tree_view)));

  rect->x = 0;
  rect->y = 0;
  rect->width = 0;
  rect->height = 0;

  if (column)
    {
      rect->x = bobgui_tree_view_column_get_x_offset (column) + _TREE_VIEW_HORIZONTAL_SEPARATOR / 2;
      rect->width = bobgui_tree_view_column_get_width (column) - _TREE_VIEW_HORIZONTAL_SEPARATOR;
    }

  if (path)
    {
      gboolean ret = _bobgui_tree_view_find_node (tree_view, path, &tree, &node);

      /* Get vertical coords */
      if ((!ret && tree == NULL) || ret)
	return;

      if (row_is_separator (tree_view, NULL, path))
        {
          /* There isn't really a "cell area" for separator, so we
           * return the y, height values for background area instead.
           */
          rect->y = bobgui_tree_view_get_row_y_offset (tree_view, tree, node);
          rect->height = bobgui_tree_view_get_row_height (tree_view, node);
        }
      else
        {
          rect->y = bobgui_tree_view_get_cell_area_y_offset (tree_view, tree, node);
          rect->height = bobgui_tree_view_get_cell_area_height (tree_view, node);
        }

      if (column &&
	  bobgui_tree_view_is_expander_column (tree_view, column))
	{
	  int depth = bobgui_tree_path_get_depth (path);
	  gboolean rtl;

	  rtl = _bobgui_widget_get_direction (BOBGUI_WIDGET (tree_view)) == BOBGUI_TEXT_DIR_RTL;

	  if (!rtl)
	    rect->x += (depth - 1) * priv->level_indentation;
	  rect->width -= (depth - 1) * priv->level_indentation;

	  if (bobgui_tree_view_draw_expanders (tree_view))
	    {
              int expander_size = bobgui_tree_view_get_expander_size (tree_view);
	      if (!rtl)
		rect->x += depth * expander_size;
	      rect->width -= depth * expander_size;
	    }

	  rect->width = MAX (rect->width, 0);
	}
    }
}

static inline int
bobgui_tree_view_get_row_height (BobguiTreeView   *tree_view,
                              BobguiTreeRBNode *node)
{
  int expander_size = bobgui_tree_view_get_expander_size (tree_view);
  int height;

  /* The "background" areas of all rows/cells add up to cover the entire tree.
   * The background includes all inter-row and inter-cell spacing.
   *
   * If the row pointed at by node does not have a height set, we default
   * to expander_size, which is the minimum height for regular nodes.
   * Non-regular nodes (e.g. separators) can have a height set smaller
   * than expander_size and should not be overruled here.
   */
  height = BOBGUI_TREE_RBNODE_GET_HEIGHT (node);
  if (height <= 0)
    height = expander_size;

  return height;
}

static inline int
bobgui_tree_view_get_row_y_offset (BobguiTreeView   *tree_view,
                                BobguiTreeRBTree *tree,
                                BobguiTreeRBNode *node)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  int offset;

  offset = bobgui_tree_rbtree_node_find_offset (tree, node);

  return RBTREE_Y_TO_TREE_WINDOW_Y (priv, offset);
}

/**
 * bobgui_tree_view_get_background_area:
 * @tree_view: a `BobguiTreeView`
 * @path: (nullable): a `BobguiTreePath` for the row, or %NULL to get only horizontal coordinates
 * @column: (nullable): a `BobguiTreeViewColumn` for the column, or %NULL to get only vertical coordinates
 * @rect: (out): rectangle to fill with cell background rect
 *
 * Fills the bounding rectangle in bin_window coordinates for the cell at the
 * row specified by @path and the column specified by @column.  If @path is
 * %NULL, or points to a node not found in the tree, the @y and @height fields of
 * the rectangle will be filled with 0. If @column is %NULL, the @x and @width
 * fields will be filled with 0.  The returned rectangle is equivalent to the
 * @background_area passed to bobgui_cell_renderer_render().  These background
 * areas tile to cover the entire bin window.  Contrast with the @cell_area,
 * returned by bobgui_tree_view_get_cell_area(), which returns only the cell
 * itself, excluding surrounding borders and the tree expander area.
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 **/
void
bobgui_tree_view_get_background_area (BobguiTreeView        *tree_view,
                                   BobguiTreePath        *path,
                                   BobguiTreeViewColumn  *column,
                                   GdkRectangle       *rect)
{
  BobguiTreeRBTree *tree = NULL;
  BobguiTreeRBNode *node = NULL;

  g_return_if_fail (BOBGUI_IS_TREE_VIEW (tree_view));
  g_return_if_fail (column == NULL || BOBGUI_IS_TREE_VIEW_COLUMN (column));
  g_return_if_fail (rect != NULL);

  rect->x = 0;
  rect->y = 0;
  rect->width = 0;
  rect->height = 0;

  if (path)
    {
      /* Get vertical coords */

      if (!_bobgui_tree_view_find_node (tree_view, path, &tree, &node) &&
	  tree == NULL)
	return;

      rect->y = bobgui_tree_view_get_row_y_offset (tree_view, tree, node);
      rect->height = bobgui_tree_view_get_row_height (tree_view, node);
    }

  if (column)
    {
      int x2 = 0;

      bobgui_tree_view_get_background_xrange (tree_view, tree, column, &rect->x, &x2);
      rect->width = x2 - rect->x;
    }
}

/**
 * bobgui_tree_view_get_visible_rect:
 * @tree_view: a `BobguiTreeView`
 * @visible_rect: (out): rectangle to fill
 *
 * Fills @visible_rect with the currently-visible region of the
 * buffer, in tree coordinates. Convert to bin_window coordinates with
 * bobgui_tree_view_convert_tree_to_bin_window_coords().
 * Tree coordinates start at 0,0 for row 0 of the tree, and cover the entire
 * scrollable area of the tree.
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 **/
void
bobgui_tree_view_get_visible_rect (BobguiTreeView  *tree_view,
                                GdkRectangle *visible_rect)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  BobguiAllocation allocation;
  BobguiWidget *widget;

  g_return_if_fail (BOBGUI_IS_TREE_VIEW (tree_view));

  widget = BOBGUI_WIDGET (tree_view);

  if (visible_rect)
    {
      bobgui_widget_get_allocation (widget, &allocation);
      visible_rect->x = bobgui_adjustment_get_value (priv->hadjustment);
      visible_rect->y = bobgui_adjustment_get_value (priv->vadjustment);
      visible_rect->width = allocation.width;
      visible_rect->height = allocation.height - bobgui_tree_view_get_effective_header_height (tree_view);
    }
}

/**
 * bobgui_tree_view_convert_widget_to_tree_coords:
 * @tree_view: a `BobguiTreeView`
 * @wx: X coordinate relative to the widget
 * @wy: Y coordinate relative to the widget
 * @tx: (out): return location for tree X coordinate
 * @ty: (out): return location for tree Y coordinate
 *
 * Converts widget coordinates to coordinates for the
 * tree (the full scrollable area of the tree).
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 **/
void
bobgui_tree_view_convert_widget_to_tree_coords (BobguiTreeView *tree_view,
                                             int          wx,
                                             int          wy,
                                             int         *tx,
                                             int         *ty)
{
  int x, y;

  g_return_if_fail (BOBGUI_IS_TREE_VIEW (tree_view));

  bobgui_tree_view_convert_widget_to_bin_window_coords (tree_view,
						      wx, wy,
						      &x, &y);
  bobgui_tree_view_convert_bin_window_to_tree_coords (tree_view,
						   x, y,
						   tx, ty);
}

/**
 * bobgui_tree_view_convert_tree_to_widget_coords:
 * @tree_view: a `BobguiTreeView`
 * @tx: X coordinate relative to the tree
 * @ty: Y coordinate relative to the tree
 * @wx: (out): return location for widget X coordinate
 * @wy: (out): return location for widget Y coordinate
 *
 * Converts tree coordinates (coordinates in full scrollable area of the tree)
 * to widget coordinates.
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 **/
void
bobgui_tree_view_convert_tree_to_widget_coords (BobguiTreeView *tree_view,
                                             int          tx,
                                             int          ty,
                                             int         *wx,
                                             int         *wy)
{
  int x, y;

  g_return_if_fail (BOBGUI_IS_TREE_VIEW (tree_view));

  bobgui_tree_view_convert_tree_to_bin_window_coords (tree_view,
						    tx, ty,
						    &x, &y);
  bobgui_tree_view_convert_bin_window_to_widget_coords (tree_view,
						     x, y,
						     wx, wy);
}

/**
 * bobgui_tree_view_convert_widget_to_bin_window_coords:
 * @tree_view: a `BobguiTreeView`
 * @wx: X coordinate relative to the widget
 * @wy: Y coordinate relative to the widget
 * @bx: (out): return location for bin_window X coordinate
 * @by: (out): return location for bin_window Y coordinate
 *
 * Converts widget coordinates to coordinates for the bin_window.
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 **/
void
bobgui_tree_view_convert_widget_to_bin_window_coords (BobguiTreeView *tree_view,
                                                   int          wx,
                                                   int          wy,
                                                   int         *bx,
                                                   int         *by)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  g_return_if_fail (BOBGUI_IS_TREE_VIEW (tree_view));

  if (bx)
    *bx = wx + bobgui_adjustment_get_value (priv->hadjustment);
  if (by)
    *by = wy - bobgui_tree_view_get_effective_header_height (tree_view);
}

/**
 * bobgui_tree_view_convert_bin_window_to_widget_coords:
 * @tree_view: a `BobguiTreeView`
 * @bx: bin_window X coordinate
 * @by: bin_window Y coordinate
 * @wx: (out): return location for widget X coordinate
 * @wy: (out): return location for widget Y coordinate
 *
 * Converts bin_window coordinates to widget relative coordinates.
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 **/
void
bobgui_tree_view_convert_bin_window_to_widget_coords (BobguiTreeView *tree_view,
                                                   int          bx,
                                                   int          by,
                                                   int         *wx,
                                                   int         *wy)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  g_return_if_fail (BOBGUI_IS_TREE_VIEW (tree_view));

  if (wx)
    *wx = bx - bobgui_adjustment_get_value (priv->hadjustment);
  if (wy)
    *wy = by + bobgui_tree_view_get_effective_header_height (tree_view);
}

/**
 * bobgui_tree_view_convert_tree_to_bin_window_coords:
 * @tree_view: a `BobguiTreeView`
 * @tx: tree X coordinate
 * @ty: tree Y coordinate
 * @bx: (out): return location for X coordinate relative to bin_window
 * @by: (out): return location for Y coordinate relative to bin_window
 *
 * Converts tree coordinates (coordinates in full scrollable area of the tree)
 * to bin_window coordinates.
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 **/
void
bobgui_tree_view_convert_tree_to_bin_window_coords (BobguiTreeView *tree_view,
                                                 int          tx,
                                                 int          ty,
                                                 int         *bx,
                                                 int         *by)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  g_return_if_fail (BOBGUI_IS_TREE_VIEW (tree_view));

  if (bx)
    *bx = tx;
  if (by)
    *by = ty - priv->dy;
}

/**
 * bobgui_tree_view_convert_bin_window_to_tree_coords:
 * @tree_view: a `BobguiTreeView`
 * @bx: X coordinate relative to bin_window
 * @by: Y coordinate relative to bin_window
 * @tx: (out): return location for tree X coordinate
 * @ty: (out): return location for tree Y coordinate
 *
 * Converts bin_window coordinates to coordinates for the
 * tree (the full scrollable area of the tree).
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 **/
void
bobgui_tree_view_convert_bin_window_to_tree_coords (BobguiTreeView *tree_view,
                                                 int          bx,
                                                 int          by,
                                                 int         *tx,
                                                 int         *ty)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  g_return_if_fail (BOBGUI_IS_TREE_VIEW (tree_view));

  if (tx)
    *tx = bx;
  if (ty)
    *ty = by + priv->dy;
}



/**
 * bobgui_tree_view_get_visible_range:
 * @tree_view: A `BobguiTreeView`
 * @start_path: (out) (optional): Return location for start of region
 * @end_path: (out) (optional): Return location for end of region
 *
 * Sets @start_path and @end_path to be the first and last visible path.
 * Note that there may be invisible paths in between.
 *
 * The paths should be freed with bobgui_tree_path_free() after use.
 *
 * Returns: %TRUE, if valid paths were placed in @start_path and @end_path.
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 */
gboolean
bobgui_tree_view_get_visible_range (BobguiTreeView  *tree_view,
                                 BobguiTreePath **start_path,
                                 BobguiTreePath **end_path)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  BobguiTreeRBTree *tree;
  BobguiTreeRBNode *node;
  gboolean retval;

  g_return_val_if_fail (BOBGUI_IS_TREE_VIEW (tree_view), FALSE);

  if (!priv->tree)
    return FALSE;

  retval = TRUE;

  if (start_path)
    {
      bobgui_tree_rbtree_find_offset (priv->tree,
                                   TREE_WINDOW_Y_TO_RBTREE_Y (priv, 0),
                                   &tree, &node);
      if (node)
        *start_path = _bobgui_tree_path_new_from_rbtree (tree, node);
      else
        retval = FALSE;
    }

  if (end_path)
    {
      int y;

      if (bobgui_tree_view_get_height (tree_view) < bobgui_adjustment_get_page_size (priv->vadjustment))
        y = bobgui_tree_view_get_height (tree_view) - 1;
      else
        y = TREE_WINDOW_Y_TO_RBTREE_Y (priv, bobgui_adjustment_get_page_size (priv->vadjustment)) - 1;

      bobgui_tree_rbtree_find_offset (priv->tree, y, &tree, &node);
      if (node)
        *end_path = _bobgui_tree_path_new_from_rbtree (tree, node);
      else
        retval = FALSE;
    }

  return retval;
}

/**
 * bobgui_tree_view_is_blank_at_pos:
 * @tree_view: A `BobguiTreeView`
 * @x: The x position to be identified (relative to bin_window)
 * @y: The y position to be identified (relative to bin_window)
 * @path: (out) (optional) (nullable): A pointer to a `BobguiTreePath` pointer to
 *   be filled in
 * @column: (out) (transfer none) (optional) (nullable): A pointer to a
 *   `BobguiTreeViewColumn` pointer to be filled in
 * @cell_x: (out) (optional): A pointer where the X coordinate relative to the
 *   cell can be placed
 * @cell_y: (out) (optional): A pointer where the Y coordinate relative to the
 *   cell can be placed
 *
 * Determine whether the point (@x, @y) in @tree_view is blank, that is no
 * cell content nor an expander arrow is drawn at the location. If so, the
 * location can be considered as the background. You might wish to take
 * special action on clicks on the background, such as clearing a current
 * selection, having a custom context menu or starting rubber banding.
 *
 * The @x and @y coordinate that are provided must be relative to bin_window
 * coordinates.  Widget-relative coordinates must be converted using
 * bobgui_tree_view_convert_widget_to_bin_window_coords().
 *
 * For converting widget coordinates (eg. the ones you get from
 * BobguiWidget::query-tooltip), please see
 * bobgui_tree_view_convert_widget_to_bin_window_coords().
 *
 * The @path, @column, @cell_x and @cell_y arguments will be filled in
 * likewise as for bobgui_tree_view_get_path_at_pos().  Please see
 * bobgui_tree_view_get_path_at_pos() for more information.
 *
 * Returns: %TRUE if the area at the given coordinates is blank,
 * %FALSE otherwise.
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 */
gboolean
bobgui_tree_view_is_blank_at_pos (BobguiTreeView       *tree_view,
                               int                 x,
                               int                 y,
                               BobguiTreePath       **path,
                               BobguiTreeViewColumn **column,
                               int                *cell_x,
                               int                *cell_y)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  BobguiTreeRBTree *tree;
  BobguiTreeRBNode *node;
  BobguiTreeIter iter;
  BobguiTreePath *real_path;
  BobguiTreeViewColumn *real_column;
  GdkRectangle cell_area, background_area;

  g_return_val_if_fail (BOBGUI_IS_TREE_VIEW (tree_view), FALSE);

  if (!bobgui_tree_view_get_path_at_pos (tree_view, x, y,
                                      &real_path, &real_column,
                                      cell_x, cell_y))
    /* If there's no path here, it is blank */
    return TRUE;

  if (path)
    *path = real_path;

  if (column)
    *column = real_column;

  bobgui_tree_model_get_iter (priv->model, &iter, real_path);
  _bobgui_tree_view_find_node (tree_view, real_path, &tree, &node);
  if (node == NULL)
    {
      if (!path)
        bobgui_tree_path_free (real_path);
      return TRUE;
    }

  /* Check if there's an expander arrow at (x, y) */
  if (real_column == priv->expander_column
      && bobgui_tree_view_draw_expanders (tree_view))
    {
      gboolean over_arrow;

      over_arrow = coords_are_over_arrow (tree_view, tree, node, x, y);

      if (over_arrow)
        {
          if (!path)
            bobgui_tree_path_free (real_path);
          return FALSE;
        }
    }

  /* Otherwise, have the column see if there's a cell at (x, y) */
  bobgui_tree_view_column_cell_set_cell_data (real_column,
                                           priv->model,
                                           &iter,
                                           BOBGUI_TREE_RBNODE_FLAG_SET (node, BOBGUI_TREE_RBNODE_IS_PARENT),
                                           node->children ? TRUE : FALSE);

  bobgui_tree_view_get_background_area (tree_view, real_path, real_column,
                                     &background_area);
  bobgui_tree_view_get_cell_area (tree_view, real_path, real_column,
                               &cell_area);

  if (!path)
    bobgui_tree_path_free (real_path);

  return _bobgui_tree_view_column_is_blank_at_pos (real_column,
                                                &cell_area,
                                                &background_area,
                                                x, y);
}

static void
unset_reorderable (BobguiTreeView *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  if (priv->reorderable)
    {
      priv->reorderable = FALSE;
      g_object_notify_by_pspec (G_OBJECT (tree_view), tree_view_props[PROP_REORDERABLE]);
    }
}

/**
 * bobgui_tree_view_enable_model_drag_source:
 * @tree_view: a `BobguiTreeView`
 * @start_button_mask: Mask of allowed buttons to start drag
 * @formats: the target formats that the drag will support
 * @actions: the bitmask of possible actions for a drag from this
 *    widget
 *
 * Turns @tree_view into a drag source for automatic DND. Calling this
 * method sets `BobguiTreeView`:reorderable to %FALSE.
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 **/
void
bobgui_tree_view_enable_model_drag_source (BobguiTreeView       *tree_view,
					GdkModifierType    start_button_mask,
					GdkContentFormats *formats,
					GdkDragAction      actions)
{
  TreeViewDragInfo *di;

  g_return_if_fail (BOBGUI_IS_TREE_VIEW (tree_view));

  di = ensure_info (tree_view);

  di->source_formats = gdk_content_formats_ref (formats);
  di->source_actions = actions;
  di->drag = NULL;

  di->start_button_mask = start_button_mask;
  di->source_set = TRUE;

  unset_reorderable (tree_view);
}

/**
 * bobgui_tree_view_enable_model_drag_dest:
 * @tree_view: a `BobguiTreeView`
 * @formats: the target formats that the drag will support
 * @actions: the bitmask of possible actions for a drag from this
 *    widget
 *
 * Turns @tree_view into a drop destination for automatic DND. Calling
 * this method sets `BobguiTreeView`:reorderable to %FALSE.
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 **/
void
bobgui_tree_view_enable_model_drag_dest (BobguiTreeView       *tree_view,
				      GdkContentFormats *formats,
				      GdkDragAction      actions)
{
  TreeViewDragInfo *di;
  BobguiCssNode *widget_node;

  g_return_if_fail (BOBGUI_IS_TREE_VIEW (tree_view));

  di = ensure_info (tree_view);
  di->dest_set = TRUE;

  di->dest = bobgui_drop_target_async_new (gdk_content_formats_ref (formats), actions);
  g_signal_connect (di->dest, "drag-leave", G_CALLBACK (bobgui_tree_view_drag_leave), tree_view);
  g_signal_connect (di->dest, "drag-enter", G_CALLBACK (bobgui_tree_view_drag_motion), tree_view);
  g_signal_connect (di->dest, "drag-motion", G_CALLBACK (bobgui_tree_view_drag_motion), tree_view);
  g_signal_connect (di->dest, "drop", G_CALLBACK (bobgui_tree_view_drag_drop), tree_view);
  bobgui_widget_add_controller (BOBGUI_WIDGET (tree_view), BOBGUI_EVENT_CONTROLLER (di->dest));
  g_object_ref (di->dest);

  widget_node = bobgui_widget_get_css_node (BOBGUI_WIDGET (tree_view));
  di->cssnode = bobgui_css_node_new ();
  bobgui_css_node_set_name (di->cssnode, g_quark_from_static_string ("dndtarget"));
  bobgui_css_node_set_parent (di->cssnode, widget_node);
  bobgui_css_node_set_state (di->cssnode, bobgui_css_node_get_state (widget_node));
  g_object_unref (di->cssnode);

  unset_reorderable (tree_view);
}

/**
 * bobgui_tree_view_unset_rows_drag_source:
 * @tree_view: a `BobguiTreeView`
 *
 * Undoes the effect of
 * bobgui_tree_view_enable_model_drag_source(). Calling this method sets
 * `BobguiTreeView`:reorderable to %FALSE.
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 **/
void
bobgui_tree_view_unset_rows_drag_source (BobguiTreeView *tree_view)
{
  TreeViewDragInfo *di;

  g_return_if_fail (BOBGUI_IS_TREE_VIEW (tree_view));

  di = get_info (tree_view);

  if (di)
    {
      if (di->source_set)
        {
          g_clear_pointer (&di->source_formats, gdk_content_formats_unref);
          di->source_set = FALSE;
        }

      if (!di->dest_set && !di->source_set)
        remove_info (tree_view);
    }

  unset_reorderable (tree_view);
}

/**
 * bobgui_tree_view_unset_rows_drag_dest:
 * @tree_view: a `BobguiTreeView`
 *
 * Undoes the effect of
 * bobgui_tree_view_enable_model_drag_dest(). Calling this method sets
 * `BobguiTreeView`:reorderable to %FALSE.
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 **/
void
bobgui_tree_view_unset_rows_drag_dest (BobguiTreeView *tree_view)
{
  TreeViewDragInfo *di;

  g_return_if_fail (BOBGUI_IS_TREE_VIEW (tree_view));

  di = get_info (tree_view);

  if (di)
    {
      if (di->dest_set)
        {
          bobgui_widget_remove_controller (BOBGUI_WIDGET (tree_view), BOBGUI_EVENT_CONTROLLER (di->dest));
          di->dest = NULL;
          di->dest_set = FALSE;

          bobgui_css_node_set_parent (di->cssnode, NULL);
          di->cssnode = NULL;
        }

      if (!di->dest_set && !di->source_set)
        remove_info (tree_view);
    }

  unset_reorderable (tree_view);
}

/**
 * bobgui_tree_view_set_drag_dest_row:
 * @tree_view: a `BobguiTreeView`
 * @path: (nullable): The path of the row to highlight
 * @pos: Specifies whether to drop before, after or into the row
 *
 * Sets the row that is highlighted for feedback.
 * If @path is %NULL, an existing highlight is removed.
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 */
void
bobgui_tree_view_set_drag_dest_row (BobguiTreeView            *tree_view,
                                 BobguiTreePath            *path,
                                 BobguiTreeViewDropPosition pos)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  BobguiTreePath *current_dest;

  /* Note; this function is exported to allow a custom DND
   * implementation, so it can't touch TreeViewDragInfo
   */

  g_return_if_fail (BOBGUI_IS_TREE_VIEW (tree_view));

  current_dest = NULL;

  if (priv->drag_dest_row)
    {
      current_dest = bobgui_tree_row_reference_get_path (priv->drag_dest_row);
      bobgui_tree_row_reference_free (priv->drag_dest_row);
    }

  /* special case a drop on an empty model */
  priv->empty_view_drop = 0;

  if (pos == BOBGUI_TREE_VIEW_DROP_BEFORE && path
      && bobgui_tree_path_get_depth (path) == 1
      && bobgui_tree_path_get_indices (path)[0] == 0)
    {
      int n_children;

      n_children = bobgui_tree_model_iter_n_children (priv->model,
                                                   NULL);

      if (!n_children)
        priv->empty_view_drop = 1;
    }

  priv->drag_dest_pos = pos;

  if (path)
    {
      priv->drag_dest_row =
        bobgui_tree_row_reference_new_proxy (G_OBJECT (tree_view), priv->model, path);
      bobgui_widget_queue_draw (BOBGUI_WIDGET (tree_view));
    }
  else
    priv->drag_dest_row = NULL;

  if (current_dest)
    {
      bobgui_widget_queue_draw (BOBGUI_WIDGET (tree_view));

      bobgui_tree_path_free (current_dest);
    }
}

/**
 * bobgui_tree_view_get_drag_dest_row:
 * @tree_view: a `BobguiTreeView`
 * @path: (out) (optional) (nullable): Return location for the path of the highlighted row
 * @pos: (out) (optional): Return location for the drop position
 *
 * Gets information about the row that is highlighted for feedback.
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 **/
void
bobgui_tree_view_get_drag_dest_row (BobguiTreeView              *tree_view,
                                 BobguiTreePath             **path,
                                 BobguiTreeViewDropPosition  *pos)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  g_return_if_fail (BOBGUI_IS_TREE_VIEW (tree_view));

  if (path)
    {
      if (priv->drag_dest_row)
        *path = bobgui_tree_row_reference_get_path (priv->drag_dest_row);
      else
        {
          if (priv->empty_view_drop)
            *path = bobgui_tree_path_new_from_indices (0, -1);
          else
            *path = NULL;
        }
    }

  if (pos)
    *pos = priv->drag_dest_pos;
}

/**
 * bobgui_tree_view_get_dest_row_at_pos:
 * @tree_view: a `BobguiTreeView`
 * @drag_x: the position to determine the destination row for
 * @drag_y: the position to determine the destination row for
 * @path: (out) (optional) (nullable): Return location for the path of
 *   the highlighted row
 * @pos: (out) (optional): Return location for the drop position, or
 *   %NULL
 *
 * Determines the destination row for a given position.  @drag_x and
 * @drag_y are expected to be in widget coordinates.  This function is only
 * meaningful if @tree_view is realized.  Therefore this function will always
 * return %FALSE if @tree_view is not realized or does not have a model.
 *
 * Returns: whether there is a row at the given position, %TRUE if this
 * is indeed the case.
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 **/
gboolean
bobgui_tree_view_get_dest_row_at_pos (BobguiTreeView             *tree_view,
                                   int                      drag_x,
                                   int                      drag_y,
                                   BobguiTreePath            **path,
                                   BobguiTreeViewDropPosition *pos)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  int cell_y;
  int bin_x, bin_y;
  double offset_into_row;
  double fourth;
  GdkRectangle cell;
  BobguiTreeViewColumn *column = NULL;
  BobguiTreePath *tmp_path = NULL;

  /* Note; this function is exported to allow a custom DND
   * implementation, so it can't touch TreeViewDragInfo
   */

  g_return_val_if_fail (tree_view != NULL, FALSE);
  g_return_val_if_fail (drag_x >= 0, FALSE);
  g_return_val_if_fail (drag_y >= 0, FALSE);

  if (path)
    *path = NULL;

  if (priv->tree == NULL)
    return FALSE;

  /* If in the top fourth of a row, we drop before that row; if
   * in the bottom fourth, drop after that row; if in the middle,
   * and the row has children, drop into the row.
   */
  bobgui_tree_view_convert_widget_to_bin_window_coords (tree_view, drag_x, drag_y,
						      &bin_x, &bin_y);

  if (!bobgui_tree_view_get_path_at_pos (tree_view,
				      bin_x,
				      bin_y,
                                      &tmp_path,
                                      &column,
                                      NULL,
                                      &cell_y))
    return FALSE;

  bobgui_tree_view_get_background_area (tree_view, tmp_path, column,
                                     &cell);

  offset_into_row = cell_y;

  if (path)
    *path = tmp_path;
  else
    bobgui_tree_path_free (tmp_path);

  tmp_path = NULL;

  fourth = cell.height / 4.0;

  if (pos)
    {
      if (offset_into_row < fourth)
        {
          *pos = BOBGUI_TREE_VIEW_DROP_BEFORE;
        }
      else if (offset_into_row < (cell.height / 2.0))
        {
          *pos = BOBGUI_TREE_VIEW_DROP_INTO_OR_BEFORE;
        }
      else if (offset_into_row < cell.height - fourth)
        {
          *pos = BOBGUI_TREE_VIEW_DROP_INTO_OR_AFTER;
        }
      else
        {
          *pos = BOBGUI_TREE_VIEW_DROP_AFTER;
        }
    }

  return TRUE;
}


static void
bobgui_treeview_snapshot_border (BobguiSnapshot           *snapshot,
                              const graphene_rect_t *rect)
{
  GskRoundedRect rounded;

  gsk_rounded_rect_init_from_rect (&rounded, rect, 0);

#define BLACK { 0, 0, 0, 1 }
  bobgui_snapshot_append_border (snapshot,
                              &rounded,
                              (float[4]) { 1, 1, 1, 1 },
                              (GdkRGBA[4]) { BLACK, BLACK, BLACK, BLACK });
#undef BLACK
}

/* KEEP IN SYNC WITH BOBGUI_TREE_VIEW_BIN_EXPOSE */
/**
 * bobgui_tree_view_create_row_drag_icon:
 * @tree_view: a `BobguiTreeView`
 * @path: a `BobguiTreePath` in @tree_view
 *
 * Creates a `cairo_surface_t` representation of the row at @path.
 * This image is used for a drag icon.
 *
 * Returns: (transfer full) (nullable): a newly-allocated surface of the drag icon.
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 **/
GdkPaintable *
bobgui_tree_view_create_row_drag_icon (BobguiTreeView  *tree_view,
                                    BobguiTreePath  *path)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  BobguiTreeIter iter;
  BobguiTreeRBTree *tree;
  BobguiTreeRBNode *node;
  BobguiStyleContext *context;
  int cell_offset;
  GList *list;
  GdkRectangle background_area;
  BobguiWidget *widget;
  BobguiSnapshot *snapshot;
  GdkPaintable *paintable;
  int depth;
  /* start drawing inside the black outline */
  int x = 1, y = 1;
  int bin_window_width;
  gboolean is_separator = FALSE;
  gboolean rtl;

  g_return_val_if_fail (BOBGUI_IS_TREE_VIEW (tree_view), NULL);
  g_return_val_if_fail (path != NULL, NULL);

  widget = BOBGUI_WIDGET (tree_view);

  if (!bobgui_widget_get_realized (widget))
    return NULL;

  depth = bobgui_tree_path_get_depth (path);

  _bobgui_tree_view_find_node (tree_view,
                            path,
                            &tree,
                            &node);

  if (tree == NULL)
    return NULL;

  if (!bobgui_tree_model_get_iter (priv->model,
                                &iter,
                                path))
    return NULL;

  context = bobgui_widget_get_style_context (widget);

  is_separator = row_is_separator (tree_view, &iter, NULL);

  cell_offset = x;

  background_area.y = y;
  background_area.height = bobgui_tree_view_get_row_height (tree_view, node);

  bin_window_width = bobgui_widget_get_width (BOBGUI_WIDGET (tree_view));

  snapshot = bobgui_snapshot_new ();

  bobgui_snapshot_render_background (snapshot, context,
                                  0, 0,
                                  bin_window_width + 2,
                                  background_area.height + 2);

  rtl = bobgui_widget_get_direction (BOBGUI_WIDGET (tree_view)) == BOBGUI_TEXT_DIR_RTL;

  for (list = (rtl ? g_list_last (priv->columns) : g_list_first (priv->columns));
      list;
      list = (rtl ? list->prev : list->next))
    {
      BobguiTreeViewColumn *column = list->data;
      GdkRectangle cell_area;

      if (!bobgui_tree_view_column_get_visible (column))
        continue;

      bobgui_tree_view_column_cell_set_cell_data (column, priv->model, &iter,
					       BOBGUI_TREE_RBNODE_FLAG_SET (node, BOBGUI_TREE_RBNODE_IS_PARENT),
					       node->children?TRUE:FALSE);

      background_area.x = cell_offset;
      background_area.width = bobgui_tree_view_column_get_width (column);

      cell_area = background_area;

      if (bobgui_tree_view_is_expander_column (tree_view, column))
        {
	  if (!rtl)
	    cell_area.x += (depth - 1) * priv->level_indentation;
	  cell_area.width -= (depth - 1) * priv->level_indentation;

          if (bobgui_tree_view_draw_expanders (tree_view))
	    {
              int expander_size = bobgui_tree_view_get_expander_size (tree_view);
	      if (!rtl)
		cell_area.x += depth * expander_size;
	      cell_area.width -= depth * expander_size;
	    }
        }

      if (bobgui_tree_view_column_cell_is_visible (column))
	{
	  if (is_separator)
            {
              GdkRGBA color;

              bobgui_style_context_save (context);
              bobgui_style_context_add_class (context, "separator");

              bobgui_style_context_get_color (context, &color);
              bobgui_snapshot_append_color (snapshot,
                                         &color,
                                         &GRAPHENE_RECT_INIT(
                                             cell_area.x,
                                             cell_area.y + cell_area.height / 2,
                                             cell_area.x + cell_area.width,
                                             1
                                         ));

              bobgui_style_context_restore (context);
            }
	  else
            {
              bobgui_tree_view_column_cell_snapshot (column,
                                                  snapshot,
                                                  &background_area,
                                                  &cell_area,
                                                  0, FALSE);
            }
	}
      cell_offset += bobgui_tree_view_column_get_width (column);
    }

  bobgui_treeview_snapshot_border (snapshot,
                                &GRAPHENE_RECT_INIT(0, 0, bin_window_width + 2, background_area.height + 2));

  paintable = bobgui_snapshot_free_to_paintable (snapshot, NULL);

  return paintable;
}


/*
 * Interactive search
 */

/**
 * bobgui_tree_view_set_enable_search:
 * @tree_view: A `BobguiTreeView`
 * @enable_search: %TRUE, if the user can search interactively
 *
 * If @enable_search is set, then the user can type in text to search through
 * the tree interactively (this is sometimes called "typeahead find").
 *
 * Note that even if this is %FALSE, the user can still initiate a search
 * using the “start-interactive-search” key binding.
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 */
void
bobgui_tree_view_set_enable_search (BobguiTreeView *tree_view,
				 gboolean     enable_search)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  g_return_if_fail (BOBGUI_IS_TREE_VIEW (tree_view));

  enable_search = !!enable_search;

  if (priv->enable_search != enable_search)
    {
       priv->enable_search = enable_search;
       g_object_notify_by_pspec (G_OBJECT (tree_view), tree_view_props[PROP_ENABLE_SEARCH]);
    }
}

/**
 * bobgui_tree_view_get_enable_search:
 * @tree_view: A `BobguiTreeView`
 *
 * Returns whether or not the tree allows to start interactive searching
 * by typing in text.
 *
 * Returns: whether or not to let the user search interactively
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 */
gboolean
bobgui_tree_view_get_enable_search (BobguiTreeView *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  g_return_val_if_fail (BOBGUI_IS_TREE_VIEW (tree_view), FALSE);

  return priv->enable_search;
}


/**
 * bobgui_tree_view_get_search_column:
 * @tree_view: A `BobguiTreeView`
 *
 * Gets the column searched on by the interactive search code.
 *
 * Returns: the column the interactive search code searches in.
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 */
int
bobgui_tree_view_get_search_column (BobguiTreeView *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  g_return_val_if_fail (BOBGUI_IS_TREE_VIEW (tree_view), -1);

  return priv->search_column;
}

/**
 * bobgui_tree_view_set_search_column:
 * @tree_view: A `BobguiTreeView`
 * @column: the column of the model to search in, or -1 to disable searching
 *
 * Sets @column as the column where the interactive search code should
 * search in for the current model.
 *
 * If the search column is set, users can use the “start-interactive-search”
 * key binding to bring up search popup. The enable-search property controls
 * whether simply typing text will also start an interactive search.
 *
 * Note that @column refers to a column of the current model. The search
 * column is reset to -1 when the model is changed.
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 */
void
bobgui_tree_view_set_search_column (BobguiTreeView *tree_view,
				 int          column)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  g_return_if_fail (BOBGUI_IS_TREE_VIEW (tree_view));
  g_return_if_fail (column >= -1);

  if (priv->search_column == column)
    return;

  priv->search_column = column;
  g_object_notify_by_pspec (G_OBJECT (tree_view), tree_view_props[PROP_SEARCH_COLUMN]);
}

/**
 * bobgui_tree_view_get_search_equal_func: (skip)
 * @tree_view: A `BobguiTreeView`
 *
 * Returns the compare function currently in use.
 *
 * Returns: the currently used compare function for the search code.
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 */

BobguiTreeViewSearchEqualFunc
bobgui_tree_view_get_search_equal_func (BobguiTreeView *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  g_return_val_if_fail (BOBGUI_IS_TREE_VIEW (tree_view), NULL);

  return priv->search_equal_func;
}

/**
 * bobgui_tree_view_set_search_equal_func:
 * @tree_view: A `BobguiTreeView`
 * @search_equal_func: the compare function to use during the search
 * @search_user_data: (nullable): user data to pass to @search_equal_func
 * @search_destroy: (nullable): Destroy notifier for @search_user_data
 *
 * Sets the compare function for the interactive search capabilities; note
 * that somewhat like strcmp() returning 0 for equality
 * `BobguiTreeView`SearchEqualFunc returns %FALSE on matches.
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 **/
void
bobgui_tree_view_set_search_equal_func (BobguiTreeView                *tree_view,
				     BobguiTreeViewSearchEqualFunc  search_equal_func,
				     gpointer                    search_user_data,
				     GDestroyNotify              search_destroy)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  g_return_if_fail (BOBGUI_IS_TREE_VIEW (tree_view));
  g_return_if_fail (search_equal_func != NULL);

  if (priv->search_destroy)
    priv->search_destroy (priv->search_user_data);

  priv->search_equal_func = search_equal_func;
  priv->search_user_data = search_user_data;
  priv->search_destroy = search_destroy;
  if (priv->search_equal_func == NULL)
    priv->search_equal_func = bobgui_tree_view_search_equal_func;
}

/**
 * bobgui_tree_view_get_search_entry:
 * @tree_view: A `BobguiTreeView`
 *
 * Returns the `BobguiEntry` which is currently in use as interactive search
 * entry for @tree_view.  In case the built-in entry is being used, %NULL
 * will be returned.
 *
 * Returns: (transfer none) (nullable): the entry currently in use as search entry.
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 */
BobguiEditable *
bobgui_tree_view_get_search_entry (BobguiTreeView *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  g_return_val_if_fail (BOBGUI_IS_TREE_VIEW (tree_view), NULL);

  if (priv->search_custom_entry_set)
    return BOBGUI_EDITABLE (priv->search_entry);

  return NULL;
}

/**
 * bobgui_tree_view_set_search_entry:
 * @tree_view: A `BobguiTreeView`
 * @entry: (nullable): the entry the interactive search code of @tree_view should use
 *
 * Sets the entry which the interactive search code will use for this
 * @tree_view.  This is useful when you want to provide a search entry
 * in our interface at all time at a fixed position.  Passing %NULL for
 * @entry will make the interactive search code use the built-in popup
 * entry again.
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 */
void
bobgui_tree_view_set_search_entry (BobguiTreeView *tree_view,
				BobguiEditable *entry)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  g_return_if_fail (BOBGUI_IS_TREE_VIEW (tree_view));
  g_return_if_fail (entry == NULL || BOBGUI_IS_ENTRY (entry) || BOBGUI_IS_SEARCH_ENTRY (entry));

  if (priv->search_custom_entry_set)
    {
      if (priv->search_entry_changed_id)
        {
	  g_signal_handler_disconnect (priv->search_entry,
				       priv->search_entry_changed_id);
	  priv->search_entry_changed_id = 0;
	}

      g_signal_handlers_disconnect_by_func (bobgui_entry_get_key_controller (BOBGUI_ENTRY (priv->search_entry)),
					    G_CALLBACK (bobgui_tree_view_search_key_pressed),
					    tree_view);

      g_object_unref (priv->search_entry);
    }
  else if (priv->search_popover)
    {
      bobgui_tree_view_destroy_search_popover (tree_view);
    }

  if (entry)
    {
      BobguiEventController *controller;

      priv->search_entry = BOBGUI_WIDGET (g_object_ref (entry));
      priv->search_custom_entry_set = TRUE;

      if (priv->search_entry_changed_id == 0)
        {
          priv->search_entry_changed_id =
	    g_signal_connect (priv->search_entry, "changed",
			      G_CALLBACK (bobgui_tree_view_search_init),
			      tree_view);
	}

      if (BOBGUI_IS_ENTRY (entry))
        controller = bobgui_entry_get_key_controller (BOBGUI_ENTRY (entry));
      else
        controller = bobgui_search_entry_get_key_controller (BOBGUI_SEARCH_ENTRY (entry));
      g_signal_connect (controller, "key-pressed",
                        G_CALLBACK (bobgui_tree_view_search_key_pressed), tree_view);

      bobgui_tree_view_search_init (priv->search_entry, tree_view);
    }
  else
    {
      priv->search_entry = NULL;
      priv->search_custom_entry_set = FALSE;
    }
}

static void
bobgui_tree_view_search_popover_hide (BobguiWidget   *search_popover,
                                   BobguiTreeView *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  if (priv->disable_popdown)
    return;

  if (priv->search_entry_changed_id)
    {
      g_signal_handler_disconnect (priv->search_entry,
				   priv->search_entry_changed_id);
      priv->search_entry_changed_id = 0;
    }
  if (priv->typeselect_flush_timeout)
    {
      g_source_remove (priv->typeselect_flush_timeout);
      priv->typeselect_flush_timeout = 0;
    }

  if (bobgui_widget_get_visible (search_popover))
    {
      bobgui_popover_popdown (BOBGUI_POPOVER (search_popover));
      bobgui_editable_set_text (BOBGUI_EDITABLE (priv->search_entry), "");
      bobgui_widget_grab_focus (BOBGUI_WIDGET (tree_view));
    }
}

/* Because we're visible but offscreen, we just set a flag in the preedit
 * callback.
 */
static void
bobgui_tree_view_search_preedit_changed (BobguiText      *text,
                                      const char   *predit,
				      BobguiTreeView  *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  priv->imcontext_changed = 1;
  if (priv->typeselect_flush_timeout)
    {
      g_source_remove (priv->typeselect_flush_timeout);
      priv->typeselect_flush_timeout =
	g_timeout_add (BOBGUI_TREE_VIEW_SEARCH_DIALOG_TIMEOUT,
                       (GSourceFunc) bobgui_tree_view_search_entry_flush_timeout,
		       tree_view);
      gdk_source_set_static_name_by_id (priv->typeselect_flush_timeout, "[bobgui] bobgui_tree_view_search_entry_flush_timeout");
    }

}

static void
bobgui_tree_view_search_changed (BobguiEditable *editable,
                              BobguiTreeView  *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  priv->imcontext_changed = 1;
}

static void
bobgui_tree_view_search_activate (BobguiEntry    *entry,
			       BobguiTreeView *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  BobguiTreePath *path;

  bobgui_tree_view_search_popover_hide (priv->search_popover, tree_view);

  /* If we have a row selected and it's the cursor row, we activate
   * the row XXX */
  if (priv->cursor_node &&
      BOBGUI_TREE_RBNODE_FLAG_SET (priv->cursor_node, BOBGUI_TREE_RBNODE_IS_SELECTED))
    {
      path = _bobgui_tree_path_new_from_rbtree (priv->cursor_tree,
                                             priv->cursor_node);

      bobgui_tree_view_row_activated (tree_view, path, priv->focus_column);

      bobgui_tree_path_free (path);
    }
}

static void
bobgui_tree_view_search_pressed_cb (BobguiGesture  *gesture,
                                 int          n_press,
                                 double       x,
                                 double       y,
                                 BobguiTreeView *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  bobgui_tree_view_search_popover_hide (priv->search_popover, tree_view);
}

static gboolean
bobgui_tree_view_search_scroll_event (BobguiWidget   *widget,
                                   double       dx,
                                   double       dy,
				   BobguiTreeView *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  GdkScrollDirection direction;

  direction = dy > 0 ? GDK_SCROLL_DOWN : GDK_SCROLL_UP;

  if (direction == GDK_SCROLL_UP)
    bobgui_tree_view_search_move (widget, tree_view, TRUE);
  else if (direction == GDK_SCROLL_DOWN)
    bobgui_tree_view_search_move (widget, tree_view, FALSE);

  /* renew the flush timeout */
  if (priv->typeselect_flush_timeout &&
      !priv->search_custom_entry_set)
    {
      g_source_remove (priv->typeselect_flush_timeout);
      priv->typeselect_flush_timeout =
	g_timeout_add (BOBGUI_TREE_VIEW_SEARCH_DIALOG_TIMEOUT,
		       (GSourceFunc) bobgui_tree_view_search_entry_flush_timeout,
		       tree_view);
      gdk_source_set_static_name_by_id (priv->typeselect_flush_timeout, "[bobgui] bobgui_tree_view_search_entry_flush_timeout");
    }

  return GDK_EVENT_STOP;
}

static gboolean
bobgui_tree_view_search_key_pressed (BobguiEventControllerKey *key,
                                  guint                  keyval,
                                  guint                  keycode,
                                  GdkModifierType        state,
                                  BobguiTreeView           *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  BobguiWidget *widget = priv->search_entry;
#ifdef __APPLE__
  GdkModifierType default_accel = GDK_META_MASK;
#else
  GdkModifierType default_accel = GDK_CONTROL_MASK;
#endif
  gboolean retval = FALSE;

  g_return_val_if_fail (BOBGUI_IS_WIDGET (widget), FALSE);
  g_return_val_if_fail (BOBGUI_IS_TREE_VIEW (tree_view), FALSE);

  /* close window and cancel the search */
  if (!priv->search_custom_entry_set
      && bobgui_tree_view_search_key_cancels_search (keyval))
    {
      bobgui_tree_view_search_popover_hide (priv->search_popover, tree_view);
      return TRUE;
    }

  /* select previous matching iter */
  if (keyval == GDK_KEY_Up || keyval == GDK_KEY_KP_Up)
    {
      if (!bobgui_tree_view_search_move (widget, tree_view, TRUE))
        bobgui_widget_error_bell (widget);

      retval = TRUE;
    }

  if (((state & (default_accel | GDK_SHIFT_MASK)) == (default_accel | GDK_SHIFT_MASK))
      && (keyval == GDK_KEY_g || keyval == GDK_KEY_G))
    {
      if (!bobgui_tree_view_search_move (widget, tree_view, TRUE))
        bobgui_widget_error_bell (widget);

      retval = TRUE;
    }

  /* select next matching iter */
  if (keyval == GDK_KEY_Down || keyval == GDK_KEY_KP_Down)
    {
      if (!bobgui_tree_view_search_move (widget, tree_view, FALSE))
        bobgui_widget_error_bell (widget);

      retval = TRUE;
    }

  if (((state & (default_accel | GDK_SHIFT_MASK)) == default_accel)
      && (keyval == GDK_KEY_g || keyval == GDK_KEY_G))
    {
      if (!bobgui_tree_view_search_move (widget, tree_view, FALSE))
        bobgui_widget_error_bell (widget);

      retval = TRUE;
    }

  /* renew the flush timeout */
  if (retval && priv->typeselect_flush_timeout
      && !priv->search_custom_entry_set)
    {
      g_source_remove (priv->typeselect_flush_timeout);
      priv->typeselect_flush_timeout =
	g_timeout_add (BOBGUI_TREE_VIEW_SEARCH_DIALOG_TIMEOUT,
		       (GSourceFunc) bobgui_tree_view_search_entry_flush_timeout,
		       tree_view);
      gdk_source_set_static_name_by_id (priv->typeselect_flush_timeout, "[bobgui] bobgui_tree_view_search_entry_flush_timeout");
    }

  if (!retval)
    bobgui_event_controller_key_forward (key, priv->search_entry);

  return retval;
}

/*  this function returns FALSE if there is a search string but
 *  nothing was found, and TRUE otherwise.
 */
static gboolean
bobgui_tree_view_search_move (BobguiWidget   *popover,
			   BobguiTreeView *tree_view,
			   gboolean     up)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  gboolean ret;
  int len;
  int count = 0;
  const char *text;
  BobguiTreeIter iter;
  BobguiTreeModel *model;
  BobguiTreeSelection *selection;

  text = bobgui_editable_get_text (BOBGUI_EDITABLE (priv->search_entry));

  g_return_val_if_fail (text != NULL, FALSE);

  len = strlen (text);

  if (up && priv->selected_iter == 1)
    return len < 1;

  if (len < 1)
    return TRUE;

  model = bobgui_tree_view_get_model (tree_view);
  selection = bobgui_tree_view_get_selection (tree_view);

  /* search */
  bobgui_tree_selection_unselect_all (selection);
  if (!bobgui_tree_model_get_iter_first (model, &iter))
    return TRUE;

  ret = bobgui_tree_view_search_iter (model, selection, &iter, text,
				   &count, up?((priv->selected_iter) - 1):((priv->selected_iter + 1)));

  if (ret)
    {
      /* found */
      priv->selected_iter += up?(-1):(1);
      return TRUE;
    }
  else
    {
      /* return to old iter */
      count = 0;
      bobgui_tree_model_get_iter_first (model, &iter);
      bobgui_tree_view_search_iter (model, selection,
				 &iter, text,
				 &count, priv->selected_iter);
      return FALSE;
    }
}

static gboolean
bobgui_tree_view_search_equal_func (BobguiTreeModel *model,
				 int           column,
				 const char   *key,
				 BobguiTreeIter  *iter,
				 gpointer      search_data)
{
  gboolean retval = TRUE;
  const char *str;
  char *normalized_string;
  char *normalized_key;
  char *case_normalized_string = NULL;
  char *case_normalized_key = NULL;
  GValue value = G_VALUE_INIT;
  GValue transformed = G_VALUE_INIT;

  bobgui_tree_model_get_value (model, iter, column, &value);

  g_value_init (&transformed, G_TYPE_STRING);

  if (!g_value_transform (&value, &transformed))
    {
      g_value_unset (&value);
      return TRUE;
    }

  g_value_unset (&value);

  str = g_value_get_string (&transformed);
  if (!str)
    {
      g_value_unset (&transformed);
      return TRUE;
    }

  normalized_string = g_utf8_normalize (str, -1, G_NORMALIZE_ALL);
  normalized_key = g_utf8_normalize (key, -1, G_NORMALIZE_ALL);

  if (normalized_string && normalized_key)
    {
      case_normalized_string = g_utf8_casefold (normalized_string, -1);
      case_normalized_key = g_utf8_casefold (normalized_key, -1);

      if (strncmp (case_normalized_key, case_normalized_string, strlen (case_normalized_key)) == 0)
        retval = FALSE;
    }

  g_value_unset (&transformed);
  g_free (normalized_key);
  g_free (normalized_string);
  g_free (case_normalized_key);
  g_free (case_normalized_string);

  return retval;
}

static gboolean
bobgui_tree_view_search_iter (BobguiTreeModel     *model,
			   BobguiTreeSelection *selection,
			   BobguiTreeIter      *iter,
			   const char       *text,
			   int              *count,
			   int               n)
{
  BobguiTreeRBTree *tree = NULL;
  BobguiTreeRBNode *node = NULL;
  BobguiTreePath *path;

  BobguiTreeView *tree_view = bobgui_tree_selection_get_tree_view (selection);
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  path = bobgui_tree_model_get_path (model, iter);
  _bobgui_tree_view_find_node (tree_view, path, &tree, &node);

  do
    {
      if (! priv->search_equal_func (model, priv->search_column, text, iter, priv->search_user_data))
        {
          (*count)++;
          if (*count == n)
            {
              bobgui_tree_view_scroll_to_cell (tree_view, path, NULL,
					    TRUE, 0.5, 0.0);
              bobgui_tree_selection_select_iter (selection, iter);
              bobgui_tree_view_real_set_cursor (tree_view, path, CLAMP_NODE);

	      if (path)
		bobgui_tree_path_free (path);

              return TRUE;
            }
        }

      if (node->children)
	{
	  gboolean has_child;
	  BobguiTreeIter tmp;

	  tree = node->children;
          node = bobgui_tree_rbtree_first (tree);

	  tmp = *iter;
	  has_child = bobgui_tree_model_iter_children (model, iter, &tmp);
	  bobgui_tree_path_down (path);

	  /* sanity check */
	  TREE_VIEW_INTERNAL_ASSERT (has_child, FALSE);
	}
      else
	{
	  gboolean done = FALSE;

	  do
	    {
	      node = bobgui_tree_rbtree_next (tree, node);

	      if (node)
		{
		  gboolean has_next;

		  has_next = bobgui_tree_model_iter_next (model, iter);

		  done = TRUE;
		  bobgui_tree_path_next (path);

		  /* sanity check */
		  TREE_VIEW_INTERNAL_ASSERT (has_next, FALSE);
		}
	      else
		{
		  gboolean has_parent;
		  BobguiTreeIter tmp_iter = *iter;

		  node = tree->parent_node;
		  tree = tree->parent_tree;

		  if (!tree)
		    {
		      if (path)
			bobgui_tree_path_free (path);

		      /* we've run out of tree, done with this func */
		      return FALSE;
		    }

		  has_parent = bobgui_tree_model_iter_parent (model,
							   iter,
							   &tmp_iter);
		  bobgui_tree_path_up (path);

		  /* sanity check */
		  TREE_VIEW_INTERNAL_ASSERT (has_parent, FALSE);
		}
	    }
	  while (!done);
	}
    }
  while (1);

  return FALSE;
}

static void
bobgui_tree_view_search_init (BobguiWidget   *entry,
			   BobguiTreeView *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  int ret;
  int count = 0;
  const char *text;
  BobguiTreeIter iter;
  BobguiTreeModel *model;
  BobguiTreeSelection *selection;

  g_return_if_fail (BOBGUI_IS_TREE_VIEW (tree_view));

  text = bobgui_editable_get_text (BOBGUI_EDITABLE (entry));

  model = bobgui_tree_view_get_model (tree_view);
  selection = bobgui_tree_view_get_selection (tree_view);

  /* search */
  bobgui_tree_selection_unselect_all (selection);
  if (priv->typeselect_flush_timeout
      && !priv->search_custom_entry_set)
    {
      g_source_remove (priv->typeselect_flush_timeout);
      priv->typeselect_flush_timeout =
	g_timeout_add (BOBGUI_TREE_VIEW_SEARCH_DIALOG_TIMEOUT,
		       (GSourceFunc) bobgui_tree_view_search_entry_flush_timeout,
		       tree_view);
      gdk_source_set_static_name_by_id (priv->typeselect_flush_timeout, "[bobgui] bobgui_tree_view_search_entry_flush_timeout");
    }

  if (*text == '\0')
    return;

  if (!bobgui_tree_model_get_iter_first (model, &iter))
    return;

  ret = bobgui_tree_view_search_iter (model, selection,
				   &iter, text,
				   &count, 1);

  if (ret)
    priv->selected_iter = 1;
}

void
_bobgui_tree_view_remove_editable (BobguiTreeView       *tree_view,
                                BobguiTreeViewColumn *column,
                                BobguiCellEditable   *cell_editable)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  if (priv->edited_column == NULL)
    return;

  g_return_if_fail (column == priv->edited_column);

  priv->edited_column = NULL;

  if (bobgui_widget_has_focus (BOBGUI_WIDGET (cell_editable)))
    bobgui_widget_grab_focus (BOBGUI_WIDGET (tree_view));

  bobgui_tree_view_remove (tree_view, BOBGUI_WIDGET (cell_editable));

  /* FIXME should only redraw a single node */
  bobgui_widget_queue_draw (BOBGUI_WIDGET (tree_view));
}

static gboolean
bobgui_tree_view_start_editing (BobguiTreeView *tree_view,
			     BobguiTreePath *cursor_path,
			     gboolean     edit_only)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  BobguiTreeIter iter;
  GdkRectangle cell_area;
  BobguiTreeViewColumn *focus_column;
  guint flags = 0; /* can be 0, as the flags are primarily for rendering */
  int retval = FALSE;
  BobguiTreeRBTree *cursor_tree;
  BobguiTreeRBNode *cursor_node;

  g_assert (priv->focus_column);
  focus_column = priv->focus_column;

  if (!bobgui_widget_get_realized (BOBGUI_WIDGET (tree_view)))
    return FALSE;

  if (_bobgui_tree_view_find_node (tree_view, cursor_path, &cursor_tree, &cursor_node) ||
      cursor_node == NULL)
    return FALSE;

  bobgui_tree_model_get_iter (priv->model, &iter, cursor_path);

  validate_row (tree_view, cursor_tree, cursor_node, &iter, cursor_path);

  bobgui_tree_view_column_cell_set_cell_data (focus_column,
                                           priv->model,
                                           &iter,
                                           BOBGUI_TREE_RBNODE_FLAG_SET (cursor_node, BOBGUI_TREE_RBNODE_IS_PARENT),
                                           cursor_node->children ? TRUE : FALSE);
  bobgui_tree_view_get_cell_area (tree_view,
                               cursor_path,
                               focus_column,
                               &cell_area);

  if (bobgui_cell_area_activate (bobgui_cell_layout_get_area (BOBGUI_CELL_LAYOUT (focus_column)),
                              _bobgui_tree_view_column_get_context (focus_column),
                              BOBGUI_WIDGET (tree_view),
                              &cell_area,
                              flags, edit_only))
    retval = TRUE;

  return retval;
}

void
_bobgui_tree_view_add_editable (BobguiTreeView       *tree_view,
                             BobguiTreeViewColumn *column,
                             BobguiTreePath       *path,
                             BobguiCellEditable   *cell_editable,
                             GdkRectangle      *cell_area)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  GdkRectangle full_area;
  BobguiBorder border;

  priv->edited_column = column;

  bobgui_tree_view_real_set_cursor (tree_view, path, CLAMP_NODE);

  priv->draw_keyfocus = TRUE;

  bobgui_tree_view_get_cell_area (tree_view, path, column, &full_area);
  border.left = cell_area->x - full_area.x;
  border.top = cell_area->y - full_area.y;
  border.right = (full_area.x + full_area.width) - (cell_area->x + cell_area->width);
  border.bottom = (full_area.y + full_area.height) - (cell_area->y + cell_area->height);

  bobgui_tree_view_put (tree_view,
                     BOBGUI_WIDGET (cell_editable),
                     path,
                     column,
                     &border);
}

static void
bobgui_tree_view_stop_editing (BobguiTreeView *tree_view,
			    gboolean     cancel_editing)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  BobguiTreeViewColumn *column;

  if (priv->edited_column == NULL)
    return;

  /*
   * This is very evil. We need to do this, because
   * bobgui_cell_editable_editing_done may trigger bobgui_tree_view_row_changed
   * later on. If bobgui_tree_view_row_changed notices
   * priv->edited_column != NULL, it'll call
   * bobgui_tree_view_stop_editing again. Bad things will happen then.
   *
   * Please read that again if you intend to modify anything here.
   */

  column = priv->edited_column;
  bobgui_cell_area_stop_editing (bobgui_cell_layout_get_area (BOBGUI_CELL_LAYOUT (column)), cancel_editing);
  priv->edited_column = NULL;
}


/**
 * bobgui_tree_view_set_hover_selection:
 * @tree_view: a `BobguiTreeView`
 * @hover: %TRUE to enable hover selection mode
 *
 * Enables or disables the hover selection mode of @tree_view.
 * Hover selection makes the selected row follow the pointer.
 * Currently, this works only for the selection modes
 * %BOBGUI_SELECTION_SINGLE and %BOBGUI_SELECTION_BROWSE.
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 **/
void
bobgui_tree_view_set_hover_selection (BobguiTreeView *tree_view,
				   gboolean     hover)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  hover = hover != FALSE;

  if (hover != priv->hover_selection)
    {
      priv->hover_selection = hover;

      g_object_notify_by_pspec (G_OBJECT (tree_view), tree_view_props[PROP_HOVER_SELECTION]);
    }
}

/**
 * bobgui_tree_view_get_hover_selection:
 * @tree_view: a `BobguiTreeView`
 *
 * Returns whether hover selection mode is turned on for @tree_view.
 *
 * Returns: %TRUE if @tree_view is in hover selection mode
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 **/
gboolean
bobgui_tree_view_get_hover_selection (BobguiTreeView *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  g_return_val_if_fail (BOBGUI_IS_TREE_VIEW (tree_view), FALSE);

  return priv->hover_selection;
}

/**
 * bobgui_tree_view_set_hover_expand:
 * @tree_view: a `BobguiTreeView`
 * @expand: %TRUE to enable hover selection mode
 *
 * Enables or disables the hover expansion mode of @tree_view.
 * Hover expansion makes rows expand or collapse if the pointer
 * moves over them.
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 **/
void
bobgui_tree_view_set_hover_expand (BobguiTreeView *tree_view,
				gboolean     expand)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  expand = expand != FALSE;

  if (expand != priv->hover_expand)
    {
      priv->hover_expand = expand;

      g_object_notify_by_pspec (G_OBJECT (tree_view), tree_view_props[PROP_HOVER_EXPAND]);
    }
}

/**
 * bobgui_tree_view_get_hover_expand:
 * @tree_view: a `BobguiTreeView`
 *
 * Returns whether hover expansion mode is turned on for @tree_view.
 *
 * Returns: %TRUE if @tree_view is in hover expansion mode
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 **/
gboolean
bobgui_tree_view_get_hover_expand (BobguiTreeView *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  g_return_val_if_fail (BOBGUI_IS_TREE_VIEW (tree_view), FALSE);

  return priv->hover_expand;
}

/**
 * bobgui_tree_view_set_rubber_banding:
 * @tree_view: a `BobguiTreeView`
 * @enable: %TRUE to enable rubber banding
 *
 * Enables or disables rubber banding in @tree_view.  If the selection mode
 * is %BOBGUI_SELECTION_MULTIPLE, rubber banding will allow the user to select
 * multiple rows by dragging the mouse.
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 **/
void
bobgui_tree_view_set_rubber_banding (BobguiTreeView *tree_view,
				  gboolean     enable)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  enable = enable != FALSE;

  if (enable != priv->rubber_banding_enable)
    {
      priv->rubber_banding_enable = enable;

      g_object_notify_by_pspec (G_OBJECT (tree_view), tree_view_props[PROP_RUBBER_BANDING]);
    }
}

/**
 * bobgui_tree_view_get_rubber_banding:
 * @tree_view: a `BobguiTreeView`
 *
 * Returns whether rubber banding is turned on for @tree_view.  If the
 * selection mode is %BOBGUI_SELECTION_MULTIPLE, rubber banding will allow the
 * user to select multiple rows by dragging the mouse.
 *
 * Returns: %TRUE if rubber banding in @tree_view is enabled.
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 **/
gboolean
bobgui_tree_view_get_rubber_banding (BobguiTreeView *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  return priv->rubber_banding_enable;
}

/**
 * bobgui_tree_view_is_rubber_banding_active:
 * @tree_view: a `BobguiTreeView`
 *
 * Returns whether a rubber banding operation is currently being done
 * in @tree_view.
 *
 * Returns: %TRUE if a rubber banding operation is currently being
 * done in @tree_view.
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 **/
gboolean
bobgui_tree_view_is_rubber_banding_active (BobguiTreeView *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  g_return_val_if_fail (BOBGUI_IS_TREE_VIEW (tree_view), FALSE);

  if (priv->rubber_banding_enable
      && priv->rubber_band_status == RUBBER_BAND_ACTIVE)
    return TRUE;

  return FALSE;
}

/**
 * bobgui_tree_view_get_row_separator_func: (skip)
 * @tree_view: a `BobguiTreeView`
 *
 * Returns the current row separator function.
 *
 * Returns: the current row separator function.
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 **/
BobguiTreeViewRowSeparatorFunc
bobgui_tree_view_get_row_separator_func (BobguiTreeView *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  g_return_val_if_fail (BOBGUI_IS_TREE_VIEW (tree_view), NULL);

  return priv->row_separator_func;
}

/**
 * bobgui_tree_view_set_row_separator_func:
 * @tree_view: a `BobguiTreeView`
 * @func: (nullable): a `BobguiTreeView`RowSeparatorFunc
 * @data: (nullable): user data to pass to @func
 * @destroy: (nullable): destroy notifier for @data
 *
 * Sets the row separator function, which is used to determine
 * whether a row should be drawn as a separator. If the row separator
 * function is %NULL, no separators are drawn. This is the default value.
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 **/
void
bobgui_tree_view_set_row_separator_func (BobguiTreeView                 *tree_view,
				      BobguiTreeViewRowSeparatorFunc  func,
				      gpointer                     data,
				      GDestroyNotify               destroy)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  g_return_if_fail (BOBGUI_IS_TREE_VIEW (tree_view));

  if (priv->row_separator_destroy)
    priv->row_separator_destroy (priv->row_separator_data);

  priv->row_separator_func = func;
  priv->row_separator_data = data;
  priv->row_separator_destroy = destroy;

  /* Have the tree recalculate heights */
  bobgui_tree_rbtree_mark_invalid (priv->tree);
  bobgui_widget_queue_resize (BOBGUI_WIDGET (tree_view));
}

/**
 * bobgui_tree_view_get_grid_lines:
 * @tree_view: a `BobguiTreeView`
 *
 * Returns which grid lines are enabled in @tree_view.
 *
 * Returns: a `BobguiTreeView`GridLines value indicating which grid lines
 * are enabled.
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 */
BobguiTreeViewGridLines
bobgui_tree_view_get_grid_lines (BobguiTreeView *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  g_return_val_if_fail (BOBGUI_IS_TREE_VIEW (tree_view), 0);

  return priv->grid_lines;
}

/**
 * bobgui_tree_view_set_grid_lines:
 * @tree_view: a `BobguiTreeView`
 * @grid_lines: a `BobguiTreeView`GridLines value indicating which grid lines to
 * enable.
 *
 * Sets which grid lines to draw in @tree_view.
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 */
void
bobgui_tree_view_set_grid_lines (BobguiTreeView           *tree_view,
			      BobguiTreeViewGridLines   grid_lines)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  BobguiTreeViewGridLines old_grid_lines;

  g_return_if_fail (BOBGUI_IS_TREE_VIEW (tree_view));

  old_grid_lines = priv->grid_lines;
  priv->grid_lines = grid_lines;

  if (old_grid_lines != grid_lines)
    {
      bobgui_widget_queue_draw (BOBGUI_WIDGET (tree_view));

      g_object_notify_by_pspec (G_OBJECT (tree_view), tree_view_props[PROP_ENABLE_GRID_LINES]);
    }
}

/**
 * bobgui_tree_view_get_enable_tree_lines:
 * @tree_view: a `BobguiTreeView`.
 *
 * Returns whether or not tree lines are drawn in @tree_view.
 *
 * Returns: %TRUE if tree lines are drawn in @tree_view, %FALSE
 * otherwise.
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 */
gboolean
bobgui_tree_view_get_enable_tree_lines (BobguiTreeView *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  g_return_val_if_fail (BOBGUI_IS_TREE_VIEW (tree_view), FALSE);

  return priv->tree_lines_enabled;
}

/**
 * bobgui_tree_view_set_enable_tree_lines:
 * @tree_view: a `BobguiTreeView`
 * @enabled: %TRUE to enable tree line drawing, %FALSE otherwise.
 *
 * Sets whether to draw lines interconnecting the expanders in @tree_view.
 * This does not have any visible effects for lists.
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 */
void
bobgui_tree_view_set_enable_tree_lines (BobguiTreeView *tree_view,
				     gboolean     enabled)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  gboolean was_enabled;

  g_return_if_fail (BOBGUI_IS_TREE_VIEW (tree_view));

  enabled = enabled != FALSE;

  was_enabled = priv->tree_lines_enabled;

  priv->tree_lines_enabled = enabled;

  if (was_enabled != enabled)
    {
      bobgui_widget_queue_draw (BOBGUI_WIDGET (tree_view));

      g_object_notify_by_pspec (G_OBJECT (tree_view), tree_view_props[PROP_ENABLE_TREE_LINES]);
    }
}


/**
 * bobgui_tree_view_set_show_expanders:
 * @tree_view: a `BobguiTreeView`
 * @enabled: %TRUE to enable expander drawing, %FALSE otherwise.
 *
 * Sets whether to draw and enable expanders and indent child rows in
 * @tree_view.  When disabled there will be no expanders visible in trees
 * and there will be no way to expand and collapse rows by default.  Also
 * note that hiding the expanders will disable the default indentation.  You
 * can set a custom indentation in this case using
 * bobgui_tree_view_set_level_indentation().
 * This does not have any visible effects for lists.
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 */
void
bobgui_tree_view_set_show_expanders (BobguiTreeView *tree_view,
				  gboolean     enabled)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  g_return_if_fail (BOBGUI_IS_TREE_VIEW (tree_view));

  enabled = enabled != FALSE;
  if (priv->show_expanders != enabled)
    {
      priv->show_expanders = enabled;
      bobgui_widget_queue_draw (BOBGUI_WIDGET (tree_view));
      g_object_notify_by_pspec (G_OBJECT (tree_view), tree_view_props[PROP_SHOW_EXPANDERS]);
    }
}

/**
 * bobgui_tree_view_get_show_expanders:
 * @tree_view: a `BobguiTreeView`.
 *
 * Returns whether or not expanders are drawn in @tree_view.
 *
 * Returns: %TRUE if expanders are drawn in @tree_view, %FALSE
 * otherwise.
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 */
gboolean
bobgui_tree_view_get_show_expanders (BobguiTreeView *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  g_return_val_if_fail (BOBGUI_IS_TREE_VIEW (tree_view), FALSE);

  return priv->show_expanders;
}

/**
 * bobgui_tree_view_set_level_indentation:
 * @tree_view: a `BobguiTreeView`
 * @indentation: the amount, in pixels, of extra indentation in @tree_view.
 *
 * Sets the amount of extra indentation for child levels to use in @tree_view
 * in addition to the default indentation.  The value should be specified in
 * pixels, a value of 0 disables this feature and in this case only the default
 * indentation will be used.
 * This does not have any visible effects for lists.
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 */
void
bobgui_tree_view_set_level_indentation (BobguiTreeView *tree_view,
				     int          indentation)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  priv->level_indentation = indentation;

  bobgui_widget_queue_draw (BOBGUI_WIDGET (tree_view));
}

/**
 * bobgui_tree_view_get_level_indentation:
 * @tree_view: a `BobguiTreeView`.
 *
 * Returns the amount, in pixels, of extra indentation for child levels
 * in @tree_view.
 *
 * Returns: the amount of extra indentation for child levels in
 * @tree_view.  A return value of 0 means that this feature is disabled.
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 */
int
bobgui_tree_view_get_level_indentation (BobguiTreeView *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  g_return_val_if_fail (BOBGUI_IS_TREE_VIEW (tree_view), 0);

  return priv->level_indentation;
}

/**
 * bobgui_tree_view_set_tooltip_row:
 * @tree_view: a `BobguiTreeView`
 * @tooltip: a `BobguiTooltip`
 * @path: a `BobguiTreePath`
 *
 * Sets the tip area of @tooltip to be the area covered by the row at @path.
 * See also bobgui_tree_view_set_tooltip_column() for a simpler alternative.
 * See also bobgui_tooltip_set_tip_area().
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 */
void
bobgui_tree_view_set_tooltip_row (BobguiTreeView *tree_view,
			       BobguiTooltip  *tooltip,
			       BobguiTreePath *path)
{
  g_return_if_fail (BOBGUI_IS_TREE_VIEW (tree_view));
  g_return_if_fail (BOBGUI_IS_TOOLTIP (tooltip));

  bobgui_tree_view_set_tooltip_cell (tree_view, tooltip, path, NULL, NULL);
}

/**
 * bobgui_tree_view_set_tooltip_cell:
 * @tree_view: a `BobguiTreeView`
 * @tooltip: a `BobguiTooltip`
 * @path: (nullable): a `BobguiTreePath`
 * @column: (nullable): a `BobguiTreeViewColumn`
 * @cell: (nullable): a `BobguiCellRenderer`
 *
 * Sets the tip area of @tooltip to the area @path, @column and @cell have
 * in common.  For example if @path is %NULL and @column is set, the tip
 * area will be set to the full area covered by @column.  See also
 * bobgui_tooltip_set_tip_area().
 *
 * Note that if @path is not specified and @cell is set and part of a column
 * containing the expander, the tooltip might not show and hide at the correct
 * position.  In such cases @path must be set to the current node under the
 * mouse cursor for this function to operate correctly.
 *
 * See also bobgui_tree_view_set_tooltip_column() for a simpler alternative.
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 */
void
bobgui_tree_view_set_tooltip_cell (BobguiTreeView       *tree_view,
				BobguiTooltip        *tooltip,
				BobguiTreePath       *path,
				BobguiTreeViewColumn *column,
				BobguiCellRenderer   *cell)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);
  GdkRectangle rect;

  g_return_if_fail (BOBGUI_IS_TREE_VIEW (tree_view));
  g_return_if_fail (BOBGUI_IS_TOOLTIP (tooltip));
  g_return_if_fail (column == NULL || BOBGUI_IS_TREE_VIEW_COLUMN (column));
  g_return_if_fail (cell == NULL || BOBGUI_IS_CELL_RENDERER (cell));

  /* Determine x values. */
  if (column && cell)
    {
      GdkRectangle tmp;
      int start, width;

      /* We always pass in path here, whether it is NULL or not.
       * For cells in expander columns path must be specified so that
       * we can correctly account for the indentation.  This also means
       * that the tooltip is constrained vertically by the "Determine y
       * values" code below; this is not a real problem since cells actually
       * don't stretch vertically in contrast to columns.
       */
      bobgui_tree_view_get_cell_area (tree_view, path, column, &tmp);
      bobgui_tree_view_column_cell_get_position (column, cell, &start, &width);

      bobgui_tree_view_convert_bin_window_to_widget_coords (tree_view,
							 tmp.x + start, 0,
							 &rect.x, NULL);
      rect.width = width;
    }
  else if (column)
    {
      GdkRectangle tmp;

      bobgui_tree_view_get_background_area (tree_view, NULL, column, &tmp);
      bobgui_tree_view_convert_bin_window_to_widget_coords (tree_view,
							 tmp.x, 0,
							 &rect.x, NULL);
      rect.width = tmp.width;
    }
  else
    {
      rect.x = 0;
      rect.width = bobgui_widget_get_width (BOBGUI_WIDGET (tree_view));;
    }

  /* Determine y values. */
  if (path)
    {
      GdkRectangle tmp;

      bobgui_tree_view_get_background_area (tree_view, path, NULL, &tmp);
      bobgui_tree_view_convert_bin_window_to_widget_coords (tree_view,
							 0, tmp.y,
							 NULL, &rect.y);
      rect.height = tmp.height;
    }
  else
    {
      rect.y = 0;
      rect.height = bobgui_adjustment_get_page_size (priv->vadjustment);
    }

  bobgui_tooltip_set_tip_area (tooltip, &rect);
}

/**
 * bobgui_tree_view_get_tooltip_context:
 * @tree_view: a `BobguiTreeView`
 * @x: the x coordinate (relative to widget coordinates)
 * @y: the y coordinate (relative to widget coordinates)
 * @keyboard_tip: whether this is a keyboard tooltip or not
 * @model: (out) (optional) (nullable) (transfer none): a pointer to
 *   receive a `BobguiTreeModel`
 * @path: (out) (optional): a pointer to receive a `BobguiTreePath`
 * @iter: (out) (optional): a pointer to receive a `BobguiTreeIter`
 *
 * This function is supposed to be used in a ::query-tooltip
 * signal handler for `BobguiTreeView`. The @x, @y and @keyboard_tip values
 * which are received in the signal handler, should be passed to this
 * function without modification.
 *
 * The return value indicates whether there is a tree view row at the given
 * coordinates (%TRUE) or not (%FALSE) for mouse tooltips. For keyboard
 * tooltips the row returned will be the cursor row. When %TRUE, then any of
 * @model, @path and @iter which have been provided will be set to point to
 * that row and the corresponding model. @x and @y will always be converted
 * to be relative to @tree_view’s bin_window if @keyboard_tooltip is %FALSE.
 *
 * Returns: whether or not the given tooltip context points to a row
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 */
gboolean
bobgui_tree_view_get_tooltip_context (BobguiTreeView   *tree_view,
				   int            x,
				   int            y,
				   gboolean       keyboard_tip,
				   BobguiTreeModel **model,
				   BobguiTreePath  **path,
				   BobguiTreeIter   *iter)
{
  BobguiTreePath *tmppath = NULL;

  g_return_val_if_fail (BOBGUI_IS_TREE_VIEW (tree_view), FALSE);

  if (keyboard_tip)
    {
      bobgui_tree_view_get_cursor (tree_view, &tmppath, NULL);

      if (!tmppath)
	return FALSE;
    }
  else
    {
      int rel_x, rel_y;

      bobgui_tree_view_convert_widget_to_bin_window_coords (tree_view, x, y,
                                                         &rel_x, &rel_y);

      if (!bobgui_tree_view_get_path_at_pos (tree_view, rel_x, rel_y,
					  &tmppath, NULL, NULL, NULL))
	return FALSE;
    }

  if (model)
    *model = bobgui_tree_view_get_model (tree_view);

  if (iter)
    bobgui_tree_model_get_iter (bobgui_tree_view_get_model (tree_view),
			     iter, tmppath);

  if (path)
    *path = tmppath;
  else
    bobgui_tree_path_free (tmppath);

  return TRUE;
}

static gboolean
bobgui_tree_view_set_tooltip_query_cb (BobguiWidget  *widget,
				    int         x,
				    int         y,
				    gboolean    keyboard_tip,
				    BobguiTooltip *tooltip,
				    gpointer    data)
{
  GValue value = G_VALUE_INIT;
  GValue transformed = G_VALUE_INIT;
  BobguiTreeIter iter;
  BobguiTreePath *path;
  BobguiTreeModel *model;
  BobguiTreeView *tree_view = BOBGUI_TREE_VIEW (widget);
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  if (!bobgui_tree_view_get_tooltip_context (BOBGUI_TREE_VIEW (widget),
					  x, y,
					  keyboard_tip,
					  &model, &path, &iter))
    return FALSE;

  bobgui_tree_model_get_value (model, &iter,
                            priv->tooltip_column, &value);

  g_value_init (&transformed, G_TYPE_STRING);

  if (!g_value_transform (&value, &transformed))
    {
      g_value_unset (&value);
      bobgui_tree_path_free (path);

      return FALSE;
    }

  g_value_unset (&value);

  if (!g_value_get_string (&transformed))
    {
      g_value_unset (&transformed);
      bobgui_tree_path_free (path);

      return FALSE;
    }

  bobgui_tooltip_set_markup (tooltip, g_value_get_string (&transformed));
  bobgui_tree_view_set_tooltip_row (tree_view, tooltip, path);

  bobgui_tree_path_free (path);
  g_value_unset (&transformed);

  return TRUE;
}

/**
 * bobgui_tree_view_set_tooltip_column:
 * @tree_view: a `BobguiTreeView`
 * @column: an integer, which is a valid column number for @tree_view’s model
 *
 * If you only plan to have simple (text-only) tooltips on full rows, you
 * can use this function to have `BobguiTreeView` handle these automatically
 * for you. @column should be set to the column in @tree_view’s model
 * containing the tooltip texts, or -1 to disable this feature.
 *
 * When enabled, `BobguiWidget:has-tooltip` will be set to %TRUE and
 * @tree_view will connect a `BobguiWidget::query-tooltip` signal handler.
 *
 * Note that the signal handler sets the text with bobgui_tooltip_set_markup(),
 * so &, <, etc have to be escaped in the text.
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 */
void
bobgui_tree_view_set_tooltip_column (BobguiTreeView *tree_view,
			          int          column)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  g_return_if_fail (BOBGUI_IS_TREE_VIEW (tree_view));

  if (column == priv->tooltip_column)
    return;

  if (column == -1)
    {
      g_signal_handlers_disconnect_by_func (tree_view,
	  				    bobgui_tree_view_set_tooltip_query_cb,
					    NULL);
      bobgui_widget_set_has_tooltip (BOBGUI_WIDGET (tree_view), FALSE);
    }
  else
    {
      if (priv->tooltip_column == -1)
        {
          g_signal_connect (tree_view, "query-tooltip",
		            G_CALLBACK (bobgui_tree_view_set_tooltip_query_cb), NULL);
          bobgui_widget_set_has_tooltip (BOBGUI_WIDGET (tree_view), TRUE);
        }
    }

  priv->tooltip_column = column;
  g_object_notify_by_pspec (G_OBJECT (tree_view), tree_view_props[PROP_TOOLTIP_COLUMN]);
}

/**
 * bobgui_tree_view_get_tooltip_column:
 * @tree_view: a `BobguiTreeView`
 *
 * Returns the column of @tree_view’s model which is being used for
 * displaying tooltips on @tree_view’s rows.
 *
 * Returns: the index of the tooltip column that is currently being
 * used, or -1 if this is disabled.
 *
 * Deprecated: 4.10: Use [class@Bobgui.ListView] or [class@Bobgui.ColumnView] instead
 */
int
bobgui_tree_view_get_tooltip_column (BobguiTreeView *tree_view)
{
  BobguiTreeViewPrivate *priv = bobgui_tree_view_get_instance_private (tree_view);

  g_return_val_if_fail (BOBGUI_IS_TREE_VIEW (tree_view), 0);

  return priv->tooltip_column;
}

static gboolean
bobgui_tree_view_get_border (BobguiScrollable *scrollable,
                          BobguiBorder     *border)
{
  border->top = bobgui_tree_view_get_effective_header_height (BOBGUI_TREE_VIEW (scrollable));

  return TRUE;
}

static void
bobgui_tree_view_scrollable_init (BobguiScrollableInterface *iface)
{
  iface->get_border = bobgui_tree_view_get_border;
}
