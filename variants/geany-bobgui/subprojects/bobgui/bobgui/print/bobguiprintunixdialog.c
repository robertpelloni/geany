/* BobguiPrintUnixDialog
 * Copyright (C) 2006 John (J5) Palmieri  <johnp@redhat.com>
 * Copyright (C) 2006 Alexander Larsson <alexl@redhat.com>
 * Copyright © 2006, 2007 Christian Persch
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

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <math.h>

#include <glib/gi18n-lib.h>

#include "bobguimarshalers.h"
#include "deprecated/bobguidialogprivate.h"

#include "bobguiprintunixdialog.h"

#include "bobguicustompaperunixdialog.h"
#include "bobguiprintbackendprivate.h"
#include "bobguiprinterprivate.h"
#include "bobguiprinteroptionwidgetprivate.h"
#include "bobguiprintutilsprivate.h"
#include "bobguipagethumbnailprivate.h"


G_GNUC_BEGIN_IGNORE_DEPRECATIONS

/**
 * BobguiPrintUnixDialog:
 *
 * A print dialog for platforms which don’t provide a native
 * print dialog, like Unix.
 *
 * <picture>
 *   <source srcset="printdialog-dark.png" media="(prefers-color-scheme: dark)">
 *   <img alt="An example BobguiPrintUnixDialog" src="printdialog.png">
 * </picture>
 *
 * It can be used very much like any other BOBGUI dialog, at the cost of
 * the portability offered by the high-level printing API with
 * [class@Bobgui.PrintOperation].
 *
 * In order to print something with `BobguiPrintUnixDialog`, you need to
 * use [method@Bobgui.PrintUnixDialog.get_selected_printer] to obtain a
 * [class@Bobgui.Printer] object and use it to construct a [class@Bobgui.PrintJob]
 * using [ctor@Bobgui.PrintJob.new].
 *
 * `BobguiPrintUnixDialog` uses the following response values:
 *
 * - %BOBGUI_RESPONSE_OK: for the “Print” button
 * - %BOBGUI_RESPONSE_APPLY: for the “Preview” button
 * - %BOBGUI_RESPONSE_CANCEL: for the “Cancel” button
 *
 * # BobguiPrintUnixDialog as BobguiBuildable
 *
 * The `BobguiPrintUnixDialog` implementation of the `BobguiBuildable` interface
 * exposes its @notebook internal children with the name “notebook”.
 *
 * An example of a `BobguiPrintUnixDialog` UI definition fragment:
 *
 * ```xml
 * <object class="BobguiPrintUnixDialog" id="dialog1">
 *   <child internal-child="notebook">
 *     <object class="BobguiNotebook" id="notebook">
 *       <child>
 *         <object type="BobguiNotebookPage">
 *           <property name="tab_expand">False</property>
 *           <property name="tab_fill">False</property>
 *           <property name="tab">
 *             <object class="BobguiLabel" id="tablabel">
 *               <property name="label">Tab label</property>
 *             </object>
 *           </property>
 *           <property name="child">
 *             <object class="BobguiLabel" id="tabcontent">
 *               <property name="label">Content on notebook tab</property>
 *             </object>
 *           </property>
 *         </object>
 *       </child>
 *     </object>
 *   </child>
 * </object>
 * ```
 *
 * # CSS nodes
 *
 * `BobguiPrintUnixDialog` has a single CSS node with name window. The style classes
 * dialog and print are added.
 */


#define EXAMPLE_PAGE_AREA_SIZE 110
#define RULER_DISTANCE 7.5
#define RULER_RADIUS 2


static void     bobgui_print_unix_dialog_constructed  (GObject            *object);
static void     bobgui_print_unix_dialog_dispose      (GObject            *object);
static void     bobgui_print_unix_dialog_finalize     (GObject            *object);
static void     bobgui_print_unix_dialog_set_property (GObject            *object,
                                                    guint               prop_id,
                                                    const GValue       *value,
                                                    GParamSpec         *pspec);
static void     bobgui_print_unix_dialog_get_property (GObject            *object,
                                                    guint               prop_id,
                                                    GValue             *value,
                                                    GParamSpec         *pspec);
static void     unschedule_idle_mark_conflicts     (BobguiPrintUnixDialog *dialog);
static void     selected_printer_changed           (BobguiPrintUnixDialog *dialog);
static void     clear_per_printer_ui               (BobguiPrintUnixDialog *dialog);
static void     printer_added_cb                   (GListModel         *model,
                                                    guint               position,
                                                    guint               removed,
                                                    guint               added,
                                                    BobguiPrintUnixDialog *dialog);
static void     printer_status_cb                  (BobguiPrintBackend    *backend,
                                                    BobguiPrinter         *printer,
                                                    BobguiPrintUnixDialog *dialog);
static void     update_collate_icon                (BobguiToggleButton    *toggle_button,
                                                    BobguiPrintUnixDialog *dialog);
static void     error_dialogs                      (BobguiPrintUnixDialog *print_dialog,
						    int                 print_dialog_response_id,
						    gpointer            data);
static gboolean page_range_entry_focus_changed     (BobguiWidget          *entry,
                                                    GParamSpec         *pspec,
                                                    BobguiPrintUnixDialog *dialog);
static void     update_page_range_entry_sensitivity(BobguiWidget          *button,
						    BobguiPrintUnixDialog *dialog);
static void     update_print_at_entry_sensitivity  (BobguiWidget          *button,
						    BobguiPrintUnixDialog *dialog);
static void     update_print_at_option             (BobguiPrintUnixDialog *dialog);
static void     update_dialog_from_capabilities    (BobguiPrintUnixDialog *dialog);
static gboolean is_printer_active                  (gpointer            item,
                                                    gpointer            data);
static  int     default_printer_list_sort_func     (gconstpointer        a,
                                                    gconstpointer        b,
						    gpointer             user_data);
static void     update_number_up_layout            (BobguiPrintUnixDialog  *dialog);
static void     draw_page                          (BobguiDrawingArea      *da,
						    cairo_t             *cr,
                                                    int                  width,
                                                    int                  height,
                                                    gpointer             data);


static gboolean dialog_get_collate                 (BobguiPrintUnixDialog *dialog);
static gboolean dialog_get_reverse                 (BobguiPrintUnixDialog *dialog);
static int      dialog_get_n_copies                (BobguiPrintUnixDialog *dialog);

static gboolean set_active_printer                 (BobguiPrintUnixDialog *dialog,
                                                    const char         *printer_name);
static void redraw_page_layout_preview             (BobguiPrintUnixDialog *dialog);
static GListModel *load_print_backends             (BobguiPrintUnixDialog *dialog);

/* BobguiBuildable */
static void bobgui_print_unix_dialog_buildable_init                    (BobguiBuildableIface *iface);
static GObject *bobgui_print_unix_dialog_buildable_get_internal_child  (BobguiBuildable *buildable,
                                                                     BobguiBuilder   *builder,
                                                                     const char   *childname);

static const char common_paper_sizes[][16] = {
  "na_letter",
  "na_legal",
  "iso_a4",
  "iso_a5",
  "roc_16k",
  "iso_b5",
  "jis_b5",
  "na_number-10",
  "iso_dl",
  "jpn_chou3",
  "na_ledger",
  "iso_a3",
};

/* Keep in line with liststore defined in bobguiprintunixdialog.ui */
enum {
  PAGE_SETUP_LIST_COL_PAGE_SETUP,
  PAGE_SETUP_LIST_COL_IS_SEPARATOR,
  PAGE_SETUP_LIST_N_COLS
};

/* Keep in line with liststore defined in bobguiprintunixdialog.ui */
enum {
  PRINTER_LIST_COL_ICON,
  PRINTER_LIST_COL_NAME,
  PRINTER_LIST_COL_STATE,
  PRINTER_LIST_COL_JOBS,
  PRINTER_LIST_COL_LOCATION,
  PRINTER_LIST_COL_PRINTER_OBJ,
  PRINTER_LIST_N_COLS
};

enum {
  PROP_0,
  PROP_PAGE_SETUP,
  PROP_CURRENT_PAGE,
  PROP_PRINT_SETTINGS,
  PROP_SELECTED_PRINTER,
  PROP_MANUAL_CAPABILITIES,
  PROP_SUPPORT_SELECTION,
  PROP_HAS_SELECTION,
  PROP_EMBED_PAGE_SETUP
};

typedef struct _BobguiPrintUnixDialogClass    BobguiPrintUnixDialogClass;

struct _BobguiPrintUnixDialog
{
  BobguiDialog parent_instance;

  BobguiWidget *notebook;

  BobguiWidget *printer_list;

  BobguiPrintCapabilities manual_capabilities;
  BobguiPrintCapabilities printer_capabilities;

  BobguiPageSetup *page_setup;
  gboolean page_setup_set;
  gboolean embed_page_setup;
  GListStore *page_setup_list;
  GListStore *custom_paper_list;
  GListStore *manage_papers_list;

  gboolean support_selection;
  gboolean has_selection;

  BobguiWidget *all_pages_radio;
  BobguiWidget *current_page_radio;
  BobguiWidget *selection_radio;
  BobguiWidget *range_table;
  BobguiWidget *page_range_radio;
  BobguiWidget *page_range_entry;

  BobguiWidget *copies_spin;
  BobguiWidget *collate_check;
  BobguiWidget *reverse_check;
  BobguiWidget *page_collate_preview;
  BobguiWidget *page_a1;
  BobguiWidget *page_a2;
  BobguiWidget *page_b1;
  BobguiWidget *page_b2;
  BobguiWidget *page_layout_preview;
  BobguiWidget *scale_spin;
  BobguiWidget *page_set_combo;
  BobguiWidget *print_now_radio;
  BobguiWidget *print_at_radio;
  BobguiWidget *print_at_entry;
  BobguiWidget *print_hold_radio;
  BobguiWidget *paper_size_combo;
  BobguiWidget *orientation_combo;
  gboolean internal_page_setup_change;
  gboolean updating_print_at;
  BobguiPrinterOptionWidget *pages_per_sheet;
  BobguiPrinterOptionWidget *duplex;
  BobguiPrinterOptionWidget *paper_type;
  BobguiPrinterOptionWidget *paper_source;
  BobguiPrinterOptionWidget *output_tray;
  BobguiPrinterOptionWidget *job_prio;
  BobguiPrinterOptionWidget *billing_info;
  BobguiPrinterOptionWidget *cover_before;
  BobguiPrinterOptionWidget *cover_after;
  BobguiPrinterOptionWidget *number_up_layout;

  BobguiWidget *conflicts_widget;

  BobguiWidget *job_page;
  BobguiWidget *finishing_table;
  BobguiWidget *finishing_page;
  BobguiWidget *image_quality_table;
  BobguiWidget *image_quality_page;
  BobguiWidget *color_table;
  BobguiWidget *color_page;

  BobguiWidget *advanced_vbox;
  BobguiWidget *advanced_page;

  BobguiWidget *extension_point;

  /* These are set initially on selected printer (either default printer,
   * printer taken from set settings, or user-selected), but when any
   * setting is changed by the user it is cleared.
   */
  BobguiPrintSettings *initial_settings;

  BobguiPrinterOption *number_up_layout_n_option;
  BobguiPrinterOption *number_up_layout_2_option;

  /* This is the initial printer set by set_settings. We look for it in
   * the added printers. We clear this whenever the user manually changes
   * to another printer, when the user changes a setting or when we find
   * this printer.
   */
  char *waiting_for_printer;
  gboolean internal_printer_change;

  GList *print_backends;

  BobguiPrinter *current_printer;
  BobguiPrinter *request_details_printer;
  gulong request_details_tag;
  BobguiPrinterOptionSet *options;
  gulong options_changed_handler;
  gulong mark_conflicts_id;

  char *format_for_printer;

  int current_page;
};

struct _BobguiPrintUnixDialogClass
{
  BobguiDialogClass parent_class;
};

G_DEFINE_TYPE_WITH_CODE (BobguiPrintUnixDialog, bobgui_print_unix_dialog, BOBGUI_TYPE_DIALOG,
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_BUILDABLE,
                                                bobgui_print_unix_dialog_buildable_init))

static BobguiBuildableIface *parent_buildable_iface;

static gboolean
is_default_printer (BobguiPrintUnixDialog *dialog,
                    BobguiPrinter         *printer)
{
  if (dialog->format_for_printer)
    return strcmp (dialog->format_for_printer,
                   bobgui_printer_get_name (printer)) == 0;
 else
   return bobgui_printer_is_default (printer);
}

static const char *css_data = ""
  "page-thumbnail {\n"
  "  border: 1px solid #e6e5e4;\n"
  "  background: white;\n"
  "}\n"
  "page-thumbnail > label {\n"
  "  font-family: Sans;\n"
  "  font-size: 9pt;\n"
  "  color: #2e3436;\n"
  "}\n";

static void
ensure_fallback_style (void)
{
  GdkDisplay *display;
  BobguiCssProvider *provider;

  if (!bobgui_is_initialized ())
    return;

  display = gdk_display_get_default ();
  if (!display)
    return;

  provider = bobgui_css_provider_new ();

  bobgui_css_provider_load_from_string (provider, css_data);

  bobgui_style_context_add_provider_for_display (display,
                                              BOBGUI_STYLE_PROVIDER (provider),
                                              BOBGUI_STYLE_PROVIDER_PRIORITY_FALLBACK);

  g_object_unref (provider);
}

