/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2016 Benjamin Otte <otte@gnome.org>
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

#include "bobguicssnumbervalueprivate.h"

#include "bobguicsscalcvalueprivate.h"
#include "bobguicssenumvalueprivate.h"
#include "bobguicssdimensionvalueprivate.h"
#include "bobguicsscolorvalueprivate.h"
#include "bobguicssstyleprivate.h"
#include "bobguiprivate.h"

#include <fenv.h>

#define RAD_TO_DEG(x) ((x) * 180.0 / G_PI)
#define DEG_TO_RAD(x) ((x) * G_PI / 180.0)

static BobguiCssValue * bobgui_css_calc_value_alloc           (guint        n_terms);
static BobguiCssValue * bobgui_css_calc_value_new_sum         (BobguiCssValue *a,
                                                         BobguiCssValue *b);
static BobguiCssValue * bobgui_css_round_value_new            (guint        mode,
                                                         BobguiCssValue *a,
                                                         BobguiCssValue *b);
static BobguiCssValue * bobgui_css_clamp_value_new            (BobguiCssValue *min,
                                                         BobguiCssValue *center,
                                                         BobguiCssValue *max);

static double _round (guint mode, double a, double b);
static double _mod (double a, double b);
static double _rem (double a, double b);
static double _sign (double a);

typedef enum {
  TYPE_CALC,
  TYPE_DIMENSION,
  TYPE_MIN,
  TYPE_MAX,
  TYPE_CLAMP,
  TYPE_ROUND,
  TYPE_MOD,
  TYPE_REM,
  TYPE_PRODUCT,
  TYPE_ABS,
  TYPE_SIGN,
  TYPE_SIN,
  TYPE_COS,
  TYPE_TAN,
  TYPE_ASIN,
  TYPE_ACOS,
  TYPE_ATAN,
  TYPE_ATAN2,
  TYPE_POW,
  TYPE_SQRT,
  TYPE_EXP,
  TYPE_LOG,
  TYPE_HYPOT,
  TYPE_COLOR_COORD,
} NumberValueType;

static const char *function_name[] = {
    "calc",
    "", /* TYPE_DIMENSION */
    "min",
    "max",
    "clamp",
    "round",
    "mod",
    "rem",
    "", /* TYPE_PRODUCT */
    "abs",
    "sign",
    "sin",
    "cos",
    "tan",
    "asin",
    "acos",
    "atan",
    "atan2",
    "pow",
    "sqrt",
    "exp",
    "log",
    "hypot",
  };

struct _BobguiCssValue {
  BOBGUI_CSS_VALUE_BASE
  guint type : 16;
  union {
    struct {
      BobguiCssUnit unit;
      double value;
    } dimension;
    struct {
      guint mode;
      guint n_terms;
      BobguiCssValue *terms[1];
    } calc;
    struct {
      BobguiCssValue *color;
      BobguiCssColorSpace color_space;
      guint coord            : 16;
      guint legacy_rgb_scale :  1;
    } color_coord;
  };
};

static BobguiCssValue *
bobgui_css_calc_value_new (guint         type,
                        guint         mode,
                        BobguiCssValue **values,
                        guint         n_values)
{
  BobguiCssValue *result;
  gboolean computed;
  gboolean contains_current_color;

  if (n_values == 1 &&
      (type == TYPE_CALC || type == TYPE_PRODUCT ||
       type == TYPE_MIN || type == TYPE_MAX))
    {
      return values[0];
    }

  result = bobgui_css_calc_value_alloc (n_values);
  result->type = type;
  result->calc.mode = mode;

  memcpy (result->calc.terms, values, n_values * sizeof (BobguiCssValue *));

  computed = TRUE;
  contains_current_color = FALSE;
  for (int i = 0; i < n_values; i++)
    {
      computed &= bobgui_css_value_is_computed (values[i]);
      contains_current_color |= bobgui_css_value_contains_current_color (values[i]);
    }

  result->is_computed = computed;
  result->contains_current_color = contains_current_color;

  return result;
}

static void
bobgui_css_value_number_free (BobguiCssValue *number)
{
  if (number->type == TYPE_COLOR_COORD)
    {
      bobgui_css_value_unref (number->color_coord.color);
    }
  else if (number->type != TYPE_DIMENSION)
    {
      for (guint i = 0; i < number->calc.n_terms; i++)
        {
          if (number->calc.terms[i])
            bobgui_css_value_unref (number->calc.terms[i]);
        }
    }

  g_free (number);
}

static double
get_dpi (BobguiCssStyle *style)
{
  return bobgui_css_number_value_get (style->core->dpi, 96);
}

static double
get_base_font_size_px (guint             property_id,
                       BobguiStyleProvider *provider,
                       BobguiCssStyle      *style,
                       BobguiCssStyle      *parent_style)
{
  if (property_id == BOBGUI_CSS_PROPERTY_FONT_SIZE)
    {
      if (parent_style)
        return bobgui_css_number_value_get (parent_style->core->font_size, 100);
      else
        return bobgui_css_font_size_get_default_px (provider, style);
    }

  return bobgui_css_number_value_get (style->core->font_size, 100);
}

/* Canonical units that can be used before compute time
 *
 * Our compatibility is a bit stricter than CSS, since we
 * have a dpi property, so PX and the dpi-dependent units
 * can't be unified before compute time.
 */
static BobguiCssUnit
canonical_unit (BobguiCssUnit unit)
{
  switch (unit)
    {
    case BOBGUI_CSS_NUMBER:  return BOBGUI_CSS_NUMBER;
    case BOBGUI_CSS_PERCENT: return BOBGUI_CSS_PERCENT;
    case BOBGUI_CSS_PX:      return BOBGUI_CSS_PX;
    case BOBGUI_CSS_EM:      return BOBGUI_CSS_EM;
    case BOBGUI_CSS_EX:      return BOBGUI_CSS_EM;
    case BOBGUI_CSS_REM:     return BOBGUI_CSS_REM;
    case BOBGUI_CSS_PT:      return BOBGUI_CSS_MM;
    case BOBGUI_CSS_PC:      return BOBGUI_CSS_MM;
    case BOBGUI_CSS_IN:      return BOBGUI_CSS_MM;
    case BOBGUI_CSS_CM:      return BOBGUI_CSS_MM;
    case BOBGUI_CSS_MM:      return BOBGUI_CSS_MM;
    case BOBGUI_CSS_RAD:     return BOBGUI_CSS_DEG;
    case BOBGUI_CSS_DEG:     return BOBGUI_CSS_DEG;
    case BOBGUI_CSS_GRAD:    return BOBGUI_CSS_DEG;
    case BOBGUI_CSS_TURN:    return BOBGUI_CSS_DEG;
    case BOBGUI_CSS_S:       return BOBGUI_CSS_S;
    case BOBGUI_CSS_MS:      return BOBGUI_CSS_S;
    default:              g_assert_not_reached ();
    }
}

static inline gboolean
unit_is_compute_time (BobguiCssUnit unit)
{
  return bobgui_css_unit_get_dimension (unit) == BOBGUI_CSS_DIMENSION_LENGTH &&
         unit != BOBGUI_CSS_PX;
}

static inline gboolean
value_is_compute_time (BobguiCssValue *value)
{
  if (value->type == TYPE_DIMENSION)
    return unit_is_compute_time (value->dimension.unit);

  return FALSE;
}

/* Units are compatible if they have the same compute time
 * dependency. This still allows some operations to applied
 * early.
 */
static gboolean
units_compatible (BobguiCssValue *v1,
                  BobguiCssValue *v2)
{
  if ((v1 && v1->type != TYPE_DIMENSION) || (v2 && v2->type != TYPE_DIMENSION))
    return FALSE;

  if (v1 && v2)
    return canonical_unit (v1->dimension.unit) == canonical_unit (v2->dimension.unit);

  return TRUE;
}

/* Assumes that value is a dimension value and
 * unit is canonical and compatible its unit.
 */
