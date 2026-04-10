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


#include <config.h>
#include <cpdb/frontend.h>

#include <bobgui/bobguiprivate.h>
#include <bobgui/bobguimodulesprivate.h>
#include "bobguiprintbackendcpdb.h"
#include "bobguiprintbackendutils.h"
#include "bobguiprintercpdb.h"

#include <cairo.h>
#include <cairo-pdf.h>

#include <glib/gi18n-lib.h>

#define BOBGUI_PRINT_BACKEND_CPDB_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BOBGUI_TYPE_PRINT_BACKEND_CPDB, BobguiPrintBackendCpdbClass))
#define BOBGUI_IS_PRINT_BACKEND_CPDB_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), BOBGUI_TYPE_PRINT_BACKEND_CPDB))
#define BOBGUI_PRINT_BACKEND_CPDB_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_PRINT_BACKEND_CPDB, BobguiPrintBackendCpdbClass))

/*
 * Multiplier for converting points to mm
 */
#define points_multiplier 2.83464567

#define _CPDB_MAX_CHUNK_SIZE 8192



static void bobgui_print_backend_cpdb_finalize                   (GObject                    *object);

static void cpdb_request_printer_list                         (BobguiPrintBackend            *backend);

static void cpdb_printer_request_details                      (BobguiPrinter                 *printer);

static void cpdb_acquire_details_cb                           (cpdb_printer_obj_t         *cpdb_printer_obj,
                                                               int                         success,
                                                               gpointer                    user_data);

static BobguiPrintCapabilities cpdb_printer_get_capabilities     (BobguiPrinter                 *printer);

static BobguiPrinterOptionSet *cpdb_printer_get_options          (BobguiPrinter                 *printer,
                                                               BobguiPrintSettings           *settings,
                                                               BobguiPageSetup               *page_setup,
                                                               BobguiPrintCapabilities        capabilities);

static BobguiPageSetup *cpdb_get_bobgui_page_setup                  (cpdb_printer_obj_t         *printer_obj,
                                                               const char                 *media);
static GList *cpdb_printer_list_papers                        (BobguiPrinter                 *printer);
static BobguiPageSetup *cpdb_printer_get_default_page_size       (BobguiPrinter                 *printer);

static gboolean cpdb_printer_get_hard_margins                 (BobguiPrinter                 *printer,
                                                               double                     *top,
                                                               double                     *bottom,
                                                               double                     *left,
                                                               double                     *right);
static gboolean cpdb_printer_get_hard_margins_for_paper_size  (BobguiPrinter                 *printer,
                                                               BobguiPaperSize               *paper_size,
                                                               double                     *top,
                                                               double                     *bottom,
                                                               double                     *left,
                                                               double                     *right);

static void cpdb_printer_get_settings_from_options            (BobguiPrinter                 *printer,
                                                               BobguiPrinterOptionSet        *options,
                                                               BobguiPrintSettings           *settings);

static void cpdb_printer_prepare_for_print                    (BobguiPrinter                 *printer,
                                                               BobguiPrintJob                *print_job,
                                                               BobguiPrintSettings           *settings,
                                                               BobguiPageSetup               *page_setup);

static void cpdb_print_cb                                     (BobguiPrintBackendCpdb        *cpdb_backend,
                                                               GError                     *error,
                                                               gpointer                    user_data);

static gboolean cpdb_write                                    (GIOChannel                 *source,
                                                               GIOCondition                con,
                                                               gpointer                    user_data);

static void cpdb_print_stream                                 (BobguiPrintBackend            *backend,
                                                               BobguiPrintJob                *job,
                                                               GIOChannel                 *data_io,
                                                               BobguiPrintJobCompleteFunc     callback,
                                                               gpointer                    user_data,
                                                               GDestroyNotify              dnotify);

static void bobgui_printer_cpdb_configure_page_setup             (BobguiPrinter                 *printer,
                                                               BobguiPageSetup               *page_setup,
                                                               BobguiPrintSettings           *settings);

static void bobgui_printer_cpdb_configure_settings	              (const char                 *key,
                                                               const char                 *value,
                                                               gpointer                    user_data);

static cairo_surface_t *cpdb_printer_create_cairo_surface     (BobguiPrinter                 *printer,
                                                               BobguiPrintSettings           *settings,
                                                               double                      width,
                                                               double                      height,
                                                               GIOChannel                 *cache_io);

static void cpdb_fill_bobgui_option                              (BobguiPrinterOption           *bobgui_option,
                                                               cpdb_option_t              *cpdb_option,
                                                               cpdb_printer_obj_t         *printer_obj);

static void printer_updates_callback                          (cpdb_frontend_obj_t        *frontend_obj,
                                                               cpdb_printer_obj_t         *printer_obj,
                                                               cpdb_printer_update_t       change);

static void add_option_to_settings                            (BobguiPrinterOption           *option,
                                                               gpointer                    user_data);

static void cpdb_printer_add_hash_table                       (gpointer                    key,
                                                               gpointer                    value,
                                                               gpointer                    user_data);

static void cpdb_add_bobgui_printer                              (BobguiPrintBackend            *backend,
                                                               cpdb_printer_obj_t         *printer_obj);

static void cpdb_remove_bobgui_printer                           (BobguiPrintBackend            *backend,
                                                               cpdb_printer_obj_t         *printer_obj);

static void set_state_message                                 (BobguiPrinter                 *printer);

static char *cpdb_option_translation                          (cpdb_printer_obj_t         *printer_obj,
                                                               const char                 *option_name);
static char *cpdb_choice_translation                          (cpdb_printer_obj_t         *printer_obj,
                                                               const char                 *option_name,
                                                               const char                 *choice_name);

static void initialize                                        (void);
static gchar *get_bobgui_group                                   (cpdb_printer_obj_t         *printer_obj,
                                                               const char                 *group_name);


/*
 * List of locales for text translation
 */
static const gchar* const* locales = NULL;

/*
 * Mark the options which have already been displayed in other tabs,
 * and exclude them from the "Advanced" tab, while getting printer options
 */
static GHashTable *already_used_options = NULL;


struct _BobguiPrintBackendCpdbClass
{
  BobguiPrintBackendClass parent_class;
};

struct _BobguiPrintBackendCpdb
{
  BobguiPrintBackend parent_instance;
  cpdb_frontend_obj_t *frontend_obj;
};

typedef struct {
  BobguiPrintBackend *backend;
  BobguiPrintJobCompleteFunc callback;
  BobguiPrintJob *job;
  char *path;
  GIOStream *target_io_stream;
  gpointer user_data;
  GDestroyNotify dnotify;
} _PrintStreamData;

BOBGUI_DEFINE_BUILTIN_MODULE_TYPE_WITH_CODE (BobguiPrintBackendCpdb, bobgui_print_backend_cpdb, BOBGUI_TYPE_PRINT_BACKEND,
                         g_io_extension_point_implement (BOBGUI_PRINT_BACKEND_EXTENSION_POINT_NAME,
                                                         g_define_type_id,
                                                         "cpdb",
                                                         20))

/*
 * BobguiPrintBackend object for currently opened print dialog
 */
static BobguiPrintBackend *bobgui_print_backend = NULL;

/*
 * BobguiPrintBackendCpdb
 */

/*
 * bobgui_print_backend_cpdb_new:
 *
 * Creates a new #BobguiPrintBackendCpdb object. #BobguiPrintBackendCpdb
 * implements the #BobguiPrintBackend interface with direct access to
 * the filesystem using Unix/Linux API calls
 *
 * Returns: the new #BobguiPrintBackendCpdb object
 */
BobguiPrintBackend *
bobgui_print_backend_cpdb_new (void)
{
  BOBGUI_DEBUG (PRINTING, "CPDB Backend: Creating a new CPDB print backend object");

  return g_object_new (BOBGUI_TYPE_PRINT_BACKEND_CPDB, NULL);
}


