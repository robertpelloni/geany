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

/**
 * BobguiBox:
 *
 * Arranges child widgets into a single row or column.
 *
 * <picture>
 *   <source srcset="box-dark.png" media="(prefers-color-scheme: dark)">
 *   <img alt="An example BobguiBox" src="box.png">
 * </picture>
 *
 * Whether it is a row or column depends on the value of its
 * [property@Bobgui.Orientable:orientation] property. Within the other
 * dimension, all children are allocated the same size. The
 * [property@Bobgui.Widget:halign] and [property@Bobgui.Widget:valign]
 * properties can be used on the children to influence their allocation.
 *
 * Use repeated calls to [method@Bobgui.Box.append] to pack widgets into a
 * `BobguiBox` from start to end. Use [method@Bobgui.Box.remove] to remove widgets
 * from the `BobguiBox`. [method@Bobgui.Box.insert_child_after] can be used to add
 * a child at a particular position.
 *
 * Use [method@Bobgui.Box.set_homogeneous] to specify whether or not all children
 * of the `BobguiBox` are forced to get the same amount of space.
 *
 * Use [method@Bobgui.Box.set_spacing] to determine how much space will be minimally
 * placed between all children in the `BobguiBox`. Note that spacing is added
 * *between* the children.
 *
 * Use [method@Bobgui.Box.reorder_child_after] to move a child to a different
 * place in the box.
 *
 * # CSS nodes
 *
 * `BobguiBox` uses a single CSS node with name box.
 *
 * # Accessibility
 *
 * Until BOBGUI 4.10, `BobguiBox` used the [enum@Bobgui.AccessibleRole.group] role.
 *
 * Starting from BOBGUI 4.12, `BobguiBox` uses the [enum@Bobgui.AccessibleRole.generic] role.
 */

#include "config.h"

#include "bobguibox.h"
#include "bobguiboxlayout.h"
#include "bobguibuildable.h"
#include "bobguiorientable.h"
#include "bobguiprivate.h"
#include "bobguitypebuiltins.h"
#include "bobguiwidgetprivate.h"


enum {
  PROP_0,
  PROP_SPACING,
  PROP_HOMOGENEOUS,
  PROP_BASELINE_CHILD,
  PROP_BASELINE_POSITION,

  /* orientable */
  PROP_ORIENTATION,
  LAST_PROP = PROP_ORIENTATION
};

typedef struct
{
  gint16          spacing;

  guint           homogeneous    : 1;
  guint           baseline_pos   : 2;
} BobguiBoxPrivate;

static GParamSpec *props[LAST_PROP] = { NULL, };

static void bobgui_box_buildable_iface_init (BobguiBuildableIface *iface);

G_DEFINE_TYPE_WITH_CODE (BobguiBox, bobgui_box, BOBGUI_TYPE_WIDGET,
                         G_ADD_PRIVATE (BobguiBox)
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_ORIENTABLE, NULL)
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_BUILDABLE,
                                                bobgui_box_buildable_iface_init))


