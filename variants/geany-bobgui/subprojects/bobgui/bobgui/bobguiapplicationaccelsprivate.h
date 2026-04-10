/*
 * Copyright © 2013 Canonical Limited
 * Copyright © 2016 Sébastien Wilmet
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
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Ryan Lortie <desrt@desrt.ca>
 *          Sébastien Wilmet <swilmet@gnome.org>
 */

#pragma once

#include <gio/gio.h>
#include "bobguiwindowprivate.h"

G_BEGIN_DECLS

#define BOBGUI_TYPE_APPLICATION_ACCELS (bobgui_application_accels_get_type ())
G_DECLARE_FINAL_TYPE (BobguiApplicationAccels, bobgui_application_accels,
                      BOBGUI, APPLICATION_ACCELS,
                      GObject)

BobguiApplicationAccels *
                bobgui_application_accels_new                          (void);

void            bobgui_application_accels_set_accels_for_action        (BobguiApplicationAccels *accels,
                                                                     const char           *detailed_action_name,
                                                                     const char * const  *accelerators);

char **        bobgui_application_accels_get_accels_for_action        (BobguiApplicationAccels *accels,
                                                                     const char           *detailed_action_name);

char **        bobgui_application_accels_get_actions_for_accel        (BobguiApplicationAccels *accels,
                                                                     const char           *accel);

char **        bobgui_application_accels_list_action_descriptions     (BobguiApplicationAccels *accels);

GListModel *    bobgui_application_accels_get_shortcuts                (BobguiApplicationAccels *accels);

G_END_DECLS

