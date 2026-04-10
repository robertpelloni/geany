/* bobgui/modules/media/audio/bobguipluginhost.h */
#ifndef BOBGUI_PLUGIN_HOST_H
#define BOBGUI_PLUGIN_HOST_H

#include <bobgui/bobgui.h>
#include <bobgui/modules/media/audio/bobguiaudio.h>

G_BEGIN_DECLS

/* Universal Plugin Host (Better than JUCE Hosting) */
#define BOBGUI_TYPE_PLUGIN_INSTANCE (bobgui_plugin_instance_get_type ())
G_DECLARE_DERIVABLE_TYPE (BobguiPluginInstance, bobgui_plugin_instance, BOBGUI, PLUGIN_INSTANCE, GObject)

typedef struct _BobguiPluginDescription BobguiPluginDescription;

/* Host Manager API */
#define BOBGUI_TYPE_PLUGIN_HOST_MANAGER (bobgui_plugin_host_manager_get_type ())
G_DECLARE_FINAL_TYPE (BobguiPluginHostManager, bobgui_plugin_host_manager, BOBGUI, PLUGIN_HOST_MANAGER, GObject)

BobguiPluginHostManager * bobgui_plugin_host_manager_new (void);

/* Scan and Load API (VST3, AU, CLAP, LV2) */
void bobgui_plugin_host_manager_scan (BobguiPluginHostManager *self, const char *path);
void bobgui_plugin_host_manager_create_instance (BobguiPluginHostManager *self, 
                                                BobguiPluginDescription *desc, 
                                                GAsyncReadyCallback callback);

/* Sandboxed Execution (Superior Parity: Prevents Plugin Crashes) */
void bobgui_plugin_instance_set_sandbox_mode (BobguiPluginInstance *self, gboolean enabled);

/* UI Integration (Bridge Plugin Editor to BobguiWidget) */
BobguiWidget * bobgui_plugin_instance_get_editor (BobguiPluginInstance *self);

G_END_DECLS

#endif /* BOBGUI_PLUGIN_HOST_H */
