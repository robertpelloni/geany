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

#include "bobguiscrolledwindow.h"

#include "bobguiadjustment.h"
#include "bobguiadjustmentprivate.h"
#include "bobguibuildable.h"
#include "bobguibuilderprivate.h"
#include "bobguidragsourceprivate.h"
#include "bobguieventcontrollermotion.h"
#include "bobguieventcontrollerscroll.h"
#include "bobguieventcontrollerprivate.h"
#include "bobguigesturedrag.h"
#include "bobguigesturelongpress.h"
#include "bobguigesturepan.h"
#include "bobguigesturesingle.h"
#include "bobguigestureswipe.h"
#include "bobguigestureprivate.h"
#include "bobguikineticscrollingprivate.h"
#include "bobguimain.h"
#include "bobguimarshalers.h"
#include "bobguiprivate.h"
#include "bobguiprogresstrackerprivate.h"
#include "bobguiscrollable.h"
#include "bobguiscrollbar.h"
#include "bobguisettingsprivate.h"
#include "bobguisnapshot.h"
#include "bobguirenderbackgroundprivate.h"
#include "bobguirenderborderprivate.h"
#include "bobguitypebuiltins.h"
#include "bobguiviewport.h"
#include "bobguiwidgetprivate.h"

#include <math.h>

/**
 * BobguiScrolledWindow:
 *
 * Makes its child scrollable.
 *
 * <picture>
 *   <source srcset="scrolledwindow-dark.png" media="(prefers-color-scheme: dark)">
 *   <img alt="An example BobguiScrolledWindow" src="scrolledwindow.png">
 * </picture>
 *
 * It does so using either internally added scrollbars or externally
 * associated adjustments, and optionally draws a frame around the child.
 *
 * Widgets with native scrolling support, i.e. those whose classes implement
 * the [iface@Bobgui.Scrollable] interface, are added directly. For other types
 * of widget, the class [class@Bobgui.Viewport] acts as an adaptor, giving
 * scrollability to other widgets. [method@Bobgui.ScrolledWindow.set_child]
 * intelligently accounts for whether or not the added child is a `BobguiScrollable`.
 * If it isn’t, then it wraps the child in a `BobguiViewport`. Therefore, you can
 * just add any child widget and not worry about the details.
 *
 * If [method@Bobgui.ScrolledWindow.set_child] has added a `BobguiViewport` for you,
 * it will be automatically removed when you unset the child.
 * Unless [property@Bobgui.ScrolledWindow:hscrollbar-policy] and
 * [property@Bobgui.ScrolledWindow:vscrollbar-policy] are %BOBGUI_POLICY_NEVER or
 * %BOBGUI_POLICY_EXTERNAL, `BobguiScrolledWindow` adds internal `BobguiScrollbar` widgets
 * around its child. The scroll position of the child, and if applicable the
 * scrollbars, is controlled by the [property@Bobgui.ScrolledWindow:hadjustment]
 * and [property@Bobgui.ScrolledWindow:vadjustment] that are associated with the
 * `BobguiScrolledWindow`. See the docs on [class@Bobgui.Scrollbar] for the details,
 * but note that the “step_increment” and “page_increment” fields are only
 * effective if the policy causes scrollbars to be present.
 *
 * If a `BobguiScrolledWindow` doesn’t behave quite as you would like, or
 * doesn’t have exactly the right layout, it’s very possible to set up
 * your own scrolling with `BobguiScrollbar` and for example a `BobguiGrid`.
 *
 * # Touch support
 *
 * `BobguiScrolledWindow` has built-in support for touch devices. When a
 * touchscreen is used, swiping will move the scrolled window, and will
 * expose 'kinetic' behavior. This can be turned off with the
 * [property@Bobgui.ScrolledWindow:kinetic-scrolling] property if it is undesired.
 *
 * `BobguiScrolledWindow` also displays visual 'overshoot' indication when
 * the content is pulled beyond the end, and this situation can be
 * captured with the [signal@Bobgui.ScrolledWindow::edge-overshot] signal.
 *
 * If no mouse device is present, the scrollbars will overlaid as
 * narrow, auto-hiding indicators over the content. If traditional
 * scrollbars are desired although no mouse is present, this behaviour
 * can be turned off with the [property@Bobgui.ScrolledWindow:overlay-scrolling]
 * property.
 *
 * # Shortcuts and Gestures
 *
 * The following signals have default keybindings:
 *
 * - [signal@Bobgui.ScrolledWindow::scroll-child]
 *
 * # CSS nodes
 *
 * `BobguiScrolledWindow` has a main CSS node with name scrolledwindow.
 * It gets a .frame style class added when [property@Bobgui.ScrolledWindow:has-frame]
 * is %TRUE.
 *
 * It uses subnodes with names overshoot and undershoot to draw the overflow
 * and underflow indications. These nodes get the .left, .right, .top or .bottom
 * style class added depending on where the indication is drawn.
 *
 * `BobguiScrolledWindow` also sets the positional style classes (.left, .right,
 * .top, .bottom) and style classes related to overlay scrolling
 * (.overlay-indicator, .dragging, .hovering) on its scrollbars.
 *
 * If both scrollbars are visible, the area where they meet is drawn
 * with a subnode named junction.
 *
 * # Accessibility
 *
 * Until BOBGUI 4.10, `BobguiScrolledWindow` used the [enum@Bobgui.AccessibleRole.group] role.
 *
 * Starting from BOBGUI 4.12, `BobguiScrolledWindow` uses the [enum@Bobgui.AccessibleRole.generic]
 * role.
 */

/* scrolled window policy and size requisition handling:
 *
 * bobgui size requisition works as follows:
 *   a widget upon size-request reports the width and height that it finds
 *   to be best suited to display its contents, including children.
 *   the width and/or height reported from a widget upon size requisition
 *   may be overridden by the user by specifying a width and/or height
 *   other than 0 through bobgui_widget_set_size_request().
 *
 * a scrolled window needs (for implementing all three policy types) to
 * request its width and height based on two different rationales.
 * 1)   the user wants the scrolled window to just fit into the space
 *      that it gets allocated for a specific dimension.
 * 1.1) this does not apply if the user specified a concrete value
 *      value for that specific dimension by either specifying usize for the
 *      scrolled window or for its child.
 * 2)   the user wants the scrolled window to take as much space up as
 *      is desired by the child for a specific dimension (i.e. POLICY_NEVER).
 *
 * also, kinda obvious:
 * 3)   a user would certainly not have chosen a scrolled window as a container
 *      for the child, if the resulting allocation takes up more space than the
 *      child would have allocated without the scrolled window.
 *
 * conclusions:
 * A) from 1) follows: the scrolled window shouldn’t request more space for a
 *    specific dimension than is required at minimum.
 * B) from 1.1) follows: the requisition may be overridden by usize of the scrolled
 *    window (done automatically) or by usize of the child (needs to be checked).
 * C) from 2) follows: for POLICY_NEVER, the scrolled window simply reports the
 *    child’s dimension.
 * D) from 3) follows: the scrolled window child’s minimum width and minimum height
 *    under A) at least correspond to the space taken up by its scrollbars.
 */

/* Kinetic scrolling */
#define MAX_OVERSHOOT_DISTANCE 100
#define DECELERATION_FRICTION 4
#define OVERSHOOT_FRICTION 20
#define VELOCITY_ACCUMULATION_FLOOR 0.33
#define VELOCITY_ACCUMULATION_CEIL 1.0
#define VELOCITY_ACCUMULATION_MAX 6.0

/* Animated scrolling */
#define ANIMATION_DURATION 200

/* Overlay scrollbars */
#define INDICATOR_FADE_OUT_DELAY 2000
#define INDICATOR_FADE_OUT_DURATION 1000
#define INDICATOR_FADE_OUT_TIME 500
#define INDICATOR_CLOSE_DISTANCE 5
#define INDICATOR_FAR_DISTANCE 10

/* Scrolled off indication */
#define UNDERSHOOT_SIZE 40

#define MAGIC_SCROLL_FACTOR 2.5

typedef struct _BobguiScrolledWindowClass         BobguiScrolledWindowClass;

struct _BobguiScrolledWindow
{
  BobguiWidget parent_instance;
};

struct _BobguiScrolledWindowClass
{
  BobguiWidgetClass parent_class;

  /* Unfortunately, BobguiScrollType is deficient in that there is
   * no horizontal/vertical variants for BOBGUI_SCROLL_START/END,
   * so we have to add an additional boolean flag.
   */
  gboolean (*scroll_child) (BobguiScrolledWindow *scrolled_window,
                            BobguiScrollType      scroll,
                            gboolean           horizontal);

  void (* move_focus_out) (BobguiScrolledWindow *scrolled_window,
                           BobguiDirectionType   direction);
};

typedef struct
{
  BobguiWidget *scrollbar;
  gboolean   over; /* either mouse over, or while dragging */
  gint64     last_scroll_time;
  guint      conceil_timer;

  double     current_pos;
  double     source_pos;
  double     target_pos;
  BobguiProgressTracker tracker;
  guint      tick_id;
  guint      over_timeout_id;
} Indicator;

typedef struct
{
  BobguiWidget *child;

  BobguiWidget     *hscrollbar;
  BobguiWidget     *vscrollbar;

  BobguiCssNode    *overshoot_node[4];
  BobguiCssNode    *undershoot_node[4];
  BobguiCssNode    *junction_node;

  Indicator hindicator;
  Indicator vindicator;

  BobguiCornerType  window_placement;
  guint    has_frame                : 1;
  guint    hscrollbar_policy        : 2;
  guint    vscrollbar_policy        : 2;
  guint    hscrollbar_visible       : 1;
  guint    vscrollbar_visible       : 1;
  guint    focus_out                : 1; /* used by ::move-focus-out implementation */
  guint    overlay_scrolling        : 1;
  guint    use_indicators           : 1;
  guint    auto_added_viewport      : 1;
  guint    propagate_natural_width  : 1;
  guint    propagate_natural_height : 1;
  guint    smooth_scroll            : 1;
  guint    scrolling                : 1;

  int      min_content_width;
  int      min_content_height;
  int      max_content_width;
  int      max_content_height;

  guint scroll_events_overshoot_id;

  /* Kinetic scrolling */
  BobguiGesture *long_press_gesture;
  BobguiGesture *swipe_gesture;
  BobguiKineticScrolling *hscrolling;
  BobguiKineticScrolling *vscrolling;
  gint64 last_deceleration_time;

  /* These two gestures are mutually exclusive */
  BobguiGesture *drag_gesture;
  BobguiGesture *pan_gesture;

  double drag_start_x;
  double drag_start_y;
  double prev_x;
  double prev_y;

  guint                  kinetic_scrolling         : 1;

  guint                  deceleration_id;
  GdkModifierType        scrolling_modifiers;

  double                 x_velocity;
  double                 y_velocity;

  double                 unclamped_hadj_value;
  double                 unclamped_vadj_value;
} BobguiScrolledWindowPrivate;

enum {
  PROP_0,
  PROP_HADJUSTMENT,
  PROP_VADJUSTMENT,
  PROP_HSCROLLBAR_POLICY,
  PROP_VSCROLLBAR_POLICY,
  PROP_WINDOW_PLACEMENT,
  PROP_HAS_FRAME,
  PROP_MIN_CONTENT_WIDTH,
  PROP_MIN_CONTENT_HEIGHT,
  PROP_KINETIC_SCROLLING,
  PROP_OVERLAY_SCROLLING,
  PROP_MAX_CONTENT_WIDTH,
  PROP_MAX_CONTENT_HEIGHT,
  PROP_PROPAGATE_NATURAL_WIDTH,
  PROP_PROPAGATE_NATURAL_HEIGHT,
  PROP_CHILD,
  NUM_PROPERTIES
};

/* Signals */
enum
{
  SCROLL_CHILD,
  MOVE_FOCUS_OUT,
  EDGE_OVERSHOT,
  EDGE_REACHED,
  LAST_SIGNAL
};

static void     bobgui_scrolled_window_set_property       (GObject           *object,
                                                        guint              prop_id,
                                                        const GValue      *value,
                                                        GParamSpec        *pspec);
static void     bobgui_scrolled_window_get_property       (GObject           *object,
                                                        guint              prop_id,
                                                        GValue            *value,
                                                        GParamSpec        *pspec);
static void     bobgui_scrolled_window_dispose            (GObject           *object);

static void     bobgui_scrolled_window_snapshot           (BobguiWidget         *widget,
                                                        BobguiSnapshot       *snapshot);
static void     bobgui_scrolled_window_size_allocate      (BobguiWidget         *widget,
                                                        int                width,
                                                        int                height,
                                                        int                baseline);
static gboolean bobgui_scrolled_window_focus              (BobguiWidget         *widget,
                                                        BobguiDirectionType   direction);
static gboolean bobgui_scrolled_window_scroll_child       (BobguiScrolledWindow *scrolled_window,
                                                        BobguiScrollType      scroll,
                                                        gboolean           horizontal);
static void     bobgui_scrolled_window_move_focus_out     (BobguiScrolledWindow *scrolled_window,
                                                        BobguiDirectionType   direction_type);

static void     bobgui_scrolled_window_relative_allocation(BobguiScrolledWindow *scrolled_window,
                                                        BobguiAllocation     *allocation);
static void     bobgui_scrolled_window_inner_allocation   (BobguiScrolledWindow *scrolled_window,
                                                        BobguiAllocation     *rect);
static void     bobgui_scrolled_window_allocate_scrollbar (BobguiScrolledWindow *scrolled_window,
                                                        BobguiWidget         *scrollbar,
                                                        BobguiAllocation     *allocation);
static void     bobgui_scrolled_window_allocate_child     (BobguiScrolledWindow   *swindow,
                                                        int                  width,
                                                        int                  height);
static void     bobgui_scrolled_window_adjustment_changed (BobguiAdjustment     *adjustment,
                                                        gpointer           data);
static void     bobgui_scrolled_window_adjustment_value_changed (BobguiAdjustment     *adjustment,
                                                              gpointer           data);
static gboolean bobgui_widget_should_animate              (BobguiWidget           *widget);
static void     bobgui_scrolled_window_measure (BobguiWidget      *widget,
                                             BobguiOrientation  orientation,
                                             int             for_size,
                                             int            *minimum_size,
                                             int            *natural_size,
                                             int            *minimum_baseline,
                                             int            *natural_baseline);
static void  bobgui_scrolled_window_map                   (BobguiWidget           *widget);
static void  bobgui_scrolled_window_unmap                 (BobguiWidget           *widget);
static void  bobgui_scrolled_window_realize               (BobguiWidget           *widget);
static void  bobgui_scrolled_window_unrealize             (BobguiWidget           *widget);
static void _bobgui_scrolled_window_set_adjustment_value  (BobguiScrolledWindow *scrolled_window,
                                                        BobguiAdjustment     *adjustment,
                                                        double             value);

static void bobgui_scrolled_window_cancel_deceleration (BobguiScrolledWindow *scrolled_window);

static gboolean _bobgui_scrolled_window_get_overshoot (BobguiScrolledWindow *scrolled_window,
                                                    int               *overshoot_x,
                                                    int               *overshoot_y);

static void     bobgui_scrolled_window_start_deceleration (BobguiScrolledWindow *scrolled_window);

static void     bobgui_scrolled_window_update_use_indicators (BobguiScrolledWindow *scrolled_window);
static void     remove_indicator     (BobguiScrolledWindow *sw,
                                      Indicator         *indicator);
static gboolean maybe_hide_indicator (gpointer data);

static void     indicator_start_fade (Indicator *indicator,
                                      double     pos);
static void     indicator_set_over   (Indicator *indicator,
                                      gboolean   over);

static gboolean scrolled_window_scroll (BobguiScrolledWindow        *scrolled_window,
                                        double                    delta_x,
                                        double                    delta_y,
                                        BobguiEventControllerScroll *scroll);

static guint signals[LAST_SIGNAL] = {0};
static GParamSpec *properties[NUM_PROPERTIES];

static void bobgui_scrolled_window_buildable_init (BobguiBuildableIface *iface);

G_DEFINE_TYPE_WITH_CODE (BobguiScrolledWindow, bobgui_scrolled_window, BOBGUI_TYPE_WIDGET,
                         G_ADD_PRIVATE (BobguiScrolledWindow)
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_BUILDABLE,
                                                bobgui_scrolled_window_buildable_init))

static BobguiBuildableIface *parent_buildable_iface;

static void
bobgui_scrolled_window_buildable_add_child (BobguiBuildable *buildable,
                                         BobguiBuilder   *builder,
                                         GObject      *child,
                                         const char   *type)
{
  if (BOBGUI_IS_WIDGET (child))
    {
      bobgui_buildable_child_deprecation_warning (buildable, builder, NULL, "child");
      bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (buildable), BOBGUI_WIDGET (child));
    }
  else
    {
      parent_buildable_iface->add_child (buildable, builder, child, type);
    }
}

static void
bobgui_scrolled_window_buildable_init (BobguiBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);

  iface->add_child = bobgui_scrolled_window_buildable_add_child;
}

static void
add_scroll_binding (BobguiWidgetClass *widget_class,
                    guint           keyval,
                    GdkModifierType mask,
                    BobguiScrollType   scroll,
                    gboolean        horizontal)
{
  guint keypad_keyval = keyval - GDK_KEY_Left + GDK_KEY_KP_Left;

  bobgui_widget_class_add_binding_signal (widget_class,
                                       keyval, mask,
                                       "scroll-child",
                                       "(ib)", scroll, horizontal);
  bobgui_widget_class_add_binding_signal (widget_class,
                                       keypad_keyval, mask,
                                       "scroll-child",
                                       "(ib)", scroll, horizontal);
}

static void
add_tab_bindings (BobguiWidgetClass   *widget_class,
                  GdkModifierType   modifiers,
                  BobguiDirectionType  direction)
{
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_Tab, modifiers,
                                       "move-focus-out",
                                       "(i)", direction);
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_KP_Tab, modifiers,
                                       "move-focus-out",
                                       "(i)", direction);
}

static void
motion_controller_leave (BobguiEventController   *controller,
                         BobguiScrolledWindow    *scrolled_window)
{
  BobguiScrolledWindowPrivate *priv = bobgui_scrolled_window_get_instance_private (scrolled_window);

  if (priv->use_indicators)
    {
      indicator_set_over (&priv->hindicator, FALSE);
      indicator_set_over (&priv->vindicator, FALSE);
    }
}

static void
update_scrollbar_positions (BobguiScrolledWindow *scrolled_window)
{
  BobguiScrolledWindowPrivate *priv = bobgui_scrolled_window_get_instance_private (scrolled_window);
  gboolean is_rtl;

  if (priv->hscrollbar != NULL)
    {
      if (priv->window_placement == BOBGUI_CORNER_TOP_LEFT ||
          priv->window_placement == BOBGUI_CORNER_TOP_RIGHT)
        {
          bobgui_widget_add_css_class (priv->hscrollbar, "bottom");
          bobgui_widget_remove_css_class (priv->hscrollbar, "top");
        }
      else
        {
          bobgui_widget_add_css_class (priv->hscrollbar, "top");
          bobgui_widget_remove_css_class (priv->hscrollbar, "bottom");
        }
    }

  if (priv->vscrollbar != NULL)
    {
      is_rtl = _bobgui_widget_get_direction (BOBGUI_WIDGET (scrolled_window)) == BOBGUI_TEXT_DIR_RTL;
      if ((is_rtl &&
          (priv->window_placement == BOBGUI_CORNER_TOP_RIGHT ||
           priv->window_placement == BOBGUI_CORNER_BOTTOM_RIGHT)) ||
         (!is_rtl &&
          (priv->window_placement == BOBGUI_CORNER_TOP_LEFT ||
           priv->window_placement == BOBGUI_CORNER_BOTTOM_LEFT)))
        {
          bobgui_widget_add_css_class (priv->vscrollbar, "right");
          bobgui_widget_remove_css_class (priv->vscrollbar, "left");
        }
      else
        {
          bobgui_widget_add_css_class (priv->vscrollbar, "left");
          bobgui_widget_remove_css_class (priv->vscrollbar, "right");
        }
    }
}

static void
bobgui_scrolled_window_direction_changed (BobguiWidget        *widget,
                                       BobguiTextDirection  previous_dir)
{
  BobguiScrolledWindow *scrolled_window = BOBGUI_SCROLLED_WINDOW (widget);

  update_scrollbar_positions (scrolled_window);

  BOBGUI_WIDGET_CLASS (bobgui_scrolled_window_parent_class)->direction_changed (widget, previous_dir);
}

static void
bobgui_scrolled_window_compute_expand (BobguiWidget *widget,
                                    gboolean  *hexpand,
                                    gboolean  *vexpand)
{
  BobguiScrolledWindow *scrolled_window = BOBGUI_SCROLLED_WINDOW (widget);
  BobguiScrolledWindowPrivate *priv = bobgui_scrolled_window_get_instance_private (scrolled_window);

  if (priv->child)
    {
      *hexpand = bobgui_widget_compute_expand (priv->child, BOBGUI_ORIENTATION_HORIZONTAL);
      *vexpand = bobgui_widget_compute_expand (priv->child, BOBGUI_ORIENTATION_VERTICAL);
    }
  else
    {
      *hexpand = FALSE;
      *vexpand = FALSE;
    }
}

static BobguiSizeRequestMode
bobgui_scrolled_window_get_request_mode (BobguiWidget *widget)
{
  BobguiScrolledWindow *scrolled_window = BOBGUI_SCROLLED_WINDOW (widget);
  BobguiScrolledWindowPrivate *priv = bobgui_scrolled_window_get_instance_private (scrolled_window);

  if (!priv->child || !bobgui_widget_get_visible (priv->child))
    return BOBGUI_SIZE_REQUEST_CONSTANT_SIZE;

  /* In many cases, we can actually get away with reporting constant-size,
   * so try to do that unless our reported size actually depends on the
   * child's size *and* the for_size we pass to the child depends on the
   * for_size passed to us.
   */
  if ((priv->hscrollbar_policy == BOBGUI_POLICY_NEVER &&
       priv->vscrollbar_policy == BOBGUI_POLICY_NEVER) ||
      (priv->hscrollbar_policy == BOBGUI_POLICY_NEVER &&
       priv->propagate_natural_height) ||
      (priv->vscrollbar_policy == BOBGUI_POLICY_NEVER &&
       priv->propagate_natural_width))
    return bobgui_widget_get_request_mode (priv->child);

  return BOBGUI_SIZE_REQUEST_CONSTANT_SIZE;
}

