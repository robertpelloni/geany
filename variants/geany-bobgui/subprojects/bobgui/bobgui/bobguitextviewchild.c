/* -*- Mode: C; c-file-style: "gnu"; tab-width: 8 -*- */
/* BOBGUI - The Bobgui Framework
 * bobgui_text_view_child.c Copyright (C) 2019 Red Hat, Inc.
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

#include "bobguicssnodeprivate.h"
#include "bobguiprivate.h"
#include "bobguitextview.h"
#include "bobguitextviewchildprivate.h"
#include "bobguitypebuiltins.h"
#include "bobguiwidgetprivate.h"

typedef struct
{
  GList      link;
  BobguiWidget *widget;
  int        x;
  int        y;
} Overlay;

struct _BobguiTextViewChild
{
  BobguiWidget          parent_instance;
  BobguiTextWindowType  window_type;
  GQueue             overlays;
  int                xoffset;
  int                yoffset;
  BobguiWidget         *child;
};

enum {
  PROP_0,
  PROP_WINDOW_TYPE,
  N_PROPS
};

G_DEFINE_TYPE (BobguiTextViewChild, bobgui_text_view_child, BOBGUI_TYPE_WIDGET)

static GParamSpec *properties[N_PROPS];

static Overlay *
overlay_new (BobguiWidget *widget,
             int        x,
             int        y)
{
  Overlay *overlay;

  overlay = g_new0 (Overlay, 1);
  overlay->link.data = overlay;
  overlay->widget = g_object_ref (widget);
  overlay->x = x;
  overlay->y = y;

  return overlay;
}

static void
overlay_free (Overlay *overlay)
{
  g_assert (overlay->link.prev == NULL);
  g_assert (overlay->link.next == NULL);

  g_object_unref (overlay->widget);
  g_free (overlay);
}

static void
bobgui_text_view_child_remove_overlay (BobguiTextViewChild *self,
                                    Overlay          *overlay)
{
  g_queue_unlink (&self->overlays, &overlay->link);
  bobgui_widget_unparent (overlay->widget);
  overlay_free (overlay);
}

static Overlay *
bobgui_text_view_child_get_overlay (BobguiTextViewChild *self,
                                 BobguiWidget        *widget)
{
  GList *iter;

  for (iter = self->overlays.head; iter; iter = iter->next)
    {
      Overlay *overlay = iter->data;

      if (overlay->widget == widget)
        return overlay;
    }

  return NULL;
}

void
bobgui_text_view_child_add (BobguiTextViewChild *self,
                         BobguiWidget        *widget)
{
  if (self->child != NULL)
    {
      g_warning ("%s allows a single child and already contains a %s",
                 G_OBJECT_TYPE_NAME (self),
                 G_OBJECT_TYPE_NAME (widget));
      return;
    }

  self->child = g_object_ref (widget);
  bobgui_widget_set_parent (widget, BOBGUI_WIDGET (self));
}

void
bobgui_text_view_child_remove (BobguiTextViewChild *self,
                            BobguiWidget        *widget)
{
  if (widget == self->child)
    {
      self->child = NULL;
      bobgui_widget_unparent (widget);
      g_object_unref (widget);
    }
  else
    {
      Overlay *overlay = bobgui_text_view_child_get_overlay (self, widget);

      if (overlay != NULL)
        bobgui_text_view_child_remove_overlay (self, overlay);
    }
}

static void
bobgui_text_view_child_measure (BobguiWidget      *widget,
                             BobguiOrientation  orientation,
                             int             for_size,
                             int            *min_size,
                             int            *nat_size,
                             int            *min_baseline,
                             int            *nat_baseline)
{
  BobguiTextViewChild *self = BOBGUI_TEXT_VIEW_CHILD (widget);
  const GList *iter;
  int real_min_size = 0;
  int real_nat_size = 0;

  if (self->child != NULL)
    bobgui_widget_measure (self->child,
                        orientation,
                        for_size,
                        &real_min_size,
                        &real_nat_size,
                        NULL,
                        NULL);

  for (iter = self->overlays.head; iter; iter = iter->next)
    {
      Overlay *overlay = iter->data;
      int child_min_size = 0;
      int child_nat_size = 0;

      bobgui_widget_measure (overlay->widget,
                          orientation,
                          for_size,
                          &child_min_size,
                          &child_nat_size,
                          NULL,
                          NULL);

      if (child_min_size > real_min_size)
        real_min_size = child_min_size;

      if (child_nat_size > real_nat_size)
        real_nat_size = child_nat_size;
    }

  if (min_size)
    *min_size = real_min_size;

  if (nat_size)
    *nat_size = real_nat_size;

  if (min_baseline)
    *min_baseline = -1;

  if (nat_baseline)
    *nat_baseline = -1;
}

static void
bobgui_text_view_child_size_allocate (BobguiWidget *widget,
                                   int        width,
                                   int        height,
                                   int        baseline)
{
  BobguiTextViewChild *self = BOBGUI_TEXT_VIEW_CHILD (widget);
  BobguiRequisition min_req;
  GdkRectangle rect;
  const GList *iter;

  BOBGUI_WIDGET_CLASS (bobgui_text_view_child_parent_class)->size_allocate (widget, width, height, baseline);

  if (self->child != NULL)
    {
      rect.x = 0;
      rect.y = 0;
      rect.width = width;
      rect.height = height;

      bobgui_widget_size_allocate (self->child, &rect, baseline);
    }

  for (iter = self->overlays.head; iter; iter = iter->next)
    {
      Overlay *overlay = iter->data;

      bobgui_widget_get_preferred_size (overlay->widget, &min_req, NULL);

      rect.width = min_req.width;
      rect.height = min_req.height;

      if (self->window_type == BOBGUI_TEXT_WINDOW_TEXT ||
          self->window_type == BOBGUI_TEXT_WINDOW_TOP ||
          self->window_type == BOBGUI_TEXT_WINDOW_BOTTOM)
        rect.x = overlay->x - self->xoffset;
      else
        rect.x = overlay->x;

      if (self->window_type == BOBGUI_TEXT_WINDOW_TEXT ||
          self->window_type == BOBGUI_TEXT_WINDOW_RIGHT ||
          self->window_type == BOBGUI_TEXT_WINDOW_LEFT)
        rect.y = overlay->y - self->yoffset;
      else
        rect.y = overlay->y;

      bobgui_widget_size_allocate (overlay->widget, &rect, -1);
    }
}

static void
bobgui_text_view_child_snapshot (BobguiWidget   *widget,
                              BobguiSnapshot *snapshot)
{
  BobguiTextViewChild *self = BOBGUI_TEXT_VIEW_CHILD (widget);
  const GList *iter;

  if (self->child)
    bobgui_widget_snapshot_child (widget, self->child, snapshot);

  for (iter = self->overlays.head; iter; iter = iter->next)
    {
      Overlay *overlay = iter->data;

      bobgui_widget_snapshot_child (widget, overlay->widget, snapshot);
    }
}

static void
bobgui_text_view_child_constructed (GObject *object)
{
  BobguiTextViewChild *self = BOBGUI_TEXT_VIEW_CHILD (object);
  BobguiCssNode *css_node;

  G_OBJECT_CLASS (bobgui_text_view_child_parent_class)->constructed (object);

  css_node = bobgui_widget_get_css_node (BOBGUI_WIDGET (self));

  switch (self->window_type)
    {
    case BOBGUI_TEXT_WINDOW_LEFT:
      bobgui_css_node_set_name (css_node, g_quark_from_static_string ("border"));
      bobgui_css_node_add_class (css_node, g_quark_from_static_string ("left"));
      break;

    case BOBGUI_TEXT_WINDOW_RIGHT:
      bobgui_css_node_set_name (css_node, g_quark_from_static_string ("border"));
      bobgui_css_node_add_class (css_node, g_quark_from_static_string ("right"));
      break;

    case BOBGUI_TEXT_WINDOW_TOP:
      bobgui_css_node_set_name (css_node, g_quark_from_static_string ("border"));
      bobgui_css_node_add_class (css_node, g_quark_from_static_string ("top"));
      break;

    case BOBGUI_TEXT_WINDOW_BOTTOM:
      bobgui_css_node_set_name (css_node, g_quark_from_static_string ("border"));
      bobgui_css_node_add_class (css_node, g_quark_from_static_string ("bottom"));
      break;

    case BOBGUI_TEXT_WINDOW_TEXT:
      bobgui_css_node_set_name (css_node, g_quark_from_static_string ("child"));
      break;

    case BOBGUI_TEXT_WINDOW_WIDGET:
    default:
      break;
    }
}

static void
bobgui_text_view_child_get_property (GObject    *object,
                                  guint       prop_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  BobguiTextViewChild *self = BOBGUI_TEXT_VIEW_CHILD (object);

  switch (prop_id)
    {
    case PROP_WINDOW_TYPE:
      g_value_set_enum (value, self->window_type);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
bobgui_text_view_child_set_property (GObject      *object,
                                  guint         prop_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  BobguiTextViewChild *self = BOBGUI_TEXT_VIEW_CHILD (object);

  switch (prop_id)
    {
    case PROP_WINDOW_TYPE:
      self->window_type = g_value_get_enum (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
bobgui_text_view_child_dispose (GObject *object)
{
  BobguiTextViewChild *self = BOBGUI_TEXT_VIEW_CHILD (object);
  BobguiWidget *child;

  while ((child = bobgui_widget_get_first_child (BOBGUI_WIDGET (self))))
    bobgui_text_view_child_remove (self, child);

  G_OBJECT_CLASS (bobgui_text_view_child_parent_class)->dispose (object);
}

static void
bobgui_text_view_child_class_init (BobguiTextViewChildClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);

  object_class->dispose = bobgui_text_view_child_dispose;
  object_class->constructed = bobgui_text_view_child_constructed;
  object_class->get_property = bobgui_text_view_child_get_property;
  object_class->set_property = bobgui_text_view_child_set_property;

  widget_class->measure = bobgui_text_view_child_measure;
  widget_class->size_allocate = bobgui_text_view_child_size_allocate;
  widget_class->snapshot = bobgui_text_view_child_snapshot;

  /**
   * BobguiTextViewChild:window-type:
   *
   * The "window-type" property is the `BobguiTextWindowType` of the
   * `BobguiTextView` that the child is attached.
   */
  properties[PROP_WINDOW_TYPE] =
    g_param_spec_enum ("window-type", NULL, NULL,
                       BOBGUI_TYPE_TEXT_WINDOW_TYPE,
                       BOBGUI_TEXT_WINDOW_TEXT,
                       BOBGUI_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
bobgui_text_view_child_init (BobguiTextViewChild *self)
{
  self->window_type = BOBGUI_TEXT_WINDOW_TEXT;

  bobgui_widget_set_overflow (BOBGUI_WIDGET (self), BOBGUI_OVERFLOW_HIDDEN);
}

BobguiWidget *
bobgui_text_view_child_new (BobguiTextWindowType window_type)
{
  g_return_val_if_fail (window_type == BOBGUI_TEXT_WINDOW_LEFT ||
                        window_type == BOBGUI_TEXT_WINDOW_RIGHT ||
                        window_type == BOBGUI_TEXT_WINDOW_TOP ||
                        window_type == BOBGUI_TEXT_WINDOW_BOTTOM ||
                        window_type == BOBGUI_TEXT_WINDOW_TEXT,
                        NULL);

  return g_object_new (BOBGUI_TYPE_TEXT_VIEW_CHILD,
                       "window-type", window_type,
                       NULL);
}

void
bobgui_text_view_child_add_overlay (BobguiTextViewChild *self,
                                 BobguiWidget        *widget,
                                 int               xpos,
                                 int               ypos)
{
  Overlay *overlay;

  g_return_if_fail (BOBGUI_IS_TEXT_VIEW_CHILD (self));
  g_return_if_fail (BOBGUI_IS_WIDGET (widget));

  overlay = overlay_new (widget, xpos, ypos);
  g_queue_push_tail (&self->overlays, &overlay->link);
  bobgui_widget_set_parent (widget, BOBGUI_WIDGET (self));
}

void
bobgui_text_view_child_move_overlay (BobguiTextViewChild *self,
                                  BobguiWidget        *widget,
                                  int               xpos,
                                  int               ypos)
{
  Overlay *overlay;

  g_return_if_fail (BOBGUI_IS_TEXT_VIEW_CHILD (self));
  g_return_if_fail (BOBGUI_IS_WIDGET (widget));

  overlay = bobgui_text_view_child_get_overlay (self, widget);

  if (overlay != NULL)
    {
      overlay->x = xpos;
      overlay->y = ypos;

      if (bobgui_widget_get_visible (BOBGUI_WIDGET (self)) &&
          bobgui_widget_get_visible (widget))
        bobgui_widget_queue_allocate (BOBGUI_WIDGET (self));
    }
}

BobguiTextWindowType
bobgui_text_view_child_get_window_type (BobguiTextViewChild *self)
{
  g_return_val_if_fail (BOBGUI_IS_TEXT_VIEW_CHILD (self), 0);

  return self->window_type;
}

void
bobgui_text_view_child_set_offset (BobguiTextViewChild *self,
                                int               xoffset,
                                int               yoffset)
{
  gboolean changed = FALSE;

  g_return_if_fail (BOBGUI_IS_TEXT_VIEW_CHILD (self));

  if (self->window_type == BOBGUI_TEXT_WINDOW_TEXT ||
      self->window_type == BOBGUI_TEXT_WINDOW_TOP ||
      self->window_type == BOBGUI_TEXT_WINDOW_BOTTOM)
    {
      if (self->xoffset != xoffset)
        {
          self->xoffset = xoffset;
          changed = TRUE;
        }
    }

  if (self->window_type == BOBGUI_TEXT_WINDOW_TEXT ||
      self->window_type == BOBGUI_TEXT_WINDOW_LEFT ||
      self->window_type == BOBGUI_TEXT_WINDOW_RIGHT)
    {
      if (self->yoffset != yoffset)
        {
          self->yoffset = yoffset;
          changed = TRUE;
        }
    }

  if (changed)
    bobgui_widget_queue_allocate (BOBGUI_WIDGET (self));
}
