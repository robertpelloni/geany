/* bobguicellarea.h
 *
 * Copyright (C) 2010 Openismus GmbH
 *
 * Authors:
 *      Tristan Van Berkom <tristanvb@openismus.com>
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

#pragma once

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <bobgui/bobguiwidget.h>
#include <bobgui/deprecated/bobguicellrenderer.h>
#include <bobgui/deprecated/bobguitreemodel.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_CELL_AREA                (bobgui_cell_area_get_type ())
#define BOBGUI_CELL_AREA(obj)                (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_CELL_AREA, BobguiCellArea))
#define BOBGUI_CELL_AREA_CLASS(klass)        (G_TYPE_CHECK_CLASS_CAST ((klass), BOBGUI_TYPE_CELL_AREA, BobguiCellAreaClass))
#define BOBGUI_IS_CELL_AREA(obj)     (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_CELL_AREA))
#define BOBGUI_IS_CELL_AREA_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BOBGUI_TYPE_CELL_AREA))
#define BOBGUI_CELL_AREA_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_CELL_AREA, BobguiCellAreaClass))

typedef struct _BobguiCellArea              BobguiCellArea;
typedef struct _BobguiCellAreaClass         BobguiCellAreaClass;
typedef struct _BobguiCellAreaContext       BobguiCellAreaContext;

/**
 * BOBGUI_CELL_AREA_WARN_INVALID_CELL_PROPERTY_ID:
 * @object: the `GObject` on which set_cell_property() or get_cell_property()
 *   was called
 * @property_id: the numeric id of the property
 * @pspec: the `GParamSpec` of the property
 *
 * This macro should be used to emit a standard warning about unexpected
 * properties in set_cell_property() and get_cell_property() implementations.
 *
 * Deprecated: 4.20: There is no replacement
 */
#define BOBGUI_CELL_AREA_WARN_INVALID_CELL_PROPERTY_ID(object, property_id, pspec) \
  G_OBJECT_WARN_INVALID_PSPEC ((object), "cell property id", (property_id), (pspec))

/**
 * BobguiCellCallback:
 * @renderer: the cell renderer to operate on
 * @data: (closure): user-supplied data
 *
 * The type of the callback functions used for iterating over
 * the cell renderers of a `BobguiCellArea`, see bobgui_cell_area_foreach().
 *
 * Returns: %TRUE to stop iterating over cells.
 *
 * Deprecated: 4.20: There is no replacement
 */
typedef gboolean    (*BobguiCellCallback) (BobguiCellRenderer  *renderer,
                                        gpointer          data);

/**
 * BobguiCellAllocCallback:
 * @renderer: the cell renderer to operate on
 * @cell_area: the area allocated to @renderer inside the rectangle
 *   provided to bobgui_cell_area_foreach_alloc().
 * @cell_background: the background area for @renderer inside the
 *   background area provided to bobgui_cell_area_foreach_alloc().
 * @data: (closure): user-supplied data
 *
 * The type of the callback functions used for iterating over the
 * cell renderers and their allocated areas inside a `BobguiCellArea`,
 * see bobgui_cell_area_foreach_alloc().
 *
 * Returns: %TRUE to stop iterating over cells.
 *
 * Deprecated: 4.20: There is no replacement
 */
typedef gboolean    (*BobguiCellAllocCallback) (BobguiCellRenderer    *renderer,
                                             const GdkRectangle *cell_area,
                                             const GdkRectangle *cell_background,
                                             gpointer            data);


struct _BobguiCellArea
{
  /*< private >*/
  GInitiallyUnowned parent_instance;
};


/**
 * BobguiCellAreaClass:
 * @add: adds a `BobguiCellRenderer` to the area.
 * @remove: removes a `BobguiCellRenderer` from the area.
 * @foreach: calls the `BobguiCellCallback` function on every `BobguiCellRenderer` in
 *   the area with the provided user data until the callback returns %TRUE.
 * @foreach_alloc: Calls the `BobguiCellAllocCallback` function on every
 *   `BobguiCellRenderer` in the area with the allocated area for the cell
 *   and the provided user data until the callback returns %TRUE.
 * @event: Handle an event in the area, this is generally used to activate
 *   a cell at the event location for button events but can also be used
 *   to generically pass events to `BobguiWidget`s drawn onto the area.
 * @snapshot: Actually snapshot the area’s cells to the specified rectangle,
 *   @background_area should be correctly distributed to the cells
 *   corresponding background areas.
 * @apply_attributes: Apply the cell attributes to the cells. This is
 *   implemented as a signal and generally `BobguiCellArea` subclasses don't
 *   need to implement it since it is handled by the base class.
 * @create_context: Creates and returns a class specific `BobguiCellAreaContext`
 *   to store cell alignment and allocation details for a said `BobguiCellArea`
 *   class.
 * @copy_context: Creates a new `BobguiCellAreaContext` in the same state as
 *   the passed @context with any cell alignment data and allocations intact.
 * @get_request_mode: This allows an area to tell its layouting widget whether
 *   it prefers to be allocated in %BOBGUI_SIZE_REQUEST_HEIGHT_FOR_WIDTH or
 *   %BOBGUI_SIZE_REQUEST_WIDTH_FOR_HEIGHT mode.
 * @get_preferred_width: Calculates the minimum and natural width of the
 *   areas cells with the current attributes applied while considering
 *   the particular layouting details of the said `BobguiCellArea`. While
 *   requests are performed over a series of rows, alignments and overall
 *   minimum and natural sizes should be stored in the corresponding
 *   `BobguiCellAreaContext`.
 * @get_preferred_height_for_width: Calculates the minimum and natural height
 *   for the area if the passed @context would be allocated the given width.
 *   When implementing this virtual method it is safe to assume that @context
 *   has already stored the aligned cell widths for every `BobguiTreeModel` row
 *   that @context will be allocated for since this information was stored
 *   at `BobguiCellAreaClass.get_preferred_width()` time. This virtual method
 *   should also store any necessary alignments of cell heights for the
 *   case that the context is allocated a height.
 * @get_preferred_height: Calculates the minimum and natural height of the
 *   areas cells with the current attributes applied. Essentially this is
 *   the same as `BobguiCellAreaClass.get_preferred_width()` only for areas
 *   that are being requested as %BOBGUI_SIZE_REQUEST_WIDTH_FOR_HEIGHT.
 * @get_preferred_width_for_height: Calculates the minimum and natural width
 *   for the area if the passed @context would be allocated the given
 *   height. The same as `BobguiCellAreaClass.get_preferred_height_for_width()`
 *   only for handling requests in the %BOBGUI_SIZE_REQUEST_WIDTH_FOR_HEIGHT
 *   mode.
 * @set_cell_property: This should be implemented to handle changes in child
 *   cell properties for a given `BobguiCellRenderer` that were previously
 *   installed on the `BobguiCellAreaClass` with bobgui_cell_area_class_install_cell_property().
 * @get_cell_property: This should be implemented to report the values of
 *   child cell properties for a given child `BobguiCellRenderer`.
 * @focus: This virtual method should be implemented to navigate focus from
 *   cell to cell inside the `BobguiCellArea`. The `BobguiCellArea` should move
 *   focus from cell to cell inside the area and return %FALSE if focus
 *   logically leaves the area with the following exceptions: When the
 *   area contains no activatable cells, the entire area receives focus.
 *   Focus should not be given to cells that are actually “focus siblings”
 *   of other sibling cells (see bobgui_cell_area_get_focus_from_sibling()).
 *   Focus is set by calling bobgui_cell_area_set_focus_cell().
 * @is_activatable: Returns whether the `BobguiCellArea` can respond to
 *   `BobguiCellAreaClass.activate()`, usually this does not need to be
 *   implemented since the base class takes care of this however it can
 *   be enhanced if the `BobguiCellArea` subclass can handle activation in
 *   other ways than activating its `BobguiCellRenderers`.
 * @activate: This is called when the layouting widget rendering the
 *   `BobguiCellArea` activates the focus cell (see bobgui_cell_area_get_focus_cell()).
 */
struct _BobguiCellAreaClass
{
  /*< private >*/
  GInitiallyUnownedClass parent_class;

  /*< public >*/

  /* Basic methods */
  void               (* add)                             (BobguiCellArea             *area,
                                                          BobguiCellRenderer         *renderer);
  void               (* remove)                          (BobguiCellArea             *area,
                                                          BobguiCellRenderer         *renderer);
  void               (* foreach)                         (BobguiCellArea             *area,
                                                          BobguiCellCallback          callback,
                                                          gpointer                 callback_data);
  void               (* foreach_alloc)                   (BobguiCellArea             *area,
                                                          BobguiCellAreaContext      *context,
                                                          BobguiWidget               *widget,
                                                          const GdkRectangle      *cell_area,
                                                          const GdkRectangle      *background_area,
                                                          BobguiCellAllocCallback     callback,
                                                          gpointer                 callback_data);
  int                (* event)                           (BobguiCellArea             *area,
                                                          BobguiCellAreaContext      *context,
                                                          BobguiWidget               *widget,
                                                          GdkEvent                *event,
                                                          const GdkRectangle      *cell_area,
                                                          BobguiCellRendererState     flags);
  void               (* snapshot)                        (BobguiCellArea             *area,
                                                          BobguiCellAreaContext      *context,
                                                          BobguiWidget               *widget,
                                                          BobguiSnapshot             *snapshot,
                                                          const GdkRectangle      *background_area,
                                                          const GdkRectangle      *cell_area,
                                                          BobguiCellRendererState     flags,
                                                          gboolean                 paint_focus);
  void               (* apply_attributes)                (BobguiCellArea             *area,
                                                          BobguiTreeModel            *tree_model,
                                                          BobguiTreeIter             *iter,
                                                          gboolean                 is_expander,
                                                          gboolean                 is_expanded);

  /* Geometry */
  BobguiCellAreaContext *(* create_context)                 (BobguiCellArea             *area);
  BobguiCellAreaContext *(* copy_context)                   (BobguiCellArea             *area,
                                                          BobguiCellAreaContext      *context);
  BobguiSizeRequestMode (* get_request_mode)                (BobguiCellArea             *area);
  void               (* get_preferred_width)             (BobguiCellArea             *area,
                                                          BobguiCellAreaContext      *context,
                                                          BobguiWidget               *widget,
                                                          int                     *minimum_width,
                                                          int                     *natural_width);
  void               (* get_preferred_height_for_width)  (BobguiCellArea             *area,
                                                          BobguiCellAreaContext      *context,
                                                          BobguiWidget               *widget,
                                                          int                      width,
                                                          int                     *minimum_height,
                                                          int                     *natural_height);
  void               (* get_preferred_height)            (BobguiCellArea             *area,
                                                          BobguiCellAreaContext      *context,
                                                          BobguiWidget               *widget,
                                                          int                     *minimum_height,
                                                          int                     *natural_height);
  void               (* get_preferred_width_for_height)  (BobguiCellArea             *area,
                                                          BobguiCellAreaContext      *context,
                                                          BobguiWidget               *widget,
                                                          int                      height,
                                                          int                     *minimum_width,
                                                          int                     *natural_width);

  /* Cell Properties */
  void               (* set_cell_property)               (BobguiCellArea             *area,
                                                          BobguiCellRenderer         *renderer,
                                                          guint                    property_id,
                                                          const GValue            *value,
                                                          GParamSpec              *pspec);
  void               (* get_cell_property)               (BobguiCellArea             *area,
                                                          BobguiCellRenderer         *renderer,
                                                          guint                    property_id,
                                                          GValue                  *value,
                                                          GParamSpec              *pspec);

  /* Focus */
  gboolean           (* focus)                           (BobguiCellArea             *area,
                                                          BobguiDirectionType         direction);
  gboolean           (* is_activatable)                  (BobguiCellArea             *area);
  gboolean           (* activate)                        (BobguiCellArea             *area,
                                                          BobguiCellAreaContext      *context,
                                                          BobguiWidget               *widget,
                                                          const GdkRectangle      *cell_area,
                                                          BobguiCellRendererState     flags,
                                                          gboolean                 edit_only);

  /*< private >*/

  gpointer padding[8];
};

GDK_AVAILABLE_IN_ALL
GType                 bobgui_cell_area_get_type                       (void) G_GNUC_CONST;

/* Basic methods */
GDK_DEPRECATED_IN_4_10
void                  bobgui_cell_area_add                            (BobguiCellArea          *area,
                                                                    BobguiCellRenderer      *renderer);
GDK_DEPRECATED_IN_4_10
void                  bobgui_cell_area_remove                         (BobguiCellArea          *area,
                                                                    BobguiCellRenderer      *renderer);
GDK_DEPRECATED_IN_4_10
gboolean              bobgui_cell_area_has_renderer                   (BobguiCellArea          *area,
                                                                    BobguiCellRenderer      *renderer);
GDK_DEPRECATED_IN_4_10
void                  bobgui_cell_area_foreach                        (BobguiCellArea          *area,
                                                                    BobguiCellCallback       callback,
                                                                    gpointer              callback_data);
GDK_DEPRECATED_IN_4_10
void                  bobgui_cell_area_foreach_alloc                  (BobguiCellArea          *area,
                                                                    BobguiCellAreaContext   *context,
                                                                    BobguiWidget            *widget,
                                                                    const GdkRectangle   *cell_area,
                                                                    const GdkRectangle   *background_area,
                                                                    BobguiCellAllocCallback  callback,
                                                                    gpointer              callback_data);
GDK_DEPRECATED_IN_4_10
int                   bobgui_cell_area_event                          (BobguiCellArea          *area,
                                                                    BobguiCellAreaContext   *context,
                                                                    BobguiWidget            *widget,
                                                                    GdkEvent             *event,
                                                                    const GdkRectangle   *cell_area,
                                                                    BobguiCellRendererState  flags);
GDK_DEPRECATED_IN_4_10
void                  bobgui_cell_area_snapshot                       (BobguiCellArea          *area,
                                                                    BobguiCellAreaContext   *context,
                                                                    BobguiWidget            *widget,
                                                                    BobguiSnapshot          *snapshot,
                                                                    const GdkRectangle   *background_area,
                                                                    const GdkRectangle   *cell_area,
                                                                    BobguiCellRendererState  flags,
                                                                    gboolean              paint_focus);

GDK_DEPRECATED_IN_4_10
void                  bobgui_cell_area_get_cell_allocation            (BobguiCellArea          *area,
                                                                    BobguiCellAreaContext   *context,
                                                                    BobguiWidget            *widget,
                                                                    BobguiCellRenderer      *renderer,
                                                                    const GdkRectangle   *cell_area,
                                                                    GdkRectangle         *allocation);
GDK_DEPRECATED_IN_4_10
BobguiCellRenderer      *bobgui_cell_area_get_cell_at_position           (BobguiCellArea          *area,
                                                                    BobguiCellAreaContext   *context,
                                                                    BobguiWidget            *widget,
                                                                    const GdkRectangle   *cell_area,
                                                                    int                   x,
                                                                    int                   y,
                                                                    GdkRectangle         *alloc_area);

/* Geometry */
GDK_DEPRECATED_IN_4_10
BobguiCellAreaContext   *bobgui_cell_area_create_context                 (BobguiCellArea        *area);
GDK_DEPRECATED_IN_4_10
BobguiCellAreaContext   *bobgui_cell_area_copy_context                   (BobguiCellArea        *area,
                                                                    BobguiCellAreaContext *context);
GDK_DEPRECATED_IN_4_10
BobguiSizeRequestMode    bobgui_cell_area_get_request_mode               (BobguiCellArea        *area);
GDK_DEPRECATED_IN_4_10
void                  bobgui_cell_area_get_preferred_width            (BobguiCellArea        *area,
                                                                    BobguiCellAreaContext *context,
                                                                    BobguiWidget          *widget,
                                                                    int                *minimum_width,
                                                                    int                *natural_width);
GDK_DEPRECATED_IN_4_10
void                  bobgui_cell_area_get_preferred_height_for_width (BobguiCellArea        *area,
                                                                    BobguiCellAreaContext *context,
                                                                    BobguiWidget          *widget,
                                                                    int                 width,
                                                                    int                *minimum_height,
                                                                    int                *natural_height);
GDK_DEPRECATED_IN_4_10
void                  bobgui_cell_area_get_preferred_height           (BobguiCellArea        *area,
                                                                    BobguiCellAreaContext *context,
                                                                    BobguiWidget          *widget,
                                                                    int                *minimum_height,
                                                                    int                *natural_height);
GDK_DEPRECATED_IN_4_10
void                  bobgui_cell_area_get_preferred_width_for_height (BobguiCellArea        *area,
                                                                    BobguiCellAreaContext *context,
                                                                    BobguiWidget          *widget,
                                                                    int                 height,
                                                                    int                *minimum_width,
                                                                    int                *natural_width);
GDK_DEPRECATED_IN_4_10
const char *         bobgui_cell_area_get_current_path_string        (BobguiCellArea        *area);


/* Attributes */
GDK_DEPRECATED_IN_4_10
void                  bobgui_cell_area_apply_attributes               (BobguiCellArea        *area,
                                                                    BobguiTreeModel       *tree_model,
                                                                    BobguiTreeIter        *iter,
                                                                    gboolean            is_expander,
                                                                    gboolean            is_expanded);
GDK_DEPRECATED_IN_4_10
void                  bobgui_cell_area_attribute_connect              (BobguiCellArea        *area,
                                                                    BobguiCellRenderer    *renderer,
                                                                    const char         *attribute,
                                                                    int                 column);
GDK_DEPRECATED_IN_4_10
void                  bobgui_cell_area_attribute_disconnect           (BobguiCellArea        *area,
                                                                    BobguiCellRenderer    *renderer,
                                                                    const char         *attribute);
GDK_DEPRECATED_IN_4_10
int                   bobgui_cell_area_attribute_get_column           (BobguiCellArea        *area,
                                                                    BobguiCellRenderer    *renderer,
                                                                    const char         *attribute);


/* Cell Properties */
GDK_DEPRECATED_IN_4_10
void                  bobgui_cell_area_class_install_cell_property    (BobguiCellAreaClass   *aclass,
                                                                    guint               property_id,
                                                                    GParamSpec         *pspec);
GDK_DEPRECATED_IN_4_10
GParamSpec*           bobgui_cell_area_class_find_cell_property       (BobguiCellAreaClass   *aclass,
                                                                    const char         *property_name);
GDK_DEPRECATED_IN_4_10
GParamSpec**          bobgui_cell_area_class_list_cell_properties     (BobguiCellAreaClass   *aclass,
                                                                    guint                   *n_properties);
GDK_DEPRECATED_IN_4_10
void                  bobgui_cell_area_add_with_properties            (BobguiCellArea        *area,
                                                                    BobguiCellRenderer    *renderer,
                                                                    const char      *first_prop_name,
                                                                    ...) G_GNUC_NULL_TERMINATED;
GDK_DEPRECATED_IN_4_10
void                  bobgui_cell_area_cell_set                       (BobguiCellArea        *area,
                                                                    BobguiCellRenderer    *renderer,
                                                                    const char         *first_prop_name,
                                                                    ...) G_GNUC_NULL_TERMINATED;
GDK_DEPRECATED_IN_4_10
void                  bobgui_cell_area_cell_get                       (BobguiCellArea        *area,
                                                                    BobguiCellRenderer    *renderer,
                                                                    const char         *first_prop_name,
                                                                    ...) G_GNUC_NULL_TERMINATED;
GDK_DEPRECATED_IN_4_10
void                  bobgui_cell_area_cell_set_valist                (BobguiCellArea        *area,
                                                                    BobguiCellRenderer    *renderer,
                                                                    const char         *first_property_name,
                                                                    va_list             var_args);
GDK_DEPRECATED_IN_4_10
void                  bobgui_cell_area_cell_get_valist                (BobguiCellArea        *area,
                                                                    BobguiCellRenderer    *renderer,
                                                                    const char         *first_property_name,
                                                                    va_list             var_args);
GDK_DEPRECATED_IN_4_10
void                  bobgui_cell_area_cell_set_property              (BobguiCellArea        *area,
                                                                    BobguiCellRenderer    *renderer,
                                                                    const char         *property_name,
                                                                    const GValue       *value);
GDK_DEPRECATED_IN_4_10
void                  bobgui_cell_area_cell_get_property              (BobguiCellArea        *area,
                                                                    BobguiCellRenderer    *renderer,
                                                                    const char         *property_name,
                                                                    GValue             *value);

/* Focus */
GDK_DEPRECATED_IN_4_10
gboolean              bobgui_cell_area_is_activatable                 (BobguiCellArea         *area);
GDK_DEPRECATED_IN_4_10
gboolean              bobgui_cell_area_activate                       (BobguiCellArea         *area,
                                                                    BobguiCellAreaContext  *context,
                                                                    BobguiWidget           *widget,
                                                                    const GdkRectangle  *cell_area,
                                                                    BobguiCellRendererState flags,
                                                                    gboolean             edit_only);
