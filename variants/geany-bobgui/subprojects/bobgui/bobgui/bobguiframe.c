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
#include <string.h>
#include "bobguiframe.h"
#include "bobguilabel.h"
#include "bobguiprivate.h"
#include "bobguitypebuiltins.h"
#include "bobguibuildable.h"
#include "bobguiwidgetprivate.h"
#include "bobguilabel.h"
#include "bobguibuilderprivate.h"

/**
 * BobguiFrame:
 *
 * Surrounds its child with a decorative frame and an optional label.
 *
 * <picture>
 *   <source srcset="frame-dark.png" media="(prefers-color-scheme: dark)">
 *   <img alt="An example BobguiFrame" src="frame.png">
 * </picture>
 *
 * If present, the label is drawn inside the top edge of the frame.
 * The horizontal position of the label can be controlled with
 * [method@Bobgui.Frame.set_label_align].
 *
 * `BobguiFrame` clips its child. You can use this to add rounded corners
 * to widgets, but be aware that it also cuts off shadows.
 *
 * # BobguiFrame as BobguiBuildable
 *
 * An example of a UI definition fragment with BobguiFrame:
 *
 * ```xml
 * <object class="BobguiFrame">
 *   <property name="label-widget">
 *     <object class="BobguiLabel" id="frame_label"/>
 *   </property>
 *   <property name="child">
 *     <object class="BobguiEntry" id="frame_content"/>
 *   </property>
 * </object>
 * ```
 *
 * # CSS nodes
 *
 * ```
 * frame
 * ├── <label widget>
 * ╰── <child>
 * ```
 *
 * `BobguiFrame` has a main CSS node with name “frame”, which is used to draw the
 * visible border. You can set the appearance of the border using CSS properties
 * like “border-style” on this node.
 *
 * # Accessibility
 *
 * `BobguiFrame` uses the [enum@Bobgui.AccessibleRole.group] role.
 */

typedef struct
{
  /* Properties */
  BobguiWidget *label_widget;
  BobguiWidget *child;

  guint has_frame : 1;
  float label_xalign;
} BobguiFramePrivate;

enum {
  PROP_0,
  PROP_LABEL,
  PROP_LABEL_XALIGN,
  PROP_LABEL_WIDGET,
  PROP_CHILD,
  LAST_PROP
};

static GParamSpec *frame_props[LAST_PROP];

static void bobgui_frame_dispose      (GObject      *object);
static void bobgui_frame_set_property (GObject      *object,
                                    guint         param_id,
                                    const GValue *value,
                                    GParamSpec   *pspec);
static void bobgui_frame_get_property (GObject     *object,
                                    guint        param_id,
                                    GValue      *value,
                                    GParamSpec  *pspec);
static void bobgui_frame_size_allocate (BobguiWidget  *widget,
                                     int         width,
                                     int         height,
                                     int         baseline);
static void bobgui_frame_real_compute_child_allocation (BobguiFrame      *frame,
                                                     BobguiAllocation *child_allocation);

/* BobguiBuildable */
static void bobgui_frame_buildable_init                (BobguiBuildableIface *iface);
static void bobgui_frame_buildable_add_child           (BobguiBuildable *buildable,
                                                     BobguiBuilder   *builder,
                                                     GObject      *child,
                                                     const char   *type);
static void     bobgui_frame_measure (BobguiWidget           *widget,
                                   BobguiOrientation       orientation,
                                   int                  for_size,
                                   int                 *minimum_size,
                                   int                 *natural_size,
                                   int                 *minimum_baseline,
                                   int                 *natural_baseline);
static void     bobgui_frame_compute_expand (BobguiWidget *widget,
                                          gboolean  *hexpand,
                                          gboolean  *vexpand);
static BobguiSizeRequestMode bobgui_frame_get_request_mode (BobguiWidget *widget);

G_DEFINE_TYPE_WITH_CODE (BobguiFrame, bobgui_frame, BOBGUI_TYPE_WIDGET,
                         G_ADD_PRIVATE (BobguiFrame)
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_BUILDABLE,
                                                bobgui_frame_buildable_init))

