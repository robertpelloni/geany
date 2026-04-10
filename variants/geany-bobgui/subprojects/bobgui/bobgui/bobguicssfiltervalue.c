/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2017 Red Hat, Inc.
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

#include "bobguicssbgsizevalueprivate.h"

#include <math.h>
#include <string.h>

#include "bobguicssfiltervalueprivate.h"
#include "bobguicssnumbervalueprivate.h"
#include "bobguicssshadowvalueprivate.h"
#include "css/bobguicssdataurlprivate.h"
#include "svg/bobguisvgprivate.h"
#include "bobguisnapshotprivate.h"

#include "gsk/gskcairoblurprivate.h"

typedef union _BobguiCssFilter BobguiCssFilter;

typedef enum {
  BOBGUI_CSS_FILTER_NONE,
  BOBGUI_CSS_FILTER_BLUR,
  BOBGUI_CSS_FILTER_BRIGHTNESS,
  BOBGUI_CSS_FILTER_CONTRAST,
  BOBGUI_CSS_FILTER_DROP_SHADOW,
  BOBGUI_CSS_FILTER_GRAYSCALE,
  BOBGUI_CSS_FILTER_HUE_ROTATE,
  BOBGUI_CSS_FILTER_INVERT,
  BOBGUI_CSS_FILTER_OPACITY,
  BOBGUI_CSS_FILTER_SATURATE,
  BOBGUI_CSS_FILTER_SEPIA,
  BOBGUI_CSS_FILTER_SVG,
} BobguiCssFilterType;

union _BobguiCssFilter {
  BobguiCssFilterType       type;
  struct {
    BobguiCssFilterType     type;
    BobguiCssValue         *value;
  }            blur, brightness, contrast, drop_shadow, grayscale, hue_rotate, invert, opacity, saturate, sepia;
  struct {
    BobguiCssFilterType type;
    char *url;
    char *ref;
    BobguiSvg *svg;
  } svg;
};

struct _BobguiCssValue {
  BOBGUI_CSS_VALUE_BASE
  guint                 n_filters;
  BobguiCssFilter          filters[1];
};

static BobguiCssValue *    bobgui_css_filter_value_alloc           (guint                  n_values);

static void
bobgui_css_filter_clear (BobguiCssFilter *filter)
{
  switch (filter->type)
    {
    case BOBGUI_CSS_FILTER_BRIGHTNESS:
      bobgui_css_value_unref (filter->brightness.value);
      break;
    case BOBGUI_CSS_FILTER_CONTRAST:
      bobgui_css_value_unref (filter->contrast.value);
      break;
    case BOBGUI_CSS_FILTER_GRAYSCALE:
      bobgui_css_value_unref (filter->grayscale.value);
      break;
    case BOBGUI_CSS_FILTER_HUE_ROTATE:
      bobgui_css_value_unref (filter->hue_rotate.value);
      break;
    case BOBGUI_CSS_FILTER_INVERT:
      bobgui_css_value_unref (filter->invert.value);
      break;
    case BOBGUI_CSS_FILTER_OPACITY:
      bobgui_css_value_unref (filter->opacity.value);
      break;
    case BOBGUI_CSS_FILTER_SATURATE:
      bobgui_css_value_unref (filter->saturate.value);
      break;
    case BOBGUI_CSS_FILTER_SEPIA:
      bobgui_css_value_unref (filter->sepia.value);
      break;
    case BOBGUI_CSS_FILTER_BLUR:
      bobgui_css_value_unref (filter->blur.value);
      break;
    case BOBGUI_CSS_FILTER_DROP_SHADOW:
      bobgui_css_value_unref (filter->drop_shadow.value);
      break;
    case BOBGUI_CSS_FILTER_SVG:
      g_free (filter->svg.ref);
      g_free (filter->svg.url);
      g_clear_object (&filter->svg.svg);
      break;
    case BOBGUI_CSS_FILTER_NONE:
    default:
      g_assert_not_reached ();
      break;
    }
}

static void
bobgui_css_filter_init_identity (BobguiCssFilter       *filter,
                              const BobguiCssFilter *other)
{
  switch (other->type)
    {
    case BOBGUI_CSS_FILTER_BRIGHTNESS:
      filter->brightness.value = bobgui_css_number_value_new (1, BOBGUI_CSS_NUMBER);
      break;
    case BOBGUI_CSS_FILTER_CONTRAST:
      filter->contrast.value = bobgui_css_number_value_new (1, BOBGUI_CSS_NUMBER);
      break;
    case BOBGUI_CSS_FILTER_GRAYSCALE:
      filter->grayscale.value = bobgui_css_number_value_new (0, BOBGUI_CSS_NUMBER);
      break;
    case BOBGUI_CSS_FILTER_HUE_ROTATE:
      filter->hue_rotate.value = bobgui_css_number_value_new (0, BOBGUI_CSS_DEG);
      break;
    case BOBGUI_CSS_FILTER_INVERT:
      filter->invert.value = bobgui_css_number_value_new (0, BOBGUI_CSS_NUMBER);
      break;
    case BOBGUI_CSS_FILTER_OPACITY:
      filter->opacity.value = bobgui_css_number_value_new (1, BOBGUI_CSS_NUMBER);
      break;
    case BOBGUI_CSS_FILTER_SATURATE:
      filter->saturate.value = bobgui_css_number_value_new (1, BOBGUI_CSS_NUMBER);
      break;
    case BOBGUI_CSS_FILTER_SEPIA:
      filter->sepia.value = bobgui_css_number_value_new (0, BOBGUI_CSS_NUMBER);
      break;
    case BOBGUI_CSS_FILTER_BLUR:
      filter->blur.value = bobgui_css_number_value_new (0, BOBGUI_CSS_PX);
      break;
    case BOBGUI_CSS_FILTER_DROP_SHADOW:
      filter->drop_shadow.value = bobgui_css_shadow_value_new_filter (other->drop_shadow.value);
      break;
    case BOBGUI_CSS_FILTER_SVG:
      filter->svg.ref = NULL;
      filter->svg.url = NULL;
      filter->svg.svg = NULL;
      break;
    case BOBGUI_CSS_FILTER_NONE:
    default:
      g_assert_not_reached ();
      break;
    }

  filter->type = other->type;
}

