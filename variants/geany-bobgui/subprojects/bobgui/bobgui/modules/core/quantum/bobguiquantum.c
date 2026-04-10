#include "bobguiquantum.h"
G_DEFINE_TYPE (BobguiQuantumScheduler, bobgui_quantum_scheduler, G_TYPE_OBJECT)
static void bobgui_quantum_scheduler_init (BobguiQuantumScheduler *self) {}
static void bobgui_quantum_scheduler_class_init (BobguiQuantumSchedulerClass *klass) {}
BobguiQuantumScheduler * bobgui_quantum_scheduler_get_default (void) {
    static BobguiQuantumScheduler *s = NULL;
    if (!s) s = g_object_new (BOBGUI_TYPE_QUANTUM_SCHEDULER, NULL);
    return s;
}