GDK_DEPRECATED_IN_4_10
gboolean              bobgui_cell_area_focus                          (BobguiCellArea         *area,
                                                                    BobguiDirectionType     direction);
GDK_DEPRECATED_IN_4_10
void                  bobgui_cell_area_set_focus_cell                 (BobguiCellArea          *area,
                                                                    BobguiCellRenderer      *renderer);
GDK_DEPRECATED_IN_4_10
BobguiCellRenderer      *bobgui_cell_area_get_focus_cell                 (BobguiCellArea          *area);


/* Focus siblings */
GDK_DEPRECATED_IN_4_10
void                  bobgui_cell_area_add_focus_sibling              (BobguiCellArea          *area,
                                                                    BobguiCellRenderer      *renderer,
                                                                    BobguiCellRenderer      *sibling);
GDK_DEPRECATED_IN_4_10
void                  bobgui_cell_area_remove_focus_sibling           (BobguiCellArea          *area,
                                                                    BobguiCellRenderer      *renderer,
                                                                    BobguiCellRenderer      *sibling);
GDK_DEPRECATED_IN_4_10
gboolean              bobgui_cell_area_is_focus_sibling               (BobguiCellArea          *area,
                                                                    BobguiCellRenderer      *renderer,
                                                                    BobguiCellRenderer      *sibling);
GDK_DEPRECATED_IN_4_10
const GList *         bobgui_cell_area_get_focus_siblings             (BobguiCellArea          *area,
                                                                    BobguiCellRenderer      *renderer);
GDK_DEPRECATED_IN_4_10
BobguiCellRenderer      *bobgui_cell_area_get_focus_from_sibling         (BobguiCellArea          *area,
                                                                    BobguiCellRenderer      *renderer);

/* Cell Activation/Editing */
GDK_DEPRECATED_IN_4_10
BobguiCellRenderer      *bobgui_cell_area_get_edited_cell                (BobguiCellArea          *area);
GDK_DEPRECATED_IN_4_10
BobguiCellEditable      *bobgui_cell_area_get_edit_widget                (BobguiCellArea          *area);
GDK_DEPRECATED_IN_4_10
gboolean              bobgui_cell_area_activate_cell                  (BobguiCellArea          *area,
                                                                    BobguiWidget            *widget,
                                                                    BobguiCellRenderer      *renderer,
                                                                    GdkEvent             *event,
                                                                    const GdkRectangle   *cell_area,
                                                                    BobguiCellRendererState  flags);
GDK_DEPRECATED_IN_4_10
void                  bobgui_cell_area_stop_editing                   (BobguiCellArea          *area,
                                                                    gboolean              canceled);

/* Functions for area implementations */

/* Distinguish the inner cell area from the whole requested area including margins */
GDK_DEPRECATED_IN_4_10
void                  bobgui_cell_area_inner_cell_area                (BobguiCellArea        *area,
                                                                    BobguiWidget          *widget,
                                                                    const GdkRectangle *cell_area,
                                                                    GdkRectangle       *inner_area);

/* Request the size of a cell while respecting the cell margins (requests are margin inclusive) */
GDK_DEPRECATED_IN_4_10
void                  bobgui_cell_area_request_renderer               (BobguiCellArea        *area,
                                                                    BobguiCellRenderer    *renderer,
                                                                    BobguiOrientation      orientation,
                                                                    BobguiWidget          *widget,
                                                                    int                 for_size,
                                                                    int                *minimum_size,
                                                                    int                *natural_size);

/* For api stability, this is called from bobguicelllayout.c in order to ensure the correct
 * object is passed to the user function in bobgui_cell_layout_set_cell_data_func.
 *
 * This private api takes gpointer & GFunc arguments to circumvent circular header file
 * dependencies.
 */
void                 _bobgui_cell_area_set_cell_data_func_with_proxy  (BobguiCellArea           *area,
								    BobguiCellRenderer       *cell,
								    GFunc                  func,
								    gpointer               func_data,
								    GDestroyNotify         destroy,
								    gpointer               proxy);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiCellArea, g_object_unref)

G_END_DECLS

