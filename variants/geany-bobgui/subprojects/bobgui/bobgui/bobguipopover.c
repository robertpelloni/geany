/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2019 Red Hat, Inc.
 *
 * Authors:
 * - Matthias Clasen <mclasen@redhat.com>
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

/**
 * BobguiPopover:
 *
 * Presents a bubble-like popup.
 *
 * <picture>
 *   <source srcset="popover-dark.png" media="(prefers-color-scheme: dark)">
 *   <img alt="An example BobguiPopover" src="popover.png">
 * </picture>
 *
 * It is primarily meant to provide context-dependent information
 * or options. Popovers are attached to a parent widget. The parent widget
 * must support popover children, as [class@Bobgui.MenuButton] and
 * [class@Bobgui.PopoverMenuBar] do. If you want to make a custom widget that
 * has an attached popover, you need to call [method@Bobgui.Popover.present]
 * in your [vfunc@Bobgui.Widget.size_allocate] vfunc, in order to update the
 * positioning of the popover.
 *
 * The position of a popover relative to the widget it is attached to
 * can also be changed with [method@Bobgui.Popover.set_position]. By default,
 * it points to the whole widget area, but it can be made to point to
 * a specific area using [method@Bobgui.Popover.set_pointing_to].
 *
 * By default, `BobguiPopover` performs a grab, in order to ensure input
 * events get redirected to it while it is shown, and also so the popover
 * is dismissed in the expected situations (clicks outside the popover,
 * or the Escape key being pressed). If no such modal behavior is desired
 * on a popover, [method@Bobgui.Popover.set_autohide] may be called on it to
 * tweak its behavior.
 *
 * ## BobguiPopover as menu replacement
 *
 * `BobguiPopover` is often used to replace menus. The best way to do this
 * is to use the [class@Bobgui.PopoverMenu] subclass which supports being
 * populated from a `GMenuModel` with [ctor@Bobgui.PopoverMenu.new_from_model].
 *
 * ```xml
 * <section>
 *   <attribute name="display-hint">horizontal-buttons</attribute>
 *   <item>
 *     <attribute name="label">Cut</attribute>
 *     <attribute name="action">app.cut</attribute>
 *     <attribute name="verb-icon">edit-cut-symbolic</attribute>
 *   </item>
 *   <item>
 *     <attribute name="label">Copy</attribute>
 *     <attribute name="action">app.copy</attribute>
 *     <attribute name="verb-icon">edit-copy-symbolic</attribute>
 *   </item>
 *   <item>
 *     <attribute name="label">Paste</attribute>
 *     <attribute name="action">app.paste</attribute>
 *     <attribute name="verb-icon">edit-paste-symbolic</attribute>
 *   </item>
 * </section>
 * ```
 *
 * # Shortcuts and Gestures
 *
 * `BobguiPopover` supports the following keyboard shortcuts:
 *
 * - <kbd>Escape</kbd> closes the popover.
 * - <kbd>Alt</kbd> makes the mnemonics visible.
 *
 * The following signals have default keybindings:
 *
 * - [signal@Bobgui.Popover::activate-default]
 *
 * # CSS nodes
 *
 * ```
 * popover.background[.menu]
 * ├── arrow
 * ╰── contents
 *     ╰── <child>
 * ```
 *
 * `BobguiPopover` has a main node with name `popover`, an arrow with name `arrow`,
 * and another node for the content named `contents`. The `popover` node always
 * gets the `.background` style class. It also gets the `.menu` style class
 * if the popover is menu-like, e.g. is a [class@Bobgui.PopoverMenu].
 *
 * Particular uses of `BobguiPopover`, such as touch selection popups or
 * magnifiers in `BobguiEntry` or `BobguiTextView` get style classes like
 * `.touch-selection` or `.magnifier` to differentiate from plain popovers.
 *
 * When styling a popover directly, the `popover` node should usually
 * not have any background. The visible part of the popover can have
 * a shadow. To specify it in CSS, set the box-shadow of the `contents` node.
 *
 * Note that, in order to accomplish appropriate arrow visuals, `BobguiPopover`
 * uses custom drawing for the `arrow` node. This makes it possible for the
 * arrow to change its shape dynamically, but it also limits the possibilities
 * of styling it using CSS. In particular, the `arrow` gets drawn over the
 * `content` node's border and shadow, so they look like one shape, which
 * means that the border width of the `content` node and the `arrow` node should
 * be the same. The arrow also does not support any border shape other than
 * solid, no border-radius, only one border width (border-bottom-width is
 * used) and no box-shadow.
 */

#include "config.h"

#include "bobguipopoverprivate.h"
#include "bobguipopovermenuprivate.h"
#include "bobguinative.h"
#include "bobguiwidgetprivate.h"
#include "bobguieventcontrollerkey.h"
#include "bobguieventcontrollerfocus.h"
#include "bobguibinlayout.h"
#include "bobguienums.h"
#include "bobguitypebuiltins.h"
#include "bobguipopovercontentprivate.h"
#include "bobguiprivate.h"
#include "bobguimain.h"
#include "bobguistack.h"
#include "bobguimenusectionboxprivate.h"
#include "gdk/gdkeventsprivate.h"
#include "bobguipointerfocusprivate.h"
#include "bobguicsscolorvalueprivate.h"
#include "bobguisnapshot.h"
#include "bobguirenderbackgroundprivate.h"
#include "bobguishortcutmanager.h"
#include "bobguibuildable.h"
#include "bobguibuilderprivate.h"
#include "bobguitooltipprivate.h"
#include "bobguicssboxesimplprivate.h"
#include "bobguinativeprivate.h"

#include "bobguiroundedboxprivate.h"
#include "gsk/gskrendererprivate.h"
#include "gsk/gskroundedrectprivate.h"
#include "bobguicssshadowvalueprivate.h"
#include "bobguicsscornervalueprivate.h"

#include "gdk/gdksurfaceprivate.h"

#include <string.h> /* memset */

#define MNEMONICS_DELAY 300 /* ms */

#define TAIL_GAP_WIDTH  24
#define TAIL_HEIGHT     12

#define POS_IS_VERTICAL(p) ((p) == BOBGUI_POS_TOP || (p) == BOBGUI_POS_BOTTOM)

typedef struct {
  GdkSurface *surface;
  GskRenderer *renderer;
  BobguiWidget *default_widget;

  GdkRectangle pointing_to;
  gboolean has_pointing_to;
  guint surface_transform_changed_cb;
  BobguiPositionType position;
  gboolean autohide;
  gboolean has_arrow;
  gboolean mnemonics_visible;
  gboolean cascade_popdown;

  int x_offset;
  int y_offset;

  guint mnemonics_display_timeout_id;

  BobguiWidget *child;
  BobguiWidget *contents_widget;
  BobguiCssNode *arrow_node;
  GskRenderNode *arrow_render_node;

  GdkPopupLayout *layout;
  GdkRectangle final_rect;
  BobguiPositionType final_position;
} BobguiPopoverPrivate;

enum {
  CLOSED,
  ACTIVATE_DEFAULT,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

enum {
  PROP_POINTING_TO = 1,
  PROP_POSITION,
  PROP_AUTOHIDE,
  PROP_DEFAULT_WIDGET,
  PROP_HAS_ARROW,
  PROP_MNEMONICS_VISIBLE,
  PROP_CHILD,
  PROP_CASCADE_POPDOWN,
  NUM_PROPERTIES
};

static GParamSpec *properties[NUM_PROPERTIES] = { NULL };

static void bobgui_popover_buildable_init (BobguiBuildableIface *iface);

static void bobgui_popover_shortcut_manager_interface_init (BobguiShortcutManagerInterface *iface);
static void bobgui_popover_native_interface_init (BobguiNativeInterface *iface);

G_DEFINE_TYPE_WITH_CODE (BobguiPopover, bobgui_popover, BOBGUI_TYPE_WIDGET,
                         G_ADD_PRIVATE (BobguiPopover)
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_BUILDABLE,
                                                bobgui_popover_buildable_init)
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_SHORTCUT_MANAGER,
                                                bobgui_popover_shortcut_manager_interface_init)
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_NATIVE,
                                                bobgui_popover_native_interface_init))


static GdkSurface *
bobgui_popover_native_get_surface (BobguiNative *native)
{
  BobguiPopover *popover = BOBGUI_POPOVER (native);
  BobguiPopoverPrivate *priv = bobgui_popover_get_instance_private (popover);

  return priv->surface;
}

static GskRenderer *
bobgui_popover_native_get_renderer (BobguiNative *native)
{
  BobguiPopover *popover = BOBGUI_POPOVER (native);
  BobguiPopoverPrivate *priv = bobgui_popover_get_instance_private (popover);

  return priv->renderer;
}

static void
bobgui_popover_native_get_surface_transform (BobguiNative *native,
                                          double    *x,
                                          double    *y)
{
  BobguiCssBoxes css_boxes;
  const graphene_rect_t *margin_rect;

  bobgui_css_boxes_init (&css_boxes, BOBGUI_WIDGET (native));
  margin_rect = bobgui_css_boxes_get_margin_rect (&css_boxes);

  *x = - margin_rect->origin.x;
  *y = - margin_rect->origin.y;
}

static gboolean
is_gravity_facing_north (GdkGravity gravity)
{
  switch (gravity)
    {
    case GDK_GRAVITY_NORTH_EAST:
    case GDK_GRAVITY_NORTH:
    case GDK_GRAVITY_NORTH_WEST:
    case GDK_GRAVITY_STATIC:
      return TRUE;
    case GDK_GRAVITY_SOUTH_WEST:
    case GDK_GRAVITY_WEST:
    case GDK_GRAVITY_SOUTH_EAST:
    case GDK_GRAVITY_EAST:
    case GDK_GRAVITY_CENTER:
    case GDK_GRAVITY_SOUTH:
      return FALSE;
    default:
      g_assert_not_reached ();
    }
}

static gboolean
is_gravity_facing_south (GdkGravity gravity)
{
  switch (gravity)
    {
    case GDK_GRAVITY_SOUTH_WEST:
    case GDK_GRAVITY_SOUTH_EAST:
    case GDK_GRAVITY_SOUTH:
      return TRUE;
    case GDK_GRAVITY_NORTH_EAST:
    case GDK_GRAVITY_NORTH:
    case GDK_GRAVITY_NORTH_WEST:
    case GDK_GRAVITY_STATIC:
    case GDK_GRAVITY_WEST:
    case GDK_GRAVITY_EAST:
    case GDK_GRAVITY_CENTER:
      return FALSE;
    default:
      g_assert_not_reached ();
    }
}

static gboolean
is_gravity_facing_west (GdkGravity gravity)
{
  switch (gravity)
    {
    case GDK_GRAVITY_NORTH_WEST:
    case GDK_GRAVITY_STATIC:
    case GDK_GRAVITY_SOUTH_WEST:
    case GDK_GRAVITY_WEST:
      return TRUE;
    case GDK_GRAVITY_NORTH_EAST:
    case GDK_GRAVITY_SOUTH_EAST:
    case GDK_GRAVITY_EAST:
    case GDK_GRAVITY_NORTH:
    case GDK_GRAVITY_CENTER:
    case GDK_GRAVITY_SOUTH:
      return FALSE;
    default:
      g_assert_not_reached ();
    }
}

