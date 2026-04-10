#include <config.h>
#include <math.h>
#include <pango/pangocairo.h>
#include <glib/gi18n.h>
#include <gmodule.h>
#include <bobgui/bobgui.h>

#include "profile_conf.h"

static BobguiWidget *main_window;
static GFile *filename = NULL;
static BobguiPageSetup *page_setup = NULL;
static BobguiPrintSettings *settings = NULL;
static gboolean file_changed = FALSE;
static BobguiTextBuffer *buffer;
static BobguiWidget *statusbar;
static GList *active_prints = NULL;

static void
update_title (BobguiWindow *window)
{
  char *basename;
  char *title;

  if (filename == NULL)
    basename = g_strdup ("Untitled");
  else
    basename = g_file_get_basename (filename);

  title = g_strdup_printf ("BOBGUI Print Editor — %s", basename);
  g_free (basename);

  bobgui_window_set_title (window, title);
  g_free (title);
}

static void
update_statusbar (void)
{
  char *msg;
  int row, col;
  BobguiTextIter iter;
  const char *print_str;

  bobgui_label_set_label (BOBGUI_LABEL (statusbar), "");

  bobgui_text_buffer_get_iter_at_mark (buffer,
                                    &iter,
                                    bobgui_text_buffer_get_insert (buffer));

  row = bobgui_text_iter_get_line (&iter);
  col = bobgui_text_iter_get_line_offset (&iter);

  print_str = "";
  if (active_prints)
    {
      BobguiPrintOperation *op = active_prints->data;
      print_str = bobgui_print_operation_get_status_string (op);
    }

  msg = g_strdup_printf ("%d, %d%s %s",
                         row, col,
                         file_changed?" - Modified":"",
                         print_str);

  bobgui_label_set_label (BOBGUI_LABEL (statusbar), msg);

  g_free (msg);
}

static void
update_ui (void)
{
  update_title (BOBGUI_WINDOW (main_window));
  update_statusbar ();
}

static char *
get_text (void)
{
  BobguiTextIter start, end;

  bobgui_text_buffer_get_start_iter (buffer, &start);
  bobgui_text_buffer_get_end_iter (buffer, &end);
  return bobgui_text_buffer_get_text (buffer, &start, &end, FALSE);
}

static void
set_text (const char *text,
          gsize       len)
{
  bobgui_text_buffer_set_text (buffer, text, len);
  file_changed = FALSE;
  update_ui ();
}

static void
load_file (GFile *open_filename)
{
  char *contents;
  GError *error;
  gsize len;

  error = NULL;
  g_file_load_contents (open_filename, NULL, &contents, &len, NULL, &error);
  if (error == NULL)
    {
      if (g_utf8_validate (contents, len, NULL))
	{
          g_clear_object (&filename);
	  filename = g_object_ref (open_filename);
	  set_text (contents, len);
	  g_free (contents);
	}
      else
	{
          GFileInfo *info = g_file_query_info (open_filename, "standard::display-name", 0, NULL, &error);
          const char *display_name = g_file_info_get_display_name (info);
          BobguiAlertDialog *alert;

          alert = bobgui_alert_dialog_new ("Error loading file %s", display_name);
          bobgui_alert_dialog_set_detail (alert, "Not valid utf8");
          bobgui_alert_dialog_show (alert, BOBGUI_WINDOW (main_window));
          g_object_unref (alert);
          g_object_unref (info);
        }
    }
  else
    {
      GFileInfo *info = g_file_query_info (open_filename, "standard::display-name", 0, NULL, &error);
      const char *display_name = g_file_info_get_display_name (info);
      BobguiAlertDialog *alert;

      alert = bobgui_alert_dialog_new ("Error loading file %s", display_name);
      bobgui_alert_dialog_set_detail (alert, error->message);
      bobgui_alert_dialog_show (alert, BOBGUI_WINDOW (main_window));
      g_object_unref (alert);
      g_object_unref (info);
      g_error_free (error);
    }
}


static void
save_file (GFile *save_filename)
{
  char *text = get_text ();
  GError *error;

  error = NULL;
  g_file_replace_contents (save_filename,
                           text, strlen (text),
                           NULL, FALSE,
                           G_FILE_CREATE_NONE,
                           NULL,
                           NULL,
                           &error);

  if (error == NULL)
    {
      if (save_filename != filename)
	{
          g_clear_object (&filename);
          filename = g_object_ref (save_filename);
	}
      file_changed = FALSE;
      update_ui ();
    }
  else
    {
      GFileInfo *info = g_file_query_info (save_filename, "standard::display-name", 0, NULL, NULL);
      const char *display_name = g_file_info_get_display_name (info);
      BobguiAlertDialog *alert;

      alert = bobgui_alert_dialog_new ("Error saving to file %s", display_name);
      bobgui_alert_dialog_set_detail (alert, error->message);
      bobgui_alert_dialog_show (alert, BOBGUI_WINDOW (main_window));
      g_object_unref (alert);
      g_error_free (error);
      g_object_unref (info);
    }
}


typedef struct {
  char *text;
  PangoLayout *layout;
  GList *page_breaks;
  BobguiWidget *font_button;
  char *font;
} PrintData;

static void
begin_print (BobguiPrintOperation *operation,
	     BobguiPrintContext *context,
	     PrintData *print_data)
{
  PangoFontDescription *desc;
  PangoLayoutLine *layout_line;
  double width, height;
  double page_height;
  GList *page_breaks;
  int num_lines;
  int line;

  width = bobgui_print_context_get_width (context);
  height = bobgui_print_context_get_height (context);

  print_data->layout = bobgui_print_context_create_pango_layout (context);

  desc = pango_font_description_from_string (print_data->font);
  pango_layout_set_font_description (print_data->layout, desc);
  pango_font_description_free (desc);

  pango_layout_set_width (print_data->layout, width * PANGO_SCALE);

  pango_layout_set_text (print_data->layout, print_data->text, -1);

  num_lines = pango_layout_get_line_count (print_data->layout);

  page_breaks = NULL;
  page_height = 0;

  for (line = 0; line < num_lines; line++)
    {
      PangoRectangle ink_rect, logical_rect;
      double line_height;

      layout_line = pango_layout_get_line (print_data->layout, line);
      pango_layout_line_get_extents (layout_line, &ink_rect, &logical_rect);

      line_height = logical_rect.height / 1024.0;

      if (page_height + line_height > height)
	{
	  page_breaks = g_list_prepend (page_breaks, GINT_TO_POINTER (line));
	  page_height = 0;
	}

      page_height += line_height;
    }

  page_breaks = g_list_reverse (page_breaks);
  bobgui_print_operation_set_n_pages (operation, g_list_length (page_breaks) + 1);

  print_data->page_breaks = page_breaks;
}

static void
draw_page (BobguiPrintOperation *operation,
	   BobguiPrintContext *context,
	   int page_nr,
	   PrintData *print_data)
{
  cairo_t *cr;
  GList *pagebreak;
  int start, end, i;
  PangoLayoutIter *iter;
  double start_pos;

  if (page_nr == 0)
    start = 0;
  else
    {
      pagebreak = g_list_nth (print_data->page_breaks, page_nr - 1);
      start = GPOINTER_TO_INT (pagebreak->data);
    }

  pagebreak = g_list_nth (print_data->page_breaks, page_nr);
  if (pagebreak == NULL)
    end = pango_layout_get_line_count (print_data->layout);
  else
    end = GPOINTER_TO_INT (pagebreak->data);

  cr = bobgui_print_context_get_cairo_context (context);

  cairo_set_source_rgb (cr, 0, 0, 0);

  i = 0;
  start_pos = 0;
  iter = pango_layout_get_iter (print_data->layout);
  do
    {
      PangoRectangle   logical_rect;
      PangoLayoutLine *line;
      int              baseline;

      if (i >= start)
	{
	  line = pango_layout_iter_get_line (iter);

	  pango_layout_iter_get_line_extents (iter, NULL, &logical_rect);
	  baseline = pango_layout_iter_get_baseline (iter);

	  if (i == start)
	    start_pos = logical_rect.y / 1024.0;

	  cairo_move_to (cr, logical_rect.x / 1024.0, baseline / 1024.0 - start_pos);

	  pango_cairo_show_layout_line  (cr, line);
	}
      i++;
    }
  while (i < end &&
	 pango_layout_iter_next_line (iter));

  pango_layout_iter_free (iter);
}

