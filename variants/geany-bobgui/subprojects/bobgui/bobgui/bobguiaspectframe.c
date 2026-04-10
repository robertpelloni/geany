/* BOBGUI - The Bobgui Framework
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * BobguiAspectFrame: Ensure that the child window has a specified aspect ratio
 *    or, if obey_child, has the same aspect ratio as its requested size
 *
 *     Copyright Owen Taylor                          4/9/97
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
 * Modified by the BOBGUI Team and others 1997-2001.  See the AUTHORS
 * file for a list of people on the BOBGUI Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BOBGUI at ftp://ftp.bobgui.org/pub/bobgui/.
 */

/**
 * BobguiAspectFrame:
 *
 * Preserves the aspect ratio of its child.
 *
 * The frame can respect the aspect ratio of the child widget,
 * or use its own aspect ratio.
 *
 * # CSS nodes
 *
 * `BobguiAspectFrame` uses a CSS node with name `aspectframe`.
 *
 * # Accessibility
 *
 * Until BOBGUI 4.10, `BobguiAspectFrame` used the [enum@Bobgui.AccessibleRole.group] role.
 *
 * Starting from BOBGUI 4.12, `BobguiAspectFrame` uses the [enum@Bobgui.AccessibleRole.generic] role.

 */

#include "config.h"

#include "bobguiaspectframe.h"

#include "bobguisizerequest.h"

#include "bobguibuildable.h"

#include "bobguiwidgetprivate.h"
#include "bobguiprivate.h"
#include "bobguibuilderprivate.h"


typedef struct _BobguiAspectFrameClass BobguiAspectFrameClass;

struct _BobguiAspectFrame
{
  BobguiWidget parent_instance;

  BobguiWidget    *child;
  float         xalign;
  float         yalign;
  float         ratio;
  int           cached_min_size[2];
  int           cached_min_baseline;
  gboolean      obey_child;
};

struct _BobguiAspectFrameClass
{
  BobguiWidgetClass parent_class;
};

enum {
  PROP_0,
  PROP_XALIGN,
  PROP_YALIGN,
  PROP_RATIO,
  PROP_OBEY_CHILD,
  PROP_CHILD
};

static void bobgui_aspect_frame_dispose      (GObject         *object);
static void bobgui_aspect_frame_set_property (GObject         *object,
                                           guint            prop_id,
                                           const GValue    *value,
                                           GParamSpec      *pspec);
static void bobgui_aspect_frame_get_property (GObject         *object,
                                           guint            prop_id,
                                           GValue          *value,
                                           GParamSpec      *pspec);
static void bobgui_aspect_frame_size_allocate (BobguiWidget      *widget,
                                            int             width,
                                            int             height,
                                            int             baseline);
static void bobgui_aspect_frame_measure       (BobguiWidget      *widget,
                                            BobguiOrientation  orientation,
                                            int             for_size,
                                            int             *minimum,
                                            int             *natural,
                                            int             *minimum_baseline,
                                            int             *natural_baseline);

static void bobgui_aspect_frame_compute_expand (BobguiWidget     *widget,
                                             gboolean      *hexpand,
                                             gboolean      *vexpand);
static BobguiSizeRequestMode
            bobgui_aspect_frame_get_request_mode (BobguiWidget *widget);

static void bobgui_aspect_frame_buildable_init (BobguiBuildableIface *iface);

#define MAX_RATIO 10000.0
#define MIN_RATIO 0.0001


G_DEFINE_TYPE_WITH_CODE (BobguiAspectFrame, bobgui_aspect_frame, BOBGUI_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_BUILDABLE,
                                                bobgui_aspect_frame_buildable_init))