static double
get_converted_value (BobguiCssValue *value,
                     BobguiCssUnit   unit)
{
  double v;

  if (value->type != TYPE_DIMENSION)
    return NAN;

  v = bobgui_css_number_value_get (value, 100);

  if (unit == value->dimension.unit)
    {
      return v;
    }
  else if (unit == BOBGUI_CSS_MM)
    {
      switch ((int) value->dimension.unit)
        {
        case BOBGUI_CSS_PT: return v * 0.35277778;
        case BOBGUI_CSS_PC: return v * 4.2333333;
        case BOBGUI_CSS_IN: return v * 25.4;
        case BOBGUI_CSS_CM: return v * 10;
        default: return NAN;
        }
    }
  else if (unit == BOBGUI_CSS_EM)
    {
      switch ((int) value->dimension.unit)
        {
        case BOBGUI_CSS_EX: return v * 0.5;
        default: return NAN;
        }
    }
  else if (unit == BOBGUI_CSS_DEG)
    {
      switch ((int) value->dimension.unit)
        {
        case BOBGUI_CSS_RAD: return v * 180.0 / G_PI;
        case BOBGUI_CSS_GRAD: return v * 360.0 / 400.0;
        case BOBGUI_CSS_TURN: return v * 360.0;
        default: return NAN;
        }
    }
  else if (unit == BOBGUI_CSS_S)
    {
      switch ((int) value->dimension.unit)
        {
        case BOBGUI_CSS_MS: return v / 1000.0;
        default: return NAN;
        }
    }
  else
    {
      return NAN;
    }
}

static BobguiCssValue *
bobgui_css_value_number_compute (BobguiCssValue          *number,
                              guint                 property_id,
                              BobguiCssComputeContext *context)
{
  BobguiStyleProvider *provider = context->provider;
  BobguiCssStyle *style = context->style;
  BobguiCssStyle *parent_style = context->parent_style;

  if (number->type == TYPE_COLOR_COORD)
    {
      BobguiCssValue *color, *result;;

      color = bobgui_css_value_compute (number->color_coord.color, property_id, context);
      result = bobgui_css_number_value_new_color_component (color,
                                                         number->color_coord.color_space,
                                                         number->color_coord.legacy_rgb_scale,
                                                         number->color_coord.coord);
      bobgui_css_value_unref (color);
      return result;
    }
  else if (number->type != TYPE_DIMENSION)
    {
      const guint n_terms = number->calc.n_terms;
      BobguiCssValue *result;
      BobguiCssValue **new_values;

      new_values = g_alloca (sizeof (BobguiCssValue *) * n_terms);

      for (gsize i = 0; i < n_terms; i++)
        new_values[i] = bobgui_css_value_compute (number->calc.terms[i], property_id, context);

      result = bobgui_css_math_value_new (number->type, number->calc.mode, new_values, n_terms);
      result->is_computed = TRUE;

      return result;
    }
  else
    {
      double value = number->dimension.value;

      switch (number->dimension.unit)
        {
        case BOBGUI_CSS_PERCENT:
          /* percentages for font sizes are computed, other percentages aren't */
          if (property_id == BOBGUI_CSS_PROPERTY_FONT_SIZE)
            {
              return bobgui_css_dimension_value_new (value / 100.0 *
                                                  get_base_font_size_px (property_id, provider, style, parent_style),
                                                  BOBGUI_CSS_PX);
            }
          G_GNUC_FALLTHROUGH;
        case BOBGUI_CSS_NUMBER:
        case BOBGUI_CSS_PX:
        case BOBGUI_CSS_DEG:
        case BOBGUI_CSS_S:
          return bobgui_css_dimension_value_new (value, number->dimension.unit);
        case BOBGUI_CSS_PT:
          return bobgui_css_dimension_value_new (value * get_dpi (style) / 72.0, BOBGUI_CSS_PX);
        case BOBGUI_CSS_PC:
          return bobgui_css_dimension_value_new (value * get_dpi (style) / 72.0 * 12.0, BOBGUI_CSS_PX);
        case BOBGUI_CSS_IN:
          return bobgui_css_dimension_value_new (value * get_dpi (style), BOBGUI_CSS_PX);
        case BOBGUI_CSS_CM:
          return bobgui_css_dimension_value_new (value * get_dpi (style) * 0.39370078740157477, BOBGUI_CSS_PX);
        case BOBGUI_CSS_MM:
          return bobgui_css_dimension_value_new (value * get_dpi (style) * 0.039370078740157477, BOBGUI_CSS_PX);
        case BOBGUI_CSS_EM:
          return bobgui_css_dimension_value_new (value *
                                              get_base_font_size_px (property_id, provider, style, parent_style),
                                              BOBGUI_CSS_PX);
        case BOBGUI_CSS_EX:
          /* for now we pretend ex is half of em */
          return bobgui_css_dimension_value_new (value * 0.5 *
                                              get_base_font_size_px (property_id, provider, style, parent_style),
                                              BOBGUI_CSS_PX);
        case BOBGUI_CSS_REM:
          return bobgui_css_dimension_value_new (value *
                                              bobgui_css_font_size_get_default_px (provider, style),
                                              BOBGUI_CSS_PX);
        case BOBGUI_CSS_RAD:
          return bobgui_css_dimension_value_new (value * 360.0 / (2 * G_PI), BOBGUI_CSS_DEG);
        case BOBGUI_CSS_GRAD:
          return bobgui_css_dimension_value_new (value * 360.0 / 400.0, BOBGUI_CSS_DEG);
        case BOBGUI_CSS_TURN:
          return bobgui_css_dimension_value_new (value * 360.0, BOBGUI_CSS_DEG);
        case BOBGUI_CSS_MS:
          return bobgui_css_dimension_value_new (value / 1000.0, BOBGUI_CSS_S);
        default:
          g_assert_not_reached();
        }
    }
}

static gboolean
bobgui_css_value_number_equal (const BobguiCssValue *val1,
                            const BobguiCssValue *val2)
{
  if (val1->type != val2->type)
    return FALSE;

  if (val1->type == TYPE_DIMENSION)
    {
      return val1->dimension.unit == val2->dimension.unit &&
             val1->dimension.value == val2->dimension.value;
    }
  else
    {
      if (val1->calc.n_terms != val2->calc.n_terms)
        return FALSE;

      for (guint i = 0; i < val1->calc.n_terms; i++)
        {
          if (!bobgui_css_value_equal (val1->calc.terms[i], val2->calc.terms[i]))
            return FALSE;
        }
    }

  return TRUE;
}

static void
bobgui_css_value_number_print (const BobguiCssValue *value,
                            GString           *string)
{
  const char *round_modes[] = { "nearest", "up", "down", "to-zero" };

  switch (value->type)
    {
    case TYPE_DIMENSION:
      {
        const char *names[] = {
          /* [BOBGUI_CSS_NUMBER] = */ "",
          /* [BOBGUI_CSS_PERCENT] = */ "%",
          /* [BOBGUI_CSS_PX] = */ "px",
          /* [BOBGUI_CSS_PT] = */ "pt",
          /* [BOBGUI_CSS_EM] = */ "em",
          /* [BOBGUI_CSS_EX] = */ "ex",
          /* [BOBGUI_CSS_REM] = */ "rem",
          /* [BOBGUI_CSS_PC] = */ "pc",
          /* [BOBGUI_CSS_IN] = */ "in",
          /* [BOBGUI_CSS_CM] = */ "cm",
          /* [BOBGUI_CSS_MM] = */ "mm",
          /* [BOBGUI_CSS_RAD] = */ "rad",
          /* [BOBGUI_CSS_DEG] = */ "deg",
          /* [BOBGUI_CSS_GRAD] = */ "grad",
          /* [BOBGUI_CSS_TURN] = */ "turn",
          /* [BOBGUI_CSS_S] = */ "s",
          /* [BOBGUI_CSS_MS] = */ "ms",
        };
        char buf[G_ASCII_DTOSTR_BUF_SIZE];

        if (isinf (value->dimension.value))
          {
            if (value->dimension.value > 0)
              g_string_append (string, "infinite");
            else
              g_string_append (string, "-infinite");
          }
        else if (isnan (value->dimension.value))
          g_string_append (string, "NaN");
        else
          {
            g_ascii_dtostr (buf, sizeof (buf), value->dimension.value);
            g_string_append (string, buf);
            if (value->dimension.value != 0.0)
              g_string_append (string, names[value->dimension.unit]);
          }

        return;
      }

    case TYPE_CLAMP:
      {
        BobguiCssValue *min = value->calc.terms[0];
        BobguiCssValue *center = value->calc.terms[1];
        BobguiCssValue *max = value->calc.terms[2];

        g_string_append (string, function_name[value->type]);
        g_string_append_c (string, '(');
        if (min != NULL)
          bobgui_css_value_print (min, string);
        else
          g_string_append (string, "none");
        g_string_append (string, ", ");
        bobgui_css_value_print (center, string);
        g_string_append (string, ", ");
        if (max != NULL)
          bobgui_css_value_print (max, string);
        else
          g_string_append (string, "none");
        g_string_append_c (string, ')');
      }
      break;

    case TYPE_ROUND:
      g_string_append (string, function_name[value->type]);
      g_string_append_c (string, '(');
      g_string_append (string, round_modes[value->calc.mode]);
      g_string_append (string, ", ");
      bobgui_css_value_print (value->calc.terms[0], string);
      if (value->calc.n_terms > 1)
        {
          g_string_append (string, ", ");
          bobgui_css_value_print (value->calc.terms[1], string);
        }
      g_string_append_c (string, ')');
      break;

    case TYPE_COLOR_COORD:
      g_string_append (string, bobgui_css_color_space_get_coord_name (value->color_coord.color_space, value->color_coord.coord));
      break;

    default:
      {
        const char *sep = value->type == TYPE_CALC ? " + " : (value->type == TYPE_PRODUCT ? " * " : ", ");

        g_string_append (string, function_name[value->type]);
        g_string_append_c (string, '(');
        bobgui_css_value_print (value->calc.terms[0], string);
        for (guint i = 1; i < value->calc.n_terms; i++)
          {
            g_string_append (string, sep);
            bobgui_css_value_print (value->calc.terms[i], string);
          }
        g_string_append_c (string, ')');
      }
      break;
    }
}

