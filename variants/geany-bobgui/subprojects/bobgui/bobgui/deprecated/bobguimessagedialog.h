/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2000 Red Hat, Inc.
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

/*
 * Modified by the BOBGUI Team and others 1997-2003.  See the AUTHORS
 * file for a list of people on the BOBGUI Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BOBGUI at ftp://ftp.bobgui.org/pub/bobgui/.
 */

#pragma once

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <bobgui/deprecated/bobguidialog.h>
#include <bobgui/bobguienums.h>

G_BEGIN_DECLS


#define BOBGUI_TYPE_MESSAGE_DIALOG                  (bobgui_message_dialog_get_type ())
#define BOBGUI_MESSAGE_DIALOG(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_MESSAGE_DIALOG, BobguiMessageDialog))
#define BOBGUI_IS_MESSAGE_DIALOG(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_MESSAGE_DIALOG))

typedef struct _BobguiMessageDialog              BobguiMessageDialog;
typedef struct _BobguiMessageDialogClass         BobguiMessageDialogClass;

struct _BobguiMessageDialog
{
  BobguiDialog parent_instance;
};

/**
 * BobguiButtonsType:
 * @BOBGUI_BUTTONS_NONE: no buttons at all
 * @BOBGUI_BUTTONS_OK: an OK button
 * @BOBGUI_BUTTONS_CLOSE: a Close button
 * @BOBGUI_BUTTONS_CANCEL: a Cancel button
 * @BOBGUI_BUTTONS_YES_NO: Yes and No buttons
 * @BOBGUI_BUTTONS_OK_CANCEL: OK and Cancel buttons
 *
 * Prebuilt sets of buttons for `BobguiDialog`.
 *
 * If none of these choices are appropriate, simply use
 * %BOBGUI_BUTTONS_NONE and call [method@Bobgui.Dialog.add_buttons].
 *
 * > Please note that %BOBGUI_BUTTONS_OK, %BOBGUI_BUTTONS_YES_NO
 * > and %BOBGUI_BUTTONS_OK_CANCEL are discouraged by the
 * > [GNOME Human Interface Guidelines](https://developer.gnome.org/hig/).
 */
typedef enum
{
  BOBGUI_BUTTONS_NONE,
  BOBGUI_BUTTONS_OK,
  BOBGUI_BUTTONS_CLOSE,
  BOBGUI_BUTTONS_CANCEL,
  BOBGUI_BUTTONS_YES_NO,
  BOBGUI_BUTTONS_OK_CANCEL
} BobguiButtonsType;

GDK_AVAILABLE_IN_ALL
GType      bobgui_message_dialog_get_type (void) G_GNUC_CONST;

GDK_DEPRECATED_IN_4_10
BobguiWidget* bobgui_message_dialog_new      (BobguiWindow      *parent,
                                        BobguiDialogFlags  flags,
                                        BobguiMessageType  type,
                                        BobguiButtonsType  buttons,
                                        const char     *message_format,
                                        ...) G_GNUC_PRINTF (5, 6);

GDK_DEPRECATED_IN_4_10
BobguiWidget* bobgui_message_dialog_new_with_markup   (BobguiWindow      *parent,
                                                 BobguiDialogFlags  flags,
                                                 BobguiMessageType  type,
                                                 BobguiButtonsType  buttons,
                                                 const char     *message_format,
                                                 ...) G_GNUC_PRINTF (5, 6);

GDK_DEPRECATED_IN_4_10
void       bobgui_message_dialog_set_markup  (BobguiMessageDialog *message_dialog,
                                           const char       *str);

GDK_DEPRECATED_IN_4_10
void       bobgui_message_dialog_format_secondary_text (BobguiMessageDialog *message_dialog,
                                                     const char       *message_format,
                                                     ...) G_GNUC_PRINTF (2, 3);

GDK_DEPRECATED_IN_4_10
void       bobgui_message_dialog_format_secondary_markup (BobguiMessageDialog *message_dialog,
                                                       const char       *message_format,
                                                       ...) G_GNUC_PRINTF (2, 3);

GDK_DEPRECATED_IN_4_10
BobguiWidget *bobgui_message_dialog_get_message_area (BobguiMessageDialog *message_dialog);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiMessageDialog, g_object_unref)

G_END_DECLS

