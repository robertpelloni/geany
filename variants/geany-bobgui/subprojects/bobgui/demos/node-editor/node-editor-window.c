/*
 * Copyright © 2019 Benjamin Otte
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Benjamin Otte <otte@gnome.org>
 */

#include "config.h"

#include "node-editor-window.h"

#include "bobguirendererpaintableprivate.h"

#include "gsk/gskrendernodeparserprivate.h"
#ifdef GDK_WINDOWING_BROADWAY
#include "gsk/broadway/gskbroadwayrenderer.h"
#endif

#include <glib/gstdio.h>

#include <cairo.h>
#ifdef CAIRO_HAS_SVG_SURFACE
#include <cairo-svg.h>
#endif

/* Hardcode zoom levels that fractional scaling is likely to use.
 * Be aware that people using this might have fractional
 * scaling enabled themselves. and might want to view stuff at device scale */
static float zoom_levels[] = {
  /* TINY */ 1.0 / 8.0,
  /*  25% */ 1.0 / 4.0,
  /*  33% */ 1.0 / 3.0,
  /*  40% */ 2.0 / 5.0,
  /*  50% */ 1.0 / 2.0,
  /*  67% */ 2.0 / 3.0,
  /*  80% */ 4.0 / 5.0,
  /* d'oh */ 1.0,
  /* 125% */ 5.0 / 4.0,
  /* 150% */ 3.0 / 2.0,
  /* 175% */ 7.0 / 4.0,
  /* 200% */ 2.0,
  /* 250% */ 5.0 / 2.0,
  /* 300% */ 3.0,
  /* 400% */ 4.0,
  /* HUGE */ 8.0
};

typedef struct
{
  gsize  start_chars;
  gsize  end_chars;
  char  *message;
} TextViewError;

struct _NodeEditorWindow
{
  BobguiApplicationWindow parent;

  BobguiWidget *picture;
  BobguiWidget *text_view;
  BobguiTextBuffer *text_buffer;
  BobguiTextTagTable *tag_table;

  BobguiWidget *testcase_popover;
  BobguiWidget *testcase_error_label;
  BobguiWidget *testcase_cairo_checkbutton;
  BobguiWidget *testcase_name_entry;
  BobguiWidget *testcase_save_button;
  BobguiWidget *zoom_in;
  BobguiWidget *zoom_out;
  BobguiWidget *zoom_label;
  BobguiWidget *crash_warning;

  BobguiWidget *renderer_listbox;
  GListStore *renderers;
  GskRenderNode *node;

  GFile *file;
  GFileMonitor *file_monitor;

  GArray *errors;

  gboolean auto_reload;
  gboolean mark_as_safe_pending;
  gulong after_paint_handler;

  int zoom_level;
  gboolean dark_mode;
  int paned_position;
};

struct _NodeEditorWindowClass
{
  BobguiApplicationWindowClass parent_class;
};

enum {
  PROP_AUTO_RELOAD = 1,
  PROP_ZOOM_LEVEL,
  PROP_DARK_MODE,
  PROP_PANED_POSITION,
  NUM_PROPERTIES
};

static GParamSpec *properties[NUM_PROPERTIES] = { NULL, };

G_DEFINE_TYPE(NodeEditorWindow, node_editor_window, BOBGUI_TYPE_APPLICATION_WINDOW);

static void
text_view_error_free (TextViewError *e)
{
  g_free (e->message);
}

static char *
get_current_text (BobguiTextBuffer *buffer)
{
  BobguiTextIter start, end;

  bobgui_text_buffer_get_start_iter (buffer, &start);
  bobgui_text_buffer_get_end_iter (buffer, &end);

  return bobgui_text_buffer_get_text (buffer, &start, &end, FALSE);
}

static void
text_buffer_remove_all_tags (BobguiTextBuffer *buffer)
{
  BobguiTextIter start, end;

  bobgui_text_buffer_get_start_iter (buffer, &start);
  bobgui_text_buffer_get_end_iter (buffer, &end);
  bobgui_text_buffer_remove_all_tags (buffer, &start, &end);
}

static void
deserialize_error_func (const GskParseLocation *start_location,
                        const GskParseLocation *end_location,
                        const GError           *error,
                        gpointer                user_data)
{
  NodeEditorWindow *self = user_data;
  BobguiTextIter start_iter, end_iter;
  TextViewError text_view_error;

  bobgui_text_buffer_get_iter_at_line_offset (self->text_buffer, &start_iter,
                                           start_location->lines,
                                           start_location->line_chars);
  bobgui_text_buffer_get_iter_at_line_offset (self->text_buffer, &end_iter,
                                           end_location->lines,
                                           end_location->line_chars);

  bobgui_text_buffer_apply_tag_by_name (self->text_buffer, "error",
                                     &start_iter, &end_iter);

  text_view_error.start_chars = start_location->chars;
  text_view_error.end_chars = end_location->chars;
  text_view_error.message = g_strdup (error->message);
  g_array_append_val (self->errors, text_view_error);
}

static void
text_iter_skip_alpha_backward (BobguiTextIter *iter)
{
  /* Just skip to the previous non-whitespace char */

  while (!bobgui_text_iter_is_start (iter))
    {
      gunichar c = bobgui_text_iter_get_char (iter);

      if (g_unichar_isspace (c))
        {
          bobgui_text_iter_forward_char (iter);
          break;
        }

      bobgui_text_iter_backward_char (iter);
    }
}

static void
text_iter_skip_whitespace_backward (BobguiTextIter *iter)
{
  while (!bobgui_text_iter_is_start (iter))
    {
      gunichar c = bobgui_text_iter_get_char (iter);

      if (g_unichar_isalpha (c))
        {
          bobgui_text_iter_forward_char (iter);
          break;
        }

      bobgui_text_iter_backward_char (iter);
    }
}

static void
highlight_text (NodeEditorWindow *self)
{
  BobguiTextIter iter;
  BobguiTextIter start, end;

  bobgui_text_buffer_get_start_iter (self->text_buffer, &iter);

  while (!bobgui_text_iter_is_end (&iter))
    {
      gunichar c = bobgui_text_iter_get_char (&iter);

      if (c == '/')
        {
          BobguiTextIter comment_start = iter;
          bobgui_text_iter_forward_char (&iter);

          c = bobgui_text_iter_get_char (&iter);
          if (c == '*')
            {
              bobgui_text_iter_forward_char (&iter);
              while (!bobgui_text_iter_is_end (&iter))
                {
                  c = bobgui_text_iter_get_char (&iter);
                  bobgui_text_iter_forward_char (&iter);

                  if (c == '*')
                    {
                      c = bobgui_text_iter_get_char (&iter);
                      bobgui_text_iter_forward_char (&iter);
                      if (c == '/')
                        break;
                    }
                }
            }

          bobgui_text_buffer_apply_tag_by_name (self->text_buffer, "comment", &comment_start, &iter);
        }
      else if (c == '{')
        {
          BobguiTextIter word_end = iter;
          BobguiTextIter word_start;

          bobgui_text_iter_backward_char (&word_end);
          text_iter_skip_whitespace_backward (&word_end);

          word_start = word_end;
          bobgui_text_iter_backward_word_start (&word_start);
          text_iter_skip_alpha_backward (&word_start);

          bobgui_text_buffer_apply_tag_by_name (self->text_buffer, "nodename", &word_start, &word_end);
        }
      else if (c == ':')
        {
          BobguiTextIter word_end = iter;
          BobguiTextIter word_start;

          bobgui_text_iter_backward_char (&word_end);
          text_iter_skip_whitespace_backward (&word_end);

          word_start = word_end;
          bobgui_text_iter_backward_word_start (&word_start);
          text_iter_skip_alpha_backward (&word_start);

          bobgui_text_buffer_apply_tag_by_name (self->text_buffer, "propname", &word_start, &word_end);
        }
      else if (c == '"')
        {
          BobguiTextIter string_start = iter;
          BobguiTextIter string_end = iter;

          bobgui_text_iter_forward_char (&iter);
          while (!bobgui_text_iter_is_end (&iter))
            {
              c = bobgui_text_iter_get_char (&iter);

              if (c == '"')
                {
                  bobgui_text_iter_forward_char (&iter);
                  string_end = iter;
                  break;
                }

              bobgui_text_iter_forward_char (&iter);
            }

          bobgui_text_buffer_apply_tag_by_name (self->text_buffer, "string", &string_start, &string_end);
        }

      bobgui_text_iter_forward_char (&iter);
    }

  bobgui_text_buffer_get_bounds (self->text_buffer, &start, &end);
  bobgui_text_buffer_apply_tag_by_name (self->text_buffer, "no-hyphens", &start, &end);
}

static void
mark_autosave_as_unsafe (void)
{
  char *path1 = NULL;
  char *path2 = NULL;

  path1 = get_autosave_path ("-unsafe");
  path2 = get_autosave_path (NULL);

  g_rename (path2, path1);

  g_free (path1);
  g_free (path2);
}

