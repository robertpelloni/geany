/* bobgui/modules/core/meta/bobguimeta.h */
#ifndef BOBGUI_META_H
#define BOBGUI_META_H

#include <glib-object.h>
#include <bobgui/bobgui.h>

G_BEGIN_DECLS

/* Meta-Programming Engine (Better than Qt MOC / Java Annotations) */
#define BOBGUI_TYPE_META_ORCHESTRATOR (bobgui_meta_orchestrator_get_type ())
G_DECLARE_FINAL_TYPE (BobguiMetaOrchestrator, bobgui_meta_orchestrator, BOBGUI, META_ORCHESTRATOR, GObject)

BobguiMetaOrchestrator * bobgui_meta_orchestrator_get_default (void);

/* Automated UI Generation (Better than manual Form Builders) */
BobguiWidget * bobgui_meta_create_form_for_type (GType object_type);

/* Automated Schema Mapping (Superior Parity with ORMs) */
void bobgui_meta_map_to_database (GType object_type, const char *table_name);

/* Automated Validation (Driven by BobguiBrain) */
void bobgui_meta_set_validation_rules (GType object_type, const char *json_rules);

/* Dynamic Type Extension (Unmatched: Add properties at runtime) */
void bobgui_meta_add_dynamic_property (GType object_type, 
                                      const char *name, 
                                      GType value_type);

G_END_DECLS

#endif /* BOBGUI_META_H */