static void
bobgui_print_backend_cpdb_class_init (BobguiPrintBackendCpdbClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  BobguiPrintBackendClass *backend_class = BOBGUI_PRINT_BACKEND_CLASS (klass);

  gobject_class->finalize = bobgui_print_backend_cpdb_finalize;

  backend_class->request_printer_list = cpdb_request_printer_list;
  backend_class->printer_request_details = cpdb_printer_request_details;
  backend_class->printer_get_capabilities = cpdb_printer_get_capabilities;
  backend_class->printer_get_options = cpdb_printer_get_options;
  backend_class->printer_list_papers = cpdb_printer_list_papers;
  backend_class->printer_get_default_page_size = cpdb_printer_get_default_page_size;
  backend_class->printer_get_hard_margins = cpdb_printer_get_hard_margins;
  backend_class->printer_get_hard_margins_for_paper_size = cpdb_printer_get_hard_margins_for_paper_size;
  backend_class->printer_get_settings_from_options = cpdb_printer_get_settings_from_options;
  backend_class->printer_prepare_for_print = cpdb_printer_prepare_for_print;
  backend_class->printer_create_cairo_surface = cpdb_printer_create_cairo_surface;
  backend_class->print_stream = cpdb_print_stream;
}

static void
bobgui_print_backend_cpdb_init (BobguiPrintBackendCpdb *backend_cpdb)
{
  initialize ();

  BOBGUI_DEBUG (PRINTING, "Creating frontendObj for CPDB backend");
  backend_cpdb->frontend_obj = cpdbGetNewFrontendObj ((cpdb_printer_callback) printer_updates_callback);
  cpdbIgnoreLastSavedSettings (backend_cpdb->frontend_obj);
  bobgui_print_backend = BOBGUI_PRINT_BACKEND (backend_cpdb);
}

static void
bobgui_print_backend_cpdb_finalize (GObject *object)
{
  BobguiPrintBackendCpdb *backend_cpdb;
  GObjectClass *backend_parent_class;

  BOBGUI_DEBUG (PRINTING, "Finalizing CPDB backend object");

  backend_cpdb = BOBGUI_PRINT_BACKEND_CPDB (object);
  backend_parent_class = G_OBJECT_CLASS (bobgui_print_backend_cpdb_parent_class);
  cpdbDeleteFrontendObj (backend_cpdb->frontend_obj);
  bobgui_print_backend = NULL;

  backend_parent_class->finalize (object);
}

/*
 * This function is responsible for displaying the printer
 * list obtained from CPDB backend on the print dialog.
 */
static void
cpdb_request_printer_list (BobguiPrintBackend *backend)
{
  BobguiPrintBackendCpdb *backend_cpdb = BOBGUI_PRINT_BACKEND_CPDB (backend);

  cpdbConnectToDBus (backend_cpdb->frontend_obj);
  g_hash_table_foreach (backend_cpdb->frontend_obj->printer,
                        cpdb_printer_add_hash_table,
                        backend);
  bobgui_print_backend_set_list_done (backend);
}

/*
 * This function is responsible for making a printer acquire all
 * the details and supported options list, which need not be queried
 * until the user clicks on the printer in the printer list in the dialog.
 * The print dialog runs this function asynchronously and displays
 * "Getting printer attributes..." as the printer status meanwhile.
 * This function is needed as some printers (like temporary CUPS queue)
 * can take a couple of seconds to materialize and acquire all the details,
 * and it's important the dialog doesn't block during that time.
 */
static void
cpdb_printer_request_details (BobguiPrinter *printer)
{
  BobguiPrinterCpdb *printer_cpdb = BOBGUI_PRINTER_CPDB (printer);
  cpdb_printer_obj_t *printer_obj = bobgui_printer_cpdb_get_printer_obj (printer_cpdb);

  cpdbAcquireDetails (printer_obj, cpdb_acquire_details_cb, printer);
}

static void
cpdb_acquire_details_cb (cpdb_printer_obj_t   *printer_obj,
                         int                  success,
                         gpointer             user_data)
{
  BobguiPrinter *printer = BOBGUI_PRINTER (user_data);
  BobguiPrintBackend *backend = bobgui_printer_get_backend (printer);
  gboolean accepting_jobs, paused, status_changed;

  if (success == 0)
    {
      BOBGUI_DEBUG (PRINTING, "Error acquiring printer details");
      g_signal_emit_by_name (printer, "details-acquired", FALSE);
      return;
    }

  accepting_jobs = cpdbIsAcceptingJobs (printer_obj);
  paused = g_strcmp0 (cpdbGetState (printer_obj), "stopped") == 0;
  status_changed = paused ^ bobgui_printer_is_paused (printer);

  bobgui_printer_set_is_accepting_jobs (printer, accepting_jobs);
  bobgui_printer_set_is_paused (printer, paused);
  set_state_message (printer);

  bobgui_printer_set_has_details (printer, TRUE);
  g_signal_emit_by_name (printer, "details-acquired", TRUE);

  if (status_changed)
    g_signal_emit_by_name (backend, "printer-status-changed", printer);
}


/*
 * This function is responsible for specifying which features the
 * print dialog should offer for the given printer.
 */
static BobguiPrintCapabilities
cpdb_printer_get_capabilities (BobguiPrinter *printer)
{
  BobguiPrintCapabilities capabilities = 0;
  cpdb_option_t *cpdb_option;
  BobguiPrinterCpdb *printer_cpdb = BOBGUI_PRINTER_CPDB (printer);
  cpdb_printer_obj_t *printer_obj = bobgui_printer_cpdb_get_printer_obj (printer_cpdb);

  cpdb_option = cpdbGetOption (printer_obj, CPDB_OPTION_PAGE_SET);
  if (cpdb_option != NULL && cpdb_option->num_supported >= 3)
    capabilities |= BOBGUI_PRINT_CAPABILITY_PAGE_SET;

  cpdb_option = cpdbGetOption (printer_obj, CPDB_OPTION_COPIES);
  if (cpdb_option != NULL &&
      g_strcmp0 (cpdb_option->supported_values[0], "1") != 0 &&
      g_strcmp0 (cpdb_option->supported_values[0], "1-1") != 0)
    capabilities |= BOBGUI_PRINT_CAPABILITY_COPIES;

  cpdb_option = cpdbGetOption (printer_obj, CPDB_OPTION_COLLATE);
  if (cpdb_option != NULL && cpdb_option->num_supported > 1)
    capabilities |= BOBGUI_PRINT_CAPABILITY_COLLATE;

  cpdb_option = cpdbGetOption (printer_obj, CPDB_OPTION_PAGE_DELIVERY);
  if (cpdb_option != NULL && cpdb_option->num_supported > 1)
    capabilities |= BOBGUI_PRINT_CAPABILITY_REVERSE;

  cpdb_option = cpdbGetOption (printer_obj, CPDB_OPTION_PRINT_SCALING);
  if (cpdb_option != NULL && cpdb_option->num_supported > 1)
    capabilities |= BOBGUI_PRINT_CAPABILITY_SCALE;

  cpdb_option = cpdbGetOption (printer_obj, CPDB_OPTION_NUMBER_UP);
  if (cpdb_option != NULL && cpdb_option->num_supported > 1)
    capabilities |= BOBGUI_PRINT_CAPABILITY_NUMBER_UP;

  cpdb_option = cpdbGetOption (printer_obj, CPDB_OPTION_NUMBER_UP_LAYOUT);
  if (cpdb_option != NULL && cpdb_option->num_supported > 1)
    capabilities |= BOBGUI_PRINT_CAPABILITY_NUMBER_UP_LAYOUT;

  return capabilities;
}


/*
 * This function is responsible for getting all the options
 * that the printer supports and display them into the
 * GUI template of BOBGUI print dialog box.
 * The backend should obtain whatever options it supports,
 * from either its print server or PPDs.
 */
