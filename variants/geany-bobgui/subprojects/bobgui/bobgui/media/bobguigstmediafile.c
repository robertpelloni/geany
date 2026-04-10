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

#include "config.h"

#include "bobguigstmediafileprivate.h"
#include "bobguigstpaintableprivate.h"
#include "bobguimodulesprivate.h"
#include "bobguigstbinprivate.h"

#include <gst/play/play.h>

struct _BobguiGstMediaFile
{
  BobguiMediaFile parent_instance;

  GstPlay *play;
  GstPlaySignalAdapter *play_adapter;
  GdkPaintable *paintable;
  GstElement *playbin;
  BobguiGstBin *src;
};

struct _BobguiGstMediaFileClass
{
  BobguiMediaFileClass parent_class;
};

#define TO_GST_TIME(ts) ((ts) * (GST_SECOND / G_USEC_PER_SEC))
#define FROM_GST_TIME(ts) ((ts) / (GST_SECOND / G_USEC_PER_SEC))

static void
bobgui_gst_media_file_paintable_snapshot (GdkPaintable *paintable,
                                       GdkSnapshot  *snapshot,
                                       double        width,
                                       double        height)
{
  BobguiGstMediaFile *self = BOBGUI_GST_MEDIA_FILE (paintable);

  gdk_paintable_snapshot (self->paintable, snapshot, width, height);
}

static GdkPaintable *
bobgui_gst_media_file_paintable_get_current_image (GdkPaintable *paintable)
{
  BobguiGstMediaFile *self = BOBGUI_GST_MEDIA_FILE (paintable);

  return gdk_paintable_get_current_image (self->paintable);
}

static int
bobgui_gst_media_file_paintable_get_intrinsic_width (GdkPaintable *paintable)
{
  BobguiGstMediaFile *self = BOBGUI_GST_MEDIA_FILE (paintable);

  return gdk_paintable_get_intrinsic_width (self->paintable);
}

static int
bobgui_gst_media_file_paintable_get_intrinsic_height (GdkPaintable *paintable)
{
  BobguiGstMediaFile *self = BOBGUI_GST_MEDIA_FILE (paintable);

  return gdk_paintable_get_intrinsic_height (self->paintable);
}

static double bobgui_gst_media_file_paintable_get_intrinsic_aspect_ratio (GdkPaintable *paintable)
{
  BobguiGstMediaFile *self = BOBGUI_GST_MEDIA_FILE (paintable);

  return gdk_paintable_get_intrinsic_aspect_ratio (self->paintable);
};

static void
bobgui_gst_media_file_paintable_init (GdkPaintableInterface *iface)
{
  iface->snapshot = bobgui_gst_media_file_paintable_snapshot;
  iface->get_current_image = bobgui_gst_media_file_paintable_get_current_image;
  iface->get_intrinsic_width = bobgui_gst_media_file_paintable_get_intrinsic_width;
  iface->get_intrinsic_height = bobgui_gst_media_file_paintable_get_intrinsic_height;
  iface->get_intrinsic_aspect_ratio = bobgui_gst_media_file_paintable_get_intrinsic_aspect_ratio;
}

BOBGUI_DEFINE_BUILTIN_MODULE_TYPE_WITH_CODE (BobguiGstMediaFile, bobgui_gst_media_file, BOBGUI_TYPE_MEDIA_FILE,
                         G_IMPLEMENT_INTERFACE (GDK_TYPE_PAINTABLE,
                                                bobgui_gst_media_file_paintable_init);
                         g_io_extension_point_implement (BOBGUI_MEDIA_FILE_EXTENSION_POINT_NAME,
                                                         g_define_type_id,
                                                         "gstreamer",
                                                         20);
                         gst_init (NULL, NULL))

static void
bobgui_gst_media_file_ensure_prepared (BobguiGstMediaFile *self)
{
  GstPlayMediaInfo *media_info;

  if (bobgui_media_stream_is_prepared (BOBGUI_MEDIA_STREAM (self)))
    return;

  media_info = gst_play_get_media_info (self->play);
  if (media_info)
    {
      GstClockTime duration = gst_play_media_info_get_duration (media_info);

      bobgui_media_stream_stream_prepared (BOBGUI_MEDIA_STREAM (self),
                                        gst_play_media_info_get_number_of_audio_streams (media_info) > 0,
                                        gst_play_media_info_get_number_of_video_streams (media_info) > 0,
                                        gst_play_media_info_is_seekable (media_info),
                                        duration == GST_CLOCK_TIME_NONE ? 0 : FROM_GST_TIME (duration));

      g_object_unref (media_info);
    }
  else
    {
      /* Assuming everything exists is better for the user than pretending it doesn't exist.
       * Better to be able to control non-existing audio than not be able to control existing audio.
       *
       * Only for seeking we can't do a thing, because with 0 duration we can't seek anywhere.
       */
      bobgui_media_stream_stream_prepared (BOBGUI_MEDIA_STREAM (self),
                                        TRUE,
                                        TRUE,
                                        FALSE,
                                        0);
    }
}

#if GST_CHECK_VERSION (1,27, 90)
static void
bobgui_gst_media_file_update_loop (BobguiGstMediaFile *self)
{
  GstStructure *config;
  gboolean loop;

  config = gst_play_get_config (self->play);
  loop = bobgui_media_stream_get_loop (BOBGUI_MEDIA_STREAM (self));
  gst_play_config_set_loop (config, loop ? GST_PLAY_LOOP_TRACK : GST_PLAY_LOOP_NONE);
  gst_play_set_config (self->play, config);
}
#endif

static void
bobgui_gst_media_file_position_updated_cb (GstPlaySignalAdapter *adapter,
                                        GstClockTime          time,
                                        BobguiGstMediaFile      *self)
{
  bobgui_gst_media_file_ensure_prepared (self);

  bobgui_media_stream_update (BOBGUI_MEDIA_STREAM (self), FROM_GST_TIME (time));
}

static void
bobgui_gst_media_file_media_info_updated_cb (GstPlaySignalAdapter *adapter,
                                          GstPlayMediaInfo     *media_info,
                                          BobguiGstMediaFile      *self)
{
  /* clock_time == 0: https://gitlab.freedesktop.org/gstreamer/gst-plugins-bad/-/issues/1588
   * GstPlayer's first media-info-updated comes with 0 duration
   *
   * clock_time == -1: Seen on loading an audio-only ogg
   */
  GstClockTime clock_time = gst_play_media_info_get_duration (media_info);
  if (clock_time == 0 || clock_time == -1)
    return;

  bobgui_gst_media_file_ensure_prepared (self);
}

static void
bobgui_gst_media_file_seek_done_cb (GstPlaySignalAdapter *adapter,
                                 GstClockTime          time,
                                 BobguiGstMediaFile      *self)
{
  /* if we're not seeking, we're doing the loop seek-back after EOS */
  if (bobgui_media_stream_is_seeking (BOBGUI_MEDIA_STREAM (self)))
    bobgui_media_stream_seek_success (BOBGUI_MEDIA_STREAM (self));
  bobgui_media_stream_update (BOBGUI_MEDIA_STREAM (self), FROM_GST_TIME (time));
}

static void
bobgui_gst_media_file_error_cb (GstPlaySignalAdapter *adapter,
                             GError               *error,
                             GstStructure         *details,
                             BobguiGstMediaFile      *self)
{
  if (bobgui_media_stream_get_error (BOBGUI_MEDIA_STREAM (self)))
    return;

  bobgui_media_stream_gerror (BOBGUI_MEDIA_STREAM (self),
                           g_error_copy (error));
}

static void
bobgui_gst_media_file_end_of_stream_cb (GstPlaySignalAdapter *adapter,
                                     BobguiGstMediaFile      *self)
{
  bobgui_gst_media_file_ensure_prepared (self);

  if (bobgui_media_stream_get_ended (BOBGUI_MEDIA_STREAM (self)))
    return;

  if (bobgui_media_stream_get_loop (BOBGUI_MEDIA_STREAM (self)))
    {
#if !GST_CHECK_VERSION (1,27, 90)
      gst_play_seek (self->play, 0);
#endif
      return;
    }

  bobgui_media_stream_stream_ended (BOBGUI_MEDIA_STREAM (self));
}

