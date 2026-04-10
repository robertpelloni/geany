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


#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <cairo.h>
#include <bobgui/bobgui.h>

#include <bobgui/print/bobguipagesetup.h>
#include <bobgui/print/bobguiprintsettings.h>
#include <bobgui/print/bobguiprintcontext.h>
#include <bobgui/print/bobguiprintoperationpreview.h>


G_BEGIN_DECLS

#define BOBGUI_TYPE_PRINT_OPERATION                (bobgui_print_operation_get_type ())
#define BOBGUI_PRINT_OPERATION(obj)                (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_PRINT_OPERATION, BobguiPrintOperation))
#define BOBGUI_PRINT_OPERATION_CLASS(klass)        (G_TYPE_CHECK_CLASS_CAST ((klass), BOBGUI_TYPE_PRINT_OPERATION, BobguiPrintOperationClass))
#define BOBGUI_IS_PRINT_OPERATION(obj)             (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_PRINT_OPERATION))
#define BOBGUI_IS_PRINT_OPERATION_CLASS(klass)     (G_TYPE_CHECK_CLASS_TYPE ((klass), BOBGUI_TYPE_PRINT_OPERATION))
#define BOBGUI_PRINT_OPERATION_GET_CLASS(obj)      (G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_PRINT_OPERATION, BobguiPrintOperationClass))

typedef struct _BobguiPrintOperationClass   BobguiPrintOperationClass;
typedef struct _BobguiPrintOperationPrivate BobguiPrintOperationPrivate;
typedef struct _BobguiPrintOperation        BobguiPrintOperation;

/**
 * BobguiPrintStatus:
 * @BOBGUI_PRINT_STATUS_INITIAL: The printing has not started yet; this
 *   status is set initially, and while the print dialog is shown.
 * @BOBGUI_PRINT_STATUS_PREPARING: This status is set while the begin-print
 *   signal is emitted and during pagination.
 * @BOBGUI_PRINT_STATUS_GENERATING_DATA: This status is set while the
 *   pages are being rendered.
 * @BOBGUI_PRINT_STATUS_SENDING_DATA: The print job is being sent off to the
 *   printer.
 * @BOBGUI_PRINT_STATUS_PENDING: The print job has been sent to the printer,
 *   but is not printed for some reason, e.g. the printer may be stopped.
 * @BOBGUI_PRINT_STATUS_PENDING_ISSUE: Some problem has occurred during
 *   printing, e.g. a paper jam.
 * @BOBGUI_PRINT_STATUS_PRINTING: The printer is processing the print job.
 * @BOBGUI_PRINT_STATUS_FINISHED: The printing has been completed successfully.
 * @BOBGUI_PRINT_STATUS_FINISHED_ABORTED: The printing has been aborted.
 *
 * The status gives a rough indication of the completion of a running
 * print operation.
 */
typedef enum {
  BOBGUI_PRINT_STATUS_INITIAL,
  BOBGUI_PRINT_STATUS_PREPARING,
  BOBGUI_PRINT_STATUS_GENERATING_DATA,
  BOBGUI_PRINT_STATUS_SENDING_DATA,
  BOBGUI_PRINT_STATUS_PENDING,
  BOBGUI_PRINT_STATUS_PENDING_ISSUE,
  BOBGUI_PRINT_STATUS_PRINTING,
  BOBGUI_PRINT_STATUS_FINISHED,
  BOBGUI_PRINT_STATUS_FINISHED_ABORTED
} BobguiPrintStatus;

/**
 * BobguiPrintOperationResult:
 * @BOBGUI_PRINT_OPERATION_RESULT_ERROR: An error has occurred.
 * @BOBGUI_PRINT_OPERATION_RESULT_APPLY: The print settings should be stored.
 * @BOBGUI_PRINT_OPERATION_RESULT_CANCEL: The print operation has been canceled,
 *   the print settings should not be stored.
 * @BOBGUI_PRINT_OPERATION_RESULT_IN_PROGRESS: The print operation is not complete
 *   yet. This value will only be returned when running asynchronously.
 *
 * The result of a print operation.
 *
 * A value of this type is returned by [method@Bobgui.PrintOperation.run].
 */
