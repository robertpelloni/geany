/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2011 Red Hat, Inc.
 *
 * Author: Cosimo Cecchi <cosimoc@gnome.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <cairo.h>
#include <pango/pango.h>

#include <bobgui/css/bobguicss.h>
#include "bobgui/css/bobguicsstokenizerprivate.h"
#include "bobgui/css/bobguicssparserprivate.h"
#include "bobguiborder.h"
#include "bobguitypes.h"
#include "bobguicssvalueprivate.h"
#include "bobguisnapshot.h"
#include "gdk/gdkcolorprivate.h"

G_BEGIN_DECLS

BobguiCssValue *   bobgui_css_shadow_value_new_none         (void);
BobguiCssValue *   bobgui_css_shadow_value_new_filter       (const BobguiCssValue        *other);

BobguiCssValue *   bobgui_css_shadow_value_parse            (BobguiCssParser             *parser,
                                                       gboolean                  box_shadow_mode);
BobguiCssValue *   bobgui_css_shadow_value_parse_filter     (BobguiCssParser             *parser);

void            bobgui_css_shadow_value_get_extents      (const BobguiCssValue        *shadow,
                                                       BobguiBorder                *border);
void            bobgui_css_shadow_value_snapshot_outset  (const BobguiCssValue        *shadow,
                                                       BobguiSnapshot              *snapshot,
                                                       const GskRoundedRect     *border_box);
void            bobgui_css_shadow_value_snapshot_inset   (const BobguiCssValue        *shadow,
                                                       BobguiSnapshot              *snapshot,
                                                       const GskRoundedRect     *padding_box);

gboolean        bobgui_css_shadow_value_is_clear         (const BobguiCssValue        *shadow) G_GNUC_PURE;
gboolean        bobgui_css_shadow_value_is_none          (const BobguiCssValue        *shadow) G_GNUC_PURE;

gboolean        bobgui_css_shadow_value_push_snapshot    (const BobguiCssValue        *value,
                                                       BobguiSnapshot              *snapshot);
void            bobgui_css_shadow_value_pop_snapshot     (const BobguiCssValue        *value,
                                                       BobguiSnapshot              *snapshot);

guint           bobgui_css_shadow_value_get_n_shadows    (const BobguiCssValue        *value);

void            bobgui_css_shadow_value_get_offset       (const BobguiCssValue        *value,
                                                       guint                     n,
                                                       graphene_point_t         *offset);

void            bobgui_css_shadow_value_get_color        (const BobguiCssValue        *value,
                                                       guint                     n,
                                                       GdkColor                 *color);

double          bobgui_css_shadow_value_get_radius       (const BobguiCssValue        *value,
                                                       guint                     n);

G_END_DECLS