#define R 0.2126
#define G 0.7152
#define B 0.0722

static gboolean
bobgui_css_filter_get_matrix (const BobguiCssFilter *filter,
                           graphene_matrix_t  *matrix,
                           graphene_vec4_t    *offset)
{
  double value;

  switch (filter->type)
    {
    case BOBGUI_CSS_FILTER_BRIGHTNESS:
      value = bobgui_css_number_value_get (filter->brightness.value, 1.0);
      graphene_matrix_init_scale (matrix, value, value, value);
      graphene_vec4_init (offset, 0.0, 0.0, 0.0, 0.0);
      break;

    case BOBGUI_CSS_FILTER_CONTRAST:
      value = bobgui_css_number_value_get (filter->contrast.value, 1.0);
      graphene_matrix_init_scale (matrix, value, value, value);
      graphene_vec4_init (offset, 0.5 - 0.5 * value, 0.5 - 0.5 * value, 0.5 - 0.5 * value, 0.0);
      break;

    case BOBGUI_CSS_FILTER_GRAYSCALE:
      value = bobgui_css_number_value_get (filter->grayscale.value, 1.0);
      graphene_matrix_init_from_float (matrix, (float[16]) {
                                           1.0 - (1.0 - R) * value, R * value, R * value, 0.0,
                                           G * value, 1.0 - (1.0 - G) * value, G * value, 0.0,
                                           B * value, B * value, 1.0 - (1.0 - B) * value, 0.0,
                                           0.0, 0.0, 0.0, 1.0
                                       });
      graphene_vec4_init (offset, 0.0, 0.0, 0.0, 0.0);
      break;

    case BOBGUI_CSS_FILTER_HUE_ROTATE:
      {
        double c, s;
        value = bobgui_css_number_value_get (filter->hue_rotate.value, 1.0) * G_PI / 180.0;
        c = cos (value);
        s = sin (value);
        graphene_matrix_init_from_float (matrix, (float[16]) {
                                             0.213 + 0.787 * c - 0.213 * s,
                                             0.213 - 0.213 * c + 0.143 * s,
                                             0.213 - 0.213 * c - 0.787 * s,
                                             0,
                                             0.715 - 0.715 * c - 0.715 * s,
                                             0.715 + 0.285 * c + 0.140 * s,
                                             0.715 - 0.715 * c + 0.715 * s,
                                             0,
                                             0.072 - 0.072 * c + 0.928 * s,
                                             0.072 - 0.072 * c - 0.283 * s,
                                             0.072 + 0.928 * c + 0.072 * s,
                                             0,
                                             0, 0, 0, 1
                                         });
        graphene_vec4_init (offset, 0.0, 0.0, 0.0, 0.0);
      }
      break;

    case BOBGUI_CSS_FILTER_INVERT:
      value = bobgui_css_number_value_get (filter->invert.value, 1.0);
      graphene_matrix_init_scale (matrix, 1.0 - 2 * value, 1.0 - 2 * value, 1.0 - 2 * value);
      graphene_vec4_init (offset, value, value, value, 0.0);
      break;

    case BOBGUI_CSS_FILTER_OPACITY:
      value = bobgui_css_number_value_get (filter->opacity.value, 1.0);
      graphene_matrix_init_from_float (matrix, (float[16]) {
                                           1.0, 0.0, 0.0, 0.0,
                                           0.0, 1.0, 0.0, 0.0,
                                           0.0, 0.0, 1.0, 0.0,
                                           0.0, 0.0, 0.0, value
                                       });
      graphene_vec4_init (offset, 0.0, 0.0, 0.0, 0.0);
      break;

    case BOBGUI_CSS_FILTER_SATURATE:
      value = bobgui_css_number_value_get (filter->saturate.value, 1.0);
      graphene_matrix_init_from_float (matrix, (float[16]) {
                                           R + (1.0 - R) * value, R - R * value, R - R * value, 0.0,
                                           G - G * value, G + (1.0 - G) * value, G - G * value, 0.0,
                                           B - B * value, B - B * value, B + (1.0 - B) * value, 0.0,
                                           0.0, 0.0, 0.0, 1.0
                                       });
      graphene_vec4_init (offset, 0.0, 0.0, 0.0, 0.0);
      break;

    case BOBGUI_CSS_FILTER_SEPIA:
      value = bobgui_css_number_value_get (filter->sepia.value, 1.0);
      graphene_matrix_init_from_float (matrix, (float[16]) {
                                           1.0 - 0.607 * value, 0.349 * value, 0.272 * value, 0.0,
                                           0.769 * value, 1.0 - 0.314 * value, 0.534 * value, 0.0,
                                           0.189 * value, 0.168 * value, 1.0 - 0.869 * value, 0.0,
                                           0.0, 0.0, 0.0, 1.0
                                       });
      graphene_vec4_init (offset, 0.0, 0.0, 0.0, 0.0);
      break;

    case BOBGUI_CSS_FILTER_NONE:
    case BOBGUI_CSS_FILTER_BLUR:
    case BOBGUI_CSS_FILTER_DROP_SHADOW:
    case BOBGUI_CSS_FILTER_SVG:
      return FALSE;
    default:
      g_assert_not_reached ();
      break;
    }

  return TRUE;
}

