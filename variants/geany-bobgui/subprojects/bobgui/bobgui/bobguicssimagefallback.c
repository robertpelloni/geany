/*
 * Copyright © 2016 Red Hat Inc.
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

#include "bobguicssimagefallbackprivate.h"
#include "bobguicsscolorvalueprivate.h"
#include "bobguicsscolorvalueprivate.h"

#include "bobguistyleproviderprivate.h"

G_DEFINE_TYPE (BobguiCssImageFallback, _bobgui_css_image_fallback, BOBGUI_TYPE_CSS_IMAGE)

static int
bobgui_css_image_fallback_get_width (BobguiCssImage *image)
{
  BobguiCssImageFallback *fallback = BOBGUI_CSS_IMAGE_FALLBACK (image);

  if (fallback->used < 0)
    return 0;

  return _bobgui_css_image_get_width (fallback->images[fallback->used]);
}

static int
bobgui_css_image_fallback_get_height (BobguiCssImage *image)
{
  BobguiCssImageFallback *fallback = BOBGUI_CSS_IMAGE_FALLBACK (image);

  if (fallback->used < 0)
    return 0;

  return _bobgui_css_image_get_height (fallback->images[fallback->used]);
}

static double
bobgui_css_image_fallback_get_aspect_ratio (BobguiCssImage *image)
{
  BobguiCssImageFallback *fallback = BOBGUI_CSS_IMAGE_FALLBACK (image);

  if (fallback->used < 0)
    return 0;

  return _bobgui_css_image_get_aspect_ratio (fallback->images[fallback->used]);
}

static void
bobgui_css_image_fallback_snapshot (BobguiCssImage *image,
                                 BobguiSnapshot *snapshot,
                                 double       width,
                                 double       height)
{
  BobguiCssImageFallback *fallback = BOBGUI_CSS_IMAGE_FALLBACK (image);

  if (fallback->used < 0)
    {
      if (fallback->color)
        {
          const GdkRGBA *color;

          color = bobgui_css_color_value_get_rgba (fallback->color);

          if (!gdk_rgba_is_clear (color))
            bobgui_snapshot_append_color (snapshot, color,
                                       &GRAPHENE_RECT_INIT (0, 0, width, height));
        }
      else
        {
          bobgui_snapshot_append_color (snapshot, &(GdkRGBA) {1, 0, 0, 1},
                                     &GRAPHENE_RECT_INIT (0, 0, width, height));
        }
    }
  else
    bobgui_css_image_snapshot (fallback->images[fallback->used], snapshot, width, height);
}

static void
bobgui_css_image_fallback_print (BobguiCssImage *image,
                              GString     *string)
{
  BobguiCssImageFallback *fallback = BOBGUI_CSS_IMAGE_FALLBACK (image);
  int i;

  g_string_append (string, "image(");
  for (i = 0; i < fallback->n_images; i++)
    {
      if (i > 0)
        g_string_append (string, ",");
      _bobgui_css_image_print (fallback->images[i], string);
    }
  if (fallback->color)
    {
      if (fallback->n_images > 0)
        g_string_append (string, ",");
      bobgui_css_value_print (fallback->color, string);
    }

  g_string_append (string, ")");
}

static void
bobgui_css_image_fallback_dispose (GObject *object)
{
  BobguiCssImageFallback *fallback = BOBGUI_CSS_IMAGE_FALLBACK (object);
  int i;

  for (i = 0; i < fallback->n_images; i++)
    g_object_unref (fallback->images[i]);
  g_free (fallback->images);
  fallback->images = NULL;

  if (fallback->color)
    {
      bobgui_css_value_unref (fallback->color);
      fallback->color = NULL;
    }

  G_OBJECT_CLASS (_bobgui_css_image_fallback_parent_class)->dispose (object);
}


static BobguiCssImage *
bobgui_css_image_fallback_compute (BobguiCssImage          *image,
                                guint                 property_id,
                                BobguiCssComputeContext *context)
{
  BobguiCssImageFallback *fallback = BOBGUI_CSS_IMAGE_FALLBACK (image);
  BobguiCssImageFallback *copy;
  int i;

  if (fallback->used < 0)
    {
      BobguiCssValue *computed_color = NULL;

      if (fallback->color)
        computed_color = bobgui_css_value_compute (fallback->color,
                                                property_id,
                                                context);

      /* image($color) that didn't change */
      if (computed_color && !fallback->images &&
          computed_color == fallback->color)
        return g_object_ref (image);

      copy = g_object_new (_bobgui_css_image_fallback_get_type (), NULL);
      copy->n_images = fallback->n_images;
      copy->images = g_new (BobguiCssImage *, fallback->n_images);
      for (i = 0; i < fallback->n_images; i++)
        {
          copy->images[i] = _bobgui_css_image_compute (fallback->images[i],
                                                    property_id,
                                                    context);

          if (bobgui_css_image_is_invalid (copy->images[i]))
            continue;

          if (copy->used < 0)
            copy->used = i;
        }

      copy->color = computed_color;

      return BOBGUI_CSS_IMAGE (copy);
    }
  else
    return BOBGUI_CSS_IMAGE (g_object_ref (fallback));
}

