/* BOBGUI - The Bobgui Framework
 * bobguitextbtree.h Copyright (C) 2000 Red Hat, Inc.
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

/*
 * Modified by the BOBGUI Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the BOBGUI Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BOBGUI at ftp://ftp.bobgui.org/pub/bobgui/.
 */

#pragma once

#if 0
#define DEBUG_VALIDATION_AND_SCROLLING
#endif

#ifdef DEBUG_VALIDATION_AND_SCROLLING
#define DV(x) (x)
#else
#define DV(x)
#endif

#include <bobgui/bobguitextbuffer.h>
#include <bobgui/bobguitexttag.h>
#include <bobgui/bobguitextmark.h>
#include <bobgui/bobguitextchild.h>
#include <bobgui/bobguitextsegmentprivate.h>
#include <bobgui/bobguitextiter.h>

G_BEGIN_DECLS

BobguiTextBTree  *_bobgui_text_btree_new        (BobguiTextTagTable *table,
                                           BobguiTextBuffer   *buffer);
void           _bobgui_text_btree_ref        (BobguiTextBTree    *tree);
void           _bobgui_text_btree_unref      (BobguiTextBTree    *tree);
BobguiTextBuffer *_bobgui_text_btree_get_buffer (BobguiTextBTree    *tree);


guint _bobgui_text_btree_get_chars_changed_stamp    (BobguiTextBTree *tree);
guint _bobgui_text_btree_get_segments_changed_stamp (BobguiTextBTree *tree);
void  _bobgui_text_btree_segments_changed           (BobguiTextBTree *tree);

gboolean _bobgui_text_btree_is_end (BobguiTextBTree       *tree,
                                 BobguiTextLine        *line,
                                 BobguiTextLineSegment *seg,
                                 int                 byte_index,
                                 int                 char_offset);

/* Indexable segment mutation */

void _bobgui_text_btree_delete           (BobguiTextIter  *start,
                                       BobguiTextIter  *end);
void _bobgui_text_btree_insert           (BobguiTextIter  *iter,
                                       const char   *text,
                                       int           len);
void _bobgui_text_btree_insert_paintable (BobguiTextIter  *iter,
                                       GdkPaintable *texture);

void _bobgui_text_btree_insert_child_anchor (BobguiTextIter        *iter,
                                          BobguiTextChildAnchor *anchor);

void _bobgui_text_btree_unregister_child_anchor (BobguiTextChildAnchor *anchor);

/* View stuff */
BobguiTextLine *_bobgui_text_btree_find_line_by_y    (BobguiTextBTree      *tree,
                                                gpointer           view_id,
                                                int                ypixel,
                                                int               *line_top_y);
int          _bobgui_text_btree_find_line_top     (BobguiTextBTree      *tree,
                                                BobguiTextLine       *line,
                                                gpointer           view_id);
void         _bobgui_text_btree_add_view          (BobguiTextBTree      *tree,
                                                BobguiTextLayout     *layout);
void         _bobgui_text_btree_remove_view       (BobguiTextBTree      *tree,
                                                gpointer           view_id);
void         _bobgui_text_btree_invalidate_region (BobguiTextBTree      *tree,
                                                const BobguiTextIter *start,
                                                const BobguiTextIter *end,
                                                gboolean           cursors_only);
void         _bobgui_text_btree_get_view_size     (BobguiTextBTree      *tree,
                                                gpointer           view_id,
                                                int               *width,
                                                int               *height);
gboolean     _bobgui_text_btree_is_valid          (BobguiTextBTree      *tree,
                                                gpointer           view_id);
gboolean     _bobgui_text_btree_validate          (BobguiTextBTree      *tree,
                                                gpointer           view_id,
                                                int                max_pixels,
                                                int               *y,
                                                int               *old_height,
                                                int               *new_height);
void         _bobgui_text_btree_validate_line     (BobguiTextBTree      *tree,
                                                BobguiTextLine       *line,
                                                gpointer           view_id);

/* Tag */

void _bobgui_text_btree_tag (const BobguiTextIter *start,
                          const BobguiTextIter *end,
                          BobguiTextTag        *tag,
                          gboolean           apply);

/* "Getters" */

BobguiTextLine * _bobgui_text_btree_get_line          (BobguiTextBTree      *tree,
                                                 int                line_number,
                                                 int               *real_line_number);
BobguiTextLine * _bobgui_text_btree_get_line_no_last  (BobguiTextBTree      *tree,
                                                 int                line_number,
                                                 int               *real_line_number);
BobguiTextLine * _bobgui_text_btree_get_end_iter_line (BobguiTextBTree      *tree);
BobguiTextLine * _bobgui_text_btree_get_line_at_char  (BobguiTextBTree      *tree,
                                                 int                char_index,
                                                 int               *line_start_index,
                                                 int               *real_char_index);
GPtrArray    * _bobgui_text_btree_get_tags          (const BobguiTextIter *iter);
char         *_bobgui_text_btree_get_text          (const BobguiTextIter *start,
                                                 const BobguiTextIter *end,
                                                 gboolean           include_hidden,
                                                 gboolean           include_nonchars);
int           _bobgui_text_btree_line_count        (BobguiTextBTree      *tree);
int           _bobgui_text_btree_char_count        (BobguiTextBTree      *tree);
gboolean      _bobgui_text_btree_char_is_invisible (const BobguiTextIter *iter);



/* Get iterators (these are implemented in bobguitextiter.c) */
void     _bobgui_text_btree_get_iter_at_char         (BobguiTextBTree       *tree,
                                                   BobguiTextIter        *iter,
                                                   int                 char_index);
void     _bobgui_text_btree_get_iter_at_line_char    (BobguiTextBTree       *tree,
                                                   BobguiTextIter        *iter,
                                                   int                 line_number,
                                                   int                 char_index);
void     _bobgui_text_btree_get_iter_at_line_byte    (BobguiTextBTree       *tree,
                                                   BobguiTextIter        *iter,
                                                   int                 line_number,
                                                   int                 byte_index);
gboolean _bobgui_text_btree_get_iter_from_string     (BobguiTextBTree       *tree,
                                                   BobguiTextIter        *iter,
                                                   const char         *string);
gboolean _bobgui_text_btree_get_iter_at_mark_name    (BobguiTextBTree       *tree,
                                                   BobguiTextIter        *iter,
                                                   const char         *mark_name);
void     _bobgui_text_btree_get_iter_at_mark         (BobguiTextBTree       *tree,
                                                   BobguiTextIter        *iter,
                                                   BobguiTextMark        *mark);
void     _bobgui_text_btree_get_iter_at_paintable    (BobguiTextBTree       *tree,
                                                   BobguiTextIter        *iter,
                                                   BobguiTextLineSegment *seg);
void     _bobgui_text_btree_get_end_iter             (BobguiTextBTree       *tree,
                                                   BobguiTextIter        *iter);
void     _bobgui_text_btree_get_iter_at_line         (BobguiTextBTree       *tree,
                                                   BobguiTextIter        *iter,
                                                   BobguiTextLine        *line,
                                                   int                 byte_offset);
void     _bobgui_text_btree_get_iter_at_line_ptr_char (BobguiTextBTree       *tree,
                                                    BobguiTextIter        *iter,
                                                    BobguiTextLine        *line,
                                                    int                 char_offset);
gboolean _bobgui_text_btree_get_iter_at_first_toggle (BobguiTextBTree       *tree,
                                                   BobguiTextIter        *iter,
                                                   BobguiTextTag         *tag);
gboolean _bobgui_text_btree_get_iter_at_last_toggle  (BobguiTextBTree       *tree,
                                                   BobguiTextIter        *iter,
                                                   BobguiTextTag         *tag);

void     _bobgui_text_btree_get_iter_at_child_anchor  (BobguiTextBTree       *tree,
                                                    BobguiTextIter        *iter,
                                                    BobguiTextChildAnchor *anchor);



/* Manipulate marks */
BobguiTextMark        *_bobgui_text_btree_set_mark                (BobguiTextBTree       *tree,
                                                             BobguiTextMark         *existing_mark,
                                                             const char         *name,
                                                             gboolean            left_gravity,
                                                             const BobguiTextIter  *index,
                                                             gboolean           should_exist);
