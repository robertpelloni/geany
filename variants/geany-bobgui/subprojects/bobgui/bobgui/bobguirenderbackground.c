/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2010 Carlos Garnacho <carlosg@gnome.org>
 * Copyright (C) 2011 Red Hat, Inc.
 * 
 * Authors: Carlos Garnacho <carlosg@gnome.org>
 *          Cosimo Cecchi <cosimoc@gnome.org>
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

#include "bobguirenderbackgroundprivate.h"

#include "bobguicssarrayvalueprivate.h"
#include "bobguicssbgsizevalueprivate.h"
#include "bobguicssboxesprivate.h"
#include "bobguicsscornervalueprivate.h"
#include "bobguicssenumvalueprivate.h"
#include "bobguicssimagevalueprivate.h"
#include "bobguicssnumbervalueprivate.h"
#include "bobguicssshadowvalueprivate.h"
#include "bobguicsspositionvalueprivate.h"
#include "bobguicssrepeatvalueprivate.h"
#include "bobguicsscolorvalueprivate.h"
#include "bobguicssstyleprivate.h"
#include "bobguicsstypesprivate.h"
#include "bobguisnapshotprivate.h"

#include <math.h>

#include <gdk/gdk.h>

#include "gsk/gskroundedrectprivate.h"

static void
bobgui_theming_background_snapshot_color (BobguiCssBoxes       *boxes,
                                       BobguiSnapshot       *snapshot,
                                       const BobguiCssColor *bg_color,
                                       guint              n_bg_values)
{
  BobguiCssStyle *style = boxes->style;
  const GskRoundedRect *box;
  BobguiCssArea clip;
  GdkColor color;

  clip = _bobgui_css_area_value_get (_bobgui_css_array_value_get_nth (style->background->background_clip, n_bg_values - 1));
  box = bobgui_css_boxes_get_box (boxes, clip);

  bobgui_css_color_to_color (bg_color, &color);
  if (gsk_rounded_rect_is_rectilinear (box))
    {
      bobgui_snapshot_add_color (snapshot, &color, &box->bounds);
    }
  else
    {
      bobgui_snapshot_push_rounded_clip (snapshot, box);
      bobgui_snapshot_add_color (snapshot, &color, &box->bounds);
      bobgui_snapshot_pop (snapshot);
    }
  gdk_color_finish (&color);
}

static void
bobgui_theming_background_snapshot_layer (BobguiCssBoxes *bg,
                                       guint        idx,
                                       BobguiSnapshot *snapshot)
{
  BobguiCssStyle *style = bg->style;
  BobguiCssRepeatStyle hrepeat, vrepeat;
  const BobguiCssValue *pos, *repeat;
  BobguiCssImage *image;
  const GskRoundedRect *origin, *clip;
  double image_width, image_height;
  double width, height;
  double x, y;

  image = _bobgui_css_image_value_get_image (_bobgui_css_array_value_get_nth (style->used->background_image, idx));

  if (image == NULL)
    return;

  pos = _bobgui_css_array_value_get_nth (style->background->background_position, idx);
  repeat = _bobgui_css_array_value_get_nth (style->background->background_repeat, idx);

  origin = bobgui_css_boxes_get_box (bg,
                                  _bobgui_css_area_value_get (
                                      _bobgui_css_array_value_get_nth (style->background->background_origin, idx)));

  width = origin->bounds.size.width;
  height = origin->bounds.size.height;

  if (width <= 0 || height <= 0)
    return;

  clip = bobgui_css_boxes_get_box (bg,
                                _bobgui_css_area_value_get (
                                    _bobgui_css_array_value_get_nth (style->background->background_clip, idx)));

  _bobgui_css_bg_size_value_compute_size (_bobgui_css_array_value_get_nth (style->background->background_size, idx),
                                       image,
                                       width,
                                       height,
                                       &image_width,
                                       &image_height);

  if (image_width <= 0 || image_height <= 0)
    return;

  /* optimization */
  if (image_width == width)
    hrepeat = BOBGUI_CSS_REPEAT_STYLE_NO_REPEAT;
  else
    hrepeat = _bobgui_css_background_repeat_value_get_x (repeat);

  if (image_height == height)
    vrepeat = BOBGUI_CSS_REPEAT_STYLE_NO_REPEAT;
  else
    vrepeat = _bobgui_css_background_repeat_value_get_y (repeat);

  bobgui_snapshot_push_debug (snapshot, "Layer %u", idx);
  bobgui_snapshot_push_rounded_clip (snapshot, clip);

  x = _bobgui_css_position_value_get_x (pos, width - image_width) + origin->bounds.origin.x;
  y = _bobgui_css_position_value_get_y (pos, height - image_height) + origin->bounds.origin.y;
  if (hrepeat == BOBGUI_CSS_REPEAT_STYLE_NO_REPEAT && vrepeat == BOBGUI_CSS_REPEAT_STYLE_NO_REPEAT)
    {
      /* shortcut for normal case */
      if (x != 0 || y != 0)
        {
          bobgui_snapshot_save (snapshot);
          bobgui_snapshot_translate (snapshot, &GRAPHENE_POINT_INIT (x, y));
          bobgui_css_image_snapshot (image, snapshot, image_width, image_height);
          bobgui_snapshot_restore (snapshot);
        }
      else
        {
          bobgui_css_image_snapshot (image, snapshot, image_width, image_height);
        }
    }
  else
    {
      float repeat_width, repeat_height;
      graphene_rect_t fill_rect;

      /* If ‘background-repeat’ is ‘round’ for one (or both) dimensions,
       * there is a second step. The UA must scale the image in that
       * dimension (or both dimensions) so that it fits a whole number of
       * times in the background positioning area. In the case of the width
       * (height is analogous):
       *
       * If X ≠ 0 is the width of the image after step one and W is the width
       * of the background positioning area, then the rounded width
       * X' = W / round(W / X) where round() is a function that returns the
       * nearest natural number (integer greater than zero). 
       *
       * If ‘background-repeat’ is ‘round’ for one dimension only and if
       * ‘background-size’ is ‘auto’ for the other dimension, then there is
       * a third step: that other dimension is scaled so that the original
       * aspect ratio is restored. 
       */
      if (hrepeat == BOBGUI_CSS_REPEAT_STYLE_ROUND)
        {
          double n = round (width / image_width);

          n = MAX (1, n);

          if (vrepeat != BOBGUI_CSS_REPEAT_STYLE_ROUND
              /* && vsize == auto (it is by default) */)
            image_height *= width / (image_width * n);
          image_width = width / n;
        }
      if (vrepeat == BOBGUI_CSS_REPEAT_STYLE_ROUND)
        {
          double n = round (height / image_height);

          n = MAX (1, n);

          if (hrepeat != BOBGUI_CSS_REPEAT_STYLE_ROUND
              /* && hsize == auto (it is by default) */)
            image_width *= height / (image_height * n);
          image_height = height / n;
        }

      /* if hrepeat or vrepeat is 'space', we create a somewhat larger surface
       * to store the extra space. */
      if (hrepeat == BOBGUI_CSS_REPEAT_STYLE_SPACE)
        {
          double n = floor (width / image_width);
          repeat_width = n ? round (width / n) : 0;
        }
      else
        repeat_width = round (image_width);

      if (vrepeat == BOBGUI_CSS_REPEAT_STYLE_SPACE)
        {
          double n = floor (height / image_height);
          repeat_height = n ? round (height / n) : 0;
        }
      else
        repeat_height = round (image_height);

      fill_rect = clip->bounds;
      if (hrepeat == BOBGUI_CSS_REPEAT_STYLE_NO_REPEAT)
        {
          fill_rect.origin.x = _bobgui_css_position_value_get_x (pos, width - image_width);
          fill_rect.size.width = image_width;
        }
      if (vrepeat == BOBGUI_CSS_REPEAT_STYLE_NO_REPEAT)
        {
          fill_rect.origin.y = _bobgui_css_position_value_get_y (pos, height - image_height);
          fill_rect.size.height = image_height;
        }

      bobgui_snapshot_push_repeat (snapshot,
                                &fill_rect,
                                &GRAPHENE_RECT_INIT (
                                    x, y,
                                    repeat_width, repeat_height
                                ));

      bobgui_snapshot_translate (snapshot, &GRAPHENE_POINT_INIT (
                              x + 0.5 * (repeat_width - image_width),
                              y + 0.5 * (repeat_height - image_height)));
      bobgui_css_image_snapshot (image, snapshot, image_width, image_height);

      bobgui_snapshot_pop (snapshot);
    }

  bobgui_snapshot_pop (snapshot);
  bobgui_snapshot_pop (snapshot);
}

