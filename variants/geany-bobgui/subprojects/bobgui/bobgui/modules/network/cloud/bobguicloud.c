#include "bobguicloud.h"
G_DEFINE_TYPE (BobguiCloudContext, bobgui_cloud_context, G_TYPE_OBJECT)
static void bobgui_cloud_context_init (BobguiCloudContext *s) {}
static void bobgui_cloud_context_class_init (BobguiCloudContextClass *k) {}
BobguiCloudContext * bobgui_cloud_context_new (const char *u, const char *k) { return g_object_new (BOBGUI_TYPE_CLOUD_CONTEXT, NULL); }
