#include "bobguiremote.h"
G_DEFINE_TYPE (BobguiRemoteServer, bobgui_remote_server, G_TYPE_OBJECT)
static void bobgui_remote_server_init (BobguiRemoteServer *s) {}
static void bobgui_remote_server_class_init (BobguiRemoteServerClass *k) {}
BobguiRemoteServer * bobgui_remote_server_new (int p, const char *c) { return g_object_new (BOBGUI_TYPE_REMOTE_SERVER, NULL); }
