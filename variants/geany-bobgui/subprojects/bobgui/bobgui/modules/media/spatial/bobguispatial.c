#include "bobguispatial.h"
G_DEFINE_TYPE (BobguiSpatialContext, bobgui_spatial_context, G_TYPE_OBJECT)
static void bobgui_spatial_context_init (BobguiSpatialContext *s) {}
static void bobgui_spatial_context_class_init (BobguiSpatialContextClass *k) {}
BobguiSpatialContext * bobgui_spatial_context_new (const char *r) { return g_object_new (BOBGUI_TYPE_SPATIAL_CONTEXT, NULL); }
