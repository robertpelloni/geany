/* bobgui/modules/system/os/bobguios.h */
#ifndef BOBGUI_OS_H
#define BOBGUI_OS_H

#include <bobgui/bobgui.h>

G_BEGIN_DECLS

/* Embedded OS Environment (Better than simple MDI windows) */
#define BOBGUI_TYPE_VIRTUAL_OS (bobgui_virtual_os_get_type ())
G_DECLARE_FINAL_TYPE (BobguiVirtualOs, bobgui_virtual_os, BOBGUI, VIRTUAL_OS, BobguiWidget)

BobguiVirtualOs * bobgui_virtual_os_new (void);

/* Virtual Window Management (Superior Parity with Desktop OS) */
void bobgui_virtual_os_launch_app (BobguiVirtualOs *self, BobguiWidget *app_root, const char *title);
void bobgui_virtual_os_set_workspace (BobguiVirtualOs *self, int workspace_index);

/* Task Orchestration (Process-like management of UI roots) */
void bobgui_virtual_os_kill_app (BobguiVirtualOs *self, BobguiWidget *app_root);
GList * bobgui_virtual_os_list_running_apps (BobguiVirtualOs *self);

/* System Bar and Dock Integration (Built-in) */
void bobgui_virtual_os_show_taskbar (BobguiVirtualOs *self, gboolean show);

G_END_DECLS

#endif /* BOBGUI_OS_H */