static void
bobgui_scrolled_window_class_init (BobguiScrolledWindowClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (class);

  gobject_class->set_property = bobgui_scrolled_window_set_property;
  gobject_class->get_property = bobgui_scrolled_window_get_property;
  gobject_class->dispose = bobgui_scrolled_window_dispose;

  widget_class->snapshot = bobgui_scrolled_window_snapshot;
  widget_class->size_allocate = bobgui_scrolled_window_size_allocate;
  widget_class->measure = bobgui_scrolled_window_measure;
  widget_class->focus = bobgui_scrolled_window_focus;
  widget_class->map = bobgui_scrolled_window_map;
  widget_class->unmap = bobgui_scrolled_window_unmap;
  widget_class->realize = bobgui_scrolled_window_realize;
  widget_class->unrealize = bobgui_scrolled_window_unrealize;
  widget_class->direction_changed = bobgui_scrolled_window_direction_changed;
  widget_class->compute_expand = bobgui_scrolled_window_compute_expand;
  widget_class->get_request_mode = bobgui_scrolled_window_get_request_mode;

  class->scroll_child = bobgui_scrolled_window_scroll_child;
  class->move_focus_out = bobgui_scrolled_window_move_focus_out;

  /**
   * BobguiScrolledWindow:hadjustment:
   *
   * The `BobguiAdjustment` for the horizontal position.
   */
  properties[PROP_HADJUSTMENT] =
      g_param_spec_object ("hadjustment", NULL, NULL,
                           BOBGUI_TYPE_ADJUSTMENT,
                           BOBGUI_PARAM_READWRITE|G_PARAM_CONSTRUCT|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiScrolledWindow:vadjustment:
   *
   * The `BobguiAdjustment` for the vertical position.
   */
  properties[PROP_VADJUSTMENT] =
      g_param_spec_object ("vadjustment", NULL, NULL,
                           BOBGUI_TYPE_ADJUSTMENT,
                           BOBGUI_PARAM_READWRITE|G_PARAM_CONSTRUCT|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiScrolledWindow:hscrollbar-policy:
   *
   * When the horizontal scrollbar is displayed.
   *
   * Use [method@Bobgui.ScrolledWindow.set_policy] to set
   * this property.
   */
  properties[PROP_HSCROLLBAR_POLICY] =
      g_param_spec_enum ("hscrollbar-policy", NULL, NULL,
                         BOBGUI_TYPE_POLICY_TYPE,
                         BOBGUI_POLICY_AUTOMATIC,
                         BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiScrolledWindow:vscrollbar-policy:
   *
   * When the vertical scrollbar is displayed.
   *
   * Use [method@Bobgui.ScrolledWindow.set_policy] to set
   * this property.
   */
  properties[PROP_VSCROLLBAR_POLICY] =
      g_param_spec_enum ("vscrollbar-policy", NULL, NULL,
                        BOBGUI_TYPE_POLICY_TYPE,
                        BOBGUI_POLICY_AUTOMATIC,
                        BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiScrolledWindow:window-placement: (getter get_placement) (setter set_placement)
   *
   * Where the contents are located with respect to the scrollbars.
   */
  properties[PROP_WINDOW_PLACEMENT] =
      g_param_spec_enum ("window-placement", NULL, NULL,
                        BOBGUI_TYPE_CORNER_TYPE,
                        BOBGUI_CORNER_TOP_LEFT,
                        BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiScrolledWindow:has-frame:
   *
   * Whether to draw a frame around the contents.
   */
  properties[PROP_HAS_FRAME] =
      g_param_spec_boolean ("has-frame", NULL, NULL,
                            FALSE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiScrolledWindow:min-content-width:
   *
   * The minimum content width of @scrolled_window.
   */
  properties[PROP_MIN_CONTENT_WIDTH] =
      g_param_spec_int ("min-content-width", NULL, NULL,
                        -1, G_MAXINT, -1,
                        BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiScrolledWindow:min-content-height:
   *
   * The minimum content height of @scrolled_window.
   */
  properties[PROP_MIN_CONTENT_HEIGHT] =
      g_param_spec_int ("min-content-height", NULL, NULL,
                        -1, G_MAXINT, -1,
                        BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiScrolledWindow:kinetic-scrolling:
   *
   * Whether kinetic scrolling is enabled or not.
   *
   * Kinetic scrolling only applies to devices with source %GDK_SOURCE_TOUCHSCREEN.
   */
  properties[PROP_KINETIC_SCROLLING] =
      g_param_spec_boolean ("kinetic-scrolling", NULL, NULL,
                            TRUE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiScrolledWindow:overlay-scrolling:
   *
   * Whether overlay scrolling is enabled or not.
   *
   * If it is, the scrollbars are only added as traditional widgets
   * when a mouse is present. Otherwise, they are overlaid on top of
   * the content, as narrow indicators.
   *
   * Note that overlay scrolling can also be globally disabled, with
   * the [property@Bobgui.Settings:bobgui-overlay-scrolling] setting.
   */
  properties[PROP_OVERLAY_SCROLLING] =
      g_param_spec_boolean ("overlay-scrolling", NULL, NULL,
                            TRUE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiScrolledWindow:max-content-width:
   *
   * The maximum content width of @scrolled_window.
   */
  properties[PROP_MAX_CONTENT_WIDTH] =
      g_param_spec_int ("max-content-width", NULL, NULL,
                        -1, G_MAXINT, -1,
                        BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiScrolledWindow:max-content-height:
   *
   * The maximum content height of @scrolled_window.
   */
  properties[PROP_MAX_CONTENT_HEIGHT] =
      g_param_spec_int ("max-content-height", NULL, NULL,
                        -1, G_MAXINT, -1,
                        BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiScrolledWindow:propagate-natural-width:
   *
   * Whether the natural width of the child should be calculated and propagated
   * through the scrolled window’s requested natural width.
   *
   * This is useful in cases where an attempt should be made to allocate exactly
   * enough space for the natural size of the child.
   */
  properties[PROP_PROPAGATE_NATURAL_WIDTH] =
      g_param_spec_boolean ("propagate-natural-width", NULL, NULL,
                            FALSE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiScrolledWindow:propagate-natural-height:
   *
   * Whether the natural height of the child should be calculated and propagated
   * through the scrolled window’s requested natural height.
   *
   * This is useful in cases where an attempt should be made to allocate exactly
   * enough space for the natural size of the child.
   */
  properties[PROP_PROPAGATE_NATURAL_HEIGHT] =
      g_param_spec_boolean ("propagate-natural-height", NULL, NULL,
                            FALSE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiScrolledWindow:child:
   *
   * The child widget.
   *
   * When setting this property, if the child widget does not implement
   * [iface@Bobgui.Scrollable], the scrolled window will add the child to
   * a [class@Bobgui.Viewport] and then set the viewport as the child.
   */
  properties[PROP_CHILD] =
      g_param_spec_object ("child", NULL, NULL,
                           BOBGUI_TYPE_WIDGET,
                           BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (gobject_class, NUM_PROPERTIES, properties);

  /**
   * BobguiScrolledWindow::scroll-child:
   * @scrolled_window: a `BobguiScrolledWindow`
   * @scroll: a `BobguiScrollType` describing how much to scroll
   * @horizontal: whether the keybinding scrolls the child
   *   horizontally or not
   *
   * Emitted when a keybinding that scrolls is pressed.
   *
   * This is a [keybinding signal](class.SignalAction.html).
   *
   * The horizontal or vertical adjustment is updated which triggers a
   * signal that the scrolled window’s child may listen to and scroll itself.
   *
   * Returns: whether the scroll happened
   */
  signals[SCROLL_CHILD] =
    g_signal_new (I_("scroll-child"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (BobguiScrolledWindowClass, scroll_child),
                  NULL, NULL,
                  _bobgui_marshal_BOOLEAN__ENUM_BOOLEAN,
                  G_TYPE_BOOLEAN, 2,
                  BOBGUI_TYPE_SCROLL_TYPE,
                  G_TYPE_BOOLEAN);
  g_signal_set_va_marshaller (signals[SCROLL_CHILD],
                              G_TYPE_FROM_CLASS (gobject_class),
                              _bobgui_marshal_BOOLEAN__ENUM_BOOLEANv);

  /**
   * BobguiScrolledWindow::move-focus-out:
   * @scrolled_window: a `BobguiScrolledWindow`
   * @direction_type: either %BOBGUI_DIR_TAB_FORWARD or
   *   %BOBGUI_DIR_TAB_BACKWARD
   *
   * Emitted when focus is moved away from the scrolled window by a
   * keybinding.
   *
   * This is a [keybinding signal](class.SignalAction.html).
   *
   * The default bindings for this signal are
   * <kbd>Ctrl</kbd>+<kbd>Tab</kbd> to move forward and
   * <kbd>Ctrl</kbd>+<kbd>Shift</kbd>+<kbd>Tab</kbd>` to move backward.
   */
  signals[MOVE_FOCUS_OUT] =
    g_signal_new (I_("move-focus-out"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (BobguiScrolledWindowClass, move_focus_out),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 1,
                  BOBGUI_TYPE_DIRECTION_TYPE);

  /**
   * BobguiScrolledWindow::edge-overshot:
   * @scrolled_window: a `BobguiScrolledWindow`
   * @pos: edge side that was hit
   *
   * Emitted whenever user initiated scrolling makes the scrolled
   * window firmly surpass the limits defined by the adjustment
   * in that orientation.
   *
   * A similar behavior without edge resistance is provided by the
   * [signal@Bobgui.ScrolledWindow::edge-reached] signal.
   *
   * Note: The @pos argument is LTR/RTL aware, so callers should be
   * aware too if intending to provide behavior on horizontal edges.
   */
  signals[EDGE_OVERSHOT] =
    g_signal_new (I_("edge-overshot"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST, 0,
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1, BOBGUI_TYPE_POSITION_TYPE);

  /**
   * BobguiScrolledWindow::edge-reached:
   * @scrolled_window: a `BobguiScrolledWindow`
   * @pos: edge side that was reached
   *
   * Emitted whenever user-initiated scrolling makes the scrolled
   * window exactly reach the lower or upper limits defined by the
   * adjustment in that orientation.
   *
   * A similar behavior with edge resistance is provided by the
   * [signal@Bobgui.ScrolledWindow::edge-overshot] signal.
   *
   * Note: The @pos argument is LTR/RTL aware, so callers should be
   * aware too if intending to provide behavior on horizontal edges.
   */
  signals[EDGE_REACHED] =
    g_signal_new (I_("edge-reached"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST, 0,
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1, BOBGUI_TYPE_POSITION_TYPE);

  add_scroll_binding (widget_class, GDK_KEY_Left,  GDK_CONTROL_MASK, BOBGUI_SCROLL_STEP_BACKWARD, TRUE);
  add_scroll_binding (widget_class, GDK_KEY_Right, GDK_CONTROL_MASK, BOBGUI_SCROLL_STEP_FORWARD,  TRUE);
  add_scroll_binding (widget_class, GDK_KEY_Up,    GDK_CONTROL_MASK, BOBGUI_SCROLL_STEP_BACKWARD, FALSE);
  add_scroll_binding (widget_class, GDK_KEY_Down,  GDK_CONTROL_MASK, BOBGUI_SCROLL_STEP_FORWARD,  FALSE);

  add_scroll_binding (widget_class, GDK_KEY_Page_Up,   GDK_CONTROL_MASK, BOBGUI_SCROLL_PAGE_BACKWARD, TRUE);
  add_scroll_binding (widget_class, GDK_KEY_Page_Down, GDK_CONTROL_MASK, BOBGUI_SCROLL_PAGE_FORWARD,  TRUE);
  add_scroll_binding (widget_class, GDK_KEY_Page_Up,   0,                BOBGUI_SCROLL_PAGE_BACKWARD, FALSE);
  add_scroll_binding (widget_class, GDK_KEY_Page_Down, 0,                BOBGUI_SCROLL_PAGE_FORWARD,  FALSE);

  add_scroll_binding (widget_class, GDK_KEY_Home, GDK_CONTROL_MASK, BOBGUI_SCROLL_START, TRUE);
  add_scroll_binding (widget_class, GDK_KEY_End,  GDK_CONTROL_MASK, BOBGUI_SCROLL_END,   TRUE);
  add_scroll_binding (widget_class, GDK_KEY_Home, 0,                BOBGUI_SCROLL_START, FALSE);
  add_scroll_binding (widget_class, GDK_KEY_End,  0,                BOBGUI_SCROLL_END,   FALSE);

  add_tab_bindings (widget_class, GDK_CONTROL_MASK, BOBGUI_DIR_TAB_FORWARD);
  add_tab_bindings (widget_class, GDK_CONTROL_MASK | GDK_SHIFT_MASK, BOBGUI_DIR_TAB_BACKWARD);

  bobgui_widget_class_set_css_name (widget_class, I_("scrolledwindow"));
  bobgui_widget_class_set_accessible_role (widget_class, BOBGUI_ACCESSIBLE_ROLE_GENERIC);
}

static gboolean
may_hscroll (BobguiScrolledWindow *scrolled_window)
{
  BobguiScrolledWindowPrivate *priv = bobgui_scrolled_window_get_instance_private (scrolled_window);

  return priv->hscrollbar_visible || priv->hscrollbar_policy == BOBGUI_POLICY_EXTERNAL;
}

static gboolean
may_vscroll (BobguiScrolledWindow *scrolled_window)
{
  BobguiScrolledWindowPrivate *priv = bobgui_scrolled_window_get_instance_private (scrolled_window);

  return priv->vscrollbar_visible || priv->vscrollbar_policy == BOBGUI_POLICY_EXTERNAL;
}

static inline gboolean
policy_may_be_visible (BobguiPolicyType policy)
{
  return policy == BOBGUI_POLICY_ALWAYS || policy == BOBGUI_POLICY_AUTOMATIC;
}

static void
scrolled_window_drag_begin_cb (BobguiScrolledWindow *scrolled_window,
                               double             start_x,
                               double             start_y,
                               BobguiGesture        *gesture)
{
  BobguiScrolledWindowPrivate *priv = bobgui_scrolled_window_get_instance_private (scrolled_window);
  GdkEventSequence *sequence;
  BobguiWidget *event_widget;

  priv->drag_start_x = priv->unclamped_hadj_value;
  priv->drag_start_y = priv->unclamped_vadj_value;
  bobgui_scrolled_window_cancel_deceleration (scrolled_window);
  sequence = bobgui_gesture_single_get_current_sequence (BOBGUI_GESTURE_SINGLE (gesture));
  event_widget = bobgui_gesture_get_last_target (gesture, sequence);

  if (event_widget == priv->vscrollbar || event_widget == priv->hscrollbar ||
      (!may_hscroll (scrolled_window) && !may_vscroll (scrolled_window)))
    bobgui_gesture_set_state (gesture, BOBGUI_EVENT_SEQUENCE_DENIED);
}

static void
bobgui_scrolled_window_invalidate_overshoot (BobguiScrolledWindow *scrolled_window)
{
  BobguiAllocation child_allocation;
  int overshoot_x, overshoot_y;

  if (!_bobgui_scrolled_window_get_overshoot (scrolled_window, &overshoot_x, &overshoot_y))
    return;

  bobgui_scrolled_window_relative_allocation (scrolled_window,
                                           &child_allocation);
  if (overshoot_x != 0)
    bobgui_widget_queue_draw (BOBGUI_WIDGET (scrolled_window));

  if (overshoot_y != 0)
    bobgui_widget_queue_draw (BOBGUI_WIDGET (scrolled_window));
}

static void
scrolled_window_drag_update_cb (BobguiScrolledWindow *scrolled_window,
                                double             offset_x,
                                double             offset_y,
                                BobguiGesture        *gesture)
{
  BobguiScrolledWindowPrivate *priv = bobgui_scrolled_window_get_instance_private (scrolled_window);
  GdkEventSequence *sequence;
  BobguiAdjustment *hadjustment;
  BobguiAdjustment *vadjustment;
  double dx, dy;

  sequence = bobgui_gesture_single_get_current_sequence (BOBGUI_GESTURE_SINGLE (gesture));

  if (bobgui_gesture_get_sequence_state (gesture, sequence) != BOBGUI_EVENT_SEQUENCE_CLAIMED &&
      !bobgui_drag_check_threshold_double (BOBGUI_WIDGET (scrolled_window),
                                        0, 0, offset_x, offset_y))
    return;

  bobgui_scrolled_window_invalidate_overshoot (scrolled_window);
  bobgui_gesture_set_state (gesture, BOBGUI_EVENT_SEQUENCE_CLAIMED);

  hadjustment = bobgui_scrollbar_get_adjustment (BOBGUI_SCROLLBAR (priv->hscrollbar));
  if (hadjustment && may_hscroll (scrolled_window))
    {
      dx = priv->drag_start_x - offset_x;
      _bobgui_scrolled_window_set_adjustment_value (scrolled_window,
                                                 hadjustment, dx);
    }

  vadjustment = bobgui_scrollbar_get_adjustment (BOBGUI_SCROLLBAR (priv->vscrollbar));
  if (vadjustment && may_vscroll (scrolled_window))
    {
      dy = priv->drag_start_y - offset_y;
      _bobgui_scrolled_window_set_adjustment_value (scrolled_window,
                                                 vadjustment, dy);
    }

  bobgui_scrolled_window_invalidate_overshoot (scrolled_window);
}

static void
bobgui_scrolled_window_decelerate (BobguiScrolledWindow *scrolled_window,
                                double             x_velocity,
                                double             y_velocity)
{
  BobguiScrolledWindowPrivate *priv = bobgui_scrolled_window_get_instance_private (scrolled_window);
  gboolean overshoot;

  overshoot = _bobgui_scrolled_window_get_overshoot (scrolled_window, NULL, NULL);
  priv->x_velocity = x_velocity;
  priv->y_velocity = y_velocity;

  /* Zero out vector components for which we don't scroll */
  if (!may_hscroll (scrolled_window))
    priv->x_velocity = 0;
  if (!may_vscroll (scrolled_window))
    priv->y_velocity = 0;

  if (priv->x_velocity != 0 || priv->y_velocity != 0 || overshoot)
    {
      if (priv->deceleration_id == 0)
        bobgui_scrolled_window_start_deceleration (scrolled_window);
      priv->x_velocity = priv->y_velocity = 0;
    }
  else
    {
      g_clear_pointer (&priv->hscrolling, bobgui_kinetic_scrolling_free);
      g_clear_pointer (&priv->vscrolling, bobgui_kinetic_scrolling_free);
    }
}

static void
scrolled_window_swipe_cb (BobguiScrolledWindow *scrolled_window,
                          double             x_velocity,
                          double             y_velocity)
{
  bobgui_scrolled_window_decelerate (scrolled_window, -x_velocity, -y_velocity);
}

static void
scrolled_window_long_press_cb (BobguiScrolledWindow *scrolled_window,
                               double             x,
                               double             y,
                               BobguiGesture        *gesture)
{
  bobgui_gesture_set_state (gesture, BOBGUI_EVENT_SEQUENCE_DENIED);
}

static void
scrolled_window_long_press_cancelled_cb (BobguiScrolledWindow *scrolled_window,
                                         BobguiGesture        *gesture)
{
  GdkEventSequence *sequence;
  GdkEvent *event;
  GdkEventType event_type;

  sequence = bobgui_gesture_get_last_updated_sequence (gesture);
  event = bobgui_gesture_get_last_event (gesture, sequence);
  event_type = gdk_event_get_event_type (event);

  if (event_type == GDK_TOUCH_BEGIN ||
      event_type == GDK_BUTTON_PRESS)
    bobgui_gesture_set_state (gesture, BOBGUI_EVENT_SEQUENCE_DENIED);
}

static void
bobgui_scrolled_window_check_attach_pan_gesture (BobguiScrolledWindow *sw)
{
  BobguiPropagationPhase phase = BOBGUI_PHASE_NONE;
  BobguiScrolledWindowPrivate *priv = bobgui_scrolled_window_get_instance_private (sw);

  if (priv->kinetic_scrolling &&
      ((may_hscroll (sw) && !may_vscroll (sw)) ||
       (!may_hscroll (sw) && may_vscroll (sw))))
    {
      BobguiOrientation orientation;

      if (may_hscroll (sw))
        orientation = BOBGUI_ORIENTATION_HORIZONTAL;
      else
        orientation = BOBGUI_ORIENTATION_VERTICAL;

      bobgui_gesture_pan_set_orientation (BOBGUI_GESTURE_PAN (priv->pan_gesture),
                                       orientation);
      phase = BOBGUI_PHASE_CAPTURE;
    }

  bobgui_event_controller_set_propagation_phase (BOBGUI_EVENT_CONTROLLER (priv->pan_gesture), phase);
}

static void
indicator_set_over (Indicator *indicator,
                    gboolean   over)
{
  g_clear_handle_id (&indicator->over_timeout_id, g_source_remove);

  if (indicator->over == over)
    return;

  indicator->over = over;

  if (indicator->over)
    bobgui_widget_add_css_class (indicator->scrollbar, "hovering");
  else
    bobgui_widget_remove_css_class (indicator->scrollbar, "hovering");

  bobgui_widget_queue_resize (indicator->scrollbar);
}

static gboolean
coords_close_to_indicator (BobguiScrolledWindow *sw,
                           Indicator         *indicator,
                           double             x,
                           double             y)
{
  BobguiScrolledWindowPrivate *priv = bobgui_scrolled_window_get_instance_private (sw);
  graphene_rect_t indicator_bounds;
  int distance;

  if (!bobgui_widget_compute_bounds (indicator->scrollbar, BOBGUI_WIDGET (sw), &indicator_bounds))
    return FALSE;

  if (indicator->over)
    distance = INDICATOR_FAR_DISTANCE;
  else
    distance = INDICATOR_CLOSE_DISTANCE;

  graphene_rect_inset (&indicator_bounds, - distance, - distance);

  if (indicator == &priv->hindicator)
    {
      if (y >= indicator_bounds.origin.y &&
          y < indicator_bounds.origin.y + indicator_bounds.size.height)
         return TRUE;
    }
  else if (indicator == &priv->vindicator)
    {
      if (x >= indicator_bounds.origin.x &&
          x < indicator_bounds.origin.x + indicator_bounds.size.width)
        return TRUE;
    }

  return FALSE;
}

static gboolean
enable_over_timeout_cb (gpointer user_data)
{
  Indicator *indicator = user_data;

  indicator_set_over (indicator, TRUE);
  return G_SOURCE_REMOVE;
}

static gboolean
check_update_scrollbar_proximity (BobguiScrolledWindow *sw,
                                  Indicator         *indicator,
                                  BobguiWidget         *target,
                                  double             x,
                                  double             y)
{
  BobguiScrolledWindowPrivate *priv = bobgui_scrolled_window_get_instance_private (sw);
  gboolean indicator_close, on_scrollbar, on_other_scrollbar;

  indicator_close = coords_close_to_indicator (sw, indicator, x, y);
  on_scrollbar = (target == indicator->scrollbar ||
                  bobgui_widget_is_ancestor (target, indicator->scrollbar));
  on_other_scrollbar = (!on_scrollbar &&
                        (target == priv->hindicator.scrollbar ||
                         target == priv->vindicator.scrollbar ||
                         bobgui_widget_is_ancestor (target, priv->hindicator.scrollbar) ||
                         bobgui_widget_is_ancestor (target, priv->vindicator.scrollbar)));


  g_clear_handle_id (&indicator->over_timeout_id, g_source_remove);

  if (on_scrollbar)
    indicator_set_over (indicator, TRUE);
  else if (indicator_close && !on_other_scrollbar)
    {
      indicator->over_timeout_id = g_timeout_add (30, enable_over_timeout_cb, indicator);
      gdk_source_set_static_name_by_id (indicator->over_timeout_id, "[bobgui] enable_over_timeout_cb");
    }
  else
    indicator_set_over (indicator, FALSE);

  return indicator_close;
}

static double
get_wheel_detent_scroll_step (BobguiScrolledWindow *sw,
                              BobguiOrientation     orientation)
{
  BobguiScrolledWindowPrivate *priv = bobgui_scrolled_window_get_instance_private (sw);
  BobguiScrollbar *scrollbar;
  BobguiAdjustment *adj;
  double page_size;
  double scroll_step;

  if (orientation == BOBGUI_ORIENTATION_HORIZONTAL)
    scrollbar = BOBGUI_SCROLLBAR (priv->hscrollbar);
  else
    scrollbar = BOBGUI_SCROLLBAR (priv->vscrollbar);

  if (!scrollbar)
    return 0;

  adj = bobgui_scrollbar_get_adjustment (scrollbar);
  page_size = bobgui_adjustment_get_page_size (adj);
  scroll_step = pow (page_size, 2.0 / 3.0);

  return scroll_step;
}

static gboolean
captured_scroll_cb (BobguiEventControllerScroll *scroll,
                    double                    delta_x,
                    double                    delta_y,
                    BobguiScrolledWindow        *scrolled_window)
{
  BobguiScrolledWindowPrivate *priv =
    bobgui_scrolled_window_get_instance_private (scrolled_window);
  int overshoot_x, overshoot_y;

  bobgui_scrolled_window_cancel_deceleration (scrolled_window);

  if (!may_hscroll (scrolled_window) &&
      !may_vscroll (scrolled_window))
    priv->scrolling = FALSE;

  if (priv->scrolling &&
      priv->scrolling_modifiers !=
      bobgui_event_controller_get_current_event_state (BOBGUI_EVENT_CONTROLLER (scroll)))
    priv->scrolling = FALSE;

  if (!priv->scrolling)
    return GDK_EVENT_PROPAGATE;

  scrolled_window_scroll (scrolled_window, delta_x, delta_y, scroll);

  _bobgui_scrolled_window_get_overshoot (scrolled_window, &overshoot_x, &overshoot_y);

  if (ABS (overshoot_x) == MAX_OVERSHOOT_DISTANCE ||
      ABS (overshoot_y) == MAX_OVERSHOOT_DISTANCE)
    {
      priv->scrolling = FALSE;
      return GDK_EVENT_PROPAGATE;
    }

  return GDK_EVENT_STOP;
}

static void
captured_motion (BobguiEventController *controller,
                 double              x,
                 double              y,
                 BobguiScrolledWindow  *sw)
{
  BobguiScrolledWindowPrivate *priv = bobgui_scrolled_window_get_instance_private (sw);
  GdkDevice *source_device;
  GdkInputSource input_source;
  GdkModifierType state;
  GdkEvent *event;
  BobguiWidget *target;

  if (priv->prev_x != x || priv->prev_y != y)
    {
      bobgui_scrolled_window_decelerate (sw, 0, 0);
      priv->prev_x = x;
      priv->prev_y = y;
    }

  if (!priv->use_indicators)
    return;

  if (!priv->child)
    return;

  target = bobgui_event_controller_get_target (controller);
  state = bobgui_event_controller_get_current_event_state (controller);
  event = bobgui_event_controller_get_current_event (controller);

  source_device = gdk_event_get_device (event);
  input_source = gdk_device_get_source (source_device);

  if (priv->hscrollbar_visible)
    indicator_start_fade (&priv->hindicator, 1.0);
  if (priv->vscrollbar_visible)
    indicator_start_fade (&priv->vindicator, 1.0);

  if ((target == priv->child ||
       bobgui_widget_is_ancestor (target, priv->child)) &&
      (state & (GDK_BUTTON1_MASK | GDK_BUTTON2_MASK | GDK_BUTTON3_MASK)) != 0)
    {
      indicator_set_over (&priv->hindicator, FALSE);
      indicator_set_over (&priv->vindicator, FALSE);
    }
  else if (input_source == GDK_SOURCE_PEN ||
           input_source == GDK_SOURCE_TRACKPOINT)
    {
      indicator_set_over (&priv->hindicator, TRUE);
      indicator_set_over (&priv->vindicator, TRUE);
    }
  else
    {
      if (!check_update_scrollbar_proximity (sw, &priv->vindicator, target, x, y))
        check_update_scrollbar_proximity (sw, &priv->hindicator, target, x, y);
      else
        indicator_set_over (&priv->hindicator, FALSE);
    }
}

static gboolean
start_scroll_deceleration_cb (gpointer user_data)
{
  BobguiScrolledWindow *scrolled_window = user_data;
  BobguiScrolledWindowPrivate *priv = bobgui_scrolled_window_get_instance_private (scrolled_window);

  priv->scroll_events_overshoot_id = 0;

  if (!priv->deceleration_id)
    bobgui_scrolled_window_start_deceleration (scrolled_window);

  return FALSE;
}

static void
scroll_controller_scroll_begin (BobguiEventControllerScroll *scroll,
                                BobguiScrolledWindow        *scrolled_window)
{
  BobguiScrolledWindowPrivate *priv = bobgui_scrolled_window_get_instance_private (scrolled_window);

  priv->smooth_scroll = TRUE;
}

static void
stop_kinetic_scrolling_cb (BobguiEventControllerScroll *scroll,
                           BobguiScrolledWindow        *scrolled_window)
{
  BobguiScrolledWindowPrivate *priv = bobgui_scrolled_window_get_instance_private (scrolled_window);

  if (priv->hscrolling)
    bobgui_kinetic_scrolling_stop (priv->hscrolling);

  if (priv->vscrolling)
    bobgui_kinetic_scrolling_stop (priv->vscrolling);
}

static gboolean
scrolled_window_scroll (BobguiScrolledWindow        *scrolled_window,
                        double                    delta_x,
                        double                    delta_y,
                        BobguiEventControllerScroll *scroll)
{
  BobguiScrolledWindowPrivate *priv =
    bobgui_scrolled_window_get_instance_private (scrolled_window);
  gboolean shifted;
  GdkModifierType state;
  gboolean changed = FALSE;

  state = bobgui_event_controller_get_current_event_state (BOBGUI_EVENT_CONTROLLER (scroll));
  shifted = (state & GDK_SHIFT_MASK) != 0;

  bobgui_scrolled_window_invalidate_overshoot (scrolled_window);

  if (shifted)
    {
      double delta;

      delta = delta_x;
      delta_x = delta_y;
      delta_y = delta;
    }

  if (delta_x != 0.0 &&
      may_hscroll (scrolled_window))
    {
      BobguiAdjustment *adj;
      double new_value;
      GdkScrollUnit scroll_unit;

      adj = bobgui_scrollbar_get_adjustment (BOBGUI_SCROLLBAR (priv->hscrollbar));
      scroll_unit = bobgui_event_controller_scroll_get_unit (scroll);

      if (scroll_unit == GDK_SCROLL_UNIT_WHEEL)
        {
          delta_x *= get_wheel_detent_scroll_step (scrolled_window,
                                                   BOBGUI_ORIENTATION_HORIZONTAL);
        }
      else if (scroll_unit == GDK_SCROLL_UNIT_SURFACE)
        delta_x *= MAGIC_SCROLL_FACTOR;

      new_value = priv->unclamped_hadj_value + delta_x;
      _bobgui_scrolled_window_set_adjustment_value (scrolled_window, adj,
                                                 new_value);
      changed |= TRUE;
    }

  if (delta_y != 0.0 &&
      may_vscroll (scrolled_window))
    {
      BobguiAdjustment *adj;
      double new_value;
      GdkScrollUnit scroll_unit;

      adj = bobgui_scrollbar_get_adjustment (BOBGUI_SCROLLBAR (priv->vscrollbar));
      scroll_unit = bobgui_event_controller_scroll_get_unit (scroll);

      if (scroll_unit == GDK_SCROLL_UNIT_WHEEL)
        {
          delta_y *= get_wheel_detent_scroll_step (scrolled_window,
                                                   BOBGUI_ORIENTATION_VERTICAL);
        }
      else if (scroll_unit == GDK_SCROLL_UNIT_SURFACE)
        delta_y *= MAGIC_SCROLL_FACTOR;

      new_value = priv->unclamped_vadj_value + delta_y;
      _bobgui_scrolled_window_set_adjustment_value (scrolled_window, adj,
                                                 new_value);
      changed |= TRUE;
    }

  g_clear_handle_id (&priv->scroll_events_overshoot_id, g_source_remove);

  if (!priv->smooth_scroll &&
      _bobgui_scrolled_window_get_overshoot (scrolled_window, NULL, NULL))
    {
      priv->scroll_events_overshoot_id =
        g_timeout_add (50, start_scroll_deceleration_cb, scrolled_window);
      gdk_source_set_static_name_by_id (priv->scroll_events_overshoot_id,
                                      "[bobgui] start_scroll_deceleration_cb");
    }

  return changed;
}

static gboolean
scroll_controller_scroll (BobguiEventControllerScroll *scroll,
                          double                    delta_x,
                          double                    delta_y,
                          BobguiScrolledWindow        *scrolled_window)
{
  BobguiScrolledWindowPrivate *priv =
    bobgui_scrolled_window_get_instance_private (scrolled_window);
  gboolean scrolled_in_non_scrollable_dir = FALSE;

  if (!may_hscroll (scrolled_window) &&
      !may_vscroll (scrolled_window))
    return GDK_EVENT_PROPAGATE;

  if (!priv->scrolling)
    {
      if (scrolled_window_scroll (scrolled_window, delta_x, delta_y, scroll))
        {
          if (priv->smooth_scroll)
            {
              priv->scrolling = TRUE;
              priv->scrolling_modifiers =
                bobgui_event_controller_get_current_event_state (BOBGUI_EVENT_CONTROLLER (scroll));
            }
        }
      else
        {
          scrolled_in_non_scrollable_dir =
            ((delta_x != 0 && !may_hscroll (scrolled_window)) ||
             (delta_y != 0 && !may_vscroll (scrolled_window)));
          if (scrolled_in_non_scrollable_dir)
            bobgui_scrolled_window_decelerate (scrolled_window, 0, 0);
        }
    }

  return (scrolled_in_non_scrollable_dir ||
          _bobgui_scrolled_window_get_overshoot (scrolled_window, NULL, NULL)) ?
    GDK_EVENT_PROPAGATE : GDK_EVENT_STOP;
}

static void
scroll_controller_scroll_end (BobguiEventControllerScroll *scroll,
                              BobguiScrolledWindow        *scrolled_window)
{
  BobguiScrolledWindowPrivate *priv = bobgui_scrolled_window_get_instance_private (scrolled_window);

  priv->smooth_scroll = FALSE;
  priv->scrolling = FALSE;
}

static void
scroll_controller_decelerate (BobguiEventControllerScroll *scroll,
                              double                    initial_vel_x,
                              double                    initial_vel_y,
                              BobguiScrolledWindow        *scrolled_window)
{
  BobguiScrolledWindowPrivate *priv =
    bobgui_scrolled_window_get_instance_private (scrolled_window);
  GdkScrollUnit scroll_unit;
  gboolean shifted;
  GdkModifierType state;

  if (!priv->scrolling)
    return;

  scroll_unit = bobgui_event_controller_scroll_get_unit (scroll);
  state = bobgui_event_controller_get_current_event_state (BOBGUI_EVENT_CONTROLLER (scroll));

  shifted = (state & GDK_SHIFT_MASK) != 0;

  if (shifted)
    {
      double tmp;

      tmp = initial_vel_x;
      initial_vel_x = initial_vel_y;
      initial_vel_y = tmp;
    }

  if (scroll_unit == GDK_SCROLL_UNIT_WHEEL)
    {
      initial_vel_x *= get_wheel_detent_scroll_step (scrolled_window,
                                                     BOBGUI_ORIENTATION_HORIZONTAL);

      initial_vel_y *= get_wheel_detent_scroll_step (scrolled_window,
                                                     BOBGUI_ORIENTATION_VERTICAL);
    }
  else
    {
      initial_vel_x *= MAGIC_SCROLL_FACTOR;
      initial_vel_y *= MAGIC_SCROLL_FACTOR;
    }

  bobgui_scrolled_window_decelerate (scrolled_window,
                                  initial_vel_x,
                                  initial_vel_y);
}

static void
bobgui_scrolled_window_update_scrollbar_visibility_flags (BobguiScrolledWindow *scrolled_window,
                                                       BobguiWidget         *scrollbar)
{
  BobguiScrolledWindowPrivate *priv = bobgui_scrolled_window_get_instance_private (scrolled_window);
  BobguiAdjustment *adjustment;

  if (scrollbar == NULL)
    return;

  adjustment = bobgui_scrollbar_get_adjustment (BOBGUI_SCROLLBAR (scrollbar));

  if (scrollbar == priv->hscrollbar)
    {
      if (priv->hscrollbar_policy == BOBGUI_POLICY_AUTOMATIC)
        {
          priv->hscrollbar_visible = (bobgui_adjustment_get_upper (adjustment) - bobgui_adjustment_get_lower (adjustment) >
                                      bobgui_adjustment_get_page_size (adjustment));
        }
    }
  else if (scrollbar == priv->vscrollbar)
    {
      if (priv->vscrollbar_policy == BOBGUI_POLICY_AUTOMATIC)
        {
          priv->vscrollbar_visible = (bobgui_adjustment_get_upper (adjustment) - bobgui_adjustment_get_lower (adjustment) >
                                      bobgui_adjustment_get_page_size (adjustment));
        }
    }
}

static void
bobgui_scrolled_window_size_allocate (BobguiWidget *widget,
                                   int        width,
                                   int        height,
                                   int        baseline)
{
  BobguiScrolledWindow *scrolled_window = BOBGUI_SCROLLED_WINDOW (widget);
  BobguiScrolledWindowPrivate *priv = bobgui_scrolled_window_get_instance_private (scrolled_window);
  BobguiAllocation child_allocation;
  int sb_width;
  int sb_height;

  /* Get possible scrollbar dimensions */
  bobgui_widget_measure (priv->vscrollbar, BOBGUI_ORIENTATION_HORIZONTAL, -1,
                      &sb_width, NULL, NULL, NULL);
  bobgui_widget_measure (priv->hscrollbar, BOBGUI_ORIENTATION_VERTICAL, -1,
                      &sb_height, NULL, NULL, NULL);

  if (priv->hscrollbar_policy == BOBGUI_POLICY_ALWAYS)
    priv->hscrollbar_visible = TRUE;
  else if (priv->hscrollbar_policy == BOBGUI_POLICY_NEVER ||
           priv->hscrollbar_policy == BOBGUI_POLICY_EXTERNAL)
    priv->hscrollbar_visible = FALSE;

  if (priv->vscrollbar_policy == BOBGUI_POLICY_ALWAYS)
    priv->vscrollbar_visible = TRUE;
  else if (priv->vscrollbar_policy == BOBGUI_POLICY_NEVER ||
           priv->vscrollbar_policy == BOBGUI_POLICY_EXTERNAL)
    priv->vscrollbar_visible = FALSE;

  if (priv->child && bobgui_widget_get_visible (priv->child))
    {
      int child_scroll_width;
      int child_scroll_height;
      gboolean previous_hvis;
      gboolean previous_vvis;
      guint count = 0;
      BobguiScrollable *scrollable_child = BOBGUI_SCROLLABLE (priv->child);
      BobguiScrollablePolicy hscroll_policy = bobgui_scrollable_get_hscroll_policy (scrollable_child);
      BobguiScrollablePolicy vscroll_policy = bobgui_scrollable_get_vscroll_policy (scrollable_child);

      /* Determine scrollbar visibility first via hfw apis */
      if (bobgui_widget_get_request_mode (priv->child) == BOBGUI_SIZE_REQUEST_HEIGHT_FOR_WIDTH)
        {
          if (hscroll_policy == BOBGUI_SCROLL_MINIMUM)
            bobgui_widget_measure (priv->child, BOBGUI_ORIENTATION_HORIZONTAL, -1,
                                &child_scroll_width, NULL, NULL, NULL);
          else
            bobgui_widget_measure (priv->child, BOBGUI_ORIENTATION_HORIZONTAL, -1,
                                NULL, &child_scroll_width, NULL, NULL);

          if (priv->vscrollbar_policy == BOBGUI_POLICY_AUTOMATIC)
            {
              /* First try without a vertical scrollbar if the content will fit the height
               * given the extra width of the scrollbar */
              if (vscroll_policy == BOBGUI_SCROLL_MINIMUM)
                bobgui_widget_measure (priv->child, BOBGUI_ORIENTATION_VERTICAL,
                                    MAX (width, child_scroll_width),
                                    &child_scroll_height, NULL, NULL, NULL);
              else
                bobgui_widget_measure (priv->child, BOBGUI_ORIENTATION_VERTICAL,
                                    MAX (width, child_scroll_width),
                                    NULL, &child_scroll_height, NULL, NULL);

              if (priv->hscrollbar_policy == BOBGUI_POLICY_AUTOMATIC)
                {
                  /* Does the content height fit the allocation height ? */
                  priv->vscrollbar_visible = child_scroll_height > height;

                  /* Does the content width fit the allocation with minus a possible scrollbar ? */
                  priv->hscrollbar_visible = child_scroll_width > width -
                    (priv->vscrollbar_visible && !priv->use_indicators ? sb_width : 0);

                  /* Now that we've guessed the hscrollbar, does the content height fit
                   * the possible new allocation height ?
                   */
                  priv->vscrollbar_visible = child_scroll_height > height -
                    (priv->hscrollbar_visible && !priv->use_indicators ? sb_height : 0);

                  /* Now that we've guessed the vscrollbar, does the content width fit
                   * the possible new allocation width ?
                   */
                  priv->hscrollbar_visible = child_scroll_width > width -
                    (priv->vscrollbar_visible && !priv->use_indicators ? sb_width : 0);
                }
              else /* priv->hscrollbar_policy != BOBGUI_POLICY_AUTOMATIC */
                {
                  priv->hscrollbar_visible = policy_may_be_visible (priv->hscrollbar_policy);
                  priv->vscrollbar_visible = child_scroll_height > height -
                    (priv->hscrollbar_visible && !priv->use_indicators ? sb_height : 0);
                }
            }
          else /* priv->vscrollbar_policy != BOBGUI_POLICY_AUTOMATIC */
            {
              priv->vscrollbar_visible = policy_may_be_visible (priv->vscrollbar_policy);

              if (priv->hscrollbar_policy == BOBGUI_POLICY_AUTOMATIC)
                priv->hscrollbar_visible = child_scroll_width > width -
                  (priv->vscrollbar_visible && !priv->use_indicators ? 0 : sb_width);
              else
                priv->hscrollbar_visible = policy_may_be_visible (priv->hscrollbar_policy);
            }
        }
      else /* BOBGUI_SIZE_REQUEST_WIDTH_FOR_HEIGHT */
        {
          if (vscroll_policy == BOBGUI_SCROLL_MINIMUM)
            bobgui_widget_measure (priv->child, BOBGUI_ORIENTATION_VERTICAL, -1,
                                &child_scroll_height, NULL, NULL, NULL);
          else
            bobgui_widget_measure (priv->child, BOBGUI_ORIENTATION_VERTICAL, -1,
                                NULL, &child_scroll_height, NULL, NULL);

          if (priv->hscrollbar_policy == BOBGUI_POLICY_AUTOMATIC)
            {
              /* First try without a horizontal scrollbar if the content will fit the width
               * given the extra height of the scrollbar */
              if (hscroll_policy == BOBGUI_SCROLL_MINIMUM)
                bobgui_widget_measure (priv->child, BOBGUI_ORIENTATION_HORIZONTAL,
                                    MAX (height, child_scroll_height),
                                    &child_scroll_width, NULL, NULL, NULL);
              else
                bobgui_widget_measure (priv->child, BOBGUI_ORIENTATION_HORIZONTAL,
                                    MAX (height, child_scroll_height),
                                    NULL, &child_scroll_width, NULL, NULL);

              if (priv->vscrollbar_policy == BOBGUI_POLICY_AUTOMATIC)
                {
                  /* Does the content width fit the allocation width ? */
                  priv->hscrollbar_visible = child_scroll_width > width;

                  /* Does the content height fit the allocation with minus a possible scrollbar ? */
                  priv->vscrollbar_visible = child_scroll_height > height -
                    (priv->hscrollbar_visible && !priv->use_indicators ? sb_height : 0);

                  /* Now that we've guessed the vscrollbar, does the content width fit
                   * the possible new allocation width ?
                   */
                  priv->hscrollbar_visible = child_scroll_width > width -
                    (priv->vscrollbar_visible && !priv->use_indicators ? sb_width : 0);

                  /* Now that we've guessed the hscrollbar, does the content height fit
                   * the possible new allocation height ?
                   */
                  priv->vscrollbar_visible = child_scroll_height > height -
                    (priv->hscrollbar_visible && !priv->use_indicators ? sb_height : 0);
                }
              else /* priv->vscrollbar_policy != BOBGUI_POLICY_AUTOMATIC */
                {
                  priv->vscrollbar_visible = policy_may_be_visible (priv->vscrollbar_policy);
                  priv->hscrollbar_visible = child_scroll_width > width -
                    (priv->vscrollbar_visible && !priv->use_indicators ? sb_width : 0);
                }
            }
          else /* priv->hscrollbar_policy != BOBGUI_POLICY_AUTOMATIC */
            {
              priv->hscrollbar_visible = policy_may_be_visible (priv->hscrollbar_policy);

              if (priv->vscrollbar_policy == BOBGUI_POLICY_AUTOMATIC)
                priv->vscrollbar_visible = child_scroll_height > height -
                  (priv->hscrollbar_visible && !priv->use_indicators ? sb_height : 0);
              else
                priv->vscrollbar_visible = policy_may_be_visible (priv->vscrollbar_policy);
            }
        }

      /* Now after guessing scrollbar visibility; fall back on the allocation loop which
       * observes the adjustments to detect scrollbar visibility and also avoids
       * infinite recursion
       */
      do
        {
          previous_hvis = priv->hscrollbar_visible;
          previous_vvis = priv->vscrollbar_visible;

          bobgui_scrolled_window_allocate_child (scrolled_window, width, height);

          /* Explicitly force scrollbar visibility checks.
           *
           * Since we make a guess above, the child might not decide to update the adjustments
           * if they logically did not change since the last configuration
           *
           * These will update priv->hscrollbar_visible and priv->vscrollbar_visible.
           */
          bobgui_scrolled_window_update_scrollbar_visibility_flags (scrolled_window,
                                                                 priv->hscrollbar);

          bobgui_scrolled_window_update_scrollbar_visibility_flags (scrolled_window,
                                                                 priv->vscrollbar);

          /* If, after the first iteration, the hscrollbar and the
           * vscrollbar flip visibility... or if one of the scrollbars flip
           * on each iteration indefinitely/infinitely, then we just need both
           * at this size.
           */
          if ((count &&
               previous_hvis != priv->hscrollbar_visible &&
               previous_vvis != priv->vscrollbar_visible) || count > 3)
            {
              priv->hscrollbar_visible = TRUE;
              priv->vscrollbar_visible = TRUE;

              bobgui_scrolled_window_allocate_child (scrolled_window, width, height);

              break;
            }

          count++;
        }
      while (previous_hvis != priv->hscrollbar_visible ||
             previous_vvis != priv->vscrollbar_visible);
    }
  else
    {
      priv->hscrollbar_visible = priv->hscrollbar_policy == BOBGUI_POLICY_ALWAYS;
      priv->vscrollbar_visible = priv->vscrollbar_policy == BOBGUI_POLICY_ALWAYS;
    }

  bobgui_widget_set_child_visible (priv->hscrollbar, priv->hscrollbar_visible);
  if (priv->hscrollbar_visible)
    {
      bobgui_scrolled_window_allocate_scrollbar (scrolled_window,
                                              priv->hscrollbar,
                                              &child_allocation);
      bobgui_widget_size_allocate (priv->hscrollbar, &child_allocation, -1);
    }

  bobgui_widget_set_child_visible (priv->vscrollbar, priv->vscrollbar_visible);
  if (priv->vscrollbar_visible)
    {
      bobgui_scrolled_window_allocate_scrollbar (scrolled_window,
                                              priv->vscrollbar,
                                              &child_allocation);
      bobgui_widget_size_allocate (priv->vscrollbar, &child_allocation, -1);
    }

  bobgui_scrolled_window_check_attach_pan_gesture (scrolled_window);
}

static void
bobgui_scrolled_window_measure (BobguiWidget      *widget,
                             BobguiOrientation  orientation,
                             int             for_size,
                             int            *minimum_size,
                             int            *natural_size,
                             int            *minimum_baseline,
                             int            *natural_baseline)
{
  BobguiScrolledWindow *scrolled_window = BOBGUI_SCROLLED_WINDOW (widget);
  BobguiScrolledWindowPrivate *priv = bobgui_scrolled_window_get_instance_private (scrolled_window);
  int minimum_req = 0, natural_req = 0;
  BobguiBorder sborder = { 0 };
  gboolean need_child_size = FALSE;

  if (priv->child)
    bobgui_scrollable_get_border (BOBGUI_SCROLLABLE (priv->child), &sborder);

  /*
   * First collect the child requisition, if we want to.
   */
  if (orientation == BOBGUI_ORIENTATION_VERTICAL)
    need_child_size = priv->propagate_natural_height || priv->vscrollbar_policy == BOBGUI_POLICY_NEVER;
  else
    need_child_size = priv->propagate_natural_width || priv->hscrollbar_policy == BOBGUI_POLICY_NEVER;

  if (priv->child && bobgui_widget_get_visible (priv->child) && need_child_size)
    {
      int min_child_size, nat_child_size;
      int child_for_size = -1;

      /* We can pass on the requested size if we have a scrollbar policy that prevents scrolling in that direction */
      if (for_size != -1 &&
          ((orientation == BOBGUI_ORIENTATION_VERTICAL && priv->hscrollbar_policy == BOBGUI_POLICY_NEVER)
           || (orientation == BOBGUI_ORIENTATION_HORIZONTAL && priv->vscrollbar_policy == BOBGUI_POLICY_NEVER)))
        {
          child_for_size = for_size;

          /* If the other scrollbar is always visible and not an overlay scrollbar we must subtract it from the measure */
          if (orientation == BOBGUI_ORIENTATION_VERTICAL && !priv->use_indicators && priv->vscrollbar_policy == BOBGUI_POLICY_ALWAYS)
            {
              int min_scrollbar_width;

              bobgui_widget_measure (priv->vscrollbar, BOBGUI_ORIENTATION_HORIZONTAL, -1,
                                  &min_scrollbar_width, NULL,
                                  NULL, NULL);

              child_for_size = MAX (0, child_for_size - min_scrollbar_width);
            }
          if (orientation == BOBGUI_ORIENTATION_HORIZONTAL && !priv->use_indicators && priv->hscrollbar_policy == BOBGUI_POLICY_ALWAYS)
            {
              int min_scrollbar_height;

              bobgui_widget_measure (priv->hscrollbar, BOBGUI_ORIENTATION_VERTICAL, -1,
                              &min_scrollbar_height, NULL,
                              NULL, NULL);

              child_for_size = MAX (0, child_for_size - min_scrollbar_height);
            }
        }

      bobgui_widget_measure (priv->child, orientation, child_for_size,
                          &min_child_size, &nat_child_size,
                          NULL, NULL);

      if (orientation == BOBGUI_ORIENTATION_HORIZONTAL)
        {
          if (priv->propagate_natural_width)
            natural_req += nat_child_size;

          if (priv->hscrollbar_policy == BOBGUI_POLICY_NEVER)
            {
              minimum_req += min_child_size;
            }
          else
            {
              int min = priv->min_content_width >= 0 ? priv->min_content_width : 0;
              int max = priv->max_content_width >= 0 ? priv->max_content_width : G_MAXINT;

              minimum_req = CLAMP (minimum_req, min, max);
              natural_req = CLAMP (natural_req, min, max);
            }
        }
      else /* BOBGUI_ORIENTATION_VERTICAL */
        {
          if (priv->propagate_natural_height)
            natural_req += nat_child_size;

          if (priv->vscrollbar_policy == BOBGUI_POLICY_NEVER)
            {
              minimum_req += min_child_size;
            }
          else
            {
              int min = priv->min_content_height >= 0 ? priv->min_content_height : 0;
              int max = priv->max_content_height >= 0 ? priv->max_content_height : G_MAXINT;

              minimum_req = CLAMP (minimum_req, min, max);
              natural_req = CLAMP (natural_req, min, max);
            }
        }
    }
  else
    {
      if (orientation == BOBGUI_ORIENTATION_HORIZONTAL)
        minimum_req = MAX (0, priv->min_content_width);
      else
        minimum_req = MAX (0, priv->min_content_height);
    }

  /* Ensure we make requests with natural size >= minimum size */
  natural_req = MAX (minimum_req, natural_req);

  /*
   * Now add to the requisition any additional space for surrounding scrollbars
   * and the special scrollable border.
   */
  if (orientation == BOBGUI_ORIENTATION_HORIZONTAL && policy_may_be_visible (priv->hscrollbar_policy))
    {
      int min_scrollbar_width, nat_scrollbar_width;

      bobgui_widget_measure (priv->hscrollbar, BOBGUI_ORIENTATION_HORIZONTAL, -1,
                          &min_scrollbar_width, &nat_scrollbar_width,
                          NULL, NULL);
      minimum_req = MAX (minimum_req, min_scrollbar_width + sborder.left + sborder.right);
      natural_req = MAX (natural_req, nat_scrollbar_width + sborder.left + sborder.right);
    }
  else if (orientation == BOBGUI_ORIENTATION_VERTICAL && policy_may_be_visible (priv->vscrollbar_policy))
    {
      int min_scrollbar_height, nat_scrollbar_height;

      bobgui_widget_measure (priv->vscrollbar, BOBGUI_ORIENTATION_VERTICAL, -1,
                          &min_scrollbar_height, &nat_scrollbar_height,
                          NULL, NULL);
      minimum_req = MAX (minimum_req, min_scrollbar_height + sborder.top + sborder.bottom);
      natural_req = MAX (natural_req, nat_scrollbar_height + sborder.top + sborder.bottom);
    }

  if (!priv->use_indicators)
    {
      if (orientation == BOBGUI_ORIENTATION_VERTICAL && priv->hscrollbar_policy == BOBGUI_POLICY_ALWAYS)
        {
          int min_scrollbar_height, nat_scrollbar_height;

          bobgui_widget_measure (priv->hscrollbar, BOBGUI_ORIENTATION_VERTICAL, -1,
                              &min_scrollbar_height, &nat_scrollbar_height,
                              NULL, NULL);

          minimum_req += min_scrollbar_height;
          natural_req += nat_scrollbar_height;
        }
      else if (orientation == BOBGUI_ORIENTATION_HORIZONTAL && priv->vscrollbar_policy == BOBGUI_POLICY_ALWAYS)
        {
          int min_scrollbar_width, nat_scrollbar_width;

          bobgui_widget_measure (priv->vscrollbar, BOBGUI_ORIENTATION_HORIZONTAL, -1,
                              &min_scrollbar_width, &nat_scrollbar_width,
                              NULL, NULL);
          minimum_req += min_scrollbar_width;
          natural_req += nat_scrollbar_width;
        }
    }

  *minimum_size = minimum_req;
  *natural_size = natural_req;
}

static void
bobgui_scrolled_window_snapshot_scrollbars_junction (BobguiScrolledWindow *scrolled_window,
                                                  BobguiSnapshot       *snapshot)
{
  BobguiScrolledWindowPrivate *priv = bobgui_scrolled_window_get_instance_private (scrolled_window);
  graphene_rect_t hscr_bounds, vscr_bounds;
  BobguiCssStyle *style;
  BobguiCssBoxes boxes;

  if (!bobgui_widget_compute_bounds (BOBGUI_WIDGET (priv->hscrollbar), BOBGUI_WIDGET (scrolled_window), &hscr_bounds))
    return;

  if (!bobgui_widget_compute_bounds (BOBGUI_WIDGET (priv->vscrollbar), BOBGUI_WIDGET (scrolled_window), &vscr_bounds))
    return;

  style = bobgui_css_node_get_style (priv->junction_node);

  bobgui_css_boxes_init_border_box (&boxes, style,
                                 vscr_bounds.origin.x, hscr_bounds.origin.y,
                                 vscr_bounds.size.width, hscr_bounds.size.height);

  bobgui_css_style_snapshot_background (&boxes, snapshot);
  bobgui_css_style_snapshot_border (&boxes, snapshot);
}

static void
bobgui_scrolled_window_snapshot_overshoot (BobguiScrolledWindow *scrolled_window,
                                        BobguiSnapshot       *snapshot)
{
  BobguiScrolledWindowPrivate *priv = bobgui_scrolled_window_get_instance_private (scrolled_window);
  int overshoot_x, overshoot_y;
  BobguiCssStyle *style;
  GdkRectangle rect;
  BobguiCssBoxes boxes;

  if (!_bobgui_scrolled_window_get_overshoot (scrolled_window, &overshoot_x, &overshoot_y))
    return;

  bobgui_scrolled_window_inner_allocation (scrolled_window, &rect);

  overshoot_x = CLAMP (overshoot_x, - MAX_OVERSHOOT_DISTANCE, MAX_OVERSHOOT_DISTANCE);
  overshoot_y = CLAMP (overshoot_y, - MAX_OVERSHOOT_DISTANCE, MAX_OVERSHOOT_DISTANCE);

  if (overshoot_x > 0)
    {
      style = bobgui_css_node_get_style (priv->overshoot_node[BOBGUI_POS_RIGHT]);
      bobgui_css_boxes_init_border_box (&boxes, style,
                                     rect.x + rect.width - overshoot_x, rect.y, overshoot_x, rect.height);
      bobgui_css_style_snapshot_background (&boxes, snapshot);
      bobgui_css_style_snapshot_border (&boxes, snapshot);
    }
  else if (overshoot_x < 0)
    {
      style = bobgui_css_node_get_style (priv->overshoot_node[BOBGUI_POS_LEFT]);
      bobgui_css_boxes_init_border_box (&boxes, style,
                                     rect.x, rect.y, -overshoot_x, rect.height);
      bobgui_css_style_snapshot_background (&boxes, snapshot);
      bobgui_css_style_snapshot_border (&boxes, snapshot);
    }

  if (overshoot_y > 0)
    {
      style = bobgui_css_node_get_style (priv->overshoot_node[BOBGUI_POS_BOTTOM]);
      bobgui_css_boxes_init_border_box (&boxes, style,
                                     rect.x, rect.y + rect.height - overshoot_y, rect.width, overshoot_y);
      bobgui_css_style_snapshot_background (&boxes, snapshot);
      bobgui_css_style_snapshot_border (&boxes, snapshot);
    }
  else if (overshoot_y < 0)
    {
      style = bobgui_css_node_get_style (priv->overshoot_node[BOBGUI_POS_TOP]);
      bobgui_css_boxes_init_border_box (&boxes, style,
                                     rect.x, rect.y, rect.width, -overshoot_y);
      bobgui_css_style_snapshot_background (&boxes, snapshot);
      bobgui_css_style_snapshot_border (&boxes, snapshot);
    }
}

static void
bobgui_scrolled_window_snapshot_undershoot (BobguiScrolledWindow *scrolled_window,
                                         BobguiSnapshot       *snapshot)
{
  BobguiScrolledWindowPrivate *priv = bobgui_scrolled_window_get_instance_private (scrolled_window);
  BobguiCssStyle *style;
  GdkRectangle rect;
  BobguiAdjustment *adj;
  BobguiCssBoxes boxes;

  bobgui_scrolled_window_inner_allocation (scrolled_window, &rect);

  adj = bobgui_scrollbar_get_adjustment (BOBGUI_SCROLLBAR (priv->hscrollbar));
  if (bobgui_adjustment_get_value (adj) < bobgui_adjustment_get_upper (adj) - bobgui_adjustment_get_page_size (adj))
    {
      style = bobgui_css_node_get_style (priv->undershoot_node[BOBGUI_POS_RIGHT]);
      bobgui_css_boxes_init_border_box (&boxes, style,
                                     rect.x + rect.width - UNDERSHOOT_SIZE, rect.y, UNDERSHOOT_SIZE, rect.height);
      bobgui_css_style_snapshot_background (&boxes, snapshot);
      bobgui_css_style_snapshot_border (&boxes, snapshot);
    }
  if (bobgui_adjustment_get_value (adj) > bobgui_adjustment_get_lower (adj))
    {
      style = bobgui_css_node_get_style (priv->undershoot_node[BOBGUI_POS_LEFT]);
      bobgui_css_boxes_init_border_box (&boxes, style,
                                     rect.x, rect.y, UNDERSHOOT_SIZE, rect.height);
      bobgui_css_style_snapshot_background (&boxes, snapshot);
      bobgui_css_style_snapshot_border (&boxes, snapshot);
    }

  adj = bobgui_scrollbar_get_adjustment (BOBGUI_SCROLLBAR (priv->vscrollbar));
  if (bobgui_adjustment_get_value (adj) < bobgui_adjustment_get_upper (adj) - bobgui_adjustment_get_page_size (adj))
    {
      style = bobgui_css_node_get_style (priv->undershoot_node[BOBGUI_POS_BOTTOM]);
      bobgui_css_boxes_init_border_box (&boxes, style,
                                     rect.x, rect.y + rect.height - UNDERSHOOT_SIZE, rect.width, UNDERSHOOT_SIZE);
      bobgui_css_style_snapshot_background (&boxes, snapshot);
      bobgui_css_style_snapshot_border (&boxes, snapshot);
    }
  if (bobgui_adjustment_get_value (adj) > bobgui_adjustment_get_lower (adj))
    {
      style = bobgui_css_node_get_style (priv->undershoot_node[BOBGUI_POS_TOP]);
      bobgui_css_boxes_init_border_box (&boxes, style,
                                     rect.x, rect.y, rect.width, UNDERSHOOT_SIZE);
      bobgui_css_style_snapshot_background (&boxes, snapshot);
      bobgui_css_style_snapshot_border (&boxes, snapshot);
    }
}

static void
bobgui_scrolled_window_init (BobguiScrolledWindow *scrolled_window)
{
  BobguiWidget *widget = BOBGUI_WIDGET (scrolled_window);
  BobguiScrolledWindowPrivate *priv = bobgui_scrolled_window_get_instance_private (scrolled_window);
  BobguiEventController *controller;
  BobguiCssNode *widget_node;
  GQuark classes[4] = {
    g_quark_from_static_string ("left"),
    g_quark_from_static_string ("right"),
    g_quark_from_static_string ("top"),
    g_quark_from_static_string ("bottom"),
  };
  int i;

  bobgui_widget_set_focusable (widget, TRUE);

  /* Instantiated by bobgui_scrolled_window_set_[hv]adjustment
   * which are both construct properties
   */
  priv->hscrollbar = NULL;
  priv->vscrollbar = NULL;
  priv->hscrollbar_policy = BOBGUI_POLICY_AUTOMATIC;
  priv->vscrollbar_policy = BOBGUI_POLICY_AUTOMATIC;
  priv->hscrollbar_visible = FALSE;
  priv->vscrollbar_visible = FALSE;
  priv->focus_out = FALSE;
  priv->auto_added_viewport = FALSE;
  priv->window_placement = BOBGUI_CORNER_TOP_LEFT;
  priv->min_content_width = -1;
  priv->min_content_height = -1;
  priv->max_content_width = -1;
  priv->max_content_height = -1;

  priv->overlay_scrolling = TRUE;

  priv->drag_gesture = bobgui_gesture_drag_new ();
  bobgui_gesture_single_set_touch_only (BOBGUI_GESTURE_SINGLE (priv->drag_gesture), TRUE);
  g_signal_connect_swapped (priv->drag_gesture, "drag-begin",
                            G_CALLBACK (scrolled_window_drag_begin_cb),
                            scrolled_window);
  g_signal_connect_swapped (priv->drag_gesture, "drag-update",
                            G_CALLBACK (scrolled_window_drag_update_cb),
                            scrolled_window);
  bobgui_widget_add_controller (widget, BOBGUI_EVENT_CONTROLLER (priv->drag_gesture));

  priv->pan_gesture = bobgui_gesture_pan_new (BOBGUI_ORIENTATION_VERTICAL);
  bobgui_gesture_single_set_touch_only (BOBGUI_GESTURE_SINGLE (priv->pan_gesture), TRUE);
  bobgui_widget_add_controller (widget, BOBGUI_EVENT_CONTROLLER (priv->pan_gesture));
  bobgui_gesture_group (priv->pan_gesture, priv->drag_gesture);

  priv->swipe_gesture = bobgui_gesture_swipe_new ();
  bobgui_gesture_single_set_touch_only (BOBGUI_GESTURE_SINGLE (priv->swipe_gesture), TRUE);
  g_signal_connect_swapped (priv->swipe_gesture, "swipe",
                            G_CALLBACK (scrolled_window_swipe_cb),
                            scrolled_window);
  bobgui_widget_add_controller (widget, BOBGUI_EVENT_CONTROLLER (priv->swipe_gesture));
  bobgui_gesture_group (priv->swipe_gesture, priv->drag_gesture);

  priv->long_press_gesture = bobgui_gesture_long_press_new ();
  bobgui_gesture_single_set_touch_only (BOBGUI_GESTURE_SINGLE (priv->long_press_gesture), TRUE);
  g_signal_connect_swapped (priv->long_press_gesture, "pressed",
                            G_CALLBACK (scrolled_window_long_press_cb),
                            scrolled_window);
  g_signal_connect_swapped (priv->long_press_gesture, "cancelled",
                            G_CALLBACK (scrolled_window_long_press_cancelled_cb),
                            scrolled_window);
  bobgui_widget_add_controller (widget, BOBGUI_EVENT_CONTROLLER (priv->long_press_gesture));
  bobgui_gesture_group (priv->long_press_gesture, priv->drag_gesture);

  bobgui_scrolled_window_set_kinetic_scrolling (scrolled_window, TRUE);

  controller = bobgui_event_controller_motion_new ();
  bobgui_event_controller_set_propagation_phase (controller, BOBGUI_PHASE_CAPTURE);
  g_signal_connect (controller, "motion",
                    G_CALLBACK (captured_motion), scrolled_window);
  bobgui_widget_add_controller (widget, controller);

  widget_node = bobgui_widget_get_css_node (widget);
  for (i = 0; i < 4; i++)
    {
      priv->overshoot_node[i] = bobgui_css_node_new ();
      bobgui_css_node_set_name (priv->overshoot_node[i], g_quark_from_static_string ("overshoot"));
      bobgui_css_node_add_class (priv->overshoot_node[i], classes[i]);
      bobgui_css_node_set_parent (priv->overshoot_node[i], widget_node);
      bobgui_css_node_set_state (priv->overshoot_node[i], bobgui_css_node_get_state (widget_node));
      g_object_unref (priv->overshoot_node[i]);

      priv->undershoot_node[i] = bobgui_css_node_new ();
      bobgui_css_node_set_name (priv->undershoot_node[i], g_quark_from_static_string ("undershoot"));
      bobgui_css_node_add_class (priv->undershoot_node[i], classes[i]);
      bobgui_css_node_set_parent (priv->undershoot_node[i], widget_node);
      bobgui_css_node_set_state (priv->undershoot_node[i], bobgui_css_node_get_state (widget_node));
      g_object_unref (priv->undershoot_node[i]);
    }

  bobgui_scrolled_window_update_use_indicators (scrolled_window);

  priv->junction_node = bobgui_css_node_new ();
  bobgui_css_node_set_name (priv->junction_node, g_quark_from_static_string ("junction"));
  bobgui_css_node_set_parent (priv->junction_node, widget_node);
  bobgui_css_node_set_state (priv->junction_node, bobgui_css_node_get_state (widget_node));
  g_object_unref (priv->junction_node);

  controller = bobgui_event_controller_scroll_new (BOBGUI_EVENT_CONTROLLER_SCROLL_BOTH_AXES |
                                                BOBGUI_EVENT_CONTROLLER_SCROLL_KINETIC);
  g_signal_connect (controller, "scroll-begin",
                    G_CALLBACK (scroll_controller_scroll_begin), scrolled_window);
  g_signal_connect (controller, "scroll",
                    G_CALLBACK (scroll_controller_scroll), scrolled_window);
  g_signal_connect (controller, "scroll-end",
                    G_CALLBACK (scroll_controller_scroll_end), scrolled_window);
  bobgui_widget_add_controller (widget, controller);

  controller = bobgui_event_controller_scroll_new (BOBGUI_EVENT_CONTROLLER_SCROLL_BOTH_AXES |
                                                BOBGUI_EVENT_CONTROLLER_SCROLL_KINETIC);
  bobgui_event_controller_set_propagation_phase (controller, BOBGUI_PHASE_CAPTURE);
  g_signal_connect (controller, "scroll-begin",
                    G_CALLBACK (stop_kinetic_scrolling_cb), scrolled_window);
  g_signal_connect (controller, "scroll",
                    G_CALLBACK (captured_scroll_cb), scrolled_window);
  g_signal_connect (controller, "decelerate",
                    G_CALLBACK (scroll_controller_decelerate), scrolled_window);
  bobgui_widget_add_controller (widget, controller);

  controller = bobgui_event_controller_motion_new ();
  g_signal_connect (controller, "leave",
                    G_CALLBACK (motion_controller_leave), scrolled_window);
  bobgui_widget_add_controller (widget, controller);
}

/**
 * bobgui_scrolled_window_new:
 *
 * Creates a new scrolled window.
 *
 * Returns: a new scrolled window
 */
BobguiWidget *
bobgui_scrolled_window_new (void)
{
  return g_object_new (BOBGUI_TYPE_SCROLLED_WINDOW, NULL);
}

/**
 * bobgui_scrolled_window_set_hadjustment:
 * @scrolled_window: a `BobguiScrolledWindow`
 * @hadjustment: (nullable): the `BobguiAdjustment` to use, or %NULL to create a new one
 *
 * Sets the `BobguiAdjustment` for the horizontal scrollbar.
 */
void
bobgui_scrolled_window_set_hadjustment (BobguiScrolledWindow *scrolled_window,
                                     BobguiAdjustment     *hadjustment)
{
  BobguiScrolledWindowPrivate *priv = bobgui_scrolled_window_get_instance_private (scrolled_window);

  g_return_if_fail (BOBGUI_IS_SCROLLED_WINDOW (scrolled_window));

  if (hadjustment)
    g_return_if_fail (BOBGUI_IS_ADJUSTMENT (hadjustment));
  else
    hadjustment = (BobguiAdjustment*) g_object_new (BOBGUI_TYPE_ADJUSTMENT, NULL);

  if (!priv->hscrollbar)
    {
      priv->hscrollbar = bobgui_scrollbar_new (BOBGUI_ORIENTATION_HORIZONTAL, hadjustment);

      bobgui_widget_insert_before (priv->hscrollbar, BOBGUI_WIDGET (scrolled_window), priv->vscrollbar);
      update_scrollbar_positions (scrolled_window);
    }
  else
    {
      BobguiAdjustment *old_adjustment;

      old_adjustment = bobgui_scrollbar_get_adjustment (BOBGUI_SCROLLBAR (priv->hscrollbar));
      if (old_adjustment == hadjustment)
        return;

      g_signal_handlers_disconnect_by_func (old_adjustment,
                                            bobgui_scrolled_window_adjustment_changed,
                                            scrolled_window);
      g_signal_handlers_disconnect_by_func (old_adjustment,
                                            bobgui_scrolled_window_adjustment_value_changed,
                                            scrolled_window);

      bobgui_adjustment_enable_animation (old_adjustment, NULL, 0);
      bobgui_scrollbar_set_adjustment (BOBGUI_SCROLLBAR (priv->hscrollbar), hadjustment);
    }

  hadjustment = bobgui_scrollbar_get_adjustment (BOBGUI_SCROLLBAR (priv->hscrollbar));

  g_signal_connect (hadjustment,
                    "changed",
                    G_CALLBACK (bobgui_scrolled_window_adjustment_changed),
                    scrolled_window);
  g_signal_connect (hadjustment,
                    "value-changed",
                    G_CALLBACK (bobgui_scrolled_window_adjustment_value_changed),
                    scrolled_window);

  bobgui_scrolled_window_adjustment_changed (hadjustment, scrolled_window);
  bobgui_scrolled_window_adjustment_value_changed (hadjustment, scrolled_window);

  if (priv->child)
    bobgui_scrollable_set_hadjustment (BOBGUI_SCROLLABLE (priv->child), hadjustment);

  if (bobgui_widget_should_animate (BOBGUI_WIDGET (scrolled_window)))
    bobgui_adjustment_enable_animation (hadjustment, bobgui_widget_get_frame_clock (BOBGUI_WIDGET (scrolled_window)), ANIMATION_DURATION);

  g_object_notify_by_pspec (G_OBJECT (scrolled_window), properties[PROP_HADJUSTMENT]);
}

/**
 * bobgui_scrolled_window_set_vadjustment:
 * @scrolled_window: a `BobguiScrolledWindow`
 * @vadjustment: (nullable): the `BobguiAdjustment` to use, or %NULL to create a new one
 *
 * Sets the `BobguiAdjustment` for the vertical scrollbar.
 */
void
bobgui_scrolled_window_set_vadjustment (BobguiScrolledWindow *scrolled_window,
                                     BobguiAdjustment     *vadjustment)
{
  BobguiScrolledWindowPrivate *priv = bobgui_scrolled_window_get_instance_private (scrolled_window);

  g_return_if_fail (BOBGUI_IS_SCROLLED_WINDOW (scrolled_window));

  if (vadjustment)
    g_return_if_fail (BOBGUI_IS_ADJUSTMENT (vadjustment));
  else
    vadjustment = (BobguiAdjustment*) g_object_new (BOBGUI_TYPE_ADJUSTMENT, NULL);

  if (!priv->vscrollbar)
    {
      priv->vscrollbar = bobgui_scrollbar_new (BOBGUI_ORIENTATION_VERTICAL, vadjustment);

      bobgui_widget_insert_after (priv->vscrollbar, BOBGUI_WIDGET (scrolled_window), priv->hscrollbar);
      update_scrollbar_positions (scrolled_window);
    }
  else
    {
      BobguiAdjustment *old_adjustment;

      old_adjustment = bobgui_scrollbar_get_adjustment (BOBGUI_SCROLLBAR (priv->vscrollbar));
      if (old_adjustment == vadjustment)
        return;

      g_signal_handlers_disconnect_by_func (old_adjustment,
                                            bobgui_scrolled_window_adjustment_changed,
                                            scrolled_window);
      g_signal_handlers_disconnect_by_func (old_adjustment,
                                            bobgui_scrolled_window_adjustment_value_changed,
                                            scrolled_window);

      bobgui_adjustment_enable_animation (old_adjustment, NULL, 0);
      bobgui_scrollbar_set_adjustment (BOBGUI_SCROLLBAR (priv->vscrollbar), vadjustment);
    }

  vadjustment = bobgui_scrollbar_get_adjustment (BOBGUI_SCROLLBAR (priv->vscrollbar));

  g_signal_connect (vadjustment,
                    "changed",
                    G_CALLBACK (bobgui_scrolled_window_adjustment_changed),
                    scrolled_window);
  g_signal_connect (vadjustment,
                    "value-changed",
                    G_CALLBACK (bobgui_scrolled_window_adjustment_value_changed),
                    scrolled_window);

  bobgui_scrolled_window_adjustment_changed (vadjustment, scrolled_window);
  bobgui_scrolled_window_adjustment_value_changed (vadjustment, scrolled_window);

  if (priv->child)
    bobgui_scrollable_set_vadjustment (BOBGUI_SCROLLABLE (priv->child), vadjustment);

  if (bobgui_widget_should_animate (BOBGUI_WIDGET (scrolled_window)))
    bobgui_adjustment_enable_animation (vadjustment, bobgui_widget_get_frame_clock (BOBGUI_WIDGET (scrolled_window)), ANIMATION_DURATION);

  g_object_notify_by_pspec (G_OBJECT (scrolled_window), properties[PROP_VADJUSTMENT]);
}

/**
 * bobgui_scrolled_window_get_hadjustment:
 * @scrolled_window: a `BobguiScrolledWindow`
 *
 * Returns the horizontal scrollbar’s adjustment.
 *
 * This is the adjustment used to connect the horizontal scrollbar
 * to the child widget’s horizontal scroll functionality.
 *
 * Returns: (transfer none): the horizontal `BobguiAdjustment`
 */
BobguiAdjustment*
bobgui_scrolled_window_get_hadjustment (BobguiScrolledWindow *scrolled_window)
{
  BobguiScrolledWindowPrivate *priv = bobgui_scrolled_window_get_instance_private (scrolled_window);

  g_return_val_if_fail (BOBGUI_IS_SCROLLED_WINDOW (scrolled_window), NULL);

  return bobgui_scrollbar_get_adjustment (BOBGUI_SCROLLBAR (priv->hscrollbar));
}

/**
 * bobgui_scrolled_window_get_vadjustment:
 * @scrolled_window: a `BobguiScrolledWindow`
 *
 * Returns the vertical scrollbar’s adjustment.
 *
 * This is the adjustment used to connect the vertical
 * scrollbar to the child widget’s vertical scroll functionality.
 *
 * Returns: (transfer none): the vertical `BobguiAdjustment`
 */
BobguiAdjustment*
bobgui_scrolled_window_get_vadjustment (BobguiScrolledWindow *scrolled_window)
{
  BobguiScrolledWindowPrivate *priv = bobgui_scrolled_window_get_instance_private (scrolled_window);

  g_return_val_if_fail (BOBGUI_IS_SCROLLED_WINDOW (scrolled_window), NULL);

  return bobgui_scrollbar_get_adjustment (BOBGUI_SCROLLBAR (priv->vscrollbar));
}

/**
 * bobgui_scrolled_window_get_hscrollbar:
 * @scrolled_window: a `BobguiScrolledWindow`
 *
 * Returns the horizontal scrollbar of @scrolled_window.
 *
 * Returns: (transfer none): the horizontal scrollbar of the scrolled window.
 */
BobguiWidget*
bobgui_scrolled_window_get_hscrollbar (BobguiScrolledWindow *scrolled_window)
{
  BobguiScrolledWindowPrivate *priv = bobgui_scrolled_window_get_instance_private (scrolled_window);

  g_return_val_if_fail (BOBGUI_IS_SCROLLED_WINDOW (scrolled_window), NULL);

  return priv->hscrollbar;
}

/**
 * bobgui_scrolled_window_get_vscrollbar:
 * @scrolled_window: a `BobguiScrolledWindow`
 *
 * Returns the vertical scrollbar of @scrolled_window.
 *
 * Returns: (transfer none): the vertical scrollbar of the scrolled window.
 */
BobguiWidget*
bobgui_scrolled_window_get_vscrollbar (BobguiScrolledWindow *scrolled_window)
{
  BobguiScrolledWindowPrivate *priv = bobgui_scrolled_window_get_instance_private (scrolled_window);

  g_return_val_if_fail (BOBGUI_IS_SCROLLED_WINDOW (scrolled_window), NULL);

  return priv->vscrollbar;
}

/**
 * bobgui_scrolled_window_set_policy:
 * @scrolled_window: a `BobguiScrolledWindow`
 * @hscrollbar_policy: policy for horizontal bar
 * @vscrollbar_policy: policy for vertical bar
 *
 * Sets the scrollbar policy for the horizontal and vertical scrollbars.
 *
 * The policy determines when the scrollbar should appear; it is a value
 * from the [enum@Bobgui.PolicyType] enumeration. If %BOBGUI_POLICY_ALWAYS, the
 * scrollbar is always present; if %BOBGUI_POLICY_NEVER, the scrollbar is
 * never present; if %BOBGUI_POLICY_AUTOMATIC, the scrollbar is present only
 * if needed (that is, if the slider part of the bar would be smaller
 * than the trough — the display is larger than the page size).
 */
void
bobgui_scrolled_window_set_policy (BobguiScrolledWindow *scrolled_window,
                                BobguiPolicyType      hscrollbar_policy,
                                BobguiPolicyType      vscrollbar_policy)
{
  BobguiScrolledWindowPrivate *priv = bobgui_scrolled_window_get_instance_private (scrolled_window);
  GObject *object = G_OBJECT (scrolled_window);

  g_return_if_fail (BOBGUI_IS_SCROLLED_WINDOW (scrolled_window));

  if ((priv->hscrollbar_policy != hscrollbar_policy) ||
      (priv->vscrollbar_policy != vscrollbar_policy))
    {
      priv->hscrollbar_policy = hscrollbar_policy;
      priv->vscrollbar_policy = vscrollbar_policy;

      bobgui_widget_queue_resize (BOBGUI_WIDGET (scrolled_window));

      g_object_notify_by_pspec (object, properties[PROP_HSCROLLBAR_POLICY]);
      g_object_notify_by_pspec (object, properties[PROP_VSCROLLBAR_POLICY]);
    }
}

/**
 * bobgui_scrolled_window_get_policy:
 * @scrolled_window: a `BobguiScrolledWindow`
 * @hscrollbar_policy: (out) (optional): location to store the policy
 *   for the horizontal scrollbar
 * @vscrollbar_policy: (out) (optional): location to store the policy
 *   for the vertical scrollbar
 *
 * Retrieves the current policy values for the horizontal and vertical
 * scrollbars.
 *
 * See [method@Bobgui.ScrolledWindow.set_policy].
 */
void
bobgui_scrolled_window_get_policy (BobguiScrolledWindow *scrolled_window,
                                BobguiPolicyType     *hscrollbar_policy,
                                BobguiPolicyType     *vscrollbar_policy)
{
  BobguiScrolledWindowPrivate *priv = bobgui_scrolled_window_get_instance_private (scrolled_window);

  g_return_if_fail (BOBGUI_IS_SCROLLED_WINDOW (scrolled_window));

  if (hscrollbar_policy)
    *hscrollbar_policy = priv->hscrollbar_policy;
  if (vscrollbar_policy)
    *vscrollbar_policy = priv->vscrollbar_policy;
}

/**
 * bobgui_scrolled_window_set_placement: (set-property window-placement)
 * @scrolled_window: a `BobguiScrolledWindow`
 * @window_placement: position of the child window
 *
 * Sets the placement of the contents with respect to the scrollbars
 * for the scrolled window.
 *
 * The default is %BOBGUI_CORNER_TOP_LEFT, meaning the child is
 * in the top left, with the scrollbars underneath and to the right.
 * Other values in [enum@Bobgui.CornerType] are %BOBGUI_CORNER_TOP_RIGHT,
 * %BOBGUI_CORNER_BOTTOM_LEFT, and %BOBGUI_CORNER_BOTTOM_RIGHT.
 *
 * See also [method@Bobgui.ScrolledWindow.get_placement] and
 * [method@Bobgui.ScrolledWindow.unset_placement].
 */
void
bobgui_scrolled_window_set_placement (BobguiScrolledWindow *scrolled_window,
                                   BobguiCornerType      window_placement)
{
  BobguiScrolledWindowPrivate *priv = bobgui_scrolled_window_get_instance_private (scrolled_window);

  g_return_if_fail (BOBGUI_IS_SCROLLED_WINDOW (scrolled_window));

  if (priv->window_placement != window_placement)
    {
      priv->window_placement = window_placement;
      update_scrollbar_positions (scrolled_window);

      bobgui_widget_queue_resize (BOBGUI_WIDGET (scrolled_window));

      g_object_notify_by_pspec (G_OBJECT (scrolled_window), properties[PROP_WINDOW_PLACEMENT]);
    }
}

/**
 * bobgui_scrolled_window_get_placement: (get-property window-placement)
 * @scrolled_window: a `BobguiScrolledWindow`
 *
 * Gets the placement of the contents with respect to the scrollbars.
 *
 * Returns: the current placement value.
 */
BobguiCornerType
bobgui_scrolled_window_get_placement (BobguiScrolledWindow *scrolled_window)
{
  BobguiScrolledWindowPrivate *priv = bobgui_scrolled_window_get_instance_private (scrolled_window);

  g_return_val_if_fail (BOBGUI_IS_SCROLLED_WINDOW (scrolled_window), BOBGUI_CORNER_TOP_LEFT);

  return priv->window_placement;
}

/**
 * bobgui_scrolled_window_unset_placement:
 * @scrolled_window: a `BobguiScrolledWindow`
 *
 * Unsets the placement of the contents with respect to the scrollbars.
 *
 * If no window placement is set for a scrolled window,
 * it defaults to %BOBGUI_CORNER_TOP_LEFT.
 */
void
bobgui_scrolled_window_unset_placement (BobguiScrolledWindow *scrolled_window)
{
  g_return_if_fail (BOBGUI_IS_SCROLLED_WINDOW (scrolled_window));

  bobgui_scrolled_window_set_placement (scrolled_window, BOBGUI_CORNER_TOP_LEFT);
}

/**
 * bobgui_scrolled_window_set_has_frame:
 * @scrolled_window: a `BobguiScrolledWindow`
 * @has_frame: whether to draw a frame around scrolled window contents
 *
 * Changes the frame drawn around the contents of @scrolled_window.
 */
void
bobgui_scrolled_window_set_has_frame (BobguiScrolledWindow *scrolled_window,
                                   gboolean           has_frame)
{
  BobguiScrolledWindowPrivate *priv = bobgui_scrolled_window_get_instance_private (scrolled_window);

  g_return_if_fail (BOBGUI_IS_SCROLLED_WINDOW (scrolled_window));

  if (priv->has_frame == !!has_frame)
    return;

  priv->has_frame = has_frame;

  if (has_frame)
    bobgui_widget_add_css_class (BOBGUI_WIDGET (scrolled_window), "frame");
  else
    bobgui_widget_remove_css_class (BOBGUI_WIDGET (scrolled_window), "frame");

  g_object_notify_by_pspec (G_OBJECT (scrolled_window), properties[PROP_HAS_FRAME]);
}

/**
 * bobgui_scrolled_window_get_has_frame:
 * @scrolled_window: a `BobguiScrolledWindow`
 *
 * Gets whether the scrolled window draws a frame.
 *
 * Returns: %TRUE if the @scrolled_window has a frame
 */
gboolean
bobgui_scrolled_window_get_has_frame (BobguiScrolledWindow *scrolled_window)
{
  BobguiScrolledWindowPrivate *priv = bobgui_scrolled_window_get_instance_private (scrolled_window);

  g_return_val_if_fail (BOBGUI_IS_SCROLLED_WINDOW (scrolled_window), FALSE);

  return priv->has_frame;
}

/**
 * bobgui_scrolled_window_set_kinetic_scrolling:
 * @scrolled_window: a `BobguiScrolledWindow`
 * @kinetic_scrolling: %TRUE to enable kinetic scrolling
 *
 * Turns kinetic scrolling on or off.
 *
 * Kinetic scrolling only applies to devices with source
 * %GDK_SOURCE_TOUCHSCREEN.
 **/
void
bobgui_scrolled_window_set_kinetic_scrolling (BobguiScrolledWindow *scrolled_window,
                                           gboolean           kinetic_scrolling)
{
  BobguiPropagationPhase phase = BOBGUI_PHASE_NONE;
  BobguiScrolledWindowPrivate *priv = bobgui_scrolled_window_get_instance_private (scrolled_window);

  g_return_if_fail (BOBGUI_IS_SCROLLED_WINDOW (scrolled_window));

  if (priv->kinetic_scrolling == kinetic_scrolling)
    return;

  priv->kinetic_scrolling = kinetic_scrolling;
  bobgui_scrolled_window_check_attach_pan_gesture (scrolled_window);

  if (priv->kinetic_scrolling)
    phase = BOBGUI_PHASE_CAPTURE;
  else
    bobgui_scrolled_window_cancel_deceleration (scrolled_window);

  bobgui_event_controller_set_propagation_phase (BOBGUI_EVENT_CONTROLLER (priv->drag_gesture), phase);
  bobgui_event_controller_set_propagation_phase (BOBGUI_EVENT_CONTROLLER (priv->swipe_gesture), phase);
  bobgui_event_controller_set_propagation_phase (BOBGUI_EVENT_CONTROLLER (priv->long_press_gesture), phase);
  bobgui_event_controller_set_propagation_phase (BOBGUI_EVENT_CONTROLLER (priv->pan_gesture), phase);

  g_object_notify_by_pspec (G_OBJECT (scrolled_window), properties[PROP_KINETIC_SCROLLING]);
}

/**
 * bobgui_scrolled_window_get_kinetic_scrolling:
 * @scrolled_window: a `BobguiScrolledWindow`
 *
 * Returns the specified kinetic scrolling behavior.
 *
 * Returns: the scrolling behavior flags.
 */
gboolean
bobgui_scrolled_window_get_kinetic_scrolling (BobguiScrolledWindow *scrolled_window)
{
  BobguiScrolledWindowPrivate *priv = bobgui_scrolled_window_get_instance_private (scrolled_window);

  g_return_val_if_fail (BOBGUI_IS_SCROLLED_WINDOW (scrolled_window), FALSE);

  return priv->kinetic_scrolling;
}

static void
bobgui_scrolled_window_dispose (GObject *object)
{
  BobguiScrolledWindow *self = BOBGUI_SCROLLED_WINDOW (object);
  BobguiScrolledWindowPrivate *priv = bobgui_scrolled_window_get_instance_private (self);

  g_clear_pointer (&priv->child, bobgui_widget_unparent);

  remove_indicator (self, &priv->hindicator);
  remove_indicator (self, &priv->vindicator);

  if (priv->hscrollbar)
    {
      BobguiAdjustment *hadjustment = bobgui_scrollbar_get_adjustment (BOBGUI_SCROLLBAR (priv->hscrollbar));

      g_signal_handlers_disconnect_by_data (hadjustment, self);
      g_signal_handlers_disconnect_by_data (hadjustment, &priv->hindicator);

      bobgui_widget_unparent (priv->hscrollbar);
      priv->hscrollbar = NULL;
    }

  if (priv->vscrollbar)
    {
      BobguiAdjustment *vadjustment = bobgui_scrollbar_get_adjustment (BOBGUI_SCROLLBAR (priv->vscrollbar));

      g_signal_handlers_disconnect_by_data (vadjustment, self);
      g_signal_handlers_disconnect_by_data (vadjustment, &priv->vindicator);

      bobgui_widget_unparent (priv->vscrollbar);
      priv->vscrollbar = NULL;
    }

  if (priv->deceleration_id)
    {
      bobgui_widget_remove_tick_callback (BOBGUI_WIDGET (self), priv->deceleration_id);
      priv->deceleration_id = 0;
    }

  g_clear_pointer (&priv->hscrolling, bobgui_kinetic_scrolling_free);
  g_clear_pointer (&priv->vscrolling, bobgui_kinetic_scrolling_free);
  g_clear_handle_id (&priv->scroll_events_overshoot_id, g_source_remove);

  G_OBJECT_CLASS (bobgui_scrolled_window_parent_class)->dispose (object);
}

static void
bobgui_scrolled_window_set_property (GObject      *object,
                                  guint         prop_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  BobguiScrolledWindow *scrolled_window = BOBGUI_SCROLLED_WINDOW (object);
  BobguiScrolledWindowPrivate *priv = bobgui_scrolled_window_get_instance_private (scrolled_window);

  switch (prop_id)
    {
    case PROP_HADJUSTMENT:
      bobgui_scrolled_window_set_hadjustment (scrolled_window,
                                           g_value_get_object (value));
      break;
    case PROP_VADJUSTMENT:
      bobgui_scrolled_window_set_vadjustment (scrolled_window,
                                           g_value_get_object (value));
      break;
    case PROP_HSCROLLBAR_POLICY:
      bobgui_scrolled_window_set_policy (scrolled_window,
                                      g_value_get_enum (value),
                                      priv->vscrollbar_policy);
      break;
    case PROP_VSCROLLBAR_POLICY:
      bobgui_scrolled_window_set_policy (scrolled_window,
                                      priv->hscrollbar_policy,
                                      g_value_get_enum (value));
      break;
    case PROP_WINDOW_PLACEMENT:
      bobgui_scrolled_window_set_placement (scrolled_window,
                                         g_value_get_enum (value));
      break;
    case PROP_HAS_FRAME:
      bobgui_scrolled_window_set_has_frame (scrolled_window,
                                         g_value_get_boolean (value));
      break;
    case PROP_MIN_CONTENT_WIDTH:
      bobgui_scrolled_window_set_min_content_width (scrolled_window,
                                                 g_value_get_int (value));
      break;
    case PROP_MIN_CONTENT_HEIGHT:
      bobgui_scrolled_window_set_min_content_height (scrolled_window,
                                                  g_value_get_int (value));
      break;
    case PROP_KINETIC_SCROLLING:
      bobgui_scrolled_window_set_kinetic_scrolling (scrolled_window,
                                                 g_value_get_boolean (value));
      break;
    case PROP_OVERLAY_SCROLLING:
      bobgui_scrolled_window_set_overlay_scrolling (scrolled_window,
                                                 g_value_get_boolean (value));
      break;
    case PROP_MAX_CONTENT_WIDTH:
      bobgui_scrolled_window_set_max_content_width (scrolled_window,
                                                 g_value_get_int (value));
      break;
    case PROP_MAX_CONTENT_HEIGHT:
      bobgui_scrolled_window_set_max_content_height (scrolled_window,
                                                  g_value_get_int (value));
      break;
    case PROP_PROPAGATE_NATURAL_WIDTH:
      bobgui_scrolled_window_set_propagate_natural_width (scrolled_window,
                                                       g_value_get_boolean (value));
      break;
    case PROP_PROPAGATE_NATURAL_HEIGHT:
      bobgui_scrolled_window_set_propagate_natural_height (scrolled_window,
                                                       g_value_get_boolean (value));
      break;
    case PROP_CHILD:
      bobgui_scrolled_window_set_child (scrolled_window, g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_scrolled_window_get_property (GObject    *object,
                                  guint       prop_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  BobguiScrolledWindow *scrolled_window = BOBGUI_SCROLLED_WINDOW (object);
  BobguiScrolledWindowPrivate *priv = bobgui_scrolled_window_get_instance_private (scrolled_window);

  switch (prop_id)
    {
    case PROP_HADJUSTMENT:
      g_value_set_object (value,
                          G_OBJECT (bobgui_scrolled_window_get_hadjustment (scrolled_window)));
      break;
    case PROP_VADJUSTMENT:
      g_value_set_object (value,
                          G_OBJECT (bobgui_scrolled_window_get_vadjustment (scrolled_window)));
      break;
    case PROP_WINDOW_PLACEMENT:
      g_value_set_enum (value, priv->window_placement);
      break;
    case PROP_HAS_FRAME:
      g_value_set_boolean (value, priv->has_frame);
      break;
    case PROP_HSCROLLBAR_POLICY:
      g_value_set_enum (value, priv->hscrollbar_policy);
      break;
    case PROP_VSCROLLBAR_POLICY:
      g_value_set_enum (value, priv->vscrollbar_policy);
      break;
    case PROP_MIN_CONTENT_WIDTH:
      g_value_set_int (value, priv->min_content_width);
      break;
    case PROP_MIN_CONTENT_HEIGHT:
      g_value_set_int (value, priv->min_content_height);
      break;
    case PROP_KINETIC_SCROLLING:
      g_value_set_boolean (value, priv->kinetic_scrolling);
      break;
    case PROP_OVERLAY_SCROLLING:
      g_value_set_boolean (value, priv->overlay_scrolling);
      break;
    case PROP_MAX_CONTENT_WIDTH:
      g_value_set_int (value, priv->max_content_width);
      break;
    case PROP_MAX_CONTENT_HEIGHT:
      g_value_set_int (value, priv->max_content_height);
      break;
    case PROP_PROPAGATE_NATURAL_WIDTH:
      g_value_set_boolean (value, priv->propagate_natural_width);
      break;
    case PROP_PROPAGATE_NATURAL_HEIGHT:
      g_value_set_boolean (value, priv->propagate_natural_height);
      break;
    case PROP_CHILD:
      g_value_set_object (value, bobgui_scrolled_window_get_child (scrolled_window));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_scrolled_window_inner_allocation (BobguiScrolledWindow *scrolled_window,
                                      BobguiAllocation     *rect)
{
  BobguiScrolledWindowPrivate *priv = bobgui_scrolled_window_get_instance_private (scrolled_window);
  BobguiBorder border = { 0 };

  bobgui_scrolled_window_relative_allocation (scrolled_window, rect);
  rect->x = 0;
  rect->y = 0;
  if (priv->child && bobgui_scrollable_get_border (BOBGUI_SCROLLABLE (priv->child), &border))
    {
      rect->x += border.left;
      rect->y += border.top;
      rect->width -= border.left + border.right;
      rect->height -= border.top + border.bottom;
    }
}

static void
bobgui_scrolled_window_snapshot (BobguiWidget   *widget,
                              BobguiSnapshot *snapshot)
{
  BobguiScrolledWindow *scrolled_window = BOBGUI_SCROLLED_WINDOW (widget);
  BobguiScrolledWindowPrivate *priv = bobgui_scrolled_window_get_instance_private (scrolled_window);

  if (priv->hscrollbar_visible &&
      priv->vscrollbar_visible &&
      !priv->use_indicators)
    bobgui_scrolled_window_snapshot_scrollbars_junction (scrolled_window, snapshot);

  BOBGUI_WIDGET_CLASS (bobgui_scrolled_window_parent_class)->snapshot (widget, snapshot);

  bobgui_scrolled_window_snapshot_undershoot (scrolled_window, snapshot);
  bobgui_scrolled_window_snapshot_overshoot (scrolled_window, snapshot);
}

static gboolean
bobgui_scrolled_window_scroll_child (BobguiScrolledWindow *scrolled_window,
                                  BobguiScrollType      scroll,
                                  gboolean           horizontal)
{
  BobguiScrolledWindowPrivate *priv = bobgui_scrolled_window_get_instance_private (scrolled_window);
  BobguiAdjustment *adjustment = NULL;

  switch (scroll)
    {
    case BOBGUI_SCROLL_STEP_UP:
      scroll = BOBGUI_SCROLL_STEP_BACKWARD;
      horizontal = FALSE;
      break;
    case BOBGUI_SCROLL_STEP_DOWN:
      scroll = BOBGUI_SCROLL_STEP_FORWARD;
      horizontal = FALSE;
      break;
    case BOBGUI_SCROLL_STEP_LEFT:
      scroll = BOBGUI_SCROLL_STEP_BACKWARD;
      horizontal = TRUE;
      break;
    case BOBGUI_SCROLL_STEP_RIGHT:
      scroll = BOBGUI_SCROLL_STEP_FORWARD;
      horizontal = TRUE;
      break;
    case BOBGUI_SCROLL_PAGE_UP:
      scroll = BOBGUI_SCROLL_PAGE_BACKWARD;
      horizontal = FALSE;
      break;
    case BOBGUI_SCROLL_PAGE_DOWN:
      scroll = BOBGUI_SCROLL_PAGE_FORWARD;
      horizontal = FALSE;
      break;
    case BOBGUI_SCROLL_PAGE_LEFT:
      scroll = BOBGUI_SCROLL_STEP_BACKWARD;
      horizontal = TRUE;
      break;
    case BOBGUI_SCROLL_PAGE_RIGHT:
      scroll = BOBGUI_SCROLL_STEP_FORWARD;
      horizontal = TRUE;
      break;
    case BOBGUI_SCROLL_STEP_BACKWARD:
    case BOBGUI_SCROLL_STEP_FORWARD:
    case BOBGUI_SCROLL_PAGE_BACKWARD:
    case BOBGUI_SCROLL_PAGE_FORWARD:
    case BOBGUI_SCROLL_START:
    case BOBGUI_SCROLL_END:
      break;
    case BOBGUI_SCROLL_NONE:
    case BOBGUI_SCROLL_JUMP:
    default:
      g_warning ("Invalid scroll type %u for BobguiScrolledWindow::scroll-child", scroll);
      return FALSE;
    }

  if (horizontal)
    {
      if (may_hscroll (scrolled_window))
        adjustment = bobgui_scrollbar_get_adjustment (BOBGUI_SCROLLBAR (priv->hscrollbar));
      else
        return FALSE;
    }
  else
    {
      if (may_vscroll (scrolled_window))
        adjustment = bobgui_scrollbar_get_adjustment (BOBGUI_SCROLLBAR (priv->vscrollbar));
      else
        return FALSE;
    }

  if (adjustment)
    {
      double value = bobgui_adjustment_get_value (adjustment);

      switch (scroll)
        {
        case BOBGUI_SCROLL_STEP_FORWARD:
          value += bobgui_adjustment_get_step_increment (adjustment);
          break;
        case BOBGUI_SCROLL_STEP_BACKWARD:
          value -= bobgui_adjustment_get_step_increment (adjustment);
          break;
        case BOBGUI_SCROLL_PAGE_FORWARD:
          value += bobgui_adjustment_get_page_increment (adjustment);
          break;
        case BOBGUI_SCROLL_PAGE_BACKWARD:
          value -= bobgui_adjustment_get_page_increment (adjustment);
          break;
        case BOBGUI_SCROLL_START:
          value = bobgui_adjustment_get_lower (adjustment);
          break;
        case BOBGUI_SCROLL_END:
          value = bobgui_adjustment_get_upper (adjustment);
          break;
        case BOBGUI_SCROLL_STEP_UP:
        case BOBGUI_SCROLL_STEP_DOWN:
        case BOBGUI_SCROLL_STEP_LEFT:
        case BOBGUI_SCROLL_STEP_RIGHT:
        case BOBGUI_SCROLL_PAGE_UP:
        case BOBGUI_SCROLL_PAGE_DOWN:
        case BOBGUI_SCROLL_PAGE_LEFT:
        case BOBGUI_SCROLL_PAGE_RIGHT:
        case BOBGUI_SCROLL_NONE:
        case BOBGUI_SCROLL_JUMP:
        default:
          g_assert_not_reached ();
          break;
        }

      bobgui_adjustment_animate_to_value (adjustment, value);

      return TRUE;
    }

  return FALSE;
}

static void
bobgui_scrolled_window_move_focus_out (BobguiScrolledWindow *scrolled_window,
                                    BobguiDirectionType   direction_type)
{
  BobguiScrolledWindowPrivate *priv = bobgui_scrolled_window_get_instance_private (scrolled_window);
  BobguiWidget *toplevel;

  /* Focus out of the scrolled window entirely. We do this by setting
   * a flag, then propagating the focus motion to the notebook.
   */
  toplevel = BOBGUI_WIDGET (bobgui_widget_get_root (BOBGUI_WIDGET (scrolled_window)));
  if (!BOBGUI_IS_ROOT (toplevel))
    return;

  g_object_ref (scrolled_window);

  priv->focus_out = TRUE;
  g_signal_emit_by_name (toplevel, "move-focus", direction_type);
  priv->focus_out = FALSE;

  g_object_unref (scrolled_window);
}

static void
bobgui_scrolled_window_relative_allocation (BobguiScrolledWindow *scrolled_window,
                                         BobguiAllocation     *allocation)
{
  BobguiScrolledWindowPrivate *priv = bobgui_scrolled_window_get_instance_private (scrolled_window);
  int sb_width;
  int sb_height;
  int width, height;

  g_return_if_fail (scrolled_window != NULL);
  g_return_if_fail (allocation != NULL);

  /* Get possible scrollbar dimensions */
  bobgui_widget_measure (priv->vscrollbar, BOBGUI_ORIENTATION_HORIZONTAL, -1,
                      &sb_width, NULL, NULL, NULL);
  bobgui_widget_measure (priv->hscrollbar, BOBGUI_ORIENTATION_VERTICAL, -1,
                      &sb_height, NULL, NULL, NULL);

  width = bobgui_widget_get_width (BOBGUI_WIDGET (scrolled_window));
  height = bobgui_widget_get_height (BOBGUI_WIDGET (scrolled_window));

  allocation->x = 0;
  allocation->y = 0;
  allocation->width = width;
  allocation->height = height;

  /* Subtract some things from our available allocation size */
  if (priv->vscrollbar_visible && !priv->use_indicators)
    {
      gboolean is_rtl;

      is_rtl = _bobgui_widget_get_direction (BOBGUI_WIDGET (scrolled_window)) == BOBGUI_TEXT_DIR_RTL;

      if ((!is_rtl &&
           (priv->window_placement == BOBGUI_CORNER_TOP_RIGHT ||
            priv->window_placement == BOBGUI_CORNER_BOTTOM_RIGHT)) ||
          (is_rtl &&
           (priv->window_placement == BOBGUI_CORNER_TOP_LEFT ||
            priv->window_placement == BOBGUI_CORNER_BOTTOM_LEFT)))
        allocation->x += sb_width;

      allocation->width = MAX (1, width - sb_width);
    }

  if (priv->hscrollbar_visible && !priv->use_indicators)
    {

      if (priv->window_placement == BOBGUI_CORNER_BOTTOM_LEFT ||
          priv->window_placement == BOBGUI_CORNER_BOTTOM_RIGHT)
        allocation->y += (sb_height);

      allocation->height = MAX (1, height - sb_height);
    }
}

static gboolean
_bobgui_scrolled_window_get_overshoot (BobguiScrolledWindow *scrolled_window,
                                    int               *overshoot_x,
                                    int               *overshoot_y)
{
  BobguiScrolledWindowPrivate *priv = bobgui_scrolled_window_get_instance_private (scrolled_window);
  BobguiAdjustment *vadjustment, *hadjustment;
  double lower, upper, x, y;

  /* Vertical overshoot */
  vadjustment = bobgui_scrollbar_get_adjustment (BOBGUI_SCROLLBAR (priv->vscrollbar));
  lower = bobgui_adjustment_get_lower (vadjustment);
  upper = bobgui_adjustment_get_upper (vadjustment) -
    bobgui_adjustment_get_page_size (vadjustment);

  if (priv->unclamped_vadj_value < lower)
    y = priv->unclamped_vadj_value - lower;
  else if (priv->unclamped_vadj_value > upper)
    y = priv->unclamped_vadj_value - upper;
  else
    y = 0;

  /* Horizontal overshoot */
  hadjustment = bobgui_scrollbar_get_adjustment (BOBGUI_SCROLLBAR (priv->hscrollbar));
  lower = bobgui_adjustment_get_lower (hadjustment);
  upper = bobgui_adjustment_get_upper (hadjustment) -
    bobgui_adjustment_get_page_size (hadjustment);

  if (priv->unclamped_hadj_value < lower)
    x = priv->unclamped_hadj_value - lower;
  else if (priv->unclamped_hadj_value > upper)
    x = priv->unclamped_hadj_value - upper;
  else
    x = 0;

  if (overshoot_x)
    *overshoot_x = x;

  if (overshoot_y)
    *overshoot_y = y;

  return (x != 0 || y != 0);
}

static void
bobgui_scrolled_window_allocate_child (BobguiScrolledWindow   *swindow,
                                    int                  width,
                                    int                  height)
{
  BobguiScrolledWindowPrivate *priv = bobgui_scrolled_window_get_instance_private (swindow);
  BobguiWidget *widget = BOBGUI_WIDGET (swindow);
  BobguiAllocation child_allocation;
  int sb_width;
  int sb_height;

  child_allocation = (BobguiAllocation) {0, 0, width, height};

  /* Get possible scrollbar dimensions */
  bobgui_widget_measure (priv->vscrollbar, BOBGUI_ORIENTATION_HORIZONTAL, -1,
                      &sb_width, NULL, NULL, NULL);
  bobgui_widget_measure (priv->hscrollbar, BOBGUI_ORIENTATION_VERTICAL, -1,
                      &sb_height, NULL, NULL, NULL);

  /* Subtract some things from our available allocation size */
  if (priv->vscrollbar_visible && !priv->use_indicators)
    {
      gboolean is_rtl;

      is_rtl = _bobgui_widget_get_direction (widget) == BOBGUI_TEXT_DIR_RTL;

      if ((!is_rtl &&
           (priv->window_placement == BOBGUI_CORNER_TOP_RIGHT ||
            priv->window_placement == BOBGUI_CORNER_BOTTOM_RIGHT)) ||
          (is_rtl &&
           (priv->window_placement == BOBGUI_CORNER_TOP_LEFT ||
            priv->window_placement == BOBGUI_CORNER_BOTTOM_LEFT)))
        child_allocation.x += sb_width;

      child_allocation.width = MAX (1, child_allocation.width - sb_width);
    }

  if (priv->hscrollbar_visible && !priv->use_indicators)
    {

      if (priv->window_placement == BOBGUI_CORNER_BOTTOM_LEFT ||
          priv->window_placement == BOBGUI_CORNER_BOTTOM_RIGHT)
        child_allocation.y += (sb_height);

      child_allocation.height = MAX (1, child_allocation.height - sb_height);
    }

  bobgui_widget_size_allocate (priv->child, &child_allocation, -1);
}

static void
bobgui_scrolled_window_allocate_scrollbar (BobguiScrolledWindow *scrolled_window,
                                        BobguiWidget         *scrollbar,
                                        BobguiAllocation     *allocation)
{
  BobguiScrolledWindowPrivate *priv = bobgui_scrolled_window_get_instance_private (scrolled_window);
  BobguiAllocation child_allocation, content_allocation;
  BobguiWidget *widget = BOBGUI_WIDGET (scrolled_window);
  int sb_height, sb_width;
  gboolean is_top, is_start, ltr, is_left;

  bobgui_scrolled_window_inner_allocation (scrolled_window, &content_allocation);
  bobgui_widget_measure (priv->vscrollbar, BOBGUI_ORIENTATION_HORIZONTAL, -1,
                      &sb_width, NULL, NULL, NULL);
  bobgui_widget_measure (priv->hscrollbar, BOBGUI_ORIENTATION_VERTICAL, -1,
                      &sb_height, NULL, NULL, NULL);

  is_top = priv->window_placement == BOBGUI_CORNER_TOP_LEFT ||
           priv->window_placement == BOBGUI_CORNER_TOP_RIGHT;
  is_start = priv->window_placement == BOBGUI_CORNER_TOP_LEFT ||
             priv->window_placement == BOBGUI_CORNER_BOTTOM_LEFT;
  ltr = _bobgui_widget_get_direction (widget) != BOBGUI_TEXT_DIR_RTL;
  is_left = ltr != is_start;

  if (scrollbar == priv->hscrollbar)
    {
      child_allocation.x = content_allocation.x;

      if (is_left &&
          priv->vscrollbar_visible &&
          !priv->use_indicators)
        {
          child_allocation.x += sb_height;
        }

      child_allocation.y = content_allocation.y;

      if (is_top)
        {
          child_allocation.y += content_allocation.height;

          if (priv->use_indicators)
            child_allocation.y -= sb_height;
        }

      child_allocation.width = content_allocation.width;
      child_allocation.height = sb_height;
    }
  else
    {
      g_assert (scrollbar == priv->vscrollbar);

      child_allocation.x = content_allocation.x;

      if (!is_left)
        {
          child_allocation.x += content_allocation.width;

          if (priv->use_indicators)
            child_allocation.x -= sb_width;
        }

      child_allocation.y = content_allocation.y;

      if (!is_top &&
          priv->hscrollbar_visible &&
          !priv->use_indicators)
        {
          child_allocation.y += sb_width;
        }

      child_allocation.width = sb_width;
      child_allocation.height = content_allocation.height;
    }

  *allocation = child_allocation;
}

static void
_bobgui_scrolled_window_set_adjustment_value (BobguiScrolledWindow *scrolled_window,
                                           BobguiAdjustment     *adjustment,
                                           double             value)
{
  BobguiScrolledWindowPrivate *priv = bobgui_scrolled_window_get_instance_private (scrolled_window);
  double lower, upper, *prev_value;
  BobguiPositionType edge_pos;
  gboolean vertical;

  lower = bobgui_adjustment_get_lower (adjustment) - MAX_OVERSHOOT_DISTANCE;
  upper = bobgui_adjustment_get_upper (adjustment) -
    bobgui_adjustment_get_page_size (adjustment) + MAX_OVERSHOOT_DISTANCE;

  if (adjustment == bobgui_scrollbar_get_adjustment (BOBGUI_SCROLLBAR (priv->hscrollbar)))
    vertical = FALSE;
  else if (adjustment == bobgui_scrollbar_get_adjustment (BOBGUI_SCROLLBAR (priv->vscrollbar)))
    vertical = TRUE;
  else
    return;

  if (vertical)
    prev_value = &priv->unclamped_vadj_value;
  else
    prev_value = &priv->unclamped_hadj_value;

  value = CLAMP (value, lower, upper);

  if (*prev_value == value)
    return;

  *prev_value = value;
  bobgui_adjustment_set_value (adjustment, value);

  if (value == lower)
    edge_pos = vertical ? BOBGUI_POS_TOP : BOBGUI_POS_LEFT;
  else if (value == upper)
    edge_pos = vertical ? BOBGUI_POS_BOTTOM : BOBGUI_POS_RIGHT;
  else
    return;

  /* Invert horizontal edge position on RTL */
  if (!vertical &&
      _bobgui_widget_get_direction (BOBGUI_WIDGET (scrolled_window)) == BOBGUI_TEXT_DIR_RTL)
    edge_pos = (edge_pos == BOBGUI_POS_LEFT) ? BOBGUI_POS_RIGHT : BOBGUI_POS_LEFT;

  g_signal_emit (scrolled_window, signals[EDGE_OVERSHOT], 0, edge_pos);
}

static gboolean
scrolled_window_deceleration_cb (BobguiWidget         *widget,
                                 GdkFrameClock     *frame_clock,
                                 gpointer           user_data)
{
  BobguiScrolledWindow *scrolled_window = user_data;
  BobguiScrolledWindowPrivate *priv = bobgui_scrolled_window_get_instance_private (scrolled_window);
  BobguiAdjustment *hadjustment, *vadjustment;
  gint64 current_time;
  double position;
  gboolean retval = G_SOURCE_REMOVE;

  current_time = gdk_frame_clock_get_frame_time (frame_clock);
  priv->last_deceleration_time = current_time;

  hadjustment = bobgui_scrollbar_get_adjustment (BOBGUI_SCROLLBAR (priv->hscrollbar));
  vadjustment = bobgui_scrollbar_get_adjustment (BOBGUI_SCROLLBAR (priv->vscrollbar));

  bobgui_scrolled_window_invalidate_overshoot (scrolled_window);

  if (priv->hscrolling &&
      bobgui_kinetic_scrolling_tick (priv->hscrolling, current_time, &position, NULL))
    {
      priv->unclamped_hadj_value = position;
      bobgui_adjustment_set_value (hadjustment, position);
      retval = G_SOURCE_CONTINUE;
    }

  if (priv->vscrolling &&
      bobgui_kinetic_scrolling_tick (priv->vscrolling, current_time, &position, NULL))
    {
      priv->unclamped_vadj_value = position;
      bobgui_adjustment_set_value (vadjustment, position);
      retval = G_SOURCE_CONTINUE;
    }

  if (retval == G_SOURCE_REMOVE)
    bobgui_scrolled_window_cancel_deceleration (scrolled_window);
  else
    bobgui_scrolled_window_invalidate_overshoot (scrolled_window);

  return retval;
}

static void
bobgui_scrolled_window_cancel_deceleration (BobguiScrolledWindow *scrolled_window)
{
  BobguiScrolledWindowPrivate *priv = bobgui_scrolled_window_get_instance_private (scrolled_window);

  if (priv->deceleration_id)
    {
      bobgui_widget_remove_tick_callback (BOBGUI_WIDGET (scrolled_window),
                                       priv->deceleration_id);
      priv->deceleration_id = 0;
    }
}

static void
kinetic_scroll_stop_notify (BobguiScrolledWindow *scrolled_window)
{
  BobguiScrolledWindowPrivate *priv = bobgui_scrolled_window_get_instance_private (scrolled_window);
  priv->deceleration_id = 0;
}

static void
bobgui_scrolled_window_accumulate_velocity (BobguiKineticScrolling **scrolling,
                                         gint64                current_time,
                                         double               *velocity)
{
    if (!*scrolling)
      return;

    double last_velocity;
    bobgui_kinetic_scrolling_tick (*scrolling, current_time, NULL, &last_velocity);
    if (((*velocity >= 0) == (last_velocity >= 0)) &&
        (fabs (*velocity) >= fabs (last_velocity) * VELOCITY_ACCUMULATION_FLOOR))
      {
        double min_velocity = last_velocity * VELOCITY_ACCUMULATION_FLOOR;
        double max_velocity = last_velocity * VELOCITY_ACCUMULATION_CEIL;
        double accumulation_multiplier = (*velocity - min_velocity) / (max_velocity - min_velocity);
        *velocity += last_velocity * fmin (accumulation_multiplier, VELOCITY_ACCUMULATION_MAX);
      }
    g_clear_pointer (scrolling, bobgui_kinetic_scrolling_free);
}

static void
bobgui_scrolled_window_start_deceleration (BobguiScrolledWindow *scrolled_window)
{
  BobguiScrolledWindowPrivate *priv = bobgui_scrolled_window_get_instance_private (scrolled_window);
  GdkFrameClock *frame_clock;
  gint64 current_time;
  int overshoot_x, overshoot_y;

  g_return_if_fail (priv->deceleration_id == 0);

  frame_clock = bobgui_widget_get_frame_clock (BOBGUI_WIDGET (scrolled_window));

  current_time = gdk_frame_clock_get_frame_time (frame_clock);
  priv->last_deceleration_time = current_time;

  _bobgui_scrolled_window_get_overshoot (scrolled_window, &overshoot_x, &overshoot_y);

  if (may_hscroll (scrolled_window))
    {
      double lower,upper;
      BobguiAdjustment *hadjustment;

      bobgui_scrolled_window_accumulate_velocity (&priv->hscrolling, current_time, &priv->x_velocity);
      g_clear_pointer (&priv->hscrolling, bobgui_kinetic_scrolling_free);

      if (priv->x_velocity != 0 || overshoot_x != 0)
        {
          hadjustment = bobgui_scrollbar_get_adjustment (BOBGUI_SCROLLBAR (priv->hscrollbar));
          lower = bobgui_adjustment_get_lower (hadjustment);
          upper = bobgui_adjustment_get_upper (hadjustment);
          upper -= bobgui_adjustment_get_page_size (hadjustment);
          priv->hscrolling =
            bobgui_kinetic_scrolling_new (current_time,
                                       lower,
                                       upper,
                                       MAX_OVERSHOOT_DISTANCE,
                                       DECELERATION_FRICTION,
                                       OVERSHOOT_FRICTION,
                                       priv->unclamped_hadj_value,
                                       priv->x_velocity);
        }
    }
  else
    g_clear_pointer (&priv->hscrolling, bobgui_kinetic_scrolling_free);

  if (may_vscroll (scrolled_window))
    {
      double lower,upper;
      BobguiAdjustment *vadjustment;

      bobgui_scrolled_window_accumulate_velocity (&priv->vscrolling, current_time, &priv->y_velocity);
      g_clear_pointer (&priv->vscrolling, bobgui_kinetic_scrolling_free);

      if (priv->y_velocity != 0 || overshoot_y != 0)
        {
          vadjustment = bobgui_scrollbar_get_adjustment (BOBGUI_SCROLLBAR (priv->vscrollbar));
          lower = bobgui_adjustment_get_lower(vadjustment);
          upper = bobgui_adjustment_get_upper(vadjustment);
          upper -= bobgui_adjustment_get_page_size(vadjustment);
          priv->vscrolling =
            bobgui_kinetic_scrolling_new (current_time,
                                       lower,
                                       upper,
                                       MAX_OVERSHOOT_DISTANCE,
                                       DECELERATION_FRICTION,
                                       OVERSHOOT_FRICTION,
                                       priv->unclamped_vadj_value,
                                       priv->y_velocity);
        }
    }
  else
    g_clear_pointer (&priv->vscrolling, bobgui_kinetic_scrolling_free);

  priv->deceleration_id = bobgui_widget_add_tick_callback (BOBGUI_WIDGET (scrolled_window),
                                                        scrolled_window_deceleration_cb, scrolled_window,
                                                        (GDestroyNotify) kinetic_scroll_stop_notify);
}

static gboolean
bobgui_scrolled_window_focus (BobguiWidget        *widget,
                           BobguiDirectionType  direction)
{
  BobguiScrolledWindow *scrolled_window = BOBGUI_SCROLLED_WINDOW (widget);
  BobguiScrolledWindowPrivate *priv = bobgui_scrolled_window_get_instance_private (scrolled_window);
  gboolean had_focus_child;

  had_focus_child = bobgui_widget_get_focus_child (widget) != NULL;

  if (priv->focus_out)
    {
      priv->focus_out = FALSE; /* Clear this to catch the wrap-around case */
      return FALSE;
    }

  if (bobgui_widget_is_focus (widget))
    return FALSE;

  /* We only put the scrolled window itself in the focus chain if it
   * isn't possible to focus any children.
   */
  if (priv->child)
    {
      if (bobgui_widget_child_focus (priv->child, direction))
        return TRUE;
    }

  if (!had_focus_child && bobgui_widget_get_can_focus (widget))
    {
      bobgui_widget_grab_focus (widget);
      return TRUE;
    }
  else
    return FALSE;
}

static void
bobgui_scrolled_window_adjustment_changed (BobguiAdjustment *adjustment,
                                        gpointer       data)
{
  BobguiScrolledWindow *scrolled_window = data;
  BobguiScrolledWindowPrivate *priv = bobgui_scrolled_window_get_instance_private (scrolled_window);

  if (adjustment == bobgui_scrollbar_get_adjustment (BOBGUI_SCROLLBAR (priv->hscrollbar)))
    {
      if (priv->hscrollbar_policy == BOBGUI_POLICY_AUTOMATIC)
        {
          gboolean visible;

          visible = priv->hscrollbar_visible;
          bobgui_scrolled_window_update_scrollbar_visibility_flags (scrolled_window, priv->hscrollbar);

          if (priv->hscrollbar_visible != visible)
            bobgui_widget_queue_resize (BOBGUI_WIDGET (scrolled_window));

          if (priv->hscrolling)
            {
              BobguiKineticScrollingChange change;
              double lower = bobgui_adjustment_get_lower (adjustment);
              double upper = bobgui_adjustment_get_upper (adjustment);
              upper -= bobgui_adjustment_get_page_size (adjustment);

              change = bobgui_kinetic_scrolling_update_size (priv->hscrolling, lower, upper);

              if ((change & BOBGUI_KINETIC_SCROLLING_CHANGE_IN_OVERSHOOT) &&
                  (change & (BOBGUI_KINETIC_SCROLLING_CHANGE_UPPER | BOBGUI_KINETIC_SCROLLING_CHANGE_LOWER)))
                {
                  g_clear_pointer (&priv->hscrolling, bobgui_kinetic_scrolling_free);
                  priv->unclamped_hadj_value = bobgui_adjustment_get_value (adjustment);
                  bobgui_scrolled_window_invalidate_overshoot (scrolled_window);
                }
            }
        }
    }
  else if (adjustment == bobgui_scrollbar_get_adjustment (BOBGUI_SCROLLBAR (priv->vscrollbar)))
    {
      if (priv->vscrollbar_policy == BOBGUI_POLICY_AUTOMATIC)
        {
          gboolean visible;

          visible = priv->vscrollbar_visible;
          bobgui_scrolled_window_update_scrollbar_visibility_flags (scrolled_window, priv->vscrollbar);

          if (priv->vscrollbar_visible != visible)
            bobgui_widget_queue_resize (BOBGUI_WIDGET (scrolled_window));

          if (priv->vscrolling)
            {
              BobguiKineticScrollingChange change;
              double lower = bobgui_adjustment_get_lower (adjustment);
              double upper = bobgui_adjustment_get_upper (adjustment);
              upper -= bobgui_adjustment_get_page_size (adjustment);

              change = bobgui_kinetic_scrolling_update_size (priv->vscrolling, lower, upper);

              if ((change & BOBGUI_KINETIC_SCROLLING_CHANGE_IN_OVERSHOOT) &&
                  (change & (BOBGUI_KINETIC_SCROLLING_CHANGE_UPPER | BOBGUI_KINETIC_SCROLLING_CHANGE_LOWER)))
                {
                  g_clear_pointer (&priv->vscrolling, bobgui_kinetic_scrolling_free);
                  priv->unclamped_vadj_value = bobgui_adjustment_get_value (adjustment);
                  bobgui_scrolled_window_invalidate_overshoot (scrolled_window);
                }
            }
        }
    }

  if (!priv->hscrolling && !priv->vscrolling)
    bobgui_scrolled_window_cancel_deceleration (scrolled_window);
}

static void
maybe_emit_edge_reached (BobguiScrolledWindow *scrolled_window,
                         BobguiAdjustment *adjustment)
{
  BobguiScrolledWindowPrivate *priv = bobgui_scrolled_window_get_instance_private (scrolled_window);
  double value, lower, upper, page_size;
  BobguiPositionType edge_pos;
  gboolean vertical;

  if (adjustment == bobgui_scrollbar_get_adjustment (BOBGUI_SCROLLBAR (priv->hscrollbar)))
    vertical = FALSE;
  else if (adjustment == bobgui_scrollbar_get_adjustment (BOBGUI_SCROLLBAR (priv->vscrollbar)))
    vertical = TRUE;
  else
    return;

  value = bobgui_adjustment_get_value (adjustment);
  lower = bobgui_adjustment_get_lower (adjustment);
  upper = bobgui_adjustment_get_upper (adjustment);
  page_size = bobgui_adjustment_get_page_size (adjustment);

  if (value == lower)
    edge_pos = vertical ? BOBGUI_POS_TOP: BOBGUI_POS_LEFT;
  else if (value == upper - page_size)
    edge_pos = vertical ? BOBGUI_POS_BOTTOM : BOBGUI_POS_RIGHT;
  else
    return;

  if (!vertical &&
      _bobgui_widget_get_direction (BOBGUI_WIDGET (scrolled_window)) == BOBGUI_TEXT_DIR_RTL)
    edge_pos = (edge_pos == BOBGUI_POS_LEFT) ? BOBGUI_POS_RIGHT : BOBGUI_POS_LEFT;

  g_signal_emit (scrolled_window, signals[EDGE_REACHED], 0, edge_pos);
}

static void
bobgui_scrolled_window_adjustment_value_changed (BobguiAdjustment *adjustment,
                                              gpointer       user_data)
{
  BobguiScrolledWindow *scrolled_window = user_data;
  BobguiScrolledWindowPrivate *priv = bobgui_scrolled_window_get_instance_private (scrolled_window);

  maybe_emit_edge_reached (scrolled_window, adjustment);

  /* Allow overshooting for kinetic scrolling operations */
  if (priv->deceleration_id)
    return;

  /* Ensure BobguiAdjustment and unclamped values are in sync */
  if (adjustment == bobgui_scrollbar_get_adjustment (BOBGUI_SCROLLBAR (priv->hscrollbar)))
    priv->unclamped_hadj_value = bobgui_adjustment_get_value (adjustment);
  else if (adjustment == bobgui_scrollbar_get_adjustment (BOBGUI_SCROLLBAR (priv->vscrollbar)))
    priv->unclamped_vadj_value = bobgui_adjustment_get_value (adjustment);
}

static gboolean
bobgui_widget_should_animate (BobguiWidget *widget)
{
  if (!bobgui_widget_get_mapped (widget))
    return FALSE;

  return bobgui_settings_get_enable_animations (bobgui_widget_get_settings (widget));
}

static void
bobgui_scrolled_window_update_animating (BobguiScrolledWindow *sw)
{
  BobguiScrolledWindowPrivate *priv = bobgui_scrolled_window_get_instance_private (sw);
  BobguiAdjustment *adjustment;
  GdkFrameClock *clock = NULL;
  guint duration = 0;

  if (bobgui_widget_should_animate (BOBGUI_WIDGET (sw)))
    {
      clock = bobgui_widget_get_frame_clock (BOBGUI_WIDGET (sw)),
      duration = ANIMATION_DURATION;
    }

  adjustment = bobgui_scrollbar_get_adjustment (BOBGUI_SCROLLBAR (priv->hscrollbar));
  bobgui_adjustment_enable_animation (adjustment, clock, duration);

  adjustment = bobgui_scrollbar_get_adjustment (BOBGUI_SCROLLBAR (priv->vscrollbar));
  bobgui_adjustment_enable_animation (adjustment, clock, duration);
}

static void
bobgui_scrolled_window_map (BobguiWidget *widget)
{
  BobguiScrolledWindow *scrolled_window = BOBGUI_SCROLLED_WINDOW (widget);

  BOBGUI_WIDGET_CLASS (bobgui_scrolled_window_parent_class)->map (widget);

  bobgui_scrolled_window_update_animating (scrolled_window);
  bobgui_scrolled_window_update_use_indicators (scrolled_window);
}

static void
indicator_reset (Indicator *indicator)
{
  g_clear_handle_id (&indicator->conceil_timer, g_source_remove);
  g_clear_handle_id (&indicator->over_timeout_id, g_source_remove);

  if (indicator->scrollbar && indicator->tick_id)
    {
      bobgui_widget_remove_tick_callback (indicator->scrollbar,
                                       indicator->tick_id);
      indicator->tick_id = 0;
    }

  indicator->over = FALSE;
  bobgui_progress_tracker_finish (&indicator->tracker);
  indicator->current_pos = indicator->source_pos = indicator->target_pos = 0;
  indicator->last_scroll_time = 0;
}

static void
bobgui_scrolled_window_unmap (BobguiWidget *widget)
{
  BobguiScrolledWindow *scrolled_window = BOBGUI_SCROLLED_WINDOW (widget);
  BobguiScrolledWindowPrivate *priv = bobgui_scrolled_window_get_instance_private (scrolled_window);

  BOBGUI_WIDGET_CLASS (bobgui_scrolled_window_parent_class)->unmap (widget);

  bobgui_scrolled_window_update_animating (scrolled_window);

  indicator_reset (&priv->hindicator);
  indicator_reset (&priv->vindicator);
}

static void
indicator_set_fade (Indicator *indicator,
                    double     pos)
{
  gboolean visible, changed;

  changed = indicator->current_pos != pos;
  indicator->current_pos = pos;

  visible = indicator->current_pos != 0.0 || indicator->target_pos != 0.0;

  if (visible && indicator->conceil_timer == 0)
    {
      indicator->conceil_timer = g_timeout_add (INDICATOR_FADE_OUT_TIME, maybe_hide_indicator, indicator);
      gdk_source_set_static_name_by_id (indicator->conceil_timer, "[bobgui] maybe_hide_indicator");
    }
  if (!visible && indicator->conceil_timer != 0)
    {
      g_source_remove (indicator->conceil_timer);
      indicator->conceil_timer = 0;
    }

  if (changed)
    {
      bobgui_widget_set_opacity (indicator->scrollbar, indicator->current_pos);
    }
}

static gboolean
indicator_fade_cb (BobguiWidget     *widget,
                   GdkFrameClock *frame_clock,
                   gpointer       user_data)
{
  Indicator *indicator = user_data;
  double t;

  bobgui_progress_tracker_advance_frame (&indicator->tracker,
                                      gdk_frame_clock_get_frame_time (frame_clock));
  t = bobgui_progress_tracker_get_ease_out_cubic (&indicator->tracker, FALSE);

  indicator_set_fade (indicator,
                      indicator->source_pos + (t * (indicator->target_pos - indicator->source_pos)));

  if (bobgui_progress_tracker_get_state (&indicator->tracker) == BOBGUI_PROGRESS_STATE_AFTER)
    {
      indicator->tick_id = 0;
      return FALSE;
    }

  return TRUE;
}

static void
indicator_start_fade (Indicator *indicator,
                      double     target)
{
  if (indicator->target_pos == target)
    return;

  indicator->target_pos = target;

  if (target != 0.0)
    indicator->last_scroll_time = g_get_monotonic_time ();

  if (bobgui_widget_should_animate (indicator->scrollbar))
    {
      indicator->source_pos = indicator->current_pos;
      bobgui_progress_tracker_start (&indicator->tracker, INDICATOR_FADE_OUT_DURATION * 1000, 0, 1.0);
      if (indicator->tick_id == 0)
        indicator->tick_id = bobgui_widget_add_tick_callback (indicator->scrollbar, indicator_fade_cb, indicator, NULL);
    }
  else
    indicator_set_fade (indicator, target);
}

static gboolean
maybe_hide_indicator (gpointer data)
{
  Indicator *indicator = data;

  if (g_get_monotonic_time () - indicator->last_scroll_time >= INDICATOR_FADE_OUT_DELAY * 1000 &&
      !indicator->over)
    indicator_start_fade (indicator, 0.0);

  return G_SOURCE_CONTINUE;
}

static void
indicator_value_changed (BobguiAdjustment *adjustment,
                         Indicator     *indicator)
{
  indicator->last_scroll_time = g_get_monotonic_time ();
  indicator_start_fade (indicator, 1.0);
}

static void
setup_indicator (BobguiScrolledWindow *scrolled_window,
                 Indicator         *indicator,
                 BobguiWidget         *scrollbar)
{
  BobguiAdjustment *adjustment;

  if (scrollbar == NULL)
    return;

  adjustment = bobgui_scrollbar_get_adjustment (BOBGUI_SCROLLBAR (scrollbar));

  indicator->scrollbar = scrollbar;

  bobgui_widget_add_css_class (scrollbar, "overlay-indicator");
  g_signal_connect (adjustment, "value-changed",
                    G_CALLBACK (indicator_value_changed), indicator);

  bobgui_widget_set_opacity (scrollbar, 0.0);
  indicator->current_pos = 0.0;
}

static void
remove_indicator (BobguiScrolledWindow *scrolled_window,
                  Indicator         *indicator)
{
  BobguiWidget *scrollbar;
  BobguiAdjustment *adjustment;

  if (indicator->scrollbar == NULL)
    return;

  scrollbar = indicator->scrollbar;
  indicator->scrollbar = NULL;

  bobgui_widget_remove_css_class (scrollbar, "overlay-indicator");

  adjustment = bobgui_scrollbar_get_adjustment (BOBGUI_SCROLLBAR (scrollbar));
  g_signal_handlers_disconnect_by_data (adjustment, indicator);

  if (indicator->conceil_timer)
    {
      g_source_remove (indicator->conceil_timer);
      indicator->conceil_timer = 0;
    }

  if (indicator->over_timeout_id)
    {
      g_source_remove (indicator->over_timeout_id);
      indicator->over_timeout_id = 0;
    }

  if (indicator->tick_id)
    {
      bobgui_widget_remove_tick_callback (scrollbar, indicator->tick_id);
      indicator->tick_id = 0;
    }

  bobgui_widget_set_opacity (scrollbar, 1.0);
  indicator->current_pos = 1.0;
}

static void
bobgui_scrolled_window_sync_use_indicators (BobguiScrolledWindow *scrolled_window)
{
  BobguiScrolledWindowPrivate *priv = bobgui_scrolled_window_get_instance_private (scrolled_window);

  if (priv->use_indicators)
    {
      setup_indicator (scrolled_window, &priv->hindicator, priv->hscrollbar);
      setup_indicator (scrolled_window, &priv->vindicator, priv->vscrollbar);
    }
  else
    {
      remove_indicator (scrolled_window, &priv->hindicator);
      remove_indicator (scrolled_window, &priv->vindicator);
    }
}

static void
bobgui_scrolled_window_update_use_indicators (BobguiScrolledWindow *scrolled_window)
{
  BobguiScrolledWindowPrivate *priv = bobgui_scrolled_window_get_instance_private (scrolled_window);
  gboolean use_indicators;
  BobguiSettings *settings = bobgui_widget_get_settings (BOBGUI_WIDGET (scrolled_window));
  gboolean overlay_scrolling;

  g_object_get (settings, "bobgui-overlay-scrolling", &overlay_scrolling, NULL);

  use_indicators = overlay_scrolling && priv->overlay_scrolling;

  if (priv->use_indicators != use_indicators)
    {
      priv->use_indicators = use_indicators;

      if (bobgui_widget_get_realized (BOBGUI_WIDGET (scrolled_window)))
        bobgui_scrolled_window_sync_use_indicators (scrolled_window);

      bobgui_widget_queue_resize (BOBGUI_WIDGET (scrolled_window));
    }
}

static void
bobgui_scrolled_window_realize (BobguiWidget *widget)
{
  BobguiScrolledWindow *scrolled_window = BOBGUI_SCROLLED_WINDOW (widget);
  BobguiScrolledWindowPrivate *priv = bobgui_scrolled_window_get_instance_private (scrolled_window);
  BobguiSettings *settings;

  priv->hindicator.scrollbar = priv->hscrollbar;
  priv->vindicator.scrollbar = priv->vscrollbar;

  bobgui_scrolled_window_sync_use_indicators (scrolled_window);

  settings = bobgui_widget_get_settings (widget);
  g_signal_connect_swapped (settings, "notify::bobgui-overlay-scrolling",
                            G_CALLBACK (bobgui_scrolled_window_update_use_indicators), widget);

  BOBGUI_WIDGET_CLASS (bobgui_scrolled_window_parent_class)->realize (widget);
}

static void
bobgui_scrolled_window_unrealize (BobguiWidget *widget)
{
  BobguiSettings *settings;

  settings = bobgui_widget_get_settings (widget);

  g_signal_handlers_disconnect_by_func (settings, bobgui_scrolled_window_update_use_indicators, widget);

  BOBGUI_WIDGET_CLASS (bobgui_scrolled_window_parent_class)->unrealize (widget);
}

/**
 * bobgui_scrolled_window_get_min_content_width:
 * @scrolled_window: a `BobguiScrolledWindow`
 *
 * Gets the minimum content width of @scrolled_window.
 *
 * Returns: the minimum content width
 */
int
bobgui_scrolled_window_get_min_content_width (BobguiScrolledWindow *scrolled_window)
{
  BobguiScrolledWindowPrivate *priv = bobgui_scrolled_window_get_instance_private (scrolled_window);

  g_return_val_if_fail (BOBGUI_IS_SCROLLED_WINDOW (scrolled_window), 0);

  return priv->min_content_width;
}

/**
 * bobgui_scrolled_window_set_min_content_width:
 * @scrolled_window: a `BobguiScrolledWindow`
 * @width: the minimal content width
 *
 * Sets the minimum width that @scrolled_window should keep visible.
 *
 * Note that this can and (usually will) be smaller than the minimum
 * size of the content.
 *
 * It is a programming error to set the minimum content width to a
 * value greater than [property@Bobgui.ScrolledWindow:max-content-width].
 */
void
bobgui_scrolled_window_set_min_content_width (BobguiScrolledWindow *scrolled_window,
                                           int                width)
{
  BobguiScrolledWindowPrivate *priv = bobgui_scrolled_window_get_instance_private (scrolled_window);

  g_return_if_fail (BOBGUI_IS_SCROLLED_WINDOW (scrolled_window));

  g_return_if_fail (width == -1 || priv->max_content_width == -1 || width <= priv->max_content_width);

  if (priv->min_content_width != width)
    {
      priv->min_content_width = width;

      bobgui_widget_queue_resize (BOBGUI_WIDGET (scrolled_window));

      g_object_notify_by_pspec (G_OBJECT (scrolled_window), properties[PROP_MIN_CONTENT_WIDTH]);
    }
}

/**
 * bobgui_scrolled_window_get_min_content_height:
 * @scrolled_window: a `BobguiScrolledWindow`
 *
 * Gets the minimal content height of @scrolled_window.
 *
 * Returns: the minimal content height
 */
int
bobgui_scrolled_window_get_min_content_height (BobguiScrolledWindow *scrolled_window)
{
  BobguiScrolledWindowPrivate *priv = bobgui_scrolled_window_get_instance_private (scrolled_window);

  g_return_val_if_fail (BOBGUI_IS_SCROLLED_WINDOW (scrolled_window), 0);

  return priv->min_content_height;
}

/**
 * bobgui_scrolled_window_set_min_content_height:
 * @scrolled_window: a `BobguiScrolledWindow`
 * @height: the minimal content height
 *
 * Sets the minimum height that @scrolled_window should keep visible.
 *
 * Note that this can and (usually will) be smaller than the minimum
 * size of the content.
 *
 * It is a programming error to set the minimum content height to a
 * value greater than [property@Bobgui.ScrolledWindow:max-content-height].
 */
void
bobgui_scrolled_window_set_min_content_height (BobguiScrolledWindow *scrolled_window,
                                            int                height)
{
  BobguiScrolledWindowPrivate *priv = bobgui_scrolled_window_get_instance_private (scrolled_window);

  g_return_if_fail (BOBGUI_IS_SCROLLED_WINDOW (scrolled_window));

  g_return_if_fail (height == -1 || priv->max_content_height == -1 || height <= priv->max_content_height);

  if (priv->min_content_height != height)
    {
      priv->min_content_height = height;

      bobgui_widget_queue_resize (BOBGUI_WIDGET (scrolled_window));

      g_object_notify_by_pspec (G_OBJECT (scrolled_window), properties[PROP_MIN_CONTENT_HEIGHT]);
    }
}

/**
 * bobgui_scrolled_window_set_overlay_scrolling:
 * @scrolled_window: a `BobguiScrolledWindow`
 * @overlay_scrolling: whether to enable overlay scrolling
 *
 * Enables or disables overlay scrolling for this scrolled window.
 */
void
bobgui_scrolled_window_set_overlay_scrolling (BobguiScrolledWindow *scrolled_window,
                                           gboolean           overlay_scrolling)
{
  BobguiScrolledWindowPrivate *priv = bobgui_scrolled_window_get_instance_private (scrolled_window);

  g_return_if_fail (BOBGUI_IS_SCROLLED_WINDOW (scrolled_window));

  if (priv->overlay_scrolling != overlay_scrolling)
    {
      priv->overlay_scrolling = overlay_scrolling;

      bobgui_scrolled_window_update_use_indicators (scrolled_window);

      g_object_notify_by_pspec (G_OBJECT (scrolled_window), properties[PROP_OVERLAY_SCROLLING]);
    }
}

/**
 * bobgui_scrolled_window_get_overlay_scrolling:
 * @scrolled_window: a `BobguiScrolledWindow`
 *
 * Returns whether overlay scrolling is enabled for this scrolled window.
 *
 * Returns: %TRUE if overlay scrolling is enabled
 */
gboolean
bobgui_scrolled_window_get_overlay_scrolling (BobguiScrolledWindow *scrolled_window)
{
  BobguiScrolledWindowPrivate *priv = bobgui_scrolled_window_get_instance_private (scrolled_window);

  g_return_val_if_fail (BOBGUI_IS_SCROLLED_WINDOW (scrolled_window), TRUE);

  return priv->overlay_scrolling;
}

/**
 * bobgui_scrolled_window_set_max_content_width:
 * @scrolled_window: a `BobguiScrolledWindow`
 * @width: the maximum content width
 *
 * Sets the maximum width that @scrolled_window should keep visible.
 *
 * The @scrolled_window will grow up to this width before it starts
 * scrolling the content.
 *
 * It is a programming error to set the maximum content width to a
 * value smaller than [property@Bobgui.ScrolledWindow:min-content-width].
 */
void
bobgui_scrolled_window_set_max_content_width (BobguiScrolledWindow *scrolled_window,
                                           int                width)
{
  BobguiScrolledWindowPrivate *priv = bobgui_scrolled_window_get_instance_private (scrolled_window);

  g_return_if_fail (BOBGUI_IS_SCROLLED_WINDOW (scrolled_window));

  g_return_if_fail (width == -1 || priv->min_content_width == -1 || width >= priv->min_content_width);

  if (width != priv->max_content_width)
    {
      priv->max_content_width = width;
      g_object_notify_by_pspec (G_OBJECT (scrolled_window), properties [PROP_MAX_CONTENT_WIDTH]);
      bobgui_widget_queue_resize (BOBGUI_WIDGET (scrolled_window));
    }
}

/**
 * bobgui_scrolled_window_get_max_content_width:
 * @scrolled_window: a `BobguiScrolledWindow`
 *
 * Returns the maximum content width set.
 *
 * Returns: the maximum content width, or -1
 */
int
bobgui_scrolled_window_get_max_content_width (BobguiScrolledWindow *scrolled_window)
{
  BobguiScrolledWindowPrivate *priv = bobgui_scrolled_window_get_instance_private (scrolled_window);

  g_return_val_if_fail (BOBGUI_IS_SCROLLED_WINDOW (scrolled_window), -1);

  return priv->max_content_width;
}

/**
 * bobgui_scrolled_window_set_max_content_height:
 * @scrolled_window: a `BobguiScrolledWindow`
 * @height: the maximum content height
 *
 * Sets the maximum height that @scrolled_window should keep visible.
 *
 * The @scrolled_window will grow up to this height before it starts
 * scrolling the content.
 *
 * It is a programming error to set the maximum content height to a value
 * smaller than [property@Bobgui.ScrolledWindow:min-content-height].
 */
void
bobgui_scrolled_window_set_max_content_height (BobguiScrolledWindow *scrolled_window,
                                            int                height)
{
  BobguiScrolledWindowPrivate *priv = bobgui_scrolled_window_get_instance_private (scrolled_window);

  g_return_if_fail (BOBGUI_IS_SCROLLED_WINDOW (scrolled_window));

  g_return_if_fail (height == -1 || priv->min_content_height == -1 || height >= priv->min_content_height);

  if (height != priv->max_content_height)
    {
      priv->max_content_height = height;
      g_object_notify_by_pspec (G_OBJECT (scrolled_window), properties [PROP_MAX_CONTENT_HEIGHT]);
      bobgui_widget_queue_resize (BOBGUI_WIDGET (scrolled_window));
    }
}

/**
 * bobgui_scrolled_window_get_max_content_height:
 * @scrolled_window: a `BobguiScrolledWindow`
 *
 * Returns the maximum content height set.
 *
 * Returns: the maximum content height, or -1
 */
int
bobgui_scrolled_window_get_max_content_height (BobguiScrolledWindow *scrolled_window)
{
  BobguiScrolledWindowPrivate *priv = bobgui_scrolled_window_get_instance_private (scrolled_window);

  g_return_val_if_fail (BOBGUI_IS_SCROLLED_WINDOW (scrolled_window), -1);

  return priv->max_content_height;
}

/**
 * bobgui_scrolled_window_set_propagate_natural_width:
 * @scrolled_window: a `BobguiScrolledWindow`
 * @propagate: whether to propagate natural width
 *
 * Sets whether the natural width of the child should be calculated
 * and propagated through the scrolled window’s requested natural width.
 */
void
bobgui_scrolled_window_set_propagate_natural_width (BobguiScrolledWindow *scrolled_window,
                                                 gboolean           propagate)
{
  BobguiScrolledWindowPrivate *priv = bobgui_scrolled_window_get_instance_private (scrolled_window);

  g_return_if_fail (BOBGUI_IS_SCROLLED_WINDOW (scrolled_window));

  propagate = !!propagate;

  if (priv->propagate_natural_width != propagate)
    {
      priv->propagate_natural_width = propagate;
      g_object_notify_by_pspec (G_OBJECT (scrolled_window), properties [PROP_PROPAGATE_NATURAL_WIDTH]);
      bobgui_widget_queue_resize (BOBGUI_WIDGET (scrolled_window));
    }
}

/**
 * bobgui_scrolled_window_get_propagate_natural_width:
 * @scrolled_window: a `BobguiScrolledWindow`
 *
 * Reports whether the natural width of the child will be calculated
 * and propagated through the scrolled window’s requested natural width.
 *
 * Returns: whether natural width propagation is enabled.
 */
gboolean
bobgui_scrolled_window_get_propagate_natural_width (BobguiScrolledWindow *scrolled_window)
{
  BobguiScrolledWindowPrivate *priv = bobgui_scrolled_window_get_instance_private (scrolled_window);

  g_return_val_if_fail (BOBGUI_IS_SCROLLED_WINDOW (scrolled_window), -1);

  return priv->propagate_natural_width;
}

/**
 * bobgui_scrolled_window_set_propagate_natural_height:
 * @scrolled_window: a `BobguiScrolledWindow`
 * @propagate: whether to propagate natural height
 *
 * Sets whether the natural height of the child should be calculated
 * and propagated through the scrolled window’s requested natural height.
 */
void
bobgui_scrolled_window_set_propagate_natural_height (BobguiScrolledWindow *scrolled_window,
                                                  gboolean           propagate)
{
  BobguiScrolledWindowPrivate *priv = bobgui_scrolled_window_get_instance_private (scrolled_window);

  g_return_if_fail (BOBGUI_IS_SCROLLED_WINDOW (scrolled_window));

  propagate = !!propagate;

  if (priv->propagate_natural_height != propagate)
    {
      priv->propagate_natural_height = propagate;
      g_object_notify_by_pspec (G_OBJECT (scrolled_window), properties [PROP_PROPAGATE_NATURAL_HEIGHT]);
      bobgui_widget_queue_resize (BOBGUI_WIDGET (scrolled_window));
    }
}

/**
 * bobgui_scrolled_window_get_propagate_natural_height:
 * @scrolled_window: a `BobguiScrolledWindow`
 *
 * Reports whether the natural height of the child will be calculated
 * and propagated through the scrolled window’s requested natural height.
 *
 * Returns: whether natural height propagation is enabled.
 */
gboolean
bobgui_scrolled_window_get_propagate_natural_height (BobguiScrolledWindow *scrolled_window)
{
  BobguiScrolledWindowPrivate *priv = bobgui_scrolled_window_get_instance_private (scrolled_window);

  g_return_val_if_fail (BOBGUI_IS_SCROLLED_WINDOW (scrolled_window), -1);

  return priv->propagate_natural_height;
}

/**
 * bobgui_scrolled_window_set_child:
 * @scrolled_window: a `BobguiScrolledWindow`
 * @child: (nullable): the child widget
 *
 * Sets the child widget of @scrolled_window.
 *
 * If @child does not implement the [iface@Bobgui.Scrollable] interface,
 * the scrolled window will add @child to a [class@Bobgui.Viewport] instance
 * and then add the viewport as its child widget.
 */
void
bobgui_scrolled_window_set_child (BobguiScrolledWindow *scrolled_window,
                               BobguiWidget         *child)
{
  BobguiScrolledWindowPrivate *priv = bobgui_scrolled_window_get_instance_private (scrolled_window);
  BobguiWidget *scrollable_child;

  g_return_if_fail (BOBGUI_IS_SCROLLED_WINDOW (scrolled_window));
  g_return_if_fail (child == NULL ||
                    priv->child == child ||
                    (priv->auto_added_viewport && bobgui_viewport_get_child (BOBGUI_VIEWPORT (priv->child)) == child) ||
                    bobgui_widget_get_parent (child) == NULL);

  if (priv->child == child ||
      (priv->auto_added_viewport && bobgui_viewport_get_child (BOBGUI_VIEWPORT (priv->child)) == child))
    return;

  if (priv->child)
    {
      if (priv->auto_added_viewport)
        bobgui_viewport_set_child (BOBGUI_VIEWPORT (priv->child), NULL);

      g_object_set (priv->child,
                    "hadjustment", NULL,
                    "vadjustment", NULL,
                    NULL);

      g_clear_pointer (&priv->child, bobgui_widget_unparent);
      priv->auto_added_viewport = FALSE;
    }

  if (child)
    {
      BobguiAdjustment *hadj, *vadj;

      /* bobgui_scrolled_window_set_[hv]adjustment have the side-effect
       * of creating the scrollbars
       */
      if (!priv->hscrollbar)
        bobgui_scrolled_window_set_hadjustment (scrolled_window, NULL);

      if (!priv->vscrollbar)
        bobgui_scrolled_window_set_vadjustment (scrolled_window, NULL);

      hadj = bobgui_scrollbar_get_adjustment (BOBGUI_SCROLLBAR (priv->hscrollbar));
      vadj = bobgui_scrollbar_get_adjustment (BOBGUI_SCROLLBAR (priv->vscrollbar));

      if (BOBGUI_IS_SCROLLABLE (child))
        {
          scrollable_child = child;
          priv->auto_added_viewport = FALSE;
        }
      else
        {
          scrollable_child = bobgui_viewport_new (hadj, vadj);
          bobgui_viewport_set_child (BOBGUI_VIEWPORT (scrollable_child), child);
          priv->auto_added_viewport = TRUE;
        }

      priv->child = scrollable_child;
      bobgui_widget_insert_after (scrollable_child, BOBGUI_WIDGET (scrolled_window), NULL);

      g_object_set (scrollable_child,
                    "hadjustment", hadj,
                    "vadjustment", vadj,
                    NULL);
    }

  if (priv->child)
    {
      bobgui_accessible_update_relation (BOBGUI_ACCESSIBLE (priv->hscrollbar),
                                      BOBGUI_ACCESSIBLE_RELATION_CONTROLS, priv->child, NULL,
                                      -1);
      bobgui_accessible_update_relation (BOBGUI_ACCESSIBLE (priv->vscrollbar),
                                      BOBGUI_ACCESSIBLE_RELATION_CONTROLS, priv->child, NULL,
                                      -1);
    }
  else
    {
      bobgui_accessible_reset_relation (BOBGUI_ACCESSIBLE (priv->hscrollbar),
                                     BOBGUI_ACCESSIBLE_RELATION_CONTROLS);
      bobgui_accessible_reset_relation (BOBGUI_ACCESSIBLE (priv->vscrollbar),
                                     BOBGUI_ACCESSIBLE_RELATION_CONTROLS);
    }

  g_object_notify_by_pspec (G_OBJECT (scrolled_window), properties[PROP_CHILD]);
}

/**
 * bobgui_scrolled_window_get_child:
 * @scrolled_window: a `BobguiScrolledWindow`
 *
 * Gets the child widget of @scrolled_window.
 *
 * If the scrolled window automatically added a [class@Bobgui.Viewport], this
 * function will return the viewport widget, and you can retrieve its child
 * using [method@Bobgui.Viewport.get_child].
 *
 * Returns: (nullable) (transfer none): the child widget of @scrolled_window
 */
BobguiWidget *
bobgui_scrolled_window_get_child (BobguiScrolledWindow *scrolled_window)
{
  BobguiScrolledWindowPrivate *priv = bobgui_scrolled_window_get_instance_private (scrolled_window);

  g_return_val_if_fail (BOBGUI_IS_SCROLLED_WINDOW (scrolled_window), NULL);

  return priv->child;
}