static void
mark_autosave_as_safe (void)
{
  char *path1 = NULL;
  char *path2 = NULL;

  path1 = get_autosave_path ("-unsafe");
  path2 = get_autosave_path (NULL);

  g_rename (path1, path2);

  g_free (path1);
  g_free (path2);
}

static void
after_paint (GdkFrameClock    *clock,
             NodeEditorWindow *self)
{
  if (self->mark_as_safe_pending)
    {
      self->mark_as_safe_pending = FALSE;
      mark_autosave_as_safe ();
    }
}

static void
reload (NodeEditorWindow *self)
{
  char *text;
  GBytes *bytes;
  GskRenderNode *big_node;

  mark_autosave_as_unsafe ();

  text = get_current_text (self->text_buffer);
  bytes = g_bytes_new_take (text, strlen (text));

  g_clear_pointer (&self->node, gsk_render_node_unref);

  /* If this is too slow, go fix the parser performance */
  self->node = gsk_render_node_deserialize (bytes, deserialize_error_func, self);

  if (self->node && self->zoom_level != 0)
    {
      GskTransform *transform;
      float scale;

      scale = zoom_levels [self->zoom_level];

      transform = gsk_transform_scale (NULL, scale, scale);
      big_node = gsk_transform_node_new (self->node, transform);

      gsk_transform_unref (transform);
    }
  else if (self->node)
    {
      big_node = gsk_render_node_ref (self->node);
    }
  else
    {
      big_node = NULL;
    }

  g_bytes_unref (bytes);
  if (self->node)
    {
      /* XXX: Is this code necessary or can we have API to turn nodes into paintables? */
      BobguiSnapshot *snapshot;
      GdkPaintable *paintable;
      graphene_rect_t bounds;
      guint i;

      snapshot = bobgui_snapshot_new ();
      gsk_render_node_get_bounds (big_node, &bounds);
      bobgui_snapshot_translate (snapshot, &GRAPHENE_POINT_INIT (- bounds.origin.x, - bounds.origin.y));
      bobgui_snapshot_append_node (snapshot, big_node);
      paintable = bobgui_snapshot_free_to_paintable (snapshot, &bounds.size);
      bobgui_picture_set_paintable (BOBGUI_PICTURE (self->picture), paintable);
      g_clear_object (&paintable);

      snapshot = bobgui_snapshot_new ();
      gsk_render_node_get_bounds (self->node, &bounds);
      bobgui_snapshot_translate (snapshot, &GRAPHENE_POINT_INIT (- bounds.origin.x, - bounds.origin.y));
      bobgui_snapshot_append_node (snapshot, self->node);
      paintable = bobgui_snapshot_free_to_paintable (snapshot, &bounds.size);

      for (i = 0; i < g_list_model_get_n_items (G_LIST_MODEL (self->renderers)); i++)
        {
          gpointer item = g_list_model_get_item (G_LIST_MODEL (self->renderers), i);
          bobgui_renderer_paintable_set_paintable (item, paintable);
          g_object_unref (item);
        }

      g_clear_object (&paintable);
    }
  else
    {
      bobgui_picture_set_paintable (BOBGUI_PICTURE (self->picture), NULL);
    }

  g_clear_pointer (&big_node, gsk_render_node_unref);

  self->mark_as_safe_pending = TRUE;
}

static void
text_changed (BobguiTextBuffer    *buffer,
              NodeEditorWindow *self)
{
  g_array_remove_range (self->errors, 0, self->errors->len);
  text_buffer_remove_all_tags (self->text_buffer);

  if (self->auto_reload)
    reload (self);

  highlight_text (self);
}

static gboolean
text_view_query_tooltip_cb (BobguiWidget        *widget,
                            int               x,
                            int               y,
                            gboolean          keyboard_tip,
                            BobguiTooltip       *tooltip,
                            NodeEditorWindow *self)
{
  BobguiTextIter iter;
  guint i;
  GString *text;

  if (keyboard_tip)
    {
      int offset;

      g_object_get (self->text_buffer, "cursor-position", &offset, NULL);
      bobgui_text_buffer_get_iter_at_offset (self->text_buffer, &iter, offset);
    }
  else
    {
      int bx, by, trailing;

      bobgui_text_view_window_to_buffer_coords (BOBGUI_TEXT_VIEW (self->text_view), BOBGUI_TEXT_WINDOW_TEXT,
                                             x, y, &bx, &by);
      bobgui_text_view_get_iter_at_position (BOBGUI_TEXT_VIEW (self->text_view), &iter, &trailing, bx, by);
    }

  text = g_string_new ("");

  for (i = 0; i < self->errors->len; i ++)
    {
      const TextViewError *e = &g_array_index (self->errors, TextViewError, i);
      BobguiTextIter start_iter, end_iter;

      bobgui_text_buffer_get_iter_at_offset (self->text_buffer, &start_iter, e->start_chars);
      bobgui_text_buffer_get_iter_at_offset (self->text_buffer, &end_iter, e->end_chars);

      if (bobgui_text_iter_in_range (&iter, &start_iter, &end_iter))
        {
          if (text->len > 0)
            g_string_append (text, "\n");
          g_string_append (text, e->message);
        }
    }

  if (text->len > 0)
    {
      bobgui_tooltip_set_text (tooltip, text->str);
      g_string_free (text, TRUE);
      return TRUE;
    }
  else
    {
      g_string_free (text, TRUE);
      return FALSE;
    }
}

static gboolean
load_bytes (NodeEditorWindow *self,
            GBytes           *bytes);

static void
load_error (NodeEditorWindow *self,
            const char        *error_message)
{
  PangoLayout *layout;
  BobguiSnapshot *snapshot;
  GskRenderNode *node;
  GBytes *bytes;

  layout = bobgui_widget_create_pango_layout (BOBGUI_WIDGET (self), error_message);
  pango_layout_set_width (layout, 300 * PANGO_SCALE);
  snapshot = bobgui_snapshot_new ();
  bobgui_snapshot_append_layout (snapshot, layout, &(GdkRGBA) { 0.7, 0.13, 0.13, 1.0 });
  node = bobgui_snapshot_free_to_node (snapshot);
  bytes = gsk_render_node_serialize (node);

  load_bytes (self, bytes);

  gsk_render_node_unref (node);
  g_object_unref (layout);
}

static gboolean
load_bytes (NodeEditorWindow *self,
            GBytes           *bytes)
{
  if (!g_utf8_validate (g_bytes_get_data (bytes, NULL), g_bytes_get_size (bytes), NULL))
    {
      load_error (self, "Invalid UTF-8");
      g_bytes_unref (bytes);
      return FALSE;
    }

  bobgui_text_buffer_set_text (self->text_buffer,
                            g_bytes_get_data (bytes, NULL),
                            g_bytes_get_size (bytes));

  g_bytes_unref (bytes);

  return TRUE;
}

static gboolean
load_file_contents (NodeEditorWindow *self,
                    GFile            *file)
{
  GError *error = NULL;
  GBytes *bytes;

  bytes = g_file_load_bytes (file, NULL, NULL, &error);
  if (bytes == NULL)
    {
      load_error (self, error->message);
      g_clear_error (&error);
      return FALSE;
    }

  return load_bytes (self, bytes);
}

static GdkContentProvider *
on_picture_drag_prepare_cb (BobguiDragSource    *source,
                            double            x,
                            double            y,
                            NodeEditorWindow *self)
{
  if (self->node == NULL)
    return NULL;

  return gdk_content_provider_new_typed (GSK_TYPE_RENDER_NODE, self->node);
}

static void
on_picture_drop_read_done_cb (GObject      *source,
                              GAsyncResult *res,
                              gpointer      data)
{
  NodeEditorWindow *self = data;
  GOutputStream *stream = G_OUTPUT_STREAM (source);
  GdkDrop *drop = g_object_get_data (source, "drop");
  GdkDragAction action = 0;
  GBytes *bytes;

  if (g_output_stream_splice_finish (stream, res, NULL) >= 0)
    {
      bytes = g_memory_output_stream_steal_as_bytes (G_MEMORY_OUTPUT_STREAM (stream));
      if (load_bytes (self, bytes))
        action = GDK_ACTION_COPY;
    }

  g_object_unref (self);
  gdk_drop_finish (drop, action);
  g_object_unref (drop);
  return;
}

static void
on_picture_drop_read_cb (GObject      *source,
                         GAsyncResult *res,
                         gpointer      data)
{
  NodeEditorWindow *self = data;
  GdkDrop *drop = GDK_DROP (source);
  GInputStream *input;
  GOutputStream *output;

  input = gdk_drop_read_finish (drop, res, NULL, NULL);
  if (input == NULL)
    {
      g_object_unref (self);
      gdk_drop_finish (drop, GDK_ACTION_NONE);
      return;
    }

  output = g_memory_output_stream_new_resizable ();
  g_object_set_data (G_OBJECT (output), "drop", drop);
  g_object_ref (drop);

  g_output_stream_splice_async (output,
                                input,
                                G_OUTPUT_STREAM_SPLICE_CLOSE_SOURCE | G_OUTPUT_STREAM_SPLICE_CLOSE_TARGET,
                                G_PRIORITY_DEFAULT,
                                NULL,
                                on_picture_drop_read_done_cb,
                                self);
  g_object_unref (output);
  g_object_unref (input);
}

