/*
 * Copyright © 2015 Red Hat Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Matthias Clasen <mclasen@redhat.com>
 */

#include "config.h"

#include "bobguicssimageradialprivate.h"

#include <math.h>

#include "bobguicsscolorvalueprivate.h"
#include "bobguicssnumbervalueprivate.h"
#include "bobguicsspositionvalueprivate.h"
#include "bobguicsscolorvalueprivate.h"
#include "bobguicssprovider.h"
#include "bobguisnapshotprivate.h"

G_DEFINE_TYPE (BobguiCssImageRadial, _bobgui_css_image_radial, BOBGUI_TYPE_CSS_IMAGE)

static void
bobgui_css_image_radial_get_start_end (BobguiCssImageRadial *radial,
                                    double             radius,
                                    double            *start,
                                    double            *end)
{
  const BobguiCssImageRadialColorStop *stop;
  double pos;
  guint i;

  if (radial->repeating)
    {
      stop = &radial->color_stops[0];
      if (stop->offset == NULL)
        *start = 0;
      else
        *start = bobgui_css_number_value_get (stop->offset, radius) / radius;

      *end = *start;

      for (i = 0; i < radial->n_stops; i++)
        {
          stop = &radial->color_stops[i];

          if (stop->offset == NULL)
            continue;

          pos = bobgui_css_number_value_get (stop->offset, radius) / radius;

          *end = MAX (pos, *end);
        }

      if (stop->offset == NULL)
        *end = MAX (*end, 1.0);
    }
  else
    {
      *start = 0;
      *end = 1;
    }
}

static void
bobgui_css_image_radial_snapshot (BobguiCssImage *image,
                               BobguiSnapshot *snapshot,
                               double       width,
                               double       height)
{
  BobguiCssImageRadial *radial = BOBGUI_CSS_IMAGE_RADIAL (image);
  GskGradientStop *stops;
  double x, y;
  double hradius, vradius;
  double start, end;
  double r1, r2, r3, r4, r;
  double offset, hint;
  int i, last;
  GskGradient *gradient;

  x = _bobgui_css_position_value_get_x (radial->position, width);
  y = _bobgui_css_position_value_get_y (radial->position, height);

  if (radial->circle)
    {
      switch (radial->size)
        {
        case BOBGUI_CSS_EXPLICIT_SIZE:
          hradius = bobgui_css_number_value_get (radial->sizes[0], width);
          break;
        case BOBGUI_CSS_CLOSEST_SIDE:
          hradius = MIN (MIN (x, width - x), MIN (y, height - y));
          break;
        case BOBGUI_CSS_FARTHEST_SIDE:
          hradius = MAX (MAX (x, width - x), MAX (y, height - y));
          break;
        case BOBGUI_CSS_CLOSEST_CORNER:
        case BOBGUI_CSS_FARTHEST_CORNER:
          r1 = x*x + y*y;
          r2 = x*x + (height - y)*(height - y);
          r3 = (width - x)*(width - x) + y*y;
          r4 = (width - x)*(width - x) + (height - y)*(height - y);
          if (radial->size == BOBGUI_CSS_CLOSEST_CORNER)
            r = MIN ( MIN (r1, r2), MIN (r3, r4));
          else
            r = MAX ( MAX (r1, r2), MAX (r3, r4));
          hradius = sqrt (r);
          break;
        default:
          g_assert_not_reached ();
        }

      hradius = MAX (1.0, hradius);
      vradius = hradius;
    }
  else
    {
      switch (radial->size)
        {
        case BOBGUI_CSS_EXPLICIT_SIZE:
          hradius = bobgui_css_number_value_get (radial->sizes[0], width);
          vradius = bobgui_css_number_value_get (radial->sizes[1], height);
          break;
        case BOBGUI_CSS_CLOSEST_SIDE:
          hradius = MIN (x, width - x);
          vradius = MIN (y, height - y);
          break;
        case BOBGUI_CSS_FARTHEST_SIDE:
          hradius = MAX (x, width - x);
          vradius = MAX (y, height - y);
          break;
        case BOBGUI_CSS_CLOSEST_CORNER:
          hradius = M_SQRT2 * MIN (x, width - x);
          vradius = M_SQRT2 * MIN (y, height - y);
          break;
        case BOBGUI_CSS_FARTHEST_CORNER:
          hradius = M_SQRT2 * MAX (x, width - x);
          vradius = M_SQRT2 * MAX (y, height - y);
          break;
        default:
          g_assert_not_reached ();
        }

      hradius = MAX (1.0, hradius);
      vradius = MAX (1.0, vradius);
    }

  bobgui_css_image_radial_get_start_end (radial, hradius, &start, &end);

  offset = start;
  last = -1;
  stops = g_newa (GskGradientStop, radial->n_stops);
  hint = start;

  for (i = 0; i < radial->n_stops; i++)
    {
      const BobguiCssImageRadialColorStop *stop = &radial->color_stops[i];
      double pos, step;

      if (stop->offset == NULL)
        {
          if (stop->transition_hint)
            {
              hint = MAX (hint, bobgui_css_number_value_get (stop->transition_hint, hradius) / hradius);
              hint = CLAMP (hint, 0.0, 1.0);
            }

          if (i == 0)
            pos = 0.0;
          else if (i + 1 == radial->n_stops)
            pos = 1.0;
          else
            continue;
        }
      else
        pos = MIN (1.0, bobgui_css_number_value_get (stop->offset, hradius) / hradius);

      pos = MAX (pos, hint);
      pos = MAX (pos, offset);
      step = (pos - offset) / (i - last);
      for (last = last + 1; last <= i; last++)
        {
          stop = &radial->color_stops[last];

          offset += step;

          stops[last].offset = (offset - start) / (end - start);
          bobgui_css_color_to_color (bobgui_css_color_value_get_color (stop->color), &stops[last].color);

          if (last > 0 && stop->transition_hint)
            {
              hint = bobgui_css_number_value_get (stop->transition_hint, hradius) / hradius;
              hint = CLAMP (hint, 0.0, 1.0);
              stops[last].transition_hint = (hint - stops[last - 1].offset) / (stops[last].offset - stops[last - 1].offset);
            }
          else
            {
              stops[last].transition_hint = 0.5;
            }
        }

      offset = pos;
      last = i;
    }

  gradient = gsk_gradient_new ();
  for (i = 0; i < radial->n_stops; i++)
    gsk_gradient_add_stop (gradient, stops[i].offset, stops[i].transition_hint, &stops[i].color);

  if (radial->color_space != BOBGUI_CSS_COLOR_SPACE_SRGB)
    g_warning_once ("Gradient interpolation color spaces are not supported yet");

  gsk_gradient_set_interpolation (gradient, bobgui_css_color_space_get_color_state (radial->color_space));
  gsk_gradient_set_hue_interpolation (gradient, bobgui_css_hue_interpolation_to_hue_interpolation (radial->hue_interp));
  gsk_gradient_set_repeat (gradient, radial->repeating ? GSK_REPEAT_REPEAT : GSK_REPEAT_PAD);

  bobgui_snapshot_add_radial_gradient (snapshot,
                                    &GRAPHENE_RECT_INIT (0, 0, width, height),
                                    &GRAPHENE_POINT_INIT (x, y), hradius * start,
                                    &GRAPHENE_POINT_INIT (x, y), hradius * end,
                                    hradius / vradius,
                                    gradient);

  for (i = 0; i < radial->n_stops; i++)
    gdk_color_finish (&stops[i].color);

  gsk_gradient_free (gradient);
}

