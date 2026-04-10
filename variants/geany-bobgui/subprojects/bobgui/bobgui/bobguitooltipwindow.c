/* BOBGUI - The Bobgui Framework
 * Copyright 2015  Emmanuele Bassi 
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

/*
 * Modified by the BOBGUI Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the BOBGUI Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BOBGUI at ftp://ftp.bobgui.org/pub/bobgui/.
 */

#include "config.h"

#include "bobguitooltipwindowprivate.h"

#include "bobguibinlayout.h"
#include "bobguibox.h"
#include "bobguiimage.h"
#include "bobguilabel.h"
#include "bobguiprivate.h"
#include "bobguisettings.h"
#include "bobguisizerequest.h"
#include "bobguiwindowprivate.h"
#include "bobguiwidgetprivate.h"
#include "bobguinativeprivate.h"
#include "bobguicssboxesimplprivate.h"

#include "gdk/gdksurfaceprivate.h"
#include "gsk/gskrendererprivate.h"

struct _BobguiTooltipWindow
{
  BobguiWidget parent_instance;

  GdkSurface *surface;
  GskRenderer *renderer;

  BobguiWidget *relative_to;
  GdkRectangle rect;
  GdkGravity rect_anchor;
  GdkGravity surface_anchor;
  GdkAnchorHints anchor_hints;
  int dx;
  int dy;
  guint surface_transform_changed_cb;

  BobguiWidget *box;
  BobguiWidget *image;
  BobguiWidget *label;
  BobguiWidget *custom_widget;
};

struct _BobguiTooltipWindowClass
{
  BobguiWidgetClass parent_class;
};

static void bobgui_tooltip_window_native_init (BobguiNativeInterface *iface);

G_DEFINE_TYPE_WITH_CODE (BobguiTooltipWindow, bobgui_tooltip_window, BOBGUI_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_NATIVE,
                                                bobgui_tooltip_window_native_init))


static GdkSurface *
bobgui_tooltip_window_native_get_surface (BobguiNative *native)
{
  BobguiTooltipWindow *window = BOBGUI_TOOLTIP_WINDOW (native);

  return window->surface;
}

static GskRenderer *
bobgui_tooltip_window_native_get_renderer (BobguiNative *native)
{
  BobguiTooltipWindow *window = BOBGUI_TOOLTIP_WINDOW (native);

  return window->renderer;
}

static void
bobgui_tooltip_window_native_get_surface_transform (BobguiNative *native,
                                                 double    *x,
                                                 double    *y)
{
  BobguiCssBoxes css_boxes;
  const graphene_rect_t *margin_rect;

  bobgui_css_boxes_init (&css_boxes, BOBGUI_WIDGET (native));
  margin_rect = bobgui_css_boxes_get_margin_rect (&css_boxes);

  *x = - margin_rect->origin.x;
  *y = - margin_rect->origin.y;
}

static GdkPopupLayout *
create_popup_layout (BobguiTooltipWindow *window)
{
  GdkPopupLayout *layout;

  layout = gdk_popup_layout_new (&window->rect,
                                 window->rect_anchor,
                                 window->surface_anchor);
  gdk_popup_layout_set_anchor_hints (layout, window->anchor_hints);
  gdk_popup_layout_set_offset (layout, window->dx, window->dy);

  return layout;
}

static void
bobgui_tooltip_window_relayout (BobguiTooltipWindow *window)
{
  BobguiRequisition req;
  GdkPopupLayout *layout;

  if (!bobgui_widget_get_visible (BOBGUI_WIDGET (window)) ||
      window->surface == NULL)
    return;

  bobgui_widget_get_preferred_size (BOBGUI_WIDGET (window), NULL, &req);
  layout = create_popup_layout (window);
  gdk_popup_present (GDK_POPUP (window->surface),
                     MAX (req.width, 1),
                     MAX (req.height, 1),
                     layout);
  gdk_popup_layout_unref (layout);
}

