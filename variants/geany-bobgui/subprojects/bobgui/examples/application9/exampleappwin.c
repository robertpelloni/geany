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
  BobguiWidget *searchentry;
  BobguiWidget *sidebar;
  BobguiWidget *words;
  BobguiWidget *lines;
  BobguiWidget *lines_label;
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
find_word (BobguiButton        *button,
           ExampleAppWindow *win)
{
  const char *word;

  word = bobgui_button_get_label (button);
  bobgui_editable_set_text (BOBGUI_EDITABLE (win->searchentry), word);
}

static void
update_words (ExampleAppWindow *win)
{
  GHashTable *strings;
  GHashTableIter iter;
  BobguiWidget *tab, *view, *row;
  BobguiTextBuffer *buffer;
  BobguiTextIter start, end;
  char *word, *key;
  BobguiWidget *child;

  tab = bobgui_stack_get_visible_child (BOBGUI_STACK (win->stack));

  if (tab == NULL)
    return;

  view = bobgui_scrolled_window_get_child (BOBGUI_SCROLLED_WINDOW (tab));
  buffer = bobgui_text_view_get_buffer (BOBGUI_TEXT_VIEW (view));

  strings = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);

  bobgui_text_buffer_get_start_iter (buffer, &start);
  while (!bobgui_text_iter_is_end (&start))
    {
      while (!bobgui_text_iter_starts_word (&start))
        {
          if (!bobgui_text_iter_forward_char (&start))
            goto done;
        }
      end = start;
      if (!bobgui_text_iter_forward_word_end (&end))
        goto done;
      word = bobgui_text_buffer_get_text (buffer, &start, &end, FALSE);
      g_hash_table_add (strings, g_utf8_strdown (word, -1));
      g_free (word);
      start = end;
    }

done:
  while ((child = bobgui_widget_get_first_child (win->words)))
    bobgui_list_box_remove (BOBGUI_LIST_BOX (win->words), child);

  g_hash_table_iter_init (&iter, strings);
  while (g_hash_table_iter_next (&iter, (gpointer *)&key, NULL))
    {
      row = bobgui_button_new_with_label (key);
      g_signal_connect (row, "clicked",
                        G_CALLBACK (find_word), win);
      bobgui_list_box_insert (BOBGUI_LIST_BOX (win->words), row, -1);
    }

  g_hash_table_unref (strings);
}

static void
update_lines (ExampleAppWindow *win)
{
  BobguiWidget *tab, *view;
  BobguiTextBuffer *buffer;
  int count;
  char *lines;

  tab = bobgui_stack_get_visible_child (BOBGUI_STACK (win->stack));

  if (tab == NULL)
    return;

  view = bobgui_scrolled_window_get_child (BOBGUI_SCROLLED_WINDOW (tab));
  buffer = bobgui_text_view_get_buffer (BOBGUI_TEXT_VIEW (view));

  count = bobgui_text_buffer_get_line_count (buffer);
  lines = g_strdup_printf ("%d", count);
  bobgui_label_set_text (BOBGUI_LABEL (win->lines), lines);
  g_free (lines);
}

static void
visible_child_changed (GObject          *stack,
                       GParamSpec       *pspec,
                       ExampleAppWindow *win)
{
  if (bobgui_widget_in_destruction (BOBGUI_WIDGET (stack)))
    return;

  bobgui_search_bar_set_search_mode (BOBGUI_SEARCH_BAR (win->searchbar), FALSE);
  update_words (win);
  update_lines (win);
}

static void
words_changed (GObject          *sidebar,
               GParamSpec       *pspec,
               ExampleAppWindow *win)
{
  update_words (win);
}

static void
example_app_window_init (ExampleAppWindow *win)
{
  BobguiBuilder *builder;
  GMenuModel *menu;
  GAction *action;

  bobgui_widget_init_template (BOBGUI_WIDGET (win));

  builder = bobgui_builder_new_from_resource ("/org/bobgui/exampleapp/gears-menu.ui");
  menu = G_MENU_MODEL (bobgui_builder_get_object (builder, "menu"));
  bobgui_menu_button_set_menu_model (BOBGUI_MENU_BUTTON (win->gears), menu);
  g_object_unref (builder);

  win->settings = g_settings_new ("org.bobgui.exampleapp");

  g_settings_bind (win->settings, "transition",
                   win->stack, "transition-type",
                   G_SETTINGS_BIND_DEFAULT);

  g_settings_bind (win->settings, "show-words",
                   win->sidebar, "reveal-child",
                   G_SETTINGS_BIND_DEFAULT);

  g_object_bind_property (win->search, "active",
                          win->searchbar, "search-mode-enabled",
                          G_BINDING_BIDIRECTIONAL);

  g_signal_connect (win->sidebar, "notify::reveal-child",
                    G_CALLBACK (words_changed), win);

  action = g_settings_create_action (win->settings, "show-words");
  g_action_map_add_action (G_ACTION_MAP (win), action);
  g_object_unref (action);

  action = (GAction*) g_property_action_new ("show-lines", win->lines, "visible");
  g_action_map_add_action (G_ACTION_MAP (win), action);
  g_object_unref (action);

  g_object_bind_property (win->lines, "visible",
                          win->lines_label, "visible",
                          G_BINDING_DEFAULT);
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
  bobgui_widget_class_bind_template_child (BOBGUI_WIDGET_CLASS (class), ExampleAppWindow, searchentry);
  bobgui_widget_class_bind_template_child (BOBGUI_WIDGET_CLASS (class), ExampleAppWindow, words);
  bobgui_widget_class_bind_template_child (BOBGUI_WIDGET_CLASS (class), ExampleAppWindow, sidebar);
  bobgui_widget_class_bind_template_child (BOBGUI_WIDGET_CLASS (class), ExampleAppWindow, lines);
  bobgui_widget_class_bind_template_child (BOBGUI_WIDGET_CLASS (class), ExampleAppWindow, lines_label);

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

  update_words (win);
  update_lines (win);
}