static void
bobgui_box_set_property (GObject      *object,
                      guint         prop_id,
                      const GValue *value,
                      GParamSpec   *pspec)
{
  BobguiBox *box = BOBGUI_BOX (object);
  BobguiLayoutManager *box_layout = bobgui_widget_get_layout_manager (BOBGUI_WIDGET (box));

  switch (prop_id)
    {
    case PROP_ORIENTATION:
      {
        BobguiOrientation orientation = g_value_get_enum (value);
        if (bobgui_orientable_get_orientation (BOBGUI_ORIENTABLE (box_layout)) != orientation)
          {
            bobgui_orientable_set_orientation (BOBGUI_ORIENTABLE (box_layout), orientation);
            bobgui_widget_update_orientation (BOBGUI_WIDGET (box), orientation);
            g_object_notify_by_pspec (G_OBJECT (box), pspec);
          }
      }
      break;
    case PROP_SPACING:
      bobgui_box_set_spacing (box, g_value_get_int (value));
      break;
    case PROP_BASELINE_CHILD:
      bobgui_box_set_baseline_child (box, g_value_get_int (value));
      break;
    case PROP_BASELINE_POSITION:
      bobgui_box_set_baseline_position (box, g_value_get_enum (value));
      break;
    case PROP_HOMOGENEOUS:
      bobgui_box_set_homogeneous (box, g_value_get_boolean (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_box_get_property (GObject    *object,
                      guint       prop_id,
                      GValue     *value,
                      GParamSpec *pspec)
{
  BobguiBox *box = BOBGUI_BOX (object);
  BobguiBoxLayout *box_layout = BOBGUI_BOX_LAYOUT (bobgui_widget_get_layout_manager (BOBGUI_WIDGET (box)));

  switch (prop_id)
    {
    case PROP_ORIENTATION:
      g_value_set_enum (value, bobgui_orientable_get_orientation (BOBGUI_ORIENTABLE (box_layout)));
      break;
    case PROP_SPACING:
      g_value_set_int (value, bobgui_box_layout_get_spacing (box_layout));
      break;
    case PROP_BASELINE_CHILD:
      g_value_set_int (value, bobgui_box_layout_get_baseline_child (box_layout));
      break;
    case PROP_BASELINE_POSITION:
      g_value_set_enum (value, bobgui_box_layout_get_baseline_position (box_layout));
      break;
    case PROP_HOMOGENEOUS:
      g_value_set_boolean (value, bobgui_box_layout_get_homogeneous (box_layout));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_box_compute_expand (BobguiWidget *widget,
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
bobgui_box_get_request_mode (BobguiWidget *widget)
{
  BobguiWidget *w;
  int wfh = 0, hfw = 0;

  for (w = bobgui_widget_get_first_child (widget);
       w != NULL;
       w = bobgui_widget_get_next_sibling (w))
    {
      BobguiSizeRequestMode mode = bobgui_widget_get_request_mode (w);

      switch (mode)
        {
        case BOBGUI_SIZE_REQUEST_HEIGHT_FOR_WIDTH:
          hfw ++;
          break;
        case BOBGUI_SIZE_REQUEST_WIDTH_FOR_HEIGHT:
          wfh ++;
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
bobgui_box_dispose (GObject *object)
{
  BobguiWidget *child;

  while ((child = bobgui_widget_get_first_child (BOBGUI_WIDGET (object))))
    bobgui_widget_unparent (child);

  G_OBJECT_CLASS (bobgui_box_parent_class)->dispose (object);
}

static void
bobgui_box_class_init (BobguiBoxClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (class);

  object_class->set_property = bobgui_box_set_property;
  object_class->get_property = bobgui_box_get_property;
  object_class->dispose = bobgui_box_dispose;

  widget_class->focus = bobgui_widget_focus_child;
  widget_class->compute_expand = bobgui_box_compute_expand;
  widget_class->get_request_mode = bobgui_box_get_request_mode;

  g_object_class_override_property (object_class,
                                    PROP_ORIENTATION,
                                    "orientation");

  /**
   * BobguiBox:spacing:
   *
   * The amount of space between children.
   */
  props[PROP_SPACING] =
    g_param_spec_int ("spacing", NULL, NULL,
                      0, G_MAXINT, 0,
                      BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiBox:homogeneous:
   *
   * Whether the children should all be the same size.
   */
  props[PROP_HOMOGENEOUS] =
    g_param_spec_boolean ("homogeneous", NULL, NULL,
                          FALSE,
                          BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiBox:baseline-child:
   *
   * The position of the child that determines the baseline.
   *
   * This is only relevant if the box is in vertical orientation.
   *
   * Since: 4.12
   */
  props[PROP_BASELINE_CHILD] =
    g_param_spec_int ("baseline-child", NULL, NULL,
                      -1, G_MAXINT, -1,
                      BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiBox:baseline-position:
   *
   * How to position baseline-aligned widgets if extra space is available.
   */
  props[PROP_BASELINE_POSITION] =
    g_param_spec_enum ("baseline-position", NULL, NULL,
                       BOBGUI_TYPE_BASELINE_POSITION,
                       BOBGUI_BASELINE_POSITION_CENTER,
                       BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, LAST_PROP, props);

  bobgui_widget_class_set_layout_manager_type (widget_class, BOBGUI_TYPE_BOX_LAYOUT);
  bobgui_widget_class_set_css_name (widget_class, I_("box"));
  bobgui_widget_class_set_accessible_role (widget_class, BOBGUI_ACCESSIBLE_ROLE_GENERIC);
}

static void
bobgui_box_init (BobguiBox *box)
{
  bobgui_widget_update_orientation (BOBGUI_WIDGET (box), BOBGUI_ORIENTATION_HORIZONTAL);
}

static BobguiBuildableIface *parent_buildable_iface;

static void
bobgui_box_buildable_add_child (BobguiBuildable *buildable,
                             BobguiBuilder   *builder,
                             GObject      *child,
                             const char   *type)
{
  if (BOBGUI_IS_WIDGET (child))
    bobgui_box_append (BOBGUI_BOX (buildable), BOBGUI_WIDGET (child));
  else
    parent_buildable_iface->add_child (buildable, builder, child, type);
}

static void
bobgui_box_buildable_iface_init (BobguiBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);

  iface->add_child = bobgui_box_buildable_add_child;
}

/**
 * bobgui_box_new:
 * @orientation: the box’s orientation
 * @spacing: the number of pixels to place between children
 *
 * Creates a new box.
 *
 * Returns: a new `BobguiBox`.
 */
BobguiWidget*
bobgui_box_new (BobguiOrientation orientation,
             int            spacing)
{
  return g_object_new (BOBGUI_TYPE_BOX,
                       "orientation", orientation,
                       "spacing", spacing,
                       NULL);
}

/**
 * bobgui_box_set_homogeneous:
 * @box: a box
 * @homogeneous: true to create equal allotments,
 *   false for variable allotments
 *
 * Sets whether or not all children are given equal space
 * in the box.
 */
void
bobgui_box_set_homogeneous (BobguiBox  *box,
                         gboolean homogeneous)
{
  BobguiBoxLayout *box_layout;

  g_return_if_fail (BOBGUI_IS_BOX (box));

  homogeneous = !!homogeneous;

  box_layout = BOBGUI_BOX_LAYOUT (bobgui_widget_get_layout_manager (BOBGUI_WIDGET (box)));
  if (homogeneous == bobgui_box_layout_get_homogeneous (box_layout))
    return;

  bobgui_box_layout_set_homogeneous (box_layout, homogeneous);
  g_object_notify_by_pspec (G_OBJECT (box), props[PROP_HOMOGENEOUS]);
}

/**
 * bobgui_box_get_homogeneous:
 * @box: a box
 *
 * Returns whether the box is homogeneous.
 *
 * In a homogeneous box all children are the same size.
 *
 * Returns: true if the box is homogeneous
 */
gboolean
bobgui_box_get_homogeneous (BobguiBox *box)
{
  BobguiLayoutManager *box_layout;

  g_return_val_if_fail (BOBGUI_IS_BOX (box), FALSE);

  box_layout = bobgui_widget_get_layout_manager (BOBGUI_WIDGET (box));

  return bobgui_box_layout_get_homogeneous (BOBGUI_BOX_LAYOUT (box_layout));
}

/**
 * bobgui_box_set_spacing:
 * @box: a box
 * @spacing: the number of pixels to put between children
 *
 * Sets the number of pixels to place between children.
 */
void
bobgui_box_set_spacing (BobguiBox *box,
                     int     spacing)
{
  BobguiBoxLayout *box_layout;

  g_return_if_fail (BOBGUI_IS_BOX (box));

  box_layout = BOBGUI_BOX_LAYOUT (bobgui_widget_get_layout_manager (BOBGUI_WIDGET (box)));
  if (spacing == bobgui_box_layout_get_spacing (box_layout))
    return;

  bobgui_box_layout_set_spacing (box_layout, spacing);
  g_object_notify_by_pspec (G_OBJECT (box), props[PROP_SPACING]);
}

/**
 * bobgui_box_get_spacing:
 * @box: a box
 *
 * Gets the value set by [method@Bobgui.Box.set_spacing].
 *
 * Returns: spacing between children
 */
int
bobgui_box_get_spacing (BobguiBox *box)
{
  BobguiLayoutManager *box_layout;

  g_return_val_if_fail (BOBGUI_IS_BOX (box), 0);

  box_layout = bobgui_widget_get_layout_manager (BOBGUI_WIDGET (box));

  return bobgui_box_layout_get_spacing (BOBGUI_BOX_LAYOUT (box_layout));
}

/**
 * bobgui_box_set_baseline_child:
 * @box: a box
 * @child: a child position, or -1
 *
 * Sets the baseline child of a box.
 *
 * This affects only vertical boxes.
 *
 * Since: 4.12
 */
void
bobgui_box_set_baseline_child (BobguiBox *box,
                            int     child)
{
  BobguiBoxLayout *box_layout;

  g_return_if_fail (BOBGUI_IS_BOX (box));
  g_return_if_fail (child >= -1);

  box_layout = BOBGUI_BOX_LAYOUT (bobgui_widget_get_layout_manager (BOBGUI_WIDGET (box)));
  if (child == bobgui_box_layout_get_baseline_child (box_layout))
    return;

  bobgui_box_layout_set_baseline_child  (box_layout, child);
  g_object_notify_by_pspec (G_OBJECT (box), props[PROP_BASELINE_CHILD]);
  bobgui_widget_queue_resize (BOBGUI_WIDGET (box));
}

/**
 * bobgui_box_get_baseline_child:
 * @box: a box
 *
 * Gets the value set by [method@Bobgui.Box.set_baseline_child].
 *
 * Returns: the baseline child
 *
 * Since: 4.12
 */
int
bobgui_box_get_baseline_child (BobguiBox *box)
{
  BobguiLayoutManager *box_layout;

  g_return_val_if_fail (BOBGUI_IS_BOX (box), -1);

  box_layout = bobgui_widget_get_layout_manager (BOBGUI_WIDGET (box));

  return bobgui_box_layout_get_baseline_child (BOBGUI_BOX_LAYOUT (box_layout));
}

/**
 * bobgui_box_set_baseline_position:
 * @box: a box
 * @position: the baseline position
 *
 * Sets the baseline position of a box.
 *
 * This affects only horizontal boxes with at least one baseline
 * aligned child. If there is more vertical space available than
 * requested, and the baseline is not allocated by the parent then
 * @position is used to allocate the baseline with respect to the
 * extra space available.
 */
void
bobgui_box_set_baseline_position (BobguiBox             *box,
                               BobguiBaselinePosition position)
{
  BobguiBoxLayout *box_layout;

  g_return_if_fail (BOBGUI_IS_BOX (box));

  box_layout = BOBGUI_BOX_LAYOUT (bobgui_widget_get_layout_manager (BOBGUI_WIDGET (box)));
  if (position == bobgui_box_layout_get_baseline_position (box_layout))
    return;

  bobgui_box_layout_set_baseline_position (box_layout, position);
  g_object_notify_by_pspec (G_OBJECT (box), props[PROP_BASELINE_POSITION]);
}

/**
 * bobgui_box_get_baseline_position:
 * @box: a box
 *
 * Gets the value set by [method@Bobgui.Box.set_baseline_position].
 *
 * Returns: the baseline position
 */
BobguiBaselinePosition
bobgui_box_get_baseline_position (BobguiBox *box)
{
  BobguiLayoutManager *box_layout;

  g_return_val_if_fail (BOBGUI_IS_BOX (box), BOBGUI_BASELINE_POSITION_CENTER);

  box_layout = bobgui_widget_get_layout_manager (BOBGUI_WIDGET (box));

  return bobgui_box_layout_get_baseline_position (BOBGUI_BOX_LAYOUT (box_layout));
}

/**
 * bobgui_box_insert_child_after:
 * @box: a box
 * @child: the widget to insert
 * @sibling: (nullable): the sibling after which to insert @child
 *
 * Inserts a child at a specific position.
 *
 * The child is added after @sibling in the list of @box children.
 *
 * If @sibling is `NULL`, the @child is placed at the beginning.
 */
void
bobgui_box_insert_child_after (BobguiBox    *box,
                            BobguiWidget *child,
                            BobguiWidget *sibling)
{
  BobguiWidget *widget;

  g_return_if_fail (BOBGUI_IS_BOX (box));
  g_return_if_fail (BOBGUI_IS_WIDGET (child));
  g_return_if_fail (bobgui_widget_get_parent (child) == NULL);

  widget = BOBGUI_WIDGET (box);

  if (sibling)
    {
      g_return_if_fail (BOBGUI_IS_WIDGET (sibling));
      g_return_if_fail (bobgui_widget_get_parent (sibling) == widget);
    }

  if (child == sibling)
    return;

  bobgui_widget_insert_after (child, widget, sibling);
}

/**
 * bobgui_box_reorder_child_after:
 * @box: a box
 * @child: the widget to move, must be a child of @box
 * @sibling: (nullable): the sibling to move @child after
 *
 * Moves a child to a different position.
 *
 * The child is moved to the position after @sibling in the list
 * of @box children.
 *
 * If @sibling is `NULL`, the child is placed at the beginning.
 */
void
bobgui_box_reorder_child_after (BobguiBox    *box,
                             BobguiWidget *child,
                             BobguiWidget *sibling)
{
  BobguiWidget *widget;

  g_return_if_fail (BOBGUI_IS_BOX (box));
  g_return_if_fail (BOBGUI_IS_WIDGET (child));
  g_return_if_fail (bobgui_widget_get_parent (child) == (BobguiWidget *)box);

 widget = BOBGUI_WIDGET (box);

  if (sibling)
    {
      g_return_if_fail (BOBGUI_IS_WIDGET (sibling));
      g_return_if_fail (bobgui_widget_get_parent (sibling) == widget);
    }

  if (child == sibling)
    return;

  bobgui_widget_insert_after (child, widget, sibling);
}

/**
 * bobgui_box_append:
 * @box: a box
 * @child: the widget to append
 *
 * Adds a child at the end.
 */
void
bobgui_box_append (BobguiBox    *box,
                BobguiWidget *child)
{
  g_return_if_fail (BOBGUI_IS_BOX (box));
  g_return_if_fail (BOBGUI_IS_WIDGET (child));
  g_return_if_fail (bobgui_widget_get_parent (child) == NULL);

  bobgui_widget_insert_before (child, BOBGUI_WIDGET (box), NULL);
}

/**
 * bobgui_box_prepend:
 * @box: a box
 * @child: the widget to prepend
 *
 * Adds a child at the beginning.
 */
void
bobgui_box_prepend (BobguiBox    *box,
                 BobguiWidget *child)
{
  g_return_if_fail (BOBGUI_IS_BOX (box));
  g_return_if_fail (BOBGUI_IS_WIDGET (child));
  g_return_if_fail (bobgui_widget_get_parent (child) == NULL);

  bobgui_widget_insert_after (child, BOBGUI_WIDGET (box), NULL);
}

/**
 * bobgui_box_remove:
 * @box: a box
 * @child: the child to remove
 *
 * Removes a child widget from the box.
 *
 * The child must have been added before with
 * [method@Bobgui.Box.append], [method@Bobgui.Box.prepend], or
 * [method@Bobgui.Box.insert_child_after].
 */
void
bobgui_box_remove (BobguiBox    *box,
                BobguiWidget *child)
{
  g_return_if_fail (BOBGUI_IS_BOX (box));
  g_return_if_fail (BOBGUI_IS_WIDGET (child));
  g_return_if_fail (bobgui_widget_get_parent (child) == (BobguiWidget *)box);

  bobgui_widget_unparent (child);
}
