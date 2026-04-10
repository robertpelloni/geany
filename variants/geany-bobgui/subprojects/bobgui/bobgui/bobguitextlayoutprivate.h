/* BOBGUI - The Bobgui Framework
 * bobguitextlayout.h
 *
 * Copyright (c) 1992-1994 The Regents of the University of California.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 * Copyright (c) 2000 Red Hat, Inc.
 * Tk->Bobgui port by Havoc Pennington
 * Pango support by Owen Taylor
 *
 * This file can be used under your choice of two licenses, the LGPL
 * and the original Tk license.
 *
 * LGPL:
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
 *
 * Original Tk license:
 *
 * This software is copyrighted by the Regents of the University of
 * California, Sun Microsystems, Inc., and other parties.  The
 * following terms apply to all files associated with the software
 * unless explicitly disclaimed in individual files.
 *
 * The authors hereby grant permission to use, copy, modify,
 * distribute, and license this software and its documentation for any
 * purpose, provided that existing copyright notices are retained in
 * all copies and that this notice is included verbatim in any
 * distributions. No written agreement, license, or royalty fee is
 * required for any of the authorized uses.  Modifications to this
 * software may be copyrighted by their authors and need not follow
 * the licensing terms described here, provided that the new terms are
 * clearly indicated on the first page of each file where they apply.
 *
 * IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY
 * PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
 * DAMAGES ARISING OUT OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION,
 * OR ANY DERIVATIVES THEREOF, EVEN IF THE AUTHORS HAVE BEEN ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND
 * NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS,
 * AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
 * MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 *
 * GOVERNMENT USE: If you are acquiring this software on behalf of the
 * U.S. government, the Government shall have only "Restricted Rights"
 * in the software and related documentation as defined in the Federal
 * Acquisition Regulations (FARs) in Clause 52.227.19 (c) (2).  If you
 * are acquiring the software on behalf of the Department of Defense,
 * the software shall be classified as "Commercial Computer Software"
 * and the Government shall have only "Restricted Rights" as defined
 * in Clause 252.227-7013 (c) (1) of DFARs.  Notwithstanding the
 * foregoing, the authors grant the U.S. Government and others acting
 * in its behalf permission to use and distribute the software in
 * accordance with the terms specified in this license.
 *
 */
/*
 * Modified by the BOBGUI Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the BOBGUI Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BOBGUI at ftp://ftp.bobgui.org/pub/bobgui/.
 */

#pragma once

#include <bobgui/bobgui.h>
#include <bobgui/bobguitextattributesprivate.h>

G_BEGIN_DECLS

/* forward declarations that have to be here to avoid including
 * bobguitextbtree.h
 */
typedef struct _BobguiTextLine     BobguiTextLine;
typedef struct _BobguiTextLineData BobguiTextLineData;

#define BOBGUI_TYPE_TEXT_LAYOUT             (bobgui_text_layout_get_type ())
#define BOBGUI_TEXT_LAYOUT(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_TEXT_LAYOUT, BobguiTextLayout))
#define BOBGUI_TEXT_LAYOUT_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BOBGUI_TYPE_TEXT_LAYOUT, BobguiTextLayoutClass))
#define BOBGUI_IS_TEXT_LAYOUT(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_TEXT_LAYOUT))
#define BOBGUI_IS_TEXT_LAYOUT_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), BOBGUI_TYPE_TEXT_LAYOUT))
#define BOBGUI_TEXT_LAYOUT_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_TEXT_LAYOUT, BobguiTextLayoutClass))

typedef struct _BobguiTextLayout         BobguiTextLayout;
typedef struct _BobguiTextLayoutClass    BobguiTextLayoutClass;
typedef struct _BobguiTextLineDisplay    BobguiTextLineDisplay;
typedef struct _BobguiTextAttrAppearance BobguiTextAttrAppearance;

struct _BobguiTextLayout
{
  GObject parent_instance;

  /* width of the display area on-screen,
   * i.e. pixels we should wrap to fit inside. */
  int screen_width;

  /* width/height of the total logical area being laid out */
  int width;
  int height;

  int left_padding;
  int right_padding;

  BobguiTextBuffer *buffer;

  /* Default style used if no tags override it */
  BobguiTextAttributes *default_style;

  /* Pango contexts used for creating layouts */
  PangoContext *ltr_context;
  PangoContext *rtl_context;

  /* Whether to show the insertion cursor */
  guint cursor_visible : 1;

  /* For what BobguiTextDirection to draw cursor BOBGUI_TEXT_DIR_NONE -
   * means draw both cursors.
   */
  guint cursor_direction : 2;

  /* The default direction is used for alignment when
     there are no strong characters.
  */
  guint default_direction : 2;

  guint overwrite_mode : 1;

  /* The preedit string and attributes, if any */

  char *preedit_string;
  PangoAttrList *preedit_attrs;
  int preedit_len;
  int preedit_cursor;
};

