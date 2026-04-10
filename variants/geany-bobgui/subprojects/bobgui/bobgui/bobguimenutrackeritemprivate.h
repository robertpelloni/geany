/*
 * Copyright © 2011, 2013 Canonical Limited
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
 * Author: Ryan Lortie <desrt@desrt.ca>
 */

#pragma once

#include "bobguiactionobservableprivate.h"

#define BOBGUI_TYPE_MENU_TRACKER_ITEM                          (bobgui_menu_tracker_item_get_type ())
#define BOBGUI_MENU_TRACKER_ITEM(inst)                         (G_TYPE_CHECK_INSTANCE_CAST ((inst), \
                                                             BOBGUI_TYPE_MENU_TRACKER_ITEM, BobguiMenuTrackerItem))
#define BOBGUI_IS_MENU_TRACKER_ITEM(inst)                      (G_TYPE_CHECK_INSTANCE_TYPE ((inst), \
                                                             BOBGUI_TYPE_MENU_TRACKER_ITEM))

typedef struct _BobguiMenuTrackerItem BobguiMenuTrackerItem;

#define BOBGUI_TYPE_MENU_TRACKER_ITEM_ROLE                     (bobgui_menu_tracker_item_role_get_type ())

typedef enum  {
  BOBGUI_MENU_TRACKER_ITEM_ROLE_NORMAL,
  BOBGUI_MENU_TRACKER_ITEM_ROLE_CHECK,
  BOBGUI_MENU_TRACKER_ITEM_ROLE_RADIO,
} BobguiMenuTrackerItemRole;

GType                   bobgui_menu_tracker_item_get_type                  (void) G_GNUC_CONST;

GType                   bobgui_menu_tracker_item_role_get_type             (void) G_GNUC_CONST;

BobguiMenuTrackerItem *   _bobgui_menu_tracker_item_new                       (BobguiActionObservable *observable,
                                                                         GMenuModel          *model,
                                                                         int                  item_index,
                                                                         gboolean             mac_os_mode,
                                                                         const char          *action_namespace,
                                                                         gboolean             is_separator);

const char *           bobgui_menu_tracker_item_get_action_name           (BobguiMenuTrackerItem *self);

GVariant *             bobgui_menu_tracker_item_get_action_target         (BobguiMenuTrackerItem *self);

const char *           bobgui_menu_tracker_item_get_special               (BobguiMenuTrackerItem *self);

const char *           bobgui_menu_tracker_item_get_custom                (BobguiMenuTrackerItem *self);

const char *           bobgui_menu_tracker_item_get_display_hint          (BobguiMenuTrackerItem *self);

const char *           bobgui_menu_tracker_item_get_text_direction        (BobguiMenuTrackerItem *self);

BobguiActionObservable *  _bobgui_menu_tracker_item_get_observable            (BobguiMenuTrackerItem *self);

gboolean                bobgui_menu_tracker_item_get_is_separator          (BobguiMenuTrackerItem *self);

gboolean                bobgui_menu_tracker_item_get_has_link              (BobguiMenuTrackerItem *self,
                                                                         const char         *link_name);

const char *           bobgui_menu_tracker_item_get_label                 (BobguiMenuTrackerItem *self);

gboolean               bobgui_menu_tracker_item_get_use_markup            (BobguiMenuTrackerItem *self);

GIcon *                 bobgui_menu_tracker_item_get_icon                  (BobguiMenuTrackerItem *self);

GIcon *                 bobgui_menu_tracker_item_get_verb_icon             (BobguiMenuTrackerItem *self);

gboolean                bobgui_menu_tracker_item_get_sensitive             (BobguiMenuTrackerItem *self);

BobguiMenuTrackerItemRole  bobgui_menu_tracker_item_get_role                  (BobguiMenuTrackerItem *self);

gboolean                bobgui_menu_tracker_item_get_toggled               (BobguiMenuTrackerItem *self);

const char *           bobgui_menu_tracker_item_get_accel                 (BobguiMenuTrackerItem *self);

GMenuModel *           _bobgui_menu_tracker_item_get_link                  (BobguiMenuTrackerItem *self,
                                                                         const char         *link_name);

char *                _bobgui_menu_tracker_item_get_link_namespace        (BobguiMenuTrackerItem *self);

gboolean                bobgui_menu_tracker_item_may_disappear             (BobguiMenuTrackerItem *self);

gboolean                bobgui_menu_tracker_item_get_is_visible            (BobguiMenuTrackerItem *self);

gboolean                bobgui_menu_tracker_item_get_should_request_show   (BobguiMenuTrackerItem *self);

void                    bobgui_menu_tracker_item_activated                 (BobguiMenuTrackerItem *self);

void                    bobgui_menu_tracker_item_request_submenu_shown     (BobguiMenuTrackerItem *self,
                                                                         gboolean            shown);

gboolean                bobgui_menu_tracker_item_get_submenu_shown         (BobguiMenuTrackerItem *self);

