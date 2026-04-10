/*
 * Copyright © 2018 Benjamin Otte
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
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Benjamin Otte <otte@gnome.org>
 */

#pragma once

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <gdk/gdk.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_MEDIA_STREAM             (bobgui_media_stream_get_type ())

GDK_AVAILABLE_IN_ALL
G_DECLARE_DERIVABLE_TYPE (BobguiMediaStream, bobgui_media_stream, BOBGUI, MEDIA_STREAM, GObject)

struct _BobguiMediaStreamClass
{
  GObjectClass parent_class;

  gboolean              (* play)                                (BobguiMediaStream *self);
  void                  (* pause)                               (BobguiMediaStream *self);
  void                  (* seek)                                (BobguiMediaStream *self,
                                                                 gint64          timestamp);
  void                  (* update_audio)                        (BobguiMediaStream *self,
                                                                 gboolean        muted,
                                                                 double          volume);
  void                  (* realize)                             (BobguiMediaStream *self,
                                                                 GdkSurface      *surface);
  void                  (* unrealize)                           (BobguiMediaStream *self,
                                                                 GdkSurface      *surface);

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
gboolean                bobgui_media_stream_is_prepared            (BobguiMediaStream *self);
GDK_AVAILABLE_IN_ALL
const GError *          bobgui_media_stream_get_error              (BobguiMediaStream *self);
GDK_AVAILABLE_IN_ALL
gboolean                bobgui_media_stream_has_audio              (BobguiMediaStream *self);
GDK_AVAILABLE_IN_ALL
gboolean                bobgui_media_stream_has_video              (BobguiMediaStream *self);

GDK_AVAILABLE_IN_ALL
void                    bobgui_media_stream_play                   (BobguiMediaStream *self);
GDK_AVAILABLE_IN_ALL
void                    bobgui_media_stream_pause                  (BobguiMediaStream *self);
GDK_AVAILABLE_IN_ALL
gboolean                bobgui_media_stream_get_playing            (BobguiMediaStream *self);
GDK_AVAILABLE_IN_ALL
void                    bobgui_media_stream_set_playing            (BobguiMediaStream *self,
                                                                 gboolean        playing);
GDK_AVAILABLE_IN_ALL
gboolean                bobgui_media_stream_get_ended              (BobguiMediaStream *self);

GDK_AVAILABLE_IN_ALL
gint64                  bobgui_media_stream_get_timestamp          (BobguiMediaStream *self);
GDK_AVAILABLE_IN_ALL
gint64                  bobgui_media_stream_get_duration           (BobguiMediaStream *self);

GDK_AVAILABLE_IN_ALL
gboolean                bobgui_media_stream_is_seekable            (BobguiMediaStream *self);
GDK_AVAILABLE_IN_ALL
gboolean                bobgui_media_stream_is_seeking             (BobguiMediaStream *self);
GDK_AVAILABLE_IN_ALL
void                    bobgui_media_stream_seek                   (BobguiMediaStream *self,
                                                                 gint64          timestamp);
GDK_AVAILABLE_IN_ALL
gboolean                bobgui_media_stream_get_loop               (BobguiMediaStream *self);
GDK_AVAILABLE_IN_ALL
void                    bobgui_media_stream_set_loop               (BobguiMediaStream *self,
                                                                 gboolean        loop);
GDK_AVAILABLE_IN_ALL
gboolean                bobgui_media_stream_get_muted              (BobguiMediaStream *self);
GDK_AVAILABLE_IN_ALL
void                    bobgui_media_stream_set_muted              (BobguiMediaStream *self,
                                                                 gboolean        muted);
GDK_AVAILABLE_IN_ALL
double                  bobgui_media_stream_get_volume             (BobguiMediaStream *self);
GDK_AVAILABLE_IN_ALL
void                    bobgui_media_stream_set_volume             (BobguiMediaStream *self,
                                                                 double          volume);
GDK_AVAILABLE_IN_ALL
void                    bobgui_media_stream_realize                (BobguiMediaStream *self,
                                                                 GdkSurface      *surface);
GDK_AVAILABLE_IN_ALL
void                    bobgui_media_stream_unrealize              (BobguiMediaStream *self,
                                                                 GdkSurface      *surface);

/* for implementations only */
GDK_DEPRECATED_IN_4_4_FOR(bobgui_media_stream_stream_prepared)
void                    bobgui_media_stream_prepared               (BobguiMediaStream *self,
                                                                 gboolean        has_audio,
                                                                 gboolean        has_video,
                                                                 gboolean        seekable,
                                                                 gint64          duration);
GDK_DEPRECATED_IN_4_4_FOR(bobgui_media_stream_stream_unprepared)
void                    bobgui_media_stream_unprepared             (BobguiMediaStream *self);

GDK_AVAILABLE_IN_4_4
void                    bobgui_media_stream_stream_prepared        (BobguiMediaStream *self,
                                                                 gboolean        has_audio,
                                                                 gboolean        has_video,
                                                                 gboolean        seekable,
                                                                 gint64          duration);
GDK_AVAILABLE_IN_4_4
void                    bobgui_media_stream_stream_unprepared      (BobguiMediaStream *self);

GDK_AVAILABLE_IN_ALL
void                    bobgui_media_stream_update                 (BobguiMediaStream *self,
                                                                 gint64          timestamp);
GDK_DEPRECATED_IN_4_4_FOR(bobgui_media_stream_stream_ended)
void                    bobgui_media_stream_ended                  (BobguiMediaStream *self);
GDK_AVAILABLE_IN_4_4
void                    bobgui_media_stream_stream_ended           (BobguiMediaStream *self);
GDK_AVAILABLE_IN_ALL
void                    bobgui_media_stream_seek_success           (BobguiMediaStream *self);
GDK_AVAILABLE_IN_ALL
void                    bobgui_media_stream_seek_failed            (BobguiMediaStream *self);
GDK_AVAILABLE_IN_ALL
void                    bobgui_media_stream_gerror                 (BobguiMediaStream *self,
                                                                 GError         *error);
GDK_AVAILABLE_IN_ALL
void                    bobgui_media_stream_error                  (BobguiMediaStream *self,
                                                                 GQuark          domain,
                                                                 int             code,
                                                                 const char     *format,
                                                                 ...) G_GNUC_PRINTF (4, 5);
GDK_AVAILABLE_IN_ALL
void                    bobgui_media_stream_error_valist           (BobguiMediaStream *self,
                                                                 GQuark          domain,
                                                                 int             code,
                                                                 const char     *format,
                                                                 va_list         args) G_GNUC_PRINTF (4, 0);

G_END_DECLS

