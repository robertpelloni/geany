/*
 * Copyright © 2018 Red Hat Inc.
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

#include "bobguicssimagepaintableprivate.h"
#include "bobguicssvalueprivate.h"

#include "bobguiprivate.h"

G_DEFINE_TYPE (BobguiCssImagePaintable, bobgui_css_image_paintable, BOBGUI_TYPE_CSS_IMAGE)

static inline GdkPaintable *
get_paintable (BobguiCssImagePaintable *paintable)
{
  if (paintable->static_paintable)
    return paintable->static_paintable;

  return paintable->paintable;
}

static int
bobgui_css_image_paintable_get_width (BobguiCssImage *image)
{
  BobguiCssImagePaintable *paintable = BOBGUI_CSS_IMAGE_PAINTABLE (image);

  return gdk_paintable_get_intrinsic_width (get_paintable (paintable));
}

static int
bobgui_css_image_paintable_get_height (BobguiCssImage *image)
{
  BobguiCssImagePaintable *paintable = BOBGUI_CSS_IMAGE_PAINTABLE (image);

  return gdk_paintable_get_intrinsic_height (get_paintable (paintable));
}

static double
bobgui_css_image_paintable_get_aspect_ratio (BobguiCssImage *image)
{
  BobguiCssImagePaintable *paintable = BOBGUI_CSS_IMAGE_PAINTABLE (image);

  return gdk_paintable_get_intrinsic_aspect_ratio (get_paintable (paintable));
}

static void
bobgui_css_image_paintable_snapshot (BobguiCssImage *image,
                                  BobguiSnapshot *snapshot,
                                  double       width,
                                  double       height)
{
  BobguiCssImagePaintable *paintable = BOBGUI_CSS_IMAGE_PAINTABLE (image);

  gdk_paintable_snapshot (get_paintable (paintable),
                          snapshot,
                          width, height);
}

static BobguiCssImage *
bobgui_css_image_paintable_get_static_image (BobguiCssImage *image)
{
  BobguiCssImagePaintable *paintable = BOBGUI_CSS_IMAGE_PAINTABLE (image);
  GdkPaintable *static_image;
  BobguiCssImage *result;

  static_image = gdk_paintable_get_current_image (paintable->paintable);

  if (paintable->static_paintable == static_image)
    {
      result = g_object_ref (image);
    }
  else
    {
      result = bobgui_css_image_paintable_new (paintable->paintable,
                                            static_image);
    }

  g_object_unref (static_image);

  return result;
}

static BobguiCssImage *
bobgui_css_image_paintable_compute (BobguiCssImage          *image,
                                 guint                 property_id,
                                 BobguiCssComputeContext *context)
{
  return bobgui_css_image_paintable_get_static_image (image);
}

static gboolean
bobgui_css_image_paintable_equal (BobguiCssImage *image1,
                               BobguiCssImage *image2)
{
  BobguiCssImagePaintable *paintable1 = BOBGUI_CSS_IMAGE_PAINTABLE (image1);
  BobguiCssImagePaintable *paintable2 = BOBGUI_CSS_IMAGE_PAINTABLE (image2);

  return paintable1->paintable == paintable2->paintable
      && paintable1->static_paintable == paintable2->static_paintable;
}

#define GDK_PAINTABLE_IMMUTABLE (GDK_PAINTABLE_STATIC_SIZE | GDK_PAINTABLE_STATIC_CONTENTS)
static gboolean
bobgui_css_image_paintable_is_dynamic (BobguiCssImage *image)
{
  BobguiCssImagePaintable *paintable = BOBGUI_CSS_IMAGE_PAINTABLE (image);

  return (gdk_paintable_get_flags (paintable->paintable) & GDK_PAINTABLE_IMMUTABLE) == GDK_PAINTABLE_IMMUTABLE;
}

static BobguiCssImage *
bobgui_css_image_paintable_get_dynamic_image (BobguiCssImage *image,
                                           gint64       monotonic_time)
{
  return bobgui_css_image_paintable_get_static_image (image);
}

static void
bobgui_css_image_paintable_print (BobguiCssImage *image,
                               GString     *string)
{
  g_string_append (string, "none /* FIXME */");
}

static void
bobgui_css_image_paintable_dispose (GObject *object)
{
  BobguiCssImagePaintable *paintable = BOBGUI_CSS_IMAGE_PAINTABLE (object);

  g_clear_object (&paintable->paintable);
  g_clear_object (&paintable->static_paintable);

  G_OBJECT_CLASS (bobgui_css_image_paintable_parent_class)->dispose (object);
}

static gboolean
bobgui_css_image_paintable_is_computed (BobguiCssImage *image)
{
  BobguiCssImagePaintable *self = BOBGUI_CSS_IMAGE_PAINTABLE (image);

  return (gdk_paintable_get_flags (self->paintable) & GDK_PAINTABLE_IMMUTABLE) == GDK_PAINTABLE_IMMUTABLE;
}

static gboolean
bobgui_css_image_paintable_contains_current_color (BobguiCssImage *image)
{
  return FALSE;
}

static BobguiCssImage *
bobgui_css_image_paintable_resolve (BobguiCssImage          *image,
                                 BobguiCssComputeContext *context,
                                 BobguiCssValue          *value)
{
  return g_object_ref (image);
}

static void
bobgui_css_image_paintable_class_init (BobguiCssImagePaintableClass *klass)
{
  BobguiCssImageClass *image_class = BOBGUI_CSS_IMAGE_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  image_class->get_width = bobgui_css_image_paintable_get_width;
  image_class->get_height = bobgui_css_image_paintable_get_height;
  image_class->get_aspect_ratio = bobgui_css_image_paintable_get_aspect_ratio;
  image_class->snapshot = bobgui_css_image_paintable_snapshot;
  image_class->print = bobgui_css_image_paintable_print;
  image_class->compute = bobgui_css_image_paintable_compute;
  image_class->equal = bobgui_css_image_paintable_equal;
  image_class->is_dynamic = bobgui_css_image_paintable_is_dynamic;
  image_class->is_computed = bobgui_css_image_paintable_is_computed;
  image_class->get_dynamic_image = bobgui_css_image_paintable_get_dynamic_image;
  image_class->contains_current_color = bobgui_css_image_paintable_contains_current_color;
  image_class->resolve = bobgui_css_image_paintable_resolve;

  object_class->dispose = bobgui_css_image_paintable_dispose;
}

static void
bobgui_css_image_paintable_init (BobguiCssImagePaintable *image_paintable)
{
}

BobguiCssImage *
bobgui_css_image_paintable_new (GdkPaintable *paintable,
                             GdkPaintable *static_paintable)
{
  BobguiCssImage *image;

  bobgui_internal_return_val_if_fail (GDK_IS_PAINTABLE (paintable), NULL);
  bobgui_internal_return_val_if_fail (static_paintable == NULL || GDK_IS_PAINTABLE (static_paintable), NULL);

  image = g_object_new (BOBGUI_TYPE_CSS_IMAGE_PAINTABLE, NULL);
  
  BOBGUI_CSS_IMAGE_PAINTABLE (image)->paintable = g_object_ref (paintable);
  if (static_paintable)
    BOBGUI_CSS_IMAGE_PAINTABLE (image)->static_paintable = g_object_ref (static_paintable);

  return image;
}
