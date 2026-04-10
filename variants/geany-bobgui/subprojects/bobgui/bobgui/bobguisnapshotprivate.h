/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2016 Benjamin Otte <otte@gnome.org>
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

#include "bobguisnapshot.h"

#include "gdk/gdksubsurfaceprivate.h"
#include "gsk/gskrendernodeprivate.h"
/* for GskRepeat */
#include "gsk/gskgradientprivate.h"
#include "gsk/gskshadownodeprivate.h"

G_BEGIN_DECLS

void                    bobgui_snapshot_append_text                (BobguiSnapshot            *snapshot,
                                                                 PangoFont              *font,
                                                                 PangoGlyphString       *glyphs,
                                                                 const GdkRGBA          *color,
                                                                 float                   x,
                                                                 float                   y);
void                    bobgui_snapshot_add_text                   (BobguiSnapshot            *snapshot,
                                                                 PangoFont              *font,
                                                                 PangoGlyphString       *glyphs,
                                                                 const GdkColor         *color,
                                                                 float                   x,
                                                                 float                   y);

void                    bobgui_snapshot_add_layout                 (BobguiSnapshot            *snapshot,
                                                                 PangoLayout            *layout,
                                                                 const GdkColor         *color);

void                    bobgui_snapshot_push_collect               (BobguiSnapshot            *snapshot);
GskRenderNode *         bobgui_snapshot_pop_collect                (BobguiSnapshot            *snapshot);

void                    bobgui_snapshot_push_subsurface            (BobguiSnapshot            *snapshot,
                                                                 GdkSubsurface          *subsurface);
void                    bobgui_snapshot_push_repeat2               (BobguiSnapshot            *snapshot,
                                                                 const graphene_rect_t  *bounds,
                                                                 const graphene_rect_t  *child_bounds,
                                                                 GskRepeat               repeat);

void                    bobgui_snapshot_add_color                  (BobguiSnapshot            *snapshot,
                                                                 const GdkColor         *color,
                                                                 const graphene_rect_t  *bounds);
void                    bobgui_snapshot_add_border                 (BobguiSnapshot            *snapshot,
                                                                 const GskRoundedRect   *outline,
                                                                 const float             border_width[4],
                                                                 const GdkColor          border_color[4]);

void                    bobgui_snapshot_add_inset_shadow           (BobguiSnapshot            *snapshot,
                                                                 const GskRoundedRect   *outline,
                                                                 const GdkColor         *color,
                                                                 const graphene_point_t *offset,
                                                                 float                   spread,
                                                                 float                   blur_radius);

void                    bobgui_snapshot_add_outset_shadow          (BobguiSnapshot            *snapshot,
                                                                 const GskRoundedRect   *outline,
                                                                 const GdkColor         *color,
                                                                 const graphene_point_t *offset,
                                                                 float                   spread,
                                                                 float                   blur_radius);

void                    bobgui_snapshot_push_shadows               (BobguiSnapshot          *snapshot,
                                                                 const GskShadowEntry *shadow,
                                                                 gsize                 n_shadows);

void                    bobgui_snapshot_add_linear_gradient        (BobguiSnapshot             *snapshot,
                                                                 const graphene_rect_t   *bounds,
                                                                 const graphene_point_t  *start_point,
                                                                 const graphene_point_t  *end_point,
                                                                 const GskGradient       *gradient);
void                    bobgui_snapshot_add_radial_gradient        (BobguiSnapshot             *snapshot,
                                                                 const graphene_rect_t   *bounds,
                                                                 const graphene_point_t  *start_center,
                                                                 float                    start_radius,
                                                                 const graphene_point_t  *end_center,
                                                                 float                    end_radius,
                                                                 float                    aspect_ratio,
                                                                 const GskGradient       *gradient);

void                    bobgui_snapshot_add_conic_gradient         (BobguiSnapshot             *snapshot,
                                                                 const graphene_rect_t   *bounds,
                                                                 const graphene_point_t  *center,
                                                                 float                    rotation,
                                                                 const GskGradient       *gradient);
void                    bobgui_snapshot_append_node_scaled         (BobguiSnapshot             *snapshot,
                                                                 GskRenderNode           *node,
                                                                 graphene_rect_t         *from,
                                                                 graphene_rect_t         *to);


G_END_DECLS
