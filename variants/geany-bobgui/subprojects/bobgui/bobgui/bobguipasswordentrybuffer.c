/* bobguipasswordentrybuffer.c: Entry buffer with secure allocation 
 *
   Copyright 2009  Stefan Walter
 * Copyright 2020  GNOME Foundation
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "bobguipasswordentrybuffer.h"

#include "bobguisecurememoryprivate.h"

#include <string.h>

/* Initial size of buffer, in bytes */
#define MIN_SIZE 16

/**
 * BobguiPasswordEntryBuffer:
 *
 * A `BobguiEntryBuffer` that locks the underlying memory to prevent it
 * from being swapped to disk.
 *
 * `BobguiPasswordEntry` uses a `BobguiPasswordEntryBuffer`.
 *
 * Since: 4.4
 */
struct _BobguiPasswordEntryBuffer
{
  BobguiEntryBuffer parent_instance;

  char *text;
  gsize text_size;
  gsize text_bytes;
  guint text_chars;
};

G_DEFINE_TYPE (BobguiPasswordEntryBuffer, bobgui_password_entry_buffer, BOBGUI_TYPE_ENTRY_BUFFER)

static const char *
bobgui_password_entry_buffer_real_get_text (BobguiEntryBuffer *buffer,
                                       gsize *n_bytes)
{
  BobguiPasswordEntryBuffer *self = BOBGUI_PASSWORD_ENTRY_BUFFER (buffer);

  if (n_bytes != NULL)
    *n_bytes = self->text_bytes;

  if (!self->text)
    return "";

  return self->text;
}

static guint
bobgui_password_entry_buffer_real_get_length (BobguiEntryBuffer *buffer)
{
  BobguiPasswordEntryBuffer *self = BOBGUI_PASSWORD_ENTRY_BUFFER (buffer);
  return self->text_chars;
}

static guint
bobgui_password_entry_buffer_real_insert_text (BobguiEntryBuffer *buffer,
                                            guint           position,
                                            const char     *chars,
                                            guint           n_chars)
{
  BobguiPasswordEntryBuffer *self = BOBGUI_PASSWORD_ENTRY_BUFFER (buffer);

  gsize n_bytes = g_utf8_offset_to_pointer (chars, n_chars) - chars;

  /* Need more memory */
  if (n_bytes + self->text_bytes + 1 > self->text_size)
    {
      /* Calculate our new buffer size */
      while (n_bytes + self->text_bytes + 1 > self->text_size)
        {
          if (self->text_size == 0)
            {
              self->text_size = MIN_SIZE;
            }
          else
            {
              if (2 * self->text_size < BOBGUI_ENTRY_BUFFER_MAX_SIZE)
                {
                  self->text_size *= 2;
                }
              else
                {
                  self->text_size = BOBGUI_ENTRY_BUFFER_MAX_SIZE;
                  if (n_bytes > self->text_size - self->text_bytes - 1)
                    {
                      n_bytes = self->text_size - self->text_bytes - 1;
                      n_bytes = g_utf8_find_prev_char (chars, chars + n_bytes + 1) - chars;
                      n_chars = g_utf8_strlen (chars, n_bytes);
                    }
                  break;
                }
            }
        }

      self->text = bobgui_secure_realloc (self->text, self->text_size);
    }

  /* Actual text insertion */
  gsize at = g_utf8_offset_to_pointer (self->text, position) - self->text;
  memmove (self->text + at + n_bytes, self->text + at, self->text_bytes - at);
  memcpy (self->text + at, chars, n_bytes);

  /* Book keeping */
  self->text_bytes += n_bytes;
  self->text_chars += n_chars;
  self->text[self->text_bytes] = '\0';

  bobgui_entry_buffer_emit_inserted_text (buffer, position, chars, n_chars);

  return n_chars;
}

static void
bobgui_password_entry_buffer_real_deleted_text (BobguiEntryBuffer *buffer,
                                             guint           position,
                                             guint           n_chars)
{
  BobguiPasswordEntryBuffer *self = BOBGUI_PASSWORD_ENTRY_BUFFER (buffer);

  gsize start = g_utf8_offset_to_pointer (self->text, position) - self->text;
  gsize end = g_utf8_offset_to_pointer (self->text, position + n_chars) - self->text;

  memmove (self->text + start, self->text + end, self->text_bytes + 1 - end);
  self->text_chars -= n_chars;
  self->text_bytes -= (end - start);

  g_object_notify (G_OBJECT (buffer), "text");
  g_object_notify (G_OBJECT (buffer), "length");
}

static guint
bobgui_password_entry_buffer_real_delete_text (BobguiEntryBuffer *buffer,
                                            guint           position,
                                            guint           n_chars)
{
  BobguiPasswordEntryBuffer *self = BOBGUI_PASSWORD_ENTRY_BUFFER (buffer);

  if (position > self->text_chars)
    position = self->text_chars;
  if (position + n_chars > self->text_chars)
    n_chars = self->text_chars - position;

  if (n_chars > 0)
    bobgui_entry_buffer_emit_deleted_text (buffer, position, n_chars);

  return n_chars;
}

static void
bobgui_password_entry_buffer_finalize (GObject *gobject)
{
  BobguiPasswordEntryBuffer *self = BOBGUI_PASSWORD_ENTRY_BUFFER (gobject);

  g_clear_pointer (&self->text, bobgui_secure_free);

  self->text_bytes = 0;
  self->text_size = 0;
  self->text_chars = 0;

  G_OBJECT_CLASS (bobgui_password_entry_buffer_parent_class)->finalize (gobject);
}

static void
bobgui_password_entry_buffer_class_init (BobguiPasswordEntryBufferClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  BobguiEntryBufferClass *buffer_class = BOBGUI_ENTRY_BUFFER_CLASS (klass);

  gobject_class->finalize = bobgui_password_entry_buffer_finalize;

  buffer_class->get_text = bobgui_password_entry_buffer_real_get_text;
  buffer_class->get_length = bobgui_password_entry_buffer_real_get_length;
  buffer_class->insert_text = bobgui_password_entry_buffer_real_insert_text;
  buffer_class->delete_text = bobgui_password_entry_buffer_real_delete_text;
  buffer_class->deleted_text = bobgui_password_entry_buffer_real_deleted_text;
}

static void
bobgui_password_entry_buffer_init (BobguiPasswordEntryBuffer *self)
{
}

/**
 * bobgui_password_entry_buffer_new:
 *
 * Creates a new `BobguiEntryBuffer` using secure memory allocations.
 *
 * Returns: (transfer full): the newly created instance
 */
BobguiEntryBuffer *
bobgui_password_entry_buffer_new (void)
{
  return g_object_new (BOBGUI_TYPE_PASSWORD_ENTRY_BUFFER, NULL);
}
