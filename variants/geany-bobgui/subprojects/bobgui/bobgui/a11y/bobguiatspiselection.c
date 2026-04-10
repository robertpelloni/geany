/* bobguiatspiselection.c: AT-SPI Selection implementation
 *
 * Copyright 2020 Red Hat, Inc.
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

#include "bobguiatspiselectionprivate.h"

#include "a11y/atspi/atspi-selection.h"

#include "bobguiatcontextprivate.h"
#include "bobguiatspicontextprivate.h"
#include "bobguiaccessibleprivate.h"
#include "bobguidebug.h"
#include "bobguilistbase.h"
#include "bobguilistbox.h"
#include "bobguiflowbox.h"
#include "deprecated/bobguicombobox.h"
#include "bobguistackswitcher.h"
#include "bobguinotebook.h"
#include "bobguilistview.h"
#include "bobguigridview.h"
#include "bobguilistitem.h"
#include "bobguibitset.h"
#include "bobguilistbaseprivate.h"
#include "bobguilistitemwidgetprivate.h"

#include <gio/gio.h>

typedef struct {
  int n;
  BobguiWidget *child;
} Counter;

static void
find_nth (BobguiWidget *box,
          BobguiWidget *child,
          gpointer   data)
{
  Counter *counter = data;

  if (counter->n == 0)
    counter->child = child;

  counter->n--;
}

/* {{{ BobguiListbox */

static void
listbox_handle_method (GDBusConnection       *connection,
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

  if (g_strcmp0 (method_name, "GetSelectedChild") == 0)
    {
      Counter counter;
      int idx;

      g_variant_get (parameters, "(i)", &idx);

      counter.n = idx;
      counter.child = NULL;

      bobgui_list_box_selected_foreach (BOBGUI_LIST_BOX (widget), (BobguiListBoxForeachFunc)find_nth, &counter);

      if (counter.child == NULL)
        g_dbus_method_invocation_return_error (invocation, G_DBUS_ERROR, G_DBUS_ERROR_INVALID_ARGS, "No selected child for %d", idx);
      else
        {
          BobguiATContext *ctx = bobgui_accessible_get_at_context (BOBGUI_ACCESSIBLE (counter.child));
          g_dbus_method_invocation_return_value (invocation,
            g_variant_new ("(@(so))", bobgui_at_spi_context_to_ref (BOBGUI_AT_SPI_CONTEXT (ctx))));
          g_object_unref (ctx);
        }
    }
  else if (g_strcmp0 (method_name, "SelectChild") == 0)
    {
      int idx;
      BobguiListBoxRow *row;

      g_variant_get (parameters, "(i)", &idx);

      row = bobgui_list_box_get_row_at_index (BOBGUI_LIST_BOX (widget), idx);
      if (!row)
        g_dbus_method_invocation_return_error (invocation, G_DBUS_ERROR, G_DBUS_ERROR_INVALID_ARGS, "No child at position %d", idx);
      else
        {
          gboolean ret;

          bobgui_list_box_select_row (BOBGUI_LIST_BOX (widget), row);
          ret = bobgui_list_box_row_is_selected (row);
          g_dbus_method_invocation_return_value (invocation, g_variant_new ("(b)", ret));
        }
    }
  else if (g_strcmp0 (method_name, "DeselectChild") == 0)
    {
      int idx;
      BobguiListBoxRow *row;

      g_variant_get (parameters, "(i)", &idx);

      row = bobgui_list_box_get_row_at_index (BOBGUI_LIST_BOX (widget), idx);
      if (!row)
        g_dbus_method_invocation_return_error (invocation, G_DBUS_ERROR, G_DBUS_ERROR_INVALID_ARGS, "No child at position %d", idx);
      else
        {
          gboolean ret;

          bobgui_list_box_unselect_row (BOBGUI_LIST_BOX (widget), row);
          ret = !bobgui_list_box_row_is_selected (row);
          g_dbus_method_invocation_return_value (invocation, g_variant_new ("(b)", ret));
        }
    }
  else if (g_strcmp0 (method_name, "DeselectSelectedChild") == 0)
    {
      Counter counter;
      int idx;

      g_variant_get (parameters, "(i)", &idx);

      counter.n = idx;
      counter.child = NULL;

      bobgui_list_box_selected_foreach (BOBGUI_LIST_BOX (widget), (BobguiListBoxForeachFunc)find_nth, &counter);

      if (counter.child == NULL)
        g_dbus_method_invocation_return_error (invocation, G_DBUS_ERROR, G_DBUS_ERROR_INVALID_ARGS, "No selected child for %d", idx);
      else
        {
          gboolean ret;

          bobgui_list_box_unselect_row (BOBGUI_LIST_BOX (widget), BOBGUI_LIST_BOX_ROW (counter.child));
          ret = !bobgui_list_box_row_is_selected (BOBGUI_LIST_BOX_ROW (counter.child));
          g_dbus_method_invocation_return_value (invocation, g_variant_new ("(b)", ret));
        }
    }
  else if (g_strcmp0 (method_name, "IsChildSelected") == 0)
    {
      int idx;
      BobguiListBoxRow *row;

      g_variant_get (parameters, "(i)", &idx);

      row = bobgui_list_box_get_row_at_index (BOBGUI_LIST_BOX (widget), idx);
      if (!row)
        g_dbus_method_invocation_return_error (invocation, G_DBUS_ERROR, G_DBUS_ERROR_INVALID_ARGS, "No child at position %d", idx);
      else
        g_dbus_method_invocation_return_value (invocation, g_variant_new ("(b)", bobgui_list_box_row_is_selected (row)));
    }
  else if (g_strcmp0 (method_name, "SelectAll") == 0)
    {
      bobgui_list_box_select_all (BOBGUI_LIST_BOX (widget));
      g_dbus_method_invocation_return_value (invocation, g_variant_new ("(b)", TRUE));
    }
  else if (g_strcmp0 (method_name, "ClearSelection") == 0)
    {
      bobgui_list_box_unselect_all (BOBGUI_LIST_BOX (widget));
      g_dbus_method_invocation_return_value (invocation, g_variant_new ("(b)", TRUE));
    }
}

