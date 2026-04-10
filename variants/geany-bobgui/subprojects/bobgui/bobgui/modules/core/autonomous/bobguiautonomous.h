/* bobgui/modules/core/autonomous/bobguiautonomous.h */
#ifndef BOBGUI_AUTONOMOUS_H
#define BOBGUI_AUTONOMOUS_H

#include <bobgui/bobgui.h>
#include <bobgui/modules/core/brain/bobguibrain.h>

G_BEGIN_DECLS

/* Autonomous UI Engine (Self-Healing & Auto-Optimizing) */
#define BOBGUI_TYPE_AUTONOMOUS_ENGINE (bobgui_autonomous_engine_get_type ())
G_DECLARE_FINAL_TYPE (BobguiAutonomousEngine, bobgui_autonomous_engine, BOBGUI, AUTONOMOUS_ENGINE, GObject)

BobguiAutonomousEngine * bobgui_autonomous_engine_get_default (void);

/* Self-Healing: Detects UI issues (overlap, contrast, accessibility) and fixes them */
void bobgui_autonomous_heal_layout (BobguiAutonomousEngine *self, BobguiWidget *root);

/* Auto-Optimization: Learns user behavior to rearrange widgets for speed */
void bobgui_autonomous_optimize_workflow (BobguiAutonomousEngine *self, BobguiWidget *root);

/* Continuous A/B Testing: Automatically tests variants of UI to find highest engagement */
void bobgui_autonomous_start_ab_test (BobguiAutonomousEngine *self, 
                                     BobguiWidget *root, 
                                     const char *goal_id);

G_END_DECLS

#endif /* BOBGUI_AUTONOMOUS_H */
