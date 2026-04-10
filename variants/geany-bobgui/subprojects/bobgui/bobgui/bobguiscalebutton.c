/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2005 Ronald S. Bultje
 * Copyright (C) 2006, 2007 Christian Persch
 * Copyright (C) 2006 Jan Arne Petersen
 * Copyright (C) 2005-2007 Red Hat, Inc.
 * Copyright (C) 2014 Red Hat, Inc.
 *
 * Authors:
 * - Ronald S. Bultje <rbultje@ronald.bitfreak.net>
 * - Bastien Nocera <bnocera@redhat.com>
 * - Jan Arne Petersen <jpetersen@jpetersen.org>
 * - Christian Persch <chpe@svn.gnome.org>
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
 * Modified by the BOBGUI Team and others 2007.  See the AUTHORS
 * file for a list of people on the BOBGUI Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BOBGUI at ftp://ftp.bobgui.org/pub/bobgui/.
 */

#include "config.h"

#include "bobguiscalebutton.h"

#include "bobguiaccessiblerange.h"
#include "bobguiadjustment.h"
#include "bobguibox.h"
#include "bobguibuttonprivate.h"
#include "bobguitogglebutton.h"
#include "bobguieventcontrollerscroll.h"
#include "bobguiframe.h"
#include "bobguigesture.h"
#include "bobguimain.h"
#include "bobguimarshalers.h"
#include "bobguiorientable.h"
#include "bobguipopover.h"
#include "bobguiprivate.h"
#include "bobguirangeprivate.h"
#include "bobguiscale.h"
#include "bobguitypebuiltins.h"
#include "bobguiwidgetprivate.h"
#include "bobguiwindowprivate.h"
#include "bobguinative.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

/**
 * BobguiScaleButton:
 *
 * Provides a button which pops up a scale widget.
 *
 * This kind of widget is commonly used for volume controls in multimedia
 * applications, and BOBGUI provides a [class@Bobgui.VolumeButton] subclass that
 * is tailored for this use case.
 *
 * # Shortcuts and Gestures
 *
 * The following signals have default keybindings:
 *
 * - [signal@Bobgui.ScaleButton::popup]
 *
 * # CSS nodes
 *
 * ```
 * scalebutton.scale
 * ╰── button.toggle
 *     ╰── <icon>
 * ```
 *
 * `BobguiScaleButton` has a single CSS node with name scalebutton and `.scale`
 * style class, and contains a `button` node with a `.toggle` style class.
 */


#define SCALE_SIZE 100

enum
{
  VALUE_CHANGED,
  POPUP,
  POPDOWN,

  LAST_SIGNAL
};

enum
{
  PROP_0,

  PROP_ORIENTATION,
  PROP_VALUE,
  PROP_SIZE,
  PROP_ADJUSTMENT,
  PROP_ICONS,
  PROP_ACTIVE,
  PROP_HAS_FRAME
};

typedef struct
{
  BobguiWidget *button;

  BobguiWidget *plus_button;
  BobguiWidget *minus_button;
  BobguiWidget *dock;
  BobguiWidget *box;
  BobguiWidget *scale;
  BobguiWidget *active_button;

  BobguiOrientation orientation;
  BobguiOrientation applied_orientation;

  guint autoscroll_timeout;
  BobguiScrollType autoscroll_step;
  gboolean autoscrolling;

  char **icon_list;

  BobguiAdjustment *adjustment; /* needed because it must be settable in init() */
} BobguiScaleButtonPrivate;

static void     bobgui_scale_button_constructed    (GObject             *object);
static void	bobgui_scale_button_dispose	(GObject             *object);
static void     bobgui_scale_button_finalize       (GObject             *object);
static void	bobgui_scale_button_set_property	(GObject             *object,
						 guint                prop_id,
						 const GValue        *value,
						 GParamSpec          *pspec);
static void	bobgui_scale_button_get_property	(GObject             *object,
						 guint                prop_id,
						 GValue              *value,
						 GParamSpec          *pspec);
static void     bobgui_scale_button_size_allocate  (BobguiWidget           *widget,
                                                 int                  width,
                                                 int                  height,
                                                 int                  baseline);
static void     bobgui_scale_button_measure        (BobguiWidget           *widget,
                                                 BobguiOrientation       orientation,
                                                 int                  for_size,
                                                 int                 *minimum,
                                                 int                 *natural,
                                                 int                 *minimum_baseline,
                                                 int                 *natural_baseline);
static void bobgui_scale_button_set_orientation_private (BobguiScaleButton *button,
                                                      BobguiOrientation  orientation);
static void     bobgui_scale_button_popup          (BobguiWidget           *widget);
static void     bobgui_scale_button_popdown        (BobguiWidget           *widget);
static void     cb_button_clicked               (BobguiWidget           *button,
                                                 gpointer             user_data);
static void     bobgui_scale_button_update_icon    (BobguiScaleButton      *button);
static void     cb_scale_value_changed          (BobguiRange            *range,
                                                 gpointer             user_data);
static void     cb_popup_mapped                 (BobguiWidget           *popup,
                                                 gpointer             user_data);

static gboolean bobgui_scale_button_scroll_controller_scroll (BobguiEventControllerScroll *scroll,
                                                           double                    dx,
                                                           double                    dy,
                                                           BobguiScaleButton           *button);

static void     bobgui_scale_button_accessible_range_init    (BobguiAccessibleRangeInterface *iface);


G_DEFINE_TYPE_WITH_CODE (BobguiScaleButton, bobgui_scale_button, BOBGUI_TYPE_WIDGET,
                         G_ADD_PRIVATE (BobguiScaleButton)
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_ACCESSIBLE_RANGE,
                                                bobgui_scale_button_accessible_range_init)
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_ORIENTABLE, NULL))

static guint signals[LAST_SIGNAL] = { 0, };

static gboolean
accessible_range_set_current_value (BobguiAccessibleRange *accessible_range,
                                    double              value)
{
  bobgui_scale_button_set_value (BOBGUI_SCALE_BUTTON (accessible_range), value);

  return TRUE;
}

static void
bobgui_scale_button_accessible_range_init (BobguiAccessibleRangeInterface *iface)
{
  iface->set_current_value = accessible_range_set_current_value;
}

static void
bobgui_scale_button_class_init (BobguiScaleButtonClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);

  gobject_class->constructed = bobgui_scale_button_constructed;
  gobject_class->finalize = bobgui_scale_button_finalize;
  gobject_class->dispose = bobgui_scale_button_dispose;
  gobject_class->set_property = bobgui_scale_button_set_property;
  gobject_class->get_property = bobgui_scale_button_get_property;

  widget_class->measure = bobgui_scale_button_measure;
  widget_class->size_allocate = bobgui_scale_button_size_allocate;
  widget_class->focus = bobgui_widget_focus_child;
  widget_class->grab_focus = bobgui_widget_grab_focus_child;


  g_object_class_override_property (gobject_class, PROP_ORIENTATION, "orientation");

  /**
   * BobguiScaleButton:value:
   *
   * The value of the scale.
   */
  g_object_class_install_property (gobject_class,
				   PROP_VALUE,
				   g_param_spec_double ("value", NULL, NULL,
							-G_MAXDOUBLE,
							G_MAXDOUBLE,
							0,
							BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * BobguiScaleButton:adjustment:
   *
   * The `BobguiAdjustment` that is used as the model.
   */
  g_object_class_install_property (gobject_class,
                                   PROP_ADJUSTMENT,
                                   g_param_spec_object ("adjustment", NULL, NULL,
                                                        BOBGUI_TYPE_ADJUSTMENT,
                                                        BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * BobguiScaleButton:icons:
   *
   * The names of the icons to be used by the scale button.
   *
   * The first item in the array will be used in the button
   * when the current value is the lowest value, the second
   * item for the highest value. All the subsequent icons will
   * be used for all the other values, spread evenly over the
   * range of values.
   *
   * If there's only one icon name in the @icons array, it will
   * be used for all the values. If only two icon names are in
   * the @icons array, the first one will be used for the bottom
   * 50% of the scale, and the second one for the top 50%.
   *
   * It is recommended to use at least 3 icons so that the
   * `BobguiScaleButton` reflects the current value of the scale
   * better for the users.
   */
  g_object_class_install_property (gobject_class,
                                   PROP_ICONS,
                                   g_param_spec_boxed ("icons", NULL, NULL,
                                                       G_TYPE_STRV,
                                                       BOBGUI_PARAM_READWRITE));

  /**
   * BobguiScaleButton:active:
   *
   * If the scale button should be pressed in.
   *
   * Since: 4.10
   */
  g_object_class_install_property (gobject_class,
                                   PROP_ACTIVE,
                                   g_param_spec_boolean ("active", NULL, NULL,
                                                         FALSE,
                                                         BOBGUI_PARAM_READABLE));

  /**
   * BobguiScaleButton:has-frame:
   *
   * If the scale button has a frame.
   *
   * Since: 4.14
   */
  g_object_class_install_property (gobject_class,
                                   PROP_HAS_FRAME,
                                   g_param_spec_boolean ("has-frame", NULL, NULL,
                                                         FALSE,
                                                         BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * BobguiScaleButton::value-changed:
   * @button: the object which received the signal
   * @value: the new value
   *
   * Emitted when the value field has changed.
   */
  signals[VALUE_CHANGED] =
    g_signal_new (I_("value-changed"),
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BobguiScaleButtonClass, value_changed),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 1, G_TYPE_DOUBLE);

  /**
   * BobguiScaleButton::popup:
   * @button: the object which received the signal
   *
   * Emitted to popup the scale widget.
   *
   * This is a [keybinding signal](class.SignalAction.html).
   *
   * The default bindings for this signal are <kbd>Space</kbd>,
   * <kbd>Enter</kbd> and <kbd>Return</kbd>.
   */
  signals[POPUP] =
    g_signal_new_class_handler (I_("popup"),
                                G_OBJECT_CLASS_TYPE (klass),
                                G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                                G_CALLBACK (bobgui_scale_button_popup),
                                NULL, NULL,
                                NULL,
                                G_TYPE_NONE, 0);

  /**
   * BobguiScaleButton::popdown:
   * @button: the object which received the signal
   *
   * Emitted to dismiss the popup.
   *
   * This is a [keybinding signal](class.SignalAction.html).
   *
   * The default binding for this signal is <kbd>Escape</kbd>.
   */
  signals[POPDOWN] =
    g_signal_new_class_handler (I_("popdown"),
                                G_OBJECT_CLASS_TYPE (klass),
                                G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                                G_CALLBACK (bobgui_scale_button_popdown),
                                NULL, NULL,
                                NULL,
                                G_TYPE_NONE, 0);

  /* Key bindings */
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_space, 0,
				       "popup",
                                       NULL);
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_KP_Space, 0,
				       "popup",
                                       NULL);
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_Return, 0,
				       "popup",
                                       NULL);
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_ISO_Enter, 0,
				       "popup",
                                       NULL);
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_KP_Enter, 0,
				       "popup",
                                       NULL);
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_Escape, 0,
				       "popdown",
                                       NULL);

  /* Bind class to template
   */
  bobgui_widget_class_set_template_from_resource (widget_class,
					       "/org/bobgui/libbobgui/ui/bobguiscalebutton.ui");

  bobgui_widget_class_bind_template_child_private (widget_class, BobguiScaleButton, button);
  bobgui_widget_class_bind_template_child_internal_private (widget_class, BobguiScaleButton, plus_button);
  bobgui_widget_class_bind_template_child_internal_private (widget_class, BobguiScaleButton, minus_button);
  bobgui_widget_class_bind_template_child_private (widget_class, BobguiScaleButton, dock);
  bobgui_widget_class_bind_template_child_private (widget_class, BobguiScaleButton, box);
  bobgui_widget_class_bind_template_child_private (widget_class, BobguiScaleButton, scale);

  bobgui_widget_class_bind_template_callback (widget_class, cb_button_clicked);
  bobgui_widget_class_bind_template_callback (widget_class, cb_scale_value_changed);
  bobgui_widget_class_bind_template_callback (widget_class, cb_popup_mapped);

  bobgui_widget_class_set_css_name (widget_class, I_("scalebutton"));
  bobgui_widget_class_set_accessible_role (widget_class, BOBGUI_ACCESSIBLE_ROLE_GROUP);
}

static gboolean
start_autoscroll (gpointer data)
{
  BobguiScaleButton *button = data;
  BobguiScaleButtonPrivate *priv = bobgui_scale_button_get_instance_private (button);

  bobgui_range_start_autoscroll (BOBGUI_RANGE (priv->scale), priv->autoscroll_step);

  priv->autoscrolling = TRUE;
  priv->autoscroll_timeout = 0;

  return G_SOURCE_REMOVE;
}

static void
button_pressed_cb (BobguiGesture     *gesture,
                   int             n_press,
                   double          x,
                   double          y,
                   BobguiScaleButton *button)
{
  BobguiScaleButtonPrivate *priv = bobgui_scale_button_get_instance_private (button);
  BobguiWidget *widget;

  widget = bobgui_event_controller_get_widget (BOBGUI_EVENT_CONTROLLER (gesture));
  if (widget == priv->plus_button)
    priv->autoscroll_step = BOBGUI_SCROLL_PAGE_FORWARD;
  else
    priv->autoscroll_step = BOBGUI_SCROLL_PAGE_BACKWARD;
  priv->autoscroll_timeout = g_timeout_add (200, start_autoscroll, button);
}

static void
bobgui_scale_button_toggled (BobguiScaleButton *button)
{
  BobguiScaleButtonPrivate *priv = bobgui_scale_button_get_instance_private (button);
  gboolean active;

  g_object_notify (G_OBJECT (button), "active");

  active = bobgui_toggle_button_get_active (BOBGUI_TOGGLE_BUTTON (priv->button));

  if (active)
    bobgui_popover_popup (BOBGUI_POPOVER (priv->dock));
  else
    bobgui_popover_popdown (BOBGUI_POPOVER (priv->dock));
}

static void
bobgui_scale_button_closed (BobguiScaleButton *button)
{
  BobguiScaleButtonPrivate *priv = bobgui_scale_button_get_instance_private (button);

  bobgui_toggle_button_set_active (BOBGUI_TOGGLE_BUTTON (priv->button), FALSE);
}

static void
bobgui_scale_button_init (BobguiScaleButton *button)
{
  BobguiScaleButtonPrivate *priv = bobgui_scale_button_get_instance_private (button);
  BobguiEventController *controller;

  priv->orientation = BOBGUI_ORIENTATION_VERTICAL;
  priv->applied_orientation = BOBGUI_ORIENTATION_VERTICAL;

  bobgui_widget_init_template (BOBGUI_WIDGET (button));
  bobgui_widget_set_parent (priv->dock, BOBGUI_WIDGET (button));

  /* Need a local reference to the adjustment */
  priv->adjustment = bobgui_adjustment_new (0, 0, 100, 2, 20, 0);
  g_object_ref_sink (priv->adjustment);
  bobgui_range_set_adjustment (BOBGUI_RANGE (priv->scale), priv->adjustment);

  bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (button),
                                  BOBGUI_ACCESSIBLE_PROPERTY_VALUE_MAX, bobgui_adjustment_get_upper (priv->adjustment) -
                                                                     bobgui_adjustment_get_page_size (priv->adjustment),
                                  BOBGUI_ACCESSIBLE_PROPERTY_VALUE_MIN, bobgui_adjustment_get_lower (priv->adjustment),
                                  BOBGUI_ACCESSIBLE_PROPERTY_VALUE_NOW, bobgui_adjustment_get_value (priv->adjustment),
                                  -1);

  bobgui_widget_add_css_class (BOBGUI_WIDGET (button), "scale");

  controller = bobgui_event_controller_scroll_new (BOBGUI_EVENT_CONTROLLER_SCROLL_VERTICAL);
  g_signal_connect (controller, "scroll",
                    G_CALLBACK (bobgui_scale_button_scroll_controller_scroll),
                    button);
  bobgui_widget_add_controller (BOBGUI_WIDGET (button), controller);

  g_signal_connect_swapped (priv->dock, "closed",
                            G_CALLBACK (bobgui_scale_button_closed), button);
  g_signal_connect_swapped (priv->button, "toggled",
                            G_CALLBACK (bobgui_scale_button_toggled), button);

  g_signal_connect (bobgui_button_get_gesture (BOBGUI_BUTTON (priv->plus_button)),
                    "pressed", G_CALLBACK (button_pressed_cb), button);
  g_signal_connect (bobgui_button_get_gesture (BOBGUI_BUTTON (priv->minus_button)),
                    "pressed", G_CALLBACK (button_pressed_cb), button);
}

static void
bobgui_scale_button_constructed (GObject *object)
{
  BobguiScaleButton *button = BOBGUI_SCALE_BUTTON (object);

  G_OBJECT_CLASS (bobgui_scale_button_parent_class)->constructed (object);

  /* set button text and size */
  bobgui_scale_button_update_icon (button);
}