static guint
bobgui_css_image_radial_parse_color_stop (BobguiCssImageRadial *radial,
                                       BobguiCssParser      *parser,
                                       GArray            *stop_array)
{
  BobguiCssImageRadialColorStop stop;

  if (bobgui_css_number_value_can_parse (parser))
    {
      stop.transition_hint = bobgui_css_number_value_parse (parser,
                                                         BOBGUI_CSS_PARSE_PERCENT
                                                         | BOBGUI_CSS_PARSE_LENGTH);
      if (stop.transition_hint == NULL)
        return 0;

      if (!bobgui_css_parser_has_token (parser, BOBGUI_CSS_TOKEN_COMMA))
        return 0;

      bobgui_css_parser_consume_token (parser);
    }
  else
    {
      stop.transition_hint = NULL;
    }

  stop.color = bobgui_css_color_value_parse (parser);
  if (stop.color == NULL)
    {
      if (stop.transition_hint)
        bobgui_css_value_unref (stop.transition_hint);
      bobgui_css_parser_error_syntax (parser, "Expected color stop to contain a color");
      return 0;
    }

  if (bobgui_css_number_value_can_parse (parser))
    {
      stop.offset = bobgui_css_number_value_parse (parser,
                                                BOBGUI_CSS_PARSE_PERCENT
                                                | BOBGUI_CSS_PARSE_LENGTH);
      if (stop.offset == NULL)
        {
          if (stop.transition_hint)
            bobgui_css_value_unref (stop.transition_hint);
          bobgui_css_value_unref (stop.color);
          return 0;
        }

      g_array_append_val (stop_array, stop);

      if (bobgui_css_number_value_can_parse (parser))
        {
          stop.transition_hint = NULL;
          stop.color = bobgui_css_value_ref (stop.color);

          stop.offset = bobgui_css_number_value_parse (parser,
                                                    BOBGUI_CSS_PARSE_PERCENT
                                                    | BOBGUI_CSS_PARSE_LENGTH);
          if (stop.offset == NULL)
            {
              bobgui_css_value_unref (stop.color);
              return 0;
            }

          g_array_append_val (stop_array, stop);
        }
    }
  else
    {
      stop.offset = NULL;
      g_array_append_val (stop_array, stop);
    }

  return 1;
}

static guint
bobgui_css_image_radial_parse_first_arg (BobguiCssImageRadial *radial,
                                      BobguiCssParser      *parser,
                                      GArray            *stop_array)
{
  gboolean has_shape = FALSE;
  gboolean has_size = FALSE;
  gboolean has_colorspace = FALSE;
  gboolean found_one = FALSE;
  guint i;
  static struct {
    const char *name;
    guint       value;
  } names[] = {
    { "closest-side", BOBGUI_CSS_CLOSEST_SIDE },
    { "farthest-side", BOBGUI_CSS_FARTHEST_SIDE },
    { "closest-corner", BOBGUI_CSS_CLOSEST_CORNER },
    { "farthest-corner", BOBGUI_CSS_FARTHEST_CORNER }
  };

  found_one = FALSE;

  do {
    if (!has_colorspace && bobgui_css_color_interpolation_method_can_parse (parser))
      {
        if (!bobgui_css_color_interpolation_method_parse (parser, &radial->color_space, &radial->hue_interp))
          return 0;
        has_colorspace = TRUE;
      }
    else if (!has_shape && bobgui_css_parser_try_ident (parser, "circle"))
      {
        radial->circle = TRUE;
        found_one = has_shape = TRUE;
      }
    else if (!has_shape && bobgui_css_parser_try_ident (parser, "ellipse"))
      {
        radial->circle = FALSE;
        found_one = has_shape = TRUE;
      }
    else if (!has_size)
      {
        for (i = 0; i < G_N_ELEMENTS (names); i++)
          {
            if (bobgui_css_parser_try_ident (parser, names[i].name))
              {
                found_one = has_size = TRUE;
                radial->size = names[i].value;
                break;
              }
          }

        if (!has_size && bobgui_css_number_value_can_parse (parser))
          {
            radial->sizes[0] = bobgui_css_number_value_parse (parser, BOBGUI_CSS_PARSE_LENGTH | BOBGUI_CSS_PARSE_PERCENT);
            if (radial->sizes[0] == NULL)
              return 0;
            if (bobgui_css_number_value_can_parse (parser))
              {
                radial->sizes[1] = bobgui_css_number_value_parse (parser, BOBGUI_CSS_PARSE_LENGTH | BOBGUI_CSS_PARSE_PERCENT);
                if (radial->sizes[1] == NULL)
                  return 0;
              }
            found_one = has_size = TRUE;
          }
        if (!has_size)
          break;
      }
    else
      {
        break;
      }
  } while (!(has_shape && has_size));

  if (bobgui_css_parser_try_ident (parser, "at"))
    {
      radial->position = _bobgui_css_position_value_parse (parser);
      if (!radial->position)
        return 0;
      found_one = TRUE;
    }
  else
    {
      radial->position = _bobgui_css_position_value_new (bobgui_css_number_value_new (50, BOBGUI_CSS_PERCENT),
                                                      bobgui_css_number_value_new (50, BOBGUI_CSS_PERCENT));
    }

  if (!has_size)
    {
      radial->size = BOBGUI_CSS_FARTHEST_CORNER;
    }

  if (!has_shape)
    {
      if (radial->sizes[0] && !radial->sizes[1])
        radial->circle = TRUE;
      else
        radial->circle = FALSE;
    }

  if (has_shape && radial->circle)
    {
      if (radial->sizes[0] && radial->sizes[1])
        {
          bobgui_css_parser_error_syntax (parser, "Circular gradient can only have one size");
          return 0;
        }

      if (radial->sizes[0] && bobgui_css_number_value_has_percent (radial->sizes[0]))
        {
          bobgui_css_parser_error_syntax (parser, "Circular gradient cannot have percentage as size");
          return 0;
        }
    }

  if (has_size && !radial->circle)
    {
      if (radial->sizes[0] && !radial->sizes[1])
        radial->sizes[1] = bobgui_css_value_ref (radial->sizes[0]);
    }

  if (found_one)
    return 1;

  if (!bobgui_css_image_radial_parse_color_stop (radial, parser, stop_array))
    return 0;

  return 2;
}