static gboolean
is_gravity_facing_east (GdkGravity gravity)
{
  switch (gravity)
    {
    case GDK_GRAVITY_NORTH_EAST:
    case GDK_GRAVITY_SOUTH_EAST:
    case GDK_GRAVITY_EAST:
      return TRUE;
    case GDK_GRAVITY_NORTH_WEST:
    case GDK_GRAVITY_STATIC:
    case GDK_GRAVITY_SOUTH_WEST:
    case GDK_GRAVITY_WEST:
    case GDK_GRAVITY_NORTH:
    case GDK_GRAVITY_CENTER:
    case GDK_GRAVITY_SOUTH:
      return FALSE;
    default:
      g_assert_not_reached ();
    }
}

static gboolean
did_flip_horizontally (GdkGravity original_gravity,
                       GdkGravity final_gravity)
{
  g_return_val_if_fail (original_gravity, FALSE);
  g_return_val_if_fail (final_gravity, FALSE);

  if (is_gravity_facing_east (original_gravity) &&
      is_gravity_facing_west (final_gravity))
    return TRUE;

  if (is_gravity_facing_west (original_gravity) &&
      is_gravity_facing_east (final_gravity))
    return TRUE;

  return FALSE;
}

static gboolean
did_flip_vertically (GdkGravity original_gravity,
                     GdkGravity final_gravity)
{
  g_return_val_if_fail (original_gravity, FALSE);
  g_return_val_if_fail (final_gravity, FALSE);

  if (is_gravity_facing_north (original_gravity) &&
      is_gravity_facing_south (final_gravity))
    return TRUE;

  if (is_gravity_facing_south (original_gravity) &&
      is_gravity_facing_north (final_gravity))
    return TRUE;

  return FALSE;
}

static void
update_popover_layout (BobguiPopover     *popover,
                       GdkPopupLayout *layout,
                       int             width,
                       int             height)
{
  BobguiPopoverPrivate *priv = bobgui_popover_get_instance_private (popover);
  GdkRectangle final_rect;
  gboolean flipped_x;
  gboolean flipped_y;
  gboolean attachment_point_changed;
  GdkPopup *popup = GDK_POPUP (priv->surface);
  BobguiPositionType position;

  g_clear_pointer (&priv->layout, gdk_popup_layout_unref);
  priv->layout = layout;

  final_rect = (GdkRectangle) {
    .x = gdk_popup_get_position_x (GDK_POPUP (priv->surface)),
    .y = gdk_popup_get_position_y (GDK_POPUP (priv->surface)),
    .width = gdk_surface_get_width (priv->surface),
    .height = gdk_surface_get_height (priv->surface),
  };

  flipped_x =
    did_flip_horizontally (gdk_popup_layout_get_rect_anchor (layout),
                           gdk_popup_get_rect_anchor (popup)) &&
    did_flip_horizontally (gdk_popup_layout_get_surface_anchor (layout),
                           gdk_popup_get_surface_anchor (popup));
  flipped_y =
    did_flip_vertically (gdk_popup_layout_get_rect_anchor (layout),
                         gdk_popup_get_rect_anchor (popup)) &&
    did_flip_vertically (gdk_popup_layout_get_surface_anchor (layout),
                         gdk_popup_get_surface_anchor (popup));

  attachment_point_changed = final_rect.x != priv->final_rect.x ||
                             final_rect.y != priv->final_rect.y;

  priv->final_rect = final_rect;

  position = priv->final_position;

  switch (priv->position)
    {
    case BOBGUI_POS_LEFT:
      priv->final_position = flipped_x ? BOBGUI_POS_RIGHT : BOBGUI_POS_LEFT;
      break;
    case BOBGUI_POS_RIGHT:
      priv->final_position = flipped_x ? BOBGUI_POS_LEFT : BOBGUI_POS_RIGHT;
      break;
    case BOBGUI_POS_TOP:
      priv->final_position = flipped_y ? BOBGUI_POS_BOTTOM : BOBGUI_POS_TOP;
      break;
    case BOBGUI_POS_BOTTOM:
      priv->final_position = flipped_y ? BOBGUI_POS_TOP : BOBGUI_POS_BOTTOM;
      break;
    default:
      g_assert_not_reached ();
      break;
    }

  if (priv->final_position != position ||
      priv->final_rect.width != width ||
      priv->final_rect.height != height ||
      attachment_point_changed)
    {
      bobgui_widget_queue_allocate (BOBGUI_WIDGET (popover));
      g_clear_pointer (&priv->arrow_render_node, gsk_render_node_unref);
    }

  bobgui_widget_queue_draw (BOBGUI_WIDGET (popover));
}

static void
compute_surface_pointing_to (BobguiPopover   *popover,
                             GdkRectangle *rect)
{
  BobguiPopoverPrivate *priv = bobgui_popover_get_instance_private (popover);
  BobguiWidget *parent;
  BobguiNative *native;
  graphene_rect_t bounds;
  double nx, ny;

  parent = bobgui_widget_get_parent (BOBGUI_WIDGET (popover));
  native = bobgui_widget_get_native (parent);

  if (priv->has_pointing_to)
    {
      graphene_matrix_t transform;
      graphene_rect_t pointing_to = GRAPHENE_RECT_INIT (priv->pointing_to.x,
                                                        priv->pointing_to.y,
                                                        priv->pointing_to.width,
                                                        priv->pointing_to.height);

      if (!bobgui_widget_compute_transform (parent, BOBGUI_WIDGET (native), &transform))
        graphene_matrix_init_identity (&transform);

      graphene_matrix_transform_bounds (&transform, &pointing_to, &bounds);
    }
  else
    {
      if (!bobgui_widget_compute_bounds (parent, BOBGUI_WIDGET (native), &bounds))
        g_warning ("Failed to compute bounds");
    }

  bobgui_native_get_surface_transform (native, &nx, &ny);

  rect->x = (int) floor (bounds.origin.x + nx);
  rect->y = (int) floor (bounds.origin.y + ny);
  rect->width = (int) ceilf (bounds.size.width);
  rect->height = (int) ceilf (bounds.size.height);
}

static GdkPopupLayout *
create_popup_layout (BobguiPopover *popover)
{
  BobguiPopoverPrivate *priv = bobgui_popover_get_instance_private (popover);
  GdkRectangle rect;
  GdkGravity parent_anchor;
  GdkGravity surface_anchor;
  GdkAnchorHints anchor_hints;
  GdkPopupLayout *layout;
  BobguiCssStyle *style;
  BobguiBorder shadow_width;
  gboolean ltr = bobgui_widget_get_direction (BOBGUI_WIDGET (popover)) != BOBGUI_TEXT_DIR_RTL;

  compute_surface_pointing_to (popover, &rect);

  style = bobgui_css_node_get_style (bobgui_widget_get_css_node (BOBGUI_WIDGET (priv->contents_widget)));
  bobgui_css_shadow_value_get_extents (style->used->box_shadow, &shadow_width);

  switch (priv->position)
    {
    case BOBGUI_POS_LEFT:
      switch (bobgui_widget_get_valign (BOBGUI_WIDGET (popover)))
        {
        case BOBGUI_ALIGN_START:
          parent_anchor = GDK_GRAVITY_NORTH_WEST;
          surface_anchor = GDK_GRAVITY_NORTH_EAST;
          break;

        case BOBGUI_ALIGN_END:
          parent_anchor = GDK_GRAVITY_SOUTH_WEST;
          surface_anchor = GDK_GRAVITY_SOUTH_EAST;
          break;

        case BOBGUI_ALIGN_FILL:
        case BOBGUI_ALIGN_CENTER:
        case BOBGUI_ALIGN_BASELINE_FILL:
        case BOBGUI_ALIGN_BASELINE_CENTER:
        default:
          parent_anchor = GDK_GRAVITY_WEST;
          surface_anchor = GDK_GRAVITY_EAST;
          break;
        }
      anchor_hints = GDK_ANCHOR_FLIP_X | GDK_ANCHOR_SLIDE_Y;
      break;

    case BOBGUI_POS_RIGHT:
      switch (bobgui_widget_get_valign (BOBGUI_WIDGET (popover)))
        {
        case BOBGUI_ALIGN_START:
          parent_anchor = GDK_GRAVITY_NORTH_EAST;
          surface_anchor = GDK_GRAVITY_NORTH_WEST;
          break;

        case BOBGUI_ALIGN_END:
          parent_anchor = GDK_GRAVITY_SOUTH_EAST;
          surface_anchor = GDK_GRAVITY_SOUTH_WEST;
          break;

        case BOBGUI_ALIGN_FILL:
        case BOBGUI_ALIGN_CENTER:
        case BOBGUI_ALIGN_BASELINE_FILL:
        case BOBGUI_ALIGN_BASELINE_CENTER:
        default:
          parent_anchor = GDK_GRAVITY_EAST;
          surface_anchor = GDK_GRAVITY_WEST;
          break;
        }
      anchor_hints = GDK_ANCHOR_FLIP_X | GDK_ANCHOR_SLIDE_Y;
      break;

    case BOBGUI_POS_TOP:
      switch (bobgui_widget_get_halign (BOBGUI_WIDGET (popover)))
        {
        case BOBGUI_ALIGN_START:
          parent_anchor = ltr ? GDK_GRAVITY_NORTH_WEST : GDK_GRAVITY_NORTH_EAST;
          surface_anchor = ltr ? GDK_GRAVITY_SOUTH_WEST : GDK_GRAVITY_SOUTH_EAST;
          break;

        case BOBGUI_ALIGN_END:
          parent_anchor = ltr ? GDK_GRAVITY_NORTH_EAST : GDK_GRAVITY_NORTH_WEST;
          surface_anchor = ltr ? GDK_GRAVITY_SOUTH_EAST : GDK_GRAVITY_SOUTH_WEST;
          break;

        case BOBGUI_ALIGN_FILL:
        case BOBGUI_ALIGN_CENTER:
        case BOBGUI_ALIGN_BASELINE_FILL:
        case BOBGUI_ALIGN_BASELINE_CENTER:
        default:
          parent_anchor = GDK_GRAVITY_NORTH;
          surface_anchor = GDK_GRAVITY_SOUTH;
          break;
        }
      anchor_hints = GDK_ANCHOR_FLIP_Y | GDK_ANCHOR_SLIDE_X;
      break;

    case BOBGUI_POS_BOTTOM:
      switch (bobgui_widget_get_halign (BOBGUI_WIDGET (popover)))
        {
        case BOBGUI_ALIGN_START:
          parent_anchor = ltr ? GDK_GRAVITY_SOUTH_WEST : GDK_GRAVITY_SOUTH_EAST;
          surface_anchor = ltr ? GDK_GRAVITY_NORTH_WEST : GDK_GRAVITY_NORTH_EAST;
          break;

        case BOBGUI_ALIGN_END:
          parent_anchor = ltr ? GDK_GRAVITY_SOUTH_EAST : GDK_GRAVITY_SOUTH_WEST;
          surface_anchor = ltr ? GDK_GRAVITY_NORTH_EAST : GDK_GRAVITY_NORTH_WEST;
          break;

        case BOBGUI_ALIGN_FILL:
        case BOBGUI_ALIGN_CENTER:
        case BOBGUI_ALIGN_BASELINE_FILL:
        case BOBGUI_ALIGN_BASELINE_CENTER:
        default:
          parent_anchor = GDK_GRAVITY_SOUTH;
          surface_anchor = GDK_GRAVITY_NORTH;
          break;
        }
      anchor_hints = GDK_ANCHOR_FLIP_Y | GDK_ANCHOR_SLIDE_X;
      break;

    default:
      g_assert_not_reached ();
    }

  anchor_hints |= GDK_ANCHOR_RESIZE;

  layout = gdk_popup_layout_new (&rect, parent_anchor, surface_anchor);
  gdk_popup_layout_set_anchor_hints (layout, anchor_hints);
  gdk_popup_layout_set_shadow_width (layout,
                                     shadow_width.left,
                                     shadow_width.right,
                                     shadow_width.top,
                                     shadow_width.bottom);

  if (priv->x_offset || priv->y_offset)
    gdk_popup_layout_set_offset (layout, priv->x_offset, priv->y_offset);

  return layout;
}

