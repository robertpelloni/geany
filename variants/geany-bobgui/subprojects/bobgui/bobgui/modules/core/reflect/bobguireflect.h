/* bobgui/modules/core/reflect/bobguireflect.h */
#ifndef BOBGUI_REFLECT_H
#define BOBGUI_REFLECT_H

#include <glib-object.h>

G_BEGIN_DECLS

/* Deep Reflection & Serialization Engine (Better than Qt MOC) */
#define BOBGUI_TYPE_REFLECT_CONTEXT (bobgui_reflect_context_get_type ())
G_DECLARE_FINAL_TYPE (BobguiReflectContext, bobgui_reflect_context, BOBGUI, REFLECT_CONTEXT, GObject)

BobguiReflectContext * bobgui_reflect_context_get_default (void);

/* Automatic Serialization to JSON (Superior Parity) */
char * bobgui_reflect_to_json (GObject *object);
void   bobgui_reflect_from_json (GObject *object, const char *json);

/* High-Performance Binary Serialization (Better than standard toolkits) */
GBytes * bobgui_reflect_to_binary (GObject *object);
void     bobgui_reflect_from_binary (GObject *object, GBytes *data);

/* Dynamic Property Inspection (Better than manual GParamSpec loops) */
GList * bobgui_reflect_list_properties (GType type);

G_END_DECLS

#endif /* BOBGUI_REFLECT_H */
