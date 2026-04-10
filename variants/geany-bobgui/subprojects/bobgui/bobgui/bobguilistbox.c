/*
 * Copyright (C) 2012 Alexander Larsson <alexl@redhat.com>
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

#include "bobguilistbox.h"

#include "bobguiaccessible.h"
#include "bobguiactionhelperprivate.h"
#include "bobguiadjustmentprivate.h"
#include "bobguibinlayout.h"
#include "bobguibuildable.h"
#include "bobguigestureclick.h"
#include "bobguimain.h"
#include "bobguimarshalers.h"
#include "bobguiprivate.h"
#include "bobguiscrollable.h"
#include "bobguitypebuiltins.h"
#include "bobguiwidgetprivate.h"
#include "bobguisizerequest.h"

#include <float.h>
#include <math.h>
#include <string.h>

/**
 * BobguiListBox:
 *
 * Shows a vertical list.
 *
 * <picture>
 *   <source srcset="list-box-dark.png" media="(prefers-color-scheme: dark)">
 *   <img alt="An example BobguiListBox" src="list-box.png">
 * </picture>
 *
 * A `BobguiListBox` only contains `BobguiListBoxRow` children. These rows can
 * by dynamically sorted and filtered, and headers can be added dynamically
 * depending on the row content. It also allows keyboard and mouse navigation
 * and selection like a typical list.
 *
 * Using `BobguiListBox` is often an alternative to `BobguiTreeView`, especially
 * when the list contents has a more complicated layout than what is allowed
 * by a `BobguiCellRenderer`, or when the contents is interactive (i.e. has a
 * button in it).
 *
 * Although a `BobguiListBox` must have only `BobguiListBoxRow` children, you can
 * add any kind of widget to it via [method@Bobgui.ListBox.prepend],
 * [method@Bobgui.ListBox.append] and [method@Bobgui.ListBox.insert] and a
 * `BobguiListBoxRow` widget will automatically be inserted between the list
 * and the widget.
 *
 * `BobguiListBoxRows` can be marked as activatable or selectable. If a row is
 * activatable, [signal@Bobgui.ListBox::row-activated] will be emitted for it when
 * the user tries to activate it. If it is selectable, the row will be marked
 * as selected when the user tries to select it.
 *
 * # BobguiListBox as BobguiBuildable
 *
 * The `BobguiListBox` implementation of the `BobguiBuildable` interface supports
 * setting a child as the placeholder by specifying “placeholder” as the “type”
 * attribute of a `<child>` element. See [method@Bobgui.ListBox.set_placeholder]
 * for info.
 *
 * # Shortcuts and Gestures
 *
 * The following signals have default keybindings:
 *
 * - [signal@Bobgui.ListBox::move-cursor]
 * - [signal@Bobgui.ListBox::select-all]
 * - [signal@Bobgui.ListBox::toggle-cursor-row]
 * - [signal@Bobgui.ListBox::unselect-all]
 *
 * # CSS nodes
 *
 * ```
 * list[.separators][.rich-list][.navigation-sidebar][.boxed-list]
 * ╰── row[.activatable]
 * ```
 *
 * `BobguiListBox` uses a single CSS node named list. It may carry the .separators
 * style class, when the [property@Bobgui.ListBox:show-separators] property is set.
 * Each `BobguiListBoxRow` uses a single CSS node named row. The row nodes get the
 * .activatable style class added when appropriate.
 *
 * It may also carry the .boxed-list style class. In this case, the list will be
 * automatically surrounded by a frame and have separators.
 *
 * The main list node may also carry style classes to select
 * the style of [list presentation](section-list-widget.html#list-styles):
 * .rich-list, .navigation-sidebar or .data-table.
 *
 * # Accessibility
 *
 * `BobguiListBox` uses the [enum@Bobgui.AccessibleRole.list] role and `BobguiListBoxRow` uses
 * the [enum@Bobgui.AccessibleRole.list_item] role.
 */

/**
 * BobguiListBoxRow:
 *
 * The kind of widget that can be added to a `BobguiListBox`.
 *
 * [class@Bobgui.ListBox] will automatically wrap its children in a `BobguiListboxRow`
 * when necessary.
 */

typedef struct _BobguiListBoxClass   BobguiListBoxClass;

struct _BobguiListBox
{
  BobguiWidget parent_instance;

  GSequence *children;
  GHashTable *header_hash;

  BobguiWidget *placeholder;

  BobguiListBoxSortFunc sort_func;
  gpointer sort_func_target;
  GDestroyNotify sort_func_target_destroy_notify;

  BobguiListBoxFilterFunc filter_func;
  gpointer filter_func_target;
  GDestroyNotify filter_func_target_destroy_notify;

  BobguiListBoxUpdateHeaderFunc update_header_func;
  gpointer update_header_func_target;
  GDestroyNotify update_header_func_target_destroy_notify;

  BobguiListBoxRow *selected_row;
  BobguiListBoxRow *cursor_row;

  BobguiListBoxRow *active_row;

  BobguiSelectionMode selection_mode;

  gulong adjustment_changed_id;
  BobguiWidget *scrollable_parent;
  BobguiAdjustment *adjustment;
  gboolean activate_single_click;
  gboolean accept_unpaired_release;
  gboolean show_separators;

  /* DnD */
  BobguiListBoxRow *drag_highlighted_row;

  int n_visible_rows;

  GListModel *bound_model;
  BobguiListBoxCreateWidgetFunc create_widget_func;
  gpointer create_widget_func_data;
  GDestroyNotify create_widget_func_data_destroy;

  BobguiListTabBehavior tab_behavior;
};

struct _BobguiListBoxClass
{
  BobguiWidgetClass parent_class;

  void (*row_selected)        (BobguiListBox      *box,
                               BobguiListBoxRow   *row);
  void (*row_activated)       (BobguiListBox      *box,
                               BobguiListBoxRow   *row);
  void (*activate_cursor_row) (BobguiListBox      *box);
  void (*toggle_cursor_row)   (BobguiListBox      *box);
  void (*move_cursor)         (BobguiListBox      *box,
                               BobguiMovementStep  step,
                               int              count,
                               gboolean         extend,
                               gboolean         modify);
  void (*selected_rows_changed) (BobguiListBox    *box);
  void (*select_all)            (BobguiListBox    *box);
  void (*unselect_all)          (BobguiListBox    *box);
};

typedef struct
{
  BobguiWidget *child;
  GSequenceIter *iter;
  BobguiWidget *header;
  BobguiActionHelper *action_helper;
  int y;
  int height;
  guint visible     :1;
  guint selected    :1;
  guint activatable :1;
  guint selectable  :1;
} BobguiListBoxRowPrivate;

enum {
  ROW_SELECTED,
  ROW_ACTIVATED,
  ACTIVATE_CURSOR_ROW,
  TOGGLE_CURSOR_ROW,
  MOVE_CURSOR,
  SELECTED_ROWS_CHANGED,
  SELECT_ALL,
  UNSELECT_ALL,
  LAST_SIGNAL
};

enum {
  ROW__ACTIVATE,
  ROW__LAST_SIGNAL
};

enum {
  PROP_0,
  PROP_SELECTION_MODE,
  PROP_ACTIVATE_ON_SINGLE_CLICK,
  PROP_ACCEPT_UNPAIRED_RELEASE,
  PROP_SHOW_SEPARATORS,
  PROP_TAB_BEHAVIOR,
  LAST_PROPERTY
};

enum {
  ROW_PROP_0,
  ROW_PROP_ACTIVATABLE,
  ROW_PROP_SELECTABLE,
  ROW_PROP_CHILD,

  /* actionable properties */
  ROW_PROP_ACTION_NAME,
  ROW_PROP_ACTION_TARGET,

  LAST_ROW_PROPERTY = ROW_PROP_ACTION_NAME
};

#define ROW_PRIV(row) ((BobguiListBoxRowPrivate*)bobgui_list_box_row_get_instance_private ((BobguiListBoxRow*)(row)))

static BobguiBuildableIface *parent_buildable_iface;

static void     bobgui_list_box_buildable_interface_init   (BobguiBuildableIface *iface);

static void     bobgui_list_box_row_buildable_iface_init (BobguiBuildableIface *iface);
static void     bobgui_list_box_row_actionable_iface_init  (BobguiActionableInterface *iface);

G_DEFINE_TYPE_WITH_CODE (BobguiListBox, bobgui_list_box, BOBGUI_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_BUILDABLE,
                                                bobgui_list_box_buildable_interface_init))
G_DEFINE_TYPE_WITH_CODE (BobguiListBoxRow, bobgui_list_box_row, BOBGUI_TYPE_WIDGET,
                         G_ADD_PRIVATE (BobguiListBoxRow)
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_BUILDABLE,
                                                bobgui_list_box_row_buildable_iface_init )
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_ACTIONABLE, bobgui_list_box_row_actionable_iface_init))

static void                 bobgui_list_box_apply_filter_all             (BobguiListBox          *box);
static void                 bobgui_list_box_update_header                (BobguiListBox          *box,
                                                                       GSequenceIter       *iter);
static GSequenceIter *      bobgui_list_box_get_next_visible             (BobguiListBox          *box,
                                                                       GSequenceIter       *iter);
static void                 bobgui_list_box_apply_filter                 (BobguiListBox          *box,
                                                                       BobguiListBoxRow       *row);
static void                 bobgui_list_box_add_move_binding             (BobguiWidgetClass      *widget_class,
                                                                       guint                keyval,
                                                                       GdkModifierType      modmask,
                                                                       BobguiMovementStep      step,
                                                                       int                  count);
static void                 bobgui_list_box_update_cursor                (BobguiListBox          *box,
                                                                       BobguiListBoxRow       *row,
                                                                       gboolean             grab_focus);
static void                 bobgui_list_box_show                         (BobguiWidget           *widget);
static gboolean             bobgui_list_box_focus                        (BobguiWidget           *widget,
                                                                       BobguiDirectionType     direction);
static GSequenceIter*       bobgui_list_box_get_previous_visible         (BobguiListBox          *box,
                                                                       GSequenceIter       *iter);
static BobguiListBoxRow       *bobgui_list_box_get_first_focusable          (BobguiListBox          *box);
static BobguiListBoxRow       *bobgui_list_box_get_last_focusable           (BobguiListBox          *box);
static void                 bobgui_list_box_compute_expand               (BobguiWidget           *widget,
                                                                       gboolean            *hexpand,
                                                                       gboolean            *vexpand);
static BobguiSizeRequestMode   bobgui_list_box_get_request_mode             (BobguiWidget           *widget);
static void                 bobgui_list_box_size_allocate                (BobguiWidget           *widget,
                                                                       int                  width,
                                                                       int                  height,
                                                                       int                  baseline);
static void                 bobgui_list_box_activate_cursor_row          (BobguiListBox          *box);
static void                 bobgui_list_box_toggle_cursor_row            (BobguiListBox          *box);
static void                 bobgui_list_box_move_cursor                  (BobguiListBox          *box,
                                                                       BobguiMovementStep      step,
                                                                       int                  count,
                                                                       gboolean             extend,
                                                                       gboolean             modify);
static void                 bobgui_list_box_parent_cb                    (GObject             *object,
                                                                       GParamSpec          *pspec,
                                                                       gpointer             user_data);
static void                 bobgui_list_box_select_row_internal            (BobguiListBox          *box,
                                                                         BobguiListBoxRow       *row);
static void                 bobgui_list_box_unselect_row_internal          (BobguiListBox          *box,
                                                                         BobguiListBoxRow       *row);
static void                 bobgui_list_box_select_all_between             (BobguiListBox          *box,
                                                                         BobguiListBoxRow       *row1,
                                                                         BobguiListBoxRow       *row2,
                                                                         gboolean             modify);
static gboolean             bobgui_list_box_unselect_all_internal          (BobguiListBox          *box);
static void                 bobgui_list_box_selected_rows_changed          (BobguiListBox          *box);
static void                 bobgui_list_box_set_accept_unpaired_release    (BobguiListBox          *box,
                                                                         gboolean             accept);

static void bobgui_list_box_click_gesture_pressed  (BobguiGestureClick  *gesture,
                                                 guint             n_press,
                                                 double            x,
                                                 double            y,
                                                 BobguiListBox       *box);
static void bobgui_list_box_click_gesture_released (BobguiGestureClick  *gesture,
                                                 guint             n_press,
                                                 double            x,
                                                 double            y,
                                                 BobguiListBox       *box);
static void bobgui_list_box_click_unpaired_release (BobguiGestureClick  *gesture,
                                                 double            x,
                                                 double            y,
                                                 guint             button,
                                                 GdkEventSequence *sequence,
                                                 BobguiListBox       *box);
static void bobgui_list_box_click_gesture_stopped  (BobguiGestureClick  *gesture,
                                                 BobguiListBox       *box);

static void bobgui_list_box_update_row        (BobguiListBox    *box,
                                            BobguiListBoxRow *row);
static void bobgui_list_box_update_rows       (BobguiListBox    *box);

static void                 bobgui_list_box_bound_model_changed            (GListModel          *list,
                                                                         guint                position,
                                                                         guint                removed,
                                                                         guint                added,
                                                                         gpointer             user_data);

static void                 bobgui_list_box_check_model_compat             (BobguiListBox          *box);

static void bobgui_list_box_measure (BobguiWidget     *widget,
                                  BobguiOrientation  orientation,
                                  int             for_size,
                                  int            *minimum,
                                  int            *natural,
                                  int            *minimum_baseline,
                                  int            *natural_baseline);



static GParamSpec *properties[LAST_PROPERTY] = { NULL, };
static guint signals[LAST_SIGNAL] = { 0 };
static GParamSpec *row_properties[LAST_ROW_PROPERTY] = { NULL, };
static guint row_signals[ROW__LAST_SIGNAL] = { 0 };


static BobguiBuildableIface *parent_row_buildable_iface;

static void
bobgui_list_box_row_buildable_add_child (BobguiBuildable *buildable,
                                      BobguiBuilder   *builder,
                                      GObject      *child,
                                      const char   *type)
{
  if (BOBGUI_IS_WIDGET (child))
    bobgui_list_box_row_set_child (BOBGUI_LIST_BOX_ROW (buildable), BOBGUI_WIDGET (child));
  else
    parent_row_buildable_iface->add_child (buildable, builder, child, type);
}

static void
bobgui_list_box_row_buildable_iface_init (BobguiBuildableIface *iface)
{
  parent_row_buildable_iface = g_type_interface_peek_parent (iface);

  iface->add_child = bobgui_list_box_row_buildable_add_child;
}

/**
 * bobgui_list_box_new:
 *
 * Creates a new `BobguiListBox` container.
 *
 * Returns: a new `BobguiListBox`
 */
BobguiWidget *
bobgui_list_box_new (void)
{
  return g_object_new (BOBGUI_TYPE_LIST_BOX, NULL);
}

static void
bobgui_list_box_get_property (GObject    *obj,
                           guint       property_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  BobguiListBox *box = BOBGUI_LIST_BOX (obj);

  switch (property_id)
    {
    case PROP_SELECTION_MODE:
      g_value_set_enum (value, box->selection_mode);
      break;
    case PROP_ACTIVATE_ON_SINGLE_CLICK:
      g_value_set_boolean (value, box->activate_single_click);
      break;
    case PROP_ACCEPT_UNPAIRED_RELEASE:
      g_value_set_boolean (value, box->accept_unpaired_release);
      break;
    case PROP_SHOW_SEPARATORS:
      g_value_set_boolean (value, box->show_separators);
      break;
    case PROP_TAB_BEHAVIOR:
      g_value_set_enum (value, box->tab_behavior);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, property_id, pspec);
      break;
    }
}