static void
bobgui_aspect_frame_class_init (BobguiAspectFrameClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (class);

  gobject_class->dispose = bobgui_aspect_frame_dispose;
  gobject_class->set_property = bobgui_aspect_frame_set_property;
  gobject_class->get_property = bobgui_aspect_frame_get_property;

  widget_class->measure = bobgui_aspect_frame_measure;
  widget_class->size_allocate = bobgui_aspect_frame_size_allocate;
  widget_class->compute_expand = bobgui_aspect_frame_compute_expand;
  widget_class->get_request_mode = bobgui_aspect_frame_get_request_mode;

  /**
   * BobguiAspectFrame:xalign:
   *
   * The horizontal alignment of the child.
   */
  g_object_class_install_property (gobject_class,
                                   PROP_XALIGN,
                                   g_param_spec_float ("xalign", NULL, NULL,
                                                       0.0, 1.0, 0.5,
                                                       BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));
  /**
   * BobguiAspectFrame:yalign:
   *
   * The vertical alignment of the child.
   */
  g_object_class_install_property (gobject_class,
                                   PROP_YALIGN,
                                   g_param_spec_float ("yalign", NULL, NULL,
                                                       0.0, 1.0, 0.5,
                                                       BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));
  /**
   * BobguiAspectFrame:ratio:
   *
   * The aspect ratio to be used by the `BobguiAspectFrame`.
   *
   * This property is only used if
   * [property@Bobgui.AspectFrame:obey-child] is set to %FALSE.
   */
  g_object_class_install_property (gobject_class,
                                   PROP_RATIO,
                                   g_param_spec_float ("ratio", NULL, NULL,
                                                       MIN_RATIO, MAX_RATIO, 1.0,
                                                       BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));
  /**
   * BobguiAspectFrame:obey-child:
   *
   * Whether the `BobguiAspectFrame` should use the aspect ratio of its child.
   */
  g_object_class_install_property (gobject_class,
                                   PROP_OBEY_CHILD,
                                   g_param_spec_boolean ("obey-child", NULL, NULL,
                                                         TRUE,
                                                         BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));
  /**
   * BobguiAspectFrame:child:
   *
   * The child widget.
   */
  g_object_class_install_property (gobject_class,
                                   PROP_CHILD,
                                   g_param_spec_object ("child", NULL, NULL,
                                                        BOBGUI_TYPE_WIDGET,
                                                        BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  bobgui_widget_class_set_css_name (BOBGUI_WIDGET_CLASS (class), I_("aspectframe"));
  bobgui_widget_class_set_accessible_role (BOBGUI_WIDGET_CLASS (class), BOBGUI_ACCESSIBLE_ROLE_GENERIC);
}

static void
bobgui_aspect_frame_resize_func (BobguiWidget *widget)
{
  BobguiAspectFrame *self = BOBGUI_ASPECT_FRAME (widget);

  self->cached_min_size[BOBGUI_ORIENTATION_VERTICAL] = -1;
  self->cached_min_size[BOBGUI_ORIENTATION_HORIZONTAL] = -1;
  self->cached_min_baseline = -1;
}

static void
bobgui_aspect_frame_init (BobguiAspectFrame *self)
{
  BobguiWidget *widget = BOBGUI_WIDGET (self);

  self->xalign = 0.5;
  self->yalign = 0.5;
  self->ratio = 1.0;
  self->obey_child = TRUE;

  widget->priv->resize_func = bobgui_aspect_frame_resize_func;
}

static void
bobgui_aspect_frame_dispose (GObject *object)
{
  BobguiAspectFrame *self = BOBGUI_ASPECT_FRAME (object);

  g_clear_pointer (&self->child, bobgui_widget_unparent);

  G_OBJECT_CLASS (bobgui_aspect_frame_parent_class)->dispose (object);
}

static void
bobgui_aspect_frame_set_property (GObject         *object,
                               guint            prop_id,
                               const GValue    *value,
                               GParamSpec      *pspec)
{
  BobguiAspectFrame *self = BOBGUI_ASPECT_FRAME (object);

  switch (prop_id)
    {
      /* g_object_notify is handled by the _frame_set function */
    case PROP_XALIGN:
      bobgui_aspect_frame_set_xalign (self, g_value_get_float (value));
      break;
    case PROP_YALIGN:
      bobgui_aspect_frame_set_yalign (self, g_value_get_float (value));
      break;
    case PROP_RATIO:
      bobgui_aspect_frame_set_ratio (self, g_value_get_float (value));
      break;
    case PROP_OBEY_CHILD:
      bobgui_aspect_frame_set_obey_child (self, g_value_get_boolean (value));
      break;
    case PROP_CHILD:
      bobgui_aspect_frame_set_child (self, g_value_get_object (value));
      break;
    default:
       G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_aspect_frame_get_property (GObject         *object,
                               guint            prop_id,
                               GValue          *value,
                               GParamSpec      *pspec)
{
  BobguiAspectFrame *self = BOBGUI_ASPECT_FRAME (object);

  switch (prop_id)
    {
    case PROP_XALIGN:
      g_value_set_float (value, self->xalign);
      break;
    case PROP_YALIGN:
      g_value_set_float (value, self->yalign);
      break;
    case PROP_RATIO:
      g_value_set_float (value, self->ratio);
      break;
    case PROP_OBEY_CHILD:
      g_value_set_boolean (value, self->obey_child);
      break;
    case PROP_CHILD:
      g_value_set_object (value, bobgui_aspect_frame_get_child (self));
      break;
    default:
       G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static BobguiBuildableIface *parent_buildable_iface;

static void
bobgui_aspect_frame_buildable_add_child (BobguiBuildable *buildable,
                                      BobguiBuilder   *builder,
                                      GObject      *child,
                                      const char   *type)
{
  if (BOBGUI_IS_WIDGET (child))
    {
      bobgui_buildable_child_deprecation_warning (buildable, builder, NULL, "child");
      bobgui_aspect_frame_set_child (BOBGUI_ASPECT_FRAME (buildable), BOBGUI_WIDGET (child));
    }
  else
    {
      parent_buildable_iface->add_child (buildable, builder, child, type);
    }
}

static void
bobgui_aspect_frame_buildable_init (BobguiBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);

  iface->add_child = bobgui_aspect_frame_buildable_add_child;
}

/**
 * bobgui_aspect_frame_new:
 * @xalign: Horizontal alignment of the child within the parent.
 *   Ranges from 0.0 (left aligned) to 1.0 (right aligned)
 * @yalign: Vertical alignment of the child within the parent.
 *   Ranges from 0.0 (top aligned) to 1.0 (bottom aligned)
 * @ratio: The desired aspect ratio.
 * @obey_child: If %TRUE, @ratio is ignored, and the aspect
 *   ratio is taken from the requistion of the child.
 *
 * Create a new `BobguiAspectFrame`.
 *
 * Returns: the new `BobguiAspectFrame`.
 */
BobguiWidget *
bobgui_aspect_frame_new (float    xalign,
                      float    yalign,
                      float    ratio,
                      gboolean obey_child)
{
  BobguiAspectFrame *self;

  self = g_object_new (BOBGUI_TYPE_ASPECT_FRAME, NULL);

  self->xalign = CLAMP (xalign, 0.0, 1.0);
  self->yalign = CLAMP (yalign, 0.0, 1.0);
  self->ratio = CLAMP (ratio, MIN_RATIO, MAX_RATIO);
  self->obey_child = obey_child != FALSE;

  return BOBGUI_WIDGET (self);
}

/**
 * bobgui_aspect_frame_set_xalign:
 * @self: a `BobguiAspectFrame`
 * @xalign: horizontal alignment, from 0.0 (left aligned) to 1.0 (right aligned)
 *
 * Sets the horizontal alignment of the child within the allocation
 * of the `BobguiAspectFrame`.
 */
void
bobgui_aspect_frame_set_xalign (BobguiAspectFrame *self,
                             float           xalign)
{
  g_return_if_fail (BOBGUI_IS_ASPECT_FRAME (self));

  xalign = CLAMP (xalign, 0.0, 1.0);

  if (self->xalign == xalign)
    return;

  self->xalign = xalign;

  g_object_notify (G_OBJECT (self), "xalign");
  bobgui_widget_queue_allocate (BOBGUI_WIDGET (self));
}

/**
 * bobgui_aspect_frame_get_xalign:
 * @self: a `BobguiAspectFrame`
 *
 * Returns the horizontal alignment of the child within the
 * allocation of the `BobguiAspectFrame`.
 *
 * Returns: the horizontal alignment
 */
float
bobgui_aspect_frame_get_xalign (BobguiAspectFrame *self)
{
  g_return_val_if_fail (BOBGUI_IS_ASPECT_FRAME (self), 0.5);

  return self->xalign;
}

/**
 * bobgui_aspect_frame_set_yalign:
 * @self: a `BobguiAspectFrame`
 * @yalign: horizontal alignment, from 0.0 (top aligned) to 1.0 (bottom aligned)
 *
 * Sets the vertical alignment of the child within the allocation
 * of the `BobguiAspectFrame`.
 */
void
bobgui_aspect_frame_set_yalign (BobguiAspectFrame *self,
                             float           yalign)
{
  g_return_if_fail (BOBGUI_IS_ASPECT_FRAME (self));

  yalign = CLAMP (yalign, 0.0, 1.0);

  if (self->yalign == yalign)
    return;

  self->yalign = yalign;

  g_object_notify (G_OBJECT (self), "yalign");
  if (self->cached_min_baseline != -1)
    bobgui_widget_queue_resize (BOBGUI_WIDGET (self));
  else
    bobgui_widget_queue_allocate (BOBGUI_WIDGET (self));
}

/**
 * bobgui_aspect_frame_get_yalign:
 * @self: a `BobguiAspectFrame`
 *
 * Returns the vertical alignment of the child within the
 * allocation of the `BobguiAspectFrame`.
 *
 * Returns: the vertical alignment
 */
float
bobgui_aspect_frame_get_yalign (BobguiAspectFrame *self)
{
  g_return_val_if_fail (BOBGUI_IS_ASPECT_FRAME (self), 0.5);

  return self->yalign;
}

/**
 * bobgui_aspect_frame_set_ratio:
 * @self: a `BobguiAspectFrame`
 * @ratio: aspect ratio of the child
 *
 * Sets the desired aspect ratio of the child.
 */
void
bobgui_aspect_frame_set_ratio (BobguiAspectFrame *self,
                            float           ratio)
{
  g_return_if_fail (BOBGUI_IS_ASPECT_FRAME (self));

  ratio = CLAMP (ratio, MIN_RATIO, MAX_RATIO);

  if (self->ratio == ratio)
    return;

  self->ratio = ratio;

  g_object_notify (G_OBJECT (self), "ratio");
  if (!self->obey_child)
    bobgui_widget_queue_resize (BOBGUI_WIDGET (self));
}

/**
 * bobgui_aspect_frame_get_ratio:
 * @self: a `BobguiAspectFrame`
 *
 * Returns the desired aspect ratio of the child.
 *
 * Returns: the desired aspect ratio
 */
float
bobgui_aspect_frame_get_ratio (BobguiAspectFrame *self)
{
  g_return_val_if_fail (BOBGUI_IS_ASPECT_FRAME (self), 1.0);

  return self->ratio;
}

/**
 * bobgui_aspect_frame_set_obey_child:
 * @self: a `BobguiAspectFrame`
 * @obey_child: If %TRUE, @ratio is ignored, and the aspect
 *    ratio is taken from the requisition of the child.
 *
 * Sets whether the aspect ratio of the child's size
 * request should override the set aspect ratio of
 * the `BobguiAspectFrame`.
 */
void
bobgui_aspect_frame_set_obey_child (BobguiAspectFrame *self,
                                 gboolean        obey_child)
{
  g_return_if_fail (BOBGUI_IS_ASPECT_FRAME (self));

  if (self->obey_child == obey_child)
    return;

  self->obey_child = obey_child;

  g_object_notify (G_OBJECT (self), "obey-child");
  bobgui_widget_queue_resize (BOBGUI_WIDGET (self));

}

/**
 * bobgui_aspect_frame_get_obey_child:
 * @self: a `BobguiAspectFrame`
 *
 * Returns whether the child's size request should override
 * the set aspect ratio of the `BobguiAspectFrame`.
 *
 * Returns: whether to obey the child's size request
 */
gboolean
bobgui_aspect_frame_get_obey_child (BobguiAspectFrame *self)
{
  g_return_val_if_fail (BOBGUI_IS_ASPECT_FRAME (self), TRUE);

  return self->obey_child;
}

static double
get_effective_ratio (BobguiAspectFrame *self)
{
  double ratio;
  BobguiRequisition child_requisition;

  if (!self->obey_child || !self->child)
    return self->ratio;

  bobgui_widget_get_preferred_size (self->child, NULL, &child_requisition);

  if (child_requisition.height != 0)
    {
      ratio = ((double) child_requisition.width /
               child_requisition.height);
      if (ratio < MIN_RATIO)
        ratio = MIN_RATIO;
    }
  else if (child_requisition.width != 0)
    ratio = MAX_RATIO;
  else
    ratio = 1.0;

  return ratio;
}

static void
get_full_allocation (BobguiAspectFrame *self,
                     BobguiAllocation  *child_allocation)
{
  child_allocation->x = 0;
  child_allocation->y = 0;
  child_allocation->width = bobgui_widget_get_width (BOBGUI_WIDGET (self));
  child_allocation->height = bobgui_widget_get_height (BOBGUI_WIDGET (self));
}

static void
compute_child_allocation (BobguiAspectFrame *self,
                          BobguiAllocation  *child_allocation)
{
  double ratio;

  if (self->child && bobgui_widget_get_visible (self->child))
    {
      BobguiAllocation full_allocation;

      get_full_allocation (self, &full_allocation);
      ratio = get_effective_ratio (self);

      if (ratio * full_allocation.height > full_allocation.width)
        {
          child_allocation->width = full_allocation.width;
          child_allocation->height = full_allocation.width / ratio + 0.5;
        }
      else
        {
          child_allocation->width = ratio * full_allocation.height + 0.5;
          child_allocation->height = full_allocation.height;
        }

      child_allocation->x = full_allocation.x + self->xalign * (full_allocation.width - child_allocation->width);
      child_allocation->y = full_allocation.y + self->yalign * (full_allocation.height - child_allocation->height);
    }
  else
    get_full_allocation (self, child_allocation);
}

static inline double
apply_ratio (int            for_size,
             double         ratio,
             BobguiOrientation orientation)
{
  if (orientation == BOBGUI_ORIENTATION_HORIZONTAL)
    /* width = height * ratio */
    return for_size * ratio;
  else
    /* height = width / ratio */
    return for_size / ratio;
}

static void
bobgui_aspect_frame_compute_minimum_size (BobguiAspectFrame *self,
                                       double          ratio)
{
  BobguiSizeRequestMode request_mode;
  BobguiOrientation orientation, opposite_orientation;
  int start_size, end_size, opposite_size, baseline;

  /* From an allocation that BobguiAspectFrame itself receives, it carves
   * out an allocation for its child that has the desired aspect ratio.
   * The possible sizes allocated to the child are ones that have that
   * aspect ratio; effectively the possible child size varies with a
   * single degree of freedom as opposed to the usual two.  Therefore,
   * there is a single minimum size for the child allocation: it is the
   * smallest among sizes having the desired aspect ratio that is still
   * acceptable to the child.
   *
   * While BobguiAspectFrame is not constant-size because our natural size
   * proportionally depends on the size in the opposite orientation,
   * its minimum width and height don't depend on the available size in
   * the opposite orientation.  This is why we can compute this minimum
   * size once, and then use the cached values whenever we're measured.
   */

  request_mode = bobgui_widget_get_request_mode (self->child);
  if (request_mode == BOBGUI_SIZE_REQUEST_CONSTANT_SIZE)
    {
      int min_width, min_height, min_baseline;

      bobgui_widget_measure (self->child, BOBGUI_ORIENTATION_HORIZONTAL, -1,
                          &min_width, NULL, NULL, NULL);
      bobgui_widget_measure (self->child, BOBGUI_ORIENTATION_VERTICAL, -1,
                          &min_height, NULL, &min_baseline, NULL);
      self->cached_min_size[BOBGUI_ORIENTATION_HORIZONTAL] = MAX (min_width, ceil (min_height * ratio));
      self->cached_min_size[BOBGUI_ORIENTATION_VERTICAL] = MAX (min_height, ceil (min_width / ratio));

      if (min_baseline != -1)
        self->cached_min_baseline = min_baseline + round (self->yalign *
          (self->cached_min_size[BOBGUI_ORIENTATION_VERTICAL] - min_height));
      else
        self->cached_min_baseline = -1;
      return;
    }
  else if (request_mode == BOBGUI_SIZE_REQUEST_HEIGHT_FOR_WIDTH)
    {
      orientation = BOBGUI_ORIENTATION_HORIZONTAL;
      opposite_orientation = BOBGUI_ORIENTATION_VERTICAL;
    }
  else
    {
      orientation = BOBGUI_ORIENTATION_VERTICAL;
      opposite_orientation = BOBGUI_ORIENTATION_HORIZONTAL;
    }

  /* Search, among sizes that are acceptable to the child, for the one
   * that fits the desired ratio best.
   *
   * Start from the overall minimum size along this orientation.
   */
  bobgui_widget_measure (self->child, orientation,
                      -1, &start_size, NULL, NULL, NULL);
  bobgui_widget_measure (self->child, opposite_orientation,
                      start_size, &opposite_size, NULL, NULL, NULL);

  end_size = apply_ratio (opposite_size, ratio, orientation);
  /* See if the minimum size in fact already fits */
  if (start_size >= end_size)
    {
      end_size = start_size;
      goto found;
    }
  /* Otherwise, we know that end_size is certain to fit.
   *
   * Binary search for the minimum size that fits.
   *
   * Invariant: start_size doesn't fit, end_size does.
   */
  while (start_size + 1 < end_size)
    {
      int mid_size;

      mid_size = (start_size + 1 + end_size) / 2;
      g_assert (mid_size > start_size);
      bobgui_widget_measure (self->child, opposite_orientation,
                          mid_size, &opposite_size, NULL, NULL, NULL);
      if (mid_size >= apply_ratio (opposite_size, ratio, orientation))
        end_size = mid_size;
      else
        start_size = mid_size;
    }

found:
  self->cached_min_size[orientation] = end_size;
  self->cached_min_size[opposite_orientation] =
    ceil (apply_ratio (end_size, ratio, opposite_orientation));

  /* Now compute the baseline */
  bobgui_widget_measure (self->child, BOBGUI_ORIENTATION_VERTICAL,
                      self->cached_min_size[BOBGUI_ORIENTATION_HORIZONTAL],
                      &opposite_size, NULL, &baseline, NULL);
  g_assert (opposite_size <= self->cached_min_size[BOBGUI_ORIENTATION_VERTICAL]);
  if (baseline != -1)
    self->cached_min_baseline = baseline + round (self->yalign *
      (self->cached_min_size[BOBGUI_ORIENTATION_VERTICAL] - opposite_size));
  else
    self->cached_min_baseline = -1;
}

static void
bobgui_aspect_frame_measure (BobguiWidget      *widget,
                          BobguiOrientation  orientation,
                          int             for_size,
                          int             *minimum,
                          int             *natural,
                          int             *minimum_baseline,
                          int             *natural_baseline)
{
  BobguiAspectFrame *self = BOBGUI_ASPECT_FRAME (widget);
  double ratio;
  int natural_constraint;

  if (!self->child || !bobgui_widget_get_visible (self->child))
    {
      *minimum = *natural = 0;
      *minimum_baseline = *natural_baseline = -1;
      return;
    }

  ratio = get_effective_ratio (self);

  if (self->cached_min_size[orientation] == -1)
    bobgui_aspect_frame_compute_minimum_size (self, ratio);

  *minimum = self->cached_min_size[orientation];
  if (orientation == BOBGUI_ORIENTATION_VERTICAL)
    *minimum_baseline = self->cached_min_baseline;

  if (for_size != -1)
    {
      /* For any specific size, our natural size follows the ratio */
      if (orientation == BOBGUI_ORIENTATION_HORIZONTAL)
        *natural = ceil (for_size * ratio);
      else
        *natural = floor (for_size / ratio);
      /* Note that (*natural) < (*minimum) could happen due to us
       * finding a close, but imprecise, ratio.
       */
      *natural = MAX (*natural, *minimum);

      if (orientation == BOBGUI_ORIENTATION_VERTICAL && *minimum_baseline != -1)
        {
          int child_min, child_nat;
          int child_min_baseline;

          bobgui_widget_measure (self->child, BOBGUI_ORIENTATION_VERTICAL,
                              for_size, &child_min, &child_nat,
                              &child_min_baseline, natural_baseline);
          if (*natural >= child_nat)
            *natural_baseline += round ((*natural - child_nat) * self->yalign);
          else
            {
              /* Interpolate from child's min baseline to its nat baseline */
              double progress = ((double) (*natural - child_min)) / (child_nat - child_min);
              *natural_baseline = child_min_baseline + round ((*natural_baseline - child_min_baseline) * progress);
            }
        }
    }
  else
    {
      /* Our overall natural size is such that we can fit the child
       * at its natural size, in both orientations.
       */
      bobgui_widget_measure (self->child, orientation, -1,
                          NULL, natural,
                          NULL, natural_baseline);
      if (!self->obey_child)
        {
          int natural_opposite;

          bobgui_widget_measure (self->child,
                              OPPOSITE_ORIENTATION (orientation), -1,
                              NULL, &natural_opposite, NULL, NULL);
          natural_constraint = ceil (apply_ratio (natural_opposite, ratio, orientation));
          natural_constraint = MAX (natural_constraint, *minimum);

          if (*natural_baseline != -1 && natural_constraint > *natural)
            *natural_baseline += round ((natural_constraint - *natural) * self->yalign);
          *natural = MAX (*natural, natural_constraint);
        }
    }
}

static void
bobgui_aspect_frame_size_allocate (BobguiWidget *widget,
                                int        width,
                                int        height,
                                int        baseline)
{
  BobguiAspectFrame *self = BOBGUI_ASPECT_FRAME (widget);
  BobguiAllocation new_allocation;

  compute_child_allocation (self, &new_allocation);

  if (self->child && bobgui_widget_get_visible (self->child))
    bobgui_widget_size_allocate (self->child, &new_allocation, -1);
}

static void
bobgui_aspect_frame_compute_expand (BobguiWidget *widget,
                                 gboolean  *hexpand,
                                 gboolean  *vexpand)
{
  BobguiAspectFrame *self = BOBGUI_ASPECT_FRAME (widget);

  if (self->child)
    {
      *hexpand = bobgui_widget_compute_expand (self->child, BOBGUI_ORIENTATION_HORIZONTAL);
      *vexpand = bobgui_widget_compute_expand (self->child, BOBGUI_ORIENTATION_VERTICAL);
    }
  else
    {
      *hexpand = FALSE;
      *vexpand = FALSE;
    }
}

static BobguiSizeRequestMode
bobgui_aspect_frame_get_request_mode (BobguiWidget *widget)
{
  BobguiAspectFrame *self = BOBGUI_ASPECT_FRAME (widget);
  BobguiSizeRequestMode request_mode;

  if (!self->child || !bobgui_widget_get_visible (self->child))
    return BOBGUI_SIZE_REQUEST_CONSTANT_SIZE;

  /* Our natural size always depends on for-size, so we're never
   * constant-size, even when our child is.
   */

  request_mode = bobgui_widget_get_request_mode (self->child);
  if (request_mode != BOBGUI_SIZE_REQUEST_CONSTANT_SIZE)
    return request_mode;
  return BOBGUI_SIZE_REQUEST_HEIGHT_FOR_WIDTH;
}

/**
 * bobgui_aspect_frame_set_child:
 * @self: a `BobguiAspectFrame`
 * @child: (nullable): the child widget
 *
 * Sets the child widget of @self.
 */
void
bobgui_aspect_frame_set_child (BobguiAspectFrame  *self,
                            BobguiWidget       *child)
{
  g_return_if_fail (BOBGUI_IS_ASPECT_FRAME (self));
  g_return_if_fail (child == NULL || self->child == child || bobgui_widget_get_parent (child) == NULL);

  if (self->child == child)
    return;

  g_clear_pointer (&self->child, bobgui_widget_unparent);

  if (child)
    {
      self->child = child;
      bobgui_widget_set_parent (child, BOBGUI_WIDGET (self));
    }

  g_object_notify (G_OBJECT (self), "child");
}

/**
 * bobgui_aspect_frame_get_child:
 * @self: a `BobguiAspectFrame`
 *
 * Gets the child widget of @self.
 *
 * Returns: (nullable) (transfer none): the child widget of @self
 */
BobguiWidget *
bobgui_aspect_frame_get_child (BobguiAspectFrame *self)
{
  g_return_val_if_fail (BOBGUI_IS_ASPECT_FRAME (self), NULL);

  return self->child;
}
