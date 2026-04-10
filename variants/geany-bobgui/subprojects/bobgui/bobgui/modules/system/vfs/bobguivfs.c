#include "bobguivfs.h"
G_DEFINE_TYPE (BobguiVFS, bobgui_vfs, G_TYPE_OBJECT)
static void bobgui_vfs_init (BobguiVFS *s) {}
static void bobgui_vfs_class_init (BobguiVFSClass *k) {}
BobguiVFS * bobgui_vfs_get_default (void) { return g_object_new (BOBGUI_TYPE_VFS, NULL); }