static void
bobgui_print_unix_dialog_class_init (BobguiPrintUnixDialogClass *class)
{
  GObjectClass *object_class;
  BobguiWidgetClass *widget_class;

  ensure_fallback_style ();

  g_type_ensure (BOBGUI_TYPE_PAGE_THUMBNAIL);

  object_class = (GObjectClass *) class;
  widget_class = (BobguiWidgetClass *) class;

  object_class->constructed = bobgui_print_unix_dialog_constructed;
  object_class->finalize = bobgui_print_unix_dialog_finalize;
  object_class->dispose = bobgui_print_unix_dialog_dispose;
  object_class->set_property = bobgui_print_unix_dialog_set_property;
  object_class->get_property = bobgui_print_unix_dialog_get_property;

  /**
   * BobguiPrintUnixDialog:page-setup:
   *
   * The `BobguiPageSetup` object to use.
   */
  g_object_class_install_property (object_class,
                                   PROP_PAGE_SETUP,
                                   g_param_spec_object ("page-setup", NULL, NULL,
                                                        BOBGUI_TYPE_PAGE_SETUP,
                                                        G_PARAM_READWRITE));

  /**
   * BobguiPrintUnixDialog:current-page:
   *
   * The current page in the document.
   */
  g_object_class_install_property (object_class,
                                   PROP_CURRENT_PAGE,
                                   g_param_spec_int ("current-page", NULL, NULL,
                                                     -1,
                                                     G_MAXINT,
                                                     -1,
                                                     G_PARAM_READWRITE));

  /**
   * BobguiPrintUnixDialog:print-settings: (getter get_settings) (setter set_settings)
   *
   * The `BobguiPrintSettings` object used for this dialog.
   */
  g_object_class_install_property (object_class,
                                   PROP_PRINT_SETTINGS,
                                   g_param_spec_object ("print-settings", NULL, NULL,
                                                        BOBGUI_TYPE_PRINT_SETTINGS,
                                                        G_PARAM_READWRITE));

  /**
   * BobguiPrintUnixDialog:selected-printer:
   *
   * The `BobguiPrinter` which is selected.
   */
  g_object_class_install_property (object_class,
                                   PROP_SELECTED_PRINTER,
                                   g_param_spec_object ("selected-printer", NULL, NULL,
                                                        BOBGUI_TYPE_PRINTER,
                                                        G_PARAM_READABLE));

  /**
   * BobguiPrintUnixDialog:manual-capabilities:
   *
   * Capabilities the application can handle.
   */
  g_object_class_install_property (object_class,
                                   PROP_MANUAL_CAPABILITIES,
                                   g_param_spec_flags ("manual-capabilities", NULL, NULL,
                                                       BOBGUI_TYPE_PRINT_CAPABILITIES,
                                                       0,
                                                       G_PARAM_READWRITE));

  /**
   * BobguiPrintUnixDialog:support-selection:
   *
   * Whether the dialog supports selection.
   */
  g_object_class_install_property (object_class,
                                   PROP_SUPPORT_SELECTION,
                                   g_param_spec_boolean ("support-selection", NULL, NULL,
                                                         FALSE,
                                                         G_PARAM_READWRITE));

  /**
   * BobguiPrintUnixDialog:has-selection:
   *
   * Whether the application has a selection.
   */
  g_object_class_install_property (object_class,
                                   PROP_HAS_SELECTION,
                                   g_param_spec_boolean ("has-selection", NULL, NULL,
                                                         FALSE,
                                                         G_PARAM_READWRITE));

   /**
    * BobguiPrintUnixDialog:embed-page-setup:
    *
    * %TRUE if the page setup controls are embedded.
    */
   g_object_class_install_property (object_class,
                                   PROP_EMBED_PAGE_SETUP,
                                   g_param_spec_boolean ("embed-page-setup", NULL, NULL,
                                                         FALSE,
                                                         G_PARAM_READWRITE));

  /* Bind class to template
   */
  bobgui_widget_class_set_template_from_resource (widget_class,
					       "/org/bobgui/libbobgui/print/ui/bobguiprintunixdialog.ui");

  /* BobguiTreeView / BobguiTreeModel */
  bobgui_widget_class_bind_template_child (widget_class, BobguiPrintUnixDialog, printer_list);

  /* General Widgetry */
  bobgui_widget_class_bind_template_child (widget_class, BobguiPrintUnixDialog, notebook);
  bobgui_widget_class_bind_template_child (widget_class, BobguiPrintUnixDialog, all_pages_radio);
  bobgui_widget_class_bind_template_child (widget_class, BobguiPrintUnixDialog, all_pages_radio);
  bobgui_widget_class_bind_template_child (widget_class, BobguiPrintUnixDialog, current_page_radio);
  bobgui_widget_class_bind_template_child (widget_class, BobguiPrintUnixDialog, selection_radio);
  bobgui_widget_class_bind_template_child (widget_class, BobguiPrintUnixDialog, range_table);
  bobgui_widget_class_bind_template_child (widget_class, BobguiPrintUnixDialog, page_range_radio);
  bobgui_widget_class_bind_template_child (widget_class, BobguiPrintUnixDialog, page_range_entry);
  bobgui_widget_class_bind_template_child (widget_class, BobguiPrintUnixDialog, copies_spin);
  bobgui_widget_class_bind_template_child (widget_class, BobguiPrintUnixDialog, collate_check);
  bobgui_widget_class_bind_template_child (widget_class, BobguiPrintUnixDialog, reverse_check);
  bobgui_widget_class_bind_template_child (widget_class, BobguiPrintUnixDialog, page_collate_preview);
  bobgui_widget_class_bind_template_child (widget_class, BobguiPrintUnixDialog, page_a1);
  bobgui_widget_class_bind_template_child (widget_class, BobguiPrintUnixDialog, page_a2);
  bobgui_widget_class_bind_template_child (widget_class, BobguiPrintUnixDialog, page_b1);
  bobgui_widget_class_bind_template_child (widget_class, BobguiPrintUnixDialog, page_b2);
  bobgui_widget_class_bind_template_child (widget_class, BobguiPrintUnixDialog, page_layout_preview);
  bobgui_widget_class_bind_template_child (widget_class, BobguiPrintUnixDialog, scale_spin);
  bobgui_widget_class_bind_template_child (widget_class, BobguiPrintUnixDialog, page_set_combo);
  bobgui_widget_class_bind_template_child (widget_class, BobguiPrintUnixDialog, print_now_radio);
  bobgui_widget_class_bind_template_child (widget_class, BobguiPrintUnixDialog, print_at_radio);
  bobgui_widget_class_bind_template_child (widget_class, BobguiPrintUnixDialog, print_at_entry);
  bobgui_widget_class_bind_template_child (widget_class, BobguiPrintUnixDialog, print_hold_radio);
  bobgui_widget_class_bind_template_child (widget_class, BobguiPrintUnixDialog, paper_size_combo);
  bobgui_widget_class_bind_template_child (widget_class, BobguiPrintUnixDialog, orientation_combo);
  bobgui_widget_class_bind_template_child (widget_class, BobguiPrintUnixDialog, conflicts_widget);
  bobgui_widget_class_bind_template_child (widget_class, BobguiPrintUnixDialog, job_page);
  bobgui_widget_class_bind_template_child (widget_class, BobguiPrintUnixDialog, finishing_table);
  bobgui_widget_class_bind_template_child (widget_class, BobguiPrintUnixDialog, finishing_page);
  bobgui_widget_class_bind_template_child (widget_class, BobguiPrintUnixDialog, image_quality_table);
  bobgui_widget_class_bind_template_child (widget_class, BobguiPrintUnixDialog, image_quality_page);
  bobgui_widget_class_bind_template_child (widget_class, BobguiPrintUnixDialog, color_table);
  bobgui_widget_class_bind_template_child (widget_class, BobguiPrintUnixDialog, color_page);
  bobgui_widget_class_bind_template_child (widget_class, BobguiPrintUnixDialog, advanced_vbox);
  bobgui_widget_class_bind_template_child (widget_class, BobguiPrintUnixDialog, advanced_page);
  bobgui_widget_class_bind_template_child (widget_class, BobguiPrintUnixDialog, extension_point);

  /* BobguiPrinterOptionWidgets... */
  bobgui_widget_class_bind_template_child (widget_class, BobguiPrintUnixDialog, pages_per_sheet);
  bobgui_widget_class_bind_template_child (widget_class, BobguiPrintUnixDialog, duplex);
  bobgui_widget_class_bind_template_child (widget_class, BobguiPrintUnixDialog, paper_type);
  bobgui_widget_class_bind_template_child (widget_class, BobguiPrintUnixDialog, paper_source);
  bobgui_widget_class_bind_template_child (widget_class, BobguiPrintUnixDialog, output_tray);
  bobgui_widget_class_bind_template_child (widget_class, BobguiPrintUnixDialog, job_prio);
  bobgui_widget_class_bind_template_child (widget_class, BobguiPrintUnixDialog, billing_info);
  bobgui_widget_class_bind_template_child (widget_class, BobguiPrintUnixDialog, cover_before);
  bobgui_widget_class_bind_template_child (widget_class, BobguiPrintUnixDialog, cover_after);
  bobgui_widget_class_bind_template_child (widget_class, BobguiPrintUnixDialog, number_up_layout);

  /* Callbacks handled in the UI */
  bobgui_widget_class_bind_template_callback (widget_class, redraw_page_layout_preview);
  bobgui_widget_class_bind_template_callback (widget_class, error_dialogs);
  bobgui_widget_class_bind_template_callback (widget_class, page_range_entry_focus_changed);
  bobgui_widget_class_bind_template_callback (widget_class, update_page_range_entry_sensitivity);
  bobgui_widget_class_bind_template_callback (widget_class, update_print_at_entry_sensitivity);
  bobgui_widget_class_bind_template_callback (widget_class, update_print_at_option);
  bobgui_widget_class_bind_template_callback (widget_class, update_dialog_from_capabilities);
  bobgui_widget_class_bind_template_callback (widget_class, update_collate_icon);
  bobgui_widget_class_bind_template_callback (widget_class, redraw_page_layout_preview);
  bobgui_widget_class_bind_template_callback (widget_class, update_number_up_layout);
  bobgui_widget_class_bind_template_callback (widget_class, redraw_page_layout_preview);
}

/* Returns a toplevel BobguiWindow, or NULL if none */
static BobguiWindow *
get_toplevel (BobguiWidget *widget)
{
  BobguiWidget *toplevel = NULL;

  toplevel = BOBGUI_WIDGET (bobgui_widget_get_root (widget));
  if (BOBGUI_IS_WINDOW (toplevel))
    return BOBGUI_WINDOW (toplevel);
  else
    return NULL;
}

static void
set_busy_cursor (BobguiPrintUnixDialog *dialog,
                 gboolean            busy)
{
  BobguiWidget *widget;
  BobguiWindow *toplevel;

  toplevel = get_toplevel (BOBGUI_WIDGET (dialog));
  widget = BOBGUI_WIDGET (toplevel);

  if (!toplevel || !bobgui_widget_get_realized (widget))
    return;

  if (busy)
    bobgui_widget_set_cursor_from_name (widget, "progress");
  else
    bobgui_widget_set_cursor (widget, NULL);
}

typedef struct {
  GMainLoop *loop;
  int response;
} ConfirmationData;

static void
on_confirmation_dialog_response (BobguiWidget *dialog,
                                 int        response,
                                 gpointer   user_data)
{
  ConfirmationData *data = user_data;

  data->response = response;

  g_main_loop_quit (data->loop);

  bobgui_window_destroy (BOBGUI_WINDOW (dialog));
}

/* This function handles error messages before printing.
 */
static void
error_dialogs (BobguiPrintUnixDialog *dialog,
               int                 dialog_response_id,
               gpointer            data)
{
  if (dialog != NULL && dialog_response_id == BOBGUI_RESPONSE_OK)
    {
      BobguiPrinter *printer = bobgui_print_unix_dialog_get_selected_printer (dialog);

      if (printer != NULL)
        {
          if (dialog->request_details_tag || !bobgui_printer_is_accepting_jobs (printer))
            {
              g_signal_stop_emission_by_name (dialog, "response");
              return;
            }

          /* Shows overwrite confirmation dialog in the case of printing
           * to file which already exists.
           */
          if (bobgui_printer_is_virtual (printer))
            {
              BobguiPrinterOption *option =
                bobgui_printer_option_set_lookup (dialog->options,
                                               "bobgui-main-page-custom-input");

              if (option != NULL &&
                  option->type == BOBGUI_PRINTER_OPTION_TYPE_FILESAVE)
                {
                  GFile *file = g_file_new_for_uri (option->value);

                  if (g_file_query_exists (file, NULL))
                    {
                      BobguiWidget *message_dialog;
                      BobguiWindow *toplevel;
                      char *basename;
                      char *dirname;
                      GFile *parent;

                      toplevel = get_toplevel (BOBGUI_WIDGET (dialog));

                      basename = g_file_get_basename (file);
                      parent = g_file_get_parent (file);
                      dirname = g_file_get_parse_name (parent);
                      g_object_unref (parent);

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
                      message_dialog = bobgui_message_dialog_new (toplevel,
                                                               BOBGUI_DIALOG_MODAL |
                                                               BOBGUI_DIALOG_DESTROY_WITH_PARENT,
                                                               BOBGUI_MESSAGE_QUESTION,
                                                               BOBGUI_BUTTONS_NONE,
                                                               _("A file named “%s” already exists.  Do you want to replace it?"),
                                                               basename);

                      bobgui_message_dialog_format_secondary_text (BOBGUI_MESSAGE_DIALOG (message_dialog),
                                                                _("The file already exists in “%s”.  Replacing it will "
                                                                "overwrite its contents."),
                                                                dirname);

                      bobgui_dialog_add_button (BOBGUI_DIALOG (message_dialog),
                                             _("_Cancel"),
                                             BOBGUI_RESPONSE_CANCEL);
                      bobgui_dialog_add_button (BOBGUI_DIALOG (message_dialog),
                                             _("_Replace"),
                                             BOBGUI_RESPONSE_ACCEPT);
                      bobgui_dialog_set_default_response (BOBGUI_DIALOG (message_dialog),
                                                       BOBGUI_RESPONSE_ACCEPT);
G_GNUC_END_IGNORE_DEPRECATIONS

                      if (bobgui_window_has_group (toplevel))
                        bobgui_window_group_add_window (bobgui_window_get_group (toplevel),
                                                     BOBGUI_WINDOW (message_dialog));

                      bobgui_window_present (BOBGUI_WINDOW (message_dialog));

                      /* Block on the confirmation dialog until we have a response,
                       * so that we can stop the "response" signal emission on the
                       * print dialog
                       */
                      ConfirmationData cdata;

                      cdata.loop = g_main_loop_new (NULL, FALSE);
                      cdata.response = 0;

                      g_signal_connect (message_dialog, "response",
                                        G_CALLBACK (on_confirmation_dialog_response),
                                        &cdata);

                      g_main_loop_run (cdata.loop);
                      g_main_loop_unref (cdata.loop);

                      g_free (dirname);
                      g_free (basename);

                      if (cdata.response != BOBGUI_RESPONSE_ACCEPT)
                        g_signal_stop_emission_by_name (dialog, "response");
                    }

                  g_object_unref (file);
                }
            }
        }
    }
}

static char *
get_printer_key (BobguiPrinter *printer)
{
  return g_strconcat ("", bobgui_printer_get_name (printer), " ", bobgui_printer_get_location (printer), NULL);
}

static void
setup_paper_size_item (BobguiSignalListItemFactory *factory,
                       BobguiListItem              *item)
{
  BobguiWidget *label;

  label = bobgui_label_new ("");
  bobgui_widget_set_halign (label, BOBGUI_ALIGN_START);
  bobgui_list_item_set_child (item, label);
}

static void
bind_paper_size_list_item (BobguiSignalListItemFactory *factory,
                           BobguiListItem              *item,
                           BobguiPrintUnixDialog       *self)
{
  BobguiPageSetup *page_setup;
  BobguiWidget *label;
  guint pos;
  GListModel *papers;
  GListModel *model;
  gpointer first;

  page_setup = bobgui_list_item_get_item (item);
  label = bobgui_list_item_get_child (item);

  pos = bobgui_list_item_get_position (item);
  papers = bobgui_drop_down_get_model (BOBGUI_DROP_DOWN (self->paper_size_combo));
  model = bobgui_flatten_list_model_get_model_for_item (BOBGUI_FLATTEN_LIST_MODEL (papers), pos);
  if (model != G_LIST_MODEL (self->manage_papers_list))
    {
      BobguiPaperSize *paper_size = bobgui_page_setup_get_paper_size (page_setup);
      bobgui_label_set_text (BOBGUI_LABEL (label), bobgui_paper_size_get_display_name (paper_size));
    }
  else
    bobgui_label_set_text (BOBGUI_LABEL (label), _("Manage Custom Sizes…"));

  first = g_list_model_get_item (model, 0);
  g_object_unref (first);
  if (pos != 0 &&
      page_setup == BOBGUI_PAGE_SETUP (first))
    bobgui_widget_add_css_class (bobgui_widget_get_parent (label), "separator");
  else
    bobgui_widget_remove_css_class (bobgui_widget_get_parent (label), "separator");
}

static void
bind_paper_size_item (BobguiSignalListItemFactory *factory,
                      BobguiListItem              *item,
                      BobguiPrintUnixDialog       *self)
{
  BobguiWidget *label;

  bind_paper_size_list_item (factory, item, self);

  label = bobgui_list_item_get_child (item);
  bobgui_widget_remove_css_class (label, "separator-before");
}

