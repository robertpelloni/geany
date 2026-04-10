/* bobguiatspitextprivate.h: AT-SPI Text implementation
 *
 * Copyright 2020 Red Hat, Inc.
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
#include "bobguiaccessible.h"

G_BEGIN_DECLS

const GDBusInterfaceVTable *bobgui_atspi_get_text_vtable (BobguiAccessible *accessible);

typedef void (BobguiAtspiTextChangedCallback) (gpointer    data,
                                            const char *kind,
                                            int         start,
                                            int         end,
                                            const char *text);
typedef void (BobguiAtspiTextSelectionCallback) (gpointer    data,
                                              const char *kind,
                                              int         position);

void bobgui_atspi_connect_text_signals    (BobguiAccessible *accessible,
                                        BobguiAtspiTextChangedCallback text_changed,
                                        BobguiAtspiTextSelectionCallback selection_changed,
                                        gpointer   data);
void bobgui_atspi_disconnect_text_signals (BobguiAccessible *accessible);

G_END_DECLS
