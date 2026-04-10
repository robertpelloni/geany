/* BOBGUI - The Bobgui Framework
 * bobguifilechooserutils.h: Private utility functions useful for
 *                        implementing a BobguiFileChooser interface
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

#include "bobguifilechooserprivate.h"
#include "bobguiicontheme.h"

G_BEGIN_DECLS

#define BOBGUI_FILE_CHOOSER_DELEGATE_QUARK	  (_bobgui_file_chooser_delegate_get_quark ())

typedef enum {
  BOBGUI_FILE_CHOOSER_PROP_FIRST                  = 0x1000,
  BOBGUI_FILE_CHOOSER_PROP_ACTION                 = BOBGUI_FILE_CHOOSER_PROP_FIRST,
  BOBGUI_FILE_CHOOSER_PROP_FILTER,
  BOBGUI_FILE_CHOOSER_PROP_SELECT_MULTIPLE,
  BOBGUI_FILE_CHOOSER_PROP_CREATE_FOLDERS,
  BOBGUI_FILE_CHOOSER_PROP_FILTERS,
  BOBGUI_FILE_CHOOSER_PROP_SHORTCUT_FOLDERS,
  BOBGUI_FILE_CHOOSER_PROP_LAST                   = BOBGUI_FILE_CHOOSER_PROP_SHORTCUT_FOLDERS
} BobguiFileChooserProp;

void _bobgui_file_chooser_install_properties (GObjectClass *klass);

void _bobgui_file_chooser_delegate_iface_init (BobguiFileChooserIface *iface);
void _bobgui_file_chooser_set_delegate        (BobguiFileChooser *receiver,
					    BobguiFileChooser *delegate);

GQuark _bobgui_file_chooser_delegate_get_quark (void) G_GNUC_CONST;

GSettings *_bobgui_file_chooser_get_settings_for_widget (BobguiWidget *widget);

char * _bobgui_file_chooser_label_for_file (GFile *file);

gboolean        _bobgui_file_info_consider_as_directory (GFileInfo *info);
gboolean        _bobgui_file_has_native_path (GFile *file);
gboolean        _bobgui_file_consider_as_remote (GFile *file);
GIcon *         _bobgui_file_info_get_icon    (GFileInfo    *info,
                                            int           icon_size,
                                            int           scale,
                                            BobguiIconTheme *icon_theme);

GFile *         _bobgui_file_info_get_file (GFileInfo *info);

G_END_DECLS

