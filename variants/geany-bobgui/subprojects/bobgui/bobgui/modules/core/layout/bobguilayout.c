#include "bobguilayout.h"
G_DEFINE_TYPE (BobguiLayoutManager, bobgui_layout_manager, G_TYPE_OBJECT)
static void bobgui_layout_manager_init (BobguiLayoutManager *s) {}
static void bobgui_layout_manager_class_init (BobguiLayoutManagerClass *k) {}
BobguiLayoutManager * bobgui_layout_manager_new (BobguiLayoutType t) { return g_object_new (BOBGUI_TYPE_LAYOUT_MANAGER, NULL); }
