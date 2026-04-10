/* BobguiIconTheme - a loader for icon themes
 * bobgui-icon-theme.c Copyright (C) 2002, 2003 Red Hat, Inc.
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

#include <glib.h>

#include "bobguiiconpaintableprivate.h"

#include "bobguiiconthemeprivate.h"
#include "bobguisnapshotprivate.h"
#include "bobguisymbolicpaintable.h"
#include "gdktextureutilsprivate.h"
#include "gdk/gdktextureprivate.h"
#include "gdk/gdkprofilerprivate.h"


/**
 * BobguiIconPaintable:
 *
 * Contains information found when looking up an icon in `BobguiIconTheme`
 * or loading it from a file.
 *
 * `BobguiIconPaintable` implements `GdkPaintable` and `BobguiSymbolicPaintable`.
 */

/* {{{ Utilities */

static inline gboolean
icon_uri_is_symbolic (const char *icon_name,
                      int         icon_name_len)
{
  if (g_str_has_suffix (icon_name, "-symbolic.svg") ||
      g_str_has_suffix (icon_name, ".symbolic.png") ||
      g_str_has_suffix (icon_name, "-symbolic-ltr.svg") ||
      g_str_has_suffix (icon_name, "-symbolic-rtl.svg"))
    return TRUE;

  return FALSE;
}

static void
bobgui_icon_paintable_set_file (BobguiIconPaintable *icon,
                             GFile            *file)
{
  if (!file)
    return;

  icon->loadable = G_LOADABLE_ICON (g_file_icon_new (file));
  icon->is_resource = g_file_has_uri_scheme (file, "resource");

  if (icon->is_resource)
    {
      char *uri = g_file_get_uri (file);
      icon->filename = g_strdup (uri + strlen ("resource://"));
      g_free (uri);
    }
  else
    {
      icon->filename = g_file_get_path (file);
    }

  icon->is_svg = g_str_has_suffix (icon->filename, ".svg");
  icon->is_symbolic = icon_uri_is_symbolic (icon->filename, -1);
}

static GFile *
new_resource_file (const char *filename)
{
  char *escaped = g_uri_escape_string (filename,
                                       G_URI_RESERVED_CHARS_ALLOWED_IN_PATH, FALSE);
  char *uri = g_strconcat ("resource://", escaped, NULL);
  GFile *file = g_file_new_for_uri (uri);

  g_free (escaped);
  g_free (uri);

  return file;
}

/* }}} */
/* {{{ Icon loading */

/* This function contains the complicated logic for deciding
 * on the size at which to load the icon and loading it at
 * that size.
 */
static void
icon_ensure_paintable__locked (BobguiIconPaintable *icon,
                               gboolean          in_thread)
{
  gint64 before;
  GError *load_error = NULL;

  icon_cache_mark_used_if_cached (icon);

  if (icon->paintable)
    return;

  before = GDK_PROFILER_CURRENT_TIME;

  if (icon->is_resource)
    {
      icon->paintable = gdk_paintable_new_from_resource (icon->filename);
    }
  else if (icon->filename)
    {
      icon->paintable = gdk_paintable_new_from_filename (icon->filename, &load_error);
    }
  else if (icon->loadable)
    {
      GInputStream *stream;
      int pixel_size = icon->desired_size * icon->desired_scale;
      stream = g_loadable_icon_load (icon->loadable, pixel_size, NULL, NULL, &load_error);
      if (stream)
        {
          icon->paintable = gdk_paintable_new_from_stream (stream, NULL, &load_error);
          g_object_unref (stream);
        }
    }
  else
    g_assert_not_reached ();

  if (!icon->paintable)
    {
      g_warning ("Failed to load icon %s: %s", icon->filename, load_error ? load_error->message : "");
      g_clear_error (&load_error);
      icon->paintable = GDK_PAINTABLE (gdk_texture_new_from_resource (IMAGE_MISSING_RESOURCE_PATH));
      g_set_str (&icon->icon_name, "image-missing");
      icon->is_symbolic = FALSE;
    }

  icon->width = gdk_paintable_get_intrinsic_width (icon->paintable);
  icon->height = gdk_paintable_get_intrinsic_height (icon->paintable);

  if (GDK_PROFILER_IS_RUNNING)
    {
      gint64 end = GDK_PROFILER_CURRENT_TIME;
      /* Don't report quick (< 0.5 msec) parses */
      if (end - before > 500000 || !in_thread)
        {
          gdk_profiler_add_markf (before, (end - before), in_thread ?  "Icon load (thread)" : "Icon load" ,
                                  "%s size %d@%d", icon->filename, icon->desired_size, icon->desired_scale);
        }
    }
}

