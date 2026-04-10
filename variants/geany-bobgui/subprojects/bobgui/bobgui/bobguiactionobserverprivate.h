/*
 * Copyright © 2011 Canonical Limited
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

#include <gio/gio.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_ACTION_OBSERVER                            (bobgui_action_observer_get_type ())
#define BOBGUI_ACTION_OBSERVER(inst)                           (G_TYPE_CHECK_INSTANCE_CAST ((inst),                     \
                                                             BOBGUI_TYPE_ACTION_OBSERVER, BobguiActionObserver))
#define BOBGUI_IS_ACTION_OBSERVER(inst)                        (G_TYPE_CHECK_INSTANCE_TYPE ((inst),                     \
                                                             BOBGUI_TYPE_ACTION_OBSERVER))
#define BOBGUI_ACTION_OBSERVER_GET_IFACE(inst)                 (G_TYPE_INSTANCE_GET_INTERFACE ((inst),                  \
                                                             BOBGUI_TYPE_ACTION_OBSERVER, BobguiActionObserverInterface))

typedef struct _BobguiActionObserverInterface                  BobguiActionObserverInterface;
typedef struct _BobguiActionObservable                         BobguiActionObservable;
typedef struct _BobguiActionObserver                           BobguiActionObserver;

struct _BobguiActionObserverInterface
{
  GTypeInterface g_iface;

  void (* action_added)           (BobguiActionObserver    *observer,
                                   BobguiActionObservable  *observable,
                                   const char           *action_name,
                                   const GVariantType   *parameter_type,
                                   gboolean              enabled,
                                   GVariant             *state);
  void (* action_enabled_changed) (BobguiActionObserver    *observer,
                                   BobguiActionObservable  *observable,
                                   const char           *action_name,
                                   gboolean              enabled);
  void (* action_state_changed)   (BobguiActionObserver    *observer,
                                   BobguiActionObservable  *observable,
                                   const char           *action_name,
                                   GVariant             *state);
  void (* action_removed)         (BobguiActionObserver    *observer,
                                   BobguiActionObservable  *observable,
                                   const char           *action_name);
  void (* primary_accel_changed)  (BobguiActionObserver    *observer,
                                   BobguiActionObservable  *observable,
                                   const char           *action_name,
                                   const char           *action_and_target);
};

GType                   bobgui_action_observer_get_type                    (void);
void                    bobgui_action_observer_action_added                (BobguiActionObserver   *observer,
                                                                         BobguiActionObservable *observable,
                                                                         const char          *action_name,
                                                                         const GVariantType  *parameter_type,
                                                                         gboolean             enabled,
                                                                         GVariant            *state);
void                    bobgui_action_observer_action_enabled_changed      (BobguiActionObserver   *observer,
                                                                         BobguiActionObservable *observable,
                                                                         const char          *action_name,
                                                                         gboolean             enabled);
void                    bobgui_action_observer_action_state_changed        (BobguiActionObserver   *observer,
                                                                         BobguiActionObservable *observable,
                                                                         const char          *action_name,
                                                                         GVariant            *state);
void                    bobgui_action_observer_action_removed              (BobguiActionObserver   *observer,
                                                                         BobguiActionObservable *observable,
                                                                         const char          *action_name);
void                    bobgui_action_observer_primary_accel_changed       (BobguiActionObserver   *observer,
                                                                         BobguiActionObservable *observable,
                                                                         const char          *action_name,
                                                                         const char          *action_and_target);

G_END_DECLS