static void
bobgui_list_box_set_property (GObject      *obj,
                           guint         property_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  BobguiListBox *box = BOBGUI_LIST_BOX (obj);

  switch (property_id)
    {
    case PROP_SELECTION_MODE:
      bobgui_list_box_set_selection_mode (box, g_value_get_enum (value));
      break;
    case PROP_ACTIVATE_ON_SINGLE_CLICK:
      bobgui_list_box_set_activate_on_single_click (box, g_value_get_boolean (value));
      break;
    case PROP_ACCEPT_UNPAIRED_RELEASE:
      bobgui_list_box_set_accept_unpaired_release (box, g_value_get_boolean (value));
      break;
    case PROP_SHOW_SEPARATORS:
      bobgui_list_box_set_show_separators (box, g_value_get_boolean (value));
      break;
    case PROP_TAB_BEHAVIOR:
      bobgui_list_box_set_tab_behavior (box, g_value_get_enum (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, property_id, pspec);
      break;
    }
}

static void
bobgui_list_box_dispose (GObject *object)
{
  BobguiListBox *self = BOBGUI_LIST_BOX (object);

  if (self->bound_model)
    {
      if (self->create_widget_func_data_destroy)
        self->create_widget_func_data_destroy (self->create_widget_func_data);

      g_signal_handlers_disconnect_by_func (self->bound_model, bobgui_list_box_bound_model_changed, self);
      g_clear_object (&self->bound_model);
    }

  bobgui_list_box_remove_all (self);

  G_OBJECT_CLASS (bobgui_list_box_parent_class)->dispose (object);
}

static void
bobgui_list_box_finalize (GObject *obj)
{
  BobguiListBox *box = BOBGUI_LIST_BOX (obj);

  if (box->sort_func_target_destroy_notify != NULL)
    box->sort_func_target_destroy_notify (box->sort_func_target);
  if (box->filter_func_target_destroy_notify != NULL)
    box->filter_func_target_destroy_notify (box->filter_func_target);
  if (box->update_header_func_target_destroy_notify != NULL)
    box->update_header_func_target_destroy_notify (box->update_header_func_target);

  g_clear_object (&box->adjustment);
  g_clear_object (&box->drag_highlighted_row);

  g_sequence_free (box->children);
  g_hash_table_unref (box->header_hash);

  G_OBJECT_CLASS (bobgui_list_box_parent_class)->finalize (obj);
}

static void
bobgui_list_box_class_init (BobguiListBoxClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);

  object_class->get_property = bobgui_list_box_get_property;
  object_class->set_property = bobgui_list_box_set_property;
  object_class->dispose = bobgui_list_box_dispose;
  object_class->finalize = bobgui_list_box_finalize;

  widget_class->show = bobgui_list_box_show;
  widget_class->focus = bobgui_list_box_focus;
  widget_class->grab_focus = bobgui_widget_grab_focus_self;
  widget_class->compute_expand = bobgui_list_box_compute_expand;
  widget_class->get_request_mode = bobgui_list_box_get_request_mode;
  widget_class->measure = bobgui_list_box_measure;
  widget_class->size_allocate = bobgui_list_box_size_allocate;
  klass->activate_cursor_row = bobgui_list_box_activate_cursor_row;
  klass->toggle_cursor_row = bobgui_list_box_toggle_cursor_row;
  klass->move_cursor = bobgui_list_box_move_cursor;
  klass->select_all = bobgui_list_box_select_all;
  klass->unselect_all = bobgui_list_box_unselect_all;
  klass->selected_rows_changed = bobgui_list_box_selected_rows_changed;

  /**
   * BobguiListBox:selection-mode:
   *
   * The selection mode used by the list box.
   */
  properties[PROP_SELECTION_MODE] =
    g_param_spec_enum ("selection-mode", NULL, NULL,
                       BOBGUI_TYPE_SELECTION_MODE,
                       BOBGUI_SELECTION_SINGLE,
                       G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

    /**
   * BobguiListBox:activate-on-single-click:
   *
   * Determines whether children can be activated with a single
   * click, or require a double-click.
   */
  properties[PROP_ACTIVATE_ON_SINGLE_CLICK] =
    g_param_spec_boolean ("activate-on-single-click", NULL, NULL,
                          TRUE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiListBox:accept-unpaired-release:
   *
   * Whether to accept unpaired release events.
   */
  properties[PROP_ACCEPT_UNPAIRED_RELEASE] =
    g_param_spec_boolean ("accept-unpaired-release", NULL, NULL,
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);


  /**
   * BobguiListBox:show-separators:
   *
   * Whether to show separators between rows.
   */
  properties[PROP_SHOW_SEPARATORS] =
    g_param_spec_boolean ("show-separators", NULL, NULL,
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

                          /**
                           * BobguiListBox:tab-behavior:
                           * 
                           * Behavior of the <kbd>Tab</kbd> key
                           *
                           * Since: 4.18 
                           */
                          properties[PROP_TAB_BEHAVIOR] =
                            g_param_spec_enum ("tab-behavior", NULL, NULL,
                                               BOBGUI_TYPE_LIST_TAB_BEHAVIOR,
                                               BOBGUI_LIST_TAB_ALL,
                                               G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, LAST_PROPERTY, properties);

  /**
   * BobguiListBox::row-selected:
   * @box: the `BobguiListBox`
   * @row: (nullable): the selected row
   *
   * Emitted when a new row is selected, or (with a %NULL @row)
   * when the selection is cleared.
   *
   * When the @box is using %BOBGUI_SELECTION_MULTIPLE, this signal will not
   * give you the full picture of selection changes, and you should use
   * the [signal@Bobgui.ListBox::selected-rows-changed] signal instead.
   */
  signals[ROW_SELECTED] =
    g_signal_new (I_("row-selected"),
                  BOBGUI_TYPE_LIST_BOX,
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BobguiListBoxClass, row_selected),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 1,
                  BOBGUI_TYPE_LIST_BOX_ROW);

  /**
   * BobguiListBox::selected-rows-changed:
   * @box: the `BobguiListBox` on which the signal is emitted
   *
   * Emitted when the set of selected rows changes.
   */
  signals[SELECTED_ROWS_CHANGED] = g_signal_new (I_("selected-rows-changed"),
                                                 BOBGUI_TYPE_LIST_BOX,
                                                 G_SIGNAL_RUN_FIRST,
                                                 G_STRUCT_OFFSET (BobguiListBoxClass, selected_rows_changed),
                                                 NULL, NULL,
                                                 NULL,
                                                 G_TYPE_NONE, 0);

  /**
   * BobguiListBox::select-all:
   * @box: the `BobguiListBox` on which the signal is emitted
   *
   * Emitted to select all children of the box, if the selection
   * mode permits it.
   *
   * This is a [keybinding signal](class.SignalAction.html).
   *
   * The default binding for this signal is <kbd>Ctrl</kbd>-<kbd>a</kbd>.
   */
  signals[SELECT_ALL] = g_signal_new (I_("select-all"),
                                      BOBGUI_TYPE_LIST_BOX,
                                      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                                      G_STRUCT_OFFSET (BobguiListBoxClass, select_all),
                                      NULL, NULL,
                                      NULL,
                                      G_TYPE_NONE, 0);

  /**
   * BobguiListBox::unselect-all:
   * @box: the `BobguiListBox` on which the signal is emitted
   *
   * Emitted to unselect all children of the box, if the selection
   * mode permits it.
   *
   * This is a [keybinding signal](class.SignalAction.html).
   *
   * The default binding for this signal is
   * <kbd>Ctrl</kbd>-<kbd>Shift</kbd>-<kbd>a</kbd>.
   */
  signals[UNSELECT_ALL] = g_signal_new (I_("unselect-all"),
                                        BOBGUI_TYPE_LIST_BOX,
                                        G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                                        G_STRUCT_OFFSET (BobguiListBoxClass, unselect_all),
                                        NULL, NULL,
                                        NULL,
                                        G_TYPE_NONE, 0);

  /**
   * BobguiListBox::row-activated:
   * @box: the `BobguiListBox`
   * @row: the activated row
   *
   * Emitted when a row has been activated by the user.
   */
  signals[ROW_ACTIVATED] =
    g_signal_new (I_("row-activated"),
                  BOBGUI_TYPE_LIST_BOX,
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BobguiListBoxClass, row_activated),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 1,
                  BOBGUI_TYPE_LIST_BOX_ROW);

  /**
   * BobguiListBox::activate-cursor-row:
   * @box: the list box
   *
   * Emitted when the cursor row is activated.
   */
  signals[ACTIVATE_CURSOR_ROW] =
    g_signal_new (I_("activate-cursor-row"),
                  BOBGUI_TYPE_LIST_BOX,
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (BobguiListBoxClass, activate_cursor_row),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);

  /**
   * BobguiListBox::toggle-cursor-row:
   * @box: the list box
   *
   * Emitted when the cursor row is toggled.
   *
   * The default bindings for this signal is <kbd>Ctrl</kbd>+<kbd>␣</kbd>.
   */
  signals[TOGGLE_CURSOR_ROW] =
    g_signal_new (I_("toggle-cursor-row"),
                  BOBGUI_TYPE_LIST_BOX,
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (BobguiListBoxClass, toggle_cursor_row),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);
  /**
   * BobguiListBox::move-cursor:
   * @box: the list box on which the signal is emitted
   * @step: the granularity of the move, as a `BobguiMovementStep`
   * @count: the number of @step units to move
   * @extend: whether to extend the selection
   * @modify: whether to modify the selection
   *
   * Emitted when the user initiates a cursor movement.
   *
   * The default bindings for this signal come in two variants, the variant with
   * the Shift modifier extends the selection, the variant without the Shift
   * modifier does not. There are too many key combinations to list them all
   * here.
   *
   * - <kbd>←</kbd>, <kbd>→</kbd>, <kbd>↑</kbd>, <kbd>↓</kbd>
   *   move by individual children
   * - <kbd>Home</kbd>, <kbd>End</kbd> move to the ends of the box
   * - <kbd>PgUp</kbd>, <kbd>PgDn</kbd> move vertically by pages
   */
  signals[MOVE_CURSOR] =
    g_signal_new (I_("move-cursor"),
                  BOBGUI_TYPE_LIST_BOX,
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (BobguiListBoxClass, move_cursor),
                  NULL, NULL,
                  _bobgui_marshal_VOID__ENUM_INT_BOOLEAN_BOOLEAN,
                  G_TYPE_NONE, 4,
                  BOBGUI_TYPE_MOVEMENT_STEP, G_TYPE_INT, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN);
  g_signal_set_va_marshaller (signals[MOVE_CURSOR],
                              G_TYPE_FROM_CLASS (klass),
                              _bobgui_marshal_VOID__ENUM_INT_BOOLEAN_BOOLEANv);

  bobgui_widget_class_set_activate_signal (widget_class, signals[ACTIVATE_CURSOR_ROW]);

  bobgui_list_box_add_move_binding (widget_class, GDK_KEY_Home, 0,
                                 BOBGUI_MOVEMENT_BUFFER_ENDS, -1);
  bobgui_list_box_add_move_binding (widget_class, GDK_KEY_KP_Home, 0,
                                 BOBGUI_MOVEMENT_BUFFER_ENDS, -1);
  bobgui_list_box_add_move_binding (widget_class, GDK_KEY_End, 0,
                                 BOBGUI_MOVEMENT_BUFFER_ENDS, 1);
  bobgui_list_box_add_move_binding (widget_class, GDK_KEY_KP_End, 0,
                                 BOBGUI_MOVEMENT_BUFFER_ENDS, 1);
  bobgui_list_box_add_move_binding (widget_class, GDK_KEY_Up, 0,
                                 BOBGUI_MOVEMENT_DISPLAY_LINES, -1);
  bobgui_list_box_add_move_binding (widget_class, GDK_KEY_KP_Up, 0,
                                 BOBGUI_MOVEMENT_DISPLAY_LINES, -1);
  bobgui_list_box_add_move_binding (widget_class, GDK_KEY_Down, 0,
                                 BOBGUI_MOVEMENT_DISPLAY_LINES, 1);
  bobgui_list_box_add_move_binding (widget_class, GDK_KEY_KP_Down, 0,
                                 BOBGUI_MOVEMENT_DISPLAY_LINES, 1);
  bobgui_list_box_add_move_binding (widget_class, GDK_KEY_Page_Up, 0,
                                 BOBGUI_MOVEMENT_PAGES, -1);
  bobgui_list_box_add_move_binding (widget_class, GDK_KEY_KP_Page_Up, 0,
                                 BOBGUI_MOVEMENT_PAGES, -1);
  bobgui_list_box_add_move_binding (widget_class, GDK_KEY_Page_Down, 0,
                                 BOBGUI_MOVEMENT_PAGES, 1);
  bobgui_list_box_add_move_binding (widget_class, GDK_KEY_KP_Page_Down, 0,
                                 BOBGUI_MOVEMENT_PAGES, 1);

  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_space, GDK_CONTROL_MASK,
                                       "toggle-cursor-row",
                                       NULL);
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_KP_Space, GDK_CONTROL_MASK,
                                       "toggle-cursor-row",
                                       NULL);

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

  bobgui_widget_class_set_css_name (widget_class, I_("list"));
  bobgui_widget_class_set_accessible_role (widget_class, BOBGUI_ACCESSIBLE_ROLE_LIST);
}

static void
bobgui_list_box_init (BobguiListBox *box)
{
  BobguiWidget *widget = BOBGUI_WIDGET (box);
  BobguiGesture *gesture;

  bobgui_widget_set_focusable (BOBGUI_WIDGET (box), TRUE);

  box->selection_mode = BOBGUI_SELECTION_SINGLE;
  box->activate_single_click = TRUE;

  box->children = g_sequence_new (NULL);
  box->header_hash = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, NULL);

  gesture = bobgui_gesture_click_new ();
  bobgui_event_controller_set_propagation_phase (BOBGUI_EVENT_CONTROLLER (gesture),
                                              BOBGUI_PHASE_BUBBLE);
  bobgui_gesture_single_set_touch_only (BOBGUI_GESTURE_SINGLE (gesture),
                                     FALSE);
  bobgui_gesture_single_set_button (BOBGUI_GESTURE_SINGLE (gesture),
                                 GDK_BUTTON_PRIMARY);
  g_signal_connect (gesture, "pressed",
                    G_CALLBACK (bobgui_list_box_click_gesture_pressed), box);
  g_signal_connect (gesture, "released",
                    G_CALLBACK (bobgui_list_box_click_gesture_released), box);
  g_signal_connect (gesture, "stopped",
                    G_CALLBACK (bobgui_list_box_click_gesture_stopped), box);
  g_signal_connect (gesture, "unpaired-release",
                    G_CALLBACK (bobgui_list_box_click_unpaired_release), box);
  bobgui_widget_add_controller (widget, BOBGUI_EVENT_CONTROLLER (gesture));

  g_signal_connect (box, "notify::parent", G_CALLBACK (bobgui_list_box_parent_cb), NULL);
}

/**
 * bobgui_list_box_get_selected_row:
 * @box: a `BobguiListBox`
 *
 * Gets the selected row, or %NULL if no rows are selected.
 *
 * Note that the box may allow multiple selection, in which
 * case you should use [method@Bobgui.ListBox.selected_foreach] to
 * find all selected rows.
 *
 * Returns: (transfer none) (nullable): the selected row
 */
BobguiListBoxRow *
bobgui_list_box_get_selected_row (BobguiListBox *box)
{
  g_return_val_if_fail (BOBGUI_IS_LIST_BOX (box), NULL);

  return box->selected_row;
}

/**
 * bobgui_list_box_get_row_at_index:
 * @box: a `BobguiListBox`
 * @index_: the index of the row
 *
 * Gets the n-th child in the list (not counting headers).
 *
 * If @index_ is negative or larger than the number of items in the
 * list, %NULL is returned.
 *
 * Returns: (transfer none) (nullable): the child `BobguiWidget`
 */
BobguiListBoxRow *
bobgui_list_box_get_row_at_index (BobguiListBox *box,
                               int         index_)
{
  GSequenceIter *iter;

  g_return_val_if_fail (BOBGUI_IS_LIST_BOX (box), NULL);

  iter = g_sequence_get_iter_at_pos (box->children, index_);
  if (!g_sequence_iter_is_end (iter))
    return g_sequence_get (iter);

  return NULL;
}

static int
row_y_cmp_func (gconstpointer a,
                gconstpointer b,
                gpointer      user_data)
{
  int y = GPOINTER_TO_INT (b);
  BobguiListBoxRowPrivate *row_priv = ROW_PRIV (a);


  if (y < row_priv->y)
    return 1;
  else if (y >= row_priv->y + row_priv->height)
    return -1;

  return 0;
}

/**
 * bobgui_list_box_get_row_at_y:
 * @box: a `BobguiListBox`
 * @y: position
 *
 * Gets the row at the @y position.
 *
 * Returns: (transfer none) (nullable): the row
 */
BobguiListBoxRow *
bobgui_list_box_get_row_at_y (BobguiListBox *box,
                           int         y)
{
  GSequenceIter *iter;

  g_return_val_if_fail (BOBGUI_IS_LIST_BOX (box), NULL);

  iter = g_sequence_lookup (box->children,
                            GINT_TO_POINTER (y),
                            row_y_cmp_func,
                            NULL);

  if (iter)
    return BOBGUI_LIST_BOX_ROW (g_sequence_get (iter));

  return NULL;
}

/**
 * bobgui_list_box_select_row:
 * @box: a `BobguiListBox`
 * @row: (nullable): The row to select
 *
 * Make @row the currently selected row.
 */
void
bobgui_list_box_select_row (BobguiListBox    *box,
                         BobguiListBoxRow *row)
{
  gboolean dirty = FALSE;

  g_return_if_fail (BOBGUI_IS_LIST_BOX (box));
  g_return_if_fail (row == NULL || BOBGUI_IS_LIST_BOX_ROW (row));

  if (row)
    bobgui_list_box_select_row_internal (box, row);
  else
    dirty = bobgui_list_box_unselect_all_internal (box);

  if (dirty)
    {
      g_signal_emit (box, signals[ROW_SELECTED], 0, NULL);
      g_signal_emit (box, signals[SELECTED_ROWS_CHANGED], 0);
    }
}

/**
 * bobgui_list_box_unselect_row:
 * @box: a `BobguiListBox`
 * @row: the row to unselect
 *
 * Unselects a single row of @box, if the selection mode allows it.
 */
void
bobgui_list_box_unselect_row (BobguiListBox    *box,
                           BobguiListBoxRow *row)
{
  g_return_if_fail (BOBGUI_IS_LIST_BOX (box));
  g_return_if_fail (BOBGUI_IS_LIST_BOX_ROW (row));

  bobgui_list_box_unselect_row_internal (box, row);
}

/**
 * bobgui_list_box_select_all:
 * @box: a `BobguiListBox`
 *
 * Select all children of @box, if the selection mode allows it.
 */
void
bobgui_list_box_select_all (BobguiListBox *box)
{
  g_return_if_fail (BOBGUI_IS_LIST_BOX (box));

  if (box->selection_mode != BOBGUI_SELECTION_MULTIPLE)
    return;

  if (g_sequence_get_length (box->children) > 0)
    {
      bobgui_list_box_select_all_between (box, NULL, NULL, FALSE);
      g_signal_emit (box, signals[SELECTED_ROWS_CHANGED], 0);
    }
}

/**
 * bobgui_list_box_unselect_all:
 * @box: a `BobguiListBox`
 *
 * Unselect all children of @box, if the selection mode allows it.
 */
void
bobgui_list_box_unselect_all (BobguiListBox *box)
{
  gboolean dirty = FALSE;

  g_return_if_fail (BOBGUI_IS_LIST_BOX (box));

  if (box->selection_mode == BOBGUI_SELECTION_BROWSE)
    return;

  dirty = bobgui_list_box_unselect_all_internal (box);

  if (dirty)
    {
      g_signal_emit (box, signals[ROW_SELECTED], 0, NULL);
      g_signal_emit (box, signals[SELECTED_ROWS_CHANGED], 0);
    }
}

static void
bobgui_list_box_selected_rows_changed (BobguiListBox *box)
{
}

/**
 * BobguiListBoxForeachFunc:
 * @box: a `BobguiListBox`
 * @row: a `BobguiListBoxRow`
 * @user_data: (closure): user data
 *
 * A function used by bobgui_list_box_selected_foreach().
 *
 * It will be called on every selected child of the @box.
 */

/**
 * bobgui_list_box_selected_foreach:
 * @box: a `BobguiListBox`
 * @func: (scope call): the function to call for each selected child
 * @data: user data to pass to the function
 *
 * Calls a function for each selected child.
 *
 * Note that the selection cannot be modified from within this function.
 */
void
bobgui_list_box_selected_foreach (BobguiListBox            *box,
                               BobguiListBoxForeachFunc  func,
                               gpointer               data)
{
  BobguiListBoxRow *row;
  GSequenceIter *iter;

  g_return_if_fail (BOBGUI_IS_LIST_BOX (box));

  for (iter = g_sequence_get_begin_iter (box->children);
       !g_sequence_iter_is_end (iter);
       iter = g_sequence_iter_next (iter))
    {
      row = g_sequence_get (iter);
      if (bobgui_list_box_row_is_selected (row))
        (*func) (box, row, data);
    }
}

/**
 * bobgui_list_box_get_selected_rows:
 * @box: a `BobguiListBox`
 *
 * Creates a list of all selected children.
 *
 * Returns: (element-type BobguiListBoxRow) (transfer container):
 *   A `GList` containing the `BobguiWidget` for each selected child.
 *   Free with g_list_free() when done.
 */
GList *
bobgui_list_box_get_selected_rows (BobguiListBox *box)
{
  BobguiListBoxRow *row;
  GSequenceIter *iter;
  GList *selected = NULL;

  g_return_val_if_fail (BOBGUI_IS_LIST_BOX (box), NULL);

  for (iter = g_sequence_get_begin_iter (box->children);
       !g_sequence_iter_is_end (iter);
       iter = g_sequence_iter_next (iter))
    {
      row = g_sequence_get (iter);
      if (bobgui_list_box_row_is_selected (row))
        selected = g_list_prepend (selected, row);
    }

  return g_list_reverse (selected);
}

/**
 * bobgui_list_box_set_placeholder:
 * @box: a `BobguiListBox`
 * @placeholder: (nullable): a `BobguiWidget`
 *
 * Sets the placeholder widget that is shown in the list when
 * it doesn't display any visible children.
 */
void
bobgui_list_box_set_placeholder (BobguiListBox *box,
                              BobguiWidget  *placeholder)
{
  g_return_if_fail (BOBGUI_IS_LIST_BOX (box));

  if (box->placeholder)
    {
      bobgui_widget_unparent (box->placeholder);
      bobgui_widget_queue_resize (BOBGUI_WIDGET (box));
    }

  box->placeholder = placeholder;

  if (placeholder)
    {
      bobgui_widget_set_parent (placeholder, BOBGUI_WIDGET (box));
      bobgui_widget_set_child_visible (placeholder,
                                    box->n_visible_rows == 0);
    }
}


/**
 * bobgui_list_box_set_adjustment:
 * @box: a `BobguiListBox`
 * @adjustment: (nullable): the adjustment
 *
 * Sets the adjustment (if any) that the widget uses to
 * for vertical scrolling.
 *
 * For instance, this is used to get the page size for
 * PageUp/Down key handling.
 *
 * In the normal case when the @box is packed inside
 * a `BobguiScrolledWindow` the adjustment from that will
 * be picked up automatically, so there is no need
 * to manually do that.
 */
void
bobgui_list_box_set_adjustment (BobguiListBox    *box,
                             BobguiAdjustment *adjustment)
{
  g_return_if_fail (BOBGUI_IS_LIST_BOX (box));
  g_return_if_fail (adjustment == NULL || BOBGUI_IS_ADJUSTMENT (adjustment));

  if (adjustment)
    g_object_ref_sink (adjustment);
  if (box->adjustment)
    g_object_unref (box->adjustment);
  box->adjustment = adjustment;
}

/**
 * bobgui_list_box_get_adjustment:
 * @box: a `BobguiListBox`
 *
 * Gets the adjustment (if any) that the widget uses to
 * for vertical scrolling.
 *
 * Returns: (transfer none) (nullable): the adjustment
 */
BobguiAdjustment *
bobgui_list_box_get_adjustment (BobguiListBox *box)
{
  g_return_val_if_fail (BOBGUI_IS_LIST_BOX (box), NULL);

  return box->adjustment;
}

static void
adjustment_changed (GObject    *object,
                    GParamSpec *pspec,
                    gpointer    data)
{
  BobguiAdjustment *adjustment;

  adjustment = bobgui_scrollable_get_vadjustment (BOBGUI_SCROLLABLE (object));
  bobgui_list_box_set_adjustment (BOBGUI_LIST_BOX (data), adjustment);
}

static void
bobgui_list_box_parent_cb (GObject    *object,
                        GParamSpec *pspec,
                        gpointer    user_data)
{
  BobguiListBox *box = BOBGUI_LIST_BOX (object);
  BobguiWidget *parent;

  parent = bobgui_widget_get_parent (BOBGUI_WIDGET (object));

  if (box->adjustment_changed_id != 0 &&
      box->scrollable_parent != NULL)
    {
      g_signal_handler_disconnect (box->scrollable_parent,
                                   box->adjustment_changed_id);
    }

  if (parent && BOBGUI_IS_SCROLLABLE (parent))
    {
      adjustment_changed (G_OBJECT (parent), NULL, object);
      box->scrollable_parent = parent;
      box->adjustment_changed_id = g_signal_connect (parent, "notify::vadjustment",
                                                      G_CALLBACK (adjustment_changed), object);
    }
  else
    {
      bobgui_list_box_set_adjustment (BOBGUI_LIST_BOX (object), NULL);
      box->adjustment_changed_id = 0;
      box->scrollable_parent = NULL;
    }
}

/**
 * bobgui_list_box_set_selection_mode:
 * @box: a `BobguiListBox`
 * @mode: The `BobguiSelectionMode`
 *
 * Sets how selection works in the listbox.
 */
void
bobgui_list_box_set_selection_mode (BobguiListBox       *box,
                                 BobguiSelectionMode  mode)
{
  gboolean dirty = FALSE;

  g_return_if_fail (BOBGUI_IS_LIST_BOX (box));

  if (box->selection_mode == mode)
    return;

  if (mode == BOBGUI_SELECTION_NONE ||
      box->selection_mode == BOBGUI_SELECTION_MULTIPLE)
    dirty = bobgui_list_box_unselect_all_internal (box);

  box->selection_mode = mode;

  bobgui_list_box_update_rows (box);

  bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (box),
                                  BOBGUI_ACCESSIBLE_PROPERTY_MULTI_SELECTABLE, mode == BOBGUI_SELECTION_MULTIPLE,
                                  -1);

  g_object_notify_by_pspec (G_OBJECT (box), properties[PROP_SELECTION_MODE]);

  if (dirty)
    {
      g_signal_emit (box, signals[ROW_SELECTED], 0, NULL);
      g_signal_emit (box, signals[SELECTED_ROWS_CHANGED], 0);
    }
}

/**
 * bobgui_list_box_get_selection_mode:
 * @box: a `BobguiListBox`
 *
 * Gets the selection mode of the listbox.
 *
 * Returns: a `BobguiSelectionMode`
 */
BobguiSelectionMode
bobgui_list_box_get_selection_mode (BobguiListBox *box)
{
  g_return_val_if_fail (BOBGUI_IS_LIST_BOX (box), BOBGUI_SELECTION_NONE);

  return box->selection_mode;
}

/**
 * bobgui_list_box_set_filter_func:
 * @box: a `BobguiListBox`
 * @filter_func: (nullable) (scope notified) (closure user_data) (destroy destroy): callback
 *   that lets you filter which rows to show
 * @user_data: user data passed to @filter_func
 * @destroy: destroy notifier for @user_data
 *
 * By setting a filter function on the @box one can decide dynamically which
 * of the rows to show.
 *
 * For instance, to implement a search function on a list that
 * filters the original list to only show the matching rows.
 *
 * The @filter_func will be called for each row after the call, and
 * it will continue to be called each time a row changes (via
 * [method@Bobgui.ListBoxRow.changed]) or when [method@Bobgui.ListBox.invalidate_filter]
 * is called.
 *
 * Note that using a filter function is incompatible with using a model
 * (see [method@Bobgui.ListBox.bind_model]).
 */
void
bobgui_list_box_set_filter_func (BobguiListBox           *box,
                              BobguiListBoxFilterFunc  filter_func,
                              gpointer              user_data,
                              GDestroyNotify        destroy)
{
  g_return_if_fail (BOBGUI_IS_LIST_BOX (box));

  if (box->filter_func_target_destroy_notify != NULL)
    box->filter_func_target_destroy_notify (box->filter_func_target);

  box->filter_func = filter_func;
  box->filter_func_target = user_data;
  box->filter_func_target_destroy_notify = destroy;

  bobgui_list_box_check_model_compat (box);

  bobgui_list_box_invalidate_filter (box);
}

/**
 * bobgui_list_box_set_header_func:
 * @box: a `BobguiListBox`
 * @update_header: (nullable) (scope notified) (closure user_data) (destroy destroy): callback
 *   that lets you add row headers
 * @user_data: user data passed to @update_header
 * @destroy: destroy notifier for @user_data
 *
 * Sets a header function.
 *
 * By setting a header function on the @box one can dynamically add headers
 * in front of rows, depending on the contents of the row and its position
 * in the list.
 *
 * For instance, one could use it to add headers in front of the first item
 * of a new kind, in a list sorted by the kind.
 *
 * The @update_header can look at the current header widget using
 * [method@Bobgui.ListBoxRow.get_header] and either update the state of the widget
 * as needed, or set a new one using [method@Bobgui.ListBoxRow.set_header]. If no
 * header is needed, set the header to %NULL.
 *
 * Note that you may get many calls @update_header to this for a particular
 * row when e.g. changing things that don’t affect the header. In this case
 * it is important for performance to not blindly replace an existing header
 * with an identical one.
 *
 * The @update_header function will be called for each row after the call,
 * and it will continue to be called each time a row changes (via
 * [method@Bobgui.ListBoxRow.changed]) and when the row before changes (either
 * by [method@Bobgui.ListBoxRow.changed] on the previous row, or when the previous
 * row becomes a different row). It is also called for all rows when
 * [method@Bobgui.ListBox.invalidate_headers] is called.
 */
void
bobgui_list_box_set_header_func (BobguiListBox                 *box,
                              BobguiListBoxUpdateHeaderFunc  update_header,
                              gpointer                    user_data,
                              GDestroyNotify              destroy)
{
  g_return_if_fail (BOBGUI_IS_LIST_BOX (box));

  if (box->update_header_func_target_destroy_notify != NULL)
    box->update_header_func_target_destroy_notify (box->update_header_func_target);

  box->update_header_func = update_header;
  box->update_header_func_target = user_data;
  box->update_header_func_target_destroy_notify = destroy;
  bobgui_list_box_invalidate_headers (box);
}

/**
 * bobgui_list_box_invalidate_filter:
 * @box: a `BobguiListBox`
 *
 * Update the filtering for all rows.
 *
 * Call this when result
 * of the filter function on the @box is changed due
 * to an external factor. For instance, this would be used
 * if the filter function just looked for a specific search
 * string and the entry with the search string has changed.
 */
void
bobgui_list_box_invalidate_filter (BobguiListBox *box)
{
  g_return_if_fail (BOBGUI_IS_LIST_BOX (box));

  bobgui_list_box_apply_filter_all (box);
  bobgui_list_box_invalidate_headers (box);
  bobgui_widget_queue_resize (BOBGUI_WIDGET (box));
}

static int
do_sort (BobguiListBoxRow *a,
         BobguiListBoxRow *b,
         BobguiListBox    *box)
{
  return box->sort_func (a, b, box->sort_func_target);
}

static void
bobgui_list_box_reorder_foreach (gpointer data,
                              gpointer user_data)
{
  BobguiWidget **previous = user_data;
  BobguiWidget *row = data;

  if (*previous)
    bobgui_widget_insert_after (row, _bobgui_widget_get_parent (row), *previous);

  *previous = row;
}

/**
 * bobgui_list_box_invalidate_sort:
 * @box: a `BobguiListBox`
 *
 * Update the sorting for all rows.
 *
 * Call this when result
 * of the sort function on the @box is changed due
 * to an external factor.
 */
void
bobgui_list_box_invalidate_sort (BobguiListBox *box)
{
  BobguiWidget *previous = NULL;

  g_return_if_fail (BOBGUI_IS_LIST_BOX (box));

  if (box->sort_func == NULL)
    return;

  g_sequence_sort (box->children, (GCompareDataFunc)do_sort, box);
  g_sequence_foreach (box->children, bobgui_list_box_reorder_foreach, &previous);

  bobgui_list_box_invalidate_headers (box);
  bobgui_widget_queue_resize (BOBGUI_WIDGET (box));
}

static void
bobgui_list_box_do_reseparate (BobguiListBox *box)
{
  GSequenceIter *iter;

  for (iter = g_sequence_get_begin_iter (box->children);
       !g_sequence_iter_is_end (iter);
       iter = g_sequence_iter_next (iter))
    bobgui_list_box_update_header (box, iter);

  bobgui_widget_queue_resize (BOBGUI_WIDGET (box));
}


/**
 * bobgui_list_box_invalidate_headers:
 * @box: a `BobguiListBox`
 *
 * Update the separators for all rows.
 *
 * Call this when result
 * of the header function on the @box is changed due
 * to an external factor.
 */
void
bobgui_list_box_invalidate_headers (BobguiListBox *box)
{
  g_return_if_fail (BOBGUI_IS_LIST_BOX (box));

  if (!bobgui_widget_get_visible (BOBGUI_WIDGET (box)))
    return;

  bobgui_list_box_do_reseparate (box);
}

/**
 * bobgui_list_box_set_sort_func:
 * @box: a `BobguiListBox`
 * @sort_func: (nullable) (scope notified) (closure user_data) (destroy destroy): the sort function
 * @user_data: user data passed to @sort_func
 * @destroy: destroy notifier for @user_data
 *
 * Sets a sort function.
 *
 * By setting a sort function on the @box one can dynamically reorder
 * the rows of the list, based on the contents of the rows.
 *
 * The @sort_func will be called for each row after the call, and will
 * continue to be called each time a row changes (via
 * [method@Bobgui.ListBoxRow.changed]) and when [method@Bobgui.ListBox.invalidate_sort]
 * is called.
 *
 * Note that using a sort function is incompatible with using a model
 * (see [method@Bobgui.ListBox.bind_model]).
 */
void
bobgui_list_box_set_sort_func (BobguiListBox         *box,
                            BobguiListBoxSortFunc  sort_func,
                            gpointer            user_data,
                            GDestroyNotify      destroy)
{
  g_return_if_fail (BOBGUI_IS_LIST_BOX (box));

  if (box->sort_func_target_destroy_notify != NULL)
    box->sort_func_target_destroy_notify (box->sort_func_target);

  box->sort_func = sort_func;
  box->sort_func_target = user_data;
  box->sort_func_target_destroy_notify = destroy;

  bobgui_list_box_check_model_compat (box);

  bobgui_list_box_invalidate_sort (box);
}

