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

#include "config.h"

#include "bobguicssanimatedstyleprivate.h"

#include "bobguicssanimationprivate.h"
#include "bobguicssarrayvalueprivate.h"
#include "bobguicssdynamicprivate.h"
#include "bobguicssenumvalueprivate.h"
#include "bobguicssinheritvalueprivate.h"
#include "bobguicssinitialvalueprivate.h"
#include "bobguicssnumbervalueprivate.h"
#include "bobguicssshorthandpropertyprivate.h"
#include "bobguicssstaticstyleprivate.h"
#include "bobguicssstringvalueprivate.h"
#include "bobguicssstylepropertyprivate.h"
#include "bobguicsstransitionprivate.h"
#include "bobguicssvaluesprivate.h"
#include "bobguiprivate.h"
#include "bobguistyleanimationprivate.h"
#include "bobguistylepropertyprivate.h"
#include "bobguistyleproviderprivate.h"
#include "bobguicsscustompropertypoolprivate.h"

G_DEFINE_TYPE (BobguiCssAnimatedStyle, bobgui_css_animated_style, BOBGUI_TYPE_CSS_STYLE)


#define DEFINE_VALUES(ENUM, TYPE, NAME) \
static inline void \
bobgui_css_ ## NAME ## _values_recompute (BobguiCssAnimatedStyle   *animated, \
                                       BobguiCssComputeContext  *context) \
{ \
  BobguiCssStyle *style = (BobguiCssStyle *)animated; \
  int i; \
\
  for (i = 0; i < G_N_ELEMENTS (NAME ## _props); i++) \
    { \
      guint id = NAME ## _props[i]; \
      BobguiCssValue *original, *computed; \
\
      original = bobgui_css_style_get_original_value (style, id); \
      if (original == NULL) \
        continue; \
\
      if (!bobgui_css_value_contains_variables (original)) \
        continue; \
\
      computed = bobgui_css_value_compute (original, id, context); \
      if (computed == NULL) \
        continue; \
\
      bobgui_css_animated_style_set_animated_value (animated, id, computed); \
    } \
}

DEFINE_VALUES (CORE, Core, core)
DEFINE_VALUES (BACKGROUND, Background, background)
DEFINE_VALUES (BORDER, Border, border)
DEFINE_VALUES (ICON, Icon, icon)
DEFINE_VALUES (OUTLINE, Outline, outline)
DEFINE_VALUES (FONT, Font, font)
DEFINE_VALUES (TEXT_DECORATION, TextDecoration, text_decoration)
DEFINE_VALUES (FONT_VARIANT, FontVariant, font_variant)
DEFINE_VALUES (ANIMATION, Animation, animation)
DEFINE_VALUES (TRANSITION, Transition, transition)
DEFINE_VALUES (SIZE, Size, size)
DEFINE_VALUES (OTHER, Other, other)

static BobguiCssSection *
bobgui_css_animated_style_get_section (BobguiCssStyle *style,
                                    guint        id)
{
  BobguiCssAnimatedStyle *animated = BOBGUI_CSS_ANIMATED_STYLE (style);

  return bobgui_css_style_get_section (animated->style, id);
}

static gboolean
bobgui_css_animated_style_is_static (BobguiCssStyle *style)
{
  BobguiCssAnimatedStyle *animated = (BobguiCssAnimatedStyle *)style;
  guint i;

  for (i = 0; i < animated->n_animations; i ++)
    {
      if (!_bobgui_style_animation_is_static (animated->animations[i]))
        return FALSE;
    }

  return TRUE;
}

static BobguiCssStaticStyle *
bobgui_css_animated_style_get_static_style (BobguiCssStyle *style)
{
  /* This is called a lot, so we avoid a dynamic type check here */
  BobguiCssAnimatedStyle *animated = (BobguiCssAnimatedStyle *) style;

  return (BobguiCssStaticStyle *)animated->style;
}

static BobguiCssValue *
bobgui_css_animated_style_get_original_value (BobguiCssStyle *style,
                                           guint        id)
{
  BobguiCssAnimatedStyle *animated = BOBGUI_CSS_ANIMATED_STYLE (style);

  return bobgui_css_style_get_original_value (animated->style, id);
}

static void
bobgui_css_animated_style_dispose (GObject *object)
{
  BobguiCssAnimatedStyle *style = BOBGUI_CSS_ANIMATED_STYLE (object);
  guint i;

  for (i = 0; i < style->n_animations; i ++)
    bobgui_style_animation_unref (style->animations[i]);

  style->n_animations = 0;
  g_free (style->animations);
  style->animations = NULL;

  G_OBJECT_CLASS (bobgui_css_animated_style_parent_class)->dispose (object);
}

static void
bobgui_css_animated_style_finalize (GObject *object)
{
  BobguiCssAnimatedStyle *style = BOBGUI_CSS_ANIMATED_STYLE (object);

  g_object_unref (style->style);
  if (style->parent_style)
    g_object_unref (style->parent_style);
  g_object_unref (style->provider);

  G_OBJECT_CLASS (bobgui_css_animated_style_parent_class)->finalize (object);
}

static void
bobgui_css_animated_style_class_init (BobguiCssAnimatedStyleClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  BobguiCssStyleClass *style_class = BOBGUI_CSS_STYLE_CLASS (klass);

  object_class->dispose = bobgui_css_animated_style_dispose;
  object_class->finalize = bobgui_css_animated_style_finalize;

  style_class->get_section = bobgui_css_animated_style_get_section;
  style_class->is_static = bobgui_css_animated_style_is_static;
  style_class->get_static_style = bobgui_css_animated_style_get_static_style;
  style_class->get_original_value = bobgui_css_animated_style_get_original_value;
}

static void
bobgui_css_animated_style_init (BobguiCssAnimatedStyle *style)
{
}

#define DEFINE_UNSHARE(TYPE, NAME) \
static inline void \
unshare_ ## NAME (BobguiCssAnimatedStyle *animated) \
{ \
  BobguiCssStyle *style = (BobguiCssStyle *)animated; \
  if (style->NAME == animated->style->NAME) \
    { \
      bobgui_css_values_unref ((BobguiCssValues *)style->NAME); \
      style->NAME = (TYPE *)bobgui_css_values_copy ((BobguiCssValues *)animated->style->NAME); \
    } \
}

DEFINE_UNSHARE (BobguiCssCoreValues, core)
DEFINE_UNSHARE (BobguiCssBackgroundValues, background)
DEFINE_UNSHARE (BobguiCssBorderValues, border)
DEFINE_UNSHARE (BobguiCssIconValues, icon)
DEFINE_UNSHARE (BobguiCssOutlineValues, outline)
DEFINE_UNSHARE (BobguiCssFontValues, font)
DEFINE_UNSHARE (BobguiCssTextDecorationValues, text_decoration)
DEFINE_UNSHARE (BobguiCssFontVariantValues, font_variant)
DEFINE_UNSHARE (BobguiCssAnimationValues, animation)
DEFINE_UNSHARE (BobguiCssTransitionValues, transition)
DEFINE_UNSHARE (BobguiCssSizeValues, size)
DEFINE_UNSHARE (BobguiCssOtherValues, other)

static inline void
bobgui_css_take_value (BobguiCssValue **variable,
                    BobguiCssValue  *value)
{
  if (*variable)
    bobgui_css_value_unref (*variable);
  *variable = value;
}

void
bobgui_css_animated_style_set_animated_value (BobguiCssAnimatedStyle *animated,
                                           guint                id,
                                           BobguiCssValue         *value)
{
  BobguiCssStyle *style = (BobguiCssStyle *)animated;

  bobgui_internal_return_if_fail (BOBGUI_IS_CSS_ANIMATED_STYLE (style));
  bobgui_internal_return_if_fail (value != NULL);

  switch (id)
    {
    case BOBGUI_CSS_PROPERTY_COLOR:
      unshare_core (animated);
      bobgui_css_take_value (&style->core->color, value);
      break;
    case BOBGUI_CSS_PROPERTY_DPI:
      unshare_core (animated);
      bobgui_css_take_value (&style->core->dpi, value);
      break;
    case BOBGUI_CSS_PROPERTY_FONT_SIZE:
      unshare_core (animated);
      bobgui_css_take_value (&style->core->font_size, value);
      break;
    case BOBGUI_CSS_PROPERTY_ICON_PALETTE:
      unshare_core (animated);
      bobgui_css_take_value (&style->core->icon_palette, value);
      break;
    case BOBGUI_CSS_PROPERTY_BACKGROUND_COLOR:
      unshare_background (animated);
      bobgui_css_take_value (&style->background->background_color, value);
      break;
    case BOBGUI_CSS_PROPERTY_FONT_FAMILY:
      unshare_font (animated);
      bobgui_css_take_value (&style->font->font_family, value);
      break;
    case BOBGUI_CSS_PROPERTY_FONT_STYLE:
      unshare_font (animated);
      bobgui_css_take_value (&style->font->font_style, value);
      break;
    case BOBGUI_CSS_PROPERTY_FONT_WEIGHT:
      unshare_font (animated);
      bobgui_css_take_value (&style->font->font_weight, value);
      break;
    case BOBGUI_CSS_PROPERTY_FONT_STRETCH:
      unshare_font (animated);
      bobgui_css_take_value (&style->font->font_stretch, value);
      break;
    case BOBGUI_CSS_PROPERTY_LETTER_SPACING:
      unshare_font (animated);
      bobgui_css_take_value (&style->font->letter_spacing, value);
      break;
    case BOBGUI_CSS_PROPERTY_LINE_HEIGHT:
      unshare_font (animated);
      bobgui_css_take_value (&style->font->line_height, value);
      break;
    case BOBGUI_CSS_PROPERTY_TEXT_DECORATION_LINE:
      unshare_text_decoration (animated);
      bobgui_css_take_value (&style->text_decoration->text_decoration_line, value);
      break;
    case BOBGUI_CSS_PROPERTY_TEXT_DECORATION_COLOR:
      unshare_text_decoration (animated);
      bobgui_css_take_value (&style->text_decoration->text_decoration_color, value);
      break;
    case BOBGUI_CSS_PROPERTY_TEXT_DECORATION_STYLE:
      unshare_text_decoration (animated);
      bobgui_css_take_value (&style->text_decoration->text_decoration_style, value);
      break;
    case BOBGUI_CSS_PROPERTY_TEXT_TRANSFORM:
      unshare_font_variant (animated);
      bobgui_css_take_value (&style->font_variant->text_transform, value);
      break;
    case BOBGUI_CSS_PROPERTY_FONT_KERNING:
      unshare_font_variant (animated);
      bobgui_css_take_value (&style->font_variant->font_kerning, value);
      break;
    case BOBGUI_CSS_PROPERTY_FONT_VARIANT_LIGATURES:
      unshare_font_variant (animated);
      bobgui_css_take_value (&style->font_variant->font_variant_ligatures, value);
      break;
    case BOBGUI_CSS_PROPERTY_FONT_VARIANT_POSITION:
      unshare_font_variant (animated);
      bobgui_css_take_value (&style->font_variant->font_variant_position, value);
      break;
    case BOBGUI_CSS_PROPERTY_FONT_VARIANT_CAPS:
      unshare_font_variant (animated);
      bobgui_css_take_value (&style->font_variant->font_variant_caps, value);
      break;
    case BOBGUI_CSS_PROPERTY_FONT_VARIANT_NUMERIC:
      unshare_font_variant (animated);
      bobgui_css_take_value (&style->font_variant->font_variant_numeric, value);
      break;
    case BOBGUI_CSS_PROPERTY_FONT_VARIANT_ALTERNATES:
      unshare_font_variant (animated);
      bobgui_css_take_value (&style->font_variant->font_variant_alternates, value);
      break;
    case BOBGUI_CSS_PROPERTY_FONT_VARIANT_EAST_ASIAN:
      unshare_font_variant (animated);
      bobgui_css_take_value (&style->font_variant->font_variant_east_asian, value);
      break;
    case BOBGUI_CSS_PROPERTY_TEXT_SHADOW:
      unshare_font (animated);
      bobgui_css_take_value (&style->font->text_shadow, value);
      break;
    case BOBGUI_CSS_PROPERTY_BOX_SHADOW:
      unshare_background (animated);
      bobgui_css_take_value (&style->background->box_shadow, value);
      break;
    case BOBGUI_CSS_PROPERTY_MARGIN_TOP:
      unshare_size (animated);
      bobgui_css_take_value (&style->size->margin_top, value);
      break;
    case BOBGUI_CSS_PROPERTY_MARGIN_LEFT:
      unshare_size (animated);
      bobgui_css_take_value (&style->size->margin_left, value);
      break;
    case BOBGUI_CSS_PROPERTY_MARGIN_BOTTOM:
      unshare_size (animated);
      bobgui_css_take_value (&style->size->margin_bottom, value);
      break;
    case BOBGUI_CSS_PROPERTY_MARGIN_RIGHT:
      unshare_size (animated);
      bobgui_css_take_value (&style->size->margin_right, value);
      break;
    case BOBGUI_CSS_PROPERTY_PADDING_TOP:
      unshare_size (animated);
      bobgui_css_take_value (&style->size->padding_top, value);
      break;
    case BOBGUI_CSS_PROPERTY_PADDING_LEFT:
      unshare_size (animated);
      bobgui_css_take_value (&style->size->padding_left, value);
      break;
    case BOBGUI_CSS_PROPERTY_PADDING_BOTTOM:
      unshare_size (animated);
      bobgui_css_take_value (&style->size->padding_bottom, value);
      break;
    case BOBGUI_CSS_PROPERTY_PADDING_RIGHT:
      unshare_size (animated);
      bobgui_css_take_value (&style->size->padding_right, value);
      break;
    case BOBGUI_CSS_PROPERTY_BORDER_TOP_STYLE:
      unshare_border (animated);
      bobgui_css_take_value (&style->border->border_top_style, value);
      break;
    case BOBGUI_CSS_PROPERTY_BORDER_TOP_WIDTH:
      unshare_border (animated);
      bobgui_css_take_value (&style->border->border_top_width, value);
      break;
    case BOBGUI_CSS_PROPERTY_BORDER_LEFT_STYLE:
      unshare_border (animated);
      bobgui_css_take_value (&style->border->border_left_style, value);
      break;
    case BOBGUI_CSS_PROPERTY_BORDER_LEFT_WIDTH:
      unshare_border (animated);
      bobgui_css_take_value (&style->border->border_left_width, value);
      break;
    case BOBGUI_CSS_PROPERTY_BORDER_BOTTOM_STYLE:
      unshare_border (animated);
      bobgui_css_take_value (&style->border->border_bottom_style, value);
      break;
    case BOBGUI_CSS_PROPERTY_BORDER_BOTTOM_WIDTH:
      unshare_border (animated);
      bobgui_css_take_value (&style->border->border_bottom_width, value);
      break;
    case BOBGUI_CSS_PROPERTY_BORDER_RIGHT_STYLE:
      unshare_border (animated);
      bobgui_css_take_value (&style->border->border_right_style, value);
      break;
    case BOBGUI_CSS_PROPERTY_BORDER_RIGHT_WIDTH:
      unshare_border (animated);
      bobgui_css_take_value (&style->border->border_right_width, value);
      break;
    case BOBGUI_CSS_PROPERTY_BORDER_TOP_LEFT_RADIUS:
      unshare_border (animated);
      bobgui_css_take_value (&style->border->border_top_left_radius, value);
      break;
    case BOBGUI_CSS_PROPERTY_BORDER_TOP_RIGHT_RADIUS:
      unshare_border (animated);
      bobgui_css_take_value (&style->border->border_top_right_radius, value);
      break;
    case BOBGUI_CSS_PROPERTY_BORDER_BOTTOM_RIGHT_RADIUS:
      unshare_border (animated);
      bobgui_css_take_value (&style->border->border_bottom_right_radius, value);
      break;
    case BOBGUI_CSS_PROPERTY_BORDER_BOTTOM_LEFT_RADIUS:
      unshare_border (animated);
      bobgui_css_take_value (&style->border->border_bottom_left_radius, value);
      break;
    case BOBGUI_CSS_PROPERTY_OUTLINE_STYLE:
      unshare_outline (animated);
      bobgui_css_take_value (&style->outline->outline_style, value);
      break;
    case BOBGUI_CSS_PROPERTY_OUTLINE_WIDTH:
      unshare_outline (animated);
      bobgui_css_take_value (&style->outline->outline_width, value);
      break;
    case BOBGUI_CSS_PROPERTY_OUTLINE_OFFSET:
      unshare_outline (animated);
      bobgui_css_take_value (&style->outline->outline_offset, value);
      break;
    case BOBGUI_CSS_PROPERTY_BACKGROUND_CLIP:
      unshare_background (animated);
      bobgui_css_take_value (&style->background->background_clip, value);
      break;
    case BOBGUI_CSS_PROPERTY_BACKGROUND_ORIGIN:
      unshare_background (animated);
      bobgui_css_take_value (&style->background->background_origin, value);
      break;
    case BOBGUI_CSS_PROPERTY_BACKGROUND_SIZE:
      unshare_background (animated);
      bobgui_css_take_value (&style->background->background_size, value);
      break;
    case BOBGUI_CSS_PROPERTY_BACKGROUND_POSITION:
      unshare_background (animated);
      bobgui_css_take_value (&style->background->background_position, value);
      break;
    case BOBGUI_CSS_PROPERTY_BORDER_TOP_COLOR:
      unshare_border (animated);
      bobgui_css_take_value (&style->border->border_top_color, value);
      break;
    case BOBGUI_CSS_PROPERTY_BORDER_RIGHT_COLOR:
      unshare_border (animated);
      bobgui_css_take_value (&style->border->border_right_color, value);
      break;
    case BOBGUI_CSS_PROPERTY_BORDER_BOTTOM_COLOR:
      unshare_border (animated);
      bobgui_css_take_value (&style->border->border_bottom_color, value);
      break;
    case BOBGUI_CSS_PROPERTY_BORDER_LEFT_COLOR:
      unshare_border (animated);
      bobgui_css_take_value (&style->border->border_left_color, value);
      break;
    case BOBGUI_CSS_PROPERTY_OUTLINE_COLOR:
      unshare_outline (animated);
      bobgui_css_take_value (&style->outline->outline_color, value);
      break;
    case BOBGUI_CSS_PROPERTY_BACKGROUND_REPEAT:
      unshare_background (animated);
      bobgui_css_take_value (&style->background->background_repeat, value);
      break;
    case BOBGUI_CSS_PROPERTY_BACKGROUND_IMAGE:
      unshare_background (animated);
      bobgui_css_take_value (&style->background->background_image, value);
      break;
    case BOBGUI_CSS_PROPERTY_BACKGROUND_BLEND_MODE:
      unshare_background (animated);
      bobgui_css_take_value (&style->background->background_blend_mode, value);
      break;
    case BOBGUI_CSS_PROPERTY_BORDER_IMAGE_SOURCE:
      unshare_border (animated);
      bobgui_css_take_value (&style->border->border_image_source, value);
      break;
    case BOBGUI_CSS_PROPERTY_BORDER_IMAGE_REPEAT:
      unshare_border (animated);
      bobgui_css_take_value (&style->border->border_image_repeat, value);
      break;
    case BOBGUI_CSS_PROPERTY_BORDER_IMAGE_SLICE:
      unshare_border (animated);
      bobgui_css_take_value (&style->border->border_image_slice, value);
      break;
    case BOBGUI_CSS_PROPERTY_BORDER_IMAGE_WIDTH:
      unshare_border (animated);
      bobgui_css_take_value (&style->border->border_image_width, value);
      break;
    case BOBGUI_CSS_PROPERTY_ICON_SOURCE:
      unshare_other (animated);
      bobgui_css_take_value (&style->other->icon_source, value);
      break;
    case BOBGUI_CSS_PROPERTY_ICON_SIZE:
      unshare_icon (animated);
      bobgui_css_take_value (&style->icon->icon_size, value);
      break;
    case BOBGUI_CSS_PROPERTY_ICON_SHADOW:
      unshare_icon (animated);
      bobgui_css_take_value (&style->icon->icon_shadow, value);
      break;
    case BOBGUI_CSS_PROPERTY_ICON_STYLE:
      unshare_icon (animated);
      bobgui_css_take_value (&style->icon->icon_style, value);
      break;
    case BOBGUI_CSS_PROPERTY_ICON_TRANSFORM:
      unshare_other (animated);
      bobgui_css_take_value (&style->other->icon_transform, value);
      break;
    case BOBGUI_CSS_PROPERTY_ICON_FILTER:
      unshare_other (animated);
      bobgui_css_take_value (&style->other->icon_filter, value);
      break;
    case BOBGUI_CSS_PROPERTY_ICON_WEIGHT:
      unshare_icon (animated);
      bobgui_css_take_value (&style->icon->icon_weight, value);
      break;
    case BOBGUI_CSS_PROPERTY_BORDER_SPACING:
      unshare_size (animated);
      bobgui_css_take_value (&style->size->border_spacing, value);
      break;
    case BOBGUI_CSS_PROPERTY_TRANSFORM:
      unshare_other (animated);
      bobgui_css_take_value (&style->other->transform, value);
      break;
    case BOBGUI_CSS_PROPERTY_TRANSFORM_ORIGIN:
      unshare_other (animated);
      bobgui_css_take_value (&style->other->transform_origin, value);
      break;
    case BOBGUI_CSS_PROPERTY_MIN_WIDTH:
      unshare_size (animated);
      bobgui_css_take_value (&style->size->min_width, value);
      break;
    case BOBGUI_CSS_PROPERTY_MIN_HEIGHT:
      unshare_size (animated);
      bobgui_css_take_value (&style->size->min_height, value);
      break;
    case BOBGUI_CSS_PROPERTY_TRANSITION_PROPERTY:
      unshare_transition (animated);
      bobgui_css_take_value (&style->transition->transition_property, value);
      break;
    case BOBGUI_CSS_PROPERTY_TRANSITION_DURATION:
      unshare_transition (animated);
      bobgui_css_take_value (&style->transition->transition_duration, value);
      break;
    case BOBGUI_CSS_PROPERTY_TRANSITION_TIMING_FUNCTION:
      unshare_transition (animated);
      bobgui_css_take_value (&style->transition->transition_timing_function, value);
      break;
    case BOBGUI_CSS_PROPERTY_TRANSITION_DELAY:
      unshare_transition (animated);
      bobgui_css_take_value (&style->transition->transition_delay, value);
      break;
    case BOBGUI_CSS_PROPERTY_ANIMATION_NAME:
      unshare_animation (animated);
      bobgui_css_take_value (&style->animation->animation_name, value);
      break;
    case BOBGUI_CSS_PROPERTY_ANIMATION_DURATION:
      unshare_animation (animated);
      bobgui_css_take_value (&style->animation->animation_duration, value);
      break;
    case BOBGUI_CSS_PROPERTY_ANIMATION_TIMING_FUNCTION:
      unshare_animation (animated);
      bobgui_css_take_value (&style->animation->animation_timing_function, value);
      break;
    case BOBGUI_CSS_PROPERTY_ANIMATION_ITERATION_COUNT:
      unshare_animation (animated);
      bobgui_css_take_value (&style->animation->animation_iteration_count, value);
      break;
    case BOBGUI_CSS_PROPERTY_ANIMATION_DIRECTION:
      unshare_animation (animated);
      bobgui_css_take_value (&style->animation->animation_direction, value);
      break;
    case BOBGUI_CSS_PROPERTY_ANIMATION_PLAY_STATE:
      unshare_animation (animated);
      bobgui_css_take_value (&style->animation->animation_play_state, value);
      break;
    case BOBGUI_CSS_PROPERTY_ANIMATION_DELAY:
      unshare_animation (animated);
      bobgui_css_take_value (&style->animation->animation_delay, value);
      break;
    case BOBGUI_CSS_PROPERTY_ANIMATION_FILL_MODE:
      unshare_animation (animated);
      bobgui_css_take_value (&style->animation->animation_fill_mode, value);
      break;
    case BOBGUI_CSS_PROPERTY_OPACITY:
      unshare_other (animated);
      bobgui_css_take_value (&style->other->opacity, value);
      break;
    case BOBGUI_CSS_PROPERTY_BACKDROP_FILTER:
      unshare_other (animated);
      bobgui_css_take_value (&style->other->backdrop_filter, value);
      break;
    case BOBGUI_CSS_PROPERTY_FILTER:
      unshare_other (animated);
      bobgui_css_take_value (&style->other->filter, value);
      break;
    case BOBGUI_CSS_PROPERTY_CARET_COLOR:
      unshare_font (animated);
      bobgui_css_take_value (&style->font->caret_color, value);
      break;
    case BOBGUI_CSS_PROPERTY_SECONDARY_CARET_COLOR:
      unshare_font (animated);
      bobgui_css_take_value (&style->font->secondary_caret_color, value);
      break;
    case BOBGUI_CSS_PROPERTY_FONT_FEATURE_SETTINGS:
      unshare_font (animated);
      bobgui_css_take_value (&style->font->font_feature_settings, value);
      break;
    case BOBGUI_CSS_PROPERTY_FONT_VARIATION_SETTINGS:
      unshare_font (animated);
      bobgui_css_take_value (&style->font->font_variation_settings, value);
      break;

    default:
      g_assert_not_reached ();
      break;
    }
}

BobguiCssValue *
bobgui_css_animated_style_get_intrinsic_value (BobguiCssAnimatedStyle *style,
                                            guint                id)
{
  return bobgui_css_style_get_value (style->style, id);
}

gboolean
bobgui_css_animated_style_set_animated_custom_value (BobguiCssAnimatedStyle *animated,
                                                  int                  id,
                                                  BobguiCssVariableValue *value)
{
  BobguiCssStyle *style = (BobguiCssStyle *)animated;
  BobguiCssVariableValue *old_value;

  bobgui_internal_return_val_if_fail (BOBGUI_IS_CSS_ANIMATED_STYLE (style), FALSE);
  bobgui_internal_return_val_if_fail (value != NULL, FALSE);

  old_value = bobgui_css_style_get_custom_property (style, id);
  if (bobgui_css_variable_value_equal (old_value, value))
    return FALSE;

  if (style->variables == NULL)
    {
      style->variables = bobgui_css_variable_set_new ();
      if (animated->parent_style)
        bobgui_css_variable_set_set_parent (style->variables,
                                         animated->parent_style->variables);
    }
  else if (style->variables == animated->style->variables)
    {
      bobgui_css_variable_set_unref (style->variables);
      style->variables = bobgui_css_variable_set_copy (animated->style->variables);
    }

  bobgui_css_variable_set_add (style->variables, id, value);

  return TRUE;
}

void
bobgui_css_animated_style_recompute (BobguiCssAnimatedStyle *style)
{
  BobguiCssComputeContext context = { NULL, };
  BobguiCssValue *shorthands[BOBGUI_CSS_SHORTHAND_PROPERTY_N_PROPERTIES] = { NULL, };

  context.provider = style->provider;
  context.style = (BobguiCssStyle *) style;
  context.parent_style = style->parent_style;
  context.variables = NULL;
  context.shorthands = shorthands;

  bobgui_css_core_values_recompute (style, &context);
  bobgui_css_background_values_recompute (style, &context);
  bobgui_css_border_values_recompute (style, &context);
  bobgui_css_icon_values_recompute (style, &context);
  bobgui_css_outline_values_recompute (style, &context);
  bobgui_css_font_values_recompute (style, &context);
  bobgui_css_text_decoration_values_recompute (style, &context);
  bobgui_css_font_variant_values_recompute (style, &context);
  bobgui_css_animation_values_recompute (style, &context);
  bobgui_css_transition_values_recompute (style, &context);
  bobgui_css_size_values_recompute (style, &context);
  bobgui_css_other_values_recompute (style, &context);

  for (unsigned int i = 0; i < BOBGUI_CSS_SHORTHAND_PROPERTY_N_PROPERTIES; i++)
    {
      if (shorthands[i])
        bobgui_css_value_unref (shorthands[i]);
    }
}

BobguiCssVariableValue *
bobgui_css_animated_style_get_intrinsic_custom_value (BobguiCssAnimatedStyle *style,
                                                   int                  id)
{
  return bobgui_css_style_get_custom_property (style->style, id);
}

static GPtrArray *
bobgui_css_animated_style_create_dynamic (GPtrArray   *animations,
                                       BobguiCssStyle *style,
                                       gint64       timestamp)
{
  guint i;

  /* XXX: Check animations if they have dynamic values */

  for (i = 0; i < BOBGUI_CSS_PROPERTY_N_PROPERTIES; i++)
    {
      if (bobgui_css_value_is_dynamic (bobgui_css_style_get_value (style, i)))
        {
          if (!animations)
            animations = g_ptr_array_new ();

          g_ptr_array_insert (animations, 0, bobgui_css_dynamic_new (timestamp));
          break;
        }
    }

  return animations;
}

/* TRANSITIONS */

typedef struct _TransitionInfo TransitionInfo;
struct _TransitionInfo {
  guint index;                  /* index into value arrays */
  gboolean pending;             /* TRUE if we still need to handle it */
};

static void
transition_info_add (TransitionInfo    infos[BOBGUI_CSS_PROPERTY_N_PROPERTIES],
                     BobguiStyleProperty *property,
                     guint             index)
{
  if (BOBGUI_IS_CSS_SHORTHAND_PROPERTY (property))
    {
      BobguiCssShorthandProperty *shorthand = (BobguiCssShorthandProperty *) property;
      guint len = _bobgui_css_shorthand_property_get_n_subproperties (shorthand);
      guint i;

      for (i = 0; i < len; i++)
        {
          BobguiCssStyleProperty *prop = _bobgui_css_shorthand_property_get_subproperty (shorthand, i);
          guint id;

          if (!_bobgui_css_style_property_is_animated ((BobguiCssStyleProperty *) prop))
            continue;

          id = _bobgui_css_style_property_get_id ((BobguiCssStyleProperty *) prop);
          infos[id].index = index;
          infos[id].pending = TRUE;
        }
    }
  else
    {
      guint id;

      if (!_bobgui_css_style_property_is_animated ((BobguiCssStyleProperty *) property))
        return;

      id = _bobgui_css_style_property_get_id ((BobguiCssStyleProperty *) property);
      g_assert (id < BOBGUI_CSS_PROPERTY_N_PROPERTIES);
      infos[id].index = index;
      infos[id].pending = TRUE;
    }
}

static void
transition_infos_set (TransitionInfo  infos[BOBGUI_CSS_PROPERTY_N_PROPERTIES],
                      BobguiCssValue    *transitions)
{
  guint i;

  for (i = 0; i < _bobgui_css_array_value_get_n_values (transitions); i++)
    {
      BobguiStyleProperty *property;
      BobguiCssValue *prop_value;

      prop_value = _bobgui_css_array_value_get_nth (transitions, i);
      if (g_ascii_strcasecmp (_bobgui_css_ident_value_get (prop_value), "all") == 0)
        {
          const guint len = _bobgui_css_style_property_get_n_properties ();
          guint j;

          for (j = 0; j < len; j++)
            {
              property = (BobguiStyleProperty *)_bobgui_css_style_property_lookup_by_id (j);
              transition_info_add (infos, property, i);
            }
        }
      else
        {
          property = _bobgui_style_property_lookup (_bobgui_css_ident_value_get (prop_value));
          if (property)
            transition_info_add (infos, property, i);
        }
    }
}

static BobguiStyleAnimation *
bobgui_css_animated_style_find_transition (BobguiCssAnimatedStyle *style,
                                        guint                property_id)
{
  guint i;

  for (i = 0; i < style->n_animations; i ++)
    {
      BobguiStyleAnimation *animation = style->animations[i];

      if (!_bobgui_css_transition_is_transition (animation))
        continue;

      if (_bobgui_css_transition_get_property ((BobguiCssTransition *)animation) == property_id)
        return animation;
    }

  return NULL;
}

static GPtrArray *
bobgui_css_animated_style_create_css_transitions (GPtrArray   *animations,
                                               BobguiCssStyle *base_style,
                                               gint64       timestamp,
                                               BobguiCssStyle *source)
{
  TransitionInfo transitions[BOBGUI_CSS_PROPERTY_N_PROPERTIES] = { { 0, } };
  BobguiCssValue *durations, *delays, *timing_functions;
  gboolean source_is_animated;
  guint i;

  durations = base_style->transition->transition_duration;
  delays = base_style->transition->transition_delay;
  timing_functions = base_style->transition->transition_timing_function;

  if (_bobgui_css_array_value_get_n_values (durations) == 1 &&
      _bobgui_css_array_value_get_n_values (delays) == 1 &&
      bobgui_css_number_value_get (_bobgui_css_array_value_get_nth (durations, 0), 100) +
      bobgui_css_number_value_get (_bobgui_css_array_value_get_nth (delays, 0), 100) == 0)
    return animations;

  transition_infos_set (transitions, base_style->transition->transition_property);

  source_is_animated = BOBGUI_IS_CSS_ANIMATED_STYLE (source);
  for (i = 0; i < BOBGUI_CSS_PROPERTY_N_PROPERTIES; i++)
    {
      BobguiStyleAnimation *animation;
      BobguiCssValue *start, *end;
      double duration, delay;

      if (!transitions[i].pending)
        continue;

      duration = bobgui_css_number_value_get (_bobgui_css_array_value_get_nth (durations, transitions[i].index), 100);
      delay = bobgui_css_number_value_get (_bobgui_css_array_value_get_nth (delays, transitions[i].index), 100);
      if (duration + delay == 0.0)
        continue;

      if (source_is_animated)
        {
          start = bobgui_css_animated_style_get_intrinsic_value ((BobguiCssAnimatedStyle *)source, i);
          end = bobgui_css_style_get_value (base_style, i);

          if (bobgui_css_value_equal (start, end))
            {
              animation = bobgui_css_animated_style_find_transition ((BobguiCssAnimatedStyle *)source, i);
              if (animation)
                {
                  animation = _bobgui_style_animation_advance (animation, timestamp);
                  if (!animations)
                    animations = g_ptr_array_new ();

                  g_ptr_array_add (animations, animation);
                }

              continue;
            }
        }

      if (bobgui_css_value_equal (bobgui_css_style_get_value (source, i),
                               bobgui_css_style_get_value (base_style, i)))
        continue;

      animation = _bobgui_css_transition_new (i,
                                           bobgui_css_style_get_value (source, i),
                                           _bobgui_css_array_value_get_nth (timing_functions, i),
                                           timestamp,
                                           duration * G_USEC_PER_SEC,
                                           delay * G_USEC_PER_SEC);
      if (!animations)
        animations = g_ptr_array_new ();

      g_ptr_array_add (animations, animation);
    }

  return animations;
}

static BobguiStyleAnimation *
bobgui_css_animated_style_find_animation (BobguiStyleAnimation **animations,
                                       guint               n_animations,
                                       const char         *name)
{
  guint i;

  for (i = 0; i < n_animations; i ++)
    {
      BobguiStyleAnimation *animation = animations[i];

      if (!_bobgui_css_animation_is_animation (animation))
        continue;

      if (g_str_equal (_bobgui_css_animation_get_name ((BobguiCssAnimation *)animation), name))
        return animation;
    }

  return NULL;
}

static GPtrArray *
bobgui_css_animated_style_create_css_animations (GPtrArray        *animations,
                                              BobguiCssStyle      *base_style,
                                              gint64            timestamp,
                                              BobguiStyleProvider *provider,
                                              BobguiCssStyle      *source)
{
  BobguiCssValue *durations, *delays, *timing_functions, *animation_names;
  BobguiCssValue *iteration_counts, *directions, *play_states, *fill_modes;
  gboolean source_is_animated;
  guint i;

  animation_names = base_style->animation->animation_name;

  if (_bobgui_css_array_value_get_n_values (animation_names) == 1)
    {
      const char *name = _bobgui_css_ident_value_get (_bobgui_css_array_value_get_nth (animation_names, 0));

      if (g_ascii_strcasecmp (name, "none") == 0)
        return animations;
    }

  durations = base_style->animation->animation_duration;
  delays = base_style->animation->animation_delay;
  timing_functions = base_style->animation->animation_timing_function;
  iteration_counts = base_style->animation->animation_iteration_count;
  directions = base_style->animation->animation_direction;
  play_states = base_style->animation->animation_play_state;
  fill_modes = base_style->animation->animation_fill_mode;
  source_is_animated = BOBGUI_IS_CSS_ANIMATED_STYLE (source);

  for (i = 0; i < _bobgui_css_array_value_get_n_values (animation_names); i++)
    {
      BobguiStyleAnimation *animation = NULL;
      BobguiCssKeyframes *keyframes;
      const char *name;

      name = _bobgui_css_ident_value_get (_bobgui_css_array_value_get_nth (animation_names, i));
      if (g_ascii_strcasecmp (name, "none") == 0)
        continue;

      if (animations)
        animation = bobgui_css_animated_style_find_animation ((BobguiStyleAnimation **)animations->pdata, animations->len, name);

      if (animation)
        continue;

      if (source_is_animated)
        animation = bobgui_css_animated_style_find_animation ((BobguiStyleAnimation **)BOBGUI_CSS_ANIMATED_STYLE (source)->animations,
                                                           BOBGUI_CSS_ANIMATED_STYLE (source)->n_animations,
                                                           name);

      if (animation)
        {
          animation = _bobgui_css_animation_advance_with_play_state ((BobguiCssAnimation *)animation,
                                                                  timestamp,
                                                                  _bobgui_css_play_state_value_get (_bobgui_css_array_value_get_nth (play_states, i)));
        }
      else
        {
          keyframes = bobgui_style_provider_get_keyframes (provider, name);
          if (keyframes == NULL)
            continue;

          animation = _bobgui_css_animation_new (name,
                                              keyframes,
                                              timestamp,
                                              bobgui_css_number_value_get (_bobgui_css_array_value_get_nth (delays, i), 100) * G_USEC_PER_SEC,
                                              bobgui_css_number_value_get (_bobgui_css_array_value_get_nth (durations, i), 100) * G_USEC_PER_SEC,
                                              _bobgui_css_array_value_get_nth (timing_functions, i),
                                              _bobgui_css_direction_value_get (_bobgui_css_array_value_get_nth (directions, i)),
                                              _bobgui_css_play_state_value_get (_bobgui_css_array_value_get_nth (play_states, i)),
                                              _bobgui_css_fill_mode_value_get (_bobgui_css_array_value_get_nth (fill_modes, i)),
                                              bobgui_css_number_value_get (_bobgui_css_array_value_get_nth (iteration_counts, i), 100));
        }

      if (!animations)
        animations = g_ptr_array_sized_new (16);

      g_ptr_array_add (animations, animation);
    }

  return animations;
}

/* PUBLIC API */

static void
bobgui_css_animated_style_apply_animations (BobguiCssAnimatedStyle *style)
{
  BobguiCssComputeContext context;

  for (guint i = 0; i < style->n_animations; i ++)
    {
      BobguiStyleAnimation *animation = style->animations[i];

      _bobgui_style_animation_apply_values (animation, style);
    }

  context.provider = style->provider;
  context.style = (BobguiCssStyle *) style;
  context.parent_style = style->parent_style;
  context.variables = NULL;
  context.shorthands = NULL;

  bobgui_css_style_resolve_used_values ((BobguiCssStyle *) style, &context);
}

BobguiCssStyle *
bobgui_css_animated_style_new (BobguiCssStyle      *base_style,
                            BobguiCssStyle      *parent_style,
                            gint64            timestamp,
                            BobguiStyleProvider *provider,
                            BobguiCssStyle      *previous_style)
{
  BobguiCssAnimatedStyle *result;
  BobguiCssStyle *style;
  GPtrArray *animations = NULL;

  bobgui_internal_return_val_if_fail (BOBGUI_IS_CSS_STYLE (base_style), NULL);
  bobgui_internal_return_val_if_fail (parent_style == NULL || BOBGUI_IS_CSS_STYLE (parent_style), NULL);
  bobgui_internal_return_val_if_fail (BOBGUI_IS_STYLE_PROVIDER (provider), NULL);
  bobgui_internal_return_val_if_fail (previous_style == NULL || BOBGUI_IS_CSS_STYLE (previous_style), NULL);

  if (timestamp == 0)
    return g_object_ref (base_style);

  if (previous_style != NULL)
    animations = bobgui_css_animated_style_create_css_transitions (animations, base_style, timestamp, previous_style);

  animations = bobgui_css_animated_style_create_css_animations (animations, base_style, timestamp, provider, previous_style);
  animations = bobgui_css_animated_style_create_dynamic (animations, base_style, timestamp);

  if (animations == NULL)
    return g_object_ref (base_style);

  result = g_object_new (BOBGUI_TYPE_CSS_ANIMATED_STYLE, NULL);

  result->style = g_object_ref (base_style);
  if (parent_style)
    result->parent_style = g_object_ref (parent_style);
  result->provider = g_object_ref (provider);
  result->current_time = timestamp;
  result->n_animations = animations->len;
  result->animations = g_ptr_array_free (animations, FALSE);

  style = (BobguiCssStyle *)result;
  style->core = (BobguiCssCoreValues *)bobgui_css_values_ref ((BobguiCssValues *)base_style->core);
  style->background = (BobguiCssBackgroundValues *)bobgui_css_values_ref ((BobguiCssValues *)base_style->background);
  style->border = (BobguiCssBorderValues *)bobgui_css_values_ref ((BobguiCssValues *)base_style->border);
  style->icon = (BobguiCssIconValues *)bobgui_css_values_ref ((BobguiCssValues *)base_style->icon);
  style->outline = (BobguiCssOutlineValues *)bobgui_css_values_ref ((BobguiCssValues *)base_style->outline);
  style->font = (BobguiCssFontValues *)bobgui_css_values_ref ((BobguiCssValues *)base_style->font);
  style->text_decoration = (BobguiCssTextDecorationValues *)bobgui_css_values_ref ((BobguiCssValues *)base_style->text_decoration);
  style->font_variant = (BobguiCssFontVariantValues *)bobgui_css_values_ref ((BobguiCssValues *)base_style->font_variant);
  style->animation = (BobguiCssAnimationValues *)bobgui_css_values_ref ((BobguiCssValues *)base_style->animation);
  style->transition = (BobguiCssTransitionValues *)bobgui_css_values_ref ((BobguiCssValues *)base_style->transition);
  style->size = (BobguiCssSizeValues *)bobgui_css_values_ref ((BobguiCssValues *)base_style->size);
  style->other = (BobguiCssOtherValues *)bobgui_css_values_ref ((BobguiCssValues *)base_style->other);
  style->used = (BobguiCssUsedValues *)bobgui_css_values_ref ((BobguiCssValues *)base_style->used);
  if (base_style->variables)
    style->variables = bobgui_css_variable_set_ref (base_style->variables);

  bobgui_css_animated_style_apply_animations (result);

  return BOBGUI_CSS_STYLE (result);
}

BobguiCssStyle *
bobgui_css_animated_style_new_advance (BobguiCssAnimatedStyle *source,
                                    BobguiCssStyle         *base_style,
                                    BobguiCssStyle         *parent_style,
                                    gint64               timestamp,
                                    BobguiStyleProvider    *provider)
{
  BobguiCssAnimatedStyle *result;
  BobguiCssStyle *style;
  GPtrArray *animations;
  guint i;

  bobgui_internal_return_val_if_fail (BOBGUI_IS_CSS_ANIMATED_STYLE (source), NULL);
  bobgui_internal_return_val_if_fail (BOBGUI_IS_CSS_STYLE (base_style), NULL);
  bobgui_internal_return_val_if_fail (parent_style == NULL || BOBGUI_IS_CSS_STYLE (parent_style), NULL);
  bobgui_internal_return_val_if_fail (BOBGUI_IS_STYLE_PROVIDER (provider), NULL);

  if (timestamp == 0)
    return g_object_ref (source->style);

  if (timestamp == source->current_time)
    return g_object_ref (BOBGUI_CSS_STYLE (source));

  bobgui_internal_return_val_if_fail (timestamp > source->current_time, NULL);

  animations = NULL;
  for (i = 0; i < source->n_animations; i ++)
    {
      BobguiStyleAnimation *animation = source->animations[i];

      if (_bobgui_style_animation_is_finished (animation))
        continue;

      if (!animations)
        animations = g_ptr_array_sized_new (16);

      animation = _bobgui_style_animation_advance (animation, timestamp);
      g_ptr_array_add (animations, animation);
    }

  if (animations == NULL)
    return g_object_ref (source->style);

  result = g_object_new (BOBGUI_TYPE_CSS_ANIMATED_STYLE, NULL);

  result->style = g_object_ref (base_style);
  if (parent_style)
    result->parent_style = g_object_ref (parent_style);
  result->provider = g_object_ref (provider);
  result->current_time = timestamp;
  result->n_animations = animations->len;
  result->animations = g_ptr_array_free (animations, FALSE);

  style = (BobguiCssStyle *)result;
  style->core = (BobguiCssCoreValues *)bobgui_css_values_ref ((BobguiCssValues *)base_style->core);
  style->background = (BobguiCssBackgroundValues *)bobgui_css_values_ref ((BobguiCssValues *)base_style->background);
  style->border = (BobguiCssBorderValues *)bobgui_css_values_ref ((BobguiCssValues *)base_style->border);
  style->icon = (BobguiCssIconValues *)bobgui_css_values_ref ((BobguiCssValues *)base_style->icon);
  style->outline = (BobguiCssOutlineValues *)bobgui_css_values_ref ((BobguiCssValues *)base_style->outline);
  style->font = (BobguiCssFontValues *)bobgui_css_values_ref ((BobguiCssValues *)base_style->font);
  style->text_decoration = (BobguiCssTextDecorationValues *)bobgui_css_values_ref ((BobguiCssValues *)base_style->text_decoration);
  style->font_variant = (BobguiCssFontVariantValues *)bobgui_css_values_ref ((BobguiCssValues *)base_style->font_variant);
  style->animation = (BobguiCssAnimationValues *)bobgui_css_values_ref ((BobguiCssValues *)base_style->animation);
  style->transition = (BobguiCssTransitionValues *)bobgui_css_values_ref ((BobguiCssValues *)base_style->transition);
  style->size = (BobguiCssSizeValues *)bobgui_css_values_ref ((BobguiCssValues *)base_style->size);
  style->other = (BobguiCssOtherValues *)bobgui_css_values_ref ((BobguiCssValues *)base_style->other);
  style->used = (BobguiCssUsedValues *)bobgui_css_values_ref ((BobguiCssValues *)base_style->used);
  if (base_style->variables)
    style->variables = bobgui_css_variable_set_ref (base_style->variables);

  bobgui_css_animated_style_apply_animations (result);

  return BOBGUI_CSS_STYLE (result);
}

BobguiCssStyle *
bobgui_css_animated_style_get_base_style (BobguiCssAnimatedStyle *style)
{
  bobgui_internal_return_val_if_fail (BOBGUI_IS_CSS_ANIMATED_STYLE (style), NULL);

  return style->style;
}

BobguiCssStyle *
bobgui_css_animated_style_get_parent_style (BobguiCssAnimatedStyle *style)
{
  bobgui_internal_return_val_if_fail (BOBGUI_IS_CSS_ANIMATED_STYLE (style), NULL);

  return style->parent_style;
}

BobguiStyleProvider *
bobgui_css_animated_style_get_provider (BobguiCssAnimatedStyle *style)
{
  bobgui_internal_return_val_if_fail (BOBGUI_IS_CSS_ANIMATED_STYLE (style), NULL);

  return style->provider;
}
