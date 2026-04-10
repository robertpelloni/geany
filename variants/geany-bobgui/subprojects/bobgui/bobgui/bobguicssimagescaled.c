/*
 * Copyright © 2013 Red Hat Inc.
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
 * Authors: Alexander Larsson <alexl@gnome.org>
 */

#include "config.h"

#include "bobguicssimagescaledprivate.h"

#include "bobguistyleproviderprivate.h"

G_DEFINE_TYPE (BobguiCssImageScaled, _bobgui_css_image_scaled, BOBGUI_TYPE_CSS_IMAGE)

static int
bobgui_css_image_scaled_get_width (BobguiCssImage *image)
{
  BobguiCssImageScaled *scaled = BOBGUI_CSS_IMAGE_SCALED (image);

  return _bobgui_css_image_get_width (scaled->images[0])/scaled->scales[0];
}

static int
bobgui_css_image_scaled_get_height (BobguiCssImage *image)
{
  BobguiCssImageScaled *scaled = BOBGUI_CSS_IMAGE_SCALED (image);

  return _bobgui_css_image_get_height (scaled->images[0])/scaled->scales[0];
}

static double
bobgui_css_image_scaled_get_aspect_ratio (BobguiCssImage *image)
{
  BobguiCssImageScaled *scaled = BOBGUI_CSS_IMAGE_SCALED (image);

  return _bobgui_css_image_get_aspect_ratio (scaled->images[0]);
}

static void
bobgui_css_image_scaled_snapshot (BobguiCssImage *image,
                               BobguiSnapshot *snapshot,
                               double       width,
                               double       height)
{
  BobguiCssImageScaled *scaled = BOBGUI_CSS_IMAGE_SCALED (image);

  bobgui_css_image_snapshot (scaled->images[0], snapshot, width, height);
  // FIXME apply scale
}

static void
bobgui_css_image_scaled_print (BobguiCssImage *image,
                             GString     *string)
{
  BobguiCssImageScaled *scaled = BOBGUI_CSS_IMAGE_SCALED (image);
  int i;
  
  g_string_append (string, "-bobgui-scaled(");
  for (i = 0; i < scaled->n_images; i++)
    {
      _bobgui_css_image_print (scaled->images[i], string);
      g_string_append_printf (string, ",%d", scaled->scales[i]);
      if (i != scaled->n_images - 1)
        g_string_append (string, ",");
    }
  g_string_append (string, ")");
}

static void
bobgui_css_image_scaled_dispose (GObject *object)
{
  BobguiCssImageScaled *scaled = BOBGUI_CSS_IMAGE_SCALED (object);
  int i;

  for (i = 0; i < scaled->n_images; i++)
    g_object_unref (scaled->images[i]);
  g_free (scaled->images);
  scaled->images = NULL;
  g_free (scaled->scales);
  scaled->scales = NULL;

  G_OBJECT_CLASS (_bobgui_css_image_scaled_parent_class)->dispose (object);
}


static BobguiCssImage *
bobgui_css_image_scaled_compute (BobguiCssImage          *image,
                              guint                 property_id,
                              BobguiCssComputeContext *context)
{
  BobguiCssImageScaled *scaled = BOBGUI_CSS_IMAGE_SCALED (image);
  int scale;
  BobguiCssImageScaled *res;
  int i;
  int best;

  scale = bobgui_style_provider_get_scale (context->provider);
  scale = MAX(scale, 1);

  best = 0;
  for (i = 0; i < scaled->n_images; i++)
    {
      if (scaled->scales[i] == scale)
        {
          best = i;
          break;
        }
      else if ((scaled->scales[best] < scaled->scales[i] && scaled->scales[i] < scale) ||
               (scale < scaled->scales[i] && scaled->scales[i] < scaled->scales[best]) ||
               (scaled->scales[best] < scale && scaled->scales[i] > scale))
        {
          best = i;
        }
    }

  res = g_object_new (BOBGUI_TYPE_CSS_IMAGE_SCALED, NULL);
  res->n_images = 1;
  res->images = g_new (BobguiCssImage *, 1);
  res->scales = g_new (int, 1);

  res->images[0] = _bobgui_css_image_compute (scaled->images[best],
                                           property_id,
                                           context);
  res->scales[0] = scaled->scales[best];

  return BOBGUI_CSS_IMAGE (res);
}

typedef struct
{
  GPtrArray *images;
  GArray *scales;
} BobguiCssImageScaledParseData;

