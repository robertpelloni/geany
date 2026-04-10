/*
 * Copyright © 2021 Benjamin Otte
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

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <bobgui/bobguitypes.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_SYMBOLIC_PAINTABLE       (bobgui_symbolic_paintable_get_type ())

GDK_AVAILABLE_IN_4_6
G_DECLARE_INTERFACE (BobguiSymbolicPaintable, bobgui_symbolic_paintable, BOBGUI, SYMBOLIC_PAINTABLE, GdkPaintable)

/**
 * BobguiSymbolicPaintableInterface:
 * @snapshot_symbolic: Snapshot the paintable using the given colors.
 *   See `BobguiSymbolicPaintable::snapshot_symbolic()` for details.
 *   If this function is not implemented, [vfunc@Gdk.Paintable.snapshot]
 *   will be called.
 * @snapshot_with_weight: Like @snapshot_symbolic, but additionally takes
 *   a font weight argument. Since: 4.22
 *
 * The list of virtual functions for the `BobguiSymbolicPaintable` interface.
 * No function must be implemented, default implementations exist for each one.
 */
struct _BobguiSymbolicPaintableInterface
{
  /*< private >*/
  GTypeInterface g_iface;

  /*< public >*/
  void                  (* snapshot_symbolic)                           (BobguiSymbolicPaintable   *paintable,
                                                                         GdkSnapshot            *snapshot,
                                                                         double                  width,
                                                                         double                  height,
                                                                         const GdkRGBA          *colors,
                                                                         gsize                   n_colors);

  void                  (* snapshot_with_weight)                        (BobguiSymbolicPaintable   *paintable,
                                                                         GdkSnapshot            *snapshot,
                                                                         double                  width,
                                                                         double                  height,
                                                                         const GdkRGBA          *colors,
                                                                         gsize                   n_colors,
                                                                         double                  weight);
};
GDK_AVAILABLE_IN_4_6
void                    bobgui_symbolic_paintable_snapshot_symbolic        (BobguiSymbolicPaintable   *paintable,
                                                                         GdkSnapshot            *snapshot,
                                                                         double                  width,
                                                                         double                  height,
                                                                         const GdkRGBA          *colors,
                                                                         gsize                   n_colors);

GDK_AVAILABLE_IN_4_22
void                    bobgui_symbolic_paintable_snapshot_with_weight     (BobguiSymbolicPaintable   *paintable,
                                                                         GdkSnapshot            *snapshot,
                                                                         double                  width,
                                                                         double                  height,
                                                                         const GdkRGBA          *colors,
                                                                         gsize                   n_colors,
                                                                         double                  weight);

G_END_DECLS


