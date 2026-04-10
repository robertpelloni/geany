/* Paintable/Symbolic Paintable
 *
 * GdkPaintables can be made to follow the theme's colors. BOBGUI calls
 * icons that do this symbolic icons, paintables that want to have
 * the same effect can implement the BobguiSymbolicPaintable interface.
 *
 * We will adapt the original paintable example by adding the ability
 * to recolor the paintable based on the symbolic colors.
 */

#include <bobgui/bobgui.h>

#include "paintable.h"

static BobguiWidget *window = NULL;

/* First, add the boilerplate for the object itself.
 * This part would normally go in the header.
 */
#define BOBGUI_TYPE_NUCLEAR_SYMBOLIC (bobgui_nuclear_symbolic_get_type ())
G_DECLARE_FINAL_TYPE (BobguiNuclearSymbolic, bobgui_nuclear_symbolic, BOBGUI, NUCLEAR_SYMBOLIC, GObject)

/* Declare a few warning levels, so we can pick colors based on them */
typedef enum
{
  WARNING_NONE,
  WARNING_ALERT,
  WARNING_EMERGENCY
} WarningLevel;

/* Declare the struct. */
struct _BobguiNuclearSymbolic
{
  GObject parent_instance;

  WarningLevel warning_level;
};

struct _BobguiNuclearSymbolicClass
{
  GObjectClass parent_class;
};

/* Add a function to draw the nuclear icon in the given colors */
static void
bobgui_nuclear_symbolic_snapshot_symbolic (BobguiSymbolicPaintable *paintable,
                                        GdkSnapshot          *snapshot,
                                        double                width,
                                        double                height,
                                        const GdkRGBA        *colors,
                                        gsize                 n_colors)
{
  BobguiNuclearSymbolic *self = BOBGUI_NUCLEAR_SYMBOLIC (paintable);
  static const GdkRGBA transparent = { 0, };
  const GdkRGBA *bg_color;

  /* select the right background color from the warning level */
  switch (self->warning_level)
  {
    case WARNING_NONE:
      bg_color = &transparent;
      break;
    case WARNING_ALERT:
      bg_color = &colors[BOBGUI_SYMBOLIC_COLOR_WARNING];
      break;
    case WARNING_EMERGENCY:
      bg_color = &colors[BOBGUI_SYMBOLIC_COLOR_ERROR];
      break;
    default:
      /* This should never happen, but we better do defensive coding
       * with this critical icon */
      g_assert_not_reached ();
      bg_color = &transparent;
      break;
  }

  /* Draw the icon with the selected warning color */
  bobgui_nuclear_snapshot (snapshot,
                        &colors[BOBGUI_SYMBOLIC_COLOR_FOREGROUND],
                        bg_color,
                        width, height,
                        0);
}

static void
bobgui_nuclear_symbolic_symbolic_paintable_init (BobguiSymbolicPaintableInterface *iface)
{
  iface->snapshot_symbolic = bobgui_nuclear_symbolic_snapshot_symbolic;
}

/* We need to implement the functionality required by the GdkPaintable interface */
static void
bobgui_nuclear_symbolic_snapshot (GdkPaintable *paintable,
                                GdkSnapshot  *snapshot,
                                double        width,
                                double        height)
{
  /* Calling this function without passing a color is a neat trick
   * to make BOBGUI use default colors and otherwise forward the call
   * to the snapshotting function above.
   */
  bobgui_symbolic_paintable_snapshot_symbolic (BOBGUI_SYMBOLIC_PAINTABLE (paintable),
                                            snapshot,
                                            width, height,
                                            NULL, 0);
}

static GdkPaintableFlags
bobgui_nuclear_symbolic_get_flags (GdkPaintable *paintable)
{
  /* This image has a static size, but the contents may change:
   * We draw different things when the warning level changes.
   */
  return GDK_PAINTABLE_STATIC_SIZE;
}

static void
bobgui_nuclear_symbolic_paintable_init (GdkPaintableInterface *iface)
{
  iface->snapshot = bobgui_nuclear_symbolic_snapshot;
  iface->get_flags = bobgui_nuclear_symbolic_get_flags;
}

/* When defining the GType, we need to implement bot the GdkPaintable
 * and the BobguiSymbolicPaintable interface */
G_DEFINE_TYPE_WITH_CODE (BobguiNuclearSymbolic, bobgui_nuclear_symbolic, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GDK_TYPE_PAINTABLE,
                                                bobgui_nuclear_symbolic_paintable_init)
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_SYMBOLIC_PAINTABLE,
                                                bobgui_nuclear_symbolic_symbolic_paintable_init))

static void
bobgui_nuclear_symbolic_class_init (BobguiNuclearSymbolicClass *klass)
{
}

static void
bobgui_nuclear_symbolic_init (BobguiNuclearSymbolic *nuclear)
{
}

/* And finally, we add the simple constructor we declared in the header. */
GdkPaintable *
bobgui_nuclear_symbolic_new (void)
{
  return g_object_new (BOBGUI_TYPE_NUCLEAR_SYMBOLIC, NULL);
}

/* Add some fun feature to the button */
static void
nuclear_button_clicked (BobguiButton          *button,
                        BobguiNuclearSymbolic *nuclear)
{
  if (nuclear->warning_level >= WARNING_EMERGENCY)
    {
      /* On maximum warning level, reset the warning */
      nuclear->warning_level = WARNING_NONE;
      /* And sometimes (but not always to confuse people)
       * close the window.
       */
      if (g_random_boolean ())
        bobgui_window_close (BOBGUI_WINDOW (window));
    }
  else
    {
      /* Otherwise just increase the warning level */
      nuclear->warning_level++;
    }

  /* Don't forget to emit the signal causing the paintable to redraw.
   * Changing the warning level changes the background color after all.
   */
  gdk_paintable_invalidate_contents (GDK_PAINTABLE (nuclear));
}

BobguiWidget *
do_paintable_symbolic (BobguiWidget *do_widget)
{
  GdkPaintable *nuclear;
  BobguiWidget *image, *button;

  if (!window)
    {
      window = bobgui_window_new ();
      bobgui_window_set_display (BOBGUI_WINDOW (window),
                              bobgui_widget_get_display (do_widget));
      bobgui_window_set_title (BOBGUI_WINDOW (window), "Don't click!");
      bobgui_window_set_default_size (BOBGUI_WINDOW (window), 200, 200);
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);

      button = bobgui_button_new ();
      bobgui_window_set_child (BOBGUI_WINDOW (window), button);

      nuclear = bobgui_nuclear_symbolic_new ();
      image = bobgui_image_new_from_paintable (nuclear);
      bobgui_image_set_pixel_size (BOBGUI_IMAGE (image), 256);

      bobgui_button_set_child (BOBGUI_BUTTON (button), image);
      g_signal_connect (button, "clicked", G_CALLBACK (nuclear_button_clicked), nuclear);
      g_object_unref (nuclear);
    }

  if (!bobgui_widget_get_visible (window))
    bobgui_widget_set_visible (window, TRUE);
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));

  return window;
}
