#include "bobguistudio.h"
G_DEFINE_TYPE (BobguiStudioManager, bobgui_studio_manager, G_TYPE_OBJECT)
static void bobgui_studio_manager_init (BobguiStudioManager *s) {}
static void bobgui_studio_manager_class_init (BobguiStudioManagerClass *k) {}
BobguiStudioManager * bobgui_studio_manager_get_default (void) { return g_object_new (BOBGUI_TYPE_STUDIO_MANAGER, NULL); }
