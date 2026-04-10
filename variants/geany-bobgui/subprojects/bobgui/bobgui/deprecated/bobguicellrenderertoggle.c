/* bobguicellrenderertoggle.c
 * Copyright (C) 2000  Red Hat, Inc.,  Jonathan Blandford <jrb@redhat.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "bobguicellrenderertoggle.h"

#include "bobguicssnumbervalueprivate.h"
#include "bobguicsstransientnodeprivate.h"
#include "bobguimarshalers.h"
#include "bobguiprivate.h"
#include "bobguirendericonprivate.h"
#include "deprecated/bobguirender.h"
#include "bobguistylecontextprivate.h"
#include "bobguiwidgetprivate.h"
#include "deprecated/bobguitreeprivate.h"

#include <stdlib.h>

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

/**
 * BobguiCellRendererToggle:
 *
 * Renders a toggle button in a cell
 *
 * `BobguiCellRendererToggle` renders a toggle button in a cell. The
 * button is drawn as a radio or a checkbutton, depending on the
 * `BobguiCellRendererToggle:radio` property.
 * When activated, it emits the `BobguiCellRendererToggle::toggled` signal.
 *
 * Deprecated: 4.10: List views use widgets to display their contents.
 *   You should use [class@Bobgui.ToggleButton] instead
 */


static void bobgui_cell_renderer_toggle_get_property  (GObject                    *object,
						    guint                       param_id,
						    GValue                     *value,
						    GParamSpec                 *pspec);
static void bobgui_cell_renderer_toggle_set_property  (GObject                    *object,
						    guint                       param_id,
						    const GValue               *value,
						    GParamSpec                 *pspec);
static void bobgui_cell_renderer_toggle_get_size   (BobguiCellRendererToggle      *self,
						 BobguiWidget                  *widget,
						 const GdkRectangle         *cell_area,
						 int                        *x_offset,
						 int                        *y_offset,
						 int                        *width,
						 int                        *height);
static void bobgui_cell_renderer_toggle_snapshot   (BobguiCellRenderer            *cell,
						 BobguiSnapshot                *snapshot,
						 BobguiWidget                  *widget,
						 const GdkRectangle         *background_area,
						 const GdkRectangle         *cell_area,
						 BobguiCellRendererState        flags);
static gboolean bobgui_cell_renderer_toggle_activate  (BobguiCellRenderer            *cell,
						    GdkEvent                   *event,
						    BobguiWidget                  *widget,
						    const char                 *path,
						    const GdkRectangle         *background_area,
						    const GdkRectangle         *cell_area,
						    BobguiCellRendererState        flags);


enum {
  TOGGLED,
  LAST_SIGNAL
};

enum {
  PROP_0,
  PROP_ACTIVATABLE,
  PROP_ACTIVE,
  PROP_RADIO,
  PROP_INCONSISTENT
};

static guint toggle_cell_signals[LAST_SIGNAL] = { 0 };

typedef struct _BobguiCellRendererTogglePrivate       BobguiCellRendererTogglePrivate;
typedef struct _BobguiCellRendererToggleClass         BobguiCellRendererToggleClass;

struct _BobguiCellRendererToggle
{
  BobguiCellRenderer parent;
};

struct _BobguiCellRendererToggleClass
{
  BobguiCellRendererClass parent_class;

  void (* toggled) (BobguiCellRendererToggle *cell,
                    const char            *path);
};

struct _BobguiCellRendererTogglePrivate
{
  guint active       : 1;
  guint activatable  : 1;
  guint inconsistent : 1;
  guint radio        : 1;
  BobguiCssNode *cssnode;
};


G_DEFINE_TYPE_WITH_PRIVATE (BobguiCellRendererToggle, bobgui_cell_renderer_toggle, BOBGUI_TYPE_CELL_RENDERER)


static void
bobgui_cell_renderer_toggle_init (BobguiCellRendererToggle *celltoggle)
{
  BobguiCellRendererTogglePrivate *priv = bobgui_cell_renderer_toggle_get_instance_private (celltoggle);

  priv->activatable = TRUE;
  priv->active = FALSE;
  priv->radio = FALSE;

  g_object_set (celltoggle, "mode", BOBGUI_CELL_RENDERER_MODE_ACTIVATABLE, NULL);
  bobgui_cell_renderer_set_padding (BOBGUI_CELL_RENDERER (celltoggle), 2, 2);

  priv->inconsistent = FALSE;
}

