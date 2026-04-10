/* BOBGUI - The Bobgui Framework
 * bobguitextiter.h Copyright (C) 2000 Red Hat, Inc.
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

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <bobgui/bobguitextchild.h>
#include <bobgui/bobguitexttag.h>

G_BEGIN_DECLS

/**
 * BobguiTextSearchFlags:
 * @BOBGUI_TEXT_SEARCH_VISIBLE_ONLY: Search only visible data. A search match may
 * have invisible text interspersed.
 * @BOBGUI_TEXT_SEARCH_TEXT_ONLY: Search only text. A match may have paintables or
 * child widgets mixed inside the matched range.
 * @BOBGUI_TEXT_SEARCH_CASE_INSENSITIVE: The text will be matched regardless of
 * what case it is in.
 *
 * Flags affecting how a search is done.
 *
 * If neither `BOBGUI_TEXT_SEARCH_VISIBLE_ONLY` nor `BOBGUI_TEXT_SEARCH_TEXT_ONLY`
 * are enabled, the match must be exact; the special 0xFFFC character will
 * match embedded paintables or child widgets.
 */
typedef enum {
  BOBGUI_TEXT_SEARCH_VISIBLE_ONLY     = 1 << 0,
  BOBGUI_TEXT_SEARCH_TEXT_ONLY        = 1 << 1,
  BOBGUI_TEXT_SEARCH_CASE_INSENSITIVE = 1 << 2
  /* Possible future plans: SEARCH_REGEXP */
} BobguiTextSearchFlags;

/*
 * Iter: represents a location in the text. Becomes invalid if the
 * characters/pixmaps/widgets (indexable objects) in the text buffer
 * are changed.
 */

typedef struct _BobguiTextBuffer BobguiTextBuffer;

#define BOBGUI_TYPE_TEXT_ITER     (bobgui_text_iter_get_type ())

struct _BobguiTextIter {
  /* BobguiTextIter is an opaque datatype; ignore all these fields.
   * Initialize the iter with bobgui_text_buffer_get_iter_*
   * functions
   */
  /*< private >*/
  gpointer dummy1;
  gpointer dummy2;
  int dummy3;
  int dummy4;
  int dummy5;
  int dummy6;
  int dummy7;
  int dummy8;
  gpointer dummy9;
  gpointer dummy10;
  int dummy11;
  int dummy12;
  /* padding */
  int dummy13;
  gpointer dummy14;
};


/* This is primarily intended for language bindings that want to avoid
   a "buffer" argument to text insertions, deletions, etc. */
GDK_AVAILABLE_IN_ALL
BobguiTextBuffer *bobgui_text_iter_get_buffer (const BobguiTextIter *iter);

/*
 * Life cycle
 */

GDK_AVAILABLE_IN_ALL
BobguiTextIter *bobgui_text_iter_copy     (const BobguiTextIter *iter);
GDK_AVAILABLE_IN_ALL
void         bobgui_text_iter_free     (BobguiTextIter       *iter);
GDK_AVAILABLE_IN_ALL
void         bobgui_text_iter_assign   (BobguiTextIter       *iter,
                                     const BobguiTextIter *other);

GDK_AVAILABLE_IN_ALL
GType        bobgui_text_iter_get_type (void) G_GNUC_CONST;

/*
 * Convert to different kinds of index
 */

GDK_AVAILABLE_IN_ALL
int bobgui_text_iter_get_offset      (const BobguiTextIter *iter);
GDK_AVAILABLE_IN_ALL
int bobgui_text_iter_get_line        (const BobguiTextIter *iter);
GDK_AVAILABLE_IN_ALL
int bobgui_text_iter_get_line_offset (const BobguiTextIter *iter);
GDK_AVAILABLE_IN_ALL
int bobgui_text_iter_get_line_index  (const BobguiTextIter *iter);

GDK_AVAILABLE_IN_ALL
int bobgui_text_iter_get_visible_line_offset (const BobguiTextIter *iter);
GDK_AVAILABLE_IN_ALL
int bobgui_text_iter_get_visible_line_index (const BobguiTextIter *iter);


/*
 * “Dereference” operators
 */
GDK_AVAILABLE_IN_ALL
gunichar bobgui_text_iter_get_char          (const BobguiTextIter  *iter);

/* includes the 0xFFFC char for pixmaps/widgets, so char offsets
 * into the returned string map properly into buffer char offsets
 */
GDK_AVAILABLE_IN_ALL
char    *bobgui_text_iter_get_slice         (const BobguiTextIter  *start,
                                          const BobguiTextIter  *end);

