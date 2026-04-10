/* bobgui/modules/core/brain/bobguitranslate.h */
#ifndef BOBGUI_TRANSLATE_H
#define BOBGUI_TRANSLATE_H

#include <bobgui/bobgui.h>
#include <bobgui/modules/core/brain/bobguibrain.h>

G_BEGIN_DECLS

/* AI-Driven Neural Translation (Better than tr() / gettext) */
#define BOBGUI_TYPE_NEURAL_TRANSLATOR (bobgui_neural_translator_get_type ())
G_DECLARE_FINAL_TYPE (BobguiNeuralTranslator, bobgui_neural_translator, BOBGUI, NEURAL_TRANSLATOR, GObject)

BobguiNeuralTranslator * bobgui_neural_translator_get_default (void);

/* Real-time Dynamic Translation (Unmatched Parity) */
void bobgui_neural_translator_translate_async (BobguiNeuralTranslator *self, 
                                               const char *source_text, 
                                               const char *target_lang, 
                                               GAsyncReadyCallback callback);

/* Global UI Translation (Translaters the entire widget tree on-the-fly) */
void bobgui_neural_translator_translate_widget_tree (BobguiNeuralTranslator *self, 
                                                    BobguiWidget *root, 
                                                    const char *target_lang);

/* Content preservation (AI understands UI context like labels vs buttons) */
void bobgui_neural_translator_set_context_hint (BobguiNeuralTranslator *self, 
                                               const char *context);

G_END_DECLS

#endif /* BOBGUI_TRANSLATE_H */
