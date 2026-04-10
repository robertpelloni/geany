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
 * Author: Alexander Larsson <alexl@redhat.com>
 *
 */

#pragma once

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <bobgui/bobguiwidget.h>
#include <bobgui/bobguiselectionmodel.h>

G_BEGIN_DECLS


#define BOBGUI_TYPE_STACK (bobgui_stack_get_type ())
#define BOBGUI_STACK(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_STACK, BobguiStack))
#define BOBGUI_IS_STACK(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_STACK))

typedef struct _BobguiStack BobguiStack;

#define BOBGUI_TYPE_STACK_PAGE (bobgui_stack_page_get_type ())
#define BOBGUI_STACK_PAGE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_STACK_PAGE, BobguiStackPage))
#define BOBGUI_IS_STACK_PAGE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_STACK_PAGE))

typedef struct _BobguiStackPage BobguiStackPage;

typedef enum {
  BOBGUI_STACK_TRANSITION_TYPE_NONE,
  BOBGUI_STACK_TRANSITION_TYPE_CROSSFADE,
  BOBGUI_STACK_TRANSITION_TYPE_SLIDE_RIGHT,
  BOBGUI_STACK_TRANSITION_TYPE_SLIDE_LEFT,
  BOBGUI_STACK_TRANSITION_TYPE_SLIDE_UP,
  BOBGUI_STACK_TRANSITION_TYPE_SLIDE_DOWN,
  BOBGUI_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT,
  BOBGUI_STACK_TRANSITION_TYPE_SLIDE_UP_DOWN,
  BOBGUI_STACK_TRANSITION_TYPE_OVER_UP,
  BOBGUI_STACK_TRANSITION_TYPE_OVER_DOWN,
  BOBGUI_STACK_TRANSITION_TYPE_OVER_LEFT,
  BOBGUI_STACK_TRANSITION_TYPE_OVER_RIGHT,
  BOBGUI_STACK_TRANSITION_TYPE_UNDER_UP,
  BOBGUI_STACK_TRANSITION_TYPE_UNDER_DOWN,
  BOBGUI_STACK_TRANSITION_TYPE_UNDER_LEFT,
  BOBGUI_STACK_TRANSITION_TYPE_UNDER_RIGHT,
  BOBGUI_STACK_TRANSITION_TYPE_OVER_UP_DOWN,
  BOBGUI_STACK_TRANSITION_TYPE_OVER_DOWN_UP,
  BOBGUI_STACK_TRANSITION_TYPE_OVER_LEFT_RIGHT,
  BOBGUI_STACK_TRANSITION_TYPE_OVER_RIGHT_LEFT,
  BOBGUI_STACK_TRANSITION_TYPE_ROTATE_LEFT,
  BOBGUI_STACK_TRANSITION_TYPE_ROTATE_RIGHT,
  BOBGUI_STACK_TRANSITION_TYPE_ROTATE_LEFT_RIGHT
} BobguiStackTransitionType;

