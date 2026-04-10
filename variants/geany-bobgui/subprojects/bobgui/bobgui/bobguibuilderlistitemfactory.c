/*
 * Copyright © 2019 Benjamin Otte
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

#include "bobguibuilderlistitemfactory.h"

#include "bobguibuilder.h"
#include "bobguibuilderprivate.h"
#include "bobguilistitemfactoryprivate.h"
#include "bobguilistitemprivate.h"

/**
 * BobguiBuilderListItemFactory:
 *
 * Creates widgets by instantiating `BobguiBuilder` UI templates.
 *
 * The templates must extend the class that the parent widget expects.
 * For example, a factory provided to [property@Bobgui.ListView:factory] must have
 * a template that extends [class@Bobgui.ListItem].
 *
 * Templates typically use [class@Bobgui.Expression] to obtain data from the items
 * in the model.
 *
 * Example:
 * ```xml
 *   <interface>
 *     <template class="BobguiListItem">
 *       <property name="child">
 *         <object class="BobguiLabel">
 *           <property name="xalign">0</property>
 *           <binding name="label">
 *             <lookup name="name" type="SettingsKey">
 *               <lookup name="item">BobguiListItem</lookup>
 *             </lookup>
 *           </binding>
 *         </object>
 *       </property>
 *     </template>
 *   </interface>
 * ```
 *
 * A common approach is to embed such templates as CDATA marked sections into
 * a surrounding UI file. Note that if you use this approach, extracting
 * translatable strings with xgettext will not work for strings inside the
 * marked section.
 */

struct _BobguiBuilderListItemFactory
{
  BobguiListItemFactory parent_instance;

  BobguiBuilderScope *scope;
  GBytes *bytes;
  GBytes *data;
  char *resource;
};

struct _BobguiBuilderListItemFactoryClass
{
  BobguiListItemFactoryClass parent_class;
};

enum {
  PROP_0,
  PROP_BYTES,
  PROP_RESOURCE,
  PROP_SCOPE,

  N_PROPS
};

G_DEFINE_TYPE (BobguiBuilderListItemFactory, bobgui_builder_list_item_factory, BOBGUI_TYPE_LIST_ITEM_FACTORY)

static GParamSpec *properties[N_PROPS] = { NULL, };

static void
bobgui_builder_list_item_factory_setup (BobguiListItemFactory *factory,
                                     GObject            *item,
                                     gboolean            bind,
                                     GFunc               func,
                                     gpointer            data)
{
  BobguiBuilderListItemFactory *self = BOBGUI_BUILDER_LIST_ITEM_FACTORY (factory);
  BobguiBuilder *builder;
  GError *error = NULL;

  BOBGUI_LIST_ITEM_FACTORY_CLASS (bobgui_builder_list_item_factory_parent_class)->setup (factory, item, bind, func, data);

  builder = bobgui_builder_new ();

  bobgui_builder_set_current_object (builder, item);
  if (self->scope)
    bobgui_builder_set_scope (builder, self->scope);

  bobgui_builder_set_allow_template_parents (builder, TRUE);
  if (!bobgui_builder_extend_with_template (builder, G_OBJECT (item), G_OBJECT_TYPE (item),
                                         (const char *)g_bytes_get_data (self->data, NULL),
                                         g_bytes_get_size (self->data),
                                         &error))
    {
      g_critical ("Error building template for list item: %s", error->message);
      g_error_free (error);

      /* This should never happen, if the template XML cannot be built
       * then it is a critical programming error.
       */
      g_object_unref (builder);
      return;
    }

  g_object_unref (builder);
}

