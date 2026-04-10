#include "bobguicompute.h"
G_DEFINE_TYPE (BobguiComputeContext, bobgui_compute_context, G_TYPE_OBJECT)
static void bobgui_compute_context_init (BobguiComputeContext *s) {}
static void bobgui_compute_context_class_init (BobguiComputeContextClass *k) {}
BobguiComputeContext * bobgui_compute_context_new (const char *b) { return g_object_new (BOBGUI_TYPE_COMPUTE_CONTEXT, NULL); }
