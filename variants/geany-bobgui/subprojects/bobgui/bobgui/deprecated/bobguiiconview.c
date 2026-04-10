/* bobguiiconview.c
 * Copyright (C) 2002, 2004  Anders Carlsson <andersca@gnu.org>
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

#include "bobguiiconviewprivate.h"

#include "bobguiadjustmentprivate.h"
#include "bobguicellareabox.h"
#include "bobguicellareacontext.h"
#include "bobguicelllayout.h"
#include "bobguicellrenderer.h"
#include "bobguicellrendererpixbuf.h"
#include "bobguicellrenderertext.h"
#include "bobguidragsourceprivate.h"
#include "bobguientry.h"
#include "bobguimain.h"
#include "bobguimarshalers.h"
#include "bobguiorientable.h"
#include "bobguiprivate.h"
#include "bobguiscrollable.h"
#include "bobguisizerequest.h"
#include "bobguirenderbackgroundprivate.h"
#include "bobguirenderborderprivate.h"
#include "bobguisnapshot.h"
#include "bobguistylecontextprivate.h"
#include "bobguitreednd.h"
#include "bobguitypebuiltins.h"
#include "bobguiwidgetprivate.h"
#include "bobguiwindow.h"
#include "bobguieventcontrollerkey.h"
#include "bobguidragsource.h"
#include "bobguidragicon.h"
#include "bobguinative.h"

#include <string.h>

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

/**
 * BobguiIconView:
 *
 * `BobguiIconView` is a widget which displays data in a grid of icons.
 *
 * <picture>
 *   <source srcset="icon-view-dark.png" media="(prefers-color-scheme: dark)">
 *   <img alt="An example BobguiIconView" src="icon-view.png">
 * </picture>
 *
 * `BobguiIconView` provides an alternative view on a `BobguiTreeModel`.
 * It displays the model as a grid of icons with labels. Like
 * [class@Bobgui.TreeView], it allows to select one or multiple items
 * (depending on the selection mode, see [method@Bobgui.IconView.set_selection_mode]).
 * In addition to selection with the arrow keys, `BobguiIconView` supports
 * rubberband selection, which is controlled by dragging the pointer.
 *
 * Note that if the tree model is backed by an actual tree store (as
 * opposed to a flat list where the mapping to icons is obvious),
 * `BobguiIconView` will only display the first level of the tree and
 * ignore the tree’s branches.
 *
 * ## CSS nodes
 *
 * ```
 * iconview.view
 * ╰── [rubberband]
 * ```
 *
 * `BobguiIconView` has a single CSS node with name iconview and style class .view.
 * For rubberband selection, a subnode with name rubberband is used.
 *
 * Deprecated: 4.10: Use [class@Bobgui.GridView] instead
 */

#define SCROLL_EDGE_SIZE 15

typedef struct _BobguiIconViewChild BobguiIconViewChild;
struct _BobguiIconViewChild
{
  BobguiWidget    *widget;
  GdkRectangle  area;
};

/* Signals */
enum
{
  ITEM_ACTIVATED,
  SELECTION_CHANGED,
  SELECT_ALL,
  UNSELECT_ALL,
  SELECT_CURSOR_ITEM,
  TOGGLE_CURSOR_ITEM,
  MOVE_CURSOR,
  ACTIVATE_CURSOR_ITEM,
  LAST_SIGNAL
};

/* Properties */
enum
{
  PROP_0,
  PROP_PIXBUF_COLUMN,
  PROP_TEXT_COLUMN,
  PROP_MARKUP_COLUMN,
  PROP_SELECTION_MODE,
  PROP_ITEM_ORIENTATION,
  PROP_MODEL,
  PROP_COLUMNS,
  PROP_ITEM_WIDTH,
  PROP_SPACING,
  PROP_ROW_SPACING,
  PROP_COLUMN_SPACING,
  PROP_MARGIN,
  PROP_REORDERABLE,
  PROP_TOOLTIP_COLUMN,
  PROP_ITEM_PADDING,
  PROP_CELL_AREA,
  PROP_HADJUSTMENT,
  PROP_VADJUSTMENT,
  PROP_HSCROLL_POLICY,
  PROP_VSCROLL_POLICY,
  PROP_ACTIVATE_ON_SINGLE_CLICK
};

/* GObject vfuncs */
static void             bobgui_icon_view_cell_layout_init          (BobguiCellLayoutIface *iface);
static void             bobgui_icon_view_dispose                   (GObject            *object);
static void             bobgui_icon_view_constructed               (GObject            *object);
static void             bobgui_icon_view_set_property              (GObject            *object,
								 guint               prop_id,
								 const GValue       *value,
								 GParamSpec         *pspec);
static void             bobgui_icon_view_get_property              (GObject            *object,
								 guint               prop_id,
								 GValue             *value,
								 GParamSpec         *pspec);
/* BobguiWidget vfuncs */
static BobguiSizeRequestMode bobgui_icon_view_get_request_mode        (BobguiWidget          *widget);
static void bobgui_icon_view_measure (BobguiWidget *widget,
                                   BobguiOrientation  orientation,
                                   int             for_size,
                                   int            *minimum,
                                   int            *natural,
                                   int            *minimum_baseline,
                                   int            *natural_baseline);
static void             bobgui_icon_view_size_allocate             (BobguiWidget          *widget,
                                                                 int                 width,
                                                                 int                 height,
                                                                 int                 baseline);
static void             bobgui_icon_view_snapshot                  (BobguiWidget          *widget,
                                                                 BobguiSnapshot        *snapshot);
static void             bobgui_icon_view_motion                    (BobguiEventController *controller,
                                                                 double              x,
                                                                 double              y,
                                                                 gpointer            user_data);
static void             bobgui_icon_view_leave                     (BobguiEventController   *controller,
                                                                 gpointer              user_data);
static void             bobgui_icon_view_button_press              (BobguiGestureClick *gesture,
                                                                 int                   n_press,
                                                                 double                x,
                                                                 double                y,
                                                                 gpointer              user_data);
static void             bobgui_icon_view_button_release            (BobguiGestureClick *gesture,
                                                                 int                   n_press,
                                                                 double                x,
                                                                 double                y,
                                                                 gpointer              user_data);
static gboolean         bobgui_icon_view_key_pressed               (BobguiEventControllerKey *controller,
                                                                 guint                  keyval,
                                                                 guint                  keycode,
                                                                 GdkModifierType        state,
                                                                 BobguiWidget             *widget);

static void             bobgui_icon_view_remove                    (BobguiIconView        *icon_view,
                                                                 BobguiWidget          *widget);

/* BobguiIconView vfuncs */
static void             bobgui_icon_view_real_select_all           (BobguiIconView        *icon_view);
static void             bobgui_icon_view_real_unselect_all         (BobguiIconView        *icon_view);
static void             bobgui_icon_view_real_select_cursor_item   (BobguiIconView        *icon_view);
static void             bobgui_icon_view_real_toggle_cursor_item   (BobguiIconView        *icon_view);
static gboolean         bobgui_icon_view_real_activate_cursor_item (BobguiIconView        *icon_view);

 /* Internal functions */
static void                 bobgui_icon_view_set_hadjustment_values         (BobguiIconView            *icon_view);
static void                 bobgui_icon_view_set_vadjustment_values         (BobguiIconView            *icon_view);
static void                 bobgui_icon_view_set_hadjustment                (BobguiIconView            *icon_view,
                                                                          BobguiAdjustment          *adjustment);
static void                 bobgui_icon_view_set_vadjustment                (BobguiIconView            *icon_view,
                                                                          BobguiAdjustment          *adjustment);
static void                 bobgui_icon_view_adjustment_changed             (BobguiAdjustment          *adjustment,
									  BobguiIconView            *icon_view);
static void                 bobgui_icon_view_layout                         (BobguiIconView            *icon_view);
static void                 bobgui_icon_view_snapshot_item                  (BobguiIconView            *icon_view,
									  BobguiSnapshot            *snapshot,
									  BobguiIconViewItem        *item,
									  int                     x,
									  int                     y,
									  gboolean                draw_focus);
static void                 bobgui_icon_view_snapshot_rubberband            (BobguiIconView            *icon_view,
								          BobguiSnapshot            *snapshot);
static void                 bobgui_icon_view_queue_draw_path                (BobguiIconView *icon_view,
									  BobguiTreePath *path);
static void                 bobgui_icon_view_queue_draw_item                (BobguiIconView            *icon_view,
									  BobguiIconViewItem        *item);
static void                 bobgui_icon_view_start_rubberbanding            (BobguiIconView            *icon_view,
                                                                          GdkDevice              *device,
									  int                     x,
									  int                     y);
static void                 bobgui_icon_view_stop_rubberbanding             (BobguiIconView            *icon_view);
static void                 bobgui_icon_view_update_rubberband_selection    (BobguiIconView            *icon_view);
static gboolean             bobgui_icon_view_item_hit_test                  (BobguiIconView            *icon_view,
									  BobguiIconViewItem        *item,
									  int                     x,
									  int                     y,
									  int                     width,
									  int                     height);
static gboolean             bobgui_icon_view_unselect_all_internal          (BobguiIconView            *icon_view);
static void                 bobgui_icon_view_update_rubberband              (BobguiIconView            *icon_view);
static void                 bobgui_icon_view_item_invalidate_size           (BobguiIconViewItem        *item);
static void                 bobgui_icon_view_invalidate_sizes               (BobguiIconView            *icon_view);
static void                 bobgui_icon_view_add_move_binding               (BobguiWidgetClass         *widget_class,
									  guint                   keyval,
									  guint                   modmask,
									  BobguiMovementStep         step,
									  int                     count);
static gboolean             bobgui_icon_view_real_move_cursor               (BobguiIconView            *icon_view,
									  BobguiMovementStep         step,
									  int                     count,
                                                                          gboolean                extend,
                                                                          gboolean                modify);
static void                 bobgui_icon_view_move_cursor_up_down            (BobguiIconView            *icon_view,
									  int                     count);
static void                 bobgui_icon_view_move_cursor_page_up_down       (BobguiIconView            *icon_view,
									  int                     count);
static void                 bobgui_icon_view_move_cursor_left_right         (BobguiIconView            *icon_view,
									  int                     count);
static void                 bobgui_icon_view_move_cursor_start_end          (BobguiIconView            *icon_view,
									  int                     count);
static void                 bobgui_icon_view_scroll_to_item                 (BobguiIconView            *icon_view,
									  BobguiIconViewItem        *item);
static gboolean             bobgui_icon_view_select_all_between             (BobguiIconView            *icon_view,
									  BobguiIconViewItem        *anchor,
									  BobguiIconViewItem        *cursor);

static void                 bobgui_icon_view_ensure_cell_area               (BobguiIconView            *icon_view,
                                                                          BobguiCellArea            *cell_area);

static BobguiCellArea         *bobgui_icon_view_cell_layout_get_area           (BobguiCellLayout          *layout);

static void                 bobgui_icon_view_add_editable                   (BobguiCellArea            *area,
									  BobguiCellRenderer        *renderer,
									  BobguiCellEditable        *editable,
									  GdkRectangle           *cell_area,
									  const char             *path,
									  BobguiIconView            *icon_view);
static void                 bobgui_icon_view_remove_editable                (BobguiCellArea            *area,
									  BobguiCellRenderer        *renderer,
									  BobguiCellEditable        *editable,
									  BobguiIconView            *icon_view);
static void                 update_text_cell                             (BobguiIconView            *icon_view);
static void                 update_pixbuf_cell                           (BobguiIconView            *icon_view);

/* Source side drag signals */
static void     bobgui_icon_view_dnd_finished_cb   (GdkDrag         *drag,
                                                 BobguiWidget       *widget);
static GdkContentProvider * bobgui_icon_view_drag_data_get                  (BobguiIconView            *icon_view,
                                                                          BobguiTreePath            *source_row);

/* Target side drag signals */
static void                 bobgui_icon_view_drag_leave                     (BobguiDropTargetAsync     *dest,
                                                                          GdkDrop                *drop,
                                                                          BobguiIconView            *icon_view);
static GdkDragAction        bobgui_icon_view_drag_motion                    (BobguiDropTargetAsync     *dest,
                                                                          GdkDrop                *drop,
                                                                          double                  x,
                                                                          double                  y,
                                                                          BobguiIconView            *icon_view);
static gboolean             bobgui_icon_view_drag_drop                      (BobguiDropTargetAsync     *dest,
                                                                          GdkDrop                *drop,
                                                                          double                  x,
                                                                          double                  y,
                                                                          BobguiIconView            *icon_view);
static void     bobgui_icon_view_drag_data_received (GObject          *source,
                                                  GAsyncResult     *result,
                                                  gpointer          data);
static gboolean bobgui_icon_view_maybe_begin_drag   (BobguiIconView      *icon_view,
                                                  double            x,
                                                  double            y,
                                                  GdkDevice        *device);

static void     remove_scroll_timeout            (BobguiIconView *icon_view);

/* BobguiBuildable */
static BobguiBuildableIface *parent_buildable_iface;
static void     bobgui_icon_view_buildable_init             (BobguiBuildableIface  *iface);
static gboolean bobgui_icon_view_buildable_custom_tag_start (BobguiBuildable       *buildable,
                                                          BobguiBuilder         *builder,
                                                          GObject            *child,
                                                          const char         *tagname,
                                                          BobguiBuildableParser *parser,
                                                          gpointer           *data);
static void     bobgui_icon_view_buildable_custom_tag_end   (BobguiBuildable       *buildable,
                                                          BobguiBuilder         *builder,
                                                          GObject            *child,
                                                          const char         *tagname,
                                                          gpointer            data);


static guint icon_view_signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE_WITH_CODE (BobguiIconView, bobgui_icon_view, BOBGUI_TYPE_WIDGET,
                         G_ADD_PRIVATE (BobguiIconView)
			 G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_CELL_LAYOUT,
						bobgui_icon_view_cell_layout_init)
			 G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_BUILDABLE,
						bobgui_icon_view_buildable_init)
			 G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_SCROLLABLE, NULL))