static void
status_changed_cb (BobguiPrintOperation *op,
		   gpointer user_data)
{
  if (bobgui_print_operation_is_finished (op))
    {
      active_prints = g_list_remove (active_prints, op);
      g_object_unref (op);
    }
  update_statusbar ();
}

static BobguiWidget *
create_custom_widget (BobguiPrintOperation *operation,
		      PrintData *data)
{
  BobguiWidget *vbox, *hbox, *font, *label;
  BobguiFontDialog *dialog;
  PangoFontDescription *desc;

  bobgui_print_operation_set_custom_tab_label (operation, "Other");
  vbox = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);

  hbox = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 8);
  bobgui_box_append (BOBGUI_BOX (vbox), hbox);

  label = bobgui_label_new ("Font:");
  bobgui_box_append (BOBGUI_BOX (hbox), label);

  dialog = bobgui_font_dialog_new ();
  font = bobgui_font_dialog_button_new (dialog);
  desc = pango_font_description_from_string (data->font);
  bobgui_font_dialog_button_set_font_desc (BOBGUI_FONT_DIALOG_BUTTON (font), desc);
  pango_font_description_free (desc);
  bobgui_box_append (BOBGUI_BOX (hbox), font);
  data->font_button = font;

  return vbox;
}

static void
custom_widget_apply (BobguiPrintOperation *operation,
		     BobguiWidget *widget,
		     PrintData *data)
{
  PangoFontDescription *desc;

  desc = bobgui_font_dialog_button_get_font_desc (BOBGUI_FONT_DIALOG_BUTTON (data->font_button));

  g_free (data->font);
  data->font = pango_font_description_to_string (desc);
}

static void
print_done (BobguiPrintOperation *op,
	    BobguiPrintOperationResult res,
	    PrintData *print_data)
{
  if (res == BOBGUI_PRINT_OPERATION_RESULT_ERROR)
    {
      BobguiAlertDialog *alert;
      GError *error = NULL;

      bobgui_print_operation_get_error (op, &error);

      alert = bobgui_alert_dialog_new ("Error printing file");
      if (error)
        bobgui_alert_dialog_set_detail (alert, error->message);
      bobgui_alert_dialog_show (alert, BOBGUI_WINDOW (main_window));
      g_object_unref (alert);
    }
  else if (res == BOBGUI_PRINT_OPERATION_RESULT_APPLY)
    {
      if (settings != NULL)
	g_object_unref (settings);
      settings = g_object_ref (bobgui_print_operation_get_print_settings (op));
    }

  g_free (print_data->text);
  g_free (print_data->font);
  g_free (print_data);

  if (!bobgui_print_operation_is_finished (op))
    {
      g_object_ref (op);
      active_prints = g_list_append (active_prints, op);
      update_statusbar ();

      /* This ref is unref:ed when we get the final state change */
      g_signal_connect (op, "status_changed",
			G_CALLBACK (status_changed_cb), NULL);
    }
}

static void
end_print (BobguiPrintOperation *op, BobguiPrintContext *context, PrintData *print_data)
{
  g_list_free (print_data->page_breaks);
  print_data->page_breaks = NULL;
  g_object_unref (print_data->layout);
  print_data->layout = NULL;
}

