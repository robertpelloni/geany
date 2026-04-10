#pragma once

#include <bobgui/bobgui.h>
#include "exampleapp.h"


#define EXAMPLE_APP_WINDOW_TYPE (example_app_window_get_type ())
G_DECLARE_FINAL_TYPE (ExampleAppWindow, example_app_window, EXAMPLE, APP_WINDOW, BobguiApplicationWindow)


ExampleAppWindow       *example_app_window_new          (ExampleApp *app);
void                    example_app_window_open         (ExampleAppWindow *win,
                                                         GFile            *file);
