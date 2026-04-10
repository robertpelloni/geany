/*
 * Copyright © 2011 Red Hat Inc.
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

#include "bobguicssimageprivate.h"

#include "bobguicssstyleprivate.h"
#include "bobguisnapshot.h"
#include "bobguiprivate.h"

/* for the types only */
#include "bobgui/bobguicssimageconicprivate.h"
#include "bobgui/bobguicssimagecrossfadeprivate.h"
#include "bobgui/bobguicssimagefallbackprivate.h"
#include "bobgui/bobguicssimageiconthemeprivate.h"
#include "bobgui/bobguicssimagelinearprivate.h"
#include "bobgui/bobguicssimageradialprivate.h"
#include "bobgui/bobguicssimageurlprivate.h"
#include "bobgui/bobguicssimagescaledprivate.h"
#include "bobgui/bobguicssimagerecolorprivate.h"

G_DEFINE_ABSTRACT_TYPE (BobguiCssImage, _bobgui_css_image, G_TYPE_OBJECT)

static int
bobgui_css_image_real_get_width (BobguiCssImage *image)
{
  return 0;
}

static int
bobgui_css_image_real_get_height (BobguiCssImage *image)
{
  return 0;
}

static double
bobgui_css_image_real_get_aspect_ratio (BobguiCssImage *image)
{
  int width, height;

  width = _bobgui_css_image_get_width (image);
  height = _bobgui_css_image_get_height (image);

  if (width && height)
    return (double) width / height;
  else
    return 0;
}

static BobguiCssImage *
bobgui_css_image_real_compute (BobguiCssImage          *image,
                            guint                 property_id,
                            BobguiCssComputeContext *context)
{
  return g_object_ref (image);
}

static gboolean
bobgui_css_image_real_equal (BobguiCssImage *image1,
                          BobguiCssImage *image2)
{
  return FALSE;
}

static BobguiCssImage *
bobgui_css_image_real_transition (BobguiCssImage *start,
                               BobguiCssImage *end,
                               guint        property_id,
                               double       progress)
{
  if (progress <= 0.0)
    return g_object_ref (start);
  else if (progress >= 1.0)
    return end ? g_object_ref (end) : NULL;
  else if (_bobgui_css_image_equal (start, end))
    return g_object_ref (start);
  else
    return _bobgui_css_image_cross_fade_new (start, end, progress);
}

static gboolean
bobgui_css_image_real_is_invalid (BobguiCssImage *image)
{
  return FALSE;
}

static gboolean
bobgui_css_image_real_is_dynamic (BobguiCssImage *image)
{
  return FALSE;
}

static BobguiCssImage *
bobgui_css_image_real_get_dynamic_image (BobguiCssImage *image,
                                      gint64       monotonic_time)
{
  return g_object_ref (image);
}

static gboolean
bobgui_css_image_real_is_computed (BobguiCssImage *image)
{
  return FALSE;
}

static gboolean
bobgui_css_image_real_contains_current_color (BobguiCssImage *image)
{
  return FALSE;
}

static BobguiCssImage *
bobgui_css_image_real_resolve (BobguiCssImage          *image,
                            BobguiCssComputeContext *context,
                            BobguiCssValue          *current_color)
{
  return g_object_ref (image);
}

static void
_bobgui_css_image_class_init (BobguiCssImageClass *klass)
{
  klass->get_width = bobgui_css_image_real_get_width;
  klass->get_height = bobgui_css_image_real_get_height;
  klass->get_aspect_ratio = bobgui_css_image_real_get_aspect_ratio;
  klass->compute = bobgui_css_image_real_compute;
  klass->equal = bobgui_css_image_real_equal;
  klass->transition = bobgui_css_image_real_transition;
  klass->is_invalid = bobgui_css_image_real_is_invalid;
  klass->is_dynamic = bobgui_css_image_real_is_dynamic;
  klass->get_dynamic_image = bobgui_css_image_real_get_dynamic_image;
  klass->is_computed = bobgui_css_image_real_is_computed;
  klass->contains_current_color = bobgui_css_image_real_contains_current_color;
  klass->resolve = bobgui_css_image_real_resolve;
}

static void
_bobgui_css_image_init (BobguiCssImage *image)
{
}

int
_bobgui_css_image_get_width (BobguiCssImage *image)
{
  BobguiCssImageClass *klass;

  klass = BOBGUI_CSS_IMAGE_GET_CLASS (image);

  return klass->get_width (image);
}

int
_bobgui_css_image_get_height (BobguiCssImage *image)
{
  BobguiCssImageClass *klass;

  klass = BOBGUI_CSS_IMAGE_GET_CLASS (image);

  return klass->get_height (image);
}

double
_bobgui_css_image_get_aspect_ratio (BobguiCssImage *image)
{
  BobguiCssImageClass *klass;

  klass = BOBGUI_CSS_IMAGE_GET_CLASS (image);

  return klass->get_aspect_ratio (image);
}

BobguiCssImage *
_bobgui_css_image_compute (BobguiCssImage          *image,
                        guint                 property_id,
                        BobguiCssComputeContext *context)
{
  BobguiCssImageClass *klass;

  bobgui_internal_return_val_if_fail (BOBGUI_IS_CSS_IMAGE (image), NULL);
  bobgui_internal_return_val_if_fail (BOBGUI_IS_CSS_STYLE (context->style), NULL);
  bobgui_internal_return_val_if_fail (context->parent_style == NULL || BOBGUI_IS_CSS_STYLE (context->parent_style), NULL);

  klass = BOBGUI_CSS_IMAGE_GET_CLASS (image);

  return klass->compute (image, property_id, context);
}

BobguiCssImage *
_bobgui_css_image_transition (BobguiCssImage *start,
                           BobguiCssImage *end,
                           guint        property_id,
                           double       progress)
{
  BobguiCssImageClass *klass;

  bobgui_internal_return_val_if_fail (start == NULL || BOBGUI_IS_CSS_IMAGE (start), NULL);
  bobgui_internal_return_val_if_fail (end == NULL || BOBGUI_IS_CSS_IMAGE (end), NULL);

  progress = CLAMP (progress, 0.0, 1.0);

  if (start == NULL)
    {
      if (end == NULL)
        return NULL;
      else
        {
          start = end;
          end = NULL;
          progress = 1.0 - progress;
        }
    }

  klass = BOBGUI_CSS_IMAGE_GET_CLASS (start);

  return klass->transition (start, end, property_id, progress);
}

gboolean
_bobgui_css_image_equal (BobguiCssImage *image1,
                      BobguiCssImage *image2)
{
  BobguiCssImageClass *klass;

  bobgui_internal_return_val_if_fail (image1 == NULL || BOBGUI_IS_CSS_IMAGE (image1), FALSE);
  bobgui_internal_return_val_if_fail (image2 == NULL || BOBGUI_IS_CSS_IMAGE (image2), FALSE);

  if (image1 == image2)
    return TRUE;

  if (image1 == NULL || image2 == NULL)
    return FALSE;

  if (G_OBJECT_TYPE (image1) != G_OBJECT_TYPE (image2))
    return FALSE;

  klass = BOBGUI_CSS_IMAGE_GET_CLASS (image1);

  return klass->equal (image1, image2);
}

static void
bobgui_css_image_draw (BobguiCssImage *image,
                    cairo_t     *cr,
                    double       width,
                    double       height)
{
  BobguiSnapshot *snapshot;
  GskRenderNode *node;

  bobgui_internal_return_if_fail (BOBGUI_IS_CSS_IMAGE (image));
  bobgui_internal_return_if_fail (cr != NULL);
  bobgui_internal_return_if_fail (width > 0);
  bobgui_internal_return_if_fail (height > 0);

  cairo_save (cr);

  snapshot = bobgui_snapshot_new ();
  bobgui_css_image_snapshot (image, snapshot, width, height);
  node = bobgui_snapshot_free_to_node (snapshot);

  if (node != NULL)
    {
      gsk_render_node_draw (node, cr);
      gsk_render_node_unref (node);
    }

  cairo_restore (cr);
}

void
bobgui_css_image_snapshot (BobguiCssImage *image,
                        BobguiSnapshot *snapshot,
                        double       width,
                        double       height)
{
  BobguiCssImageClass *klass;

  bobgui_internal_return_if_fail (BOBGUI_IS_CSS_IMAGE (image));
  bobgui_internal_return_if_fail (snapshot != NULL);
  bobgui_internal_return_if_fail (width > 0);
  bobgui_internal_return_if_fail (height > 0);

  klass = BOBGUI_CSS_IMAGE_GET_CLASS (image);

  klass->snapshot (image, snapshot, width, height);
}

gboolean
bobgui_css_image_is_invalid (BobguiCssImage *image)
{
  BobguiCssImageClass *klass;

  bobgui_internal_return_val_if_fail (BOBGUI_IS_CSS_IMAGE (image), FALSE);

  klass = BOBGUI_CSS_IMAGE_GET_CLASS (image);

  return klass->is_invalid (image);
}

gboolean
bobgui_css_image_is_dynamic (BobguiCssImage *image)
{
  BobguiCssImageClass *klass;

  bobgui_internal_return_val_if_fail (BOBGUI_IS_CSS_IMAGE (image), FALSE);

  klass = BOBGUI_CSS_IMAGE_GET_CLASS (image);

  return klass->is_dynamic (image);
}

