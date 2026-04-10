/*
 * Copyright (c) 2015 Red Hat, Inc.
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

#include "strv-editor.h"
#include "bobguiaccessible.h"
#include "bobguibutton.h"
#include "bobguientry.h"
#include "bobguibox.h"
#include "bobguiorientable.h"
#include "bobguimarshalers.h"

enum
{
  CHANGED,
  N_SIGNALS
};

static guint signals[N_SIGNALS] = { 0 };

G_DEFINE_TYPE (BobguiInspectorStrvEditor, bobgui_inspector_strv_editor, BOBGUI_TYPE_BOX);

static void
emit_changed (BobguiInspectorStrvEditor *editor)
{
  if (editor->blocked)
    return;

  g_signal_emit (editor, signals[CHANGED], 0);
}

static void
remove_string (BobguiButton              *button,
               BobguiInspectorStrvEditor *editor)
{
  BobguiWidget *row;

  row = bobgui_widget_get_parent (BOBGUI_WIDGET (button));
  bobgui_box_remove (BOBGUI_BOX (bobgui_widget_get_parent (row)), row);
  emit_changed (editor);
}

static void
add_string (BobguiInspectorStrvEditor *editor,
            const char             *str)
{
  BobguiWidget *box;
  BobguiWidget *entry;
  BobguiWidget *button;

  box = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
  bobgui_widget_add_css_class (box, "linked");

  entry = bobgui_entry_new ();
  bobgui_editable_set_text (BOBGUI_EDITABLE (entry), str);
  bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (entry),
                                  BOBGUI_ACCESSIBLE_PROPERTY_LABEL, _("Value"),
                                  -1);
  bobgui_box_append (BOBGUI_BOX (box), entry);
  g_object_set_data (G_OBJECT (box), "entry", entry);
  g_signal_connect_swapped (entry, "notify::text", G_CALLBACK (emit_changed), editor);

  button = bobgui_button_new_from_icon_name ("user-trash-symbolic");
  bobgui_widget_add_css_class (button, "image-button");
  bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (button),
                                  BOBGUI_ACCESSIBLE_PROPERTY_LABEL,
                                  g_strdup_printf (_("Remove %s"), str),
                                  -1);
  bobgui_box_append (BOBGUI_BOX (box), button);
  g_signal_connect (button, "clicked", G_CALLBACK (remove_string), editor);

  bobgui_box_append (BOBGUI_BOX (editor->box), box);

  bobgui_widget_grab_focus (entry);

  emit_changed (editor);
}

static void
add_cb (BobguiButton              *button,
        BobguiInspectorStrvEditor *editor)
{
  add_string (editor, "");
}

static void
bobgui_inspector_strv_editor_init (BobguiInspectorStrvEditor *editor)
{
  bobgui_box_set_spacing (BOBGUI_BOX (editor), 6);
  bobgui_orientable_set_orientation (BOBGUI_ORIENTABLE (editor), BOBGUI_ORIENTATION_HORIZONTAL);
  editor->box = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 6);

  editor->button = bobgui_button_new_from_icon_name ("list-add-symbolic");
  bobgui_widget_add_css_class (editor->button, "image-button");
  bobgui_widget_set_focus_on_click (editor->button, FALSE);
  bobgui_widget_set_valign (editor->button, BOBGUI_ALIGN_START);
  bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (editor->button),
                                  BOBGUI_ACCESSIBLE_PROPERTY_LABEL, _("Add"),
                                  -1);
  g_signal_connect (editor->button, "clicked", G_CALLBACK (add_cb), editor);

  bobgui_box_append (BOBGUI_BOX (editor), editor->box);
  bobgui_box_append (BOBGUI_BOX (editor), editor->button);
}

static void
bobgui_inspector_strv_editor_class_init (BobguiInspectorStrvEditorClass *class)
{
  signals[CHANGED] =
    g_signal_new ("changed",
                  G_TYPE_FROM_CLASS (class),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (BobguiInspectorStrvEditorClass, changed),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);
}

void
bobgui_inspector_strv_editor_set_strv (BobguiInspectorStrvEditor  *editor,
                                    char                   **strv)
{
  BobguiWidget *child;
  int i;

  editor->blocked = TRUE;

  while ((child = bobgui_widget_get_first_child (BOBGUI_WIDGET (editor->box))))
    bobgui_box_remove (BOBGUI_BOX (editor->box), child);

  if (strv)
    {
      for (i = 0; strv[i]; i++)
        add_string (editor, strv[i]);
    }

  editor->blocked = FALSE;

  emit_changed (editor);
}

char **
bobgui_inspector_strv_editor_get_strv (BobguiInspectorStrvEditor *editor)
{
  BobguiWidget *child;
  GPtrArray *p;

  p = g_ptr_array_new ();

  for (child = bobgui_widget_get_first_child (editor->box);
       child != NULL;
       child = bobgui_widget_get_next_sibling (child))
    {
      BobguiEntry *entry;

      entry = BOBGUI_ENTRY (g_object_get_data (G_OBJECT (child), "entry"));
      g_ptr_array_add (p, g_strdup (bobgui_editable_get_text (BOBGUI_EDITABLE (entry))));
    }

  g_ptr_array_add (p, NULL);

  return (char **)g_ptr_array_free (p, FALSE);
}
