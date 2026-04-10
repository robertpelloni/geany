/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2024 Red Hat, Inc.
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

#include "config.h"

#include "bobguicsscolorprivate.h"
#include "bobguicolorutilsprivate.h"
#include "gdkcolorstateprivate.h"

/* {{{ Initialization */

void
bobgui_css_color_init (BobguiCssColor      *color,
                    BobguiCssColorSpace  color_space,
                    const float       values[4])
{
  gboolean missing[4] = { 0, };

  /* look for powerless components */
  switch (color_space)
    {
    case BOBGUI_CSS_COLOR_SPACE_SRGB:
    case BOBGUI_CSS_COLOR_SPACE_SRGB_LINEAR:
    case BOBGUI_CSS_COLOR_SPACE_OKLAB:
    case BOBGUI_CSS_COLOR_SPACE_DISPLAY_P3:
    case BOBGUI_CSS_COLOR_SPACE_XYZ:
    case BOBGUI_CSS_COLOR_SPACE_REC2020:
    case BOBGUI_CSS_COLOR_SPACE_REC2100_PQ:
      break;

    case BOBGUI_CSS_COLOR_SPACE_HSL:
      if (fabs (values[2]) < 0.001)
        missing[0] = 1;
      break;

    case BOBGUI_CSS_COLOR_SPACE_HWB:
      if (values[1] + values[2] > 99.999)
        missing[0] = 1;
      break;

    case BOBGUI_CSS_COLOR_SPACE_OKLCH:
      if (fabs (values[1]) < 0.001)
        missing[2] = 1;
      break;

    default:
      g_assert_not_reached ();
    }

  bobgui_css_color_init_with_missing (color, color_space, values, missing);
}

/* }}} */
/* {{{ Utilities */

static inline void
append_color_component (GString           *string,
                        const BobguiCssColor *color,
                        guint              idx)
{
  if (bobgui_css_color_component_missing (color, idx))
    g_string_append (string, "none");
  else
    g_string_append_printf (string, "%g", bobgui_css_color_get_component (color, idx));
}

static void
print_as_rgb (const BobguiCssColor *color,
              GString           *string)
{
  BobguiCssColor tmp;

  bobgui_css_color_convert (color, BOBGUI_CSS_COLOR_SPACE_SRGB, &tmp);
  if (tmp.values[3] > 0.999)
    {
      g_string_append_printf (string, "rgb(%d,%d,%d)",
                              (int)(0.5 + CLAMP (tmp.values[0], 0., 1.) * 255.),
                              (int)(0.5 + CLAMP (tmp.values[1], 0., 1.) * 255.),
                              (int)(0.5 + CLAMP (tmp.values[2], 0., 1.) * 255.));
    }
  else
    {
      char alpha[G_ASCII_DTOSTR_BUF_SIZE];

      g_ascii_formatd (alpha, G_ASCII_DTOSTR_BUF_SIZE, "%g", CLAMP (tmp.values[3], 0, 1));

      g_string_append_printf (string, "rgba(%d,%d,%d,%s)",
                              (int)(0.5 + CLAMP (tmp.values[0], 0., 1.) * 255.),
                              (int)(0.5 + CLAMP (tmp.values[1], 0., 1.) * 255.),
                              (int)(0.5 + CLAMP (tmp.values[2], 0., 1.) * 255.),
                              alpha);
    }
}

GString *
bobgui_css_color_print (const BobguiCssColor *color,
                     gboolean           serialize_as_rgb,
                     GString           *string)
{
  BobguiCssColorSpace print_color_space = color->color_space;
  BobguiCssColor tmp;

  switch (color->color_space)
    {
    case BOBGUI_CSS_COLOR_SPACE_SRGB:
    case BOBGUI_CSS_COLOR_SPACE_HSL:
    case BOBGUI_CSS_COLOR_SPACE_HWB:
      if (serialize_as_rgb)
        {
          print_as_rgb (color, string);
          return string;
        }

      print_color_space = BOBGUI_CSS_COLOR_SPACE_SRGB;
      g_string_append (string, "color(srgb ");
      break;

    case BOBGUI_CSS_COLOR_SPACE_SRGB_LINEAR:
      g_string_append (string, "color(srgb-linear ");
      break;

    case BOBGUI_CSS_COLOR_SPACE_OKLAB:
      g_string_append (string, "oklab(");
      break;

    case BOBGUI_CSS_COLOR_SPACE_OKLCH:
      g_string_append (string, "oklch(");
      break;

    case BOBGUI_CSS_COLOR_SPACE_DISPLAY_P3:
      g_string_append (string, "color(display-p3 ");
      break;

    case BOBGUI_CSS_COLOR_SPACE_XYZ:
      g_string_append (string, "color(xyz ");
      break;

    case BOBGUI_CSS_COLOR_SPACE_REC2020:
      g_string_append (string, "color(rec2020 ");
      break;

    case BOBGUI_CSS_COLOR_SPACE_REC2100_PQ:
      g_string_append (string, "color(rec2100-pq ");
      break;

    default:
      g_assert_not_reached ();
    }

  if (print_color_space != color->color_space)
    bobgui_css_color_convert (color, print_color_space, &tmp);
  else
    tmp = *color;

  for (guint i = 0; i < 3; i++)
    {
      if (i > 0)
        g_string_append_c (string, ' ');
      append_color_component (string, &tmp, i);
    }

  if (bobgui_css_color_component_missing (&tmp, 3) ||
      tmp.values[3] < 0.999)
    {
      g_string_append (string, " / ");
      append_color_component (string, &tmp, 3);
    }

  g_string_append_c (string, ')');

  return string;
}

char *
bobgui_css_color_to_string (const BobguiCssColor *color)
{
  return g_string_free (bobgui_css_color_print (color, FALSE, g_string_new ("")), FALSE);
}

const char *
bobgui_css_color_space_get_coord_name (BobguiCssColorSpace color_space,
                                    guint            coord)
{
  if (coord == 3)
    return "alpha";

  switch (color_space)
    {
    case BOBGUI_CSS_COLOR_SPACE_SRGB:
    case BOBGUI_CSS_COLOR_SPACE_SRGB_LINEAR:
    case BOBGUI_CSS_COLOR_SPACE_DISPLAY_P3:
    case BOBGUI_CSS_COLOR_SPACE_REC2020:
    case BOBGUI_CSS_COLOR_SPACE_REC2100_PQ:
      switch (coord)
        {
        case 0: return "r";
        case 1: return "g";
        case 2: return "b";
        default: g_assert_not_reached ();
        }
    case BOBGUI_CSS_COLOR_SPACE_XYZ:
      switch (coord)
        {
        case 0: return "x";
        case 1: return "y";
        case 2: return "z";
        default: g_assert_not_reached ();
        }
    case BOBGUI_CSS_COLOR_SPACE_HSL:
      switch (coord)
        {
        case 0: return "h";
        case 1: return "s";
        case 2: return "l";
        default: g_assert_not_reached ();
        }
    case BOBGUI_CSS_COLOR_SPACE_HWB:
      switch (coord)
        {
        case 0: return "h";
        case 1: return "w";
        case 2: return "b";
        default: g_assert_not_reached ();
        }
    case BOBGUI_CSS_COLOR_SPACE_OKLAB:
      switch (coord)
        {
        case 0: return "l";
        case 1: return "a";
        case 2: return "b";
        default: g_assert_not_reached ();
        }
    case BOBGUI_CSS_COLOR_SPACE_OKLCH:
      switch (coord)
        {
        case 0: return "l";
        case 1: return "c";
        case 2: return "h";
        default: g_assert_not_reached ();
        }
    default:
      g_assert_not_reached ();
    }
}

void
bobgui_css_color_space_get_coord_range (BobguiCssColorSpace  color_space,
                                     gboolean          legacy_rgb_scale,
                                     guint             coord,
                                     float            *lower,
                                     float            *upper)
{
  if (coord == 3)
    {
      *lower = 0;
      *upper = 1;
      return;
    }

  switch (color_space)
    {
    case BOBGUI_CSS_COLOR_SPACE_SRGB:
      *lower = 0;
      *upper = legacy_rgb_scale ? 255 : 1;
      return;
    case BOBGUI_CSS_COLOR_SPACE_SRGB_LINEAR:
    case BOBGUI_CSS_COLOR_SPACE_DISPLAY_P3:
    case BOBGUI_CSS_COLOR_SPACE_XYZ:
    case BOBGUI_CSS_COLOR_SPACE_REC2020:
    case BOBGUI_CSS_COLOR_SPACE_REC2100_PQ:
      *lower = 0;
      *upper = 1;
      return;
    case BOBGUI_CSS_COLOR_SPACE_HSL:
    case BOBGUI_CSS_COLOR_SPACE_HWB:
      switch (coord)
        {
        case 0: *lower = *upper = NAN; return;
        case 1:
        case 2: *lower = 0; *upper = 100; return;
        default: g_assert_not_reached ();
        }
    case BOBGUI_CSS_COLOR_SPACE_OKLAB:
      switch (coord)
        {
        case 0: *lower = 0; *upper = 1; return;
        case 1:
        case 2: *lower = -0.4; *upper = 0.4; return;
        default: g_assert_not_reached ();
        }
    case BOBGUI_CSS_COLOR_SPACE_OKLCH:
      switch (coord)
        {
        case 0: *lower = 0; *upper = 1; return;
        case 1: *lower = 0; *upper = 0.4; return;
        case 2: *lower = *upper = NAN; return;
        default: g_assert_not_reached ();
        }
    default:
      g_assert_not_reached ();
    }
}

static gboolean
color_space_is_polar (BobguiCssColorSpace color_space)
{
  switch (color_space)
    {
    case BOBGUI_CSS_COLOR_SPACE_SRGB:
    case BOBGUI_CSS_COLOR_SPACE_SRGB_LINEAR:
    case BOBGUI_CSS_COLOR_SPACE_OKLAB:
    case BOBGUI_CSS_COLOR_SPACE_DISPLAY_P3:
    case BOBGUI_CSS_COLOR_SPACE_XYZ:
    case BOBGUI_CSS_COLOR_SPACE_REC2020:
    case BOBGUI_CSS_COLOR_SPACE_REC2100_PQ:
      return FALSE;

    case BOBGUI_CSS_COLOR_SPACE_HSL:
    case BOBGUI_CSS_COLOR_SPACE_HWB:
    case BOBGUI_CSS_COLOR_SPACE_OKLCH:
      return TRUE;

    default:
      g_assert_not_reached ();
    }
}

/* }}} */
/* {{{ Color conversion */

