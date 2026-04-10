/* BOBGUI - The Bobgui Framework
 * bobguisizegroup.c:
 * Copyright (C) 2001 Red Hat Software
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"
#include <string.h>

#include "bobguibuildable.h"
#include "bobguibuilderprivate.h"
#include "bobguiwidget.h"
#include "bobguitypebuiltins.h"
#include "bobguiprivate.h"
#include "bobguisizegroup-private.h"
#include "bobguiwidgetprivate.h"


/**
 * BobguiSizeGroup
 *
 * Groups widgets together so they all request the same size.
 *
 * This is typically useful when you want a column of widgets to have
 * the same size, but you can’t use a [class@Bobgui.Grid] or [class@Bobgui.Box].
 *
 * In detail, the size requested for each widget in a `BobguiSizeGroup` is
 * the maximum of the sizes that would have been requested for each
 * widget in the size group if they were not in the size group. The
 * [mode][method@Bobgui.SizeGroup.set_mode] of the size group determines
 * whether this applies to the horizontal size, the vertical size, or
 * both sizes.
 *
 * Note that size groups only affect the amount of space requested, not
 * the size that the widgets finally receive. If you want the widgets in
 * a `BobguiSizeGroup` to actually be the same size, you need to pack them in
 * such a way that they get the size they request and not more. In
 * particular it doesn't make a lot of sense to set
 * [the expand flags][method@Bobgui.Widget.set_hexpand] on the widgets that
 * are members of a size group.
 *
 * `BobguiSizeGroup` objects are referenced by each widget in the size group,
 * so once you have added all widgets to a `BobguiSizeGroup`, you can drop
 * the initial reference to the size group with
 * [method@GObject.Object.unref]. If the widgets in the size group are
 * subsequently destroyed, then they will be removed from the size group
 * and drop their references on the size group; when all widgets have been
 * removed, the size group will be freed.
 *
 * Widgets can be part of multiple size groups; BOBGUI will compute the
 * horizontal size of a widget from the horizontal requisition of all widgets
 * that can be reached from the widget by a chain of size groups with mode
 * [enum@Bobgui.SizeGroupMode.HORIZONTAL] or [enum@Bobgui.SizeGroupMode.BOTH], and
 * the vertical size from the vertical requisition of all widgets that can be
 * reached from the widget by a chain of size groups with mode
 * [enum@Bobgui.SizeGroupMode.VERTICAL] or [enum@Bobgui.SizeGroupMode.BOTH].
 *
 * # Size groups and trading height-for-width
 *
 * ::: warning
 *     Generally, size groups don't interact well with widgets that
 *     trade height for width (or width for height), such as wrappable
 *     labels. Avoid using size groups with such widgets.
 *
 * A size group with mode [enum@Bobgui.SizeGroupMode.HORIZONTAL] or
 * [enum@Bobgui.SizeGroupMode.VERTICAL] only consults non-contextual sizes
 * of widgets other than the one being measured, since it has no
 * knowledge of what size a widget will get allocated in the other
 * orientation. This can lead to widgets in a group actually requesting
 * different contextual sizes, contrary to the purpose of
 * `BobguiSizeGroup`.
 *
 * In contrast, a size group with mode [enum@Bobgui.SizeGroupMode.BOTH] can
 * properly propagate the available size in the opposite orientation
 * when measuring widgets in the group, which results in consistent and
 * accurate measurements.
 *
 * In case some mechanism other than a size group is already used to
 * ensure that widgets in a group all get the same size in one
 * orientation (for example, some common ancestor is known to allocate
 * the same width to all its children), and the size group is only
 * really needed to also make the widgets request the same size in the
 * other orientation, it is beneficial to still set the group's mode to
 * [enum@Bobgui.SizeGroupMode.BOTH]. This lets the group assume and count
 * on sizes of the widgets in the former orientation being the same,
 * which enables it to propagate the available size as described above.
 *
 * # Alternatives to size groups
 *
 * Size groups have many limitations, such as only influencing size
 * requests but not allocations, and poor height-for-width support. When
 * possible, prefer using dedicated mechanisms that can properly ensure
 * that the widgets get the same size.
 *
 * Various container widgets and layout managers support a homogeneous
 * layout mode, where they will explicitly give the same size to their
 * children (see [property@Bobgui.Box:homogeneous]). Using homogeneous mode
 * can also have large performance benefits compared to either the same
 * container in non-homogeneous mode, or to size groups.
 *
 * [class@Bobgui.Grid] can be used to position widgets into rows and
 * columns. Members of each column will have the same width among them;
 * likewise, members of each row will have the same height. On top of
 * that, the heights can be made equal between all rows with
 * [property@Bobgui.Grid:row-homogeneous], and the widths can be made equal
 * between all columns with [property@Bobgui.Grid:column-homogeneous].
 *
 * # BobguiSizeGroup as BobguiBuildable
 *
 * Size groups can be specified in a UI definition by placing an `<object>`
 * element with `class="BobguiSizeGroup"` somewhere in the UI definition. The
 * widgets that belong to the size group are specified by a `<widgets>` element
 * that may contain multiple `<widget>` elements, one for each member of the
 * size group. The ”name” attribute gives the id of the widget.
 *
 * An example of a UI definition fragment with `BobguiSizeGroup`:
 * ```xml
 * <object class="BobguiSizeGroup">
 *   <property name="mode">horizontal</property>
 *   <widgets>
 *     <widget name="radio1"/>
 *     <widget name="radio2"/>
 *   </widgets>
 * </object>
 * ```
 */

