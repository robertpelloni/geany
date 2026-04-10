#ifndef BOBGUI_QUANTUM_H
#define BOBGUI_QUANTUM_H

#include <glib-object.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_QUANTUM_SCHEDULER (bobgui_quantum_scheduler_get_type ())
G_DECLARE_FINAL_TYPE (BobguiQuantumScheduler, bobgui_quantum_scheduler, BOBGUI, QUANTUM_SCHEDULER, GObject)

BobguiQuantumScheduler * bobgui_quantum_scheduler_get_default (void);

G_END_DECLS

#endif
