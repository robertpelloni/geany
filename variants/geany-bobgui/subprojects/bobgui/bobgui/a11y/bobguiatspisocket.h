/* bobguiatspisocket.h: AT-SPI socket accessible object
 *
 * Copyright 2024 GNOME Foundation Inc.
 *           2024 Igalia S.L.
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
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
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#if !defined (__BOBGUIATSPI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/a11y/bobguiatspi.h> can be included directly."
#endif

#include <bobgui/bobguiaccessible.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_AT_SPI_SOCKET (bobgui_at_spi_socket_get_type())

GDK_AVAILABLE_IN_4_14
G_DECLARE_FINAL_TYPE (BobguiAtSpiSocket, bobgui_at_spi_socket, BOBGUI, AT_SPI_SOCKET, GObject)

GDK_AVAILABLE_IN_4_14
BobguiAccessible * bobgui_at_spi_socket_new (const char  *bus_name,
                                       const char  *object_path,
                                       GError     **error);

GDK_AVAILABLE_IN_4_14
const char * bobgui_at_spi_socket_get_bus_name (BobguiAtSpiSocket *self);

GDK_AVAILABLE_IN_4_14
const char * bobgui_at_spi_socket_get_object_path (BobguiAtSpiSocket *self);

G_END_DECLS
