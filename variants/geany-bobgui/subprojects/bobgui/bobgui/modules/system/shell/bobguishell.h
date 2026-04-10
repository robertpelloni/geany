#ifndef BOBGUI_SHELL_H
#define BOBGUI_SHELL_H

#include <bobgui/bobgui.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_SHELL_MANAGER (bobgui_shell_manager_get_type ())
G_DECLARE_FINAL_TYPE (BobguiShellManager, bobgui_shell_manager, BOBGUI, SHELL_MANAGER, GObject)

BobguiShellManager * bobgui_shell_manager_get_default (void);
void bobgui_shell_set_progress (BobguiShellManager *self, double fraction);

G_END_DECLS

#endif
