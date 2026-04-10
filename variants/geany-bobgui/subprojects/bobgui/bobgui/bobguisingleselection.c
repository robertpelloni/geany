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

#include "bobguisingleselection.h"

#include "bobguibitset.h"
#include "bobguisectionmodelprivate.h"
#include "bobguiselectionmodel.h"

/**
 * BobguiSingleSelection:
 *
 * A selection model that allows selecting a single item.
 *
 * Note that the selection is *persistent* -- if the selected item is removed
 * and re-added in the same [signal@Gio.ListModel::items-changed] emission, it
 * stays selected. In particular, this means that changing the sort order of an
 * underlying sort model will preserve the selection.
 */
struct _BobguiSingleSelection
{
  GObject parent_instance;

  GListModel *model;
  guint selected;
  gpointer selected_item;

  guint autoselect : 1;
  guint can_unselect : 1;
};

struct _BobguiSingleSelectionClass
{
  GObjectClass parent_class;
};

enum {
  PROP_0,
  PROP_AUTOSELECT,
  PROP_CAN_UNSELECT,
  PROP_ITEM_TYPE,
  PROP_MODEL,
  PROP_N_ITEMS,
  PROP_SELECTED,
  PROP_SELECTED_ITEM,
  N_PROPS
};

static GParamSpec *properties[N_PROPS] = { NULL, };

static GType
bobgui_single_selection_get_item_type (GListModel *list)
{
  return G_TYPE_OBJECT;
}

static guint
bobgui_single_selection_get_n_items (GListModel *list)
{
  BobguiSingleSelection *self = BOBGUI_SINGLE_SELECTION (list);

  if (self->model == NULL)
    return 0;

  return g_list_model_get_n_items (self->model);
}

static gpointer
bobgui_single_selection_get_item (GListModel *list,
                               guint       position)
{
  BobguiSingleSelection *self = BOBGUI_SINGLE_SELECTION (list);

  if (self->model == NULL)
    return NULL;

  return g_list_model_get_item (self->model, position);
}

static void
bobgui_single_selection_list_model_init (GListModelInterface *iface)
{
  iface->get_item_type = bobgui_single_selection_get_item_type;
  iface->get_n_items = bobgui_single_selection_get_n_items;
  iface->get_item = bobgui_single_selection_get_item;
}

static void
bobgui_single_selection_get_section (BobguiSectionModel *model,
                                  guint            position,
                                  guint           *out_start,
                                  guint           *out_end)
{
  BobguiSingleSelection *self = BOBGUI_SINGLE_SELECTION (model);

  bobgui_list_model_get_section (self->model, position, out_start, out_end);
}

static void
bobgui_single_selection_section_model_init (BobguiSectionModelInterface *iface)
{
  iface->get_section = bobgui_single_selection_get_section;
}

static gboolean
bobgui_single_selection_is_selected (BobguiSelectionModel *model,
                                  guint              position)
{
  BobguiSingleSelection *self = BOBGUI_SINGLE_SELECTION (model);

  return self->selected == position;
}

static BobguiBitset *
bobgui_single_selection_get_selection_in_range (BobguiSelectionModel *model,
                                             guint              position,
                                             guint              n_items)
{
  BobguiSingleSelection *self = BOBGUI_SINGLE_SELECTION (model);
  BobguiBitset *result;

  result = bobgui_bitset_new_empty ();
  if (self->selected != BOBGUI_INVALID_LIST_POSITION)
    bobgui_bitset_add (result, self->selected);

  return result;
}

static gboolean
bobgui_single_selection_select_item (BobguiSelectionModel *model,
                                  guint              position,
                                  gboolean           exclusive)
{
  BobguiSingleSelection *self = BOBGUI_SINGLE_SELECTION (model);

  /* XXX: Should we check that position < n_items here? */
  bobgui_single_selection_set_selected (self, position);

  return TRUE;
}

static gboolean
bobgui_single_selection_unselect_item (BobguiSelectionModel *model,
                                    guint              position)
{
  BobguiSingleSelection *self = BOBGUI_SINGLE_SELECTION (model);

  if (!self->can_unselect || self->autoselect)
    return FALSE;

  if (self->selected == position)
    bobgui_single_selection_set_selected (self, BOBGUI_INVALID_LIST_POSITION);

  return TRUE;
}

static gboolean
bobgui_single_selection_unselect_all (BobguiSelectionModel *model)
{
  BobguiSingleSelection *self = BOBGUI_SINGLE_SELECTION (model);

  return bobgui_single_selection_unselect_item (model, self->selected);
}

static void
bobgui_single_selection_selection_model_init (BobguiSelectionModelInterface *iface)
{
  iface->is_selected = bobgui_single_selection_is_selected; 
  iface->get_selection_in_range = bobgui_single_selection_get_selection_in_range; 
  iface->select_item = bobgui_single_selection_select_item; 
  iface->unselect_all = bobgui_single_selection_unselect_all;
  iface->unselect_item = bobgui_single_selection_unselect_item; 
}

G_DEFINE_TYPE_EXTENDED (BobguiSingleSelection, bobgui_single_selection, G_TYPE_OBJECT, 0,
                        G_IMPLEMENT_INTERFACE (G_TYPE_LIST_MODEL,
                                               bobgui_single_selection_list_model_init)
                        G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_SECTION_MODEL,
                                               bobgui_single_selection_section_model_init)
                        G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_SELECTION_MODEL,
                                               bobgui_single_selection_selection_model_init))

static void
bobgui_single_selection_items_changed_cb (GListModel         *model,
                                       guint               position,
                                       guint               removed,
                                       guint               added,
                                       BobguiSingleSelection *self)
{
  g_object_freeze_notify (G_OBJECT (self));

  if (self->selected_item == NULL)
    {
      if (self->autoselect)
        {
          self->selected_item = g_list_model_get_item (self->model, 0);
          if (self->selected_item)
            {
              self->selected = 0;
              g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_SELECTED]);
              g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_SELECTED_ITEM]);
            }
        }
    }
  else if (self->selected < position)
    {
      /* unchanged */
    }
  else if (self->selected >= position + removed)
    {
      self->selected += added - removed;
      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_SELECTED]);
    }
  else
    {
      guint i;

      for (i = 0; i < added; i++)
        {
          gpointer item = g_list_model_get_item (model, position + i);
          if (item == self->selected_item)
            {
              /* the item moved */
              if (self->selected != position + i)
                {
                  self->selected = position + i;
                  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_SELECTED]);
                }

              g_object_unref (item);
              break;
            }
          g_object_unref (item);
        }
      if (i == added)
        {
          guint old_selected = self->selected;

          /* the item really was deleted */
          g_clear_object (&self->selected_item);
          if (self->autoselect)
            {
              self->selected = position + (self->selected - position) * added / removed;
              self->selected_item = g_list_model_get_item (self->model, self->selected);
              if (self->selected_item == NULL)
                {
                  if (position > 0)
                    {
                      self->selected = position - 1;
                      self->selected_item = g_list_model_get_item (self->model, self->selected);
                      g_assert (self->selected_item);
                      /* We pretend the newly selected item was part of the original model change.
                       * This way we get around inconsistent state (no item selected) during
                       * the items-changed emission. */
                      position--;
                      removed++;
                      added++;
                    }
                  else
                    self->selected = BOBGUI_INVALID_LIST_POSITION;
                }
              else
                {
                  if (self->selected == position + added)
                    {
                      /* We pretend the newly selected item was part of the original model change.
                       * This way we get around inconsistent state (no item selected) during
                       * the items-changed emission. */
                      removed++;
                      added++;
                    }
                }
            }
          else
            {
              g_clear_object (&self->selected_item);
              self->selected = BOBGUI_INVALID_LIST_POSITION;
            }
          if (old_selected != self->selected)
            g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_SELECTED]);
          /* the item was deleted above, so this is guaranteed to be new, even if the position didn't change */
          g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_SELECTED_ITEM]);
        }
    }

  g_list_model_items_changed (G_LIST_MODEL (self), position, removed, added);
  if (removed != added)
    g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_N_ITEMS]);

  g_object_thaw_notify (G_OBJECT (self));
}

static void
bobgui_single_selection_sections_changed_cb (BobguiSectionModel *model,
                                          unsigned int     position,
                                          unsigned int     n_items,
                                          gpointer         user_data)
{
  BobguiSingleSelection *self = BOBGUI_SINGLE_SELECTION (user_data);

  bobgui_section_model_sections_changed (BOBGUI_SECTION_MODEL (self), position, n_items);
}

static void
bobgui_single_selection_clear_model (BobguiSingleSelection *self)
{
  if (self->model == NULL)
    return;

  g_signal_handlers_disconnect_by_func (self->model,
                                        bobgui_single_selection_items_changed_cb,
                                        self);
  g_signal_handlers_disconnect_by_func (self->model,
                                        bobgui_single_selection_sections_changed_cb,
                                        self);
  g_clear_object (&self->model);
}

