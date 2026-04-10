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

#include "bobguiflattenlistmodel.h"

#include "bobguisectionmodel.h"
#include "bobguirbtreeprivate.h"

/**
 * BobguiFlattenListModel:
 *
 * A list model that concatenates other list models.
 *
 * `BobguiFlattenListModel` takes a list model containing list models, and flattens
 * it into a single model. Each list model becomes a section in the single model.
 */

enum {
  PROP_0,
  PROP_ITEM_TYPE,
  PROP_MODEL,
  PROP_N_ITEMS,

  NUM_PROPERTIES
};

typedef struct _FlattenNode FlattenNode;
typedef struct _FlattenAugment FlattenAugment;

struct _FlattenNode
{
  GListModel *model;
  BobguiFlattenListModel *list;
};

struct _FlattenAugment
{
  guint n_items;
  guint n_models;
};

struct _BobguiFlattenListModel
{
  GObject parent_instance;

  GListModel *model;
  BobguiRbTree *items; /* NULL if model == NULL */
};

struct _BobguiFlattenListModelClass
{
  GObjectClass parent_class;
};

static GParamSpec *properties[NUM_PROPERTIES] = { NULL, };

static FlattenNode *
bobgui_flatten_list_model_get_nth (BobguiRbTree *tree,
                                guint      position,
                                guint     *model_position)
{
  FlattenNode *node, *tmp;
  guint model_n_items;

  node = bobgui_rb_tree_get_root (tree);

  while (node)
    {
      tmp = bobgui_rb_tree_node_get_left (node);
      if (tmp)
        {
          FlattenAugment *aug = bobgui_rb_tree_get_augment (tree, tmp);
          if (position < aug->n_items)
            {
              node = tmp;
              continue;
            }
          position -= aug->n_items;
        }

      model_n_items = g_list_model_get_n_items (node->model);
      if (position < model_n_items)
        break;
      position -= model_n_items;

      node = bobgui_rb_tree_node_get_right (node);
    }

  if (model_position)
    *model_position = node ? position : 0;

  return node;
}

static FlattenNode *
bobgui_flatten_list_model_get_nth_model (BobguiRbTree *tree,
                                      guint      position,
                                      guint     *items_before)
{
  FlattenNode *node, *tmp;
  guint before;

  node = bobgui_rb_tree_get_root (tree);
  before = 0;

  while (node)
    {
      tmp = bobgui_rb_tree_node_get_left (node);
      if (tmp)
        {
          FlattenAugment *aug = bobgui_rb_tree_get_augment (tree, tmp);
          if (position < aug->n_models)
            {
              node = tmp;
              continue;
            }
          position -= aug->n_models;
          before += aug->n_items;
        }

      if (position == 0)
        break;
      position--;
      before += g_list_model_get_n_items (node->model);

      node = bobgui_rb_tree_node_get_right (node);
    }

  if (items_before)
    *items_before = before;

  return node;
}

static GType
bobgui_flatten_list_model_get_item_type (GListModel *list)
{
  return G_TYPE_OBJECT;
}

static guint
bobgui_flatten_list_model_get_n_items (GListModel *list)
{
  BobguiFlattenListModel *self = BOBGUI_FLATTEN_LIST_MODEL (list);
  FlattenAugment *aug;
  FlattenNode *node;

  if (!self->items)
    return 0;

  node = bobgui_rb_tree_get_root (self->items);
  if (node == NULL)
    return 0;

  aug = bobgui_rb_tree_get_augment (self->items, node);
  return aug->n_items;
}

static gpointer
bobgui_flatten_list_model_get_item (GListModel *list,
                                 guint       position)
{
  BobguiFlattenListModel *self = BOBGUI_FLATTEN_LIST_MODEL (list);
  FlattenNode *node;
  guint model_pos;

  if (!self->items)
    return NULL;

  node = bobgui_flatten_list_model_get_nth (self->items, position, &model_pos);
  if (node == NULL)
    return NULL;

  return g_list_model_get_item (node->model, model_pos);
}

static void
bobgui_flatten_list_model_model_init (GListModelInterface *iface)
{
  iface->get_item_type = bobgui_flatten_list_model_get_item_type;
  iface->get_n_items = bobgui_flatten_list_model_get_n_items;
  iface->get_item = bobgui_flatten_list_model_get_item;
}

static void
bobgui_flatten_list_model_get_section (BobguiSectionModel *model,
                                    guint            position,
                                    guint           *out_start,
                                    guint           *out_end)
{
  BobguiFlattenListModel *self = BOBGUI_FLATTEN_LIST_MODEL (model);
  FlattenNode *node;
  guint model_pos;

  node = bobgui_flatten_list_model_get_nth (self->items, position, &model_pos);
  if (node == NULL)
    {
      *out_start = bobgui_flatten_list_model_get_n_items (G_LIST_MODEL (self));
      *out_end = G_MAXUINT;
      return;
    }

  *out_start = position - model_pos;
  *out_end = position - model_pos + g_list_model_get_n_items (node->model);
}

