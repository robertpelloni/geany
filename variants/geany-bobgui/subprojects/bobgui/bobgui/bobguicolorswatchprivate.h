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

#include <bobgui/bobguidrawingarea.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_COLOR_SWATCH                  (bobgui_color_swatch_get_type ())
#define BOBGUI_COLOR_SWATCH(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_COLOR_SWATCH, BobguiColorSwatch))
#define BOBGUI_IS_COLOR_SWATCH(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_COLOR_SWATCH))


typedef struct _BobguiColorSwatch        BobguiColorSwatch;

GType       bobgui_color_swatch_get_type         (void) G_GNUC_CONST;
BobguiWidget * bobgui_color_swatch_new              (void);
void        bobgui_color_swatch_set_rgba         (BobguiColorSwatch *swatch,
                                               const GdkRGBA  *color);
gboolean    bobgui_color_swatch_get_rgba         (BobguiColorSwatch *swatch,
                                               GdkRGBA        *color);
void        bobgui_color_swatch_set_hsva         (BobguiColorSwatch *swatch,
                                               double          h,
                                               double          s,
                                               double          v,
                                               double          a);
void        bobgui_color_swatch_set_can_drop     (BobguiColorSwatch *swatch,
                                               gboolean        can_drop);
void        bobgui_color_swatch_set_can_drag     (BobguiColorSwatch *swatch,
                                               gboolean        can_drag);
void        bobgui_color_swatch_set_icon         (BobguiColorSwatch *swatch,
                                               const char     *icon);
void        bobgui_color_swatch_set_use_alpha    (BobguiColorSwatch *swatch,
                                               gboolean        use_alpha);
void        bobgui_color_swatch_set_selectable   (BobguiColorSwatch *swatch,
                                               gboolean        selectable);
gboolean    bobgui_color_swatch_get_selectable   (BobguiColorSwatch *swatch);

void        bobgui_color_swatch_select    (BobguiColorSwatch *swatch);
void        bobgui_color_swatch_activate  (BobguiColorSwatch *swatch);
void        bobgui_color_swatch_customize (BobguiColorSwatch *swatch);

G_END_DECLS

