#include "bobguirhi.h"
G_DEFINE_TYPE (BobguiRhiDevice, bobgui_rhi_device, G_TYPE_OBJECT)
static void bobgui_rhi_device_init (BobguiRhiDevice *s) {}
static void bobgui_rhi_device_class_init (BobguiRhiDeviceClass *k) {}
BobguiRhiDevice * bobgui_rhi_device_new (BobguiRhiBackend b) { return g_object_new (BOBGUI_TYPE_RHI_DEVICE, NULL); }
