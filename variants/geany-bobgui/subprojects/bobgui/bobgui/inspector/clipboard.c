/*
 * Copyright (c) 2021 Benjamin Otte
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

#include "clipboard.h"
#include "bobguidataviewer.h"
#include "window.h"

#include "bobguibinlayout.h"
#include "bobguibox.h"
#include "bobguidebug.h"
#include "bobguidropcontrollermotion.h"
#include "bobguilabel.h"
#include "bobguilistbox.h"
#include "bobguitogglebutton.h"

struct _BobguiInspectorClipboard
{
  BobguiWidget parent;

  GdkDisplay *display;

  BobguiWidget *swin;

  BobguiWidget *dnd_formats;
  BobguiWidget *dnd_info;

  BobguiWidget *clipboard_formats;
  BobguiWidget *clipboard_info;

  BobguiWidget *primary_formats;
  BobguiWidget *primary_info;
};

typedef struct _BobguiInspectorClipboardClass
{
  BobguiWidgetClass parent_class;
} BobguiInspectorClipboardClass;

G_DEFINE_TYPE (BobguiInspectorClipboard, bobgui_inspector_clipboard, BOBGUI_TYPE_WIDGET)

static void
load_gtype_value (GObject      *source,
                  GAsyncResult *res,
                  gpointer      data)
{
  BobguiDataViewer *viewer = data;
  const GValue *value;
  GError *error = NULL;

  if (GDK_IS_CLIPBOARD (source))
    value = gdk_clipboard_read_value_finish (GDK_CLIPBOARD (source), res, &error);
  else if (GDK_IS_DROP (source))
    value = gdk_drop_read_value_finish (GDK_DROP (source), res, &error);
  else
    g_assert_not_reached ();

  if (value == NULL)
    bobgui_data_viewer_load_error (viewer, error);
  else
    bobgui_data_viewer_load_value (viewer, value);

  g_object_unref (viewer);
}

static gboolean
load_gtype (BobguiDataViewer *viewer,
            GCancellable  *cancellable,
            gpointer       gtype)
{
  GObject *data_source = g_object_get_data (G_OBJECT (viewer), "data-source");

  if (GDK_IS_CLIPBOARD (data_source))
    {
      gdk_clipboard_read_value_async (GDK_CLIPBOARD (data_source),
                                      GPOINTER_TO_SIZE (gtype),
                                      G_PRIORITY_DEFAULT,
                                      cancellable,
                                      load_gtype_value,
                                      g_object_ref (viewer));
    }
  else if (GDK_IS_DROP (data_source))
    {
      gdk_drop_read_value_async (GDK_DROP (data_source),
                                 GPOINTER_TO_SIZE (gtype),
                                 G_PRIORITY_DEFAULT,
                                 cancellable,
                                 load_gtype_value,
                                 g_object_ref (viewer));
    }
  else
    {
      g_assert_not_reached ();
    }

  return TRUE;
}

static void
load_mime_type_stream (GObject      *source,
                       GAsyncResult *res,
                       gpointer      data)
{
  BobguiDataViewer *viewer = data;
  GInputStream *stream;
  GError *error = NULL;
  const char *mime_type;

  if (GDK_IS_CLIPBOARD (source))
    stream = gdk_clipboard_read_finish (GDK_CLIPBOARD (source), res, &mime_type, &error);
  else if (GDK_IS_DROP (source))
    stream = gdk_drop_read_finish (GDK_DROP (source), res, &mime_type, &error);
  else
    g_assert_not_reached ();

  if (stream == NULL)
    bobgui_data_viewer_load_error (viewer, error);
  else
    bobgui_data_viewer_load_stream (viewer, stream, mime_type);

  g_object_unref (viewer);
}

static gboolean
load_mime_type (BobguiDataViewer *viewer,
                GCancellable  *cancellable,
                gpointer       mime_type)
{
  GObject *data_source = g_object_get_data (G_OBJECT (viewer), "data-source");

  if (GDK_IS_CLIPBOARD (data_source))
    {
      gdk_clipboard_read_async (GDK_CLIPBOARD (data_source),
                                (const char *[2]) { mime_type, NULL },
                                G_PRIORITY_DEFAULT,
                                cancellable,
                                load_mime_type_stream,
                                g_object_ref (viewer));
    }
  else if (GDK_IS_DROP (data_source))
    {
      gdk_drop_read_async (GDK_DROP (data_source),
                           (const char *[2]) { mime_type, NULL },
                           G_PRIORITY_DEFAULT,
                           cancellable,
                           load_mime_type_stream,
                           g_object_ref (viewer));
    }
  else
    {
      g_assert_not_reached ();
    }

  return TRUE;
}

static void
on_drop_row_enter (BobguiDropControllerMotion *motion,
                   double                   x,
                   double                   y,
                   BobguiWidget               *viewer)
{
  bobgui_widget_set_visible (viewer, TRUE);
}

static void
add_content_type_row (BobguiInspectorClipboard *self,
                      BobguiListBox            *list,
                      const char            *type_name,
                      GObject               *data_source,
                      GCallback              load_func,
                      gpointer               load_func_data)
{
  BobguiWidget *row, *vbox, *hbox, *label, *viewer, *button;

  vbox = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 10);

  hbox = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 40);
  bobgui_box_append (BOBGUI_BOX (vbox), hbox);

  label = bobgui_label_new (type_name);
  bobgui_widget_set_halign (label, BOBGUI_ALIGN_START);
  bobgui_widget_set_valign (label, BOBGUI_ALIGN_BASELINE_FILL);
  bobgui_label_set_xalign (BOBGUI_LABEL (label), 0.0);
  bobgui_label_set_max_width_chars (BOBGUI_LABEL (label), 60);
  bobgui_label_set_ellipsize (BOBGUI_LABEL (label), PANGO_ELLIPSIZE_END);
  bobgui_widget_set_hexpand (label, TRUE);
  bobgui_box_append (BOBGUI_BOX (hbox), label);

  viewer = bobgui_data_viewer_new ();
  g_signal_connect (viewer, "load", load_func, load_func_data);
  g_object_set_data (G_OBJECT (viewer), "data-source", data_source);
  bobgui_box_append (BOBGUI_BOX (vbox), viewer);

  if (GDK_IS_CLIPBOARD (data_source))
    {
      button = bobgui_toggle_button_new_with_label (_("Show"));
      bobgui_widget_set_halign (button, BOBGUI_ALIGN_END);
      bobgui_widget_set_valign (button, BOBGUI_ALIGN_BASELINE_FILL);
      bobgui_box_append (BOBGUI_BOX (hbox), button);

      g_object_bind_property (G_OBJECT (button), "active",
                              G_OBJECT (viewer), "visible",
                              G_BINDING_SYNC_CREATE);
    }
  else
    {
      BobguiEventController *controller = bobgui_drop_controller_motion_new ();
      g_signal_connect (controller, "enter", G_CALLBACK (on_drop_row_enter), viewer);
      bobgui_widget_add_controller (vbox, controller);

      bobgui_widget_set_visible (viewer, FALSE);

      label = bobgui_label_new (_("Hover to load"));
      g_object_bind_property (G_OBJECT (viewer), "visible",
                              G_OBJECT (label), "visible",
                              G_BINDING_SYNC_CREATE | G_BINDING_INVERT_BOOLEAN);
      bobgui_widget_set_halign (label, BOBGUI_ALIGN_END);
      bobgui_widget_set_valign (label, BOBGUI_ALIGN_BASELINE_FILL);
      bobgui_box_append (BOBGUI_BOX (hbox), label);
    }

  row = bobgui_list_box_row_new ();
  bobgui_list_box_row_set_child (BOBGUI_LIST_BOX_ROW (row), vbox);
  bobgui_list_box_row_set_activatable (BOBGUI_LIST_BOX_ROW (row), FALSE);

  bobgui_list_box_insert (list, row, -1);
}

static void
clear_formats (BobguiInspectorClipboard *self,
               BobguiListBox            *list)
{
  BobguiListBoxRow *row;

  while ((row = bobgui_list_box_get_row_at_index (list, 1)))
    bobgui_list_box_remove (list, BOBGUI_WIDGET (row));
}

static void
init_formats (BobguiInspectorClipboard *self,
              BobguiListBox            *list,
              GdkContentFormats     *formats,
              GObject               *data_source)
{
  const char * const *mime_types;
  const GType *gtypes;
  gsize i, n;

  clear_formats (self, list);

  gtypes = gdk_content_formats_get_gtypes (formats, &n);
  for (i = 0; i < n; i++)
    add_content_type_row (self, list, g_type_name (gtypes[i]), data_source, G_CALLBACK (load_gtype), GSIZE_TO_POINTER (gtypes[i]));

  mime_types = gdk_content_formats_get_mime_types (formats, &n);
  for (i = 0; i < n; i++)
    add_content_type_row (self, list, mime_types[i], data_source, G_CALLBACK (load_mime_type), (gpointer) mime_types[i]);
}

static void
init_info (BobguiInspectorClipboard *self,
           BobguiLabel              *label,
           GdkClipboard          *clipboard)
{
  GdkContentFormats *formats;

  formats = gdk_clipboard_get_formats (clipboard);
  if (gdk_content_formats_get_gtypes (formats, NULL) == NULL &&
      gdk_content_formats_get_mime_types (formats, NULL) == NULL)
    {
      bobgui_label_set_text (label, C_("clipboard", "empty"));
      return;
    }

  if (gdk_clipboard_is_local (clipboard))
    bobgui_label_set_text (label, C_("clipboard", "local"));
  else
    bobgui_label_set_text (label, C_("clipboard", "remote"));
}

static void
clipboard_notify (GdkClipboard          *clipboard,
                  GParamSpec            *pspec,
                  BobguiInspectorClipboard *self)
{
  if (g_str_equal (pspec->name, "formats"))
    {
      init_formats (self, BOBGUI_LIST_BOX (self->clipboard_formats), gdk_clipboard_get_formats (clipboard), G_OBJECT (clipboard));
    }

  init_info (self, BOBGUI_LABEL (self->clipboard_info), clipboard);
}

static void
primary_notify (GdkClipboard          *clipboard,
                GParamSpec            *pspec,
                BobguiInspectorClipboard *self)
{
  if (g_str_equal (pspec->name, "formats"))
    {
      init_formats (self, BOBGUI_LIST_BOX (self->primary_formats), gdk_clipboard_get_formats (clipboard), G_OBJECT (clipboard));
    }

  init_info (self, BOBGUI_LABEL (self->primary_info), clipboard);
}

static void
drop_done (gpointer data,
           GObject *object)
{
  BobguiInspectorClipboard *self = data;

  clear_formats (self, BOBGUI_LIST_BOX (self->dnd_formats));
}

static void
on_drop_enter (BobguiDropControllerMotion *motion,
               double                   x,
               double                   y,
               BobguiInspectorClipboard   *self)
{
  GdkDrop *drop = bobgui_drop_controller_motion_get_drop (motion);

  g_object_weak_ref (G_OBJECT (drop), drop_done, self);

  init_formats (self, BOBGUI_LIST_BOX (self->dnd_formats), gdk_drop_get_formats (drop), G_OBJECT (drop));

  if (gdk_drop_get_drag (drop))
    bobgui_label_set_text (BOBGUI_LABEL (self->dnd_info), C_("clipboard", "local"));
  else
    bobgui_label_set_text (BOBGUI_LABEL (self->dnd_info), C_("clipboard", "remote"));
}

static void
bobgui_inspector_clipboard_unset_display (BobguiInspectorClipboard *self)
{
  GdkClipboard *clipboard;

  if (self->display == NULL)
    return;

  clipboard = gdk_display_get_clipboard (self->display);
  g_signal_handlers_disconnect_by_func (clipboard, clipboard_notify, self);

  clipboard = gdk_display_get_primary_clipboard (self->display);
  g_signal_handlers_disconnect_by_func (clipboard, primary_notify, self);
}

static void
bobgui_inspector_clipboard_init (BobguiInspectorClipboard *self)
{
  bobgui_widget_init_template (BOBGUI_WIDGET (self));
}

static void
bobgui_inspector_clipboard_dispose (GObject *object)
{
  BobguiInspectorClipboard *self = BOBGUI_INSPECTOR_CLIPBOARD (object);

  bobgui_inspector_clipboard_unset_display (self);

  bobgui_widget_dispose_template (BOBGUI_WIDGET (self), BOBGUI_TYPE_INSPECTOR_CLIPBOARD);

  G_OBJECT_CLASS (bobgui_inspector_clipboard_parent_class)->dispose (object);
}

static void
bobgui_inspector_clipboard_class_init (BobguiInspectorClipboardClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);

  object_class->dispose = bobgui_inspector_clipboard_dispose;

  bobgui_widget_class_set_template_from_resource (widget_class, "/org/bobgui/libbobgui/inspector/clipboard.ui");
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorClipboard, swin);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorClipboard, dnd_formats);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorClipboard, dnd_info);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorClipboard, clipboard_formats);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorClipboard, clipboard_info);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorClipboard, primary_formats);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorClipboard, primary_info);

  bobgui_widget_class_bind_template_callback (widget_class, on_drop_enter);

  bobgui_widget_class_set_layout_manager_type (widget_class, BOBGUI_TYPE_BIN_LAYOUT);
}

void
bobgui_inspector_clipboard_set_display (BobguiInspectorClipboard *self,
                                     GdkDisplay            *display)
{
  GdkClipboard *clipboard;

  bobgui_inspector_clipboard_unset_display (self);

  self->display = display;

  if (display == NULL)
    return;

  clipboard = gdk_display_get_clipboard (display);
  g_signal_connect (clipboard, "notify", G_CALLBACK (clipboard_notify), self);
  init_formats (self, BOBGUI_LIST_BOX (self->clipboard_formats), gdk_clipboard_get_formats (clipboard), G_OBJECT (clipboard));
  init_info (self, BOBGUI_LABEL (self->clipboard_info), clipboard);

  clipboard = gdk_display_get_primary_clipboard (display);
  g_signal_connect (clipboard, "notify", G_CALLBACK (primary_notify), self);
  init_formats (self, BOBGUI_LIST_BOX (self->primary_formats), gdk_clipboard_get_formats (clipboard), G_OBJECT (clipboard));
  init_info (self, BOBGUI_LABEL (self->primary_info), clipboard);
}

