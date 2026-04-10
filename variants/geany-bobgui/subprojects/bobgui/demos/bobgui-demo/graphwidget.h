#pragma once

#include <bobgui/bobgui.h>

#define GRAPH_TYPE_WIDGET (graph_widget_get_type ())
G_DECLARE_FINAL_TYPE (GraphWidget, graph_widget, GRAPH, WIDGET, BobguiWidget)

BobguiWidget * graph_widget_new       (void);
