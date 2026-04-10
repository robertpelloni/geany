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

#pragma once

#include "bobgui/bobguicssimageprivate.h"
#include "bobgui/bobguicssvalueprivate.h"

G_BEGIN_DECLS

#define BOBGUI_TYPE_CSS_IMAGE_FALLBACK           (_bobgui_css_image_fallback_get_type ())
#define BOBGUI_CSS_IMAGE_FALLBACK(obj)           (G_TYPE_CHECK_INSTANCE_CAST (obj, BOBGUI_TYPE_CSS_IMAGE_FALLBACK, BobguiCssImageFallback))
#define BOBGUI_CSS_IMAGE_FALLBACK_CLASS(cls)     (G_TYPE_CHECK_CLASS_CAST (cls, BOBGUI_TYPE_CSS_IMAGE_FALLBACK, BobguiCssImageFallbackClass))
#define BOBGUI_IS_CSS_IMAGE_FALLBACK(obj)        (G_TYPE_CHECK_INSTANCE_TYPE (obj, BOBGUI_TYPE_CSS_IMAGE_FALLBACK))
#define BOBGUI_IS_CSS_IMAGE_FALLBACK_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE (obj, BOBGUI_TYPE_CSS_IMAGE_FALLBACK))
#define BOBGUI_CSS_IMAGE_FALLBACK_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_CSS_IMAGE_FALLBACK, BobguiCssImageFallbackClass))

typedef struct _BobguiCssImageFallback           BobguiCssImageFallback;
typedef struct _BobguiCssImageFallbackClass      BobguiCssImageFallbackClass;

struct _BobguiCssImageFallback
{
  BobguiCssImage parent;

  BobguiCssImage **images;
  int          n_images;

  int used;

  BobguiCssValue *color;
};

struct _BobguiCssImageFallbackClass
{
  BobguiCssImageClass parent_class;
};

GType        _bobgui_css_image_fallback_get_type      (void) G_GNUC_CONST;

BobguiCssImage *_bobgui_css_image_fallback_new_for_color (BobguiCssValue *color);

G_END_DECLS