/* includes only text, no 0xFFFC */
GDK_AVAILABLE_IN_ALL
char    *bobgui_text_iter_get_text          (const BobguiTextIter  *start,
                                          const BobguiTextIter  *end);
/* exclude invisible chars */
GDK_AVAILABLE_IN_ALL
char    *bobgui_text_iter_get_visible_slice (const BobguiTextIter  *start,
                                          const BobguiTextIter  *end);
GDK_AVAILABLE_IN_ALL
char    *bobgui_text_iter_get_visible_text  (const BobguiTextIter  *start,
                                          const BobguiTextIter  *end);

GDK_AVAILABLE_IN_ALL
GdkPaintable *bobgui_text_iter_get_paintable (const BobguiTextIter *iter);
GDK_AVAILABLE_IN_ALL
GSList      *bobgui_text_iter_get_marks      (const BobguiTextIter *iter);

GDK_AVAILABLE_IN_ALL
BobguiTextChildAnchor* bobgui_text_iter_get_child_anchor (const BobguiTextIter *iter);

/* Return list of tags toggled at this point (toggled_on determines
 * whether the list is of on-toggles or off-toggles)
 */
GDK_AVAILABLE_IN_ALL
GSList  *bobgui_text_iter_get_toggled_tags  (const BobguiTextIter  *iter,
                                          gboolean            toggled_on);

GDK_AVAILABLE_IN_ALL
gboolean bobgui_text_iter_starts_tag        (const BobguiTextIter  *iter,
                                          BobguiTextTag         *tag);

GDK_AVAILABLE_IN_ALL
gboolean bobgui_text_iter_ends_tag          (const BobguiTextIter  *iter,
                                          BobguiTextTag         *tag);

GDK_AVAILABLE_IN_ALL
gboolean bobgui_text_iter_toggles_tag       (const BobguiTextIter  *iter,
                                          BobguiTextTag         *tag);

GDK_AVAILABLE_IN_ALL
gboolean bobgui_text_iter_has_tag           (const BobguiTextIter   *iter,
                                          BobguiTextTag          *tag);
GDK_AVAILABLE_IN_ALL
GSList  *bobgui_text_iter_get_tags          (const BobguiTextIter   *iter);

GDK_AVAILABLE_IN_ALL
gboolean bobgui_text_iter_editable          (const BobguiTextIter   *iter,
                                          gboolean             default_setting);
GDK_AVAILABLE_IN_ALL
gboolean bobgui_text_iter_can_insert        (const BobguiTextIter   *iter,
                                          gboolean             default_editability);

GDK_AVAILABLE_IN_ALL
gboolean bobgui_text_iter_starts_word        (const BobguiTextIter   *iter);
GDK_AVAILABLE_IN_ALL
gboolean bobgui_text_iter_ends_word          (const BobguiTextIter   *iter);
GDK_AVAILABLE_IN_ALL
gboolean bobgui_text_iter_inside_word        (const BobguiTextIter   *iter);
GDK_AVAILABLE_IN_ALL
gboolean bobgui_text_iter_starts_sentence    (const BobguiTextIter   *iter);
GDK_AVAILABLE_IN_ALL
gboolean bobgui_text_iter_ends_sentence      (const BobguiTextIter   *iter);
GDK_AVAILABLE_IN_ALL
gboolean bobgui_text_iter_inside_sentence    (const BobguiTextIter   *iter);
GDK_AVAILABLE_IN_ALL
gboolean bobgui_text_iter_starts_line        (const BobguiTextIter   *iter);
GDK_AVAILABLE_IN_ALL
gboolean bobgui_text_iter_ends_line          (const BobguiTextIter   *iter);
GDK_AVAILABLE_IN_ALL
gboolean bobgui_text_iter_is_cursor_position (const BobguiTextIter   *iter);

GDK_AVAILABLE_IN_ALL
int      bobgui_text_iter_get_chars_in_line (const BobguiTextIter   *iter);
GDK_AVAILABLE_IN_ALL
int      bobgui_text_iter_get_bytes_in_line (const BobguiTextIter   *iter);

GDK_AVAILABLE_IN_ALL
PangoLanguage* bobgui_text_iter_get_language   (const BobguiTextIter *iter);
GDK_AVAILABLE_IN_ALL
gboolean       bobgui_text_iter_is_end         (const BobguiTextIter *iter);
GDK_AVAILABLE_IN_ALL
gboolean       bobgui_text_iter_is_start       (const BobguiTextIter *iter);

/*
 * Moving around the buffer
 */

GDK_AVAILABLE_IN_ALL
gboolean bobgui_text_iter_forward_char         (BobguiTextIter *iter);
GDK_AVAILABLE_IN_ALL
gboolean bobgui_text_iter_backward_char        (BobguiTextIter *iter);
GDK_AVAILABLE_IN_ALL
gboolean bobgui_text_iter_forward_chars        (BobguiTextIter *iter,
                                             int          count);
GDK_AVAILABLE_IN_ALL
gboolean bobgui_text_iter_backward_chars       (BobguiTextIter *iter,
                                             int          count);
GDK_AVAILABLE_IN_ALL
gboolean bobgui_text_iter_forward_line         (BobguiTextIter *iter);
GDK_AVAILABLE_IN_ALL
gboolean bobgui_text_iter_backward_line        (BobguiTextIter *iter);
GDK_AVAILABLE_IN_ALL
gboolean bobgui_text_iter_forward_lines        (BobguiTextIter *iter,
                                             int          count);
GDK_AVAILABLE_IN_ALL
gboolean bobgui_text_iter_backward_lines       (BobguiTextIter *iter,
                                             int          count);
GDK_AVAILABLE_IN_ALL
gboolean bobgui_text_iter_forward_word_end     (BobguiTextIter *iter);
GDK_AVAILABLE_IN_ALL
gboolean bobgui_text_iter_backward_word_start  (BobguiTextIter *iter);
GDK_AVAILABLE_IN_ALL
gboolean bobgui_text_iter_forward_word_ends    (BobguiTextIter *iter,
                                             int          count);
GDK_AVAILABLE_IN_ALL
gboolean bobgui_text_iter_backward_word_starts (BobguiTextIter *iter,
                                             int          count);

GDK_AVAILABLE_IN_ALL
gboolean bobgui_text_iter_forward_visible_line   (BobguiTextIter *iter);
GDK_AVAILABLE_IN_ALL
gboolean bobgui_text_iter_backward_visible_line  (BobguiTextIter *iter);
GDK_AVAILABLE_IN_ALL
gboolean bobgui_text_iter_forward_visible_lines  (BobguiTextIter *iter,
                                               int          count);
GDK_AVAILABLE_IN_ALL
gboolean bobgui_text_iter_backward_visible_lines (BobguiTextIter *iter,
                                               int          count);

GDK_AVAILABLE_IN_ALL
gboolean bobgui_text_iter_forward_visible_word_end     (BobguiTextIter *iter);
GDK_AVAILABLE_IN_ALL
gboolean bobgui_text_iter_backward_visible_word_start  (BobguiTextIter *iter);
GDK_AVAILABLE_IN_ALL
gboolean bobgui_text_iter_forward_visible_word_ends    (BobguiTextIter *iter,
                                             int          count);
GDK_AVAILABLE_IN_ALL
gboolean bobgui_text_iter_backward_visible_word_starts (BobguiTextIter *iter,
                                             int          count);

GDK_AVAILABLE_IN_ALL
gboolean bobgui_text_iter_forward_sentence_end     (BobguiTextIter *iter);
GDK_AVAILABLE_IN_ALL
gboolean bobgui_text_iter_backward_sentence_start  (BobguiTextIter *iter);
GDK_AVAILABLE_IN_ALL
gboolean bobgui_text_iter_forward_sentence_ends    (BobguiTextIter *iter,
                                                 int          count);
GDK_AVAILABLE_IN_ALL
gboolean bobgui_text_iter_backward_sentence_starts (BobguiTextIter *iter,
                                                 int          count);
/* cursor positions are almost equivalent to chars, but not quite;
 * in some languages, you can’t put the cursor between certain
 * chars. Also, you can’t put the cursor between \r\n at the end
 * of a line.
 */
GDK_AVAILABLE_IN_ALL
gboolean bobgui_text_iter_forward_cursor_position   (BobguiTextIter *iter);
GDK_AVAILABLE_IN_ALL
gboolean bobgui_text_iter_backward_cursor_position  (BobguiTextIter *iter);
GDK_AVAILABLE_IN_ALL
gboolean bobgui_text_iter_forward_cursor_positions  (BobguiTextIter *iter,
                                                  int          count);
GDK_AVAILABLE_IN_ALL
gboolean bobgui_text_iter_backward_cursor_positions (BobguiTextIter *iter,
                                                  int          count);

GDK_AVAILABLE_IN_ALL
gboolean bobgui_text_iter_forward_visible_cursor_position   (BobguiTextIter *iter);
GDK_AVAILABLE_IN_ALL
gboolean bobgui_text_iter_backward_visible_cursor_position  (BobguiTextIter *iter);
GDK_AVAILABLE_IN_ALL
gboolean bobgui_text_iter_forward_visible_cursor_positions  (BobguiTextIter *iter,
                                                          int          count);
