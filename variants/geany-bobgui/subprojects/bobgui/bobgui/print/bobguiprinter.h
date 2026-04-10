/* BobguiPrinter
 * Copyright (C) 2006 John (J5) Palmieri <johnp@redhat.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#if !defined (__BOBGUI_UNIX_PRINT_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobguiunixprint.h> can be included directly."
#endif

#include <cairo.h>
#include <bobgui/bobgui.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_PRINT_CAPABILITIES (bobgui_print_capabilities_get_type ())

/* Note, this type is manually registered with GObject in bobguiprinter.c
 * If you add any flags, update the registration as well!
 */
/**
 * BobguiPrintCapabilities:
 * @BOBGUI_PRINT_CAPABILITY_PAGE_SET: Print dialog will offer printing even/odd pages.
 * @BOBGUI_PRINT_CAPABILITY_COPIES: Print dialog will allow to print multiple copies.
 * @BOBGUI_PRINT_CAPABILITY_COLLATE: Print dialog will allow to collate multiple copies.
 * @BOBGUI_PRINT_CAPABILITY_REVERSE: Print dialog will allow to print pages in reverse order.
 * @BOBGUI_PRINT_CAPABILITY_SCALE: Print dialog will allow to scale the output.
 * @BOBGUI_PRINT_CAPABILITY_GENERATE_PDF: The program will send the document to
 *   the printer in PDF format
 * @BOBGUI_PRINT_CAPABILITY_GENERATE_PS: The program will send the document to
 *   the printer in Postscript format
 * @BOBGUI_PRINT_CAPABILITY_PREVIEW: Print dialog will offer a preview
 * @BOBGUI_PRINT_CAPABILITY_NUMBER_UP: Print dialog will offer printing multiple
 *   pages per sheet
 * @BOBGUI_PRINT_CAPABILITY_NUMBER_UP_LAYOUT: Print dialog will allow to rearrange
 *   pages when printing multiple pages per sheet
 *
 * Specifies which features the print dialog should offer.
 *
 * If neither %BOBGUI_PRINT_CAPABILITY_GENERATE_PDF nor
 * %BOBGUI_PRINT_CAPABILITY_GENERATE_PS is specified, BOBGUI assumes that all
 * formats are supported.
 */
typedef enum
{
  BOBGUI_PRINT_CAPABILITY_PAGE_SET         = 1 << 0,
  BOBGUI_PRINT_CAPABILITY_COPIES           = 1 << 1,
  BOBGUI_PRINT_CAPABILITY_COLLATE          = 1 << 2,
  BOBGUI_PRINT_CAPABILITY_REVERSE          = 1 << 3,
  BOBGUI_PRINT_CAPABILITY_SCALE            = 1 << 4,
  BOBGUI_PRINT_CAPABILITY_GENERATE_PDF     = 1 << 5,
  BOBGUI_PRINT_CAPABILITY_GENERATE_PS      = 1 << 6,
  BOBGUI_PRINT_CAPABILITY_PREVIEW          = 1 << 7,
  BOBGUI_PRINT_CAPABILITY_NUMBER_UP        = 1 << 8,
  BOBGUI_PRINT_CAPABILITY_NUMBER_UP_LAYOUT = 1 << 9
} BobguiPrintCapabilities;

GDK_AVAILABLE_IN_ALL
GType bobgui_print_capabilities_get_type (void) G_GNUC_CONST;

#define BOBGUI_TYPE_PRINTER                  (bobgui_printer_get_type ())
#define BOBGUI_PRINTER(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_PRINTER, BobguiPrinter))
#define BOBGUI_IS_PRINTER(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_PRINTER))

typedef struct _BobguiPrinter      BobguiPrinter;
typedef struct _BobguiPrintBackend BobguiPrintBackend;

