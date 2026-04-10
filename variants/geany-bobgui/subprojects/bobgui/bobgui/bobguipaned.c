/* BOBGUI - The Bobgui Framework
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
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

/*
 * Modified by the BOBGUI Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the BOBGUI Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BOBGUI at ftp://ftp.bobgui.org/pub/bobgui/.
 */

#include "config.h"

#include "bobguipaned.h"

#include "bobguiaccessiblerange.h"
#include "bobguicssboxesprivate.h"
#include "bobguieventcontrollermotion.h"
#include "bobguigesturepan.h"
#include "bobguigesturesingle.h"
#include "bobguipanedhandleprivate.h"
#include "bobguimarshalers.h"
#include "bobguiorientable.h"
#include "bobguiprivate.h"
#include "bobguitypebuiltins.h"
#include "bobguiwidgetprivate.h"
#include "bobguibuildable.h"
#include "bobguibuilderprivate.h"

#include <math.h>

/**
 * BobguiPaned:
 *
 * Arranges its children in two panes, horizontally or vertically.
 *
 * <picture>
 *   <source srcset="panes-dark.png" media="(prefers-color-scheme: dark)">
 *   <img alt="An example BobguiPaned" src="panes.png">
 * </picture>
 *
 * The division between the two panes is adjustable by the user
 * by dragging a handle.
 *
 * Child widgets are added to the panes of the widget with
 * [method@Bobgui.Paned.set_start_child] and [method@Bobgui.Paned.set_end_child].
 * The division between the two children is set by default from the size
 * requests of the children, but it can be adjusted by the user.
 *
 * A paned widget draws a separator between the two child widgets and a
 * small handle that the user can drag to adjust the division. It does not
 * draw any relief around the children or around the separator. (The space
 * in which the separator is called the gutter.) Often, it is useful to put
 * each child inside a [class@Bobgui.Frame] so that the gutter appears as a
 * ridge. No separator is drawn if one of the children is missing.
 *
 * Each child has two options that can be set, "resize" and "shrink". If
 * "resize" is true then, when the `BobguiPaned` is resized, that child will
 * expand or shrink along with the paned widget. If "shrink" is true, then
 * that child can be made smaller than its requisition by the user.
 * Setting "shrink" to false allows the application to set a minimum size.
 * If "resize" is false for both children, then this is treated as if
 * "resize" is true for both children.
 *
 * The application can set the position of the slider as if it were set
 * by the user, by calling [method@Bobgui.Paned.set_position].
 *
 * # Shortcuts and Gestures
 *
 * The following signals have default keybindings:
 *
 * - [signal@Bobgui.Paned::accept-position]
 * - [signal@Bobgui.Paned::cancel-position]
 * - [signal@Bobgui.Paned::cycle-child-focus]
 * - [signal@Bobgui.Paned::cycle-handle-focus]
 * - [signal@Bobgui.Paned::move-handle]
 * - [signal@Bobgui.Paned::toggle-handle-focus]
 *
 * # CSS nodes
 *
 * ```
 * paned
 * ├── <child>
 * ├── separator[.wide]
 * ╰── <child>
 * ```
 *
 * `BobguiPaned` has a main CSS node with name paned, and a subnode for
 * the separator with name separator. The subnode gets a .wide style
 * class when the paned is supposed to be wide.
 *
 * In horizontal orientation, the nodes are arranged based on the text
 * direction, so in left-to-right mode, :first-child will select the
 * leftmost child, while it will select the rightmost child in
 * RTL layouts.
 *
 * ## Creating a paned widget with minimum sizes.
 *
 * ```c
 * BobguiWidget *hpaned = bobgui_paned_new (BOBGUI_ORIENTATION_HORIZONTAL);
 * BobguiWidget *frame1 = bobgui_frame_new (NULL);
 * BobguiWidget *frame2 = bobgui_frame_new (NULL);
 *
 * bobgui_widget_set_size_request (hpaned, 200, -1);
 *
 * bobgui_paned_set_start_child (BOBGUI_PANED (hpaned), frame1);
 * bobgui_paned_set_resize_start_child (BOBGUI_PANED (hpaned), TRUE);
 * bobgui_paned_set_shrink_start_child (BOBGUI_PANED (hpaned), FALSE);
 * bobgui_widget_set_size_request (frame1, 50, -1);
 *
 * bobgui_paned_set_end_child (BOBGUI_PANED (hpaned), frame2);
 * bobgui_paned_set_resize_end_child (BOBGUI_PANED (hpaned), FALSE);
 * bobgui_paned_set_shrink_end_child (BOBGUI_PANED (hpaned), FALSE);
 * bobgui_widget_set_size_request (frame2, 50, -1);
 * ```
 */

#define HANDLE_EXTRA_SIZE 6

typedef struct _BobguiPanedClass   BobguiPanedClass;

struct _BobguiPaned
{
  BobguiWidget parent_instance;

  BobguiPaned       *first_paned;
  BobguiWidget      *start_child;
  BobguiWidget      *end_child;
  BobguiWidget      *last_start_child_focus;
  BobguiWidget      *last_end_child_focus;
  BobguiWidget      *saved_focus;
  BobguiOrientation  orientation;

  BobguiWidget     *handle_widget;

  BobguiGesture    *pan_gesture;  /* Used for touch */
  BobguiGesture    *drag_gesture; /* Used for mice */

  int           start_child_size;
  int           drag_pos;
  int           last_allocation;
  int           max_position;
  int           min_position;
  int           original_position;

  guint         in_recursion  : 1;
  guint         resize_start_child : 1;
  guint         shrink_start_child : 1;
  guint         resize_end_child : 1;
  guint         shrink_end_child : 1;
  guint         position_set  : 1;
  guint         panning       : 1;
};

struct _BobguiPanedClass
{
  BobguiWidgetClass parent_class;

  gboolean (* cycle_child_focus)   (BobguiPaned      *paned,
                                    gboolean       reverse);
  gboolean (* toggle_handle_focus) (BobguiPaned      *paned);
  gboolean (* move_handle)         (BobguiPaned      *paned,
                                    BobguiScrollType  scroll);
  gboolean (* cycle_handle_focus)  (BobguiPaned      *paned,
                                    gboolean       reverse);
  gboolean (* accept_position)     (BobguiPaned      *paned);
  gboolean (* cancel_position)     (BobguiPaned      *paned);
};

enum {
  PROP_0,
  PROP_POSITION,
  PROP_POSITION_SET,
  PROP_MIN_POSITION,
  PROP_MAX_POSITION,
  PROP_WIDE_HANDLE,
  PROP_RESIZE_START_CHILD,
  PROP_RESIZE_END_CHILD,
  PROP_SHRINK_START_CHILD,
  PROP_SHRINK_END_CHILD,
  PROP_START_CHILD,
  PROP_END_CHILD,
  LAST_PROP,

  /* BobguiOrientable */
  PROP_ORIENTATION,
};

enum {
  CYCLE_CHILD_FOCUS,
  TOGGLE_HANDLE_FOCUS,
  MOVE_HANDLE,
  CYCLE_HANDLE_FOCUS,
  ACCEPT_POSITION,
  CANCEL_POSITION,
  LAST_SIGNAL
};

static void     bobgui_paned_set_property          (GObject          *object,
                                                 guint             prop_id,
                                                 const GValue     *value,
                                                 GParamSpec       *pspec);
static void     bobgui_paned_get_property          (GObject          *object,
                                                 guint             prop_id,
                                                 GValue           *value,
                                                 GParamSpec       *pspec);
static void     bobgui_paned_dispose               (GObject          *object);
static void     bobgui_paned_measure (BobguiWidget *widget,
                                   BobguiOrientation  orientation,
                                   int             for_size,
                                   int            *minimum,
                                   int            *natural,
                                   int            *minimum_baseline,
                                   int            *natural_baseline);
static void     bobgui_paned_size_allocate         (BobguiWidget           *widget,
                                                 int                  width,
                                                 int                  height,
                                                 int                  baseline);
static void     bobgui_paned_unrealize             (BobguiWidget           *widget);
static void     bobgui_paned_css_changed           (BobguiWidget           *widget,
                                                 BobguiCssStyleChange   *change);
static void     bobgui_paned_calc_position         (BobguiPaned         *paned,
                                                 int               allocation,
                                                 int               start_child_req,
                                                 int               end_child_req);
static void     bobgui_paned_set_focus_child       (BobguiWidget        *widget,
                                                 BobguiWidget        *child);
static void     bobgui_paned_set_saved_focus       (BobguiPaned         *paned,
                                                 BobguiWidget        *widget);
static void     bobgui_paned_set_first_paned       (BobguiPaned         *paned,
                                                 BobguiPaned         *first_paned);
static void     bobgui_paned_set_last_start_child_focus (BobguiPaned         *paned,
                                                 BobguiWidget        *widget);
static void     bobgui_paned_set_last_end_child_focus (BobguiPaned         *paned,
                                                 BobguiWidget        *widget);
static gboolean bobgui_paned_cycle_child_focus     (BobguiPaned         *paned,
                                                 gboolean          reverse);
static gboolean bobgui_paned_cycle_handle_focus    (BobguiPaned         *paned,
                                                 gboolean          reverse);
static gboolean bobgui_paned_move_handle           (BobguiPaned         *paned,
                                                 BobguiScrollType     scroll);
static gboolean bobgui_paned_accept_position       (BobguiPaned         *paned);
static gboolean bobgui_paned_cancel_position       (BobguiPaned         *paned);
static gboolean bobgui_paned_toggle_handle_focus   (BobguiPaned         *paned);

static void     update_drag                     (BobguiPaned         *paned,
                                                 int               xpos,
                                                 int               ypos);

static void bobgui_paned_buildable_iface_init (BobguiBuildableIface *iface);

static void bobgui_paned_accessible_range_init (BobguiAccessibleRangeInterface *iface);

G_DEFINE_TYPE_WITH_CODE (BobguiPaned, bobgui_paned, BOBGUI_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_ORIENTABLE, NULL)
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_ACCESSIBLE_RANGE,
                                                bobgui_paned_accessible_range_init)
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_BUILDABLE,
                                                bobgui_paned_buildable_iface_init))

static guint signals[LAST_SIGNAL] = { 0 };
static GParamSpec *paned_props[LAST_PROP] = { NULL, };

static void
add_tab_bindings (BobguiWidgetClass  *widget_class,
                  GdkModifierType  modifiers)
{
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_Tab, modifiers,
                                       "toggle-handle-focus",
                                       NULL);
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_KP_Tab, modifiers,
                                       "toggle-handle-focus",
                                       NULL);
}

