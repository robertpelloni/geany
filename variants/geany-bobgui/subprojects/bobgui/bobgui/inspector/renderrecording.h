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

#define BOBGUI_TYPE_INSPECTOR_RENDER_RECORDING            (bobgui_inspector_render_recording_get_type())
#define BOBGUI_INSPECTOR_RENDER_RECORDING(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), BOBGUI_TYPE_INSPECTOR_RENDER_RECORDING, BobguiInspectorRenderRecording))
#define BOBGUI_INSPECTOR_RENDER_RECORDING_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), BOBGUI_TYPE_INSPECTOR_RENDER_RECORDING, BobguiInspectorRenderRecordingClass))
#define BOBGUI_INSPECTOR_IS_RENDER_RECORDING(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), BOBGUI_TYPE_INSPECTOR_RENDER_RECORDING))
#define BOBGUI_INSPECTOR_IS_RENDER_RECORDING_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), BOBGUI_TYPE_INSPECTOR_RENDER_RECORDING))
#define BOBGUI_INSPECTOR_RENDER_RECORDING_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), BOBGUI_TYPE_INSPECTOR_RENDER_RECORDING, BobguiInspectorRenderRecordingClass))


typedef struct _BobguiInspectorRenderRecordingPrivate BobguiInspectorRenderRecordingPrivate;

typedef struct _BobguiInspectorRenderRecording
{
  BobguiInspectorRecording parent;

  GdkRectangle area;
  cairo_region_t *clip_region;
  GskRenderNode *node;
  GskRenderNode *profile_node;
  gpointer surface;
} BobguiInspectorRenderRecording;

typedef struct _BobguiInspectorRenderRecordingClass
{
  BobguiInspectorRecordingClass parent;
} BobguiInspectorRenderRecordingClass;

GType           bobgui_inspector_render_recording_get_type         (void);

BobguiInspectorRecording *
                bobgui_inspector_render_recording_new              (gint64                             timestamp,
                                                                 const GdkRectangle                *area,
                                                                 const cairo_region_t              *clip_region,
                                                                 GskRenderNode                     *node,
                                                                 gpointer                           surface);

GskRenderNode * bobgui_inspector_render_recording_get_node         (BobguiInspectorRenderRecording       *recording);
GskRenderNode * bobgui_inspector_render_recording_get_profile_node (BobguiInspectorRenderRecording       *recording);
void            bobgui_inspector_render_recording_set_profile_node (BobguiInspectorRenderRecording       *recording,
                                                                 GskRenderNode                     *profile_node);
const cairo_region_t *
                bobgui_inspector_render_recording_get_clip_region  (BobguiInspectorRenderRecording       *recording);
const cairo_rectangle_int_t *
                bobgui_inspector_render_recording_get_area         (BobguiInspectorRenderRecording       *recording);

gpointer
                bobgui_inspector_render_recording_get_surface      (BobguiInspectorRenderRecording       *recording);

G_END_DECLS


// vim: set et sw=2 ts=2:
