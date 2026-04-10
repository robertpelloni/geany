/* BOBGUI - The Bobgui Framework
 * bobguitextbuffer.h Copyright (C) 2000 Red Hat, Inc.
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

#include <bobgui/bobguiwidget.h>
#include <bobgui/bobguitexttagtable.h>
#include <bobgui/bobguitextiter.h>
#include <bobgui/bobguitextmark.h>
#include <bobgui/bobguitextchild.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_TEXT_BUFFER            (bobgui_text_buffer_get_type ())
#define BOBGUI_TEXT_BUFFER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_TEXT_BUFFER, BobguiTextBuffer))
#define BOBGUI_TEXT_BUFFER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BOBGUI_TYPE_TEXT_BUFFER, BobguiTextBufferClass))
#define BOBGUI_IS_TEXT_BUFFER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_TEXT_BUFFER))
#define BOBGUI_IS_TEXT_BUFFER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BOBGUI_TYPE_TEXT_BUFFER))
#define BOBGUI_TEXT_BUFFER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_TEXT_BUFFER, BobguiTextBufferClass))

typedef struct _BobguiTextBufferPrivate BobguiTextBufferPrivate;
typedef struct _BobguiTextBufferClass BobguiTextBufferClass;

struct _BobguiTextBuffer
{
  GObject parent_instance;

  BobguiTextBufferPrivate *priv;
};

/**
 * BobguiTextBufferCommitNotify:
 * @buffer: the text buffer being notified
 * @flags: the type of commit notification
 * @position: the position of the text operation
 * @length: the length of the text operation in characters
 * @user_data: (closure): user data passed to the callback
 *
 * A notification callback used by [method@Bobgui.TextBuffer.add_commit_notify].
 *
 * You may not modify the [class@Bobgui.TextBuffer] from a
 * [callback@Bobgui.TextBufferCommitNotify] callback and that is enforced
 * by the [class@Bobgui.TextBuffer] API.
 *
 * [callback@Bobgui.TextBufferCommitNotify] may be used to be notified about
 * changes to the underlying buffer right before-or-after the changes are
 * committed to the underlying B-Tree. This is useful if you want to observe
 * changes to the buffer without other signal handlers potentially modifying
 * state on the way to the default signal handler.
 *
 * When @flags is `BOBGUI_TEXT_BUFFER_NOTIFY_BEFORE_INSERT`, `position` is set to
 * the offset in characters from the start of the buffer where the insertion
 * will occur. `length` is set to the number of characters to be inserted.  You
 * may not yet retrieve the text until it has been inserted. You may access the
 * text from `BOBGUI_TEXT_BUFFER_NOTIFY_AFTER_INSERT` using
 * [method@Bobgui.TextBuffer.get_slice].
 *
 * When @flags is `BOBGUI_TEXT_BUFFER_NOTIFY_AFTER_INSERT`, `position` is set to
 * offset in characters where the insertion occurred and `length` is set
 * to the number of characters inserted.
 *
 * When @flags is `BOBGUI_TEXT_BUFFER_NOTIFY_BEFORE_DELETE`, `position` is set to
 * offset in characters where the deletion will occur and `length` is set
 * to the number of characters that will be removed. You may still retrieve
 * the text from this handler using `position` and `length`.
 *
 * When @flags is `BOBGUI_TEXT_BUFFER_NOTIFY_AFTER_DELETE`, `length` is set to
 * zero to denote that the delete-range has already been committed to the
 * underlying B-Tree. You may no longer retrieve the text that has been
 * deleted from the [class@Bobgui.TextBuffer].
 *
 * Since: 4.16
 */
typedef void (*BobguiTextBufferCommitNotify) (BobguiTextBuffer            *buffer,
                                           BobguiTextBufferNotifyFlags  flags,
                                           guint                     position,
                                           guint                     length,
                                           gpointer                  user_data);

