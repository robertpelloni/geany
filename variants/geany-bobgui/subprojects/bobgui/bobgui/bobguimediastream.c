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

#include "bobguimediastream.h"


/**
 * BobguiMediaStream:
 *
 * The integration point for media playback inside BOBGUI.
 *
 * BOBGUI provides an implementation of the `BobguiMediaStream` interface that
 * is called [class@Bobgui.MediaFile].
 *
 * Apart from application-facing API for stream playback, `BobguiMediaStream`
 * has a number of APIs that are only useful for implementations and should
 * not be used in applications:
 * [method@Bobgui.MediaStream.prepared],
 * [method@Bobgui.MediaStream.unprepared],
 * [method@Bobgui.MediaStream.update],
 * [method@Bobgui.MediaStream.ended],
 * [method@Bobgui.MediaStream.seek_success],
 * [method@Bobgui.MediaStream.seek_failed],
 * [method@Bobgui.MediaStream.gerror],
 * [method@Bobgui.MediaStream.error],
 * [method@Bobgui.MediaStream.error_valist].
 */

typedef struct _BobguiMediaStreamPrivate BobguiMediaStreamPrivate;

struct _BobguiMediaStreamPrivate
{
  gint64 timestamp;
  gint64 duration;
  GError *error;
  double volume;

  guint has_audio : 1;
  guint has_video : 1;
  guint playing : 1;
  guint ended : 1;
  guint seekable : 1;
  guint seeking : 1;
  guint loop : 1;
  guint prepared : 1;
  guint muted : 1;
};

enum {
  PROP_0,
  PROP_PREPARED,
  PROP_ERROR,
  PROP_HAS_AUDIO,
  PROP_HAS_VIDEO,
  PROP_PLAYING,
  PROP_ENDED,
  PROP_TIMESTAMP,
  PROP_DURATION,
  PROP_SEEKABLE,
  PROP_SEEKING,
  PROP_LOOP,
  PROP_MUTED,
  PROP_VOLUME,

  N_PROPS,
};

static GParamSpec *properties[N_PROPS] = { NULL, };

static void
bobgui_media_stream_paintable_snapshot (GdkPaintable *paintable,
                                     GdkSnapshot  *snapshot,
                                     double        width,
                                     double        height)
{
}

static void
bobgui_media_stream_paintable_init (GdkPaintableInterface *iface)
{
  /* We implement the behavior for "no video stream" here */
  iface->snapshot = bobgui_media_stream_paintable_snapshot;
}

G_DEFINE_ABSTRACT_TYPE_WITH_CODE (BobguiMediaStream, bobgui_media_stream, G_TYPE_OBJECT,
                                  G_IMPLEMENT_INTERFACE (GDK_TYPE_PAINTABLE,
                                                         bobgui_media_stream_paintable_init)
                                  G_ADD_PRIVATE (BobguiMediaStream))

