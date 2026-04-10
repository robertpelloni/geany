/*
 * Copyright (c) 2014 Red Hat, Inc.
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

#include "variant-editor.h"

#include "bobguisizegroup.h"
#include "bobguicheckbutton.h"
#include "bobguientry.h"
#include "bobguilabel.h"
#include "bobguibox.h"
#include "bobguibinlayout.h"


struct _BobguiInspectorVariantEditor
{
  BobguiWidget parent;

  const GVariantType *type;

  BobguiWidget *editor;
  BobguiInspectorVariantEditorChanged callback;
  gpointer   data;
};

typedef struct
{
  BobguiWidgetClass parent;
} BobguiInspectorVariantEditorClass;

static void
variant_editor_changed_cb (GObject                   *obj,
                           GParamSpec                *pspec,
                           BobguiInspectorVariantEditor *self)
{
  self->callback (BOBGUI_WIDGET (self), self->data);
}

G_DEFINE_TYPE (BobguiInspectorVariantEditor, bobgui_inspector_variant_editor, BOBGUI_TYPE_WIDGET)

static void
bobgui_inspector_variant_editor_init (BobguiInspectorVariantEditor *editor)
{
}


static void
dispose (GObject *object)
{
  BobguiInspectorVariantEditor *self = BOBGUI_INSPECTOR_VARIANT_EDITOR (object);

  if (self->editor)
   {
      g_signal_handlers_disconnect_by_func (self->editor, variant_editor_changed_cb, self->data);

      bobgui_widget_unparent (self->editor);
      self->editor = NULL;
    }

  G_OBJECT_CLASS (bobgui_inspector_variant_editor_parent_class)->dispose (object);
}

static void
bobgui_inspector_variant_editor_class_init (BobguiInspectorVariantEditorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);

  object_class->dispose = dispose;

  bobgui_widget_class_set_layout_manager_type (widget_class, BOBGUI_TYPE_BIN_LAYOUT);
}

static void
ensure_editor (BobguiInspectorVariantEditor *self,
               const GVariantType        *type)
{
  if (self->type &&
      g_variant_type_equal (self->type, type))
    return;

  self->type = type;

  if (g_variant_type_equal (type, G_VARIANT_TYPE_BOOLEAN))
    {
      if (self->editor)
        bobgui_widget_unparent (self->editor);

      self->editor = bobgui_check_button_new ();
      g_signal_connect (self->editor, "notify::active",
                        G_CALLBACK (variant_editor_changed_cb), self);

      bobgui_widget_set_parent (self->editor, BOBGUI_WIDGET (self));
    }
  else if (g_variant_type_equal (type, G_VARIANT_TYPE_STRING))
    {
      if (self->editor)
        bobgui_widget_unparent (self->editor);

      self->editor = bobgui_entry_new ();
      bobgui_editable_set_width_chars (BOBGUI_EDITABLE (self->editor), 10);
      g_signal_connect (self->editor, "notify::text",
                        G_CALLBACK (variant_editor_changed_cb), self);

      bobgui_widget_set_parent (self->editor, BOBGUI_WIDGET (self));
    }
  else if (!BOBGUI_IS_BOX (self->editor))
    {
      BobguiWidget *entry, *label;

      if (self->editor)
        bobgui_widget_unparent (self->editor);

      self->editor = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 10);
      entry = bobgui_entry_new ();
      bobgui_editable_set_width_chars (BOBGUI_EDITABLE (entry), 10);
      bobgui_box_append (BOBGUI_BOX (self->editor), entry);
      label = bobgui_label_new (g_variant_type_peek_string (type));
      bobgui_box_append (BOBGUI_BOX (self->editor), label);
      g_signal_connect (entry, "notify::text",
                        G_CALLBACK (variant_editor_changed_cb), self);

      bobgui_widget_set_parent (self->editor, BOBGUI_WIDGET (self));
    }
}

BobguiWidget *
bobgui_inspector_variant_editor_new (const GVariantType               *type,
                                  BobguiInspectorVariantEditorChanged  callback,
                                  gpointer                          data)
{
  BobguiInspectorVariantEditor *self;

  self = g_object_new (BOBGUI_TYPE_INSPECTOR_VARIANT_EDITOR, NULL);

  self->callback = callback;
  self->data = data;

  if (type)
    ensure_editor (self, type);

  return BOBGUI_WIDGET (self);
}

void
bobgui_inspector_variant_editor_set_type (BobguiWidget          *editor,
                                       const GVariantType *type)
{
  BobguiInspectorVariantEditor *self = BOBGUI_INSPECTOR_VARIANT_EDITOR (editor);

  ensure_editor (self, type);
}

void
bobgui_inspector_variant_editor_set_value (BobguiWidget *editor,
                                        GVariant  *value)
{
  BobguiInspectorVariantEditor *self = BOBGUI_INSPECTOR_VARIANT_EDITOR (editor);

  ensure_editor (self, g_variant_get_type (value));

  g_signal_handlers_block_by_func (self->editor, variant_editor_changed_cb, self);

  if (g_variant_type_equal (self->type, G_VARIANT_TYPE_BOOLEAN))
    {
      BobguiCheckButton *b = BOBGUI_CHECK_BUTTON (self->editor);

      if (bobgui_check_button_get_active (b) != g_variant_get_boolean (value))
        bobgui_check_button_set_active (b, g_variant_get_boolean (value));
    }
  else if (g_variant_type_equal (self->type, G_VARIANT_TYPE_STRING))
    {
      BobguiEntry *entry = BOBGUI_ENTRY (self->editor);

      bobgui_editable_set_text (BOBGUI_EDITABLE (entry),
                             g_variant_get_string (value, NULL));
    }
  else
    {
      BobguiWidget *entry;
      char *text;

      entry = bobgui_widget_get_first_child (self->editor);

      text = g_variant_print (value, FALSE);
      bobgui_editable_set_text (BOBGUI_EDITABLE (entry), text);
      g_free (text);
    }

  g_signal_handlers_unblock_by_func (self->editor, variant_editor_changed_cb, self);
}

GVariant *
bobgui_inspector_variant_editor_get_value (BobguiWidget *editor)
{
  BobguiInspectorVariantEditor *self = BOBGUI_INSPECTOR_VARIANT_EDITOR (editor);
  GVariant *value;

  if (self->type == NULL)
    return NULL;

  if (g_variant_type_equal (self->type, G_VARIANT_TYPE_BOOLEAN))
    {
      BobguiCheckButton *b = BOBGUI_CHECK_BUTTON (self->editor);
      value = g_variant_new_boolean (bobgui_check_button_get_active (b));
    }
  else if (g_variant_type_equal (self->type, G_VARIANT_TYPE_STRING))
    {
      BobguiEntry *entry = BOBGUI_ENTRY (self->editor);
      value = g_variant_new_string (bobgui_editable_get_text (BOBGUI_EDITABLE (entry)));
    }
  else
    {
      BobguiWidget *entry;
      const char *text;

      entry = bobgui_widget_get_first_child (self->editor);
      text = bobgui_editable_get_text (BOBGUI_EDITABLE (entry));

      value = g_variant_parse (self->type, text, NULL, NULL, NULL);
    }

  return value;
}
