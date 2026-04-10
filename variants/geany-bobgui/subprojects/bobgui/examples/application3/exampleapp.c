#include <bobgui/bobgui.h>

#include "exampleapp.h"
#include "exampleappwin.h"

struct _ExampleApp
{
  BobguiApplication parent;
};

G_DEFINE_TYPE(ExampleApp, example_app, BOBGUI_TYPE_APPLICATION);

static void
example_app_init (ExampleApp *app)
{
}

static void
example_app_activate (GApplication *app)
{
  ExampleAppWindow *win;

  win = example_app_window_new (EXAMPLE_APP (app));
  bobgui_window_present (BOBGUI_WINDOW (win));
}

static void
example_app_open (GApplication  *app,
                  GFile        **files,
                  int            n_files,
                  const char    *hint)
{
  GList *windows;
  ExampleAppWindow *win;
  int i;

  windows = bobgui_application_get_windows (BOBGUI_APPLICATION (app));
  if (windows)
    win = EXAMPLE_APP_WINDOW (windows->data);
  else
    win = example_app_window_new (EXAMPLE_APP (app));

  for (i = 0; i < n_files; i++)
    example_app_window_open (win, files[i]);

  bobgui_window_present (BOBGUI_WINDOW (win));
}

static void
example_app_class_init (ExampleAppClass *class)
{
  G_APPLICATION_CLASS (class)->activate = example_app_activate;
  G_APPLICATION_CLASS (class)->open = example_app_open;
}

ExampleApp *
example_app_new (void)
{
  return g_object_new (EXAMPLE_APP_TYPE,
                       "application-id", "org.bobgui.exampleapp",
                       "flags", G_APPLICATION_HANDLES_OPEN,
                       NULL);
}
