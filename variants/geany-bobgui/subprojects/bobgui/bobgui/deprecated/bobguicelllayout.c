/* bobguicelllayout.c
 * Copyright (C) 2003  Kristian Rietveld  <kris@bobgui.org>
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
 * BobguiCellLayout:
 *
 * An interface for packing cells
 *
 * `BobguiCellLayout` is an interface to be implemented by all objects which
 * want to provide a `BobguiTreeViewColumn` like API for packing cells,
 * setting attributes and data funcs.
 *
 * One of the notable features provided by implementations of
 * `BobguiCellLayout` are attributes. Attributes let you set the properties
 * in flexible ways. They can just be set to constant values like regular
 * properties. But they can also be mapped to a column of the underlying
 * tree model with bobgui_cell_layout_set_attributes(), which means that the value
 * of the attribute can change from cell to cell as they are rendered by
 * the cell renderer. Finally, it is possible to specify a function with
 * bobgui_cell_layout_set_cell_data_func() that is called to determine the
 * value of the attribute for each cell that is rendered.
 *
 * ## BobguiCellLayouts as BobguiBuildable
 *
 * Implementations of BobguiCellLayout which also implement the BobguiBuildable
 * interface (`BobguiCellView`, `BobguiIconView`, `BobguiComboBox`,
 * `BobguiEntryCompletion`, `BobguiTreeViewColumn`) accept `BobguiCellRenderer` objects
 * as `<child>` elements in UI definitions. They support a custom `<attributes>`
 * element for their children, which can contain multiple `<attribute>`
 * elements. Each `<attribute>` element has a name attribute which specifies
 * a property of the cell renderer; the content of the element is the
 * attribute value.
 *
 * This is an example of a UI definition fragment specifying attributes:
 *
 * ```xml
 * <object class="BobguiCellView">
 *   <child>
 *     <object class="BobguiCellRendererText"/>
 *     <attributes>
 *       <attribute name="text">0</attribute>
 *     </attributes>
 *   </child>
 * </object>
 * ```
 *
 * Furthermore for implementations of `BobguiCellLayout` that use a `BobguiCellArea`
 * to lay out cells (all `BobguiCellLayout`s in BOBGUI use a `BobguiCellArea`)
 * [cell properties](class.CellArea.html#cell-properties) can also be defined
 * in the format by specifying the custom `<cell-packing>` attribute which can
 * contain multiple `<property>` elements.
 *
 * Here is a UI definition fragment specifying cell properties:
 *
 * ```xml
 * <object class="BobguiTreeViewColumn">
 *   <child>
 *     <object class="BobguiCellRendererText"/>
 *     <cell-packing>
 *       <property name="align">True</property>
 *       <property name="expand">False</property>
 *     </cell-packing>
 *   </child>
 * </object>
 * ```
 *
 * ## Subclassing BobguiCellLayout implementations
 *
 * When subclassing a widget that implements `BobguiCellLayout` like
 * `BobguiIconView` or `BobguiComboBox`, there are some considerations related
 * to the fact that these widgets internally use a `BobguiCellArea`.
 * The cell area is exposed as a construct-only property by these
 * widgets. This means that it is possible to e.g. do
 *
 * ```c
 * BobguiWIdget *combo =
 *   g_object_new (BOBGUI_TYPE_COMBO_BOX, "cell-area", my_cell_area, NULL);
 * ```
 *
 * to use a custom cell area with a combo box. But construct properties
 * are only initialized after instance `init()`
 * functions have run, which means that using functions which rely on
 * the existence of the cell area in your subclass `init()` function will
 * cause the default cell area to be instantiated. In this case, a provided
 * construct property value will be ignored (with a warning, to alert
 * you to the problem).
 *
 * ```c
 * static void
 * my_combo_box_init (MyComboBox *b)
 * {
 *   BobguiCellRenderer *cell;
 *
 *   cell = bobgui_cell_renderer_pixbuf_new ();
 *
 *   // The following call causes the default cell area for combo boxes,
 *   // a BobguiCellAreaBox, to be instantiated
 *   bobgui_cell_layout_pack_start (BOBGUI_CELL_LAYOUT (b), cell, FALSE);
 *   ...
 * }
 *
 * BobguiWidget *
 * my_combo_box_new (BobguiCellArea *area)
 * {
 *   // This call is going to cause a warning about area being ignored
 *   return g_object_new (MY_TYPE_COMBO_BOX, "cell-area", area, NULL);
 * }
 * ```
 *
 * If supporting alternative cell areas with your derived widget is
 * not important, then this does not have to concern you. If you want
 * to support alternative cell areas, you can do so by moving the
 * problematic calls out of `init()` and into a `constructor()`
 * for your class.
 *
 * Deprecated: 4.10: List views use widgets to display their contents.
 *   See [class@Bobgui.LayoutManager] for layout manager delegate objects
 */

#include "config.h"
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "bobguicelllayout.h"
#include "bobguibuilderprivate.h"

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

#define warn_no_cell_area(func)					\
  g_critical ("%s: Called but no BobguiCellArea is available yet", func)

typedef BobguiCellLayoutIface BobguiCellLayoutInterface;
G_DEFINE_INTERFACE (BobguiCellLayout, bobgui_cell_layout, G_TYPE_OBJECT);

static void   bobgui_cell_layout_default_pack_start         (BobguiCellLayout         *cell_layout,
							  BobguiCellRenderer       *cell,
							  gboolean               expand);
static void   bobgui_cell_layout_default_pack_end           (BobguiCellLayout         *cell_layout,
							  BobguiCellRenderer       *cell,
							  gboolean               expand);
static void   bobgui_cell_layout_default_clear              (BobguiCellLayout         *cell_layout);
static void   bobgui_cell_layout_default_add_attribute      (BobguiCellLayout         *cell_layout,
							  BobguiCellRenderer       *cell,
							  const char            *attribute,
							  int                    column);
static void   bobgui_cell_layout_default_set_cell_data_func (BobguiCellLayout         *cell_layout,
							  BobguiCellRenderer       *cell,
							  BobguiCellLayoutDataFunc  func,
							  gpointer               func_data,
							  GDestroyNotify         destroy);
static void   bobgui_cell_layout_default_clear_attributes   (BobguiCellLayout         *cell_layout,
							  BobguiCellRenderer       *cell);
static void   bobgui_cell_layout_default_reorder            (BobguiCellLayout         *cell_layout,
							  BobguiCellRenderer       *cell,
							  int                    position);
static GList *bobgui_cell_layout_default_get_cells          (BobguiCellLayout         *cell_layout);


static void
bobgui_cell_layout_default_init (BobguiCellLayoutIface *iface)
{
  iface->pack_start         = bobgui_cell_layout_default_pack_start;
  iface->pack_end           = bobgui_cell_layout_default_pack_end;
  iface->clear              = bobgui_cell_layout_default_clear;
  iface->add_attribute      = bobgui_cell_layout_default_add_attribute;
  iface->set_cell_data_func = bobgui_cell_layout_default_set_cell_data_func;
  iface->clear_attributes   = bobgui_cell_layout_default_clear_attributes;
  iface->reorder            = bobgui_cell_layout_default_reorder;
  iface->get_cells          = bobgui_cell_layout_default_get_cells;
}

/* Default implementation is to fall back on an underlying cell area */
static void
bobgui_cell_layout_default_pack_start (BobguiCellLayout         *cell_layout,
				    BobguiCellRenderer       *cell,
				    gboolean               expand)
{
  BobguiCellLayoutIface *iface;
  BobguiCellArea        *area;

  iface = BOBGUI_CELL_LAYOUT_GET_IFACE (cell_layout);

  if (iface->get_area)
    {
      area = iface->get_area (cell_layout);

      if (area)
	bobgui_cell_layout_pack_start (BOBGUI_CELL_LAYOUT (area), cell, expand);
      else
	warn_no_cell_area ("BobguiCellLayoutIface->pack_start()");
    }
}

static void
bobgui_cell_layout_default_pack_end (BobguiCellLayout         *cell_layout,
				  BobguiCellRenderer       *cell,
				  gboolean               expand)
{
  BobguiCellLayoutIface *iface;
  BobguiCellArea        *area;

  iface = BOBGUI_CELL_LAYOUT_GET_IFACE (cell_layout);

  if (iface->get_area)
    {
      area = iface->get_area (cell_layout);

      if (area)
	bobgui_cell_layout_pack_end (BOBGUI_CELL_LAYOUT (area), cell, expand);
      else
	warn_no_cell_area ("BobguiCellLayoutIface->pack_end()");
    }
}

