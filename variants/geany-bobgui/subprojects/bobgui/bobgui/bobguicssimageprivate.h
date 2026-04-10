/*
 * Copyright © 2011 Red Hat Inc.
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

#pragma once

#include <cairo.h>
#include <glib-object.h>

#include <bobgui/css/bobguicss.h>
#include "bobgui/css/bobguicsstokenizerprivate.h"
#include "bobgui/css/bobguicssparserprivate.h"
#include "bobgui/bobguicsstypesprivate.h"
#include "bobgui/bobguicssvariablesetprivate.h"
#include "bobgui/bobguicssvalueprivate.h"
#include "bobgui/bobguisnapshot.h"
#include "bobgui/bobguistyleprovider.h"

G_BEGIN_DECLS

#define BOBGUI_TYPE_CSS_IMAGE           (_bobgui_css_image_get_type ())
#define BOBGUI_CSS_IMAGE(obj)           (G_TYPE_CHECK_INSTANCE_CAST (obj, BOBGUI_TYPE_CSS_IMAGE, BobguiCssImage))
#define BOBGUI_CSS_IMAGE_CLASS(cls)     (G_TYPE_CHECK_CLASS_CAST (cls, BOBGUI_TYPE_CSS_IMAGE, BobguiCssImageClass))
#define BOBGUI_IS_CSS_IMAGE(obj)        (G_TYPE_CHECK_INSTANCE_TYPE (obj, BOBGUI_TYPE_CSS_IMAGE))
#define BOBGUI_IS_CSS_IMAGE_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE (obj, BOBGUI_TYPE_CSS_IMAGE))
#define BOBGUI_CSS_IMAGE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_CSS_IMAGE, BobguiCssImageClass))

typedef struct _BobguiCssImage           BobguiCssImage;
typedef struct _BobguiCssImageClass      BobguiCssImageClass;

struct _BobguiCssImage
{
  GObject parent;
};

struct _BobguiCssImageClass
{
  GObjectClass parent_class;

  /* width of image or 0 if it has no width (optional) */
  int          (* get_width)                       (BobguiCssImage                *image);
  /* height of image or 0 if it has no height (optional) */
  int          (* get_height)                      (BobguiCssImage                *image);
  /* aspect ratio (width / height) of image or 0 if it has no aspect ratio (optional) */
  double       (* get_aspect_ratio)                (BobguiCssImage                *image);

  /* create "computed value" in CSS terms, returns a new reference */
  BobguiCssImage *(* compute)                         (BobguiCssImage                *image,
                                                    guint                       property_id,
                                                    BobguiCssComputeContext       *context);
  /* compare two images for equality */
  gboolean     (* equal)                           (BobguiCssImage                *image1,
                                                    BobguiCssImage                *image2);
  /* transition between start and end image (end may be NULL), returns new reference (optional) */
  BobguiCssImage *(* transition)                      (BobguiCssImage                *start,
                                                    BobguiCssImage                *end,
                                                    guint                       property_id,
                                                    double                      progress);

  /* draw to 0,0 with the given width and height */
  void         (* snapshot)                        (BobguiCssImage                *image,
                                                    BobguiSnapshot                *snapshot,
                                                    double                      width,
                                                    double                      height);
  /* is this image to be considered invalid (see https://drafts.csswg.org/css-images-4/#invalid-image for details) */
  gboolean     (* is_invalid)                      (BobguiCssImage                *image);
  /* does this image change based on timestamp? (optional) */
  gboolean     (* is_dynamic)                      (BobguiCssImage                *image);
  /* get image for given timestamp or @image when not dynamic (optional) */
  BobguiCssImage *(* get_dynamic_image)               (BobguiCssImage                *image,
                                                    gint64                      monotonic_time);
  /* parse CSS, return TRUE on success */
  gboolean     (* parse)                           (BobguiCssImage                *image,
                                                    BobguiCssParser               *parser);
  /* print to CSS */
  void         (* print)                           (BobguiCssImage                *image,
                                                    GString                    *string);
  gboolean     (* is_computed)                     (BobguiCssImage                *image);
  gboolean     (* contains_current_color)          (BobguiCssImage                *image);
  BobguiCssImage *( * resolve)                        (BobguiCssImage                *image,
                                                    BobguiCssComputeContext       *context,
                                                    BobguiCssValue                *current_color);
};

GType          _bobgui_css_image_get_type             (void) G_GNUC_CONST;

gboolean       _bobgui_css_image_can_parse            (BobguiCssParser               *parser);
BobguiCssImage *  _bobgui_css_image_new_parse            (BobguiCssParser               *parser);

int            _bobgui_css_image_get_width            (BobguiCssImage                *image) G_GNUC_PURE;
int            _bobgui_css_image_get_height           (BobguiCssImage                *image) G_GNUC_PURE;
double         _bobgui_css_image_get_aspect_ratio     (BobguiCssImage                *image) G_GNUC_PURE;

BobguiCssImage *  _bobgui_css_image_compute              (BobguiCssImage                *image,
                                                    guint                       property_id,
                                                    BobguiCssComputeContext       *context);
gboolean       _bobgui_css_image_equal                (BobguiCssImage                *image1,
                                                    BobguiCssImage                *image2) G_GNUC_PURE;
BobguiCssImage *  _bobgui_css_image_transition           (BobguiCssImage                *start,
                                                    BobguiCssImage                *end,
                                                    guint                       property_id,
                                                    double                      progress);

void           bobgui_css_image_snapshot              (BobguiCssImage                *image,
                                                    BobguiSnapshot                *snapshot,
                                                    double                      width,
                                                    double                      height);
gboolean       bobgui_css_image_is_invalid            (BobguiCssImage                *image) G_GNUC_PURE;
gboolean       bobgui_css_image_is_dynamic            (BobguiCssImage                *image) G_GNUC_PURE;
BobguiCssImage *  bobgui_css_image_get_dynamic_image     (BobguiCssImage                *image,
                                                    gint64                      monotonic_time);
void           _bobgui_css_image_print                (BobguiCssImage                *image,
                                                    GString                    *string);
char *         bobgui_css_image_to_string             (BobguiCssImage                *image);

void           _bobgui_css_image_get_concrete_size    (BobguiCssImage                *image,
                                                    double                      specified_width,
                                                    double                      specified_height,
                                                    double                      default_width,
                                                    double                      default_height,
                                                    double                     *concrete_width,
                                                    double                     *concrete_height);
cairo_surface_t *
               _bobgui_css_image_get_surface          (BobguiCssImage                *image,
                                                    cairo_surface_t            *target,
                                                    int                         surface_width,
                                                    int                         surface_height);
gboolean       bobgui_css_image_is_computed           (BobguiCssImage                *image) G_GNUC_PURE;

gboolean       bobgui_css_image_contains_current_color (BobguiCssImage *image) G_GNUC_PURE;

BobguiCssImage *  bobgui_css_image_resolve                (BobguiCssImage          *image,
                                                     BobguiCssComputeContext *context,
                                                     BobguiCssValue          *current_color);

G_END_DECLS

