#ifndef BOBGUI_SPATIAL_H
#define BOBGUI_SPATIAL_H

#include <glib-object.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_SPATIAL_CONTEXT (bobgui_spatial_context_get_type ())
G_DECLARE_FINAL_TYPE (BobguiSpatialContext, bobgui_spatial_context, BOBGUI, SPATIAL_CONTEXT, GObject)

BobguiSpatialContext * bobgui_spatial_context_new (const char *runtime_name);

G_END_DECLS

#endif