static void
bobgui_cell_layout_default_clear (BobguiCellLayout *cell_layout)
{
  BobguiCellLayoutIface *iface;
  BobguiCellArea        *area;

  iface = BOBGUI_CELL_LAYOUT_GET_IFACE (cell_layout);

  if (iface->get_area)
    {
      area = iface->get_area (cell_layout);

      if (area)
	bobgui_cell_layout_clear (BOBGUI_CELL_LAYOUT (area));
      else
	warn_no_cell_area ("BobguiCellLayoutIface->clear()");
    }
}

static void
bobgui_cell_layout_default_add_attribute (BobguiCellLayout         *cell_layout,
				       BobguiCellRenderer       *cell,
				       const char            *attribute,
				       int                    column)
{
  BobguiCellLayoutIface *iface;
  BobguiCellArea        *area;

  iface = BOBGUI_CELL_LAYOUT_GET_IFACE (cell_layout);

  if (iface->get_area)
    {
      area = iface->get_area (cell_layout);

      if (area)
	bobgui_cell_layout_add_attribute (BOBGUI_CELL_LAYOUT (area), cell, attribute, column);
      else
	warn_no_cell_area ("BobguiCellLayoutIface->add_attribute()");
    }
}

static void
bobgui_cell_layout_default_set_cell_data_func (BobguiCellLayout         *cell_layout,
					    BobguiCellRenderer       *cell,
					    BobguiCellLayoutDataFunc  func,
					    gpointer               func_data,
					    GDestroyNotify         destroy)
{
  BobguiCellLayoutIface *iface;
  BobguiCellArea        *area;

  iface = BOBGUI_CELL_LAYOUT_GET_IFACE (cell_layout);

  if (iface->get_area)
    {
      area = iface->get_area (cell_layout);

      if (area)
	_bobgui_cell_area_set_cell_data_func_with_proxy (area, cell,
						      (GFunc)func, func_data, destroy,
						      cell_layout);
      else
	warn_no_cell_area ("BobguiCellLayoutIface->set_cell_data_func()");
    }
}

static void
bobgui_cell_layout_default_clear_attributes (BobguiCellLayout         *cell_layout,
					  BobguiCellRenderer       *cell)
{
  BobguiCellLayoutIface *iface;
  BobguiCellArea        *area;

  iface = BOBGUI_CELL_LAYOUT_GET_IFACE (cell_layout);

  if (iface->get_area)
    {
      area = iface->get_area (cell_layout);

      if (area)
	bobgui_cell_layout_clear_attributes (BOBGUI_CELL_LAYOUT (area), cell);
      else
	warn_no_cell_area ("BobguiCellLayoutIface->clear_attributes()");
    }
}

static void
bobgui_cell_layout_default_reorder (BobguiCellLayout         *cell_layout,
				 BobguiCellRenderer       *cell,
				 int                    position)
{
  BobguiCellLayoutIface *iface;
  BobguiCellArea        *area;

  iface = BOBGUI_CELL_LAYOUT_GET_IFACE (cell_layout);

  if (iface->get_area)
    {
      area = iface->get_area (cell_layout);

      if (area)
	bobgui_cell_layout_reorder (BOBGUI_CELL_LAYOUT (area), cell, position);
      else
	warn_no_cell_area ("BobguiCellLayoutIface->reorder()");
    }
}

static GList *
bobgui_cell_layout_default_get_cells (BobguiCellLayout *cell_layout)
{
  BobguiCellLayoutIface *iface;
  BobguiCellArea        *area;

  iface = BOBGUI_CELL_LAYOUT_GET_IFACE (cell_layout);

  if (iface->get_area)
    {
      area = iface->get_area (cell_layout);

      if (area)
	return bobgui_cell_layout_get_cells (BOBGUI_CELL_LAYOUT (area));
      else
	warn_no_cell_area ("BobguiCellLayoutIface->get_cells()");
    }
  return NULL;
}


/**
 * bobgui_cell_layout_pack_start:
 * @cell_layout: a `BobguiCellLayout`
 * @cell: a `BobguiCellRenderer`
 * @expand: %TRUE if @cell is to be given extra space allocated to @cell_layout
 *
 * Packs the @cell into the beginning of @cell_layout. If @expand is %FALSE,
 * then the @cell is allocated no more space than it needs. Any unused space
 * is divided evenly between cells for which @expand is %TRUE.
 *
 * Note that reusing the same cell renderer is not supported.
 *
 * Deprecated: 4.10
 */
void
bobgui_cell_layout_pack_start (BobguiCellLayout   *cell_layout,
                            BobguiCellRenderer *cell,
                            gboolean         expand)
{
  g_return_if_fail (BOBGUI_IS_CELL_LAYOUT (cell_layout));
  g_return_if_fail (BOBGUI_IS_CELL_RENDERER (cell));

  BOBGUI_CELL_LAYOUT_GET_IFACE (cell_layout)->pack_start (cell_layout, cell, expand);
}

/**
 * bobgui_cell_layout_pack_end:
 * @cell_layout: a `BobguiCellLayout`
 * @cell: a `BobguiCellRenderer`
 * @expand: %TRUE if @cell is to be given extra space allocated to @cell_layout
 *
 * Adds the @cell to the end of @cell_layout. If @expand is %FALSE, then the
 * @cell is allocated no more space than it needs. Any unused space is
 * divided evenly between cells for which @expand is %TRUE.
 *
 * Note that reusing the same cell renderer is not supported.
 *
 * Deprecated: 4.10
 */
void
bobgui_cell_layout_pack_end (BobguiCellLayout   *cell_layout,
                          BobguiCellRenderer *cell,
                          gboolean         expand)
{
  g_return_if_fail (BOBGUI_IS_CELL_LAYOUT (cell_layout));
  g_return_if_fail (BOBGUI_IS_CELL_RENDERER (cell));

  BOBGUI_CELL_LAYOUT_GET_IFACE (cell_layout)->pack_end (cell_layout, cell, expand);
}

/**
 * bobgui_cell_layout_clear:
 * @cell_layout: a `BobguiCellLayout`
 *
 * Unsets all the mappings on all renderers on @cell_layout and
 * removes all renderers from @cell_layout.
 *
 * Deprecated: 4.10
 */
void
bobgui_cell_layout_clear (BobguiCellLayout *cell_layout)
{
  g_return_if_fail (BOBGUI_IS_CELL_LAYOUT (cell_layout));

  BOBGUI_CELL_LAYOUT_GET_IFACE (cell_layout)->clear (cell_layout);
}

static void
bobgui_cell_layout_set_attributesv (BobguiCellLayout   *cell_layout,
                                 BobguiCellRenderer *cell,
                                 va_list          args)
{
  char *attribute;
  int column;

  attribute = va_arg (args, char *);

  bobgui_cell_layout_clear_attributes (cell_layout, cell);

  while (attribute != NULL)
    {
      column = va_arg (args, int);

      bobgui_cell_layout_add_attribute (cell_layout, cell, attribute, column);

      attribute = va_arg (args, char *);
    }
}

/**
 * bobgui_cell_layout_set_attributes:
 * @cell_layout: a `BobguiCellLayout`
 * @cell: a `BobguiCellRenderer`
 * @...: a %NULL-terminated list of attributes
 *
 * Sets the attributes in the parameter list as the attributes
 * of @cell_layout.
 *
 * See [method@Bobgui.CellLayout.add_attribute] for more details.
 *
 * The attributes should be in attribute/column order, as in
 * bobgui_cell_layout_add_attribute(). All existing attributes are
 * removed, and replaced with the new attributes.
 *
 * Deprecated: 4.10
 */
void
bobgui_cell_layout_set_attributes (BobguiCellLayout   *cell_layout,
                                BobguiCellRenderer *cell,
                                ...)
{
  va_list args;

  g_return_if_fail (BOBGUI_IS_CELL_LAYOUT (cell_layout));
  g_return_if_fail (BOBGUI_IS_CELL_RENDERER (cell));

  va_start (args, cell);
  bobgui_cell_layout_set_attributesv (cell_layout, cell, args);
  va_end (args);
}

/**
 * bobgui_cell_layout_add_attribute:
 * @cell_layout: a `BobguiCellLayout`
 * @cell: a `BobguiCellRenderer`
 * @attribute: a property on the renderer
 * @column: the column position on the model to get the attribute from
 *
 * Adds an attribute mapping to the list in @cell_layout.
 *
 * The @column is the column of the model to get a value from, and the
 * @attribute is the property on @cell to be set from that value. So for
 * example if column 2 of the model contains strings, you could have the
 * “text” attribute of a `BobguiCellRendererText` get its values from column 2.
 * In this context "attribute" and "property" are used interchangeably.
 *
 * Deprecated: 4.10
 */
void
bobgui_cell_layout_add_attribute (BobguiCellLayout   *cell_layout,
                               BobguiCellRenderer *cell,
                               const char      *attribute,
                               int              column)
{
  g_return_if_fail (BOBGUI_IS_CELL_LAYOUT (cell_layout));
  g_return_if_fail (BOBGUI_IS_CELL_RENDERER (cell));
  g_return_if_fail (attribute != NULL);
  g_return_if_fail (column >= 0);

  BOBGUI_CELL_LAYOUT_GET_IFACE (cell_layout)->add_attribute (cell_layout, cell, attribute, column);
}

/**
 * bobgui_cell_layout_set_cell_data_func:
 * @cell_layout: a `BobguiCellLayout`
 * @cell: a `BobguiCellRenderer`
 * @func: (nullable) (scope notified) (closure func_data) (destroy destroy): the `BobguiCellLayout`DataFunc to use
 * @func_data: user data for @func
 * @destroy: destroy notify for @func_data
 *
 * Sets the `BobguiCellLayout`DataFunc to use for @cell_layout.
 *
 * This function is used instead of the standard attributes mapping
 * for setting the column value, and should set the value of @cell_layout’s
 * cell renderer(s) as appropriate.
 *
 * @func may be %NULL to remove a previously set function.
 *
 * Deprecated: 4.10
 */
void
bobgui_cell_layout_set_cell_data_func (BobguiCellLayout         *cell_layout,
                                    BobguiCellRenderer       *cell,
                                    BobguiCellLayoutDataFunc  func,
                                    gpointer               func_data,
                                    GDestroyNotify         destroy)
{
  g_return_if_fail (BOBGUI_IS_CELL_LAYOUT (cell_layout));
  g_return_if_fail (BOBGUI_IS_CELL_RENDERER (cell));

  BOBGUI_CELL_LAYOUT_GET_IFACE
    (cell_layout)->set_cell_data_func (cell_layout, cell, func, func_data, destroy);
}

/**
 * bobgui_cell_layout_clear_attributes:
 * @cell_layout: a `BobguiCellLayout`
 * @cell: a `BobguiCellRenderer` to clear the attribute mapping on
 *
 * Clears all existing attributes previously set with
 * bobgui_cell_layout_set_attributes().
 *
 * Deprecated: 4.10
 */
void
bobgui_cell_layout_clear_attributes (BobguiCellLayout   *cell_layout,
                                  BobguiCellRenderer *cell)
{
  g_return_if_fail (BOBGUI_IS_CELL_LAYOUT (cell_layout));
  g_return_if_fail (BOBGUI_IS_CELL_RENDERER (cell));

  BOBGUI_CELL_LAYOUT_GET_IFACE (cell_layout)->clear_attributes (cell_layout, cell);
}

/**
 * bobgui_cell_layout_reorder:
 * @cell_layout: a `BobguiCellLayout`
 * @cell: a `BobguiCellRenderer` to reorder
 * @position: new position to insert @cell at
 *
 * Re-inserts @cell at @position.
 *
 * Note that @cell has already to be packed into @cell_layout
 * for this to function properly.
 *
 * Deprecated: 4.10
 */
void
bobgui_cell_layout_reorder (BobguiCellLayout   *cell_layout,
                         BobguiCellRenderer *cell,
                         int              position)
{
  g_return_if_fail (BOBGUI_IS_CELL_LAYOUT (cell_layout));
  g_return_if_fail (BOBGUI_IS_CELL_RENDERER (cell));

  BOBGUI_CELL_LAYOUT_GET_IFACE (cell_layout)->reorder (cell_layout, cell, position);
}

/**
 * bobgui_cell_layout_get_cells:
 * @cell_layout: a `BobguiCellLayout`
 *
 * Returns the cell renderers which have been added to @cell_layout.
 *
 * Returns: (element-type BobguiCellRenderer) (transfer container):
 *   a list of cell renderers. The list, but not the renderers has
 *   been newly allocated and should be freed with g_list_free()
 *   when no longer needed.
 *
 * Deprecated: 4.10
 */
GList *
bobgui_cell_layout_get_cells (BobguiCellLayout *cell_layout)
{
  g_return_val_if_fail (BOBGUI_IS_CELL_LAYOUT (cell_layout), NULL);

  return BOBGUI_CELL_LAYOUT_GET_IFACE (cell_layout)->get_cells (cell_layout);
}

/**
 * bobgui_cell_layout_get_area:
 * @cell_layout: a `BobguiCellLayout`
 *
 * Returns the underlying `BobguiCellArea` which might be @cell_layout
 * if called on a `BobguiCellArea` or might be %NULL if no `BobguiCellArea`
 * is used by @cell_layout.
 *
 * Returns: (transfer none) (nullable): the cell area used by @cell_layout
 *
 * Deprecated: 4.10
 */
BobguiCellArea *
bobgui_cell_layout_get_area (BobguiCellLayout *cell_layout)
{
  BobguiCellLayoutIface *iface;

  g_return_val_if_fail (BOBGUI_IS_CELL_LAYOUT (cell_layout), NULL);

  iface = BOBGUI_CELL_LAYOUT_GET_IFACE (cell_layout);
  if (iface->get_area)
    return iface->get_area (cell_layout);

  return NULL;
}

/* Attribute parsing */
typedef struct {
  BobguiCellLayout   *cell_layout;
  BobguiCellRenderer *renderer;
  BobguiBuilder      *builder;
  char            *attr_name;
  GString         *string;
} AttributesSubParserData;

static void
attributes_start_element (BobguiBuildableParseContext *context,
                          const char               *element_name,
                          const char              **names,
                          const char              **values,
                          gpointer                  user_data,
                          GError                  **error)
{
  AttributesSubParserData *data = (AttributesSubParserData*)user_data;

  if (strcmp (element_name, "attribute") == 0)
    {
      const char *name;

      if (!_bobgui_builder_check_parent (data->builder, context, "attributes", error))
        return;

      if (!g_markup_collect_attributes (element_name, names, values, error,
                                        G_MARKUP_COLLECT_STRING, "name", &name,
                                        G_MARKUP_COLLECT_INVALID))
        {
          _bobgui_builder_prefix_error (data->builder, context, error);
          return;
        }

      data->attr_name = g_strdup (name);
    }
  else if (strcmp (element_name, "attributes") == 0)
    {
      if (!_bobgui_builder_check_parent (data->builder, context, "child", error))
        return;

      if (!g_markup_collect_attributes (element_name, names, values, error,
                                        G_MARKUP_COLLECT_INVALID, NULL, NULL,
                                        G_MARKUP_COLLECT_INVALID))
        _bobgui_builder_prefix_error (data->builder, context, error);
    }
  else
    {
      _bobgui_builder_error_unhandled_tag (data->builder, context,
                                        "BobguiCellLayout", element_name,
                                        error);
    }
}

static void
attributes_text_element (BobguiBuildableParseContext  *context,
                         const char                *text,
                         gsize                      text_len,
                         gpointer                   user_data,
                         GError                   **error)
{
  AttributesSubParserData *data = (AttributesSubParserData*)user_data;

  if (data->attr_name)
    g_string_append_len (data->string, text, text_len);
}

static void
attributes_end_element (BobguiBuildableParseContext  *context,
                        const char                *element_name,
                        gpointer                   user_data,
                        GError                   **error)
{
  AttributesSubParserData *data = (AttributesSubParserData*)user_data;
  GValue val = G_VALUE_INIT;

  if (!data->attr_name)
    return;

  if (!bobgui_builder_value_from_string_type (data->builder, G_TYPE_INT, data->string->str, &val, error))
    {
      _bobgui_builder_prefix_error (data->builder, context, error);
       return;
    }

  bobgui_cell_layout_add_attribute (data->cell_layout,
				 data->renderer,
				 data->attr_name,
                                 g_value_get_int (&val));

  g_free (data->attr_name);
  data->attr_name = NULL;

  g_string_set_size (data->string, 0);
}

static const BobguiBuildableParser attributes_parser =
  {
    attributes_start_element,
    attributes_end_element,
    attributes_text_element
  };


/* Cell packing parsing */
static void
bobgui_cell_layout_buildable_set_cell_property (BobguiCellArea     *area,
					     BobguiBuilder      *builder,
					     BobguiCellRenderer *cell,
					     char            *name,
					     const char      *value)
{
  GParamSpec *pspec;
  GValue gvalue = G_VALUE_INIT;
  GError *error = NULL;

  pspec = bobgui_cell_area_class_find_cell_property (BOBGUI_CELL_AREA_GET_CLASS (area), name);
  if (!pspec)
    {
      g_warning ("%s does not have a property called %s",
		 g_type_name (G_OBJECT_TYPE (area)), name);
      return;
    }

  if (!bobgui_builder_value_from_string (builder, pspec, value, &gvalue, &error))
    {
      g_warning ("Could not read property %s:%s with value %s of type %s: %s",
		 g_type_name (G_OBJECT_TYPE (area)),
		 name,
		 value,
		 g_type_name (G_PARAM_SPEC_VALUE_TYPE (pspec)),
		 error->message);
      g_error_free (error);
      return;
    }

  bobgui_cell_area_cell_set_property (area, cell, name, &gvalue);
  g_value_unset (&gvalue);
}

typedef struct {
  BobguiBuilder      *builder;
  BobguiCellLayout   *cell_layout;
  BobguiCellRenderer *renderer;
  GString         *string;
  char            *cell_prop_name;
  char            *context;
  gboolean         translatable;
} CellPackingSubParserData;

static void
cell_packing_start_element (BobguiBuildableParseContext *context,
                            const char               *element_name,
                            const char              **names,
                            const char              **values,
                            gpointer                  user_data,
                            GError                  **error)
{
  CellPackingSubParserData *data = (CellPackingSubParserData*)user_data;

  if (strcmp (element_name, "property") == 0)
    {
      const char *name;
      gboolean translatable = FALSE;
      const char *ctx = NULL;

      if (!_bobgui_builder_check_parent (data->builder, context, "cell-packing", error))
        return;

      if (!g_markup_collect_attributes (element_name, names, values, error,
                                        G_MARKUP_COLLECT_STRING, "name", &name,
                                        G_MARKUP_COLLECT_BOOLEAN|G_MARKUP_COLLECT_OPTIONAL, "translatable", &translatable,
                                        G_MARKUP_COLLECT_STRING|G_MARKUP_COLLECT_OPTIONAL, "comments", NULL,
                                        G_MARKUP_COLLECT_STRING|G_MARKUP_COLLECT_OPTIONAL, "context", &ctx,
                                        G_MARKUP_COLLECT_INVALID))
       {
         _bobgui_builder_prefix_error (data->builder, context, error);
         return;
       }

      data->cell_prop_name = g_strdup (name);
      data->translatable = translatable;
      data->context = g_strdup (ctx);
    }
  else if (strcmp (element_name, "cell-packing") == 0)
    {
      if (!_bobgui_builder_check_parent (data->builder, context, "child", error))
        return;

      if (!g_markup_collect_attributes (element_name, names, values, error,
                                        G_MARKUP_COLLECT_INVALID, NULL, NULL,
                                        G_MARKUP_COLLECT_INVALID))
        _bobgui_builder_prefix_error (data->builder, context, error);
    }
  else
    {
      _bobgui_builder_error_unhandled_tag (data->builder, context,
                                        "BobguiCellLayout", element_name,
                                        error);
    }
}

static void
cell_packing_text_element (BobguiBuildableParseContext *context,
                           const char               *text,
                           gsize                     text_len,
                           gpointer                  user_data,
                           GError                  **error)
{
  CellPackingSubParserData *data = (CellPackingSubParserData*)user_data;

  if (data->cell_prop_name)
    g_string_append_len (data->string, text, text_len);
}

static void
cell_packing_end_element (BobguiBuildableParseContext *context,
                          const char               *element_name,
                          gpointer                  user_data,
                          GError                  **error)
{
  CellPackingSubParserData *data = (CellPackingSubParserData*)user_data;
  BobguiCellArea *area;

  area = bobgui_cell_layout_get_area (data->cell_layout);

  if (area)
    {
      /* translate the string */
      if (data->string->len && data->translatable)
	{
	  const char *translated;
	  const char * domain;

	  domain = bobgui_builder_get_translation_domain (data->builder);

	  translated = _bobgui_builder_parser_translate (domain,
						      data->context,
						      data->string->str);
	  g_string_assign (data->string, translated);
	}

      if (data->cell_prop_name)
	bobgui_cell_layout_buildable_set_cell_property (area,
						     data->builder,
						     data->renderer,
						     data->cell_prop_name,
						     data->string->str);
    }
  else
    g_warning ("%s does not have an internal BobguiCellArea class and cannot apply child cell properties",
	       g_type_name (G_OBJECT_TYPE (data->cell_layout)));

  g_string_set_size (data->string, 0);
  g_free (data->cell_prop_name);
  g_free (data->context);
  data->cell_prop_name = NULL;
  data->context = NULL;
  data->translatable = FALSE;
}


static const BobguiBuildableParser cell_packing_parser =
  {
    cell_packing_start_element,
    cell_packing_end_element,
    cell_packing_text_element
  };

gboolean
_bobgui_cell_layout_buildable_custom_tag_start (BobguiBuildable       *buildable,
                                             BobguiBuilder         *builder,
                                             GObject            *child,
                                             const char         *tagname,
                                             BobguiBuildableParser *parser,
                                             gpointer           *data)
{
  AttributesSubParserData  *attr_data;
  CellPackingSubParserData *packing_data;

  if (!child)
    return FALSE;

  if (strcmp (tagname, "attributes") == 0)
    {
      attr_data = g_slice_new0 (AttributesSubParserData);
      attr_data->cell_layout = BOBGUI_CELL_LAYOUT (buildable);
      attr_data->renderer = BOBGUI_CELL_RENDERER (child);
      attr_data->builder = builder;
      attr_data->attr_name = NULL;
      attr_data->string = g_string_new ("");

      *parser = attributes_parser;
      *data = attr_data;

      return TRUE;
    }
  else if (strcmp (tagname, "cell-packing") == 0)
    {
      packing_data = g_slice_new0 (CellPackingSubParserData);
      packing_data->string = g_string_new ("");
      packing_data->builder = builder;
      packing_data->cell_layout = BOBGUI_CELL_LAYOUT (buildable);
      packing_data->renderer = BOBGUI_CELL_RENDERER (child);

      *parser = cell_packing_parser;
      *data = packing_data;

      return TRUE;
    }

  return FALSE;
}

gboolean
_bobgui_cell_layout_buildable_custom_tag_end (BobguiBuildable *buildable,
					   BobguiBuilder   *builder,
					   GObject      *child,
					   const char   *tagname,
					   gpointer     *data)
{
  AttributesSubParserData *attr_data;
  CellPackingSubParserData *packing_data;

  if (strcmp (tagname, "attributes") == 0)
    {
      attr_data = (AttributesSubParserData*)data;
      g_assert (!attr_data->attr_name);
      g_string_free (attr_data->string, TRUE);
      g_slice_free (AttributesSubParserData, attr_data);

      return TRUE;
    }
  else if (strcmp (tagname, "cell-packing") == 0)
    {
      packing_data = (CellPackingSubParserData *)data;
      g_string_free (packing_data->string, TRUE);
      g_slice_free (CellPackingSubParserData, packing_data);

      return TRUE;
    }
  return FALSE;
}

void
_bobgui_cell_layout_buildable_add_child (BobguiBuildable      *buildable,
				      BobguiBuilder        *builder,
				      GObject           *child,
				      const char        *type)
{
  g_return_if_fail (BOBGUI_IS_CELL_LAYOUT (buildable));
  g_return_if_fail (BOBGUI_IS_CELL_RENDERER (child));

  bobgui_cell_layout_pack_start (BOBGUI_CELL_LAYOUT (buildable), BOBGUI_CELL_RENDERER (child), FALSE);
}
