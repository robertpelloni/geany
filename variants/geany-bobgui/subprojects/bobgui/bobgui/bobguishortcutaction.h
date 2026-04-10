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

#pragma once

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <bobgui/bobguitypes.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_SHORTCUT_ACTION (bobgui_shortcut_action_get_type ())

/**
 * BobguiShortcutFunc:
 * @widget: The widget passed to the activation
 * @args: (nullable): The arguments passed to the activation
 * @user_data: (nullable): The user data provided when activating the action
 *
 * Type for shortcuts based on user callbacks.
 *
 * Returns: true if the action was successful
 */
typedef gboolean (* BobguiShortcutFunc) (BobguiWidget *widget,
                                      GVariant  *args,
                                      gpointer   user_data);

/**
 * BobguiShortcutActionFlags:
 * @BOBGUI_SHORTCUT_ACTION_EXCLUSIVE: The action is the only
 *   action that can be activated. If this flag is not set,
 *   a future activation may select a different action.
 *
 * Flags that can be passed to action activation.
 *
 * More flags may be added in the future.
 **/
typedef enum {
  BOBGUI_SHORTCUT_ACTION_EXCLUSIVE = 1 << 0
} BobguiShortcutActionFlags;

GDK_AVAILABLE_IN_ALL
GDK_DECLARE_INTERNAL_TYPE (BobguiShortcutAction, bobgui_shortcut_action, BOBGUI, SHORTCUT_ACTION, GObject)

GDK_AVAILABLE_IN_ALL
char *                  bobgui_shortcut_action_to_string           (BobguiShortcutAction      *self);
GDK_AVAILABLE_IN_ALL
BobguiShortcutAction *     bobgui_shortcut_action_parse_string        (const char *            string);

GDK_AVAILABLE_IN_ALL
void                    bobgui_shortcut_action_print               (BobguiShortcutAction      *self,
                                                                 GString                *string);
GDK_AVAILABLE_IN_ALL
gboolean                bobgui_shortcut_action_activate            (BobguiShortcutAction      *self,
                                                                 BobguiShortcutActionFlags  flags,
                                                                 BobguiWidget              *widget,
                                                                 GVariant               *args);

#define BOBGUI_TYPE_NOTHING_ACTION (bobgui_nothing_action_get_type())

/**
 * BobguiNothingAction:
 *
 * Does nothing.
 */
GDK_AVAILABLE_IN_ALL
GDK_DECLARE_INTERNAL_TYPE (BobguiNothingAction, bobgui_nothing_action, BOBGUI, NOTHING_ACTION, BobguiShortcutAction)

GDK_AVAILABLE_IN_ALL
BobguiShortcutAction *     bobgui_nothing_action_get                  (void);

#define BOBGUI_TYPE_CALLBACK_ACTION (bobgui_callback_action_get_type())

/**
 * BobguiCallbackAction:
 *
 * Invokes a callback.
 */
GDK_AVAILABLE_IN_ALL
GDK_DECLARE_INTERNAL_TYPE (BobguiCallbackAction, bobgui_callback_action, BOBGUI, CALLBACK_ACTION, BobguiShortcutAction)

GDK_AVAILABLE_IN_ALL
BobguiShortcutAction *     bobgui_callback_action_new                 (BobguiShortcutFunc         callback,
                                                                 gpointer                data,
                                                                 GDestroyNotify          destroy);

#define BOBGUI_TYPE_MNEMONIC_ACTION (bobgui_mnemonic_action_get_type())

/**
 * BobguiMnemonicAction:
 *
 * Activates a widget with a mnemonic.
 *
 * This means that [method@Bobgui.Widget.mnemonic_activate] is called.
 */
GDK_AVAILABLE_IN_ALL
GDK_DECLARE_INTERNAL_TYPE (BobguiMnemonicAction, bobgui_mnemonic_action, BOBGUI, MNEMONIC_ACTION, BobguiShortcutAction)

GDK_AVAILABLE_IN_ALL
BobguiShortcutAction *     bobgui_mnemonic_action_get                 (void);

#define BOBGUI_TYPE_ACTIVATE_ACTION (bobgui_activate_action_get_type())

/**
 * BobguiActivateAction:
 *
 * Activates a widget.
 *
 * Widgets are activated by calling [method@Bobgui.Widget.activate].
 */
GDK_AVAILABLE_IN_ALL
GDK_DECLARE_INTERNAL_TYPE (BobguiActivateAction, bobgui_activate_action, BOBGUI, ACTIVATE_ACTION, BobguiShortcutAction)

GDK_AVAILABLE_IN_ALL
BobguiShortcutAction *     bobgui_activate_action_get                 (void);

#define BOBGUI_TYPE_SIGNAL_ACTION (bobgui_signal_action_get_type())

/**
 * BobguiSignalAction:
 *
 * Emits a signal on a widget.
 *
 * Signals that are used in this way are referred to as keybinding signals,
 * and they are expected to be defined with the `G_SIGNAL_ACTION` flag.
 */
GDK_AVAILABLE_IN_ALL
GDK_DECLARE_INTERNAL_TYPE (BobguiSignalAction, bobgui_signal_action, BOBGUI, SIGNAL_ACTION, BobguiShortcutAction)

GDK_AVAILABLE_IN_ALL
BobguiShortcutAction *     bobgui_signal_action_new                   (const char      *signal_name);
GDK_AVAILABLE_IN_ALL
const char *            bobgui_signal_action_get_signal_name       (BobguiSignalAction *self);

#define BOBGUI_TYPE_NAMED_ACTION (bobgui_named_action_get_type())

/**
 * BobguiNamedAction:
 *
 * Activates a named action.
 *
 * See [method@Bobgui.WidgetClass.install_action] and
 * [method@Bobgui.Widget.insert_action_group] for ways
 * to associate named actions with widgets.
 */
GDK_AVAILABLE_IN_ALL
GDK_DECLARE_INTERNAL_TYPE (BobguiNamedAction, bobgui_named_action, BOBGUI, NAMED_ACTION, BobguiShortcutAction)

GDK_AVAILABLE_IN_ALL
BobguiShortcutAction *     bobgui_named_action_new                    (const char     *name);
GDK_AVAILABLE_IN_ALL
const char *            bobgui_named_action_get_action_name        (BobguiNamedAction *self);

G_END_DECLS

