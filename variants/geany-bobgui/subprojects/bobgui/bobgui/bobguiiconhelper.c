/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2011 Red Hat, Inc.
 *
 * Authors: Cosimo Cecchi <cosimoc@gnome.org>
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

#include "bobguiiconhelperprivate.h"

#include <math.h>

#include "bobguicssenumvalueprivate.h"
#include "bobguicssnumbervalueprivate.h"
#include "bobguicssstyleprivate.h"
#include "bobguicsstransientnodeprivate.h"
#include "bobguiiconthemeprivate.h"
#include "bobguirendericonprivate.h"
#include "bobguiscalerprivate.h"
#include "bobguisnapshot.h"
#include "bobguiwidgetprivate.h"
#include "gdk/gdkprofilerprivate.h"
#include "bobguisymbolicpaintable.h"

struct _BobguiIconHelper
{
  GObject parent_instance;

  BobguiImageDefinition *def;

  int pixel_size;

  guint use_fallback : 1;
  guint texture_is_symbolic : 1;

  BobguiWidget *owner;
  BobguiCssNode *node;
  GdkPaintable *paintable;
};

static BobguiIconLookupFlags
get_icon_lookup_flags (BobguiIconHelper *self,
                       BobguiCssStyle   *style)
{
  BobguiIconLookupFlags flags;
  BobguiCssIconStyle icon_style;

  flags = 0;

  icon_style = _bobgui_css_icon_style_value_get (style->icon->icon_style);

  switch (icon_style)
    {
    case BOBGUI_CSS_ICON_STYLE_REGULAR:
      flags |= BOBGUI_ICON_LOOKUP_FORCE_REGULAR;
      break;
    case BOBGUI_CSS_ICON_STYLE_SYMBOLIC:
      flags |= BOBGUI_ICON_LOOKUP_FORCE_SYMBOLIC;
      break;
    case BOBGUI_CSS_ICON_STYLE_REQUESTED:
      break;
    default:
      g_assert_not_reached ();
      return 0;
    }

  return flags;
}

static GdkPaintable *
ensure_paintable_for_gicon (BobguiIconHelper    *self,
                            BobguiCssStyle      *style,
                            int               scale,
                            BobguiTextDirection  dir,
                            gboolean          preload,
                            GIcon            *gicon,
                            gboolean         *symbolic)
{
  BobguiIconTheme *icon_theme;
  int width, height;
  BobguiIconPaintable *icon;
  BobguiIconLookupFlags flags;

  icon_theme = bobgui_icon_theme_get_for_display (bobgui_widget_get_display (self->owner));
  flags = get_icon_lookup_flags (self, style);
  if (preload)
    flags |= BOBGUI_ICON_LOOKUP_PRELOAD;

  width = height = bobgui_icon_helper_get_size (self);

  icon = bobgui_icon_theme_lookup_by_gicon (icon_theme,
                                         gicon,
                                         MIN (width, height),
                                         scale,
                                         dir,
                                         flags);

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  *symbolic = bobgui_icon_paintable_is_symbolic (icon);
G_GNUC_END_IGNORE_DEPRECATIONS

  return GDK_PAINTABLE (icon);
}

static GdkPaintable *
bobgui_icon_helper_load_paintable (BobguiIconHelper   *self,
                                gboolean         preload,
                                gboolean        *out_symbolic)
{
  GdkPaintable *paintable;
  GIcon *gicon;
  gboolean symbolic;

  switch (bobgui_image_definition_get_storage_type (self->def))
    {
    case BOBGUI_IMAGE_PAINTABLE:
      paintable = g_object_ref (bobgui_image_definition_get_paintable (self->def));
      if (BOBGUI_IS_ICON_PAINTABLE (paintable))
        g_object_set (paintable, "scale", bobgui_widget_get_scale_factor (self->owner), NULL);
      symbolic = BOBGUI_IS_SYMBOLIC_PAINTABLE (paintable);
      break;

    case BOBGUI_IMAGE_ICON_NAME:
      if (self->use_fallback)
        gicon = g_themed_icon_new_with_default_fallbacks (bobgui_image_definition_get_icon_name (self->def));
      else
        gicon = g_themed_icon_new (bobgui_image_definition_get_icon_name (self->def));
      paintable = ensure_paintable_for_gicon (self,
                                              bobgui_css_node_get_style (self->node),
                                              bobgui_widget_get_scale_factor (self->owner),
                                              bobgui_widget_get_direction (self->owner),
                                              preload,
                                              gicon,
                                              &symbolic);
      g_object_unref (gicon);
      break;

    case BOBGUI_IMAGE_GICON:
      paintable = ensure_paintable_for_gicon (self,
                                              bobgui_css_node_get_style (self->node),
                                              bobgui_widget_get_scale_factor (self->owner),
                                              bobgui_widget_get_direction (self->owner),
                                              preload,
                                              bobgui_image_definition_get_gicon (self->def),
                                              &symbolic);
      break;

    case BOBGUI_IMAGE_EMPTY:
    default:
      paintable = NULL;
      symbolic = FALSE;
      break;
    }

  *out_symbolic = symbolic;

  return paintable;
}

