/*  Copyright 2025 Red Hat, Inc.
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
#include "bobgui-image-tool.h"

static void
quit_cb (BobguiWidget *widget,
         gpointer   user_data)
{
  gboolean *is_done = user_data;

  *is_done = TRUE;

  g_main_context_wakeup (NULL);
}

static void
update_tooltip (BobguiWidget    *widget,
                unsigned int  state)
{
  char *text = g_strdup_printf ("State: %u", state);
  bobgui_widget_set_tooltip_text (widget, text);
  g_free (text);
}

static void
clicked (BobguiGestureClick *click,
         int              n_press,
         double           x,
         double           y,
         BobguiSvg          *svg)
{
  unsigned int state;
  BobguiWidget *widget;

  widget = bobgui_event_controller_get_widget (BOBGUI_EVENT_CONTROLLER (click));

  state = bobgui_svg_get_state (svg);

  if (bobgui_gesture_single_get_current_button (BOBGUI_GESTURE_SINGLE (click)) == 1)
    {
      if (state == 63)
        state = 0;
      else
        state++;
    }
  else
    {
      if (state == 0)
        state = 63;
      else
        state--;
    }

  bobgui_svg_set_state (svg, state);
  update_tooltip (widget, state);
}

static gboolean
toggle_playing (BobguiWidget *widget,
                GVariant  *args,
                gpointer   user_data)
{
  BobguiSvg *svg;
  gboolean playing;

  svg = BOBGUI_SVG (bobgui_picture_get_paintable (BOBGUI_PICTURE (widget)));
  g_object_get (svg, "playing", &playing, NULL);
  g_object_set (svg, "playing", !playing, NULL);
  return TRUE;
}

static BobguiSvg * load_animation_file (const char     *filename,
                                     BobguiSvgFeatures  features);

static gboolean
restart (BobguiWidget *widget,
         GVariant  *args,
         gpointer   user_data)
{
  BobguiSvg *svg;
  const char *filename;
  BobguiSvgFeatures features;

  svg = BOBGUI_SVG (bobgui_picture_get_paintable (BOBGUI_PICTURE (widget)));
  features = bobgui_svg_get_features (svg);

  filename = (const char *) g_object_get_data (G_OBJECT (widget), "filename");

  svg = load_animation_file (filename, features);

  bobgui_svg_set_frame_clock (svg, bobgui_widget_get_frame_clock (widget));

  bobgui_svg_play (svg);
  bobgui_picture_set_paintable (BOBGUI_PICTURE (widget), GDK_PAINTABLE (svg));
  g_object_unref (svg);

  return TRUE;
}

static void
error_cb (BobguiSvg *svg, GError *error)
{
  if (error->domain == BOBGUI_SVG_ERROR)
    {
      const BobguiSvgLocation *start = bobgui_svg_error_get_start (error);
      const BobguiSvgLocation *end = bobgui_svg_error_get_end (error);
      const char *element = bobgui_svg_error_get_element (error);
      const char *attribute = bobgui_svg_error_get_attribute (error);

      if (start)
        {
          if (end->lines != start->lines || end->line_chars != start->line_chars)
            g_print ("%" G_GSIZE_FORMAT ".%" G_GSIZE_FORMAT " - %" G_GSIZE_FORMAT ".%" G_GSIZE_FORMAT ": ",
                     start->lines, start->line_chars,
                     end->lines, end->line_chars);
          else
            g_print ("%" G_GSIZE_FORMAT ".%" G_GSIZE_FORMAT ": ", start->lines, start->line_chars);
        }

      if (element && attribute)
        g_print ("(%s / %s) ", element, attribute);
      else if (element)
        g_print ("(%s) ", element);
    }

  g_print ("%s\n", error->message);
}

static BobguiSvg *
load_animation_file (const char     *filename,
                     BobguiSvgFeatures  features)
{
  char *contents;
  size_t length;
  GBytes *bytes;
  GError *error = NULL;
  BobguiSvg *svg;

  if (!g_file_get_contents (filename, &contents, &length, &error))
    {
      g_printerr ("%s\n", error->message);
      exit (1);
    }

  bytes = g_bytes_new_take (contents, length);

  svg = bobgui_svg_new ();
  bobgui_svg_set_features (svg, features);
  g_signal_connect (svg, "error", G_CALLBACK (error_cb), NULL);
  bobgui_svg_load_from_bytes (svg, bytes);

  g_bytes_unref (bytes);

  return svg;
}

static void
show_files (char           **filenames,
            gboolean         decorated,
            int              size,
            BobguiSvgFeatures   features)
{
  BobguiWidget *sw;
  BobguiWidget *window;
  gboolean done = FALSE;
  GString *title;
  BobguiWidget *box;

  window = bobgui_window_new ();
  g_signal_connect (window, "destroy", G_CALLBACK (quit_cb), &done);

  bobgui_widget_realize (window);

  title = g_string_new ("");
  for (int i = 0; i < g_strv_length (filenames); i++)
    {
      char *name = g_path_get_basename (filenames[i]);

      if (title->len > 0)
        g_string_append (title, " / ");
      g_string_append (title, name);

      g_free (name);
    }

  bobgui_window_set_decorated (BOBGUI_WINDOW (window), decorated);
  bobgui_window_set_resizable (BOBGUI_WINDOW (window), decorated);

  bobgui_window_set_title (BOBGUI_WINDOW (window), title->str);
  g_string_free (title, TRUE);

  sw = bobgui_scrolled_window_new ();
  bobgui_scrolled_window_set_propagate_natural_width (BOBGUI_SCROLLED_WINDOW (sw), TRUE);
  bobgui_scrolled_window_set_propagate_natural_height (BOBGUI_SCROLLED_WINDOW (sw), TRUE);

  bobgui_window_set_child (BOBGUI_WINDOW (window), sw);

  box = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);

  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (sw), box);

  for (int i = 0; i < g_strv_length (filenames); i++)
    {
      BobguiSvg *svg;
      BobguiWidget *picture;
      BobguiEventController *click;
      BobguiEventController *shortcuts;
      BobguiShortcutTrigger *trigger;
      BobguiShortcutAction *action;

      svg = load_animation_file (filenames[i], features);

      bobgui_svg_set_frame_clock (svg, bobgui_widget_get_frame_clock (window));

      bobgui_svg_play (svg);

      if (size == 0)
        {
          picture = bobgui_picture_new_for_paintable (GDK_PAINTABLE (svg));
          bobgui_picture_set_can_shrink (BOBGUI_PICTURE (picture), FALSE);
          bobgui_picture_set_content_fit (BOBGUI_PICTURE (picture), BOBGUI_CONTENT_FIT_CONTAIN);
          bobgui_widget_set_hexpand (picture, TRUE);
          bobgui_widget_set_vexpand (picture, TRUE);
        }
      else
        {
          picture = bobgui_image_new_from_paintable (GDK_PAINTABLE (svg));
          bobgui_image_set_pixel_size (BOBGUI_IMAGE (picture), size);
        }

      g_object_set_data_full (G_OBJECT (picture), "filename", g_strdup (filenames[i]), g_free);

      click = BOBGUI_EVENT_CONTROLLER (bobgui_gesture_click_new ());
      bobgui_gesture_single_set_button (BOBGUI_GESTURE_SINGLE (click), 0);
      g_signal_connect (click, "pressed", G_CALLBACK (clicked), svg);
      bobgui_widget_add_controller (picture, click);

      shortcuts = bobgui_shortcut_controller_new ();
      bobgui_shortcut_controller_set_scope (BOBGUI_SHORTCUT_CONTROLLER (shortcuts), BOBGUI_SHORTCUT_SCOPE_GLOBAL);

      trigger = bobgui_alternative_trigger_new (
                  bobgui_keyval_trigger_new (GDK_KEY_AudioPlay, 0),
                  bobgui_keyval_trigger_new (GDK_KEY_P, GDK_CONTROL_MASK));
      action = bobgui_callback_action_new (toggle_playing, NULL, NULL);
      bobgui_shortcut_controller_add_shortcut (BOBGUI_SHORTCUT_CONTROLLER (shortcuts), bobgui_shortcut_new (trigger, action));
      trigger = bobgui_alternative_trigger_new (
                  bobgui_keyval_trigger_new (GDK_KEY_AudioRewind, 0),
                  bobgui_keyval_trigger_new (GDK_KEY_R, GDK_CONTROL_MASK));
      action = bobgui_callback_action_new (restart, NULL, NULL);
      bobgui_shortcut_controller_add_shortcut (BOBGUI_SHORTCUT_CONTROLLER (shortcuts), bobgui_shortcut_new (trigger, action));
      bobgui_widget_add_controller (picture, shortcuts);

      if (i > 0)
        bobgui_box_append (BOBGUI_BOX (box), bobgui_separator_new (BOBGUI_ORIENTATION_VERTICAL));
      bobgui_box_append (BOBGUI_BOX (box), picture);

      update_tooltip (picture, bobgui_svg_get_state (svg));

      g_object_unref (svg);
    }

  bobgui_window_present (BOBGUI_WINDOW (window));

  while (!done)
    g_main_context_iteration (NULL, TRUE);
}

void
do_play (int          *argc,
         const char ***argv)
{
  GOptionContext *context;
  char **filenames = NULL;
  gboolean decorated = TRUE;
  int size = 0;
  gboolean allow_animations = TRUE;
  gboolean allow_system = TRUE;
  gboolean allow_external = TRUE;
  gboolean allow_extensions = TRUE;
  gboolean traditional_symbolic = FALSE;
  const GOptionEntry entries[] = {
    { "undecorated", 0, G_OPTION_FLAG_REVERSE, G_OPTION_ARG_NONE, &decorated, N_("Don't add a titlebar"), NULL },
    { "size", 0, 0, G_OPTION_ARG_INT, &size, N_("SIZE") },
    { "no-animations", 0, G_OPTION_FLAG_REVERSE, G_OPTION_ARG_NONE, &allow_animations, N_("Don't run animations"), NULL },
    { "no-system-resources", 0, G_OPTION_FLAG_REVERSE, G_OPTION_ARG_NONE, &allow_system, N_("Don't use system resources"), NULL },
    { "no-external-resources", 0, G_OPTION_FLAG_REVERSE, G_OPTION_ARG_NONE, &allow_external, N_("Don't load external resources"), NULL },
    { "no-extensions", 0, G_OPTION_FLAG_REVERSE, G_OPTION_ARG_NONE, &allow_extensions, N_("Don't allow gpa extensions"), NULL },
    { "traditional-symbolic", 0, 0, G_OPTION_ARG_NONE, &traditional_symbolic, N_("Traditional symbolic icon"), NULL },
    { G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_FILENAME_ARRAY, &filenames, NULL, N_("FILE…") },
    { NULL, }
  };
  GError *error = NULL;
  BobguiSvgFeatures features;

  g_set_prgname ("bobgui4-image-tool play");
  context = g_option_context_new (NULL);
  g_option_context_set_translation_domain (context, GETTEXT_PACKAGE);
  g_option_context_add_main_entries (context, entries, NULL);
  g_option_context_set_summary (context, _("Show one or more animations."));

  if (!g_option_context_parse (context, argc, (char ***)argv, &error))
    {
      g_printerr ("%s\n", error->message);
      g_error_free (error);
      exit (1);
    }

  g_option_context_free (context);

  if (filenames == NULL)
    {
      g_printerr (_("No animation file specified\n"));
      exit (1);
    }

  features = (allow_animations ? BOBGUI_SVG_ANIMATIONS : 0) |
             (allow_system ? BOBGUI_SVG_SYSTEM_RESOURCES : 0) |
             (allow_external ? BOBGUI_SVG_EXTERNAL_RESOURCES : 0) |
             (allow_extensions ? BOBGUI_SVG_EXTENSIONS : 0) |
             (traditional_symbolic ? BOBGUI_SVG_TRADITIONAL_SYMBOLIC : 0);

  show_files (filenames, decorated, size, features);

  g_strfreev (filenames);
}
