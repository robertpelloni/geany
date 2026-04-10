/*
 * Copyright (c) 2025 Benjamin Otte
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

#include <bobgui.h>

#include "gsk/gskdebugnodeprivate.h"

G_BEGIN_DECLS

/* Keep in sync with dropdown in recorder.ui */
typedef enum  {
  NODE_WRAPPER_RENDER_DEFAULT,
  NODE_WRAPPER_RENDER_GPU_TIME,
  NODE_WRAPPER_RENDER_OFFSCREENS,
  NODE_WRAPPER_RENDER_UPLOADS,
} NodeWrapperRendering;

#define BOBGUI_TYPE_INSPECTOR_NODE_WRAPPER bobgui_inspector_node_wrapper_get_type ()

G_DECLARE_FINAL_TYPE (BobguiInspectorNodeWrapper, bobgui_inspector_node_wrapper, BOBGUI, INSPECTOR_NODE_WRAPPER, GObject)


BobguiInspectorNodeWrapper *
                        bobgui_inspector_node_wrapper_new                  (GskRenderNode                  *node,
                                                                         GskRenderNode                  *perf_node,
                                                                         GskRenderNode                  *draw_node,
                                                                         const char                     *role);

GskRenderNode *         bobgui_inspector_node_wrapper_get_node             (BobguiInspectorNodeWrapper        *self);
GskRenderNode *         bobgui_inspector_node_wrapper_get_draw_node        (BobguiInspectorNodeWrapper        *self);
GskRenderNode *         bobgui_inspector_node_wrapper_get_profile_node     (BobguiInspectorNodeWrapper        *self);
const GskDebugProfile * bobgui_inspector_node_wrapper_get_profile          (BobguiInspectorNodeWrapper        *self);
const char *            bobgui_inspector_node_wrapper_get_role             (BobguiInspectorNodeWrapper        *self);

GskRenderNode *         bobgui_inspector_node_wrapper_render               (BobguiInspectorNodeWrapper        *self,
                                                                         NodeWrapperRendering            rendering);
GListModel *            bobgui_inspector_node_wrapper_create_children_model(BobguiInspectorNodeWrapper        *self);

G_END_DECLS
