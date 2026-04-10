/*
 * Copyright (c) 2014 Intel Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Author:
 *      Ikey Doherty <michael.i.doherty@intel.com>
 */

#include "config.h"

#include "bobguistacksidebar.h"

#include "bobguibinlayout.h"
#include "bobguilabel.h"
#include "bobguilistbox.h"
#include "bobguiscrolledwindow.h"
#include "bobguiseparator.h"
#include "bobguiselectionmodel.h"
#include "bobguistack.h"
#include "bobguiprivate.h"
#include "bobguiwidgetprivate.h"

#include <glib/gi18n-lib.h>

/**
 * BobguiStackSidebar:
 *
 * Uses a sidebar to switch between `BobguiStack` pages.
 *
 * <picture>
 *   <source srcset="sidebar-dark.png" media="(prefers-color-scheme: dark)">
 *   <img alt="An example BobguiStackSidebar" src="sidebar.png">
 * </picture>
 *
 * In order to use a `BobguiStackSidebar`, you simply use a `BobguiStack` to
 * organize your UI flow, and add the sidebar to your sidebar area. You
 * can use [method@Bobgui.StackSidebar.set_stack] to connect the `BobguiStackSidebar`
 * to the `BobguiStack`.
 *
 * # CSS nodes
 *
 * `BobguiStackSidebar` has a single CSS node with name stacksidebar and
 * style class .sidebar.
 *
 * When circumstances require it, `BobguiStackSidebar` adds the
 * .needs-attention style class to the widgets representing the stack
 * pages.
 */

typedef struct _BobguiStackSidebarClass   BobguiStackSidebarClass;

struct _BobguiStackSidebar
{
  BobguiWidget parent_instance;

  BobguiListBox *list;
  BobguiStack *stack;
  BobguiSelectionModel *pages;
  /* HashTable<ref BobguiStackPage, BobguiListBoxRow> */
  GHashTable *rows;
};

struct _BobguiStackSidebarClass
{
  BobguiWidgetClass parent_class;
};

G_DEFINE_TYPE (BobguiStackSidebar, bobgui_stack_sidebar, BOBGUI_TYPE_WIDGET)

enum
{
  PROP_0,
  PROP_STACK,
  N_PROPERTIES
};
static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

static void
bobgui_stack_sidebar_set_property (GObject    *object,
                                guint       prop_id,
                                const       GValue *value,
                                GParamSpec *pspec)
{
  switch (prop_id)
    {
    case PROP_STACK:
      bobgui_stack_sidebar_set_stack (BOBGUI_STACK_SIDEBAR (object), g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_stack_sidebar_get_property (GObject    *object,
                                guint       prop_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  BobguiStackSidebar *self = BOBGUI_STACK_SIDEBAR (object);

  switch (prop_id)
    {
    case PROP_STACK:
      g_value_set_object (value, self->stack);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_stack_sidebar_row_selected (BobguiListBox    *box,
                                BobguiListBoxRow *row,
                                gpointer       userdata)
{
  BobguiStackSidebar *self = BOBGUI_STACK_SIDEBAR (userdata);
  guint index;

  if (row == NULL)
    return;

  index = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (row), "child-index"));
  bobgui_selection_model_select_item (self->pages, index, TRUE);
}

static void
bobgui_stack_sidebar_init (BobguiStackSidebar *self)
{
  BobguiWidget *sw;

  sw = bobgui_scrolled_window_new ();
  bobgui_scrolled_window_set_policy (BOBGUI_SCROLLED_WINDOW (sw),
                                  BOBGUI_POLICY_NEVER,
                                  BOBGUI_POLICY_AUTOMATIC);

  bobgui_widget_set_parent (sw, BOBGUI_WIDGET (self));

  self->list = BOBGUI_LIST_BOX (bobgui_list_box_new ());
  bobgui_widget_add_css_class (BOBGUI_WIDGET (self->list), "navigation-sidebar");
  bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (self->list),
                                  BOBGUI_ACCESSIBLE_PROPERTY_LABEL,
                                  C_("accessibility", "Sidebar"),
                                  -1);


  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (sw), BOBGUI_WIDGET (self->list));

  g_signal_connect (self->list, "row-selected",
                    G_CALLBACK (bobgui_stack_sidebar_row_selected), self);

  bobgui_widget_add_css_class (BOBGUI_WIDGET (self), "sidebar");

  self->rows = g_hash_table_new_full (NULL, NULL, g_object_unref, NULL);
}

static void
update_row (BobguiStackSidebar *self,
            BobguiStackPage    *page,
            BobguiWidget       *row)
{
  BobguiWidget *item;
  char *title;
  gboolean needs_attention;
  gboolean visible;

  g_object_get (page,
                "title", &title,
                "needs-attention", &needs_attention,
                "visible", &visible,
                NULL);

  item = bobgui_list_box_row_get_child (BOBGUI_LIST_BOX_ROW (row));
  bobgui_label_set_text (BOBGUI_LABEL (item), title);

  bobgui_widget_set_visible (row, visible && title != NULL);

  if (needs_attention)
    bobgui_widget_add_css_class (row, "needs-attention");
  else
    bobgui_widget_remove_css_class (row, "needs-attention");

  g_free (title);
}

static void
on_page_updated (BobguiStackPage    *page,
                 GParamSpec      *pspec,
                 BobguiStackSidebar *self)
{
  BobguiWidget *row;

  row = g_hash_table_lookup (self->rows, page);
  update_row (self, page, row);
}

static void
add_child (guint            position,
           BobguiStackSidebar *self)
{
  BobguiWidget *item;
  BobguiWidget *row;
  BobguiStackPage *page;

  /* Make a pretty item when we add kids */
  item = bobgui_label_new ("");
  bobgui_widget_set_halign (item, BOBGUI_ALIGN_START);
  bobgui_widget_set_valign (item, BOBGUI_ALIGN_CENTER);
  row = bobgui_list_box_row_new ();
  bobgui_list_box_row_set_child (BOBGUI_LIST_BOX_ROW (row), item);

  bobgui_accessible_update_relation (BOBGUI_ACCESSIBLE (row),
                                  BOBGUI_ACCESSIBLE_RELATION_LABELLED_BY,
                                  item,
                                  NULL,
                                  -1);

  page = g_list_model_get_item (G_LIST_MODEL (self->pages), position);
  update_row (self, page, row);

  bobgui_list_box_insert (BOBGUI_LIST_BOX (self->list), row, -1);

  g_object_set_data (G_OBJECT (row), "child-index", GUINT_TO_POINTER (position));
  if (bobgui_selection_model_is_selected (self->pages, position))
    bobgui_list_box_select_row (self->list, BOBGUI_LIST_BOX_ROW (row));
  else
    bobgui_list_box_unselect_row (self->list, BOBGUI_LIST_BOX_ROW (row));

  g_signal_connect (page, "notify", G_CALLBACK (on_page_updated), self);

  g_hash_table_insert (self->rows, g_object_ref (page), row);

  g_object_unref (page);
}

static void
populate_sidebar (BobguiStackSidebar *self)
{
  guint i, n;

  n = g_list_model_get_n_items (G_LIST_MODEL (self->pages));
  for (i = 0; i < n; i++)
    add_child (i, self);
}

static void
clear_sidebar (BobguiStackSidebar *self)
{
  GHashTableIter iter;
  BobguiStackPage *page;
  BobguiWidget *row;

  g_hash_table_iter_init (&iter, self->rows);
  while (g_hash_table_iter_next (&iter, (gpointer *)&page, (gpointer *)&row))
    {
      g_signal_handlers_disconnect_by_func (page, on_page_updated, self);
      bobgui_list_box_remove (BOBGUI_LIST_BOX (self->list), row);
      /* This will unref page, but it is safe now: */
      g_hash_table_iter_remove (&iter);
    }
}

static void
items_changed_cb (GListModel       *model,
                  guint             position,
                  guint             removed,
                  guint             added,
                  BobguiStackSidebar  *self)
{
  /* FIXME: we can do better */
  clear_sidebar (self);
  populate_sidebar (self);
}

static void
selection_changed_cb (BobguiSelectionModel *model,
                      guint              position,
                      guint              n_items,
                      BobguiStackSidebar   *self)
{
  guint i;

  for (i = position; i < position + n_items; i++)
    {
      BobguiStackPage *page;
      BobguiWidget *row;

      page = g_list_model_get_item (G_LIST_MODEL (self->pages), i);
      row = g_hash_table_lookup (self->rows, page);
      if (bobgui_selection_model_is_selected (self->pages, i))
        bobgui_list_box_select_row (self->list, BOBGUI_LIST_BOX_ROW (row));
      else
        bobgui_list_box_unselect_row (self->list, BOBGUI_LIST_BOX_ROW (row));
      g_object_unref (page);
    }
}

static void
set_stack (BobguiStackSidebar *self,
           BobguiStack        *stack)
{
  if (stack)
    {
      self->stack = g_object_ref (stack);
      self->pages = bobgui_stack_get_pages (stack);
      populate_sidebar (self);
      g_signal_connect (self->pages, "items-changed", G_CALLBACK (items_changed_cb), self);
      g_signal_connect (self->pages, "selection-changed", G_CALLBACK (selection_changed_cb), self);
    }
}

static void
unset_stack (BobguiStackSidebar *self)
{
  if (self->stack)
    {
      g_signal_handlers_disconnect_by_func (self->pages, items_changed_cb, self);
      g_signal_handlers_disconnect_by_func (self->pages, selection_changed_cb, self);
      clear_sidebar (self);
      g_clear_object (&self->stack);
      g_clear_object (&self->pages);
    }
}

static void
bobgui_stack_sidebar_dispose (GObject *object)
{
  BobguiStackSidebar *self = BOBGUI_STACK_SIDEBAR (object);
  BobguiWidget *child;

  unset_stack (self);

  /* The scrolled window */
  child = bobgui_widget_get_first_child (BOBGUI_WIDGET (self));
  if (child)
    {
      bobgui_widget_unparent (child);
      self->list = NULL;
    }

  G_OBJECT_CLASS (bobgui_stack_sidebar_parent_class)->dispose (object);
}

static void
bobgui_stack_sidebar_finalize (GObject *object)
{
  BobguiStackSidebar *self = BOBGUI_STACK_SIDEBAR (object);

  g_hash_table_destroy (self->rows);

  G_OBJECT_CLASS (bobgui_stack_sidebar_parent_class)->finalize (object);
}

static void
bobgui_stack_sidebar_class_init (BobguiStackSidebarClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);

  object_class->dispose = bobgui_stack_sidebar_dispose;
  object_class->finalize = bobgui_stack_sidebar_finalize;
  object_class->set_property = bobgui_stack_sidebar_set_property;
  object_class->get_property = bobgui_stack_sidebar_get_property;

   /**
   * BobguiStackSidebar:stack:
   *
   * The stack.
   */
  obj_properties[PROP_STACK] =
      g_param_spec_object (I_("stack"), NULL, NULL,
                           BOBGUI_TYPE_STACK,
                           G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS|G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, N_PROPERTIES, obj_properties);

  bobgui_widget_class_set_layout_manager_type (widget_class, BOBGUI_TYPE_BIN_LAYOUT);
  bobgui_widget_class_set_css_name (widget_class, I_("stacksidebar"));
}

