/* bobguiatspieditabletext.c: EditableText interface for BobguiAtspiContext
 *
 * Copyright 2020 Red Hat, Inc
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
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
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "bobguiatspieditabletextprivate.h"
#include "bobguiatcontextprivate.h"

#include "a11y/atspi/atspi-editabletext.h"

#include "bobguieditable.h"
#include "bobguientry.h"
#include "bobguisearchentry.h"
#include "bobguipasswordentry.h"
#include "bobguispinbutton.h"
#include "bobguitextview.h"

#include <gio/gio.h>

/* {{{ BobguiEditable */

typedef struct
{
  BobguiWidget *widget;
  int position;
} PasteData;

static void
text_received (GObject      *source,
               GAsyncResult *result,
               gpointer      data)
{
  GdkClipboard *clipboard = GDK_CLIPBOARD (source);
  PasteData *pdata = data;
  char *text;

  text = gdk_clipboard_read_text_finish (clipboard, result, NULL);
  if (text)
    bobgui_editable_insert_text (BOBGUI_EDITABLE (pdata->widget), text, -1, &pdata->position);
  g_free (text);
  g_free (pdata);
}

static void
editable_handle_method (GDBusConnection       *connection,
                        const gchar           *sender,
                        const gchar           *object_path,
                        const gchar           *interface_name,
                        const gchar           *method_name,
                        GVariant              *parameters,
                        GDBusMethodInvocation *invocation,
                        gpointer               user_data)
{
  BobguiATContext *self = user_data;
  BobguiAccessible *accessible = bobgui_at_context_get_accessible (self);
  BobguiWidget *widget = BOBGUI_WIDGET (accessible);

  if (g_strcmp0 (method_name, "SetTextContents") == 0)
    {
      char *text;
      gboolean ret = FALSE;

      g_variant_get (parameters, "(&s)", &text);

      if (bobgui_editable_get_editable (BOBGUI_EDITABLE (widget)))
        {
          bobgui_editable_set_text (BOBGUI_EDITABLE (widget), text);
          ret = TRUE;
        }

      g_dbus_method_invocation_return_value (invocation, g_variant_new ("(b)", ret));
    }
  else if (g_strcmp0 (method_name, "InsertText") == 0)
    {
      int position;
      char *text;
      int len;
      gboolean ret = FALSE;

      g_variant_get (parameters, "(i&si)", &position, &text, &len);

      if (bobgui_editable_get_editable (BOBGUI_EDITABLE (widget)))
        {
          bobgui_editable_insert_text (BOBGUI_EDITABLE (widget), text, -1, &position);
          ret = TRUE;
        }

      g_dbus_method_invocation_return_value (invocation, g_variant_new ("(b)", ret));
    }
  else if (g_strcmp0 (method_name, "CopyText") == 0)
    {
      int start, end;
      char *str;

      g_variant_get (parameters, "(ii)", &start, &end);

      str = bobgui_editable_get_chars (BOBGUI_EDITABLE (widget), start, end);
      gdk_clipboard_set_text (bobgui_widget_get_clipboard (widget), str);
      g_free (str);
      g_dbus_method_invocation_return_value (invocation, NULL);
    }
  else if (g_strcmp0 (method_name, "CutText") == 0)
    {
      int start, end;
      gboolean ret = FALSE;

      g_variant_get (parameters, "(ii)", &start, &end);

      if (bobgui_editable_get_editable (BOBGUI_EDITABLE (widget)))
        {
          char *str;

          str = bobgui_editable_get_chars (BOBGUI_EDITABLE (widget), start, end);
          gdk_clipboard_set_text (bobgui_widget_get_clipboard (widget), str);
          g_free (str);
          bobgui_editable_delete_text (BOBGUI_EDITABLE (widget), start, end);
          ret = TRUE;
        }

      g_dbus_method_invocation_return_value (invocation, g_variant_new ("(b)", ret));
    }
  else if (g_strcmp0 (method_name, "DeleteText") == 0)
    {
      int start, end;
      gboolean ret = FALSE;

      g_variant_get (parameters, "(ii)", &start, &end);

      if (bobgui_editable_get_editable (BOBGUI_EDITABLE (widget)))
        {
          bobgui_editable_delete_text (BOBGUI_EDITABLE (widget), start, end);
          ret = TRUE;
        }

      g_dbus_method_invocation_return_value (invocation, g_variant_new ("(b)", ret));
    }
  else if (g_strcmp0 (method_name, "PasteText") == 0)
    {
      int position;
      gboolean ret = FALSE;

      g_variant_get (parameters, "(i)", &position);

      if (bobgui_editable_get_editable (BOBGUI_EDITABLE (widget)))
        {
          PasteData *data;

          data = g_new (PasteData, 1);
          data->widget = widget;
          data->position = position;

          gdk_clipboard_read_text_async (bobgui_widget_get_clipboard (widget), NULL, text_received, data);

          ret = TRUE;
        }

      g_dbus_method_invocation_return_value (invocation, g_variant_new ("(b)", ret));
    }
}