static BobguiPrinterOptionSet *
cpdb_printer_get_options (BobguiPrinter            *printer,
                          BobguiPrintSettings      *settings,
                          BobguiPageSetup          *page_setup,
                          BobguiPrintCapabilities  capabilities)
{
  char *option_name;
  char *display_name;
  gpointer key, value;
  GHashTableIter iter;
  BobguiPrinterCpdb *printer_cpdb;
  cpdb_printer_obj_t *printer_obj;
  cpdb_option_t *cpdb_option;
  BobguiPrinterOption *bobgui_option;
  BobguiPrinterOptionSet *bobgui_option_set;

  bobgui_option_set = bobgui_printer_option_set_new ();
  printer_cpdb = BOBGUI_PRINTER_CPDB (printer);
  printer_obj = bobgui_printer_cpdb_get_printer_obj (printer_cpdb);

  /** Page-Setup **/
  cpdb_option = cpdbGetOption (printer_obj, CPDB_OPTION_NUMBER_UP);
  if ((capabilities & BOBGUI_PRINT_CAPABILITY_NUMBER_UP) && cpdb_option != NULL)
    {
      display_name = cpdb_option_translation (printer_obj,
                                              cpdb_option->option_name);
      bobgui_option = bobgui_printer_option_new ("bobgui-n-up",
                                           display_name,
                                           BOBGUI_PRINTER_OPTION_TYPE_PICKONE);
      cpdb_fill_bobgui_option (bobgui_option, cpdb_option, printer_obj);
      bobgui_printer_option_set_add (bobgui_option_set, bobgui_option);
      g_object_unref (bobgui_option);
    }

  cpdb_option = cpdbGetOption (printer_obj, CPDB_OPTION_NUMBER_UP_LAYOUT);
  if ((capabilities & BOBGUI_PRINT_CAPABILITY_NUMBER_UP_LAYOUT) && cpdb_option != NULL)
    {
      display_name = cpdb_option_translation (printer_obj,
                                              cpdb_option->option_name);
      bobgui_option = bobgui_printer_option_new ("bobgui-n-up-layout",
                                           display_name,
                                           BOBGUI_PRINTER_OPTION_TYPE_PICKONE);
      cpdb_fill_bobgui_option (bobgui_option, cpdb_option, printer_obj);
      bobgui_printer_option_set_add (bobgui_option_set, bobgui_option);
      g_object_unref (bobgui_option);
    }

  cpdb_option = cpdbGetOption (printer_obj, CPDB_OPTION_SIDES);
  if (cpdb_option != NULL && cpdb_option->num_supported > 1)
    {
      display_name = cpdb_option_translation (printer_obj,
                                              cpdb_option->option_name);
      bobgui_option = bobgui_printer_option_new ("bobgui-duplex",
                                           display_name,
                                           BOBGUI_PRINTER_OPTION_TYPE_PICKONE);
      cpdb_fill_bobgui_option (bobgui_option, cpdb_option, printer_obj);
      bobgui_printer_option_set_add (bobgui_option_set, bobgui_option);
      g_object_unref (bobgui_option);
    }

  cpdb_option = cpdbGetOption (printer_obj, CPDB_OPTION_MEDIA_SOURCE);
  if (cpdb_option != NULL && cpdb_option->num_supported > 1)
    {
      display_name = cpdb_option_translation (printer_obj,
                                              cpdb_option->option_name);
      bobgui_option = bobgui_printer_option_new ("bobgui-paper-source",
                                           display_name,
                                           BOBGUI_PRINTER_OPTION_TYPE_PICKONE);
      cpdb_fill_bobgui_option (bobgui_option, cpdb_option, printer_obj);
      bobgui_printer_option_set_add (bobgui_option_set, bobgui_option);
      g_object_unref (bobgui_option);
    }

  cpdb_option = cpdbGetOption (printer_obj, CPDB_OPTION_MEDIA_TYPE);
  if (cpdb_option != NULL && cpdb_option->num_supported > 1)
    {
      display_name = cpdb_option_translation (printer_obj,
                                              cpdb_option->option_name);
      bobgui_option = bobgui_printer_option_new ("bobgui-paper-type",
                                           display_name,
                                           BOBGUI_PRINTER_OPTION_TYPE_PICKONE);
      cpdb_fill_bobgui_option (bobgui_option, cpdb_option, printer_obj);
      bobgui_printer_option_set_add (bobgui_option_set, bobgui_option);
      g_object_unref (bobgui_option);
    }

  cpdb_option = cpdbGetOption (printer_obj, CPDB_OPTION_OUTPUT_BIN);
  if (cpdb_option != NULL && cpdb_option->num_supported > 1)
    {
      display_name = cpdb_option_translation (printer_obj,
                                              cpdb_option->option_name);
      bobgui_option = bobgui_printer_option_new ("bobgui-output-tray",
                                           display_name,
                                           BOBGUI_PRINTER_OPTION_TYPE_PICKONE);
      cpdb_fill_bobgui_option (bobgui_option, cpdb_option, printer_obj);
      bobgui_printer_option_set_add (bobgui_option_set, bobgui_option);
      g_object_unref (bobgui_option);
    }


  /** Jobs **/
  cpdb_option = cpdbGetOption (printer_obj, CPDB_OPTION_JOB_PRIORITY);
  if (cpdb_option != NULL)
    {
      /* job-priority is represented as a number from 1-100 */
      const char *prio[] = {"100", "80", "50", "30"};
      const char *prio_display[] = {
        NC_("Print job priority", "Urgent"),
        NC_("Print job priority", "High"),
        NC_("Print job priority", "Medium"),
        NC_("Print job priority", "Low")
      };

      for (int i = 0; i < G_N_ELEMENTS(prio_display); i++)
        prio_display[i] = _(prio_display[i]);
      display_name = cpdb_option_translation (printer_obj,
                                              cpdb_option->option_name);
      bobgui_option = bobgui_printer_option_new ("bobgui-job-prio",
                                           display_name,
                                           BOBGUI_PRINTER_OPTION_TYPE_PICKONE);
      bobgui_printer_option_choices_from_array (bobgui_option,
                                             G_N_ELEMENTS (prio),
                                             prio, prio_display);
      bobgui_printer_option_set (bobgui_option, "50");
      bobgui_printer_option_set_add (bobgui_option_set, bobgui_option);
      g_object_unref (bobgui_option);
    }

  cpdb_option = cpdbGetOption (printer_obj, CPDB_OPTION_JOB_SHEETS);
  if (cpdb_option != NULL && cpdb_option->num_supported > 1)
    {
      bobgui_option = bobgui_printer_option_new ("bobgui-cover-before",
                                           C_("printer option", "Before"),
                                           BOBGUI_PRINTER_OPTION_TYPE_PICKONE);
      cpdb_fill_bobgui_option (bobgui_option, cpdb_option, printer_obj);
      bobgui_printer_option_set_add (bobgui_option_set, bobgui_option);
      g_object_unref (bobgui_option);

      bobgui_option = bobgui_printer_option_new ("bobgui-cover-after",
                                           C_("printer option", "After"),
                                           BOBGUI_PRINTER_OPTION_TYPE_PICKONE);

      cpdb_fill_bobgui_option (bobgui_option, cpdb_option, printer_obj);
      bobgui_printer_option_set_add (bobgui_option_set, bobgui_option);
      g_object_unref (bobgui_option);
    }

  cpdb_option = cpdbGetOption (printer_obj, CPDB_OPTION_BILLING_INFO);
  if (cpdb_option != NULL)
    {
      display_name = cpdb_option_translation (printer_obj,
                                              cpdb_option->option_name);
      bobgui_option = bobgui_printer_option_new ("bobgui-billing-info",
                                          display_name,
                                          BOBGUI_PRINTER_OPTION_TYPE_STRING);
      bobgui_printer_option_set (bobgui_option, "");
      bobgui_printer_option_set_add (bobgui_option_set, bobgui_option);
      g_object_unref (bobgui_option);
    }

  const char *print_at[] = {"now", "at", "on-hold"};
  bobgui_option = bobgui_printer_option_new ("bobgui-print-time",
                                       _("Print at"),
                                       BOBGUI_PRINTER_OPTION_TYPE_PICKONE);
  bobgui_printer_option_choices_from_array (bobgui_option,
                                         G_N_ELEMENTS (print_at),
                                         print_at, print_at);
  bobgui_printer_option_set (bobgui_option, "now");
  bobgui_printer_option_set_add (bobgui_option_set, bobgui_option);
  g_object_unref (bobgui_option);

  bobgui_option = bobgui_printer_option_new ("bobgui-print-time-text",
                                       _("Print at time"),
                                       BOBGUI_PRINTER_OPTION_TYPE_STRING);
  bobgui_printer_option_set (bobgui_option, "");
  bobgui_printer_option_set_add (bobgui_option_set, bobgui_option);
  g_object_unref (bobgui_option);

  /** Other options **/
  g_hash_table_iter_init (&iter, printer_obj->options->table);
  while (g_hash_table_iter_next (&iter, &key, &value))
    {
      option_name = (char *) key;
      if (g_hash_table_contains (already_used_options, option_name))
          continue;

      cpdb_option = (cpdb_option_t *) value;
      if (cpdb_option->num_supported <= 1)
        continue;

      display_name = cpdb_option_translation (printer_obj,
                                              cpdb_option->option_name);
      bobgui_option = bobgui_printer_option_new (cpdb_option->option_name,
                                           display_name,
                                           BOBGUI_PRINTER_OPTION_TYPE_PICKONE);
      cpdb_fill_bobgui_option (bobgui_option, cpdb_option, printer_obj);
      bobgui_option->group = get_bobgui_group (printer_obj,
                                         cpdb_option->group_name);
      bobgui_printer_option_set_add (bobgui_option_set, bobgui_option);
      g_object_unref (bobgui_option);
    }

  /*
   * Check if borderles printing is supported
   */
  gboolean borderless = TRUE;
  const char *attrs[] = {CPDB_OPTION_MARGIN_TOP, CPDB_OPTION_MARGIN_BOTTOM,
                         CPDB_OPTION_MARGIN_LEFT, CPDB_OPTION_MARGIN_RIGHT};
  for (int i = 0; i < 4; i++)
    {
      cpdb_option = cpdbGetOption (printer_obj, (gchar *) attrs[i]);
      gboolean found = FALSE;

      if (cpdb_option != NULL)
        {
          for (int j = 0; j < cpdb_option->num_supported; j++)
            {
              if (g_strcmp0 (cpdb_option->supported_values[j], "0") == 0)
                {
                  found = TRUE;
                  break;
                }
            }
        }

      if (!found)
        {
          borderless = FALSE;
          break;
        }
    }

  if (borderless)
    {
      bobgui_option = bobgui_printer_option_new ("borderless",
                                           C_("print option", "Borderless"),
                                           BOBGUI_PRINTER_OPTION_TYPE_BOOLEAN);
      bobgui_option->group = get_bobgui_group (printer_obj, CPDB_GROUP_MEDIA);
      bobgui_printer_option_set_add (bobgui_option_set, bobgui_option);
      g_object_unref (bobgui_option);
    }

  return bobgui_option_set;
}

