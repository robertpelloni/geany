/*
 * Copyright © 2011 Canonical Limited
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

#include <gio/gio.h>
#include "bobguiwidget.h"

G_BEGIN_DECLS

#define BOBGUI_TYPE_ACTION_MUXER                               (bobgui_action_muxer_get_type ())
#define BOBGUI_ACTION_MUXER(inst)                              (G_TYPE_CHECK_INSTANCE_CAST ((inst),                     \
                                                             BOBGUI_TYPE_ACTION_MUXER, BobguiActionMuxer))
#define BOBGUI_IS_ACTION_MUXER(inst)                           (G_TYPE_CHECK_INSTANCE_TYPE ((inst),                     \
                                                             BOBGUI_TYPE_ACTION_MUXER))

typedef struct _BobguiWidgetAction BobguiWidgetAction;
typedef struct _BobguiActionMuxer BobguiActionMuxer;

struct _BobguiWidgetAction
{
  BobguiWidgetAction *next;

  char *name;
  GType owner;

  const GVariantType *parameter_type;
  BobguiWidgetActionActivateFunc activate;

  const GVariantType *state_type;
  GParamSpec *pspec;
};

GType                   bobgui_action_muxer_get_type                       (void);
BobguiActionMuxer *        bobgui_action_muxer_new                            (BobguiWidget      *widget);

void                    bobgui_action_muxer_insert                         (BobguiActionMuxer *muxer,
                                                                         const char     *prefix,
                                                                         GActionGroup   *action_group);

void                    bobgui_action_muxer_remove                         (BobguiActionMuxer *muxer,
                                                                         const char     *prefix);
GActionGroup *          bobgui_action_muxer_find                           (BobguiActionMuxer *muxer,
                                                                         const char     *action_name,
                                                                         const char    **unprefixed_name);
GActionGroup *          bobgui_action_muxer_get_group                      (BobguiActionMuxer *muxer,
                                                                         const char     *group_name);
BobguiActionMuxer *        bobgui_action_muxer_get_parent                     (BobguiActionMuxer *muxer);

void                    bobgui_action_muxer_set_parent                     (BobguiActionMuxer *muxer,
                                                                         BobguiActionMuxer *parent);

/* GActionGroup equivalent api */
gboolean                bobgui_action_muxer_query_action                   (BobguiActionMuxer      *muxer,
                                                                         const char          *action_name,
                                                                         gboolean            *enabled,
                                                                         const GVariantType **parameter_type,
                                                                         const GVariantType **state_type,
                                                                         GVariant           **state_hint,
                                                                         GVariant           **state) G_GNUC_WARN_UNUSED_RESULT;
void                    bobgui_action_muxer_activate_action                (BobguiActionMuxer      *muxer,
                                                                         const char          *action_name,
                                                                         GVariant            *parameter);
void                    bobgui_action_muxer_change_action_state            (BobguiActionMuxer      *muxer,
                                                                         const char          *action_name,
                                                                         GVariant            *state);
gboolean                bobgui_action_muxer_has_action                     (BobguiActionMuxer      *muxer,
                                                                         const char          *action_name);
char **                 bobgui_action_muxer_list_actions                   (BobguiActionMuxer      *muxer,
                                                                         gboolean             local_only);

/* api for class actions */
void                    bobgui_action_muxer_action_enabled_changed         (BobguiActionMuxer      *muxer,
                                                                         const char          *action_name,
                                                                         gboolean             enabled);
void                    bobgui_action_muxer_action_state_changed           (BobguiActionMuxer      *muxer,
                                                                         const char          *action_name,
                                                                         GVariant            *state);
void                    bobgui_action_muxer_connect_class_actions          (BobguiActionMuxer      *muxer);

/* api for accels */
void                    bobgui_action_muxer_set_primary_accel              (BobguiActionMuxer      *muxer,
                                                                         const char          *action_and_target,
                                                                         const char          *primary_accel);
const char *            bobgui_action_muxer_get_primary_accel              (BobguiActionMuxer      *muxer,
                                                                         const char          *action_and_target);

/* No better place for these... */
char *                  bobgui_print_action_and_target                     (const char          *action_namespace,
                                                                         const char          *action_name,
                                                                         GVariant            *target);
char *                  bobgui_normalise_detailed_action_name              (const char          *detailed_action_name);

G_END_DECLS