static const GDBusInterfaceVTable editable_vtable = {
  editable_handle_method,
  NULL,
};

/* }}} */
/* {{{ BobguiTextView */

static void
text_view_received (GObject      *source,
                    GAsyncResult *result,
                    gpointer      data)
{
  GdkClipboard *clipboard = GDK_CLIPBOARD (source);
  PasteData *pdata = data;
  BobguiTextBuffer *buffer = bobgui_text_view_get_buffer (BOBGUI_TEXT_VIEW (pdata->widget));
  BobguiTextIter iter;
  char *text;

  text = gdk_clipboard_read_text_finish (clipboard, result, NULL);
  if (text)
    {
      bobgui_text_buffer_get_iter_at_offset (buffer, &iter, pdata->position);
      bobgui_text_buffer_insert (buffer, &iter, text, -1);
    }

  g_free (text);
  g_free (pdata);
}

static void
text_view_handle_method (GDBusConnection       *connection,
                         const gchar           *sender,
                         const gchar           *object_path,
                         const gchar           *interface_name,
                         const gchar           *method_name,
                         GVariant              *parameters,
                         GDBusMethodInvocation *invocation,
                         gpointer               user_data)
{
  BobguiATContext *self = user_data;
  BobguiAccessible *accessible = bobgui_at_context_get_accessible (self);
  BobguiWidget *widget = BOBGUI_WIDGET (accessible);

  if (g_strcmp0 (method_name, "SetTextContents") == 0)
    {
      BobguiTextBuffer *buffer = bobgui_text_view_get_buffer (BOBGUI_TEXT_VIEW (widget));
      char *text;
      gboolean ret = FALSE;

      g_variant_get (parameters, "(&s)", &text);

      if (bobgui_text_view_get_editable (BOBGUI_TEXT_VIEW (widget)))
        {
          bobgui_text_buffer_set_text (buffer, text, -1);
          ret = TRUE;
        }

      g_dbus_method_invocation_return_value (invocation, g_variant_new ("(b)", ret));
    }
  else if (g_strcmp0 (method_name, "InsertText") == 0)
    {
      BobguiTextBuffer *buffer = bobgui_text_view_get_buffer (BOBGUI_TEXT_VIEW (widget));
      BobguiTextIter iter;
      int position;
      char *text;
      int len;
      gboolean ret = FALSE;

      g_variant_get (parameters, "(i&si)", &position, &text, &len);

      if (bobgui_text_view_get_editable (BOBGUI_TEXT_VIEW (widget)))
        {
          bobgui_text_buffer_get_iter_at_offset (buffer, &iter, position);
          bobgui_text_buffer_insert (buffer, &iter, text, len);
          ret = TRUE;
        }

      g_dbus_method_invocation_return_value (invocation, g_variant_new ("(b)", ret));
    }
  else if (g_strcmp0 (method_name, "CopyText") == 0)
    {
      BobguiTextBuffer *buffer = bobgui_text_view_get_buffer (BOBGUI_TEXT_VIEW (widget));
      BobguiTextIter start_iter, end_iter;
      int start, end;
      char *str;

      g_variant_get (parameters, "(ii)", &start, &end);

      bobgui_text_buffer_get_iter_at_offset (buffer, &start_iter, start);
      bobgui_text_buffer_get_iter_at_offset (buffer, &end_iter, end);
      str = bobgui_text_buffer_get_text (buffer, &start_iter, &end_iter, FALSE);
      gdk_clipboard_set_text (bobgui_widget_get_clipboard (widget), str);
      g_free (str);
      g_dbus_method_invocation_return_value (invocation, NULL);
    }
  else if (g_strcmp0 (method_name, "CutText") == 0)
    {
      BobguiTextBuffer *buffer = bobgui_text_view_get_buffer (BOBGUI_TEXT_VIEW (widget));
      BobguiTextIter start_iter, end_iter;
      int start, end;
      gboolean ret = FALSE;

      g_variant_get (parameters, "(ii)", &start, &end);

      if (bobgui_text_view_get_editable (BOBGUI_TEXT_VIEW (widget)))
        {
          char *str;

          bobgui_text_buffer_get_iter_at_offset (buffer, &start_iter, start);
          bobgui_text_buffer_get_iter_at_offset (buffer, &end_iter, end);
          str = bobgui_text_buffer_get_text (buffer, &start_iter, &end_iter, FALSE);
          gdk_clipboard_set_text (bobgui_widget_get_clipboard (widget), str);
          g_free (str);
          bobgui_text_buffer_delete (buffer, &start_iter, &end_iter);
          ret = TRUE;
        }

      g_dbus_method_invocation_return_value (invocation, g_variant_new ("(b)", ret));
    }
  else if (g_strcmp0 (method_name, "DeleteText") == 0)
    {
      BobguiTextBuffer *buffer = bobgui_text_view_get_buffer (BOBGUI_TEXT_VIEW (widget));
      BobguiTextIter start_iter, end_iter;
      int start, end;
      gboolean ret = FALSE;

      g_variant_get (parameters, "(ii)", &start, &end);

      if (bobgui_text_view_get_editable (BOBGUI_TEXT_VIEW (widget)))
        {
          bobgui_text_buffer_get_iter_at_offset (buffer, &start_iter, start);
          bobgui_text_buffer_get_iter_at_offset (buffer, &end_iter, end);
          bobgui_text_buffer_delete (buffer, &start_iter, &end_iter);
          ret = TRUE;
        }

      g_dbus_method_invocation_return_value (invocation, g_variant_new ("(b)", ret));
    }
  else if (g_strcmp0 (method_name, "PasteText") == 0)
    {
      int position;
      gboolean ret = FALSE;

      g_variant_get (parameters, "(i)", &position);

      if (bobgui_text_view_get_editable (BOBGUI_TEXT_VIEW (widget)))
        {
          PasteData *data;

          data = g_new (PasteData, 1);
          data->widget = widget;
          data->position = position;

          gdk_clipboard_read_text_async (bobgui_widget_get_clipboard (widget), NULL, text_view_received, data);

          ret = TRUE;
        }

      g_dbus_method_invocation_return_value (invocation, g_variant_new ("(b)", ret));
    }
}

static const GDBusInterfaceVTable text_view_vtable = {
  text_view_handle_method,
  NULL,
};

/* }}} */

const GDBusInterfaceVTable *
bobgui_atspi_get_editable_text_vtable (BobguiAccessible *accessible)
{
  if (BOBGUI_IS_EDITABLE (accessible))
    return &editable_vtable;
  else if (BOBGUI_IS_TEXT_VIEW (accessible))
    return &text_view_vtable;

  return NULL;
}

/* vim:set foldmethod=marker: */