static void
bobgui_list_box_got_row_changed (BobguiListBox    *box,
                              BobguiListBoxRow *row)
{
  BobguiListBoxRowPrivate *row_priv = ROW_PRIV (row);
  GSequenceIter *prev_next, *next;

  g_return_if_fail (BOBGUI_IS_LIST_BOX (box));
  g_return_if_fail (BOBGUI_IS_LIST_BOX_ROW (row));

  prev_next = bobgui_list_box_get_next_visible (box, row_priv->iter);
  if (box->sort_func != NULL)
    {
      g_sequence_sort_changed (row_priv->iter,
                               (GCompareDataFunc)do_sort,
                               box);
      bobgui_widget_queue_resize (BOBGUI_WIDGET (box));
    }
  bobgui_list_box_apply_filter (box, row);
  if (bobgui_widget_get_visible (BOBGUI_WIDGET (box)))
    {
      next = bobgui_list_box_get_next_visible (box, row_priv->iter);
      bobgui_list_box_update_header (box, row_priv->iter);
      bobgui_list_box_update_header (box, next);
      bobgui_list_box_update_header (box, prev_next);
    }
}

/**
 * bobgui_list_box_set_activate_on_single_click:
 * @box: a `BobguiListBox`
 * @single: a boolean
 *
 * If @single is %TRUE, rows will be activated when you click on them,
 * otherwise you need to double-click.
 */
void
bobgui_list_box_set_activate_on_single_click (BobguiListBox *box,
                                           gboolean    single)
{
  g_return_if_fail (BOBGUI_IS_LIST_BOX (box));

  single = single != FALSE;

  if (box->activate_single_click == single)
    return;

  box->activate_single_click = single;

  g_object_notify_by_pspec (G_OBJECT (box), properties[PROP_ACTIVATE_ON_SINGLE_CLICK]);
}

/**
 * bobgui_list_box_get_activate_on_single_click:
 * @box: a `BobguiListBox`
 *
 * Returns whether rows activate on single clicks.
 *
 * Returns: %TRUE if rows are activated on single click, %FALSE otherwise
 */
gboolean
bobgui_list_box_get_activate_on_single_click (BobguiListBox *box)
{
  g_return_val_if_fail (BOBGUI_IS_LIST_BOX (box), FALSE);

  return box->activate_single_click;
}

void
bobgui_list_box_set_accept_unpaired_release (BobguiListBox *box,
                                          gboolean    accept)
{
  if (box->accept_unpaired_release == accept)
    return;

  box->accept_unpaired_release = accept;

  g_object_notify_by_pspec (G_OBJECT (box), properties[PROP_ACCEPT_UNPAIRED_RELEASE]);
}

static void
bobgui_list_box_add_move_binding (BobguiWidgetClass  *widget_class,
                               guint            keyval,
                               GdkModifierType  modmask,
                               BobguiMovementStep  step,
                               int              count)
{
  bobgui_widget_class_add_binding_signal (widget_class,
                                       keyval, modmask,
                                       "move-cursor",
                                       "(iibb)", step, count, FALSE, FALSE);
  bobgui_widget_class_add_binding_signal (widget_class,
                                       keyval, modmask | GDK_SHIFT_MASK,
                                       "move-cursor",
                                       "(iibb)", step, count, TRUE, FALSE);
  bobgui_widget_class_add_binding_signal (widget_class,
                                       keyval, modmask | GDK_CONTROL_MASK,
                                       "move-cursor",
                                       "(iibb)", step, count, FALSE, TRUE);
  bobgui_widget_class_add_binding_signal (widget_class,
                                       keyval, modmask | GDK_SHIFT_MASK | GDK_CONTROL_MASK,
                                       "move-cursor",
                                       "(iibb)", step, count, TRUE, TRUE);
}

static void
ensure_row_visible (BobguiListBox    *box,
                    BobguiListBoxRow *row)
{
  BobguiWidget *header;
  int y, height;
  graphene_rect_t rect;

  if (!box->adjustment)
    return;

  if (!bobgui_widget_compute_bounds (BOBGUI_WIDGET (row), BOBGUI_WIDGET (box), &rect))
    return;

  y = rect.origin.y;
  height = rect.size.height;

  /* If the row has a header, we want to ensure that it is visible as well. */
  header = ROW_PRIV (row)->header;
  if (BOBGUI_IS_WIDGET (header) && bobgui_widget_is_drawable (header))
    {
      if (bobgui_widget_compute_bounds (header, BOBGUI_WIDGET (box), &rect))
        {
          y = rect.origin.y;
          height += rect.size.height;
        }
    }

  bobgui_adjustment_clamp_page (box->adjustment, y, y + height);
}

static void
bobgui_list_box_update_cursor (BobguiListBox    *box,
                            BobguiListBoxRow *row,
                            gboolean grab_focus)
{
  box->cursor_row = row;
  ensure_row_visible (box, row);
  if (grab_focus)
    {
      BobguiWidget *focus;

      focus = bobgui_root_get_focus (bobgui_widget_get_root (BOBGUI_WIDGET (box)));
      if (!focus || !bobgui_widget_is_ancestor (focus, BOBGUI_WIDGET (row)))
        bobgui_widget_grab_focus (BOBGUI_WIDGET (row));
    }
  bobgui_widget_queue_draw (BOBGUI_WIDGET (row));
}

static BobguiListBox *
bobgui_list_box_row_get_box (BobguiListBoxRow *row)
{
  BobguiWidget *parent;

  parent = bobgui_widget_get_parent (BOBGUI_WIDGET (row));
  if (parent && BOBGUI_IS_LIST_BOX (parent))
    return BOBGUI_LIST_BOX (parent);

  return NULL;
}

static gboolean
row_is_visible (BobguiListBoxRow *row)
{
  return ROW_PRIV (row)->visible;
}

static gboolean
bobgui_list_box_row_set_selected (BobguiListBoxRow *row,
                               gboolean       selected)
{
  if (!ROW_PRIV (row)->selectable)
    return FALSE;

  if (ROW_PRIV (row)->selected != selected)
    {
      ROW_PRIV (row)->selected = selected;
      if (selected)
        bobgui_widget_set_state_flags (BOBGUI_WIDGET (row),
                                    BOBGUI_STATE_FLAG_SELECTED, FALSE);
      else
        bobgui_widget_unset_state_flags (BOBGUI_WIDGET (row),
                                      BOBGUI_STATE_FLAG_SELECTED);

      bobgui_accessible_update_state (BOBGUI_ACCESSIBLE (row),
                                   BOBGUI_ACCESSIBLE_STATE_SELECTED, selected,
                                   -1);

      return TRUE;
    }

  return FALSE;
}

static gboolean
bobgui_list_box_unselect_all_internal (BobguiListBox *box)
{
  BobguiListBoxRow *row;
  GSequenceIter *iter;
  gboolean dirty = FALSE;

  if (box->selection_mode == BOBGUI_SELECTION_NONE)
    return FALSE;

  for (iter = g_sequence_get_begin_iter (box->children);
       !g_sequence_iter_is_end (iter);
       iter = g_sequence_iter_next (iter))
    {
      row = g_sequence_get (iter);
      dirty |= bobgui_list_box_row_set_selected (row, FALSE);
    }

  box->selected_row = NULL;

  return dirty;
}

static void
bobgui_list_box_unselect_row_internal (BobguiListBox    *box,
                                    BobguiListBoxRow *row)
{
  if (!ROW_PRIV (row)->selected)
    return;

  if (box->selection_mode == BOBGUI_SELECTION_NONE)
    return;
  else if (box->selection_mode != BOBGUI_SELECTION_MULTIPLE)
    bobgui_list_box_unselect_all_internal (box);
  else
    bobgui_list_box_row_set_selected (row, FALSE);

  g_signal_emit (box, signals[ROW_SELECTED], 0, NULL);
  g_signal_emit (box, signals[SELECTED_ROWS_CHANGED], 0);
}

static void
bobgui_list_box_select_row_internal (BobguiListBox    *box,
                                  BobguiListBoxRow *row)
{
  if (!ROW_PRIV (row)->selectable)
    return;

  if (ROW_PRIV (row)->selected)
    return;

  if (box->selection_mode == BOBGUI_SELECTION_NONE)
    return;

  if (box->selection_mode != BOBGUI_SELECTION_MULTIPLE)
    bobgui_list_box_unselect_all_internal (box);

  bobgui_list_box_row_set_selected (row, TRUE);
  box->selected_row = row;

  g_signal_emit (box, signals[ROW_SELECTED], 0, row);
  g_signal_emit (box, signals[SELECTED_ROWS_CHANGED], 0);
}

static void
bobgui_list_box_select_all_between (BobguiListBox    *box,
                                 BobguiListBoxRow *row1,
                                 BobguiListBoxRow *row2,
                                 gboolean       modify)
{
  GSequenceIter *iter, *iter1, *iter2;

  if (row1)
    iter1 = ROW_PRIV (row1)->iter;
  else
    iter1 = g_sequence_get_begin_iter (box->children);

  if (row2)
    iter2 = ROW_PRIV (row2)->iter;
  else
    iter2 = g_sequence_get_end_iter (box->children);

  if (g_sequence_iter_compare (iter2, iter1) < 0)
    {
      iter = iter1;
      iter1 = iter2;
      iter2 = iter;
    }

  for (iter = iter1;
       !g_sequence_iter_is_end (iter);
       iter = g_sequence_iter_next (iter))
    {
      BobguiListBoxRow *row;

      row = BOBGUI_LIST_BOX_ROW (g_sequence_get (iter));
      if (row_is_visible (row))
        {
          if (modify)
            bobgui_list_box_row_set_selected (row, !ROW_PRIV (row)->selected);
          else
            bobgui_list_box_row_set_selected (row, TRUE);
        }

      if (g_sequence_iter_compare (iter, iter2) == 0)
        break;
    }
}

#define bobgui_list_box_update_selection(b,r,m,e) \
  bobgui_list_box_update_selection_full((b), (r), (m), (e), TRUE)
static void
bobgui_list_box_update_selection_full (BobguiListBox    *box,
                                    BobguiListBoxRow *row,
                                    gboolean       modify,
                                    gboolean       extend,
                                    gboolean       grab_cursor)
{
  bobgui_list_box_update_cursor (box, row, grab_cursor);

  if (box->selection_mode == BOBGUI_SELECTION_NONE)
    return;

  if (!ROW_PRIV (row)->selectable)
    return;

  if (box->selection_mode == BOBGUI_SELECTION_BROWSE)
    {
      bobgui_list_box_unselect_all_internal (box);
      bobgui_list_box_row_set_selected (row, TRUE);
      box->selected_row = row;
      g_signal_emit (box, signals[ROW_SELECTED], 0, row);
    }
  else if (box->selection_mode == BOBGUI_SELECTION_SINGLE)
    {
      gboolean was_selected;

      was_selected = ROW_PRIV (row)->selected;
      bobgui_list_box_unselect_all_internal (box);
      bobgui_list_box_row_set_selected (row, modify ? !was_selected : TRUE);
      box->selected_row = ROW_PRIV (row)->selected ? row : NULL;
      g_signal_emit (box, signals[ROW_SELECTED], 0, box->selected_row);
    }
  else /* BOBGUI_SELECTION_MULTIPLE */
    {
      if (extend)
        {
          BobguiListBoxRow *selected_row;

          selected_row = box->selected_row;

          bobgui_list_box_unselect_all_internal (box);

          if (selected_row == NULL)
            {
              bobgui_list_box_row_set_selected (row, TRUE);
              box->selected_row = row;
              g_signal_emit (box, signals[ROW_SELECTED], 0, row);
            }
          else
            {
              bobgui_list_box_select_all_between (box, selected_row, row, FALSE);
              box->selected_row = selected_row;
            }
        }
      else
        {
          if (modify)
            {
              bobgui_list_box_row_set_selected (row, !ROW_PRIV (row)->selected);
              g_signal_emit (box, signals[ROW_SELECTED], 0, ROW_PRIV (row)->selected ? row
                                                                                     : NULL);
            }
          else
            {
              bobgui_list_box_unselect_all_internal (box);
              bobgui_list_box_row_set_selected (row, !ROW_PRIV (row)->selected);
              box->selected_row = row;
              g_signal_emit (box, signals[ROW_SELECTED], 0, row);
            }
        }
    }

  g_signal_emit (box, signals[SELECTED_ROWS_CHANGED], 0);
}

static void
bobgui_list_box_activate (BobguiListBox    *box,
                       BobguiListBoxRow *row)
{
  if (!bobgui_list_box_row_get_activatable (row))
    return;

  if (ROW_PRIV (row)->action_helper)
    bobgui_action_helper_activate (ROW_PRIV (row)->action_helper);
  else
    g_signal_emit (box, signals[ROW_ACTIVATED], 0, row);
}

#define bobgui_list_box_select_and_activate(b,r) \
  bobgui_list_box_select_and_activate_full ((b), (r), TRUE)
static void
bobgui_list_box_select_and_activate_full (BobguiListBox    *box,
                                       BobguiListBoxRow *row,
                                       gboolean       grab_focus)
{
  if (row != NULL)
    {
      bobgui_list_box_select_row_internal (box, row);
      bobgui_list_box_update_cursor (box, row, grab_focus);
      bobgui_list_box_activate (box, row);
    }
}

static void
bobgui_list_box_click_gesture_pressed (BobguiGestureClick *gesture,
                                    guint            n_press,
                                    double           x,
                                    double           y,
                                    BobguiListBox      *box)
{
  BobguiListBoxRow *row;

  box->active_row = NULL;
  row = bobgui_list_box_get_row_at_y (box, y);

  if (row != NULL && bobgui_widget_is_sensitive (BOBGUI_WIDGET (row)))
    {
      box->active_row = row;

      if (n_press == 2 && !box->activate_single_click)
        bobgui_list_box_activate (box, row);
    }
}

static void
bobgui_list_box_click_unpaired_release (BobguiGestureClick  *gesture,
                                     double            x,
                                     double            y,
                                     guint             button,
                                     GdkEventSequence *sequence,
                                     BobguiListBox       *box)
{
  BobguiListBoxRow *row;

  if (!box->activate_single_click || !box->accept_unpaired_release)
    return;

  row = bobgui_list_box_get_row_at_y (box, y);

  if (row)
    bobgui_list_box_select_and_activate (box, row);
}

static void
bobgui_list_box_click_gesture_released (BobguiGestureClick *gesture,
                                     guint            n_press,
                                     double           x,
                                     double           y,
                                     BobguiListBox      *box)
{
  /* Take a ref to protect against reentrancy
   * (the activation may destroy the widget)
   */
  g_object_ref (box);

  if (box->active_row != NULL &&
      box->active_row == bobgui_list_box_get_row_at_y (box, y))
    {
      gboolean focus_on_click = bobgui_widget_get_focus_on_click (BOBGUI_WIDGET (box->active_row));

      if (n_press == 1 && box->activate_single_click)
        bobgui_list_box_select_and_activate_full (box, box->active_row, focus_on_click);
      else
        {
          GdkEventSequence *sequence;
          GdkInputSource source;
          GdkEvent *event;
          GdkModifierType state;
          gboolean extend;
          gboolean modify;

          /* With touch, we default to modifying the selection.
           * You can still clear the selection and start over
           * by holding Ctrl.
           */
          sequence = bobgui_gesture_single_get_current_sequence (BOBGUI_GESTURE_SINGLE (gesture));
          event = bobgui_gesture_get_last_event (BOBGUI_GESTURE (gesture), sequence);
          state = gdk_event_get_modifier_state (event);
          extend = (state & GDK_SHIFT_MASK) != 0;
          modify = (state & GDK_CONTROL_MASK) != 0;
#ifdef __APPLE__
          modify = modify | ((state & GDK_META_MASK) != 0);
#endif
          source = gdk_device_get_source (gdk_event_get_device (event));

          if (source == GDK_SOURCE_TOUCHSCREEN)
            modify = !modify;

          bobgui_list_box_update_selection_full (box, box->active_row, modify, extend, focus_on_click);
        }
    }

  if (box->active_row)
    {
      box->active_row = NULL;
    }

  g_object_unref (box);
}

static void
bobgui_list_box_click_gesture_stopped (BobguiGestureClick *gesture,
                                    BobguiListBox      *box)
{
  if (box->active_row)
    {
      box->active_row = NULL;
      bobgui_widget_queue_draw (BOBGUI_WIDGET (box));
    }
}

static void
bobgui_list_box_show (BobguiWidget *widget)
{
  bobgui_list_box_do_reseparate (BOBGUI_LIST_BOX (widget));

  BOBGUI_WIDGET_CLASS (bobgui_list_box_parent_class)->show (widget);
}

