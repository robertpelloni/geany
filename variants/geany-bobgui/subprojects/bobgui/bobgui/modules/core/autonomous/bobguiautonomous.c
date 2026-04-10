#include "bobguiautonomous.h"
G_DEFINE_TYPE (BobguiAutonomousEngine, bobgui_autonomous_engine, G_TYPE_OBJECT)
static void bobgui_autonomous_engine_init (BobguiAutonomousEngine *s) {}
static void bobgui_autonomous_engine_class_init (BobguiAutonomousEngineClass *k) {}
BobguiAutonomousEngine * bobgui_autonomous_engine_get_default (void) {
    static BobguiAutonomousEngine *e = NULL;
    if (!e) e = g_object_new (BOBGUI_TYPE_AUTONOMOUS_ENGINE, NULL);
    return e;
}
