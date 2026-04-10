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

#include "action-editor.h"
#include "variant-editor.h"

#include "bobguitogglebutton.h"
#include "bobguientry.h"
#include "bobguilabel.h"
#include "bobguibox.h"
#include "bobguiboxlayout.h"
#include "bobguiorientable.h"
#include "bobguiactionmuxerprivate.h"

struct _BobguiInspectorActionEditor
{
  BobguiWidget parent;

  GObject *owner;
  char *name;
  gboolean enabled;
  const GVariantType *parameter_type;
  GVariantType *state_type;
  GVariant *state;
  BobguiWidget *activate_button;
  BobguiWidget *parameter_entry;
  BobguiWidget *state_entry;
  BobguiWidget *state_editor;
};

typedef struct
{
  BobguiWidgetClass parent;
} BobguiInspectorActionEditorClass;

enum
{
  PROP_0,
  PROP_OWNER,
  PROP_NAME
};

G_DEFINE_TYPE (BobguiInspectorActionEditor, bobgui_inspector_action_editor, BOBGUI_TYPE_WIDGET)

static void update_widgets (BobguiInspectorActionEditor *r);

static void
activate_action (BobguiWidget                *button,
                 BobguiInspectorActionEditor *r)
{
  GVariant *parameter = NULL;

  if (r->parameter_entry)
    parameter = bobgui_inspector_variant_editor_get_value (r->parameter_entry);
  if (G_IS_ACTION_GROUP (r->owner))
    g_action_group_activate_action (G_ACTION_GROUP (r->owner), r->name, parameter);
  else if (BOBGUI_IS_ACTION_MUXER (r->owner))
    bobgui_action_muxer_activate_action (BOBGUI_ACTION_MUXER (r->owner), r->name, parameter);

  update_widgets (r);
}

static void
parameter_changed (BobguiWidget *editor,
                   gpointer   data)
{
  BobguiInspectorActionEditor *r = data;
  GVariant *value;

  value = bobgui_inspector_variant_editor_get_value (editor);
  bobgui_widget_set_sensitive (r->activate_button, r->enabled && value != NULL);
  if (value)
    g_variant_unref (value);
}

static void
state_changed (BobguiWidget *editor,
               gpointer   data)
{
  BobguiInspectorActionEditor *r = data;
  GVariant *value;

  value = bobgui_inspector_variant_editor_get_value (editor);
  if (!value)
    return;

  g_variant_ref_sink (value);
  if (g_variant_equal (value, r->state))
    {
      g_variant_unref (value);
      return;
    }

  if (G_IS_ACTION_GROUP (r->owner))
    g_action_group_change_action_state (G_ACTION_GROUP (r->owner), r->name, value);
  else if (BOBGUI_IS_ACTION_MUXER (r->owner))
    bobgui_action_muxer_change_action_state (BOBGUI_ACTION_MUXER (r->owner), r->name, value);

  g_variant_unref (value);
}

static void
bobgui_inspector_action_editor_init (BobguiInspectorActionEditor *r)
{
  BobguiBoxLayout *layout;
  BobguiWidget *row, *activate, *label;

  layout = BOBGUI_BOX_LAYOUT (bobgui_widget_get_layout_manager (BOBGUI_WIDGET (r)));
  bobgui_orientable_set_orientation (BOBGUI_ORIENTABLE (layout), BOBGUI_ORIENTATION_HORIZONTAL);
  bobgui_box_layout_set_spacing (layout, 10);

  row = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 10);
  activate = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 10);
  bobgui_box_append (BOBGUI_BOX (row), activate);

  r->activate_button = bobgui_button_new_with_label (_("Activate"));
  g_signal_connect (r->activate_button, "clicked", G_CALLBACK (activate_action), r);

  bobgui_box_append (BOBGUI_BOX (activate), r->activate_button);

  r->parameter_entry = bobgui_inspector_variant_editor_new (NULL, parameter_changed, r);
  bobgui_widget_set_visible (r->parameter_entry, FALSE);
  bobgui_box_append (BOBGUI_BOX (activate), r->parameter_entry);

  bobgui_widget_set_parent (row, BOBGUI_WIDGET (r));

  r->state_editor = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 10);
  label = bobgui_label_new (_("Set State"));
  bobgui_box_append (BOBGUI_BOX (r->state_editor), label);
  r->state_entry = bobgui_inspector_variant_editor_new (NULL, state_changed, r);
  bobgui_box_append (BOBGUI_BOX (r->state_editor), r->state_entry);
  bobgui_widget_set_parent (r->state_editor, BOBGUI_WIDGET (r));
  bobgui_widget_set_visible (r->state_editor, FALSE);
}

