/* BOBGUI - The Bobgui Framework
 * bobguiprintcontext.c: Print Context
 * Copyright (C) 2006, Red Hat, Inc.
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

#include "config.h"
#include "bobguiprintoperation-private.h"


/**
 * BobguiPrintContext:
 *
 * Encapsulates context information that is required when
 * drawing pages for printing.
 *
 * This includes the cairo context and important parameters like page size
 * and resolution. It also lets you easily create [class@Pango.Layout] and
 * [class@Pango.Context] objects that match the font metrics of the cairo surface.
 *
 * `BobguiPrintContext` objects get passed to the
 * [signal@Bobgui.PrintOperation::begin-print],
 * [signal@Bobgui.PrintOperation::end-print],
 * [signal@Bobgui.PrintOperation::request-page-setup] and
 * [signal@Bobgui.PrintOperation::draw-page] signals on the
 * [class@Bobgui.PrintOperation] object.
 *
 * ## Using BobguiPrintContext in a ::draw-page callback
 *
 * ```c
 * static void
 * draw_page (BobguiPrintOperation *operation,
 *            BobguiPrintContext   *context,
 *            int                page_nr)
 * {
 *   cairo_t *cr;
 *   PangoLayout *layout;
 *   PangoFontDescription *desc;
 *
 *   cr = bobgui_print_context_get_cairo_context (context);
 *
 *   // Draw a red rectangle, as wide as the paper (inside the margins)
 *   cairo_set_source_rgb (cr, 1.0, 0, 0);
 *   cairo_rectangle (cr, 0, 0, bobgui_print_context_get_width (context), 50);
 *
 *   cairo_fill (cr);
 *
 *   // Draw some lines
 *   cairo_move_to (cr, 20, 10);
 *   cairo_line_to (cr, 40, 20);
 *   cairo_arc (cr, 60, 60, 20, 0, M_PI);
 *   cairo_line_to (cr, 80, 20);
 *
 *   cairo_set_source_rgb (cr, 0, 0, 0);
 *   cairo_set_line_width (cr, 5);
 *   cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);
 *   cairo_set_line_join (cr, CAIRO_LINE_JOIN_ROUND);
 *
 *   cairo_stroke (cr);
 *
 *   // Draw some text
 *   layout = bobgui_print_context_create_pango_layout (context);
 *   pango_layout_set_text (layout, "Hello World! Printing is easy", -1);
 *   desc = pango_font_description_from_string ("sans 28");
 *   pango_layout_set_font_description (layout, desc);
 *   pango_font_description_free (desc);
 *
 *   cairo_move_to (cr, 30, 20);
 *   pango_cairo_layout_path (cr, layout);
 *
 *   // Font Outline
 *   cairo_set_source_rgb (cr, 0.93, 1.0, 0.47);
 *   cairo_set_line_width (cr, 0.5);
 *   cairo_stroke_preserve (cr);
 *
 *   // Font Fill
 *   cairo_set_source_rgb (cr, 0, 0.0, 1.0);
 *   cairo_fill (cr);
 *
 *   g_object_unref (layout);
 * }
 * ```
 */


typedef struct _BobguiPrintContextClass BobguiPrintContextClass;

#define BOBGUI_IS_PRINT_CONTEXT_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), BOBGUI_TYPE_PRINT_CONTEXT))
#define BOBGUI_PRINT_CONTEXT_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BOBGUI_TYPE_PRINT_CONTEXT, BobguiPrintContextClass))
#define BOBGUI_PRINT_CONTEXT_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_PRINT_CONTEXT, BobguiPrintContextClass))

#define MM_PER_INCH 25.4
#define POINTS_PER_INCH 72

struct _BobguiPrintContext
{
  GObject parent_instance;

  BobguiPrintOperation *op;
  cairo_t *cr;
  BobguiPageSetup *page_setup;

  double surface_dpi_x;
  double surface_dpi_y;
  
  double pixels_per_unit_x;
  double pixels_per_unit_y;

  gboolean has_hard_margins;
  double hard_margin_top;
  double hard_margin_bottom;
  double hard_margin_left;
  double hard_margin_right;

};

