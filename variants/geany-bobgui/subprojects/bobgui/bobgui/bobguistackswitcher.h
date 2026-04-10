/*
 * Copyright (c) 2013 Red Hat, Inc.
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

#include <bobgui/bobguistack.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_STACK_SWITCHER            (bobgui_stack_switcher_get_type ())
#define BOBGUI_STACK_SWITCHER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_STACK_SWITCHER, BobguiStackSwitcher))
#define BOBGUI_IS_STACK_SWITCHER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_STACK_SWITCHER))

typedef struct _BobguiStackSwitcher              BobguiStackSwitcher;


GDK_AVAILABLE_IN_ALL
GType        bobgui_stack_switcher_get_type          (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
BobguiWidget *  bobgui_stack_switcher_new               (void);
GDK_AVAILABLE_IN_ALL
void         bobgui_stack_switcher_set_stack         (BobguiStackSwitcher *switcher,
                                                   BobguiStack         *stack);
GDK_AVAILABLE_IN_ALL
BobguiStack *   bobgui_stack_switcher_get_stack         (BobguiStackSwitcher *switcher);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiStackSwitcher, g_object_unref)

G_END_DECLS

