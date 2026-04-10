#include "bobguivision.h"
G_DEFINE_TYPE (BobguiVisionContext, bobgui_vision_context, G_TYPE_OBJECT)
static void bobgui_vision_context_init (BobguiVisionContext *s) {}
static void bobgui_vision_context_class_init (BobguiVisionContextClass *k) {}
BobguiVisionContext * bobgui_vision_context_get_default (void) { return g_object_new (BOBGUI_TYPE_VISION_CONTEXT, NULL); }