static gboolean
bobgui_list_box_focus (BobguiWidget        *widget,
                    BobguiDirectionType  direction)
{
  BobguiListBox *box = BOBGUI_LIST_BOX (widget);
  BobguiWidget *focus_child;
  BobguiListBoxRow *next_focus_row;
  BobguiWidget *row;
  BobguiWidget *header;

  focus_child = bobgui_widget_get_focus_child (widget);

  next_focus_row = NULL;
  if (focus_child != NULL)
    {
      GSequenceIter *i;

      if (bobgui_widget_child_focus (focus_child, direction))
        return TRUE;

      if (direction == BOBGUI_DIR_UP || (direction == BOBGUI_DIR_TAB_BACKWARD && box->tab_behavior == BOBGUI_LIST_TAB_ALL))
        {
          if (BOBGUI_IS_LIST_BOX_ROW (focus_child))
            {
              header = ROW_PRIV (BOBGUI_LIST_BOX_ROW (focus_child))->header;
              if (header && bobgui_widget_child_focus (header, direction))
                return TRUE;
            }

          if (BOBGUI_IS_LIST_BOX_ROW (focus_child))
            row = focus_child;
          else
            row = g_hash_table_lookup (box->header_hash, focus_child);

          if (BOBGUI_IS_LIST_BOX_ROW (row))
            i = bobgui_list_box_get_previous_visible (box, ROW_PRIV (BOBGUI_LIST_BOX_ROW (row))->iter);
          else
            i = NULL;

          while (i != NULL)
            {
              if (bobgui_widget_get_sensitive (g_sequence_get (i)))
                {
                  next_focus_row = g_sequence_get (i);
                  break;
                }

              i = bobgui_list_box_get_previous_visible (box, i);
            }
        }
      else if (direction == BOBGUI_DIR_DOWN || (direction == BOBGUI_DIR_TAB_FORWARD && box->tab_behavior == BOBGUI_LIST_TAB_ALL))
        {
          if (BOBGUI_IS_LIST_BOX_ROW (focus_child))
            i = bobgui_list_box_get_next_visible (box, ROW_PRIV (BOBGUI_LIST_BOX_ROW (focus_child))->iter);
          else
            {
              row = g_hash_table_lookup (box->header_hash, focus_child);
              if (BOBGUI_IS_LIST_BOX_ROW (row))
                i = ROW_PRIV (BOBGUI_LIST_BOX_ROW (row))->iter;
              else
                i = NULL;
            }

          while (i != NULL && !g_sequence_iter_is_end (i))
            {
              if (bobgui_widget_get_sensitive (g_sequence_get (i)))
                {
                  next_focus_row = g_sequence_get (i);
                  break;
                }

              i = bobgui_list_box_get_next_visible (box, i);
            }
        }
    }
  else
    {
      /* No current focus row */
      switch (direction)
        {
        case BOBGUI_DIR_UP:
        case BOBGUI_DIR_TAB_BACKWARD:
          next_focus_row = box->selected_row;
          if (next_focus_row == NULL)
            next_focus_row = bobgui_list_box_get_last_focusable (box);
          break;
        case BOBGUI_DIR_DOWN:
        case BOBGUI_DIR_TAB_FORWARD:
        case BOBGUI_DIR_LEFT:
        case BOBGUI_DIR_RIGHT:
        default:
          next_focus_row = box->selected_row;
          if (next_focus_row == NULL)
            next_focus_row = bobgui_list_box_get_first_focusable (box);
          break;
        }
    }

  if (next_focus_row == NULL)
    {
      if (direction == BOBGUI_DIR_UP || direction == BOBGUI_DIR_DOWN)
        {
          if (bobgui_widget_keynav_failed (BOBGUI_WIDGET (box), direction))
            return TRUE;
        }

      return FALSE;
    }

  if (direction == BOBGUI_DIR_DOWN || direction == BOBGUI_DIR_TAB_FORWARD)
    {
      header = ROW_PRIV (next_focus_row)->header;
      if (header && bobgui_widget_child_focus (header, direction))
        return TRUE;
    }

  if (bobgui_widget_child_focus (BOBGUI_WIDGET (next_focus_row), direction))
    return TRUE;

  return FALSE;
}

static void
list_box_add_visible_rows (BobguiListBox *box,
                           int         n)
{
  int was_zero;

  was_zero = box->n_visible_rows == 0;
  box->n_visible_rows += n;

  if (box->placeholder &&
      (was_zero || box->n_visible_rows == 0))
    bobgui_widget_set_child_visible (BOBGUI_WIDGET (box->placeholder),
                                  box->n_visible_rows == 0);
}

/* Children are visible if they are shown by the app (visible)
 * and not filtered out (child_visible) by the listbox
 */
static void
update_row_is_visible (BobguiListBox    *box,
                       BobguiListBoxRow *row)
{
  BobguiListBoxRowPrivate *row_priv = ROW_PRIV (row);
  gboolean was_visible;

  was_visible = row_priv->visible;

  row_priv->visible =
    bobgui_widget_get_visible (BOBGUI_WIDGET (row)) &&
    bobgui_widget_get_child_visible (BOBGUI_WIDGET (row));

  if (was_visible && !row_priv->visible)
    list_box_add_visible_rows (box, -1);
  if (!was_visible && row_priv->visible)
    list_box_add_visible_rows (box, 1);
}

static void
bobgui_list_box_apply_filter (BobguiListBox    *box,
                           BobguiListBoxRow *row)
{
  gboolean do_show;

  do_show = TRUE;
  if (box->filter_func != NULL)
    do_show = box->filter_func (row, box->filter_func_target);

  bobgui_widget_set_child_visible (BOBGUI_WIDGET (row), do_show);

  update_row_is_visible (box, row);
}

static void
bobgui_list_box_apply_filter_all (BobguiListBox *box)
{
  BobguiListBoxRow *row;
  GSequenceIter *iter;

  for (iter = g_sequence_get_begin_iter (box->children);
       !g_sequence_iter_is_end (iter);
       iter = g_sequence_iter_next (iter))
    {
      row = g_sequence_get (iter);
      bobgui_list_box_apply_filter (box, row);
    }
}

static BobguiListBoxRow *
bobgui_list_box_get_first_focusable (BobguiListBox *box)
{
  BobguiListBoxRow *row;
  GSequenceIter *iter;

  for (iter = g_sequence_get_begin_iter (box->children);
       !g_sequence_iter_is_end (iter);
       iter = g_sequence_iter_next (iter))
    {
        row = g_sequence_get (iter);
        if (row_is_visible (row) && bobgui_widget_is_sensitive (BOBGUI_WIDGET (row)))
          return row;
    }

  return NULL;
}

static BobguiListBoxRow *
bobgui_list_box_get_last_focusable (BobguiListBox *box)
{
  BobguiListBoxRow *row;
  GSequenceIter *iter;

  iter = g_sequence_get_end_iter (box->children);
  while (!g_sequence_iter_is_begin (iter))
    {
      iter = g_sequence_iter_prev (iter);
      row = g_sequence_get (iter);
      if (row_is_visible (row) && bobgui_widget_is_sensitive (BOBGUI_WIDGET (row)))
        return row;
    }

  return NULL;
}

static GSequenceIter *
bobgui_list_box_get_previous_visible (BobguiListBox    *box,
                                   GSequenceIter *iter)
{
  BobguiListBoxRow *row;

  if (g_sequence_iter_is_begin (iter))
    return NULL;

  do
    {
      iter = g_sequence_iter_prev (iter);
      row = g_sequence_get (iter);
      if (row_is_visible (row))
        return iter;
    }
  while (!g_sequence_iter_is_begin (iter));

  return NULL;
}

static GSequenceIter *
bobgui_list_box_get_next_visible (BobguiListBox    *box,
                               GSequenceIter *iter)
{
  BobguiListBoxRow *row;

  if (g_sequence_iter_is_end (iter))
    return iter;

  do
    {
      iter = g_sequence_iter_next (iter);
      if (!g_sequence_iter_is_end (iter))
        {
        row = g_sequence_get (iter);
        if (row_is_visible (row))
          return iter;
        }
    }
  while (!g_sequence_iter_is_end (iter));

  return iter;
}

static GSequenceIter *
bobgui_list_box_get_last_visible (BobguiListBox    *box,
                               GSequenceIter *iter)
{
  GSequenceIter *next = NULL;

  if (g_sequence_iter_is_end (iter))
    return NULL;

  do
    {
      next = bobgui_list_box_get_next_visible (box, iter);

      if (!g_sequence_iter_is_end (next))
        iter = next;
    }
  while (!g_sequence_iter_is_end (next));

  return iter;
}

static void
bobgui_list_box_update_header (BobguiListBox    *box,
                            GSequenceIter *iter)
{
  BobguiListBoxRow *row;
  GSequenceIter *before_iter;
  BobguiListBoxRow *before_row;
  BobguiWidget *old_header, *new_header;

  if (iter == NULL || g_sequence_iter_is_end (iter))
    return;

  row = g_sequence_get (iter);
  g_object_ref (row);

  before_iter = bobgui_list_box_get_previous_visible (box, iter);
  before_row = NULL;
  if (before_iter != NULL)
    {
      before_row = g_sequence_get (before_iter);
      if (before_row)
        g_object_ref (before_row);
    }

  if (box->update_header_func != NULL &&
      row_is_visible (row))
    {
      old_header = ROW_PRIV (row)->header;
      if (old_header)
        g_object_ref (old_header);
      box->update_header_func (row,
                                before_row,
                                box->update_header_func_target);
      new_header = ROW_PRIV (row)->header;
      if (old_header != new_header)
        {
          if (old_header != NULL &&
              g_hash_table_lookup (box->header_hash, old_header) == row)
            {
              /* Only unparent the @old_header if it hasn’t been re-used as the
               * header for a different row. */
              bobgui_widget_unparent (old_header);
              g_hash_table_remove (box->header_hash, old_header);
            }
          if (new_header != NULL)
            {
              g_hash_table_insert (box->header_hash, new_header, row);
              bobgui_widget_unparent (new_header);
              bobgui_widget_set_parent (new_header, BOBGUI_WIDGET (box));
            }
          bobgui_widget_queue_resize (BOBGUI_WIDGET (box));
        }
      if (old_header)
        g_object_unref (old_header);
    }
  else
    {
      if (ROW_PRIV (row)->header != NULL)
        {
          g_hash_table_remove (box->header_hash, ROW_PRIV (row)->header);
          bobgui_widget_unparent (ROW_PRIV (row)->header);
          bobgui_list_box_row_set_header (row, NULL);
          bobgui_widget_queue_resize (BOBGUI_WIDGET (box));
        }
    }
  if (before_row)
    g_object_unref (before_row);
  g_object_unref (row);
}

static void
bobgui_list_box_row_visibility_changed (BobguiListBox    *box,
                                     BobguiListBoxRow *row)
{
  update_row_is_visible (box, row);

  if (bobgui_widget_get_visible (BOBGUI_WIDGET (box)) &&
      ROW_PRIV (row)->iter)
    {
      bobgui_list_box_update_header (box, ROW_PRIV (row)->iter);
      bobgui_list_box_update_header (box,
                                  bobgui_list_box_get_next_visible (box, ROW_PRIV (row)->iter));
    }
}

/**
 * bobgui_list_box_remove:
 * @box: a `BobguiListBox`
 * @child: the child to remove
 *
 * Removes a child from @box.
 */
void
bobgui_list_box_remove (BobguiListBox *box,
                     BobguiWidget  *child)
{
  BobguiWidget *widget;
  gboolean was_visible;
  gboolean was_selected;
  BobguiListBoxRow *row;
  GSequenceIter *iter;
  GSequenceIter *next;

  g_return_if_fail (BOBGUI_IS_LIST_BOX (box));
  g_return_if_fail (BOBGUI_IS_WIDGET (child));

  widget = BOBGUI_WIDGET (box);
  was_visible = bobgui_widget_get_visible (child);

  if (child == box->placeholder)
    {
      bobgui_widget_unparent (child);
      box->placeholder = NULL;
      if (was_visible && bobgui_widget_get_visible (widget))
        bobgui_widget_queue_resize (widget);

      return;
    }

  if (!BOBGUI_IS_LIST_BOX_ROW (child))
    {
      row = g_hash_table_lookup (box->header_hash, child);
      if (row != NULL)
        {
          g_hash_table_remove (box->header_hash, child);
          g_clear_object (&ROW_PRIV (row)->header);
          bobgui_widget_unparent (child);
          if (was_visible && bobgui_widget_get_visible (widget))
            bobgui_widget_queue_resize (widget);
        }
      else
        {
          g_warning ("Tried to remove non-child %p", child);
        }
      return;
    }

  row = BOBGUI_LIST_BOX_ROW (child);
  iter = ROW_PRIV (row)->iter;
  ROW_PRIV (row)->iter = NULL;

  if (g_sequence_iter_get_sequence (iter) != box->children)
    {
      g_warning ("Tried to remove non-child %p", child);
      return;
    }

  was_selected = ROW_PRIV (row)->selected;

  if (ROW_PRIV (row)->visible)
    list_box_add_visible_rows (box, -1);

  if (ROW_PRIV (row)->header != NULL)
    {
      g_hash_table_remove (box->header_hash, ROW_PRIV (row)->header);
      bobgui_widget_unparent (ROW_PRIV (row)->header);
      g_clear_object (&ROW_PRIV (row)->header);
    }

  if (row == box->selected_row)
    box->selected_row = NULL;
  if (row == box->cursor_row)
    box->cursor_row = NULL;
  if (row == box->active_row)
    box->active_row = NULL;

  if (row == box->drag_highlighted_row)
    bobgui_list_box_drag_unhighlight_row (box);

  next = bobgui_list_box_get_next_visible (box, iter);
  bobgui_widget_unparent (child);
  g_sequence_remove (iter);

  /* After unparenting, those values are garbage */
  iter = NULL;
  row = NULL;
  child = NULL;

  if (bobgui_widget_get_visible (widget))
    bobgui_list_box_update_header (box, next);

  if (was_visible && bobgui_widget_get_visible (BOBGUI_WIDGET (box)))
    bobgui_widget_queue_resize (widget);

  if (was_selected && !bobgui_widget_in_destruction (widget))
    {
      g_signal_emit (box, signals[ROW_SELECTED], 0, NULL);
      g_signal_emit (box, signals[SELECTED_ROWS_CHANGED], 0);
    }
}

/**
 * bobgui_list_box_remove_all:
 * @box: a `BobguiListBox`
 *
 * Removes all rows from @box.
 *
 * This function does nothing if @box is backed by a model.
 *
 * Since: 4.12
 */
void
bobgui_list_box_remove_all (BobguiListBox *box)
{
  BobguiWidget *widget = BOBGUI_WIDGET (box);
  BobguiWidget *child;

  g_return_if_fail (BOBGUI_IS_LIST_BOX (box));

  if (box->bound_model)
    return;

  while ((child = bobgui_widget_get_first_child (widget)) != NULL)
    bobgui_list_box_remove (box, child);
}

