#include "bobguistate.h"
G_DEFINE_TYPE (BobguiStateMachine, bobgui_state_machine, G_TYPE_OBJECT)
static void bobgui_state_machine_init (BobguiStateMachine *self) {}
static void bobgui_state_machine_class_init (BobguiStateMachineClass *klass) {}
BobguiStateMachine * bobgui_state_machine_new (BobguiStateNode *i) { return g_object_new (BOBGUI_TYPE_STATE_MACHINE, NULL); }
