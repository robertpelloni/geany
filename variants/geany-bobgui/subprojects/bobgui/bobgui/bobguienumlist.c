/*
 * Copyright (C) 2018-2020 Purism SPC
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "config.h"

#include "bobguienumlist.h"

#include <gio/gio.h>
#include <bobgui/bobgui.h>

/**
 * BobguiEnumList:
 *
 * A [iface@Gio.ListModel] representing values of a given enum.
 *
 * `BobguiEnumList` contains objects of type [class@Bobgui.EnumListItem].
 *
 * A simple way to use a `BobguiEnumList` is to populate a [class@Bobgui.DropDown]
 * widget using the short name (or "nick") of the values of an
 * enumeration type:
 *
 * ```c
 * choices = bobgui_drop_down_new (G_LIST_MODEL (bobgui_enum_list_new (type)),
 *                              bobgui_property_expression_new (BOBGUI_TYPE_ENUM_LIST_ITEM,
 *                                                           NULL,
 *                                                           "nick"));
 * ```
 *
 * Since: 4.24
 */

struct _BobguiEnumList
{
  GObject parent_instance;

  GType enum_type;
  GEnumClass *enum_class;

  BobguiEnumListItem **objects;
};

enum {
  PROP_0,
  PROP_ENUM_TYPE,
  PROP_ITEM_TYPE,
  PROP_N_ITEMS,
  LAST_PROP,
};

static GParamSpec *props[LAST_PROP];

static void bobgui_enum_list_iface_init (GListModelInterface *iface);

G_DEFINE_FINAL_TYPE_WITH_CODE (BobguiEnumList, bobgui_enum_list, G_TYPE_OBJECT,
                               G_IMPLEMENT_INTERFACE (G_TYPE_LIST_MODEL, bobgui_enum_list_iface_init))

/**
 * BobguiEnumListItem:
 *
 * `BobguiEnumListItem` is the type of items in a [class@Bobgui.EnumList].
 *
 * Since: 4.24
 */

struct _BobguiEnumListItem
{
  GObject parent_instance;

  GEnumValue enum_value;
};

enum {
  VALUE_PROP_0,
  VALUE_PROP_VALUE,
  VALUE_PROP_NAME,
  VALUE_PROP_NICK,
  LAST_VALUE_PROP,
};

static GParamSpec *value_props[LAST_VALUE_PROP];

G_DEFINE_FINAL_TYPE (BobguiEnumListItem, bobgui_enum_list_item, G_TYPE_OBJECT)