static void
count_selected (BobguiWidget *box,
                BobguiWidget *child,
                gpointer   data)
{
  *(int *)data += 1;
}

static GVariant *
listbox_get_property (GDBusConnection  *connection,
                      const gchar      *sender,
                      const gchar      *object_path,
                      const gchar      *interface_name,
                      const gchar      *property_name,
                      GError          **error,
                      gpointer          user_data)
{
  BobguiATContext *self = BOBGUI_AT_CONTEXT (user_data);
  BobguiAccessible *accessible = bobgui_at_context_get_accessible (self);
  BobguiWidget *widget = BOBGUI_WIDGET (accessible);

  if (g_strcmp0 (property_name, "NSelectedChildren") == 0)
    {
      int count = 0;

      bobgui_list_box_selected_foreach (BOBGUI_LIST_BOX (widget), (BobguiListBoxForeachFunc)count_selected, &count);

      return g_variant_new_int32 (count);
    }

  return NULL;
}

static const GDBusInterfaceVTable listbox_vtable = {
  listbox_handle_method,
  listbox_get_property,
  NULL
};

/* }}} */
/* {{{ BobguiListView */

static void
listview_handle_method (GDBusConnection       *connection,
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
  BobguiSelectionModel *model = bobgui_list_base_get_model (BOBGUI_LIST_BASE (widget));

  if (g_strcmp0 (method_name, "GetSelectedChild") == 0)
    {
      int idx;
      BobguiWidget *child;

      g_variant_get (parameters, "(i)", &idx);

      /* We are asked for the idx-the selected child *among the
       * current children*
       */
      for (child = bobgui_widget_get_first_child (widget);
           child;
           child = bobgui_widget_get_next_sibling (child))
        {
          if (!BOBGUI_IS_LIST_ITEM_BASE (child))
            continue;
          if (bobgui_list_item_base_get_selected (BOBGUI_LIST_ITEM_BASE (child)))
            {
              if (idx == 0)
                break;
              idx--;
            }
        }

      if (child == NULL)
        g_dbus_method_invocation_return_error (invocation,
                                               G_DBUS_ERROR,
                                               G_DBUS_ERROR_INVALID_ARGS,
                                               "No selected child for %d", idx);
      else
        {
          BobguiATContext *ctx = bobgui_accessible_get_at_context (BOBGUI_ACCESSIBLE (child));
          g_dbus_method_invocation_return_value (invocation,
            g_variant_new ("(@(so))", bobgui_at_spi_context_to_ref (BOBGUI_AT_SPI_CONTEXT (ctx))));
          g_object_unref (ctx);
        }
    }
  else if (g_strcmp0 (method_name, "SelectChild") == 0)
    {
      int idx;
      BobguiWidget *child;

      g_variant_get (parameters, "(i)", &idx);

      for (child = bobgui_widget_get_first_child (widget);
           child;
           child = bobgui_widget_get_next_sibling (child))
        {
          if (!BOBGUI_IS_LIST_ITEM_BASE (child))
            continue;
          if (idx == 0)
            break;
          idx--;
        }

      if (child == NULL)
        g_dbus_method_invocation_return_error (invocation,
                                               G_DBUS_ERROR,
                                               G_DBUS_ERROR_INVALID_ARGS,
                                               "No child for %d", idx);
      else
        {
          guint pos;
          gboolean ret;

          pos = bobgui_list_item_base_get_position (BOBGUI_LIST_ITEM_BASE (child));
          ret = bobgui_selection_model_select_item (model, pos, FALSE);

          g_dbus_method_invocation_return_value (invocation, g_variant_new ("(b)", ret));
        }
    }
  else if (g_strcmp0 (method_name, "DeselectChild") == 0)
    {
      int idx;
      BobguiWidget *child;

      g_variant_get (parameters, "(i)", &idx);

      for (child = bobgui_widget_get_first_child (widget);
           child;
           child = bobgui_widget_get_next_sibling (child))
        {
          if (!BOBGUI_IS_LIST_ITEM_BASE (child))
            continue;
          if (idx == 0)
            break;
          idx--;
        }

      if (child == NULL)
        g_dbus_method_invocation_return_error (invocation,
                                               G_DBUS_ERROR,
                                               G_DBUS_ERROR_INVALID_ARGS,
                                               "No child for %d", idx);
      else
        {
          guint pos;
          gboolean ret;

          pos = bobgui_list_item_base_get_position (BOBGUI_LIST_ITEM_BASE (child));
          ret = bobgui_selection_model_unselect_item (model, pos);

          g_dbus_method_invocation_return_value (invocation, g_variant_new ("(b)", ret));
        }
    }
  else if (g_strcmp0 (method_name, "DeselectSelectedChild") == 0)
    {
      int idx;
      BobguiWidget *child;

      g_variant_get (parameters, "(i)", &idx);

      /* We are asked for the n-th selected child *among the current children* */
      for (child = bobgui_widget_get_first_child (widget);
           child;
           child = bobgui_widget_get_next_sibling (child))
        {
          if (!BOBGUI_IS_LIST_ITEM_BASE (child))
            continue;
          if (bobgui_list_item_base_get_selected (BOBGUI_LIST_ITEM_BASE (child)))
            {
              if (idx == 0)
                break;
              idx--;
            }
        }

      if (child == NULL)
        g_dbus_method_invocation_return_error (invocation,
                                               G_DBUS_ERROR,
                                               G_DBUS_ERROR_INVALID_ARGS,
                                               "No selected child for %d", idx);
      else
        {
          guint pos;
          gboolean ret;

          pos = bobgui_list_item_base_get_position (BOBGUI_LIST_ITEM_BASE (child));
          ret = bobgui_selection_model_unselect_item (model, pos);

          g_dbus_method_invocation_return_value (invocation, g_variant_new ("(b)", ret));
        }
    }
  else if (g_strcmp0 (method_name, "IsChildSelected") == 0)
    {
      int idx;
      BobguiWidget *child;

      g_variant_get (parameters, "(i)", &idx);

      for (child = bobgui_widget_get_first_child (widget);
           child;
           child = bobgui_widget_get_next_sibling (child))
        {
          if (!BOBGUI_IS_LIST_ITEM_BASE (child))
            continue;
          if (idx == 0)
            break;
          idx--;
        }

      if (child == NULL)
        g_dbus_method_invocation_return_error (invocation,
                                               G_DBUS_ERROR,
                                               G_DBUS_ERROR_INVALID_ARGS,
                                               "No child for %d", idx);
      else
        {
          gboolean ret;

          ret = bobgui_list_item_base_get_selected (BOBGUI_LIST_ITEM_BASE (child));

          g_dbus_method_invocation_return_value (invocation, g_variant_new ("(b)", ret));
        }
    }
  else if (g_strcmp0 (method_name, "SelectAll") == 0)
    {
      gboolean ret;

      /* This is a bit inconsistent - the Selection interface is defined in terms
       * of the current children, but this selects all items in the model, whether
       * they are currently represented or not.
       */
      ret = bobgui_selection_model_select_all (model);

      g_dbus_method_invocation_return_value (invocation, g_variant_new ("(b)", ret));
    }
  else if (g_strcmp0 (method_name, "ClearSelection") == 0)
    {
      gboolean ret;

      ret = bobgui_selection_model_unselect_all (model);

      g_dbus_method_invocation_return_value (invocation, g_variant_new ("(b)", ret));
    }
}