static void
bobgui_single_selection_set_property (GObject      *object,
                                   guint         prop_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)

{
  BobguiSingleSelection *self = BOBGUI_SINGLE_SELECTION (object);

  switch (prop_id)
    {
    case PROP_AUTOSELECT:
      bobgui_single_selection_set_autoselect (self, g_value_get_boolean (value));
      break;

    case PROP_CAN_UNSELECT:
      bobgui_single_selection_set_can_unselect (self, g_value_get_boolean (value));
      break;

    case PROP_MODEL:
      bobgui_single_selection_set_model (self, g_value_get_object (value));
      break;

    case PROP_SELECTED:
      bobgui_single_selection_set_selected (self, g_value_get_uint (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_single_selection_get_property (GObject    *object,
                                   guint       prop_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  BobguiSingleSelection *self = BOBGUI_SINGLE_SELECTION (object);

  switch (prop_id)
    {
    case PROP_AUTOSELECT:
      g_value_set_boolean (value, self->autoselect);
      break;

    case PROP_CAN_UNSELECT:
      g_value_set_boolean (value, self->can_unselect);
      break;

    case PROP_ITEM_TYPE:
      g_value_set_gtype (value, bobgui_single_selection_get_item_type (G_LIST_MODEL (self)));
      break;

    case PROP_MODEL:
      g_value_set_object (value, self->model);
      break;

    case PROP_N_ITEMS:
      g_value_set_uint (value, bobgui_single_selection_get_n_items (G_LIST_MODEL (self)));
      break;

    case PROP_SELECTED:
      g_value_set_uint (value, self->selected);
      break;

    case PROP_SELECTED_ITEM:
      g_value_set_object (value, self->selected_item);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_single_selection_dispose (GObject *object)
{
  BobguiSingleSelection *self = BOBGUI_SINGLE_SELECTION (object);

  bobgui_single_selection_clear_model (self);

  self->selected = BOBGUI_INVALID_LIST_POSITION;
  g_clear_object (&self->selected_item);

  G_OBJECT_CLASS (bobgui_single_selection_parent_class)->dispose (object);
}

static void
bobgui_single_selection_class_init (BobguiSingleSelectionClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->get_property = bobgui_single_selection_get_property;
  gobject_class->set_property = bobgui_single_selection_set_property;
  gobject_class->dispose = bobgui_single_selection_dispose;

  /**
   * BobguiSingleSelection:autoselect:
   *
   * If the selection will always select an item.
   */
  properties[PROP_AUTOSELECT] =
    g_param_spec_boolean ("autoselect", NULL, NULL,
                          TRUE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * BobguiSingleSelection:can-unselect:
   *
   * If unselecting the selected item is allowed.
   */
  properties[PROP_CAN_UNSELECT] =
    g_param_spec_boolean ("can-unselect", NULL, NULL,
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * BobguiSingleSelection:item-type:
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
   * BobguiSingleSelection:model:
   *
   * The model being managed.
   */
  properties[PROP_MODEL] =
    g_param_spec_object ("model", NULL, NULL,
                         G_TYPE_LIST_MODEL,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiSingleSelection:n-items:
   *
   * The number of items. See [method@Gio.ListModel.get_n_items].
   *
   * Since: 4.8
   **/
  properties[PROP_N_ITEMS] =
    g_param_spec_uint ("n-items", NULL, NULL,
                       0, G_MAXUINT, 0,
                       G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  /**
   * BobguiSingleSelection:selected:
   *
   * Position of the selected item.
   */
  properties[PROP_SELECTED] =
    g_param_spec_uint ("selected", NULL, NULL,
                       0, G_MAXUINT, BOBGUI_INVALID_LIST_POSITION,
                       G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * BobguiSingleSelection:selected-item:
   *
   * The selected item.
   */
  properties[PROP_SELECTED_ITEM] =
    g_param_spec_object ("selected-item", NULL, NULL,
                         G_TYPE_OBJECT,
                         G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (gobject_class, N_PROPS, properties);
}

static void
bobgui_single_selection_init (BobguiSingleSelection *self)
{
  self->selected = BOBGUI_INVALID_LIST_POSITION;
  self->autoselect = TRUE;
}

/**
 * bobgui_single_selection_new:
 * @model: (nullable) (transfer full): the `GListModel` to manage
 *
 * Creates a new selection to handle @model.
 *
 * Returns: (transfer full) (type BobguiSingleSelection): a new `BobguiSingleSelection`
 */
BobguiSingleSelection *
bobgui_single_selection_new (GListModel *model)
{
  BobguiSingleSelection *self;

  g_return_val_if_fail (model == NULL || G_IS_LIST_MODEL (model), NULL);

  self = g_object_new (BOBGUI_TYPE_SINGLE_SELECTION,
                       "model", model,
                       NULL);

  /* consume the reference */
  g_clear_object (&model);

  return self;
}

/**
 * bobgui_single_selection_get_model:
 * @self: a `BobguiSingleSelection`
 *
 * Gets the model that @self is wrapping.
 *
 * Returns: (transfer none) (nullable): The model being wrapped
 */
GListModel *
bobgui_single_selection_get_model (BobguiSingleSelection *self)
{
  g_return_val_if_fail (BOBGUI_IS_SINGLE_SELECTION (self), NULL);

  return self->model;
}

/**
 * bobgui_single_selection_set_model:
 * @self: a `BobguiSingleSelection`
 * @model: (nullable): A `GListModel` to wrap
 *
 * Sets the model that @self should wrap.
 *
 * If @model is %NULL, @self will be empty.
 */
void
bobgui_single_selection_set_model (BobguiSingleSelection *self,
                                GListModel         *model)
{
  guint n_items_before;

  g_return_if_fail (BOBGUI_IS_SINGLE_SELECTION (self));
  g_return_if_fail (model == NULL || G_IS_LIST_MODEL (model));

  if (self->model == model)
    return;

  g_object_freeze_notify (G_OBJECT (self));

  n_items_before = self->model ? g_list_model_get_n_items (self->model) : 0;
  bobgui_single_selection_clear_model (self);

  if (model)
    {
      self->model = g_object_ref (model);
      g_signal_connect (self->model, "items-changed",
                        G_CALLBACK (bobgui_single_selection_items_changed_cb), self);
      if (BOBGUI_IS_SECTION_MODEL (self->model))
        g_signal_connect (self->model, "sections-changed",
                          G_CALLBACK (bobgui_single_selection_sections_changed_cb), self);
      bobgui_single_selection_items_changed_cb (self->model,
                                             0,
                                             n_items_before,
                                             g_list_model_get_n_items (model),
                                             self);
    }
  else
    {
      if (self->selected != BOBGUI_INVALID_LIST_POSITION)
        {
          self->selected = BOBGUI_INVALID_LIST_POSITION;
          g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_SELECTED]);
        }
      if (self->selected_item)
        {
          g_clear_object (&self->selected_item);
          g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_SELECTED_ITEM]);
        }
      g_list_model_items_changed (G_LIST_MODEL (self), 0, n_items_before, 0);
      if (n_items_before)
        g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_N_ITEMS]);
    }

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_MODEL]);

  g_object_thaw_notify (G_OBJECT (self));
}

/**
 * bobgui_single_selection_get_selected:
 * @self: a `BobguiSingleSelection`
 *
 * Gets the position of the selected item.
 *
 * If no item is selected, %BOBGUI_INVALID_LIST_POSITION is returned.
 *
 * Returns: The position of the selected item
 */
guint
bobgui_single_selection_get_selected (BobguiSingleSelection *self)
{
  g_return_val_if_fail (BOBGUI_IS_SINGLE_SELECTION (self), BOBGUI_INVALID_LIST_POSITION);

  return self->selected;
}

/**
 * bobgui_single_selection_set_selected:
 * @self: a `BobguiSingleSelection`
 * @position: the item to select or %BOBGUI_INVALID_LIST_POSITION
 *
 * Selects the item at the given position.
 *
 * If the list does not have an item at @position or
 * %BOBGUI_INVALID_LIST_POSITION is given, the behavior depends on the
 * value of the [property@Bobgui.SingleSelection:autoselect] property:
 * If it is set, no change will occur and the old item will stay
 * selected. If it is unset, the selection will be unset and no item
 * will be selected. This also applies if [property@Bobgui.SingleSelection:can-unselect]
 * is set to %FALSE.
 */
void
bobgui_single_selection_set_selected (BobguiSingleSelection *self,
                                   guint               position)
{
  gpointer new_selected = NULL;
  guint old_position;

  g_return_if_fail (BOBGUI_IS_SINGLE_SELECTION (self));

  if (self->selected == position)
    return;

  if (self->model)
    new_selected = g_list_model_get_item (self->model, position);

  if (new_selected == NULL)
    {
      if (!self->can_unselect || self->autoselect)
        return;

      position = BOBGUI_INVALID_LIST_POSITION;
    }

  if (self->selected == position)
    return;

  old_position = self->selected;
  self->selected = position;
  g_clear_object (&self->selected_item);
  self->selected_item = new_selected;

  if (old_position == BOBGUI_INVALID_LIST_POSITION)
    bobgui_selection_model_selection_changed (BOBGUI_SELECTION_MODEL (self), position, 1);
  else if (position == BOBGUI_INVALID_LIST_POSITION)
    bobgui_selection_model_selection_changed (BOBGUI_SELECTION_MODEL (self), old_position, 1);
  else if (position < old_position)
    bobgui_selection_model_selection_changed (BOBGUI_SELECTION_MODEL (self), position, old_position - position + 1);
  else
    bobgui_selection_model_selection_changed (BOBGUI_SELECTION_MODEL (self), old_position, position - old_position + 1);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_SELECTED]);
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_SELECTED_ITEM]);
}

