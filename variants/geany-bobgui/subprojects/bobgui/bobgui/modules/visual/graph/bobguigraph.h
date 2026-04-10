#ifndef BOBGUI_GRAPH_H
#define BOBGUI_GRAPH_H

#include <bobgui/bobgui.h>

G_BEGIN_DECLS

typedef struct _BobguiGraphNode BobguiGraphNode;
typedef struct _BobguiGraphPin BobguiGraphPin;

#define BOBGUI_TYPE_GRAPH_VIEW (bobgui_graph_view_get_type ())
G_DECLARE_FINAL_TYPE (BobguiGraphView, bobgui_graph_view, BOBGUI, GRAPH_VIEW, BobguiWidget)

BobguiGraphView * bobgui_graph_view_new (void);

G_END_DECLS

#endif
