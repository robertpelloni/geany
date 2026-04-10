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

#include "bobguimediacontrols.h"

#include "bobguiadjustment.h"
#include "bobguibutton.h"
#include <glib/gi18n-lib.h>
#include "bobguilabel.h"
#include "bobguiwidgetprivate.h"
#include "bobguiprivate.h"

/**
 * BobguiMediaControls:
 *
 * Shows controls for video playback.
 *
 * <picture>
 *   <source srcset="media-controls-dark.png" media="(prefers-color-scheme: dark)">
 *   <img alt="An example BobguiMediaControls" src="media-controls.png">
 * </picture>
 *
 * Usually, `BobguiMediaControls` is used as part of [class@Bobgui.Video].
 */

struct _BobguiMediaControls
{
  BobguiWidget parent_instance;

  BobguiMediaStream *stream;

  BobguiAdjustment *time_adjustment;
  BobguiAdjustment *volume_adjustment;
  BobguiWidget *box;
  BobguiWidget *play_button;
  BobguiWidget *time_box;
  BobguiWidget *time_label;
  BobguiWidget *seek_scale;
  BobguiWidget *duration_label;
  BobguiWidget *volume_button;
};

enum
{
  PROP_0,
  PROP_MEDIA_STREAM,

  N_PROPS
};

G_DEFINE_TYPE (BobguiMediaControls, bobgui_media_controls, BOBGUI_TYPE_WIDGET)

static GParamSpec *properties[N_PROPS] = { NULL, };

/* FIXME: Remove
 * See https://bugzilla.gnome.org/show_bug.cgi?id=679850 */
static char *
totem_time_to_string (gint64   usecs,
		      gboolean remaining,
		      gboolean force_hour)
{
	int sec, min, hour, _time;

	_time = (int) (usecs / G_USEC_PER_SEC);
	/* When calculating the remaining time,
	 * we want to make sure that:
	 * current time + time remaining = total run time */
	if (remaining)
		_time++;

	sec = _time % 60;
	_time = _time - sec;
	min = (_time % (60*60)) / 60;
	_time = _time - (min * 60);
	hour = _time / (60*60);

	if (hour > 0 || force_hour) {
		if (!remaining) {
			/* hour:minutes:seconds */
			/* Translators: This is a time format, like "9:05:02" for 9
			 * hours, 5 minutes, and 2 seconds. You may change ":" to
			 * the separator that your locale uses or use "%Id" instead
			 * of "%d" if your locale uses localized digits.
			 */
			return g_strdup_printf (C_("long time format", "%d:%02d:%02d"), hour, min, sec);
		} else {
			/* -hour:minutes:seconds */
			/* Translators: This is a time format, like "-9:05:02" for 9
			 * hours, 5 minutes, and 2 seconds playback remaining. You may
			 * change ":" to the separator that your locale uses or use
			 * "%Id" instead of "%d" if your locale uses localized digits.
			 */
			return g_strdup_printf (C_("long time format", "-%d:%02d:%02d"), hour, min, sec);
		}
	}

	if (remaining) {
		/* -minutes:seconds */
		/* Translators: This is a time format, like "-5:02" for 5
		 * minutes and 2 seconds playback remaining. You may change
		 * ":" to the separator that your locale uses or use "%Id"
		 * instead of "%d" if your locale uses localized digits.
		 */
		return g_strdup_printf (C_("short time format", "-%d:%02d"), min, sec);
	}

	/* minutes:seconds */
	/* Translators: This is a time format, like "5:02" for 5
	 * minutes and 2 seconds. You may change ":" to the
	 * separator that your locale uses or use "%Id" instead of
	 * "%d" if your locale uses localized digits.
	 */
	return g_strdup_printf (C_("short time format", "%d:%02d"), min, sec);
}