GDK_AVAILABLE_IN_ALL
GType                  bobgui_stack_page_get_type            (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
BobguiWidget *            bobgui_stack_page_get_child           (BobguiStackPage           *self);
GDK_AVAILABLE_IN_ALL
gboolean               bobgui_stack_page_get_visible         (BobguiStackPage           *self);
GDK_AVAILABLE_IN_ALL
void                   bobgui_stack_page_set_visible         (BobguiStackPage           *self,
                                                           gboolean                visible);
GDK_AVAILABLE_IN_ALL
gboolean               bobgui_stack_page_get_needs_attention (BobguiStackPage           *self);
GDK_AVAILABLE_IN_ALL
void                   bobgui_stack_page_set_needs_attention (BobguiStackPage           *self,
                                                           gboolean                setting);
GDK_AVAILABLE_IN_ALL
gboolean               bobgui_stack_page_get_use_underline   (BobguiStackPage           *self);
GDK_AVAILABLE_IN_ALL
void                   bobgui_stack_page_set_use_underline   (BobguiStackPage           *self,
                                                           gboolean                setting);
GDK_AVAILABLE_IN_ALL
const char *           bobgui_stack_page_get_name            (BobguiStackPage           *self);
GDK_AVAILABLE_IN_ALL
void                   bobgui_stack_page_set_name            (BobguiStackPage           *self,
                                                            const char            *setting);
GDK_AVAILABLE_IN_ALL
const char *           bobgui_stack_page_get_title           (BobguiStackPage           *self);
GDK_AVAILABLE_IN_ALL
void                   bobgui_stack_page_set_title           (BobguiStackPage           *self,
                                                           const char             *setting);
GDK_AVAILABLE_IN_ALL
const char *           bobgui_stack_page_get_icon_name       (BobguiStackPage           *self);
GDK_AVAILABLE_IN_ALL
void                   bobgui_stack_page_set_icon_name       (BobguiStackPage           *self,
                                                           const char             *setting);



GDK_AVAILABLE_IN_ALL
GType                  bobgui_stack_get_type                (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
BobguiWidget *            bobgui_stack_new                     (void);
GDK_AVAILABLE_IN_ALL
BobguiStackPage *         bobgui_stack_add_child               (BobguiStack               *stack,
                                                          BobguiWidget              *child);
GDK_AVAILABLE_IN_ALL
BobguiStackPage *         bobgui_stack_add_named               (BobguiStack               *stack,
                                                          BobguiWidget              *child,
                                                          const char             *name);
GDK_AVAILABLE_IN_ALL
BobguiStackPage *         bobgui_stack_add_titled              (BobguiStack               *stack,
                                                          BobguiWidget              *child,
                                                          const char             *name,
                                                          const char             *title);
GDK_AVAILABLE_IN_ALL
void                   bobgui_stack_remove                  (BobguiStack               *stack,
                                                          BobguiWidget              *child);

GDK_AVAILABLE_IN_ALL
BobguiStackPage *         bobgui_stack_get_page                (BobguiStack               *stack,
                                                          BobguiWidget              *child);
GDK_AVAILABLE_IN_ALL
BobguiWidget *            bobgui_stack_get_child_by_name       (BobguiStack               *stack,
                                                          const char             *name);
GDK_AVAILABLE_IN_ALL
void                   bobgui_stack_set_visible_child       (BobguiStack               *stack,
                                                          BobguiWidget              *child);
GDK_AVAILABLE_IN_ALL
BobguiWidget *            bobgui_stack_get_visible_child       (BobguiStack               *stack);
GDK_AVAILABLE_IN_ALL
void                   bobgui_stack_set_visible_child_name  (BobguiStack               *stack,
                                                          const char             *name);
GDK_AVAILABLE_IN_ALL
const char *          bobgui_stack_get_visible_child_name  (BobguiStack               *stack);
GDK_AVAILABLE_IN_ALL
void                   bobgui_stack_set_visible_child_full  (BobguiStack               *stack,
                                                          const char             *name,
                                                          BobguiStackTransitionType  transition);
GDK_AVAILABLE_IN_ALL
void                   bobgui_stack_set_hhomogeneous        (BobguiStack               *stack,
                                                          gboolean                hhomogeneous);
GDK_AVAILABLE_IN_ALL
gboolean               bobgui_stack_get_hhomogeneous        (BobguiStack               *stack);
GDK_AVAILABLE_IN_ALL
void                   bobgui_stack_set_vhomogeneous        (BobguiStack               *stack,
                                                          gboolean                vhomogeneous);
GDK_AVAILABLE_IN_ALL
gboolean               bobgui_stack_get_vhomogeneous        (BobguiStack               *stack);
GDK_AVAILABLE_IN_ALL
void                   bobgui_stack_set_transition_duration (BobguiStack               *stack,
                                                          guint                   duration);
GDK_AVAILABLE_IN_ALL
guint                  bobgui_stack_get_transition_duration (BobguiStack               *stack);
GDK_AVAILABLE_IN_ALL
void                   bobgui_stack_set_transition_type     (BobguiStack               *stack,
                                                          BobguiStackTransitionType  transition);
GDK_AVAILABLE_IN_ALL
BobguiStackTransitionType bobgui_stack_get_transition_type     (BobguiStack               *stack);
GDK_AVAILABLE_IN_ALL
gboolean               bobgui_stack_get_transition_running  (BobguiStack               *stack);
GDK_AVAILABLE_IN_ALL
void                   bobgui_stack_set_interpolate_size    (BobguiStack *stack,
                                                          gboolean  interpolate_size);
GDK_AVAILABLE_IN_ALL
gboolean               bobgui_stack_get_interpolate_size    (BobguiStack *stack);

GDK_AVAILABLE_IN_ALL
BobguiSelectionModel *    bobgui_stack_get_pages               (BobguiStack *stack);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiStack, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiStackPage, g_object_unref)

G_END_DECLS

