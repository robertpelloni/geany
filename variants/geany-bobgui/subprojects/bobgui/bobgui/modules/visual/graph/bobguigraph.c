#include "bobguigraph.h"
G_DEFINE_TYPE (BobguiGraphView, bobgui_graph_view, BOBGUI_TYPE_WIDGET)
static void bobgui_graph_view_init (BobguiGraphView *s) {}
static void bobgui_graph_view_class_init (BobguiGraphViewClass *k) {}
BobguiGraphView * bobgui_graph_view_new (void) { return g_object_new (BOBGUI_TYPE_GRAPH_VIEW, NULL); }
