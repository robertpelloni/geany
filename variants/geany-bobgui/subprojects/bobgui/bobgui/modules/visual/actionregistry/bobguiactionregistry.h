#ifndef BOBGUI_ACTION_REGISTRY_H
#define BOBGUI_ACTION_REGISTRY_H

#include <gio/gio.h>

G_BEGIN_DECLS

typedef void (*BobguiActionRegistryFunc) (const char *action_id,
                                          gpointer    user_data);
typedef void (*BobguiActionRegistryVisitFunc) (const char *action_id,
                                               const char *title,
                                               const char *subtitle,
                                               const char *section,
                                               const char *category,
                                               const char *shortcut,
                                               const char *icon_name,
                                               const char *tags,
                                               gboolean    checkable,
                                               gboolean    checked,
                                               gpointer    user_data);

typedef struct _BobguiCommandPalette BobguiCommandPalette;

#define BOBGUI_TYPE_ACTION_REGISTRY (bobgui_action_registry_get_type ())
G_DECLARE_FINAL_TYPE (BobguiActionRegistry, bobgui_action_registry, BOBGUI, ACTION_REGISTRY, GObject)

BobguiActionRegistry * bobgui_action_registry_new              (void);
void                   bobgui_action_registry_add_tagged       (BobguiActionRegistry     *self,
                                                               const char               *action_id,
                                                               const char               *title,
                                                               const char               *subtitle,
                                                               const char               *section,
                                                               const char               *category,
                                                               const char               *shortcut,
                                                               const char               *icon_name,
                                                               const char               *tags,
                                                               BobguiActionRegistryFunc  callback,
                                                               gpointer                  user_data);
void                   bobgui_action_registry_add_sectioned    (BobguiActionRegistry     *self,
                                                               const char               *action_id,
                                                               const char               *title,
                                                               const char               *subtitle,
                                                               const char               *section,
                                                               const char               *category,
                                                               const char               *shortcut,
                                                               const char               *icon_name,
                                                               BobguiActionRegistryFunc  callback,
                                                               gpointer                  user_data);
void                   bobgui_action_registry_add_detailed     (BobguiActionRegistry     *self,
                                                               const char               *action_id,
                                                               const char               *title,
                                                               const char               *subtitle,
                                                               const char               *category,
                                                               const char               *shortcut,
                                                               const char               *icon_name,
                                                               BobguiActionRegistryFunc  callback,
                                                               gpointer                  user_data);
void                   bobgui_action_registry_add              (BobguiActionRegistry     *self,
                                                               const char               *action_id,
                                                               const char               *title,
                                                               const char               *subtitle,
                                                               BobguiActionRegistryFunc  callback,
                                                               gpointer                  user_data);
void                   bobgui_action_registry_add_toggle       (BobguiActionRegistry     *self,
                                                               const char               *action_id,
                                                               const char               *title,
                                                               const char               *subtitle,
                                                               const char               *section,
                                                               const char               *category,
                                                               const char               *shortcut,
                                                               const char               *icon_name,
                                                               gboolean                  checked,
                                                               BobguiActionRegistryFunc  callback,
                                                               gpointer                  user_data);
void                   bobgui_action_registry_set_checked      (BobguiActionRegistry     *self,
                                                               const char               *action_id,
                                                               gboolean                  checked);
gboolean               bobgui_action_registry_get_checked      (BobguiActionRegistry     *self,
                                                               const char               *action_id);
void                   bobgui_action_registry_activate         (BobguiActionRegistry     *self,
                                                               const char               *action_id);
GMenuModel *           bobgui_action_registry_create_menu_model(BobguiActionRegistry     *self);
void                   bobgui_action_registry_visit            (BobguiActionRegistry     *self,
                                                               BobguiActionRegistryVisitFunc func,
                                                               gpointer                  user_data);
void                   bobgui_action_registry_populate_palette (BobguiActionRegistry     *self,
                                                               BobguiCommandPalette     *palette);

G_END_DECLS

#endif