static void
bobgui_builder_list_item_factory_get_property (GObject    *object,
                                            guint       property_id,
                                            GValue     *value,
                                            GParamSpec *pspec)
{
  BobguiBuilderListItemFactory *self = BOBGUI_BUILDER_LIST_ITEM_FACTORY (object);

  switch (property_id)
    {
    case PROP_BYTES:
      g_value_set_boxed (value, self->bytes);
      break;

    case PROP_RESOURCE:
      g_value_set_string (value, self->resource);
      break;

    case PROP_SCOPE:
      g_value_set_object (value, self->scope);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gboolean
bobgui_builder_list_item_factory_set_bytes (BobguiBuilderListItemFactory *self,
                                         GBytes                    *bytes)
{
  if (bytes == NULL)
    return FALSE;

  if (self->bytes)
    {
      g_critical ("Data for BobguiBuilderListItemFactory has already been set.");
      return FALSE;
    }

  self->bytes = g_bytes_ref (bytes);

  if (!_bobgui_buildable_parser_is_precompiled (g_bytes_get_data (bytes, NULL), g_bytes_get_size (bytes)))
    {
      GError *error = NULL;
      GBytes *data;

      data = _bobgui_buildable_parser_precompile (g_bytes_get_data (bytes, NULL),
                                               g_bytes_get_size (bytes),
                                               &error);
      if (data == NULL)
        {
          g_warning ("Failed to precompile template for BobguiBuilderListItemFactory: %s", error->message);
          g_error_free (error);
          self->data = g_bytes_ref (bytes);
        }
      else
        {
          self->data = data;
        }
    }

  return TRUE;
}

static void
bobgui_builder_list_item_factory_set_property (GObject      *object,
                                            guint         property_id,
                                            const GValue *value,
                                            GParamSpec   *pspec)
{
  BobguiBuilderListItemFactory *self = BOBGUI_BUILDER_LIST_ITEM_FACTORY (object);

  switch (property_id)
    {
    case PROP_BYTES:
      bobgui_builder_list_item_factory_set_bytes (self, g_value_get_boxed (value));
      break;

    case PROP_RESOURCE:
      {
        GError *error = NULL;
        GBytes *bytes;
        const char *resource;

        resource = g_value_get_string (value);
        if (resource == NULL)
          break;

        bytes = g_resources_lookup_data (resource, 0, &error);
        if (bytes)
          {
            if (bobgui_builder_list_item_factory_set_bytes (self, bytes))
              self->resource = g_strdup (resource);
            g_bytes_unref (bytes);
          }
        else
          {
            g_critical ("Unable to load resource for list item template: %s", error->message);
            g_error_free (error);
          }
      }
      break;

    case PROP_SCOPE:
      self->scope = g_value_dup_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
bobgui_builder_list_item_factory_finalize (GObject *object)
{
  BobguiBuilderListItemFactory *self = BOBGUI_BUILDER_LIST_ITEM_FACTORY (object);

  g_clear_object (&self->scope);
  g_bytes_unref (self->bytes);
  g_bytes_unref (self->data);
  g_free (self->resource);

  G_OBJECT_CLASS (bobgui_builder_list_item_factory_parent_class)->finalize (object);
}

static void
bobgui_builder_list_item_factory_class_init (BobguiBuilderListItemFactoryClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  BobguiListItemFactoryClass *factory_class = BOBGUI_LIST_ITEM_FACTORY_CLASS (klass);

  gobject_class->finalize = bobgui_builder_list_item_factory_finalize;
  gobject_class->get_property = bobgui_builder_list_item_factory_get_property;
  gobject_class->set_property = bobgui_builder_list_item_factory_set_property;

  factory_class->setup = bobgui_builder_list_item_factory_setup;

  /**
   * BobguiBuilderListItemFactory:bytes:
   *
   * `GBytes` containing the UI definition.
   */
  properties[PROP_BYTES] =
    g_param_spec_boxed ("bytes", NULL, NULL,
                        G_TYPE_BYTES,
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  /**
   * BobguiBuilderListItemFactory:resource:
   *
   * Path of the resource containing the UI definition.
   */
  properties[PROP_RESOURCE] =
    g_param_spec_string ("resource", NULL, NULL,
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  /**
   * BobguiBuilderListItemFactory:scope:
   *
   * `BobguiBuilderScope` to use when instantiating listitems
   */
  properties[PROP_SCOPE] =
    g_param_spec_object ("scope", NULL, NULL,
                         BOBGUI_TYPE_BUILDER_SCOPE,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (gobject_class, N_PROPS, properties);
}

static void
bobgui_builder_list_item_factory_init (BobguiBuilderListItemFactory *self)
{
}

/**
 * bobgui_builder_list_item_factory_new_from_bytes:
 * @scope: (nullable) (transfer none): A scope to use when instantiating
 * @bytes: the `GBytes` containing the UI definition to instantiate
 *
 * Creates a new `BobguiBuilderListItemFactory` that instantiates widgets
 * using @bytes as the data to pass to `BobguiBuilder`.
 *
 * Returns: a new `BobguiBuilderListItemFactory`
 **/
BobguiListItemFactory *
bobgui_builder_list_item_factory_new_from_bytes (BobguiBuilderScope *scope,
                                              GBytes          *bytes)
{
  g_return_val_if_fail (bytes != NULL, NULL);

  return g_object_new (BOBGUI_TYPE_BUILDER_LIST_ITEM_FACTORY,
                       "bytes", bytes,
                       "scope", scope,
                       NULL);
}

/**
 * bobgui_builder_list_item_factory_new_from_resource:
 * @scope: (nullable) (transfer none): A scope to use when instantiating
 * @resource_path: valid path to a resource that contains the UI definition
 *
 * Creates a new `BobguiBuilderListItemFactory` that instantiates widgets
 * using data read from the given @resource_path to pass to `BobguiBuilder`.
 *
 * Returns: a new `BobguiBuilderListItemFactory`
 **/
BobguiListItemFactory *
bobgui_builder_list_item_factory_new_from_resource (BobguiBuilderScope *scope,
                                                 const char      *resource_path)
{
  g_return_val_if_fail (scope == NULL || BOBGUI_IS_BUILDER_SCOPE (scope), NULL);
  g_return_val_if_fail (resource_path != NULL, NULL);

  return g_object_new (BOBGUI_TYPE_BUILDER_LIST_ITEM_FACTORY,
                       "resource", resource_path,
                       "scope", scope,
                       NULL);
}

/**
 * bobgui_builder_list_item_factory_get_bytes:
 * @self: a `BobguiBuilderListItemFactory`
 *
 * Gets the data used as the `BobguiBuilder` UI template for constructing
 * listitems.
 *
 * Returns: (transfer none): The `BobguiBuilder` data
 */
GBytes *
bobgui_builder_list_item_factory_get_bytes (BobguiBuilderListItemFactory *self)
{
  g_return_val_if_fail (BOBGUI_IS_BUILDER_LIST_ITEM_FACTORY (self), NULL);

  return self->bytes;
}

/**
 * bobgui_builder_list_item_factory_get_resource:
 * @self: a `BobguiBuilderListItemFactory`
 *
 * If the data references a resource, gets the path of that resource.
 *
 * Returns: (transfer none) (nullable): The path to the resource
 */
const char *
bobgui_builder_list_item_factory_get_resource (BobguiBuilderListItemFactory *self)
{
  g_return_val_if_fail (BOBGUI_IS_BUILDER_LIST_ITEM_FACTORY (self), NULL);

  return self->resource;
}

/**
 * bobgui_builder_list_item_factory_get_scope:
 * @self: a `BobguiBuilderListItemFactory`
 *
 * Gets the scope used when constructing listitems.
 *
 * Returns: (transfer none) (nullable): The scope used when constructing listitems
 */
BobguiBuilderScope *
bobgui_builder_list_item_factory_get_scope (BobguiBuilderListItemFactory *self)
{
  g_return_val_if_fail (BOBGUI_IS_BUILDER_LIST_ITEM_FACTORY (self), NULL);

  return self->scope;
}