static void
bobgui_flatten_list_model_section_model_init (BobguiSectionModelInterface *iface)
{
  iface->get_section = bobgui_flatten_list_model_get_section;
}

G_DEFINE_TYPE_WITH_CODE (BobguiFlattenListModel, bobgui_flatten_list_model, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (G_TYPE_LIST_MODEL,
                                                bobgui_flatten_list_model_model_init)
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_SECTION_MODEL,
                                                bobgui_flatten_list_model_section_model_init))

static void
bobgui_flatten_list_model_items_changed_cb (GListModel *model,
                                         guint       position,
                                         guint       removed,
                                         guint       added,
                                         gpointer    _node)
{
  FlattenNode *node = _node, *parent, *left;
  BobguiFlattenListModel *self = node->list;
  guint real_position;

  bobgui_rb_tree_node_mark_dirty (node);
  real_position = position;

  left = bobgui_rb_tree_node_get_left (node);
  if (left)
    {
      FlattenAugment *aug = bobgui_rb_tree_get_augment (self->items, left);
      real_position += aug->n_items;
    }

  for (;
       (parent = bobgui_rb_tree_node_get_parent (node)) != NULL;
       node = parent)
    {
      left = bobgui_rb_tree_node_get_left (parent);
      if (left != node)
        {
          if (left)
            {
              FlattenAugment *aug = bobgui_rb_tree_get_augment (self->items, left);
              real_position += aug->n_items;
            }
          real_position += g_list_model_get_n_items (parent->model);
        }
    }

  g_list_model_items_changed (G_LIST_MODEL (self), real_position, removed, added);
  if (removed != added)
    g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_N_ITEMS]);
}

static void
bobgui_flatten_list_model_clear_node (gpointer _node)
{
  FlattenNode *node= _node;

  g_signal_handlers_disconnect_by_func (node->model, bobgui_flatten_list_model_items_changed_cb, node);
  g_object_unref (node->model);
}

static void
bobgui_flatten_list_model_augment (BobguiRbTree *flatten,
                                gpointer   _aug,
                                gpointer   _node,
                                gpointer   left,
                                gpointer   right)
{
  FlattenNode *node = _node;
  FlattenAugment *aug = _aug;

  aug->n_items = g_list_model_get_n_items (node->model);
  aug->n_models = 1;

  if (left)
    {
      FlattenAugment *left_aug = bobgui_rb_tree_get_augment (flatten, left);
      aug->n_items += left_aug->n_items;
      aug->n_models += left_aug->n_models;
    }
  if (right)
    {
      FlattenAugment *right_aug = bobgui_rb_tree_get_augment (flatten, right);
      aug->n_items += right_aug->n_items;
      aug->n_models += right_aug->n_models;
    }
}

static guint
bobgui_flatten_list_model_add_items (BobguiFlattenListModel *self,
                                  FlattenNode         *after,
                                  guint                position,
                                  guint                n)
{
  FlattenNode *node;
  guint added, i;

  added = 0;
  for (i = 0; i < n; i++)
    {
      node = bobgui_rb_tree_insert_before (self->items, after);
      node->model = g_list_model_get_item (self->model, position + i);
      g_signal_connect (node->model,
                        "items-changed",
                        G_CALLBACK (bobgui_flatten_list_model_items_changed_cb),
                        node);
      node->list = self;
      added += g_list_model_get_n_items (node->model);
    }

  return added;
}