static gboolean
present_popup (BobguiPopover *popover)
{
  BobguiPopoverPrivate *priv = bobgui_popover_get_instance_private (popover);
  BobguiRequisition nat;
  GdkPopupLayout *layout;

  layout = create_popup_layout (popover);
  bobgui_widget_get_preferred_size (BOBGUI_WIDGET (popover), NULL, &nat);

  if (gdk_popup_present (GDK_POPUP (priv->surface), nat.width, nat.height, layout))
    {
      update_popover_layout (popover, layout, nat.width, nat.height);
      return TRUE;
    }

  return FALSE;
}

/**
 * bobgui_popover_present:
 * @popover: a `BobguiPopover`
 *
 * Allocate a size for the `BobguiPopover`.
 *
 * This function needs to be called in size-allocate by widgets
 * who have a `BobguiPopover` as child. When using a layout manager,
 * this is happening automatically.
 *
 * To make a popover appear on screen, use [method@Bobgui.Popover.popup].
 */
void
bobgui_popover_present (BobguiPopover *popover)
{
  BobguiWidget *widget = BOBGUI_WIDGET (popover);

  if (!_bobgui_widget_get_alloc_needed (widget))
    bobgui_widget_ensure_allocate (widget);
  else if (bobgui_widget_get_visible (widget))
    present_popup (popover);
}

static void
maybe_request_motion_event (BobguiPopover *popover)
{
  BobguiWidget *widget = BOBGUI_WIDGET (popover);
  BobguiRoot *root = bobgui_widget_get_root (widget);
  GdkSeat *seat;
  GdkDevice *device;
  BobguiWidget *focus;
  GdkSurface *focus_surface;

  seat = gdk_display_get_default_seat (bobgui_widget_get_display (widget));
  if (!seat)
    return;


  device = gdk_seat_get_pointer (seat);
  focus = bobgui_window_lookup_pointer_focus_widget (BOBGUI_WINDOW (root),
                                                  device, NULL);
  if (!focus)
    return;

  if (!bobgui_widget_is_ancestor (focus, BOBGUI_WIDGET (popover)))
    return;

  focus_surface = bobgui_native_get_surface (bobgui_widget_get_native (focus));
  gdk_surface_request_motion (focus_surface);
}

static gboolean
is_acceptable_size (BobguiWidget *widget,
                    int        width,
                    int        height)
{
  if (bobgui_widget_get_request_mode (widget) == BOBGUI_SIZE_REQUEST_WIDTH_FOR_HEIGHT)
    {
      int min_height, min_width_for_height;

      bobgui_widget_measure (widget, BOBGUI_ORIENTATION_VERTICAL, -1,
                          &min_height, NULL, NULL, NULL);
      if (height < min_height)
        return FALSE;
      bobgui_widget_measure (widget, BOBGUI_ORIENTATION_HORIZONTAL, height,
                          &min_width_for_height, NULL, NULL, NULL);
      if (width < min_width_for_height)
        return FALSE;
    }
  else /* BOBGUI_SIZE_REQUEST_HEIGHT_FOR_WIDTH or CONSTANT_SIZE */
    {
      int min_width, min_height_for_width;

      bobgui_widget_measure (widget, BOBGUI_ORIENTATION_HORIZONTAL, -1,
                          &min_width, NULL, NULL, NULL);
      if (width < min_width)
        return FALSE;
      bobgui_widget_measure (widget, BOBGUI_ORIENTATION_VERTICAL, width,
                          &min_height_for_width, NULL, NULL, NULL);
      if (height < min_height_for_width)
        return FALSE;
    }

  return TRUE;
}

static void
bobgui_popover_native_layout (BobguiNative *native,
                           int        width,
                           int        height)
{
  BobguiPopover *popover = BOBGUI_POPOVER (native);
  BobguiPopoverPrivate *priv = bobgui_popover_get_instance_private (popover);
  BobguiWidget *widget = BOBGUI_WIDGET (popover);

  if (!is_acceptable_size (widget, width, height))
    {
      bobgui_popover_popdown (popover);
      return;
    }

  update_popover_layout (popover, gdk_popup_layout_ref (priv->layout), width, height);

  if (bobgui_widget_needs_allocate (widget))
    {
      /* We know the popup's position in the toplevel's coordinate space,
       * so convert it to be relative to the parent widget to define the
       * popover's transform.
       */
      GskTransform *transform = NULL;
      double native_x, native_y;
      BobguiWidget *parent = bobgui_widget_get_parent (widget);
      BobguiWidget *root = BOBGUI_WIDGET (bobgui_widget_get_root (parent));
      graphene_point_t parent_coords;
      if (bobgui_widget_compute_point (parent, root, &GRAPHENE_POINT_INIT (0, 0), &parent_coords))
        {
          transform = gsk_transform_translate (transform,
                                               &GRAPHENE_POINT_INIT (
                                                 priv->final_rect.x - parent_coords.x,
                                                 priv->final_rect.y - parent_coords.y));
        }
      bobgui_native_get_surface_transform (native, &native_x, &native_y);
      transform = gsk_transform_translate (transform,
                                           &GRAPHENE_POINT_INIT (-native_x, -native_y));
      bobgui_widget_allocate (widget, width, height, -1, transform);

      /* This fake motion event is needed for getting up to date pointer focus
       * and coordinates when the pointer didn't move but the layout changed
       * within the popover.
       */
      maybe_request_motion_event (popover);
    }
  else
    {
      bobgui_widget_ensure_allocate (widget);
    }
}

static gboolean
bobgui_popover_has_mnemonic_modifier_pressed (BobguiPopover *popover)
{
  GList *seats, *s;
  gboolean retval = FALSE;

  seats = gdk_display_list_seats (bobgui_widget_get_display (BOBGUI_WIDGET (popover)));

  for (s = seats; s; s = s->next)
    {
      GdkDevice *dev = gdk_seat_get_keyboard (s->data);
      GdkModifierType mask;

      mask = gdk_device_get_modifier_state (dev);
      if ((mask & bobgui_accelerator_get_default_mod_mask ()) == GDK_ALT_MASK)
        {
          retval = TRUE;
          break;
        }
    }

  g_list_free (seats);

  return retval;
}

static gboolean
schedule_mnemonics_visible_cb (gpointer data)
{
  BobguiPopover *popover = data;
  BobguiPopoverPrivate *priv = bobgui_popover_get_instance_private (popover);

  priv->mnemonics_display_timeout_id = 0;

  bobgui_popover_set_mnemonics_visible (popover, TRUE);

  return G_SOURCE_REMOVE;
}

static void
bobgui_popover_schedule_mnemonics_visible (BobguiPopover *popover)
{
  BobguiPopoverPrivate *priv = bobgui_popover_get_instance_private (popover);

  if (priv->mnemonics_display_timeout_id)
    return;

  priv->mnemonics_display_timeout_id =
    g_timeout_add (MNEMONICS_DELAY, schedule_mnemonics_visible_cb, popover);
  gdk_source_set_static_name_by_id (priv->mnemonics_display_timeout_id, "[bobgui] popover_schedule_mnemonics_visible_cb");
}

static void
bobgui_popover_focus_in (BobguiWidget *widget)
{
  BobguiPopover *popover = BOBGUI_POPOVER (widget);

  if (bobgui_widget_get_visible (widget))
    {
      if (bobgui_popover_has_mnemonic_modifier_pressed (popover))
        bobgui_popover_schedule_mnemonics_visible (popover);
    }
}

static void
bobgui_popover_focus_out (BobguiWidget *widget)
{
  BobguiPopover *popover = BOBGUI_POPOVER (widget);

  bobgui_popover_set_mnemonics_visible (popover, FALSE);
}

static void
update_mnemonics_visible (BobguiPopover      *popover,
                          guint            keyval,
                          GdkModifierType  state,
                          gboolean         visible)
{
  if ((keyval == GDK_KEY_Alt_L || keyval == GDK_KEY_Alt_R) &&
      ((state & (bobgui_accelerator_get_default_mod_mask ()) & ~(GDK_ALT_MASK)) == 0))
    {
      if (visible)
        bobgui_popover_schedule_mnemonics_visible (popover);
      else
        bobgui_popover_set_mnemonics_visible (popover, FALSE);
    }
}

static gboolean
bobgui_popover_key_pressed (BobguiWidget       *widget,
                         guint            keyval,
                         guint            keycode,
                         GdkModifierType  state)
{
  BobguiPopover *popover = BOBGUI_POPOVER (widget);
  BobguiWindow *root;

  if (keyval == GDK_KEY_Escape)
    {
      bobgui_popover_popdown (popover);
      return TRUE;
    }

  root = BOBGUI_WINDOW (bobgui_widget_get_root (BOBGUI_WIDGET (popover)));
  _bobgui_window_update_focus_visible (root, keyval, state, TRUE);
  update_mnemonics_visible (popover, keyval, state, TRUE);

  return FALSE;
}

static gboolean
bobgui_popover_key_released (BobguiWidget       *widget,
                          guint            keyval,
                          guint            keycode,
                          GdkModifierType  state)
{
  BobguiPopover *popover = BOBGUI_POPOVER (widget);
  BobguiWindow *root;

  root = BOBGUI_WINDOW (bobgui_widget_get_root (BOBGUI_WIDGET (popover)));
  _bobgui_window_update_focus_visible (root, keyval, state, FALSE);
  update_mnemonics_visible (popover, keyval, state, FALSE);

  return FALSE;
}

static void
surface_mapped_changed (BobguiWidget *widget)
{
  BobguiPopover *popover = BOBGUI_POPOVER (widget);
  BobguiPopoverPrivate *priv = bobgui_popover_get_instance_private (popover);

  bobgui_widget_set_visible (widget, gdk_surface_get_mapped (priv->surface));
}

static gboolean
surface_render (GdkSurface     *surface,
                cairo_region_t *region,
                BobguiWidget      *widget)
{
  bobgui_widget_render (widget, surface, region);
  return TRUE;
}

static gboolean
surface_event (GdkSurface *surface,
               GdkEvent   *event,
               BobguiWidget  *widget)
{
  bobgui_main_do_event (event);
  return TRUE;
}

