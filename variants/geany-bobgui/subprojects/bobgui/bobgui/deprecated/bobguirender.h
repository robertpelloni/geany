/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2010 Carlos Garnacho <carlosg@gnome.org>
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

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <cairo.h>
#include <pango/pango.h>
#include <gdk/gdk.h>

#include <bobgui/bobguienums.h>
#include <bobgui/bobguitypes.h>
#include <bobgui/bobguisnapshot.h>

G_BEGIN_DECLS

GDK_DEPRECATED_IN_4_10
void        bobgui_render_check       (BobguiStyleContext     *context,
                                    cairo_t             *cr,
                                    double               x,
                                    double               y,
                                    double               width,
                                    double               height);
GDK_DEPRECATED_IN_4_10
void        bobgui_render_option      (BobguiStyleContext     *context,
                                    cairo_t             *cr,
                                    double               x,
                                    double               y,
                                    double               width,
                                    double               height);
GDK_DEPRECATED_IN_4_10
void        bobgui_render_arrow       (BobguiStyleContext     *context,
                                    cairo_t             *cr,
                                    double               angle,
                                    double               x,
                                    double               y,
                                    double               size);
GDK_DEPRECATED_IN_4_10
void        bobgui_render_background  (BobguiStyleContext     *context,
                                    cairo_t             *cr,
                                    double               x,
                                    double               y,
                                    double               width,
                                    double               height);

GDK_DEPRECATED_IN_4_10
void        bobgui_render_frame       (BobguiStyleContext     *context,
                                    cairo_t             *cr,
                                    double               x,
                                    double               y,
                                    double               width,
                                    double               height);
GDK_DEPRECATED_IN_4_10
void        bobgui_render_expander    (BobguiStyleContext     *context,
                                    cairo_t             *cr,
                                    double               x,
                                    double               y,
                                    double               width,
                                    double               height);
GDK_DEPRECATED_IN_4_10
void        bobgui_render_focus       (BobguiStyleContext     *context,
                                    cairo_t             *cr,
                                    double               x,
                                    double               y,
                                    double               width,
                                    double               height);
GDK_DEPRECATED_IN_4_10
void        bobgui_render_layout      (BobguiStyleContext     *context,
                                    cairo_t             *cr,
                                    double               x,
                                    double               y,
                                    PangoLayout         *layout);
GDK_DEPRECATED_IN_4_10
void        bobgui_render_line        (BobguiStyleContext     *context,
                                    cairo_t             *cr,
                                    double               x0,
                                    double               y0,
                                    double               x1,
                                    double               y1);
GDK_DEPRECATED_IN_4_10
void        bobgui_render_handle      (BobguiStyleContext     *context,
                                    cairo_t             *cr,
                                    double               x,
                                    double               y,
                                    double               width,
                                    double               height);
GDK_DEPRECATED_IN_4_10
void        bobgui_render_activity    (BobguiStyleContext     *context,
                                    cairo_t             *cr,
                                    double               x,
                                    double               y,
                                    double               width,
                                    double               height);
GDK_DEPRECATED_IN_4_10
void        bobgui_render_icon        (BobguiStyleContext     *context,
                                    cairo_t             *cr,
                                    GdkTexture          *texture,
                                    double               x,
                                    double               y);

GDK_DEPRECATED_IN_4_10
void            bobgui_snapshot_render_background          (BobguiSnapshot            *snapshot,
                                                         BobguiStyleContext        *context,
                                                         double                  x,
                                                         double                  y,
                                                         double                  width,
                                                         double                  height);
GDK_DEPRECATED_IN_4_10
void            bobgui_snapshot_render_frame               (BobguiSnapshot            *snapshot,
                                                         BobguiStyleContext        *context,
                                                         double                  x,
                                                         double                  y,
                                                         double                  width,
                                                         double                  height);
GDK_DEPRECATED_IN_4_10
void            bobgui_snapshot_render_focus               (BobguiSnapshot            *snapshot,
                                                         BobguiStyleContext        *context,
                                                         double                  x,
                                                         double                  y,
                                                         double                  width,
                                                         double                  height);
GDK_DEPRECATED_IN_4_10
void            bobgui_snapshot_render_layout              (BobguiSnapshot            *snapshot,
                                                         BobguiStyleContext        *context,
                                                         double                  x,
                                                         double                  y,
                                                         PangoLayout            *layout);
GDK_DEPRECATED_IN_4_10
void            bobgui_snapshot_render_insertion_cursor    (BobguiSnapshot            *snapshot,
                                                         BobguiStyleContext        *context,
                                                         double                  x,
                                                         double                  y,
                                                         PangoLayout            *layout,
                                                         int                     index,
                                                         PangoDirection          direction);

G_END_DECLS

