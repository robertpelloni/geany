/* BOBGUI - The Bobgui Framework
 * Copyright (c) 2024 Arjan Molenaar <amolenaar@gnome.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "config.h"

#import <AppKit/AppKit.h>
#include <math.h>
#include <gdk/macos/GdkMacosWindow.h>
#include "bobguiprivate.h"
#include "bobguinative.h"
#include "bobguiwindowbuttonsquartzprivate.h"

@interface NSWindow()
/* Expose the private titlebarHeight property, so we can set
 * the titlebar height to match the height of a BOBGUI header bar.
 */
@property CGFloat titlebarHeight;

@end

enum
{
  PROP_0,
  PROP_DECORATION_LAYOUT,
  NUM_PROPERTIES
};

static GParamSpec *props[NUM_PROPERTIES] = { NULL, };

/**
 * BobguiWindowButtonsQuartz:
 *
 * This class provides macOS native window buttons for close/minimize/maximize.
 *
 * The buttons can be set by adding "native" to the `decoration-layout` of
 * BobguiWindowControls or BobguiHeader.
 *
 * ## Accessibility
 *
 * `BobguiWindowButtonsQuartz` uses the `BOBGUI_ACCESSIBLE_ROLE_IMG` role.
 */

typedef struct _BobguiWindowButtonsQuartzClass BobguiWindowButtonsQuartzClass;

struct _BobguiWindowButtonsQuartz
{
  BobguiWidget parent_instance;

  gboolean close;
  gboolean minimize;
  gboolean maximize;

  char *decoration_layout;

  GBinding *fullscreen_binding;
};

struct _BobguiWindowButtonsQuartzClass
{
  BobguiWidgetClass parent_class;
};

G_DEFINE_TYPE (BobguiWindowButtonsQuartz, bobgui_window_buttons_quartz, BOBGUI_TYPE_WIDGET)

static void
set_window_controls_height (GdkMacosWindow *window,
                            int             height)
{
  g_return_if_fail (GDK_IS_MACOS_WINDOW (window));

  if ([window respondsToSelector:@selector(setTitlebarHeight:)])
    [window setTitlebarHeight:height];
  [[window contentView] setNeedsLayout:YES];
}

static void
window_controls_bounds (NSWindow *window, NSRect *out_bounds)
{
  NSRect bounds = NSZeroRect;
  NSButton* button;

  button = [window standardWindowButton:NSWindowCloseButton];
  bounds = NSUnionRect(bounds, [button frame]);

  button = [window standardWindowButton:NSWindowZoomButton];
  bounds = NSUnionRect(bounds, [button frame]);

  *out_bounds = bounds;
}

static GdkMacosWindow*
native_window (BobguiWidget *widget)
{
  BobguiNative *native = BOBGUI_NATIVE (bobgui_widget_get_root (widget));

  if (native != NULL)
    {
      GdkSurface *surface = bobgui_native_get_surface (native);

      if (GDK_IS_MACOS_SURFACE (surface))
        return (GdkMacosWindow*) gdk_macos_surface_get_native_window (GDK_MACOS_SURFACE (surface));
    }
  return NULL;
}

static void
enable_window_controls (BobguiWindowButtonsQuartz *self,
                        gboolean                enabled)
{
  gboolean is_sovereign_window;
  gboolean resizable;
  gboolean deletable;
  BobguiRoot *root;
  BobguiWindow *window;
  GdkMacosWindow *nswindow;

  root = bobgui_widget_get_root (BOBGUI_WIDGET (self));
  if (!root || !BOBGUI_IS_WINDOW (root))
    return;

  nswindow = native_window (BOBGUI_WIDGET (self));
  if (!GDK_IS_MACOS_WINDOW (nswindow))
    return;

  window = BOBGUI_WINDOW (root);
  is_sovereign_window = !bobgui_window_get_modal (window) &&
                         bobgui_window_get_transient_for (window) == NULL;
  resizable = bobgui_window_get_resizable (window);
  deletable = bobgui_window_get_deletable (window);

  [[nswindow standardWindowButton:NSWindowCloseButton] setEnabled:enabled && self->close && deletable];
  [[nswindow standardWindowButton:NSWindowMiniaturizeButton] setEnabled:enabled && self->minimize && is_sovereign_window];
  [[nswindow standardWindowButton:NSWindowZoomButton] setEnabled:enabled && self->maximize && resizable && is_sovereign_window];
}

