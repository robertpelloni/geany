#include "config.h"

#include <bobgui/bobgui.h>

typedef BobguiApplication DemoApplication;
typedef BobguiApplicationClass DemoApplicationClass;

static GType demo_application_get_type (void);
G_DEFINE_TYPE (DemoApplication, demo_application, BOBGUI_TYPE_APPLICATION)

typedef struct {
  BobguiApplicationWindow parent_instance;

  BobguiWidget *message;
  BobguiWidget *infobar;
  BobguiWidget *status;
  BobguiWidget *menubutton;
  GMenuModel *toolmenu;
  BobguiTextBuffer *buffer;
} DemoApplicationWindow;
typedef BobguiApplicationWindowClass DemoApplicationWindowClass;

static GType demo_application_window_get_type (void);
G_DEFINE_TYPE (DemoApplicationWindow, demo_application_window, BOBGUI_TYPE_APPLICATION_WINDOW)

static DemoApplicationWindow *create_demo_window (GApplication *app, const char *contents);

static void
show_action_dialog (GSimpleAction *action)
{
  BobguiAlertDialog *dialog;

  dialog = bobgui_alert_dialog_new ("You activated action: \"%s\"",
                                 g_action_get_name (G_ACTION (action)));
  bobgui_alert_dialog_show (dialog, NULL);
  g_object_unref (dialog);
}

static void
show_action_infobar (GSimpleAction *action,
                     GVariant      *parameter,
                     gpointer       data)
{
  DemoApplicationWindow *window = data;
  char *text;
  const char *name;
  const char *value;

  name = g_action_get_name (G_ACTION (action));
  value = g_variant_get_string (parameter, NULL);

  text = g_strdup_printf ("You activated radio action: \"%s\".\n"
                          "Current value: %s", name, value);
  bobgui_label_set_text (BOBGUI_LABEL (window->message), text);
  bobgui_widget_set_visible (window->infobar, TRUE);
  g_free (text);
}

static void
activate_action (GSimpleAction *action,
                 GVariant      *parameter,
                 gpointer       user_data)
{
  show_action_dialog (action);
}

static void
activate_new (GSimpleAction *action,
              GVariant      *parameter,
              gpointer       user_data)
{
  GApplication *app = user_data;
  DemoApplicationWindow *window;

  window = create_demo_window (app, NULL);

  bobgui_window_present (BOBGUI_WINDOW (window));
}

static void
open_response_cb (GObject *source,
                  GAsyncResult *result,
                  gpointer user_data)
{
  BobguiFileDialog *dialog = BOBGUI_FILE_DIALOG (source);
  GApplication *app = G_APPLICATION (user_data);
  GFile *file;
  GError *error = NULL;

  file = bobgui_file_dialog_open_finish (dialog, result, &error);
  if (file)
    {
      char *contents;

      if (g_file_load_contents (file, NULL, &contents, NULL, NULL, &error))
        {
          DemoApplicationWindow *window;

          window = create_demo_window (app, contents);
          bobgui_window_present (BOBGUI_WINDOW (window));
          g_free (contents);
        }
    }

  if (error)
    {
      BobguiAlertDialog *alert;

      alert = bobgui_alert_dialog_new ("Error loading file: \"%s\"", error->message);
      bobgui_alert_dialog_show (alert, NULL);
      g_object_unref (alert);
      g_error_free (error);
    }

  g_object_unref (app);
}


static void
activate_open (GSimpleAction *action,
               GVariant      *parameter,
               gpointer       user_data)
{
  GApplication *app = user_data;
  BobguiFileDialog *dialog;

  dialog = bobgui_file_dialog_new ();
  bobgui_file_dialog_open (dialog, NULL, NULL, open_response_cb, g_object_ref (app));
  g_object_unref (dialog);
}

static void
activate_toggle (GSimpleAction *action,
                 GVariant      *parameter,
                 gpointer       user_data)
{
  GVariant *state;

  show_action_dialog (action);

  state = g_action_get_state (G_ACTION (action));
  g_action_change_state (G_ACTION (action), g_variant_new_boolean (!g_variant_get_boolean (state)));
  g_variant_unref (state);
}

static void
activate_radio (GSimpleAction *action,
                GVariant      *parameter,
                gpointer       user_data)
{
  show_action_infobar (action, parameter, user_data);

  g_action_change_state (G_ACTION (action), parameter);
}

static void
activate_about (GSimpleAction *action,
                GVariant      *parameter,
                gpointer       user_data)
{
  BobguiWidget *window = user_data;

  const char *authors[] = {
    "Peter Mattis",
    "Spencer Kimball",
    "Josh MacDonald",
    "and many more...",
    NULL
  };

  const char *documentors[] = {
    "Owen Taylor",
    "Tony Gale",
    "Matthias Clasen <mclasen@redhat.com>",
    "and many more...",
    NULL
  };

  bobgui_show_about_dialog (BOBGUI_WINDOW (window),
                         "program-name", "BOBGUI Code Demos",
                         "version", g_strdup_printf ("%s,\nRunning against BOBGUI %d.%d.%d",
                                                     PACKAGE_VERSION,
                                                     bobgui_get_major_version (),
                                                     bobgui_get_minor_version (),
                                                     bobgui_get_micro_version ()),
                         "copyright", "(C) 1997-2013 The BOBGUI Team",
                         "license-type", BOBGUI_LICENSE_LGPL_2_1,
                         "website", "http://www.bobgui.org",
                         "comments", "Program to demonstrate BOBGUI functions.",
                         "authors", authors,
                         "documenters", documentors,
                         "logo-icon-name", "org.bobgui.Demo4",
                         "title", "About BOBGUI Code Demos",
                         NULL);
}

static void
activate_quit (GSimpleAction *action,
               GVariant      *parameter,
               gpointer       user_data)
{
  BobguiApplication *app = user_data;
  BobguiWidget *win;
  GList *list, *next;

  list = bobgui_application_get_windows (app);
  while (list)
    {
      win = list->data;
      next = list->next;

      bobgui_window_destroy (BOBGUI_WINDOW (win));

      list = next;
    }
}

static void
delete_messages (gpointer data)
{
  g_list_free_full ((GList *)data, g_free);
}

static void
pop_message (BobguiWidget *status)
{
  GList *messages = (GList *) g_object_steal_data (G_OBJECT (status), "messages");

  if (messages)
    {
      char *message = messages->data;
      messages = g_list_remove (messages, message);

      g_object_set_data_full (G_OBJECT (status), "messages",
                              messages, delete_messages);

      bobgui_label_set_label (BOBGUI_LABEL (status), message);
    }
}

static void
push_message (BobguiWidget  *status,
              const char *message)
{
  GList *messages = (GList *) g_object_steal_data (G_OBJECT (status), "messages");

  bobgui_label_set_label (BOBGUI_LABEL (status), message);
  messages = g_list_prepend (messages, g_strdup (message));
  g_object_set_data_full (G_OBJECT (status), "messages",
                          messages, delete_messages);
}

static void
update_statusbar (BobguiTextBuffer         *buffer,
                  DemoApplicationWindow *window)
{
  char *msg;
  int row, col;
  int count;
  BobguiTextIter iter;

  /* clear any previous message, underflow is allowed */
  pop_message (window->status);

  count = bobgui_text_buffer_get_char_count (buffer);

  bobgui_text_buffer_get_iter_at_mark (buffer,
                                    &iter,
                                    bobgui_text_buffer_get_insert (buffer));

  row = bobgui_text_iter_get_line (&iter);
  col = bobgui_text_iter_get_line_offset (&iter);

  msg = g_strdup_printf ("Cursor at row %d column %d - %d chars in document",
                         row, col, count);

  push_message (window->status, msg);

  g_free (msg);
}

static void
mark_set_callback (BobguiTextBuffer         *buffer,
                   const BobguiTextIter     *new_location,
                   BobguiTextMark           *mark,
                   DemoApplicationWindow *window)
{
  update_statusbar (buffer, window);
}

static void
change_theme_state (GSimpleAction *action,
                    GVariant      *state,
                    gpointer       user_data)
{
  BobguiSettings *settings = bobgui_settings_get_default ();

  g_object_set (G_OBJECT (settings),
                "bobgui-application-prefer-dark-theme",
                g_variant_get_boolean (state),
                NULL);

  g_simple_action_set_state (action, state);
}

static void
change_radio_state (GSimpleAction *action,
                    GVariant      *state,
                    gpointer       user_data)
{
  g_simple_action_set_state (action, state);
}

static GActionEntry app_entries[] = {
  { "new", activate_new, NULL, NULL, NULL },
  { "open", activate_open, NULL, NULL, NULL },
  { "save", activate_action, NULL, NULL, NULL },
  { "save-as", activate_action, NULL, NULL, NULL },
  { "quit", activate_quit, NULL, NULL, NULL },
  { "dark", activate_toggle, NULL, "false", change_theme_state }
};

