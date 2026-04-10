/*
 * Copyright (c) 2017 Timm Bäder <mail@baedert.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Author: Timm Bäder <mail@baedert.org>
 */

/**
 * BobguiCenterBox:
 *
 * Arranges three children in a row, keeping the middle child
 * centered as well as possible.
 *
 * <picture>
 *   <source srcset="centerbox-dark.png" media="(prefers-color-scheme: dark)">
 *   <img alt="An example BobguiCenterBox" src="centerbox.png">
 * </picture>
 *
 * To add children to `BobguiCenterBox`, use [method@Bobgui.CenterBox.set_start_widget],
 * [method@Bobgui.CenterBox.set_center_widget] and
 * [method@Bobgui.CenterBox.set_end_widget].
 *
 * The sizing and positioning of children can be influenced with the
 * align and expand properties of the children.
 *
 * # BobguiCenterBox as BobguiBuildable
 *
 * The `BobguiCenterBox` implementation of the `BobguiBuildable` interface
 * supports placing children in the 3 positions by specifying “start”, “center”
 * or “end” as the “type” attribute of a `<child>` element.
 *
 * # CSS nodes
 *
 * `BobguiCenterBox` uses a single CSS node with the name “box”,
 *
 * The first child of the `BobguiCenterBox` will be allocated depending on the
 * text direction, i.e. in left-to-right layouts it will be allocated on the
 * left and in right-to-left layouts on the right.
 *
 * In vertical orientation, the nodes of the children are arranged from top to
 * bottom.
 *
 * # Accessibility
 *
 * Until BOBGUI 4.10, `BobguiCenterBox` used the [enum@Bobgui.AccessibleRole.group] role.
 *
 * Starting from BOBGUI 4.12, `BobguiCenterBox` uses the [enum@Bobgui.AccessibleRole.generic]
 * role.
 */

#include "config.h"
#include "bobguicenterbox.h"
#include "bobguicenterlayout.h"
#include "bobguiwidgetprivate.h"
#include "bobguiorientable.h"
#include "bobguibuildable.h"
#include "bobguibuilderprivate.h"
#include "bobguisizerequest.h"
#include "bobguitypebuiltins.h"
#include "bobguiprivate.h"

struct _BobguiCenterBox
{
  BobguiWidget parent_instance;

  BobguiWidget *start_widget;
  BobguiWidget *center_widget;
  BobguiWidget *end_widget;
};

struct _BobguiCenterBoxClass
{
  BobguiWidgetClass parent_class;
};


enum {
  PROP_0,
  PROP_START_WIDGET,
  PROP_CENTER_WIDGET,
  PROP_END_WIDGET,
  PROP_BASELINE_POSITION,
  PROP_SHRINK_CENTER_LAST,

  /* orientable */
  PROP_ORIENTATION,
  LAST_PROP = PROP_ORIENTATION
};

static GParamSpec *props[LAST_PROP] = { NULL, };

static BobguiBuildableIface *parent_buildable_iface;

static void bobgui_center_box_buildable_init (BobguiBuildableIface *iface);

G_DEFINE_TYPE_WITH_CODE (BobguiCenterBox, bobgui_center_box, BOBGUI_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_ORIENTABLE, NULL)
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_BUILDABLE, bobgui_center_box_buildable_init))

static void
bobgui_center_box_buildable_add_child (BobguiBuildable  *buildable,
                                    BobguiBuilder    *builder,
                                    GObject       *child,
                                    const char    *type)
{
  if (g_strcmp0 (type, "start") == 0)
    {
      bobgui_buildable_child_deprecation_warning (buildable, builder, "start", "start-widget");
      bobgui_center_box_set_start_widget (BOBGUI_CENTER_BOX (buildable), BOBGUI_WIDGET (child));
    }
  else if (g_strcmp0 (type, "center") == 0)
    {
      bobgui_buildable_child_deprecation_warning (buildable, builder, "center", "center-widget");
      bobgui_center_box_set_center_widget (BOBGUI_CENTER_BOX (buildable), BOBGUI_WIDGET (child));
    }
  else if (g_strcmp0 (type, "end") == 0)
    {
      bobgui_buildable_child_deprecation_warning (buildable, builder, "end", "end-widget");
      bobgui_center_box_set_end_widget (BOBGUI_CENTER_BOX (buildable), BOBGUI_WIDGET (child));
    }
  else
    {
      parent_buildable_iface->add_child (buildable, builder, child, type);
    }
}

