/* bobgui/modules/tools/studio/bobguistudio.h */
#ifndef BOBGUI_STUDIO_H
#define BOBGUI_STUDIO_H

#include <bobgui/bobgui.h>

G_BEGIN_DECLS

/* Integrated Development & Profiling Suite (Better than ImGui Metrics) */
#define BOBGUI_TYPE_STUDIO_MANAGER (bobgui_studio_manager_get_type ())
G_DECLARE_FINAL_TYPE (BobguiStudioManager, bobgui_studio_manager, BOBGUI, STUDIO_MANAGER, GObject)

BobguiStudioManager * bobgui_studio_manager_get_default (void);

/* Real-time Performance Monitoring (Superior to standard profilers) */
void bobgui_studio_show_performance_overlay (BobguiStudioManager *self, gboolean show);
void bobgui_studio_log_metric (BobguiStudioManager *self, const char *name, double value);

/* Live UI Inspection and Editing (Better than standard Inspector) */
void bobgui_studio_inspect_widget (BobguiStudioManager *self, BobguiWidget *widget);
void bobgui_studio_enable_live_edit (BobguiStudioManager *self, gboolean enable);

/* Resource Tracking (Memory, Textures, Shaders) */
void bobgui_studio_show_resource_usage (BobguiStudioManager *self);

G_END_DECLS

#endif /* BOBGUI_STUDIO_H */
