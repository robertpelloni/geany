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

G_BEGIN_DECLS

#define BOBGUI_TYPE_CSS_IMAGE_CROSS_FADE           (bobgui_css_image_cross_fade_get_type ())
#define BOBGUI_CSS_IMAGE_CROSS_FADE(obj)           (G_TYPE_CHECK_INSTANCE_CAST (obj, BOBGUI_TYPE_CSS_IMAGE_CROSS_FADE, BobguiCssImageCrossFade))
#define BOBGUI_CSS_IMAGE_CROSS_FADE_CLASS(cls)     (G_TYPE_CHECK_CLASS_CAST (cls, BOBGUI_TYPE_CSS_IMAGE_CROSS_FADE, BobguiCssImageCrossFadeClass))
#define BOBGUI_IS_CSS_IMAGE_CROSS_FADE(obj)        (G_TYPE_CHECK_INSTANCE_TYPE (obj, BOBGUI_TYPE_CSS_IMAGE_CROSS_FADE))
#define BOBGUI_IS_CSS_IMAGE_CROSS_FADE_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE (obj, BOBGUI_TYPE_CSS_IMAGE_CROSS_FADE))
#define BOBGUI_CSS_IMAGE_CROSS_FADE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_CSS_IMAGE_CROSS_FADE, BobguiCssImageCrossFadeClass))

typedef struct _BobguiCssImageCrossFade           BobguiCssImageCrossFade;
typedef struct _BobguiCssImageCrossFadeClass      BobguiCssImageCrossFadeClass;

struct _BobguiCssImageCrossFade
{
  BobguiCssImage parent;

  GArray *images;
  double total_progress;
};

struct _BobguiCssImageCrossFadeClass
{
  BobguiCssImageClass parent_class;
};

GType          bobgui_css_image_cross_fade_get_type              (void) G_GNUC_CONST;

BobguiCssImage *  _bobgui_css_image_cross_fade_new                  (BobguiCssImage      *start,
                                                               BobguiCssImage      *end,
                                                               double            progress);

G_END_DECLS