static void
bobgui_popover_activate_default (BobguiPopover *popover)
{
  BobguiPopoverPrivate *priv = bobgui_popover_get_instance_private (popover);
  BobguiWidget *focus_widget;

  focus_widget = bobgui_window_get_focus (BOBGUI_WINDOW (bobgui_widget_get_root (BOBGUI_WIDGET (popover))));
  if (!bobgui_widget_is_ancestor (focus_widget, BOBGUI_WIDGET (popover)))
    focus_widget = NULL;

  if (priv->default_widget && bobgui_widget_is_sensitive (priv->default_widget) &&
      (!focus_widget || !bobgui_widget_get_receives_default (focus_widget)
))
    bobgui_widget_activate (priv->default_widget);
  else if (focus_widget && bobgui_widget_is_sensitive (focus_widget))
    bobgui_widget_activate (focus_widget);
}

static void
activate_default_cb (GSimpleAction *action,
                     GVariant      *parameter,
                     gpointer       data)
{
  bobgui_popover_activate_default (BOBGUI_POPOVER (data));
}

static void
add_actions (BobguiPopover *popover)
{
  GActionEntry entries[] = {
    { "activate", activate_default_cb, NULL, NULL, NULL },
  };

  GActionGroup *actions;

  actions = G_ACTION_GROUP (g_simple_action_group_new ());
  g_action_map_add_action_entries (G_ACTION_MAP (actions),
                                   entries, G_N_ELEMENTS (entries),
                                   popover);
  bobgui_widget_insert_action_group (BOBGUI_WIDGET (popover), "default", actions);
  g_object_unref (actions);
}

static void
node_style_changed_cb (BobguiCssNode        *node,
                       BobguiCssStyleChange *change,
                       BobguiWidget         *widget)
{
  BobguiPopoverPrivate *priv = bobgui_popover_get_instance_private (BOBGUI_POPOVER (widget));
  g_clear_pointer (&priv->arrow_render_node, gsk_render_node_unref);

  if (bobgui_css_style_change_affects (change, BOBGUI_CSS_AFFECTS_SIZE))
    bobgui_widget_queue_resize (widget);
  else
    bobgui_widget_queue_draw (widget);
}

static void
bobgui_popover_init (BobguiPopover *popover)
{
  BobguiPopoverPrivate *priv = bobgui_popover_get_instance_private (popover);
  BobguiWidget *widget = BOBGUI_WIDGET (popover);
  BobguiEventController *controller;

  priv->position = BOBGUI_POS_BOTTOM;
  priv->final_position = BOBGUI_POS_BOTTOM;
  priv->autohide = TRUE;
  priv->has_arrow = TRUE;
  priv->cascade_popdown = FALSE;

  controller = bobgui_event_controller_key_new ();
  g_signal_connect_swapped (controller, "key-pressed", G_CALLBACK (bobgui_popover_key_pressed), popover);
  g_signal_connect_swapped (controller, "key-released", G_CALLBACK (bobgui_popover_key_released), popover);
  bobgui_widget_add_controller (BOBGUI_WIDGET (popover), controller);

  controller = bobgui_event_controller_focus_new ();
  g_signal_connect_swapped (controller, "enter", G_CALLBACK (bobgui_popover_focus_in), popover);
  g_signal_connect_swapped (controller, "leave", G_CALLBACK (bobgui_popover_focus_out), popover);
  bobgui_widget_add_controller (widget, controller);

  priv->arrow_node = bobgui_css_node_new ();
  bobgui_css_node_set_name (priv->arrow_node, g_quark_from_static_string ("arrow"));
  bobgui_css_node_set_parent (priv->arrow_node, bobgui_widget_get_css_node (widget));
  bobgui_css_node_set_state (priv->arrow_node,
                          bobgui_css_node_get_state (bobgui_widget_get_css_node (widget)));
  g_signal_connect_object (priv->arrow_node, "style-changed",
                           G_CALLBACK (node_style_changed_cb), popover, 0);
  g_object_unref (priv->arrow_node);

  priv->contents_widget = bobgui_popover_content_new ();

  bobgui_widget_set_layout_manager (priv->contents_widget, bobgui_bin_layout_new ());
  bobgui_widget_set_parent (priv->contents_widget, BOBGUI_WIDGET (popover));
  bobgui_widget_set_overflow (priv->contents_widget, BOBGUI_OVERFLOW_HIDDEN);

  bobgui_widget_add_css_class (widget, "background");

  add_actions (popover);
}

static void
bobgui_popover_realize (BobguiWidget *widget)
{
  BobguiPopover *popover = BOBGUI_POPOVER (widget);
  BobguiPopoverPrivate *priv = bobgui_popover_get_instance_private (popover);
  GdkSurface *parent_surface;
  BobguiWidget *parent;

  parent = bobgui_widget_get_parent (widget);
  parent_surface = bobgui_native_get_surface (bobgui_widget_get_native (parent));
  priv->surface = gdk_surface_new_popup (parent_surface, priv->autohide);

  gdk_surface_set_widget (priv->surface, widget);

  g_signal_connect_swapped (priv->surface, "notify::mapped", G_CALLBACK (surface_mapped_changed), widget);
  g_signal_connect (priv->surface, "render", G_CALLBACK (surface_render), widget);
  g_signal_connect (priv->surface, "event", G_CALLBACK (surface_event), widget);

  BOBGUI_WIDGET_CLASS (bobgui_popover_parent_class)->realize (widget);

  priv->renderer = gsk_renderer_new_for_surface_full (priv->surface, TRUE);

  bobgui_native_realize (BOBGUI_NATIVE (popover));
}

static void
bobgui_popover_unrealize (BobguiWidget *widget)
{
  BobguiPopover *popover = BOBGUI_POPOVER (widget);
  BobguiPopoverPrivate *priv = bobgui_popover_get_instance_private (popover);

  bobgui_native_unrealize (BOBGUI_NATIVE (popover));

  BOBGUI_WIDGET_CLASS (bobgui_popover_parent_class)->unrealize (widget);

  gsk_renderer_unrealize (priv->renderer);
  g_clear_object (&priv->renderer);

  g_signal_handlers_disconnect_by_func (priv->surface, surface_mapped_changed, widget);
  g_signal_handlers_disconnect_by_func (priv->surface, surface_render, widget);
  g_signal_handlers_disconnect_by_func (priv->surface, surface_event, widget);
  gdk_surface_set_widget (priv->surface, NULL);
  g_clear_pointer (&priv->surface, gdk_surface_destroy);
}

static gboolean
bobgui_popover_focus (BobguiWidget        *widget,
                   BobguiDirectionType  direction)
{
  if (!bobgui_widget_get_visible (widget))
    return FALSE;

  /* This code initially comes from bobguipopovermenu.c */
  if (bobgui_widget_get_first_child (widget) == NULL)
    {
      /* Empty popover, so nothing to Tab through. */
      return FALSE;
    }
  else
    {
      /* Move focus normally, but when nothing can be focused in this direction then we cycle around. */
      if (bobgui_widget_focus_move (widget, direction))
        return TRUE;

      if (bobgui_popover_get_autohide (BOBGUI_POPOVER (widget)))
        {
          BobguiWidget *p = bobgui_root_get_focus (bobgui_widget_get_root (widget));

          /* In the case where the popover doesn't have any focusable child (like
           * the BobguiTreePopover for combo boxes) then the focus will end up out of
           * the popover, hence creating an infinite loop below. To avoid this, just
           * say we had focus and stop here.
           */
          if (!bobgui_widget_is_ancestor (p, widget) && p != widget)
            return TRUE;

          /* Cycle around with (Shift+)Tab */
          if (direction == BOBGUI_DIR_TAB_FORWARD || direction == BOBGUI_DIR_TAB_BACKWARD)
            {
              for (;
                   p != widget;
                   p = bobgui_widget_get_parent (p))
                {
                  /* Unfocus everything in the popover. */
                  bobgui_widget_set_focus_child (p, NULL);
                }
            }
          /* Focus again from scratch */
          bobgui_widget_focus_move (widget, direction);
          return TRUE;
        }
      else
        {
          return FALSE;
        }
    }
}

static void
bobgui_popover_show (BobguiWidget *widget)
{
  BobguiPopover *popover = BOBGUI_POPOVER (widget);
  BobguiPopoverPrivate *priv = bobgui_popover_get_instance_private (popover);

  _bobgui_widget_set_visible_flag (widget, TRUE);
  bobgui_widget_realize (widget);
  if (!present_popup (popover))
    return;

  bobgui_widget_map (widget);

  if (priv->autohide)
    {
      if (!bobgui_widget_get_focus_child (widget))
        bobgui_widget_child_focus (widget, BOBGUI_DIR_TAB_FORWARD);
    }
}

static void
bobgui_popover_hide (BobguiWidget *widget)
{
  bobgui_popover_set_mnemonics_visible (BOBGUI_POPOVER (widget), FALSE);
  _bobgui_widget_set_visible_flag (widget, FALSE);
  bobgui_widget_unmap (widget);
  g_signal_emit (widget, signals[CLOSED], 0);
}

static void
unset_surface_transform_changed_cb (gpointer data)
{
  BobguiPopover *popover = data;
  BobguiPopoverPrivate *priv = bobgui_popover_get_instance_private (popover);

  priv->surface_transform_changed_cb = 0;
}

static gboolean
surface_transform_changed_cb (BobguiWidget               *widget,
                              const graphene_matrix_t *transform,
                              gpointer                 user_data)
{
  BobguiPopover *popover = user_data;
  BobguiPopoverPrivate *priv = bobgui_popover_get_instance_private (popover);

  if (priv->surface && gdk_surface_get_mapped (priv->surface))
    present_popup (popover);

  return G_SOURCE_CONTINUE;
}

static void
bobgui_popover_map (BobguiWidget *widget)
{
  BobguiPopover *popover = BOBGUI_POPOVER (widget);
  BobguiPopoverPrivate *priv = bobgui_popover_get_instance_private (popover);
  BobguiWidget *parent;

  present_popup (popover);

  parent = bobgui_widget_get_parent (widget);
  priv->surface_transform_changed_cb =
    bobgui_widget_add_surface_transform_changed_callback (parent,
                                                       surface_transform_changed_cb,
                                                       popover,
                                                       unset_surface_transform_changed_cb);

  BOBGUI_WIDGET_CLASS (bobgui_popover_parent_class)->map (widget);

  if (priv->autohide)
    bobgui_grab_add (widget);
}

static void
bobgui_popover_unmap (BobguiWidget *widget)
{
  BobguiPopover *popover = BOBGUI_POPOVER (widget);
  BobguiPopoverPrivate *priv = bobgui_popover_get_instance_private (popover);
  BobguiWidget *parent;

  if (priv->autohide)
    bobgui_grab_remove (widget);

  parent = bobgui_widget_get_parent (widget);
  bobgui_widget_remove_surface_transform_changed_callback (parent,
                                                        priv->surface_transform_changed_cb);
  priv->surface_transform_changed_cb = 0;

  BOBGUI_WIDGET_CLASS (bobgui_popover_parent_class)->unmap (widget);
  bobgui_tooltip_unset_surface (BOBGUI_NATIVE (popover));
  gdk_surface_hide (priv->surface);
}

