/* BOBGUI - The Bobgui Framework
 * bobguiprintbackendcups.h: Default implementation of BobguiPrintBackend for the Common Unix Print System (CUPS)
 * Copyright (C) 2006, 2007 Red Hat, Inc.
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

#define BOBGUI_TYPE_PRINT_BACKEND_CUPS             (bobgui_print_backend_cups_get_type ())
#define BOBGUI_PRINT_BACKEND_CUPS(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_PRINT_BACKEND_CUPS, BobguiPrintBackendCups))
#define BOBGUI_IS_PRINT_BACKEND_CUPS(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_PRINT_BACKEND_CUPS))

typedef struct _BobguiPrintBackendCups      BobguiPrintBackendCups;

BobguiPrintBackend *bobgui_print_backend_cups_new      (void);
GType          bobgui_print_backend_cups_get_type (void) G_GNUC_CONST;

G_END_DECLS
