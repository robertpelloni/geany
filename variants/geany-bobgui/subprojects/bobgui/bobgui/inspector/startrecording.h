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

#include <gdk/gdk.h>
#include <gsk/gsk.h>

#include "inspector/recording.h"

G_BEGIN_DECLS

#define BOBGUI_TYPE_INSPECTOR_START_RECORDING            (bobgui_inspector_start_recording_get_type())
#define BOBGUI_INSPECTOR_START_RECORDING(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), BOBGUI_TYPE_INSPECTOR_START_RECORDING, BobguiInspectorStartRecording))
#define BOBGUI_INSPECTOR_START_RECORDING_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), BOBGUI_TYPE_INSPECTOR_START_RECORDING, BobguiInspectorStartRecordingClass))
#define BOBGUI_INSPECTOR_IS_START_RECORDING(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), BOBGUI_TYPE_INSPECTOR_START_RECORDING))
#define BOBGUI_INSPECTOR_IS_START_RECORDING_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), BOBGUI_TYPE_INSPECTOR_START_RECORDING))
#define BOBGUI_INSPECTOR_START_RECORDING_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), BOBGUI_TYPE_INSPECTOR_START_RECORDING, BobguiInspectorStartRecordingClass))


typedef struct _BobguiInspectorStartRecordingPrivate BobguiInspectorStartRecordingPrivate;

typedef struct _BobguiInspectorStartRecording
{
  BobguiInspectorRecording parent;

} BobguiInspectorStartRecording;

typedef struct _BobguiInspectorStartRecordingClass
{
  BobguiInspectorRecordingClass parent;
} BobguiInspectorStartRecordingClass;

GType           bobgui_inspector_start_recording_get_type          (void);

BobguiInspectorRecording *
                bobgui_inspector_start_recording_new               (void);


G_END_DECLS


// vim: set et sw=2 ts=2:
