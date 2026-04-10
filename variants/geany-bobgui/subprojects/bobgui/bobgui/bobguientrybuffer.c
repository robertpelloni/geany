/* bobguientrybuffer.c
 * Copyright (C) 2009  Stefan Walter <stef@memberwebs.com>
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

#include "config.h"

#include "bobguientrybuffer.h"
#include "bobguimarshalers.h"
#include "bobguiprivate.h"
#include "bobguiwidget.h"

#include <gdk/gdk.h>

#include <string.h>

/**
 * BobguiEntryBuffer:
 *
 * Holds the text that is displayed in a single-line text entry widget.
 *
 * A single `BobguiEntryBuffer` object can be shared by multiple widgets
 * which will then share the same text content, but not the cursor
 * position, visibility attributes, icon etc.
 *
 * `BobguiEntryBuffer` may be derived from. Such a derived class might allow
 * text to be stored in an alternate location, such as non-pageable memory,
 * useful in the case of important passwords. Or a derived class could
 * integrate with an application’s concept of undo/redo.
 */

/* Initial size of buffer, in bytes */
#define MIN_SIZE 16

enum {
  PROP_0,
  PROP_TEXT,
  PROP_LENGTH,
  PROP_MAX_LENGTH,
  NUM_PROPERTIES
};

static GParamSpec *entry_buffer_props[NUM_PROPERTIES] = { NULL, };

enum {
  INSERTED_TEXT,
  DELETED_TEXT,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

typedef struct _BobguiEntryBufferPrivate BobguiEntryBufferPrivate;
struct _BobguiEntryBufferPrivate
{
  /* Only valid if this class is not derived */
  char *normal_text;
  gsize  normal_text_size;
  gsize  normal_text_bytes;
  guint  normal_text_chars;

  int    max_length;
};

G_DEFINE_TYPE_WITH_PRIVATE (BobguiEntryBuffer, bobgui_entry_buffer, G_TYPE_OBJECT)

/* --------------------------------------------------------------------------------
 * DEFAULT IMPLEMENTATIONS OF TEXT BUFFER
 *
 * These may be overridden by a derived class, behavior may be changed etc...
 * The normal_text and normal_text_xxxx fields may not be valid when
 * this class is derived from.
 */

/* Overwrite a memory that might contain sensitive information. */
static void
trash_area (char *area,
            gsize  len)
{
  volatile char *varea = (volatile char *)area;
  while (len-- > 0)
    *varea++ = 0;
}

static const char *
bobgui_entry_buffer_normal_get_text (BobguiEntryBuffer *buffer,
                                  gsize          *n_bytes)
{
  BobguiEntryBufferPrivate *priv = bobgui_entry_buffer_get_instance_private (buffer);

  if (n_bytes)
    *n_bytes = priv->normal_text_bytes;

  if (!priv->normal_text)
      return "";

  return priv->normal_text;
}

static guint
bobgui_entry_buffer_normal_get_length (BobguiEntryBuffer *buffer)
{
  BobguiEntryBufferPrivate *priv = bobgui_entry_buffer_get_instance_private (buffer);

  return priv->normal_text_chars;
}

static guint
bobgui_entry_buffer_normal_insert_text (BobguiEntryBuffer *buffer,
                                     guint           position,
                                     const char     *chars,
                                     guint           n_chars)
{
  BobguiEntryBufferPrivate *pv = bobgui_entry_buffer_get_instance_private (buffer);
  gsize prev_size;
  gsize n_bytes;
  gsize at;

  n_bytes = g_utf8_offset_to_pointer (chars, n_chars) - chars;

  /* Need more memory */
  if (n_bytes + pv->normal_text_bytes + 1 > pv->normal_text_size)
    {
      char *et_new;

      prev_size = pv->normal_text_size;

      /* Calculate our new buffer size */
      while (n_bytes + pv->normal_text_bytes + 1 > pv->normal_text_size)
        {
          if (pv->normal_text_size == 0)
            pv->normal_text_size = MIN_SIZE;
          else
            {
              if (2 * pv->normal_text_size < BOBGUI_ENTRY_BUFFER_MAX_SIZE)
                pv->normal_text_size *= 2;
              else
                {
                  pv->normal_text_size = BOBGUI_ENTRY_BUFFER_MAX_SIZE;
                  if (n_bytes > pv->normal_text_size - pv->normal_text_bytes - 1)
                    {
                      n_bytes = pv->normal_text_size - pv->normal_text_bytes - 1;
                      n_bytes = g_utf8_find_prev_char (chars, chars + n_bytes + 1) - chars;
                      n_chars = g_utf8_strlen (chars, n_bytes);
                    }
                  break;
                }
            }
        }

      /* Could be a password, so can't leave stuff in memory. */
      et_new = g_malloc (pv->normal_text_size);
      if (pv->normal_text)
        {
          memcpy (et_new, pv->normal_text, MIN (prev_size, pv->normal_text_size));
          trash_area (pv->normal_text, prev_size);
          g_free (pv->normal_text);
        }
      pv->normal_text = et_new;
    }

  /* Actual text insertion */
  at = g_utf8_offset_to_pointer (pv->normal_text, position) - pv->normal_text;
  memmove (pv->normal_text + at + n_bytes, pv->normal_text + at, pv->normal_text_bytes - at);
  memcpy (pv->normal_text + at, chars, n_bytes);

  /* Book keeping */
  pv->normal_text_bytes += n_bytes;
  pv->normal_text_chars += n_chars;
  pv->normal_text[pv->normal_text_bytes] = '\0';

  bobgui_entry_buffer_emit_inserted_text (buffer, position, chars, n_chars);
  return n_chars;
}

static guint
bobgui_entry_buffer_normal_delete_text (BobguiEntryBuffer *buffer,
                                     guint           position,
                                     guint           n_chars)
{
  BobguiEntryBufferPrivate *pv = bobgui_entry_buffer_get_instance_private (buffer);

  if (position > pv->normal_text_chars)
    position = pv->normal_text_chars;
  if (position + n_chars > pv->normal_text_chars)
    n_chars = pv->normal_text_chars - position;

  if (n_chars > 0)
    bobgui_entry_buffer_emit_deleted_text (buffer, position, n_chars);

  return n_chars;
}

/* --------------------------------------------------------------------------------
 *
 */

static void
bobgui_entry_buffer_real_inserted_text (BobguiEntryBuffer *buffer,
                                     guint           position,
                                     const char     *chars,
                                     guint           n_chars)
{
  g_object_notify_by_pspec (G_OBJECT (buffer), entry_buffer_props[PROP_TEXT]);
  g_object_notify_by_pspec (G_OBJECT (buffer), entry_buffer_props[PROP_LENGTH]);
}

static void
bobgui_entry_buffer_real_deleted_text (BobguiEntryBuffer *buffer,
                                    guint           position,
                                    guint           n_chars)
{
  BobguiEntryBufferPrivate *pv = bobgui_entry_buffer_get_instance_private (buffer);
  gsize start, end;

  start = g_utf8_offset_to_pointer (pv->normal_text, position) - pv->normal_text;
  end = g_utf8_offset_to_pointer (pv->normal_text, position + n_chars) - pv->normal_text;

  memmove (pv->normal_text + start, pv->normal_text + end, pv->normal_text_bytes + 1 - end);
  pv->normal_text_chars -= n_chars;
  pv->normal_text_bytes -= (end - start);

  /*
   * Could be a password, make sure we don't leave anything sensitive after
   * the terminating zero.  Note, that the terminating zero already trashed
   * one byte.
   */
  trash_area (pv->normal_text + pv->normal_text_bytes + 1, end - start - 1);

  g_object_notify_by_pspec (G_OBJECT (buffer), entry_buffer_props[PROP_TEXT]);
  g_object_notify_by_pspec (G_OBJECT (buffer), entry_buffer_props[PROP_LENGTH]);
}

/* --------------------------------------------------------------------------------
 *
 */

static void
bobgui_entry_buffer_init (BobguiEntryBuffer *buffer)
{
  BobguiEntryBufferPrivate *pv = bobgui_entry_buffer_get_instance_private (buffer);

  pv->normal_text = NULL;
  pv->normal_text_chars = 0;
  pv->normal_text_bytes = 0;
  pv->normal_text_size = 0;
}

static void
bobgui_entry_buffer_finalize (GObject *obj)
{
  BobguiEntryBuffer *buffer = BOBGUI_ENTRY_BUFFER (obj);
  BobguiEntryBufferPrivate *pv = bobgui_entry_buffer_get_instance_private (buffer);

  if (pv->normal_text)
    {
      trash_area (pv->normal_text, pv->normal_text_size);
      g_free (pv->normal_text);
      pv->normal_text = NULL;
      pv->normal_text_bytes = pv->normal_text_size = 0;
      pv->normal_text_chars = 0;
    }

  G_OBJECT_CLASS (bobgui_entry_buffer_parent_class)->finalize (obj);
}

static void
bobgui_entry_buffer_set_property (GObject      *obj,
                               guint         prop_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  BobguiEntryBuffer *buffer = BOBGUI_ENTRY_BUFFER (obj);

  switch (prop_id)
    {
    case PROP_TEXT:
      bobgui_entry_buffer_set_text (buffer, g_value_get_string (value), -1);
      break;
    case PROP_MAX_LENGTH:
      bobgui_entry_buffer_set_max_length (buffer, g_value_get_int (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, prop_id, pspec);
      break;
    }
}

static void
bobgui_entry_buffer_get_property (GObject    *obj,
                               guint       prop_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  BobguiEntryBuffer *buffer = BOBGUI_ENTRY_BUFFER (obj);

  switch (prop_id)
    {
    case PROP_TEXT:
      g_value_set_string (value, bobgui_entry_buffer_get_text (buffer));
      break;
    case PROP_LENGTH:
      g_value_set_uint (value, bobgui_entry_buffer_get_length (buffer));
      break;
    case PROP_MAX_LENGTH:
      g_value_set_int (value, bobgui_entry_buffer_get_max_length (buffer));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, prop_id, pspec);
      break;
    }
}

static void
bobgui_entry_buffer_class_init (BobguiEntryBufferClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->finalize = bobgui_entry_buffer_finalize;
  gobject_class->set_property = bobgui_entry_buffer_set_property;
  gobject_class->get_property = bobgui_entry_buffer_get_property;

  klass->get_text = bobgui_entry_buffer_normal_get_text;
  klass->get_length = bobgui_entry_buffer_normal_get_length;
  klass->insert_text = bobgui_entry_buffer_normal_insert_text;
  klass->delete_text = bobgui_entry_buffer_normal_delete_text;

  klass->inserted_text = bobgui_entry_buffer_real_inserted_text;
  klass->deleted_text = bobgui_entry_buffer_real_deleted_text;

  /**
   * BobguiEntryBuffer:text:
   *
   * The contents of the buffer.
   */
  entry_buffer_props[PROP_TEXT] =
      g_param_spec_string ("text", NULL, NULL,
                           "",
                           BOBGUI_PARAM_READWRITE);

  /**
   * BobguiEntryBuffer:length:
   *
   * The length (in characters) of the text in buffer.
   */
   entry_buffer_props[PROP_LENGTH] =
       g_param_spec_uint ("length", NULL, NULL,
                          0, BOBGUI_ENTRY_BUFFER_MAX_SIZE, 0,
                          BOBGUI_PARAM_READABLE);

  /**
   * BobguiEntryBuffer:max-length:
   *
   * The maximum length (in characters) of the text in the buffer.
   */
  entry_buffer_props[PROP_MAX_LENGTH] =
      g_param_spec_int ("max-length", NULL, NULL,
                        0, BOBGUI_ENTRY_BUFFER_MAX_SIZE, 0,
                        BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (gobject_class, NUM_PROPERTIES, entry_buffer_props);

  /**
   * BobguiEntryBuffer::inserted-text:
   * @buffer: a `BobguiEntryBuffer`
   * @position: the position the text was inserted at.
   * @chars: The text that was inserted.
   * @n_chars: The number of characters that were inserted.
   *
   * This signal is emitted after text is inserted into the buffer.
   */
  signals[INSERTED_TEXT] = g_signal_new (I_("inserted-text"),
                                         BOBGUI_TYPE_ENTRY_BUFFER,
                                         G_SIGNAL_RUN_FIRST,
                                         G_STRUCT_OFFSET (BobguiEntryBufferClass, inserted_text),
                                         NULL, NULL,
                                         _bobgui_marshal_VOID__UINT_STRING_UINT,
                                         G_TYPE_NONE, 3,
                                         G_TYPE_UINT,
                                         G_TYPE_STRING,
                                         G_TYPE_UINT);
  g_signal_set_va_marshaller (signals[INSERTED_TEXT],
                              BOBGUI_TYPE_ENTRY_BUFFER,
                              _bobgui_marshal_VOID__UINT_STRING_UINTv);

  /**
   * BobguiEntryBuffer::deleted-text:
   * @buffer: a `BobguiEntryBuffer`
   * @position: the position the text was deleted at.
   * @n_chars: The number of characters that were deleted.
   *
   * The text is altered in the default handler for this signal.
   *
   * If you want access to the text after the text has been modified,
   * use %G_CONNECT_AFTER.
   */
  signals[DELETED_TEXT] =  g_signal_new (I_("deleted-text"),
                                         BOBGUI_TYPE_ENTRY_BUFFER,
                                         G_SIGNAL_RUN_LAST,
                                         G_STRUCT_OFFSET (BobguiEntryBufferClass, deleted_text),
                                         NULL, NULL,
                                         _bobgui_marshal_VOID__UINT_UINT,
                                         G_TYPE_NONE, 2,
                                         G_TYPE_UINT,
                                         G_TYPE_UINT);
  g_signal_set_va_marshaller (signals[DELETED_TEXT],
                              BOBGUI_TYPE_ENTRY_BUFFER,
                              _bobgui_marshal_VOID__UINT_UINTv);
}

/* --------------------------------------------------------------------------------
 *
 */

/**
 * bobgui_entry_buffer_new:
 * @initial_chars: (nullable): initial buffer text
 * @n_initial_chars: number of characters in @initial_chars, or -1
 *
 * Create a new `BobguiEntryBuffer` object.
 *
 * Optionally, specify initial text to set in the buffer.
 *
 * Returns: A new `BobguiEntryBuffer` object.
 */
BobguiEntryBuffer*
bobgui_entry_buffer_new (const char *initial_chars,
                      int          n_initial_chars)
{
  BobguiEntryBuffer *buffer = g_object_new (BOBGUI_TYPE_ENTRY_BUFFER, NULL);
  if (initial_chars)
    bobgui_entry_buffer_set_text (buffer, initial_chars, n_initial_chars);
  return buffer;
}

/**
 * bobgui_entry_buffer_get_length:
 * @buffer: a `BobguiEntryBuffer`
 *
 * Retrieves the length in characters of the buffer.
 *
 * Returns: The number of characters in the buffer.
 **/
guint
bobgui_entry_buffer_get_length (BobguiEntryBuffer *buffer)
{
  BobguiEntryBufferClass *klass;

  g_return_val_if_fail (BOBGUI_IS_ENTRY_BUFFER (buffer), 0);

  klass = BOBGUI_ENTRY_BUFFER_GET_CLASS (buffer);
  g_return_val_if_fail (klass->get_length != NULL, 0);

  return (*klass->get_length) (buffer);
}

/**
 * bobgui_entry_buffer_get_bytes:
 * @buffer: a `BobguiEntryBuffer`
 *
 * Retrieves the length in bytes of the buffer.
 *
 * See [method@Bobgui.EntryBuffer.get_length].
 *
 * Returns: The byte length of the buffer.
 **/
gsize
bobgui_entry_buffer_get_bytes (BobguiEntryBuffer *buffer)
{
  BobguiEntryBufferClass *klass;
  gsize bytes = 0;

  g_return_val_if_fail (BOBGUI_IS_ENTRY_BUFFER (buffer), 0);

  klass = BOBGUI_ENTRY_BUFFER_GET_CLASS (buffer);
  g_return_val_if_fail (klass->get_text != NULL, 0);

  (*klass->get_text) (buffer, &bytes);
  return bytes;
}

/**
 * bobgui_entry_buffer_get_text:
 * @buffer: a `BobguiEntryBuffer`
 *
 * Retrieves the contents of the buffer.
 *
 * The memory pointer returned by this call will not change
 * unless this object emits a signal, or is finalized.
 *
 * Returns: a pointer to the contents of the widget as a
 *   string. This string points to internally allocated storage
 *   in the buffer and must not be freed, modified or stored.
 */
const char *
bobgui_entry_buffer_get_text (BobguiEntryBuffer *buffer)
{
  BobguiEntryBufferClass *klass;

  g_return_val_if_fail (BOBGUI_IS_ENTRY_BUFFER (buffer), NULL);

  klass = BOBGUI_ENTRY_BUFFER_GET_CLASS (buffer);
  g_return_val_if_fail (klass->get_text != NULL, NULL);

  return (*klass->get_text) (buffer, NULL);
}

/**
 * bobgui_entry_buffer_set_text:
 * @buffer: a `BobguiEntryBuffer`
 * @chars: the new text
 * @n_chars: the number of characters in @text, or -1
 *
 * Sets the text in the buffer.
 *
 * This is roughly equivalent to calling
 * [method@Bobgui.EntryBuffer.delete_text] and
 * [method@Bobgui.EntryBuffer.insert_text].
 *
 * Note that @n_chars is in characters, not in bytes.
 **/
void
bobgui_entry_buffer_set_text (BobguiEntryBuffer *buffer,
                           const char     *chars,
                           int             n_chars)
{
  g_return_if_fail (BOBGUI_IS_ENTRY_BUFFER (buffer));
  g_return_if_fail (chars != NULL);

  g_object_freeze_notify (G_OBJECT (buffer));
  bobgui_entry_buffer_delete_text (buffer, 0, -1);
  bobgui_entry_buffer_insert_text (buffer, 0, chars, n_chars);
  g_object_thaw_notify (G_OBJECT (buffer));
}

/**
 * bobgui_entry_buffer_set_max_length:
 * @buffer: a `BobguiEntryBuffer`
 * @max_length: the maximum length of the entry buffer, or 0 for no maximum.
 *   (other than the maximum length of entries.) The value passed in will
 *   be clamped to the range 0-65536.
 *
 * Sets the maximum allowed length of the contents of the buffer.
 *
 * If the current contents are longer than the given length, then
 * they will be truncated to fit.
 */
void
bobgui_entry_buffer_set_max_length (BobguiEntryBuffer *buffer,
                                 int             max_length)
{
  BobguiEntryBufferPrivate *priv = bobgui_entry_buffer_get_instance_private (buffer);

  g_return_if_fail (BOBGUI_IS_ENTRY_BUFFER (buffer));

  max_length = CLAMP (max_length, 0, BOBGUI_ENTRY_BUFFER_MAX_SIZE);

  if (priv->max_length == max_length)
    return;

  if (max_length > 0 && bobgui_entry_buffer_get_length (buffer) > max_length)
    bobgui_entry_buffer_delete_text (buffer, max_length, -1);

  priv->max_length = max_length;
  g_object_notify_by_pspec (G_OBJECT (buffer), entry_buffer_props[PROP_MAX_LENGTH]);
}

/**
 * bobgui_entry_buffer_get_max_length:
 * @buffer: a `BobguiEntryBuffer`
 *
 * Retrieves the maximum allowed length of the text in @buffer.
 *
 * Returns: the maximum allowed number of characters
 *   in `BobguiEntryBuffer`, or 0 if there is no maximum.
 */
int
bobgui_entry_buffer_get_max_length (BobguiEntryBuffer *buffer)
{
  BobguiEntryBufferPrivate *priv = bobgui_entry_buffer_get_instance_private (buffer);

  g_return_val_if_fail (BOBGUI_IS_ENTRY_BUFFER (buffer), 0);

  return priv->max_length;
}

/**
 * bobgui_entry_buffer_insert_text:
 * @buffer: a `BobguiEntryBuffer`
 * @position: the position at which to insert text.
 * @chars: the text to insert into the buffer.
 * @n_chars: the length of the text in characters, or -1
 *
 * Inserts @n_chars characters of @chars into the contents of the
 * buffer, at position @position.
 *
 * If @n_chars is negative, then characters from chars will be inserted
 * until a null-terminator is found. If @position or @n_chars are out of
 * bounds, or the maximum buffer text length is exceeded, then they are
 * coerced to sane values.
 *
 * Note that the position and length are in characters, not in bytes.
 *
 * Returns: The number of characters actually inserted.
 */
guint
bobgui_entry_buffer_insert_text (BobguiEntryBuffer *buffer,
                              guint           position,
                              const char     *chars,
                              int             n_chars)
{
  BobguiEntryBufferPrivate *pv = bobgui_entry_buffer_get_instance_private (buffer);
  BobguiEntryBufferClass *klass;
  guint length;

  g_return_val_if_fail (BOBGUI_IS_ENTRY_BUFFER (buffer), 0);

  length = bobgui_entry_buffer_get_length (buffer);

  if (n_chars < 0)
    n_chars = g_utf8_strlen (chars, -1);

  /* Bring position into bounds */
  if (position > length)
    position = length;

  /* Make sure not entering too much data */
  if (pv->max_length > 0)
    {
      if (length >= pv->max_length)
        n_chars = 0;
      else if (length + n_chars > pv->max_length)
        n_chars -= (length + n_chars) - pv->max_length;
    }

  if (n_chars == 0)
    return 0;

  klass = BOBGUI_ENTRY_BUFFER_GET_CLASS (buffer);
  g_return_val_if_fail (klass->insert_text != NULL, 0);

  return (*klass->insert_text) (buffer, position, chars, n_chars);
}

/**
 * bobgui_entry_buffer_delete_text:
 * @buffer: a `BobguiEntryBuffer`
 * @position: position at which to delete text
 * @n_chars: number of characters to delete
 *
 * Deletes a sequence of characters from the buffer.
 *
 * @n_chars characters are deleted starting at @position.
 * If @n_chars is negative, then all characters until the
 * end of the text are deleted.
 *
 * If @position or @n_chars are out of bounds, then they
 * are coerced to sane values.
 *
 * Note that the positions are specified in characters,
 * not bytes.
 *
 * Returns: The number of characters deleted.
 */
guint
bobgui_entry_buffer_delete_text (BobguiEntryBuffer *buffer,
                              guint           position,
                              int             n_chars)
{
  BobguiEntryBufferClass *klass;
  guint length;

  g_return_val_if_fail (BOBGUI_IS_ENTRY_BUFFER (buffer), 0);

  length = bobgui_entry_buffer_get_length (buffer);
  if (n_chars < 0)
    n_chars = length;
  if (position > length)
    position = length;
  if (position + n_chars > length)
    n_chars = length - position;

  klass = BOBGUI_ENTRY_BUFFER_GET_CLASS (buffer);
  g_return_val_if_fail (klass->delete_text != NULL, 0);

  return (*klass->delete_text) (buffer, position, n_chars);
}

/**
 * bobgui_entry_buffer_emit_inserted_text:
 * @buffer: a `BobguiEntryBuffer`
 * @position: position at which text was inserted
 * @chars: text that was inserted
 * @n_chars: number of characters inserted
 *
 * Used when subclassing `BobguiEntryBuffer`.
 */
void
bobgui_entry_buffer_emit_inserted_text (BobguiEntryBuffer *buffer,
                                     guint           position,
                                     const char     *chars,
                                     guint           n_chars)
{
  g_return_if_fail (BOBGUI_IS_ENTRY_BUFFER (buffer));
  g_signal_emit (buffer, signals[INSERTED_TEXT], 0, position, chars, n_chars);
}

/**
 * bobgui_entry_buffer_emit_deleted_text:
 * @buffer: a `BobguiEntryBuffer`
 * @position: position at which text was deleted
 * @n_chars: number of characters deleted
 *
 * Used when subclassing `BobguiEntryBuffer`.
 */
void
bobgui_entry_buffer_emit_deleted_text (BobguiEntryBuffer *buffer,
                                    guint           position,
                                    guint           n_chars)
{
  g_return_if_fail (BOBGUI_IS_ENTRY_BUFFER (buffer));
  g_signal_emit (buffer, signals[DELETED_TEXT], 0, position, n_chars);
}
