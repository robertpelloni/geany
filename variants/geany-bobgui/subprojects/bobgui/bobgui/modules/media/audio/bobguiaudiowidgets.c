#include "bobguiaudiowidgets.h"
G_DEFINE_TYPE (BobguiAudioDial, bobgui_audio_dial, BOBGUI_TYPE_WIDGET)
static void bobgui_audio_dial_init (BobguiAudioDial *s) {}
static void bobgui_audio_dial_class_init (BobguiAudioDialClass *k) {}
BobguiAudioDial * bobgui_audio_dial_new (void) { return g_object_new (BOBGUI_TYPE_AUDIO_DIAL, NULL); }

G_DEFINE_TYPE (BobguiAudioFader, bobgui_audio_fader, BOBGUI_TYPE_WIDGET)
static void bobgui_audio_fader_init (BobguiAudioFader *s) {}
static void bobgui_audio_fader_class_init (BobguiAudioFaderClass *k) {}
BobguiAudioFader * bobgui_audio_fader_new (void) { return g_object_new (BOBGUI_TYPE_AUDIO_FADER, NULL); }
