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

#include "bobguiviewport.h"

#include "bobguiadjustmentprivate.h"
#include "bobguimarshalers.h"
#include "bobguiprivate.h"
#include "bobguiscrollable.h"
#include "bobguiscrollinfoprivate.h"
#include "bobguitypebuiltins.h"
#include "bobguiwidgetprivate.h"
#include "bobguibuildable.h"
#include "bobguibuilderprivate.h"
#include "bobguitext.h"

#include <math.h>

/**
 * BobguiViewport:
 *
 * Implements scrollability for widgets that don't support scrolling
 * on their own.
 *
 * Use `BobguiViewport` to scroll child widgets such as `BobguiGrid`,
 * `BobguiBox`, and so on.
 *
 * The `BobguiViewport` will start scrolling content only if allocated
 * less than the child widget’s minimum size in a given orientation.
 *
 * # CSS nodes
 *
 * `BobguiViewport` has a single CSS node with name `viewport`.
 *
 * # Accessibility
 *
 * Until BOBGUI 4.10, `BobguiViewport` used the [enum@Bobgui.AccessibleRole.group] role.
 *
 * Starting from BOBGUI 4.12, `BobguiViewport` uses the [enum@Bobgui.AccessibleRole.generic] role.
 */

typedef struct _BobguiViewportPrivate       BobguiViewportPrivate;
typedef struct _BobguiViewportClass         BobguiViewportClass;

struct _BobguiViewport
{
  BobguiWidget parent_instance;

  BobguiWidget *child;

  BobguiAdjustment *adjustment[2];
  BobguiScrollablePolicy scroll_policy[2];
  guint scroll_to_focus : 1;

  gulong focus_handler;
};

struct _BobguiViewportClass
{
  BobguiWidgetClass parent_class;
};

enum {
  PROP_0,
  PROP_HADJUSTMENT,
  PROP_VADJUSTMENT,
  PROP_HSCROLL_POLICY,
  PROP_VSCROLL_POLICY,
  PROP_SCROLL_TO_FOCUS,
  PROP_CHILD
};


static void bobgui_viewport_set_property             (GObject         *object,
                                                   guint            prop_id,
                                                   const GValue    *value,
                                                   GParamSpec      *pspec);
static void bobgui_viewport_get_property             (GObject         *object,
                                                   guint            prop_id,
                                                   GValue          *value,
                                                   GParamSpec      *pspec);
static void bobgui_viewport_dispose                  (GObject         *object);
static void bobgui_viewport_size_allocate            (BobguiWidget       *widget,
                                                   int              width,
                                                   int              height,
                                                   int              baseline);

static void bobgui_viewport_adjustment_value_changed (BobguiAdjustment    *adjustment,
                                                   gpointer          data);
static void viewport_set_adjustment               (BobguiViewport      *viewport,
                                                   BobguiOrientation    orientation,
                                                   BobguiAdjustment    *adjustment);

static void setup_focus_change_handler (BobguiViewport *viewport);
static void clear_focus_change_handler (BobguiViewport *viewport);

static void bobgui_viewport_buildable_init (BobguiBuildableIface *iface);


G_DEFINE_TYPE_WITH_CODE (BobguiViewport, bobgui_viewport, BOBGUI_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_BUILDABLE,
                                                bobgui_viewport_buildable_init)
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_SCROLLABLE, NULL))

static BobguiBuildableIface *parent_buildable_iface;

static void
bobgui_viewport_buildable_add_child (BobguiBuildable *buildable,
                                  BobguiBuilder   *builder,
                                  GObject      *child,
                                  const char   *type)
{
  if (BOBGUI_IS_WIDGET (child))
    {
      bobgui_buildable_child_deprecation_warning (buildable, builder, NULL, "child");
      bobgui_viewport_set_child (BOBGUI_VIEWPORT (buildable), BOBGUI_WIDGET (child));
    }
  else
    {
      parent_buildable_iface->add_child (buildable, builder, child, type);
    }
}

static void
bobgui_viewport_buildable_init (BobguiBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);

  iface->add_child = bobgui_viewport_buildable_add_child;
}

