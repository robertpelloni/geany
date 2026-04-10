/* bobgui/modules/media/audio/bobguimidi2.h */
#ifndef BOBGUI_MIDI2_H
#define BOBGUI_MIDI2_H

#include <glib-object.h>

G_BEGIN_DECLS

/* Next-Gen MIDI 2.0 Hub (Better than standard MIDI toolkits) */
#define BOBGUI_TYPE_MIDI2_CONTEXT (bobgui_midi2_context_get_type ())
G_DECLARE_FINAL_TYPE (BobguiMidi2Context, bobgui_midi2_context, BOBGUI, MIDI2_CONTEXT, GObject)

BobguiMidi2Context * bobgui_midi2_context_get_default (void);

/* Universal MIDI Packet (UMP) API */
typedef struct {
    uint32_t data[4];
} BobguiMidi2Packet;

void bobgui_midi2_send_packet (BobguiMidi2Context *self, BobguiMidi2Packet *packet);

/* Per-Note Controller (Unmatched Parity: High-res expressivity) */
void bobgui_midi2_set_per_note_pitch_bend (BobguiMidi2Context *self, 
                                          int channel, 
                                          int note, 
                                          double pitch_bend);

/* Capability Inquiry (CI) and Auto-Configuration */
void bobgui_midi2_discover_profiles (BobguiMidi2Context *self, int device_id);

G_END_DECLS

#endif /* BOBGUI_MIDI2_H */
