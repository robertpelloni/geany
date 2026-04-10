/* Paintable/Media Stream
 *
 * GdkPaintable is also used by the BobguiMediaStream class.
 *
 * This demo code turns the nuclear media_stream into the object
 * BOBGUI uses for videos. This allows treating the icon like a
 * regular video, so we can for example attach controls to it.
 *
 * After all, what good is a media_stream if one cannot pause
 * it.
 */

#include <bobgui/bobgui.h>

#include "paintable.h"

static BobguiWidget *window = NULL;

/* First, add the boilerplate for the object itself.
 * This part would normally go in the header.
 */
#define BOBGUI_TYPE_NUCLEAR_MEDIA_STREAM (bobgui_nuclear_media_stream_get_type ())
G_DECLARE_FINAL_TYPE (BobguiNuclearMediaStream, bobgui_nuclear_media_stream, BOBGUI, NUCLEAR_MEDIA_STREAM, BobguiMediaStream)

/* Do a full rotation in 5 seconds.
 *
 * We do not save steps here but real timestamps.
 * BobguiMediaStream uses microseconds, so we will do so, too.
 */
#define DURATION (5 * G_USEC_PER_SEC)

/* Declare the struct. */
struct _BobguiNuclearMediaStream
{
  /* We now inherit from the media stream object. */
  BobguiMediaStream parent_instance;

  /* This variable stores the progress of our video.
   */
  gint64 progress;

  /* This variable stores the timestamp of the last
   * time we updated the progress variable when the
   * video is currently playing.
   * This is so that we can always accurately compute the
   * progress we've had, even if the timeout does not
   * exactly work.
   */
  gint64 last_time;

  /* This variable again holds the ID of the timer that 
   * updates our progress variable. Nothing changes about
   * how this works compared to the previous example.
   */
  guint source_id;
};

struct _BobguiNuclearMediaStreamClass
{
  GObjectClass parent_class;
};

/* BobguiMediaStream is a GdkPaintable. So when we want to display video,
 * we have to implement the interface, just like in the animation example.
 */
static void
bobgui_nuclear_media_stream_snapshot (GdkPaintable *paintable,
                                   GdkSnapshot  *snapshot,
                                   double        width,
                                   double        height)
{
  BobguiNuclearMediaStream *nuclear = BOBGUI_NUCLEAR_MEDIA_STREAM (paintable);

  /* We call the function from the previous example here. */
  bobgui_nuclear_snapshot (snapshot,
                        &(GdkRGBA) { 0, 0, 0, 1 }, /* black */
                        &(GdkRGBA) { 0.9, 0.75, 0.15, 1.0 }, /* yellow */
                        width, height,
                        360 * nuclear->progress / DURATION);
}

static GdkPaintable *
bobgui_nuclear_media_stream_get_current_image (GdkPaintable *paintable)
{
  BobguiNuclearMediaStream *nuclear = BOBGUI_NUCLEAR_MEDIA_STREAM (paintable);

  /* Same thing as with the animation */
  return bobgui_nuclear_icon_new (360 * nuclear->progress / DURATION);
}

static GdkPaintableFlags
bobgui_nuclear_media_stream_get_flags (GdkPaintable *paintable)
{
  /* And same thing as with the animation over here, too. */
  return GDK_PAINTABLE_STATIC_SIZE;
}

static void
bobgui_nuclear_media_stream_paintable_init (GdkPaintableInterface *iface)
{
  iface->snapshot = bobgui_nuclear_media_stream_snapshot;
  iface->get_current_image = bobgui_nuclear_media_stream_get_current_image;
  iface->get_flags = bobgui_nuclear_media_stream_get_flags;
}

/* This time, we inherit from BOBGUI_TYPE_MEDIA_STREAM */
G_DEFINE_TYPE_WITH_CODE (BobguiNuclearMediaStream, bobgui_nuclear_media_stream, BOBGUI_TYPE_MEDIA_STREAM,
                         G_IMPLEMENT_INTERFACE (GDK_TYPE_PAINTABLE,
                                                bobgui_nuclear_media_stream_paintable_init))

static gboolean
bobgui_nuclear_media_stream_step (gpointer data)
{
  BobguiNuclearMediaStream *nuclear = data;
  gint64 current_time;

  /* Compute the time that has elapsed since the last time we were called
   * and add it to our current progress.
   */
  current_time = g_source_get_time (g_main_current_source ());
  nuclear->progress += current_time - nuclear->last_time;

  /* Check if we've ended */
  if (nuclear->progress > DURATION)
    {
      if (bobgui_media_stream_get_loop (BOBGUI_MEDIA_STREAM (nuclear)))
        {
          /* We're looping. So make the progress loop using modulo */
          nuclear->progress %= DURATION;
        }
      else
        {
          /* Just make sure we don't overflow */
          nuclear->progress = DURATION;
        }
    }

  /* Update the last time to the current timestamp. */
  nuclear->last_time = current_time;

  /* Update the timestamp of the media stream */
  bobgui_media_stream_update (BOBGUI_MEDIA_STREAM (nuclear), nuclear->progress);

  /* We also need to invalidate our contents again.
   * After all, we are a video and not just an audio stream.
   */
  gdk_paintable_invalidate_contents (GDK_PAINTABLE (nuclear));

  /* Now check if we have finished playing and if so,
   * tell the media stream. The media stream will then
   * call our pause function to pause the stream.
   */
  if (nuclear->progress >= DURATION)
    bobgui_media_stream_stream_ended (BOBGUI_MEDIA_STREAM (nuclear));

  /* The timeout function is removed by the pause function,
   * so we can just always return this value.
   */
  return G_SOURCE_CONTINUE;
}