static void
bobgui_popover_dispose (GObject *object)
{
  BobguiPopover *popover = BOBGUI_POPOVER (object);
  BobguiPopoverPrivate *priv = bobgui_popover_get_instance_private (popover);

  g_clear_object (&priv->default_widget);
  g_clear_pointer (&priv->contents_widget, bobgui_widget_unparent);
  g_clear_pointer (&priv->arrow_render_node, gsk_render_node_unref);

  G_OBJECT_CLASS (bobgui_popover_parent_class)->dispose (object);
}

static void
bobgui_popover_finalize (GObject *object)
{
  BobguiPopover *popover = BOBGUI_POPOVER (object);
  BobguiPopoverPrivate *priv = bobgui_popover_get_instance_private (popover);

  g_clear_pointer (&priv->layout, gdk_popup_layout_unref);

  if (priv->mnemonics_display_timeout_id)
    {
      g_source_remove (priv->mnemonics_display_timeout_id);
      priv->mnemonics_display_timeout_id = 0;
    }

  G_OBJECT_CLASS (bobgui_popover_parent_class)->finalize (object);
}

static double
get_border_radius (BobguiWidget *widget)
{
  BobguiCssStyle *style = bobgui_css_node_get_style (bobgui_widget_get_css_node (widget));

  /* FIXME this is a very crude interpretation of border radius */
  return MAX (_bobgui_css_corner_value_get_x (style->border->border_top_left_radius, 100),
              _bobgui_css_corner_value_get_y (style->border->border_top_left_radius, 100));
}

static void
bobgui_popover_get_gap_coords (BobguiPopover *popover,
                            int        *initial_x_out,
                            int        *initial_y_out,
                            int        *tip_x_out,
                            int        *tip_y_out,
                            int        *final_x_out,
                            int        *final_y_out)
{
  BobguiWidget *widget = BOBGUI_WIDGET (popover);
  BobguiPopoverPrivate *priv = bobgui_popover_get_instance_private (popover);
  GdkRectangle rect = { 0 };
  int base, tip, tip_pos;
  int initial_x, initial_y;
  int tip_x, tip_y;
  int final_x, final_y;
  BobguiPositionType pos;
  int border_top, border_right, border_bottom;
  int border_radius;
  int popover_width, popover_height;
  BobguiCssStyle *style;
  BobguiBorder shadow_width;

  popover_width = bobgui_widget_get_width (widget);
  popover_height = bobgui_widget_get_height (widget);

  compute_surface_pointing_to (popover, &rect);

  rect.x -= priv->final_rect.x;
  rect.y -= priv->final_rect.y;

  pos = priv->final_position;

  style = bobgui_css_node_get_style (bobgui_widget_get_css_node (priv->contents_widget));
  border_radius = round (get_border_radius (widget));
  border_top = bobgui_css_number_value_get (style->border->border_top_width, 100);
  border_right = bobgui_css_number_value_get (style->border->border_right_width, 100);
  border_bottom = bobgui_css_number_value_get (style->border->border_bottom_width, 100);

  bobgui_css_shadow_value_get_extents (style->used->box_shadow, &shadow_width);

  if (pos == BOBGUI_POS_BOTTOM)
    {
      tip = shadow_width.top;
      base = tip + TAIL_HEIGHT + border_top;
    }
  else if (pos == BOBGUI_POS_RIGHT)
    {
      tip = shadow_width.left;
      base = tip + TAIL_HEIGHT + border_top;
    }
  else if (pos == BOBGUI_POS_TOP)
    {
      tip = popover_height - shadow_width.bottom;
      base = tip - border_bottom - TAIL_HEIGHT;
    }
  else if (pos == BOBGUI_POS_LEFT)
    {
      tip = popover_width - shadow_width.right;
      base = tip - border_right - TAIL_HEIGHT;
    }
  else
    g_assert_not_reached ();

  if (POS_IS_VERTICAL (pos))
    {
      tip_pos = rect.x + (rect.width / 2);
      initial_x = CLAMP (tip_pos - TAIL_GAP_WIDTH / 2,
                         border_radius,
                         popover_width - TAIL_GAP_WIDTH - border_radius);
      initial_y = base;

      tip_x = CLAMP (tip_pos, 0, popover_width);
      tip_y = tip;

      final_x = CLAMP (tip_pos + TAIL_GAP_WIDTH / 2,
                       border_radius + TAIL_GAP_WIDTH,
                       popover_width - border_radius);
      final_y = base;
    }
  else
    {
      tip_pos = rect.y + (rect.height / 2);

      initial_x = base;
      initial_y = CLAMP (tip_pos - TAIL_GAP_WIDTH / 2,
                         border_radius,
                         popover_height - TAIL_GAP_WIDTH - border_radius);

      tip_x = tip;
      tip_y = CLAMP (tip_pos, 0, popover_height);

      final_x = base;
      final_y = CLAMP (tip_pos + TAIL_GAP_WIDTH / 2,
                       border_radius + TAIL_GAP_WIDTH,
                       popover_height - border_radius);
    }

  *initial_x_out = initial_x;
  *initial_y_out = initial_y;

  *tip_x_out = tip_x;
  *tip_y_out = tip_y;

  *final_x_out = final_x;
  *final_y_out = final_y;
}

static void
get_border (BobguiCssNode *node,
            BobguiBorder *border)
{
  BobguiCssStyle *style;

  style = bobgui_css_node_get_style (node);

  border->top = bobgui_css_number_value_get (style->border->border_top_width, 100);
  border->right = bobgui_css_number_value_get (style->border->border_right_width, 100);
  border->bottom = bobgui_css_number_value_get (style->border->border_bottom_width, 100);
  border->left = bobgui_css_number_value_get (style->border->border_left_width, 100);
}

static void
bobgui_popover_apply_tail_path (BobguiPopover *popover,
                             cairo_t    *cr)
{
  BobguiPopoverPrivate *priv = bobgui_popover_get_instance_private (popover);
  int initial_x, initial_y;
  int tip_x, tip_y;
  int final_x, final_y;
  BobguiBorder border;
  BobguiWidget *parent;

  parent = bobgui_widget_get_parent (BOBGUI_WIDGET (popover));

  if (!parent)
    return;

  get_border (priv->arrow_node, &border);

  cairo_set_line_width (cr, 1);
  bobgui_popover_get_gap_coords (popover,
                              &initial_x, &initial_y,
                              &tip_x, &tip_y,
                              &final_x, &final_y);

  cairo_move_to (cr, initial_x, initial_y);
  cairo_line_to (cr, tip_x, tip_y);
  cairo_line_to (cr, final_x, final_y);
}

static void
bobgui_popover_update_shape (BobguiPopover *popover)
{
  BobguiPopoverPrivate *priv = bobgui_popover_get_instance_private (popover);

  if (priv->has_arrow)
    {
      BobguiCssBoxes content_css_boxes;
      const GskRoundedRect *box;
      cairo_surface_t *cairo_surface;
      cairo_region_t *region;
      cairo_t *cr;
      graphene_point_t p;
      double native_x, native_y;
      int width, height, scale;

      bobgui_native_get_surface_transform (BOBGUI_NATIVE (popover), &native_x, &native_y);
      bobgui_css_boxes_init (&content_css_boxes, priv->contents_widget);

      width = gdk_surface_get_width (priv->surface);
      height = gdk_surface_get_height (priv->surface);
      scale = gdk_surface_get_scale_factor (priv->surface);

      cairo_surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width * scale, height * scale);
      cairo_surface_set_device_scale (cairo_surface, scale, scale);

      cr = cairo_create (cairo_surface);

      cairo_translate (cr, native_x, native_y);

      cairo_set_source_rgba (cr, 0, 0, 0, 1);
      bobgui_popover_apply_tail_path (popover, cr);
      cairo_close_path (cr);
      cairo_fill (cr);

      box = bobgui_css_boxes_get_border_box (&content_css_boxes);
      if (!bobgui_widget_compute_point (priv->contents_widget, BOBGUI_WIDGET (popover),
                                     &GRAPHENE_POINT_INIT (0, 0), &p))
        graphene_point_init (&p, 0, 0);

      cairo_translate (cr, p.x, p.y);
      gsk_rounded_rect_path (box, cr);
      cairo_fill (cr);
      cairo_destroy (cr);

      region = gdk_cairo_region_create_from_surface (cairo_surface);
      cairo_surface_destroy (cairo_surface);

      gdk_surface_set_input_region (priv->surface, region);
      cairo_region_destroy (region);
    }
  else
    {
      BobguiCssNode *content_css_node;
      BobguiCssStyle *style;
      BobguiBorder shadow_width;
      cairo_rectangle_int_t input_rect;
      cairo_region_t *region;

      content_css_node =
        bobgui_widget_get_css_node (BOBGUI_WIDGET (priv->contents_widget));
      style = bobgui_css_node_get_style (content_css_node);
      bobgui_css_shadow_value_get_extents (style->used->box_shadow, &shadow_width);

      input_rect.x = shadow_width.left;
      input_rect.y = shadow_width.top;
      input_rect.width =
        gdk_surface_get_width (priv->surface) -
        (shadow_width.left + shadow_width.right);
      input_rect.height =
        gdk_surface_get_height (priv->surface) -
        (shadow_width.top + shadow_width.bottom);

      region = cairo_region_create_rectangle (&input_rect);
      gdk_surface_set_input_region (priv->surface, region);
      cairo_region_destroy (region);
    }
}

static int
get_minimal_size (BobguiPopover     *popover,
                  BobguiOrientation  orientation)
{
  BobguiPopoverPrivate *priv = bobgui_popover_get_instance_private (popover);
  BobguiPositionType pos;
  int minimal_size;
  int tail_gap_width = priv->has_arrow ? TAIL_GAP_WIDTH : 0;
  int min_width, min_height;

  minimal_size = 2 * round (get_border_radius (BOBGUI_WIDGET (priv->contents_widget)));
  pos = priv->position;

  if ((orientation == BOBGUI_ORIENTATION_HORIZONTAL && POS_IS_VERTICAL (pos)) ||
      (orientation == BOBGUI_ORIENTATION_VERTICAL && !POS_IS_VERTICAL (pos)))
    minimal_size += tail_gap_width;

  bobgui_widget_get_size_request (BOBGUI_WIDGET (popover), &min_width, &min_height);

  if (orientation == BOBGUI_ORIENTATION_HORIZONTAL)
    minimal_size = MAX (minimal_size, min_width);
  else
    minimal_size = MAX (minimal_size, min_height);

  return minimal_size;
}

