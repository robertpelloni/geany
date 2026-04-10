#pragma once

#include <bobgui/bobgui.h>
#include "path-paintable.h"

#define MINI_GRAPH_TYPE (mini_graph_get_type ())
G_DECLARE_FINAL_TYPE (MiniGraph, mini_graph, MINI, GRAPH, BobguiWidget)

BobguiWidget * mini_graph_new (void);

void mini_graph_set_easing (MiniGraph *self,
                            GpaEasing  easing);