/**
 * bobgui_single_selection_get_selected_item:
 * @self: a `BobguiSingleSelection`
 *
 * Gets the selected item.
 *
 * If no item is selected, %NULL is returned.
 *
 * Returns: (transfer none) (type GObject) (nullable): The selected item
 */
gpointer
bobgui_single_selection_get_selected_item (BobguiSingleSelection *self)
{
  g_return_val_if_fail (BOBGUI_IS_SINGLE_SELECTION (self), NULL);

  return self->selected_item;
}

/**
 * bobgui_single_selection_get_autoselect:
 * @self: a `BobguiSingleSelection`
 *
 * Checks if autoselect has been enabled or disabled via
 * bobgui_single_selection_set_autoselect().
 *
 * Returns: %TRUE if autoselect is enabled
 **/
gboolean
bobgui_single_selection_get_autoselect (BobguiSingleSelection *self)
{
  g_return_val_if_fail (BOBGUI_IS_SINGLE_SELECTION (self), TRUE);

  return self->autoselect;
}

/**
 * bobgui_single_selection_set_autoselect:
 * @self: a `BobguiSingleSelection`
 * @autoselect: %TRUE to always select an item
 *
 * Enables or disables autoselect.
 *
 * If @autoselect is %TRUE, @self will enforce that an item is always
 * selected. It will select a new item when the currently selected
 * item is deleted and it will disallow unselecting the current item.
 */