/**
 * BobguiTextBufferClass:
 * @parent_class: The object class structure needs to be the first.
 * @insert_text: The class handler for the `BobguiTextBuffer::insert-text` signal.
 * @insert_paintable: The class handler for the `BobguiTextBuffer::insert-paintable` signal.
 * @insert_child_anchor: The class handler for the `BobguiTextBuffer::insert-child-anchor` signal.
 * @delete_range: The class handler for the `BobguiTextBuffer::delete-range` signal.
 * @changed: The class handler for the `BobguiTextBuffer::changed` signal.
 * @modified_changed: The class handler for the `BobguiTextBuffer::modified-changed` signal.
 * @mark_set: The class handler for the `BobguiTextBuffer::mark-set` signal.
 * @mark_deleted: The class handler for the `BobguiTextBuffer::mark-deleted` signal.
 * @apply_tag: The class handler for the `BobguiTextBuffer::apply-tag` signal.
 * @remove_tag: The class handler for the `BobguiTextBuffer::remove-tag` signal.
 * @begin_user_action: The class handler for the `BobguiTextBuffer::begin-user-action` signal.
 * @end_user_action: The class handler for the `BobguiTextBuffer::end-user-action` signal.
 * @paste_done: The class handler for the `BobguiTextBuffer::paste-done` signal.
 * @undo: The class handler for the `BobguiTextBuffer::undo` signal
 * @redo: The class handler for the `BobguiTextBuffer::redo` signal
 *
 * The class structure for `BobguiTextBuffer`.
 */
struct _BobguiTextBufferClass
{
  GObjectClass parent_class;

  void (* insert_text)            (BobguiTextBuffer      *buffer,
                                   BobguiTextIter        *pos,
                                   const char         *new_text,
                                   int                 new_text_length);

  void (* insert_paintable)       (BobguiTextBuffer      *buffer,
                                   BobguiTextIter        *iter,
                                   GdkPaintable       *paintable);

  void (* insert_child_anchor)    (BobguiTextBuffer      *buffer,
                                   BobguiTextIter        *iter,
                                   BobguiTextChildAnchor *anchor);

  void (* delete_range)           (BobguiTextBuffer      *buffer,
                                   BobguiTextIter        *start,
                                   BobguiTextIter        *end);

  void (* changed)                (BobguiTextBuffer      *buffer);

  void (* modified_changed)       (BobguiTextBuffer      *buffer);

  void (* mark_set)               (BobguiTextBuffer      *buffer,
                                   const BobguiTextIter  *location,
                                   BobguiTextMark        *mark);

  void (* mark_deleted)           (BobguiTextBuffer      *buffer,
                                   BobguiTextMark        *mark);

  void (* apply_tag)              (BobguiTextBuffer      *buffer,
                                   BobguiTextTag         *tag,
                                   const BobguiTextIter  *start,
                                   const BobguiTextIter  *end);

  void (* remove_tag)             (BobguiTextBuffer      *buffer,
                                   BobguiTextTag         *tag,
                                   const BobguiTextIter  *start,
                                   const BobguiTextIter  *end);

  void (* begin_user_action)      (BobguiTextBuffer      *buffer);

  void (* end_user_action)        (BobguiTextBuffer      *buffer);

  void (* paste_done)             (BobguiTextBuffer      *buffer,
                                   GdkClipboard       *clipboard);
  void (* undo)                   (BobguiTextBuffer      *buffer);
  void (* redo)                   (BobguiTextBuffer      *buffer);

  /*< private >*/

  /* Padding for future expansion */
  void (*_bobgui_reserved1) (void);
  void (*_bobgui_reserved2) (void);
  void (*_bobgui_reserved3) (void);
  void (*_bobgui_reserved4) (void);
};

GDK_AVAILABLE_IN_ALL
GType        bobgui_text_buffer_get_type       (void) G_GNUC_CONST;



/* table is NULL to create a new one */
GDK_AVAILABLE_IN_ALL
BobguiTextBuffer *bobgui_text_buffer_new            (BobguiTextTagTable *table);
GDK_AVAILABLE_IN_ALL
int            bobgui_text_buffer_get_line_count (BobguiTextBuffer   *buffer);
GDK_AVAILABLE_IN_ALL
int            bobgui_text_buffer_get_char_count (BobguiTextBuffer   *buffer);