typedef enum {
  BOBGUI_PRINT_OPERATION_RESULT_ERROR,
  BOBGUI_PRINT_OPERATION_RESULT_APPLY,
  BOBGUI_PRINT_OPERATION_RESULT_CANCEL,
  BOBGUI_PRINT_OPERATION_RESULT_IN_PROGRESS
} BobguiPrintOperationResult;

/**
 * BobguiPrintOperationAction:
 * @BOBGUI_PRINT_OPERATION_ACTION_PRINT_DIALOG: Show the print dialog.
 * @BOBGUI_PRINT_OPERATION_ACTION_PRINT: Start to print without showing
 *   the print dialog, based on the current print settings, if possible.
 *   Depending on the platform, a print dialog might appear anyway.
 * @BOBGUI_PRINT_OPERATION_ACTION_PREVIEW: Show the print preview.
 * @BOBGUI_PRINT_OPERATION_ACTION_EXPORT: Export to a file. This requires
 *   the export-filename property to be set.
 *
 * Determines what action the print operation should perform.
 *
 * A parameter of this typs is passed to [method@Bobgui.PrintOperation.run].
 */
typedef enum {
  BOBGUI_PRINT_OPERATION_ACTION_PRINT_DIALOG,
  BOBGUI_PRINT_OPERATION_ACTION_PRINT,
  BOBGUI_PRINT_OPERATION_ACTION_PREVIEW,
  BOBGUI_PRINT_OPERATION_ACTION_EXPORT
} BobguiPrintOperationAction;


struct _BobguiPrintOperation
{
  GObject parent_instance;

  /*< private >*/
  BobguiPrintOperationPrivate *priv;
};

/**
 * BobguiPrintOperationClass:
 * @parent_class: The parent class.
 * @done: Signal emitted when the print operation run has finished
 *    doing everything required for printing.
 * @begin_print: Signal emitted after the user has finished changing
 *    print settings in the dialog, before the actual rendering starts.
 * @paginate: Signal emitted after the “begin-print” signal, but
 *    before the actual rendering starts.
 * @request_page_setup: Emitted once for every page that is printed,
 *    to give the application a chance to modify the page setup.
 * @draw_page: Signal emitted for every page that is printed.
 * @end_print: Signal emitted after all pages have been rendered.
 * @status_changed: Emitted at between the various phases of the print
 *    operation.
 * @create_custom_widget: Signal emitted when displaying the print dialog.
 * @custom_widget_apply: Signal emitted right before “begin-print” if
 *    you added a custom widget in the “create-custom-widget” handler.
 * @preview: Signal emitted when a preview is requested from the
 *    native dialog.
 * @update_custom_widget: Emitted after change of selected printer.
 */
struct _BobguiPrintOperationClass
{
  GObjectClass parent_class;

  /*< public >*/

  void     (*done)               (BobguiPrintOperation *operation,
                                  BobguiPrintOperationResult result);
  void     (*begin_print)        (BobguiPrintOperation *operation,
                                  BobguiPrintContext   *context);
  gboolean (*paginate)           (BobguiPrintOperation *operation,
                                  BobguiPrintContext   *context);
  void     (*request_page_setup) (BobguiPrintOperation *operation,
                                  BobguiPrintContext   *context,
                                  int                page_nr,
                                  BobguiPageSetup      *setup);
  void     (*draw_page)          (BobguiPrintOperation *operation,
                                  BobguiPrintContext   *context,
                                  int                page_nr);
  void     (*end_print)          (BobguiPrintOperation *operation,
                                  BobguiPrintContext   *context);
  void     (*status_changed)     (BobguiPrintOperation *operation);

  BobguiWidget *(*create_custom_widget) (BobguiPrintOperation *operation);
  void       (*custom_widget_apply)  (BobguiPrintOperation *operation,
                                      BobguiWidget         *widget);