struct _BobguiPrintContextClass
{
  GObjectClass parent_class;
};

G_DEFINE_TYPE (BobguiPrintContext, bobgui_print_context, G_TYPE_OBJECT)

static void
bobgui_print_context_finalize (GObject *object)
{
  BobguiPrintContext *context = BOBGUI_PRINT_CONTEXT (object);

  if (context->page_setup)
    g_object_unref (context->page_setup);

  if (context->cr)
    cairo_destroy (context->cr);
  
  G_OBJECT_CLASS (bobgui_print_context_parent_class)->finalize (object);
}

static void
bobgui_print_context_init (BobguiPrintContext *context)
{
}

static void
bobgui_print_context_class_init (BobguiPrintContextClass *class)
{
  GObjectClass *gobject_class = (GObjectClass *)class;

  gobject_class->finalize = bobgui_print_context_finalize;
}


BobguiPrintContext *
_bobgui_print_context_new (BobguiPrintOperation *op)
{
  BobguiPrintContext *context;

  context = g_object_new (BOBGUI_TYPE_PRINT_CONTEXT, NULL);

  context->op = op;
  context->cr = NULL;
  context->has_hard_margins = FALSE;
  
  return context;
}

static PangoFontMap *
_bobgui_print_context_get_fontmap (BobguiPrintContext *context)
{
  return pango_cairo_font_map_get_default ();
}

/**
 * bobgui_print_context_set_cairo_context:
 * @context: a `BobguiPrintContext`
 * @cr: the cairo context
 * @dpi_x: the horizontal resolution to use with @cr
 * @dpi_y: the vertical resolution to use with @cr
 *
 * Sets a new cairo context on a print context.
 *
 * This function is intended to be used when implementing
 * an internal print preview, it is not needed for printing,
 * since BOBGUI itself creates a suitable cairo context in that
 * case.
 */
void
bobgui_print_context_set_cairo_context (BobguiPrintContext *context,
				     cairo_t         *cr,
				     double           dpi_x,
				     double           dpi_y)
{
  if (context->cr)
    cairo_destroy (context->cr);

  context->cr = cairo_reference (cr);
  context->surface_dpi_x = dpi_x;
  context->surface_dpi_y = dpi_y;

  switch (context->op->priv->unit)
    {
    default:
    case BOBGUI_UNIT_NONE:
      /* Do nothing, this is the cairo default unit */
      context->pixels_per_unit_x = 1.0;
      context->pixels_per_unit_y = 1.0;
      break;
    case BOBGUI_UNIT_POINTS:
      context->pixels_per_unit_x = dpi_x / POINTS_PER_INCH;
      context->pixels_per_unit_y = dpi_y / POINTS_PER_INCH;
      break;
    case BOBGUI_UNIT_INCH:
      context->pixels_per_unit_x = dpi_x;
      context->pixels_per_unit_y = dpi_y;
      break;
    case BOBGUI_UNIT_MM:
      context->pixels_per_unit_x = dpi_x / MM_PER_INCH;
      context->pixels_per_unit_y = dpi_y / MM_PER_INCH;
      break;
    }
  cairo_scale (context->cr,
	       context->pixels_per_unit_x,
	       context->pixels_per_unit_y);
}


void
_bobgui_print_context_rotate_according_to_orientation (BobguiPrintContext *context)
{
  cairo_t *cr = context->cr;
  cairo_matrix_t matrix;
  BobguiPaperSize *paper_size;
  double width, height;

  paper_size = bobgui_page_setup_get_paper_size (context->page_setup);

  width = bobgui_paper_size_get_width (paper_size, BOBGUI_UNIT_INCH);
  width = width * context->surface_dpi_x / context->pixels_per_unit_x;
  height = bobgui_paper_size_get_height (paper_size, BOBGUI_UNIT_INCH);
  height = height * context->surface_dpi_y / context->pixels_per_unit_y;
  
  switch (bobgui_page_setup_get_orientation (context->page_setup))
    {
    default:
    case BOBGUI_PAGE_ORIENTATION_PORTRAIT:
      break;
    case BOBGUI_PAGE_ORIENTATION_LANDSCAPE:
      cairo_translate (cr, 0, height);
      cairo_matrix_init (&matrix,
			 0, -1,
			 1,  0,
			 0,  0);
      cairo_transform (cr, &matrix);
      break;
    case BOBGUI_PAGE_ORIENTATION_REVERSE_PORTRAIT:
      cairo_translate (cr, width, height);
      cairo_matrix_init (&matrix,
			 -1,  0,
			  0, -1,
			  0,  0);
      cairo_transform (cr, &matrix);
      break;
    case BOBGUI_PAGE_ORIENTATION_REVERSE_LANDSCAPE:
      cairo_translate (cr, width, 0);
      cairo_matrix_init (&matrix,
			  0,  1,
			 -1,  0,
			  0,  0);
      cairo_transform (cr, &matrix);
      break;
    }
}