#undef R
#undef G
#undef B

static int
bobgui_css_filter_value_compute_matrix (const BobguiCssValue *value,
                                     int                first,
                                     graphene_matrix_t *matrix,
                                     graphene_vec4_t   *offset,
                                     gboolean          *all_opacity)
{
  graphene_matrix_t m, m2;
  graphene_vec4_t o, o2;
  int i;

  if (!bobgui_css_filter_get_matrix (&value->filters[first], matrix, offset))
    return first;

  *all_opacity = value->filters[first].type == BOBGUI_CSS_FILTER_OPACITY;

  for (i = first + 1; i < value->n_filters; i++)
    {
      if (!bobgui_css_filter_get_matrix (&value->filters[i], &m, &o))
        return i;

      *all_opacity &= value->filters[i].type == BOBGUI_CSS_FILTER_OPACITY;
      graphene_matrix_multiply (matrix, &m, &m2);
      graphene_matrix_transform_vec4 (&m, offset, &o2);

      graphene_matrix_init_from_matrix (matrix, &m2);
      graphene_vec4_add (&o, &o2, offset);
    }

  return value->n_filters;
}

static void
bobgui_css_value_filter_free (BobguiCssValue *value)
{
  guint i;

  for (i = 0; i < value->n_filters; i++)
    bobgui_css_filter_clear (&value->filters[i]);

  g_free (value);
}

/* returns TRUE if dest == src */
static gboolean
bobgui_css_filter_compute (BobguiCssFilter         *dest,
                        BobguiCssFilter         *src,
                        guint                 property_id,
                        BobguiCssComputeContext *context)
{
  dest->type = src->type;

  switch (src->type)
    {
    case BOBGUI_CSS_FILTER_BRIGHTNESS:
      dest->brightness.value = bobgui_css_value_compute (src->brightness.value, property_id, context);
      return dest->brightness.value == src->brightness.value;

    case BOBGUI_CSS_FILTER_CONTRAST:
      dest->contrast.value = bobgui_css_value_compute (src->contrast.value, property_id, context);
      return dest->contrast.value == src->contrast.value;

    case BOBGUI_CSS_FILTER_GRAYSCALE:
      dest->grayscale.value = bobgui_css_value_compute (src->grayscale.value, property_id, context);
      return dest->grayscale.value == src->grayscale.value;

    case BOBGUI_CSS_FILTER_HUE_ROTATE:
      dest->hue_rotate.value = bobgui_css_value_compute (src->hue_rotate.value, property_id, context);
      return dest->hue_rotate.value == src->hue_rotate.value;

    case BOBGUI_CSS_FILTER_INVERT:
      dest->invert.value = bobgui_css_value_compute (src->invert.value, property_id, context);
      return dest->invert.value == src->invert.value;

    case BOBGUI_CSS_FILTER_OPACITY:
      dest->opacity.value = bobgui_css_value_compute (src->opacity.value, property_id, context);
      return dest->opacity.value == src->opacity.value;

    case BOBGUI_CSS_FILTER_SATURATE:
      dest->saturate.value = bobgui_css_value_compute (src->saturate.value, property_id, context);
      return dest->saturate.value == src->saturate.value;

    case BOBGUI_CSS_FILTER_SEPIA:
      dest->sepia.value = bobgui_css_value_compute (src->sepia.value, property_id, context);
      return dest->sepia.value == src->sepia.value;

    case BOBGUI_CSS_FILTER_BLUR:
      dest->blur.value = bobgui_css_value_compute (src->blur.value, property_id, context);
      return dest->blur.value == src->blur.value;

    case BOBGUI_CSS_FILTER_DROP_SHADOW:
      dest->drop_shadow.value = bobgui_css_value_compute (src->drop_shadow.value, property_id, context);
      return dest->drop_shadow.value == src->drop_shadow.value;

    case BOBGUI_CSS_FILTER_SVG:
      if (src->svg.ref)
        {
          dest->svg.ref = g_strdup (src->svg.ref);
          dest->svg.url = g_strdup (src->svg.url);
          dest->svg.svg = g_object_ref (src->svg.svg);
        }
      return TRUE;

    case BOBGUI_CSS_FILTER_NONE:
    default:
      g_assert_not_reached ();
      return FALSE;
    }
}

static BobguiCssValue *
bobgui_css_value_filter_compute (BobguiCssValue          *value,
                              guint                 property_id,
                              BobguiCssComputeContext *context)
{
  BobguiCssValue *result;
  gboolean changes;
  guint i;

  /* Special case the 99% case of "none" */
  if (bobgui_css_filter_value_is_none (value))
    return bobgui_css_value_ref (value);

  changes = FALSE;
  result = bobgui_css_filter_value_alloc (value->n_filters);

  for (i = 0; i < value->n_filters; i++)
    {
      changes |= !bobgui_css_filter_compute (&result->filters[i],
                                          &value->filters[i],
                                          property_id,
                                          context);
    }

  if (!changes)
    {
      bobgui_css_value_unref (result);
      result = bobgui_css_value_ref (value);
    }

  return result;
}

