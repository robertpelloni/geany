#include "bobguidesign.h"
G_DEFINE_TYPE (BobguiDesignSystem, bobgui_design_system, G_TYPE_OBJECT)
static void bobgui_design_system_init (BobguiDesignSystem *s) {}
static void bobgui_design_system_class_init (BobguiDesignSystemClass *k) {}
BobguiDesignSystem * bobgui_design_system_get_default (void) { return g_object_new (BOBGUI_TYPE_DESIGN_SYSTEM, NULL); }