static gboolean
on_picture_drop_cb (BobguiDropTargetAsync *dest,
                    GdkDrop            *drop,
                    double              x,
                    double              y,
                    NodeEditorWindow   *self)
{
  gdk_drop_read_async (drop,
                       (const char *[2]) { "application/x-bobgui-render-node", NULL },
                       G_PRIORITY_DEFAULT,
                       NULL,
                       on_picture_drop_read_cb,
                       g_object_ref (self));

  return TRUE;
}

static void
file_changed_cb (GFileMonitor      *monitor,
                 GFile             *file,
                 GFile             *other_file,
                 GFileMonitorEvent  event_type,
                 gpointer           user_data)
{
  NodeEditorWindow *self = user_data;

  if (event_type == G_FILE_MONITOR_EVENT_CHANGED)
    load_file_contents (self, file);
}

gboolean
node_editor_window_load (NodeEditorWindow *self,
                         GFile            *file)
{
  GError *error = NULL;

  g_clear_object (&self->file);
  g_clear_object (&self->file_monitor);

  if (!load_file_contents (self, file))
    return FALSE;

  self->file = g_object_ref (file);
  self->file_monitor = g_file_monitor_file (self->file, G_FILE_MONITOR_NONE, NULL, &error);

  if (error)
    {
      g_warning ("couldn't monitor file: %s", error->message);
      g_error_free (error);
      g_clear_object (&self->file_monitor);
    }
  else
    {
      g_signal_connect (self->file_monitor, "changed", G_CALLBACK (file_changed_cb), self);
    }

  return TRUE;
}

static void
open_response_cb (GObject *source,
                  GAsyncResult *result,
                  void *user_data)
{
  BobguiFileDialog *dialog = BOBGUI_FILE_DIALOG (source);
  NodeEditorWindow *self = user_data;
  GFile *file;

  file = bobgui_file_dialog_open_finish (dialog, result, NULL);
  if (file)
    {
      node_editor_window_load (self, file);
      g_object_unref (file);
    }
}

static void
show_open_filechooser (NodeEditorWindow *self)
{
  BobguiFileDialog *dialog;

  dialog = bobgui_file_dialog_new ();
  bobgui_file_dialog_set_title (dialog, "Open node file");
  if (self->file)
    {
      bobgui_file_dialog_set_initial_file (dialog, self->file);
    }
  else
    {
      GFile *cwd;
      cwd = g_file_new_for_path (".");
      bobgui_file_dialog_set_initial_folder (dialog, cwd);
      g_object_unref (cwd);
    }

  bobgui_file_dialog_open (dialog, BOBGUI_WINDOW (self),
                        NULL, open_response_cb, self);
  g_object_unref (dialog);
}

static void
open_cb (BobguiWidget        *button,
         NodeEditorWindow *self)
{
  show_open_filechooser (self);
}

static void
save_response_cb (GObject *source,
                  GAsyncResult *result,
                  void *user_data)
{
  BobguiFileDialog *dialog = BOBGUI_FILE_DIALOG (source);
  NodeEditorWindow *self = user_data;
  GFile *file;

  file = bobgui_file_dialog_save_finish (dialog, result, NULL);
  if (file)
    {
      char *text;
      GError *error = NULL;

      text = get_current_text (self->text_buffer);

      g_file_replace_contents (file, text, strlen (text),
                               NULL, FALSE,
                               G_FILE_CREATE_NONE,
                               NULL,
                               NULL,
                               &error);
      if (error != NULL)
        {
          BobguiAlertDialog *alert;

          alert = bobgui_alert_dialog_new ("Saving failed");
          bobgui_alert_dialog_set_detail (alert, error->message);
          bobgui_alert_dialog_show (alert,
                                 BOBGUI_WINDOW (bobgui_widget_get_root (BOBGUI_WIDGET (self))));
          g_object_unref (alert);
          g_error_free (error);
        }

      g_free (text);
      g_object_unref (file);
    }
}

static void
save_cb (BobguiWidget        *button,
         NodeEditorWindow *self)
{
  BobguiFileDialog *dialog;

  dialog = bobgui_file_dialog_new ();
  bobgui_file_dialog_set_title (dialog, "Save node");
  if (self->file)
    {
      bobgui_file_dialog_set_initial_file (dialog, self->file);
    }
  else
    {
      GFile *cwd = g_file_new_for_path (".");
      bobgui_file_dialog_set_initial_folder (dialog, cwd);
      bobgui_file_dialog_set_initial_name (dialog, "demo.node");
      g_object_unref (cwd);
    }

  bobgui_file_dialog_save (dialog,
                        BOBGUI_WINDOW (bobgui_widget_get_root (BOBGUI_WIDGET (button))),
                        NULL,
                        save_response_cb, self);
  g_object_unref (dialog);
}

static GskRenderNode *
create_node (NodeEditorWindow *self)
{
  GdkPaintable *paintable;
  BobguiSnapshot *snapshot;
  GskRenderNode *node;

  paintable = bobgui_picture_get_paintable (BOBGUI_PICTURE (self->picture));
  if (paintable == NULL ||
      gdk_paintable_get_intrinsic_width (paintable) <= 0 ||
      gdk_paintable_get_intrinsic_height (paintable) <= 0)
    return NULL;

  snapshot = bobgui_snapshot_new ();
  gdk_paintable_snapshot (paintable, snapshot, gdk_paintable_get_intrinsic_width (paintable), gdk_paintable_get_intrinsic_height (paintable));
  node = bobgui_snapshot_free_to_node (snapshot);

  return node;
}

static GdkTexture *
create_texture (NodeEditorWindow *self)
{
  GskRenderer *renderer;
  GskRenderNode *node;
  GdkTexture *texture;

  node = create_node (self);
  if (node == NULL)
    return NULL;

  renderer = bobgui_native_get_renderer (bobgui_widget_get_native (BOBGUI_WIDGET (self)));
  texture = gsk_renderer_render_texture (renderer, node, NULL);
  gsk_render_node_unref (node);

  return texture;
}

#ifdef CAIRO_HAS_SVG_SURFACE
static cairo_status_t
cairo_serializer_write (gpointer             user_data,
                        const unsigned char *data,
                        unsigned int         length)
{
  g_byte_array_append (user_data, data, length);

  return CAIRO_STATUS_SUCCESS;
}

static GBytes *
create_svg (GskRenderNode  *node,
            GError        **error)
{
  cairo_surface_t *surface;
  cairo_t *cr;
  graphene_rect_t bounds;
  GByteArray *array;

  gsk_render_node_get_bounds (node, &bounds);
  array = g_byte_array_new ();

  surface = cairo_svg_surface_create_for_stream (cairo_serializer_write,
                                                 array,
                                                 bounds.size.width,
                                                 bounds.size.height);
  cairo_svg_surface_set_document_unit (surface, CAIRO_SVG_UNIT_PX);
  cairo_surface_set_device_offset (surface, -bounds.origin.x, -bounds.origin.y);

  cr = cairo_create (surface);
  gsk_render_node_draw (node, cr);
  cairo_destroy (cr);

  cairo_surface_finish (surface);
  if (cairo_surface_status (surface) == CAIRO_STATUS_SUCCESS)
    {
      cairo_surface_destroy (surface);
      return g_byte_array_free_to_bytes (array);
    }
  else
    {
      g_set_error (error,
                   G_IO_ERROR, G_IO_ERROR_FAILED,
                   "%s", cairo_status_to_string (cairo_surface_status (surface)));
      cairo_surface_destroy (surface);
      g_byte_array_unref (array);
      return NULL;
    }
}
#endif

static GdkTexture *
create_cairo_texture (NodeEditorWindow *self)
{
  GdkPaintable *paintable;
  BobguiSnapshot *snapshot;
  GskRenderer *renderer;
  GskRenderNode *node;
  GdkTexture *texture;

  paintable = bobgui_picture_get_paintable (BOBGUI_PICTURE (self->picture));
  if (paintable == NULL ||
      gdk_paintable_get_intrinsic_width (paintable) <= 0 ||
      gdk_paintable_get_intrinsic_height (paintable) <= 0)
    return NULL;
  snapshot = bobgui_snapshot_new ();
  gdk_paintable_snapshot (paintable, snapshot, gdk_paintable_get_intrinsic_width (paintable), gdk_paintable_get_intrinsic_height (paintable));
  node = bobgui_snapshot_free_to_node (snapshot);
  if (node == NULL)
    return NULL;

  renderer = gsk_cairo_renderer_new ();
  gsk_renderer_realize_for_display (renderer, bobgui_widget_get_display (BOBGUI_WIDGET (self)), NULL);

  texture = gsk_renderer_render_texture (renderer, node, NULL);
  gsk_render_node_unref (node);
  gsk_renderer_unrealize (renderer);
  g_object_unref (renderer);

  return texture;
}

