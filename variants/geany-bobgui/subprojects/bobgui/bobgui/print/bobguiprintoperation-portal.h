/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2016, Red Hat, Inc.
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

#include "bobguiprintoperation.h"

G_BEGIN_DECLS

BobguiPrintOperationResult bobgui_print_operation_portal_run_dialog             (BobguiPrintOperation           *op,
                                                                           gboolean                     show_dialog,
                                                                           BobguiWindow                   *parent,
                                                                           gboolean                    *do_print);
void                    bobgui_print_operation_portal_run_dialog_async       (BobguiPrintOperation           *op,
                                                                           gboolean                     show_dialog,
                                                                           BobguiWindow                   *parent,
                                                                           BobguiPrintOperationPrintFunc   print_cb);
void                    bobgui_print_operation_portal_launch_preview         (BobguiPrintOperation           *op,
                                                                           cairo_surface_t             *surface,
                                                                           BobguiWindow                   *parent,
                                                                           const char                  *filename);

G_END_DECLS