void
_bobgui_print_context_reverse_according_to_orientation (BobguiPrintContext *context)
{
  cairo_t *cr = context->cr;
  cairo_matrix_t matrix;
  double width, height;

  width = bobgui_page_setup_get_paper_width (context->page_setup, BOBGUI_UNIT_INCH);
  width = width * context->surface_dpi_x / context->pixels_per_unit_x;
  height = bobgui_page_setup_get_paper_height (context->page_setup, BOBGUI_UNIT_INCH);
  height = height * context->surface_dpi_y / context->pixels_per_unit_y;

  switch (bobgui_page_setup_get_orientation (context->page_setup))
    {
    default:
    case BOBGUI_PAGE_ORIENTATION_PORTRAIT:
    case BOBGUI_PAGE_ORIENTATION_LANDSCAPE:
      break;
    case BOBGUI_PAGE_ORIENTATION_REVERSE_PORTRAIT:
    case BOBGUI_PAGE_ORIENTATION_REVERSE_LANDSCAPE:
      cairo_translate (cr, width, height);
      cairo_matrix_init (&matrix,
			 -1,  0,
			  0, -1,
			  0,  0);
      cairo_transform (cr, &matrix);
      break;
    }
}

void
_bobgui_print_context_translate_into_margin (BobguiPrintContext *context)
{
  double dx, dy;

  g_return_if_fail (BOBGUI_IS_PRINT_CONTEXT (context));

  /* We do it this way to also handle BOBGUI_UNIT_NONE */
  switch (bobgui_page_setup_get_orientation (context->page_setup))
    {
      default:
      case BOBGUI_PAGE_ORIENTATION_PORTRAIT:
        dx = bobgui_page_setup_get_left_margin (context->page_setup, BOBGUI_UNIT_INCH);
        dy = bobgui_page_setup_get_top_margin (context->page_setup, BOBGUI_UNIT_INCH);
        break;
      case BOBGUI_PAGE_ORIENTATION_LANDSCAPE:
        dx = bobgui_page_setup_get_bottom_margin (context->page_setup, BOBGUI_UNIT_INCH);
        dy = bobgui_page_setup_get_left_margin (context->page_setup, BOBGUI_UNIT_INCH);
        break;
      case BOBGUI_PAGE_ORIENTATION_REVERSE_PORTRAIT:
        dx = bobgui_page_setup_get_right_margin (context->page_setup, BOBGUI_UNIT_INCH);
        dy = bobgui_page_setup_get_bottom_margin (context->page_setup, BOBGUI_UNIT_INCH);
        break;
      case BOBGUI_PAGE_ORIENTATION_REVERSE_LANDSCAPE:
        dx = bobgui_page_setup_get_top_margin (context->page_setup, BOBGUI_UNIT_INCH);
        dy = bobgui_page_setup_get_right_margin (context->page_setup, BOBGUI_UNIT_INCH);
        break;
    }

  cairo_translate (context->cr,
                   dx * context->surface_dpi_x / context->pixels_per_unit_x,
                   dy * context->surface_dpi_y / context->pixels_per_unit_y);
}

