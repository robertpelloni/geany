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

#include "bobguiactionobserverprivate.h"

G_BEGIN_DECLS

#define BOBGUI_TYPE_ACTION_OBSERVABLE                          (bobgui_action_observable_get_type ())
#define BOBGUI_ACTION_OBSERVABLE(inst)                         (G_TYPE_CHECK_INSTANCE_CAST ((inst),                     \
                                                             BOBGUI_TYPE_ACTION_OBSERVABLE, BobguiActionObservable))
#define BOBGUI_IS_ACTION_OBSERVABLE(inst)                      (G_TYPE_CHECK_INSTANCE_TYPE ((inst),                     \
                                                             BOBGUI_TYPE_ACTION_OBSERVABLE))
#define BOBGUI_ACTION_OBSERVABLE_GET_IFACE(inst)               (G_TYPE_INSTANCE_GET_INTERFACE ((inst),                  \
                                                             BOBGUI_TYPE_ACTION_OBSERVABLE,                             \
                                                             BobguiActionObservableInterface))

typedef struct _BobguiActionObservableInterface                BobguiActionObservableInterface;

struct _BobguiActionObservableInterface
{
  GTypeInterface g_iface;

  void (* register_observer)   (BobguiActionObservable *observable,
                                const char          *action_name,
                                BobguiActionObserver   *observer);
  void (* unregister_observer) (BobguiActionObservable *observable,
                                const char          *action_name,
                                BobguiActionObserver   *observer);
};

GType                   bobgui_action_observable_get_type                  (void);
void                    bobgui_action_observable_register_observer         (BobguiActionObservable *observable,
                                                                         const char          *action_name,
                                                                         BobguiActionObserver   *observer);
void                    bobgui_action_observable_unregister_observer       (BobguiActionObservable *observable,
                                                                         const char          *action_name,
                                                                         BobguiActionObserver   *observer);

G_END_DECLS