static void
print_or_preview (GSimpleAction *action, BobguiPrintOperationAction print_action)
{
  BobguiPrintOperation *print;
  PrintData *print_data;

  print_data = g_new0 (PrintData, 1);

  print_data->text = get_text ();
  print_data->font = g_strdup ("Sans 12");

  print = bobgui_print_operation_new ();

  bobgui_print_operation_set_track_print_status (print, TRUE);

  if (settings != NULL)
    bobgui_print_operation_set_print_settings (print, settings);

  if (page_setup != NULL)
    bobgui_print_operation_set_default_page_setup (print, page_setup);

  g_signal_connect (print, "begin_print", G_CALLBACK (begin_print), print_data);
  g_signal_connect (print, "end-print", G_CALLBACK (end_print), print_data);
  g_signal_connect (print, "draw_page", G_CALLBACK (draw_page), print_data);
  g_signal_connect (print, "create_custom_widget", G_CALLBACK (create_custom_widget), print_data);
  g_signal_connect (print, "custom_widget_apply", G_CALLBACK (custom_widget_apply), print_data);

  g_signal_connect (print, "done", G_CALLBACK (print_done), print_data);

  bobgui_print_operation_set_export_filename (print, "test.pdf");

#if 0
  bobgui_print_operation_set_allow_async (print, TRUE);
#endif
  bobgui_print_operation_run (print, print_action, BOBGUI_WINDOW (main_window), NULL);

  g_object_unref (print);
}

static void
activate_page_setup (GSimpleAction *action,
                     GVariant      *parameter,
                     gpointer       user_data)
{
  BobguiPageSetup *new_page_setup;

  new_page_setup = bobgui_print_run_page_setup_dialog (BOBGUI_WINDOW (main_window),
                                                    page_setup, settings);

  if (page_setup)
    g_object_unref (page_setup);

  page_setup = new_page_setup;
}

static void
activate_print (GSimpleAction *action,
                GVariant      *parameter,
                gpointer       user_data)
{
  print_or_preview (action, BOBGUI_PRINT_OPERATION_ACTION_PRINT_DIALOG);
}

static void
activate_preview (GSimpleAction *action,
                GVariant      *parameter,
                gpointer       user_data)
{
  print_or_preview (action, BOBGUI_PRINT_OPERATION_ACTION_PREVIEW);
}

static void
on_save_response (GObject *source,
                  GAsyncResult *result,
                  void *user_data)
{
  BobguiFileDialog *dialog = BOBGUI_FILE_DIALOG (source);
  GFile *file;

  file = bobgui_file_dialog_save_finish (dialog, result, NULL);
  if (file)
    {
      save_file (file);
      g_object_unref (file);
    }
}

static void
activate_save_as (GSimpleAction *action,
                  GVariant      *parameter,
                  gpointer       user_data)
{
  BobguiFileDialog *dialog;

  dialog = bobgui_file_dialog_new ();
  bobgui_file_dialog_set_title (dialog, "Select file");
  bobgui_file_dialog_save (dialog,
                        BOBGUI_WINDOW (main_window),
                        NULL,
                        on_save_response, NULL);
  g_object_unref (dialog);
}

static void
activate_save (GSimpleAction *action,
               GVariant      *parameter,
               gpointer       user_data)
{
  if (filename == NULL)
    activate_save_as (action, NULL, NULL);
  else
    save_file (filename);
}

static void
on_open_response (GObject *source,
                  GAsyncResult *result,
                  void *user_data)
{
  BobguiFileDialog *dialog = BOBGUI_FILE_DIALOG (source);
  GFile *file;

  file = bobgui_file_dialog_open_finish (dialog, result, NULL);
  if (file)
    {
      load_file (file);
      g_object_unref (file);
    }
}

static void
activate_open (GSimpleAction *action,
               GVariant      *parameter,
               gpointer       user_data)
{
  BobguiFileDialog *dialog;

  dialog = bobgui_file_dialog_new ();
  bobgui_file_dialog_set_title (dialog, "Select file");
  bobgui_file_dialog_open (dialog,
                        BOBGUI_WINDOW (main_window),
                        NULL,
                        on_open_response, NULL);
  g_object_unref (dialog);
}

static void
activate_new (GSimpleAction *action,
              GVariant      *parameter,
              gpointer       user_data)
{
  g_clear_object (&filename);
  set_text ("", 0);
}