static gboolean
bobgui_nuclear_media_stream_play (BobguiMediaStream *stream)
{
  BobguiNuclearMediaStream *nuclear = BOBGUI_NUCLEAR_MEDIA_STREAM (stream);

  /* If we're already at the end of the stream, we don't want
   * to start playing and exit early.
   */
  if (nuclear->progress >= DURATION)
    return FALSE;

  /* This time, we add the source only when we start playing.
   */
  nuclear->source_id = g_timeout_add (10,
                                      bobgui_nuclear_media_stream_step,
                                      nuclear);

  /* We also want to initialize our time, so that we can
   * do accurate updates.
   */
  nuclear->last_time = g_get_monotonic_time ();
  
  /* We successfully started playing, so we return TRUE here. */
  return TRUE;
}

static void
bobgui_nuclear_media_stream_pause (BobguiMediaStream *stream)
{
  BobguiNuclearMediaStream *nuclear = BOBGUI_NUCLEAR_MEDIA_STREAM (stream);

  /* This function will be called when a playing stream
   * gets paused.
   * So we remove the updating source here and set it
   * back to 0 so that the finalize function doesn't try
   * to remove it again.
   */
  g_source_remove (nuclear->source_id);
  nuclear->source_id = 0;
  nuclear->last_time = 0;
}

static void
bobgui_nuclear_media_stream_seek (BobguiMediaStream *stream,
                               gint64          timestamp)
{
  BobguiNuclearMediaStream *nuclear = BOBGUI_NUCLEAR_MEDIA_STREAM (stream);

  /* This is optional functionality for media streams,
   * but not being able to seek is kinda boring.
   * And it's trivial to implement, so let's go for it.
   */
  nuclear->progress = timestamp;

  /* Media streams are asynchronous, so seeking can take a while.
   * We however don't need that functionality, so we can just
   * report success.
   */
  bobgui_media_stream_seek_success (stream);

  /* We also have to update our timestamp and tell the
   * paintable interface about the seek
   */
  bobgui_media_stream_update (stream, nuclear->progress);
  gdk_paintable_invalidate_contents (GDK_PAINTABLE (nuclear));
}

/* Again, we need to implement the finalize function.
 */
static void
bobgui_nuclear_media_stream_finalize (GObject *object)
{
  BobguiNuclearMediaStream *nuclear = BOBGUI_NUCLEAR_MEDIA_STREAM (object);

  /* This time, we need to check if the source exists before
   * removing it as it only exists while we are playing.
   */
  if (nuclear->source_id > 0)
    g_source_remove (nuclear->source_id);

  /* Don't forget to chain up to the parent class' implementation
   * of the finalize function.
   */
  G_OBJECT_CLASS (bobgui_nuclear_media_stream_parent_class)->finalize (object);
}

/* In the class declaration, we need to implement the media stream */
static void
bobgui_nuclear_media_stream_class_init (BobguiNuclearMediaStreamClass *klass)
{
  BobguiMediaStreamClass *stream_class = BOBGUI_MEDIA_STREAM_CLASS (klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  stream_class->play = bobgui_nuclear_media_stream_play;
  stream_class->pause = bobgui_nuclear_media_stream_pause;
  stream_class->seek = bobgui_nuclear_media_stream_seek;

  gobject_class->finalize = bobgui_nuclear_media_stream_finalize;
}

static void
bobgui_nuclear_media_stream_init (BobguiNuclearMediaStream *nuclear)
{
  /* This time, we don't have to add a timer here, because media
   * streams start paused.
   *
   * However, media streams need to tell BOBGUI once they are initialized,
   * so we do that here.
   */
  bobgui_media_stream_stream_prepared (BOBGUI_MEDIA_STREAM (nuclear),
                                    FALSE,
                                    TRUE,
                                    TRUE,
                                    DURATION);
}

/* And finally, we add the simple constructor we declared in the header. */
BobguiMediaStream *
bobgui_nuclear_media_stream_new (void)
{
  return g_object_new (BOBGUI_TYPE_NUCLEAR_MEDIA_STREAM, NULL);
}

BobguiWidget *
do_paintable_mediastream (BobguiWidget *do_widget)
{
  BobguiMediaStream *nuclear;
  BobguiWidget *video;

  if (!window)
    {
      window = bobgui_window_new ();
      bobgui_window_set_display (BOBGUI_WINDOW (window),
                              bobgui_widget_get_display (do_widget));
      bobgui_window_set_title (BOBGUI_WINDOW (window), "Nuclear MediaStream");
      bobgui_window_set_default_size (BOBGUI_WINDOW (window), 300, 200);
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);

      nuclear = bobgui_nuclear_media_stream_new ();
      bobgui_media_stream_set_loop (BOBGUI_MEDIA_STREAM (nuclear), TRUE);

      video = bobgui_video_new_for_media_stream (nuclear);
      bobgui_window_set_child (BOBGUI_WINDOW (window), video);

      g_object_unref (nuclear);
    }

  if (!bobgui_widget_get_visible (window))
    bobgui_widget_set_visible (window, TRUE);
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));

  return window;
}
