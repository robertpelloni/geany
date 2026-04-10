/* Pickers and Launchers
 * #Keywords: BobguiColorDialog, BobguiFontDialog, BobguiFileDialog, BobguiPrintDialog, BobguiFileLauncher, BobguiUriLauncher
 *
 * The dialogs are mainly intended for use in preference dialogs.
 * They allow to select colors, fonts and files. There is also a
 * print dialog.
 *
 * The launchers let you open files or URIs in applications that
 * can handle them.
 */

#include <bobgui/bobgui.h>

static BobguiWidget *app_picker;
static BobguiWidget *open_folder_button;
static BobguiWidget *print_button;

static void
set_file (GFile    *file,
          gpointer  data)
{
  GFileInfo *info;
  char *name;

  if (!file)
    {
      bobgui_widget_set_sensitive (app_picker, FALSE);
      g_object_set_data (G_OBJECT (app_picker), "file", NULL);
      bobgui_widget_set_sensitive (open_folder_button, FALSE);
      g_object_set_data (G_OBJECT (open_folder_button), "file", NULL);
      return;
    }

  name = g_file_get_basename (file);
  bobgui_label_set_label (BOBGUI_LABEL (data), name);
  g_free (name);

  bobgui_widget_set_sensitive (app_picker, TRUE);
  g_object_set_data_full (G_OBJECT (app_picker), "file", g_object_ref (file), g_object_unref);
  bobgui_widget_set_sensitive (open_folder_button, TRUE);
  g_object_set_data_full (G_OBJECT (open_folder_button), "file", g_object_ref (file), g_object_unref);

  info = g_file_query_info (file, "standard::content-type", 0, NULL, NULL);
  if (strcmp (g_file_info_get_content_type (info), "application/pdf") == 0)
    {
      bobgui_widget_set_sensitive (print_button, TRUE);
      g_object_set_data_full (G_OBJECT (print_button), "file", g_object_ref (file), g_object_unref);
    }
}

static void
file_opened (GObject *source,
             GAsyncResult *result,
             void *data)
{
  GFile *file;
  GError *error = NULL;

  file = bobgui_file_dialog_open_finish (BOBGUI_FILE_DIALOG (source), result, &error);

  if (!file)
    {
      g_print ("%s\n", error->message);
      g_error_free (error);
      bobgui_widget_set_sensitive (app_picker, FALSE);
      g_object_set_data (G_OBJECT (app_picker), "file", NULL);
      bobgui_widget_set_sensitive (print_button, FALSE);
      g_object_set_data (G_OBJECT (print_button), "file", NULL);
    }

  set_file (file, data);
}

static gboolean
abort_mission (gpointer data)
{
  GCancellable *cancellable = data;

  g_cancellable_cancel (cancellable);

  return G_SOURCE_REMOVE;
}

static void
open_file (BobguiButton *picker,
           BobguiLabel  *label)
{
  BobguiWindow *parent = BOBGUI_WINDOW (bobgui_widget_get_root (BOBGUI_WIDGET (picker)));
  BobguiFileDialog *dialog;
  GCancellable *cancellable;

  dialog = bobgui_file_dialog_new ();

  cancellable = g_cancellable_new ();

  g_timeout_add_seconds_full (G_PRIORITY_DEFAULT,
                              20,
                              abort_mission, g_object_ref (cancellable), g_object_unref);

  bobgui_file_dialog_open (dialog, parent, cancellable, file_opened, label);

  g_object_unref (cancellable);
  g_object_unref (dialog);
}

static void
open_app_done (GObject      *source,
               GAsyncResult *result,
               gpointer      data)
{
  BobguiFileLauncher *launcher = BOBGUI_FILE_LAUNCHER (source);
  GError *error = NULL;

  if (!bobgui_file_launcher_launch_finish (launcher, result, &error))
    {
      g_print ("%s\n", error->message);
      g_error_free (error);
    }
}

static void
open_app (BobguiButton *picker)
{
  BobguiWindow *parent = BOBGUI_WINDOW (bobgui_widget_get_root (BOBGUI_WIDGET (picker)));
  BobguiFileLauncher *launcher;
  GFile *file;

  file = G_FILE (g_object_get_data (G_OBJECT (picker), "file"));
  launcher = bobgui_file_launcher_new (file);

  bobgui_file_launcher_launch (launcher, parent, NULL, open_app_done, NULL);

  g_object_unref (launcher);
}

static void
open_folder_done (GObject      *source,
                  GAsyncResult *result,
                  gpointer      data)
{
  BobguiFileLauncher *launcher = BOBGUI_FILE_LAUNCHER (source);
  GError *error = NULL;

  if (!bobgui_file_launcher_open_containing_folder_finish (launcher, result, &error))
    {
      g_print ("%s\n", error->message);
      g_error_free (error);
    }
}

static void
open_folder (BobguiButton *picker)
{
  BobguiWindow *parent = BOBGUI_WINDOW (bobgui_widget_get_root (BOBGUI_WIDGET (picker)));
  BobguiFileLauncher *launcher;
  GFile *file;

  file = G_FILE (g_object_get_data (G_OBJECT (picker), "file"));
  launcher = bobgui_file_launcher_new (file);

  bobgui_file_launcher_open_containing_folder (launcher, parent, NULL, open_folder_done, NULL);

  g_object_unref (launcher);
}

static void
print_file_done (GObject      *source,
                 GAsyncResult *result,
                 gpointer      data)
{
  BobguiPrintDialog *dialog = BOBGUI_PRINT_DIALOG (source);
  GError *error = NULL;
  GCancellable *cancellable;
  unsigned int id;

  cancellable = g_task_get_cancellable (G_TASK (result));
  id = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (cancellable), "timeout"));
  if (id)
    g_source_remove (id);

  if (!bobgui_print_dialog_print_file_finish (dialog, result, &error))
    {
      g_print ("%s\n", error->message);
      g_error_free (error);
    }
}

static void
print_file (BobguiButton *picker)
{
  BobguiWindow *parent = BOBGUI_WINDOW (bobgui_widget_get_root (BOBGUI_WIDGET (picker)));
  BobguiPrintDialog *dialog;
  GCancellable *cancellable;
  GFile *file;
  unsigned int id;

  file = G_FILE (g_object_get_data (G_OBJECT (picker), "file"));
  dialog = bobgui_print_dialog_new ();

  cancellable = g_cancellable_new ();

  id = g_timeout_add_seconds_full (G_PRIORITY_DEFAULT,
                                   20,
                                   abort_mission, g_object_ref (cancellable), g_object_unref);
  g_object_set_data (G_OBJECT (cancellable), "timeout", GUINT_TO_POINTER (id));

  bobgui_print_dialog_print_file (dialog, parent, NULL, file, cancellable, print_file_done, NULL);

  g_object_unref (cancellable);
  g_object_unref (dialog);
}

static void
open_uri_done (GObject      *source,
               GAsyncResult *result,
               gpointer      data)
{
  BobguiUriLauncher *launcher = BOBGUI_URI_LAUNCHER (source);
  GError *error = NULL;

  if (!bobgui_uri_launcher_launch_finish (launcher, result, &error))
    {
      g_print ("%s\n", error->message);
      g_error_free (error);
    }
}

static void
launch_uri (BobguiButton *picker)
{
  BobguiWindow *parent = BOBGUI_WINDOW (bobgui_widget_get_root (BOBGUI_WIDGET (picker)));
  BobguiUriLauncher *launcher;

  launcher = bobgui_uri_launcher_new ("http://www.bobgui.org");

  bobgui_uri_launcher_launch (launcher, parent, NULL, open_uri_done, NULL);

  g_object_unref (launcher);
}

static gboolean
on_drop (BobguiDropTarget *target,
         const GValue  *value,
         double         x,
         double         y,
         gpointer       data)
{
  if (G_VALUE_HOLDS (value, G_TYPE_FILE))
    {
      set_file (g_value_get_object (value), data);
      return TRUE;
    }

  return FALSE;
}

