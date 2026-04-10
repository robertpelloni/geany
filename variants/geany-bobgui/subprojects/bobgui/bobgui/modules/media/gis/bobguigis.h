#ifndef BOBGUI_GIS_H
#define BOBGUI_GIS_H

#include <bobgui/bobgui.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_MAP_VIEW (bobgui_map_view_get_type ())
G_DECLARE_FINAL_TYPE (BobguiMapView, bobgui_map_view, BOBGUI, MAP_VIEW, BobguiWidget)

BobguiMapView * bobgui_map_view_new (void);

G_END_DECLS

#endif