static gboolean
bobgui_css_filter_equal (const BobguiCssFilter *filter1,
                      const BobguiCssFilter *filter2)
{
  if (filter1->type != filter2->type)
    return FALSE;

  switch (filter1->type)
    {
    case BOBGUI_CSS_FILTER_BRIGHTNESS:
      return bobgui_css_value_equal (filter1->brightness.value, filter2->brightness.value);

    case BOBGUI_CSS_FILTER_CONTRAST:
      return bobgui_css_value_equal (filter1->contrast.value, filter2->contrast.value);

    case BOBGUI_CSS_FILTER_GRAYSCALE:
      return bobgui_css_value_equal (filter1->grayscale.value, filter2->grayscale.value);

    case BOBGUI_CSS_FILTER_HUE_ROTATE:
      return bobgui_css_value_equal (filter1->hue_rotate.value, filter2->hue_rotate.value);

    case BOBGUI_CSS_FILTER_INVERT:
      return bobgui_css_value_equal (filter1->invert.value, filter2->invert.value);

    case BOBGUI_CSS_FILTER_OPACITY:
      return bobgui_css_value_equal (filter1->opacity.value, filter2->opacity.value);

    case BOBGUI_CSS_FILTER_SATURATE:
      return bobgui_css_value_equal (filter1->saturate.value, filter2->saturate.value);

    case BOBGUI_CSS_FILTER_SEPIA:
      return bobgui_css_value_equal (filter1->sepia.value, filter2->sepia.value);

    case BOBGUI_CSS_FILTER_BLUR:
      return bobgui_css_value_equal (filter1->blur.value, filter2->blur.value);

    case BOBGUI_CSS_FILTER_DROP_SHADOW:
      return bobgui_css_value_equal (filter1->drop_shadow.value, filter2->drop_shadow.value);

    case BOBGUI_CSS_FILTER_SVG:
      return g_strcmp0 (filter1->svg.url, filter2->svg.url) == 0 &&
             g_strcmp0 (filter1->svg.ref, filter2->svg.ref) == 0;

    case BOBGUI_CSS_FILTER_NONE:
    default:
      g_assert_not_reached ();
      return FALSE;
    }
}

static gboolean
bobgui_css_value_filter_equal (const BobguiCssValue *value1,
                            const BobguiCssValue *value2)
{
  const BobguiCssValue *larger;
  guint i, n;

  n = MIN (value1->n_filters, value2->n_filters);
  for (i = 0; i < n; i++)
    {
      if (!bobgui_css_filter_equal (&value1->filters[i], &value2->filters[i]))
        return FALSE;
    }

  larger = value1->n_filters > value2->n_filters ? value1 : value2;

  for (; i < larger->n_filters; i++)
    {
      BobguiCssFilter filter;

      bobgui_css_filter_init_identity (&filter, &larger->filters[i]);

      if (!bobgui_css_filter_equal (&larger->filters[i], &filter))
        {
          bobgui_css_filter_clear (&filter);
          return FALSE;
        }

      bobgui_css_filter_clear (&filter);
    }

  return TRUE;
}

static void
bobgui_css_filter_transition (BobguiCssFilter       *result,
                           const BobguiCssFilter *start,
                           const BobguiCssFilter *end,
                           guint               property_id,
                           double              progress)
{
  result->type = start->type;

  switch (start->type)
    {
    case BOBGUI_CSS_FILTER_BRIGHTNESS:
      result->brightness.value = bobgui_css_value_transition (start->brightness.value, end->brightness.value, property_id, progress);
      break;

    case BOBGUI_CSS_FILTER_CONTRAST:
      result->contrast.value = bobgui_css_value_transition (start->contrast.value, end->contrast.value, property_id, progress);
      break;

    case BOBGUI_CSS_FILTER_GRAYSCALE:
      result->grayscale.value = bobgui_css_value_transition (start->grayscale.value, end->grayscale.value, property_id, progress);
      break;

    case BOBGUI_CSS_FILTER_HUE_ROTATE:
      result->hue_rotate.value = bobgui_css_value_transition (start->hue_rotate.value, end->hue_rotate.value, property_id, progress);
      break;

    case BOBGUI_CSS_FILTER_INVERT:
      result->invert.value = bobgui_css_value_transition (start->invert.value, end->invert.value, property_id, progress);
      break;

    case BOBGUI_CSS_FILTER_OPACITY:
      result->opacity.value = bobgui_css_value_transition (start->opacity.value, end->opacity.value, property_id, progress);
      break;

    case BOBGUI_CSS_FILTER_SATURATE:
      result->saturate.value = bobgui_css_value_transition (start->saturate.value, end->saturate.value, property_id, progress);
      break;

    case BOBGUI_CSS_FILTER_SEPIA:
      result->sepia.value = bobgui_css_value_transition (start->sepia.value, end->sepia.value, property_id, progress);
      break;

    case BOBGUI_CSS_FILTER_BLUR:
      result->blur.value = bobgui_css_value_transition (start->blur.value, end->blur.value, property_id, progress);
      break;

    case BOBGUI_CSS_FILTER_DROP_SHADOW:
      result->drop_shadow.value = bobgui_css_value_transition (start->drop_shadow.value, end->drop_shadow.value, property_id, progress);
      break;

    case BOBGUI_CSS_FILTER_SVG:
      if (progress < 0.5)
        {
          result->svg.ref = g_strdup (start->svg.ref);
          result->svg.url = g_strdup (start->svg.url);
          result->svg.svg = g_object_ref (start->svg.svg);
        }
      else
        {
          result->svg.ref = g_strdup (end->svg.ref);
          result->svg.url = g_strdup (end->svg.url);
          result->svg.svg = g_object_ref (end->svg.svg);
        }
      break;

    case BOBGUI_CSS_FILTER_NONE:
    default:
      g_assert_not_reached ();
      break;
    }
}

