/* BOBGUI - The Bobgui Framework
 * bobguinativedialog.h: Native dialog
 * Copyright (C) 2015, Red Hat, Inc.
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

#include <bobgui/bobguiwindow.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_NATIVE_DIALOG             (bobgui_native_dialog_get_type ())

GDK_AVAILABLE_IN_ALL
G_DECLARE_DERIVABLE_TYPE (BobguiNativeDialog, bobgui_native_dialog, BOBGUI, NATIVE_DIALOG, GObject)

/**
 * BobguiNativeDialogClass:
 * @response: class handler for the `BobguiNativeDialog::response` signal
 *
 * Class structure for `BobguiNativeDialog`.
 */
struct _BobguiNativeDialogClass
{
  /*< private >*/
  GObjectClass parent_class;

  /*< public >*/
  void (* response) (BobguiNativeDialog *self, int response_id);

  /* <private> */
  void (* show) (BobguiNativeDialog *self);
  void (* hide) (BobguiNativeDialog *self);

  /* Padding for future expansion */
  void (*_bobgui_reserved1) (void);
  void (*_bobgui_reserved2) (void);
  void (*_bobgui_reserved3) (void);
  void (*_bobgui_reserved4) (void);
};

GDK_AVAILABLE_IN_ALL
void                  bobgui_native_dialog_show (BobguiNativeDialog *self);
GDK_AVAILABLE_IN_ALL
void                  bobgui_native_dialog_hide (BobguiNativeDialog *self);
GDK_AVAILABLE_IN_ALL
void                  bobgui_native_dialog_destroy (BobguiNativeDialog *self);
GDK_AVAILABLE_IN_ALL
gboolean              bobgui_native_dialog_get_visible (BobguiNativeDialog *self);
GDK_AVAILABLE_IN_ALL
void                  bobgui_native_dialog_set_modal (BobguiNativeDialog *self,
                                                   gboolean modal);
GDK_AVAILABLE_IN_ALL
gboolean              bobgui_native_dialog_get_modal (BobguiNativeDialog *self);
GDK_AVAILABLE_IN_ALL
void                  bobgui_native_dialog_set_title (BobguiNativeDialog *self,
                                                   const char *title);
GDK_AVAILABLE_IN_ALL
const char *          bobgui_native_dialog_get_title (BobguiNativeDialog *self);
GDK_AVAILABLE_IN_ALL
void                  bobgui_native_dialog_set_transient_for (BobguiNativeDialog *self,
                                                           BobguiWindow *parent);
GDK_AVAILABLE_IN_ALL
BobguiWindow *           bobgui_native_dialog_get_transient_for (BobguiNativeDialog *self);

G_END_DECLS

