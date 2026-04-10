/* BOBGUI - The Bobgui Framework
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 * Copyright (C) 2001 Red Hat, Inc.
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

#include "bobguiscrollbar.h"
#include "bobguirange.h"

#include "bobguiaccessiblerange.h"
#include "bobguiadjustment.h"
#include "bobguiorientable.h"
#include "bobguiprivate.h"
#include "bobguiwidgetprivate.h"
#include "bobguiboxlayout.h"


/**
 * BobguiScrollbar:
 *
 * Shows a horizontal or vertical scrollbar.
 *
 * <picture>
 *   <source srcset="scrollbar-dark.png" media="(prefers-color-scheme: dark)">
 *   <img alt="An example BobguiScrollbar" src="scrollbar.png">
 * </picture>
 *
 * Its position and movement are controlled by the adjustment that is passed to
 * or created by [ctor@Bobgui.Scrollbar.new]. See [class@Bobgui.Adjustment] for more
 * details. The [property@Bobgui.Adjustment:value] field sets the position of the
 * thumb and must be between [property@Bobgui.Adjustment:lower] and
 * [property@Bobgui.Adjustment:upper] - [property@Bobgui.Adjustment:page-size].
 * The [property@Bobgui.Adjustment:page-size] represents the size of the visible
 * scrollable area.
 *
 * The fields [property@Bobgui.Adjustment:step-increment] and
 * [property@Bobgui.Adjustment:page-increment] fields are added to or subtracted
 * from the [property@Bobgui.Adjustment:value] when the user asks to move by a step
 * (using e.g. the cursor arrow keys) or by a page (using e.g. the Page Down/Up
 * keys).
 *
 * # CSS nodes
 *
 * ```
 * scrollbar
 * ╰── range[.fine-tune]
 *     ╰── trough
 *         ╰── slider
 * ```
 *
 * `BobguiScrollbar` has a main CSS node with name scrollbar and a subnode for its
 * contents. The main node gets the .horizontal or .vertical style classes applied,
 * depending on the scrollbar's orientation.
 *
 * The range node gets the style class .fine-tune added when the scrollbar is
 * in 'fine-tuning' mode.
 *
 * Other style classes that may be added to scrollbars inside
 * [class@Bobgui.ScrolledWindow] include the positional classes (.left, .right,
 * .top, .bottom) and style classes related to overlay scrolling (.overlay-indicator,
 * .dragging, .hovering).
 *
 * # Accessibility
 *
 * `BobguiScrollbar` uses the [enum@Bobgui.AccessibleRole.scrollbar] role.
 */

typedef struct _BobguiScrollbarClass   BobguiScrollbarClass;

struct _BobguiScrollbar
{
  BobguiWidget parent_instance;
};

struct _BobguiScrollbarClass
{
  BobguiWidgetClass parent_class;
};

typedef struct {
  BobguiOrientation orientation;
  BobguiWidget *range;
} BobguiScrollbarPrivate;

enum {
  PROP_0,
  PROP_ADJUSTMENT,

  PROP_ORIENTATION,
  LAST_PROP = PROP_ORIENTATION
};

static void bobgui_scrollbar_accessible_range_init (BobguiAccessibleRangeInterface *iface);

G_DEFINE_TYPE_WITH_CODE (BobguiScrollbar, bobgui_scrollbar, BOBGUI_TYPE_WIDGET,
                         G_ADD_PRIVATE (BobguiScrollbar)
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_ORIENTABLE, NULL)
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_ACCESSIBLE_RANGE, 
                         bobgui_scrollbar_accessible_range_init))

static GParamSpec *props[LAST_PROP] = { NULL, };

static gboolean
accessible_range_set_current_value (BobguiAccessibleRange *range,
                                    double              value)
{
  BobguiScrollbar *self = BOBGUI_SCROLLBAR (range);
  BobguiAdjustment *adjustment = bobgui_scrollbar_get_adjustment (self);

  if (adjustment)
    {
      bobgui_adjustment_set_value (adjustment, value);
      return TRUE;
    }

  return FALSE;
}

static void
bobgui_scrollbar_accessible_range_init (BobguiAccessibleRangeInterface *iface)
{
  iface->set_current_value = accessible_range_set_current_value;
}

static void
bobgui_scrollbar_get_property (GObject    *object,
                            guint       property_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  BobguiScrollbar *self = BOBGUI_SCROLLBAR (object);
  BobguiScrollbarPrivate *priv = bobgui_scrollbar_get_instance_private (self);

  switch (property_id)
   {
    case PROP_ADJUSTMENT:
      g_value_set_object (value, bobgui_scrollbar_get_adjustment (self));
      break;
    case PROP_ORIENTATION:
      g_value_set_enum (value, priv->orientation);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
bobgui_scrollbar_set_property (GObject      *object,
                            guint         property_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  BobguiScrollbar *self = BOBGUI_SCROLLBAR (object);
  BobguiScrollbarPrivate *priv = bobgui_scrollbar_get_instance_private (self);

  switch (property_id)
    {
    case PROP_ADJUSTMENT:
      bobgui_scrollbar_set_adjustment (self, g_value_get_object (value));
      break;
    case PROP_ORIENTATION:
      {
        BobguiOrientation orientation = g_value_get_enum (value);

        if (orientation != priv->orientation)
          {
            BobguiLayoutManager *layout = bobgui_widget_get_layout_manager (BOBGUI_WIDGET (self));
            bobgui_orientable_set_orientation (BOBGUI_ORIENTABLE (layout), orientation);
            bobgui_orientable_set_orientation (BOBGUI_ORIENTABLE (priv->range), orientation);
            priv->orientation = orientation;
            bobgui_widget_update_orientation (BOBGUI_WIDGET (self), priv->orientation);
            bobgui_widget_queue_resize (BOBGUI_WIDGET (self));
            g_object_notify_by_pspec (object, pspec);
            bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (self),
                                            BOBGUI_ACCESSIBLE_PROPERTY_ORIENTATION, orientation,
                                            -1);
          }
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void bobgui_scrollbar_adjustment_changed       (BobguiAdjustment *adjustment,
                                                    gpointer       data);
static void bobgui_scrollbar_adjustment_value_changed (BobguiAdjustment *adjustment,
                                                    gpointer       data);

static void
bobgui_scrollbar_dispose (GObject *object)
{
  BobguiScrollbar *self = BOBGUI_SCROLLBAR (object);
  BobguiScrollbarPrivate *priv = bobgui_scrollbar_get_instance_private (self);
  BobguiAdjustment *adj;

  adj = bobgui_range_get_adjustment (BOBGUI_RANGE (priv->range));
  if (adj)
    {
      g_signal_handlers_disconnect_by_func (adj, bobgui_scrollbar_adjustment_changed, self);
      g_signal_handlers_disconnect_by_func (adj, bobgui_scrollbar_adjustment_value_changed, self);
    }

  g_clear_pointer (&priv->range, bobgui_widget_unparent);

  G_OBJECT_CLASS (bobgui_scrollbar_parent_class)->dispose (object);
}

static void
bobgui_scrollbar_class_init (BobguiScrollbarClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (class);

  object_class->get_property = bobgui_scrollbar_get_property;
  object_class->set_property = bobgui_scrollbar_set_property;
  object_class->dispose = bobgui_scrollbar_dispose;

  /**
   * BobguiScrollbar:adjustment:
   *
   * The `BobguiAdjustment` controlled by this scrollbar.
   */
  props[PROP_ADJUSTMENT] =
      g_param_spec_object ("adjustment", NULL, NULL,
                           BOBGUI_TYPE_ADJUSTMENT,
                           BOBGUI_PARAM_READWRITE|G_PARAM_CONSTRUCT|G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, LAST_PROP, props);

  g_object_class_override_property (object_class, PROP_ORIENTATION, "orientation");

  bobgui_widget_class_set_css_name (widget_class, I_("scrollbar"));
  bobgui_widget_class_set_layout_manager_type (widget_class, BOBGUI_TYPE_BOX_LAYOUT);
  bobgui_widget_class_set_accessible_role (widget_class, BOBGUI_ACCESSIBLE_ROLE_SCROLLBAR);
}

static void
bobgui_scrollbar_init (BobguiScrollbar *self)
{
  BobguiScrollbarPrivate *priv = bobgui_scrollbar_get_instance_private (self);

  priv->orientation = BOBGUI_ORIENTATION_HORIZONTAL;

  priv->range = g_object_new (BOBGUI_TYPE_RANGE, NULL);
  bobgui_range_set_flippable (BOBGUI_RANGE (priv->range), TRUE);
  bobgui_widget_set_hexpand (priv->range, TRUE);
  bobgui_widget_set_vexpand (priv->range, TRUE);
  bobgui_widget_set_parent (priv->range, BOBGUI_WIDGET (self));
  bobgui_widget_update_orientation (BOBGUI_WIDGET (self), priv->orientation);
  bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (self),
                                  BOBGUI_ACCESSIBLE_PROPERTY_ORIENTATION, priv->orientation,
                                  -1);
}

/**
 * bobgui_scrollbar_new:
 * @orientation: the scrollbar’s orientation.
 * @adjustment: (nullable): the [class@Bobgui.Adjustment] to use, or %NULL
 *   to create a new adjustment.
 *
 * Creates a new scrollbar with the given orientation.
 *
 * Returns:  the new `BobguiScrollbar`.
 */
BobguiWidget *
bobgui_scrollbar_new (BobguiOrientation  orientation,
                   BobguiAdjustment  *adjustment)
{
  g_return_val_if_fail (adjustment == NULL || BOBGUI_IS_ADJUSTMENT (adjustment),
                        NULL);

  return g_object_new (BOBGUI_TYPE_SCROLLBAR,
                       "orientation", orientation,
                       "adjustment",  adjustment,
                       NULL);
}

static void
bobgui_scrollbar_adjustment_changed (BobguiAdjustment *adjustment,
                                  gpointer       data)
{
  BobguiScrollbar *self = data;

  bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (self),
                                  BOBGUI_ACCESSIBLE_PROPERTY_VALUE_MAX, bobgui_adjustment_get_upper (adjustment) -
                                                                     bobgui_adjustment_get_page_size (adjustment),
                                  BOBGUI_ACCESSIBLE_PROPERTY_VALUE_MIN, bobgui_adjustment_get_lower (adjustment),
                                  -1);
}

static void
bobgui_scrollbar_adjustment_value_changed (BobguiAdjustment *adjustment,
                                        gpointer       data)
{
  BobguiScrollbar *self = data;

  bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (self),
                                  BOBGUI_ACCESSIBLE_PROPERTY_VALUE_NOW, bobgui_adjustment_get_value (adjustment),
                                  -1);
}

/**
 * bobgui_scrollbar_set_adjustment:
 * @self: a `BobguiScrollbar`
 * @adjustment: (nullable): the adjustment to set
 *
 * Makes the scrollbar use the given adjustment.
 */
void
bobgui_scrollbar_set_adjustment (BobguiScrollbar  *self,
                              BobguiAdjustment *adjustment)
{
  BobguiScrollbarPrivate *priv = bobgui_scrollbar_get_instance_private (self);
  BobguiAdjustment *adj;

  g_return_if_fail (BOBGUI_IS_SCROLLBAR (self));
  g_return_if_fail (adjustment == NULL || BOBGUI_IS_ADJUSTMENT (adjustment));

  adj = bobgui_range_get_adjustment (BOBGUI_RANGE (priv->range));
  if (adj == adjustment)
    return;

  if (adj)
    {
      g_signal_handlers_disconnect_by_func (adj, bobgui_scrollbar_adjustment_changed, self);
      g_signal_handlers_disconnect_by_func (adj, bobgui_scrollbar_adjustment_value_changed, self);
    }

  bobgui_range_set_adjustment (BOBGUI_RANGE (priv->range), adjustment);

  if (adjustment)
    {
      g_signal_connect (adjustment, "changed",
                        G_CALLBACK (bobgui_scrollbar_adjustment_changed), self);
      g_signal_connect (adjustment, "value-changed",
                        G_CALLBACK (bobgui_scrollbar_adjustment_value_changed), self);

      bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (self),
                                      BOBGUI_ACCESSIBLE_PROPERTY_VALUE_MAX, bobgui_adjustment_get_upper (adjustment) -
                                                                         bobgui_adjustment_get_page_size (adjustment),
                                      BOBGUI_ACCESSIBLE_PROPERTY_VALUE_MIN, bobgui_adjustment_get_lower (adjustment),
                                      BOBGUI_ACCESSIBLE_PROPERTY_VALUE_NOW, bobgui_adjustment_get_value (adjustment),
                                      -1);
    }

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_ADJUSTMENT]);
}

/**
 * bobgui_scrollbar_get_adjustment:
 * @self: a `BobguiScrollbar`
 *
 * Returns the scrollbar's adjustment.
 *
 * Returns: (transfer none): the scrollbar's adjustment
 */
BobguiAdjustment *
bobgui_scrollbar_get_adjustment (BobguiScrollbar  *self)
{
  BobguiScrollbarPrivate *priv = bobgui_scrollbar_get_instance_private (self);

  g_return_val_if_fail (BOBGUI_IS_SCROLLBAR (self), NULL);

  if (priv->range)
    return bobgui_range_get_adjustment (BOBGUI_RANGE (priv->range));

  return NULL;
}
