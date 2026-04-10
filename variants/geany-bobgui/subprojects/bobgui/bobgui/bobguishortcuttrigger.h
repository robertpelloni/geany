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

#define BOBGUI_TYPE_SHORTCUT_TRIGGER (bobgui_shortcut_trigger_get_type ())

GDK_AVAILABLE_IN_ALL
GDK_DECLARE_INTERNAL_TYPE (BobguiShortcutTrigger, bobgui_shortcut_trigger, BOBGUI, SHORTCUT_TRIGGER, GObject)

GDK_AVAILABLE_IN_ALL
BobguiShortcutTrigger *    bobgui_shortcut_trigger_parse_string       (const char         *string);

GDK_AVAILABLE_IN_ALL
char *                  bobgui_shortcut_trigger_to_string          (BobguiShortcutTrigger *self);
GDK_AVAILABLE_IN_ALL
void                    bobgui_shortcut_trigger_print              (BobguiShortcutTrigger *self,
                                                                 GString            *string);
GDK_AVAILABLE_IN_ALL
char *                  bobgui_shortcut_trigger_to_label           (BobguiShortcutTrigger *self,
                                                                 GdkDisplay         *display);
GDK_AVAILABLE_IN_ALL
gboolean                bobgui_shortcut_trigger_print_label        (BobguiShortcutTrigger *self,
                                                                 GdkDisplay         *display,
                                                                 GString            *string);

GDK_AVAILABLE_IN_ALL
guint                   bobgui_shortcut_trigger_hash               (gconstpointer       trigger);
GDK_AVAILABLE_IN_ALL
gboolean                bobgui_shortcut_trigger_equal              (gconstpointer       trigger1,
                                                                 gconstpointer       trigger2);
GDK_AVAILABLE_IN_ALL
int                     bobgui_shortcut_trigger_compare            (gconstpointer       trigger1,
                                                                 gconstpointer       trigger2);

GDK_AVAILABLE_IN_ALL
GdkKeyMatch             bobgui_shortcut_trigger_trigger            (BobguiShortcutTrigger *self,
                                                                 GdkEvent           *event,
                                                                 gboolean            enable_mnemonics);


#define BOBGUI_TYPE_NEVER_TRIGGER (bobgui_never_trigger_get_type())

/**
 * BobguiNeverTrigger:
 *
 * A `BobguiShortcutTrigger` that never triggers.
 */
GDK_AVAILABLE_IN_ALL
GDK_DECLARE_INTERNAL_TYPE (BobguiNeverTrigger, bobgui_never_trigger, BOBGUI, NEVER_TRIGGER, BobguiShortcutTrigger)

GDK_AVAILABLE_IN_ALL
BobguiShortcutTrigger *    bobgui_never_trigger_get                   (void);

#define BOBGUI_TYPE_KEYVAL_TRIGGER (bobgui_keyval_trigger_get_type())

/**
 * BobguiKeyvalTrigger:
 *
 * Triggers when a specific keyval and modifiers are pressed.
 */

GDK_AVAILABLE_IN_ALL
GDK_DECLARE_INTERNAL_TYPE (BobguiKeyvalTrigger, bobgui_keyval_trigger, BOBGUI, KEYVAL_TRIGGER, BobguiShortcutTrigger)

GDK_AVAILABLE_IN_ALL
BobguiShortcutTrigger *    bobgui_keyval_trigger_new                  (guint             keyval,
                                                                 GdkModifierType   modifiers);
GDK_AVAILABLE_IN_ALL
GdkModifierType         bobgui_keyval_trigger_get_modifiers        (BobguiKeyvalTrigger *self);
GDK_AVAILABLE_IN_ALL
guint                   bobgui_keyval_trigger_get_keyval           (BobguiKeyvalTrigger *self);

#define BOBGUI_TYPE_MNEMONIC_TRIGGER (bobgui_mnemonic_trigger_get_type())

/**
 * BobguiMnemonicTrigger:
 *
 * Triggers when a specific mnemonic is pressed.
 *
 * Mnemonics require a *mnemonic modifier* (typically <kbd>Alt</kbd>) to be
 * pressed together with the mnemonic key.
 */
GDK_AVAILABLE_IN_ALL
GDK_DECLARE_INTERNAL_TYPE (BobguiMnemonicTrigger, bobgui_mnemonic_trigger, BOBGUI, MNEMONIC_TRIGGER, BobguiShortcutTrigger)

GDK_AVAILABLE_IN_ALL
BobguiShortcutTrigger *    bobgui_mnemonic_trigger_new                (guint               keyval);
GDK_AVAILABLE_IN_ALL
guint                   bobgui_mnemonic_trigger_get_keyval         (BobguiMnemonicTrigger *self);

#define BOBGUI_TYPE_ALTERNATIVE_TRIGGER (bobgui_alternative_trigger_get_type())

/**
 * BobguiAlternativeTrigger:
 *
 * Combines two shortcut triggers.
 *
 * The `BobguiAlternativeTrigger` triggers when either of the two trigger.
 *
 * This can be cascaded to combine more than two triggers.
 */

GDK_AVAILABLE_IN_ALL
GDK_DECLARE_INTERNAL_TYPE (BobguiAlternativeTrigger, bobgui_alternative_trigger, BOBGUI, ALTERNATIVE_TRIGGER, BobguiShortcutTrigger)

GDK_AVAILABLE_IN_ALL
BobguiShortcutTrigger *    bobgui_alternative_trigger_new             (BobguiShortcutTrigger    *first,
                                                                 BobguiShortcutTrigger    *second);
GDK_AVAILABLE_IN_ALL
BobguiShortcutTrigger *    bobgui_alternative_trigger_get_first       (BobguiAlternativeTrigger *self);
GDK_AVAILABLE_IN_ALL
BobguiShortcutTrigger *    bobgui_alternative_trigger_get_second      (BobguiAlternativeTrigger *self);

G_END_DECLS

