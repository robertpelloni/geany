/*
 * Copyright © 2013 Canonical Limited
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the licence, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Ryan Lortie <desrt@desrt.ca>
 */

#pragma once

#include "bobguimenutrackeritemprivate.h"

typedef struct _BobguiMenuTracker BobguiMenuTracker;

typedef void         (* BobguiMenuTrackerInsertFunc)                       (BobguiMenuTrackerItem       *item,
                                                                         int                       position,
                                                                         gpointer                  user_data);

typedef void         (* BobguiMenuTrackerRemoveFunc)                       (int                       position,
                                                                         gpointer                  user_data);


BobguiMenuTracker *        bobgui_menu_tracker_new                            (BobguiActionObservable      *observer,
                                                                         GMenuModel               *model,
                                                                         gboolean                  with_separators,
                                                                         gboolean                  merge_sections,
                                                                         gboolean                  mac_os_mode,
                                                                         const char               *action_namespace,
                                                                         BobguiMenuTrackerInsertFunc  insert_func,
                                                                         BobguiMenuTrackerRemoveFunc  remove_func,
                                                                         gpointer                  user_data);

BobguiMenuTracker *        bobgui_menu_tracker_new_for_item_link              (BobguiMenuTrackerItem       *item,
                                                                         const char               *link_name,
                                                                         gboolean                  merge_sections,
                                                                         gboolean                  mac_os_mode,
                                                                         BobguiMenuTrackerInsertFunc  insert_func,
                                                                         BobguiMenuTrackerRemoveFunc  remove_func,
                                                                         gpointer                  user_data);

void                    bobgui_menu_tracker_free                           (BobguiMenuTracker           *tracker);