GDK_AVAILABLE_IN_ALL
gboolean bobgui_text_iter_backward_visible_cursor_positions (BobguiTextIter *iter,
                                                          int          count);

GDK_AVAILABLE_IN_ALL
void     bobgui_text_iter_set_offset         (BobguiTextIter *iter,
                                           int          char_offset);
GDK_AVAILABLE_IN_ALL
void     bobgui_text_iter_set_line           (BobguiTextIter *iter,
                                           int          line_number);
GDK_AVAILABLE_IN_ALL
void     bobgui_text_iter_set_line_offset    (BobguiTextIter *iter,
                                           int          char_on_line);
GDK_AVAILABLE_IN_ALL
void     bobgui_text_iter_set_line_index     (BobguiTextIter *iter,
                                           int          byte_on_line);
GDK_AVAILABLE_IN_ALL
void     bobgui_text_iter_forward_to_end     (BobguiTextIter *iter);
GDK_AVAILABLE_IN_ALL
gboolean bobgui_text_iter_forward_to_line_end (BobguiTextIter *iter);

GDK_AVAILABLE_IN_ALL
void     bobgui_text_iter_set_visible_line_offset (BobguiTextIter *iter,
                                                int          char_on_line);
GDK_AVAILABLE_IN_ALL
void     bobgui_text_iter_set_visible_line_index  (BobguiTextIter *iter,
                                                int          byte_on_line);

/* returns TRUE if a toggle was found; NULL for the tag pointer
 * means “any tag toggle”, otherwise the next toggle of the
 * specified tag is located.
 */
GDK_AVAILABLE_IN_ALL
gboolean bobgui_text_iter_forward_to_tag_toggle (BobguiTextIter *iter,
                                              BobguiTextTag  *tag);

GDK_AVAILABLE_IN_ALL
gboolean bobgui_text_iter_backward_to_tag_toggle (BobguiTextIter *iter,
                                               BobguiTextTag  *tag);

/**
 * BobguiTextCharPredicate:
 * @ch: a Unicode code point
 * @user_data: data passed to the callback
 *
 * The predicate function used by bobgui_text_iter_forward_find_char() and
 * bobgui_text_iter_backward_find_char().
 *
 * Returns: %TRUE if the predicate is satisfied, and the iteration should
 *   stop, and %FALSE otherwise
 */
typedef gboolean (* BobguiTextCharPredicate) (gunichar ch, gpointer user_data);

GDK_AVAILABLE_IN_ALL
gboolean bobgui_text_iter_forward_find_char  (BobguiTextIter          *iter,
                                           BobguiTextCharPredicate  pred,
                                           gpointer              user_data,
                                           const BobguiTextIter    *limit);
GDK_AVAILABLE_IN_ALL
gboolean bobgui_text_iter_backward_find_char (BobguiTextIter          *iter,
                                           BobguiTextCharPredicate  pred,
                                           gpointer              user_data,
                                           const BobguiTextIter    *limit);

GDK_AVAILABLE_IN_ALL
gboolean bobgui_text_iter_forward_search  (const BobguiTextIter *iter,
                                        const char        *str,
                                        BobguiTextSearchFlags flags,
                                        BobguiTextIter       *match_start,
                                        BobguiTextIter       *match_end,
                                        const BobguiTextIter *limit);

GDK_AVAILABLE_IN_ALL
gboolean bobgui_text_iter_backward_search (const BobguiTextIter *iter,
                                        const char        *str,
                                        BobguiTextSearchFlags flags,
                                        BobguiTextIter       *match_start,
                                        BobguiTextIter       *match_end,
                                        const BobguiTextIter *limit);

/*
 * Comparisons
 */
GDK_AVAILABLE_IN_ALL
gboolean bobgui_text_iter_equal           (const BobguiTextIter *lhs,
                                        const BobguiTextIter *rhs);
GDK_AVAILABLE_IN_ALL
int      bobgui_text_iter_compare         (const BobguiTextIter *lhs,
                                        const BobguiTextIter *rhs);
GDK_AVAILABLE_IN_ALL
gboolean bobgui_text_iter_in_range        (const BobguiTextIter *iter,
                                        const BobguiTextIter *start,
                                        const BobguiTextIter *end);

/* Put these two in ascending order */
GDK_AVAILABLE_IN_ALL
void     bobgui_text_iter_order           (BobguiTextIter *first,
                                        BobguiTextIter *second);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiTextIter, bobgui_text_iter_free)

G_END_DECLS

