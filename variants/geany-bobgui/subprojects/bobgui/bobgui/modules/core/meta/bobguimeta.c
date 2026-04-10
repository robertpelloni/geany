#include "bobguimeta.h"
G_DEFINE_TYPE (BobguiMetaOrchestrator, bobgui_meta_orchestrator, G_TYPE_OBJECT)
static void bobgui_meta_orchestrator_init (BobguiMetaOrchestrator *s) {}
static void bobgui_meta_orchestrator_class_init (BobguiMetaOrchestratorClass *k) {}
BobguiMetaOrchestrator * bobgui_meta_orchestrator_get_default (void) {
    static BobguiMetaOrchestrator *o = NULL;
    if (!o) o = g_object_new (BOBGUI_TYPE_META_ORCHESTRATOR, NULL);
    return o;
}
