/*
 * Copyright (c) 2021 Red Hat, Inc.
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
 */

#pragma once

#include <gdk/gdk.h>
#include <gsk/gsk.h>
#include "bobgui/bobguienums.h"
#include "bobgui/bobguiwidget.h"
#include "bobgui/bobguieventcontroller.h"

#include "inspector/recording.h"

G_BEGIN_DECLS

#define BOBGUI_TYPE_INSPECTOR_EVENT_RECORDING            (bobgui_inspector_event_recording_get_type())
#define BOBGUI_INSPECTOR_EVENT_RECORDING(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), BOBGUI_TYPE_INSPECTOR_EVENT_RECORDING, BobguiInspectorEventRecording))
#define BOBGUI_INSPECTOR_EVENT_RECORDING_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), BOBGUI_TYPE_INSPECTOR_EVENT_RECORDING, BobguiInspectorEventRecordingClass))
#define BOBGUI_INSPECTOR_IS_EVENT_RECORDING(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), BOBGUI_TYPE_INSPECTOR_EVENT_RECORDING))
#define BOBGUI_INSPECTOR_IS_EVENT_RECORDING_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), BOBGUI_TYPE_INSPECTOR_EVENT_RECORDING))
#define BOBGUI_INSPECTOR_EVENT_RECORDING_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), BOBGUI_TYPE_INSPECTOR_EVENT_RECORDING, BobguiInspectorEventRecordingClass))


typedef struct _BobguiInspectorEventRecordingPrivate BobguiInspectorEventRecordingPrivate;

typedef struct
{
  BobguiPropagationPhase phase;
  GType widget_type;
  graphene_rect_t bounds;
  GType controller_type;
  gboolean handled;
} EventTrace;

typedef struct _BobguiInspectorEventRecording
{
  BobguiInspectorRecording parent;

  GdkEvent *event;
  GType target_type;
  graphene_rect_t bounds;
  GArray *traces;
} BobguiInspectorEventRecording;

typedef struct _BobguiInspectorEventRecordingClass
{
  BobguiInspectorRecordingClass parent;
} BobguiInspectorEventRecordingClass;

GType           bobgui_inspector_event_recording_get_type      (void);

BobguiInspectorRecording *
                bobgui_inspector_event_recording_new            (gint64    timestamp,
                                                              GdkEvent *event);

GdkEvent *      bobgui_inspector_event_recording_get_event      (BobguiInspectorEventRecording       *recording);

void            bobgui_inspector_event_recording_add_trace      (BobguiInspectorEventRecording       *recording,
                                                              BobguiPropagationPhase               phase,
                                                              BobguiWidget                        *widget,
                                                              BobguiEventController               *controller,
                                                              BobguiWidget                        *target,
                                                              gboolean                          handled);

EventTrace *   bobgui_inspector_event_recording_get_traces      (BobguiInspectorEventRecording       *recording,
                                                              gsize                            *n_traces);
GType          bobgui_inspector_event_recording_get_target_type (BobguiInspectorEventRecording       *recording);
void           bobgui_inspector_event_recording_get_target_bounds (BobguiInspectorEventRecording     *recording,
                                                                graphene_rect_t                *bounds);

G_END_DECLS


// vim: set et sw=2 ts=2:
