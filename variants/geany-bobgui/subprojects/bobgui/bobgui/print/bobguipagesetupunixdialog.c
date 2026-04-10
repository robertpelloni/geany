/* BobguiPageSetupUnixDialog
 * Copyright (C) 2006 Alexander Larsson <alexl@redhat.com>
 * Copyright © 2006, 2007, 2008 Christian Persch
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
#include <string.h>
#include <locale.h>

#include <glib/gi18n-lib.h>

#include "deprecated/bobguidialogprivate.h"

#include "bobguipagesetupunixdialog.h"
#include "bobguicustompaperunixdialog.h"
#include "bobguiprintbackendprivate.h"
#include "bobguipapersize.h"
#include "bobguiprintutilsprivate.h"

/**
 * BobguiPageSetupUnixDialog:
 *
 * Presents a page setup dialog for platforms which don’t provide
 * a native page setup dialog, like Unix.
 *
 * <picture>
 *   <source srcset="pagesetupdialog-dark.png" media="(prefers-color-scheme: dark)">
 *   <img alt="An example BobguiPageSetupUnixDialog" src="pagesetupdialog.png">
 * </picture>
 *
 * It can be used very much like any other BOBGUI dialog, at the
 * cost of the portability offered by the high-level printing
 * API in [class@Bobgui.PrintOperation].
 *
 * ## CSS nodes
 *
 * `BobguiPageSetupUnixDialog` has a single CSS node with the name `window` and
 * style class `.pagesetup`.
 */

typedef struct _BobguiPageSetupUnixDialogClass    BobguiPageSetupUnixDialogClass;

struct _BobguiPageSetupUnixDialog
{
  BobguiDialog parent_instance;

  GListModel *printer_list;
  GListStore *page_setup_list;
  GListStore *custom_paper_list;
  GListStore *manage_papers_list;
  GListStore *paper_size_list;

  GList *print_backends;

  BobguiWidget *printer_combo;
  BobguiWidget *paper_size_combo;
  BobguiWidget *paper_size_label;

  BobguiWidget *portrait_radio;
  BobguiWidget *reverse_portrait_radio;
  BobguiWidget *landscape_radio;
  BobguiWidget *reverse_landscape_radio;

  gulong request_details_tag;
  BobguiPrinter *request_details_printer;

  BobguiPrintSettings *print_settings;

  gboolean internal_change;

  /* Save last setup so we can re-set it after selecting manage custom sizes */
  BobguiPageSetup *last_setup;
};

struct _BobguiPageSetupUnixDialogClass
{
  BobguiDialogClass parent_class;
};


/* Keep these in line with BobguiListStores defined in bobguipagesetupunixprintdialog.ui */
enum {
  PRINTER_LIST_COL_NAME,
  PRINTER_LIST_COL_PRINTER,
  PRINTER_LIST_N_COLS
};

enum {
  PAGE_SETUP_LIST_COL_PAGE_SETUP,
  PAGE_SETUP_LIST_COL_IS_SEPARATOR,
  PAGE_SETUP_LIST_N_COLS
};

G_DEFINE_TYPE (BobguiPageSetupUnixDialog, bobgui_page_setup_unix_dialog, BOBGUI_TYPE_DIALOG)

static void bobgui_page_setup_unix_dialog_finalize  (GObject                *object);
static void fill_paper_sizes_from_printer        (BobguiPageSetupUnixDialog *dialog,
                                                  BobguiPrinter             *printer);
static void printer_changed_callback             (BobguiDropDown            *combo_box,
                                                  GParamSpec             *pspec,
                                                  BobguiPageSetupUnixDialog *dialog);
static void paper_size_changed                   (BobguiDropDown            *combo_box,
                                                  GParamSpec             *pspec,
                                                  BobguiPageSetupUnixDialog *dialog);
static void load_print_backends                  (BobguiPageSetupUnixDialog *dialog);


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
  "iso_a3"
};


