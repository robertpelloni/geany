/* bobgui/modules/core/realtime/bobguirealtime.h */
#ifndef BOBGUI_REALTIME_H
#define BOBGUI_REALTIME_H

#include <glib-object.h>

G_BEGIN_DECLS

/* Hard Real-Time Execution Engine (Better than standard threads) */
#define BOBGUI_TYPE_REALTIME_KERNEL (bobgui_realtime_kernel_get_type ())
G_DECLARE_FINAL_TYPE (BobguiRealtimeKernel, bobgui_realtime_kernel, BOBGUI, REALTIME_KERNEL, GObject)

BobguiRealtimeKernel * bobgui_realtime_kernel_get_instance (void);

/* Deterministic Scheduling (No GC pauses, No Lock contention) */
typedef void (*BobguiRealtimeFunc) (gpointer data);

void bobgui_realtime_reserve_priority (BobguiRealtimeKernel *self, int priority_level);
void bobgui_realtime_spawn_task (BobguiRealtimeKernel *self, 
                                BobguiRealtimeFunc func, 
                                gpointer data);

/* Critical Path Monitoring: Guarantees execution within microsecond deadlines */
void bobgui_realtime_check_deadlines (BobguiRealtimeKernel *self);

/* Industrial Integration: Fieldbus and PLC synchronization support */
void bobgui_realtime_sync_to_external_clock (BobguiRealtimeKernel *self, const char *clock_id);

G_END_DECLS

#endif /* BOBGUI_REALTIME_H */
