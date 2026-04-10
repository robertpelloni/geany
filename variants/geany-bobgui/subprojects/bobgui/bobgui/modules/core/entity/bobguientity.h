/* bobgui/modules/core/entity/bobguientity.h */
#ifndef BOBGUI_ENTITY_H
#define BOBGUI_ENTITY_H

#include <glib-object.h>

G_BEGIN_DECLS

/* Entity Component System (Better than standard OO hierarchies) */
typedef uint64_t BobguiEntityId;

#define BOBGUI_TYPE_ENTITY_WORLD (bobgui_entity_world_get_type ())
G_DECLARE_FINAL_TYPE (BobguiEntityWorld, bobgui_entity_world, BOBGUI, ENTITY_WORLD, GObject)

BobguiEntityWorld * bobgui_entity_world_new (void);

/* Component API (Zero-allocation / Data-oriented) */
typedef struct {
    float x, y, z;
} BobguiComponentTransform;

typedef struct {
    float r, g, b, a;
} BobguiComponentColor;

void bobgui_entity_add_component (BobguiEntityWorld *self, 
                                 BobguiEntityId entity, 
                                 GType component_type, 
                                 gpointer data);

/* High-Performance System Processing (Million+ entities per frame) */
typedef void (*BobguiEntitySystemFunc) (BobguiEntityWorld *world, GList *entities);
void bobgui_entity_register_system (BobguiEntityWorld *self, 
                                   BobguiEntitySystemFunc system_func);

void bobgui_entity_world_step (BobguiEntityWorld *self, float dt);

G_END_DECLS

#endif /* BOBGUI_ENTITY_H */
