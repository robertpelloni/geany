/* BOBGUI - The Bobgui Framework
 * bobguifilechooserprivate.h: Interface definition for file selector GUIs
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
#include "bobguifilesystemmodelprivate.h"
#include "deprecated/bobguiliststore.h"
#include "bobguirecentmanager.h"
#include "bobguisearchengineprivate.h"
#include "bobguiquery.h"
#include "bobguisizegroup.h"
#include "deprecated/bobguitreemodelsort.h"
#include "deprecated/bobguitreestore.h"
#include "deprecated/bobguitreeview.h"
#include "bobguibox.h"

G_BEGIN_DECLS

#define SETTINGS_KEY_LOCATION_MODE          "location-mode"
#define SETTINGS_KEY_SHOW_HIDDEN            "show-hidden"
#define SETTINGS_KEY_SHOW_SIZE_COLUMN       "show-size-column"
#define SETTINGS_KEY_SHOW_TYPE_COLUMN       "show-type-column"
#define SETTINGS_KEY_SORT_COLUMN            "sort-column"
#define SETTINGS_KEY_SORT_ORDER             "sort-order"
#define SETTINGS_KEY_WINDOW_SIZE            "window-size"
#define SETTINGS_KEY_SIDEBAR_WIDTH          "sidebar-width"
#define SETTINGS_KEY_STARTUP_MODE           "startup-mode"
#define SETTINGS_KEY_SORT_DIRECTORIES_FIRST "sort-directories-first"
#define SETTINGS_KEY_CLOCK_FORMAT           "clock-format"
#define SETTINGS_KEY_DATE_FORMAT            "date-format"
#define SETTINGS_KEY_TYPE_FORMAT            "type-format"
#define SETTINGS_KEY_VIEW_TYPE              "view-type"

#define BOBGUI_FILE_CHOOSER_GET_IFACE(inst)  (G_TYPE_INSTANCE_GET_INTERFACE ((inst), BOBGUI_TYPE_FILE_CHOOSER, BobguiFileChooserIface))

typedef struct _BobguiFileChooserIface BobguiFileChooserIface;

struct _BobguiFileChooserIface
{
  GTypeInterface base_iface;

  /* Methods
   */
  gboolean       (*set_current_folder)     (BobguiFileChooser    *chooser,
                                            GFile             *file,
                                            GError           **error);
  GFile *        (*get_current_folder)     (BobguiFileChooser    *chooser);
  void           (*set_current_name)       (BobguiFileChooser    *chooser,
                                            const char        *name);
  char *        (*get_current_name)       (BobguiFileChooser    *chooser);
  gboolean       (*select_file)            (BobguiFileChooser    *chooser,
                                            GFile             *file,
                                            GError           **error);
  void           (*unselect_file)          (BobguiFileChooser    *chooser,
                                            GFile             *file);
  void           (*select_all)             (BobguiFileChooser    *chooser);
  void           (*unselect_all)           (BobguiFileChooser    *chooser);
  GListModel *   (*get_files)              (BobguiFileChooser    *chooser);
  void           (*add_filter)             (BobguiFileChooser    *chooser,
                                            BobguiFileFilter     *filter);
  void           (*remove_filter)          (BobguiFileChooser    *chooser,
                                            BobguiFileFilter     *filter);
  GListModel *   (*get_filters)            (BobguiFileChooser    *chooser);
  gboolean       (*add_shortcut_folder)    (BobguiFileChooser    *chooser,
                                            GFile             *file,
                                            GError           **error);
  gboolean       (*remove_shortcut_folder) (BobguiFileChooser    *chooser,
                                            GFile             *file,
                                            GError           **error);
  GListModel *   (*get_shortcut_folders)   (BobguiFileChooser    *chooser);

  /* Signals
   */
  void (*current_folder_changed) (BobguiFileChooser *chooser);
  void (*selection_changed)      (BobguiFileChooser *chooser);
  void (*update_preview)         (BobguiFileChooser *chooser);
  void (*file_activated)         (BobguiFileChooser *chooser);

  /* 3.22 additions */
  void           (*add_choice)    (BobguiFileChooser *chooser,
                                   const char      *id,
                                   const char      *label,
                                   const char     **options,
                                   const char     **option_labels);
  void           (*remove_choice) (BobguiFileChooser  *chooser,
                                   const char      *id);
  void           (*set_choice)    (BobguiFileChooser  *chooser,
                                   const char      *id,
                                   const char      *option);
  const char *   (*get_choice)    (BobguiFileChooser  *chooser,
                                   const char      *id);
};

void     bobgui_file_chooser_select_all         (BobguiFileChooser *chooser);
void     bobgui_file_chooser_unselect_all       (BobguiFileChooser *chooser);
gboolean bobgui_file_chooser_select_file             (BobguiFileChooser  *chooser,
                                                   GFile           *file,
                                                   GError         **error);
void     bobgui_file_chooser_unselect_file           (BobguiFileChooser  *chooser,
                                                   GFile           *file);
G_END_DECLS

