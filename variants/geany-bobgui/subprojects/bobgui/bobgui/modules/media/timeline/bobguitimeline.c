#include "bobguitimeline.h"
G_DEFINE_TYPE (BobguiTimeline, bobgui_timeline, G_TYPE_OBJECT)
static void bobgui_timeline_init (BobguiTimeline *s) {}
static void bobgui_timeline_class_init (BobguiTimelineClass *k) {}
BobguiTimeline * bobgui_timeline_new (void) { return g_object_new (BOBGUI_TYPE_TIMELINE, NULL); }
