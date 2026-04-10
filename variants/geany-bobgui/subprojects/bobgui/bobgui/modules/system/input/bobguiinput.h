/* bobgui/modules/system/input/bobguiinput.h */
#ifndef BOBGUI_INPUT_H
#define BOBGUI_INPUT_H

#include <glib-object.h>
#include <bobgui/bobgui.h>

G_BEGIN_DECLS

/* Unified Input Hub (Better than Qt Gamepad / SDL wrappers) */
#define BOBGUI_TYPE_INPUT_MANAGER (bobgui_input_manager_get_type ())
G_DECLARE_FINAL_TYPE (BobguiInputManager, bobgui_input_manager, BOBGUI, INPUT_MANAGER, GObject)

BobguiInputManager * bobgui_input_manager_get_default (void);

/* Gamepad and Joystick API (High-precision / Low-latency) */
void bobgui_input_scan_devices (BobguiInputManager *self);
void bobgui_input_get_axis_state (BobguiInputManager *self, int device_id, int axis, float *out_value);

/* Advanced Haptic Feedback (Superior to standard vibration) */
typedef struct _BobguiHapticPattern BobguiHapticPattern;
void bobgui_input_play_haptic (BobguiInputManager *self, 
                              int device_id, 
                              BobguiHapticPattern *pattern);

/* Raw Input Access (For specialized controllers/simulators) */
void bobgui_input_capture_raw (BobguiInputManager *self, 
                              int device_id, 
                              GAsyncReadyCallback callback);

G_END_DECLS

#endif /* BOBGUI_INPUT_H */
