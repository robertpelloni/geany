/* BOBGUI - The Bobgui Framework
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
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

/*
 * Modified by the BOBGUI Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the BOBGUI Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BOBGUI at ftp://ftp.bobgui.org/pub/bobgui/.
 */

#pragma once

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <bobgui/bobguiwindow.h>

G_BEGIN_DECLS

/**
 * BobguiDialogFlags:
 * @BOBGUI_DIALOG_MODAL: Make the constructed dialog modal
 * @BOBGUI_DIALOG_DESTROY_WITH_PARENT: Destroy the dialog when its parent is destroyed
 * @BOBGUI_DIALOG_USE_HEADER_BAR: Create dialog with actions in header
 *   bar instead of action area
 *
 * Flags used to influence dialog construction.
 *
 * Deprecated: 4.20: There is no replacement.
 */
typedef enum
{
  BOBGUI_DIALOG_MODAL               = 1 << 0,
  BOBGUI_DIALOG_DESTROY_WITH_PARENT = 1 << 1,
  BOBGUI_DIALOG_USE_HEADER_BAR      = 1 << 2
} BobguiDialogFlags;

/**
 * BobguiResponseType:
 * @BOBGUI_RESPONSE_NONE: Returned if an action widget has no response id,
 *   or if the dialog gets programmatically hidden or destroyed
 * @BOBGUI_RESPONSE_REJECT: Generic response id, not used by BOBGUI dialogs
 * @BOBGUI_RESPONSE_ACCEPT: Generic response id, not used by BOBGUI dialogs
 * @BOBGUI_RESPONSE_DELETE_EVENT: Returned if the dialog is deleted
 * @BOBGUI_RESPONSE_OK: Returned by OK buttons in BOBGUI dialogs
 * @BOBGUI_RESPONSE_CANCEL: Returned by Cancel buttons in BOBGUI dialogs
 * @BOBGUI_RESPONSE_CLOSE: Returned by Close buttons in BOBGUI dialogs
 * @BOBGUI_RESPONSE_YES: Returned by Yes buttons in BOBGUI dialogs
 * @BOBGUI_RESPONSE_NO: Returned by No buttons in BOBGUI dialogs
 * @BOBGUI_RESPONSE_APPLY: Returned by Apply buttons in BOBGUI dialogs
 * @BOBGUI_RESPONSE_HELP: Returned by Help buttons in BOBGUI dialogs
 *
 * Predefined values for use as response ids in bobgui_dialog_add_button().
 *
 * All predefined values are negative; BOBGUI leaves values of 0 or greater for
 * application-defined response ids.
 *
 * Deprecated: 4.20: There is no replacement.
 */
typedef enum
{
  BOBGUI_RESPONSE_NONE         = -1,
  BOBGUI_RESPONSE_REJECT       = -2,
  BOBGUI_RESPONSE_ACCEPT       = -3,
  BOBGUI_RESPONSE_DELETE_EVENT = -4,
  BOBGUI_RESPONSE_OK           = -5,
  BOBGUI_RESPONSE_CANCEL       = -6,
  BOBGUI_RESPONSE_CLOSE        = -7,
  BOBGUI_RESPONSE_YES          = -8,
  BOBGUI_RESPONSE_NO           = -9,
  BOBGUI_RESPONSE_APPLY        = -10,
  BOBGUI_RESPONSE_HELP         = -11
} BobguiResponseType;


#define BOBGUI_TYPE_DIALOG                  (bobgui_dialog_get_type ())
#define BOBGUI_DIALOG(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_DIALOG, BobguiDialog))
#define BOBGUI_DIALOG_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), BOBGUI_TYPE_DIALOG, BobguiDialogClass))
#define BOBGUI_IS_DIALOG(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_DIALOG))
#define BOBGUI_IS_DIALOG_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), BOBGUI_TYPE_DIALOG))
#define BOBGUI_DIALOG_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_DIALOG, BobguiDialogClass))


typedef struct _BobguiDialog              BobguiDialog;
typedef struct _BobguiDialogClass         BobguiDialogClass;

struct _BobguiDialog
{
  BobguiWindow parent_instance;
};

/**
 * BobguiDialogClass:
 * @parent_class: The parent class.
 * @response: Signal emitted when an action widget is activated.
 * @close: Signal emitted when the user uses a keybinding to close the dialog.
 */
struct _BobguiDialogClass
{
  BobguiWindowClass parent_class;

  /*< public >*/

  void (* response) (BobguiDialog *dialog, int response_id);

  /* Keybinding signals */

  void (* close)    (BobguiDialog *dialog);

  /*< private >*/

  gpointer padding[8];
};


GDK_AVAILABLE_IN_ALL
GType      bobgui_dialog_get_type (void) G_GNUC_CONST;
GDK_DEPRECATED_IN_4_10
BobguiWidget* bobgui_dialog_new      (void);

GDK_DEPRECATED_IN_4_10
BobguiWidget* bobgui_dialog_new_with_buttons (const char      *title,
                                        BobguiWindow       *parent,
                                        BobguiDialogFlags   flags,
                                        const char      *first_button_text,
                                        ...) G_GNUC_NULL_TERMINATED;

GDK_DEPRECATED_IN_4_10
void       bobgui_dialog_add_action_widget (BobguiDialog   *dialog,
                                         BobguiWidget   *child,
                                         int          response_id);
GDK_DEPRECATED_IN_4_10
BobguiWidget* bobgui_dialog_add_button        (BobguiDialog   *dialog,
                                         const char *button_text,
                                         int          response_id);
GDK_DEPRECATED_IN_4_10
void       bobgui_dialog_add_buttons       (BobguiDialog   *dialog,
                                         const char *first_button_text,
                                         ...) G_GNUC_NULL_TERMINATED;

GDK_DEPRECATED_IN_4_10
void bobgui_dialog_set_response_sensitive (BobguiDialog *dialog,
                                        int        response_id,
                                        gboolean   setting);
GDK_DEPRECATED_IN_4_10
void bobgui_dialog_set_default_response   (BobguiDialog *dialog,
                                        int        response_id);
GDK_DEPRECATED_IN_4_10
BobguiWidget* bobgui_dialog_get_widget_for_response (BobguiDialog *dialog,
                                               int        response_id);
GDK_DEPRECATED_IN_4_10
int bobgui_dialog_get_response_for_widget (BobguiDialog *dialog,
                                         BobguiWidget *widget);

/* Emit response signal */
GDK_DEPRECATED_IN_4_10
void bobgui_dialog_response           (BobguiDialog *dialog,
                                    int        response_id);

GDK_DEPRECATED_IN_4_10
BobguiWidget * bobgui_dialog_get_content_area (BobguiDialog *dialog);
GDK_DEPRECATED_IN_4_10
BobguiWidget * bobgui_dialog_get_header_bar   (BobguiDialog *dialog);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiDialog, g_object_unref)

G_END_DECLS

