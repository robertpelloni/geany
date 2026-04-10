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

G_BEGIN_DECLS

#define BOBGUI_TYPE_ALERT_DIALOG (bobgui_alert_dialog_get_type ())

GDK_AVAILABLE_IN_4_10
G_DECLARE_FINAL_TYPE (BobguiAlertDialog, bobgui_alert_dialog, BOBGUI, ALERT_DIALOG, GObject)

GDK_AVAILABLE_IN_4_10
BobguiAlertDialog *bobgui_alert_dialog_new               (const char           *format,
                                                    ...) G_GNUC_PRINTF (1, 2);

GDK_AVAILABLE_IN_4_10
gboolean        bobgui_alert_dialog_get_modal          (BobguiAlertDialog      *self);

GDK_AVAILABLE_IN_4_10
void            bobgui_alert_dialog_set_modal          (BobguiAlertDialog      *self,
                                                     gboolean             modal);

GDK_AVAILABLE_IN_4_10
const char *    bobgui_alert_dialog_get_message        (BobguiAlertDialog      *self);

GDK_AVAILABLE_IN_4_10
void            bobgui_alert_dialog_set_message        (BobguiAlertDialog      *self,
                                                     const char          *message);

GDK_AVAILABLE_IN_4_10
const char *    bobgui_alert_dialog_get_detail         (BobguiAlertDialog      *self);

GDK_AVAILABLE_IN_4_10
void            bobgui_alert_dialog_set_detail         (BobguiAlertDialog      *self,
                                                     const char          *detail);

GDK_AVAILABLE_IN_4_10
const char * const *
                bobgui_alert_dialog_get_buttons        (BobguiAlertDialog      *self);

GDK_AVAILABLE_IN_4_10
void            bobgui_alert_dialog_set_buttons        (BobguiAlertDialog      *self,
                                                     const char * const  *labels);

GDK_AVAILABLE_IN_4_10
int             bobgui_alert_dialog_get_cancel_button  (BobguiAlertDialog      *self);

GDK_AVAILABLE_IN_4_10
void            bobgui_alert_dialog_set_cancel_button  (BobguiAlertDialog      *self,
                                                     int                  button);
GDK_AVAILABLE_IN_4_10
int             bobgui_alert_dialog_get_default_button (BobguiAlertDialog      *self);

GDK_AVAILABLE_IN_4_10
void            bobgui_alert_dialog_set_default_button (BobguiAlertDialog      *self,
                                                     int                  button);

GDK_AVAILABLE_IN_4_10
void            bobgui_alert_dialog_choose             (BobguiAlertDialog      *self,
                                                     BobguiWindow           *parent,
                                                     GCancellable        *cancellable,
                                                     GAsyncReadyCallback  callback,
                                                     gpointer             user_data);

GDK_AVAILABLE_IN_4_10
int             bobgui_alert_dialog_choose_finish      (BobguiAlertDialog      *self,
                                                     GAsyncResult        *result,
                                                     GError             **error);

GDK_AVAILABLE_IN_4_10
void            bobgui_alert_dialog_show               (BobguiAlertDialog      *self,
                                                     BobguiWindow           *parent);

G_END_DECLS