void
bobgui_tooltip_window_present (BobguiTooltipWindow *window)
{
  BobguiWidget *widget = BOBGUI_WIDGET (window);

  if (!_bobgui_widget_get_alloc_needed (widget))
    {
      bobgui_widget_ensure_allocate (widget);
    }
  else if (bobgui_widget_get_visible (widget))
    {
      bobgui_tooltip_window_relayout (window);
    }
}

static void
bobgui_tooltip_window_native_layout (BobguiNative *native,
                                  int        width,
                                  int        height)
{
  BobguiWidget *widget = BOBGUI_WIDGET (native);

  if (bobgui_widget_needs_allocate (widget))
    bobgui_widget_allocate (widget, width, height, -1, NULL);
  else
    bobgui_widget_ensure_allocate (widget);

}

static void
bobgui_tooltip_window_native_init (BobguiNativeInterface *iface)
{
  iface->get_surface = bobgui_tooltip_window_native_get_surface;
  iface->get_renderer = bobgui_tooltip_window_native_get_renderer;
  iface->get_surface_transform = bobgui_tooltip_window_native_get_surface_transform;
  iface->layout = bobgui_tooltip_window_native_layout;
}

static void
mapped_changed (GdkSurface *surface,
                GParamSpec *pspec,
                BobguiWidget  *widget)
{
  if (!gdk_surface_get_mapped (surface))
    bobgui_widget_set_visible (widget, FALSE);
}

static gboolean
surface_render (GdkSurface     *surface,
                cairo_region_t *region,
                BobguiWidget      *widget)
{
  bobgui_widget_render (widget, surface, region);
  return TRUE;
}

static gboolean
surface_event (GdkSurface *surface,
               GdkEvent   *event,
               BobguiWidget  *widget)
{
  return bobgui_main_do_event (event);
}

static void
bobgui_tooltip_window_realize (BobguiWidget *widget)
{
  BobguiTooltipWindow *window = BOBGUI_TOOLTIP_WINDOW (widget);
  GdkSurface *parent;

  parent = bobgui_native_get_surface (bobgui_widget_get_native (window->relative_to));
  window->surface = gdk_surface_new_popup (parent, FALSE);

  gdk_surface_set_widget (window->surface, widget);

  g_signal_connect (window->surface, "notify::mapped", G_CALLBACK (mapped_changed), widget);
  g_signal_connect (window->surface, "render", G_CALLBACK (surface_render), widget);
  g_signal_connect (window->surface, "event", G_CALLBACK (surface_event), widget);

  BOBGUI_WIDGET_CLASS (bobgui_tooltip_window_parent_class)->realize (widget);

  window->renderer = gsk_renderer_new_for_surface_full (window->surface, TRUE);

  bobgui_native_realize (BOBGUI_NATIVE (window));
}

static void
bobgui_tooltip_window_unrealize (BobguiWidget *widget)
{
  BobguiTooltipWindow *window = BOBGUI_TOOLTIP_WINDOW (widget);

  bobgui_native_unrealize (BOBGUI_NATIVE (window));

  BOBGUI_WIDGET_CLASS (bobgui_tooltip_window_parent_class)->unrealize (widget);

  gsk_renderer_unrealize (window->renderer);
  g_clear_object (&window->renderer);

  g_signal_handlers_disconnect_by_func (window->surface, mapped_changed, widget);
  g_signal_handlers_disconnect_by_func (window->surface, surface_render, widget);
  g_signal_handlers_disconnect_by_func (window->surface, surface_event, widget);
  gdk_surface_set_widget (window->surface, NULL);
  g_clear_pointer (&window->surface, gdk_surface_destroy);
}


static void
unset_surface_transform_changed_cb (gpointer data)
{
  BobguiTooltipWindow *window = BOBGUI_TOOLTIP_WINDOW (data);

  window->surface_transform_changed_cb = 0;
}

