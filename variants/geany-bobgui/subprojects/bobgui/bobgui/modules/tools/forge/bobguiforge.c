#include "bobguiforge.h"
G_DEFINE_TYPE (BobguiForgeContext, bobgui_forge_context, G_TYPE_OBJECT)
static void bobgui_forge_context_init (BobguiForgeContext *s) {}
static void bobgui_forge_context_class_init (BobguiForgeContextClass *k) {}
BobguiForgeContext * bobgui_forge_context_new (const char *p) { return g_object_new (BOBGUI_TYPE_FORGE_CONTEXT, NULL); }
