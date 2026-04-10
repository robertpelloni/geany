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

#include "bobguicssimagerecolorprivate.h"
#include "bobguicssimageprivate.h"
#include "bobguicsspalettevalueprivate.h"
#include "bobguicsscolorvalueprivate.h"
#include "bobguisnapshotprivate.h"
#include "gdktextureutilsprivate.h"
#include "svg/bobguisvg.h"
#include "bobguisymbolicpaintable.h"

#include "bobguistyleproviderprivate.h"

G_DEFINE_TYPE (BobguiCssImageRecolor, _bobgui_css_image_recolor, BOBGUI_TYPE_CSS_IMAGE)

static void
bobgui_css_image_recolor_print (BobguiCssImage *image,
                             GString     *string)
{
  BobguiCssImageRecolor *recolor = BOBGUI_CSS_IMAGE_RECOLOR (image);
  char *uri;

  g_string_append (string, "-bobgui-recolor(url(");
  uri = g_file_get_uri (recolor->file);
  g_string_append (string, uri);
  g_free (uri);
  g_string_append (string, ")");
  if (recolor->palette)
    {
      g_string_append (string, ",");
      bobgui_css_value_print (recolor->palette, string);
    }
  g_string_append (string, ")");
}

static void
bobgui_css_image_recolor_dispose (GObject *object)
{
  BobguiCssImageRecolor *recolor = BOBGUI_CSS_IMAGE_RECOLOR (object);

  g_clear_pointer (&recolor->palette, bobgui_css_value_unref);
  g_clear_pointer (&recolor->color, bobgui_css_value_unref);
  g_clear_object (&recolor->file);
  g_clear_object (&recolor->paintable);

  G_OBJECT_CLASS (_bobgui_css_image_recolor_parent_class)->dispose (object);
}

static void
bobgui_css_image_recolor_load_paintable (BobguiCssImageRecolor  *recolor,
                                      GError             **error)
{
  char *uri;

  if (recolor->paintable)
    return;

  uri = g_file_get_uri (recolor->file);

  if (g_file_has_uri_scheme (recolor->file, "resource"))
    {
      char *resource_path = g_uri_unescape_string (uri + strlen ("resource://"), NULL);

      if (g_str_has_suffix (resource_path, ".svg"))
        {
          BobguiSvg *svg = bobgui_svg_new ();

          if (g_str_has_suffix (resource_path, "-symbolic.svg") ||
              g_str_has_suffix (resource_path, "-symbolic-ltr.svg") ||
              g_str_has_suffix (resource_path, "-symbolic-rtl.svg"))
            bobgui_svg_set_features (svg, BOBGUI_SVG_DEFAULT_FEATURES | BOBGUI_SVG_TRADITIONAL_SYMBOLIC);

          bobgui_svg_load_from_resource (svg, resource_path);
          recolor->paintable = GDK_PAINTABLE (svg);
        }
      else if (g_str_has_suffix (uri, ".symbolic.png"))
        {
          recolor->paintable = GDK_PAINTABLE (gdk_texture_new_from_resource (resource_path));
        }

      g_free (resource_path);
    }
  else
    {
      const char *path = g_file_peek_path (recolor->file);

      if (g_str_has_suffix (path, ".svg"))
        {
          BobguiSvg *svg;
          char *data;
          size_t size;

          svg = bobgui_svg_new ();

          if (g_str_has_suffix (path, "-symbolic.svg") ||
              g_str_has_suffix (path, "-symbolic-ltr.svg") ||
              g_str_has_suffix (path, "-symbolic-rtl.svg"))
            bobgui_svg_set_features (svg, BOBGUI_SVG_DEFAULT_FEATURES | BOBGUI_SVG_TRADITIONAL_SYMBOLIC);


          if (g_file_get_contents (path, &data, &size, NULL))
            {
              GBytes *bytes = g_bytes_new_take (data, size);
              bobgui_svg_load_from_bytes (svg, bytes);
              g_bytes_unref (bytes);
            }

          recolor->paintable = GDK_PAINTABLE (svg);
        }
      else if (g_str_has_suffix (uri, ".symbolic.png"))
        {
          recolor->paintable = GDK_PAINTABLE (gdk_texture_new_from_file (recolor->file, NULL));
        }
    }

  if (recolor->paintable)
    {
      recolor->width = gdk_paintable_get_intrinsic_width (recolor->paintable);
      recolor->height = gdk_paintable_get_intrinsic_height (recolor->paintable);
    }

  g_free (uri);
}

