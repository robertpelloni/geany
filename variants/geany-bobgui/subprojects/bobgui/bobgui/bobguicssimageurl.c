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

#include <string.h>

#include "bobguicssimageurlprivate.h"

#include "bobguicssimageinvalidprivate.h"
#include "bobguicssimagepaintableprivate.h"
#include "bobguistyleproviderprivate.h"
#include "bobgui/css/bobguicssdataurlprivate.h"


G_DEFINE_TYPE (BobguiCssImageUrl, _bobgui_css_image_url, BOBGUI_TYPE_CSS_IMAGE)

static BobguiCssImage *
bobgui_css_image_url_load_image (BobguiCssImageUrl  *url,
                              GError         **error)
{
  GdkTexture *texture;
  GError *local_error = NULL;

  if (url->loaded_image)
    return url->loaded_image;

  if (url->file == NULL)
    {
      url->loaded_image = bobgui_css_image_invalid_new ();
      return url->loaded_image;
    }

  texture = gdk_texture_new_from_file (url->file, &local_error);

  if (texture == NULL)
    {
      if (error && local_error)
        {
          char *uri;

          uri = g_file_get_uri (url->file);
          g_set_error (error,
                       BOBGUI_CSS_PARSER_ERROR,
                       BOBGUI_CSS_PARSER_ERROR_FAILED,
                       "Error loading image '%s': %s", uri, local_error->message);
          g_free (uri);
       }

      url->loaded_image = bobgui_css_image_invalid_new ();
    }
  else
    {
      url->loaded_image = bobgui_css_image_paintable_new (GDK_PAINTABLE (texture), GDK_PAINTABLE (texture));
      g_object_unref (texture);
    }

  g_clear_error (&local_error);

  return url->loaded_image;
}

static int
bobgui_css_image_url_get_width (BobguiCssImage *image)
{
  BobguiCssImageUrl *url = BOBGUI_CSS_IMAGE_URL (image);

  return _bobgui_css_image_get_width (bobgui_css_image_url_load_image (url, NULL));
}

static int
bobgui_css_image_url_get_height (BobguiCssImage *image)
{
  BobguiCssImageUrl *url = BOBGUI_CSS_IMAGE_URL (image);

  return _bobgui_css_image_get_height (bobgui_css_image_url_load_image (url, NULL));
}

static double
bobgui_css_image_url_get_aspect_ratio (BobguiCssImage *image)
{
  BobguiCssImageUrl *url = BOBGUI_CSS_IMAGE_URL (image);

  return _bobgui_css_image_get_aspect_ratio (bobgui_css_image_url_load_image (url, NULL));
}

static void
bobgui_css_image_url_snapshot (BobguiCssImage *image,
                            BobguiSnapshot *snapshot,
                            double       width,
                            double       height)
{
  BobguiCssImageUrl *url = BOBGUI_CSS_IMAGE_URL (image);

  bobgui_css_image_snapshot (bobgui_css_image_url_load_image (url, NULL), snapshot, width, height);
}

static BobguiCssImage *
bobgui_css_image_url_compute (BobguiCssImage          *image,
                           guint                 property_id,
                           BobguiCssComputeContext *context)
{
  BobguiCssImageUrl *url = BOBGUI_CSS_IMAGE_URL (image);
  BobguiCssImage *copy;
  GError *error = NULL;

  copy = bobgui_css_image_url_load_image (url, &error);
  if (error)
    {
      BobguiCssSection *section = bobgui_css_style_get_section (context->style, property_id);
      bobgui_style_provider_emit_error (context->provider, section, error);
      g_error_free (error);
    }

  return g_object_ref (copy);
}

static gboolean
bobgui_css_image_url_equal (BobguiCssImage *image1,
                         BobguiCssImage *image2)
{
  BobguiCssImageUrl *url1 = BOBGUI_CSS_IMAGE_URL (image1);
  BobguiCssImageUrl *url2 = BOBGUI_CSS_IMAGE_URL (image2);
  
  /* FIXME: We don't save data: urls, so we can't compare them here */
  if (url1->file == NULL || url2->file == NULL)
    return FALSE;

  return g_file_equal (url1->file, url2->file);
}