typedef struct _BobguiSizeGroupClass BobguiSizeGroupClass;
typedef struct _BobguiSizeGroupPrivate BobguiSizeGroupPrivate;

struct _BobguiSizeGroupClass
{
  GObjectClass parent_class;
};

struct _BobguiSizeGroupPrivate
{
  GSList         *widgets;

  guint8          mode;
};

enum {
  PROP_0,
  PROP_MODE
};

static void bobgui_size_group_set_property (GObject      *object,
					 guint         prop_id,
					 const GValue *value,
					 GParamSpec   *pspec);
static void bobgui_size_group_get_property (GObject      *object,
					 guint         prop_id,
					 GValue       *value,
					 GParamSpec   *pspec);

/* BobguiBuildable */
static void     bobgui_size_group_buildable_init             (BobguiBuildableIface  *iface);
static gboolean bobgui_size_group_buildable_custom_tag_start (BobguiBuildable       *buildable,
                                                           BobguiBuilder         *builder,
                                                           GObject            *child,
                                                           const char         *tagname,
                                                           BobguiBuildableParser *parser,
                                                           gpointer           *data);
static void     bobgui_size_group_buildable_custom_finished  (BobguiBuildable       *buildable,
                                                           BobguiBuilder         *builder,
                                                           GObject            *child,
                                                           const char         *tagname,
                                                           gpointer            user_data);

G_STATIC_ASSERT (BOBGUI_SIZE_GROUP_HORIZONTAL == (1 << BOBGUI_ORIENTATION_HORIZONTAL));
G_STATIC_ASSERT (BOBGUI_SIZE_GROUP_VERTICAL == (1 << BOBGUI_ORIENTATION_VERTICAL));
G_STATIC_ASSERT (BOBGUI_SIZE_GROUP_BOTH == (BOBGUI_SIZE_GROUP_HORIZONTAL | BOBGUI_SIZE_GROUP_VERTICAL));

G_DEFINE_TYPE_WITH_CODE (BobguiSizeGroup, bobgui_size_group, G_TYPE_OBJECT,
                         G_ADD_PRIVATE (BobguiSizeGroup)
			 G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_BUILDABLE,
						bobgui_size_group_buildable_init))