static void
bobgui_print_unix_dialog_init (BobguiPrintUnixDialog *dialog)
{
  BobguiWidget *widget;
  GListModel *model;
  GListModel *sorted;
  GListModel *filtered;
  GListModel *selection;
  BobguiSorter *sorter;
  BobguiFilter *filter;
  BobguiStringFilter *filter1;
  BobguiCustomFilter *filter2;
  BobguiListItemFactory *factory;
  GListStore *store;
  GListModel *paper_size_list;
  BobguiPageSetup *page_setup;

  dialog->print_backends = NULL;
  dialog->current_page = -1;
  dialog->number_up_layout_n_option = NULL;
  dialog->number_up_layout_2_option = NULL;

  dialog->page_setup = bobgui_page_setup_new ();
  dialog->page_setup_set = FALSE;
  dialog->embed_page_setup = FALSE;
  dialog->internal_page_setup_change = FALSE;
  dialog->page_setup_list = g_list_store_new (BOBGUI_TYPE_PAGE_SETUP);
  dialog->custom_paper_list = g_list_store_new (BOBGUI_TYPE_PAGE_SETUP);
  dialog->manage_papers_list = g_list_store_new (BOBGUI_TYPE_PAGE_SETUP);
  page_setup = bobgui_page_setup_new ();
  g_list_store_append (dialog->manage_papers_list, page_setup);
  g_object_unref (page_setup);

  dialog->support_selection = FALSE;
  dialog->has_selection = FALSE;

  g_type_ensure (BOBGUI_TYPE_PRINTER);
  g_type_ensure (BOBGUI_TYPE_PRINTER_OPTION);
  g_type_ensure (BOBGUI_TYPE_PRINTER_OPTION_SET);
  g_type_ensure (BOBGUI_TYPE_PRINTER_OPTION_WIDGET);

  bobgui_widget_init_template (BOBGUI_WIDGET (dialog));
  bobgui_widget_add_css_class (BOBGUI_WIDGET (dialog), "print");

  bobgui_dialog_set_use_header_bar_from_setting (BOBGUI_DIALOG (dialog));
  bobgui_dialog_add_buttons (BOBGUI_DIALOG (dialog),
                          _("Pre_view"), BOBGUI_RESPONSE_APPLY,
                          _("_Cancel"), BOBGUI_RESPONSE_CANCEL,
                          _("_Print"), BOBGUI_RESPONSE_OK,
                          NULL);
  bobgui_dialog_set_default_response (BOBGUI_DIALOG (dialog), BOBGUI_RESPONSE_OK);
  widget = bobgui_dialog_get_widget_for_response (BOBGUI_DIALOG (dialog), BOBGUI_RESPONSE_OK);
  bobgui_widget_set_sensitive (widget, FALSE);

  bobgui_widget_set_visible (dialog->selection_radio, FALSE);
  bobgui_widget_set_visible (dialog->conflicts_widget, FALSE);

  factory = bobgui_signal_list_item_factory_new ();
  g_signal_connect (factory, "setup", G_CALLBACK (setup_paper_size_item), dialog);
  g_signal_connect (factory, "bind", G_CALLBACK (bind_paper_size_item), dialog);
  bobgui_drop_down_set_factory (BOBGUI_DROP_DOWN (dialog->paper_size_combo), factory);
  g_object_unref (factory);

  factory = bobgui_signal_list_item_factory_new ();
  g_signal_connect (factory, "setup", G_CALLBACK (setup_paper_size_item), dialog);
  g_signal_connect (factory, "bind", G_CALLBACK (bind_paper_size_list_item), dialog);
  bobgui_drop_down_set_list_factory (BOBGUI_DROP_DOWN (dialog->paper_size_combo), factory);
  g_object_unref (factory);

  store = g_list_store_new (G_TYPE_LIST_MODEL);
  g_list_store_append (store, dialog->page_setup_list);
  g_list_store_append (store, dialog->custom_paper_list);
  g_list_store_append (store, dialog->manage_papers_list);
  paper_size_list = G_LIST_MODEL (bobgui_flatten_list_model_new (G_LIST_MODEL (store)));
  bobgui_drop_down_set_model (BOBGUI_DROP_DOWN (dialog->paper_size_combo), paper_size_list);
  g_object_unref (paper_size_list);

  /* Load backends */
  model = load_print_backends (dialog);
  sorter = BOBGUI_SORTER (bobgui_custom_sorter_new (default_printer_list_sort_func, NULL, NULL));
  sorted = G_LIST_MODEL (bobgui_sort_list_model_new (model, sorter));

  filter = BOBGUI_FILTER (bobgui_every_filter_new ());

  filter1 = bobgui_string_filter_new (
                bobgui_cclosure_expression_new (G_TYPE_STRING,
                                             NULL, 0, NULL,
                                             G_CALLBACK (get_printer_key),
                                             NULL, NULL));
  bobgui_string_filter_set_match_mode (filter1, BOBGUI_STRING_FILTER_MATCH_MODE_SUBSTRING);
  bobgui_string_filter_set_ignore_case (filter1, TRUE);
  bobgui_multi_filter_append (BOBGUI_MULTI_FILTER (filter), BOBGUI_FILTER (filter1));

  filter2 = bobgui_custom_filter_new (is_printer_active, dialog, NULL);
  bobgui_multi_filter_append (BOBGUI_MULTI_FILTER (filter), BOBGUI_FILTER (filter2));

  filtered = G_LIST_MODEL (bobgui_filter_list_model_new (sorted, filter));

  selection = G_LIST_MODEL (bobgui_single_selection_new (NULL));
  bobgui_single_selection_set_autoselect (BOBGUI_SINGLE_SELECTION (selection), FALSE);

  bobgui_single_selection_set_model (BOBGUI_SINGLE_SELECTION (selection), filtered);

  g_object_unref (filtered);

  bobgui_column_view_set_model (BOBGUI_COLUMN_VIEW (dialog->printer_list), BOBGUI_SELECTION_MODEL (selection));

  g_signal_connect (selection, "items-changed", G_CALLBACK (printer_added_cb), dialog);
  g_signal_connect_swapped (selection, "notify::selected", G_CALLBACK (selected_printer_changed), dialog);

  g_object_unref (selection);

  bobgui_print_load_custom_papers (dialog->custom_paper_list);

  bobgui_drawing_area_set_draw_func (BOBGUI_DRAWING_AREA (dialog->page_layout_preview),
                                  draw_page,
                                  dialog, NULL);
}

static void
bobgui_print_unix_dialog_constructed (GObject *object)
{
  gboolean use_header;

  G_OBJECT_CLASS (bobgui_print_unix_dialog_parent_class)->constructed (object);

  g_object_get (object, "use-header-bar", &use_header, NULL);
  if (use_header)
    {
       /* Reorder the preview button */
       BobguiWidget *button, *parent;
       button = bobgui_dialog_get_widget_for_response (BOBGUI_DIALOG (object), BOBGUI_RESPONSE_APPLY);
       g_object_ref (button);
       parent = bobgui_widget_get_ancestor (button, BOBGUI_TYPE_HEADER_BAR);
       bobgui_box_remove (BOBGUI_BOX (bobgui_widget_get_parent (button)), button);
       bobgui_header_bar_pack_end (BOBGUI_HEADER_BAR (parent), button);
       g_object_unref (button);
    }

  update_dialog_from_capabilities (BOBGUI_PRINT_UNIX_DIALOG (object));
}

static void
bobgui_print_unix_dialog_dispose (GObject *object)
{
  BobguiPrintUnixDialog *dialog = BOBGUI_PRINT_UNIX_DIALOG (object);

  /* Make sure we don't destroy custom widgets owned by the backends */
  clear_per_printer_ui (dialog);

  G_OBJECT_CLASS (bobgui_print_unix_dialog_parent_class)->dispose (object);
}

static void
disconnect_printer_details_request (BobguiPrintUnixDialog *dialog,
                                    gboolean            details_failed)
{
  if (dialog->request_details_tag)
    {
      g_signal_handler_disconnect (dialog->request_details_printer,
                                   dialog->request_details_tag);
      dialog->request_details_tag = 0;
      set_busy_cursor (dialog, FALSE);
      if (details_failed)
        bobgui_printer_set_state_message (dialog->request_details_printer, _("Getting printer information failed"));
      g_clear_object (&dialog->request_details_printer);
    }
}

static void
bobgui_print_unix_dialog_finalize (GObject *object)
{
  BobguiPrintUnixDialog *dialog = BOBGUI_PRINT_UNIX_DIALOG (object);
  GList *iter;

  unschedule_idle_mark_conflicts (dialog);
  disconnect_printer_details_request (dialog, FALSE);

  g_clear_object (&dialog->current_printer);
  g_clear_object (&dialog->options);

  if (dialog->number_up_layout_2_option)
    {
      dialog->number_up_layout_2_option->choices[0] = NULL;
      dialog->number_up_layout_2_option->choices[1] = NULL;
      g_free (dialog->number_up_layout_2_option->choices_display[0]);
      g_free (dialog->number_up_layout_2_option->choices_display[1]);
      dialog->number_up_layout_2_option->choices_display[0] = NULL;
      dialog->number_up_layout_2_option->choices_display[1] = NULL;
      g_object_unref (dialog->number_up_layout_2_option);
      dialog->number_up_layout_2_option = NULL;
    }

  g_clear_object (&dialog->number_up_layout_n_option);
  g_clear_object (&dialog->page_setup);
  g_clear_object (&dialog->initial_settings);
  g_clear_pointer (&dialog->waiting_for_printer, (GDestroyNotify)g_free);
  g_clear_pointer (&dialog->format_for_printer, (GDestroyNotify)g_free);

  for (iter = dialog->print_backends; iter != NULL; iter = iter->next)
    bobgui_print_backend_destroy (BOBGUI_PRINT_BACKEND (iter->data));
  g_list_free_full (dialog->print_backends, g_object_unref);
  dialog->print_backends = NULL;

  g_clear_object (&dialog->page_setup_list);
  g_clear_object (&dialog->custom_paper_list);
  g_clear_object (&dialog->manage_papers_list);

  G_OBJECT_CLASS (bobgui_print_unix_dialog_parent_class)->finalize (object);
}

static void
bobgui_print_unix_dialog_buildable_init (BobguiBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);

  iface->get_internal_child = bobgui_print_unix_dialog_buildable_get_internal_child;
}

static GObject *
bobgui_print_unix_dialog_buildable_get_internal_child (BobguiBuildable *buildable,
                                                    BobguiBuilder   *builder,
                                                    const char   *childname)
{
  BobguiPrintUnixDialog *dialog = BOBGUI_PRINT_UNIX_DIALOG (buildable);

  if (strcmp (childname, "notebook") == 0)
    return G_OBJECT (dialog->notebook);

  return parent_buildable_iface->get_internal_child (buildable, builder, childname);
}

static void
printer_status_cb (BobguiPrintBackend    *backend,
                   BobguiPrinter         *printer,
                   BobguiPrintUnixDialog *dialog)
{
  GListModel *model;

  /* When the pause state change then we need to update sensitive property
   * of BOBGUI_RESPONSE_OK button inside of selected_printer_changed function.
   */
  selected_printer_changed (dialog);

  model = G_LIST_MODEL (bobgui_column_view_get_model (BOBGUI_COLUMN_VIEW (dialog->printer_list)));

  if (bobgui_print_backend_printer_list_is_done (backend) &&
      bobgui_printer_is_default (printer) &&
      bobgui_single_selection_get_selected (BOBGUI_SINGLE_SELECTION (model)) == BOBGUI_INVALID_LIST_POSITION)
    set_active_printer (dialog, bobgui_printer_get_name (printer));
}

static void
printer_added_cb (GListModel         *model,
                  guint               position,
                  guint               removed,
                  guint               added,
                  BobguiPrintUnixDialog *dialog)
{
  guint i;

  for (i = position; i < position + added; i++)
    {
      BobguiPrinter *printer = g_list_model_get_item (model, i);

      if (dialog->waiting_for_printer != NULL &&
          strcmp (bobgui_printer_get_name (printer), dialog->waiting_for_printer) == 0)
        {
          bobgui_single_selection_set_selected (BOBGUI_SINGLE_SELECTION (model), i);
          g_free (dialog->waiting_for_printer);
          dialog->waiting_for_printer = NULL;
          g_object_unref (printer);
          return;
        }
      else if (is_default_printer (dialog, printer) &&
               bobgui_single_selection_get_selected (BOBGUI_SINGLE_SELECTION (model)) == BOBGUI_INVALID_LIST_POSITION)
        {
          bobgui_single_selection_set_selected (BOBGUI_SINGLE_SELECTION (model), i);
          g_object_unref (printer);
          return;
        }

      g_object_unref (printer);
    }
}

static GListModel *
load_print_backends (BobguiPrintUnixDialog *dialog)
{
  GList *node;
  GListStore *lists;

  lists = g_list_store_new (G_TYPE_LIST_MODEL);

  if (g_module_supported ())
    dialog->print_backends = bobgui_print_backend_load_modules ();

  for (node = dialog->print_backends; node != NULL; node = node->next)
    {
      BobguiPrintBackend *backend = node->data;

      g_signal_connect_object (backend, "printer-status-changed",
                               G_CALLBACK (printer_status_cb), G_OBJECT (dialog), 0);
      g_list_store_append (lists, bobgui_print_backend_get_printers (backend));
    }

  return G_LIST_MODEL (bobgui_flatten_list_model_new (G_LIST_MODEL (lists)));
}

static void
bobgui_print_unix_dialog_set_property (GObject      *object,
                                    guint         prop_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)

