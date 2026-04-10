#include "bobguiscript.h"
G_DEFINE_TYPE (BobguiScriptContext, bobgui_script_context, G_TYPE_OBJECT)
static void bobgui_script_context_init (BobguiScriptContext *s) {}
static void bobgui_script_context_class_init (BobguiScriptContextClass *k) {}
BobguiScriptContext * bobgui_script_context_new (const char *e) { return g_object_new (BOBGUI_TYPE_SCRIPT_CONTEXT, NULL); }
