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

#include "bobguicssimageconicprivate.h"

#include <math.h>

#include "bobguicsscolorvalueprivate.h"
#include "bobguicssnumbervalueprivate.h"
#include "bobguicsspositionvalueprivate.h"
#include "bobguicssprovider.h"
#include "bobguisnapshotprivate.h"

G_DEFINE_TYPE (BobguiCssImageConic, bobgui_css_image_conic, BOBGUI_TYPE_CSS_IMAGE)

static void
bobgui_css_image_conic_snapshot (BobguiCssImage        *image,
                              BobguiSnapshot        *snapshot,
                              double              width,
                              double              height)
{
  BobguiCssImageConic *self = BOBGUI_CSS_IMAGE_CONIC (image);
  GskGradientStop *stops;
  int i, last;
  double offset, hint;
  GskGradient *gradient;

  stops = g_newa (GskGradientStop, self->n_stops);

  last = -1;
  offset = 0;
  hint = 0;
  for (i = 0; i < self->n_stops; i++)
    {
      const BobguiCssImageConicColorStop *stop = &self->color_stops[i];
      double pos, step;

      if (stop->offset == NULL)
        {
          if (stop->transition_hint)
            {
              hint = MAX (hint, bobgui_css_number_value_get (stop->transition_hint, 360) / 360);
              hint = CLAMP (hint, 0.0, 1.0);
            }

          if (i == 0)
            pos = 0.0;
          else if (i + 1 == self->n_stops)
            pos = 1.0;
          else
            continue;
        }
      else
        {
          pos = bobgui_css_number_value_get (stop->offset, 360) / 360;
          pos = CLAMP (pos, 0.0, 1.0);
        }

      pos = MAX (pos, hint);
      pos = MAX (pos, offset);
      step = (pos - offset) / (i - last);
      for (last = last + 1; last <= i; last++)
        {
          stop = &self->color_stops[last];

          offset += step;

          stops[last].offset = offset;
          bobgui_css_color_to_color (bobgui_css_color_value_get_color (stop->color), &stops[last].color);
          if (last > 0 && stop->transition_hint)
            {
              hint = bobgui_css_number_value_get (stop->transition_hint, 360) / 360;
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
  for (i = 0; i < self->n_stops; i++)
    gsk_gradient_add_stop (gradient, stops[i].offset, stops[i].transition_hint, &stops[i].color);

  if (self->color_space != BOBGUI_CSS_COLOR_SPACE_SRGB)
    g_warning_once ("Gradient interpolation color spaces are not supported yet");

  gsk_gradient_set_interpolation (gradient, bobgui_css_color_space_get_color_state (self->color_space));
  gsk_gradient_set_hue_interpolation (gradient, bobgui_css_hue_interpolation_to_hue_interpolation (self->hue_interp));

  bobgui_snapshot_add_conic_gradient (
          snapshot,
          &GRAPHENE_RECT_INIT (0, 0, width, height),
          &GRAPHENE_POINT_INIT (_bobgui_css_position_value_get_x (self->center, width),
                                _bobgui_css_position_value_get_y (self->center, height)),
          bobgui_css_number_value_get (self->rotation, 360),
          gradient);

  for (i = 0; i < self->n_stops; i++)
    gdk_color_finish (&stops[i].color);

  gsk_gradient_free (gradient);
}

static guint
bobgui_css_image_conic_parse_color_stop (BobguiCssImageConic *self,
                                      BobguiCssParser      *parser,
                                      GArray            *stop_array)
{
  BobguiCssValue *hint = NULL;
  BobguiCssValue *color = NULL;
  BobguiCssValue *angles[2] = { NULL, NULL };

  if (bobgui_css_number_value_can_parse (parser))
    {
      hint = bobgui_css_number_value_parse (parser,
                                         BOBGUI_CSS_PARSE_PERCENT
                                         | BOBGUI_CSS_PARSE_ANGLE);
      if (hint == NULL)
        {
          bobgui_css_parser_error_syntax (parser, "Failed to parse transition hint");
          goto fail;
        }

      if (!bobgui_css_parser_has_token (parser, BOBGUI_CSS_TOKEN_COMMA))
        goto fail;

      bobgui_css_parser_consume_token (parser);
    }

  color = bobgui_css_color_value_parse (parser);
  if (color == NULL)
    {
      bobgui_css_parser_error_syntax (parser, "Expected color stop to contain a color");
      goto fail;
    }

  if (bobgui_css_number_value_can_parse (parser))
    {
      angles[0] = bobgui_css_number_value_parse (parser, BOBGUI_CSS_PARSE_ANGLE | BOBGUI_CSS_PARSE_PERCENT);
      if (angles[0] == NULL)
        goto fail;

      if (bobgui_css_number_value_can_parse (parser))
        {
          angles[1] = bobgui_css_number_value_parse (parser, BOBGUI_CSS_PARSE_ANGLE | BOBGUI_CSS_PARSE_PERCENT);
          if (angles[1] == NULL)
            goto fail;
        }
    }

  g_array_append_vals (stop_array, (BobguiCssImageConicColorStop[1]) {
                         { angles[0], hint, color }
                       },
                       1);
  if (angles[1])
    g_array_append_vals (stop_array, (BobguiCssImageConicColorStop[1]) {
                           { angles[1], NULL, bobgui_css_value_ref (color) }
                         },
                         1);

  return 1;

fail:
  g_clear_pointer (&hint, bobgui_css_value_unref);
  g_clear_pointer (&angles[0], bobgui_css_value_unref);
  g_clear_pointer (&angles[1], bobgui_css_value_unref);
  g_clear_pointer (&color, bobgui_css_value_unref);

  return 0;
}

static guint
bobgui_css_image_conic_parse_first_arg (BobguiCssImageConic *self,
                                     BobguiCssParser      *parser,
                                     GArray            *stop_array)
{
  gboolean has_rotation = FALSE;
  gboolean has_center = FALSE;
  gboolean has_colorspace = FALSE;
  int retval = 1;

  do
    {
      if (!has_colorspace && bobgui_css_color_interpolation_method_can_parse (parser))
        {
          if (!bobgui_css_color_interpolation_method_parse (parser, &self->color_space, &self->hue_interp))
            return 0;
          has_colorspace = TRUE;
        }
      else if (!has_rotation && bobgui_css_parser_try_ident (parser, "from"))
        {
          self->rotation = bobgui_css_number_value_parse (parser, BOBGUI_CSS_PARSE_ANGLE);
          if (self->rotation == NULL)
            return 0;
          has_rotation = TRUE;
        }
      else if (!has_center && bobgui_css_parser_try_ident (parser, "at"))
        {
          self->center = _bobgui_css_position_value_parse (parser);
          if (self->center == NULL)
            return 0;
          has_center = TRUE;
        }
      else if (bobgui_css_token_is (bobgui_css_parser_get_token (parser), BOBGUI_CSS_TOKEN_COMMA))
        {
          retval = 1;
          break;
        }
      else
        {
          if (bobgui_css_image_conic_parse_color_stop (self, parser, stop_array))
            {
              retval = 2;
              break;
            }
        }
    }
  while (!(has_colorspace && has_rotation && has_center));

  if (!has_rotation)
    self->rotation = bobgui_css_number_value_new (0, BOBGUI_CSS_DEG);

  if (!has_center)
    self->center = _bobgui_css_position_value_new (bobgui_css_number_value_new (50, BOBGUI_CSS_PERCENT),
                                                bobgui_css_number_value_new (50, BOBGUI_CSS_PERCENT));

  return retval;
}

typedef struct
{
  BobguiCssImageConic *self;
  GArray *stop_array;
} ParseData;

static guint
bobgui_css_image_conic_parse_arg (BobguiCssParser *parser,
                                guint         arg,
                                gpointer      user_data)
{
  ParseData *parse_data = user_data;
  BobguiCssImageConic *self = parse_data->self;

  if (arg == 0)
    return bobgui_css_image_conic_parse_first_arg (self, parser, parse_data->stop_array);
  else
    return bobgui_css_image_conic_parse_color_stop (self, parser, parse_data->stop_array);
}

static gboolean
bobgui_css_image_conic_parse (BobguiCssImage  *image,
                            BobguiCssParser *parser)
{
  BobguiCssImageConic *self = BOBGUI_CSS_IMAGE_CONIC (image);
  ParseData parse_data;
  gboolean success;

  if (!bobgui_css_parser_has_function (parser, "conic-gradient"))
    {
      bobgui_css_parser_error_syntax (parser, "Not a conic gradient");
      return FALSE;
    }

  parse_data.self = self;
  parse_data.stop_array = g_array_new (TRUE, FALSE, sizeof (BobguiCssImageConicColorStop));

  success = bobgui_css_parser_consume_function (parser, 3, G_MAXUINT, bobgui_css_image_conic_parse_arg, &parse_data);

  if (!success)
    {
      g_array_free (parse_data.stop_array, TRUE);
    }
  else
    {
      self->n_stops = parse_data.stop_array->len;
      self->color_stops = (BobguiCssImageConicColorStop *)g_array_free (parse_data.stop_array, FALSE);
    }

  return success;
}

static void
bobgui_css_image_conic_print (BobguiCssImage *image,
                           GString     *string)
{
  BobguiCssImageConic *self = BOBGUI_CSS_IMAGE_CONIC (image);
  gboolean written = FALSE;
  guint i;

  g_string_append (string, "conic-gradient(");

  if (self->center)
    {
      BobguiCssValue *compare = _bobgui_css_position_value_new (bobgui_css_number_value_new (50, BOBGUI_CSS_PERCENT),
                                                          bobgui_css_number_value_new (50, BOBGUI_CSS_PERCENT));

      if (!bobgui_css_value_equal (self->center, compare))
        {
          g_string_append (string, "at ");
          bobgui_css_value_print (self->center, string);
          written = TRUE;
        }

      bobgui_css_value_unref (compare);
    }

  if (self->rotation && bobgui_css_number_value_get (self->rotation, 360) != 0)
    {
      if (written)
        g_string_append_c (string, ' ');
      g_string_append (string, "from ");
      bobgui_css_value_print (self->rotation, string);
      written = TRUE;
    }

  if (self->color_space != BOBGUI_CSS_COLOR_SPACE_SRGB)
    {
      if (written)
        g_string_append_c (string, ' ');
      bobgui_css_color_interpolation_method_print (self->color_space,
                                                self->hue_interp,
                                                string);
      written = TRUE;
    }

  if (written)
    g_string_append (string, ", ");

  for (i = 0; i < self->n_stops; i++)
    {
      const BobguiCssImageConicColorStop *stop = &self->color_stops[i];

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
bobgui_css_image_conic_compute (BobguiCssImage          *image,
                             guint                 property_id,
                             BobguiCssComputeContext *context)
{
  BobguiCssImageConic *self = BOBGUI_CSS_IMAGE_CONIC (image);
  BobguiCssImageConic *copy;
  guint i;

  copy = g_object_new (BOBGUI_TYPE_CSS_IMAGE_CONIC, NULL);

  copy->center = bobgui_css_value_compute (self->center, property_id, context);
  copy->rotation = bobgui_css_value_compute (self->rotation, property_id, context);
  copy->color_space = self->color_space;
  copy->hue_interp = self->hue_interp;

  copy->n_stops = self->n_stops;
  copy->color_stops = g_new (BobguiCssImageConicColorStop, self->n_stops);
  for (i = 0; i < self->n_stops; i++)
    {
      const BobguiCssImageConicColorStop *stop = &self->color_stops[i];
      BobguiCssImageConicColorStop *scopy = &copy->color_stops[i];

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
bobgui_css_image_conic_transition (BobguiCssImage *start_image,
                                BobguiCssImage *end_image,
                                guint        property_id,
                                double       progress)
{
  BobguiCssImageConic *start, *end, *result;
  guint i;

  start = BOBGUI_CSS_IMAGE_CONIC (start_image);

  if (end_image == NULL)
    return BOBGUI_CSS_IMAGE_CLASS (bobgui_css_image_conic_parent_class)->transition (start_image, end_image, property_id, progress);

  if (!BOBGUI_IS_CSS_IMAGE_CONIC (end_image))
    return BOBGUI_CSS_IMAGE_CLASS (bobgui_css_image_conic_parent_class)->transition (start_image, end_image, property_id, progress);

  end = BOBGUI_CSS_IMAGE_CONIC (end_image);

  if (start->n_stops != end->n_stops)
    return BOBGUI_CSS_IMAGE_CLASS (bobgui_css_image_conic_parent_class)->transition (start_image, end_image, property_id, progress);

  result = g_object_new (BOBGUI_TYPE_CSS_IMAGE_CONIC, NULL);

  result->center = bobgui_css_value_transition (start->center, end->center, property_id, progress);
  if (result->center == NULL)
    goto fail;

  result->rotation = bobgui_css_value_transition (start->rotation, end->rotation, property_id, progress);
  if (result->rotation == NULL)
    goto fail;

  result->color_space = start->color_space;
  result->hue_interp = start->hue_interp;

  result->color_stops = g_malloc (sizeof (BobguiCssImageConicColorStop) * start->n_stops);
  result->n_stops = 0;
  for (i = 0; i < start->n_stops; i++)
    {
      const BobguiCssImageConicColorStop *start_stop = &start->color_stops[i];
      const BobguiCssImageConicColorStop *end_stop = &end->color_stops[i];
      BobguiCssImageConicColorStop *stop = &result->color_stops[i];

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
  return BOBGUI_CSS_IMAGE_CLASS (bobgui_css_image_conic_parent_class)->transition (start_image, end_image, property_id, progress);
}

static gboolean
bobgui_css_image_conic_equal (BobguiCssImage *image1,
                            BobguiCssImage *image2)
{
  BobguiCssImageConic *conic1 = (BobguiCssImageConic *) image1;
  BobguiCssImageConic *conic2 = (BobguiCssImageConic *) image2;
  guint i;

  if (!bobgui_css_value_equal (conic1->center, conic2->center) ||
      !bobgui_css_value_equal (conic1->rotation, conic2->rotation) ||
      conic1->color_space != conic2->color_space ||
      conic1->hue_interp != conic2->hue_interp)
    return FALSE;

  for (i = 0; i < conic1->n_stops; i++)
    {
      const BobguiCssImageConicColorStop *stop1 = &conic1->color_stops[i];
      const BobguiCssImageConicColorStop *stop2 = &conic2->color_stops[i];

      if (!bobgui_css_value_equal0 (stop1->offset, stop2->offset) ||
          !bobgui_css_value_equal0 (stop1->transition_hint, stop2->transition_hint) ||
          !bobgui_css_value_equal (stop1->color, stop2->color))
        return FALSE;
    }

  return TRUE;
}

static void
bobgui_css_image_conic_dispose (GObject *object)
{
  BobguiCssImageConic *self = BOBGUI_CSS_IMAGE_CONIC (object);
  guint i;

  for (i = 0; i < self->n_stops; i ++)
    {
      BobguiCssImageConicColorStop *stop = &self->color_stops[i];

      bobgui_css_value_unref (stop->color);
      if (stop->offset)
        bobgui_css_value_unref (stop->offset);
      if (stop->transition_hint)
        bobgui_css_value_unref (stop->transition_hint);
    }
  g_free (self->color_stops);

  g_clear_pointer (&self->center, bobgui_css_value_unref);
  g_clear_pointer (&self->rotation, bobgui_css_value_unref);

  G_OBJECT_CLASS (bobgui_css_image_conic_parent_class)->dispose (object);
}

static gboolean
bobgui_css_image_conic_is_computed (BobguiCssImage *image)
{
  BobguiCssImageConic *self = BOBGUI_CSS_IMAGE_CONIC (image);
  guint i;
  gboolean computed = TRUE;

  computed = !self->center || bobgui_css_value_is_computed (self->center);
  computed &= !self->rotation || bobgui_css_value_is_computed (self->rotation);

  if (computed)
    for (i = 0; i < self->n_stops; i ++)
      {
        const BobguiCssImageConicColorStop *stop = &self->color_stops[i];

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
bobgui_css_image_conic_contains_current_color (BobguiCssImage *image)
{
  BobguiCssImageConic *self = BOBGUI_CSS_IMAGE_CONIC (image);

  for (guint i = 0; i < self->n_stops; i ++)
    {
      const BobguiCssImageConicColorStop *stop = &self->color_stops[i];

      if (bobgui_css_value_contains_current_color (stop->color))
        return TRUE;
    }

  return FALSE;
}

static BobguiCssImage *
bobgui_css_image_conic_resolve (BobguiCssImage          *image,
                             BobguiCssComputeContext *context,
                             BobguiCssValue          *current_color)
{
  BobguiCssImageConic *self = BOBGUI_CSS_IMAGE_CONIC (image);
  BobguiCssImageConic *resolved;

  if (!bobgui_css_image_conic_contains_current_color (image))
    return g_object_ref (image);

  resolved = g_object_new (BOBGUI_TYPE_CSS_IMAGE_CONIC, NULL);

  resolved->center = bobgui_css_value_ref (self->center);
  resolved->rotation = bobgui_css_value_ref (self->rotation);

  resolved->n_stops = self->n_stops;
  resolved->color_stops = g_new (BobguiCssImageConicColorStop, self->n_stops);

  for (guint i = 0; i < self->n_stops; i++)
    {
      if (self->color_stops[i].transition_hint)
        resolved->color_stops[i].transition_hint = bobgui_css_value_ref (self->color_stops[i].transition_hint);
      else
        resolved->color_stops[i].transition_hint = NULL;

      if (self->color_stops[i].offset)
        resolved->color_stops[i].offset = bobgui_css_value_ref (self->color_stops[i].offset);
      else
        resolved->color_stops[i].offset = NULL;

      resolved->color_stops[i].color = bobgui_css_value_resolve (self->color_stops[i].color, context, current_color);
    }

  return BOBGUI_CSS_IMAGE (resolved);
}

static void
bobgui_css_image_conic_class_init (BobguiCssImageConicClass *klass)
{
  BobguiCssImageClass *image_class = BOBGUI_CSS_IMAGE_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  image_class->snapshot = bobgui_css_image_conic_snapshot;
  image_class->parse = bobgui_css_image_conic_parse;
  image_class->print = bobgui_css_image_conic_print;
  image_class->compute = bobgui_css_image_conic_compute;
  image_class->equal = bobgui_css_image_conic_equal;
  image_class->transition = bobgui_css_image_conic_transition;
  image_class->is_computed = bobgui_css_image_conic_is_computed;
  image_class->contains_current_color = bobgui_css_image_conic_contains_current_color;
  image_class->resolve = bobgui_css_image_conic_resolve;

  object_class->dispose = bobgui_css_image_conic_dispose;
}

static void
bobgui_css_image_conic_init (BobguiCssImageConic *self)
{
}