static void
bobgui_icon_helper_ensure_paintable (BobguiIconHelper *self, gboolean preload)
{
  gboolean symbolic;

  if (self->paintable)
    return;

  self->paintable = bobgui_icon_helper_load_paintable (self, preload, &symbolic);
  self->texture_is_symbolic = symbolic;
}

static void
bobgui_icon_helper_paintable_snapshot (GdkPaintable *paintable,
                                    GdkSnapshot  *snapshot,
                                    double        width,
                                    double        height)
{
  BobguiIconHelper *self = BOBGUI_ICON_HELPER (paintable);
  BobguiCssStyle *style;
  double image_ratio, ratio;
  double x, y, w, h;

  style = bobgui_css_node_get_style (self->node);

  bobgui_icon_helper_ensure_paintable (self, FALSE);
  if (self->paintable == NULL)
    return;

  image_ratio = (double) width / height;
  ratio = gdk_paintable_get_intrinsic_aspect_ratio (self->paintable);
  if (ratio == 0)
    {
      w = MIN (width, bobgui_icon_helper_get_size (self));
      h = MIN (height, bobgui_icon_helper_get_size (self));
    }
  else if (ratio > image_ratio)
    {
      w = MIN (width, bobgui_icon_helper_get_size (self));
      h = w / ratio;
    }
  else
    {
      h = MIN (height, bobgui_icon_helper_get_size (self));
      w = h * ratio;
    }

  x = floor (width - ceil (w)) / 2;
  y = floor (height - ceil (h)) / 2;

  if (x != 0 || y != 0)
    {
      bobgui_snapshot_save (snapshot);
      bobgui_snapshot_translate (snapshot, &GRAPHENE_POINT_INIT (x, y));
      bobgui_css_style_snapshot_icon_paintable (style,
                                             snapshot,
                                             self->paintable,
                                             w, h);
      bobgui_snapshot_restore (snapshot);
    }
  else
    {
      bobgui_css_style_snapshot_icon_paintable (style,
                                             snapshot,
                                             self->paintable,
                                             w, h);
    }
}

static GdkPaintable *
bobgui_icon_helper_paintable_get_current_image (GdkPaintable *paintable)
{
  BobguiIconHelper *self = BOBGUI_ICON_HELPER (paintable);

  bobgui_icon_helper_ensure_paintable (self, FALSE);
  if (self->paintable == NULL)
    return NULL;

  return gdk_paintable_get_current_image (self->paintable);
}

static int
bobgui_icon_helper_paintable_get_intrinsic_width (GdkPaintable *paintable)
{
  BobguiIconHelper *self = BOBGUI_ICON_HELPER (paintable);

  return bobgui_icon_helper_get_size (self);
}

static int
bobgui_icon_helper_paintable_get_intrinsic_height (GdkPaintable *paintable)
{
  BobguiIconHelper *self = BOBGUI_ICON_HELPER (paintable);

  return bobgui_icon_helper_get_size (self);
}

static double bobgui_icon_helper_paintable_get_intrinsic_aspect_ratio (GdkPaintable *paintable)
{
  return 1.0;
};

