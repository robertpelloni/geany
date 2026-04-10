
#include "config.h"
#include "layoutoverlay.h"
#include "bobguiwidgetprivate.h"
#include "bobguicssstyleprivate.h"
#include "bobguicssnodeprivate.h"
#include "bobguicssnumbervalueprivate.h"
#include "bobguinative.h"

struct _BobguiLayoutOverlay
{
  BobguiInspectorOverlay parent_instance;
};

struct _BobguiLayoutOverlayClass
{
  BobguiInspectorOverlayClass parent_class;
};

G_DEFINE_TYPE (BobguiLayoutOverlay, bobgui_layout_overlay, BOBGUI_TYPE_INSPECTOR_OVERLAY)


static int
get_number (BobguiCssValue *value)
{
  double d = bobgui_css_number_value_get (value, 100);

  if (d < 1)
    return ceil (d);
  else
    return floor (d);
}

static void
get_box_margin (BobguiCssStyle *style,
                BobguiBorder   *margin)
{
  margin->top = get_number (style->size->margin_top);
  margin->left = get_number (style->size->margin_left);
  margin->bottom = get_number (style->size->margin_bottom);
  margin->right = get_number (style->size->margin_right);
}

static void
get_box_border (BobguiCssStyle *style,
                BobguiBorder   *border)
{
  border->top = get_number (style->border->border_top_width);
  border->left = get_number (style->border->border_left_width);
  border->bottom = get_number (style->border->border_bottom_width);
  border->right = get_number (style->border->border_right_width);
}

static void
get_box_padding (BobguiCssStyle *style,
                 BobguiBorder   *border)
{
  border->top = get_number (style->size->padding_top);
  border->left = get_number (style->size->padding_left);
  border->bottom = get_number (style->size->padding_bottom);
  border->right = get_number (style->size->padding_right);
}

static void
recurse_child_widgets (BobguiWidget   *widget,
                       BobguiSnapshot *snapshot)
{
  int width = bobgui_widget_get_width (widget);
  int height = bobgui_widget_get_height (widget);
  gboolean needs_clip;
  BobguiCssStyle *style;
  BobguiWidget *child;
  BobguiBorder boxes[4];
  const GdkRGBA colors[4] = {
    {0.7, 0.0, 0.7, 0.6}, /* Padding */
    {0.0, 0.0, 0.0, 0.0}, /* Border */
    {0.7, 0.7, 0.0, 0.6}, /* CSS Margin */
    {0.7, 0.0, 0.0, 0.6}, /* Widget Margin */
  };
  int i;

  if (!bobgui_widget_get_mapped (widget))
    return;

  G_STATIC_ASSERT (G_N_ELEMENTS (boxes) == G_N_ELEMENTS (colors));

  style = bobgui_css_node_get_style (bobgui_widget_get_css_node (widget));
  get_box_padding (style, &boxes[0]);
  get_box_border (style, &boxes[1]);
  get_box_margin (style, &boxes[2]);

  /* TODO: Eh, left = start? RTL? */
  boxes[3].left = bobgui_widget_get_margin_start (widget);
  boxes[3].top = bobgui_widget_get_margin_top (widget);
  boxes[3].right = bobgui_widget_get_margin_end (widget);
  boxes[3].bottom = bobgui_widget_get_margin_bottom (widget);

  /* width/height are the content size and we're going to grow that
   * as we're drawing the boxes, as well as offset the origin.
   * Right now we're at the widget's own origin.
   */
  bobgui_snapshot_save (snapshot);
  bobgui_snapshot_push_debug (snapshot, "Widget layout debugging");

  for (i = 0; i < G_N_ELEMENTS (boxes); i ++)
    {
      const GdkRGBA *color = &colors[i];
      const BobguiBorder *box = &boxes[i];

      if (gdk_rgba_is_clear (color))
        goto next;

      if (box->top > 0)
        bobgui_snapshot_append_color (snapshot, color,
                                   &GRAPHENE_RECT_INIT ( 0, - box->top, width, box->top));
      if (box->right > 0)
        bobgui_snapshot_append_color (snapshot, color,
                                   &GRAPHENE_RECT_INIT (width, 0, box->right, height));
      if (box->bottom > 0)
        bobgui_snapshot_append_color (snapshot, color,
                                   &GRAPHENE_RECT_INIT (0, height, width, box->bottom));
      if (box->left > 0)
        bobgui_snapshot_append_color (snapshot, color,
                                   &GRAPHENE_RECT_INIT (- box->left, 0, box->left, height));

next:
      /* Grow box + offset */
      width += box->left + box->right;
      height += box->top + box->bottom;
      bobgui_snapshot_translate (snapshot, &GRAPHENE_POINT_INIT (- box->left, - box->top));
    }

  bobgui_snapshot_pop (snapshot);


  needs_clip = bobgui_widget_get_overflow (widget) == BOBGUI_OVERFLOW_HIDDEN &&
               bobgui_widget_get_first_child (widget) != NULL;

  if (needs_clip)
    bobgui_snapshot_push_clip (snapshot,
                            &GRAPHENE_RECT_INIT (0, 0, bobgui_widget_get_width (widget), bobgui_widget_get_height (widget)));

  /* Recurse into child widgets */
  for (child = bobgui_widget_get_first_child (widget);
       child != NULL;
       child = bobgui_widget_get_next_sibling (child))
    {
      bobgui_snapshot_save (snapshot);
      bobgui_snapshot_transform (snapshot, child->priv->transform);

      recurse_child_widgets (child, snapshot);

      bobgui_snapshot_restore (snapshot);
    }

  if (needs_clip)
    bobgui_snapshot_pop (snapshot);

  bobgui_snapshot_restore (snapshot);
}

static void
bobgui_layout_overlay_snapshot (BobguiInspectorOverlay *overlay,
                             BobguiSnapshot         *snapshot,
                             GskRenderNode       *node,
                             BobguiWidget           *widget)
{
  recurse_child_widgets (widget, snapshot);
}

static void
bobgui_layout_overlay_init (BobguiLayoutOverlay *self)
{

}

static void
bobgui_layout_overlay_class_init (BobguiLayoutOverlayClass *klass)
{
  BobguiInspectorOverlayClass *overlay_class = (BobguiInspectorOverlayClass *)klass;

  overlay_class->snapshot = bobgui_layout_overlay_snapshot;
}

BobguiInspectorOverlay *
bobgui_layout_overlay_new (void)
{
  return g_object_new (BOBGUI_TYPE_LAYOUT_OVERLAY, NULL);
}
