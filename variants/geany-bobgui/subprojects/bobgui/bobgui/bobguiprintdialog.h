/* BOBGUI - The Bobgui Framework
 *
 * Copyright (C) 2022 Red Hat, Inc.
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

#include <gdk/gdk.h>
#include <bobgui/bobguiwindow.h>
#include <bobgui/print/bobguiprintsettings.h>
#include <bobgui/print/bobguipagesetup.h>

G_BEGIN_DECLS

typedef struct _BobguiPrintSetup BobguiPrintSetup;

#define BOBGUI_TYPE_PRINT_SETUP (bobgui_print_setup_get_type ())

GDK_AVAILABLE_IN_4_14
GType           bobgui_print_setup_get_type                (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_4_14
BobguiPrintSetup  *bobgui_print_setup_ref                     (BobguiPrintSetup        *setup);

GDK_AVAILABLE_IN_4_14
void            bobgui_print_setup_unref                   (BobguiPrintSetup        *setup);

GDK_AVAILABLE_IN_4_14
BobguiPrintSettings *
                bobgui_print_setup_get_print_settings      (BobguiPrintSetup        *setup);

GDK_AVAILABLE_IN_4_14
BobguiPageSetup *  bobgui_print_setup_get_page_setup          (BobguiPrintSetup        *setup);


#define BOBGUI_TYPE_PRINT_DIALOG (bobgui_print_dialog_get_type ())

GDK_AVAILABLE_IN_4_14
G_DECLARE_FINAL_TYPE (BobguiPrintDialog, bobgui_print_dialog, BOBGUI, PRINT_DIALOG, GObject)

GDK_AVAILABLE_IN_4_14
BobguiPrintDialog *bobgui_print_dialog_new                    (void);

GDK_AVAILABLE_IN_4_14
const char *    bobgui_print_dialog_get_title              (BobguiPrintDialog       *self);

GDK_AVAILABLE_IN_4_14
void            bobgui_print_dialog_set_title              (BobguiPrintDialog       *self,
                                                         const char           *title);

GDK_AVAILABLE_IN_4_14
const char *    bobgui_print_dialog_get_accept_label       (BobguiPrintDialog       *self);

GDK_AVAILABLE_IN_4_14
void            bobgui_print_dialog_set_accept_label       (BobguiPrintDialog       *self,
                                                         const char           *accept_label);

GDK_AVAILABLE_IN_4_14
gboolean        bobgui_print_dialog_get_modal              (BobguiPrintDialog       *self);

GDK_AVAILABLE_IN_4_14
void            bobgui_print_dialog_set_modal              (BobguiPrintDialog       *self,
                                                         gboolean              modal);

GDK_AVAILABLE_IN_4_14
BobguiPageSetup *  bobgui_print_dialog_get_page_setup         (BobguiPrintDialog       *self);

GDK_AVAILABLE_IN_4_14
void            bobgui_print_dialog_set_page_setup         (BobguiPrintDialog       *self,
                                                         BobguiPageSetup         *page_setup);

GDK_AVAILABLE_IN_4_14
BobguiPrintSettings * bobgui_print_dialog_get_print_settings  (BobguiPrintDialog       *self);

GDK_AVAILABLE_IN_4_14
void               bobgui_print_dialog_set_print_settings  (BobguiPrintDialog       *self,
                                                         BobguiPrintSettings     *print_settings);

GDK_AVAILABLE_IN_4_14
void            bobgui_print_dialog_setup                  (BobguiPrintDialog       *self,
                                                         BobguiWindow            *parent,
                                                         GCancellable         *cancellable,
                                                         GAsyncReadyCallback   callback,
                                                         gpointer              user_data);

GDK_AVAILABLE_IN_4_14
BobguiPrintSetup  *bobgui_print_dialog_setup_finish           (BobguiPrintDialog       *self,
                                                         GAsyncResult         *result,
                                                         GError              **error);

GDK_AVAILABLE_IN_4_14
void            bobgui_print_dialog_print                  (BobguiPrintDialog       *self,
                                                         BobguiWindow            *parent,
                                                         BobguiPrintSetup        *setup,
                                                         GCancellable         *cancellable,
                                                         GAsyncReadyCallback   callback,
                                                         gpointer              user_data);

GDK_AVAILABLE_IN_4_14
GOutputStream * bobgui_print_dialog_print_finish           (BobguiPrintDialog       *self,
                                                         GAsyncResult         *result,
                                                         GError              **error);

GDK_AVAILABLE_IN_4_14
void            bobgui_print_dialog_print_file             (BobguiPrintDialog       *self,
                                                         BobguiWindow            *parent,
                                                         BobguiPrintSetup        *setup,
                                                         GFile                *file,
                                                         GCancellable         *cancellable,
                                                         GAsyncReadyCallback   callback,
                                                         gpointer              user_data);

GDK_AVAILABLE_IN_4_14
gboolean        bobgui_print_dialog_print_file_finish      (BobguiPrintDialog       *self,
                                                         GAsyncResult         *result,
                                                         GError              **error);

G_END_DECLS
