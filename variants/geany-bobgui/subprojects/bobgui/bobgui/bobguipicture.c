/*
 * Copyright © 2018 Benjamin Otte
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
 * Authors: Benjamin Otte <otte@gnome.org>
 */

#include "config.h"

#include "bobguipicture.h"

#include "bobguicssnodeprivate.h"
#include "bobguicssnumbervalueprivate.h"
#include "bobguicssstyleprivate.h"
#include "bobguiprivate.h"
#include "bobguisnapshot.h"
#include "bobguitypebuiltins.h"
#include "bobguiwidgetprivate.h"
#include "gdktextureutilsprivate.h"
#include "bobguisymbolicpaintable.h"
#include "bobguirendericonprivate.h"

/**
 * BobguiPicture:
 *
 * Displays a `GdkPaintable`.
 *
 * <picture>
 *   <source srcset="picture-dark.png" media="(prefers-color-scheme: dark)">
 *   <img alt="An example BobguiPicture" src="picture.png">
 * </picture>
 *
 * Many convenience functions are provided to make pictures simple to use.
 * For example, if you want to load an image from a file, and then display
 * it, there’s a convenience function to do this:
 *
 * ```c
 * BobguiWidget *widget = bobgui_picture_new_for_filename ("myfile.png");
 * ```
 *
 * If the file isn’t loaded successfully, the picture will contain a
 * “broken image” icon similar to that used in many web browsers.
 * If you want to handle errors in loading the file yourself,
 * for example by displaying an error message, then load the image with
 * and image loading framework such as libglycin, then create the `BobguiPicture`
 * with [ctor@Bobgui.Picture.new_for_paintable].
 *
 * Sometimes an application will want to avoid depending on external data
 * files, such as image files. See the documentation of `GResource` for details.
 * In this case, [ctor@Bobgui.Picture.new_for_resource] and
 * [method@Bobgui.Picture.set_resource] should be used.
 *
 * `BobguiPicture` displays an image at its natural size. See [class@Bobgui.Image]
 * if you want to display a fixed-size image, such as an icon.
 *
 * ## Sizing the paintable
 *
 * You can influence how the paintable is displayed inside the `BobguiPicture`
 * by changing [property@Bobgui.Picture:content-fit]. See [enum@Bobgui.ContentFit]
 * for details. [property@Bobgui.Picture:can-shrink] can be unset to make sure
 * that paintables are never made smaller than their ideal size - but
 * be careful if you do not know the size of the paintable in use (like
 * when displaying user-loaded images). This can easily cause the picture to
 * grow larger than the screen. And [property@Bobgui.Widget:halign] and
 * [property@Bobgui.Widget:valign] can be used to make sure the paintable doesn't
 * fill all available space but is instead displayed at its original size.
 *
 * ## CSS nodes
 *
 * `BobguiPicture` has a single CSS node with the name `picture`.
 *
 * ## Accessibility
 *
 * `BobguiPicture` uses the [enum@Bobgui.AccessibleRole.img] role.
 */

enum
{
  PROP_0,
  PROP_PAINTABLE,
  PROP_FILE,
  PROP_ALTERNATIVE_TEXT,
  PROP_KEEP_ASPECT_RATIO,
  PROP_CAN_SHRINK,
  PROP_CONTENT_FIT,
  PROP_ISOLATE_CONTENTS,
  NUM_PROPERTIES
};

struct _BobguiPicture
{
  BobguiWidget parent_instance;

  GdkPaintable *paintable;
  GFile *file;

  char *alternative_text;
  BobguiContentFit content_fit;
  guint can_shrink : 1;
  guint isolate_contents : 1;
};

struct _BobguiPictureClass
{
  BobguiWidgetClass parent_class;
};

static GParamSpec *properties[NUM_PROPERTIES] = { NULL, };

G_DEFINE_TYPE (BobguiPicture, bobgui_picture, BOBGUI_TYPE_WIDGET)

static void
bobgui_picture_snapshot (BobguiWidget   *widget,
                      BobguiSnapshot *snapshot)
{
  BobguiPicture *self = BOBGUI_PICTURE (widget);
  BobguiCssStyle *style;
  double ratio;
  int x, y, width, height;
  double w, h;

  if (self->paintable == NULL)
    return;

  style = bobgui_css_node_get_style (bobgui_widget_get_css_node (widget));

  width = bobgui_widget_get_width (widget);
  height = bobgui_widget_get_height (widget);
  ratio = gdk_paintable_get_intrinsic_aspect_ratio (self->paintable);

  if (self->isolate_contents)
    bobgui_snapshot_push_isolation (snapshot, GSK_ISOLATION_ALL);

  if (self->content_fit == BOBGUI_CONTENT_FIT_FILL || ratio == 0)
    {
      if (BOBGUI_IS_SYMBOLIC_PAINTABLE (self->paintable))
        bobgui_css_style_snapshot_icon_paintable (style,
                                               snapshot,
                                               self->paintable,
                                               width, height);
      else
        gdk_paintable_snapshot (self->paintable, snapshot, width, height);
    }
  else
    {
      double picture_ratio = (double) width / height;
      int paintable_width = gdk_paintable_get_intrinsic_width (self->paintable);
      int paintable_height = gdk_paintable_get_intrinsic_height (self->paintable);

      if (self->content_fit == BOBGUI_CONTENT_FIT_SCALE_DOWN &&
          width >= paintable_width && height >= paintable_height)
        {
          w = paintable_width;
          h = paintable_height;
        }
      else if (ratio > picture_ratio)
        {
          if (self->content_fit == BOBGUI_CONTENT_FIT_COVER)
            {
              w = height * ratio;
              h = height;
            }
          else
            {
              w = width;
              h = width / ratio;
            }
        }
      else
        {
          if (self->content_fit == BOBGUI_CONTENT_FIT_COVER)
            {
              w = width;
              h = width / ratio;
            }
          else
            {
              w = height * ratio;
              h = height;
            }
        }

      w = ceil (w);
      h = ceil (h);

      x = (width - w) / 2;
      y = floor(height - h) / 2;

      bobgui_snapshot_save (snapshot);
      bobgui_snapshot_translate (snapshot, &GRAPHENE_POINT_INIT (x, y));
      if (BOBGUI_IS_SYMBOLIC_PAINTABLE (self->paintable))
        bobgui_css_style_snapshot_icon_paintable (style,
                                               snapshot,
                                               self->paintable,
                                               w, h);
      else
        gdk_paintable_snapshot (self->paintable, snapshot, w, h);
      bobgui_snapshot_restore (snapshot);
    }

  if (self->isolate_contents)
    bobgui_snapshot_pop (snapshot); /* isolation */
}

static BobguiSizeRequestMode
bobgui_picture_get_request_mode (BobguiWidget *widget)
{
  return BOBGUI_SIZE_REQUEST_HEIGHT_FOR_WIDTH;
}

static void
bobgui_picture_measure (BobguiWidget      *widget,
                     BobguiOrientation  orientation,
                     int            for_size,
                     int           *minimum,
                     int           *natural,
                     int           *minimum_baseline,
                     int           *natural_baseline)
{
  BobguiPicture *self = BOBGUI_PICTURE (widget);
  BobguiCssStyle *style;
  double min_width, min_height, nat_width, nat_height;
  double default_size;

  /* for_size = 0 below is treated as -1, but we want to return zeros. */
  if (self->paintable == NULL || for_size == 0)
    {
      *minimum = 0;
      *natural = 0;
      return;
    }

  style = bobgui_css_node_get_style (bobgui_widget_get_css_node (widget));
  default_size = bobgui_css_number_value_get (style->icon->icon_size, 100);

  if (self->can_shrink)
    {
      min_width = min_height = 0;
    }
  else
    {
      gdk_paintable_compute_concrete_size (self->paintable,
                                           0, 0,
                                           default_size, default_size,
                                           &min_width, &min_height);
    }

  if (for_size > 0 && self->content_fit == BOBGUI_CONTENT_FIT_SCALE_DOWN)
    {
      double opposite_intrinsic_size;

      if (orientation == BOBGUI_ORIENTATION_HORIZONTAL)
        opposite_intrinsic_size = gdk_paintable_get_intrinsic_height (self->paintable);
      else
        opposite_intrinsic_size = gdk_paintable_get_intrinsic_width (self->paintable);

      if (opposite_intrinsic_size != 0 && opposite_intrinsic_size < for_size)
        for_size = opposite_intrinsic_size;
    }

  if (orientation == BOBGUI_ORIENTATION_HORIZONTAL)
    {
      gdk_paintable_compute_concrete_size (self->paintable,
                                           0,
                                           for_size < 0 ? 0 : for_size,
                                           default_size, default_size,
                                           &nat_width, &nat_height);
      *minimum = ceil (min_width);
      *natural = ceil (nat_width);
    }
  else
    {
      gdk_paintable_compute_concrete_size (self->paintable,
                                           for_size < 0 ? 0 : for_size,
                                           0,
                                           default_size, default_size,
                                           &nat_width, &nat_height);
      *minimum = ceil (min_height);
      *natural = ceil (nat_height);
    }
}