static BobguiCssValue *
bobgui_css_value_number_transition (BobguiCssValue *start,
                                 BobguiCssValue *end,
                                 guint        property_id,
                                 double       progress)
{
  BobguiCssValue *result, *mul_start, *mul_end;

  if (start == end)
    return bobgui_css_value_ref (start);

  if (G_LIKELY (start->type == TYPE_DIMENSION && end->type == TYPE_DIMENSION))
    {
      if (start->dimension.unit == end->dimension.unit)
        {
          const double start_value = start->dimension.value;
          const double end_value = end->dimension.value;
          return bobgui_css_dimension_value_new (start_value + (end_value - start_value) * progress,
                                              start->dimension.unit);
        }
    }

  mul_start = bobgui_css_number_value_multiply (start, 1 - progress);
  mul_end = bobgui_css_number_value_multiply (end, progress);

  result = bobgui_css_number_value_add (mul_start, mul_end);

  bobgui_css_value_unref (mul_start);
  bobgui_css_value_unref (mul_end);

  return result;
}

static BobguiCssValue *
bobgui_css_value_number_resolve (BobguiCssValue          *number,
                              BobguiCssComputeContext *context,
                              BobguiCssValue          *current)
{
  if (number->type == TYPE_COLOR_COORD)
    {
      BobguiCssValue *color, *result;

      color = bobgui_css_value_resolve (number->color_coord.color, context, current);
      result = bobgui_css_number_value_new_color_component (color,
                                                         number->color_coord.color_space,
                                                         number->color_coord.legacy_rgb_scale,
                                                         number->color_coord.coord);
      bobgui_css_value_unref (color);
      return result;
    }
  else if (number->type != TYPE_DIMENSION)
    {
      const guint n_terms = number->calc.n_terms;
      BobguiCssValue *result;
      gboolean changed = FALSE;
      BobguiCssValue **new_values;

      new_values = g_alloca (sizeof (BobguiCssValue *) * n_terms);

      for (gsize i = 0; i < n_terms; i++)
        {
          BobguiCssValue *resolved;

          resolved = bobgui_css_value_resolve (number->calc.terms[i], context, current);
          changed |= resolved != number->calc.terms[i];
          new_values[i] = resolved ;
        }

      if (changed)
        {
          result = bobgui_css_math_value_new (number->type, number->calc.mode, new_values, n_terms);
        }
      else
        {
          for (gsize i = 0; i < n_terms; i++)
            bobgui_css_value_unref (new_values[i]);

          result = bobgui_css_value_ref (number);
        }

      return result;
    }
  else
    {
      return bobgui_css_value_ref (number);
    }
}

static const BobguiCssValueClass BOBGUI_CSS_VALUE_NUMBER = {
  "BobguiCssNumberValue",
  bobgui_css_value_number_free,
  bobgui_css_value_number_compute,
  bobgui_css_value_number_resolve,
  bobgui_css_value_number_equal,
  bobgui_css_value_number_transition,
  NULL,
  NULL,
  bobgui_css_value_number_print
};

static gsize
bobgui_css_value_calc_get_size (gsize n_terms)
{
  g_assert (n_terms > 0);

  return sizeof (BobguiCssValue) + sizeof (BobguiCssValue *) * (n_terms - 1);
}

static BobguiCssValue *
bobgui_css_calc_value_alloc (guint n_terms)
{
  BobguiCssValue *result;

  result = bobgui_css_value_alloc (&BOBGUI_CSS_VALUE_NUMBER,
                                bobgui_css_value_calc_get_size (n_terms));
  result->calc.n_terms = n_terms;

  return result;
}

BobguiCssValue *
bobgui_css_dimension_value_new (double     value,
                             BobguiCssUnit unit)
{
  static BobguiCssValue number_singletons[] = {
    { &BOBGUI_CSS_VALUE_NUMBER, 1, 1, 0, 0, TYPE_DIMENSION, {{ BOBGUI_CSS_NUMBER, 0 }} },
    { &BOBGUI_CSS_VALUE_NUMBER, 1, 1, 0, 0, TYPE_DIMENSION, {{ BOBGUI_CSS_NUMBER, 1 }} },
    { &BOBGUI_CSS_VALUE_NUMBER, 1, 1, 0, 0, TYPE_DIMENSION, {{ BOBGUI_CSS_NUMBER, 96 }} }, /* DPI default */
  };
  static BobguiCssValue px_singletons[] = {
    { &BOBGUI_CSS_VALUE_NUMBER, 1, 1, 0, 0, TYPE_DIMENSION, {{ BOBGUI_CSS_PX, 0 }} },
    { &BOBGUI_CSS_VALUE_NUMBER, 1, 1, 0, 0, TYPE_DIMENSION, {{ BOBGUI_CSS_PX, 1 }} },
    { &BOBGUI_CSS_VALUE_NUMBER, 1, 1, 0, 0, TYPE_DIMENSION, {{ BOBGUI_CSS_PX, 2 }} },
    { &BOBGUI_CSS_VALUE_NUMBER, 1, 1, 0, 0, TYPE_DIMENSION, {{ BOBGUI_CSS_PX, 3 }} },
    { &BOBGUI_CSS_VALUE_NUMBER, 1, 1, 0, 0, TYPE_DIMENSION, {{ BOBGUI_CSS_PX, 4 }} },
    { &BOBGUI_CSS_VALUE_NUMBER, 1, 1, 0, 0, TYPE_DIMENSION, {{ BOBGUI_CSS_PX, 5 }} },
    { &BOBGUI_CSS_VALUE_NUMBER, 1, 1, 0, 0, TYPE_DIMENSION, {{ BOBGUI_CSS_PX, 6 }} },
    { &BOBGUI_CSS_VALUE_NUMBER, 1, 1, 0, 0, TYPE_DIMENSION, {{ BOBGUI_CSS_PX, 7 }} },
    { &BOBGUI_CSS_VALUE_NUMBER, 1, 1, 0, 0, TYPE_DIMENSION, {{ BOBGUI_CSS_PX, 8 }} },
    { &BOBGUI_CSS_VALUE_NUMBER, 1, 1, 0, 0, TYPE_DIMENSION, {{ BOBGUI_CSS_PX, 16 }} }, /* Icon size default */
    { &BOBGUI_CSS_VALUE_NUMBER, 1, 1, 0, 0, TYPE_DIMENSION, {{ BOBGUI_CSS_PX, 32 }} },
    { &BOBGUI_CSS_VALUE_NUMBER, 1, 1, 0, 0, TYPE_DIMENSION, {{ BOBGUI_CSS_PX, 64 }} },
  };
  static BobguiCssValue percent_singletons[] = {
    { &BOBGUI_CSS_VALUE_NUMBER, 1, 0, 0, 0, TYPE_DIMENSION, {{ BOBGUI_CSS_PERCENT, 0 }} },
    { &BOBGUI_CSS_VALUE_NUMBER, 1, 0, 0, 0, TYPE_DIMENSION, {{ BOBGUI_CSS_PERCENT, 50 }} },
    { &BOBGUI_CSS_VALUE_NUMBER, 1, 0, 0, 0, TYPE_DIMENSION, {{ BOBGUI_CSS_PERCENT, 100 }} },
  };
  static BobguiCssValue second_singletons[] = {
    { &BOBGUI_CSS_VALUE_NUMBER, 1, 1, 0, 0, TYPE_DIMENSION, {{ BOBGUI_CSS_S, 0 }} },
    { &BOBGUI_CSS_VALUE_NUMBER, 1, 1, 0, 0, TYPE_DIMENSION, {{ BOBGUI_CSS_S, 1 }} },
  };
  static BobguiCssValue deg_singletons[] = {
    { &BOBGUI_CSS_VALUE_NUMBER, 1, 1, 0, 0, TYPE_DIMENSION, {{ BOBGUI_CSS_DEG, 0 }} },
    { &BOBGUI_CSS_VALUE_NUMBER, 1, 1, 0, 0, TYPE_DIMENSION, {{ BOBGUI_CSS_DEG, 90 }} },
    { &BOBGUI_CSS_VALUE_NUMBER, 1, 1, 0, 0, TYPE_DIMENSION, {{ BOBGUI_CSS_DEG, 180 }} },
    { &BOBGUI_CSS_VALUE_NUMBER, 1, 1, 0, 0, TYPE_DIMENSION, {{ BOBGUI_CSS_DEG, 270 }} },
  };
  BobguiCssValue *result;

  switch ((guint)unit)
    {
    case BOBGUI_CSS_NUMBER:
      if (value == 0 || value == 1)
        return bobgui_css_value_ref (&number_singletons[(int) value]);

      if (value == 96)
        return bobgui_css_value_ref (&number_singletons[2]);

      break;

    case BOBGUI_CSS_PX:
      if (value == 0 || value == 1 || value == 2 || value == 3 ||
          value == 4 || value == 5 || value == 6 || value == 7 ||
          value == 8)
        return bobgui_css_value_ref (&px_singletons[(int) value]);
      if (value == 16)
        return bobgui_css_value_ref (&px_singletons[9]);
      if (value == 32)
        return bobgui_css_value_ref (&px_singletons[10]);
      if (value == 64)
        return bobgui_css_value_ref (&px_singletons[11]);

      break;

    case BOBGUI_CSS_PERCENT:
      if (value == 0)
        return bobgui_css_value_ref (&percent_singletons[0]);
      if (value == 50)
        return bobgui_css_value_ref (&percent_singletons[1]);
      if (value == 100)
        return bobgui_css_value_ref (&percent_singletons[2]);

      break;

    case BOBGUI_CSS_S:
      if (value == 0 || value == 1)
        return bobgui_css_value_ref (&second_singletons[(int)value]);

      break;

    case BOBGUI_CSS_DEG:
      if (value == 0)
        return bobgui_css_value_ref (&deg_singletons[0]);
      if (value == 90)
        return bobgui_css_value_ref (&deg_singletons[1]);
      if (value == 180)
        return bobgui_css_value_ref (&deg_singletons[2]);
      if (value == 270)
        return bobgui_css_value_ref (&deg_singletons[3]);

      break;

    default:
      ;
    }

  result = bobgui_css_value_new (BobguiCssValue, &BOBGUI_CSS_VALUE_NUMBER);
  result->type = TYPE_DIMENSION;
  result->dimension.unit = unit;
  result->dimension.value = value;
  result->is_computed = value == 0 ||
                        unit == BOBGUI_CSS_NUMBER ||
                        unit == BOBGUI_CSS_PX ||
                        unit == BOBGUI_CSS_DEG ||
                        unit == BOBGUI_CSS_S;
  return result;
}

/*
 * bobgui_css_number_value_get_calc_term_order:
 * @value: Value to compute order for
 *
 * Determines the position of @value when printed as part of a calc()
 * expression. Values with lower numbers are printed first. Note that
 * these numbers are arbitrary, so when adding new types of values to
 * print, feel free to change them in implementations so that they
 * match.
 *
 * Returns: Magic value determining placement when printing calc()
 *   expression.
 */
static int
bobgui_css_number_value_get_calc_term_order (const BobguiCssValue *value)
{
  if (G_LIKELY (value->type == TYPE_DIMENSION))
    {
      /* note: the order is alphabetic */
      static const guint order_per_unit[] = {
        /* [BOBGUI_CSS_NUMBER] = */ 0,
        /* [BOBGUI_CSS_PERCENT] = */ 16,
        /* [BOBGUI_CSS_PX] = */ 11,
        /* [BOBGUI_CSS_PT] = */ 10,
        /* [BOBGUI_CSS_EM] = */ 3,
        /* [BOBGUI_CSS_EX] = */ 4,
        /* [BOBGUI_CSS_REM] = */ 13,
        /* [BOBGUI_CSS_PC] = */ 9,
        /* [BOBGUI_CSS_IN] = */ 6,
        /* [BOBGUI_CSS_CM] = */ 1,
        /* [BOBGUI_CSS_MM] = */ 7,
        /* [BOBGUI_CSS_RAD] = */ 12,
        /* [BOBGUI_CSS_DEG] = */ 2,
        /* [BOBGUI_CSS_GRAD] = */ 5,
        /* [BOBGUI_CSS_TURN] = */ 15,
        /* [BOBGUI_CSS_S] = */ 14,
        /* [BOBGUI_CSS_MS] = */ 8
      };
      return 1000 + order_per_unit[value->dimension.unit];
    }

  /* This should never be needed because calc() can't contain calc(),
   * but eh...
   */
  return 0;
}

static void
bobgui_css_calc_array_add (GPtrArray *array, BobguiCssValue *value)
{
  gsize i;
  int calc_term_order;

  calc_term_order = bobgui_css_number_value_get_calc_term_order (value);

  for (i = 0; i < array->len; i++)
    {
      BobguiCssValue *sum = bobgui_css_number_value_try_add (g_ptr_array_index (array, i), value);

      if (sum)
        {
          g_ptr_array_index (array, i) = sum;
          bobgui_css_value_unref (value);
          return;
        }
      else if (bobgui_css_number_value_get_calc_term_order (g_ptr_array_index (array, i)) > calc_term_order)
        {
          g_ptr_array_insert (array, i, value);
          return;
        }
    }

  g_ptr_array_add (array, value);
}

static BobguiCssValue *
bobgui_css_calc_value_new_sum (BobguiCssValue *value1,
                            BobguiCssValue *value2)
{
  GPtrArray *array;
  BobguiCssValue *result;
  gsize i;

  array = g_ptr_array_new ();

  if (value1->class == &BOBGUI_CSS_VALUE_NUMBER &&
      value1->type == TYPE_CALC)
    {
      for (i = 0; i < value1->calc.n_terms; i++)
        {
          bobgui_css_calc_array_add (array, bobgui_css_value_ref (value1->calc.terms[i]));
        }
    }
  else
    {
      bobgui_css_calc_array_add (array, bobgui_css_value_ref (value1));
    }

  if (value2->class == &BOBGUI_CSS_VALUE_NUMBER &&
      value2->type == TYPE_CALC)
    {
      for (i = 0; i < value2->calc.n_terms; i++)
        {
          bobgui_css_calc_array_add (array, bobgui_css_value_ref (value2->calc.terms[i]));
        }
    }
  else
    {
      bobgui_css_calc_array_add (array, bobgui_css_value_ref (value2));
    }

  result = bobgui_css_math_value_new (TYPE_CALC, 0, (BobguiCssValue **)array->pdata, array->len);
  g_ptr_array_free (array, TRUE);

  return result;
}

BobguiCssDimension
bobgui_css_number_value_get_dimension (const BobguiCssValue *value)
{
  switch ((NumberValueType) value->type)
    {
    case TYPE_DIMENSION:
      return bobgui_css_unit_get_dimension (value->dimension.unit);

    case TYPE_CALC:
    case TYPE_MIN:
    case TYPE_MAX:
    case TYPE_HYPOT:
    case TYPE_ABS:
    case TYPE_ROUND:
    case TYPE_MOD:
    case TYPE_REM:
    case TYPE_CLAMP:
      {
        BobguiCssDimension dimension = BOBGUI_CSS_DIMENSION_PERCENTAGE;

        for (guint i = 0; i < value->calc.n_terms && dimension == BOBGUI_CSS_DIMENSION_PERCENTAGE; i++)
          {
            dimension = bobgui_css_number_value_get_dimension (value->calc.terms[i]);
            if (dimension != BOBGUI_CSS_DIMENSION_PERCENTAGE)
              break;
          }
        return dimension;
      }

    case TYPE_PRODUCT:
      if (bobgui_css_number_value_get_dimension (value->calc.terms[0]) != BOBGUI_CSS_DIMENSION_NUMBER)
        return bobgui_css_number_value_get_dimension (value->calc.terms[0]);
      else
        return bobgui_css_number_value_get_dimension (value->calc.terms[1]);

    case TYPE_SIGN:
    case TYPE_SIN:
    case TYPE_COS:
    case TYPE_TAN:
    case TYPE_EXP:
    case TYPE_SQRT:
    case TYPE_POW:
    case TYPE_LOG:
    case TYPE_COLOR_COORD:
      return BOBGUI_CSS_DIMENSION_NUMBER;

    case TYPE_ASIN:
    case TYPE_ACOS:
    case TYPE_ATAN:
    case TYPE_ATAN2:
      return BOBGUI_CSS_DIMENSION_ANGLE;

    default:
      g_assert_not_reached ();
    }
}

gboolean
bobgui_css_number_value_has_percent (const BobguiCssValue *value)
{
  if (value->type == TYPE_COLOR_COORD)
    {
      return FALSE;
    }
  else if (value->type == TYPE_DIMENSION)
    {
      return bobgui_css_unit_get_dimension (value->dimension.unit) == BOBGUI_CSS_DIMENSION_PERCENTAGE;
    }
  else
    {
      for (guint i = 0; i < value->calc.n_terms; i++)
        {
          if (bobgui_css_number_value_has_percent (value->calc.terms[i]))
            return TRUE;
        }

      return FALSE;
    }
}

BobguiCssValue *
bobgui_css_number_value_multiply (BobguiCssValue *value,
                               double       factor)
{
  if (factor == 1)
    return bobgui_css_value_ref (value);

  switch (value->type)
    {
    case TYPE_DIMENSION:
      return bobgui_css_dimension_value_new (value->dimension.value * factor,
                                          value->dimension.unit);

    case TYPE_MIN:
    case TYPE_MAX:
    case TYPE_MOD:
    case TYPE_REM:
      {
        BobguiCssValue **values;
        guint type = value->type;

        values = g_new (BobguiCssValue *, value->calc.n_terms);
        for (guint i = 0; i < value->calc.n_terms; i++)
          values[i] = bobgui_css_number_value_multiply (value->calc.terms[i], factor);

        if (factor < 0)
          {
            if (type == TYPE_MIN)
              type = TYPE_MAX;
            else if (type == TYPE_MAX)
              type = TYPE_MIN;
          }

        return bobgui_css_math_value_new (type, 0, values, value->calc.n_terms);
      }

    case TYPE_CALC:
      {
        BobguiCssValue *result = bobgui_css_calc_value_alloc (value->calc.n_terms);

        result->type = value->type;
        result->calc.mode = value->calc.mode;
        for (guint i = 0; i < value->calc.n_terms; i++)
          result->calc.terms[i] = bobgui_css_number_value_multiply (value->calc.terms[i], factor);
        return result;
      }

    case TYPE_PRODUCT:
      {
        BobguiCssValue *result = bobgui_css_calc_value_alloc (value->calc.n_terms);
        gboolean found = FALSE;

        result->type = value->type;
        result->calc.mode = value->calc.mode;
        for (guint i = 0; i < value->calc.n_terms; i++)
          {
            if (!found &&
                value->calc.terms[i]->type == TYPE_DIMENSION &&
                value->calc.terms[i]->dimension.unit == BOBGUI_CSS_NUMBER)
              {
                result->calc.terms[i] = bobgui_css_number_value_multiply (value->calc.terms[i], factor);
                found = TRUE;
              }
            else
              {
                result->calc.terms[i] = bobgui_css_value_ref (value->calc.terms[i]);
              }
          }

        if (found)
          return result;

        bobgui_css_value_unref (result);
      }
      break;

    case TYPE_ROUND:
      {
        BobguiCssValue *a = bobgui_css_number_value_multiply (value->calc.terms[0], factor);
        BobguiCssValue *b = value->calc.n_terms > 0
                           ? bobgui_css_number_value_multiply (value->calc.terms[1], factor)
                           : bobgui_css_number_value_new (factor, BOBGUI_CSS_NUMBER);

        return bobgui_css_round_value_new (value->calc.mode, a, b);
      }

    case TYPE_CLAMP:
      {
        BobguiCssValue *min = value->calc.terms[0];
        BobguiCssValue *center = value->calc.terms[1];
        BobguiCssValue *max = value->calc.terms[2];

        if (min)
          min = bobgui_css_number_value_multiply (min, factor);
        center = bobgui_css_number_value_multiply (center, factor);
        if (max)
          max = bobgui_css_number_value_multiply (max, factor);

        if (factor < 0)
          {
            BobguiCssValue *tmp = min;
            min = max;
            max = tmp;
          }

        return bobgui_css_clamp_value_new (min, center, max);
      }

    default:
      break;
    }

  return bobgui_css_math_value_new (TYPE_PRODUCT, 0,
                                 (BobguiCssValue *[]) {
                                    bobgui_css_value_ref (value),
                                    bobgui_css_number_value_new (factor, BOBGUI_CSS_NUMBER)
                                 }, 2);
}

BobguiCssValue *
bobgui_css_number_value_add (BobguiCssValue *value1,
                          BobguiCssValue *value2)
{
  BobguiCssValue *sum;

  sum = bobgui_css_number_value_try_add (value1, value2);
  if (sum == NULL)
    sum = bobgui_css_calc_value_new_sum (value1, value2);

  return sum;
}

BobguiCssValue *
bobgui_css_number_value_try_add (BobguiCssValue *value1,
                              BobguiCssValue *value2)
{
  if (G_UNLIKELY (value1->type != value2->type))
    return NULL;

  if (G_LIKELY (value1->type == TYPE_DIMENSION))
    {
      BobguiCssUnit unit = canonical_unit (value1->dimension.unit);
      double v1, v2;

      if (unit != canonical_unit (value2->dimension.unit))
        return NULL;

      if (value1->dimension.value == 0)
        return bobgui_css_value_ref (value2);

      if (value2->dimension.value == 0)
        return bobgui_css_value_ref (value1);

      v1 = get_converted_value (value1, unit);
      v2 = get_converted_value (value2, unit);

      return bobgui_css_dimension_value_new (v1 + v2, unit);
    }

  return NULL;
}

BobguiCssValue *
bobgui_css_number_value_new (double     value,
                          BobguiCssUnit unit)
{
  return bobgui_css_dimension_value_new (value, unit);
}

static BobguiCssValue *
bobgui_css_clamp_value_new (BobguiCssValue *min,
                         BobguiCssValue *center,
                         BobguiCssValue *max)
{
  BobguiCssValue *values[] = { min, center, max };
  BobguiCssUnit unit;
  double min_, center_, max_, v;

  if (min == NULL && max == NULL)
    return center;

  if (!units_compatible (center, min) || !units_compatible (center, max))
    return bobgui_css_calc_value_new (TYPE_CLAMP, 0, values, 3);

  unit = canonical_unit (center->dimension.unit);
  min_ = min ? get_converted_value (min, unit) : -INFINITY;
  center_ = get_converted_value (center, unit);
  max_ = max ? get_converted_value (max, unit) : INFINITY;

  v = CLAMP (center_, min_, max_);

  g_clear_pointer (&min, bobgui_css_value_unref);
  g_clear_pointer (&center, bobgui_css_value_unref);
  g_clear_pointer (&max, bobgui_css_value_unref);

  return bobgui_css_dimension_value_new (v, unit);
}

static BobguiCssValue *
bobgui_css_round_value_new (guint        mode,
                         BobguiCssValue *a,
                         BobguiCssValue *b)
{
  BobguiCssValue *values[2] = { a, b };
  BobguiCssUnit unit;
  double a_, b_, v;

  if (!units_compatible (a, b))
    return bobgui_css_calc_value_new (TYPE_ROUND, mode, values, b != NULL ? 2 : 1);

  unit = canonical_unit (a->dimension.unit);
  a_ = get_converted_value (a, unit);
  b_ = b ? get_converted_value (b, unit) : 1;

  v = _round (mode, a_, b_);

  bobgui_css_value_unref (a);
  bobgui_css_value_unref (b);

  return bobgui_css_dimension_value_new (v, unit);
}

static BobguiCssValue *
bobgui_css_minmax_value_new (guint         type,
                          BobguiCssValue **values,
                          guint         n_values)
{
  BobguiCssValue **vals;
  guint n_vals;

  if (n_values == 1)
    return values[0];

  vals = g_newa (BobguiCssValue *, n_values);
  memset (vals, 0, sizeof (BobguiCssValue *) * n_values);

  n_vals = 0;

  for (guint i = 0; i < n_values; i++)
    {
      BobguiCssValue *value = values[i];

      if (value->type == TYPE_DIMENSION)
        {
          BobguiCssUnit unit;
          double v;

          unit = canonical_unit (value->dimension.unit);
          v = get_converted_value (value, unit);

          for (guint j = 0; j < n_vals; j++)
            {
              if ((vals[j]->type == TYPE_DIMENSION) &&
                  (canonical_unit (vals[j]->dimension.unit) == unit))
                {
                  double v1 = get_converted_value (vals[j], unit);

                  if ((type == TYPE_MIN && v < v1) ||
                      (type == TYPE_MAX && v > v1))
                    {
                      bobgui_css_value_unref (vals[j]);
                      vals[j] = g_steal_pointer (&value);
                    }
                  else
                    {
                      g_clear_pointer (&value, bobgui_css_value_unref);
                    }

                  break;
                }
            }
        }

      if (value)
        {
          vals[n_vals] = value;
          n_vals++;
        }
    }

  return bobgui_css_calc_value_new (type, 0, vals, n_vals);
}

static BobguiCssValue *
bobgui_css_hypot_value_new (BobguiCssValue **values,
                         guint         n_values)
{
  BobguiCssUnit unit;
  double acc;
  double v;

  for (guint i = 0; i < n_values; i++)
    {
      if (value_is_compute_time (values[i]))
        return bobgui_css_calc_value_new (TYPE_HYPOT, 0, values, n_values);
    }

  unit = canonical_unit (values[0]->dimension.unit);
  acc = 0;

  for (guint i = 0; i < n_values; i++)
    {
      double a = get_converted_value (values[i], unit);
      acc += a * a;
    }

  v = sqrt (acc);

  for (guint i = 0; i < n_values; i++)
    bobgui_css_value_unref (values[i]);

  return bobgui_css_dimension_value_new (v, unit);
}

static BobguiCssValue *
bobgui_css_arg1_value_new (guint        type,
                        BobguiCssValue *value)
{
  BobguiCssUnit unit;
  double a;
  double v;

  if (value_is_compute_time (value))
    return bobgui_css_calc_value_new (type, 0, &value, 1);

  a = get_converted_value (value, canonical_unit (value->dimension.unit));

  if (type == TYPE_SIN || type == TYPE_COS || type == TYPE_TAN)
    {
      if (bobgui_css_unit_get_dimension (value->dimension.unit) == BOBGUI_CSS_DIMENSION_ANGLE)
        a = DEG_TO_RAD (a);
    }

  switch (type)
    {
    case TYPE_SIN: v = sin (a); break;
    case TYPE_COS: v = cos (a); break;
    case TYPE_TAN: v = tan (a); break;
    case TYPE_ASIN: v = asin (a); break;
    case TYPE_ACOS: v = acos (a); break;
    case TYPE_ATAN: v = atan (a); break;
    case TYPE_SQRT: v = sqrt (a); break;
    case TYPE_EXP: v = exp (a); break;
    case TYPE_ABS: v = fabs (a); break;
    case TYPE_SIGN: v = _sign (a); break;
    default: g_assert_not_reached ();
    }

  if (type == TYPE_ASIN || type == TYPE_ACOS || type == TYPE_ATAN)
    {
      unit = BOBGUI_CSS_DEG;
      v = RAD_TO_DEG (v);
    }
  else if (type == TYPE_ABS)
    {
      unit = value->dimension.unit;
    }
  else
    {
      unit = BOBGUI_CSS_NUMBER;
    }

  bobgui_css_value_unref (value);

  return bobgui_css_dimension_value_new (v, unit);
}

static BobguiCssValue *
bobgui_css_arg2_value_new (guint        type,
                        BobguiCssValue *value1,
                        BobguiCssValue *value2)
{
  BobguiCssValue *values[2] = { value1, value2 };
  BobguiCssUnit unit;
  double a, b = 1;
  double v;

  if (value_is_compute_time (value1) ||
      (value2 && value_is_compute_time (value2)))
    return bobgui_css_calc_value_new (type, 0, values, 2);

  a = get_converted_value (value1, canonical_unit (value1->dimension.unit));
  if (value2)
    b = get_converted_value (value2, canonical_unit (value2->dimension.unit));

  switch (type)
    {
    case TYPE_MOD: v = _mod (a, b); break;
    case TYPE_REM: v = _rem (a, b); break;
    case TYPE_ATAN2: v = atan2 (a, b); break;
    case TYPE_POW: v = pow (a, b); break;
    case TYPE_LOG: v = value2 ? log (a) / log (b) : log (a); break;
    default: g_assert_not_reached ();
    }

  if (type == TYPE_ATAN2)
    {
      unit = BOBGUI_CSS_DEG;
      v = RAD_TO_DEG (v);
    }
  else
    {
      unit = BOBGUI_CSS_NUMBER;
    }

  bobgui_css_value_unref (value1);
  bobgui_css_value_unref (value2);

  return bobgui_css_dimension_value_new (v, unit);
}

/* This function is called at parsing time, so units are not
 * canonical, and length values can't necessarily be unified.
 */
BobguiCssValue *
bobgui_css_math_value_new (guint         type,
                        guint         mode,
                        BobguiCssValue **values,
                        guint         n_values)
{
  switch ((NumberValueType) type)
    {
    case TYPE_DIMENSION:
    case TYPE_COLOR_COORD:
      g_assert_not_reached ();

    case TYPE_ROUND:
      return bobgui_css_round_value_new (mode, values[0], values[1]);

    case TYPE_CLAMP:
      return bobgui_css_clamp_value_new (values[0], values[1], values[2]);

    case TYPE_HYPOT:
      return bobgui_css_hypot_value_new (values, n_values);

    case TYPE_MIN:
    case TYPE_MAX:
      return bobgui_css_minmax_value_new (type, values, n_values);

    case TYPE_SIN:
    case TYPE_COS:
    case TYPE_TAN:
    case TYPE_ASIN:
    case TYPE_ACOS:
    case TYPE_ATAN:
    case TYPE_SQRT:
    case TYPE_EXP:
    case TYPE_ABS:
    case TYPE_SIGN:
      return bobgui_css_arg1_value_new (type, values[0]);

    case TYPE_MOD:
    case TYPE_REM:
    case TYPE_ATAN2:
    case TYPE_POW:
    case TYPE_LOG:
      return bobgui_css_arg2_value_new (type, values[0], values[1]);

    case TYPE_PRODUCT:
    case TYPE_CALC:
    default:
      return bobgui_css_calc_value_new (type, mode, values, n_values);
    }
}

gboolean
bobgui_css_number_value_can_parse (BobguiCssParser *parser)
{
  const BobguiCssToken *token = bobgui_css_parser_get_token (parser);

  switch ((int) token->type)
    {
    case BOBGUI_CSS_TOKEN_SIGNED_NUMBER:
    case BOBGUI_CSS_TOKEN_SIGNLESS_NUMBER:
    case BOBGUI_CSS_TOKEN_SIGNED_INTEGER:
    case BOBGUI_CSS_TOKEN_SIGNLESS_INTEGER:
    case BOBGUI_CSS_TOKEN_PERCENTAGE:
    case BOBGUI_CSS_TOKEN_SIGNED_INTEGER_DIMENSION:
    case BOBGUI_CSS_TOKEN_SIGNLESS_INTEGER_DIMENSION:
    case BOBGUI_CSS_TOKEN_SIGNED_DIMENSION:
    case BOBGUI_CSS_TOKEN_SIGNLESS_DIMENSION:
      return TRUE;

    case BOBGUI_CSS_TOKEN_FUNCTION:
      {
        const char *name = bobgui_css_token_get_string (token);

        for (guint i = 0; i < G_N_ELEMENTS (function_name); i++)
          {
            if (g_ascii_strcasecmp (function_name[i], name) == 0)
              return TRUE;
          }
      }
      break;

    default:
      break;
    }

  return FALSE;
}

BobguiCssValue *
bobgui_css_number_value_parse (BobguiCssParser           *parser,
                            BobguiCssNumberParseFlags  flags)
{
  BobguiCssNumberParseContext ctx = { NULL, 0, FALSE };

  return bobgui_css_number_value_parse_with_context (parser, flags, &ctx);
}