typedef struct
{
  BobguiCssImageRadial *self;
  GArray *stop_array;
} ParseData;

static guint
bobgui_css_image_radial_parse_arg (BobguiCssParser *parser,
                                guint         arg,
                                gpointer      user_data)
{
  ParseData *parse_data = user_data;
  BobguiCssImageRadial *self = parse_data->self;

  if (arg == 0)
    return bobgui_css_image_radial_parse_first_arg (self, parser, parse_data->stop_array);
  else
    return bobgui_css_image_radial_parse_color_stop (self, parser, parse_data->stop_array);

}

static gboolean
bobgui_css_image_radial_parse (BobguiCssImage  *image,
                            BobguiCssParser *parser)
{
  BobguiCssImageRadial *self = BOBGUI_CSS_IMAGE_RADIAL (image);
  ParseData parse_data;
  gboolean success;

  if (bobgui_css_parser_has_function (parser, "repeating-radial-gradient"))
    self->repeating = TRUE;
  else if (bobgui_css_parser_has_function (parser, "radial-gradient"))
    self->repeating = FALSE;
  else
    {
      bobgui_css_parser_error_syntax (parser, "Not a radial gradient");
      return FALSE;
    }

  parse_data.self = self;
  parse_data.stop_array = g_array_new (TRUE, FALSE, sizeof (BobguiCssImageRadialColorStop));

  success = bobgui_css_parser_consume_function (parser, 3, G_MAXUINT, bobgui_css_image_radial_parse_arg, &parse_data);

  if (!success)
    {
      g_array_free (parse_data.stop_array, TRUE);
    }
  else
    {
      self->n_stops = parse_data.stop_array->len;
      self->color_stops = (BobguiCssImageRadialColorStop *)g_array_free (parse_data.stop_array, FALSE);
    }

  return success;
}

static void
bobgui_css_image_radial_print (BobguiCssImage *image,
                            GString     *string)
{
  BobguiCssImageRadial *radial = BOBGUI_CSS_IMAGE_RADIAL (image);
  guint i;
  const char *names[] = {
    NULL,
    "closest-side",
    "farthest-side",
    "closest-corner",
    "farthest-corner"
  };

  if (radial->repeating)
    g_string_append (string, "repeating-radial-gradient(");
  else
    g_string_append (string, "radial-gradient(");

  if (radial->circle)
    g_string_append (string, "circle ");
  else
    g_string_append (string, "ellipse ");

  if (radial->size != 0)
    g_string_append (string, names[radial->size]);
  else
    {
      if (radial->sizes[0])
        bobgui_css_value_print (radial->sizes[0], string);
      if (radial->sizes[1])
        {
          g_string_append (string, " ");
          bobgui_css_value_print (radial->sizes[1], string);
        }
    }

  g_string_append (string, " at ");
  bobgui_css_value_print (radial->position, string);

  if (radial->color_space != BOBGUI_CSS_COLOR_SPACE_SRGB)
    {
      g_string_append_c (string, ' ');
      bobgui_css_color_interpolation_method_print (radial->color_space,
                                                radial->hue_interp,
                                                string);
    }

  g_string_append (string, ", ");

  for (i = 0; i < radial->n_stops; i++)
    {
      const BobguiCssImageRadialColorStop *stop = &radial->color_stops[i];

      if (i > 0)
        g_string_append (string, ", ");

      if (stop->transition_hint)
        {
          bobgui_css_value_print (stop->transition_hint, string);
          g_string_append (string, ", ");
        }

      bobgui_css_value_print (stop->color, string);

      if (stop->offset)
        {
          g_string_append (string, " ");
          bobgui_css_value_print (stop->offset, string);
        }
    }

  g_string_append (string, ")");
}

