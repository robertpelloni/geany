#include <bobgui/bobgui.h>

#include "exampleapp.h"
#include "exampleappwin.h"

struct _ExampleAppWindow
{
  BobguiApplicationWindow parent;

  GSettings *settings;
  BobguiWidget *stack;
  BobguiWidget *gears;
  BobguiWidget *search;
  BobguiWidget *searchbar;
};

G_DEFINE_TYPE (ExampleAppWindow, example_app_window, BOBGUI_TYPE_APPLICATION_WINDOW)

static void
search_text_changed (BobguiEntry         *entry,
                     ExampleAppWindow *win)
{
  const char *text;
  BobguiWidget *tab;
  BobguiWidget *view;
  BobguiTextBuffer *buffer;
  BobguiTextIter start, match_start, match_end;

  text = bobgui_editable_get_text (BOBGUI_EDITABLE (entry));

  if (text[0] == '\0')
    return;

  tab = bobgui_stack_get_visible_child (BOBGUI_STACK (win->stack));
  view = bobgui_scrolled_window_get_child (BOBGUI_SCROLLED_WINDOW (tab));
  buffer = bobgui_text_view_get_buffer (BOBGUI_TEXT_VIEW (view));

  /* Very simple-minded search implementation */
  bobgui_text_buffer_get_start_iter (buffer, &start);
  if (bobgui_text_iter_forward_search (&start, text, BOBGUI_TEXT_SEARCH_CASE_INSENSITIVE,
                                    &match_start, &match_end, NULL))
    {
      bobgui_text_buffer_select_range (buffer, &match_start, &match_end);
      bobgui_text_view_scroll_to_iter (BOBGUI_TEXT_VIEW (view), &match_start,
                                    0.0, FALSE, 0.0, 0.0);
    }
}

static void
visible_child_changed (GObject          *stack,
                       GParamSpec       *pspec,
                       ExampleAppWindow *win)
{
  if (bobgui_widget_in_destruction (BOBGUI_WIDGET (stack)))
    return;

  bobgui_search_bar_set_search_mode (BOBGUI_SEARCH_BAR (win->searchbar), FALSE);
}

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

  win->settings = g_settings_new ("org.bobgui.exampleapp");

  g_settings_bind (win->settings, "transition",
                   win->stack, "transition-type",
                   G_SETTINGS_BIND_DEFAULT);

  g_object_bind_property (win->search, "active",
                          win->searchbar, "search-mode-enabled",
                          G_BINDING_BIDIRECTIONAL);
}

static void
example_app_window_dispose (GObject *object)
{
  ExampleAppWindow *win;

  win = EXAMPLE_APP_WINDOW (object);

  g_clear_object (&win->settings);

  G_OBJECT_CLASS (example_app_window_parent_class)->dispose (object);
}

static void
example_app_window_class_init (ExampleAppWindowClass *class)
{
  G_OBJECT_CLASS (class)->dispose = example_app_window_dispose;

  bobgui_widget_class_set_template_from_resource (BOBGUI_WIDGET_CLASS (class),
                                               "/org/bobgui/exampleapp/window.ui");

  bobgui_widget_class_bind_template_child (BOBGUI_WIDGET_CLASS (class), ExampleAppWindow, stack);
  bobgui_widget_class_bind_template_child (BOBGUI_WIDGET_CLASS (class), ExampleAppWindow, gears);
  bobgui_widget_class_bind_template_child (BOBGUI_WIDGET_CLASS (class), ExampleAppWindow, search);
  bobgui_widget_class_bind_template_child (BOBGUI_WIDGET_CLASS (class), ExampleAppWindow, searchbar);

  bobgui_widget_class_bind_template_callback (BOBGUI_WIDGET_CLASS (class), search_text_changed);
  bobgui_widget_class_bind_template_callback (BOBGUI_WIDGET_CLASS (class), visible_child_changed);
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
  BobguiTextBuffer *buffer;
  BobguiTextTag *tag;
  BobguiTextIter start_iter, end_iter;

  basename = g_file_get_basename (file);

  scrolled = bobgui_scrolled_window_new ();
  bobgui_widget_set_hexpand (scrolled, TRUE);
  bobgui_widget_set_vexpand (scrolled, TRUE);
  view = bobgui_text_view_new ();
  bobgui_text_view_set_editable (BOBGUI_TEXT_VIEW (view), FALSE);
  bobgui_text_view_set_cursor_visible (BOBGUI_TEXT_VIEW (view), FALSE);
  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (scrolled), view);
  bobgui_stack_add_titled (BOBGUI_STACK (win->stack), scrolled, basename, basename);

  buffer = bobgui_text_view_get_buffer (BOBGUI_TEXT_VIEW (view));

  if (g_file_load_contents (file, NULL, &contents, &length, NULL, NULL))
    {
      bobgui_text_buffer_set_text (buffer, contents, length);
      g_free (contents);
    }

  tag = bobgui_text_buffer_create_tag (buffer, NULL, NULL);
  g_settings_bind (win->settings, "font",
                   tag, "font",
                   G_SETTINGS_BIND_DEFAULT);

  bobgui_text_buffer_get_start_iter (buffer, &start_iter);
  bobgui_text_buffer_get_end_iter (buffer, &end_iter);
  bobgui_text_buffer_apply_tag (buffer, tag, &start_iter, &end_iter);

  g_free (basename);

  bobgui_widget_set_sensitive (win->search, TRUE);
}
