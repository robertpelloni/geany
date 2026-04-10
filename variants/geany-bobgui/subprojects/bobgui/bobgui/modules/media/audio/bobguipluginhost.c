#include "bobguipluginhost.h"
G_DEFINE_TYPE (BobguiPluginHostManager, bobgui_plugin_host_manager, G_TYPE_OBJECT)
static void bobgui_plugin_host_manager_init (BobguiPluginHostManager *s) {}
static void bobgui_plugin_host_manager_class_init (BobguiPluginHostManagerClass *k) {}
BobguiPluginHostManager * bobgui_plugin_host_manager_new (void) { return g_object_new (BOBGUI_TYPE_PLUGIN_HOST_MANAGER, NULL); }