/**
 * bobgui_stack_sidebar_new:
 *
 * Creates a new `BobguiStackSidebar`.
 *
 * Returns: the new `BobguiStackSidebar`
 */
BobguiWidget *
bobgui_stack_sidebar_new (void)
{
  return BOBGUI_WIDGET (g_object_new (BOBGUI_TYPE_STACK_SIDEBAR, NULL));
}

/**
 * bobgui_stack_sidebar_set_stack:
 * @self: a `BobguiStackSidebar`
 * @stack: a `BobguiStack`
 *
 * Set the `BobguiStack` associated with this `BobguiStackSidebar`.
 *
 * The sidebar widget will automatically update according to
 * the order and items within the given `BobguiStack`.
 */
void
bobgui_stack_sidebar_set_stack (BobguiStackSidebar *self,
                             BobguiStack        *stack)
{
  g_return_if_fail (BOBGUI_IS_STACK_SIDEBAR (self));
  g_return_if_fail (BOBGUI_IS_STACK (stack) || stack == NULL);


  if (self->stack == stack)
    return;

  unset_stack (self);
  set_stack (self, stack);

  bobgui_widget_queue_resize (BOBGUI_WIDGET (self));

  g_object_notify (G_OBJECT (self), "stack");
}

/**
 * bobgui_stack_sidebar_get_stack:
 * @self: a `BobguiStackSidebar`
 *
 * Retrieves the stack.
 *
 * Returns: (nullable) (transfer none): the associated `BobguiStack` or
 *   %NULL if none has been set explicitly
 */
BobguiStack *
bobgui_stack_sidebar_get_stack (BobguiStackSidebar *self)
{
  g_return_val_if_fail (BOBGUI_IS_STACK_SIDEBAR (self), NULL);

  return BOBGUI_STACK (self->stack);
}
