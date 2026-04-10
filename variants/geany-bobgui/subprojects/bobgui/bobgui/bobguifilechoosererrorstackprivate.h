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

#pragma once


#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include "bobguistack.h"

G_BEGIN_DECLS

#define BOBGUI_TYPE_FILE_CHOOSER_ERROR_STACK                 (bobgui_file_chooser_error_stack_get_type ())
#define BOBGUI_FILE_CHOOSER_ERROR_STACK(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_FILE_CHOOSER_ERROR_STACK, BobguiFileChooserErrorStack))
#define BOBGUI_FILE_CHOOSER_ERROR_STACK_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), BOBGUI_TYPE_FILE_CHOOSER_ERROR_STACK, BobguiFileChooserErrorStackClass))
#define BOBGUI_IS_FILE_CHOOSER_ERROR_STACK(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_FILE_CHOOSER_ERROR_STACK))
#define BOBGUI_IS_FILE_CHOOSER_ERROR_STACK_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), BOBGUI_TYPE_FILE_CHOOSER_ERROR_STACK))
#define BOBGUI_FILE_CHOOSER_ERROR_STACK_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_FILE_CHOOSER_ERROR_STACK, BobguiFileChooserErrorStackClass))

typedef struct _BobguiFileChooserErrorStack             BobguiFileChooserErrorStack;
typedef struct _BobguiFileChooserErrorStackClass        BobguiFileChooserErrorStackClass;

struct _BobguiFileChooserErrorStack
{
  BobguiWidget parent_instance;

  BobguiWidget *stack;
};

struct _BobguiFileChooserErrorStackClass
{
  BobguiWidgetClass parent_class;
};

GType  bobgui_file_chooser_error_stack_get_type          (void) G_GNUC_CONST;

void   bobgui_file_chooser_error_stack_set_error         (BobguiFileChooserErrorStack *self,
                                                       gboolean                  is_folder,
                                                       const char               *label_name);

void   bobgui_file_chooser_error_stack_set_custom_error  (BobguiFileChooserErrorStack *self,
                                                       const char               *label_text);
BobguiWidget *bobgui_file_chooser_error_stack_get_error     (BobguiFileChooserErrorStack *self);

G_END_DECLS

