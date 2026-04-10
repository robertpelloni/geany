/*
 * Copyright © 2015 Red Hat Inc.
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

#define BOBGUI_TYPE_CSS_IMAGE_RADIAL           (_bobgui_css_image_radial_get_type ())
#define BOBGUI_CSS_IMAGE_RADIAL(obj)           (G_TYPE_CHECK_INSTANCE_CAST (obj, BOBGUI_TYPE_CSS_IMAGE_RADIAL, BobguiCssImageRadial))
#define BOBGUI_CSS_IMAGE_RADIAL_CLASS(cls)     (G_TYPE_CHECK_CLASS_CAST (cls, BOBGUI_TYPE_CSS_IMAGE_RADIAL, BobguiCssImageRadialClass))
#define BOBGUI_IS_CSS_IMAGE_RADIAL(obj)        (G_TYPE_CHECK_INSTANCE_TYPE (obj, BOBGUI_TYPE_CSS_IMAGE_RADIAL))
#define BOBGUI_IS_CSS_IMAGE_RADIAL_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE (obj, BOBGUI_TYPE_CSS_IMAGE_RADIAL))
#define BOBGUI_CSS_IMAGE_RADIAL_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_CSS_IMAGE_RADIAL, BobguiCssImageRadialClass))

typedef struct _BobguiCssImageRadial           BobguiCssImageRadial;
typedef struct _BobguiCssImageRadialClass      BobguiCssImageRadialClass;
typedef struct _BobguiCssImageRadialColorStop  BobguiCssImageRadialColorStop;

struct _BobguiCssImageRadialColorStop {
  BobguiCssValue        *transition_hint;
  BobguiCssValue        *color;
  BobguiCssValue        *offset;
};

typedef enum {
  BOBGUI_CSS_EXPLICIT_SIZE,
  BOBGUI_CSS_CLOSEST_SIDE,
  BOBGUI_CSS_FARTHEST_SIDE,
  BOBGUI_CSS_CLOSEST_CORNER,
  BOBGUI_CSS_FARTHEST_CORNER
} BobguiCssRadialSize;

struct _BobguiCssImageRadial
{
  BobguiCssImage parent;

  BobguiCssValue *position;
  BobguiCssValue *sizes[2];

  BobguiCssColorSpace color_space;
  BobguiCssHueInterpolation hue_interp;

  guint n_stops;
  BobguiCssImageRadialColorStop *color_stops;

  BobguiCssRadialSize size;
  guint circle : 1;
  guint repeating :1;
};

struct _BobguiCssImageRadialClass
{
  BobguiCssImageClass parent_class;
};

GType          _bobgui_css_image_radial_get_type             (void) G_GNUC_CONST;

G_END_DECLS

