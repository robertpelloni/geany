/*
 * Copyright (c) 2026 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#include "svg.h"
#include "window.h"

#include "bobguialertdialog.h"
#include "bobguibinlayout.h"
#include "bobguidialogerror.h"
#include "bobguifiledialog.h"
#include "bobguistack.h"
#include "svg/bobguisvgprivate.h"
#include "bobguitooltip.h"
#include "bobguitypebuiltins.h"
#include "bobguitextview.h"
#include "bobguitextbuffer.h"

struct _BobguiInspectorSvg
{
  BobguiWidget parent;

  GObject *object;
  BobguiTextView *xml_view;
  BobguiTextBuffer *xml_buffer;

  guint timeout;
  GList *errors;
};

typedef struct _BobguiInspectorSvgClass
{
  BobguiWidgetClass parent_class;
} BobguiInspectorSvgClass;

G_DEFINE_TYPE (BobguiInspectorSvg, bobgui_inspector_svg, BOBGUI_TYPE_WIDGET)

typedef struct
{
  GError *error;
  BobguiTextIter start;
  BobguiTextIter end;
} SvgError;

static void
svg_error_free (gpointer data)
{
  SvgError *error = data;

  g_error_free (error->error);
  g_free (error);
}

typedef struct
{
  BobguiInspectorSvg *self;
  const char *text;
} ErrorData;

static void
error_cb (BobguiSvg   *svg,
          GError   *error,
          gpointer  data)
{
/* Without GLib 2.88, we don't get usable location
 * information from GMarkup, so don't try to highlight
 * errors
 */
#if GLIB_CHECK_VERSION (2, 88, 0)
  ErrorData *d = data;
  BobguiInspectorSvg *self = d->self;
  SvgError *svg_error;
  size_t offset;
  const BobguiSvgLocation *start, *end;
  const char *tag;

  if (error->domain != BOBGUI_SVG_ERROR)
    return;

  start = bobgui_svg_error_get_start (error);
  end = bobgui_svg_error_get_end (error);

  svg_error = g_new (SvgError, 1);
  svg_error->error = g_error_copy (error);

  offset = g_utf8_pointer_to_offset (d->text, d->text + start->bytes);
  bobgui_text_buffer_get_iter_at_offset (self->xml_buffer, &svg_error->start, offset);
  offset = g_utf8_pointer_to_offset (d->text, d->text + end->bytes);
  bobgui_text_buffer_get_iter_at_offset (self->xml_buffer, &svg_error->end, offset);

  self->errors = g_list_append (self->errors, svg_error);

  if (bobgui_text_iter_equal (&svg_error->start, &svg_error->end))
    bobgui_text_iter_forward_chars (&svg_error->end, 1);

  if (error->code == BOBGUI_SVG_ERROR_IGNORED_ELEMENT)
    tag = "ignored";
  else if (error->code == BOBGUI_SVG_ERROR_NOT_IMPLEMENTED)
    tag = "unimplemented";
  else
    tag = "error";

  bobgui_text_buffer_apply_tag_by_name (self->xml_buffer,
                                     tag,
                                     &svg_error->start,
                                     &svg_error->end);
#endif
}

static void
update_timeout (gpointer data)
{
  BobguiInspectorSvg *self = data;
  BobguiSvg *svg = BOBGUI_SVG (self->object);
  BobguiTextIter start, end;
  char *text;
  GBytes *bytes = NULL;
  gulong handler;
  ErrorData d;

  self->timeout = 0;

  bobgui_text_buffer_get_bounds (self->xml_buffer, &start, &end);
  bobgui_text_buffer_remove_all_tags (self->xml_buffer, &start, &end);

  text = bobgui_text_buffer_get_text (self->xml_buffer, &start, &end, FALSE);
  bytes = g_bytes_new_take (text, strlen (text));

  g_list_free_full (self->errors, svg_error_free);
  self->errors = NULL;

  d.self = self;
  d.text = text;

  handler = g_signal_connect (svg, "error", G_CALLBACK (error_cb), &d);
  bobgui_svg_load_from_bytes (svg, bytes);
  g_signal_handler_disconnect (svg, handler);
}

static void
xml_changed (BobguiInspectorSvg *self)
{
  if (self->timeout != 0)
    g_source_remove (self->timeout);
  self->timeout = g_timeout_add_once (100, update_timeout, self);
}

static void
update_xml (BobguiInspectorSvg *self)
{
  BobguiSvg *svg = BOBGUI_SVG (self->object);
  GBytes *xml = bobgui_svg_serialize (svg);

  g_signal_handlers_block_by_func (self->xml_buffer, xml_changed, self);
  bobgui_text_buffer_set_text (self->xml_buffer,
                            g_bytes_get_data (xml, NULL),
                            g_bytes_get_size (xml));
  update_timeout (self);
  g_signal_handlers_unblock_by_func (self->xml_buffer, xml_changed, self);
  g_bytes_unref (xml);
}

static void
save_to_file (BobguiInspectorSvg *self,
              GFile           *file)
{
  BobguiTextIter start, end;
  char *text;
  GError *error = NULL;

  bobgui_text_buffer_get_bounds (self->xml_buffer, &start, &end);
  text = bobgui_text_buffer_get_text (self->xml_buffer, &start, &end, FALSE);

  if (!g_file_replace_contents (file,
                                text, strlen (text),
                                NULL,
                                FALSE,
                                0,
                                NULL,
                                NULL,
                                &error))
    {
      BobguiAlertDialog *alert;

      alert = bobgui_alert_dialog_new ("This did not work");
      bobgui_alert_dialog_set_detail (alert, error->message);
      bobgui_alert_dialog_show (alert, BOBGUI_WINDOW (bobgui_widget_get_root (BOBGUI_WIDGET (self))));
      g_object_unref (alert);
      g_error_free (error);
    }

  g_free (text);
}