static void
bobgui_center_box_buildable_init (BobguiBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);

  iface->add_child = bobgui_center_box_buildable_add_child;
}

static void
bobgui_center_box_set_property (GObject      *object,
                             guint         prop_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  BobguiCenterBox *self = BOBGUI_CENTER_BOX (object);
  BobguiLayoutManager *layout = bobgui_widget_get_layout_manager (BOBGUI_WIDGET (object));

  switch (prop_id)
    {
    case PROP_BASELINE_POSITION:
      bobgui_center_box_set_baseline_position (self, g_value_get_enum (value));
      break;

    case PROP_ORIENTATION:
      {
        BobguiOrientation orientation = g_value_get_enum (value);
        BobguiOrientation current = bobgui_center_layout_get_orientation (BOBGUI_CENTER_LAYOUT (layout));
        if (current != orientation)
          {
            bobgui_center_layout_set_orientation (BOBGUI_CENTER_LAYOUT (layout), orientation);
            bobgui_widget_update_orientation (BOBGUI_WIDGET (self), orientation);
            bobgui_widget_queue_resize (BOBGUI_WIDGET (self));
            g_object_notify (object, "orientation");
          }
      }
      break;

    case PROP_START_WIDGET:
      bobgui_center_box_set_start_widget (self, BOBGUI_WIDGET (g_value_get_object (value)));
      break;

    case PROP_CENTER_WIDGET:
      bobgui_center_box_set_center_widget (self, BOBGUI_WIDGET (g_value_get_object (value)));
      break;

    case PROP_END_WIDGET:
      bobgui_center_box_set_end_widget (self, BOBGUI_WIDGET (g_value_get_object (value)));
      break;

    case PROP_SHRINK_CENTER_LAST:
      bobgui_center_box_set_shrink_center_last (self, g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_center_box_get_property (GObject    *object,
                             guint       prop_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  BobguiCenterBox *self = BOBGUI_CENTER_BOX (object);
  BobguiLayoutManager *layout = bobgui_widget_get_layout_manager (BOBGUI_WIDGET (object));

  switch (prop_id)
    {
    case PROP_BASELINE_POSITION:
      g_value_set_enum (value, bobgui_center_layout_get_baseline_position (BOBGUI_CENTER_LAYOUT (layout)));
      break;

    case PROP_ORIENTATION:
      g_value_set_enum (value, bobgui_center_layout_get_orientation (BOBGUI_CENTER_LAYOUT (layout)));
      break;

    case PROP_START_WIDGET:
      g_value_set_object (value, self->start_widget);
      break;

    case PROP_CENTER_WIDGET:
      g_value_set_object (value, self->center_widget);
      break;

    case PROP_END_WIDGET:
      g_value_set_object (value, self->end_widget);
      break;

    case PROP_SHRINK_CENTER_LAST:
      g_value_set_boolean (value, bobgui_center_layout_get_shrink_center_last (BOBGUI_CENTER_LAYOUT (layout)));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_center_box_dispose (GObject *object)
{
  BobguiCenterBox *self = BOBGUI_CENTER_BOX (object);

  g_clear_pointer (&self->start_widget, bobgui_widget_unparent);
  g_clear_pointer (&self->center_widget, bobgui_widget_unparent);
  g_clear_pointer (&self->end_widget, bobgui_widget_unparent);

  G_OBJECT_CLASS (bobgui_center_box_parent_class)->dispose (object);
}

static void
bobgui_center_box_class_init (BobguiCenterBoxClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);

  object_class->set_property = bobgui_center_box_set_property;
  object_class->get_property = bobgui_center_box_get_property;
  object_class->dispose = bobgui_center_box_dispose;

  g_object_class_override_property (object_class, PROP_ORIENTATION, "orientation");

  /**
   * BobguiCenterBox:baseline-position:
   *
   * The position of the baseline aligned widget if extra space is available.
   */
  props[PROP_BASELINE_POSITION] =
      g_param_spec_enum ("baseline-position", NULL, NULL,
                         BOBGUI_TYPE_BASELINE_POSITION,
                         BOBGUI_BASELINE_POSITION_CENTER,
                         BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiCenterBox:start-widget:
   *
   * The widget that is placed at the start position.
   *
   * In vertical orientation, the start position is at the top.
   * In horizontal orientation, the start position is at the leading
   * edge with respect to the text direction.
   *
   * Since: 4.10
   */
  props[PROP_START_WIDGET] =
      g_param_spec_object ("start-widget", NULL, NULL,
                           BOBGUI_TYPE_WIDGET,
                           BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiCenterBox:center-widget:
   *
   * The widget that is placed at the center position.
   *
   * Since: 4.10
   */
  props[PROP_CENTER_WIDGET] =
      g_param_spec_object ("center-widget", NULL, NULL,
                           BOBGUI_TYPE_WIDGET,
                           BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiCenterBox:end-widget:
   *
   * The widget that is placed at the end position.
   *
   * In vertical orientation, the end position is at the bottom.
   * In horizontal orientation, the end position is at the trailing
   * edge with respect to the text direction.
   *
   * Since: 4.10
   */
  props[PROP_END_WIDGET] =
      g_param_spec_object ("end-widget", NULL, NULL,
                           BOBGUI_TYPE_WIDGET,
                           BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiCenterBox:shrink-center-last:
   *
   * Whether to shrink the center widget after other children.
   *
   * By default, when there's no space to give all three children their
   * natural widths, the start and end widgets start shrinking and the
   * center child keeps natural width until they reach minimum width.
   *
   * If false, start and end widgets keep natural width and the
   * center widget starts shrinking instead.
   *
   * Since: 4.12
   */
  props[PROP_SHRINK_CENTER_LAST] =
      g_param_spec_boolean ("shrink-center-last", NULL, NULL,
                            TRUE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, LAST_PROP, props);

  bobgui_widget_class_set_layout_manager_type (widget_class, BOBGUI_TYPE_CENTER_LAYOUT);
  bobgui_widget_class_set_css_name (widget_class, I_("box"));
  bobgui_widget_class_set_accessible_role (widget_class, BOBGUI_ACCESSIBLE_ROLE_GENERIC);
}

static void
bobgui_center_box_init (BobguiCenterBox *self)
{
  self->start_widget = NULL;
  self->center_widget = NULL;
  self->end_widget = NULL;
}

/**
 * bobgui_center_box_new:
 *
 * Creates a new `BobguiCenterBox`.
 *
 * Returns: the new `BobguiCenterBox`
 */
BobguiWidget *
bobgui_center_box_new (void)
{
  return BOBGUI_WIDGET (g_object_new (BOBGUI_TYPE_CENTER_BOX, NULL));
}

/**
 * bobgui_center_box_set_start_widget:
 * @self: a center box
 * @child: (nullable): the new start widget
 *
 * Sets the start widget.
 *
 * To remove the existing start widget, pass `NULL`.
 */
void
bobgui_center_box_set_start_widget (BobguiCenterBox *self,
                                 BobguiWidget    *child)
{
  BobguiLayoutManager *layout_manager;

  g_return_if_fail (BOBGUI_IS_CENTER_BOX (self));
  g_return_if_fail (child == NULL || self->start_widget == child || bobgui_widget_get_parent (child) == NULL);

  if (self->start_widget == child)
    return;

  if (self->start_widget)
    bobgui_widget_unparent (self->start_widget);

  self->start_widget = child;
  if (child)
    bobgui_widget_insert_after (child, BOBGUI_WIDGET (self), NULL);

  layout_manager = bobgui_widget_get_layout_manager (BOBGUI_WIDGET (self));
  bobgui_center_layout_set_start_widget (BOBGUI_CENTER_LAYOUT (layout_manager), child);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_START_WIDGET]);
}

/**
 * bobgui_center_box_set_center_widget:
 * @self: a center box
 * @child: (nullable): the new center widget
 *
 * Sets the center widget.
 *
 * To remove the existing center widget, pass `NULL`.
 */
void
bobgui_center_box_set_center_widget (BobguiCenterBox *self,
                                  BobguiWidget    *child)
{
  BobguiLayoutManager *layout_manager;

  g_return_if_fail (BOBGUI_IS_CENTER_BOX (self));
  g_return_if_fail (child == NULL || self->center_widget == child || bobgui_widget_get_parent (child) == NULL);

  if (self->center_widget == child)
    return;

  if (self->center_widget)
    bobgui_widget_unparent (self->center_widget);

  self->center_widget = child;
  if (child)
    bobgui_widget_insert_after (child, BOBGUI_WIDGET (self), self->start_widget);

  layout_manager = bobgui_widget_get_layout_manager (BOBGUI_WIDGET (self));
  bobgui_center_layout_set_center_widget (BOBGUI_CENTER_LAYOUT (layout_manager), child);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_CENTER_WIDGET]);
}

/**
 * bobgui_center_box_set_end_widget:
 * @self: a center box
 * @child: (nullable): the new end widget
 *
 * Sets the end widget.
 *
 * To remove the existing end widget, pass `NULL`.
 */
void
bobgui_center_box_set_end_widget (BobguiCenterBox *self,
                               BobguiWidget    *child)
{
  BobguiLayoutManager *layout_manager;

  g_return_if_fail (BOBGUI_IS_CENTER_BOX (self));
  g_return_if_fail (child == NULL || self->end_widget == child || bobgui_widget_get_parent (child) == NULL);

  if (self->end_widget == child)
    return;

  if (self->end_widget)
    bobgui_widget_unparent (self->end_widget);

  self->end_widget = child;
  if (child)
    bobgui_widget_insert_before (child, BOBGUI_WIDGET (self), NULL);

  layout_manager = bobgui_widget_get_layout_manager (BOBGUI_WIDGET (self));
  bobgui_center_layout_set_end_widget (BOBGUI_CENTER_LAYOUT (layout_manager), child);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_END_WIDGET]);
}

/**
 * bobgui_center_box_get_start_widget:
 * @self: a center box
 *
 * Gets the start widget.
 *
 * Returns: (transfer none) (nullable): the start widget
 */
BobguiWidget *
bobgui_center_box_get_start_widget (BobguiCenterBox *self)
{
  return self->start_widget;
}

/**
 * bobgui_center_box_get_center_widget:
 * @self: a center box
 *
 * Gets the center widget.
 *
 * Returns: (transfer none) (nullable): the center widget
 */
BobguiWidget *
bobgui_center_box_get_center_widget (BobguiCenterBox *self)
{
  return self->center_widget;
}

/**
 * bobgui_center_box_get_end_widget:
 * @self: a center box
 *
 * Gets the end widget.
 *
 * Returns: (transfer none) (nullable): the end widget
 */
BobguiWidget *
bobgui_center_box_get_end_widget (BobguiCenterBox *self)
{
  return self->end_widget;
}

/**
 * bobgui_center_box_set_baseline_position:
 * @self: a center box
 * @position: the baseline position
 *
 * Sets the baseline position of a center box.
 *
 * This affects only horizontal boxes with at least one baseline
 * aligned child. If there is more vertical space available than
 * requested, and the baseline is not allocated by the parent then
 * @position is used to allocate the baseline with respect to the
 * extra space available.
 */
void
bobgui_center_box_set_baseline_position (BobguiCenterBox        *self,
                                      BobguiBaselinePosition  position)
{
  BobguiBaselinePosition current_position;
  BobguiLayoutManager *layout;

  g_return_if_fail (BOBGUI_IS_CENTER_BOX (self));

  layout = bobgui_widget_get_layout_manager (BOBGUI_WIDGET (self));
  current_position = bobgui_center_layout_get_baseline_position (BOBGUI_CENTER_LAYOUT (layout));
  if (current_position != position)
    {
      bobgui_center_layout_set_baseline_position (BOBGUI_CENTER_LAYOUT (layout), position);
      g_object_notify_by_pspec (G_OBJECT (self), props[PROP_BASELINE_POSITION]);
      bobgui_widget_queue_resize (BOBGUI_WIDGET (self));
    }
}

/**
 * bobgui_center_box_get_baseline_position:
 * @self: a center box
 *
 * Gets the baseline position of the center box.
 *
 * See [method@Bobgui.CenterBox.set_baseline_position].
 *
 * Returns: the baseline position
 */
BobguiBaselinePosition
bobgui_center_box_get_baseline_position (BobguiCenterBox *self)
{
  BobguiLayoutManager *layout;

  g_return_val_if_fail (BOBGUI_IS_CENTER_BOX (self), BOBGUI_BASELINE_POSITION_CENTER);

  layout = bobgui_widget_get_layout_manager (BOBGUI_WIDGET (self));

  return bobgui_center_layout_get_baseline_position (BOBGUI_CENTER_LAYOUT (layout));
}

/**
 * bobgui_center_box_set_shrink_center_last:
 * @self: a cener box
 * @shrink_center_last: whether to shrink the center widget after others
 *
 * Sets whether to shrink the center widget after other children.
 *
 * By default, when there's no space to give all three children their
 * natural widths, the start and end widgets start shrinking and the
 * center child keeps natural width until they reach minimum width.
 *
 * If @shrink_center_last is false, start and end widgets keep natural
 * width and the center widget starts shrinking instead.
 *
 * Since: 4.12
 */
void
bobgui_center_box_set_shrink_center_last (BobguiCenterBox *self,
                                       gboolean      shrink_center_last)
{
  BobguiLayoutManager *layout;
  gboolean current_shrink_center_last;

  g_return_if_fail (BOBGUI_IS_CENTER_BOX (self));

  shrink_center_last = !!shrink_center_last;

  layout = bobgui_widget_get_layout_manager (BOBGUI_WIDGET (self));
  current_shrink_center_last = bobgui_center_layout_get_shrink_center_last (BOBGUI_CENTER_LAYOUT (layout));
  if (current_shrink_center_last != shrink_center_last)
    {
      bobgui_center_layout_set_shrink_center_last (BOBGUI_CENTER_LAYOUT (layout), shrink_center_last);
      g_object_notify_by_pspec (G_OBJECT (self), props[PROP_SHRINK_CENTER_LAST]);
      bobgui_widget_queue_allocate (BOBGUI_WIDGET (self));
    }
}

/**
 * bobgui_center_box_get_shrink_center_last:
 * @self: a center box
 *
 * Gets whether the center widget shrinks after other children.
 *
 * Returns: whether to shrink the center widget after others
 *
 * Since: 4.12
 */
gboolean
bobgui_center_box_get_shrink_center_last (BobguiCenterBox *self)
{
  BobguiLayoutManager *layout;

  g_return_val_if_fail (BOBGUI_IS_CENTER_BOX (self), FALSE);

  layout = bobgui_widget_get_layout_manager (BOBGUI_WIDGET (self));

  return bobgui_center_layout_get_shrink_center_last (BOBGUI_CENTER_LAYOUT (layout));
}
