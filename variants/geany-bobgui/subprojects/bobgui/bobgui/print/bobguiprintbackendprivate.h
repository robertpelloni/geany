/* BOBGUI - The Bobgui Framework
 * bobguiprintbackend.h: Abstract printer backend interfaces
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

/* This is a "semi-private" header; it is meant only for
 * alternate BobguiPrintDialog backend modules; no stability guarantees
 * are made at this point
 */
#ifndef BOBGUI_PRINT_BACKEND_ENABLE_UNSUPPORTED
#error "BobguiPrintBackend is not supported API for general use"
#endif

#include <bobgui/bobgui.h>
#include "bobguiunixprint.h"
#include "bobguiprinteroptionsetprivate.h"

G_BEGIN_DECLS

typedef struct _BobguiPrintBackendClass    BobguiPrintBackendClass;
typedef struct _BobguiPrintBackendPrivate  BobguiPrintBackendPrivate;

#define BOBGUI_PRINT_BACKEND_ERROR (bobgui_print_backend_error_quark ())

typedef enum
{
  /* TODO: add specific errors */
  BOBGUI_PRINT_BACKEND_ERROR_GENERIC
} BobguiPrintBackendError;

GDK_AVAILABLE_IN_ALL
GQuark     bobgui_print_backend_error_quark      (void);

#define BOBGUI_TYPE_PRINT_BACKEND                  (bobgui_print_backend_get_type ())
#define BOBGUI_PRINT_BACKEND(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_PRINT_BACKEND, BobguiPrintBackend))
#define BOBGUI_PRINT_BACKEND_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), BOBGUI_TYPE_PRINT_BACKEND, BobguiPrintBackendClass))
#define BOBGUI_IS_PRINT_BACKEND(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_PRINT_BACKEND))
#define BOBGUI_IS_PRINT_BACKEND_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), BOBGUI_TYPE_PRINT_BACKEND))
#define BOBGUI_PRINT_BACKEND_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_PRINT_BACKEND, BobguiPrintBackendClass))

typedef enum 
{
  BOBGUI_PRINT_BACKEND_STATUS_UNKNOWN,
  BOBGUI_PRINT_BACKEND_STATUS_OK,
  BOBGUI_PRINT_BACKEND_STATUS_UNAVAILABLE
} BobguiPrintBackendStatus;

struct _BobguiPrintBackend
{
  GObject parent_instance;

  BobguiPrintBackendPrivate *priv;
};

struct _BobguiPrintBackendClass
{
  GObjectClass parent_class;

  /* Global backend methods: */
  void                   (*request_printer_list)            (BobguiPrintBackend        *backend);
  void                   (*print_stream)                    (BobguiPrintBackend        *backend,
							     BobguiPrintJob            *job,
							     GIOChannel             *data_io,
							     BobguiPrintJobCompleteFunc callback,
							     gpointer                user_data,
							     GDestroyNotify          dnotify);

  /* Printer methods: */
  void                  (*printer_request_details)           (BobguiPrinter          *printer);
  cairo_surface_t *     (*printer_create_cairo_surface)      (BobguiPrinter          *printer,
							      BobguiPrintSettings    *settings,
							      double               height,
							      double               width,
							      GIOChannel          *cache_io);
  BobguiPrinterOptionSet * (*printer_get_options)               (BobguiPrinter          *printer,
							      BobguiPrintSettings    *settings,
							      BobguiPageSetup        *page_setup,
							      BobguiPrintCapabilities capabilities);
  gboolean              (*printer_mark_conflicts)            (BobguiPrinter          *printer,
							      BobguiPrinterOptionSet *options);
  void                  (*printer_get_settings_from_options) (BobguiPrinter          *printer,
							      BobguiPrinterOptionSet *options,
							      BobguiPrintSettings    *settings);
  void                  (*printer_prepare_for_print)         (BobguiPrinter          *printer,
							      BobguiPrintJob         *print_job,
							      BobguiPrintSettings    *settings,
							      BobguiPageSetup        *page_setup);
  GList  *              (*printer_list_papers)               (BobguiPrinter          *printer);
  BobguiPageSetup *        (*printer_get_default_page_size)     (BobguiPrinter          *printer);
  gboolean              (*printer_get_hard_margins)          (BobguiPrinter          *printer,
							      double              *top,
							      double              *bottom,
							      double              *left,
							      double              *right);
  BobguiPrintCapabilities  (*printer_get_capabilities)          (BobguiPrinter          *printer);

  /* Signals */
  void                  (*printer_list_changed)              (BobguiPrintBackend     *backend);
  void                  (*printer_list_done)                 (BobguiPrintBackend     *backend);
  void                  (*printer_added)                     (BobguiPrintBackend     *backend,
							      BobguiPrinter          *printer);
  void                  (*printer_removed)                   (BobguiPrintBackend     *backend,
							      BobguiPrinter          *printer);
  void                  (*printer_status_changed)            (BobguiPrintBackend     *backend,
							      BobguiPrinter          *printer);
  void                  (*request_password)                  (BobguiPrintBackend     *backend,
                                                              gpointer             auth_info_required,
                                                              gpointer             auth_info_default,
                                                              gpointer             auth_info_display,
                                                              gpointer             auth_info_visible,
                                                              const char          *prompt,
                                                              gboolean             can_store_auth_info);