static BobguiCssImage *
bobgui_css_image_radial_compute (BobguiCssImage          *image,
                              guint                 property_id,
                              BobguiCssComputeContext *context)
{
  BobguiCssImageRadial *radial = BOBGUI_CSS_IMAGE_RADIAL (image);
  BobguiCssImageRadial *copy;
  guint i;

  copy = g_object_new (BOBGUI_TYPE_CSS_IMAGE_RADIAL, NULL);
  copy->repeating = radial->repeating;
  copy->circle = radial->circle;
  copy->size = radial->size;
  copy->color_space = radial->color_space;
  copy->hue_interp = radial->hue_interp;

  copy->position = bobgui_css_value_compute (radial->position, property_id, context);

  if (radial->sizes[0])
    copy->sizes[0] = bobgui_css_value_compute (radial->sizes[0], property_id, context);

  if (radial->sizes[1])
    copy->sizes[1] = bobgui_css_value_compute (radial->sizes[1], property_id, context);

  copy->n_stops = radial->n_stops;
  copy->color_stops = g_malloc (sizeof (BobguiCssImageRadialColorStop) * copy->n_stops);
  for (i = 0; i < radial->n_stops; i++)
    {
      const BobguiCssImageRadialColorStop *stop = &radial->color_stops[i];
      BobguiCssImageRadialColorStop *scopy = &copy->color_stops[i];

      scopy->color = bobgui_css_value_compute (stop->color, property_id, context);

      if (stop->offset)
        scopy->offset = bobgui_css_value_compute (stop->offset, property_id, context);
      else
        scopy->offset = NULL;

      if (stop->transition_hint)
        scopy->transition_hint = bobgui_css_value_compute (stop->transition_hint, property_id, context);
      else
        scopy->transition_hint = NULL;
    }

  return BOBGUI_CSS_IMAGE (copy);
}

static BobguiCssImage *
bobgui_css_image_radial_transition (BobguiCssImage *start_image,
                                 BobguiCssImage *end_image,
                                 guint        property_id,
                                 double       progress)
{
  BobguiCssImageRadial *start, *end, *result;
  guint i;

  start = BOBGUI_CSS_IMAGE_RADIAL (start_image);

  if (end_image == NULL)
    return BOBGUI_CSS_IMAGE_CLASS (_bobgui_css_image_radial_parent_class)->transition (start_image, end_image, property_id, progress);

  if (!BOBGUI_IS_CSS_IMAGE_RADIAL (end_image))
    return BOBGUI_CSS_IMAGE_CLASS (_bobgui_css_image_radial_parent_class)->transition (start_image, end_image, property_id, progress);

  end = BOBGUI_CSS_IMAGE_RADIAL (end_image);

  if (start->repeating != end->repeating ||
      start->n_stops != end->n_stops ||
      start->size != end->size ||
      start->circle != end->circle ||
      start->color_space != end->color_space ||
      start->hue_interp != end->hue_interp)
    return BOBGUI_CSS_IMAGE_CLASS (_bobgui_css_image_radial_parent_class)->transition (start_image, end_image, property_id, progress);

  result = g_object_new (BOBGUI_TYPE_CSS_IMAGE_RADIAL, NULL);
  result->repeating = start->repeating;
  result->circle = start->circle;
  result->size = start->size;

  result->position = bobgui_css_value_transition (start->position, end->position, property_id, progress);
  if (result->position == NULL)
    goto fail;

  if (start->sizes[0] && end->sizes[0])
    {
      result->sizes[0] = bobgui_css_value_transition (start->sizes[0], end->sizes[0], property_id, progress);
      if (result->sizes[0] == NULL)
        goto fail;
    }
  else
    result->sizes[0] = 0;

  if (start->sizes[1] && end->sizes[1])
    {
      result->sizes[1] = bobgui_css_value_transition (start->sizes[1], end->sizes[1], property_id, progress);
      if (result->sizes[1] == NULL)
        goto fail;
    }
  else
    result->sizes[1] = 0;

  result->color_stops = g_malloc (sizeof (BobguiCssImageRadialColorStop) * start->n_stops);
  result->n_stops = 0;
  for (i = 0; i < start->n_stops; i++)
    {
      const BobguiCssImageRadialColorStop *start_stop = &start->color_stops[i];
      const BobguiCssImageRadialColorStop *end_stop = &end->color_stops[i];
      BobguiCssImageRadialColorStop *stop = &result->color_stops[i];

      if ((start_stop->offset != NULL) != (end_stop->offset != NULL))
        goto fail;

      if (start_stop->transition_hint == NULL)
        {
          stop->transition_hint = NULL;
        }
      else
        {
          stop->transition_hint = bobgui_css_value_transition (start_stop->transition_hint,
                                                            end_stop->transition_hint,
                                                            property_id,
                                                            progress);
          if (stop->transition_hint == NULL)
            goto fail;
        }

      if (start_stop->offset == NULL)
        {
          stop->offset = NULL;
        }
      else
        {
          stop->offset = bobgui_css_value_transition (start_stop->offset,
                                                   end_stop->offset,
                                                   property_id,
                                                   progress);
          if (stop->offset == NULL)
            goto fail;
        }

      stop->color = bobgui_css_value_transition (start_stop->color,
                                              end_stop->color,
                                              property_id,
                                              progress);
      if (stop->color == NULL)
        {
          if (stop->offset)
            bobgui_css_value_unref (stop->offset);
          goto fail;
        }

      result->n_stops++;
    }

  return BOBGUI_CSS_IMAGE (result);

fail:
  g_object_unref (result);
  return BOBGUI_CSS_IMAGE_CLASS (_bobgui_css_image_radial_parent_class)->transition (start_image, end_image, property_id, progress);
}

