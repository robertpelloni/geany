/*
 * Copyright © 2012 Canonical Limited
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * licence or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Ryan Lortie <desrt@desrt.ca>
 */

#pragma once

#include <bobgui/bobguiapplication.h>
#include <bobgui/bobguiactionable.h>
#include <bobgui/bobguimodelbuttonprivate.h>

#define BOBGUI_TYPE_ACTION_HELPER                              (bobgui_action_helper_get_type ())
#define BOBGUI_ACTION_HELPER(inst)                             (G_TYPE_CHECK_INSTANCE_CAST ((inst),                      \
                                                             BOBGUI_TYPE_ACTION_HELPER, BobguiActionHelper))
#define BOBGUI_IS_ACTION_HELPER(inst)                          (G_TYPE_CHECK_INSTANCE_TYPE ((inst),                      \
                                                             BOBGUI_TYPE_ACTION_HELPER))

typedef struct _BobguiActionHelper                             BobguiActionHelper;

GType                   bobgui_action_helper_get_type                      (void);

BobguiActionHelper *       bobgui_action_helper_new                           (BobguiActionable   *widget);

void                    bobgui_action_helper_set_action_name               (BobguiActionHelper *helper,
                                                                         const char      *action_name);
void                    bobgui_action_helper_set_action_target_value       (BobguiActionHelper *helper,
                                                                         GVariant        *action_target);
const char *           bobgui_action_helper_get_action_name               (BobguiActionHelper *helper);
GVariant *              bobgui_action_helper_get_action_target_value       (BobguiActionHelper *helper);

gboolean                bobgui_action_helper_get_enabled                   (BobguiActionHelper *helper);
gboolean                bobgui_action_helper_get_active                    (BobguiActionHelper *helper);

void                    bobgui_action_helper_activate                      (BobguiActionHelper *helper);

BobguiButtonRole           bobgui_action_helper_get_role                      (BobguiActionHelper *helper);


