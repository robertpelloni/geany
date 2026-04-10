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

#pragma once

#include "bobgui/bobguicssimageprivate.h"
#include "bobgui/bobguicssvalueprivate.h"

G_BEGIN_DECLS

#define BOBGUI_TYPE_CSS_IMAGE_CONIC           (bobgui_css_image_conic_get_type ())
#define BOBGUI_CSS_IMAGE_CONIC(obj)           (G_TYPE_CHECK_INSTANCE_CAST (obj, BOBGUI_TYPE_CSS_IMAGE_CONIC, BobguiCssImageConic))
#define BOBGUI_CSS_IMAGE_CONIC_CLASS(cls)     (G_TYPE_CHECK_CLASS_CAST (cls, BOBGUI_TYPE_CSS_IMAGE_CONIC, BobguiCssImageConicClass))
#define BOBGUI_IS_CSS_IMAGE_CONIC(obj)        (G_TYPE_CHECK_INSTANCE_TYPE (obj, BOBGUI_TYPE_CSS_IMAGE_CONIC))
#define BOBGUI_IS_CSS_IMAGE_CONIC_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE (obj, BOBGUI_TYPE_CSS_IMAGE_CONIC))
#define BOBGUI_CSS_IMAGE_CONIC_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_CSS_IMAGE_CONIC, BobguiCssImageConicClass))

typedef struct _BobguiCssImageConic           BobguiCssImageConic;
typedef struct _BobguiCssImageConicClass      BobguiCssImageConicClass;
typedef struct _BobguiCssImageConicColorStop  BobguiCssImageConicColorStop;

struct _BobguiCssImageConicColorStop {
  BobguiCssValue        *offset;
  BobguiCssValue        *transition_hint;
  BobguiCssValue        *color;
};

struct _BobguiCssImageConic
{
  BobguiCssImage parent;

  BobguiCssValue *center;
  BobguiCssValue *rotation;

  BobguiCssColorSpace color_space;
  BobguiCssHueInterpolation hue_interp;

  guint n_stops;
  BobguiCssImageConicColorStop *color_stops;
};

struct _BobguiCssImageConicClass
{
  BobguiCssImageClass parent_class;
};

GType           bobgui_css_image_conic_get_type                    (void) G_GNUC_CONST;

G_END_DECLS