GDK_AVAILABLE_IN_ALL
BobguiTextTagTable* bobgui_text_buffer_get_tag_table (BobguiTextBuffer  *buffer);

/* Delete whole buffer, then insert */
GDK_AVAILABLE_IN_ALL
void bobgui_text_buffer_set_text          (BobguiTextBuffer *buffer,
                                        const char    *text,
                                        int            len);

/* Insert into the buffer */
GDK_AVAILABLE_IN_ALL
void bobgui_text_buffer_insert            (BobguiTextBuffer *buffer,
                                        BobguiTextIter   *iter,
                                        const char    *text,
                                        int            len);
GDK_AVAILABLE_IN_ALL
void bobgui_text_buffer_insert_at_cursor  (BobguiTextBuffer *buffer,
                                        const char    *text,
                                        int            len);

GDK_AVAILABLE_IN_ALL
gboolean bobgui_text_buffer_insert_interactive           (BobguiTextBuffer *buffer,
                                                       BobguiTextIter   *iter,
                                                       const char    *text,
                                                       int            len,
                                                       gboolean       default_editable);
GDK_AVAILABLE_IN_ALL
gboolean bobgui_text_buffer_insert_interactive_at_cursor (BobguiTextBuffer *buffer,
                                                       const char    *text,
                                                       int            len,
                                                       gboolean       default_editable);

GDK_AVAILABLE_IN_ALL
void     bobgui_text_buffer_insert_range             (BobguiTextBuffer     *buffer,
                                                   BobguiTextIter       *iter,
                                                   const BobguiTextIter *start,
                                                   const BobguiTextIter *end);
GDK_AVAILABLE_IN_ALL
gboolean bobgui_text_buffer_insert_range_interactive (BobguiTextBuffer     *buffer,
                                                   BobguiTextIter       *iter,
                                                   const BobguiTextIter *start,
                                                   const BobguiTextIter *end,
                                                   gboolean           default_editable);

GDK_AVAILABLE_IN_ALL
void    bobgui_text_buffer_insert_with_tags          (BobguiTextBuffer     *buffer,
                                                   BobguiTextIter       *iter,
                                                   const char        *text,
                                                   int                len,
                                                   BobguiTextTag        *first_tag,
                                                   ...) G_GNUC_NULL_TERMINATED;

GDK_AVAILABLE_IN_ALL
void    bobgui_text_buffer_insert_with_tags_by_name  (BobguiTextBuffer     *buffer,
                                                   BobguiTextIter       *iter,
                                                   const char        *text,
                                                   int                len,
                                                   const char        *first_tag_name,
                                                   ...) G_GNUC_NULL_TERMINATED;

GDK_AVAILABLE_IN_ALL
void     bobgui_text_buffer_insert_markup            (BobguiTextBuffer     *buffer,
                                                   BobguiTextIter       *iter,
                                                   const char        *markup,
                                                   int                len);

/* Delete from the buffer */
GDK_AVAILABLE_IN_ALL
void     bobgui_text_buffer_delete             (BobguiTextBuffer *buffer,
					     BobguiTextIter   *start,
					     BobguiTextIter   *end);
GDK_AVAILABLE_IN_ALL
gboolean bobgui_text_buffer_delete_interactive (BobguiTextBuffer *buffer,
					     BobguiTextIter   *start_iter,
					     BobguiTextIter   *end_iter,
					     gboolean       default_editable);
GDK_AVAILABLE_IN_ALL
gboolean bobgui_text_buffer_backspace          (BobguiTextBuffer *buffer,
					     BobguiTextIter   *iter,
					     gboolean       interactive,
					     gboolean       default_editable);

/* Obtain strings from the buffer */
GDK_AVAILABLE_IN_ALL
char           *bobgui_text_buffer_get_text            (BobguiTextBuffer     *buffer,
                                                     const BobguiTextIter *start,
                                                     const BobguiTextIter *end,
                                                     gboolean           include_hidden_chars);

