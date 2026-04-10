#include "bobguirealtime.h"
G_DEFINE_TYPE (BobguiRealtimeKernel, bobgui_realtime_kernel, G_TYPE_OBJECT)
static void bobgui_realtime_kernel_init (BobguiRealtimeKernel *s) {}
static void bobgui_realtime_kernel_class_init (BobguiRealtimeKernelClass *k) {}
BobguiRealtimeKernel * bobgui_realtime_kernel_get_instance (void) {
    static BobguiRealtimeKernel *k = NULL;
    if (!k) k = g_object_new (BOBGUI_TYPE_REALTIME_KERNEL, NULL);
    return k;
}