static BobguiCssValue *
bobgui_css_value_filter_transition (BobguiCssValue *start,
                                 BobguiCssValue *end,
                                 guint        property_id,
                                 double       progress)
{
  BobguiCssValue *result;
  guint i, n;

  if (bobgui_css_filter_value_is_none (start))
    {
      if (bobgui_css_filter_value_is_none (end))
        return bobgui_css_value_ref (start);

      n = 0;
    }
  else if (bobgui_css_filter_value_is_none (end))
    {
      n = 0;
    }
  else
    {
      n = MIN (start->n_filters, end->n_filters);
    }

  /* Check filters are compatible. If not, transition between
   * their result matrices.
   */
  for (i = 0; i < n; i++)
    {
      if (start->filters[i].type != end->filters[i].type)
        {
          /* XXX: can we improve this? */
          return NULL;
        }
    }

  result = bobgui_css_filter_value_alloc (MAX (start->n_filters, end->n_filters));

  for (i = 0; i < n; i++)
    {
      bobgui_css_filter_transition (&result->filters[i],
                                 &start->filters[i],
                                 &end->filters[i],
                                 property_id,
                                 progress);
    }

  for (; i < start->n_filters; i++)
    {
      BobguiCssFilter filter;

      bobgui_css_filter_init_identity (&filter, &start->filters[i]);
      bobgui_css_filter_transition (&result->filters[i],
                                 &start->filters[i],
                                 &filter,
                                 property_id,
                                 progress);
      bobgui_css_filter_clear (&filter);
    }
  for (; i < end->n_filters; i++)
    {
      BobguiCssFilter filter;

      bobgui_css_filter_init_identity (&filter, &end->filters[i]);
      bobgui_css_filter_transition (&result->filters[i],
                                 &filter,
                                 &end->filters[i],
                                 property_id,
                                 progress);
      bobgui_css_filter_clear (&filter);
    }

  g_assert (i == MAX (start->n_filters, end->n_filters));

  return result;
}

static void
bobgui_css_filter_print (const BobguiCssFilter *filter,
                      GString            *string)
{
  switch (filter->type)
    {
    case BOBGUI_CSS_FILTER_BRIGHTNESS:
      g_string_append (string, "brightness(");
      bobgui_css_value_print (filter->brightness.value, string);
      g_string_append (string, ")");
      break;

    case BOBGUI_CSS_FILTER_CONTRAST:
      g_string_append (string, "contrast(");
      bobgui_css_value_print (filter->contrast.value, string);
      g_string_append (string, ")");
      break;

    case BOBGUI_CSS_FILTER_GRAYSCALE:
      g_string_append (string, "grayscale(");
      bobgui_css_value_print (filter->grayscale.value, string);
      g_string_append (string, ")");
      break;

    case BOBGUI_CSS_FILTER_HUE_ROTATE:
      g_string_append (string, "hue-rotate(");
      bobgui_css_value_print (filter->hue_rotate.value, string);
      g_string_append (string, ")");
      break;

    case BOBGUI_CSS_FILTER_INVERT:
      g_string_append (string, "invert(");
      bobgui_css_value_print (filter->invert.value, string);
      g_string_append (string, ")");
      break;

    case BOBGUI_CSS_FILTER_OPACITY:
      g_string_append (string, "opacity(");
      bobgui_css_value_print (filter->opacity.value, string);
      g_string_append (string, ")");
      break;

    case BOBGUI_CSS_FILTER_SATURATE:
      g_string_append (string, "saturate(");
      bobgui_css_value_print (filter->saturate.value, string);
      g_string_append (string, ")");
      break;

    case BOBGUI_CSS_FILTER_SEPIA:
      g_string_append (string, "sepia(");
      bobgui_css_value_print (filter->sepia.value, string);
      g_string_append (string, ")");
      break;

    case BOBGUI_CSS_FILTER_BLUR:
      g_string_append (string, "blur(");
      bobgui_css_value_print (filter->blur.value, string);
      g_string_append (string, ")");
      break;

    case BOBGUI_CSS_FILTER_DROP_SHADOW:
      g_string_append (string, "drop-shadow(");
      bobgui_css_value_print (filter->drop_shadow.value, string);
      g_string_append (string, ")");
      break;

    case BOBGUI_CSS_FILTER_SVG:
      g_string_append_printf (string, "url(\"%s#%s\")", filter->svg.url, filter->svg.ref);
      break;

    case BOBGUI_CSS_FILTER_NONE:
    default:
      g_assert_not_reached ();
      break;
    }
}

static void
bobgui_css_value_filter_print (const BobguiCssValue *value,
                            GString           *string)
{
  guint i;

  if (bobgui_css_filter_value_is_none (value))
    {
      g_string_append (string, "none");
      return;
    }

  for (i = 0; i < value->n_filters; i++)
    {
      if (i > 0)
        g_string_append_c (string, ' ');

      bobgui_css_filter_print (&value->filters[i], string);
    }
}