static void
bobgui_gst_media_file_source_setup_cb (GstElement      *playbin,
                                    GstElement      *source,
                                    BobguiGstMediaFile *self)
{
  GFile *file;
  GInputStream *stream;

  g_return_if_fail (BOBGUI_IS_GST_BIN (source));

  self->src = BOBGUI_GST_BIN (g_object_ref (source));

  file = bobgui_media_file_get_file (BOBGUI_MEDIA_FILE (self));
  stream = bobgui_media_file_get_input_stream (BOBGUI_MEDIA_FILE (self));

  if (stream)
    g_object_ref (stream);
  else if (file)
    stream = G_INPUT_STREAM (g_file_read (file, NULL, NULL));
  else
    stream = NULL;

  g_return_if_fail (stream != NULL);

  bobgui_gst_bin_set_stream (self->src, stream);

  g_clear_object (&stream);
}

static void
bobgui_gst_media_file_destroy_play (BobguiGstMediaFile *self)
{
  if (self->play == NULL)
    return;

  g_signal_handlers_disconnect_by_func (self->play_adapter, bobgui_gst_media_file_media_info_updated_cb, self);
  g_signal_handlers_disconnect_by_func (self->play_adapter, bobgui_gst_media_file_position_updated_cb, self);
  g_signal_handlers_disconnect_by_func (self->play_adapter, bobgui_gst_media_file_end_of_stream_cb, self);
  g_signal_handlers_disconnect_by_func (self->play_adapter, bobgui_gst_media_file_seek_done_cb, self);
  g_signal_handlers_disconnect_by_func (self->play_adapter, bobgui_gst_media_file_error_cb, self);
  g_signal_handlers_disconnect_by_func (self->playbin, bobgui_gst_media_file_source_setup_cb, self);
#if GST_CHECK_VERSION (1,27, 90)
  g_signal_handlers_disconnect_by_func (self, bobgui_gst_media_file_update_loop, NULL);
#endif
  g_object_unref (self->play_adapter);
  gst_play_stop (self->play);
  g_object_unref (self->play);
  g_object_unref (self->playbin);
  g_object_unref (self->src);
  self->play = NULL;
}

static void
bobgui_gst_media_file_create_play (BobguiGstMediaFile *file)
{
  BobguiGstMediaFile *self = BOBGUI_GST_MEDIA_FILE (file);

  if (self->play != NULL)
    return;

  self->play = gst_play_new (GST_PLAY_VIDEO_RENDERER (g_object_ref (self->paintable)));
  self->play_adapter = gst_play_signal_adapter_new (self->play);
  g_signal_connect (self->play_adapter, "media-info-updated", G_CALLBACK (bobgui_gst_media_file_media_info_updated_cb), self);
  g_signal_connect (self->play_adapter, "position-updated", G_CALLBACK (bobgui_gst_media_file_position_updated_cb), self);
  g_signal_connect (self->play_adapter, "end-of-stream", G_CALLBACK (bobgui_gst_media_file_end_of_stream_cb), self);
  g_signal_connect (self->play_adapter, "seek-done", G_CALLBACK (bobgui_gst_media_file_seek_done_cb), self);
  g_signal_connect (self->play_adapter, "error", G_CALLBACK (bobgui_gst_media_file_error_cb), self);

  g_object_get (self->play, "pipeline", &self->playbin, NULL);
  g_signal_connect (self->playbin, "source-setup", G_CALLBACK (bobgui_gst_media_file_source_setup_cb), self);

#if GST_CHECK_VERSION (1,27, 90)
  g_signal_connect (self, "notify::loop", G_CALLBACK (bobgui_gst_media_file_update_loop), NULL);
  bobgui_gst_media_file_update_loop (self);
#endif
}

static void
bobgui_gst_media_file_open (BobguiMediaFile *media_file)
{
  BobguiGstMediaFile *self = BOBGUI_GST_MEDIA_FILE (media_file);

  bobgui_gst_media_file_create_play (self);

  gst_play_set_uri (self->play, "bobgui-media-stream://");

  gst_play_pause (self->play);
}