static void
viewport_set_adjustment_values (BobguiViewport    *viewport,
                                BobguiOrientation  orientation,
                                int             viewport_size,
                                int             child_size)
{
  BobguiAdjustment *adjustment;
  double upper, value;

  adjustment = viewport->adjustment[orientation];
  upper = child_size;
  value = bobgui_adjustment_get_value (adjustment);

  /* We clamp to the left in RTL mode */
  if (orientation == BOBGUI_ORIENTATION_HORIZONTAL &&
      _bobgui_widget_get_direction (BOBGUI_WIDGET (viewport)) == BOBGUI_TEXT_DIR_RTL)
    {
      double dist = bobgui_adjustment_get_upper (adjustment)
                     - value
                     - bobgui_adjustment_get_page_size (adjustment);
      value = upper - dist - viewport_size;
    }

  bobgui_adjustment_configure (adjustment,
                            value,
                            0,
                            upper,
                            viewport_size * 0.1,
                            viewport_size * 0.9,
                            viewport_size);
}

static void
bobgui_viewport_measure (BobguiWidget      *widget,
                      BobguiOrientation  orientation,
                      int             for_size,
                      int            *minimum,
                      int            *natural,
                      int            *minimum_baseline,
                      int            *natural_baseline)
{
  BobguiViewport *viewport = BOBGUI_VIEWPORT (widget);

  if (viewport->child)
    bobgui_widget_measure (viewport->child,
                        orientation,
                        for_size,
                        minimum, natural,
                        NULL, NULL);
}

static void
bobgui_viewport_compute_expand (BobguiWidget *widget,
                             gboolean  *hexpand,
                             gboolean  *vexpand)
{
  BobguiViewport *viewport = BOBGUI_VIEWPORT (widget);

  if (viewport->child)
    {
      *hexpand = bobgui_widget_compute_expand (viewport->child, BOBGUI_ORIENTATION_HORIZONTAL);
      *vexpand = bobgui_widget_compute_expand (viewport->child, BOBGUI_ORIENTATION_VERTICAL);
    }
  else
    {
      *hexpand = FALSE;
      *vexpand = FALSE;
    }
}

static BobguiSizeRequestMode
bobgui_viewport_get_request_mode (BobguiWidget *widget)
{
  BobguiViewport *viewport = BOBGUI_VIEWPORT (widget);

  if (viewport->child)
    return bobgui_widget_get_request_mode (viewport->child);
  else
    return BOBGUI_SIZE_REQUEST_CONSTANT_SIZE;
}

#define ADJUSTMENT_POINTER(orientation)            \
  (((orientation) == BOBGUI_ORIENTATION_HORIZONTAL) ? \
     &viewport->adjustment[BOBGUI_ORIENTATION_HORIZONTAL] : &viewport->adjustment[BOBGUI_ORIENTATION_VERTICAL])

static void
viewport_disconnect_adjustment (BobguiViewport    *viewport,
                                BobguiOrientation  orientation)
{
  BobguiAdjustment **adjustmentp = ADJUSTMENT_POINTER (orientation);

  if (*adjustmentp)
    {
      g_signal_handlers_disconnect_by_func (*adjustmentp,
                                            bobgui_viewport_adjustment_value_changed,
                                            viewport);
      g_object_unref (*adjustmentp);
      *adjustmentp = NULL;
    }
}

static void
bobgui_viewport_dispose (GObject *object)
{
  BobguiViewport *viewport = BOBGUI_VIEWPORT (object);

  viewport_disconnect_adjustment (viewport, BOBGUI_ORIENTATION_HORIZONTAL);
  viewport_disconnect_adjustment (viewport, BOBGUI_ORIENTATION_VERTICAL);

  clear_focus_change_handler (viewport);

  g_clear_pointer (&viewport->child, bobgui_widget_unparent);

  G_OBJECT_CLASS (bobgui_viewport_parent_class)->dispose (object);

}

static void
bobgui_viewport_root (BobguiWidget *widget)
{
  BobguiViewport *viewport = BOBGUI_VIEWPORT (widget);

  BOBGUI_WIDGET_CLASS (bobgui_viewport_parent_class)->root (widget);

  if (viewport->scroll_to_focus)
    setup_focus_change_handler (viewport);
}

static void
bobgui_viewport_unroot (BobguiWidget *widget)
{
  BobguiViewport *viewport = BOBGUI_VIEWPORT (widget);

  if (viewport->scroll_to_focus)
    clear_focus_change_handler (viewport);

  BOBGUI_WIDGET_CLASS (bobgui_viewport_parent_class)->unroot (widget);
}

