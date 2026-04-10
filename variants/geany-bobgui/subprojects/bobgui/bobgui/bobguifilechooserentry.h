/* BOBGUI - The Bobgui Framework
 * bobguifilechooserentry.h: Entry with filename completion
 * Copyright (C) 2003, Red Hat, Inc.
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

#include "deprecated/bobguifilechooser.h"

G_BEGIN_DECLS

#define BOBGUI_TYPE_FILE_CHOOSER_ENTRY    (_bobgui_file_chooser_entry_get_type ())
#define BOBGUI_FILE_CHOOSER_ENTRY(obj)    (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_FILE_CHOOSER_ENTRY, BobguiFileChooserEntry))
#define BOBGUI_IS_FILE_CHOOSER_ENTRY(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_FILE_CHOOSER_ENTRY))

typedef struct _BobguiFileChooserEntry      BobguiFileChooserEntry;

GType              _bobgui_file_chooser_entry_get_type           (void) G_GNUC_CONST;
BobguiWidget *        _bobgui_file_chooser_entry_new                (gboolean             eat_tab,
                                                               gboolean             eat_escape);
void               _bobgui_file_chooser_entry_set_action         (BobguiFileChooserEntry *chooser_entry,
							       BobguiFileChooserAction action);
BobguiFileChooserAction _bobgui_file_chooser_entry_get_action       (BobguiFileChooserEntry *chooser_entry);
void               _bobgui_file_chooser_entry_set_base_folder    (BobguiFileChooserEntry *chooser_entry,
							       GFile               *folder);
GFile *            _bobgui_file_chooser_entry_get_current_folder (BobguiFileChooserEntry *chooser_entry);
const char *      _bobgui_file_chooser_entry_get_file_part      (BobguiFileChooserEntry *chooser_entry);
gboolean           _bobgui_file_chooser_entry_get_is_folder      (BobguiFileChooserEntry *chooser_entry,
							       GFile               *file);
void               _bobgui_file_chooser_entry_select_filename    (BobguiFileChooserEntry *chooser_entry);
void               _bobgui_file_chooser_entry_set_file_filter    (BobguiFileChooserEntry *chooser_entry,
                                                               BobguiFileFilter       *filter);
void               bobgui_file_chooser_entry_set_text            (BobguiFileChooserEntry *entry,
                                                               const char          *text);

G_END_DECLS

