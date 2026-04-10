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

#include "config.h"

#include "bobguiselectionmodel.h"

#include "bobguibitset.h"
#include "bobguimarshalers.h"

/**
 * BobguiSelectionModel:
 *
 * An interface that adds support for selection to list models.
 *
 * This support is then used by widgets using list models to add the ability
 * to select and unselect various items.
 *
 * BOBGUI provides default implementations of the most common selection modes such
 * as [class@Bobgui.SingleSelection], so you will only need to implement this
 * interface if you want detailed control about how selections should be handled.
 *
 * A `BobguiSelectionModel` supports a single boolean per item indicating if an item is
 * selected or not. This can be queried via [method@Bobgui.SelectionModel.is_selected].
 * When the selected state of one or more items changes, the model will emit the
 * [signal@Bobgui.SelectionModel::selection-changed] signal by calling the
 * [method@Bobgui.SelectionModel.selection_changed] function. The positions given
 * in that signal may have their selection state changed, though that is not a
 * requirement. If new items added to the model via the
 * [signal@Gio.ListModel::items-changed] signal are selected or not is up to the
 * implementation.
 *
 * Note that items added via [signal@Gio.ListModel::items-changed] may already
 * be selected and no [signal@Bobgui.SelectionModel::selection-changed] will be
 * emitted for them. So to track which items are selected, it is necessary to
 * listen to both signals.
 *
 * Additionally, the interface can expose functionality to select and unselect
 * items. If these functions are implemented, BOBGUI's list widgets will allow users
 * to select and unselect items. However, `BobguiSelectionModel`s are free to only
 * implement them partially or not at all. In that case the widgets will not
 * support the unimplemented operations.
 *
 * When selecting or unselecting is supported by a model, the return values of
 * the selection functions do *not* indicate if selection or unselection happened.
 * They are only meant to indicate complete failure, like when this mode of
 * selecting is not supported by the model.
 *
 * Selections may happen asynchronously, so the only reliable way to find out
 * when an item was selected is to listen to the signals that indicate selection.
 */

G_DEFINE_INTERFACE (BobguiSelectionModel, bobgui_selection_model, G_TYPE_LIST_MODEL)