static void
export_image_saved_cb (GObject      *source,
                       GAsyncResult *result,
                       void         *user_data)
{
  GError *error = NULL;

  if (!g_file_replace_contents_finish (G_FILE (source), result, NULL, &error))
    {
      BobguiAlertDialog *alert;

      alert = bobgui_alert_dialog_new ("Exporting to image failed");
      bobgui_alert_dialog_set_detail (alert, error->message);
      bobgui_alert_dialog_show (alert, NULL);
      g_object_unref (alert);
      g_clear_error (&error);
    }
}

static void
export_image_response_cb (GObject      *source,
                          GAsyncResult *result,
                          void         *user_data)
{
  BobguiFileDialog *dialog = BOBGUI_FILE_DIALOG (source);
  GskRenderNode *node = user_data;
  GFile *file;
  char *uri;
  GBytes *bytes;

  file = bobgui_file_dialog_save_finish (dialog, result, NULL);
  if (file == NULL)
    {
      gsk_render_node_unref (node);
      return;
    }

  uri = g_file_get_uri (file);
#ifdef CAIRO_HAS_SVG_SURFACE
  if (g_str_has_suffix (uri, "svg"))
    {
      GError *error = NULL;

      bytes = create_svg (node, &error);
      if (bytes == NULL)
        {
          BobguiAlertDialog *alert;

          alert = bobgui_alert_dialog_new ("Exporting to image failed");
          bobgui_alert_dialog_set_detail (alert, error->message);
          bobgui_alert_dialog_show (alert, NULL);
          g_object_unref (alert);
          g_clear_error (&error);
        }
    }
  else
#endif
    {
      GdkTexture *texture;
      GskRenderer *renderer;

      renderer = gsk_gl_renderer_new ();
      if (!gsk_renderer_realize_for_display (renderer, gdk_display_get_default (), NULL))
        {
          g_object_unref (renderer);
          renderer = gsk_cairo_renderer_new ();
          if (!gsk_renderer_realize_for_display (renderer, gdk_display_get_default (), NULL))
            {
              g_assert_not_reached ();
            }
        }
      texture = gsk_renderer_render_texture (renderer, node, NULL);
      gsk_renderer_unrealize (renderer);
      g_object_unref (renderer);

      if (g_str_has_suffix (uri, "tiff"))
        bytes = gdk_texture_save_to_tiff_bytes (texture);
      else
        bytes = gdk_texture_save_to_png_bytes (texture);
      g_object_unref (texture);
    }
  g_free (uri);

  if (bytes)
    {
      g_file_replace_contents_bytes_async (file,
                                           bytes,
                                           NULL,
                                           FALSE,
                                           0,
                                           NULL,
                                           export_image_saved_cb,
                                           NULL);
      g_bytes_unref (bytes);
    }
  gsk_render_node_unref (node);
  g_object_unref (file);
}

static void
export_image_cb (BobguiWidget        *button,
                 NodeEditorWindow *self)
{
  GskRenderNode *node;
  BobguiFileDialog *dialog;
  BobguiFileFilter *filter;
  GListStore *filters;

  node = create_node (self);
  if (node == NULL)
    return;

  filters = g_list_store_new (BOBGUI_TYPE_FILE_FILTER);
  filter = bobgui_file_filter_new ();
  bobgui_file_filter_add_mime_type (filter, "image/png");
  g_list_store_append (filters, filter);
  g_object_unref (filter);
  filter = bobgui_file_filter_new ();
  bobgui_file_filter_add_mime_type (filter, "image/svg+xml");
  g_list_store_append (filters, filter);
  g_object_unref (filter);
  filter = bobgui_file_filter_new ();
  bobgui_file_filter_add_mime_type (filter, "image/tiff");
  g_list_store_append (filters, filter);
  g_object_unref (filter);

  dialog = bobgui_file_dialog_new ();
  bobgui_file_dialog_set_title (dialog, "");
  bobgui_file_dialog_set_initial_name (dialog, "example.png");
  bobgui_file_dialog_set_filters (dialog, G_LIST_MODEL (filters));
  bobgui_file_dialog_save (dialog,
                        BOBGUI_WINDOW (bobgui_widget_get_root (BOBGUI_WIDGET (button))),
                        NULL,
                        export_image_response_cb, node);
  g_object_unref (filters);
  g_object_unref (dialog);
}


static void
clip_image_cb (BobguiWidget        *button,
               NodeEditorWindow *self)
{
  GdkTexture *texture;
  GdkClipboard *clipboard;

  texture = create_texture (self);
  if (texture == NULL)
    return;

  clipboard = bobgui_widget_get_clipboard (BOBGUI_WIDGET (self));

  gdk_clipboard_set_texture (clipboard, texture);

  g_object_unref (texture);
}

static void
testcase_name_entry_changed_cb (BobguiWidget        *button,
                                GParamSpec       *pspec,
                                NodeEditorWindow *self)

{
  const char *text = bobgui_editable_get_text (BOBGUI_EDITABLE (self->testcase_name_entry));

  if (strlen (text) > 0)
    bobgui_widget_set_sensitive (self->testcase_save_button, TRUE);
  else
    bobgui_widget_set_sensitive (self->testcase_save_button, FALSE);
}

/* Returns the location where gsk test cases are stored in
 * the BOBGUI testsuite, if we can determine it.
 *
 * When running node editor outside of a BOBGUI build, you can
 * set BOBGUI_SOURCE_DIR to point it at the checkout.
 */
static char *
get_source_dir (void)
{
  const char *subdir = "testsuite/gsk/compare";
  const char *source_dir;
  char *current_dir;
  char *dir;

  source_dir = g_getenv ("BOBGUI_SOURCE_DIR");
  current_dir = g_get_current_dir ();

  if (source_dir)
    {
      char *abs_source_dir = g_canonicalize_filename (source_dir, NULL);
      dir = g_canonicalize_filename (subdir, abs_source_dir);
      g_free (abs_source_dir);
    }
  else
    {
      dir = g_canonicalize_filename (subdir, current_dir);
    }

  if (g_file_test (dir, G_FILE_TEST_EXISTS))
    {
      g_free (current_dir);

      return dir;
    }

  g_free (dir);

  return current_dir;
}

static void
testcase_save_clicked_cb (BobguiWidget        *button,
                          NodeEditorWindow *self)
{
  const char *testcase_name = bobgui_editable_get_text (BOBGUI_EDITABLE (self->testcase_name_entry));
  char *source_dir = get_source_dir ();
  char *node_file_name;
  char *node_file;
  char *png_file_name;
  char *png_file;
  char *text = NULL;
  GdkTexture *texture;
  GError *error = NULL;

  node_file_name = g_strconcat (testcase_name, ".node", NULL);
  node_file = g_build_filename (source_dir, node_file_name, NULL);
  g_free (node_file_name);

  g_debug ("Saving testcase in %s", node_file);

  png_file_name = g_strconcat (testcase_name, ".png", NULL);
  png_file = g_build_filename (source_dir, png_file_name, NULL);
  g_free (png_file_name);

  if (bobgui_check_button_get_active (BOBGUI_CHECK_BUTTON (self->testcase_cairo_checkbutton)))
    texture = create_cairo_texture (self);
  else
    texture = create_texture (self);

  if (!gdk_texture_save_to_png (texture, png_file))
    {
      bobgui_label_set_label (BOBGUI_LABEL (self->testcase_error_label),
                           "Could not save texture file");
      goto out;
    }

  text = get_current_text (self->text_buffer);
  {
    GBytes *bytes;
    GskRenderNode *node;
    gsize size;

    bytes = g_bytes_new_take (text, strlen (text) + 1);
    node = gsk_render_node_deserialize (bytes, NULL, NULL);
    g_bytes_unref (bytes);
    bytes = gsk_render_node_serialize (node);
    gsk_render_node_unref (node);
    text = g_bytes_unref_to_data (bytes, &size);
  }

  if (!g_file_set_contents (node_file, text, -1, &error))
    {
      bobgui_label_set_label (BOBGUI_LABEL (self->testcase_error_label), error->message);
      /* TODO: Remove texture file again? */
      goto out;
    }

  bobgui_editable_set_text (BOBGUI_EDITABLE (self->testcase_name_entry), "");
  bobgui_popover_popdown (BOBGUI_POPOVER (self->testcase_popover));

out:
  g_free (text);
  g_free (png_file);
  g_free (node_file);
  g_free (source_dir);
}

