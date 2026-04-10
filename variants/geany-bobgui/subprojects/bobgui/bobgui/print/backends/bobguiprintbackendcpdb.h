/* BOBGUI - The Bobgui Framework
 * bobguiprintbackendcpdb.h: Default implementation of BobguiPrintBackend
 * for the Common Print Dialog Backends (CPDB)
 * Copyright (C) 2022, 2023 TinyTrebuchet <tinytrebuchet@protonmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <glib-object.h>
#include <bobgui/bobgui.h>
#include "bobgui/print/bobguiprintbackendprivate.h"

G_BEGIN_DECLS

#define BOBGUI_TYPE_PRINT_BACKEND_CPDB             (bobgui_print_backend_cpdb_get_type ())
#define BOBGUI_PRINT_BACKEND_CPDB(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_PRINT_BACKEND_CPDB, BobguiPrintBackendCpdb))
#define BOBGUI_IS_PRINT_BACKEND_CPDB(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_PRINT_BACKEND_CPDB))

typedef struct _BobguiPrintBackendCpdbClass BobguiPrintBackendCpdbClass;
typedef struct _BobguiPrintBackendCpdb      BobguiPrintBackendCpdb;

BobguiPrintBackend  *bobgui_print_backend_cpdb_new        (void);
GType             bobgui_print_backend_cpdb_get_type   (void) G_GNUC_CONST;

G_END_DECLS
