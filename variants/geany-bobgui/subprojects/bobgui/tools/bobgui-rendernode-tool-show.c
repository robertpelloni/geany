/*  Copyright 2023 Red Hat, Inc.
 *
 * BOBGUI is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * BOBGUI is distributed in the hope that it will be useful,
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
#include "bobgui-rendernode-tool.h"
#include "bobgui-tool-utils.h"


static void
set_window_title (BobguiWindow  *window,
                  const char *filename)
{
  char *name;

  name = g_path_get_basename (filename);
  bobgui_window_set_title (window, name);
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
show_file (const char *filename,
           gboolean    decorated,
           gboolean    offload)
{
  GskRenderNode *node;
  graphene_rect_t node_bounds;
  GdkPaintable *paintable;
  BobguiWidget *sw;
  BobguiWidget *handle;
  BobguiWidget *window;
  gboolean done = FALSE;
  BobguiSnapshot *snapshot;
  BobguiWidget *picture;

  node = load_node_file (filename);
  gsk_render_node_get_bounds (node, &node_bounds);

  snapshot = bobgui_snapshot_new ();
  bobgui_snapshot_translate (snapshot, &GRAPHENE_POINT_INIT (- node_bounds.origin.x, - node_bounds.origin.y));
  bobgui_snapshot_append_node (snapshot, node);
  paintable = bobgui_snapshot_free_to_paintable (snapshot, NULL);

  picture = bobgui_picture_new_for_paintable (paintable);
  bobgui_picture_set_can_shrink (BOBGUI_PICTURE (picture), FALSE);
  bobgui_picture_set_content_fit (BOBGUI_PICTURE (picture), BOBGUI_CONTENT_FIT_SCALE_DOWN);

  if (offload)
    picture = bobgui_graphics_offload_new (picture);

  sw = bobgui_scrolled_window_new ();
  bobgui_scrolled_window_set_propagate_natural_width (BOBGUI_SCROLLED_WINDOW (sw), TRUE);
  bobgui_scrolled_window_set_propagate_natural_height (BOBGUI_SCROLLED_WINDOW (sw), TRUE);
  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (sw), picture);

  handle = bobgui_window_handle_new ();
  bobgui_window_handle_set_child (BOBGUI_WINDOW_HANDLE (handle), sw);

  window = bobgui_window_new ();
  bobgui_window_set_decorated (BOBGUI_WINDOW (window), decorated);
  bobgui_window_set_resizable (BOBGUI_WINDOW (window), decorated);
  if (!decorated)
    bobgui_widget_remove_css_class (window, "background");
  set_window_title (BOBGUI_WINDOW (window), filename);
  bobgui_window_set_child (BOBGUI_WINDOW (window), handle);

  bobgui_window_present (BOBGUI_WINDOW (window));
  g_signal_connect (window, "destroy", G_CALLBACK (quit_cb), &done);

  while (!done)
    g_main_context_iteration (NULL, TRUE);

  g_clear_object (&paintable);
  g_clear_pointer (&node, gsk_render_node_unref);
}

void
do_show (int          *argc,
         const char ***argv)
{
  GOptionContext *context;
  char **filenames = NULL;
  gboolean decorated = TRUE;
  gboolean offload = FALSE;
  const GOptionEntry entries[] = {
    { "offload", 0, G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, &offload, N_("Put node into offload container"), NULL },
    { "undecorated", 0, G_OPTION_FLAG_REVERSE, G_OPTION_ARG_NONE, &decorated, N_("Don't add a titlebar"), NULL },
    { G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_FILENAME_ARRAY, &filenames, NULL, N_("FILE") },
    { NULL, }
  };
  GError *error = NULL;

  if (gdk_display_get_default () == NULL)
    {
      g_printerr (_("Could not initialize windowing system\n"));
      exit (1);
    }

  g_set_prgname ("bobgui4-rendernode-tool show");
  context = g_option_context_new (NULL);
  g_option_context_set_translation_domain (context, GETTEXT_PACKAGE);
  g_option_context_add_main_entries (context, entries, NULL);
  g_option_context_set_summary (context, _("Show the render node."));

  if (!g_option_context_parse (context, argc, (char ***)argv, &error))
    {
      g_printerr ("%s\n", error->message);
      g_error_free (error);
      exit (1);
    }

  g_option_context_free (context);

  if (filenames == NULL)
    {
      g_printerr (_("No .node file specified\n"));
      exit (1);
    }

  if (g_strv_length (filenames) > 1)
    {
      g_printerr (_("Can only preview a single .node file\n"));
      exit (1);
    }

  show_file (filenames[0], decorated, offload);

  g_strfreev (filenames);
}
