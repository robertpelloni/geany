#include "bobguibrain.h"
G_DEFINE_TYPE (BobguiBrainModel, bobgui_brain_model, G_TYPE_OBJECT)
static void bobgui_brain_model_init (BobguiBrainModel *self) {}
static void bobgui_brain_model_class_init (BobguiBrainModelClass *klass) {}
BobguiBrainModel * bobgui_brain_model_load (const char *p) { return g_object_new (BOBGUI_TYPE_BRAIN_MODEL, NULL); }