static const BobguiCssValueClass BOBGUI_CSS_VALUE_FILTER = {
  "BobguiCssFilterValue",
  bobgui_css_value_filter_free,
  bobgui_css_value_filter_compute,
  NULL,
  bobgui_css_value_filter_equal,
  bobgui_css_value_filter_transition,
  NULL,
  NULL,
  bobgui_css_value_filter_print
};

static BobguiCssValue filter_none_singleton = { &BOBGUI_CSS_VALUE_FILTER, 1, 1, 0, 0, 0, {  { BOBGUI_CSS_FILTER_NONE } } };

static BobguiCssValue *
bobgui_css_filter_value_alloc (guint n_filters)
{
  BobguiCssValue *result;

  g_return_val_if_fail (n_filters > 0, NULL);

  result = bobgui_css_value_alloc (&BOBGUI_CSS_VALUE_FILTER, sizeof (BobguiCssValue) + sizeof (BobguiCssFilter) * (n_filters - 1));
  result->n_filters = n_filters;

  return result;
}

BobguiCssValue *
bobgui_css_filter_value_new_none (void)
{
  return bobgui_css_value_ref (&filter_none_singleton);
}

gboolean
bobgui_css_filter_value_is_none (const BobguiCssValue *value)
{
  return value->n_filters == 0;
}

static guint
bobgui_css_filter_parse_number (BobguiCssParser *parser,
                             guint         n,
                             gpointer      data)
{
  BobguiCssValue **values = data;

  values[n] = bobgui_css_number_value_parse (parser, BOBGUI_CSS_PARSE_NUMBER | BOBGUI_CSS_PARSE_PERCENT | BOBGUI_CSS_POSITIVE_ONLY);
  if (values[n] == NULL)
    return 0;

  return 1;
}

static guint
bobgui_css_filter_parse_length (BobguiCssParser *parser,
                             guint         n,
                             gpointer      data)
{
  BobguiCssValue **values = data;

  values[n] = bobgui_css_number_value_parse (parser, BOBGUI_CSS_PARSE_LENGTH | BOBGUI_CSS_POSITIVE_ONLY);
  if (values[n] == NULL)
    return 0;

  return 1;
}

static guint
bobgui_css_filter_parse_angle (BobguiCssParser *parser,
                            guint         n,
                            gpointer      data)
{
  BobguiCssValue **values = data;

  values[n] = bobgui_css_number_value_parse (parser, BOBGUI_CSS_PARSE_ANGLE);
  if (values[n] == NULL)
    return 0;

  return 1;
}

static guint
bobgui_css_filter_parse_shadow (BobguiCssParser *parser,
                             guint         n,
                             gpointer      data)
{
  BobguiCssValue **values = data;

  values[n] = bobgui_css_shadow_value_parse_filter (parser);
  if (values[n] == NULL)
    return 0;

  return 1;
}

typedef struct
{
  BobguiCssParser *parser;
  gboolean is_data;
  BobguiCssLocation start, end;
} ParserErrorData;

static void
css_location_update (BobguiCssLocation *l,
                     int             bytes,
                     int             chars)
{
  l->bytes += bytes;
  l->chars += chars;
  l->line_bytes += bytes;
  l->line_chars += chars;
}

static void
svg_error_cb (BobguiSvg          *svg,
              const GError    *svg_error,
              ParserErrorData *d)
{
  BobguiCssLocation start = d->start;
  BobguiCssLocation end = d->end;

#if 0
  /* GMarkup error locations are not good enough for this :( */
  if (d->is_data && svg_error->domain == BOBGUI_SVG_ERROR)
    {
      const BobguiSvgLocation *s = bobgui_svg_error_get_start (svg_error);
      const BobguiSvgLocation *e = bobgui_svg_error_get_end (svg_error);

      start = end = d->start;
      css_location_update (&start, s->line_chars, e->line_chars);
      css_location_update (&end, e->line_chars, e->line_chars);
    }
#endif

  bobgui_css_parser_error (d->parser,
                        BOBGUI_CSS_PARSER_ERROR_SYNTAX,
                        &start, &end,
                        "%s", svg_error->message);
}

