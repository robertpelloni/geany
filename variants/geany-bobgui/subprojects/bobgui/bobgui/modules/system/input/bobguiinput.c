#include "bobguiinput.h"
G_DEFINE_TYPE (BobguiInputManager, bobgui_input_manager, G_TYPE_OBJECT)
static void bobgui_input_manager_init (BobguiInputManager *s) {}
static void bobgui_input_manager_class_init (BobguiInputManagerClass *k) {}
BobguiInputManager * bobgui_input_manager_get_default (void) {
    static BobguiInputManager *m = NULL;
    if (!m) m = g_object_new (BOBGUI_TYPE_INPUT_MANAGER, NULL);
    return m;
}
