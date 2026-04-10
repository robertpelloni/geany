/* BobguiPrintUnixDialog
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

#include <bobgui/bobgui.h>
#include <bobgui/print/bobguiprinter.h>
#include <bobgui/print/bobguiprintjob.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_PRINT_UNIX_DIALOG                  (bobgui_print_unix_dialog_get_type ())
#define BOBGUI_PRINT_UNIX_DIALOG(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_PRINT_UNIX_DIALOG, BobguiPrintUnixDialog))
#define BOBGUI_IS_PRINT_UNIX_DIALOG(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_PRINT_UNIX_DIALOG))


typedef struct _BobguiPrintUnixDialog BobguiPrintUnixDialog;


GDK_AVAILABLE_IN_ALL
GType                bobgui_print_unix_dialog_get_type                (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
BobguiWidget *          bobgui_print_unix_dialog_new                     (const char *title,
                                                                    BobguiWindow   *parent);

GDK_AVAILABLE_IN_ALL
void                 bobgui_print_unix_dialog_set_page_setup          (BobguiPrintUnixDialog *dialog,
								    BobguiPageSetup       *page_setup);
GDK_AVAILABLE_IN_ALL
BobguiPageSetup *       bobgui_print_unix_dialog_get_page_setup          (BobguiPrintUnixDialog *dialog);
GDK_AVAILABLE_IN_ALL
void                 bobgui_print_unix_dialog_set_current_page        (BobguiPrintUnixDialog *dialog,
								    int                 current_page);
GDK_AVAILABLE_IN_ALL
int                  bobgui_print_unix_dialog_get_current_page        (BobguiPrintUnixDialog *dialog);
GDK_AVAILABLE_IN_ALL
void                 bobgui_print_unix_dialog_set_settings            (BobguiPrintUnixDialog *dialog,
								    BobguiPrintSettings   *settings);
GDK_AVAILABLE_IN_ALL
BobguiPrintSettings *   bobgui_print_unix_dialog_get_settings            (BobguiPrintUnixDialog *dialog);
GDK_AVAILABLE_IN_ALL
BobguiPrinter *         bobgui_print_unix_dialog_get_selected_printer    (BobguiPrintUnixDialog *dialog);
GDK_AVAILABLE_IN_ALL
void                 bobgui_print_unix_dialog_add_custom_tab          (BobguiPrintUnixDialog *dialog,
								    BobguiWidget          *child,
								    BobguiWidget          *tab_label);
GDK_AVAILABLE_IN_ALL
void                 bobgui_print_unix_dialog_set_manual_capabilities (BobguiPrintUnixDialog *dialog,
								    BobguiPrintCapabilities capabilities);
GDK_AVAILABLE_IN_ALL
BobguiPrintCapabilities bobgui_print_unix_dialog_get_manual_capabilities (BobguiPrintUnixDialog  *dialog);
GDK_AVAILABLE_IN_ALL
void                 bobgui_print_unix_dialog_set_support_selection   (BobguiPrintUnixDialog  *dialog,
								    gboolean             support_selection);
GDK_AVAILABLE_IN_ALL
gboolean             bobgui_print_unix_dialog_get_support_selection   (BobguiPrintUnixDialog  *dialog);
GDK_AVAILABLE_IN_ALL
void                 bobgui_print_unix_dialog_set_has_selection       (BobguiPrintUnixDialog  *dialog,
								    gboolean             has_selection);
GDK_AVAILABLE_IN_ALL
gboolean             bobgui_print_unix_dialog_get_has_selection       (BobguiPrintUnixDialog  *dialog);
GDK_AVAILABLE_IN_ALL
void                 bobgui_print_unix_dialog_set_embed_page_setup    (BobguiPrintUnixDialog *dialog,
								    gboolean            embed);
GDK_AVAILABLE_IN_ALL
gboolean             bobgui_print_unix_dialog_get_embed_page_setup    (BobguiPrintUnixDialog *dialog);
GDK_AVAILABLE_IN_ALL
gboolean             bobgui_print_unix_dialog_get_page_setup_set      (BobguiPrintUnixDialog *dialog);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiPrintUnixDialog, g_object_unref)

G_END_DECLS

