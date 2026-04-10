/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2040 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <glib.h>
#include <math.h>

#include "bobgui/css/bobguicssparserprivate.h"
#include "bobguicsstypesprivate.h"
#include "gdk/gdkcolorprivate.h"
#include "gsk/gskrendernodeprivate.h"

G_BEGIN_DECLS


typedef struct
{
  BobguiCssColorSpace color_space;
  float values[4];
  guint missing;
} BobguiCssColor;

static inline gboolean
bobgui_css_color_equal (const BobguiCssColor *color1,
                     const BobguiCssColor *color2)
{
  return color1->color_space == color2->color_space &&
         color1->missing == color2->missing &&
         memcmp (color1->values, color2->values, sizeof (float) * 4) == 0;
}

static inline gboolean
bobgui_css_color_component_missing (const BobguiCssColor *color,
                                 guint              idx)
{
  return (color->missing & (1 << idx)) != 0;
}

static inline float
bobgui_css_color_get_component (const BobguiCssColor *color,
                             guint              idx)
{
  return color->values[idx];
}

static inline void
bobgui_css_color_init_with_missing (BobguiCssColor      *color,
                                 BobguiCssColorSpace  color_space,
                                 const float       values[4],
                                 gboolean          missing[4])
{
  color->color_space = color_space;
  for (guint i = 0; i < 4; i++)
    color->values[i] = missing[i] ? 0 : values[i];
  color->missing = missing[0] | (missing[1] << 1) | (missing[2] << 2) | (missing[3] << 3);
}

static inline void
bobgui_css_color_init_from_color (BobguiCssColor       *color,
                               const BobguiCssColor *src)
{
  memcpy (color, src, sizeof (BobguiCssColor));
}

void    bobgui_css_color_init      (BobguiCssColor            *color,
                                 BobguiCssColorSpace        color_space,
                                 const float             values[4]);

GString * bobgui_css_color_print   (const BobguiCssColor      *color,
                                 gboolean                serialize_as_rgb,
                                 GString                *string);

char *  bobgui_css_color_to_string (const BobguiCssColor      *color);

void    bobgui_css_color_convert   (const BobguiCssColor      *input,
                                 BobguiCssColorSpace        dest,
                                 BobguiCssColor            *output);

void    bobgui_css_color_interpolate (const BobguiCssColor      *from,
                                   const BobguiCssColor      *to,
                                   float                   progress,
                                   BobguiCssColorSpace        in,
                                   BobguiCssHueInterpolation  interp,
                                   BobguiCssColor            *output);

const char * bobgui_css_color_space_get_coord_name (BobguiCssColorSpace color_space,
                                                 guint            coord);

void bobgui_css_color_space_get_coord_range (BobguiCssColorSpace  color_space,
                                          gboolean          legacy_rgb_scale,
                                          guint             coord,
                                          float            *lower,
                                          float            *upper);

GdkColorState *bobgui_css_color_space_get_color_state (BobguiCssColorSpace color_space);

gboolean bobgui_css_color_interpolation_method_can_parse (BobguiCssParser *parser);

gboolean bobgui_css_color_interpolation_method_parse (BobguiCssParser           *parser,
                                                   BobguiCssColorSpace       *in,
                                                   BobguiCssHueInterpolation *interp);

void bobgui_css_color_interpolation_method_print (BobguiCssColorSpace        in,
                                               BobguiCssHueInterpolation  interp,
                                               GString                *string);

GskHueInterpolation bobgui_css_hue_interpolation_to_hue_interpolation (BobguiCssHueInterpolation interp);

static inline gboolean
bobgui_css_color_is_clear (const BobguiCssColor *color)
{
  return color->values[3] < (float) 0x00ff / (float) 0xffff;
}

void bobgui_css_color_to_color (const BobguiCssColor *css,
                             GdkColor          *color);

G_END_DECLS
