/* BOBGUI - The Bobgui Framework
 * bobguiprintbackendpdf.h: Default implementation of BobguiPrintBackend
 * for printing to a file
 * Copyright (C) 2003, Red Hat, Inc.
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
#include "bobgui/print/bobguiprintbackendprivate.h"

G_BEGIN_DECLS

#define BOBGUI_TYPE_PRINT_BACKEND_FILE    (bobgui_print_backend_file_get_type ())
#define BOBGUI_PRINT_BACKEND_FILE(obj)    (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_PRINT_BACKEND_FILE, BobguiPrintBackendFile))
#define BOBGUI_IS_PRINT_BACKEND_FILE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_PRINT_BACKEND_FILE))

typedef struct _BobguiPrintBackendFile    BobguiPrintBackendFile;

BobguiPrintBackend *bobgui_print_backend_file_new      (void);
GType            bobgui_print_backend_file_get_type (void) G_GNUC_CONST;

G_END_DECLS
