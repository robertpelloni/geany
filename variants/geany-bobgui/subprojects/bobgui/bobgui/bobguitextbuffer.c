/* BOBGUI - The Bobgui Framework
 * bobguitextbuffer.c Copyright (C) 2000 Red Hat, Inc.
 *                 Copyright (C) 2004 Nokia Corporation
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

#include "config.h"
#include <string.h>
#include <stdarg.h>

#include "bobguimarshalers.h"
#include "bobguitextbuffer.h"
#include "bobguitexthistoryprivate.h"
#include "bobguitextbufferprivate.h"
#include "bobguitextbtreeprivate.h"
#include "bobguitextiterprivate.h"
#include "bobguitexttagprivate.h"
#include "bobguitexttagtableprivate.h"
#include "bobguipangoprivate.h"
#include "bobguiprivate.h"

#define DEFAULT_MAX_UNDO 200

/**
 * BobguiTextBuffer:
 *
 * Stores text and attributes for display in a `BobguiTextView`.
 *
 * You may wish to begin by reading the
 * [text widget conceptual overview](section-text-widget.html),
 * which gives an overview of all the objects and data types
 * related to the text widget and how they work together.
 *
 * BobguiTextBuffer can support undoing changes to the buffer
 * content, see [method@Bobgui.TextBuffer.set_enable_undo].
 */

typedef struct _BobguiTextLogAttrCache BobguiTextLogAttrCache;

struct _BobguiTextBufferPrivate
{
  BobguiTextTagTable *tag_table;
  BobguiTextBTree *btree;

  GSList *selection_clipboards;
  GdkContentProvider *selection_content;

  BobguiTextLogAttrCache *log_attr_cache;

  BobguiTextHistory *history;

  GArray *commit_funcs;
  guint last_commit_handler;

  guint user_action_count;

  /* Whether the buffer has been modified since last save */
  guint modified : 1;
  guint has_selection : 1;
  guint can_undo : 1;
  guint can_redo : 1;
  guint in_commit_notify : 1;
};

typedef struct _ClipboardRequest ClipboardRequest;

struct _ClipboardRequest
{
  BobguiTextBuffer *buffer;
  guint interactive : 1;
  guint default_editable : 1;
  guint replace_selection : 1;
};

typedef struct _CommitFunc
{
  BobguiTextBufferCommitNotify callback;
  gpointer user_data;
  GDestroyNotify user_data_destroy;
  BobguiTextBufferNotifyFlags flags;
  guint handler_id;
} CommitFunc;

enum {
  INSERT_TEXT,
  INSERT_PAINTABLE,
  INSERT_CHILD_ANCHOR,
  DELETE_RANGE,
  CHANGED,
  MODIFIED_CHANGED,
  MARK_SET,
  MARK_DELETED,
  APPLY_TAG,
  REMOVE_TAG,
  BEGIN_USER_ACTION,
  END_USER_ACTION,
  PASTE_DONE,
  UNDO,
  REDO,
  LAST_SIGNAL
};

enum {
  PROP_0,

  /* Construct */
  PROP_TAG_TABLE,

  /* Normal */
  PROP_TEXT,
  PROP_HAS_SELECTION,
  PROP_CURSOR_POSITION,
  PROP_CAN_UNDO,
  PROP_CAN_REDO,
  PROP_ENABLE_UNDO,
  LAST_PROP
};

static void bobgui_text_buffer_finalize   (GObject            *object);

static void bobgui_text_buffer_real_insert_text           (BobguiTextBuffer     *buffer,
                                                        BobguiTextIter       *iter,
                                                        const char        *text,
                                                        int                len);
static void bobgui_text_buffer_real_insert_paintable      (BobguiTextBuffer     *buffer,
                                                        BobguiTextIter       *iter,
                                                        GdkPaintable      *paintable);
static void bobgui_text_buffer_real_insert_anchor         (BobguiTextBuffer     *buffer,
                                                        BobguiTextIter       *iter,
                                                        BobguiTextChildAnchor *anchor);
static void bobgui_text_buffer_real_delete_range          (BobguiTextBuffer     *buffer,
                                                        BobguiTextIter       *start,
                                                        BobguiTextIter       *end);
static void bobgui_text_buffer_real_apply_tag             (BobguiTextBuffer     *buffer,
                                                        BobguiTextTag        *tag,
                                                        const BobguiTextIter *start_char,
                                                        const BobguiTextIter *end_char);
static void bobgui_text_buffer_real_remove_tag            (BobguiTextBuffer     *buffer,
                                                        BobguiTextTag        *tag,
                                                        const BobguiTextIter *start_char,
                                                        const BobguiTextIter *end_char);
static void bobgui_text_buffer_real_changed               (BobguiTextBuffer     *buffer);
static void bobgui_text_buffer_real_mark_set              (BobguiTextBuffer     *buffer,
                                                        const BobguiTextIter *iter,
                                                        BobguiTextMark       *mark);
static void bobgui_text_buffer_real_undo                  (BobguiTextBuffer     *buffer);
static void bobgui_text_buffer_real_redo                  (BobguiTextBuffer     *buffer);
static void bobgui_text_buffer_commit_notify              (BobguiTextBuffer     *buffer,
                                                        BobguiTextBufferNotifyFlags flags,
                                                        guint              position,
                                                        guint              length);

static BobguiTextBTree* get_btree (BobguiTextBuffer *buffer);
static void          free_log_attr_cache (BobguiTextLogAttrCache *cache);

static void remove_all_selection_clipboards       (BobguiTextBuffer *buffer);
static void update_selection_clipboards           (BobguiTextBuffer *buffer);

static void bobgui_text_buffer_set_property (GObject         *object,
				          guint            prop_id,
				          const GValue    *value,
				          GParamSpec      *pspec);
static void bobgui_text_buffer_get_property (GObject         *object,
				          guint            prop_id,
				          GValue          *value,
				          GParamSpec      *pspec);

static void bobgui_text_buffer_history_change_state (gpointer     funcs_data,
                                                  gboolean     is_modified,
                                                  gboolean     can_undo,
                                                  gboolean     can_redo);
static void bobgui_text_buffer_history_insert       (gpointer     funcs_data,
                                                  guint        begin,
                                                  guint        end,
                                                  const char  *text,
                                                  guint        len);
static void bobgui_text_buffer_history_delete       (gpointer     funcs_data,
                                                  guint        begin,
                                                  guint        end,
                                                  const char  *expected_text,
                                                  guint        len);
static void bobgui_text_buffer_history_select       (gpointer     funcs_data,
                                                  int          selection_insert,
                                                  int          selection_bound);

static guint signals[LAST_SIGNAL] = { 0 };
static GParamSpec *text_buffer_props[LAST_PROP];

G_DEFINE_TYPE_WITH_PRIVATE (BobguiTextBuffer, bobgui_text_buffer, G_TYPE_OBJECT)

#define BOBGUI_TYPE_TEXT_BUFFER_CONTENT            (bobgui_text_buffer_content_get_type ())
#define BOBGUI_TEXT_BUFFER_CONTENT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_TEXT_BUFFER_CONTENT, BobguiTextBufferContent))
#define BOBGUI_IS_TEXT_BUFFER_CONTENT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_TEXT_BUFFER_CONTENT))
#define BOBGUI_TEXT_BUFFER_CONTENT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BOBGUI_TYPE_TEXT_BUFFER_CONTENT, BobguiTextBufferContentClass))
#define BOBGUI_IS_TEXT_BUFFER_CONTENT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BOBGUI_TYPE_TEXT_BUFFER_CONTENT))
#define BOBGUI_TEXT_BUFFER_CONTENT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_TEXT_BUFFER_CONTENT, BobguiTextBufferContentClass))

typedef struct _BobguiTextBufferContent BobguiTextBufferContent;
typedef struct _BobguiTextBufferContentClass BobguiTextBufferContentClass;

struct _BobguiTextBufferContent
{
  GdkContentProvider parent;

  BobguiTextBuffer *text_buffer;
};

struct _BobguiTextBufferContentClass
{
  GdkContentProviderClass parent_class;
};

GType bobgui_text_buffer_content_get_type (void) G_GNUC_CONST;

G_DEFINE_TYPE (BobguiTextBufferContent, bobgui_text_buffer_content, GDK_TYPE_CONTENT_PROVIDER)

static BobguiTextHistoryFuncs history_funcs = {
  bobgui_text_buffer_history_change_state,
  bobgui_text_buffer_history_insert,
  bobgui_text_buffer_history_delete,
  bobgui_text_buffer_history_select,
};

static GdkContentFormats *
bobgui_text_buffer_content_ref_formats (GdkContentProvider *provider)
{
  BobguiTextBufferContent *content = BOBGUI_TEXT_BUFFER_CONTENT (provider);

  if (content->text_buffer)
    return gdk_content_formats_new_for_gtype (BOBGUI_TYPE_TEXT_BUFFER);
  else
    return gdk_content_formats_new (NULL, 0);
}

static gboolean
bobgui_text_buffer_content_get_value (GdkContentProvider  *provider,
                                   GValue              *value,
                                   GError             **error)
{
  BobguiTextBufferContent *content = BOBGUI_TEXT_BUFFER_CONTENT (provider);

  if (G_VALUE_HOLDS (value, BOBGUI_TYPE_TEXT_BUFFER) &&
      content->text_buffer != NULL)
    {
      g_value_set_object (value, content->text_buffer);
      return TRUE;
    }

  return GDK_CONTENT_PROVIDER_CLASS (bobgui_text_buffer_content_parent_class)->get_value (provider, value, error);
}

static void
bobgui_text_buffer_content_detach (GdkContentProvider *provider,
                                GdkClipboard       *clipboard)
{
  BobguiTextBufferContent *content = BOBGUI_TEXT_BUFFER_CONTENT (provider);
  BobguiTextIter insert;
  BobguiTextIter selection_bound;

  if (content->text_buffer == NULL)
    return;

  /* Move selection_bound to the insertion point */
  bobgui_text_buffer_get_iter_at_mark (content->text_buffer, &insert,
                                    bobgui_text_buffer_get_insert (content->text_buffer));
  bobgui_text_buffer_get_iter_at_mark (content->text_buffer, &selection_bound,
                                    bobgui_text_buffer_get_selection_bound (content->text_buffer));

  if (!bobgui_text_iter_equal (&insert, &selection_bound))
    bobgui_text_buffer_move_mark (content->text_buffer,
                               bobgui_text_buffer_get_selection_bound (content->text_buffer),
                               &insert);
}

static void
bobgui_text_buffer_content_class_init (BobguiTextBufferContentClass *class)
{
  GdkContentProviderClass *provider_class = GDK_CONTENT_PROVIDER_CLASS (class);

  provider_class->ref_formats = bobgui_text_buffer_content_ref_formats;
  provider_class->get_value = bobgui_text_buffer_content_get_value;
  provider_class->detach_clipboard = bobgui_text_buffer_content_detach;
}

static void
bobgui_text_buffer_content_init (BobguiTextBufferContent *content)
{
}

static GdkContentProvider *
bobgui_text_buffer_content_new (BobguiTextBuffer *buffer)
{
  BobguiTextBufferContent *content;

  content = g_object_new (BOBGUI_TYPE_TEXT_BUFFER_CONTENT, NULL);
  content->text_buffer = g_object_ref (buffer);

  return GDK_CONTENT_PROVIDER (content);
}

static void
bobgui_text_buffer_deserialize_text_plain_finish (GObject      *source,
                                               GAsyncResult *result,
                                               gpointer      deserializer)
{
  GOutputStream *stream = G_OUTPUT_STREAM (source);
  BobguiTextBuffer *buffer;
  BobguiTextIter start, end;
  GError *error = NULL;
  gssize written;
  gsize size;
  char *data;

  written = g_output_stream_splice_finish (stream, result, &error);
  if (written < 0)
    {
      gdk_content_deserializer_return_error (deserializer, error);
      return;
    }

  size = g_memory_output_stream_get_size (G_MEMORY_OUTPUT_STREAM (
                               g_filter_output_stream_get_base_stream (G_FILTER_OUTPUT_STREAM (stream))));
  data = g_memory_output_stream_steal_data (G_MEMORY_OUTPUT_STREAM (
                               g_filter_output_stream_get_base_stream (G_FILTER_OUTPUT_STREAM (stream))));

  if (data)
    {
      if (memchr (data, '\0', size))
        size = -1;
      buffer = g_value_get_object (gdk_content_deserializer_get_value (deserializer));
      bobgui_text_buffer_get_end_iter (buffer, &end);
      bobgui_text_buffer_insert (buffer, &end, data, size);
      bobgui_text_buffer_get_bounds (buffer, &start, &end);
      bobgui_text_buffer_select_range (buffer, &start, &end);

      g_free (data);
    }

  gdk_content_deserializer_return_success (deserializer);
}

static void
bobgui_text_buffer_deserialize_text_plain (GdkContentDeserializer *deserializer)
{
  GOutputStream *output, *filter;
  GCharsetConverter *converter;
  BobguiTextBuffer *buffer;
  GError *error = NULL;

  buffer = bobgui_text_buffer_new (NULL);
  g_value_set_object (gdk_content_deserializer_get_value (deserializer),
                      buffer);

  /* validates the stream */
  converter = g_charset_converter_new ("utf-8",
                                       gdk_content_deserializer_get_user_data (deserializer),
                                       &error);
  if (converter == NULL)
    {
      gdk_content_deserializer_return_error (deserializer, error);
      return;
    }
  g_charset_converter_set_use_fallback (converter, TRUE);

  output = g_memory_output_stream_new_resizable ();
  filter = g_converter_output_stream_new (output, G_CONVERTER (converter));
  g_object_unref (output);
  g_object_unref (converter);

  g_output_stream_splice_async (filter,
                                gdk_content_deserializer_get_input_stream (deserializer),
                                G_OUTPUT_STREAM_SPLICE_CLOSE_SOURCE | G_OUTPUT_STREAM_SPLICE_CLOSE_TARGET,
                                gdk_content_deserializer_get_priority (deserializer),
                                gdk_content_deserializer_get_cancellable (deserializer),
                                bobgui_text_buffer_deserialize_text_plain_finish,
                                deserializer);
  g_object_unref (filter);
}

static void
bobgui_text_buffer_serialize_text_plain_finish (GObject      *source,
                                             GAsyncResult *result,
                                             gpointer      serializer)
{
  GOutputStream *stream = G_OUTPUT_STREAM (source);
  GError *error = NULL;

  if (!g_output_stream_write_all_finish (stream, result, NULL, &error))
    gdk_content_serializer_return_error (serializer, error);
  else
    gdk_content_serializer_return_success (serializer);
}

static void
bobgui_text_buffer_serialize_text_plain (GdkContentSerializer *serializer)
{
  BobguiTextBuffer *buffer;
  BobguiTextIter start, end;
  char *str;
  GOutputStream *filter;
  GCharsetConverter *converter;
  GError *error = NULL;

  converter = g_charset_converter_new (gdk_content_serializer_get_user_data (serializer),
                                       "utf-8",
                                       &error);
  if (converter == NULL)
    {
      gdk_content_serializer_return_error (serializer, error);
      return;
    }
  g_charset_converter_set_use_fallback (converter, TRUE);

  filter = g_converter_output_stream_new (gdk_content_serializer_get_output_stream (serializer),
                                          G_CONVERTER (converter));
  g_object_unref (converter);

  buffer = g_value_get_object (gdk_content_serializer_get_value (serializer));

  if (bobgui_text_buffer_get_selection_bounds (buffer, &start, &end))
    {
      str = bobgui_text_iter_get_visible_text (&start, &end);
    }
  else
    {
      str = g_strdup ("");
    }
  gdk_content_serializer_set_task_data (serializer, str, g_free);

  g_output_stream_write_all_async (filter,
                                   str,
                                   strlen (str),
                                   gdk_content_serializer_get_priority (serializer),
                                   gdk_content_serializer_get_cancellable (serializer),
                                   bobgui_text_buffer_serialize_text_plain_finish,
                                   serializer);
  g_object_unref (filter);
}

static void
bobgui_text_buffer_register_serializers (void)
{
  const char *charset;

  gdk_content_register_deserializer ("text/plain;charset=utf-8",
                                     BOBGUI_TYPE_TEXT_BUFFER,
                                     bobgui_text_buffer_deserialize_text_plain,
                                     (gpointer) "utf-8",
                                     NULL);
  gdk_content_register_serializer (BOBGUI_TYPE_TEXT_BUFFER,
                                   "text/plain;charset=utf-8",
                                   bobgui_text_buffer_serialize_text_plain,
                                   (gpointer) "utf-8",
                                   NULL);
  gdk_content_register_deserializer ("text/plain",
                                     BOBGUI_TYPE_TEXT_BUFFER,
                                     bobgui_text_buffer_deserialize_text_plain,
                                     (gpointer) "ASCII",
                                     NULL);
  gdk_content_register_serializer (BOBGUI_TYPE_TEXT_BUFFER,
                                   "text/plain",
                                   bobgui_text_buffer_serialize_text_plain,
                                   (gpointer) "ASCII",
                                   NULL);
  if (!g_get_charset (&charset))
    {
      char *mime = g_strdup_printf ("text/plain;charset=%s", charset);
      gdk_content_register_serializer (BOBGUI_TYPE_TEXT_BUFFER,
                                       mime,
                                       bobgui_text_buffer_serialize_text_plain,
                                       (gpointer) charset,
                                       NULL);
      gdk_content_register_deserializer (mime,
                                         BOBGUI_TYPE_TEXT_BUFFER,
                                         bobgui_text_buffer_deserialize_text_plain,
                                         (gpointer) charset,
                                         NULL);
      g_free (mime);
    }

}