static gboolean
surface_transform_changed_cb (BobguiWidget               *widget,
                              const graphene_matrix_t *transform,
                              gpointer                 user_data)
{
  BobguiTooltipWindow *window = BOBGUI_TOOLTIP_WINDOW (user_data);

  bobgui_tooltip_window_relayout (window);

  return G_SOURCE_CONTINUE;
}


static void
bobgui_tooltip_window_map (BobguiWidget *widget)
{
  BobguiTooltipWindow *window = BOBGUI_TOOLTIP_WINDOW (widget);
  GdkPopupLayout *layout;

  layout = create_popup_layout (window);
  gdk_popup_present (GDK_POPUP (window->surface),
                     gdk_surface_get_width (window->surface),
                     gdk_surface_get_height (window->surface),
                     layout);
  gdk_popup_layout_unref (layout);

  window->surface_transform_changed_cb =
    bobgui_widget_add_surface_transform_changed_callback (window->relative_to,
                                                       surface_transform_changed_cb,
                                                       window,
                                                       unset_surface_transform_changed_cb);

  BOBGUI_WIDGET_CLASS (bobgui_tooltip_window_parent_class)->map (widget);

  if (bobgui_widget_get_visible (window->box))
    bobgui_widget_map (window->box);
}

static void
bobgui_tooltip_window_unmap (BobguiWidget *widget)
{
  BobguiTooltipWindow *window = BOBGUI_TOOLTIP_WINDOW (widget);

  bobgui_widget_remove_surface_transform_changed_callback (window->relative_to,
                                                        window->surface_transform_changed_cb);
  window->surface_transform_changed_cb = 0;

  BOBGUI_WIDGET_CLASS (bobgui_tooltip_window_parent_class)->unmap (widget);
  gdk_surface_hide (window->surface);

  bobgui_widget_unmap (window->box);
}

static void
bobgui_tooltip_window_show (BobguiWidget *widget)
{
  _bobgui_widget_set_visible_flag (widget, TRUE);
  bobgui_widget_realize (widget);
  bobgui_tooltip_window_present (BOBGUI_TOOLTIP_WINDOW (widget));
  bobgui_widget_map (widget);
}

static void
bobgui_tooltip_window_hide (BobguiWidget *widget)
{
  _bobgui_widget_set_visible_flag (widget, FALSE);
  bobgui_widget_unmap (widget);
}

static void
bobgui_tooltip_window_dispose (GObject *object)
{
  BobguiTooltipWindow *window = BOBGUI_TOOLTIP_WINDOW (object);

  if (window->relative_to)
    bobgui_widget_unparent (BOBGUI_WIDGET (window));

  g_clear_pointer (&window->box, bobgui_widget_unparent);

  G_OBJECT_CLASS (bobgui_tooltip_window_parent_class)->dispose (object);
}

static void
bobgui_tooltip_window_class_init (BobguiTooltipWindowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);

  object_class->dispose = bobgui_tooltip_window_dispose;
  widget_class->realize = bobgui_tooltip_window_realize;
  widget_class->unrealize = bobgui_tooltip_window_unrealize;
  widget_class->map = bobgui_tooltip_window_map;
  widget_class->unmap = bobgui_tooltip_window_unmap;
  widget_class->show = bobgui_tooltip_window_show;
  widget_class->hide = bobgui_tooltip_window_hide;

  bobgui_widget_class_set_css_name (widget_class, I_("tooltip"));
  bobgui_widget_class_set_layout_manager_type (widget_class, BOBGUI_TYPE_BIN_LAYOUT);
  bobgui_widget_class_set_template_from_resource (widget_class, "/org/bobgui/libbobgui/ui/bobguitooltipwindow.ui");

  bobgui_widget_class_bind_template_child (widget_class, BobguiTooltipWindow, box);
  bobgui_widget_class_bind_template_child (widget_class, BobguiTooltipWindow, image);
  bobgui_widget_class_bind_template_child (widget_class, BobguiTooltipWindow, label);
}

static void
bobgui_tooltip_window_init (BobguiTooltipWindow *self)
{
  bobgui_widget_init_template (BOBGUI_WIDGET (self));
}