static void
update_window_controls_from_decoration_layout (BobguiWindowButtonsQuartz *self)
{
  char **tokens;

  if (self->decoration_layout)
    tokens = g_strsplit_set (self->decoration_layout, ",:", -1);
  else
    {
      char *layout_desc;
      g_object_get (bobgui_widget_get_settings (BOBGUI_WIDGET (self)),
                    "bobgui-decoration-layout", &layout_desc,
                    NULL);
      tokens = g_strsplit_set (layout_desc, ",:", -1);

      g_free (layout_desc);
    }

  self->close = g_strv_contains ((const char * const *) tokens, "close");
  self->minimize = g_strv_contains ((const char * const *) tokens, "minimize");
  self->maximize = g_strv_contains ((const char * const *) tokens, "maximize");

  g_strfreev (tokens);

  enable_window_controls (self, TRUE);
}

static void
bobgui_window_buttons_quartz_finalize (GObject *object)
{
  BobguiWindowButtonsQuartz *self = BOBGUI_WINDOW_BUTTONS_QUARTZ (object);

  g_clear_pointer (&self->decoration_layout, g_free);

  G_OBJECT_CLASS (bobgui_window_buttons_quartz_parent_class)->finalize (object);
}

static void
bobgui_window_buttons_quartz_get_property (GObject     *object,
                                        guint        prop_id,
                                        GValue      *value,
                                        GParamSpec  *pspec)
{
  BobguiWindowButtonsQuartz *self = BOBGUI_WINDOW_BUTTONS_QUARTZ (object);

  switch (prop_id)
    {
    case PROP_DECORATION_LAYOUT:
      g_value_set_string (value, self->decoration_layout);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_window_buttons_quartz_set_property (GObject      *object,
                                        guint         prop_id,
                                        const GValue *value,
                                        GParamSpec   *pspec)
{
  BobguiWindowButtonsQuartz *self = BOBGUI_WINDOW_BUTTONS_QUARTZ (object);

  switch (prop_id)
    {
    case PROP_DECORATION_LAYOUT:
      g_free (self->decoration_layout);
      self->decoration_layout = g_strdup (g_value_get_string (value));

      update_window_controls_from_decoration_layout (self);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_window_buttons_quartz_root (BobguiWidget *widget)
{
  BobguiWindowButtonsQuartz *self = BOBGUI_WINDOW_BUTTONS_QUARTZ (widget);

  BOBGUI_WIDGET_CLASS (bobgui_window_buttons_quartz_parent_class)->root (widget);

  if (self->fullscreen_binding)
    g_object_unref (self->fullscreen_binding);

  self->fullscreen_binding = g_object_bind_property (bobgui_widget_get_root (widget),
                                                     "fullscreened",
                                                     widget,
                                                     "visible",
                                                     G_BINDING_SYNC_CREATE | G_BINDING_INVERT_BOOLEAN);
}

static void
bobgui_window_buttons_quartz_unroot (BobguiWidget *widget)
{
  BobguiWindowButtonsQuartz *self = BOBGUI_WINDOW_BUTTONS_QUARTZ (widget);

  if (self->fullscreen_binding)
    {
      g_object_unref (self->fullscreen_binding);
      self->fullscreen_binding = NULL;
    }

  BOBGUI_WIDGET_CLASS (bobgui_window_buttons_quartz_parent_class)->unroot (widget);
}

static void
bobgui_window_buttons_quartz_realize (BobguiWidget *widget)
{
  BobguiWindowButtonsQuartz *self = BOBGUI_WINDOW_BUTTONS_QUARTZ (widget);
  GdkMacosWindow *window;
  NSRect bounds;

  BOBGUI_WIDGET_CLASS (bobgui_window_buttons_quartz_parent_class)->realize (widget);

  window = native_window (widget);

  if (!GDK_IS_MACOS_WINDOW (window))
    {
      g_critical ("Cannot show BobguiWindowButtonsQuartz on a non-macos window");
      return;
    }

  [window setShowStandardWindowButtons:YES];

  enable_window_controls (self, TRUE);

  window_controls_bounds (window, &bounds);
  bobgui_widget_set_size_request (widget, bounds.origin.x + bounds.size.width, bounds.size.height);
}

static void
bobgui_window_buttons_quartz_unrealize (BobguiWidget *widget)
{
  GdkMacosWindow *window = native_window (widget);

  if (GDK_IS_MACOS_WINDOW (window))
    [window setShowStandardWindowButtons:NO];

  BOBGUI_WIDGET_CLASS (bobgui_window_buttons_quartz_parent_class)->unrealize (widget);
}

static void
bobgui_window_buttons_quartz_measure (BobguiWidget      *widget,
                                   BobguiOrientation  orientation,
                                   int             for_size,
                                   int            *minimum,
                                   int            *natural,
                                   int            *minimum_baseline,
                                   int            *natural_baseline)
{
  GdkMacosWindow *window = native_window (widget);

  if (GDK_IS_MACOS_WINDOW (window))
    {
      NSRect bounds;

      window_controls_bounds (window, &bounds);

      if (orientation == BOBGUI_ORIENTATION_VERTICAL)
        *minimum = *natural = ceil(bounds.size.height);
      else if (orientation == BOBGUI_ORIENTATION_HORIZONTAL)
        *minimum = *natural = ceil(bounds.origin.x + bounds.size.width);
    }
}

static void
bobgui_window_buttons_quartz_size_allocate (BobguiWidget *widget,
                                         int        width,
                                         int        height,
                                         int        baseline)
{
  GdkMacosWindow *window = native_window (widget);
  graphene_rect_t bounds = { 0 };

  BOBGUI_WIDGET_CLASS (bobgui_window_buttons_quartz_parent_class)->size_allocate (widget, width, height, baseline);

  if (!bobgui_widget_compute_bounds (widget, (BobguiWidget *) bobgui_widget_get_root (widget), &bounds))
    g_warning ("Could not calculate widget bounds");

  set_window_controls_height (window, bounds.origin.y * 2 + height);
}

static void
bobgui_window_buttons_quartz_class_init (BobguiWindowButtonsQuartzClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (class);

  gobject_class->finalize = bobgui_window_buttons_quartz_finalize;
  gobject_class->get_property = bobgui_window_buttons_quartz_get_property;
  gobject_class->set_property = bobgui_window_buttons_quartz_set_property;

  widget_class->measure = bobgui_window_buttons_quartz_measure;
  widget_class->size_allocate = bobgui_window_buttons_quartz_size_allocate;
  widget_class->root = bobgui_window_buttons_quartz_root;
  widget_class->unroot = bobgui_window_buttons_quartz_unroot;
  widget_class->realize = bobgui_window_buttons_quartz_realize;
  widget_class->unrealize = bobgui_window_buttons_quartz_unrealize;

  /**
   * BobguiWindowButtonsQuartz:decoration-layout:
   *
   * The decoration layout for window buttons.
   *
   * If this property is not set, the
   * [property@Bobgui.Settings:bobgui-decoration-layout] setting is used.
   */
  props[PROP_DECORATION_LAYOUT] =
      g_param_spec_string ("decoration-layout", NULL, NULL,
                           NULL,
                           BOBGUI_PARAM_READWRITE);

  g_object_class_install_properties (gobject_class, NUM_PROPERTIES, props);

  bobgui_widget_class_set_css_name (widget_class, I_("windowbuttonsquartz"));

  bobgui_widget_class_set_accessible_role (widget_class, BOBGUI_ACCESSIBLE_ROLE_IMG);
}

static void
bobgui_window_buttons_quartz_init (BobguiWindowButtonsQuartz *self)
{
  self->close = TRUE;
  self->minimize = TRUE;
  self->maximize = TRUE;
  self->decoration_layout = NULL;
  self->fullscreen_binding = NULL;
}