static void
set_dark_mode (NodeEditorWindow *self,
               gboolean          dark_mode)
{
  BobguiSettings *settings;

  if (self->dark_mode == dark_mode)
    return;

  self->dark_mode = dark_mode;

  settings = bobgui_widget_get_settings (BOBGUI_WIDGET (self));

  if (dark_mode)
    {
      BobguiInterfaceColorScheme color_scheme;

      g_object_get (settings, "bobgui-interface-color-scheme", &color_scheme, NULL);
      if (color_scheme == BOBGUI_INTERFACE_COLOR_SCHEME_DARK)
        color_scheme = BOBGUI_INTERFACE_COLOR_SCHEME_LIGHT;
      else
        color_scheme = BOBGUI_INTERFACE_COLOR_SCHEME_DARK;
      g_object_set (settings, "bobgui-interface-color-scheme", color_scheme, NULL);
    }
  else
    bobgui_settings_reset_property (settings, "bobgui-interface-color-scheme");

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_DARK_MODE]);
}

static void
node_editor_window_dispose (GObject *object)
{
  bobgui_widget_dispose_template (BOBGUI_WIDGET (object), NODE_EDITOR_WINDOW_TYPE);

  G_OBJECT_CLASS (node_editor_window_parent_class)->dispose (object);
}

static void
node_editor_window_finalize (GObject *object)
{
  NodeEditorWindow *self = (NodeEditorWindow *)object;

  g_array_free (self->errors, TRUE);

  g_clear_pointer (&self->node, gsk_render_node_unref);
  g_clear_object (&self->renderers);
  g_clear_object (&self->file_monitor);
  g_clear_object (&self->file);

  G_OBJECT_CLASS (node_editor_window_parent_class)->finalize (object);
}

static void
node_editor_window_add_renderer (NodeEditorWindow *self,
                                 GskRenderer      *renderer,
                                 const char       *description)
{
  GdkPaintable *paintable;
  GdkDisplay *display;

  display = bobgui_widget_get_display (BOBGUI_WIDGET (self));

  if (!gsk_renderer_realize_for_display (renderer, display, NULL))
    {
      GdkSurface *surface = bobgui_native_get_surface (BOBGUI_NATIVE (self));
      g_assert (surface != NULL);

      if (!gsk_renderer_realize (renderer, surface, NULL))
        {
          g_object_unref (renderer);
          return;
        }
    }

  paintable = bobgui_renderer_paintable_new (renderer, bobgui_picture_get_paintable (BOBGUI_PICTURE (self->picture)));
  g_object_set_data_full (G_OBJECT (paintable), "description", g_strdup (description), g_free);
  g_clear_object (&renderer);

  g_list_store_append (self->renderers, paintable);
  g_object_unref (paintable);
}

static void
update_paste_action (GdkClipboard *clipboard,
                     GParamSpec   *pspec,
                     gpointer      data)
{
  BobguiWidget *widget = BOBGUI_WIDGET (data);
  gboolean has_node;

  has_node = gdk_content_formats_contain_mime_type (gdk_clipboard_get_formats (clipboard), "application/x-bobgui-render-node");

  bobgui_widget_action_set_enabled (widget, "paste-node", has_node);
}

static void
node_editor_window_realize (BobguiWidget *widget)
{
  NodeEditorWindow *self = NODE_EDITOR_WINDOW (widget);
  GdkFrameClock *frameclock;

  BOBGUI_WIDGET_CLASS (node_editor_window_parent_class)->realize (widget);

#if 0
  node_editor_window_add_renderer (self,
                                   NULL,
                                   "Default");
#endif
  node_editor_window_add_renderer (self,
                                   gsk_gl_renderer_new (),
                                   "OpenGL");
  node_editor_window_add_renderer (self,
                                   gsk_vulkan_renderer_new (),
                                   "Vulkan");
#ifdef GDK_WINDOWING_BROADWAY
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  node_editor_window_add_renderer (self,
                                   gsk_broadway_renderer_new (),
                                   "Broadway");
G_GNUC_END_IGNORE_DEPRECATIONS
#endif
  node_editor_window_add_renderer (self,
                                   gsk_cairo_renderer_new (),
                                   "Cairo");

  frameclock = bobgui_widget_get_frame_clock (widget);
  self->after_paint_handler = g_signal_connect (frameclock, "after-paint",
                                                G_CALLBACK (after_paint), self);

  g_signal_connect (bobgui_widget_get_clipboard (widget), "notify::formats", G_CALLBACK (update_paste_action), widget);
}

static void
node_editor_window_unrealize (BobguiWidget *widget)
{
  NodeEditorWindow *self = NODE_EDITOR_WINDOW (widget);
  GdkFrameClock *frameclock;
  guint i;

  g_signal_handlers_disconnect_by_func (bobgui_widget_get_clipboard (widget), update_paste_action, widget);

  frameclock = bobgui_widget_get_frame_clock (widget);
  g_signal_handler_disconnect (frameclock, self->after_paint_handler);
  self->after_paint_handler = 0;

  for (i = 0; i < g_list_model_get_n_items (G_LIST_MODEL (self->renderers)); i ++)
    {
      gpointer item = g_list_model_get_item (G_LIST_MODEL (self->renderers), i);
      gsk_renderer_unrealize (bobgui_renderer_paintable_get_renderer (item));
      g_object_unref (item);
    }

  g_list_store_remove_all (self->renderers);

  BOBGUI_WIDGET_CLASS (node_editor_window_parent_class)->unrealize (widget);
}

typedef struct
{
  NodeEditorWindow *self;
  BobguiTextIter start, end;
} Selection;

static void
color_cb (GObject *source,
          GAsyncResult *result,
          gpointer data)
{
  BobguiColorDialog *dialog = BOBGUI_COLOR_DIALOG (source);
  Selection *selection = data;
  NodeEditorWindow *self = selection->self;
  GdkRGBA *color;
  char *text;
  GError *error = NULL;
  BobguiTextBuffer *buffer;

  color = bobgui_color_dialog_choose_rgba_finish (dialog, result, &error);
  if (!color)
    {
      g_print ("%s\n", error->message);
      g_error_free (error);
      g_free (selection);
      return;
    }

  buffer = bobgui_text_view_get_buffer (BOBGUI_TEXT_VIEW (self->text_view));

  text = gdk_rgba_to_string (color);
  bobgui_text_buffer_delete (buffer, &selection->start, &selection->end);
  bobgui_text_buffer_insert (buffer, &selection->start, text, -1);

  g_free (text);
  gdk_rgba_free (color);
  g_free (selection);
}

static void
font_cb (GObject *source,
          GAsyncResult *result,
          gpointer data)
{
  BobguiFontDialog *dialog = BOBGUI_FONT_DIALOG (source);
  Selection *selection = data;
  NodeEditorWindow *self = selection->self;
  GError *error = NULL;
  PangoFontDescription *desc;
  BobguiTextBuffer *buffer;
  char *text;

  desc = bobgui_font_dialog_choose_font_finish (dialog, result, &error);
  if (!desc)
    {
      g_print ("%s\n", error->message);
      g_error_free (error);
      g_free (selection);
      return;
    }

  buffer = bobgui_text_view_get_buffer (BOBGUI_TEXT_VIEW (self->text_view));

  text = pango_font_description_to_string (desc);
  bobgui_text_buffer_delete (buffer, &selection->start, &selection->end);
  bobgui_text_buffer_insert (buffer, &selection->start, text, -1);

  g_free (text);
  pango_font_description_free (desc);
  g_free (selection);
}

static void
file_cb (GObject *source,
          GAsyncResult *result,
          gpointer data)
{
  BobguiFileDialog *dialog = BOBGUI_FILE_DIALOG (source);
  Selection *selection = data;
  NodeEditorWindow *self = selection->self;
  GError *error = NULL;
  GFile *file;
  BobguiTextBuffer *buffer;
  char *text;

  file = bobgui_file_dialog_open_finish (dialog, result, &error);
  if (!file)
    {
      g_print ("%s\n", error->message);
      g_error_free (error);
      g_free (selection);
      return;
    }

  buffer = bobgui_text_view_get_buffer (BOBGUI_TEXT_VIEW (self->text_view));

  text = g_file_get_uri (file);
  bobgui_text_buffer_delete (buffer, &selection->start, &selection->end);
  bobgui_text_buffer_insert (buffer, &selection->start, text, -1);

  g_free (text);
  g_object_unref (file);
  g_free (selection);
}

