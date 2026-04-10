/* bobguientrybuffer.h
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

#pragma once

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <glib-object.h>
#include <gdk/gdk.h>

G_BEGIN_DECLS

/* Maximum size of text buffer, in bytes */
#define BOBGUI_ENTRY_BUFFER_MAX_SIZE        G_MAXUSHORT

#define BOBGUI_TYPE_ENTRY_BUFFER            (bobgui_entry_buffer_get_type ())
#define BOBGUI_ENTRY_BUFFER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_ENTRY_BUFFER, BobguiEntryBuffer))
#define BOBGUI_ENTRY_BUFFER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BOBGUI_TYPE_ENTRY_BUFFER, BobguiEntryBufferClass))
#define BOBGUI_IS_ENTRY_BUFFER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_ENTRY_BUFFER))
#define BOBGUI_IS_ENTRY_BUFFER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BOBGUI_TYPE_ENTRY_BUFFER))
#define BOBGUI_ENTRY_BUFFER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_ENTRY_BUFFER, BobguiEntryBufferClass))

typedef struct _BobguiEntryBuffer            BobguiEntryBuffer;
typedef struct _BobguiEntryBufferClass       BobguiEntryBufferClass;

struct _BobguiEntryBuffer
{
  GObject parent_instance;
};

struct _BobguiEntryBufferClass
{
  GObjectClass parent_class;

  /* Signals */

  void         (*inserted_text)          (BobguiEntryBuffer *buffer,
                                          guint           position,
                                          const char     *chars,
                                          guint           n_chars);

  void         (*deleted_text)           (BobguiEntryBuffer *buffer,
                                          guint           position,
                                          guint           n_chars);

  /* Virtual Methods */

  const char * (*get_text)               (BobguiEntryBuffer *buffer,
                                          gsize          *n_bytes);

  guint        (*get_length)             (BobguiEntryBuffer *buffer);

  guint        (*insert_text)            (BobguiEntryBuffer *buffer,
                                          guint           position,
                                          const char     *chars,
                                          guint           n_chars);

  guint        (*delete_text)            (BobguiEntryBuffer *buffer,
                                          guint           position,
                                          guint           n_chars);

  /* Padding for future expansion */
  void (*_bobgui_reserved1) (void);
  void (*_bobgui_reserved2) (void);
  void (*_bobgui_reserved3) (void);
  void (*_bobgui_reserved4) (void);
  void (*_bobgui_reserved5) (void);
  void (*_bobgui_reserved6) (void);
  void (*_bobgui_reserved7) (void);
  void (*_bobgui_reserved8) (void);
};

GDK_AVAILABLE_IN_ALL
GType                     bobgui_entry_buffer_get_type               (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
BobguiEntryBuffer*           bobgui_entry_buffer_new                    (const char      *initial_chars,
                                                                   int              n_initial_chars);

GDK_AVAILABLE_IN_ALL
gsize                     bobgui_entry_buffer_get_bytes              (BobguiEntryBuffer  *buffer);

GDK_AVAILABLE_IN_ALL
guint                     bobgui_entry_buffer_get_length             (BobguiEntryBuffer  *buffer);

GDK_AVAILABLE_IN_ALL
const char *              bobgui_entry_buffer_get_text               (BobguiEntryBuffer  *buffer);

GDK_AVAILABLE_IN_ALL
void                      bobgui_entry_buffer_set_text               (BobguiEntryBuffer  *buffer,
                                                                   const char      *chars,
                                                                   int              n_chars);

GDK_AVAILABLE_IN_ALL
void                      bobgui_entry_buffer_set_max_length         (BobguiEntryBuffer  *buffer,
                                                                   int              max_length);

GDK_AVAILABLE_IN_ALL
int                       bobgui_entry_buffer_get_max_length         (BobguiEntryBuffer  *buffer);

GDK_AVAILABLE_IN_ALL
guint                     bobgui_entry_buffer_insert_text            (BobguiEntryBuffer  *buffer,
                                                                   guint            position,
                                                                   const char      *chars,
                                                                   int              n_chars);

GDK_AVAILABLE_IN_ALL
guint                     bobgui_entry_buffer_delete_text            (BobguiEntryBuffer  *buffer,
                                                                   guint            position,
                                                                   int              n_chars);

GDK_AVAILABLE_IN_ALL
void                      bobgui_entry_buffer_emit_inserted_text     (BobguiEntryBuffer  *buffer,
                                                                   guint            position,
                                                                   const char      *chars,
                                                                   guint            n_chars);

GDK_AVAILABLE_IN_ALL
void                      bobgui_entry_buffer_emit_deleted_text      (BobguiEntryBuffer  *buffer,
                                                                   guint            position,
                                                                   guint            n_chars);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiEntryBuffer, g_object_unref)

G_END_DECLS