static void
activate_about (GSimpleAction *action,
                GVariant      *parameter,
                gpointer       user_data)
{
  char *version;
  GString *sysinfo;
  char *setting;
  char **backends;
  int i;
  char *os_name;
  char *os_version;
  BobguiWidget *dialog;
  GFile *logo_file;
  BobguiIconPaintable *logo;

  os_name = g_get_os_info (G_OS_INFO_KEY_NAME);
  os_version = g_get_os_info (G_OS_INFO_KEY_VERSION_ID);
  sysinfo = g_string_new ("");
  if (os_name && os_version)
    g_string_append_printf (sysinfo, "OS\t%s %s\n\n", os_name, os_version);
  g_string_append (sysinfo, "System libraries\n");
  g_string_append_printf (sysinfo, "\tGLib\t%d.%d.%d\n",
                          glib_major_version,
                          glib_minor_version,
                          glib_micro_version);
  g_string_append_printf (sysinfo, "\tPango\t%s\n",
                          pango_version_string ());
  g_string_append_printf (sysinfo, "\tBOBGUI \t%d.%d.%d\n",
                          bobgui_get_major_version (),
                          bobgui_get_minor_version (),
                          bobgui_get_micro_version ());

  g_string_append (sysinfo, "\nPrint backends\n");

  g_object_get (bobgui_settings_get_default (), "bobgui-print-backends", &setting, NULL);
  backends = g_strsplit (setting, ",", -1);
  g_string_append (sysinfo, "\t");
  for (i = 0; backends[i]; i++)
    g_string_append_printf (sysinfo, "%s ", backends[i]);
  g_strfreev (backends);
  g_free (setting);

  version = g_strdup_printf ("%s%s%s\nRunning against BOBGUI %d.%d.%d",
                             PACKAGE_VERSION,
                             g_strcmp0 (PROFILE, "devel") == 0 ? "-" : "",
                             g_strcmp0 (PROFILE, "devel") == 0 ? VCS_TAG : "",
                             bobgui_get_major_version (),
                             bobgui_get_minor_version (),
                             bobgui_get_micro_version ());

  logo_file = g_file_new_for_uri ("resource:///org/bobgui/bobgui4/print-editor/icons/apps/org.bobgui.PrintEditor4.svg");
  logo = bobgui_icon_paintable_new_for_file (logo_file, 64, 1);
  dialog = g_object_new (BOBGUI_TYPE_ABOUT_DIALOG,
                         "transient-for", main_window,
                         "program-name", g_strcmp0 (PROFILE, "devel") == 0
                                         ? "BOBGUI Print Editor (Development)"
                                         : "BOBGUI Print Editor",
                         "version", version,
                         "copyright", "© 2006-2024 Red Hat, Inc",
                         "license-type", BOBGUI_LICENSE_LGPL_2_1,
                         "website", "http://www.bobgui.org",
                         "comments", "Program to demonstrate BOBGUI printing",
                         "authors", (const char *[]) { "Alexander Larsson", NULL },
                         "logo", logo,
                         "title", "About BOBGUI Print Editor",
                         "system-information", sysinfo->str,
                         NULL);
  g_object_unref (logo);
  g_object_unref (logo_file);

  bobgui_about_dialog_add_credit_section (BOBGUI_ABOUT_DIALOG (dialog),
                                       _("Artwork by"), (const char *[]) { "Jakub Steiner", NULL });

  bobgui_about_dialog_add_credit_section (BOBGUI_ABOUT_DIALOG (dialog),
                                       _("Maintained by"), (const char *[]) { "The BOBGUI Team", NULL });

  bobgui_window_present (BOBGUI_WINDOW (dialog));

  g_string_free (sysinfo, TRUE);
  g_free (version);
  g_free (os_name);
  g_free (os_version);
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

static GActionEntry app_entries[] = {
  { "new", activate_new, NULL, NULL, NULL },
  { "open", activate_open, NULL, NULL, NULL },
  { "save", activate_save, NULL, NULL, NULL },
  { "save-as", activate_save_as, NULL, NULL, NULL },
  { "quit", activate_quit, NULL, NULL, NULL },
  { "about", activate_about, NULL, NULL, NULL },
  { "page-setup", activate_page_setup, NULL, NULL, NULL },
  { "preview", activate_preview, NULL, NULL, NULL },
  { "print", activate_print, NULL, NULL, NULL }
};

static const char ui_info[] =
  "<interface>"
  "  <menu id='menubar'>"
  "    <submenu>"
  "      <attribute name='label'>_File</attribute>"
  "      <section>"
  "        <item>"
  "          <attribute name='label'>_New</attribute>"
  "          <attribute name='action'>app.new</attribute>"
  "        </item>"
  "        <item>"
  "          <attribute name='label'>_Open</attribute>"
  "          <attribute name='action'>app.open</attribute>"
  "        </item>"
  "        <item>"
  "          <attribute name='label'>_Save</attribute>"
  "          <attribute name='action'>app.save</attribute>"
  "        </item>"
  "        <item>"
  "          <attribute name='label'>Save _As...</attribute>"
  "          <attribute name='action'>app.save-as</attribute>"
  "        </item>"
  "      </section>"
  "      <section>"
  "        <item>"
  "          <attribute name='label'>Page Setup</attribute>"
  "          <attribute name='action'>app.page-setup</attribute>"
  "        </item>"
  "        <item>"
  "          <attribute name='label'>Preview</attribute>"
  "          <attribute name='action'>app.preview</attribute>"
  "        </item>"
  "        <item>"
  "          <attribute name='label'>Print</attribute>"
  "          <attribute name='action'>app.print</attribute>"
  "        </item>"
  "      </section>"
  "      <section>"
  "        <item>"
  "          <attribute name='label'>_Quit</attribute>"
  "          <attribute name='action'>app.quit</attribute>"
  "        </item>"
  "      </section>"
  "    </submenu>"
  "    <submenu>"
  "      <attribute name='label'>_Help</attribute>"
  "      <section>"
  "        <item>"
  "          <attribute name='label'>_About Print Editor</attribute>"
  "          <attribute name='action'>app.about</attribute>"
  "        </item>"
  "      </section>"
  "    </submenu>"
  "  </menu>"
  "</interface>";

static void
buffer_changed_callback (BobguiTextBuffer *text_buffer)
{
  file_changed = TRUE;
  update_statusbar ();
}

static void
mark_set_callback (BobguiTextBuffer     *text_buffer,
                   const BobguiTextIter *new_location,
                   BobguiTextMark       *mark,
                   gpointer           data)
{
  update_statusbar ();
}

static void
startup (GApplication *app)
{
  BobguiBuilder *builder;
  GMenuModel *menubar;
  struct {
    const char *action_and_target;
    const char *accelerators[2];
  } accels[] = {
    { "app.new", { "<Control>n", NULL } },
    { "app.quit", { "<Control>q", NULL } },
    { "app.save", { "<Control>s", NULL } },
    { "app.about", { "<Control>a", NULL } },
  };

  builder = bobgui_builder_new ();
  bobgui_builder_add_from_string (builder, ui_info, -1, NULL);

  menubar = (GMenuModel *)bobgui_builder_get_object (builder, "menubar");

  bobgui_application_set_menubar (BOBGUI_APPLICATION (app), menubar);

  for (int i = 0; i < G_N_ELEMENTS (accels); i++)
    bobgui_application_set_accels_for_action (BOBGUI_APPLICATION (app), accels[i].action_and_target, accels[i].accelerators);

  g_object_unref (builder);
}

static void
activate (GApplication *app)
{
  GList *list;
  BobguiWidget *box;
  BobguiWidget *sw;
  BobguiWidget *contents;

  if ((list = bobgui_application_get_windows (BOBGUI_APPLICATION (app))) != NULL)
    {
      bobgui_window_present (BOBGUI_WINDOW (list->data));
      return;
    }

  main_window = bobgui_application_window_new (BOBGUI_APPLICATION (app));

  if (g_strcmp0 (PROFILE, "devel") == 0)
    bobgui_widget_add_css_class (BOBGUI_WIDGET (main_window), "devel");

  bobgui_window_set_default_size (BOBGUI_WINDOW (main_window), 400, 600);
  bobgui_application_window_set_show_menubar (BOBGUI_APPLICATION_WINDOW (main_window), TRUE);
  update_title (BOBGUI_WINDOW (main_window));

  box = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
  bobgui_window_set_child (BOBGUI_WINDOW (main_window), box);

  /* Create document  */
  sw = bobgui_scrolled_window_new ();

  bobgui_scrolled_window_set_policy (BOBGUI_SCROLLED_WINDOW (sw),
                                  BOBGUI_POLICY_AUTOMATIC,
                                  BOBGUI_POLICY_AUTOMATIC);

  bobgui_scrolled_window_set_has_frame (BOBGUI_SCROLLED_WINDOW (sw), TRUE);

  bobgui_widget_set_vexpand (sw, TRUE);
  bobgui_box_append (BOBGUI_BOX (box), sw);

  contents = bobgui_text_view_new ();
  bobgui_widget_grab_focus (contents);

  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (sw),
                     contents);

  /* Create statusbar */
  statusbar = bobgui_label_new ("");
  bobgui_label_set_xalign (BOBGUI_LABEL (statusbar), 0);
  bobgui_widget_set_margin_start (statusbar, 2);
  bobgui_widget_set_margin_end (statusbar, 2);
  bobgui_widget_set_margin_top (statusbar, 2);
  bobgui_widget_set_margin_bottom (statusbar, 2);
  bobgui_box_append (BOBGUI_BOX (box), statusbar);

  /* Show text widget info in the statusbar */
  buffer = bobgui_text_view_get_buffer (BOBGUI_TEXT_VIEW (contents));

  g_signal_connect_object (buffer,
                           "changed",
                           G_CALLBACK (buffer_changed_callback),
                           NULL,
                           0);

  g_signal_connect_object (buffer,
                           "mark_set", /* cursor moved */
                           G_CALLBACK (mark_set_callback),
                           NULL,
                           0);

  update_ui ();

  bobgui_window_present (BOBGUI_WINDOW (main_window));
}