static void
time_adjustment_changed (BobguiAdjustment    *adjustment,
                         BobguiMediaControls *controls)
{
  if (controls->stream == NULL)
    return;

  /* We just updated the adjustment and it's correct now */
  if (bobgui_adjustment_get_value (adjustment) == (double) bobgui_media_stream_get_timestamp (controls->stream) / G_USEC_PER_SEC)
    return;

  bobgui_media_stream_seek (controls->stream,
                         bobgui_adjustment_get_value (adjustment) * G_USEC_PER_SEC + 0.5);
}

static void
volume_adjustment_changed (BobguiAdjustment    *adjustment,
                           BobguiMediaControls *controls)
{
  if (controls->stream == NULL)
    return;

  /* We just updated the adjustment and it's correct now */
  if (bobgui_adjustment_get_value (adjustment) == bobgui_media_stream_get_volume (controls->stream))
    return;

  bobgui_media_stream_set_muted (controls->stream, bobgui_adjustment_get_value (adjustment) == 0.0);
  bobgui_media_stream_set_volume (controls->stream, bobgui_adjustment_get_value (adjustment));
}

static void
play_button_clicked (BobguiWidget        *button,
                     BobguiMediaControls *controls)
{
  if (controls->stream == NULL)
    return;

  bobgui_media_stream_set_playing (controls->stream,
                                !bobgui_media_stream_get_playing (controls->stream));
}

static void
bobgui_media_controls_measure (BobguiWidget      *widget,
                            BobguiOrientation  orientation,
                            int             for_size,
                            int            *minimum,
                            int            *natural,
                            int            *minimum_baseline,
                            int            *natural_baseline)
{
  BobguiMediaControls *controls = BOBGUI_MEDIA_CONTROLS (widget);

  bobgui_widget_measure (controls->box,
                      orientation,
                      for_size,
                      minimum, natural,
                      minimum_baseline, natural_baseline);
}

static void
bobgui_media_controls_size_allocate (BobguiWidget *widget,
                                  int        width,
                                  int        height,
                                  int        baseline)
{
  BobguiMediaControls *controls = BOBGUI_MEDIA_CONTROLS (widget);

  bobgui_widget_size_allocate (controls->box,
                            &(BobguiAllocation) {
                              0, 0,
                              width, height
                            }, baseline);
}

static void
bobgui_media_controls_dispose (GObject *object)
{
  BobguiMediaControls *controls = BOBGUI_MEDIA_CONTROLS (object);

  bobgui_media_controls_set_media_stream (controls, NULL);

  bobgui_widget_dispose_template (BOBGUI_WIDGET (object), BOBGUI_TYPE_MEDIA_CONTROLS);

  G_OBJECT_CLASS (bobgui_media_controls_parent_class)->dispose (object);
}

