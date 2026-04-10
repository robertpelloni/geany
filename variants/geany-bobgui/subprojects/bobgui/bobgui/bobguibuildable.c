/* bobguibuildable.c
 * Copyright (C) 2006-2007 Async Open Source,
 *                         Johan Dahlin <jdahlin@async.com.br>
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
 * BobguiBuildable:
 *
 * Allows objects to extend and customize deserialization from ui files.
 *
 * The `BobguiBuildable` interface includes methods for setting names and
 * properties of objects, parsing custom tags and constructing child objects.
 *
 * It is implemented by all widgets and many of the non-widget objects that are
 * provided by BOBGUI. The main user of this interface is [class@Bobgui.Builder].
 * There should be very little need for applications to call any of these
 * functions directly.
 *
 * An object only needs to implement this interface if it needs to extend the
 * `BobguiBuilder` XML format or run any extra routines at deserialization time.
 */

#include "config.h"
#include "bobguibuildableprivate.h"


typedef BobguiBuildableIface BobguiBuildableInterface;
G_DEFINE_INTERFACE (BobguiBuildable, bobgui_buildable, G_TYPE_OBJECT)

static void
bobgui_buildable_default_init (BobguiBuildableInterface *iface)
{
}

/*< private >
 * bobgui_buildable_set_buildable_id:
 * @buildable: a `BobguiBuildable`
 * @id: name to set
 *
 * Sets the ID of the @buildable object.
 */
void
bobgui_buildable_set_buildable_id (BobguiBuildable *buildable,
                                const char   *id)
{
  BobguiBuildableIface *iface;

  g_return_if_fail (BOBGUI_IS_BUILDABLE (buildable));
  g_return_if_fail (id != NULL);

  iface = BOBGUI_BUILDABLE_GET_IFACE (buildable);

  if (iface->set_id)
    (* iface->set_id) (buildable, id);
  else
    g_object_set_data_full (G_OBJECT (buildable),
			    "bobgui-builder-id",
			    g_strdup (id),
			    g_free);
}

/**
 * bobgui_buildable_get_buildable_id:
 * @buildable: a `BobguiBuildable`
 *
 * Gets the ID of the @buildable object.
 *
 * `BobguiBuilder` sets the name based on the ID attribute
 * of the `<object>` tag used to construct the @buildable.
 *
 * Returns: (nullable): the ID of the buildable object
 **/
const char *
bobgui_buildable_get_buildable_id (BobguiBuildable *buildable)
{
  BobguiBuildableIface *iface;

  g_return_val_if_fail (BOBGUI_IS_BUILDABLE (buildable), NULL);

  iface = BOBGUI_BUILDABLE_GET_IFACE (buildable);

  if (iface->get_id)
    return (* iface->get_id) (buildable);
  else
    return (const char *)g_object_get_data (G_OBJECT (buildable),
					    "bobgui-builder-id");
}

/*< private >
 * bobgui_buildable_add_child:
 * @buildable: a `BobguiBuildable`
 * @builder: a `BobguiBuilder`
 * @child: child to add
 * @type: (nullable): kind of child
 *
 * Adds a child to @buildable. @type is an optional string
 * describing how the child should be added.
 */
void
bobgui_buildable_add_child (BobguiBuildable *buildable,
			 BobguiBuilder   *builder,
			 GObject      *child,
			 const char   *type)
{
  BobguiBuildableIface *iface;

  g_return_if_fail (BOBGUI_IS_BUILDABLE (buildable));
  g_return_if_fail (BOBGUI_IS_BUILDER (builder));

  iface = BOBGUI_BUILDABLE_GET_IFACE (buildable);
  g_return_if_fail (iface->add_child != NULL);

  (* iface->add_child) (buildable, builder, child, type);
}

/*< private >
 * bobgui_buildable_parser_finished:
 * @buildable: a `BobguiBuildable`
 * @builder: a `BobguiBuilder`
 *
 * Called when the builder finishes the parsing of a
 * BobguiBuilder UI definition.
 *
 * Note that this will be called once for each time
 * bobgui_builder_add_from_file() or bobgui_builder_add_from_string()
 * is called on a builder.
 */
void
bobgui_buildable_parser_finished (BobguiBuildable *buildable,
			       BobguiBuilder   *builder)
{
  BobguiBuildableIface *iface;

  g_return_if_fail (BOBGUI_IS_BUILDABLE (buildable));
  g_return_if_fail (BOBGUI_IS_BUILDER (builder));

  iface = BOBGUI_BUILDABLE_GET_IFACE (buildable);
  if (iface->parser_finished)
    (* iface->parser_finished) (buildable, builder);
}

/*< private >
 * bobgui_buildable_construct_child:
 * @buildable: A `BobguiBuildable`
 * @builder: `BobguiBuilder` used to construct this object
 * @name: name of child to construct
 *
 * Constructs a child of @buildable with the name @name.
 *
 * `BobguiBuilder` calls this function if a “constructor” has been
 * specified in the UI definition.
 *
 * Returns: (transfer full): the constructed child
 */
