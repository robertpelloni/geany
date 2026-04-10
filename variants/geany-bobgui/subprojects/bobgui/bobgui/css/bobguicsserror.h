/*
 * Copyright © 2019 Benjamin Otte
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

#include <glib.h>
#include <gdk/version/gdkversionmacros.h>

G_BEGIN_DECLS

/**
 * BOBGUI_CSS_PARSER_ERROR:
 *
 * Domain for `BobguiCssParser` errors.
 */
#define BOBGUI_CSS_PARSER_ERROR (bobgui_css_parser_error_quark ())

GDK_AVAILABLE_IN_ALL
GQuark bobgui_css_parser_error_quark (void);

/**
 * BOBGUI_CSS_PARSER_WARNING:
 *
 * Domain for `BobguiCssParser` warnings.
 */
#define BOBGUI_CSS_PARSER_WARNING (bobgui_css_parser_warning_quark ())

GDK_AVAILABLE_IN_ALL
GQuark bobgui_css_parser_warning_quark (void);

G_END_DECLS