static guint
bobgui_css_image_scaled_parse_arg (BobguiCssParser *parser,
                                guint         arg,
                                gpointer      data_)
{
  BobguiCssImageScaledParseData *data = data_;
  BobguiCssImage *child;
  int scale;

  child = _bobgui_css_image_new_parse (parser);
  if (child == NULL)
    return 0;

  if (!bobgui_css_parser_has_integer (parser))
    scale = arg > 0 ? g_array_index (data->scales, int, arg - 1) + 1 : 1;
  else if (!bobgui_css_parser_consume_integer (parser, &scale))
    return 0;

  g_ptr_array_add (data->images, child);
  g_array_append_val (data->scales, scale);

  return 1;
}

static gboolean
bobgui_css_image_scaled_parse (BobguiCssImage  *image,
                            BobguiCssParser *parser)
{
  BobguiCssImageScaled *self = BOBGUI_CSS_IMAGE_SCALED (image);
  BobguiCssImageScaledParseData data;

  if (!bobgui_css_parser_has_function (parser, "-bobgui-scaled"))
    {
      bobgui_css_parser_error_syntax (parser, "Expected '-bobgui-scaled('");
      return FALSE;
    }

  data.images = g_ptr_array_new_with_free_func (g_object_unref);
  data.scales = g_array_new (FALSE, FALSE, sizeof (int));

  if (!bobgui_css_parser_consume_function (parser, 1, G_MAXUINT, bobgui_css_image_scaled_parse_arg, &data))
    {
      g_ptr_array_unref (data.images);
      g_array_unref (data.scales);
      return FALSE;
    }

  self->n_images = data.images->len;
  self->images = (BobguiCssImage **) g_ptr_array_free (data.images, FALSE);
  self->scales = (int *) g_array_free (data.scales, FALSE);

  return TRUE;
}

static gboolean
bobgui_css_image_scaled_is_computed (BobguiCssImage *image)
{
  BobguiCssImageScaled *self = BOBGUI_CSS_IMAGE_SCALED (image);

  return self->n_images == 1 &&
         bobgui_css_image_is_computed (self->images[0]);
}

static gboolean
bobgui_css_image_scaled_contains_current_color (BobguiCssImage *image)
{
  BobguiCssImageScaled *self = BOBGUI_CSS_IMAGE_SCALED (image);

  for (guint i = 0; i < self->n_images; i++)
    {
      if (bobgui_css_image_contains_current_color (self->images[i]))
        return TRUE;
    }

  return FALSE;
}

static BobguiCssImage *
bobgui_css_image_scaled_resolve (BobguiCssImage          *image,
                              BobguiCssComputeContext *context,
                              BobguiCssValue          *current_color)
{
  BobguiCssImageScaled *self = BOBGUI_CSS_IMAGE_SCALED (image);
  BobguiCssImageScaled *res;

  if (!bobgui_css_image_scaled_contains_current_color (image))
    return g_object_ref (image);

  res = g_object_new (BOBGUI_TYPE_CSS_IMAGE_SCALED, NULL);

  res->n_images = self->n_images;
  res->images = g_new (BobguiCssImage *, self->n_images);
  res->scales = g_new (int, self->n_images);

  for (guint i = 0; i < self->n_images; i++)
    {
      res->images[i] = bobgui_css_image_resolve (self->images[i], context, current_color);
      res->scales[i] = self->scales[i];
    }

  return BOBGUI_CSS_IMAGE (res);
}

static void
_bobgui_css_image_scaled_class_init (BobguiCssImageScaledClass *klass)
{
  BobguiCssImageClass *image_class = BOBGUI_CSS_IMAGE_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  image_class->get_width = bobgui_css_image_scaled_get_width;
  image_class->get_height = bobgui_css_image_scaled_get_height;
  image_class->get_aspect_ratio = bobgui_css_image_scaled_get_aspect_ratio;
  image_class->snapshot = bobgui_css_image_scaled_snapshot;
  image_class->parse = bobgui_css_image_scaled_parse;
  image_class->compute = bobgui_css_image_scaled_compute;
  image_class->print = bobgui_css_image_scaled_print;
  image_class->is_computed = bobgui_css_image_scaled_is_computed;
  image_class->contains_current_color = bobgui_css_image_scaled_contains_current_color;
  image_class->resolve = bobgui_css_image_scaled_resolve;

  object_class->dispose = bobgui_css_image_scaled_dispose;
}

static void
_bobgui_css_image_scaled_init (BobguiCssImageScaled *image_scaled)
{
}
