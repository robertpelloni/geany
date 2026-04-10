
#include "config.h"
#include "subsurfaceoverlay.h"
#include "bobguiwidgetprivate.h"
#include "bobguinative.h"
#include "gdksurfaceprivate.h"
#include "gdksubsurfaceprivate.h"
#include "gdkrgbaprivate.h"

struct _BobguiSubsurfaceOverlay
{
  BobguiInspectorOverlay parent_instance;
};

struct _BobguiSubsurfaceOverlayClass
{
  BobguiInspectorOverlayClass parent_class;
};

G_DEFINE_TYPE (BobguiSubsurfaceOverlay, bobgui_subsurface_overlay, BOBGUI_TYPE_INSPECTOR_OVERLAY)

static void
bobgui_subsurface_overlay_snapshot (BobguiInspectorOverlay *overlay,
                                 BobguiSnapshot         *snapshot,
                                 GskRenderNode       *node,
                                 BobguiWidget           *widget)
{
  GdkSurface *surface = bobgui_widget_get_surface (widget);
  double native_x, native_y;

  bobgui_native_get_surface_transform (BOBGUI_NATIVE (widget), &native_x, &native_y);

  bobgui_snapshot_save (snapshot);

  /* Subsurface positions are relative to the surface, so undo the surface
   * transform that bobgui_inspector_prepare_render does.
   */
  bobgui_snapshot_translate (snapshot, &(graphene_point_t) { - native_x, - native_y });

  for (gsize i = 0; i < gdk_surface_get_n_subsurfaces (surface); i++)
    {
      GdkSubsurface *subsurface = gdk_surface_get_subsurface (surface, i);
      graphene_rect_t rect;
      GdkRGBA color;

      if (gdk_subsurface_get_texture (subsurface) == NULL)
        continue;

      gdk_subsurface_get_texture_rect (subsurface, &rect);

      if (gdk_subsurface_is_above_parent (subsurface))
        color = GDK_RGBA ("DAA520"); /* goldenrod */
      else
        color = GDK_RGBA ("FF00FF"); /* magenta */

      /* Use 4 color nodes since a border node overlaps and prevents
       * the subsurface from being raised.
       */
      bobgui_snapshot_append_color (snapshot, &color, &GRAPHENE_RECT_INIT (rect.origin.x - 2, rect.origin.y - 2, 2, rect.size.height + 4));
      bobgui_snapshot_append_color (snapshot, &color, &GRAPHENE_RECT_INIT (rect.origin.x - 2, rect.origin.y - 2, rect.size.width + 4, 2));
      bobgui_snapshot_append_color (snapshot, &color, &GRAPHENE_RECT_INIT (rect.origin.x - 2, rect.origin.y + rect.size.height, rect.size.width + 4, 2));
      bobgui_snapshot_append_color (snapshot, &color, &GRAPHENE_RECT_INIT (rect.origin.x + rect.size.width, rect.origin.y - 2, 2, rect.size.height + 4));
    }

  bobgui_snapshot_restore (snapshot);
}

static void
bobgui_subsurface_overlay_init (BobguiSubsurfaceOverlay *self)
{

}

static void
bobgui_subsurface_overlay_class_init (BobguiSubsurfaceOverlayClass *klass)
{
  BobguiInspectorOverlayClass *overlay_class = (BobguiInspectorOverlayClass *)klass;

  overlay_class->snapshot = bobgui_subsurface_overlay_snapshot;
}

BobguiInspectorOverlay *
bobgui_subsurface_overlay_new (void)
{
  return g_object_new (BOBGUI_TYPE_SUBSURFACE_OVERLAY, NULL);
}