static void
bobgui_icon_helper_paintable_init (GdkPaintableInterface *iface)
{
  iface->snapshot = bobgui_icon_helper_paintable_snapshot;
  iface->get_current_image = bobgui_icon_helper_paintable_get_current_image;
  iface->get_intrinsic_width = bobgui_icon_helper_paintable_get_intrinsic_width;
  iface->get_intrinsic_height = bobgui_icon_helper_paintable_get_intrinsic_height;
  iface->get_intrinsic_aspect_ratio = bobgui_icon_helper_paintable_get_intrinsic_aspect_ratio;
}

G_DEFINE_TYPE_WITH_CODE (BobguiIconHelper, bobgui_icon_helper, G_TYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE (GDK_TYPE_PAINTABLE,
						bobgui_icon_helper_paintable_init))

void
bobgui_icon_helper_invalidate (BobguiIconHelper *self)
{
  g_clear_object (&self->paintable);
  self->texture_is_symbolic = FALSE;

  bobgui_widget_queue_draw (self->owner);
}

void
bobgui_icon_helper_invalidate_for_change (BobguiIconHelper     *self,
                                       BobguiCssStyleChange *change)
{
  if (change == NULL ||
      bobgui_css_style_change_affects (change, BOBGUI_CSS_AFFECTS_ICON_TEXTURE |
                                            BOBGUI_CSS_AFFECTS_ICON_SIZE))
    {
      /* Avoid the queue_resize in bobgui_icon_helper_invalidate */
      g_clear_object (&self->paintable);
      self->texture_is_symbolic = FALSE;
      bobgui_widget_queue_draw (self->owner);
    }

  if (change == NULL ||
      bobgui_css_style_change_affects (change, BOBGUI_CSS_AFFECTS_ICON_SIZE))
    {
      bobgui_widget_queue_resize (self->owner);
    }
  else if (bobgui_css_style_change_affects (change, BOBGUI_CSS_AFFECTS_ICON_REDRAW) ||
           (self->texture_is_symbolic &&
            bobgui_css_style_change_affects (change, BOBGUI_CSS_AFFECTS_ICON_REDRAW_SYMBOLIC)))
    {
      bobgui_widget_queue_draw (self->owner);
    }

  /* The css size is valid now, preload */
  bobgui_icon_helper_ensure_paintable (self, TRUE);
}

static void
bobgui_icon_helper_take_definition (BobguiIconHelper      *self,
                                 BobguiImageDefinition *def)
{
  _bobgui_icon_helper_clear (self);

  if (def == NULL)
    return;

  bobgui_image_definition_unref (self->def);
  self->def = def;

  bobgui_icon_helper_invalidate (self);
}

void
_bobgui_icon_helper_clear (BobguiIconHelper *self)
{
  g_clear_object (&self->paintable);
  self->texture_is_symbolic = FALSE;

  if (bobgui_image_definition_get_storage_type (self->def) != BOBGUI_IMAGE_EMPTY)
    {
      bobgui_image_definition_unref (self->def);
      self->def = bobgui_image_definition_new_empty ();
      bobgui_icon_helper_invalidate (self);
    }
}

static void
bobgui_icon_helper_finalize (GObject *object)
{
  BobguiIconHelper *self = BOBGUI_ICON_HELPER (object);

  _bobgui_icon_helper_clear (self);
  g_signal_handlers_disconnect_by_func (self->owner, G_CALLBACK (bobgui_icon_helper_invalidate), self);
  bobgui_image_definition_unref (self->def);

  G_OBJECT_CLASS (bobgui_icon_helper_parent_class)->finalize (object);
}

void
bobgui_icon_helper_class_init (BobguiIconHelperClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->finalize = bobgui_icon_helper_finalize;
}

void
bobgui_icon_helper_init (BobguiIconHelper *self)
{
  self->def = bobgui_image_definition_new_empty ();
}

BobguiIconHelper *
bobgui_icon_helper_new (BobguiCssNode *css_node,
                     BobguiWidget  *owner)
{
  BobguiIconHelper *self;
  
  self = g_object_new (BOBGUI_TYPE_ICON_HELPER, NULL);

  self->pixel_size = -1;
  self->texture_is_symbolic = FALSE;

  self->node = css_node;
  self->owner = owner;
  g_signal_connect_swapped (owner, "direction-changed", G_CALLBACK (bobgui_icon_helper_invalidate), self);
  g_signal_connect_swapped (owner, "notify::scale-factor", G_CALLBACK (bobgui_icon_helper_invalidate), self);

  return self;
}

