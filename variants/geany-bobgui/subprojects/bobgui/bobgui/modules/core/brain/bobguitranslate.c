#include "bobguitranslate.h"
G_DEFINE_TYPE (BobguiNeuralTranslator, bobgui_neural_translator, G_TYPE_OBJECT)
static void bobgui_neural_translator_init (BobguiNeuralTranslator *s) {}
static void bobgui_neural_translator_class_init (BobguiNeuralTranslatorClass *k) {}
BobguiNeuralTranslator * bobgui_neural_translator_get_default (void) {
    static BobguiNeuralTranslator *t = NULL;
    if (!t) t = g_object_new (BOBGUI_TYPE_NEURAL_TRANSLATOR, NULL);
    return t;
}
