/* BobguiPageSetupUnixDialog
 * Copyright (C) 2006 Alexander Larsson <alexl@redhat.com>
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

G_BEGIN_DECLS

#define BOBGUI_TYPE_PAGE_SETUP_UNIX_DIALOG                  (bobgui_page_setup_unix_dialog_get_type ())
#define BOBGUI_PAGE_SETUP_UNIX_DIALOG(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_PAGE_SETUP_UNIX_DIALOG, BobguiPageSetupUnixDialog))
#define BOBGUI_IS_PAGE_SETUP_UNIX_DIALOG(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_PAGE_SETUP_UNIX_DIALOG))


typedef struct _BobguiPageSetupUnixDialog BobguiPageSetupUnixDialog;


GDK_AVAILABLE_IN_ALL
GType 		  bobgui_page_setup_unix_dialog_get_type	        (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
BobguiWidget *       bobgui_page_setup_unix_dialog_new                (const char             *title,
								 BobguiWindow              *parent);
GDK_AVAILABLE_IN_ALL
void              bobgui_page_setup_unix_dialog_set_page_setup     (BobguiPageSetupUnixDialog *dialog,
								 BobguiPageSetup           *page_setup);
GDK_AVAILABLE_IN_ALL
BobguiPageSetup *    bobgui_page_setup_unix_dialog_get_page_setup     (BobguiPageSetupUnixDialog *dialog);
GDK_AVAILABLE_IN_ALL
void              bobgui_page_setup_unix_dialog_set_print_settings (BobguiPageSetupUnixDialog *dialog,
								 BobguiPrintSettings       *print_settings);
GDK_AVAILABLE_IN_ALL
BobguiPrintSettings *bobgui_page_setup_unix_dialog_get_print_settings (BobguiPageSetupUnixDialog *dialog);

G_END_DECLS

