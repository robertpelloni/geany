#include "bobguiipc.h"
G_DEFINE_TYPE (BobguiIpcManager, bobgui_ipc_manager, G_TYPE_OBJECT)
static void bobgui_ipc_manager_init (BobguiIpcManager *s) {}
static void bobgui_ipc_manager_class_init (BobguiIpcManagerClass *k) {}
BobguiIpcManager * bobgui_ipc_manager_new (const char *n) { return g_object_new (BOBGUI_TYPE_IPC_MANAGER, NULL); }
