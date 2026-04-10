/* BOBGUI - The Bobgui Framework
 * Copyright (c) 2024 Arjan Molenaar <amolenaar@gnome.org>
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
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once


#include <bobgui/bobguiwidget.h>


G_BEGIN_DECLS

#define BOBGUI_TYPE_WINDOW_BUTTONS_QUARTZ                  (bobgui_window_buttons_quartz_get_type ())
#define BOBGUI_WINDOW_BUTTONS_QUARTZ(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_WINDOW_BUTTONS_QUARTZ, BobguiWindowButtonsQuartz))
#define BOBGUI_IS_WINDOW_BUTTONS_QUARTZ(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_WINDOW_BUTTONS_QUARTZ))

typedef struct _BobguiWindowButtonsQuartz              BobguiWindowButtonsQuartz;

GDK_AVAILABLE_IN_ALL
GType      bobgui_window_buttons_quartz_get_type (void) G_GNUC_CONST;

G_END_DECLS

