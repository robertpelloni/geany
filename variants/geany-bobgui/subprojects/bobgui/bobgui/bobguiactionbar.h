/*
 * Copyright (c) 2013 - 2014 Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#pragma once

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <bobgui/bobguiwidget.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_ACTION_BAR            (bobgui_action_bar_get_type ())
#define BOBGUI_ACTION_BAR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_ACTION_BAR, BobguiActionBar))
#define BOBGUI_IS_ACTION_BAR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_ACTION_BAR))

typedef struct _BobguiActionBar              BobguiActionBar;

GDK_AVAILABLE_IN_ALL
GType        bobgui_action_bar_get_type          (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
BobguiWidget   *bobgui_action_bar_new               (void);
GDK_AVAILABLE_IN_ALL
BobguiWidget   *bobgui_action_bar_get_center_widget (BobguiActionBar *action_bar);
GDK_AVAILABLE_IN_ALL
void         bobgui_action_bar_set_center_widget (BobguiActionBar *action_bar,
                                               BobguiWidget    *center_widget);

GDK_AVAILABLE_IN_ALL
void         bobgui_action_bar_pack_start        (BobguiActionBar *action_bar,
                                               BobguiWidget    *child);
GDK_AVAILABLE_IN_ALL
void         bobgui_action_bar_pack_end          (BobguiActionBar *action_bar,
                                               BobguiWidget    *child);

GDK_AVAILABLE_IN_ALL
void         bobgui_action_bar_remove            (BobguiActionBar *action_bar,
                                               BobguiWidget    *child);

GDK_AVAILABLE_IN_ALL
void        bobgui_action_bar_set_revealed       (BobguiActionBar *action_bar,
                                               gboolean      revealed);
GDK_AVAILABLE_IN_ALL
gboolean    bobgui_action_bar_get_revealed       (BobguiActionBar *action_bar);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiActionBar, g_object_unref)

G_END_DECLS

