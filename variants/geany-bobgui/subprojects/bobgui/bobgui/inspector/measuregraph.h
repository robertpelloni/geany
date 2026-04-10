/*
 * Copyright © 2021 Benjamin Otte
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Benjamin Otte <otte@gnome.org>
 */

#pragma once

#include <bobgui/bobgui.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_INSPECTOR_MEASURE_GRAPH (bobgui_inspector_measure_graph_get_type ())

G_DECLARE_FINAL_TYPE (BobguiInspectorMeasureGraph, bobgui_inspector_measure_graph, BOBGUI, INSPECTOR_MEASURE_GRAPH, GObject)

BobguiInspectorMeasureGraph * bobgui_inspector_measure_graph_new      (void);

void                    bobgui_inspector_measure_graph_clear       (BobguiInspectorMeasureGraph       *self);
void                    bobgui_inspector_measure_graph_measure     (BobguiInspectorMeasureGraph       *self,
                                                                 BobguiWidget                      *widget);
GdkTexture *            bobgui_inspector_measure_graph_get_texture (BobguiInspectorMeasureGraph       *self);

G_END_DECLS

