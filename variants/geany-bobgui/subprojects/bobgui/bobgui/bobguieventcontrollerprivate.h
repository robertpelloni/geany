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

#include "bobguieventcontroller.h"

/* GdkCrossingType:
 * @BOBGUI_CROSSING_FOCUS: Focus moved from one widget to another
 * @BOBGUI_CROSSING_ACTIVE: The active window changed (the crossing
 *    events in this case leave from the old active window's focus
 *    location to the new active window's one.
 * @BOBGUI_CROSSING_POINTER: The pointer moved from one widget to another
 * @BOBGUI_CROSSING_DROP: An active drag moved from one widget to another
 *
 * We emit various kinds of crossing events when the target widget
 * for keyboard or pointer events changes.
 */
typedef enum {
  BOBGUI_CROSSING_FOCUS,
  BOBGUI_CROSSING_ACTIVE,
  BOBGUI_CROSSING_POINTER,
  BOBGUI_CROSSING_DROP
} BobguiCrossingType;

/*
 * GdkCrossingirection:
 * @BOBGUI_CROSSING_IN: the event is on the downward slope, towards the new target
 * @BOBGUI_CROSSING_OUT: the event is on the upward slope, away from the old target
 */
typedef enum {
  BOBGUI_CROSSING_IN,
  BOBGUI_CROSSING_OUT
} BobguiCrossingDirection;

typedef struct _BobguiCrossingData BobguiCrossingData;

/**
 * BobguiCrossingData:
 * @type: the type of crossing event
 * @direction: whether this is a focus-in or focus-out event
 * @mode: the crossing mode
 * @old_target: the old target
 * @old_descendent: the direct child of the receiving widget that
 *   is an ancestor of @old_target, or %NULL if @old_target is not
 *   a descendent of the receiving widget
 * @new_target: the new target
 * @new_descendent: the direct child of the receiving widget that
 *   is an ancestor of @new_target, or %NULL if @new_target is not
 *   a descendent of the receiving widget
 * @drop: the `GdkDrop` if this is info for a drop operation
 *
 * The struct that is passed to bobgui_event_controller_handle_crossing().
 *
 * The @old_target and @new_target fields are set to the old or new
 * focus, drop or hover location.
 */
struct _BobguiCrossingData {
  BobguiCrossingType type;
  BobguiCrossingDirection direction;
  GdkCrossingMode mode;
  BobguiWidget *old_target;
  BobguiWidget *old_descendent;
  BobguiWidget *new_target;
  BobguiWidget *new_descendent;
  GdkDrop *drop;
};

struct _BobguiEventController
{
  GObject parent_instance;
};

struct _BobguiEventControllerClass
{
  GObjectClass parent_class;

  void     (* set_widget)   (BobguiEventController *controller,
                             BobguiWidget          *widget);
  void     (* unset_widget) (BobguiEventController *controller);
  gboolean (* handle_event) (BobguiEventController *controller,
                             GdkEvent            *event,
                             double              x,
                             double              y);
  void     (* reset)        (BobguiEventController *controller);

  void     (* handle_crossing) (BobguiEventController    *controller,
                                const BobguiCrossingData *crossing,
                                double                 x,
                                double                 y);

  /*<private>*/

  /* Tells whether the event is filtered out, %TRUE makes
   * the event unseen by the handle_event vfunc.
   */
  gboolean (* filter_event) (BobguiEventController *controller,
                             GdkEvent           *event);

  gpointer padding[10];
};

BobguiWidget * bobgui_event_controller_get_target (BobguiEventController *controller);


gboolean   bobgui_event_controller_handle_event   (BobguiEventController *controller,
                                                GdkEvent           *event,
                                                BobguiWidget          *target,
                                                double              x,
                                                double              y);
void       bobgui_event_controller_handle_crossing (BobguiEventController    *controller,
                                                 const BobguiCrossingData *crossing,
                                                 double                 x,
                                                 double                 y);

