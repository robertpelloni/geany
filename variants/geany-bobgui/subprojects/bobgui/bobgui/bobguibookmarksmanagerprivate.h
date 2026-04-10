/* -*- Mode: C; c-file-style: "gnu"; tab-width: 8 -*- */
/* BOBGUI - The Bobgui Framework
 * bobguibookmarksmanager.h: Utilities to manage and monitor ~/.bobgui-bookmarks
 * Copyright (C) 2003, Red Hat, Inc.
 * Copyright (C) 2007-2008 Carlos Garnacho
 * Copyright (C) 2011 Suse
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Federico Mena Quintero <federico@gnome.org>
 */

#pragma once

#include <gio/gio.h>

typedef void (* BobguiBookmarksChangedFunc) (gpointer data);

typedef struct
{
  /* This list contains BobguiBookmark structs */
  GSList *bookmarks;

  GFileMonitor *bookmarks_monitor;
  gulong bookmarks_monitor_changed_id;

  gpointer changed_func_data;
  BobguiBookmarksChangedFunc changed_func;

  GCancellable *cancellable;
} BobguiBookmarksManager;

typedef struct
{
  GFile *file;
  char *label;
} BobguiBookmark;

BobguiBookmarksManager *_bobgui_bookmarks_manager_new (BobguiBookmarksChangedFunc changed_func,
						 gpointer                changed_func_data);


void _bobgui_bookmarks_manager_free (BobguiBookmarksManager *manager);

GSList *_bobgui_bookmarks_manager_list_bookmarks (BobguiBookmarksManager *manager);

gboolean _bobgui_bookmarks_manager_insert_bookmark (BobguiBookmarksManager *manager,
						 GFile               *file,
						 int                  position,
						 GError             **error);

gboolean _bobgui_bookmarks_manager_remove_bookmark (BobguiBookmarksManager *manager,
						 GFile               *file,
						 GError             **error);

gboolean _bobgui_bookmarks_manager_reorder_bookmark (BobguiBookmarksManager *manager,
						  GFile               *file,
						  int                  new_position,
						  GError             **error);

gboolean _bobgui_bookmarks_manager_has_bookmark (BobguiBookmarksManager *manager,
                                              GFile               *file);

char * _bobgui_bookmarks_manager_get_bookmark_label (BobguiBookmarksManager *manager,
						   GFile               *file);

gboolean _bobgui_bookmarks_manager_set_bookmark_label (BobguiBookmarksManager *manager,
						    GFile               *file,
						    const char          *label,
						    GError             **error);

gboolean _bobgui_bookmarks_manager_get_is_builtin (BobguiBookmarksManager *manager,
                                                GFile               *file);

gboolean _bobgui_bookmarks_manager_get_is_xdg_dir_builtin (GUserDirectory xdg_type);