static void
bobgui_media_controls_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  BobguiMediaControls *controls = BOBGUI_MEDIA_CONTROLS (object);

  switch (property_id)
    {
    case PROP_MEDIA_STREAM:
      g_value_set_object (value, controls->stream);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
bobgui_media_controls_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  BobguiMediaControls *controls = BOBGUI_MEDIA_CONTROLS (object);

  switch (property_id)
    {
    case PROP_MEDIA_STREAM:
      bobgui_media_controls_set_media_stream (controls, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
bobgui_media_controls_class_init (BobguiMediaControlsClass *klass)
{
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  widget_class->measure = bobgui_media_controls_measure;
  widget_class->size_allocate = bobgui_media_controls_size_allocate;

  gobject_class->dispose = bobgui_media_controls_dispose;
  gobject_class->get_property = bobgui_media_controls_get_property;
  gobject_class->set_property = bobgui_media_controls_set_property;

  /**
   * BobguiMediaControls:media-stream:
   *
   * The media-stream managed by this object or %NULL if none.
   */
  properties[PROP_MEDIA_STREAM] =
    g_param_spec_object ("media-stream", NULL, NULL,
                         BOBGUI_TYPE_MEDIA_STREAM,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (gobject_class, N_PROPS, properties);

  bobgui_widget_class_set_template_from_resource (widget_class, "/org/bobgui/libbobgui/ui/bobguimediacontrols.ui");
  bobgui_widget_class_bind_template_child (widget_class, BobguiMediaControls, time_adjustment);
  bobgui_widget_class_bind_template_child (widget_class, BobguiMediaControls, volume_adjustment);
  bobgui_widget_class_bind_template_child (widget_class, BobguiMediaControls, box);
  bobgui_widget_class_bind_template_child (widget_class, BobguiMediaControls, play_button);
  bobgui_widget_class_bind_template_child (widget_class, BobguiMediaControls, time_box);
  bobgui_widget_class_bind_template_child (widget_class, BobguiMediaControls, time_label);
  bobgui_widget_class_bind_template_child (widget_class, BobguiMediaControls, seek_scale);
  bobgui_widget_class_bind_template_child (widget_class, BobguiMediaControls, duration_label);
  bobgui_widget_class_bind_template_child (widget_class, BobguiMediaControls, volume_button);

  bobgui_widget_class_bind_template_callback (widget_class, play_button_clicked);
  bobgui_widget_class_bind_template_callback (widget_class, time_adjustment_changed);
  bobgui_widget_class_bind_template_callback (widget_class, volume_adjustment_changed);

  bobgui_widget_class_set_css_name (widget_class, I_("controls"));
}

static void
bobgui_media_controls_init (BobguiMediaControls *controls)
{
  bobgui_widget_init_template (BOBGUI_WIDGET (controls));
}

/**
 * bobgui_media_controls_new:
 * @stream: (nullable) (transfer none): a `BobguiMediaStream` to manage
 *
 * Creates a new `BobguiMediaControls` managing the @stream passed to it.
 *
 * Returns: a new `BobguiMediaControls`
 */
BobguiWidget *
bobgui_media_controls_new (BobguiMediaStream *stream)
{
  return g_object_new (BOBGUI_TYPE_MEDIA_CONTROLS,
                       "media-stream", stream,
                       NULL);
}

/**
 * bobgui_media_controls_get_media_stream:
 * @controls: a `BobguiMediaControls`
 *
 * Gets the media stream managed by @controls or %NULL if none.
 *
 * Returns: (nullable) (transfer none): The media stream managed by @controls
 */
BobguiMediaStream *
bobgui_media_controls_get_media_stream (BobguiMediaControls *controls)
{
  g_return_val_if_fail (BOBGUI_IS_MEDIA_CONTROLS (controls), NULL);

  return controls->stream;
}

static void
update_timestamp (BobguiMediaControls *controls)
{
  gint64 timestamp, duration;
  char *time_string;

  if (controls->stream)
    {
      timestamp = bobgui_media_stream_get_timestamp (controls->stream);
      duration = bobgui_media_stream_get_duration (controls->stream);
    }
  else
    {
      timestamp = 0;
      duration = 0;
    }

  time_string = totem_time_to_string (timestamp, FALSE, FALSE);
  bobgui_label_set_text (BOBGUI_LABEL (controls->time_label), time_string);
  g_free (time_string);

  if (duration > 0)
    {
      time_string = totem_time_to_string (duration > timestamp ? duration - timestamp : 0, TRUE, FALSE);
      bobgui_label_set_text (BOBGUI_LABEL (controls->duration_label), time_string);
      g_free (time_string);

      bobgui_adjustment_set_value (controls->time_adjustment, (double) timestamp / G_USEC_PER_SEC);
    }
}

static void
update_duration (BobguiMediaControls *controls)
{
  gint64 timestamp, duration;
  char *time_string;

  if (controls->stream)
    {
      timestamp = bobgui_media_stream_get_timestamp (controls->stream);
      duration = bobgui_media_stream_get_duration (controls->stream);
    }
  else
    {
      timestamp = 0;
      duration = 0;
    }

  time_string = totem_time_to_string (duration > timestamp ? duration - timestamp : 0, TRUE, FALSE);
  bobgui_label_set_text (BOBGUI_LABEL (controls->duration_label), time_string);
  bobgui_widget_set_visible (controls->duration_label, duration > 0);
  g_free (time_string);

  bobgui_adjustment_set_upper (controls->time_adjustment,
                            bobgui_adjustment_get_page_size (controls->time_adjustment)
                            + (double) duration / G_USEC_PER_SEC);
  bobgui_adjustment_set_value (controls->time_adjustment, (double) timestamp / G_USEC_PER_SEC);
}

static void
update_playing (BobguiMediaControls *controls)
{
  gboolean playing;
  const char *icon_name;
  const char *tooltip_text;

  if (controls->stream)
    playing = bobgui_media_stream_get_playing (controls->stream);
  else
    playing = FALSE;

  if (playing)
    {
      icon_name = "media-playback-pause-symbolic";
      tooltip_text = C_("media controls tooltip", "Stop");
    }
  else
    {
      icon_name = "media-playback-start-symbolic";
      tooltip_text = C_("media controls tooltip", "Play");
    }

  bobgui_button_set_icon_name (BOBGUI_BUTTON (controls->play_button), icon_name);
  bobgui_widget_set_tooltip_text (controls->play_button, tooltip_text);
}

static void
update_seekable (BobguiMediaControls *controls)
{
  gboolean seekable;

  if (controls->stream)
    seekable = bobgui_media_stream_is_seekable (controls->stream);
  else
    seekable = FALSE;

  bobgui_widget_set_sensitive (controls->seek_scale, seekable);
}

static void
update_volume (BobguiMediaControls *controls)
{
  double volume;

  if (controls->stream == NULL)
    volume = 1.0;
  else if (bobgui_media_stream_get_muted (controls->stream))
    volume = 0.0;
  else
    volume = bobgui_media_stream_get_volume (controls->stream);

  bobgui_adjustment_set_value (controls->volume_adjustment, volume);

  bobgui_widget_set_sensitive (controls->volume_button,
                            controls->stream == NULL ||
                            bobgui_media_stream_has_audio (controls->stream));
}

static void
update_all (BobguiMediaControls *controls)
{
  update_timestamp (controls);
  update_duration (controls);
  update_playing (controls);
  update_seekable (controls);
  update_volume (controls);
}

static void
bobgui_media_controls_notify_cb (BobguiMediaStream   *stream,
                              GParamSpec       *pspec,
                              BobguiMediaControls *controls)
{
  if (g_str_equal (pspec->name, "timestamp"))
    update_timestamp (controls);
  else if (g_str_equal (pspec->name, "duration"))
    update_duration (controls);
  else if (g_str_equal (pspec->name, "playing"))
    update_playing (controls);
  else if (g_str_equal (pspec->name, "seekable"))
    update_seekable (controls);
  else if (g_str_equal (pspec->name, "muted"))
    update_volume (controls);
  else if (g_str_equal (pspec->name, "volume"))
    update_volume (controls);
  else if (g_str_equal (pspec->name, "has-audio"))
    update_volume (controls);
}

/**
 * bobgui_media_controls_set_media_stream:
 * @controls: a `BobguiMediaControls` widget
 * @stream: (nullable):  a `BobguiMediaStream`
 *
 * Sets the stream that is controlled by @controls.
 */
void
bobgui_media_controls_set_media_stream (BobguiMediaControls *controls,
                                     BobguiMediaStream   *stream)
{
  g_return_if_fail (BOBGUI_IS_MEDIA_CONTROLS (controls));
  g_return_if_fail (stream == NULL || BOBGUI_IS_MEDIA_STREAM (stream));

  if (controls->stream == stream)
    return;

  if (controls->stream)
    {
      g_signal_handlers_disconnect_by_func (controls->stream,
                                            bobgui_media_controls_notify_cb,
                                            controls);
      g_object_unref (controls->stream);
      controls->stream = NULL;
    }

  if (stream)
    {
      controls->stream = g_object_ref (stream);
      g_signal_connect (controls->stream,
                        "notify",
                        G_CALLBACK (bobgui_media_controls_notify_cb),
                        controls);
    }

  update_all (controls);
  bobgui_widget_set_sensitive (controls->box, stream != NULL);

  g_object_notify_by_pspec (G_OBJECT (controls), properties[PROP_MEDIA_STREAM]);
}