static GdkPaintable *
bobgui_icon_paintable_ensure_paintable (BobguiIconPaintable *self)
{
  GdkPaintable *paintable = NULL;

  g_mutex_lock (&self->texture_lock);

  icon_ensure_paintable__locked (self, FALSE);

  paintable = self->paintable;

  g_mutex_unlock (&self->texture_lock);

  g_assert (paintable != NULL);

  return paintable;
}

/* }}} */
/* {{{ Recoloring by color matrix */

static void
init_color_matrix (graphene_matrix_t *color_matrix,
                   graphene_vec4_t   *color_offset,
                   const GdkRGBA     *foreground_color,
                   const GdkRGBA     *success_color,
                   const GdkRGBA     *warning_color,
                   const GdkRGBA     *error_color)
{
  const GdkRGBA fg_default = { 0.7450980392156863, 0.7450980392156863, 0.7450980392156863, 1.0};
  const GdkRGBA success_default = { 0.3046921492332342,0.6015716792553597, 0.023437857633325704, 1.0};
  const GdkRGBA warning_default = {0.9570458533607996, 0.47266346227206835, 0.2421911955443656, 1.0 };
  const GdkRGBA error_default = { 0.796887159533074, 0 ,0, 1.0 };
  const GdkRGBA *fg = foreground_color ? foreground_color : &fg_default;
  const GdkRGBA *sc = success_color ? success_color : &success_default;
  const GdkRGBA *wc = warning_color ? warning_color : &warning_default;
  const GdkRGBA *ec = error_color ? error_color : &error_default;
  
  graphene_matrix_init_from_float (color_matrix,
                                   (float[16]) {
                                     sc->red - fg->red, sc->green - fg->green, sc->blue - fg->blue, 0,
                                     wc->red - fg->red, wc->green - fg->green, wc->blue - fg->blue, 0,
                                     ec->red - fg->red, ec->green - fg->green, ec->blue - fg->blue, 0,
                                     0, 0, 0, fg->alpha
                                   });
  graphene_vec4_init (color_offset, fg->red, fg->green, fg->blue, 0);
}

/* }}} */
/* {{{ BobguiSymbolicPaintable implementation */

static void
bobgui_icon_paintable_snapshot_with_weight (BobguiSymbolicPaintable *paintable,
                                         BobguiSnapshot          *snapshot,
                                         double                width,
                                         double                height,
                                         const GdkRGBA        *colors,
                                         gsize                 n_colors,
                                         double                weight)
{
  BobguiIconPaintable *icon = BOBGUI_ICON_PAINTABLE (paintable);
  GdkPaintable *p;

  p = bobgui_icon_paintable_ensure_paintable (icon);

  bobgui_snapshot_push_debug (snapshot, "BobguiIconPaintable %s (%s)",
                           icon->filename ? icon->filename : icon->icon_name,
                           GDK_IS_TEXTURE (p) ? "texture" : "svg");

  if (BOBGUI_IS_SYMBOLIC_PAINTABLE (p))
    bobgui_symbolic_paintable_snapshot_with_weight (BOBGUI_SYMBOLIC_PAINTABLE (p),
                                                 snapshot,
                                                 width, height,
                                                 colors, n_colors,
                                                 weight);
  else if (GDK_IS_TEXTURE (p) && icon->is_symbolic)
    {
      graphene_matrix_t matrix;
      graphene_vec4_t offset;

      init_color_matrix (&matrix, &offset,
                         &colors[BOBGUI_SYMBOLIC_COLOR_FOREGROUND],
                         &colors[BOBGUI_SYMBOLIC_COLOR_SUCCESS],
                         &colors[BOBGUI_SYMBOLIC_COLOR_WARNING],
                         &colors[BOBGUI_SYMBOLIC_COLOR_ERROR]);

      bobgui_snapshot_push_color_matrix (snapshot, &matrix, &offset);
      gdk_paintable_snapshot (p, snapshot, width, height);
      bobgui_snapshot_pop (snapshot);
    }
  else
    gdk_paintable_snapshot (p, snapshot, width, height);

  bobgui_snapshot_pop (snapshot);
}

