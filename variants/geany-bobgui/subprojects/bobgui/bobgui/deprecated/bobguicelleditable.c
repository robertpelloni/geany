/* bobguicelleditable.c
 * Copyright (C) 2000  Red Hat, Inc.,  Jonathan Blandford <jrb@redhat.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * BobguiCellEditable:
 *
 * Interface for widgets that can be used for editing cells
 *
 * The `BobguiCellEditable` interface must be implemented for widgets to be usable
 * to edit the contents of a `BobguiTreeView` cell. It provides a way to specify how
 * temporary widgets should be configured for editing, get the new value, etc.
 *
 * Deprecated: 4.10: List views use widgets for displaying their
 *   contents. See [iface@Bobgui.Editable] for editable text widgets
 */

#include "config.h"
#include "bobguicelleditable.h"
#include "bobguimarshalers.h"
#include "bobguiprivate.h"

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

typedef BobguiCellEditableIface BobguiCellEditableInterface;
G_DEFINE_INTERFACE(BobguiCellEditable, bobgui_cell_editable, BOBGUI_TYPE_WIDGET)

static void
bobgui_cell_editable_default_init (BobguiCellEditableInterface *iface)
{
  /**
   * BobguiCellEditable:editing-canceled:
   *
   * Indicates whether editing on the cell has been canceled.
   */
  g_object_interface_install_property (iface,
                                       g_param_spec_boolean ("editing-canceled", NULL, NULL,
                                       FALSE,
                                       BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * BobguiCellEditable::editing-done:
   * @cell_editable: the object on which the signal was emitted
   *
   * This signal is a sign for the cell renderer to update its
   * value from the @cell_editable.
   *
   * Implementations of `BobguiCellEditable` are responsible for
   * emitting this signal when they are done editing, e.g.
   * `BobguiEntry` emits this signal when the user presses Enter. Typical things to
   * do in a handler for ::editing-done are to capture the edited value,
   * disconnect the @cell_editable from signals on the `BobguiCellRenderer`, etc.
   *
   * bobgui_cell_editable_editing_done() is a convenience method
   * for emitting `BobguiCellEditable::editing-done`.
   */
  g_signal_new (I_("editing-done"),
                BOBGUI_TYPE_CELL_EDITABLE,
                G_SIGNAL_RUN_LAST,
                G_STRUCT_OFFSET (BobguiCellEditableIface, editing_done),
                NULL, NULL,
                NULL,
                G_TYPE_NONE, 0);

  /**
   * BobguiCellEditable::remove-widget:
   * @cell_editable: the object on which the signal was emitted
   *
   * This signal is meant to indicate that the cell is finished
   * editing, and the @cell_editable widget is being removed and may
   * subsequently be destroyed.
   *
   * Implementations of `BobguiCellEditable` are responsible for
   * emitting this signal when they are done editing. It must
   * be emitted after the `BobguiCellEditable::editing-done` signal,
   * to give the cell renderer a chance to update the cell's value
   * before the widget is removed.
   *
   * bobgui_cell_editable_remove_widget() is a convenience method
   * for emitting `BobguiCellEditable::remove-widget`.
   */
  g_signal_new (I_("remove-widget"),
                BOBGUI_TYPE_CELL_EDITABLE,
                G_SIGNAL_RUN_LAST,
                G_STRUCT_OFFSET (BobguiCellEditableIface, remove_widget),
                NULL, NULL,
                NULL,
                G_TYPE_NONE, 0);
}

/**
 * bobgui_cell_editable_start_editing:
 * @cell_editable: A `BobguiCellEditable`
 * @event: (nullable): The `GdkEvent` that began the editing process, or
 *   %NULL if editing was initiated programmatically
 *
 * Begins editing on a @cell_editable.
 *
 * The `BobguiCellRenderer` for the cell creates and returns a `BobguiCellEditable` from
 * bobgui_cell_renderer_start_editing(), configured for the `BobguiCellRenderer` type.
 *
 * bobgui_cell_editable_start_editing() can then set up @cell_editable suitably for
 * editing a cell, e.g. making the Esc key emit `BobguiCellEditable::editing-done`.
 *
 * Note that the @cell_editable is created on-demand for the current edit; its
 * lifetime is temporary and does not persist across other edits and/or cells.
 **/
void
bobgui_cell_editable_start_editing (BobguiCellEditable *cell_editable,
				 GdkEvent        *event)
{
  g_return_if_fail (BOBGUI_IS_CELL_EDITABLE (cell_editable));

  (* BOBGUI_CELL_EDITABLE_GET_IFACE (cell_editable)->start_editing) (cell_editable, event);
}

/**
 * bobgui_cell_editable_editing_done:
 * @cell_editable: A `BobguiCellEditable`
 *
 * Emits the `BobguiCellEditable::editing-done` signal.
 *
 * Deprecated: 4.10
 */
void
bobgui_cell_editable_editing_done (BobguiCellEditable *cell_editable)
{
  g_return_if_fail (BOBGUI_IS_CELL_EDITABLE (cell_editable));

  g_signal_emit_by_name (cell_editable, "editing-done");
}

/**
 * bobgui_cell_editable_remove_widget:
 * @cell_editable: A `BobguiCellEditable`
 *
 * Emits the `BobguiCellEditable::remove-widget` signal.
 *
 * Deprecated: 4.10
 **/
void
bobgui_cell_editable_remove_widget (BobguiCellEditable *cell_editable)
{
  g_return_if_fail (BOBGUI_IS_CELL_EDITABLE (cell_editable));

  g_signal_emit_by_name (cell_editable, "remove-widget");
}
