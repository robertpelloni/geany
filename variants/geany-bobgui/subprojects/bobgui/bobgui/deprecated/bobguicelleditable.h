/* bobguicelleditable.h
 * Copyright (C) 2001  Red Hat, Inc.
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

#pragma once

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <bobgui/bobguiwidget.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_CELL_EDITABLE            (bobgui_cell_editable_get_type ())
#define BOBGUI_CELL_EDITABLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_CELL_EDITABLE, BobguiCellEditable))
#define BOBGUI_IS_CELL_EDITABLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_CELL_EDITABLE))
#define BOBGUI_CELL_EDITABLE_GET_IFACE(obj)  (G_TYPE_INSTANCE_GET_INTERFACE ((obj), BOBGUI_TYPE_CELL_EDITABLE, BobguiCellEditableIface))

typedef struct _BobguiCellEditable      BobguiCellEditable; /* Dummy typedef */
typedef struct _BobguiCellEditableIface BobguiCellEditableIface;

/**
 * BobguiCellEditableIface:
 * @editing_done: Signal is a sign for the cell renderer to update its
 *    value from the cell_editable.
 * @remove_widget: Signal is meant to indicate that the cell is
 *    finished editing, and the widget may now be destroyed.
 * @start_editing: Begins editing on a cell_editable.
 */
struct _BobguiCellEditableIface
{
  /*< private >*/
  GTypeInterface g_iface;

  /*< public >*/

  /* signals */
  void (* editing_done)  (BobguiCellEditable *cell_editable);
  void (* remove_widget) (BobguiCellEditable *cell_editable);

  /* virtual table */
  void (* start_editing) (BobguiCellEditable *cell_editable,
			  GdkEvent        *event);
};


GDK_AVAILABLE_IN_ALL
GType bobgui_cell_editable_get_type      (void) G_GNUC_CONST;

GDK_DEPRECATED_IN_4_10
void  bobgui_cell_editable_start_editing (BobguiCellEditable *cell_editable,
				       GdkEvent        *event);
GDK_DEPRECATED_IN_4_10
void  bobgui_cell_editable_editing_done  (BobguiCellEditable *cell_editable);
GDK_DEPRECATED_IN_4_10
void  bobgui_cell_editable_remove_widget (BobguiCellEditable *cell_editable);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiCellEditable, g_object_unref)

G_END_DECLS