/*
 * Helper function to create BobguiPageSetup from given "media" size
 */
static BobguiPageSetup *
cpdb_get_bobgui_page_setup (cpdb_printer_obj_t *printer_obj,
                         const char *media)
{
  char *display_name;
  int width, height, num_margins, ok;
  cpdb_margin_t *margins;
  BobguiPaperSize *paper_size;
  BobguiPageSetup *page_setup;

  page_setup = bobgui_page_setup_new ();
  display_name = cpdb_choice_translation (printer_obj,
                                          CPDB_OPTION_MEDIA,
                                          media);
  ok = cpdbGetMediaSize (printer_obj,
                         media,
                         &width,
                         &height);
  if (ok == 1)
    {
      paper_size = bobgui_paper_size_new_custom (media,
                                              display_name,
                                              width / 100.0,
                                              height / 100.0,
                                              BOBGUI_UNIT_MM);
      bobgui_page_setup_set_paper_size (page_setup, paper_size);
      bobgui_paper_size_free (paper_size);
    }
  num_margins = cpdbGetMediaMargins (printer_obj,
                                     media,
                                     &margins);
  if (num_margins > 0)
    {
      bobgui_page_setup_set_left_margin   (page_setup,
                                        margins[0].left / 100.0,
                                        BOBGUI_UNIT_MM);
      bobgui_page_setup_set_right_margin  (page_setup,
                                        margins[0].right / 100.0,
                                        BOBGUI_UNIT_MM);
      bobgui_page_setup_set_top_margin    (page_setup,
                                        margins[0].top / 100.0,
                                        BOBGUI_UNIT_MM);
      bobgui_page_setup_set_bottom_margin (page_setup,
                                        margins[0].bottom / 100.0,
                                        BOBGUI_UNIT_MM);
    }
  return page_setup;
}

/*
 * This function is responsible for listing all the page sizes supported by a printer
 */
static GList *
cpdb_printer_list_papers (BobguiPrinter *printer)
{
  cpdb_option_t *media;
  GList *result = NULL;
  BobguiPageSetup *page_setup;

  BobguiPrinterCpdb *printer_cpdb = BOBGUI_PRINTER_CPDB (printer);
  cpdb_printer_obj_t *printer_obj = bobgui_printer_cpdb_get_printer_obj (printer_cpdb);

  media = cpdbGetOption (printer_obj, CPDB_OPTION_MEDIA);
  if (media == NULL)
    return NULL;

  for (int i = 0; i < media->num_supported; i++)
    {
      if (g_str_has_prefix (media->supported_values[i], "custom_min") ||
          g_str_has_prefix (media->supported_values[i], "custom_max"))
        continue;

      page_setup = cpdb_get_bobgui_page_setup (printer_obj,
                                            media->supported_values[i]);
      result = g_list_prepend (result, page_setup);
    }
  result = g_list_reverse (result);
  return result;
}

/*
 * This function is responsible for getting the default page size for a printer
 */
static BobguiPageSetup *
cpdb_printer_get_default_page_size (BobguiPrinter *printer)
{
  char *default_media;
  BobguiPageSetup *page_setup = NULL;

  BobguiPrinterCpdb *printer_cpdb = BOBGUI_PRINTER_CPDB (printer);
  cpdb_printer_obj_t *printer_obj = bobgui_printer_cpdb_get_printer_obj (printer_cpdb);

  default_media = cpdbGetDefault (printer_obj, CPDB_OPTION_MEDIA);
  if (default_media != NULL)
    {
      page_setup = cpdb_get_bobgui_page_setup (printer_obj, default_media);
    }

  return page_setup;
}

/*
 * This function is responsible for getting the
 * default page size margins supported by a printer
 */
static gboolean
cpdb_printer_get_hard_margins (BobguiPrinter   *printer,
                               double       *top,
                               double       *bottom,
                               double       *left,
                               double       *right)
{
  char *left_margin, *right_margin, *top_margin, *bottom_margin;

  BobguiPrinterCpdb *printer_cpdb = BOBGUI_PRINTER_CPDB (printer);
  cpdb_printer_obj_t *printer_obj = bobgui_printer_cpdb_get_printer_obj (printer_cpdb);

  top_margin = cpdbGetDefault (printer_obj, CPDB_OPTION_MARGIN_TOP);
  left_margin = cpdbGetDefault (printer_obj, CPDB_OPTION_MARGIN_LEFT);
  right_margin = cpdbGetDefault (printer_obj, CPDB_OPTION_MARGIN_RIGHT);
  bottom_margin = cpdbGetDefault (printer_obj, CPDB_OPTION_MARGIN_BOTTOM);

  if (top_margin != NULL &&
      left_margin != NULL &&
      right_margin != NULL &&
      bottom_margin != NULL)
    {
      *top = g_ascii_strtod (top_margin, NULL) * points_multiplier / 100.0;
      *left = g_ascii_strtod (left_margin, NULL) * points_multiplier / 100.0;
      *right = g_ascii_strtod (right_margin, NULL) * points_multiplier / 100.0;
      *bottom = g_ascii_strtod (bottom_margin, NULL) * points_multiplier / 100.0;
      return TRUE;
    }

  return FALSE;
}