static void
bobgui_page_setup_unix_dialog_class_init (BobguiPageSetupUnixDialogClass *class)
{
  GObjectClass *object_class;
  BobguiWidgetClass *widget_class;

  object_class = G_OBJECT_CLASS (class);
  widget_class = BOBGUI_WIDGET_CLASS (class);

  object_class->finalize = bobgui_page_setup_unix_dialog_finalize;

  /* Bind class to template
   */
  bobgui_widget_class_set_template_from_resource (widget_class,
                                               "/org/bobgui/libbobgui/print/ui/bobguipagesetupunixdialog.ui");

  bobgui_widget_class_bind_template_child (widget_class, BobguiPageSetupUnixDialog, printer_combo);
  bobgui_widget_class_bind_template_child (widget_class, BobguiPageSetupUnixDialog, paper_size_combo);
  bobgui_widget_class_bind_template_child (widget_class, BobguiPageSetupUnixDialog, paper_size_label);
  bobgui_widget_class_bind_template_child (widget_class, BobguiPageSetupUnixDialog, portrait_radio);
  bobgui_widget_class_bind_template_child (widget_class, BobguiPageSetupUnixDialog, reverse_portrait_radio);
  bobgui_widget_class_bind_template_child (widget_class, BobguiPageSetupUnixDialog, landscape_radio);
  bobgui_widget_class_bind_template_child (widget_class, BobguiPageSetupUnixDialog, reverse_landscape_radio);

  bobgui_widget_class_bind_template_callback (widget_class, printer_changed_callback);
  bobgui_widget_class_bind_template_callback (widget_class, paper_size_changed);
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
                           BobguiPageSetupUnixDialog   *self)
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
                      BobguiPageSetupUnixDialog   *self)
{
  BobguiWidget *label;

  bind_paper_size_list_item (factory, item, self);

  label = bobgui_list_item_get_child (item);
  bobgui_widget_remove_css_class (label, "separator-before");
}

static gboolean
match_func (gpointer item, gpointer user_data)
{
  return !bobgui_printer_is_virtual (BOBGUI_PRINTER (item));
}

static void
setup_printer_item (BobguiSignalListItemFactory *factory,
                    BobguiListItem              *item)
{
  BobguiWidget *label;

  label = bobgui_label_new ("");
  bobgui_widget_set_halign (label, BOBGUI_ALIGN_START);
  bobgui_label_set_xalign (BOBGUI_LABEL (label), 0.0);
  bobgui_list_item_set_child (item, label);
}

static void
bind_printer_item (BobguiSignalListItemFactory *factory,
                   BobguiListItem              *item,
                   BobguiPageSetupUnixDialog   *self)
{
  BobguiPrinter *printer;
  BobguiWidget *label;
  const char *location;
  const char *name;
  char *str;

  printer = bobgui_list_item_get_item (item);
  label = bobgui_list_item_get_child (item);

  name = bobgui_printer_get_name (printer);
  location = bobgui_printer_get_location (printer);
  str = g_strdup_printf ("<b>%s</b>\n%s", name, location ? location : "");
  bobgui_label_set_markup (BOBGUI_LABEL (label), str);
  g_free (str);
}

