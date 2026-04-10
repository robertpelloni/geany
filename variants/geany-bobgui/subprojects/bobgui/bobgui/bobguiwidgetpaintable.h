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

#include <bobgui/bobguitypes.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_WIDGET_PAINTABLE (bobgui_widget_paintable_get_type ())

GDK_AVAILABLE_IN_ALL
G_DECLARE_FINAL_TYPE (BobguiWidgetPaintable, bobgui_widget_paintable, BOBGUI, WIDGET_PAINTABLE, GObject)

GDK_AVAILABLE_IN_ALL
GdkPaintable *  bobgui_widget_paintable_new                (BobguiWidget              *widget);

GDK_AVAILABLE_IN_ALL
BobguiWidget *     bobgui_widget_paintable_get_widget         (BobguiWidgetPaintable     *self);
GDK_AVAILABLE_IN_ALL
void            bobgui_widget_paintable_set_widget         (BobguiWidgetPaintable     *self,
                                                         BobguiWidget              *widget);

G_END_DECLS

