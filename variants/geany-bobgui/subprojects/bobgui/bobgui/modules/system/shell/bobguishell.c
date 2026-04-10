#include "bobguishell.h"

G_DEFINE_TYPE (BobguiShellManager, bobgui_shell_manager, G_TYPE_OBJECT)

static void
bobgui_shell_manager_init (BobguiShellManager *self)
{
}

static void
bobgui_shell_manager_class_init (BobguiShellManagerClass *klass)
{
}

BobguiShellManager *
bobgui_shell_manager_get_default (void)
{
  static BobguiShellManager *manager = NULL;
  if (!manager)
    manager = g_object_new (BOBGUI_TYPE_SHELL_MANAGER, NULL);
  return manager;
}

void
bobgui_shell_set_progress (BobguiShellManager *self,
                           double              fraction)
{
  (void) self;
  (void) fraction;
}
