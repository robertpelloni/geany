#include "bobguipackage.h"

G_DEFINE_TYPE (BobguiPackageManager, bobgui_package_manager, G_TYPE_OBJECT)

static void
bobgui_package_manager_init (BobguiPackageManager *self)
{
}

static void
bobgui_package_manager_class_init (BobguiPackageManagerClass *klass)
{
}

BobguiPackageManager *
bobgui_package_manager_get_default (void)
{
  static BobguiPackageManager *manager = NULL;
  if (!manager)
    manager = g_object_new (BOBGUI_TYPE_PACKAGE_MANAGER, NULL);
  return manager;
}
