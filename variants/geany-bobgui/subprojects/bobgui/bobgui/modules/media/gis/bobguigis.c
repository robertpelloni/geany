#include "bobguigis.h"
G_DEFINE_TYPE (BobguiMapView, bobgui_map_view, BOBGUI_TYPE_WIDGET)
static void bobgui_map_view_init (BobguiMapView *self) {}
static void bobgui_map_view_class_init (BobguiMapViewClass *klass) {}
BobguiMapView * bobgui_map_view_new (void) { return g_object_new (BOBGUI_TYPE_MAP_VIEW, NULL); }