static void
add_widget_to_closure (GHashTable *peers,
                       GHashTable *peers_for_both,
                       GHashTable *groups,
                       BobguiWidget  *widget,
		       int         orientation)
{
  GSList *tmp_groups, *tmp_widgets;

  if (g_hash_table_lookup (peers, widget))
    return;

  g_hash_table_add (peers, widget);
  if (peers_for_both)
    g_hash_table_add (peers_for_both, widget);

  for (tmp_groups = _bobgui_widget_get_sizegroups (widget); tmp_groups; tmp_groups = tmp_groups->next)
    {
      BobguiSizeGroup        *tmp_group = tmp_groups->data;
      BobguiSizeGroupPrivate *tmp_priv  = bobgui_size_group_get_instance_private (tmp_group);

      if (g_hash_table_lookup (groups, tmp_group))
        continue;

      if (orientation >= 0 && !(tmp_priv->mode & (1 << orientation)))
        continue;

      g_hash_table_add (groups, tmp_group);

      for (tmp_widgets = tmp_priv->widgets; tmp_widgets; tmp_widgets = tmp_widgets->next)
        add_widget_to_closure (peers, tmp_priv->mode == BOBGUI_SIZE_GROUP_BOTH ? peers_for_both : NULL,
                               groups, tmp_widgets->data, orientation);
    }
}

void
_bobgui_size_group_get_widget_peers (BobguiWidget      *for_widget,
                                  BobguiOrientation  orientation,
                                  GHashTable    **peers,
                                  GHashTable    **peers_for_both)
{
  GHashTable *groups;

  *peers = g_hash_table_new (NULL, NULL);
  *peers_for_both = g_hash_table_new (NULL, NULL);
  groups = g_hash_table_new (NULL, NULL);

  add_widget_to_closure (*peers, *peers_for_both, groups, for_widget, orientation);

  g_hash_table_unref (groups);
}

static void
queue_resize_on_group (BobguiSizeGroup *size_group)
{
  BobguiSizeGroupPrivate *priv = bobgui_size_group_get_instance_private (size_group);
  GSList *list;

  for (list = priv->widgets; list; list = list->next)
    {
      bobgui_widget_queue_resize (list->data);
    }
}

static void
bobgui_size_group_class_init (BobguiSizeGroupClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->set_property = bobgui_size_group_set_property;
  gobject_class->get_property = bobgui_size_group_get_property;

  /**
   * BobguiSizeGroup:mode:
   *
   * The direction in which the size group affects requested sizes.
   */
  g_object_class_install_property (gobject_class,
                                   PROP_MODE,
                                   g_param_spec_enum ("mode", NULL, NULL,
                                                      BOBGUI_TYPE_SIZE_GROUP_MODE,
                                                      BOBGUI_SIZE_GROUP_HORIZONTAL,
                                                      BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));
}

static void
bobgui_size_group_init (BobguiSizeGroup *size_group)
{
  BobguiSizeGroupPrivate *priv;

  priv = bobgui_size_group_get_instance_private (size_group);

  priv->widgets = NULL;
  priv->mode = BOBGUI_SIZE_GROUP_HORIZONTAL;
}

static void
bobgui_size_group_buildable_init (BobguiBuildableIface *iface)
{
  iface->custom_tag_start = bobgui_size_group_buildable_custom_tag_start;
  iface->custom_finished = bobgui_size_group_buildable_custom_finished;
}