#define BOBGUI_MEDIA_STREAM_WARN_NOT_IMPLEMENTED_METHOD(obj,method) \
  g_critical ("Media stream of type '%s' does not implement BobguiMediaStream::" # method, G_OBJECT_TYPE_NAME (obj))

static gboolean
bobgui_media_stream_default_play (BobguiMediaStream *self)
{
  BOBGUI_MEDIA_STREAM_WARN_NOT_IMPLEMENTED_METHOD (self, play);

  return FALSE;
}

static void
bobgui_media_stream_default_pause (BobguiMediaStream *self)
{
  BOBGUI_MEDIA_STREAM_WARN_NOT_IMPLEMENTED_METHOD (self, pause);
}

static void
bobgui_media_stream_default_seek (BobguiMediaStream *self,
                               gint64          timestamp)
{
  bobgui_media_stream_seek_failed (self);
}

static void
bobgui_media_stream_default_update_audio (BobguiMediaStream *self,
                                       gboolean        muted,
                                       double          volume)
{
}

static void
bobgui_media_stream_default_realize (BobguiMediaStream *self,
                                  GdkSurface      *surface)
{
}

static void
bobgui_media_stream_default_unrealize (BobguiMediaStream *self,
                                    GdkSurface      *surface)
{
}

static void
bobgui_media_stream_set_property (GObject      *object,
                               guint         prop_id,
                               const GValue *value,
                               GParamSpec   *pspec)

{
  BobguiMediaStream *self = BOBGUI_MEDIA_STREAM (object);

  switch (prop_id)
    {
    case PROP_PLAYING:
      bobgui_media_stream_set_playing (self, g_value_get_boolean (value));
      break;

    case PROP_LOOP:
      bobgui_media_stream_set_loop (self, g_value_get_boolean (value));
      break;

    case PROP_MUTED:
      bobgui_media_stream_set_muted (self, g_value_get_boolean (value));
      break;

    case PROP_VOLUME:
      bobgui_media_stream_set_volume (self, g_value_get_double (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_media_stream_get_property (GObject    *object,
                               guint       prop_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  BobguiMediaStream *self = BOBGUI_MEDIA_STREAM (object);
  BobguiMediaStreamPrivate *priv = bobgui_media_stream_get_instance_private (self);

  switch (prop_id)
    {
    case PROP_PREPARED:
      g_value_set_boolean (value, priv->prepared);
      break;

    case PROP_ERROR:
      g_value_set_boxed (value, priv->error);
      break;

    case PROP_HAS_AUDIO:
      g_value_set_boolean (value, priv->has_audio);
      break;

    case PROP_HAS_VIDEO:
      g_value_set_boolean (value, priv->has_video);
      break;

    case PROP_PLAYING:
      g_value_set_boolean (value, priv->playing);
      break;

    case PROP_ENDED:
      g_value_set_boolean (value, priv->ended);
      break;

    case PROP_TIMESTAMP:
      g_value_set_int64 (value, priv->timestamp);
      break;

    case PROP_DURATION:
      g_value_set_int64 (value, priv->duration);
      break;

    case PROP_SEEKABLE:
      g_value_set_boolean (value, priv->seekable);
      break;

    case PROP_SEEKING:
      g_value_set_boolean (value, priv->seeking);
      break;

    case PROP_LOOP:
      g_value_set_boolean (value, priv->loop);
      break;

    case PROP_MUTED:
      g_value_set_boolean (value, priv->muted);
      break;

    case PROP_VOLUME:
      g_value_set_double (value, priv->volume);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_media_stream_dispose (GObject *object)
{
  BobguiMediaStream *self = BOBGUI_MEDIA_STREAM (object);
  BobguiMediaStreamPrivate *priv = bobgui_media_stream_get_instance_private (self);

  g_clear_error (&priv->error);

  G_OBJECT_CLASS (bobgui_media_stream_parent_class)->dispose (object);
}

static void
bobgui_media_stream_finalize (GObject *object)
{
#if 0
  BobguiMediaStream *self = BOBGUI_MEDIA_STREAM (object);
  BobguiMediaStreamPrivate *priv = bobgui_media_stream_get_instance_private (self);
#endif

  G_OBJECT_CLASS (bobgui_media_stream_parent_class)->finalize (object);
}

static void
bobgui_media_stream_class_init (BobguiMediaStreamClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);

  class->play = bobgui_media_stream_default_play;
  class->pause = bobgui_media_stream_default_pause;
  class->seek = bobgui_media_stream_default_seek;
  class->update_audio = bobgui_media_stream_default_update_audio;
  class->realize = bobgui_media_stream_default_realize;
  class->unrealize = bobgui_media_stream_default_unrealize;

  gobject_class->set_property = bobgui_media_stream_set_property;
  gobject_class->get_property = bobgui_media_stream_get_property;
  gobject_class->finalize = bobgui_media_stream_finalize;
  gobject_class->dispose = bobgui_media_stream_dispose;

  /**
   * BobguiMediaStream:prepared: (getter is_prepared)
   *
   * Whether the stream has finished initializing and existence of
   * audio and video is known.
   */
  properties[PROP_PREPARED] =
    g_param_spec_boolean ("prepared", NULL, NULL,
                          FALSE,
                          G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * BobguiMediaStream:error:
   *
   * %NULL for a properly working stream or the `GError`
   * that the stream is in.
   */
  properties[PROP_ERROR] =
    g_param_spec_boxed ("error", NULL, NULL,
                        G_TYPE_ERROR,
                        G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * BobguiMediaStream:has-audio:
   *
   * Whether the stream contains audio.
   */
  properties[PROP_HAS_AUDIO] =
    g_param_spec_boolean ("has-audio", NULL, NULL,
                          FALSE,
                          G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * BobguiMediaStream:has-video:
   *
   * Whether the stream contains video.
   */
  properties[PROP_HAS_VIDEO] =
    g_param_spec_boolean ("has-video", NULL, NULL,
                          FALSE,
                          G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * BobguiMediaStream:playing:
   *
   * Whether the stream is currently playing.
   */
  properties[PROP_PLAYING] =
    g_param_spec_boolean ("playing", NULL, NULL,
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * BobguiMediaStream:ended: (getter get_ended)
   *
   * Set when playback has finished.
   */
  properties[PROP_ENDED] =
    g_param_spec_boolean ("ended", NULL, NULL,
                          FALSE,
                          G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * BobguiMediaStream:timestamp:
   *
   * The current presentation timestamp in microseconds.
   */
  properties[PROP_TIMESTAMP] =
    g_param_spec_int64 ("timestamp", NULL, NULL,
                        0, G_MAXINT64, 0,
                        G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * BobguiMediaStream:duration:
   *
   * The stream's duration in microseconds or 0 if unknown.
   */
  properties[PROP_DURATION] =
    g_param_spec_int64 ("duration", NULL, NULL,
                        0, G_MAXINT64, 0,
                        G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * BobguiMediaStream:seekable: (getter is_seekable)
   *
   * Set unless the stream is known to not support seeking.
   */
  properties[PROP_SEEKABLE] =
    g_param_spec_boolean ("seekable", NULL, NULL,
                          TRUE,
                          G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * BobguiMediaStream:seeking: (getter is_seeking)
   *
   * Set while a seek is in progress.
   */
  properties[PROP_SEEKING] =
    g_param_spec_boolean ("seeking", NULL, NULL,
                          FALSE,
                          G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * BobguiMediaStream:loop:
   *
   * Try to restart the media from the beginning once it ended.
   */
  properties[PROP_LOOP] =
    g_param_spec_boolean ("loop", NULL, NULL,
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * BobguiMediaStream:muted:
   *
   * Whether the audio stream should be muted.
   */
  properties[PROP_MUTED] =
    g_param_spec_boolean ("muted", NULL, NULL,
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * BobguiMediaStream:volume:
   *
   * Volume of the audio stream.
   */
  properties[PROP_VOLUME] =
    g_param_spec_double ("volume", NULL, NULL,
                         0.0, 1.0, 1.0,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (gobject_class, N_PROPS, properties);
}

static void
bobgui_media_stream_init (BobguiMediaStream *self)
{
  BobguiMediaStreamPrivate *priv = bobgui_media_stream_get_instance_private (self);

  priv->volume = 1.0;
}

/**
 * bobgui_media_stream_is_prepared: (get-property prepared)
 * @self: a `BobguiMediaStream`
 *
 * Returns whether the stream has finished initializing.
 *
 * At this point the existence of audio and video is known.
 *
 * Returns: %TRUE if the stream is prepared
 */
gboolean
bobgui_media_stream_is_prepared (BobguiMediaStream *self)
{
  BobguiMediaStreamPrivate *priv = bobgui_media_stream_get_instance_private (self);

  g_return_val_if_fail (BOBGUI_IS_MEDIA_STREAM (self), FALSE);

  return priv->prepared;
}

/**
 * bobgui_media_stream_has_audio:
 * @self: a `BobguiMediaStream`
 *
 * Returns whether the stream has audio.
 *
 * Returns: %TRUE if the stream has audio
 */
gboolean
bobgui_media_stream_has_audio (BobguiMediaStream *self)
{
  BobguiMediaStreamPrivate *priv = bobgui_media_stream_get_instance_private (self);

  g_return_val_if_fail (BOBGUI_IS_MEDIA_STREAM (self), FALSE);

  return priv->has_audio;
}

/**
 * bobgui_media_stream_has_video:
 * @self: a `BobguiMediaStream`
 *
 * Returns whether the stream has video.
 *
 * Returns: %TRUE if the stream has video
 */
gboolean
bobgui_media_stream_has_video (BobguiMediaStream *self)
{
  BobguiMediaStreamPrivate *priv = bobgui_media_stream_get_instance_private (self);

  g_return_val_if_fail (BOBGUI_IS_MEDIA_STREAM (self), FALSE);

  return priv->has_video;
}

/**
 * bobgui_media_stream_play:
 * @self: a `BobguiMediaStream`
 *
 * Starts playing the stream.
 *
 * If the stream is in error or already playing, do nothing.
 */
void
bobgui_media_stream_play (BobguiMediaStream *self)
{
  BobguiMediaStreamPrivate *priv = bobgui_media_stream_get_instance_private (self);

  g_return_if_fail (BOBGUI_IS_MEDIA_STREAM (self));

  if (priv->error)
    return;

  if (priv->playing)
    return;

  if (BOBGUI_MEDIA_STREAM_GET_CLASS (self)->play (self))
    {
      g_object_freeze_notify (G_OBJECT (self));

      priv->playing = TRUE;
      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_PLAYING]);

      if (priv->ended)
        {
          priv->ended = FALSE;
          g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_ENDED]);
        }

      g_object_thaw_notify (G_OBJECT (self));
    }
}

/**
 * bobgui_media_stream_pause:
 * @self: a `BobguiMediaStream`
 *
 * Pauses playback of the stream.
 *
 * If the stream is not playing, do nothing.
 */
void
bobgui_media_stream_pause (BobguiMediaStream *self)
{
  BobguiMediaStreamPrivate *priv = bobgui_media_stream_get_instance_private (self);

  g_return_if_fail (BOBGUI_IS_MEDIA_STREAM (self));

  /* Don't check for error here because we call this function right
   * after setting the error to pause the stream */

  if (!priv->playing)
    return;

  BOBGUI_MEDIA_STREAM_GET_CLASS (self)->pause (self);

  priv->playing = FALSE;
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_PLAYING]);
}

/**
 * bobgui_media_stream_get_playing:
 * @self: a `BobguiMediaStream`
 *
 * Return whether the stream is currently playing.
 *
 * Returns: %TRUE if the stream is playing
 */
gboolean
bobgui_media_stream_get_playing (BobguiMediaStream *self)
{
  BobguiMediaStreamPrivate *priv = bobgui_media_stream_get_instance_private (self);

  g_return_val_if_fail (BOBGUI_IS_MEDIA_STREAM (self), FALSE);

  return priv->playing;
}

/**
 * bobgui_media_stream_set_playing:
 * @self: a `BobguiMediaStream`
 * @playing: whether to start or pause playback
 *
 * Starts or pauses playback of the stream.
 */
void
bobgui_media_stream_set_playing (BobguiMediaStream *self,
                              gboolean        playing)
{
  g_return_if_fail (BOBGUI_IS_MEDIA_STREAM (self));

  if (playing)
    bobgui_media_stream_play (self);
  else
    bobgui_media_stream_pause (self);
}

/**
 * bobgui_media_stream_get_ended:
 * @self: a `BobguiMediaStream`
 *
 * Returns whether the streams playback is finished.
 *
 * Returns: %TRUE if playback is finished
 */
gboolean
bobgui_media_stream_get_ended (BobguiMediaStream *self)
{
  BobguiMediaStreamPrivate *priv = bobgui_media_stream_get_instance_private (self);

  g_return_val_if_fail (BOBGUI_IS_MEDIA_STREAM (self), FALSE);

  return priv->ended;
}

/**
 * bobgui_media_stream_get_timestamp:
 * @self: a `BobguiMediaStream`
 *
 * Returns the current presentation timestamp in microseconds.
 *
 * Returns: the timestamp in microseconds
 */
gint64
bobgui_media_stream_get_timestamp (BobguiMediaStream *self)
{
  BobguiMediaStreamPrivate *priv = bobgui_media_stream_get_instance_private (self);

  g_return_val_if_fail (BOBGUI_IS_MEDIA_STREAM (self), FALSE);

  return priv->timestamp;
}

/**
 * bobgui_media_stream_get_duration:
 * @self: a `BobguiMediaStream`
 *
 * Gets the duration of the stream.
 *
 * If the duration is not known, 0 will be returned.
 *
 * Returns: the duration of the stream or 0 if not known.
 */
gint64
bobgui_media_stream_get_duration (BobguiMediaStream *self)
{
  BobguiMediaStreamPrivate *priv = bobgui_media_stream_get_instance_private (self);

  g_return_val_if_fail (BOBGUI_IS_MEDIA_STREAM (self), FALSE);

  return priv->duration;
}

/**
 * bobgui_media_stream_is_seekable: (get-property seekable)
 * @self: a `BobguiMediaStream`
 *
 * Checks if a stream may be seekable.
 *
 * This is meant to be a hint. Streams may not allow seeking even if
 * this function returns %TRUE. However, if this function returns
 * %FALSE, streams are guaranteed to not be seekable and user interfaces
 * may hide controls that allow seeking.
 *
 * It is allowed to call [method@Bobgui.MediaStream.seek] on a non-seekable
 * stream, though it will not do anything.
 *
 * Returns: %TRUE if the stream may support seeking
 */
gboolean
bobgui_media_stream_is_seekable (BobguiMediaStream *self)
{
  BobguiMediaStreamPrivate *priv = bobgui_media_stream_get_instance_private (self);

  g_return_val_if_fail (BOBGUI_IS_MEDIA_STREAM (self), FALSE);

  return priv->seekable;
}

/**
 * bobgui_media_stream_is_seeking: (get-property seeking)
 * @self: a `BobguiMediaStream`
 *
 * Checks if there is currently a seek operation going on.
 *
 * Returns: %TRUE if a seek operation is ongoing.
 */
gboolean
bobgui_media_stream_is_seeking (BobguiMediaStream *self)
{
  BobguiMediaStreamPrivate *priv = bobgui_media_stream_get_instance_private (self);

  g_return_val_if_fail (BOBGUI_IS_MEDIA_STREAM (self), FALSE);

  return priv->seeking;
}

/**
 * bobgui_media_stream_get_error:
 * @self: a `BobguiMediaStream`
 *
 * If the stream is in an error state, returns the `GError`
 * explaining that state.
 *
 * Any type of error can be reported here depending on the
 * implementation of the media stream.
 *
 * A media stream in an error cannot be operated on, calls
 * like [method@Bobgui.MediaStream.play] or
 * [method@Bobgui.MediaStream.seek] will not have any effect.
 *
 * `BobguiMediaStream` itself does not provide a way to unset
 * an error, but implementations may provide options. For example,
 * a [class@Bobgui.MediaFile] will unset errors when a new source is
 * set, e.g. with [method@Bobgui.MediaFile.set_file].
 *
 * Returns: (nullable) (transfer none): %NULL if not in an
 *   error state or the `GError` of the stream
 */
const GError *
bobgui_media_stream_get_error (BobguiMediaStream *self)
{
  BobguiMediaStreamPrivate *priv = bobgui_media_stream_get_instance_private (self);

  g_return_val_if_fail (BOBGUI_IS_MEDIA_STREAM (self), FALSE);

  return priv->error;
}

/**
 * bobgui_media_stream_seek:
 * @self: a `BobguiMediaStream`
 * @timestamp: timestamp to seek to.
 *
 * Start a seek operation on @self to @timestamp.
 *
 * If @timestamp is out of range, it will be clamped.
 *
 * Seek operations may not finish instantly. While a
 * seek operation is in process, the [property@Bobgui.MediaStream:seeking]
 * property will be set.
 *
 * When calling bobgui_media_stream_seek() during an
 * ongoing seek operation, the new seek will override
 * any pending seek.
 */
void
bobgui_media_stream_seek (BobguiMediaStream *self,
                       gint64          timestamp)
{
  BobguiMediaStreamPrivate *priv = bobgui_media_stream_get_instance_private (self);
  gboolean was_seeking;

  g_return_if_fail (BOBGUI_IS_MEDIA_STREAM (self));
  g_return_if_fail (timestamp >= 0);

  if (priv->error)
    return;

  if (!priv->seekable)
    return;

  g_object_freeze_notify (G_OBJECT (self));

  was_seeking = priv->seeking;
  priv->seeking = TRUE;

  BOBGUI_MEDIA_STREAM_GET_CLASS (self)->seek (self, timestamp);

  if (was_seeking != priv->seeking)
    g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_SEEKING]);

  g_object_thaw_notify (G_OBJECT (self));
}

/**
 * bobgui_media_stream_get_loop:
 * @self: a `BobguiMediaStream`
 *
 * Returns whether the stream is set to loop.
 *
 * See [method@Bobgui.MediaStream.set_loop] for details.
 *
 * Returns: %TRUE if the stream should loop
 */
gboolean
bobgui_media_stream_get_loop (BobguiMediaStream *self)
{
  BobguiMediaStreamPrivate *priv = bobgui_media_stream_get_instance_private (self);

  g_return_val_if_fail (BOBGUI_IS_MEDIA_STREAM (self), FALSE);

  return priv->loop;
}

/**
 * bobgui_media_stream_set_loop:
 * @self: a `BobguiMediaStream`
 * @loop: %TRUE if the stream should loop
 *
 * Sets whether the stream should loop.
 *
 * In this case, it will attempt to restart playback
 * from the beginning instead of stopping at the end.
 *
 * Not all streams may support looping, in particular
 * non-seekable streams. Those streams will ignore the
 * loop setting and just end.
 */
void
bobgui_media_stream_set_loop (BobguiMediaStream *self,
                           gboolean        loop)
{
  BobguiMediaStreamPrivate *priv = bobgui_media_stream_get_instance_private (self);

  g_return_if_fail (BOBGUI_IS_MEDIA_STREAM (self));

  if (priv->loop == loop)
    return;

  priv->loop = loop;
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_LOOP]);
}

/**
 * bobgui_media_stream_get_muted:
 * @self: a `BobguiMediaStream`
 *
 * Returns whether the audio for the stream is muted.
 *
 * See [method@Bobgui.MediaStream.set_muted] for details.
 *
 * Returns: %TRUE if the stream is muted
 */
gboolean
bobgui_media_stream_get_muted (BobguiMediaStream *self)
{
  BobguiMediaStreamPrivate *priv = bobgui_media_stream_get_instance_private (self);

  g_return_val_if_fail (BOBGUI_IS_MEDIA_STREAM (self), FALSE);

  return priv->muted;
}

/**
 * bobgui_media_stream_set_muted:
 * @self: a `BobguiMediaStream`
 * @muted: %TRUE if the stream should be muted
 *
 * Sets whether the audio stream should be muted.
 *
 * Muting a stream will cause no audio to be played, but it
 * does not modify the volume. This means that muting and
 * then unmuting the stream will restore the volume settings.
 *
 * If the stream has no audio, calling this function will
 * still work but it will not have an audible effect.
 */
void
bobgui_media_stream_set_muted (BobguiMediaStream *self,
                            gboolean        muted)
{
  BobguiMediaStreamPrivate *priv = bobgui_media_stream_get_instance_private (self);

  g_return_if_fail (BOBGUI_IS_MEDIA_STREAM (self));

  if (priv->muted == muted)
    return;

  priv->muted = muted;

  BOBGUI_MEDIA_STREAM_GET_CLASS (self)->update_audio (self, priv->muted, priv->volume);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_MUTED]);
}

/**
 * bobgui_media_stream_get_volume:
 * @self: a `BobguiMediaStream`
 *
 * Returns the volume of the audio for the stream.
 *
 * See [method@Bobgui.MediaStream.set_volume] for details.
 *
 * Returns: volume of the stream from 0.0 to 1.0
 */
double
bobgui_media_stream_get_volume (BobguiMediaStream *self)
{
  BobguiMediaStreamPrivate *priv = bobgui_media_stream_get_instance_private (self);

  g_return_val_if_fail (BOBGUI_IS_MEDIA_STREAM (self), FALSE);

  return priv->volume;
}

/**
 * bobgui_media_stream_set_volume:
 * @self: a `BobguiMediaStream`
 * @volume: New volume of the stream from 0.0 to 1.0
 *
 * Sets the volume of the audio stream.
 *
 * This function call will work even if the stream is muted.
 *
 * The given @volume should range from 0.0 for silence to 1.0
 * for as loud as possible. Values outside of this range will
 * be clamped to the nearest value.
 *
 * If the stream has no audio or is muted, calling this function
 * will still work but it will not have an immediate audible effect.
 * When the stream is unmuted, the new volume setting will take effect.
 */
void
bobgui_media_stream_set_volume (BobguiMediaStream *self,
                             double          volume)
{
  BobguiMediaStreamPrivate *priv = bobgui_media_stream_get_instance_private (self);

  g_return_if_fail (BOBGUI_IS_MEDIA_STREAM (self));

  volume = CLAMP (volume, 0.0, 1.0);

  if (priv->volume == volume)
    return;

  priv->volume = volume;

  BOBGUI_MEDIA_STREAM_GET_CLASS (self)->update_audio (self, priv->muted, priv->volume);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_VOLUME]);
}

/**
 * bobgui_media_stream_realize:
 * @self: a `BobguiMediaStream`
 * @surface: a `GdkSurface`
 *
 * Called by users to attach the media stream to a `GdkSurface` they manage.
 *
 * The stream can then access the resources of @surface for its
 * rendering purposes. In particular, media streams might want to
 * create a `GdkGLContext` or sync to the `GdkFrameClock`.
 *
 * Whoever calls this function is responsible for calling
 * [method@Bobgui.MediaStream.unrealize] before either the stream
 * or @surface get destroyed.
 *
 * Multiple calls to this function may happen from different
 * users of the video, even with the same @surface. Each of these
 * calls must be followed by its own call to
 * [method@Bobgui.MediaStream.unrealize].
 *
 * It is not required to call this function to make a media stream work.
 */
void
bobgui_media_stream_realize (BobguiMediaStream *self,
                          GdkSurface      *surface)
{
  g_return_if_fail (BOBGUI_IS_MEDIA_STREAM (self));
  g_return_if_fail (GDK_IS_SURFACE (surface));

  g_object_ref (self);
  g_object_ref (surface);

  BOBGUI_MEDIA_STREAM_GET_CLASS (self)->realize (self, surface);
}

/**
 * bobgui_media_stream_unrealize:
 * @self: a `BobguiMediaStream` previously realized
 * @surface: the `GdkSurface` the stream was realized with
 *
 * Undoes a previous call to bobgui_media_stream_realize().
 *
 * This causes the stream to release all resources it had
 * allocated from @surface.
 */
void
bobgui_media_stream_unrealize (BobguiMediaStream *self,
                            GdkSurface      *surface)
{
  g_return_if_fail (BOBGUI_IS_MEDIA_STREAM (self));
  g_return_if_fail (GDK_IS_SURFACE (surface));

  BOBGUI_MEDIA_STREAM_GET_CLASS (self)->unrealize (self, surface);

  g_object_unref (surface);
  g_object_unref (self);
}

/**
 * bobgui_media_stream_stream_prepared:
 * @self: a `BobguiMediaStream`
 * @has_audio: %TRUE if the stream should advertise audio support
 * @has_video: %TRUE if the stream should advertise video support
 * @seekable: %TRUE if the stream should advertise seekability
 * @duration: The duration of the stream or 0 if unknown
 *
 * Called by `BobguiMediaStream` implementations to advertise the stream
 * being ready to play and providing details about the stream.
 *
 * Note that the arguments are hints. If the stream implementation
 * cannot determine the correct values, it is better to err on the
 * side of caution and return %TRUE. User interfaces will use those
 * values to determine what controls to show.
 *
 * This function may not be called again until the stream has been
 * reset via [method@Bobgui.MediaStream.stream_unprepared].
 *
 * Since: 4.4
 */
void
bobgui_media_stream_stream_prepared (BobguiMediaStream *self,
                                  gboolean        has_audio,
                                  gboolean        has_video,
                                  gboolean        seekable,
                                  gint64          duration)
{
  BobguiMediaStreamPrivate *priv = bobgui_media_stream_get_instance_private (self);

  g_return_if_fail (BOBGUI_IS_MEDIA_STREAM (self));
  g_return_if_fail (!bobgui_media_stream_is_prepared (self));

  g_object_freeze_notify (G_OBJECT (self));

  if (priv->has_audio != has_audio)
    {
      priv->has_audio = has_audio;
      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_HAS_AUDIO]);
    }
  if (priv->has_video != has_video)
    {
      priv->has_video = has_video;
      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_HAS_VIDEO]);
    }
  if (priv->seekable != seekable)
    {
      priv->seekable = seekable;
      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_SEEKABLE]);
    }
  if (priv->duration != duration)
    {
      priv->duration = duration;
      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_DURATION]);
    }

  priv->prepared = TRUE;
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_PREPARED]);

  g_object_thaw_notify (G_OBJECT (self));
}

/**
 * bobgui_media_stream_stream_unprepared:
 * @self: a `BobguiMediaStream`
 *
 * Resets a given media stream implementation.
 *
 * [method@Bobgui.MediaStream.stream_prepared] can then be called again.
 *
 * This function will also reset any error state the stream was in.
 *
 * Since: 4.4
 */
void
bobgui_media_stream_stream_unprepared (BobguiMediaStream *self)
{
  BobguiMediaStreamPrivate *priv = bobgui_media_stream_get_instance_private (self);

  g_return_if_fail (BOBGUI_IS_MEDIA_STREAM (self));
  g_return_if_fail (bobgui_media_stream_is_prepared (self));

  g_object_freeze_notify (G_OBJECT (self));

  bobgui_media_stream_pause (self);

  if (priv->has_audio)
    {
      priv->has_audio = FALSE;
      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_HAS_AUDIO]);
    }
  if (priv->has_video)
    {
      priv->has_video = FALSE;
      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_HAS_VIDEO]);
    }
  if (priv->seekable)
    {
      priv->seekable = FALSE;
      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_SEEKABLE]);
    }
  if (priv->seeking)
    {
      priv->seeking = FALSE;
      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_SEEKING]);
    }
  if (priv->duration != 0)
    {
      priv->duration = 0;
      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_DURATION]);
    }
  if (priv->timestamp != 0)
    {
      priv->timestamp = 0;
      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_TIMESTAMP]);
    }
  if (priv->error)
    {
      g_clear_error (&priv->error);
      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_ERROR]);
    }

  priv->prepared = FALSE;
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_PREPARED]);

  g_object_thaw_notify (G_OBJECT (self));
}