GDK_AVAILABLE_IN_ALL
char           *bobgui_text_buffer_get_slice           (BobguiTextBuffer     *buffer,
                                                     const BobguiTextIter *start,
                                                     const BobguiTextIter *end,
                                                     gboolean           include_hidden_chars);

/* Insert a paintable */
GDK_AVAILABLE_IN_ALL
void bobgui_text_buffer_insert_paintable      (BobguiTextBuffer *buffer,
                                            BobguiTextIter   *iter,
                                            GdkPaintable  *paintable);

/* Insert a child anchor */
GDK_AVAILABLE_IN_ALL
void               bobgui_text_buffer_insert_child_anchor (BobguiTextBuffer      *buffer,
                                                        BobguiTextIter        *iter,
                                                        BobguiTextChildAnchor *anchor);

/* Convenience, create and insert a child anchor */
GDK_AVAILABLE_IN_ALL
BobguiTextChildAnchor *bobgui_text_buffer_create_child_anchor (BobguiTextBuffer *buffer,
                                                         BobguiTextIter   *iter);

/* Mark manipulation */
GDK_AVAILABLE_IN_ALL
void           bobgui_text_buffer_add_mark    (BobguiTextBuffer     *buffer,
                                            BobguiTextMark       *mark,
                                            const BobguiTextIter *where);
GDK_AVAILABLE_IN_ALL
BobguiTextMark   *bobgui_text_buffer_create_mark (BobguiTextBuffer     *buffer,
                                            const char        *mark_name,
                                            const BobguiTextIter *where,
                                            gboolean           left_gravity);
GDK_AVAILABLE_IN_ALL
void           bobgui_text_buffer_move_mark   (BobguiTextBuffer     *buffer,
                                            BobguiTextMark       *mark,
                                            const BobguiTextIter *where);
GDK_AVAILABLE_IN_ALL
void           bobgui_text_buffer_delete_mark (BobguiTextBuffer     *buffer,
                                            BobguiTextMark       *mark);
GDK_AVAILABLE_IN_ALL
BobguiTextMark*   bobgui_text_buffer_get_mark    (BobguiTextBuffer     *buffer,
                                            const char        *name);

GDK_AVAILABLE_IN_ALL
void bobgui_text_buffer_move_mark_by_name   (BobguiTextBuffer     *buffer,
                                          const char        *name,
                                          const BobguiTextIter *where);
GDK_AVAILABLE_IN_ALL
void bobgui_text_buffer_delete_mark_by_name (BobguiTextBuffer     *buffer,
                                          const char        *name);

GDK_AVAILABLE_IN_ALL
BobguiTextMark* bobgui_text_buffer_get_insert          (BobguiTextBuffer *buffer);
GDK_AVAILABLE_IN_ALL
BobguiTextMark* bobgui_text_buffer_get_selection_bound (BobguiTextBuffer *buffer);

/* efficiently move insert and selection_bound at the same time */
GDK_AVAILABLE_IN_ALL
void bobgui_text_buffer_place_cursor (BobguiTextBuffer     *buffer,
                                   const BobguiTextIter *where);
GDK_AVAILABLE_IN_ALL
void bobgui_text_buffer_select_range (BobguiTextBuffer     *buffer,
                                   const BobguiTextIter *ins,
				   const BobguiTextIter *bound);



/* Tag manipulation */
GDK_AVAILABLE_IN_ALL
void bobgui_text_buffer_apply_tag             (BobguiTextBuffer     *buffer,
                                            BobguiTextTag        *tag,
                                            const BobguiTextIter *start,
                                            const BobguiTextIter *end);
GDK_AVAILABLE_IN_ALL
void bobgui_text_buffer_remove_tag            (BobguiTextBuffer     *buffer,
                                            BobguiTextTag        *tag,
                                            const BobguiTextIter *start,
                                            const BobguiTextIter *end);
