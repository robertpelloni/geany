#include "bobguisync.h"
G_DEFINE_TYPE (BobguiSyncEngine, bobgui_sync_engine, G_TYPE_OBJECT)
static void bobgui_sync_engine_init (BobguiSyncEngine *s) {}
static void bobgui_sync_engine_class_init (BobguiSyncEngineClass *k) {}
BobguiSyncEngine * bobgui_sync_engine_new (BobguiStateMachine *sm) { return g_object_new (BOBGUI_TYPE_SYNC_ENGINE, NULL); }
