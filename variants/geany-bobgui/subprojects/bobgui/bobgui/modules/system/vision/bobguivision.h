/* bobgui/modules/system/vision/bobguivision.h */
#ifndef BOBGUI_VISION_H
#define BOBGUI_VISION_H

#include <bobgui/bobgui.h>
#include <bobgui/modules/core/brain/bobguibrain.h>

G_BEGIN_DECLS

/* AI-Driven Accessibility Engine (Better than standard Screen Readers) */
#define BOBGUI_TYPE_VISION_CONTEXT (bobgui_vision_context_get_type ())
G_DECLARE_FINAL_TYPE (BobguiVisionContext, bobgui_vision_context, BOBGUI, VISION_CONTEXT, GObject)

BobguiVisionContext * bobgui_vision_context_get_default (void);

/* Automated Semantic Analysis (No manual tagging required) */
void bobgui_vision_analyze_ui (BobguiVisionContext *self, BobguiWidget *root);

/* Intent-based Navigation (Predictive A11y) */
void bobgui_vision_predict_next_action (BobguiVisionContext *self, 
                                       BobguiWidget *root, 
                                       char **out_description);

/* Real-time Gesture Analysis (Superior Parity with Apple VisionOS) */
void bobgui_vision_process_camera_feed (BobguiVisionContext *self, 
                                       gpointer frame_data, 
                                       int width, int height);

G_END_DECLS

#endif /* BOBGUI_VISION_H */