BobguiWidget *
bobgui_tooltip_window_new (void)
{
  return g_object_new (BOBGUI_TYPE_TOOLTIP_WINDOW, NULL);
}

void
bobgui_tooltip_window_set_label_markup (BobguiTooltipWindow *window,
                                     const char       *markup)
{
  if (markup != NULL)
    bobgui_label_set_markup (BOBGUI_LABEL (window->label), markup);

  bobgui_widget_set_visible (window->label, markup != NULL);
}

void
bobgui_tooltip_window_set_label_text (BobguiTooltipWindow *window,
                                   const char       *text)
{
  if (text != NULL)
    bobgui_label_set_text (BOBGUI_LABEL (window->label), text);

  bobgui_widget_set_visible (window->label, text != NULL);
}

void
bobgui_tooltip_window_set_image_icon (BobguiTooltipWindow *window,
                                   GdkPaintable     *paintable)
{
  if (paintable != NULL)
    bobgui_image_set_from_paintable (BOBGUI_IMAGE (window->image), paintable);

  bobgui_widget_set_visible (window->image, paintable != NULL);
}

void
bobgui_tooltip_window_set_image_icon_from_name (BobguiTooltipWindow *window,
                                             const char       *icon_name)
{
  bobgui_widget_set_visible (window->image, icon_name != NULL);
  if (icon_name)
    bobgui_image_set_from_icon_name (BOBGUI_IMAGE (window->image), icon_name);
}

void
bobgui_tooltip_window_set_image_icon_from_gicon (BobguiTooltipWindow *window,
                                              GIcon            *gicon)
{
  bobgui_widget_set_visible (window->image, gicon != NULL);
  if (gicon != NULL)
    bobgui_image_set_from_gicon (BOBGUI_IMAGE (window->image), gicon);
}

void
bobgui_tooltip_window_set_custom_widget (BobguiTooltipWindow *window,
                                      BobguiWidget        *custom_widget)
{
  /* No need to do anything if the custom widget stays the same */
  if (window->custom_widget == custom_widget)
    return;

  if (window->custom_widget != NULL)
    {
      BobguiWidget *custom = window->custom_widget;

      /* Note: We must reset window->custom_widget first,
       * since bobgui_container_remove() will recurse into
       * bobgui_tooltip_set_custom()
       */
      window->custom_widget = NULL;
      bobgui_box_remove (BOBGUI_BOX (window->box), custom);
      g_object_unref (custom);
    }

  if (custom_widget != NULL)
    {
      window->custom_widget = g_object_ref (custom_widget);

      bobgui_box_append (BOBGUI_BOX (window->box), custom_widget);
      bobgui_widget_set_visible (custom_widget, TRUE);
      bobgui_widget_set_visible (window->image, FALSE);
      bobgui_widget_set_visible (window->label, FALSE);
    }
}

void
bobgui_tooltip_window_set_relative_to (BobguiTooltipWindow *window,
                                    BobguiWidget        *relative_to)
{
  g_return_if_fail (BOBGUI_WIDGET (window) != relative_to);

  if (window->relative_to == relative_to)
    return;

  g_object_ref (window);

  if (window->relative_to)
    bobgui_widget_unparent (BOBGUI_WIDGET (window));

  window->relative_to = relative_to;

  if (window->relative_to)
    bobgui_widget_set_parent (BOBGUI_WIDGET (window), relative_to);

  g_object_unref (window);
}

void
bobgui_tooltip_window_position (BobguiTooltipWindow *window,
                             GdkRectangle     *rect,
                             GdkGravity        rect_anchor,
                             GdkGravity        surface_anchor,
                             GdkAnchorHints    anchor_hints,
                             int               dx,
                             int               dy)
{
  window->rect = *rect;
  window->rect_anchor = rect_anchor;
  window->surface_anchor = surface_anchor;
  window->anchor_hints = anchor_hints;
  window->dx = dx;
  window->dy = dy;

  bobgui_tooltip_window_relayout (window);
}
