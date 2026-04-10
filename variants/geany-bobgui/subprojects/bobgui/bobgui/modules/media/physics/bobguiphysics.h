/* bobgui/physics/bobguiphysics.h */
#ifndef BOBGUI_PHYSICS_H
#define BOBGUI_PHYSICS_H

#include <bobgui/bobgui.h>
#include <graphene.h>

G_BEGIN_DECLS

typedef enum {
  BOBGUI_PHYSICS_BODY_STATIC,
  BOBGUI_PHYSICS_BODY_DYNAMIC,
  BOBGUI_PHYSICS_BODY_KINEMATIC
} BobguiPhysicsBodyType;

#define BOBGUI_TYPE_PHYSICS_WORLD (bobgui_physics_world_get_type ())
G_DECLARE_FINAL_TYPE (BobguiPhysicsWorld, bobgui_physics_world, BOBGUI, PHYSICS_WORLD, GObject)

BobguiPhysicsWorld * bobgui_physics_world_new (void);
void                  bobgui_physics_world_step (BobguiPhysicsWorld *self, float delta_time);
void                  bobgui_physics_world_set_gravity (BobguiPhysicsWorld *self, float x, float y, float z);

/* Integrated Physics Properties for Widgets */
void bobgui_widget_enable_physics (BobguiWidget *widget, 
                                   BobguiPhysicsWorld *world, 
                                   BobguiPhysicsBodyType type);

void bobgui_widget_apply_force (BobguiWidget *widget, graphene_vec3_t *force);

G_END_DECLS

#endif /* BOBGUI_PHYSICS_H */
