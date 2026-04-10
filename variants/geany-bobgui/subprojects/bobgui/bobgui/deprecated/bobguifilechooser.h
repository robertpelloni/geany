/* BOBGUI - The Bobgui Framework
 * bobguifilechooser.h: Abstract interface for file selector GUIs
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

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <bobgui/bobguifilefilter.h>
#include <bobgui/bobguiwidget.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_FILE_CHOOSER             (bobgui_file_chooser_get_type ())
#define BOBGUI_FILE_CHOOSER(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_FILE_CHOOSER, BobguiFileChooser))
#define BOBGUI_IS_FILE_CHOOSER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_FILE_CHOOSER))

typedef struct _BobguiFileChooser      BobguiFileChooser;

/**
 * BobguiFileChooserAction:
 * @BOBGUI_FILE_CHOOSER_ACTION_OPEN: Indicates open mode.  The file chooser
 *  will only let the user pick an existing file.
 * @BOBGUI_FILE_CHOOSER_ACTION_SAVE: Indicates save mode.  The file chooser
 *  will let the user pick an existing file, or type in a new
 *  filename.
 * @BOBGUI_FILE_CHOOSER_ACTION_SELECT_FOLDER: Indicates an Open mode for
 *  selecting folders.  The file chooser will let the user pick an
 *  existing folder.
 *
 * Describes whether a `BobguiFileChooser` is being used to open existing files
 * or to save to a possibly new file.
 */
typedef enum
{
  BOBGUI_FILE_CHOOSER_ACTION_OPEN,
  BOBGUI_FILE_CHOOSER_ACTION_SAVE,
  BOBGUI_FILE_CHOOSER_ACTION_SELECT_FOLDER
} BobguiFileChooserAction;

GDK_AVAILABLE_IN_ALL
GType bobgui_file_chooser_get_type (void) G_GNUC_CONST;

/* GError enumeration for BobguiFileChooser */
/**
 * BOBGUI_FILE_CHOOSER_ERROR:
 *
 * Used to get the `GError` quark for `BobguiFileChooser` errors.
 *
 * Deprecated: 4.10: There is no replacement
 */
#define BOBGUI_FILE_CHOOSER_ERROR (bobgui_file_chooser_error_quark ())

/**
 * BobguiFileChooserError:
 * @BOBGUI_FILE_CHOOSER_ERROR_NONEXISTENT: Indicates that a file does not exist.
 * @BOBGUI_FILE_CHOOSER_ERROR_BAD_FILENAME: Indicates a malformed filename.
 * @BOBGUI_FILE_CHOOSER_ERROR_ALREADY_EXISTS: Indicates a duplicate path (e.g. when
 *  adding a bookmark).
 * @BOBGUI_FILE_CHOOSER_ERROR_INCOMPLETE_HOSTNAME: Indicates an incomplete hostname
 *  (e.g. "http://foo" without a slash after that).
 *
 * These identify the various errors that can occur while calling
 * `BobguiFileChooser` functions.
 *
 * Deprecated: 4.20: There is no replacement
 */
typedef enum {
  BOBGUI_FILE_CHOOSER_ERROR_NONEXISTENT,
  BOBGUI_FILE_CHOOSER_ERROR_BAD_FILENAME,
  BOBGUI_FILE_CHOOSER_ERROR_ALREADY_EXISTS,
  BOBGUI_FILE_CHOOSER_ERROR_INCOMPLETE_HOSTNAME
} BobguiFileChooserError;

GDK_DEPRECATED_IN_4_10
GQuark bobgui_file_chooser_error_quark (void);

/* Configuration */

GDK_DEPRECATED_IN_4_10
void                 bobgui_file_chooser_set_action          (BobguiFileChooser       *chooser,
                                                           BobguiFileChooserAction  action);
GDK_DEPRECATED_IN_4_10
BobguiFileChooserAction bobgui_file_chooser_get_action          (BobguiFileChooser       *chooser);
GDK_DEPRECATED_IN_4_10
void                 bobgui_file_chooser_set_select_multiple (BobguiFileChooser       *chooser,
                                                           gboolean              select_multiple);