/**
 * bobgui_media_stream_prepared: (skip)
 * @self: a `BobguiMediaStream`
 * @has_audio: %TRUE if the stream should advertise audio support
 * @has_video: %TRUE if the stream should advertise video support
 * @seekable: %TRUE if the stream should advertise seekability
 * @duration: The duration of the stream or 0 if unknown
 *
 * Same as bobgui_media_stream_stream_prepared().
 *
 * Deprecated: 4.4: Use [method@Bobgui.MediaStream.stream_prepared] instead.
 */
void
bobgui_media_stream_prepared (BobguiMediaStream *self,
                           gboolean        has_audio,
                           gboolean        has_video,
                           gboolean        seekable,
                           gint64          duration)
{
  bobgui_media_stream_stream_prepared (self, has_audio, has_video, seekable, duration);
}

/**
 * bobgui_media_stream_unprepared: (skip)
 * @self: a `BobguiMediaStream`
 *
 * Same as bobgui_media_stream_stream_unprepared().
 *
 * Deprecated: 4.4: Use [method@Bobgui.MediaStream.stream_unprepared] instead.
 */
void
bobgui_media_stream_unprepared (BobguiMediaStream *self)
{
  bobgui_media_stream_stream_unprepared (self);
}

/**
 * bobgui_media_stream_gerror:
 * @self: a `BobguiMediaStream`
 * @error: (transfer full): the `GError` to set
 *
 * Sets @self into an error state.
 *
 * This will pause the stream (you can check for an error
 * via [method@Bobgui.MediaStream.get_error] in your
 * BobguiMediaStream.pause() implementation), abort pending
 * seeks and mark the stream as prepared.
 *
 * if the stream is already in an error state, this call
 * will be ignored and the existing error will be retained.
 *
 * To unset an error, the stream must be reset via a call to
 * [method@Bobgui.MediaStream.unprepared].
 */
