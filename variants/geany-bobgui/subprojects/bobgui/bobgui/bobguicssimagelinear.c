/*
 * Copyright © 2012 Red Hat Inc.
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
 * Authors: Benjamin Otte <otte@gnome.org>
 */

#include "config.h"

#include "bobguicssimagelinearprivate.h"

#include <math.h>

#include "bobguicsscolorvalueprivate.h"
#include "bobguicssnumbervalueprivate.h"
#include "bobguicsscolorvalueprivate.h"
#include "bobguicssprovider.h"
#include "bobguisnapshotprivate.h"

G_DEFINE_TYPE (BobguiCssImageLinear, _bobgui_css_image_linear, BOBGUI_TYPE_CSS_IMAGE)

static void
bobgui_css_image_linear_get_repeating_start_end (BobguiCssImageLinear *linear,
                                              double             length,
                                              double            *start,
                                              double            *end)
{
  const BobguiCssImageLinearColorStop *stop;
  double pos;
  guint i;

  g_assert (linear->repeating);

  stop = &linear->color_stops[0];
  if (stop->offset == NULL)
    *start = 0;
  else
    *start = bobgui_css_number_value_get (stop->offset, length) / length;

  *end = *start;

  for (i = 0; i < linear->n_stops; i++)
    {
      stop = &linear->color_stops[i];

      if (stop->offset == NULL)
        continue;

      pos = bobgui_css_number_value_get (stop->offset, length) / length;

      *end = MAX (pos, *end);
    }

  if (stop->offset == NULL)
    *end = MAX (*end, 1.0);
}

static void
bobgui_css_image_linear_compute_start_point (double angle_in_degrees,
                                          double width,
                                          double height,
                                          double *out_x,
                                          double *out_y,
                                          double *out_length)
{
  double angle, c, slope, perpendicular;
  double x, y, length;

  angle = fmod (angle_in_degrees, 360);
  if (angle < 0)
    angle += 360;

  if (angle == 0)
    {
      *out_x = 0;
      *out_y = -height;
      *out_length = height;
      return;
    }
  else if (angle == 90)
    {
      *out_x = width;
      *out_y = 0;
      *out_length = width;
      return;
    }
  else if (angle == 180)
    {
      *out_x = 0;
      *out_y = height;
      *out_length = height;
      return;
    }
  else if (angle == 270)
    {
      *out_x = -width;
      *out_y = 0;
      *out_length = width;
      return;
    }

  /* The tan() is confusing because the angle is clockwise
   * from 'to top' */
  perpendicular = tan (angle * G_PI / 180);
  slope = -1 / perpendicular;

  if (angle > 180)
    width = - width;
  if (angle < 90 || angle > 270)
    height = - height;

  /* Compute c (of y = mx + c) of perpendicular */
  c = height - perpendicular * width;

  x = c / (slope - perpendicular);
  y = perpendicular * x + c;

  length = sqrt (x * x + y * y);

  *out_x = x;
  *out_y = y;
  *out_length = length;
}