static void
update_widgets (BobguiInspectorActionEditor *r)
{
  g_clear_pointer (&r->state, g_variant_unref);

  if (G_IS_ACTION_GROUP (r->owner))
    {
      if (!g_action_group_query_action (G_ACTION_GROUP (r->owner), r->name,
                                        &r->enabled, &r->parameter_type, NULL, NULL,
                                        &r->state))
        {
          r->enabled = FALSE;
          r->parameter_type = NULL;
          r->state = NULL;
        }
    }
  else if (BOBGUI_IS_ACTION_MUXER (r->owner))
    {
      if (!bobgui_action_muxer_query_action (BOBGUI_ACTION_MUXER (r->owner), r->name,
                                          &r->enabled, &r->parameter_type, NULL, NULL,
                                          &r->state))
        {
          r->enabled = FALSE;
          r->parameter_type = NULL;
          r->state = NULL;
        }
    }
  else
    {
      r->enabled = FALSE;
      r->parameter_type = NULL;
      r->state = NULL;
    }

  bobgui_widget_set_sensitive (r->activate_button, r->enabled);
  bobgui_widget_set_sensitive (r->parameter_entry, r->enabled);
  bobgui_widget_set_visible (r->parameter_entry, r->parameter_type != NULL);
  if (r->parameter_type)
    bobgui_inspector_variant_editor_set_type (r->parameter_entry, r->parameter_type);

  bobgui_widget_set_visible (r->state_editor, r->state != NULL);
  if (r->state)
    {
      if (r->state_type)
        g_variant_type_free (r->state_type);
      r->state_type = g_variant_type_copy (g_variant_get_type (r->state));
      bobgui_inspector_variant_editor_set_value (r->state_entry, r->state);
    }
}

static void
dispose (GObject *object)
{
  BobguiInspectorActionEditor *r = BOBGUI_INSPECTOR_ACTION_EDITOR (object);
  BobguiWidget *child;

  while ((child = bobgui_widget_get_first_child (BOBGUI_WIDGET (r))))
    bobgui_widget_unparent (child);

  g_clear_pointer (&r->name, g_free);
  g_clear_pointer (&r->state_type, g_variant_type_free);
  g_clear_pointer (&r->state, g_variant_unref);

  G_OBJECT_CLASS (bobgui_inspector_action_editor_parent_class)->dispose (object);
}

static void
get_property (GObject    *object,
              guint       param_id,
              GValue     *value,
              GParamSpec *pspec)
{
  BobguiInspectorActionEditor *r = BOBGUI_INSPECTOR_ACTION_EDITOR (object);

  switch (param_id)
    {
    case PROP_OWNER:
      g_value_set_object (value, r->owner);
      break;

    case PROP_NAME:
      g_value_set_string (value, r->name);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

static void
set_property (GObject      *object,
              guint         param_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  BobguiInspectorActionEditor *r = BOBGUI_INSPECTOR_ACTION_EDITOR (object);

  switch (param_id)
    {
    case PROP_OWNER:
      r->owner = g_value_get_object (value);
      g_assert (r->owner == NULL ||
                G_IS_ACTION_GROUP (r->owner) ||
                BOBGUI_IS_ACTION_MUXER (r->owner));
      break;

    case PROP_NAME:
      g_free (r->name);
      r->name = g_value_dup_string (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, param_id, pspec);
      break;
    }
}

static void
bobgui_inspector_action_editor_class_init (BobguiInspectorActionEditorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);

  object_class->dispose = dispose;
  object_class->get_property = get_property;
  object_class->set_property = set_property;

  g_object_class_install_property (object_class, PROP_OWNER,
      g_param_spec_object ("owner", NULL, NULL,
                           G_TYPE_OBJECT, G_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_NAME,
      g_param_spec_string ("name", NULL, NULL,
                           NULL, G_PARAM_READWRITE));

  bobgui_widget_class_set_layout_manager_type (widget_class, BOBGUI_TYPE_BOX_LAYOUT);
}

BobguiWidget *
bobgui_inspector_action_editor_new (void)
{
  return g_object_new (BOBGUI_TYPE_INSPECTOR_ACTION_EDITOR, NULL);
}

void
bobgui_inspector_action_editor_set (BobguiInspectorActionEditor *self,
                                 GObject                  *owner,
                                 const char               *name)
{
  g_object_set (self, "owner", owner, "name", name, NULL);
  update_widgets (self);
}

void
bobgui_inspector_action_editor_update (BobguiInspectorActionEditor *self)
{
  update_widgets (self);
}
