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

/*
 * Modified by the BOBGUI Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the BOBGUI Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BOBGUI at ftp://ftp.bobgui.org/pub/bobgui/.
 */

#pragma once


#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <gsk/gsk.h>

#include <bobgui/bobguitypes.h>

G_BEGIN_DECLS

typedef struct _BobguiSnapshotClass       BobguiSnapshotClass;

#define BOBGUI_TYPE_SNAPSHOT               (bobgui_snapshot_get_type ())

#define BOBGUI_SNAPSHOT(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_SNAPSHOT, BobguiSnapshot))
#define BOBGUI_IS_SNAPSHOT(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_SNAPSHOT))

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiSnapshot, g_object_unref)



GDK_AVAILABLE_IN_ALL
GType           bobgui_snapshot_get_type                   (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
BobguiSnapshot *   bobgui_snapshot_new                        (void);
GDK_AVAILABLE_IN_ALL
GskRenderNode * bobgui_snapshot_free_to_node               (BobguiSnapshot            *snapshot);
GDK_AVAILABLE_IN_ALL
GdkPaintable *  bobgui_snapshot_free_to_paintable          (BobguiSnapshot            *snapshot,
                                                         const graphene_size_t  *size);

GDK_AVAILABLE_IN_ALL
GskRenderNode * bobgui_snapshot_to_node                    (BobguiSnapshot            *snapshot);
GDK_AVAILABLE_IN_ALL
GdkPaintable *  bobgui_snapshot_to_paintable               (BobguiSnapshot            *snapshot,
                                                         const graphene_size_t  *size);

GDK_AVAILABLE_IN_ALL
void            bobgui_snapshot_push_debug                 (BobguiSnapshot            *snapshot,
                                                         const char             *message,
                                                         ...) G_GNUC_PRINTF (2, 3);
GDK_AVAILABLE_IN_ALL
void            bobgui_snapshot_push_opacity               (BobguiSnapshot            *snapshot,
                                                         double                  opacity);
GDK_AVAILABLE_IN_4_22
void            bobgui_snapshot_push_isolation             (BobguiSnapshot            *snapshot,
                                                         GskIsolation            features);
GDK_AVAILABLE_IN_ALL
void            bobgui_snapshot_push_blur                  (BobguiSnapshot            *snapshot,
                                                         double                  radius);
GDK_AVAILABLE_IN_ALL
void            bobgui_snapshot_push_color_matrix          (BobguiSnapshot            *snapshot,
                                                         const graphene_matrix_t*color_matrix,
                                                         const graphene_vec4_t  *color_offset);

GDK_AVAILABLE_IN_4_20
void            bobgui_snapshot_push_component_transfer   (BobguiSnapshot                *snapshot,
                                                        const GskComponentTransfer *red,
                                                        const GskComponentTransfer *green,
                                                        const GskComponentTransfer *blue,
                                                        const GskComponentTransfer *alpha);

GDK_AVAILABLE_IN_ALL
void            bobgui_snapshot_push_repeat                (BobguiSnapshot            *snapshot,
                                                         const graphene_rect_t  *bounds,
                                                         const graphene_rect_t  *child_bounds);
GDK_AVAILABLE_IN_ALL
void            bobgui_snapshot_push_clip                  (BobguiSnapshot            *snapshot,
                                                         const graphene_rect_t  *bounds);
GDK_AVAILABLE_IN_ALL
void            bobgui_snapshot_push_rounded_clip          (BobguiSnapshot            *snapshot,
                                                         const GskRoundedRect   *bounds);
GDK_AVAILABLE_IN_4_14
void            bobgui_snapshot_push_fill                  (BobguiSnapshot            *snapshot,
                                                         GskPath                *path,
                                                         GskFillRule             fill_rule);
GDK_AVAILABLE_IN_4_14
void            bobgui_snapshot_push_stroke                (BobguiSnapshot            *snapshot,
                                                         GskPath                *path,
                                                         const GskStroke        *stroke);
GDK_AVAILABLE_IN_ALL
void            bobgui_snapshot_push_shadow                (BobguiSnapshot            *snapshot,
                                                         const GskShadow        *shadow,
                                                         gsize                   n_shadows);
GDK_AVAILABLE_IN_ALL
void            bobgui_snapshot_push_blend                 (BobguiSnapshot            *snapshot,
                                                         GskBlendMode            blend_mode);
GDK_AVAILABLE_IN_4_10
void            bobgui_snapshot_push_mask                  (BobguiSnapshot            *snapshot,
                                                         GskMaskMode             mask_mode);
GDK_AVAILABLE_IN_4_22
void            bobgui_snapshot_push_copy                  (BobguiSnapshot            *snapshot);
GDK_AVAILABLE_IN_4_22
void            bobgui_snapshot_push_composite             (BobguiSnapshot            *snapshot,
                                                         GskPorterDuff           op);


GDK_AVAILABLE_IN_ALL
void            bobgui_snapshot_push_cross_fade            (BobguiSnapshot            *snapshot,
                                                         double                  progress);

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
GDK_DEPRECATED_IN_4_16_FOR(BobguiGLArea)
void            bobgui_snapshot_push_gl_shader             (BobguiSnapshot            *snapshot,
                                                         GskGLShader            *shader,
                                                         const graphene_rect_t  *bounds,
                                                         GBytes                 *take_args);
GDK_DEPRECATED_IN_4_16_FOR(BobguiGLArea)
void           bobgui_snapshot_gl_shader_pop_texture       (BobguiSnapshot            *snapshot);
G_GNUC_END_IGNORE_DEPRECATIONS
GDK_AVAILABLE_IN_ALL
void            bobgui_snapshot_pop                        (BobguiSnapshot            *snapshot);
GDK_AVAILABLE_IN_ALL
void            bobgui_snapshot_save                       (BobguiSnapshot            *snapshot);
GDK_AVAILABLE_IN_ALL
void            bobgui_snapshot_restore                    (BobguiSnapshot            *snapshot);
GDK_AVAILABLE_IN_ALL
void            bobgui_snapshot_transform                  (BobguiSnapshot            *snapshot,
                                                         GskTransform           *transform);
GDK_AVAILABLE_IN_ALL
void            bobgui_snapshot_transform_matrix           (BobguiSnapshot            *snapshot,
                                                         const graphene_matrix_t*matrix);
GDK_AVAILABLE_IN_ALL
void            bobgui_snapshot_translate                  (BobguiSnapshot            *snapshot,
                                                         const graphene_point_t *point);
GDK_AVAILABLE_IN_ALL
void            bobgui_snapshot_translate_3d               (BobguiSnapshot            *snapshot,
                                                         const graphene_point3d_t*point);
GDK_AVAILABLE_IN_ALL
void            bobgui_snapshot_rotate                     (BobguiSnapshot            *snapshot,
                                                         float                  angle);
GDK_AVAILABLE_IN_ALL
void            bobgui_snapshot_rotate_3d                  (BobguiSnapshot            *snapshot,
                                                         float                   angle,
                                                         const graphene_vec3_t  *axis);
GDK_AVAILABLE_IN_ALL
void            bobgui_snapshot_scale                      (BobguiSnapshot            *snapshot,
                                                         float                   factor_x,
                                                         float                   factor_y);
GDK_AVAILABLE_IN_ALL
void            bobgui_snapshot_scale_3d                   (BobguiSnapshot            *snapshot,
                                                         float                   factor_x,
                                                         float                   factor_y,
                                                         float                   factor_z);
GDK_AVAILABLE_IN_ALL
void            bobgui_snapshot_perspective                (BobguiSnapshot            *snapshot,
                                                         float                   depth);
GDK_AVAILABLE_IN_ALL
void            bobgui_snapshot_append_node                (BobguiSnapshot            *snapshot,
                                                         GskRenderNode          *node);
GDK_AVAILABLE_IN_ALL
cairo_t *       bobgui_snapshot_append_cairo               (BobguiSnapshot            *snapshot,
                                                         const graphene_rect_t  *bounds);
GDK_AVAILABLE_IN_ALL
void            bobgui_snapshot_append_texture             (BobguiSnapshot            *snapshot,
                                                         GdkTexture             *texture,
                                                         const graphene_rect_t  *bounds);
GDK_AVAILABLE_IN_4_10
void            bobgui_snapshot_append_scaled_texture      (BobguiSnapshot            *snapshot,
                                                         GdkTexture             *texture,
                                                         GskScalingFilter        filter,
                                                         const graphene_rect_t  *bounds);
GDK_AVAILABLE_IN_ALL
void            bobgui_snapshot_append_color               (BobguiSnapshot            *snapshot,
                                                         const GdkRGBA          *color,
                                                         const graphene_rect_t  *bounds);
GDK_AVAILABLE_IN_ALL
void            bobgui_snapshot_append_linear_gradient     (BobguiSnapshot            *snapshot,
                                                         const graphene_rect_t  *bounds,
                                                         const graphene_point_t *start_point,
                                                         const graphene_point_t *end_point,
                                                         const GskColorStop     *stops,
                                                         gsize                   n_stops);
GDK_AVAILABLE_IN_ALL
void            bobgui_snapshot_append_repeating_linear_gradient (BobguiSnapshot            *snapshot,
                                                               const graphene_rect_t  *bounds,
                                                               const graphene_point_t *start_point,
                                                               const graphene_point_t *end_point,
                                                               const GskColorStop     *stops,
                                                               gsize                   n_stops);
GDK_AVAILABLE_IN_ALL
void            bobgui_snapshot_append_radial_gradient     (BobguiSnapshot            *snapshot,
                                                         const graphene_rect_t  *bounds,
                                                         const graphene_point_t *center,
                                                         float                   hradius,
                                                         float                   vradius,
                                                         float                   start,
                                                         float                   end,
                                                         const GskColorStop     *stops,
                                                         gsize                   n_stops);
GDK_AVAILABLE_IN_ALL
void            bobgui_snapshot_append_repeating_radial_gradient (BobguiSnapshot            *snapshot,
                                                               const graphene_rect_t  *bounds,
                                                               const graphene_point_t *center,
                                                               float                   hradius,
                                                               float                   vradius,
                                                               float                   start,
                                                               float                   end,
                                                               const GskColorStop     *stops,
                                                               gsize                   n_stops);
GDK_AVAILABLE_IN_ALL
void            bobgui_snapshot_append_conic_gradient      (BobguiSnapshot            *snapshot,
                                                         const graphene_rect_t  *bounds,
                                                         const graphene_point_t *center,
                                                         float                   rotation,
                                                         const GskColorStop     *stops,
                                                         gsize                   n_stops);
GDK_AVAILABLE_IN_ALL
void            bobgui_snapshot_append_border              (BobguiSnapshot            *snapshot,
                                                         const GskRoundedRect   *outline,
                                                         const float             border_width[4],
                                                         const GdkRGBA           border_color[4]);
GDK_AVAILABLE_IN_ALL
void            bobgui_snapshot_append_inset_shadow        (BobguiSnapshot            *snapshot,
                                                         const GskRoundedRect   *outline,
                                                         const GdkRGBA          *color,
                                                         float                   dx,
                                                         float                   dy,
                                                         float                   spread,
                                                         float                   blur_radius);
GDK_AVAILABLE_IN_ALL
void            bobgui_snapshot_append_outset_shadow       (BobguiSnapshot            *snapshot,
                                                         const GskRoundedRect   *outline,
                                                         const GdkRGBA          *color,
                                                         float                   dx,
                                                         float                   dy,
                                                         float                   spread,
                                                         float                   blur_radius);
/* next function implemented in gskpango.c */
GDK_AVAILABLE_IN_ALL
void            bobgui_snapshot_append_layout              (BobguiSnapshot            *snapshot,
                                                         PangoLayout            *layout,
                                                         const GdkRGBA          *color);

GDK_AVAILABLE_IN_4_14
void            bobgui_snapshot_append_fill                (BobguiSnapshot            *snapshot,
                                                         GskPath                *path,
                                                         GskFillRule             fill_rule,
                                                         const GdkRGBA          *color);
GDK_AVAILABLE_IN_4_14
void            bobgui_snapshot_append_stroke              (BobguiSnapshot            *snapshot,
                                                         GskPath                *path,
                                                         const GskStroke        *stroke,
                                                         const GdkRGBA          *color);
GDK_AVAILABLE_IN_4_22
void            bobgui_snapshot_append_paste               (BobguiSnapshot            *snapshot,
                                                         const graphene_rect_t  *bounds,
                                                         gsize                   nth);

G_END_DECLS

