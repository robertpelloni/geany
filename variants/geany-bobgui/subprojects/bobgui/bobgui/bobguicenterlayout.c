/*
 * SPDX-License-Identifier: LGPL-2.1-or-later
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

#include "bobguicenterlayout.h"

#include "bobguicsspositionvalueprivate.h"
#include "bobguilayoutchild.h"
#include "bobguiprivate.h"
#include "bobguisizerequest.h"
#include "bobguiwidgetprivate.h"
#include "bobguicssnodeprivate.h"

/**
 * BobguiCenterLayout:
 *
 * Manages up to three children.
 *
 * The start widget is allocated at the start of the layout (left in
 * left-to-right locales and right in right-to-left ones), and the end
 * widget at the end.
 *
 * The center widget is centered regarding the full width of the layout's.
 */
struct _BobguiCenterLayout
{
  BobguiLayoutManager parent_instance;

  BobguiBaselinePosition baseline_pos;
  BobguiOrientation orientation;
  gboolean shrink_center_last;

  union {
    struct {
      BobguiWidget *start_widget;
      BobguiWidget *center_widget;
      BobguiWidget *end_widget;
    };
    BobguiWidget *children[3];
  };
};

enum {
  PROP_0,
  PROP_SHRINK_CENTER_LAST,
  LAST_PROP
};

static GParamSpec *props[LAST_PROP] = { NULL, };

G_DEFINE_TYPE (BobguiCenterLayout, bobgui_center_layout, BOBGUI_TYPE_LAYOUT_MANAGER)

static int
get_spacing (BobguiCenterLayout *self,
             BobguiCssNode      *node)
{
  BobguiCssStyle *style = bobgui_css_node_get_style (node);
  BobguiCssValue *border_spacing;
  int css_spacing;

  border_spacing = style->size->border_spacing;
  if (self->orientation == BOBGUI_ORIENTATION_HORIZONTAL)
    css_spacing = _bobgui_css_position_value_get_x (border_spacing, 100);
  else
    css_spacing = _bobgui_css_position_value_get_y (border_spacing, 100);

  return css_spacing;
}

static BobguiSizeRequestMode
bobgui_center_layout_get_request_mode (BobguiLayoutManager *layout_manager,
                                    BobguiWidget        *widget)
{
  BobguiCenterLayout *self = BOBGUI_CENTER_LAYOUT (layout_manager);
  int count[3] = { 0, 0, 0 };

  if (self->start_widget)
    count[bobgui_widget_get_request_mode (self->start_widget)]++;

  if (self->center_widget)
    count[bobgui_widget_get_request_mode (self->center_widget)]++;

  if (self->end_widget)
    count[bobgui_widget_get_request_mode (self->end_widget)]++;

  if (!count[BOBGUI_SIZE_REQUEST_HEIGHT_FOR_WIDTH] &&
      !count[BOBGUI_SIZE_REQUEST_WIDTH_FOR_HEIGHT])
    return BOBGUI_SIZE_REQUEST_CONSTANT_SIZE;
  else
    return count[BOBGUI_SIZE_REQUEST_WIDTH_FOR_HEIGHT] > count[BOBGUI_SIZE_REQUEST_HEIGHT_FOR_WIDTH]
           ? BOBGUI_SIZE_REQUEST_WIDTH_FOR_HEIGHT
           : BOBGUI_SIZE_REQUEST_HEIGHT_FOR_WIDTH;
}

static void
bobgui_center_layout_distribute (BobguiCenterLayout  *self,
                              int               for_size,
                              int               size,
                              int               spacing,
                              BobguiRequestedSize *sizes)
{
  int center_size = 0;
  int start_size = 0;
  int end_size = 0;
  gboolean center_expand = FALSE;
  gboolean start_expand = FALSE;
  gboolean end_expand = FALSE;
  int avail;
  int i;
  int needed_spacing = 0;

  /* Usable space is really less... */
  for (i = 0; i < 3; i++)
    {
      if (self->children[i])
        needed_spacing += spacing;
    }
  needed_spacing -= spacing;

  sizes[0].minimum_size = sizes[0].natural_size = 0;
  sizes[1].minimum_size = sizes[1].natural_size = 0;
  sizes[2].minimum_size = sizes[2].natural_size = 0;

  for (i = 0; i < 3; i ++)
    {
      if (self->children[i])
        bobgui_widget_measure (self->children[i], self->orientation, for_size,
                            &sizes[i].minimum_size, &sizes[i].natural_size,
                            NULL, NULL);
    }

  if (self->center_widget)
    {
      int natural_size;

      avail = size - needed_spacing - (sizes[0].minimum_size + sizes[2].minimum_size);

      if (self->shrink_center_last)
        natural_size = sizes[1].natural_size;
      else
        natural_size = CLAMP (size - needed_spacing - (sizes[0].natural_size + sizes[2].natural_size), sizes[1].minimum_size, sizes[1].natural_size);

      center_size = CLAMP (avail, sizes[1].minimum_size, natural_size);
      center_expand = bobgui_widget_compute_expand (self->center_widget, self->orientation);
    }

  if (self->start_widget)
    {
      avail = size - needed_spacing - (center_size + sizes[2].minimum_size);
      start_size = CLAMP (avail, sizes[0].minimum_size, sizes[0].natural_size);
      start_expand = bobgui_widget_compute_expand (self->start_widget, self->orientation);
    }

   if (self->end_widget)
    {
      avail = size - needed_spacing - (center_size + sizes[0].minimum_size);
      end_size = CLAMP (avail, sizes[2].minimum_size, sizes[2].natural_size);
      end_expand = bobgui_widget_compute_expand (self->end_widget, self->orientation);
    }

  if (self->center_widget)
    {
      int center_pos;

      center_pos = (size / 2) - (center_size / 2);

      /* Push in from start/end */
      if (start_size > 0 && start_size + spacing > center_pos)
        center_pos = start_size + spacing;
      else if (end_size > 0 && size - end_size - spacing < center_pos + center_size)
        center_pos = size - center_size - end_size - spacing;
      else if (center_expand)
        {
          center_size = size - 2 * (MAX (start_size, end_size) + spacing);
          center_pos = (size / 2) - (center_size / 2) + spacing;
        }

      if (start_expand)
        start_size = center_pos - spacing;

      if (end_expand)
        end_size = size - (center_pos + center_size) - spacing;
    }
  else
    {
      avail = size - needed_spacing - (start_size + end_size);
      if (start_expand && end_expand)
        {
          start_size += avail / 2;
          end_size += avail / 2;
        }
      else if (start_expand)
        {
          start_size += avail;
        }
      else if (end_expand)
        {
          end_size += avail;
        }
    }

  sizes[0].minimum_size = start_size;
  sizes[1].minimum_size = center_size;
  sizes[2].minimum_size = end_size;
}

static void
bobgui_center_layout_measure_orientation (BobguiCenterLayout *self,
                                       BobguiWidget       *widget,
                                       BobguiOrientation   orientation,
                                       int              for_size,
                                       int             *minimum,
                                       int             *natural,
                                       int             *minimum_baseline,
                                       int             *natural_baseline)
{
  int min[3];
  int nat[3];
  int n_visible_children = 0;
  int spacing;
  int i;

  spacing = get_spacing (self, bobgui_widget_get_css_node (widget));

  for (i = 0; i < 3; i ++)
    {
      BobguiWidget *child = self->children[i];

      if (child)
        {
          bobgui_widget_measure (child,
                              orientation,
                              for_size,
                              &min[i], &nat[i], NULL, NULL);

          if (_bobgui_widget_get_visible (child))
            n_visible_children ++;
        }
      else
        {
          min[i] = 0;
          nat[i] = 0;
        }
    }

  *minimum = min[0] + min[1] + min[2];
  *natural = nat[1] + 2 * MAX (nat[0], nat[2]);

  if (n_visible_children > 0)
    {
      *minimum += (n_visible_children - 1) * spacing;
      *natural += (n_visible_children - 1) * spacing;
    }
}

