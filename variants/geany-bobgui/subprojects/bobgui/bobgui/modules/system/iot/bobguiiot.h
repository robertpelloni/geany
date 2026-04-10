#ifndef BOBGUI_IOT_H
#define BOBGUI_IOT_H

#include <glib-object.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_IOT_MANAGER (bobgui_iot_manager_get_type ())
G_DECLARE_FINAL_TYPE (BobguiIotManager, bobgui_iot_manager, BOBGUI, IOT_MANAGER, GObject)

BobguiIotManager * bobgui_iot_manager_get_default (void);

G_END_DECLS

#endif
