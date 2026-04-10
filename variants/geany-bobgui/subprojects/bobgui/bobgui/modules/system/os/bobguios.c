#include "bobguios.h"
G_DEFINE_TYPE (BobguiVirtualOs, bobgui_virtual_os, BOBGUI_TYPE_WIDGET)
static void bobgui_virtual_os_init (BobguiVirtualOs *s) {}
static void bobgui_virtual_os_class_init (BobguiVirtualOsClass *k) {}
BobguiVirtualOs * bobgui_virtual_os_new (void) { return g_object_new (BOBGUI_TYPE_VIRTUAL_OS, NULL); }
