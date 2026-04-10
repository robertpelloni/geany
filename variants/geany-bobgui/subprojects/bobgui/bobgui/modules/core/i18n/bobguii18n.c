#include "bobguii18n.h"
G_DEFINE_TYPE (BobguiI18nEngine, bobgui_i18n_engine, G_TYPE_OBJECT)
static void bobgui_i18n_engine_init (BobguiI18nEngine *self) {}
static void bobgui_i18n_engine_class_init (BobguiI18nEngineClass *klass) {}
BobguiI18nEngine * bobgui_i18n_engine_get_default (void) {
    static BobguiI18nEngine *eng = NULL;
    if (!eng) eng = g_object_new (BOBGUI_TYPE_I18N_ENGINE, NULL);
    return eng;
}
