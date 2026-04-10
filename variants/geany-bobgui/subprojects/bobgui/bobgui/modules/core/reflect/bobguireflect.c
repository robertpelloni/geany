#include "bobguireflect.h"
G_DEFINE_TYPE (BobguiReflectContext, bobgui_reflect_context, G_TYPE_OBJECT)
static void bobgui_reflect_context_init (BobguiReflectContext *s) {}
static void bobgui_reflect_context_class_init (BobguiReflectContextClass *k) {}
BobguiReflectContext * bobgui_reflect_context_get_default (void) { return g_object_new (BOBGUI_TYPE_REFLECT_CONTEXT, NULL); }