static void
bobgui_size_group_set_property (GObject      *object,
			     guint         prop_id,
			     const GValue *value,
			     GParamSpec   *pspec)
{
  BobguiSizeGroup *size_group = BOBGUI_SIZE_GROUP (object);

  switch (prop_id)
    {
    case PROP_MODE:
      bobgui_size_group_set_mode (size_group, g_value_get_enum (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_size_group_get_property (GObject      *object,
			     guint         prop_id,
			     GValue       *value,
			     GParamSpec   *pspec)
{
  BobguiSizeGroup *size_group = BOBGUI_SIZE_GROUP (object);
  BobguiSizeGroupPrivate *priv = bobgui_size_group_get_instance_private (size_group);

  switch (prop_id)
    {
    case PROP_MODE:
      g_value_set_enum (value, priv->mode);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

/**
 * bobgui_size_group_new:
 * @mode: the mode for the new size group.
 *
 * Create a new `BobguiSizeGroup`.
 *
 * Returns: a newly created `BobguiSizeGroup`
 */
BobguiSizeGroup *
bobgui_size_group_new (BobguiSizeGroupMode mode)
{
  BobguiSizeGroup *size_group = g_object_new (BOBGUI_TYPE_SIZE_GROUP, NULL);
  BobguiSizeGroupPrivate *priv = bobgui_size_group_get_instance_private (size_group);

  priv->mode = mode;

  return size_group;
}

/**
 * bobgui_size_group_set_mode:
 * @size_group: a `BobguiSizeGroup`
 * @mode: the mode to set for the size group.
 *
 * Sets the `BobguiSizeGroupMode` of the size group.
 *
 * The mode of the size group determines whether the widgets in the
 * size group should all have the same horizontal requisition
 * (%BOBGUI_SIZE_GROUP_HORIZONTAL) all have the same vertical requisition
 * (%BOBGUI_SIZE_GROUP_VERTICAL), or should all have the same requisition
 * in both directions (%BOBGUI_SIZE_GROUP_BOTH).
 */
void
bobgui_size_group_set_mode (BobguiSizeGroup     *size_group,
			 BobguiSizeGroupMode  mode)
{
  BobguiSizeGroupPrivate *priv = bobgui_size_group_get_instance_private (size_group);

  g_return_if_fail (BOBGUI_IS_SIZE_GROUP (size_group));

  if (priv->mode != mode)
    {
      if (priv->mode != BOBGUI_SIZE_GROUP_NONE)
	queue_resize_on_group (size_group);
      priv->mode = mode;
      if (priv->mode != BOBGUI_SIZE_GROUP_NONE)
	queue_resize_on_group (size_group);

      g_object_notify (G_OBJECT (size_group), "mode");
    }
}

/**
 * bobgui_size_group_get_mode:
 * @size_group: a `BobguiSizeGroup`
 *
 * Gets the current mode of the size group.
 *
 * Returns: the current mode of the size group.
 */
BobguiSizeGroupMode
bobgui_size_group_get_mode (BobguiSizeGroup *size_group)
{
  BobguiSizeGroupPrivate *priv = bobgui_size_group_get_instance_private (size_group);

  g_return_val_if_fail (BOBGUI_IS_SIZE_GROUP (size_group), BOBGUI_SIZE_GROUP_BOTH);

  return priv->mode;
}

/**
 * bobgui_size_group_add_widget:
 * @size_group: a `BobguiSizeGroup`
 * @widget: the `BobguiWidget` to add
 *
 * Adds a widget to a `BobguiSizeGroup`.
 *
 * In the future, the requisition
 * of the widget will be determined as the maximum of its requisition
 * and the requisition of the other widgets in the size group.
 * Whether this applies horizontally, vertically, or in both directions
 * depends on the mode of the size group.
 * See [method@Bobgui.SizeGroup.set_mode].
 *
 * When the widget is destroyed or no longer referenced elsewhere, it
 * will be removed from the size group.
 */
void
bobgui_size_group_add_widget (BobguiSizeGroup *size_group,
			   BobguiWidget    *widget)
{
  BobguiSizeGroupPrivate *priv = bobgui_size_group_get_instance_private (size_group);
  GSList *groups;
  
  g_return_if_fail (BOBGUI_IS_SIZE_GROUP (size_group));
  g_return_if_fail (BOBGUI_IS_WIDGET (widget));

  groups = _bobgui_widget_get_sizegroups (widget);

  if (!g_slist_find (groups, size_group))
    {
      _bobgui_widget_add_sizegroup (widget, size_group);

      priv->widgets = g_slist_prepend (priv->widgets, widget);

      g_object_ref (size_group);
    }
  
  queue_resize_on_group (size_group);
}

/**
 * bobgui_size_group_remove_widget:
 * @size_group: a `BobguiSizeGroup`
 * @widget: the `BobguiWidget` to remove
 *
 * Removes a widget from a `BobguiSizeGroup`.
 */
void
bobgui_size_group_remove_widget (BobguiSizeGroup *size_group,
			      BobguiWidget    *widget)
{
  BobguiSizeGroupPrivate *priv = bobgui_size_group_get_instance_private (size_group);
  
  g_return_if_fail (BOBGUI_IS_SIZE_GROUP (size_group));
  g_return_if_fail (BOBGUI_IS_WIDGET (widget));

  g_return_if_fail (g_slist_find (priv->widgets, widget));

  _bobgui_widget_remove_sizegroup (widget, size_group);

  priv->widgets = g_slist_remove (priv->widgets, widget);
  queue_resize_on_group (size_group);
  bobgui_widget_queue_resize (widget);

  g_object_unref (size_group);
}

/**
 * bobgui_size_group_get_widgets:
 * @size_group: a `BobguiSizeGroup`
 *
 * Returns the list of widgets associated with @size_group.
 *
 * Returns:  (element-type BobguiWidget) (transfer none): a `GSList` of
 *   widgets. The list is owned by BOBGUI and should not be modified.
 */
GSList *
bobgui_size_group_get_widgets (BobguiSizeGroup *size_group)
{
  BobguiSizeGroupPrivate *priv = bobgui_size_group_get_instance_private (size_group);

  return priv->widgets;
}

typedef struct {
  char *name;
  int line;
  int col;
} ItemData;

static void
item_data_free (gpointer data)
{
  ItemData *item_data = data;

  g_free (item_data->name);
  g_free (item_data);
}

typedef struct {
  GObject *object;
  BobguiBuilder *builder;
  GSList *items;
} GSListSubParserData;

static void
size_group_start_element (BobguiBuildableParseContext  *context,
                          const char                *element_name,
                          const char               **names,
                          const char               **values,
                          gpointer                   user_data,
                          GError                   **error)
{
  GSListSubParserData *data = (GSListSubParserData*)user_data;

  if (strcmp (element_name, "widget") == 0)
    {
      const char *name;
      ItemData *item_data;

      if (!_bobgui_builder_check_parent (data->builder, context, "widgets", error))
        return;

      if (!g_markup_collect_attributes (element_name, names, values, error,
                                        G_MARKUP_COLLECT_STRING, "name", &name,
                                        G_MARKUP_COLLECT_INVALID))
        {
          _bobgui_builder_prefix_error (data->builder, context, error);
          return;
        }

      item_data = g_new (ItemData, 1);
      item_data->name = g_strdup (name);
      bobgui_buildable_parse_context_get_position (context, &item_data->line, &item_data->col);
      data->items = g_slist_prepend (data->items, item_data);
    }
  else if (strcmp (element_name, "widgets") == 0)
    {
      if (!_bobgui_builder_check_parent (data->builder, context, "object", error))
        return;

      if (!g_markup_collect_attributes (element_name, names, values, error,
                                        G_MARKUP_COLLECT_INVALID, NULL, NULL,
                                        G_MARKUP_COLLECT_INVALID))
        _bobgui_builder_prefix_error (data->builder, context, error);
    }
  else
    {
      _bobgui_builder_error_unhandled_tag (data->builder, context,
                                        "BobguiSizeGroup", element_name,
                                        error);
    }
}

static const BobguiBuildableParser size_group_parser =
  {
    size_group_start_element
  };

static gboolean
bobgui_size_group_buildable_custom_tag_start (BobguiBuildable       *buildable,
                                           BobguiBuilder         *builder,
                                           GObject            *child,
                                           const char         *tagname,
                                           BobguiBuildableParser *parser,
                                           gpointer           *parser_data)
{
  GSListSubParserData *data;

  if (child)
    return FALSE;

  if (strcmp (tagname, "widgets") == 0)
    {
      data = g_new0 (GSListSubParserData, 1);
      data->items = NULL;
      data->object = G_OBJECT (buildable);
      data->builder = builder;

      *parser = size_group_parser;
      *parser_data = data;

      return TRUE;
    }

  return FALSE;
}

static void
bobgui_size_group_buildable_custom_finished (BobguiBuildable  *buildable,
                                          BobguiBuilder    *builder,
                                          GObject       *child,
                                          const char    *tagname,
                                          gpointer       user_data)
{
  GSList *l;
  GSListSubParserData *data;
  GObject *object;

  if (strcmp (tagname, "widgets") != 0)
    return;

  data = (GSListSubParserData*)user_data;
  data->items = g_slist_reverse (data->items);

  for (l = data->items; l; l = l->next)
    {
      ItemData *item_data = l->data;
      object = _bobgui_builder_lookup_object (builder, item_data->name, item_data->line, item_data->col);
      if (!object)
        continue;
      bobgui_size_group_add_widget (BOBGUI_SIZE_GROUP (data->object), BOBGUI_WIDGET (object));
    }
  g_slist_free_full (data->items, item_data_free);
  g_free (data);
}