/*
 * This function is responsible for getting the
 * default page size margins for give paper size
 */
static gboolean
cpdb_printer_get_hard_margins_for_paper_size (BobguiPrinter      *printer,
                                              BobguiPaperSize    *paper_size,
                                              double          *top,
                                              double          *bottom,
                                              double          *left,
                                              double          *right)
{
  int num_margins;
  const char *media;
  cpdb_margin_t *margins;

  BobguiPrinterCpdb *printer_cpdb = BOBGUI_PRINTER_CPDB (printer);
  cpdb_printer_obj_t *printer_obj = bobgui_printer_cpdb_get_printer_obj (printer_cpdb);

  media = bobgui_paper_size_get_name (paper_size);
  if (media != NULL)
    {
      num_margins = cpdbGetMediaMargins (printer_obj, media, &margins);
      if (num_margins > 0)
        {
          *top = margins[0].top * points_multiplier / 100.0;
          *left = margins[0].left * points_multiplier / 100.0;
          *right = margins[0].right * points_multiplier / 100.0;
          *bottom = margins[0].bottom * points_multiplier / 100.0;

          return TRUE;
        }
    }

  return FALSE;
}

/*
 * @user_data: BobguiPrintSettings
 */
static void
add_option_to_settings (BobguiPrinterOption    *option,
                        gpointer            user_data)
{
  BobguiPrintSettings *settings = (BobguiPrintSettings *) user_data;
  bobgui_print_settings_set (settings, option->name, option->value);
}

/*
 * This method is invoked when the print button on the print dialog is pressed.
 * It's responsible for gathering all the settings from the print dialog that the
 * user may have selected before pressing the print button.
 */
static void
cpdb_printer_get_settings_from_options (BobguiPrinter            *printer,
                                        BobguiPrinterOptionSet   *options,
                                        BobguiPrintSettings      *settings)
{
  char *value;
  BobguiPrinterOption *option, *cover_before, *cover_after;

  option = bobgui_printer_option_set_lookup (options, "bobgui-n-up");
  if (option != NULL)
    {
      bobgui_print_settings_set (settings,
                              CPDB_OPTION_NUMBER_UP,
                              option->value);
    }

  option = bobgui_printer_option_set_lookup (options, "bobgui-n-up-layout");
  if (option != NULL)
    {
      bobgui_print_settings_set (settings,
                              CPDB_OPTION_NUMBER_UP_LAYOUT,
                              option->value);
    }

  option = bobgui_printer_option_set_lookup (options, "bobgui-duplex");
  if (option != NULL)
    {
      bobgui_print_settings_set (settings,
                              CPDB_OPTION_SIDES,
                              option->value);
    }

  option = bobgui_printer_option_set_lookup (options, "bobgui-paper-source");
  if (option != NULL)
    {
      bobgui_print_settings_set (settings,
                              CPDB_OPTION_MEDIA_SOURCE,
                              option->value);
    }

  option = bobgui_printer_option_set_lookup (options, "bobgui-paper-type");
  if (option != NULL)
    {
      bobgui_print_settings_set (settings,
                              CPDB_OPTION_MEDIA_TYPE,
                              option->value);
    }

  option = bobgui_printer_option_set_lookup (options, "bobgui-output-tray");
  if (option != NULL)
    {
      bobgui_print_settings_set (settings,
                              CPDB_OPTION_OUTPUT_BIN,
                              option->value);
    }

  option = bobgui_printer_option_set_lookup (options, "bobgui-job-prio");
  if (option != NULL)
    {
      bobgui_print_settings_set (settings,
                              CPDB_OPTION_JOB_PRIORITY,
                              option->value);
    }

  cover_before = bobgui_printer_option_set_lookup (options, "bobgui-cover-before");
  cover_after = bobgui_printer_option_set_lookup (options, "bobgui-cover-after");
  if (cover_before != NULL && cover_after != NULL)
    {
      value = g_strdup_printf ("%s,%s",
                               cover_before->value,
                               cover_after->value);
      bobgui_print_settings_set (settings,
                              CPDB_OPTION_JOB_SHEETS,
                              value);
      g_free (value);
    }

  option = bobgui_printer_option_set_lookup (options, "bobgui-billing-info");
  if (option != NULL)
    {
      bobgui_print_settings_set (settings,
                              CPDB_OPTION_BILLING_INFO,
                              option->value);
    }

  char *print_at = NULL;
  option = bobgui_printer_option_set_lookup (options, "bobgui-print-time");
  if (option != NULL)
    print_at = g_strdup (option->value);

  char *print_at_time = NULL;
  option = bobgui_printer_option_set_lookup (options, "bobgui-print-time-text");
  if (option != NULL)
    print_at_time = g_strdup (option->value);

  if (print_at != NULL && print_at_time != NULL)
    {
      if (g_strcmp0 (print_at, "at") == 0)
        {
          char *utc_time = NULL;

          utc_time = localtime_to_utctime (print_at_time);
          if (utc_time != NULL)
            {
              bobgui_print_settings_set (settings,
                                      CPDB_OPTION_JOB_HOLD_UNTIL,
                                      utc_time);
              g_free (utc_time);
            }
          else
            bobgui_print_settings_set (settings,
                                    CPDB_OPTION_JOB_HOLD_UNTIL,
                                    print_at_time);
        }
      else if (g_strcmp0 (print_at, "on-hold") == 0)
        bobgui_print_settings_set (settings,
                                CPDB_OPTION_JOB_HOLD_UNTIL,
                                CPDB_JOB_HOLD_INDEFINITE);
    }
  if (print_at != NULL)
    g_free (print_at);
  if (print_at_time != NULL)
    g_free (print_at_time);

  bobgui_printer_option_set_foreach (options, add_option_to_settings, settings);
}

static cairo_status_t
_cairo_write (void                  *closure,
              const unsigned char   *data,
              unsigned int          length)
{
  GIOChannel *io = (GIOChannel *)closure;
  gsize written;
  GError *error;

  error = NULL;

  BOBGUI_DEBUG (PRINTING, "CPDB Backend: Writing %i byte chunk to temp file", length);

  while (length > 0)
    {
      g_io_channel_write_chars (io, (const char *)data, length, &written, &error);

      if (error != NULL)
        {
          BOBGUI_DEBUG (PRINTING, "CPDB Backend: Error writing to temp file, %s", error->message);

          g_error_free (error);
          return CAIRO_STATUS_WRITE_ERROR;
        }

      BOBGUI_DEBUG (PRINTING, "CPDB Backend: Wrote %" G_GSIZE_FORMAT " bytes to temp file\n", written);

      data += written;
      length -= written;
    }

  return CAIRO_STATUS_SUCCESS;
}


/*
 * called after prepare_for_print ()
 */
static cairo_surface_t *
cpdb_printer_create_cairo_surface  (BobguiPrinter          *printer,
                                    BobguiPrintSettings    *settings,
                                    double              width,
                                    double              height,
                                    GIOChannel          *cache_io)
{
  cairo_surface_t *surface;

  surface = cairo_pdf_surface_create_for_stream (_cairo_write, cache_io, width, height);

  cairo_surface_set_fallback_resolution  (surface,
                                          2.0 * bobgui_print_settings_get_printer_lpi (settings),
                                          2.0 * bobgui_print_settings_get_printer_lpi (settings));

  return surface;
}

