/* bobguicellrenderer.h
 * Copyright (C) 2000  Red Hat, Inc.,  Jonathan Blandford <jrb@redhat.com>
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

#include <bobgui/deprecated/bobguicelleditable.h>

G_BEGIN_DECLS


/**
 * BobguiCellRendererState:
 * @BOBGUI_CELL_RENDERER_SELECTED: The cell is currently selected, and
 *  probably has a selection colored background to render to.
 * @BOBGUI_CELL_RENDERER_PRELIT: The mouse is hovering over the cell.
 * @BOBGUI_CELL_RENDERER_INSENSITIVE: The cell is drawn in an insensitive manner
 * @BOBGUI_CELL_RENDERER_SORTED: The cell is in a sorted row
 * @BOBGUI_CELL_RENDERER_FOCUSED: The cell is in the focus row.
 * @BOBGUI_CELL_RENDERER_EXPANDABLE: The cell is in a row that can be expanded
 * @BOBGUI_CELL_RENDERER_EXPANDED: The cell is in a row that is expanded
 *
 * Tells how a cell is to be rendered.
 *
 * Deprecated: 4.20: There is no replacement.
 */
typedef enum
{
  BOBGUI_CELL_RENDERER_SELECTED    = 1 << 0,
  BOBGUI_CELL_RENDERER_PRELIT      = 1 << 1,
  BOBGUI_CELL_RENDERER_INSENSITIVE = 1 << 2,
  /* this flag means the cell is in the sort column/row */
  BOBGUI_CELL_RENDERER_SORTED      = 1 << 3,
  BOBGUI_CELL_RENDERER_FOCUSED     = 1 << 4,
  BOBGUI_CELL_RENDERER_EXPANDABLE  = 1 << 5,
  BOBGUI_CELL_RENDERER_EXPANDED    = 1 << 6
} BobguiCellRendererState;

/**
 * BobguiCellRendererMode:
 * @BOBGUI_CELL_RENDERER_MODE_INERT: The cell is just for display
 *  and cannot be interacted with.  Note that this doesn’t mean that eg. the
 *  row being drawn can’t be selected -- just that a particular element of
 *  it cannot be individually modified.
 * @BOBGUI_CELL_RENDERER_MODE_ACTIVATABLE: The cell can be clicked.
 * @BOBGUI_CELL_RENDERER_MODE_EDITABLE: The cell can be edited or otherwise modified.
 *
 * Identifies how the user can interact with a particular cell.
 *
 * Deprecated: 4.20: There is no replacement.
 */
typedef enum
{
  BOBGUI_CELL_RENDERER_MODE_INERT,
  BOBGUI_CELL_RENDERER_MODE_ACTIVATABLE,
  BOBGUI_CELL_RENDERER_MODE_EDITABLE
} BobguiCellRendererMode;

#define BOBGUI_TYPE_CELL_RENDERER		  (bobgui_cell_renderer_get_type ())
#define BOBGUI_CELL_RENDERER(obj)		  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_CELL_RENDERER, BobguiCellRenderer))
#define BOBGUI_CELL_RENDERER_CLASS(klass)	  (G_TYPE_CHECK_CLASS_CAST ((klass), BOBGUI_TYPE_CELL_RENDERER, BobguiCellRendererClass))
#define BOBGUI_IS_CELL_RENDERER(obj)	  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_CELL_RENDERER))
#define BOBGUI_IS_CELL_RENDERER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BOBGUI_TYPE_CELL_RENDERER))
#define BOBGUI_CELL_RENDERER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_CELL_RENDERER, BobguiCellRendererClass))

typedef struct _BobguiCellRenderer              BobguiCellRenderer;
typedef struct _BobguiCellRendererPrivate       BobguiCellRendererPrivate;
typedef struct _BobguiCellRendererClass         BobguiCellRendererClass;
typedef struct _BobguiCellRendererClassPrivate  BobguiCellRendererClassPrivate;

struct _BobguiCellRenderer
{
  GInitiallyUnowned parent_instance;

  /*< private >*/
  BobguiCellRendererPrivate *priv;
};

/**
 * BobguiCellRendererClass:
 * @get_request_mode: Called to gets whether the cell renderer prefers
 *    a height-for-width layout or a width-for-height layout.
 * @get_preferred_width: Called to get a renderer’s natural width.
 * @get_preferred_height_for_width: Called to get a renderer’s natural height for width.
 * @get_preferred_height: Called to get a renderer’s natural height.
 * @get_preferred_width_for_height: Called to get a renderer’s natural width for height.
 * @get_aligned_area: Called to get the aligned area used by @cell inside @cell_area.
 * @snapshot: Called to snapshot the content of the `BobguiCellRenderer`.
 * @activate: Called to activate the content of the `BobguiCellRenderer`.
 * @start_editing: Called to initiate editing the content of the `BobguiCellRenderer`.
 * @editing_canceled: Signal gets emitted when the user cancels the process of editing a cell.
 * @editing_started: Signal gets emitted when a cell starts to be edited.
 */
struct _BobguiCellRendererClass
{
  /*< private >*/
  GInitiallyUnownedClass parent_class;

  /*< public >*/

  /* vtable - not signals */
  BobguiSizeRequestMode (* get_request_mode)                (BobguiCellRenderer      *cell);
  void               (* get_preferred_width)             (BobguiCellRenderer      *cell,
                                                          BobguiWidget            *widget,
                                                          int                  *minimum_size,
                                                          int                  *natural_size);
  void               (* get_preferred_height_for_width)  (BobguiCellRenderer      *cell,
                                                          BobguiWidget            *widget,
                                                          int                   width,
                                                          int                  *minimum_height,
                                                          int                  *natural_height);
  void               (* get_preferred_height)            (BobguiCellRenderer      *cell,
                                                          BobguiWidget            *widget,
                                                          int                  *minimum_size,
                                                          int                  *natural_size);
  void               (* get_preferred_width_for_height)  (BobguiCellRenderer      *cell,
                                                          BobguiWidget            *widget,
                                                          int                   height,
                                                          int                  *minimum_width,
                                                          int                  *natural_width);
  void               (* get_aligned_area)                (BobguiCellRenderer      *cell,
                                                          BobguiWidget            *widget,
							  BobguiCellRendererState  flags,
                                                          const GdkRectangle   *cell_area,
                                                          GdkRectangle         *aligned_area);
  void               (* snapshot)                        (BobguiCellRenderer      *cell,
                                                          BobguiSnapshot          *snapshot,
                                                          BobguiWidget            *widget,
                                                          const GdkRectangle   *background_area,
                                                          const GdkRectangle   *cell_area,
                                                          BobguiCellRendererState  flags);
  gboolean           (* activate)                        (BobguiCellRenderer      *cell,
                                                          GdkEvent             *event,
                                                          BobguiWidget            *widget,
                                                          const char           *path,
                                                          const GdkRectangle   *background_area,
                                                          const GdkRectangle   *cell_area,
                                                          BobguiCellRendererState  flags);
  BobguiCellEditable *  (* start_editing)                   (BobguiCellRenderer      *cell,
                                                          GdkEvent             *event,
                                                          BobguiWidget            *widget,
                                                          const char           *path,
                                                          const GdkRectangle   *background_area,
                                                          const GdkRectangle   *cell_area,
                                                          BobguiCellRendererState  flags);

  /* Signals */
  void (* editing_canceled) (BobguiCellRenderer *cell);
  void (* editing_started)  (BobguiCellRenderer *cell,
			     BobguiCellEditable *editable,
			     const char      *path);

  /*< private >*/
  gpointer padding[8];
};

GDK_AVAILABLE_IN_ALL
GType              bobgui_cell_renderer_get_type       (void) G_GNUC_CONST;

GDK_DEPRECATED_IN_4_10
BobguiSizeRequestMode bobgui_cell_renderer_get_request_mode               (BobguiCellRenderer    *cell);
GDK_DEPRECATED_IN_4_10
void               bobgui_cell_renderer_get_preferred_width            (BobguiCellRenderer    *cell,
                                                                     BobguiWidget          *widget,
                                                                     int                *minimum_size,
                                                                     int                *natural_size);
GDK_DEPRECATED_IN_4_10
void               bobgui_cell_renderer_get_preferred_height_for_width (BobguiCellRenderer    *cell,
                                                                     BobguiWidget          *widget,
                                                                     int                 width,
                                                                     int                *minimum_height,
                                                                     int                *natural_height);
GDK_DEPRECATED_IN_4_10
void               bobgui_cell_renderer_get_preferred_height           (BobguiCellRenderer    *cell,
                                                                     BobguiWidget          *widget,
                                                                     int                *minimum_size,
                                                                     int                *natural_size);
GDK_DEPRECATED_IN_4_10
void               bobgui_cell_renderer_get_preferred_width_for_height (BobguiCellRenderer    *cell,
                                                                     BobguiWidget          *widget,
                                                                     int                 height,
                                                                     int                *minimum_width,
                                                                     int                *natural_width);
GDK_DEPRECATED_IN_4_10
void               bobgui_cell_renderer_get_preferred_size             (BobguiCellRenderer    *cell,
                                                                     BobguiWidget          *widget,
                                                                     BobguiRequisition     *minimum_size,
                                                                     BobguiRequisition     *natural_size);