static BobguiCssImage *
bobgui_css_image_recolor_load (BobguiCssImageRecolor    *recolor,
                            BobguiCssComputeContext  *context,
                            BobguiCssValue           *palette,
                            int                    scale,
                            GError               **gerror)
{
  GError *local_error = NULL;
  BobguiCssImageRecolor *image;

  image = g_object_new (BOBGUI_TYPE_CSS_IMAGE_RECOLOR, NULL);

  image->file = g_object_ref (recolor->file);
  image->palette = bobgui_css_value_ref (palette);
  image->color = bobgui_css_value_ref (context->style->core->color);

  bobgui_css_image_recolor_load_paintable (recolor, &local_error);

  if (recolor->paintable)
    {
      image->paintable = g_object_ref (recolor->paintable);
      image->width = recolor->width;
      image->height = recolor->height;
    }
  else
    {
      if (gerror)
        {
          char *uri;

          uri = g_file_get_uri (recolor->file);
          g_set_error (gerror,
                       BOBGUI_CSS_PARSER_ERROR,
                       BOBGUI_CSS_PARSER_ERROR_FAILED,
                       "Error loading image '%s': %s", uri, local_error ? local_error->message : "");
          g_free (uri);
       }
    }

  g_clear_error (&local_error);

  return BOBGUI_CSS_IMAGE (image);
}

static void
init_color_matrix (graphene_matrix_t *color_matrix,
                   graphene_vec4_t   *color_offset,
                   const GdkRGBA     *foreground_color,
                   const GdkRGBA     *success_color,
                   const GdkRGBA     *warning_color,
                   const GdkRGBA     *error_color)
{
  const GdkRGBA fg_default = { 0.7450980392156863, 0.7450980392156863, 0.7450980392156863, 1.0};
  const GdkRGBA success_default = { 0.3046921492332342,0.6015716792553597, 0.023437857633325704, 1.0};
  const GdkRGBA warning_default = {0.9570458533607996, 0.47266346227206835, 0.2421911955443656, 1.0 };
  const GdkRGBA error_default = { 0.796887159533074, 0 ,0, 1.0 };
  const GdkRGBA *fg = foreground_color ? foreground_color : &fg_default;
  const GdkRGBA *sc = success_color ? success_color : &success_default;
  const GdkRGBA *wc = warning_color ? warning_color : &warning_default;
  const GdkRGBA *ec = error_color ? error_color : &error_default;

  graphene_matrix_init_from_float (color_matrix,
                                   (float[16]) {
                                     sc->red - fg->red, sc->green - fg->green, sc->blue - fg->blue, 0,
                                     wc->red - fg->red, wc->green - fg->green, wc->blue - fg->blue, 0,
                                     ec->red - fg->red, ec->green - fg->green, ec->blue - fg->blue, 0,
                                     0, 0, 0, fg->alpha
                                   });
  graphene_vec4_init (color_offset, fg->red, fg->green, fg->blue, 0);
}

static void
bobgui_css_image_recolor_snapshot (BobguiCssImage *image,
                                BobguiSnapshot *snapshot,
                                double       width,
                                double       height)
{
  BobguiCssImageRecolor *recolor = BOBGUI_CSS_IMAGE_RECOLOR (image);
  GdkRGBA colors[5];
  const char *symbolic[5] = {
    [BOBGUI_SYMBOLIC_COLOR_FOREGROUND] = "foreground",
    [BOBGUI_SYMBOLIC_COLOR_SUCCESS] = "success",
    [BOBGUI_SYMBOLIC_COLOR_WARNING] = "warning",
    [BOBGUI_SYMBOLIC_COLOR_ERROR] = "error",
    [BOBGUI_SYMBOLIC_COLOR_ACCENT] = "accent",
  };

  if (recolor->paintable == NULL)
    return;

  colors[BOBGUI_SYMBOLIC_COLOR_FOREGROUND] = *bobgui_css_color_value_get_rgba (recolor->color);

  for (unsigned int i = BOBGUI_SYMBOLIC_COLOR_SUCCESS; i <= BOBGUI_SYMBOLIC_COLOR_ACCENT; i++)
    {
      const BobguiCssValue *color = bobgui_css_palette_value_get_color (recolor->palette, symbolic[i]);
      if (color)
        colors[i] = *bobgui_css_color_value_get_rgba (color);
      else
        colors[i] = colors[BOBGUI_SYMBOLIC_COLOR_FOREGROUND];
    }

  if (BOBGUI_IS_SYMBOLIC_PAINTABLE (recolor->paintable))
    {
      bobgui_symbolic_paintable_snapshot_with_weight (BOBGUI_SYMBOLIC_PAINTABLE (recolor->paintable),
                                                   snapshot,
                                                   width, height,
                                                   colors, 5,
                                                   400);
    }
  else
    {
      graphene_matrix_t matrix;
      graphene_vec4_t offset;

      init_color_matrix (&matrix, &offset,
                         &colors[BOBGUI_SYMBOLIC_COLOR_FOREGROUND],
                         &colors[BOBGUI_SYMBOLIC_COLOR_SUCCESS],
                         &colors[BOBGUI_SYMBOLIC_COLOR_WARNING],
                         &colors[BOBGUI_SYMBOLIC_COLOR_ERROR]);

      bobgui_snapshot_push_color_matrix (snapshot, &matrix, &offset);
      gdk_paintable_snapshot (recolor->paintable, snapshot, width, height);
      bobgui_snapshot_pop (snapshot);
    }
}