GDK_AVAILABLE_IN_ALL
GType                    bobgui_printer_get_type              (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
BobguiPrinter              *bobgui_printer_new                   (const char      *name,
							    BobguiPrintBackend *backend,
							    gboolean         virtual_);
GDK_AVAILABLE_IN_ALL
BobguiPrintBackend         *bobgui_printer_get_backend           (BobguiPrinter      *printer);
GDK_AVAILABLE_IN_ALL
const char *            bobgui_printer_get_name              (BobguiPrinter      *printer);
GDK_AVAILABLE_IN_ALL
const char *            bobgui_printer_get_state_message     (BobguiPrinter      *printer);
GDK_AVAILABLE_IN_ALL
const char *            bobgui_printer_get_description       (BobguiPrinter      *printer);
GDK_AVAILABLE_IN_ALL
const char *            bobgui_printer_get_location          (BobguiPrinter      *printer);
GDK_AVAILABLE_IN_ALL
const char *            bobgui_printer_get_icon_name         (BobguiPrinter      *printer);
GDK_AVAILABLE_IN_ALL
int                      bobgui_printer_get_job_count         (BobguiPrinter      *printer);
GDK_AVAILABLE_IN_ALL
gboolean                 bobgui_printer_is_active             (BobguiPrinter      *printer);
GDK_AVAILABLE_IN_ALL
gboolean                 bobgui_printer_is_paused             (BobguiPrinter      *printer);
GDK_AVAILABLE_IN_ALL
gboolean                 bobgui_printer_is_accepting_jobs     (BobguiPrinter      *printer);
GDK_AVAILABLE_IN_ALL
gboolean                 bobgui_printer_is_virtual            (BobguiPrinter      *printer);
GDK_AVAILABLE_IN_ALL
gboolean                 bobgui_printer_is_default            (BobguiPrinter      *printer);
GDK_AVAILABLE_IN_ALL
gboolean                 bobgui_printer_accepts_pdf           (BobguiPrinter      *printer);
GDK_AVAILABLE_IN_ALL
gboolean                 bobgui_printer_accepts_ps            (BobguiPrinter      *printer);
GDK_AVAILABLE_IN_ALL
GList                   *bobgui_printer_list_papers           (BobguiPrinter      *printer);
GDK_AVAILABLE_IN_ALL
BobguiPageSetup            *bobgui_printer_get_default_page_size (BobguiPrinter      *printer);
GDK_AVAILABLE_IN_ALL
int                      bobgui_printer_compare               (BobguiPrinter *a,
						    	    BobguiPrinter *b);
GDK_AVAILABLE_IN_ALL
gboolean                 bobgui_printer_has_details           (BobguiPrinter       *printer);
GDK_AVAILABLE_IN_ALL
void                     bobgui_printer_request_details       (BobguiPrinter       *printer);
GDK_AVAILABLE_IN_ALL
BobguiPrintCapabilities     bobgui_printer_get_capabilities      (BobguiPrinter       *printer);
GDK_AVAILABLE_IN_ALL
gboolean                 bobgui_printer_get_hard_margins      (BobguiPrinter       *printer,
                                                            double           *top,
                                                            double           *bottom,
                                                            double           *left,
                                                            double           *right);
GDK_AVAILABLE_IN_ALL
gboolean                 bobgui_printer_get_hard_margins_for_paper_size (BobguiPrinter       *printer,
								      BobguiPaperSize     *paper_size,
								      double           *top,
								      double           *bottom,
								      double           *left,
								      double           *right);

/**
 * BobguiPrinterFunc:
 * @printer: a `BobguiPrinter`
 * @data: (closure): user data passed to bobgui_enumerate_printers()
 *
 * The type of function passed to bobgui_enumerate_printers().
 *
 * Note that you need to ref @printer, if you want to keep
 * a reference to it after the function has returned.
 *
 * Returns: %TRUE to stop the enumeration, %FALSE to continue
 */
typedef gboolean (*BobguiPrinterFunc) (BobguiPrinter *printer,
				    gpointer    data);

GDK_AVAILABLE_IN_ALL
void                     bobgui_enumerate_printers        (BobguiPrinterFunc   func,
							gpointer         data,
							GDestroyNotify   destroy,
							gboolean         wait);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiPrinter, g_object_unref)

G_END_DECLS