static void
bobgui_popover_measure (BobguiWidget      *widget,
                     BobguiOrientation  orientation,
                     int             for_size,
                     int            *minimum,
                     int            *natural,
                     int            *minimum_baseline,
                     int            *natural_baseline)
{
  BobguiPopover *popover = BOBGUI_POPOVER (widget);
  BobguiPopoverPrivate *priv = bobgui_popover_get_instance_private (popover);
  int minimal_size;
  int tail_height = priv->has_arrow ? TAIL_HEIGHT : 0;
  BobguiCssStyle *style;
  BobguiBorder shadow_width;

  style = bobgui_css_node_get_style (bobgui_widget_get_css_node (BOBGUI_WIDGET (priv->contents_widget)));
  bobgui_css_shadow_value_get_extents (style->used->box_shadow, &shadow_width);

  if (for_size >= 0)
    {
      if ((POS_IS_VERTICAL (priv->position) == (orientation == BOBGUI_ORIENTATION_HORIZONTAL)))
        for_size -= tail_height;

      if (orientation == BOBGUI_ORIENTATION_HORIZONTAL)
        for_size -= shadow_width.top + shadow_width.bottom;
      else
        for_size -= shadow_width.left + shadow_width.right;
    }

  bobgui_widget_measure (priv->contents_widget,
                      orientation, for_size,
                      minimum, natural,
                      minimum_baseline, natural_baseline);

  minimal_size = get_minimal_size (popover, orientation);
  *minimum = MAX (*minimum, minimal_size);
  *natural = MAX (*natural, minimal_size);

  if (orientation == BOBGUI_ORIENTATION_HORIZONTAL)
    {
      *minimum += shadow_width.left + shadow_width.right;
      *natural += shadow_width.left + shadow_width.right;
    }
  else
    {
      *minimum += shadow_width.top + shadow_width.bottom;
      *natural += shadow_width.top + shadow_width.bottom;
    }

  if (POS_IS_VERTICAL (priv->position) == (orientation == BOBGUI_ORIENTATION_VERTICAL))
    {
      *minimum += tail_height;
      *natural += tail_height;
    }
}

static void
bobgui_popover_size_allocate (BobguiWidget *widget,
                           int        width,
                           int        height,
                           int        baseline)
{
  BobguiPopover *popover = BOBGUI_POPOVER (widget);
  BobguiPopoverPrivate *priv = bobgui_popover_get_instance_private (popover);
  BobguiAllocation child_alloc;
  int tail_height = priv->has_arrow ? TAIL_HEIGHT : 0;
  BobguiCssStyle *style;
  BobguiBorder shadow_width;

  style = bobgui_css_node_get_style (bobgui_widget_get_css_node (BOBGUI_WIDGET (priv->contents_widget)));
  bobgui_css_shadow_value_get_extents (style->used->box_shadow, &shadow_width);

  switch (priv->final_position)
    {
    case BOBGUI_POS_TOP:
      child_alloc.x = shadow_width.left;
      child_alloc.y = shadow_width.top;
      child_alloc.width = width - (shadow_width.left + shadow_width.right);
      child_alloc.height = height - (shadow_width.top + shadow_width.bottom + tail_height);
      break;
    case BOBGUI_POS_BOTTOM:
      child_alloc.x = shadow_width.left;
      child_alloc.y = shadow_width.top + tail_height;
      child_alloc.width = width - (shadow_width.left + shadow_width.right);
      child_alloc.height = height - (shadow_width.top + shadow_width.bottom + tail_height);
      break;
    case BOBGUI_POS_LEFT:
      child_alloc.x = shadow_width.left;
      child_alloc.y = shadow_width.top;
      child_alloc.width = width - (shadow_width.left + shadow_width.right + tail_height);
      child_alloc.height = height - (shadow_width.top + shadow_width.bottom);
      break;
    case BOBGUI_POS_RIGHT:
      child_alloc.x = shadow_width.left + tail_height;
      child_alloc.y = shadow_width.top;
      child_alloc.width = width - (shadow_width.left + shadow_width.right + tail_height);
      child_alloc.height = height - (shadow_width.top + shadow_width.bottom);
      break;
    default:
      break;
    }

  bobgui_widget_size_allocate (priv->contents_widget, &child_alloc, baseline);

  if (priv->surface)
    {
      bobgui_popover_update_shape (popover);
      g_clear_pointer (&priv->arrow_render_node, gsk_render_node_unref);
    }

  bobgui_tooltip_maybe_allocate (BOBGUI_NATIVE (popover));
}

static void
create_arrow_render_node (BobguiPopover *popover)
{
  BobguiPopoverPrivate *priv = bobgui_popover_get_instance_private (popover);
  BobguiWidget *widget = BOBGUI_WIDGET (popover);
  BobguiBorder border;
  cairo_t *cr;
  BobguiSnapshot *snapshot;
  BobguiSnapshot *bg_snapshot;
  GskRenderNode *node;
  BobguiCssBoxes boxes;
  BobguiCssStyle *style;

  snapshot = bobgui_snapshot_new ();

  cr = bobgui_snapshot_append_cairo (snapshot,
                                  &GRAPHENE_RECT_INIT (
                                    0, 0,
                                    bobgui_widget_get_width (widget),
                                    bobgui_widget_get_height (widget)
                                  ));

  /* Clip to the arrow shape */
  cairo_save (cr);
  bobgui_popover_apply_tail_path (popover, cr);
  cairo_clip (cr);

  get_border (priv->arrow_node, &border);

  style = bobgui_css_node_get_style (priv->arrow_node);

  /* Render the arrow background */
  bg_snapshot = bobgui_snapshot_new ();
  bobgui_css_boxes_init_border_box (&boxes, style,
                                 0, 0,
                                 bobgui_widget_get_width (widget),
                                 bobgui_widget_get_height (widget));
  bobgui_css_style_snapshot_background (&boxes, bg_snapshot);
  node = bobgui_snapshot_free_to_node (bg_snapshot);
  if (node)
    {
      gsk_render_node_draw (node, cr);
      gsk_render_node_unref (node);
    }

  /* Render the border of the arrow tip */
  if (border.bottom > 0)
    {
      const GdkRGBA *border_color;

      border_color = bobgui_css_color_value_get_rgba (style->used->border_left_color);

      bobgui_popover_apply_tail_path (popover, cr);
      gdk_cairo_set_source_rgba (cr, border_color);

      cairo_set_line_width (cr, border.bottom + 1);
      cairo_stroke (cr);
    }

  cairo_restore (cr);
  cairo_destroy (cr);

  priv->arrow_render_node = bobgui_snapshot_free_to_node (snapshot);
}

static void
bobgui_popover_snapshot (BobguiWidget   *widget,
                      BobguiSnapshot *snapshot)
{
  BobguiPopover *popover = BOBGUI_POPOVER (widget);
  BobguiPopoverPrivate *priv = bobgui_popover_get_instance_private (popover);

  bobgui_widget_snapshot_child (widget, priv->contents_widget, snapshot);

  if (priv->has_arrow)
    {
      if (!priv->arrow_render_node)
        create_arrow_render_node (popover);

      bobgui_snapshot_append_node (snapshot, priv->arrow_render_node);
    }
}