static void
bobgui_list_box_compute_expand (BobguiWidget *widget,
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
bobgui_list_box_get_request_mode (BobguiWidget *widget)
{
  BobguiListBox *box = BOBGUI_LIST_BOX (widget);
  GSequenceIter *iter;
  BobguiListBoxRow *row;

  if (box->placeholder && bobgui_widget_get_child_visible (box->placeholder))
    return bobgui_widget_get_request_mode (box->placeholder);

  /* Return constant-size, unless any of the children do hfw (or wfh) */

  for (iter = g_sequence_get_begin_iter (box->children);
       !g_sequence_iter_is_end (iter);
       iter = g_sequence_iter_next (iter))
    {
      row = g_sequence_get (iter);
      if (!row_is_visible (row))
        continue;

      if (ROW_PRIV (row)->header != NULL &&
          bobgui_widget_get_request_mode (ROW_PRIV (row)->header) != BOBGUI_SIZE_REQUEST_CONSTANT_SIZE)
        return BOBGUI_SIZE_REQUEST_HEIGHT_FOR_WIDTH;
      if (bobgui_widget_get_request_mode (BOBGUI_WIDGET (row)) != BOBGUI_SIZE_REQUEST_CONSTANT_SIZE)
        return BOBGUI_SIZE_REQUEST_HEIGHT_FOR_WIDTH;
    }

  return BOBGUI_SIZE_REQUEST_CONSTANT_SIZE;
}

static void
bobgui_list_box_measure_height_for_width (BobguiListBox       *box,
                                       int               for_width,
                                       int              *minimum,
                                       int              *natural,
                                       BobguiRequestedSize *sizes)
{
  GSequenceIter *iter;
  BobguiListBoxRow *row;
  int i = 0;

  if (box->placeholder && bobgui_widget_get_child_visible (box->placeholder))
    {
      bobgui_widget_measure (box->placeholder, BOBGUI_ORIENTATION_VERTICAL,
                          for_width, minimum, natural, NULL, NULL);
      return;
    }

  *minimum = 0;
  *natural = 0;

  for (iter = g_sequence_get_begin_iter (box->children);
       !g_sequence_iter_is_end (iter);
       iter = g_sequence_iter_next (iter))
    {
      int row_min = 0, row_nat = 0;

      row = g_sequence_get (iter);
      if (!row_is_visible (row))
        continue;

      if (ROW_PRIV (row)->header != NULL)
        {
          bobgui_widget_measure (ROW_PRIV (row)->header, BOBGUI_ORIENTATION_VERTICAL,
                              for_width, &row_min, &row_nat, NULL, NULL);
          *minimum += row_min;
          *natural += row_nat;

          if (sizes)
            {
              sizes[i].minimum_size = row_min;
              sizes[i].natural_size = row_nat;
              i++;
            }
        }
      bobgui_widget_measure (BOBGUI_WIDGET (row), BOBGUI_ORIENTATION_VERTICAL,
                          for_width, &row_min, &row_nat, NULL, NULL);
      *minimum += row_min;
      *natural += row_nat;

      if (sizes)
        {
          sizes[i].minimum_size = row_min;
          sizes[i].natural_size = row_nat;
          i++;
        }
    }
}

static void
bobgui_list_box_measure_width_for_height (BobguiListBox *box,
                                       int         for_height,
                                       int        *minimum,
                                       int        *natural)
{
  GSequenceIter *iter;
  BobguiListBoxRow *row;
  int i = 0;
  BobguiRequestedSize *sizes = NULL;
  int min, max, min_height, nat_height;
  int n_vexpand_children = 0;
  int extra_height;

  if (box->placeholder && bobgui_widget_get_child_visible (box->placeholder))
    {
      bobgui_widget_measure (box->placeholder, BOBGUI_ORIENTATION_HORIZONTAL,
                          for_height, minimum, natural, NULL, NULL);
      return;
    }

  *minimum = 0;
  *natural = 0;

  /* Measure width for natural height */
  for (iter = g_sequence_get_begin_iter (box->children);
       !g_sequence_iter_is_end (iter);
       iter = g_sequence_iter_next (iter))
    {
      int row_min, row_nat;

      row = g_sequence_get (iter);

      /* We *do* take visible but filtered rows into account here so that
       * the list width doesn't change during filtering
       */
      if (!bobgui_widget_get_visible (BOBGUI_WIDGET (row)))
        continue;

      bobgui_widget_measure (BOBGUI_WIDGET (row), BOBGUI_ORIENTATION_HORIZONTAL,
                          -1, &row_min, &row_nat, NULL, NULL);

      *minimum = MAX (*minimum, row_min);
      *natural = MAX (*natural, row_nat);

      if (for_height >= 0 && bobgui_widget_compute_expand (BOBGUI_WIDGET (row), BOBGUI_ORIENTATION_VERTICAL))
        n_vexpand_children++;

      i++;

      if (ROW_PRIV (row)->header != NULL)
        {
          bobgui_widget_measure (ROW_PRIV (row)->header, BOBGUI_ORIENTATION_HORIZONTAL,
                              -1, &row_min, &row_nat, NULL, NULL);
          *minimum = MAX (*minimum, row_min);
          *natural = MAX (*natural, row_nat);

          if (for_height >= 0 && bobgui_widget_compute_expand (ROW_PRIV (row)->header, BOBGUI_ORIENTATION_VERTICAL))
            n_vexpand_children++;

          i++;
        }
    }

  if (for_height < 0)
    return;

  /* Binary search for the smallest width that lets us fit
   * into the suggested height.  */
  min = *minimum;
  max = G_MAXINT;

  while (min < max)
    {
      int test;

      /* We're most likely to be measured for a height that matches
       * our min or nat width, so start by checking around those
       * sizes.  */
      if (min == *minimum + 1 && max == *natural)
        test = max - 1;
      else if (max != G_MAXINT)
        test = (min + max) / 2;
      else if (min == *minimum)
        test = min;
      else if (min == *minimum + 1 && *natural >= min)
        test = *natural;
      else
        test = min * 2;

      bobgui_list_box_measure_height_for_width (box, test,
                                             &min_height, &nat_height,
                                             NULL);
      if (min_height > for_height)
        min = test + 1;
      else
        max = test;
    }

  *minimum = min;

  /* Now find the natural width */
  sizes = g_new (BobguiRequestedSize, i);
  bobgui_list_box_measure_height_for_width (box, -1,
                                         &min_height, &nat_height,
                                         sizes);
  extra_height = bobgui_distribute_natural_allocation (for_height - min_height,
                                                    i, sizes);
  *natural = 0;
  i = 0;

  for (iter = g_sequence_get_begin_iter (box->children);
       !g_sequence_iter_is_end (iter);
       iter = g_sequence_iter_next (iter))
    {
      int row_height, row_min, row_nat;

      row = g_sequence_get (iter);

      if (!bobgui_widget_get_visible (BOBGUI_WIDGET (row)))
        continue;

      row_height = sizes[i].minimum_size;
      if (bobgui_widget_compute_expand (BOBGUI_WIDGET (row), BOBGUI_ORIENTATION_VERTICAL))
        row_height += extra_height / n_vexpand_children;

      bobgui_widget_measure (BOBGUI_WIDGET (row), BOBGUI_ORIENTATION_HORIZONTAL,
                          row_height, &row_min, &row_nat,
                          NULL, NULL);

      *natural = MAX (*natural, row_nat);
      i++;

      if (ROW_PRIV (row)->header != NULL)
        {
          row_height = sizes[i].minimum_size;
          if (bobgui_widget_compute_expand (ROW_PRIV (row)->header, BOBGUI_ORIENTATION_VERTICAL))
            row_height += extra_height / n_vexpand_children;

          bobgui_widget_measure (ROW_PRIV (row)->header, BOBGUI_ORIENTATION_HORIZONTAL,
                              row_height, &row_min, &row_nat,
                              NULL, NULL);

          *natural = MAX (*natural, row_nat);
          i++;
        }
    }

  g_free (sizes);
}

static void
bobgui_list_box_measure (BobguiWidget     *widget,
                      BobguiOrientation orientation,
                      int            for_size,
                      int           *minimum,
                      int           *natural,
                      int           *minimum_baseline,
                      int           *natural_baseline)
{
  BobguiListBox *box = BOBGUI_LIST_BOX (widget);

  if (orientation == BOBGUI_ORIENTATION_HORIZONTAL)
    bobgui_list_box_measure_width_for_height (box, for_size,
                                           minimum, natural);
  else
    bobgui_list_box_measure_height_for_width (box, for_size,
                                           minimum, natural, NULL);
}

static void
bobgui_list_box_size_allocate (BobguiWidget *widget,
                            int        width,
                            int        height,
                            int        baseline)
{
  BobguiListBox *box = BOBGUI_LIST_BOX (widget);
  BobguiAllocation child_allocation;
  BobguiAllocation header_allocation;
  BobguiListBoxRow *row;
  GSequenceIter *iter;
  int child_min, child_nat;
  int total_min = 0, total_nat = 0;
  gboolean allocate_min = FALSE, allocate_nat = FALSE;
  BobguiRequestedSize *sizes = NULL;
  int i = 0;
  int n_vexpand_children = 0;
  int extra_height = height;


  child_allocation.x = 0;
  child_allocation.y = 0;
  child_allocation.width = width;
  child_allocation.height = 0;

  header_allocation.x = 0;
  header_allocation.y = 0;
  header_allocation.width = width;
  header_allocation.height = 0;

  if (box->placeholder && bobgui_widget_get_child_visible (box->placeholder))
    {
      bobgui_widget_measure (box->placeholder, BOBGUI_ORIENTATION_VERTICAL,
                          width,
                          &child_min, NULL, NULL, NULL);
      header_allocation.height = height;
      header_allocation.y = child_allocation.y;
      bobgui_widget_size_allocate (box->placeholder, &header_allocation, -1);
      child_allocation.y += child_min;
    }

  for (iter = g_sequence_get_begin_iter (box->children);
       !g_sequence_iter_is_end (iter);
       iter = g_sequence_iter_next (iter))
    {
      row = g_sequence_get (iter);
      if (!row_is_visible (row))
        continue;

      if (ROW_PRIV (row)->header != NULL)
        {
          bobgui_widget_measure (ROW_PRIV (row)->header, BOBGUI_ORIENTATION_VERTICAL,
                              width, &child_min, &child_nat, NULL, NULL);
          total_min += child_min;
          total_nat += child_nat;
          i++;

          if (bobgui_widget_compute_expand (ROW_PRIV (row)->header, BOBGUI_ORIENTATION_VERTICAL))
            n_vexpand_children++;
        }

      bobgui_widget_measure (BOBGUI_WIDGET (row), BOBGUI_ORIENTATION_VERTICAL,
                          width, &child_min, &child_nat, NULL, NULL);
      total_min += child_min;
      total_nat += child_nat;
      i++;

      if (bobgui_widget_compute_expand (BOBGUI_WIDGET (row), BOBGUI_ORIENTATION_VERTICAL))
        n_vexpand_children++;
    }

  /* We're most likely to be allocated either our minimum or natural
   * height, even more so when we're placed inside a BobguiScrolledWindow &
   * BobguiViewport. Detect these cases and skip the logic for distributing
   * sizes.
   */
  if (height == total_min)
    {
      allocate_min = TRUE;
      extra_height = 0;
      goto do_allocate;
    }
  else if (height >= total_nat)
    {
      allocate_nat = TRUE;
      extra_height = height - total_nat;
      goto do_allocate;
    }

  extra_height = height - total_min;
  sizes = g_new (BobguiRequestedSize, i);
  i = 0;

  for (iter = g_sequence_get_begin_iter (box->children);
       !g_sequence_iter_is_end (iter);
       iter = g_sequence_iter_next (iter))
    {
      row = g_sequence_get (iter);
      if (!row_is_visible (row))
        continue;

      if (ROW_PRIV (row)->header != NULL)
        {
          bobgui_widget_measure (ROW_PRIV (row)->header,
                              BOBGUI_ORIENTATION_VERTICAL, width,
                              &sizes[i].minimum_size,
                              &sizes[i].natural_size,
                              NULL, NULL);
          i++;
        }

      bobgui_widget_measure (BOBGUI_WIDGET (row),
                          BOBGUI_ORIENTATION_VERTICAL, width,
                          &sizes[i].minimum_size,
                          &sizes[i].natural_size,
                          NULL, NULL);
      i++;
    }

  extra_height = bobgui_distribute_natural_allocation (extra_height, i, sizes);

do_allocate:
  i = 0;
  for (iter = g_sequence_get_begin_iter (box->children);
       !g_sequence_iter_is_end (iter);
       iter = g_sequence_iter_next (iter))
    {
      row = g_sequence_get (iter);
      if (!row_is_visible (row))
        {
          ROW_PRIV (row)->y = child_allocation.y;
          ROW_PRIV (row)->height = 0;
          continue;
        }

      if (ROW_PRIV (row)->header != NULL)
        {
          if (allocate_min || allocate_nat)
            {
              bobgui_widget_measure (ROW_PRIV (row)->header,
                                  BOBGUI_ORIENTATION_VERTICAL, width,
                                  &child_min, &child_nat,
                                  NULL, NULL);
              if (allocate_min)
                header_allocation.height = child_min;
              else
                header_allocation.height = child_nat;
            }
          else
            header_allocation.height = sizes[i].minimum_size;

          if (bobgui_widget_compute_expand (ROW_PRIV (row)->header, BOBGUI_ORIENTATION_VERTICAL))
            header_allocation.height += extra_height / n_vexpand_children;
          header_allocation.y = child_allocation.y;
          bobgui_widget_size_allocate (ROW_PRIV (row)->header,
                                    &header_allocation,
                                    -1);
          child_allocation.y += header_allocation.height;
          i++;
        }

      if (allocate_min || allocate_nat)
        {
          bobgui_widget_measure (BOBGUI_WIDGET (row),
                              BOBGUI_ORIENTATION_VERTICAL, width,
                              &child_min, &child_nat,
                              NULL, NULL);
          if (allocate_min)
            child_allocation.height = child_min;
          else
            child_allocation.height = child_nat;
        }
      else
        child_allocation.height = sizes[i].minimum_size;

      if (bobgui_widget_compute_expand (BOBGUI_WIDGET (row), BOBGUI_ORIENTATION_VERTICAL))
        child_allocation.height += extra_height / n_vexpand_children;

      ROW_PRIV (row)->y = child_allocation.y;
      ROW_PRIV (row)->height = child_allocation.height;
      bobgui_widget_size_allocate (BOBGUI_WIDGET (row), &child_allocation, -1);
      child_allocation.y += child_allocation.height;
      i++;
    }

  g_free (sizes);
}

/**
 * bobgui_list_box_prepend:
 * @box: a `BobguiListBox`
 * @child: the `BobguiWidget` to add
 *
 * Prepend a widget to the list.
 *
 * If a sort function is set, the widget will
 * actually be inserted at the calculated position.
 */
void
bobgui_list_box_prepend (BobguiListBox *box,
                      BobguiWidget  *child)
{
  bobgui_list_box_insert (box, child, 0);
}

/**
 * bobgui_list_box_append:
 * @box: a `BobguiListBox`
 * @child: the `BobguiWidget` to add
 *
 * Append a widget to the list.
 *
 * If a sort function is set, the widget will
 * actually be inserted at the calculated position.
 */
void
bobgui_list_box_append (BobguiListBox *box,
                     BobguiWidget  *child)
{
  bobgui_list_box_insert (box, child, -1);
}

/**
 * bobgui_list_box_insert:
 * @box: a `BobguiListBox`
 * @child: the `BobguiWidget` to add
 * @position: the position to insert @child in
 *
 * Insert the @child into the @box at @position.
 *
 * If a sort function is
 * set, the widget will actually be inserted at the calculated position.
 *
 * If @position is -1, or larger than the total number of items in the
 * @box, then the @child will be appended to the end.
 */
void
bobgui_list_box_insert (BobguiListBox *box,
                     BobguiWidget  *child,
                     int         position)
{
  BobguiListBoxRow *row;
  GSequenceIter *prev = NULL;
  GSequenceIter *iter = NULL;

  g_return_if_fail (BOBGUI_IS_LIST_BOX (box));
  g_return_if_fail (BOBGUI_IS_WIDGET (child));

  if (BOBGUI_IS_LIST_BOX_ROW (child))
    row = BOBGUI_LIST_BOX_ROW (child);
  else
    {
      row = BOBGUI_LIST_BOX_ROW (bobgui_list_box_row_new ());
      bobgui_list_box_row_set_child (BOBGUI_LIST_BOX_ROW (row), child);
    }

  if (box->sort_func != NULL)
    iter = g_sequence_insert_sorted (box->children, row,
                                     (GCompareDataFunc)do_sort, box);
  else if (position == 0)
    iter = g_sequence_prepend (box->children, row);
  else if (position == -1)
    iter = g_sequence_append (box->children, row);
  else
    {
      GSequenceIter *current_iter;

      current_iter = g_sequence_get_iter_at_pos (box->children, position);
      iter = g_sequence_insert_before (current_iter, row);
    }

  ROW_PRIV (row)->iter = iter;
  prev = g_sequence_iter_prev (iter);
  bobgui_widget_insert_after (BOBGUI_WIDGET (row), BOBGUI_WIDGET (box),
                           prev != iter ? g_sequence_get (prev) : NULL);

  bobgui_widget_set_child_visible (BOBGUI_WIDGET (row), TRUE);
  ROW_PRIV (row)->visible = bobgui_widget_get_visible (BOBGUI_WIDGET (row));
  if (ROW_PRIV (row)->visible)
    list_box_add_visible_rows (box, 1);
  bobgui_list_box_apply_filter (box, row);
  bobgui_list_box_update_row (box, row);
  if (bobgui_widget_get_visible (BOBGUI_WIDGET (box)))
    {
      bobgui_list_box_update_header (box, ROW_PRIV (row)->iter);
      bobgui_list_box_update_header (box,
                                  bobgui_list_box_get_next_visible (box, ROW_PRIV (row)->iter));
    }
}

/**
 * bobgui_list_box_drag_unhighlight_row:
 * @box: a `BobguiListBox`
 *
 * If a row has previously been highlighted via bobgui_list_box_drag_highlight_row(),
 * it will have the highlight removed.
 */
void
bobgui_list_box_drag_unhighlight_row (BobguiListBox *box)
{
  g_return_if_fail (BOBGUI_IS_LIST_BOX (box));

  if (box->drag_highlighted_row == NULL)
    return;

  bobgui_widget_unset_state_flags (BOBGUI_WIDGET (box->drag_highlighted_row), BOBGUI_STATE_FLAG_DROP_ACTIVE);
  g_clear_object (&box->drag_highlighted_row);
}

/**
 * bobgui_list_box_drag_highlight_row:
 * @box: a `BobguiListBox`
 * @row: a `BobguiListBoxRow`
 *
 * Add a drag highlight to a row.
 *
 * This is a helper function for implementing DnD onto a `BobguiListBox`.
 * The passed in @row will be highlighted by setting the
 * %BOBGUI_STATE_FLAG_DROP_ACTIVE state and any previously highlighted
 * row will be unhighlighted.
 *
 * The row will also be unhighlighted when the widget gets
 * a drag leave event.
 */
void
bobgui_list_box_drag_highlight_row (BobguiListBox    *box,
                                 BobguiListBoxRow *row)
{
  g_return_if_fail (BOBGUI_IS_LIST_BOX (box));
  g_return_if_fail (BOBGUI_IS_LIST_BOX_ROW (row));

  if (box->drag_highlighted_row == row)
    return;

  bobgui_list_box_drag_unhighlight_row (box);
  bobgui_widget_set_state_flags (BOBGUI_WIDGET (row), BOBGUI_STATE_FLAG_DROP_ACTIVE, FALSE);
  box->drag_highlighted_row = g_object_ref (row);
}

static void
bobgui_list_box_activate_cursor_row (BobguiListBox *box)
{
  bobgui_list_box_select_and_activate (box, box->cursor_row);
}

static void
bobgui_list_box_toggle_cursor_row (BobguiListBox *box)
{
  if (box->cursor_row == NULL)
    return;

  if ((box->selection_mode == BOBGUI_SELECTION_SINGLE ||
       box->selection_mode == BOBGUI_SELECTION_MULTIPLE) &&
      ROW_PRIV (box->cursor_row)->selected)
    bobgui_list_box_unselect_row_internal (box, box->cursor_row);
  else
    bobgui_list_box_select_and_activate (box, box->cursor_row);
}

static void
bobgui_list_box_move_cursor (BobguiListBox      *box,
                          BobguiMovementStep  step,
                          int              count,
                          gboolean         extend,
                          gboolean         modify)
{
  BobguiListBoxRow *row;
  int page_size;
  GSequenceIter *iter;
  int start_y;
  int end_y;
  int height;

  row = NULL;
  switch ((guint) step)
    {
    case BOBGUI_MOVEMENT_BUFFER_ENDS:
      if (count < 0)
        row = bobgui_list_box_get_first_focusable (box);
      else
        row = bobgui_list_box_get_last_focusable (box);
      break;
    case BOBGUI_MOVEMENT_DISPLAY_LINES:
      if (box->cursor_row != NULL)
        {
          int i = count;

          iter = ROW_PRIV (box->cursor_row)->iter;

          while (i < 0  && iter != NULL)
            {
              iter = bobgui_list_box_get_previous_visible (box, iter);
              i = i + 1;
            }
          while (i > 0  && iter != NULL)
            {
              iter = bobgui_list_box_get_next_visible (box, iter);
              i = i - 1;
            }

          if (iter != NULL && !g_sequence_iter_is_end (iter))
            row = g_sequence_get (iter);
        }
      break;
    case BOBGUI_MOVEMENT_PAGES:
      page_size = 100;
      if (box->adjustment != NULL)
        page_size = bobgui_adjustment_get_page_increment (box->adjustment);

      if (box->cursor_row != NULL)
        {
          start_y = ROW_PRIV (box->cursor_row)->y;
          height = bobgui_widget_get_height (BOBGUI_WIDGET (box));
          end_y = CLAMP (start_y + page_size * count, 0, height - 1);
          row = bobgui_list_box_get_row_at_y (box, end_y);

          if (!row)
            {
              GSequenceIter *cursor_iter;
              GSequenceIter *next_iter;

              if (count > 0)
                {
                  cursor_iter = ROW_PRIV (box->cursor_row)->iter;
                  next_iter = bobgui_list_box_get_last_visible (box, cursor_iter);

                  if (next_iter)
                    {
                      row = g_sequence_get (next_iter);
                      end_y = ROW_PRIV (row)->y;
                    }
                }
              else
                {
                  row = bobgui_list_box_get_row_at_index (box, 0);
                  end_y = ROW_PRIV (row)->y;
                }
            }
          else if (row == box->cursor_row)
            {
              iter = ROW_PRIV (row)->iter;

              /* Move at least one row. This is important when the cursor_row's height is
               * greater than page_size */
              if (count < 0)
                iter = g_sequence_iter_prev (iter);
              else
                iter = g_sequence_iter_next (iter);

              if (!g_sequence_iter_is_begin (iter) && !g_sequence_iter_is_end (iter))
                {
                  row = g_sequence_get (iter);
                  end_y = ROW_PRIV (row)->y;
                }
            }

          if (end_y != start_y && box->adjustment != NULL)
            bobgui_adjustment_animate_to_value (box->adjustment, end_y);
        }
      break;
    default:
      return;
    }

  if (row == NULL || row == box->cursor_row)
    {
      BobguiDirectionType direction = count < 0 ? BOBGUI_DIR_UP : BOBGUI_DIR_DOWN;

      if (!bobgui_widget_keynav_failed (BOBGUI_WIDGET (box), direction))
        {
          BobguiWidget *toplevel = BOBGUI_WIDGET (bobgui_widget_get_root (BOBGUI_WIDGET (box)));

          if (toplevel)
            bobgui_widget_child_focus (toplevel,
                                    direction == BOBGUI_DIR_UP ?
                                    BOBGUI_DIR_TAB_BACKWARD :
                                    BOBGUI_DIR_TAB_FORWARD);

        }

      return;
    }

  bobgui_list_box_update_cursor (box, row, TRUE);
  if (!modify)
    bobgui_list_box_update_selection (box, row, FALSE, extend);
}


/**
 * bobgui_list_box_row_new:
 *
 * Creates a new `BobguiListBoxRow`.
 *
 * Returns: a new `BobguiListBoxRow`
 */
BobguiWidget *
bobgui_list_box_row_new (void)
{
  return g_object_new (BOBGUI_TYPE_LIST_BOX_ROW, NULL);
}

/**
 * bobgui_list_box_row_set_child:
 * @row: a `BobguiListBoxRow`
 * @child: (nullable): the child widget
 *
 * Sets the child widget of @self.
 */
void
bobgui_list_box_row_set_child (BobguiListBoxRow *row,
                            BobguiWidget     *child)
{
  BobguiListBoxRowPrivate *priv = ROW_PRIV (row);

  g_return_if_fail (BOBGUI_IS_LIST_BOX_ROW (row));
  g_return_if_fail (child == NULL || priv->child == child || bobgui_widget_get_parent (child) == NULL);

  if (priv->child == child)
    return;

  g_clear_pointer (&priv->child, bobgui_widget_unparent);

  priv->child = child;
  if (child)
    bobgui_widget_set_parent (child, BOBGUI_WIDGET (row));

  g_object_notify_by_pspec (G_OBJECT (row), row_properties[ROW_PROP_CHILD]);
}

/**
 * bobgui_list_box_row_get_child:
 * @row: a `BobguiListBoxRow`
 *
 * Gets the child widget of @row.
 *
 * Returns: (nullable) (transfer none): the child widget of @row
 */
BobguiWidget *
bobgui_list_box_row_get_child (BobguiListBoxRow *row)
{
  return ROW_PRIV (row)->child;
}

static void
bobgui_list_box_row_set_focus (BobguiListBoxRow *row)
{
  BobguiListBox *box = bobgui_list_box_row_get_box (row);

  if (!box)
    return;

  bobgui_list_box_update_selection (box, row, FALSE, FALSE);
}

static gboolean
bobgui_list_box_row_focus (BobguiWidget        *widget,
                        BobguiDirectionType  direction)
{
  BobguiListBoxRow *row = BOBGUI_LIST_BOX_ROW (widget);
  gboolean had_focus = FALSE;
  BobguiWidget *child = ROW_PRIV (row)->child;
  BobguiWidget *focus_child = bobgui_widget_get_focus_child (widget);

  g_object_get (widget, "has-focus", &had_focus, NULL);

  /* If a child has focus, always try to navigate within that first. */
  if (focus_child != NULL)
    {
      if (bobgui_widget_child_focus (focus_child, direction))
        return TRUE;
    }

  /* Otherwise, decide based on the direction. */
  if (direction == BOBGUI_DIR_RIGHT || direction == BOBGUI_DIR_TAB_FORWARD || direction == BOBGUI_DIR_DOWN)
    {
      /* If a child was focused and focus couldn't be moved within that (see
       * above), let focus leave. */
      if (focus_child != NULL)
        return FALSE;

      /* If the row is not focused, try to focus it. */
      if (!had_focus && bobgui_widget_grab_focus (widget))
        {
          bobgui_list_box_row_set_focus (row);
          return TRUE;
        }

      /* Finally, try to move focus into the child. */
      if (child != NULL && bobgui_widget_child_focus (child, direction))
        return TRUE;

      return FALSE;
    }
  else if (direction == BOBGUI_DIR_LEFT || direction == BOBGUI_DIR_TAB_BACKWARD || direction == BOBGUI_DIR_UP)
    {
      /* If the row itself is focused, let focus leave it. */
      if (had_focus)
        return FALSE;

      /* Otherwise, let focus enter the child widget, if possible. */
      if (child != NULL && bobgui_widget_child_focus (child, direction))
        return TRUE;

      /* If that didn't work, try to focus the row itself. */
      if (bobgui_widget_grab_focus (widget))
        {
          bobgui_list_box_row_set_focus (row);
          return TRUE;
        }

      return FALSE;
    }

  return FALSE;
}

static void
bobgui_list_box_row_activate (BobguiListBoxRow *row)
{
  BobguiListBox *box;

  box = bobgui_list_box_row_get_box (row);
  if (box)
    bobgui_list_box_select_and_activate (box, row);
}

static void
bobgui_list_box_row_show (BobguiWidget *widget)
{
  BobguiListBoxRow *row = BOBGUI_LIST_BOX_ROW (widget);
  BobguiListBox *box;

  BOBGUI_WIDGET_CLASS (bobgui_list_box_row_parent_class)->show (widget);

  box = bobgui_list_box_row_get_box (row);
  if (box)
    bobgui_list_box_row_visibility_changed (box, row);
}

static void
bobgui_list_box_row_hide (BobguiWidget *widget)
{
  BobguiListBoxRow *row = BOBGUI_LIST_BOX_ROW (widget);
  BobguiListBox *box;

  BOBGUI_WIDGET_CLASS (bobgui_list_box_row_parent_class)->hide (widget);

  box = bobgui_list_box_row_get_box (row);
  if (box)
    bobgui_list_box_row_visibility_changed (box, row);
}

static void
bobgui_list_box_row_root (BobguiWidget *widget)
{
  BobguiListBoxRow *row = BOBGUI_LIST_BOX_ROW (widget);
  BobguiListBox *box;

  BOBGUI_WIDGET_CLASS (bobgui_list_box_row_parent_class)->root (widget);

  box = bobgui_list_box_row_get_box (row);
  bobgui_list_box_update_row (box, row);
}

/**
 * bobgui_list_box_row_changed:
 * @row: a `BobguiListBoxRow`
 *
 * Marks @row as changed, causing any state that depends on this
 * to be updated.
 *
 * This affects sorting, filtering and headers.
 *
 * Note that calls to this method must be in sync with the data
 * used for the row functions. For instance, if the list is
 * mirroring some external data set, and *two* rows changed in the
 * external data set then when you call bobgui_list_box_row_changed()
 * on the first row the sort function must only read the new data
 * for the first of the two changed rows, otherwise the resorting
 * of the rows will be wrong.
 *
 * This generally means that if you don’t fully control the data
 * model you have to duplicate the data that affects the listbox
 * row functions into the row widgets themselves. Another alternative
 * is to call [method@Bobgui.ListBox.invalidate_sort] on any model change,
 * but that is more expensive.
 */
void
bobgui_list_box_row_changed (BobguiListBoxRow *row)
{
  BobguiListBox *box;

  g_return_if_fail (BOBGUI_IS_LIST_BOX_ROW (row));

  box = bobgui_list_box_row_get_box (row);
  if (box)
    bobgui_list_box_got_row_changed (box, row);
}

/**
 * bobgui_list_box_row_get_header:
 * @row: a `BobguiListBoxRow`
 *
 * Returns the current header of the @row.
 *
 * This can be used
 * in a [callback@Bobgui.ListBoxUpdateHeaderFunc] to see if
 * there is a header set already, and if so to update
 * the state of it.
 *
 * Returns: (transfer none) (nullable): the current header
 */
BobguiWidget *
bobgui_list_box_row_get_header (BobguiListBoxRow *row)
{
  g_return_val_if_fail (BOBGUI_IS_LIST_BOX_ROW (row), NULL);

  return ROW_PRIV (row)->header;
}

/**
 * bobgui_list_box_row_set_header:
 * @row: a `BobguiListBoxRow`
 * @header: (nullable): the header
 *
 * Sets the current header of the @row.
 *
 * This is only allowed to be called
 * from a [callback@Bobgui.ListBoxUpdateHeaderFunc].
 * It will replace any existing header in the row,
 * and be shown in front of the row in the listbox.
 */
void
bobgui_list_box_row_set_header (BobguiListBoxRow *row,
                             BobguiWidget     *header)
{
  BobguiListBoxRowPrivate *priv = ROW_PRIV (row);

  g_return_if_fail (BOBGUI_IS_LIST_BOX_ROW (row));
  g_return_if_fail (header == NULL || BOBGUI_IS_WIDGET (header));

  if (priv->header)
    g_object_unref (priv->header);

  priv->header = header;

  if (header)
    g_object_ref_sink (header);
}

/**
 * bobgui_list_box_row_get_index:
 * @row: a `BobguiListBoxRow`
 *
 * Gets the current index of the @row in its `BobguiListBox` container.
 *
 * Returns: the index of the @row, or -1 if the @row is not in a listbox
 */
int
bobgui_list_box_row_get_index (BobguiListBoxRow *row)
{
  BobguiListBoxRowPrivate *priv = ROW_PRIV (row);

  g_return_val_if_fail (BOBGUI_IS_LIST_BOX_ROW (row), -1);

  if (priv->iter != NULL)
    return g_sequence_iter_get_position (priv->iter);

  return -1;
}

/**
 * bobgui_list_box_row_is_selected:
 * @row: a `BobguiListBoxRow`
 *
 * Returns whether the child is currently selected in its
 * `BobguiListBox` container.
 *
 * Returns: %TRUE if @row is selected
 */
gboolean
bobgui_list_box_row_is_selected (BobguiListBoxRow *row)
{
  g_return_val_if_fail (BOBGUI_IS_LIST_BOX_ROW (row), FALSE);

  return ROW_PRIV (row)->selected;
}

/*< private >
 * bobgui_list_box_update_row:
 * @box: the list box
 * @row: the row
 *
 * Update the visual and accessible representation of a row.
 */
static void
bobgui_list_box_update_row (BobguiListBox    *box,
                         BobguiListBoxRow *row)
{
  gboolean can_select;

  if (box && box->selection_mode != BOBGUI_SELECTION_NONE)
    can_select = TRUE;
  else
    can_select = FALSE;

  if (ROW_PRIV (row)->activatable ||
      (ROW_PRIV (row)->selectable && can_select))
    bobgui_widget_add_css_class (BOBGUI_WIDGET (row), "activatable");
  else
    bobgui_widget_remove_css_class (BOBGUI_WIDGET (row), "activatable");

  if (ROW_PRIV (row)->selectable && can_select)
    bobgui_accessible_update_state (BOBGUI_ACCESSIBLE (row),
                                 BOBGUI_ACCESSIBLE_STATE_SELECTED, ROW_PRIV (row)->selected,
                                 -1);
  else
    bobgui_accessible_reset_state (BOBGUI_ACCESSIBLE (row),
                                BOBGUI_ACCESSIBLE_STATE_SELECTED);
}

static void
bobgui_list_box_update_rows (BobguiListBox *box)
{
  GSequenceIter *iter;

  for (iter = g_sequence_get_begin_iter (box->children);
       !g_sequence_iter_is_end (iter);
       iter = g_sequence_iter_next (iter))
    {
      BobguiListBoxRow *row = g_sequence_get (iter);
      bobgui_list_box_update_row (box, row);
    }
}

/**
 * bobgui_list_box_row_set_activatable:
 * @row: a `BobguiListBoxRow`
 * @activatable: %TRUE to mark the row as activatable
 *
 * Set whether the row is activatable.
 */
void
bobgui_list_box_row_set_activatable (BobguiListBoxRow *row,
                                  gboolean       activatable)
{
  g_return_if_fail (BOBGUI_IS_LIST_BOX_ROW (row));

  activatable = activatable != FALSE;

  if (ROW_PRIV (row)->activatable != activatable)
    {
      ROW_PRIV (row)->activatable = activatable;

      bobgui_list_box_update_row (bobgui_list_box_row_get_box (row), row);
      g_object_notify_by_pspec (G_OBJECT (row), row_properties[ROW_PROP_ACTIVATABLE]);
    }
}

/**
 * bobgui_list_box_row_get_activatable:
 * @row: a `BobguiListBoxRow`
 *
 * Gets whether the row is activatable.
 *
 * Returns: %TRUE if the row is activatable
 */
gboolean
bobgui_list_box_row_get_activatable (BobguiListBoxRow *row)
{
  g_return_val_if_fail (BOBGUI_IS_LIST_BOX_ROW (row), TRUE);

  return ROW_PRIV (row)->activatable;
}

/**
 * bobgui_list_box_row_set_selectable:
 * @row: a `BobguiListBoxRow`
 * @selectable: %TRUE to mark the row as selectable
 *
 * Set whether the row can be selected.
 */
void
bobgui_list_box_row_set_selectable (BobguiListBoxRow *row,
                                 gboolean       selectable)
{
  g_return_if_fail (BOBGUI_IS_LIST_BOX_ROW (row));

  selectable = selectable != FALSE;

  if (ROW_PRIV (row)->selectable != selectable)
    {
      if (!selectable)
        bobgui_list_box_row_set_selected (row, FALSE);

      ROW_PRIV (row)->selectable = selectable;

      bobgui_list_box_update_row (bobgui_list_box_row_get_box (row), row);

      g_object_notify_by_pspec (G_OBJECT (row), row_properties[ROW_PROP_SELECTABLE]);
    }
}

/**
 * bobgui_list_box_row_get_selectable:
 * @row: a `BobguiListBoxRow`
 *
 * Gets whether the row can be selected.
 *
 * Returns: %TRUE if the row is selectable
 */
gboolean
bobgui_list_box_row_get_selectable (BobguiListBoxRow *row)
{
  g_return_val_if_fail (BOBGUI_IS_LIST_BOX_ROW (row), TRUE);

  return ROW_PRIV (row)->selectable;
}

static void
bobgui_list_box_row_set_action_name (BobguiActionable *actionable,
                                  const char    *action_name)
{
  BobguiListBoxRow *row = BOBGUI_LIST_BOX_ROW (actionable);
  BobguiListBoxRowPrivate *priv = ROW_PRIV (row);

  if (!priv->action_helper)
    priv->action_helper = bobgui_action_helper_new (actionable);

  bobgui_action_helper_set_action_name (priv->action_helper, action_name);
}

static void
bobgui_list_box_row_set_action_target_value (BobguiActionable *actionable,
                                          GVariant      *action_target)
{
  BobguiListBoxRow *row = BOBGUI_LIST_BOX_ROW (actionable);
  BobguiListBoxRowPrivate *priv = ROW_PRIV (row);

  if (!priv->action_helper)
    priv->action_helper = bobgui_action_helper_new (actionable);

  bobgui_action_helper_set_action_target_value (priv->action_helper, action_target);
}

static void
bobgui_list_box_row_get_property (GObject    *obj,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  BobguiListBoxRow *row = BOBGUI_LIST_BOX_ROW (obj);

  switch (property_id)
    {
    case ROW_PROP_ACTIVATABLE:
      g_value_set_boolean (value, bobgui_list_box_row_get_activatable (row));
      break;
    case ROW_PROP_SELECTABLE:
      g_value_set_boolean (value, bobgui_list_box_row_get_selectable (row));
      break;
    case ROW_PROP_ACTION_NAME:
      g_value_set_string (value, bobgui_action_helper_get_action_name (ROW_PRIV (row)->action_helper));
      break;
    case ROW_PROP_ACTION_TARGET:
      g_value_set_variant (value, bobgui_action_helper_get_action_target_value (ROW_PRIV (row)->action_helper));
      break;
    case ROW_PROP_CHILD:
      g_value_set_object (value, bobgui_list_box_row_get_child (row));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, property_id, pspec);
      break;
    }
}

