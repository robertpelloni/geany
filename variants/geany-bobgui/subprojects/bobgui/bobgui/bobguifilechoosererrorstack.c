/* This library is free software; you can redistribute it and/or
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
#include "bobguifilechoosererrorstackprivate.h"
#include "bobguistack.h"
#include "bobguilabel.h"
#include <glib/gi18n-lib.h>
#include "bobguibinlayout.h"

G_DEFINE_TYPE (BobguiFileChooserErrorStack, bobgui_file_chooser_error_stack, BOBGUI_TYPE_WIDGET)

static void
bobgui_file_chooser_error_stack_dispose (GObject *object)
{
  BobguiFileChooserErrorStack *self = BOBGUI_FILE_CHOOSER_ERROR_STACK (object);

  g_clear_pointer (&self->stack, bobgui_widget_unparent);

  G_OBJECT_CLASS (bobgui_file_chooser_error_stack_parent_class)->dispose (object);
}

static void
bobgui_file_chooser_error_stack_class_init (BobguiFileChooserErrorStackClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (class);

  object_class->dispose = bobgui_file_chooser_error_stack_dispose;

  bobgui_widget_class_set_layout_manager_type (widget_class, BOBGUI_TYPE_BIN_LAYOUT);
}

static void
bobgui_file_chooser_error_stack_init (BobguiFileChooserErrorStack *self)
{
  BobguiWidget *label;
  BobguiStack *stack;

  self->stack = bobgui_stack_new ();
  bobgui_widget_set_parent (self->stack, BOBGUI_WIDGET (self));
  stack = BOBGUI_STACK (self->stack);

  bobgui_stack_set_transition_type (stack, BOBGUI_STACK_TRANSITION_TYPE_CROSSFADE);
  bobgui_stack_set_transition_duration (stack, 50);

  label = bobgui_label_new ("");
  bobgui_widget_set_halign (label, BOBGUI_ALIGN_START);
  bobgui_stack_add_named (stack, label, "no-error");

  label = bobgui_label_new ("");
  bobgui_widget_set_halign (label, BOBGUI_ALIGN_START);
  bobgui_stack_add_named (stack, label, "custom");

  label = bobgui_label_new (_("A folder cannot be called “.”"));
  bobgui_widget_set_halign (label, BOBGUI_ALIGN_START);
  bobgui_stack_add_named (stack, label, "folder-cannot-be-called-dot");

  label = bobgui_label_new (_("A file cannot be called “.”"));
  bobgui_widget_set_halign (label, BOBGUI_ALIGN_START);
  bobgui_stack_add_named (stack, label, "file-cannot-be-called-dot");

  label = bobgui_label_new (_("A folder cannot be called “..”"));
  bobgui_widget_set_halign (label, BOBGUI_ALIGN_START);
  bobgui_stack_add_named (stack, label, "folder-cannot-be-called-dot-dot");

  label = bobgui_label_new (_("A file cannot be called “..”"));
  bobgui_widget_set_halign (label, BOBGUI_ALIGN_START);
  bobgui_stack_add_named (stack, label, "file-cannot-be-called-dot-dot");

  label = bobgui_label_new (_("Folder names cannot contain “/”"));
  bobgui_widget_set_halign (label, BOBGUI_ALIGN_START);
  bobgui_stack_add_named (stack, label, "folder-name-cannot-contain-slash");

  label = bobgui_label_new (_("File names cannot contain “/”"));
  bobgui_widget_set_halign (label, BOBGUI_ALIGN_START);
  bobgui_stack_add_named (stack, label, "file-name-cannot-contain-slash");

  label = bobgui_label_new (_("Folder names should not begin with a space"));
  bobgui_widget_set_halign (label, BOBGUI_ALIGN_START);
  bobgui_stack_add_named (stack, label, "folder-name-should-not-begin-with-space");

  label = bobgui_label_new (_("File names should not begin with a space"));
  bobgui_widget_set_halign (label, BOBGUI_ALIGN_START);
  bobgui_stack_add_named (stack, label, "file-name-should-not-begin-with-space");

  label = bobgui_label_new (_("Folder names should not end with a space"));
  bobgui_widget_set_halign (label, BOBGUI_ALIGN_START);
  bobgui_stack_add_named (stack, label, "folder-name-should-not-end-with-space");

  label = bobgui_label_new (_("File names should not end with a space"));
  bobgui_widget_set_halign (label, BOBGUI_ALIGN_START);
  bobgui_stack_add_named (stack, label, "file-name-should-not-end-with-space");

  label = bobgui_label_new (_("Folder names starting with a “.” are hidden"));
  bobgui_widget_set_halign (label, BOBGUI_ALIGN_START);
  bobgui_stack_add_named (stack, label, "folder-name-with-dot-is-hidden");

  label = bobgui_label_new (_("File names starting with a “.” are hidden"));
  bobgui_widget_set_halign (label, BOBGUI_ALIGN_START);
  bobgui_stack_add_named (stack, label, "file-name-with-dot-is-hidden");

  label = bobgui_label_new (_("A folder with that name already exists"));
  bobgui_widget_set_halign (label, BOBGUI_ALIGN_START);
  bobgui_stack_add_named (stack, label, "folder-name-already-exists");

  label = bobgui_label_new (_("A file with that name already exists"));
  bobgui_widget_set_halign (label, BOBGUI_ALIGN_START);
  bobgui_stack_add_named (stack, label, "file-name-already-exists");

  bobgui_stack_set_visible_child_name (stack, "no-error");
}

void
bobgui_file_chooser_error_stack_set_error (BobguiFileChooserErrorStack *self,
                                        gboolean                  is_folder,
                                        const char               *label_name)
{
  char *child_name;

  if (g_strcmp0 (label_name, "no-error") == 0)
    {
      bobgui_stack_set_visible_child_name (BOBGUI_STACK (self->stack), "no-error");
      return;
    }

  child_name = g_strdup_printf ("%s-%s",
                                is_folder ? "folder" : "file",
                                label_name);

  bobgui_stack_set_visible_child_name (BOBGUI_STACK (self->stack), child_name);

  g_free (child_name);
}


void
bobgui_file_chooser_error_stack_set_custom_error  (BobguiFileChooserErrorStack *self,
                                                const char               *label_text)
{
  BobguiWidget *label = bobgui_stack_get_child_by_name (BOBGUI_STACK (self->stack), "custom");

  bobgui_label_set_text (BOBGUI_LABEL (label), label_text);

  bobgui_stack_set_visible_child_name (BOBGUI_STACK (self->stack), "custom");
}

BobguiWidget *
bobgui_file_chooser_error_stack_get_error (BobguiFileChooserErrorStack *self)
{
  if (strcmp (bobgui_stack_get_visible_child_name (BOBGUI_STACK (self->stack)), "no-error") != 0)
    return bobgui_stack_get_visible_child (BOBGUI_STACK (self->stack));

  return NULL;
}
