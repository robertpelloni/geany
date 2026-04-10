#include <bobgui/bobgui.h>

#include "exampleapp.h"
#include "exampleappwin.h"

struct _ExampleAppWindow
{
  BobguiApplicationWindow parent;

  BobguiWidget *stack;
  BobguiWidget *gears;
};

G_DEFINE_TYPE (ExampleAppWindow, example_app_window, BOBGUI_TYPE_APPLICATION_WINDOW)

static void
example_app_window_init (ExampleAppWindow *win)
{
  BobguiBuilder *builder;
  GMenuModel *menu;

  bobgui_widget_init_template (BOBGUI_WIDGET (win));

  builder = bobgui_builder_new_from_resource ("/org/bobgui/exampleapp/gears-menu.ui");
  menu = G_MENU_MODEL (bobgui_builder_get_object (builder, "menu"));
  bobgui_menu_button_set_menu_model (BOBGUI_MENU_BUTTON (win->gears), menu);
  g_object_unref (builder);
}

static void
example_app_window_class_init (ExampleAppWindowClass *class)
{
  bobgui_widget_class_set_template_from_resource (BOBGUI_WIDGET_CLASS (class),
                                               "/org/bobgui/exampleapp/window.ui");
  bobgui_widget_class_bind_template_child (BOBGUI_WIDGET_CLASS (class), ExampleAppWindow, stack);
  bobgui_widget_class_bind_template_child (BOBGUI_WIDGET_CLASS (class), ExampleAppWindow, gears);
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
  char *basename;
  BobguiWidget *scrolled, *view;
  char *contents;
  gsize length;

  basename = g_file_get_basename (file);

  scrolled = bobgui_scrolled_window_new ();
  bobgui_widget_set_hexpand (scrolled, TRUE);
  bobgui_widget_set_vexpand (scrolled, TRUE);
  view = bobgui_text_view_new ();
  bobgui_text_view_set_editable (BOBGUI_TEXT_VIEW (view), FALSE);
  bobgui_text_view_set_cursor_visible (BOBGUI_TEXT_VIEW (view), FALSE);
  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (scrolled), view);
  bobgui_stack_add_titled (BOBGUI_STACK (win->stack), scrolled, basename, basename);

  if (g_file_load_contents (file, NULL, &contents, &length, NULL, NULL))
    {
      BobguiTextBuffer *buffer;

      buffer = bobgui_text_view_get_buffer (BOBGUI_TEXT_VIEW (view));
      bobgui_text_buffer_set_text (buffer, contents, length);
      g_free (contents);
    }

  g_free (basename);
}