static void
bobgui_text_buffer_class_init (BobguiTextBufferClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = bobgui_text_buffer_finalize;
  object_class->set_property = bobgui_text_buffer_set_property;
  object_class->get_property = bobgui_text_buffer_get_property;

  klass->insert_text = bobgui_text_buffer_real_insert_text;
  klass->insert_paintable = bobgui_text_buffer_real_insert_paintable;
  klass->insert_child_anchor = bobgui_text_buffer_real_insert_anchor;
  klass->delete_range = bobgui_text_buffer_real_delete_range;
  klass->apply_tag = bobgui_text_buffer_real_apply_tag;
  klass->remove_tag = bobgui_text_buffer_real_remove_tag;
  klass->changed = bobgui_text_buffer_real_changed;
  klass->mark_set = bobgui_text_buffer_real_mark_set;
  klass->undo = bobgui_text_buffer_real_undo;
  klass->redo = bobgui_text_buffer_real_redo;

  /* Construct */
  /**
   * BobguiTextBuffer:tag-table:
   *
   * The BobguiTextTagTable for the buffer.
   */
  text_buffer_props[PROP_TAG_TABLE] =
      g_param_spec_object ("tag-table", NULL, NULL,
                           BOBGUI_TYPE_TEXT_TAG_TABLE,
                           BOBGUI_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  /* Normal properties */

  /**
   * BobguiTextBuffer:text:
   *
   * The text content of the buffer.
   *
   * Without child widgets and images,
   * see [method@Bobgui.TextBuffer.get_text] for more information.
   */
  text_buffer_props[PROP_TEXT] =
      g_param_spec_string ("text", NULL, NULL,
                           "",
                           BOBGUI_PARAM_READWRITE);

  /**
   * BobguiTextBuffer:has-selection:
   *
   * Whether the buffer has some text currently selected.
   */
  text_buffer_props[PROP_HAS_SELECTION] =
      g_param_spec_boolean ("has-selection", NULL, NULL,
                            FALSE,
                            BOBGUI_PARAM_READABLE);

  /**
   * BobguiTextBuffer:can-undo:
   *
   * Denotes that the buffer can undo the last applied action.
   */
  text_buffer_props[PROP_CAN_UNDO] =
    g_param_spec_boolean ("can-undo", NULL, NULL,
                          FALSE,
                          BOBGUI_PARAM_READABLE);

  /**
   * BobguiTextBuffer:can-redo:
   *
   * Denotes that the buffer can reapply the last undone action.
   */
  text_buffer_props[PROP_CAN_REDO] =
    g_param_spec_boolean ("can-redo", NULL, NULL,
                          FALSE,
                          BOBGUI_PARAM_READABLE);

  /**
   * BobguiTextBuffer:enable-undo:
   *
   * Denotes if support for undoing and redoing changes to the buffer is allowed.
   */
  text_buffer_props[PROP_ENABLE_UNDO] =
    g_param_spec_boolean ("enable-undo", NULL, NULL,
                          TRUE,
                          BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiTextBuffer:cursor-position:
   *
   * The position of the insert mark.
   *
   * This is an offset from the beginning of the buffer.
   * It is useful for getting notified when the cursor moves.
   */
  text_buffer_props[PROP_CURSOR_POSITION] =
      g_param_spec_int ("cursor-position", NULL, NULL,
			0, G_MAXINT,
                        0,
                        BOBGUI_PARAM_READABLE);

  g_object_class_install_properties (object_class, LAST_PROP, text_buffer_props);

  /**
   * BobguiTextBuffer::insert-text:
   * @textbuffer: the object which received the signal
   * @location: position to insert @text in @textbuffer
   * @text: the UTF-8 text to be inserted
   * @len: length of the inserted text in bytes
   *
   * Emitted to insert text in a `BobguiTextBuffer`.
   *
   * Insertion actually occurs in the default handler.
   *
   * Note that if your handler runs before the default handler
   * it must not invalidate the @location iter (or has to
   * revalidate it). The default signal handler revalidates
   * it to point to the end of the inserted text.
   *
   * See also: [method@Bobgui.TextBuffer.insert],
   * [method@Bobgui.TextBuffer.insert_range].
   */
  signals[INSERT_TEXT] =
    g_signal_new (I_("insert-text"),
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BobguiTextBufferClass, insert_text),
                  NULL, NULL,
                  _bobgui_marshal_VOID__BOXED_STRING_INT,
                  G_TYPE_NONE,
                  3,
                  BOBGUI_TYPE_TEXT_ITER | G_SIGNAL_TYPE_STATIC_SCOPE,
                  G_TYPE_STRING | G_SIGNAL_TYPE_STATIC_SCOPE,
                  G_TYPE_INT);
  g_signal_set_va_marshaller (signals[INSERT_TEXT], G_TYPE_FROM_CLASS (klass),
                              _bobgui_marshal_VOID__BOXED_STRING_INTv);

  /**
   * BobguiTextBuffer::insert-paintable:
   * @textbuffer: the object which received the signal
   * @location: position to insert @paintable in @textbuffer
   * @paintable: the `GdkPaintable` to be inserted
   *
   * Emitted to insert a `GdkPaintable` in a `BobguiTextBuffer`.
   *
   * Insertion actually occurs in the default handler.
   *
   * Note that if your handler runs before the default handler
   * it must not invalidate the @location iter (or has to
   * revalidate it). The default signal handler revalidates
   * it to be placed after the inserted @paintable.
   *
   * See also: [method@Bobgui.TextBuffer.insert_paintable].
   */
  signals[INSERT_PAINTABLE] =
    g_signal_new (I_("insert-paintable"),
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BobguiTextBufferClass, insert_paintable),
                  NULL, NULL,
                  _bobgui_marshal_VOID__BOXED_OBJECT,
                  G_TYPE_NONE,
                  2,
                  BOBGUI_TYPE_TEXT_ITER | G_SIGNAL_TYPE_STATIC_SCOPE,
                  GDK_TYPE_PAINTABLE);
  g_signal_set_va_marshaller (signals[INSERT_PAINTABLE],
                              G_TYPE_FROM_CLASS (klass),
                              _bobgui_marshal_VOID__BOXED_OBJECTv);


  /**
   * BobguiTextBuffer::insert-child-anchor:
   * @textbuffer: the object which received the signal
   * @location: position to insert @anchor in @textbuffer
   * @anchor: the `BobguiTextChildAnchor` to be inserted
   *
   * Emitted to insert a `BobguiTextChildAnchor` in a `BobguiTextBuffer`.
   *
   * Insertion actually occurs in the default handler.
   *
   * Note that if your handler runs before the default handler
   * it must not invalidate the @location iter (or has to
   * revalidate it). The default signal handler revalidates
   * it to be placed after the inserted @anchor.
   *
   * See also: [method@Bobgui.TextBuffer.insert_child_anchor].
   */
  signals[INSERT_CHILD_ANCHOR] =
    g_signal_new (I_("insert-child-anchor"),
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BobguiTextBufferClass, insert_child_anchor),
                  NULL, NULL,
                  _bobgui_marshal_VOID__BOXED_OBJECT,
                  G_TYPE_NONE,
                  2,
                  BOBGUI_TYPE_TEXT_ITER | G_SIGNAL_TYPE_STATIC_SCOPE,
                  BOBGUI_TYPE_TEXT_CHILD_ANCHOR);
  g_signal_set_va_marshaller (signals[INSERT_CHILD_ANCHOR],
                              G_TYPE_FROM_CLASS (klass),
                              _bobgui_marshal_VOID__BOXED_OBJECTv);

  /**
   * BobguiTextBuffer::delete-range:
   * @textbuffer: the object which received the signal
   * @start: the start of the range to be deleted
   * @end: the end of the range to be deleted
   *
   * Emitted to delete a range from a `BobguiTextBuffer`.
   *
   * Note that if your handler runs before the default handler
   * it must not invalidate the @start and @end iters (or has
   * to revalidate them). The default signal handler revalidates
   * the @start and @end iters to both point to the location
   * where text was deleted. Handlers which run after the default
   * handler (see g_signal_connect_after()) do not have access to
   * the deleted text.
   *
   * See also: [method@Bobgui.TextBuffer.delete].
   */
  signals[DELETE_RANGE] =
    g_signal_new (I_("delete-range"),
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BobguiTextBufferClass, delete_range),
                  NULL, NULL,
                  _bobgui_marshal_VOID__BOXED_BOXED,
                  G_TYPE_NONE,
                  2,
                  BOBGUI_TYPE_TEXT_ITER | G_SIGNAL_TYPE_STATIC_SCOPE,
                  BOBGUI_TYPE_TEXT_ITER | G_SIGNAL_TYPE_STATIC_SCOPE);
  g_signal_set_va_marshaller (signals[DELETE_RANGE],
                              G_TYPE_FROM_CLASS (klass),
                              _bobgui_marshal_VOID__BOXED_BOXEDv);

  /**
   * BobguiTextBuffer::changed:
   * @textbuffer: the object which received the signal
   *
   * Emitted when the content of a `BobguiTextBuffer` has changed.
   */
  signals[CHANGED] =
    g_signal_new (I_("changed"),
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BobguiTextBufferClass, changed),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE,
                  0);

  /**
   * BobguiTextBuffer::modified-changed:
   * @textbuffer: the object which received the signal
   *
   * Emitted when the modified bit of a `BobguiTextBuffer` flips.
   *
   * See also: [method@Bobgui.TextBuffer.set_modified].
   */
  signals[MODIFIED_CHANGED] =
    g_signal_new (I_("modified-changed"),
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BobguiTextBufferClass, modified_changed),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE,
                  0);

  /**
   * BobguiTextBuffer::mark-set:
   * @textbuffer: the object which received the signal
   * @location: The location of @mark in @textbuffer
   * @mark: The mark that is set
   *
   * Emitted as notification after a `BobguiTextMark` is set.
   *
   * See also:
   * [method@Bobgui.TextBuffer.create_mark],
   * [method@Bobgui.TextBuffer.move_mark].
   */
  signals[MARK_SET] =
    g_signal_new (I_("mark-set"),
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BobguiTextBufferClass, mark_set),
                  NULL, NULL,
                  _bobgui_marshal_VOID__BOXED_OBJECT,
                  G_TYPE_NONE,
                  2,
                  BOBGUI_TYPE_TEXT_ITER,
                  BOBGUI_TYPE_TEXT_MARK);
  g_signal_set_va_marshaller (signals[MARK_SET],
                              G_TYPE_FROM_CLASS (klass),
                              _bobgui_marshal_VOID__BOXED_OBJECTv);

  /**
   * BobguiTextBuffer::mark-deleted:
   * @textbuffer: the object which received the signal
   * @mark: The mark that was deleted
   *
   * Emitted as notification after a `BobguiTextMark` is deleted.
   *
   * See also: [method@Bobgui.TextBuffer.delete_mark].
   */
  signals[MARK_DELETED] =
    g_signal_new (I_("mark-deleted"),
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BobguiTextBufferClass, mark_deleted),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE,
                  1,
                  BOBGUI_TYPE_TEXT_MARK);

   /**
   * BobguiTextBuffer::apply-tag:
   * @textbuffer: the object which received the signal
   * @tag: the applied tag
   * @start: the start of the range the tag is applied to
   * @end: the end of the range the tag is applied to
   *
   * Emitted to apply a tag to a range of text in a `BobguiTextBuffer`.
   *
   * Applying actually occurs in the default handler.
   *
   * Note that if your handler runs before the default handler
   * it must not invalidate the @start and @end iters (or has to
   * revalidate them).
   *
   * See also:
   * [method@Bobgui.TextBuffer.apply_tag],
   * [method@Bobgui.TextBuffer.insert_with_tags],
   * [method@Bobgui.TextBuffer.insert_range].
   */
  signals[APPLY_TAG] =
    g_signal_new (I_("apply-tag"),
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BobguiTextBufferClass, apply_tag),
                  NULL, NULL,
                  _bobgui_marshal_VOID__OBJECT_BOXED_BOXED,
                  G_TYPE_NONE,
                  3,
                  BOBGUI_TYPE_TEXT_TAG,
                  BOBGUI_TYPE_TEXT_ITER,
                  BOBGUI_TYPE_TEXT_ITER);
  g_signal_set_va_marshaller (signals[APPLY_TAG],
                              G_TYPE_FROM_CLASS (klass),
                              _bobgui_marshal_VOID__OBJECT_BOXED_BOXEDv);

   /**
   * BobguiTextBuffer::remove-tag:
   * @textbuffer: the object which received the signal
   * @tag: the tag to be removed
   * @start: the start of the range the tag is removed from
   * @end: the end of the range the tag is removed from
   *
   * Emitted to remove all occurrences of @tag from a range
   * of text in a `BobguiTextBuffer`.
   *
   * Removal actually occurs in the default handler.
   *
   * Note that if your handler runs before the default handler
   * it must not invalidate the @start and @end iters (or has
   * to revalidate them).
   *
   * See also: [method@Bobgui.TextBuffer.remove_tag].
   */
  signals[REMOVE_TAG] =
    g_signal_new (I_("remove-tag"),
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BobguiTextBufferClass, remove_tag),
                  NULL, NULL,
                  _bobgui_marshal_VOID__OBJECT_BOXED_BOXED,
                  G_TYPE_NONE,
                  3,
                  BOBGUI_TYPE_TEXT_TAG,
                  BOBGUI_TYPE_TEXT_ITER,
                  BOBGUI_TYPE_TEXT_ITER);
  g_signal_set_va_marshaller (signals[REMOVE_TAG],
                              G_TYPE_FROM_CLASS (klass),
                              _bobgui_marshal_VOID__OBJECT_BOXED_BOXEDv);

   /**
   * BobguiTextBuffer::begin-user-action:
   * @textbuffer: the object which received the signal
   *
   * Emitted at the beginning of a single user-visible
   * operation on a `BobguiTextBuffer`.
   *
   * See also:
   * [method@Bobgui.TextBuffer.begin_user_action],
   * [method@Bobgui.TextBuffer.insert_interactive],
   * [method@Bobgui.TextBuffer.insert_range_interactive],
   * [method@Bobgui.TextBuffer.delete_interactive],
   * [method@Bobgui.TextBuffer.backspace],
   * [method@Bobgui.TextBuffer.delete_selection].
   */
  signals[BEGIN_USER_ACTION] =
    g_signal_new (I_("begin-user-action"),
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BobguiTextBufferClass, begin_user_action),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE,
                  0);

   /**
   * BobguiTextBuffer::end-user-action:
   * @textbuffer: the object which received the signal
   *
   * Emitted at the end of a single user-visible
   * operation on the `BobguiTextBuffer`.
   *
   * See also:
   * [method@Bobgui.TextBuffer.end_user_action],
   * [method@Bobgui.TextBuffer.insert_interactive],
   * [method@Bobgui.TextBuffer.insert_range_interactive],
   * [method@Bobgui.TextBuffer.delete_interactive],
   * [method@Bobgui.TextBuffer.backspace],
   * [method@Bobgui.TextBuffer.delete_selection],
   * [method@Bobgui.TextBuffer.backspace].
   */
  signals[END_USER_ACTION] =
    g_signal_new (I_("end-user-action"),
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BobguiTextBufferClass, end_user_action),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE,
                  0);

   /**
   * BobguiTextBuffer::paste-done:
   * @textbuffer: the object which received the signal
   * @clipboard: the `GdkClipboard` pasted from
   *
   * Emitted after paste operation has been completed.
   *
   * This is useful to properly scroll the view to the end
   * of the pasted text. See [method@Bobgui.TextBuffer.paste_clipboard]
   * for more details.
   */
  signals[PASTE_DONE] =
    g_signal_new (I_("paste-done"),
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BobguiTextBufferClass, paste_done),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE,
                  1,
                  GDK_TYPE_CLIPBOARD);

  /**
   * BobguiTextBuffer::redo:
   * @buffer: a `BobguiTextBuffer`
   *
   * Emitted when a request has been made to redo the
   * previously undone operation.
   */
  signals[REDO] =
    g_signal_new (I_("redo"),
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BobguiTextBufferClass, redo),
                  NULL, NULL, NULL, G_TYPE_NONE, 0);

  /**
   * BobguiTextBuffer::undo:
   * @buffer: a `BobguiTextBuffer`
   *
   * Emitted when a request has been made to undo the
   * previous operation or set of operations that have
   * been grouped together.
   */
  signals[UNDO] =
    g_signal_new (I_("undo"),
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BobguiTextBufferClass, undo),
                  NULL, NULL, NULL, G_TYPE_NONE, 0);

  bobgui_text_buffer_register_serializers ();
}

static void
bobgui_text_buffer_init (BobguiTextBuffer *buffer)
{
  buffer->priv = bobgui_text_buffer_get_instance_private (buffer);
  buffer->priv->tag_table = NULL;
  buffer->priv->history = bobgui_text_history_new (&history_funcs, buffer);

  bobgui_text_history_set_max_undo_levels (buffer->priv->history, DEFAULT_MAX_UNDO);
}

static void
set_table (BobguiTextBuffer *buffer, BobguiTextTagTable *table)
{
  BobguiTextBufferPrivate *priv = buffer->priv;

  g_return_if_fail (priv->tag_table == NULL);

  if (table)
    {
      priv->tag_table = table;
      g_object_ref (priv->tag_table);
      _bobgui_text_tag_table_add_buffer (table, buffer);
    }
}

static BobguiTextTagTable*
get_table (BobguiTextBuffer *buffer)
{
  BobguiTextBufferPrivate *priv = buffer->priv;

  if (priv->tag_table == NULL)
    {
      priv->tag_table = bobgui_text_tag_table_new ();
      _bobgui_text_tag_table_add_buffer (priv->tag_table, buffer);
    }

  return priv->tag_table;
}

static void
bobgui_text_buffer_set_property (GObject         *object,
                              guint            prop_id,
                              const GValue    *value,
                              GParamSpec      *pspec)
{
  BobguiTextBuffer *text_buffer;

  text_buffer = BOBGUI_TEXT_BUFFER (object);

  switch (prop_id)
    {
    case PROP_ENABLE_UNDO:
      bobgui_text_buffer_set_enable_undo (text_buffer, g_value_get_boolean (value));
      break;

    case PROP_TAG_TABLE:
      set_table (text_buffer, g_value_get_object (value));
      break;

    case PROP_TEXT:
      bobgui_text_buffer_set_text (text_buffer,
                                g_value_get_string (value), -1);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_text_buffer_get_property (GObject         *object,
                              guint            prop_id,
                              GValue          *value,
                              GParamSpec      *pspec)
{
  BobguiTextBuffer *text_buffer;
  BobguiTextIter iter;

  text_buffer = BOBGUI_TEXT_BUFFER (object);

  switch (prop_id)
    {
    case PROP_ENABLE_UNDO:
      g_value_set_boolean (value, bobgui_text_buffer_get_enable_undo (text_buffer));
      break;

    case PROP_TAG_TABLE:
      g_value_set_object (value, get_table (text_buffer));
      break;

    case PROP_TEXT:
      {
        BobguiTextIter start, end;

        bobgui_text_buffer_get_start_iter (text_buffer, &start);
        bobgui_text_buffer_get_end_iter (text_buffer, &end);

        g_value_take_string (value,
                            bobgui_text_buffer_get_text (text_buffer,
                                                      &start, &end, FALSE));
        break;
      }

    case PROP_HAS_SELECTION:
      g_value_set_boolean (value, text_buffer->priv->has_selection);
      break;

    case PROP_CURSOR_POSITION:
      bobgui_text_buffer_get_iter_at_mark (text_buffer, &iter,
    				        bobgui_text_buffer_get_insert (text_buffer));
      g_value_set_int (value, bobgui_text_iter_get_offset (&iter));
      break;

    case PROP_CAN_UNDO:
      g_value_set_boolean (value, bobgui_text_buffer_get_can_undo (text_buffer));
      break;

    case PROP_CAN_REDO:
      g_value_set_boolean (value, bobgui_text_buffer_get_can_redo (text_buffer));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

/**
 * bobgui_text_buffer_new:
 * @table: (nullable): a tag table, or %NULL to create a new one
 *
 * Creates a new text buffer.
 *
 * Returns: a new text buffer
 */
BobguiTextBuffer*
bobgui_text_buffer_new (BobguiTextTagTable *table)
{
  BobguiTextBuffer *text_buffer;

  text_buffer = g_object_new (BOBGUI_TYPE_TEXT_BUFFER, "tag-table", table, NULL);

  return text_buffer;
}

static void
bobgui_text_buffer_finalize (GObject *object)
{
  BobguiTextBuffer *buffer;
  BobguiTextBufferPrivate *priv;

  buffer = BOBGUI_TEXT_BUFFER (object);
  priv = buffer->priv;

  remove_all_selection_clipboards (buffer);

  g_clear_pointer (&buffer->priv->commit_funcs, g_array_unref);

  g_clear_object (&buffer->priv->history);

  if (priv->tag_table)
    {
      _bobgui_text_tag_table_remove_buffer (priv->tag_table, buffer);
      g_object_unref (priv->tag_table);
      priv->tag_table = NULL;
    }

  if (priv->btree)
    {
      _bobgui_text_btree_unref (priv->btree);
      priv->btree = NULL;
    }

  if (priv->log_attr_cache)
    free_log_attr_cache (priv->log_attr_cache);

  priv->log_attr_cache = NULL;

  G_OBJECT_CLASS (bobgui_text_buffer_parent_class)->finalize (object);
}

static BobguiTextBTree*
get_btree (BobguiTextBuffer *buffer)
{
  BobguiTextBufferPrivate *priv = buffer->priv;

  if (priv->btree == NULL)
    priv->btree = _bobgui_text_btree_new (bobgui_text_buffer_get_tag_table (buffer),
                                       buffer);

  return priv->btree;
}

BobguiTextBTree*
_bobgui_text_buffer_get_btree (BobguiTextBuffer *buffer)
{
  return get_btree (buffer);
}

/**
 * bobgui_text_buffer_get_tag_table:
 * @buffer: a `BobguiTextBuffer`
 *
 * Get the `BobguiTextTagTable` associated with this buffer.
 *
 * Returns: (transfer none): the buffer’s tag table
 **/
BobguiTextTagTable*
bobgui_text_buffer_get_tag_table (BobguiTextBuffer *buffer)
{
  g_return_val_if_fail (BOBGUI_IS_TEXT_BUFFER (buffer), NULL);

  return get_table (buffer);
}

/**
 * bobgui_text_buffer_set_text:
 * @buffer: a `BobguiTextBuffer`
 * @text: UTF-8 text to insert
 * @len: length of @text in bytes
 *
 * Deletes current contents of @buffer, and inserts @text instead. This is
 * automatically marked as an irreversible action in the undo stack. If you
 * wish to mark this action as part of a larger undo operation, call
 * [method@TextBuffer.delete] and [method@TextBuffer.insert] directly instead.
 *
 * If @len is -1, @text must be nul-terminated.
 * @text must be valid UTF-8.
 */
void
bobgui_text_buffer_set_text (BobguiTextBuffer *buffer,
                          const char    *text,
                          int            len)
{
  BobguiTextIter start, end;

  g_return_if_fail (BOBGUI_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (text != NULL);

  if (len < 0)
    len = strlen (text);

  bobgui_text_history_begin_irreversible_action (buffer->priv->history);

  bobgui_text_buffer_get_bounds (buffer, &start, &end);

  bobgui_text_buffer_delete (buffer, &start, &end);

  if (len > 0)
    {
      bobgui_text_buffer_get_iter_at_offset (buffer, &start, 0);
      bobgui_text_buffer_insert (buffer, &start, text, len);
    }

  bobgui_text_history_end_irreversible_action (buffer->priv->history);
}

/*
 * Insertion
 */

static void
bobgui_text_buffer_real_insert_text (BobguiTextBuffer *buffer,
                                  BobguiTextIter   *iter,
                                  const char    *text,
                                  int            len)
{
  g_return_if_fail (BOBGUI_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (iter != NULL);

  bobgui_text_history_text_inserted (buffer->priv->history,
                                  bobgui_text_iter_get_offset (iter),
                                  text,
                                  len);

  if (buffer->priv->commit_funcs == NULL)
    {
      _bobgui_text_btree_insert (iter, text, len);
    }
  else
    {
      guint position;
      guint n_chars;

      if (len < 0)
        len = strlen (text);

      position = bobgui_text_iter_get_offset (iter);
      n_chars = g_utf8_strlen (text, len);

      bobgui_text_buffer_commit_notify (buffer,
                                     BOBGUI_TEXT_BUFFER_NOTIFY_BEFORE_INSERT,
                                     position, n_chars);
      _bobgui_text_btree_insert (iter, text, len);
      bobgui_text_buffer_commit_notify (buffer,
                                     BOBGUI_TEXT_BUFFER_NOTIFY_AFTER_INSERT,
                                     position, n_chars);
    }

  g_signal_emit (buffer, signals[CHANGED], 0);
  g_object_notify_by_pspec (G_OBJECT (buffer), text_buffer_props[PROP_CURSOR_POSITION]);
}

static void
bobgui_text_buffer_emit_insert (BobguiTextBuffer *buffer,
                             BobguiTextIter   *iter,
                             const char    *text,
                             int            len)
{
  g_return_if_fail (BOBGUI_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (buffer->priv->in_commit_notify == FALSE);
  g_return_if_fail (iter != NULL);
  g_return_if_fail (text != NULL);

  if (len < 0)
    len = strlen (text);

  g_return_if_fail (g_utf8_validate (text, len, NULL));

  if (len > 0)
    {
      g_signal_emit (buffer, signals[INSERT_TEXT], 0,
                     iter, text, len);
    }
}

/**
 * bobgui_text_buffer_insert:
 * @buffer: a `BobguiTextBuffer`
 * @iter: a position in the buffer
 * @text: text in UTF-8 format
 * @len: length of text in bytes, or -1
 *
 * Inserts @len bytes of @text at position @iter.
 *
 * If @len is -1, @text must be nul-terminated and will be inserted in its
 * entirety. Emits the “insert-text” signal; insertion actually occurs
 * in the default handler for the signal. @iter is invalidated when
 * insertion occurs (because the buffer contents change), but the
 * default signal handler revalidates it to point to the end of the
 * inserted text.
 */
void
bobgui_text_buffer_insert (BobguiTextBuffer *buffer,
                        BobguiTextIter   *iter,
                        const char    *text,
                        int            len)
{
  g_return_if_fail (BOBGUI_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (iter != NULL);
  g_return_if_fail (text != NULL);
  g_return_if_fail (bobgui_text_iter_get_buffer (iter) == buffer);

  bobgui_text_buffer_emit_insert (buffer, iter, text, len);
}

/**
 * bobgui_text_buffer_insert_at_cursor:
 * @buffer: a `BobguiTextBuffer`
 * @text: text in UTF-8 format
 * @len: length of text, in bytes
 *
 * Inserts @text in @buffer.
 *
 * Simply calls [method@Bobgui.TextBuffer.insert],
 * using the current cursor position as the insertion point.
 */
void
bobgui_text_buffer_insert_at_cursor (BobguiTextBuffer *buffer,
                                  const char    *text,
                                  int            len)
{
  BobguiTextIter iter;

  g_return_if_fail (BOBGUI_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (text != NULL);

  bobgui_text_buffer_get_iter_at_mark (buffer, &iter,
                                    bobgui_text_buffer_get_insert (buffer));

  bobgui_text_buffer_insert (buffer, &iter, text, len);
}

/**
 * bobgui_text_buffer_insert_interactive:
 * @buffer: a `BobguiTextBuffer`
 * @iter: a position in @buffer
 * @text: some UTF-8 text
 * @len: length of text in bytes, or -1
 * @default_editable: default editability of buffer
 *
 * Inserts @text in @buffer.
 *
 * Like [method@Bobgui.TextBuffer.insert], but the insertion will not occur
 * if @iter is at a non-editable location in the buffer. Usually you
 * want to prevent insertions at ineditable locations if the insertion
 * results from a user action (is interactive).
 *
 * @default_editable indicates the editability of text that doesn't
 * have a tag affecting editability applied to it. Typically the
 * result of [method@Bobgui.TextView.get_editable] is appropriate here.
 *
 * Returns: whether text was actually inserted
 */
gboolean
bobgui_text_buffer_insert_interactive (BobguiTextBuffer *buffer,
                                    BobguiTextIter   *iter,
                                    const char    *text,
                                    int            len,
                                    gboolean       default_editable)
{
  g_return_val_if_fail (BOBGUI_IS_TEXT_BUFFER (buffer), FALSE);
  g_return_val_if_fail (text != NULL, FALSE);
  g_return_val_if_fail (bobgui_text_iter_get_buffer (iter) == buffer, FALSE);

  if (bobgui_text_iter_can_insert (iter, default_editable))
    {
      bobgui_text_buffer_begin_user_action (buffer);
      bobgui_text_buffer_emit_insert (buffer, iter, text, len);
      bobgui_text_buffer_end_user_action (buffer);
      return TRUE;
    }
  else
    return FALSE;
}

/**
 * bobgui_text_buffer_insert_interactive_at_cursor:
 * @buffer: a `BobguiTextBuffer`
 * @text: text in UTF-8 format
 * @len: length of text in bytes, or -1
 * @default_editable: default editability of buffer
 *
 * Inserts @text in @buffer.
 *
 * Calls [method@Bobgui.TextBuffer.insert_interactive]
 * at the cursor position.
 *
 * @default_editable indicates the editability of text that doesn't
 * have a tag affecting editability applied to it. Typically the
 * result of [method@Bobgui.TextView.get_editable] is appropriate here.
 *
 * Returns: whether text was actually inserted
 */
gboolean
bobgui_text_buffer_insert_interactive_at_cursor (BobguiTextBuffer *buffer,
                                              const char    *text,
                                              int            len,
                                              gboolean       default_editable)
{
  BobguiTextIter iter;

  g_return_val_if_fail (BOBGUI_IS_TEXT_BUFFER (buffer), FALSE);
  g_return_val_if_fail (text != NULL, FALSE);

  bobgui_text_buffer_get_iter_at_mark (buffer, &iter,
                                    bobgui_text_buffer_get_insert (buffer));

  return bobgui_text_buffer_insert_interactive (buffer, &iter, text, len,
                                             default_editable);
}

static gboolean
possibly_not_text (gunichar ch,
                   gpointer user_data)
{
  return ch == BOBGUI_TEXT_UNKNOWN_CHAR;
}

static void
insert_text_range (BobguiTextBuffer     *buffer,
                   BobguiTextIter       *iter,
                   const BobguiTextIter *orig_start,
                   const BobguiTextIter *orig_end,
                   gboolean           interactive)
{
  char *text;

  text = bobgui_text_iter_get_text (orig_start, orig_end);

  bobgui_text_buffer_emit_insert (buffer, iter, text, -1);

  g_free (text);
}

typedef struct _Range Range;
struct _Range
{
  BobguiTextBuffer *buffer;
  BobguiTextMark *start_mark;
  BobguiTextMark *end_mark;
  BobguiTextMark *whole_end_mark;
  BobguiTextIter *range_start;
  BobguiTextIter *range_end;
  BobguiTextIter *whole_end;
};

static Range*
save_range (BobguiTextIter *range_start,
            BobguiTextIter *range_end,
            BobguiTextIter *whole_end)
{
  Range *r;

  r = g_new (Range, 1);

  r->buffer = bobgui_text_iter_get_buffer (range_start);
  g_object_ref (r->buffer);

  r->start_mark =
    bobgui_text_buffer_create_mark (bobgui_text_iter_get_buffer (range_start),
                                 NULL,
                                 range_start,
                                 FALSE);
  r->end_mark =
    bobgui_text_buffer_create_mark (bobgui_text_iter_get_buffer (range_start),
                                 NULL,
                                 range_end,
                                 TRUE);

  r->whole_end_mark =
    bobgui_text_buffer_create_mark (bobgui_text_iter_get_buffer (range_start),
                                 NULL,
                                 whole_end,
                                 TRUE);

  r->range_start = range_start;
  r->range_end = range_end;
  r->whole_end = whole_end;

  return r;
}

static void
restore_range (Range *r)
{
  bobgui_text_buffer_get_iter_at_mark (r->buffer,
                                    r->range_start,
                                    r->start_mark);

  bobgui_text_buffer_get_iter_at_mark (r->buffer,
                                    r->range_end,
                                    r->end_mark);

  bobgui_text_buffer_get_iter_at_mark (r->buffer,
                                    r->whole_end,
                                    r->whole_end_mark);

  bobgui_text_buffer_delete_mark (r->buffer, r->start_mark);
  bobgui_text_buffer_delete_mark (r->buffer, r->end_mark);
  bobgui_text_buffer_delete_mark (r->buffer, r->whole_end_mark);

  /* Due to the gravities on the marks, the ordering could have
   * gotten mangled; we switch to an empty range in that
   * case
   */

  if (bobgui_text_iter_compare (r->range_start, r->range_end) > 0)
    *r->range_start = *r->range_end;

  if (bobgui_text_iter_compare (r->range_end, r->whole_end) > 0)
    *r->range_end = *r->whole_end;

  g_object_unref (r->buffer);
  g_free (r);
}

static void
insert_range_untagged (BobguiTextBuffer     *buffer,
                       BobguiTextIter       *iter,
                       const BobguiTextIter *orig_start,
                       const BobguiTextIter *orig_end,
                       gboolean           interactive)
{
  BobguiTextIter range_start;
  BobguiTextIter range_end;
  BobguiTextIter start, end;
  Range *r;

  if (bobgui_text_iter_equal (orig_start, orig_end))
    return;

  start = *orig_start;
  end = *orig_end;

  range_start = start;
  range_end = start;

  while (TRUE)
    {
      if (bobgui_text_iter_equal (&range_start, &range_end))
        {
          /* Figure out how to move forward */

          g_assert (bobgui_text_iter_compare (&range_end, &end) <= 0);

          if (bobgui_text_iter_equal (&range_end, &end))
            {
              /* nothing left to do */
              break;
            }
          else if (bobgui_text_iter_get_char (&range_end) == BOBGUI_TEXT_UNKNOWN_CHAR)
            {
              GdkPaintable *paintable;
              BobguiTextChildAnchor *anchor;

              paintable = bobgui_text_iter_get_paintable (&range_end);
              anchor = bobgui_text_iter_get_child_anchor (&range_end);

              if (paintable)
                {
                  r = save_range (&range_start,
                                  &range_end,
                                  &end);

                  bobgui_text_buffer_insert_paintable (buffer, iter, paintable);

                  restore_range (r);
                  r = NULL;

                  bobgui_text_iter_forward_char (&range_end);

                  range_start = range_end;
                }
              else if (anchor)
                {
                  /* Just skip anchors */

                  bobgui_text_iter_forward_char (&range_end);
                  range_start = range_end;
                }
              else
                {
                  /* The BOBGUI_TEXT_UNKNOWN_CHAR was in a text segment, so
                   * keep going.
                   */
                  bobgui_text_iter_forward_find_char (&range_end,
                                                   possibly_not_text, NULL,
                                                   &end);

                  g_assert (bobgui_text_iter_compare (&range_end, &end) <= 0);
                }
            }
          else
            {
              /* Text segment starts here, so forward search to
               * find its possible endpoint
               */
              bobgui_text_iter_forward_find_char (&range_end,
                                               possibly_not_text, NULL,
                                               &end);

              g_assert (bobgui_text_iter_compare (&range_end, &end) <= 0);
            }
        }
      else
        {
          r = save_range (&range_start,
                          &range_end,
                          &end);

          insert_text_range (buffer,
                             iter,
                             &range_start,
                             &range_end,
                             interactive);

          restore_range (r);
          r = NULL;

          range_start = range_end;
        }
    }
}

static void
insert_range_not_inside_self (BobguiTextBuffer     *buffer,
                              BobguiTextIter       *iter,
                              const BobguiTextIter *orig_start,
                              const BobguiTextIter *orig_end,
                              gboolean           interactive)
{
  /* Find each range of uniformly-tagged text, insert it,
   * then apply the tags.
   */
  BobguiTextIter start = *orig_start;
  BobguiTextIter end = *orig_end;
  BobguiTextIter range_start;
  BobguiTextIter range_end;
  gboolean insert_tags;

  if (bobgui_text_iter_equal (orig_start, orig_end))
    return;

  insert_tags = bobgui_text_buffer_get_tag_table (bobgui_text_iter_get_buffer (orig_start))
                == bobgui_text_buffer_get_tag_table (buffer);

  bobgui_text_iter_order (&start, &end);

  range_start = start;
  range_end = start;

  while (TRUE)
    {
      int start_offset;
      BobguiTextIter start_iter;
      GSList *tags;
      GSList *tmp_list;
      Range *r;

      if (bobgui_text_iter_equal (&range_start, &end))
        break; /* All done */

      g_assert (bobgui_text_iter_compare (&range_start, &end) < 0);

      bobgui_text_iter_forward_to_tag_toggle (&range_end, NULL);

      g_assert (!bobgui_text_iter_equal (&range_start, &range_end));

      /* Clamp to the end iterator */
      if (bobgui_text_iter_compare (&range_end, &end) > 0)
        range_end = end;

      /* We have a range with unique tags; insert it, and
       * apply all tags.
       */
      start_offset = bobgui_text_iter_get_offset (iter);

      r = save_range (&range_start, &range_end, &end);

      insert_range_untagged (buffer, iter, &range_start, &range_end, interactive);

      restore_range (r);
      r = NULL;

      if (insert_tags)
        {
          bobgui_text_buffer_get_iter_at_offset (buffer, &start_iter, start_offset);

          tags = bobgui_text_iter_get_tags (&range_start);
          tmp_list = tags;
          while (tmp_list != NULL)
            {
              bobgui_text_buffer_apply_tag (buffer, tmp_list->data, &start_iter, iter);
              tmp_list = tmp_list->next;
            }
          g_slist_free (tags);
        }

      range_start = range_end;
    }
}

static void
bobgui_text_buffer_real_insert_range (BobguiTextBuffer     *buffer,
                                   BobguiTextIter       *iter,
                                   const BobguiTextIter *orig_start,
                                   const BobguiTextIter *orig_end,
                                   gboolean           interactive)
{
  BobguiTextBuffer *src_buffer;

  /* Find each range of uniformly-tagged text, insert it,
   * then apply the tags.
   */
  if (bobgui_text_iter_equal (orig_start, orig_end))
    return;

  if (interactive)
    bobgui_text_buffer_begin_user_action (buffer);

  src_buffer = bobgui_text_iter_get_buffer (orig_start);

  if (bobgui_text_iter_get_buffer (iter) != src_buffer ||
      !bobgui_text_iter_in_range (iter, orig_start, orig_end))
    {
      insert_range_not_inside_self (buffer, iter, orig_start, orig_end, interactive);
    }
  else
    {
      /* If you insert a range into itself, it could loop infinitely
       * because the region being copied keeps growing as we insert. So
       * we have to separately copy the range before and after
       * the insertion point.
       */
      BobguiTextIter start = *orig_start;
      BobguiTextIter end = *orig_end;
      BobguiTextIter range_start;
      BobguiTextIter range_end;
      Range *first_half;
      Range *second_half;

      bobgui_text_iter_order (&start, &end);

      range_start = start;
      range_end = *iter;
      first_half = save_range (&range_start, &range_end, &end);

      range_start = *iter;
      range_end = end;
      second_half = save_range (&range_start, &range_end, &end);

      restore_range (first_half);
      insert_range_not_inside_self (buffer, iter, &range_start, &range_end, interactive);

      restore_range (second_half);
      insert_range_not_inside_self (buffer, iter, &range_start, &range_end, interactive);
    }

  if (interactive)
    bobgui_text_buffer_end_user_action (buffer);
}

/**
 * bobgui_text_buffer_insert_range:
 * @buffer: a `BobguiTextBuffer`
 * @iter: a position in @buffer
 * @start: a position in a `BobguiTextBuffer`
 * @end: another position in the same buffer as @start
 *
 * Copies text, tags, and paintables between @start and @end
 * and inserts the copy at @iter.
 *
 * The order of @start and @end doesn’t matter.
 *
 * Used instead of simply getting/inserting text because it preserves
 * images and tags. If @start and @end are in a different buffer from
 * @buffer, the two buffers must share the same tag table.
 *
 * Implemented via emissions of the ::insert-text and ::apply-tag signals,
 * so expect those.
 */
void
bobgui_text_buffer_insert_range (BobguiTextBuffer     *buffer,
                              BobguiTextIter       *iter,
                              const BobguiTextIter *start,
                              const BobguiTextIter *end)
{
  g_return_if_fail (BOBGUI_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (iter != NULL);
  g_return_if_fail (start != NULL);
  g_return_if_fail (end != NULL);
  g_return_if_fail (bobgui_text_iter_get_buffer (start) ==
                    bobgui_text_iter_get_buffer (end));
  g_return_if_fail (bobgui_text_iter_get_buffer (start)->priv->tag_table ==
                    buffer->priv->tag_table);
  g_return_if_fail (bobgui_text_iter_get_buffer (iter) == buffer);

  bobgui_text_buffer_real_insert_range (buffer, iter, start, end, FALSE);
}

/**
 * bobgui_text_buffer_insert_range_interactive:
 * @buffer: a `BobguiTextBuffer`
 * @iter: a position in @buffer
 * @start: a position in a `BobguiTextBuffer`
 * @end: another position in the same buffer as @start
 * @default_editable: default editability of the buffer
 *
 * Copies text, tags, and paintables between @start and @end
 * and inserts the copy at @iter.
 *
 * Same as [method@Bobgui.TextBuffer.insert_range], but does nothing
 * if the insertion point isn’t editable. The @default_editable
 * parameter indicates whether the text is editable at @iter if
 * no tags enclosing @iter affect editability. Typically the result
 * of [method@Bobgui.TextView.get_editable] is appropriate here.
 *
 * Returns: whether an insertion was possible at @iter
 */
gboolean
bobgui_text_buffer_insert_range_interactive (BobguiTextBuffer     *buffer,
                                          BobguiTextIter       *iter,
                                          const BobguiTextIter *start,
                                          const BobguiTextIter *end,
                                          gboolean           default_editable)
{
  g_return_val_if_fail (BOBGUI_IS_TEXT_BUFFER (buffer), FALSE);
  g_return_val_if_fail (iter != NULL, FALSE);
  g_return_val_if_fail (start != NULL, FALSE);
  g_return_val_if_fail (end != NULL, FALSE);
  g_return_val_if_fail (bobgui_text_iter_get_buffer (start) ==
                        bobgui_text_iter_get_buffer (end), FALSE);
  g_return_val_if_fail (bobgui_text_iter_get_buffer (start)->priv->tag_table ==
                        buffer->priv->tag_table, FALSE);

  if (bobgui_text_iter_can_insert (iter, default_editable))
    {
      bobgui_text_buffer_real_insert_range (buffer, iter, start, end, TRUE);
      return TRUE;
    }
  else
    return FALSE;
}

/**
 * bobgui_text_buffer_insert_with_tags:
 * @buffer: a `BobguiTextBuffer`
 * @iter: an iterator in @buffer
 * @text: UTF-8 text
 * @len: length of @text, or -1
 * @first_tag: first tag to apply to @text
 * @...: %NULL-terminated list of tags to apply
 *
 * Inserts @text into @buffer at @iter, applying the list of tags to
 * the newly-inserted text.
 *
 * The last tag specified must be %NULL to terminate the list.
 * Equivalent to calling [method@Bobgui.TextBuffer.insert],
 * then [method@Bobgui.TextBuffer.apply_tag] on the inserted text;
 * this is just a convenience function.
 */
void
bobgui_text_buffer_insert_with_tags (BobguiTextBuffer *buffer,
                                  BobguiTextIter   *iter,
                                  const char    *text,
                                  int            len,
                                  BobguiTextTag    *first_tag,
                                  ...)
{
  int start_offset;
  BobguiTextIter start;
  va_list args;
  BobguiTextTag *tag;

  g_return_if_fail (BOBGUI_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (iter != NULL);
  g_return_if_fail (text != NULL);
  g_return_if_fail (bobgui_text_iter_get_buffer (iter) == buffer);

  start_offset = bobgui_text_iter_get_offset (iter);

  bobgui_text_buffer_insert (buffer, iter, text, len);

  if (first_tag == NULL)
    return;

  bobgui_text_buffer_get_iter_at_offset (buffer, &start, start_offset);

  va_start (args, first_tag);
  tag = first_tag;
  while (tag)
    {
      bobgui_text_buffer_apply_tag (buffer, tag, &start, iter);

      tag = va_arg (args, BobguiTextTag*);
    }

  va_end (args);
}

/**
 * bobgui_text_buffer_insert_with_tags_by_name:
 * @buffer: a `BobguiTextBuffer`
 * @iter: position in @buffer
 * @text: UTF-8 text
 * @len: length of @text, or -1
 * @first_tag_name: name of a tag to apply to @text
 * @...: more tag names
 *
 * Inserts @text into @buffer at @iter, applying the list of tags to
 * the newly-inserted text.
 *
 * Same as [method@Bobgui.TextBuffer.insert_with_tags], but allows you
 * to pass in tag names instead of tag objects.
 */
void
bobgui_text_buffer_insert_with_tags_by_name  (BobguiTextBuffer *buffer,
                                           BobguiTextIter   *iter,
                                           const char    *text,
                                           int            len,
                                           const char    *first_tag_name,
                                           ...)
{
  int start_offset;
  BobguiTextIter start;
  va_list args;
  const char *tag_name;

  g_return_if_fail (BOBGUI_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (iter != NULL);
  g_return_if_fail (text != NULL);
  g_return_if_fail (bobgui_text_iter_get_buffer (iter) == buffer);

  start_offset = bobgui_text_iter_get_offset (iter);

  bobgui_text_buffer_insert (buffer, iter, text, len);

  if (first_tag_name == NULL)
    return;

  bobgui_text_buffer_get_iter_at_offset (buffer, &start, start_offset);

  va_start (args, first_tag_name);
  tag_name = first_tag_name;
  while (tag_name)
    {
      BobguiTextTag *tag;

      tag = bobgui_text_tag_table_lookup (buffer->priv->tag_table,
                                       tag_name);

      if (tag == NULL)
        {
          g_warning ("%s: no tag with name '%s'!", G_STRLOC, tag_name);
          va_end (args);
          return;
        }

      bobgui_text_buffer_apply_tag (buffer, tag, &start, iter);

      tag_name = va_arg (args, const char *);
    }

  va_end (args);
}


/*
 * Deletion
 */

static void
bobgui_text_buffer_real_delete_range (BobguiTextBuffer *buffer,
                                   BobguiTextIter   *start,
                                   BobguiTextIter   *end)
{
  gboolean has_selection;

  g_return_if_fail (BOBGUI_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (start != NULL);
  g_return_if_fail (end != NULL);

  if (bobgui_text_history_get_enabled (buffer->priv->history))
    {
      BobguiTextIter sel_begin, sel_end;
      char *text;

      if (bobgui_text_buffer_get_selection_bounds (buffer, &sel_begin, &sel_end))
        bobgui_text_history_selection_changed (buffer->priv->history,
                                            bobgui_text_iter_get_offset (&sel_begin),
                                            bobgui_text_iter_get_offset (&sel_end));
      else
        bobgui_text_history_selection_changed (buffer->priv->history,
                                            bobgui_text_iter_get_offset (&sel_begin),
                                            -1);

      text = bobgui_text_iter_get_slice (start, end);
      bobgui_text_history_text_deleted (buffer->priv->history,
                                     bobgui_text_iter_get_offset (start),
                                     bobgui_text_iter_get_offset (end),
                                     text, -1);
      g_free (text);
    }



  if (buffer->priv->commit_funcs == NULL)
    {
      _bobgui_text_btree_delete (start, end);
    }
  else
    {
      guint off1 = bobgui_text_iter_get_offset (start);
      guint off2 = bobgui_text_iter_get_offset (end);

      if (off2 < off1)
        {
          guint tmp = off1;
          off1 = off2;
          off2 = tmp;
        }

      buffer->priv->in_commit_notify = TRUE;

      bobgui_text_buffer_commit_notify (buffer,
                                     BOBGUI_TEXT_BUFFER_NOTIFY_BEFORE_DELETE,
                                     off1, off2 - off1);
      _bobgui_text_btree_delete (start, end);
      bobgui_text_buffer_commit_notify (buffer,
                                     BOBGUI_TEXT_BUFFER_NOTIFY_AFTER_DELETE,
                                     off1, 0);

      buffer->priv->in_commit_notify = FALSE;
    }

  /* may have deleted the selection... */
  update_selection_clipboards (buffer);

  has_selection = bobgui_text_buffer_get_selection_bounds (buffer, NULL, NULL);
  if (has_selection != buffer->priv->has_selection)
    {
      buffer->priv->has_selection = has_selection;
      g_object_notify_by_pspec (G_OBJECT (buffer), text_buffer_props[PROP_HAS_SELECTION]);
    }

  g_signal_emit (buffer, signals[CHANGED], 0);
  g_object_notify_by_pspec (G_OBJECT (buffer), text_buffer_props[PROP_CURSOR_POSITION]);
}

static void
bobgui_text_buffer_emit_delete (BobguiTextBuffer *buffer,
                             BobguiTextIter *start,
                             BobguiTextIter *end)
{
  g_return_if_fail (BOBGUI_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (start != NULL);
  g_return_if_fail (end != NULL);
  g_return_if_fail (buffer->priv->in_commit_notify == FALSE);

  if (bobgui_text_iter_equal (start, end))
    return;

  bobgui_text_iter_order (start, end);

  g_signal_emit (buffer,
                 signals[DELETE_RANGE],
                 0,
                 start, end);
}

/**
 * bobgui_text_buffer_delete:
 * @buffer: a `BobguiTextBuffer`
 * @start: a position in @buffer
 * @end: another position in @buffer
 *
 * Deletes text between @start and @end.
 *
 * The order of @start and @end is not actually relevant;
 * bobgui_text_buffer_delete() will reorder them.
 *
 * This function actually emits the “delete-range” signal, and
 * the default handler of that signal deletes the text. Because the
 * buffer is modified, all outstanding iterators become invalid after
 * calling this function; however, the @start and @end will be
 * re-initialized to point to the location where text was deleted.
 */
void
bobgui_text_buffer_delete (BobguiTextBuffer *buffer,
                        BobguiTextIter   *start,
                        BobguiTextIter   *end)
{
  g_return_if_fail (BOBGUI_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (start != NULL);
  g_return_if_fail (end != NULL);
  g_return_if_fail (bobgui_text_iter_get_buffer (start) == buffer);
  g_return_if_fail (bobgui_text_iter_get_buffer (end) == buffer);

  bobgui_text_buffer_emit_delete (buffer, start, end);
}

/**
 * bobgui_text_buffer_delete_interactive:
 * @buffer: a `BobguiTextBuffer`
 * @start_iter: start of range to delete
 * @end_iter: end of range
 * @default_editable: whether the buffer is editable by default
 *
 * Deletes all editable text in the given range.
 *
 * Calls [method@Bobgui.TextBuffer.delete] for each editable
 * sub-range of [@start,@end). @start and @end are revalidated
 * to point to the location of the last deleted range, or left
 * untouched if no text was deleted.
 *
 * Returns: whether some text was actually deleted
 */
gboolean
bobgui_text_buffer_delete_interactive (BobguiTextBuffer *buffer,
                                    BobguiTextIter   *start_iter,
                                    BobguiTextIter   *end_iter,
                                    gboolean       default_editable)
{
  BobguiTextMark *end_mark;
  BobguiTextMark *start_mark;
  BobguiTextIter iter;
  gboolean current_state;
  gboolean deleted_stuff = FALSE;

  /* Delete all editable text in the range start_iter, end_iter */

  g_return_val_if_fail (BOBGUI_IS_TEXT_BUFFER (buffer), FALSE);
  g_return_val_if_fail (start_iter != NULL, FALSE);
  g_return_val_if_fail (end_iter != NULL, FALSE);
  g_return_val_if_fail (bobgui_text_iter_get_buffer (start_iter) == buffer, FALSE);
  g_return_val_if_fail (bobgui_text_iter_get_buffer (end_iter) == buffer, FALSE);


  bobgui_text_buffer_begin_user_action (buffer);

  bobgui_text_iter_order (start_iter, end_iter);

  start_mark = bobgui_text_buffer_create_mark (buffer, NULL,
                                            start_iter, TRUE);
  end_mark = bobgui_text_buffer_create_mark (buffer, NULL,
                                          end_iter, FALSE);

  bobgui_text_buffer_get_iter_at_mark (buffer, &iter, start_mark);

  current_state = bobgui_text_iter_editable (&iter, default_editable);

  while (TRUE)
    {
      gboolean new_state;
      gboolean done = FALSE;
      BobguiTextIter end;

      bobgui_text_iter_forward_to_tag_toggle (&iter, NULL);

      bobgui_text_buffer_get_iter_at_mark (buffer, &end, end_mark);

      if (bobgui_text_iter_compare (&iter, &end) >= 0)
        {
          done = TRUE;
          iter = end; /* clamp to the last boundary */
        }

      new_state = bobgui_text_iter_editable (&iter, default_editable);

      if (current_state == new_state)
        {
          if (done)
            {
              if (current_state)
                {
                  /* We're ending an editable region. Delete said region. */
                  BobguiTextIter start;

                  bobgui_text_buffer_get_iter_at_mark (buffer, &start, start_mark);

                  bobgui_text_buffer_emit_delete (buffer, &start, &iter);

                  deleted_stuff = TRUE;

                  /* revalidate user's iterators. */
                  *start_iter = start;
                  *end_iter = iter;
                }

              break;
            }
          else
            continue;
        }

      if (current_state && !new_state)
        {
          /* End of an editable region. Delete it. */
          BobguiTextIter start;

          bobgui_text_buffer_get_iter_at_mark (buffer, &start, start_mark);

          bobgui_text_buffer_emit_delete (buffer, &start, &iter);

	  /* It's more robust to ask for the state again then to assume that
	   * we're on the next not-editable segment. We don't know what the
	   * ::delete-range handler did.... maybe it deleted the following
           * not-editable segment because it was associated with the editable
           * segment.
	   */
	  current_state = bobgui_text_iter_editable (&iter, default_editable);
          deleted_stuff = TRUE;

          /* revalidate user's iterators. */
          *start_iter = start;
          *end_iter = iter;
        }
      else
        {
          /* We are at the start of an editable region. We won't be deleting
           * the previous region. Move start mark to start of this region.
           */

          g_assert (!current_state && new_state);

          bobgui_text_buffer_move_mark (buffer, start_mark, &iter);

          current_state = TRUE;
        }

      if (done)
        break;
    }

  bobgui_text_buffer_delete_mark (buffer, start_mark);
  bobgui_text_buffer_delete_mark (buffer, end_mark);

  bobgui_text_buffer_end_user_action (buffer);

  return deleted_stuff;
}

/*
 * Extracting textual buffer contents
 */

/**
 * bobgui_text_buffer_get_text:
 * @buffer: a `BobguiTextBuffer`
 * @start: start of a range
 * @end: end of a range
 * @include_hidden_chars: whether to include invisible text
 *
 * Returns the text in the range [@start,@end).
 *
 * Excludes undisplayed text (text marked with tags that set the
 * invisibility attribute) if @include_hidden_chars is %FALSE.
 * Does not include characters representing embedded images, so
 * byte and character indexes into the returned string do not
 * correspond to byte and character indexes into the buffer.
 * Contrast with [method@Bobgui.TextBuffer.get_slice].
 *
 * Returns: (transfer full): an allocated UTF-8 string
 **/
char *
bobgui_text_buffer_get_text (BobguiTextBuffer     *buffer,
                          const BobguiTextIter *start,
                          const BobguiTextIter *end,
                          gboolean           include_hidden_chars)
{
  g_return_val_if_fail (BOBGUI_IS_TEXT_BUFFER (buffer), NULL);
  g_return_val_if_fail (start != NULL, NULL);
  g_return_val_if_fail (end != NULL, NULL);
  g_return_val_if_fail (bobgui_text_iter_get_buffer (start) == buffer, NULL);
  g_return_val_if_fail (bobgui_text_iter_get_buffer (end) == buffer, NULL);

  if (include_hidden_chars)
    return bobgui_text_iter_get_text (start, end);
  else
    return bobgui_text_iter_get_visible_text (start, end);
}

/**
 * bobgui_text_buffer_get_slice:
 * @buffer: a `BobguiTextBuffer`
 * @start: start of a range
 * @end: end of a range
 * @include_hidden_chars: whether to include invisible text
 *
 * Returns the text in the range [@start,@end).
 *
 * Excludes undisplayed text (text marked with tags that set the
 * invisibility attribute) if @include_hidden_chars is %FALSE.
 * The returned string includes a 0xFFFC character whenever the
 * buffer contains embedded images, so byte and character indexes
 * into the returned string do correspond to byte and character
 * indexes into the buffer. Contrast with [method@Bobgui.TextBuffer.get_text].
 * Note that 0xFFFC can occur in normal text as well, so it is not a
 * reliable indicator that a paintable or widget is in the buffer.
 *
 * Returns: (transfer full): an allocated UTF-8 string
 */
char *
bobgui_text_buffer_get_slice (BobguiTextBuffer     *buffer,
                           const BobguiTextIter *start,
                           const BobguiTextIter *end,
                           gboolean           include_hidden_chars)
{
  g_return_val_if_fail (BOBGUI_IS_TEXT_BUFFER (buffer), NULL);
  g_return_val_if_fail (start != NULL, NULL);
  g_return_val_if_fail (end != NULL, NULL);
  g_return_val_if_fail (bobgui_text_iter_get_buffer (start) == buffer, NULL);
  g_return_val_if_fail (bobgui_text_iter_get_buffer (end) == buffer, NULL);

  if (include_hidden_chars)
    return bobgui_text_iter_get_slice (start, end);
  else
    return bobgui_text_iter_get_visible_slice (start, end);
}

/*
 * Pixbufs
 */

static void
bobgui_text_buffer_real_insert_paintable (BobguiTextBuffer *buffer,
                                       BobguiTextIter   *iter,
                                       GdkPaintable  *paintable)
{
  _bobgui_text_btree_insert_paintable (iter, paintable);

  g_signal_emit (buffer, signals[CHANGED], 0);
}

/**
 * bobgui_text_buffer_insert_paintable:
 * @buffer: a `BobguiTextBuffer`
 * @iter: location to insert the paintable
 * @paintable: a `GdkPaintable`
 *
 * Inserts an image into the text buffer at @iter.
 *
 * The image will be counted as one character in character counts,
 * and when obtaining the buffer contents as a string, will be
 * represented by the Unicode “object replacement character” 0xFFFC.
 * Note that the “slice” variants for obtaining portions of the buffer
 * as a string include this character for paintable, but the “text”
 * variants do not. e.g. see [method@Bobgui.TextBuffer.get_slice] and
 * [method@Bobgui.TextBuffer.get_text].
 */
void
bobgui_text_buffer_insert_paintable (BobguiTextBuffer *buffer,
                                  BobguiTextIter   *iter,
                                  GdkPaintable  *paintable)
{
  g_return_if_fail (BOBGUI_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (iter != NULL);
  g_return_if_fail (GDK_IS_PAINTABLE (paintable));
  g_return_if_fail (bobgui_text_iter_get_buffer (iter) == buffer);

  g_signal_emit (buffer, signals[INSERT_PAINTABLE], 0, iter, paintable);
}

/*
 * Child anchor
 */


static void
bobgui_text_buffer_real_insert_anchor (BobguiTextBuffer      *buffer,
                                    BobguiTextIter        *iter,
                                    BobguiTextChildAnchor *anchor)
{
  _bobgui_text_btree_insert_child_anchor (iter, anchor);

  g_signal_emit (buffer, signals[CHANGED], 0);
}

/**
 * bobgui_text_buffer_insert_child_anchor:
 * @buffer: a `BobguiTextBuffer`
 * @iter: location to insert the anchor
 * @anchor: a `BobguiTextChildAnchor`
 *
 * Inserts a child widget anchor into the text buffer at @iter.
 *
 * The anchor will be counted as one character in character counts, and
 * when obtaining the buffer contents as a string, will be represented
 * by the Unicode “object replacement character” 0xFFFC. Note that the
 * “slice” variants for obtaining portions of the buffer as a string
 * include this character for child anchors, but the “text” variants do
 * not. E.g. see [method@Bobgui.TextBuffer.get_slice] and
 * [method@Bobgui.TextBuffer.get_text].
 *
 * Consider [method@Bobgui.TextBuffer.create_child_anchor] as a more
 * convenient alternative to this function. The buffer will add a
 * reference to the anchor, so you can unref it after insertion.
 **/
void
bobgui_text_buffer_insert_child_anchor (BobguiTextBuffer      *buffer,
                                     BobguiTextIter        *iter,
                                     BobguiTextChildAnchor *anchor)
{
  g_return_if_fail (BOBGUI_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (iter != NULL);
  g_return_if_fail (BOBGUI_IS_TEXT_CHILD_ANCHOR (anchor));
  g_return_if_fail (bobgui_text_iter_get_buffer (iter) == buffer);

  g_signal_emit (buffer, signals[INSERT_CHILD_ANCHOR], 0,
                 iter, anchor);
}

/**
 * bobgui_text_buffer_create_child_anchor:
 * @buffer: a `BobguiTextBuffer`
 * @iter: location in the buffer
 *
 * Creates and inserts a child anchor.
 *
 * This is a convenience function which simply creates a child anchor
 * with [ctor@Bobgui.TextChildAnchor.new] and inserts it into the buffer
 * with [method@Bobgui.TextBuffer.insert_child_anchor].
 *
 * The new anchor is owned by the buffer; no reference count is
 * returned to the caller of this function.
 *
 * Returns: (transfer none): the created child anchor
 */
BobguiTextChildAnchor*
bobgui_text_buffer_create_child_anchor (BobguiTextBuffer *buffer,
                                     BobguiTextIter   *iter)
{
  BobguiTextChildAnchor *anchor;

  g_return_val_if_fail (BOBGUI_IS_TEXT_BUFFER (buffer), NULL);
  g_return_val_if_fail (iter != NULL, NULL);
  g_return_val_if_fail (bobgui_text_iter_get_buffer (iter) == buffer, NULL);

  anchor = bobgui_text_child_anchor_new ();

  bobgui_text_buffer_insert_child_anchor (buffer, iter, anchor);

  g_object_unref (anchor);

  return anchor;
}

/*
 * Mark manipulation
 */

static void
bobgui_text_buffer_mark_set (BobguiTextBuffer     *buffer,
                          const BobguiTextIter *location,
                          BobguiTextMark       *mark)
{
  /* IMO this should NOT work like insert_text and delete_range,
   * where the real action happens in the default handler.
   *
   * The reason is that the default handler would be _required_,
   * i.e. the whole widget would start breaking and segfaulting if the
   * default handler didn't get run. So you can't really override the
   * default handler or stop the emission; that is, this signal is
   * purely for notification, and not to allow users to modify the
   * default behavior.
   */

  g_object_ref (mark);

  g_signal_emit (buffer,
                 signals[MARK_SET],
                 0,
                 location,
                 mark);

  g_object_unref (mark);
}

/**
 * bobgui_text_buffer_set_mark:
 * @buffer: a `BobguiTextBuffer`
 * @mark_name: name of the mark
 * @iter: location for the mark
 * @left_gravity: if the mark is created by this function, gravity for
 *   the new mark
 * @should_exist: if %TRUE, warn if the mark does not exist, and return
 *   immediately
 *
 * Move the mark to the given position.
 *
 * If not @should_exist, create the mark.
 *
 * Returns: mark
 */
static BobguiTextMark*
bobgui_text_buffer_set_mark (BobguiTextBuffer     *buffer,
                          BobguiTextMark       *existing_mark,
                          const char        *mark_name,
                          const BobguiTextIter *iter,
                          gboolean           left_gravity,
                          gboolean           should_exist)
{
  BobguiTextIter location;
  BobguiTextMark *mark;

  g_return_val_if_fail (bobgui_text_iter_get_buffer (iter) == buffer, NULL);

  mark = _bobgui_text_btree_set_mark (get_btree (buffer),
                                   existing_mark,
                                   mark_name,
                                   left_gravity,
                                   iter,
                                   should_exist);

  _bobgui_text_btree_get_iter_at_mark (get_btree (buffer),
                                   &location,
                                   mark);

  bobgui_text_buffer_mark_set (buffer, &location, mark);

  return mark;
}

/**
 * bobgui_text_buffer_create_mark:
 * @buffer: a `BobguiTextBuffer`
 * @mark_name: (nullable): name for mark
 * @where: location to place mark
 * @left_gravity: whether the mark has left gravity
 *
 * Creates a mark at position @where.
 *
 * If @mark_name is %NULL, the mark is anonymous; otherwise, the mark
 * can be retrieved by name using [method@Bobgui.TextBuffer.get_mark].
 * If a mark has left gravity, and text is inserted at the mark’s
 * current location, the mark will be moved to the left of the
 * newly-inserted text. If the mark has right gravity
 * (@left_gravity = %FALSE), the mark will end up on the right of
 * newly-inserted text. The standard left-to-right cursor is a mark
 * with right gravity (when you type, the cursor stays on the right
 * side of the text you’re typing).
 *
 * The caller of this function does not own a
 * reference to the returned `BobguiTextMark`, so you can ignore the
 * return value if you like. Marks are owned by the buffer and go
 * away when the buffer does.
 *
 * Emits the [signal@Bobgui.TextBuffer::mark-set] signal as notification
 * of the mark's initial placement.
 *
 * Returns: (transfer none): the new `BobguiTextMark` object
 */
BobguiTextMark*
bobgui_text_buffer_create_mark (BobguiTextBuffer     *buffer,
                             const char        *mark_name,
                             const BobguiTextIter *where,
                             gboolean           left_gravity)
{
  g_return_val_if_fail (BOBGUI_IS_TEXT_BUFFER (buffer), NULL);

  return bobgui_text_buffer_set_mark (buffer, NULL, mark_name, where,
                                   left_gravity, FALSE);
}

/**
 * bobgui_text_buffer_add_mark:
 * @buffer: a `BobguiTextBuffer`
 * @mark: the mark to add
 * @where: location to place mark
 *
 * Adds the mark at position @where.
 *
 * The mark must not be added to another buffer, and if its name
 * is not %NULL then there must not be another mark in the buffer
 * with the same name.
 *
 * Emits the [signal@Bobgui.TextBuffer::mark-set] signal as notification
 * of the mark's initial placement.
 */
void
bobgui_text_buffer_add_mark (BobguiTextBuffer     *buffer,
                          BobguiTextMark       *mark,
                          const BobguiTextIter *where)
{
  const char *name;

  g_return_if_fail (BOBGUI_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (BOBGUI_IS_TEXT_MARK (mark));
  g_return_if_fail (where != NULL);
  g_return_if_fail (bobgui_text_mark_get_buffer (mark) == NULL);

  name = bobgui_text_mark_get_name (mark);

  if (name != NULL && bobgui_text_buffer_get_mark (buffer, name) != NULL)
    {
      g_critical ("Mark %s already exists in the buffer", name);
      return;
    }

  bobgui_text_buffer_set_mark (buffer, mark, NULL, where, FALSE, FALSE);
}

/**
 * bobgui_text_buffer_move_mark:
 * @buffer: a `BobguiTextBuffer`
 * @mark: a `BobguiTextMark`
 * @where: new location for @mark in @buffer
 *
 * Moves @mark to the new location @where.
 *
 * Emits the [signal@Bobgui.TextBuffer::mark-set] signal
 * as notification of the move.
 */
void
bobgui_text_buffer_move_mark (BobguiTextBuffer     *buffer,
                           BobguiTextMark       *mark,
                           const BobguiTextIter *where)
{
  g_return_if_fail (BOBGUI_IS_TEXT_MARK (mark));
  g_return_if_fail (!bobgui_text_mark_get_deleted (mark));
  g_return_if_fail (BOBGUI_IS_TEXT_BUFFER (buffer));

  bobgui_text_buffer_set_mark (buffer, mark, NULL, where, FALSE, TRUE);
}

/**
 * bobgui_text_buffer_get_iter_at_mark:
 * @buffer: a `BobguiTextBuffer`
 * @iter: (out): iterator to initialize
 * @mark: a `BobguiTextMark` in @buffer
 *
 * Initializes @iter with the current position of @mark.
 */
void
bobgui_text_buffer_get_iter_at_mark (BobguiTextBuffer *buffer,
                                  BobguiTextIter   *iter,
                                  BobguiTextMark   *mark)
{
  g_return_if_fail (BOBGUI_IS_TEXT_MARK (mark));
  g_return_if_fail (!bobgui_text_mark_get_deleted (mark));
  g_return_if_fail (BOBGUI_IS_TEXT_BUFFER (buffer));

  _bobgui_text_btree_get_iter_at_mark (get_btree (buffer),
                                    iter,
                                    mark);
}

/**
 * bobgui_text_buffer_delete_mark:
 * @buffer: a `BobguiTextBuffer`
 * @mark: a `BobguiTextMark` in @buffer
 *
 * Deletes @mark, so that it’s no longer located anywhere in the
 * buffer.
 *
 * Removes the reference the buffer holds to the mark, so if
 * you haven’t called g_object_ref() on the mark, it will be freed.
 * Even if the mark isn’t freed, most operations on @mark become
 * invalid, until it gets added to a buffer again with
 * [method@Bobgui.TextBuffer.add_mark]. Use [method@Bobgui.TextMark.get_deleted]
 * to find out if a mark has been removed from its buffer.
 *
 * The [signal@Bobgui.TextBuffer::mark-deleted] signal will be emitted as
 * notification after the mark is deleted.
 */
void
bobgui_text_buffer_delete_mark (BobguiTextBuffer *buffer,
                             BobguiTextMark   *mark)
{
  g_return_if_fail (BOBGUI_IS_TEXT_MARK (mark));
  g_return_if_fail (!bobgui_text_mark_get_deleted (mark));
  g_return_if_fail (BOBGUI_IS_TEXT_BUFFER (buffer));

  g_object_ref (mark);

  _bobgui_text_btree_remove_mark (get_btree (buffer), mark);

  /* See rationale above for MARK_SET on why we emit this after
   * removing the mark, rather than removing the mark in a default
   * handler.
   */
  g_signal_emit (buffer, signals[MARK_DELETED],
                 0,
                 mark);

  g_object_unref (mark);
}

/**
 * bobgui_text_buffer_get_mark:
 * @buffer: a `BobguiTextBuffer`
 * @name: a mark name
 *
 * Returns the mark named @name in buffer @buffer, or %NULL if no such
 * mark exists in the buffer.
 *
 * Returns: (nullable) (transfer none): a `BobguiTextMark`
 **/
BobguiTextMark*
bobgui_text_buffer_get_mark (BobguiTextBuffer *buffer,
                          const char    *name)
{
  BobguiTextMark *mark;

  g_return_val_if_fail (BOBGUI_IS_TEXT_BUFFER (buffer), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  mark = _bobgui_text_btree_get_mark_by_name (get_btree (buffer), name);

  return mark;
}

/**
 * bobgui_text_buffer_move_mark_by_name:
 * @buffer: a `BobguiTextBuffer`
 * @name: name of a mark
 * @where: new location for mark
 *
 * Moves the mark named @name (which must exist) to location @where.
 *
 * See [method@Bobgui.TextBuffer.move_mark] for details.
 */
void
bobgui_text_buffer_move_mark_by_name (BobguiTextBuffer     *buffer,
                                   const char        *name,
                                   const BobguiTextIter *where)
{
  BobguiTextMark *mark;

  g_return_if_fail (BOBGUI_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (name != NULL);

  mark = _bobgui_text_btree_get_mark_by_name (get_btree (buffer), name);

  if (mark == NULL)
    {
      g_warning ("%s: no mark named '%s'", G_STRLOC, name);
      return;
    }

  bobgui_text_buffer_move_mark (buffer, mark, where);
}

/**
 * bobgui_text_buffer_delete_mark_by_name:
 * @buffer: a `BobguiTextBuffer`
 * @name: name of a mark in @buffer
 *
 * Deletes the mark named @name; the mark must exist.
 *
 * See [method@Bobgui.TextBuffer.delete_mark] for details.
 **/
void
bobgui_text_buffer_delete_mark_by_name (BobguiTextBuffer *buffer,
                                     const char    *name)
{
  BobguiTextMark *mark;

  g_return_if_fail (BOBGUI_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (name != NULL);

  mark = _bobgui_text_btree_get_mark_by_name (get_btree (buffer), name);

  if (mark == NULL)
    {
      g_warning ("%s: no mark named '%s'", G_STRLOC, name);
      return;
    }

  bobgui_text_buffer_delete_mark (buffer, mark);
}

/**
 * bobgui_text_buffer_get_insert:
 * @buffer: a `BobguiTextBuffer`
 *
 * Returns the mark that represents the cursor (insertion point).
 *
 * Equivalent to calling [method@Bobgui.TextBuffer.get_mark]
 * to get the mark named “insert”, but very slightly more
 * efficient, and involves less typing.
 *
 * Returns: (transfer none): insertion point mark
 */
BobguiTextMark*
bobgui_text_buffer_get_insert (BobguiTextBuffer *buffer)
{
  g_return_val_if_fail (BOBGUI_IS_TEXT_BUFFER (buffer), NULL);

  return _bobgui_text_btree_get_insert (get_btree (buffer));
}

/**
 * bobgui_text_buffer_get_selection_bound:
 * @buffer: a `BobguiTextBuffer`
 *
 * Returns the mark that represents the selection bound.
 *
 * Equivalent to calling [method@Bobgui.TextBuffer.get_mark]
 * to get the mark named “selection_bound”, but very slightly
 * more efficient, and involves less typing.
 *
 * The currently-selected text in @buffer is the region between the
 * “selection_bound” and “insert” marks. If “selection_bound” and
 * “insert” are in the same place, then there is no current selection.
 * [method@Bobgui.TextBuffer.get_selection_bounds] is another convenient
 * function for handling the selection, if you just want to know whether
 * there’s a selection and what its bounds are.
 *
 * Returns: (transfer none): selection bound mark
 */
BobguiTextMark*
bobgui_text_buffer_get_selection_bound (BobguiTextBuffer *buffer)
{
  g_return_val_if_fail (BOBGUI_IS_TEXT_BUFFER (buffer), NULL);

  return _bobgui_text_btree_get_selection_bound (get_btree (buffer));
}

/**
 * bobgui_text_buffer_get_iter_at_child_anchor:
 * @buffer: a `BobguiTextBuffer`
 * @iter: (out): an iterator to be initialized
 * @anchor: a child anchor that appears in @buffer
 *
 * Obtains the location of @anchor within @buffer.
 */
void
bobgui_text_buffer_get_iter_at_child_anchor (BobguiTextBuffer      *buffer,
                                          BobguiTextIter        *iter,
                                          BobguiTextChildAnchor *anchor)
{
  g_return_if_fail (BOBGUI_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (iter != NULL);
  g_return_if_fail (BOBGUI_IS_TEXT_CHILD_ANCHOR (anchor));
  g_return_if_fail (!bobgui_text_child_anchor_get_deleted (anchor));

  _bobgui_text_btree_get_iter_at_child_anchor (get_btree (buffer),
                                           iter,
                                           anchor);
}

/**
 * bobgui_text_buffer_place_cursor:
 * @buffer: a `BobguiTextBuffer`
 * @where: where to put the cursor
 *
 * This function moves the “insert” and “selection_bound” marks
 * simultaneously.
 *
 * If you move them to the same place in two steps with
 * [method@Bobgui.TextBuffer.move_mark], you will temporarily select a
 * region in between their old and new locations, which can be pretty
 * inefficient since the temporarily-selected region will force stuff
 * to be recalculated. This function moves them as a unit, which can
 * be optimized.
 */
void
bobgui_text_buffer_place_cursor (BobguiTextBuffer     *buffer,
                              const BobguiTextIter *where)
{
  bobgui_text_buffer_select_range (buffer, where, where);
}

/**
 * bobgui_text_buffer_select_range:
 * @buffer: a `BobguiTextBuffer`
 * @ins: where to put the “insert” mark
 * @bound: where to put the “selection_bound” mark
 *
 * This function moves the “insert” and “selection_bound” marks
 * simultaneously.
 *
 * If you move them in two steps with
 * [method@Bobgui.TextBuffer.move_mark], you will temporarily select a
 * region in between their old and new locations, which can be pretty
 * inefficient since the temporarily-selected region will force stuff
 * to be recalculated. This function moves them as a unit, which can
 * be optimized.
 */
void
bobgui_text_buffer_select_range (BobguiTextBuffer     *buffer,
			      const BobguiTextIter *ins,
                              const BobguiTextIter *bound)
{
  BobguiTextIter real_ins;
  BobguiTextIter real_bound;

  g_return_if_fail (BOBGUI_IS_TEXT_BUFFER (buffer));

  real_ins = *ins;
  real_bound = *bound;

  _bobgui_text_btree_select_range (get_btree (buffer), &real_ins, &real_bound);
  bobgui_text_buffer_mark_set (buffer, &real_ins,
                            bobgui_text_buffer_get_insert (buffer));
  bobgui_text_buffer_mark_set (buffer, &real_bound,
                            bobgui_text_buffer_get_selection_bound (buffer));
}

/*
 * Tags
 */

/**
 * bobgui_text_buffer_create_tag:
 * @buffer: a `BobguiTextBuffer`
 * @tag_name: (nullable): name of the new tag
 * @first_property_name: (nullable): name of first property to set
 * @...: %NULL-terminated list of property names and values
 *
 * Creates a tag and adds it to the tag table for @buffer.
 *
 * Equivalent to calling [ctor@Bobgui.TextTag.new] and then adding the
 * tag to the buffer’s tag table. The returned tag is owned by
 * the buffer’s tag table, so the ref count will be equal to one.
 *
 * If @tag_name is %NULL, the tag is anonymous.
 *
 * If @tag_name is non-%NULL, a tag called @tag_name must not already
 * exist in the tag table for this buffer.
 *
 * The @first_property_name argument and subsequent arguments are a list
 * of properties to set on the tag, as with g_object_set().
 *
 * Returns: (transfer none): a new tag
 */
BobguiTextTag*
bobgui_text_buffer_create_tag (BobguiTextBuffer *buffer,
                            const char    *tag_name,
                            const char    *first_property_name,
                            ...)
{
  BobguiTextTag *tag;
  va_list list;

  g_return_val_if_fail (BOBGUI_IS_TEXT_BUFFER (buffer), NULL);

  tag = bobgui_text_tag_new (tag_name);

  if (!bobgui_text_tag_table_add (get_table (buffer), tag))
    {
      g_object_unref (tag);
      return NULL;
    }

  if (first_property_name)
    {
      va_start (list, first_property_name);
      g_object_set_valist (G_OBJECT (tag), first_property_name, list);
      va_end (list);
    }

  g_object_unref (tag);

  return tag;
}

static void
bobgui_text_buffer_real_apply_tag (BobguiTextBuffer     *buffer,
                                BobguiTextTag        *tag,
                                const BobguiTextIter *start,
                                const BobguiTextIter *end)
{
  if (tag->priv->table != buffer->priv->tag_table)
    {
      g_warning ("Can only apply tags that are in the tag table for the buffer");
      return;
    }

  _bobgui_text_btree_tag (start, end, tag, TRUE);
}

static void
bobgui_text_buffer_real_remove_tag (BobguiTextBuffer     *buffer,
                                 BobguiTextTag        *tag,
                                 const BobguiTextIter *start,
                                 const BobguiTextIter *end)
{
  if (tag->priv->table != buffer->priv->tag_table)
    {
      g_warning ("Can only remove tags that are in the tag table for the buffer");
      return;
    }

  _bobgui_text_btree_tag (start, end, tag, FALSE);
}

static void
bobgui_text_buffer_real_changed (BobguiTextBuffer *buffer)
{
  bobgui_text_buffer_set_modified (buffer, TRUE);

  g_object_notify_by_pspec (G_OBJECT (buffer), text_buffer_props[PROP_TEXT]);
}

static void
bobgui_text_buffer_real_mark_set (BobguiTextBuffer     *buffer,
                               const BobguiTextIter *iter,
                               BobguiTextMark       *mark)
{
  BobguiTextMark *insert;

  insert = bobgui_text_buffer_get_insert (buffer);

  if (mark == insert || mark == bobgui_text_buffer_get_selection_bound (buffer))
    {
      gboolean has_selection;

      update_selection_clipboards (buffer);

      has_selection = bobgui_text_buffer_get_selection_bounds (buffer,
                                                            NULL,
                                                            NULL);

      if (has_selection != buffer->priv->has_selection)
        {
          buffer->priv->has_selection = has_selection;
          g_object_notify_by_pspec (G_OBJECT (buffer), text_buffer_props[PROP_HAS_SELECTION]);
        }
    }

    if (mark == insert)
      g_object_notify_by_pspec (G_OBJECT (buffer), text_buffer_props[PROP_CURSOR_POSITION]);
}

static void
bobgui_text_buffer_emit_tag (BobguiTextBuffer     *buffer,
                          BobguiTextTag        *tag,
                          gboolean           apply,
                          const BobguiTextIter *start,
                          const BobguiTextIter *end)
{
  BobguiTextIter start_tmp = *start;
  BobguiTextIter end_tmp = *end;

  g_return_if_fail (tag != NULL);

  bobgui_text_iter_order (&start_tmp, &end_tmp);

  if (apply)
    g_signal_emit (buffer, signals[APPLY_TAG],
                   0,
                   tag, &start_tmp, &end_tmp);
  else
    g_signal_emit (buffer, signals[REMOVE_TAG],
                   0,
                   tag, &start_tmp, &end_tmp);
}

/**
 * bobgui_text_buffer_apply_tag:
 * @buffer: a `BobguiTextBuffer`
 * @tag: a `BobguiTextTag`
 * @start: one bound of range to be tagged
 * @end: other bound of range to be tagged
 *
 * Emits the “apply-tag” signal on @buffer.
 *
 * The default handler for the signal applies
 * @tag to the given range. @start and @end do
 * not have to be in order.
 */
void
bobgui_text_buffer_apply_tag (BobguiTextBuffer     *buffer,
                           BobguiTextTag        *tag,
                           const BobguiTextIter *start,
                           const BobguiTextIter *end)
{
  g_return_if_fail (BOBGUI_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (BOBGUI_IS_TEXT_TAG (tag));
  g_return_if_fail (start != NULL);
  g_return_if_fail (end != NULL);
  g_return_if_fail (bobgui_text_iter_get_buffer (start) == buffer);
  g_return_if_fail (bobgui_text_iter_get_buffer (end) == buffer);
  g_return_if_fail (tag->priv->table == buffer->priv->tag_table);

  bobgui_text_buffer_emit_tag (buffer, tag, TRUE, start, end);
}

/**
 * bobgui_text_buffer_remove_tag:
 * @buffer: a `BobguiTextBuffer`
 * @tag: a `BobguiTextTag`
 * @start: one bound of range to be untagged
 * @end: other bound of range to be untagged
 *
 * Emits the “remove-tag” signal.
 *
 * The default handler for the signal removes all occurrences
 * of @tag from the given range. @start and @end don’t have
 * to be in order.
 */
void
bobgui_text_buffer_remove_tag (BobguiTextBuffer     *buffer,
                            BobguiTextTag        *tag,
                            const BobguiTextIter *start,
                            const BobguiTextIter *end)

{
  g_return_if_fail (BOBGUI_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (BOBGUI_IS_TEXT_TAG (tag));
  g_return_if_fail (start != NULL);
  g_return_if_fail (end != NULL);
  g_return_if_fail (bobgui_text_iter_get_buffer (start) == buffer);
  g_return_if_fail (bobgui_text_iter_get_buffer (end) == buffer);
  g_return_if_fail (tag->priv->table == buffer->priv->tag_table);

  bobgui_text_buffer_emit_tag (buffer, tag, FALSE, start, end);
}

/**
 * bobgui_text_buffer_apply_tag_by_name:
 * @buffer: a `BobguiTextBuffer`
 * @name: name of a named `BobguiTextTag`
 * @start: one bound of range to be tagged
 * @end: other bound of range to be tagged
 *
 * Emits the “apply-tag” signal on @buffer.
 *
 * Calls [method@Bobgui.TextTagTable.lookup] on the buffer’s
 * tag table to get a `BobguiTextTag`, then calls
 * [method@Bobgui.TextBuffer.apply_tag].
 */
void
bobgui_text_buffer_apply_tag_by_name (BobguiTextBuffer     *buffer,
                                   const char        *name,
                                   const BobguiTextIter *start,
                                   const BobguiTextIter *end)
{
  BobguiTextTag *tag;

  g_return_if_fail (BOBGUI_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (name != NULL);
  g_return_if_fail (start != NULL);
  g_return_if_fail (end != NULL);
  g_return_if_fail (bobgui_text_iter_get_buffer (start) == buffer);
  g_return_if_fail (bobgui_text_iter_get_buffer (end) == buffer);

  tag = bobgui_text_tag_table_lookup (get_table (buffer),
                                   name);

  if (tag == NULL)
    {
      g_warning ("Unknown tag '%s'", name);
      return;
    }

  bobgui_text_buffer_emit_tag (buffer, tag, TRUE, start, end);
}

/**
 * bobgui_text_buffer_remove_tag_by_name:
 * @buffer: a `BobguiTextBuffer`
 * @name: name of a `BobguiTextTag`
 * @start: one bound of range to be untagged
 * @end: other bound of range to be untagged
 *
 * Emits the “remove-tag” signal.
 *
 * Calls [method@Bobgui.TextTagTable.lookup] on the buffer’s
 * tag table to get a `BobguiTextTag`, then calls
 * [method@Bobgui.TextBuffer.remove_tag].
 **/
void
bobgui_text_buffer_remove_tag_by_name (BobguiTextBuffer     *buffer,
                                    const char        *name,
                                    const BobguiTextIter *start,
                                    const BobguiTextIter *end)
{
  BobguiTextTag *tag;

  g_return_if_fail (BOBGUI_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (name != NULL);
  g_return_if_fail (start != NULL);
  g_return_if_fail (end != NULL);
  g_return_if_fail (bobgui_text_iter_get_buffer (start) == buffer);
  g_return_if_fail (bobgui_text_iter_get_buffer (end) == buffer);

  tag = bobgui_text_tag_table_lookup (get_table (buffer),
                                   name);

  if (tag == NULL)
    {
      g_warning ("Unknown tag '%s'", name);
      return;
    }

  bobgui_text_buffer_emit_tag (buffer, tag, FALSE, start, end);
}

static int
pointer_cmp (gconstpointer a,
             gconstpointer b)
{
  if (a < b)
    return -1;
  else if (a > b)
    return 1;
  else
    return 0;
}

/**
 * bobgui_text_buffer_remove_all_tags:
 * @buffer: a `BobguiTextBuffer`
 * @start: one bound of range to be untagged
 * @end: other bound of range to be untagged
 *
 * Removes all tags in the range between @start and @end.
 *
 * Be careful with this function; it could remove tags added in code
 * unrelated to the code you’re currently writing. That is, using this
 * function is probably a bad idea if you have two or more unrelated
 * code sections that add tags.
 */
void
bobgui_text_buffer_remove_all_tags (BobguiTextBuffer     *buffer,
                                 const BobguiTextIter *start,
                                 const BobguiTextIter *end)
{
  BobguiTextIter first, second, tmp;
  GSList *tags;
  GSList *tmp_list;
  GSList *prev, *next;
  BobguiTextTag *tag;

  g_return_if_fail (BOBGUI_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (start != NULL);
  g_return_if_fail (end != NULL);
  g_return_if_fail (bobgui_text_iter_get_buffer (start) == buffer);
  g_return_if_fail (bobgui_text_iter_get_buffer (end) == buffer);

  first = *start;
  second = *end;

  bobgui_text_iter_order (&first, &second);

  /* Get all tags turned on at the start */
  tags = bobgui_text_iter_get_tags (&first);

  /* Find any that are toggled on within the range */
  tmp = first;
  while (bobgui_text_iter_forward_to_tag_toggle (&tmp, NULL))
    {
      GSList *toggled;
      GSList *tmp_list2;

      if (bobgui_text_iter_compare (&tmp, &second) >= 0)
        break; /* past the end of the range */

      toggled = bobgui_text_iter_get_toggled_tags (&tmp, TRUE);

      /* We could end up with a really big-ass list here.
       * Fix it someday.
       */
      tmp_list2 = toggled;
      while (tmp_list2 != NULL)
        {
          tags = g_slist_prepend (tags, tmp_list2->data);

          tmp_list2 = tmp_list2->next;
        }

      g_slist_free (toggled);
    }

  /* Sort the list */
  tags = g_slist_sort (tags, pointer_cmp);

  /* Strip duplicates */
  tag = NULL;
  prev = NULL;
  tmp_list = tags;
  while (tmp_list != NULL)
    {
      if (tag == tmp_list->data)
        {
          /* duplicate */
          next = tmp_list->next;
          if (prev)
            prev->next = next;

          tmp_list->next = NULL;

          g_slist_free (tmp_list);

          tmp_list = next;
          /* prev is unchanged */
        }
      else
        {
          /* not a duplicate */
          tag = BOBGUI_TEXT_TAG (tmp_list->data);
          prev = tmp_list;
          tmp_list = tmp_list->next;
        }
    }

  g_slist_foreach (tags, (GFunc) g_object_ref, NULL);

  tmp_list = tags;
  while (tmp_list != NULL)
    {
      tag = BOBGUI_TEXT_TAG (tmp_list->data);

      bobgui_text_buffer_remove_tag (buffer, tag, &first, &second);

      tmp_list = tmp_list->next;
    }

  g_slist_free_full (tags, g_object_unref);
}


/*
 * Obtain various iterators
 */

/**
 * bobgui_text_buffer_get_iter_at_line_offset:
 * @buffer: a `BobguiTextBuffer`
 * @iter: (out): iterator to initialize
 * @line_number: line number counting from 0
 * @char_offset: char offset from start of line
 *
 * Obtains an iterator pointing to @char_offset within the given line.
 *
 * Note characters, not bytes; UTF-8 may encode one character as multiple
 * bytes.
 *
 * If @line_number is greater than or equal to the number of lines in the @buffer,
 * the end iterator is returned. And if @char_offset is off the
 * end of the line, the iterator at the end of the line is returned.
 *
 * Returns: whether the exact position has been found
 */
gboolean
bobgui_text_buffer_get_iter_at_line_offset (BobguiTextBuffer *buffer,
                                         BobguiTextIter   *iter,
                                         int            line_number,
                                         int            char_offset)
{
  BobguiTextIter end_line_iter;

  g_return_val_if_fail (iter != NULL, FALSE);
  g_return_val_if_fail (BOBGUI_IS_TEXT_BUFFER (buffer), FALSE);

  if (line_number >= bobgui_text_buffer_get_line_count (buffer))
    {
      bobgui_text_buffer_get_end_iter (buffer, iter);
      return FALSE;
    }

  _bobgui_text_btree_get_iter_at_line_char (get_btree (buffer), iter, line_number, 0);

  end_line_iter = *iter;
  if (!bobgui_text_iter_ends_line (&end_line_iter))
    bobgui_text_iter_forward_to_line_end (&end_line_iter);

  if (char_offset > bobgui_text_iter_get_line_offset (&end_line_iter))
    {
      *iter = end_line_iter;
      return FALSE;
    }

  bobgui_text_iter_set_line_offset (iter, char_offset);
  return TRUE;
}

/**
 * bobgui_text_buffer_get_iter_at_line_index:
 * @buffer: a `BobguiTextBuffer`
 * @iter: (out): iterator to initialize
 * @line_number: line number counting from 0
 * @byte_index: byte index from start of line
 *
 * Obtains an iterator pointing to @byte_index within the given line.
 *
 * @byte_index must be the start of a UTF-8 character. Note bytes, not
 * characters; UTF-8 may encode one character as multiple bytes.
 *
 * If @line_number is greater than or equal to the number of lines in the @buffer,
 * the end iterator is returned. And if @byte_index is off the
 * end of the line, the iterator at the end of the line is returned.
 *
 * Returns: whether the exact position has been found
 */
gboolean
bobgui_text_buffer_get_iter_at_line_index  (BobguiTextBuffer *buffer,
                                         BobguiTextIter   *iter,
                                         int            line_number,
                                         int            byte_index)
{
  BobguiTextIter end_line_iter;

  g_return_val_if_fail (iter != NULL, FALSE);
  g_return_val_if_fail (BOBGUI_IS_TEXT_BUFFER (buffer), FALSE);

  if (line_number >= bobgui_text_buffer_get_line_count (buffer))
    {
      bobgui_text_buffer_get_end_iter (buffer, iter);
      return FALSE;
    }

  bobgui_text_buffer_get_iter_at_line (buffer, iter, line_number);

  end_line_iter = *iter;
  if (!bobgui_text_iter_ends_line (&end_line_iter))
    bobgui_text_iter_forward_to_line_end (&end_line_iter);

  if (byte_index > bobgui_text_iter_get_line_index (&end_line_iter))
    {
      *iter = end_line_iter;
      return FALSE;
    }

  bobgui_text_iter_set_line_index (iter, byte_index);
  return TRUE;
}

/**
 * bobgui_text_buffer_get_iter_at_line:
 * @buffer: a `BobguiTextBuffer`
 * @iter: (out): iterator to initialize
 * @line_number: line number counting from 0
 *
 * Initializes @iter to the start of the given line.
 *
 * If @line_number is greater than or equal to the number of lines
 * in the @buffer, the end iterator is returned.
 *
 * Returns: whether the exact position has been found
 */
gboolean
bobgui_text_buffer_get_iter_at_line (BobguiTextBuffer *buffer,
                                  BobguiTextIter   *iter,
                                  int            line_number)
{
  g_return_val_if_fail (iter != NULL, FALSE);
  g_return_val_if_fail (BOBGUI_IS_TEXT_BUFFER (buffer), FALSE);

  return bobgui_text_buffer_get_iter_at_line_offset (buffer, iter, line_number, 0);
}

/**
 * bobgui_text_buffer_get_iter_at_offset:
 * @buffer: a `BobguiTextBuffer`
 * @iter: (out): iterator to initialize
 * @char_offset: char offset from start of buffer, counting from 0, or -1
 *
 * Initializes @iter to a position @char_offset chars from the start
 * of the entire buffer.
 *
 * If @char_offset is -1 or greater than the number
 * of characters in the buffer, @iter is initialized to the end iterator,
 * the iterator one past the last valid character in the buffer.
 */
void
bobgui_text_buffer_get_iter_at_offset (BobguiTextBuffer *buffer,
                                    BobguiTextIter   *iter,
                                    int            char_offset)
{
  g_return_if_fail (iter != NULL);
  g_return_if_fail (BOBGUI_IS_TEXT_BUFFER (buffer));

  _bobgui_text_btree_get_iter_at_char (get_btree (buffer), iter, char_offset);
}

/**
 * bobgui_text_buffer_get_start_iter:
 * @buffer: a `BobguiTextBuffer`
 * @iter: (out): iterator to initialize
 *
 * Initialized @iter with the first position in the text buffer.
 *
 * This is the same as using [method@Bobgui.TextBuffer.get_iter_at_offset]
 * to get the iter at character offset 0.
 */
void
bobgui_text_buffer_get_start_iter (BobguiTextBuffer *buffer,
                                BobguiTextIter   *iter)
{
  g_return_if_fail (iter != NULL);
  g_return_if_fail (BOBGUI_IS_TEXT_BUFFER (buffer));

  _bobgui_text_btree_get_iter_at_char (get_btree (buffer), iter, 0);
}

/**
 * bobgui_text_buffer_get_end_iter:
 * @buffer: a `BobguiTextBuffer`
 * @iter: (out): iterator to initialize
 *
 * Initializes @iter with the “end iterator,” one past the last valid
 * character in the text buffer.
 *
 * If dereferenced with [method@Bobgui.TextIter.get_char], the end
 * iterator has a character value of 0.
 * The entire buffer lies in the range from the first position in
 * the buffer (call [method@Bobgui.TextBuffer.get_start_iter] to get
 * character position 0) to the end iterator.
 */
void
bobgui_text_buffer_get_end_iter (BobguiTextBuffer *buffer,
                              BobguiTextIter   *iter)
{
  g_return_if_fail (iter != NULL);
  g_return_if_fail (BOBGUI_IS_TEXT_BUFFER (buffer));

  _bobgui_text_btree_get_end_iter (get_btree (buffer), iter);
}

/**
 * bobgui_text_buffer_get_bounds:
 * @buffer: a `BobguiTextBuffer`
 * @start: (out): iterator to initialize with first position in the buffer
 * @end: (out): iterator to initialize with the end iterator
 *
 * Retrieves the first and last iterators in the buffer, i.e. the
 * entire buffer lies within the range [@start,@end).
 */
void
bobgui_text_buffer_get_bounds (BobguiTextBuffer *buffer,
                            BobguiTextIter   *start,
                            BobguiTextIter   *end)
{
  g_return_if_fail (start != NULL);
  g_return_if_fail (end != NULL);
  g_return_if_fail (BOBGUI_IS_TEXT_BUFFER (buffer));

  _bobgui_text_btree_get_iter_at_char (get_btree (buffer), start, 0);
  _bobgui_text_btree_get_end_iter (get_btree (buffer), end);
}

/*
 * Modified flag
 */

/**
 * bobgui_text_buffer_get_modified:
 * @buffer: a `BobguiTextBuffer`
 *
 * Indicates whether the buffer has been modified since the last call
 * to [method@Bobgui.TextBuffer.set_modified] set the modification flag to
 * %FALSE.
 *
 * Used for example to enable a “save” function in a text editor.
 *
 * Returns: %TRUE if the buffer has been modified
 */
gboolean
bobgui_text_buffer_get_modified (BobguiTextBuffer *buffer)
{
  g_return_val_if_fail (BOBGUI_IS_TEXT_BUFFER (buffer), FALSE);

  return buffer->priv->modified;
}

/**
 * bobgui_text_buffer_set_modified:
 * @buffer: a `BobguiTextBuffer`
 * @setting: modification flag setting
 *
 * Used to keep track of whether the buffer has been
 * modified since the last time it was saved.
 *
 * Whenever the buffer is saved to disk, call
 * `bobgui_text_buffer_set_modified (@buffer, FALSE)`.
 * When the buffer is modified, it will automatically
 * toggle on the modified bit again. When the modified
 * bit flips, the buffer emits the
 * [signal@Bobgui.TextBuffer::modified-changed] signal.
 */
void
bobgui_text_buffer_set_modified (BobguiTextBuffer *buffer,
                              gboolean       setting)
{
  g_return_if_fail (BOBGUI_IS_TEXT_BUFFER (buffer));

  setting = !!setting;

  if (buffer->priv->modified != setting)
    {
      buffer->priv->modified = setting;
      bobgui_text_history_modified_changed (buffer->priv->history, setting);
      g_signal_emit (buffer, signals[MODIFIED_CHANGED], 0);
    }
}

/**
 * bobgui_text_buffer_get_has_selection:
 * @buffer: a `BobguiTextBuffer`
 *
 * Indicates whether the buffer has some text currently selected.
 *
 * Returns: %TRUE if the there is text selected
 */
gboolean
bobgui_text_buffer_get_has_selection (BobguiTextBuffer *buffer)
{
  g_return_val_if_fail (BOBGUI_IS_TEXT_BUFFER (buffer), FALSE);

  return buffer->priv->has_selection;
}


/*
 * Assorted other stuff
 */

/**
 * bobgui_text_buffer_get_line_count:
 * @buffer: a `BobguiTextBuffer`
 *
 * Obtains the number of lines in the buffer.
 *
 * This value is cached, so the function is very fast.
 *
 * Returns: number of lines in the buffer
 */
int
bobgui_text_buffer_get_line_count (BobguiTextBuffer *buffer)
{
  g_return_val_if_fail (BOBGUI_IS_TEXT_BUFFER (buffer), 0);

  return _bobgui_text_btree_line_count (get_btree (buffer));
}

/**
 * bobgui_text_buffer_get_char_count:
 * @buffer: a `BobguiTextBuffer`
 *
 * Gets the number of characters in the buffer.
 *
 * Note that characters and bytes are not the same, you can’t e.g.
 * expect the contents of the buffer in string form to be this
 * many bytes long.
 *
 * The character count is cached, so this function is very fast.
 *
 * Returns: number of characters in the buffer
 */
int
bobgui_text_buffer_get_char_count (BobguiTextBuffer *buffer)
{
  g_return_val_if_fail (BOBGUI_IS_TEXT_BUFFER (buffer), 0);

  return _bobgui_text_btree_char_count (get_btree (buffer));
}

static BobguiTextBuffer *
create_clipboard_contents_buffer (BobguiTextBuffer *buffer,
                                  BobguiTextIter   *start_iter,
                                  BobguiTextIter   *end_iter)
{
  BobguiTextIter start, end;
  BobguiTextBuffer *contents;

  contents = bobgui_text_buffer_new (bobgui_text_buffer_get_tag_table (buffer));

  bobgui_text_buffer_get_iter_at_offset (contents, &start, 0);
  bobgui_text_buffer_insert_range (contents, &start, start_iter, end_iter);
  bobgui_text_buffer_get_bounds (contents, &start, &end);
  bobgui_text_buffer_select_range (contents, &start, &end);

  /*  Ref the source buffer as long as the clipboard contents buffer
   *  exists, because it's needed for serializing the contents buffer.
   *  See http://bugzilla.gnome.org/show_bug.cgi?id=339195
   */
  g_object_ref (buffer);
  g_object_weak_ref (G_OBJECT (contents), (GWeakNotify) g_object_unref, buffer);

  return contents;
}

static void
get_paste_point (BobguiTextBuffer *buffer,
                 BobguiTextIter   *iter,
                 gboolean       clear_afterward)
{
  BobguiTextIter insert_point;
  BobguiTextMark *paste_point_override;

  paste_point_override = bobgui_text_buffer_get_mark (buffer,
                                                   "bobgui_paste_point_override");

  if (paste_point_override != NULL)
    {
      bobgui_text_buffer_get_iter_at_mark (buffer, &insert_point,
                                        paste_point_override);
      if (clear_afterward)
        bobgui_text_buffer_delete_mark (buffer, paste_point_override);
    }
  else
    {
      bobgui_text_buffer_get_iter_at_mark (buffer, &insert_point,
                                        bobgui_text_buffer_get_insert (buffer));
    }

  *iter = insert_point;
}

static void
pre_paste_prep (ClipboardRequest *request_data,
                BobguiTextIter      *insert_point)
{
  BobguiTextBuffer *buffer = request_data->buffer;

  get_paste_point (buffer, insert_point, TRUE);

  if (request_data->replace_selection)
    {
      BobguiTextIter start, end;

      if (bobgui_text_buffer_get_selection_bounds (buffer, &start, &end))
        {
          if (request_data->interactive)
            bobgui_text_buffer_delete_interactive (request_data->buffer,
                                                &start,
                                                &end,
                                                request_data->default_editable);
          else
            bobgui_text_buffer_delete (request_data->buffer, &start, &end);

          *insert_point = start;
        }
    }
}

static void
emit_paste_done (BobguiTextBuffer *buffer,
                 GdkClipboard  *clipboard)
{
  g_signal_emit (buffer, signals[PASTE_DONE], 0, clipboard);
}

static void
free_clipboard_request (ClipboardRequest *request_data)
{
  g_object_unref (request_data->buffer);
  g_free (request_data);
}

#if 0
/* These are pretty handy functions; maybe something like them
 * should be in the public API. Also, there are other places in this
 * file where they could be used.
 */
static gpointer
save_iter (const BobguiTextIter *iter,
           gboolean           left_gravity)
{
  return bobgui_text_buffer_create_mark (bobgui_text_iter_get_buffer (iter),
                                      NULL,
                                      iter,
                                      TRUE);
}

static void
restore_iter (const BobguiTextIter *iter,
              gpointer           save_id)
{
  bobgui_text_buffer_get_iter_at_mark (bobgui_text_mark_get_buffer (save_id),
                                    (BobguiTextIter*) iter,
                                    save_id);
  bobgui_text_buffer_delete_mark (bobgui_text_mark_get_buffer (save_id),
                               save_id);
}
#endif

static void
paste_from_buffer (GdkClipboard      *clipboard,
                   ClipboardRequest  *request_data,
                   BobguiTextBuffer     *src_buffer,
                   const BobguiTextIter *start,
                   const BobguiTextIter *end)
{
  BobguiTextIter insert_point;
  BobguiTextBuffer *buffer = request_data->buffer;

  /* We're about to emit a bunch of signals, so be safe */
  g_object_ref (src_buffer);

  /* Replacing the selection with itself */
  if (request_data->replace_selection &&
      buffer == src_buffer)
    {
      /* Clear the paste point if needed */
      get_paste_point (buffer, &insert_point, TRUE);
      goto done;
    }

  if (request_data->interactive)
    bobgui_text_buffer_begin_user_action (buffer);

  pre_paste_prep (request_data, &insert_point);

  if (!bobgui_text_iter_equal (start, end))
    {
      if (!request_data->interactive ||
          (bobgui_text_iter_can_insert (&insert_point,
                                     request_data->default_editable)))
        bobgui_text_buffer_real_insert_range (buffer,
                                           &insert_point,
                                           start,
                                           end,
                                           request_data->interactive);
    }

  if (request_data->interactive)
    bobgui_text_buffer_end_user_action (buffer);

done:
  emit_paste_done (buffer, clipboard);

  g_object_unref (src_buffer);

  free_clipboard_request (request_data);
}

static void
bobgui_text_buffer_paste_clipboard_finish (GObject      *source,
                                        GAsyncResult *result,
                                        gpointer      data)
{
  GdkClipboard *clipboard = GDK_CLIPBOARD (source);
  ClipboardRequest *request_data = data;
  BobguiTextBuffer *src_buffer;
  BobguiTextIter start, end;
  BobguiTextMark *paste_point_override;
  const GValue *value;

  value = gdk_clipboard_read_value_finish (clipboard, result, NULL);
  if (value == NULL)
    {
      /* Clear paste point override from empty middle-click paste */
      paste_point_override = bobgui_text_buffer_get_mark (request_data->buffer,
                                                       "bobgui_paste_point_override");
      if (paste_point_override != NULL)
        bobgui_text_buffer_delete_mark (request_data->buffer, paste_point_override);

      return;
    }

  src_buffer = g_value_get_object (value);

  if (bobgui_text_buffer_get_selection_bounds (src_buffer, &start, &end))
    paste_from_buffer (clipboard, request_data, src_buffer,
                       &start, &end);
}

typedef struct
{
  GdkClipboard *clipboard;
  guint ref_count;
} SelectionClipboard;

static void
update_selection_clipboards (BobguiTextBuffer *buffer)
{
  BobguiTextBufferPrivate *priv;
  BobguiTextIter start;
  BobguiTextIter end;
  GSList *l;

  priv = buffer->priv;

  if (bobgui_text_buffer_get_selection_bounds (buffer, &start, &end))
    {
      if (priv->selection_content)
        gdk_content_provider_content_changed (priv->selection_content);
      else
        priv->selection_content = bobgui_text_buffer_content_new (buffer);

      for (l = priv->selection_clipboards; l; l = l->next)
        {
          SelectionClipboard *selection_clipboard = l->data;
          gdk_clipboard_set_content (selection_clipboard->clipboard, priv->selection_content);
        }
    }
  else
    {
      if (priv->selection_content)
        {
          BOBGUI_TEXT_BUFFER_CONTENT (priv->selection_content)->text_buffer = NULL;
          for (l = priv->selection_clipboards; l; l = l->next)
            {
              SelectionClipboard *selection_clipboard = l->data;
              GdkClipboard *clipboard = selection_clipboard->clipboard;
              if (gdk_clipboard_get_content (clipboard) == priv->selection_content)
                gdk_clipboard_set_content (clipboard, NULL);
            }
          g_clear_object (&priv->selection_content);
        }
    }
}

static SelectionClipboard *
find_selection_clipboard (BobguiTextBuffer *buffer,
			  GdkClipboard  *clipboard)
{
  GSList *tmp_list = buffer->priv->selection_clipboards;
  while (tmp_list)
    {
      SelectionClipboard *selection_clipboard = tmp_list->data;
      if (selection_clipboard->clipboard == clipboard)
	return selection_clipboard;

      tmp_list = tmp_list->next;
    }

  return NULL;
}

/**
 * bobgui_text_buffer_add_selection_clipboard:
 * @buffer: a `BobguiTextBuffer`
 * @clipboard: a `GdkClipboard`
 *
 * Adds @clipboard to the list of clipboards in which the selection
 * contents of @buffer are available.
 *
 * In most cases, @clipboard will be the `GdkClipboard` returned by
 * [method@Bobgui.Widget.get_primary_clipboard] for a view of @buffer.
 */
void
bobgui_text_buffer_add_selection_clipboard (BobguiTextBuffer *buffer,
					 GdkClipboard  *clipboard)
{
  SelectionClipboard *selection_clipboard;

  g_return_if_fail (BOBGUI_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (clipboard != NULL);

  selection_clipboard = find_selection_clipboard (buffer, clipboard);
  if (selection_clipboard)
    {
      selection_clipboard->ref_count++;
    }
  else
    {
      selection_clipboard = g_new (SelectionClipboard, 1);

      selection_clipboard->clipboard = clipboard;
      selection_clipboard->ref_count = 1;

      buffer->priv->selection_clipboards = g_slist_prepend (buffer->priv->selection_clipboards,
                                                            selection_clipboard);
    }
}

/**
 * bobgui_text_buffer_remove_selection_clipboard:
 * @buffer: a `BobguiTextBuffer`
 * @clipboard: a `GdkClipboard` added to @buffer by
 *   [method@Bobgui.TextBuffer.add_selection_clipboard]
 *
 * Removes a `GdkClipboard` added with
 * [method@Bobgui.TextBuffer.add_selection_clipboard]
 */
void
bobgui_text_buffer_remove_selection_clipboard (BobguiTextBuffer *buffer,
					    GdkClipboard  *clipboard)
{
  BobguiTextBufferPrivate *priv;
  SelectionClipboard *selection_clipboard;

  g_return_if_fail (BOBGUI_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (clipboard != NULL);

  selection_clipboard = find_selection_clipboard (buffer, clipboard);
  g_return_if_fail (selection_clipboard != NULL);

  priv = buffer->priv;
  selection_clipboard->ref_count--;
  if (selection_clipboard->ref_count == 0)
    {
      if (priv->selection_content &&
          gdk_clipboard_get_content (clipboard) == priv->selection_content)
	gdk_clipboard_set_content (clipboard, NULL);

      buffer->priv->selection_clipboards = g_slist_remove (buffer->priv->selection_clipboards,
                                                           selection_clipboard);

      g_free (selection_clipboard);
    }
}

static void
remove_all_selection_clipboards (BobguiTextBuffer *buffer)
{
  BobguiTextBufferPrivate *priv = buffer->priv;
  GSList *l;

  for (l = priv->selection_clipboards; l != NULL; l = l->next)
    {
      SelectionClipboard *selection_clipboard = l->data;
      g_free (selection_clipboard);
    }

  g_slist_free (priv->selection_clipboards);
  priv->selection_clipboards = NULL;
}

/**
 * bobgui_text_buffer_paste_clipboard:
 * @buffer: a `BobguiTextBuffer`
 * @clipboard: the `GdkClipboard` to paste from
 * @override_location: (nullable): location to insert pasted text
 * @default_editable: whether the buffer is editable by default
 *
 * Pastes the contents of a clipboard.
 *
 * If @override_location is %NULL, the pasted text will be inserted
 * at the cursor position, or the buffer selection will be replaced
 * if the selection is non-empty.
 *
 * Note: pasting is asynchronous, that is, we’ll ask for the paste data
 * and return, and at some point later after the main loop runs, the paste
 * data will be inserted.
 */
void
bobgui_text_buffer_paste_clipboard (BobguiTextBuffer *buffer,
				 GdkClipboard  *clipboard,
				 BobguiTextIter   *override_location,
                                 gboolean       default_editable)
{
  ClipboardRequest *data = g_new (ClipboardRequest, 1);
  BobguiTextIter paste_point;
  BobguiTextIter start, end;

  if (override_location != NULL)
    bobgui_text_buffer_create_mark (buffer,
                                 "bobgui_paste_point_override",
                                 override_location, FALSE);

  data->buffer = g_object_ref (buffer);
  data->interactive = TRUE;
  data->default_editable = !!default_editable;

  /* When pasting with the cursor inside the selection area, you
   * replace the selection with the new text, otherwise, you
   * simply insert the new text at the point where the click
   * occurred, unselecting any selected text. The replace_selection
   * flag toggles this behavior.
   */
  data->replace_selection = FALSE;

  get_paste_point (buffer, &paste_point, FALSE);
  if (bobgui_text_buffer_get_selection_bounds (buffer, &start, &end) &&
      (bobgui_text_iter_in_range (&paste_point, &start, &end) ||
       bobgui_text_iter_equal (&paste_point, &end)))
    data->replace_selection = TRUE;

  gdk_clipboard_read_value_async (clipboard,
                                  BOBGUI_TYPE_TEXT_BUFFER,
                                  G_PRIORITY_DEFAULT,
                                  NULL,
                                  bobgui_text_buffer_paste_clipboard_finish,
                                  data);
}

/**
 * bobgui_text_buffer_delete_selection:
 * @buffer: a `BobguiTextBuffer`
 * @interactive: whether the deletion is caused by user interaction
 * @default_editable: whether the buffer is editable by default
 *
 * Deletes the range between the “insert” and “selection_bound” marks,
 * that is, the currently-selected text.
 *
 * If @interactive is %TRUE, the editability of the selection will be
 * considered (users can’t delete uneditable text).
 *
 * Returns: whether there was a non-empty selection to delete
 */
gboolean
bobgui_text_buffer_delete_selection (BobguiTextBuffer *buffer,
                                  gboolean interactive,
                                  gboolean default_editable)
{
  BobguiTextIter start;
  BobguiTextIter end;

  if (!bobgui_text_buffer_get_selection_bounds (buffer, &start, &end))
    {
      return FALSE; /* No selection */
    }
  else
    {
      if (interactive)
        bobgui_text_buffer_delete_interactive (buffer, &start, &end, default_editable);
      else
        bobgui_text_buffer_delete (buffer, &start, &end);

      return TRUE; /* We deleted stuff */
    }
}

/**
 * bobgui_text_buffer_backspace:
 * @buffer: a `BobguiTextBuffer`
 * @iter: a position in @buffer
 * @interactive: whether the deletion is caused by user interaction
 * @default_editable: whether the buffer is editable by default
 *
 * Performs the appropriate action as if the user hit the delete
 * key with the cursor at the position specified by @iter.
 *
 * In the normal case a single character will be deleted, but when
 * combining accents are involved, more than one character can
 * be deleted, and when precomposed character and accent combinations
 * are involved, less than one character will be deleted.
 *
 * Because the buffer is modified, all outstanding iterators become
 * invalid after calling this function; however, the @iter will be
 * re-initialized to point to the location where text was deleted.
 *
 * Returns: %TRUE if the buffer was modified
 */
gboolean
bobgui_text_buffer_backspace (BobguiTextBuffer *buffer,
			   BobguiTextIter   *iter,
			   gboolean       interactive,
			   gboolean       default_editable)
{
  char *cluster_text;
  BobguiTextIter start;
  BobguiTextIter end;
  gboolean retval = FALSE;
  const PangoLogAttr *attrs;
  int offset;
  gboolean backspace_deletes_character;

  g_return_val_if_fail (BOBGUI_IS_TEXT_BUFFER (buffer), FALSE);
  g_return_val_if_fail (iter != NULL, FALSE);

  start = *iter;
  end = *iter;

  attrs = _bobgui_text_buffer_get_line_log_attrs (buffer, &start, NULL);
  offset = bobgui_text_iter_get_line_offset (&start);
  backspace_deletes_character = attrs[offset].backspace_deletes_character;

  bobgui_text_iter_backward_cursor_position (&start);

  if (bobgui_text_iter_equal (&start, &end))
    return FALSE;

  cluster_text = bobgui_text_iter_get_text (&start, &end);

  if (interactive)
    bobgui_text_buffer_begin_user_action (buffer);

  if (bobgui_text_buffer_delete_interactive (buffer, &start, &end,
					  default_editable))
    {
      /* special case \r\n, since we never want to reinsert \r */
      if (backspace_deletes_character && strcmp ("\r\n", cluster_text))
	{
	  char *normalized_text = g_utf8_normalize (cluster_text,
						     strlen (cluster_text),
						     G_NORMALIZE_NFD);
	  glong len = g_utf8_strlen (normalized_text, -1);

	  if (len > 1)
	    bobgui_text_buffer_insert_interactive (buffer,
						&start,
						normalized_text,
						g_utf8_offset_to_pointer (normalized_text, len - 1) - normalized_text,
						default_editable);

	  g_free (normalized_text);
	}

      retval = TRUE;
    }

  if (interactive)
    bobgui_text_buffer_end_user_action (buffer);

  g_free (cluster_text);

  /* Revalidate the users iter */
  *iter = start;

  return retval;
}

static void
cut_or_copy (BobguiTextBuffer *buffer,
	     GdkClipboard  *clipboard,
             gboolean       delete_region_after,
             gboolean       interactive,
             gboolean       default_editable)
{
  /* We prefer to cut the selected region between selection_bound and
   * insertion point. If that region is empty, then we cut the region
   * between the "anchor" and the insertion point (this is for
   * C-space and M-w and other Emacs-style copy/yank keys). Note that
   * insert and selection_bound are guaranteed to exist, but the
   * anchor only exists sometimes.
   */
  BobguiTextIter start;
  BobguiTextIter end;

  if (!bobgui_text_buffer_get_selection_bounds (buffer, &start, &end))
    {
      /* Let's try the anchor thing */
      BobguiTextMark * anchor = bobgui_text_buffer_get_mark (buffer, "anchor");

      if (anchor == NULL)
        return;
      else
        {
          bobgui_text_buffer_get_iter_at_mark (buffer, &end, anchor);
          bobgui_text_iter_order (&start, &end);
        }
    }

  if (!bobgui_text_iter_equal (&start, &end))
    {
      BobguiTextBuffer *contents;

      contents = create_clipboard_contents_buffer (buffer, &start, &end);
      gdk_clipboard_set (clipboard, BOBGUI_TYPE_TEXT_BUFFER, contents);
      g_object_unref (contents);

      if (delete_region_after)
        {
          if (interactive)
            bobgui_text_buffer_delete_interactive (buffer, &start, &end,
                                                default_editable);
          else
            bobgui_text_buffer_delete (buffer, &start, &end);
        }
    }
}

/**
 * bobgui_text_buffer_get_selection_content:
 * @buffer: a `BobguiTextBuffer`
 *
 * Get a content provider for this buffer.
 *
 * It can be used to make the content of @buffer available
 * in a `GdkClipboard`, see [method@Gdk.Clipboard.set_content].
 *
 * Returns: (transfer full): a new `GdkContentProvider`.
 */
GdkContentProvider *
bobgui_text_buffer_get_selection_content (BobguiTextBuffer *buffer)
{
  return bobgui_text_buffer_content_new (buffer);
}


/**
 * bobgui_text_buffer_cut_clipboard:
 * @buffer: a `BobguiTextBuffer`
 * @clipboard: the `GdkClipboard` object to cut to
 * @default_editable: default editability of the buffer
 *
 * Copies the currently-selected text to a clipboard,
 * then deletes said text if it’s editable.
 */
void
bobgui_text_buffer_cut_clipboard (BobguiTextBuffer *buffer,
			       GdkClipboard  *clipboard,
                               gboolean       default_editable)
{
  bobgui_text_buffer_begin_user_action (buffer);
  cut_or_copy (buffer, clipboard, TRUE, TRUE, default_editable);
  bobgui_text_buffer_end_user_action (buffer);
}

/**
 * bobgui_text_buffer_copy_clipboard:
 * @buffer: a `BobguiTextBuffer`
 * @clipboard: the `GdkClipboard` object to copy to
 *
 * Copies the currently-selected text to a clipboard.
 */
void
bobgui_text_buffer_copy_clipboard (BobguiTextBuffer *buffer,
				GdkClipboard  *clipboard)
{
  cut_or_copy (buffer, clipboard, FALSE, TRUE, TRUE);
}

/**
 * bobgui_text_buffer_get_selection_bounds:
 * @buffer: a `BobguiTextBuffer` a `BobguiTextBuffer`
 * @start: (out): iterator to initialize with selection start
 * @end: (out): iterator to initialize with selection end
 *
 * Returns %TRUE if some text is selected; places the bounds
 * of the selection in @start and @end.
 *
 * If the selection has length 0, then @start and @end are filled
 * in with the same value. @start and @end will be in ascending order.
 * If @start and @end are %NULL, then they are not filled in, but the
 * return value still indicates whether text is selected.
 *
 * Returns: whether the selection has nonzero length
 */
gboolean
bobgui_text_buffer_get_selection_bounds (BobguiTextBuffer *buffer,
                                      BobguiTextIter   *start,
                                      BobguiTextIter   *end)
{
  g_return_val_if_fail (BOBGUI_IS_TEXT_BUFFER (buffer), FALSE);

  return _bobgui_text_btree_get_selection_bounds (get_btree (buffer), start, end);
}

/**
 * bobgui_text_buffer_begin_user_action:
 * @buffer: a `BobguiTextBuffer`
 *
 * Called to indicate that the buffer operations between here and a
 * call to bobgui_text_buffer_end_user_action() are part of a single
 * user-visible operation.
 *
 * The operations between bobgui_text_buffer_begin_user_action() and
 * bobgui_text_buffer_end_user_action() can then be grouped when creating
 * an undo stack. `BobguiTextBuffer` maintains a count of calls to
 * bobgui_text_buffer_begin_user_action() that have not been closed with
 * a call to bobgui_text_buffer_end_user_action(), and emits the
 * “begin-user-action” and “end-user-action” signals only for the
 * outermost pair of calls. This allows you to build user actions
 * from other user actions.
 *
 * The “interactive” buffer mutation functions, such as
 * [method@Bobgui.TextBuffer.insert_interactive], automatically call
 * begin/end user action around the buffer operations they perform,
 * so there's no need to add extra calls if you user action consists
 * solely of a single call to one of those functions.
 */
void
bobgui_text_buffer_begin_user_action (BobguiTextBuffer *buffer)
{
  g_return_if_fail (BOBGUI_IS_TEXT_BUFFER (buffer));

  buffer->priv->user_action_count += 1;

  if (buffer->priv->user_action_count == 1)
    {
      /* Outermost nested user action begin emits the signal */
      bobgui_text_history_begin_user_action (buffer->priv->history);
      g_signal_emit (buffer, signals[BEGIN_USER_ACTION], 0);
    }
}

/**
 * bobgui_text_buffer_end_user_action:
 * @buffer: a `BobguiTextBuffer`
 *
 * Ends a user-visible operation.
 *
 * Should be paired with a call to
 * [method@Bobgui.TextBuffer.begin_user_action].
 * See that function for a full explanation.
 */
void
bobgui_text_buffer_end_user_action (BobguiTextBuffer *buffer)
{
  g_return_if_fail (BOBGUI_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (buffer->priv->user_action_count > 0);

  buffer->priv->user_action_count -= 1;

  if (buffer->priv->user_action_count == 0)
    {
      /* Ended the outermost-nested user action end, so emit the signal */
      g_signal_emit (buffer, signals[END_USER_ACTION], 0);
      bobgui_text_history_end_user_action (buffer->priv->history);
    }
}

/*
 * Logical attribute cache
 */

#define ATTR_CACHE_SIZE 2

typedef struct _CacheEntry CacheEntry;
struct _CacheEntry
{
  int line;
  int char_len;
  PangoLogAttr *attrs;
};

struct _BobguiTextLogAttrCache
{
  int chars_changed_stamp;
  CacheEntry entries[ATTR_CACHE_SIZE];
};

static void
free_log_attr_cache (BobguiTextLogAttrCache *cache)
{
  int i;

  for (i = 0; i < ATTR_CACHE_SIZE; i++)
    g_free (cache->entries[i].attrs);

  g_free (cache);
}

static void
clear_log_attr_cache (BobguiTextLogAttrCache *cache)
{
  int i;

  for (i = 0; i < ATTR_CACHE_SIZE; i++)
    {
      g_free (cache->entries[i].attrs);
      cache->entries[i].attrs = NULL;
    }
}

static PangoLogAttr*
compute_log_attrs (const BobguiTextIter *iter,
                   int               *char_lenp)
{
  BobguiTextIter start;
  BobguiTextIter end;
  char *paragraph;
  int char_len, byte_len;
  PangoLogAttr *attrs = NULL;

  start = *iter;
  end = *iter;

  bobgui_text_iter_set_line_offset (&start, 0);
  bobgui_text_iter_forward_line (&end);

  paragraph = bobgui_text_iter_get_slice (&start, &end);
  char_len = g_utf8_strlen (paragraph, -1);
  byte_len = strlen (paragraph);

  if (char_lenp != NULL)
    *char_lenp = char_len;

  attrs = g_new (PangoLogAttr, char_len + 1);

  /* FIXME we need to follow PangoLayout and allow different language
   * tags within the paragraph
   */
  pango_get_log_attrs (paragraph, byte_len, -1,
		       bobgui_text_iter_get_language (&start),
                       attrs,
                       char_len + 1);

  g_free (paragraph);

  return attrs;
}

/* The return value from this is valid until you call this a second time.
 * Returns (char_len + 1) PangoLogAttr's, one for each text position.
 */
const PangoLogAttr *
_bobgui_text_buffer_get_line_log_attrs (BobguiTextBuffer     *buffer,
                                     const BobguiTextIter *anywhere_in_line,
                                     int               *char_len)
{
  BobguiTextBufferPrivate *priv;
  int line;
  BobguiTextLogAttrCache *cache;
  int i;

  g_return_val_if_fail (BOBGUI_IS_TEXT_BUFFER (buffer), NULL);
  g_return_val_if_fail (anywhere_in_line != NULL, NULL);

  priv = buffer->priv;

  /* FIXME we also need to recompute log attrs if the language tag at
   * the start of a paragraph changes
   */

  if (priv->log_attr_cache == NULL)
    {
      priv->log_attr_cache = g_new0 (BobguiTextLogAttrCache, 1);
      priv->log_attr_cache->chars_changed_stamp =
        _bobgui_text_btree_get_chars_changed_stamp (get_btree (buffer));
    }
  else if (priv->log_attr_cache->chars_changed_stamp !=
           _bobgui_text_btree_get_chars_changed_stamp (get_btree (buffer)))
    {
      clear_log_attr_cache (priv->log_attr_cache);
    }

  cache = priv->log_attr_cache;
  line = bobgui_text_iter_get_line (anywhere_in_line);

  for (i = 0; i < ATTR_CACHE_SIZE; i++)
    {
      if (cache->entries[i].attrs != NULL &&
          cache->entries[i].line == line)
        {
          if (char_len != NULL)
            *char_len = cache->entries[i].char_len;
          return cache->entries[i].attrs;
        }
    }

  /* Not in cache; open up the first cache entry */
  g_free (cache->entries[ATTR_CACHE_SIZE-1].attrs);

  memmove (cache->entries + 1, cache->entries,
           sizeof (CacheEntry) * (ATTR_CACHE_SIZE - 1));

  cache->entries[0].line = line;
  cache->entries[0].attrs = compute_log_attrs (anywhere_in_line,
                                               &cache->entries[0].char_len);

  if (char_len != NULL)
    *char_len = cache->entries[0].char_len;

  return cache->entries[0].attrs;
}

void
_bobgui_text_buffer_notify_will_remove_tag (BobguiTextBuffer *buffer,
                                         BobguiTextTag    *tag)
{
  /* This removes tag from the buffer, but DOESN'T emit the
   * remove-tag signal, because we can't afford to have user
   * code messing things up at this point; the tag MUST be removed
   * entirely.
   */
  if (buffer->priv->btree)
    _bobgui_text_btree_notify_will_remove_tag (buffer->priv->btree, tag);
}

/*
 * Debug spew
 */

void
_bobgui_text_buffer_spew (BobguiTextBuffer *buffer)
{
  _bobgui_text_btree_spew (get_btree (buffer));
}

static void
insert_tags_for_attributes (BobguiTextBuffer     *buffer,
                            PangoAttrIterator *iter,
                            BobguiTextIter       *start,
                            BobguiTextIter       *end)
{
  BobguiTextTagTable *table;
  GSList *attrs, *l;
  BobguiTextTag *tag;
  char name[256];
  float fg_alpha, bg_alpha;

  table = bobgui_text_buffer_get_tag_table (buffer);

#define LANGUAGE_ATTR(attr_name) \
    { \
      const char *language = pango_language_to_string (((PangoAttrLanguage*)attr)->value); \
      g_snprintf (name, 256, "language=%s", language); \
      tag = bobgui_text_tag_table_lookup (table, name); \
      if (!tag) \
        { \
          tag = bobgui_text_tag_new (name); \
          g_object_set (tag, #attr_name, language, NULL); \
          bobgui_text_tag_table_add (table, tag); \
          g_object_unref (tag); \
        } \
      bobgui_text_buffer_apply_tag (buffer, tag, start, end); \
    }

#define STRING_ATTR(attr_name) \
    { \
      const char *string = ((PangoAttrString*)attr)->value; \
      g_snprintf (name, 256, #attr_name "=%s", string); \
      tag = bobgui_text_tag_table_lookup (table, name); \
      if (!tag) \
        { \
          tag = bobgui_text_tag_new (name); \
          g_object_set (tag, #attr_name, string, NULL); \
          bobgui_text_tag_table_add (table, tag); \
          g_object_unref (tag); \
        } \
      bobgui_text_buffer_apply_tag (buffer, tag, start, end); \
    }

#define INT_ATTR(attr_name) \
    { \
      int value = ((PangoAttrInt*)attr)->value; \
      g_snprintf (name, 256, #attr_name "=%d", value); \
      tag = bobgui_text_tag_table_lookup (table, name); \
      if (!tag) \
        { \
          tag = bobgui_text_tag_new (name); \
          g_object_set (tag, #attr_name, value, NULL); \
          bobgui_text_tag_table_add (table, tag); \
          g_object_unref (tag); \
        } \
      bobgui_text_buffer_apply_tag (buffer, tag, start, end); \
    }

#define FONT_ATTR(attr_name) \
    { \
      PangoFontDescription *desc = ((PangoAttrFontDesc*)attr)->desc; \
      char *str = pango_font_description_to_string (desc); \
      g_snprintf (name, 256, "font-desc=%s", str); \
      g_free (str); \
      tag = bobgui_text_tag_table_lookup (table, name); \
      if (!tag) \
        { \
          tag = bobgui_text_tag_new (name); \
          g_object_set (tag, #attr_name, desc, NULL); \
          bobgui_text_tag_table_add (table, tag); \
          g_object_unref (tag); \
        } \
      bobgui_text_buffer_apply_tag (buffer, tag, start, end); \
    }

#define FLOAT_ATTR(attr_name) \
    { \
      float value = ((PangoAttrFloat*)attr)->value; \
      g_snprintf (name, 256, #attr_name "=%g", value); \
      tag = bobgui_text_tag_table_lookup (table, name); \
      if (!tag) \
        { \
          tag = bobgui_text_tag_new (name); \
          g_object_set (tag, #attr_name, value, NULL); \
          bobgui_text_tag_table_add (table, tag); \
          g_object_unref (tag); \
        } \
      bobgui_text_buffer_apply_tag (buffer, tag, start, end); \
    }

#define RGBA_ATTR(attr_name, alpha_value) \
    { \
      PangoColor *color; \
      GdkRGBA rgba; \
      color = &((PangoAttrColor*)attr)->color; \
      rgba.red = color->red / 65535.; \
      rgba.green = color->green / 65535.; \
      rgba.blue = color->blue / 65535.; \
      rgba.alpha = alpha_value; \
      char *str = gdk_rgba_to_string (&rgba); \
      g_snprintf (name, 256, #attr_name "=%s", str); \
      g_free (str); \
      tag = bobgui_text_tag_table_lookup (table, name); \
      if (!tag) \
        { \
          tag = bobgui_text_tag_new (name); \
          g_object_set (tag, #attr_name, &rgba, NULL); \
          bobgui_text_tag_table_add (table, tag); \
          g_object_unref (tag); \
        } \
      bobgui_text_buffer_apply_tag (buffer, tag, start, end); \
    }

#define VOID_ATTR(attr_name) \
    { \
      tag = bobgui_text_tag_table_lookup (table, #attr_name); \
      if (!tag) \
        { \
          tag = bobgui_text_tag_new (#attr_name); \
          g_object_set (tag, #attr_name, TRUE, NULL); \
          bobgui_text_tag_table_add (table, tag); \
          g_object_unref (tag); \
        } \
      bobgui_text_buffer_apply_tag (buffer, tag, start, end); \
    }

  fg_alpha = bg_alpha = 1.;
  attrs = pango_attr_iterator_get_attrs (iter);
  for (l = attrs; l; l = l->next)
    {
      PangoAttribute *attr = l->data;

      switch ((int)attr->klass->type)
        {
        case PANGO_ATTR_FOREGROUND_ALPHA:
          fg_alpha = ((PangoAttrInt*)attr)->value / 65535.;
          break;

        case PANGO_ATTR_BACKGROUND_ALPHA:
          bg_alpha = ((PangoAttrInt*)attr)->value / 65535.;
          break;

        default:
          break;
        }
    }

  for (l = attrs; l; l = l->next)
    {
      PangoAttribute *attr = l->data;

      switch (attr->klass->type)
        {
        case PANGO_ATTR_LANGUAGE:
          LANGUAGE_ATTR (language);
          break;

        case PANGO_ATTR_FAMILY:
          STRING_ATTR (family);
          break;

        case PANGO_ATTR_STYLE:
          INT_ATTR (style);
          break;

        case PANGO_ATTR_WEIGHT:
          INT_ATTR (weight);
          break;

        case PANGO_ATTR_VARIANT:
          INT_ATTR (variant);
          break;

        case PANGO_ATTR_STRETCH:
          INT_ATTR (stretch);
          break;

        case PANGO_ATTR_SIZE:
          INT_ATTR (size);
          break;

        case PANGO_ATTR_FONT_DESC:
          FONT_ATTR (font-desc);
          break;

        case PANGO_ATTR_FOREGROUND:
          RGBA_ATTR (foreground_rgba, fg_alpha);
          break;

        case PANGO_ATTR_BACKGROUND:
          RGBA_ATTR (background_rgba, bg_alpha);
          break;

        case PANGO_ATTR_UNDERLINE:
          INT_ATTR (underline);
          break;

        case PANGO_ATTR_UNDERLINE_COLOR:
          RGBA_ATTR (underline_rgba, fg_alpha);
          break;

        case PANGO_ATTR_OVERLINE:
          INT_ATTR (overline);
          break;

        case PANGO_ATTR_OVERLINE_COLOR:
          RGBA_ATTR (overline_rgba, fg_alpha);
          break;

        case PANGO_ATTR_STRIKETHROUGH:
          INT_ATTR (strikethrough);
          break;

        case PANGO_ATTR_STRIKETHROUGH_COLOR:
          RGBA_ATTR (strikethrough_rgba, fg_alpha);
          break;

        case PANGO_ATTR_RISE:
          INT_ATTR (rise);
          break;

        case PANGO_ATTR_SCALE:
          FLOAT_ATTR (scale);
          break;

        case PANGO_ATTR_FALLBACK:
          INT_ATTR (fallback);
          break;

        case PANGO_ATTR_LETTER_SPACING:
          INT_ATTR (letter_spacing);
          break;

        case PANGO_ATTR_LINE_HEIGHT:
          FLOAT_ATTR (line_height);
          break;

        case PANGO_ATTR_ABSOLUTE_LINE_HEIGHT:
          break;

        case PANGO_ATTR_FONT_FEATURES:
          STRING_ATTR (font_features);
          break;

        case PANGO_ATTR_ALLOW_BREAKS:
          INT_ATTR (allow_breaks);
          break;

        case PANGO_ATTR_SHOW:
          INT_ATTR (show_spaces);
          break;

        case PANGO_ATTR_INSERT_HYPHENS:
          INT_ATTR (insert_hyphens);
          break;

        case PANGO_ATTR_TEXT_TRANSFORM:
          INT_ATTR (text_transform);
          break;

        case PANGO_ATTR_WORD:
          VOID_ATTR (word);
          break;

        case PANGO_ATTR_SENTENCE:
          VOID_ATTR (sentence);
          break;

        case PANGO_ATTR_BASELINE_SHIFT:
          INT_ATTR (baseline_shift);
          break;

        case PANGO_ATTR_FONT_SCALE:
          INT_ATTR (font_scale);
          break;

        case PANGO_ATTR_SHAPE:
        case PANGO_ATTR_ABSOLUTE_SIZE:
        case PANGO_ATTR_GRAVITY:
        case PANGO_ATTR_GRAVITY_HINT:
        case PANGO_ATTR_FOREGROUND_ALPHA:
        case PANGO_ATTR_BACKGROUND_ALPHA:
          break;

        case PANGO_ATTR_INVALID:
        default:
          g_assert_not_reached ();
          break;
        }
    }

  g_slist_free_full (attrs, (GDestroyNotify)pango_attribute_destroy);

#undef LANGUAGE_ATTR
#undef STRING_ATTR
#undef INT_ATTR
#undef FONT_ATTR
#undef FLOAT_ATTR
#undef RGBA_ATTR
}

static void
bobgui_text_buffer_insert_with_attributes (BobguiTextBuffer *buffer,
                                        BobguiTextIter   *iter,
                                        const char    *text,
                                        PangoAttrList *attributes)
{
  BobguiTextMark *mark;
  PangoAttrIterator *attr;

  g_return_if_fail (BOBGUI_IS_TEXT_BUFFER (buffer));

  if (!attributes)
    {
      bobgui_text_buffer_insert (buffer, iter, text, -1);
      return;
    }

  /* create mark with right gravity */
  mark = bobgui_text_buffer_create_mark (buffer, NULL, iter, FALSE);
  attr = pango_attr_list_get_iterator (attributes);

  do
    {
      int start, end;
      int start_offset;
      BobguiTextIter start_iter;

      pango_attr_iterator_range (attr, &start, &end);

      if (end == G_MAXINT) /* last chunk */
        end = start - 1; /* resulting in -1 to be passed to _insert */

      start_offset = bobgui_text_iter_get_offset (iter);
      bobgui_text_buffer_insert (buffer, iter, text + start, end - start);
      bobgui_text_buffer_get_iter_at_offset (buffer, &start_iter, start_offset);

      insert_tags_for_attributes (buffer, attr, &start_iter, iter);

      bobgui_text_buffer_get_iter_at_mark (buffer, iter, mark);
    }
  while (pango_attr_iterator_next (attr));

  bobgui_text_buffer_delete_mark (buffer, mark);
  pango_attr_iterator_destroy (attr);
}

/**
 * bobgui_text_buffer_insert_markup:
 * @buffer: a `BobguiTextBuffer`
 * @iter: location to insert the markup
 * @markup: a nul-terminated UTF-8 string containing Pango markup
 * @len: length of @markup in bytes, or -1
 *
 * Inserts the text in @markup at position @iter.
 *
 * @markup will be inserted in its entirety and must be nul-terminated
 * and valid UTF-8. Emits the [signal@Bobgui.TextBuffer::insert-text] signal,
 * possibly multiple times; insertion actually occurs in the default handler
 * for the signal. @iter will point to the end of the inserted text on return.
 */
void
bobgui_text_buffer_insert_markup (BobguiTextBuffer *buffer,
                               BobguiTextIter   *iter,
                               const char    *markup,
                               int            len)
{
  PangoAttrList *attributes;
  char *text;
  GError *error = NULL;

  if (!pango_parse_markup (markup, len, 0, &attributes, &text, NULL, &error))
    {
      g_warning ("Invalid markup string: %s", error->message);
      g_error_free (error);
      return;
    }

  bobgui_text_buffer_insert_with_attributes (buffer, iter, text, attributes);

  pango_attr_list_unref (attributes);
  g_free (text);
}

static void
bobgui_text_buffer_real_undo (BobguiTextBuffer *buffer)
{
  if (bobgui_text_history_get_can_undo (buffer->priv->history))
    bobgui_text_history_undo (buffer->priv->history);
}

static void
bobgui_text_buffer_real_redo (BobguiTextBuffer *buffer)
{
  if (bobgui_text_history_get_can_redo (buffer->priv->history))
    bobgui_text_history_redo (buffer->priv->history);
}

/**
 * bobgui_text_buffer_get_can_undo:
 * @buffer: a `BobguiTextBuffer`
 *
 * Gets whether there is an undoable action in the history.
 *
 * Returns: %TRUE if there is an undoable action
 */
gboolean
bobgui_text_buffer_get_can_undo (BobguiTextBuffer *buffer)
{
  g_return_val_if_fail (BOBGUI_IS_TEXT_BUFFER (buffer), FALSE);

  return bobgui_text_history_get_can_undo (buffer->priv->history);
}

/**
 * bobgui_text_buffer_get_can_redo:
 * @buffer: a `BobguiTextBuffer`
 *
 * Gets whether there is a redoable action in the history.
 *
 * Returns: %TRUE if there is a redoable action
 */
gboolean
bobgui_text_buffer_get_can_redo (BobguiTextBuffer *buffer)
{
  g_return_val_if_fail (BOBGUI_IS_TEXT_BUFFER (buffer), FALSE);

  return bobgui_text_history_get_can_redo (buffer->priv->history);
}

static void
bobgui_text_buffer_history_change_state (gpointer funcs_data,
                                      gboolean is_modified,
                                      gboolean can_undo,
                                      gboolean can_redo)
{
  BobguiTextBuffer *buffer = funcs_data;

  if (buffer->priv->can_undo != can_undo)
    {
      buffer->priv->can_undo = can_undo;
      g_object_notify_by_pspec (G_OBJECT (buffer), text_buffer_props[PROP_CAN_UNDO]);
    }

  if (buffer->priv->can_redo != can_redo)
    {
      buffer->priv->can_redo = can_redo;
      g_object_notify_by_pspec (G_OBJECT (buffer), text_buffer_props[PROP_CAN_REDO]);
    }

  if (buffer->priv->modified != is_modified)
    bobgui_text_buffer_set_modified (buffer, is_modified);
}

static void
bobgui_text_buffer_history_insert (gpointer    funcs_data,
                                guint       begin,
                                guint       end,
                                const char *text,
                                guint       len)
{
  BobguiTextBuffer *buffer = funcs_data;
  BobguiTextIter iter;

  bobgui_text_buffer_get_iter_at_offset (buffer, &iter, begin);
  bobgui_text_buffer_insert (buffer, &iter, text, len);
}

static void
bobgui_text_buffer_history_delete (gpointer    funcs_data,
                                guint       begin,
                                guint       end,
                                const char *expected_text,
                                guint       len)
{
  BobguiTextBuffer *buffer = funcs_data;
  BobguiTextIter iter;
  BobguiTextIter end_iter;

  bobgui_text_buffer_get_iter_at_offset (buffer, &iter, begin);
  bobgui_text_buffer_get_iter_at_offset (buffer, &end_iter, end);
  bobgui_text_buffer_delete (buffer, &iter, &end_iter);
}

static void
bobgui_text_buffer_history_select (gpointer funcs_data,
                                int      selection_insert,
                                int      selection_bound)
{
  BobguiTextBuffer *buffer = funcs_data;
  BobguiTextIter insert;
  BobguiTextIter bound;

  if (selection_insert == -1 || selection_bound == -1)
    return;

  bobgui_text_buffer_get_iter_at_offset (buffer, &insert, selection_insert);
  bobgui_text_buffer_get_iter_at_offset (buffer, &bound, selection_bound);
  bobgui_text_buffer_select_range (buffer, &insert, &bound);
}

/**
 * bobgui_text_buffer_undo:
 * @buffer: a `BobguiTextBuffer`
 *
 * Undoes the last undoable action on the buffer, if there is one.
 */
void
bobgui_text_buffer_undo (BobguiTextBuffer *buffer)
{
  g_return_if_fail (BOBGUI_IS_TEXT_BUFFER (buffer));

  if (bobgui_text_buffer_get_can_undo (buffer))
    g_signal_emit (buffer, signals[UNDO], 0);
}

/**
 * bobgui_text_buffer_redo:
 * @buffer: a `BobguiTextBuffer`
 *
 * Redoes the next redoable action on the buffer, if there is one.
 */
void
bobgui_text_buffer_redo (BobguiTextBuffer *buffer)
{
  g_return_if_fail (BOBGUI_IS_TEXT_BUFFER (buffer));

  if (bobgui_text_buffer_get_can_redo (buffer))
    g_signal_emit (buffer, signals[REDO], 0);
}

/**
 * bobgui_text_buffer_get_enable_undo:
 * @buffer: a `BobguiTextBuffer`
 *
 * Gets whether the buffer is saving modifications to the buffer
 * to allow for undo and redo actions.
 *
 * See [method@Bobgui.TextBuffer.begin_irreversible_action] and
 * [method@Bobgui.TextBuffer.end_irreversible_action] to create
 * changes to the buffer that cannot be undone.
 *
 * Returns: %TRUE if undoing and redoing changes to the buffer is allowed.
 */
gboolean
bobgui_text_buffer_get_enable_undo (BobguiTextBuffer *buffer)
{
  g_return_val_if_fail (BOBGUI_IS_TEXT_BUFFER (buffer), FALSE);

  return bobgui_text_history_get_enabled (buffer->priv->history);
}

/**
 * bobgui_text_buffer_set_enable_undo:
 * @buffer: a `BobguiTextBuffer`
 * @enable_undo: %TRUE to enable undo
 *
 * Sets whether or not to enable undoable actions in the text buffer.
 *
 * Undoable actions in this context are changes to the text content of
 * the buffer. Changes to tags and marks are not tracked.
 *
 * If enabled, the user will be able to undo the last number of actions
 * up to [method@Bobgui.TextBuffer.get_max_undo_levels].
 *
 * See [method@Bobgui.TextBuffer.begin_irreversible_action] and
 * [method@Bobgui.TextBuffer.end_irreversible_action] to create
 * changes to the buffer that cannot be undone.
 */
void
bobgui_text_buffer_set_enable_undo (BobguiTextBuffer *buffer,
                                 gboolean       enable_undo)
{
  g_return_if_fail (BOBGUI_IS_TEXT_BUFFER (buffer));

  if (enable_undo != bobgui_text_history_get_enabled (buffer->priv->history))
    {
      bobgui_text_history_set_enabled (buffer->priv->history, enable_undo);
      g_object_notify_by_pspec (G_OBJECT (buffer),
                                text_buffer_props[PROP_ENABLE_UNDO]);
    }
}

/**
 * bobgui_text_buffer_begin_irreversible_action:
 * @buffer: a `BobguiTextBuffer`
 *
 * Denotes the beginning of an action that may not be undone.
 *
 * This will cause any previous operations in the undo/redo queue
 * to be cleared.
 *
 * This should be paired with a call to
 * [method@Bobgui.TextBuffer.end_irreversible_action] after the irreversible
 * action has completed.
 *
 * You may nest calls to bobgui_text_buffer_begin_irreversible_action()
 * and bobgui_text_buffer_end_irreversible_action() pairs.
 */
void
bobgui_text_buffer_begin_irreversible_action (BobguiTextBuffer *buffer)
{
  g_return_if_fail (BOBGUI_IS_TEXT_BUFFER (buffer));

  bobgui_text_history_begin_irreversible_action (buffer->priv->history);
}

/**
 * bobgui_text_buffer_end_irreversible_action:
 * @buffer: a `BobguiTextBuffer`
 *
 * Denotes the end of an action that may not be undone.
 *
 * This will cause any previous operations in the undo/redo
 * queue to be cleared.
 *
 * This should be called after completing modifications to the
 * text buffer after [method@Bobgui.TextBuffer.begin_irreversible_action]
 * was called.
 *
 * You may nest calls to bobgui_text_buffer_begin_irreversible_action()
 * and bobgui_text_buffer_end_irreversible_action() pairs.
 */
void
bobgui_text_buffer_end_irreversible_action (BobguiTextBuffer *buffer)
{
  g_return_if_fail (BOBGUI_IS_TEXT_BUFFER (buffer));

  bobgui_text_history_end_irreversible_action (buffer->priv->history);
}

/**
 * bobgui_text_buffer_get_max_undo_levels:
 * @buffer: a `BobguiTextBuffer`
 *
 * Gets the maximum number of undo levels to perform.
 *
 * If 0, unlimited undo actions may be performed. Note that this may
 * have a memory usage impact as it requires storing an additional
 * copy of the inserted or removed text within the text buffer.
 *
 * Returns: The max number of undo levels allowed (0 indicates unlimited).
 */
guint
bobgui_text_buffer_get_max_undo_levels (BobguiTextBuffer *buffer)
{
  g_return_val_if_fail (BOBGUI_IS_TEXT_BUFFER (buffer), 0);

  return bobgui_text_history_get_max_undo_levels (buffer->priv->history);
}

/**
 * bobgui_text_buffer_set_max_undo_levels:
 * @buffer: a `BobguiTextBuffer`
 * @max_undo_levels: the maximum number of undo actions to perform
 *
 * Sets the maximum number of undo levels to perform.
 *
 * If 0, unlimited undo actions may be performed. Note that this may
 * have a memory usage impact as it requires storing an additional
 * copy of the inserted or removed text within the text buffer.
 */
void
bobgui_text_buffer_set_max_undo_levels (BobguiTextBuffer *buffer,
                                     guint          max_undo_levels)
{
  g_return_if_fail (BOBGUI_IS_TEXT_BUFFER (buffer));

  bobgui_text_history_set_max_undo_levels (buffer->priv->history, max_undo_levels);
}

const char *
bobgui_justification_to_string (BobguiJustification just)
{
  switch (just)
    {
    case BOBGUI_JUSTIFY_LEFT:
      return "left";
    case BOBGUI_JUSTIFY_RIGHT:
      return "right";
    case BOBGUI_JUSTIFY_CENTER:
      return "center";
    case BOBGUI_JUSTIFY_FILL:
      return "fill";
    default:
      g_assert_not_reached ();
    }
}

const char *
bobgui_text_direction_to_string (BobguiTextDirection direction)
{
  switch (direction)
    {
    case BOBGUI_TEXT_DIR_NONE:
      return "none";
    case BOBGUI_TEXT_DIR_LTR:
      return "ltr";
    case BOBGUI_TEXT_DIR_RTL:
      return "rtl";
    default:
      g_assert_not_reached ();
    }
}

const char *
bobgui_wrap_mode_to_string (BobguiWrapMode wrap_mode)
{
  /* Keep these in sync with pango_wrap_mode_to_string() */
  switch (wrap_mode)
    {
    case BOBGUI_WRAP_NONE:
      return "none";
    case BOBGUI_WRAP_CHAR:
      return "char";
    case BOBGUI_WRAP_WORD:
      return "word";
    case BOBGUI_WRAP_WORD_CHAR:
      return "word-char";
    default:
      g_assert_not_reached ();
    }
}

/*< private >
 * bobgui_text_buffer_add_run_attributes:
 * @buffer: the buffer to serialize
 * @offset: the offset into the text buffer
 * @attributes: a hash table of serialized text attributes
 * @start_offset: (out): the start offset for the attributes run
 * @end_offset: (out): the end offset for the attributes run
 *
 * Serializes the attributes inside a text buffer at the given offset.
 *
 * All attributes are serialized as key/value string pairs inside the
 * provided @attributes dictionary.
 *
 * The serialization format is private to BOBGUI and should not be
 * considered stable.
 */
void
bobgui_text_buffer_add_run_attributes (BobguiTextBuffer *buffer,
                                    int            offset,
                                    GHashTable    *attributes,
                                    int           *start_offset,
                                    int           *end_offset)
{
  BobguiTextIter iter;
  GSList *tags, *temp_tags;
  double scale = 1;
  gboolean val_set = FALSE;

  bobgui_text_buffer_get_iter_at_offset (buffer, &iter, offset);

  bobgui_text_iter_forward_to_tag_toggle (&iter, NULL);
  *end_offset = bobgui_text_iter_get_offset (&iter);

  bobgui_text_iter_backward_to_tag_toggle (&iter, NULL);
  *start_offset = bobgui_text_iter_get_offset (&iter);

  bobgui_text_buffer_get_iter_at_offset (buffer, &iter, offset);

  tags = bobgui_text_iter_get_tags (&iter);
  tags = g_slist_reverse (tags);

  temp_tags = tags;
  while (temp_tags && !val_set)
    {
      BobguiTextTag *tag = BOBGUI_TEXT_TAG (temp_tags->data);
      PangoStyle style;

      g_object_get (tag,
                    "style-set", &val_set,
                    "style", &style,
                    NULL);
      if (val_set)
        g_hash_table_insert (attributes,
                             g_strdup ("style"),
                             g_strdup (pango_style_to_string (style)));
      temp_tags = temp_tags->next;
    }
  val_set = FALSE;

  temp_tags = tags;
  while (temp_tags && !val_set)
    {
      BobguiTextTag *tag = BOBGUI_TEXT_TAG (temp_tags->data);
      PangoVariant variant;

      g_object_get (tag,
                    "variant-set", &val_set,
                    "variant", &variant,
                    NULL);
      if (val_set)
        g_hash_table_insert (attributes,
                             g_strdup ("variant"),
                             g_strdup (pango_variant_to_string (variant)));
      temp_tags = temp_tags->next;
    }
  val_set = FALSE;

  temp_tags = tags;
  while (temp_tags && !val_set)
    {
      BobguiTextTag *tag = BOBGUI_TEXT_TAG (temp_tags->data);
      PangoStretch stretch;

      g_object_get (tag,
                    "stretch-set", &val_set,
                    "stretch", &stretch,
                    NULL);
      if (val_set)
        g_hash_table_insert (attributes,
                             g_strdup ("stretch"),
                             g_strdup (pango_stretch_to_string (stretch)));
      temp_tags = temp_tags->next;
    }
  val_set = FALSE;

  temp_tags = tags;
  while (temp_tags && !val_set)
    {
      BobguiTextTag *tag = BOBGUI_TEXT_TAG (temp_tags->data);
      BobguiJustification justification;

      g_object_get (tag,
                    "justification-set", &val_set,
                    "justification", &justification,
                    NULL);
      if (val_set)
        g_hash_table_insert (attributes,
                             g_strdup ("justification"),
                             g_strdup (bobgui_justification_to_string (justification)));
      temp_tags = temp_tags->next;
    }
  val_set = FALSE;

  temp_tags = tags;
  while (temp_tags && !val_set)
    {
      BobguiTextTag *tag = BOBGUI_TEXT_TAG (temp_tags->data);
      BobguiTextDirection direction;

      g_object_get (tag, "direction", &direction, NULL);
      if (direction != BOBGUI_TEXT_DIR_NONE)
        {
          val_set = TRUE;
          g_hash_table_insert (attributes,
                               g_strdup ("direction"),
                               g_strdup (bobgui_text_direction_to_string (direction)));
        }
      temp_tags = temp_tags->next;
    }
  val_set = FALSE;

  temp_tags = tags;
  while (temp_tags && !val_set)
    {
      BobguiTextTag *tag = BOBGUI_TEXT_TAG (temp_tags->data);
      BobguiWrapMode wrap_mode;

      g_object_get (tag,
                    "wrap-mode-set", &val_set,
                    "wrap-mode", &wrap_mode,
                    NULL);
      if (val_set)
        g_hash_table_insert (attributes,
                             g_strdup ("wrap-mode"),
                             g_strdup (bobgui_wrap_mode_to_string (wrap_mode)));
      temp_tags = temp_tags->next;
    }
  val_set = FALSE;

  temp_tags = tags;
  while (temp_tags && !val_set)
    {
      BobguiTextTag *tag = BOBGUI_TEXT_TAG (temp_tags->data);

      g_object_get (tag, "foreground-set", &val_set, NULL);
      if (val_set)
        {
          GdkRGBA *rgba;
          char *value;

          g_object_get (tag, "foreground-rgba", &rgba, NULL);
          value = g_strdup_printf ("%u,%u,%u",
                                   (guint) rgba->red * 65535,
                                   (guint) rgba->green * 65535,
                                   (guint) rgba->blue * 65535);
          gdk_rgba_free (rgba);

          g_hash_table_insert (attributes, g_strdup ("fg-color"), value);
        }
      temp_tags = temp_tags->next;
    }
  val_set = FALSE;

  temp_tags = tags;
  while (temp_tags && !val_set)
    {
      BobguiTextTag *tag = BOBGUI_TEXT_TAG (temp_tags->data);

      g_object_get (tag, "background-set", &val_set, NULL);
      if (val_set)
        {
          GdkRGBA *rgba;
          char *value;

          g_object_get (tag, "background-rgba", &rgba, NULL);
          value = g_strdup_printf ("%u,%u,%u",
                                   (guint) rgba->red * 65535,
                                   (guint) rgba->green * 65535,
                                   (guint) rgba->blue * 65535);
          gdk_rgba_free (rgba);

          g_hash_table_insert (attributes, g_strdup ("bg-color"), value);
        }
      temp_tags = temp_tags->next;
    }
  val_set = FALSE;

  temp_tags = tags;
  while (temp_tags && !val_set)
    {
      BobguiTextTag *tag = BOBGUI_TEXT_TAG (temp_tags->data);

      g_object_get (tag, "family-set", &val_set, NULL);

      if (val_set)
        {
          char *value;
          g_object_get (tag, "family", &value, NULL);

          g_hash_table_insert (attributes, g_strdup ("family-name"), value);
        }
      temp_tags = temp_tags->next;
    }
  val_set = FALSE;

  temp_tags = tags;
  while (temp_tags && !val_set)
    {
      BobguiTextTag *tag = BOBGUI_TEXT_TAG (temp_tags->data);

      g_object_get (tag, "language-set", &val_set, NULL);

      if (val_set)
        {
          char *value;
          g_object_get (tag, "language", &value, NULL);
          g_hash_table_insert (attributes, g_strdup ("language"), value);
        }
      temp_tags = temp_tags->next;
    }
  val_set = FALSE;

  temp_tags = tags;
  while (temp_tags && !val_set)
    {
      BobguiTextTag *tag = BOBGUI_TEXT_TAG (temp_tags->data);
      int weight;

      g_object_get (tag,
                    "weight-set", &val_set,
                    "weight", &weight,
                    NULL);

      if (val_set)
        {
          g_hash_table_insert (attributes,
                               g_strdup ("weight"),
                               g_strdup_printf ("%d", weight));
        }
      temp_tags = temp_tags->next;
    }
  val_set = FALSE;

  /* scale is special as the effective value is the product
   * of all specified values
   */
  temp_tags = tags;
  while (temp_tags)
    {
      BobguiTextTag *tag = BOBGUI_TEXT_TAG (temp_tags->data);
      gboolean scale_set;

      g_object_get (tag, "scale-set", &scale_set, NULL);
      if (scale_set)
        {
          double font_scale;
          g_object_get (tag, "scale", &font_scale, NULL);
          val_set = TRUE;
          scale *= font_scale;
        }
      temp_tags = temp_tags->next;
    }
  if (val_set)
    {
      g_hash_table_insert (attributes,
                           g_strdup ("scale"),
                           g_strdup_printf ("%g", scale));
    }
  val_set = FALSE;

 temp_tags = tags;
  while (temp_tags && !val_set)
    {
      BobguiTextTag *tag = BOBGUI_TEXT_TAG (temp_tags->data);
      int size;

      g_object_get (tag,
                    "size-set", &val_set,
                    "size", &size,
                    NULL);
      if (val_set)
        {
          g_hash_table_insert (attributes,
                               g_strdup ("size"),
                               g_strdup_printf ("%i", size));
        }
      temp_tags = temp_tags->next;
    }
  val_set = FALSE;

  temp_tags = tags;
  while (temp_tags && !val_set)
    {
      BobguiTextTag *tag = BOBGUI_TEXT_TAG (temp_tags->data);
      gboolean strikethrough;

      g_object_get (tag,
                    "strikethrough-set", &val_set,
                    "strikethrough", &strikethrough,
                    NULL);
      if (val_set)
        g_hash_table_insert (attributes,
                             g_strdup ("strikethrough"),
                             strikethrough ? g_strdup ("true") : g_strdup ("false"));
      temp_tags = temp_tags->next;
    }
  val_set = FALSE;

  temp_tags = tags;
  while (temp_tags && !val_set)
    {
      BobguiTextTag *tag = BOBGUI_TEXT_TAG (temp_tags->data);
      PangoUnderline underline;

      g_object_get (tag,
                    "underline-set", &val_set,
                    "underline", &underline,
                    NULL);
      if (val_set)
        g_hash_table_insert (attributes,
                             g_strdup ("underline"),
                             g_strdup (pango_underline_to_string (underline)));
      temp_tags = temp_tags->next;
    }
  val_set = FALSE;

  temp_tags = tags;
  while (temp_tags && !val_set)
    {
      BobguiTextTag *tag = BOBGUI_TEXT_TAG (temp_tags->data);
      int rise;

      g_object_get (tag,
                    "rise-set", &val_set,
                    "rise", &rise,
                    NULL);
      if (val_set)
        {
          g_hash_table_insert (attributes,
                               g_strdup ("rise"),
                               g_strdup_printf ("%i", rise));
        }
      temp_tags = temp_tags->next;
    }
  val_set = FALSE;

  temp_tags = tags;
  while (temp_tags && !val_set)
    {
      BobguiTextTag *tag = BOBGUI_TEXT_TAG (temp_tags->data);
      gboolean bg_full_height;

      g_object_get (tag,
                    "background-full-height-set", &val_set,
                    "background-full-height", &bg_full_height,
                    NULL);
      if (val_set)
        g_hash_table_insert (attributes,
                             g_strdup ("bg-full-height"),
                             bg_full_height ? g_strdup ("true") : g_strdup ("false"));
      temp_tags = temp_tags->next;
    }
  val_set = FALSE;

  temp_tags = tags;
  while (temp_tags && !val_set)
    {
      BobguiTextTag *tag = BOBGUI_TEXT_TAG (temp_tags->data);
      int pixels;

      g_object_get (tag,
                    "pixels-inside-wrap-set", &val_set,
                    "pixels-inside-wrap", &pixels,
                    NULL);
      if (val_set)
        {
          g_hash_table_insert (attributes,
                               g_strdup ("pixels-inside-wrap"),
                               g_strdup_printf ("%i", pixels));
        }
      temp_tags = temp_tags->next;
    }
  val_set = FALSE;

  temp_tags = tags;
  while (temp_tags && !val_set)
    {
      BobguiTextTag *tag = BOBGUI_TEXT_TAG (temp_tags->data);
      int pixels;

      g_object_get (tag,
                    "pixels-below-lines-set", &val_set,
                    "pixels-below-lines", &pixels,
                    NULL);
      if (val_set)
        {
          g_hash_table_insert (attributes,
                               g_strdup ("pixels-below-lines"),
                               g_strdup_printf ("%i", pixels));
        }
      temp_tags = temp_tags->next;
    }
  val_set = FALSE;

  temp_tags = tags;
  while (temp_tags && !val_set)
    {
      BobguiTextTag *tag = BOBGUI_TEXT_TAG (temp_tags->data);
      int pixels;

      g_object_get (tag,
                    "pixels-above-lines-set", &val_set,
                    "pixels-above-lines", &pixels,
                    NULL);
      if (val_set)
        {
          g_hash_table_insert (attributes,
                               g_strdup ("pixels-above-lines"),
                               g_strdup_printf ("%i", pixels));
        }
      temp_tags = temp_tags->next;
    }
  val_set = FALSE;

  temp_tags = tags;
  while (temp_tags && !val_set)
    {
      BobguiTextTag *tag = BOBGUI_TEXT_TAG (temp_tags->data);
      gboolean editable;

      g_object_get (tag,
                    "editable-set", &val_set,
                    "editable", &editable,
                    NULL);
      if (val_set)
        g_hash_table_insert (attributes,
                             g_strdup ("editable"),
                             editable ? g_strdup ("true") : g_strdup ("false"));
      temp_tags = temp_tags->next;
    }
  val_set = FALSE;

  temp_tags = tags;
  while (temp_tags && !val_set)
    {
      BobguiTextTag *tag = BOBGUI_TEXT_TAG (temp_tags->data);
      gboolean invisible;

      g_object_get (tag,
                    "invisible-set", &val_set,
                    "invisible", &invisible,
                    NULL);
      if (val_set)
        g_hash_table_insert (attributes,
                             g_strdup ("invisible"),
                             invisible ? g_strdup ("true") : g_strdup ("false"));
      temp_tags = temp_tags->next;
    }
  val_set = FALSE;

  temp_tags = tags;
  while (temp_tags && !val_set)
    {
      BobguiTextTag *tag = BOBGUI_TEXT_TAG (temp_tags->data);
      int indent;

      g_object_get (tag,
                    "indent-set", &val_set,
                    "indent", &indent,
                    NULL);
      if (val_set)
        {
          g_hash_table_insert (attributes,
                               g_strdup ("indent"),
                               g_strdup_printf ("%i", indent));
        }
      temp_tags = temp_tags->next;
    }
  val_set = FALSE;

  temp_tags = tags;
  while (temp_tags && !val_set)
    {
      BobguiTextTag *tag = BOBGUI_TEXT_TAG (temp_tags->data);
      int margin;

      g_object_get (tag,
                    "right-margin-set", &val_set,
                    "right-margin", &margin,
                    NULL);
      if (val_set)
        {
          g_hash_table_insert (attributes,
                               g_strdup ("right-margin"),
                               g_strdup_printf ("%i", margin));
        }
      temp_tags = temp_tags->next;
    }
  val_set = FALSE;

  temp_tags = tags;
  while (temp_tags && !val_set)
    {
      BobguiTextTag *tag = BOBGUI_TEXT_TAG (temp_tags->data);
      int margin;

      g_object_get (tag,
                    "left-margin-set", &val_set,
                    "left-margin", &margin,
                    NULL);
      if (val_set)
        {
          g_hash_table_insert (attributes,
                               g_strdup ("left-margin"),
                               g_strdup_printf ("%i", margin));
        }
      temp_tags = temp_tags->next;
    }
  val_set = FALSE;

  g_slist_free (tags);
}

static void
clear_commit_func (gpointer data)
{
  CommitFunc *func = data;

  if (func->user_data_destroy)
    func->user_data_destroy (func->user_data);
}

/**
 * bobgui_text_buffer_add_commit_notify:
 * @buffer: a [type@Bobgui.TextBuffer]
 * @flags: which notifications should be dispatched to @callback
 * @commit_notify: (scope async) (closure user_data) (destroy destroy): a
 *   [callback@Bobgui.TextBufferCommitNotify] to call for commit notifications
 * @user_data: closure data for @commit_notify
 * @destroy: a callback to free @user_data when @commit_notify is removed
 *
 * Adds a [callback@Bobgui.TextBufferCommitNotify] to be called when a change
 * is to be made to the [type@Bobgui.TextBuffer].
 *
 * Functions are explicitly forbidden from making changes to the
 * [type@Bobgui.TextBuffer] from this callback. It is intended for tracking
 * changes to the buffer only.
 *
 * It may be advantageous to use [callback@Bobgui.TextBufferCommitNotify] over
 * connecting to the [signal@Bobgui.TextBuffer::insert-text] or
 * [signal@Bobgui.TextBuffer::delete-range] signals to avoid ordering issues with
 * other signal handlers which may further modify the [type@Bobgui.TextBuffer].
 *
 * Returns: a handler id which may be used to remove the commit notify
 *   callback using [method@Bobgui.TextBuffer.remove_commit_notify].
 *
 * Since: 4.16
 */
guint
bobgui_text_buffer_add_commit_notify (BobguiTextBuffer             *buffer,
                                   BobguiTextBufferNotifyFlags   flags,
                                   BobguiTextBufferCommitNotify  commit_notify,
                                   gpointer                   user_data,
                                   GDestroyNotify             destroy)
{
  CommitFunc func;

  g_return_val_if_fail (BOBGUI_IS_TEXT_BUFFER (buffer), 0);
  g_return_val_if_fail (buffer->priv->in_commit_notify == FALSE, 0);

  func.callback = commit_notify;
  func.user_data = user_data;
  func.user_data_destroy = destroy;
  func.handler_id = ++buffer->priv->last_commit_handler;
  func.flags = flags;

  if (buffer->priv->commit_funcs == NULL)
    {
      buffer->priv->commit_funcs = g_array_new (FALSE, FALSE, sizeof (CommitFunc));
      g_array_set_clear_func (buffer->priv->commit_funcs, clear_commit_func);
    }

  g_array_append_val (buffer->priv->commit_funcs, func);

  return func.handler_id;
}

/**
 * bobgui_text_buffer_remove_commit_notify:
 * @buffer: a `BobguiTextBuffer`
 * @commit_notify_handler: the notify handler identifier returned from
 *   [method@Bobgui.TextBuffer.add_commit_notify].
 *
 * Removes the `BobguiTextBufferCommitNotify` handler previously registered
 * with [method@Bobgui.TextBuffer.add_commit_notify].
 *
 * This may result in the `user_data_destroy` being called that was passed when registering
 * the commit notify functions.
 *
 * Since: 4.16
 */
void
bobgui_text_buffer_remove_commit_notify (BobguiTextBuffer *buffer,
                                      guint          commit_notify_handler)
{
  g_return_if_fail (BOBGUI_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (commit_notify_handler > 0);
  g_return_if_fail (buffer->priv->in_commit_notify == FALSE);

  if (buffer->priv->commit_funcs != NULL)
    {
      for (guint i = 0; i < buffer->priv->commit_funcs->len; i++)
        {
          const CommitFunc *func = &g_array_index (buffer->priv->commit_funcs, CommitFunc, i);

          if (func->handler_id == commit_notify_handler)
            {
              g_array_remove_index (buffer->priv->commit_funcs, i);

              if (buffer->priv->commit_funcs->len == 0)
                g_clear_pointer (&buffer->priv->commit_funcs, g_array_unref);

              return;
            }
        }
    }

  g_warning ("No such BobguiTextBufferCommitNotify matching %u",
             commit_notify_handler);
}

static void
bobgui_text_buffer_commit_notify (BobguiTextBuffer            *buffer,
                               BobguiTextBufferNotifyFlags  flags,
                               guint                     position,
                               guint                     length)
{
  buffer->priv->in_commit_notify = TRUE;

  for (guint i = 0; i < buffer->priv->commit_funcs->len; i++)
    {
      const CommitFunc *func = &g_array_index (buffer->priv->commit_funcs, CommitFunc, i);

      if (func->flags & flags)
        func->callback (buffer, flags, position, length, func->user_data);
    }

  buffer->priv->in_commit_notify = FALSE;
}
