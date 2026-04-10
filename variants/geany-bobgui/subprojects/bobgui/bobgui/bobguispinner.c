/* BOBGUI - The Bobgui Framework
 *
 * Copyright (C) 2007 John Stowers, Neil Jagdish Patel.
 * Copyright (C) 2009 Bastien Nocera, David Zeuthen
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Code adapted from egg-spinner
 * by Christian Hergert <christian.hergert@gmail.com>
 */

/*
 * Modified by the BOBGUI Team and others 2007.  See the AUTHORS
 * file for a list of people on the BOBGUI Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BOBGUI at ftp://ftp.bobgui.org/pub/bobgui/.
 */

#include "config.h"

#include "bobguispinner.h"

#include "bobguiimage.h"
#include "bobguiprivate.h"
#include "bobguicssnodeprivate.h"
#include "bobguiwidgetprivate.h"
#include "bobguicssnumbervalueprivate.h"
#include "bobguirendericonprivate.h"


/**
 * BobguiSpinner:
 *
 * Displays an icon-size spinning animation.
 *
 * It is often used as an alternative to a [class@Bobgui.ProgressBar]
 * for displaying indefinite activity, instead of actual progress.
 *
 * <picture>
 *   <source srcset="spinner-dark.png" media="(prefers-color-scheme: dark)">
 *   <img alt="An example BobguiSpinner" src="spinner.png">
 * </picture>
 *
 * To start the animation, use [method@Bobgui.Spinner.start], to stop it
 * use [method@Bobgui.Spinner.stop].
 *
 * # CSS nodes
 *
 * `BobguiSpinner` has a single CSS node with the name spinner.
 * When the animation is active, the :checked pseudoclass is
 * added to this node.
 *
 * # Accessibility
 *
 * `BobguiSpinner` uses the [enum@Bobgui.AccessibleRole.progress_bar] role.
 */

typedef struct _BobguiSpinnerClass BobguiSpinnerClass;

struct _BobguiSpinner
{
  BobguiWidget parent;

  guint spinning : 1;
};

struct _BobguiSpinnerClass
{
  BobguiWidgetClass parent_class;
};


enum {
  PROP_0,
  PROP_SPINNING
};

G_DEFINE_TYPE (BobguiSpinner, bobgui_spinner, BOBGUI_TYPE_WIDGET)

#define DEFAULT_SIZE 16

static void
update_state_flags (BobguiSpinner *spinner)
{
  if (spinner->spinning && bobgui_widget_get_mapped (BOBGUI_WIDGET (spinner)))
    bobgui_widget_set_state_flags (BOBGUI_WIDGET (spinner),
                                BOBGUI_STATE_FLAG_CHECKED, FALSE);
  else
    bobgui_widget_unset_state_flags (BOBGUI_WIDGET (spinner),
                                  BOBGUI_STATE_FLAG_CHECKED);
}

static void
bobgui_spinner_measure (BobguiWidget      *widget,
                     BobguiOrientation  orientation,
                     int             for_size,
                     int            *minimum,
                     int            *natural,
                     int            *minimum_baseline,
                     int            *natural_baseline)
{
  BobguiCssStyle *style;

  style = bobgui_css_node_get_style (bobgui_widget_get_css_node (widget));
  *minimum = *natural = bobgui_css_number_value_get (style->icon->icon_size, 100);
}

static void
bobgui_spinner_snapshot (BobguiWidget   *widget,
                      BobguiSnapshot *snapshot)
{
  BobguiCssStyle *style = bobgui_css_node_get_style (bobgui_widget_get_css_node (widget));

  bobgui_css_style_snapshot_icon (style,
                               snapshot,
                               bobgui_widget_get_width (widget),
                               bobgui_widget_get_height (widget));
}

static void
bobgui_spinner_map (BobguiWidget *widget)
{
  BobguiSpinner *spinner = BOBGUI_SPINNER (widget);

  BOBGUI_WIDGET_CLASS (bobgui_spinner_parent_class)->map (widget);

  update_state_flags (spinner);
}

static void
bobgui_spinner_unmap (BobguiWidget *widget)
{
  BobguiSpinner *spinner = BOBGUI_SPINNER (widget);

  BOBGUI_WIDGET_CLASS (bobgui_spinner_parent_class)->unmap (widget);

  update_state_flags (spinner);
}

static void
bobgui_spinner_css_changed (BobguiWidget         *widget,
                         BobguiCssStyleChange *change)
{ 
  BOBGUI_WIDGET_CLASS (bobgui_spinner_parent_class)->css_changed (widget, change);
  
  if (change == NULL ||
      bobgui_css_style_change_affects (change, BOBGUI_CSS_AFFECTS_ICON_SIZE))
    {
      bobgui_widget_queue_resize (widget);
    }
  else if (bobgui_css_style_change_affects (change, BOBGUI_CSS_AFFECTS_ICON_TEXTURE |
                                                 BOBGUI_CSS_AFFECTS_ICON_REDRAW))
    {
      bobgui_widget_queue_draw (widget);
    }
}