static BobguiCssImage *
bobgui_css_image_recolor_compute (BobguiCssImage          *image,
                               guint                 property_id,
                               BobguiCssComputeContext *context)
{
  BobguiCssImageRecolor *recolor = BOBGUI_CSS_IMAGE_RECOLOR (image);
  BobguiCssValue *palette;
  BobguiCssImage *img;
  int scale;
  GError *error = NULL;

  scale = bobgui_style_provider_get_scale (context->provider);

  if (recolor->palette)
    palette = bobgui_css_value_compute (recolor->palette, property_id, context);
  else
    palette = bobgui_css_value_ref (context->style->core->icon_palette);

  img = bobgui_css_image_recolor_load (recolor, context, palette, scale, &error);

  if (error)
    {
      BobguiCssSection *section = bobgui_css_style_get_section (context->style, property_id);
      bobgui_style_provider_emit_error (context->provider, section, error);
      g_error_free (error);
    }

  bobgui_css_value_unref (palette);

  return img;
}

static guint
bobgui_css_image_recolor_parse_arg (BobguiCssParser *parser,
                                 guint         arg,
                                 gpointer      data)
{
  BobguiCssImageRecolor *self = data;

  switch (arg)
  {
    case 0:
      {
        char *url = bobgui_css_parser_consume_url (parser);
        if (url == NULL)
          return 0;
        self->file = bobgui_css_parser_resolve_url (parser, url);
        g_free (url);
        if (self->file == NULL)
          return 0;
        return 1;
      }

    case 1:
      self->palette = bobgui_css_palette_value_parse (parser);
      if (self->palette == NULL)
        return 0;
      return 1;

    default:
      g_assert_not_reached ();
      return 0;
  }
}

static gboolean
bobgui_css_image_recolor_parse (BobguiCssImage  *image,
                             BobguiCssParser *parser)
{
  if (!bobgui_css_parser_has_function (parser, "-bobgui-recolor"))
    {
      bobgui_css_parser_error_syntax (parser, "Expected '-bobgui-recolor('");
      return FALSE;
    }

  return bobgui_css_parser_consume_function (parser, 1, 2, bobgui_css_image_recolor_parse_arg, image);
}

static int
bobgui_css_image_recolor_get_width (BobguiCssImage *image)
{
  BobguiCssImageRecolor *recolor = BOBGUI_CSS_IMAGE_RECOLOR (image);

  bobgui_css_image_recolor_load_paintable (recolor, NULL);

  if (recolor->paintable == NULL)
    return 0;

  return (int) recolor->width;
}

static int
bobgui_css_image_recolor_get_height (BobguiCssImage *image)
{
  BobguiCssImageRecolor *recolor = BOBGUI_CSS_IMAGE_RECOLOR (image);

  bobgui_css_image_recolor_load_paintable (recolor, NULL);

  if (recolor->paintable == NULL)
    return 0;

  return (int) recolor->height;
}

static gboolean
bobgui_css_image_recolor_is_computed (BobguiCssImage *image)
{
  BobguiCssImageRecolor *recolor = BOBGUI_CSS_IMAGE_RECOLOR (image);

  return recolor->paintable && bobgui_css_value_is_computed (recolor->palette);
}

static gboolean
bobgui_css_image_recolor_contains_current_color (BobguiCssImage *image)
{
  BobguiCssImageRecolor *recolor = BOBGUI_CSS_IMAGE_RECOLOR (image);

  if (!recolor->palette || !recolor->color)
    return TRUE;

  return bobgui_css_value_contains_current_color (recolor->palette) ||
         bobgui_css_value_contains_current_color (recolor->color);
}

static BobguiCssImage *
bobgui_css_image_recolor_resolve (BobguiCssImage          *image,
                               BobguiCssComputeContext *context,
                               BobguiCssValue          *current_color)
{
  BobguiCssImageRecolor *recolor = BOBGUI_CSS_IMAGE_RECOLOR (image);
  BobguiCssImageRecolor *img;

  img = g_object_new (BOBGUI_TYPE_CSS_IMAGE_RECOLOR, NULL);

  img->palette = bobgui_css_value_resolve (recolor->palette, context, current_color);
  img->color = bobgui_css_value_resolve (recolor->color, context, current_color);
  img->file = g_object_ref (recolor->file);
  if (recolor->paintable)
    img->paintable = g_object_ref (recolor->paintable);

  return BOBGUI_CSS_IMAGE (img);
}

static void
_bobgui_css_image_recolor_class_init (BobguiCssImageRecolorClass *klass)
{
  BobguiCssImageClass *image_class = BOBGUI_CSS_IMAGE_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  image_class->get_width = bobgui_css_image_recolor_get_width;
  image_class->get_height = bobgui_css_image_recolor_get_height;
  image_class->compute = bobgui_css_image_recolor_compute;
  image_class->snapshot = bobgui_css_image_recolor_snapshot;
  image_class->parse = bobgui_css_image_recolor_parse;
  image_class->print = bobgui_css_image_recolor_print;
  image_class->is_computed = bobgui_css_image_recolor_is_computed;
  image_class->contains_current_color = bobgui_css_image_recolor_contains_current_color;
  image_class->resolve = bobgui_css_image_recolor_resolve;

  object_class->dispose = bobgui_css_image_recolor_dispose;
}

static void
_bobgui_css_image_recolor_init (BobguiCssImageRecolor *image_recolor)
{
}