int
bobgui_icon_helper_get_size (BobguiIconHelper *self)
{
  BobguiCssStyle *style;

  if (self->pixel_size != -1)
    return self->pixel_size;

  style = bobgui_css_node_get_style (self->node);
  return bobgui_css_number_value_get (style->icon->icon_size, 100);
}

void
_bobgui_icon_helper_set_definition (BobguiIconHelper *self,
                                 BobguiImageDefinition *def)
{
  if (def)
    bobgui_icon_helper_take_definition (self, bobgui_image_definition_ref (def));
  else
    _bobgui_icon_helper_clear (self);
}

void
_bobgui_icon_helper_set_gicon (BobguiIconHelper *self,
                            GIcon         *gicon)
{
  bobgui_icon_helper_take_definition (self, bobgui_image_definition_new_gicon (gicon));
}

void
_bobgui_icon_helper_set_icon_name (BobguiIconHelper *self,
                                const char    *icon_name)
{
  bobgui_icon_helper_take_definition (self, bobgui_image_definition_new_icon_name (icon_name));
}

void
_bobgui_icon_helper_set_paintable (BobguiIconHelper *self,
			        GdkPaintable  *paintable)
{
  bobgui_icon_helper_take_definition (self, bobgui_image_definition_new_paintable (paintable));
}

gboolean
_bobgui_icon_helper_set_pixel_size (BobguiIconHelper *self,
                                 int            pixel_size)
{
  if (self->pixel_size != pixel_size)
    {
      self->pixel_size = pixel_size;
      bobgui_icon_helper_invalidate (self);
      return TRUE;
    }
  return FALSE;
}

gboolean
_bobgui_icon_helper_set_use_fallback (BobguiIconHelper *self,
                                   gboolean       use_fallback)
{
  if (self->use_fallback != use_fallback)
    {
      self->use_fallback = use_fallback;
      bobgui_icon_helper_invalidate (self);
      return TRUE;
    }
  return FALSE;
}

BobguiImageType
_bobgui_icon_helper_get_storage_type (BobguiIconHelper *self)
{
  return bobgui_image_definition_get_storage_type (self->def);
}

gboolean
_bobgui_icon_helper_get_use_fallback (BobguiIconHelper *self)
{
  return self->use_fallback;
}

int
_bobgui_icon_helper_get_pixel_size (BobguiIconHelper *self)
{
  return self->pixel_size;
}

BobguiImageDefinition *
bobgui_icon_helper_get_definition (BobguiIconHelper *self)
{
  return self->def;
}

GIcon *
_bobgui_icon_helper_peek_gicon (BobguiIconHelper *self)
{
  return bobgui_image_definition_get_gicon (self->def);
}

GdkPaintable *
_bobgui_icon_helper_peek_paintable (BobguiIconHelper *self)
{
  return bobgui_image_definition_get_paintable (self->def);
}

const char *
_bobgui_icon_helper_get_icon_name (BobguiIconHelper *self)
{
  return bobgui_image_definition_get_icon_name (self->def);
}

gboolean
_bobgui_icon_helper_get_is_empty (BobguiIconHelper *self)
{
  return bobgui_image_definition_get_storage_type (self->def) == BOBGUI_IMAGE_EMPTY;
}

void
bobgui_icon_size_set_style_classes (BobguiCssNode  *cssnode,
                                 BobguiIconSize  icon_size)
{
  struct {
    BobguiIconSize icon_size;
    const char *class_name;
  } class_names[] = {
    { BOBGUI_ICON_SIZE_NORMAL, "normal-icons" },
    { BOBGUI_ICON_SIZE_LARGE, "large-icons" }
  };
  guint i;

  for (i = 0; i < G_N_ELEMENTS (class_names); i++)
    {
      if (icon_size == class_names[i].icon_size)
        bobgui_css_node_add_class (cssnode, g_quark_from_static_string (class_names[i].class_name));
      else
        bobgui_css_node_remove_class (cssnode, g_quark_from_static_string (class_names[i].class_name));
    }
}
