/* bobguicellrendererprogress.c
 * Copyright (C) 2002 Naba Kumar <kh_naba@users.sourceforge.net>
 * heavily modified by Jörgen Scheibengruber <mfcn@gmx.de>
 * heavily modified by Marco Pesenti Gritti <marco@gnome.org>
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
/*
 * Modified by the BOBGUI Team and others 1997-2007.  See the AUTHORS
 * file for a list of people on the BOBGUI Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BOBGUI at ftp://ftp.bobgui.org/pub/bobgui/.
 */

#include "config.h"
#include <stdlib.h>

#include "bobguicellrendererprogress.h"
#include <glib/gi18n-lib.h>
#include "bobguiorientable.h"
#include "bobguiprivate.h"
#include "deprecated/bobguirender.h"
#include "bobguistylecontext.h"

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

/**
 * BobguiCellRendererProgress:
 *
 * Renders numbers as progress bars
 *
 * `BobguiCellRendererProgress` renders a numeric value as a progress par in a cell.
 * Additionally, it can display a text on top of the progress bar.
 *
 * Deprecated: 4.10: List views use widgets to display their contents.
 *   You should use [class@Bobgui.ProgressBar] instead
 */


enum
{
  PROP_0,
  PROP_VALUE,
  PROP_TEXT,
  PROP_PULSE,
  PROP_TEXT_XALIGN,
  PROP_TEXT_YALIGN,
  PROP_ORIENTATION,
  PROP_INVERTED
};

typedef struct _BobguiCellRendererProgressClass    BobguiCellRendererProgressClass;
typedef struct _BobguiCellRendererProgressPrivate  BobguiCellRendererProgressPrivate;

struct _BobguiCellRendererProgress
{
  BobguiCellRenderer parent_instance;
};

struct _BobguiCellRendererProgressClass
{
  BobguiCellRendererClass parent_class;
};

struct _BobguiCellRendererProgressPrivate
{
  int value;
  char *text;
  char *label;
  int min_h;
  int min_w;
  int pulse;
  int offset;
  float text_xalign;
  float text_yalign;
  BobguiOrientation orientation;
  gboolean inverted;
};

static void bobgui_cell_renderer_progress_finalize     (GObject                 *object);
static void bobgui_cell_renderer_progress_get_property (GObject                 *object,
						     guint                    param_id,
						     GValue                  *value,
						     GParamSpec              *pspec);
static void bobgui_cell_renderer_progress_set_property (GObject                 *object,
						     guint                    param_id,
						     const GValue            *value,
						     GParamSpec              *pspec);
static void bobgui_cell_renderer_progress_set_value    (BobguiCellRendererProgress *cellprogress,
						     int                      value);
static void bobgui_cell_renderer_progress_set_text     (BobguiCellRendererProgress *cellprogress,
						     const char              *text);
static void bobgui_cell_renderer_progress_set_pulse    (BobguiCellRendererProgress *cellprogress,
						     int                      pulse);
static void compute_dimensions                      (BobguiCellRenderer         *cell,
						     BobguiWidget               *widget,
						     const char              *text,
						     int                     *width,
						     int                     *height);
static void bobgui_cell_renderer_progress_snapshot     (BobguiCellRenderer         *cell,
						     BobguiSnapshot             *snapshot,
						     BobguiWidget               *widget,
						     const GdkRectangle      *background_area,
						     const GdkRectangle      *cell_area,
				                     BobguiCellRendererState    flags);


G_DEFINE_TYPE_WITH_CODE (BobguiCellRendererProgress, bobgui_cell_renderer_progress, BOBGUI_TYPE_CELL_RENDERER,
                         G_ADD_PRIVATE (BobguiCellRendererProgress)
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_ORIENTABLE, NULL))

static void
recompute_label (BobguiCellRendererProgress *cellprogress)
{
  BobguiCellRendererProgressPrivate *priv = bobgui_cell_renderer_progress_get_instance_private (cellprogress);
  char *label;

  if (priv->text)
    label = g_strdup (priv->text);
  else if (priv->pulse < 0)
    label = g_strdup_printf (C_("progress bar label", "%d %%"), priv->value);
  else
    label = NULL;

  g_free (priv->label);
  priv->label = label;
}

static void
bobgui_cell_renderer_progress_set_value (BobguiCellRendererProgress *cellprogress,
				      int                      value)
{
  BobguiCellRendererProgressPrivate *priv = bobgui_cell_renderer_progress_get_instance_private (cellprogress);

  if (priv->value != value)
    {
      priv->value = value;
      recompute_label (cellprogress);
      g_object_notify (G_OBJECT (cellprogress), "value");
    }
}

static void
bobgui_cell_renderer_progress_set_text (BobguiCellRendererProgress *cellprogress,
				     const char              *text)
{
  BobguiCellRendererProgressPrivate *priv = bobgui_cell_renderer_progress_get_instance_private (cellprogress);
  char *new_text;

  new_text = g_strdup (text);
  g_free (priv->text);
  priv->text = new_text;
  recompute_label (cellprogress);
  g_object_notify (G_OBJECT (cellprogress), "text");
}

static void
bobgui_cell_renderer_progress_set_pulse (BobguiCellRendererProgress *cellprogress,
				      int                      pulse)
{
  BobguiCellRendererProgressPrivate *priv = bobgui_cell_renderer_progress_get_instance_private (cellprogress);

  if (pulse != priv->pulse)
    {
      if (pulse <= 0)
        priv->offset = 0;
      else
        priv->offset = pulse;
      g_object_notify (G_OBJECT (cellprogress), "pulse");
    }

  priv->pulse = pulse;
  recompute_label (cellprogress);
}

static void
bobgui_cell_renderer_progress_finalize (GObject *object)
{
  BobguiCellRendererProgress *cellprogress = BOBGUI_CELL_RENDERER_PROGRESS (object);
  BobguiCellRendererProgressPrivate *priv = bobgui_cell_renderer_progress_get_instance_private (cellprogress);

  g_free (priv->text);
  g_free (priv->label);

  G_OBJECT_CLASS (bobgui_cell_renderer_progress_parent_class)->finalize (object);
}

static void
bobgui_cell_renderer_progress_get_property (GObject    *object,
					 guint       param_id,
					 GValue     *value,
					 GParamSpec *pspec)
{
  BobguiCellRendererProgress *cellprogress = BOBGUI_CELL_RENDERER_PROGRESS (object);
  BobguiCellRendererProgressPrivate *priv = bobgui_cell_renderer_progress_get_instance_private (cellprogress);

  switch (param_id)
    {
    case PROP_VALUE:
      g_value_set_int (value, priv->value);
      break;
    case PROP_TEXT:
      g_value_set_string (value, priv->text);
      break;
    case PROP_PULSE:
      g_value_set_int (value, priv->pulse);
      break;
    case PROP_TEXT_XALIGN:
      g_value_set_float (value, priv->text_xalign);
      break;
    case PROP_TEXT_YALIGN:
      g_value_set_float (value, priv->text_yalign);
      break;
    case PROP_ORIENTATION:
      g_value_set_enum (value, priv->orientation);
      break;
    case PROP_INVERTED:
      g_value_set_boolean (value, priv->inverted);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
    }
}