static void
add_move_binding (BobguiWidgetClass  *widget_class,
                  guint            keyval,
                  GdkModifierType  mask,
                  BobguiScrollType    scroll)
{
  bobgui_widget_class_add_binding_signal (widget_class,
                                       keyval, mask,
                                       "move-handle",
                                       "(i)", scroll);
}

static void
get_handle_area (BobguiPaned        *paned,
                 graphene_rect_t *area)
{
  int extra = 0;

  if (!bobgui_widget_compute_bounds (paned->handle_widget, BOBGUI_WIDGET (paned), area))
    return;

  if (!bobgui_paned_get_wide_handle (paned))
    extra = HANDLE_EXTRA_SIZE;

  graphene_rect_inset (area, - extra, - extra);
}

static void
bobgui_paned_compute_expand (BobguiWidget *widget,
                          gboolean  *hexpand,
                          gboolean  *vexpand)
{
  BobguiPaned *paned = BOBGUI_PANED (widget);
  gboolean h = FALSE;
  gboolean v = FALSE;

  if (paned->start_child)
    {
      h = h || bobgui_widget_compute_expand (paned->start_child, BOBGUI_ORIENTATION_HORIZONTAL);
      v = v || bobgui_widget_compute_expand (paned->start_child, BOBGUI_ORIENTATION_VERTICAL);
    }

  if (paned->end_child)
    {
      h = h || bobgui_widget_compute_expand (paned->end_child, BOBGUI_ORIENTATION_HORIZONTAL);
      v = v || bobgui_widget_compute_expand (paned->end_child, BOBGUI_ORIENTATION_VERTICAL);
    }

  *hexpand = h;
  *vexpand = v;
}

