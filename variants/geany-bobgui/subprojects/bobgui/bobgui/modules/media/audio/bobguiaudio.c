#include "bobguiaudio.h"
G_DEFINE_ABSTRACT_TYPE (BobguiAudioProcessor, bobgui_audio_processor, G_TYPE_OBJECT)
static void bobgui_audio_processor_class_init (BobguiAudioProcessorClass *klass) {}
static void bobgui_audio_processor_init (BobguiAudioProcessor *self) {}
BobguiAudioDeviceManager * bobgui_audio_device_manager_new (void) { return g_new0 (BobguiAudioDeviceManager, 1); }
