#include "bobguiomni.h"
G_DEFINE_TYPE (BobguiOmniRuntime, bobgui_omni_runtime, G_TYPE_OBJECT)
static void bobgui_omni_runtime_init (BobguiOmniRuntime *s) {}
static void bobgui_omni_runtime_class_init (BobguiOmniRuntimeClass *k) {}
BobguiOmniRuntime * bobgui_omni_runtime_get_default (void) {
    static BobguiOmniRuntime *r = NULL;
    if (!r) r = g_object_new (BOBGUI_TYPE_OMNI_RUNTIME, NULL);
    return r;
}