static void
bobgui_flatten_list_model_set_property (GObject      *object,
                                     guint         prop_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  BobguiFlattenListModel *self = BOBGUI_FLATTEN_LIST_MODEL (object);

  switch (prop_id)
    {
    case PROP_MODEL:
      bobgui_flatten_list_model_set_model (self, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void 
bobgui_flatten_list_model_get_property (GObject    *object,
                                     guint       prop_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
  BobguiFlattenListModel *self = BOBGUI_FLATTEN_LIST_MODEL (object);

  switch (prop_id)
    {
    case PROP_ITEM_TYPE:
      g_value_set_gtype (value, bobgui_flatten_list_model_get_item_type (G_LIST_MODEL (self)));
      break;

    case PROP_MODEL:
      g_value_set_object (value, self->model);
      break;

    case PROP_N_ITEMS:
      g_value_set_uint (value, bobgui_flatten_list_model_get_n_items (G_LIST_MODEL (self)));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_flatten_list_model_model_items_changed_cb (GListModel          *model,
                                               guint                position,
                                               guint                removed,
                                               guint                added,
                                               BobguiFlattenListModel *self)
{
  FlattenNode *node;
  guint i, real_position, real_removed, real_added;

  node = bobgui_flatten_list_model_get_nth_model (self->items, position, &real_position);

  real_removed = 0;
  for (i = 0; i < removed; i++)
    {
      FlattenNode *next = bobgui_rb_tree_node_get_next (node);
      real_removed += g_list_model_get_n_items (node->model);
      bobgui_rb_tree_remove (self->items, node);
      node = next;
    }

  real_added = bobgui_flatten_list_model_add_items (self, node, position, added);

  if (real_removed > 0 || real_added > 0)
    g_list_model_items_changed (G_LIST_MODEL (self), real_position, real_removed, real_added);
  if (real_removed != real_added)
    g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_N_ITEMS]);
}

static void
bobgui_flatten_list_clear_model (BobguiFlattenListModel *self)
{
  if (self->model)
    {
      g_signal_handlers_disconnect_by_func (self->model, bobgui_flatten_list_model_model_items_changed_cb, self);
      g_clear_object (&self->model);
      g_clear_pointer (&self->items, bobgui_rb_tree_unref);
    }
}

static void
bobgui_flatten_list_model_dispose (GObject *object)
{
  BobguiFlattenListModel *self = BOBGUI_FLATTEN_LIST_MODEL (object);

  bobgui_flatten_list_clear_model (self);

  G_OBJECT_CLASS (bobgui_flatten_list_model_parent_class)->dispose (object);
}

static void
bobgui_flatten_list_model_class_init (BobguiFlattenListModelClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);

  gobject_class->set_property = bobgui_flatten_list_model_set_property;
  gobject_class->get_property = bobgui_flatten_list_model_get_property;
  gobject_class->dispose = bobgui_flatten_list_model_dispose;

  /**
   * BobguiFlattenListModel:item-type:
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
   * BobguiFlattenListModel:model:
   *
   * The model being flattened.
   */
  properties[PROP_MODEL] =
      g_param_spec_object ("model", NULL, NULL,
                           G_TYPE_LIST_MODEL,
                           G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiFlattenListModel:n-items:
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
bobgui_flatten_list_model_init (BobguiFlattenListModel *self)
{
}

/**
 * bobgui_flatten_list_model_new:
 * @model: (nullable) (transfer full): the model to be flattened
 *
 * Creates a new `BobguiFlattenListModel` that flattens @list.
 *
 * Returns: a new `BobguiFlattenListModel`
 */
BobguiFlattenListModel *
bobgui_flatten_list_model_new (GListModel *model)
{
  BobguiFlattenListModel *result;

  g_return_val_if_fail (model == NULL || G_IS_LIST_MODEL (model), NULL);

  result = g_object_new (BOBGUI_TYPE_FLATTEN_LIST_MODEL,
                         "model", model,
                         NULL);

  /* we consume the reference */
  g_clear_object (&model);

  return result;
}

/**
 * bobgui_flatten_list_model_set_model:
 * @self: a `BobguiFlattenListModel`
 * @model: (nullable) (transfer none): the new model
 *
 * Sets a new model to be flattened.
 */
void
bobgui_flatten_list_model_set_model (BobguiFlattenListModel *self,
                                  GListModel          *model)
{
  guint removed, added = 0;

  g_return_if_fail (BOBGUI_IS_FLATTEN_LIST_MODEL (self));
  g_return_if_fail (model == NULL || G_IS_LIST_MODEL (model));

  if (self->model == model)
    return;

  removed = g_list_model_get_n_items (G_LIST_MODEL (self)); 
  bobgui_flatten_list_clear_model (self);

  self->model = model;

  if (model)
    {
      g_object_ref (model);
      g_signal_connect (model, "items-changed", G_CALLBACK (bobgui_flatten_list_model_model_items_changed_cb), self);
      self->items = bobgui_rb_tree_new (FlattenNode,
                                     FlattenAugment,
                                     bobgui_flatten_list_model_augment,
                                     bobgui_flatten_list_model_clear_node,
                                     NULL);

      added = bobgui_flatten_list_model_add_items (self, NULL, 0, g_list_model_get_n_items (model));
    }

  if (removed > 0 || added > 0)
    g_list_model_items_changed (G_LIST_MODEL (self), 0, removed, added);
  if (removed != added)
    g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_N_ITEMS]);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_MODEL]);
}

/**
 * bobgui_flatten_list_model_get_model:
 * @self: a `BobguiFlattenListModel`
 *
 * Gets the model set via bobgui_flatten_list_model_set_model().
 *
 * Returns: (nullable) (transfer none): The model flattened by @self
 **/
GListModel *
bobgui_flatten_list_model_get_model (BobguiFlattenListModel *self)
{
  g_return_val_if_fail (BOBGUI_IS_FLATTEN_LIST_MODEL (self), NULL);

  return self->model;
}

/**
 * bobgui_flatten_list_model_get_model_for_item:
 * @self: a `BobguiFlattenListModel`
 * @position: a position
 *
 * Returns the model containing the item at the given position.
 *
 * Returns: (transfer none) (nullable): the model containing the item at @position
 */
GListModel *
bobgui_flatten_list_model_get_model_for_item (BobguiFlattenListModel *self,
                                           guint                position)
{
  FlattenNode *node;

  if (!self->items)
    return NULL;

  node = bobgui_flatten_list_model_get_nth (self->items, position, NULL);
  if (node == NULL)
    return NULL;

  return node->model;
}