GDK_DEPRECATED_IN_4_10
void               bobgui_cell_renderer_get_aligned_area               (BobguiCellRenderer    *cell,
								     BobguiWidget          *widget,
								     BobguiCellRendererState flags,
								     const GdkRectangle *cell_area,
								     GdkRectangle       *aligned_area);
GDK_DEPRECATED_IN_4_10
void             bobgui_cell_renderer_snapshot       (BobguiCellRenderer      *cell,
                                                   BobguiSnapshot          *snapshot,
						   BobguiWidget            *widget,
						   const GdkRectangle   *background_area,
						   const GdkRectangle   *cell_area,
						   BobguiCellRendererState  flags);
GDK_DEPRECATED_IN_4_10
gboolean         bobgui_cell_renderer_activate       (BobguiCellRenderer      *cell,
						   GdkEvent             *event,
						   BobguiWidget            *widget,
						   const char           *path,
						   const GdkRectangle   *background_area,
						   const GdkRectangle   *cell_area,
						   BobguiCellRendererState  flags);
GDK_DEPRECATED_IN_4_10
BobguiCellEditable *bobgui_cell_renderer_start_editing  (BobguiCellRenderer      *cell,
						   GdkEvent             *event,
						   BobguiWidget            *widget,
						   const char           *path,
						   const GdkRectangle   *background_area,
						   const GdkRectangle   *cell_area,
						   BobguiCellRendererState  flags);

GDK_DEPRECATED_IN_4_10
void             bobgui_cell_renderer_set_fixed_size (BobguiCellRenderer      *cell,
						   int                   width,
						   int                   height);
GDK_DEPRECATED_IN_4_10
void             bobgui_cell_renderer_get_fixed_size (BobguiCellRenderer      *cell,
						   int                  *width,
						   int                  *height);

GDK_DEPRECATED_IN_4_10
void             bobgui_cell_renderer_set_alignment  (BobguiCellRenderer      *cell,
                                                   float                 xalign,
                                                   float                 yalign);
GDK_DEPRECATED_IN_4_10
void             bobgui_cell_renderer_get_alignment  (BobguiCellRenderer      *cell,
                                                   float                *xalign,
                                                   float                *yalign);

GDK_DEPRECATED_IN_4_10
void             bobgui_cell_renderer_set_padding    (BobguiCellRenderer      *cell,
                                                   int                   xpad,
                                                   int                   ypad);
GDK_DEPRECATED_IN_4_10
void             bobgui_cell_renderer_get_padding    (BobguiCellRenderer      *cell,
                                                   int                  *xpad,
                                                   int                  *ypad);

GDK_DEPRECATED_IN_4_10
void             bobgui_cell_renderer_set_visible    (BobguiCellRenderer      *cell,
                                                   gboolean              visible);
GDK_DEPRECATED_IN_4_10
gboolean         bobgui_cell_renderer_get_visible    (BobguiCellRenderer      *cell);

GDK_DEPRECATED_IN_4_10
void             bobgui_cell_renderer_set_sensitive  (BobguiCellRenderer      *cell,
                                                   gboolean              sensitive);
GDK_DEPRECATED_IN_4_10
gboolean         bobgui_cell_renderer_get_sensitive  (BobguiCellRenderer      *cell);

GDK_DEPRECATED_IN_4_10
gboolean         bobgui_cell_renderer_is_activatable (BobguiCellRenderer      *cell);

GDK_DEPRECATED_IN_4_10
void             bobgui_cell_renderer_set_is_expander (BobguiCellRenderer     *cell,
                                                    gboolean             is_expander);
GDK_DEPRECATED_IN_4_10
gboolean         bobgui_cell_renderer_get_is_expander (BobguiCellRenderer     *cell);

GDK_DEPRECATED_IN_4_10
void             bobgui_cell_renderer_set_is_expanded (BobguiCellRenderer     *cell,
                                                    gboolean             is_expanded);
GDK_DEPRECATED_IN_4_10
gboolean         bobgui_cell_renderer_get_is_expanded (BobguiCellRenderer     *cell);




/* For use by cell renderer implementations only */
GDK_DEPRECATED_IN_4_10
void             bobgui_cell_renderer_stop_editing   (BobguiCellRenderer      *cell,
                                                   gboolean              canceled);


void            _bobgui_cell_renderer_calc_offset    (BobguiCellRenderer      *cell,
                                                   const GdkRectangle   *cell_area,
                                                   BobguiTextDirection      direction,
                                                   int                   width,
                                                   int                   height,
                                                   int                  *x_offset,
                                                   int                  *y_offset);

GDK_DEPRECATED_IN_4_10
BobguiStateFlags   bobgui_cell_renderer_get_state       (BobguiCellRenderer      *cell,
                                                   BobguiWidget            *widget,
                                                   BobguiCellRendererState  cell_state);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiCellRenderer, g_object_unref)

G_END_DECLS

