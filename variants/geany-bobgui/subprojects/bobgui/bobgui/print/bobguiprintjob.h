/* BobguiPrintJob
 * Copyright (C) 2006 Red Hat,Inc.
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
#include <bobgui/print/bobguiprinter.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_PRINT_JOB                  (bobgui_print_job_get_type ())
#define BOBGUI_PRINT_JOB(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_PRINT_JOB, BobguiPrintJob))
#define BOBGUI_IS_PRINT_JOB(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_PRINT_JOB))

typedef struct _BobguiPrintJob          BobguiPrintJob;

/**
 * BobguiPrintJobCompleteFunc:
 * @print_job: the `BobguiPrintJob`
 * @user_data: user data that has been passed to bobgui_print_job_send()
 * @error: a `GError` that contains error information if the sending
 *   of the print job failed, otherwise %NULL
 *
 * The type of callback that is passed to bobgui_print_job_send().
 *
 * It is called when the print job has been completely sent.
 */
typedef void (*BobguiPrintJobCompleteFunc) (BobguiPrintJob  *print_job,
                                         gpointer      user_data,
                                         const GError *error);


GDK_AVAILABLE_IN_ALL
GType                    bobgui_print_job_get_type               (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
BobguiPrintJob             *bobgui_print_job_new                    (const char               *title,
							       BobguiPrinter               *printer,
							       BobguiPrintSettings         *settings,
							       BobguiPageSetup             *page_setup);
GDK_AVAILABLE_IN_ALL
BobguiPrintSettings        *bobgui_print_job_get_settings           (BobguiPrintJob              *job);
GDK_AVAILABLE_IN_ALL
BobguiPrinter              *bobgui_print_job_get_printer            (BobguiPrintJob              *job);
GDK_AVAILABLE_IN_ALL
const char *            bobgui_print_job_get_title              (BobguiPrintJob              *job);
GDK_AVAILABLE_IN_ALL
BobguiPrintStatus           bobgui_print_job_get_status             (BobguiPrintJob              *job);
GDK_AVAILABLE_IN_ALL
gboolean                 bobgui_print_job_set_source_file        (BobguiPrintJob              *job,
							       const char               *filename,
							       GError                  **error);
GDK_AVAILABLE_IN_ALL
gboolean                 bobgui_print_job_set_source_fd          (BobguiPrintJob              *job,
							       int                       fd,
							       GError                  **error);
GDK_AVAILABLE_IN_ALL
cairo_surface_t         *bobgui_print_job_get_surface            (BobguiPrintJob              *job,
							       GError                  **error);
GDK_AVAILABLE_IN_ALL
void                     bobgui_print_job_set_track_print_status (BobguiPrintJob              *job,
							       gboolean                  track_status);
GDK_AVAILABLE_IN_ALL
gboolean                 bobgui_print_job_get_track_print_status (BobguiPrintJob              *job);
GDK_AVAILABLE_IN_ALL
void                     bobgui_print_job_send                   (BobguiPrintJob              *job,
							       BobguiPrintJobCompleteFunc   callback,
							       gpointer                  user_data,
							       GDestroyNotify            dnotify);

GDK_AVAILABLE_IN_ALL
BobguiPrintPages     bobgui_print_job_get_pages       (BobguiPrintJob       *job);
GDK_AVAILABLE_IN_ALL
void              bobgui_print_job_set_pages       (BobguiPrintJob       *job,
                                                 BobguiPrintPages      pages);
GDK_AVAILABLE_IN_ALL
BobguiPageRange *    bobgui_print_job_get_page_ranges (BobguiPrintJob       *job,
                                                 int               *n_ranges);
GDK_AVAILABLE_IN_ALL
void              bobgui_print_job_set_page_ranges (BobguiPrintJob       *job,
                                                 BobguiPageRange      *ranges,
                                                 int                n_ranges);
GDK_AVAILABLE_IN_ALL
BobguiPageSet        bobgui_print_job_get_page_set    (BobguiPrintJob       *job);
GDK_AVAILABLE_IN_ALL
void              bobgui_print_job_set_page_set    (BobguiPrintJob       *job,
                                                 BobguiPageSet         page_set);
GDK_AVAILABLE_IN_ALL
int               bobgui_print_job_get_num_copies  (BobguiPrintJob       *job);
GDK_AVAILABLE_IN_ALL
void              bobgui_print_job_set_num_copies  (BobguiPrintJob       *job,
                                                 int                num_copies);
GDK_AVAILABLE_IN_ALL
double            bobgui_print_job_get_scale       (BobguiPrintJob       *job);
GDK_AVAILABLE_IN_ALL
void              bobgui_print_job_set_scale       (BobguiPrintJob       *job,
                                                 double             scale);
GDK_AVAILABLE_IN_ALL
guint             bobgui_print_job_get_n_up        (BobguiPrintJob       *job);
GDK_AVAILABLE_IN_ALL
void              bobgui_print_job_set_n_up        (BobguiPrintJob       *job,
                                                 guint              n_up);
GDK_AVAILABLE_IN_ALL
BobguiNumberUpLayout bobgui_print_job_get_n_up_layout (BobguiPrintJob       *job);
GDK_AVAILABLE_IN_ALL
void              bobgui_print_job_set_n_up_layout (BobguiPrintJob       *job,
                                                 BobguiNumberUpLayout  layout);
GDK_AVAILABLE_IN_ALL
gboolean          bobgui_print_job_get_rotate      (BobguiPrintJob       *job);
GDK_AVAILABLE_IN_ALL
void              bobgui_print_job_set_rotate      (BobguiPrintJob       *job,
                                                 gboolean           rotate);
GDK_AVAILABLE_IN_ALL
gboolean          bobgui_print_job_get_collate     (BobguiPrintJob       *job);
GDK_AVAILABLE_IN_ALL
void              bobgui_print_job_set_collate     (BobguiPrintJob       *job,
                                                 gboolean           collate);
GDK_AVAILABLE_IN_ALL
gboolean          bobgui_print_job_get_reverse     (BobguiPrintJob       *job);
GDK_AVAILABLE_IN_ALL
void              bobgui_print_job_set_reverse     (BobguiPrintJob       *job,
                                                 gboolean           reverse);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiPrintJob, g_object_unref)

G_END_DECLS