static void
bobgui_page_setup_unix_dialog_init (BobguiPageSetupUnixDialog *dialog)
{
  BobguiListItemFactory *factory;
  GListStore *store;
  GListModel *paper_size_list;
  BobguiPrinter *printer;
  GListStore *printer_list;
  GListStore *printer_list_list;
  GListModel *full_list;
  BobguiFilter *filter;
  BobguiPageSetup *page_setup;

  dialog->internal_change = TRUE;
  dialog->print_backends = NULL;

  bobgui_widget_init_template (BOBGUI_WIDGET (dialog));

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  bobgui_dialog_set_use_header_bar_from_setting (BOBGUI_DIALOG (dialog));
  bobgui_dialog_add_buttons (BOBGUI_DIALOG (dialog),
                          _("_Cancel"), BOBGUI_RESPONSE_CANCEL,
                          _("_Apply"), BOBGUI_RESPONSE_OK,
                          NULL);
  bobgui_dialog_set_default_response (BOBGUI_DIALOG (dialog), BOBGUI_RESPONSE_OK);
G_GNUC_END_IGNORE_DEPRECATIONS

  dialog->page_setup_list = g_list_store_new (BOBGUI_TYPE_PAGE_SETUP);
  dialog->custom_paper_list = g_list_store_new (BOBGUI_TYPE_PAGE_SETUP);
  dialog->manage_papers_list = g_list_store_new (BOBGUI_TYPE_PAGE_SETUP);
  page_setup = bobgui_page_setup_new ();
  g_list_store_append (dialog->manage_papers_list, page_setup);
  g_object_unref (page_setup);

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

  /* Do this in code, we want the translatable strings without the markup */
  printer_list_list = g_list_store_new (G_TYPE_LIST_MODEL);
  printer_list = g_list_store_new (BOBGUI_TYPE_PRINTER);
  printer = bobgui_printer_new (_("Any Printer"), NULL, FALSE);
  bobgui_printer_set_location (printer, _("For portable documents"));
  g_list_store_append (printer_list, printer);
  g_object_unref (printer);
  g_list_store_append (printer_list_list, printer_list);
  g_object_unref (printer_list);

  full_list = G_LIST_MODEL (bobgui_flatten_list_model_new (G_LIST_MODEL (printer_list_list)));

  filter = BOBGUI_FILTER (bobgui_custom_filter_new (match_func, NULL, NULL));
  dialog->printer_list = G_LIST_MODEL (bobgui_filter_list_model_new (full_list, filter));

  factory = bobgui_signal_list_item_factory_new ();
  g_signal_connect (factory, "setup", G_CALLBACK (setup_printer_item), dialog);
  g_signal_connect (factory, "bind", G_CALLBACK (bind_printer_item), dialog);
  bobgui_drop_down_set_factory (BOBGUI_DROP_DOWN (dialog->printer_combo), factory);
  g_object_unref (factory);

  bobgui_drop_down_set_model (BOBGUI_DROP_DOWN (dialog->printer_combo), dialog->printer_list);
  printer_changed_callback (BOBGUI_DROP_DOWN (dialog->printer_combo), NULL, dialog);

  /* Load data */
  bobgui_print_load_custom_papers (dialog->custom_paper_list);
  load_print_backends (dialog);
  dialog->internal_change = FALSE;
}

static void
bobgui_page_setup_unix_dialog_finalize (GObject *object)
{
  BobguiPageSetupUnixDialog *dialog = BOBGUI_PAGE_SETUP_UNIX_DIALOG (object);
  GList *node;

  if (dialog->request_details_tag)
    {
      g_signal_handler_disconnect (dialog->request_details_printer,
                                   dialog->request_details_tag);
      g_object_unref (dialog->request_details_printer);
      dialog->request_details_printer = NULL;
      dialog->request_details_tag = 0;
    }

  g_clear_object (&dialog->printer_list);
  g_clear_object (&dialog->page_setup_list);
  g_clear_object (&dialog->custom_paper_list);
  g_clear_object (&dialog->manage_papers_list);

  if (dialog->print_settings)
    {
      g_object_unref (dialog->print_settings);
      dialog->print_settings = NULL;
    }

  for (node = dialog->print_backends; node != NULL; node = node->next)
    bobgui_print_backend_destroy (BOBGUI_PRINT_BACKEND (node->data));
  g_list_free_full (dialog->print_backends, g_object_unref);
  dialog->print_backends = NULL;

  G_OBJECT_CLASS (bobgui_page_setup_unix_dialog_parent_class)->finalize (object);
}

static void
load_print_backends (BobguiPageSetupUnixDialog *dialog)
{
  GListModel *full_list;
  GListStore *printer_list_list;
  GList *node;

  full_list = bobgui_filter_list_model_get_model (BOBGUI_FILTER_LIST_MODEL (dialog->printer_list));
  printer_list_list = G_LIST_STORE (bobgui_flatten_list_model_get_model (BOBGUI_FLATTEN_LIST_MODEL (full_list)));

  if (g_module_supported ())
    dialog->print_backends = bobgui_print_backend_load_modules ();

  for (node = dialog->print_backends; node != NULL; node = node->next)
    {
      BobguiPrintBackend *backend = node->data;
      g_list_store_append (printer_list_list, bobgui_print_backend_get_printers (backend));
    }
}

static BobguiPageSetup *
get_current_page_setup (BobguiPageSetupUnixDialog *dialog)
{
  guint selected;
  GListModel *model;

  selected = bobgui_drop_down_get_selected (BOBGUI_DROP_DOWN (dialog->paper_size_combo));
  model = bobgui_drop_down_get_model (BOBGUI_DROP_DOWN (dialog->paper_size_combo));
  if (selected != BOBGUI_INVALID_LIST_POSITION)
    return g_list_model_get_item (model, selected);

  return bobgui_page_setup_new ();
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
set_paper_size (BobguiPageSetupUnixDialog *dialog,
                BobguiPageSetup           *page_setup,
                gboolean                size_only,
                gboolean                add_item)
{
  GListModel *model;
  BobguiPageSetup *list_page_setup;
  guint i;

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
      return TRUE;
    }

  return FALSE;
}

static void
fill_paper_sizes_from_printer (BobguiPageSetupUnixDialog *dialog,
                               BobguiPrinter             *printer)
{
  GList *list, *l;
  BobguiPageSetup *current_page_setup, *page_setup;
  BobguiPaperSize *paper_size;
  int i;

  g_list_store_remove_all (dialog->page_setup_list);

  if (printer == NULL)
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
      list = bobgui_printer_list_papers (printer);
      /* TODO: We should really sort this list so interesting size
         are at the top */
      for (l = list; l != NULL; l = l->next)
        {
          page_setup = l->data;
          g_list_store_append (dialog->page_setup_list, page_setup);
          g_object_unref (page_setup);
        }
      g_list_free (list);
    }

  current_page_setup = NULL;

  /* When selecting a different printer, select its default paper size */
  if (printer != NULL)
    current_page_setup = bobgui_printer_get_default_page_size (printer);

  if (current_page_setup == NULL)
    current_page_setup = get_current_page_setup (dialog);

  if (!set_paper_size (dialog, current_page_setup, FALSE, FALSE))
    set_paper_size (dialog, current_page_setup, TRUE, TRUE);

  if (current_page_setup)
    g_object_unref (current_page_setup);
}

static void
printer_changed_finished_callback (BobguiPrinter             *printer,
                                   gboolean                success,
                                   BobguiPageSetupUnixDialog *dialog)
{
  g_signal_handler_disconnect (dialog->request_details_printer,
                               dialog->request_details_tag);
  g_object_unref (dialog->request_details_printer);
  dialog->request_details_tag = 0;
  dialog->request_details_printer = NULL;

  if (success)
    fill_paper_sizes_from_printer (dialog, printer);

}

static void
printer_changed_callback (BobguiDropDown            *combo_box,
                          GParamSpec             *pspec,
                          BobguiPageSetupUnixDialog *dialog)
{
  BobguiPrinter *printer;
  guint selected;

  if (dialog->request_details_tag)
    {
      g_signal_handler_disconnect (dialog->request_details_printer,
                                   dialog->request_details_tag);
      g_object_unref (dialog->request_details_printer);
      dialog->request_details_printer = NULL;
      dialog->request_details_tag = 0;
    }

  selected = bobgui_drop_down_get_selected (BOBGUI_DROP_DOWN (dialog->printer_combo));
  if (selected != BOBGUI_INVALID_LIST_POSITION)
    {
      GListModel *model;

      model = bobgui_drop_down_get_model (BOBGUI_DROP_DOWN (dialog->printer_combo));
      printer = g_list_model_get_item (model, selected);
      if (strcmp (bobgui_printer_get_name (printer), _("Any Printer")) == 0)
        g_clear_object (&printer);

      if (printer == NULL ||
          bobgui_printer_has_details (printer))
        fill_paper_sizes_from_printer (dialog, printer);
      else
        {
          dialog->request_details_printer = g_object_ref (printer);
          dialog->request_details_tag =
            g_signal_connect (printer, "details-acquired",
                              G_CALLBACK (printer_changed_finished_callback), dialog);
          bobgui_printer_request_details (printer);
        }

      if (printer)
        g_object_unref (printer);

      if (dialog->print_settings)
        {
          const char *name = NULL;

          if (printer)
            name = bobgui_printer_get_name (printer);

          bobgui_print_settings_set (dialog->print_settings,
                                  "format-for-printer", name);
        }
    }
}

/* We do this munging because we don't want to show zero digits
   after the decimal point, and not to many such digits if they
   are nonzero. I wish printf let you specify max precision for %f... */
static char *
double_to_string (double d,
                  BobguiUnit unit)
{
  char *val, *p;
  struct lconv *locale_data;
  const char *decimal_point;
  int decimal_point_len;

  locale_data = localeconv ();
  decimal_point = locale_data->decimal_point;
  decimal_point_len = strlen (decimal_point);

  /* Max two decimal digits for inch, max one for mm */
  if (unit == BOBGUI_UNIT_INCH)
    val = g_strdup_printf ("%.2f", d);
  else
    val = g_strdup_printf ("%.1f", d);

  if (strstr (val, decimal_point))
    {
      p = val + strlen (val) - 1;
      while (*p == '0')
        p--;
      if (p - val + 1 >= decimal_point_len &&
          strncmp (p - (decimal_point_len - 1), decimal_point, decimal_point_len) == 0)
        p -= decimal_point_len;
      p[1] = '\0';
    }

  return val;
}


static void
custom_paper_dialog_response_cb (BobguiDialog *custom_paper_dialog,
                                 int        response_id,
                                 gpointer   user_data)
{
  BobguiPageSetupUnixDialog *dialog = BOBGUI_PAGE_SETUP_UNIX_DIALOG (user_data);
  BobguiPageSetup *last_page_setup;

  dialog->internal_change = TRUE;
  bobgui_print_load_custom_papers (dialog->custom_paper_list);
  printer_changed_callback (BOBGUI_DROP_DOWN (dialog->printer_combo), NULL, dialog);
  dialog->internal_change = FALSE;

  if (dialog->last_setup)
    last_page_setup = g_object_ref (dialog->last_setup);
  else
    last_page_setup = bobgui_page_setup_new (); /* "good" default */
  set_paper_size (dialog, last_page_setup, FALSE, TRUE);
  g_object_unref (last_page_setup);

  bobgui_window_destroy (BOBGUI_WINDOW (custom_paper_dialog));
}

static void
paper_size_changed (BobguiDropDown            *combo_box,
                    GParamSpec             *pspec,
                    BobguiPageSetupUnixDialog *dialog)
{
  BobguiPageSetup *page_setup, *last_page_setup;
  guint selected;
  BobguiUnit unit;
  char *str, *w, *h;
  char *top, *bottom, *left, *right;
  BobguiLabel *label;
  const char *unit_str;

  if (dialog->internal_change)
    return;

  label = BOBGUI_LABEL (dialog->paper_size_label);

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
          if (dialog->last_setup)
            last_page_setup = g_object_ref (dialog->last_setup);
          else
            last_page_setup = bobgui_page_setup_new (); /* "good" default */
          set_paper_size (dialog, last_page_setup, FALSE, TRUE);
          g_object_unref (last_page_setup);

          /* And show the custom paper dialog */
          custom_paper_dialog = _bobgui_custom_paper_unix_dialog_new (BOBGUI_WINDOW (dialog), NULL);
          g_signal_connect (custom_paper_dialog, "response", G_CALLBACK (custom_paper_dialog_response_cb), dialog);
          bobgui_window_present (BOBGUI_WINDOW (custom_paper_dialog));

          g_object_unref (page_setup);

          return;
        }

      if (dialog->last_setup)
        g_object_unref (dialog->last_setup);

      dialog->last_setup = g_object_ref (page_setup);

      unit = _bobgui_print_get_default_user_units ();

      if (unit == BOBGUI_UNIT_MM)
        unit_str = _("mm");
      else
        unit_str = _("inch");

      w = double_to_string (bobgui_page_setup_get_paper_width (page_setup, unit),
                            unit);
      h = double_to_string (bobgui_page_setup_get_paper_height (page_setup, unit),
                            unit);
      str = g_strdup_printf ("%s × %s %s", w, h, unit_str);
      g_free (w);
      g_free (h);

      bobgui_label_set_text (label, str);
      g_free (str);

      top = double_to_string (bobgui_page_setup_get_top_margin (page_setup, unit), unit);
      bottom = double_to_string (bobgui_page_setup_get_bottom_margin (page_setup, unit), unit);
      left = double_to_string (bobgui_page_setup_get_left_margin (page_setup, unit), unit);
      right = double_to_string (bobgui_page_setup_get_right_margin (page_setup, unit), unit);

      str = g_strdup_printf (_("Margins:\n"
                               " Left: %s %s\n"
                               " Right: %s %s\n"
                               " Top: %s %s\n"
                               " Bottom: %s %s"
                               ),
                             left, unit_str,
                             right, unit_str,
                             top, unit_str,
                             bottom, unit_str);
      g_free (top);
      g_free (bottom);
      g_free (left);
      g_free (right);

      bobgui_widget_set_tooltip_text (dialog->paper_size_label, str);
      g_free (str);

      g_object_unref (page_setup);
    }
  else
    {
      bobgui_label_set_text (label, "");
      bobgui_widget_set_tooltip_text (dialog->paper_size_label, NULL);
      if (dialog->last_setup)
        g_object_unref (dialog->last_setup);
      dialog->last_setup = NULL;
    }
}

/**
 * bobgui_page_setup_unix_dialog_new:
 * @title: (nullable): the title of the dialog
 * @parent: (nullable): transient parent of the dialog
 *
 * Creates a new page setup dialog.
 *
 * Returns: the new `BobguiPageSetupUnixDialog`
 */
BobguiWidget *
bobgui_page_setup_unix_dialog_new (const char *title,
                                BobguiWindow   *parent)
{
  BobguiWidget *result;

  if (title == NULL)
    title = _("Page Setup");

  result = g_object_new (BOBGUI_TYPE_PAGE_SETUP_UNIX_DIALOG,
                         "title", title,
                         NULL);

  if (parent)
    bobgui_window_set_transient_for (BOBGUI_WINDOW (result), parent);

  return result;
}

static BobguiPageOrientation
get_orientation (BobguiPageSetupUnixDialog *dialog)
{
  if (bobgui_check_button_get_active (BOBGUI_CHECK_BUTTON (dialog->portrait_radio)))
    return BOBGUI_PAGE_ORIENTATION_PORTRAIT;
  if (bobgui_check_button_get_active (BOBGUI_CHECK_BUTTON (dialog->landscape_radio)))
    return BOBGUI_PAGE_ORIENTATION_LANDSCAPE;
  if (bobgui_check_button_get_active (BOBGUI_CHECK_BUTTON (dialog->reverse_landscape_radio)))
    return BOBGUI_PAGE_ORIENTATION_REVERSE_LANDSCAPE;
  return BOBGUI_PAGE_ORIENTATION_REVERSE_PORTRAIT;
}

