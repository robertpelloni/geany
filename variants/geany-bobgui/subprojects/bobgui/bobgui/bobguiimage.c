/* BOBGUI - The Bobgui Framework
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
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

#include "bobguiimageprivate.h"

#include "bobguiiconhelperprivate.h"
#include "bobguiprivate.h"
#include "bobguisnapshot.h"
#include "bobguitypebuiltins.h"
#include "bobguiwidgetprivate.h"
#include "gdktextureutilsprivate.h"

#include <math.h>
#include <string.h>
#include <cairo-gobject.h>

/**
 * BobguiImage:
 *
 * Displays an image.
 *
 * <picture>
 *   <source srcset="image-dark.png" media="(prefers-color-scheme: dark)">
 *   <img alt="An example BobguiImage" src="image.png">
 * </picture>
 *
 * Various kinds of object can be displayed as an image; most typically,
 * you would load a `GdkTexture` from a file, using the convenience function
 * [ctor@Bobgui.Image.new_from_file], for instance:
 *
 * ```c
 * BobguiWidget *image = bobgui_image_new_from_file ("myfile.png");
 * ```
 *
 * If the file isn’t loaded successfully, the image will contain a
 * “broken image” icon similar to that used in many web browsers.
 *
 * If you want to handle errors in loading the file yourself, for example
 * by displaying an error message, then load the image with an image
 * loading framework such as libglycin, then create the `BobguiImage` with
 * [ctor@Bobgui.Image.new_from_paintable].
 *
 * Sometimes an application will want to avoid depending on external data
 * files, such as image files. See the documentation of `GResource` inside
 * GIO, for details. In this case, [property@Bobgui.Image:resource],
 * [ctor@Bobgui.Image.new_from_resource], and [method@Bobgui.Image.set_from_resource]
 * should be used.
 *
 * `BobguiImage` displays its image as an icon, with a size that is determined
 * by the application. See [class@Bobgui.Picture] if you want to show an image
 * at is actual size.
 *
 * ## CSS nodes
 *
 * `BobguiImage` has a single CSS node with the name `image`. The style classes
 * `.normal-icons` or `.large-icons` may appear, depending on the
 * [property@Bobgui.Image:icon-size] property.
 *
 * ## Accessibility
 *
 * `BobguiImage` uses the [enum@Bobgui.AccessibleRole.img] role.
 */

typedef struct _BobguiImageClass BobguiImageClass;

struct _BobguiImage
{
  BobguiWidget parent_instance;

  BobguiIconHelper *icon_helper;
  BobguiIconSize icon_size;

  float baseline_align;

  char *filename;
  char *resource_path;
};

struct _BobguiImageClass
{
  BobguiWidgetClass parent_class;
};


static void bobgui_image_snapshot             (BobguiWidget    *widget,
                                            BobguiSnapshot  *snapshot);
static void bobgui_image_unrealize            (BobguiWidget    *widget);
static void bobgui_image_measure (BobguiWidget      *widget,
                               BobguiOrientation  orientation,
                               int            for_size,
                               int           *minimum,
                               int           *natural,
                               int           *minimum_baseline,
                               int           *natural_baseline);

static void bobgui_image_css_changed          (BobguiWidget    *widget,
                                            BobguiCssStyleChange *change);
static void bobgui_image_system_setting_changed (BobguiWidget        *widget,
                                              BobguiSystemSetting  setting);
static void bobgui_image_finalize             (GObject      *object);

static void bobgui_image_set_property         (GObject      *object,
                                            guint         prop_id,
                                            const GValue *value,
                                            GParamSpec   *pspec);
static void bobgui_image_get_property         (GObject      *object,
                                            guint         prop_id,
                                            GValue       *value,
                                            GParamSpec   *pspec);
static void bobgui_image_clear_internal       (BobguiImage *self,
                                            gboolean  notify);

enum
{
  PROP_0,
  PROP_PAINTABLE,
  PROP_FILE,
  PROP_ICON_SIZE,
  PROP_PIXEL_SIZE,
  PROP_ICON_NAME,
  PROP_STORAGE_TYPE,
  PROP_GICON,
  PROP_RESOURCE,
  PROP_USE_FALLBACK,
  NUM_PROPERTIES
};

static GParamSpec *image_props[NUM_PROPERTIES] = { NULL, };

G_DEFINE_TYPE (BobguiImage, bobgui_image, BOBGUI_TYPE_WIDGET)