typedef struct
{
  BobguiCssValue *color;
  GPtrArray *images;
} ParseData;

static guint
bobgui_css_image_fallback_parse_arg (BobguiCssParser *parser,
                                  guint         arg,
                                  gpointer      _data)
{
  ParseData *data = _data;

  if (data->color != NULL)
    {
      bobgui_css_parser_error_syntax (parser, "The color must be the last parameter");
      return 0;
    }
  else if (_bobgui_css_image_can_parse (parser))
    {
      BobguiCssImage *image = _bobgui_css_image_new_parse (parser);
      if (image == NULL)
        return 0;

      if (!data->images)
        data->images = g_ptr_array_new_with_free_func (g_object_unref);

      g_ptr_array_add (data->images, image);
      return 1;
    }
  else
    {
      data->color = bobgui_css_color_value_parse (parser);
      if (data->color == NULL)
        return 0;

      return 1;
    }
}

static gboolean
bobgui_css_image_fallback_parse (BobguiCssImage  *image,
                              BobguiCssParser *parser)
{
  BobguiCssImageFallback *self = BOBGUI_CSS_IMAGE_FALLBACK (image);
  ParseData data = { NULL, NULL };

  if (!bobgui_css_parser_has_function (parser, "image"))
    {
      bobgui_css_parser_error_syntax (parser, "Expected 'image('");
      return FALSE;
    }

  if (!bobgui_css_parser_consume_function (parser, 1, G_MAXUINT, bobgui_css_image_fallback_parse_arg, &data))
    {
      g_clear_pointer (&data.color, bobgui_css_value_unref);
      if (data.images)
        g_ptr_array_free (data.images, TRUE);
      return FALSE;
    }

  self->color = data.color;
  if (data.images)
    {
      self->n_images = data.images->len;
      self->images = (BobguiCssImage **) g_ptr_array_free (data.images, FALSE);
    }
  else
    {
      self->n_images = 0;
      self->images = NULL;
    }

  return TRUE;
}

static gboolean
bobgui_css_image_fallback_equal (BobguiCssImage *image1,
                              BobguiCssImage *image2)
{
  BobguiCssImageFallback *fallback1 = BOBGUI_CSS_IMAGE_FALLBACK (image1);
  BobguiCssImageFallback *fallback2 = BOBGUI_CSS_IMAGE_FALLBACK (image2);

  if (fallback1->used < 0)
    {
      if (fallback2->used >= 0)
        return FALSE;

      return bobgui_css_value_equal (fallback1->color, fallback2->color);
    }

  if (fallback2->used < 0)
    return FALSE;

  return _bobgui_css_image_equal (fallback1->images[fallback1->used],
                               fallback2->images[fallback2->used]);
}