BobguiCssValue *
bobgui_css_number_value_parse_with_context (BobguiCssParser             *parser,
                                         BobguiCssNumberParseFlags    flags,
                                         BobguiCssNumberParseContext *ctx)
{
  const BobguiCssToken *token = bobgui_css_parser_get_token (parser);

  if (bobgui_css_token_is (token, BOBGUI_CSS_TOKEN_FUNCTION))
    {
      const char *name = bobgui_css_token_get_string (token);

      if (g_ascii_strcasecmp (name, "calc") == 0)
        return bobgui_css_calc_value_parse (parser, flags, ctx);
      else if (g_ascii_strcasecmp (name, "min") == 0)
        return bobgui_css_argn_value_parse (parser, flags, ctx, "min", TYPE_MIN);
      else if (g_ascii_strcasecmp (name, "max") == 0)
        return bobgui_css_argn_value_parse (parser, flags, ctx, "max", TYPE_MAX);
      else if (g_ascii_strcasecmp (name, "hypot") == 0)
        return bobgui_css_argn_value_parse (parser, flags, ctx, "hypot", TYPE_HYPOT);
      else if (g_ascii_strcasecmp (name, "clamp") == 0)
        return bobgui_css_clamp_value_parse (parser, flags, ctx, TYPE_CLAMP);
      else if (g_ascii_strcasecmp (name, "round") == 0)
        return bobgui_css_round_value_parse (parser, flags, ctx, TYPE_ROUND);
      else if (g_ascii_strcasecmp (name, "mod") == 0)
        return bobgui_css_arg2_value_parse (parser, flags, ctx, 2, 2, "mod", TYPE_MOD);
      else if (g_ascii_strcasecmp (name, "rem") == 0)
        return bobgui_css_arg2_value_parse (parser, flags, ctx, 2, 2, "rem", TYPE_REM);
      else if (g_ascii_strcasecmp (name, "abs") == 0)
        return bobgui_css_arg2_value_parse (parser, flags, ctx, 1, 1, "abs", TYPE_ABS);
      else if ((flags & BOBGUI_CSS_PARSE_NUMBER) && g_ascii_strcasecmp (name, "sign") == 0)
        return bobgui_css_arg2_value_parse (parser, BOBGUI_CSS_PARSE_NUMBER|BOBGUI_CSS_PARSE_DIMENSION|BOBGUI_CSS_PARSE_PERCENT, ctx, 1, 1, "sign", TYPE_SIGN);
      else if ((flags & BOBGUI_CSS_PARSE_NUMBER) && g_ascii_strcasecmp (name, "sin") == 0)
        return bobgui_css_arg2_value_parse (parser, BOBGUI_CSS_PARSE_NUMBER|BOBGUI_CSS_PARSE_ANGLE, ctx, 1, 1, "sin", TYPE_SIN);
      else if ((flags & BOBGUI_CSS_PARSE_NUMBER) && g_ascii_strcasecmp (name, "cos") == 0)
        return bobgui_css_arg2_value_parse (parser, BOBGUI_CSS_PARSE_NUMBER|BOBGUI_CSS_PARSE_ANGLE, ctx, 1, 1, "cos", TYPE_COS);
      else if ((flags & BOBGUI_CSS_PARSE_NUMBER) && g_ascii_strcasecmp (name, "tan") == 0)
        return bobgui_css_arg2_value_parse (parser, BOBGUI_CSS_PARSE_NUMBER|BOBGUI_CSS_PARSE_ANGLE, ctx, 1, 1, "tan", TYPE_TAN);
      else if ((flags & BOBGUI_CSS_PARSE_ANGLE) && g_ascii_strcasecmp (name, "asin") == 0)
        return bobgui_css_arg2_value_parse (parser, BOBGUI_CSS_PARSE_NUMBER, ctx, 1, 1, "asin", TYPE_ASIN);
      else if ((flags & BOBGUI_CSS_PARSE_ANGLE) && g_ascii_strcasecmp (name, "acos") == 0)
        return bobgui_css_arg2_value_parse (parser, BOBGUI_CSS_PARSE_NUMBER, ctx, 1, 1, "acos", TYPE_ACOS);
      else if ((flags & BOBGUI_CSS_PARSE_ANGLE) && g_ascii_strcasecmp (name, "atan") == 0)
        return bobgui_css_arg2_value_parse (parser, BOBGUI_CSS_PARSE_NUMBER, ctx, 1, 1, "atan", TYPE_ATAN);
      else if ((flags & BOBGUI_CSS_PARSE_ANGLE) && g_ascii_strcasecmp (name, "atan2") == 0)
        return bobgui_css_arg2_value_parse (parser, BOBGUI_CSS_PARSE_NUMBER|BOBGUI_CSS_PARSE_DIMENSION|BOBGUI_CSS_PARSE_PERCENT, ctx, 2, 2, "atan2", TYPE_ATAN2);
      else if ((flags & BOBGUI_CSS_PARSE_NUMBER) && g_ascii_strcasecmp (name, "pow") == 0)
        return bobgui_css_arg2_value_parse (parser, BOBGUI_CSS_PARSE_NUMBER, ctx, 2, 2, "pow", TYPE_POW);
      else if ((flags & BOBGUI_CSS_PARSE_NUMBER) && g_ascii_strcasecmp (name, "sqrt") == 0)
        return bobgui_css_arg2_value_parse (parser, BOBGUI_CSS_PARSE_NUMBER, ctx, 1, 1, "sqrt", TYPE_SQRT);
      else if ((flags & BOBGUI_CSS_PARSE_NUMBER) && g_ascii_strcasecmp (name, "exp") == 0)
        return bobgui_css_arg2_value_parse (parser, BOBGUI_CSS_PARSE_NUMBER, ctx, 1, 1, "exp", TYPE_EXP);
      else if ((flags & BOBGUI_CSS_PARSE_NUMBER) && g_ascii_strcasecmp (name, "log") == 0)
        return bobgui_css_arg2_value_parse (parser, BOBGUI_CSS_PARSE_NUMBER, ctx, 1, 2, "log", TYPE_LOG);
    }
  else if (bobgui_css_token_is (token, BOBGUI_CSS_TOKEN_IDENT))
    {
      const char *name = bobgui_css_token_get_string (token);
      struct {
        const char *name;
        double value;
      } constants[] = {
        { "e", G_E },
        { "pi", G_PI },
        { "infinity", INFINITY },
        { "-infinity", -INFINITY },
        { "NaN", NAN },
      };

      for (guint i = 0; i < G_N_ELEMENTS (constants); i++)
        {
          if (g_ascii_strcasecmp (name, constants[i].name) == 0)
            {
              bobgui_css_parser_consume_token (parser);
              return bobgui_css_number_value_new (constants[i].value, BOBGUI_CSS_NUMBER);
            }
        }

      if (ctx->color)
        {
          for (guint i = 0; i < 4; i++)
            {
              if (g_ascii_strcasecmp (name, bobgui_css_color_space_get_coord_name (ctx->color_space, i)) == 0)
                {
                  bobgui_css_parser_consume_token (parser);
                  return bobgui_css_number_value_new_color_component (ctx->color, ctx->color_space, ctx->legacy_rgb_scale, i);
                }
            }
        }
    }

  return bobgui_css_dimension_value_parse (parser, flags);
}

/* This function is safe to call on computed values, since all
 * units are canonical and all lengths are in px at that time.
 */