GDK_DEPRECATED_IN_4_10
gboolean             bobgui_file_chooser_get_select_multiple (BobguiFileChooser       *chooser);
GDK_DEPRECATED_IN_4_10
void                 bobgui_file_chooser_set_create_folders  (BobguiFileChooser       *chooser,
                                                           gboolean              create_folders);
GDK_DEPRECATED_IN_4_10
gboolean             bobgui_file_chooser_get_create_folders  (BobguiFileChooser       *chooser);

/* Suggested name for the Save-type actions */

GDK_DEPRECATED_IN_4_10
void                 bobgui_file_chooser_set_current_name    (BobguiFileChooser       *chooser,
                                                           const char           *name);
GDK_DEPRECATED_IN_4_10
char *               bobgui_file_chooser_get_current_name    (BobguiFileChooser       *chooser);

/* GFile manipulation */

GDK_DEPRECATED_IN_4_10
GFile *              bobgui_file_chooser_get_file            (BobguiFileChooser       *chooser);
GDK_DEPRECATED_IN_4_10
gboolean             bobgui_file_chooser_set_file            (BobguiFileChooser       *chooser,
                                                           GFile                *file,
                                                           GError              **error);
GDK_DEPRECATED_IN_4_10
GListModel *         bobgui_file_chooser_get_files           (BobguiFileChooser       *chooser);
GDK_DEPRECATED_IN_4_10
gboolean             bobgui_file_chooser_set_current_folder  (BobguiFileChooser       *chooser,
                                                           GFile                *file,
                                                           GError              **error);
GDK_DEPRECATED_IN_4_10
GFile *              bobgui_file_chooser_get_current_folder  (BobguiFileChooser       *chooser);

/* List of user selectable filters */

GDK_DEPRECATED_IN_4_10
void                 bobgui_file_chooser_add_filter          (BobguiFileChooser       *chooser,
                                                           BobguiFileFilter        *filter);
GDK_DEPRECATED_IN_4_10
void                 bobgui_file_chooser_remove_filter       (BobguiFileChooser       *chooser,
                                                           BobguiFileFilter        *filter);
GDK_DEPRECATED_IN_4_10
GListModel *         bobgui_file_chooser_get_filters         (BobguiFileChooser       *chooser);

/* Current filter */

GDK_DEPRECATED_IN_4_10
void                 bobgui_file_chooser_set_filter          (BobguiFileChooser       *chooser,
                                                           BobguiFileFilter        *filter);
GDK_DEPRECATED_IN_4_10
BobguiFileFilter *      bobgui_file_chooser_get_filter          (BobguiFileChooser       *chooser);

/* Per-application shortcut folders */

GDK_DEPRECATED_IN_4_10
gboolean             bobgui_file_chooser_add_shortcut_folder (BobguiFileChooser       *chooser,
                                                           GFile                *folder,
                                                           GError              **error);
GDK_DEPRECATED_IN_4_10
gboolean             bobgui_file_chooser_remove_shortcut_folder
                                                          (BobguiFileChooser       *chooser,
                                                           GFile                *folder,
                                                           GError              **error);
GDK_DEPRECATED_IN_4_10
GListModel *         bobgui_file_chooser_get_shortcut_folders (BobguiFileChooser      *chooser);

/* Custom widgets */

GDK_DEPRECATED_IN_4_10
void                 bobgui_file_chooser_add_choice           (BobguiFileChooser      *chooser,
                                                            const char          *id,
                                                            const char          *label,
                                                            const char         **options,
                                                            const char         **option_labels);
GDK_DEPRECATED_IN_4_10
void                 bobgui_file_chooser_remove_choice        (BobguiFileChooser      *chooser,
                                                            const char          *id);
GDK_DEPRECATED_IN_4_10
void                 bobgui_file_chooser_set_choice           (BobguiFileChooser      *chooser,
                                                            const char          *id,
                                                            const char          *option);
GDK_DEPRECATED_IN_4_10
const char *         bobgui_file_chooser_get_choice           (BobguiFileChooser      *chooser,
                                                            const char          *id);

G_END_DECLS

