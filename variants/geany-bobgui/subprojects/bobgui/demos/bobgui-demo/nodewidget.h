#pragma once

#include <bobgui/bobgui.h>

#define NODE_TYPE_WIDGET (node_widget_get_type ())
G_DECLARE_FINAL_TYPE (NodeWidget, node_widget, NODE, WIDGET, BobguiWidget)

BobguiWidget * node_widget_new       (const char *resource);
