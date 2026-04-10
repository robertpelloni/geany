/* BobguiCellRendererSpin
 * Copyright (C) 2004 Lorenzo Gil Sanchez
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
 *
 * Authors: Lorenzo Gil Sanchez    <lgs@sicem.biz>
 *          Carlos Garnacho Parro  <carlosg@gnome.org>
 */

#include "config.h"

#include "bobguicellrendererspin.h"

#include "bobguiadjustment.h"
#include "bobguiprivate.h"
#include "bobguispinbutton.h"
#include "bobguientry.h"
#include "bobguieventcontrollerkey.h"

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

/**
 * BobguiCellRendererSpin:
 *
 * Renders a spin button in a cell
 *
 * `BobguiCellRendererSpin` renders text in a cell like `BobguiCellRendererText` from
 * which it is derived. But while `BobguiCellRendererText` offers a simple entry to
 * edit the text, `BobguiCellRendererSpin` offers a `BobguiSpinButton` widget. Of course,
 * that means that the text has to be parseable as a floating point number.
 *
 * The range of the spinbutton is taken from the adjustment property of the
 * cell renderer, which can be set explicitly or mapped to a column in the
 * tree model, like all properties of cell renders. `BobguiCellRendererSpin`
 * also has properties for the `BobguiCellRendererSpin:climb-rate` and the number
 * of `BobguiCellRendererSpin:digits` to display. Other `BobguiSpinButton` properties
 * can be set in a handler for the `BobguiCellRenderer::editing-started` signal.
 *
 * Deprecated: 4.10: List views use widgets to display their contents.
 *   You should use [class@Bobgui.SpinButton] instead
 */

typedef struct _BobguiCellRendererSpinClass   BobguiCellRendererSpinClass;
typedef struct _BobguiCellRendererSpinPrivate BobguiCellRendererSpinPrivate;

struct _BobguiCellRendererSpin
{
  BobguiCellRendererText parent;
};

struct _BobguiCellRendererSpinClass
{
  BobguiCellRendererTextClass parent;
};

struct _BobguiCellRendererSpinPrivate
{
  BobguiWidget *spin;
  BobguiAdjustment *adjustment;
  double climb_rate;
  guint   digits;
};

static void bobgui_cell_renderer_spin_finalize   (GObject                  *object);

static void bobgui_cell_renderer_spin_get_property (GObject      *object,
						 guint         prop_id,
						 GValue       *value,
						 GParamSpec   *spec);
static void bobgui_cell_renderer_spin_set_property (GObject      *object,
						 guint         prop_id,
						 const GValue *value,
						 GParamSpec   *spec);

static gboolean bobgui_cell_renderer_spin_key_pressed (BobguiEventControllerKey *controller,
                                                    guint                  keyval,
                                                    guint                  keycode,
                                                    GdkModifierType        state,
                                                    BobguiWidget             *widget);

static BobguiCellEditable * bobgui_cell_renderer_spin_start_editing (BobguiCellRenderer     *cell,
							       GdkEvent            *event,
							       BobguiWidget           *widget,
							       const char          *path,
							       const GdkRectangle  *background_area,
							       const GdkRectangle  *cell_area,
							       BobguiCellRendererState flags);
enum {
  PROP_0,
  PROP_ADJUSTMENT,
  PROP_CLIMB_RATE,
  PROP_DIGITS
};

#define BOBGUI_CELL_RENDERER_SPIN_PATH "bobgui-cell-renderer-spin-path"

G_DEFINE_TYPE_WITH_PRIVATE (BobguiCellRendererSpin, bobgui_cell_renderer_spin, BOBGUI_TYPE_CELL_RENDERER_TEXT)


static void
bobgui_cell_renderer_spin_class_init (BobguiCellRendererSpinClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  BobguiCellRendererClass *cell_class = BOBGUI_CELL_RENDERER_CLASS (klass);

  object_class->finalize     = bobgui_cell_renderer_spin_finalize;
  object_class->get_property = bobgui_cell_renderer_spin_get_property;
  object_class->set_property = bobgui_cell_renderer_spin_set_property;

  cell_class->start_editing  = bobgui_cell_renderer_spin_start_editing;

  /**
   * BobguiCellRendererSpin:adjustment:
   *
   * The adjustment that holds the value of the spinbutton.
   * This must be non-%NULL for the cell renderer to be editable.
   */
  g_object_class_install_property (object_class,
				   PROP_ADJUSTMENT,
				   g_param_spec_object ("adjustment", NULL, NULL,
							BOBGUI_TYPE_ADJUSTMENT,
							BOBGUI_PARAM_READWRITE));


  /**
   * BobguiCellRendererSpin:climb-rate:
   *
   * The acceleration rate when you hold down a button.
   */
  g_object_class_install_property (object_class,
				   PROP_CLIMB_RATE,
				   g_param_spec_double ("climb-rate", NULL, NULL,
							0.0, G_MAXDOUBLE, 0.0,
							BOBGUI_PARAM_READWRITE));
  /**
   * BobguiCellRendererSpin:digits:
   *
   * The number of decimal places to display.
   */
  g_object_class_install_property (object_class,
				   PROP_DIGITS,
				   g_param_spec_uint ("digits", NULL, NULL,
						      0, 20, 0,
						      BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));
}

static void
bobgui_cell_renderer_spin_init (BobguiCellRendererSpin *self)
{
  BobguiCellRendererSpinPrivate *priv = bobgui_cell_renderer_spin_get_instance_private (self);

  priv->adjustment = NULL;
  priv->climb_rate = 0.0;
  priv->digits = 0;
}

static void
bobgui_cell_renderer_spin_finalize (GObject *object)
{
  BobguiCellRendererSpinPrivate *priv = bobgui_cell_renderer_spin_get_instance_private (BOBGUI_CELL_RENDERER_SPIN (object));

  g_clear_object (&priv->adjustment);
  g_clear_object (&priv->spin);

  G_OBJECT_CLASS (bobgui_cell_renderer_spin_parent_class)->finalize (object);
}