void
bobgui_css_style_snapshot_background (BobguiCssBoxes *boxes,
                                   BobguiSnapshot *snapshot)
{
  BobguiCssStyle *style = boxes->style;
  BobguiCssValue *background_image;
  const BobguiCssColor *bg_color;
  const BobguiCssValue *box_shadow;
  gboolean has_bg_color;
  gboolean has_bg_image;
  gboolean has_shadow;
  int idx;
  guint number_of_layers;

  if (style->background->base.type == BOBGUI_CSS_BACKGROUND_INITIAL_VALUES)
    return;

  background_image = style->used->background_image;
  bg_color = bobgui_css_color_value_get_color (style->used->background_color);
  box_shadow = style->used->box_shadow;

  has_bg_color = !bobgui_css_color_is_clear (bg_color);
  has_bg_image = _bobgui_css_image_value_get_image (_bobgui_css_array_value_get_nth (background_image, 0)) != NULL;
  has_shadow = !bobgui_css_shadow_value_is_none (box_shadow);

  /* This is the common default case of no background */
  if (!has_bg_color && !has_bg_image && !has_shadow)
    return;

  bobgui_snapshot_push_debug (snapshot, "CSS background");

  if (has_shadow)
    bobgui_css_shadow_value_snapshot_outset (box_shadow,
                                          snapshot,
                                          bobgui_css_boxes_get_border_box (boxes));

  number_of_layers = _bobgui_css_array_value_get_n_values (background_image);

  if (has_bg_image)
    {
      BobguiCssValue *blend_modes = style->background->background_blend_mode;
      GskBlendMode *blend_mode_values = g_alloca (sizeof (GskBlendMode) * number_of_layers);

      for (idx = number_of_layers - 1; idx >= 0; idx--)
        {
          blend_mode_values[idx] = _bobgui_css_blend_mode_value_get (_bobgui_css_array_value_get_nth (blend_modes, idx));

          if (blend_mode_values[idx] != GSK_BLEND_MODE_DEFAULT)
            bobgui_snapshot_push_blend (snapshot, blend_mode_values[idx]);
        }

      if (has_bg_color)
        bobgui_theming_background_snapshot_color (boxes, snapshot, bg_color, number_of_layers);

      for (idx = number_of_layers - 1; idx >= 0; idx--)
        {
          if (blend_mode_values[idx] == GSK_BLEND_MODE_DEFAULT)
            {
              bobgui_theming_background_snapshot_layer (boxes, idx, snapshot);
            }
          else
            {
              bobgui_snapshot_pop (snapshot);
              bobgui_theming_background_snapshot_layer (boxes, idx, snapshot);
              bobgui_snapshot_pop (snapshot);
            }
        }
    }
  else if (has_bg_color)
    {
      bobgui_theming_background_snapshot_color (boxes, snapshot, bg_color, number_of_layers);
    }

  if (has_shadow)
    bobgui_css_shadow_value_snapshot_inset (box_shadow,
                                         snapshot,
                                         bobgui_css_boxes_get_padding_box (boxes));

  bobgui_snapshot_pop (snapshot);
}