static GVariant *
listview_get_property (GDBusConnection  *connection,
                       const gchar      *sender,
                       const gchar      *object_path,
                       const gchar      *interface_name,
                       const gchar      *property_name,
                       GError          **error,
                       gpointer          user_data)
{
  BobguiATContext *self = BOBGUI_AT_CONTEXT (user_data);
  BobguiAccessible *accessible = bobgui_at_context_get_accessible (self);
  BobguiWidget *widget = BOBGUI_WIDGET (accessible);
  BobguiSelectionModel *model = bobgui_list_base_get_model (BOBGUI_LIST_BASE (widget));

  if (g_strcmp0 (property_name, "NSelectedChildren") == 0)
    {
      int count = 0;
      BobguiBitset *set;

      set = bobgui_selection_model_get_selection (model);
      count = bobgui_bitset_get_size (set);
      bobgui_bitset_unref (set);

      return g_variant_new_int32 (count);
    }

  return NULL;
}

static const GDBusInterfaceVTable listview_vtable = {
  listview_handle_method,
  listview_get_property,
  NULL
};

/* }}} */
/* {{{ BobguiFlowBox */

static void
flowbox_handle_method (GDBusConnection       *connection,
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

  if (g_strcmp0 (method_name, "GetSelectedChild") == 0)
    {
      Counter counter;
      int idx;

      g_variant_get (parameters, "(i)", &idx);

      counter.n = idx;
      counter.child = NULL;

      bobgui_flow_box_selected_foreach (BOBGUI_FLOW_BOX (widget), (BobguiFlowBoxForeachFunc)find_nth, &counter);

      if (counter.child == NULL)
        g_dbus_method_invocation_return_error (invocation, G_DBUS_ERROR, G_DBUS_ERROR_INVALID_ARGS, "No selected child for %d", idx);
      else
        {
          BobguiATContext *ctx = bobgui_accessible_get_at_context (BOBGUI_ACCESSIBLE (counter.child));
          g_dbus_method_invocation_return_value (invocation,
            g_variant_new ("(@(so))", bobgui_at_spi_context_to_ref (BOBGUI_AT_SPI_CONTEXT (ctx))));
          g_object_unref (ctx);
        }
    }
  else if (g_strcmp0 (method_name, "SelectChild") == 0)
    {
      int idx;
      BobguiFlowBoxChild *child;

      g_variant_get (parameters, "(i)", &idx);

      child = bobgui_flow_box_get_child_at_index (BOBGUI_FLOW_BOX (widget), idx);
      if (!child)
        g_dbus_method_invocation_return_error (invocation, G_DBUS_ERROR, G_DBUS_ERROR_INVALID_ARGS, "No child at position %d", idx);
      else
        {
          gboolean ret;

          bobgui_flow_box_select_child (BOBGUI_FLOW_BOX (widget), child);
          ret = bobgui_flow_box_child_is_selected (child);
          g_dbus_method_invocation_return_value (invocation, g_variant_new ("(b)", ret));
        }
    }
  else if (g_strcmp0 (method_name, "DeselectChild") == 0)
    {
      int idx;
      BobguiFlowBoxChild *child;

      g_variant_get (parameters, "(i)", &idx);

      child = bobgui_flow_box_get_child_at_index (BOBGUI_FLOW_BOX (widget), idx);
      if (!child)
        g_dbus_method_invocation_return_error (invocation, G_DBUS_ERROR, G_DBUS_ERROR_INVALID_ARGS, "No child at position %d", idx);
      else
        {
          gboolean ret;

          bobgui_flow_box_unselect_child (BOBGUI_FLOW_BOX (widget), child);
          ret = !bobgui_flow_box_child_is_selected (child);
          g_dbus_method_invocation_return_value (invocation, g_variant_new ("(b)", ret));
        }
    }
  else if (g_strcmp0 (method_name, "DeselectSelectedChild") == 0)
    {
      Counter counter;
      int idx;

      g_variant_get (parameters, "(i)", &idx);

      counter.n = idx;
      counter.child = NULL;

      bobgui_flow_box_selected_foreach (BOBGUI_FLOW_BOX (widget), (BobguiFlowBoxForeachFunc)find_nth, &counter);

      if (counter.child == NULL)
        g_dbus_method_invocation_return_error (invocation, G_DBUS_ERROR, G_DBUS_ERROR_INVALID_ARGS, "No selected child for %d", idx);
      else
        {
          gboolean ret;

          bobgui_flow_box_unselect_child (BOBGUI_FLOW_BOX (widget), BOBGUI_FLOW_BOX_CHILD (counter.child));
          ret = !bobgui_flow_box_child_is_selected (BOBGUI_FLOW_BOX_CHILD (counter.child));
          g_dbus_method_invocation_return_value (invocation, g_variant_new ("(b)", ret));
        }
    }
  else if (g_strcmp0 (method_name, "IsChildSelected") == 0)
    {
      int idx;
      BobguiFlowBoxChild *child;

      g_variant_get (parameters, "(i)", &idx);

      child = bobgui_flow_box_get_child_at_index (BOBGUI_FLOW_BOX (widget), idx);
      if (!child)
        g_dbus_method_invocation_return_error (invocation, G_DBUS_ERROR, G_DBUS_ERROR_INVALID_ARGS, "No child at position %d", idx);
      else
        g_dbus_method_invocation_return_value (invocation, g_variant_new ("(b)", bobgui_flow_box_child_is_selected (child)));
    }
  else if (g_strcmp0 (method_name, "SelectAll") == 0)
    {
      bobgui_flow_box_select_all (BOBGUI_FLOW_BOX (widget));
      g_dbus_method_invocation_return_value (invocation, g_variant_new ("(b)", TRUE));
    }
  else if (g_strcmp0 (method_name, "ClearSelection") == 0)
    {
      bobgui_flow_box_unselect_all (BOBGUI_FLOW_BOX (widget));
      g_dbus_method_invocation_return_value (invocation, g_variant_new ("(b)", TRUE));
    }
}

