#ifndef BOBGUI_COMPUTE_H
#define BOBGUI_COMPUTE_H

#include <glib-object.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_COMPUTE_CONTEXT (bobgui_compute_context_get_type ())
G_DECLARE_FINAL_TYPE (BobguiComputeContext, bobgui_compute_context, BOBGUI, COMPUTE_CONTEXT, GObject)

BobguiComputeContext * bobgui_compute_context_new (const char *backend);

G_END_DECLS

#endif