static void
bobgui_picture_set_property (GObject      *object,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  BobguiPicture *self = BOBGUI_PICTURE (object);

  switch (prop_id)
    {
    case PROP_PAINTABLE:
      bobgui_picture_set_paintable (self, g_value_get_object (value));
      break;

    case PROP_FILE:
      bobgui_picture_set_file (self, g_value_get_object (value));
      break;

    case PROP_ALTERNATIVE_TEXT:
      bobgui_picture_set_alternative_text (self, g_value_get_string (value));
      break;

    case PROP_KEEP_ASPECT_RATIO:
      G_GNUC_BEGIN_IGNORE_DEPRECATIONS
      bobgui_picture_set_keep_aspect_ratio (self, g_value_get_boolean (value));
      G_GNUC_END_IGNORE_DEPRECATIONS
      break;

    case PROP_CAN_SHRINK:
      bobgui_picture_set_can_shrink (self, g_value_get_boolean (value));
      break;

    case PROP_CONTENT_FIT:
      bobgui_picture_set_content_fit (self, g_value_get_enum (value));
      break;

    case PROP_ISOLATE_CONTENTS:
      bobgui_picture_set_isolate_contents (self, g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_picture_get_property (GObject     *object,
                          guint        prop_id,
                          GValue      *value,
                          GParamSpec  *pspec)
{
  BobguiPicture *self = BOBGUI_PICTURE (object);

  switch (prop_id)
    {
    case PROP_PAINTABLE:
      g_value_set_object (value, self->paintable);
      break;

    case PROP_FILE:
      g_value_set_object (value, self->file);
      break;

    case PROP_ALTERNATIVE_TEXT:
      g_value_set_string (value, self->alternative_text);
      break;

    case PROP_KEEP_ASPECT_RATIO:
      G_GNUC_BEGIN_IGNORE_DEPRECATIONS
      g_value_set_boolean (value, bobgui_picture_get_keep_aspect_ratio (self));
      G_GNUC_END_IGNORE_DEPRECATIONS
      break;

    case PROP_CAN_SHRINK:
      g_value_set_boolean (value, self->can_shrink);
      break;

    case PROP_CONTENT_FIT:
      g_value_set_enum (value, self->content_fit);
      break;

    case PROP_ISOLATE_CONTENTS:
      g_value_set_boolean (value, self->isolate_contents);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_picture_paintable_invalidate_contents (GdkPaintable *paintable,
                                           BobguiPicture   *self)
{
  bobgui_widget_queue_draw (BOBGUI_WIDGET (self));
}

static void
bobgui_picture_paintable_invalidate_size (GdkPaintable *paintable,
                                       BobguiPicture   *self)
{
  bobgui_widget_queue_resize (BOBGUI_WIDGET (self));
}

static void
bobgui_picture_css_changed (BobguiWidget         *widget,
                         BobguiCssStyleChange *change)
{
  BobguiPicture *self = BOBGUI_PICTURE (widget);

  BOBGUI_WIDGET_CLASS (bobgui_picture_parent_class)->css_changed (widget, change);

  if (bobgui_css_style_change_affects (change, BOBGUI_CSS_AFFECTS_ICON_REDRAW_SYMBOLIC) &&
      self->paintable && BOBGUI_IS_SYMBOLIC_PAINTABLE (self->paintable))
    {
      bobgui_widget_queue_draw (widget);
    }
}

static void
bobgui_picture_clear_paintable (BobguiPicture *self)
{
  guint flags;

  if (self->paintable == NULL)
    return;

  flags = gdk_paintable_get_flags (self->paintable);

  if ((flags & GDK_PAINTABLE_STATIC_CONTENTS) == 0)
    g_signal_handlers_disconnect_by_func (self->paintable,
                                          bobgui_picture_paintable_invalidate_contents,
                                          self);

  if ((flags & GDK_PAINTABLE_STATIC_SIZE) == 0)
    g_signal_handlers_disconnect_by_func (self->paintable,
                                          bobgui_picture_paintable_invalidate_size,
                                          self);

  g_object_unref (self->paintable);
}

static void
bobgui_picture_dispose (GObject *object)
{
  BobguiPicture *self = BOBGUI_PICTURE (object);

  bobgui_picture_clear_paintable (self);

  g_clear_object (&self->file);
  g_clear_pointer (&self->alternative_text, g_free);

  G_OBJECT_CLASS (bobgui_picture_parent_class)->dispose (object);
};

static void
bobgui_picture_class_init (BobguiPictureClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (class);

  gobject_class->set_property = bobgui_picture_set_property;
  gobject_class->get_property = bobgui_picture_get_property;
  gobject_class->dispose = bobgui_picture_dispose;

  widget_class->snapshot = bobgui_picture_snapshot;
  widget_class->get_request_mode = bobgui_picture_get_request_mode;
  widget_class->measure = bobgui_picture_measure;
  widget_class->css_changed = bobgui_picture_css_changed;

  /**
   * BobguiPicture:paintable:
   *
   * The `GdkPaintable` to be displayed by this `BobguiPicture`.
   */
  properties[PROP_PAINTABLE] =
      g_param_spec_object ("paintable", NULL, NULL,
                           GDK_TYPE_PAINTABLE,
                           BOBGUI_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiPicture:file:
   *
   * The `GFile` that is displayed or %NULL if none.
   */
  properties[PROP_FILE] =
      g_param_spec_object ("file", NULL, NULL,
                           G_TYPE_FILE,
                           BOBGUI_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiPicture:alternative-text:
   *
   * The alternative textual description for the picture.
   */
  properties[PROP_ALTERNATIVE_TEXT] =
      g_param_spec_string ("alternative-text", NULL, NULL,
                           NULL,
                           BOBGUI_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiPicture:keep-aspect-ratio:
   *
   * Whether the BobguiPicture will render its contents trying to preserve the aspect
   * ratio.
   *
   * Deprecated: 4.8: Use [property@Bobgui.Picture:content-fit] instead.
   */
  properties[PROP_KEEP_ASPECT_RATIO] =
      g_param_spec_boolean ("keep-aspect-ratio", NULL, NULL,
                            TRUE,
                            BOBGUI_PARAM_READWRITE |
                            G_PARAM_EXPLICIT_NOTIFY |
                            G_PARAM_DEPRECATED);

  /**
   * BobguiPicture:can-shrink:
   *
   * If the `BobguiPicture` can be made smaller than the natural size of its contents.
   */
  properties[PROP_CAN_SHRINK] =
      g_param_spec_boolean ("can-shrink", NULL, NULL,
                            TRUE,
                            BOBGUI_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiPicture:content-fit:
   *
   * How the content should be resized to fit inside the `BobguiPicture`.
   *
   * Since: 4.8
   */
  properties[PROP_CONTENT_FIT] =
      g_param_spec_enum ("content-fit", NULL, NULL,
                         BOBGUI_TYPE_CONTENT_FIT,
                         BOBGUI_CONTENT_FIT_CONTAIN,
                         BOBGUI_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiPicture:isolate-contents:
   *
   * If the rendering of the contents is isolated from the rest of the widget tree.
   *
   * Since: 4.22
   */
  properties[PROP_ISOLATE_CONTENTS] =
      g_param_spec_boolean ("isolate-contents", NULL, NULL,
                            TRUE,
                            BOBGUI_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (gobject_class, NUM_PROPERTIES, properties);

  bobgui_widget_class_set_css_name (widget_class, I_("picture"));
  bobgui_widget_class_set_accessible_role (widget_class, BOBGUI_ACCESSIBLE_ROLE_IMG);
}

static void
bobgui_picture_init (BobguiPicture *self)
{
  self->can_shrink = TRUE;
  self->content_fit = BOBGUI_CONTENT_FIT_CONTAIN;
  self->isolate_contents = TRUE;

  bobgui_widget_set_overflow (BOBGUI_WIDGET (self), BOBGUI_OVERFLOW_HIDDEN);
}

/**
 * bobgui_picture_new:
 *
 * Creates a new empty `BobguiPicture` widget.
 *
 * Returns: a newly created `BobguiPicture` widget.
 */
BobguiWidget*
bobgui_picture_new (void)
{
  return g_object_new (BOBGUI_TYPE_PICTURE, NULL);
}

/**
 * bobgui_picture_new_for_paintable:
 * @paintable: (nullable): a `GdkPaintable`
 *
 * Creates a new `BobguiPicture` displaying @paintable.
 *
 * The `BobguiPicture` will track changes to the @paintable and update
 * its size and contents in response to it.
 *
 * Returns: a new `BobguiPicture`
 */
BobguiWidget*
bobgui_picture_new_for_paintable (GdkPaintable *paintable)
{
  g_return_val_if_fail (paintable == NULL || GDK_IS_PAINTABLE (paintable), NULL);

  return g_object_new (BOBGUI_TYPE_PICTURE,
                       "paintable", paintable,
                       NULL);
}

/**
 * bobgui_picture_new_for_pixbuf:
 * @pixbuf: (nullable): a `GdkPixbuf`
 *
 * Creates a new `BobguiPicture` displaying @pixbuf.
 *
 * This is a utility function that calls [ctor@Bobgui.Picture.new_for_paintable],
 * See that function for details.
 *
 * The pixbuf must not be modified after passing it to this function.
 *
 * Returns: a new `BobguiPicture`
 *
 * Deprecated: 4.12: Use [ctor@Bobgui.Picture.new_for_paintable] and
 *   [ctor@Gdk.Texture.new_for_pixbuf] instead
 */
BobguiWidget*
bobgui_picture_new_for_pixbuf (GdkPixbuf *pixbuf)
{
  BobguiWidget *result;
  GdkPaintable *paintable;

  g_return_val_if_fail (pixbuf == NULL || GDK_IS_PIXBUF (pixbuf), NULL);

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  if (pixbuf)
    paintable = GDK_PAINTABLE (gdk_texture_new_for_pixbuf (pixbuf));
  else
    paintable = NULL;
G_GNUC_END_IGNORE_DEPRECATIONS

  result = bobgui_picture_new_for_paintable (paintable);

  if (paintable)
    g_object_unref (paintable);

  return result;
}

/**
 * bobgui_picture_new_for_file:
 * @file: (nullable): a `GFile`
 *
 * Creates a new `BobguiPicture` displaying the given @file.
 *
 * If the file isn’t found or can’t be loaded, the resulting
 * `BobguiPicture` is empty.
 *
 * If you need to detect failures to load the file, use an
 * image loading framework such as libglycin to load the file
 * yourself, then create the `BobguiPicture` from the texture.
 *
 * Returns: a new `BobguiPicture`
 */
BobguiWidget*
bobgui_picture_new_for_file (GFile *file)
{
  g_return_val_if_fail (file == NULL || G_IS_FILE (file), NULL);

  return g_object_new (BOBGUI_TYPE_PICTURE,
                       "file", file,
                       NULL);
}

/**
 * bobgui_picture_new_for_filename:
 * @filename: (type filename) (nullable): a filename
 *
 * Creates a new `BobguiPicture` displaying the file @filename.
 *
 * This is a utility function that calls [ctor@Bobgui.Picture.new_for_file].
 * See that function for details.
 *
 * Returns: a new `BobguiPicture`
 */
BobguiWidget*
bobgui_picture_new_for_filename (const char *filename)
{
  BobguiWidget *result;
  GFile *file;

  if (filename)
    file = g_file_new_for_path (filename);
  else
    file = NULL;

  result = bobgui_picture_new_for_file (file);

  if (file)
    g_object_unref (file);

  return result;
}

/**
 * bobgui_picture_new_for_resource:
 * @resource_path: (nullable): resource path to play back
 *
 * Creates a new `BobguiPicture` displaying the resource at @resource_path.
 *
 * This is a utility function that calls [ctor@Bobgui.Picture.new_for_file].
 * See that function for details.
 *
 * Returns: a new `BobguiPicture`
 */
BobguiWidget *
bobgui_picture_new_for_resource (const char *resource_path)
{
  BobguiWidget *result;
  GFile *file;

  if (resource_path)
    {
      char *uri, *escaped;

      escaped = g_uri_escape_string (resource_path,
                                     G_URI_RESERVED_CHARS_ALLOWED_IN_PATH, FALSE);
      uri = g_strconcat ("resource://", escaped, NULL);
      g_free (escaped);

      file = g_file_new_for_uri (uri);
      g_free (uri);
    }
  else
    {
      file = NULL;
    }

  result = bobgui_picture_new_for_file (file);

  if (file)
    g_object_unref (file);

  return result;
}

/**
 * bobgui_picture_set_file:
 * @self: a `BobguiPicture`
 * @file: (nullable): a `GFile`
 *
 * Makes @self load and display @file.
 *
 * See [ctor@Bobgui.Picture.new_for_file] for details.
 *
 * ::: warning
 *     Note that this function should not be used with untrusted data.
 *     Use a proper image loading framework such as libglycin, which can
 *     load many image formats into a `GdkTexture`, and then use
 *     [method@Bobgui.Image.set_from_paintable].
 */
void
bobgui_picture_set_file (BobguiPicture *self,
                      GFile      *file)
{
  GdkPaintable *paintable;

  g_return_if_fail (BOBGUI_IS_PICTURE (self));
  g_return_if_fail (file == NULL || G_IS_FILE (file));

  if (self->file == file)
    return;

  g_object_freeze_notify (G_OBJECT (self));

  g_set_object (&self->file, file);
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_FILE]);

  if (file)
    paintable = gdk_paintable_new_from_file (file, NULL);
  else
    paintable = NULL;

  bobgui_picture_set_paintable (self, paintable);
  g_clear_object (&paintable);

  g_object_thaw_notify (G_OBJECT (self));
}

/**
 * bobgui_picture_get_file:
 * @self: a `BobguiPicture`
 *
 * Gets the `GFile` currently displayed if @self is displaying a file.
 *
 * If @self is not displaying a file, for example when
 * [method@Bobgui.Picture.set_paintable] was used, then %NULL is returned.
 *
 * Returns: (nullable) (transfer none): The `GFile` displayed by @self.
 */
GFile *
bobgui_picture_get_file (BobguiPicture *self)
{
  g_return_val_if_fail (BOBGUI_IS_PICTURE (self), FALSE);

  return self->file;
}

/**
 * bobgui_picture_set_filename:
 * @self: a `BobguiPicture`
 * @filename: (type filename) (nullable): the filename to play
 *
 * Makes @self load and display the given @filename.
 *
 * This is a utility function that calls [method@Bobgui.Picture.set_file].
 *
 * ::: warning
 *     Note that this function should not be used with untrusted data.
 *     Use a proper image loading framework such as libglycin, which can
 *     load many image formats into a `GdkTexture`, and then use
 *     [method@Bobgui.Image.set_from_paintable].
 */
void
bobgui_picture_set_filename (BobguiPicture *self,
                          const char *filename)
{
  GFile *file;

  g_return_if_fail (BOBGUI_IS_PICTURE (self));

  if (filename)
    file = g_file_new_for_path (filename);
  else
    file = NULL;

  bobgui_picture_set_file (self, file);

  if (file)
    g_object_unref (file);
}

/**
 * bobgui_picture_set_resource:
 * @self: a `BobguiPicture`
 * @resource_path: (nullable): the resource to set
 *
 * Makes @self load and display the resource at the given
 * @resource_path.
 *
 * This is a utility function that calls [method@Bobgui.Picture.set_file].
 */
void
bobgui_picture_set_resource (BobguiPicture *self,
                          const char *resource_path)
{
  GFile *file;

  g_return_if_fail (BOBGUI_IS_PICTURE (self));

  if (resource_path)
    {
      char *uri, *escaped;

      escaped = g_uri_escape_string (resource_path,
                                     G_URI_RESERVED_CHARS_ALLOWED_IN_PATH, FALSE);
      uri = g_strconcat ("resource://", escaped, NULL);
      g_free (escaped);

      file = g_file_new_for_uri (uri);
      g_free (uri);
    }
  else
    {
      file = NULL;
    }

  bobgui_picture_set_file (self, file);

  if (file)
    g_object_unref (file);
}

/**
 * bobgui_picture_set_pixbuf:
 * @self: a `BobguiPicture`
 * @pixbuf: (nullable): a `GdkPixbuf`
 *
 * Sets a `BobguiPicture` to show a `GdkPixbuf`.
 *
 * See [ctor@Bobgui.Picture.new_for_pixbuf] for details.
 *
 * This is a utility function that calls [method@Bobgui.Picture.set_paintable].
 *
 * Deprecated: 4.12: Use [method@Bobgui.Picture.set_paintable] instead
 */
void
bobgui_picture_set_pixbuf (BobguiPicture *self,
                        GdkPixbuf  *pixbuf)
{
  GdkTexture *texture;

  g_return_if_fail (BOBGUI_IS_PICTURE (self));
  g_return_if_fail (pixbuf == NULL || GDK_IS_PIXBUF (pixbuf));

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  if (pixbuf)
    texture = gdk_texture_new_for_pixbuf (pixbuf);
  else
    texture = NULL;
G_GNUC_END_IGNORE_DEPRECATIONS

  bobgui_picture_set_paintable (self, GDK_PAINTABLE (texture));

  if (texture)
    g_object_unref (texture);
}

static gboolean
paintable_size_equal (GdkPaintable *one,
                      GdkPaintable *two)
{
  if (one == NULL)
    return two == NULL;
  else if (two == NULL)
    return FALSE;

  return gdk_paintable_get_intrinsic_width (one) == gdk_paintable_get_intrinsic_width (two) &&
         gdk_paintable_get_intrinsic_height (one) == gdk_paintable_get_intrinsic_height (two) &&
         gdk_paintable_get_intrinsic_aspect_ratio (one) == gdk_paintable_get_intrinsic_aspect_ratio (two);
}

/**
 * bobgui_picture_set_paintable:
 * @self: a `BobguiPicture`
 * @paintable: (nullable): a `GdkPaintable`
 *
 * Makes @self display the given @paintable.
 *
 * If @paintable is `NULL`, nothing will be displayed.
 *
 * See [ctor@Bobgui.Picture.new_for_paintable] for details.
 */
void
bobgui_picture_set_paintable (BobguiPicture   *self,
                           GdkPaintable *paintable)
{
  gboolean size_changed;

  g_return_if_fail (BOBGUI_IS_PICTURE (self));
  g_return_if_fail (paintable == NULL || GDK_IS_PAINTABLE (paintable));

  if (self->paintable == paintable)
    return;

  g_object_freeze_notify (G_OBJECT (self));

  if (paintable)
    g_object_ref (paintable);

  size_changed = !paintable_size_equal (self->paintable, paintable);
 
  bobgui_picture_clear_paintable (self);

  self->paintable = paintable;

  if (paintable)
    {
      const guint flags = gdk_paintable_get_flags (paintable);

      if ((flags & GDK_PAINTABLE_STATIC_CONTENTS) == 0)
        g_signal_connect (paintable,
                          "invalidate-contents",
                          G_CALLBACK (bobgui_picture_paintable_invalidate_contents),
                          self);

      if ((flags & GDK_PAINTABLE_STATIC_SIZE) == 0)
        g_signal_connect (paintable,
                          "invalidate-size",
                          G_CALLBACK (bobgui_picture_paintable_invalidate_size),
                          self);
    }

  if (size_changed)
    bobgui_widget_queue_resize (BOBGUI_WIDGET (self));
  else
    bobgui_widget_queue_draw (BOBGUI_WIDGET (self));

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_PAINTABLE]);

  g_object_thaw_notify (G_OBJECT (self));
}

/**
 * bobgui_picture_get_paintable:
 * @self: a `BobguiPicture`
 *
 * Gets the `GdkPaintable` being displayed by the `BobguiPicture`.
 *
 * Returns: (nullable) (transfer none): the displayed paintable
 */
GdkPaintable *
bobgui_picture_get_paintable (BobguiPicture *self)
{
  g_return_val_if_fail (BOBGUI_IS_PICTURE (self), NULL);

  return self->paintable;
}

/**
 * bobgui_picture_set_keep_aspect_ratio:
 * @self: a `BobguiPicture`
 * @keep_aspect_ratio: whether to keep aspect ratio
 *
 * If set to %TRUE, the @self will render its contents according to
 * their aspect ratio.
 *
 * That means that empty space may show up at the top/bottom or
 * left/right of @self.
 *
 * If set to %FALSE or if the contents provide no aspect ratio,
 * the contents will be stretched over the picture's whole area.
 *
 * Deprecated: 4.8: Use [method@Bobgui.Picture.set_content_fit] instead. If still
 *   used, this method will always set the [property@Bobgui.Picture:content-fit]
 *   property to `BOBGUI_CONTENT_FIT_CONTAIN` if @keep_aspect_ratio is true,
 *   otherwise it will set it to `BOBGUI_CONTENT_FIT_FILL`.
 */
void
bobgui_picture_set_keep_aspect_ratio (BobguiPicture *self,
                                   gboolean    keep_aspect_ratio)
{
  if (keep_aspect_ratio)
    bobgui_picture_set_content_fit (self, BOBGUI_CONTENT_FIT_CONTAIN);
  else
    bobgui_picture_set_content_fit (self, BOBGUI_CONTENT_FIT_FILL);
}

/**
 * bobgui_picture_get_keep_aspect_ratio:
 * @self: a `BobguiPicture`
 *
 * Returns whether the `BobguiPicture` preserves its contents aspect ratio.
 *
 * Returns: %TRUE if the self tries to keep the contents' aspect ratio
 *
 * Deprecated: 4.8: Use [method@Bobgui.Picture.get_content_fit] instead. This will
 *   now return `FALSE` only if [property@Bobgui.Picture:content-fit] is
 *   `BOBGUI_CONTENT_FIT_FILL`. Returns `TRUE` otherwise.
 */
gboolean
bobgui_picture_get_keep_aspect_ratio (BobguiPicture *self)
{
  g_return_val_if_fail (BOBGUI_IS_PICTURE (self), TRUE);

  return self->content_fit != BOBGUI_CONTENT_FIT_FILL;
}

/**
 * bobgui_picture_set_can_shrink:
 * @self: a `BobguiPicture`
 * @can_shrink: if @self can be made smaller than its contents
 *
 * If set to %TRUE, then @self can be made smaller than its contents.
 *
 * The contents will then be scaled down when rendering.
 *
 * If you want to still force a minimum size manually, consider using
 * [method@Bobgui.Widget.set_size_request].
 *
 * Also of note is that a similar function for growing does not exist
 * because the grow behavior can be controlled via
 * [method@Bobgui.Widget.set_halign] and [method@Bobgui.Widget.set_valign].
 */
void
bobgui_picture_set_can_shrink (BobguiPicture *self,
                            gboolean    can_shrink)
{
  g_return_if_fail (BOBGUI_IS_PICTURE (self));

  if (self->can_shrink == can_shrink)
    return;

  self->can_shrink = can_shrink;

  bobgui_widget_queue_resize (BOBGUI_WIDGET (self));

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_CAN_SHRINK]);
}

/**
 * bobgui_picture_get_can_shrink:
 * @self: a `BobguiPicture`
 *
 * Returns whether the `BobguiPicture` respects its contents size.
 *
 * Returns: %TRUE if the picture can be made smaller than its contents
 */
gboolean
bobgui_picture_get_can_shrink (BobguiPicture *self)
{
  g_return_val_if_fail (BOBGUI_IS_PICTURE (self), FALSE);

  return self->can_shrink;
}

/**
 * bobgui_picture_set_content_fit:
 * @self: a `BobguiPicture`
 * @content_fit: the content fit mode
 *
 * Sets how the content should be resized to fit the `BobguiPicture`.
 *
 * See [enum@Bobgui.ContentFit] for details.
 *
 * Since: 4.8
 */
void
bobgui_picture_set_content_fit (BobguiPicture    *self,
                             BobguiContentFit  content_fit)
{
  gboolean notify_keep_aspect_ratio;
  gboolean queue_resize;

  g_return_if_fail (BOBGUI_IS_PICTURE (self));

  if (self->content_fit == content_fit)
    return;

  notify_keep_aspect_ratio = (content_fit == BOBGUI_CONTENT_FIT_FILL ||
                              self->content_fit == BOBGUI_CONTENT_FIT_FILL);
  queue_resize = (content_fit == BOBGUI_CONTENT_FIT_SCALE_DOWN ||
                  self->content_fit == BOBGUI_CONTENT_FIT_SCALE_DOWN);

  self->content_fit = content_fit;

  if (queue_resize)
    bobgui_widget_queue_resize (BOBGUI_WIDGET (self));
  else
    bobgui_widget_queue_draw (BOBGUI_WIDGET (self));

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_CONTENT_FIT]);

  if (notify_keep_aspect_ratio)
      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_KEEP_ASPECT_RATIO]);
}

/**
 * bobgui_picture_get_content_fit:
 * @self: a `BobguiPicture`
 *
 * Returns the fit mode for the content of the `BobguiPicture`.
 *
 * See [enum@Bobgui.ContentFit] for details.
 *
 * Returns: the content fit mode
 *
 * Since: 4.8
 */
BobguiContentFit
bobgui_picture_get_content_fit (BobguiPicture *self)
{
  g_return_val_if_fail (BOBGUI_IS_PICTURE (self), FALSE);

  return self->content_fit;
}

/**
 * bobgui_picture_set_isolate_contents:
 * @self: a `BobguiPicture`
 * @isolate_contents: if contents are rendered separately
 *
 * If set to true, then the contents will be rendered individually.
 *
 * If set to false they will be able to erase or otherwise mix with
 * the background.
 *
 * BOBGUI supports finer grained isolation, in rare cases where you need
 * this, you can use [method@Bobgui.Snapshot.push_isolation] yourself to
 * achieve this.
 *
 * By default contents are isolated.
 *
 * Since: 4.22
 */
void
bobgui_picture_set_isolate_contents (BobguiPicture *self,
                                  gboolean    isolate_contents)
{
  g_return_if_fail (BOBGUI_IS_PICTURE (self));

  if (self->isolate_contents == isolate_contents)
    return;

  self->isolate_contents = isolate_contents;

  bobgui_widget_queue_resize (BOBGUI_WIDGET (self));

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_ISOLATE_CONTENTS]);
}

