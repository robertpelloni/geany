/*
 * Copyright © 2020 Red Hat, Inc.
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
 * Authors: Matthias Clasen
 */

#include "config.h"

#include "bobguiselectionfiltermodel.h"
#include "bobguibitset.h"

#include "bobguiprivate.h"

/**
 * BobguiSelectionFilterModel:
 *
 * A list model that presents the selection from a `BobguiSelectionModel`.
 */

enum {
  PROP_0,
  PROP_ITEM_TYPE,
  PROP_MODEL,
  PROP_N_ITEMS,

  NUM_PROPERTIES
};

struct _BobguiSelectionFilterModel
{
  GObject parent_instance;

  BobguiSelectionModel *model;
  BobguiBitset *selection;
};

struct _BobguiSelectionFilterModelClass
{
  GObjectClass parent_class;
};

static GParamSpec *properties[NUM_PROPERTIES] = { NULL, };

static GType
bobgui_selection_filter_model_get_item_type (GListModel *list)
{
  return G_TYPE_OBJECT;
}

static guint
bobgui_selection_filter_model_get_n_items (GListModel *list)
{
  BobguiSelectionFilterModel *self = BOBGUI_SELECTION_FILTER_MODEL (list);

  if (!self->selection)
    return 0;

  return bobgui_bitset_get_size (self->selection);
}

static gpointer
bobgui_selection_filter_model_get_item (GListModel *list,
                                     guint       position)
{
  BobguiSelectionFilterModel *self = BOBGUI_SELECTION_FILTER_MODEL (list);

  if (!self->selection)
    return NULL;

  if (position >= bobgui_bitset_get_size (self->selection))
    return NULL;

  position = bobgui_bitset_get_nth (self->selection, position);

  return g_list_model_get_item (G_LIST_MODEL (self->model), position);
}

static void
bobgui_selection_filter_model_list_model_init (GListModelInterface *iface)
{
  iface->get_item_type = bobgui_selection_filter_model_get_item_type;
  iface->get_n_items = bobgui_selection_filter_model_get_n_items;
  iface->get_item = bobgui_selection_filter_model_get_item;
}

G_DEFINE_TYPE_WITH_CODE (BobguiSelectionFilterModel, bobgui_selection_filter_model, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (G_TYPE_LIST_MODEL, bobgui_selection_filter_model_list_model_init))

static void
selection_filter_model_items_changed (BobguiSelectionFilterModel *self,
                                      guint                    position,
                                      guint                    removed,
                                      guint                    added)
{
  BobguiBitset *selection;
  guint sel_position = 0;
  guint sel_removed = 0;
  guint sel_added = 0;

  selection = bobgui_selection_model_get_selection (self->model);

  if (position > 0)
    sel_position = bobgui_bitset_get_size_in_range (self->selection, 0, position - 1);

  if (removed > 0)
    sel_removed = bobgui_bitset_get_size_in_range (self->selection, position, position + removed - 1);

  if (added > 0)
    sel_added = bobgui_bitset_get_size_in_range (selection, position, position + added - 1);

  bobgui_bitset_unref (self->selection);
  self->selection = bobgui_bitset_copy (selection);

  bobgui_bitset_unref (selection);

  if (sel_removed > 0 || sel_added > 0)
    g_list_model_items_changed (G_LIST_MODEL (self), sel_position, sel_removed, sel_added);
  if (sel_removed != sel_added)
    g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_N_ITEMS]);
}

static void
bobgui_selection_filter_model_items_changed_cb (GListModel              *model,
                                             guint                    position,
                                             guint                    removed,
                                             guint                    added,
                                             BobguiSelectionFilterModel *self)
{
  selection_filter_model_items_changed (self, position, removed, added);
}

static void
bobgui_selection_filter_model_selection_changed_cb (GListModel              *model,
                                                 guint                    position,
                                                 guint                    n_items,
                                                 BobguiSelectionFilterModel *self)
{
  selection_filter_model_items_changed (self, position, n_items, n_items);
}