BobguiCssValue *
bobgui_css_filter_value_parse (BobguiCssParser *parser)
{
  BobguiCssValue *value;
  GArray *array;
  guint i;
  gboolean computed = TRUE;

  if (bobgui_css_parser_try_ident (parser, "none"))
    return bobgui_css_filter_value_new_none ();

  array = g_array_new (FALSE, FALSE, sizeof (BobguiCssFilter));

  while (TRUE)
    {
      BobguiCssFilter filter;

      if (bobgui_css_parser_has_function (parser, "blur"))
        {
          if (!bobgui_css_parser_consume_function (parser, 1, 1, bobgui_css_filter_parse_length, &filter.blur.value))
            goto fail;

          filter.type = BOBGUI_CSS_FILTER_BLUR;
          computed = computed && bobgui_css_value_is_computed (filter.blur.value);
        }
      else if (bobgui_css_parser_has_function (parser, "brightness"))
        {
          if (!bobgui_css_parser_consume_function (parser, 1, 1, bobgui_css_filter_parse_number, &filter.brightness.value))
            goto fail;

          filter.type = BOBGUI_CSS_FILTER_BRIGHTNESS;
          computed = computed && bobgui_css_value_is_computed (filter.brightness.value);
        }
      else if (bobgui_css_parser_has_function (parser, "contrast"))
        {
          if (!bobgui_css_parser_consume_function (parser, 1, 1, bobgui_css_filter_parse_number, &filter.contrast.value))
            goto fail;

          filter.type = BOBGUI_CSS_FILTER_CONTRAST;
          computed = computed && bobgui_css_value_is_computed (filter.contrast.value);
        }
      else if (bobgui_css_parser_has_function (parser, "grayscale"))
        {
          if (!bobgui_css_parser_consume_function (parser, 1, 1, bobgui_css_filter_parse_number, &filter.grayscale.value))
            goto fail;

          filter.type = BOBGUI_CSS_FILTER_GRAYSCALE;
          computed = computed && bobgui_css_value_is_computed (filter.grayscale.value);
        }
      else if (bobgui_css_parser_has_function (parser, "hue-rotate"))
        {
          if (!bobgui_css_parser_consume_function (parser, 1, 1, bobgui_css_filter_parse_angle, &filter.hue_rotate.value))
            goto fail;

          filter.type = BOBGUI_CSS_FILTER_HUE_ROTATE;
          computed = computed && bobgui_css_value_is_computed (filter.hue_rotate.value);
        }
      else if (bobgui_css_parser_has_function (parser, "invert"))
        {
          if (!bobgui_css_parser_consume_function (parser, 1, 1, bobgui_css_filter_parse_number, &filter.invert.value))
            goto fail;

          filter.type = BOBGUI_CSS_FILTER_INVERT;
          computed = computed && bobgui_css_value_is_computed (filter.invert.value);
        }
      else if (bobgui_css_parser_has_function (parser, "opacity"))
        {
          if (!bobgui_css_parser_consume_function (parser, 1, 1, bobgui_css_filter_parse_number, &filter.opacity.value))
            goto fail;

          filter.type = BOBGUI_CSS_FILTER_OPACITY;
          computed = computed && bobgui_css_value_is_computed (filter.opacity.value);
        }
      else if (bobgui_css_parser_has_function (parser, "saturate"))
        {
          if (!bobgui_css_parser_consume_function (parser, 1, 1, bobgui_css_filter_parse_number, &filter.saturate.value))
            goto fail;

          filter.type = BOBGUI_CSS_FILTER_SATURATE;
          computed = computed && bobgui_css_value_is_computed (filter.saturate.value);
        }
      else if (bobgui_css_parser_has_function (parser, "sepia"))
        {
          if (!bobgui_css_parser_consume_function (parser, 1, 1, bobgui_css_filter_parse_number, &filter.sepia.value))
            goto fail;

          filter.type = BOBGUI_CSS_FILTER_SEPIA;
          computed = computed && bobgui_css_value_is_computed (filter.sepia.value);
        }
      else if (bobgui_css_parser_has_function (parser, "drop-shadow"))
        {
          if (!bobgui_css_parser_consume_function (parser, 1, 1, bobgui_css_filter_parse_shadow, &filter.drop_shadow.value))
            goto fail;

          filter.type = BOBGUI_CSS_FILTER_DROP_SHADOW;
          computed = computed && bobgui_css_value_is_computed (filter.drop_shadow.value);
        }
      else if (bobgui_css_parser_has_url (parser))
        {
          BobguiCssLocation start, end;
          char *url;
          char *scheme = NULL;
          char *path = NULL;
          GBytes *bytes = NULL;
          char *fragment = NULL;
          GError *error = NULL;
          unsigned long signal_id;
          gboolean is_data;
          int len;

          start = *bobgui_css_parser_get_start_location (parser);

          url = bobgui_css_parser_consume_url (parser);
          if (!url)
            goto fail;

          end = *bobgui_css_parser_get_end_location (parser);

          len = strlen ("url(\"");
          css_location_update (&start, len, len);
          len = strlen ("\")");
          css_location_update (&end, - len, - len);

          g_uri_split (url, 0, &scheme, NULL, NULL, NULL, &path, NULL, &fragment, NULL);
          if (!fragment)
            {
              g_set_error (&error,
                           BOBGUI_CSS_PARSER_ERROR,
                           BOBGUI_CSS_PARSER_ERROR_UNKNOWN_VALUE,
                           "Filter url without fragment ID");
              bobgui_css_parser_emit_error (parser, &start, &end, error);
              g_error_free (error);
              g_free (scheme);
              g_free (path);
              g_free (url);
              goto fail;
            }

          url[strlen (url) - (strlen (fragment) + 1)] = '\0';

          is_data = scheme && g_ascii_strcasecmp (scheme, "data") == 0;

          if (is_data)
            {
              char *mimetype = NULL;

              bytes = bobgui_css_data_url_parse (url, &mimetype, &error);

              if (mimetype && strcmp (mimetype, "image/svg+xml") != 0)
                {
                  g_bytes_unref (bytes);
                  g_set_error (&error,
                               BOBGUI_CSS_PARSER_ERROR,
                               BOBGUI_CSS_PARSER_ERROR_UNKNOWN_VALUE,
                               "Filter url contains non-SVG data");
                  g_clear_pointer (&bytes, g_bytes_unref);
                }

              g_free (mimetype);

              if (bytes)
                {
                  len = strchr (url, ',') - url;
                  css_location_update (&start, len, len);
                  len = strlen (fragment) - 1;
                  css_location_update (&end, - len, - len);
                }
            }
          else
            {
              GFile *file;

              file = bobgui_css_parser_resolve_url (parser, url);
              if (!file)
                file = g_file_new_for_path (path);

              bytes = g_file_load_bytes (file, NULL, NULL, &error);
              g_object_unref (file);
            }

          g_free (scheme);
          g_free (path);

          if (!bytes)
            {
              bobgui_css_parser_emit_error (parser, &start, &end, error);
              g_error_free (error);
              g_free (fragment);
              g_free (url);
              goto fail;
            }

          /* Don't allow animations and gpa extensions for now,
           * to avoid complications
           */
          filter.svg.svg = bobgui_svg_new ();
          bobgui_svg_set_features (filter.svg.svg,
                                BOBGUI_SVG_SYSTEM_RESOURCES & BOBGUI_SVG_EXTERNAL_RESOURCES);
          signal_id = g_signal_connect (filter.svg.svg, "error",
                                        G_CALLBACK (svg_error_cb),
                                        (&(ParserErrorData) { parser, is_data, start, end }));
          bobgui_svg_load_from_bytes (filter.svg.svg, bytes);
          g_signal_handler_disconnect (filter.svg.svg, signal_id);
          filter.svg.ref = fragment;
          filter.svg.url = url;
          filter.svg.type = BOBGUI_CSS_FILTER_SVG;
        }
      else
        {
          break;
        }

      g_array_append_val (array, filter);
    }

  if (array->len == 0)
    {
      bobgui_css_parser_error_syntax (parser, "Expected a filter");
      goto fail;
    }

  value = bobgui_css_filter_value_alloc (array->len);
  memcpy (value->filters, array->data, sizeof (BobguiCssFilter) * array->len);
  value->is_computed = computed;

  g_array_free (array, TRUE);

  return value;

fail:
  for (i = 0; i < array->len; i++)
    {
      bobgui_css_filter_clear (&g_array_index (array, BobguiCssFilter, i));
    }
  g_array_free (array, TRUE);
  return NULL;
}

