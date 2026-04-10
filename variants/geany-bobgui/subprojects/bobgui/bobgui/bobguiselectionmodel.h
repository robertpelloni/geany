/*
 * Copyright © 2018 Benjamin Otte
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
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Benjamin Otte <otte@gnome.org>
 */

#pragma once

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <bobgui/bobguitypes.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_SELECTION_MODEL       (bobgui_selection_model_get_type ())

GDK_AVAILABLE_IN_ALL
G_DECLARE_INTERFACE (BobguiSelectionModel, bobgui_selection_model, BOBGUI, SELECTION_MODEL, GListModel)

/**
 * BobguiSelectionModelInterface:
 * @is_selected: Return if the item at the given position is selected.
 * @get_selection_in_range: Return a bitset with all currently selected
 *   items in the given range. By default, this function will call
 *   `BobguiSelectionModel::is_selected()` on all items in the given range.
 * @select_item: Select the item in the given position. If the operation
 *   is known to fail, return %FALSE.
 * @unselect_item: Unselect the item in the given position. If the
 *   operation is known to fail, return %FALSE.
 * @select_range: Select all items in the given range. If the operation
 *   is unsupported or known to fail for all items, return %FALSE.
 * @unselect_range: Unselect all items in the given range. If the
 *   operation is unsupported or known to fail for all items, return
 *   %FALSE.
 * @select_all: Select all items in the model. If the operation is
 *   unsupported or known to fail for all items, return %FALSE.
 * @unselect_all: Unselect all items in the model. If the operation is
 *   unsupported or known to fail for all items, return %FALSE.
 * @set_selection: Set selection state of all items in mask to selected.
 *   See bobgui_selection_model_set_selection() for a detailed explanation
 *   of this function.
 *
 * The list of virtual functions for the `BobguiSelectionModel` interface.
 * No function must be implemented, but unless `BobguiSelectionModel::is_selected()`
 * is implemented, it will not be possible to select items in the set.
 * 
 * The model does not need to implement any functions to support either
 * selecting or unselecting items. Of course, if the model does not do that,
 * it means that users cannot select or unselect items in a list widget
 * using the model.
 *
 * All selection functions fall back to `BobguiSelectionModel::set_selection()`
 * so it is sufficient to implement just that function for full selection
 * support.
 */
struct _BobguiSelectionModelInterface
{
  /*< private >*/
  GTypeInterface g_iface;

  /*< public >*/
  gboolean              (* is_selected)                         (BobguiSelectionModel      *model,
                                                                 guint                   position);
  BobguiBitset *           (* get_selection_in_range)              (BobguiSelectionModel      *model,
                                                                 guint                   position,
                                                                 guint                   n_items);

  gboolean              (* select_item)                         (BobguiSelectionModel      *model,
                                                                 guint                   position,
                                                                 gboolean                unselect_rest);
  gboolean              (* unselect_item)                       (BobguiSelectionModel      *model,
                                                                 guint                   position);
  gboolean              (* select_range)                        (BobguiSelectionModel      *model,
                                                                 guint                   position,
                                                                 guint                   n_items,
                                                                 gboolean                unselect_rest);
  gboolean              (* unselect_range)                      (BobguiSelectionModel      *model,
                                                                 guint                   position,
                                                                 guint                   n_items);
  gboolean              (* select_all)                          (BobguiSelectionModel      *model);
  gboolean              (* unselect_all)                        (BobguiSelectionModel      *model);
  gboolean              (* set_selection)                       (BobguiSelectionModel      *model,
                                                                 BobguiBitset              *selected,
                                                                 BobguiBitset              *mask);
};

GDK_AVAILABLE_IN_ALL
gboolean                bobgui_selection_model_is_selected         (BobguiSelectionModel      *model,
                                                                 guint                   position);
GDK_AVAILABLE_IN_ALL
BobguiBitset *             bobgui_selection_model_get_selection       (BobguiSelectionModel      *model);
GDK_AVAILABLE_IN_ALL
BobguiBitset *             bobgui_selection_model_get_selection_in_range
                                                                (BobguiSelectionModel      *model,
                                                                 guint                   position,
                                                                 guint                   n_items);

GDK_AVAILABLE_IN_ALL
gboolean                bobgui_selection_model_select_item         (BobguiSelectionModel      *model,
                                                                 guint                   position,
                                                                 gboolean                unselect_rest);
GDK_AVAILABLE_IN_ALL
gboolean                bobgui_selection_model_unselect_item       (BobguiSelectionModel      *model,
                                                                 guint                   position);
GDK_AVAILABLE_IN_ALL
gboolean                bobgui_selection_model_select_range        (BobguiSelectionModel      *model,
                                                                 guint                   position,
                                                                 guint                   n_items,
                                                                 gboolean                unselect_rest);
GDK_AVAILABLE_IN_ALL
gboolean                bobgui_selection_model_unselect_range      (BobguiSelectionModel      *model,
                                                                 guint                   position,
                                                                 guint                   n_items);
GDK_AVAILABLE_IN_ALL
gboolean                bobgui_selection_model_select_all          (BobguiSelectionModel      *model);
GDK_AVAILABLE_IN_ALL
gboolean                bobgui_selection_model_unselect_all        (BobguiSelectionModel      *model);
GDK_AVAILABLE_IN_ALL
gboolean                bobgui_selection_model_set_selection       (BobguiSelectionModel      *model,
                                                                 BobguiBitset              *selected,
                                                                 BobguiBitset              *mask);

/* for implementations only */
GDK_AVAILABLE_IN_ALL
void                    bobgui_selection_model_selection_changed   (BobguiSelectionModel      *model,
                                                                 guint                   position,
                                                                 guint                   n_items);

G_END_DECLS

