/* Paintable/Animated Paintable
 *
 * GdkPaintable also allows paintables to change.
 *
 * This demo code gives an example of how this could work. It builds
 * on the previous simple example.
 *
 * Paintables can also change their size, this works similarly, but
 * we will not demonstrate this here as our icon does not have any size.
 */

#include <bobgui/bobgui.h>

#include "paintable.h"

static BobguiWidget *window = NULL;

/* First, add the boilerplate for the object itself.
 * This part would normally go in the header.
 */
#define BOBGUI_TYPE_NUCLEAR_ANIMATION (bobgui_nuclear_animation_get_type ())
G_DECLARE_FINAL_TYPE (BobguiNuclearAnimation, bobgui_nuclear_animation, BOBGUI, NUCLEAR_ANIMATION, GObject)

/* Do a full rotation in 5 seconds.
 * We will register the timeout for doing a single step to
 * be executed every 10ms, which means after 1000 steps
 * 10s will have elapsed.
 */
#define MAX_PROGRESS 500

/* Declare the struct. */
struct _BobguiNuclearAnimation
{
  GObject parent_instance;

  gboolean draw_background;

  /* This variable stores the progress of our animation.
   * We just count upwards until we hit MAX_PROGRESS and
   * then start from scratch.
   */
  int progress;

  /* This variable holds the ID of the timer that updates
   * our progress variable.
   * We need to keep track of it so that we can remove it
   * again.
   */
  guint source_id;
};

struct _BobguiNuclearAnimationClass
{
  GObjectClass parent_class;
};

/* Again, we implement the functionality required by the GdkPaintable interface */
static void
bobgui_nuclear_animation_snapshot (GdkPaintable *paintable,
                                GdkSnapshot  *snapshot,
                                double        width,
                                double        height)
{
  BobguiNuclearAnimation *nuclear = BOBGUI_NUCLEAR_ANIMATION (paintable);

  /* We call the function from the previous example here. */
  bobgui_nuclear_snapshot (snapshot,
                        &(GdkRGBA) { 0, 0, 0, 1 }, /* black */
                        nuclear->draw_background
                          ? &(GdkRGBA) { 0.9, 0.75, 0.15, 1.0 } /* yellow */
                          : &(GdkRGBA) { 0, 0, 0, 0 }, /* transparent */
                        width, height,
                        360 * nuclear->progress / MAX_PROGRESS);
}

static GdkPaintable *
bobgui_nuclear_animation_get_current_image (GdkPaintable *paintable)
{
  BobguiNuclearAnimation *nuclear = BOBGUI_NUCLEAR_ANIMATION (paintable);

  /* For non-static paintables, this function needs to be implemented.
   * It must return a static paintable with the same contents
   * as this one currently has.
   *
   * Luckily we added the rotation property to the nuclear icon
   * object previously, so we can just return an instance of that one.
   */
  return bobgui_nuclear_icon_new (360 * nuclear->progress / MAX_PROGRESS);
}

static GdkPaintableFlags
bobgui_nuclear_animation_get_flags (GdkPaintable *paintable)
{
  /* This time, we cannot set the static contents flag because our animation
   * changes the contents.
   * However, our size still doesn't change, so report that flag.
   */
  return GDK_PAINTABLE_STATIC_SIZE;
}

static void
bobgui_nuclear_animation_paintable_init (GdkPaintableInterface *iface)
{
  iface->snapshot = bobgui_nuclear_animation_snapshot;
  iface->get_current_image = bobgui_nuclear_animation_get_current_image;
  iface->get_flags = bobgui_nuclear_animation_get_flags;
}

/* When defining the GType, we need to implement the GdkPaintable interface */
G_DEFINE_TYPE_WITH_CODE (BobguiNuclearAnimation, bobgui_nuclear_animation, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GDK_TYPE_PAINTABLE,
                                                bobgui_nuclear_animation_paintable_init))

/* This time, we need to implement the finalize function,
 */
static void
bobgui_nuclear_animation_finalize (GObject *object)
{
  BobguiNuclearAnimation *nuclear = BOBGUI_NUCLEAR_ANIMATION (object);

  /* Remove the timeout we registered when constructing
   * the object. 
   */
  g_source_remove (nuclear->source_id);

  /* Don't forget to chain up to the parent class' implementation
   * of the finalize function.
   */
  G_OBJECT_CLASS (bobgui_nuclear_animation_parent_class)->finalize (object);
}

/* In the class declaration, we need to add our finalize function.
 */
static void
bobgui_nuclear_animation_class_init (BobguiNuclearAnimationClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->finalize = bobgui_nuclear_animation_finalize;
}

static gboolean
bobgui_nuclear_animation_step (gpointer data)
{
  BobguiNuclearAnimation *nuclear = data;

  /* Add 1 to the progress and reset it when we've reached
   * the maximum value.
   * The animation will rotate by 360 degrees at MAX_PROGRESS
   * so it will be identical to the original unrotated one.
   */
  nuclear->progress = (nuclear->progress + 1) % MAX_PROGRESS;

  /* Now we need to tell all listeners that we've changed out contents
   * so that they can redraw this paintable.
   */
  gdk_paintable_invalidate_contents (GDK_PAINTABLE (nuclear));

  /* We want this timeout function to be called repeatedly,
   * so we return this value here.
   * If this was a single-shot timeout, we could also
   * return G_SOURCE_REMOVE here to get rid of it.
   */
  return G_SOURCE_CONTINUE;
}

static void
bobgui_nuclear_animation_init (BobguiNuclearAnimation *nuclear)
{
  /* Add a timer here that constantly updates our animations.
   * We want to update it often enough to guarantee a smooth animation.
   *
   * Ideally, we'd attach to the frame clock, but because we do
   * not have it available here, we just use a regular timeout
   * that hopefully triggers often enough to be smooth.
   */
  nuclear->source_id = g_timeout_add (10,
                                      bobgui_nuclear_animation_step,
                                      nuclear);
}

/* And finally, we add the simple constructor we declared in the header. */
GdkPaintable *
bobgui_nuclear_animation_new (gboolean draw_background)
{
  BobguiNuclearAnimation *nuclear;

  nuclear = g_object_new (BOBGUI_TYPE_NUCLEAR_ANIMATION, NULL);

  nuclear->draw_background = draw_background;

  return GDK_PAINTABLE (nuclear);
}

BobguiWidget *
do_paintable_animated (BobguiWidget *do_widget)
{
  GdkPaintable *nuclear;
  BobguiWidget *image;

  if (!window)
    {
      window = bobgui_window_new ();
      bobgui_window_set_display (BOBGUI_WINDOW (window),
                              bobgui_widget_get_display (do_widget));
      bobgui_window_set_title (BOBGUI_WINDOW (window), "Nuclear Animation");
      bobgui_window_set_default_size (BOBGUI_WINDOW (window), 300, 200);
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);

      nuclear = bobgui_nuclear_animation_new (TRUE);
      image = bobgui_image_new_from_paintable (nuclear);
      bobgui_image_set_pixel_size (BOBGUI_IMAGE (image), 256);
      bobgui_window_set_child (BOBGUI_WINDOW (window), image);
      g_object_unref (nuclear);
    }

  if (!bobgui_widget_get_visible (window))
    bobgui_widget_set_visible (window, TRUE);
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));

  return window;
}
