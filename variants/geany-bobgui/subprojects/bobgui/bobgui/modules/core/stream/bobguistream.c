#include "bobguistream.h"
G_DEFINE_TYPE (BobguiStream, bobgui_stream, G_TYPE_OBJECT)
static void bobgui_stream_init (BobguiStream *s) {}
static void bobgui_stream_class_init (BobguiStreamClass *k) {}
BobguiStream * bobgui_stream_interval (int i) { return g_object_new (BOBGUI_TYPE_STREAM, NULL); }
