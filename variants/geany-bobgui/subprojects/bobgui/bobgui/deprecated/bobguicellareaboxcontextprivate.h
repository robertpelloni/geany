/* bobguicellareaboxcontext.h
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

#include <bobgui/deprecated/bobguicellareacontext.h>
#include <bobgui/deprecated/bobguicellareabox.h>
#include <bobgui/deprecated/bobguicellrenderer.h>
#include <bobgui/bobguisizerequest.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_CELL_AREA_BOX_CONTEXT            (_bobgui_cell_area_box_context_get_type ())
#define BOBGUI_CELL_AREA_BOX_CONTEXT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_CELL_AREA_BOX_CONTEXT, BobguiCellAreaBoxContext))
#define BOBGUI_CELL_AREA_BOX_CONTEXT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BOBGUI_TYPE_CELL_AREA_BOX_CONTEXT, BobguiCellAreaBoxContextClass))
#define BOBGUI_IS_CELL_AREA_BOX_CONTEXT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_CELL_AREA_BOX_CONTEXT))
#define BOBGUI_IS_CELL_AREA_BOX_CONTEXT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BOBGUI_TYPE_CELL_AREA_BOX_CONTEXT))
#define BOBGUI_CELL_AREA_BOX_CONTEXT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_CELL_AREA_BOX_CONTEXT, BobguiCellAreaBoxContextClass))

typedef struct _BobguiCellAreaBoxContext              BobguiCellAreaBoxContext;
typedef struct _BobguiCellAreaBoxContextClass         BobguiCellAreaBoxContextClass;
typedef struct _BobguiCellAreaBoxContextPrivate       BobguiCellAreaBoxContextPrivate;

struct _BobguiCellAreaBoxContext
{
  BobguiCellAreaContext parent_instance;

  BobguiCellAreaBoxContextPrivate *priv;
};

struct _BobguiCellAreaBoxContextClass
{
  BobguiCellAreaContextClass parent_class;

};

GType   _bobgui_cell_area_box_context_get_type                     (void) G_GNUC_CONST;


/* Create a duplicate of the context */
BobguiCellAreaBoxContext *_bobgui_cell_area_box_context_copy          (BobguiCellAreaBox        *box,
                                                                BobguiCellAreaBoxContext *box_context);

/* Initialize group array dimensions */
void    _bobgui_cell_area_box_init_groups                         (BobguiCellAreaBoxContext *box_context,
                                                                guint                  n_groups,
                                                                gboolean              *expand_groups,
                                                                gboolean              *align_groups);

/* Update cell-group sizes */
void    _bobgui_cell_area_box_context_push_group_width             (BobguiCellAreaBoxContext *box_context,
                                                                int                    group_idx,
                                                                int                    minimum_width,
                                                                int                    natural_width);

void    _bobgui_cell_area_box_context_push_group_height_for_width  (BobguiCellAreaBoxContext *box_context,
                                                                int                    group_idx,
                                                                int                    for_width,
                                                                int                    minimum_height,
                                                                int                    natural_height);

void    _bobgui_cell_area_box_context_push_group_height            (BobguiCellAreaBoxContext *box_context,
                                                                int                    group_idx,
                                                                int                    minimum_height,
                                                                int                    natural_height);

void    _bobgui_cell_area_box_context_push_group_width_for_height  (BobguiCellAreaBoxContext *box_context,
                                                                int                    group_idx,
                                                                int                    for_height,
                                                                int                    minimum_width,
                                                                int                    natural_width);

/* Fetch cell-group sizes */
void    _bobgui_cell_area_box_context_get_group_width              (BobguiCellAreaBoxContext *box_context,
                                                                int                    group_idx,
                                                                int                   *minimum_width,
                                                                int                   *natural_width);

void    _bobgui_cell_area_box_context_get_group_height_for_width   (BobguiCellAreaBoxContext *box_context,
                                                                int                    group_idx,
                                                                int                    for_width,
                                                                int                   *minimum_height,
                                                                int                   *natural_height);

void    _bobgui_cell_area_box_context_get_group_height             (BobguiCellAreaBoxContext *box_context,
                                                                int                    group_idx,
                                                                int                   *minimum_height,
                                                                int                   *natural_height);

void    _bobgui_cell_area_box_context_get_group_width_for_height   (BobguiCellAreaBoxContext *box_context,
                                                                int                    group_idx,
                                                                int                    for_height,
                                                                int                   *minimum_width,
                                                                int                   *natural_width);

BobguiRequestedSize *_bobgui_cell_area_box_context_get_widths         (BobguiCellAreaBoxContext *box_context,
                                                                int                   *n_widths);
BobguiRequestedSize *_bobgui_cell_area_box_context_get_heights        (BobguiCellAreaBoxContext *box_context,
                                                                int                   *n_heights);

/* Private context/area interaction */
typedef struct {
  int group_idx; /* Groups containing only invisible cells are not allocated */
  int position;  /* Relative group allocation position in the orientation of the box */
  int size;      /* Full allocated size of the cells in this group spacing inclusive */
} BobguiCellAreaBoxAllocation;

BobguiCellAreaBoxAllocation *
_bobgui_cell_area_box_context_get_orientation_allocs (BobguiCellAreaBoxContext *context,
                                                  int                   *n_allocs);

G_END_DECLS

