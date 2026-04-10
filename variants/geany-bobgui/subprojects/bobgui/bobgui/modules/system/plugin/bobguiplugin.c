#include "bobguiplugin.h"
G_DEFINE_TYPE (BobguiPluginManager, bobgui_plugin_manager, G_TYPE_OBJECT)
static void bobgui_plugin_manager_init (BobguiPluginManager *s) {}
static void bobgui_plugin_manager_class_init (BobguiPluginManagerClass *k) {}
BobguiPluginManager * bobgui_plugin_manager_new (const char *p) { return g_object_new (BOBGUI_TYPE_PLUGIN_MANAGER, NULL); }
