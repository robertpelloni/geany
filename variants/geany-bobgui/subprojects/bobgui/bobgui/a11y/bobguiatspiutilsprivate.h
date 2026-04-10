/* bobguiatspiutilsprivate.h: Shared utilities for AT-SPI
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

#include "bobguiatspiprivate.h"
#include "bobguiatcontextprivate.h"

G_BEGIN_DECLS

AtspiRole
bobgui_atspi_role_for_context (BobguiATContext *context);

GVariant *
bobgui_at_spi_null_ref (void);

void
bobgui_at_spi_emit_children_changed (GDBusConnection         *connection,
                                  const char              *path,
                                  BobguiAccessibleChildState  state,
                                  int                      idx,
                                  GVariant                *child_ref);

void
bobgui_at_spi_translate_coordinates_to_accessible (BobguiAccessible  *accessible,
                                                AtspiCoordType  coordtype,
                                                int             xi,
                                                int             yi,
                                                int            *xo,
                                                int            *yo);


void
bobgui_at_spi_translate_coordinates_from_accessible (BobguiAccessible  *accessible,
                                                  AtspiCoordType  coordtype,
                                                  int             xi,
                                                  int             yi,
                                                  int            *xo,
                                                  int            *yo);

G_END_DECLS
