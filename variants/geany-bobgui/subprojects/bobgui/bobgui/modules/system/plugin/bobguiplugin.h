#ifndef BOBGUI_PLUGIN_H
#define BOBGUI_PLUGIN_H

#include <glib-object.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_PLUGIN_MANAGER (bobgui_plugin_manager_get_type ())
G_DECLARE_FINAL_TYPE (BobguiPluginManager, bobgui_plugin_manager, BOBGUI, PLUGIN_MANAGER, GObject)

BobguiPluginManager * bobgui_plugin_manager_new (const char *search_path);

G_END_DECLS

#endif