static void
bobgui_printer_cpdb_configure_settings (const char     *key,
                                     const char     *value,
                                     gpointer       user_data)
{
  const char *option_name, *option_value;
  BobguiPrinterCpdb *printer_cpdb = BOBGUI_PRINTER_CPDB (user_data);
  cpdb_printer_obj_t *printer_obj = bobgui_printer_cpdb_get_printer_obj (printer_cpdb);

  option_name = key;
  option_value = value;
  if (g_str_has_prefix (option_name, "bobgui") ||
      g_strcmp0 (option_value, "") == 0)
    {
      return;
    }

  cpdbAddSettingToPrinter (printer_obj, option_name, option_value);
}

static void
bobgui_printer_cpdb_configure_page_setup (BobguiPrinter         *printer,
                                       BobguiPageSetup       *page_setup,
                                       BobguiPrintSettings   *settings)
{
  char *value, *orientation, *default_orientation;
  const char *borderless;
  double width, height, left, top, right, bottom;
  BobguiPageOrientation page_orientation;

  BobguiPrinterCpdb *printer_cpdb = BOBGUI_PRINTER_CPDB (printer);
  cpdb_printer_obj_t *printer_obj = bobgui_printer_cpdb_get_printer_obj (printer_cpdb);

  width = bobgui_page_setup_get_paper_width (page_setup, BOBGUI_UNIT_MM) * 100.0;
  height = bobgui_page_setup_get_paper_height (page_setup, BOBGUI_UNIT_MM) * 100.0;
  left = bobgui_page_setup_get_left_margin (page_setup, BOBGUI_UNIT_MM) * 100.0;
  right = bobgui_page_setup_get_right_margin (page_setup, BOBGUI_UNIT_MM) * 100.0;
  top = bobgui_page_setup_get_top_margin (page_setup, BOBGUI_UNIT_MM) * 100.0;
  bottom = bobgui_page_setup_get_bottom_margin (page_setup, BOBGUI_UNIT_MM) * 100.0;

  borderless = bobgui_print_settings_get (settings, "borderless");
  if (g_ascii_strcasecmp (borderless, "True") == 0)
    left = right = top = bottom = 0;

  value = g_strdup_printf ("{media-size={x-dimension=%.0f y-dimension=%.0f} "
                            "media-bottom-margin=%.0f "
                            "media-left-margin=%.0f "
                            "media-right-margin=%.0f "
                            "media-top-margin=%.0f}",
                            width, height, bottom, left, right, top);
  bobgui_print_settings_set (settings, CPDB_OPTION_MEDIA_COL, value);
  g_free (value);

  page_orientation = bobgui_page_setup_get_orientation (page_setup);
  default_orientation = cpdbGetDefault (printer_obj, CPDB_OPTION_ORIENTATION);

  switch (page_orientation)
    {
    case BOBGUI_PAGE_ORIENTATION_PORTRAIT:
      orientation = g_strdup_printf (CPDB_ORIENTATION_PORTRAIT);
      break;

    case BOBGUI_PAGE_ORIENTATION_LANDSCAPE:
      orientation = g_strdup_printf (CPDB_ORIENTATION_LANDSCAPE);
      break;

    case BOBGUI_PAGE_ORIENTATION_REVERSE_LANDSCAPE:
      orientation = g_strdup_printf (CPDB_ORIENTATION_RLANDSCAPE);
      break;

    case BOBGUI_PAGE_ORIENTATION_REVERSE_PORTRAIT:
      orientation = g_strdup_printf (CPDB_ORIENTATION_RPORTRAIT);
      break;

    default:
      orientation = default_orientation;
    }

  bobgui_print_settings_set (settings, CPDB_OPTION_ORIENTATION, orientation);
  g_free (orientation);
}

static void
cpdb_printer_prepare_for_print (BobguiPrinter          *printer,
                                BobguiPrintJob         *print_job,
                                BobguiPrintSettings    *settings,
                                BobguiPageSetup        *page_setup)
{
  int n_ranges;
  double scale;

  BobguiPrintPages pages;
  BobguiPageRange *ranges;
  BobguiPageSet page_set;
  BobguiPrintCapabilities capabilities;

  capabilities = cpdb_printer_get_capabilities (printer);

  pages = bobgui_print_settings_get_print_pages (settings);
  bobgui_print_job_set_pages (print_job, pages);
  bobgui_print_settings_unset (settings, BOBGUI_PRINT_SETTINGS_PRINT_PAGES);

  if (pages == BOBGUI_PRINT_PAGES_RANGES)
    ranges = bobgui_print_settings_get_page_ranges (settings, &n_ranges);
  else
    {
      ranges = NULL;
      n_ranges = 0;
    }
  bobgui_print_job_set_page_ranges (print_job, ranges, n_ranges);
  bobgui_print_settings_unset (settings, BOBGUI_PRINT_SETTINGS_PAGE_RANGES);

  scale = bobgui_print_settings_get_scale (settings);
  if (scale != 100.0)
    bobgui_print_job_set_scale (print_job, scale / 100.0);
  bobgui_print_settings_unset (settings, BOBGUI_PRINT_SETTINGS_SCALE);

  if (capabilities & BOBGUI_PRINT_CAPABILITY_COLLATE)
    {
      if (bobgui_print_settings_get_collate (settings))
        bobgui_print_settings_set (settings,
                                CPDB_OPTION_COLLATE,
                                CPDB_COLLATE_ENABLED);
    }
  bobgui_print_job_set_collate (print_job, FALSE);
  bobgui_print_settings_unset (settings, BOBGUI_PRINT_SETTINGS_COLLATE);

  if (capabilities & BOBGUI_PRINT_CAPABILITY_REVERSE)
    {
      if (bobgui_print_settings_get_reverse (settings))
        bobgui_print_settings_set (settings,
                                CPDB_OPTION_PAGE_DELIVERY,
                                CPDB_PAGE_DELIVERY_REVERSE);
    }
  bobgui_print_job_set_reverse (print_job, FALSE);
  bobgui_print_settings_unset (settings, BOBGUI_PRINT_SETTINGS_REVERSE);

  if (capabilities & BOBGUI_PRINT_CAPABILITY_COPIES)
    {
      int copies = bobgui_print_settings_get_n_copies (settings);
      if (copies > 1)
        {
          char *value = g_strdup_printf ("%d", copies);
          bobgui_print_settings_set (settings, CPDB_OPTION_COPIES, value);
          g_free (value);
        }
    }
  bobgui_print_job_set_num_copies (print_job, 1);
  bobgui_print_settings_unset (settings, BOBGUI_PRINT_SETTINGS_N_COPIES);

  page_set = bobgui_print_settings_get_page_set (settings);
  switch (page_set)
    {
    case BOBGUI_PAGE_SET_EVEN :
      bobgui_print_settings_set (settings,
                              CPDB_OPTION_PAGE_SET,
                              CPDB_PAGE_SET_EVEN);
      break;

    case BOBGUI_PAGE_SET_ODD :
      bobgui_print_settings_set (settings,
                              CPDB_OPTION_PAGE_SET,
                              CPDB_PAGE_SET_ODD);
      break;

    case BOBGUI_PAGE_SET_ALL :
    default :
      bobgui_print_settings_set (settings,
                              CPDB_OPTION_PAGE_SET,
                              CPDB_PAGE_SET_ALL);
    }
  bobgui_print_job_set_page_set (print_job, BOBGUI_PAGE_SET_ALL);
  bobgui_print_settings_unset (settings, BOBGUI_PRINT_SETTINGS_PAGE_SET);

  bobgui_print_settings_unset (settings, "printer");

  bobgui_printer_cpdb_configure_page_setup (printer, page_setup, settings);
  bobgui_print_settings_unset (settings, "borderless");

  bobgui_print_settings_foreach (settings,
                              bobgui_printer_cpdb_configure_settings,
                              printer);
}


static void
cpdb_print_cb  (BobguiPrintBackendCpdb   *backend_cpdb,
                GError                *error,
                gpointer              user_data)
{;
  BobguiPrinterCpdb *printer_cpdb;
  cpdb_printer_obj_t *printer_obj;
  _PrintStreamData *ps = (_PrintStreamData *) user_data;

  if (ps->target_io_stream != NULL)
    g_io_stream_close (G_IO_STREAM (ps->target_io_stream), NULL, NULL);

  if (ps->callback)
    ps->callback (ps->job, ps->user_data, error);

  if (ps->dnotify)
    ps->dnotify (ps->user_data);

  bobgui_print_job_set_status (ps->job,
                            error ? BOBGUI_PRINT_STATUS_FINISHED_ABORTED
                                  : BOBGUI_PRINT_STATUS_FINISHED);

  if (!error)
    {
      printer_cpdb = BOBGUI_PRINTER_CPDB (bobgui_print_job_get_printer (ps->job));
      printer_obj = bobgui_printer_cpdb_get_printer_obj (printer_cpdb);

      BOBGUI_DEBUG (PRINTING, "CPDB Backend: Sending file to CPDB for printing - %s\n", ps->path);

      cpdbPrintFile (printer_obj, ps->path);

      g_free (ps->path);
    }

  if (ps->job)
    g_object_unref (ps->job);

  g_free (ps);
}

static gboolean
cpdb_write (GIOChannel      *source,
            GIOCondition    con,
            gpointer        user_data)
{
  char buf[_CPDB_MAX_CHUNK_SIZE];
  gsize bytes_read;
  GError *error;
  GIOStatus status;
  GOutputStream *out_stream;
  _PrintStreamData *ps = (_PrintStreamData *) user_data;

  error = NULL;

  status = g_io_channel_read_chars (source,
                                    buf,
                                    _CPDB_MAX_CHUNK_SIZE,
                                    &bytes_read,
                                    &error);

  if (status != G_IO_STATUS_ERROR)
    {
      gsize bytes_written;

      out_stream = g_io_stream_get_output_stream (ps->target_io_stream);
      g_output_stream_write_all (out_stream,
                                buf,
                                bytes_read,
                                &bytes_written,
                                NULL,
                                &error);
    }

  if (error != NULL || status == G_IO_STATUS_EOF)
    {
      cpdb_print_cb (BOBGUI_PRINT_BACKEND_CPDB (ps->backend),
                     error,
                     user_data);

      if (error != NULL)
        {
          BOBGUI_DEBUG (PRINTING, "CPDB Backend: Error writing to file - %s\n", error->message);

          g_error_free (error);
        }

      return FALSE;
    }

  BOBGUI_DEBUG (PRINTING, "CPDB Backend: Writing %" G_GSIZE_FORMAT " byte chunk to cpdb pipe\n", bytes_read);

  return TRUE;
}



static void
cpdb_print_stream  (BobguiPrintBackend           *backend,
                    BobguiPrintJob               *job,
                    GIOChannel                *data_io,
                    BobguiPrintJobCompleteFunc   callback,
                    gpointer                  user_data,
                    GDestroyNotify            dnotify)
{
  GError *error;
  GFile *file;
  _PrintStreamData *ps;
  GFileIOStream *iostream;

  ps = g_new0 (_PrintStreamData, 1);
  ps->callback = callback;
  ps->user_data = user_data;
  ps->dnotify = dnotify;
  ps->job = g_object_ref (job);
  ps->backend = backend;

  error = NULL;
  file = g_file_new_tmp (NULL, &iostream, &error);
  if (file == NULL)
    goto error;

  ps->path = g_file_get_path (file);
  ps->target_io_stream = G_IO_STREAM (iostream);

  g_object_unref (file);

error:
  if (error != NULL)
    {
      BOBGUI_DEBUG (PRINTING, "Error: %s\n", error->message);

      cpdb_print_cb (BOBGUI_PRINT_BACKEND_CPDB (backend), error, ps);

      g_error_free (error);
      return;
    }

  g_io_add_watch (data_io,
                  G_IO_IN | G_IO_PRI | G_IO_ERR | G_IO_HUP,
                  (GIOFunc) cpdb_write,
                  ps);

}

/*
 * Reflect changes in printers to the BOBGUI print dialog.
 */
static void
printer_updates_callback (cpdb_frontend_obj_t     *frontend_obj,
                          cpdb_printer_obj_t      *printer_obj,
                          cpdb_printer_update_t    change)
{
  BobguiPrinter *printer;

  if (bobgui_print_backend == NULL)
    return;
  
  switch (change)
    {
    case CPDB_CHANGE_PRINTER_ADDED:
      cpdb_add_bobgui_printer (bobgui_print_backend, printer_obj);
      break;

    case CPDB_CHANGE_PRINTER_REMOVED:
      cpdb_remove_bobgui_printer (bobgui_print_backend, printer_obj);
      break;

    case CPDB_CHANGE_PRINTER_STATE_CHANGED:
      printer = bobgui_print_backend_find_printer (bobgui_print_backend, printer_obj->name);
      set_state_message (printer);
      g_signal_emit_by_name (bobgui_print_backend, "printer-status-changed", printer);
      break;
    
    default:
      break;
    }
}

 /*
  * @key: printer name
  * @value: cpdb_printer_obj_t
  * @user_data: BobguiPrintBackend
  */
static void
cpdb_printer_add_hash_table (gpointer key,
                             gpointer value,
                             gpointer user_data)
{
  cpdb_add_bobgui_printer (user_data, value);
}

/*
 * Adds given printer to given BobguiPrintBackend
 */
static void
cpdb_add_bobgui_printer (BobguiPrintBackend     *backend,
                      cpdb_printer_obj_t  *printer_obj)
{
  BobguiPrinter *printer;
  BobguiPrinterCpdb *printer_cpdb;
  cpdb_printer_obj_t *default_printer;

  BobguiPrintBackendCpdb *backend_cpdb = BOBGUI_PRINT_BACKEND_CPDB (backend);

  /*
   * Ignore printers from FILE backend, 
   * since we are using "Print To File" BOBGUI print backend.
   */
  if (g_strcmp0 (printer_obj->backend_name, "FILE") == 0)
    return;
  
  printer_cpdb = g_object_new (BOBGUI_TYPE_PRINTER_CPDB,
                               "name", printer_obj->name,
                               "backend", backend,
                               NULL);
  bobgui_printer_cpdb_set_printer_obj (printer_cpdb, printer_obj);

  printer = BOBGUI_PRINTER (printer_cpdb);
  bobgui_printer_set_icon_name (printer, "printer");
  bobgui_printer_set_location (printer, printer_obj->location);
  bobgui_printer_set_description (printer, printer_obj->info);
  bobgui_printer_set_accepts_pdf (printer, TRUE);
  bobgui_printer_set_accepts_ps (printer, TRUE);
  bobgui_printer_set_is_active (printer, TRUE);
  bobgui_printer_set_has_details (printer, FALSE);

  default_printer = cpdbGetDefaultPrinter (backend_cpdb->frontend_obj);
  if (default_printer == printer_obj)
    bobgui_printer_set_is_default (printer, TRUE);

  /*
   * If printer state is not available, wait till cpdbAcquireDetails ()
   * is called when the printer is clicked on in the print dialog
   */
  if (g_strcmp0 (printer_obj->state, "NA") == 0)
    {
      bobgui_printer_set_is_accepting_jobs (printer, TRUE);
      bobgui_printer_set_is_paused (printer, FALSE);
      bobgui_printer_set_state_message (printer, "");
    }
  else
    {
      bobgui_printer_set_is_accepting_jobs (printer,
                                         cpdbIsAcceptingJobs (printer_obj));
      bobgui_printer_set_is_paused (printer,
                                 g_strcmp0 (cpdbGetState (printer_obj),
                                            CPDB_STATE_STOPPED) == 0);
      set_state_message (printer);
    }

  bobgui_print_backend_add_printer (backend, printer);
  if (bobgui_print_backend_printer_list_is_done (backend))
    {
      g_signal_emit_by_name (backend, "printer-added", printer);
      g_signal_emit_by_name (backend, "printer-list-changed");
    }
  g_object_unref (printer);
}