static void
bobgui_icon_paintable_snapshot_symbolic (BobguiSymbolicPaintable *paintable,
                                      BobguiSnapshot          *snapshot,
                                      double                width,
                                      double                height,
                                      const GdkRGBA        *colors,
                                      gsize                 n_colors)
{
  bobgui_icon_paintable_snapshot_with_weight (paintable, snapshot,
                                           width, height,
                                           colors, n_colors,
                                           400);
}

static void
icon_symbolic_paintable_init (BobguiSymbolicPaintableInterface *iface)
{
  iface->snapshot_with_weight = bobgui_icon_paintable_snapshot_with_weight;
  iface->snapshot_symbolic = bobgui_icon_paintable_snapshot_symbolic;
}

/* }}} */
/* {{{ GdkPaintable implementation */

static void
icon_paintable_snapshot (GdkPaintable *paintable,
                         BobguiSnapshot  *snapshot,
                         double        width,
                         double        height)
{
  bobgui_symbolic_paintable_snapshot_symbolic (BOBGUI_SYMBOLIC_PAINTABLE (paintable), snapshot, width, height, NULL, 0);
}

static GdkPaintableFlags
icon_paintable_get_flags (GdkPaintable *paintable)
{
  return GDK_PAINTABLE_STATIC_SIZE | GDK_PAINTABLE_STATIC_CONTENTS;
}

static int
icon_paintable_get_intrinsic_width (GdkPaintable *paintable)
{
  BobguiIconPaintable *icon = BOBGUI_ICON_PAINTABLE (paintable);

  return icon->desired_size;
}

static int
icon_paintable_get_intrinsic_height (GdkPaintable *paintable)
{
  BobguiIconPaintable *icon = BOBGUI_ICON_PAINTABLE (paintable);

  return icon->desired_size;
}

static void
icon_paintable_init (GdkPaintableInterface *iface)
{
  iface->snapshot = icon_paintable_snapshot;
  iface->get_flags = icon_paintable_get_flags;
  iface->get_intrinsic_width = icon_paintable_get_intrinsic_width;
  iface->get_intrinsic_height = icon_paintable_get_intrinsic_height;
}

/* }}} */
/* {{{ GObject boilerplate */

struct _BobguiIconPaintableClass
{
  GObjectClass parent_class;
};

enum
{
  PROP_0,
  PROP_FILE,
  PROP_ICON_NAME,
  PROP_IS_SYMBOLIC,
  PROP_SIZE,
  PROP_SCALE,
};

G_DEFINE_TYPE_WITH_CODE (BobguiIconPaintable, bobgui_icon_paintable, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GDK_TYPE_PAINTABLE,
                                                icon_paintable_init)
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_SYMBOLIC_PAINTABLE,
                                                icon_symbolic_paintable_init))

static void
bobgui_icon_paintable_init (BobguiIconPaintable *icon)
{
  g_mutex_init (&icon->texture_lock);

  icon->desired_size = 16;
  icon->desired_scale = 1;
}

static void
bobgui_icon_paintable_finalize (GObject *object)
{
  BobguiIconPaintable *icon = (BobguiIconPaintable *) object;

  icon_cache_remove (icon);

  g_strfreev (icon->key.icon_names);

  g_free (icon->filename);
  g_free (icon->icon_name);

  g_clear_object (&icon->loadable);
  g_clear_object (&icon->paintable);

  g_mutex_clear (&icon->texture_lock);

  G_OBJECT_CLASS (bobgui_icon_paintable_parent_class)->finalize (object);
}