static void
bobgui_list_box_row_set_property (GObject      *obj,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  BobguiListBoxRow *row = BOBGUI_LIST_BOX_ROW (obj);

  switch (property_id)
    {
    case ROW_PROP_ACTIVATABLE:
      bobgui_list_box_row_set_activatable (row, g_value_get_boolean (value));
      break;
    case ROW_PROP_SELECTABLE:
      bobgui_list_box_row_set_selectable (row, g_value_get_boolean (value));
      break;
    case ROW_PROP_ACTION_NAME:
      bobgui_list_box_row_set_action_name (BOBGUI_ACTIONABLE (row), g_value_get_string (value));
      break;
    case ROW_PROP_ACTION_TARGET:
      bobgui_list_box_row_set_action_target_value (BOBGUI_ACTIONABLE (row), g_value_get_variant (value));
      break;
    case ROW_PROP_CHILD:
      bobgui_list_box_row_set_child (row, g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, property_id, pspec);
      break;
    }
}

static const char *
bobgui_list_box_row_get_action_name (BobguiActionable *actionable)
{
  BobguiListBoxRow *row = BOBGUI_LIST_BOX_ROW (actionable);

  return bobgui_action_helper_get_action_name (ROW_PRIV (row)->action_helper);
}

static GVariant *
bobgui_list_box_row_get_action_target_value (BobguiActionable *actionable)
{
  BobguiListBoxRow *row = BOBGUI_LIST_BOX_ROW (actionable);

  return bobgui_action_helper_get_action_target_value (ROW_PRIV (row)->action_helper);
}

