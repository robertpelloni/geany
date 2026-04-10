/*  Copyright 2015 Red Hat, Inc.
 *
 * BOBGUI is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * GLib is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with BOBGUI; see the file COPYING.  If not,
 * see <http://www.gnu.org/licenses/>.
 *
 * Author: Matthias Clasen
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <glib/gi18n-lib.h>
#include <glib/gprintf.h>
#include <glib/gstdio.h>
#include <bobgui/bobgui.h>
#include "bobguibuilderprivate.h"
#include "bobgui-builder-tool.h"


static void
set_window_title (BobguiWindow  *window,
                  const char *filename,
                  const char *id)
{
  char *name;
  char *title;

  name = g_path_get_basename (filename);

  if (id)
    title = g_strdup_printf ("%s in %s", id, name);
  else
    title = g_strdup (name);

  bobgui_window_set_title (window, title);

  g_free (title);
  g_free (name);
}

static void
quit_cb (BobguiWidget *widget,
         gpointer   user_data)
{
  gboolean *is_done = user_data;

  *is_done = TRUE;

  g_main_context_wakeup (NULL);
}

static void
preview_file (const char *filename,
              const char *id,
              const char *cssfile)
{
  BobguiBuilder *builder;
  GError *error = NULL;
  GObject *object;
  BobguiWidget *window;
  gboolean done = FALSE;

  if (cssfile)
    {
      BobguiCssProvider *provider;

      provider = bobgui_css_provider_new ();
      bobgui_css_provider_load_from_path (provider, cssfile);

      bobgui_style_context_add_provider_for_display (gdk_display_get_default (),
                                                  BOBGUI_STYLE_PROVIDER (provider),
                                                  BOBGUI_STYLE_PROVIDER_PRIORITY_APPLICATION);
    }

  builder = bobgui_builder_new ();
  if (!bobgui_builder_add_from_file (builder, filename, &error))
    {
      g_printerr ("%s\n", error->message);
      exit (1);
    }

  object = NULL;

  if (id)
    {
      object = bobgui_builder_get_object (builder, id);
    }
  else
    {
      GSList *objects, *l;

      objects = bobgui_builder_get_objects (builder);
      for (l = objects; l; l = l->next)
        {
          GObject *obj = l->data;

          if (BOBGUI_IS_WINDOW (obj))
            {
              object = obj;
              break;
            }
          else if (BOBGUI_IS_WIDGET (obj))
            {
              if (object == NULL)
                object = obj;
            }
        }
      g_slist_free (objects);
    }

  if (object == NULL)
    {
      if (id)
        g_printerr (_("No object with ID '%s' found\n"), id);
      else
        g_printerr (_("No previewable object found\n"));
      exit (1);
    }

  if (!BOBGUI_IS_WIDGET (object))
    {
      g_printerr (_("Objects of type %s can't be previewed\n"), G_OBJECT_TYPE_NAME (object));
      exit (1);
    }

  if (BOBGUI_IS_WINDOW (object))
    window = BOBGUI_WIDGET (object);
  else
    {
      BobguiWidget *widget = BOBGUI_WIDGET (object);

      window = bobgui_window_new ();

      if (BOBGUI_IS_BUILDABLE (object))
        id = bobgui_buildable_get_buildable_id (BOBGUI_BUILDABLE (object));

      set_window_title (BOBGUI_WINDOW (window), filename, id);

      g_object_ref (widget);
      if (bobgui_widget_get_parent (widget) != NULL)
        bobgui_box_remove (BOBGUI_BOX (bobgui_widget_get_parent (widget)), widget);
      bobgui_window_set_child (BOBGUI_WINDOW (window), widget);
      g_object_unref (widget);
    }

  bobgui_window_present (BOBGUI_WINDOW (window));
  g_signal_connect (window, "destroy", G_CALLBACK (quit_cb), &done);

  while (!done)
    g_main_context_iteration (NULL, TRUE);

  g_object_unref (builder);
}

void
do_preview (int          *argc,
            const char ***argv)
{
  GOptionContext *context;
  char *id = NULL;
  char *css = NULL;
  char **filenames = NULL;
  const GOptionEntry entries[] = {
    { "id", 0, 0, G_OPTION_ARG_STRING, &id, N_("Preview only the named object"), N_("ID") },
    { "css", 0, 0, G_OPTION_ARG_FILENAME, &css, N_("Use style from CSS file"), N_("FILE") },
    { G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_FILENAME_ARRAY, &filenames, NULL, N_("FILE") },
    { NULL, }
  };
  GError *error = NULL;

  if (gdk_display_get_default () == NULL)
    {
      g_printerr (_("Could not initialize windowing system\n"));
      exit (1);
    }

  g_set_prgname ("bobgui4-builder-tool preview");
  context = g_option_context_new (NULL);
  g_option_context_set_translation_domain (context, GETTEXT_PACKAGE);
  g_option_context_add_main_entries (context, entries, NULL);
  g_option_context_set_summary (context, _("Preview the file."));

  if (!g_option_context_parse (context, argc, (char ***)argv, &error))
    {
      g_printerr ("%s\n", error->message);
      g_error_free (error);
      exit (1);
    }

  g_option_context_free (context);

  if (filenames == NULL)
    {
      g_printerr (_("No .ui file specified\n"));
      exit (1);
    }

  if (g_strv_length (filenames) > 1)
    {
      g_printerr (_("Can only preview a single .ui file\n"));
      exit (1);
    }

  preview_file (filenames[0], id, css);

  g_strfreev (filenames);
  g_free (id);
  g_free (css);
}