void
bobgui_media_stream_gerror (BobguiMediaStream *self,
                         GError         *error)
{
  BobguiMediaStreamPrivate *priv = bobgui_media_stream_get_instance_private (self);

  g_return_if_fail (BOBGUI_IS_MEDIA_STREAM (self));
  g_return_if_fail (error != NULL);

  if (priv->error)
    {
      g_error_free (error);
      return;
    }

  g_object_freeze_notify (G_OBJECT (self));

  priv->error = error;

  bobgui_media_stream_pause (self);

  if (!priv->prepared)
    {
      priv->prepared = TRUE;
      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_PREPARED]);
    }
  
  if (priv->seeking)
    bobgui_media_stream_seek_failed (self);
  
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_ERROR]);

  g_object_thaw_notify (G_OBJECT (self));
}

/**
 * bobgui_media_stream_error:
 * @self: a `BobguiMediaStream`
 * @domain: error domain
 * @code: error code
 * @format: printf()-style format for error message
 * @...: parameters for message format
 *
 * Sets @self into an error state using a printf()-style format string.
 *
 * This is a utility function that calls [method@Bobgui.MediaStream.gerror].
 * See that function for details.
 */
void
bobgui_media_stream_error (BobguiMediaStream *self,
                        GQuark          domain,
                        int             code,
                        const char     *format,
                        ...)
{
  GError *error;
  va_list args;

  g_return_if_fail (BOBGUI_IS_MEDIA_STREAM (self));
  g_return_if_fail (domain != 0);
  g_return_if_fail (format != NULL);

  va_start (args, format);
  error = g_error_new_valist (domain, code, format, args);
  va_end (args);

  bobgui_media_stream_gerror (self, error);
}

/**
 * bobgui_media_stream_error_valist:
 * @self: a `BobguiMediaStream`
 * @domain: error domain
 * @code: error code
 * @format: printf()-style format for error message
 * @args: `va_list` of parameters for the message format
 *
 * Sets @self into an error state using a printf()-style format string.
 *
 * This is a utility function that calls [method@Bobgui.MediaStream.gerror].
 * See that function for details.
 */
void
bobgui_media_stream_error_valist (BobguiMediaStream *self,
                               GQuark          domain,
                               int             code,
                               const char     *format,
                               va_list         args)
{
  GError *error;

  g_return_if_fail (BOBGUI_IS_MEDIA_STREAM (self));
  g_return_if_fail (domain != 0);
  g_return_if_fail (format != NULL);

  error = g_error_new_valist (domain, code, format, args);

  bobgui_media_stream_gerror (self, error);
}

/**
 * bobgui_media_stream_update:
 * @self: a `BobguiMediaStream`
 * @timestamp: the new timestamp
 *
 * Media stream implementations should regularly call this
 * function to update the timestamp reported by the stream.
 *
 * It is up to implementations to call this at the frequency
 * they deem appropriate.
 *
 * The media stream must be prepared when this function is called.
 */
void
bobgui_media_stream_update (BobguiMediaStream *self,
                         gint64          timestamp)
{
  BobguiMediaStreamPrivate *priv = bobgui_media_stream_get_instance_private (self);

  g_return_if_fail (BOBGUI_IS_MEDIA_STREAM (self));
  g_return_if_fail (bobgui_media_stream_is_prepared (self));

  g_object_freeze_notify (G_OBJECT (self));

  /* Timestamp before duration is important here.
   * This way the duration notify will be emitted first which will
   * make BobguiMediaControls update adjustment->upper so that the
   * timestamp notify will cause the timestamp to not be clamped.
   */
  if (priv->timestamp != timestamp)
    {
      priv->timestamp = timestamp;
      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_TIMESTAMP]);
    }
  if (priv->duration > 0 && timestamp > priv->duration)
    {
      priv->duration = priv->timestamp;
      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_DURATION]);
    }

  g_object_thaw_notify (G_OBJECT (self));
}