static void
bobgui_cell_renderer_progress_set_property (GObject      *object,
					 guint         param_id,
					 const GValue *value,
					 GParamSpec   *pspec)
{
  BobguiCellRendererProgress *cellprogress = BOBGUI_CELL_RENDERER_PROGRESS (object);
  BobguiCellRendererProgressPrivate *priv = bobgui_cell_renderer_progress_get_instance_private (cellprogress);

  switch (param_id)
    {
    case PROP_VALUE:
      bobgui_cell_renderer_progress_set_value (cellprogress,
					    g_value_get_int (value));
      break;
    case PROP_TEXT:
      bobgui_cell_renderer_progress_set_text (cellprogress,
					   g_value_get_string (value));
      break;
    case PROP_PULSE:
      bobgui_cell_renderer_progress_set_pulse (cellprogress,
					    g_value_get_int (value));
      break;
    case PROP_TEXT_XALIGN:
      priv->text_xalign = g_value_get_float (value);
      break;
    case PROP_TEXT_YALIGN:
      priv->text_yalign = g_value_get_float (value);
      break;
    case PROP_ORIENTATION:
      if (priv->orientation != g_value_get_enum (value))
        {
          priv->orientation = g_value_get_enum (value);
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    case PROP_INVERTED:
      if (priv->inverted != g_value_get_boolean (value))
        {
          priv->inverted = g_value_get_boolean (value);
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
    }
}

static void
compute_dimensions (BobguiCellRenderer *cell,
		    BobguiWidget       *widget,
		    const char      *text,
		    int             *width,
		    int             *height)
{
  PangoRectangle logical_rect;
  PangoLayout *layout;
  int xpad, ypad;

  layout = bobgui_widget_create_pango_layout (widget, text);
  pango_layout_get_pixel_extents (layout, NULL, &logical_rect);

  bobgui_cell_renderer_get_padding (cell, &xpad, &ypad);

  if (width)
    *width = logical_rect.width + xpad * 2;

  if (height)
    *height = logical_rect.height + ypad * 2;

  g_object_unref (layout);
}

static void
bobgui_cell_renderer_progress_get_preferred_width (BobguiCellRenderer *cell,
                                                BobguiWidget       *widget,
                                                int             *minimum,
                                                int             *natural)
{
  BobguiCellRendererProgress *self = BOBGUI_CELL_RENDERER_PROGRESS (cell);
  BobguiCellRendererProgressPrivate *priv = bobgui_cell_renderer_progress_get_instance_private (self);
  int w, h;
  int size;

  if (priv->min_w < 0)
    {
      char *text = g_strdup_printf (C_("progress bar label", "%d %%"), 100);
      compute_dimensions (cell, widget, text,
			  &priv->min_w,
			  &priv->min_h);
      g_free (text);
    }

  compute_dimensions (cell, widget, priv->label, &w, &h);

  size = MAX (priv->min_w, w);

  if (minimum != NULL)
    *minimum = size;
  if (natural != NULL)
    *natural = size;
}

static void
bobgui_cell_renderer_progress_get_preferred_height (BobguiCellRenderer *cell,
                                                 BobguiWidget       *widget,
                                                 int             *minimum,
                                                 int             *natural)
{
  BobguiCellRendererProgress *self = BOBGUI_CELL_RENDERER_PROGRESS (cell);
  BobguiCellRendererProgressPrivate *priv = bobgui_cell_renderer_progress_get_instance_private (self);
  int w, h;
  int size;

  if (priv->min_w < 0)
    {
      char *text = g_strdup_printf (C_("progress bar label", "%d %%"), 100);
      compute_dimensions (cell, widget, text,
			  &priv->min_w,
			  &priv->min_h);
      g_free (text);
    }

  compute_dimensions (cell, widget, priv->label, &w, &h);

  size = MIN (priv->min_h, h);

  if (minimum != NULL)
    *minimum = size;
  if (natural != NULL)
    *natural = size;
}

static inline int
get_bar_size (int pulse,
	      int value,
	      int full_size)
{
  int bar_size;

  if (pulse < 0)
    bar_size = full_size * MAX (0, value) / 100;
  else if (pulse == 0)
    bar_size = 0;
  else if (pulse == G_MAXINT)
    bar_size = full_size;
  else
    bar_size = MAX (2, full_size / 5);

  return bar_size;
}

static inline int
get_bar_position (int      start,
		  int      full_size,
		  int      bar_size,
		  int      pulse,
		  int      offset,
		  gboolean is_rtl)
{
  int position;

  if (pulse < 0 || pulse == 0 || pulse == G_MAXINT)
    {
      position = is_rtl ? (start + full_size - bar_size) : start;
    }
  else
    {
      position = (is_rtl ? offset + 12 : offset) % 24;
      if (position > 12)
	position = 24 - position;
      position = start + full_size * position / 15;
    }

  return position;
}

static void
bobgui_cell_renderer_progress_snapshot (BobguiCellRenderer      *cell,
                                     BobguiSnapshot          *snapshot,
                                     BobguiWidget            *widget,
                                     const GdkRectangle   *background_area,
                                     const GdkRectangle   *cell_area,
                                     BobguiCellRendererState  flags)
{
  BobguiCellRendererProgress *cellprogress = BOBGUI_CELL_RENDERER_PROGRESS (cell);
  BobguiCellRendererProgressPrivate *priv = bobgui_cell_renderer_progress_get_instance_private (cellprogress);
  BobguiStyleContext *context;
  BobguiBorder padding;
  PangoLayout *layout;
  PangoRectangle logical_rect;
  int x, y, w, h, x_pos, y_pos, bar_position, bar_size, start, full_size;
  int xpad, ypad;
  GdkRectangle clip;
  gboolean is_rtl;

  context = bobgui_widget_get_style_context (widget);
  is_rtl = bobgui_widget_get_direction (widget) == BOBGUI_TEXT_DIR_RTL;

  bobgui_cell_renderer_get_padding (cell, &xpad, &ypad);
  x = cell_area->x + xpad;
  y = cell_area->y + ypad;
  w = cell_area->width - xpad * 2;
  h = cell_area->height - ypad * 2;

  bobgui_style_context_save (context);
  bobgui_style_context_add_class (context, "trough");

  bobgui_snapshot_render_background (snapshot, context, x, y, w, h);
  bobgui_snapshot_render_frame (snapshot, context, x, y, w, h);

  bobgui_style_context_get_padding (context, &padding);

  x += padding.left;
  y += padding.top;
  w -= padding.left + padding.right;
  h -= padding.top + padding.bottom;

  bobgui_style_context_restore (context);

  if (priv->orientation == BOBGUI_ORIENTATION_HORIZONTAL)
    {
      clip.y = y;
      clip.height = h;

      start = x;
      full_size = w;

      bar_size = get_bar_size (priv->pulse, priv->value, full_size);

      if (!priv->inverted)
	bar_position = get_bar_position (start, full_size, bar_size,
					 priv->pulse, priv->offset, is_rtl);
      else
	bar_position = get_bar_position (start, full_size, bar_size,
					 priv->pulse, priv->offset, !is_rtl);

      clip.width = bar_size;
      clip.x = bar_position;
    }
  else
    {
      clip.x = x;
      clip.width = w;

      start = y;
      full_size = h;

      bar_size = get_bar_size (priv->pulse, priv->value, full_size);

      if (priv->inverted)
	bar_position = get_bar_position (start, full_size, bar_size,
					 priv->pulse, priv->offset, TRUE);
      else
	bar_position = get_bar_position (start, full_size, bar_size,
					 priv->pulse, priv->offset, FALSE);

      clip.height = bar_size;
      clip.y = bar_position;
    }

  if (bar_size > 0)
    {
      bobgui_style_context_save (context);
      bobgui_style_context_add_class (context, "progressbar");

      bobgui_snapshot_render_background (snapshot, context, clip.x, clip.y, clip.width, clip.height);
      bobgui_snapshot_render_frame (snapshot, context, clip.x, clip.y, clip.width, clip.height);

      bobgui_style_context_restore (context);
    }

  if (priv->label)
    {
      float text_xalign;

      layout = bobgui_widget_create_pango_layout (widget, priv->label);
      pango_layout_get_pixel_extents (layout, NULL, &logical_rect);

      if (bobgui_widget_get_direction (widget) != BOBGUI_TEXT_DIR_LTR)
	text_xalign = 1.0 - priv->text_xalign;
      else
	text_xalign = priv->text_xalign;

      x_pos = x + padding.left + text_xalign *
	(w - padding.left - padding.right - logical_rect.width);

      y_pos = y + padding.top + priv->text_yalign *
	(h - padding.top - padding.bottom - logical_rect.height);

      bobgui_snapshot_push_clip (snapshot,
                              &GRAPHENE_RECT_INIT(
                                  clip.x, clip.y,
                                  clip.width, clip.height
                              ));

      bobgui_style_context_save (context);
      bobgui_style_context_add_class (context, "progressbar");

      bobgui_snapshot_render_layout (snapshot, context,
                                  x_pos, y_pos,
                                  layout);

      bobgui_style_context_restore (context);
      bobgui_snapshot_pop (snapshot);

      bobgui_style_context_save (context);
      bobgui_style_context_add_class (context, "trough");

      if (bar_position > start)
        {
	  if (priv->orientation == BOBGUI_ORIENTATION_HORIZONTAL)
	    {
	      clip.x = x;
	      clip.width = bar_position - x;
	    }
	  else
	    {
	      clip.y = y;
	      clip.height = bar_position - y;
	    }

          bobgui_snapshot_push_clip (snapshot,
                                  &GRAPHENE_RECT_INIT(
                                      clip.x, clip.y,
                                      clip.width, clip.height
                                  ));

          bobgui_snapshot_render_layout (snapshot, context,
                                      x_pos, y_pos,
                                      layout);

          bobgui_snapshot_pop (snapshot);
        }

      if (bar_position + bar_size < start + full_size)
        {
	  if (priv->orientation == BOBGUI_ORIENTATION_HORIZONTAL)
	    {
	      clip.x = bar_position + bar_size;
	      clip.width = x + w - (bar_position + bar_size);
	    }
	  else
	    {
	      clip.y = bar_position + bar_size;
	      clip.height = y + h - (bar_position + bar_size);
	    }

          bobgui_snapshot_push_clip (snapshot,
                                  &GRAPHENE_RECT_INIT(
                                      clip.x, clip.y,
                                      clip.width, clip.height
                                  ));

          bobgui_snapshot_render_layout (snapshot, context,
                                      x_pos, y_pos,
                                      layout);

          bobgui_snapshot_pop (snapshot);
        }

      bobgui_style_context_restore (context);
      g_object_unref (layout);
    }
}

static void
bobgui_cell_renderer_progress_class_init (BobguiCellRendererProgressClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  BobguiCellRendererClass *cell_class = BOBGUI_CELL_RENDERER_CLASS (klass);

  object_class->finalize = bobgui_cell_renderer_progress_finalize;
  object_class->get_property = bobgui_cell_renderer_progress_get_property;
  object_class->set_property = bobgui_cell_renderer_progress_set_property;

  cell_class->get_preferred_width = bobgui_cell_renderer_progress_get_preferred_width;
  cell_class->get_preferred_height = bobgui_cell_renderer_progress_get_preferred_height;
  cell_class->snapshot = bobgui_cell_renderer_progress_snapshot;

  /**
   * BobguiCellRendererProgress:value:
   *
   * The "value" property determines the percentage to which the
   * progress bar will be "filled in".
   **/
  g_object_class_install_property (object_class,
				   PROP_VALUE,
				   g_param_spec_int ("value", NULL, NULL,
						     0, 100, 0,
						     BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * BobguiCellRendererProgress:text:
   *
   * The "text" property determines the label which will be drawn
   * over the progress bar. Setting this property to %NULL causes the default
   * label to be displayed. Setting this property to an empty string causes
   * no label to be displayed.
   **/
  g_object_class_install_property (object_class,
				   PROP_TEXT,
				   g_param_spec_string ("text", NULL, NULL,
							NULL,
							BOBGUI_PARAM_READWRITE));

  /**
   * BobguiCellRendererProgress:pulse:
   *
   * Setting this to a non-negative value causes the cell renderer to
   * enter "activity mode", where a block bounces back and forth to
   * indicate that some progress is made, without specifying exactly how
   * much.
   *
   * Each increment of the property causes the block to move by a little
   * bit.
   *
   * To indicate that the activity has not started yet, set the property
   * to zero. To indicate completion, set the property to %G_MAXINT.
   */
  g_object_class_install_property (object_class,
                                   PROP_PULSE,
                                   g_param_spec_int ("pulse", NULL, NULL,
                                                     -1, G_MAXINT, -1,
                                                     BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * BobguiCellRendererProgress:text-xalign:
   *
   * The "text-xalign" property controls the horizontal alignment of the
   * text in the progress bar.  Valid values range from 0 (left) to 1
   * (right).  Reserved for RTL layouts.
   */
  g_object_class_install_property (object_class,
                                   PROP_TEXT_XALIGN,
                                   g_param_spec_float ("text-xalign", NULL, NULL,
                                                       0.0, 1.0, 0.5,
                                                       BOBGUI_PARAM_READWRITE));

  /**
   * BobguiCellRendererProgress:text-yalign:
   *
   * The "text-yalign" property controls the vertical alignment of the
   * text in the progress bar.  Valid values range from 0 (top) to 1
   * (bottom).
   */
  g_object_class_install_property (object_class,
                                   PROP_TEXT_YALIGN,
                                   g_param_spec_float ("text-yalign", NULL, NULL,
                                                       0.0, 1.0, 0.5,
                                                       BOBGUI_PARAM_READWRITE));

  g_object_class_override_property (object_class,
                                    PROP_ORIENTATION,
                                    "orientation");

  /**
   * BobguiCellRendererProgress:inverted:
   *
   * Whether progess is inverted.
   */
  g_object_class_install_property (object_class,
                                   PROP_INVERTED,
                                   g_param_spec_boolean ("inverted", NULL, NULL,
                                                         FALSE,
                                                         BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));
}

static void
bobgui_cell_renderer_progress_init (BobguiCellRendererProgress *cellprogress)
{
  BobguiCellRendererProgressPrivate *priv = bobgui_cell_renderer_progress_get_instance_private (cellprogress);

  priv->value = 0;
  priv->text = NULL;
  priv->label = NULL;
  priv->min_w = -1;
  priv->min_h = -1;
  priv->pulse = -1;
  priv->offset = 0;

  priv->text_xalign = 0.5;
  priv->text_yalign = 0.5;

  priv->orientation = BOBGUI_ORIENTATION_HORIZONTAL,
  priv->inverted = FALSE;
}

/**
 * bobgui_cell_renderer_progress_new:
 *
 * Creates a new `BobguiCellRendererProgress`.
 *
 * Returns: the new cell renderer
 *
 * Deprecated: 4.10
 **/
BobguiCellRenderer*
bobgui_cell_renderer_progress_new (void)
{
  return g_object_new (BOBGUI_TYPE_CELL_RENDERER_PROGRESS, NULL);
}
