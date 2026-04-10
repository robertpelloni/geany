#include "bobguiiot.h"
G_DEFINE_TYPE (BobguiIotManager, bobgui_iot_manager, G_TYPE_OBJECT)
static void bobgui_iot_manager_init (BobguiIotManager *s) {}
static void bobgui_iot_manager_class_init (BobguiIotManagerClass *k) {}
BobguiIotManager * bobgui_iot_manager_get_default (void) { return g_object_new (BOBGUI_TYPE_IOT_MANAGER, NULL); }