static gboolean
bobgui_css_image_radial_equal (BobguiCssImage *image1,
                            BobguiCssImage *image2)
{
  BobguiCssImageRadial *radial1 = (BobguiCssImageRadial *) image1;
  BobguiCssImageRadial *radial2 = (BobguiCssImageRadial *) image2;
  guint i;

  if (radial1->repeating != radial2->repeating ||
      radial1->size != radial2->size ||
      !bobgui_css_value_equal (radial1->position, radial2->position) ||
      ((radial1->sizes[0] == NULL) != (radial2->sizes[0] == NULL)) ||
      (radial1->sizes[0] && radial2->sizes[0] && !bobgui_css_value_equal (radial1->sizes[0], radial2->sizes[0])) ||
      ((radial1->sizes[1] == NULL) != (radial2->sizes[1] == NULL)) ||
      (radial1->sizes[1] && radial2->sizes[1] && !bobgui_css_value_equal (radial1->sizes[1], radial2->sizes[1])) ||
      radial1->n_stops != radial2->n_stops ||
      radial1->color_space != radial2->color_space ||
      radial1->hue_interp != radial2->hue_interp)
    return FALSE;

  for (i = 0; i < radial1->n_stops; i++)
    {
      const BobguiCssImageRadialColorStop *stop1 = &radial1->color_stops[i];
      const BobguiCssImageRadialColorStop *stop2 = &radial2->color_stops[i];

      if (!bobgui_css_value_equal0 (stop1->offset, stop2->offset) ||
          !bobgui_css_value_equal0 (stop1->transition_hint, stop2->transition_hint) ||
          !bobgui_css_value_equal (stop1->color, stop2->color))
        return FALSE;
    }

  return TRUE;
}

static void
bobgui_css_image_radial_dispose (GObject *object)
{
  BobguiCssImageRadial *radial = BOBGUI_CSS_IMAGE_RADIAL (object);
  guint i;

  for (i = 0; i < radial->n_stops; i ++)
    {
      BobguiCssImageRadialColorStop *stop = &radial->color_stops[i];

      if (stop->transition_hint)
        bobgui_css_value_unref (stop->transition_hint);
      bobgui_css_value_unref (stop->color);
      if (stop->offset)
        bobgui_css_value_unref (stop->offset);
    }
  g_free (radial->color_stops);

  if (radial->position)
    {
      bobgui_css_value_unref (radial->position);
      radial->position = NULL;
    }

  for (i = 0; i < 2; i++)
    if (radial->sizes[i])
      {
        bobgui_css_value_unref (radial->sizes[i]);
        radial->sizes[i] = NULL;
      }

  G_OBJECT_CLASS (_bobgui_css_image_radial_parent_class)->dispose (object);
}

