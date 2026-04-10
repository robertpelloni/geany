#ifndef BOBGUI_STATE_H
#define BOBGUI_STATE_H

#include <glib-object.h>

G_BEGIN_DECLS

typedef struct _BobguiStateNode BobguiStateNode;

#define BOBGUI_TYPE_STATE_MACHINE (bobgui_state_machine_get_type ())
G_DECLARE_FINAL_TYPE (BobguiStateMachine, bobgui_state_machine, BOBGUI, STATE_MACHINE, GObject)

BobguiStateMachine * bobgui_state_machine_new (BobguiStateNode *initial_state);

G_END_DECLS

#endif