void
_bobgui_print_context_set_page_setup (BobguiPrintContext *context,
				   BobguiPageSetup    *page_setup)
{
  g_return_if_fail (BOBGUI_IS_PRINT_CONTEXT (context));
  g_return_if_fail (page_setup == NULL ||
		    BOBGUI_IS_PAGE_SETUP (page_setup));

  if (page_setup != NULL)
    g_object_ref (page_setup);

  if (context->page_setup != NULL)
    g_object_unref (context->page_setup);

  context->page_setup = page_setup;
}

/**
 * bobgui_print_context_get_cairo_context:
 * @context: a `BobguiPrintContext`
 *
 * Obtains the cairo context that is associated with the
 * `BobguiPrintContext`.
 *
 * Returns: (transfer none): the cairo context of @context
 */
cairo_t *
bobgui_print_context_get_cairo_context (BobguiPrintContext *context)
{
  g_return_val_if_fail (BOBGUI_IS_PRINT_CONTEXT (context), NULL);

  return context->cr;
}

/**
 * bobgui_print_context_get_page_setup:
 * @context: a `BobguiPrintContext`
 *
 * Obtains the `BobguiPageSetup` that determines the page
 * dimensions of the `BobguiPrintContext`.
 *
 * Returns: (transfer none): the page setup of @context
 */
BobguiPageSetup *
bobgui_print_context_get_page_setup (BobguiPrintContext *context)
{
  g_return_val_if_fail (BOBGUI_IS_PRINT_CONTEXT (context), NULL);

  return context->page_setup;
}

/**
 * bobgui_print_context_get_width:
 * @context: a `BobguiPrintContext`
 *
 * Obtains the width of the `BobguiPrintContext`, in pixels.
 *
 * Returns: the width of @context
 */
double
bobgui_print_context_get_width (BobguiPrintContext *context)
{
  BobguiPrintOperationPrivate *priv;
  double width;

  g_return_val_if_fail (BOBGUI_IS_PRINT_CONTEXT (context), 0);

  priv = context->op->priv;

  if (priv->use_full_page)
    width = bobgui_page_setup_get_paper_width (context->page_setup, BOBGUI_UNIT_INCH);
  else
    width = bobgui_page_setup_get_page_width (context->page_setup, BOBGUI_UNIT_INCH);

  /* Really dpi_x? What about landscape? what does dpi_x mean in that case? */
  return width * context->surface_dpi_x / context->pixels_per_unit_x;
}

/**
 * bobgui_print_context_get_height:
 * @context: a `BobguiPrintContext`
 *
 * Obtains the height of the `BobguiPrintContext`, in pixels.
 *
 * Returns: the height of @context
 */
double
bobgui_print_context_get_height (BobguiPrintContext *context)
{
  BobguiPrintOperationPrivate *priv;
  double height;

  g_return_val_if_fail (BOBGUI_IS_PRINT_CONTEXT (context), 0);

  priv = context->op->priv;

  if (priv->use_full_page)
    height = bobgui_page_setup_get_paper_height (context->page_setup, BOBGUI_UNIT_INCH);
  else
    height = bobgui_page_setup_get_page_height (context->page_setup, BOBGUI_UNIT_INCH);

  /* Really dpi_y? What about landscape? what does dpi_y mean in that case? */
  return height * context->surface_dpi_y / context->pixels_per_unit_y;
}

/**
 * bobgui_print_context_get_dpi_x:
 * @context: a `BobguiPrintContext`
 *
 * Obtains the horizontal resolution of the `BobguiPrintContext`,
 * in dots per inch.
 *
 * Returns: the horizontal resolution of @context
 */
double
bobgui_print_context_get_dpi_x (BobguiPrintContext *context)
{
  g_return_val_if_fail (BOBGUI_IS_PRINT_CONTEXT (context), 0);

  return context->surface_dpi_x;
}

/**
 * bobgui_print_context_get_dpi_y:
 * @context: a `BobguiPrintContext`
 *
 * Obtains the vertical resolution of the `BobguiPrintContext`,
 * in dots per inch.
 *
 * Returns: the vertical resolution of @context
 */
double
bobgui_print_context_get_dpi_y (BobguiPrintContext *context)
{
  g_return_val_if_fail (BOBGUI_IS_PRINT_CONTEXT (context), 0);

  return context->surface_dpi_y;
}