static gboolean
bobgui_css_image_radial_is_computed (BobguiCssImage *image)
{
  BobguiCssImageRadial *radial = BOBGUI_CSS_IMAGE_RADIAL (image);
  guint i;
  gboolean computed = TRUE;

  computed = computed && (!radial->position || bobgui_css_value_is_computed (radial->position));
  computed = computed && (!radial->sizes[0] || bobgui_css_value_is_computed (radial->sizes[0]));
  computed = computed && (!radial->sizes[1] || bobgui_css_value_is_computed (radial->sizes[1]));

  if (computed)
    for (i = 0; i < radial->n_stops; i ++)
      {
        const BobguiCssImageRadialColorStop *stop = &radial->color_stops[i];

       if (stop->transition_hint && !bobgui_css_value_is_computed (stop->transition_hint))
         {
           computed = FALSE;
           break;
         }

        if (stop->offset && !bobgui_css_value_is_computed (stop->offset))
          {
            computed = FALSE;
            break;
          }

        if (!bobgui_css_value_is_computed (stop->color))
          {
            computed = FALSE;
            break;
          }
      }

  return computed;
}

static gboolean
bobgui_css_image_radial_contains_current_color (BobguiCssImage *image)
{
  BobguiCssImageRadial *radial = BOBGUI_CSS_IMAGE_RADIAL (image);

  for (guint i = 0; i < radial->n_stops; i ++)
    {
      const BobguiCssImageRadialColorStop *stop = &radial->color_stops[i];

      if (bobgui_css_value_contains_current_color (stop->color))
        return TRUE;
    }

  return FALSE;
}

static BobguiCssImage *
bobgui_css_image_radial_resolve (BobguiCssImage          *image,
                              BobguiCssComputeContext *context,
                              BobguiCssValue          *current_color)
{
  BobguiCssImageRadial *radial = BOBGUI_CSS_IMAGE_RADIAL (image);
  BobguiCssImageRadial *copy;

  if (!bobgui_css_image_radial_contains_current_color (image))
    return g_object_ref (image);

  copy = g_object_new (BOBGUI_TYPE_CSS_IMAGE_RADIAL, NULL);
  copy->repeating = radial->repeating;
  copy->circle = radial->circle;
  copy->size = radial->size;

  copy->position = bobgui_css_value_ref (radial->position);

  if (radial->sizes[0])
    copy->sizes[0] = bobgui_css_value_ref (radial->sizes[0]);

  if (radial->sizes[1])
    copy->sizes[1] = bobgui_css_value_ref (radial->sizes[1]);

  copy->n_stops = radial->n_stops;
  copy->color_stops = g_new (BobguiCssImageRadialColorStop, copy->n_stops);

  for (guint i = 0; i < radial->n_stops; i++)
    {
      const BobguiCssImageRadialColorStop *stop = &radial->color_stops[i];
      BobguiCssImageRadialColorStop *scopy = &copy->color_stops[i];

      scopy->color = bobgui_css_value_resolve (stop->color, context, current_color);

      if (stop->offset)
        scopy->offset = bobgui_css_value_ref (stop->offset);
      else
        scopy->offset = NULL;

      if (stop->transition_hint)
        scopy->transition_hint = bobgui_css_value_ref (stop->transition_hint);
      else
        scopy->transition_hint = NULL;
    }

  return BOBGUI_CSS_IMAGE (copy);
}

static void
_bobgui_css_image_radial_class_init (BobguiCssImageRadialClass *klass)
{
  BobguiCssImageClass *image_class = BOBGUI_CSS_IMAGE_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  image_class->snapshot = bobgui_css_image_radial_snapshot;
  image_class->parse = bobgui_css_image_radial_parse;
  image_class->print = bobgui_css_image_radial_print;
  image_class->compute = bobgui_css_image_radial_compute;
  image_class->transition = bobgui_css_image_radial_transition;
  image_class->equal = bobgui_css_image_radial_equal;
  image_class->is_computed = bobgui_css_image_radial_is_computed;
  image_class->contains_current_color = bobgui_css_image_radial_contains_current_color;
  image_class->resolve = bobgui_css_image_radial_resolve;

  object_class->dispose = bobgui_css_image_radial_dispose;
}

static void
_bobgui_css_image_radial_init (BobguiCssImageRadial *radial)
{
}