static gboolean
bobgui_css_image_fallback_is_computed (BobguiCssImage *image)
{
  BobguiCssImageFallback *fallback = BOBGUI_CSS_IMAGE_FALLBACK (image);

  if (fallback->used < 0)
    {
      guint i;

      if (fallback->color && !fallback->images)
        return bobgui_css_value_is_computed (fallback->color);

      for (i = 0; i < fallback->n_images; i++)
        {
          if (!bobgui_css_image_is_computed (fallback->images[i]))
            {
              return FALSE;
            }
        }
    }

  return TRUE;
}

static gboolean
bobgui_css_image_fallback_contains_current_color (BobguiCssImage *image)
{
  BobguiCssImageFallback *fallback = BOBGUI_CSS_IMAGE_FALLBACK (image);

  if (fallback->used < 0)
    {
      guint i;

      if (fallback->color && !fallback->images)
        return bobgui_css_value_contains_current_color (fallback->color);

      for (i = 0; i < fallback->n_images; i++)
        {
          if (bobgui_css_image_contains_current_color (fallback->images[i]))
            return TRUE;
        }

      return FALSE;
    }

  return bobgui_css_image_contains_current_color (fallback->images[fallback->used]);
}

static BobguiCssImage *
bobgui_css_image_fallback_resolve (BobguiCssImage          *image,
                                BobguiCssComputeContext *context,
                                BobguiCssValue          *current_color)
{
  BobguiCssImageFallback *fallback = BOBGUI_CSS_IMAGE_FALLBACK (image);
  BobguiCssImageFallback *resolved;
  int i;

  if (!bobgui_css_image_fallback_contains_current_color (image))
    return g_object_ref (image);

  if (fallback->used < 0)
    {
      BobguiCssValue *resolved_color = NULL;

      if (fallback->color)
        resolved_color = bobgui_css_value_resolve (fallback->color, context, current_color);

      /* image($color) that didn't change */
      if (resolved_color && !fallback->images && resolved_color == fallback->color)
        return g_object_ref (image);

      resolved = g_object_new (_bobgui_css_image_fallback_get_type (), NULL);
      resolved->n_images = fallback->n_images;
      resolved->images = g_new (BobguiCssImage *, fallback->n_images);
      for (i = 0; i < fallback->n_images; i++)
        {
          resolved->images[i] = bobgui_css_image_resolve (fallback->images[i], context, current_color);

          if (bobgui_css_image_is_invalid (resolved->images[i]))
            continue;

          if (resolved->used < 0)
            resolved->used = i;
        }

      resolved->color = resolved_color;

      return BOBGUI_CSS_IMAGE (resolved);
    }
  else
    return bobgui_css_image_resolve (fallback->images[fallback->used], context, current_color);
}

static void
_bobgui_css_image_fallback_class_init (BobguiCssImageFallbackClass *klass)
{
  BobguiCssImageClass *image_class = BOBGUI_CSS_IMAGE_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  image_class->get_width = bobgui_css_image_fallback_get_width;
  image_class->get_height = bobgui_css_image_fallback_get_height;
  image_class->get_aspect_ratio = bobgui_css_image_fallback_get_aspect_ratio;
  image_class->snapshot = bobgui_css_image_fallback_snapshot;
  image_class->parse = bobgui_css_image_fallback_parse;
  image_class->compute = bobgui_css_image_fallback_compute;
  image_class->print = bobgui_css_image_fallback_print;
  image_class->equal = bobgui_css_image_fallback_equal;
  image_class->is_computed = bobgui_css_image_fallback_is_computed;
  image_class->contains_current_color = bobgui_css_image_fallback_contains_current_color;
  image_class->resolve = bobgui_css_image_fallback_resolve;

  object_class->dispose = bobgui_css_image_fallback_dispose;
}

static void
_bobgui_css_image_fallback_init (BobguiCssImageFallback *image_fallback)
{
  image_fallback->used = -1;
}

BobguiCssImage *
_bobgui_css_image_fallback_new_for_color (BobguiCssValue *color)
{
  BobguiCssImageFallback *image;

  image = g_object_new (BOBGUI_TYPE_CSS_IMAGE_FALLBACK, NULL);
  image->color = bobgui_css_value_ref (color);

  return (BobguiCssImage *)image;
}