void
bobgui_single_selection_set_autoselect (BobguiSingleSelection *self,
                                     gboolean            autoselect)
{
  g_return_if_fail (BOBGUI_IS_SINGLE_SELECTION (self));

  if (self->autoselect == autoselect)
    return;

  self->autoselect = autoselect;

  g_object_freeze_notify (G_OBJECT (self));
  
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_AUTOSELECT]);

  if (self->autoselect && !self->selected_item)
    bobgui_single_selection_set_selected (self, 0);

  g_object_thaw_notify (G_OBJECT (self));
}

/**
 * bobgui_single_selection_get_can_unselect:
 * @self: a `BobguiSingleSelection`
 *
 * If %TRUE, bobgui_selection_model_unselect_item() is supported and allows
 * unselecting the selected item.
 *
 * Returns: %TRUE to support unselecting
 */
gboolean
bobgui_single_selection_get_can_unselect (BobguiSingleSelection *self)
{
  g_return_val_if_fail (BOBGUI_IS_SINGLE_SELECTION (self), FALSE);

  return self->can_unselect;
}

/**
 * bobgui_single_selection_set_can_unselect:
 * @self: a `BobguiSingleSelection`
 * @can_unselect: %TRUE to allow unselecting
 *
 * If %TRUE, unselecting the current item via
 * bobgui_selection_model_unselect_item() is supported.
 *
 * Note that setting [property@Bobgui.SingleSelection:autoselect] will
 * cause unselecting to not work, so it practically makes no sense
 * to set both at the same time.
 */
void
bobgui_single_selection_set_can_unselect (BobguiSingleSelection *self,
                                       gboolean            can_unselect)
{
  g_return_if_fail (BOBGUI_IS_SINGLE_SELECTION (self));

  if (self->can_unselect == can_unselect)
    return;

  self->can_unselect = can_unselect;

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_CAN_UNSELECT]);
}