/**
 * bobgui_print_context_get_hard_margins:
 * @context: a `BobguiPrintContext`
 * @top: (out): top hardware printer margin
 * @bottom: (out): bottom hardware printer margin
 * @left: (out): left hardware printer margin
 * @right: (out): right hardware printer margin
 *
 * Obtains the hardware printer margins of the `BobguiPrintContext`,
 * in units.
 *
 * Returns: %TRUE if the hard margins were retrieved
 */
gboolean
bobgui_print_context_get_hard_margins (BobguiPrintContext *context,
				    double          *top,
				    double          *bottom,
				    double          *left,
				    double          *right)
{
  if (context->has_hard_margins)
    {
      *top    = context->hard_margin_top / context->pixels_per_unit_y;
      *bottom = context->hard_margin_bottom / context->pixels_per_unit_y;
      *left   = context->hard_margin_left / context->pixels_per_unit_x;
      *right  = context->hard_margin_right / context->pixels_per_unit_x;
    }

  return context->has_hard_margins;
}

/**
 * bobgui_print_context_set_hard_margins:
 * @context: a `BobguiPrintContext`
 * @top: top hardware printer margin
 * @bottom: bottom hardware printer margin
 * @left: left hardware printer margin
 * @right: right hardware printer margin
 *
 * Sets the hard margins in pixels.
 */
void
_bobgui_print_context_set_hard_margins (BobguiPrintContext *context,
				     double           top,
				     double           bottom,
				     double           left,
				     double           right)
{
  context->hard_margin_top    = top;
  context->hard_margin_bottom = bottom;
  context->hard_margin_left   = left;
  context->hard_margin_right  = right;
  context->has_hard_margins   = TRUE;
}

/**
 * bobgui_print_context_get_pango_fontmap:
 * @context: a `BobguiPrintContext`
 *
 * Returns a `PangoFontMap` that is suitable for use
 * with the `BobguiPrintContext`.
 *
 * Returns: (transfer none): the font map of @context
 */
PangoFontMap *
bobgui_print_context_get_pango_fontmap (BobguiPrintContext *context)
{
  g_return_val_if_fail (BOBGUI_IS_PRINT_CONTEXT (context), NULL);

  return _bobgui_print_context_get_fontmap (context);
}

/**
 * bobgui_print_context_create_pango_context:
 * @context: a `BobguiPrintContext`
 *
 * Creates a new `PangoContext` that can be used with the
 * `BobguiPrintContext`.
 *
 * Returns: (transfer full): a new Pango context for @context
 */
PangoContext *
bobgui_print_context_create_pango_context (BobguiPrintContext *context)
{
  PangoContext *pango_context;
  cairo_font_options_t *options;

  g_return_val_if_fail (BOBGUI_IS_PRINT_CONTEXT (context), NULL);
  
  pango_context = pango_font_map_create_context (_bobgui_print_context_get_fontmap (context));

  options = cairo_font_options_create ();
  cairo_font_options_set_hint_metrics (options, CAIRO_HINT_METRICS_OFF);
  pango_cairo_context_set_font_options (pango_context, options);
  cairo_font_options_destroy (options);
  
  /* We use the unit-scaled resolution, as we still want 
   * fonts given in points to work 
   */
  pango_cairo_context_set_resolution (pango_context,
				      context->surface_dpi_y / context->pixels_per_unit_y);
  return pango_context;
}

/**
 * bobgui_print_context_create_pango_layout:
 * @context: a `BobguiPrintContext`
 *
 * Creates a new `PangoLayout` that is suitable for use
 * with the `BobguiPrintContext`.
 *
 * Returns: (transfer full): a new Pango layout for @context
 */
PangoLayout *
bobgui_print_context_create_pango_layout (BobguiPrintContext *context)
{
  PangoContext *pango_context;
  PangoLayout *layout;

  g_return_val_if_fail (BOBGUI_IS_PRINT_CONTEXT (context), NULL);

  pango_context = bobgui_print_context_create_pango_context (context);
  layout = pango_layout_new (pango_context);

  pango_cairo_update_context (context->cr, pango_context);
  g_object_unref (pango_context);

  return layout;
}