static void
bobgui_selection_filter_model_set_property (GObject      *object,
                                         guint         prop_id,
                                         const GValue *value,
                                         GParamSpec   *pspec)
{
  BobguiSelectionFilterModel *self = BOBGUI_SELECTION_FILTER_MODEL (object);

  switch (prop_id)
    {
    case PROP_MODEL:
      bobgui_selection_filter_model_set_model (self, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_selection_filter_model_get_property (GObject     *object,
                                         guint        prop_id,
                                         GValue      *value,
                                         GParamSpec  *pspec)
{
  BobguiSelectionFilterModel *self = BOBGUI_SELECTION_FILTER_MODEL (object);

  switch (prop_id)
    {
    case PROP_ITEM_TYPE:
      g_value_set_gtype (value, bobgui_selection_filter_model_get_item_type (G_LIST_MODEL (self)));
      break;

    case PROP_MODEL:
      g_value_set_object (value, self->model);
      break;

    case PROP_N_ITEMS:
      g_value_set_uint (value, bobgui_selection_filter_model_get_n_items (G_LIST_MODEL (self)));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_selection_filter_model_clear_model (BobguiSelectionFilterModel *self)
{
  if (self->model == NULL)
    return;

  g_signal_handlers_disconnect_by_func (self->model, bobgui_selection_filter_model_items_changed_cb, self);
  g_signal_handlers_disconnect_by_func (self->model, bobgui_selection_filter_model_selection_changed_cb, self);

  g_clear_object (&self->model);
  g_clear_pointer (&self->selection, bobgui_bitset_unref);
}

static void
bobgui_selection_filter_model_dispose (GObject *object)
{
  BobguiSelectionFilterModel *self = BOBGUI_SELECTION_FILTER_MODEL (object);

  bobgui_selection_filter_model_clear_model (self);

  G_OBJECT_CLASS (bobgui_selection_filter_model_parent_class)->dispose (object);
}

static void
bobgui_selection_filter_model_class_init (BobguiSelectionFilterModelClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);

  gobject_class->set_property = bobgui_selection_filter_model_set_property;
  gobject_class->get_property = bobgui_selection_filter_model_get_property;
  gobject_class->dispose = bobgui_selection_filter_model_dispose;

  /**
   * BobguiSelectionFilterModel:item-type:
   *
   * The type of items. See [method@Gio.ListModel.get_item_type].
   *
   * Since: 4.8
   **/
  properties[PROP_ITEM_TYPE] =
    g_param_spec_gtype ("item-type", NULL, NULL,
                        G_TYPE_OBJECT,
                        G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  /**
   * BobguiSelectionFilterModel:model:
   *
   * The model being filtered.
   */
  properties[PROP_MODEL] =
      g_param_spec_object ("model", NULL, NULL,
                           BOBGUI_TYPE_SELECTION_MODEL,
                           BOBGUI_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiSelectionFilterModel:n-items:
   *
   * The number of items. See [method@Gio.ListModel.get_n_items].
   *
   * Since: 4.8
   **/
  properties[PROP_N_ITEMS] =
    g_param_spec_uint ("n-items", NULL, NULL,
                       0, G_MAXUINT, 0,
                       G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (gobject_class, NUM_PROPERTIES, properties);
}

static void
bobgui_selection_filter_model_init (BobguiSelectionFilterModel *self)
{
}

/**
 * bobgui_selection_filter_model_new:
 * @model: (nullable) (transfer none): the selection model to filter
 *
 * Creates a new `BobguiSelectionFilterModel` that will include the
 * selected items from the underlying selection model.
 *
 * Returns: a new `BobguiSelectionFilterModel`
 */
BobguiSelectionFilterModel *
bobgui_selection_filter_model_new (BobguiSelectionModel *model)
{
  return g_object_new (BOBGUI_TYPE_SELECTION_FILTER_MODEL,
                       "model", model,
                       NULL);
}

/**
 * bobgui_selection_filter_model_set_model:
 * @self: a `BobguiSelectionFilterModel`
 * @model: (nullable): The model to be filtered
 *
 * Sets the model to be filtered.
 *
 * Note that BOBGUI makes no effort to ensure that @model conforms to
 * the item type of @self. It assumes that the caller knows what they
 * are doing and have set up an appropriate filter to ensure that item
 * types match.
 **/
void
bobgui_selection_filter_model_set_model (BobguiSelectionFilterModel *self,
                                      BobguiSelectionModel       *model)
{
  guint removed, added;

  g_return_if_fail (BOBGUI_IS_SELECTION_FILTER_MODEL (self));
  g_return_if_fail (model == NULL || BOBGUI_IS_SELECTION_MODEL (model));

  if (self->model == model)
    return;

  removed = g_list_model_get_n_items (G_LIST_MODEL (self));
  bobgui_selection_filter_model_clear_model (self);

  if (model)
    {
      BobguiBitset *selection;

      self->model = g_object_ref (model);

      selection = bobgui_selection_model_get_selection (self->model);
      self->selection = bobgui_bitset_copy (selection);
      bobgui_bitset_unref (selection);

      g_signal_connect (model, "items-changed", G_CALLBACK (bobgui_selection_filter_model_items_changed_cb), self);
      g_signal_connect (model, "selection-changed", G_CALLBACK (bobgui_selection_filter_model_selection_changed_cb), self);
    }

  added = g_list_model_get_n_items (G_LIST_MODEL (self));

  if (removed > 0 || added > 0)
    g_list_model_items_changed (G_LIST_MODEL (self), 0, removed, added);
  if (removed != added)
    g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_N_ITEMS]);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_MODEL]);
}

/**
 * bobgui_selection_filter_model_get_model:
 * @self: a `BobguiSelectionFilterModel`
 *
 * Gets the model currently filtered or %NULL if none.
 *
 * Returns: (nullable) (transfer none): The model that gets filtered
 */
BobguiSelectionModel *
bobgui_selection_filter_model_get_model (BobguiSelectionFilterModel *self)
{
  g_return_val_if_fail (BOBGUI_IS_SELECTION_FILTER_MODEL (self), NULL);

  return self->model;
}