BobguiCssImage *
bobgui_css_image_get_dynamic_image (BobguiCssImage *image,
                                 gint64       monotonic_time)
{
  BobguiCssImageClass *klass;

  bobgui_internal_return_val_if_fail (BOBGUI_IS_CSS_IMAGE (image), NULL);

  klass = BOBGUI_CSS_IMAGE_GET_CLASS (image);

  return klass->get_dynamic_image (image, monotonic_time);
}

void
_bobgui_css_image_print (BobguiCssImage *image,
                      GString     *string)
{
  BobguiCssImageClass *klass;

  bobgui_internal_return_if_fail (BOBGUI_IS_CSS_IMAGE (image));
  bobgui_internal_return_if_fail (string != NULL);

  klass = BOBGUI_CSS_IMAGE_GET_CLASS (image);

  klass->print (image, string);
}

char *
bobgui_css_image_to_string (BobguiCssImage *image)
{
  GString *str = g_string_new ("");

  _bobgui_css_image_print (image, str);

  return g_string_free (str, FALSE);
}


/* Applies the algorithm outlined in
 * http://dev.w3.org/csswg/css3-images/#default-sizing
 */
void
_bobgui_css_image_get_concrete_size (BobguiCssImage *image,
                                  double       specified_width,
                                  double       specified_height,
                                  double       default_width,
                                  double       default_height,
                                  double      *concrete_width,
                                  double      *concrete_height)
{
  double image_width, image_height, image_aspect;

  bobgui_internal_return_if_fail (BOBGUI_IS_CSS_IMAGE (image));
  bobgui_internal_return_if_fail (specified_width >= 0);
  bobgui_internal_return_if_fail (specified_height >= 0);
  bobgui_internal_return_if_fail (default_width > 0);
  bobgui_internal_return_if_fail (default_height > 0);
  bobgui_internal_return_if_fail (concrete_width != NULL);
  bobgui_internal_return_if_fail (concrete_height != NULL);

  /* If the specified size is a definite width and height,
   * the concrete object size is given that width and height.
   */
  if (specified_width && specified_height)
    {
      *concrete_width = specified_width;
      *concrete_height = specified_height;
      return;
    }

  image_width  = _bobgui_css_image_get_width (image);
  image_height = _bobgui_css_image_get_height (image);
  image_aspect = _bobgui_css_image_get_aspect_ratio (image);

  /* If the specified size has neither a definite width nor height,
   * and has no additional constraints, the dimensions of the concrete
   * object size are calculated as follows:
   */
  if (specified_width == 0.0 && specified_height == 0.0)
    {
      /* If the object has only an intrinsic aspect ratio,
       * the concrete object size must have that aspect ratio,
       * and additionally be as large as possible without either
       * its height or width exceeding the height or width of the
       * default object size.
       */
      if (image_aspect > 0 && image_width == 0 && image_height == 0)
        {
          if (image_aspect * default_height > default_width)
            {
              *concrete_width = default_width;
              *concrete_height = default_width / image_aspect;
            }
          else
            {
              *concrete_width = default_height * image_aspect;
              *concrete_height = default_height;
            }
        }
      else
        {
          /* Otherwise, the width and height of the concrete object
           * size is the same as the object's intrinsic width and
           * intrinsic height, if they exist.
           * If the concrete object size is still missing a width or
           * height, and the object has an intrinsic aspect ratio,
           * the missing dimension is calculated from the present
           * dimension and the intrinsic aspect ratio.
           * Otherwise, the missing dimension is taken from the default
           * object size. 
           */
          if (image_width)
            *concrete_width = image_width;
          else if (image_aspect)
            *concrete_width = image_height * image_aspect;
          else
            *concrete_width = default_width;

          if (image_height)
            *concrete_height = image_height;
          else if (image_aspect)
            *concrete_height = image_width / image_aspect;
          else
            *concrete_height = default_height;
        }

      return;
    }

  /* If the specified size has only a width or height, but not both,
   * then the concrete object size is given that specified width or height.
   * The other dimension is calculated as follows:
   * If the object has an intrinsic aspect ratio, the missing dimension of
   * the concrete object size is calculated using the intrinsic aspect-ratio
   * and the present dimension.
   * Otherwise, if the missing dimension is present in the object's intrinsic
   * dimensions, the missing dimension is taken from the object's intrinsic
   * dimensions.
   * Otherwise, the missing dimension of the concrete object size is taken
   * from the default object size. 
   */
  if (specified_width)
    {
      *concrete_width = specified_width;
      if (image_aspect)
        *concrete_height = specified_width / image_aspect;
      else if (image_height)
        *concrete_height = image_height;
      else
        *concrete_height = default_height;
    }
  else
    {
      *concrete_height = specified_height;
      if (image_aspect)
        *concrete_width = specified_height * image_aspect;
      else if (image_width)
        *concrete_width = image_width;
      else
        *concrete_width = default_width;
    }
}

