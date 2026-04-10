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

#include "bobguisvg.h"
#include "bobguienums.h"
#include "bobguibitmaskprivate.h"
#include "bobguicssnodeprivate.h"
#include "bobgui/svg/bobguisvgtypesprivate.h"
#include "bobgui/svg/bobguisvgenumsprivate.h"
#include "bobgui/svg/bobguisvgelementtypeprivate.h"
#include "bobgui/svg/bobguisvgfiltertypeprivate.h"
#include "bobgui/svg/bobguisvgpropertyprivate.h"

G_BEGIN_DECLS

#define DEFAULT_FONT_SIZE 13.333

struct _BobguiSvg
{
  GObject parent_instance;
  SvgElement *content;

  double current_width, current_height; /* Last snapshot size */

  double width, height; /* Intrinsic size */

  double weight;
  unsigned int initial_state;
  unsigned int state;
  unsigned int max_state;
  unsigned int n_state_names;
  char **state_names;
  int64_t state_change_delay;
  gboolean has_animations;
  BobguiSvgFeatures features;
  BobguiSvgUses used;

  char *resource;

  int64_t load_time;
  int64_t current_time;
  int64_t pause_time;

  BobguiOverflow overflow;
  gboolean playing;
  BobguiSvgRunMode run_mode;
  GdkFrameClock *clock;
  unsigned long clock_update_id;

  int64_t next_update;
  unsigned int pending_advance;
  gboolean advance_after_snapshot;

  unsigned int gpa_version;
  char *author;
  char *license;
  char *description;
  char *keywords;

  Timeline *timeline;

  GHashTable *images;

  PangoFontMap *fontmap;
  GPtrArray *font_files;

  GskRenderNode *node;

  struct {
    double width, height;
    GdkRGBA colors[5];
    size_t n_colors;
    double weight;
    int64_t time;
    unsigned int state;
  } node_for;
};

/* --- */

gboolean       bobgui_svg_set_state_names (BobguiSvg                *svg,
                                        const char           **names);

BobguiSvg *       bobgui_svg_copy            (BobguiSvg                *orig);

gboolean       bobgui_svg_equal           (BobguiSvg                *svg1,
                                        BobguiSvg                *svg2);

void           bobgui_svg_set_load_time   (BobguiSvg                *self,
                                        int64_t                load_time);

void           bobgui_svg_set_playing     (BobguiSvg                *self,
                                        gboolean               playing);

void           bobgui_svg_clear_content   (BobguiSvg                *self);

void           bobgui_svg_advance         (BobguiSvg                *self,
                                        int64_t                current_time);

BobguiSvgRunMode  bobgui_svg_get_run_mode    (BobguiSvg                *self);

int64_t        bobgui_svg_get_next_update (BobguiSvg                *self);

/*< private >
 * BobguiSvgSerializeFlags:
 * @BOBGUI_SVG_SERIALIZE_DEFAULT: Default behavior. Serialize
 *   the DOM, with gpa attributes, and with compatibility
 *   tweaks
 * @BOBGUI_SVG_SERIALIZE_AT_CURRENT_TIME: Serialize the current
 *   values of a running animation, as opposed to the DOM
 *   values that the parser produced
 * @BOBGUI_SVG_SERIALIZE_INCLUDE_STATE: Include custom attributes
 *   with various information about the state of the renderer,
 *   such as the current time, or the status of running animations
 * @BOBGUI_SVG_SERIALIZE_EXPAND_GPA_ATTRS: Instead of gpa attributes,
 *   include the animations that were generated from them
 * @BOBGUI_SVG_SERIALIZE_NO_COMPAT: Don't include things that
 *   improve the rendering of the serialized result in renderers
 *   which don't support extensions, but stick to the pristine
 *   DOM
 */
typedef enum
{
  BOBGUI_SVG_SERIALIZE_DEFAULT            = 0,
  BOBGUI_SVG_SERIALIZE_AT_CURRENT_TIME    = 1 << 0,
  BOBGUI_SVG_SERIALIZE_EXCLUDE_ANIMATION  = 1 << 1,
  BOBGUI_SVG_SERIALIZE_INCLUDE_STATE      = 1 << 2,
  BOBGUI_SVG_SERIALIZE_EXPAND_GPA_ATTRS   = 1 << 3,
  BOBGUI_SVG_SERIALIZE_NO_COMPAT          = 1 << 4,
} BobguiSvgSerializeFlags;

GBytes *       bobgui_svg_serialize_full  (BobguiSvg                *self,
                                        const GdkRGBA         *colors,
                                        size_t                 n_colors,
                                        BobguiSvgSerializeFlags   flags);

GskRenderNode *bobgui_svg_apply_filter    (BobguiSvg                *svg,
                                        const char            *filter,
                                        const graphene_rect_t *bounds,
                                        GskRenderNode         *node);

G_END_DECLS