/* Returns: extra size due to blur radii */
double
bobgui_css_filter_value_push_snapshot (const BobguiCssValue *filter,
                                    BobguiSnapshot       *snapshot)
{
  graphene_matrix_t matrix;
  graphene_vec4_t offset;
  gboolean all_opacity;
  double extra_size;
  int i, j;

  if (bobgui_css_filter_value_is_none (filter))
    return 0;

  extra_size = 0;
  i = 0;
  while (i < filter->n_filters)
    {
      j = bobgui_css_filter_value_compute_matrix (filter, i, &matrix, &offset, &all_opacity);
      if (i < j)
        {
          if (all_opacity)
            bobgui_snapshot_push_opacity (snapshot, graphene_matrix_get_value (&matrix, 3, 3));
          else
            bobgui_snapshot_push_color_matrix (snapshot, &matrix, &offset);
        }


      if (j < filter->n_filters)
        {
          if (filter->filters[j].type == BOBGUI_CSS_FILTER_BLUR)
            {
              double std_dev = bobgui_css_number_value_get (filter->filters[j].blur.value, 100.0);
              bobgui_snapshot_push_blur (snapshot, 2 * std_dev);
              extra_size += gsk_cairo_blur_compute_pixels (std_dev);
            }
          else if (filter->filters[j].type == BOBGUI_CSS_FILTER_DROP_SHADOW)
            {
              bobgui_css_shadow_value_push_snapshot (filter->filters[j].drop_shadow.value, snapshot);
            }
          else if (filter->filters[j].type == BOBGUI_CSS_FILTER_SVG)
            {
              bobgui_snapshot_push_collect (snapshot);
            }
          else
            g_warning ("Don't know how to handle filter type %d", filter->filters[j].type);
        }

      i = j + 1;
    }

  return extra_size;
}

void
bobgui_css_filter_value_pop_snapshot (const BobguiCssValue     *filter,
                                   const graphene_rect_t *bounds,
                                   BobguiSnapshot           *snapshot)
{
  int i, j;

  if (bobgui_css_filter_value_is_none (filter))
    return;

  i = 0;
  while (i < filter->n_filters)
    {
      for (j = i; j < filter->n_filters; j++)
        {
          if (filter->filters[j].type == BOBGUI_CSS_FILTER_BLUR ||
              filter->filters[j].type == BOBGUI_CSS_FILTER_DROP_SHADOW ||
              filter->filters[j].type == BOBGUI_CSS_FILTER_SVG)
            break;
        }

      if (i < j)
        bobgui_snapshot_pop (snapshot);

      if (j < filter->n_filters)
        {
          if (filter->filters[j].type == BOBGUI_CSS_FILTER_BLUR)
            bobgui_snapshot_pop (snapshot);
          else if (filter->filters[j].type == BOBGUI_CSS_FILTER_DROP_SHADOW)
            bobgui_css_shadow_value_pop_snapshot (filter->filters[j].drop_shadow.value, snapshot);
          else if (filter->filters[j].type == BOBGUI_CSS_FILTER_SVG)
            {
              GskRenderNode *source, *node;

              source = bobgui_snapshot_pop_collect (snapshot);
              node = bobgui_svg_apply_filter (filter->filters[j].svg.svg,
                                           filter->filters[j].svg.ref,
                                           bounds,
                                           source);
              bobgui_snapshot_append_node (snapshot, node);
              gsk_render_node_unref (node);
              gsk_render_node_unref (source);
            }
        }

      i = j + 1;
    }
}
