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

#include <bobgui/bobguiwidget.h>

G_BEGIN_DECLS


#define BOBGUI_TYPE_REVEALER (bobgui_revealer_get_type ())
#define BOBGUI_REVEALER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_REVEALER, BobguiRevealer))
#define BOBGUI_IS_REVEALER(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_REVEALER))

typedef struct _BobguiRevealer BobguiRevealer;

/**
 * BOBGUI_REVEALER_TRANSITION_TYPE_FADE_SLIDE_RIGHT:
 *
 * Combination of [enum@Bobgui.RevealerTransitionType.CROSSFADE]
 * and [enum@Bobgui.RevealerTransitionType.SLIDE_RIGHT].
 *
 * Since: 4.22
 */
/**
 * BOBGUI_REVEALER_TRANSITION_TYPE_FADE_SLIDE_LEFT:
 *
 * Combination of [enum@Bobgui.RevealerTransitionType.CROSSFADE]
 * and [enum@Bobgui.RevealerTransitionType.SLIDE_LEFT].
 *
 * Since: 4.22
 */
/**
 * BOBGUI_REVEALER_TRANSITION_TYPE_FADE_SLIDE_UP:
 *
 * Combination of [enum@Bobgui.RevealerTransitionType.CROSSFADE]
 * and [enum@Bobgui.RevealerTransitionType.SLIDE_UP].
 *
 * Since: 4.22
 */
/**
 * BOBGUI_REVEALER_TRANSITION_TYPE_FADE_SLIDE_DOWN:
 *
 * Combination of [enum@Bobgui.RevealerTransitionType.CROSSFADE]
 * and [enum@Bobgui.RevealerTransitionType.SLIDE_DOWN].
 *
 * Since: 4.22
 */
typedef enum {
  BOBGUI_REVEALER_TRANSITION_TYPE_NONE,
  BOBGUI_REVEALER_TRANSITION_TYPE_CROSSFADE,
  BOBGUI_REVEALER_TRANSITION_TYPE_SLIDE_RIGHT,
  BOBGUI_REVEALER_TRANSITION_TYPE_SLIDE_LEFT,
  BOBGUI_REVEALER_TRANSITION_TYPE_SLIDE_UP,
  BOBGUI_REVEALER_TRANSITION_TYPE_SLIDE_DOWN,
  BOBGUI_REVEALER_TRANSITION_TYPE_SWING_RIGHT,
  BOBGUI_REVEALER_TRANSITION_TYPE_SWING_LEFT,
  BOBGUI_REVEALER_TRANSITION_TYPE_SWING_UP,
  BOBGUI_REVEALER_TRANSITION_TYPE_SWING_DOWN,
  BOBGUI_REVEALER_TRANSITION_TYPE_FADE_SLIDE_RIGHT,
  BOBGUI_REVEALER_TRANSITION_TYPE_FADE_SLIDE_LEFT,
  BOBGUI_REVEALER_TRANSITION_TYPE_FADE_SLIDE_UP,
  BOBGUI_REVEALER_TRANSITION_TYPE_FADE_SLIDE_DOWN
} BobguiRevealerTransitionType;

GDK_AVAILABLE_IN_ALL
GType                      bobgui_revealer_get_type                (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
BobguiWidget*                 bobgui_revealer_new                     (void);
GDK_AVAILABLE_IN_ALL
gboolean                   bobgui_revealer_get_reveal_child        (BobguiRevealer               *revealer);
GDK_AVAILABLE_IN_ALL
void                       bobgui_revealer_set_reveal_child        (BobguiRevealer               *revealer,
                                                                 gboolean                   reveal_child);
GDK_AVAILABLE_IN_ALL
gboolean                   bobgui_revealer_get_child_revealed      (BobguiRevealer               *revealer);
GDK_AVAILABLE_IN_ALL
guint                      bobgui_revealer_get_transition_duration (BobguiRevealer               *revealer);
GDK_AVAILABLE_IN_ALL
void                       bobgui_revealer_set_transition_duration (BobguiRevealer               *revealer,
                                                                 guint                      duration);
GDK_AVAILABLE_IN_ALL
void                       bobgui_revealer_set_transition_type     (BobguiRevealer               *revealer,
                                                                 BobguiRevealerTransitionType  transition);
GDK_AVAILABLE_IN_ALL
BobguiRevealerTransitionType  bobgui_revealer_get_transition_type     (BobguiRevealer               *revealer);

GDK_AVAILABLE_IN_ALL
void                       bobgui_revealer_set_child               (BobguiRevealer               *revealer,
                                                                 BobguiWidget                 *child);
GDK_AVAILABLE_IN_ALL
BobguiWidget *                bobgui_revealer_get_child               (BobguiRevealer               *revealer);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiRevealer, g_object_unref)

G_END_DECLS

