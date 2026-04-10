/* BOBGUI - The Bobgui Framework
 * Copyright © 2013 Carlos Garnacho <carlosg@gnome.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"
#include "bobgui/bobgui.h"
#include "bobguimagnifierprivate.h"
#include "bobguiwidgetprivate.h"
#include "bobguisnapshot.h"
#include "bobguicssboxesprivate.h"

enum {
  PROP_INSPECTED = 1,
  PROP_RESIZE,
  PROP_MAGNIFICATION
};

struct _BobguiMagnifier
{
  BobguiWidget parent_instance;

  GdkPaintable *paintable;
  double magnification;
  int x;
  int y;
  gboolean resize;
};

typedef struct
{
  BobguiWidgetClass parent_class;
} BobguiMagnifierClass;

G_DEFINE_TYPE (BobguiMagnifier, bobgui_magnifier, BOBGUI_TYPE_WIDGET)

static void
_bobgui_magnifier_set_property (GObject      *object,
                             guint         param_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  BobguiMagnifier *magnifier = BOBGUI_MAGNIFIER (object);

  switch (param_id)
    {
    case PROP_INSPECTED:
      _bobgui_magnifier_set_inspected (magnifier, g_value_get_object (value));
      break;

    case PROP_MAGNIFICATION:
      _bobgui_magnifier_set_magnification (magnifier, g_value_get_double (value));
      break;

    case PROP_RESIZE:
      _bobgui_magnifier_set_resize (magnifier, g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
    }
}

static void
_bobgui_magnifier_get_property (GObject    *object,
                             guint       param_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  BobguiMagnifier *magnifier = BOBGUI_MAGNIFIER (object);

  switch (param_id)
    {
    case PROP_INSPECTED:
      g_value_set_object (value, bobgui_widget_paintable_get_widget (BOBGUI_WIDGET_PAINTABLE (magnifier->paintable)));
      break;

    case PROP_MAGNIFICATION:
      g_value_set_double (value, magnifier->magnification);
      break;

    case PROP_RESIZE:
      g_value_set_boolean (value, magnifier->resize);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
    }
}

static void
bobgui_magnifier_snapshot (BobguiWidget   *widget,
                        BobguiSnapshot *snapshot)
{
  BobguiMagnifier *magnifier = BOBGUI_MAGNIFIER (widget);
  double width, height, paintable_width, paintable_height;
  BobguiWidget *inspected;
  BobguiCssBoxes boxes;
  const graphene_rect_t *content_rect;
  const graphene_rect_t *border_rect;
  graphene_point_t offset;

  inspected = bobgui_widget_paintable_get_widget (BOBGUI_WIDGET_PAINTABLE (magnifier->paintable));
  if (inspected == NULL)
    return;

  bobgui_css_boxes_init (&boxes, inspected);
  content_rect = bobgui_css_boxes_get_content_rect (&boxes);
  border_rect = bobgui_css_boxes_get_border_rect (&boxes);

  offset.x = content_rect->origin.x - border_rect->origin.x;
  offset.y = content_rect->origin.y - border_rect->origin.y;

  width = bobgui_widget_get_width (widget);
  height = bobgui_widget_get_height (widget);
  paintable_width = gdk_paintable_get_intrinsic_width (magnifier->paintable);
  paintable_height = gdk_paintable_get_intrinsic_height (magnifier->paintable);
  if (paintable_width <= 0.0 || paintable_height <= 0.0)
    return;

  bobgui_snapshot_save (snapshot);
  if (!magnifier->resize)
    bobgui_snapshot_translate (snapshot, &GRAPHENE_POINT_INIT (width / 2, height / 2));
  bobgui_snapshot_scale (snapshot, magnifier->magnification, magnifier->magnification);
  bobgui_snapshot_translate (snapshot, &GRAPHENE_POINT_INIT (
                          - CLAMP (magnifier->x + offset.x, 0, paintable_width),
                          - CLAMP (magnifier->y + offset.y, 0, paintable_height)));

  gdk_paintable_snapshot (magnifier->paintable, snapshot, paintable_width, paintable_height);
  bobgui_snapshot_restore (snapshot);
}

static void
bobgui_magnifier_measure (BobguiWidget      *widget,
                       BobguiOrientation  orientation,
                       int             for_size,
                       int            *minimum,
                       int            *natural,
                       int            *minimum_baseline,
                       int            *natural_baseline)
{
  BobguiMagnifier *magnifier = BOBGUI_MAGNIFIER (widget);
  int size;

  if (magnifier->resize)
    {
      if (orientation == BOBGUI_ORIENTATION_HORIZONTAL)
        size = magnifier->magnification * gdk_paintable_get_intrinsic_width (magnifier->paintable);
      else
        size = magnifier->magnification * gdk_paintable_get_intrinsic_height (magnifier->paintable);
    }
  else
    size = 0;

  *minimum = size;
  *natural = size;
}

static void
bobgui_magnifier_dispose (GObject *object)
{
  BobguiMagnifier *magnifier = BOBGUI_MAGNIFIER (object);

  if (magnifier->paintable)
    _bobgui_magnifier_set_inspected (magnifier, NULL);

  g_clear_object (&magnifier->paintable);

  G_OBJECT_CLASS (bobgui_magnifier_parent_class)->dispose (object);
}

static void
bobgui_magnifier_class_init (BobguiMagnifierClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);

  object_class->set_property = _bobgui_magnifier_set_property;
  object_class->get_property = _bobgui_magnifier_get_property;
  object_class->dispose = bobgui_magnifier_dispose;

  widget_class->snapshot = bobgui_magnifier_snapshot;
  widget_class->measure = bobgui_magnifier_measure;

  g_object_class_install_property (object_class,
                                   PROP_INSPECTED,
                                   g_param_spec_object ("inspected", NULL, NULL,
                                                        BOBGUI_TYPE_WIDGET,
                                                        G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
                                   PROP_MAGNIFICATION,
                                   g_param_spec_double ("magnification", NULL, NULL,
                                                        1, G_MAXDOUBLE, 1,
                                                        G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
                                   PROP_RESIZE,
                                   g_param_spec_boolean ("resize", NULL, NULL,
                                                         FALSE,
                                                         G_PARAM_READWRITE));

  bobgui_widget_class_set_css_name (widget_class, "magnifier");
}

static void
bobgui_magnifier_init (BobguiMagnifier *magnifier)
{
  BobguiWidget *widget = BOBGUI_WIDGET (magnifier);

  bobgui_widget_set_overflow (widget, BOBGUI_OVERFLOW_HIDDEN);

  magnifier->magnification = 1;
  magnifier->resize = FALSE;
  magnifier->paintable = bobgui_widget_paintable_new (NULL);
  g_signal_connect_swapped (magnifier->paintable, "invalidate-contents", G_CALLBACK (bobgui_widget_queue_draw), magnifier);
  g_signal_connect_swapped (magnifier->paintable, "invalidate-size", G_CALLBACK (bobgui_widget_queue_resize), magnifier);
}

BobguiWidget *
_bobgui_magnifier_new (BobguiWidget *inspected)
{
  g_return_val_if_fail (BOBGUI_IS_WIDGET (inspected), NULL);

  return g_object_new (BOBGUI_TYPE_MAGNIFIER,
                       "inspected", inspected,
                       NULL);
}

BobguiWidget *
_bobgui_magnifier_get_inspected (BobguiMagnifier *magnifier)
{
  g_return_val_if_fail (BOBGUI_IS_MAGNIFIER (magnifier), NULL);

  return bobgui_widget_paintable_get_widget (BOBGUI_WIDGET_PAINTABLE (magnifier->paintable));
}

void
_bobgui_magnifier_set_inspected (BobguiMagnifier *magnifier,
                              BobguiWidget    *inspected)
{
  g_return_if_fail (BOBGUI_IS_MAGNIFIER (magnifier));
  g_return_if_fail (inspected == NULL || BOBGUI_IS_WIDGET (inspected));

  bobgui_widget_paintable_set_widget (BOBGUI_WIDGET_PAINTABLE (magnifier->paintable), inspected);

  g_object_notify (G_OBJECT (magnifier), "inspected");
}

void
_bobgui_magnifier_set_coords (BobguiMagnifier *magnifier,
                           double        x,
                           double        y)
{
  g_return_if_fail (BOBGUI_IS_MAGNIFIER (magnifier));

  if (magnifier->x == x && magnifier->y == y)
    return;

  magnifier->x = x;
  magnifier->y = y;

  bobgui_widget_queue_draw (BOBGUI_WIDGET (magnifier));
}

void
_bobgui_magnifier_get_coords (BobguiMagnifier *magnifier,
                           double       *x,
                           double       *y)
{
  g_return_if_fail (BOBGUI_IS_MAGNIFIER (magnifier));

  if (x)
    *x = magnifier->x;

  if (y)
    *y = magnifier->y;
}

void
_bobgui_magnifier_set_magnification (BobguiMagnifier *magnifier,
                                  double        magnification)
{
  g_return_if_fail (BOBGUI_IS_MAGNIFIER (magnifier));

  if (magnifier->magnification == magnification)
    return;

  magnifier->magnification = magnification;
  g_object_notify (G_OBJECT (magnifier), "magnification");

  if (magnifier->resize)
    bobgui_widget_queue_resize (BOBGUI_WIDGET (magnifier));

  bobgui_widget_queue_draw (BOBGUI_WIDGET (magnifier));
}

double
_bobgui_magnifier_get_magnification (BobguiMagnifier *magnifier)
{
  g_return_val_if_fail (BOBGUI_IS_MAGNIFIER (magnifier), 1);

  return magnifier->magnification;
}

void
_bobgui_magnifier_set_resize (BobguiMagnifier *magnifier,
                           gboolean      resize)
{
  g_return_if_fail (BOBGUI_IS_MAGNIFIER (magnifier));

  if (magnifier->resize == resize)
    return;

  magnifier->resize = resize;

  bobgui_widget_queue_resize (BOBGUI_WIDGET (magnifier));
}

gboolean
_bobgui_magnifier_get_resize (BobguiMagnifier *magnifier)
{
  g_return_val_if_fail (BOBGUI_IS_MAGNIFIER (magnifier), FALSE);

  return magnifier->resize;
}