static void
bobgui_center_layout_measure_opposite (BobguiCenterLayout *self,
                                    BobguiOrientation   orientation,
                                    int              for_size,
                                    int             *minimum,
                                    int             *natural,
                                    int             *minimum_baseline,
                                    int             *natural_baseline)
{
  int child_min, child_nat;
  int child_min_baseline, child_nat_baseline;
  int total_min, above_min, below_min;
  int total_nat, above_nat, below_nat;
  BobguiWidget *child[3];
  BobguiRequestedSize sizes[3];
  gboolean have_baseline = FALSE;
  gboolean align_baseline = FALSE;
  int i;

  child[0] = self->start_widget;
  child[1] = self->center_widget;
  child[2] = self->end_widget;

  if (for_size >= 0)
    bobgui_center_layout_distribute (self, -1, for_size, 0, sizes);

  above_min = below_min = above_nat = below_nat = -1;
  total_min = total_nat = 0;

  for (i = 0; i < 3; i++)
    {
      if (child[i] == NULL)
        continue;

      bobgui_widget_measure (child[i],
                          orientation,
                          for_size >= 0 ? sizes[i].minimum_size : -1,
                          &child_min, &child_nat,
                          &child_min_baseline, &child_nat_baseline);

      total_min = MAX (total_min, child_min);
      total_nat = MAX (total_nat, child_nat);

      if (orientation == BOBGUI_ORIENTATION_VERTICAL && child_min_baseline >= 0)
        {
          have_baseline = TRUE;
          if (bobgui_widget_get_valign (child[i]) == BOBGUI_ALIGN_BASELINE_FILL ||
              bobgui_widget_get_valign (child[i]) == BOBGUI_ALIGN_BASELINE_CENTER)
            align_baseline = TRUE;

          below_min = MAX (below_min, child_min - child_min_baseline);
          above_min = MAX (above_min, child_min_baseline);
          below_nat = MAX (below_nat, child_nat - child_nat_baseline);
          above_nat = MAX (above_nat, child_nat_baseline);
        }
   }

  if (have_baseline)
    {
      int min_baseline = -1;
      int nat_baseline = -1;

      if (align_baseline)
        {
          total_min = MAX (total_min, above_min + below_min);
          total_nat = MAX (total_nat, above_nat + below_nat);
        }

      switch (self->baseline_pos)
        {
        case BOBGUI_BASELINE_POSITION_TOP:
          min_baseline = above_min;
          nat_baseline = above_nat;
          break;
        case BOBGUI_BASELINE_POSITION_CENTER:
          min_baseline = above_min + (total_min - (above_min + below_min)) / 2;
          nat_baseline = above_nat + (total_nat - (above_nat + below_nat)) / 2;
          break;
        case BOBGUI_BASELINE_POSITION_BOTTOM:
          min_baseline = total_min - below_min;
          nat_baseline = total_nat - below_nat;
          break;
        default:
          break;
        }

      if (minimum_baseline)
        *minimum_baseline = min_baseline;
      if (natural_baseline)
        *natural_baseline = nat_baseline;
    }

  *minimum = total_min;
  *natural = total_nat;
}



static void
bobgui_center_layout_measure (BobguiLayoutManager *layout_manager,
                           BobguiWidget        *widget,
                           BobguiOrientation    orientation,
                           int               for_size,
                           int              *minimum,
                           int              *natural,
                           int              *minimum_baseline,
                           int              *natural_baseline)
{
  BobguiCenterLayout *self = BOBGUI_CENTER_LAYOUT (layout_manager);

  if (self->orientation == orientation)
    bobgui_center_layout_measure_orientation (self, widget, orientation, for_size,
                                           minimum, natural, minimum_baseline, natural_baseline);
  else
    bobgui_center_layout_measure_opposite (self, orientation, for_size,
                                        minimum, natural, minimum_baseline, natural_baseline);
}