/**
 * bobgui_spinner_get_spinning:
 * @spinner: a `BobguiSpinner`
 *
 * Returns whether the spinner is spinning.
 *
 * Returns: %TRUE if the spinner is active
 */
gboolean
bobgui_spinner_get_spinning (BobguiSpinner *spinner)
{
  g_return_val_if_fail (BOBGUI_IS_SPINNER (spinner), FALSE);

  return spinner->spinning;
}

/**
 * bobgui_spinner_set_spinning:
 * @spinner: a `BobguiSpinner`
 * @spinning: whether the spinner should be spinning
 *
 * Sets the activity of the spinner.
 */
void
bobgui_spinner_set_spinning (BobguiSpinner *spinner,
                          gboolean    spinning)
{
  g_return_if_fail (BOBGUI_IS_SPINNER (spinner));

  spinning = !!spinning;

  if (spinning == spinner->spinning)
    return;

  spinner->spinning = spinning;

  update_state_flags (spinner);

  g_object_notify (G_OBJECT (spinner), "spinning");
}

static void
bobgui_spinner_get_property (GObject    *object,
                          guint       param_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  switch (param_id)
    {
      case PROP_SPINNING:
        g_value_set_boolean (value, bobgui_spinner_get_spinning (BOBGUI_SPINNER (object)));
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
    }
}

static void
bobgui_spinner_set_property (GObject      *object,
                          guint         param_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  switch (param_id)
    {
      case PROP_SPINNING:
        bobgui_spinner_set_spinning (BOBGUI_SPINNER (object), g_value_get_boolean (value));
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
    }
}

static void
bobgui_spinner_class_init (BobguiSpinnerClass *klass)
{
  GObjectClass *gobject_class;
  BobguiWidgetClass *widget_class;

  gobject_class = G_OBJECT_CLASS(klass);
  gobject_class->get_property = bobgui_spinner_get_property;
  gobject_class->set_property = bobgui_spinner_set_property;

  widget_class = BOBGUI_WIDGET_CLASS(klass);
  widget_class->snapshot = bobgui_spinner_snapshot;
  widget_class->measure = bobgui_spinner_measure;
  widget_class->map = bobgui_spinner_map;
  widget_class->unmap = bobgui_spinner_unmap;
  widget_class->css_changed = bobgui_spinner_css_changed;

  /**
   * BobguiSpinner:spinning:
   *
   * Whether the spinner is spinning
   */
  g_object_class_install_property (gobject_class,
                                   PROP_SPINNING,
                                   g_param_spec_boolean ("spinning", NULL, NULL,
                                                         FALSE,
                                                         BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  bobgui_widget_class_set_css_name (widget_class, I_("spinner"));
  bobgui_widget_class_set_accessible_role (widget_class, BOBGUI_ACCESSIBLE_ROLE_PROGRESS_BAR);
}

static void
bobgui_spinner_init (BobguiSpinner *spinner)
{
}

/**
 * bobgui_spinner_new:
 *
 * Returns a new spinner widget. Not yet started.
 *
 * Returns: a new `BobguiSpinner`
 */
BobguiWidget *
bobgui_spinner_new (void)
{
  return g_object_new (BOBGUI_TYPE_SPINNER, NULL);
}

/**
 * bobgui_spinner_start:
 * @spinner: a `BobguiSpinner`
 *
 * Starts the animation of the spinner.
 */
void
bobgui_spinner_start (BobguiSpinner *spinner)
{
  g_return_if_fail (BOBGUI_IS_SPINNER (spinner));

  bobgui_spinner_set_spinning (spinner, TRUE);

  bobgui_accessible_update_state (BOBGUI_ACCESSIBLE (spinner),
                               BOBGUI_ACCESSIBLE_STATE_BUSY, TRUE,
                               -1);
}

/**
 * bobgui_spinner_stop:
 * @spinner: a `BobguiSpinner`
 *
 * Stops the animation of the spinner.
 */
void
bobgui_spinner_stop (BobguiSpinner *spinner)
{
  g_return_if_fail (BOBGUI_IS_SPINNER (spinner));

  bobgui_spinner_set_spinning (spinner, FALSE);

  bobgui_accessible_update_state (BOBGUI_ACCESSIBLE (spinner),
                               BOBGUI_ACCESSIBLE_STATE_BUSY, FALSE,
                               -1);
}
