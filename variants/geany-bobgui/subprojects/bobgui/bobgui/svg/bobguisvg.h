/*
 * Copyright © 2025 Red Hat, Inc
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
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
 * Authors: Matthias Clasen <mclasen@redhat.com>
 */

#pragma once

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <gdk/gdk.h>
#include <gsk/gsk.h>
#include <bobgui/bobguisnapshot.h>
#include <bobgui/bobguienums.h>
#include <bobgui/svg/bobguisvgenumtypes.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_SVG (bobgui_svg_get_type ())

GDK_AVAILABLE_IN_4_22
G_DECLARE_FINAL_TYPE (BobguiSvg, bobgui_svg, BOBGUI, SVG, GObject)

GDK_AVAILABLE_IN_4_22
BobguiSvg *         bobgui_svg_new               (void);

GDK_AVAILABLE_IN_4_22
BobguiSvg *         bobgui_svg_new_from_bytes    (GBytes        *bytes);

GDK_AVAILABLE_IN_4_22
BobguiSvg *         bobgui_svg_new_from_resource (const char    *path);

GDK_AVAILABLE_IN_4_22
void             bobgui_svg_load_from_bytes   (BobguiSvg        *self,
                                            GBytes        *bytes);

GDK_AVAILABLE_IN_4_22
void             bobgui_svg_load_from_resource (BobguiSvg        *self,
                                             const char    *path);

GDK_AVAILABLE_IN_4_22
GBytes *         bobgui_svg_serialize         (BobguiSvg        *self);

GDK_AVAILABLE_IN_4_22
gboolean         bobgui_svg_write_to_file     (BobguiSvg        *self,
                                            const char    *filename,
                                            GError       **error);

GDK_AVAILABLE_IN_4_22
void             bobgui_svg_set_weight        (BobguiSvg        *self,
                                            double         weight);
GDK_AVAILABLE_IN_4_22
double           bobgui_svg_get_weight        (BobguiSvg        *self);

GDK_AVAILABLE_IN_4_22
void             bobgui_svg_set_state         (BobguiSvg        *self,
                                            unsigned int   state);
GDK_AVAILABLE_IN_4_22
unsigned int     bobgui_svg_get_state         (BobguiSvg        *self);

GDK_AVAILABLE_IN_4_22
const char **    bobgui_svg_get_state_names   (BobguiSvg        *self,
                                            unsigned int  *length);

GDK_AVAILABLE_IN_4_22
void             bobgui_svg_set_frame_clock   (BobguiSvg        *self,
                                            GdkFrameClock *clock);

GDK_AVAILABLE_IN_4_22
void             bobgui_svg_play              (BobguiSvg        *self);

GDK_AVAILABLE_IN_4_22
void             bobgui_svg_pause             (BobguiSvg        *self);

GDK_AVAILABLE_IN_4_24
void             bobgui_svg_set_overflow      (BobguiSvg        *self,
                                            BobguiOverflow    overflow);
GDK_AVAILABLE_IN_4_24
BobguiOverflow      bobgui_svg_get_overflow      (BobguiSvg        *self);

/**
 * BobguiSvgFeatures:
 * @BOBGUI_SVG_ANIMATIONS: Whether to run animations. If disabled,
 *   state changes are applied without transitions
 * @BOBGUI_SVG_SYSTEM_RESOURCES: Whether to use system resources,
 *   such as fonts. If disabled, only embedded fonts are used
 * @BOBGUI_SVG_EXTERNAL_RESOURCES: Whether to load external
 *   resources, such as images. If disabled, only embedded
 *   images are loaded
 * @BOBGUI_SVG_EXTENSIONS: Whether to allow gpa extensions, such
 *   as states and transitions
 * @BOBGUI_SVG_TRADITIONAL_SYMBOLIC: This feature is meant for
 *   compatibility with old symbolic icons. If this is enabled,
 *   fill and stroke attributes are ignored. The used colors
 *   are derived from symbolic style classes if present, and
 *   the default fill color is the symbolic foreground color.
 *
 * Features of the SVG renderer that can be enabled or disabled.
 *
 * By default, all features except `BOBGUI_SVG_TRADITIONAL_SYMBOLIC`
 * are enabled.
 *
 * New values may be added in the future.
 *
 * Since: 4.22
 */
typedef enum
{
  BOBGUI_SVG_ANIMATIONS           = 1 << 0,
  BOBGUI_SVG_SYSTEM_RESOURCES     = 1 << 1,
  BOBGUI_SVG_EXTERNAL_RESOURCES   = 1 << 2,
  BOBGUI_SVG_EXTENSIONS           = 1 << 3,
  BOBGUI_SVG_TRADITIONAL_SYMBOLIC = 1 << 4,
} BobguiSvgFeatures;

/**
 * BOBGUI_SVG_DEFAULT_FEATURES:
 *
 * The `BobguiSvgFeatures` that are enabled by default.
 *
 * Since: 4.22
 */
#define BOBGUI_SVG_DEFAULT_FEATURES \
  (BOBGUI_SVG_ANIMATIONS | \
   BOBGUI_SVG_SYSTEM_RESOURCES | \
   BOBGUI_SVG_EXTERNAL_RESOURCES | \
   BOBGUI_SVG_EXTENSIONS)

GDK_AVAILABLE_IN_4_22
void             bobgui_svg_set_features      (BobguiSvg         *self,
                                            BobguiSvgFeatures  features);

GDK_AVAILABLE_IN_4_22
BobguiSvgFeatures   bobgui_svg_get_features      (BobguiSvg         *self);

typedef enum
{
  BOBGUI_SVG_ERROR_INVALID_SYNTAX,
  BOBGUI_SVG_ERROR_INVALID_ELEMENT,
  BOBGUI_SVG_ERROR_INVALID_ATTRIBUTE,
  BOBGUI_SVG_ERROR_MISSING_ATTRIBUTE,
  BOBGUI_SVG_ERROR_INVALID_REFERENCE,
  BOBGUI_SVG_ERROR_FAILED_UPDATE,
  BOBGUI_SVG_ERROR_FAILED_RENDERING,
  BOBGUI_SVG_ERROR_IGNORED_ELEMENT,
  BOBGUI_SVG_ERROR_LIMITS_EXCEEDED,
  BOBGUI_SVG_ERROR_NOT_IMPLEMENTED,
  BOBGUI_SVG_ERROR_FEATURE_DISABLED,
} BobguiSvgError;

typedef struct
{
  size_t bytes;
  size_t lines;
  size_t line_chars;
} BobguiSvgLocation;

#define BOBGUI_SVG_ERROR (bobgui_svg_error_quark ())

GDK_AVAILABLE_IN_4_22
GQuark       bobgui_svg_error_quark               (void);
GDK_AVAILABLE_IN_4_22
const char * bobgui_svg_error_get_element     (const GError *error);
GDK_AVAILABLE_IN_4_22
const char * bobgui_svg_error_get_attribute   (const GError *error);
GDK_AVAILABLE_IN_4_22
const BobguiSvgLocation *
              bobgui_svg_error_get_start      (const GError *error);
GDK_AVAILABLE_IN_4_22
const BobguiSvgLocation *
              bobgui_svg_error_get_end        (const GError *error);

G_END_DECLS