static void
bobgui_list_box_row_actionable_iface_init (BobguiActionableInterface *iface)
{
  iface->get_action_name = bobgui_list_box_row_get_action_name;
  iface->set_action_name = bobgui_list_box_row_set_action_name;
  iface->get_action_target_value = bobgui_list_box_row_get_action_target_value;
  iface->set_action_target_value = bobgui_list_box_row_set_action_target_value;
}

static void
bobgui_list_box_row_finalize (GObject *obj)
{
  g_clear_object (&ROW_PRIV (BOBGUI_LIST_BOX_ROW (obj))->header);

  G_OBJECT_CLASS (bobgui_list_box_row_parent_class)->finalize (obj);
}

static void
bobgui_list_box_row_dispose (GObject *object)
{
  BobguiListBoxRow *row = BOBGUI_LIST_BOX_ROW (object);
  BobguiListBoxRowPrivate *priv = ROW_PRIV (row);

  g_clear_object (&priv->action_helper);
  g_clear_pointer (&priv->child, bobgui_widget_unparent);

  G_OBJECT_CLASS (bobgui_list_box_row_parent_class)->dispose (object);
}

static gboolean
bobgui_list_box_row_grab_focus (BobguiWidget *widget)
{
  BobguiListBoxRow *row = BOBGUI_LIST_BOX_ROW (widget);
  BobguiListBox *box = bobgui_list_box_row_get_box (row);

  g_return_val_if_fail (box != NULL, FALSE);

  if (bobgui_widget_grab_focus_self (widget))
    {
      if (box->cursor_row != row)
        bobgui_list_box_update_cursor (box, row, FALSE);

      return TRUE;
    }

  return FALSE;
}

static void
bobgui_list_box_row_class_init (BobguiListBoxRowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);

  object_class->get_property = bobgui_list_box_row_get_property;
  object_class->set_property = bobgui_list_box_row_set_property;
  object_class->finalize = bobgui_list_box_row_finalize;
  object_class->dispose = bobgui_list_box_row_dispose;

  widget_class->root = bobgui_list_box_row_root;
  widget_class->show = bobgui_list_box_row_show;
  widget_class->hide = bobgui_list_box_row_hide;
  widget_class->focus = bobgui_list_box_row_focus;
  widget_class->grab_focus = bobgui_list_box_row_grab_focus;

  klass->activate = bobgui_list_box_row_activate;

  /**
   * BobguiListBoxRow::activate:
   *
   * This is a keybinding signal, which will cause this row to be activated.
   *
   * If you want to be notified when the user activates a row (by key or not),
   * use the [signal@Bobgui.ListBox::row-activated] signal on the row’s parent
   * `BobguiListBox`.
   */
  row_signals[ROW__ACTIVATE] =
    g_signal_new (I_("activate"),
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (BobguiListBoxRowClass, activate),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);

  bobgui_widget_class_set_activate_signal (widget_class, row_signals[ROW__ACTIVATE]);

  /**
   * BobguiListBoxRow:activatable:
   *
   * Determines whether the ::row-activated
   * signal will be emitted for this row.
   */
  row_properties[ROW_PROP_ACTIVATABLE] =
    g_param_spec_boolean ("activatable", NULL, NULL,
                          TRUE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiListBoxRow:selectable:
   *
   * Determines whether this row can be selected.
   */
  row_properties[ROW_PROP_SELECTABLE] =
    g_param_spec_boolean ("selectable", NULL, NULL,
                          TRUE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiListBoxRow:child:
   *
   * The child widget.
   */
  row_properties[ROW_PROP_CHILD] =
    g_param_spec_object ("child", NULL, NULL,
                         BOBGUI_TYPE_WIDGET,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, LAST_ROW_PROPERTY, row_properties);

  g_object_class_override_property (object_class, ROW_PROP_ACTION_NAME, "action-name");
  g_object_class_override_property (object_class, ROW_PROP_ACTION_TARGET, "action-target");

  bobgui_widget_class_set_layout_manager_type (widget_class, BOBGUI_TYPE_BIN_LAYOUT);
  bobgui_widget_class_set_css_name (widget_class, I_("row"));
  bobgui_widget_class_set_accessible_role (widget_class, BOBGUI_ACCESSIBLE_ROLE_LIST_ITEM);
}

static void
bobgui_list_box_row_init (BobguiListBoxRow *row)
{
  ROW_PRIV (row)->activatable = TRUE;
  ROW_PRIV (row)->selectable = TRUE;

  bobgui_widget_set_focusable (BOBGUI_WIDGET (row), TRUE);
  bobgui_widget_add_css_class (BOBGUI_WIDGET (row), "activatable");
}

static void
bobgui_list_box_buildable_add_child (BobguiBuildable *buildable,
                                  BobguiBuilder   *builder,
                                  GObject      *child,
                                  const char   *type)
{
  if (type && strcmp (type, "placeholder") == 0)
    bobgui_list_box_set_placeholder (BOBGUI_LIST_BOX (buildable), BOBGUI_WIDGET (child));
  else if (BOBGUI_IS_WIDGET (child))
    bobgui_list_box_insert (BOBGUI_LIST_BOX (buildable), BOBGUI_WIDGET (child), -1);
  else
    parent_buildable_iface->add_child (buildable, builder, child, type);
}

static void
bobgui_list_box_buildable_interface_init (BobguiBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);

  iface->add_child = bobgui_list_box_buildable_add_child;
}

static void
bobgui_list_box_bound_model_changed (GListModel *list,
                                  guint       position,
                                  guint       removed,
                                  guint       added,
                                  gpointer    user_data)
{
  BobguiListBox *box = user_data;
  guint i;

  while (removed--)
    {
      BobguiListBoxRow *row;

      row = bobgui_list_box_get_row_at_index (box, position);
      bobgui_list_box_remove (box, BOBGUI_WIDGET (row));
    }

  for (i = 0; i < added; i++)
    {
      GObject *item;
      BobguiWidget *widget;

      item = g_list_model_get_item (list, position + i);
      widget = box->create_widget_func (item, box->create_widget_func_data);

      /* We allow the create_widget_func to either return a full
       * reference or a floating reference.  If we got the floating
       * reference, then turn it into a full reference now.  That means
       * that bobgui_list_box_insert() will take another full reference.
       * Finally, we'll release this full reference below, leaving only
       * the one held by the box.
       */
      if (g_object_is_floating (widget))
        g_object_ref_sink (widget);

      bobgui_list_box_insert (box, widget, position + i);

      g_object_unref (widget);
      g_object_unref (item);
    }
}

static void
bobgui_list_box_check_model_compat (BobguiListBox *box)
{
  if (box->bound_model &&
      (box->sort_func || box->filter_func))
    g_warning ("BobguiListBox with a model will ignore sort and filter functions");
}

/**
 * bobgui_list_box_bind_model:
 * @box: a `BobguiListBox`
 * @model: (nullable): the `GListModel` to be bound to @box
 * @create_widget_func: (nullable) (scope notified) (closure user_data) (destroy user_data_free_func): a function
 *   that creates widgets for items or %NULL in case you also passed %NULL as @model
 * @user_data: user data passed to @create_widget_func
 * @user_data_free_func: function for freeing @user_data
 *
 * Binds @model to @box.
 *
 * If @box was already bound to a model, that previous binding is
 * destroyed.
 *
 * The contents of @box are cleared and then filled with widgets that
 * represent items from @model. @box is updated whenever @model changes.
 * If @model is %NULL, @box is left empty.
 *
 * It is undefined to add or remove widgets directly (for example, with
 * [method@Bobgui.ListBox.insert]) while @box is bound to a model.
 *
 * Note that using a model is incompatible with the filtering and sorting
 * functionality in `BobguiListBox`. When using a model, filtering and sorting
 * should be implemented by the model.
 */
void
bobgui_list_box_bind_model (BobguiListBox                 *box,
                         GListModel                 *model,
                         BobguiListBoxCreateWidgetFunc  create_widget_func,
                         gpointer                    user_data,
                         GDestroyNotify              user_data_free_func)
{
  GSequenceIter *iter;

  g_return_if_fail (BOBGUI_IS_LIST_BOX (box));
  g_return_if_fail (model == NULL || G_IS_LIST_MODEL (model));
  g_return_if_fail (model == NULL || create_widget_func != NULL);

  if (box->bound_model)
    {
      if (box->create_widget_func_data_destroy)
        box->create_widget_func_data_destroy (box->create_widget_func_data);

      g_signal_handlers_disconnect_by_func (box->bound_model, bobgui_list_box_bound_model_changed, box);
      g_clear_object (&box->bound_model);
    }

  iter = g_sequence_get_begin_iter (box->children);
  while (!g_sequence_iter_is_end (iter))
    {
      BobguiWidget *row = g_sequence_get (iter);
      iter = g_sequence_iter_next (iter);
      bobgui_list_box_remove (box, row);
    }


  if (model == NULL)
    return;

  box->bound_model = g_object_ref (model);
  box->create_widget_func = create_widget_func;
  box->create_widget_func_data = user_data;
  box->create_widget_func_data_destroy = user_data_free_func;

  bobgui_list_box_check_model_compat (box);

  g_signal_connect (box->bound_model, "items-changed", G_CALLBACK (bobgui_list_box_bound_model_changed), box);
  bobgui_list_box_bound_model_changed (model, 0, 0, g_list_model_get_n_items (model), box);
}

/**
 * bobgui_list_box_set_show_separators:
 * @box: a `BobguiListBox`
 * @show_separators: %TRUE to show separators
 *
 * Sets whether the list box should show separators
 * between rows.
 */
void
bobgui_list_box_set_show_separators (BobguiListBox *box,
                                  gboolean    show_separators)
{
  g_return_if_fail (BOBGUI_IS_LIST_BOX (box));

  if (box->show_separators == show_separators)
    return;

  box->show_separators = show_separators;

  if (show_separators)
    bobgui_widget_add_css_class (BOBGUI_WIDGET (box), "separators");
  else
    bobgui_widget_remove_css_class (BOBGUI_WIDGET (box), "separators");

  g_object_notify_by_pspec (G_OBJECT (box), properties[PROP_SHOW_SEPARATORS]);
}

/**
 * bobgui_list_box_get_show_separators:
 * @box: a `BobguiListBox`
 *
 * Returns whether the list box should show separators
 * between rows.
 *
 * Returns: %TRUE if the list box shows separators
 */
gboolean
bobgui_list_box_get_show_separators (BobguiListBox *box)
{
  g_return_val_if_fail (BOBGUI_IS_LIST_BOX (box), FALSE);

  return box->show_separators;
}

/**
 * bobgui_list_box_set_tab_behavior:
 * @box: a `BobguiListBox`
 * @behavior: the tab behavior
 * 
 * Sets the behavior of the <kbd>Tab</kbd> and <kbd>Shift</kbd>+<kbd>Tab</kbd> keys.
 *
 * Since: 4.18
 */
void
bobgui_list_box_set_tab_behavior (BobguiListBox         *box,
                               BobguiListTabBehavior  behavior)
{
  g_return_if_fail (BOBGUI_IS_LIST_BOX (box));

  if (box->tab_behavior == behavior)
    return;

  box->tab_behavior = behavior;

  g_object_notify_by_pspec (G_OBJECT (box), properties[PROP_TAB_BEHAVIOR]);
}

/**
 * bobgui_list_box_get_tab_behavior:
 * @box: a `BobguiListBox`
 *
 * Returns the behavior of the <kbd>Tab</kbd> and <kbd>Shift</kbd>+<kbd>Tab</kbd> keys.
 *
 * Returns: the tab behavior
 * 
 * Since: 4.18
 */
BobguiListTabBehavior
bobgui_list_box_get_tab_behavior (BobguiListBox *box)
{
  g_return_val_if_fail (BOBGUI_IS_LIST_BOX (box), BOBGUI_LIST_TAB_ALL);

  return box->tab_behavior;
}
