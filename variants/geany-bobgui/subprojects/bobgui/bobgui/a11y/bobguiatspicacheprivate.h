/* bobguiatspicacheprivate.h: AT-SPI object cache
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

#include <gio/gio.h>
#include "bobguiatspiprivate.h"

G_BEGIN_DECLS

#define BOBGUI_TYPE_AT_SPI_CACHE (bobgui_at_spi_cache_get_type())

G_DECLARE_FINAL_TYPE (BobguiAtSpiCache, bobgui_at_spi_cache, BOBGUI, AT_SPI_CACHE, GObject)

BobguiAtSpiCache *
bobgui_at_spi_cache_new (GDBusConnection *connection,
                      const char      *cache_path,
                      BobguiAtSpiRoot    *root);

void
bobgui_at_spi_cache_add_context (BobguiAtSpiCache *self,
                              BobguiAtSpiContext *context);

void
bobgui_at_spi_cache_remove_context (BobguiAtSpiCache *self,
                                 BobguiAtSpiContext *context);

G_END_DECLS