  gboolean (*preview)        (BobguiPrintOperation        *operation,
                              BobguiPrintOperationPreview *preview,
                              BobguiPrintContext          *context,
                              BobguiWindow                *parent);

  void     (*update_custom_widget) (BobguiPrintOperation *operation,
                                    BobguiWidget         *widget,
                                    BobguiPageSetup      *setup,
                                    BobguiPrintSettings  *settings);

  /*< private >*/

  gpointer padding[8];
};

/**
 * BOBGUI_PRINT_ERROR:
 *
 * The error domain for `BobguiPrintError` errors.
 */
#define BOBGUI_PRINT_ERROR bobgui_print_error_quark ()

/**
 * BobguiPrintError:
 * @BOBGUI_PRINT_ERROR_GENERAL: An unspecified error occurred.
 * @BOBGUI_PRINT_ERROR_INTERNAL_ERROR: An internal error occurred.
 * @BOBGUI_PRINT_ERROR_NOMEM: A memory allocation failed.
 * @BOBGUI_PRINT_ERROR_INVALID_FILE: An error occurred while loading a page setup
 *   or paper size from a key file.
 *
 * Error codes that identify various errors that can occur while
 * using the BOBGUI printing support.
 */
typedef enum
{
  BOBGUI_PRINT_ERROR_GENERAL,
  BOBGUI_PRINT_ERROR_INTERNAL_ERROR,
  BOBGUI_PRINT_ERROR_NOMEM,
  BOBGUI_PRINT_ERROR_INVALID_FILE
} BobguiPrintError;

GDK_AVAILABLE_IN_ALL
GQuark bobgui_print_error_quark (void);