static void
convert_to_rectangular (BobguiCssColor *output)
{
  float v[4];
  gboolean no_missing[4] = { 0, };

  switch (output->color_space)
    {
    case BOBGUI_CSS_COLOR_SPACE_SRGB:
    case BOBGUI_CSS_COLOR_SPACE_SRGB_LINEAR:
    case BOBGUI_CSS_COLOR_SPACE_OKLAB:
    case BOBGUI_CSS_COLOR_SPACE_DISPLAY_P3:
    case BOBGUI_CSS_COLOR_SPACE_XYZ:
    case BOBGUI_CSS_COLOR_SPACE_REC2020:
    case BOBGUI_CSS_COLOR_SPACE_REC2100_PQ:
      break;

    case BOBGUI_CSS_COLOR_SPACE_HSL:
      bobgui_hsl_to_rgb (output->values[0],
                      output->values[1] / 100,
                      output->values[2] / 100,
                      &v[0], &v[1], &v[2]);
      v[3] = output->values[3];
      bobgui_css_color_init_with_missing (output, BOBGUI_CSS_COLOR_SPACE_SRGB, v, no_missing);
      break;

    case BOBGUI_CSS_COLOR_SPACE_HWB:
      bobgui_hwb_to_rgb (output->values[0],
                      output->values[1] / 100,
                      output->values[2] / 100,
                      &v[0], &v[1], &v[2]);
      v[3] = output->values[3];
      bobgui_css_color_init_with_missing (output, BOBGUI_CSS_COLOR_SPACE_SRGB, v, no_missing);
      break;

    case BOBGUI_CSS_COLOR_SPACE_OKLCH:
      bobgui_oklch_to_oklab (output->values[0],
                          output->values[1],
                          output->values[2],
                          &v[0], &v[1], &v[2]);
      v[3] = output->values[3];
      bobgui_css_color_init_with_missing (output, BOBGUI_CSS_COLOR_SPACE_OKLAB, v, no_missing);
      break;

    default:
      g_assert_not_reached ();
    }
}

static void
convert_to_linear (BobguiCssColor *output)
{
  float v[4];

  g_assert (output->color_space == BOBGUI_CSS_COLOR_SPACE_SRGB ||
            output->color_space == BOBGUI_CSS_COLOR_SPACE_SRGB_LINEAR ||
            output->color_space == BOBGUI_CSS_COLOR_SPACE_OKLAB ||
            output->color_space == BOBGUI_CSS_COLOR_SPACE_DISPLAY_P3 ||
            output->color_space == BOBGUI_CSS_COLOR_SPACE_XYZ ||
            output->color_space == BOBGUI_CSS_COLOR_SPACE_REC2020 ||
            output->color_space == BOBGUI_CSS_COLOR_SPACE_REC2100_PQ);

  if (output->color_space == BOBGUI_CSS_COLOR_SPACE_SRGB)
    {
      bobgui_rgb_to_linear_srgb (output->values[0],
                              output->values[1],
                              output->values[2],
                              &v[0], &v[1], &v[2]);
      v[3] = output->values[3];
      bobgui_css_color_init (output, BOBGUI_CSS_COLOR_SPACE_SRGB_LINEAR, v);
    }
  else if (output->color_space == BOBGUI_CSS_COLOR_SPACE_DISPLAY_P3)
    {
      bobgui_p3_to_rgb (output->values[0],
                     output->values[1],
                     output->values[2],
                     &v[0], &v[1], &v[2]);
      bobgui_rgb_to_linear_srgb (v[0], v[1], v[2],
                              &v[0], &v[1], &v[2]);
      v[3] = output->values[3];
      bobgui_css_color_init (output, BOBGUI_CSS_COLOR_SPACE_SRGB_LINEAR, v);
    }
  else if (output->color_space == BOBGUI_CSS_COLOR_SPACE_XYZ)
    {
      bobgui_xyz_to_linear_srgb (output->values[0],
                              output->values[1],
                              output->values[2],
                              &v[0], &v[1], &v[2]);
      v[3] = output->values[3];
      bobgui_css_color_init (output, BOBGUI_CSS_COLOR_SPACE_SRGB_LINEAR, v);
    }
  else if (output->color_space == BOBGUI_CSS_COLOR_SPACE_REC2020)
    {
      bobgui_rec2020_to_xyz (output->values[0],
                          output->values[1],
                          output->values[2],
                          &v[0], &v[1], &v[2]);
      bobgui_xyz_to_linear_srgb (v[0], v[1], v[2],
                              &v[0], &v[1], &v[2]);
      v[3] = output->values[3];
      bobgui_css_color_init (output, BOBGUI_CSS_COLOR_SPACE_SRGB_LINEAR, v);
    }
  else if (output->color_space == BOBGUI_CSS_COLOR_SPACE_REC2100_PQ)
    {
      bobgui_rec2100_pq_to_rec2100_linear (output->values[0],
                                        output->values[1],
                                        output->values[2],
                                        &v[0], &v[1], &v[2]);
      bobgui_rec2100_linear_to_rec2020_linear (v[0], v[1], v[2],
                                            &v[0], &v[1], &v[2]);
      bobgui_rec2020_linear_to_xyz (v[0], v[1], v[2],
                                 &v[0], &v[1], &v[2]);
      bobgui_xyz_to_linear_srgb (v[0], v[1], v[2],
                              &v[0], &v[1], &v[2]);
      v[3] = output->values[3];
      bobgui_css_color_init (output, BOBGUI_CSS_COLOR_SPACE_SRGB_LINEAR, v);
    }
}

