#include <bobgui/bobgui.h>

#include "exampleapp.h"
#include "exampleappwin.h"

struct _ExampleAppWindow
{
  BobguiApplicationWindow parent;
};

G_DEFINE_TYPE(ExampleAppWindow, example_app_window, BOBGUI_TYPE_APPLICATION_WINDOW);

static void
example_app_window_init (ExampleAppWindow *win)
{
  bobgui_widget_init_template (BOBGUI_WIDGET (win));
}

static void
example_app_window_class_init (ExampleAppWindowClass *class)
{
  bobgui_widget_class_set_template_from_resource (BOBGUI_WIDGET_CLASS (class),
                                               "/org/bobgui/exampleapp/window.ui");
}

ExampleAppWindow *
example_app_window_new (ExampleApp *app)
{
  return g_object_new (EXAMPLE_APP_WINDOW_TYPE, "application", app, NULL);
}

void
example_app_window_open (ExampleAppWindow *win,
                         GFile            *file)
{
}