static void
key_pressed (BobguiEventControllerKey *controller,
             unsigned int keyval,
             unsigned int keycode,
             GdkModifierType state,
             gpointer data)
{
  BobguiWidget *dd = bobgui_event_controller_get_widget (BOBGUI_EVENT_CONTROLLER (controller));
  Selection *selection = data;
  NodeEditorWindow *self = selection->self;
  unsigned int selected;
  BobguiStringList *strings;
  BobguiTextBuffer *buffer;
  const char *text;

  if (keyval != GDK_KEY_Escape)
    return;

  strings = BOBGUI_STRING_LIST (bobgui_drop_down_get_model (BOBGUI_DROP_DOWN (dd)));
  selected = bobgui_drop_down_get_selected (BOBGUI_DROP_DOWN (dd));
  text = bobgui_string_list_get_string (strings, selected);

  bobgui_text_view_remove (BOBGUI_TEXT_VIEW (self->text_view), dd);

  buffer = bobgui_text_view_get_buffer (BOBGUI_TEXT_VIEW (self->text_view));
  bobgui_text_iter_backward_search (&selection->start, "mode:", 0, NULL, &selection->start, NULL);
  bobgui_text_iter_forward_search (&selection->start, ";", 0, &selection->end, NULL, NULL);
  bobgui_text_buffer_delete (buffer, &selection->start, &selection->end);
  bobgui_text_buffer_insert (buffer, &selection->start, " ", -1);
  bobgui_text_buffer_insert (buffer, &selection->start, text, -1);
}

static void
node_editor_window_edit (NodeEditorWindow *self,
                         BobguiTextIter      *iter)
{
  BobguiTextIter start, end;
  BobguiTextBuffer *buffer;
  Selection *selection;

  buffer = bobgui_text_view_get_buffer (BOBGUI_TEXT_VIEW (self->text_view));

  bobgui_text_iter_set_line_offset (iter, 0);

  if (bobgui_text_iter_forward_search (iter, ";", 0, &end, NULL, NULL) &&
      bobgui_text_iter_forward_search (iter, "color:", 0, NULL, &start, &end))
    {
      BobguiColorDialog *dialog;
      GdkRGBA color;
      char *text;

      while (g_unichar_isspace (bobgui_text_iter_get_char (&start)))
        bobgui_text_iter_forward_char (&start);

      bobgui_text_buffer_select_range (buffer, &start, &end);
      text = bobgui_text_buffer_get_text (buffer, &start, &end, TRUE);
      gdk_rgba_parse (&color, text);
      g_free (text);

      selection = g_new0 (Selection, 1);
      selection->self = self;
      selection->start = start;
      selection->end = end;

      dialog = bobgui_color_dialog_new ();
      bobgui_color_dialog_choose_rgba (dialog, BOBGUI_WINDOW (self), &color, NULL, color_cb, selection);
    }
  else if (bobgui_text_iter_forward_search (iter, ";", 0, &end, NULL, NULL) &&
           bobgui_text_iter_forward_search (iter, "font:", 0, NULL, &start, &end))
    {
      BobguiFontDialog *dialog;
      PangoFontDescription *desc;
      char *text;

      while (g_unichar_isspace (bobgui_text_iter_get_char (&start)))
        bobgui_text_iter_forward_char (&start);

      /* Skip the quotes */
      bobgui_text_iter_forward_char (&start);
      bobgui_text_iter_backward_char (&end);

      bobgui_text_buffer_select_range (buffer, &start, &end);

      text = bobgui_text_buffer_get_text (buffer, &start, &end, TRUE);
      desc = pango_font_description_from_string (text);
      g_free (text);

      selection = g_new0 (Selection, 1);
      selection->self = self;
      selection->start = start;
      selection->end = end;

      dialog = bobgui_font_dialog_new ();
      bobgui_font_dialog_choose_font (dialog, BOBGUI_WINDOW (self), desc, NULL, font_cb, selection);
      pango_font_description_free (desc);
    }
  else if (bobgui_text_iter_forward_search (iter, ";", 0, &end, NULL, NULL) &&
           bobgui_text_iter_forward_search (iter, "mode:", 0, NULL, &start, &end))
    {
      /* Assume we have a blend node, for now */
      GEnumClass *class;
      BobguiStringList *strings;
      BobguiWidget *dd;
      BobguiTextChildAnchor *anchor;
      unsigned int selected = 0;
      BobguiEventController *key_controller;
      gboolean is_blend_mode = FALSE;
      char *text;

      while (g_unichar_isspace (bobgui_text_iter_get_char (&start)))
        bobgui_text_iter_forward_char (&start);

      text = bobgui_text_buffer_get_text (buffer, &start, &end, TRUE);

      strings = bobgui_string_list_new (NULL);
      class = g_type_class_ref (GSK_TYPE_BLEND_MODE);
      for (unsigned int i = 0; i < class->n_values; i++)
        {
          if (strcmp (class->values[i].value_nick, text) == 0)
            is_blend_mode = TRUE;
        }
      g_type_class_unref (class);

      if (is_blend_mode)
        class = g_type_class_ref (GSK_TYPE_BLEND_MODE);
      else
        class = g_type_class_ref (GSK_TYPE_MASK_MODE);

       for (unsigned int i = 0; i < class->n_values; i++)
         {
           if (i == 0 && is_blend_mode)
             bobgui_string_list_append (strings, "normal");
           else
             bobgui_string_list_append (strings, class->values[i].value_nick);

           if (strcmp (class->values[i].value_nick, text) == 0)
             selected = i;
         }
      g_type_class_unref (class);

      bobgui_text_buffer_delete (buffer, &start, &end);

      anchor = bobgui_text_buffer_create_child_anchor (buffer, &start);
      dd = bobgui_drop_down_new (G_LIST_MODEL (strings), NULL);
      bobgui_drop_down_set_selected (BOBGUI_DROP_DOWN (dd), selected);
      bobgui_text_view_add_child_at_anchor (BOBGUI_TEXT_VIEW (self->text_view), dd, anchor);

      selection = g_new0 (Selection, 1);
      selection->self = self;
      selection->start = start;
      selection->end = end;

      key_controller = bobgui_event_controller_key_new ();
      g_signal_connect (key_controller, "key-pressed", G_CALLBACK (key_pressed), selection);
      bobgui_widget_add_controller (dd, key_controller);
    }
  else if (bobgui_text_iter_forward_search (iter, ";", 0, &end, NULL, NULL) &&
           bobgui_text_iter_forward_search (iter, "texture:", 0, NULL, &start, &end))
    {
      BobguiFileDialog *dialog;
      BobguiTextIter skip;
      char *text;
      GFile *file;

      while (g_unichar_isspace (bobgui_text_iter_get_char (&start)))
        bobgui_text_iter_forward_char (&start);

      skip = start;
      bobgui_text_iter_forward_chars (&skip, strlen ("url(\""));
      text = bobgui_text_iter_get_text (&start, &skip);
      if (strcmp (text, "url(\"") != 0)
        {
          g_free (text);
          return;
        }
      g_free (text);
      start = skip;

      skip = end;
      bobgui_text_iter_backward_chars (&skip, strlen ("\")"));
      text = bobgui_text_iter_get_text (&skip, &end);
      if (strcmp (text, "\")") != 0)
        {
          g_free (text);
          return;
        }
      g_free (text);
      end = skip;

      bobgui_text_buffer_select_range (buffer, &start, &end);

      text = bobgui_text_buffer_get_text (buffer, &start, &end, TRUE);
      file = g_file_new_for_uri (text);
      g_free (text);

      selection = g_new0 (Selection, 1);
      selection->self = self;
      selection->start = start;
      selection->end = end;

      dialog = bobgui_file_dialog_new ();
      bobgui_file_dialog_set_initial_file (dialog, file);
      bobgui_file_dialog_open (dialog, BOBGUI_WINDOW (self), NULL, file_cb, selection);
      g_object_unref (file);
    }
}

static void
click_gesture_pressed (BobguiGestureClick  *gesture,
                       int               n_press,
                       double            x,
                       double            y,
                       NodeEditorWindow *self)
{
  BobguiTextIter iter;
  int bx, by, trailing;
  GdkModifierType state;

  state = bobgui_event_controller_get_current_event_state (BOBGUI_EVENT_CONTROLLER (gesture));
  if ((state & GDK_CONTROL_MASK) == 0)
    return;

  bobgui_text_view_window_to_buffer_coords (BOBGUI_TEXT_VIEW (self->text_view), BOBGUI_TEXT_WINDOW_TEXT, x, y, &bx, &by);
  bobgui_text_view_get_iter_at_position (BOBGUI_TEXT_VIEW (self->text_view), &iter, &trailing, bx, by);

  node_editor_window_edit (self, &iter);
}

static void
edit_action_cb (BobguiWidget  *widget,
                const char *action_name,
                GVariant   *parameter)
{
  NodeEditorWindow *self = NODE_EDITOR_WINDOW (widget);
  BobguiTextBuffer *buffer;
  BobguiTextIter start, end;

  buffer = bobgui_text_view_get_buffer (BOBGUI_TEXT_VIEW (self->text_view));
  bobgui_text_buffer_get_selection_bounds (buffer, &start, &end);

  node_editor_window_edit (self, &start);
}