double
bobgui_css_number_value_get (const BobguiCssValue *value,
                          double             one_hundred_percent)
{
  guint type = value->type;
  guint mode = value->calc.mode;
  BobguiCssValue * const *terms = value->calc.terms;
  guint n_terms = value->calc.n_terms;

  switch ((NumberValueType) type)
    {
    case TYPE_DIMENSION:
      if (value->dimension.unit == BOBGUI_CSS_PERCENT)
        return value->dimension.value * one_hundred_percent / 100;
      else
        return value->dimension.value;

    case TYPE_CALC:
      {
        double result = 0.0;

        for (guint i = 0; i < n_terms; i++)
          result += bobgui_css_number_value_get (terms[i], one_hundred_percent);

        return result;
      }

    case TYPE_PRODUCT:
      {
        double result = 1.0;

        for (guint i = 0; i < n_terms; i++)
          result *= bobgui_css_number_value_get (terms[i], one_hundred_percent);

        return result;
      }

    case TYPE_MIN:
      {
        double result = G_MAXDOUBLE;

        for (guint i = 0; i < n_terms; i++)
          result = MIN (result, bobgui_css_number_value_get (terms[i], one_hundred_percent));

        return result;
      }

    case TYPE_MAX:
      {
        double result = -G_MAXDOUBLE;

        for (guint i = 0; i < n_terms; i++)
          result = MAX (result, bobgui_css_number_value_get (terms[i], one_hundred_percent));

        return result;
      }

    case TYPE_CLAMP:
      {
        BobguiCssValue *min = terms[0];
        BobguiCssValue *center = terms[1];
        BobguiCssValue *max = terms[2];
        double result;

        result = bobgui_css_number_value_get (center, one_hundred_percent);

        if (max)
          result = MIN (result, bobgui_css_number_value_get (max, one_hundred_percent));
        if (min)
          result = MAX (result, bobgui_css_number_value_get (min, one_hundred_percent));

        return result;
      }

    case TYPE_ROUND:
      {
        double a = bobgui_css_number_value_get (terms[0], one_hundred_percent);

        double b = terms[1] != NULL ? bobgui_css_number_value_get (terms[1], one_hundred_percent) : 1;

        return _round (mode, a, b);
      }

    case TYPE_MOD:
      {
        double a = bobgui_css_number_value_get (terms[0], one_hundred_percent);
        double b = bobgui_css_number_value_get (terms[1], one_hundred_percent);

        return _mod (a, b);
      }

    case TYPE_REM:
      {
        double a = bobgui_css_number_value_get (terms[0], one_hundred_percent);
        double b = bobgui_css_number_value_get (terms[1], one_hundred_percent);

        return _rem (a, b);
      }

    case TYPE_ABS:
      {
        double a = bobgui_css_number_value_get (terms[0], one_hundred_percent);

        return fabs (a);
      }

    case TYPE_SIGN:
      {
        double a = bobgui_css_number_value_get (terms[0], one_hundred_percent);

        return _sign (a);
      }

    case TYPE_SIN:
      {
        double a = bobgui_css_number_value_get (terms[0], one_hundred_percent);

        if (bobgui_css_number_value_get_dimension (value) == BOBGUI_CSS_DIMENSION_ANGLE)
          a = DEG_TO_RAD (a);

        return sin (a);
      }

    case TYPE_COS:
      {
        double a = bobgui_css_number_value_get (terms[0], one_hundred_percent);

        if (bobgui_css_number_value_get_dimension (value) == BOBGUI_CSS_DIMENSION_ANGLE)
          a = DEG_TO_RAD (a);

        return cos (a);
      }

    case TYPE_TAN:
      {
        double a = bobgui_css_number_value_get (terms[0], one_hundred_percent);

        if (bobgui_css_number_value_get_dimension (value) == BOBGUI_CSS_DIMENSION_ANGLE)
          a = DEG_TO_RAD (a);

        return tan (a);
      }

    case TYPE_ASIN:
      {
        double a = bobgui_css_number_value_get (terms[0], one_hundred_percent);

        return RAD_TO_DEG (asin (a));
      }

    case TYPE_ACOS:
      {
        double a = bobgui_css_number_value_get (terms[0], one_hundred_percent);

        return RAD_TO_DEG (acos (a));
      }

    case TYPE_ATAN:
      {
        double a = bobgui_css_number_value_get (terms[0], one_hundred_percent);

        return RAD_TO_DEG (atan (a));
      }

    case TYPE_ATAN2:
      {
        double a = bobgui_css_number_value_get (terms[0], one_hundred_percent);
        double b = bobgui_css_number_value_get (terms[1], one_hundred_percent);

        return RAD_TO_DEG (atan2 (a, b));
      }

    case TYPE_POW:
      {
        double a = bobgui_css_number_value_get (terms[0], one_hundred_percent);
        double b = bobgui_css_number_value_get (terms[1], one_hundred_percent);

        return pow (a, b);
      }

    case TYPE_SQRT:
      {
        double a = bobgui_css_number_value_get (terms[0], one_hundred_percent);

        return sqrt (a);
      }

    case TYPE_EXP:
      {
        double a = bobgui_css_number_value_get (terms[0], one_hundred_percent);

        return exp (a);
      }

    case TYPE_LOG:
      if (n_terms > 1)
        {
          double a = bobgui_css_number_value_get (terms[0], one_hundred_percent);
          double b = bobgui_css_number_value_get (terms[1], one_hundred_percent);

          return log (a) / log (b);
        }
      else
        {
          double a = bobgui_css_number_value_get (terms[0], one_hundred_percent);

          return log (a);
        }

    case TYPE_HYPOT:
      {
        double acc = 0;

        for (guint i = 0; i < n_terms; i++)
          {
            double a = bobgui_css_number_value_get (terms[i], one_hundred_percent);

            acc += a * a;
          }

        return sqrt (acc);
      }

    case TYPE_COLOR_COORD:
      return bobgui_css_color_value_get_coord (value->color_coord.color,
                                            value->color_coord.color_space,
                                            value->color_coord.legacy_rgb_scale,
                                            value->color_coord.coord);

    default:
      g_assert_not_reached ();
    }
}

double
bobgui_css_number_value_get_canonical (BobguiCssValue *number,
                                    double       one_hundred_percent)
{
  if (number->type == TYPE_DIMENSION && number->dimension.unit != BOBGUI_CSS_PERCENT)
    return get_converted_value (number, canonical_unit (number->dimension.unit));

  return bobgui_css_number_value_get (number, one_hundred_percent);
}

gboolean
bobgui_css_dimension_value_is_zero (const BobguiCssValue *value)
{
  g_assert (value != 0);
  g_assert (value->class == &BOBGUI_CSS_VALUE_NUMBER);

  if (value->type != TYPE_DIMENSION)
    return FALSE;

  return value->dimension.value == 0;
}

static double
_round (guint mode, double a, double b)
{
  int old_mode;
  int modes[] = { FE_TONEAREST, FE_UPWARD, FE_DOWNWARD, FE_TOWARDZERO };
  double result;

  if (b == 0)
    return NAN;

  if (isinf (a))
    {
      if (isinf (b))
        return NAN;
      else
        return a;
    }

  if (isinf (b))
    {
      switch (mode)
        {
        case ROUND_NEAREST:
        case ROUND_TO_ZERO:
          return 0;
        case ROUND_UP:
          return a > 0 ? INFINITY : 0;
        case ROUND_DOWN:
          return a < 0 ? -INFINITY : 0;
        default:
          g_assert_not_reached ();
        }
    }

  old_mode = fegetround ();
  fesetround (modes[mode]);

  result = nearbyint (a/b) * b;

  fesetround (old_mode);

  return result;
}

static double
_mod (double a, double b)
{
  double z;

  if (b == 0 || isinf (a))
    return NAN;

  if (isinf (b) && (a < 0) != (b < 0))
    return NAN;

  z = fmod (a, b);
  if (z < 0)
    z += b;

  return z;
}

static double
_rem (double a, double b)
{
  if (b == 0 || isinf (a))
    return NAN;

  if (isinf (b))
    return a;

  return fmod (a, b);
}

static double
_sign (double a)
{
  if (a < 0)
    return -1;
  else if (a > 0)
    return 1;
  else
    return 0;
}

BobguiCssValue *
bobgui_css_number_value_new_color_component (BobguiCssValue      *color,
                                          BobguiCssColorSpace  color_space,
                                          gboolean          legacy_rgb_scale,
                                          guint             coord)
{
  if (bobgui_css_value_is_computed (color) &&
      !bobgui_css_value_contains_current_color (color))
    {
      float v;

      v = bobgui_css_color_value_get_coord (color, color_space, legacy_rgb_scale, coord);

      return bobgui_css_number_value_new (v, BOBGUI_CSS_NUMBER);
    }
  else
    {
      BobguiCssValue *result;

      result = bobgui_css_value_new (BobguiCssValue, &BOBGUI_CSS_VALUE_NUMBER);
      result->type = TYPE_COLOR_COORD;
      result->color_coord.color_space = color_space;
      result->color_coord.color = bobgui_css_value_ref (color);
      result->color_coord.coord = coord;
      result->color_coord.legacy_rgb_scale = legacy_rgb_scale;
      result->is_computed = bobgui_css_value_is_computed (color);
      result->contains_current_color = bobgui_css_value_contains_current_color (color);

      return result;
    }
}
