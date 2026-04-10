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

#include "config.h"

#include "bobguiactionobservableprivate.h"

G_DEFINE_INTERFACE (BobguiActionObservable, bobgui_action_observable, G_TYPE_OBJECT)

void
bobgui_action_observable_default_init (BobguiActionObservableInterface *iface)
{
}

/**
 * bobgui_action_observable_register_observer:
 * @observable: a `BobguiActionObservable`
 * @action_name: the name of the action
 * @observer: the `BobguiActionObserver` to which the events will be reported
 *
 * Registers @observer as being interested in changes to @action_name on
 * @observable.
 */
void
bobgui_action_observable_register_observer (BobguiActionObservable *observable,
                                         const char          *action_name,
                                         BobguiActionObserver   *observer)
{
  g_return_if_fail (BOBGUI_IS_ACTION_OBSERVABLE (observable));

  BOBGUI_ACTION_OBSERVABLE_GET_IFACE (observable)
    ->register_observer (observable, action_name, observer);
}

/**
 * bobgui_action_observable_unregister_observer:
 * @observable: a `BobguiActionObservable`
 * @action_name: the name of the action
 * @observer: the `BobguiActionObserver` to which the events will be reported
 *
 * Removes the registration of @observer as being interested in changes
 * to @action_name on @observable.
 *
 * If the observer was registered multiple times, it must be
 * unregistered an equal number of times.
 */
void
bobgui_action_observable_unregister_observer (BobguiActionObservable *observable,
                                           const char          *action_name,
                                           BobguiActionObserver   *observer)
{
  g_return_if_fail (BOBGUI_IS_ACTION_OBSERVABLE (observable));

  BOBGUI_ACTION_OBSERVABLE_GET_IFACE (observable)
    ->unregister_observer (observable, action_name, observer);
}