void                _bobgui_text_btree_remove_mark_by_name     (BobguiTextBTree       *tree,
                                                             const char         *name);
void                _bobgui_text_btree_remove_mark             (BobguiTextBTree       *tree,
                                                             BobguiTextMark        *segment);
gboolean            _bobgui_text_btree_get_selection_bounds    (BobguiTextBTree       *tree,
                                                             BobguiTextIter        *start,
                                                             BobguiTextIter        *end);
void                _bobgui_text_btree_place_cursor            (BobguiTextBTree       *tree,
                                                             const BobguiTextIter  *where);
void                _bobgui_text_btree_select_range            (BobguiTextBTree       *tree,
                                                             const BobguiTextIter  *ins,
							     const BobguiTextIter  *bound);
gboolean            _bobgui_text_btree_mark_is_insert          (BobguiTextBTree       *tree,
                                                             BobguiTextMark        *segment);
gboolean            _bobgui_text_btree_mark_is_selection_bound (BobguiTextBTree       *tree,
                                                             BobguiTextMark        *segment);
BobguiTextMark        *_bobgui_text_btree_get_insert		    (BobguiTextBTree       *tree);
BobguiTextMark        *_bobgui_text_btree_get_selection_bound	    (BobguiTextBTree       *tree);
BobguiTextMark        *_bobgui_text_btree_get_mark_by_name        (BobguiTextBTree       *tree,
                                                             const char         *name);
BobguiTextLine *       _bobgui_text_btree_first_could_contain_tag (BobguiTextBTree       *tree,
                                                             BobguiTextTag         *tag);
BobguiTextLine *       _bobgui_text_btree_last_could_contain_tag  (BobguiTextBTree       *tree,
                                                             BobguiTextTag         *tag);

/* Lines */

/* Chunk of data associated with a line; views can use this to store
   info at the line. They should "subclass" the header struct here. */
struct _BobguiTextLineData {
  gpointer view_id;
  BobguiTextLineData *next;
  int height;
  int top_ink : 16;
  int bottom_ink : 16;
  signed int width : 24;
  guint valid : 8;		/* Actually a boolean */
};

/*
 * The data structure below defines a single line of text (from newline
 * to newline, not necessarily what appears on one line of the screen).
 *
 * You can consider this line a "paragraph" also
 */

struct _BobguiTextLine {
  BobguiTextBTreeNode *parent;             /* Pointer to parent node containing
                                         * line. */
  BobguiTextLine *next;            /* Next in linked list of lines with
                                 * same parent node in B-tree.  NULL
                                 * means end of list. */
  BobguiTextLineSegment *segments; /* First in ordered list of segments
                                 * that make up the line. */
  BobguiTextLineData *views;      /* data stored here by views */
  guchar dir_strong;                /* BiDi algo dir of line */
  guchar dir_propagated_back;       /* BiDi algo dir of next line */
  guchar dir_propagated_forward;    /* BiDi algo dir of prev line */
};


int                 _bobgui_text_line_get_number                 (BobguiTextLine         *line);
gboolean            _bobgui_text_line_char_has_tag               (BobguiTextLine         *line,
                                                               BobguiTextBTree        *tree,
                                                               int                  char_in_line,
                                                               BobguiTextTag          *tag);
gboolean            _bobgui_text_line_byte_has_tag               (BobguiTextLine         *line,
                                                               BobguiTextBTree        *tree,
                                                               int                  byte_in_line,
                                                               BobguiTextTag          *tag);
gboolean            _bobgui_text_line_is_last                    (BobguiTextLine         *line,
                                                               BobguiTextBTree        *tree);
gboolean            _bobgui_text_line_contains_end_iter          (BobguiTextLine         *line,
                                                               BobguiTextBTree        *tree);
BobguiTextLine *       _bobgui_text_line_next                       (BobguiTextLine         *line);
BobguiTextLine *       _bobgui_text_line_next_excluding_last        (BobguiTextLine         *line);
BobguiTextLine *       _bobgui_text_line_previous                   (BobguiTextLine         *line);
void                _bobgui_text_line_add_data                   (BobguiTextLine         *line,
                                                               BobguiTextLineData     *data);
gpointer            _bobgui_text_line_remove_data                (BobguiTextLine         *line,
                                                               gpointer             view_id);
gpointer            _bobgui_text_line_get_data                   (BobguiTextLine         *line,
                                                               gpointer             view_id);
void                _bobgui_text_line_invalidate_wrap            (BobguiTextLine         *line,
                                                               BobguiTextLineData     *ld);
int                 _bobgui_text_line_char_count                 (BobguiTextLine         *line);
int                 _bobgui_text_line_byte_count                 (BobguiTextLine         *line);
int                 _bobgui_text_line_char_index                 (BobguiTextLine         *line);
BobguiTextLineSegment *_bobgui_text_line_byte_to_segment            (BobguiTextLine         *line,
                                                               int                  byte_offset,
                                                               int                 *seg_offset);
BobguiTextLineSegment *_bobgui_text_line_char_to_segment            (BobguiTextLine         *line,
                                                               int                  char_offset,
                                                               int                 *seg_offset);
gboolean            _bobgui_text_line_byte_locate                (BobguiTextLine         *line,
                                                               int                  byte_offset,
                                                               BobguiTextLineSegment **segment,
                                                               BobguiTextLineSegment **any_segment,
                                                               int                 *seg_byte_offset,
                                                               int                 *line_byte_offset);
gboolean            _bobgui_text_line_char_locate                (BobguiTextLine         *line,
                                                               int                  char_offset,
                                                               BobguiTextLineSegment **segment,
                                                               BobguiTextLineSegment **any_segment,
                                                               int                 *seg_char_offset,
                                                               int                 *line_char_offset);
void                _bobgui_text_line_byte_to_char_offsets       (BobguiTextLine         *line,
                                                               int                  byte_offset,
                                                               int                 *line_char_offset,
                                                               int                 *seg_char_offset);
void                _bobgui_text_line_char_to_byte_offsets       (BobguiTextLine         *line,
                                                               int                  char_offset,
                                                               int                 *line_byte_offset,
                                                               int                 *seg_byte_offset);
BobguiTextLineSegment *_bobgui_text_line_byte_to_any_segment        (BobguiTextLine         *line,
                                                               int                  byte_offset,
                                                               int                 *seg_offset);
BobguiTextLineSegment *_bobgui_text_line_char_to_any_segment        (BobguiTextLine         *line,
                                                               int                  char_offset,
                                                               int                 *seg_offset);
int                 _bobgui_text_line_byte_to_char               (BobguiTextLine         *line,
                                                               int                  byte_offset);
int                 _bobgui_text_line_char_to_byte               (BobguiTextLine         *line,
                                                               int                  char_offset);
BobguiTextLine    *    _bobgui_text_line_next_could_contain_tag     (BobguiTextLine         *line,
                                                               BobguiTextBTree        *tree,
                                                               BobguiTextTag          *tag);
BobguiTextLine    *    _bobgui_text_line_previous_could_contain_tag (BobguiTextLine         *line,
                                                               BobguiTextBTree        *tree,
                                                               BobguiTextTag          *tag);

BobguiTextLineData    *_bobgui_text_line_data_new                   (BobguiTextLayout     *layout,
                                                               BobguiTextLine       *line);

/* Debug */
void _bobgui_text_btree_check (BobguiTextBTree *tree);
void _bobgui_text_btree_spew (BobguiTextBTree *tree);
extern gboolean _bobgui_text_view_debug_btree;

/* ignore, exported only for bobguitextsegment.c */
void _bobgui_toggle_segment_check_func (BobguiTextLineSegment *segPtr,
                                     BobguiTextLine        *line);
void _bobgui_change_node_toggle_count  (BobguiTextBTreeNode   *node,
                                     BobguiTextTagInfo     *info,
                                     int                 delta);

/* for bobguitextmark.c */
void _bobgui_text_btree_release_mark_segment (BobguiTextBTree       *tree,
                                           BobguiTextLineSegment *segment);

/* for coordination with the tag table */
void _bobgui_text_btree_notify_will_remove_tag (BobguiTextBTree *tree,
                                             BobguiTextTag   *tag);

G_END_DECLS



