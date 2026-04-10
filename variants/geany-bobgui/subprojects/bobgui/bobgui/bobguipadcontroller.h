/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2016, Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Author(s): Carlos Garnacho <carlosg@gnome.org>
 */

#pragma once

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <gdk/gdk.h>
#include <bobgui/bobguieventcontroller.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_PAD_CONTROLLER         (bobgui_pad_controller_get_type ())
#define BOBGUI_PAD_CONTROLLER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), BOBGUI_TYPE_PAD_CONTROLLER, BobguiPadController))
#define BOBGUI_PAD_CONTROLLER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), BOBGUI_TYPE_PAD_CONTROLLER, BobguiPadControllerClass))
#define BOBGUI_IS_PAD_CONTROLLER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), BOBGUI_TYPE_PAD_CONTROLLER))
#define BOBGUI_IS_PAD_CONTROLLER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), BOBGUI_TYPE_PAD_CONTROLLER))
#define BOBGUI_PAD_CONTROLLER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), BOBGUI_TYPE_PAD_CONTROLLER, BobguiPadControllerClass))

typedef struct _BobguiPadController BobguiPadController;
typedef struct _BobguiPadControllerClass BobguiPadControllerClass;
typedef struct _BobguiPadActionEntry BobguiPadActionEntry;

/**
 * BobguiPadActionType:
 * @BOBGUI_PAD_ACTION_BUTTON: Action is triggered by a pad button
 * @BOBGUI_PAD_ACTION_RING: Action is triggered by a pad ring
 * @BOBGUI_PAD_ACTION_STRIP: Action is triggered by a pad strip
 * @BOBGUI_PAD_ACTION_DIAL: Action is triggered by a pad dial
 *
 * The type of a pad action.
 */
typedef enum {
  BOBGUI_PAD_ACTION_BUTTON,
  BOBGUI_PAD_ACTION_RING,
  BOBGUI_PAD_ACTION_STRIP,
  BOBGUI_PAD_ACTION_DIAL
} BobguiPadActionType;

/**
 * BobguiPadActionEntry:
 * @type: the type of pad feature that will trigger this action entry.
 * @index: the 0-indexed button/ring/strip/dial number that will trigger this action
 *   entry.
 * @mode: the mode that will trigger this action entry, or -1 for all modes.
 * @label: Human readable description of this action entry, this string should
 *   be deemed user-visible.
 * @action_name: action name that will be activated in the `GActionGroup`.
 *
 * Struct defining a pad action entry.
 */
struct _BobguiPadActionEntry {
  BobguiPadActionType type;
  int index;
  int mode;
  const char *label;
  const char *action_name;
};

GDK_AVAILABLE_IN_ALL
GType bobgui_pad_controller_get_type           (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
BobguiPadController *bobgui_pad_controller_new    (GActionGroup     *group,
                                             GdkDevice        *pad);

GDK_AVAILABLE_IN_ALL
void  bobgui_pad_controller_set_action_entries (BobguiPadController        *controller,
                                             const BobguiPadActionEntry *entries,
                                             int                      n_entries);
GDK_AVAILABLE_IN_ALL
void  bobgui_pad_controller_set_action         (BobguiPadController *controller,
                                             BobguiPadActionType  type,
                                             int               index,
                                             int               mode,
                                             const char       *label,
                                             const char       *action_name);

G_END_DECLS