static gboolean
bobgui_css_image_url_is_invalid (BobguiCssImage *image)
{
  BobguiCssImageUrl *url = BOBGUI_CSS_IMAGE_URL (image);

  return bobgui_css_image_is_invalid (bobgui_css_image_url_load_image (url, NULL));
}

static gboolean
bobgui_css_image_url_is_computed (BobguiCssImage *image)
{
  return TRUE;
}

static BobguiCssImage *
bobgui_css_image_url_resolve (BobguiCssImage          *image,
                           BobguiCssComputeContext *context,
                           BobguiCssValue          *current)
{
  return g_object_ref (image);
}

static gboolean
bobgui_css_image_url_contains_current_color (BobguiCssImage *image)
{
  return FALSE;
}


static gboolean
bobgui_css_image_url_parse (BobguiCssImage  *image,
                         BobguiCssParser *parser)
{
  BobguiCssImageUrl *self = BOBGUI_CSS_IMAGE_URL (image);
  char *url, *scheme;

  url = bobgui_css_parser_consume_url (parser);
  if (url == NULL)
    return FALSE;

  scheme = g_uri_parse_scheme (url);
  if (scheme && g_ascii_strcasecmp (scheme, "data") == 0)
    {
      GBytes *bytes;
      GError *error = NULL;

      bytes = bobgui_css_data_url_parse (url, NULL, &error);
      if (bytes)
        {
          GdkTexture *texture;

          texture = gdk_texture_new_from_bytes (bytes, &error);
          g_bytes_unref (bytes);
          if (texture)
            {
              GdkPaintable *paintable = GDK_PAINTABLE (texture);
              self->loaded_image = bobgui_css_image_paintable_new (paintable, paintable);
            }
          else
            {
              bobgui_css_parser_emit_error (parser,
                                         bobgui_css_parser_get_start_location (parser),
                                         bobgui_css_parser_get_end_location (parser),
                                         error);
              g_clear_error (&error);
            }
        }
      else
        {
          bobgui_css_parser_emit_error (parser,
                                     bobgui_css_parser_get_start_location (parser),
                                     bobgui_css_parser_get_end_location (parser),
                                     error);
          g_clear_error (&error);
        }
    }
  else
    {
      self->file = bobgui_css_parser_resolve_url (parser, url);
    }

  g_free (url);
  g_free (scheme);

  return TRUE;
}

static void
bobgui_css_image_url_print (BobguiCssImage *image,
                         GString     *string)
{
  BobguiCssImageUrl *url = BOBGUI_CSS_IMAGE_URL (image);

  _bobgui_css_image_print (bobgui_css_image_url_load_image (url, NULL), string);
}

static void
bobgui_css_image_url_dispose (GObject *object)
{
  BobguiCssImageUrl *url = BOBGUI_CSS_IMAGE_URL (object);

  g_clear_object (&url->file);
  g_clear_object (&url->loaded_image);

  G_OBJECT_CLASS (_bobgui_css_image_url_parent_class)->dispose (object);
}

static void
_bobgui_css_image_url_class_init (BobguiCssImageUrlClass *klass)
{
  BobguiCssImageClass *image_class = BOBGUI_CSS_IMAGE_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  image_class->get_width = bobgui_css_image_url_get_width;
  image_class->get_height = bobgui_css_image_url_get_height;
  image_class->get_aspect_ratio = bobgui_css_image_url_get_aspect_ratio;
  image_class->compute = bobgui_css_image_url_compute;
  image_class->snapshot = bobgui_css_image_url_snapshot;
  image_class->parse = bobgui_css_image_url_parse;
  image_class->print = bobgui_css_image_url_print;
  image_class->equal = bobgui_css_image_url_equal;
  image_class->is_invalid = bobgui_css_image_url_is_invalid;
  image_class->is_computed = bobgui_css_image_url_is_computed;
  image_class->contains_current_color = bobgui_css_image_url_contains_current_color;
  image_class->resolve = bobgui_css_image_url_resolve;

  object_class->dispose = bobgui_css_image_url_dispose;
}

static void
_bobgui_css_image_url_init (BobguiCssImageUrl *image_url)
{
}