static GVariant *
flowbox_get_property (GDBusConnection  *connection,
                      const gchar      *sender,
                      const gchar      *object_path,
                      const gchar      *interface_name,
                      const gchar      *property_name,
                      GError          **error,
                      gpointer          user_data)
{
  BobguiATContext *self = BOBGUI_AT_CONTEXT (user_data);
  BobguiAccessible *accessible = bobgui_at_context_get_accessible (self);
  BobguiWidget *widget = BOBGUI_WIDGET (accessible);

  if (g_strcmp0 (property_name, "NSelectedChildren") == 0)
    {
      int count = 0;

      bobgui_flow_box_selected_foreach (BOBGUI_FLOW_BOX (widget), (BobguiFlowBoxForeachFunc)count_selected, &count);

      return g_variant_new_int32 (count);
    }

  return NULL;
}

static const GDBusInterfaceVTable flowbox_vtable = {
  flowbox_handle_method,
  flowbox_get_property,
  NULL
};

/* }}} */
/* {{{ BobguiComboBox */

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

static void
combobox_handle_method (GDBusConnection       *connection,
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

  if (g_strcmp0 (method_name, "GetSelectedChild") == 0)
    {
      /* Need to figure out what to do here */
      g_dbus_method_invocation_return_error_literal (invocation, G_DBUS_ERROR, G_DBUS_ERROR_NOT_SUPPORTED, "");
    }
  else if (g_strcmp0 (method_name, "SelectChild") == 0)
    {
      int idx;

      g_variant_get (parameters, "(i)", &idx);
      bobgui_combo_box_set_active (BOBGUI_COMBO_BOX (widget), idx);
      g_dbus_method_invocation_return_value (invocation, g_variant_new ("(b)", TRUE));
    }
  else if (g_strcmp0 (method_name, "DeselectChild") == 0)
    {
      int idx;

      g_variant_get (parameters, "(i)", &idx);

      bobgui_combo_box_set_active (BOBGUI_COMBO_BOX (widget), -1);
      g_dbus_method_invocation_return_value (invocation, g_variant_new ("(b)", TRUE));
    }
  else if (g_strcmp0 (method_name, "DeselectSelectedChild") == 0)
    {
      int idx;

      g_variant_get (parameters, "(i)", &idx);
      if (idx == 0)
        bobgui_combo_box_set_active (BOBGUI_COMBO_BOX (widget), -1);
      g_dbus_method_invocation_return_value (invocation, g_variant_new ("(b)", idx == 0));
    }
  else if (g_strcmp0 (method_name, "IsChildSelected") == 0)
    {
      int idx;
      gboolean active;

      g_variant_get (parameters, "(i)", &idx);
      active = idx = bobgui_combo_box_get_active (BOBGUI_COMBO_BOX (widget));
      g_dbus_method_invocation_return_value (invocation, g_variant_new ("(b)", active));
    }
  else if (g_strcmp0 (method_name, "SelectAll") == 0)
    {
      g_dbus_method_invocation_return_value (invocation, g_variant_new ("(b)", FALSE));
    }
  else if (g_strcmp0 (method_name, "ClearSelection") == 0)
    {
      bobgui_combo_box_set_active (BOBGUI_COMBO_BOX (widget), -1);
      g_dbus_method_invocation_return_value (invocation, g_variant_new ("(b)", TRUE));
    }
}

