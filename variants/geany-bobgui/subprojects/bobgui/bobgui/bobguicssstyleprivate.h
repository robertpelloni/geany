/*
 * Copyright © 2012 Red Hat Inc.
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

#include <glib-object.h>
#include <bobgui/css/bobguicss.h>

#include "bobgui/bobguibitmaskprivate.h"
#include "bobgui/bobguicssvalueprivate.h"
#include "bobgui/bobguicssvariablesetprivate.h"
#include "bobgui/css/bobguicssvariablevalueprivate.h"

G_BEGIN_DECLS

#define BOBGUI_TYPE_CSS_STYLE           (bobgui_css_style_get_type ())
#define BOBGUI_CSS_STYLE(obj)           (G_TYPE_CHECK_INSTANCE_CAST (obj, BOBGUI_TYPE_CSS_STYLE, BobguiCssStyle))
#define BOBGUI_CSS_STYLE_CLASS(cls)     (G_TYPE_CHECK_CLASS_CAST (cls, BOBGUI_TYPE_CSS_STYLE, BobguiCssStyleClass))
#define BOBGUI_IS_CSS_STYLE(obj)        (G_TYPE_CHECK_INSTANCE_TYPE (obj, BOBGUI_TYPE_CSS_STYLE))
#define BOBGUI_IS_CSS_STYLE_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE (obj, BOBGUI_TYPE_CSS_STYLE))
#define BOBGUI_CSS_STYLE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_CSS_STYLE, BobguiCssStyleClass))

typedef enum {
  BOBGUI_CSS_CORE_VALUES,
  BOBGUI_CSS_CORE_INITIAL_VALUES,
  BOBGUI_CSS_BACKGROUND_VALUES,
  BOBGUI_CSS_BACKGROUND_INITIAL_VALUES,
  BOBGUI_CSS_BORDER_VALUES,
  BOBGUI_CSS_BORDER_INITIAL_VALUES,
  BOBGUI_CSS_ICON_VALUES,
  BOBGUI_CSS_ICON_INITIAL_VALUES,
  BOBGUI_CSS_OUTLINE_VALUES,
  BOBGUI_CSS_OUTLINE_INITIAL_VALUES,
  BOBGUI_CSS_FONT_VALUES,
  BOBGUI_CSS_FONT_INITIAL_VALUES,
  BOBGUI_CSS_TEXT_DECORATION_VALUES,
  BOBGUI_CSS_TEXT_DECORATION_INITIAL_VALUES,
  BOBGUI_CSS_FONT_VARIANT_VALUES,
  BOBGUI_CSS_FONT_VARIANT_INITIAL_VALUES,
  BOBGUI_CSS_ANIMATION_VALUES,
  BOBGUI_CSS_ANIMATION_INITIAL_VALUES,
  BOBGUI_CSS_TRANSITION_VALUES,
  BOBGUI_CSS_TRANSITION_INITIAL_VALUES,
  BOBGUI_CSS_SIZE_VALUES,
  BOBGUI_CSS_SIZE_INITIAL_VALUES,
  BOBGUI_CSS_OTHER_VALUES,
  BOBGUI_CSS_OTHER_INITIAL_VALUES,
  BOBGUI_CSS_USED_VALUES,
} BobguiCssValuesType;

typedef struct _BobguiCssValues BobguiCssValues;
typedef struct _BobguiCssCoreValues BobguiCssCoreValues;
typedef struct _BobguiCssBackgroundValues BobguiCssBackgroundValues;
typedef struct _BobguiCssBorderValues BobguiCssBorderValues;
typedef struct _BobguiCssIconValues BobguiCssIconValues;
typedef struct _BobguiCssOutlineValues BobguiCssOutlineValues;
typedef struct _BobguiCssFontValues BobguiCssFontValues;
typedef struct _BobguiCssTextDecorationValues BobguiCssTextDecorationValues;
typedef struct _BobguiCssFontVariantValues BobguiCssFontVariantValues;
typedef struct _BobguiCssAnimationValues BobguiCssAnimationValues;
typedef struct _BobguiCssTransitionValues BobguiCssTransitionValues;
typedef struct _BobguiCssSizeValues BobguiCssSizeValues;
typedef struct _BobguiCssOtherValues BobguiCssOtherValues;
typedef struct _BobguiCssUsedValues BobguiCssUsedValues;

struct _BobguiCssValues {
  int ref_count;
  BobguiCssValuesType type;
};

struct _BobguiCssCoreValues {
  BobguiCssValues base;

  BobguiCssValue *color;
  BobguiCssValue *dpi;
  BobguiCssValue *font_size;
  BobguiCssValue *icon_palette;
};

struct _BobguiCssBackgroundValues {
  BobguiCssValues base;

  BobguiCssValue *background_color;
  BobguiCssValue *box_shadow;
  BobguiCssValue *background_clip;
  BobguiCssValue *background_origin;
  BobguiCssValue *background_size;
  BobguiCssValue *background_position;
  BobguiCssValue *background_repeat;
  BobguiCssValue *background_image;
  BobguiCssValue *background_blend_mode;
};

struct _BobguiCssBorderValues {
  BobguiCssValues base;

  BobguiCssValue *border_top_style;
  BobguiCssValue *border_top_width;
  BobguiCssValue *border_left_style;
  BobguiCssValue *border_left_width;
  BobguiCssValue *border_bottom_style;
  BobguiCssValue *border_bottom_width;
  BobguiCssValue *border_right_style;
  BobguiCssValue *border_right_width;
  BobguiCssValue *border_top_left_radius;
  BobguiCssValue *border_top_right_radius;
  BobguiCssValue *border_bottom_right_radius;
  BobguiCssValue *border_bottom_left_radius;
  BobguiCssValue *border_top_color;
  BobguiCssValue *border_right_color;
  BobguiCssValue *border_bottom_color;
  BobguiCssValue *border_left_color;
  BobguiCssValue *border_image_source;
  BobguiCssValue *border_image_repeat;
  BobguiCssValue *border_image_slice;
  BobguiCssValue *border_image_width;
};

struct _BobguiCssIconValues {
  BobguiCssValues base;

  BobguiCssValue *icon_size;
  BobguiCssValue *icon_shadow;
  BobguiCssValue *icon_style;
  BobguiCssValue *icon_weight;
};


struct _BobguiCssOutlineValues {
  BobguiCssValues base;

  BobguiCssValue *outline_style;
  BobguiCssValue *outline_width;
  BobguiCssValue *outline_offset;
  BobguiCssValue *outline_color;
};

struct _BobguiCssFontValues {
  BobguiCssValues base;

  BobguiCssValue *font_family;
  BobguiCssValue *font_style;
  BobguiCssValue *font_weight;
  BobguiCssValue *font_stretch;
  BobguiCssValue *letter_spacing;
  BobguiCssValue *text_shadow;
  BobguiCssValue *caret_color;
  BobguiCssValue *secondary_caret_color;
  BobguiCssValue *font_feature_settings;
  BobguiCssValue *font_variation_settings;
  BobguiCssValue *line_height;
};

struct _BobguiCssTextDecorationValues {
  BobguiCssValues base;

  BobguiCssValue *text_decoration_line;
  BobguiCssValue *text_decoration_color;
  BobguiCssValue *text_decoration_style;
};

struct _BobguiCssFontVariantValues {
  BobguiCssValues base;

  BobguiCssValue *text_transform;
  BobguiCssValue *font_kerning;
  BobguiCssValue *font_variant_ligatures;
  BobguiCssValue *font_variant_position;
  BobguiCssValue *font_variant_caps;
  BobguiCssValue *font_variant_numeric;
  BobguiCssValue *font_variant_alternates;
  BobguiCssValue *font_variant_east_asian;
};

struct _BobguiCssAnimationValues {
  BobguiCssValues base;

  BobguiCssValue *animation_name;
  BobguiCssValue *animation_duration;
  BobguiCssValue *animation_timing_function;
  BobguiCssValue *animation_iteration_count;
  BobguiCssValue *animation_direction;
  BobguiCssValue *animation_play_state;
  BobguiCssValue *animation_delay;
  BobguiCssValue *animation_fill_mode;
};

struct _BobguiCssTransitionValues {
  BobguiCssValues base;

  BobguiCssValue *transition_property;
  BobguiCssValue *transition_duration;
  BobguiCssValue *transition_timing_function;
  BobguiCssValue *transition_delay;
};

struct _BobguiCssSizeValues {
  BobguiCssValues base;

  BobguiCssValue *margin_top;
  BobguiCssValue *margin_left;
  BobguiCssValue *margin_bottom;
  BobguiCssValue *margin_right;
  BobguiCssValue *padding_top;
  BobguiCssValue *padding_left;
  BobguiCssValue *padding_bottom;
  BobguiCssValue *padding_right;
  BobguiCssValue *border_spacing;
  BobguiCssValue *min_width;
  BobguiCssValue *min_height;
};

struct _BobguiCssOtherValues {
  BobguiCssValues base;

  BobguiCssValue *icon_source;
  BobguiCssValue *icon_transform;
  BobguiCssValue *icon_filter;
  BobguiCssValue *transform;
  BobguiCssValue *transform_origin;
  BobguiCssValue *opacity;
  BobguiCssValue *backdrop_filter;
  BobguiCssValue *filter;
};

struct _BobguiCssUsedValues {
  BobguiCssValues base;

  BobguiCssValue *color;
  BobguiCssValue *icon_palette;
  BobguiCssValue *background_color;
  BobguiCssValue *box_shadow;
  BobguiCssValue *background_image;
  BobguiCssValue *border_top_color;
  BobguiCssValue *border_right_color;
  BobguiCssValue *border_bottom_color;
  BobguiCssValue *border_left_color;
  BobguiCssValue *border_image_source;
  BobguiCssValue *icon_shadow;
  BobguiCssValue *outline_color;
  BobguiCssValue *caret_color;
  BobguiCssValue *secondary_caret_color;
  BobguiCssValue *text_shadow;
  BobguiCssValue *text_decoration_color;
  BobguiCssValue *icon_source;
};

/* typedef struct _BobguiCssStyle           BobguiCssStyle; */
typedef struct _BobguiCssStyleClass      BobguiCssStyleClass;

struct _BobguiCssStyle
{
  GObject parent;

  BobguiCssCoreValues           *core;
  BobguiCssBackgroundValues     *background;
  BobguiCssBorderValues         *border;
  BobguiCssIconValues           *icon;
  BobguiCssOutlineValues        *outline;
  BobguiCssFontValues           *font;
  BobguiCssTextDecorationValues *text_decoration;
  BobguiCssFontVariantValues    *font_variant;
  BobguiCssAnimationValues      *animation;
  BobguiCssTransitionValues     *transition;
  BobguiCssSizeValues           *size;
  BobguiCssOtherValues          *other;
  BobguiCssUsedValues           *used;

  BobguiCssVariableSet          *variables;

  BobguiCssValue                *variable_values;
  int                         n_variable_values;
};

struct _BobguiCssStyleClass
{
  GObjectClass parent_class;

  /* Get the section the value at the given id was declared at or NULL if unavailable.
   * Optional: default impl will just return NULL */
  BobguiCssSection *       (* get_section)                         (BobguiCssStyle            *style,
                                                                 guint                   id);
  /* TRUE if this style will require changes based on timestamp */
  gboolean              (* is_static)                           (BobguiCssStyle            *style);

  BobguiCssStaticStyle *   (* get_static_style)                    (BobguiCssStyle            *style);

  BobguiCssValue *         (* get_original_value)                  (BobguiCssStyle            *style,
                                                                 guint                   id);
};

GType                   bobgui_css_style_get_type                  (void) G_GNUC_CONST;

BobguiCssValue *           bobgui_css_style_get_value                 (BobguiCssStyle            *style,
                                                                 guint                   id) G_GNUC_PURE;
BobguiCssValue *           bobgui_css_style_get_computed_value        (BobguiCssStyle            *style,
                                                                 guint                   id) G_GNUC_PURE;
BobguiCssValue *           bobgui_css_style_get_used_value            (BobguiCssStyle            *style,
                                                                 guint                   id) G_GNUC_PURE;
BobguiCssSection *         bobgui_css_style_get_section               (BobguiCssStyle            *style,
                                                                 guint                   id) G_GNUC_PURE;
gboolean                bobgui_css_style_is_static                 (BobguiCssStyle            *style) G_GNUC_PURE;
BobguiCssStaticStyle *     bobgui_css_style_get_static_style          (BobguiCssStyle            *style);

BobguiCssValue *           bobgui_css_style_get_original_value        (BobguiCssStyle            *style,
                                                                 guint                   id) G_GNUC_PURE;

char *                  bobgui_css_style_to_string                 (BobguiCssStyle            *style);
gboolean                bobgui_css_style_print                     (BobguiCssStyle            *style,
                                                                 GString                *string,
                                                                 guint                   indent,
                                                                 gboolean                skip_initial);

PangoTextTransform      bobgui_css_style_get_pango_text_transform  (BobguiCssStyle            *style);
char *                  bobgui_css_style_compute_font_features     (BobguiCssStyle            *style);
PangoAttrList *         bobgui_css_style_get_pango_attributes      (BobguiCssStyle            *style);
PangoFontDescription *  bobgui_css_style_get_pango_font            (BobguiCssStyle            *style);

void                    bobgui_css_style_lookup_symbolic_colors    (BobguiCssStyle            *style,
                                                                 GdkRGBA                 color_out[5]);

BobguiCssVariableValue *   bobgui_css_style_get_custom_property       (BobguiCssStyle            *style,
                                                                 int                     id);
GArray *                bobgui_css_style_list_custom_properties    (BobguiCssStyle            *style);

BobguiCssValue *           bobgui_css_style_resolve_used_value        (BobguiCssStyle            *style,
                                                                 BobguiCssValue            *value,
                                                                 guint                   property_id,
                                                                 BobguiCssComputeContext   *context);
void                    bobgui_css_style_resolve_used_values       (BobguiCssStyle            *style,
                                                                 BobguiCssComputeContext   *context);

BobguiCssValues *bobgui_css_values_new   (BobguiCssValuesType  type);
BobguiCssValues *bobgui_css_values_ref   (BobguiCssValues     *values);
void          bobgui_css_values_unref (BobguiCssValues     *values);
BobguiCssValues *bobgui_css_values_copy  (BobguiCssValues     *values);

void bobgui_css_core_values_compute_changes_and_affects (BobguiCssStyle *style1,
                                                      BobguiCssStyle *style2,
                                                      BobguiBitmask    **changes,
                                                      BobguiCssAffects *affects);
void bobgui_css_background_values_compute_changes_and_affects (BobguiCssStyle *style1,
                                                      BobguiCssStyle *style2,
                                                      BobguiBitmask    **changes,
                                                      BobguiCssAffects *affects);
void bobgui_css_border_values_compute_changes_and_affects (BobguiCssStyle *style1,
                                                      BobguiCssStyle *style2,
                                                      BobguiBitmask    **changes,
                                                      BobguiCssAffects *affects);
void bobgui_css_icon_values_compute_changes_and_affects (BobguiCssStyle *style1,
                                                      BobguiCssStyle *style2,
                                                      BobguiBitmask    **changes,
                                                      BobguiCssAffects *affects);
void bobgui_css_outline_values_compute_changes_and_affects (BobguiCssStyle *style1,
                                                      BobguiCssStyle *style2,
                                                      BobguiBitmask    **changes,
                                                      BobguiCssAffects *affects);
void bobgui_css_font_values_compute_changes_and_affects (BobguiCssStyle *style1,
                                                      BobguiCssStyle *style2,
                                                      BobguiBitmask    **changes,
                                                      BobguiCssAffects *affects);
void bobgui_css_text_decoration_values_compute_changes_and_affects (BobguiCssStyle *style1,
                                                      BobguiCssStyle *style2,
                                                      BobguiBitmask    **changes,
                                                      BobguiCssAffects *affects);
void bobgui_css_font_variant_values_compute_changes_and_affects (BobguiCssStyle *style1,
                                                      BobguiCssStyle *style2,
                                                      BobguiBitmask    **changes,
                                                      BobguiCssAffects *affects);
void bobgui_css_animation_values_compute_changes_and_affects (BobguiCssStyle *style1,
                                                      BobguiCssStyle *style2,
                                                      BobguiBitmask    **changes,
                                                      BobguiCssAffects *affects);
void bobgui_css_transition_values_compute_changes_and_affects (BobguiCssStyle *style1,
                                                      BobguiCssStyle *style2,
                                                      BobguiBitmask    **changes,
                                                      BobguiCssAffects *affects);
void bobgui_css_size_values_compute_changes_and_affects (BobguiCssStyle *style1,
                                                      BobguiCssStyle *style2,
                                                      BobguiBitmask    **changes,
                                                      BobguiCssAffects *affects);
void bobgui_css_other_values_compute_changes_and_affects (BobguiCssStyle *style1,
                                                      BobguiCssStyle *style2,
                                                      BobguiBitmask    **changes,
                                                      BobguiCssAffects *affects);
void bobgui_css_custom_values_compute_changes_and_affects (BobguiCssStyle *style1,
                                                      BobguiCssStyle *style2,
                                                      BobguiBitmask    **changes,
                                                      BobguiCssAffects *affects);

G_END_DECLS

