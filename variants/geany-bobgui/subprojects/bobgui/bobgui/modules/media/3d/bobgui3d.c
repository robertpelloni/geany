#include "bobgui3d.h"
G_DEFINE_TYPE (Bobgui3dNode, bobgui_3d_node, G_TYPE_OBJECT)
static void bobgui_3d_node_init (Bobgui3dNode *self) {}
static void bobgui_3d_node_class_init (Bobgui3dNodeClass *klass) {}
Bobgui3dNode * bobgui_3d_node_new (void) { return g_object_new (BOBGUI_TYPE_3D_NODE, NULL); }
