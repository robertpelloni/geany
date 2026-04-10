#ifndef BOBGUI_AUDIO_H
#define BOBGUI_AUDIO_H

#include <glib-object.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_AUDIO_PROCESSOR (bobgui_audio_processor_get_type ())
G_DECLARE_DERIVABLE_TYPE (BobguiAudioProcessor, bobgui_audio_processor, BOBGUI, AUDIO_PROCESSOR, GObject)

struct _BobguiAudioProcessorClass {
  GObjectClass parent_class;
};

typedef struct _BobguiAudioDeviceManager BobguiAudioDeviceManager;

BobguiAudioDeviceManager * bobgui_audio_device_manager_new (void);

G_END_DECLS

#endif
