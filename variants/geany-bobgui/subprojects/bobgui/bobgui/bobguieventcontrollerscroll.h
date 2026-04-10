/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2017, Red Hat, Inc.
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

#define BOBGUI_TYPE_EVENT_CONTROLLER_SCROLL         (bobgui_event_controller_scroll_get_type ())
#define BOBGUI_EVENT_CONTROLLER_SCROLL(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), BOBGUI_TYPE_EVENT_CONTROLLER_SCROLL, BobguiEventControllerScroll))
#define BOBGUI_EVENT_CONTROLLER_SCROLL_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), BOBGUI_TYPE_EVENT_CONTROLLER_SCROLL, BobguiEventControllerScrollClass))
#define BOBGUI_IS_EVENT_CONTROLLER_SCROLL(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), BOBGUI_TYPE_EVENT_CONTROLLER_SCROLL))
#define BOBGUI_IS_EVENT_CONTROLLER_SCROLL_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), BOBGUI_TYPE_EVENT_CONTROLLER_SCROLL))
#define BOBGUI_EVENT_CONTROLLER_SCROLL_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), BOBGUI_TYPE_EVENT_CONTROLLER_SCROLL, BobguiEventControllerScrollClass))

typedef struct _BobguiEventControllerScroll BobguiEventControllerScroll;
typedef struct _BobguiEventControllerScrollClass BobguiEventControllerScrollClass;

/**
 * BobguiEventControllerScrollFlags:
 * @BOBGUI_EVENT_CONTROLLER_SCROLL_NONE: Don't emit scroll.
 * @BOBGUI_EVENT_CONTROLLER_SCROLL_VERTICAL: Emit scroll with vertical deltas.
 * @BOBGUI_EVENT_CONTROLLER_SCROLL_HORIZONTAL: Emit scroll with horizontal deltas.
 * @BOBGUI_EVENT_CONTROLLER_SCROLL_DISCRETE: Only emit deltas that are multiples of 1.
 * @BOBGUI_EVENT_CONTROLLER_SCROLL_KINETIC: Emit ::decelerate after continuous scroll finishes.
 * @BOBGUI_EVENT_CONTROLLER_SCROLL_BOTH_AXES: Emit scroll on both axes.
 *
 * Describes the behavior of a `BobguiEventControllerScroll`.
 **/

/**
 * BOBGUI_EVENT_CONTROLLER_SCROLL_PHYSICAL_DIRECTION:
 *
 * A #BobguiEventControllerScrollFlags value to prefer physical direction over
 * logical direction (i.e. oblivious to natural scroll).
 *
 * Since: 4.20
 */
typedef enum {
  BOBGUI_EVENT_CONTROLLER_SCROLL_NONE       = 0,
  BOBGUI_EVENT_CONTROLLER_SCROLL_VERTICAL   = 1 << 0,
  BOBGUI_EVENT_CONTROLLER_SCROLL_HORIZONTAL = 1 << 1,
  BOBGUI_EVENT_CONTROLLER_SCROLL_DISCRETE   = 1 << 2,
  BOBGUI_EVENT_CONTROLLER_SCROLL_KINETIC    = 1 << 3,
  BOBGUI_EVENT_CONTROLLER_SCROLL_PHYSICAL_DIRECTION = 1 << 4,
  BOBGUI_EVENT_CONTROLLER_SCROLL_BOTH_AXES  = (BOBGUI_EVENT_CONTROLLER_SCROLL_VERTICAL | BOBGUI_EVENT_CONTROLLER_SCROLL_HORIZONTAL),
} BobguiEventControllerScrollFlags;

GDK_AVAILABLE_IN_ALL
GType               bobgui_event_controller_scroll_get_type  (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
BobguiEventController *bobgui_event_controller_scroll_new       (BobguiEventControllerScrollFlags  flags);
GDK_AVAILABLE_IN_ALL
void                bobgui_event_controller_scroll_set_flags (BobguiEventControllerScroll      *scroll,
                                                           BobguiEventControllerScrollFlags  flags);
GDK_AVAILABLE_IN_ALL
BobguiEventControllerScrollFlags
                    bobgui_event_controller_scroll_get_flags (BobguiEventControllerScroll      *scroll);

GDK_AVAILABLE_IN_4_8
GdkScrollUnit       bobgui_event_controller_scroll_get_unit (BobguiEventControllerScroll       *scroll);

G_END_DECLS

