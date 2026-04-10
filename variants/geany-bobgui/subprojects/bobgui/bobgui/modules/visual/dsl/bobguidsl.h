#ifndef BOBGUI_DSL_H
#define BOBGUI_DSL_H

#include <bobgui/bobgui.h>

G_BEGIN_DECLS

BobguiWidget * bobgui_dsl_box (BobguiOrientation orientation, ...);
BobguiWidget * bobgui_dsl_button (const char *label, GCallback callback);
BobguiWidget * bobgui_dsl_label (const char *text);
BobguiWidget * bobgui_dsl_set_padding (BobguiWidget *widget, int padding);

G_END_DECLS

#endif