static void
bobgui_viewport_class_init (BobguiViewportClass *class)
{
  GObjectClass   *gobject_class;
  BobguiWidgetClass *widget_class;

  gobject_class = G_OBJECT_CLASS (class);
  widget_class = (BobguiWidgetClass*) class;

  gobject_class->dispose = bobgui_viewport_dispose;
  gobject_class->set_property = bobgui_viewport_set_property;
  gobject_class->get_property = bobgui_viewport_get_property;

  widget_class->size_allocate = bobgui_viewport_size_allocate;
  widget_class->measure = bobgui_viewport_measure;
  widget_class->root = bobgui_viewport_root;
  widget_class->unroot = bobgui_viewport_unroot;
  widget_class->compute_expand = bobgui_viewport_compute_expand;
  widget_class->get_request_mode = bobgui_viewport_get_request_mode;

  /* BobguiScrollable implementation */
  g_object_class_override_property (gobject_class, PROP_HADJUSTMENT,    "hadjustment");
  g_object_class_override_property (gobject_class, PROP_VADJUSTMENT,    "vadjustment");
  g_object_class_override_property (gobject_class, PROP_HSCROLL_POLICY, "hscroll-policy");
  g_object_class_override_property (gobject_class, PROP_VSCROLL_POLICY, "vscroll-policy");

  /**
   * BobguiViewport:scroll-to-focus:
   *
   * Whether to scroll when the focus changes.
   *
   * Before 4.6.2, this property was mistakenly defaulting to FALSE, so if your
   * code needs to work with older versions, consider setting it explicitly to
   * TRUE.
   */
  g_object_class_install_property (gobject_class,
                                   PROP_SCROLL_TO_FOCUS,
                                   g_param_spec_boolean ("scroll-to-focus", NULL, NULL,
                                                         TRUE,
                                                         BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * BobguiViewport:child:
   *
   * The child widget.
   */
  g_object_class_install_property (gobject_class,
                                   PROP_CHILD,
                                   g_param_spec_object ("child", NULL, NULL,
                                                        BOBGUI_TYPE_WIDGET,
                                                        BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  bobgui_widget_class_set_css_name (widget_class, I_("viewport"));
  bobgui_widget_class_set_accessible_role (widget_class, BOBGUI_ACCESSIBLE_ROLE_GENERIC);
}

static void
bobgui_viewport_set_property (GObject         *object,
                           guint            prop_id,
                           const GValue    *value,
                           GParamSpec      *pspec)
{
  BobguiViewport *viewport = BOBGUI_VIEWPORT (object);

  switch (prop_id)
    {
    case PROP_HADJUSTMENT:
      viewport_set_adjustment (viewport, BOBGUI_ORIENTATION_HORIZONTAL, g_value_get_object (value));
      break;
    case PROP_VADJUSTMENT:
      viewport_set_adjustment (viewport, BOBGUI_ORIENTATION_VERTICAL, g_value_get_object (value));
      break;
    case PROP_HSCROLL_POLICY:
      if (viewport->scroll_policy[BOBGUI_ORIENTATION_HORIZONTAL] != g_value_get_enum (value))
        {
          viewport->scroll_policy[BOBGUI_ORIENTATION_HORIZONTAL] = g_value_get_enum (value);
          bobgui_widget_queue_resize (BOBGUI_WIDGET (viewport));
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    case PROP_VSCROLL_POLICY:
      if (viewport->scroll_policy[BOBGUI_ORIENTATION_VERTICAL] != g_value_get_enum (value))
        {
          viewport->scroll_policy[BOBGUI_ORIENTATION_VERTICAL] = g_value_get_enum (value);
          bobgui_widget_queue_resize (BOBGUI_WIDGET (viewport));
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    case PROP_SCROLL_TO_FOCUS:
      bobgui_viewport_set_scroll_to_focus (viewport, g_value_get_boolean (value));
      break;
    case PROP_CHILD:
      bobgui_viewport_set_child (viewport, g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_viewport_get_property (GObject         *object,
                           guint            prop_id,
                           GValue          *value,
                           GParamSpec      *pspec)
{
  BobguiViewport *viewport = BOBGUI_VIEWPORT (object);

  switch (prop_id)
    {
    case PROP_HADJUSTMENT:
      g_value_set_object (value, viewport->adjustment[BOBGUI_ORIENTATION_HORIZONTAL]);
      break;
    case PROP_VADJUSTMENT:
      g_value_set_object (value, viewport->adjustment[BOBGUI_ORIENTATION_VERTICAL]);
      break;
    case PROP_HSCROLL_POLICY:
      g_value_set_enum (value, viewport->scroll_policy[BOBGUI_ORIENTATION_HORIZONTAL]);
      break;
    case PROP_VSCROLL_POLICY:
      g_value_set_enum (value, viewport->scroll_policy[BOBGUI_ORIENTATION_VERTICAL]);
      break;
    case PROP_SCROLL_TO_FOCUS:
      g_value_set_boolean (value, viewport->scroll_to_focus);
      break;
    case PROP_CHILD:
      g_value_set_object (value, bobgui_viewport_get_child (viewport));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_viewport_init (BobguiViewport *viewport)
{
  BobguiWidget *widget;

  widget = BOBGUI_WIDGET (viewport);

  bobgui_widget_set_overflow (widget, BOBGUI_OVERFLOW_HIDDEN);

  viewport->adjustment[BOBGUI_ORIENTATION_HORIZONTAL] = NULL;
  viewport->adjustment[BOBGUI_ORIENTATION_VERTICAL] = NULL;

  viewport_set_adjustment (viewport, BOBGUI_ORIENTATION_HORIZONTAL, NULL);
  viewport_set_adjustment (viewport, BOBGUI_ORIENTATION_VERTICAL, NULL);

  viewport->scroll_to_focus = TRUE;
}

/**
 * bobgui_viewport_new:
 * @hadjustment: (nullable): horizontal adjustment
 * @vadjustment: (nullable): vertical adjustment
 *
 * Creates a new `BobguiViewport`.
 *
 * The new viewport uses the given adjustments, or default
 * adjustments if none are given.
 *
 * Returns: a new `BobguiViewport`
 */
BobguiWidget*
bobgui_viewport_new (BobguiAdjustment *hadjustment,
                  BobguiAdjustment *vadjustment)
{
  BobguiWidget *viewport;

  viewport = g_object_new (BOBGUI_TYPE_VIEWPORT,
                           "hadjustment", hadjustment,
                           "vadjustment", vadjustment,
                           NULL);

  return viewport;
}

static void
viewport_set_adjustment (BobguiViewport    *viewport,
                         BobguiOrientation  orientation,
                         BobguiAdjustment  *adjustment)
{
  BobguiAdjustment **adjustmentp = ADJUSTMENT_POINTER (orientation);

  if (adjustment && adjustment == *adjustmentp)
    return;

  if (!adjustment)
    adjustment = bobgui_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
  viewport_disconnect_adjustment (viewport, orientation);
  *adjustmentp = adjustment;
  g_object_ref_sink (adjustment);

  g_signal_connect (adjustment, "value-changed",
                    G_CALLBACK (bobgui_viewport_adjustment_value_changed),
                    viewport);

  bobgui_viewport_adjustment_value_changed (adjustment, viewport);
}

static void
bobgui_viewport_size_allocate (BobguiWidget *widget,
                            int        width,
                            int        height,
                            int        baseline)
{
  BobguiViewport *viewport = BOBGUI_VIEWPORT (widget);
  int child_size[2];

  g_object_freeze_notify (G_OBJECT (viewport->adjustment[BOBGUI_ORIENTATION_HORIZONTAL]));
  g_object_freeze_notify (G_OBJECT (viewport->adjustment[BOBGUI_ORIENTATION_VERTICAL]));

  child_size[BOBGUI_ORIENTATION_HORIZONTAL] = width;
  child_size[BOBGUI_ORIENTATION_VERTICAL] = height;

  if (viewport->child && bobgui_widget_get_visible (viewport->child))
    {
      BobguiOrientation orientation, opposite;
      int min, nat;

      if (bobgui_widget_get_request_mode (viewport->child) == BOBGUI_SIZE_REQUEST_WIDTH_FOR_HEIGHT)
        orientation = BOBGUI_ORIENTATION_VERTICAL;
      else
        orientation = BOBGUI_ORIENTATION_HORIZONTAL;
      opposite = OPPOSITE_ORIENTATION (orientation);

      bobgui_widget_measure (viewport->child,
                          orientation, -1,
                          &min, &nat,
                          NULL, NULL);
      if (viewport->scroll_policy[orientation] == BOBGUI_SCROLL_MINIMUM)
        child_size[orientation] = MAX (child_size[orientation], min);
      else
        child_size[orientation] = MAX (child_size[orientation], nat);

      bobgui_widget_measure (viewport->child,
                          opposite, child_size[orientation],
                          &min, &nat,
                          NULL, NULL);
      if (viewport->scroll_policy[opposite] == BOBGUI_SCROLL_MINIMUM)
        child_size[opposite] = MAX (child_size[opposite], min);
      else
        child_size[opposite] = MAX (child_size[opposite], nat);
    }

  viewport_set_adjustment_values (viewport, BOBGUI_ORIENTATION_HORIZONTAL, width, child_size[BOBGUI_ORIENTATION_HORIZONTAL]);
  viewport_set_adjustment_values (viewport, BOBGUI_ORIENTATION_VERTICAL, height, child_size[BOBGUI_ORIENTATION_VERTICAL]);

  if (viewport->child && bobgui_widget_get_visible (viewport->child))
    {
      BobguiAllocation child_allocation;

      child_allocation.width = child_size[BOBGUI_ORIENTATION_HORIZONTAL];
      child_allocation.height = child_size[BOBGUI_ORIENTATION_VERTICAL];
      child_allocation.x = - bobgui_adjustment_get_value (viewport->adjustment[BOBGUI_ORIENTATION_HORIZONTAL]);
      child_allocation.y = - bobgui_adjustment_get_value (viewport->adjustment[BOBGUI_ORIENTATION_VERTICAL]);

      bobgui_widget_size_allocate (viewport->child, &child_allocation, -1);
    }

  g_object_thaw_notify (G_OBJECT (viewport->adjustment[BOBGUI_ORIENTATION_HORIZONTAL]));
  g_object_thaw_notify (G_OBJECT (viewport->adjustment[BOBGUI_ORIENTATION_VERTICAL]));
}

static void
bobgui_viewport_adjustment_value_changed (BobguiAdjustment *adjustment,
                                       gpointer       data)
{
  bobgui_widget_queue_allocate (BOBGUI_WIDGET (data));
}

/**
 * bobgui_viewport_get_scroll_to_focus:
 * @viewport: a `BobguiViewport`
 *
 * Gets whether the viewport is scrolling to keep the focused
 * child in view.
 *
 * Returns: %TRUE if the viewport keeps the focus child scrolled to view
 */
gboolean
bobgui_viewport_get_scroll_to_focus (BobguiViewport *viewport)
{
  g_return_val_if_fail (BOBGUI_IS_VIEWPORT (viewport), FALSE);

  return viewport->scroll_to_focus;
}

/**
 * bobgui_viewport_set_scroll_to_focus:
 * @viewport: a `BobguiViewport`
 * @scroll_to_focus: whether to keep the focus widget scrolled to view
 *
 * Sets whether the viewport should automatically scroll
 * to keep the focused child in view.
 */
void
bobgui_viewport_set_scroll_to_focus (BobguiViewport *viewport,
                                  gboolean     scroll_to_focus)
{
  g_return_if_fail (BOBGUI_IS_VIEWPORT (viewport));

  if (viewport->scroll_to_focus == scroll_to_focus)
    return;

  viewport->scroll_to_focus = scroll_to_focus;

  if (bobgui_widget_get_root (BOBGUI_WIDGET (viewport)))
    {
      if (scroll_to_focus)
        setup_focus_change_handler (viewport);
      else
        clear_focus_change_handler (viewport);
    }

  g_object_notify (G_OBJECT (viewport), "scroll-to-focus");
}

static void
focus_change_handler (BobguiWidget *widget)
{
  BobguiViewport *viewport = BOBGUI_VIEWPORT (widget);
  BobguiRoot *root;
  BobguiWidget *focus_widget;

  if ((bobgui_widget_get_state_flags (widget) & BOBGUI_STATE_FLAG_FOCUS_WITHIN) == 0)
    return;

  root = bobgui_widget_get_root (widget);
  focus_widget = bobgui_root_get_focus (root);

  if (!focus_widget)
    return;

  if (BOBGUI_IS_TEXT (focus_widget))
    focus_widget = bobgui_widget_get_parent (focus_widget);

  if (bobgui_widget_get_native (focus_widget) != bobgui_widget_get_native (widget))
    return;

  bobgui_viewport_scroll_to (viewport, focus_widget, NULL);
}

static void
setup_focus_change_handler (BobguiViewport *viewport)
{
  BobguiRoot *root;

  root = bobgui_widget_get_root (BOBGUI_WIDGET (viewport));

  viewport->focus_handler = g_signal_connect_swapped (root, "notify::focus-widget",
                                                      G_CALLBACK (focus_change_handler), viewport);
}

static void
clear_focus_change_handler (BobguiViewport *viewport)
{
  BobguiRoot *root;

  root = bobgui_widget_get_root (BOBGUI_WIDGET (viewport));

  if (viewport->focus_handler)
    {
      g_signal_handler_disconnect (root, viewport->focus_handler);
      viewport->focus_handler = 0;
    }
}

/**
 * bobgui_viewport_set_child:
 * @viewport: a `BobguiViewport`
 * @child: (nullable): the child widget
 *
 * Sets the child widget of @viewport.
 */
void
bobgui_viewport_set_child (BobguiViewport *viewport,
                        BobguiWidget   *child)
{
  g_return_if_fail (BOBGUI_IS_VIEWPORT (viewport));
  g_return_if_fail (child == NULL || viewport->child == child || bobgui_widget_get_parent (child) == NULL);

  if (viewport->child == child)
    return;

  g_clear_pointer (&viewport->child, bobgui_widget_unparent);

  if (child)
    {
      viewport->child = child;
      bobgui_widget_set_parent (child, BOBGUI_WIDGET (viewport));
    }

  g_object_notify (G_OBJECT (viewport), "child");
}

/**
 * bobgui_viewport_get_child:
 * @viewport: a `BobguiViewport`
 *
 * Gets the child widget of @viewport.
 *
 * Returns: (nullable) (transfer none): the child widget of @viewport
 */
BobguiWidget *
bobgui_viewport_get_child (BobguiViewport *viewport)
{
  g_return_val_if_fail (BOBGUI_IS_VIEWPORT (viewport), NULL);

  return viewport->child;
}

/**
 * bobgui_viewport_scroll_to:
 * @viewport: a `BobguiViewport`
 * @descendant: a descendant widget of the viewport
 * @scroll: (nullable) (transfer full): details of how to perform
 *   the scroll operation or NULL to scroll into view
 *
 * Scrolls a descendant of the viewport into view.
 *
 * The viewport and the descendant must be visible and mapped for
 * this function to work, otherwise no scrolling will be performed.
 *
 * Since: 4.12
 **/
void
bobgui_viewport_scroll_to (BobguiViewport   *viewport,
                        BobguiWidget     *descendant,
                        BobguiScrollInfo *scroll)
{
  graphene_rect_t bounds;
  int x, y;
  double adj_x, adj_y;

  g_return_if_fail (BOBGUI_IS_VIEWPORT (viewport));
  g_return_if_fail (BOBGUI_IS_WIDGET (descendant));

  if (!bobgui_widget_compute_bounds (descendant, BOBGUI_WIDGET (viewport), &bounds))
    return;

  adj_x = bobgui_adjustment_get_value (viewport->adjustment[BOBGUI_ORIENTATION_HORIZONTAL]);
  adj_y = bobgui_adjustment_get_value (viewport->adjustment[BOBGUI_ORIENTATION_VERTICAL]);

  bobgui_scroll_info_compute_scroll (scroll,
                                  &(GdkRectangle) {
                                    floor (bounds.origin.x + adj_x),
                                    floor (bounds.origin.y + adj_y),
                                    ceil (bounds.origin.x + bounds.size.width) - floor (bounds.origin.x),
                                    ceil (bounds.origin.y + bounds.size.height) - floor (bounds.origin.y)
                                  },
                                  &(GdkRectangle) {
                                    adj_x,
                                    adj_y,
                                    bobgui_widget_get_width (BOBGUI_WIDGET (viewport)),
                                    bobgui_widget_get_height (BOBGUI_WIDGET (viewport))
                                  },
                                  &x, &y);

  bobgui_adjustment_animate_to_value (viewport->adjustment[BOBGUI_ORIENTATION_HORIZONTAL], x);
  bobgui_adjustment_animate_to_value (viewport->adjustment[BOBGUI_ORIENTATION_VERTICAL], y);

  g_clear_pointer (&scroll, bobgui_scroll_info_unref);
}

