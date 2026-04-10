#ifndef BOBGUI_SCRIPT_H
#define BOBGUI_SCRIPT_H

#include <glib-object.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_SCRIPT_CONTEXT (bobgui_script_context_get_type ())
G_DECLARE_FINAL_TYPE (BobguiScriptContext, bobgui_script_context, BOBGUI, SCRIPT_CONTEXT, GObject)

BobguiScriptContext * bobgui_script_context_new (const char *engine_type);

G_END_DECLS

#endif
