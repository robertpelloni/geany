/* bobguiatspicontextprivate.h: AT-SPI BobguiATContext implementation
 *
 * Copyright 2020  GNOME Foundation
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

#include "bobguiatcontextprivate.h"
#include "bobguiatspiprivate.h"

G_BEGIN_DECLS

#define BOBGUI_TYPE_AT_SPI_CONTEXT (bobgui_at_spi_context_get_type())

G_DECLARE_FINAL_TYPE (BobguiAtSpiContext, bobgui_at_spi_context, BOBGUI, AT_SPI_CONTEXT, BobguiATContext)

BobguiATContext *
bobgui_at_spi_create_context (BobguiAccessibleRole  accessible_role,
                           BobguiAccessible     *accessible,
                           GdkDisplay        *display);

const char *
bobgui_at_spi_context_get_context_path (BobguiAtSpiContext *self);

GVariant *
bobgui_at_spi_context_to_ref (BobguiAtSpiContext *self);

BobguiAtSpiRoot *
bobgui_at_spi_context_get_root (BobguiAtSpiContext *self);

GVariant *
bobgui_at_spi_context_get_parent_ref (BobguiAtSpiContext *self);

GVariant *
bobgui_at_spi_context_get_interfaces (BobguiAtSpiContext *self);

GVariant *
bobgui_at_spi_context_get_states (BobguiAtSpiContext *self);

int
bobgui_at_spi_context_get_index_in_parent (BobguiAtSpiContext *self);

int
bobgui_at_spi_context_get_child_count (BobguiAtSpiContext *self);

G_END_DECLS