static void
bobgui_center_layout_allocate (BobguiLayoutManager *layout_manager,
                            BobguiWidget        *widget,
                            int               width,
                            int               height,
                            int               baseline)
{
  BobguiCenterLayout *self = BOBGUI_CENTER_LAYOUT (layout_manager);
  BobguiWidget *child[3];
  int child_size[3];
  int child_pos[3];
  BobguiRequestedSize sizes[3];
  int size;
  int for_size;
  int i;
  int spacing;

  spacing = get_spacing (self, bobgui_widget_get_css_node (widget));

  if (self->orientation == BOBGUI_ORIENTATION_HORIZONTAL)
    {
      size = width;
      for_size = height;
    }
  else
    {
      size = height;
      for_size = width;
      baseline = -1;
    }

  /* Allocate child sizes */

  bobgui_center_layout_distribute (self, for_size, size, spacing, sizes);

  child[1] = self->center_widget;
  child_size[1] = sizes[1].minimum_size;

  if (self->orientation == BOBGUI_ORIENTATION_HORIZONTAL &&
      bobgui_widget_get_direction (widget) == BOBGUI_TEXT_DIR_RTL)
    {
      child[0] = self->end_widget;
      child[2] = self->start_widget;
      child_size[0] = sizes[2].minimum_size;
      child_size[2] = sizes[0].minimum_size;
    }
  else
    {
      child[0] = self->start_widget;
      child[2] = self->end_widget;
      child_size[0] = sizes[0].minimum_size;
      child_size[2] = sizes[2].minimum_size;
    }

  /* Determine baseline */
  if (self->orientation == BOBGUI_ORIENTATION_HORIZONTAL &&
      baseline == -1)
    {
      int min_above, nat_above;
      int min_below, nat_below;
      gboolean have_baseline;

      have_baseline = FALSE;
      min_above = nat_above = 0;
      min_below = nat_below = 0;

      for (i = 0; i < 3; i++)
        {
          if (child[i] &&
              (bobgui_widget_get_valign (child[i]) == BOBGUI_ALIGN_BASELINE_FILL ||
               bobgui_widget_get_valign (child[i]) == BOBGUI_ALIGN_BASELINE_CENTER))
            {
              int child_min_height, child_nat_height;
              int child_min_baseline, child_nat_baseline;

              child_min_baseline = child_nat_baseline = -1;

              bobgui_widget_measure (child[i], BOBGUI_ORIENTATION_VERTICAL,
                                  child_size[i],
                                  &child_min_height, &child_nat_height,
                                  &child_min_baseline, &child_nat_baseline);

              if (child_min_baseline >= 0)
                {
                  have_baseline = TRUE;
                  min_below = MAX (min_below, child_min_height - child_min_baseline);
                  nat_below = MAX (nat_below, child_nat_height - child_nat_baseline);
                  min_above = MAX (min_above, child_min_baseline);
                  nat_above = MAX (nat_above, child_nat_baseline);
                }
            }
        }

      if (have_baseline)
        {
          /* TODO: This is purely based on the minimum baseline.
           * When things fit we should use the natural one
           */
          switch (self->baseline_pos)
            {
            default:
            case BOBGUI_BASELINE_POSITION_TOP:
              baseline = min_above;
              break;
            case BOBGUI_BASELINE_POSITION_CENTER:
              baseline = min_above + (height - (min_above + min_below)) / 2;
              break;
            case BOBGUI_BASELINE_POSITION_BOTTOM:
              baseline = height - min_below;
              break;
            }
        }
    }

  /* Allocate child positions */

  child_pos[0] = 0;
  child_pos[1] = (size / 2) - (child_size[1] / 2);
  child_pos[2] = size - child_size[2];

  if (child[1])
    {
      /* Push in from start/end */
      if (child_size[0] > 0 && child_size[0] + spacing > child_pos[1])
        child_pos[1] = child_size[0] + spacing;
      else if (child_size[2] > 0 && size - child_size[2] - spacing < child_pos[1] + child_size[1])
        child_pos[1] = size - child_size[1] - child_size[2] - spacing;
    }

  for (i = 0; i < 3; i++)
    {
      BobguiAllocation child_allocation;

      if (child[i] == NULL)
        continue;

      if (self->orientation == BOBGUI_ORIENTATION_HORIZONTAL)
        {
          child_allocation.x = child_pos[i];
          child_allocation.y = 0;
          child_allocation.width = child_size[i];
          child_allocation.height = height;
        }
      else
        {
          child_allocation.x = 0;
          child_allocation.y = child_pos[i];
          child_allocation.width = width;
          child_allocation.height = child_size[i];
        }

      bobgui_widget_size_allocate (child[i], &child_allocation, baseline);
    }
}

