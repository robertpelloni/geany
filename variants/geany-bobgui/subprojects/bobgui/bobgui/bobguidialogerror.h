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

G_BEGIN_DECLS

/**
 * BOBGUI_DIALOG_ERROR:
 *
 * The error domain for errors returned by async dialog functions.
 *
 * Since: 4.10
 */
#define BOBGUI_DIALOG_ERROR (bobgui_dialog_error_quark ())

/**
 * BobguiDialogError:
 * @BOBGUI_DIALOG_ERROR_FAILED: Generic error condition for when
 *   an operation fails and no more specific code is applicable
 * @BOBGUI_DIALOG_ERROR_CANCELLED: The async function call was cancelled
 *   via its `GCancellable`
 * @BOBGUI_DIALOG_ERROR_DISMISSED: The operation was cancelled
 *   by the user (via a Cancel or Close button)
 *
 * Error codes in the `BOBGUI_DIALOG_ERROR` domain that can be returned
 * by async dialog functions.
 *
 * Since: 4.10
 */
typedef enum
{
  BOBGUI_DIALOG_ERROR_FAILED,
  BOBGUI_DIALOG_ERROR_CANCELLED,
  BOBGUI_DIALOG_ERROR_DISMISSED
} BobguiDialogError;

GDK_AVAILABLE_IN_4_10
GQuark bobgui_dialog_error_quark (void);

G_END_DECLS
