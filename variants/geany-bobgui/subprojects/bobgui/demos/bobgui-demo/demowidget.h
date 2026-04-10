#pragma once

#include <bobgui/bobgui.h>

#define DEMO_TYPE_WIDGET (demo_widget_get_type ())
G_DECLARE_FINAL_TYPE (DemoWidget, demo_widget, DEMO, WIDGET, BobguiWidget)

BobguiWidget * demo_widget_new       (void);

void        demo_widget_add_child (DemoWidget *self,
                                   BobguiWidget  *child);