static void
set_orientation (BobguiPageSetupUnixDialog *dialog,
                 BobguiPageOrientation      orientation)
{
  switch (orientation)
    {
    case BOBGUI_PAGE_ORIENTATION_REVERSE_PORTRAIT:
      bobgui_check_button_set_active (BOBGUI_CHECK_BUTTON (dialog->reverse_portrait_radio), TRUE);
      break;
    case BOBGUI_PAGE_ORIENTATION_PORTRAIT:
      bobgui_check_button_set_active (BOBGUI_CHECK_BUTTON (dialog->portrait_radio), TRUE);
      break;
    case BOBGUI_PAGE_ORIENTATION_LANDSCAPE:
      bobgui_check_button_set_active (BOBGUI_CHECK_BUTTON (dialog->landscape_radio), TRUE);
      break;
    case BOBGUI_PAGE_ORIENTATION_REVERSE_LANDSCAPE:
      bobgui_check_button_set_active (BOBGUI_CHECK_BUTTON (dialog->reverse_landscape_radio), TRUE);
      break;
    default:
      break;
    }
}

/**
 * bobgui_page_setup_unix_dialog_set_page_setup:
 * @dialog: a `BobguiPageSetupUnixDialog`
 * @page_setup: a `BobguiPageSetup`
 *
 * Sets the `BobguiPageSetup` from which the page setup
 * dialog takes its values.
 */
void
bobgui_page_setup_unix_dialog_set_page_setup (BobguiPageSetupUnixDialog *dialog,
                                           BobguiPageSetup           *page_setup)
{
  if (page_setup)
    {
      set_paper_size (dialog, page_setup, FALSE, TRUE);
      set_orientation (dialog, bobgui_page_setup_get_orientation (page_setup));
    }
}

/**
 * bobgui_page_setup_unix_dialog_get_page_setup:
 * @dialog: a `BobguiPageSetupUnixDialog`
 *
 * Gets the currently selected page setup from the dialog.
 *
 * Returns: (transfer none): the current page setup
 */
BobguiPageSetup *
bobgui_page_setup_unix_dialog_get_page_setup (BobguiPageSetupUnixDialog *dialog)
{
  BobguiPageSetup *page_setup;

  page_setup = get_current_page_setup (dialog);

  bobgui_page_setup_set_orientation (page_setup, get_orientation (dialog));

  return page_setup;
}

static gboolean
set_active_printer (BobguiPageSetupUnixDialog *dialog,
                    const char             *printer_name)
{
  guint i, n;
  BobguiPrinter *printer;

  if (!printer_name)
    return FALSE;

  for (i = 0, n = g_list_model_get_n_items (dialog->printer_list); i < n; i++)
    {
      printer = g_list_model_get_item (dialog->printer_list, i);

      if (strcmp (bobgui_printer_get_name (printer), printer_name) == 0)
        {
          bobgui_drop_down_set_selected (BOBGUI_DROP_DOWN (dialog->printer_combo), i);
          g_object_unref (printer);

          return TRUE;
        }

      g_object_unref (printer);
    }

  return FALSE;
}

/**
 * bobgui_page_setup_unix_dialog_set_print_settings:
 * @dialog: a `BobguiPageSetupUnixDialog`
 * @print_settings: (nullable): a `BobguiPrintSettings`
 *
 * Sets the `BobguiPrintSettings` from which the page setup dialog
 * takes its values.
 */
void
bobgui_page_setup_unix_dialog_set_print_settings (BobguiPageSetupUnixDialog *dialog,
                                               BobguiPrintSettings       *print_settings)
{
  const char *format_for_printer;

  if (dialog->print_settings == print_settings) return;

  if (dialog->print_settings)
    g_object_unref (dialog->print_settings);

  dialog->print_settings = print_settings;

  if (print_settings)
    {
      g_object_ref (print_settings);

      format_for_printer = bobgui_print_settings_get (print_settings, "format-for-printer");

      /* Set printer if in list, otherwise set when
       * that printer is added
       */
      set_active_printer (dialog, format_for_printer);
    }
}

/**
 * bobgui_page_setup_unix_dialog_get_print_settings:
 * @dialog: a `BobguiPageSetupUnixDialog`
 *
 * Gets the current print settings from the dialog.
 *
 * Returns: (transfer none) (nullable): the current print settings
 **/
BobguiPrintSettings *
bobgui_page_setup_unix_dialog_get_print_settings (BobguiPageSetupUnixDialog *dialog)
{
  return dialog->print_settings;
}