static void
bobgui_image_class_init (BobguiImageClass *class)
{
  GObjectClass *gobject_class;
  BobguiWidgetClass *widget_class;

  gobject_class = G_OBJECT_CLASS (class);

  gobject_class->set_property = bobgui_image_set_property;
  gobject_class->get_property = bobgui_image_get_property;
  gobject_class->finalize = bobgui_image_finalize;

  widget_class = BOBGUI_WIDGET_CLASS (class);
  widget_class->snapshot = bobgui_image_snapshot;
  widget_class->measure = bobgui_image_measure;
  widget_class->unrealize = bobgui_image_unrealize;
  widget_class->css_changed = bobgui_image_css_changed;
  widget_class->system_setting_changed = bobgui_image_system_setting_changed;

  /**
   * BobguiImage:paintable: (getter get_paintable) (setter set_from_paintable)
   *
   * The `GdkPaintable` to display.
   */
  image_props[PROP_PAINTABLE] =
      g_param_spec_object ("paintable", NULL, NULL,
                           GDK_TYPE_PAINTABLE,
                           BOBGUI_PARAM_READWRITE);

  /**
   * BobguiImage:file: (attributes org.bobgui.Property.set=bobgui_image_set_from_file)
   *
   * A path to the file to display.
   */
  image_props[PROP_FILE] =
      g_param_spec_string ("file", NULL, NULL,
                           NULL,
                           BOBGUI_PARAM_READWRITE);

  /**
   * BobguiImage:icon-size:
   *
   * The symbolic size to display icons at.
   */
  image_props[PROP_ICON_SIZE] =
      g_param_spec_enum ("icon-size", NULL, NULL,
                         BOBGUI_TYPE_ICON_SIZE,
                         BOBGUI_ICON_SIZE_INHERIT,
                         BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiImage:pixel-size:
   *
   * The size in pixels to display icons at.
   *
   * If set to a value != -1, this property overrides the
   * [property@Bobgui.Image:icon-size] property for images of type
   * `BOBGUI_IMAGE_ICON_NAME`.
   */
  image_props[PROP_PIXEL_SIZE] =
      g_param_spec_int ("pixel-size", NULL, NULL,
                        -1, G_MAXINT,
                        -1,
                        BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiImage:icon-name: (getter get_icon_name) (setter set_from_icon_name)
   *
   * The name of the icon in the icon theme.
   *
   * If the icon theme is changed, the image will be updated automatically.
   */
  image_props[PROP_ICON_NAME] =
      g_param_spec_string ("icon-name", NULL, NULL,
                           NULL,
                           BOBGUI_PARAM_READWRITE);

  /**
   * BobguiImage:gicon: (getter get_gicon) (setter set_from_gicon)
   *
   * The `GIcon` displayed in the BobguiImage.
   *
   * For themed icons, If the icon theme is changed, the image will be updated
   * automatically.
   */
  image_props[PROP_GICON] =
      g_param_spec_object ("gicon", NULL, NULL,
                           G_TYPE_ICON,
                           BOBGUI_PARAM_READWRITE);

  /**
   * BobguiImage:resource: (attributes org.bobgui.Property.set=bobgui_image_set_from_resource)
   *
   * A path to a resource file to display.
   */
  image_props[PROP_RESOURCE] =
      g_param_spec_string ("resource", NULL, NULL,
                           NULL,
                           BOBGUI_PARAM_READWRITE);

  /**
   * BobguiImage:storage-type:
   *
   * The representation being used for image data.
   */
  image_props[PROP_STORAGE_TYPE] =
      g_param_spec_enum ("storage-type", NULL, NULL,
                         BOBGUI_TYPE_IMAGE_TYPE,
                         BOBGUI_IMAGE_EMPTY,
                         BOBGUI_PARAM_READABLE);

  /**
   * BobguiImage:use-fallback:
   *
   * Whether the icon displayed in the `BobguiImage` will use
   * standard icon names fallback.
   *
   * The value of this property is only relevant for images of type
   * %BOBGUI_IMAGE_ICON_NAME and %BOBGUI_IMAGE_GICON.
   */
  image_props[PROP_USE_FALLBACK] =
      g_param_spec_boolean ("use-fallback", NULL, NULL,
                            FALSE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (gobject_class, NUM_PROPERTIES, image_props);

  bobgui_widget_class_set_css_name (widget_class, I_("image"));

  bobgui_widget_class_set_accessible_role (widget_class, BOBGUI_ACCESSIBLE_ROLE_IMG);
}

static void
bobgui_image_init (BobguiImage *image)
{
  BobguiCssNode *widget_node;

  widget_node = bobgui_widget_get_css_node (BOBGUI_WIDGET (image));

  image->icon_helper = bobgui_icon_helper_new (widget_node, BOBGUI_WIDGET (image));
}

static void
bobgui_image_finalize (GObject *object)
{
  BobguiImage *image = BOBGUI_IMAGE (object);

  bobgui_image_clear_internal (image, FALSE);

  g_clear_object (&image->icon_helper);

  g_free (image->filename);
  g_free (image->resource_path);

  G_OBJECT_CLASS (bobgui_image_parent_class)->finalize (object);
};

static void
bobgui_image_set_property (GObject      *object,
			guint         prop_id,
			const GValue *value,
			GParamSpec   *pspec)
{
  BobguiImage *image = BOBGUI_IMAGE (object);

  switch (prop_id)
    {
    case PROP_PAINTABLE:
      bobgui_image_set_from_paintable (image, g_value_get_object (value));
      break;
    case PROP_FILE:
      bobgui_image_set_from_file (image, g_value_get_string (value));
      break;
    case PROP_ICON_SIZE:
      bobgui_image_set_icon_size (image, g_value_get_enum (value));
      break;
    case PROP_PIXEL_SIZE:
      bobgui_image_set_pixel_size (image, g_value_get_int (value));
      break;
    case PROP_ICON_NAME:
      bobgui_image_set_from_icon_name (image, g_value_get_string (value));
      break;
    case PROP_GICON:
      bobgui_image_set_from_gicon (image, g_value_get_object (value));
      break;
    case PROP_RESOURCE:
      bobgui_image_set_from_resource (image, g_value_get_string (value));
      break;

    case PROP_USE_FALLBACK:
      if (_bobgui_icon_helper_set_use_fallback (image->icon_helper, g_value_get_boolean (value)))
        g_object_notify_by_pspec (object, pspec);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_image_get_property (GObject     *object,
			guint        prop_id,
			GValue      *value,
			GParamSpec  *pspec)
{
  BobguiImage *image = BOBGUI_IMAGE (object);

  switch (prop_id)
    {
    case PROP_PAINTABLE:
      g_value_set_object (value, _bobgui_icon_helper_peek_paintable (image->icon_helper));
      break;
    case PROP_FILE:
      g_value_set_string (value, image->filename);
      break;
    case PROP_ICON_SIZE:
      g_value_set_enum (value, image->icon_size);
      break;
    case PROP_PIXEL_SIZE:
      g_value_set_int (value, _bobgui_icon_helper_get_pixel_size (image->icon_helper));
      break;
    case PROP_ICON_NAME:
      g_value_set_string (value, _bobgui_icon_helper_get_icon_name (image->icon_helper));
      break;
    case PROP_GICON:
      g_value_set_object (value, _bobgui_icon_helper_peek_gicon (image->icon_helper));
      break;
    case PROP_RESOURCE:
      g_value_set_string (value, image->resource_path);
      break;
    case PROP_USE_FALLBACK:
      g_value_set_boolean (value, _bobgui_icon_helper_get_use_fallback (image->icon_helper));
      break;
    case PROP_STORAGE_TYPE:
      g_value_set_enum (value, _bobgui_icon_helper_get_storage_type (image->icon_helper));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}


/**
 * bobgui_image_new_from_file:
 * @filename: (type filename): a filename
 *
 * Creates a new `BobguiImage` displaying the file @filename.
 *
 * If the file isn’t found or can’t be loaded, the resulting `BobguiImage`
 * will display a “broken image” icon. This function never returns %NULL,
 * it always returns a valid `BobguiImage` widget.
 *
 * If you need to detect failures to load the file, use an
 * image loading framework such as libglycin to load the file
 * yourself, then create the `BobguiImage` from the texture.
 *
 * The storage type (see [method@Bobgui.Image.get_storage_type])
 * of the returned image is not defined, it will be whatever
 * is appropriate for displaying the file.
 *
 * Returns: a new `BobguiImage`
 */
BobguiWidget*
bobgui_image_new_from_file   (const char *filename)
{
  BobguiImage *image;

  image = g_object_new (BOBGUI_TYPE_IMAGE, NULL);

  bobgui_image_set_from_file (image, filename);

  return BOBGUI_WIDGET (image);
}

/**
 * bobgui_image_new_from_resource:
 * @resource_path: a resource path
 *
 * Creates a new `BobguiImage` displaying the resource file @resource_path.
 *
 * If the file isn’t found or can’t be loaded, the resulting `BobguiImage` will
 * display a “broken image” icon. This function never returns %NULL,
 * it always returns a valid `BobguiImage` widget.
 *
 * If you need to detect failures to load the file, use an
 * image loading framework such as libglycin to load the file
 * yourself, then create the `BobguiImage` from the texture.
 *
 * The storage type (see [method@Bobgui.Image.get_storage_type]) of
 * the returned image is not defined, it will be whatever is
 * appropriate for displaying the file.
 *
 * Returns: a new `BobguiImage`
 */
BobguiWidget*
bobgui_image_new_from_resource (const char *resource_path)
{
  BobguiImage *image;

  image = g_object_new (BOBGUI_TYPE_IMAGE, NULL);

  bobgui_image_set_from_resource (image, resource_path);

  return BOBGUI_WIDGET (image);
}

/**
 * bobgui_image_new_from_pixbuf:
 * @pixbuf: (nullable): a `GdkPixbuf`
 *
 * Creates a new `BobguiImage` displaying @pixbuf.
 *
 * The `BobguiImage` does not assume a reference to the pixbuf; you still
 * need to unref it if you own references. `BobguiImage` will add its own
 * reference rather than adopting yours.
 *
 * This is a helper for [ctor@Bobgui.Image.new_from_paintable], and you can't
 * get back the exact pixbuf once this is called, only a texture.
 *
 * Note that this function just creates an `BobguiImage` from the pixbuf.
 * The `BobguiImage` created will not react to state changes. Should you
 * want that, you should use [ctor@Bobgui.Image.new_from_icon_name].
 *
 * Returns: a new `BobguiImage`
 *
 * Deprecated: 4.12: Use [ctor@Bobgui.Image.new_from_paintable] and
 *   [ctor@Gdk.Texture.new_for_pixbuf] instead
 */
BobguiWidget*
bobgui_image_new_from_pixbuf (GdkPixbuf *pixbuf)
{
  BobguiImage *image;

  image = g_object_new (BOBGUI_TYPE_IMAGE, NULL);

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  bobgui_image_set_from_pixbuf (image, pixbuf);
G_GNUC_END_IGNORE_DEPRECATIONS

  return BOBGUI_WIDGET (image);
}

/**
 * bobgui_image_new_from_paintable:
 * @paintable: (nullable): a `GdkPaintable`
 *
 * Creates a new `BobguiImage` displaying @paintable.
 *
 * The `BobguiImage` does not assume a reference to the paintable; you still
 * need to unref it if you own references. `BobguiImage` will add its own
 * reference rather than adopting yours.
 *
 * The `BobguiImage` will track changes to the @paintable and update
 * its size and contents in response to it.
 *
 * Note that paintables are still subject to the icon size that is
 * set on the image. If you want to display a paintable at its intrinsic
 * size, use [class@Bobgui.Picture] instead.
 *
 * If @paintable is a [iface@Bobgui.SymbolicPaintable], then it will be
 * recolored with the symbolic palette from the theme.
 *
 * Returns: a new `BobguiImage`
 */
BobguiWidget*
bobgui_image_new_from_paintable (GdkPaintable *paintable)
{
  BobguiImage *image;

  image = g_object_new (BOBGUI_TYPE_IMAGE, NULL);

  bobgui_image_set_from_paintable (image, paintable);

  return BOBGUI_WIDGET (image);
}

/**
 * bobgui_image_new_from_icon_name:
 * @icon_name: (nullable): an icon name
 *
 * Creates a `BobguiImage` displaying an icon from the current icon theme.
 *
 * If the icon name isn’t known, a “broken image” icon will be
 * displayed instead. If the current icon theme is changed, the icon
 * will be updated appropriately.
 *
 * Returns: a new `BobguiImage` displaying the themed icon
 */
BobguiWidget*
bobgui_image_new_from_icon_name (const char *icon_name)
{
  BobguiImage *image;

  image = g_object_new (BOBGUI_TYPE_IMAGE, NULL);

  bobgui_image_set_from_icon_name (image, icon_name);

  return BOBGUI_WIDGET (image);
}

/**
 * bobgui_image_new_from_gicon:
 * @icon: an icon
 *
 * Creates a `BobguiImage` displaying an icon from the current icon theme.
 *
 * If the icon name isn’t known, a “broken image” icon will be
 * displayed instead. If the current icon theme is changed, the icon
 * will be updated appropriately.
 *
 * Returns: a new `BobguiImage` displaying the themed icon
 */
BobguiWidget*
bobgui_image_new_from_gicon (GIcon *icon)
{
  BobguiImage *image;

  image = g_object_new (BOBGUI_TYPE_IMAGE, NULL);

  bobgui_image_set_from_gicon (image, icon);

  return BOBGUI_WIDGET (image);
}

/**
 * bobgui_image_set_from_file: (set-property file)
 * @image: a `BobguiImage`
 * @filename: (type filename) (nullable): a filename
 *
 * Sets a `BobguiImage` to show a file.
 *
 * See [ctor@Bobgui.Image.new_from_file] for details.
 *
 * ::: warning
 *     Note that this function should not be used with untrusted data.
 *     Use a proper image loading framework such as libglycin, which can
 *     load many image formats into a `GdkTexture`, and then use
 *     [method@Bobgui.Image.set_from_paintable].
 */
void
bobgui_image_set_from_file (BobguiImage    *image,
                         const char *filename)
{
  GdkPaintable *paintable;

  g_return_if_fail (BOBGUI_IS_IMAGE (image));

  g_object_freeze_notify (G_OBJECT (image));

  bobgui_image_clear (image);

  if (filename == NULL)
    {
      image->filename = NULL;
      g_object_thaw_notify (G_OBJECT (image));
      return;
    }

  paintable = gdk_paintable_new_from_filename (filename, NULL);

  if (paintable == NULL)
    {
      bobgui_image_set_from_icon_name (image, "image-missing");
      g_object_thaw_notify (G_OBJECT (image));
      return;
    }

  bobgui_image_set_from_paintable (image, paintable);

  g_object_unref (paintable);

  image->filename = g_strdup (filename);

  g_object_thaw_notify (G_OBJECT (image));
}

#ifndef GDK_PIXBUF_MAGIC_NUMBER
#define GDK_PIXBUF_MAGIC_NUMBER (0x47646b50)    /* 'GdkP' */
#endif

static gboolean
resource_is_pixdata (const char *resource_path)
{
  const guint8 *stream;
  guint32 magic;
  gsize data_size;
  GBytes *bytes;
  gboolean ret = FALSE;

  bytes = g_resources_lookup_data (resource_path, 0, NULL);
  if (bytes == NULL)
    return FALSE;

  stream = g_bytes_get_data (bytes, &data_size);
  if (data_size < sizeof(guint32))
    goto out;

  magic = (((guint32)(stream[0])) << 24) | (((guint32)(stream[1])) << 16) | (((guint32)(stream[2])) << 8) | (guint32)(stream[3]);
  if (magic == GDK_PIXBUF_MAGIC_NUMBER)
    ret = TRUE;

out:
  g_bytes_unref (bytes);
  return ret;
}

/**
 * bobgui_image_set_from_resource: (set-property resource)
 * @image: a `BobguiImage`
 * @resource_path: (nullable): a resource path
 *
 * Sets a `BobguiImage` to show a resource.
 *
 * See [ctor@Bobgui.Image.new_from_resource] for details.
 */
void
bobgui_image_set_from_resource (BobguiImage   *image,
                             const char *resource_path)
{
  GdkPaintable *paintable;

  g_return_if_fail (BOBGUI_IS_IMAGE (image));

  g_object_freeze_notify (G_OBJECT (image));

  bobgui_image_clear (image);

  if (resource_path == NULL)
    {
      g_object_thaw_notify (G_OBJECT (image));
      return;
    }

  if (resource_is_pixdata (resource_path))
    {
      g_warning ("GdkPixdata format images are not supported, remove the \"to-pixdata\" option from your GResource files");
      paintable = NULL;
    }
  else
    {
      paintable = gdk_paintable_new_from_resource (resource_path);
    }

  if (paintable == NULL)
    {
      bobgui_image_set_from_icon_name (image, "image-missing");
      g_object_thaw_notify (G_OBJECT (image));
      return;
    }

  bobgui_image_set_from_paintable (image, paintable);

  g_object_unref (paintable);

  image->resource_path = g_strdup (resource_path);

  g_object_notify_by_pspec (G_OBJECT (image), image_props[PROP_RESOURCE]);

  g_object_thaw_notify (G_OBJECT (image));
}


/**
 * bobgui_image_set_from_pixbuf:
 * @image: a `BobguiImage`
 * @pixbuf: (nullable): a `GdkPixbuf` or `NULL`
 *
 * Sets a `BobguiImage` to show a `GdkPixbuf`.
 *
 * See [ctor@Bobgui.Image.new_from_pixbuf] for details.
 *
 * Note: This is a helper for [method@Bobgui.Image.set_from_paintable],
 * and you can't get back the exact pixbuf once this is called,
 * only a paintable.
 *
 * Deprecated: 4.12: Use [method@Bobgui.Image.set_from_paintable] instead
 */
void
bobgui_image_set_from_pixbuf (BobguiImage  *image,
                           GdkPixbuf *pixbuf)
{
  GdkTexture *texture;

  g_return_if_fail (BOBGUI_IS_IMAGE (image));
  g_return_if_fail (pixbuf == NULL || GDK_IS_PIXBUF (pixbuf));

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  if (pixbuf)
    texture = gdk_texture_new_for_pixbuf (pixbuf);
  else
    texture = NULL;
G_GNUC_END_IGNORE_DEPRECATIONS

  bobgui_image_set_from_paintable (image, GDK_PAINTABLE (texture));

  if (texture)
    g_object_unref (texture);
}

/**
 * bobgui_image_set_from_icon_name: (set-property icon-name)
 * @image: a `BobguiImage`
 * @icon_name: (nullable): an icon name
 *
 * Sets a `BobguiImage` to show a named icon.
 *
 * See [ctor@Bobgui.Image.new_from_icon_name] for details.
 */
void
bobgui_image_set_from_icon_name  (BobguiImage    *image,
			       const char *icon_name)
{
  g_return_if_fail (BOBGUI_IS_IMAGE (image));

  g_object_freeze_notify (G_OBJECT (image));

  bobgui_image_clear (image);

  if (icon_name)
    _bobgui_icon_helper_set_icon_name (image->icon_helper, icon_name);

  g_object_notify_by_pspec (G_OBJECT (image), image_props[PROP_ICON_NAME]);
  g_object_notify_by_pspec (G_OBJECT (image), image_props[PROP_STORAGE_TYPE]);

  g_object_thaw_notify (G_OBJECT (image));
}

/**
 * bobgui_image_set_from_gicon: (set-property gicon)
 * @image: a `BobguiImage`
 * @icon: an icon
 *
 * Sets a `BobguiImage` to show a `GIcon`.
 *
 * See [ctor@Bobgui.Image.new_from_gicon] for details.
 */
void
bobgui_image_set_from_gicon  (BobguiImage       *image,
			   GIcon          *icon)
{
  g_return_if_fail (BOBGUI_IS_IMAGE (image));

  g_object_freeze_notify (G_OBJECT (image));

  if (icon)
    g_object_ref (icon);

  bobgui_image_clear (image);

  if (icon)
    {
      _bobgui_icon_helper_set_gicon (image->icon_helper, icon);
      g_object_unref (icon);
    }

  g_object_notify_by_pspec (G_OBJECT (image), image_props[PROP_GICON]);
  g_object_notify_by_pspec (G_OBJECT (image), image_props[PROP_STORAGE_TYPE]);

  g_object_thaw_notify (G_OBJECT (image));
}

static void
bobgui_image_paintable_invalidate_contents (GdkPaintable *paintable,
                                         BobguiImage     *image)
{
  bobgui_widget_queue_draw (BOBGUI_WIDGET (image));
}

static void
bobgui_image_paintable_invalidate_size (GdkPaintable *paintable,
                                     BobguiImage     *image)
{
  bobgui_icon_helper_invalidate (image->icon_helper);
}

/**
 * bobgui_image_set_from_paintable: (set-property paintable)
 * @image: a `BobguiImage`
 * @paintable: (nullable): a `GdkPaintable`
 *
 * Sets a `BobguiImage` to show a `GdkPaintable`.
 *
 * See [ctor@Bobgui.Image.new_from_paintable] for details.
 */
void
bobgui_image_set_from_paintable (BobguiImage     *image,
			      GdkPaintable *paintable)
{
  g_return_if_fail (BOBGUI_IS_IMAGE (image));
  g_return_if_fail (paintable == NULL || GDK_IS_PAINTABLE (paintable));

  g_object_freeze_notify (G_OBJECT (image));

  if (paintable)
    g_object_ref (paintable);

  bobgui_image_clear (image);

  if (paintable)
    {
      const guint flags = gdk_paintable_get_flags (paintable);

      _bobgui_icon_helper_set_paintable (image->icon_helper, paintable);

      if ((flags & GDK_PAINTABLE_STATIC_CONTENTS) == 0)
        g_signal_connect (paintable,
                          "invalidate-contents",
                          G_CALLBACK (bobgui_image_paintable_invalidate_contents),
                          image);

      if ((flags & GDK_PAINTABLE_STATIC_SIZE) == 0)
        g_signal_connect (paintable,
                          "invalidate-size",
                          G_CALLBACK (bobgui_image_paintable_invalidate_size),
                          image);
      g_object_unref (paintable);
    }

  g_object_notify_by_pspec (G_OBJECT (image), image_props[PROP_PAINTABLE]);
  g_object_notify_by_pspec (G_OBJECT (image), image_props[PROP_STORAGE_TYPE]);

  g_object_thaw_notify (G_OBJECT (image));
}

/**
 * bobgui_image_get_storage_type:
 * @image: a `BobguiImage`
 *
 * Gets the type of representation being used by the `BobguiImage`
 * to store image data.
 *
 * If the `BobguiImage` has no image data, the return value will
 * be %BOBGUI_IMAGE_EMPTY.
 *
 * Returns: image representation being used
 */
BobguiImageType
bobgui_image_get_storage_type (BobguiImage *image)
{
  g_return_val_if_fail (BOBGUI_IS_IMAGE (image), BOBGUI_IMAGE_EMPTY);

  return _bobgui_icon_helper_get_storage_type (image->icon_helper);
}

/**
 * bobgui_image_get_paintable:
 * @image: a `BobguiImage`
 *
 * Gets the image `GdkPaintable` being displayed by the `BobguiImage`.
 *
 * The storage type of the image must be %BOBGUI_IMAGE_EMPTY or
 * %BOBGUI_IMAGE_PAINTABLE (see [method@Bobgui.Image.get_storage_type]).
 * The caller of this function does not own a reference to the
 * returned paintable.
 *
 * Returns: (nullable) (transfer none): the displayed paintable
 */
GdkPaintable *
bobgui_image_get_paintable (BobguiImage *image)
{
  g_return_val_if_fail (BOBGUI_IS_IMAGE (image), NULL);

  return _bobgui_icon_helper_peek_paintable (image->icon_helper);
}

/**
 * bobgui_image_get_icon_name:
 * @image: a `BobguiImage`
 *
 * Gets the icon name and size being displayed by the `BobguiImage`.
 *
 * The storage type of the image must be %BOBGUI_IMAGE_EMPTY or
 * %BOBGUI_IMAGE_ICON_NAME (see [method@Bobgui.Image.get_storage_type]).
 * The returned string is owned by the `BobguiImage` and should not
 * be freed.
 *
 * Returns: (transfer none) (nullable): the icon name
 */
const char *
bobgui_image_get_icon_name (BobguiImage *image)
{
  g_return_val_if_fail (BOBGUI_IS_IMAGE (image), NULL);

  return _bobgui_icon_helper_get_icon_name (image->icon_helper);
}

/**
 * bobgui_image_get_gicon:
 * @image: a `BobguiImage`
 *
 * Gets the `GIcon` being displayed by the `BobguiImage`.
 *
 * The storage type of the image must be %BOBGUI_IMAGE_EMPTY or
 * %BOBGUI_IMAGE_GICON (see [method@Bobgui.Image.get_storage_type]).
 * The caller of this function does not own a reference to the
 * returned `GIcon`.
 *
 * Returns: (transfer none) (nullable): a `GIcon`
 **/
GIcon *
bobgui_image_get_gicon (BobguiImage *image)
{
  g_return_val_if_fail (BOBGUI_IS_IMAGE (image), NULL);

  return _bobgui_icon_helper_peek_gicon (image->icon_helper);
}

/**
 * bobgui_image_new:
 *
 * Creates a new empty `BobguiImage` widget.
 *
 * Returns: a newly created `BobguiImage` widget.
 */
BobguiWidget*
bobgui_image_new (void)
{
  return g_object_new (BOBGUI_TYPE_IMAGE, NULL);
}

static void
bobgui_image_unrealize (BobguiWidget *widget)
{
  BobguiImage *image = BOBGUI_IMAGE (widget);

  bobgui_icon_helper_invalidate (image->icon_helper);

  BOBGUI_WIDGET_CLASS (bobgui_image_parent_class)->unrealize (widget);
}

static float
bobgui_image_get_baseline_align (BobguiImage *image)
{
  PangoContext *pango_context;
  PangoFontMetrics *metrics;

  if (image->baseline_align == 0.0)
    {
      pango_context = bobgui_widget_get_pango_context (BOBGUI_WIDGET (image));
      metrics = pango_context_get_metrics (pango_context, NULL, NULL);
      image->baseline_align =
                (float)pango_font_metrics_get_ascent (metrics) /
                (pango_font_metrics_get_ascent (metrics) + pango_font_metrics_get_descent (metrics));

      pango_font_metrics_unref (metrics);
    }

  return image->baseline_align;
}

static void
bobgui_image_snapshot (BobguiWidget   *widget,
                    BobguiSnapshot *snapshot)
{
  BobguiImage *image = BOBGUI_IMAGE (widget);
  double ratio;
  int x, y, width, height, baseline;
  double w, h;

  width = bobgui_widget_get_width (widget);
  height = bobgui_widget_get_height (widget);
  ratio = gdk_paintable_get_intrinsic_aspect_ratio (GDK_PAINTABLE (image->icon_helper));

  bobgui_snapshot_push_isolation (snapshot, GSK_ISOLATION_ALL);

  if (ratio == 0)
    {
      gdk_paintable_snapshot (GDK_PAINTABLE (image->icon_helper), snapshot, width, height);
    }
  else
    {
      double image_ratio = (double) width / height;

      if (ratio > image_ratio)
        {
          w = width;
          h = width / ratio;
        }
      else
        {
          w = height * ratio;
          h = height;
        }

      x = (width - ceil (w)) / 2;

      baseline = bobgui_widget_get_baseline (widget);
      if (baseline == -1)
        y = floor(height - ceil (h)) / 2;
      else
        y = CLAMP (baseline - floor (ceil (h) * bobgui_image_get_baseline_align (image)), 0, height - ceil (h));

      if (x != 0 || y != 0)
        {
          bobgui_snapshot_save (snapshot);
          bobgui_snapshot_translate (snapshot, &GRAPHENE_POINT_INIT (x, y));
          gdk_paintable_snapshot (GDK_PAINTABLE (image->icon_helper), snapshot, w, h);
          bobgui_snapshot_restore (snapshot);
        }
      else
        {
          gdk_paintable_snapshot (GDK_PAINTABLE (image->icon_helper), snapshot, w, h);
        }
    }

  bobgui_snapshot_pop (snapshot); /* isolation */
}

static void
bobgui_image_notify_for_storage_type (BobguiImage     *image,
                                   BobguiImageType  storage_type)
{
  switch (storage_type)
    {
    case BOBGUI_IMAGE_ICON_NAME:
      g_object_notify_by_pspec (G_OBJECT (image), image_props[PROP_ICON_NAME]);
      break;
    case BOBGUI_IMAGE_GICON:
      g_object_notify_by_pspec (G_OBJECT (image), image_props[PROP_GICON]);
      break;
    case BOBGUI_IMAGE_PAINTABLE:
      g_object_notify_by_pspec (G_OBJECT (image), image_props[PROP_PAINTABLE]);
      break;
    case BOBGUI_IMAGE_EMPTY:
    default:
      break;
    }
}

void
bobgui_image_set_from_definition (BobguiImage           *image,
                               BobguiImageDefinition *def)
{
  g_return_if_fail (BOBGUI_IS_IMAGE (image));

  g_object_freeze_notify (G_OBJECT (image));

  bobgui_image_clear (image);

  if (def != NULL)
    {
      _bobgui_icon_helper_set_definition (image->icon_helper, def);

      bobgui_image_notify_for_storage_type (image, bobgui_image_definition_get_storage_type (def));
    }

  g_object_notify_by_pspec (G_OBJECT (image), image_props[PROP_STORAGE_TYPE]);

  g_object_thaw_notify (G_OBJECT (image));
}

BobguiImageDefinition *
bobgui_image_get_definition (BobguiImage *image)
{
  return bobgui_icon_helper_get_definition (image->icon_helper);
}

static void
bobgui_image_clear_internal (BobguiImage *self,
                          gboolean  notify)
{
  BobguiImageType storage_type = bobgui_image_get_storage_type (self);
  GObject *gobject = G_OBJECT (self);

  if (notify)
    {
      if (storage_type != BOBGUI_IMAGE_EMPTY)
        g_object_notify_by_pspec (gobject, image_props[PROP_STORAGE_TYPE]);

      g_object_notify_by_pspec (gobject, image_props[PROP_ICON_SIZE]);

      bobgui_image_notify_for_storage_type (self, storage_type);
    }

  if (self->filename)
    {
      g_free (self->filename);
      self->filename = NULL;

      if (notify)
        g_object_notify_by_pspec (gobject, image_props[PROP_FILE]);
    }

  if (self->resource_path)
    {
      g_free (self->resource_path);
      self->resource_path = NULL;

      if (notify)
        g_object_notify_by_pspec (gobject, image_props[PROP_RESOURCE]);
    }

  if (storage_type == BOBGUI_IMAGE_PAINTABLE)
    {
      GdkPaintable *paintable = _bobgui_icon_helper_peek_paintable (self->icon_helper);
      const guint flags = gdk_paintable_get_flags (paintable);

      if ((flags & GDK_PAINTABLE_STATIC_CONTENTS) == 0)
        g_signal_handlers_disconnect_by_func (paintable,
                                              bobgui_image_paintable_invalidate_contents,
                                              self);

      if ((flags & GDK_PAINTABLE_STATIC_SIZE) == 0)
        g_signal_handlers_disconnect_by_func (paintable,
                                              bobgui_image_paintable_invalidate_size,
                                              self);
    }

  _bobgui_icon_helper_clear (self->icon_helper);
}

/**
 * bobgui_image_clear:
 * @image: a `BobguiImage`
 *
 * Resets the image to be empty.
 */
void
bobgui_image_clear (BobguiImage *image)
{
  g_return_if_fail (BOBGUI_IS_IMAGE (image));

  g_object_freeze_notify (G_OBJECT (image));
  bobgui_image_clear_internal (image, TRUE);
  g_object_thaw_notify (G_OBJECT (image));
}

static void
bobgui_image_measure (BobguiWidget      *widget,
                   BobguiOrientation  orientation,
                   int            for_size,
                   int           *minimum,
                   int           *natural,
                   int           *minimum_baseline,
                   int           *natural_baseline)
{
  BobguiImage *image = BOBGUI_IMAGE (widget);
  float baseline_align;

  *minimum = *natural = bobgui_icon_helper_get_size (image->icon_helper);

  if (orientation == BOBGUI_ORIENTATION_VERTICAL)
    {
      baseline_align = bobgui_image_get_baseline_align (BOBGUI_IMAGE (widget));
      *minimum_baseline = *minimum * baseline_align;
      *natural_baseline = *natural * baseline_align;
    }
}

static void
bobgui_image_css_changed (BobguiWidget         *widget,
                       BobguiCssStyleChange *change)
{
  BobguiImage *image = BOBGUI_IMAGE (widget);

  bobgui_icon_helper_invalidate_for_change (image->icon_helper, change);

  BOBGUI_WIDGET_CLASS (bobgui_image_parent_class)->css_changed (widget, change);

  image->baseline_align = 0.0;
}

static void
bobgui_image_system_setting_changed (BobguiWidget        *widget,
                                  BobguiSystemSetting  setting)
{
  BobguiImage *image = BOBGUI_IMAGE (widget);

  if (setting == BOBGUI_SYSTEM_SETTING_ICON_THEME)
    bobgui_icon_helper_invalidate (image->icon_helper);

  BOBGUI_WIDGET_CLASS (bobgui_image_parent_class)->system_setting_changed (widget, setting);
}

/**
 * bobgui_image_set_pixel_size:
 * @image: a `BobguiImage`
 * @pixel_size: the new pixel size
 *
 * Sets the pixel size to use for named icons.
 *
 * If the pixel size is set to a value != -1, it is used instead
 * of the icon size set by [method@Bobgui.Image.set_icon_size].
 */
void
bobgui_image_set_pixel_size (BobguiImage *image,
			  int       pixel_size)
{
  g_return_if_fail (BOBGUI_IS_IMAGE (image));

  if (_bobgui_icon_helper_set_pixel_size (image->icon_helper, pixel_size))
    {
      if (bobgui_widget_get_visible (BOBGUI_WIDGET (image)))
        bobgui_widget_queue_resize (BOBGUI_WIDGET (image));
      g_object_notify_by_pspec (G_OBJECT (image), image_props[PROP_PIXEL_SIZE]);
    }
}

/**
 * bobgui_image_get_pixel_size:
 * @image: a `BobguiImage`
 *
 * Gets the pixel size used for named icons.
 *
 * Returns: the pixel size used for named icons.
 */
int
bobgui_image_get_pixel_size (BobguiImage *image)
{
  g_return_val_if_fail (BOBGUI_IS_IMAGE (image), -1);

  return _bobgui_icon_helper_get_pixel_size (image->icon_helper);
}

/**
 * bobgui_image_set_icon_size:
 * @image: a `BobguiImage`
 * @icon_size: the new icon size
 *
 * Suggests an icon size to the theme for named icons.
 */
void
bobgui_image_set_icon_size (BobguiImage    *image,
			 BobguiIconSize  icon_size)
{
  g_return_if_fail (BOBGUI_IS_IMAGE (image));

  if (image->icon_size == icon_size)
    return;

  image->icon_size = icon_size;
  bobgui_icon_size_set_style_classes (bobgui_widget_get_css_node (BOBGUI_WIDGET (image)), icon_size);
  g_object_notify_by_pspec (G_OBJECT (image), image_props[PROP_ICON_SIZE]);
}

/**
 * bobgui_image_get_icon_size:
 * @image: a `BobguiImage`
 *
 * Gets the icon size used by the @image when rendering icons.
 *
 * Returns: the image size used by icons
 */
BobguiIconSize
bobgui_image_get_icon_size (BobguiImage *image)
{
  g_return_val_if_fail (BOBGUI_IS_IMAGE (image), BOBGUI_ICON_SIZE_INHERIT);

  return image->icon_size;
}

void
bobgui_image_get_image_size (BobguiImage *image,
                          int      *width,
                          int      *height)
{
  *width = *height = bobgui_icon_helper_get_size (image->icon_helper);
}
