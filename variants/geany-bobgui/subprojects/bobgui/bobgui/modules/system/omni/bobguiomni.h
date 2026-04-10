/* bobgui/modules/system/omni/bobguiomni.h */
#ifndef BOBGUI_OMNI_H
#define BOBGUI_OMNI_H

#include <glib-object.h>
#include <bobgui/bobgui.h>

G_BEGIN_DECLS

/* Omni Platform Abstraction (Zero-Config Cross-Platform) */
#define BOBGUI_TYPE_OMNI_RUNTIME (bobgui_omni_runtime_get_type ())
G_DECLARE_FINAL_TYPE (BobguiOmniRuntime, bobgui_omni_runtime, BOBGUI, OMNI_RUNTIME, GObject)

BobguiOmniRuntime * bobgui_omni_runtime_get_default (void);

/* Single Entry Point: Handles Desktop, Mobile, and WASM transparently */
void bobgui_omni_run_application (BobguiOmniRuntime *self, 
                                 BobguiWidget *main_ui, 
                                 int argc, 
                                 char **argv);

/* Deep Platform Integration: Bridge native APIs (Camera, GPS, FaceID) to generic GObject properties */
void bobgui_omni_request_capability (BobguiOmniRuntime *self, 
                                    const char *capability_id, 
                                    GAsyncReadyCallback callback);

/* Automated Binary Translation: Optimize execution for local CPU/GPU architecture at launch */
void bobgui_omni_optimize_jit (BobguiOmniRuntime *self);

G_END_DECLS

#endif /* BOBGUI_OMNI_H */
