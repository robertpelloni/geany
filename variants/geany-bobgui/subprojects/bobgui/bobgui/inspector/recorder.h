/*
 * Copyright (c) 2016 Red Hat, Inc.
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

#include <bobgui/bobguiwidget.h>

#define BOBGUI_TYPE_INSPECTOR_RECORDER            (bobgui_inspector_recorder_get_type())
#define BOBGUI_INSPECTOR_RECORDER(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), BOBGUI_TYPE_INSPECTOR_RECORDER, BobguiInspectorRecorder))
#define BOBGUI_INSPECTOR_IS_RECORDER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), BOBGUI_TYPE_INSPECTOR_RECORDER))

typedef struct _BobguiInspectorRecorder BobguiInspectorRecorder;

G_BEGIN_DECLS

GType           bobgui_inspector_recorder_get_type                 (void);

void            bobgui_inspector_recorder_set_recording            (BobguiInspectorRecorder   *recorder,
                                                                 gboolean                record);
gboolean        bobgui_inspector_recorder_is_recording             (BobguiInspectorRecorder   *recorder);
void            bobgui_inspector_recorder_record_single_frame      (BobguiInspectorRecorder   *recorder,
                                                                 gboolean                save_to_file);

void            bobgui_inspector_recorder_set_debug_nodes          (BobguiInspectorRecorder   *recorder,
                                                                 gboolean                debug_nodes);

void            bobgui_inspector_recorder_set_highlight_sequences  (BobguiInspectorRecorder   *recorder,
                                                                 gboolean                highlight_sequences);

void            bobgui_inspector_recorder_set_selected_sequence    (BobguiInspectorRecorder   *recorder,
                                                                 GdkEventSequence       *sequence);

void            bobgui_inspector_recorder_record_render            (BobguiInspectorRecorder   *recorder,
                                                                 BobguiWidget              *widget,
                                                                 GskRenderer            *renderer,
                                                                 GdkSurface             *surface,
                                                                 const cairo_region_t   *region,
                                                                 GskRenderNode          *node);

void            bobgui_inspector_recorder_record_event             (BobguiInspectorRecorder   *recorder,
                                                                 BobguiWidget              *widget,
                                                                 GdkEvent               *event);
void            bobgui_inspector_recorder_trace_event              (BobguiInspectorRecorder   *recorder,
                                                                 GdkEvent               *event,
                                                                 BobguiPropagationPhase     phase,
                                                                 BobguiWidget              *widget,
                                                                 BobguiEventController     *controller,
                                                                 BobguiWidget              *target,
                                                                 gboolean                handled);
void            bobgui_inspector_recorder_add_profile_node         (BobguiInspectorRecorder   *self,
                                                                 GskRenderNode          *node,
                                                                 GskRenderNode          *profile_node);

G_END_DECLS


// vim: set et sw=2 ts=2:
