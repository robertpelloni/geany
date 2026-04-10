/* BobguiPrinterCups
 * Copyright (C) 2006 John (J5) Palmieri <johnp@redhat.com>
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
#include <cups/cups.h>
#include <cups/ppd.h>
#include "bobguicupsutils.h"

#include <bobgui/bobguiunixprint.h>
#include <bobgui/print/bobguiprinterprivate.h>

#ifdef HAVE_COLORD
#include <colord.h>
#endif

G_BEGIN_DECLS

#define BOBGUI_TYPE_PRINTER_CUPS                  (bobgui_printer_cups_get_type ())
#define BOBGUI_PRINTER_CUPS(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_PRINTER_CUPS, BobguiPrinterCups))
#define BOBGUI_PRINTER_CUPS_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), BOBGUI_TYPE_PRINTER_CUPS, BobguiPrinterCupsClass))
#define BOBGUI_IS_PRINTER_CUPS(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_PRINTER_CUPS))
#define BOBGUI_IS_PRINTER_CUPS_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), BOBGUI_TYPE_PRINTER_CUPS))
#define BOBGUI_PRINTER_CUPS_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_PRINTER_CUPS, BobguiPrinterCupsClass))

typedef struct _BobguiPrinterCups          BobguiPrinterCups;
typedef struct _BobguiPrinterCupsClass     BobguiPrinterCupsClass;
typedef struct _BobguiPrinterCupsPrivate   BobguiPrinterCupsPrivate;

struct _BobguiPrinterCups
{
  BobguiPrinter parent_instance;

  char *device_uri;
  char *original_device_uri;
  char *printer_uri;
  char *hostname;
  int port;
  char **auth_info_required;
  char *original_hostname;
  char *original_resource;
  int original_port;
  gboolean request_original_uri;     /* Request PPD from original hostname */
  gboolean is_temporary;             /* This printer is temporary queue */
  gchar *temporary_queue_device_uri; /* Device uri of temporary queue for this printer */

  ipp_pstate_t state;
#if CUPS_VERSION_MAJOR < 3
  gboolean reading_ppd;
  char       *ppd_name;
  ppd_file_t *ppd_file;
#endif

  char     *media_default;
  GList    *media_supported;
  GList    *media_size_supported;
  int       media_bottom_margin_default;
  int       media_top_margin_default;
  int       media_left_margin_default;
  int       media_right_margin_default;
  gboolean  media_margin_default_set;
  char     *sides_default;
  GList    *sides_supported;
  char     *output_bin_default;
  GList    *output_bin_supported;

  char   *default_cover_before;
  char   *default_cover_after;

  int     default_number_up;

  gboolean remote;
  guint get_remote_ppd_poll;
  int   get_remote_ppd_attempts;
  BobguiCupsConnectionTest *remote_cups_connection_test;

#ifdef HAVE_COLORD
  CdClient     *colord_client;
  CdDevice     *colord_device;
  CdProfile    *colord_profile;
  GCancellable *colord_cancellable;
  char         *colord_title;
  char         *colord_qualifier;
#endif

  gboolean  avahi_browsed;
  char *avahi_name;
  char *avahi_type;
  char *avahi_domain;

  guchar ipp_version_major;
  guchar ipp_version_minor;
  gboolean supports_copies;
  gboolean supports_collate;
  gboolean supports_number_up;
  char   **covers;
  int      number_of_covers;
};

struct _BobguiPrinterCupsClass
{
  BobguiPrinterClass parent_class;

};

GType                    bobgui_printer_cups_get_type      (void) G_GNUC_CONST;
void                     bobgui_printer_cups_register_type (GTypeModule     *module);

BobguiPrinterCups          *bobgui_printer_cups_new           (const char      *name,
                                                         BobguiPrintBackend *backend,
                                                         gpointer         colord_client);
#if CUPS_VERSION_MAJOR < 3
ppd_file_t              *bobgui_printer_cups_get_ppd       (BobguiPrinterCups  *printer);
const char              *bobgui_printer_cups_get_ppd_name  (BobguiPrinterCups  *printer);
#endif

#ifdef HAVE_COLORD
void                     bobgui_printer_cups_update_settings (BobguiPrinterCups *printer,
                                                         BobguiPrintSettings *settings,
                                                         BobguiPrinterOptionSet *set);
#endif

G_END_DECLS
