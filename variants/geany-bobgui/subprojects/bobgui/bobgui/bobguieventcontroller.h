/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2012, One Laptop Per Child.
 * Copyright (C) 2014, Red Hat, Inc.
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

typedef struct _BobguiEventControllerClass BobguiEventControllerClass;

#include <gdk/gdk.h>
#include <bobgui/bobguitypes.h>
#include <bobgui/bobguienums.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_EVENT_CONTROLLER         (bobgui_event_controller_get_type ())
#define BOBGUI_EVENT_CONTROLLER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), BOBGUI_TYPE_EVENT_CONTROLLER, BobguiEventController))
#define BOBGUI_EVENT_CONTROLLER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), BOBGUI_TYPE_EVENT_CONTROLLER, BobguiEventControllerClass))
#define BOBGUI_IS_EVENT_CONTROLLER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), BOBGUI_TYPE_EVENT_CONTROLLER))
#define BOBGUI_IS_EVENT_CONTROLLER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), BOBGUI_TYPE_EVENT_CONTROLLER))
#define BOBGUI_EVENT_CONTROLLER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), BOBGUI_TYPE_EVENT_CONTROLLER, BobguiEventControllerClass))



GDK_AVAILABLE_IN_ALL
GType        bobgui_event_controller_get_type       (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
BobguiWidget  * bobgui_event_controller_get_widget     (BobguiEventController *controller);

GDK_AVAILABLE_IN_ALL
void         bobgui_event_controller_reset          (BobguiEventController *controller);

GDK_AVAILABLE_IN_ALL
BobguiPropagationPhase bobgui_event_controller_get_propagation_phase (BobguiEventController *controller);

GDK_AVAILABLE_IN_ALL
void                bobgui_event_controller_set_propagation_phase (BobguiEventController  *controller,
                                                                BobguiPropagationPhase  phase);

GDK_AVAILABLE_IN_ALL
BobguiPropagationLimit bobgui_event_controller_get_propagation_limit (BobguiEventController *controller);

GDK_AVAILABLE_IN_ALL
void                bobgui_event_controller_set_propagation_limit (BobguiEventController  *controller,
                                                                BobguiPropagationLimit  limit);
GDK_AVAILABLE_IN_ALL
const char *        bobgui_event_controller_get_name              (BobguiEventController *controller);
GDK_AVAILABLE_IN_ALL
void                bobgui_event_controller_set_name              (BobguiEventController *controller,
                                                                const char         *name);
GDK_AVAILABLE_IN_4_8
void                bobgui_event_controller_set_static_name       (BobguiEventController *controller,
                                                                const char         *name);

GDK_AVAILABLE_IN_ALL
GdkEvent *          bobgui_event_controller_get_current_event    (BobguiEventController *controller);
GDK_AVAILABLE_IN_ALL
guint32             bobgui_event_controller_get_current_event_time   (BobguiEventController *controller);
GDK_AVAILABLE_IN_ALL
GdkDevice *         bobgui_event_controller_get_current_event_device (BobguiEventController *controller);
GDK_AVAILABLE_IN_ALL
GdkModifierType     bobgui_event_controller_get_current_event_state (BobguiEventController *controller);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiEventController, g_object_unref)

G_END_DECLS