static void
bobgui_cell_renderer_spin_get_property (GObject      *object,
				     guint         prop_id,
				     GValue       *value,
				     GParamSpec   *pspec)
{
  BobguiCellRendererSpinPrivate *priv = bobgui_cell_renderer_spin_get_instance_private (BOBGUI_CELL_RENDERER_SPIN (object));

  switch (prop_id)
    {
    case PROP_ADJUSTMENT:
      g_value_set_object (value, priv->adjustment);
      break;
    case PROP_CLIMB_RATE:
      g_value_set_double (value, priv->climb_rate);
      break;
    case PROP_DIGITS:
      g_value_set_uint (value, priv->digits);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_cell_renderer_spin_set_property (GObject      *object,
				     guint         prop_id,
				     const GValue *value,
				     GParamSpec   *pspec)
{
  BobguiCellRendererSpinPrivate *priv = bobgui_cell_renderer_spin_get_instance_private (BOBGUI_CELL_RENDERER_SPIN (object));
  GObject *obj;

  switch (prop_id)
    {
    case PROP_ADJUSTMENT:
      obj = g_value_get_object (value);

      if (priv->adjustment)
	{
	  g_object_unref (priv->adjustment);
	  priv->adjustment = NULL;
	}

      if (obj)
	priv->adjustment = BOBGUI_ADJUSTMENT (g_object_ref_sink (obj));

      break;
    case PROP_CLIMB_RATE:
      priv->climb_rate = g_value_get_double (value);
      break;
    case PROP_DIGITS:
      if (priv->digits != g_value_get_uint (value))
        {
          priv->digits = g_value_get_uint (value);
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_cell_renderer_spin_focus_changed (BobguiWidget  *widget,
                                      GParamSpec *pspec,
                                      gpointer    data)
{
  const char *path;
  const char *new_text;
  gboolean canceled;

  if (bobgui_widget_has_focus (widget))
    return;

  g_object_get (widget, "editing-canceled", &canceled, NULL);

  g_signal_handlers_disconnect_by_func (widget,
					bobgui_cell_renderer_spin_focus_changed,
					data);

  bobgui_cell_renderer_stop_editing (BOBGUI_CELL_RENDERER (data), canceled);

  if (canceled)
    return;

  path = g_object_get_data (G_OBJECT (widget), BOBGUI_CELL_RENDERER_SPIN_PATH);
  new_text = bobgui_editable_get_text (BOBGUI_EDITABLE (widget));
  g_signal_emit_by_name (data, "edited", path, new_text);
}

static gboolean
bobgui_cell_renderer_spin_key_pressed (BobguiEventControllerKey *controller,
                                    guint                  keyval,
                                    guint                  keycode,
                                    GdkModifierType        state,
                                    BobguiWidget             *widget)
{
  if (state == 0)
    {
      if (keyval == GDK_KEY_Up)
	{
	  bobgui_spin_button_spin (BOBGUI_SPIN_BUTTON (widget), BOBGUI_SPIN_STEP_FORWARD, 1);
	  return TRUE;
	}
      else if (keyval == GDK_KEY_Down)
	{
	  bobgui_spin_button_spin (BOBGUI_SPIN_BUTTON (widget), BOBGUI_SPIN_STEP_BACKWARD, 1);
	  return TRUE;
	}
    }

  return FALSE;
}

static void
bobgui_cell_renderer_spin_editing_done (BobguiSpinButton       *spin,
                                     BobguiCellRendererSpin *cell)
{
  BobguiCellRendererSpinPrivate *priv = bobgui_cell_renderer_spin_get_instance_private (BOBGUI_CELL_RENDERER_SPIN (cell));
  gboolean canceled;
  const char *path;
  const char *new_text;

  g_clear_object (&priv->spin);

  g_object_get (spin, "editing-canceled", &canceled, NULL);
  bobgui_cell_renderer_stop_editing (BOBGUI_CELL_RENDERER (cell), canceled);

  if (canceled)
    return;

  path = g_object_get_data (G_OBJECT (spin), BOBGUI_CELL_RENDERER_SPIN_PATH);
  new_text = bobgui_editable_get_text (BOBGUI_EDITABLE (spin));
  g_signal_emit_by_name (cell, "edited", path, new_text);
}

static BobguiCellEditable *
bobgui_cell_renderer_spin_start_editing (BobguiCellRenderer      *cell,
				      GdkEvent             *event,
				      BobguiWidget            *widget,
				      const char           *path,
				      const GdkRectangle   *background_area,
				      const GdkRectangle   *cell_area,
				      BobguiCellRendererState  flags)
{
  BobguiCellRendererSpinPrivate *priv = bobgui_cell_renderer_spin_get_instance_private (BOBGUI_CELL_RENDERER_SPIN (cell));
  BobguiCellRendererText *cell_text = BOBGUI_CELL_RENDERER_TEXT (cell);
  BobguiEventController *key_controller;
  gboolean editable;
  char *text;

  g_object_get (cell_text, "editable", &editable, NULL);
  if (!editable)
    return NULL;

  if (!priv->adjustment)
    return NULL;

  priv->spin = bobgui_spin_button_new (priv->adjustment, priv->climb_rate, priv->digits);
  g_object_ref_sink (priv->spin);

  g_object_get (cell_text, "text", &text, NULL);
  if (text)
    {
      bobgui_spin_button_set_value (BOBGUI_SPIN_BUTTON (priv->spin), g_strtod (text, NULL));
      g_free (text);
    }

  key_controller = bobgui_event_controller_key_new ();
  g_signal_connect (key_controller, "key-pressed",
                    G_CALLBACK (bobgui_cell_renderer_spin_key_pressed), priv->spin);
  bobgui_widget_add_controller (priv->spin, key_controller);

  g_object_set_data_full (G_OBJECT (priv->spin), BOBGUI_CELL_RENDERER_SPIN_PATH,
			  g_strdup (path), g_free);

  g_signal_connect (priv->spin, "editing-done",
                    G_CALLBACK (bobgui_cell_renderer_spin_editing_done), cell);

  g_signal_connect (priv->spin, "notify::has-focus",
		    G_CALLBACK (bobgui_cell_renderer_spin_focus_changed), cell);

  return BOBGUI_CELL_EDITABLE (priv->spin);
}

/**
 * bobgui_cell_renderer_spin_new:
 *
 * Creates a new `BobguiCellRendererSpin`.
 *
 * Returns: a new `BobguiCellRendererSpin`
 *
 * Deprecated: 4.10
 */
BobguiCellRenderer *
bobgui_cell_renderer_spin_new (void)
{
  return g_object_new (BOBGUI_TYPE_CELL_RENDERER_SPIN, NULL);
}
