#include "bobguimidi2.h"
G_DEFINE_TYPE (BobguiMidi2Context, bobgui_midi2_context, G_TYPE_OBJECT)
static void bobgui_midi2_context_init (BobguiMidi2Context *s) {}
static void bobgui_midi2_context_class_init (BobguiMidi2ContextClass *k) {}
BobguiMidi2Context * bobgui_midi2_context_get_default (void) {
    static BobguiMidi2Context *c = NULL;
    if (!c) c = g_object_new (BOBGUI_TYPE_MIDI2_CONTEXT, NULL);
    return c;
}