static void
bobgui_frame_class_init (BobguiFrameClass *class)
{
  GObjectClass *gobject_class;
  BobguiWidgetClass *widget_class;

  gobject_class = (GObjectClass*) class;
  widget_class = BOBGUI_WIDGET_CLASS (class);

  gobject_class->dispose = bobgui_frame_dispose;
  gobject_class->set_property = bobgui_frame_set_property;
  gobject_class->get_property = bobgui_frame_get_property;

  widget_class->size_allocate = bobgui_frame_size_allocate;
  widget_class->measure = bobgui_frame_measure;
  widget_class->compute_expand = bobgui_frame_compute_expand;
  widget_class->get_request_mode = bobgui_frame_get_request_mode;

  class->compute_child_allocation = bobgui_frame_real_compute_child_allocation;

  /**
   * BobguiFrame:label:
   *
   * Text of the frame's label.
   */
  frame_props[PROP_LABEL] =
      g_param_spec_string ("label", NULL, NULL,
                           NULL,
                           BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiFrame:label-xalign: (getter get_label_align) (setter set_label_align)
   *
   * The horizontal alignment of the label.
   */
  frame_props[PROP_LABEL_XALIGN] =
      g_param_spec_float ("label-xalign", NULL, NULL,
                          0.0, 1.0,
                          0.0,
                          BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiFrame:label-widget:
   *
   * Widget to display in place of the usual frame label.
   */
  frame_props[PROP_LABEL_WIDGET] =
      g_param_spec_object ("label-widget", NULL, NULL,
                           BOBGUI_TYPE_WIDGET,
                           BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiFrame:child:
   *
   * The child widget.
   */
  frame_props[PROP_CHILD] =
      g_param_spec_object ("child", NULL, NULL,
                           BOBGUI_TYPE_WIDGET,
                           BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (gobject_class, LAST_PROP, frame_props);

  bobgui_widget_class_set_css_name (widget_class, I_("frame"));
  bobgui_widget_class_set_accessible_role (widget_class, BOBGUI_ACCESSIBLE_ROLE_GROUP);
}

static BobguiBuildableIface *parent_buildable_iface;

static void
bobgui_frame_buildable_init (BobguiBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);

  iface->add_child = bobgui_frame_buildable_add_child;
}

static void
bobgui_frame_buildable_add_child (BobguiBuildable *buildable,
                               BobguiBuilder   *builder,
                               GObject      *child,
                               const char   *type)
{
  if (type && strcmp (type, "label") == 0)
    {
      bobgui_buildable_child_deprecation_warning (buildable, builder, "label", "label-widget");
      bobgui_frame_set_label_widget (BOBGUI_FRAME (buildable), BOBGUI_WIDGET (child));
    }
  else if (BOBGUI_IS_WIDGET (child))
    {
      bobgui_buildable_child_deprecation_warning (buildable, builder, NULL, "child");
      bobgui_frame_set_child (BOBGUI_FRAME (buildable), BOBGUI_WIDGET (child));
    }
  else
    {
      parent_buildable_iface->add_child (buildable, builder, child, type);
    }
}

static void
bobgui_frame_init (BobguiFrame *frame)
{
  BobguiFramePrivate *priv = bobgui_frame_get_instance_private (frame);

  bobgui_widget_set_overflow (BOBGUI_WIDGET (frame), BOBGUI_OVERFLOW_HIDDEN);

  priv->label_widget = NULL;
  priv->has_frame = TRUE;
  priv->label_xalign = 0.0;
}

static void
bobgui_frame_dispose (GObject *object)
{
  BobguiFrame *frame = BOBGUI_FRAME (object);
  BobguiFramePrivate *priv = bobgui_frame_get_instance_private (frame);

  g_clear_pointer (&priv->label_widget, bobgui_widget_unparent);
  g_clear_pointer (&priv->child, bobgui_widget_unparent);

  G_OBJECT_CLASS (bobgui_frame_parent_class)->dispose (object);
}

static void
bobgui_frame_set_property (GObject         *object,
                        guint            prop_id,
                        const GValue    *value,
                        GParamSpec      *pspec)
{
  BobguiFrame *frame = BOBGUI_FRAME (object);

  switch (prop_id)
    {
    case PROP_LABEL:
      bobgui_frame_set_label (frame, g_value_get_string (value));
      break;
    case PROP_LABEL_XALIGN:
      bobgui_frame_set_label_align (frame, g_value_get_float (value));
     break;
    case PROP_LABEL_WIDGET:
      bobgui_frame_set_label_widget (frame, g_value_get_object (value));
      break;
    case PROP_CHILD:
      bobgui_frame_set_child (frame, g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_frame_get_property (GObject         *object,
                        guint            prop_id,
                        GValue          *value,
                        GParamSpec      *pspec)
{
  BobguiFrame *frame = BOBGUI_FRAME (object);
  BobguiFramePrivate *priv = bobgui_frame_get_instance_private (frame);

  switch (prop_id)
    {
    case PROP_LABEL:
      g_value_set_string (value, bobgui_frame_get_label (frame));
      break;
    case PROP_LABEL_XALIGN:
      g_value_set_float (value, priv->label_xalign);
      break;
    case PROP_LABEL_WIDGET:
      g_value_set_object (value,
                          priv->label_widget ?
                          G_OBJECT (priv->label_widget) : NULL);
      break;
    case PROP_CHILD:
      g_value_set_object (value, bobgui_frame_get_child (frame));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

/**
 * bobgui_frame_new:
 * @label: (nullable): the text to use as the label of the frame
 *
 * Creates a new `BobguiFrame`, with optional label @label.
 *
 * If @label is %NULL, the label is omitted.
 *
 * Returns: a new `BobguiFrame` widget
 */
BobguiWidget*
bobgui_frame_new (const char *label)
{
  return g_object_new (BOBGUI_TYPE_FRAME, "label", label, NULL);
}

/**
 * bobgui_frame_set_label:
 * @frame: a `BobguiFrame`
 * @label: (nullable): the text to use as the label of the frame
 *
 * Creates a new `BobguiLabel` with the @label and sets it as the frame's
 * label widget.
 */
void
bobgui_frame_set_label (BobguiFrame *frame,
                     const char *label)
{
  g_return_if_fail (BOBGUI_IS_FRAME (frame));

  if (!label)
    bobgui_frame_set_label_widget (frame, NULL);
  else
    bobgui_frame_set_label_widget (frame, bobgui_label_new (label));
}

/**
 * bobgui_frame_get_label:
 * @frame: a `BobguiFrame`
 *
 * Returns the frame labels text.
 *
 * If the frame's label widget is not a `BobguiLabel`, %NULL
 * is returned.
 *
 * Returns: (nullable): the text in the label, or %NULL if there
 *    was no label widget or the label widget was not a `BobguiLabel`.
 *    This string is owned by BOBGUI and must not be modified or freed.
 */
const char *
bobgui_frame_get_label (BobguiFrame *frame)
{
  BobguiFramePrivate *priv = bobgui_frame_get_instance_private (frame);

  g_return_val_if_fail (BOBGUI_IS_FRAME (frame), NULL);

  if (BOBGUI_IS_LABEL (priv->label_widget))
    return bobgui_label_get_text (BOBGUI_LABEL (priv->label_widget));
  else
    return NULL;
}

static void
update_accessible_relation (BobguiFrame *frame)
{
  BobguiFramePrivate *priv = bobgui_frame_get_instance_private (frame);

  if (!priv->child)
    return;

  if (priv->label_widget)
    bobgui_accessible_update_relation (BOBGUI_ACCESSIBLE (priv->child),
                                    BOBGUI_ACCESSIBLE_RELATION_LABELLED_BY, priv->label_widget, NULL,
                                    -1);
  else
    bobgui_accessible_reset_relation (BOBGUI_ACCESSIBLE (priv->child),
                                   BOBGUI_ACCESSIBLE_RELATION_LABELLED_BY);
}

/**
 * bobgui_frame_set_label_widget:
 * @frame: a `BobguiFrame`
 * @label_widget: (nullable): the new label widget
 *
 * Sets the label widget for the frame.
 *
 * This is the widget that will appear embedded in the top edge
 * of the frame as a title.
 */
void
bobgui_frame_set_label_widget (BobguiFrame  *frame,
                            BobguiWidget *label_widget)
{
  BobguiFramePrivate *priv = bobgui_frame_get_instance_private (frame);

  g_return_if_fail (BOBGUI_IS_FRAME (frame));
  g_return_if_fail (label_widget == NULL || priv->label_widget == label_widget || bobgui_widget_get_parent (label_widget) == NULL);

  if (priv->label_widget == label_widget)
    return;

  if (priv->label_widget)
    bobgui_widget_unparent (priv->label_widget);

  priv->label_widget = label_widget;

  if (label_widget)
    {
      priv->label_widget = label_widget;
      bobgui_widget_set_parent (label_widget, BOBGUI_WIDGET (frame));
    }

  update_accessible_relation (frame);

  g_object_freeze_notify (G_OBJECT (frame));
  g_object_notify_by_pspec (G_OBJECT (frame), frame_props[PROP_LABEL_WIDGET]);
  g_object_notify_by_pspec (G_OBJECT (frame),  frame_props[PROP_LABEL]);
  g_object_thaw_notify (G_OBJECT (frame));
}

/**
 * bobgui_frame_get_label_widget:
 * @frame: a `BobguiFrame`
 *
 * Retrieves the label widget for the frame.
 *
 * Returns: (nullable) (transfer none): the label widget
 */
BobguiWidget *
bobgui_frame_get_label_widget (BobguiFrame *frame)
{
  BobguiFramePrivate *priv = bobgui_frame_get_instance_private (frame);

  g_return_val_if_fail (BOBGUI_IS_FRAME (frame), NULL);

  return priv->label_widget;
}

/**
 * bobgui_frame_set_label_align: (set-property label-xalign)
 * @frame: a `BobguiFrame`
 * @xalign: The position of the label along the top edge
 *   of the widget. A value of 0.0 represents left alignment;
 *   1.0 represents right alignment.
 *
 * Sets the X alignment of the frame widget’s label.
 *
 * The default value for a newly created frame is 0.0.
 */
void
bobgui_frame_set_label_align (BobguiFrame *frame,
                           float     xalign)
{
  BobguiFramePrivate *priv = bobgui_frame_get_instance_private (frame);

  g_return_if_fail (BOBGUI_IS_FRAME (frame));

  xalign = CLAMP (xalign, 0.0, 1.0);
  if (priv->label_xalign == xalign)
    return;

  priv->label_xalign = xalign;
  g_object_notify_by_pspec (G_OBJECT (frame), frame_props[PROP_LABEL_XALIGN]);
  bobgui_widget_queue_allocate (BOBGUI_WIDGET (frame));
}

/**
 * bobgui_frame_get_label_align: (get-property label-xalign)
 * @frame: a `BobguiFrame`
 *
 * Retrieves the X alignment of the frame’s label.
 *
 * Returns: the frames X alignment
 */
float
bobgui_frame_get_label_align (BobguiFrame *frame)
{
  BobguiFramePrivate *priv = bobgui_frame_get_instance_private (frame);

  g_return_val_if_fail (BOBGUI_IS_FRAME (frame), 0.0);

  return priv->label_xalign;
}

static void
bobgui_frame_size_allocate (BobguiWidget *widget,
                         int        width,
                         int        height,
                         int        baseline)
{
  BobguiFrame *frame = BOBGUI_FRAME (widget);
  BobguiFramePrivate *priv = bobgui_frame_get_instance_private (frame);
  BobguiAllocation new_allocation;

  BOBGUI_FRAME_GET_CLASS (frame)->compute_child_allocation (frame, &new_allocation);

  if (priv->label_widget &&
      bobgui_widget_get_visible (priv->label_widget))
    {
      BobguiAllocation label_allocation;
      int nat_width, label_width, label_height;
      float xalign;

      if (_bobgui_widget_get_direction (widget) == BOBGUI_TEXT_DIR_LTR)
        xalign = priv->label_xalign;
      else
        xalign = 1 - priv->label_xalign;

      bobgui_widget_measure (priv->label_widget, BOBGUI_ORIENTATION_HORIZONTAL, -1,
                          NULL, &nat_width, NULL, NULL);
      label_width = MIN (new_allocation.width, nat_width);
      bobgui_widget_measure (priv->label_widget, BOBGUI_ORIENTATION_VERTICAL, width,
                          &label_height, NULL, NULL, NULL);

      label_allocation.x = new_allocation.x + (new_allocation.width - label_width) * xalign;
      label_allocation.y = new_allocation.y - label_height;
      label_allocation.height = label_height;
      label_allocation.width = label_width;

      bobgui_widget_size_allocate (priv->label_widget, &label_allocation, -1);
    }

  if (priv->child && bobgui_widget_get_visible (priv->child))
    bobgui_widget_size_allocate (priv->child, &new_allocation, -1);
}

static void
bobgui_frame_real_compute_child_allocation (BobguiFrame      *frame,
                                         BobguiAllocation *child_allocation)
{
  BobguiFramePrivate *priv = bobgui_frame_get_instance_private (frame);
  int frame_width, frame_height;
  int height;

  frame_width = bobgui_widget_get_width (BOBGUI_WIDGET (frame));
  frame_height = bobgui_widget_get_height (BOBGUI_WIDGET (frame));

  if (priv->label_widget)
    {
      int nat_width, width;

      bobgui_widget_measure (priv->label_widget, BOBGUI_ORIENTATION_HORIZONTAL, -1,
                          NULL, &nat_width, NULL, NULL);
      width = MIN (frame_width, nat_width);
      bobgui_widget_measure (priv->label_widget, BOBGUI_ORIENTATION_VERTICAL, width,
                          &height, NULL, NULL, NULL);
    }
  else
    height = 0;

  child_allocation->x = 0;
  child_allocation->y = height;
  child_allocation->width = MAX (1, frame_width);
  child_allocation->height = MAX (1, frame_height - height);
}

static void
bobgui_frame_measure (BobguiWidget      *widget,
                   BobguiOrientation  orientation,
                   int             for_size,
                   int             *minimum,
                   int             *natural,
                   int             *minimum_baseline,
                   int             *natural_baseline)
{
  BobguiFrame *frame = BOBGUI_FRAME (widget);
  BobguiFramePrivate *priv = bobgui_frame_get_instance_private (frame);
  int child_min, child_nat;

  if (priv->child && bobgui_widget_get_visible (priv->child))
    {
      bobgui_widget_measure (priv->child,
                          orientation, for_size,
                          &child_min, &child_nat,
                          NULL, NULL);

      *minimum = child_min;
      *natural = child_nat;
    }
  else
    {
      *minimum = 0;
      *natural = 0;
    }

  if (priv->label_widget && bobgui_widget_get_visible (priv->label_widget))
    {
      if (orientation == BOBGUI_ORIENTATION_HORIZONTAL)
        {
          bobgui_widget_measure (priv->label_widget,
                              orientation, -1,
                              &child_min, &child_nat,
                              NULL, NULL);

          *minimum = MAX (child_min, *minimum);
          *natural = MAX (child_nat, *natural);
        }
      else
        {
          bobgui_widget_measure (priv->label_widget,
                              orientation, for_size,
                              &child_min, &child_nat,
                              NULL, NULL);

          *minimum += child_min;
          *natural += child_nat;
        }
    }
}

static void
bobgui_frame_compute_expand (BobguiWidget *widget,
                          gboolean  *hexpand,
                          gboolean  *vexpand)
{
  BobguiFrame *frame = BOBGUI_FRAME (widget);
  BobguiFramePrivate *priv = bobgui_frame_get_instance_private (frame);

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
bobgui_frame_get_request_mode (BobguiWidget *widget)
{
  BobguiFrame *frame = BOBGUI_FRAME (widget);
  BobguiFramePrivate *priv = bobgui_frame_get_instance_private (frame);

  if (priv->child)
    return bobgui_widget_get_request_mode (priv->child);
  else
    return BOBGUI_SIZE_REQUEST_CONSTANT_SIZE;
}

/**
 * bobgui_frame_set_child:
 * @frame: a `BobguiFrame`
 * @child: (nullable): the child widget
 *
 * Sets the child widget of @frame.
 */
void
bobgui_frame_set_child (BobguiFrame  *frame,
                     BobguiWidget *child)
{
  BobguiFramePrivate *priv = bobgui_frame_get_instance_private (frame);

  g_return_if_fail (BOBGUI_IS_FRAME (frame));
  g_return_if_fail (child == NULL || priv->child == child || bobgui_widget_get_parent (child) == NULL);

  if (priv->child == child)
    return;

  g_clear_pointer (&priv->child, bobgui_widget_unparent);

  if (child)
    {
      priv->child = child;
      bobgui_widget_set_parent (child, BOBGUI_WIDGET (frame));
    }

  update_accessible_relation (frame);

  g_object_notify_by_pspec (G_OBJECT (frame), frame_props[PROP_CHILD]);
}

/**
 * bobgui_frame_get_child:
 * @frame: a `BobguiFrame`
 *
 * Gets the child widget of @frame.
 *
 * Returns: (nullable) (transfer none): the child widget of @frame
 */
BobguiWidget *
bobgui_frame_get_child (BobguiFrame *frame)
{
  BobguiFramePrivate *priv = bobgui_frame_get_instance_private (frame);

  g_return_val_if_fail (BOBGUI_IS_FRAME (frame), NULL);

  return priv->child;
}