/*
 * Removes given printer from given BobguiPrintBackend
 */
static void
cpdb_remove_bobgui_printer (BobguiPrintBackend      *backend,
                         cpdb_printer_obj_t   *printer_obj)
{
  BobguiPrinter *printer = bobgui_print_backend_find_printer (backend, printer_obj->name);

  bobgui_print_backend_remove_printer (backend, printer);
  g_signal_emit_by_name (backend, "printer-removed", printer);
  g_signal_emit_by_name (backend, "printer-list-changed");
}

/*
 * Sets printer status
 */
static void
set_state_message (BobguiPrinter *printer)
{
  gboolean stopped, accepting_jobs;
  BobguiPrinterCpdb *printer_cpdb = BOBGUI_PRINTER_CPDB (printer);
  cpdb_printer_obj_t *printer_obj = bobgui_printer_cpdb_get_printer_obj (printer_cpdb);

  stopped = g_strcmp0 (cpdbGetState (printer_obj), CPDB_STATE_STOPPED) == 0;
  accepting_jobs = cpdbIsAcceptingJobs (printer_obj);

  if (stopped && !accepting_jobs)
    /* Translators: this is a printer status. */
    bobgui_printer_set_state_message (printer, _("Paused; Rejecting Jobs"));
  else if (stopped && accepting_jobs)
    /* Translators: this is a printer status. */
    bobgui_printer_set_state_message (printer, _("Paused"));
  else if (!accepting_jobs)
    /* Translators: this is a printer status. */
    bobgui_printer_set_state_message (printer, _("Rejecting Jobs"));
  else
    bobgui_printer_set_state_message (printer, "");
}

/*
 * Fills option choices & sets default in bobgui_option from cpdb_option
 */
static void
cpdb_fill_bobgui_option (BobguiPrinterOption    *bobgui_option,
                      cpdb_option_t       *cpdb_option,
                      cpdb_printer_obj_t  *printer_obj)
{
  char *display_val;
  char **default_val;

  bobgui_printer_option_allocate_choices (bobgui_option, cpdb_option->num_supported);
  for (int i = 0; i < cpdb_option->num_supported; i++)
    {
      bobgui_option->choices[i] = g_strdup (cpdb_option->supported_values[i]);
      display_val = cpdb_choice_translation (printer_obj,
                                             cpdb_option->option_name,
                                             cpdb_option->supported_values[i]);
      bobgui_option->choices_display[i] = g_strdup (display_val);
    }

  if (g_strcmp0 (cpdb_option->default_value, "NA") != 0)
    {
      if (g_strcmp0 (cpdb_option->option_name, CPDB_OPTION_JOB_SHEETS) == 0)
        {
          default_val = g_strsplit (cpdb_option->default_value, ",", 2);
          if (g_strcmp0 (bobgui_option->name, "bobgui-cover-before") == 0)
            bobgui_printer_option_set (bobgui_option, default_val[0]);
          else if (g_strcmp0 (bobgui_option->name, "bobgui-cover-after") == 0)
            bobgui_printer_option_set (bobgui_option, default_val[1]);
        }
      else
        {
          bobgui_printer_option_set (bobgui_option, g_strdup (cpdb_option->default_value));
        }
     }
}

/*
 * Wrapper for getting translation of an option name
 */
static char *
cpdb_option_translation (cpdb_printer_obj_t *printer_obj,
                         const char *option_name)
{
  int i;
  char *translation = NULL;

  for (i = 0; locales[i] != NULL; i++)
    {
      translation = cpdbGetOptionTranslation (printer_obj,
                                              option_name,
                                              locales[i]);
      if (translation != NULL)
        break;
    }
  return translation;
}

/*
 * Wrapper for getting translation of a choice name
 */
static char *
cpdb_choice_translation (cpdb_printer_obj_t *printer_obj,
                         const char *option_name,
                         const char *choice_name)
{
  int i;
  char *translation = NULL;

  for (i = 0; locales[i] != NULL; i++)
    {
      translation = cpdbGetChoiceTranslation (printer_obj,
                                              option_name,
                                              choice_name,
                                              locales[i]);
      if (translation != NULL)
        break;
    }
  return translation;
}

/*
 * Wrapper for getting translation of a group name
 */
static char *
cpdb_group_translation (cpdb_printer_obj_t *printer_obj,
                        const char *group_name)
{
  int i;
  char *translation = NULL;

  for (i = 0; locales[i] != NULL; i++)
    {
      translation = cpdbGetGroupTranslation (printer_obj,
                                             group_name,
                                             locales[i]);
      if (translation != NULL)
        break;
    }
  return translation;
}

/*
 * Convert CPDB groups explicitly supported by BOBGUI print dialog
 */
static gchar *
get_bobgui_group (cpdb_printer_obj_t *printer_obj,
               const char *group_name)
{
  if (g_strcmp0 (group_name, CPDB_GROUP_COLOR) == 0)
    return g_strdup ("ColorPage");
  if (g_strcmp0 (group_name, CPDB_GROUP_QUALITY) == 0)
    return g_strdup ("ImageQualityPage");
  if (g_strcmp0 (group_name, CPDB_GROUP_FINISHINGS) == 0)
    return g_strdup ("FinishingPage");
  return cpdb_group_translation (printer_obj, group_name);
}

/*
 * Do initial setup
 */
static void
initialize (void)
{
  cpdbInit ();

  if (locales == NULL)
    locales = g_get_language_names ();

  if (already_used_options == NULL)
   {
      already_used_options = g_hash_table_new (g_str_hash, g_str_equal);
      g_hash_table_add (already_used_options, (gpointer) CPDB_OPTION_COPIES);
      g_hash_table_add (already_used_options, (gpointer) CPDB_OPTION_PAGE_RANGES);
      g_hash_table_add (already_used_options, (gpointer) CPDB_OPTION_ORIENTATION);
      g_hash_table_add (already_used_options, (gpointer) CPDB_OPTION_PAGE_DELIVERY);
      g_hash_table_add (already_used_options, (gpointer) CPDB_OPTION_COLLATE);
      g_hash_table_add (already_used_options, (gpointer) CPDB_OPTION_NUMBER_UP);
      g_hash_table_add (already_used_options, (gpointer) CPDB_OPTION_NUMBER_UP_LAYOUT);
      g_hash_table_add (already_used_options, (gpointer) CPDB_OPTION_PAGE_SET);
      g_hash_table_add (already_used_options, (gpointer) CPDB_OPTION_MEDIA);
      g_hash_table_add (already_used_options, (gpointer) CPDB_OPTION_MARGIN_TOP);
      g_hash_table_add (already_used_options, (gpointer) CPDB_OPTION_MARGIN_BOTTOM);
      g_hash_table_add (already_used_options, (gpointer) CPDB_OPTION_MARGIN_LEFT);
      g_hash_table_add (already_used_options, (gpointer) CPDB_OPTION_MARGIN_RIGHT);
      g_hash_table_add (already_used_options, (gpointer) CPDB_OPTION_SIDES);
      g_hash_table_add (already_used_options, (gpointer) CPDB_OPTION_MEDIA_SOURCE);
      g_hash_table_add (already_used_options, (gpointer) CPDB_OPTION_MEDIA_TYPE);
      g_hash_table_add (already_used_options, (gpointer) CPDB_OPTION_OUTPUT_BIN);
      g_hash_table_add (already_used_options, (gpointer) CPDB_OPTION_JOB_PRIORITY);
      g_hash_table_add (already_used_options, (gpointer) CPDB_OPTION_JOB_SHEETS);
      g_hash_table_add (already_used_options, (gpointer) CPDB_OPTION_JOB_HOLD_UNTIL);
      g_hash_table_add (already_used_options, (gpointer) CPDB_OPTION_BILLING_INFO);
      g_hash_table_add (already_used_options, (gpointer) "borderless");
   }
}
