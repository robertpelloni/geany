#include "bobguinetwork.h"
G_DEFINE_TYPE (BobguiNetworkAccessManager, bobgui_network_access_manager, G_TYPE_OBJECT)
static void bobgui_network_access_manager_init (BobguiNetworkAccessManager *s) {}
static void bobgui_network_access_manager_class_init (BobguiNetworkAccessManagerClass *k) {}
BobguiNetworkAccessManager * bobgui_network_access_manager_new (void) { return g_object_new (BOBGUI_TYPE_NETWORK_ACCESS_MANAGER, NULL); }