/**
 * bobgui_media_stream_stream_ended:
 * @self: a `BobguiMediaStream`
 *
 * Pauses the media stream and marks it as ended.
 *
 * This is a hint only, calls to [method@Bobgui.MediaStream.play]
 * may still happen.
 *
 * The media stream must be prepared when this function is called.
 *
 * Since: 4.4
 */
void
bobgui_media_stream_stream_ended (BobguiMediaStream *self)
{
  BobguiMediaStreamPrivate *priv = bobgui_media_stream_get_instance_private (self);

  g_return_if_fail (BOBGUI_IS_MEDIA_STREAM (self));
  g_return_if_fail (bobgui_media_stream_is_prepared (self));
  g_return_if_fail (!bobgui_media_stream_get_ended (self));

  g_object_freeze_notify (G_OBJECT (self));

  bobgui_media_stream_pause (self);

  priv->ended = TRUE;
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_ENDED]);

  g_object_thaw_notify (G_OBJECT (self));
}

/**
 * bobgui_media_stream_ended: (skip)
 * @self: a `BobguiMediaStream`
 *
 * Pauses the media stream and marks it as ended.
 *
 * This is a hint only, calls to [method@Bobgui.MediaStream.play]
 * may still happen.
 *
 * The media stream must be prepared when this function is called.
 *
 * Deprecated: 4.4: Use [method@Bobgui.MediaStream.stream_ended] instead
 */