static void
bobgui_icon_view_class_init (BobguiIconViewClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);

  gobject_class->constructed = bobgui_icon_view_constructed;
  gobject_class->dispose = bobgui_icon_view_dispose;
  gobject_class->set_property = bobgui_icon_view_set_property;
  gobject_class->get_property = bobgui_icon_view_get_property;

  widget_class->get_request_mode = bobgui_icon_view_get_request_mode;
  widget_class->measure = bobgui_icon_view_measure;
  widget_class->size_allocate = bobgui_icon_view_size_allocate;
  widget_class->snapshot = bobgui_icon_view_snapshot;
  widget_class->focus = bobgui_widget_focus_self;
  widget_class->grab_focus = bobgui_widget_grab_focus_self;

  klass->select_all = bobgui_icon_view_real_select_all;
  klass->unselect_all = bobgui_icon_view_real_unselect_all;
  klass->select_cursor_item = bobgui_icon_view_real_select_cursor_item;
  klass->toggle_cursor_item = bobgui_icon_view_real_toggle_cursor_item;
  klass->activate_cursor_item = bobgui_icon_view_real_activate_cursor_item;
  klass->move_cursor = bobgui_icon_view_real_move_cursor;

  /* Properties */
  /**
   * BobguiIconView:selection-mode:
   *
   * The ::selection-mode property specifies the selection mode of
   * icon view. If the mode is %BOBGUI_SELECTION_MULTIPLE, rubberband selection
   * is enabled, for the other modes, only keyboard selection is possible.
   */
  g_object_class_install_property (gobject_class,
				   PROP_SELECTION_MODE,
				   g_param_spec_enum ("selection-mode", NULL, NULL,
						      BOBGUI_TYPE_SELECTION_MODE,
						      BOBGUI_SELECTION_SINGLE,
						      BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * BobguiIconView:pixbuf-column:
   *
   * The ::pixbuf-column property contains the number of the model column
   * containing the pixbufs which are displayed. The pixbuf column must be
   * of type `GDK_TYPE_PIXBUF`. Setting this property to -1 turns off the
   * display of pixbufs.
   */
  g_object_class_install_property (gobject_class,
				   PROP_PIXBUF_COLUMN,
				   g_param_spec_int ("pixbuf-column", NULL, NULL,
						     -1, G_MAXINT, -1,
						     BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * BobguiIconView:text-column:
   *
   * The ::text-column property contains the number of the model column
   * containing the texts which are displayed. The text column must be
   * of type `G_TYPE_STRING`. If this property and the :markup-column
   * property are both set to -1, no texts are displayed.
   */
  g_object_class_install_property (gobject_class,
				   PROP_TEXT_COLUMN,
				   g_param_spec_int ("text-column", NULL, NULL,
						     -1, G_MAXINT, -1,
						     BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));


  /**
   * BobguiIconView:markup-column:
   *
   * The ::markup-column property contains the number of the model column
   * containing markup information to be displayed. The markup column must be
   * of type `G_TYPE_STRING`. If this property and the :text-column property
   * are both set to column numbers, it overrides the text column.
   * If both are set to -1, no texts are displayed.
   */
  g_object_class_install_property (gobject_class,
				   PROP_MARKUP_COLUMN,
				   g_param_spec_int ("markup-column", NULL, NULL,
						     -1, G_MAXINT, -1,
						     BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * BobguiIconView:model:
   *
   * The model of the icon view.
   */
  g_object_class_install_property (gobject_class,
                                   PROP_MODEL,
                                   g_param_spec_object ("model", NULL, NULL,
							BOBGUI_TYPE_TREE_MODEL,
							BOBGUI_PARAM_READWRITE));

  /**
   * BobguiIconView:columns:
   *
   * The columns property contains the number of the columns in which the
   * items should be displayed. If it is -1, the number of columns will
   * be chosen automatically to fill the available area.
   */
  g_object_class_install_property (gobject_class,
				   PROP_COLUMNS,
				   g_param_spec_int ("columns", NULL, NULL,
						     -1, G_MAXINT, -1,
						     BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));


  /**
   * BobguiIconView:item-width:
   *
   * The item-width property specifies the width to use for each item.
   * If it is set to -1, the icon view will automatically determine a
   * suitable item size.
   */
  g_object_class_install_property (gobject_class,
				   PROP_ITEM_WIDTH,
				   g_param_spec_int ("item-width", NULL, NULL,
						     -1, G_MAXINT, -1,
						     BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * BobguiIconView:spacing:
   *
   * The spacing property specifies the space which is inserted between
   * the cells (i.e. the icon and the text) of an item.
   */
  g_object_class_install_property (gobject_class,
                                   PROP_SPACING,
                                   g_param_spec_int ("spacing", NULL, NULL,
						     0, G_MAXINT, 0,
						     BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * BobguiIconView:row-spacing:
   *
   * The row-spacing property specifies the space which is inserted between
   * the rows of the icon view.
   */
  g_object_class_install_property (gobject_class,
                                   PROP_ROW_SPACING,
                                   g_param_spec_int ("row-spacing", NULL, NULL,
						     0, G_MAXINT, 6,
						     BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * BobguiIconView:column-spacing:
   *
   * The column-spacing property specifies the space which is inserted between
   * the columns of the icon view.
   */
  g_object_class_install_property (gobject_class,
                                   PROP_COLUMN_SPACING,
                                   g_param_spec_int ("column-spacing", NULL, NULL,
						     0, G_MAXINT, 6,
						     BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * BobguiIconView:margin:
   *
   * The margin property specifies the space which is inserted
   * at the edges of the icon view.
   */
  g_object_class_install_property (gobject_class,
                                   PROP_MARGIN,
                                   g_param_spec_int ("margin", NULL, NULL,
						     0, G_MAXINT, 6,
						     BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * BobguiIconView:item-orientation:
   *
   * The item-orientation property specifies how the cells (i.e. the icon and
   * the text) of the item are positioned relative to each other.
   */
  g_object_class_install_property (gobject_class,
				   PROP_ITEM_ORIENTATION,
				   g_param_spec_enum ("item-orientation", NULL, NULL,
						      BOBGUI_TYPE_ORIENTATION,
						      BOBGUI_ORIENTATION_VERTICAL,
						      BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * BobguiIconView:reorderable:
   *
   * The reorderable property specifies if the items can be reordered
   * by DND.
   */
  g_object_class_install_property (gobject_class,
                                   PROP_REORDERABLE,
                                   g_param_spec_boolean ("reorderable", NULL, NULL,
							 FALSE,
							 G_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

    /**
     * BobguiIconView:tooltip-column:
     *
     * The column of the icon view model which is being used for displaying
     * tooltips on it's rows.
     */
    g_object_class_install_property (gobject_class,
                                     PROP_TOOLTIP_COLUMN,
                                     g_param_spec_int ("tooltip-column", NULL, NULL,
                                                       -1,
                                                       G_MAXINT,
                                                       -1,
                                                       BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * BobguiIconView:item-padding:
   *
   * The item-padding property specifies the padding around each
   * of the icon view's item.
   */
  g_object_class_install_property (gobject_class,
                                   PROP_ITEM_PADDING,
                                   g_param_spec_int ("item-padding", NULL, NULL,
						     0, G_MAXINT, 6,
						     BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * BobguiIconView:cell-area:
   *
   * The `BobguiCellArea` used to layout cell renderers for this view.
   *
   * If no area is specified when creating the icon view with bobgui_icon_view_new_with_area()
   * a `BobguiCellAreaBox` will be used.
   */
  g_object_class_install_property (gobject_class,
				   PROP_CELL_AREA,
				   g_param_spec_object ("cell-area", NULL, NULL,
							BOBGUI_TYPE_CELL_AREA,
							BOBGUI_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

  /**
   * BobguiIconView:activate-on-single-click:
   *
   * The activate-on-single-click property specifies whether the "item-activated" signal
   * will be emitted after a single click.
   */
  g_object_class_install_property (gobject_class,
                                   PROP_ACTIVATE_ON_SINGLE_CLICK,
                                   g_param_spec_boolean ("activate-on-single-click", NULL, NULL,
							 FALSE,
							 BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  /* Scrollable interface properties */
  g_object_class_override_property (gobject_class, PROP_HADJUSTMENT,    "hadjustment");
  g_object_class_override_property (gobject_class, PROP_VADJUSTMENT,    "vadjustment");
  g_object_class_override_property (gobject_class, PROP_HSCROLL_POLICY, "hscroll-policy");
  g_object_class_override_property (gobject_class, PROP_VSCROLL_POLICY, "vscroll-policy");

  /* Signals */
  /**
   * BobguiIconView::item-activated:
   * @iconview: the object on which the signal is emitted
   * @path: the `BobguiTreePath` for the activated item
   *
   * The ::item-activated signal is emitted when the method
   * bobgui_icon_view_item_activated() is called, when the user double
   * clicks an item with the "activate-on-single-click" property set
   * to %FALSE, or when the user single clicks an item when the
   * "activate-on-single-click" property set to %TRUE. It is also
   * emitted when a non-editable item is selected and one of the keys:
   * Space, Return or Enter is pressed.
   */
  icon_view_signals[ITEM_ACTIVATED] =
    g_signal_new (I_("item-activated"),
		  G_TYPE_FROM_CLASS (gobject_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BobguiIconViewClass, item_activated),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 1,
		  BOBGUI_TYPE_TREE_PATH);

  /**
   * BobguiIconView::selection-changed:
   * @iconview: the object on which the signal is emitted
   *
   * The ::selection-changed signal is emitted when the selection
   * (i.e. the set of selected items) changes.
   */
  icon_view_signals[SELECTION_CHANGED] =
    g_signal_new (I_("selection-changed"),
		  G_TYPE_FROM_CLASS (gobject_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (BobguiIconViewClass, selection_changed),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 0);

  /**
   * BobguiIconView::select-all:
   * @iconview: the object on which the signal is emitted
   *
   * A [keybinding signal][class@Bobgui.SignalAction]
   * which gets emitted when the user selects all items.
   *
   * Applications should not connect to it, but may emit it with
   * g_signal_emit_by_name() if they need to control selection
   * programmatically.
   *
   * The default binding for this signal is Ctrl-a.
   */
  icon_view_signals[SELECT_ALL] =
    g_signal_new (I_("select-all"),
		  G_TYPE_FROM_CLASS (gobject_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (BobguiIconViewClass, select_all),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 0);

  /**
   * BobguiIconView::unselect-all:
   * @iconview: the object on which the signal is emitted
   *
   * A [keybinding signal][class@Bobgui.SignalAction]
   * which gets emitted when the user unselects all items.
   *
   * Applications should not connect to it, but may emit it with
   * g_signal_emit_by_name() if they need to control selection
   * programmatically.
   *
   * The default binding for this signal is Ctrl-Shift-a.
   */
  icon_view_signals[UNSELECT_ALL] =
    g_signal_new (I_("unselect-all"),
		  G_TYPE_FROM_CLASS (gobject_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (BobguiIconViewClass, unselect_all),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 0);

  /**
   * BobguiIconView::select-cursor-item:
   * @iconview: the object on which the signal is emitted
   *
   * A [keybinding signal][class@Bobgui.SignalAction]
   * which gets emitted when the user selects the item that is currently
   * focused.
   *
   * Applications should not connect to it, but may emit it with
   * g_signal_emit_by_name() if they need to control selection
   * programmatically.
   *
   * There is no default binding for this signal.
   */
  icon_view_signals[SELECT_CURSOR_ITEM] =
    g_signal_new (I_("select-cursor-item"),
		  G_TYPE_FROM_CLASS (gobject_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (BobguiIconViewClass, select_cursor_item),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 0);

  /**
   * BobguiIconView::toggle-cursor-item:
   * @iconview: the object on which the signal is emitted
   *
   * A [keybinding signal][class@Bobgui.SignalAction]
   * which gets emitted when the user toggles whether the currently
   * focused item is selected or not. The exact effect of this
   * depend on the selection mode.
   *
   * Applications should not connect to it, but may emit it with
   * g_signal_emit_by_name() if they need to control selection
   * programmatically.
   *
   * There is no default binding for this signal is Ctrl-Space.
   */
  icon_view_signals[TOGGLE_CURSOR_ITEM] =
    g_signal_new (I_("toggle-cursor-item"),
		  G_TYPE_FROM_CLASS (gobject_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (BobguiIconViewClass, toggle_cursor_item),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 0);

  /**
   * BobguiIconView::activate-cursor-item:
   * @iconview: the object on which the signal is emitted
   *
   * A [keybinding signal][class@Bobgui.SignalAction]
   * which gets emitted when the user activates the currently
   * focused item.
   *
   * Applications should not connect to it, but may emit it with
   * g_signal_emit_by_name() if they need to control activation
   * programmatically.
   *
   * The default bindings for this signal are Space, Return and Enter.
   *
   * Returns: whether the item was activated
   */
  icon_view_signals[ACTIVATE_CURSOR_ITEM] =
    g_signal_new (I_("activate-cursor-item"),
		  G_TYPE_FROM_CLASS (gobject_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (BobguiIconViewClass, activate_cursor_item),
		  NULL, NULL,
		  _bobgui_marshal_BOOLEAN__VOID,
		  G_TYPE_BOOLEAN, 0);
  g_signal_set_va_marshaller (icon_view_signals[ACTIVATE_CURSOR_ITEM],
                              G_TYPE_FROM_CLASS (klass),
                              _bobgui_marshal_BOOLEAN__VOIDv);

  /**
   * BobguiIconView::move-cursor:
   * @iconview: the object which received the signal
   * @step: the granularity of the move, as a `BobguiMovementStep`
   * @count: the number of @step units to move
   * @extend: whether to extend the selection
   * @modify: whether to modify the selection
   *
   * The ::move-cursor signal is a
   * [keybinding signal][class@Bobgui.SignalAction]
   * which gets emitted when the user initiates a cursor movement.
   *
   * Applications should not connect to it, but may emit it with
   * g_signal_emit_by_name() if they need to control the cursor
   * programmatically.
   *
   * The default bindings for this signal include
   * - Arrow keys which move by individual steps
   * - Home/End keys which move to the first/last item
   * - PageUp/PageDown which move by "pages"
   * All of these will extend the selection when combined with
   * the Shift modifier.
   *
   * Returns: whether the cursor was moved
   */
  icon_view_signals[MOVE_CURSOR] =
    g_signal_new (I_("move-cursor"),
		  G_TYPE_FROM_CLASS (gobject_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (BobguiIconViewClass, move_cursor),
		  NULL, NULL,
		  _bobgui_marshal_BOOLEAN__ENUM_INT_BOOLEAN_BOOLEAN,
		  G_TYPE_BOOLEAN, 4,
		  BOBGUI_TYPE_MOVEMENT_STEP,
		  G_TYPE_INT,
                  G_TYPE_BOOLEAN,
                  G_TYPE_BOOLEAN);
  g_signal_set_va_marshaller (icon_view_signals[MOVE_CURSOR],
                              G_TYPE_FROM_CLASS (klass),
                              _bobgui_marshal_BOOLEAN__ENUM_INT_BOOLEAN_BOOLEANv);

  /* Key bindings */

#ifdef __APPLE__
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_a, GDK_META_MASK,
				       "select-all",
                                       NULL);
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_a, GDK_META_MASK | GDK_SHIFT_MASK,
				       "unselect-all",
                                       NULL);
#else
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_a, GDK_CONTROL_MASK,
				       "select-all",
                                       NULL);
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_a, GDK_CONTROL_MASK | GDK_SHIFT_MASK,
				       "unselect-all",
                                       NULL);
#endif

  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_space, GDK_CONTROL_MASK,
				       "toggle-cursor-item",
                                       NULL);
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_KP_Space, GDK_CONTROL_MASK,
				       "toggle-cursor-item",
                                       NULL);

  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_space, 0,
				       "activate-cursor-item",
                                       NULL);
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_KP_Space, 0,
				       "activate-cursor-item",
                                       NULL);
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_Return, 0,
				       "activate-cursor-item",
                                       NULL);
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_ISO_Enter, 0,
				       "activate-cursor-item",
                                       NULL);
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_KP_Enter, 0,
				       "activate-cursor-item",
                                       NULL);

  bobgui_icon_view_add_move_binding (widget_class, GDK_KEY_Up, 0,
				  BOBGUI_MOVEMENT_DISPLAY_LINES, -1);
  bobgui_icon_view_add_move_binding (widget_class, GDK_KEY_KP_Up, 0,
				  BOBGUI_MOVEMENT_DISPLAY_LINES, -1);

  bobgui_icon_view_add_move_binding (widget_class, GDK_KEY_Down, 0,
				  BOBGUI_MOVEMENT_DISPLAY_LINES, 1);
  bobgui_icon_view_add_move_binding (widget_class, GDK_KEY_KP_Down, 0,
				  BOBGUI_MOVEMENT_DISPLAY_LINES, 1);

  bobgui_icon_view_add_move_binding (widget_class, GDK_KEY_p, GDK_CONTROL_MASK,
				  BOBGUI_MOVEMENT_DISPLAY_LINES, -1);

  bobgui_icon_view_add_move_binding (widget_class, GDK_KEY_n, GDK_CONTROL_MASK,
				  BOBGUI_MOVEMENT_DISPLAY_LINES, 1);

  bobgui_icon_view_add_move_binding (widget_class, GDK_KEY_Home, 0,
				  BOBGUI_MOVEMENT_BUFFER_ENDS, -1);
  bobgui_icon_view_add_move_binding (widget_class, GDK_KEY_KP_Home, 0,
				  BOBGUI_MOVEMENT_BUFFER_ENDS, -1);

  bobgui_icon_view_add_move_binding (widget_class, GDK_KEY_End, 0,
				  BOBGUI_MOVEMENT_BUFFER_ENDS, 1);
  bobgui_icon_view_add_move_binding (widget_class, GDK_KEY_KP_End, 0,
				  BOBGUI_MOVEMENT_BUFFER_ENDS, 1);

  bobgui_icon_view_add_move_binding (widget_class, GDK_KEY_Page_Up, 0,
				  BOBGUI_MOVEMENT_PAGES, -1);
  bobgui_icon_view_add_move_binding (widget_class, GDK_KEY_KP_Page_Up, 0,
				  BOBGUI_MOVEMENT_PAGES, -1);

  bobgui_icon_view_add_move_binding (widget_class, GDK_KEY_Page_Down, 0,
				  BOBGUI_MOVEMENT_PAGES, 1);
  bobgui_icon_view_add_move_binding (widget_class, GDK_KEY_KP_Page_Down, 0,
				  BOBGUI_MOVEMENT_PAGES, 1);

  bobgui_icon_view_add_move_binding (widget_class, GDK_KEY_Right, 0,
				  BOBGUI_MOVEMENT_VISUAL_POSITIONS, 1);
  bobgui_icon_view_add_move_binding (widget_class, GDK_KEY_Left, 0,
				  BOBGUI_MOVEMENT_VISUAL_POSITIONS, -1);

  bobgui_icon_view_add_move_binding (widget_class, GDK_KEY_KP_Right, 0,
				  BOBGUI_MOVEMENT_VISUAL_POSITIONS, 1);
  bobgui_icon_view_add_move_binding (widget_class, GDK_KEY_KP_Left, 0,
				  BOBGUI_MOVEMENT_VISUAL_POSITIONS, -1);

  bobgui_widget_class_set_css_name (widget_class, I_("iconview"));
}

static void
bobgui_icon_view_buildable_add_child (BobguiBuildable *buildable,
                                   BobguiBuilder   *builder,
                                   GObject      *child,
                                   const char   *type)
{
  if (BOBGUI_IS_CELL_RENDERER (child))
    _bobgui_cell_layout_buildable_add_child (buildable, builder, child, type);
  else
    parent_buildable_iface->add_child (buildable, builder, child, type);
}

static void
bobgui_icon_view_buildable_init (BobguiBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);
  iface->add_child = bobgui_icon_view_buildable_add_child;
  iface->custom_tag_start = bobgui_icon_view_buildable_custom_tag_start;
  iface->custom_tag_end = bobgui_icon_view_buildable_custom_tag_end;
}

static void
bobgui_icon_view_cell_layout_init (BobguiCellLayoutIface *iface)
{
  iface->get_area = bobgui_icon_view_cell_layout_get_area;
}

static void
bobgui_icon_view_init (BobguiIconView *icon_view)
{
  BobguiEventController *controller;
  BobguiGesture *gesture;

  icon_view->priv = bobgui_icon_view_get_instance_private (icon_view);

  icon_view->priv->width = 0;
  icon_view->priv->height = 0;
  icon_view->priv->selection_mode = BOBGUI_SELECTION_SINGLE;
  icon_view->priv->pressed_button = -1;
  icon_view->priv->press_start_x = -1;
  icon_view->priv->press_start_y = -1;
  icon_view->priv->text_column = -1;
  icon_view->priv->markup_column = -1;
  icon_view->priv->pixbuf_column = -1;
  icon_view->priv->text_cell = NULL;
  icon_view->priv->pixbuf_cell = NULL;
  icon_view->priv->tooltip_column = -1;
  icon_view->priv->mouse_x = -1;
  icon_view->priv->mouse_y = -1;

  bobgui_widget_set_overflow (BOBGUI_WIDGET (icon_view), BOBGUI_OVERFLOW_HIDDEN);
  bobgui_widget_set_focusable (BOBGUI_WIDGET (icon_view), TRUE);

  icon_view->priv->item_orientation = BOBGUI_ORIENTATION_VERTICAL;

  icon_view->priv->columns = -1;
  icon_view->priv->item_width = -1;
  icon_view->priv->spacing = 0;
  icon_view->priv->row_spacing = 6;
  icon_view->priv->column_spacing = 6;
  icon_view->priv->margin = 6;
  icon_view->priv->item_padding = 6;
  icon_view->priv->activate_on_single_click = FALSE;

  icon_view->priv->draw_focus = TRUE;

  icon_view->priv->row_contexts =
    g_ptr_array_new_with_free_func ((GDestroyNotify)g_object_unref);

  bobgui_widget_add_css_class (BOBGUI_WIDGET (icon_view), "view");

  gesture = bobgui_gesture_click_new ();
  g_signal_connect (gesture, "pressed", G_CALLBACK (bobgui_icon_view_button_press),
                    icon_view);
  g_signal_connect (gesture, "released", G_CALLBACK (bobgui_icon_view_button_release),
                    icon_view);
  bobgui_widget_add_controller (BOBGUI_WIDGET (icon_view), BOBGUI_EVENT_CONTROLLER (gesture));

  controller = bobgui_event_controller_motion_new ();
  g_signal_connect (controller, "leave", G_CALLBACK (bobgui_icon_view_leave), icon_view);
  g_signal_connect (controller, "motion", G_CALLBACK (bobgui_icon_view_motion), icon_view);
  bobgui_widget_add_controller (BOBGUI_WIDGET (icon_view), controller);

  controller = bobgui_event_controller_key_new ();
  g_signal_connect (controller, "key-pressed", G_CALLBACK (bobgui_icon_view_key_pressed),
                    icon_view);
  bobgui_widget_add_controller (BOBGUI_WIDGET (icon_view), controller);
}

/* GObject methods */

static void
bobgui_icon_view_constructed (GObject *object)
{
  BobguiIconView *icon_view = BOBGUI_ICON_VIEW (object);

  G_OBJECT_CLASS (bobgui_icon_view_parent_class)->constructed (object);

  bobgui_icon_view_ensure_cell_area (icon_view, NULL);
}

static void
bobgui_icon_view_dispose (GObject *object)
{
  BobguiIconView *icon_view;
  BobguiIconViewPrivate *priv;

  icon_view = BOBGUI_ICON_VIEW (object);
  priv      = icon_view->priv;

  bobgui_icon_view_set_model (icon_view, NULL);

  if (icon_view->priv->scroll_to_path != NULL)
    {
      bobgui_tree_row_reference_free (icon_view->priv->scroll_to_path);
      icon_view->priv->scroll_to_path = NULL;
    }

  remove_scroll_timeout (icon_view);

  if (icon_view->priv->hadjustment != NULL)
    {
      g_object_unref (icon_view->priv->hadjustment);
      icon_view->priv->hadjustment = NULL;
    }

  if (icon_view->priv->vadjustment != NULL)
    {
      g_object_unref (icon_view->priv->vadjustment);
      icon_view->priv->vadjustment = NULL;
    }

  if (priv->cell_area_context)
    {
      g_object_unref (priv->cell_area_context);
      priv->cell_area_context = NULL;
    }

  if (priv->row_contexts)
    {
      g_ptr_array_free (priv->row_contexts, TRUE);
      priv->row_contexts = NULL;
    }

  if (priv->cell_area)
    {
      bobgui_cell_area_stop_editing (icon_view->priv->cell_area, TRUE);

      g_signal_handler_disconnect (priv->cell_area, priv->add_editable_id);
      g_signal_handler_disconnect (priv->cell_area, priv->remove_editable_id);
      priv->add_editable_id = 0;
      priv->remove_editable_id = 0;

      g_object_unref (priv->cell_area);
      priv->cell_area = NULL;
    }

  g_clear_object (&priv->key_controller);

  g_clear_pointer (&priv->source_formats, gdk_content_formats_unref);

  G_OBJECT_CLASS (bobgui_icon_view_parent_class)->dispose (object);
}

static void
bobgui_icon_view_set_property (GObject      *object,
			    guint         prop_id,
			    const GValue *value,
			    GParamSpec   *pspec)
{
  BobguiIconView *icon_view;
  BobguiCellArea *area;

  icon_view = BOBGUI_ICON_VIEW (object);

  switch (prop_id)
    {
    case PROP_SELECTION_MODE:
      bobgui_icon_view_set_selection_mode (icon_view, g_value_get_enum (value));
      break;
    case PROP_PIXBUF_COLUMN:
      bobgui_icon_view_set_pixbuf_column (icon_view, g_value_get_int (value));
      break;
    case PROP_TEXT_COLUMN:
      bobgui_icon_view_set_text_column (icon_view, g_value_get_int (value));
      break;
    case PROP_MARKUP_COLUMN:
      bobgui_icon_view_set_markup_column (icon_view, g_value_get_int (value));
      break;
    case PROP_MODEL:
      bobgui_icon_view_set_model (icon_view, g_value_get_object (value));
      break;
    case PROP_ITEM_ORIENTATION:
      bobgui_icon_view_set_item_orientation (icon_view, g_value_get_enum (value));
      break;
    case PROP_COLUMNS:
      bobgui_icon_view_set_columns (icon_view, g_value_get_int (value));
      break;
    case PROP_ITEM_WIDTH:
      bobgui_icon_view_set_item_width (icon_view, g_value_get_int (value));
      break;
    case PROP_SPACING:
      bobgui_icon_view_set_spacing (icon_view, g_value_get_int (value));
      break;
    case PROP_ROW_SPACING:
      bobgui_icon_view_set_row_spacing (icon_view, g_value_get_int (value));
      break;
    case PROP_COLUMN_SPACING:
      bobgui_icon_view_set_column_spacing (icon_view, g_value_get_int (value));
      break;
    case PROP_MARGIN:
      bobgui_icon_view_set_margin (icon_view, g_value_get_int (value));
      break;
    case PROP_REORDERABLE:
      bobgui_icon_view_set_reorderable (icon_view, g_value_get_boolean (value));
      break;

    case PROP_TOOLTIP_COLUMN:
      bobgui_icon_view_set_tooltip_column (icon_view, g_value_get_int (value));
      break;

    case PROP_ITEM_PADDING:
      bobgui_icon_view_set_item_padding (icon_view, g_value_get_int (value));
      break;

    case PROP_ACTIVATE_ON_SINGLE_CLICK:
      bobgui_icon_view_set_activate_on_single_click (icon_view, g_value_get_boolean (value));
      break;

    case PROP_CELL_AREA:
      /* Construct-only, can only be assigned once */
      area = g_value_get_object (value);
      if (area)
        {
          if (icon_view->priv->cell_area != NULL)
            {
              g_warning ("cell-area has already been set, ignoring construct property");
              g_object_ref_sink (area);
              g_object_unref (area);
            }
          else
            bobgui_icon_view_ensure_cell_area (icon_view, area);
        }
      break;

    case PROP_HADJUSTMENT:
      bobgui_icon_view_set_hadjustment (icon_view, g_value_get_object (value));
      break;
    case PROP_VADJUSTMENT:
      bobgui_icon_view_set_vadjustment (icon_view, g_value_get_object (value));
      break;
    case PROP_HSCROLL_POLICY:
      if (icon_view->priv->hscroll_policy != g_value_get_enum (value))
        {
          icon_view->priv->hscroll_policy = g_value_get_enum (value);
          bobgui_widget_queue_resize (BOBGUI_WIDGET (icon_view));
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    case PROP_VSCROLL_POLICY:
      if (icon_view->priv->vscroll_policy != g_value_get_enum (value))
        {
          icon_view->priv->vscroll_policy = g_value_get_enum (value);
          bobgui_widget_queue_resize (BOBGUI_WIDGET (icon_view));
          g_object_notify_by_pspec (object, pspec);
        }
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_icon_view_get_property (GObject      *object,
			    guint         prop_id,
			    GValue       *value,
			    GParamSpec   *pspec)
{
  BobguiIconView *icon_view;

  icon_view = BOBGUI_ICON_VIEW (object);

  switch (prop_id)
    {
    case PROP_SELECTION_MODE:
      g_value_set_enum (value, icon_view->priv->selection_mode);
      break;
    case PROP_PIXBUF_COLUMN:
      g_value_set_int (value, icon_view->priv->pixbuf_column);
      break;
    case PROP_TEXT_COLUMN:
      g_value_set_int (value, icon_view->priv->text_column);
      break;
    case PROP_MARKUP_COLUMN:
      g_value_set_int (value, icon_view->priv->markup_column);
      break;
    case PROP_MODEL:
      g_value_set_object (value, icon_view->priv->model);
      break;
    case PROP_ITEM_ORIENTATION:
      g_value_set_enum (value, icon_view->priv->item_orientation);
      break;
    case PROP_COLUMNS:
      g_value_set_int (value, icon_view->priv->columns);
      break;
    case PROP_ITEM_WIDTH:
      g_value_set_int (value, icon_view->priv->item_width);
      break;
    case PROP_SPACING:
      g_value_set_int (value, icon_view->priv->spacing);
      break;
    case PROP_ROW_SPACING:
      g_value_set_int (value, icon_view->priv->row_spacing);
      break;
    case PROP_COLUMN_SPACING:
      g_value_set_int (value, icon_view->priv->column_spacing);
      break;
    case PROP_MARGIN:
      g_value_set_int (value, icon_view->priv->margin);
      break;
    case PROP_REORDERABLE:
      g_value_set_boolean (value, icon_view->priv->reorderable);
      break;
    case PROP_TOOLTIP_COLUMN:
      g_value_set_int (value, icon_view->priv->tooltip_column);
      break;

    case PROP_ITEM_PADDING:
      g_value_set_int (value, icon_view->priv->item_padding);
      break;

    case PROP_ACTIVATE_ON_SINGLE_CLICK:
      g_value_set_boolean (value, icon_view->priv->activate_on_single_click);
      break;

    case PROP_CELL_AREA:
      g_value_set_object (value, icon_view->priv->cell_area);
      break;

    case PROP_HADJUSTMENT:
      g_value_set_object (value, icon_view->priv->hadjustment);
      break;
    case PROP_VADJUSTMENT:
      g_value_set_object (value, icon_view->priv->vadjustment);
      break;
    case PROP_HSCROLL_POLICY:
      g_value_set_enum (value, icon_view->priv->hscroll_policy);
      break;
    case PROP_VSCROLL_POLICY:
      g_value_set_enum (value, icon_view->priv->vscroll_policy);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

/* BobguiWidget methods */

static int
bobgui_icon_view_get_n_items (BobguiIconView *icon_view)
{
  BobguiIconViewPrivate *priv = icon_view->priv;

  if (priv->model == NULL)
    return 0;

  return bobgui_tree_model_iter_n_children (priv->model, NULL);
}

static void
adjust_wrap_width (BobguiIconView *icon_view)
{
  if (icon_view->priv->text_cell)
    {
      int pixbuf_width, wrap_width;

      if (icon_view->priv->items && icon_view->priv->pixbuf_cell)
        {
          bobgui_cell_renderer_get_preferred_width (icon_view->priv->pixbuf_cell,
                                                 BOBGUI_WIDGET (icon_view),
                                                 &pixbuf_width, NULL);
        }
      else
        {
          pixbuf_width = 0;
        }

      if (icon_view->priv->item_width >= 0)
        {
          if (icon_view->priv->item_orientation == BOBGUI_ORIENTATION_VERTICAL)
            {
              wrap_width = icon_view->priv->item_width;
            }
          else
            {
              wrap_width = icon_view->priv->item_width - pixbuf_width;
            }

          wrap_width -= 2 * icon_view->priv->item_padding * 2;
        }
      else
        {
          wrap_width = MAX (pixbuf_width * 2, 50);
        }

      if (icon_view->priv->items && icon_view->priv->pixbuf_cell)
	{
          /* Here we go with the same old guess, try the icon size and set double
           * the size of the first icon found in the list, naive but works much
           * of the time */

	  wrap_width = MAX (wrap_width * 2, 50);
	}

      g_object_set (icon_view->priv->text_cell, "wrap-width", wrap_width, NULL);
      g_object_set (icon_view->priv->text_cell, "width", wrap_width, NULL);
    }
}

/* General notes about layout
 *
 * The icon view is layouted like this:
 *
 * +----------+  s  +----------+
 * | padding  |  p  | padding  |
 * | +------+ |  a  | +------+ |
 * | | cell | |  c  | | cell | |
 * | +------+ |  i  | +------+ |
 * |          |  n  |          |
 * +----------+  g  +----------+
 *
 * In size request and allocation code, there are 3 sizes that are used:
 * * cell size
 *   This is the size returned by bobgui_cell_area_get_preferred_foo(). In places
 *   where code is interacting with the cell area and renderers this is useful.
 * * padded size
 *   This is the cell size plus the item padding on each side.
 * * spaced size
 *   This is the padded size plus the spacing. This is what’s used for most
 *   calculations because it can (ab)use the following formula:
 *   iconview_size = 2 * margin + n_items * spaced_size - spacing
 * So when reading this code and fixing my bugs where I confuse these two, be
 * aware of this distinction.
 */
static void
cell_area_get_preferred_size (BobguiIconView        *icon_view,
                              BobguiCellAreaContext *context,
                              BobguiOrientation      orientation,
                              int                 for_size,
                              int                *minimum,
                              int                *natural)
{
  if (orientation == BOBGUI_ORIENTATION_HORIZONTAL)
    {
      if (for_size > 0)
        bobgui_cell_area_get_preferred_width_for_height (icon_view->priv->cell_area,
                                                      context,
                                                      BOBGUI_WIDGET (icon_view),
                                                      for_size,
                                                      minimum, natural);
      else
        bobgui_cell_area_get_preferred_width (icon_view->priv->cell_area,
                                           context,
                                           BOBGUI_WIDGET (icon_view),
                                           minimum, natural);
    }
  else
    {
      if (for_size > 0)
        bobgui_cell_area_get_preferred_height_for_width (icon_view->priv->cell_area,
                                                      context,
                                                      BOBGUI_WIDGET (icon_view),
                                                      for_size,
                                                      minimum, natural);
      else
        bobgui_cell_area_get_preferred_height (icon_view->priv->cell_area,
                                            context,
                                            BOBGUI_WIDGET (icon_view),
                                            minimum, natural);
    }
}

static gboolean
bobgui_icon_view_is_empty (BobguiIconView *icon_view)
{
  return icon_view->priv->items == NULL;
}

static void
bobgui_icon_view_get_preferred_item_size (BobguiIconView    *icon_view,
                                       BobguiOrientation  orientation,
                                       int             for_size,
                                       int            *minimum,
                                       int            *natural)
{
  BobguiIconViewPrivate *priv = icon_view->priv;
  BobguiCellAreaContext *context;
  GList *items;

  g_assert (!bobgui_icon_view_is_empty (icon_view));

  context = bobgui_cell_area_create_context (priv->cell_area);

  for_size -= 2 * priv->item_padding;

  if (for_size > 0)
    {
      /* This is necessary for the context to work properly */
      for (items = priv->items; items; items = items->next)
        {
          BobguiIconViewItem *item = items->data;

          _bobgui_icon_view_set_cell_data (icon_view, item);
          cell_area_get_preferred_size (icon_view, context, 1 - orientation, -1, NULL, NULL);
        }
    }

  for (items = priv->items; items; items = items->next)
    {
      BobguiIconViewItem *item = items->data;

      _bobgui_icon_view_set_cell_data (icon_view, item);
      if (items == priv->items)
        adjust_wrap_width (icon_view);
      cell_area_get_preferred_size (icon_view, context, orientation, for_size, NULL, NULL);
    }

  if (orientation == BOBGUI_ORIENTATION_HORIZONTAL)
    {
      if (for_size > 0)
        bobgui_cell_area_context_get_preferred_width_for_height (context,
                                                              for_size,
                                                              minimum, natural);
      else
        bobgui_cell_area_context_get_preferred_width (context,
                                                   minimum, natural);
    }
  else
    {
      if (for_size > 0)
        bobgui_cell_area_context_get_preferred_height_for_width (context,
                                                              for_size,
                                                              minimum, natural);
      else
        bobgui_cell_area_context_get_preferred_height (context,
                                                    minimum, natural);
    }

  if (orientation == BOBGUI_ORIENTATION_HORIZONTAL && priv->item_width >= 0)
    {
      if (minimum)
        *minimum = MAX (*minimum, priv->item_width);
      if (natural)
        *natural = *minimum;
    }

  if (minimum)
    *minimum = MAX (1, *minimum + 2 * priv->item_padding);
  if (natural)
    *natural = MAX (1, *natural + 2 * priv->item_padding);

  g_object_unref (context);
}

static void
bobgui_icon_view_compute_n_items_for_size (BobguiIconView    *icon_view,
                                        BobguiOrientation  orientation,
                                        int             size,
                                        int            *min_items,
                                        int            *min_item_size,
                                        int            *max_items,
                                        int            *max_item_size)
{
  BobguiIconViewPrivate *priv = icon_view->priv;
  int minimum, natural, spacing;

  g_return_if_fail (min_item_size == NULL || min_items != NULL);
  g_return_if_fail (max_item_size == NULL || max_items != NULL);
  g_return_if_fail (!bobgui_icon_view_is_empty (icon_view));

  bobgui_icon_view_get_preferred_item_size (icon_view, orientation, -1, &minimum, &natural);

  if (orientation == BOBGUI_ORIENTATION_HORIZONTAL)
    spacing = priv->column_spacing;
  else
    spacing = priv->row_spacing;

  size -= 2 * priv->margin;
  size += spacing;
  minimum += spacing;
  natural += spacing;

  if (priv->columns > 0)
    {
      if (orientation == BOBGUI_ORIENTATION_HORIZONTAL)
        {
          if (min_items)
            *min_items = priv->columns;
          if (max_items)
            *max_items = priv->columns;
        }
      else
        {
          int n_items = bobgui_icon_view_get_n_items (icon_view);

          if (min_items)
            *min_items = (n_items + priv->columns - 1) / priv->columns;
          if (max_items)
            *max_items = (n_items + priv->columns - 1) / priv->columns;
        }
    }
  else
    {
      if (max_items)
        {
          if (size <= minimum)
            *max_items = 1;
          else
            *max_items = size / minimum;
        }

      if (min_items)
        {
          if (size <= natural)
            *min_items = 1;
          else
            *min_items = size / natural;
        }
    }

  if (min_item_size)
    {
      *min_item_size = size / *min_items;
      *min_item_size = CLAMP (*min_item_size, minimum, natural);
      *min_item_size -= spacing;
      *min_item_size -= 2 * priv->item_padding;
    }

  if (max_item_size)
    {
      *max_item_size = size / *max_items;
      *max_item_size = CLAMP (*max_item_size, minimum, natural);
      *max_item_size -= spacing;
      *max_item_size -= 2 * priv->item_padding;
    }
}

static BobguiSizeRequestMode
bobgui_icon_view_get_request_mode (BobguiWidget *widget)
{
  return BOBGUI_SIZE_REQUEST_HEIGHT_FOR_WIDTH;
}

static void
bobgui_icon_view_measure (BobguiWidget *widget,
                       BobguiOrientation  orientation,
                       int             for_size,
                       int            *minimum,
                       int            *natural,
                       int            *minimum_baseline,
                       int            *natural_baseline)
{
  BobguiIconView *icon_view = BOBGUI_ICON_VIEW (widget);
  BobguiIconViewPrivate *priv = icon_view->priv;
  BobguiOrientation other_orientation = orientation == BOBGUI_ORIENTATION_HORIZONTAL ?
                                     BOBGUI_ORIENTATION_VERTICAL : BOBGUI_ORIENTATION_HORIZONTAL;
  int item_min, item_nat, items = 0, item_size = 0, n_items;

  if (bobgui_icon_view_is_empty (icon_view))
    {
      *minimum = *natural = 2 * priv->margin;
      return;
    }

  n_items = bobgui_icon_view_get_n_items (icon_view);

  if (for_size < 0)
    {
      bobgui_icon_view_get_preferred_item_size (icon_view, orientation, -1, &item_min, &item_nat);

      if (priv->columns > 0)
        {
          int n_rows = (n_items + priv->columns - 1) / priv->columns;

          *minimum = item_min * n_rows + priv->row_spacing * (n_rows - 1);
          *natural = item_nat * n_rows + priv->row_spacing * (n_rows - 1);
        }
      else
        {
          *minimum = item_min;
          *natural = item_nat * n_items + priv->row_spacing * (n_items - 1);
        }
    }
  else
    {
      bobgui_icon_view_compute_n_items_for_size (icon_view, orientation, for_size, NULL, NULL, &items, &item_size);
      bobgui_icon_view_get_preferred_item_size (icon_view, other_orientation, item_size, &item_min, &item_nat);
      *minimum = (item_min + priv->row_spacing) * ((n_items + items - 1) / items) - priv->row_spacing;
      *natural = (item_nat + priv->row_spacing) * ((n_items + items - 1) / items) - priv->row_spacing;
    }

  *minimum += 2 * priv->margin;
  *natural += 2 * priv->margin;
}


static void
bobgui_icon_view_allocate_children (BobguiIconView *icon_view)
{
  GList *list;

  for (list = icon_view->priv->children; list; list = list->next)
    {
      BobguiIconViewChild *child = list->data;

      /* totally ignore our child's requisition */
      bobgui_widget_size_allocate (child->widget, &child->area, -1);
    }
}

static void
bobgui_icon_view_size_allocate (BobguiWidget *widget,
                             int        width,
                             int        height,
                             int        baseline)
{
  BobguiIconView *icon_view = BOBGUI_ICON_VIEW (widget);

  bobgui_icon_view_layout (icon_view);

  bobgui_icon_view_allocate_children (icon_view);

  /* Delay signal emission */
  g_object_freeze_notify (G_OBJECT (icon_view->priv->hadjustment));
  g_object_freeze_notify (G_OBJECT (icon_view->priv->vadjustment));

  bobgui_icon_view_set_hadjustment_values (icon_view);
  bobgui_icon_view_set_vadjustment_values (icon_view);

  if (bobgui_widget_get_realized (widget) &&
      icon_view->priv->scroll_to_path)
    {
      BobguiTreePath *path;
      path = bobgui_tree_row_reference_get_path (icon_view->priv->scroll_to_path);
      bobgui_tree_row_reference_free (icon_view->priv->scroll_to_path);
      icon_view->priv->scroll_to_path = NULL;

      bobgui_icon_view_scroll_to_path (icon_view, path,
				    icon_view->priv->scroll_to_use_align,
				    icon_view->priv->scroll_to_row_align,
				    icon_view->priv->scroll_to_col_align);
      bobgui_tree_path_free (path);
    }

  /* Emit any pending signals now */
  g_object_thaw_notify (G_OBJECT (icon_view->priv->hadjustment));
  g_object_thaw_notify (G_OBJECT (icon_view->priv->vadjustment));
}

static void
bobgui_icon_view_snapshot (BobguiWidget   *widget,
                        BobguiSnapshot *snapshot)
{
  BobguiIconView *icon_view;
  GList *icons;
  BobguiTreePath *path;
  int dest_index;
  BobguiIconViewDropPosition dest_pos;
  BobguiIconViewItem *dest_item = NULL;
  BobguiStyleContext *context;
  int width, height;
  double offset_x, offset_y;
  BobguiCssBoxes boxes;

  icon_view = BOBGUI_ICON_VIEW (widget);

  context = bobgui_widget_get_style_context (widget);
  width = bobgui_widget_get_width (widget);
  height = bobgui_widget_get_height (widget);

  offset_x = bobgui_adjustment_get_value (icon_view->priv->hadjustment);
  offset_y = bobgui_adjustment_get_value (icon_view->priv->vadjustment);

  bobgui_snapshot_save (snapshot);
  bobgui_snapshot_translate (snapshot, &GRAPHENE_POINT_INIT (- offset_x, - offset_y));

  bobgui_icon_view_get_drag_dest_item (icon_view, &path, &dest_pos);

  if (path)
    {
      dest_index = bobgui_tree_path_get_indices (path)[0];
      bobgui_tree_path_free (path);
    }
  else
    dest_index = -1;

  for (icons = icon_view->priv->items; icons; icons = icons->next)
    {
      BobguiIconViewItem *item = icons->data;
      graphene_rect_t area;

      graphene_rect_init (&area,
                          item->cell_area.x - icon_view->priv->item_padding,
                          item->cell_area.y - icon_view->priv->item_padding,
                          item->cell_area.width  + icon_view->priv->item_padding * 2,
                          item->cell_area.height + icon_view->priv->item_padding * 2);

      if (gdk_rectangle_intersect (&item->cell_area,
                                   &(GdkRectangle) { offset_x, offset_y, width, height }, NULL))
        {
          bobgui_icon_view_snapshot_item (icon_view, snapshot, item,
                                       item->cell_area.x, item->cell_area.y,
                                       icon_view->priv->draw_focus);

          if (dest_index == item->index)
            dest_item = item;
        }
    }

  if (dest_item &&
      dest_pos != BOBGUI_ICON_VIEW_NO_DROP)
    {
      GdkRectangle rect = { 0 };

      switch (dest_pos)
	{
	case BOBGUI_ICON_VIEW_DROP_INTO:
          rect = dest_item->cell_area;
	  break;
	case BOBGUI_ICON_VIEW_DROP_ABOVE:
          rect.x = dest_item->cell_area.x;
          rect.y = dest_item->cell_area.y - 1;
          rect.width = dest_item->cell_area.width;
          rect.height = 2;
	  break;
	case BOBGUI_ICON_VIEW_DROP_LEFT:
          rect.x = dest_item->cell_area.x - 1;
          rect.y = dest_item->cell_area.y;
          rect.width = 2;
          rect.height = dest_item->cell_area.height;
	  break;
	case BOBGUI_ICON_VIEW_DROP_BELOW:
          rect.x = dest_item->cell_area.x;
          rect.y = dest_item->cell_area.y + dest_item->cell_area.height - 1;
          rect.width = dest_item->cell_area.width;
          rect.height = 2;
	  break;
	case BOBGUI_ICON_VIEW_DROP_RIGHT:
          rect.x = dest_item->cell_area.x + dest_item->cell_area.width - 1;
          rect.y = dest_item->cell_area.y;
          rect.width = 2;
          rect.height = dest_item->cell_area.height;
          break;
	case BOBGUI_ICON_VIEW_NO_DROP:
        default:
	  break;
        }

      bobgui_style_context_save_to_node (context, icon_view->priv->dndnode);
      bobgui_style_context_set_state (context, bobgui_style_context_get_state (context) | BOBGUI_STATE_FLAG_DROP_ACTIVE);

      bobgui_css_boxes_init_border_box (&boxes,
                                     bobgui_style_context_lookup_style (context),
                                     rect.x, rect.y, rect.width, rect.height);
      bobgui_css_style_snapshot_border (&boxes, snapshot);

      bobgui_style_context_restore (context);
    }

  if (icon_view->priv->doing_rubberband)
    bobgui_icon_view_snapshot_rubberband (icon_view, snapshot);

  bobgui_snapshot_restore (snapshot);

  BOBGUI_WIDGET_CLASS (bobgui_icon_view_parent_class)->snapshot (widget, snapshot);
}

static gboolean
rubberband_scroll_timeout (gpointer data)
{
  BobguiIconView *icon_view = data;

  bobgui_adjustment_set_value (icon_view->priv->vadjustment,
                            bobgui_adjustment_get_value (icon_view->priv->vadjustment) +
                            icon_view->priv->scroll_value_diff);

  bobgui_icon_view_update_rubberband (icon_view);

  return TRUE;
}

static BobguiIconViewItem *
_bobgui_icon_view_get_item_at_widget_coords (BobguiIconView      *icon_view,
                                          int               x,
                                          int               y,
                                          gboolean          only_in_cell,
                                          BobguiCellRenderer **cell_at_pos)
{
  x += bobgui_adjustment_get_value (icon_view->priv->hadjustment);
  y += bobgui_adjustment_get_value (icon_view->priv->vadjustment);

  return _bobgui_icon_view_get_item_at_coords (icon_view, x, y,
                                            only_in_cell, cell_at_pos);
}

static void
bobgui_icon_view_motion (BobguiEventController *controller,
                      double              x,
                      double              y,
                      gpointer            user_data)
{
  BobguiIconView *icon_view;
  int abs_y;
  GdkDevice *device;

  icon_view = BOBGUI_ICON_VIEW (user_data);

  icon_view->priv->mouse_x = x;
  icon_view->priv->mouse_y = y;

  device = bobgui_event_controller_get_current_event_device (controller);
  bobgui_icon_view_maybe_begin_drag (icon_view, x, y, device);

  if (icon_view->priv->doing_rubberband)
    {
      int height;
      bobgui_icon_view_update_rubberband (icon_view);

      abs_y = icon_view->priv->mouse_y - icon_view->priv->height *
	(bobgui_adjustment_get_value (icon_view->priv->vadjustment) /
	 (bobgui_adjustment_get_upper (icon_view->priv->vadjustment) -
	  bobgui_adjustment_get_lower (icon_view->priv->vadjustment)));

      height = bobgui_widget_get_height (BOBGUI_WIDGET (icon_view));


      if (abs_y < 0 || abs_y > height)
	{
	  if (abs_y < 0)
	    icon_view->priv->scroll_value_diff = abs_y;
	  else
	    icon_view->priv->scroll_value_diff = abs_y - height;

	  icon_view->priv->event_last_x = icon_view->priv->mouse_x;
	  icon_view->priv->event_last_y = icon_view->priv->mouse_x;

	  if (icon_view->priv->scroll_timeout_id == 0) {
	    icon_view->priv->scroll_timeout_id = g_timeout_add (30, rubberband_scroll_timeout, icon_view);
	    gdk_source_set_static_name_by_id (icon_view->priv->scroll_timeout_id, "[bobgui] rubberband_scroll_timeout");
	  }
 	}
      else
	remove_scroll_timeout (icon_view);
    }
  else
    {
      BobguiIconViewItem *item, *last_prelight_item;
      BobguiCellRenderer *cell = NULL;

      last_prelight_item = icon_view->priv->last_prelight;
      item = _bobgui_icon_view_get_item_at_widget_coords (icon_view,
                                                       icon_view->priv->mouse_x,
                                                       icon_view->priv->mouse_y,
                                                       FALSE,
                                                       &cell);

      if (item != last_prelight_item)
        {
          if (item != NULL)
            {
              bobgui_icon_view_queue_draw_item (icon_view, item);
            }

          if (last_prelight_item != NULL)
            {
              bobgui_icon_view_queue_draw_item (icon_view,
                                             icon_view->priv->last_prelight);
            }

          icon_view->priv->last_prelight = item;
        }
    }
}

static void
bobgui_icon_view_leave (BobguiEventController *controller,
                     gpointer            user_data)
{
  BobguiIconView *icon_view;
  BobguiIconViewPrivate *priv;

  icon_view = BOBGUI_ICON_VIEW (user_data);
  priv = icon_view->priv;

  if (priv->last_prelight)
    {
      bobgui_icon_view_queue_draw_item (icon_view, priv->last_prelight);
      priv->last_prelight = NULL;
    }
}

static void
bobgui_icon_view_remove (BobguiIconView *icon_view,
                      BobguiWidget   *widget)
{
  BobguiIconViewChild *child = NULL;
  GList *tmp_list;

  tmp_list = icon_view->priv->children;
  while (tmp_list)
    {
      child = tmp_list->data;
      if (child->widget == widget)
	{
	  bobgui_widget_unparent (widget);

	  icon_view->priv->children = g_list_remove_link (icon_view->priv->children, tmp_list);
	  g_list_free_1 (tmp_list);
	  g_free (child);
	  return;
	}

      tmp_list = tmp_list->next;
    }
}

static void
bobgui_icon_view_add_editable (BobguiCellArea            *area,
			    BobguiCellRenderer        *renderer,
			    BobguiCellEditable        *editable,
			    GdkRectangle           *cell_area,
			    const char             *path,
			    BobguiIconView            *icon_view)
{
  BobguiIconViewChild *child;
  BobguiWidget *widget = BOBGUI_WIDGET (editable);

  child = g_new (BobguiIconViewChild, 1);

  child->widget      = widget;
  child->area.x      = cell_area->x;
  child->area.y      = cell_area->y;
  child->area.width  = cell_area->width;
  child->area.height = cell_area->height;

  icon_view->priv->children = g_list_append (icon_view->priv->children, child);

  bobgui_widget_set_parent (widget, BOBGUI_WIDGET (icon_view));
}

static void
bobgui_icon_view_remove_editable (BobguiCellArea            *area,
                               BobguiCellRenderer        *renderer,
                               BobguiCellEditable        *editable,
                               BobguiIconView            *icon_view)
{
  BobguiTreePath *path;

  if (bobgui_widget_has_focus (BOBGUI_WIDGET (editable)))
    bobgui_widget_grab_focus (BOBGUI_WIDGET (icon_view));

  bobgui_icon_view_remove (icon_view, BOBGUI_WIDGET (editable));

  path = bobgui_tree_path_new_from_string (bobgui_cell_area_get_current_path_string (area));
  bobgui_icon_view_queue_draw_path (icon_view, path);
  bobgui_tree_path_free (path);
}

/**
 * bobgui_icon_view_set_cursor:
 * @icon_view: A `BobguiIconView`
 * @path: A `BobguiTreePath`
 * @cell: (nullable): One of the cell renderers of @icon_view
 * @start_editing: %TRUE if the specified cell should start being edited.
 *
 * Sets the current keyboard focus to be at @path, and selects it.  This is
 * useful when you want to focus the user’s attention on a particular item.
 * If @cell is not %NULL, then focus is given to the cell specified by
 * it. Additionally, if @start_editing is %TRUE, then editing should be
 * started in the specified cell.
 *
 * This function is often followed by `bobgui_widget_grab_focus
 * (icon_view)` in order to give keyboard focus to the widget.
 * Please note that editing can only happen when the widget is realized.
 *
 * Deprecated: 4.10: Use [class@Bobgui.GridView] instead
 **/
void
bobgui_icon_view_set_cursor (BobguiIconView     *icon_view,
			  BobguiTreePath     *path,
			  BobguiCellRenderer *cell,
			  gboolean         start_editing)
{
  BobguiIconViewItem *item = NULL;

  g_return_if_fail (BOBGUI_IS_ICON_VIEW (icon_view));
  g_return_if_fail (path != NULL);
  g_return_if_fail (cell == NULL || BOBGUI_IS_CELL_RENDERER (cell));

  if (icon_view->priv->cell_area)
    bobgui_cell_area_stop_editing (icon_view->priv->cell_area, TRUE);

  if (bobgui_tree_path_get_depth (path) == 1)
    item = g_list_nth_data (icon_view->priv->items,
			    bobgui_tree_path_get_indices(path)[0]);

  if (!item)
    return;

  _bobgui_icon_view_set_cursor_item (icon_view, item, cell);
  bobgui_icon_view_scroll_to_path (icon_view, path, FALSE, 0.0, 0.0);

  if (start_editing &&
      icon_view->priv->cell_area)
    {
      BobguiCellAreaContext *context;

      context = g_ptr_array_index (icon_view->priv->row_contexts, item->row);
      _bobgui_icon_view_set_cell_data (icon_view, item);
      bobgui_cell_area_activate (icon_view->priv->cell_area, context,
			      BOBGUI_WIDGET (icon_view), &item->cell_area,
			      0, TRUE);
    }
}

/**
 * bobgui_icon_view_get_cursor:
 * @icon_view: A `BobguiIconView`
 * @path: (out) (optional) (transfer full): Return location for the current
 *   cursor path
 * @cell: (out) (optional) (transfer none): Return location the current
 *   focus cell
 *
 * Fills in @path and @cell with the current cursor path and cell.
 * If the cursor isn’t currently set, then *@path will be %NULL.
 * If no cell currently has focus, then *@cell will be %NULL.
 *
 * The returned `BobguiTreePath` must be freed with bobgui_tree_path_free().
 *
 * Returns: %TRUE if the cursor is set.
 *
 * Deprecated: 4.10: Use [class@Bobgui.GridView] instead
 **/
gboolean
bobgui_icon_view_get_cursor (BobguiIconView      *icon_view,
			  BobguiTreePath     **path,
			  BobguiCellRenderer **cell)
{
  BobguiIconViewItem *item;

  g_return_val_if_fail (BOBGUI_IS_ICON_VIEW (icon_view), FALSE);

  item = icon_view->priv->cursor_item;

  if (path != NULL)
    {
      if (item != NULL)
	*path = bobgui_tree_path_new_from_indices (item->index, -1);
      else
	*path = NULL;
    }

  if (cell != NULL && item != NULL && icon_view->priv->cell_area != NULL)
    *cell = bobgui_cell_area_get_focus_cell (icon_view->priv->cell_area);

  return (item != NULL);
}

static void
bobgui_icon_view_button_press (BobguiGestureClick *gesture,
                            int              n_press,
                            double           x,
                            double           y,
                            gpointer         user_data)
{
  BobguiIconView *icon_view = user_data;
  BobguiWidget *widget = BOBGUI_WIDGET (icon_view);
  BobguiIconViewItem *item;
  gboolean dirty = FALSE;
  BobguiCellRenderer *cell = NULL, *cursor_cell = NULL;
  int button = bobgui_gesture_single_get_current_button (BOBGUI_GESTURE_SINGLE (gesture));
  GdkEventSequence *sequence = bobgui_gesture_single_get_current_sequence (BOBGUI_GESTURE_SINGLE (gesture));
  GdkEvent *event = bobgui_gesture_get_last_event (BOBGUI_GESTURE (gesture), sequence);
  GdkModifierType state;

  if (!bobgui_widget_has_focus (widget))
    bobgui_widget_grab_focus (widget);

  if (button == GDK_BUTTON_PRIMARY)
    {
      GdkModifierType extend_mod_mask = GDK_SHIFT_MASK;
#ifdef __APPLE__
      GdkModifierType modify_mod_mask = GDK_META_MASK;
#else
      GdkModifierType modify_mod_mask = GDK_CONTROL_MASK;
#endif

      state = gdk_event_get_modifier_state (event);

      item = _bobgui_icon_view_get_item_at_widget_coords (icon_view,
                                                       x, y,
                                                       FALSE,
                                                       &cell);

      /*
       * We consider only the cells' area as the item area if the
       * item is not selected, but if it *is* selected, the complete
       * selection rectangle is considered to be part of the item.
       */
      if (item != NULL && (cell != NULL || item->selected))
	{
	  if (cell != NULL)
	    {
	      if (bobgui_cell_renderer_is_activatable (cell))
		cursor_cell = cell;
	    }

	  bobgui_icon_view_scroll_to_item (icon_view, item);

	  if (icon_view->priv->selection_mode == BOBGUI_SELECTION_NONE)
	    {
	      _bobgui_icon_view_set_cursor_item (icon_view, item, cursor_cell);
	    }
	  else if (icon_view->priv->selection_mode == BOBGUI_SELECTION_MULTIPLE &&
		   (state & extend_mod_mask))
	    {
	      bobgui_icon_view_unselect_all_internal (icon_view);

	      _bobgui_icon_view_set_cursor_item (icon_view, item, cursor_cell);
	      if (!icon_view->priv->anchor_item)
		icon_view->priv->anchor_item = item;
	      else
		bobgui_icon_view_select_all_between (icon_view,
						  icon_view->priv->anchor_item,
						  item);
	      dirty = TRUE;
	    }
	  else
	    {
	      if ((icon_view->priv->selection_mode == BOBGUI_SELECTION_MULTIPLE ||
		  ((icon_view->priv->selection_mode == BOBGUI_SELECTION_SINGLE) && item->selected)) &&
		  (state & modify_mod_mask))
		{
		  item->selected = !item->selected;
		  bobgui_icon_view_queue_draw_item (icon_view, item);
		  dirty = TRUE;
		}
	      else
		{
		  bobgui_icon_view_unselect_all_internal (icon_view);

		  item->selected = TRUE;
		  bobgui_icon_view_queue_draw_item (icon_view, item);
		  dirty = TRUE;
		}
	      _bobgui_icon_view_set_cursor_item (icon_view, item, cursor_cell);
	      icon_view->priv->anchor_item = item;
	    }

	  /* Save press to possibly begin a drag */
	  if (icon_view->priv->pressed_button < 0)
	    {
	      icon_view->priv->pressed_button = button;
	      icon_view->priv->press_start_x = x;
	      icon_view->priv->press_start_y = y;
	    }

          icon_view->priv->last_single_clicked = item;

	  /* cancel the current editing, if it exists */
	  bobgui_cell_area_stop_editing (icon_view->priv->cell_area, TRUE);

	  if (cell != NULL && bobgui_cell_renderer_is_activatable (cell))
	    {
	      BobguiCellAreaContext *context;

	      context = g_ptr_array_index (icon_view->priv->row_contexts, item->row);

	      _bobgui_icon_view_set_cell_data (icon_view, item);
	      bobgui_cell_area_activate (icon_view->priv->cell_area, context,
				      BOBGUI_WIDGET (icon_view),
				      &item->cell_area, 0, FALSE);
	    }
	}
      else
	{
	  if (icon_view->priv->selection_mode != BOBGUI_SELECTION_BROWSE &&
	      !(state & modify_mod_mask))
	    {
	      dirty = bobgui_icon_view_unselect_all_internal (icon_view);
	    }

	  if (icon_view->priv->selection_mode == BOBGUI_SELECTION_MULTIPLE)
            bobgui_icon_view_start_rubberbanding (icon_view,
					       bobgui_gesture_get_device (BOBGUI_GESTURE (gesture)),
					       x, y);
	}

      /* don't draw keyboard focus around a clicked-on item */
      icon_view->priv->draw_focus = FALSE;
    }

  if (!icon_view->priv->activate_on_single_click
      && button == GDK_BUTTON_PRIMARY
      && n_press == 2)
    {
      item = _bobgui_icon_view_get_item_at_widget_coords (icon_view,
                                                       x, y,
                                                       FALSE,
                                                       NULL);

      if (item && item == icon_view->priv->last_single_clicked)
	{
	  BobguiTreePath *path;

	  path = bobgui_tree_path_new_from_indices (item->index, -1);
	  bobgui_icon_view_item_activated (icon_view, path);
	  bobgui_tree_path_free (path);
	}

      icon_view->priv->last_single_clicked = NULL;
      icon_view->priv->pressed_button = -1;
    }

  if (dirty)
    g_signal_emit (icon_view, icon_view_signals[SELECTION_CHANGED], 0);
}

static gboolean
button_event_modifies_selection (GdkEvent *event)
{
  guint state = gdk_event_get_modifier_state (event);

  return (state & (GDK_CONTROL_MASK | GDK_SHIFT_MASK)) != 0;
}

static void
bobgui_icon_view_button_release (BobguiGestureClick *gesture,
                              int              n_press,
                              double           x,
                              double           y,
                              gpointer         user_data)
{
  BobguiIconView *icon_view = user_data;
  int button = bobgui_gesture_single_get_current_button (BOBGUI_GESTURE_SINGLE (gesture));
  GdkEventSequence *sequence = bobgui_gesture_single_get_current_sequence (BOBGUI_GESTURE_SINGLE (gesture));
  GdkEvent *event = bobgui_gesture_get_last_event (BOBGUI_GESTURE (gesture), sequence);

  if (icon_view->priv->pressed_button == button)
    icon_view->priv->pressed_button = -1;

  bobgui_icon_view_stop_rubberbanding (icon_view);

  remove_scroll_timeout (icon_view);

  if (button == GDK_BUTTON_PRIMARY
      && icon_view->priv->activate_on_single_click
      && !button_event_modifies_selection (event)
      && icon_view->priv->last_single_clicked != NULL)
    {
      BobguiIconViewItem *item;

      item = _bobgui_icon_view_get_item_at_widget_coords (icon_view,
                                                       x, y,
                                                       FALSE,
                                                       NULL);
      if (item == icon_view->priv->last_single_clicked)
        {
          BobguiTreePath *path;
          path = bobgui_tree_path_new_from_indices (item->index, -1);
          bobgui_icon_view_item_activated (icon_view, path);
          bobgui_tree_path_free (path);
        }

      icon_view->priv->last_single_clicked = NULL;
    }
}

static gboolean
bobgui_icon_view_key_pressed (BobguiEventControllerKey *controller,
                           guint                  keyval,
                           guint                  keycode,
                           GdkModifierType        state,
                           BobguiWidget             *widget)
{
  BobguiIconView *icon_view = BOBGUI_ICON_VIEW (widget);

  if (icon_view->priv->doing_rubberband)
    {
      if (keyval == GDK_KEY_Escape)
	bobgui_icon_view_stop_rubberbanding (icon_view);

      return TRUE;
    }

  return FALSE;
}

static void
bobgui_icon_view_update_rubberband (BobguiIconView *icon_view)
{
  int x, y;

  x = MAX (icon_view->priv->mouse_x, 0);
  y = MAX (icon_view->priv->mouse_y, 0);

  icon_view->priv->rubberband_x2 = x + bobgui_adjustment_get_value (icon_view->priv->hadjustment);
  icon_view->priv->rubberband_y2 = y + bobgui_adjustment_get_value (icon_view->priv->vadjustment);

  bobgui_icon_view_update_rubberband_selection (icon_view);
  bobgui_widget_queue_draw (BOBGUI_WIDGET (icon_view));
}

static void
bobgui_icon_view_start_rubberbanding (BobguiIconView  *icon_view,
                                   GdkDevice    *device,
				   int           x,
				   int           y)
{
  BobguiIconViewPrivate *priv = icon_view->priv;
  GList *items;
  BobguiCssNode *widget_node;

  if (priv->rubberband_device)
    return;

  for (items = priv->items; items; items = items->next)
    {
      BobguiIconViewItem *item = items->data;

      item->selected_before_rubberbanding = item->selected;
    }

  priv->rubberband_x1 = x + bobgui_adjustment_get_value (priv->hadjustment);
  priv->rubberband_y1 = y + bobgui_adjustment_get_value (priv->vadjustment);
  priv->rubberband_x2 = priv->rubberband_x1;
  priv->rubberband_y2 = priv->rubberband_y1;

  priv->doing_rubberband = TRUE;
  priv->rubberband_device = device;

  widget_node = bobgui_widget_get_css_node (BOBGUI_WIDGET (icon_view));
  priv->rubberband_node = bobgui_css_node_new ();
  bobgui_css_node_set_name (priv->rubberband_node, g_quark_from_static_string ("rubberband"));
  bobgui_css_node_set_parent (priv->rubberband_node, widget_node);
  bobgui_css_node_set_state (priv->rubberband_node, bobgui_css_node_get_state (widget_node));
  g_object_unref (priv->rubberband_node);
}

static void
bobgui_icon_view_stop_rubberbanding (BobguiIconView *icon_view)
{
  BobguiIconViewPrivate *priv = icon_view->priv;

  if (!priv->doing_rubberband)
    return;

  priv->doing_rubberband = FALSE;
  priv->rubberband_device = NULL;
  bobgui_css_node_set_parent (priv->rubberband_node, NULL);
  priv->rubberband_node = NULL;

  bobgui_widget_queue_draw (BOBGUI_WIDGET (icon_view));
}

static void
bobgui_icon_view_update_rubberband_selection (BobguiIconView *icon_view)
{
  GList *items;
  int x, y, width, height;
  gboolean dirty = FALSE;

  x = MIN (icon_view->priv->rubberband_x1,
	   icon_view->priv->rubberband_x2);
  y = MIN (icon_view->priv->rubberband_y1,
	   icon_view->priv->rubberband_y2);
  width = ABS (icon_view->priv->rubberband_x1 -
	       icon_view->priv->rubberband_x2);
  height = ABS (icon_view->priv->rubberband_y1 -
		icon_view->priv->rubberband_y2);

  for (items = icon_view->priv->items; items; items = items->next)
    {
      BobguiIconViewItem *item = items->data;
      gboolean is_in;
      gboolean selected;

      is_in = bobgui_icon_view_item_hit_test (icon_view, item,
					   x, y, width, height);

      selected = is_in ^ item->selected_before_rubberbanding;

      if (item->selected != selected)
	{
	  item->selected = selected;
	  dirty = TRUE;
	  bobgui_icon_view_queue_draw_item (icon_view, item);
	}
    }

  if (dirty)
    g_signal_emit (icon_view, icon_view_signals[SELECTION_CHANGED], 0);
}


typedef struct {
  GdkRectangle hit_rect;
  gboolean     hit;
} HitTestData;

static gboolean
hit_test (BobguiCellRenderer    *renderer,
	  const GdkRectangle *cell_area,
	  const GdkRectangle *cell_background,
	  HitTestData        *data)
{
  if (MIN (data->hit_rect.x + data->hit_rect.width, cell_area->x + cell_area->width) -
      MAX (data->hit_rect.x, cell_area->x) > 0 &&
      MIN (data->hit_rect.y + data->hit_rect.height, cell_area->y + cell_area->height) -
      MAX (data->hit_rect.y, cell_area->y) > 0)
    data->hit = TRUE;

  return (data->hit != FALSE);
}

static gboolean
bobgui_icon_view_item_hit_test (BobguiIconView      *icon_view,
			     BobguiIconViewItem  *item,
			     int               x,
			     int               y,
			     int               width,
			     int               height)
{
  HitTestData data = { { x, y, width, height }, FALSE };
  BobguiCellAreaContext *context;
  GdkRectangle *item_area = &item->cell_area;

  if (MIN (x + width, item_area->x + item_area->width) - MAX (x, item_area->x) <= 0 ||
      MIN (y + height, item_area->y + item_area->height) - MAX (y, item_area->y) <= 0)
    return FALSE;

  context = g_ptr_array_index (icon_view->priv->row_contexts, item->row);

  _bobgui_icon_view_set_cell_data (icon_view, item);
  bobgui_cell_area_foreach_alloc (icon_view->priv->cell_area, context,
			       BOBGUI_WIDGET (icon_view),
			       item_area, item_area,
			       (BobguiCellAllocCallback)hit_test, &data);

  return data.hit;
}

static gboolean
bobgui_icon_view_unselect_all_internal (BobguiIconView  *icon_view)
{
  gboolean dirty = FALSE;
  GList *items;

  if (icon_view->priv->selection_mode == BOBGUI_SELECTION_NONE)
    return FALSE;

  for (items = icon_view->priv->items; items; items = items->next)
    {
      BobguiIconViewItem *item = items->data;

      if (item->selected)
	{
	  item->selected = FALSE;
	  dirty = TRUE;
	  bobgui_icon_view_queue_draw_item (icon_view, item);
	}
    }

  return dirty;
}


/* BobguiIconView signals */
static void
bobgui_icon_view_real_select_all (BobguiIconView *icon_view)
{
  bobgui_icon_view_select_all (icon_view);
}

static void
bobgui_icon_view_real_unselect_all (BobguiIconView *icon_view)
{
  bobgui_icon_view_unselect_all (icon_view);
}

static void
bobgui_icon_view_real_select_cursor_item (BobguiIconView *icon_view)
{
  bobgui_icon_view_unselect_all (icon_view);

  if (icon_view->priv->cursor_item != NULL)
    _bobgui_icon_view_select_item (icon_view, icon_view->priv->cursor_item);
}

static gboolean
bobgui_icon_view_real_activate_cursor_item (BobguiIconView *icon_view)
{
  BobguiTreePath *path;
  BobguiCellAreaContext *context;

  if (!icon_view->priv->cursor_item)
    return FALSE;

  context = g_ptr_array_index (icon_view->priv->row_contexts, icon_view->priv->cursor_item->row);

  _bobgui_icon_view_set_cell_data (icon_view, icon_view->priv->cursor_item);
  bobgui_cell_area_activate (icon_view->priv->cell_area, context,
                          BOBGUI_WIDGET (icon_view),
                          &icon_view->priv->cursor_item->cell_area,
                          0,
                          FALSE);

  path = bobgui_tree_path_new_from_indices (icon_view->priv->cursor_item->index, -1);
  bobgui_icon_view_item_activated (icon_view, path);
  bobgui_tree_path_free (path);

  return TRUE;
}

static void
bobgui_icon_view_real_toggle_cursor_item (BobguiIconView *icon_view)
{
  if (!icon_view->priv->cursor_item)
    return;

  switch (icon_view->priv->selection_mode)
    {
    case BOBGUI_SELECTION_NONE:
    default:
      break;
    case BOBGUI_SELECTION_BROWSE:
      _bobgui_icon_view_select_item (icon_view, icon_view->priv->cursor_item);
      break;
    case BOBGUI_SELECTION_SINGLE:
      if (icon_view->priv->cursor_item->selected)
	_bobgui_icon_view_unselect_item (icon_view, icon_view->priv->cursor_item);
      else
	_bobgui_icon_view_select_item (icon_view, icon_view->priv->cursor_item);
      break;
    case BOBGUI_SELECTION_MULTIPLE:
      icon_view->priv->cursor_item->selected = !icon_view->priv->cursor_item->selected;
      g_signal_emit (icon_view, icon_view_signals[SELECTION_CHANGED], 0);

      bobgui_icon_view_queue_draw_item (icon_view, icon_view->priv->cursor_item);
      break;
    }
}

static void
bobgui_icon_view_set_hadjustment_values (BobguiIconView *icon_view)
{
  int width;
  BobguiAdjustment *adj = icon_view->priv->hadjustment;
  double old_page_size;
  double old_upper;
  double old_value;
  double new_value;
  double new_upper;

  width = bobgui_widget_get_width (BOBGUI_WIDGET (icon_view));

  old_value = bobgui_adjustment_get_value (adj);
  old_upper = bobgui_adjustment_get_upper (adj);
  old_page_size = bobgui_adjustment_get_page_size (adj);
  new_upper = MAX (width, icon_view->priv->width);

  if (bobgui_widget_get_direction (BOBGUI_WIDGET (icon_view)) == BOBGUI_TEXT_DIR_RTL)
    {
      /* Make sure no scrolling occurs for RTL locales also (if possible) */
      /* Quick explanation:
       *   In LTR locales, leftmost portion of visible rectangle should stay
       *   fixed, which means left edge of scrollbar thumb should remain fixed
       *   and thus adjustment's value should stay the same.
       *
       *   In RTL locales, we want to keep rightmost portion of visible
       *   rectangle fixed. This means right edge of thumb should remain fixed.
       *   In this case, upper - value - page_size should remain constant.
       */
      new_value = (new_upper - width) -
                  (old_upper - old_value - old_page_size);
      new_value = CLAMP (new_value, 0, new_upper - width);
    }
  else
    new_value = CLAMP (old_value, 0, new_upper - width);

  bobgui_adjustment_configure (adj,
                            new_value,
                            0.0,
                            new_upper,
                            width * 0.1,
                            width * 0.9,
                            width);
}

static void
bobgui_icon_view_set_vadjustment_values (BobguiIconView *icon_view)
{
  int height;
  BobguiAdjustment *adj = icon_view->priv->vadjustment;

  height = bobgui_widget_get_height (BOBGUI_WIDGET (icon_view));

  bobgui_adjustment_configure (adj,
                            bobgui_adjustment_get_value (adj),
                            0.0,
                            MAX (height, icon_view->priv->height),
                            height * 0.1,
                            height * 0.9,
                            height);
}

static void
bobgui_icon_view_set_hadjustment (BobguiIconView   *icon_view,
                               BobguiAdjustment *adjustment)
{
  BobguiIconViewPrivate *priv = icon_view->priv;

  if (adjustment && priv->hadjustment == adjustment)
    return;

  if (priv->hadjustment != NULL)
    {
      g_signal_handlers_disconnect_matched (priv->hadjustment,
                                            G_SIGNAL_MATCH_DATA,
                                            0, 0, NULL, NULL, icon_view);
      g_object_unref (priv->hadjustment);
    }

  if (!adjustment)
    adjustment = bobgui_adjustment_new (0.0, 0.0, 0.0,
                                     0.0, 0.0, 0.0);

  g_signal_connect (adjustment, "value-changed",
                    G_CALLBACK (bobgui_icon_view_adjustment_changed), icon_view);
  priv->hadjustment = g_object_ref_sink (adjustment);
  bobgui_icon_view_set_hadjustment_values (icon_view);

  g_object_notify (G_OBJECT (icon_view), "hadjustment");
}

static void
bobgui_icon_view_set_vadjustment (BobguiIconView   *icon_view,
                               BobguiAdjustment *adjustment)
{
  BobguiIconViewPrivate *priv = icon_view->priv;

  if (adjustment && priv->vadjustment == adjustment)
    return;

  if (priv->vadjustment != NULL)
    {
      g_signal_handlers_disconnect_matched (priv->vadjustment,
                                            G_SIGNAL_MATCH_DATA,
                                            0, 0, NULL, NULL, icon_view);
      g_object_unref (priv->vadjustment);
    }

  if (!adjustment)
    adjustment = bobgui_adjustment_new (0.0, 0.0, 0.0,
                                     0.0, 0.0, 0.0);

  g_signal_connect (adjustment, "value-changed",
                    G_CALLBACK (bobgui_icon_view_adjustment_changed), icon_view);
  priv->vadjustment = g_object_ref_sink (adjustment);
  bobgui_icon_view_set_vadjustment_values (icon_view);

  g_object_notify (G_OBJECT (icon_view), "vadjustment");
}

static void
bobgui_icon_view_adjustment_changed (BobguiAdjustment *adjustment,
                                  BobguiIconView   *icon_view)
{
  BobguiWidget *widget = BOBGUI_WIDGET (icon_view);

  if (bobgui_widget_get_realized (widget))
    {
      if (icon_view->priv->doing_rubberband)
        bobgui_icon_view_update_rubberband (icon_view);
    }

  bobgui_widget_queue_draw (BOBGUI_WIDGET (icon_view));
}

static int
compare_sizes (gconstpointer p1,
	       gconstpointer p2,
               gpointer      unused)
{
  return GPOINTER_TO_INT (((const BobguiRequestedSize *) p1)->data)
       - GPOINTER_TO_INT (((const BobguiRequestedSize *) p2)->data);
}

static void
bobgui_icon_view_layout (BobguiIconView *icon_view)
{
  BobguiIconViewPrivate *priv = icon_view->priv;
  BobguiWidget *widget = BOBGUI_WIDGET (icon_view);
  GList *items;
  int item_width = 0; /* this doesn't include item_padding */
  int n_columns, n_rows, n_items;
  int col, row;
  BobguiRequestedSize *sizes;
  gboolean rtl;
  int width, height;

  if (bobgui_icon_view_is_empty (icon_view))
    return;

  rtl = bobgui_widget_get_direction (BOBGUI_WIDGET (icon_view)) == BOBGUI_TEXT_DIR_RTL;
  n_items = bobgui_icon_view_get_n_items (icon_view);

  width = bobgui_widget_get_width (widget);
  height = bobgui_widget_get_height (widget);

  bobgui_icon_view_compute_n_items_for_size (icon_view,
                                          BOBGUI_ORIENTATION_HORIZONTAL,
                                          width,
                                          NULL, NULL,
                                          &n_columns, &item_width);
  n_rows = (n_items + n_columns - 1) / n_columns;

  priv->width = n_columns * (item_width + 2 * priv->item_padding + priv->column_spacing) - priv->column_spacing;
  priv->width += 2 * priv->margin;
  priv->width = MAX (priv->width, width);

  /* Clear the per row contexts */
  g_ptr_array_set_size (icon_view->priv->row_contexts, 0);

  bobgui_cell_area_context_reset (priv->cell_area_context);
  /* because layouting is complicated. We designed an API
   * that is O(N²) and nonsensical.
   * And we're proud of it. */
  for (items = priv->items; items; items = items->next)
    {
      _bobgui_icon_view_set_cell_data (icon_view, items->data);
      bobgui_cell_area_get_preferred_width (priv->cell_area,
                                         priv->cell_area_context,
                                         widget,
                                         NULL, NULL);
    }

  sizes = g_newa (BobguiRequestedSize, n_rows);
  items = priv->items;
  priv->height = priv->margin;

  /* Collect the heights for all rows */
  for (row = 0; row < n_rows; row++)
    {
      BobguiCellAreaContext *context = bobgui_cell_area_copy_context (priv->cell_area, priv->cell_area_context);
      g_ptr_array_add (priv->row_contexts, context);

      for (col = 0; col < n_columns && items; col++, items = items->next)
        {
          BobguiIconViewItem *item = items->data;

          _bobgui_icon_view_set_cell_data (icon_view, item);
          bobgui_cell_area_get_preferred_height_for_width (priv->cell_area,
                                                        context,
                                                        widget,
                                                        item_width,
                                                        NULL, NULL);
        }

      sizes[row].data = GINT_TO_POINTER (row);
      bobgui_cell_area_context_get_preferred_height_for_width (context,
                                                            item_width,
                                                            &sizes[row].minimum_size,
                                                            &sizes[row].natural_size);
      priv->height += sizes[row].minimum_size + 2 * priv->item_padding + priv->row_spacing;
    }

  priv->height -= priv->row_spacing;
  priv->height += priv->margin;
  priv->height = MIN (priv->height, height);

  bobgui_distribute_natural_allocation (height - priv->height,
                                     n_rows,
                                     sizes);

  /* Actually allocate the rows */
  g_sort_array (sizes, n_rows, sizeof (BobguiRequestedSize), compare_sizes, NULL);

  items = priv->items;
  priv->height = priv->margin;

  for (row = 0; row < n_rows; row++)
    {
      BobguiCellAreaContext *context = g_ptr_array_index (priv->row_contexts, row);
      bobgui_cell_area_context_allocate (context, item_width, sizes[row].minimum_size);

      priv->height += priv->item_padding;

      for (col = 0; col < n_columns && items; col++, items = items->next)
        {
          BobguiIconViewItem *item = items->data;

          item->cell_area.x = priv->margin + (col * 2 + 1) * priv->item_padding + col * (priv->column_spacing + item_width);
          item->cell_area.width = item_width;
          item->cell_area.y = priv->height;
          item->cell_area.height = sizes[row].minimum_size;
          item->row = row;
          item->col = col;
          if (rtl)
            {
              item->cell_area.x = priv->width - item_width - item->cell_area.x;
              item->col = n_columns - 1 - col;
            }
        }

      priv->height += sizes[row].minimum_size + priv->item_padding + priv->row_spacing;
    }

  priv->height -= priv->row_spacing;
  priv->height += priv->margin;
  priv->height = MAX (priv->height, height);
}

static void
bobgui_icon_view_invalidate_sizes (BobguiIconView *icon_view)
{
  /* Clear all item sizes */
  g_list_foreach (icon_view->priv->items,
		  (GFunc)bobgui_icon_view_item_invalidate_size, NULL);

  /* Re-layout the items */
  bobgui_widget_queue_resize (BOBGUI_WIDGET (icon_view));
}

static void
bobgui_icon_view_item_invalidate_size (BobguiIconViewItem *item)
{
  item->cell_area.width = -1;
  item->cell_area.height = -1;
}

static void
bobgui_icon_view_snapshot_item (BobguiIconView     *icon_view,
                             BobguiSnapshot     *snapshot,
                             BobguiIconViewItem *item,
                             int              x,
                             int              y,
                             gboolean         draw_focus)
{
  GdkRectangle cell_area;
  BobguiStateFlags state = 0;
  BobguiCellRendererState flags = 0;
  BobguiStyleContext *style_context;
  BobguiWidget *widget = BOBGUI_WIDGET (icon_view);
  BobguiIconViewPrivate *priv = icon_view->priv;
  BobguiCellAreaContext *context;
  BobguiCssBoxes boxes;

  if (priv->model == NULL || item->cell_area.width <= 0 || item->cell_area.height <= 0)
    return;

  _bobgui_icon_view_set_cell_data (icon_view, item);

  style_context = bobgui_widget_get_style_context (widget);
  state = bobgui_widget_get_state_flags (widget);

  bobgui_style_context_save (style_context);
  bobgui_style_context_add_class (style_context, "cell");

  state &= ~(BOBGUI_STATE_FLAG_SELECTED | BOBGUI_STATE_FLAG_PRELIGHT);

  if ((state & BOBGUI_STATE_FLAG_FOCUSED) &&
      item == icon_view->priv->cursor_item)
    flags |= BOBGUI_CELL_RENDERER_FOCUSED;

  if (item->selected)
    {
      state |= BOBGUI_STATE_FLAG_SELECTED;
      flags |= BOBGUI_CELL_RENDERER_SELECTED;
    }

  if (item == priv->last_prelight)
    {
      state |= BOBGUI_STATE_FLAG_PRELIGHT;
      flags |= BOBGUI_CELL_RENDERER_PRELIT;
    }

  bobgui_style_context_set_state (style_context, state);

  bobgui_css_boxes_init_border_box (&boxes,
                                 bobgui_style_context_lookup_style (style_context),
                                 x - priv->item_padding,
                                 y - priv->item_padding,
                                 item->cell_area.width  + priv->item_padding * 2,
                                 item->cell_area.height + priv->item_padding * 2);
  bobgui_css_style_snapshot_background (&boxes, snapshot);
  bobgui_css_style_snapshot_border (&boxes, snapshot);

  cell_area.x      = x;
  cell_area.y      = y;
  cell_area.width  = item->cell_area.width;
  cell_area.height = item->cell_area.height;

  context = g_ptr_array_index (priv->row_contexts, item->row);
  bobgui_cell_area_snapshot (priv->cell_area, context,
                          widget, snapshot, &cell_area, &cell_area, flags,
                          draw_focus);

  bobgui_style_context_restore (style_context);
}

static void
bobgui_icon_view_snapshot_rubberband (BobguiIconView *icon_view,
                                   BobguiSnapshot *snapshot)
{
  BobguiIconViewPrivate *priv = icon_view->priv;
  BobguiStyleContext *context;
  GdkRectangle rect;
  BobguiCssBoxes boxes;

  rect.x = MIN (priv->rubberband_x1, priv->rubberband_x2);
  rect.y = MIN (priv->rubberband_y1, priv->rubberband_y2);
  rect.width = ABS (priv->rubberband_x1 - priv->rubberband_x2) + 1;
  rect.height = ABS (priv->rubberband_y1 - priv->rubberband_y2) + 1;

  context = bobgui_widget_get_style_context (BOBGUI_WIDGET (icon_view));

  bobgui_style_context_save_to_node (context, priv->rubberband_node);

  bobgui_css_boxes_init_border_box (&boxes,
                                 bobgui_style_context_lookup_style (context),
                                 rect.x, rect.y,
                                 rect.width, rect.height);
  bobgui_css_style_snapshot_background (&boxes, snapshot);
  bobgui_css_style_snapshot_border (&boxes, snapshot);

  bobgui_style_context_restore (context);
}

static void
bobgui_icon_view_queue_draw_path (BobguiIconView *icon_view,
			       BobguiTreePath *path)
{
  GList *l;
  int index;

  index = bobgui_tree_path_get_indices (path)[0];

  for (l = icon_view->priv->items; l; l = l->next)
    {
      BobguiIconViewItem *item = l->data;

      if (item->index == index)
	{
	  bobgui_icon_view_queue_draw_item (icon_view, item);
	  break;
	}
    }
}

static void
bobgui_icon_view_queue_draw_item (BobguiIconView     *icon_view,
			       BobguiIconViewItem *item)
{
  bobgui_widget_queue_draw (BOBGUI_WIDGET (icon_view));
}

void
_bobgui_icon_view_set_cursor_item (BobguiIconView     *icon_view,
                                BobguiIconViewItem *item,
                                BobguiCellRenderer *cursor_cell)
{
  /* When hitting this path from keynav, the focus cell is already set,
   * but we still need to queue the draw here (in the case that the focus
   * cell changes but not the cursor item).
   */
  bobgui_icon_view_queue_draw_item (icon_view, item);

  if (icon_view->priv->cursor_item == item &&
      (cursor_cell == NULL || cursor_cell == bobgui_cell_area_get_focus_cell (icon_view->priv->cell_area)))
    return;

  if (icon_view->priv->cursor_item != NULL)
    bobgui_icon_view_queue_draw_item (icon_view, icon_view->priv->cursor_item);

  icon_view->priv->cursor_item = item;

  if (cursor_cell)
    bobgui_cell_area_set_focus_cell (icon_view->priv->cell_area, cursor_cell);
  else
    {
      /* Make sure there is a cell in focus initially */
      if (!bobgui_cell_area_get_focus_cell (icon_view->priv->cell_area))
	bobgui_cell_area_focus (icon_view->priv->cell_area, BOBGUI_DIR_TAB_FORWARD);
    }
}


static BobguiIconViewItem *
bobgui_icon_view_item_new (void)
{
  BobguiIconViewItem *item;

  item = g_slice_new0 (BobguiIconViewItem);

  item->cell_area.width  = -1;
  item->cell_area.height = -1;

  return item;
}

static void
bobgui_icon_view_item_free (BobguiIconViewItem *item)
{
  g_return_if_fail (item != NULL);

  g_slice_free (BobguiIconViewItem, item);
}

BobguiIconViewItem *
_bobgui_icon_view_get_item_at_coords (BobguiIconView          *icon_view,
                                   int                   x,
                                   int                   y,
                                   gboolean              only_in_cell,
                                   BobguiCellRenderer     **cell_at_pos)
{
  GList *items;

  if (cell_at_pos)
    *cell_at_pos = NULL;

  for (items = icon_view->priv->items; items; items = items->next)
    {
      BobguiIconViewItem *item = items->data;
      GdkRectangle    *item_area = &item->cell_area;

      if (x >= item_area->x - icon_view->priv->column_spacing/2 &&
	  x <= item_area->x + item_area->width + icon_view->priv->column_spacing/2 &&
	  y >= item_area->y - icon_view->priv->row_spacing/2 &&
	  y <= item_area->y + item_area->height + icon_view->priv->row_spacing/2)
	{
	  if (only_in_cell || cell_at_pos)
	    {
	      BobguiCellRenderer *cell = NULL;
	      BobguiCellAreaContext *context;

	      context = g_ptr_array_index (icon_view->priv->row_contexts, item->row);
	      _bobgui_icon_view_set_cell_data (icon_view, item);

	      if (x >= item_area->x && x <= item_area->x + item_area->width &&
		  y >= item_area->y && y <= item_area->y + item_area->height)
		cell = bobgui_cell_area_get_cell_at_position (icon_view->priv->cell_area, context,
							   BOBGUI_WIDGET (icon_view),
							   item_area,
							   x, y, NULL);

	      if (cell_at_pos)
		*cell_at_pos = cell;

	      if (only_in_cell)
		return cell != NULL ? item : NULL;
	      else
		return item;
	    }
	  return item;
	}
    }
  return NULL;
}

void
_bobgui_icon_view_select_item (BobguiIconView      *icon_view,
                            BobguiIconViewItem  *item)
{
  g_return_if_fail (BOBGUI_IS_ICON_VIEW (icon_view));
  g_return_if_fail (item != NULL);

  if (item->selected)
    return;

  if (icon_view->priv->selection_mode == BOBGUI_SELECTION_NONE)
    return;
  else if (icon_view->priv->selection_mode != BOBGUI_SELECTION_MULTIPLE)
    bobgui_icon_view_unselect_all_internal (icon_view);

  item->selected = TRUE;

  g_signal_emit (icon_view, icon_view_signals[SELECTION_CHANGED], 0);

  bobgui_icon_view_queue_draw_item (icon_view, item);
}


void
_bobgui_icon_view_unselect_item (BobguiIconView      *icon_view,
                              BobguiIconViewItem  *item)
{
  g_return_if_fail (BOBGUI_IS_ICON_VIEW (icon_view));
  g_return_if_fail (item != NULL);

  if (!item->selected)
    return;

  if (icon_view->priv->selection_mode == BOBGUI_SELECTION_NONE ||
      icon_view->priv->selection_mode == BOBGUI_SELECTION_BROWSE)
    return;

  item->selected = FALSE;

  g_signal_emit (icon_view, icon_view_signals[SELECTION_CHANGED], 0);

  bobgui_icon_view_queue_draw_item (icon_view, item);
}

static void
verify_items (BobguiIconView *icon_view)
{
  GList *items;
  int i = 0;

  for (items = icon_view->priv->items; items; items = items->next)
    {
      BobguiIconViewItem *item = items->data;

      if (item->index != i)
	g_error ("List item does not match its index: "
		 "item index %d and list index %d\n", item->index, i);

      i++;
    }
}

static void
bobgui_icon_view_row_changed (BobguiTreeModel *model,
                           BobguiTreePath  *path,
                           BobguiTreeIter  *iter,
                           gpointer      data)
{
  BobguiIconView *icon_view = BOBGUI_ICON_VIEW (data);

  /* ignore changes in branches */
  if (bobgui_tree_path_get_depth (path) > 1)
    return;

  /* An icon view subclass might add it's own model and populate
   * things at init() time instead of waiting for the constructor()
   * to be called
   */
  if (icon_view->priv->cell_area)
    bobgui_cell_area_stop_editing (icon_view->priv->cell_area, TRUE);

  /* Here we can use a "grow-only" strategy for optimization
   * and only invalidate a single item and queue a relayout
   * instead of invalidating the whole thing.
   *
   * For now BobguiIconView still can't deal with huge models
   * so just invalidate the whole thing when the model
   * changes.
   */
  bobgui_icon_view_invalidate_sizes (icon_view);

  verify_items (icon_view);
}

static void
bobgui_icon_view_row_inserted (BobguiTreeModel *model,
			    BobguiTreePath  *path,
			    BobguiTreeIter  *iter,
			    gpointer      data)
{
  BobguiIconView *icon_view = BOBGUI_ICON_VIEW (data);
  int index;
  BobguiIconViewItem *item;
  GList *list;

  /* ignore changes in branches */
  if (bobgui_tree_path_get_depth (path) > 1)
    return;

  bobgui_tree_model_ref_node (model, iter);

  index = bobgui_tree_path_get_indices(path)[0];

  item = bobgui_icon_view_item_new ();

  item->index = index;

  /* FIXME: We can be more efficient here,
     we can store a tail pointer and use that when
     appending (which is a rather common operation)
  */
  icon_view->priv->items = g_list_insert (icon_view->priv->items,
					 item, index);

  list = g_list_nth (icon_view->priv->items, index + 1);
  for (; list; list = list->next)
    {
      item = list->data;

      item->index++;
    }

  verify_items (icon_view);

  bobgui_widget_queue_resize (BOBGUI_WIDGET (icon_view));
}

static void
bobgui_icon_view_row_deleted (BobguiTreeModel *model,
			   BobguiTreePath  *path,
			   gpointer      data)
{
  BobguiIconView *icon_view = BOBGUI_ICON_VIEW (data);
  int index;
  BobguiIconViewItem *item;
  GList *list, *next;
  gboolean emit = FALSE;
  BobguiTreeIter iter;

  /* ignore changes in branches */
  if (bobgui_tree_path_get_depth (path) > 1)
    return;

  if (bobgui_tree_model_get_iter (model, &iter, path))
    bobgui_tree_model_unref_node (model, &iter);

  index = bobgui_tree_path_get_indices(path)[0];

  list = g_list_nth (icon_view->priv->items, index);
  item = list->data;

  if (icon_view->priv->cell_area)
    bobgui_cell_area_stop_editing (icon_view->priv->cell_area, TRUE);

  if (item == icon_view->priv->anchor_item)
    icon_view->priv->anchor_item = NULL;

  if (item == icon_view->priv->cursor_item)
    icon_view->priv->cursor_item = NULL;

  if (item == icon_view->priv->last_prelight)
    icon_view->priv->last_prelight = NULL;

  if (item->selected)
    emit = TRUE;

  bobgui_icon_view_item_free (item);

  for (next = list->next; next; next = next->next)
    {
      item = next->data;

      item->index--;
    }

  icon_view->priv->items = g_list_delete_link (icon_view->priv->items, list);

  verify_items (icon_view);

  bobgui_widget_queue_resize (BOBGUI_WIDGET (icon_view));

  if (emit)
    g_signal_emit (icon_view, icon_view_signals[SELECTION_CHANGED], 0);
}

static void
bobgui_icon_view_rows_reordered (BobguiTreeModel *model,
			      BobguiTreePath  *parent,
			      BobguiTreeIter  *iter,
			      int          *new_order,
			      gpointer      data)
{
  BobguiIconView *icon_view = BOBGUI_ICON_VIEW (data);
  int i;
  int length;
  GList *items = NULL, *list;
  BobguiIconViewItem **item_array;
  int *order;

  /* ignore changes in branches */
  if (iter != NULL)
    return;

  if (icon_view->priv->cell_area)
    bobgui_cell_area_stop_editing (icon_view->priv->cell_area, TRUE);

  length = bobgui_tree_model_iter_n_children (model, NULL);

  order = g_new (int, length);
  for (i = 0; i < length; i++)
    order [new_order[i]] = i;

  item_array = g_new (BobguiIconViewItem *, length);
  for (i = 0, list = icon_view->priv->items; list != NULL; list = list->next, i++)
    item_array[order[i]] = list->data;
  g_free (order);

  for (i = length - 1; i >= 0; i--)
    {
      item_array[i]->index = i;
      items = g_list_prepend (items, item_array[i]);
    }

  g_free (item_array);
  g_list_free (icon_view->priv->items);
  icon_view->priv->items = items;

  bobgui_widget_queue_resize (BOBGUI_WIDGET (icon_view));

  verify_items (icon_view);
}

static void
bobgui_icon_view_build_items (BobguiIconView *icon_view)
{
  BobguiTreeIter iter;
  int i;
  GList *items = NULL;

  if (!bobgui_tree_model_get_iter_first (icon_view->priv->model,
				      &iter))
    return;

  i = 0;

  do
    {
      BobguiIconViewItem *item = bobgui_icon_view_item_new ();

      item->index = i;

      i++;

      items = g_list_prepend (items, item);

    } while (bobgui_tree_model_iter_next (icon_view->priv->model, &iter));

  icon_view->priv->items = g_list_reverse (items);
}

static void
bobgui_icon_view_add_move_binding (BobguiWidgetClass *widget_class,
				guint           keyval,
				guint           modmask,
				BobguiMovementStep step,
				int             count)
{

  bobgui_widget_class_add_binding_signal (widget_class,
                                       keyval, modmask,
                                       I_("move-cursor"),
                                       "(iibb)", step, count, FALSE, FALSE);

  bobgui_widget_class_add_binding_signal (widget_class,
                                       keyval, GDK_SHIFT_MASK,
                                       "move-cursor",
                                       "(iibb)", step, count, TRUE, FALSE);

  if ((modmask & GDK_CONTROL_MASK) == GDK_CONTROL_MASK)
   return;

  bobgui_widget_class_add_binding_signal (widget_class,
                                       keyval, GDK_CONTROL_MASK | GDK_SHIFT_MASK,
                                       "move-cursor",
                                       "(iibb)", step, count, TRUE, TRUE);

  bobgui_widget_class_add_binding_signal (widget_class,
                                       keyval, GDK_CONTROL_MASK,
                                       "move-cursor",
                                       "(iibb)", step, count, FALSE, TRUE);
}

static gboolean
bobgui_icon_view_real_move_cursor (BobguiIconView     *icon_view,
				BobguiMovementStep  step,
				int              count,
                                gboolean         extend,
                                gboolean         modify)
{
  g_return_val_if_fail (BOBGUI_ICON_VIEW (icon_view), FALSE);
  g_return_val_if_fail (step == BOBGUI_MOVEMENT_LOGICAL_POSITIONS ||
			step == BOBGUI_MOVEMENT_VISUAL_POSITIONS ||
			step == BOBGUI_MOVEMENT_DISPLAY_LINES ||
			step == BOBGUI_MOVEMENT_PAGES ||
			step == BOBGUI_MOVEMENT_BUFFER_ENDS, FALSE);

  if (!bobgui_widget_has_focus (BOBGUI_WIDGET (icon_view)))
    return FALSE;

  bobgui_cell_area_stop_editing (icon_view->priv->cell_area, FALSE);
  bobgui_widget_grab_focus (BOBGUI_WIDGET (icon_view));

  icon_view->priv->extend_selection_pressed = extend;
  icon_view->priv->modify_selection_pressed = modify;

  switch (step)
    {
    case BOBGUI_MOVEMENT_LOGICAL_POSITIONS:
    case BOBGUI_MOVEMENT_VISUAL_POSITIONS:
      bobgui_icon_view_move_cursor_left_right (icon_view, count);
      break;
    case BOBGUI_MOVEMENT_DISPLAY_LINES:
      bobgui_icon_view_move_cursor_up_down (icon_view, count);
      break;
    case BOBGUI_MOVEMENT_PAGES:
      bobgui_icon_view_move_cursor_page_up_down (icon_view, count);
      break;
    case BOBGUI_MOVEMENT_BUFFER_ENDS:
      bobgui_icon_view_move_cursor_start_end (icon_view, count);
      break;
    case BOBGUI_MOVEMENT_WORDS:
    case BOBGUI_MOVEMENT_DISPLAY_LINE_ENDS:
    case BOBGUI_MOVEMENT_PARAGRAPHS:
    case BOBGUI_MOVEMENT_PARAGRAPH_ENDS:
    case BOBGUI_MOVEMENT_HORIZONTAL_PAGES:
    default:
      g_assert_not_reached ();
      break;
    }

  icon_view->priv->modify_selection_pressed = FALSE;
  icon_view->priv->extend_selection_pressed = FALSE;

  icon_view->priv->draw_focus = TRUE;

  return TRUE;
}

static BobguiIconViewItem *
find_item (BobguiIconView     *icon_view,
	   BobguiIconViewItem *current,
	   int              row_ofs,
	   int              col_ofs)
{
  int row, col;
  GList *items;
  BobguiIconViewItem *item;

  /* FIXME: this could be more efficient
   */
  row = current->row + row_ofs;
  col = current->col + col_ofs;

  for (items = icon_view->priv->items; items; items = items->next)
    {
      item = items->data;
      if (item->row == row && item->col == col)
	return item;
    }

  return NULL;
}

static BobguiIconViewItem *
find_item_page_up_down (BobguiIconView     *icon_view,
			BobguiIconViewItem *current,
			int              count)
{
  GList *item, *next;
  int y, col;

  col = current->col;
  y = current->cell_area.y + count * bobgui_adjustment_get_page_size (icon_view->priv->vadjustment);

  item = g_list_find (icon_view->priv->items, current);
  if (count > 0)
    {
      while (item)
	{
	  for (next = item->next; next; next = next->next)
	    {
	      if (((BobguiIconViewItem *)next->data)->col == col)
		break;
	    }
	  if (!next || ((BobguiIconViewItem *)next->data)->cell_area.y > y)
	    break;

	  item = next;
	}
    }
  else
    {
      while (item)
	{
	  for (next = item->prev; next; next = next->prev)
	    {
	      if (((BobguiIconViewItem *)next->data)->col == col)
		break;
	    }
	  if (!next || ((BobguiIconViewItem *)next->data)->cell_area.y < y)
	    break;

	  item = next;
	}
    }

  if (item)
    return item->data;

  return NULL;
}

static gboolean
bobgui_icon_view_select_all_between (BobguiIconView     *icon_view,
				  BobguiIconViewItem *anchor,
				  BobguiIconViewItem *cursor)
{
  GList *items;
  BobguiIconViewItem *item;
  int row1, row2, col1, col2;
  gboolean dirty = FALSE;

  if (anchor->row < cursor->row)
    {
      row1 = anchor->row;
      row2 = cursor->row;
    }
  else
    {
      row1 = cursor->row;
      row2 = anchor->row;
    }

  if (anchor->col < cursor->col)
    {
      col1 = anchor->col;
      col2 = cursor->col;
    }
  else
    {
      col1 = cursor->col;
      col2 = anchor->col;
    }

  for (items = icon_view->priv->items; items; items = items->next)
    {
      item = items->data;

      if (row1 <= item->row && item->row <= row2 &&
	  col1 <= item->col && item->col <= col2)
	{
	  if (!item->selected)
	    {
	      dirty = TRUE;
	      item->selected = TRUE;
	    }
	  bobgui_icon_view_queue_draw_item (icon_view, item);
	}
    }

  return dirty;
}

static void
bobgui_icon_view_move_cursor_up_down (BobguiIconView *icon_view,
				   int          count)
{
  BobguiIconViewItem *item;
  BobguiCellRenderer *cell = NULL;
  gboolean dirty = FALSE;
  int step;
  BobguiDirectionType direction;

  if (!bobgui_widget_has_focus (BOBGUI_WIDGET (icon_view)))
    return;

  direction = count < 0 ? BOBGUI_DIR_UP : BOBGUI_DIR_DOWN;

  if (!icon_view->priv->cursor_item)
    {
      GList *list;

      if (count > 0)
	list = icon_view->priv->items;
      else
	list = g_list_last (icon_view->priv->items);

      if (list)
        {
          item = list->data;

          /* Give focus to the first cell initially */
          _bobgui_icon_view_set_cell_data (icon_view, item);
          bobgui_cell_area_focus (icon_view->priv->cell_area, direction);
        }
      else
        {
          item = NULL;
        }
    }
  else
    {
      item = icon_view->priv->cursor_item;
      step = count > 0 ? 1 : -1;

      /* Save the current focus cell in case we hit the edge */
      cell = bobgui_cell_area_get_focus_cell (icon_view->priv->cell_area);

      while (item)
	{
	  _bobgui_icon_view_set_cell_data (icon_view, item);

	  if (bobgui_cell_area_focus (icon_view->priv->cell_area, direction))
	    break;

	  item = find_item (icon_view, item, step, 0);
	}
    }

  if (!item)
    {
      if (!bobgui_widget_keynav_failed (BOBGUI_WIDGET (icon_view), direction))
        {
          BobguiWidget *toplevel = BOBGUI_WIDGET (bobgui_widget_get_root (BOBGUI_WIDGET (icon_view)));
          if (toplevel)
            bobgui_widget_child_focus (toplevel,
                                    direction == BOBGUI_DIR_UP ?
                                    BOBGUI_DIR_TAB_BACKWARD :
                                    BOBGUI_DIR_TAB_FORWARD);

        }

      bobgui_cell_area_set_focus_cell (icon_view->priv->cell_area, cell);
      return;
    }

  if (icon_view->priv->modify_selection_pressed ||
      !icon_view->priv->extend_selection_pressed ||
      !icon_view->priv->anchor_item ||
      icon_view->priv->selection_mode != BOBGUI_SELECTION_MULTIPLE)
    icon_view->priv->anchor_item = item;

  cell = bobgui_cell_area_get_focus_cell (icon_view->priv->cell_area);
  _bobgui_icon_view_set_cursor_item (icon_view, item, cell);

  if (!icon_view->priv->modify_selection_pressed &&
      icon_view->priv->selection_mode != BOBGUI_SELECTION_NONE)
    {
      dirty = bobgui_icon_view_unselect_all_internal (icon_view);
      dirty = bobgui_icon_view_select_all_between (icon_view,
						icon_view->priv->anchor_item,
						item) || dirty;
    }

  bobgui_icon_view_scroll_to_item (icon_view, item);

  if (dirty)
    g_signal_emit (icon_view, icon_view_signals[SELECTION_CHANGED], 0);
}

static void
bobgui_icon_view_move_cursor_page_up_down (BobguiIconView *icon_view,
					int          count)
{
  BobguiIconViewItem *item;
  gboolean dirty = FALSE;

  if (!bobgui_widget_has_focus (BOBGUI_WIDGET (icon_view)))
    return;

  if (!icon_view->priv->cursor_item)
    {
      GList *list;

      if (count > 0)
	list = icon_view->priv->items;
      else
	list = g_list_last (icon_view->priv->items);

      item = list ? list->data : NULL;
    }
  else
    item = find_item_page_up_down (icon_view,
				   icon_view->priv->cursor_item,
				   count);

  if (item == icon_view->priv->cursor_item)
    bobgui_widget_error_bell (BOBGUI_WIDGET (icon_view));

  if (!item)
    return;

  if (icon_view->priv->modify_selection_pressed ||
      !icon_view->priv->extend_selection_pressed ||
      !icon_view->priv->anchor_item ||
      icon_view->priv->selection_mode != BOBGUI_SELECTION_MULTIPLE)
    icon_view->priv->anchor_item = item;

  _bobgui_icon_view_set_cursor_item (icon_view, item, NULL);

  if (!icon_view->priv->modify_selection_pressed &&
      icon_view->priv->selection_mode != BOBGUI_SELECTION_NONE)
    {
      dirty = bobgui_icon_view_unselect_all_internal (icon_view);
      dirty = bobgui_icon_view_select_all_between (icon_view,
						icon_view->priv->anchor_item,
						item) || dirty;
    }

  bobgui_icon_view_scroll_to_item (icon_view, item);

  if (dirty)
    g_signal_emit (icon_view, icon_view_signals[SELECTION_CHANGED], 0);
}

static void
bobgui_icon_view_move_cursor_left_right (BobguiIconView *icon_view,
				      int          count)
{
  BobguiIconViewItem *item;
  BobguiCellRenderer *cell = NULL;
  gboolean dirty = FALSE;
  int step;
  BobguiDirectionType direction;

  if (!bobgui_widget_has_focus (BOBGUI_WIDGET (icon_view)))
    return;

  direction = count < 0 ? BOBGUI_DIR_LEFT : BOBGUI_DIR_RIGHT;

  if (!icon_view->priv->cursor_item)
    {
      GList *list;

      if (count > 0)
	list = icon_view->priv->items;
      else
	list = g_list_last (icon_view->priv->items);

      if (list)
        {
          item = list->data;

          /* Give focus to the first cell initially */
          _bobgui_icon_view_set_cell_data (icon_view, item);
          bobgui_cell_area_focus (icon_view->priv->cell_area, direction);
        }
      else
        {
          item = NULL;
        }
    }
  else
    {
      item = icon_view->priv->cursor_item;
      step = count > 0 ? 1 : -1;

      /* Save the current focus cell in case we hit the edge */
      cell = bobgui_cell_area_get_focus_cell (icon_view->priv->cell_area);

      while (item)
	{
	  _bobgui_icon_view_set_cell_data (icon_view, item);

	  if (bobgui_cell_area_focus (icon_view->priv->cell_area, direction))
	    break;

	  item = find_item (icon_view, item, 0, step);
	}
    }

  if (!item)
    {
      if (!bobgui_widget_keynav_failed (BOBGUI_WIDGET (icon_view), direction))
        {
          BobguiWidget *toplevel = BOBGUI_WIDGET (bobgui_widget_get_root (BOBGUI_WIDGET (icon_view)));
          if (toplevel)
            bobgui_widget_child_focus (toplevel,
                                    direction == BOBGUI_DIR_LEFT ?
                                    BOBGUI_DIR_TAB_BACKWARD :
                                    BOBGUI_DIR_TAB_FORWARD);

        }

      bobgui_cell_area_set_focus_cell (icon_view->priv->cell_area, cell);
      return;
    }

  if (icon_view->priv->modify_selection_pressed ||
      !icon_view->priv->extend_selection_pressed ||
      !icon_view->priv->anchor_item ||
      icon_view->priv->selection_mode != BOBGUI_SELECTION_MULTIPLE)
    icon_view->priv->anchor_item = item;

  cell = bobgui_cell_area_get_focus_cell (icon_view->priv->cell_area);
  _bobgui_icon_view_set_cursor_item (icon_view, item, cell);

  if (!icon_view->priv->modify_selection_pressed &&
      icon_view->priv->selection_mode != BOBGUI_SELECTION_NONE)
    {
      dirty = bobgui_icon_view_unselect_all_internal (icon_view);
      dirty = bobgui_icon_view_select_all_between (icon_view,
						icon_view->priv->anchor_item,
						item) || dirty;
    }

  bobgui_icon_view_scroll_to_item (icon_view, item);

  if (dirty)
    g_signal_emit (icon_view, icon_view_signals[SELECTION_CHANGED], 0);
}

static void
bobgui_icon_view_move_cursor_start_end (BobguiIconView *icon_view,
				     int          count)
{
  BobguiIconViewItem *item;
  GList *list;
  gboolean dirty = FALSE;

  if (!bobgui_widget_has_focus (BOBGUI_WIDGET (icon_view)))
    return;

  if (count < 0)
    list = icon_view->priv->items;
  else
    list = g_list_last (icon_view->priv->items);

  item = list ? list->data : NULL;

  if (item == icon_view->priv->cursor_item)
    bobgui_widget_error_bell (BOBGUI_WIDGET (icon_view));

  if (!item)
    return;

  if (icon_view->priv->modify_selection_pressed ||
      !icon_view->priv->extend_selection_pressed ||
      !icon_view->priv->anchor_item ||
      icon_view->priv->selection_mode != BOBGUI_SELECTION_MULTIPLE)
    icon_view->priv->anchor_item = item;

  _bobgui_icon_view_set_cursor_item (icon_view, item, NULL);

  if (!icon_view->priv->modify_selection_pressed &&
      icon_view->priv->selection_mode != BOBGUI_SELECTION_NONE)
    {
      dirty = bobgui_icon_view_unselect_all_internal (icon_view);
      dirty = bobgui_icon_view_select_all_between (icon_view,
						icon_view->priv->anchor_item,
						item) || dirty;
    }

  bobgui_icon_view_scroll_to_item (icon_view, item);

  if (dirty)
    g_signal_emit (icon_view, icon_view_signals[SELECTION_CHANGED], 0);
}

/**
 * bobgui_icon_view_scroll_to_path:
 * @icon_view: A `BobguiIconView`
 * @path: The path of the item to move to.
 * @use_align: whether to use alignment arguments, or %FALSE.
 * @row_align: The vertical alignment of the item specified by @path.
 * @col_align: The horizontal alignment of the item specified by @path.
 *
 * Moves the alignments of @icon_view to the position specified by @path.
 * @row_align determines where the row is placed, and @col_align determines
 * where @column is placed.  Both are expected to be between 0.0 and 1.0.
 * 0.0 means left/top alignment, 1.0 means right/bottom alignment, 0.5 means
 * center.
 *
 * If @use_align is %FALSE, then the alignment arguments are ignored, and the
 * tree does the minimum amount of work to scroll the item onto the screen.
 * This means that the item will be scrolled to the edge closest to its current
 * position.  If the item is currently visible on the screen, nothing is done.
 *
 * This function only works if the model is set, and @path is a valid row on
 * the model. If the model changes before the @icon_view is realized, the
 * centered path will be modified to reflect this change.
 *
 * Deprecated: 4.10: Use [class@Bobgui.GridView] instead
 **/
void
bobgui_icon_view_scroll_to_path (BobguiIconView *icon_view,
			      BobguiTreePath *path,
			      gboolean     use_align,
			      float        row_align,
			      float        col_align)
{
  BobguiIconViewItem *item = NULL;
  BobguiWidget *widget;

  g_return_if_fail (BOBGUI_IS_ICON_VIEW (icon_view));
  g_return_if_fail (path != NULL);
  g_return_if_fail (row_align >= 0.0 && row_align <= 1.0);
  g_return_if_fail (col_align >= 0.0 && col_align <= 1.0);

  widget = BOBGUI_WIDGET (icon_view);

  if (bobgui_tree_path_get_depth (path) > 0)
    item = g_list_nth_data (icon_view->priv->items,
			    bobgui_tree_path_get_indices(path)[0]);

  if (!item || item->cell_area.width < 0 ||
      !bobgui_widget_get_realized (widget))
    {
      if (icon_view->priv->scroll_to_path)
	bobgui_tree_row_reference_free (icon_view->priv->scroll_to_path);

      icon_view->priv->scroll_to_path = NULL;

      if (path)
	icon_view->priv->scroll_to_path = bobgui_tree_row_reference_new_proxy (G_OBJECT (icon_view), icon_view->priv->model, path);

      icon_view->priv->scroll_to_use_align = use_align;
      icon_view->priv->scroll_to_row_align = row_align;
      icon_view->priv->scroll_to_col_align = col_align;

      return;
    }

  if (use_align)
    {
      int width, height;
      int x, y;
      float offset;
      GdkRectangle item_area =
	{
	  item->cell_area.x - icon_view->priv->item_padding,
	  item->cell_area.y - icon_view->priv->item_padding,
	  item->cell_area.width  + icon_view->priv->item_padding * 2,
	  item->cell_area.height + icon_view->priv->item_padding * 2
	};

      x =0;
      y =0;

      width = bobgui_widget_get_width (widget);
      height = bobgui_widget_get_height (widget);

      offset = y + item_area.y - row_align * (height - item_area.height);

      bobgui_adjustment_set_value (icon_view->priv->vadjustment,
                                bobgui_adjustment_get_value (icon_view->priv->vadjustment) + offset);

      offset = x + item_area.x - col_align * (width - item_area.width);

      bobgui_adjustment_set_value (icon_view->priv->hadjustment,
                                bobgui_adjustment_get_value (icon_view->priv->hadjustment) + offset);
    }
  else
    bobgui_icon_view_scroll_to_item (icon_view, item);
}


static void
bobgui_icon_view_scroll_to_item (BobguiIconView     *icon_view,
                              BobguiIconViewItem *item)
{
  BobguiIconViewPrivate *priv = icon_view->priv;
  BobguiWidget *widget = BOBGUI_WIDGET (icon_view);
  BobguiAdjustment *hadj, *vadj;
  int widget_width, widget_height;
  int x, y;
  GdkRectangle item_area;

  item_area.x = item->cell_area.x - priv->item_padding;
  item_area.y = item->cell_area.y - priv->item_padding;
  item_area.width = item->cell_area.width  + priv->item_padding * 2;
  item_area.height = item->cell_area.height + priv->item_padding * 2;

  widget_width = bobgui_widget_get_width (widget);
  widget_height = bobgui_widget_get_height (widget);

  hadj = icon_view->priv->hadjustment;
  vadj = icon_view->priv->vadjustment;

  x = - bobgui_adjustment_get_value (hadj);
  y = - bobgui_adjustment_get_value (vadj);

  if (y + item_area.y < 0)
    bobgui_adjustment_animate_to_value (vadj,
                                     bobgui_adjustment_get_value (vadj)
                                     + y + item_area.y);
  else if (y + item_area.y + item_area.height > widget_height)
    bobgui_adjustment_animate_to_value (vadj,
                                     bobgui_adjustment_get_value (vadj)
                                     + y + item_area.y + item_area.height - widget_height);

  if (x + item_area.x < 0)
    bobgui_adjustment_animate_to_value (hadj,
                                     bobgui_adjustment_get_value (hadj)
                                     + x + item_area.x);
  else if (x + item_area.x + item_area.width > widget_width)
    bobgui_adjustment_animate_to_value (hadj,
                                     bobgui_adjustment_get_value (hadj)
                                     + x + item_area.x + item_area.width - widget_width);
}

/* BobguiCellLayout implementation */

static void
bobgui_icon_view_ensure_cell_area (BobguiIconView *icon_view,
                                BobguiCellArea *cell_area)
{
  BobguiIconViewPrivate *priv = icon_view->priv;

  if (priv->cell_area)
    return;

  if (cell_area)
    priv->cell_area = cell_area;
  else
    priv->cell_area = bobgui_cell_area_box_new ();

  g_object_ref_sink (priv->cell_area);

  if (BOBGUI_IS_ORIENTABLE (priv->cell_area))
    bobgui_orientable_set_orientation (BOBGUI_ORIENTABLE (priv->cell_area), priv->item_orientation);

  priv->cell_area_context = bobgui_cell_area_create_context (priv->cell_area);

  priv->add_editable_id =
    g_signal_connect (priv->cell_area, "add-editable",
                      G_CALLBACK (bobgui_icon_view_add_editable), icon_view);
  priv->remove_editable_id =
    g_signal_connect (priv->cell_area, "remove-editable",
                      G_CALLBACK (bobgui_icon_view_remove_editable), icon_view);

  update_text_cell (icon_view);
  update_pixbuf_cell (icon_view);
}

static BobguiCellArea *
bobgui_icon_view_cell_layout_get_area (BobguiCellLayout *cell_layout)
{
  BobguiIconView *icon_view = BOBGUI_ICON_VIEW (cell_layout);
  BobguiIconViewPrivate *priv = icon_view->priv;

  if (G_UNLIKELY (!priv->cell_area))
    bobgui_icon_view_ensure_cell_area (icon_view, NULL);

  return icon_view->priv->cell_area;
}

void
_bobgui_icon_view_set_cell_data (BobguiIconView     *icon_view,
			      BobguiIconViewItem *item)
{
  BobguiTreeIter iter;
  BobguiTreePath *path;

  path = bobgui_tree_path_new_from_indices (item->index, -1);
  if (!bobgui_tree_model_get_iter (icon_view->priv->model, &iter, path))
    return;
  bobgui_tree_path_free (path);

  bobgui_cell_area_apply_attributes (icon_view->priv->cell_area,
				  icon_view->priv->model,
				  &iter, FALSE, FALSE);
}



/* Public API */


/**
 * bobgui_icon_view_new:
 *
 * Creates a new `BobguiIconView` widget
 *
 * Returns: A newly created `BobguiIconView` widget
 *
 * Deprecated: 4.10: Use [class@Bobgui.GridView] instead
 **/
BobguiWidget *
bobgui_icon_view_new (void)
{
  return g_object_new (BOBGUI_TYPE_ICON_VIEW, NULL);
}

/**
 * bobgui_icon_view_new_with_area:
 * @area: the `BobguiCellArea` to use to layout cells
 *
 * Creates a new `BobguiIconView` widget using the
 * specified @area to layout cells inside the icons.
 *
 * Returns: A newly created `BobguiIconView` widget
 *
 * Deprecated: 4.10: Use [class@Bobgui.GridView] instead
 **/
BobguiWidget *
bobgui_icon_view_new_with_area (BobguiCellArea *area)
{
  return g_object_new (BOBGUI_TYPE_ICON_VIEW, "cell-area", area, NULL);
}

/**
 * bobgui_icon_view_new_with_model:
 * @model: The model.
 *
 * Creates a new `BobguiIconView` widget with the model @model.
 *
 * Returns: A newly created `BobguiIconView` widget.
 *
 * Deprecated: 4.10: Use [class@Bobgui.GridView] instead
 **/
BobguiWidget *
bobgui_icon_view_new_with_model (BobguiTreeModel *model)
{
  return g_object_new (BOBGUI_TYPE_ICON_VIEW, "model", model, NULL);
}

/**
 * bobgui_icon_view_get_path_at_pos:
 * @icon_view: A `BobguiIconView`.
 * @x: The x position to be identified
 * @y: The y position to be identified
 *
 * Gets the path for the icon at the given position.
 *
 * Returns: (nullable) (transfer full): The `BobguiTreePath` corresponding
 * to the icon or %NULL if no icon exists at that position.
 *
 * Deprecated: 4.10: Use [class@Bobgui.GridView] instead
 **/
BobguiTreePath *
bobgui_icon_view_get_path_at_pos (BobguiIconView *icon_view,
			       int          x,
			       int          y)
{
  BobguiIconViewItem *item;
  BobguiTreePath *path;

  g_return_val_if_fail (BOBGUI_IS_ICON_VIEW (icon_view), NULL);

  item = _bobgui_icon_view_get_item_at_coords (icon_view, x, y, TRUE, NULL);

  if (!item)
    return NULL;

  path = bobgui_tree_path_new_from_indices (item->index, -1);

  return path;
}

/**
 * bobgui_icon_view_get_item_at_pos:
 * @icon_view: A `BobguiIconView`.
 * @x: The x position to be identified
 * @y: The y position to be identified
 * @path: (out) (optional): Return location for the path
 * @cell: (out) (optional) (transfer none): Return location for the renderer
 *   responsible for the cell at (@x, @y)
 *
 * Gets the path and cell for the icon at the given position.
 *
 * Returns: %TRUE if an item exists at the specified position
 *
 * Deprecated: 4.10: Use [class@Bobgui.GridView] instead
 **/
gboolean
bobgui_icon_view_get_item_at_pos (BobguiIconView      *icon_view,
			       int               x,
			       int               y,
			       BobguiTreePath     **path,
			       BobguiCellRenderer **cell)
{
  BobguiIconViewItem *item;
  BobguiCellRenderer *renderer = NULL;

  g_return_val_if_fail (BOBGUI_IS_ICON_VIEW (icon_view), FALSE);

  item = _bobgui_icon_view_get_item_at_coords (icon_view, x, y, TRUE, &renderer);

  if (path != NULL)
    {
      if (item != NULL)
	*path = bobgui_tree_path_new_from_indices (item->index, -1);
      else
	*path = NULL;
    }

  if (cell != NULL)
    *cell = renderer;

  return (item != NULL);
}

/**
 * bobgui_icon_view_get_cell_rect:
 * @icon_view: a `BobguiIconView`
 * @path: a `BobguiTreePath`
 * @cell: (nullable): a `BobguiCellRenderer`
 * @rect: (out): rectangle to fill with cell rect
 *
 * Fills the bounding rectangle in widget coordinates for the cell specified by
 * @path and @cell. If @cell is %NULL the main cell area is used.
 *
 * This function is only valid if @icon_view is realized.
 *
 * Returns: %FALSE if there is no such item, %TRUE otherwise
 *
 * Deprecated: 4.10: Use [class@Bobgui.GridView] instead
 */
gboolean
bobgui_icon_view_get_cell_rect (BobguiIconView     *icon_view,
                             BobguiTreePath     *path,
                             BobguiCellRenderer *cell,
                             GdkRectangle    *rect)
{
  BobguiIconViewItem *item = NULL;

  g_return_val_if_fail (BOBGUI_IS_ICON_VIEW (icon_view), FALSE);
  g_return_val_if_fail (cell == NULL || BOBGUI_IS_CELL_RENDERER (cell), FALSE);

  if (bobgui_tree_path_get_depth (path) > 0)
    item = g_list_nth_data (icon_view->priv->items,
			    bobgui_tree_path_get_indices(path)[0]);

  if (!item)
    return FALSE;

  if (cell)
    {
      BobguiCellAreaContext *context;

      context = g_ptr_array_index (icon_view->priv->row_contexts, item->row);
      _bobgui_icon_view_set_cell_data (icon_view, item);
      bobgui_cell_area_get_cell_allocation (icon_view->priv->cell_area, context,
					 BOBGUI_WIDGET (icon_view),
					 cell, &item->cell_area, rect);
    }
  else
    {
      rect->x = item->cell_area.x - icon_view->priv->item_padding;
      rect->y = item->cell_area.y - icon_view->priv->item_padding;
      rect->width  = item->cell_area.width  + icon_view->priv->item_padding * 2;
      rect->height = item->cell_area.height + icon_view->priv->item_padding * 2;
    }

  return TRUE;
}

/**
 * bobgui_icon_view_set_tooltip_item:
 * @icon_view: a `BobguiIconView`
 * @tooltip: a `BobguiTooltip`
 * @path: a `BobguiTreePath`
 *
 * Sets the tip area of @tooltip to be the area covered by the item at @path.
 * See also bobgui_icon_view_set_tooltip_column() for a simpler alternative.
 * See also bobgui_tooltip_set_tip_area().
 *
 * Deprecated: 4.10: Use [class@Bobgui.GridView] instead
 */
void
bobgui_icon_view_set_tooltip_item (BobguiIconView     *icon_view,
                                BobguiTooltip      *tooltip,
                                BobguiTreePath     *path)
{
  g_return_if_fail (BOBGUI_IS_ICON_VIEW (icon_view));
  g_return_if_fail (BOBGUI_IS_TOOLTIP (tooltip));

  bobgui_icon_view_set_tooltip_cell (icon_view, tooltip, path, NULL);
}

/**
 * bobgui_icon_view_set_tooltip_cell:
 * @icon_view: a `BobguiIconView`
 * @tooltip: a `BobguiTooltip`
 * @path: a `BobguiTreePath`
 * @cell: (nullable): a `BobguiCellRenderer`
 *
 * Sets the tip area of @tooltip to the area which @cell occupies in
 * the item pointed to by @path. See also bobgui_tooltip_set_tip_area().
 *
 * See also bobgui_icon_view_set_tooltip_column() for a simpler alternative.
 *
 * Deprecated: 4.10: Use [class@Bobgui.GridView] instead
 */
void
bobgui_icon_view_set_tooltip_cell (BobguiIconView     *icon_view,
                                BobguiTooltip      *tooltip,
                                BobguiTreePath     *path,
                                BobguiCellRenderer *cell)
{
  GdkRectangle rect;

  g_return_if_fail (BOBGUI_IS_ICON_VIEW (icon_view));
  g_return_if_fail (BOBGUI_IS_TOOLTIP (tooltip));
  g_return_if_fail (cell == NULL || BOBGUI_IS_CELL_RENDERER (cell));

  if (!bobgui_icon_view_get_cell_rect (icon_view, path, cell, &rect))
    return;

  bobgui_tooltip_set_tip_area (tooltip, &rect);
}


/**
 * bobgui_icon_view_get_tooltip_context:
 * @icon_view: an `BobguiIconView`
 * @x: the x coordinate (relative to widget coordinates)
 * @y: the y coordinate (relative to widget coordinates)
 * @keyboard_tip: whether this is a keyboard tooltip or not
 * @model: (out) (optional) (transfer none): a pointer to receive a `BobguiTreeModel`
 * @path: (out) (optional): a pointer to receive a `BobguiTreePath`
 * @iter: (out) (optional): a pointer to receive a `BobguiTreeIter`
 *
 * This function is supposed to be used in a `BobguiWidget::query-tooltip`
 * signal handler for `BobguiIconView`. The @x, @y and @keyboard_tip values
 * which are received in the signal handler, should be passed to this
 * function without modification.
 *
 * The return value indicates whether there is an icon view item at the given
 * coordinates (%TRUE) or not (%FALSE) for mouse tooltips. For keyboard
 * tooltips the item returned will be the cursor item. When %TRUE, then any of
 * @model, @path and @iter which have been provided will be set to point to
 * that row and the corresponding model.
 *
 * Returns: whether or not the given tooltip context points to an item
 *
 * Deprecated: 4.10: Use [class@Bobgui.GridView] instead
 */
gboolean
bobgui_icon_view_get_tooltip_context (BobguiIconView   *icon_view,
                                   int            x,
                                   int            y,
                                   gboolean       keyboard_tip,
                                   BobguiTreeModel **model,
                                   BobguiTreePath  **path,
                                   BobguiTreeIter   *iter)
{
  BobguiTreePath *tmppath = NULL;

  g_return_val_if_fail (BOBGUI_IS_ICON_VIEW (icon_view), FALSE);

  if (keyboard_tip)
    {
      bobgui_icon_view_get_cursor (icon_view, &tmppath, NULL);

      if (!tmppath)
        return FALSE;
    }
  else
    {
      if (!bobgui_icon_view_get_item_at_pos (icon_view, x, y, &tmppath, NULL))
        return FALSE;
    }

  if (model)
    *model = bobgui_icon_view_get_model (icon_view);

  if (iter)
    bobgui_tree_model_get_iter (bobgui_icon_view_get_model (icon_view),
                             iter, tmppath);

  if (path)
    *path = tmppath;
  else
    bobgui_tree_path_free (tmppath);

  return TRUE;
}

static gboolean
bobgui_icon_view_set_tooltip_query_cb (BobguiWidget  *widget,
                                    int         x,
                                    int         y,
                                    gboolean    keyboard_tip,
                                    BobguiTooltip *tooltip,
                                    gpointer    data)
{
  char *str;
  BobguiTreeIter iter;
  BobguiTreePath *path;
  BobguiTreeModel *model;
  BobguiIconView *icon_view = BOBGUI_ICON_VIEW (widget);

  if (!bobgui_icon_view_get_tooltip_context (BOBGUI_ICON_VIEW (widget),
                                          x, y,
                                          keyboard_tip,
                                          &model, &path, &iter))
    return FALSE;

  bobgui_tree_model_get (model, &iter, icon_view->priv->tooltip_column, &str, -1);

  if (!str)
    {
      bobgui_tree_path_free (path);
      return FALSE;
    }

  bobgui_tooltip_set_markup (tooltip, str);
  bobgui_icon_view_set_tooltip_item (icon_view, tooltip, path);

  bobgui_tree_path_free (path);
  g_free (str);

  return TRUE;
}


/**
 * bobgui_icon_view_set_tooltip_column:
 * @icon_view: a `BobguiIconView`
 * @column: an integer, which is a valid column number for @icon_view’s model
 *
 * If you only plan to have simple (text-only) tooltips on full items, you
 * can use this function to have `BobguiIconView` handle these automatically
 * for you. @column should be set to the column in @icon_view’s model
 * containing the tooltip texts, or -1 to disable this feature.
 *
 * When enabled, `BobguiWidget:has-tooltip` will be set to %TRUE and
 * @icon_view will connect a `BobguiWidget::query-tooltip` signal handler.
 *
 * Note that the signal handler sets the text with bobgui_tooltip_set_markup(),
 * so &, <, etc have to be escaped in the text.
 *
 * Deprecated: 4.10: Use [class@Bobgui.GridView] instead
 */
void
bobgui_icon_view_set_tooltip_column (BobguiIconView *icon_view,
                                  int          column)
{
  g_return_if_fail (BOBGUI_IS_ICON_VIEW (icon_view));

  if (column == icon_view->priv->tooltip_column)
    return;

  if (column == -1)
    {
      g_signal_handlers_disconnect_by_func (icon_view,
                                            bobgui_icon_view_set_tooltip_query_cb,
                                            NULL);
      bobgui_widget_set_has_tooltip (BOBGUI_WIDGET (icon_view), FALSE);
    }
  else
    {
      if (icon_view->priv->tooltip_column == -1)
        {
          g_signal_connect (icon_view, "query-tooltip",
                            G_CALLBACK (bobgui_icon_view_set_tooltip_query_cb), NULL);
          bobgui_widget_set_has_tooltip (BOBGUI_WIDGET (icon_view), TRUE);
        }
    }

  icon_view->priv->tooltip_column = column;
  g_object_notify (G_OBJECT (icon_view), "tooltip-column");
}

/**
 * bobgui_icon_view_get_tooltip_column:
 * @icon_view: a `BobguiIconView`
 *
 * Returns the column of @icon_view’s model which is being used for
 * displaying tooltips on @icon_view’s rows.
 *
 * Returns: the index of the tooltip column that is currently being
 * used, or -1 if this is disabled.
 *
 * Deprecated: 4.10: Use [class@Bobgui.GridView] instead
 */
int
bobgui_icon_view_get_tooltip_column (BobguiIconView *icon_view)
{
  g_return_val_if_fail (BOBGUI_IS_ICON_VIEW (icon_view), 0);

  return icon_view->priv->tooltip_column;
}

/**
 * bobgui_icon_view_get_visible_range:
 * @icon_view: A `BobguiIconView`
 * @start_path: (out) (optional): Return location for start of region
 * @end_path: (out) (optional): Return location for end of region
 *
 * Sets @start_path and @end_path to be the first and last visible path.
 * Note that there may be invisible paths in between.
 *
 * Both paths should be freed with bobgui_tree_path_free() after use.
 *
 * Returns: %TRUE, if valid paths were placed in @start_path and @end_path
 *
 * Deprecated: 4.10: Use [class@Bobgui.GridView] instead
 */
gboolean
bobgui_icon_view_get_visible_range (BobguiIconView  *icon_view,
				 BobguiTreePath **start_path,
				 BobguiTreePath **end_path)
{
  int start_index = -1;
  int end_index = -1;
  GList *icons;

  g_return_val_if_fail (BOBGUI_IS_ICON_VIEW (icon_view), FALSE);

  if (icon_view->priv->hadjustment == NULL ||
      icon_view->priv->vadjustment == NULL)
    return FALSE;

  if (start_path == NULL && end_path == NULL)
    return FALSE;

  for (icons = icon_view->priv->items; icons; icons = icons->next)
    {
      BobguiIconViewItem *item = icons->data;
      GdkRectangle    *item_area = &item->cell_area;

      if ((item_area->x + item_area->width >= (int)bobgui_adjustment_get_value (icon_view->priv->hadjustment)) &&
	  (item_area->y + item_area->height >= (int)bobgui_adjustment_get_value (icon_view->priv->vadjustment)) &&
	  (item_area->x <=
	   (int) (bobgui_adjustment_get_value (icon_view->priv->hadjustment) +
		  bobgui_adjustment_get_page_size (icon_view->priv->hadjustment))) &&
	  (item_area->y <=
	   (int) (bobgui_adjustment_get_value (icon_view->priv->vadjustment) +
		  bobgui_adjustment_get_page_size (icon_view->priv->vadjustment))))
	{
	  if (start_index == -1)
	    start_index = item->index;
	  end_index = item->index;
	}
    }

  if (start_path && start_index != -1)
    *start_path = bobgui_tree_path_new_from_indices (start_index, -1);
  if (end_path && end_index != -1)
    *end_path = bobgui_tree_path_new_from_indices (end_index, -1);

  return start_index != -1;
}

/**
 * bobgui_icon_view_selected_foreach:
 * @icon_view: A `BobguiIconView`.
 * @func: (scope call): The function to call for each selected icon.
 * @data: User data to pass to the function.
 *
 * Calls a function for each selected icon. Note that the model or
 * selection cannot be modified from within this function.
 *
 * Deprecated: 4.10: Use [class@Bobgui.GridView] instead
 **/
void
bobgui_icon_view_selected_foreach (BobguiIconView           *icon_view,
				BobguiIconViewForeachFunc func,
				gpointer               data)
{
  GList *list;

  for (list = icon_view->priv->items; list; list = list->next)
    {
      BobguiIconViewItem *item = list->data;
      BobguiTreePath *path = bobgui_tree_path_new_from_indices (item->index, -1);

      if (item->selected)
	(* func) (icon_view, path, data);

      bobgui_tree_path_free (path);
    }
}

/**
 * bobgui_icon_view_set_selection_mode:
 * @icon_view: A `BobguiIconView`.
 * @mode: The selection mode
 *
 * Sets the selection mode of the @icon_view.
 *
 * Deprecated: 4.10: Use [class@Bobgui.GridView] instead
 **/
void
bobgui_icon_view_set_selection_mode (BobguiIconView      *icon_view,
				  BobguiSelectionMode  mode)
{
  g_return_if_fail (BOBGUI_IS_ICON_VIEW (icon_view));

  if (mode == icon_view->priv->selection_mode)
    return;

  if (mode == BOBGUI_SELECTION_NONE ||
      icon_view->priv->selection_mode == BOBGUI_SELECTION_MULTIPLE)
    bobgui_icon_view_unselect_all (icon_view);

  icon_view->priv->selection_mode = mode;

  g_object_notify (G_OBJECT (icon_view), "selection-mode");
}

/**
 * bobgui_icon_view_get_selection_mode:
 * @icon_view: A `BobguiIconView`.
 *
 * Gets the selection mode of the @icon_view.
 *
 * Returns: the current selection mode
 *
 * Deprecated: 4.10: Use [class@Bobgui.GridView] instead
 **/
BobguiSelectionMode
bobgui_icon_view_get_selection_mode (BobguiIconView *icon_view)
{
  g_return_val_if_fail (BOBGUI_IS_ICON_VIEW (icon_view), BOBGUI_SELECTION_SINGLE);

  return icon_view->priv->selection_mode;
}

/**
 * bobgui_icon_view_set_model:
 * @icon_view: A `BobguiIconView`.
 * @model: (nullable): The model.
 *
 * Sets the model for a `BobguiIconView`.
 * If the @icon_view already has a model set, it will remove
 * it before setting the new model.  If @model is %NULL, then
 * it will unset the old model.
 *
 * Deprecated: 4.10: Use [class@Bobgui.GridView] instead
 **/
void
bobgui_icon_view_set_model (BobguiIconView *icon_view,
			 BobguiTreeModel *model)
{
  gboolean dirty;

  g_return_if_fail (BOBGUI_IS_ICON_VIEW (icon_view));
  g_return_if_fail (model == NULL || BOBGUI_IS_TREE_MODEL (model));

  if (icon_view->priv->model == model)
    return;

  if (icon_view->priv->scroll_to_path)
    {
      bobgui_tree_row_reference_free (icon_view->priv->scroll_to_path);
      icon_view->priv->scroll_to_path = NULL;
    }

  /* The area can be NULL while disposing */
  if (icon_view->priv->cell_area)
    bobgui_cell_area_stop_editing (icon_view->priv->cell_area, TRUE);

  dirty = bobgui_icon_view_unselect_all_internal (icon_view);

  if (model)
    {
      if (icon_view->priv->pixbuf_column != -1)
	{
	  g_return_if_fail (bobgui_tree_model_get_column_type (model, icon_view->priv->pixbuf_column) == GDK_TYPE_PIXBUF);
	}

      if (icon_view->priv->text_column != -1)
	{
	  g_return_if_fail (bobgui_tree_model_get_column_type (model, icon_view->priv->text_column) == G_TYPE_STRING);
	}

      if (icon_view->priv->markup_column != -1)
	{
	  g_return_if_fail (bobgui_tree_model_get_column_type (model, icon_view->priv->markup_column) == G_TYPE_STRING);
	}

    }

  if (icon_view->priv->model)
    {
      g_signal_handlers_disconnect_by_func (icon_view->priv->model,
					    bobgui_icon_view_row_changed,
					    icon_view);
      g_signal_handlers_disconnect_by_func (icon_view->priv->model,
					    bobgui_icon_view_row_inserted,
					    icon_view);
      g_signal_handlers_disconnect_by_func (icon_view->priv->model,
					    bobgui_icon_view_row_deleted,
					    icon_view);
      g_signal_handlers_disconnect_by_func (icon_view->priv->model,
					    bobgui_icon_view_rows_reordered,
					    icon_view);

      g_object_unref (icon_view->priv->model);

      g_list_free_full (icon_view->priv->items, (GDestroyNotify) bobgui_icon_view_item_free);
      icon_view->priv->items = NULL;
      icon_view->priv->anchor_item = NULL;
      icon_view->priv->cursor_item = NULL;
      icon_view->priv->last_single_clicked = NULL;
      icon_view->priv->last_prelight = NULL;
      icon_view->priv->width = 0;
      icon_view->priv->height = 0;
    }

  icon_view->priv->model = model;

  if (icon_view->priv->model)
    {
      g_object_ref (icon_view->priv->model);
      g_signal_connect (icon_view->priv->model,
			"row-changed",
			G_CALLBACK (bobgui_icon_view_row_changed),
			icon_view);
      g_signal_connect (icon_view->priv->model,
			"row-inserted",
			G_CALLBACK (bobgui_icon_view_row_inserted),
			icon_view);
      g_signal_connect (icon_view->priv->model,
			"row-deleted",
			G_CALLBACK (bobgui_icon_view_row_deleted),
			icon_view);
      g_signal_connect (icon_view->priv->model,
			"rows-reordered",
			G_CALLBACK (bobgui_icon_view_rows_reordered),
			icon_view);

      bobgui_icon_view_build_items (icon_view);
    }

  g_object_notify (G_OBJECT (icon_view), "model");

  if (dirty)
    g_signal_emit (icon_view, icon_view_signals[SELECTION_CHANGED], 0);

  bobgui_widget_queue_resize (BOBGUI_WIDGET (icon_view));
}

/**
 * bobgui_icon_view_get_model:
 * @icon_view: a `BobguiIconView`
 *
 * Returns the model the `BobguiIconView` is based on.  Returns %NULL if the
 * model is unset.
 *
 * Returns: (nullable) (transfer none): The currently used `BobguiTreeModel`
 *
 * Deprecated: 4.10: Use [class@Bobgui.GridView] instead
 */
BobguiTreeModel *
bobgui_icon_view_get_model (BobguiIconView *icon_view)
{
  g_return_val_if_fail (BOBGUI_IS_ICON_VIEW (icon_view), NULL);

  return icon_view->priv->model;
}

static void
update_text_cell (BobguiIconView *icon_view)
{
  if (!icon_view->priv->cell_area)
    return;

  if (icon_view->priv->text_column == -1 &&
      icon_view->priv->markup_column == -1)
    {
      if (icon_view->priv->text_cell != NULL)
	{
	  bobgui_cell_area_remove (icon_view->priv->cell_area,
				icon_view->priv->text_cell);
	  icon_view->priv->text_cell = NULL;
	}
    }
  else
    {
      if (icon_view->priv->text_cell == NULL)
	{
	  icon_view->priv->text_cell = bobgui_cell_renderer_text_new ();

	  bobgui_cell_layout_pack_end (BOBGUI_CELL_LAYOUT (icon_view), icon_view->priv->text_cell, FALSE);
	}

      if (icon_view->priv->markup_column != -1)
	bobgui_cell_layout_set_attributes (BOBGUI_CELL_LAYOUT (icon_view),
					icon_view->priv->text_cell,
					"markup", icon_view->priv->markup_column,
					NULL);
      else
	bobgui_cell_layout_set_attributes (BOBGUI_CELL_LAYOUT (icon_view),
					icon_view->priv->text_cell,
					"text", icon_view->priv->text_column,
					NULL);

      if (icon_view->priv->item_orientation == BOBGUI_ORIENTATION_VERTICAL)
	g_object_set (icon_view->priv->text_cell,
                      "alignment", PANGO_ALIGN_CENTER,
		      "wrap-mode", PANGO_WRAP_WORD_CHAR,
		      "xalign", 0.5,
		      "yalign", 0.0,
		      NULL);
      else
	g_object_set (icon_view->priv->text_cell,
                      "alignment", PANGO_ALIGN_LEFT,
		      "wrap-mode", PANGO_WRAP_WORD_CHAR,
		      "xalign", 0.0,
		      "yalign", 0.5,
		      NULL);
    }
}

static void
update_pixbuf_cell (BobguiIconView *icon_view)
{
  if (!icon_view->priv->cell_area)
    return;

  if (icon_view->priv->pixbuf_column == -1)
    {
      if (icon_view->priv->pixbuf_cell != NULL)
	{
	  bobgui_cell_area_remove (icon_view->priv->cell_area,
				icon_view->priv->pixbuf_cell);

	  icon_view->priv->pixbuf_cell = NULL;
	}
    }
  else
    {
      if (icon_view->priv->pixbuf_cell == NULL)
	{
	  icon_view->priv->pixbuf_cell = bobgui_cell_renderer_pixbuf_new ();

	  bobgui_cell_layout_pack_start (BOBGUI_CELL_LAYOUT (icon_view), icon_view->priv->pixbuf_cell, FALSE);
	}

      bobgui_cell_layout_set_attributes (BOBGUI_CELL_LAYOUT (icon_view),
				      icon_view->priv->pixbuf_cell,
				      "pixbuf", icon_view->priv->pixbuf_column,
				      NULL);

      if (icon_view->priv->item_orientation == BOBGUI_ORIENTATION_VERTICAL)
	g_object_set (icon_view->priv->pixbuf_cell,
		      "xalign", 0.5,
		      "yalign", 1.0,
		      NULL);
      else
	g_object_set (icon_view->priv->pixbuf_cell,
		      "xalign", 0.0,
		      "yalign", 0.0,
		      NULL);
    }
}

/**
 * bobgui_icon_view_set_text_column:
 * @icon_view: A `BobguiIconView`.
 * @column: A column in the currently used model, or -1 to display no text
 *
 * Sets the column with text for @icon_view to be @column. The text
 * column must be of type `G_TYPE_STRING`.
 *
 * Deprecated: 4.10: Use [class@Bobgui.GridView] instead
 **/
void
bobgui_icon_view_set_text_column (BobguiIconView *icon_view,
			       int           column)
{
  if (column == icon_view->priv->text_column)
    return;

  if (column == -1)
    icon_view->priv->text_column = -1;
  else
    {
      if (icon_view->priv->model != NULL)
	{
	  g_return_if_fail (bobgui_tree_model_get_column_type (icon_view->priv->model, column) == G_TYPE_STRING);
	}

      icon_view->priv->text_column = column;
    }

  if (icon_view->priv->cell_area)
    bobgui_cell_area_stop_editing (icon_view->priv->cell_area, TRUE);

  update_text_cell (icon_view);

  bobgui_icon_view_invalidate_sizes (icon_view);

  g_object_notify (G_OBJECT (icon_view), "text-column");
}

/**
 * bobgui_icon_view_get_text_column:
 * @icon_view: A `BobguiIconView`.
 *
 * Returns the column with text for @icon_view.
 *
 * Returns: the text column, or -1 if it’s unset.
 *
 * Deprecated: 4.10: Use [class@Bobgui.GridView] instead
 */
int
bobgui_icon_view_get_text_column (BobguiIconView  *icon_view)
{
  g_return_val_if_fail (BOBGUI_IS_ICON_VIEW (icon_view), -1);

  return icon_view->priv->text_column;
}

/**
 * bobgui_icon_view_set_markup_column:
 * @icon_view: A `BobguiIconView`.
 * @column: A column in the currently used model, or -1 to display no text
 *
 * Sets the column with markup information for @icon_view to be
 * @column. The markup column must be of type `G_TYPE_STRING`.
 * If the markup column is set to something, it overrides
 * the text column set by bobgui_icon_view_set_text_column().
 *
 * Deprecated: 4.10: Use [class@Bobgui.GridView] instead
 **/
void
bobgui_icon_view_set_markup_column (BobguiIconView *icon_view,
				 int          column)
{
  if (column == icon_view->priv->markup_column)
    return;

  if (column == -1)
    icon_view->priv->markup_column = -1;
  else
    {
      if (icon_view->priv->model != NULL)
	{
	  g_return_if_fail (bobgui_tree_model_get_column_type (icon_view->priv->model, column) == G_TYPE_STRING);
	}

      icon_view->priv->markup_column = column;
    }

  if (icon_view->priv->cell_area)
    bobgui_cell_area_stop_editing (icon_view->priv->cell_area, TRUE);

  update_text_cell (icon_view);

  bobgui_icon_view_invalidate_sizes (icon_view);

  g_object_notify (G_OBJECT (icon_view), "markup-column");
}

/**
 * bobgui_icon_view_get_markup_column:
 * @icon_view: A `BobguiIconView`.
 *
 * Returns the column with markup text for @icon_view.
 *
 * Returns: the markup column, or -1 if it’s unset.
 *
 * Deprecated: 4.10: Use [class@Bobgui.GridView] instead
 */
int
bobgui_icon_view_get_markup_column (BobguiIconView  *icon_view)
{
  g_return_val_if_fail (BOBGUI_IS_ICON_VIEW (icon_view), -1);

  return icon_view->priv->markup_column;
}

/**
 * bobgui_icon_view_set_pixbuf_column:
 * @icon_view: A `BobguiIconView`.
 * @column: A column in the currently used model, or -1 to disable
 *
 * Sets the column with pixbufs for @icon_view to be @column. The pixbuf
 * column must be of type `GDK_TYPE_PIXBUF`
 *
 * Deprecated: 4.10: Use [class@Bobgui.GridView] instead
 **/
void
bobgui_icon_view_set_pixbuf_column (BobguiIconView *icon_view,
				 int          column)
{
  if (column == icon_view->priv->pixbuf_column)
    return;

  if (column == -1)
    icon_view->priv->pixbuf_column = -1;
  else
    {
      if (icon_view->priv->model != NULL)
	{
	  g_return_if_fail (bobgui_tree_model_get_column_type (icon_view->priv->model, column) == GDK_TYPE_PIXBUF);
	}

      icon_view->priv->pixbuf_column = column;
    }

  if (icon_view->priv->cell_area)
    bobgui_cell_area_stop_editing (icon_view->priv->cell_area, TRUE);

  update_pixbuf_cell (icon_view);

  bobgui_icon_view_invalidate_sizes (icon_view);

  g_object_notify (G_OBJECT (icon_view), "pixbuf-column");

}

/**
 * bobgui_icon_view_get_pixbuf_column:
 * @icon_view: A `BobguiIconView`.
 *
 * Returns the column with pixbufs for @icon_view.
 *
 * Returns: the pixbuf column, or -1 if it’s unset.
 *
 * Deprecated: 4.10: Use [class@Bobgui.GridView] instead
 */
int
bobgui_icon_view_get_pixbuf_column (BobguiIconView  *icon_view)
{
  g_return_val_if_fail (BOBGUI_IS_ICON_VIEW (icon_view), -1);

  return icon_view->priv->pixbuf_column;
}

/**
 * bobgui_icon_view_select_path:
 * @icon_view: A `BobguiIconView`.
 * @path: The `BobguiTreePath` to be selected.
 *
 * Selects the row at @path.
 *
 * Deprecated: 4.10: Use [class@Bobgui.GridView] instead
 **/
void
bobgui_icon_view_select_path (BobguiIconView *icon_view,
			   BobguiTreePath *path)
{
  BobguiIconViewItem *item = NULL;

  g_return_if_fail (BOBGUI_IS_ICON_VIEW (icon_view));
  g_return_if_fail (icon_view->priv->model != NULL);
  g_return_if_fail (path != NULL);

  if (bobgui_tree_path_get_depth (path) > 0)
    item = g_list_nth_data (icon_view->priv->items,
			    bobgui_tree_path_get_indices(path)[0]);

  if (item)
    _bobgui_icon_view_select_item (icon_view, item);
}

/**
 * bobgui_icon_view_unselect_path:
 * @icon_view: A `BobguiIconView`.
 * @path: The `BobguiTreePath` to be unselected.
 *
 * Unselects the row at @path.
 *
 * Deprecated: 4.10: Use [class@Bobgui.GridView] instead
 **/
void
bobgui_icon_view_unselect_path (BobguiIconView *icon_view,
			     BobguiTreePath *path)
{
  BobguiIconViewItem *item;

  g_return_if_fail (BOBGUI_IS_ICON_VIEW (icon_view));
  g_return_if_fail (icon_view->priv->model != NULL);
  g_return_if_fail (path != NULL);

  item = g_list_nth_data (icon_view->priv->items,
			  bobgui_tree_path_get_indices(path)[0]);

  if (!item)
    return;

  _bobgui_icon_view_unselect_item (icon_view, item);
}

/**
 * bobgui_icon_view_get_selected_items:
 * @icon_view: A `BobguiIconView`.
 *
 * Creates a list of paths of all selected items. Additionally, if you are
 * planning on modifying the model after calling this function, you may
 * want to convert the returned list into a list of `BobguiTreeRowReferences`.
 * To do this, you can use bobgui_tree_row_reference_new().
 *
 * To free the return value, use `g_list_free_full`:
 *
 * ```c
 * BobguiWidget *icon_view = bobgui_icon_view_new ();
 * // Use icon_view
 *
 * GList *list = bobgui_icon_view_get_selected_items (BOBGUI_ICON_VIEW (icon_view));
 *
 * // use list
 *
 * g_list_free_full (list, (GDestroyNotify) bobgui_tree_path_free);
 * ```
 *
 * Returns: (element-type BobguiTreePath) (transfer full): A `GList` containing a `BobguiTreePath` for each selected row.
 *
 * Deprecated: 4.10: Use [class@Bobgui.GridView] instead
 **/
GList *
bobgui_icon_view_get_selected_items (BobguiIconView *icon_view)
{
  GList *list;
  GList *selected = NULL;

  g_return_val_if_fail (BOBGUI_IS_ICON_VIEW (icon_view), NULL);

  for (list = icon_view->priv->items; list != NULL; list = list->next)
    {
      BobguiIconViewItem *item = list->data;

      if (item->selected)
	{
	  BobguiTreePath *path = bobgui_tree_path_new_from_indices (item->index, -1);

	  selected = g_list_prepend (selected, path);
	}
    }

  return selected;
}

/**
 * bobgui_icon_view_select_all:
 * @icon_view: A `BobguiIconView`.
 *
 * Selects all the icons. @icon_view must has its selection mode set
 * to %BOBGUI_SELECTION_MULTIPLE.
 *
 * Deprecated: 4.10: Use [class@Bobgui.GridView] instead
 **/
void
bobgui_icon_view_select_all (BobguiIconView *icon_view)
{
  GList *items;
  gboolean dirty = FALSE;

  g_return_if_fail (BOBGUI_IS_ICON_VIEW (icon_view));

  if (icon_view->priv->selection_mode != BOBGUI_SELECTION_MULTIPLE)
    return;

  for (items = icon_view->priv->items; items; items = items->next)
    {
      BobguiIconViewItem *item = items->data;

      if (!item->selected)
	{
	  dirty = TRUE;
	  item->selected = TRUE;
	  bobgui_icon_view_queue_draw_item (icon_view, item);
	}
    }

  if (dirty)
    g_signal_emit (icon_view, icon_view_signals[SELECTION_CHANGED], 0);
}

/**
 * bobgui_icon_view_unselect_all:
 * @icon_view: A `BobguiIconView`.
 *
 * Unselects all the icons.
 *
 * Deprecated: 4.10: Use [class@Bobgui.GridView] instead
 **/
void
bobgui_icon_view_unselect_all (BobguiIconView *icon_view)
{
  gboolean dirty = FALSE;

  g_return_if_fail (BOBGUI_IS_ICON_VIEW (icon_view));

  if (icon_view->priv->selection_mode == BOBGUI_SELECTION_BROWSE)
    return;

  dirty = bobgui_icon_view_unselect_all_internal (icon_view);

  if (dirty)
    g_signal_emit (icon_view, icon_view_signals[SELECTION_CHANGED], 0);
}

/**
 * bobgui_icon_view_path_is_selected:
 * @icon_view: A `BobguiIconView`.
 * @path: A `BobguiTreePath` to check selection on.
 *
 * Returns %TRUE if the icon pointed to by @path is currently
 * selected. If @path does not point to a valid location, %FALSE is returned.
 *
 * Returns: %TRUE if @path is selected.
 *
 * Deprecated: 4.10: Use [class@Bobgui.GridView] instead
 **/
gboolean
bobgui_icon_view_path_is_selected (BobguiIconView *icon_view,
				BobguiTreePath *path)
{
  BobguiIconViewItem *item;

  g_return_val_if_fail (BOBGUI_IS_ICON_VIEW (icon_view), FALSE);
  g_return_val_if_fail (icon_view->priv->model != NULL, FALSE);
  g_return_val_if_fail (path != NULL, FALSE);

  item = g_list_nth_data (icon_view->priv->items,
			  bobgui_tree_path_get_indices(path)[0]);

  if (!item)
    return FALSE;

  return item->selected;
}

/**
 * bobgui_icon_view_get_item_row:
 * @icon_view: a `BobguiIconView`
 * @path: the `BobguiTreePath` of the item
 *
 * Gets the row in which the item @path is currently
 * displayed. Row numbers start at 0.
 *
 * Returns: The row in which the item is displayed
 *
 * Deprecated: 4.10: Use [class@Bobgui.GridView] instead
 */
int
bobgui_icon_view_get_item_row (BobguiIconView *icon_view,
                            BobguiTreePath *path)
{
  BobguiIconViewItem *item;

  g_return_val_if_fail (BOBGUI_IS_ICON_VIEW (icon_view), -1);
  g_return_val_if_fail (icon_view->priv->model != NULL, -1);
  g_return_val_if_fail (path != NULL, -1);

  item = g_list_nth_data (icon_view->priv->items,
                          bobgui_tree_path_get_indices(path)[0]);

  if (!item)
    return -1;

  return item->row;
}

/**
 * bobgui_icon_view_get_item_column:
 * @icon_view: a `BobguiIconView`
 * @path: the `BobguiTreePath` of the item
 *
 * Gets the column in which the item @path is currently
 * displayed. Column numbers start at 0.
 *
 * Returns: The column in which the item is displayed
 *
 * Deprecated: 4.10: Use [class@Bobgui.GridView] instead
 */
int
bobgui_icon_view_get_item_column (BobguiIconView *icon_view,
                               BobguiTreePath *path)
{
  BobguiIconViewItem *item;

  g_return_val_if_fail (BOBGUI_IS_ICON_VIEW (icon_view), -1);
  g_return_val_if_fail (icon_view->priv->model != NULL, -1);
  g_return_val_if_fail (path != NULL, -1);

  item = g_list_nth_data (icon_view->priv->items,
                          bobgui_tree_path_get_indices(path)[0]);

  if (!item)
    return -1;

  return item->col;
}

/**
 * bobgui_icon_view_item_activated:
 * @icon_view: A `BobguiIconView`
 * @path: The `BobguiTreePath` to be activated
 *
 * Activates the item determined by @path.
 *
 * Deprecated: 4.10: Use [class@Bobgui.GridView] instead
 **/
void
bobgui_icon_view_item_activated (BobguiIconView      *icon_view,
			      BobguiTreePath      *path)
{
  g_return_if_fail (BOBGUI_IS_ICON_VIEW (icon_view));
  g_return_if_fail (path != NULL);

  g_signal_emit (icon_view, icon_view_signals[ITEM_ACTIVATED], 0, path);
}

/**
 * bobgui_icon_view_set_item_orientation:
 * @icon_view: a `BobguiIconView`
 * @orientation: the relative position of texts and icons
 *
 * Sets the ::item-orientation property which determines whether the labels
 * are drawn beside the icons instead of below.
 *
 * Deprecated: 4.10: Use [class@Bobgui.GridView] instead
 **/
void
bobgui_icon_view_set_item_orientation (BobguiIconView    *icon_view,
                                    BobguiOrientation  orientation)
{
  g_return_if_fail (BOBGUI_IS_ICON_VIEW (icon_view));

  if (icon_view->priv->item_orientation != orientation)
    {
      icon_view->priv->item_orientation = orientation;

      if (icon_view->priv->cell_area)
	{
	  if (BOBGUI_IS_ORIENTABLE (icon_view->priv->cell_area))
	    bobgui_orientable_set_orientation (BOBGUI_ORIENTABLE (icon_view->priv->cell_area),
					    icon_view->priv->item_orientation);

	  bobgui_cell_area_stop_editing (icon_view->priv->cell_area, TRUE);
	}

      bobgui_icon_view_invalidate_sizes (icon_view);

      update_text_cell (icon_view);
      update_pixbuf_cell (icon_view);

      g_object_notify (G_OBJECT (icon_view), "item-orientation");
    }
}

/**
 * bobgui_icon_view_get_item_orientation:
 * @icon_view: a `BobguiIconView`
 *
 * Returns the value of the ::item-orientation property which determines
 * whether the labels are drawn beside the icons instead of below.
 *
 * Returns: the relative position of texts and icons
 *
 * Deprecated: 4.10: Use [class@Bobgui.GridView] instead
 **/
BobguiOrientation
bobgui_icon_view_get_item_orientation (BobguiIconView *icon_view)
{
  g_return_val_if_fail (BOBGUI_IS_ICON_VIEW (icon_view),
			BOBGUI_ORIENTATION_VERTICAL);

  return icon_view->priv->item_orientation;
}

/**
 * bobgui_icon_view_set_columns:
 * @icon_view: a `BobguiIconView`
 * @columns: the number of columns
 *
 * Sets the ::columns property which determines in how
 * many columns the icons are arranged. If @columns is
 * -1, the number of columns will be chosen automatically
 * to fill the available area.
 *
 * Deprecated: 4.10: Use [class@Bobgui.GridView] instead
 */
void
bobgui_icon_view_set_columns (BobguiIconView *icon_view,
			   int          columns)
{
  g_return_if_fail (BOBGUI_IS_ICON_VIEW (icon_view));

  if (icon_view->priv->columns != columns)
    {
      icon_view->priv->columns = columns;

      if (icon_view->priv->cell_area)
	bobgui_cell_area_stop_editing (icon_view->priv->cell_area, TRUE);

      bobgui_widget_queue_resize (BOBGUI_WIDGET (icon_view));

      g_object_notify (G_OBJECT (icon_view), "columns");
    }
}

/**
 * bobgui_icon_view_get_columns:
 * @icon_view: a `BobguiIconView`
 *
 * Returns the value of the ::columns property.
 *
 * Returns: the number of columns, or -1
 *
 * Deprecated: 4.10: Use [class@Bobgui.GridView] instead
 */
int
bobgui_icon_view_get_columns (BobguiIconView *icon_view)
{
  g_return_val_if_fail (BOBGUI_IS_ICON_VIEW (icon_view), -1);

  return icon_view->priv->columns;
}

/**
 * bobgui_icon_view_set_item_width:
 * @icon_view: a `BobguiIconView`
 * @item_width: the width for each item
 *
 * Sets the ::item-width property which specifies the width
 * to use for each item. If it is set to -1, the icon view will
 * automatically determine a suitable item size.
 *
 * Deprecated: 4.10: Use [class@Bobgui.GridView] instead
 */
void
bobgui_icon_view_set_item_width (BobguiIconView *icon_view,
			      int          item_width)
{
  g_return_if_fail (BOBGUI_IS_ICON_VIEW (icon_view));

  if (icon_view->priv->item_width != item_width)
    {
      icon_view->priv->item_width = item_width;

      if (icon_view->priv->cell_area)
	bobgui_cell_area_stop_editing (icon_view->priv->cell_area, TRUE);

      bobgui_icon_view_invalidate_sizes (icon_view);

      update_text_cell (icon_view);

      g_object_notify (G_OBJECT (icon_view), "item-width");
    }
}

/**
 * bobgui_icon_view_get_item_width:
 * @icon_view: a `BobguiIconView`
 *
 * Returns the value of the ::item-width property.
 *
 * Returns: the width of a single item, or -1
 *
 * Deprecated: 4.10: Use [class@Bobgui.GridView] instead
 */
int
bobgui_icon_view_get_item_width (BobguiIconView *icon_view)
{
  g_return_val_if_fail (BOBGUI_IS_ICON_VIEW (icon_view), -1);

  return icon_view->priv->item_width;
}


/**
 * bobgui_icon_view_set_spacing:
 * @icon_view: a `BobguiIconView`
 * @spacing: the spacing
 *
 * Sets the ::spacing property which specifies the space
 * which is inserted between the cells (i.e. the icon and
 * the text) of an item.
 *
 * Deprecated: 4.10: Use [class@Bobgui.GridView] instead
 */
void
bobgui_icon_view_set_spacing (BobguiIconView *icon_view,
			   int          spacing)
{
  g_return_if_fail (BOBGUI_IS_ICON_VIEW (icon_view));

  if (icon_view->priv->spacing != spacing)
    {
      icon_view->priv->spacing = spacing;

      if (icon_view->priv->cell_area)
	bobgui_cell_area_stop_editing (icon_view->priv->cell_area, TRUE);

      bobgui_icon_view_invalidate_sizes (icon_view);

      g_object_notify (G_OBJECT (icon_view), "spacing");
    }
}

/**
 * bobgui_icon_view_get_spacing:
 * @icon_view: a `BobguiIconView`
 *
 * Returns the value of the ::spacing property.
 *
 * Returns: the space between cells
 *
 * Deprecated: 4.10: Use [class@Bobgui.GridView] instead
 */
int
bobgui_icon_view_get_spacing (BobguiIconView *icon_view)
{
  g_return_val_if_fail (BOBGUI_IS_ICON_VIEW (icon_view), -1);

  return icon_view->priv->spacing;
}

/**
 * bobgui_icon_view_set_row_spacing:
 * @icon_view: a `BobguiIconView`
 * @row_spacing: the row spacing
 *
 * Sets the ::row-spacing property which specifies the space
 * which is inserted between the rows of the icon view.
 *
 * Deprecated: 4.10: Use [class@Bobgui.GridView] instead
 */
void
bobgui_icon_view_set_row_spacing (BobguiIconView *icon_view,
			       int          row_spacing)
{
  g_return_if_fail (BOBGUI_IS_ICON_VIEW (icon_view));

  if (icon_view->priv->row_spacing != row_spacing)
    {
      icon_view->priv->row_spacing = row_spacing;

      if (icon_view->priv->cell_area)
	bobgui_cell_area_stop_editing (icon_view->priv->cell_area, TRUE);

      bobgui_icon_view_invalidate_sizes (icon_view);

      g_object_notify (G_OBJECT (icon_view), "row-spacing");
    }
}

/**
 * bobgui_icon_view_get_row_spacing:
 * @icon_view: a `BobguiIconView`
 *
 * Returns the value of the ::row-spacing property.
 *
 * Returns: the space between rows
 *
 * Deprecated: 4.10: Use [class@Bobgui.GridView] instead
 */
int
bobgui_icon_view_get_row_spacing (BobguiIconView *icon_view)
{
  g_return_val_if_fail (BOBGUI_IS_ICON_VIEW (icon_view), -1);

  return icon_view->priv->row_spacing;
}

/**
 * bobgui_icon_view_set_column_spacing:
 * @icon_view: a `BobguiIconView`
 * @column_spacing: the column spacing
 *
 * Sets the ::column-spacing property which specifies the space
 * which is inserted between the columns of the icon view.
 *
 * Deprecated: 4.10: Use [class@Bobgui.GridView] instead
 */
void
bobgui_icon_view_set_column_spacing (BobguiIconView *icon_view,
				  int          column_spacing)
{
  g_return_if_fail (BOBGUI_IS_ICON_VIEW (icon_view));

  if (icon_view->priv->column_spacing != column_spacing)
    {
      icon_view->priv->column_spacing = column_spacing;

      if (icon_view->priv->cell_area)
	bobgui_cell_area_stop_editing (icon_view->priv->cell_area, TRUE);

      bobgui_icon_view_invalidate_sizes (icon_view);

      g_object_notify (G_OBJECT (icon_view), "column-spacing");
    }
}

/**
 * bobgui_icon_view_get_column_spacing:
 * @icon_view: a `BobguiIconView`
 *
 * Returns the value of the ::column-spacing property.
 *
 * Returns: the space between columns
 *
 * Deprecated: 4.10: Use [class@Bobgui.GridView] instead
 */
int
bobgui_icon_view_get_column_spacing (BobguiIconView *icon_view)
{
  g_return_val_if_fail (BOBGUI_IS_ICON_VIEW (icon_view), -1);

  return icon_view->priv->column_spacing;
}

/**
 * bobgui_icon_view_set_margin:
 * @icon_view: a `BobguiIconView`
 * @margin: the margin
 *
 * Sets the ::margin property which specifies the space
 * which is inserted at the top, bottom, left and right
 * of the icon view.
 *
 * Deprecated: 4.10: Use [class@Bobgui.GridView] instead
 */
void
bobgui_icon_view_set_margin (BobguiIconView *icon_view,
			  int          margin)
{
  g_return_if_fail (BOBGUI_IS_ICON_VIEW (icon_view));

  if (icon_view->priv->margin != margin)
    {
      icon_view->priv->margin = margin;

      if (icon_view->priv->cell_area)
	bobgui_cell_area_stop_editing (icon_view->priv->cell_area, TRUE);

      bobgui_icon_view_invalidate_sizes (icon_view);

      g_object_notify (G_OBJECT (icon_view), "margin");
    }
}

/**
 * bobgui_icon_view_get_margin:
 * @icon_view: a `BobguiIconView`
 *
 * Returns the value of the ::margin property.
 *
 * Returns: the space at the borders
 *
 * Deprecated: 4.10: Use [class@Bobgui.GridView] instead
 */
int
bobgui_icon_view_get_margin (BobguiIconView *icon_view)
{
  g_return_val_if_fail (BOBGUI_IS_ICON_VIEW (icon_view), -1);

  return icon_view->priv->margin;
}

/**
 * bobgui_icon_view_set_item_padding:
 * @icon_view: a `BobguiIconView`
 * @item_padding: the item padding
 *
 * Sets the `BobguiIconView`:item-padding property which specifies the padding
 * around each of the icon view’s items.
 *
 * Deprecated: 4.10: Use [class@Bobgui.GridView] instead
 */
void
bobgui_icon_view_set_item_padding (BobguiIconView *icon_view,
				int          item_padding)
{
  g_return_if_fail (BOBGUI_IS_ICON_VIEW (icon_view));

  if (icon_view->priv->item_padding != item_padding)
    {
      icon_view->priv->item_padding = item_padding;

      if (icon_view->priv->cell_area)
	bobgui_cell_area_stop_editing (icon_view->priv->cell_area, TRUE);

      bobgui_icon_view_invalidate_sizes (icon_view);

      g_object_notify (G_OBJECT (icon_view), "item-padding");
    }
}

/**
 * bobgui_icon_view_get_item_padding:
 * @icon_view: a `BobguiIconView`
 *
 * Returns the value of the ::item-padding property.
 *
 * Returns: the padding around items
 *
 * Deprecated: 4.10: Use [class@Bobgui.GridView] instead
 */
int
bobgui_icon_view_get_item_padding (BobguiIconView *icon_view)
{
  g_return_val_if_fail (BOBGUI_IS_ICON_VIEW (icon_view), -1);

  return icon_view->priv->item_padding;
}

/* Get/set whether drag_motion requested the drag data and
 * drag_data_received should thus not actually insert the data,
 * since the data doesn’t result from a drop.
 */
static void
set_status_pending (GdkDrop        *drop,
                    GdkDragAction   suggested_action)
{
  g_object_set_data (G_OBJECT (drop),
                     I_("bobgui-icon-view-status-pending"),
                     GINT_TO_POINTER (suggested_action));
}

static GdkDragAction
get_status_pending (GdkDrop *drop)
{
  return GPOINTER_TO_INT (g_object_get_data (G_OBJECT (drop),
                                             "bobgui-icon-view-status-pending"));
}

static void
unset_reorderable (BobguiIconView *icon_view)
{
  if (icon_view->priv->reorderable)
    {
      icon_view->priv->reorderable = FALSE;
      g_object_notify (G_OBJECT (icon_view), "reorderable");
    }
}

typedef struct
{
  BobguiTreeRowReference *dest_row;
  gboolean             empty_view_drop;
  gboolean             drop_append_mode;
} DestRow;

static void
dest_row_free (gpointer data)
{
  DestRow *dr = (DestRow *)data;

  bobgui_tree_row_reference_free (dr->dest_row);
  g_free (dr);
}

static void
set_dest_row (GdkDrop        *drop,
	      BobguiTreeModel   *model,
	      BobguiTreePath    *dest_row,
	      gboolean        empty_view_drop,
	      gboolean        drop_append_mode)
{
  DestRow *dr;

  if (!dest_row)
    {
      g_object_set_data_full (G_OBJECT (drop),
			      I_("bobgui-icon-view-dest-row"),
			      NULL, NULL);
      return;
    }

  dr = g_new0 (DestRow, 1);

  dr->dest_row = bobgui_tree_row_reference_new (model, dest_row);
  dr->empty_view_drop = empty_view_drop;
  dr->drop_append_mode = drop_append_mode;
  g_object_set_data_full (G_OBJECT (drop),
                          I_("bobgui-icon-view-dest-row"),
                          dr, (GDestroyNotify) dest_row_free);
}

static BobguiTreePath*
get_dest_row (GdkDrop *drop)
{
  DestRow *dr;

  dr = g_object_get_data (G_OBJECT (drop), "bobgui-icon-view-dest-row");

  if (dr)
    {
      BobguiTreePath *path = NULL;

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

static gboolean
check_model_dnd (BobguiTreeModel *model,
                 GType         required_iface,
                 const char   *signal)
{
  if (model == NULL || !G_TYPE_CHECK_INSTANCE_TYPE ((model), required_iface))
    {
      g_warning ("You must override the default '%s' handler "
                 "on BobguiIconView when using models that don't support "
                 "the %s interface and enabling drag-and-drop. The simplest way to do this "
                 "is to connect to '%s' and call "
                 "g_signal_stop_emission_by_name() in your signal handler to prevent "
                 "the default handler from running. Look at the source code "
                 "for the default handler in bobguiiconview.c to get an idea what "
                 "your handler should do. (bobguiiconview.c is in the BOBGUI source "
                 "code.) If you're using BOBGUI from a language other than C, "
                 "there may be a more natural way to override default handlers, e.g. via derivation.",
                 signal, g_type_name (required_iface), signal);
      return FALSE;
    }
  else
    return TRUE;
}

static void
remove_scroll_timeout (BobguiIconView *icon_view)
{
  if (icon_view->priv->scroll_timeout_id != 0)
    {
      g_source_remove (icon_view->priv->scroll_timeout_id);

      icon_view->priv->scroll_timeout_id = 0;
    }
}

static void
bobgui_icon_view_autoscroll (BobguiIconView *icon_view)
{
  int px, py, width, height;
  int hoffset, voffset;

  px = icon_view->priv->event_last_x;
  py = icon_view->priv->event_last_y;

  width = bobgui_widget_get_width (BOBGUI_WIDGET (icon_view));
  height = bobgui_widget_get_height (BOBGUI_WIDGET (icon_view));

  /* see if we are near the edge. */
  voffset = py - 2 * SCROLL_EDGE_SIZE;
  if (voffset > 0)
    voffset = MAX (py - (height - 2 * SCROLL_EDGE_SIZE), 0);

  hoffset = px - 2 * SCROLL_EDGE_SIZE;
  if (hoffset > 0)
    hoffset = MAX (px - (width - 2 * SCROLL_EDGE_SIZE), 0);

  if (voffset != 0)
    bobgui_adjustment_set_value (icon_view->priv->vadjustment,
                              bobgui_adjustment_get_value (icon_view->priv->vadjustment) + voffset);

  if (hoffset != 0)
    bobgui_adjustment_set_value (icon_view->priv->hadjustment,
                              bobgui_adjustment_get_value (icon_view->priv->hadjustment) + hoffset);
}

static gboolean
drag_scroll_timeout (gpointer data)
{
  bobgui_icon_view_autoscroll (data);

  return TRUE;
}

static GdkDragAction
bobgui_icon_view_get_action (BobguiWidget *widget,
                          GdkDrop   *drop)
{
  BobguiIconView *iconview = BOBGUI_ICON_VIEW (widget);
  GdkDrag *drag = gdk_drop_get_drag (drop);
  GdkDragAction actions;

  actions = gdk_drop_get_actions (drop);

  if (drag == iconview->priv->drag &&
      actions & GDK_ACTION_MOVE)
    return GDK_ACTION_MOVE;

  if (actions & GDK_ACTION_COPY)
    return GDK_ACTION_COPY;

  if (actions & GDK_ACTION_MOVE)
    return GDK_ACTION_MOVE;

  if (actions & GDK_ACTION_LINK)
    return GDK_ACTION_LINK;

  return 0;
}

static gboolean
set_destination (BobguiIconView        *icon_view,
		 GdkDrop            *drop,
		 BobguiDropTargetAsync *dest,
		 int                 x,
		 int                 y,
		 GdkDragAction      *suggested_action,
		 GType              *target)
{
  BobguiWidget *widget;
  BobguiTreePath *path = NULL;
  BobguiIconViewDropPosition pos;
  BobguiIconViewDropPosition old_pos;
  BobguiTreePath *old_dest_path = NULL;
  GdkContentFormats *formats;
  gboolean can_drop = FALSE;

  widget = BOBGUI_WIDGET (icon_view);

  *suggested_action = 0;
  *target = G_TYPE_INVALID;

  if (!icon_view->priv->dest_set)
    {
      /* someone unset us as a drag dest, note that if
       * we return FALSE drag_leave isn't called
       */

      bobgui_icon_view_set_drag_dest_item (icon_view,
					NULL,
					BOBGUI_ICON_VIEW_DROP_LEFT);

      remove_scroll_timeout (BOBGUI_ICON_VIEW (widget));

      return FALSE; /* no longer a drop site */
    }

  formats = bobgui_drop_target_async_get_formats (dest);
  *target = gdk_content_formats_match_gtype (formats, formats);
  if (*target == G_TYPE_INVALID)
    return FALSE;

  if (!bobgui_icon_view_get_dest_item_at_pos (icon_view, x, y, &path, &pos))
    {
      int n_children;
      BobguiTreeModel *model;

      /* the row got dropped on empty space, let's setup a special case
       */

      if (path)
	bobgui_tree_path_free (path);

      model = bobgui_icon_view_get_model (icon_view);

      n_children = bobgui_tree_model_iter_n_children (model, NULL);
      if (n_children)
        {
          pos = BOBGUI_ICON_VIEW_DROP_BELOW;
          path = bobgui_tree_path_new_from_indices (n_children - 1, -1);
        }
      else
        {
          pos = BOBGUI_ICON_VIEW_DROP_ABOVE;
          path = bobgui_tree_path_new_from_indices (0, -1);
        }

      can_drop = TRUE;

      goto out;
    }

  g_assert (path);

  bobgui_icon_view_get_drag_dest_item (icon_view,
				    &old_dest_path,
				    &old_pos);

  if (old_dest_path)
    bobgui_tree_path_free (old_dest_path);

  if (TRUE /* FIXME if the location droppable predicate */)
    {
      can_drop = TRUE;
    }

out:
  if (can_drop)
    {
      *suggested_action = bobgui_icon_view_get_action (widget, drop);

      bobgui_icon_view_set_drag_dest_item (BOBGUI_ICON_VIEW (widget),
					path, pos);
    }
  else
    {
      /* can't drop here */
      bobgui_icon_view_set_drag_dest_item (BOBGUI_ICON_VIEW (widget),
					NULL,
					BOBGUI_ICON_VIEW_DROP_LEFT);
    }

  if (path)
    bobgui_tree_path_free (path);

  return TRUE;
}

static BobguiTreePath*
get_logical_destination (BobguiIconView *icon_view,
			 gboolean    *drop_append_mode)
{
  /* adjust path to point to the row the drop goes in front of */
  BobguiTreePath *path = NULL;
  BobguiIconViewDropPosition pos;

  *drop_append_mode = FALSE;

  bobgui_icon_view_get_drag_dest_item (icon_view, &path, &pos);

  if (path == NULL)
    return NULL;

  if (pos == BOBGUI_ICON_VIEW_DROP_RIGHT ||
      pos == BOBGUI_ICON_VIEW_DROP_BELOW)
    {
      BobguiTreeIter iter;
      BobguiTreeModel *model = icon_view->priv->model;

      if (!bobgui_tree_model_get_iter (model, &iter, path) ||
          !bobgui_tree_model_iter_next (model, &iter))
        *drop_append_mode = TRUE;
      else
        {
          *drop_append_mode = FALSE;
          bobgui_tree_path_next (path);
        }
    }

  return path;
}

static gboolean
bobgui_icon_view_maybe_begin_drag (BobguiIconView *icon_view,
                                double       x,
                                double       y,
                                GdkDevice   *device)
{
  BobguiTreePath *path = NULL;
  BobguiTreeModel *model;
  gboolean retval = FALSE;
  GdkContentProvider *content;
  GdkPaintable *icon;
  BobguiIconViewItem *item;
  GdkSurface *surface;
  GdkDrag *drag;

  if (!icon_view->priv->source_set)
    goto out;

  if (icon_view->priv->pressed_button < 0)
    goto out;

  if (!bobgui_drag_check_threshold_double (BOBGUI_WIDGET (icon_view),
                                        icon_view->priv->press_start_x,
                                        icon_view->priv->press_start_y,
                                        x, y))
    goto out;

  model = bobgui_icon_view_get_model (icon_view);

  if (model == NULL)
    goto out;

  icon_view->priv->pressed_button = -1;

  item = _bobgui_icon_view_get_item_at_coords (icon_view,
					   icon_view->priv->press_start_x,
					   icon_view->priv->press_start_y,
					   TRUE,
					   NULL);

  if (item == NULL)
    goto out;

  path = bobgui_tree_path_new_from_indices (item->index, -1);

  if (!BOBGUI_IS_TREE_DRAG_SOURCE (model) ||
      !bobgui_tree_drag_source_row_draggable (BOBGUI_TREE_DRAG_SOURCE (model), path))
    goto out;

  /* FIXME Check whether we're a start button, if not return FALSE and
   * free path
   */

  /* Now we can begin the drag */

  retval = TRUE;

  surface = bobgui_native_get_surface (bobgui_widget_get_native (BOBGUI_WIDGET (icon_view)));

  content = bobgui_icon_view_drag_data_get (icon_view, path);
  if (content == NULL)
    goto out;

  drag = gdk_drag_begin (surface,
                         device,
                         content,
                         icon_view->priv->source_actions,
                         icon_view->priv->press_start_x,
                         icon_view->priv->press_start_y);

  g_object_unref (content);

  g_signal_connect (drag, "dnd-finished", G_CALLBACK (bobgui_icon_view_dnd_finished_cb), icon_view);

  icon_view->priv->source_item = bobgui_tree_row_reference_new (model, path);

  x = icon_view->priv->press_start_x - item->cell_area.x + icon_view->priv->item_padding;
  y = icon_view->priv->press_start_y - item->cell_area.y + icon_view->priv->item_padding;

  icon = bobgui_icon_view_create_drag_icon (icon_view, path);
  bobgui_drag_icon_set_from_paintable (drag, icon, x, y);
  g_object_unref (icon);

  icon_view->priv->drag = drag;

  g_object_unref (drag);

 out:
  if (path)
    bobgui_tree_path_free (path);

  return retval;
}

/* Source side drag signals */
static GdkContentProvider *
bobgui_icon_view_drag_data_get (BobguiIconView *icon_view,
                             BobguiTreePath *source_row)
{
  GdkContentProvider *content;
  BobguiTreeModel *model;

  model = bobgui_icon_view_get_model (icon_view);

  if (model == NULL)
    return NULL;

  if (!icon_view->priv->source_set)
    return NULL;

  /* We can implement the BOBGUI_TREE_MODEL_ROW target generically for
   * any model; for DragSource models there are some other formats
   * we also support.
   */

  if (BOBGUI_IS_TREE_DRAG_SOURCE (model))
    content = bobgui_tree_drag_source_drag_data_get (BOBGUI_TREE_DRAG_SOURCE (model), source_row);
  else
    content = NULL;

  /* If drag_data_get does nothing, try providing row data. */
  if (content == NULL)
    content = bobgui_tree_create_row_drag_content (model, source_row);

  return content;
}

static void
bobgui_icon_view_dnd_finished_cb (GdkDrag   *drag,
                               BobguiWidget *widget)
{
  BobguiTreeModel *model;
  BobguiIconView *icon_view;
  BobguiTreePath *source_row;

  if (gdk_drag_get_selected_action (drag) != GDK_ACTION_MOVE)
    return;

  icon_view = BOBGUI_ICON_VIEW (widget);
  model = bobgui_icon_view_get_model (icon_view);

  if (!check_model_dnd (model, BOBGUI_TYPE_TREE_DRAG_SOURCE, "drag-data-delete"))
    return;

  if (!icon_view->priv->source_set)
    return;

  source_row = bobgui_tree_row_reference_get_path (icon_view->priv->source_item);
  if (source_row == NULL)
    return;

  bobgui_tree_drag_source_drag_data_delete (BOBGUI_TREE_DRAG_SOURCE (model), source_row);

  bobgui_tree_path_free (source_row);

  g_clear_pointer (&icon_view->priv->source_item, bobgui_tree_row_reference_free);
  icon_view->priv->drag = NULL;
}

/* Target side drag signals */
static void
bobgui_icon_view_drag_leave (BobguiDropTargetAsync *dest,
                          GdkDrop            *drop,
                          BobguiIconView        *icon_view)
{
  /* unset any highlight row */
  bobgui_icon_view_set_drag_dest_item (icon_view,
				    NULL,
				    BOBGUI_ICON_VIEW_DROP_LEFT);

  remove_scroll_timeout (icon_view);
}

static GdkDragAction
bobgui_icon_view_drag_motion (BobguiDropTargetAsync *dest,
                           GdkDrop            *drop,
			   double              x,
			   double              y,
                           BobguiIconView        *icon_view)
{
  BobguiTreePath *path = NULL;
  BobguiIconViewDropPosition pos;
  GdkDragAction suggested_action = 0;
  GType target;
  gboolean empty;
  GdkDragAction result;

  if (!set_destination (icon_view, drop, dest, x, y, &suggested_action, &target))
    return 0;

  bobgui_icon_view_get_drag_dest_item (icon_view, &path, &pos);

  /* we only know this *after* set_desination_row */
  empty = icon_view->priv->empty_view_drop;

  if (path == NULL && !empty)
    {
      /* Can't drop here. */
      result = 0;
    }
  else
    {
      if (icon_view->priv->scroll_timeout_id == 0)
	{
	  icon_view->priv->scroll_timeout_id = g_timeout_add (50, drag_scroll_timeout, icon_view);
	  gdk_source_set_static_name_by_id (icon_view->priv->scroll_timeout_id, "[bobgui] drag_scroll_timeout");
	}

      if (target == BOBGUI_TYPE_TREE_ROW_DATA)
        {
          /* Request data so we can use the source row when
           * determining whether to accept the drop
           */
          set_status_pending (drop, suggested_action);
          gdk_drop_read_value_async (drop, target, G_PRIORITY_DEFAULT, NULL, bobgui_icon_view_drag_data_received, icon_view);
        }
      else
        {
          set_status_pending (drop, 0);
        }
      result = suggested_action;
    }

  if (path)
    bobgui_tree_path_free (path);

  return result;
}

static gboolean
bobgui_icon_view_drag_drop (BobguiDropTargetAsync *dest,
                         GdkDrop            *drop,
			 double              x,
			 double              y,
                         BobguiIconView        *icon_view)
{
  BobguiTreePath *path;
  GdkDragAction suggested_action = 0;
  GType target = G_TYPE_INVALID;
  BobguiTreeModel *model;
  gboolean drop_append_mode;

  model = bobgui_icon_view_get_model (icon_view);

  remove_scroll_timeout (icon_view);

  if (!icon_view->priv->dest_set)
    return FALSE;

  if (!check_model_dnd (model, BOBGUI_TYPE_TREE_DRAG_DEST, "drop"))
    return FALSE;

  if (!set_destination (icon_view, drop, dest, x, y, &suggested_action, &target))
    return FALSE;

  path = get_logical_destination (icon_view, &drop_append_mode);

  if (target != G_TYPE_INVALID && path != NULL)
    {
      /* in case a motion had requested drag data, change things so we
       * treat drag data receives as a drop.
       */
      set_status_pending (drop, 0);
      set_dest_row (drop, model, path,
		    icon_view->priv->empty_view_drop, drop_append_mode);
    }

  if (path)
    bobgui_tree_path_free (path);

  /* Unset this thing */
  bobgui_icon_view_set_drag_dest_item (icon_view, NULL, BOBGUI_ICON_VIEW_DROP_LEFT);

  if (target != G_TYPE_INVALID)
    {
      gdk_drop_read_value_async (drop, target, G_PRIORITY_DEFAULT, NULL, bobgui_icon_view_drag_data_received, icon_view);
      return TRUE;
    }
  else
    return FALSE;
}

static void
bobgui_icon_view_drag_data_received (GObject *source,
                                  GAsyncResult *result,
                                  gpointer data)
{
  BobguiIconView *icon_view = data;
  GdkDrop *drop = GDK_DROP (source);
  BobguiTreePath *path;
  BobguiTreeModel *model;
  BobguiTreePath *dest_row;
  GdkDragAction suggested_action;
  gboolean drop_append_mode;
  const GValue *value;

  value = gdk_drop_read_value_finish (drop, result, NULL);
  if (!value)
    return;

  model = bobgui_icon_view_get_model (icon_view);

  if (!check_model_dnd (model, BOBGUI_TYPE_TREE_DRAG_DEST, "drag-data-received"))
    return;

  if (!icon_view->priv->dest_set)
    return;

  suggested_action = get_status_pending (drop);

  if (suggested_action)
    {
      /* We are getting this data due to a request in drag_motion,
       * rather than due to a request in drag_drop, so we are just
       * supposed to call drag_status, not actually paste in the
       * data.
       */
      path = get_logical_destination (icon_view, &drop_append_mode);

      if (path == NULL)
        suggested_action = 0;

      if (suggested_action)
        {
	  if (!bobgui_tree_drag_dest_row_drop_possible (BOBGUI_TREE_DRAG_DEST (model),
						     path,
						     value))
	    suggested_action = 0;
        }

      if (path)
        bobgui_tree_path_free (path);

      /* If you can't drop, remove user drop indicator until the next motion */
      if (suggested_action == 0)
        bobgui_icon_view_set_drag_dest_item (icon_view,
					  NULL,
					  BOBGUI_ICON_VIEW_DROP_LEFT);
      return;
    }


  dest_row = get_dest_row (drop);

  if (dest_row == NULL)
    return;

  suggested_action = bobgui_icon_view_get_action (BOBGUI_WIDGET (icon_view), drop);

  if (suggested_action &&
      !bobgui_tree_drag_dest_drag_data_received (BOBGUI_TREE_DRAG_DEST (model),
                                              dest_row,
                                              value))
    suggested_action = 0;

  gdk_drop_finish (drop, suggested_action);

  bobgui_tree_path_free (dest_row);

  /* drop dest_row */
  set_dest_row (drop, NULL, NULL, FALSE, FALSE);
}

/* Drag-and-Drop support */

/**
 * bobgui_icon_view_enable_model_drag_source:
 * @icon_view: a `BobguiIconView`
 * @start_button_mask: Mask of allowed buttons to start drag
 * @formats: the formats that the drag will support
 * @actions: the bitmask of possible actions for a drag from this
 *    widget
 *
 * Turns @icon_view into a drag source for automatic DND. Calling this
 * method sets `BobguiIconView`:reorderable to %FALSE.
 *
 * Deprecated: 4.10: Use [class@Bobgui.GridView] instead
 **/
void
bobgui_icon_view_enable_model_drag_source (BobguiIconView              *icon_view,
					GdkModifierType           start_button_mask,
                                        GdkContentFormats        *formats,
					GdkDragAction             actions)
{
  g_return_if_fail (BOBGUI_IS_ICON_VIEW (icon_view));

  icon_view->priv->source_formats = gdk_content_formats_ref (formats);
  icon_view->priv->source_actions = actions;

  icon_view->priv->source_set = TRUE;

  unset_reorderable (icon_view);
}

/**
 * bobgui_icon_view_enable_model_drag_dest:
 * @icon_view: a `BobguiIconView`
 * @formats: the formats that the drag will support
 * @actions: the bitmask of possible actions for a drag to this
 *    widget
 *
 * Turns @icon_view into a drop destination for automatic DND. Calling this
 * method sets `BobguiIconView`:reorderable to %FALSE.
 *
 * Deprecated: 4.10: Use [class@Bobgui.GridView] instead
 **/
void
bobgui_icon_view_enable_model_drag_dest (BobguiIconView       *icon_view,
                                      GdkContentFormats *formats,
				      GdkDragAction      actions)
{
  BobguiCssNode *widget_node;

  g_return_if_fail (BOBGUI_IS_ICON_VIEW (icon_view));

  icon_view->priv->dest = bobgui_drop_target_async_new (gdk_content_formats_ref (formats), actions);
  g_signal_connect (icon_view->priv->dest, "drag-leave", G_CALLBACK (bobgui_icon_view_drag_leave), icon_view);
  g_signal_connect (icon_view->priv->dest, "drag-enter", G_CALLBACK (bobgui_icon_view_drag_motion), icon_view);
  g_signal_connect (icon_view->priv->dest, "drag-motion", G_CALLBACK (bobgui_icon_view_drag_motion), icon_view);
  g_signal_connect (icon_view->priv->dest, "drop", G_CALLBACK (bobgui_icon_view_drag_drop), icon_view);
  bobgui_widget_add_controller (BOBGUI_WIDGET (icon_view), BOBGUI_EVENT_CONTROLLER (icon_view->priv->dest));

  icon_view->priv->dest_actions = actions;

  icon_view->priv->dest_set = TRUE;

  unset_reorderable (icon_view);

  widget_node = bobgui_widget_get_css_node (BOBGUI_WIDGET (icon_view));
  icon_view->priv->dndnode = bobgui_css_node_new ();
  bobgui_css_node_set_name (icon_view->priv->dndnode, g_quark_from_static_string ("dndtarget"));
  bobgui_css_node_set_parent (icon_view->priv->dndnode, widget_node);
  bobgui_css_node_set_state (icon_view->priv->dndnode, bobgui_css_node_get_state (widget_node));
  g_object_unref (icon_view->priv->dndnode);
}

/**
 * bobgui_icon_view_unset_model_drag_source:
 * @icon_view: a `BobguiIconView`
 *
 * Undoes the effect of bobgui_icon_view_enable_model_drag_source(). Calling this
 * method sets `BobguiIconView`:reorderable to %FALSE.
 *
 * Deprecated: 4.10: Use [class@Bobgui.GridView] instead
 **/
void
bobgui_icon_view_unset_model_drag_source (BobguiIconView *icon_view)
{
  g_return_if_fail (BOBGUI_IS_ICON_VIEW (icon_view));

  if (icon_view->priv->source_set)
    {
      g_clear_pointer (&icon_view->priv->source_formats, gdk_content_formats_unref);
      icon_view->priv->source_set = FALSE;
    }

  unset_reorderable (icon_view);
}

/**
 * bobgui_icon_view_unset_model_drag_dest:
 * @icon_view: a `BobguiIconView`
 *
 * Undoes the effect of bobgui_icon_view_enable_model_drag_dest(). Calling this
 * method sets `BobguiIconView`:reorderable to %FALSE.
 *
 * Deprecated: 4.10: Use [class@Bobgui.GridView] instead
 **/
void
bobgui_icon_view_unset_model_drag_dest (BobguiIconView *icon_view)
{
  g_return_if_fail (BOBGUI_IS_ICON_VIEW (icon_view));

  if (icon_view->priv->dest_set)
    {
      bobgui_widget_remove_controller (BOBGUI_WIDGET (icon_view), BOBGUI_EVENT_CONTROLLER (icon_view->priv->dest));
      icon_view->priv->dest = NULL;
      icon_view->priv->dest_set = FALSE;

      bobgui_css_node_set_parent (icon_view->priv->dndnode, NULL);
      icon_view->priv->dndnode = NULL;
    }

  unset_reorderable (icon_view);
}

/* These are useful to implement your own custom stuff. */
/**
 * bobgui_icon_view_set_drag_dest_item:
 * @icon_view: a `BobguiIconView`
 * @path: (nullable): The path of the item to highlight
 * @pos: Specifies where to drop, relative to the item
 *
 * Sets the item that is highlighted for feedback.
 *
 * Deprecated: 4.10: Use [class@Bobgui.GridView] instead
 */
void
bobgui_icon_view_set_drag_dest_item (BobguiIconView              *icon_view,
				  BobguiTreePath              *path,
				  BobguiIconViewDropPosition   pos)
{
  /* Note; this function is exported to allow a custom DND
   * implementation, so it can't touch TreeViewDragInfo
   */

  g_return_if_fail (BOBGUI_IS_ICON_VIEW (icon_view));

  if (icon_view->priv->dest_item)
    {
      BobguiTreePath *current_path;
      current_path = bobgui_tree_row_reference_get_path (icon_view->priv->dest_item);
      bobgui_tree_row_reference_free (icon_view->priv->dest_item);
      icon_view->priv->dest_item = NULL;

      bobgui_icon_view_queue_draw_path (icon_view, current_path);
      bobgui_tree_path_free (current_path);
    }

  /* special case a drop on an empty model */
  icon_view->priv->empty_view_drop = FALSE;
  if (pos == BOBGUI_ICON_VIEW_DROP_ABOVE && path
      && bobgui_tree_path_get_depth (path) == 1
      && bobgui_tree_path_get_indices (path)[0] == 0)
    {
      int n_children;

      n_children = bobgui_tree_model_iter_n_children (icon_view->priv->model,
                                                   NULL);

      if (n_children == 0)
        icon_view->priv->empty_view_drop = TRUE;
    }

  icon_view->priv->dest_pos = pos;

  if (path)
    {
      icon_view->priv->dest_item =
        bobgui_tree_row_reference_new_proxy (G_OBJECT (icon_view),
					  icon_view->priv->model, path);

      bobgui_icon_view_queue_draw_path (icon_view, path);
    }
}

/**
 * bobgui_icon_view_get_drag_dest_item:
 * @icon_view: a `BobguiIconView`
 * @path: (out) (nullable) (optional): Return location for the path of
 *   the highlighted item
 * @pos: (out) (optional): Return location for the drop position
 *
 * Gets information about the item that is highlighted for feedback.
 *
 * Deprecated: 4.10: Use [class@Bobgui.GridView] instead
 */
void
bobgui_icon_view_get_drag_dest_item (BobguiIconView              *icon_view,
				  BobguiTreePath             **path,
				  BobguiIconViewDropPosition  *pos)
{
  g_return_if_fail (BOBGUI_IS_ICON_VIEW (icon_view));

  if (path)
    {
      if (icon_view->priv->dest_item)
        *path = bobgui_tree_row_reference_get_path (icon_view->priv->dest_item);
      else
	*path = NULL;
    }

  if (pos)
    *pos = icon_view->priv->dest_pos;
}

/**
 * bobgui_icon_view_get_dest_item_at_pos:
 * @icon_view: a `BobguiIconView`
 * @drag_x: the position to determine the destination item for
 * @drag_y: the position to determine the destination item for
 * @path: (out) (optional): Return location for the path of the item
 * @pos: (out) (optional): Return location for the drop position
 *
 * Determines the destination item for a given position.
 *
 * Returns: whether there is an item at the given position.
 *
 * Deprecated: 4.10: Use [class@Bobgui.GridView] instead
 **/
gboolean
bobgui_icon_view_get_dest_item_at_pos (BobguiIconView              *icon_view,
				    int                       drag_x,
				    int                       drag_y,
				    BobguiTreePath             **path,
				    BobguiIconViewDropPosition  *pos)
{
  BobguiIconViewItem *item;

  /* Note; this function is exported to allow a custom DND
   * implementation, so it can't touch TreeViewDragInfo
   */

  g_return_val_if_fail (BOBGUI_IS_ICON_VIEW (icon_view), FALSE);
  g_return_val_if_fail (drag_x >= 0, FALSE);
  g_return_val_if_fail (drag_y >= 0, FALSE);


  if (path)
    *path = NULL;

  item = _bobgui_icon_view_get_item_at_coords (icon_view,
					   drag_x + bobgui_adjustment_get_value (icon_view->priv->hadjustment),
					   drag_y + bobgui_adjustment_get_value (icon_view->priv->vadjustment),
					   FALSE, NULL);

  if (item == NULL)
    return FALSE;

  if (path)
    *path = bobgui_tree_path_new_from_indices (item->index, -1);

  if (pos)
    {
      if (drag_x < item->cell_area.x + item->cell_area.width / 4)
	*pos = BOBGUI_ICON_VIEW_DROP_LEFT;
      else if (drag_x > item->cell_area.x + item->cell_area.width * 3 / 4)
	*pos = BOBGUI_ICON_VIEW_DROP_RIGHT;
      else if (drag_y < item->cell_area.y + item->cell_area.height / 4)
	*pos = BOBGUI_ICON_VIEW_DROP_ABOVE;
      else if (drag_y > item->cell_area.y + item->cell_area.height * 3 / 4)
	*pos = BOBGUI_ICON_VIEW_DROP_BELOW;
      else
	*pos = BOBGUI_ICON_VIEW_DROP_INTO;
    }

  return TRUE;
}

/**
 * bobgui_icon_view_create_drag_icon:
 * @icon_view: a `BobguiIconView`
 * @path: a `BobguiTreePath` in @icon_view
 *
 * Creates a `GdkPaintable` representation of the item at @path.
 * This image is used for a drag icon.
 *
 * Returns: (transfer full) (nullable): a newly-allocated `GdkPaintable` of the drag icon.
 *
 * Deprecated: 4.10: Use [class@Bobgui.GridView] instead
 **/
GdkPaintable *
bobgui_icon_view_create_drag_icon (BobguiIconView *icon_view,
				BobguiTreePath *path)
{
  BobguiWidget *widget;
  BobguiSnapshot *snapshot;
  GdkPaintable *paintable;
  GList *l;
  int index;

  g_return_val_if_fail (BOBGUI_IS_ICON_VIEW (icon_view), NULL);
  g_return_val_if_fail (path != NULL, NULL);

  widget = BOBGUI_WIDGET (icon_view);

  if (!bobgui_widget_get_realized (widget))
    return NULL;

  index = bobgui_tree_path_get_indices (path)[0];

  for (l = icon_view->priv->items; l; l = l->next)
    {
      BobguiIconViewItem *item = l->data;

      if (index == item->index)
        {
          snapshot = bobgui_snapshot_new ();
          bobgui_icon_view_snapshot_item (icon_view, snapshot, item,
                                       icon_view->priv->item_padding,
                                       icon_view->priv->item_padding,
                                       FALSE);
          paintable = bobgui_snapshot_free_to_paintable (snapshot, NULL);

          return paintable;
       }
    }

  return NULL;
}

/**
 * bobgui_icon_view_get_reorderable:
 * @icon_view: a `BobguiIconView`
 *
 * Retrieves whether the user can reorder the list via drag-and-drop.
 * See bobgui_icon_view_set_reorderable().
 *
 * Returns: %TRUE if the list can be reordered.
 *
 * Deprecated: 4.10: Use [class@Bobgui.GridView] instead
 **/
gboolean
bobgui_icon_view_get_reorderable (BobguiIconView *icon_view)
{
  g_return_val_if_fail (BOBGUI_IS_ICON_VIEW (icon_view), FALSE);

  return icon_view->priv->reorderable;
}

/**
 * bobgui_icon_view_set_reorderable:
 * @icon_view: A `BobguiIconView`.
 * @reorderable: %TRUE, if the list of items can be reordered.
 *
 * This function is a convenience function to allow you to reorder models that
 * support the `BobguiTreeDragSourceIface` and the `BobguiTreeDragDestIface`. Both
 * `BobguiTreeStore` and `BobguiListStore` support these. If @reorderable is %TRUE, then
 * the user can reorder the model by dragging and dropping rows.  The
 * developer can listen to these changes by connecting to the model's
 * row_inserted and row_deleted signals. The reordering is implemented by setting up
 * the icon view as a drag source and destination. Therefore, drag and
 * drop can not be used in a reorderable view for any other purpose.
 *
 * This function does not give you any degree of control over the order -- any
 * reordering is allowed.  If more control is needed, you should probably
 * handle drag and drop manually.
 *
 * Deprecated: 4.10: Use [class@Bobgui.GridView] instead
 **/
void
bobgui_icon_view_set_reorderable (BobguiIconView *icon_view,
			       gboolean     reorderable)
{
  g_return_if_fail (BOBGUI_IS_ICON_VIEW (icon_view));

  reorderable = reorderable != FALSE;

  if (icon_view->priv->reorderable == reorderable)
    return;

  if (reorderable)
    {
      GdkContentFormats *formats = gdk_content_formats_new_for_gtype (BOBGUI_TYPE_TREE_ROW_DATA);
      bobgui_icon_view_enable_model_drag_source (icon_view,
					      GDK_BUTTON1_MASK,
					      formats,
					      GDK_ACTION_MOVE);
      bobgui_icon_view_enable_model_drag_dest (icon_view,
					    formats,
					    GDK_ACTION_MOVE);
      gdk_content_formats_unref (formats);
    }
  else
    {
      bobgui_icon_view_unset_model_drag_source (icon_view);
      bobgui_icon_view_unset_model_drag_dest (icon_view);
    }

  icon_view->priv->reorderable = reorderable;

  g_object_notify (G_OBJECT (icon_view), "reorderable");
}

/**
 * bobgui_icon_view_set_activate_on_single_click:
 * @icon_view: a `BobguiIconView`
 * @single: %TRUE to emit item-activated on a single click
 *
 * Causes the `BobguiIconView`::item-activated signal to be emitted on
 * a single click instead of a double click.
 *
 * Deprecated: 4.10: Use [class@Bobgui.GridView] instead
 **/
void
bobgui_icon_view_set_activate_on_single_click (BobguiIconView *icon_view,
                                            gboolean     single)
{
  g_return_if_fail (BOBGUI_IS_ICON_VIEW (icon_view));

  single = single != FALSE;

  if (icon_view->priv->activate_on_single_click == single)
    return;

  icon_view->priv->activate_on_single_click = single;
  g_object_notify (G_OBJECT (icon_view), "activate-on-single-click");
}

/**
 * bobgui_icon_view_get_activate_on_single_click:
 * @icon_view: a `BobguiIconView`
 *
 * Gets the setting set by bobgui_icon_view_set_activate_on_single_click().
 *
 * Returns: %TRUE if item-activated will be emitted on a single click
 *
 * Deprecated: 4.10: Use [class@Bobgui.GridView] instead
 **/
gboolean
bobgui_icon_view_get_activate_on_single_click (BobguiIconView *icon_view)
{
  g_return_val_if_fail (BOBGUI_IS_ICON_VIEW (icon_view), FALSE);

  return icon_view->priv->activate_on_single_click;
}

static gboolean
bobgui_icon_view_buildable_custom_tag_start (BobguiBuildable       *buildable,
                                          BobguiBuilder         *builder,
                                          GObject            *child,
                                          const char         *tagname,
                                          BobguiBuildableParser *parser,
                                          gpointer           *data)
{
  if (parent_buildable_iface->custom_tag_start (buildable, builder, child,
                                                tagname, parser, data))
    return TRUE;

  return _bobgui_cell_layout_buildable_custom_tag_start (buildable, builder, child,
                                                      tagname, parser, data);
}

static void
bobgui_icon_view_buildable_custom_tag_end (BobguiBuildable *buildable,
                                        BobguiBuilder   *builder,
                                        GObject      *child,
                                        const char   *tagname,
                                        gpointer      data)
{
  if (!_bobgui_cell_layout_buildable_custom_tag_end (buildable, builder,
                                                  child, tagname, data))
    parent_buildable_iface->custom_tag_end (buildable, builder,
                                            child, tagname, data);
}