static void
bobgui_css_image_linear_snapshot (BobguiCssImage *image,
                               BobguiSnapshot *snapshot,
                               double       width,
                               double       height)
{
  BobguiCssImageLinear *linear = BOBGUI_CSS_IMAGE_LINEAR (image);
  GskGradientStop *stops;
  double angle; /* actual angle of the gradient line in degrees */
  double x, y; /* coordinates of start point */
  double length; /* distance in pixels for 100% */
  double start, end; /* position of first/last point on gradient line - with gradient line being [0, 1] */
  double offset, hint;
  int i, last;
  GskGradient *gradient;

  if (linear->side)
    {
      /* special casing the regular cases here so we don't get rounding errors */
      switch (linear->side)
      {
        case 1 << BOBGUI_CSS_RIGHT:
          angle = 90;
          break;
        case 1 << BOBGUI_CSS_LEFT:
          angle = 270;
          break;
        case 1 << BOBGUI_CSS_TOP:
          angle = 0;
          break;
        case 1 << BOBGUI_CSS_BOTTOM:
          angle = 180;
          break;
        default:
          angle = atan2 (linear->side & 1 << BOBGUI_CSS_TOP ? -width : width,
                         linear->side & 1 << BOBGUI_CSS_LEFT ? -height : height);
          angle = 180 * angle / G_PI + 90;
          break;
      }
    }
  else
    {
      angle = bobgui_css_number_value_get (linear->angle, 100);
    }

  bobgui_css_image_linear_compute_start_point (angle,
                                            width, height,
                                            &x, &y,
                                            &length);

  if (linear->repeating)
    {
      bobgui_css_image_linear_get_repeating_start_end (linear, length, &start, &end);

      if (start == end)
        {
          /* Repeating gradients with all color stops sharing the same offset
           * get the color of the last color stop
           */
          const BobguiCssImageLinearColorStop *stop = &linear->color_stops[linear->n_stops - 1];
          GdkColor color;

          bobgui_css_color_to_color (bobgui_css_color_value_get_color (stop->color), &color);

          bobgui_snapshot_add_color (snapshot,
                                  &color,
                                  &GRAPHENE_RECT_INIT (0, 0, width, height));

          gdk_color_finish (&color);
          return;
        }
    }
  else
    {
      start = 0;
      end = 1;
    }

  offset = start;
  last = -1;
  stops = g_newa (GskGradientStop, linear->n_stops);
  hint = start;

  for (i = 0; i < linear->n_stops; i++)
    {
      const BobguiCssImageLinearColorStop *stop = &linear->color_stops[i];
      double pos, step;

      if (stop->offset == NULL)
        {
          if (stop->transition_hint)
            {
              hint = MAX (hint, bobgui_css_number_value_get (stop->transition_hint, length) / length);
              hint = CLAMP (hint, 0.0, 1.0);
            }

          if (i == 0)
            pos = 0.0;
          else if (i + 1 == linear->n_stops)
            pos = 1.0;
          else
            continue;
        }
      else
        {
          pos = bobgui_css_number_value_get (stop->offset, length) / length;
          pos = CLAMP (pos, 0.0, 1.0);
        }

      pos = MAX (pos, hint);
      pos = MAX (pos, offset);
      step = (pos - offset) / (i - last);
      for (last = last + 1; last <= i; last++)
        {
          stop = &linear->color_stops[last];

          offset += step;

          bobgui_css_color_to_color (bobgui_css_color_value_get_color (stop->color), &stops[last].color);

          stops[last].offset = (offset - start) / (end - start);

          if (last > 0 && stop->transition_hint)
            {
              hint = bobgui_css_number_value_get (stop->transition_hint, length) / length;
              hint = CLAMP (hint, 0, 1);

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
  for (i = 0; i < linear->n_stops; i++)
    gsk_gradient_add_stop (gradient, stops[i].offset, stops[i].transition_hint, &stops[i].color);

  if (linear->color_space != BOBGUI_CSS_COLOR_SPACE_SRGB)
    g_warning_once ("Gradient interpolation color spaces are not supported yet");

  gsk_gradient_set_interpolation (gradient, bobgui_css_color_space_get_color_state (linear->color_space));
  gsk_gradient_set_hue_interpolation (gradient, bobgui_css_hue_interpolation_to_hue_interpolation (linear->hue_interp));
  gsk_gradient_set_repeat (gradient, linear->repeating ? GSK_REPEAT_REPEAT : GSK_REPEAT_PAD);

  bobgui_snapshot_add_linear_gradient (
      snapshot,
      &GRAPHENE_RECT_INIT (0, 0, width, height),
      &GRAPHENE_POINT_INIT (width / 2 + x * (start - 0.5), height / 2 + y * (start - 0.5)),
      &GRAPHENE_POINT_INIT (width / 2 + x * (end - 0.5),   height / 2 + y * (end - 0.5)),
      gradient);

  for (i = 0; i < linear->n_stops; i++)
    gdk_color_finish (&stops[i].color);

  gsk_gradient_free (gradient);
}

static guint
bobgui_css_image_linear_parse_color_stop (BobguiCssImageLinear *self,
                                       BobguiCssParser      *parser,
                                       GArray            *stop_array)
{
  BobguiCssImageLinearColorStop stop;

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
bobgui_css_image_linear_parse_first_arg (BobguiCssImageLinear *linear,
                                      BobguiCssParser      *parser,
                                      GArray            *stop_array)
{
  guint i;
  gboolean has_colorspace = FALSE;
  gboolean has_side_or_angle = FALSE;
  guint retval = 1;

  do
    {
      if (!has_colorspace &&bobgui_css_color_interpolation_method_can_parse (parser))
        {
          if (!bobgui_css_color_interpolation_method_parse (parser, &linear->color_space, &linear->hue_interp))
            return 0;
          has_colorspace = TRUE;
        }
      else if (!has_side_or_angle && bobgui_css_parser_try_ident (parser, "to"))
        {
          bobgui_css_parser_consume_token (parser);

          for (i = 0; i < 2; i++)
            {
              if (bobgui_css_parser_try_ident (parser, "left"))
                {
                  if (linear->side & ((1 << BOBGUI_CSS_LEFT) | (1 << BOBGUI_CSS_RIGHT)))
                    {
                      bobgui_css_parser_error_syntax (parser, "Expected 'top', 'bottom' or comma");
                      return 0;
                    }
                  linear->side |= (1 << BOBGUI_CSS_LEFT);
                }
              else if (bobgui_css_parser_try_ident (parser, "right"))
                {
                  if (linear->side & ((1 << BOBGUI_CSS_LEFT) | (1 << BOBGUI_CSS_RIGHT)))
                    {
                      bobgui_css_parser_error_syntax (parser, "Expected 'top', 'bottom' or comma");
                      return 0;
                    }
                  linear->side |= (1 << BOBGUI_CSS_RIGHT);
                }
              else if (bobgui_css_parser_try_ident (parser, "top"))
                {
                  if (linear->side & ((1 << BOBGUI_CSS_TOP) | (1 << BOBGUI_CSS_BOTTOM)))
                    {
                      bobgui_css_parser_error_syntax (parser, "Expected 'left', 'right' or comma");
                      return 0;
                    }
                  linear->side |= (1 << BOBGUI_CSS_TOP);
                }
              else if (bobgui_css_parser_try_ident (parser, "bottom"))
                {
                  if (linear->side & ((1 << BOBGUI_CSS_TOP) | (1 << BOBGUI_CSS_BOTTOM)))
                    {
                      bobgui_css_parser_error_syntax (parser, "Expected 'left', 'right' or comma");
                      return 0;
                    }
                  linear->side |= (1 << BOBGUI_CSS_BOTTOM);
                }
              else
                break;
            }

          if (linear->side == 0)
            {
              bobgui_css_parser_error_syntax (parser, "Expected side that gradient should go to");
              return 0;
            }

          has_side_or_angle = TRUE;
        }
      else if (!has_side_or_angle && bobgui_css_number_value_can_parse (parser))
        {
          linear->angle = bobgui_css_number_value_parse (parser, BOBGUI_CSS_PARSE_ANGLE);
          if (linear->angle == NULL)
            return 0;

          has_side_or_angle = TRUE;
        }
      else if (bobgui_css_token_is (bobgui_css_parser_get_token (parser), BOBGUI_CSS_TOKEN_COMMA))
        {
          retval = 1;
          break;
        }
      else
        {
          if (bobgui_css_image_linear_parse_color_stop (linear, parser, stop_array))
            {
              retval = 2;
              break;
            }

          return 0;
        }
    }
  while (!(has_colorspace && has_side_or_angle));

  if (linear->angle == NULL && linear->side == 0)
    linear->side = (1 << BOBGUI_CSS_BOTTOM);

  return retval;
}

typedef struct
{
  BobguiCssImageLinear *self;
  GArray *stop_array;
} ParseData;

static guint
bobgui_css_image_linear_parse_arg (BobguiCssParser *parser,
                                guint         arg,
                                gpointer      user_data)
{
  ParseData *parse_data = user_data;
  BobguiCssImageLinear *self = parse_data->self;

  if (arg == 0)
    return bobgui_css_image_linear_parse_first_arg (self, parser, parse_data->stop_array);
  else
    return bobgui_css_image_linear_parse_color_stop (self, parser, parse_data->stop_array);
}

static gboolean
bobgui_css_image_linear_parse (BobguiCssImage  *image,
                            BobguiCssParser *parser)
{
  BobguiCssImageLinear *self = BOBGUI_CSS_IMAGE_LINEAR (image);
  ParseData parse_data;
  gboolean success;

  if (bobgui_css_parser_has_function (parser, "repeating-linear-gradient"))
    self->repeating = TRUE;
  else if (bobgui_css_parser_has_function (parser, "linear-gradient"))
    self->repeating = FALSE;
  else
    {
      bobgui_css_parser_error_syntax (parser, "Not a linear gradient");
      return FALSE;
    }

  parse_data.self = self;
  parse_data.stop_array = g_array_new (TRUE, FALSE, sizeof (BobguiCssImageLinearColorStop));

  success = bobgui_css_parser_consume_function (parser, 3, G_MAXUINT, bobgui_css_image_linear_parse_arg, &parse_data);

  if (!success)
    {
      g_array_free (parse_data.stop_array, TRUE);
    }
  else
    {
      self->n_stops = parse_data.stop_array->len;
      self->color_stops = (BobguiCssImageLinearColorStop *)g_array_free (parse_data.stop_array, FALSE);
    }

  return success;
}

static void
bobgui_css_image_linear_print (BobguiCssImage *image,
                            GString     *string)
{
  BobguiCssImageLinear *linear = BOBGUI_CSS_IMAGE_LINEAR (image);
  guint i;
  gboolean has_printed = FALSE;

  if (linear->repeating)
    g_string_append (string, "repeating-linear-gradient(");
  else
    g_string_append (string, "linear-gradient(");

  if (linear->side)
    {
      if (linear->side != (1 << BOBGUI_CSS_BOTTOM))
        {
          g_string_append (string, "to");

          if (linear->side & (1 << BOBGUI_CSS_TOP))
            g_string_append (string, " top");
          else if (linear->side & (1 << BOBGUI_CSS_BOTTOM))
            g_string_append (string, " bottom");

          if (linear->side & (1 << BOBGUI_CSS_LEFT))
            g_string_append (string, " left");
          else if (linear->side & (1 << BOBGUI_CSS_RIGHT))
            g_string_append (string, " right");

          has_printed = TRUE;
        }
    }
  else
    {
      bobgui_css_value_print (linear->angle, string);
      has_printed = TRUE;
    }

  if (linear->color_space != BOBGUI_CSS_COLOR_SPACE_SRGB)
    {
      if (has_printed)
        g_string_append_c (string, ' ');

      bobgui_css_color_interpolation_method_print (linear->color_space,
                                                linear->hue_interp,
                                                string);
      has_printed = TRUE;
    }

  if (has_printed)
    g_string_append (string, ", ");

  for (i = 0; i < linear->n_stops; i++)
    {
      const BobguiCssImageLinearColorStop *stop = &linear->color_stops[i];

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
bobgui_css_image_linear_compute (BobguiCssImage          *image,
                              guint                 property_id,
                              BobguiCssComputeContext *context)
{
  BobguiCssImageLinear *linear = BOBGUI_CSS_IMAGE_LINEAR (image);
  BobguiCssImageLinear *copy;
  guint i;

  copy = g_object_new (BOBGUI_TYPE_CSS_IMAGE_LINEAR, NULL);
  copy->repeating = linear->repeating;
  copy->side = linear->side;
  copy->color_space = linear->color_space;
  copy->hue_interp = linear->hue_interp;

  if (linear->angle)
    copy->angle = bobgui_css_value_compute (linear->angle, property_id, context);

  copy->n_stops = linear->n_stops;
  copy->color_stops = g_malloc (sizeof (BobguiCssImageLinearColorStop) * copy->n_stops);
  for (i = 0; i < linear->n_stops; i++)
    {
      const BobguiCssImageLinearColorStop *stop = &linear->color_stops[i];
      BobguiCssImageLinearColorStop *scopy = &copy->color_stops[i];

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
bobgui_css_image_linear_transition (BobguiCssImage *start_image,
                                 BobguiCssImage *end_image,
                                 guint        property_id,
                                 double       progress)
{
  BobguiCssImageLinear *start, *end, *result;
  guint i;

  start = BOBGUI_CSS_IMAGE_LINEAR (start_image);

  if (end_image == NULL)
    return BOBGUI_CSS_IMAGE_CLASS (_bobgui_css_image_linear_parent_class)->transition (start_image, end_image, property_id, progress);

  if (!BOBGUI_IS_CSS_IMAGE_LINEAR (end_image))
    return BOBGUI_CSS_IMAGE_CLASS (_bobgui_css_image_linear_parent_class)->transition (start_image, end_image, property_id, progress);

  end = BOBGUI_CSS_IMAGE_LINEAR (end_image);

  if ((start->repeating != end->repeating)
      || (start->n_stops != end->n_stops)
      || (start->color_space != end->color_space)
      || (start->hue_interp != end->hue_interp))
    return BOBGUI_CSS_IMAGE_CLASS (_bobgui_css_image_linear_parent_class)->transition (start_image, end_image, property_id, progress);

  result = g_object_new (BOBGUI_TYPE_CSS_IMAGE_LINEAR, NULL);
  result->repeating = start->repeating;
  result->color_space = start->color_space;
  result->hue_interp = start->hue_interp;

  if (start->side != end->side)
    goto fail;

  result->side = start->side;
  if (result->side == 0)
    result->angle = bobgui_css_value_transition (start->angle, end->angle, property_id, progress);
  if (result->angle == NULL)
    goto fail;

  /* Maximum amountof stops */
  result->color_stops = g_malloc (sizeof (BobguiCssImageLinearColorStop) * start->n_stops);
  result->n_stops = 0;
  for (i = 0; i < start->n_stops; i++)
    {
      const BobguiCssImageLinearColorStop *start_stop = &start->color_stops[i];
      const BobguiCssImageLinearColorStop *end_stop = &end->color_stops[i];
      BobguiCssImageLinearColorStop *stop = &result->color_stops[i];

      if ((start_stop->transition_hint != NULL) != (end_stop->transition_hint != NULL))
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

      if ((start_stop->offset != NULL) != (end_stop->offset != NULL))
        goto fail;

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

      result->n_stops ++;
    }

  return BOBGUI_CSS_IMAGE (result);

fail:
  g_object_unref (result);
  return BOBGUI_CSS_IMAGE_CLASS (_bobgui_css_image_linear_parent_class)->transition (start_image, end_image, property_id, progress);
}

static gboolean
bobgui_css_image_linear_equal (BobguiCssImage *image1,
                            BobguiCssImage *image2)
{
  BobguiCssImageLinear *linear1 = (BobguiCssImageLinear *) image1;
  BobguiCssImageLinear *linear2 = (BobguiCssImageLinear *) image2;
  guint i;

  if (linear1->repeating != linear2->repeating ||
      linear1->side != linear2->side ||
      (linear1->side == 0 && !bobgui_css_value_equal (linear1->angle, linear2->angle)) ||
      linear1->n_stops != linear2->n_stops ||
      linear1->color_space != linear2->color_space ||
      linear1->hue_interp != linear2->hue_interp)
    return FALSE;

  for (i = 0; i < linear1->n_stops; i++)
    {
      const BobguiCssImageLinearColorStop *stop1 = &linear1->color_stops[i];
      const BobguiCssImageLinearColorStop *stop2 = &linear2->color_stops[i];

      if (!bobgui_css_value_equal0 (stop1->offset, stop2->offset) ||
          !bobgui_css_value_equal0 (stop1->transition_hint, stop2->transition_hint) ||
          !bobgui_css_value_equal (stop1->color, stop2->color))
        return FALSE;
    }

  return TRUE;
}

static void
bobgui_css_image_linear_dispose (GObject *object)
{
  BobguiCssImageLinear *linear = BOBGUI_CSS_IMAGE_LINEAR (object);
  guint i;

  for (i = 0; i < linear->n_stops; i ++)
    {
      BobguiCssImageLinearColorStop *stop = &linear->color_stops[i];

      if (stop->transition_hint)
        bobgui_css_value_unref (stop->transition_hint);
      bobgui_css_value_unref (stop->color);
      if (stop->offset)
        bobgui_css_value_unref (stop->offset);
    }
  g_free (linear->color_stops);

  linear->side = 0;
  if (linear->angle)
    {
      bobgui_css_value_unref (linear->angle);
      linear->angle = NULL;
    }

  G_OBJECT_CLASS (_bobgui_css_image_linear_parent_class)->dispose (object);
}

static gboolean
bobgui_css_image_linear_is_computed (BobguiCssImage *image)
{
  BobguiCssImageLinear *linear = BOBGUI_CSS_IMAGE_LINEAR (image);
  guint i;
  gboolean computed = TRUE;

  computed = !linear->angle || bobgui_css_value_is_computed (linear->angle);

  if (computed)
    for (i = 0; i < linear->n_stops; i++)
      {
        const BobguiCssImageLinearColorStop *stop = &linear->color_stops[i];

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
bobgui_css_image_linear_contains_current_color (BobguiCssImage *image)
{
  BobguiCssImageLinear *linear = BOBGUI_CSS_IMAGE_LINEAR (image);

  for (guint i = 0; i < linear->n_stops; i ++)
    {
      const BobguiCssImageLinearColorStop *stop = &linear->color_stops[i];

      if (bobgui_css_value_contains_current_color (stop->color))
        return TRUE;
    }

  return FALSE;
}

static BobguiCssImage *
bobgui_css_image_linear_resolve (BobguiCssImage          *image,
                              BobguiCssComputeContext *context,
                              BobguiCssValue          *current_color)
{
  BobguiCssImageLinear *linear = BOBGUI_CSS_IMAGE_LINEAR (image);
  BobguiCssImageLinear *copy;
  guint i;

  if (!bobgui_css_image_linear_contains_current_color (image))
    return g_object_ref (image);

  copy = g_object_new (BOBGUI_TYPE_CSS_IMAGE_LINEAR, NULL);
  copy->repeating = linear->repeating;
  copy->side = linear->side;

  if (linear->angle)
    copy->angle = bobgui_css_value_ref (linear->angle);

  copy->n_stops = linear->n_stops;
  copy->color_stops = g_new (BobguiCssImageLinearColorStop, copy->n_stops);

  for (i = 0; i < linear->n_stops; i++)
    {
      const BobguiCssImageLinearColorStop *stop = &linear->color_stops[i];
      BobguiCssImageLinearColorStop *scopy = &copy->color_stops[i];

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
_bobgui_css_image_linear_class_init (BobguiCssImageLinearClass *klass)
{
  BobguiCssImageClass *image_class = BOBGUI_CSS_IMAGE_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  image_class->snapshot = bobgui_css_image_linear_snapshot;
  image_class->parse = bobgui_css_image_linear_parse;
  image_class->print = bobgui_css_image_linear_print;
  image_class->compute = bobgui_css_image_linear_compute;
  image_class->equal = bobgui_css_image_linear_equal;
  image_class->transition = bobgui_css_image_linear_transition;
  image_class->is_computed = bobgui_css_image_linear_is_computed;
  image_class->contains_current_color = bobgui_css_image_linear_contains_current_color;
  image_class->resolve = bobgui_css_image_linear_resolve;

  object_class->dispose = bobgui_css_image_linear_dispose;
}

static void
_bobgui_css_image_linear_init (BobguiCssImageLinear *linear)
{
}