static void
open (GApplication  *application,
      GFile        **files,
      int            n_files,
      const char    *hint)
{
  if (n_files > 1)
    g_warning ("Can only open a single file");

  activate (application);

  load_file (files[0]);
}

G_MODULE_EXPORT
int
main (int argc, char **argv)
{
  BobguiApplication *app;
  GError *error = NULL;
  char version[80];

  bobgui_init ();

  settings = bobgui_print_settings_new_from_file ("print-settings.ini", &error);
  if (error) {
    g_print ("Failed to load print settings: %s\n", error->message);
    g_clear_error (&error);

    settings = bobgui_print_settings_new ();
  }
  g_assert (settings != NULL);

  page_setup = bobgui_page_setup_new_from_file ("page-setup.ini", &error);
  if (error) {
    g_print ("Failed to load page setup: %s\n", error->message);
    g_clear_error (&error);
  }

  app = bobgui_application_new ("org.bobgui.PrintEditor4", G_APPLICATION_HANDLES_OPEN);

  g_snprintf (version, sizeof (version), "%s%s%s\n",
              PACKAGE_VERSION,
              g_strcmp0 (PROFILE, "devel") == 0 ? "-" : "",
              g_strcmp0 (PROFILE, "devel") == 0 ? VCS_TAG : "");

  g_application_set_version (G_APPLICATION (app), version);

  g_action_map_add_action_entries (G_ACTION_MAP (app),
                                   app_entries, G_N_ELEMENTS (app_entries),
                                   app);

  g_signal_connect (app, "startup", G_CALLBACK (startup), NULL);
  g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
  g_signal_connect (app, "open", G_CALLBACK (open), NULL);

  g_application_run (G_APPLICATION (app), argc, argv);

  if (!bobgui_print_settings_to_file (settings, "print-settings.ini", &error)) {
    g_print ("Failed to save print settings: %s\n", error->message);
    g_clear_error (&error);
  }
  if (page_setup &&
      !bobgui_page_setup_to_file (page_setup, "page-setup.ini", &error)) {
    g_print ("Failed to save page setup: %s\n", error->message);
    g_clear_error (&error);
  }

  return 0;
}
