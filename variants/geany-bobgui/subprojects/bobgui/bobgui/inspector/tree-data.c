/*
 * Copyright (c) 2014, 2020 Red Hat, Inc.
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

#include "tree-data.h"

#include "object-tree.h"

#include "deprecated/bobguitreeview.h"
#include "deprecated/bobguicellrenderertext.h"
#include "bobguitogglebutton.h"
#include "bobguilabel.h"
#include "bobguistack.h"
#include "bobguiboxlayout.h"
#include "bobguiorientable.h"

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

struct _BobguiInspectorTreeData
{
  BobguiWidget parent_instance;

  BobguiWidget *box;
  BobguiWidget *swin;
  BobguiTreeModel *object;
  BobguiTreeModel *types;
  BobguiTreeView *view;
  BobguiWidget *object_title;
  gboolean show_data;
};

typedef struct _BobguiInspectorTreeDataClass BobguiInspectorTreeDataClass;
struct _BobguiInspectorTreeDataClass
{
  BobguiWidgetClass parent_class;
};


G_DEFINE_TYPE (BobguiInspectorTreeData, bobgui_inspector_tree_data, BOBGUI_TYPE_WIDGET)

static void
bobgui_inspector_tree_data_init (BobguiInspectorTreeData *sl)
{
  bobgui_widget_init_template (BOBGUI_WIDGET (sl));

  bobgui_orientable_set_orientation (BOBGUI_ORIENTABLE (bobgui_widget_get_layout_manager (BOBGUI_WIDGET (sl))),
                                  BOBGUI_ORIENTATION_VERTICAL);
}

static void
cell_data_func (BobguiTreeViewColumn *col,
                BobguiCellRenderer   *cell,
                BobguiTreeModel      *model,
                BobguiTreeIter       *iter,
                gpointer           data)
{
  int num;
  GValue gvalue = { 0, };
  char *value;

  num = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (col), "num"));
  bobgui_tree_model_get_value (model, iter, num, &gvalue);
  value = g_strdup_value_contents (&gvalue);
  g_object_set (cell, "text", value ? value : "", NULL);
  g_free (value);
  g_value_unset (&gvalue);
}

static void
add_columns (BobguiInspectorTreeData *sl)
{
  int n_columns;
  BobguiCellRenderer *cell;
  GType type;
  char *title;
  BobguiTreeViewColumn *col;
  int i;

  n_columns = bobgui_tree_model_get_n_columns (sl->object);
  for (i = 0; i < n_columns; i++)
    {
      cell = bobgui_cell_renderer_text_new ();
      type = bobgui_tree_model_get_column_type (sl->object, i);
      title = g_strdup_printf ("%d: %s", i, g_type_name (type));
      col = bobgui_tree_view_column_new_with_attributes (title, cell, NULL);
      g_object_set_data (G_OBJECT (col), "num", GINT_TO_POINTER (i));
      bobgui_tree_view_column_set_cell_data_func (col, cell, cell_data_func, sl, NULL);
      bobgui_tree_view_append_column (sl->view, col);
      g_free (title);
    }
}

static void
show_types (BobguiInspectorTreeData *sl)
{
  bobgui_tree_view_set_model (sl->view, NULL);
  sl->show_data = FALSE;
}

static void
show_data (BobguiInspectorTreeData *sl)
{
  bobgui_tree_view_set_model (sl->view, sl->object);
  sl->show_data = TRUE;
}

static void
clear_view (BobguiInspectorTreeData *sl)
{
  bobgui_tree_view_set_model (sl->view, NULL);
  while (bobgui_tree_view_get_n_columns (sl->view) > 0)
    bobgui_tree_view_remove_column (sl->view,
                                 bobgui_tree_view_get_column (sl->view, 0));
}

void
bobgui_inspector_tree_data_set_object (BobguiInspectorTreeData *sl,
                                    GObject              *object)
{
  BobguiWidget *stack;
  BobguiStackPage *page;
  char *title;

  stack = bobgui_widget_get_parent (BOBGUI_WIDGET (sl));
  page = bobgui_stack_get_page (BOBGUI_STACK (stack), BOBGUI_WIDGET (sl));

  clear_view (sl);
  sl->object = NULL;
  sl->show_data = FALSE;

  if (!BOBGUI_IS_TREE_MODEL (object))
    {
      g_object_set (page, "visible", FALSE, NULL);
      return;
    }

  title = bobgui_inspector_get_object_title (object);
  bobgui_label_set_label (BOBGUI_LABEL (sl->object_title), title);
  g_free (title);

  g_object_set (page, "visible", TRUE, NULL);

  sl->object = BOBGUI_TREE_MODEL (object);
  add_columns (sl);
  show_types (sl);
}

static void
toggle_show (BobguiToggleButton      *button,
             BobguiInspectorTreeData *sl)
{
  if (bobgui_toggle_button_get_active (button) == sl->show_data)
    return;

  if (bobgui_toggle_button_get_active (button))
    show_data (sl);
  else
    show_types (sl);
}

static void
dispose (GObject *object)
{
  BobguiInspectorTreeData *sl = BOBGUI_INSPECTOR_TREE_DATA (object);

  bobgui_widget_dispose_template (BOBGUI_WIDGET (sl), BOBGUI_TYPE_INSPECTOR_TREE_DATA);

  G_OBJECT_CLASS (bobgui_inspector_tree_data_parent_class)->dispose (object);
}

static void
bobgui_inspector_tree_data_class_init (BobguiInspectorTreeDataClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);

  object_class->dispose = dispose;

  bobgui_widget_class_set_template_from_resource (widget_class, "/org/bobgui/libbobgui/inspector/tree-data.ui");
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorTreeData, box);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorTreeData, swin);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorTreeData, view);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorTreeData, object_title);
  bobgui_widget_class_bind_template_callback (widget_class, toggle_show);

  bobgui_widget_class_set_layout_manager_type (widget_class, BOBGUI_TYPE_BOX_LAYOUT);
}

// vim: set et sw=2 ts=2:
