/* bobguioverlaylayout.c: Overlay layout manager
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * Copyright 2019 Red Hat, Inc.
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

#include "bobguioverlaylayout.h"

#include "bobguilayoutchild.h"
#include "bobguioverlay.h"
#include "bobguiprivate.h"
#include "bobguiwidgetprivate.h"

#include <graphene-gobject.h>


/**
 * BobguiOverlayLayout:
 *
 * The layout manager used by [class@Bobgui.Overlay].
 *
 * It places widgets as overlays on top of the main child.
 *
 * This is not a reusable layout manager, since it expects its widget
 * to be a `BobguiOverlay`. It is only listed here so that its layout
 * properties get documented.
 */

/**
 * BobguiOverlayLayoutChild:
 *
 * `BobguiLayoutChild` subclass for children in a `BobguiOverlayLayout`.
 */

struct _BobguiOverlayLayout
{
  BobguiLayoutManager parent_instance;
};

struct _BobguiOverlayLayoutChild
{
  BobguiLayoutChild parent_instance;

  guint measure : 1;
  guint clip_overlay : 1;
};

enum
{
  PROP_MEASURE = 1,
  PROP_CLIP_OVERLAY,

  N_CHILD_PROPERTIES
};

static GParamSpec *child_props[N_CHILD_PROPERTIES];

G_DEFINE_TYPE (BobguiOverlayLayoutChild, bobgui_overlay_layout_child, BOBGUI_TYPE_LAYOUT_CHILD)

static void
bobgui_overlay_layout_child_set_property (GObject      *gobject,
                                       guint         prop_id,
                                       const GValue *value,
                                       GParamSpec   *pspec)
{
  BobguiOverlayLayoutChild *self = BOBGUI_OVERLAY_LAYOUT_CHILD (gobject);

  switch (prop_id)
    {
    case PROP_MEASURE:
      bobgui_overlay_layout_child_set_measure (self, g_value_get_boolean (value));
      break;

    case PROP_CLIP_OVERLAY:
      bobgui_overlay_layout_child_set_clip_overlay (self, g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static void
bobgui_overlay_layout_child_get_property (GObject    *gobject,
                                       guint       prop_id,
                                       GValue     *value,
                                       GParamSpec *pspec)
{
  BobguiOverlayLayoutChild *self = BOBGUI_OVERLAY_LAYOUT_CHILD (gobject);

  switch (prop_id)
    {
    case PROP_MEASURE:
      g_value_set_boolean (value, self->measure);
      break;

    case PROP_CLIP_OVERLAY:
      g_value_set_boolean (value, self->clip_overlay);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static void
bobgui_overlay_layout_child_class_init (BobguiOverlayLayoutChildClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->set_property = bobgui_overlay_layout_child_set_property;
  gobject_class->get_property = bobgui_overlay_layout_child_get_property;

  /**
   * BobguiOverlayLayoutChild:measure:
   *
   * Whether the child size should contribute to the `BobguiOverlayLayout`'s
   * measurement.
   */
  child_props[PROP_MEASURE] =
    g_param_spec_boolean ("measure", NULL, NULL,
                          FALSE,
                          BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiOverlayLayoutChild:clip-overlay:
   *
   * Whether the child should be clipped to fit the parent's size.
   */
  child_props[PROP_CLIP_OVERLAY] =
    g_param_spec_boolean ("clip-overlay", NULL, NULL,
                          FALSE,
                          BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (gobject_class, N_CHILD_PROPERTIES, child_props);
}

static void
bobgui_overlay_layout_child_init (BobguiOverlayLayoutChild *self)
{
}

/**
 * bobgui_overlay_layout_child_set_measure:
 * @child: a `BobguiOverlayLayoutChild`
 * @measure: whether to measure this child
 *
 * Sets whether to measure this child.
 */
void
bobgui_overlay_layout_child_set_measure (BobguiOverlayLayoutChild *child,
                                      gboolean               measure)
{
  BobguiLayoutManager *layout;

  g_return_if_fail (BOBGUI_IS_OVERLAY_LAYOUT_CHILD (child));

  if (child->measure == measure)
    return;

  child->measure = measure;

  layout = bobgui_layout_child_get_layout_manager (BOBGUI_LAYOUT_CHILD (child));
  bobgui_layout_manager_layout_changed (layout);

  g_object_notify_by_pspec (G_OBJECT (child), child_props[PROP_MEASURE]);
}

/**
 * bobgui_overlay_layout_child_get_measure:
 * @child: a `BobguiOverlayLayoutChild`
 *
 * Retrieves whether the child is measured.
 *
 * Returns: whether the child is measured
 */
gboolean
bobgui_overlay_layout_child_get_measure (BobguiOverlayLayoutChild *child)
{
  g_return_val_if_fail (BOBGUI_IS_OVERLAY_LAYOUT_CHILD (child), FALSE);

  return child->measure;
}

/**
 * bobgui_overlay_layout_child_set_clip_overlay:
 * @child: a `BobguiOverlayLayoutChild`
 * @clip_overlay: whether to clip this child
 *
 * Sets whether to clip this child.
 */
void
bobgui_overlay_layout_child_set_clip_overlay (BobguiOverlayLayoutChild *child,
                                           gboolean               clip_overlay)
{
  BobguiLayoutManager *layout;

  g_return_if_fail (BOBGUI_IS_OVERLAY_LAYOUT_CHILD (child));

  if (child->clip_overlay == clip_overlay)
    return;

  child->clip_overlay = clip_overlay;

  layout = bobgui_layout_child_get_layout_manager (BOBGUI_LAYOUT_CHILD (child));
  bobgui_layout_manager_layout_changed (layout);

  g_object_notify_by_pspec (G_OBJECT (child), child_props[PROP_CLIP_OVERLAY]);
}

/**
 * bobgui_overlay_layout_child_get_clip_overlay:
 * @child: a `BobguiOverlayLayoutChild`
 *
 * Retrieves whether the child is clipped.
 *
 * Returns: whether the child is clipped
 */
gboolean
bobgui_overlay_layout_child_get_clip_overlay (BobguiOverlayLayoutChild *child)
{
  g_return_val_if_fail (BOBGUI_IS_OVERLAY_LAYOUT_CHILD (child), FALSE);

  return child->clip_overlay;
}

G_DEFINE_TYPE (BobguiOverlayLayout, bobgui_overlay_layout, BOBGUI_TYPE_LAYOUT_MANAGER)

static void
bobgui_overlay_layout_measure (BobguiLayoutManager *layout_manager,
                            BobguiWidget        *widget,
                            BobguiOrientation    orientation,
                            int               for_size,
                            int              *minimum,
                            int              *natural,
                            int              *minimum_baseline,
                            int              *natural_baseline)
{
  BobguiOverlayLayoutChild *child_info;
  BobguiWidget *child;
  int min, nat;
  BobguiWidget *main_widget;

  main_widget = bobgui_overlay_get_child (BOBGUI_OVERLAY (widget));

  min = 0;
  nat = 0;

  for (child = _bobgui_widget_get_first_child (widget);
       child != NULL;
       child = _bobgui_widget_get_next_sibling (child))
    {
      if (!bobgui_widget_should_layout (child))
        continue;

      child_info = BOBGUI_OVERLAY_LAYOUT_CHILD (bobgui_layout_manager_get_layout_child (layout_manager, child));

      if (child == main_widget || child_info->measure)
        {
          int child_min, child_nat, child_min_baseline, child_nat_baseline;

          bobgui_widget_measure (child,
                              orientation,
                              for_size,
                              &child_min, &child_nat,
                              &child_min_baseline, &child_nat_baseline);

          min = MAX (min, child_min);
          nat = MAX (nat, child_nat);
        }
    }

  if (minimum != NULL)
    *minimum = min;
  if (natural != NULL)
    *natural = nat;
}

static void
bobgui_overlay_compute_child_allocation (BobguiOverlay            *overlay,
                                      BobguiWidget             *widget,
                                      BobguiOverlayLayoutChild *child,
                                      BobguiAllocation         *widget_allocation)
{
  BobguiAllocation allocation;
  gboolean result;

  g_signal_emit_by_name (overlay, "get-child-position",
                         widget, &allocation, &result);

  widget_allocation->x = allocation.x;
  widget_allocation->y = allocation.y;
  widget_allocation->width = allocation.width;
  widget_allocation->height = allocation.height;
}

static BobguiAlign
effective_align (BobguiAlign         align,
                 BobguiTextDirection direction)
{
  switch (align)
    {
    case BOBGUI_ALIGN_START:
      return direction == BOBGUI_TEXT_DIR_RTL ? BOBGUI_ALIGN_END : BOBGUI_ALIGN_START;
    case BOBGUI_ALIGN_END:
      return direction == BOBGUI_TEXT_DIR_RTL ? BOBGUI_ALIGN_START : BOBGUI_ALIGN_END;
    case BOBGUI_ALIGN_FILL:
    case BOBGUI_ALIGN_CENTER:
    case BOBGUI_ALIGN_BASELINE_FILL:
    case BOBGUI_ALIGN_BASELINE_CENTER:
    default:
      return align;
    }
}

static void
bobgui_overlay_child_update_style_classes (BobguiOverlay *overlay,
                                        BobguiWidget *child,
                                        BobguiAllocation *child_allocation)
{
  BobguiWidget *widget = BOBGUI_WIDGET (overlay);
  int width, height;
  BobguiAlign valign, halign;
  gboolean is_left, is_right, is_top, is_bottom;
  gboolean has_left, has_right, has_top, has_bottom;

  has_left = bobgui_widget_has_css_class (child, "left");
  has_right = bobgui_widget_has_css_class (child, "right");
  has_top = bobgui_widget_has_css_class (child, "top");
  has_bottom = bobgui_widget_has_css_class (child, "bottom");

  is_left = is_right = is_top = is_bottom = FALSE;

  width = bobgui_widget_get_width (widget);
  height = bobgui_widget_get_height (widget);

  halign = effective_align (bobgui_widget_get_halign (child),
                            bobgui_widget_get_direction (child));

  if (halign == BOBGUI_ALIGN_START)
    is_left = (child_allocation->x == 0);
  else if (halign == BOBGUI_ALIGN_END)
    is_right = (child_allocation->x + child_allocation->width == width);

  valign = bobgui_widget_get_valign (child);

  if (valign == BOBGUI_ALIGN_START)
    is_top = (child_allocation->y == 0);
  else if (valign == BOBGUI_ALIGN_END)
    is_bottom = (child_allocation->y + child_allocation->height == height);

  if (has_left && !is_left)
    bobgui_widget_remove_css_class (child, "left");
  else if (!has_left && is_left)
    bobgui_widget_add_css_class (child, "left");

  if (has_right && !is_right)
    bobgui_widget_remove_css_class (child, "right");
  else if (!has_right && is_right)
    bobgui_widget_add_css_class (child, "right");

  if (has_top && !is_top)
    bobgui_widget_remove_css_class (child, "top");
  else if (!has_top && is_top)
    bobgui_widget_add_css_class (child, "top");

  if (has_bottom && !is_bottom)
    bobgui_widget_remove_css_class (child, "bottom");
  else if (!has_bottom && is_bottom)
    bobgui_widget_add_css_class (child, "bottom");
}

static void
bobgui_overlay_child_allocate (BobguiOverlay            *overlay,
                            BobguiWidget             *widget,
                            BobguiOverlayLayoutChild *child)
{
  BobguiAllocation child_allocation;

  if (!bobgui_widget_should_layout (widget))
    return;

  bobgui_overlay_compute_child_allocation (overlay, widget, child, &child_allocation);

  bobgui_overlay_child_update_style_classes (overlay, widget, &child_allocation);
  bobgui_widget_size_allocate (widget, &child_allocation, -1);
}

static void
bobgui_overlay_layout_allocate (BobguiLayoutManager *layout_manager,
                             BobguiWidget        *widget,
                             int               width,
                             int               height,
                             int               baseline)
{
  BobguiWidget *child;
  BobguiWidget *main_widget;

  main_widget = bobgui_overlay_get_child (BOBGUI_OVERLAY (widget));
  if (main_widget && bobgui_widget_get_visible (main_widget))
    bobgui_widget_size_allocate (main_widget,
                              &(BobguiAllocation) { 0, 0, width, height },
                              -1);

  for (child = _bobgui_widget_get_first_child (widget);
       child != NULL;
       child = _bobgui_widget_get_next_sibling (child))
    {
      if (child != main_widget)
        {
          BobguiOverlayLayoutChild *child_data;
          child_data = BOBGUI_OVERLAY_LAYOUT_CHILD (bobgui_layout_manager_get_layout_child (layout_manager, child));

          bobgui_overlay_child_allocate (BOBGUI_OVERLAY (widget), child, child_data);
        }
    }
}

static void
bobgui_overlay_layout_class_init (BobguiOverlayLayoutClass *klass)
{
  BobguiLayoutManagerClass *layout_class = BOBGUI_LAYOUT_MANAGER_CLASS (klass);

  layout_class->layout_child_type = BOBGUI_TYPE_OVERLAY_LAYOUT_CHILD;
  layout_class->measure = bobgui_overlay_layout_measure;
  layout_class->allocate = bobgui_overlay_layout_allocate;
}

static void
bobgui_overlay_layout_init (BobguiOverlayLayout *self)
{
}

/**
 * bobgui_overlay_layout_new:
 *
 * Creates a new `BobguiOverlayLayout` instance.
 *
 * Returns: the newly created instance
 */
BobguiLayoutManager *
bobgui_overlay_layout_new (void)
{
  return g_object_new (BOBGUI_TYPE_OVERLAY_LAYOUT, NULL);
}