static void
save_cb (GObject      *source,
         GAsyncResult *result,
         gpointer      data)
{
  BobguiFileDialog *dialog = BOBGUI_FILE_DIALOG (source);
  BobguiInspectorSvg *self = data;
  GError *error = NULL;
  GFile *file;

  file = bobgui_file_dialog_save_finish (dialog, result, &error);
  if (!file)
    {
      if (g_error_matches (error, BOBGUI_DIALOG_ERROR, BOBGUI_DIALOG_ERROR_FAILED))
        g_print ("%s", error->message);
      g_error_free (error);
      return;
    }

  save_to_file (self, file);

  g_object_unref (file);
}

static void
save_as (BobguiWidget  *widget,
         const char *action_name,
         GVariant   *parameter)
{
  BobguiFileDialog *dialog;

  dialog = bobgui_file_dialog_new ();

  bobgui_file_dialog_set_title (dialog, "Save SVG");
  bobgui_file_dialog_set_modal (dialog, TRUE);
  bobgui_file_dialog_set_initial_name (dialog, "saved.svg");
  bobgui_file_dialog_save (dialog,
                        BOBGUI_WINDOW (bobgui_widget_get_root (widget)),
                        NULL,
                        save_cb,
                        widget);
  g_object_unref (dialog);
}

static void
copy_clipboard (BobguiWidget  *widget,
                const char *action_name,
                GVariant   *parameter)
{
  BobguiInspectorSvg *self = BOBGUI_INSPECTOR_SVG (widget);
  GdkClipboard *clipboard;
  BobguiTextIter start, end;
  char *text;

  bobgui_text_buffer_get_bounds (self->xml_buffer, &start, &end);
  text = bobgui_text_buffer_get_text (self->xml_buffer, &start, &end, FALSE);

  clipboard = bobgui_widget_get_clipboard (widget);
  gdk_clipboard_set_text (clipboard, text);
  g_free (text);
}

static gboolean
query_tooltip_cb (BobguiWidget       *widget,
                  int              x,
                  int              y,
                  gboolean         keyboard_tip,
                  BobguiTooltip      *tooltip,
                  BobguiInspectorSvg *self)
{
  BobguiTextIter iter;

  if (!BOBGUI_IS_SVG (self->object))
    return FALSE;

  if (keyboard_tip)
    {
      int offset;

      g_object_get (self->xml_view, "cursor-position", &offset, NULL);
      bobgui_text_buffer_get_iter_at_offset (self->xml_buffer, &iter, offset);
    }
  else
    {
      int bx, by, trailing;

      bobgui_text_view_window_to_buffer_coords (self->xml_view,
                                             BOBGUI_TEXT_WINDOW_TEXT,
                                             x, y, &bx, &by);
      bobgui_text_view_get_iter_at_position (self->xml_view, &iter, &trailing, bx, by);
    }

  for (GList *l = self->errors; l; l = l->next)
    {
      SvgError *error = l->data;

      if (bobgui_text_iter_in_range (&iter, &error->start, &error->end))
        {
          bobgui_tooltip_set_text (tooltip, error->error->message);
          return TRUE;
        }
    }

  return FALSE;
}

void
bobgui_inspector_svg_set_object (BobguiInspectorSvg *self,
                              GObject         *object)
{
  BobguiWidget *stack;
  BobguiStackPage *page;

  g_set_object (&self->object, object);

  stack = bobgui_widget_get_parent (BOBGUI_WIDGET (self));
  page = bobgui_stack_get_page (BOBGUI_STACK (stack), BOBGUI_WIDGET (self));

  if (BOBGUI_IS_SVG (self->object))
    {
      bobgui_stack_page_set_visible (page, TRUE);
      update_xml (self);
    }
  else
    {
      bobgui_stack_page_set_visible (page, FALSE);
    }
}

static void
bobgui_inspector_svg_init (BobguiInspectorSvg *self)
{
  bobgui_widget_init_template (BOBGUI_WIDGET (self));
}

static void
dispose (GObject *o)
{
  BobguiInspectorSvg *self = BOBGUI_INSPECTOR_SVG (o);

  g_list_free_full (self->errors, svg_error_free);
  self->errors = NULL;

  if (self->timeout)
    {
      g_source_remove (self->timeout);
      self->timeout = 0;
    }

  g_clear_object (&self->object);

  bobgui_widget_dispose_template (BOBGUI_WIDGET (o), BOBGUI_TYPE_INSPECTOR_SVG);

  G_OBJECT_CLASS (bobgui_inspector_svg_parent_class)->dispose (o);
}

static void
bobgui_inspector_svg_class_init (BobguiInspectorSvgClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);

  object_class->dispose = dispose;

  bobgui_widget_class_set_template_from_resource (widget_class, "/org/bobgui/libbobgui/inspector/svg.ui");
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorSvg, xml_view);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorSvg, xml_buffer);
  bobgui_widget_class_bind_template_callback (widget_class, xml_changed);
  bobgui_widget_class_bind_template_callback (widget_class, query_tooltip_cb);

  bobgui_widget_class_set_layout_manager_type (widget_class, BOBGUI_TYPE_BIN_LAYOUT);

  bobgui_widget_class_install_action (widget_class, "widget.save-as", NULL, save_as);
  bobgui_widget_class_install_action (widget_class, "widget.copy-clipboard", NULL, copy_clipboard);
}

// vim: set et sw=2 ts=2:
