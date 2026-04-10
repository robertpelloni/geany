/* Paintable/Emblems
 *
 * This demo shows how GdkPaintable can be used to
 * overlay an emblem on top of an icon. The emblem
 * can either be another icon, or an arbitrary
 * paintable.
 */

#include <bobgui/bobgui.h>

#include "paintable.h"

static BobguiWidget *window = NULL;

#define DEMO_TYPE_ICON (demo_icon_get_type ())
G_DECLARE_FINAL_TYPE (DemoIcon, demo_icon, DEMO, ICON, GObject)

struct _DemoIcon
{
  GObject parent_instance;

  GdkPaintable *icon;
  GdkPaintable *emblem;
  GdkPaintableFlags flags;
};

struct _DemoIconClass
{
  GObjectClass parent_class;
};

void
demo_icon_snapshot (GdkPaintable *paintable,
                    BobguiSnapshot  *snapshot,
                    double        width,
                    double        height)
{
  DemoIcon *self = DEMO_ICON (paintable);

  gdk_paintable_snapshot (self->icon, snapshot, width, height);
  bobgui_snapshot_save (snapshot);
  bobgui_snapshot_translate (snapshot, &GRAPHENE_POINT_INIT (0.5 * width, 0));
  gdk_paintable_snapshot (self->emblem, snapshot, 0.5 * width, 0.5 * height);
  bobgui_snapshot_restore (snapshot);
}

static GdkPaintableFlags
demo_icon_get_flags (GdkPaintable *paintable)
{
  DemoIcon *self = DEMO_ICON (paintable);

  return self->flags;
}

static void
demo_icon_paintable_init (GdkPaintableInterface *iface)
{
  iface->snapshot = demo_icon_snapshot;
  iface->get_flags = demo_icon_get_flags;
}

G_DEFINE_TYPE_WITH_CODE (DemoIcon, demo_icon, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GDK_TYPE_PAINTABLE,
                                                demo_icon_paintable_init))

static void
demo_icon_dispose (GObject *object)
{
  DemoIcon *self = DEMO_ICON (object);

  g_signal_handlers_disconnect_by_func (self->emblem,
                                        gdk_paintable_invalidate_contents,
                                        self);

  g_clear_object (&self->icon);
  g_clear_object (&self->emblem);

  G_OBJECT_CLASS (demo_icon_parent_class)->dispose (object);
}

static void
demo_icon_class_init (DemoIconClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = demo_icon_dispose;
}

static void
demo_icon_init (DemoIcon *self)
{
  self->flags = GDK_PAINTABLE_STATIC_SIZE | GDK_PAINTABLE_STATIC_CONTENTS;
}

GdkPaintable *
demo_icon_new_with_paintable (const char   *icon_name,
                              GdkPaintable *emblem)
{
  BobguiIconTheme *theme;
  BobguiIconPaintable *icon;
  DemoIcon *self;

  theme = bobgui_icon_theme_get_for_display (gdk_display_get_default ());

  icon = bobgui_icon_theme_lookup_icon (theme,
                                     icon_name, NULL,
                                     128, 1,
                                     BOBGUI_TEXT_DIR_LTR, 0);

  self = g_object_new (DEMO_TYPE_ICON, NULL);

  self->icon = GDK_PAINTABLE (icon);
  self->emblem = emblem;

  if ((gdk_paintable_get_flags (emblem) & GDK_PAINTABLE_STATIC_CONTENTS) == 0)
    {
      self->flags &= ~GDK_PAINTABLE_STATIC_CONTENTS;

      g_signal_connect_swapped (emblem, "invalidate-contents",
                                G_CALLBACK (gdk_paintable_invalidate_contents), self);
    }

  return GDK_PAINTABLE (self);
}

GdkPaintable *
demo_icon_new (const char *icon_name,
               const char *emblem_name)
{
  BobguiIconTheme *theme;
  BobguiIconPaintable *emblem;

  theme = bobgui_icon_theme_get_for_display (gdk_display_get_default ());

  emblem = bobgui_icon_theme_lookup_icon (theme,
                                       emblem_name, NULL,
                                       128, 1,
                                       BOBGUI_TEXT_DIR_LTR, 0);

  return GDK_PAINTABLE (demo_icon_new_with_paintable (icon_name,
                                                      GDK_PAINTABLE (emblem)));
}

BobguiWidget *
do_paintable_emblem (BobguiWidget *do_widget)
{
  GdkPaintable *icon;
  BobguiWidget *grid;
  BobguiWidget *image;

  if (!window)
    {
      window = bobgui_window_new ();
      bobgui_window_set_display (BOBGUI_WINDOW (window),
                              bobgui_widget_get_display (do_widget));
      bobgui_window_set_title (BOBGUI_WINDOW (window), "Paintable — Emblems");
      bobgui_window_set_default_size (BOBGUI_WINDOW (window), 300, 200);
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);

      grid = bobgui_grid_new ();

      icon = demo_icon_new ("folder", "starred");
      image = bobgui_image_new_from_paintable (icon);
      bobgui_image_set_pixel_size (BOBGUI_IMAGE (image), 256);
      bobgui_widget_set_hexpand (image, TRUE);
      bobgui_widget_set_vexpand (image, TRUE);
      bobgui_grid_attach (BOBGUI_GRID (grid), image, 0, 0, 1, 1);

      icon = demo_icon_new_with_paintable ("drive-multidisk",
                                           bobgui_nuclear_animation_new (FALSE));
      image = bobgui_image_new_from_paintable (icon);
      bobgui_image_set_pixel_size (BOBGUI_IMAGE (image), 256);
      bobgui_widget_set_hexpand (image, TRUE);
      bobgui_widget_set_vexpand (image, TRUE);
      bobgui_grid_attach (BOBGUI_GRID (grid), image, 1, 0, 1, 1);

      bobgui_window_set_child (BOBGUI_WINDOW (window), grid);
      g_object_unref (icon);
    }

  if (!bobgui_widget_get_visible (window))
    bobgui_widget_set_visible (window, TRUE);
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));

  return window;
}