BobguiWidget *
do_pickers (BobguiWidget *do_widget)
{
  static BobguiWidget *window = NULL;
  BobguiWidget *table, *label, *picker, *button;
  BobguiDropTarget *drop_target;

  if (!window)
  {
    window = bobgui_window_new ();
    bobgui_window_set_display (BOBGUI_WINDOW (window),
                            bobgui_widget_get_display (do_widget));
    bobgui_window_set_title (BOBGUI_WINDOW (window), "Pickers and Launchers");
    g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);

    table = bobgui_grid_new ();
    bobgui_widget_set_margin_start (table, 20);
    bobgui_widget_set_margin_end (table, 20);
    bobgui_widget_set_margin_top (table, 20);
    bobgui_widget_set_margin_bottom (table, 20);
    bobgui_grid_set_row_spacing (BOBGUI_GRID (table), 6);
    bobgui_grid_set_column_spacing (BOBGUI_GRID (table), 6);
    bobgui_window_set_child (BOBGUI_WINDOW (window), table);

    label = bobgui_label_new_with_mnemonic ("_Color:");
    bobgui_widget_set_halign (label, BOBGUI_ALIGN_START);
    bobgui_widget_set_valign (label, BOBGUI_ALIGN_CENTER);
    bobgui_widget_set_hexpand (label, TRUE);
    bobgui_grid_attach (BOBGUI_GRID (table), label, 0, 0, 1, 1);

    picker = bobgui_color_dialog_button_new (bobgui_color_dialog_new ());
    bobgui_label_set_mnemonic_widget (BOBGUI_LABEL (label), picker);
    bobgui_grid_attach (BOBGUI_GRID (table), picker, 1, 0, 1, 1);

    label = bobgui_label_new_with_mnemonic ("_Font:");
    bobgui_widget_set_halign (label, BOBGUI_ALIGN_START);
    bobgui_widget_set_valign (label, BOBGUI_ALIGN_CENTER);
    bobgui_widget_set_hexpand (label, TRUE);
    bobgui_grid_attach (BOBGUI_GRID (table), label, 0, 1, 1, 1);

    picker = bobgui_font_dialog_button_new (bobgui_font_dialog_new ());
    bobgui_label_set_mnemonic_widget (BOBGUI_LABEL (label), picker);
    bobgui_grid_attach (BOBGUI_GRID (table), picker, 1, 1, 1, 1);

    label = bobgui_label_new_with_mnemonic ("_File:");
    bobgui_widget_set_halign (label, BOBGUI_ALIGN_START);
    bobgui_widget_set_valign (label, BOBGUI_ALIGN_CENTER);
    bobgui_widget_set_hexpand (label, TRUE);
    bobgui_grid_attach (BOBGUI_GRID (table), label, 0, 2, 1, 1);

    picker = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 6);
    button = bobgui_button_new_from_icon_name ("document-open-symbolic");
    bobgui_label_set_mnemonic_widget (BOBGUI_LABEL (label), button);
    bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (button),
                                    BOBGUI_ACCESSIBLE_PROPERTY_LABEL, "Select File",
                                    BOBGUI_ACCESSIBLE_PROPERTY_HAS_POPUP, TRUE,
                                    -1);

    label = bobgui_label_new ("None");

    drop_target = bobgui_drop_target_new (G_TYPE_FILE, GDK_ACTION_COPY);
    g_signal_connect (drop_target, "drop", G_CALLBACK (on_drop), label);
    bobgui_widget_add_controller (button, BOBGUI_EVENT_CONTROLLER (drop_target));

    bobgui_label_set_xalign (BOBGUI_LABEL (label), 0.);
    bobgui_label_set_ellipsize (BOBGUI_LABEL (label), PANGO_ELLIPSIZE_MIDDLE);
    bobgui_widget_set_hexpand (label, TRUE);
    g_signal_connect (button, "clicked", G_CALLBACK (open_file), label);
    bobgui_box_append (BOBGUI_BOX (picker), label);
    bobgui_box_append (BOBGUI_BOX (picker), button);
    app_picker = bobgui_button_new_from_icon_name ("system-run-symbolic");
    bobgui_widget_set_halign (app_picker, BOBGUI_ALIGN_END);
    bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (app_picker),
                                    BOBGUI_ACCESSIBLE_PROPERTY_LABEL, "Open File",
                                    BOBGUI_ACCESSIBLE_PROPERTY_HAS_POPUP, TRUE,
                                    -1);
    bobgui_widget_set_sensitive (app_picker, FALSE);
    g_signal_connect (app_picker, "clicked", G_CALLBACK (open_app), NULL);
    bobgui_box_append (BOBGUI_BOX (picker), app_picker);
    open_folder_button = bobgui_button_new_from_icon_name ("folder-symbolic");
    bobgui_widget_set_halign (open_folder_button, BOBGUI_ALIGN_END);
    bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (open_folder_button),
                                    BOBGUI_ACCESSIBLE_PROPERTY_LABEL, "Open in Folder",
                                    BOBGUI_ACCESSIBLE_PROPERTY_HAS_POPUP, TRUE,
                                    -1);
    bobgui_widget_set_sensitive (open_folder_button, FALSE);
    g_signal_connect (open_folder_button, "clicked", G_CALLBACK (open_folder), NULL);
    bobgui_box_append (BOBGUI_BOX (picker), open_folder_button);

    print_button = bobgui_button_new_from_icon_name ("printer-symbolic");
    bobgui_widget_set_tooltip_text (print_button, "Print File");
    bobgui_widget_set_sensitive (print_button, FALSE);
    bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (print_button),
                                    BOBGUI_ACCESSIBLE_PROPERTY_LABEL, "Print File",
                                    BOBGUI_ACCESSIBLE_PROPERTY_HAS_POPUP, TRUE,
                                    -1);
    g_signal_connect (print_button, "clicked", G_CALLBACK (print_file), NULL);
    bobgui_box_append (BOBGUI_BOX (picker), print_button);

    bobgui_grid_attach (BOBGUI_GRID (table), picker, 1, 2, 1, 1);

    label = bobgui_label_new_with_mnemonic ("_URI:");
    bobgui_widget_set_halign (label, BOBGUI_ALIGN_START);
    bobgui_widget_set_valign (label, BOBGUI_ALIGN_CENTER);
    bobgui_widget_set_hexpand (label, TRUE);
    bobgui_grid_attach (BOBGUI_GRID (table), label, 0, 3, 1, 1);

    picker = bobgui_button_new_with_label ("www.bobgui.org");
    bobgui_label_set_mnemonic_widget (BOBGUI_LABEL (label), picker);
    bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (picker),
                                    BOBGUI_ACCESSIBLE_PROPERTY_LABEL, "Open www.bobgui.org",
                                    BOBGUI_ACCESSIBLE_PROPERTY_HAS_POPUP, TRUE,
                                    -1);
    g_signal_connect (picker, "clicked", G_CALLBACK (launch_uri), NULL);
    bobgui_grid_attach (BOBGUI_GRID (table), picker, 1, 3, 1, 1);
  }

  if (!bobgui_widget_get_visible (window))
    bobgui_window_present (BOBGUI_WINDOW (window));
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));

  return window;
}
