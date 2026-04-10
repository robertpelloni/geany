/* bobgui/modules/media/audio/bobguiaudiowidgets.h */
#ifndef BOBGUI_AUDIO_WIDGETS_H
#define BOBGUI_AUDIO_WIDGETS_H

#include <bobgui/bobgui.h>

G_BEGIN_DECLS

/* High-fidelity Audio Controls (Better than JUCE standard components) */

/* Dial/Knob with specialized rendering modes */
#define BOBGUI_TYPE_AUDIO_DIAL (bobgui_audio_dial_get_type ())
G_DECLARE_FINAL_TYPE (BobguiAudioDial, bobgui_audio_dial, BOBGUI, AUDIO_DIAL, BobguiWidget)
BobguiAudioDial * bobgui_audio_dial_new (void);

/* Vertical/Horizontal Fader with logarithmic scaling */
#define BOBGUI_TYPE_AUDIO_FADER (bobgui_audio_fader_get_type ())
G_DECLARE_FINAL_TYPE (BobguiAudioFader, bobgui_audio_fader, BOBGUI, AUDIO_FADER, BobguiWidget)
BobguiAudioFader * bobgui_audio_fader_new (void);

/* Real-time Oscilloscope (Hardware Accelerated) */
#define BOBGUI_TYPE_AUDIO_SCOPE (bobgui_audio_scope_get_type ())
G_DECLARE_FINAL_TYPE (BobguiAudioScope, bobgui_audio_scope, BOBGUI, AUDIO_SCOPE, BobguiWidget)
BobguiAudioScope * bobgui_audio_scope_new (void);

/* Spectral Analyzer (FFT-driven visualization) */
#define BOBGUI_TYPE_AUDIO_ANALYZER (bobgui_audio_analyzer_get_type ())
G_DECLARE_FINAL_TYPE (BobguiAudioAnalyzer, bobgui_audio_analyzer, BOBGUI, AUDIO_ANALYZER, BobguiWidget)
BobguiAudioAnalyzer * bobgui_audio_analyzer_new (void);

/* Data Binding (Reactive integration with AudioProcessor) */
void bobgui_audio_widget_bind_parameter (BobguiWidget *widget, const char *param_id);

G_END_DECLS

#endif /* BOBGUI_AUDIO_WIDGETS_H */