static void
convert_from_linear (BobguiCssColor      *output,
                     BobguiCssColorSpace  dest)
{
  float v[4];

  g_assert (output->color_space == BOBGUI_CSS_COLOR_SPACE_SRGB_LINEAR ||
            output->color_space == BOBGUI_CSS_COLOR_SPACE_OKLAB);

  switch (dest)
    {
    case BOBGUI_CSS_COLOR_SPACE_SRGB:
    case BOBGUI_CSS_COLOR_SPACE_HSL:
    case BOBGUI_CSS_COLOR_SPACE_HWB:
      bobgui_linear_srgb_to_rgb (output->values[0],
                              output->values[1],
                              output->values[2],
                              &v[0], &v[1], &v[2]);
      v[3] = output->values[3];
      bobgui_css_color_init (output, BOBGUI_CSS_COLOR_SPACE_SRGB, v);
      break;

    case BOBGUI_CSS_COLOR_SPACE_DISPLAY_P3:
      bobgui_linear_srgb_to_rgb (output->values[0],
                              output->values[1],
                              output->values[2],
                              &v[0], &v[1], &v[2]);
      bobgui_rgb_to_p3 (v[0], v[1], v[2],
                     &v[0], &v[1], &v[2]);
      v[3] = output->values[3];
      bobgui_css_color_init (output, BOBGUI_CSS_COLOR_SPACE_DISPLAY_P3, v);
      break;

    case BOBGUI_CSS_COLOR_SPACE_XYZ:
      bobgui_linear_srgb_to_xyz (output->values[0],
                              output->values[1],
                              output->values[2],
                              &v[0], &v[1], &v[2]);
      v[3] = output->values[3];
      bobgui_css_color_init (output, BOBGUI_CSS_COLOR_SPACE_XYZ, v);
      break;

    case BOBGUI_CSS_COLOR_SPACE_REC2020:
      bobgui_linear_srgb_to_xyz (output->values[0],
                              output->values[1],
                              output->values[2],
                              &v[0], &v[1], &v[2]);
      bobgui_xyz_to_rec2020 (v[0], v[1], v[2],
                          &v[0], &v[1], &v[2]);
      v[3] = output->values[3];
      bobgui_css_color_init (output, BOBGUI_CSS_COLOR_SPACE_REC2020, v);
      break;

    case BOBGUI_CSS_COLOR_SPACE_REC2100_PQ:
      bobgui_linear_srgb_to_xyz (output->values[0],
                              output->values[1],
                              output->values[2],
                              &v[0], &v[1], &v[2]);
      bobgui_xyz_to_rec2020_linear (v[0], v[1], v[2],
                                 &v[0], &v[1], &v[2]);
      bobgui_rec2020_linear_to_rec2100_linear (v[0], v[1], v[2],
                                            &v[0], &v[1], &v[2]);
      bobgui_rec2100_linear_to_rec2100_pq (v[0], v[1], v[2],
                                        &v[0], &v[1], &v[2]);
      v[3] = output->values[3];
      bobgui_css_color_init (output, BOBGUI_CSS_COLOR_SPACE_REC2100_PQ, v);
      break;

    case BOBGUI_CSS_COLOR_SPACE_SRGB_LINEAR:
    case BOBGUI_CSS_COLOR_SPACE_OKLAB:
    case BOBGUI_CSS_COLOR_SPACE_OKLCH:
      break;

    default:
      g_assert_not_reached ();
    }
}

static void
convert_from_rectangular (BobguiCssColor      *output,
                          BobguiCssColorSpace  dest)
{
  float v[4];

  switch (dest)
    {
    case BOBGUI_CSS_COLOR_SPACE_SRGB:
    case BOBGUI_CSS_COLOR_SPACE_SRGB_LINEAR:
    case BOBGUI_CSS_COLOR_SPACE_OKLAB:
    case BOBGUI_CSS_COLOR_SPACE_DISPLAY_P3:
    case BOBGUI_CSS_COLOR_SPACE_XYZ:
    case BOBGUI_CSS_COLOR_SPACE_REC2020:
    case BOBGUI_CSS_COLOR_SPACE_REC2100_PQ:
      g_assert (output->color_space == dest);
      break;

    case BOBGUI_CSS_COLOR_SPACE_HSL:
      g_assert (output->color_space == BOBGUI_CSS_COLOR_SPACE_SRGB);
      bobgui_rgb_to_hsl (output->values[0],
                      output->values[1],
                      output->values[2],
                      &v[0], &v[1], &v[2]);
      v[1] *= 100;
      v[2] *= 100;
      v[3] = output->values[3];

      bobgui_css_color_init (output, dest, v);
      break;

    case BOBGUI_CSS_COLOR_SPACE_HWB:
      g_assert (output->color_space == BOBGUI_CSS_COLOR_SPACE_SRGB);
      bobgui_rgb_to_hwb (output->values[0],
                      output->values[1],
                      output->values[2],
                      &v[0], &v[1], &v[2]);

      v[1] *= 100;
      v[2] *= 100;
      v[3] = output->values[3];

      bobgui_css_color_init (output, dest, v);
      break;

    case BOBGUI_CSS_COLOR_SPACE_OKLCH:
      g_assert (output->color_space == BOBGUI_CSS_COLOR_SPACE_OKLAB);
      bobgui_oklab_to_oklch (output->values[0],
                          output->values[1],
                          output->values[2],
                          &v[0], &v[1], &v[2]);
      v[3] = output->values[3];

      bobgui_css_color_init (output, dest, v);
      break;

    default:
      g_assert_not_reached ();
    }
}

static void
convert_linear_to_linear (BobguiCssColor      *output,
                          BobguiCssColorSpace  dest)
{
  BobguiCssColorSpace dest_linear;
  float v[4];

  if (dest == BOBGUI_CSS_COLOR_SPACE_OKLCH ||
      dest == BOBGUI_CSS_COLOR_SPACE_OKLAB)
    dest_linear = BOBGUI_CSS_COLOR_SPACE_OKLAB;
  else
    dest_linear = BOBGUI_CSS_COLOR_SPACE_SRGB_LINEAR;

  if (dest_linear == BOBGUI_CSS_COLOR_SPACE_SRGB_LINEAR &&
      output->color_space == BOBGUI_CSS_COLOR_SPACE_OKLAB)
    {
      bobgui_oklab_to_linear_srgb (output->values[0],
                                output->values[1],
                                output->values[2],
                                &v[0], &v[1], &v[2]);
      v[3] = output->values[3];
      bobgui_css_color_init (output, dest_linear, v);
    }
  else if (dest_linear == BOBGUI_CSS_COLOR_SPACE_OKLAB &&
           output->color_space == BOBGUI_CSS_COLOR_SPACE_SRGB_LINEAR)
    {
      bobgui_linear_srgb_to_oklab (output->values[0],
                                output->values[1],
                                output->values[2],
                                &v[0], &v[1], &v[2]);
      v[3] = output->values[3];
      bobgui_css_color_init (output, dest_linear, v);
    }

  g_assert (output->color_space == dest_linear);
}

/* See https://www.w3.org/TR/css-color-4/#color-conversion */
void
bobgui_css_color_convert (const BobguiCssColor *input,
                       BobguiCssColorSpace   dest,
                       BobguiCssColor       *output)
{
  bobgui_css_color_init_from_color (output, input);

  convert_to_rectangular (output);
  convert_to_linear (output);

  /* FIXME: White point adaptation goes here */

  g_assert (output->color_space == BOBGUI_CSS_COLOR_SPACE_SRGB_LINEAR ||
            output->color_space == BOBGUI_CSS_COLOR_SPACE_OKLAB);

  convert_linear_to_linear (output, dest);
  convert_from_linear (output, dest);

  /* FIXME: Gamut mapping goes here */

  convert_from_rectangular (output, dest);
}

/* }}} */
/* {{{ Color interpolation */

static void
adjust_hue (float                  *h1,
            float                  *h2,
            BobguiCssHueInterpolation  interp)
{

  switch (interp)
    {
    case BOBGUI_CSS_HUE_INTERPOLATION_SHORTER:
      {
        float d = *h2 - *h1;

        if (d > 180)
          *h1 += 360;
        else if (d < -180)
          *h2 += 360;
      }
      break;

    case BOBGUI_CSS_HUE_INTERPOLATION_LONGER:
      {
        float d = *h2 - *h1;

        if (0 < d && d < 180)
          *h1 += 360;
        else if (-180 < d && d <= 0)
          *h2 += 360;
      }
      break;

    case BOBGUI_CSS_HUE_INTERPOLATION_INCREASING:
      if (*h2 < *h1)
        *h2 += 360;
      break;

    case BOBGUI_CSS_HUE_INTERPOLATION_DECREASING:
      if (*h1 < *h2)
        *h1 += 360;
      break;

    default:
      g_assert_not_reached ();
    }
}

static void
apply_hue_interpolation (BobguiCssColor            *from,
                         BobguiCssColor            *to,
                         BobguiCssColorSpace        in,
                         BobguiCssHueInterpolation  interp)
{
  switch (in)
    {
    case BOBGUI_CSS_COLOR_SPACE_SRGB:
    case BOBGUI_CSS_COLOR_SPACE_SRGB_LINEAR:
    case BOBGUI_CSS_COLOR_SPACE_OKLAB:
    case BOBGUI_CSS_COLOR_SPACE_DISPLAY_P3:
    case BOBGUI_CSS_COLOR_SPACE_XYZ:
    case BOBGUI_CSS_COLOR_SPACE_REC2020:
    case BOBGUI_CSS_COLOR_SPACE_REC2100_PQ:
      break;

    case BOBGUI_CSS_COLOR_SPACE_HSL:
    case BOBGUI_CSS_COLOR_SPACE_HWB:
      adjust_hue (&from->values[0], &to->values[0], interp);
      break;

    case BOBGUI_CSS_COLOR_SPACE_OKLCH:
      adjust_hue (&from->values[2], &to->values[2], interp);
      break;

    default:
      g_assert_not_reached ();
    }
}

static void
normalize_hue_component (float *v)
{
  *v = fmod (*v, 360);
  if (*v < 0)
    *v += 360;
}

static void
normalize_hue (BobguiCssColor *color)
{
  switch (color->color_space)
    {
    case BOBGUI_CSS_COLOR_SPACE_SRGB:
    case BOBGUI_CSS_COLOR_SPACE_SRGB_LINEAR:
    case BOBGUI_CSS_COLOR_SPACE_OKLAB:
    case BOBGUI_CSS_COLOR_SPACE_DISPLAY_P3:
    case BOBGUI_CSS_COLOR_SPACE_XYZ:
    case BOBGUI_CSS_COLOR_SPACE_REC2020:
    case BOBGUI_CSS_COLOR_SPACE_REC2100_PQ:
      break;

    case BOBGUI_CSS_COLOR_SPACE_HSL:
    case BOBGUI_CSS_COLOR_SPACE_HWB:
      normalize_hue_component (&color->values[0]);
      break;

    case BOBGUI_CSS_COLOR_SPACE_OKLCH:
      normalize_hue_component  (&color->values[2]);
      break;

    default:
      g_assert_not_reached ();
    }
}

static inline void
premultiply_component (BobguiCssColor *color,
                       guint        i)
{
  if ((color->missing & (1 << i)) != 0)
    return;

  color->values[i] *= color->values[3];
}

static void
premultiply (BobguiCssColor *color)
{
  if (color->missing & (1 << 3))
    return;

  switch (color->color_space)
    {
    case BOBGUI_CSS_COLOR_SPACE_SRGB:
    case BOBGUI_CSS_COLOR_SPACE_SRGB_LINEAR:
    case BOBGUI_CSS_COLOR_SPACE_OKLAB:
    case BOBGUI_CSS_COLOR_SPACE_DISPLAY_P3:
    case BOBGUI_CSS_COLOR_SPACE_XYZ:
    case BOBGUI_CSS_COLOR_SPACE_REC2020:
    case BOBGUI_CSS_COLOR_SPACE_REC2100_PQ:
      premultiply_component (color, 0);
      premultiply_component (color, 1);
      premultiply_component (color, 2);
      break;

    case BOBGUI_CSS_COLOR_SPACE_HSL:
    case BOBGUI_CSS_COLOR_SPACE_HWB:
      premultiply_component (color, 1);
      premultiply_component (color, 2);
      break;

    case BOBGUI_CSS_COLOR_SPACE_OKLCH:
      premultiply_component (color, 0);
      premultiply_component (color, 1);
      break;

    default:
      g_assert_not_reached ();
    }
}

static void
unpremultiply_component (BobguiCssColor *color,
                         guint        i)
{
  if ((color->missing & (1 << i)) != 0)
    return;

  color->values[i] /= color->values[3];
}

static void
unpremultiply (BobguiCssColor *color)
{
  if ((color->missing & (1 << 3)) != 0 || color->values[3] == 0)
    return;

  switch (color->color_space)
    {
    case BOBGUI_CSS_COLOR_SPACE_SRGB:
    case BOBGUI_CSS_COLOR_SPACE_SRGB_LINEAR:
    case BOBGUI_CSS_COLOR_SPACE_OKLAB:
    case BOBGUI_CSS_COLOR_SPACE_DISPLAY_P3:
    case BOBGUI_CSS_COLOR_SPACE_XYZ:
    case BOBGUI_CSS_COLOR_SPACE_REC2020:
    case BOBGUI_CSS_COLOR_SPACE_REC2100_PQ:
      unpremultiply_component (color, 0);
      unpremultiply_component (color, 1);
      unpremultiply_component (color, 2);
      break;

    case BOBGUI_CSS_COLOR_SPACE_HSL:
    case BOBGUI_CSS_COLOR_SPACE_HWB:
      unpremultiply_component (color, 1);
      unpremultiply_component (color, 2);
      break;

    case BOBGUI_CSS_COLOR_SPACE_OKLCH:
      unpremultiply_component (color, 0);
      unpremultiply_component (color, 1);
      break;

    default:
      g_assert_not_reached ();
    }
}

static void
collect_analogous_missing (const BobguiCssColor *color,
                           BobguiCssColorSpace   color_space,
                           gboolean           missing[4])
{
  /* Coords for red, green, blue, lightness, colorfulness, hue,
   * opposite a, opposite b, alpha, for each of our colorspaces,
   */
  static int analogous[][9] = {
    {  0,  1,  2, -1, -1, -1, -1, -1, 3 }, /* srgb */
    {  0,  1,  2, -1, -1, -1, -1, -1, 3 }, /* srgb-linear */
    { -1, -1, -1,  2,  1,  0, -1, -1, 3 }, /* hsl */
    { -1, -1, -1, -1, -1,  0, -1, -1, 3 }, /* hwb */
    { -1, -1, -1,  0, -1, -1,  1,  2, 3 }, /* oklab */
    { -1, -1, -1,  0,  1,  2, -1, -1, 3 }, /* oklch */
    {  0,  1,  2, -1, -1, -1, -1, -1, 3 }, /* display-p3 */
    {  0,  1,  2, -1, -1, -1, -1, -1, 3 }, /* xyz */
    {  0,  1,  2, -1, -1, -1, -1, -1, 3 }, /* rec2020 */
    {  0,  1,  2, -1, -1, -1, -1, -1, 3 }, /* rec2100-pq */
  };

  int *src = analogous[color->color_space];
  int *dest = analogous[color_space];

  for (guint i = 0; i < 4; i++)
    missing[i] = 0;

  for (guint i = 0; i < 4; i++)
    {
      if ((color->missing & (1 << i)) == 0)
        continue;

      for (guint j = 0; j < 9; j++)
        {
          if (src[j] == i)
            {
              int idx = dest[j];

              if (idx != -1)
                missing[idx] = TRUE;

              break;
            }
        }
    }
}

/* See https://www.w3.org/TR/css-color-4/#interpolation */
void
bobgui_css_color_interpolate (const BobguiCssColor      *from,
                           const BobguiCssColor      *to,
                           float                   progress,
                           BobguiCssColorSpace        in,
                           BobguiCssHueInterpolation  interp,
                           BobguiCssColor            *output)
{
  BobguiCssColor from1, to1;
  gboolean from_missing[4];
  gboolean to_missing[4];
  gboolean missing[4];
  float v[4];

  collect_analogous_missing (from, in, from_missing);
  collect_analogous_missing (to, in, to_missing);

  bobgui_css_color_convert (from, in, &from1);
  bobgui_css_color_convert (to, in, &to1);

  for (guint i = 0; i < 4; i++)
    {
      gboolean m1 = from_missing[i];
      gboolean m2 = to_missing[i];

      if (m1 && !m2)
        from1.values[i] = to1.values[i];
      else if (!m1 && m2)
        to1.values[i] = from1.values[i];

      missing[i] = from_missing[i] && to_missing[i];
    }

  from1.missing = 0;
  to1.missing = 0;

  apply_hue_interpolation (&from1, &to1, in, interp);

  premultiply (&from1);
  premultiply (&to1);

  v[0] = from1.values[0] * (1 - progress) + to1.values[0] * progress;
  v[1] = from1.values[1] * (1 - progress) + to1.values[1] * progress;
  v[2] = from1.values[2] * (1 - progress) + to1.values[2] * progress;
  v[3] = from1.values[3] * (1 - progress) + to1.values[3] * progress;

  bobgui_css_color_init_with_missing (output, in, v, missing);

  normalize_hue (output);

  unpremultiply (output);
}