/**
 * bobgui_picture_get_isolate_contents:
 * @self: a `BobguiPicture`
 *
 * Returns whether the contents are isolated.
 *
 * Returns: True if contents are isolated
 *
 * Since: 4.22
 */
gboolean
bobgui_picture_get_isolate_contents (BobguiPicture *self)
{
  g_return_val_if_fail (BOBGUI_IS_PICTURE (self), TRUE);

  return self->isolate_contents;
}

/**
 * bobgui_picture_set_alternative_text:
 * @self: a `BobguiPicture`
 * @alternative_text: (nullable): a textual description of the contents
 *
 * Sets an alternative textual description for the picture contents.
 *
 * It is equivalent to the "alt" attribute for images on websites.
 *
 * This text will be made available to accessibility tools.
 *
 * If the picture cannot be described textually, set this property to %NULL.
 */
void
bobgui_picture_set_alternative_text (BobguiPicture *self,
                                  const char *alternative_text)
{
  g_return_if_fail (BOBGUI_IS_PICTURE (self));

  if (g_strcmp0 (self->alternative_text, alternative_text) == 0)
    return;

  g_free (self->alternative_text);
  self->alternative_text = g_strdup (alternative_text);

  bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (self),
                                  BOBGUI_ACCESSIBLE_PROPERTY_DESCRIPTION, alternative_text,
                                  -1);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_ALTERNATIVE_TEXT]);
}

/**
 * bobgui_picture_get_alternative_text:
 * @self: a `BobguiPicture`
 *
 * Gets the alternative textual description of the picture.
 *
 * The returned string will be %NULL if the picture cannot be described textually.
 *
 * Returns: (nullable) (transfer none): the alternative textual description of @self.
 */
const char *
bobgui_picture_get_alternative_text (BobguiPicture *self)
{
  g_return_val_if_fail (BOBGUI_IS_PICTURE (self), NULL);

  return self->alternative_text;
}