  /* not a signal */
  void                  (*set_password)                      (BobguiPrintBackend     *backend,
                                                              char               **auth_info_required,
                                                              char               **auth_info,
                                                              gboolean             store_auth_info);

  gboolean              (*printer_get_hard_margins_for_paper_size) (BobguiPrinter    *printer,
								    BobguiPaperSize  *paper_size,
								    double        *top,
								    double        *bottom,
								    double        *left,
								    double        *right);
};

#define BOBGUI_PRINT_BACKEND_EXTENSION_POINT_NAME "bobgui-print-backend"

GDK_AVAILABLE_IN_ALL
GType   bobgui_print_backend_get_type       (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
GList      *bobgui_print_backend_get_printer_list     (BobguiPrintBackend         *print_backend);
GDK_AVAILABLE_IN_ALL
GListModel *bobgui_print_backend_get_printers         (BobguiPrintBackend         *print_backend);
GDK_AVAILABLE_IN_ALL
gboolean    bobgui_print_backend_printer_list_is_done (BobguiPrintBackend         *print_backend);
GDK_AVAILABLE_IN_ALL
BobguiPrinter *bobgui_print_backend_find_printer         (BobguiPrintBackend         *print_backend,
						    const char              *printer_name);
GDK_AVAILABLE_IN_ALL
void        bobgui_print_backend_print_stream         (BobguiPrintBackend         *print_backend,
						    BobguiPrintJob             *job,
						    GIOChannel              *data_io,
						    BobguiPrintJobCompleteFunc  callback,
						    gpointer                 user_data,
						    GDestroyNotify           dnotify);
GDK_AVAILABLE_IN_ALL
GList *     bobgui_print_backend_load_modules         (void);
GDK_AVAILABLE_IN_ALL
void        bobgui_print_backend_destroy              (BobguiPrintBackend         *print_backend);
GDK_AVAILABLE_IN_ALL
void        bobgui_print_backend_set_password         (BobguiPrintBackend         *backend, 
                                                    char                   **auth_info_required,
                                                    char                   **auth_info,
                                                    gboolean                 can_store_auth_info);

/* Backend-only functions for BobguiPrintBackend */

GDK_AVAILABLE_IN_ALL
void        bobgui_print_backend_add_printer          (BobguiPrintBackend         *print_backend,
						    BobguiPrinter              *printer);
GDK_AVAILABLE_IN_ALL
void        bobgui_print_backend_remove_printer       (BobguiPrintBackend         *print_backend,
						    BobguiPrinter              *printer);
GDK_AVAILABLE_IN_ALL
void        bobgui_print_backend_set_list_done        (BobguiPrintBackend         *backend);


/* Backend-only functions for BobguiPrinter */
GDK_AVAILABLE_IN_ALL
gboolean    bobgui_printer_is_new                (BobguiPrinter      *printer);
GDK_AVAILABLE_IN_ALL
void        bobgui_printer_set_accepts_pdf       (BobguiPrinter      *printer,
					       gboolean         val);
GDK_AVAILABLE_IN_ALL
void        bobgui_printer_set_accepts_ps        (BobguiPrinter      *printer,
					       gboolean         val);
GDK_AVAILABLE_IN_ALL
void        bobgui_printer_set_is_new            (BobguiPrinter      *printer,
					       gboolean         val);
GDK_AVAILABLE_IN_ALL
void        bobgui_printer_set_is_active         (BobguiPrinter      *printer,
					       gboolean         val);
GDK_AVAILABLE_IN_ALL
gboolean    bobgui_printer_set_is_paused         (BobguiPrinter      *printer,
					       gboolean         val);
GDK_AVAILABLE_IN_ALL
gboolean    bobgui_printer_set_is_accepting_jobs (BobguiPrinter      *printer,
					       gboolean         val);
GDK_AVAILABLE_IN_ALL
void        bobgui_printer_set_has_details       (BobguiPrinter      *printer,
					       gboolean         val);
GDK_AVAILABLE_IN_ALL
void        bobgui_printer_set_is_default        (BobguiPrinter      *printer,
					       gboolean         val);
GDK_AVAILABLE_IN_ALL
void        bobgui_printer_set_icon_name         (BobguiPrinter      *printer,
					       const char      *icon);
GDK_AVAILABLE_IN_ALL
gboolean    bobgui_printer_set_job_count         (BobguiPrinter      *printer,
					       int              count);
GDK_AVAILABLE_IN_ALL
gboolean    bobgui_printer_set_location          (BobguiPrinter      *printer,
					       const char      *location);
GDK_AVAILABLE_IN_ALL
gboolean    bobgui_printer_set_description       (BobguiPrinter      *printer,
					       const char      *description);
GDK_AVAILABLE_IN_ALL
gboolean    bobgui_printer_set_state_message     (BobguiPrinter      *printer,
					       const char      *message);

void        bobgui_print_backends_init (void);


G_END_DECLS