void
bobgui_media_stream_ended (BobguiMediaStream *self)
{
  bobgui_media_stream_stream_ended (self);
}

/**
 * bobgui_media_stream_seek_success:
 * @self: a `BobguiMediaStream`
 *
 * Ends a seek operation started via BobguiMediaStream.seek() successfully.
 *
 * This function will unset the BobguiMediaStream:ended property
 * if it was set.
 *
 * See [method@Bobgui.MediaStream.seek_failed] for the other way of
 * ending a seek.
 */
void
bobgui_media_stream_seek_success (BobguiMediaStream *self)
{
  BobguiMediaStreamPrivate *priv = bobgui_media_stream_get_instance_private (self);

  g_return_if_fail (BOBGUI_IS_MEDIA_STREAM (self));
  g_return_if_fail (bobgui_media_stream_is_seeking (self));

  g_object_freeze_notify (G_OBJECT (self));

  priv->seeking = FALSE;
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_SEEKING]);

  if (priv->ended)
    {
      priv->ended = FALSE;
      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_ENDED]);
    }

  g_object_thaw_notify (G_OBJECT (self));
}

/**
 * bobgui_media_stream_seek_failed:
 * @self: a `BobguiMediaStream`
 *
 * Ends a seek operation started via BobguiMediaStream.seek() as a failure.
 *
 * This will not cause an error on the stream and will assume that
 * playback continues as if no seek had happened.
 *
 * See [method@Bobgui.MediaStream.seek_success] for the other way of
 * ending a seek.
 */
void
bobgui_media_stream_seek_failed (BobguiMediaStream *self)
{
  BobguiMediaStreamPrivate *priv = bobgui_media_stream_get_instance_private (self);

  g_return_if_fail (BOBGUI_IS_MEDIA_STREAM (self));
  g_return_if_fail (bobgui_media_stream_is_seeking (self));

  priv->seeking = FALSE;

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_SEEKING]);
}