GDK_AVAILABLE_IN_ALL
GType                   bobgui_print_operation_get_type               (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
BobguiPrintOperation *     bobgui_print_operation_new                    (void);
GDK_AVAILABLE_IN_ALL
void                    bobgui_print_operation_set_default_page_setup (BobguiPrintOperation  *op,
                                                                    BobguiPageSetup       *default_page_setup);
GDK_AVAILABLE_IN_ALL
BobguiPageSetup *          bobgui_print_operation_get_default_page_setup (BobguiPrintOperation  *op);
GDK_AVAILABLE_IN_ALL
void                    bobgui_print_operation_set_print_settings     (BobguiPrintOperation  *op,
                                                                    BobguiPrintSettings   *print_settings);
GDK_AVAILABLE_IN_ALL
BobguiPrintSettings *      bobgui_print_operation_get_print_settings     (BobguiPrintOperation  *op);
GDK_AVAILABLE_IN_ALL
void                    bobgui_print_operation_set_job_name           (BobguiPrintOperation  *op,
                                                                    const char         *job_name);
GDK_AVAILABLE_IN_ALL
void                    bobgui_print_operation_set_n_pages            (BobguiPrintOperation  *op,
                                                                    int                 n_pages);
GDK_AVAILABLE_IN_ALL
void                    bobgui_print_operation_set_current_page       (BobguiPrintOperation  *op,
                                                                    int                 current_page);
GDK_AVAILABLE_IN_ALL
void                    bobgui_print_operation_set_use_full_page      (BobguiPrintOperation  *op,
                                                                    gboolean            full_page);
GDK_AVAILABLE_IN_ALL
void                    bobgui_print_operation_set_unit               (BobguiPrintOperation  *op,
                                                                    BobguiUnit             unit);
GDK_AVAILABLE_IN_ALL
void                    bobgui_print_operation_set_export_filename    (BobguiPrintOperation  *op,
                                                                    const char         *filename);
GDK_AVAILABLE_IN_ALL
void                    bobgui_print_operation_set_track_print_status (BobguiPrintOperation  *op,
                                                                    gboolean            track_status);
GDK_AVAILABLE_IN_ALL
void                    bobgui_print_operation_set_show_progress      (BobguiPrintOperation  *op,
                                                                    gboolean            show_progress);
GDK_AVAILABLE_IN_ALL
void                    bobgui_print_operation_set_allow_async        (BobguiPrintOperation  *op,
                                                                    gboolean            allow_async);
GDK_AVAILABLE_IN_ALL
void                    bobgui_print_operation_set_custom_tab_label   (BobguiPrintOperation  *op,
                                                                    const char         *label);
GDK_AVAILABLE_IN_ALL
BobguiPrintOperationResult bobgui_print_operation_run                    (BobguiPrintOperation  *op,
                                                                    BobguiPrintOperationAction action,
                                                                    BobguiWindow          *parent,
                                                                    GError            **error);
GDK_AVAILABLE_IN_ALL
void                    bobgui_print_operation_get_error              (BobguiPrintOperation  *op,
                                                                    GError            **error);
GDK_AVAILABLE_IN_ALL
BobguiPrintStatus          bobgui_print_operation_get_status             (BobguiPrintOperation  *op);
GDK_AVAILABLE_IN_ALL
const char *           bobgui_print_operation_get_status_string      (BobguiPrintOperation  *op);
GDK_AVAILABLE_IN_ALL
gboolean                bobgui_print_operation_is_finished            (BobguiPrintOperation  *op);
GDK_AVAILABLE_IN_ALL
void                    bobgui_print_operation_cancel                 (BobguiPrintOperation  *op);
GDK_AVAILABLE_IN_ALL
void                    bobgui_print_operation_draw_page_finish       (BobguiPrintOperation  *op);
GDK_AVAILABLE_IN_ALL
void                    bobgui_print_operation_set_defer_drawing      (BobguiPrintOperation  *op);
GDK_AVAILABLE_IN_ALL
void                    bobgui_print_operation_set_support_selection  (BobguiPrintOperation  *op,
                                                                    gboolean            support_selection);
GDK_AVAILABLE_IN_ALL
gboolean                bobgui_print_operation_get_support_selection  (BobguiPrintOperation  *op);
GDK_AVAILABLE_IN_ALL
void                    bobgui_print_operation_set_has_selection      (BobguiPrintOperation  *op,
                                                                    gboolean            has_selection);
GDK_AVAILABLE_IN_ALL
gboolean                bobgui_print_operation_get_has_selection      (BobguiPrintOperation  *op);
GDK_AVAILABLE_IN_ALL
void                    bobgui_print_operation_set_embed_page_setup   (BobguiPrintOperation  *op,
                                                                    gboolean            embed);
GDK_AVAILABLE_IN_ALL
gboolean                bobgui_print_operation_get_embed_page_setup   (BobguiPrintOperation  *op);
GDK_AVAILABLE_IN_ALL
int                     bobgui_print_operation_get_n_pages_to_print   (BobguiPrintOperation  *op);

GDK_AVAILABLE_IN_ALL
BobguiPageSetup           *bobgui_print_run_page_setup_dialog            (BobguiWindow          *parent,
                                                                    BobguiPageSetup       *page_setup,
                                                                    BobguiPrintSettings   *settings);

/**
 * BobguiPageSetupDoneFunc:
 * @page_setup: the `BobguiPageSetup` that has been passed to
 *   bobgui_print_run_page_setup_dialog_async()
 * @data: (closure): user data that has been passed to
 *   bobgui_print_run_page_setup_dialog_async()
 *
 * The type of function that is passed to
 * bobgui_print_run_page_setup_dialog_async().
 *
 * This function will be called when the page setup dialog
 * is dismissed, and also serves as destroy notify for @data.
 */
typedef void  (* BobguiPageSetupDoneFunc) (BobguiPageSetup *page_setup,
                                        gpointer      data);

GDK_AVAILABLE_IN_ALL
void                    bobgui_print_run_page_setup_dialog_async      (BobguiWindow            *parent,
                                                                    BobguiPageSetup         *page_setup,
                                                                    BobguiPrintSettings     *settings,
                                                                    BobguiPageSetupDoneFunc  done_cb,
                                                                    gpointer              data);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiPrintOperation, g_object_unref)

G_END_DECLS