static GVariant *
combobox_get_property (GDBusConnection  *connection,
                       const gchar      *sender,
                       const gchar      *object_path,
                       const gchar      *interface_name,
                       const gchar      *property_name,
                       GError          **error,
                       gpointer          user_data)
{
  BobguiATContext *self = BOBGUI_AT_CONTEXT (user_data);
  BobguiAccessible *accessible = bobgui_at_context_get_accessible (self);
  BobguiWidget *widget = BOBGUI_WIDGET (accessible);

  if (g_strcmp0 (property_name, "NSelectedChildren") == 0)
    {
      if (bobgui_combo_box_get_active (BOBGUI_COMBO_BOX (widget)))
        return g_variant_new_int32 (1);
      else
        return g_variant_new_int32 (0);
    }

  return NULL;
}

static const GDBusInterfaceVTable combobox_vtable = {
  combobox_handle_method,
  combobox_get_property,
  NULL
};

G_GNUC_END_IGNORE_DEPRECATIONS

/* }}} */
/* {{{ BobguiStackSwitcher */

static void
stackswitcher_handle_method (GDBusConnection       *connection,
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
  BobguiStack *stack = bobgui_stack_switcher_get_stack (BOBGUI_STACK_SWITCHER (widget));

  if (g_strcmp0 (method_name, "GetSelectedChild") == 0)
    {
      guint i, n;
      BobguiSelectionModel *pages;
      BobguiWidget *child;

      pages = bobgui_stack_get_pages (stack);
      n = g_list_model_get_n_items (G_LIST_MODEL (pages));
      for (i = 0, child = bobgui_widget_get_first_child (widget);
           i < n && child;
           i++, child = bobgui_widget_get_next_sibling (child))
        {
          if (bobgui_selection_model_is_selected (pages, i))
            break;
        }
      g_object_unref (pages);

      if (child == NULL)
        g_dbus_method_invocation_return_error_literal (invocation,
                                                       G_DBUS_ERROR,
                                                       G_DBUS_ERROR_INVALID_ARGS,
                                                       "No selected child");
      else
        {
          BobguiATContext *ctx = bobgui_accessible_get_at_context (BOBGUI_ACCESSIBLE (child));
          g_dbus_method_invocation_return_value (invocation,
            g_variant_new ("(@(so))", bobgui_at_spi_context_to_ref (BOBGUI_AT_SPI_CONTEXT (ctx))));
          g_object_unref (ctx);
        }
    }
  else if (g_strcmp0 (method_name, "SelectChild") == 0)
    {
      int idx;
      BobguiSelectionModel *pages;

      g_variant_get (parameters, "(i)", &idx);

      pages = bobgui_stack_get_pages (stack);
      bobgui_selection_model_select_item (pages, idx, TRUE);
      g_object_unref (pages);

      g_dbus_method_invocation_return_value (invocation, g_variant_new ("(b)", TRUE));
    }
  else if (g_strcmp0 (method_name, "DeselectChild") == 0)
    {
      g_dbus_method_invocation_return_value (invocation, g_variant_new ("(b)", FALSE));
    }
  else if (g_strcmp0 (method_name, "DeselectSelectedChild") == 0)
    {
      g_dbus_method_invocation_return_value (invocation, g_variant_new ("(b)", FALSE));
    }
  else if (g_strcmp0 (method_name, "IsChildSelected") == 0)
    {
      int idx;
      BobguiSelectionModel *pages;
      gboolean active;

      g_variant_get (parameters, "(i)", &idx);

      pages = bobgui_stack_get_pages (stack);
      active = bobgui_selection_model_is_selected (pages, idx);
      g_object_unref (pages);

      g_dbus_method_invocation_return_value (invocation, g_variant_new ("(b)", active));
    }
  else if (g_strcmp0 (method_name, "SelectAll") == 0)
    {
      g_dbus_method_invocation_return_value (invocation, g_variant_new ("(b)", FALSE));
    }
  else if (g_strcmp0 (method_name, "ClearSelection") == 0)
    {
      g_dbus_method_invocation_return_value (invocation, g_variant_new ("(b)", FALSE));
    }
}

static GVariant *
stackswitcher_get_property (GDBusConnection  *connection,
                            const gchar      *sender,
                            const gchar      *object_path,
                            const gchar      *interface_name,
                            const gchar      *property_name,
                            GError          **error,
                            gpointer          user_data)
{
  BobguiATContext *self = BOBGUI_AT_CONTEXT (user_data);
  BobguiAccessible *accessible = bobgui_at_context_get_accessible (self);
  BobguiWidget *widget = BOBGUI_WIDGET (accessible);

  if (g_strcmp0 (property_name, "NSelectedChildren") == 0)
    {
      BobguiStack *stack = bobgui_stack_switcher_get_stack (BOBGUI_STACK_SWITCHER (widget));

      if (stack == NULL || bobgui_stack_get_visible_child (stack) == NULL)
        return g_variant_new_int32 (0);
      else
        return g_variant_new_int32 (1);
    }

  return NULL;
}

static const GDBusInterfaceVTable stackswitcher_vtable = {
  stackswitcher_handle_method,
  stackswitcher_get_property,
  NULL
};

/* }}} */
/* {{{ BobguiNotebook */

static void
notebook_handle_method (GDBusConnection       *connection,
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
  BobguiWidget *notebook = bobgui_widget_get_parent (bobgui_widget_get_parent (widget));

  if (g_strcmp0 (method_name, "GetSelectedChild") == 0)
    {
      int i;
      BobguiWidget *child;

      i = bobgui_notebook_get_current_page (BOBGUI_NOTEBOOK (notebook));

      for (child = bobgui_widget_get_first_child (widget);
           child;
           child = bobgui_widget_get_next_sibling (child))
        {
          /* skip actions */
          if (bobgui_accessible_get_accessible_role (BOBGUI_ACCESSIBLE (child)) != BOBGUI_ACCESSIBLE_ROLE_TAB)
            continue;

          if (i == 0)
            break;

          i--;
        }

      if (child == NULL)
        {
          g_dbus_method_invocation_return_error_literal (invocation,
                                                         G_DBUS_ERROR,
                                                         G_DBUS_ERROR_INVALID_ARGS,
                                                         "No selected child");
        }
      else
        {
          BobguiATContext *ctx = bobgui_accessible_get_at_context (BOBGUI_ACCESSIBLE (child));
          g_dbus_method_invocation_return_value (invocation,
            g_variant_new ("(@(so))", bobgui_at_spi_context_to_ref (BOBGUI_AT_SPI_CONTEXT (ctx))));
          g_object_unref (ctx);
        }
    }
  else if (g_strcmp0 (method_name, "SelectChild") == 0)
    {
      int i;
      BobguiWidget *child;

      g_variant_get (parameters, "(i)", &i);

      /* skip an action widget */
      child = bobgui_widget_get_first_child (widget);
      if (bobgui_accessible_get_accessible_role (BOBGUI_ACCESSIBLE (child)) != BOBGUI_ACCESSIBLE_ROLE_TAB)
        i--;

      bobgui_notebook_set_current_page (BOBGUI_NOTEBOOK (notebook), i);

      g_dbus_method_invocation_return_value (invocation, g_variant_new ("(b)", TRUE));
    }
  else if (g_strcmp0 (method_name, "DeselectChild") == 0)
    {
      g_dbus_method_invocation_return_value (invocation, g_variant_new ("(b)", FALSE));
    }
  else if (g_strcmp0 (method_name, "DeselectSelectedChild") == 0)
    {
      g_dbus_method_invocation_return_value (invocation, g_variant_new ("(b)", FALSE));
    }
  else if (g_strcmp0 (method_name, "IsChildSelected") == 0)
    {
      int i;
      gboolean active;
      BobguiWidget *child;

      g_variant_get (parameters, "(i)", &i);

      /* skip an action widget */
      child = bobgui_widget_get_first_child (widget);
      if (bobgui_accessible_get_accessible_role (BOBGUI_ACCESSIBLE (child)) != BOBGUI_ACCESSIBLE_ROLE_TAB)
        i--;

      active = i == bobgui_notebook_get_current_page (BOBGUI_NOTEBOOK (notebook));

      g_dbus_method_invocation_return_value (invocation, g_variant_new ("(b)", active));
    }
  else if (g_strcmp0 (method_name, "SelectAll") == 0)
    {
      g_dbus_method_invocation_return_value (invocation, g_variant_new ("(b)", FALSE));
    }
  else if (g_strcmp0 (method_name, "ClearSelection") == 0)
    {
      g_dbus_method_invocation_return_value (invocation, g_variant_new ("(b)", FALSE));
    }
}