GDK_AVAILABLE_IN_ALL
void bobgui_text_buffer_apply_tag_by_name     (BobguiTextBuffer     *buffer,
                                            const char        *name,
                                            const BobguiTextIter *start,
                                            const BobguiTextIter *end);
GDK_AVAILABLE_IN_ALL
void bobgui_text_buffer_remove_tag_by_name    (BobguiTextBuffer     *buffer,
                                            const char        *name,
                                            const BobguiTextIter *start,
                                            const BobguiTextIter *end);
GDK_AVAILABLE_IN_ALL
void bobgui_text_buffer_remove_all_tags       (BobguiTextBuffer     *buffer,
                                            const BobguiTextIter *start,
                                            const BobguiTextIter *end);


/* You can either ignore the return value, or use it to
 * set the attributes of the tag. tag_name can be NULL
 */
GDK_AVAILABLE_IN_ALL
BobguiTextTag    *bobgui_text_buffer_create_tag (BobguiTextBuffer *buffer,
                                           const char    *tag_name,
                                           const char    *first_property_name,
                                           ...);

/* Obtain iterators pointed at various places, then you can move the
 * iterator around using the BobguiTextIter operators
 */
GDK_AVAILABLE_IN_ALL
gboolean bobgui_text_buffer_get_iter_at_line_offset (BobguiTextBuffer *buffer,
                                                  BobguiTextIter   *iter,
                                                  int            line_number,
                                                  int            char_offset);
GDK_AVAILABLE_IN_ALL
gboolean bobgui_text_buffer_get_iter_at_line_index  (BobguiTextBuffer *buffer,
                                                  BobguiTextIter   *iter,
                                                  int            line_number,
                                                  int            byte_index);
GDK_AVAILABLE_IN_ALL
void     bobgui_text_buffer_get_iter_at_offset      (BobguiTextBuffer *buffer,
                                                  BobguiTextIter   *iter,
                                                  int            char_offset);
GDK_AVAILABLE_IN_ALL
gboolean bobgui_text_buffer_get_iter_at_line        (BobguiTextBuffer *buffer,
                                                  BobguiTextIter   *iter,
                                                  int            line_number);
GDK_AVAILABLE_IN_ALL
void bobgui_text_buffer_get_start_iter          (BobguiTextBuffer *buffer,
                                              BobguiTextIter   *iter);
GDK_AVAILABLE_IN_ALL
void bobgui_text_buffer_get_end_iter            (BobguiTextBuffer *buffer,
                                              BobguiTextIter   *iter);
GDK_AVAILABLE_IN_ALL
void bobgui_text_buffer_get_bounds              (BobguiTextBuffer *buffer,
                                              BobguiTextIter   *start,
                                              BobguiTextIter   *end);
GDK_AVAILABLE_IN_ALL
void bobgui_text_buffer_get_iter_at_mark        (BobguiTextBuffer *buffer,
                                              BobguiTextIter   *iter,
                                              BobguiTextMark   *mark);

GDK_AVAILABLE_IN_ALL
void bobgui_text_buffer_get_iter_at_child_anchor (BobguiTextBuffer      *buffer,
                                               BobguiTextIter        *iter,
                                               BobguiTextChildAnchor *anchor);

/* There's no get_first_iter because you just get the iter for
   line or char 0 */

/* Used to keep track of whether the buffer needs saving; anytime the
   buffer contents change, the modified flag is turned on. Whenever
   you save, turn it off. Tags and marks do not affect the modified
   flag, but if you would like them to you can connect a handler to
   the tag/mark signals and call set_modified in your handler */

GDK_AVAILABLE_IN_ALL
gboolean        bobgui_text_buffer_get_modified            (BobguiTextBuffer *buffer);
GDK_AVAILABLE_IN_ALL
void            bobgui_text_buffer_set_modified            (BobguiTextBuffer *buffer,
                                                         gboolean       setting);

GDK_AVAILABLE_IN_ALL
gboolean        bobgui_text_buffer_get_has_selection       (BobguiTextBuffer *buffer);

