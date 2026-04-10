/* bobgui/modules/i18n/bobguii18n.h */
#ifndef BOBGUI_I18N_H
#define BOBGUI_I18N_H

#include <glib-object.h>
#include <bobgui/bobgui.h>

G_BEGIN_DECLS

/* Reactive Translation Engine (Better than tr() / gettext) */
#define BOBGUI_TYPE_I18N_ENGINE (bobgui_i18n_engine_get_type ())
G_DECLARE_FINAL_TYPE (BobguiI18nEngine, bobgui_i18n_engine, BOBGUI, I18N_ENGINE, GObject)

BobguiI18nEngine * bobgui_i18n_engine_get_default (void);

/* Dynamic Language Management (Unmatched Parity) */
void bobgui_i18n_set_language (BobguiI18nEngine *self, const char *lang_code);
const char * bobgui_i18n_get_current_language (BobguiI18nEngine *self);

/* Reactive Translation (Updates UI automatically on language change) */
void bobgui_label_bind_i18n (BobguiLabel *label, const char *msgid);
void bobgui_button_bind_i18n (BobguiButton *button, const char *msgid);

/* Direct GObject Property Binding for Translations */
void bobgui_widget_bind_i18n_property (BobguiWidget *widget, 
                                      const char *property, 
                                      const char *msgid);

/* Catalog Loading (Supports JSON/PO/MO) */
void bobgui_i18n_load_catalog (BobguiI18nEngine *self, const char *path);

G_END_DECLS

#endif /* BOBGUI_I18N_H */
