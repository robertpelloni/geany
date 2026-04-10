#ifndef BOBGUI_BRAIN_H
#define BOBGUI_BRAIN_H

#include <glib-object.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_BRAIN_MODEL (bobgui_brain_model_get_type ())
G_DECLARE_FINAL_TYPE (BobguiBrainModel, bobgui_brain_model, BOBGUI, BRAIN_MODEL, GObject)

BobguiBrainModel * bobgui_brain_model_load (const char *path);

G_END_DECLS

#endif
