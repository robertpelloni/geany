/*
 * Copyright © 2018 Benjamin Otte
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

#include <gdk/gdk.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_SCALER (bobgui_scaler_get_type ())

G_DECLARE_FINAL_TYPE (BobguiScaler, bobgui_scaler, BOBGUI, SCALER, GObject)

GdkPaintable *  bobgui_scaler_new                  (GdkPaintable   *paintable,
                                                 double          scale);

G_END_DECLS

