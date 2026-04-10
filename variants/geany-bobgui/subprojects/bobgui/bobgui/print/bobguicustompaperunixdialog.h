/* BobguiCustomPaperUnixDialog
 * Copyright (C) 2006 Alexander Larsson <alexl@redhat.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <bobgui/bobgui.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_CUSTOM_PAPER_UNIX_DIALOG                  (bobgui_custom_paper_unix_dialog_get_type ())
#define BOBGUI_CUSTOM_PAPER_UNIX_DIALOG(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_CUSTOM_PAPER_UNIX_DIALOG, BobguiCustomPaperUnixDialog))
#define BOBGUI_IS_CUSTOM_PAPER_UNIX_DIALOG(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_CUSTOM_PAPER_UNIX_DIALOG))

typedef struct _BobguiCustomPaperUnixDialog         BobguiCustomPaperUnixDialog;

GDK_AVAILABLE_IN_ALL
GType             bobgui_custom_paper_unix_dialog_get_type           (void) G_GNUC_CONST;
BobguiWidget *       _bobgui_custom_paper_unix_dialog_new                (BobguiWindow   *parent,
                                                                    const char *title);
BobguiUnit           _bobgui_print_get_default_user_units                (void);
void               bobgui_print_load_custom_papers                    (GListStore *store);
GList *           _bobgui_load_custom_papers                          (void);


G_END_DECLS

