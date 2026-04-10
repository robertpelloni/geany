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

#include <bobgui/bobguieventcontroller.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_SHORTCUT_CONTROLLER         (bobgui_shortcut_controller_get_type ())
#define BOBGUI_SHORTCUT_CONTROLLER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), BOBGUI_TYPE_SHORTCUT_CONTROLLER, BobguiShortcutController))
#define BOBGUI_SHORTCUT_CONTROLLER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), BOBGUI_TYPE_SHORTCUT_CONTROLLER, BobguiShortcutControllerClass))
#define BOBGUI_IS_SHORTCUT_CONTROLLER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), BOBGUI_TYPE_SHORTCUT_CONTROLLER))
#define BOBGUI_IS_SHORTCUT_CONTROLLER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), BOBGUI_TYPE_SHORTCUT_CONTROLLER))
#define BOBGUI_SHORTCUT_CONTROLLER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), BOBGUI_TYPE_SHORTCUT_CONTROLLER, BobguiShortcutControllerClass))

typedef struct _BobguiShortcutController BobguiShortcutController;
typedef struct _BobguiShortcutControllerClass BobguiShortcutControllerClass;

GDK_AVAILABLE_IN_ALL
GType                   bobgui_shortcut_controller_get_type                (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
BobguiEventController *    bobgui_shortcut_controller_new                     (void);
GDK_AVAILABLE_IN_ALL
BobguiEventController *    bobgui_shortcut_controller_new_for_model           (GListModel             *model);

GDK_AVAILABLE_IN_ALL
void                    bobgui_shortcut_controller_set_mnemonics_modifiers (BobguiShortcutController  *self,
                                                                         GdkModifierType         modifiers);
GDK_AVAILABLE_IN_ALL
GdkModifierType         bobgui_shortcut_controller_get_mnemonics_modifiers (BobguiShortcutController  *self);
GDK_AVAILABLE_IN_ALL
void                    bobgui_shortcut_controller_set_scope               (BobguiShortcutController  *self,
                                                                         BobguiShortcutScope        scope);
GDK_AVAILABLE_IN_ALL
BobguiShortcutScope        bobgui_shortcut_controller_get_scope               (BobguiShortcutController  *self);

GDK_AVAILABLE_IN_ALL
void                    bobgui_shortcut_controller_add_shortcut            (BobguiShortcutController  *self,
                                                                         BobguiShortcut            *shortcut);
GDK_AVAILABLE_IN_ALL
void                    bobgui_shortcut_controller_remove_shortcut         (BobguiShortcutController  *self,
                                                                         BobguiShortcut            *shortcut);

G_END_DECLS

