/* testfilechooser.c
 * Copyright (C) 2003  Red Hat, Inc.
 * Author: Owen Taylor
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */
#include "config.h"

#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <time.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <bobgui/bobgui.h>

#ifdef G_OS_WIN32
#  include <io.h>
#  define localtime_r(t,b) *(b) = *localtime (t)
#  ifndef S_ISREG
#    define S_ISREG(m) ((m) & _S_IFREG)
#  endif
#endif

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

#if 0
static BobguiWidget *preview_label;
static BobguiWidget *preview_image;
#endif
static BobguiFileChooserAction action;

static void
response_cb (BobguiDialog *dialog,
	     int        response_id,
             gpointer   data)
{
  gboolean *done = data;

  if (response_id == BOBGUI_RESPONSE_OK)
    {
      GListModel *files;
      guint i, n;

      files = bobgui_file_chooser_get_files (BOBGUI_FILE_CHOOSER (dialog));
      n = g_list_model_get_n_items (files);

      g_print ("Selected files:\n");
      for (i = 0; i < n; i++)
        {
          GFile *file = g_list_model_get_item (files, i);
          char *uri = g_file_get_uri (file);
          g_print ("  %s\n", uri ? uri : "(null)");
          g_free (uri);
          g_object_unref (file);
        }

      g_object_unref (files);
    }
  else
    g_print ("Dialog was closed\n");

  *done = TRUE;

  g_main_context_wakeup (NULL);
}

static void
filter_changed (BobguiFileChooserDialog *dialog,
		gpointer              data)
{
  g_print ("file filter changed\n");
}

#include <stdio.h>
#include <errno.h>
#define _(s) (s)


static void
set_current_folder (BobguiFileChooser *chooser,
		    const char     *name)
{
  GFile *file = g_file_new_for_path (name);
  if (!bobgui_file_chooser_set_current_folder (chooser, file, NULL))
    {
      BobguiWidget *dialog;

      dialog = bobgui_message_dialog_new (BOBGUI_WINDOW (chooser),
				       BOBGUI_DIALOG_MODAL | BOBGUI_DIALOG_DESTROY_WITH_PARENT,
				       BOBGUI_MESSAGE_ERROR,
				       BOBGUI_BUTTONS_CLOSE,
				       "Could not set the folder to %s",
				       name);
      bobgui_window_present (BOBGUI_WINDOW (dialog));
      g_signal_connect (dialog, "response",
                        G_CALLBACK (bobgui_window_destroy),
                        NULL);
    }
  g_object_unref (file);
}

static void
set_folder_nonexistent_cb (BobguiButton      *button,
			   BobguiFileChooser *chooser)
{
  set_current_folder (chooser, "/nonexistent");
}

static void
set_folder_existing_nonexistent_cb (BobguiButton      *button,
				    BobguiFileChooser *chooser)
{
  set_current_folder (chooser, "/usr/nonexistent");
}

static void
set_filename (BobguiFileChooser *chooser,
	      const char     *name)
{
  GFile *file = g_file_new_for_path (name);
  if (!bobgui_file_chooser_set_file (chooser, file, NULL))
    {
      BobguiWidget *dialog;

      dialog = bobgui_message_dialog_new (BOBGUI_WINDOW (chooser),
				       BOBGUI_DIALOG_MODAL | BOBGUI_DIALOG_DESTROY_WITH_PARENT,
				       BOBGUI_MESSAGE_ERROR,
				       BOBGUI_BUTTONS_CLOSE,
				       "Could not select %s",
				       name);
      bobgui_window_present (BOBGUI_WINDOW (dialog));
      g_signal_connect (dialog, "response",
                        G_CALLBACK (bobgui_window_destroy),
                        NULL);
    }
  g_object_unref (file);
}

static void
set_filename_nonexistent_cb (BobguiButton      *button,
			     BobguiFileChooser *chooser)
{
  set_filename (chooser, "/nonexistent");
}

static void
set_filename_existing_nonexistent_cb (BobguiButton      *button,
				      BobguiFileChooser *chooser)
{
  set_filename (chooser, "/usr/nonexistent");
}

static void
get_selection_cb (BobguiButton      *button,
		  BobguiFileChooser *chooser)
{
  GListModel *selection;
  guint i, n;

  selection = bobgui_file_chooser_get_files (chooser);
  n = g_list_model_get_n_items (selection);

  g_print ("Selection: ");

  for (i = 0; i < n; i++)
    {
      GFile *file = g_list_model_get_item (selection, i);
      char *uri = g_file_get_uri (file);
      g_print ("%s\n", uri);
      g_free (uri);
      g_object_unref (file);
    }

  g_object_unref (selection);
}

static void
get_current_name_cb (BobguiButton      *button,
		     BobguiFileChooser *chooser)
{
  char *name;

  name = bobgui_file_chooser_get_current_name (chooser);
  g_print ("Current name: %s\n", name ? name : "NULL");
  g_free (name);
}