enum {
  SELECTION_CHANGED,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

static gboolean
bobgui_selection_model_default_is_selected (BobguiSelectionModel *model,
                                         guint              position)
{
  BobguiBitset *bitset;
  gboolean selected;

  bitset = bobgui_selection_model_get_selection_in_range (model, position, 1);
  selected = bobgui_bitset_contains (bitset, position);
  bobgui_bitset_unref (bitset);

  return selected;
}

static BobguiBitset *
bobgui_selection_model_default_get_selection_in_range (BobguiSelectionModel *model,
                                                    guint              position,
                                                    guint              n_items)
{
  BobguiBitset *bitset;
  guint i;

  bitset = bobgui_bitset_new_empty ();

  for (i = position; i < position + n_items; i++)
    {
      if (bobgui_selection_model_is_selected (model, i))
        bobgui_bitset_add (bitset, i);
    }

  return bitset;
}

static gboolean
bobgui_selection_model_default_select_item (BobguiSelectionModel *model,
                                         guint              position,
                                         gboolean           unselect_rest)
{
  BobguiBitset *selected;
  BobguiBitset *mask;
  gboolean result;

  selected = bobgui_bitset_new_empty ();
  bobgui_bitset_add (selected, position);
  if (unselect_rest)
    {
      mask = bobgui_bitset_new_empty ();
      bobgui_bitset_add_range (mask, 0, g_list_model_get_n_items (G_LIST_MODEL (model)));
    }
  else
    {
      mask = bobgui_bitset_ref (selected);
    }

  result = bobgui_selection_model_set_selection (model, selected, mask);

  bobgui_bitset_unref (selected);
  bobgui_bitset_unref (mask);

  return result;
}

static gboolean
bobgui_selection_model_default_unselect_item (BobguiSelectionModel *model,
                                           guint              position)
{
  BobguiBitset *selected;
  BobguiBitset *mask;
  gboolean result;

  selected = bobgui_bitset_new_empty ();
  mask = bobgui_bitset_new_empty ();
  bobgui_bitset_add (mask, position);

  result = bobgui_selection_model_set_selection (model, selected, mask);

  bobgui_bitset_unref (selected);
  bobgui_bitset_unref (mask);

  return result;
}

static gboolean
bobgui_selection_model_default_select_range (BobguiSelectionModel *model,
                                          guint              position,
                                          guint              n_items,
                                          gboolean           unselect_rest)
{
  BobguiBitset *selected;
  BobguiBitset *mask;
  gboolean result;

  selected = bobgui_bitset_new_empty ();
  bobgui_bitset_add_range (selected, position, n_items);
  if (unselect_rest)
    {
      mask = bobgui_bitset_new_empty ();
      bobgui_bitset_add_range (mask, 0, g_list_model_get_n_items (G_LIST_MODEL (model)));
    }
  else
    {
      mask = bobgui_bitset_ref (selected);
    }

  result = bobgui_selection_model_set_selection (model, selected, mask);

  bobgui_bitset_unref (selected);
  bobgui_bitset_unref (mask);

  return result;
}

static gboolean
bobgui_selection_model_default_unselect_range (BobguiSelectionModel *model,
                                            guint              position,
                                            guint              n_items)
{
  BobguiBitset *selected;
  BobguiBitset *mask;
  gboolean result;

  selected = bobgui_bitset_new_empty ();
  mask = bobgui_bitset_new_empty ();
  bobgui_bitset_add_range (mask, position, n_items);

  result = bobgui_selection_model_set_selection (model, selected, mask);

  bobgui_bitset_unref (selected);
  bobgui_bitset_unref (mask);

  return result;
}

static gboolean
bobgui_selection_model_default_select_all (BobguiSelectionModel *model)
{
  return bobgui_selection_model_select_range (model, 0, g_list_model_get_n_items (G_LIST_MODEL (model)), FALSE);
}

static gboolean
bobgui_selection_model_default_unselect_all (BobguiSelectionModel *model)
{
  return bobgui_selection_model_unselect_range (model, 0, g_list_model_get_n_items (G_LIST_MODEL (model)));
}

static gboolean
bobgui_selection_model_default_set_selection (BobguiSelectionModel *model,
                                           BobguiBitset         *selected,
                                           BobguiBitset         *mask)
{
  return FALSE;
}

static void
bobgui_selection_model_default_init (BobguiSelectionModelInterface *iface)
{
  iface->is_selected = bobgui_selection_model_default_is_selected;
  iface->get_selection_in_range = bobgui_selection_model_default_get_selection_in_range;
  iface->select_item = bobgui_selection_model_default_select_item;
  iface->unselect_item = bobgui_selection_model_default_unselect_item;
  iface->select_range = bobgui_selection_model_default_select_range;
  iface->unselect_range = bobgui_selection_model_default_unselect_range;
  iface->select_all = bobgui_selection_model_default_select_all;
  iface->unselect_all = bobgui_selection_model_default_unselect_all;
  iface->set_selection = bobgui_selection_model_default_set_selection;

  /**
   * BobguiSelectionModel::selection-changed
   * @model: a `BobguiSelectionModel`
   * @position: The first item that may have changed
   * @n_items: number of items with changes
   *
   * Emitted when the selection state of some of the items in @model changes.
   *
   * Note that this signal does not specify the new selection state of the
   * items, they need to be queried manually. It is also not necessary for
   * a model to change the selection state of any of the items in the selection
   * model, though it would be rather useless to emit such a signal.
   */
  signals[SELECTION_CHANGED] =
    g_signal_new ("selection-changed",
                  BOBGUI_TYPE_SELECTION_MODEL,
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL, NULL,
                  _bobgui_marshal_VOID__UINT_UINT,
                  G_TYPE_NONE, 2, G_TYPE_UINT, G_TYPE_UINT);
  g_signal_set_va_marshaller (signals[SELECTION_CHANGED],
                              BOBGUI_TYPE_SELECTION_MODEL,
                              _bobgui_marshal_VOID__UINT_UINTv);
}

/**
 * bobgui_selection_model_is_selected:
 * @model: a `BobguiSelectionModel`
 * @position: the position of the item to query
 *
 * Checks if the given item is selected.
 *
 * Returns: %TRUE if the item is selected
 */
gboolean
bobgui_selection_model_is_selected (BobguiSelectionModel *model,
                                 guint              position)
{
  BobguiSelectionModelInterface *iface;

  g_return_val_if_fail (BOBGUI_IS_SELECTION_MODEL (model), 0);

  iface = BOBGUI_SELECTION_MODEL_GET_IFACE (model);
  return iface->is_selected (model, position);
}

/**
 * bobgui_selection_model_get_selection:
 * @model: a `BobguiSelectionModel`
 *
 * Gets the set containing all currently selected items in the model.
 *
 * This function may be slow, so if you are only interested in single item,
 * consider using [method@Bobgui.SelectionModel.is_selected] or if you are only
 * interested in a few, consider [method@Bobgui.SelectionModel.get_selection_in_range].
 *
 * Returns: (transfer full): a `BobguiBitset` containing all the values currently
 *   selected in @model. If no items are selected, the bitset is empty.
 *   The bitset must not be modified.
 */
BobguiBitset *
bobgui_selection_model_get_selection (BobguiSelectionModel *model)
{
  g_return_val_if_fail (BOBGUI_IS_SELECTION_MODEL (model), bobgui_bitset_new_empty ());

  return bobgui_selection_model_get_selection_in_range (model, 0, g_list_model_get_n_items (G_LIST_MODEL (model)));
}

/**
 * bobgui_selection_model_get_selection_in_range:
 * @model: a `BobguiSelectionModel`
 * @position: start of the queried range
 * @n_items: number of items in the queried range
 *
 * Gets the set of selected items in a range.
 *
 * This function is an optimization for
 * [method@Bobgui.SelectionModel.get_selection] when you are only
 * interested in part of the model's selected state. A common use
 * case is in response to the [signal@Bobgui.SelectionModel::selection-changed]
 * signal.
 *
 * Returns: A `BobguiBitset` that matches the selection state
 *   for the given range with all other values being undefined.
 *   The bitset must not be modified.
 */
BobguiBitset *
bobgui_selection_model_get_selection_in_range (BobguiSelectionModel *model,
                                            guint              position,
                                            guint              n_items)
{
  BobguiSelectionModelInterface *iface;

  g_return_val_if_fail (BOBGUI_IS_SELECTION_MODEL (model), bobgui_bitset_new_empty ());

  if (n_items == 0)
    return bobgui_bitset_new_empty ();

  iface = BOBGUI_SELECTION_MODEL_GET_IFACE (model);
  return iface->get_selection_in_range (model, position, n_items);
}

/**
 * bobgui_selection_model_select_item:
 * @model: a `BobguiSelectionModel`
 * @position: the position of the item to select
 * @unselect_rest: whether previously selected items should be unselected
 *
 * Requests to select an item in the model.
 *
 * Returns: %TRUE if this action was supported and no fallback should be
 *   tried. This does not mean the item was selected.
 */
gboolean
bobgui_selection_model_select_item (BobguiSelectionModel *model,
                                 guint              position,
                                 gboolean           unselect_rest)
{
  BobguiSelectionModelInterface *iface;

  g_return_val_if_fail (BOBGUI_IS_SELECTION_MODEL (model), FALSE);

  iface = BOBGUI_SELECTION_MODEL_GET_IFACE (model);
  return iface->select_item (model, position, unselect_rest);
}

/**
 * bobgui_selection_model_unselect_item:
 * @model: a `BobguiSelectionModel`
 * @position: the position of the item to unselect
 *
 * Requests to unselect an item in the model.
 *
 * Returns: %TRUE if this action was supported and no fallback should be
 *   tried. This does not mean the item was unselected.
 */
gboolean
bobgui_selection_model_unselect_item (BobguiSelectionModel *model,
                                   guint              position)
{
  BobguiSelectionModelInterface *iface;

  g_return_val_if_fail (BOBGUI_IS_SELECTION_MODEL (model), FALSE);

  iface = BOBGUI_SELECTION_MODEL_GET_IFACE (model);
  return iface->unselect_item (model, position);
}

/**
 * bobgui_selection_model_select_range:
 * @model: a `BobguiSelectionModel`
 * @position: the first item to select
 * @n_items: the number of items to select
 * @unselect_rest: whether previously selected items should be unselected
 *
 * Requests to select a range of items in the model.
 *
 * Returns: %TRUE if this action was supported and no fallback should be
 *   tried. This does not mean the range was selected.
 */
gboolean
bobgui_selection_model_select_range (BobguiSelectionModel *model,
                                  guint              position,
                                  guint              n_items,
                                  gboolean           unselect_rest)
{
  BobguiSelectionModelInterface *iface;

  g_return_val_if_fail (BOBGUI_IS_SELECTION_MODEL (model), FALSE);

  iface = BOBGUI_SELECTION_MODEL_GET_IFACE (model);
  return iface->select_range (model, position, n_items, unselect_rest);
}

/**
 * bobgui_selection_model_unselect_range:
 * @model: a `BobguiSelectionModel`
 * @position: the first item to unselect
 * @n_items: the number of items to unselect
 *
 * Requests to unselect a range of items in the model.
 *
 * Returns: %TRUE if this action was supported and no fallback should be
 *   tried. This does not mean the range was unselected.
 */
gboolean
bobgui_selection_model_unselect_range (BobguiSelectionModel *model,
                                    guint              position,
                                    guint              n_items)
{
  BobguiSelectionModelInterface *iface;

  g_return_val_if_fail (BOBGUI_IS_SELECTION_MODEL (model), FALSE);

  iface = BOBGUI_SELECTION_MODEL_GET_IFACE (model);
  return iface->unselect_range (model, position, n_items);
}

/**
 * bobgui_selection_model_select_all:
 * @model: a `BobguiSelectionModel`
 *
 * Requests to select all items in the model.
 *
 * Returns: %TRUE if this action was supported and no fallback should be
 *   tried. This does not mean that all items are now selected.
 */
gboolean
bobgui_selection_model_select_all (BobguiSelectionModel *model)
{
  BobguiSelectionModelInterface *iface;

  g_return_val_if_fail (BOBGUI_IS_SELECTION_MODEL (model), FALSE);

  iface = BOBGUI_SELECTION_MODEL_GET_IFACE (model);
  return iface->select_all (model);
}

/**
 * bobgui_selection_model_unselect_all:
 * @model: a `BobguiSelectionModel`
 *
 * Requests to unselect all items in the model.
 *
 * Returns: %TRUE if this action was supported and no fallback should be
 *   tried. This does not mean that all items are now unselected.
 */
gboolean
bobgui_selection_model_unselect_all (BobguiSelectionModel *model)
{
  BobguiSelectionModelInterface *iface;

  g_return_val_if_fail (BOBGUI_IS_SELECTION_MODEL (model), FALSE);

  iface = BOBGUI_SELECTION_MODEL_GET_IFACE (model);
  return iface->unselect_all (model);
}

/**
 * bobgui_selection_model_set_selection:
 * @model: a `BobguiSelectionModel`
 * @selected: bitmask specifying if items should be selected or unselected
 * @mask: bitmask specifying which items should be updated
 *
 * Make selection changes.
 *
 * This is the most advanced selection updating method that allows
 * the most fine-grained control over selection changes. If you can,
 * you should try the simpler versions, as implementations are more
 * likely to implement support for those.
 *
 * Requests that the selection state of all positions set in @mask
 * be updated to the respective value in the @selected bitmask.
 *
 * In pseudocode, it would look something like this:
 *
 * ```c
 * for (i = 0; i < n_items; i++)
 *   {
 *     // don't change values not in the mask
 *     if (!bobgui_bitset_contains (mask, i))
 *       continue;
 *
 *     if (bobgui_bitset_contains (selected, i))
 *       select_item (i);
 *     else
 *       unselect_item (i);
 *   }
 *
 * bobgui_selection_model_selection_changed (model,
 *                                        first_changed_item,
 *                                        n_changed_items);
 * ```
 *
 * @mask and @selected must not be modified. They may refer to the
 * same bitset, which would mean that every item in the set should
 * be selected.
 *
 * Returns: %TRUE if this action was supported and no fallback should be
 *   tried. This does not mean that all items were updated according
 *   to the inputs.
 */
gboolean
bobgui_selection_model_set_selection (BobguiSelectionModel *model,
                                   BobguiBitset         *selected,
                                   BobguiBitset         *mask)
{
  BobguiSelectionModelInterface *iface;

  g_return_val_if_fail (BOBGUI_IS_SELECTION_MODEL (model), FALSE);
  g_return_val_if_fail (selected != NULL, FALSE);
  g_return_val_if_fail (mask != NULL, FALSE);

  iface = BOBGUI_SELECTION_MODEL_GET_IFACE (model);
  return iface->set_selection (model, selected, mask);
}

/**
 * bobgui_selection_model_selection_changed:
 * @model: a `BobguiSelectionModel`
 * @position: the first changed item
 * @n_items: the number of changed items
 *
 * Helper function for implementations of `BobguiSelectionModel`.
 *
 * Call this when the selection changes to emit the
 * [signal@Bobgui.SelectionModel::selection-changed] signal.
 */
void
bobgui_selection_model_selection_changed (BobguiSelectionModel *model,
                                       guint              position,
                                       guint              n_items)
{
  g_return_if_fail (BOBGUI_IS_SELECTION_MODEL (model));
  g_return_if_fail (n_items > 0);
  g_return_if_fail (position + n_items <= g_list_model_get_n_items (G_LIST_MODEL (model)));

  g_signal_emit (model, signals[SELECTION_CHANGED], 0, position, n_items);
}