GDK_AVAILABLE_IN_ALL
void bobgui_text_buffer_add_selection_clipboard    (BobguiTextBuffer     *buffer,
						 GdkClipboard      *clipboard);
GDK_AVAILABLE_IN_ALL
void bobgui_text_buffer_remove_selection_clipboard (BobguiTextBuffer     *buffer,
						 GdkClipboard      *clipboard);

GDK_AVAILABLE_IN_ALL
void            bobgui_text_buffer_cut_clipboard           (BobguiTextBuffer *buffer,
							 GdkClipboard  *clipboard,
                                                         gboolean       default_editable);
GDK_AVAILABLE_IN_ALL
void            bobgui_text_buffer_copy_clipboard          (BobguiTextBuffer *buffer,
							 GdkClipboard  *clipboard);
GDK_AVAILABLE_IN_ALL
void            bobgui_text_buffer_paste_clipboard         (BobguiTextBuffer *buffer,
							 GdkClipboard  *clipboard,
							 BobguiTextIter   *override_location,
                                                         gboolean       default_editable);

GDK_AVAILABLE_IN_ALL
gboolean        bobgui_text_buffer_get_selection_bounds    (BobguiTextBuffer *buffer,
                                                         BobguiTextIter   *start,
                                                         BobguiTextIter   *end);
GDK_AVAILABLE_IN_ALL
gboolean        bobgui_text_buffer_delete_selection        (BobguiTextBuffer *buffer,
                                                         gboolean       interactive,
                                                         gboolean       default_editable);

GDK_AVAILABLE_IN_ALL
GdkContentProvider *
                bobgui_text_buffer_get_selection_content    (BobguiTextBuffer *buffer);

GDK_AVAILABLE_IN_ALL
gboolean        bobgui_text_buffer_get_can_undo              (BobguiTextBuffer *buffer);
GDK_AVAILABLE_IN_ALL
gboolean        bobgui_text_buffer_get_can_redo              (BobguiTextBuffer *buffer);
GDK_AVAILABLE_IN_ALL
gboolean        bobgui_text_buffer_get_enable_undo           (BobguiTextBuffer *buffer);
GDK_AVAILABLE_IN_ALL
void            bobgui_text_buffer_set_enable_undo           (BobguiTextBuffer *buffer,
                                                           gboolean       enable_undo);
GDK_AVAILABLE_IN_ALL
guint           bobgui_text_buffer_get_max_undo_levels       (BobguiTextBuffer *buffer);
GDK_AVAILABLE_IN_ALL
void            bobgui_text_buffer_set_max_undo_levels       (BobguiTextBuffer *buffer,
                                                           guint          max_undo_levels);
GDK_AVAILABLE_IN_ALL
void            bobgui_text_buffer_undo                      (BobguiTextBuffer *buffer);
GDK_AVAILABLE_IN_ALL
void            bobgui_text_buffer_redo                      (BobguiTextBuffer *buffer);
GDK_AVAILABLE_IN_ALL
void            bobgui_text_buffer_begin_irreversible_action (BobguiTextBuffer *buffer);
GDK_AVAILABLE_IN_ALL
void            bobgui_text_buffer_end_irreversible_action   (BobguiTextBuffer *buffer);
GDK_AVAILABLE_IN_ALL
void            bobgui_text_buffer_begin_user_action         (BobguiTextBuffer *buffer);
GDK_AVAILABLE_IN_ALL
void            bobgui_text_buffer_end_user_action           (BobguiTextBuffer *buffer);
GDK_AVAILABLE_IN_4_16
guint           bobgui_text_buffer_add_commit_notify         (BobguiTextBuffer             *buffer,
                                                           BobguiTextBufferNotifyFlags   flags,
                                                           BobguiTextBufferCommitNotify  commit_notify,
                                                           gpointer                   user_data,
                                                           GDestroyNotify             destroy);
GDK_AVAILABLE_IN_4_16
void            bobgui_text_buffer_remove_commit_notify      (BobguiTextBuffer             *buffer,
                                                           guint                      commit_notify_handler);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiTextBuffer, g_object_unref)

G_END_DECLS