static void
text_received (GObject      *source,
               GAsyncResult *result,
               gpointer      data)
{
  GdkClipboard *clipboard = GDK_CLIPBOARD (source);
  NodeEditorWindow *self = NODE_EDITOR_WINDOW (data);
  char *text;

  text = gdk_clipboard_read_text_finish (clipboard, result, NULL);
  if (text)
    {
      BobguiTextBuffer *buffer;
      BobguiTextIter start, end;

      buffer = bobgui_text_view_get_buffer (BOBGUI_TEXT_VIEW (self->text_view));
      bobgui_text_buffer_begin_user_action (buffer);
      bobgui_text_buffer_get_bounds (buffer, &start, &end);
      bobgui_text_buffer_delete (buffer, &start, &end);
      bobgui_text_buffer_insert (buffer, &start, text, -1);
      bobgui_text_buffer_end_user_action (buffer);
      g_free (text);
    }
}

static void
paste_node_cb (BobguiWidget  *widget,
               const char *action_name,
               GVariant   *parameter)
{
  GdkClipboard *clipboard = bobgui_widget_get_clipboard (widget);

  gdk_clipboard_read_text_async (clipboard, NULL, text_received, widget);
}

static void set_zoom_level (NodeEditorWindow *self,
                            int               zoom_level);