static BobguiSizeRequestMode
bobgui_paned_get_request_mode (BobguiWidget *widget)
{
  BobguiPaned *paned = BOBGUI_PANED (widget);
  int wfh = 0, hfw = 0;

  if (paned->start_child)
    {
      switch (bobgui_widget_get_request_mode (paned->start_child))
        {
        case BOBGUI_SIZE_REQUEST_HEIGHT_FOR_WIDTH:
          hfw++;
          break;
        case BOBGUI_SIZE_REQUEST_WIDTH_FOR_HEIGHT:
          wfh++;
          break;
        case BOBGUI_SIZE_REQUEST_CONSTANT_SIZE:
        default:
          break;
        }
    }
  if (paned->end_child)
    {
      switch (bobgui_widget_get_request_mode (paned->end_child))
        {
        case BOBGUI_SIZE_REQUEST_HEIGHT_FOR_WIDTH:
          hfw++;
          break;
        case BOBGUI_SIZE_REQUEST_WIDTH_FOR_HEIGHT:
          wfh++;
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
bobgui_paned_set_orientation (BobguiPaned       *self,
                           BobguiOrientation  orientation)
{
  if (self->orientation != orientation)
    {
      static const char *cursor_name[2] = {
        "col-resize",
        "row-resize",
      };

      self->orientation = orientation;

      bobgui_widget_update_orientation (BOBGUI_WIDGET (self), self->orientation);
      bobgui_widget_set_cursor_from_name (self->handle_widget,
                                       cursor_name[orientation]);
      bobgui_gesture_pan_set_orientation (BOBGUI_GESTURE_PAN (self->pan_gesture),
                                       orientation);

      bobgui_widget_queue_resize (BOBGUI_WIDGET (self));
      g_object_notify (G_OBJECT (self), "orientation");
    }
}

static void
bobgui_paned_class_init (BobguiPanedClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (class);

  object_class->set_property = bobgui_paned_set_property;
  object_class->get_property = bobgui_paned_get_property;
  object_class->dispose = bobgui_paned_dispose;

  widget_class->measure = bobgui_paned_measure;
  widget_class->size_allocate = bobgui_paned_size_allocate;
  widget_class->unrealize = bobgui_paned_unrealize;
  widget_class->focus = bobgui_widget_focus_child;
  widget_class->set_focus_child = bobgui_paned_set_focus_child;
  widget_class->css_changed = bobgui_paned_css_changed;
  widget_class->get_request_mode = bobgui_paned_get_request_mode;
  widget_class->compute_expand = bobgui_paned_compute_expand;

  class->cycle_child_focus = bobgui_paned_cycle_child_focus;
  class->toggle_handle_focus = bobgui_paned_toggle_handle_focus;
  class->move_handle = bobgui_paned_move_handle;
  class->cycle_handle_focus = bobgui_paned_cycle_handle_focus;
  class->accept_position = bobgui_paned_accept_position;
  class->cancel_position = bobgui_paned_cancel_position;

  /**
   * BobguiPaned:position:
   *
   * Position of the separator in pixels, from the left/top.
   */
  paned_props[PROP_POSITION] =
    g_param_spec_int ("position", NULL, NULL,
                      0, G_MAXINT, 0,
                      BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiPaned:position-set:
   *
   * Whether the [property@Bobgui.Paned:position] property has been set.
   */
  paned_props[PROP_POSITION_SET] =
    g_param_spec_boolean ("position-set", NULL, NULL,
                          FALSE,
                          BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiPaned:min-position:
   *
   * The smallest possible value for the [property@Bobgui.Paned:position]
   * property.
   *
   * This property is derived from the size and shrinkability
   * of the widget's children.
   */
  paned_props[PROP_MIN_POSITION] =
    g_param_spec_int ("min-position", NULL, NULL,
                      0, G_MAXINT, 0,
                      BOBGUI_PARAM_READABLE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiPaned:max-position:
   *
   * The largest possible value for the [property@Bobgui.Paned:position]
   * property.
   *
   * This property is derived from the size and shrinkability
   * of the widget's children.
   */
  paned_props[PROP_MAX_POSITION] =
    g_param_spec_int ("max-position", NULL, NULL,
                      0, G_MAXINT, G_MAXINT,
                      BOBGUI_PARAM_READABLE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiPaned:wide-handle:
   *
   * Whether the `BobguiPaned` should provide a stronger visual separation.
   *
   * For example, this could be set when a paned contains two
   * [class@Bobgui.Notebook]s, whose tab rows would otherwise merge visually.
   */
  paned_props[PROP_WIDE_HANDLE] =
    g_param_spec_boolean ("wide-handle", NULL, NULL,
                          FALSE,
                          BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiPaned:resize-start-child:
   *
   * Determines whether the first child expands and shrinks
   * along with the paned widget.
   */
  paned_props[PROP_RESIZE_START_CHILD] =
    g_param_spec_boolean ("resize-start-child", NULL, NULL,
                          TRUE,
                          BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiPaned:resize-end-child:
   *
   * Determines whether the second child expands and shrinks
   * along with the paned widget.
   */
  paned_props[PROP_RESIZE_END_CHILD] =
    g_param_spec_boolean ("resize-end-child", NULL, NULL,
                          TRUE,
                          BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiPaned:shrink-start-child:
   *
   * Determines whether the first child can be made smaller
   * than its requisition.
   */
  paned_props[PROP_SHRINK_START_CHILD] =
    g_param_spec_boolean ("shrink-start-child", NULL, NULL,
                          TRUE,
                          BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiPaned:shrink-end-child:
   *
   * Determines whether the second child can be made smaller
   * than its requisition.
   */
  paned_props[PROP_SHRINK_END_CHILD] =
    g_param_spec_boolean ("shrink-end-child", NULL, NULL,
                          TRUE,
                          BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiPaned:start-child:
   *
   * The first child.
   */
  paned_props[PROP_START_CHILD] =
    g_param_spec_object ("start-child", NULL, NULL,
                          BOBGUI_TYPE_WIDGET,
                          BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiPaned:end-child:
   *
   * The second child.
   */
  paned_props[PROP_END_CHILD] =
    g_param_spec_object ("end-child", NULL, NULL,
                          BOBGUI_TYPE_WIDGET,
                          BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, LAST_PROP, paned_props);
  g_object_class_override_property (object_class, PROP_ORIENTATION, "orientation");

  /**
   * BobguiPaned::cycle-child-focus:
   * @widget: the object that received the signal
   * @reversed: whether cycling backward or forward
   *
   * Emitted to cycle the focus between the children of the paned.
   *
   * This is a [keybinding signal](class.SignalAction.html).
   *
   * The default binding is <kbd>F6</kbd>.
   *
   * Returns: whether the behavior was cycled
   */
  signals [CYCLE_CHILD_FOCUS] =
    g_signal_new (I_("cycle-child-focus"),
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (BobguiPanedClass, cycle_child_focus),
                  NULL, NULL,
                  _bobgui_marshal_BOOLEAN__BOOLEAN,
                  G_TYPE_BOOLEAN, 1,
                  G_TYPE_BOOLEAN);
  g_signal_set_va_marshaller (signals[CYCLE_CHILD_FOCUS],
                              G_TYPE_FROM_CLASS (object_class),
                              _bobgui_marshal_BOOLEAN__BOOLEANv);

  /**
   * BobguiPaned::toggle-handle-focus:
   * @widget: the object that received the signal
   *
   * Emitted to accept the current position of the handle and then
   * move focus to the next widget in the focus chain.
   *
   * This is a [keybinding signal](class.SignalAction.html).
   *
   * The default binding is <kbd>Tab</kbd>.
   *
   * Return: whether handle focus was toggled
   */
  signals [TOGGLE_HANDLE_FOCUS] =
    g_signal_new (I_("toggle-handle-focus"),
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (BobguiPanedClass, toggle_handle_focus),
                  NULL, NULL,
                  _bobgui_marshal_BOOLEAN__VOID,
                  G_TYPE_BOOLEAN, 0);
  g_signal_set_va_marshaller (signals[TOGGLE_HANDLE_FOCUS],
                              G_TYPE_FROM_CLASS (object_class),
                              _bobgui_marshal_BOOLEAN__VOIDv);

  /**
   * BobguiPaned::move-handle:
   * @widget: the object that received the signal
   * @scroll_type: a `BobguiScrollType`
   *
   * Emitted to move the handle with key bindings.
   *
   * This is a [keybinding signal](class.SignalAction.html).
   *
   * The default bindings for this signal are
   * <kbd>Ctrl</kbd>+<kbd>←</kbd>, <kbd>←</kbd>,
   * <kbd>Ctrl</kbd>+<kbd>→</kbd>, <kbd>→</kbd>,
   * <kbd>Ctrl</kbd>+<kbd>↑</kbd>, <kbd>↑</kbd>,
   * <kbd>Ctrl</kbd>+<kbd>↓</kbd>, <kbd>↓</kbd>,
   * <kbd>PgUp</kbd>, <kbd>PgDn</kbd>, <kbd>Home</kbd>, <kbd>End</kbd>.
   *
   * Returns: whether the handle was moved
   */
  signals[MOVE_HANDLE] =
    g_signal_new (I_("move-handle"),
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (BobguiPanedClass, move_handle),
                  NULL, NULL,
                  _bobgui_marshal_BOOLEAN__ENUM,
                  G_TYPE_BOOLEAN, 1,
                  BOBGUI_TYPE_SCROLL_TYPE);
  g_signal_set_va_marshaller (signals[MOVE_HANDLE],
                              G_TYPE_FROM_CLASS (object_class),
                              _bobgui_marshal_BOOLEAN__ENUMv);

  /**
   * BobguiPaned::cycle-handle-focus:
   * @widget: the object that received the signal
   * @reversed: whether cycling backward or forward
   *
   * Emitted to cycle whether the paned should grab focus to allow
   * the user to change position of the handle by using key bindings.
   *
   * This is a [keybinding signal](class.SignalAction.html).
   *
   * The default binding for this signal is <kbd>F8</kbd>.
   *
   * Returns: whether the behavior was cycled
   */
  signals [CYCLE_HANDLE_FOCUS] =
    g_signal_new (I_("cycle-handle-focus"),
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (BobguiPanedClass, cycle_handle_focus),
                  NULL, NULL,
                  _bobgui_marshal_BOOLEAN__BOOLEAN,
                  G_TYPE_BOOLEAN, 1,
                  G_TYPE_BOOLEAN);
  g_signal_set_va_marshaller (signals[CYCLE_HANDLE_FOCUS],
                              G_TYPE_FROM_CLASS (object_class),
                              _bobgui_marshal_BOOLEAN__BOOLEANv);

  /**
   * BobguiPaned::accept-position:
   * @widget: the object that received the signal
   *
   * Emitted to accept the current position of the handle when
   * moving it using key bindings.
   *
   * This is a [keybinding signal](class.SignalAction.html).
   *
   * The default binding for this signal is <kbd>Return</kbd> or
   * <kbd>Space</kbd>.
   *
   * Returns: whether the position was accepted
   */
  signals [ACCEPT_POSITION] =
    g_signal_new (I_("accept-position"),
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (BobguiPanedClass, accept_position),
                  NULL, NULL,
                  _bobgui_marshal_BOOLEAN__VOID,
                  G_TYPE_BOOLEAN, 0);
  g_signal_set_va_marshaller (signals[ACCEPT_POSITION],
                              G_TYPE_FROM_CLASS (object_class),
                              _bobgui_marshal_BOOLEAN__VOIDv);

  /**
   * BobguiPaned::cancel-position:
   * @widget: the object that received the signal
   *
   * Emitted to cancel moving the position of the handle using key
   * bindings.
   *
   * The position of the handle will be reset to the value prior to
   * moving it.
   *
   * This is a [keybinding signal](class.SignalAction.html).
   *
   * The default binding for this signal is <kbd>Escape</kbd>.
   *
   * Returns: whether the position was canceled
   */
  signals [CANCEL_POSITION] =
    g_signal_new (I_("cancel-position"),
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (BobguiPanedClass, cancel_position),
                  NULL, NULL,
                  _bobgui_marshal_BOOLEAN__VOID,
                  G_TYPE_BOOLEAN, 0);
  g_signal_set_va_marshaller (signals[CANCEL_POSITION],
                              G_TYPE_FROM_CLASS (object_class),
                              _bobgui_marshal_BOOLEAN__VOIDv);

  /* F6 and friends */
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_F6, 0,
                                       "cycle-child-focus",
                                       "(b)", FALSE);
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_F6, GDK_SHIFT_MASK,
                                       "cycle-child-focus",
                                       "(b)", TRUE);

  /* F8 and friends */
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_F8, 0,
                                       "cycle-handle-focus",
                                       "(b)", FALSE);
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_F8, GDK_SHIFT_MASK,
                                       "cycle-handle-focus",
                                       "(b)", TRUE);

  add_tab_bindings (widget_class, 0);
  add_tab_bindings (widget_class, GDK_CONTROL_MASK);
  add_tab_bindings (widget_class, GDK_SHIFT_MASK);
  add_tab_bindings (widget_class, GDK_CONTROL_MASK | GDK_SHIFT_MASK);

  /* accept and cancel positions */
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_Escape, 0,
                                       "cancel-position",
                                       NULL);

  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_Return, 0,
                                       "accept-position",
                                       NULL);
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_ISO_Enter, 0,
                                       "accept-position",
                                       NULL);
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_KP_Enter, 0,
                                       "accept-position",
                                       NULL);
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_space, 0,
                                       "accept-position",
                                       NULL);
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_KP_Space, 0,
                                       "accept-position",
                                       NULL);

  /* move handle */
  add_move_binding (widget_class, GDK_KEY_Left, 0, BOBGUI_SCROLL_STEP_LEFT);
  add_move_binding (widget_class, GDK_KEY_KP_Left, 0, BOBGUI_SCROLL_STEP_LEFT);
  add_move_binding (widget_class, GDK_KEY_Left, GDK_CONTROL_MASK, BOBGUI_SCROLL_PAGE_LEFT);
  add_move_binding (widget_class, GDK_KEY_KP_Left, GDK_CONTROL_MASK, BOBGUI_SCROLL_PAGE_LEFT);

  add_move_binding (widget_class, GDK_KEY_Right, 0, BOBGUI_SCROLL_STEP_RIGHT);
  add_move_binding (widget_class, GDK_KEY_Right, GDK_CONTROL_MASK, BOBGUI_SCROLL_PAGE_RIGHT);
  add_move_binding (widget_class, GDK_KEY_KP_Right, 0, BOBGUI_SCROLL_STEP_RIGHT);
  add_move_binding (widget_class, GDK_KEY_KP_Right, GDK_CONTROL_MASK, BOBGUI_SCROLL_PAGE_RIGHT);

  add_move_binding (widget_class, GDK_KEY_Up, 0, BOBGUI_SCROLL_STEP_UP);
  add_move_binding (widget_class, GDK_KEY_Up, GDK_CONTROL_MASK, BOBGUI_SCROLL_PAGE_UP);
  add_move_binding (widget_class, GDK_KEY_KP_Up, 0, BOBGUI_SCROLL_STEP_UP);
  add_move_binding (widget_class, GDK_KEY_KP_Up, GDK_CONTROL_MASK, BOBGUI_SCROLL_PAGE_UP);
  add_move_binding (widget_class, GDK_KEY_Page_Up, 0, BOBGUI_SCROLL_PAGE_UP);
  add_move_binding (widget_class, GDK_KEY_KP_Page_Up, 0, BOBGUI_SCROLL_PAGE_UP);

  add_move_binding (widget_class, GDK_KEY_Down, 0, BOBGUI_SCROLL_STEP_DOWN);
  add_move_binding (widget_class, GDK_KEY_Down, GDK_CONTROL_MASK, BOBGUI_SCROLL_PAGE_DOWN);
  add_move_binding (widget_class, GDK_KEY_KP_Down, 0, BOBGUI_SCROLL_STEP_DOWN);
  add_move_binding (widget_class, GDK_KEY_KP_Down, GDK_CONTROL_MASK, BOBGUI_SCROLL_PAGE_DOWN);
  add_move_binding (widget_class, GDK_KEY_Page_Down, 0, BOBGUI_SCROLL_PAGE_RIGHT);
  add_move_binding (widget_class, GDK_KEY_KP_Page_Down, 0, BOBGUI_SCROLL_PAGE_RIGHT);

  add_move_binding (widget_class, GDK_KEY_Home, 0, BOBGUI_SCROLL_START);
  add_move_binding (widget_class, GDK_KEY_KP_Home, 0, BOBGUI_SCROLL_START);
  add_move_binding (widget_class, GDK_KEY_End, 0, BOBGUI_SCROLL_END);
  add_move_binding (widget_class, GDK_KEY_KP_End, 0, BOBGUI_SCROLL_END);

  bobgui_widget_class_set_css_name (widget_class, I_("paned"));
}

static BobguiBuildableIface *parent_buildable_iface;

static void
bobgui_paned_buildable_add_child (BobguiBuildable *buildable,
                               BobguiBuilder   *builder,
                               GObject      *child,
                               const char   *type)
{
  BobguiPaned *self = BOBGUI_PANED (buildable);

  if (g_strcmp0 (type, "start") == 0)
    {
      bobgui_buildable_child_deprecation_warning (buildable, builder, "start", "start-child");
      bobgui_paned_set_start_child (self, BOBGUI_WIDGET (child));
      bobgui_paned_set_resize_start_child (self, FALSE);
      bobgui_paned_set_shrink_start_child (self, TRUE);
    }
  else if (g_strcmp0 (type, "end") == 0)
    {
      bobgui_buildable_child_deprecation_warning (buildable, builder, "end", "end-child");
      bobgui_paned_set_end_child (self, BOBGUI_WIDGET (child));
      bobgui_paned_set_resize_end_child (self, TRUE);
      bobgui_paned_set_shrink_end_child (self, TRUE);
    }
  else if (type == NULL && BOBGUI_IS_WIDGET (child))
    {
      bobgui_buildable_child_deprecation_warning (buildable, builder, NULL, "start-child or end-child");
      if (self->start_child == NULL)
        {
          bobgui_paned_set_start_child (self, BOBGUI_WIDGET (child));
          bobgui_paned_set_resize_start_child (self, FALSE);
          bobgui_paned_set_shrink_start_child (self, TRUE);
        }
      else if (self->end_child == NULL)
        {
          bobgui_paned_set_end_child (self, BOBGUI_WIDGET (child));
          bobgui_paned_set_resize_end_child (self, TRUE);
          bobgui_paned_set_shrink_end_child (self, TRUE);
        }
      else
        g_warning ("BobguiPaned only accepts two widgets as children");
    }
  else
    parent_buildable_iface->add_child (buildable, builder, child, type);
}

static void
bobgui_paned_buildable_iface_init (BobguiBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);
  iface->add_child = bobgui_paned_buildable_add_child;
}

static gboolean
accessible_range_set_current_value (BobguiAccessibleRange *accessible_range,
                                    double              value)
{
  bobgui_paned_set_position (BOBGUI_PANED (accessible_range), (int) value + 0.5);
  return TRUE;
}

static void
bobgui_paned_accessible_range_init (BobguiAccessibleRangeInterface *iface)
{
  iface->set_current_value = accessible_range_set_current_value;
}

static gboolean
initiates_touch_drag (BobguiPaned *paned,
                      double    start_x,
                      double    start_y)
{
  int handle_size, handle_pos, drag_pos;
  graphene_rect_t handle_area;

#define TOUCH_EXTRA_AREA_WIDTH 50
  get_handle_area (paned, &handle_area);

  if (paned->orientation == BOBGUI_ORIENTATION_HORIZONTAL)
    {
      handle_pos = handle_area.origin.x;
      drag_pos = start_x;
      handle_size = handle_area.size.width;
    }
  else
    {
      handle_pos = handle_area.origin.y;
      drag_pos = start_y;
      handle_size = handle_area.size.height;
    }

  if (drag_pos < handle_pos - TOUCH_EXTRA_AREA_WIDTH ||
      drag_pos > handle_pos + handle_size + TOUCH_EXTRA_AREA_WIDTH)
    return FALSE;

#undef TOUCH_EXTRA_AREA_WIDTH

  return TRUE;
}

static void
gesture_drag_begin_cb (BobguiGestureDrag *gesture,
                       double          start_x,
                       double          start_y,
                       BobguiPaned       *paned)
{
  GdkEventSequence *sequence;
  graphene_rect_t handle_area;
  GdkEvent *event;
  GdkDevice *device;
  gboolean is_touch;

  /* Only drag the handle when it's visible */
  if (!bobgui_widget_get_child_visible (paned->handle_widget))
    {
      bobgui_gesture_set_state (BOBGUI_GESTURE (gesture),
                             BOBGUI_EVENT_SEQUENCE_DENIED);
      return;
    }

  sequence = bobgui_gesture_single_get_current_sequence (BOBGUI_GESTURE_SINGLE (gesture));
  event = bobgui_gesture_get_last_event (BOBGUI_GESTURE (gesture), sequence);
  device = gdk_event_get_device (event);
  paned->panning = FALSE;

  is_touch = (gdk_event_get_event_type (event) == GDK_TOUCH_BEGIN ||
              gdk_device_get_source (device) == GDK_SOURCE_TOUCHSCREEN);

  get_handle_area (paned, &handle_area);

  if ((is_touch && BOBGUI_GESTURE (gesture) == paned->drag_gesture) ||
      (!is_touch && BOBGUI_GESTURE (gesture) == paned->pan_gesture))
    {
      bobgui_gesture_set_state (BOBGUI_GESTURE (gesture),
                             BOBGUI_EVENT_SEQUENCE_DENIED);
      return;
    }

  if (graphene_rect_contains_point (&handle_area, &(graphene_point_t){start_x, start_y}) ||
      (is_touch && initiates_touch_drag (paned, start_x, start_y)))
    {
      if (paned->orientation == BOBGUI_ORIENTATION_HORIZONTAL)
        paned->drag_pos = start_x - handle_area.origin.x;
      else
        paned->drag_pos = start_y - handle_area.origin.y;

      if (!bobgui_paned_get_wide_handle (paned))
        paned->drag_pos -= HANDLE_EXTRA_SIZE;

      paned->panning = TRUE;

      bobgui_gesture_set_state (BOBGUI_GESTURE (gesture),
                             BOBGUI_EVENT_SEQUENCE_CLAIMED);
    }
  else
    {
      bobgui_gesture_set_state (BOBGUI_GESTURE (gesture),
                             BOBGUI_EVENT_SEQUENCE_DENIED);
    }
}

static void
gesture_drag_update_cb (BobguiGestureDrag   *gesture,
                        double            offset_x,
                        double            offset_y,
                        BobguiPaned         *paned)
{
  double start_x, start_y;

  bobgui_gesture_drag_get_start_point (BOBGUI_GESTURE_DRAG (gesture),
                               &start_x, &start_y);
  update_drag (paned, start_x + offset_x, start_y + offset_y);
}

static void
gesture_drag_end_cb (BobguiGestureDrag *gesture,
                     double          offset_x,
                     double          offset_y,
                     BobguiPaned       *paned)
{
  if (!paned->panning)
    bobgui_gesture_set_state (BOBGUI_GESTURE (gesture), BOBGUI_EVENT_SEQUENCE_DENIED);

  paned->panning = FALSE;
}

static void
bobgui_paned_set_property (GObject        *object,
                        guint           prop_id,
                        const GValue   *value,
                        GParamSpec     *pspec)
{
  BobguiPaned *paned = BOBGUI_PANED (object);

  switch (prop_id)
    {
    case PROP_ORIENTATION:
      bobgui_paned_set_orientation (paned, g_value_get_enum (value));
      break;
    case PROP_POSITION:
      bobgui_paned_set_position (paned, g_value_get_int (value));
      break;
    case PROP_POSITION_SET:
      if (paned->position_set != g_value_get_boolean (value))
        {
          paned->position_set = g_value_get_boolean (value);
          bobgui_widget_queue_resize (BOBGUI_WIDGET (paned));
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    case PROP_WIDE_HANDLE:
      bobgui_paned_set_wide_handle (paned, g_value_get_boolean (value));
      break;
    case PROP_RESIZE_START_CHILD:
      bobgui_paned_set_resize_start_child (paned, g_value_get_boolean (value));
      break;
    case PROP_RESIZE_END_CHILD:
      bobgui_paned_set_resize_end_child (paned, g_value_get_boolean (value));
      break;
    case PROP_SHRINK_START_CHILD:
      bobgui_paned_set_shrink_start_child (paned, g_value_get_boolean (value));
      break;
    case PROP_SHRINK_END_CHILD:
      bobgui_paned_set_shrink_end_child (paned, g_value_get_boolean (value));
      break;
    case PROP_START_CHILD:
      bobgui_paned_set_start_child (paned, g_value_get_object (value));
      break;
    case PROP_END_CHILD:
      bobgui_paned_set_end_child (paned, g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_paned_get_property (GObject        *object,
                        guint           prop_id,
                        GValue         *value,
                        GParamSpec     *pspec)
{
  BobguiPaned *paned = BOBGUI_PANED (object);

  switch (prop_id)
    {
    case PROP_ORIENTATION:
      g_value_set_enum (value, paned->orientation);
      break;
    case PROP_POSITION:
      g_value_set_int (value, paned->start_child_size);
      break;
    case PROP_POSITION_SET:
      g_value_set_boolean (value, paned->position_set);
      break;
    case PROP_MIN_POSITION:
      g_value_set_int (value, paned->min_position);
      break;
    case PROP_MAX_POSITION:
      g_value_set_int (value, paned->max_position);
      break;
    case PROP_WIDE_HANDLE:
      g_value_set_boolean (value, bobgui_paned_get_wide_handle (paned));
      break;
    case PROP_RESIZE_START_CHILD:
      g_value_set_boolean (value, paned->resize_start_child);
      break;
    case PROP_RESIZE_END_CHILD:
      g_value_set_boolean (value, paned->resize_end_child);
      break;
    case PROP_SHRINK_START_CHILD:
      g_value_set_boolean (value, paned->shrink_start_child);
      break;
    case PROP_SHRINK_END_CHILD:
      g_value_set_boolean (value, paned->shrink_end_child);
      break;
    case PROP_START_CHILD:
      g_value_set_object (value, bobgui_paned_get_start_child (paned));
      break;
    case PROP_END_CHILD:
      g_value_set_object (value, bobgui_paned_get_end_child (paned));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_paned_dispose (GObject *object)
{
  BobguiPaned *paned = BOBGUI_PANED (object);

  bobgui_paned_set_saved_focus (paned, NULL);
  bobgui_paned_set_first_paned (paned, NULL);

  g_clear_pointer (&paned->start_child, bobgui_widget_unparent);
  g_clear_pointer (&paned->end_child, bobgui_widget_unparent);
  g_clear_pointer (&paned->handle_widget, bobgui_widget_unparent);

  G_OBJECT_CLASS (bobgui_paned_parent_class)->dispose (object);
}

static void
bobgui_paned_compute_position (BobguiPaned *paned,
                            int       allocation,
                            int       start_child_req,
                            int       end_child_req,
                            int      *min_pos,
                            int      *max_pos,
                            int      *out_pos)
{
  int min, max, pos;

  min = paned->shrink_start_child ? 0 : start_child_req;

  max = allocation;
  if (!paned->shrink_end_child)
    max = MAX (1, max - end_child_req);
  max = MAX (min, max);

  if (!paned->position_set)
    {
      if (paned->resize_start_child && !paned->resize_end_child)
        pos = MAX (0, allocation - end_child_req);
      else if (!paned->resize_start_child && paned->resize_end_child)
        pos = start_child_req;
      else if (start_child_req + end_child_req != 0)
        pos = allocation * ((double)start_child_req / (start_child_req + end_child_req)) + 0.5;
      else
        pos = allocation * 0.5 + 0.5;
    }
  else
    {
      /* If the position was set before the initial allocation.
       * (paned->last_allocation <= 0) just clamp it and leave it.
       */
      if (paned->last_allocation > 0)
        {
          if (paned->resize_start_child && !paned->resize_end_child)
            pos = paned->start_child_size + allocation - paned->last_allocation;
          else if (!(!paned->resize_start_child && paned->resize_end_child))
            pos = allocation * ((double) paned->start_child_size / (paned->last_allocation)) + 0.5;
          else
            pos = paned->start_child_size;
        }
      else
        pos = paned->start_child_size;
    }

  pos = CLAMP (pos, min, max);

  if (min_pos)
    *min_pos = min;
  if (max_pos)
    *max_pos = max;
  if (out_pos)
    *out_pos = pos;
}

static void
bobgui_paned_get_preferred_size_for_orientation (BobguiWidget      *widget,
                                              int             size,
                                              int            *minimum,
                                              int            *natural)
{
  BobguiPaned *paned = BOBGUI_PANED (widget);
  int child_min, child_nat;

  *minimum = *natural = 0;

  if (paned->start_child && bobgui_widget_get_visible (paned->start_child))
    {
      bobgui_widget_measure (paned->start_child, paned->orientation, size, &child_min, &child_nat, NULL, NULL);
      if (paned->shrink_start_child)
        *minimum = 0;
      else
        *minimum = child_min;
      *natural = child_nat;
    }

  if (paned->end_child && bobgui_widget_get_visible (paned->end_child))
    {
      bobgui_widget_measure (paned->end_child, paned->orientation, size, &child_min, &child_nat, NULL, NULL);

      if (!paned->shrink_end_child)
        *minimum += child_min;
      *natural += child_nat;
    }

  if (paned->start_child && bobgui_widget_get_visible (paned->start_child) &&
      paned->end_child && bobgui_widget_get_visible (paned->end_child))
    {
      int handle_size;

      bobgui_widget_measure (paned->handle_widget,
                          paned->orientation,
                          -1,
                          NULL, &handle_size,
                          NULL, NULL);

      *minimum += handle_size;
      *natural += handle_size;
    }
}

static void
bobgui_paned_get_preferred_size_for_opposite_orientation (BobguiWidget      *widget,
                                                       int             size,
                                                       int            *minimum,
                                                       int            *natural)
{
  BobguiPaned *paned = BOBGUI_PANED (widget);
  int for_start_child, for_end_child, for_handle;
  int child_min, child_nat;

  if (size > -1 &&
      paned->start_child && bobgui_widget_get_visible (paned->start_child) &&
      paned->end_child && bobgui_widget_get_visible (paned->end_child))
    {
      int start_child_req, end_child_req;

      bobgui_widget_measure (paned->handle_widget,
                          paned->orientation,
                          -1,
                          NULL, &for_handle,
                          NULL, NULL);

      bobgui_widget_measure (paned->start_child, paned->orientation, -1, &start_child_req, NULL, NULL, NULL);
      bobgui_widget_measure (paned->end_child, paned->orientation, -1, &end_child_req, NULL, NULL, NULL);

      bobgui_paned_compute_position (paned,
                                  size - for_handle, start_child_req, end_child_req,
                                  NULL, NULL, &for_start_child);

      for_end_child = size - for_start_child - for_handle;

      if (paned->shrink_start_child)
        for_start_child = MAX (start_child_req, for_start_child);
      if (paned->shrink_end_child)
        for_end_child = MAX (end_child_req, for_end_child);
    }
  else
    {
      for_start_child = size;
      for_end_child = size;
      for_handle = -1;
    }

  *minimum = *natural = 0;

  if (paned->start_child && bobgui_widget_get_visible (paned->start_child))
    {
      bobgui_widget_measure (paned->start_child,
                          OPPOSITE_ORIENTATION (paned->orientation),
                          for_start_child,
                          &child_min, &child_nat,
                          NULL, NULL);

      *minimum = child_min;
      *natural = child_nat;
    }

  if (paned->end_child && bobgui_widget_get_visible (paned->end_child))
    {
      bobgui_widget_measure (paned->end_child,
                          OPPOSITE_ORIENTATION (paned->orientation),
                          for_end_child,
                          &child_min, &child_nat,
                          NULL, NULL);

      *minimum = MAX (*minimum, child_min);
      *natural = MAX (*natural, child_nat);
    }

  if (paned->start_child && bobgui_widget_get_visible (paned->start_child) &&
      paned->end_child && bobgui_widget_get_visible (paned->end_child))
    {
      bobgui_widget_measure (paned->handle_widget,
                          OPPOSITE_ORIENTATION (paned->orientation),
                          for_handle,
                          &child_min, &child_nat,
                          NULL, NULL);

      *minimum = MAX (*minimum, child_min);
      *natural = MAX (*natural, child_nat);
    }
}

static void
bobgui_paned_measure (BobguiWidget *widget,
                   BobguiOrientation  orientation,
                   int             for_size,
                   int            *minimum,
                   int            *natural,
                   int            *minimum_baseline,
                   int            *natural_baseline)
{
  BobguiPaned *paned = BOBGUI_PANED (widget);

  if (orientation == paned->orientation)
    bobgui_paned_get_preferred_size_for_orientation (widget, for_size, minimum, natural);
  else
    bobgui_paned_get_preferred_size_for_opposite_orientation (widget, for_size, minimum, natural);
}

static void
flip_child (int            width,
            BobguiAllocation *child_pos)
{
  child_pos->x = width - child_pos->x - child_pos->width;
}

static void
bobgui_paned_size_allocate (BobguiWidget *widget,
                         int        width,
                         int        height,
                         int        baseline)
{
  BobguiPaned *paned = BOBGUI_PANED (widget);

  if (paned->start_child && bobgui_widget_get_visible (paned->start_child) &&
      paned->end_child && bobgui_widget_get_visible (paned->end_child))
    {
      BobguiAllocation start_child_allocation;
      BobguiAllocation end_child_allocation;
      BobguiAllocation handle_allocation;
      int handle_size;

      bobgui_widget_measure (paned->handle_widget,
                          paned->orientation,
                          -1,
                          NULL, &handle_size,
                          NULL, NULL);

      if (paned->orientation == BOBGUI_ORIENTATION_HORIZONTAL)
        {
          int start_child_width, end_child_width;

          bobgui_widget_measure (paned->start_child, BOBGUI_ORIENTATION_HORIZONTAL,
                              height,
                              &start_child_width, NULL, NULL, NULL);
          bobgui_widget_measure (paned->end_child, BOBGUI_ORIENTATION_HORIZONTAL,
                              height,
                              &end_child_width, NULL, NULL, NULL);

          bobgui_paned_calc_position (paned,
                                   MAX (1, width - handle_size),
                                   start_child_width,
                                   end_child_width);

          handle_allocation = (GdkRectangle){
            paned->start_child_size,
            0,
            handle_size,
            height
          };

          start_child_allocation.height = end_child_allocation.height = height;
          start_child_allocation.width = MAX (1, paned->start_child_size);
          start_child_allocation.x = 0;
          start_child_allocation.y = end_child_allocation.y = 0;

          end_child_allocation.x = start_child_allocation.x + paned->start_child_size + handle_size;
          end_child_allocation.width = MAX (1, width - paned->start_child_size - handle_size);

          if (bobgui_widget_get_direction (BOBGUI_WIDGET (widget)) == BOBGUI_TEXT_DIR_RTL)
            {
              flip_child (width, &(end_child_allocation));
              flip_child (width, &(start_child_allocation));
              flip_child (width, &(handle_allocation));
            }

          if (start_child_width > start_child_allocation.width)
            {
              if (bobgui_widget_get_direction (BOBGUI_WIDGET (widget)) == BOBGUI_TEXT_DIR_LTR)
                start_child_allocation.x -= start_child_width - start_child_allocation.width;
              start_child_allocation.width = start_child_width;
            }

          if (end_child_width > end_child_allocation.width)
            {
              if (bobgui_widget_get_direction (BOBGUI_WIDGET (widget)) == BOBGUI_TEXT_DIR_RTL)
                end_child_allocation.x -= end_child_width - end_child_allocation.width;
              end_child_allocation.width = end_child_width;
            }
        }
      else
        {
          int start_child_height, end_child_height;

          bobgui_widget_measure (paned->start_child, BOBGUI_ORIENTATION_VERTICAL,
                              width,
                              &start_child_height, NULL, NULL, NULL);
          bobgui_widget_measure (paned->end_child, BOBGUI_ORIENTATION_VERTICAL,
                              width,
                              &end_child_height, NULL, NULL, NULL);

          bobgui_paned_calc_position (paned,
                                   MAX (1, height - handle_size),
                                   start_child_height,
                                   end_child_height);

          handle_allocation = (GdkRectangle){
            0,
            paned->start_child_size,
            width,
            handle_size,
          };

          start_child_allocation.width = end_child_allocation.width = width;
          start_child_allocation.height = MAX (1, paned->start_child_size);
          start_child_allocation.x = end_child_allocation.x = 0;
          start_child_allocation.y = 0;

          end_child_allocation.y = start_child_allocation.y + paned->start_child_size + handle_size;
          end_child_allocation.height = MAX (1, height - end_child_allocation.y);

          if (start_child_height > start_child_allocation.height)
            {
              start_child_allocation.y -= start_child_height - start_child_allocation.height;
              start_child_allocation.height = start_child_height;
            }

          if (end_child_height > end_child_allocation.height)
            end_child_allocation.height = end_child_height;
        }

      bobgui_widget_set_child_visible (paned->handle_widget, TRUE);

      bobgui_widget_size_allocate (paned->handle_widget, &handle_allocation, -1);
      bobgui_widget_size_allocate (paned->start_child, &start_child_allocation, -1);
      bobgui_widget_size_allocate (paned->end_child, &end_child_allocation, -1);
    }
  else
    {
      if (paned->start_child && bobgui_widget_get_visible (paned->start_child))
        {
          bobgui_widget_set_child_visible (paned->start_child, TRUE);

          bobgui_widget_size_allocate (paned->start_child,
                                    &(BobguiAllocation) {0, 0, width, height}, -1);
        }
      else if (paned->end_child && bobgui_widget_get_visible (paned->end_child))
        {
          bobgui_widget_set_child_visible (paned->end_child, TRUE);

          bobgui_widget_size_allocate (paned->end_child,
                                    &(BobguiAllocation) {0, 0, width, height}, -1);

        }

      bobgui_widget_set_child_visible (paned->handle_widget, FALSE);
    }

  bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (paned),
                                  BOBGUI_ACCESSIBLE_PROPERTY_VALUE_MIN, 0.0,
                                  BOBGUI_ACCESSIBLE_PROPERTY_VALUE_MAX,
                                      (double) (paned->orientation == BOBGUI_ORIENTATION_HORIZONTAL ?  width : height),
                                  BOBGUI_ACCESSIBLE_PROPERTY_VALUE_NOW,
                                      (double) paned->start_child_size,
                                  -1);
}


static void
bobgui_paned_unrealize (BobguiWidget *widget)
{
  BobguiPaned *paned = BOBGUI_PANED (widget);

  bobgui_paned_set_last_start_child_focus (paned, NULL);
  bobgui_paned_set_last_end_child_focus (paned, NULL);
  bobgui_paned_set_saved_focus (paned, NULL);
  bobgui_paned_set_first_paned (paned, NULL);

  BOBGUI_WIDGET_CLASS (bobgui_paned_parent_class)->unrealize (widget);
}

static void
connect_drag_gesture_signals (BobguiPaned   *paned,
                              BobguiGesture *gesture)
{
  g_signal_connect (gesture, "drag-begin",
                    G_CALLBACK (gesture_drag_begin_cb), paned);
  g_signal_connect (gesture, "drag-update",
                    G_CALLBACK (gesture_drag_update_cb), paned);
  g_signal_connect (gesture, "drag-end",
                    G_CALLBACK (gesture_drag_end_cb), paned);
}

static void
bobgui_paned_init (BobguiPaned *paned)
{
  BobguiGesture *gesture;

  bobgui_widget_set_focusable (BOBGUI_WIDGET (paned), TRUE);
  bobgui_widget_set_overflow (BOBGUI_WIDGET (paned), BOBGUI_OVERFLOW_HIDDEN);

  paned->orientation = BOBGUI_ORIENTATION_HORIZONTAL;

  paned->start_child = NULL;
  paned->end_child = NULL;

  paned->position_set = FALSE;
  paned->last_allocation = -1;

  paned->last_start_child_focus = NULL;
  paned->last_end_child_focus = NULL;
  paned->in_recursion = FALSE;
  paned->original_position = -1;
  paned->max_position = G_MAXINT;
  paned->resize_start_child = TRUE;
  paned->resize_end_child = TRUE;
  paned->shrink_start_child = TRUE;
  paned->shrink_end_child = TRUE;

  bobgui_widget_update_orientation (BOBGUI_WIDGET (paned), paned->orientation);

  /* Touch gesture */
  gesture = bobgui_gesture_pan_new (BOBGUI_ORIENTATION_HORIZONTAL);
  connect_drag_gesture_signals (paned, gesture);
  bobgui_gesture_single_set_touch_only (BOBGUI_GESTURE_SINGLE (gesture), TRUE);
  bobgui_event_controller_set_propagation_phase (BOBGUI_EVENT_CONTROLLER (gesture),
                                              BOBGUI_PHASE_CAPTURE);
  bobgui_widget_add_controller (BOBGUI_WIDGET (paned), BOBGUI_EVENT_CONTROLLER (gesture));
  paned->pan_gesture = gesture;

  /* Pointer gesture */
  gesture = bobgui_gesture_drag_new ();
  bobgui_event_controller_set_propagation_phase (BOBGUI_EVENT_CONTROLLER (gesture),
                                              BOBGUI_PHASE_CAPTURE);
  connect_drag_gesture_signals (paned, gesture);
  bobgui_widget_add_controller (BOBGUI_WIDGET (paned), BOBGUI_EVENT_CONTROLLER (gesture));
  paned->drag_gesture = gesture;

  paned->handle_widget = bobgui_paned_handle_new ();
  bobgui_widget_set_parent (paned->handle_widget, BOBGUI_WIDGET (paned));
  bobgui_widget_set_cursor_from_name (paned->handle_widget, "col-resize");
}

static gboolean
is_rtl (BobguiPaned *paned)
{
  return paned->orientation == BOBGUI_ORIENTATION_HORIZONTAL &&
         bobgui_widget_get_direction (BOBGUI_WIDGET (paned)) == BOBGUI_TEXT_DIR_RTL;
}

static void
update_drag (BobguiPaned *paned,
             int       xpos,
             int       ypos)
{
  int pos;
  int handle_size;
  int size;

  if (paned->orientation == BOBGUI_ORIENTATION_HORIZONTAL)
    pos = xpos;
  else
    pos = ypos;

  pos -= paned->drag_pos;

  if (is_rtl (paned))
    {
      bobgui_widget_measure (paned->handle_widget,
                          BOBGUI_ORIENTATION_HORIZONTAL,
                          -1,
                          NULL, &handle_size,
                          NULL, NULL);

      size = bobgui_widget_get_width (BOBGUI_WIDGET (paned)) - pos - handle_size;
    }
  else
    {
      size = pos;
    }

  size = CLAMP (size, paned->min_position, paned->max_position);

  if (size != paned->start_child_size)
    bobgui_paned_set_position (paned, size);
}

static void
bobgui_paned_css_changed (BobguiWidget         *widget,
                       BobguiCssStyleChange *change)
{
  BOBGUI_WIDGET_CLASS (bobgui_paned_parent_class)->css_changed (widget, change);

  if (change == NULL ||
      bobgui_css_style_change_affects (change, BOBGUI_CSS_AFFECTS_ICON_SIZE))
    {
      bobgui_widget_queue_resize (widget);
    }
  else if (bobgui_css_style_change_affects (change, BOBGUI_CSS_AFFECTS_ICON_TEXTURE |
                                                 BOBGUI_CSS_AFFECTS_ICON_REDRAW))
    {
      bobgui_widget_queue_draw (widget);
    }
}

/**
 * bobgui_paned_new:
 * @orientation: the paned’s orientation.
 *
 * Creates a new `BobguiPaned` widget.
 *
 * Returns: the newly created paned widget
 */
BobguiWidget *
bobgui_paned_new (BobguiOrientation orientation)
{
  return g_object_new (BOBGUI_TYPE_PANED,
                       "orientation", orientation,
                       NULL);
}

/**
 * bobgui_paned_set_start_child:
 * @paned: a `BobguiPaned`
 * @child: (nullable): the widget to add
 *
 * Sets the start child of @paned to @child.
 *
 * If @child is `NULL`, the existing child will be removed.
 */
void
bobgui_paned_set_start_child (BobguiPaned  *paned,
                           BobguiWidget *child)
{
  g_return_if_fail (BOBGUI_IS_PANED (paned));
  g_return_if_fail (child == NULL || paned->start_child == child || bobgui_widget_get_parent (child) == NULL);

  if (paned->start_child == child)
    return;

  g_clear_pointer (&paned->start_child, bobgui_widget_unparent);

  if (child)
    {
      paned->start_child = child;
      bobgui_widget_insert_before (child, BOBGUI_WIDGET (paned), paned->handle_widget);
    }

  g_object_notify (G_OBJECT (paned), "start-child");
}

/**
 * bobgui_paned_get_start_child:
 * @paned: a `BobguiPaned`
 *
 * Retrieves the start child of the given `BobguiPaned`.
 *
 * Returns: (transfer none) (nullable): the start child widget
 */
BobguiWidget *
bobgui_paned_get_start_child (BobguiPaned *paned)
{
  g_return_val_if_fail (BOBGUI_IS_PANED (paned), NULL);

  return paned->start_child;
}

/**
 * bobgui_paned_set_resize_start_child:
 * @paned: a `BobguiPaned`
 * @resize: true to let the start child be resized
 *
 * Sets whether the [property@Bobgui.Paned:start-child] can be resized.
 */
void
bobgui_paned_set_resize_start_child (BobguiPaned *paned,
                                  gboolean  resize)
{
  g_return_if_fail (BOBGUI_IS_PANED (paned));

  if (paned->resize_start_child == resize)
    return;

  paned->resize_start_child = resize;

  g_object_notify (G_OBJECT (paned), "resize-start-child");
}

/**
 * bobgui_paned_get_resize_start_child:
 * @paned: a `BobguiPaned`
 *
 * Returns whether the [property@Bobgui.Paned:start-child] can be resized.
 *
 * Returns: true if the start child is resizable
 */
gboolean
bobgui_paned_get_resize_start_child (BobguiPaned *paned)
{
  g_return_val_if_fail (BOBGUI_IS_PANED (paned), FALSE);

  return paned->resize_start_child;
}

/**
 * bobgui_paned_set_shrink_start_child:
 * @paned: a `BobguiPaned`
 * @resize: true to let the start child be shrunk
 *
 * Sets whether the [property@Bobgui.Paned:start-child] can shrink.
 */
void
bobgui_paned_set_shrink_start_child (BobguiPaned *paned,
                                  gboolean  shrink)
{
  g_return_if_fail (BOBGUI_IS_PANED (paned));

  if (paned->shrink_start_child == shrink)
    return;

  paned->shrink_start_child = shrink;

  g_object_notify (G_OBJECT (paned), "shrink-start-child");
}

/**
 * bobgui_paned_get_shrink_start_child:
 * @paned: a `BobguiPaned`
 *
 * Returns whether the [property@Bobgui.Paned:start-child] can shrink.
 *
 * Returns: true if the start child is shrinkable
 */
gboolean
bobgui_paned_get_shrink_start_child (BobguiPaned *paned)
{
  g_return_val_if_fail (BOBGUI_IS_PANED (paned), FALSE);

  return paned->shrink_start_child;
}

/**
 * bobgui_paned_set_end_child:
 * @paned: a `BobguiPaned`
 * @child: (nullable): the widget to add
 *
 * Sets the end child of @paned to @child.
 *
 * If @child is `NULL`, the existing child will be removed.
 */
void
bobgui_paned_set_end_child (BobguiPaned  *paned,
                         BobguiWidget *child)
{
  g_return_if_fail (BOBGUI_IS_PANED (paned));
  g_return_if_fail (child == NULL || paned->end_child == child || bobgui_widget_get_parent (child) == NULL);

  if (paned->end_child == child)
    return;

  g_clear_pointer (&paned->end_child, bobgui_widget_unparent);

  if (child)
    {
      paned->end_child = child;
      bobgui_widget_insert_after (child, BOBGUI_WIDGET (paned), paned->handle_widget);
    }

  g_object_notify (G_OBJECT (paned), "end-child");
}

/**
 * bobgui_paned_get_end_child:
 * @paned: a `BobguiPaned`
 *
 * Retrieves the end child of the given `BobguiPaned`.
 *
 * Returns: (transfer none) (nullable): the end child widget
 */
BobguiWidget *
bobgui_paned_get_end_child (BobguiPaned *paned)
{
  g_return_val_if_fail (BOBGUI_IS_PANED (paned), NULL);

  return paned->end_child;
}

/**
 * bobgui_paned_set_resize_end_child:
 * @paned: a `BobguiPaned`
 * @resize: true to let the end child be resized
 *
 * Sets whether the [property@Bobgui.Paned:end-child] can be resized.
 */
void
bobgui_paned_set_resize_end_child (BobguiPaned *paned,
                                gboolean  resize)
{
  g_return_if_fail (BOBGUI_IS_PANED (paned));

  if (paned->resize_end_child == resize)
    return;

  paned->resize_end_child = resize;

  g_object_notify (G_OBJECT (paned), "resize-end-child");
}

/**
 * bobgui_paned_get_resize_end_child:
 * @paned: a `BobguiPaned`
 *
 * Returns whether the [property@Bobgui.Paned:end-child] can be resized.
 *
 * Returns: true if the end child is resizable
 */
gboolean
bobgui_paned_get_resize_end_child (BobguiPaned *paned)
{
  g_return_val_if_fail (BOBGUI_IS_PANED (paned), FALSE);

  return paned->resize_end_child;
}

/**
 * bobgui_paned_set_shrink_end_child:
 * @paned: a `BobguiPaned`
 * @resize: true to let the end child be shrunk
 *
 * Sets whether the [property@Bobgui.Paned:end-child] can shrink.
 */
void
bobgui_paned_set_shrink_end_child (BobguiPaned *paned,
                                gboolean  shrink)
{
  g_return_if_fail (BOBGUI_IS_PANED (paned));

  if (paned->shrink_end_child == shrink)
    return;

  paned->shrink_end_child = shrink;

  g_object_notify (G_OBJECT (paned), "shrink-end-child");
}

/**
 * bobgui_paned_get_shrink_end_child:
 * @paned: a `BobguiPaned`
 *
 * Returns whether the [property@Bobgui.Paned:end-child] can shrink.
 *
 * Returns: true if the end child is shrinkable
 */
gboolean
bobgui_paned_get_shrink_end_child (BobguiPaned *paned)
{
  g_return_val_if_fail (BOBGUI_IS_PANED (paned), FALSE);

  return paned->shrink_end_child;
}

/**
 * bobgui_paned_get_position:
 * @paned: a `BobguiPaned` widget
 *
 * Obtains the position of the divider between the two panes.
 *
 * Returns: the position of the divider, in pixels
 **/
int
bobgui_paned_get_position (BobguiPaned  *paned)
{
  g_return_val_if_fail (BOBGUI_IS_PANED (paned), 0);

  return paned->start_child_size;
}

/**
 * bobgui_paned_set_position:
 * @paned: a `BobguiPaned` widget
 * @position: pixel position of divider, a negative value means that the position
 *   is unset
 *
 * Sets the position of the divider between the two panes.
 */
void
bobgui_paned_set_position (BobguiPaned *paned,
                        int       position)
{
  g_return_if_fail (BOBGUI_IS_PANED (paned));

  g_object_freeze_notify (G_OBJECT (paned));

  if (position >= 0)
    {
      /* We don't clamp here - the assumption is that
       * if the total allocation changes at the same time
       * as the position, the position set is with reference
       * to the new total size. If only the position changes,
       * then clamping will occur in bobgui_paned_calc_position()
       */

      if (!paned->position_set)
        g_object_notify_by_pspec (G_OBJECT (paned), paned_props[PROP_POSITION_SET]);

      if (paned->start_child_size != position)
        {
          g_object_notify_by_pspec (G_OBJECT (paned), paned_props[PROP_POSITION]);
          bobgui_widget_queue_allocate (BOBGUI_WIDGET (paned));
        }

      paned->start_child_size = position;
      paned->position_set = TRUE;
    }
  else
    {
      if (paned->position_set)
        g_object_notify_by_pspec (G_OBJECT (paned), paned_props[PROP_POSITION_SET]);

      paned->position_set = FALSE;
    }

  g_object_thaw_notify (G_OBJECT (paned));

#ifdef G_OS_WIN32
  /* Hacky work-around for bug #144269 */
  if (paned->end_child != NULL)
    {
      bobgui_widget_queue_draw (paned->end_child);
    }
#endif
}

static void
bobgui_paned_calc_position (BobguiPaned *paned,
                         int       allocation,
                         int       start_child_req,
                         int       end_child_req)
{
  int old_position;
  int old_min_position;
  int old_max_position;

  old_position = paned->start_child_size;
  old_min_position = paned->min_position;
  old_max_position = paned->max_position;

  bobgui_paned_compute_position (paned,
                              allocation, start_child_req, end_child_req,
                              &paned->min_position, &paned->max_position,
                              &paned->start_child_size);

  bobgui_widget_set_child_visible (paned->start_child, paned->start_child_size != 0);
  bobgui_widget_set_child_visible (paned->end_child, paned->start_child_size != allocation);

  g_object_freeze_notify (G_OBJECT (paned));
  if (paned->start_child_size != old_position)
    g_object_notify_by_pspec (G_OBJECT (paned), paned_props[PROP_POSITION]);
  if (paned->min_position != old_min_position)
    g_object_notify_by_pspec (G_OBJECT (paned), paned_props[PROP_MIN_POSITION]);
  if (paned->max_position != old_max_position)
    g_object_notify_by_pspec (G_OBJECT (paned), paned_props[PROP_MAX_POSITION]);
  g_object_thaw_notify (G_OBJECT (paned));

  paned->last_allocation = allocation;
}

static void
bobgui_paned_set_saved_focus (BobguiPaned *paned, BobguiWidget *widget)
{
  if (paned->saved_focus)
    g_object_remove_weak_pointer (G_OBJECT (paned->saved_focus),
                                  (gpointer *)&(paned->saved_focus));

  paned->saved_focus = widget;

  if (paned->saved_focus)
    g_object_add_weak_pointer (G_OBJECT (paned->saved_focus),
                               (gpointer *)&(paned->saved_focus));
}

static void
bobgui_paned_set_first_paned (BobguiPaned *paned, BobguiPaned *first_paned)
{
  if (paned->first_paned)
    g_object_remove_weak_pointer (G_OBJECT (paned->first_paned),
                                  (gpointer *)&(paned->first_paned));

  paned->first_paned = first_paned;

  if (paned->first_paned)
    g_object_add_weak_pointer (G_OBJECT (paned->first_paned),
                               (gpointer *)&(paned->first_paned));
}

static void
bobgui_paned_set_last_start_child_focus (BobguiPaned *paned, BobguiWidget *widget)
{
  if (paned->last_start_child_focus)
    g_object_remove_weak_pointer (G_OBJECT (paned->last_start_child_focus),
                                  (gpointer *)&(paned->last_start_child_focus));

  paned->last_start_child_focus = widget;

  if (paned->last_start_child_focus)
    g_object_add_weak_pointer (G_OBJECT (paned->last_start_child_focus),
                               (gpointer *)&(paned->last_start_child_focus));
}

static void
bobgui_paned_set_last_end_child_focus (BobguiPaned *paned, BobguiWidget *widget)
{
  if (paned->last_end_child_focus)
    g_object_remove_weak_pointer (G_OBJECT (paned->last_end_child_focus),
                                  (gpointer *)&(paned->last_end_child_focus));

  paned->last_end_child_focus = widget;

  if (paned->last_end_child_focus)
    g_object_add_weak_pointer (G_OBJECT (paned->last_end_child_focus),
                               (gpointer *)&(paned->last_end_child_focus));
}

static BobguiWidget *
paned_get_focus_widget (BobguiPaned *paned)
{
  BobguiWidget *toplevel;

  toplevel = BOBGUI_WIDGET (bobgui_widget_get_root (BOBGUI_WIDGET (paned)));
  if (BOBGUI_IS_WINDOW (toplevel))
    return bobgui_window_get_focus (BOBGUI_WINDOW (toplevel));

  return NULL;
}

static void
bobgui_paned_set_focus_child (BobguiWidget *widget,
                           BobguiWidget *child)
{
  BobguiPaned *paned = BOBGUI_PANED (widget);
  BobguiWidget *focus_child;

  if (child == NULL)
    {
      BobguiWidget *last_focus;
      BobguiWidget *w;

      last_focus = paned_get_focus_widget (paned);

      if (last_focus)
        {
          /* If there is one or more paned widgets between us and the
           * focus widget, we want the topmost of those as last_focus
           */
          for (w = last_focus; w && w != BOBGUI_WIDGET (paned); w = bobgui_widget_get_parent (w))
            if (BOBGUI_IS_PANED (w))
              last_focus = w;

          if (w == NULL)
            {
              g_warning ("Error finding last focus widget of BobguiPaned %p, "
                         "bobgui_paned_set_focus_child was called on widget %p "
                         "which is not child of %p.",
                         widget, child, widget);
              return;
            }

          focus_child = bobgui_widget_get_focus_child (widget);
          if (focus_child == paned->start_child)
            bobgui_paned_set_last_start_child_focus (paned, last_focus);
          else if (focus_child == paned->end_child)
            bobgui_paned_set_last_end_child_focus (paned, last_focus);
        }
    }

  BOBGUI_WIDGET_CLASS (bobgui_paned_parent_class)->set_focus_child (widget, child);
}

static void
bobgui_paned_get_cycle_chain (BobguiPaned          *paned,
                           BobguiDirectionType   direction,
                           GList            **widgets)
{
  BobguiWidget *ancestor = NULL;
  BobguiWidget *focus_child;
  BobguiWidget *parent;
  BobguiWidget *widget = BOBGUI_WIDGET (paned);
  GList *temp_list = NULL;
  GList *list;

  if (paned->in_recursion)
    return;

  g_assert (widgets != NULL);

  if (paned->last_start_child_focus &&
      !bobgui_widget_is_ancestor (paned->last_start_child_focus, widget))
    {
      bobgui_paned_set_last_start_child_focus (paned, NULL);
    }

  if (paned->last_end_child_focus &&
      !bobgui_widget_is_ancestor (paned->last_end_child_focus, widget))
    {
      bobgui_paned_set_last_end_child_focus (paned, NULL);
    }

  parent = bobgui_widget_get_parent (widget);
  if (parent)
    ancestor = bobgui_widget_get_ancestor (parent, BOBGUI_TYPE_PANED);

  /* The idea here is that temp_list is a list of widgets we want to cycle
   * to. The list is prioritized so that the first element is our first
   * choice, the next our second, and so on.
   *
   * We can't just use g_list_reverse(), because we want to try
   * paned->last_child?_focus before paned->child?, both when we
   * are going forward and backward.
   */
  focus_child = bobgui_widget_get_focus_child (BOBGUI_WIDGET (paned));
  if (direction == BOBGUI_DIR_TAB_FORWARD)
    {
      if (focus_child == paned->start_child)
        {
          temp_list = g_list_append (temp_list, paned->last_end_child_focus);
          temp_list = g_list_append (temp_list, paned->end_child);
          temp_list = g_list_append (temp_list, ancestor);
        }
      else if (focus_child == paned->end_child)
        {
          temp_list = g_list_append (temp_list, ancestor);
          temp_list = g_list_append (temp_list, paned->last_start_child_focus);
          temp_list = g_list_append (temp_list, paned->start_child);
        }
      else
        {
          temp_list = g_list_append (temp_list, paned->last_start_child_focus);
          temp_list = g_list_append (temp_list, paned->start_child);
          temp_list = g_list_append (temp_list, paned->last_end_child_focus);
          temp_list = g_list_append (temp_list, paned->end_child);
          temp_list = g_list_append (temp_list, ancestor);
        }
    }
  else
    {
      if (focus_child == paned->start_child)
        {
          temp_list = g_list_append (temp_list, ancestor);
          temp_list = g_list_append (temp_list, paned->last_end_child_focus);
          temp_list = g_list_append (temp_list, paned->end_child);
        }
      else if (focus_child == paned->end_child)
        {
          temp_list = g_list_append (temp_list, paned->last_start_child_focus);
          temp_list = g_list_append (temp_list, paned->start_child);
          temp_list = g_list_append (temp_list, ancestor);
        }
      else
        {
          temp_list = g_list_append (temp_list, paned->last_end_child_focus);
          temp_list = g_list_append (temp_list, paned->end_child);
          temp_list = g_list_append (temp_list, paned->last_start_child_focus);
          temp_list = g_list_append (temp_list, paned->start_child);
          temp_list = g_list_append (temp_list, ancestor);
        }
    }

  /* Walk the list and expand all the paned widgets. */
  for (list = temp_list; list != NULL; list = list->next)
    {
      widget = list->data;

      if (widget)
        {
          if (BOBGUI_IS_PANED (widget))
            {
              paned->in_recursion = TRUE;
              bobgui_paned_get_cycle_chain (BOBGUI_PANED (widget), direction, widgets);
              paned->in_recursion = FALSE;
            }
          else
            {
              *widgets = g_list_append (*widgets, widget);
            }
        }
    }

  g_list_free (temp_list);
}

static gboolean
bobgui_paned_cycle_child_focus (BobguiPaned *paned,
                             gboolean  reversed)
{
  GList *cycle_chain = NULL;
  GList *list;

  BobguiDirectionType direction = reversed? BOBGUI_DIR_TAB_BACKWARD : BOBGUI_DIR_TAB_FORWARD;

  /* ignore f6 if the handle is focused */
  if (bobgui_widget_is_focus (BOBGUI_WIDGET (paned)))
    return TRUE;

  /* we can't just let the event propagate up the hierarchy,
   * because the paned will want to cycle focus _unless_ an
   * ancestor paned handles the event
   */
  bobgui_paned_get_cycle_chain (paned, direction, &cycle_chain);

  for (list = cycle_chain; list != NULL; list = list->next)
    if (bobgui_widget_child_focus (BOBGUI_WIDGET (list->data), direction))
      break;

  g_list_free (cycle_chain);

  return TRUE;
}

static void
get_child_panes (BobguiWidget  *widget,
                 GList     **panes)
{
  if (!widget || !bobgui_widget_get_realized (widget))
    return;

  if (BOBGUI_IS_PANED (widget))
    {
      BobguiPaned *paned = BOBGUI_PANED (widget);

      get_child_panes (paned->start_child, panes);
      *panes = g_list_prepend (*panes, widget);
      get_child_panes (paned->end_child, panes);
    }
  else
    {
      BobguiWidget *child;

      for (child = bobgui_widget_get_first_child (widget);
           child != NULL;
           child = bobgui_widget_get_next_sibling (child))
        get_child_panes (child, panes);
    }
}

static GList *
get_all_panes (BobguiPaned *paned)
{
  BobguiPaned *topmost = NULL;
  GList *result = NULL;
  BobguiWidget *w;

  for (w = BOBGUI_WIDGET (paned); w != NULL; w = bobgui_widget_get_parent (w))
    {
      if (BOBGUI_IS_PANED (w))
        topmost = BOBGUI_PANED (w);
    }

  g_assert (topmost);

  get_child_panes (BOBGUI_WIDGET (topmost), &result);

  return g_list_reverse (result);
}

static void
bobgui_paned_find_neighbours (BobguiPaned  *paned,
                           BobguiPaned **next,
                           BobguiPaned **prev)
{
  GList *all_panes;
  GList *this_link;

  all_panes = get_all_panes (paned);
  g_assert (all_panes);

  this_link = g_list_find (all_panes, paned);

  g_assert (this_link);

  if (this_link->next)
    *next = this_link->next->data;
  else
    *next = all_panes->data;

  if (this_link->prev)
    *prev = this_link->prev->data;
  else
    *prev = g_list_last (all_panes)->data;

  g_list_free (all_panes);
}

static gboolean
bobgui_paned_move_handle (BobguiPaned      *paned,
                       BobguiScrollType  scroll)
{
  if (bobgui_widget_is_focus (BOBGUI_WIDGET (paned)))
    {
      int old_position;
      int new_position;
      int increment;

      enum {
        SINGLE_STEP_SIZE = 1,
        PAGE_STEP_SIZE   = 75
      };

      new_position = old_position = bobgui_paned_get_position (paned);
      increment = 0;

      switch (scroll)
        {
        case BOBGUI_SCROLL_STEP_LEFT:
        case BOBGUI_SCROLL_STEP_UP:
        case BOBGUI_SCROLL_STEP_BACKWARD:
          increment = - SINGLE_STEP_SIZE;
          break;

        case BOBGUI_SCROLL_STEP_RIGHT:
        case BOBGUI_SCROLL_STEP_DOWN:
        case BOBGUI_SCROLL_STEP_FORWARD:
          increment = SINGLE_STEP_SIZE;
          break;

        case BOBGUI_SCROLL_PAGE_LEFT:
        case BOBGUI_SCROLL_PAGE_UP:
        case BOBGUI_SCROLL_PAGE_BACKWARD:
          increment = - PAGE_STEP_SIZE;
          break;

        case BOBGUI_SCROLL_PAGE_RIGHT:
        case BOBGUI_SCROLL_PAGE_DOWN:
        case BOBGUI_SCROLL_PAGE_FORWARD:
          increment = PAGE_STEP_SIZE;
          break;

        case BOBGUI_SCROLL_START:
          new_position = paned->min_position;
          break;

        case BOBGUI_SCROLL_END:
          new_position = paned->max_position;
          break;

        case BOBGUI_SCROLL_NONE:
        case BOBGUI_SCROLL_JUMP:
        default:
          break;
        }

      if (increment)
        {
          if (is_rtl (paned))
            increment = -increment;

          new_position = old_position + increment;
        }

      new_position = CLAMP (new_position, paned->min_position, paned->max_position);

      if (old_position != new_position)
        bobgui_paned_set_position (paned, new_position);

      return TRUE;
    }

  return FALSE;
}

static void
bobgui_paned_restore_focus (BobguiPaned *paned)
{
  if (bobgui_widget_is_focus (BOBGUI_WIDGET (paned)))
    {
      if (paned->saved_focus &&
          bobgui_widget_get_sensitive (paned->saved_focus))
        {
          bobgui_widget_grab_focus (paned->saved_focus);
        }
      else
        {
          /* the saved focus is somehow not available for focusing,
           * try
           *   1) tabbing into the paned widget
           * if that didn't work,
           *   2) unset focus for the window if there is one
           */

          if (!bobgui_widget_child_focus (BOBGUI_WIDGET (paned), BOBGUI_DIR_TAB_FORWARD))
            {
              BobguiRoot *root = bobgui_widget_get_root (BOBGUI_WIDGET (paned));
              bobgui_root_set_focus (root, NULL);
            }
        }

      bobgui_paned_set_saved_focus (paned, NULL);
      bobgui_paned_set_first_paned (paned, NULL);
    }
}

static gboolean
bobgui_paned_accept_position (BobguiPaned *paned)
{
  if (bobgui_widget_is_focus (BOBGUI_WIDGET (paned)))
    {
      paned->original_position = -1;
      bobgui_paned_restore_focus (paned);

      return TRUE;
    }

  return FALSE;
}


static gboolean
bobgui_paned_cancel_position (BobguiPaned *paned)
{
  if (bobgui_widget_is_focus (BOBGUI_WIDGET (paned)))
    {
      if (paned->original_position != -1)
        {
          bobgui_paned_set_position (paned, paned->original_position);
          paned->original_position = -1;
        }

      bobgui_paned_restore_focus (paned);
      return TRUE;
    }

  return FALSE;
}

static gboolean
bobgui_paned_cycle_handle_focus (BobguiPaned *paned,
                              gboolean  reversed)
{
  BobguiPaned *next, *prev;

  if (bobgui_widget_is_focus (BOBGUI_WIDGET (paned)))
    {
      BobguiPaned *focus = NULL;

      if (!paned->first_paned)
        {
          /* The first_pane has disappeared. As an ad-hoc solution,
           * we make the currently focused paned the first_paned. To the
           * user this will seem like the paned cycling has been reset.
           */

          bobgui_paned_set_first_paned (paned, paned);
        }

      bobgui_paned_find_neighbours (paned, &next, &prev);

      if (reversed && prev &&
          prev != paned && paned != paned->first_paned)
        {
          focus = prev;
        }
      else if (!reversed && next &&
               next != paned && next != paned->first_paned)
        {
          focus = next;
        }
      else
        {
          bobgui_paned_accept_position (paned);
          return TRUE;
        }

      g_assert (focus);

      bobgui_paned_set_saved_focus (focus, paned->saved_focus);
      bobgui_paned_set_first_paned (focus, paned->first_paned);

      bobgui_paned_set_saved_focus (paned, NULL);
      bobgui_paned_set_first_paned (paned, NULL);

      bobgui_widget_grab_focus (BOBGUI_WIDGET (focus));

      if (!bobgui_widget_is_focus (BOBGUI_WIDGET (paned)))
        {
          paned->original_position = -1;
          paned->original_position = bobgui_paned_get_position (focus);
        }
    }
  else
    {
      BobguiPaned *focus;
      BobguiPaned *first;
      BobguiWidget *focus_child;

      bobgui_paned_find_neighbours (paned, &next, &prev);
      focus_child = bobgui_widget_get_focus_child (BOBGUI_WIDGET (paned));

      if (focus_child == paned->start_child)
        {
          if (reversed)
            {
              focus = prev;
              first = paned;
            }
          else
            {
              focus = paned;
              first = paned;
            }
        }
      else if (focus_child == paned->end_child)
        {
          if (reversed)
            {
              focus = paned;
              first = next;
            }
          else
            {
              focus = next;
              first = next;
            }
        }
      else
        {
          /* Focus is not inside this paned, and we don't have focus.
           * Presumably this happened because the application wants us
           * to start keyboard navigating.
           */
          focus = paned;

          if (reversed)
            first = paned;
          else
            first = next;
        }

      bobgui_paned_set_saved_focus (focus, bobgui_root_get_focus (bobgui_widget_get_root (BOBGUI_WIDGET (paned))));
      bobgui_paned_set_first_paned (focus, first);
      paned->original_position = bobgui_paned_get_position (focus);

      bobgui_widget_grab_focus (BOBGUI_WIDGET (focus));
   }

  return TRUE;
}

static gboolean
bobgui_paned_toggle_handle_focus (BobguiPaned *paned)
{
  /* This function/signal has the wrong name. It is called when you
   * press Tab or Shift-Tab and what we do is act as if
   * the user pressed Return and then Tab or Shift-Tab
   */
  if (bobgui_widget_is_focus (BOBGUI_WIDGET (paned)))
    bobgui_paned_accept_position (paned);

  return FALSE;
}

/**
 * bobgui_paned_set_wide_handle:
 * @paned: a `BobguiPaned`
 * @wide: the new value for the [property@Bobgui.Paned:wide-handle] property
 *
 * Sets whether the separator should be wide.
 */
void
bobgui_paned_set_wide_handle (BobguiPaned *paned,
                           gboolean  wide)
{
  gboolean old_wide;

  g_return_if_fail (BOBGUI_IS_PANED (paned));

  old_wide = bobgui_paned_get_wide_handle (paned);
  if (old_wide != wide)
    {
      if (wide)
        bobgui_widget_add_css_class (paned->handle_widget, "wide");
      else
        bobgui_widget_remove_css_class (paned->handle_widget, "wide");

      g_object_notify_by_pspec (G_OBJECT (paned), paned_props[PROP_WIDE_HANDLE]);
    }
}

/**
 * bobgui_paned_get_wide_handle:
 * @paned: a `BobguiPaned`
 *
 * Gets whether the separator should be wide.
 *
 * Returns: %TRUE if the paned should have a wide handle
 */
gboolean
bobgui_paned_get_wide_handle (BobguiPaned *paned)
{
  g_return_val_if_fail (BOBGUI_IS_PANED (paned), FALSE);

  return bobgui_widget_has_css_class (paned->handle_widget, "wide");
}
