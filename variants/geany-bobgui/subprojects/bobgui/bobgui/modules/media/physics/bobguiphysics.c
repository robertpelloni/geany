#include "bobguiphysics.h"
G_DEFINE_TYPE (BobguiPhysicsWorld, bobgui_physics_world, G_TYPE_OBJECT)
static void bobgui_physics_world_init (BobguiPhysicsWorld *s) {}
static void bobgui_physics_world_class_init (BobguiPhysicsWorldClass *k) {}
BobguiPhysicsWorld * bobgui_physics_world_new (void) { return g_object_new (BOBGUI_TYPE_PHYSICS_WORLD, NULL); }