static void
bobgui_popover_set_property (GObject      *object,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  BobguiPopover *popover = BOBGUI_POPOVER (object);

  switch (prop_id)
    {
    case PROP_POINTING_TO:
      bobgui_popover_set_pointing_to (popover, g_value_get_boxed (value));
      break;

    case PROP_POSITION:
      bobgui_popover_set_position (popover, g_value_get_enum (value));
      break;

    case PROP_AUTOHIDE:
      bobgui_popover_set_autohide (popover, g_value_get_boolean (value));
      break;

    case PROP_DEFAULT_WIDGET:
      bobgui_popover_set_default_widget (popover, g_value_get_object (value));
      break;

    case PROP_HAS_ARROW:
      bobgui_popover_set_has_arrow (popover, g_value_get_boolean (value));
      break;

    case PROP_MNEMONICS_VISIBLE:
      bobgui_popover_set_mnemonics_visible (popover, g_value_get_boolean (value));
      break;

    case PROP_CHILD:
      bobgui_popover_set_child (popover, g_value_get_object (value));
      break;

    case PROP_CASCADE_POPDOWN:
      bobgui_popover_set_cascade_popdown (popover, g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_popover_get_property (GObject      *object,
                          guint         prop_id,
                          GValue       *value,
                          GParamSpec   *pspec)
{
  BobguiPopover *popover = BOBGUI_POPOVER (object);
  BobguiPopoverPrivate *priv = bobgui_popover_get_instance_private (popover);

  switch (prop_id)
    {
    case PROP_POINTING_TO:
      g_value_set_boxed (value, &priv->pointing_to);
      break;

    case PROP_POSITION:
      g_value_set_enum (value, priv->position);
      break;

    case PROP_AUTOHIDE:
      g_value_set_boolean (value, priv->autohide);
      break;

    case PROP_DEFAULT_WIDGET:
      g_value_set_object (value, priv->default_widget);
      break;

    case PROP_HAS_ARROW:
      g_value_set_boolean (value, priv->has_arrow);
      break;

    case PROP_MNEMONICS_VISIBLE:
      g_value_set_boolean (value, priv->mnemonics_visible);
      break;

    case PROP_CHILD:
      g_value_set_object (value, bobgui_popover_get_child (popover));
      break;

    case PROP_CASCADE_POPDOWN:
      g_value_set_boolean (value, priv->cascade_popdown);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
add_tab_bindings (BobguiWidgetClass   *widget_class,
                  GdkModifierType   modifiers,
                  BobguiDirectionType  direction)
{
  bobgui_widget_class_add_binding_signal (widget_class, GDK_KEY_Tab, modifiers,
                                       "move-focus",
                                       "(i)", direction);
  bobgui_widget_class_add_binding_signal (widget_class, GDK_KEY_KP_Tab, modifiers,
                                       "move-focus",
                                       "(i)", direction);
}

static void
add_arrow_bindings (BobguiWidgetClass   *widget_class,
                    guint             keysym,
                    BobguiDirectionType  direction)
{
  guint keypad_keysym = keysym - GDK_KEY_Left + GDK_KEY_KP_Left;

  bobgui_widget_class_add_binding_signal (widget_class, keysym, 0,
                                       "move-focus",
                                       "(i)",
                                       direction);
  bobgui_widget_class_add_binding_signal (widget_class, keysym, GDK_CONTROL_MASK,
                                       "move-focus",
                                       "(i)",
                                       direction);
  bobgui_widget_class_add_binding_signal (widget_class, keypad_keysym, 0,
                                       "move-focus",
                                       "(i)",
                                       direction);
  bobgui_widget_class_add_binding_signal (widget_class, keypad_keysym, GDK_CONTROL_MASK,
                                       "move-focus",
                                       "(i)",
                                       direction);
}

static void
bobgui_popover_compute_expand (BobguiWidget *widget,
                            gboolean  *hexpand,
                            gboolean  *vexpand)
{
  BobguiPopover *popover = BOBGUI_POPOVER (widget);
  BobguiPopoverPrivate *priv = bobgui_popover_get_instance_private (popover);

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
bobgui_popover_get_request_mode (BobguiWidget *widget)
{
  BobguiPopover *popover = BOBGUI_POPOVER (widget);
  BobguiPopoverPrivate *priv = bobgui_popover_get_instance_private (popover);

  if (priv->child)
    return bobgui_widget_get_request_mode (priv->child);
  else
    return BOBGUI_SIZE_REQUEST_CONSTANT_SIZE;
}

static void
bobgui_popover_class_init (BobguiPopoverClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);

  object_class->dispose = bobgui_popover_dispose;
  object_class->finalize = bobgui_popover_finalize;
  object_class->set_property = bobgui_popover_set_property;
  object_class->get_property = bobgui_popover_get_property;

  widget_class->realize = bobgui_popover_realize;
  widget_class->unrealize = bobgui_popover_unrealize;
  widget_class->map = bobgui_popover_map;
  widget_class->unmap = bobgui_popover_unmap;
  widget_class->focus = bobgui_popover_focus;
  widget_class->show = bobgui_popover_show;
  widget_class->hide = bobgui_popover_hide;
  widget_class->measure = bobgui_popover_measure;
  widget_class->size_allocate = bobgui_popover_size_allocate;
  widget_class->snapshot = bobgui_popover_snapshot;
  widget_class->compute_expand = bobgui_popover_compute_expand;
  widget_class->get_request_mode = bobgui_popover_get_request_mode;

  klass->activate_default = bobgui_popover_activate_default;

  /**
   * BobguiPopover:pointing-to:
   *
   * Rectangle in the parent widget that the popover points to.
   */
  properties[PROP_POINTING_TO] =
      g_param_spec_boxed ("pointing-to", NULL, NULL,
                          GDK_TYPE_RECTANGLE,
                          BOBGUI_PARAM_READWRITE);

  /**
   * BobguiPopover:position:
   *
   * How to place the popover, relative to its parent.
   */
  properties[PROP_POSITION] =
      g_param_spec_enum ("position", NULL, NULL,
                         BOBGUI_TYPE_POSITION_TYPE, BOBGUI_POS_BOTTOM,
                         BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiPopover:autohide:
   *
   * Whether to dismiss the popover on outside clicks.
   */
  properties[PROP_AUTOHIDE] =
      g_param_spec_boolean ("autohide", NULL, NULL,
                            TRUE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiPopover:default-widget:
   *
   * The default widget inside the popover.
   */
  properties[PROP_DEFAULT_WIDGET] =
      g_param_spec_object ("default-widget", NULL, NULL,
                           BOBGUI_TYPE_WIDGET,
                           BOBGUI_PARAM_READWRITE|G_PARAM_STATIC_STRINGS|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiPopover:has-arrow:
   *
   * Whether to draw an arrow.
   */
  properties[PROP_HAS_ARROW] =
      g_param_spec_boolean ("has-arrow", NULL, NULL,
                            TRUE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiPopover:mnemonics-visible:
   *
   * Whether mnemonics are currently visible in this popover.
   */
  properties[PROP_MNEMONICS_VISIBLE] =
      g_param_spec_boolean ("mnemonics-visible", NULL, NULL,
                            FALSE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiPopover:child:
   *
   * The child widget.
   */
  properties[PROP_CHILD] =
      g_param_spec_object ("child", NULL, NULL,
                           BOBGUI_TYPE_WIDGET,
                           BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiPopover:cascade-popdown:
   *
   * Whether the popover pops down after a child popover.
   *
   * This is used to implement the expected behavior of submenus.
   */
  properties[PROP_CASCADE_POPDOWN] =
      g_param_spec_boolean ("cascade-popdown", NULL, NULL,
                            FALSE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, NUM_PROPERTIES, properties);

  /**
   * BobguiPopover::closed:
   * @self: the `BobguiPopover` which received the signal
   *
   * Emitted when the popover is closed.
   */
  signals[CLOSED] =
    g_signal_new (I_("closed"),
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BobguiPopoverClass, closed),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE,
                  0);

  /**
   * BobguiPopover::activate-default:
   * @self: the `BobguiPopover` which received the signal
   *
   * Emitted whend the user activates the default widget.
   *
   * This is a [keybinding signal](class.SignalAction.html).
   *
   * The default binding for this signal is <kbd>Enter</kbd>.
   */
  signals[ACTIVATE_DEFAULT] =
    g_signal_new (I_("activate-default"),
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (BobguiPopoverClass, activate_default),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE,
                  0);

  add_arrow_bindings (widget_class, GDK_KEY_Up, BOBGUI_DIR_UP);
  add_arrow_bindings (widget_class, GDK_KEY_Down, BOBGUI_DIR_DOWN);
  add_arrow_bindings (widget_class, GDK_KEY_Left, BOBGUI_DIR_LEFT);
  add_arrow_bindings (widget_class, GDK_KEY_Right, BOBGUI_DIR_RIGHT);

  add_tab_bindings (widget_class, 0, BOBGUI_DIR_TAB_FORWARD);
  add_tab_bindings (widget_class, GDK_CONTROL_MASK, BOBGUI_DIR_TAB_FORWARD);
  add_tab_bindings (widget_class, GDK_SHIFT_MASK, BOBGUI_DIR_TAB_BACKWARD);
  add_tab_bindings (widget_class, GDK_CONTROL_MASK | GDK_SHIFT_MASK, BOBGUI_DIR_TAB_BACKWARD);

  bobgui_widget_class_add_binding_signal (widget_class, GDK_KEY_Return, 0,
                                       "activate-default", NULL);
  bobgui_widget_class_add_binding_signal (widget_class, GDK_KEY_ISO_Enter, 0,
                                       "activate-default", NULL);
  bobgui_widget_class_add_binding_signal (widget_class, GDK_KEY_KP_Enter, 0,
                                       "activate-default", NULL);

  bobgui_widget_class_set_css_name (widget_class, "popover");
}

/**
 * bobgui_popover_new:
 *
 * Creates a new `BobguiPopover`.
 *
 * Returns: the new `BobguiPopover`
 */
BobguiWidget *
bobgui_popover_new (void)
{
  return g_object_new (BOBGUI_TYPE_POPOVER, NULL);
}

/**
 * bobgui_popover_set_child:
 * @popover: a `BobguiPopover`
 * @child: (nullable): the child widget
 *
 * Sets the child widget of @popover.
 */
void
bobgui_popover_set_child (BobguiPopover *popover,
                       BobguiWidget  *child)
{
  BobguiPopoverPrivate *priv = bobgui_popover_get_instance_private (popover);

  g_return_if_fail (BOBGUI_IS_POPOVER (popover));
  g_return_if_fail (child == NULL || priv->child == child || bobgui_widget_get_parent (child) == NULL);

  if (priv->child == child)
    return;

  g_clear_pointer (&priv->child, bobgui_widget_unparent);

  if (child)
    {
      priv->child = child;
      bobgui_widget_set_parent (child, priv->contents_widget);
    }

  g_object_notify_by_pspec (G_OBJECT (popover), properties[PROP_CHILD]);
}

/**
 * bobgui_popover_get_child:
 * @popover: a `BobguiPopover`
 *
 * Gets the child widget of @popover.
 *
 * Returns: (nullable) (transfer none): the child widget of @popover
 */
BobguiWidget *
bobgui_popover_get_child (BobguiPopover *popover)
{
  BobguiPopoverPrivate *priv = bobgui_popover_get_instance_private (popover);

  g_return_val_if_fail (BOBGUI_IS_POPOVER (popover), NULL);

  return priv->child;
}


/**
 * bobgui_popover_set_default_widget:
 * @popover: a `BobguiPopover`
 * @widget: (nullable): a child widget of @popover to set as
 *   the default, or %NULL to unset the default widget for the popover
 *
 * Sets the default widget of a `BobguiPopover`.
 *
 * The default widget is the widget that’s activated when the user
 * presses Enter in a dialog (for example). This function sets or
 * unsets the default widget for a `BobguiPopover`.
 */
void
bobgui_popover_set_default_widget (BobguiPopover *popover,
                                BobguiWidget  *widget)
{
  BobguiPopoverPrivate *priv = bobgui_popover_get_instance_private (popover);

  g_return_if_fail (BOBGUI_IS_POPOVER (popover));

  if (priv->default_widget == widget)
    return;

  if (priv->default_widget)
    {
      _bobgui_widget_set_has_default (priv->default_widget, FALSE);
      bobgui_widget_queue_draw (priv->default_widget);
      g_object_notify (G_OBJECT (priv->default_widget), "has-default");
    }

  g_set_object (&priv->default_widget, widget);

  if (priv->default_widget)
    {
      _bobgui_widget_set_has_default (priv->default_widget, TRUE);
      bobgui_widget_queue_draw (priv->default_widget);
      g_object_notify (G_OBJECT (priv->default_widget), "has-default");
    }

  g_object_notify_by_pspec (G_OBJECT (popover), properties[PROP_DEFAULT_WIDGET]);
}

static void
bobgui_popover_shortcut_manager_interface_init (BobguiShortcutManagerInterface *iface)
{
}

static void
bobgui_popover_native_interface_init (BobguiNativeInterface *iface)
{
  iface->get_surface = bobgui_popover_native_get_surface;
  iface->get_renderer = bobgui_popover_native_get_renderer;
  iface->get_surface_transform = bobgui_popover_native_get_surface_transform;
  iface->layout = bobgui_popover_native_layout;
}

static BobguiBuildableIface *parent_buildable_iface;

static void
bobgui_popover_buildable_add_child (BobguiBuildable *buildable,
                                 BobguiBuilder   *builder,
                                 GObject      *child,
                                 const char   *type)
{
  if (BOBGUI_IS_WIDGET (child))
    {
      bobgui_buildable_child_deprecation_warning (buildable, builder, NULL, "child");
      bobgui_popover_set_child (BOBGUI_POPOVER (buildable), BOBGUI_WIDGET (child));
    }
  else
    {
      parent_buildable_iface->add_child (buildable, builder, child, type);
    }
}

static void
bobgui_popover_buildable_init (BobguiBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);

  iface->add_child = bobgui_popover_buildable_add_child;
}

/**
 * bobgui_popover_set_pointing_to:
 * @popover: a `BobguiPopover`
 * @rect: (nullable): rectangle to point to
 *
 * Sets the rectangle that @popover points to.
 *
 * This is in the coordinate space of the @popover parent.
 */
void
bobgui_popover_set_pointing_to (BobguiPopover         *popover,
                             const GdkRectangle *rect)
{
  BobguiPopoverPrivate *priv = bobgui_popover_get_instance_private (popover);

  g_return_if_fail (BOBGUI_IS_POPOVER (popover));

  if (rect)
    {
      priv->pointing_to = *rect;
      priv->has_pointing_to = TRUE;
      priv->pointing_to.width = MAX (priv->pointing_to.width, 1);
      priv->pointing_to.height = MAX (priv->pointing_to.height, 1);
    }
  else
    priv->has_pointing_to = FALSE;

  g_object_notify_by_pspec (G_OBJECT (popover), properties[PROP_POINTING_TO]);

  if (bobgui_widget_is_visible (BOBGUI_WIDGET (popover)))
    present_popup (popover);
}

/**
 * bobgui_popover_get_pointing_to:
 * @popover: a `BobguiPopover`
 * @rect: (out): location to store the rectangle
 *
 * Gets the rectangle that the popover points to.
 *
 * If a rectangle to point to has been set, this function will
 * return %TRUE and fill in @rect with such rectangle, otherwise
 * it will return %FALSE and fill in @rect with the parent
 * widget coordinates.
 *
 * Returns: %TRUE if a rectangle to point to was set.
 */
gboolean
bobgui_popover_get_pointing_to (BobguiPopover   *popover,
                             GdkRectangle *rect)
{
  BobguiPopoverPrivate *priv = bobgui_popover_get_instance_private (popover);

  g_return_val_if_fail (BOBGUI_IS_POPOVER (popover), FALSE);
  g_return_val_if_fail (rect != NULL, FALSE);

  if (priv->has_pointing_to)
    *rect = priv->pointing_to;
  else
    {
      graphene_rect_t r;
      BobguiWidget *parent = bobgui_widget_get_parent (BOBGUI_WIDGET (popover));

      if (!bobgui_widget_compute_bounds (parent, parent, &r))
        {
          memset (rect, 0, sizeof (GdkRectangle));
          return FALSE;
        }

      rect->x = floorf (r.origin.x);
      rect->y = floorf (r.origin.y);
      rect->width = ceilf (r.size.width);
      rect->height = ceilf (r.size.height);
    }

  return priv->has_pointing_to;
}

/**
 * bobgui_popover_set_position:
 * @popover: a `BobguiPopover`
 * @position: preferred popover position
 *
 * Sets the preferred position for @popover to appear.
 *
 * If the @popover is currently visible, it will be immediately
 * updated.
 *
 * This preference will be respected where possible, although
 * on lack of space (eg. if close to the window edges), the
 * `BobguiPopover` may choose to appear on the opposite side.
 */
void
bobgui_popover_set_position (BobguiPopover      *popover,
                          BobguiPositionType  position)
{
  BobguiPopoverPrivate *priv = bobgui_popover_get_instance_private (popover);

  g_return_if_fail (BOBGUI_IS_POPOVER (popover));

  if (priv->position == position)
    return;

  priv->position = position;
  priv->final_position = position;

  g_object_notify_by_pspec (G_OBJECT (popover), properties[PROP_POSITION]);

  bobgui_widget_queue_resize (BOBGUI_WIDGET (popover));

  if (bobgui_widget_is_visible (BOBGUI_WIDGET (popover)))
    present_popup (popover);
}

/**
 * bobgui_popover_get_position:
 * @popover: a `BobguiPopover`
 *
 * Returns the preferred position of @popover.
 *
 * Returns: The preferred position.
 */
BobguiPositionType
bobgui_popover_get_position (BobguiPopover *popover)
{
  BobguiPopoverPrivate *priv = bobgui_popover_get_instance_private (popover);

  g_return_val_if_fail (BOBGUI_IS_POPOVER (popover), BOBGUI_POS_TOP);

  return priv->position;
}

/**
 * bobgui_popover_set_autohide:
 * @popover: a `BobguiPopover`
 * @autohide: %TRUE to dismiss the popover on outside clicks
 *
 * Sets whether @popover is modal.
 *
 * A modal popover will grab the keyboard focus on it when being
 * displayed. Focus will wrap around within the popover. Clicking
 * outside the popover area or pressing Esc will dismiss the popover.
 *
 * Called this function on an already showing popup with a new
 * autohide value different from the current one, will cause the
 * popup to be hidden.
 */
void
bobgui_popover_set_autohide (BobguiPopover *popover,
                          gboolean    autohide)
{
  BobguiPopoverPrivate *priv = bobgui_popover_get_instance_private (popover);

  g_return_if_fail (BOBGUI_IS_POPOVER (popover));

  autohide = autohide != FALSE;

  if (priv->autohide == autohide)
    return;

  priv->autohide = autohide;

  bobgui_widget_unrealize (BOBGUI_WIDGET (popover));

  g_object_notify_by_pspec (G_OBJECT (popover), properties[PROP_AUTOHIDE]);
}

/**
 * bobgui_popover_get_autohide:
 * @popover: a `BobguiPopover`
 *
 * Returns whether the popover is modal.
 *
 * See [method@Bobgui.Popover.set_autohide] for the
 * implications of this.
 *
 * Returns: %TRUE if @popover is modal
 */
gboolean
bobgui_popover_get_autohide (BobguiPopover *popover)
{
  BobguiPopoverPrivate *priv = bobgui_popover_get_instance_private (popover);

  g_return_val_if_fail (BOBGUI_IS_POPOVER (popover), FALSE);

  return priv->autohide;
}

/**
 * bobgui_popover_popup:
 * @popover: a `BobguiPopover`
 *
 * Pops @popover up.
 */
void
bobgui_popover_popup (BobguiPopover *popover)
{
  g_return_if_fail (BOBGUI_IS_POPOVER (popover));

  bobgui_widget_set_visible (BOBGUI_WIDGET (popover), TRUE);
}

static void
cascade_popdown (BobguiPopover *popover)
{
  BobguiWidget *parent;
  BobguiWidget *new_focus;

  /* Do not trigger cascade close from non-modal popovers */
  if (!bobgui_popover_get_autohide (popover))
    return;

  parent = bobgui_widget_get_parent (BOBGUI_WIDGET (popover));
  new_focus = parent;

  while (parent)
    {
      if (BOBGUI_IS_POPOVER (parent))
        {
          new_focus = bobgui_widget_get_parent (parent);
          if (bobgui_popover_get_cascade_popdown (BOBGUI_POPOVER (parent)))
            bobgui_widget_set_visible (parent, FALSE);
          else
            break;
        }

      parent = bobgui_widget_get_parent (parent);
    }

    if (new_focus)
      bobgui_widget_grab_focus (new_focus);
}

/**
 * bobgui_popover_popdown:
 * @popover: a `BobguiPopover`
 *
 * Pops @popover down.
 *
 * This may have the side-effect of closing a parent popover
 * as well. See [property@Bobgui.Popover:cascade-popdown].
 */
void
bobgui_popover_popdown (BobguiPopover *popover)
{
  g_return_if_fail (BOBGUI_IS_POPOVER (popover));

  if (!bobgui_widget_get_visible (BOBGUI_WIDGET (popover)))
    return;

  bobgui_widget_set_visible (BOBGUI_WIDGET (popover), FALSE);

  cascade_popdown (popover);
}

BobguiWidget *
bobgui_popover_get_contents_widget (BobguiPopover *popover)
{
  BobguiPopoverPrivate *priv = bobgui_popover_get_instance_private (popover);

  return priv->contents_widget;
}

/**
 * bobgui_popover_set_has_arrow:
 * @popover: a `BobguiPopover`
 * @has_arrow: %TRUE to draw an arrow
 *
 * Sets whether this popover should draw an arrow
 * pointing at the widget it is relative to.
 */
void
bobgui_popover_set_has_arrow (BobguiPopover *popover,
                           gboolean    has_arrow)
{
  BobguiPopoverPrivate *priv = bobgui_popover_get_instance_private (popover);

  g_return_if_fail (BOBGUI_IS_POPOVER (popover));

  if (priv->has_arrow == has_arrow)
    return;

  priv->has_arrow = has_arrow;

  g_object_notify_by_pspec (G_OBJECT (popover), properties[PROP_HAS_ARROW]);
  bobgui_widget_queue_resize (BOBGUI_WIDGET (popover));
}

/**
 * bobgui_popover_get_has_arrow:
 * @popover: a `BobguiPopover`
 *
 * Gets whether this popover is showing an arrow
 * pointing at the widget that it is relative to.
 *
 * Returns: whether the popover has an arrow
 */
gboolean
bobgui_popover_get_has_arrow (BobguiPopover *popover)
{
  BobguiPopoverPrivate *priv = bobgui_popover_get_instance_private (popover);

  g_return_val_if_fail (BOBGUI_IS_POPOVER (popover), TRUE);

  return priv->has_arrow;
}

/**
 * bobgui_popover_set_mnemonics_visible:
 * @popover: a `BobguiPopover`
 * @mnemonics_visible: the new value
 *
 * Sets whether mnemonics should be visible.
 */
void
bobgui_popover_set_mnemonics_visible (BobguiPopover *popover,
                                   gboolean    mnemonics_visible)
{
  BobguiPopoverPrivate *priv = bobgui_popover_get_instance_private (popover);

  g_return_if_fail (BOBGUI_IS_POPOVER (popover));

  if (priv->mnemonics_visible == mnemonics_visible)
    return;

  priv->mnemonics_visible = mnemonics_visible;

  g_object_notify_by_pspec (G_OBJECT (popover), properties[PROP_MNEMONICS_VISIBLE]);
  bobgui_widget_queue_resize (BOBGUI_WIDGET (popover));

  if (priv->mnemonics_display_timeout_id)
    {
      g_source_remove (priv->mnemonics_display_timeout_id);
      priv->mnemonics_display_timeout_id = 0;
    }
}

/**
 * bobgui_popover_get_mnemonics_visible:
 * @popover: a `BobguiPopover`
 *
 * Gets whether mnemonics are visible.
 *
 * Returns: %TRUE if mnemonics are supposed to be visible
 *   in this popover
 */
gboolean
bobgui_popover_get_mnemonics_visible (BobguiPopover *popover)
{
  BobguiPopoverPrivate *priv = bobgui_popover_get_instance_private (popover);

  g_return_val_if_fail (BOBGUI_IS_POPOVER (popover), FALSE);

  return priv->mnemonics_visible;
}

/**
 * bobgui_popover_set_offset:
 * @popover: a `BobguiPopover`
 * @x_offset: the x offset to adjust the position by
 * @y_offset: the y offset to adjust the position by
 *
 * Sets the offset to use when calculating the position
 * of the popover.
 *
 * These values are used when preparing the [struct@Gdk.PopupLayout]
 * for positioning the popover.
 */
void
bobgui_popover_set_offset (BobguiPopover *popover,
                        int         x_offset,
                        int         y_offset)
{
  BobguiPopoverPrivate *priv = bobgui_popover_get_instance_private (popover);

  g_return_if_fail (BOBGUI_IS_POPOVER (popover));

  if (priv->x_offset != x_offset || priv->y_offset != y_offset)
    {
      priv->x_offset = x_offset;
      priv->y_offset = y_offset;

      bobgui_widget_queue_resize (BOBGUI_WIDGET (popover));
    }
}

/**
 * bobgui_popover_get_offset:
 * @popover: a `BobguiPopover`
 * @x_offset: (out) (optional): a location for the x_offset
 * @y_offset: (out) (optional): a location for the y_offset
 *
 * Gets the offset previous set with [method@Bobgui.Popover.set_offset].
 */
void
bobgui_popover_get_offset (BobguiPopover *popover,
                        int        *x_offset,
                        int        *y_offset)
{
  BobguiPopoverPrivate *priv = bobgui_popover_get_instance_private (popover);

  g_return_if_fail (BOBGUI_IS_POPOVER (popover));

  if (x_offset)
    *x_offset = priv->x_offset;

  if (y_offset)
    *y_offset = priv->y_offset;
}

/**
 * bobgui_popover_set_cascade_popdown:
 * @popover: A `BobguiPopover`
 * @cascade_popdown: %TRUE if the popover should follow a child closing
 *
 * If @cascade_popdown is %TRUE, the popover will be
 * closed when a child modal popover is closed.
 *
 * If %FALSE, @popover will stay visible.
 */
void
bobgui_popover_set_cascade_popdown (BobguiPopover *popover,
                                 gboolean    cascade_popdown)
{
  BobguiPopoverPrivate *priv = bobgui_popover_get_instance_private (popover);

  if (priv->cascade_popdown != !!cascade_popdown)
    {
      priv->cascade_popdown = !!cascade_popdown;
      g_object_notify (G_OBJECT (popover), "cascade-popdown");
    }
}

/**
 * bobgui_popover_get_cascade_popdown:
 * @popover: a `BobguiPopover`
 *
 * Returns whether the popover will close after a modal child is closed.
 *
 * Returns: %TRUE if @popover will close after a modal child.
 */
gboolean
bobgui_popover_get_cascade_popdown (BobguiPopover *popover)
{
  BobguiPopoverPrivate *priv = bobgui_popover_get_instance_private (popover);

  return priv->cascade_popdown;
}
