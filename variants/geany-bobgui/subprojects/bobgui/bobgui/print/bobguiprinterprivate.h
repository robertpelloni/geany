/* BOBGUI - The Bobgui Framework
 * bobguiprintoperation.h: Print Operation
 * Copyright (C) 2006, Red Hat, Inc.
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

#include <bobgui/bobgui.h>
#include "bobguiunixprint.h"
#include "bobguiprinteroptionsetprivate.h"

G_BEGIN_DECLS

#define BOBGUI_PRINTER_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), BOBGUI_TYPE_PRINTER, BobguiPrinterClass))
#define BOBGUI_IS_PRINTER_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), BOBGUI_TYPE_PRINTER))
#define BOBGUI_PRINTER_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_PRINTER, BobguiPrinterClass))

typedef struct _BobguiPrinterClass     BobguiPrinterClass;
typedef struct _BobguiPrinterPrivate   BobguiPrinterPrivate;

struct _BobguiPrinter
{
  GObject parent_instance;
};

struct _BobguiPrinterClass
{
  GObjectClass parent_class;

  void (*details_acquired) (BobguiPrinter *printer,
                            gboolean    success);
};

BobguiPrinterOptionSet *_bobgui_printer_get_options               (BobguiPrinter          *printer,
							     BobguiPrintSettings    *settings,
							     BobguiPageSetup        *page_setup,
							     BobguiPrintCapabilities capabilities);
gboolean             _bobgui_printer_mark_conflicts            (BobguiPrinter          *printer,
							     BobguiPrinterOptionSet *options);
void                 _bobgui_printer_get_settings_from_options (BobguiPrinter          *printer,
							     BobguiPrinterOptionSet *options,
							     BobguiPrintSettings    *settings);
void                 _bobgui_printer_prepare_for_print         (BobguiPrinter          *printer,
							     BobguiPrintJob         *print_job,
							     BobguiPrintSettings    *settings,
							     BobguiPageSetup        *page_setup);
cairo_surface_t *    _bobgui_printer_create_cairo_surface      (BobguiPrinter          *printer,
							     BobguiPrintSettings    *settings,
							     double               width,
							     double               height,
							     GIOChannel          *cache_io);
GHashTable *         _bobgui_printer_get_custom_widgets        (BobguiPrinter          *printer);

/* BobguiPrintJob private methods: */
GDK_AVAILABLE_IN_ALL
void bobgui_print_job_set_status (BobguiPrintJob   *job,
			       BobguiPrintStatus status);

BobguiPrinter *         bobgui_printer_find (const char *name);

G_END_DECLS