static void
unmap_and_remap_cb (BobguiButton *button,
		    BobguiFileChooser *chooser)
{
  bobgui_widget_set_visible (BOBGUI_WIDGET (chooser), FALSE);
  bobgui_widget_set_visible (BOBGUI_WIDGET (chooser), TRUE);
}

static void
kill_dependent (BobguiWindow *win, BobguiWidget *dep)
{
  bobgui_window_destroy (BOBGUI_WINDOW (dep));
}

int
main (int argc, char **argv)
{
  BobguiWidget *control_window;
  BobguiWidget *vbbox;
  BobguiWidget *button;
  BobguiWidget *dialog;
  BobguiFileFilter *filter;
  gboolean force_rtl = FALSE;
  gboolean multiple = FALSE;
  char *action_arg = NULL;
  char *initial_filename = NULL;
  char *initial_folder = NULL;
  GFile *file;
  GError *error = NULL;
  GOptionEntry options[] = {
    { "action", 'a', 0, G_OPTION_ARG_STRING, &action_arg, "Filechooser action", "ACTION" },
    { "multiple", 'm', 0, G_OPTION_ARG_NONE, &multiple, "Select multiple", NULL },
    { "right-to-left", 'r', 0, G_OPTION_ARG_NONE, &force_rtl, "Force right-to-left layout.", NULL },
    { "initial-filename", 'f', 0, G_OPTION_ARG_FILENAME, &initial_filename, "Initial filename to select", "FILENAME" },
    { "initial-folder", 'F', 0, G_OPTION_ARG_FILENAME, &initial_folder, "Initial folder to show", "FILENAME" },
    { NULL }
  };
  GOptionContext *context;
  gboolean done = FALSE;

  context = g_option_context_new ("");
  g_option_context_add_main_entries (context, options, NULL);
  if (!g_option_context_parse (context, &argc, &argv, &error))
    {
      g_print ("Failed to parse args: %s\n", error->message);
      g_error_free (error);
      return 1;
    }
  g_option_context_free (context);

  bobgui_init ();

  if (initial_filename && initial_folder)
    {
      g_print ("Only one of --initial-filename and --initial-folder may be specified");
      return 1;
    }

  if (force_rtl)
    bobgui_widget_set_default_direction (BOBGUI_TEXT_DIR_RTL);

  action = BOBGUI_FILE_CHOOSER_ACTION_OPEN;

  if (action_arg != NULL)
    {
      if (! strcmp ("open", action_arg))
	action = BOBGUI_FILE_CHOOSER_ACTION_OPEN;
      else if (! strcmp ("save", action_arg))
	action = BOBGUI_FILE_CHOOSER_ACTION_SAVE;
      else if (! strcmp ("select_folder", action_arg))
	action = BOBGUI_FILE_CHOOSER_ACTION_SELECT_FOLDER;
      else
	{
	  g_print ("--action must be one of \"open\", \"save\", \"select_folder\"\n");
	  return 1;
	}

      g_free (action_arg);
    }

  dialog = g_object_new (BOBGUI_TYPE_FILE_CHOOSER_DIALOG,
			 "action", action,
			 "select-multiple", multiple,
			 NULL);

  switch (action)
    {
    case BOBGUI_FILE_CHOOSER_ACTION_OPEN:
    case BOBGUI_FILE_CHOOSER_ACTION_SELECT_FOLDER:
      bobgui_window_set_title (BOBGUI_WINDOW (dialog), "Select a file");
      bobgui_dialog_add_buttons (BOBGUI_DIALOG (dialog),
			      _("_Cancel"), BOBGUI_RESPONSE_CANCEL,
			      _("_Open"), BOBGUI_RESPONSE_OK,
			      NULL);
      break;
    case BOBGUI_FILE_CHOOSER_ACTION_SAVE:
      bobgui_window_set_title (BOBGUI_WINDOW (dialog), "Save a file");
      bobgui_dialog_add_buttons (BOBGUI_DIALOG (dialog),
			      _("_Cancel"), BOBGUI_RESPONSE_CANCEL,
			      _("_Save"), BOBGUI_RESPONSE_OK,
			      NULL);
      break;
    default:
      g_assert_not_reached ();
    }
  bobgui_dialog_set_default_response (BOBGUI_DIALOG (dialog), BOBGUI_RESPONSE_OK);

  g_signal_connect (dialog, "response",
		    G_CALLBACK (response_cb), &done);

  /* Filters */
  filter = bobgui_file_filter_new ();
  bobgui_file_filter_set_name (filter, "All Files");
  bobgui_file_filter_add_pattern (filter, "*");
  bobgui_file_chooser_add_filter (BOBGUI_FILE_CHOOSER (dialog), filter);

  /* Make this filter the default */
  bobgui_file_chooser_set_filter (BOBGUI_FILE_CHOOSER (dialog), filter);
  g_object_unref (filter);

  filter = bobgui_file_filter_new ();
  bobgui_file_filter_set_name (filter, "Starts with D");
  bobgui_file_filter_add_pattern (filter, "D*");
  bobgui_file_chooser_add_filter (BOBGUI_FILE_CHOOSER (dialog), filter);
  g_object_unref (filter);

  g_signal_connect (dialog, "notify::filter",
		    G_CALLBACK (filter_changed), NULL);

  filter = bobgui_file_filter_new ();
  bobgui_file_filter_set_name (filter, "PNG and JPEG");
  bobgui_file_filter_add_mime_type (filter, "image/jpeg");
  bobgui_file_filter_add_mime_type (filter, "image/png");
  bobgui_file_chooser_add_filter (BOBGUI_FILE_CHOOSER (dialog), filter);
  g_object_unref (filter);

  filter = bobgui_file_filter_new ();
  bobgui_file_filter_set_name (filter, "Images");
  bobgui_file_filter_add_pixbuf_formats (filter);
  bobgui_file_chooser_add_filter (BOBGUI_FILE_CHOOSER (dialog), filter);
  g_object_unref (filter);

  /* Choices */

  bobgui_file_chooser_add_choice (BOBGUI_FILE_CHOOSER (dialog), "choice1",
                               "Choose one:",
                               (const char *[]){"one", "two", "three", NULL},
                               (const char *[]){"One", "Two", "Three", NULL});
  bobgui_file_chooser_set_choice (BOBGUI_FILE_CHOOSER (dialog), "choice1", "two");

  /* Shortcuts */

  file = g_file_new_for_uri ("file:///usr/share/pixmaps");
  bobgui_file_chooser_add_shortcut_folder (BOBGUI_FILE_CHOOSER (dialog), file, NULL);
  g_object_unref (file);

  file = g_file_new_for_path (g_get_user_special_dir (G_USER_DIRECTORY_MUSIC));
  bobgui_file_chooser_add_shortcut_folder (BOBGUI_FILE_CHOOSER (dialog), file, NULL);
  g_object_unref (file);

  /* Initial filename or folder */

  if (initial_filename)
    set_filename (BOBGUI_FILE_CHOOSER (dialog), initial_filename);

  if (initial_folder)
    set_current_folder (BOBGUI_FILE_CHOOSER (dialog), initial_folder);

  bobgui_window_present (BOBGUI_WINDOW (dialog));

  /* Extra controls for manipulating the test environment
   */

  control_window = bobgui_window_new ();

  vbbox = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
  bobgui_window_set_child (BOBGUI_WINDOW (control_window), vbbox);

  button = bobgui_button_new_with_label ("set_current_folder (\"/nonexistent\")");
  bobgui_box_append (BOBGUI_BOX (vbbox), button);
  g_signal_connect (button, "clicked",
		    G_CALLBACK (set_folder_nonexistent_cb), dialog);

  button = bobgui_button_new_with_label ("set_current_folder (\"/usr/nonexistent\")");
  bobgui_box_append (BOBGUI_BOX (vbbox), button);
  g_signal_connect (button, "clicked",
		    G_CALLBACK (set_folder_existing_nonexistent_cb), dialog);

  button = bobgui_button_new_with_label ("set_filename (\"/nonexistent\")");
  bobgui_box_append (BOBGUI_BOX (vbbox), button);
  g_signal_connect (button, "clicked",
		    G_CALLBACK (set_filename_nonexistent_cb), dialog);

  button = bobgui_button_new_with_label ("set_filename (\"/usr/nonexistent\")");
  bobgui_box_append (BOBGUI_BOX (vbbox), button);
  g_signal_connect (button, "clicked",
		    G_CALLBACK (set_filename_existing_nonexistent_cb), dialog);

  button = bobgui_button_new_with_label ("Get selection");
  bobgui_box_append (BOBGUI_BOX (vbbox), button);
  g_signal_connect (button, "clicked",
		    G_CALLBACK (get_selection_cb), dialog);

  button = bobgui_button_new_with_label ("Get current name");
  bobgui_box_append (BOBGUI_BOX (vbbox), button);
  g_signal_connect (button, "clicked",
		    G_CALLBACK (get_current_name_cb), dialog);

  button = bobgui_button_new_with_label ("Unmap and remap");
  bobgui_box_append (BOBGUI_BOX (vbbox), button);
  g_signal_connect (button, "clicked",
		    G_CALLBACK (unmap_and_remap_cb), dialog);

  bobgui_widget_show (control_window);

  g_signal_connect (dialog, "destroy",
		    G_CALLBACK (kill_dependent), control_window);

  /* We need to hold a ref until we have destroyed the widgets, just in case
   * someone else destroys them.  We explicitly destroy windows to catch leaks.
   */
  g_object_ref (dialog);
  while (!done)
    g_main_context_iteration (NULL, TRUE);
  bobgui_window_destroy (BOBGUI_WINDOW (dialog));
  g_object_unref (dialog);

  return 0;
}