static gboolean
parse_hue_interpolation (BobguiCssParser           *parser,
                         BobguiCssHueInterpolation *interp)
{
  const BobguiCssToken *token = bobgui_css_parser_get_token (parser);

  if (bobgui_css_token_is_ident (token, "shorter"))
    {
      bobgui_css_parser_consume_token (parser);
      *interp = BOBGUI_CSS_HUE_INTERPOLATION_SHORTER;
    }
  else if (bobgui_css_token_is_ident (token, "longer"))
    {
      bobgui_css_parser_consume_token (parser);
      *interp = BOBGUI_CSS_HUE_INTERPOLATION_LONGER;
    }
  else if (bobgui_css_token_is_ident (token, "increasing"))
    {
      bobgui_css_parser_consume_token (parser);
      *interp = BOBGUI_CSS_HUE_INTERPOLATION_INCREASING;
    }
  else if (bobgui_css_token_is_ident (token, "decreasing"))
    {
      bobgui_css_parser_consume_token (parser);
      *interp = BOBGUI_CSS_HUE_INTERPOLATION_DECREASING;
    }
  else if (bobgui_css_token_is_ident (token, "hue"))
    {
      bobgui_css_parser_error_syntax (parser, "'hue' goes after the interpolation method");
      return FALSE;
    }
  else
    {
      *interp = BOBGUI_CSS_HUE_INTERPOLATION_SHORTER;
      return TRUE;
    }

  if (!bobgui_css_parser_try_ident (parser, "hue"))
    {
      bobgui_css_parser_error_syntax (parser, "Expected 'hue'");
      return FALSE;
    }

  return TRUE;
}

gboolean
bobgui_css_color_interpolation_method_can_parse (BobguiCssParser *parser)
{
  return bobgui_css_token_is_ident (bobgui_css_parser_get_token (parser), "in");
}

gboolean
bobgui_css_color_interpolation_method_parse (BobguiCssParser           *parser,
                                          BobguiCssColorSpace       *in,
                                          BobguiCssHueInterpolation *interp)
{
  const BobguiCssToken *token;

  if (!bobgui_css_parser_try_ident (parser, "in"))
    {
      bobgui_css_parser_error_syntax (parser, "Expected 'in'");
      return FALSE;
    }

  token = bobgui_css_parser_get_token (parser);

  if (bobgui_css_token_is_ident (token, "srgb"))
    {
      bobgui_css_parser_consume_token (parser);
      *in = BOBGUI_CSS_COLOR_SPACE_SRGB;
    }
  else if (bobgui_css_token_is_ident (token, "srgb-linear"))
    {
      bobgui_css_parser_consume_token (parser);
      *in = BOBGUI_CSS_COLOR_SPACE_SRGB_LINEAR;
    }
  else if (bobgui_css_token_is_ident (token, "hsl"))
    {
      bobgui_css_parser_consume_token (parser);
      *in = BOBGUI_CSS_COLOR_SPACE_HSL;
    }
  else if (bobgui_css_token_is_ident (token, "hwb"))
    {
      bobgui_css_parser_consume_token (parser);
      *in = BOBGUI_CSS_COLOR_SPACE_HWB;
    }
  else if (bobgui_css_token_is_ident (token, "oklab"))
    {
      bobgui_css_parser_consume_token (parser);
      *in = BOBGUI_CSS_COLOR_SPACE_OKLAB;
    }
  else if (bobgui_css_token_is_ident (token, "oklch"))
    {
      bobgui_css_parser_consume_token (parser);
      *in = BOBGUI_CSS_COLOR_SPACE_OKLCH;
    }
  else if (bobgui_css_token_is_ident (token, "display-p3"))
    {
      bobgui_css_parser_consume_token (parser);
      *in = BOBGUI_CSS_COLOR_SPACE_DISPLAY_P3;
    }
  else if (bobgui_css_token_is_ident (token, "xyz"))
    {
      bobgui_css_parser_consume_token (parser);
      *in = BOBGUI_CSS_COLOR_SPACE_XYZ;
    }
  else if (bobgui_css_token_is_ident (token, "rec2020"))
    {
      bobgui_css_parser_consume_token (parser);
      *in = BOBGUI_CSS_COLOR_SPACE_REC2020;
    }
  else if (bobgui_css_token_is_ident (token, "rec2100-pq"))
    {
      bobgui_css_parser_consume_token (parser);
      *in = BOBGUI_CSS_COLOR_SPACE_REC2100_PQ;
    }
  else
    {
      bobgui_css_parser_error_syntax (parser, "Invalid color space: %s", bobgui_css_token_to_string (token));
      return FALSE;
    }

  if (color_space_is_polar (*in))
    return parse_hue_interpolation (parser, interp);

  return TRUE;
}


void
bobgui_css_color_interpolation_method_print (BobguiCssColorSpace        in,
                                          BobguiCssHueInterpolation  interp,
                                          GString                *string)
{
  g_string_append (string, "in ");

  switch (in)
  {
    case BOBGUI_CSS_COLOR_SPACE_SRGB:
      g_string_append (string, "srgb");
      break;
    case BOBGUI_CSS_COLOR_SPACE_SRGB_LINEAR:
      g_string_append (string, "srgb-linear");
      break;
    case BOBGUI_CSS_COLOR_SPACE_HSL:
      g_string_append (string, "hsl");
      break;
    case BOBGUI_CSS_COLOR_SPACE_HWB:
      g_string_append (string, "hwb");
      break;
    case BOBGUI_CSS_COLOR_SPACE_OKLCH:
      g_string_append (string, "oklch");
      break;
    case BOBGUI_CSS_COLOR_SPACE_OKLAB:
      g_string_append (string, "oklab");
      break;
    case BOBGUI_CSS_COLOR_SPACE_DISPLAY_P3:
      g_string_append (string, "display-p3");
      break;
    case BOBGUI_CSS_COLOR_SPACE_XYZ:
      g_string_append (string, "xyz");
      break;
    case BOBGUI_CSS_COLOR_SPACE_REC2020:
      g_string_append (string, "rec2020");
      break;
    case BOBGUI_CSS_COLOR_SPACE_REC2100_PQ:
      g_string_append (string, "rec2100-pq");
      break;
    default:
      g_assert_not_reached ();
  }

  if (!color_space_is_polar (in))
    return;

  switch (interp)
  {
    case BOBGUI_CSS_HUE_INTERPOLATION_SHORTER:
      /* shorter is the default mode, don't print it */
      break;
    case BOBGUI_CSS_HUE_INTERPOLATION_LONGER:
      g_string_append (string, " longer hue");
      break;
    case BOBGUI_CSS_HUE_INTERPOLATION_INCREASING:
      g_string_append (string, " increasing hue");
      break;
    case BOBGUI_CSS_HUE_INTERPOLATION_DECREASING:
      g_string_append (string, " decreasing hue");
      break;
    default:
      g_assert_not_reached ();
  }
}

/* }}} */
/* {{{ GdkColor conversion */

/*< private >
 * bobgui_css_color_space_get_color_state:
 * @color_space: a CSS color space
 *
 * Returns the best-matching GdkColorState for a given CSS color
 * space.
 *
 * Note that we don't guarantee a 1:1 match between CSS color
 * spaces and color states, so conversion of the color may
 * still be necessary.
 *
 * Returns: (transfer none): the `GdkColorState`
 */
GdkColorState *
bobgui_css_color_space_get_color_state (BobguiCssColorSpace color_space)
{
  switch (color_space)
    {
    case BOBGUI_CSS_COLOR_SPACE_SRGB:
    case BOBGUI_CSS_COLOR_SPACE_HSL:
    case BOBGUI_CSS_COLOR_SPACE_HWB:
      return GDK_COLOR_STATE_SRGB;

    case BOBGUI_CSS_COLOR_SPACE_OKLAB:
      return GDK_COLOR_STATE_OKLAB;

    case BOBGUI_CSS_COLOR_SPACE_OKLCH:
      return GDK_COLOR_STATE_OKLCH;

    case BOBGUI_CSS_COLOR_SPACE_SRGB_LINEAR:
      return GDK_COLOR_STATE_SRGB_LINEAR;

    case BOBGUI_CSS_COLOR_SPACE_REC2020:
    case BOBGUI_CSS_COLOR_SPACE_DISPLAY_P3:
    case BOBGUI_CSS_COLOR_SPACE_XYZ:
    case BOBGUI_CSS_COLOR_SPACE_REC2100_PQ:
      return GDK_COLOR_STATE_REC2100_PQ;
      break;

    default:
      g_assert_not_reached ();
    }
}

void
bobgui_css_color_to_color (const BobguiCssColor *css,
                        GdkColor          *color)
{
  switch (css->color_space)
    {
    case BOBGUI_CSS_COLOR_SPACE_SRGB:
      gdk_color_init (color, GDK_COLOR_STATE_SRGB, css->values);
      break;

    case BOBGUI_CSS_COLOR_SPACE_SRGB_LINEAR:
      gdk_color_init (color, GDK_COLOR_STATE_SRGB_LINEAR, css->values);
      break;

    case BOBGUI_CSS_COLOR_SPACE_REC2100_PQ:
      gdk_color_init (color, GDK_COLOR_STATE_REC2100_PQ, css->values);
      break;

    case BOBGUI_CSS_COLOR_SPACE_OKLAB:
      gdk_color_init (color, GDK_COLOR_STATE_OKLAB, css->values);
      break;

    case BOBGUI_CSS_COLOR_SPACE_OKLCH:
      gdk_color_init (color, GDK_COLOR_STATE_OKLCH, css->values);
      break;


    case BOBGUI_CSS_COLOR_SPACE_HSL:
    case BOBGUI_CSS_COLOR_SPACE_HWB:
      {
        BobguiCssColor tmp;
        bobgui_css_color_convert (css, BOBGUI_CSS_COLOR_SPACE_SRGB, &tmp);
        gdk_color_init (color, GDK_COLOR_STATE_SRGB, tmp.values);
      }
      break;

    case BOBGUI_CSS_COLOR_SPACE_REC2020:
    case BOBGUI_CSS_COLOR_SPACE_DISPLAY_P3:
    case BOBGUI_CSS_COLOR_SPACE_XYZ:
      {
        BobguiCssColor tmp;
        bobgui_css_color_convert (css, BOBGUI_CSS_COLOR_SPACE_REC2100_PQ, &tmp);
        gdk_color_init (color, GDK_COLOR_STATE_REC2100_PQ, tmp.values);
      }
      break;

    default:
      g_assert_not_reached ();
    }
}

GskHueInterpolation
bobgui_css_hue_interpolation_to_hue_interpolation (BobguiCssHueInterpolation interp)
{
  return (GskHueInterpolation) interp;
}

/* }}} */

/* vim:set foldmethod=marker: */