struct _BobguiTextLayoutClass
{
  GObjectClass parent_class;
};

struct _BobguiTextAttrAppearance
{
  PangoAttribute attr;
  BobguiTextAppearance appearance;
};

typedef struct _CursorPosition CursorPosition;
struct _CursorPosition {
  int pos;
  guint is_insert          : 1;
  guint is_selection_bound : 1;
};

struct _BobguiTextLineDisplay
{
  PangoLayout *layout;

  GskRenderNode *node;

  GArray *cursors;      /* indexes of cursors in the PangoLayout, and mark names */

  /* GSequenceIter backpointer for use within cache */
  GSequenceIter *cache_iter;

  /* GQueue link for use in MRU to help cull cache */
  GList          mru_link;

  BobguiTextDirection direction;

  int width;                   /* Width of layout */
  int total_width;             /* width - margins, if no width set on layout, if width set on layout, -1 */
  int height;
  /* Amount layout is shifted from left edge - this is the left margin
   * plus any other factors, such as alignment or indentation.
   */
  int x_offset;
  int left_margin;
  int right_margin;
  int top_margin;
  int bottom_margin;
  int insert_index;		/* Byte index of insert cursor within para or -1 */

  BobguiTextLine *line;

  GdkRectangle block_cursor;

  guint cursors_invalid : 1;
  guint has_block_cursor : 1;
  guint cursor_at_line_end : 1;
  guint size_only : 1;
  guint pg_bg_rgba_set : 1;
  guint has_children : 1;

  GdkRGBA pg_bg_rgba;
};

#ifdef BOBGUI_COMPILATION
extern G_GNUC_INTERNAL PangoAttrType bobgui_text_attr_appearance_type;
#endif

GType         bobgui_text_layout_get_type    (void) G_GNUC_CONST;

BobguiTextLayout*     bobgui_text_layout_new                   (void);
void               bobgui_text_layout_set_buffer            (BobguiTextLayout     *layout,
							  BobguiTextBuffer     *buffer);
BobguiTextBuffer     *bobgui_text_layout_get_buffer            (BobguiTextLayout     *layout);
void               bobgui_text_layout_set_default_style     (BobguiTextLayout     *layout,
							  BobguiTextAttributes *values);
void               bobgui_text_layout_set_contexts          (BobguiTextLayout     *layout,
							  PangoContext      *ltr_context,
							  PangoContext      *rtl_context);
void               bobgui_text_layout_set_cursor_direction  (BobguiTextLayout     *layout,
                                                          BobguiTextDirection   direction);
void		   bobgui_text_layout_set_overwrite_mode	 (BobguiTextLayout     *layout,
							  gboolean           overwrite);
void               bobgui_text_layout_set_default_direction (BobguiTextLayout     *layout,
							   BobguiTextDirection default_dir);
void               bobgui_text_layout_default_style_changed (BobguiTextLayout     *layout);

void bobgui_text_layout_set_screen_width       (BobguiTextLayout     *layout,
                                             int                width);
void bobgui_text_layout_set_preedit_string     (BobguiTextLayout     *layout,
 					     const char        *preedit_string,
 					     PangoAttrList     *preedit_attrs,
 					     int                cursor_pos);

void     bobgui_text_layout_set_cursor_visible (BobguiTextLayout     *layout,
                                             gboolean           cursor_visible);
gboolean bobgui_text_layout_get_cursor_visible (BobguiTextLayout     *layout);

void    bobgui_text_layout_get_size  (BobguiTextLayout  *layout,
                                   int            *width,
                                   int            *height);

void bobgui_text_layout_wrap_loop_start (BobguiTextLayout *layout);
void bobgui_text_layout_wrap_loop_end   (BobguiTextLayout *layout);

BobguiTextLineDisplay* bobgui_text_layout_get_line_display  (BobguiTextLayout      *layout,
                                                       BobguiTextLine        *line,
                                                       gboolean            size_only);

BobguiTextLineDisplay *bobgui_text_line_display_ref         (BobguiTextLineDisplay       *display);
void                bobgui_text_line_display_unref       (BobguiTextLineDisplay       *display);
int                 bobgui_text_line_display_compare     (const BobguiTextLineDisplay *display1,
                                                       const BobguiTextLineDisplay *display2,
                                                       BobguiTextLayout            *layout);

void bobgui_text_layout_get_line_at_y     (BobguiTextLayout     *layout,
                                        BobguiTextIter       *target_iter,
                                        int                y,
                                        int               *line_top);
gboolean bobgui_text_layout_get_iter_at_pixel (BobguiTextLayout     *layout,
                                            BobguiTextIter       *iter,
                                            int                x,
                                            int                y);
gboolean bobgui_text_layout_get_iter_at_position (BobguiTextLayout     *layout,
                                               BobguiTextIter       *iter,
                                               int               *trailing,
                                               int                x,
                                               int                y);
void bobgui_text_layout_invalidate        (BobguiTextLayout     *layout,
                                        const BobguiTextIter *start,
                                        const BobguiTextIter *end);
void bobgui_text_layout_invalidate_cursors(BobguiTextLayout     *layout,
                                        const BobguiTextIter *start,
                                        const BobguiTextIter *end);
void bobgui_text_layout_invalidate_selection (BobguiTextLayout  *layout);
void bobgui_text_layout_free_line_data    (BobguiTextLayout     *layout,
                                        BobguiTextLine       *line,
                                        BobguiTextLineData   *line_data);

gboolean bobgui_text_layout_is_valid        (BobguiTextLayout *layout);
void     bobgui_text_layout_validate_yrange (BobguiTextLayout *layout,
                                          BobguiTextIter   *anchor_line,
                                          int            y0_,
                                          int            y1_);
void     bobgui_text_layout_validate        (BobguiTextLayout *layout,
                                          int            max_pixels);

BobguiTextLineData* bobgui_text_layout_wrap  (BobguiTextLayout   *layout,
                                        BobguiTextLine     *line,
                                        BobguiTextLineData *line_data);
void     bobgui_text_layout_changed              (BobguiTextLayout     *layout,
                                               int                y,
                                               int                old_height,
                                               int                new_height);
void     bobgui_text_layout_cursors_changed      (BobguiTextLayout     *layout,
                                               int                y,
                                               int                old_height,
                                               int                new_height);
void     bobgui_text_layout_get_iter_location    (BobguiTextLayout     *layout,
                                               const BobguiTextIter *iter,
                                               GdkRectangle      *rect);
void     bobgui_text_layout_get_line_yrange      (BobguiTextLayout     *layout,
                                               const BobguiTextIter *iter,
                                               int               *y,
                                               int               *height);
void     bobgui_text_layout_get_cursor_locations (BobguiTextLayout     *layout,
                                               BobguiTextIter       *iter,
                                               GdkRectangle      *strong_pos,
                                               GdkRectangle      *weak_pos);
BobguiTextLineDisplay *bobgui_text_layout_create_display (BobguiTextLayout *layout,
                                                    BobguiTextLine   *line,
                                                    gboolean       size_only);
void     bobgui_text_layout_update_display_cursors (BobguiTextLayout      *layout,
                                                 BobguiTextLine        *line,
                                                 BobguiTextLineDisplay *display);
void     bobgui_text_layout_update_children        (BobguiTextLayout      *layout,
                                                 BobguiTextLineDisplay *display);
gboolean _bobgui_text_layout_get_block_cursor    (BobguiTextLayout     *layout,
					       GdkRectangle      *pos);
gboolean bobgui_text_layout_clamp_iter_to_vrange (BobguiTextLayout     *layout,
                                               BobguiTextIter       *iter,
                                               int                top,
                                               int                bottom);

gboolean bobgui_text_layout_move_iter_to_line_end      (BobguiTextLayout *layout,
                                                     BobguiTextIter   *iter,
                                                     int            direction);
gboolean bobgui_text_layout_move_iter_to_previous_line (BobguiTextLayout *layout,
                                                     BobguiTextIter   *iter);
gboolean bobgui_text_layout_move_iter_to_next_line     (BobguiTextLayout *layout,
                                                     BobguiTextIter   *iter);
void     bobgui_text_layout_move_iter_to_x             (BobguiTextLayout *layout,
                                                     BobguiTextIter   *iter,
                                                     int            x);
gboolean bobgui_text_layout_move_iter_visually         (BobguiTextLayout *layout,
                                                     BobguiTextIter   *iter,
                                                     int            count);

gboolean bobgui_text_layout_iter_starts_line           (BobguiTextLayout       *layout,
                                                     const BobguiTextIter   *iter);

void     bobgui_text_layout_get_iter_at_line           (BobguiTextLayout *layout,
                                                     BobguiTextIter    *iter,
                                                     BobguiTextLine    *line,
                                                     int             byte_offset);

void bobgui_text_child_anchor_register_child   (BobguiTextChildAnchor *anchor,
                                             BobguiWidget          *child,
                                             BobguiTextLayout      *layout);
void bobgui_text_child_anchor_unregister_child (BobguiTextChildAnchor *anchor,
                                             BobguiWidget          *child);

void bobgui_text_child_anchor_queue_resize     (BobguiTextChildAnchor *anchor,
                                             BobguiTextLayout      *layout);

void bobgui_text_anchored_child_set_layout     (BobguiWidget          *child,
                                             BobguiTextLayout      *layout);

void bobgui_text_layout_spew (BobguiTextLayout *layout);

void bobgui_text_layout_snapshot (BobguiTextLayout         *layout,
                               BobguiWidget             *widget,
                               BobguiSnapshot           *snapshot,
                               const graphene_rect_t *clip,
                               gboolean               selection_style_changed,
                               float                  cursor_alpha);

void bobgui_text_layout_set_mru_size (BobguiTextLayout *layout,
                                   guint          mru_size);

G_END_DECLS