cairo_surface_t *
_bobgui_css_image_get_surface (BobguiCssImage     *image,
                            cairo_surface_t *target,
                            int              surface_width,
                            int              surface_height)
{
  cairo_surface_t *result;
  cairo_t *cr;

  bobgui_internal_return_val_if_fail (BOBGUI_IS_CSS_IMAGE (image), NULL);
  bobgui_internal_return_val_if_fail (surface_width > 0, NULL);
  bobgui_internal_return_val_if_fail (surface_height > 0, NULL);

  if (target)
    result = cairo_surface_create_similar (target,
                                           CAIRO_CONTENT_COLOR_ALPHA,
                                           surface_width,
                                           surface_height);
  else
    result = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
                                         surface_width,
                                         surface_height);

  cr = cairo_create (result);
  bobgui_css_image_draw (image, cr, surface_width, surface_height);
  cairo_destroy (cr);

  return result;
}

static GType
bobgui_css_image_get_parser_type (BobguiCssParser *parser)
{
  static const struct {
    const char *prefix;
    GType (* type_func) (void);
  } image_types[] = {
    { "url", _bobgui_css_image_url_get_type },
    { "-bobgui-icontheme", _bobgui_css_image_icon_theme_get_type },
    { "-bobgui-scaled", _bobgui_css_image_scaled_get_type },
    { "-bobgui-recolor", _bobgui_css_image_recolor_get_type },
    { "linear-gradient", _bobgui_css_image_linear_get_type },
    { "repeating-linear-gradient", _bobgui_css_image_linear_get_type },
    { "radial-gradient", _bobgui_css_image_radial_get_type },
    { "repeating-radial-gradient", _bobgui_css_image_radial_get_type },
    { "conic-gradient", bobgui_css_image_conic_get_type },
    { "cross-fade", bobgui_css_image_cross_fade_get_type },
    { "image", _bobgui_css_image_fallback_get_type }
  };
  guint i;

  for (i = 0; i < G_N_ELEMENTS (image_types); i++)
    {
      if (bobgui_css_parser_has_function (parser, image_types[i].prefix))
        return image_types[i].type_func ();
    }

  if (bobgui_css_parser_has_token (parser, BOBGUI_CSS_TOKEN_URL))
    return _bobgui_css_image_url_get_type ();

  return G_TYPE_INVALID;
}

/**
 * _bobgui_css_image_can_parse:
 * @parser: a css parser
 *
 * Checks if the parser can potentially parse the given stream as an
 * image from looking at the first token of @parser. This is useful for
 * implementing shorthand properties. A successful parse of an image
 * can not be guaranteed.
 *
 * Returns: %TRUE if it looks like an image.
 **/
gboolean
_bobgui_css_image_can_parse (BobguiCssParser *parser)
{
  return bobgui_css_image_get_parser_type (parser) != G_TYPE_INVALID;
}

BobguiCssImage *
_bobgui_css_image_new_parse (BobguiCssParser *parser)
{
  BobguiCssImageClass *klass;
  BobguiCssImage *image;
  GType image_type;

  bobgui_internal_return_val_if_fail (parser != NULL, NULL);

  image_type = bobgui_css_image_get_parser_type (parser);
  if (image_type == G_TYPE_INVALID)
    {
      bobgui_css_parser_error_syntax (parser, "Not a valid image");
      return NULL;
    }

  image = g_object_new (image_type, NULL);

  klass = BOBGUI_CSS_IMAGE_GET_CLASS (image);
  if (!klass->parse (image, parser))
    {
      g_object_unref (image);
      return NULL;
    }

  return image;
}

gboolean
bobgui_css_image_is_computed (BobguiCssImage *image)
{
  BobguiCssImageClass *klass = BOBGUI_CSS_IMAGE_GET_CLASS (image);

  return klass->is_computed (image);
}

gboolean
bobgui_css_image_contains_current_color (BobguiCssImage *image)
{
  BobguiCssImageClass *klass = BOBGUI_CSS_IMAGE_GET_CLASS (image);

  return klass->contains_current_color (image);
}

BobguiCssImage *
bobgui_css_image_resolve (BobguiCssImage          *image,
                       BobguiCssComputeContext *context,
                       BobguiCssValue          *current_color)
{
  BobguiCssImageClass *klass = BOBGUI_CSS_IMAGE_GET_CLASS (image);

  return klass->resolve (image, context, current_color);
}
