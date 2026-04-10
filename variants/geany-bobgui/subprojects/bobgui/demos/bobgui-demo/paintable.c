/* Paintable/Simple Paintable
 *
 * GdkPaintable is an interface used by BOBGUI for drawings of any sort
 * that do not require layouting or positioning.
 *
 * This demo code gives a simple example on how a paintable can
 * be created.
 *
 * Paintables can be used in many places inside BOBGUI widgets, but the
 * most common usage is inside BobguiImage and that's what we're going
 * to do here.
 */

#include <bobgui/bobgui.h>

#include "paintable.h"

static BobguiWidget *window = NULL;

/* First, add the boilerplate for the object itself.
 * This part would normally go in the header.
 */
#define BOBGUI_TYPE_NUCLEAR_ICON (bobgui_nuclear_icon_get_type ())
G_DECLARE_FINAL_TYPE (BobguiNuclearIcon, bobgui_nuclear_icon, BOBGUI, NUCLEAR_ICON, GObject)

/* Declare the struct. */
struct _BobguiNuclearIcon
{
  GObject parent_instance;

  /* We store this rotation value here.
   * We are not doing with it here, but it will come in
   * very useful in the followup demos.
   */
  double rotation;
};

struct _BobguiNuclearIconClass
{
  GObjectClass parent_class;
};

/* This is the function that draws the actual icon.
 * We make it a custom function and define it in the paintable.h header
 * so that it can be called from all the other demos, too.
 */
void
bobgui_nuclear_snapshot (BobguiSnapshot   *snapshot,
                      const GdkRGBA *foreground,
                      const GdkRGBA *background,
                      double         width,
                      double         height,
                      double         rotation)
{
#define RADIUS 0.3
  GskPathBuilder *builder;
  GskPath *path;
  GskStroke *stroke;
  double size;

  bobgui_snapshot_append_color (snapshot,
                             background,
                             &GRAPHENE_RECT_INIT (0, 0, width, height));

  size = MIN (width, height);

  bobgui_snapshot_save (snapshot);

  bobgui_snapshot_translate (snapshot, &GRAPHENE_POINT_INIT (width / 2.0, height / 2.0));
  bobgui_snapshot_scale (snapshot, size, size);
  bobgui_snapshot_rotate (snapshot, rotation);

  builder = gsk_path_builder_new ();
  gsk_path_builder_add_circle (builder, graphene_point_zero (), 0.1);
  path = gsk_path_builder_free_to_path (builder);
  bobgui_snapshot_append_fill (snapshot, path, GSK_FILL_RULE_WINDING, foreground);
  gsk_path_unref (path);

  stroke = gsk_stroke_new (RADIUS);
  gsk_stroke_set_dash (stroke, (float[1]) { RADIUS * G_PI / 3 }, 1);
  builder = gsk_path_builder_new ();
  gsk_path_builder_add_circle (builder, graphene_point_zero(), RADIUS);
  path = gsk_path_builder_free_to_path (builder);
  bobgui_snapshot_append_stroke (snapshot, path, stroke, foreground);
  gsk_path_unref (path);
  gsk_stroke_free (stroke);

  bobgui_snapshot_restore (snapshot);
}

/* Here, we implement the functionality required by the GdkPaintable interface */
static void
bobgui_nuclear_icon_snapshot (GdkPaintable *paintable,
                           GdkSnapshot  *snapshot,
                           double        width,
                           double        height)
{
  BobguiNuclearIcon *nuclear = BOBGUI_NUCLEAR_ICON (paintable);

  /* The snapshot function is the only function we need to implement.
   * It does the actual drawing of the paintable.
   */

  bobgui_nuclear_snapshot (snapshot,
                        &(GdkRGBA) { 0, 0, 0, 1 }, /* black */
                        &(GdkRGBA) { 0.9, 0.75, 0.15, 1.0 }, /* yellow */
                        width, height,
                        nuclear->rotation);
}

static GdkPaintableFlags
bobgui_nuclear_icon_get_flags (GdkPaintable *paintable)
{
  /* The flags are very useful to let BOBGUI know that this image
   * is never going to change.
   * This allows many optimizations and should therefore always
   * be set.
   */
  return GDK_PAINTABLE_STATIC_CONTENTS | GDK_PAINTABLE_STATIC_SIZE;
}

static void
bobgui_nuclear_icon_paintable_init (GdkPaintableInterface *iface)
{
  iface->snapshot = bobgui_nuclear_icon_snapshot;
  iface->get_flags = bobgui_nuclear_icon_get_flags;
}

/* When defining the GType, we need to implement the GdkPaintable interface */
G_DEFINE_TYPE_WITH_CODE (BobguiNuclearIcon, bobgui_nuclear_icon, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GDK_TYPE_PAINTABLE,
                                                bobgui_nuclear_icon_paintable_init))

/* Here's the boilerplate for the GObject declaration.
 * We don't need to do anything special here, because we keep no
 * data of our own.
 */
static void
bobgui_nuclear_icon_class_init (BobguiNuclearIconClass *klass)
{
}

static void
bobgui_nuclear_icon_init (BobguiNuclearIcon *nuclear)
{
}

/* And finally, we add a simple constructor.
 * It is declared in the header so that the other examples
 * can use it.
 */
GdkPaintable *
bobgui_nuclear_icon_new (double rotation)
{
  BobguiNuclearIcon *nuclear;

  nuclear = g_object_new (BOBGUI_TYPE_NUCLEAR_ICON, NULL);
  nuclear->rotation = rotation;

  return GDK_PAINTABLE (nuclear);
}

BobguiWidget *
do_paintable (BobguiWidget *do_widget)
{
  GdkPaintable *nuclear;
  BobguiWidget *image;

  if (!window)
    {
      window = bobgui_window_new ();
      bobgui_window_set_display (BOBGUI_WINDOW (window),
                              bobgui_widget_get_display (do_widget));
      bobgui_window_set_title (BOBGUI_WINDOW (window), "Nuclear Icon");
      bobgui_window_set_default_size (BOBGUI_WINDOW (window), 300, 200);
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);

      nuclear = bobgui_nuclear_icon_new (0.0);
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
