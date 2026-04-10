#ifndef BOBGUI_DESIGN_H
#define BOBGUI_DESIGN_H

#include <glib-object.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_DESIGN_SYSTEM (bobgui_design_system_get_type ())
G_DECLARE_FINAL_TYPE (BobguiDesignSystem, bobgui_design_system, BOBGUI, DESIGN_SYSTEM, GObject)

BobguiDesignSystem * bobgui_design_system_get_default (void);

G_END_DECLS

#endif