static void
bobgui_icon_paintable_get_property (GObject    *object,
                                 guint       prop_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  BobguiIconPaintable *icon = BOBGUI_ICON_PAINTABLE (object);

  switch (prop_id)
    {
    case PROP_FILE:
      g_value_take_object (value, bobgui_icon_paintable_get_file (icon));
      break;

    case PROP_ICON_NAME:
      g_value_set_string (value, icon->icon_name);
      break;

    case PROP_IS_SYMBOLIC:
      g_value_set_boolean (value, icon->is_symbolic);
      break;

    case PROP_SIZE:
      g_value_set_int (value, icon->desired_size);
      break;

    case PROP_SCALE:
      g_value_set_int (value, icon->desired_scale);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
bobgui_icon_paintable_set_property (GObject      *object,
                                 guint         prop_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  BobguiIconPaintable *icon = BOBGUI_ICON_PAINTABLE (object);

  switch (prop_id)
    {
    case PROP_FILE:
      bobgui_icon_paintable_set_file (icon, g_value_get_object (value));
      break;

    case PROP_ICON_NAME:
      icon->icon_name = g_value_dup_string (value);
      break;

    case PROP_IS_SYMBOLIC:
      if (icon->is_symbolic != g_value_get_boolean (value))
        {
          icon->is_symbolic = g_value_get_boolean (value);
          g_object_notify_by_pspec (object, pspec);
        }
      break;

    case PROP_SIZE:
      if (icon->desired_size != g_value_get_int (value))
        {
          icon->desired_size = g_value_get_int (value);
          g_object_notify_by_pspec (object, pspec);
        }
      break;

    case PROP_SCALE:
      if (icon->desired_scale != g_value_get_int (value))
        {
          icon->desired_scale = g_value_get_int (value);
          g_object_notify_by_pspec (object, pspec);
        }
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}


static void
bobgui_icon_paintable_class_init (BobguiIconPaintableClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->get_property = bobgui_icon_paintable_get_property;
  gobject_class->set_property = bobgui_icon_paintable_set_property;
  gobject_class->finalize = bobgui_icon_paintable_finalize;

  /**
   * BobguiIconPaintable:file:
   *
   * The file representing the icon, if any.
   */
  g_object_class_install_property (gobject_class, PROP_FILE,
                                   g_param_spec_object ("file", NULL, NULL,
                                                        G_TYPE_FILE,
                                                        G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_NAME | G_PARAM_STATIC_BLURB | G_PARAM_STATIC_NICK));

  /**
   * BobguiIconPaintable:icon-name:
   *
   * The icon name that was chosen during lookup.
   *
   * Deprecated: 4.20
   */
  g_object_class_install_property (gobject_class, PROP_ICON_NAME,
                                   g_param_spec_string ("icon-name", NULL, NULL,
                                                        NULL,
                                                        G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_NAME | G_PARAM_DEPRECATED));

  /**
   * BobguiIconPaintable:is-symbolic: (getter is_symbolic)
   *
   * Whether the icon is symbolic or not.
   *
   * Deprecated: 4.20
   */
  g_object_class_install_property (gobject_class, PROP_IS_SYMBOLIC,
                                   g_param_spec_boolean ("is-symbolic", NULL, NULL,
                                                        FALSE,
                                                        G_PARAM_READWRITE | G_PARAM_STATIC_NAME | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_DEPRECATED));

  g_object_class_install_property (gobject_class, PROP_SIZE,
                                   g_param_spec_int ("size", NULL, NULL,
                                                     0, G_MAXINT, 16,
                                                     G_PARAM_READWRITE | G_PARAM_STATIC_NAME | G_PARAM_EXPLICIT_NOTIFY));

  g_object_class_install_property (gobject_class, PROP_SCALE,
                                   g_param_spec_int ("scale", NULL, NULL,
                                                     0, G_MAXINT, 1,
                                                     G_PARAM_READWRITE | G_PARAM_STATIC_NAME | G_PARAM_EXPLICIT_NOTIFY));
}

/* }}} */
/* {{{ Private API */

void
bobgui_icon_paintable_load_in_thread (BobguiIconPaintable *self)
{
  g_mutex_lock (&self->texture_lock);
  icon_ensure_paintable__locked (self, TRUE);
  g_mutex_unlock (&self->texture_lock);
}

void
bobgui_icon_paintable_set_icon_name (BobguiIconPaintable *icon,
                                  const char       *name)
{
  g_set_str (&icon->icon_name, name);
}

BobguiIconPaintable *
bobgui_icon_paintable_new_for_texture (GdkTexture *texture,
                                    int         desired_size,
                                    int         desired_scale)
{
  BobguiIconPaintable *icon;

  icon = g_object_new (BOBGUI_TYPE_ICON_PAINTABLE, NULL);
  icon->desired_size = desired_size;
  icon->desired_scale = desired_scale;
  icon->width = gdk_texture_get_width (texture);
  icon->height = gdk_texture_get_height (texture);
  icon->paintable = GDK_PAINTABLE (g_object_ref (texture));

  return icon;
}

BobguiIconPaintable *
bobgui_icon_paintable_new_for_path (const char *path,
                                 gboolean    is_resource,
                                 int         desired_size,
                                 int         desired_scale)
{
  BobguiIconPaintable *icon;

  icon = g_object_new (BOBGUI_TYPE_ICON_PAINTABLE, NULL);
  icon->desired_size = desired_size;
  icon->desired_scale = desired_scale;
  icon->filename = g_strdup (path);
  icon->is_resource = is_resource;
  icon->is_svg = g_str_has_suffix (icon->filename, ".svg");
  icon->is_symbolic = icon_uri_is_symbolic (icon->filename, -1);

  return icon;
}

BobguiIconPaintable *
bobgui_icon_paintable_new_for_loadable (GLoadableIcon *loadable,
                                     int            desired_size,
                                     int            desired_scale)
{
  BobguiIconPaintable *icon;

  icon = g_object_new (BOBGUI_TYPE_ICON_PAINTABLE, NULL);
  icon->desired_size = desired_size;
  icon->desired_scale = desired_scale;
  icon->loadable = g_object_ref (loadable);

  return icon;
}

/* }}} */
/* {{{ Public API */

/**
 * bobgui_icon_paintable_new_for_file:
 * @file: a `GFile`
 * @size: desired icon size, in application pixels
 * @scale: the desired scale
 *
 * Creates a `BobguiIconPaintable` for a file with a given size and scale.
 *
 * The icon can then be rendered by using it as a `GdkPaintable`.
 *
 * Returns: (transfer full): a `BobguiIconPaintable` containing
 *   for the icon. Unref with g_object_unref()
 */
BobguiIconPaintable *
bobgui_icon_paintable_new_for_file (GFile *file,
                                 int    size,
                                 int    scale)
{
  return g_object_new (BOBGUI_TYPE_ICON_PAINTABLE,
                       "file", file,
                       "size", size,
                       "scale", scale,
                       NULL);
}

/**
 * bobgui_icon_paintable_is_symbolic: (get-property is-symbolic)
 * @self: an icon paintable
 *
 * Checks if the icon is symbolic or not.
 *
 * This currently uses only the file name and not the file contents
 * for determining this. This behaviour may change in the future.
 *
 * Returns: true if the icon is symbolic, false otherwise
 *
 * Deprecated: 4.20
 */
gboolean
bobgui_icon_paintable_is_symbolic (BobguiIconPaintable *icon)
{
  g_return_val_if_fail (BOBGUI_IS_ICON_PAINTABLE (icon), FALSE);

  return icon->is_symbolic;
}

/**
 * bobgui_icon_paintable_get_icon_name:
 * @self: a `BobguiIconPaintable`
 *
 * Get the icon name being used for this icon.
 *
 * When an icon looked up in the icon theme was not available, the
 * icon theme may use fallback icons - either those specified to
 * bobgui_icon_theme_lookup_icon() or the always-available
 * "image-missing". The icon chosen is returned by this function.
 *
 * If the icon was created without an icon theme, this function
 * returns %NULL.
 *
 * Returns: (nullable) (type filename): the themed icon-name for the
 *   icon, or %NULL if its not a themed icon.
 *
 * Deprecated: 4.20
 */
const char *
bobgui_icon_paintable_get_icon_name (BobguiIconPaintable *icon)
{
  g_return_val_if_fail (icon != NULL, NULL);

  return icon->icon_name;
}

/**
 * bobgui_icon_paintable_get_file:
 * @self: a `BobguiIconPaintable`
 *
 * Gets the `GFile` that was used to load the icon.
 *
 * Returns %NULL if the icon was not loaded from a file.
 *
 * Returns: (nullable) (transfer full): the `GFile` for the icon
 */
GFile *
bobgui_icon_paintable_get_file (BobguiIconPaintable *icon)
{
  if (G_IS_FILE_ICON (icon->loadable))
    {
      return g_object_ref (g_file_icon_get_file (G_FILE_ICON (icon->loadable)));
    }
  else if (icon->filename)
    {
      if (icon->is_resource)
        return new_resource_file (icon->filename);
      else
        return g_file_new_for_path (icon->filename);
    }

  return NULL;
}

/* }}} */

/* vim:set foldmethod=marker: */