static GActionEntry win_entries[] = {
  { "shape", activate_radio, "s", "'oval'", change_radio_state },
  { "bold", activate_toggle, NULL, "false", NULL },
  { "about", activate_about, NULL, NULL, NULL },
  { "file1", activate_action, NULL, NULL, NULL },
  { "logo", activate_action, NULL, NULL, NULL }
};

static void
clicked_cb (BobguiWidget *widget, DemoApplicationWindow *window)
{
  bobgui_widget_set_visible (window->infobar, FALSE);
}

static void
startup (GApplication *app)
{
  BobguiBuilder *builder;

  G_APPLICATION_CLASS (demo_application_parent_class)->startup (app);

  builder = bobgui_builder_new ();
  bobgui_builder_add_from_resource (builder, "/application_demo/menus.ui", NULL);

  bobgui_application_set_menubar (BOBGUI_APPLICATION (app),
                               G_MENU_MODEL (bobgui_builder_get_object (builder, "menubar")));

  g_object_unref (builder);
}

static DemoApplicationWindow *
create_demo_window (GApplication *app,
                    const char   *content)
{
  DemoApplicationWindow *window;

  window = (DemoApplicationWindow *)g_object_new (demo_application_window_get_type (),
                                                  "application", app,
                                                  "show-menubar", TRUE,
                                                  NULL);
  if (content)
    bobgui_text_buffer_set_text (window->buffer, content, -1);

  return window;
}

static void
restore_window (BobguiApplication   *app,
                BobguiRestoreReason  reason,
                GVariant         *state)
{
  create_demo_window (G_APPLICATION (app), NULL);
}

static void
demo_application_init (DemoApplication *app)
{
  GSettings *settings;
  GAction *action;

  settings = g_settings_new ("org.bobgui.Demo4.Application");

  g_action_map_add_action_entries (G_ACTION_MAP (app),
                                   app_entries, G_N_ELEMENTS (app_entries),
                                   app);

  action = g_settings_create_action (settings, "color");

  g_action_map_add_action (G_ACTION_MAP (app), action);

  g_object_unref (settings);
}

static void
demo_application_class_init (DemoApplicationClass *class)
{
  GApplicationClass *app_class = G_APPLICATION_CLASS (class);
  BobguiApplicationClass *bobgui_app_class = BOBGUI_APPLICATION_CLASS (class);

  app_class->startup = startup;
  bobgui_app_class->restore_window = restore_window;
}

static void
demo_application_window_init (DemoApplicationWindow *window)
{
  BobguiWidget *popover;

  bobgui_widget_init_template (BOBGUI_WIDGET (window));

  popover = bobgui_popover_menu_new_from_model (window->toolmenu);
  bobgui_menu_button_set_popover (BOBGUI_MENU_BUTTON (window->menubutton), popover);

  g_action_map_add_action_entries (G_ACTION_MAP (window),
                                   win_entries, G_N_ELEMENTS (win_entries),
                                   window);
}

static void
demo_application_window_dispose (GObject *object)
{
  DemoApplicationWindow *window = (DemoApplicationWindow *)object;

  bobgui_widget_dispose_template (BOBGUI_WIDGET (window), demo_application_window_get_type ());

  G_OBJECT_CLASS (demo_application_window_parent_class)->dispose (object);
}

static void
demo_application_window_class_init (DemoApplicationWindowClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (class);

  object_class->dispose = demo_application_window_dispose;

  bobgui_widget_class_set_template_from_resource (widget_class, "/application_demo/application.ui");
  bobgui_widget_class_bind_template_child (widget_class, DemoApplicationWindow, message);
  bobgui_widget_class_bind_template_child (widget_class, DemoApplicationWindow, infobar);
  bobgui_widget_class_bind_template_child (widget_class, DemoApplicationWindow, status);
  bobgui_widget_class_bind_template_child (widget_class, DemoApplicationWindow, buffer);
  bobgui_widget_class_bind_template_child (widget_class, DemoApplicationWindow, menubutton);
  bobgui_widget_class_bind_template_child (widget_class, DemoApplicationWindow, toolmenu);
  bobgui_widget_class_bind_template_callback (widget_class, clicked_cb);
  bobgui_widget_class_bind_template_callback (widget_class, update_statusbar);
  bobgui_widget_class_bind_template_callback (widget_class, mark_set_callback);
}

int
main (int argc, char *argv[])
{
  BobguiApplication *app;

  app = BOBGUI_APPLICATION (g_object_new (demo_application_get_type (),
                                       "application-id", "org.bobgui.Demo4.App",
                                       "flags", G_APPLICATION_HANDLES_OPEN,
                                       "register-session", TRUE,
                                       NULL));

  return g_application_run (G_APPLICATION (app), 0, NULL);
}