static void
bobgui_scale_button_set_property (GObject       *object,
			       guint          prop_id,
			       const GValue  *value,
			       GParamSpec    *pspec)
{
  BobguiScaleButton *button = BOBGUI_SCALE_BUTTON (object);

  switch (prop_id)
    {
    case PROP_ORIENTATION:
      bobgui_scale_button_set_orientation_private (button, g_value_get_enum (value));
      break;
    case PROP_VALUE:
      bobgui_scale_button_set_value (button, g_value_get_double (value));
      break;
    case PROP_ADJUSTMENT:
      bobgui_scale_button_set_adjustment (button, g_value_get_object (value));
      break;
    case PROP_ICONS:
      bobgui_scale_button_set_icons (button,
                                  (const char **)g_value_get_boxed (value));
      break;
    case PROP_HAS_FRAME:
      bobgui_scale_button_set_has_frame (button, g_value_get_boolean (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_scale_button_get_property (GObject     *object,
			       guint        prop_id,
			       GValue      *value,
			       GParamSpec  *pspec)
{
  BobguiScaleButton *button = BOBGUI_SCALE_BUTTON (object);
  BobguiScaleButtonPrivate *priv = bobgui_scale_button_get_instance_private (button);

  switch (prop_id)
    {
    case PROP_ORIENTATION:
      g_value_set_enum (value, priv->orientation);
      break;
    case PROP_VALUE:
      g_value_set_double (value, bobgui_scale_button_get_value (button));
      break;
    case PROP_ADJUSTMENT:
      g_value_set_object (value, bobgui_scale_button_get_adjustment (button));
      break;
    case PROP_ICONS:
      g_value_set_boxed (value, priv->icon_list);
      break;
    case PROP_ACTIVE:
      g_value_set_boolean (value, bobgui_scale_button_get_active (button));
      break;
    case PROP_HAS_FRAME:
      g_value_set_boolean (value, bobgui_scale_button_get_has_frame (button));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_scale_button_finalize (GObject *object)
{
  BobguiScaleButton *button = BOBGUI_SCALE_BUTTON (object);
  BobguiScaleButtonPrivate *priv = bobgui_scale_button_get_instance_private (button);

  g_clear_pointer (&priv->icon_list, g_strfreev);
  g_clear_object (&priv->adjustment);
  g_clear_handle_id (&priv->autoscroll_timeout, g_source_remove);

  G_OBJECT_CLASS (bobgui_scale_button_parent_class)->finalize (object);
}

static void
bobgui_scale_button_dispose (GObject *object)
{
  BobguiScaleButton *button = BOBGUI_SCALE_BUTTON (object);
  BobguiScaleButtonPrivate *priv = bobgui_scale_button_get_instance_private (button);

  g_clear_pointer (&priv->dock, bobgui_widget_unparent);
  g_clear_pointer (&priv->button, bobgui_widget_unparent);

  G_OBJECT_CLASS (bobgui_scale_button_parent_class)->dispose (object);
}

/**
 * bobgui_scale_button_new:
 * @min: the minimum value of the scale (usually 0)
 * @max: the maximum value of the scale (usually 100)
 * @step: the stepping of value when a scroll-wheel event,
 *   or up/down arrow event occurs (usually 2)
 * @icons: (nullable) (array zero-terminated=1): a %NULL-terminated
 *   array of icon names, or %NULL if you want to set the list
 *   later with bobgui_scale_button_set_icons()
 *
 * Creates a `BobguiScaleButton`.
 *
 * The new scale button has a range between @min and @max,
 * with a stepping of @step.
 *
 * Returns: a new `BobguiScaleButton`
 */
BobguiWidget *
bobgui_scale_button_new (double        min,
		      double        max,
		      double        step,
		      const char **icons)
{
  BobguiScaleButton *button;
  BobguiAdjustment *adjustment;

  adjustment = bobgui_adjustment_new (min, min, max, step, 10 * step, 0);

  button = g_object_new (BOBGUI_TYPE_SCALE_BUTTON,
                         "adjustment", adjustment,
                         "icons", icons,
                         NULL);

  return BOBGUI_WIDGET (button);
}

/**
 * bobgui_scale_button_get_value:
 * @button: a `BobguiScaleButton`
 *
 * Gets the current value of the scale button.
 *
 * Returns: current value of the scale button
 */
double
bobgui_scale_button_get_value (BobguiScaleButton * button)
{
  BobguiScaleButtonPrivate *priv = bobgui_scale_button_get_instance_private (button);

  g_return_val_if_fail (BOBGUI_IS_SCALE_BUTTON (button), 0);

  return bobgui_adjustment_get_value (priv->adjustment);
}

/**
 * bobgui_scale_button_set_value:
 * @button: a `BobguiScaleButton`
 * @value: new value of the scale button
 *
 * Sets the current value of the scale.
 *
 * If the value is outside the minimum or maximum range values,
 * it will be clamped to fit inside them.
 *
 * The scale button emits the [signal@Bobgui.ScaleButton::value-changed]
 * signal if the value changes.
 */
void
bobgui_scale_button_set_value (BobguiScaleButton *button,
			    double          value)
{
  BobguiScaleButtonPrivate *priv = bobgui_scale_button_get_instance_private (button);

  g_return_if_fail (BOBGUI_IS_SCALE_BUTTON (button));

  bobgui_range_set_value (BOBGUI_RANGE (priv->scale), value);
  g_object_notify (G_OBJECT (button), "value");
}

/**
 * bobgui_scale_button_set_icons:
 * @button: a `BobguiScaleButton`
 * @icons: (array zero-terminated=1): a %NULL-terminated array of icon names
 *
 * Sets the icons to be used by the scale button.
 */
void
bobgui_scale_button_set_icons (BobguiScaleButton  *button,
			    const char     **icons)
{
  BobguiScaleButtonPrivate *priv = bobgui_scale_button_get_instance_private (button);
  char **tmp;

  g_return_if_fail (BOBGUI_IS_SCALE_BUTTON (button));

  tmp = priv->icon_list;
  priv->icon_list = g_strdupv ((char **) icons);
  g_strfreev (tmp);
  bobgui_scale_button_update_icon (button);

  g_object_notify (G_OBJECT (button), "icons");
}

/**
 * bobgui_scale_button_get_adjustment:
 * @button: a `BobguiScaleButton`
 *
 * Gets the `BobguiAdjustment` associated with the `BobguiScaleButton`’s scale.
 *
 * See [method@Bobgui.Range.get_adjustment] for details.
 *
 * Returns: (transfer none): the adjustment associated with the scale
 */
BobguiAdjustment*
bobgui_scale_button_get_adjustment	(BobguiScaleButton *button)
{
  BobguiScaleButtonPrivate *priv = bobgui_scale_button_get_instance_private (button);

  g_return_val_if_fail (BOBGUI_IS_SCALE_BUTTON (button), NULL);

  return priv->adjustment;
}

/**
 * bobgui_scale_button_set_adjustment:
 * @button: a `BobguiScaleButton`
 * @adjustment: a `BobguiAdjustment`
 *
 * Sets the `BobguiAdjustment` to be used as a model
 * for the `BobguiScaleButton`’s scale.
 *
 * See [method@Bobgui.Range.set_adjustment] for details.
 */
void
bobgui_scale_button_set_adjustment	(BobguiScaleButton *button,
				 BobguiAdjustment  *adjustment)
{
  BobguiScaleButtonPrivate *priv = bobgui_scale_button_get_instance_private (button);

  g_return_if_fail (BOBGUI_IS_SCALE_BUTTON (button));

  if (!adjustment)
    adjustment = bobgui_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
  else
    g_return_if_fail (BOBGUI_IS_ADJUSTMENT (adjustment));

  if (priv->adjustment != adjustment)
    {
      if (priv->adjustment)
        g_object_unref (priv->adjustment);
      priv->adjustment = g_object_ref_sink (adjustment);

      if (priv->scale)
        bobgui_range_set_adjustment (BOBGUI_RANGE (priv->scale), adjustment);

      g_object_notify (G_OBJECT (button), "adjustment");

      bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (button),
                                      BOBGUI_ACCESSIBLE_PROPERTY_VALUE_MAX, bobgui_adjustment_get_upper (adjustment) -
                                                                         bobgui_adjustment_get_page_size (adjustment),
                                      BOBGUI_ACCESSIBLE_PROPERTY_VALUE_MIN, bobgui_adjustment_get_lower (adjustment),
                                      BOBGUI_ACCESSIBLE_PROPERTY_VALUE_NOW, bobgui_adjustment_get_value (adjustment),
                                      -1);

    }
}

/**
 * bobgui_scale_button_get_plus_button:
 * @button: a `BobguiScaleButton`
 *
 * Retrieves the plus button of the `BobguiScaleButton.`
 *
 * Returns: (transfer none) (type Bobgui.Button): the plus button
 *   of the `BobguiScaleButton`
 */
BobguiWidget *
bobgui_scale_button_get_plus_button (BobguiScaleButton *button)
{
  BobguiScaleButtonPrivate *priv = bobgui_scale_button_get_instance_private (button);

  g_return_val_if_fail (BOBGUI_IS_SCALE_BUTTON (button), NULL);

  return priv->plus_button;
}

/**
 * bobgui_scale_button_get_minus_button:
 * @button: a `BobguiScaleButton`
 *
 * Retrieves the minus button of the `BobguiScaleButton`.
 *
 * Returns: (transfer none) (type Bobgui.Button): the minus button
 *   of the `BobguiScaleButton`
 */
BobguiWidget *
bobgui_scale_button_get_minus_button (BobguiScaleButton *button)
{
  BobguiScaleButtonPrivate *priv = bobgui_scale_button_get_instance_private (button);

  g_return_val_if_fail (BOBGUI_IS_SCALE_BUTTON (button), NULL);

  return priv->minus_button;
}

/**
 * bobgui_scale_button_get_popup:
 * @button: a `BobguiScaleButton`
 *
 * Retrieves the popup of the `BobguiScaleButton`.
 *
 * Returns: (transfer none): the popup of the `BobguiScaleButton`
 */
BobguiWidget *
bobgui_scale_button_get_popup (BobguiScaleButton *button)
{
  BobguiScaleButtonPrivate *priv = bobgui_scale_button_get_instance_private (button);

  g_return_val_if_fail (BOBGUI_IS_SCALE_BUTTON (button), NULL);

  return priv->dock;
}

/**
 * bobgui_scale_button_get_active:
 * @button: a `BobguiScaleButton`
 *
 * Queries a `BobguiScaleButton` and returns its current state.
 *
 * Returns %TRUE if the scale button is pressed in and %FALSE
 * if it is raised.
 *
 * Returns: whether the button is pressed
 *
 * Since: 4.10
 */
gboolean
bobgui_scale_button_get_active (BobguiScaleButton *button)
{
  BobguiScaleButtonPrivate *priv = bobgui_scale_button_get_instance_private (button);

  g_return_val_if_fail (BOBGUI_IS_SCALE_BUTTON (button), FALSE);

  return bobgui_toggle_button_get_active (BOBGUI_TOGGLE_BUTTON (priv->button));
}

/**
 * bobgui_scale_button_get_has_frame:
 * @button: a `BobguiScaleButton`
 *
 * Returns whether the button has a frame.
 *
 * Returns: %TRUE if the button has a frame
 *
 * Since: 4.14
 */
gboolean
bobgui_scale_button_get_has_frame (BobguiScaleButton *button)
{
  BobguiScaleButtonPrivate *priv = bobgui_scale_button_get_instance_private (button);

  g_return_val_if_fail (BOBGUI_IS_SCALE_BUTTON (button), TRUE);

  return bobgui_button_get_has_frame (BOBGUI_BUTTON (priv->button));
}

/**
 * bobgui_scale_button_set_has_frame:
 * @button: a `BobguiScaleButton`
 * @has_frame: whether the button should have a visible frame
 *
 * Sets the style of the button.
 *
 * Since: 4.14
 */
void
bobgui_scale_button_set_has_frame (BobguiScaleButton *button,
                                gboolean        has_frame)
{
  BobguiScaleButtonPrivate *priv = bobgui_scale_button_get_instance_private (button);

  g_return_if_fail (BOBGUI_IS_SCALE_BUTTON (button));

  if (bobgui_button_get_has_frame (BOBGUI_BUTTON (priv->button)) == has_frame)
    return;

  bobgui_button_set_has_frame (BOBGUI_BUTTON (priv->button), has_frame);
  g_object_notify (G_OBJECT (button), "has-frame");
}

static void
apply_orientation (BobguiScaleButton *button,
                   BobguiOrientation  orientation)
{
  BobguiScaleButtonPrivate *priv = bobgui_scale_button_get_instance_private (button);

  if (priv->applied_orientation != orientation)
    {
      priv->applied_orientation = orientation;

      bobgui_orientable_set_orientation (BOBGUI_ORIENTABLE (priv->box), orientation);
      bobgui_orientable_set_orientation (BOBGUI_ORIENTABLE (priv->scale), orientation);

      if (orientation == BOBGUI_ORIENTATION_VERTICAL)
        {
          bobgui_box_reorder_child_after (BOBGUI_BOX (priv->box), priv->scale,
                                                            priv->plus_button);
          bobgui_box_reorder_child_after (BOBGUI_BOX (priv->box), priv->minus_button,
                                                            priv->scale);
          bobgui_widget_set_size_request (BOBGUI_WIDGET (priv->scale), -1, SCALE_SIZE);
          bobgui_range_set_inverted (BOBGUI_RANGE (priv->scale), TRUE);
        }
      else
        {
          bobgui_box_reorder_child_after (BOBGUI_BOX (priv->box), priv->scale,
                                                            priv->minus_button);
          bobgui_box_reorder_child_after (BOBGUI_BOX (priv->box), priv->plus_button,
                                                            priv->scale);
          bobgui_widget_set_size_request (BOBGUI_WIDGET (priv->scale), SCALE_SIZE, -1);
          bobgui_range_set_inverted (BOBGUI_RANGE (priv->scale), FALSE);
        }
    }
}

static void
bobgui_scale_button_set_orientation_private (BobguiScaleButton *button,
                                          BobguiOrientation  orientation)
{
  BobguiScaleButtonPrivate *priv = bobgui_scale_button_get_instance_private (button);

  if (priv->orientation != orientation)
    {
      priv->orientation = orientation;

      apply_orientation (button, priv->orientation);

      g_object_notify (G_OBJECT (button), "orientation");
    }
}

static gboolean
bobgui_scale_button_scroll_controller_scroll (BobguiEventControllerScroll *scroll,
                                           double                    dx,
                                           double                    dy,
                                           BobguiScaleButton           *button)
{
  BobguiScaleButtonPrivate *priv = bobgui_scale_button_get_instance_private (button);
  BobguiAdjustment *adjustment;
  double d;

  adjustment = priv->adjustment;

  d = CLAMP (bobgui_scale_button_get_value (button) -
             (dy * bobgui_adjustment_get_step_increment (adjustment)),
             bobgui_adjustment_get_lower (adjustment),
             bobgui_adjustment_get_upper (adjustment));

  bobgui_scale_button_set_value (button, d);

  return GDK_EVENT_STOP;
}

/*
 * button callbacks.
 */

static void
bobgui_scale_popup (BobguiWidget *widget)
{
  BobguiScaleButton *button = BOBGUI_SCALE_BUTTON (widget);
  BobguiScaleButtonPrivate *priv = bobgui_scale_button_get_instance_private (button);

  apply_orientation (button, priv->orientation);

  bobgui_toggle_button_set_active (BOBGUI_TOGGLE_BUTTON (priv->button), TRUE);
}

static void
bobgui_scale_button_popdown (BobguiWidget *widget)
{
  BobguiScaleButton *button = BOBGUI_SCALE_BUTTON (widget);
  BobguiScaleButtonPrivate *priv = bobgui_scale_button_get_instance_private (button);

  bobgui_toggle_button_set_active (BOBGUI_TOGGLE_BUTTON (priv->button), FALSE);
}

static void
bobgui_scale_button_popup (BobguiWidget *widget)
{
  bobgui_scale_popup (widget);
}

/*
 * +/- button callbacks.
 */
static gboolean
button_click (BobguiScaleButton *button,
              BobguiWidget      *active)
{
  BobguiScaleButtonPrivate *priv = bobgui_scale_button_get_instance_private (button);
  BobguiAdjustment *adjustment = priv->adjustment;
  gboolean can_continue = TRUE;
  double val;

  val = bobgui_scale_button_get_value (button);

  if (active == priv->plus_button)
    val += bobgui_adjustment_get_page_increment (adjustment);
  else
    val -= bobgui_adjustment_get_page_increment (adjustment);

  if (val <= bobgui_adjustment_get_lower (adjustment))
    {
      can_continue = FALSE;
      val = bobgui_adjustment_get_lower (adjustment);
    }
  else if (val > bobgui_adjustment_get_upper (adjustment))
    {
      can_continue = FALSE;
      val = bobgui_adjustment_get_upper (adjustment);
    }

  bobgui_scale_button_set_value (button, val);

  return can_continue;
}

static void
cb_button_clicked (BobguiWidget *widget,
                   gpointer   user_data)
{
  BobguiScaleButton *button = BOBGUI_SCALE_BUTTON (user_data);
  BobguiScaleButtonPrivate *priv = bobgui_scale_button_get_instance_private (button);

  if (priv->autoscroll_timeout)
    {
      g_source_remove (priv->autoscroll_timeout);
      priv->autoscroll_timeout = 0;
    }

  if (priv->autoscrolling)
    {
      bobgui_range_stop_autoscroll (BOBGUI_RANGE (priv->scale));
      priv->autoscrolling = FALSE;
      return;
    }

  button_click (button, widget);
}

static void
bobgui_scale_button_update_icon (BobguiScaleButton *button)
{
  BobguiScaleButtonPrivate *priv = bobgui_scale_button_get_instance_private (button);
  BobguiAdjustment *adjustment;
  double value;
  const char *name;
  guint num_icons;

  if (!priv->icon_list || !priv->icon_list[0] || priv->icon_list[0][0] == '\0')
    {
      bobgui_button_set_icon_name (BOBGUI_BUTTON (priv->button), "image-missing");
      return;
    }

  num_icons = g_strv_length (priv->icon_list);

  /* The 1-icon special case */
  if (num_icons == 1)
    {
      bobgui_button_set_icon_name (BOBGUI_BUTTON (priv->button), priv->icon_list[0]);
      return;
    }

  adjustment = priv->adjustment;
  value = bobgui_scale_button_get_value (button);

  /* The 2-icons special case */
  if (num_icons == 2)
    {
      double limit;

      limit = (bobgui_adjustment_get_upper (adjustment) - bobgui_adjustment_get_lower (adjustment)) / 2 + bobgui_adjustment_get_lower (adjustment);
      if (value < limit)
        name = priv->icon_list[0];
      else
        name = priv->icon_list[1];

      bobgui_button_set_icon_name (BOBGUI_BUTTON (priv->button), name);
      return;
    }

  /* With 3 or more icons */
  if (value == bobgui_adjustment_get_lower (adjustment))
    {
      name = priv->icon_list[0];
    }
  else if (value == bobgui_adjustment_get_upper (adjustment))
    {
      name = priv->icon_list[1];
    }
  else
    {
      double step;
      guint i;

      step = (bobgui_adjustment_get_upper (adjustment) - bobgui_adjustment_get_lower (adjustment)) / (num_icons - 2); i = (guint) ((value - bobgui_adjustment_get_lower (adjustment)) / step) + 2;
      g_assert (i < num_icons);
      name = priv->icon_list[i];
    }

  bobgui_button_set_icon_name (BOBGUI_BUTTON (priv->button), name);
}

static void
cb_scale_value_changed (BobguiRange *range,
                        gpointer  user_data)
{
  BobguiScaleButton *button = user_data;
  BobguiScaleButtonPrivate *priv = bobgui_scale_button_get_instance_private (button);
  double value;
  double upper, lower;

  value = bobgui_range_get_value (range);
  upper = bobgui_adjustment_get_upper (priv->adjustment);
  lower = bobgui_adjustment_get_lower (priv->adjustment);

  bobgui_scale_button_update_icon (button);

  bobgui_widget_set_sensitive (priv->plus_button, value < upper);
  bobgui_widget_set_sensitive (priv->minus_button, lower < value);

  g_signal_emit (button, signals[VALUE_CHANGED], 0, value);
  g_object_notify (G_OBJECT (button), "value");

  bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (button),
                                  BOBGUI_ACCESSIBLE_PROPERTY_VALUE_NOW, value,
                                  -1);
}

static void
cb_popup_mapped (BobguiWidget *popup,
                 gpointer   user_data)
{
  BobguiScaleButton *button = user_data;
  BobguiScaleButtonPrivate *priv = bobgui_scale_button_get_instance_private (button);

  bobgui_widget_grab_focus (priv->scale);
}

static void
bobgui_scale_button_measure (BobguiWidget      *widget,
                          BobguiOrientation  orientation,
                          int             for_size,
                          int            *minimum,
                          int            *natural,
                          int            *minimum_baseline,
                          int            *natural_baseline)
{
  BobguiScaleButton *button = BOBGUI_SCALE_BUTTON (widget);
  BobguiScaleButtonPrivate *priv = bobgui_scale_button_get_instance_private (button);

  bobgui_widget_measure (priv->button,
                      orientation,
                      for_size,
                      minimum, natural,
                      minimum_baseline, natural_baseline);

}

static void
bobgui_scale_button_size_allocate (BobguiWidget *widget,
                                int        width,
                                int        height,
                                int        baseline)
{
  BobguiScaleButton *button = BOBGUI_SCALE_BUTTON (widget);
  BobguiScaleButtonPrivate *priv = bobgui_scale_button_get_instance_private (button);

  bobgui_widget_size_allocate (priv->button,
                            &(BobguiAllocation) { 0, 0, width, height },
                            baseline);

  bobgui_popover_present (BOBGUI_POPOVER (priv->dock));
}
