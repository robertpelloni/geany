#include "bobguientity.h"
G_DEFINE_TYPE (BobguiEntityWorld, bobgui_entity_world, G_TYPE_OBJECT)
static void bobgui_entity_world_init (BobguiEntityWorld *s) {}
static void bobgui_entity_world_class_init (BobguiEntityWorldClass *k) {}
BobguiEntityWorld * bobgui_entity_world_new (void) { return g_object_new (BOBGUI_TYPE_ENTITY_WORLD, NULL); }