static GVariant *
notebook_get_property (GDBusConnection  *connection,
                       const gchar      *sender,
                       const gchar      *object_path,
                       const gchar      *interface_name,
                       const gchar      *property_name,
                       GError          **error,
                       gpointer          user_data)
{
  if (g_strcmp0 (property_name, "NSelectedChildren") == 0)
    {
      return g_variant_new_int32 (1);
    }

  return NULL;
}

static const GDBusInterfaceVTable notebook_vtable = {
  notebook_handle_method,
  notebook_get_property,
  NULL
};

/* }}} */

#define IS_NOTEBOOK_TAB_LIST(s,r) \
  ((r == BOBGUI_ACCESSIBLE_ROLE_TAB_LIST) && \
   (bobgui_widget_get_parent (BOBGUI_WIDGET (s)) != NULL) && \
   BOBGUI_IS_NOTEBOOK (bobgui_widget_get_parent (bobgui_widget_get_parent (BOBGUI_WIDGET (s)))))

const GDBusInterfaceVTable *
bobgui_atspi_get_selection_vtable (BobguiAccessible     *accessible,
                                BobguiAccessibleRole  role)
{
  if (BOBGUI_IS_LIST_BOX (accessible))
    return &listbox_vtable;
  else if (BOBGUI_IS_LIST_VIEW (accessible) ||
           BOBGUI_IS_GRID_VIEW (accessible))
    return &listview_vtable;
  else if (BOBGUI_IS_FLOW_BOX (accessible))
    return &flowbox_vtable;
  else if (BOBGUI_IS_COMBO_BOX (accessible))
    return &combobox_vtable;
  else if (BOBGUI_IS_STACK_SWITCHER (accessible))
    return &stackswitcher_vtable;
  else if (IS_NOTEBOOK_TAB_LIST (accessible, role))
    return &notebook_vtable;

  return NULL;
}

typedef struct {
  BobguiAtspiSelectionCallback *changed;
  gpointer data;
} SelectionChanged;

/* {{{ BobguiListView notification */

typedef struct {
  BobguiSelectionModel *model;
  BobguiAtspiSelectionCallback *changed;
  gpointer data;
} ListViewData;

static void
update_model (ListViewData      *data,
              BobguiSelectionModel *model)
{
  if (data->model)
    g_signal_handlers_disconnect_by_func (data->model, data->changed, data->data);

  g_set_object (&data->model, model);

  if (data->model)
    g_signal_connect_swapped (data->model, "selection-changed", G_CALLBACK (data->changed), data->data);
}

static void
list_view_data_free (gpointer user_data)
{
  ListViewData *data = user_data;
  update_model (data, NULL);
  g_free (data);
}

static void
model_changed (BobguiListBase  *list,
               GParamSpec   *pspec,
               gpointer      unused)
{
  ListViewData *data;

  data = (ListViewData *)g_object_get_data (G_OBJECT (list), "accessible-selection-data");
  update_model (data, bobgui_list_base_get_model (list));
}

/* }}} */
/* {{{ Stackswitcher notification */

typedef struct {
  BobguiStack *stack;
  BobguiAtspiSelectionCallback *changed;
  gpointer data;
} StackSwitcherData;

static void
update_stack (StackSwitcherData *data,
              BobguiStack          *stack)
{
  if (data->stack)
    g_signal_handlers_disconnect_by_func (data->stack, data->changed, data->data);

  g_set_object (&data->stack, stack);

  if (data->stack)
    g_signal_connect_swapped (data->stack, "notify::visible-child", G_CALLBACK (data->changed), data->data);
}

static void
stack_switcher_data_free (gpointer user_data)
{
  StackSwitcherData *data = user_data;
  update_stack (data, NULL);
  g_free (data);
}

static void
stack_changed (BobguiStackSwitcher *self,
               GParamSpec       *pspec,
               gpointer          unused)
{
  StackSwitcherData *data;

  data = (StackSwitcherData *) g_object_get_data (G_OBJECT (self), "accessible-selection-data");
  update_stack (data, bobgui_stack_switcher_get_stack (self));
}

/* }}} */

