#pragma once

#include <bobgui/bobgui.h>


#define EXAMPLE_APP_TYPE (example_app_get_type ())
G_DECLARE_FINAL_TYPE (ExampleApp, example_app, EXAMPLE, APP, BobguiApplication)


ExampleApp     *example_app_new         (void);