static void
bobgui_enum_list_item_get_property (GObject    *object,
                                 guint       prop_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  BobguiEnumListItem *self = BOBGUI_ENUM_LIST_ITEM (object);

  switch (prop_id)
    {
    case VALUE_PROP_VALUE:
      g_value_set_int (value, bobgui_enum_list_item_get_value (self));
      break;
    case VALUE_PROP_NAME:
      g_value_set_string (value, bobgui_enum_list_item_get_name (self));
      break;
    case VALUE_PROP_NICK:
      g_value_set_string (value, bobgui_enum_list_item_get_nick (self));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
bobgui_enum_list_item_class_init (BobguiEnumListItemClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = bobgui_enum_list_item_get_property;

  /**
   * BobguiEnumListItem:value:
   *
   * The enum value.
   *
   * Since: 4.24
   */
  value_props[VALUE_PROP_VALUE] =
    g_param_spec_int ("value", NULL, NULL,
                      G_MININT, G_MAXINT, 0,
                      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  /**
   * BobguiEnumListItem:name:
   *
   * The enum value name.
   *
   * Since: 4.24
   */
  value_props[VALUE_PROP_NAME] =
    g_param_spec_string ("name", NULL, NULL,
                         NULL,
                         G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  /**
   * BobguiEnumListItem:nick:
   *
   * The enum value nick.
   *
   * Since: 4.24
   */
  value_props[VALUE_PROP_NICK] =
    g_param_spec_string ("nick", NULL, NULL,
                         NULL,
                         G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, LAST_VALUE_PROP, value_props);
}

static void
bobgui_enum_list_item_init (BobguiEnumListItem *self)
{
}

static BobguiEnumListItem *
bobgui_enum_list_item_new (GEnumValue *enum_value)
{
  BobguiEnumListItem *self = g_object_new (BOBGUI_TYPE_ENUM_LIST_ITEM, NULL);

  self->enum_value = *enum_value;

  return self;
}

/**
 * bobgui_enum_list_item_get_value:
 *
 * Gets the enum value.
 *
 * Returns: the enum value
 */
int
bobgui_enum_list_item_get_value (BobguiEnumListItem *self)
{
  g_return_val_if_fail (BOBGUI_IS_ENUM_LIST_ITEM (self), 0);

  return self->enum_value.value;
}

/**
 * bobgui_enum_list_item_get_name:
 *
 * Gets the enum value name.
 *
 * Returns: the enum value name
 */
const char *
bobgui_enum_list_item_get_name (BobguiEnumListItem *self)
{
  g_return_val_if_fail (BOBGUI_IS_ENUM_LIST_ITEM (self), NULL);

  return self->enum_value.value_name;
}

/**
 * bobgui_enum_list_item_get_nick:
 *
 * Gets the enum value nick.
 *
 * Returns: the enum value nick
 */
const char *
bobgui_enum_list_item_get_nick (BobguiEnumListItem *self)
{
  g_return_val_if_fail (BOBGUI_IS_ENUM_LIST_ITEM (self), NULL);

  return self->enum_value.value_nick;
}

static void
bobgui_enum_list_constructed (GObject *object)
{
  BobguiEnumList *self = BOBGUI_ENUM_LIST (object);
  guint i;

  self->enum_class = g_type_class_get (self->enum_type);

  self->objects = g_new (BobguiEnumListItem *, self->enum_class->n_values);

  for (i = 0; i < self->enum_class->n_values; i++)
    self->objects[i] = bobgui_enum_list_item_new (&self->enum_class->values[i]);

  G_OBJECT_CLASS (bobgui_enum_list_parent_class)->constructed (object);
}

static void
bobgui_enum_list_finalize (GObject *object)
{
  BobguiEnumList *self = BOBGUI_ENUM_LIST (object);

  for (unsigned int i = 0; i < self->enum_class->n_values; i++)
    g_object_unref (self->objects[i]);

  g_clear_pointer (&self->objects, g_free);

  G_OBJECT_CLASS (bobgui_enum_list_parent_class)->finalize (object);
}

static void
bobgui_enum_list_get_property (GObject      *object,
                            unsigned int  prop_id,
                            GValue       *value,
                            GParamSpec   *pspec)
{
  BobguiEnumList *self = BOBGUI_ENUM_LIST (object);

  switch (prop_id)
    {
    case PROP_ENUM_TYPE:
      g_value_set_gtype (value, self->enum_type);
      break;
    case PROP_ITEM_TYPE:
      g_value_set_gtype (value, BOBGUI_TYPE_ENUM_LIST_ITEM);
      break;
    case PROP_N_ITEMS:
      g_value_set_uint (value, self->enum_class->n_values);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
bobgui_enum_list_set_property (GObject      *object,
                            unsigned int  prop_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  BobguiEnumList *self = BOBGUI_ENUM_LIST (object);

  switch (prop_id)
    {
    case PROP_ENUM_TYPE:
      self->enum_type = g_value_get_gtype (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
bobgui_enum_list_class_init (BobguiEnumListClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = bobgui_enum_list_constructed;
  object_class->finalize = bobgui_enum_list_finalize;
  object_class->get_property = bobgui_enum_list_get_property;
  object_class->set_property = bobgui_enum_list_set_property;

  /**
   * BobguiEnumList:enum-type:
   *
   * The type of the enum represented by the model.
   *
   * Since: 4.24
   */
  props[PROP_ENUM_TYPE] =
    g_param_spec_gtype ("enum-type", NULL, NULL,
                        G_TYPE_ENUM,
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  /**
   * BobguiEnumList:item-type:
   *
   * The type of the items. See [method@Gio.ListModel.get_item_type].
   *
   * Since: 4.24
   */
  props[PROP_ITEM_TYPE] =
    g_param_spec_gtype ("item-type", NULL, NULL,
                        BOBGUI_TYPE_ENUM_LIST_ITEM,
                        G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  /**
   * BobguiEnumList:n-items:
   *
   * The number of items. See [method@Gio.ListModel.get_n_items].
   *
   * Since: 4.24
   */
  props[PROP_N_ITEMS] =
    g_param_spec_uint ("n-items", NULL, NULL,
                       0, G_MAXUINT, 0,
                       G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, LAST_PROP, props);
}

static void
bobgui_enum_list_init (BobguiEnumList *self)
{
}

static GType
bobgui_enum_list_get_item_type (GListModel *list)
{
  return BOBGUI_TYPE_ENUM_LIST_ITEM;
}

static guint
bobgui_enum_list_get_n_items (GListModel *list)
{
  BobguiEnumList *self = BOBGUI_ENUM_LIST (list);

  return self->enum_class->n_values;
}

static gpointer
bobgui_enum_list_get_item (GListModel *list,
                        guint       position)
{
  BobguiEnumList *self = BOBGUI_ENUM_LIST (list);

  if (position < self->enum_class->n_values)
    return g_object_ref (self->objects[position]);

  return NULL;
}

static void
bobgui_enum_list_iface_init (GListModelInterface *iface)
{
  iface->get_item_type = bobgui_enum_list_get_item_type;
  iface->get_n_items = bobgui_enum_list_get_n_items;
  iface->get_item = bobgui_enum_list_get_item;
}

/**
 * bobgui_enum_list_new:
 * @enum_type: the type of the enum to construct the model from
 *
 * Creates a new `BobguiEnumList` for @enum_type.
 *
 * Returns: the newly created `BobguiEnumList`
 *
 * Since: 4.24
 */
BobguiEnumList *
bobgui_enum_list_new (GType enum_type)
{
  g_return_val_if_fail (G_TYPE_IS_ENUM (enum_type), NULL);

  return g_object_new (BOBGUI_TYPE_ENUM_LIST,
                       "enum-type", enum_type,
                       NULL);
}

/**
 * bobgui_enum_list_get_enum_type:
 *
 * Gets the type of the enum represented by @self.
 *
 * Returns: the enum type
 *
 * Since: 4.24
 */
GType
bobgui_enum_list_get_enum_type (BobguiEnumList *self)
{
  g_return_val_if_fail (BOBGUI_IS_ENUM_LIST (self), G_TYPE_INVALID);

  return self->enum_type;
}

/**
 * bobgui_enum_list_find:
 * @value: an enum value
 *
 * Finds the position of a given enum value in @self.
 *
 * If the value is not found, [const@Bobgui.INVALID_LIST_POSITION] is returned.
 *
 * Returns: the position of the value
 *
 * Since: 4.24
 */
unsigned int
bobgui_enum_list_find (BobguiEnumList *self,
                    int          value)
{
  g_return_val_if_fail (BOBGUI_IS_ENUM_LIST (self), BOBGUI_INVALID_LIST_POSITION);

  for (unsigned int i = 0; i < self->enum_class->n_values; i++)
    {
      if (self->enum_class->values[i].value == value)
        return i;
    }

  return BOBGUI_INVALID_LIST_POSITION;
}
