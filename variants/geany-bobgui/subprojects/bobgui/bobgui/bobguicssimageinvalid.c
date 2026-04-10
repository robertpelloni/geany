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

#include "bobguicssimageinvalidprivate.h"

G_DEFINE_TYPE (BobguiCssImageInvalid, bobgui_css_image_invalid, BOBGUI_TYPE_CSS_IMAGE)

static void
bobgui_css_image_invalid_snapshot (BobguiCssImage *image,
                                BobguiSnapshot *snapshot,
                                double       width,
                                double       height)
{
}

static gboolean
bobgui_css_image_invalid_equal (BobguiCssImage *image1,
                             BobguiCssImage *image2)
{
  return TRUE;
}

static void
bobgui_css_image_invalid_print (BobguiCssImage *image,
                             GString     *string)
{
  g_string_append (string, "none /* invalid image */");
}

static gboolean
bobgui_css_image_invalid_is_invalid (BobguiCssImage *image)
{
  return TRUE;
}

static void
bobgui_css_image_invalid_class_init (BobguiCssImageInvalidClass *klass)
{
  BobguiCssImageClass *image_class = BOBGUI_CSS_IMAGE_CLASS (klass);

  image_class->snapshot = bobgui_css_image_invalid_snapshot;
  image_class->print = bobgui_css_image_invalid_print;
  image_class->equal = bobgui_css_image_invalid_equal;
  image_class->is_invalid = bobgui_css_image_invalid_is_invalid;
}

static void
bobgui_css_image_invalid_init (BobguiCssImageInvalid *image_invalid)
{
}

BobguiCssImage *
bobgui_css_image_invalid_new (void)
{
  return g_object_new (BOBGUI_TYPE_CSS_IMAGE_INVALID, NULL);
}
