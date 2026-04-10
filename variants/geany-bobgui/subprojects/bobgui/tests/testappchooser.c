/* testappchooser.c
 * Copyright (C) 2010 Red Hat, Inc.
 * Authors: Cosimo Cecchi
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

#include <stdlib.h>
#include <bobgui/bobgui.h>

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

static BobguiWidget *toplevel;
static GFile *file;
static BobguiWidget *grid, *file_l, *open;
static BobguiWidget *radio_file, *radio_content, *dialog;
static BobguiWidget *app_chooser_widget;
static BobguiWidget *def, *recommended, *fallback, *other, *all;

static void
dialog_response (BobguiDialog *d,
                 int        response_id,
                 gpointer   user_data)
{
  GAppInfo *app_info;
  const char *name;

  g_print ("Response: %d\n", response_id);

  if (response_id == BOBGUI_RESPONSE_OK)
    {
      app_info = bobgui_app_chooser_get_app_info (BOBGUI_APP_CHOOSER (d));
      if (app_info)
        {
          name = g_app_info_get_name (app_info);
          g_print ("Application selected: %s\n", name);
          g_object_unref (app_info);
        }
      else
        g_print ("No application selected\n");
    }

  bobgui_window_destroy (BOBGUI_WINDOW (d));
  dialog = NULL;
}

static void
bind_props (void)
{
  g_object_bind_property (def, "active",
                          app_chooser_widget, "show-default",
                          G_BINDING_SYNC_CREATE);
  g_object_bind_property (recommended, "active",
                          app_chooser_widget, "show-recommended",
                          G_BINDING_SYNC_CREATE);
  g_object_bind_property (fallback, "active",
                          app_chooser_widget, "show-fallback",
                          G_BINDING_SYNC_CREATE);
  g_object_bind_property (other, "active",
                          app_chooser_widget, "show-other",
                          G_BINDING_SYNC_CREATE);
  g_object_bind_property (all, "active",
                          app_chooser_widget, "show-all",
                          G_BINDING_SYNC_CREATE);
}

static void
prepare_dialog (void)
{
  gboolean use_file = FALSE;
  char *content_type = NULL;

  if (bobgui_check_button_get_active (BOBGUI_CHECK_BUTTON (radio_file)))
    use_file = TRUE;
  else if (bobgui_check_button_get_active (BOBGUI_CHECK_BUTTON (radio_content)))
    use_file = FALSE;

  if (use_file)
    {
      dialog = bobgui_app_chooser_dialog_new (BOBGUI_WINDOW (toplevel), 0, file);
    }
  else
    {
      GFileInfo *info;

      info = g_file_query_info (file,
                                G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
                                0, NULL, NULL);
      content_type = g_strdup (g_file_info_get_content_type (info));

      g_object_unref (info);

      dialog = bobgui_app_chooser_dialog_new_for_content_type (BOBGUI_WINDOW (toplevel),
                                                            0, content_type);
    }

  bobgui_app_chooser_dialog_set_heading (BOBGUI_APP_CHOOSER_DIALOG (dialog), "Select one already, you <i>fool</i>");

  g_signal_connect (dialog, "response",
                    G_CALLBACK (dialog_response), NULL);

  g_free (content_type);

  app_chooser_widget = bobgui_app_chooser_dialog_get_widget (BOBGUI_APP_CHOOSER_DIALOG (dialog));
  bind_props ();
}

static void
display_dialog (void)
{
  if (dialog == NULL)
    prepare_dialog ();

  bobgui_window_present (BOBGUI_WINDOW (dialog));
}

static void
on_open_response (BobguiWidget *file_chooser,
                  int        response)
{
  if (response == BOBGUI_RESPONSE_ACCEPT)
    {
      char *path;

      file = bobgui_file_chooser_get_file (BOBGUI_FILE_CHOOSER (file_chooser));
      path = g_file_get_path (file);

      bobgui_button_set_label (BOBGUI_BUTTON (file_l), path);

      g_free (path);
    }

  bobgui_window_destroy (BOBGUI_WINDOW (file_chooser));

  bobgui_widget_set_sensitive (open, TRUE);
}

static void
button_clicked (BobguiButton *b,
                gpointer   user_data)
{
  BobguiWidget *w;

  w = bobgui_file_chooser_dialog_new ("Select file",
                                   BOBGUI_WINDOW (toplevel),
                                   BOBGUI_FILE_CHOOSER_ACTION_OPEN,
                                   "_Cancel", BOBGUI_RESPONSE_CANCEL,
                                   "_Open", BOBGUI_RESPONSE_ACCEPT,
                                   NULL);

  bobgui_window_set_modal (BOBGUI_WINDOW (w), TRUE);

  g_signal_connect (w, "response",
                    G_CALLBACK (on_open_response),
                    NULL);

  bobgui_window_present (BOBGUI_WINDOW (w));
}

static void
quit_cb (BobguiWidget *widget,
         gpointer   data)
{
  gboolean *done = data;

  *done = TRUE;

  g_main_context_wakeup (NULL);
}

int
main (int argc, char **argv)
{
  BobguiWidget *w1;
  char *path;
  gboolean done = FALSE;

  bobgui_init ();

  toplevel = bobgui_window_new ();
  grid = bobgui_grid_new ();

  w1 = bobgui_label_new ("File:");
  bobgui_widget_set_halign (w1, BOBGUI_ALIGN_START);
  bobgui_grid_attach (BOBGUI_GRID (grid),
                   w1, 0, 0, 1, 1);

  file_l = bobgui_button_new ();
  path = g_build_filename (BOBGUI_SRCDIR, "apple-red.png", NULL);
  file = g_file_new_for_path (path);
  bobgui_button_set_label (BOBGUI_BUTTON (file_l), path);
  g_free (path);

  bobgui_widget_set_halign (file_l, BOBGUI_ALIGN_START);
  bobgui_grid_attach_next_to (BOBGUI_GRID (grid), file_l,
                           w1, BOBGUI_POS_RIGHT, 3, 1);
  g_signal_connect (file_l, "clicked",
                    G_CALLBACK (button_clicked), NULL);

  radio_file = bobgui_check_button_new_with_label ("Use GFile");
  radio_content = bobgui_check_button_new_with_label ("Use content type");
  bobgui_check_button_set_group (BOBGUI_CHECK_BUTTON (radio_content), BOBGUI_CHECK_BUTTON (radio_file));
  bobgui_check_button_set_active (BOBGUI_CHECK_BUTTON (radio_file), TRUE);

  bobgui_grid_attach (BOBGUI_GRID (grid), radio_file,
                   0, 1, 1, 1);
  bobgui_grid_attach_next_to (BOBGUI_GRID (grid), radio_content,
                           radio_file, BOBGUI_POS_BOTTOM, 1, 1);

  open = bobgui_button_new_with_label ("Trigger App Chooser dialog");
  bobgui_grid_attach_next_to (BOBGUI_GRID (grid), open,
                           radio_content, BOBGUI_POS_BOTTOM, 1, 1);

  recommended = bobgui_check_button_new_with_label ("Show recommended");
  bobgui_grid_attach_next_to (BOBGUI_GRID (grid), recommended,
                           open, BOBGUI_POS_BOTTOM, 1, 1);
  g_object_set (recommended, "active", TRUE, NULL);

  fallback = bobgui_check_button_new_with_label ("Show fallback");
  bobgui_grid_attach_next_to (BOBGUI_GRID (grid), fallback,
                           recommended, BOBGUI_POS_RIGHT, 1, 1);

  other = bobgui_check_button_new_with_label ("Show other");
  bobgui_grid_attach_next_to (BOBGUI_GRID (grid), other,
                           fallback, BOBGUI_POS_RIGHT, 1, 1);

  all = bobgui_check_button_new_with_label ("Show all");
  bobgui_grid_attach_next_to (BOBGUI_GRID (grid), all,
                           other, BOBGUI_POS_RIGHT, 1, 1);

  def = bobgui_check_button_new_with_label ("Show default");
  bobgui_grid_attach_next_to (BOBGUI_GRID (grid), def,
                           all, BOBGUI_POS_RIGHT, 1, 1);

  g_object_set (recommended, "active", TRUE, NULL);
  prepare_dialog ();
  g_signal_connect (open, "clicked",
                    G_CALLBACK (display_dialog), NULL);

  bobgui_window_set_child (BOBGUI_WINDOW (toplevel), grid);

  bobgui_window_present (BOBGUI_WINDOW (toplevel));
  g_signal_connect (toplevel, "destroy", G_CALLBACK (quit_cb), &done);

  while (!done)
    g_main_context_iteration (NULL, TRUE);

  return EXIT_SUCCESS;
}