static void
node_editor_window_set_property (GObject      *object,
                                 guint         prop_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  NodeEditorWindow *self = NODE_EDITOR_WINDOW (object);

  switch (prop_id)
    {
    case PROP_AUTO_RELOAD:
      {
        gboolean auto_reload = g_value_get_boolean (value);
        if (self->auto_reload != auto_reload)
          {
            self->auto_reload = auto_reload;

            if (self->auto_reload)
              reload (self);
          }
      }
      break;

    case PROP_ZOOM_LEVEL:
      set_zoom_level (self, g_value_get_int (value));
      break;

    case PROP_DARK_MODE:
      set_dark_mode (self, g_value_get_boolean (value));
      break;

    case PROP_PANED_POSITION:
      self->paned_position = g_value_get_int (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
node_editor_window_get_property (GObject    *object,
                                 guint       prop_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  NodeEditorWindow *self = NODE_EDITOR_WINDOW (object);

  switch (prop_id)
    {
    case PROP_AUTO_RELOAD:
      g_value_set_boolean (value, self->auto_reload);
      break;

    case PROP_ZOOM_LEVEL:
      g_value_set_int (value, self->zoom_level);
      break;

    case PROP_DARK_MODE:
      g_value_set_boolean (value, self->dark_mode);
      break;

    case PROP_PANED_POSITION:
      g_value_set_int (value, self->paned_position);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
close_crash_warning (BobguiButton        *button,
                     NodeEditorWindow *self)
{
  bobgui_revealer_set_reveal_child (BOBGUI_REVEALER (self->crash_warning), FALSE);
}

static void
update_zoom_buttons (NodeEditorWindow *self)
{
  GString *s = g_string_new ("");

  bobgui_widget_set_sensitive (self->zoom_in, self->zoom_level < G_N_ELEMENTS (zoom_levels) - 1);
  bobgui_widget_set_sensitive (self->zoom_out, self->zoom_level > 0);
  g_string_append_printf (s, "%3d%%", (guint) round (100 * zoom_levels[self->zoom_level]));
  g_string_replace (s, " ", " ", 0); /* Replace ASCII space by U+2007 figure space */
  bobgui_label_set_text (BOBGUI_LABEL (self->zoom_label), s->str);
  g_string_free (s, TRUE);
}

static void
set_zoom_level (NodeEditorWindow *self,
                int               zoom_level)
{
  zoom_level = CLAMP (zoom_level, 0, G_N_ELEMENTS (zoom_levels) - 1);

  if (self->zoom_level == zoom_level)
    return;

  self->zoom_level = zoom_level;
  update_zoom_buttons (self);
  text_changed (self->text_buffer, self);
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_ZOOM_LEVEL]);
}

static void
zoom_in_cb (BobguiButton        *button,
            NodeEditorWindow *self)
{
  set_zoom_level (self, self->zoom_level + 1);
}

static void
zoom_out_cb (BobguiButton        *button,
             NodeEditorWindow *self)
{
  set_zoom_level (self, self->zoom_level - 1);
}

static gboolean
node_editor_window_save_state (BobguiApplicationWindow *win,
                               GVariantDict         *state)
{
  NodeEditorWindow *self = NODE_EDITOR_WINDOW (win);

  g_variant_dict_insert (state, "zoom-level", "i", self->zoom_level);
  g_variant_dict_insert (state, "auto-reload", "b", self->auto_reload);
  g_variant_dict_insert (state, "dark-mode", "b", self->dark_mode);
  g_variant_dict_insert (state, "paned-position", "i", self->paned_position);

  return TRUE;
}

static void
node_editor_window_class_init (NodeEditorWindowClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (class);
  BobguiApplicationWindowClass *appwin_class = BOBGUI_APPLICATION_WINDOW_CLASS (class);

  BobguiShortcutTrigger *trigger;
  BobguiShortcutAction *action;
  BobguiShortcut *shortcut;

  object_class->dispose = node_editor_window_dispose;
  object_class->finalize = node_editor_window_finalize;
  object_class->set_property = node_editor_window_set_property;
  object_class->get_property = node_editor_window_get_property;

  appwin_class->save_state = node_editor_window_save_state;

  bobgui_widget_class_set_template_from_resource (widget_class,
                                               "/org/bobgui/bobgui4/node-editor/node-editor-window.ui");

  widget_class->realize = node_editor_window_realize;
  widget_class->unrealize = node_editor_window_unrealize;

  properties[PROP_AUTO_RELOAD] = g_param_spec_boolean ("auto-reload", NULL, NULL,
                                                       TRUE,
                                                       G_PARAM_READWRITE | G_PARAM_STATIC_NAME);

  properties[PROP_ZOOM_LEVEL] = g_param_spec_int ("zoom-level", NULL, NULL,
                                                  0, G_N_ELEMENTS (zoom_levels) - 1, 0,
                                                  G_PARAM_READWRITE | G_PARAM_STATIC_NAME);

  properties[PROP_DARK_MODE] = g_param_spec_boolean ("dark-mode", NULL, NULL,
                                                     FALSE,
                                                     G_PARAM_READWRITE | G_PARAM_STATIC_NAME);

  properties[PROP_PANED_POSITION] = g_param_spec_int ("paned-position", NULL, NULL,
                                                      0, G_MAXINT, 0,
                                                      G_PARAM_READWRITE | G_PARAM_STATIC_NAME);

  g_object_class_install_properties (object_class, NUM_PROPERTIES, properties);

  bobgui_widget_class_bind_template_child (widget_class, NodeEditorWindow, text_view);
  bobgui_widget_class_bind_template_child (widget_class, NodeEditorWindow, picture);
  bobgui_widget_class_bind_template_child (widget_class, NodeEditorWindow, renderer_listbox);
  bobgui_widget_class_bind_template_child (widget_class, NodeEditorWindow, testcase_popover);
  bobgui_widget_class_bind_template_child (widget_class, NodeEditorWindow, testcase_error_label);
  bobgui_widget_class_bind_template_child (widget_class, NodeEditorWindow, testcase_cairo_checkbutton);
  bobgui_widget_class_bind_template_child (widget_class, NodeEditorWindow, testcase_name_entry);
  bobgui_widget_class_bind_template_child (widget_class, NodeEditorWindow, testcase_save_button);
  bobgui_widget_class_bind_template_child (widget_class, NodeEditorWindow, crash_warning);
  bobgui_widget_class_bind_template_child (widget_class, NodeEditorWindow, zoom_in);
  bobgui_widget_class_bind_template_child (widget_class, NodeEditorWindow, zoom_out);
  bobgui_widget_class_bind_template_child (widget_class, NodeEditorWindow, zoom_label);

  bobgui_widget_class_bind_template_callback (widget_class, text_view_query_tooltip_cb);
  bobgui_widget_class_bind_template_callback (widget_class, open_cb);
  bobgui_widget_class_bind_template_callback (widget_class, save_cb);
  bobgui_widget_class_bind_template_callback (widget_class, export_image_cb);
  bobgui_widget_class_bind_template_callback (widget_class, clip_image_cb);
  bobgui_widget_class_bind_template_callback (widget_class, testcase_save_clicked_cb);
  bobgui_widget_class_bind_template_callback (widget_class, testcase_name_entry_changed_cb);
  bobgui_widget_class_bind_template_callback (widget_class, on_picture_drag_prepare_cb);
  bobgui_widget_class_bind_template_callback (widget_class, on_picture_drop_cb);
  bobgui_widget_class_bind_template_callback (widget_class, click_gesture_pressed);
  bobgui_widget_class_bind_template_callback (widget_class, close_crash_warning);
  bobgui_widget_class_bind_template_callback (widget_class, zoom_in_cb);
  bobgui_widget_class_bind_template_callback (widget_class, zoom_out_cb);

  bobgui_widget_class_install_action (widget_class, "smart-edit", NULL, edit_action_cb);

  trigger = bobgui_keyval_trigger_new (GDK_KEY_e, GDK_CONTROL_MASK);
  action = bobgui_named_action_new ("smart-edit");
  shortcut = bobgui_shortcut_new (trigger, action);
  bobgui_widget_class_add_shortcut (widget_class, shortcut);

  bobgui_widget_class_install_action (widget_class, "paste-node", NULL, paste_node_cb);

  trigger = bobgui_keyval_trigger_new (GDK_KEY_v, GDK_CONTROL_MASK | GDK_SHIFT_MASK);
  action = bobgui_named_action_new ("paste-node");
  shortcut = bobgui_shortcut_new (trigger, action);
  bobgui_widget_class_add_shortcut (widget_class, shortcut);
}

static BobguiWidget *
node_editor_window_create_renderer_widget (gpointer item,
                                           gpointer user_data)
{
  GdkPaintable *paintable = item;
  BobguiWidget *box, *label, *picture;
  BobguiWidget *row;

  box = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
  bobgui_widget_set_size_request (box, 120, 90);

  label = bobgui_label_new (g_object_get_data (G_OBJECT (paintable), "description"));
  bobgui_widget_add_css_class (label, "title-4");
  bobgui_box_append (BOBGUI_BOX (box), label);

  picture = bobgui_picture_new_for_paintable (paintable);
  /* don't ever scale up, we want to be as accurate as possible */
  bobgui_widget_set_halign (picture, BOBGUI_ALIGN_CENTER);
  bobgui_widget_set_valign (picture, BOBGUI_ALIGN_CENTER);
  bobgui_box_append (BOBGUI_BOX (box), picture);

  row = bobgui_list_box_row_new ();
  bobgui_list_box_row_set_child (BOBGUI_LIST_BOX_ROW (row), box);
  bobgui_list_box_row_set_activatable (BOBGUI_LIST_BOX_ROW (row), FALSE);

  return row;
}

static void
window_open (GSimpleAction *action,
             GVariant      *parameter,
             gpointer       user_data)
{
  NodeEditorWindow *self = user_data;

  show_open_filechooser (self);
}

static GActionEntry win_entries[] = {
  { "open", window_open, NULL, NULL, NULL },
};

char *
get_autosave_path (const char *suffix)
{
  char *path;
  char *name;

  name = g_strconcat ("autosave", suffix, NULL);
  path = g_build_filename (g_get_user_cache_dir (), "bobgui4-node-editor", name, NULL);
  g_free (name);

  return path;
}

static void
set_initial_text (NodeEditorWindow *self)
{
  char *path, *path1;
  char *initial_text;
  gsize len;

  path = get_autosave_path (NULL);
  path1 = get_autosave_path ("-unsafe");

  if (g_file_get_contents (path1, &initial_text, &len, NULL))
    {
      self->auto_reload = FALSE;
      bobgui_revealer_set_reveal_child (BOBGUI_REVEALER (self->crash_warning), TRUE);

      bobgui_text_buffer_set_text (self->text_buffer, initial_text, len);
      g_free (initial_text);
    }
  else if (g_file_get_contents (path, &initial_text, &len, NULL))
    {
      bobgui_text_buffer_set_text (self->text_buffer, initial_text, len);
      g_free (initial_text);
    }
  else
    {
      /* Default */
      bobgui_text_buffer_set_text (self->text_buffer,
         "shadow {\n"
         "  child: texture {\n"
         "    bounds: 0 0 128 128;\n"
         "    texture: url(\"resource:///org/bobgui/bobgui4/node-editor/icons/apps/org.bobgui.bobgui4.NodeEditor.svg\");\n"
         "  }\n"
         "  shadows: rgba(0,0,0,0.5) 0 1 12;\n"
         "}\n"
         "\n"
         "transform {\n"
         "  child: text {\n"
         "    color: rgb(46,52,54);\n"
         "    font: \"Sans Bold 14.6px\";\n"
         "    glyphs: \"BOBGUI Node Editor\";\n"
         "    offset: 8 14.418;\n"
         "  }\n"
         "  transform: translate(0, 140);\n"
         "}", -1);
    }

  g_free (path);
  g_free (path1);
}

static void
autosave_contents (NodeEditorWindow *self)
{
  char *path = NULL;
  char *dir = NULL;
  char *contents;
  BobguiTextIter start, end;

  bobgui_text_buffer_get_bounds (self->text_buffer, &start, &end);
  contents = bobgui_text_buffer_get_text (self->text_buffer, &start, &end, TRUE);
  path = get_autosave_path ("-unsafe");
  dir = g_path_get_dirname (path);
  g_mkdir_with_parents (dir, 0755);
  g_file_set_contents (path, contents, -1, NULL);

  g_free (dir);
  g_free (path);
  g_free (contents);
}

static void
node_editor_window_init (NodeEditorWindow *self)
{
  GAction *action;
  gsize i;

  bobgui_widget_init_template (BOBGUI_WIDGET (self));

  for (i = 0; i < G_N_ELEMENTS (zoom_levels); i++)
    {
      if (zoom_levels[i] == 1.0)
        {
          self->zoom_level = i;
          break;
        }
    }
  self->auto_reload = TRUE;

  self->renderers = g_list_store_new (GDK_TYPE_PAINTABLE);
  bobgui_list_box_bind_model (BOBGUI_LIST_BOX (self->renderer_listbox),
                           G_LIST_MODEL (self->renderers),
                           node_editor_window_create_renderer_widget,
                           self,
                           NULL);

  self->errors = g_array_new (FALSE, TRUE, sizeof (TextViewError));
  g_array_set_clear_func (self->errors, (GDestroyNotify)text_view_error_free);

  g_action_map_add_action_entries (G_ACTION_MAP (self), win_entries, G_N_ELEMENTS (win_entries), self);

  action = G_ACTION (g_property_action_new ("auto-reload", self, "auto-reload"));
  g_action_map_add_action (G_ACTION_MAP (self), action);
  g_object_unref (action);

  self->tag_table = bobgui_text_tag_table_new ();
  bobgui_text_tag_table_add (self->tag_table,
                          g_object_new (BOBGUI_TYPE_TEXT_TAG,
                                        "name", "error",
                                        "underline", PANGO_UNDERLINE_ERROR,
                                        NULL));
  bobgui_text_tag_table_add (self->tag_table,
                          g_object_new (BOBGUI_TYPE_TEXT_TAG,
                                        "name", "comment",
                                        "foreground-rgba", &(GdkRGBA) { 0.8, 0.52, 0.5, 1},
                                        NULL));
  bobgui_text_tag_table_add (self->tag_table,
                          g_object_new (BOBGUI_TYPE_TEXT_TAG,
                                        "name", "nodename",
                                        "foreground-rgba", &(GdkRGBA) { 0.9, 0.78, 0.53, 1},
                                        NULL));
  bobgui_text_tag_table_add (self->tag_table,
                          g_object_new (BOBGUI_TYPE_TEXT_TAG,
                                        "name", "propname",
                                        "foreground-rgba", &(GdkRGBA) { 0.7, 0.55, 0.67, 1},
                                        NULL));
  bobgui_text_tag_table_add (self->tag_table,
                          g_object_new (BOBGUI_TYPE_TEXT_TAG,
                                        "name", "string",
                                        "foreground-rgba", &(GdkRGBA) { 0.63, 0.73, 0.54, 1},
                                        NULL));
  bobgui_text_tag_table_add (self->tag_table,
                          g_object_new (BOBGUI_TYPE_TEXT_TAG,
                                        "name", "number",
                                        "foreground-rgba", &(GdkRGBA) { 0.8, 0.52, 0.43, 1},
                                        NULL));
  bobgui_text_tag_table_add (self->tag_table,
                          g_object_new (BOBGUI_TYPE_TEXT_TAG,
                                        "name", "no-hyphens",
                                        "insert-hyphens", FALSE,
                                        NULL));

  self->text_buffer = bobgui_text_buffer_new (self->tag_table);
  g_signal_connect (self->text_buffer, "changed", G_CALLBACK (text_changed), self);
  bobgui_text_view_set_buffer (BOBGUI_TEXT_VIEW (self->text_view), self->text_buffer);

  set_initial_text (self);

  g_signal_connect_swapped (self->text_buffer, "changed", G_CALLBACK (autosave_contents), self);

  if (g_getenv ("GSK_RENDERER"))
    {
      char *new_title = g_strdup_printf ("BOBGUI Node Editor - %s", g_getenv ("GSK_RENDERER"));
      bobgui_window_set_title (BOBGUI_WINDOW (self), new_title);
      g_free (new_title);
    }
}

NodeEditorWindow *
node_editor_window_new (NodeEditorApplication *application)
{
  return g_object_new (NODE_EDITOR_WINDOW_TYPE,
                       "application", application,
                       NULL);
}