void
bobgui_atspi_connect_selection_signals (BobguiAccessible *accessible,
                                     BobguiAtspiSelectionCallback selection_changed,
                                     gpointer   data)
{
  if (BOBGUI_IS_LIST_BOX (accessible))
    {
      SelectionChanged *changed;

      changed = g_new0 (SelectionChanged, 1);
      changed->changed = selection_changed;
      changed->data = data;

      g_object_set_data_full (G_OBJECT (accessible), "accessible-selection-data", changed, g_free);

      g_signal_connect_swapped (accessible, "selected-rows-changed", G_CALLBACK (selection_changed), data);
    }
  else if (BOBGUI_IS_FLOW_BOX (accessible))
    {
      SelectionChanged *changed;

      changed = g_new0 (SelectionChanged, 1);
      changed->changed = selection_changed;
      changed->data = data;

      g_object_set_data_full (G_OBJECT (accessible), "accessible-selection-data", changed, g_free);

      g_signal_connect_swapped (accessible, "selected-children-changed", G_CALLBACK (selection_changed), data);
    }
  else if (BOBGUI_IS_COMBO_BOX (accessible))
    {
      SelectionChanged *changed;

      changed = g_new0 (SelectionChanged, 1);
      changed->changed = selection_changed;
      changed->data = data;

      g_object_set_data_full (G_OBJECT (accessible), "accessible-selection-data", changed, g_free);

      g_signal_connect_swapped (accessible, "changed", G_CALLBACK (selection_changed), data);
    }
  else if (BOBGUI_IS_STACK_SWITCHER (accessible))
    {
      StackSwitcherData *changed;

      changed = g_new0 (StackSwitcherData, 1);
      changed->changed = selection_changed;
      changed->data = data;

      g_object_set_data_full (G_OBJECT (accessible), "accessible-selection-data", changed, stack_switcher_data_free);

      g_signal_connect (accessible, "notify::stack", G_CALLBACK (stack_changed), NULL);
      stack_changed (BOBGUI_STACK_SWITCHER (accessible), NULL, NULL);
    }
  else if (IS_NOTEBOOK_TAB_LIST (accessible, BOBGUI_AT_CONTEXT (data)->accessible_role))
    {
      BobguiWidget *notebook = bobgui_widget_get_parent (bobgui_widget_get_parent (BOBGUI_WIDGET (accessible)));
      SelectionChanged *changed;

      changed = g_new0 (SelectionChanged, 1);
      changed->changed = selection_changed;
      changed->data = data;

      g_object_set_data_full (G_OBJECT (accessible), "accessible-selection-data", changed, g_free);

      g_signal_connect_swapped (notebook, "notify::page", G_CALLBACK (selection_changed), data);
    }
  else if (BOBGUI_IS_LIST_VIEW (accessible) ||
           BOBGUI_IS_GRID_VIEW (accessible))
    {
      ListViewData *changed;

      changed = g_new0 (ListViewData, 1);
      changed->changed = selection_changed;
      changed->data = data;

      g_object_set_data_full (G_OBJECT (accessible), "accessible-selection-data", changed, list_view_data_free);

      g_signal_connect (accessible, "notify::model", G_CALLBACK (model_changed), NULL);
      model_changed (BOBGUI_LIST_BASE (accessible), NULL, changed);
    }
}

void
bobgui_atspi_disconnect_selection_signals (BobguiAccessible *accessible)
{
  if (BOBGUI_IS_LIST_BOX (accessible) ||
      BOBGUI_IS_FLOW_BOX (accessible) ||
      BOBGUI_IS_COMBO_BOX (accessible))
    {
      SelectionChanged *changed;

      changed = g_object_get_data (G_OBJECT (accessible), "accessible-selection-data");
      if (changed == NULL)
        return;

      g_signal_handlers_disconnect_by_func (accessible, changed->changed, changed->data);

      g_object_set_data (G_OBJECT (accessible), "accessible-selection-data", NULL);
    }
  else if (BOBGUI_IS_STACK_SWITCHER (accessible))
    {
      g_signal_handlers_disconnect_by_func (accessible, stack_changed, NULL);

      g_object_set_data (G_OBJECT (accessible), "accessible-selection-data", NULL);
    }
  else if (IS_NOTEBOOK_TAB_LIST (accessible, bobgui_accessible_get_accessible_role (accessible)))
    {
      BobguiWidget *notebook = bobgui_widget_get_parent (bobgui_widget_get_parent (BOBGUI_WIDGET (accessible)));
      SelectionChanged *changed;

      changed = g_object_get_data (G_OBJECT (accessible), "accessible-selection-data");
      if (changed == NULL)
        return;

      g_signal_handlers_disconnect_by_func (notebook, changed->changed, changed->data);

      g_object_set_data (G_OBJECT (accessible), "accessible-selection-data", NULL);
    }
  else if (BOBGUI_IS_LIST_VIEW (accessible) ||
           BOBGUI_IS_GRID_VIEW (accessible))
    {
      g_signal_handlers_disconnect_by_func (accessible, model_changed, NULL);

      g_object_set_data (G_OBJECT (accessible), "accessible-selection-data", NULL);
    }
}

/* vim:set foldmethod=marker: */

