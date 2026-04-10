/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2012 Red Hat, Inc.
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

#include <bobgui/bobguiscale.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_COLOR_SCALE            (bobgui_color_scale_get_type ())
#define BOBGUI_COLOR_SCALE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_COLOR_SCALE, BobguiColorScale))
#define BOBGUI_IS_COLOR_SCALE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_COLOR_SCALE))


typedef struct _BobguiColorScale BobguiColorScale;

typedef enum
{
  BOBGUI_COLOR_SCALE_HUE,
  BOBGUI_COLOR_SCALE_ALPHA
} BobguiColorScaleType;

GType       bobgui_color_scale_get_type        (void) G_GNUC_CONST;
BobguiWidget * bobgui_color_scale_new             (BobguiAdjustment     *adjustment,
                                             BobguiColorScaleType  type);
void        bobgui_color_scale_set_rgba        (BobguiColorScale     *scale,
                                             const GdkRGBA     *color);

void        bobgui_color_scale_snapshot_trough (BobguiColorScale     *scale,
                                             BobguiSnapshot       *snapshot,
                                             int                width,
                                             int                height);

G_END_DECLS

