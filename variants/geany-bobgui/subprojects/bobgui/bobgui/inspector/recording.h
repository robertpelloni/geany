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

#include <glib-object.h>

#define BOBGUI_TYPE_INSPECTOR_RECORDING            (bobgui_inspector_recording_get_type())
#define BOBGUI_INSPECTOR_RECORDING(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), BOBGUI_TYPE_INSPECTOR_RECORDING, BobguiInspectorRecording))
#define BOBGUI_INSPECTOR_RECORDING_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), BOBGUI_TYPE_INSPECTOR_RECORDING, BobguiInspectorRecordingClass))
#define BOBGUI_INSPECTOR_IS_RECORDING(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), BOBGUI_TYPE_INSPECTOR_RECORDING))
#define BOBGUI_INSPECTOR_IS_RECORDING_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), BOBGUI_TYPE_INSPECTOR_RECORDING))
#define BOBGUI_INSPECTOR_RECORDING_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), BOBGUI_TYPE_INSPECTOR_RECORDING, BobguiInspectorRecordingClass))


typedef struct _BobguiInspectorRecordingPrivate BobguiInspectorRecordingPrivate;

typedef struct _BobguiInspectorRecording
{
  GObject parent;

  gint64 timestamp;
} BobguiInspectorRecording;

typedef struct _BobguiInspectorRecordingClass
{
  GObjectClass parent;
} BobguiInspectorRecordingClass;

G_BEGIN_DECLS

GType           bobgui_inspector_recording_get_type                (void);

gint64          bobgui_inspector_recording_get_timestamp           (BobguiInspectorRecording  *recording);

G_END_DECLS


// vim: set et sw=2 ts=2:
