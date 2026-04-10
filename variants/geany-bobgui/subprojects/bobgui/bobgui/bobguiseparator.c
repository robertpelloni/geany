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

#include "bobguiseparator.h"

#include "bobguiaccessible.h"
#include "bobguiorientable.h"
#include "bobguiprivate.h"
#include "bobguiwidgetprivate.h"

/**
 * BobguiSeparator:
 *
 * Draws a horizontal or vertical line to separate other widgets.
 *
 * <picture>
 *   <source srcset="separator-dark.png" media="(prefers-color-scheme: dark)">
 *   <img alt="An example BobguiSeparator" src="separator.png">
 * </picture>
 *
 * A `BobguiSeparator` can be used to group the widgets within a window.
 * It displays a line with a shadow to make it appear sunken into the
 * interface.
 *
 * # CSS nodes
 *
 * `BobguiSeparator` has a single CSS node with name separator. The node
 * gets one of the .horizontal or .vertical style classes.
 *
 * # Accessibility
 *
 * `BobguiSeparator` uses the [enum@Bobgui.AccessibleRole.separator] role.
 */

typedef struct _BobguiSeparatorClass BobguiSeparatorClass;

struct _BobguiSeparator
{
  BobguiWidget parent_instance;

  BobguiOrientation orientation;
};

struct _BobguiSeparatorClass
{
  BobguiWidgetClass parent_class;
};

enum {
  PROP_0,
  PROP_ORIENTATION
};


G_DEFINE_TYPE_WITH_CODE (BobguiSeparator, bobgui_separator, BOBGUI_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_ORIENTABLE, NULL))

static void
bobgui_separator_set_orientation (BobguiSeparator   *self,
                               BobguiOrientation  orientation)
{
  if (self->orientation != orientation)
    {
      self->orientation = orientation;

      bobgui_widget_update_orientation (BOBGUI_WIDGET (self), orientation);
      bobgui_widget_queue_resize (BOBGUI_WIDGET (self));

      bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (self),
                                      BOBGUI_ACCESSIBLE_PROPERTY_ORIENTATION, orientation,
                                      -1);

      g_object_notify (G_OBJECT (self), "orientation");
    }
}

static void
bobgui_separator_set_property (GObject      *object,
                            guint         prop_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  BobguiSeparator *separator = BOBGUI_SEPARATOR (object);

  switch (prop_id)
    {
    case PROP_ORIENTATION:
      bobgui_separator_set_orientation (separator, g_value_get_enum (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_separator_get_property (GObject    *object,
                            guint       prop_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  BobguiSeparator *separator = BOBGUI_SEPARATOR (object);

  switch (prop_id)
    {
    case PROP_ORIENTATION:
      g_value_set_enum (value, separator->orientation);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_separator_init (BobguiSeparator *separator)
{
  separator->orientation = BOBGUI_ORIENTATION_HORIZONTAL;

  bobgui_widget_update_orientation (BOBGUI_WIDGET (separator),
                                 separator->orientation);
}

static void
bobgui_separator_class_init (BobguiSeparatorClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (class);

  object_class->set_property = bobgui_separator_set_property;
  object_class->get_property = bobgui_separator_get_property;

  g_object_class_override_property (object_class, PROP_ORIENTATION, "orientation");

  bobgui_widget_class_set_css_name (widget_class, I_("separator"));
  bobgui_widget_class_set_accessible_role (widget_class, BOBGUI_ACCESSIBLE_ROLE_SEPARATOR);
}

/**
 * bobgui_separator_new:
 * @orientation: the separator’s orientation.
 *
 * Creates a new `BobguiSeparator` with the given orientation.
 *
 * Returns: a new `BobguiSeparator`.
 */
BobguiWidget *
bobgui_separator_new (BobguiOrientation orientation)
{
  return g_object_new (BOBGUI_TYPE_SEPARATOR,
                       "orientation", orientation,
                       NULL);
}