static void
bobgui_gst_media_file_close (BobguiMediaFile *file)
{
  BobguiGstMediaFile *self = BOBGUI_GST_MEDIA_FILE (file);

  bobgui_gst_media_file_destroy_play (self);
}

static gboolean
bobgui_gst_media_file_play (BobguiMediaStream *stream)
{
  BobguiGstMediaFile *self = BOBGUI_GST_MEDIA_FILE (stream);

  if (self->play == NULL)
    return FALSE;

  gst_play_play (self->play);

  return TRUE;
}

static void
bobgui_gst_media_file_pause (BobguiMediaStream *stream)
{
  BobguiGstMediaFile *self = BOBGUI_GST_MEDIA_FILE (stream);

  gst_play_pause (self->play);
}

static void
bobgui_gst_media_file_seek (BobguiMediaStream *stream,
                         gint64          timestamp)
{
  BobguiGstMediaFile *self = BOBGUI_GST_MEDIA_FILE (stream);

  gst_play_seek (self->play, TO_GST_TIME (timestamp));
}

static void
bobgui_gst_media_file_update_audio (BobguiMediaStream *stream,
                                 gboolean        muted,
                                 double          volume)
{
  BobguiGstMediaFile *self = BOBGUI_GST_MEDIA_FILE (stream);

  gst_play_set_mute (self->play, muted);
  gst_play_set_volume (self->play, volume * volume * volume);
}

static void
bobgui_gst_media_file_realize (BobguiMediaStream *stream,
                            GdkSurface     *surface)
{
  BobguiGstMediaFile *self = BOBGUI_GST_MEDIA_FILE (stream);

  bobgui_gst_paintable_realize (BOBGUI_GST_PAINTABLE (self->paintable), surface);
}

static void
bobgui_gst_media_file_unrealize (BobguiMediaStream *stream,
                              GdkSurface     *surface)
{
  BobguiGstMediaFile *self = BOBGUI_GST_MEDIA_FILE (stream);

  bobgui_gst_paintable_unrealize (BOBGUI_GST_PAINTABLE (self->paintable), surface);
}

static void
bobgui_gst_media_file_dispose (GObject *object)
{
  BobguiGstMediaFile *self = BOBGUI_GST_MEDIA_FILE (object);

  bobgui_gst_media_file_destroy_play (self);
  if (self->paintable)
    {
      g_signal_handlers_disconnect_by_func (self->paintable, gdk_paintable_invalidate_size, self);
      g_signal_handlers_disconnect_by_func (self->paintable, gdk_paintable_invalidate_contents, self);
      g_clear_object (&self->paintable);
    }

  G_OBJECT_CLASS (bobgui_gst_media_file_parent_class)->dispose (object);
}

static void
bobgui_gst_media_file_class_init (BobguiGstMediaFileClass *klass)
{
  BobguiMediaFileClass *file_class = BOBGUI_MEDIA_FILE_CLASS (klass);
  BobguiMediaStreamClass *stream_class = BOBGUI_MEDIA_STREAM_CLASS (klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gst_element_register (NULL, "BobguiGstBin", GST_RANK_PRIMARY, BOBGUI_TYPE_GST_BIN);

  file_class->open = bobgui_gst_media_file_open;
  file_class->close = bobgui_gst_media_file_close;

  stream_class->play = bobgui_gst_media_file_play;
  stream_class->pause = bobgui_gst_media_file_pause;
  stream_class->seek = bobgui_gst_media_file_seek;
  stream_class->update_audio = bobgui_gst_media_file_update_audio;
  stream_class->realize = bobgui_gst_media_file_realize;
  stream_class->unrealize = bobgui_gst_media_file_unrealize;

  gobject_class->dispose = bobgui_gst_media_file_dispose;
}

static void
bobgui_gst_media_file_init (BobguiGstMediaFile *self)
{
  self->paintable = bobgui_gst_paintable_new ();
  g_signal_connect_swapped (self->paintable, "invalidate-size", G_CALLBACK (gdk_paintable_invalidate_size), self);
  g_signal_connect_swapped (self->paintable, "invalidate-contents", G_CALLBACK (gdk_paintable_invalidate_contents), self);
}