{
  BobguiPrintUnixDialog *dialog = BOBGUI_PRINT_UNIX_DIALOG (object);

  switch (prop_id)
    {
    case PROP_PAGE_SETUP:
      bobgui_print_unix_dialog_set_page_setup (dialog, g_value_get_object (value));
      break;
    case PROP_CURRENT_PAGE:
      bobgui_print_unix_dialog_set_current_page (dialog, g_value_get_int (value));
      break;
    case PROP_PRINT_SETTINGS:
      bobgui_print_unix_dialog_set_settings (dialog, g_value_get_object (value));
      break;
    case PROP_MANUAL_CAPABILITIES:
      bobgui_print_unix_dialog_set_manual_capabilities (dialog, g_value_get_flags (value));
      break;
    case PROP_SUPPORT_SELECTION:
      bobgui_print_unix_dialog_set_support_selection (dialog, g_value_get_boolean (value));
      break;
    case PROP_HAS_SELECTION:
      bobgui_print_unix_dialog_set_has_selection (dialog, g_value_get_boolean (value));
      break;
    case PROP_EMBED_PAGE_SETUP:
      bobgui_print_unix_dialog_set_embed_page_setup (dialog, g_value_get_boolean (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_print_unix_dialog_get_property (GObject    *object,
                                    guint       prop_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  BobguiPrintUnixDialog *dialog = BOBGUI_PRINT_UNIX_DIALOG (object);

  switch (prop_id)
    {
    case PROP_PAGE_SETUP:
      g_value_set_object (value, dialog->page_setup);
      break;
    case PROP_CURRENT_PAGE:
      g_value_set_int (value, dialog->current_page);
      break;
    case PROP_PRINT_SETTINGS:
      g_value_take_object (value, bobgui_print_unix_dialog_get_settings (dialog));
      break;
    case PROP_SELECTED_PRINTER:
      g_value_set_object (value, dialog->current_printer);
      break;
    case PROP_MANUAL_CAPABILITIES:
      g_value_set_flags (value, dialog->manual_capabilities);
      break;
    case PROP_SUPPORT_SELECTION:
      g_value_set_boolean (value, dialog->support_selection);
      break;
    case PROP_HAS_SELECTION:
      g_value_set_boolean (value, dialog->has_selection);
      break;
    case PROP_EMBED_PAGE_SETUP:
      g_value_set_boolean (value, dialog->embed_page_setup);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static gboolean
is_printer_active (gpointer item, gpointer data)
{
  BobguiPrinter *printer = item;
  BobguiPrintUnixDialog *dialog = data;
  gboolean result;

  result = bobgui_printer_is_active (printer);

  if (result &&
      dialog->manual_capabilities & (BOBGUI_PRINT_CAPABILITY_GENERATE_PDF |
                                   BOBGUI_PRINT_CAPABILITY_GENERATE_PS))
    {
       /* Check that the printer can handle at least one of the data
        * formats that the application supports.
        */
       result = ((dialog->manual_capabilities & BOBGUI_PRINT_CAPABILITY_GENERATE_PDF) &&
                 bobgui_printer_accepts_pdf (printer)) ||
                ((dialog->manual_capabilities & BOBGUI_PRINT_CAPABILITY_GENERATE_PS) &&
                 bobgui_printer_accepts_ps (printer));
    }

  return result;
}

static int
default_printer_list_sort_func (gconstpointer a,
                                gconstpointer b,
                                gpointer      user_data)
{
  BobguiPrinter *a_printer = (gpointer)a;
  BobguiPrinter *b_printer = (gpointer)b;
  const char *a_name;
  const char *b_name;

  if (a_printer == NULL && b_printer == NULL)
    return 0;
  else if (a_printer == NULL)
   return 1;
  else if (b_printer == NULL)
   return -1;

  if (bobgui_printer_is_virtual (a_printer) && bobgui_printer_is_virtual (b_printer))
    return 0;
  else if (bobgui_printer_is_virtual (a_printer) && !bobgui_printer_is_virtual (b_printer))
    return -1;
  else if (!bobgui_printer_is_virtual (a_printer) && bobgui_printer_is_virtual (b_printer))
    return 1;

  a_name = bobgui_printer_get_name (a_printer);
  b_name = bobgui_printer_get_name (b_printer);

  if (a_name == NULL && b_name == NULL)
    return  0;
  else if (a_name == NULL && b_name != NULL)
    return  1;
  else if (a_name != NULL && b_name == NULL)
    return -1;

  return g_ascii_strcasecmp (a_name, b_name);
}

static BobguiWidget *
wrap_in_frame (const char *label,
               BobguiWidget   *child)
{
  BobguiWidget *box, *label_widget;
  char *bold_text;

  label_widget = bobgui_label_new (NULL);
  bobgui_widget_set_halign (label_widget, BOBGUI_ALIGN_START);
  bobgui_widget_set_valign (label_widget, BOBGUI_ALIGN_CENTER);

  bold_text = g_markup_printf_escaped ("<b>%s</b>", label);
  bobgui_label_set_markup (BOBGUI_LABEL (label_widget), bold_text);
  g_free (bold_text);

  box = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 6);
  bobgui_box_append (BOBGUI_BOX (box), label_widget);

  bobgui_widget_set_margin_start (child, 12);
  bobgui_widget_set_halign (child, BOBGUI_ALIGN_FILL);
  bobgui_widget_set_valign (child, BOBGUI_ALIGN_FILL);

  bobgui_box_append (BOBGUI_BOX (box), child);

  return box;
}

static gboolean
setup_option (BobguiPrintUnixDialog     *dialog,
              const char             *option_name,
              BobguiPrinterOptionWidget *widget)
{
  BobguiPrinterOption *option;

  option = bobgui_printer_option_set_lookup (dialog->options, option_name);
  bobgui_printer_option_widget_set_source (widget, option);

  return option != NULL;
}

static void
add_option_to_extension_point (BobguiPrinterOption *option,
                               gpointer          data)
{
  BobguiWidget *extension_point = data;
  BobguiWidget *widget;

  widget = bobgui_printer_option_widget_new (option);

  if (bobgui_printer_option_widget_has_external_label (BOBGUI_PRINTER_OPTION_WIDGET (widget)))
    {
      BobguiWidget *label, *hbox;

      bobgui_widget_set_valign (widget, BOBGUI_ALIGN_BASELINE_FILL);

      label = bobgui_printer_option_widget_get_external_label (BOBGUI_PRINTER_OPTION_WIDGET (widget));
      bobgui_widget_set_visible (label, TRUE);
      bobgui_widget_set_halign (label, BOBGUI_ALIGN_START);
      bobgui_widget_set_valign (label, BOBGUI_ALIGN_BASELINE_FILL);
      bobgui_label_set_mnemonic_widget (BOBGUI_LABEL (label), widget);

      hbox = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 12);
      bobgui_widget_set_valign (hbox, BOBGUI_ALIGN_BASELINE_FILL);
      bobgui_box_append (BOBGUI_BOX (hbox), label);
      bobgui_box_append (BOBGUI_BOX (hbox), widget);

      bobgui_box_append (BOBGUI_BOX (extension_point), hbox);
    }
  else
    bobgui_box_append (BOBGUI_BOX (extension_point), widget);
}

static int
grid_rows (BobguiGrid *table)
{
  int t0, t1, l, t, w, h;
  BobguiWidget *c;
  gboolean first;

  t0 = t1 = 0;
  for (c = bobgui_widget_get_first_child (BOBGUI_WIDGET (table)), first = TRUE;
       c != NULL;
       c  = bobgui_widget_get_next_sibling (BOBGUI_WIDGET (c)), first = FALSE)
    {
      bobgui_grid_query_child (table, c, &l, &t, &w, &h);
      if (first)
        {
          t0 = t;
          t1 = t + h;
        }
      else
        {
          if (t < t0)
            t0 = t;
          if (t + h > t1)
            t1 = t + h;
        }
    }

  return t1 - t0;
}

static void
add_option_to_table (BobguiPrinterOption *option,
                     gpointer          user_data)
{
  BobguiGrid *table;
  BobguiWidget *label, *widget;
  guint row;

  table = BOBGUI_GRID (user_data);

  if (g_str_has_prefix (option->name, "bobgui-"))
    return;

  row = grid_rows (table);

  widget = bobgui_printer_option_widget_new (option);

  if (bobgui_printer_option_widget_has_external_label (BOBGUI_PRINTER_OPTION_WIDGET (widget)))
    {
      label = bobgui_printer_option_widget_get_external_label (BOBGUI_PRINTER_OPTION_WIDGET (widget));
      bobgui_widget_set_visible (label, TRUE);

      bobgui_widget_set_halign (label, BOBGUI_ALIGN_START);
      bobgui_widget_set_valign (label, BOBGUI_ALIGN_CENTER);
      bobgui_label_set_mnemonic_widget (BOBGUI_LABEL (label), widget);

      bobgui_grid_attach (table, label, 0, row - 1, 1, 1);
      bobgui_grid_attach (table, widget, 1, row - 1, 1, 1);
    }
  else
    bobgui_grid_attach (table, widget, 0, row - 1, 2, 1);
}

static void
setup_page_table (BobguiPrinterOptionSet *options,
                  const char          *group,
                  BobguiWidget           *table,
                  BobguiWidget           *page)
{
  int nrows;

  bobgui_printer_option_set_foreach_in_group (options, group,
                                           add_option_to_table,
                                           table);

  nrows = grid_rows (BOBGUI_GRID (table));
  bobgui_widget_set_visible (page, nrows > 0);
}

static void
update_print_at_option (BobguiPrintUnixDialog *dialog)
{
  BobguiPrinterOption *option;

  option = bobgui_printer_option_set_lookup (dialog->options, "bobgui-print-time");

  if (option == NULL)
    return;

  if (dialog->updating_print_at)
    return;

  if (bobgui_check_button_get_active (BOBGUI_CHECK_BUTTON (dialog->print_at_radio)))
    bobgui_printer_option_set (option, "at");
  else if (bobgui_check_button_get_active (BOBGUI_CHECK_BUTTON (dialog->print_hold_radio)))
    bobgui_printer_option_set (option, "on-hold");
  else
    bobgui_printer_option_set (option, "now");

  option = bobgui_printer_option_set_lookup (dialog->options, "bobgui-print-time-text");
  if (option != NULL)
    {
      const char *text;

      text = bobgui_editable_get_text (BOBGUI_EDITABLE (dialog->print_at_entry));
      bobgui_printer_option_set (option, text);
    }
}


static gboolean
setup_print_at (BobguiPrintUnixDialog *dialog)
{
  BobguiPrinterOption *option;

  option = bobgui_printer_option_set_lookup (dialog->options, "bobgui-print-time");

  if (option == NULL)
    {
      bobgui_check_button_set_active (BOBGUI_CHECK_BUTTON (dialog->print_now_radio), TRUE);
      bobgui_widget_set_sensitive (dialog->print_at_radio, FALSE);
      bobgui_widget_set_sensitive (dialog->print_at_entry, FALSE);
      bobgui_widget_set_sensitive (dialog->print_hold_radio, FALSE);
      bobgui_editable_set_text (BOBGUI_EDITABLE (dialog->print_at_entry), "");
      return FALSE;
    }

  dialog->updating_print_at = TRUE;

  bobgui_widget_set_sensitive (dialog->print_at_entry, FALSE);
  bobgui_widget_set_sensitive (dialog->print_at_radio,
                            bobgui_printer_option_has_choice (option, "at"));

  bobgui_widget_set_sensitive (dialog->print_hold_radio,
                            bobgui_printer_option_has_choice (option, "on-hold"));

  update_print_at_option (dialog);

  if (strcmp (option->value, "at") == 0)
    bobgui_check_button_set_active (BOBGUI_CHECK_BUTTON (dialog->print_at_radio), TRUE);
  else if (strcmp (option->value, "on-hold") == 0)
    bobgui_check_button_set_active (BOBGUI_CHECK_BUTTON (dialog->print_hold_radio), TRUE);
  else
    bobgui_check_button_set_active (BOBGUI_CHECK_BUTTON (dialog->print_now_radio), TRUE);

  option = bobgui_printer_option_set_lookup (dialog->options, "bobgui-print-time-text");
  if (option != NULL)
    bobgui_editable_set_text (BOBGUI_EDITABLE (dialog->print_at_entry), option->value);

  dialog->updating_print_at = FALSE;

  return TRUE;
}

static void
update_dialog_from_settings (BobguiPrintUnixDialog *dialog)
{
  GList *groups, *l;
  char *group;
  BobguiWidget *table, *frame;
  gboolean has_advanced, has_job;
  guint nrows;
  BobguiWidget *child;

  if (dialog->current_printer == NULL)
    {
       clear_per_printer_ui (dialog);
       bobgui_widget_set_visible (dialog->job_page, FALSE);
       bobgui_widget_set_visible (dialog->advanced_page, FALSE);
       bobgui_widget_set_visible (dialog->image_quality_page, FALSE);
       bobgui_widget_set_visible (dialog->finishing_page, FALSE);
       bobgui_widget_set_visible (dialog->color_page, FALSE);
       bobgui_dialog_set_response_sensitive (BOBGUI_DIALOG (dialog), BOBGUI_RESPONSE_OK, FALSE);

       return;
    }

  setup_option (dialog, "bobgui-n-up", dialog->pages_per_sheet);
  setup_option (dialog, "bobgui-n-up-layout", dialog->number_up_layout);
  setup_option (dialog, "bobgui-duplex", dialog->duplex);
  setup_option (dialog, "bobgui-paper-type", dialog->paper_type);
  setup_option (dialog, "bobgui-paper-source", dialog->paper_source);
  setup_option (dialog, "bobgui-output-tray", dialog->output_tray);

  has_job = FALSE;
  has_job |= setup_option (dialog, "bobgui-job-prio", dialog->job_prio);
  has_job |= setup_option (dialog, "bobgui-billing-info", dialog->billing_info);
  has_job |= setup_option (dialog, "bobgui-cover-before", dialog->cover_before);
  has_job |= setup_option (dialog, "bobgui-cover-after", dialog->cover_after);
  has_job |= setup_print_at (dialog);

  bobgui_widget_set_visible (dialog->job_page, has_job);

  setup_page_table (dialog->options,
                    "ImageQualityPage",
                    dialog->image_quality_table,
                    dialog->image_quality_page);

  setup_page_table (dialog->options,
                    "FinishingPage",
                    dialog->finishing_table,
                    dialog->finishing_page);

  setup_page_table (dialog->options,
                    "ColorPage",
                    dialog->color_table,
                    dialog->color_page);

  bobgui_printer_option_set_foreach_in_group (dialog->options,
                                           "BobguiPrintDialogExtension",
                                           add_option_to_extension_point,
                                           dialog->extension_point);

  /* A bit of a hack, keep the last option flush right.
   * This keeps the file format radios from moving as the
   * filename changes.
   */
  child = bobgui_widget_get_last_child (dialog->extension_point);
  if (child && child != bobgui_widget_get_first_child (dialog->extension_point))
    bobgui_widget_set_halign (child, BOBGUI_ALIGN_END);

  /* Put the rest of the groups in the advanced page */
  groups = bobgui_printer_option_set_get_groups (dialog->options);

  has_advanced = FALSE;
  for (l = groups; l != NULL; l = l->next)
    {
      group = l->data;

      if (group == NULL)
        continue;

      if (strcmp (group, "ImageQualityPage") == 0 ||
          strcmp (group, "ColorPage") == 0 ||
          strcmp (group, "FinishingPage") == 0 ||
          strcmp (group, "BobguiPrintDialogExtension") == 0)
        continue;

      table = bobgui_grid_new ();
      bobgui_grid_set_row_spacing (BOBGUI_GRID (table), 6);
      bobgui_grid_set_column_spacing (BOBGUI_GRID (table), 12);

      bobgui_printer_option_set_foreach_in_group (dialog->options,
                                               group,
                                               add_option_to_table,
                                               table);

      nrows = grid_rows (BOBGUI_GRID (table));
      if (nrows == 0)
        {
          g_object_unref (g_object_ref_sink (table));
        }
      else
        {
          has_advanced = TRUE;
          frame = wrap_in_frame (group, table);
          bobgui_box_append (BOBGUI_BOX (dialog->advanced_vbox), frame);
        }
    }

  bobgui_widget_set_visible (dialog->advanced_page, has_advanced);

  g_list_free_full (groups, g_free);
}

static void
update_dialog_from_capabilities (BobguiPrintUnixDialog *dialog)
{
  BobguiPrintCapabilities caps;
  gboolean can_collate;
  const char *copies;
  BobguiWidget *button;

  copies = bobgui_editable_get_text (BOBGUI_EDITABLE (dialog->copies_spin));
  can_collate = (*copies != '\0' && atoi (copies) > 1);

  caps = dialog->manual_capabilities | dialog->printer_capabilities;

  bobgui_widget_set_sensitive (dialog->page_set_combo,
                            caps & BOBGUI_PRINT_CAPABILITY_PAGE_SET);
  bobgui_widget_set_sensitive (dialog->copies_spin,
                            caps & BOBGUI_PRINT_CAPABILITY_COPIES);
  bobgui_widget_set_sensitive (dialog->collate_check,
                            can_collate &&
                            (caps & BOBGUI_PRINT_CAPABILITY_COLLATE));
  bobgui_widget_set_sensitive (dialog->reverse_check,
                            caps & BOBGUI_PRINT_CAPABILITY_REVERSE);
  bobgui_widget_set_sensitive (dialog->scale_spin,
                            caps & BOBGUI_PRINT_CAPABILITY_SCALE);
  bobgui_widget_set_sensitive (BOBGUI_WIDGET (dialog->pages_per_sheet),
                            caps & BOBGUI_PRINT_CAPABILITY_NUMBER_UP);

  button = bobgui_dialog_get_widget_for_response (BOBGUI_DIALOG (dialog), BOBGUI_RESPONSE_APPLY);
  bobgui_widget_set_visible (button, (caps & BOBGUI_PRINT_CAPABILITY_PREVIEW) != 0);

  update_collate_icon (NULL, dialog);
}

static gboolean
page_setup_is_equal (BobguiPageSetup *a,
                     BobguiPageSetup *b)
{
  return
    bobgui_paper_size_is_equal (bobgui_page_setup_get_paper_size (a),
                             bobgui_page_setup_get_paper_size (b)) &&
    bobgui_page_setup_get_top_margin (a, BOBGUI_UNIT_MM) == bobgui_page_setup_get_top_margin (b, BOBGUI_UNIT_MM) &&
    bobgui_page_setup_get_bottom_margin (a, BOBGUI_UNIT_MM) == bobgui_page_setup_get_bottom_margin (b, BOBGUI_UNIT_MM) &&
    bobgui_page_setup_get_left_margin (a, BOBGUI_UNIT_MM) == bobgui_page_setup_get_left_margin (b, BOBGUI_UNIT_MM) &&
    bobgui_page_setup_get_right_margin (a, BOBGUI_UNIT_MM) == bobgui_page_setup_get_right_margin (b, BOBGUI_UNIT_MM);
}

static gboolean
page_setup_is_same_size (BobguiPageSetup *a,
                         BobguiPageSetup *b)
{
  return bobgui_paper_size_is_equal (bobgui_page_setup_get_paper_size (a),
                                  bobgui_page_setup_get_paper_size (b));
}

static gboolean
set_paper_size (BobguiPrintUnixDialog *dialog,
                BobguiPageSetup       *page_setup,
                gboolean            size_only,
                gboolean            add_item)
{
  GListModel *model;
  BobguiPageSetup *list_page_setup;
  guint i;

  if (!dialog->internal_page_setup_change)
    return TRUE;

  if (page_setup == NULL)
    return FALSE;

  model = bobgui_drop_down_get_model (BOBGUI_DROP_DOWN (dialog->paper_size_combo));
  for (i = 0; i < g_list_model_get_n_items (model); i++)
    {
      list_page_setup = g_list_model_get_item (model, i);
      if (list_page_setup == NULL)
        continue;

      if ((size_only && page_setup_is_same_size (page_setup, list_page_setup)) ||
          (!size_only && page_setup_is_equal (page_setup, list_page_setup)))
        {
          bobgui_drop_down_set_selected (BOBGUI_DROP_DOWN (dialog->paper_size_combo), i);
          bobgui_drop_down_set_selected (BOBGUI_DROP_DOWN (dialog->orientation_combo),
                                      bobgui_page_setup_get_orientation (page_setup));
          g_object_unref (list_page_setup);
          return TRUE;
        }

      g_object_unref (list_page_setup);
    }

  if (add_item)
    {
      i = g_list_model_get_n_items (model);
      g_list_store_append (dialog->page_setup_list, page_setup);
      bobgui_drop_down_set_selected (BOBGUI_DROP_DOWN (dialog->paper_size_combo), i);
      bobgui_drop_down_set_selected (BOBGUI_DROP_DOWN (dialog->orientation_combo),
                                  bobgui_page_setup_get_orientation (page_setup));
      return TRUE;
    }

  return FALSE;
}

static void
fill_custom_paper_sizes (BobguiPrintUnixDialog *dialog)
{
  g_list_store_remove_all (dialog->custom_paper_list);
  bobgui_print_load_custom_papers (dialog->custom_paper_list);
}

static void
fill_paper_sizes (BobguiPrintUnixDialog *dialog,
                  BobguiPrinter         *printer)
{
  GList *list, *l;
  BobguiPageSetup *page_setup;
  BobguiPaperSize *paper_size;
  int i;

  g_list_store_remove_all (dialog->page_setup_list);

  if (printer == NULL || (list = bobgui_printer_list_papers (printer)) == NULL)
    {
      for (i = 0; i < G_N_ELEMENTS (common_paper_sizes); i++)
        {
          page_setup = bobgui_page_setup_new ();
          paper_size = bobgui_paper_size_new (common_paper_sizes[i]);
          bobgui_page_setup_set_paper_size_and_default_margins (page_setup, paper_size);
          bobgui_paper_size_free (paper_size);
          g_list_store_append (dialog->page_setup_list, page_setup);
          g_object_unref (page_setup);
        }
    }
  else
    {
      for (l = list; l != NULL; l = l->next)
        {
          page_setup = l->data;
          g_list_store_append (dialog->page_setup_list, page_setup);
          g_object_unref (page_setup);
        }
      g_list_free (list);
    }
}

static void
update_paper_sizes (BobguiPrintUnixDialog *dialog)
{
  BobguiPageSetup *current_page_setup = NULL;
  BobguiPrinter   *printer;

  printer = bobgui_print_unix_dialog_get_selected_printer (dialog);

  fill_paper_sizes (dialog, printer);
  fill_custom_paper_sizes (dialog);

  current_page_setup = bobgui_page_setup_copy (bobgui_print_unix_dialog_get_page_setup (dialog));

  if (current_page_setup)
    {
      if (!set_paper_size (dialog, current_page_setup, FALSE, FALSE))
        set_paper_size (dialog, current_page_setup, TRUE, TRUE);

      g_object_unref (current_page_setup);
    }
}

static void
mark_conflicts (BobguiPrintUnixDialog *dialog)
{
  BobguiPrinter *printer;
  gboolean have_conflict;

  have_conflict = FALSE;

  printer = dialog->current_printer;

  if (printer)
    {
      g_signal_handler_block (dialog->options, dialog->options_changed_handler);

      bobgui_printer_option_set_clear_conflicts (dialog->options);
      have_conflict = _bobgui_printer_mark_conflicts (printer, dialog->options);

      g_signal_handler_unblock (dialog->options, dialog->options_changed_handler);
    }

  bobgui_widget_set_visible (dialog->conflicts_widget, have_conflict);
}

static gboolean
mark_conflicts_callback (gpointer data)
{
  BobguiPrintUnixDialog *dialog = data;

  dialog->mark_conflicts_id = 0;

  mark_conflicts (dialog);

  return FALSE;
}

static void
unschedule_idle_mark_conflicts (BobguiPrintUnixDialog *dialog)
{
  if (dialog->mark_conflicts_id != 0)
    {
      g_source_remove (dialog->mark_conflicts_id);
      dialog->mark_conflicts_id = 0;
    }
}

static void
schedule_idle_mark_conflicts (BobguiPrintUnixDialog *dialog)
{
  if (dialog->mark_conflicts_id != 0)
    return;

  dialog->mark_conflicts_id = g_idle_add (mark_conflicts_callback, dialog);
  g_source_set_static_name (g_main_context_find_source_by_id (NULL, dialog->mark_conflicts_id),
                            "[bobgui] mark_conflicts_callback");
}

static void
options_changed_cb (BobguiPrintUnixDialog *dialog)
{
  schedule_idle_mark_conflicts (dialog);

  g_free (dialog->waiting_for_printer);
  dialog->waiting_for_printer = NULL;
}

static void
clear_per_printer_ui (BobguiPrintUnixDialog *dialog)
{
  BobguiWidget *child;

  if (dialog->finishing_table == NULL)
    return;

  while ((child = bobgui_widget_get_first_child (dialog->finishing_table)))
    bobgui_grid_remove (BOBGUI_GRID (dialog->finishing_table), child);
  while ((child = bobgui_widget_get_first_child (dialog->image_quality_table)))
    bobgui_grid_remove (BOBGUI_GRID (dialog->image_quality_table), child);
  while ((child = bobgui_widget_get_first_child (dialog->color_table)))
    bobgui_grid_remove (BOBGUI_GRID (dialog->color_table), child);
  while ((child = bobgui_widget_get_first_child (dialog->advanced_vbox)))
    bobgui_box_remove (BOBGUI_BOX (dialog->advanced_vbox), child);
  while ((child = bobgui_widget_get_first_child (dialog->extension_point)))
    bobgui_box_remove (BOBGUI_BOX (dialog->extension_point), child);
}

static void
printer_details_acquired (BobguiPrinter         *printer,
                          gboolean            success,
                          BobguiPrintUnixDialog *dialog)
{
  disconnect_printer_details_request (dialog, !success);

  if (success)
    selected_printer_changed (dialog);
}

static void
selected_printer_changed (BobguiPrintUnixDialog *dialog)
{
  GListModel *model = G_LIST_MODEL (bobgui_column_view_get_model (BOBGUI_COLUMN_VIEW (dialog->printer_list)));
  BobguiPrinter *printer;

  /* Whenever the user selects a printer we stop looking for
   * the printer specified in the initial settings
   */
  if (dialog->waiting_for_printer &&
      !dialog->internal_printer_change)
    {
      g_free (dialog->waiting_for_printer);
      dialog->waiting_for_printer = NULL;
    }

  disconnect_printer_details_request (dialog, FALSE);

  printer = bobgui_single_selection_get_selected_item (BOBGUI_SINGLE_SELECTION (model));

  /* sets BOBGUI_RESPONSE_OK button sensitivity depending on whether the printer
   * accepts/rejects jobs
   */
  if (printer != NULL)
    {
      if (!bobgui_printer_is_accepting_jobs (printer))
        bobgui_dialog_set_response_sensitive (BOBGUI_DIALOG (dialog), BOBGUI_RESPONSE_OK, FALSE);
      else if (dialog->current_printer == printer && bobgui_printer_has_details (printer))
        bobgui_dialog_set_response_sensitive (BOBGUI_DIALOG (dialog), BOBGUI_RESPONSE_OK, TRUE);
    }

  if (printer != NULL && !bobgui_printer_has_details (printer))
    {
      bobgui_dialog_set_response_sensitive (BOBGUI_DIALOG (dialog), BOBGUI_RESPONSE_OK, FALSE);
      dialog->request_details_tag = g_signal_connect (printer, "details-acquired",
                                                      G_CALLBACK (printer_details_acquired), dialog);

      dialog->request_details_printer = g_object_ref (printer);
      set_busy_cursor (dialog, TRUE);
      bobgui_printer_set_state_message (printer, _("Getting printer information…"));
      bobgui_printer_request_details (printer);
      return;
    }

  if (printer == dialog->current_printer)
    return;

  if (dialog->options)
    {
      g_clear_object (&dialog->options);
      clear_per_printer_ui (dialog);
    }

  g_clear_object (&dialog->current_printer);
  dialog->printer_capabilities = 0;

  if (printer != NULL && bobgui_printer_is_accepting_jobs (printer))
    bobgui_dialog_set_response_sensitive (BOBGUI_DIALOG (dialog), BOBGUI_RESPONSE_OK, TRUE);
  dialog->current_printer = g_object_ref (printer);

  if (printer != NULL)
    {
      if (!dialog->page_setup_set)
        {
          /* if no explicit page setup has been set, use the printer default */
          BobguiPageSetup *page_setup;

          page_setup = bobgui_printer_get_default_page_size (printer);

          if (!page_setup)
            page_setup = bobgui_page_setup_new ();

          if (page_setup && dialog->page_setup)
            bobgui_page_setup_set_orientation (page_setup, bobgui_page_setup_get_orientation (dialog->page_setup));

          g_clear_object (&dialog->page_setup);
          dialog->page_setup = page_setup; /* transfer ownership */
        }

      dialog->printer_capabilities = bobgui_printer_get_capabilities (printer);
      dialog->options = _bobgui_printer_get_options (printer,
                                                dialog->initial_settings,
                                                dialog->page_setup,
                                                dialog->manual_capabilities);

      dialog->options_changed_handler =
        g_signal_connect_swapped (dialog->options, "changed", G_CALLBACK (options_changed_cb), dialog);
      schedule_idle_mark_conflicts (dialog);
    }

  update_dialog_from_settings (dialog);
  update_dialog_from_capabilities (dialog);

  dialog->internal_page_setup_change = TRUE;
  update_paper_sizes (dialog);
  dialog->internal_page_setup_change = FALSE;

  g_object_notify (G_OBJECT (dialog), "selected-printer");
}

static void
update_collate_icon (BobguiToggleButton    *toggle_button,
                     BobguiPrintUnixDialog *dialog)
{
  gboolean collate;
  gboolean reverse;
  int copies;

  collate = dialog_get_collate (dialog);
  reverse = dialog_get_reverse (dialog);
  copies = dialog_get_n_copies (dialog);

  if (collate)
    {
      bobgui_page_thumbnail_set_page_num (BOBGUI_PAGE_THUMBNAIL (dialog->page_a1), reverse ? 1 : 2);
      bobgui_page_thumbnail_set_page_num (BOBGUI_PAGE_THUMBNAIL (dialog->page_a2), reverse ? 2 : 1);
      bobgui_page_thumbnail_set_page_num (BOBGUI_PAGE_THUMBNAIL (dialog->page_b1), reverse ? 1 : 2);
      bobgui_page_thumbnail_set_page_num (BOBGUI_PAGE_THUMBNAIL (dialog->page_b2), reverse ? 2 : 1);
    }
  else
    {
      bobgui_page_thumbnail_set_page_num (BOBGUI_PAGE_THUMBNAIL (dialog->page_a1), reverse ? 2 : 1);
      bobgui_page_thumbnail_set_page_num (BOBGUI_PAGE_THUMBNAIL (dialog->page_a2), reverse ? 2 : 1);
      bobgui_page_thumbnail_set_page_num (BOBGUI_PAGE_THUMBNAIL (dialog->page_b1), reverse ? 1 : 2);
      bobgui_page_thumbnail_set_page_num (BOBGUI_PAGE_THUMBNAIL (dialog->page_b2), reverse ? 1 : 2);
    }

  bobgui_widget_set_visible (dialog->page_b1, copies > 1);
  bobgui_widget_set_visible (dialog->page_b2, copies > 1);
}

static gboolean
page_range_entry_focus_changed (BobguiWidget          *entry,
                                GParamSpec         *pspec,
                                BobguiPrintUnixDialog *dialog)
{
  if (bobgui_widget_has_focus (entry))
    bobgui_check_button_set_active (BOBGUI_CHECK_BUTTON (dialog->page_range_radio), TRUE);

  return FALSE;
}

static void
update_page_range_entry_sensitivity (BobguiWidget *button,
				     BobguiPrintUnixDialog *dialog)
{
  gboolean active;

  active = bobgui_check_button_get_active (BOBGUI_CHECK_BUTTON (button));

  if (active)
    bobgui_widget_grab_focus (dialog->page_range_entry);
}

static void
update_print_at_entry_sensitivity (BobguiWidget *button,
				   BobguiPrintUnixDialog *dialog)
{
  gboolean active;

  active = bobgui_check_button_get_active (BOBGUI_CHECK_BUTTON (button));

  bobgui_widget_set_sensitive (dialog->print_at_entry, active);

  if (active)
    bobgui_widget_grab_focus (dialog->print_at_entry);
}

static gboolean
is_range_separator (char c)
{
  return (c == ',' || c == ';' || c == ':');
}

static BobguiPageRange *
dialog_get_page_ranges (BobguiPrintUnixDialog *dialog,
                        int                *n_ranges_out)
{
  int i, n_ranges;
  const char *text, *p;
  char *next;
  BobguiPageRange *ranges;
  int start, end;

  text = bobgui_editable_get_text (BOBGUI_EDITABLE (dialog->page_range_entry));

  if (*text == 0)
    {
      *n_ranges_out = 0;
      return NULL;
    }

  n_ranges = 1;
  p = text;
  while (*p)
    {
      if (is_range_separator (*p))
        n_ranges++;
      p++;
    }

  ranges = g_new0 (BobguiPageRange, n_ranges);

  i = 0;
  p = text;
  while (*p)
    {
      while (isspace (*p)) p++;

      if (*p == '-')
        {
          /* a half-open range like -2 */
          start = 1;
        }
      else
        {
          start = (int)strtol (p, &next, 10);
          if (start < 1)
            start = 1;
          p = next;
        }

      end = start;

      while (isspace (*p)) p++;

      if (*p == '-')
        {
          p++;
          end = (int)strtol (p, &next, 10);
          if (next == p) /* a half-open range like 2- */
            end = 0;
          else if (end < start)
            end = start;
        }

      ranges[i].start = start - 1;
      ranges[i].end = end - 1;
      i++;

      /* Skip until end or separator */
      while (*p && !is_range_separator (*p))
        p++;

      /* if not at end, skip separator */
      if (*p)
        p++;
    }

  *n_ranges_out = i;

  return ranges;
}

static void
dialog_set_page_ranges (BobguiPrintUnixDialog *dialog,
                        BobguiPageRange       *ranges,
                        int                 n_ranges)
{
  int i;
  GString *s = g_string_new (NULL);

  for (i = 0; i < n_ranges; i++)
    {
      g_string_append_printf (s, "%d", ranges[i].start + 1);
      if (ranges[i].end > ranges[i].start)
        g_string_append_printf (s, "-%d", ranges[i].end + 1);
      else if (ranges[i].end == -1)
        g_string_append (s, "-");

      if (i != n_ranges - 1)
        g_string_append (s, ",");
    }

  bobgui_editable_set_text (BOBGUI_EDITABLE (dialog->page_range_entry), s->str);

  g_string_free (s, TRUE);
}

static BobguiPrintPages
dialog_get_print_pages (BobguiPrintUnixDialog *dialog)
{
  if (bobgui_check_button_get_active (BOBGUI_CHECK_BUTTON (dialog->all_pages_radio)))
    return BOBGUI_PRINT_PAGES_ALL;
  else if (bobgui_check_button_get_active (BOBGUI_CHECK_BUTTON (dialog->current_page_radio)))
    return BOBGUI_PRINT_PAGES_CURRENT;
  else if (bobgui_check_button_get_active (BOBGUI_CHECK_BUTTON (dialog->selection_radio)))
    return BOBGUI_PRINT_PAGES_SELECTION;
  else
    return BOBGUI_PRINT_PAGES_RANGES;
}

static void
dialog_set_print_pages (BobguiPrintUnixDialog *dialog,
                        BobguiPrintPages       pages)
{
  if (pages == BOBGUI_PRINT_PAGES_RANGES)
    bobgui_check_button_set_active (BOBGUI_CHECK_BUTTON (dialog->page_range_radio), TRUE);
  else if (pages == BOBGUI_PRINT_PAGES_CURRENT)
    bobgui_check_button_set_active (BOBGUI_CHECK_BUTTON (dialog->current_page_radio), TRUE);
  else if (pages == BOBGUI_PRINT_PAGES_SELECTION)
    bobgui_check_button_set_active (BOBGUI_CHECK_BUTTON (dialog->selection_radio), TRUE);
  else
    bobgui_check_button_set_active (BOBGUI_CHECK_BUTTON (dialog->all_pages_radio), TRUE);
}

static double
dialog_get_scale (BobguiPrintUnixDialog *dialog)
{
  if (bobgui_widget_is_sensitive (dialog->scale_spin))
    return bobgui_spin_button_get_value (BOBGUI_SPIN_BUTTON (dialog->scale_spin));
  else
    return 100.0;
}

static void
dialog_set_scale (BobguiPrintUnixDialog *dialog,
                  double              val)
{
  bobgui_spin_button_set_value (BOBGUI_SPIN_BUTTON (dialog->scale_spin), val);
}

static BobguiPageSet
dialog_get_page_set (BobguiPrintUnixDialog *dialog)
{
  if (bobgui_widget_is_sensitive (dialog->page_set_combo))
    return (BobguiPageSet)bobgui_drop_down_get_selected (BOBGUI_DROP_DOWN (dialog->page_set_combo));
  else
    return BOBGUI_PAGE_SET_ALL;
}

static void
dialog_set_page_set (BobguiPrintUnixDialog *dialog,
                     BobguiPageSet          val)
{
  bobgui_drop_down_set_selected (BOBGUI_DROP_DOWN (dialog->page_set_combo), (guint)val);
}

static int
dialog_get_n_copies (BobguiPrintUnixDialog *dialog)
{
  BobguiAdjustment *adjustment;
  const char *text;
  char *endptr = NULL;
  int n_copies;

  adjustment = bobgui_spin_button_get_adjustment (BOBGUI_SPIN_BUTTON (dialog->copies_spin));

  text = bobgui_editable_get_text (BOBGUI_EDITABLE (dialog->copies_spin));
  n_copies = g_ascii_strtoull (text, &endptr, 0);

  if (bobgui_widget_is_sensitive (dialog->copies_spin))
    {
      if (n_copies != 0 && endptr != text && (endptr != NULL && endptr[0] == '\0') &&
          n_copies >= bobgui_adjustment_get_lower (adjustment) &&
          n_copies <= bobgui_adjustment_get_upper (adjustment))
        {
          return n_copies;
        }

      return bobgui_spin_button_get_value_as_int (BOBGUI_SPIN_BUTTON (dialog->copies_spin));
    }

  return 1;
}

static void
dialog_set_n_copies (BobguiPrintUnixDialog *dialog,
                     int                 n_copies)
{
  bobgui_spin_button_set_value (BOBGUI_SPIN_BUTTON (dialog->copies_spin), n_copies);
}

static gboolean
dialog_get_collate (BobguiPrintUnixDialog *dialog)
{
  if (bobgui_widget_is_sensitive (dialog->collate_check))
    return bobgui_check_button_get_active (BOBGUI_CHECK_BUTTON (dialog->collate_check));
  return TRUE;
}

static void
dialog_set_collate (BobguiPrintUnixDialog *dialog,
                    gboolean            collate)
{
  bobgui_check_button_set_active (BOBGUI_CHECK_BUTTON (dialog->collate_check), collate);
}

static gboolean
dialog_get_reverse (BobguiPrintUnixDialog *dialog)
{
  if (bobgui_widget_is_sensitive (dialog->reverse_check))
    return bobgui_check_button_get_active (BOBGUI_CHECK_BUTTON (dialog->reverse_check));
  return FALSE;
}

static void
dialog_set_reverse (BobguiPrintUnixDialog *dialog,
                    gboolean            reverse)
{
  bobgui_check_button_set_active (BOBGUI_CHECK_BUTTON (dialog->reverse_check), reverse);
}

static int
dialog_get_pages_per_sheet (BobguiPrintUnixDialog *dialog)
{
  const char *val;
  int num;

  val = bobgui_printer_option_widget_get_value (dialog->pages_per_sheet);

  num = 1;

  if (val)
    {
      num = atoi(val);
      if (num < 1)
        num = 1;
    }

  return num;
}

static BobguiNumberUpLayout
dialog_get_number_up_layout (BobguiPrintUnixDialog *dialog)
{
  BobguiPrintCapabilities       caps;
  BobguiNumberUpLayout          layout;
  const char                *val;
  GEnumClass                *enum_class;
  GEnumValue                *enum_value;

  val = bobgui_printer_option_widget_get_value (dialog->number_up_layout);

  caps = dialog->manual_capabilities | dialog->printer_capabilities;

  if ((caps & BOBGUI_PRINT_CAPABILITY_NUMBER_UP_LAYOUT) == 0)
    return BOBGUI_NUMBER_UP_LAYOUT_LEFT_TO_RIGHT_TOP_TO_BOTTOM;

  if (bobgui_widget_get_direction (BOBGUI_WIDGET (dialog)) == BOBGUI_TEXT_DIR_LTR)
    layout = BOBGUI_NUMBER_UP_LAYOUT_LEFT_TO_RIGHT_TOP_TO_BOTTOM;
  else
    layout = BOBGUI_NUMBER_UP_LAYOUT_RIGHT_TO_LEFT_TOP_TO_BOTTOM;

  if (val == NULL)
    return layout;

  if (val[0] == '\0' && dialog->options)
    {
      BobguiPrinterOption *option = bobgui_printer_option_set_lookup (dialog->options, "bobgui-n-up-layout");
      if (option)
        val = option->value;
    }

  enum_class = g_type_class_ref (BOBGUI_TYPE_NUMBER_UP_LAYOUT);
  enum_value = g_enum_get_value_by_nick (enum_class, val);
  if (enum_value)
    layout = enum_value->value;
  g_type_class_unref (enum_class);

  return layout;
}

static void
draw_page (BobguiDrawingArea *da,
           cairo_t        *cr,
           int             width,
           int             height,
           gpointer        data)
{
  BobguiWidget *widget = BOBGUI_WIDGET (da);
  BobguiPrintUnixDialog *dialog = BOBGUI_PRINT_UNIX_DIALOG (data);
  double ratio;
  int w, h, tmp;
  int pages_x, pages_y, i, x, y, layout_w, layout_h;
  double page_width, page_height;
  BobguiPageOrientation orientation;
  gboolean landscape;
  PangoLayout *layout;
  PangoFontDescription *font;
  char *text;
  GdkRGBA color;
  BobguiNumberUpLayout number_up_layout;
  int start_x, end_x, start_y, end_y;
  int dx, dy;
  gboolean horizontal;
  BobguiPageSetup *page_setup;
  double paper_width, paper_height;
  double pos_x, pos_y;
  int pages_per_sheet;
  gboolean ltr = TRUE;

  orientation = bobgui_page_setup_get_orientation (dialog->page_setup);
  landscape =
    (orientation == BOBGUI_PAGE_ORIENTATION_LANDSCAPE) ||
    (orientation == BOBGUI_PAGE_ORIENTATION_REVERSE_LANDSCAPE);

  number_up_layout = dialog_get_number_up_layout (dialog);

  cairo_save (cr);

  page_setup = bobgui_print_unix_dialog_get_page_setup (dialog);

  if (page_setup != NULL)
    {
      if (!landscape)
        {
          paper_width = bobgui_page_setup_get_paper_width (page_setup, BOBGUI_UNIT_MM);
          paper_height = bobgui_page_setup_get_paper_height (page_setup, BOBGUI_UNIT_MM);
        }
      else
        {
          paper_width = bobgui_page_setup_get_paper_height (page_setup, BOBGUI_UNIT_MM);
          paper_height = bobgui_page_setup_get_paper_width (page_setup, BOBGUI_UNIT_MM);
        }

      if (paper_width < paper_height)
        {
          h = EXAMPLE_PAGE_AREA_SIZE - 3;
          w = (paper_height != 0) ? h * paper_width / paper_height : 0;
        }
      else
        {
          w = EXAMPLE_PAGE_AREA_SIZE - 3;
          h = (paper_width != 0) ? w * paper_height / paper_width : 0;
        }

      if (paper_width == 0)
        w = 0;

      if (paper_height == 0)
        h = 0;
    }
  else
    {
      ratio = G_SQRT2;
      w = (EXAMPLE_PAGE_AREA_SIZE - 3) / ratio;
      h = EXAMPLE_PAGE_AREA_SIZE - 3;
    }

  pages_per_sheet = dialog_get_pages_per_sheet (dialog);
  switch (pages_per_sheet)
    {
    default:
    case 1:
      pages_x = 1; pages_y = 1;
      break;
    case 2:
      landscape = !landscape;
      pages_x = 1; pages_y = 2;
      break;
    case 4:
      pages_x = 2; pages_y = 2;
      break;
    case 6:
      landscape = !landscape;
      pages_x = 2; pages_y = 3;
      break;
    case 9:
      pages_x = 3; pages_y = 3;
      break;
    case 16:
      pages_x = 4; pages_y = 4;
      break;
    }

  if (landscape)
    {
      tmp = w;
      w = h;
      h = tmp;

      tmp = pages_x;
      pages_x = pages_y;
      pages_y = tmp;
    }

  bobgui_widget_get_color (dialog->page_a1, &color);

  pos_x = (width - w) / 2;
  pos_y = (height - h) / 2 - 10;
  cairo_translate (cr, pos_x, pos_y);

  cairo_rectangle (cr, 1, 1, w, h);
  cairo_set_source_rgba (cr, 1, 1, 1, 1);
  cairo_fill_preserve (cr);
  cairo_set_source_rgba (cr, 0.5, 0.5, 0.5, 0.5);
  cairo_set_line_width (cr, 1.0);
  cairo_stroke (cr);

  i = 1;

  page_width = (double)w / pages_x;
  page_height = (double)h / pages_y;

  layout  = pango_cairo_create_layout (cr);

  font = pango_font_description_new ();
  pango_font_description_set_family (font, "sans");

  if (page_height > 0)
    pango_font_description_set_absolute_size (font, page_height * 0.4 * PANGO_SCALE);
  else
    pango_font_description_set_absolute_size (font, 1);

  pango_layout_set_font_description (layout, font);
  pango_font_description_free (font);

  pango_layout_set_width (layout, page_width * PANGO_SCALE);
  pango_layout_set_alignment (layout, PANGO_ALIGN_CENTER);

  switch (number_up_layout)
    {
      default:
      case BOBGUI_NUMBER_UP_LAYOUT_LEFT_TO_RIGHT_TOP_TO_BOTTOM:
        start_x = 0;
        end_x = pages_x - 1;
        start_y = 0;
        end_y = pages_y - 1;
        dx = 1;
        dy = 1;
        horizontal = TRUE;
        break;
      case BOBGUI_NUMBER_UP_LAYOUT_LEFT_TO_RIGHT_BOTTOM_TO_TOP:
        start_x = 0;
        end_x = pages_x - 1;
        start_y = pages_y - 1;
        end_y = 0;
        dx = 1;
        dy = - 1;
        horizontal = TRUE;
        break;
      case BOBGUI_NUMBER_UP_LAYOUT_RIGHT_TO_LEFT_TOP_TO_BOTTOM:
        start_x = pages_x - 1;
        end_x = 0;
        start_y = 0;
        end_y = pages_y - 1;
        dx = - 1;
        dy = 1;
        horizontal = TRUE;
        break;
      case BOBGUI_NUMBER_UP_LAYOUT_RIGHT_TO_LEFT_BOTTOM_TO_TOP:
        start_x = pages_x - 1;
        end_x = 0;
        start_y = pages_y - 1;
        end_y = 0;
        dx = - 1;
        dy = - 1;
        horizontal = TRUE;
        break;
      case BOBGUI_NUMBER_UP_LAYOUT_TOP_TO_BOTTOM_LEFT_TO_RIGHT:
        start_x = 0;
        end_x = pages_x - 1;
        start_y = 0;
        end_y = pages_y - 1;
        dx = 1;
        dy = 1;
        horizontal = FALSE;
        break;
      case BOBGUI_NUMBER_UP_LAYOUT_TOP_TO_BOTTOM_RIGHT_TO_LEFT:
        start_x = pages_x - 1;
        end_x = 0;
        start_y = 0;
        end_y = pages_y - 1;
        dx = - 1;
        dy = 1;
        horizontal = FALSE;
        break;
      case BOBGUI_NUMBER_UP_LAYOUT_BOTTOM_TO_TOP_LEFT_TO_RIGHT:
        start_x = 0;
        end_x = pages_x - 1;
        start_y = pages_y - 1;
        end_y = 0;
        dx = 1;
        dy = - 1;
        horizontal = FALSE;
        break;
      case BOBGUI_NUMBER_UP_LAYOUT_BOTTOM_TO_TOP_RIGHT_TO_LEFT:
        start_x = pages_x - 1;
        end_x = 0;
        start_y = pages_y - 1;
        end_y = 0;
        dx = - 1;
        dy = - 1;
        horizontal = FALSE;
        break;
    }

  gdk_cairo_set_source_rgba (cr, &color);
  if (horizontal)
    for (y = start_y; y != end_y + dy; y += dy)
      {
        for (x = start_x; x != end_x + dx; x += dx)
          {
            text = g_strdup_printf ("%d", i++);
            pango_layout_set_text (layout, text, -1);
            g_free (text);
            pango_layout_get_size (layout, &layout_w, &layout_h);
            cairo_save (cr);
            cairo_translate (cr,
                             x * page_width,
                             y * page_height + (page_height - layout_h / 1024.0) / 2);

            pango_cairo_show_layout (cr, layout);
            cairo_restore (cr);
          }
      }
  else
    for (x = start_x; x != end_x + dx; x += dx)
      {
        for (y = start_y; y != end_y + dy; y += dy)
          {
            text = g_strdup_printf ("%d", i++);
            pango_layout_set_text (layout, text, -1);
            g_free (text);
            pango_layout_get_size (layout, &layout_w, &layout_h);
            cairo_save (cr);
            cairo_translate (cr,
                             x * page_width,
                             y * page_height + (page_height - layout_h / 1024.0) / 2);

            pango_cairo_show_layout (cr, layout);
            cairo_restore (cr);
          }
      }

  g_object_unref (layout);

  bobgui_widget_get_color (widget, &color);

  if (page_setup != NULL)
    {
      PangoContext *pango_c = NULL;
      PangoFontDescription *pango_f = NULL;
      int font_size = 12 * PANGO_SCALE;

      pos_x += 1;
      pos_y += 1;

      if (pages_per_sheet == 2 || pages_per_sheet == 6)
        {
          paper_width = bobgui_page_setup_get_paper_height (page_setup, _bobgui_print_get_default_user_units ());
          paper_height = bobgui_page_setup_get_paper_width (page_setup, _bobgui_print_get_default_user_units ());
        }
      else
        {
          paper_width = bobgui_page_setup_get_paper_width (page_setup, _bobgui_print_get_default_user_units ());
          paper_height = bobgui_page_setup_get_paper_height (page_setup, _bobgui_print_get_default_user_units ());
        }

      cairo_restore (cr);
      cairo_save (cr);

      layout = pango_cairo_create_layout (cr);

      font = pango_font_description_new ();
      pango_font_description_set_family (font, "sans");

      pango_c = bobgui_widget_get_pango_context (widget);
      if (pango_c != NULL)
        {
          pango_f = pango_context_get_font_description (pango_c);
          if (pango_f != NULL)
            font_size = pango_font_description_get_size (pango_f);
        }

      pango_font_description_set_size (font, font_size);
      pango_layout_set_font_description (layout, font);
      pango_font_description_free (font);

      pango_layout_set_width (layout, -1);
      pango_layout_set_alignment (layout, PANGO_ALIGN_CENTER);

      if (_bobgui_print_get_default_user_units () == BOBGUI_UNIT_MM)
        text = g_strdup_printf ("%.1f mm", paper_height);
      else
        text = g_strdup_printf ("%.2f inch", paper_height);

      pango_layout_set_text (layout, text, -1);
      g_free (text);
      pango_layout_get_size (layout, &layout_w, &layout_h);

      ltr = bobgui_widget_get_direction (BOBGUI_WIDGET (dialog)) == BOBGUI_TEXT_DIR_LTR;

      if (ltr)
        cairo_translate (cr, pos_x - layout_w / PANGO_SCALE - 2 * RULER_DISTANCE,
                             (height - layout_h / PANGO_SCALE) / 2);
      else
        cairo_translate (cr, pos_x + w + 2 * RULER_DISTANCE,
                             (height - layout_h / PANGO_SCALE) / 2);

      gdk_cairo_set_source_rgba (cr, &color);
      pango_cairo_show_layout (cr, layout);

      cairo_restore (cr);
      cairo_save (cr);

      if (_bobgui_print_get_default_user_units () == BOBGUI_UNIT_MM)
        text = g_strdup_printf ("%.1f mm", paper_width);
      else
        text = g_strdup_printf ("%.2f inch", paper_width);

      pango_layout_set_text (layout, text, -1);
      g_free (text);
      pango_layout_get_size (layout, &layout_w, &layout_h);

      cairo_translate (cr, (width - layout_w / PANGO_SCALE) / 2,
                           pos_y + h + 2 * RULER_DISTANCE);

      gdk_cairo_set_source_rgba (cr, &color);
      pango_cairo_show_layout (cr, layout);

      g_object_unref (layout);

      cairo_restore (cr);

      cairo_set_line_width (cr, 1);

      gdk_cairo_set_source_rgba (cr, &color);

      if (ltr)
        {
          cairo_move_to (cr, pos_x - RULER_DISTANCE, pos_y);
          cairo_line_to (cr, pos_x - RULER_DISTANCE, pos_y + h);
          cairo_stroke (cr);

          cairo_move_to (cr, pos_x - RULER_DISTANCE - RULER_RADIUS, pos_y - 0.5);
          cairo_line_to (cr, pos_x - RULER_DISTANCE + RULER_RADIUS, pos_y - 0.5);
          cairo_stroke (cr);

          cairo_move_to (cr, pos_x - RULER_DISTANCE - RULER_RADIUS, pos_y + h + 0.5);
          cairo_line_to (cr, pos_x - RULER_DISTANCE + RULER_RADIUS, pos_y + h + 0.5);
          cairo_stroke (cr);
        }
      else
        {
          cairo_move_to (cr, pos_x + w + RULER_DISTANCE, pos_y);
          cairo_line_to (cr, pos_x + w + RULER_DISTANCE, pos_y + h);
          cairo_stroke (cr);

          cairo_move_to (cr, pos_x + w + RULER_DISTANCE - RULER_RADIUS, pos_y - 0.5);
          cairo_line_to (cr, pos_x + w + RULER_DISTANCE + RULER_RADIUS, pos_y - 0.5);
          cairo_stroke (cr);

          cairo_move_to (cr, pos_x + w + RULER_DISTANCE - RULER_RADIUS, pos_y + h + 0.5);
          cairo_line_to (cr, pos_x + w + RULER_DISTANCE + RULER_RADIUS, pos_y + h + 0.5);
          cairo_stroke (cr);
        }

      cairo_move_to (cr, pos_x, pos_y + h + RULER_DISTANCE);
      cairo_line_to (cr, pos_x + w, pos_y + h + RULER_DISTANCE);
      cairo_stroke (cr);

      cairo_move_to (cr, pos_x - 0.5, pos_y + h + RULER_DISTANCE - RULER_RADIUS);
      cairo_line_to (cr, pos_x - 0.5, pos_y + h + RULER_DISTANCE + RULER_RADIUS);
      cairo_stroke (cr);

      cairo_move_to (cr, pos_x + w + 0.5, pos_y + h + RULER_DISTANCE - RULER_RADIUS);
      cairo_line_to (cr, pos_x + w + 0.5, pos_y + h + RULER_DISTANCE + RULER_RADIUS);
      cairo_stroke (cr);
    }
}

static void
redraw_page_layout_preview (BobguiPrintUnixDialog *dialog)
{
  if (dialog->page_layout_preview)
    bobgui_widget_queue_draw (dialog->page_layout_preview);
}

static void
update_number_up_layout (BobguiPrintUnixDialog *dialog)
{
  BobguiPrintCapabilities       caps;
  BobguiPrinterOptionSet       *set;
  BobguiNumberUpLayout          layout;
  BobguiPrinterOption          *option;
  BobguiPrinterOption          *old_option;
  BobguiPageOrientation         page_orientation;

  set = dialog->options;

  caps = dialog->manual_capabilities | dialog->printer_capabilities;

  if (caps & BOBGUI_PRINT_CAPABILITY_NUMBER_UP_LAYOUT)
    {
      if (dialog->number_up_layout_n_option == NULL)
        {
          dialog->number_up_layout_n_option = bobgui_printer_option_set_lookup (set, "bobgui-n-up-layout");
          if (dialog->number_up_layout_n_option == NULL)
            {
              const char *n_up_layout[] = { "lrtb", "lrbt", "rltb", "rlbt", "tblr", "tbrl", "btlr", "btrl" };
               /* Translators: These strings name the possible arrangements of
                * multiple pages on a sheet when printing (same as in bobguiprintbackendcups.c)
                */
              const char *n_up_layout_display[] = { N_("Left to right, top to bottom"), N_("Left to right, bottom to top"),
                                                    N_("Right to left, top to bottom"), N_("Right to left, bottom to top"),
                                                    N_("Top to bottom, left to right"), N_("Top to bottom, right to left"),
                                                    N_("Bottom to top, left to right"), N_("Bottom to top, right to left") };
              int i;

              dialog->number_up_layout_n_option = bobgui_printer_option_new ("bobgui-n-up-layout",
                                                                        _("Page Ordering"),
                                                                        BOBGUI_PRINTER_OPTION_TYPE_PICKONE);
              bobgui_printer_option_allocate_choices (dialog->number_up_layout_n_option, 8);

              for (i = 0; i < G_N_ELEMENTS (n_up_layout_display); i++)
                {
                  dialog->number_up_layout_n_option->choices[i] = g_strdup (n_up_layout[i]);
                  dialog->number_up_layout_n_option->choices_display[i] = g_strdup (_(n_up_layout_display[i]));
                }
            }
          g_object_ref (dialog->number_up_layout_n_option);

          dialog->number_up_layout_2_option = bobgui_printer_option_new ("bobgui-n-up-layout",
                                                                    _("Page Ordering"),
                                                                    BOBGUI_PRINTER_OPTION_TYPE_PICKONE);
          bobgui_printer_option_allocate_choices (dialog->number_up_layout_2_option, 2);
        }

      page_orientation = bobgui_page_setup_get_orientation (dialog->page_setup);
      if (page_orientation == BOBGUI_PAGE_ORIENTATION_PORTRAIT ||
          page_orientation == BOBGUI_PAGE_ORIENTATION_REVERSE_PORTRAIT)
        {
          if (! (dialog->number_up_layout_2_option->choices[0] == dialog->number_up_layout_n_option->choices[0] &&
                 dialog->number_up_layout_2_option->choices[1] == dialog->number_up_layout_n_option->choices[2]))
            {
              g_free (dialog->number_up_layout_2_option->choices_display[0]);
              g_free (dialog->number_up_layout_2_option->choices_display[1]);
              dialog->number_up_layout_2_option->choices[0] = dialog->number_up_layout_n_option->choices[0];
              dialog->number_up_layout_2_option->choices[1] = dialog->number_up_layout_n_option->choices[2];
              dialog->number_up_layout_2_option->choices_display[0] = g_strdup ( _("Left to right"));
              dialog->number_up_layout_2_option->choices_display[1] = g_strdup ( _("Right to left"));
            }
        }
      else
        {
          if (! (dialog->number_up_layout_2_option->choices[0] == dialog->number_up_layout_n_option->choices[0] &&
                 dialog->number_up_layout_2_option->choices[1] == dialog->number_up_layout_n_option->choices[1]))
            {
              g_free (dialog->number_up_layout_2_option->choices_display[0]);
              g_free (dialog->number_up_layout_2_option->choices_display[1]);
              dialog->number_up_layout_2_option->choices[0] = dialog->number_up_layout_n_option->choices[0];
              dialog->number_up_layout_2_option->choices[1] = dialog->number_up_layout_n_option->choices[1];
              dialog->number_up_layout_2_option->choices_display[0] = g_strdup ( _("Top to bottom"));
              dialog->number_up_layout_2_option->choices_display[1] = g_strdup ( _("Bottom to top"));
            }
        }

      layout = dialog_get_number_up_layout (dialog);

      old_option = bobgui_printer_option_set_lookup (set, "bobgui-n-up-layout");
      if (old_option != NULL)
        bobgui_printer_option_set_remove (set, old_option);

      if (dialog_get_pages_per_sheet (dialog) != 1)
        {
          GEnumClass *enum_class;
          GEnumValue *enum_value;
          enum_class = g_type_class_ref (BOBGUI_TYPE_NUMBER_UP_LAYOUT);

          if (dialog_get_pages_per_sheet (dialog) == 2)
            {
              option = dialog->number_up_layout_2_option;

              switch (layout)
                {
                case BOBGUI_NUMBER_UP_LAYOUT_LEFT_TO_RIGHT_TOP_TO_BOTTOM:
                case BOBGUI_NUMBER_UP_LAYOUT_TOP_TO_BOTTOM_LEFT_TO_RIGHT:
                  enum_value = g_enum_get_value (enum_class, BOBGUI_NUMBER_UP_LAYOUT_LEFT_TO_RIGHT_TOP_TO_BOTTOM);
                  break;

                case BOBGUI_NUMBER_UP_LAYOUT_LEFT_TO_RIGHT_BOTTOM_TO_TOP:
                case BOBGUI_NUMBER_UP_LAYOUT_BOTTOM_TO_TOP_LEFT_TO_RIGHT:
                  enum_value = g_enum_get_value (enum_class, BOBGUI_NUMBER_UP_LAYOUT_LEFT_TO_RIGHT_BOTTOM_TO_TOP);
                  break;

                case BOBGUI_NUMBER_UP_LAYOUT_RIGHT_TO_LEFT_TOP_TO_BOTTOM:
                case BOBGUI_NUMBER_UP_LAYOUT_TOP_TO_BOTTOM_RIGHT_TO_LEFT:
                  enum_value = g_enum_get_value (enum_class, BOBGUI_NUMBER_UP_LAYOUT_RIGHT_TO_LEFT_TOP_TO_BOTTOM);
                  break;

                case BOBGUI_NUMBER_UP_LAYOUT_RIGHT_TO_LEFT_BOTTOM_TO_TOP:
                case BOBGUI_NUMBER_UP_LAYOUT_BOTTOM_TO_TOP_RIGHT_TO_LEFT:
                  enum_value = g_enum_get_value (enum_class, BOBGUI_NUMBER_UP_LAYOUT_RIGHT_TO_LEFT_BOTTOM_TO_TOP);
                  break;

                default:
                  g_assert_not_reached();
                  enum_value = NULL;
                }
            }
          else
            {
              option = dialog->number_up_layout_n_option;

              enum_value = g_enum_get_value (enum_class, layout);
            }

          g_assert (enum_value != NULL);
          bobgui_printer_option_set (option, enum_value->value_nick);
          g_type_class_unref (enum_class);

          bobgui_printer_option_set_add (set, option);
        }
    }

  setup_option (dialog, "bobgui-n-up-layout", dialog->number_up_layout);

  if (dialog->number_up_layout != NULL)
    bobgui_widget_set_sensitive (BOBGUI_WIDGET (dialog->number_up_layout),
                              (caps & BOBGUI_PRINT_CAPABILITY_NUMBER_UP_LAYOUT) &&
                              (dialog_get_pages_per_sheet (dialog) > 1));
}

static void
custom_paper_dialog_response_cb (BobguiDialog *custom_paper_dialog,
                                 int        response_id,
                                 gpointer   user_data)
{
  BobguiPrintUnixDialog *dialog = BOBGUI_PRINT_UNIX_DIALOG (user_data);

  dialog->internal_page_setup_change = TRUE;
  bobgui_print_load_custom_papers (dialog->custom_paper_list);
  update_paper_sizes (dialog);
  dialog->internal_page_setup_change = FALSE;

  if (dialog->page_setup_set)
    {
      GListModel *model;
      guint n, i;

      model = G_LIST_MODEL (dialog->custom_paper_list);
      n = g_list_model_get_n_items (model);
      for (i = 0; i < n; i++)
        {
          BobguiPageSetup *page_setup = g_list_model_get_item (model, i);

          if (g_strcmp0 (bobgui_paper_size_get_display_name (bobgui_page_setup_get_paper_size (page_setup)),
                         bobgui_paper_size_get_display_name (bobgui_page_setup_get_paper_size (dialog->page_setup))) == 0)
            bobgui_print_unix_dialog_set_page_setup (dialog, page_setup);

          g_clear_object (&page_setup);
        }
    }

  bobgui_window_destroy (BOBGUI_WINDOW (custom_paper_dialog));
}

static void
orientation_changed (GObject            *object,
                     GParamSpec         *pspec,
                     BobguiPrintUnixDialog *dialog)
{
  BobguiPageOrientation orientation;
  BobguiPageSetup *page_setup;

  if (dialog->internal_page_setup_change)
    return;

  orientation = (BobguiPageOrientation) bobgui_drop_down_get_selected (BOBGUI_DROP_DOWN (dialog->orientation_combo));

  if (dialog->page_setup)
    {
      page_setup = bobgui_page_setup_copy (dialog->page_setup);
      if (page_setup)
        bobgui_page_setup_set_orientation (page_setup, orientation);

      bobgui_print_unix_dialog_set_page_setup (dialog, page_setup);
    }

  redraw_page_layout_preview (dialog);
}

static void
paper_size_changed (BobguiDropDown *combo_box,
                    GParamSpec *pspec,
                    BobguiPrintUnixDialog *dialog)
{
  BobguiPageSetup *page_setup, *last_page_setup;
  BobguiPageOrientation orientation;
  guint selected;

  if (dialog->internal_page_setup_change)
    return;

  selected = bobgui_drop_down_get_selected (BOBGUI_DROP_DOWN (combo_box));
  if (selected != BOBGUI_INVALID_LIST_POSITION)
    {
      GListModel *papers, *model;

      papers = bobgui_drop_down_get_model (BOBGUI_DROP_DOWN (dialog->paper_size_combo));
      page_setup = g_list_model_get_item (papers, selected);
      model = bobgui_flatten_list_model_get_model_for_item (BOBGUI_FLATTEN_LIST_MODEL (papers), selected);

      if (model == G_LIST_MODEL (dialog->manage_papers_list))
        {
          BobguiWidget *custom_paper_dialog;

          /* Change from "manage" menu item to last value */
          if (dialog->page_setup)
            last_page_setup = g_object_ref (dialog->page_setup);
          else
            last_page_setup = bobgui_page_setup_new (); /* "good" default */

          if (!set_paper_size (dialog, last_page_setup, FALSE, FALSE))
            set_paper_size (dialog, last_page_setup, TRUE, TRUE);
          g_object_unref (last_page_setup);

          /* And show the custom paper dialog */
          custom_paper_dialog = _bobgui_custom_paper_unix_dialog_new (BOBGUI_WINDOW (dialog), _("Manage Custom Sizes"));
          g_signal_connect (custom_paper_dialog, "response", G_CALLBACK (custom_paper_dialog_response_cb), dialog);
          bobgui_window_present (BOBGUI_WINDOW (custom_paper_dialog));

          g_object_unref (page_setup);

          return;
        }

      if (dialog->page_setup)
        orientation = bobgui_page_setup_get_orientation (dialog->page_setup);
      else
        orientation = BOBGUI_PAGE_ORIENTATION_PORTRAIT;

      bobgui_page_setup_set_orientation (page_setup, orientation);
      bobgui_print_unix_dialog_set_page_setup (dialog, page_setup);

      g_object_unref (page_setup);
    }

  redraw_page_layout_preview (dialog);
}

/**
 * bobgui_print_unix_dialog_new:
 * @title: (nullable): Title of the dialog
 * @parent: (nullable): Transient parent of the dialog
 *
 * Creates a new `BobguiPrintUnixDialog`.
 *
 * Returns: a new `BobguiPrintUnixDialog`
 */
BobguiWidget *
bobgui_print_unix_dialog_new (const char *title,
                           BobguiWindow   *parent)
{
  BobguiWidget *result;

  result = g_object_new (BOBGUI_TYPE_PRINT_UNIX_DIALOG,
                         "transient-for", parent,
                         "title", title ? title : _("Print"),
                         NULL);

  return result;
}

/**
 * bobgui_print_unix_dialog_get_selected_printer:
 * @dialog: a `BobguiPrintUnixDialog`
 *
 * Gets the currently selected printer.
 *
 * Returns: (transfer none) (nullable): the currently selected printer
 */
BobguiPrinter *
bobgui_print_unix_dialog_get_selected_printer (BobguiPrintUnixDialog *dialog)
{
  g_return_val_if_fail (BOBGUI_IS_PRINT_UNIX_DIALOG (dialog), NULL);

  return dialog->current_printer;
}

/**
 * bobgui_print_unix_dialog_set_page_setup:
 * @dialog: a `BobguiPrintUnixDialog`
 * @page_setup: a `BobguiPageSetup`
 *
 * Sets the page setup of the `BobguiPrintUnixDialog`.
 */
void
bobgui_print_unix_dialog_set_page_setup (BobguiPrintUnixDialog *dialog,
                                      BobguiPageSetup       *page_setup)
{
  g_return_if_fail (BOBGUI_IS_PRINT_UNIX_DIALOG (dialog));
  g_return_if_fail (BOBGUI_IS_PAGE_SETUP (page_setup));

  if (dialog->page_setup != page_setup)
    {
      g_clear_object (&dialog->page_setup);
      dialog->page_setup = g_object_ref (page_setup);

      dialog->page_setup_set = TRUE;

      g_object_notify (G_OBJECT (dialog), "page-setup");
    }
}

/**
 * bobgui_print_unix_dialog_get_page_setup:
 * @dialog: a `BobguiPrintUnixDialog`
 *
 * Gets the page setup that is used by the `BobguiPrintUnixDialog`.
 *
 * Returns: (transfer none): the page setup of @dialog.
 */
BobguiPageSetup *
bobgui_print_unix_dialog_get_page_setup (BobguiPrintUnixDialog *dialog)
{
  g_return_val_if_fail (BOBGUI_IS_PRINT_UNIX_DIALOG (dialog), NULL);

  return dialog->page_setup;
}

/**
 * bobgui_print_unix_dialog_get_page_setup_set:
 * @dialog: a `BobguiPrintUnixDialog`
 *
 * Gets whether a page setup was set by the user.
 *
 * Returns: whether a page setup was set by user.
 */
gboolean
bobgui_print_unix_dialog_get_page_setup_set (BobguiPrintUnixDialog *dialog)
{
  g_return_val_if_fail (BOBGUI_IS_PRINT_UNIX_DIALOG (dialog), FALSE);

  return dialog->page_setup_set;
}

/**
 * bobgui_print_unix_dialog_set_current_page:
 * @dialog: a `BobguiPrintUnixDialog`
 * @current_page: the current page number.
 *
 * Sets the current page number.
 *
 * If @current_page is not -1, this enables the current page choice
 * for the range of pages to print.
 */
void
bobgui_print_unix_dialog_set_current_page (BobguiPrintUnixDialog *dialog,
                                        int                 current_page)
{
  g_return_if_fail (BOBGUI_IS_PRINT_UNIX_DIALOG (dialog));

  if (dialog->current_page != current_page)
    {
      dialog->current_page = current_page;

      if (dialog->current_page_radio)
        bobgui_widget_set_sensitive (dialog->current_page_radio, current_page != -1);

      g_object_notify (G_OBJECT (dialog), "current-page");
    }
}

/**
 * bobgui_print_unix_dialog_get_current_page:
 * @dialog: a `BobguiPrintUnixDialog`
 *
 * Gets the current page of the `BobguiPrintUnixDialog`.
 *
 * Returns: the current page of @dialog
 */
int
bobgui_print_unix_dialog_get_current_page (BobguiPrintUnixDialog *dialog)
{
  g_return_val_if_fail (BOBGUI_IS_PRINT_UNIX_DIALOG (dialog), -1);

  return dialog->current_page;
}

static gboolean
set_active_printer (BobguiPrintUnixDialog *dialog,
                    const char         *printer_name)
{
  GListModel *model;
  BobguiPrinter *printer;
  guint i;

  model = G_LIST_MODEL (bobgui_column_view_get_model (BOBGUI_COLUMN_VIEW (dialog->printer_list)));

  for (i = 0; i < g_list_model_get_n_items (model); i++)
    {
      printer = g_list_model_get_item (model, i);

      if (strcmp (bobgui_printer_get_name (printer), printer_name) == 0)
        {
          bobgui_single_selection_set_selected (BOBGUI_SINGLE_SELECTION (model), i);

          g_free (dialog->waiting_for_printer);
          dialog->waiting_for_printer = NULL;

          g_object_unref (printer);
          return TRUE;
        }

      g_object_unref (printer);
    }

  return FALSE;
}

/**
 * bobgui_print_unix_dialog_set_settings: (set-property print-settings)
 * @dialog: a `BobguiPrintUnixDialog`
 * @settings: (nullable): a `BobguiPrintSettings`
 *
 * Sets the `BobguiPrintSettings` for the `BobguiPrintUnixDialog`.
 *
 * Typically, this is used to restore saved print settings
 * from a previous print operation before the print dialog
 * is shown.
 */
void
bobgui_print_unix_dialog_set_settings (BobguiPrintUnixDialog *dialog,
                                    BobguiPrintSettings   *settings)
{
  const char *printer;
  BobguiPageRange *ranges;
  int num_ranges;

  g_return_if_fail (BOBGUI_IS_PRINT_UNIX_DIALOG (dialog));
  g_return_if_fail (settings == NULL || BOBGUI_IS_PRINT_SETTINGS (settings));

  if (settings != NULL)
    {
      dialog_set_collate (dialog, bobgui_print_settings_get_collate (settings));
      dialog_set_reverse (dialog, bobgui_print_settings_get_reverse (settings));
      dialog_set_n_copies (dialog, bobgui_print_settings_get_n_copies (settings));
      dialog_set_scale (dialog, bobgui_print_settings_get_scale (settings));
      dialog_set_page_set (dialog, bobgui_print_settings_get_page_set (settings));
      dialog_set_print_pages (dialog, bobgui_print_settings_get_print_pages (settings));
      ranges = bobgui_print_settings_get_page_ranges (settings, &num_ranges);
      if (ranges)
        {
          dialog_set_page_ranges (dialog, ranges, num_ranges);
          g_free (ranges);
        }

      dialog->format_for_printer =
        g_strdup (bobgui_print_settings_get (settings, "format-for-printer"));
    }

  if (dialog->initial_settings)
    g_object_unref (dialog->initial_settings);

  dialog->initial_settings = settings;

  g_free (dialog->waiting_for_printer);
  dialog->waiting_for_printer = NULL;

  if (settings)
    {
      g_object_ref (settings);

      printer = bobgui_print_settings_get_printer (settings);

      if (printer && !set_active_printer (dialog, printer))
        dialog->waiting_for_printer = g_strdup (printer);
    }

  g_object_notify (G_OBJECT (dialog), "print-settings");
}

/**
 * bobgui_print_unix_dialog_get_settings: (get-property print-settings)
 * @dialog: a `BobguiPrintUnixDialog`
 *
 * Gets a new `BobguiPrintSettings` object that represents the
 * current values in the print dialog.
 *
 * Note that this creates a new object, and you need to unref
 * it if don’t want to keep it.
 *
 * Returns: (transfer full): a new `BobguiPrintSettings` object with the values from @dialog
 */
BobguiPrintSettings *
bobgui_print_unix_dialog_get_settings (BobguiPrintUnixDialog *dialog)
{
  BobguiPrintSettings *settings;
  BobguiPrintPages print_pages;
  BobguiPageRange *ranges;
  int n_ranges;

  g_return_val_if_fail (BOBGUI_IS_PRINT_UNIX_DIALOG (dialog), NULL);

  settings = bobgui_print_settings_new ();

  if (dialog->current_printer)
    bobgui_print_settings_set_printer (settings,
                                    bobgui_printer_get_name (dialog->current_printer));
  else
    bobgui_print_settings_set_printer (settings, "default");

  bobgui_print_settings_set (settings, "format-for-printer",
                          dialog->format_for_printer);

  bobgui_print_settings_set_collate (settings,
                                  dialog_get_collate (dialog));

  bobgui_print_settings_set_reverse (settings,
                                  dialog_get_reverse (dialog));

  bobgui_print_settings_set_n_copies (settings,
                                   dialog_get_n_copies (dialog));

  bobgui_print_settings_set_scale (settings,
                                dialog_get_scale (dialog));

  bobgui_print_settings_set_page_set (settings,
                                   dialog_get_page_set (dialog));

  print_pages = dialog_get_print_pages (dialog);
  bobgui_print_settings_set_print_pages (settings, print_pages);

  ranges = dialog_get_page_ranges (dialog, &n_ranges);
  if (ranges)
    {
      bobgui_print_settings_set_page_ranges (settings, ranges, n_ranges);
      g_free (ranges);
    }

  /* TODO: print when. How to handle? */

  if (dialog->current_printer)
    _bobgui_printer_get_settings_from_options (dialog->current_printer,
                                            dialog->options,
                                            settings);

  return settings;
}

/**
 * bobgui_print_unix_dialog_add_custom_tab:
 * @dialog: a `BobguiPrintUnixDialog`
 * @child: the widget to put in the custom tab
 * @tab_label: the widget to use as tab label
 *
 * Adds a custom tab to the print dialog.
 */
void
bobgui_print_unix_dialog_add_custom_tab (BobguiPrintUnixDialog *dialog,
                                      BobguiWidget          *child,
                                      BobguiWidget          *tab_label)
{
  bobgui_notebook_insert_page (BOBGUI_NOTEBOOK (dialog->notebook),
                            child, tab_label, 2);
  bobgui_widget_set_visible (child, TRUE);
  bobgui_widget_set_visible (tab_label, TRUE);
}

/**
 * bobgui_print_unix_dialog_set_manual_capabilities:
 * @dialog: a `BobguiPrintUnixDialog`
 * @capabilities: the printing capabilities of your application
 *
 * This lets you specify the printing capabilities your application
 * supports.
 *
 * For instance, if you can handle scaling the output then you pass
 * %BOBGUI_PRINT_CAPABILITY_SCALE. If you don’t pass that, then the dialog
 * will only let you select the scale if the printing system automatically
 * handles scaling.
 */
void
bobgui_print_unix_dialog_set_manual_capabilities (BobguiPrintUnixDialog   *dialog,
                                               BobguiPrintCapabilities  capabilities)
{
  if (dialog->manual_capabilities != capabilities)
    {
      dialog->manual_capabilities = capabilities;
      update_dialog_from_capabilities (dialog);

      if (dialog->current_printer)
        {
          g_clear_object (&dialog->current_printer);
          selected_printer_changed (dialog);
       }

      g_object_notify (G_OBJECT (dialog), "manual-capabilities");
    }
}

/**
 * bobgui_print_unix_dialog_get_manual_capabilities:
 * @dialog: a `BobguiPrintUnixDialog`
 *
 * Gets the capabilities that have been set on this `BobguiPrintUnixDialog`.
 *
 * Returns: the printing capabilities
 */
BobguiPrintCapabilities
bobgui_print_unix_dialog_get_manual_capabilities (BobguiPrintUnixDialog *dialog)
{
  g_return_val_if_fail (BOBGUI_IS_PRINT_UNIX_DIALOG (dialog), FALSE);

  return dialog->manual_capabilities;
}

/**
 * bobgui_print_unix_dialog_set_support_selection:
 * @dialog: a `BobguiPrintUnixDialog`
 * @support_selection: %TRUE to allow print selection
 *
 * Sets whether the print dialog allows user to print a selection.
 */
void
bobgui_print_unix_dialog_set_support_selection (BobguiPrintUnixDialog *dialog,
                                             gboolean            support_selection)
{
  g_return_if_fail (BOBGUI_IS_PRINT_UNIX_DIALOG (dialog));

  support_selection = support_selection != FALSE;
  if (dialog->support_selection != support_selection)
    {
      dialog->support_selection = support_selection;

      if (dialog->selection_radio)
        {
          bobgui_widget_set_visible (dialog->selection_radio, support_selection);
          bobgui_widget_set_sensitive (dialog->selection_radio, support_selection && dialog->has_selection);
        }

      g_object_notify (G_OBJECT (dialog), "support-selection");
    }
}

/**
 * bobgui_print_unix_dialog_get_support_selection:
 * @dialog: a `BobguiPrintUnixDialog`
 *
 * Gets whether the print dialog allows user to print a selection.
 *
 * Returns: whether the application supports print of selection
 */
gboolean
bobgui_print_unix_dialog_get_support_selection (BobguiPrintUnixDialog *dialog)
{
  g_return_val_if_fail (BOBGUI_IS_PRINT_UNIX_DIALOG (dialog), FALSE);

  return dialog->support_selection;
}

/**
 * bobgui_print_unix_dialog_set_has_selection:
 * @dialog: a `BobguiPrintUnixDialog`
 * @has_selection: %TRUE indicates that a selection exists
 *
 * Sets whether a selection exists.
 */
void
bobgui_print_unix_dialog_set_has_selection (BobguiPrintUnixDialog *dialog,
                                         gboolean            has_selection)
{
  g_return_if_fail (BOBGUI_IS_PRINT_UNIX_DIALOG (dialog));

  has_selection = has_selection != FALSE;
  if (dialog->has_selection != has_selection)
    {
      dialog->has_selection = has_selection;

      if (dialog->selection_radio)
        {
          if (dialog->support_selection)
            bobgui_widget_set_sensitive (dialog->selection_radio, has_selection);
          else
            bobgui_widget_set_sensitive (dialog->selection_radio, FALSE);
        }

      g_object_notify (G_OBJECT (dialog), "has-selection");
    }
}

/**
 * bobgui_print_unix_dialog_get_has_selection:
 * @dialog: a `BobguiPrintUnixDialog`
 *
 * Gets whether there is a selection.
 *
 * Returns: whether there is a selection
 */
gboolean
bobgui_print_unix_dialog_get_has_selection (BobguiPrintUnixDialog *dialog)
{
  g_return_val_if_fail (BOBGUI_IS_PRINT_UNIX_DIALOG (dialog), FALSE);

  return dialog->has_selection;
}

/**
 * bobgui_print_unix_dialog_set_embed_page_setup:
 * @dialog: a `BobguiPrintUnixDialog`
 * @embed: embed page setup selection
 *
 * Embed page size combo box and orientation combo box into page setup page.
 */
void
bobgui_print_unix_dialog_set_embed_page_setup (BobguiPrintUnixDialog *dialog,
                                            gboolean            embed)
{
  g_return_if_fail (BOBGUI_IS_PRINT_UNIX_DIALOG (dialog));

  embed = embed != FALSE;
  if (dialog->embed_page_setup != embed)
    {
      dialog->embed_page_setup = embed;

      bobgui_widget_set_sensitive (dialog->paper_size_combo, dialog->embed_page_setup);
      bobgui_widget_set_sensitive (dialog->orientation_combo, dialog->embed_page_setup);

      if (dialog->embed_page_setup)
        {
          if (dialog->paper_size_combo != NULL)
            g_signal_connect (dialog->paper_size_combo, "notify::selected", G_CALLBACK (paper_size_changed), dialog);

          if (dialog->orientation_combo)
            g_signal_connect (dialog->orientation_combo, "notify::selected", G_CALLBACK (orientation_changed), dialog);
        }
      else
        {
          if (dialog->paper_size_combo != NULL)
            g_signal_handlers_disconnect_by_func (dialog->paper_size_combo, G_CALLBACK (paper_size_changed), dialog);

          if (dialog->orientation_combo)
            g_signal_handlers_disconnect_by_func (dialog->orientation_combo, G_CALLBACK (orientation_changed), dialog);
        }

      dialog->internal_page_setup_change = TRUE;
      update_paper_sizes (dialog);
      dialog->internal_page_setup_change = FALSE;
    }
}

/**
 * bobgui_print_unix_dialog_get_embed_page_setup:
 * @dialog: a `BobguiPrintUnixDialog`
 *
 * Gets whether to embed the page setup.
 *
 * Returns: whether to embed the page setup
 */
gboolean
bobgui_print_unix_dialog_get_embed_page_setup (BobguiPrintUnixDialog *dialog)
{
  g_return_val_if_fail (BOBGUI_IS_PRINT_UNIX_DIALOG (dialog), FALSE);

  return dialog->embed_page_setup;
}
