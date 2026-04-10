#ifndef BOBGUI_PACKAGE_H
#define BOBGUI_PACKAGE_H

#include <glib-object.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_PACKAGE_MANAGER (bobgui_package_manager_get_type ())
G_DECLARE_FINAL_TYPE (BobguiPackageManager, bobgui_package_manager, BOBGUI, PACKAGE_MANAGER, GObject)

BobguiPackageManager * bobgui_package_manager_get_default (void);

G_END_DECLS

#endif
