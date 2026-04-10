/*
 * Copyright © 2018 Benjamin Otte
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Benjamin Otte <otte@gnome.org>
 */

#include "config.h"

#include "bobguishortcutmanager.h"
#include "bobguishortcutmanagerprivate.h"
#include "bobguiflattenlistmodel.h"

/**
 * BobguiShortcutManager:
 *
 * An interface that is used to implement shortcut scopes.
 *
 * This is important for [iface@Bobgui.Native] widgets that have their
 * own surface, since the event controllers that are used to implement
 * managed and global scopes are limited to the same native.
 *
 * Examples for widgets implementing `BobguiShortcutManager` are
 * [class@Bobgui.Window] and [class@Bobgui.Popover].
 *
 * Every widget that implements `BobguiShortcutManager` will be used as a
 * `BOBGUI_SHORTCUT_SCOPE_MANAGED`.
 */

G_DEFINE_INTERFACE (BobguiShortcutManager, bobgui_shortcut_manager, G_TYPE_OBJECT)

void
bobgui_shortcut_manager_create_controllers (BobguiWidget *widget)
{
  BobguiFlattenListModel *model;
  BobguiEventController *controller;

  model = bobgui_flatten_list_model_new (G_LIST_MODEL (g_list_store_new (BOBGUI_TYPE_SHORTCUT_CONTROLLER)));
  g_object_set_data_full (G_OBJECT (widget), "bobgui-shortcut-manager-bubble", model, g_object_unref);
  controller = bobgui_shortcut_controller_new_for_model (G_LIST_MODEL (model));
  bobgui_event_controller_set_static_name (controller, "bobgui-shortcut-manager-bubble");
  bobgui_widget_add_controller (widget, controller);

  model = bobgui_flatten_list_model_new (G_LIST_MODEL (g_list_store_new (BOBGUI_TYPE_SHORTCUT_CONTROLLER)));
  g_object_set_data_full (G_OBJECT (widget), "bobgui-shortcut-manager-capture", model, g_object_unref);
  controller = bobgui_shortcut_controller_new_for_model (G_LIST_MODEL (model));
  bobgui_event_controller_set_static_name (controller, "bobgui-shortcut-manager-capture");
  bobgui_event_controller_set_propagation_phase (controller, BOBGUI_PHASE_CAPTURE);
  bobgui_widget_add_controller (widget, controller);
}

static BobguiFlattenListModel *
bobgui_shortcut_manager_get_model (BobguiShortcutManager  *self,
                                BobguiPropagationPhase  phase)
{
  switch (phase)
    {
    case BOBGUI_PHASE_CAPTURE:
      return g_object_get_data (G_OBJECT (self), "bobgui-shortcut-manager-capture");
    case BOBGUI_PHASE_BUBBLE:
      return g_object_get_data (G_OBJECT (self), "bobgui-shortcut-manager-bubble");
    case BOBGUI_PHASE_NONE:
    case BOBGUI_PHASE_TARGET:
      return NULL;
    default:
      g_assert_not_reached ();
      return NULL;
    }
}

static void
bobgui_shortcut_manager_add_controller (BobguiShortcutManager    *self,
                                     BobguiShortcutController *controller)
{
  BobguiFlattenListModel *model;
  BobguiPropagationPhase phase;

  phase = bobgui_event_controller_get_propagation_phase (BOBGUI_EVENT_CONTROLLER (controller));
  model = bobgui_shortcut_manager_get_model (self, phase);
  if (model)
    {
      GListModel *store = bobgui_flatten_list_model_get_model (model); 
      g_list_store_append (G_LIST_STORE (store), controller);
    }
}

static void
bobgui_shortcut_manager_remove_controller_for_phase (BobguiShortcutManager    *self,
                                                  BobguiShortcutController *controller,
                                                  BobguiPropagationPhase    phase)
{
  BobguiFlattenListModel *model;

  model = bobgui_shortcut_manager_get_model (self, phase);
  if (model)
    {
      GListModel *store;
      guint position;

      store = bobgui_flatten_list_model_get_model (model);
      if (g_list_store_find (G_LIST_STORE (store), controller, &position))
        g_list_store_remove (G_LIST_STORE (store), position);
    }
}

static void
propagation_phase_changed (BobguiShortcutController *controller,
                           GParamSpec            *pspec,
                           BobguiShortcutManager    *self)
{
  /* Remove from all models and readd */
  bobgui_shortcut_manager_remove_controller_for_phase (self, controller, BOBGUI_PHASE_CAPTURE);
  bobgui_shortcut_manager_remove_controller_for_phase (self, controller, BOBGUI_PHASE_BUBBLE);

  bobgui_shortcut_manager_add_controller (self, controller);
}

static void
bobgui_shortcut_manager_default_add_controller (BobguiShortcutManager    *self,
                                             BobguiShortcutController *controller)
{
  bobgui_shortcut_manager_add_controller (self, controller);

  g_signal_connect_object (controller, "notify::propagation-phase",
                           G_CALLBACK (propagation_phase_changed), self, G_CONNECT_DEFAULT);

}

static void
bobgui_shortcut_manager_default_remove_controller (BobguiShortcutManager    *self,
                                                BobguiShortcutController *controller)
{
  BobguiPropagationPhase phase;

  phase = bobgui_event_controller_get_propagation_phase (BOBGUI_EVENT_CONTROLLER (controller));
  bobgui_shortcut_manager_remove_controller_for_phase (self, controller, phase);

  g_signal_handlers_disconnect_by_func (controller, propagation_phase_changed, self);
}

static void
bobgui_shortcut_manager_default_init (BobguiShortcutManagerInterface *iface)
{
  iface->add_controller = bobgui_shortcut_manager_default_add_controller;
  iface->remove_controller = bobgui_shortcut_manager_default_remove_controller;
}

