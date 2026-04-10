/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2023 Benjamin Otte
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


#include <bobgui/bobguiscrollinfo.h>

G_BEGIN_DECLS

void                    bobgui_scroll_info_compute_scroll          (BobguiScrollInfo                  *self,
                                                                 const cairo_rectangle_int_t    *area,
                                                                 const cairo_rectangle_int_t    *viewport,
                                                                 int                            *out_x,
                                                                 int                            *out_y);
int                     bobgui_scroll_info_compute_for_orientation (BobguiScrollInfo                  *self,
                                                                 BobguiOrientation                  orientation,
                                                                 int                             area_origin,
                                                                 int                             area_size,
                                                                 int                             viewport_origin,
                                                                 int                             viewport_size);

G_END_DECLS

