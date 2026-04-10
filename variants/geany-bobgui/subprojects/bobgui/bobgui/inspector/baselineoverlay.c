
#include "config.h"
#include "baselineoverlay.h"
#include "bobguiwidgetprivate.h"
#include "bobguicssstyleprivate.h"
#include "bobguicssnodeprivate.h"
#include "bobguicssnumbervalueprivate.h"
#include "bobguicssboxesprivate.h"

struct _BobguiBaselineOverlay
{
  BobguiInspectorOverlay parent_instance;
};

struct _BobguiBaselineOverlayClass
{
  BobguiInspectorOverlayClass parent_class;
};

G_DEFINE_TYPE (BobguiBaselineOverlay, bobgui_baseline_overlay, BOBGUI_TYPE_INSPECTOR_OVERLAY)

static void
recurse_child_widgets (BobguiWidget   *widget,
                       BobguiSnapshot *snapshot)
{
  int baseline;
  BobguiWidget *child;
  BobguiCssBoxes boxes;

  if (!bobgui_widget_get_mapped (widget))
    return;

  if (bobgui_widget_get_overflow (widget) == BOBGUI_OVERFLOW_HIDDEN)
    {
      bobgui_css_boxes_init (&boxes, widget);
      bobgui_snapshot_push_rounded_clip (snapshot, bobgui_css_boxes_get_padding_box (&boxes));
    }

  baseline = bobgui_widget_get_baseline (widget);

  if (baseline != -1)
    {
      GdkRGBA red = {1, 0, 0, 1};
      graphene_rect_t bounds;
      int width;

      width = bobgui_widget_get_width (widget);

      /* Now do all the stuff */
      bobgui_snapshot_push_debug (snapshot, "Widget baseline debugging");

      graphene_rect_init (&bounds,
                          0, baseline,
                          width, 1);
      bobgui_snapshot_append_color (snapshot, &red, &bounds);

      bobgui_snapshot_pop (snapshot);
    }

  /* Recurse into child widgets */
  for (child = bobgui_widget_get_first_child (widget);
       child != NULL;
       child = bobgui_widget_get_next_sibling (child))
    {
      graphene_matrix_t matrix;

      if (bobgui_widget_compute_transform (child, widget, &matrix))
        {
          bobgui_snapshot_save (snapshot);
          bobgui_snapshot_transform_matrix (snapshot, &matrix);
          recurse_child_widgets (child, snapshot);
          bobgui_snapshot_restore (snapshot);
        }
    }

  if (bobgui_widget_get_overflow (widget) == BOBGUI_OVERFLOW_HIDDEN)
    bobgui_snapshot_pop (snapshot);
}

static void
bobgui_baseline_overlay_snapshot (BobguiInspectorOverlay *overlay,
                               BobguiSnapshot         *snapshot,
                               GskRenderNode       *node,
                               BobguiWidget           *widget)
{
  recurse_child_widgets (widget, snapshot);
}

static void
bobgui_baseline_overlay_init (BobguiBaselineOverlay *self)
{
}

static void
bobgui_baseline_overlay_class_init (BobguiBaselineOverlayClass *klass)
{
  BobguiInspectorOverlayClass *overlay_class = (BobguiInspectorOverlayClass *)klass;

  overlay_class->snapshot = bobgui_baseline_overlay_snapshot;
}

BobguiInspectorOverlay *
bobgui_baseline_overlay_new (void)
{
  return g_object_new (BOBGUI_TYPE_BASELINE_OVERLAY, NULL);
}
