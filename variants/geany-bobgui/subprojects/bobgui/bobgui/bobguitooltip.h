/* bobguitooltip.h
 *
 * Copyright (C) 2006-2007 Imendio AB
 * Contact: Kristian Rietveld <kris@imendio.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <bobgui/bobguiwindow.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_TOOLTIP                 (bobgui_tooltip_get_type ())
#define BOBGUI_TOOLTIP(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_TOOLTIP, BobguiTooltip))
#define BOBGUI_IS_TOOLTIP(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_TOOLTIP))

GDK_AVAILABLE_IN_ALL
GType bobgui_tooltip_get_type (void);

GDK_AVAILABLE_IN_ALL
void bobgui_tooltip_set_markup              (BobguiTooltip         *tooltip,
                                          const char         *markup);
GDK_AVAILABLE_IN_ALL
void bobgui_tooltip_set_text                (BobguiTooltip         *tooltip,
                                          const char         *text);
GDK_AVAILABLE_IN_ALL
void bobgui_tooltip_set_icon                (BobguiTooltip         *tooltip,
                                          GdkPaintable       *paintable);
GDK_AVAILABLE_IN_ALL
void bobgui_tooltip_set_icon_from_icon_name (BobguiTooltip         *tooltip,
				          const char         *icon_name);
GDK_AVAILABLE_IN_ALL
void bobgui_tooltip_set_icon_from_gicon     (BobguiTooltip         *tooltip,
					  GIcon              *gicon);
GDK_AVAILABLE_IN_ALL
void bobgui_tooltip_set_custom	         (BobguiTooltip         *tooltip,
                                          BobguiWidget          *custom_widget);

GDK_AVAILABLE_IN_ALL
void bobgui_tooltip_set_tip_area            (BobguiTooltip         *tooltip,
                                          const GdkRectangle *rect);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiTooltip, g_object_unref)

G_END_DECLS