GObject *
bobgui_buildable_construct_child (BobguiBuildable *buildable,
                               BobguiBuilder   *builder,
                               const char   *name)
{
  BobguiBuildableIface *iface;

  g_return_val_if_fail (BOBGUI_IS_BUILDABLE (buildable), NULL);
  g_return_val_if_fail (BOBGUI_IS_BUILDER (builder), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  iface = BOBGUI_BUILDABLE_GET_IFACE (buildable);
  g_return_val_if_fail (iface->construct_child != NULL, NULL);

  return (* iface->construct_child) (buildable, builder, name);
}

/*< private >
 * bobgui_buildable_custom_tag_start:
 * @buildable: a `BobguiBuildable`
 * @builder: a `BobguiBuilder` used to construct this object
 * @child: (nullable): child object or %NULL for non-child tags
 * @tagname: name of tag
 * @parser: (out): a `GMarkupParser` to fill in
 * @data: (out): return location for user data that will be passed in
 *   to parser functions
 *
 * This is called for each unknown element under `<child>`.
 *
 * Returns: %TRUE if an object has a custom implementation, %FALSE
 *   if it doesn't.
 */
gboolean
bobgui_buildable_custom_tag_start (BobguiBuildable       *buildable,
                                BobguiBuilder         *builder,
                                GObject            *child,
                                const char         *tagname,
                                BobguiBuildableParser *parser,
                                gpointer           *data)
{
  BobguiBuildableIface *iface;

  g_return_val_if_fail (BOBGUI_IS_BUILDABLE (buildable), FALSE);
  g_return_val_if_fail (BOBGUI_IS_BUILDER (builder), FALSE);
  g_return_val_if_fail (tagname != NULL, FALSE);

  iface = BOBGUI_BUILDABLE_GET_IFACE (buildable);
  g_return_val_if_fail (iface->custom_tag_start != NULL, FALSE);

  return (* iface->custom_tag_start) (buildable, builder, child,
                                      tagname, parser, data);
}

/*< private >
 * bobgui_buildable_custom_tag_end:
 * @buildable: A `BobguiBuildable`
 * @builder: `BobguiBuilder` used to construct this object
 * @child: (nullable): child object or %NULL for non-child tags
 * @tagname: name of tag
 * @data: user data that will be passed in to parser functions
 *
 * This is called at the end of each custom element handled by
 * the buildable.
 */
void
bobgui_buildable_custom_tag_end (BobguiBuildable  *buildable,
                              BobguiBuilder    *builder,
                              GObject       *child,
                              const char    *tagname,
                              gpointer       data)
{
  BobguiBuildableIface *iface;

  g_return_if_fail (BOBGUI_IS_BUILDABLE (buildable));
  g_return_if_fail (BOBGUI_IS_BUILDER (builder));
  g_return_if_fail (tagname != NULL);

  iface = BOBGUI_BUILDABLE_GET_IFACE (buildable);
  if (iface->custom_tag_end)
    (* iface->custom_tag_end) (buildable, builder, child, tagname, data);
}

/*< private >
 * bobgui_buildable_custom_finished:
 * @buildable: a `BobguiBuildable`
 * @builder: a `BobguiBuilder`
 * @child: (nullable): child object or %NULL for non-child tags
 * @tagname: the name of the tag
 * @data: user data created in custom_tag_start
 *
 * This is similar to bobgui_buildable_parser_finished() but is
 * called once for each custom tag handled by the @buildable.
 */
void
bobgui_buildable_custom_finished (BobguiBuildable  *buildable,
			       BobguiBuilder    *builder,
			       GObject       *child,
			       const char    *tagname,
			       gpointer       data)
{
  BobguiBuildableIface *iface;

  g_return_if_fail (BOBGUI_IS_BUILDABLE (buildable));
  g_return_if_fail (BOBGUI_IS_BUILDER (builder));

  iface = BOBGUI_BUILDABLE_GET_IFACE (buildable);
  if (iface->custom_finished)
    (* iface->custom_finished) (buildable, builder, child, tagname, data);
}

/*< private >
 * bobgui_buildable_get_internal_child:
 * @buildable: a `BobguiBuildable`
 * @builder: a `BobguiBuilder`
 * @childname: name of child
 *
 * Get the internal child called @childname of the @buildable object.
 *
 * Returns: (transfer none): the internal child of the buildable object
 */
GObject *
bobgui_buildable_get_internal_child (BobguiBuildable *buildable,
                                  BobguiBuilder   *builder,
                                  const char   *childname)
{
  BobguiBuildableIface *iface;

  g_return_val_if_fail (BOBGUI_IS_BUILDABLE (buildable), NULL);
  g_return_val_if_fail (BOBGUI_IS_BUILDER (builder), NULL);
  g_return_val_if_fail (childname != NULL, NULL);

  iface = BOBGUI_BUILDABLE_GET_IFACE (buildable);
  if (!iface->get_internal_child)
    return NULL;

  return (* iface->get_internal_child) (buildable, builder, childname);
}