static void
bobgui_center_layout_set_property (GObject      *object,
                                guint         prop_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  BobguiCenterLayout *self = BOBGUI_CENTER_LAYOUT (object);

  switch (prop_id)
    {
    case PROP_SHRINK_CENTER_LAST:
      bobgui_center_layout_set_shrink_center_last (self, g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_center_layout_get_property (GObject    *object,
                                guint       prop_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  BobguiCenterLayout *self = BOBGUI_CENTER_LAYOUT (object);

  switch (prop_id)
    {
    case PROP_SHRINK_CENTER_LAST:
      g_value_set_boolean (value, self->shrink_center_last);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_center_layout_dispose (GObject *object)
{
  BobguiCenterLayout *self = BOBGUI_CENTER_LAYOUT (object);

  g_clear_object (&self->start_widget);
  g_clear_object (&self->center_widget);
  g_clear_object (&self->end_widget);

  G_OBJECT_CLASS (bobgui_center_layout_parent_class)->dispose (object);
}

static void
bobgui_center_layout_class_init (BobguiCenterLayoutClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  BobguiLayoutManagerClass *layout_class = BOBGUI_LAYOUT_MANAGER_CLASS (klass);

  object_class->get_property = bobgui_center_layout_get_property;
  object_class->set_property = bobgui_center_layout_set_property;
  object_class->dispose = bobgui_center_layout_dispose;

  layout_class->get_request_mode = bobgui_center_layout_get_request_mode;
  layout_class->measure = bobgui_center_layout_measure;
  layout_class->allocate = bobgui_center_layout_allocate;

  /**
   * BobguiCenterLayout:shrink-center-last:
   *
   * Whether to shrink the center widget after other children.
   *
   * By default, when there's no space to give all three children their
   * natural widths, the start and end widgets start shrinking and the
   * center child keeps natural width until they reach minimum width.
   *
   * If set to `FALSE`, start and end widgets keep natural width and the
   * center widget starts shrinking instead.
   *
   * Since: 4.12
   */
  props[PROP_SHRINK_CENTER_LAST] =
      g_param_spec_boolean ("shrink-center-last", NULL, NULL,
                            TRUE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, LAST_PROP, props);
}

static void
bobgui_center_layout_init (BobguiCenterLayout *self)
{
  self->orientation = BOBGUI_ORIENTATION_HORIZONTAL;
  self->baseline_pos = BOBGUI_BASELINE_POSITION_CENTER;
  self->shrink_center_last = TRUE;
}

/**
 * bobgui_center_layout_new:
 *
 * Creates a new `BobguiCenterLayout`.
 *
 * Returns: the newly created `BobguiCenterLayout`
 */
BobguiLayoutManager *
bobgui_center_layout_new (void)
{
  return g_object_new (BOBGUI_TYPE_CENTER_LAYOUT, NULL);
}

/**
 * bobgui_center_layout_set_orientation:
 * @self: a `BobguiCenterLayout`
 * @orientation: the new orientation
 *
 * Sets the orientation of @self.
 */
void
bobgui_center_layout_set_orientation (BobguiCenterLayout *self,
                                   BobguiOrientation   orientation)
{
  g_return_if_fail (BOBGUI_IS_CENTER_LAYOUT (self));

  if (orientation != self->orientation)
    {
      self->orientation = orientation;
      bobgui_layout_manager_layout_changed (BOBGUI_LAYOUT_MANAGER (self));
    }
}

/**
 * bobgui_center_layout_get_orientation:
 * @self: a `BobguiCenterLayout`
 *
 * Gets the current orienration of the layout manager.
 *
 * Returns: The current orientation of @self
 */
BobguiOrientation
bobgui_center_layout_get_orientation (BobguiCenterLayout *self)
{
  g_return_val_if_fail (BOBGUI_IS_CENTER_LAYOUT (self), BOBGUI_ORIENTATION_HORIZONTAL);

  return self->orientation;
}

/**
 * bobgui_center_layout_set_baseline_position:
 * @self: a `BobguiCenterLayout`
 * @baseline_position: the new baseline position
 *
 * Sets the new baseline position of @self
 */
void
bobgui_center_layout_set_baseline_position (BobguiCenterLayout     *self,
                                         BobguiBaselinePosition  baseline_position)
{
  g_return_if_fail (BOBGUI_IS_CENTER_LAYOUT (self));

  if (baseline_position != self->baseline_pos)
    {
      self->baseline_pos = baseline_position;
      bobgui_layout_manager_layout_changed (BOBGUI_LAYOUT_MANAGER (self));
    }
}

/**
 * bobgui_center_layout_get_baseline_position:
 * @self: a `BobguiCenterLayout`
 *
 * Returns the baseline position of the layout.
 *
 * Returns: The current baseline position of @self.
 */
BobguiBaselinePosition
bobgui_center_layout_get_baseline_position (BobguiCenterLayout *self)
{
  g_return_val_if_fail (BOBGUI_IS_CENTER_LAYOUT (self), BOBGUI_BASELINE_POSITION_TOP);

  return self->baseline_pos;
}

/**
 * bobgui_center_layout_set_start_widget:
 * @self: a `BobguiCenterLayout`
 * @widget: (nullable): the new start widget
 *
 * Sets the new start widget of @self.
 *
 * To remove the existing start widget, pass %NULL.
 */
void
bobgui_center_layout_set_start_widget (BobguiCenterLayout *self,
                                    BobguiWidget       *widget)
{
  g_return_if_fail (BOBGUI_IS_CENTER_LAYOUT (self));
  g_return_if_fail (widget == NULL || BOBGUI_IS_WIDGET (widget));

  if (g_set_object (&self->start_widget, widget))
    bobgui_layout_manager_layout_changed (BOBGUI_LAYOUT_MANAGER (self));
}

/**
 * bobgui_center_layout_get_start_widget:
 * @self: a `BobguiCenterLayout`
 *
 * Returns the start widget of the layout.
 *
 * Returns: (nullable) (transfer none): The current start widget of @self
 */
BobguiWidget *
bobgui_center_layout_get_start_widget (BobguiCenterLayout *self)
{
  g_return_val_if_fail (BOBGUI_IS_CENTER_LAYOUT (self), NULL);

  return self->start_widget;
}

/**
 * bobgui_center_layout_set_center_widget:
 * @self: a `BobguiCenterLayout`
 * @widget: (nullable): the new center widget
 *
 * Sets the new center widget of @self.
 *
 * To remove the existing center widget, pass %NULL.
 */
void
bobgui_center_layout_set_center_widget (BobguiCenterLayout *self,
                                     BobguiWidget       *widget)
{
  g_return_if_fail (BOBGUI_IS_CENTER_LAYOUT (self));
  g_return_if_fail (widget == NULL || BOBGUI_IS_WIDGET (widget));

  if (g_set_object (&self->center_widget, widget))
    bobgui_layout_manager_layout_changed (BOBGUI_LAYOUT_MANAGER (self));
}

/**
 * bobgui_center_layout_get_center_widget:
 * @self: a `BobguiCenterLayout`
 *
 * Returns the center widget of the layout.
 *
 * Returns: (nullable) (transfer none): the current center widget of @self
 */
BobguiWidget *
bobgui_center_layout_get_center_widget (BobguiCenterLayout *self)
{
  g_return_val_if_fail (BOBGUI_IS_CENTER_LAYOUT (self), NULL);

  return self->center_widget;
}

/**
 * bobgui_center_layout_set_end_widget:
 * @self: a `BobguiCenterLayout`
 * @widget: (nullable): the new end widget
 *
 * Sets the new end widget of @self.
 *
 * To remove the existing center widget, pass %NULL.
 */
void
bobgui_center_layout_set_end_widget (BobguiCenterLayout *self,
                                  BobguiWidget       *widget)
{
  g_return_if_fail (BOBGUI_IS_CENTER_LAYOUT (self));
  g_return_if_fail (widget == NULL || BOBGUI_IS_WIDGET (widget));

  if (g_set_object (&self->end_widget, widget))
    bobgui_layout_manager_layout_changed (BOBGUI_LAYOUT_MANAGER (self));
}

/**
 * bobgui_center_layout_get_end_widget:
 * @self: a `BobguiCenterLayout`
 *
 * Returns the end widget of the layout.
 *
 * Returns: (nullable) (transfer none): the current end widget of @self
 */
BobguiWidget *
bobgui_center_layout_get_end_widget (BobguiCenterLayout *self)
{
  g_return_val_if_fail (BOBGUI_IS_CENTER_LAYOUT (self), NULL);

  return self->end_widget;
}

/**
 * bobgui_center_layout_set_shrink_center_last:
 * @self: a `BobguiCenterLayout`
 * @shrink_center_last: whether to shrink the center widget after others
 *
 * Sets whether to shrink the center widget after other children.
 *
 * By default, when there's no space to give all three children their
 * natural widths, the start and end widgets start shrinking and the
 * center child keeps natural width until they reach minimum width.
 *
 * If set to `FALSE`, start and end widgets keep natural width and the
 * center widget starts shrinking instead.
 *
 * Since: 4.12
 */
void
bobgui_center_layout_set_shrink_center_last (BobguiCenterLayout *self,
                                          gboolean         shrink_center_last)
{
  g_return_if_fail (BOBGUI_IS_CENTER_LAYOUT (self));

  shrink_center_last = !!shrink_center_last;

  if (shrink_center_last == self->shrink_center_last)
    return;

  self->shrink_center_last = shrink_center_last;

  bobgui_layout_manager_layout_changed (BOBGUI_LAYOUT_MANAGER (self));

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_SHRINK_CENTER_LAST]);
}

/**
 * bobgui_center_layout_get_shrink_center_last:
 * @self: a `BobguiCenterLayout`
 *
 * Gets whether @self shrinks the center widget after other children.
 *
 * Returns: whether to shrink the center widget after others
 *
 * Since: 4.12
 */
gboolean
bobgui_center_layout_get_shrink_center_last (BobguiCenterLayout *self)
{
  g_return_val_if_fail (BOBGUI_IS_CENTER_LAYOUT (self), FALSE);

  return self->shrink_center_last;
}