static BobguiSizeRequestMode
bobgui_cell_renderer_toggle_get_request_mode (BobguiCellRenderer *cell)
{
  return BOBGUI_SIZE_REQUEST_CONSTANT_SIZE;
}

static void
bobgui_cell_renderer_toggle_get_preferred_width (BobguiCellRenderer *cell,
                                              BobguiWidget       *widget,
                                              int             *minimum,
                                              int             *natural)
{
  int width = 0;

  bobgui_cell_renderer_toggle_get_size (BOBGUI_CELL_RENDERER_TOGGLE (cell), widget,
                                     NULL,
                                     NULL, NULL, &width, NULL);

  if (minimum)
    *minimum = width;
  if (natural)
    *natural = width;
}

static void
bobgui_cell_renderer_toggle_get_preferred_height (BobguiCellRenderer *cell,
                                               BobguiWidget       *widget,
                                               int             *minimum,
                                               int             *natural)
{
  int height = 0;

  bobgui_cell_renderer_toggle_get_size (BOBGUI_CELL_RENDERER_TOGGLE (cell), widget,
                                     NULL,
                                     NULL, NULL, NULL, &height);

  if (minimum)
    *minimum = height;
  if (natural)
    *natural = height;
}

static void
bobgui_cell_renderer_toggle_class_init (BobguiCellRendererToggleClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  BobguiCellRendererClass *cell_class = BOBGUI_CELL_RENDERER_CLASS (class);

  object_class->get_property = bobgui_cell_renderer_toggle_get_property;
  object_class->set_property = bobgui_cell_renderer_toggle_set_property;

  cell_class->get_request_mode = bobgui_cell_renderer_toggle_get_request_mode;
  cell_class->get_preferred_width = bobgui_cell_renderer_toggle_get_preferred_width;
  cell_class->get_preferred_height = bobgui_cell_renderer_toggle_get_preferred_height;
  cell_class->snapshot = bobgui_cell_renderer_toggle_snapshot;
  cell_class->activate = bobgui_cell_renderer_toggle_activate;

  g_object_class_install_property (object_class,
				   PROP_ACTIVE,
				   g_param_spec_boolean ("active", NULL, NULL,
							 FALSE,
							 BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  g_object_class_install_property (object_class,
		                   PROP_INCONSISTENT,
				   g_param_spec_boolean ("inconsistent", NULL, NULL,
							 FALSE,
							 BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  g_object_class_install_property (object_class,
				   PROP_ACTIVATABLE,
				   g_param_spec_boolean ("activatable", NULL, NULL,
							 TRUE,
							 BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  g_object_class_install_property (object_class,
				   PROP_RADIO,
				   g_param_spec_boolean ("radio", NULL, NULL,
							 FALSE,
							 BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));


  /**
   * BobguiCellRendererToggle::toggled:
   * @cell_renderer: the object which received the signal
   * @path: string representation of `BobguiTreePath` describing the
   *        event location
   *
   * The ::toggled signal is emitted when the cell is toggled.
   *
   * It is the responsibility of the application to update the model
   * with the correct value to store at @path.  Often this is simply the
   * opposite of the value currently stored at @path.
   **/
  toggle_cell_signals[TOGGLED] =
    g_signal_new (I_("toggled"),
		  G_OBJECT_CLASS_TYPE (object_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BobguiCellRendererToggleClass, toggled),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 1,
		  G_TYPE_STRING);
}

static void
bobgui_cell_renderer_toggle_get_property (GObject     *object,
				       guint        param_id,
				       GValue      *value,
				       GParamSpec  *pspec)
{
  BobguiCellRendererToggle *celltoggle = BOBGUI_CELL_RENDERER_TOGGLE (object);
  BobguiCellRendererTogglePrivate *priv = bobgui_cell_renderer_toggle_get_instance_private (celltoggle);

  switch (param_id)
    {
    case PROP_ACTIVE:
      g_value_set_boolean (value, priv->active);
      break;
    case PROP_INCONSISTENT:
      g_value_set_boolean (value, priv->inconsistent);
      break;
    case PROP_ACTIVATABLE:
      g_value_set_boolean (value, priv->activatable);
      break;
    case PROP_RADIO:
      g_value_set_boolean (value, priv->radio);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}


static void
bobgui_cell_renderer_toggle_set_property (GObject      *object,
				       guint         param_id,
				       const GValue *value,
				       GParamSpec   *pspec)
{
  BobguiCellRendererToggle *celltoggle = BOBGUI_CELL_RENDERER_TOGGLE (object);
  BobguiCellRendererTogglePrivate *priv = bobgui_cell_renderer_toggle_get_instance_private (celltoggle);

  switch (param_id)
    {
    case PROP_ACTIVE:
      if (priv->active != g_value_get_boolean (value))
        {
          priv->active = g_value_get_boolean (value);
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    case PROP_INCONSISTENT:
      if (priv->inconsistent != g_value_get_boolean (value))
        {
          priv->inconsistent = g_value_get_boolean (value);
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    case PROP_ACTIVATABLE:
      if (priv->activatable != g_value_get_boolean (value))
        {
          priv->activatable = g_value_get_boolean (value);
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    case PROP_RADIO:
      if (priv->radio != g_value_get_boolean (value))
        {
          bobgui_cell_renderer_toggle_set_radio (celltoggle, g_value_get_boolean (value));
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

/**
 * bobgui_cell_renderer_toggle_new:
 *
 * Creates a new `BobguiCellRendererToggle`. Adjust rendering
 * parameters using object properties. Object properties can be set
 * globally (with g_object_set()). Also, with `BobguiTreeViewColumn`, you
 * can bind a property to a value in a `BobguiTreeModel`. For example, you
 * can bind the “active” property on the cell renderer to a boolean value
 * in the model, thus causing the check button to reflect the state of
 * the model.
 *
 * Returns: the new cell renderer
 *
 * Deprecated: 4.10
 **/
BobguiCellRenderer *
bobgui_cell_renderer_toggle_new (void)
{
  return g_object_new (BOBGUI_TYPE_CELL_RENDERER_TOGGLE, NULL);
}

static BobguiStyleContext *
bobgui_cell_renderer_toggle_save_context (BobguiCellRendererToggle *cell,
                                       BobguiWidget             *widget)
{
  BobguiCellRendererTogglePrivate *priv = bobgui_cell_renderer_toggle_get_instance_private (cell);
  BobguiStyleContext *context;
  BobguiCssNode *cssnode;

  context = bobgui_widget_get_style_context (widget);

  cssnode = bobgui_css_transient_node_new (bobgui_widget_get_css_node (widget));
  if (priv->radio)
    bobgui_css_node_set_name (cssnode, g_quark_from_static_string ("radio"));
  else
    bobgui_css_node_set_name (cssnode, g_quark_from_static_string ("check"));
  bobgui_style_context_save_to_node (context, cssnode);
  g_object_unref (cssnode);

  return context;
}

static void
bobgui_cell_renderer_toggle_restore_context (BobguiCellRendererToggle *cell,
                                          BobguiStyleContext       *context)
{
  bobgui_style_context_restore (context);
}

static int
calc_indicator_size (BobguiStyleContext *context)
{
  BobguiCssStyle *style = bobgui_style_context_lookup_style (context);
  return bobgui_css_number_value_get (style->icon->icon_size, 100);
}

static void
bobgui_cell_renderer_toggle_get_size (BobguiCellRendererToggle *self,
				   BobguiWidget             *widget,
				   const GdkRectangle    *cell_area,
				   int                   *x_offset,
				   int                   *y_offset,
				   int                   *width,
				   int                   *height)
{
  BobguiCellRenderer *cell = BOBGUI_CELL_RENDERER (self);
  int calc_width;
  int calc_height;
  int xpad, ypad;
  BobguiStyleContext *context;
  BobguiBorder border, padding;

  bobgui_cell_renderer_get_padding (cell, &xpad, &ypad);

  context = bobgui_cell_renderer_toggle_save_context (self, widget);
  bobgui_style_context_get_padding (context, &padding);
  bobgui_style_context_get_border (context, &border);

  calc_width = calc_height = calc_indicator_size (context);
  calc_width += xpad * 2 + padding.left + padding.right + border.left + border.right;
  calc_height += ypad * 2 + padding.top + padding.bottom + border.top + border.bottom;

  bobgui_cell_renderer_toggle_restore_context (self, context);

  if (width)
    *width = calc_width;

  if (height)
    *height = calc_height;

  if (cell_area)
    {
      float xalign, yalign;

      bobgui_cell_renderer_get_alignment (cell, &xalign, &yalign);

      if (x_offset)
	{
	  *x_offset = ((bobgui_widget_get_direction (widget) == BOBGUI_TEXT_DIR_RTL) ?
		       (1.0 - xalign) : xalign) * (cell_area->width - calc_width);
	  *x_offset = MAX (*x_offset, 0);
	}
      if (y_offset)
	{
	  *y_offset = yalign * (cell_area->height - calc_height);
	  *y_offset = MAX (*y_offset, 0);
	}
    }
  else
    {
      if (x_offset)
        *x_offset = 0;
      if (y_offset)
        *y_offset = 0;
    }
}

static void
bobgui_cell_renderer_toggle_snapshot (BobguiCellRenderer      *cell,
				   BobguiSnapshot          *snapshot,
                                   BobguiWidget            *widget,
                                   const GdkRectangle   *background_area,
                                   const GdkRectangle   *cell_area,
                                   BobguiCellRendererState  flags)
{
  BobguiCellRendererToggle *celltoggle = BOBGUI_CELL_RENDERER_TOGGLE (cell);
  BobguiCellRendererTogglePrivate *priv = bobgui_cell_renderer_toggle_get_instance_private (celltoggle);
  BobguiStyleContext *context;
  int width, height;
  int x_offset, y_offset;
  int xpad, ypad;
  BobguiStateFlags state;
  BobguiBorder padding, border;

  bobgui_cell_renderer_toggle_get_size (celltoggle, widget, cell_area,
				     &x_offset, &y_offset,
				     &width, &height);
  bobgui_cell_renderer_get_padding (cell, &xpad, &ypad);
  width -= xpad * 2;
  height -= ypad * 2;

  if (width <= 0 || height <= 0)
    return;

  state = bobgui_cell_renderer_get_state (cell, widget, flags);

  if (!priv->activatable)
    state |= BOBGUI_STATE_FLAG_INSENSITIVE;

  state &= ~(BOBGUI_STATE_FLAG_INCONSISTENT | BOBGUI_STATE_FLAG_CHECKED);

  if (priv->inconsistent)
    state |= BOBGUI_STATE_FLAG_INCONSISTENT;

  if (priv->active)
    state |= BOBGUI_STATE_FLAG_CHECKED;

  bobgui_snapshot_push_clip (snapshot,
                          &GRAPHENE_RECT_INIT (
                             cell_area->x, cell_area->y,
                             cell_area->width, cell_area->height
                          ));

  context = bobgui_cell_renderer_toggle_save_context (celltoggle, widget);
  bobgui_style_context_set_state (context, state);

  bobgui_snapshot_render_background (snapshot, context,
                                  cell_area->x + x_offset + xpad,
                                  cell_area->y + y_offset + ypad,
                                  width, height);
  bobgui_snapshot_render_frame (snapshot, context,
                             cell_area->x + x_offset + xpad,
                             cell_area->y + y_offset + ypad,
                             width, height);

  bobgui_style_context_get_padding (context, &padding);
  bobgui_style_context_get_border (context, &border);

  bobgui_snapshot_translate (snapshot,
                          &GRAPHENE_POINT_INIT (cell_area->x + x_offset + xpad + padding.left + border.left,
                                                cell_area->y + y_offset + ypad + padding.top + border.top));
  bobgui_css_style_snapshot_icon (bobgui_style_context_lookup_style (context), snapshot,
                               width - padding.left - padding.right - border.left - border.right,
                               height - padding.top - padding.bottom - border.top - border.bottom);

  bobgui_cell_renderer_toggle_restore_context (celltoggle, context);
  bobgui_snapshot_pop (snapshot);
}

static int
bobgui_cell_renderer_toggle_activate (BobguiCellRenderer      *cell,
				   GdkEvent             *event,
				   BobguiWidget            *widget,
				   const char           *path,
				   const GdkRectangle   *background_area,
				   const GdkRectangle   *cell_area,
				   BobguiCellRendererState  flags)
{
  BobguiCellRendererToggle *celltoggle = BOBGUI_CELL_RENDERER_TOGGLE (cell);
  BobguiCellRendererTogglePrivate *priv = bobgui_cell_renderer_toggle_get_instance_private (celltoggle);

  if (priv->activatable)
    {
      g_signal_emit (cell, toggle_cell_signals[TOGGLED], 0, path);
      return TRUE;
    }

  return FALSE;
}

/**
 * bobgui_cell_renderer_toggle_set_radio:
 * @toggle: a `BobguiCellRendererToggle`
 * @radio: %TRUE to make the toggle look like a radio button
 *
 * If @radio is %TRUE, the cell renderer renders a radio toggle
 * (i.e. a toggle in a group of mutually-exclusive toggles).
 * If %FALSE, it renders a check toggle (a standalone boolean option).
 * This can be set globally for the cell renderer, or changed just
 * before rendering each cell in the model (for `BobguiTreeView`, you set
 * up a per-row setting using `BobguiTreeViewColumn` to associate model
 * columns with cell renderer properties).
 *
 * Deprecated: 4.10
 **/
void
bobgui_cell_renderer_toggle_set_radio (BobguiCellRendererToggle *toggle,
				    gboolean               radio)
{
  BobguiCellRendererTogglePrivate *priv = bobgui_cell_renderer_toggle_get_instance_private (toggle);

  g_return_if_fail (BOBGUI_IS_CELL_RENDERER_TOGGLE (toggle));

  priv->radio = radio;
}

/**
 * bobgui_cell_renderer_toggle_get_radio:
 * @toggle: a `BobguiCellRendererToggle`
 *
 * Returns whether we’re rendering radio toggles rather than checkboxes.
 *
 * Returns: %TRUE if we’re rendering radio toggles rather than checkboxes
 *
 * Deprecated: 4.10
 **/
gboolean
bobgui_cell_renderer_toggle_get_radio (BobguiCellRendererToggle *toggle)
{
  BobguiCellRendererTogglePrivate *priv = bobgui_cell_renderer_toggle_get_instance_private (toggle);

  g_return_val_if_fail (BOBGUI_IS_CELL_RENDERER_TOGGLE (toggle), FALSE);

  return priv->radio;
}

/**
 * bobgui_cell_renderer_toggle_get_active:
 * @toggle: a `BobguiCellRendererToggle`
 *
 * Returns whether the cell renderer is active. See
 * bobgui_cell_renderer_toggle_set_active().
 *
 * Returns: %TRUE if the cell renderer is active.
 *
 * Deprecated: 4.10
 **/
gboolean
bobgui_cell_renderer_toggle_get_active (BobguiCellRendererToggle *toggle)
{
  BobguiCellRendererTogglePrivate *priv = bobgui_cell_renderer_toggle_get_instance_private (toggle);

  g_return_val_if_fail (BOBGUI_IS_CELL_RENDERER_TOGGLE (toggle), FALSE);

  return priv->active;
}

/**
 * bobgui_cell_renderer_toggle_set_active:
 * @toggle: a `BobguiCellRendererToggle`.
 * @setting: the value to set.
 *
 * Activates or deactivates a cell renderer.
 *
 * Deprecated: 4.10
 **/
void
bobgui_cell_renderer_toggle_set_active (BobguiCellRendererToggle *toggle,
				     gboolean               setting)
{
  g_return_if_fail (BOBGUI_IS_CELL_RENDERER_TOGGLE (toggle));

  g_object_set (toggle, "active", setting ? TRUE : FALSE, NULL);
}

/**
 * bobgui_cell_renderer_toggle_get_activatable:
 * @toggle: a `BobguiCellRendererToggle`
 *
 * Returns whether the cell renderer is activatable. See
 * bobgui_cell_renderer_toggle_set_activatable().
 *
 * Returns: %TRUE if the cell renderer is activatable.
 *
 * Deprecated: 4.10
 **/
gboolean
bobgui_cell_renderer_toggle_get_activatable (BobguiCellRendererToggle *toggle)
{
  BobguiCellRendererTogglePrivate *priv = bobgui_cell_renderer_toggle_get_instance_private (toggle);

  g_return_val_if_fail (BOBGUI_IS_CELL_RENDERER_TOGGLE (toggle), FALSE);

  return priv->activatable;
}

/**
 * bobgui_cell_renderer_toggle_set_activatable:
 * @toggle: a `BobguiCellRendererToggle`.
 * @setting: the value to set.
 *
 * Makes the cell renderer activatable.
 *
 * Deprecated: 4.10
 **/
void
bobgui_cell_renderer_toggle_set_activatable (BobguiCellRendererToggle *toggle,
                                          gboolean               setting)
{
  BobguiCellRendererTogglePrivate *priv = bobgui_cell_renderer_toggle_get_instance_private (toggle);

  g_return_if_fail (BOBGUI_IS_CELL_RENDERER_TOGGLE (toggle));

  if (priv->activatable != setting)
    {
      priv->activatable = setting ? TRUE : FALSE;
      g_object_notify (G_OBJECT (toggle), "activatable");
    }
}
