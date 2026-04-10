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

#include "bobguicssstaticstyleprivate.h"

#include "bobguicssanimationprivate.h"
#include "bobguicssarrayvalueprivate.h"
#include "bobguicssenumvalueprivate.h"
#include "bobguicssinheritvalueprivate.h"
#include "bobguicssinitialvalueprivate.h"
#include "bobguicssnumbervalueprivate.h"
#include "bobguicssshorthandpropertyprivate.h"
#include "bobguicssstringvalueprivate.h"
#include "bobguicssstylepropertyprivate.h"
#include "bobguicsstransitionprivate.h"
#include "bobguicssvaluesprivate.h"
#include "bobguiprivate.h"
#include "bobguisettings.h"
#include "bobguistyleanimationprivate.h"
#include "bobguistylepropertyprivate.h"
#include "bobguistyleproviderprivate.h"
#include "bobguicssdimensionvalueprivate.h"


static void bobgui_css_static_style_compute_value (BobguiCssStaticStyle    *style,
                                                guint                 id,
                                                BobguiCssValue          *specified,
                                                BobguiCssSection        *section,
                                                BobguiCssComputeContext *context);

#define GET_VALUES(v) (BobguiCssValue **)((guint8*)(v) + sizeof (BobguiCssValues))

#define DEFINE_VALUES(ENUM, TYPE, NAME) \
void \
bobgui_css_## NAME ## _values_compute_changes_and_affects (BobguiCssStyle *style1, \
                                                        BobguiCssStyle *style2, \
                                                        BobguiBitmask    **changes, \
                                                        BobguiCssAffects *affects) \
{ \
  BobguiCssValue **g1 = GET_VALUES (style1->NAME); \
  BobguiCssValue **g2 = GET_VALUES (style2->NAME); \
  BobguiCssValue **u1 = GET_VALUES (style1->used); \
  BobguiCssValue **u2 = GET_VALUES (style2->used); \
  int i; \
  for (i = 0; i < G_N_ELEMENTS (NAME ## _props); i++) \
    { \
      guint id = NAME ## _props[i]; \
      int j = used_props_map[id]; \
      BobguiCssValue *v1 = j < 0 ? g1[i] : u1[j]; \
      BobguiCssValue *v2 = j < 0 ? g2[i] : u2[j]; \
      if (!bobgui_css_value_equal (v1, v2)) \
        { \
          *changes = _bobgui_bitmask_set (*changes, id, TRUE); \
          *affects |= _bobgui_css_style_property_get_affects (_bobgui_css_style_property_lookup_by_id (id)); \
        } \
    } \
} \
\
static inline void \
bobgui_css_ ## NAME ## _values_new_compute (BobguiCssStaticStyle    *sstyle, \
                                         BobguiCssLookup         *lookup, \
                                         BobguiCssComputeContext *context) \
{ \
  BobguiCssStyle *style = (BobguiCssStyle *)sstyle; \
  int i; \
\
  style->NAME = (BobguiCss ## TYPE ## Values *)bobgui_css_values_new (BOBGUI_CSS_ ## ENUM ## _VALUES); \
\
  for (i = 0; i < G_N_ELEMENTS (NAME ## _props); i++) \
    { \
      guint id = NAME ## _props[i]; \
      bobgui_css_static_style_compute_value (sstyle, \
                                          id, \
                                          lookup->values[id].value, \
                                          lookup->values[id].section, \
                                          context); \
    } \
} \
static BobguiBitmask * bobgui_css_ ## NAME ## _values_mask; \
static BobguiCssValues * bobgui_css_ ## NAME ## _initial_values; \
\
static BobguiCssValues * bobgui_css_ ## NAME ## _create_initial_values (void); \
\
static void \
bobgui_css_ ## NAME ## _values_init (void) \
{ \
  int i; \
  bobgui_css_ ## NAME ## _values_mask = _bobgui_bitmask_new (); \
  for (i = 0; i < G_N_ELEMENTS(NAME ## _props); i++) \
    { \
      guint id = NAME ## _props[i]; \
      bobgui_css_ ## NAME ## _values_mask = _bobgui_bitmask_set (bobgui_css_ ## NAME ## _values_mask, id, TRUE); \
    } \
\
  bobgui_css_ ## NAME ## _initial_values = bobgui_css_ ## NAME ## _create_initial_values (); \
} \
\
static inline gboolean \
bobgui_css_ ## NAME ## _values_unset (const BobguiCssLookup *lookup) \
{ \
  const BobguiBitmask *set_values = _bobgui_css_lookup_get_set_values (lookup); \
  return !_bobgui_bitmask_intersects (set_values, bobgui_css_ ## NAME ## _values_mask); \
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

#define VERIFY_MASK(NAME) \
  { \
    BobguiBitmask *copy; \
    copy = _bobgui_bitmask_intersect (_bobgui_bitmask_copy (bobgui_css_ ## NAME ## _values_mask), all); \
    g_assert (_bobgui_bitmask_equals (copy, bobgui_css_ ## NAME ## _values_mask)); \
    _bobgui_bitmask_free (copy); \
  } \
 all = _bobgui_bitmask_subtract (all, bobgui_css_ ## NAME ## _values_mask);
  
/* Verify that every style property is present in one group, and none
 * is present in more than one group.
 */
static void
verify_style_groups (void)
{
  BobguiBitmask *all;
  guint id;

  all = _bobgui_bitmask_new ();

  for (id = 0; id < BOBGUI_CSS_PROPERTY_N_PROPERTIES; id++)
    all = _bobgui_bitmask_set (all, id, TRUE);

  VERIFY_MASK (core);
  VERIFY_MASK (background);
  VERIFY_MASK (border);
  VERIFY_MASK (icon);
  VERIFY_MASK (outline);
  VERIFY_MASK (font);
  VERIFY_MASK (text_decoration);
  VERIFY_MASK (font_variant);
  VERIFY_MASK (animation);
  VERIFY_MASK (transition);
  VERIFY_MASK (size);
  VERIFY_MASK (other);

  g_assert (_bobgui_bitmask_is_empty (all));

  _bobgui_bitmask_free (all);
}

#undef VERIFY_MASK

static void
verify_used_map (void)
{
  for (guint id = 0; id < BOBGUI_CSS_PROPERTY_N_PROPERTIES; id++)
    {
      if (used_props_map[id] != -1)
        g_assert (used_props[used_props_map[id]] == id);
    }

  for (guint i = 0; i < G_N_ELEMENTS (used_props); i++)
    {
      g_assert (used_props_map[used_props[i]] == i);
    }
}

G_DEFINE_TYPE (BobguiCssStaticStyle, bobgui_css_static_style, BOBGUI_TYPE_CSS_STYLE)

static BobguiCssSection *
bobgui_css_static_style_get_section (BobguiCssStyle *style,
                                  guint        id)
{
  BobguiCssStaticStyle *sstyle = BOBGUI_CSS_STATIC_STYLE (style);

  if (sstyle->sections == NULL ||
      id >= sstyle->sections->len)
    return NULL;

  return g_ptr_array_index (sstyle->sections, id);
}

static void
bobgui_css_static_style_dispose (GObject *object)
{
  BobguiCssStaticStyle *style = BOBGUI_CSS_STATIC_STYLE (object);

  if (style->sections)
    {
      g_ptr_array_unref (style->sections);
      style->sections = NULL;
    }

  if (style->original_values)
    {
      g_ptr_array_unref (style->original_values);
      style->original_values = NULL;
    }

  G_OBJECT_CLASS (bobgui_css_static_style_parent_class)->dispose (object);
}

static BobguiCssStaticStyle *
bobgui_css_static_style_get_static_style (BobguiCssStyle *style)
{
  return (BobguiCssStaticStyle *)style;
}

static BobguiCssValue *
bobgui_css_static_style_get_original_value (BobguiCssStyle *style,
                                         guint        id)
{
  BobguiCssStaticStyle *sstyle = BOBGUI_CSS_STATIC_STYLE (style);

  if (sstyle->original_values == NULL ||
      id >= sstyle->original_values->len)
    return NULL;

  return g_ptr_array_index (sstyle->original_values, id);
}

static void
bobgui_css_static_style_class_init (BobguiCssStaticStyleClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  BobguiCssStyleClass *style_class = BOBGUI_CSS_STYLE_CLASS (klass);

  object_class->dispose = bobgui_css_static_style_dispose;

  style_class->get_section = bobgui_css_static_style_get_section;
  style_class->get_static_style = bobgui_css_static_style_get_static_style;
  style_class->get_original_value = bobgui_css_static_style_get_original_value;

  bobgui_css_core_values_init ();
  bobgui_css_background_values_init ();
  bobgui_css_border_values_init ();
  bobgui_css_icon_values_init ();
  bobgui_css_outline_values_init ();
  bobgui_css_font_values_init ();
  bobgui_css_text_decoration_values_init ();
  bobgui_css_font_variant_values_init ();
  bobgui_css_animation_values_init ();
  bobgui_css_transition_values_init ();
  bobgui_css_size_values_init ();
  bobgui_css_other_values_init ();

  verify_style_groups ();
  verify_used_map ();
}

static void
bobgui_css_static_style_init (BobguiCssStaticStyle *style)
{
}

static void
maybe_unref_section (gpointer section)
{
  if (section)
    bobgui_css_section_unref (section);
}

static void
maybe_unref_value (gpointer value)
{
  if (value)
    bobgui_css_value_unref (value);
}

static inline void
bobgui_css_take_value (BobguiCssValue **variable,
                    BobguiCssValue  *value)
{
  if (*variable)
    bobgui_css_value_unref (*variable);
  *variable = value;
}

static void
bobgui_css_static_style_set_value (BobguiCssStaticStyle *sstyle,
                                guint              id,
                                BobguiCssValue       *value,
                                BobguiCssValue       *original_value,
                                BobguiCssSection     *section)
{
  BobguiCssStyle *style = (BobguiCssStyle *)sstyle;

  switch (id)
    {
    case BOBGUI_CSS_PROPERTY_COLOR:
      bobgui_css_take_value (&style->core->color, value);
      break;
    case BOBGUI_CSS_PROPERTY_DPI:
      bobgui_css_take_value (&style->core->dpi, value);
      break;
    case BOBGUI_CSS_PROPERTY_FONT_SIZE:
      bobgui_css_take_value (&style->core->font_size, value);
      break;
    case BOBGUI_CSS_PROPERTY_ICON_PALETTE:
      bobgui_css_take_value (&style->core->icon_palette, value);
      break;
    case BOBGUI_CSS_PROPERTY_BACKGROUND_COLOR:
      bobgui_css_take_value (&style->background->background_color, value);
      break;
    case BOBGUI_CSS_PROPERTY_FONT_FAMILY:
      bobgui_css_take_value (&style->font->font_family, value);
      break;
    case BOBGUI_CSS_PROPERTY_FONT_STYLE:
      bobgui_css_take_value (&style->font->font_style, value);
      break;
    case BOBGUI_CSS_PROPERTY_FONT_WEIGHT:
      bobgui_css_take_value (&style->font->font_weight, value);
      break;
    case BOBGUI_CSS_PROPERTY_FONT_STRETCH:
      bobgui_css_take_value (&style->font->font_stretch, value);
      break;
    case BOBGUI_CSS_PROPERTY_LETTER_SPACING:
      bobgui_css_take_value (&style->font->letter_spacing, value);
      break;
    case BOBGUI_CSS_PROPERTY_LINE_HEIGHT:
      bobgui_css_take_value (&style->font->line_height, value);
      break;
    case BOBGUI_CSS_PROPERTY_TEXT_DECORATION_LINE:
      bobgui_css_take_value (&style->text_decoration->text_decoration_line, value);
      break;
    case BOBGUI_CSS_PROPERTY_TEXT_DECORATION_COLOR:
      bobgui_css_take_value (&style->text_decoration->text_decoration_color, value);
      break;
    case BOBGUI_CSS_PROPERTY_TEXT_DECORATION_STYLE:
      bobgui_css_take_value (&style->text_decoration->text_decoration_style, value);
      break;
    case BOBGUI_CSS_PROPERTY_TEXT_TRANSFORM:
      bobgui_css_take_value (&style->font_variant->text_transform, value);
      break;
    case BOBGUI_CSS_PROPERTY_FONT_KERNING:
      bobgui_css_take_value (&style->font_variant->font_kerning, value);
      break;
    case BOBGUI_CSS_PROPERTY_FONT_VARIANT_LIGATURES:
      bobgui_css_take_value (&style->font_variant->font_variant_ligatures, value);
      break;
    case BOBGUI_CSS_PROPERTY_FONT_VARIANT_POSITION:
      bobgui_css_take_value (&style->font_variant->font_variant_position, value);
      break;
    case BOBGUI_CSS_PROPERTY_FONT_VARIANT_CAPS:
      bobgui_css_take_value (&style->font_variant->font_variant_caps, value);
      break;
    case BOBGUI_CSS_PROPERTY_FONT_VARIANT_NUMERIC:
      bobgui_css_take_value (&style->font_variant->font_variant_numeric, value);
      break;
    case BOBGUI_CSS_PROPERTY_FONT_VARIANT_ALTERNATES:
      bobgui_css_take_value (&style->font_variant->font_variant_alternates, value);
      break;
    case BOBGUI_CSS_PROPERTY_FONT_VARIANT_EAST_ASIAN:
      bobgui_css_take_value (&style->font_variant->font_variant_east_asian, value);
      break;
    case BOBGUI_CSS_PROPERTY_TEXT_SHADOW:
      bobgui_css_take_value (&style->font->text_shadow, value);
      break;
    case BOBGUI_CSS_PROPERTY_BOX_SHADOW:
      bobgui_css_take_value (&style->background->box_shadow, value);
      break;
    case BOBGUI_CSS_PROPERTY_MARGIN_TOP:
      bobgui_css_take_value (&style->size->margin_top, value);
      break;
    case BOBGUI_CSS_PROPERTY_MARGIN_LEFT:
      bobgui_css_take_value (&style->size->margin_left, value);
      break;
    case BOBGUI_CSS_PROPERTY_MARGIN_BOTTOM:
      bobgui_css_take_value (&style->size->margin_bottom, value);
      break;
    case BOBGUI_CSS_PROPERTY_MARGIN_RIGHT:
      bobgui_css_take_value (&style->size->margin_right, value);
      break;
    case BOBGUI_CSS_PROPERTY_PADDING_TOP:
      bobgui_css_take_value (&style->size->padding_top, value);
      break;
    case BOBGUI_CSS_PROPERTY_PADDING_LEFT:
      bobgui_css_take_value (&style->size->padding_left, value);
      break;
    case BOBGUI_CSS_PROPERTY_PADDING_BOTTOM:
      bobgui_css_take_value (&style->size->padding_bottom, value);
      break;
    case BOBGUI_CSS_PROPERTY_PADDING_RIGHT:
      bobgui_css_take_value (&style->size->padding_right, value);
      break;
    case BOBGUI_CSS_PROPERTY_BORDER_TOP_STYLE:
      bobgui_css_take_value (&style->border->border_top_style, value);
      break;
    case BOBGUI_CSS_PROPERTY_BORDER_TOP_WIDTH:
      bobgui_css_take_value (&style->border->border_top_width, value);
      break;
    case BOBGUI_CSS_PROPERTY_BORDER_LEFT_STYLE:
      bobgui_css_take_value (&style->border->border_left_style, value);
      break;
    case BOBGUI_CSS_PROPERTY_BORDER_LEFT_WIDTH:
      bobgui_css_take_value (&style->border->border_left_width, value);
      break;
    case BOBGUI_CSS_PROPERTY_BORDER_BOTTOM_STYLE:
      bobgui_css_take_value (&style->border->border_bottom_style, value);
      break;
    case BOBGUI_CSS_PROPERTY_BORDER_BOTTOM_WIDTH:
      bobgui_css_take_value (&style->border->border_bottom_width, value);
      break;
    case BOBGUI_CSS_PROPERTY_BORDER_RIGHT_STYLE:
      bobgui_css_take_value (&style->border->border_right_style, value);
      break;
    case BOBGUI_CSS_PROPERTY_BORDER_RIGHT_WIDTH:
      bobgui_css_take_value (&style->border->border_right_width, value);
      break;
    case BOBGUI_CSS_PROPERTY_BORDER_TOP_LEFT_RADIUS:
      bobgui_css_take_value (&style->border->border_top_left_radius, value);
      break;
    case BOBGUI_CSS_PROPERTY_BORDER_TOP_RIGHT_RADIUS:
      bobgui_css_take_value (&style->border->border_top_right_radius, value);
      break;
    case BOBGUI_CSS_PROPERTY_BORDER_BOTTOM_RIGHT_RADIUS:
      bobgui_css_take_value (&style->border->border_bottom_right_radius, value);
      break;
    case BOBGUI_CSS_PROPERTY_BORDER_BOTTOM_LEFT_RADIUS:
      bobgui_css_take_value (&style->border->border_bottom_left_radius, value);
      break;
    case BOBGUI_CSS_PROPERTY_OUTLINE_STYLE:
      bobgui_css_take_value (&style->outline->outline_style, value);
      break;
    case BOBGUI_CSS_PROPERTY_OUTLINE_WIDTH:
      bobgui_css_take_value (&style->outline->outline_width, value);
      break;
    case BOBGUI_CSS_PROPERTY_OUTLINE_OFFSET:
      bobgui_css_take_value (&style->outline->outline_offset, value);
      break;
    case BOBGUI_CSS_PROPERTY_BACKGROUND_CLIP:
      bobgui_css_take_value (&style->background->background_clip, value);
      break;
    case BOBGUI_CSS_PROPERTY_BACKGROUND_ORIGIN:
      bobgui_css_take_value (&style->background->background_origin, value);
      break;
    case BOBGUI_CSS_PROPERTY_BACKGROUND_SIZE:
      bobgui_css_take_value (&style->background->background_size, value);
      break;
    case BOBGUI_CSS_PROPERTY_BACKGROUND_POSITION:
      bobgui_css_take_value (&style->background->background_position, value);
      break;
    case BOBGUI_CSS_PROPERTY_BORDER_TOP_COLOR:
      bobgui_css_take_value (&style->border->border_top_color, value);
      break;
    case BOBGUI_CSS_PROPERTY_BORDER_RIGHT_COLOR:
      bobgui_css_take_value (&style->border->border_right_color, value);
      break;
    case BOBGUI_CSS_PROPERTY_BORDER_BOTTOM_COLOR:
      bobgui_css_take_value (&style->border->border_bottom_color, value);
      break;
    case BOBGUI_CSS_PROPERTY_BORDER_LEFT_COLOR:
      bobgui_css_take_value (&style->border->border_left_color, value);
      break;
    case BOBGUI_CSS_PROPERTY_OUTLINE_COLOR:
      bobgui_css_take_value (&style->outline->outline_color, value);
      break;
    case BOBGUI_CSS_PROPERTY_BACKGROUND_REPEAT:
      bobgui_css_take_value (&style->background->background_repeat, value);
      break;
    case BOBGUI_CSS_PROPERTY_BACKGROUND_IMAGE:
      bobgui_css_take_value (&style->background->background_image, value);
      break;
    case BOBGUI_CSS_PROPERTY_BACKGROUND_BLEND_MODE:
      bobgui_css_take_value (&style->background->background_blend_mode, value);
      break;
    case BOBGUI_CSS_PROPERTY_BORDER_IMAGE_SOURCE:
      bobgui_css_take_value (&style->border->border_image_source, value);
      break;
    case BOBGUI_CSS_PROPERTY_BORDER_IMAGE_REPEAT:
      bobgui_css_take_value (&style->border->border_image_repeat, value);
      break;
    case BOBGUI_CSS_PROPERTY_BORDER_IMAGE_SLICE:
      bobgui_css_take_value (&style->border->border_image_slice, value);
      break;
    case BOBGUI_CSS_PROPERTY_BORDER_IMAGE_WIDTH:
      bobgui_css_take_value (&style->border->border_image_width, value);
      break;
    case BOBGUI_CSS_PROPERTY_ICON_SOURCE:
      bobgui_css_take_value (&style->other->icon_source, value);
      break;
    case BOBGUI_CSS_PROPERTY_ICON_SIZE:
      bobgui_css_take_value (&style->icon->icon_size, value);
      break;
    case BOBGUI_CSS_PROPERTY_ICON_SHADOW:
      bobgui_css_take_value (&style->icon->icon_shadow, value);
      break;
    case BOBGUI_CSS_PROPERTY_ICON_STYLE:
      bobgui_css_take_value (&style->icon->icon_style, value);
      break;
    case BOBGUI_CSS_PROPERTY_ICON_WEIGHT:
      bobgui_css_take_value (&style->icon->icon_weight, value);
      break;
    case BOBGUI_CSS_PROPERTY_ICON_TRANSFORM:
      bobgui_css_take_value (&style->other->icon_transform, value);
      break;
    case BOBGUI_CSS_PROPERTY_ICON_FILTER:
      bobgui_css_take_value (&style->other->icon_filter, value);
      break;
    case BOBGUI_CSS_PROPERTY_BORDER_SPACING:
      bobgui_css_take_value (&style->size->border_spacing, value);
      break;
    case BOBGUI_CSS_PROPERTY_TRANSFORM:
      bobgui_css_take_value (&style->other->transform, value);
      break;
    case BOBGUI_CSS_PROPERTY_TRANSFORM_ORIGIN:
      bobgui_css_take_value (&style->other->transform_origin, value);
      break;
    case BOBGUI_CSS_PROPERTY_MIN_WIDTH:
      bobgui_css_take_value (&style->size->min_width, value);
      break;
    case BOBGUI_CSS_PROPERTY_MIN_HEIGHT:
      bobgui_css_take_value (&style->size->min_height, value);
      break;
    case BOBGUI_CSS_PROPERTY_TRANSITION_PROPERTY:
      bobgui_css_take_value (&style->transition->transition_property, value);
      break;
    case BOBGUI_CSS_PROPERTY_TRANSITION_DURATION:
      bobgui_css_take_value (&style->transition->transition_duration, value);
      break;
    case BOBGUI_CSS_PROPERTY_TRANSITION_TIMING_FUNCTION:
      bobgui_css_take_value (&style->transition->transition_timing_function, value);
      break;
    case BOBGUI_CSS_PROPERTY_TRANSITION_DELAY:
      bobgui_css_take_value (&style->transition->transition_delay, value);
      break;
    case BOBGUI_CSS_PROPERTY_ANIMATION_NAME:
      bobgui_css_take_value (&style->animation->animation_name, value);
      break;
    case BOBGUI_CSS_PROPERTY_ANIMATION_DURATION:
      bobgui_css_take_value (&style->animation->animation_duration, value);
      break;
    case BOBGUI_CSS_PROPERTY_ANIMATION_TIMING_FUNCTION:
      bobgui_css_take_value (&style->animation->animation_timing_function, value);
      break;
    case BOBGUI_CSS_PROPERTY_ANIMATION_ITERATION_COUNT:
      bobgui_css_take_value (&style->animation->animation_iteration_count, value);
      break;
    case BOBGUI_CSS_PROPERTY_ANIMATION_DIRECTION:
      bobgui_css_take_value (&style->animation->animation_direction, value);
      break;
    case BOBGUI_CSS_PROPERTY_ANIMATION_PLAY_STATE:
      bobgui_css_take_value (&style->animation->animation_play_state, value);
      break;
    case BOBGUI_CSS_PROPERTY_ANIMATION_DELAY:
      bobgui_css_take_value (&style->animation->animation_delay, value);
      break;
    case BOBGUI_CSS_PROPERTY_ANIMATION_FILL_MODE:
      bobgui_css_take_value (&style->animation->animation_fill_mode, value);
      break;
    case BOBGUI_CSS_PROPERTY_OPACITY:
      bobgui_css_take_value (&style->other->opacity, value);
      break;
    case BOBGUI_CSS_PROPERTY_BACKDROP_FILTER:
      bobgui_css_take_value (&style->other->backdrop_filter, value);
      break;
    case BOBGUI_CSS_PROPERTY_FILTER:
      bobgui_css_take_value (&style->other->filter, value);
      break;
    case BOBGUI_CSS_PROPERTY_CARET_COLOR:
      bobgui_css_take_value (&style->font->caret_color, value);
      break;
    case BOBGUI_CSS_PROPERTY_SECONDARY_CARET_COLOR:
      bobgui_css_take_value (&style->font->secondary_caret_color, value);
      break;
    case BOBGUI_CSS_PROPERTY_FONT_FEATURE_SETTINGS:
      bobgui_css_take_value (&style->font->font_feature_settings, value);
      break;
    case BOBGUI_CSS_PROPERTY_FONT_VARIATION_SETTINGS:
      bobgui_css_take_value (&style->font->font_variation_settings, value);
      break;

    default:
      g_assert_not_reached ();
      break;
    }

  if (sstyle->sections && sstyle->sections->len > id && g_ptr_array_index (sstyle->sections, id))
    {
      bobgui_css_section_unref (g_ptr_array_index (sstyle->sections, id));
      g_ptr_array_index (sstyle->sections, id) = NULL;
    }

  if (section)
    {
      if (sstyle->sections == NULL)
        sstyle->sections = g_ptr_array_new_with_free_func (maybe_unref_section);
      if (sstyle->sections->len <= id)
        g_ptr_array_set_size (sstyle->sections, id + 1);
      g_ptr_array_index (sstyle->sections, id) = bobgui_css_section_ref (section);
    }

  if (sstyle->original_values && sstyle->original_values->len > id &&
      g_ptr_array_index (sstyle->original_values, id))
    {
      bobgui_css_value_unref (g_ptr_array_index (sstyle->original_values, id));
      g_ptr_array_index (sstyle->original_values, id) = NULL;
    }

  if (original_value)
    {
      if (sstyle->original_values == NULL)
        sstyle->original_values = g_ptr_array_new_with_free_func (maybe_unref_value);
      if (sstyle->original_values->len <= id)
        g_ptr_array_set_size (sstyle->original_values, id + 1);
      g_ptr_array_index (sstyle->original_values, id) = bobgui_css_value_ref (original_value);
    }
}

static BobguiCssStyle *default_style;

static void
clear_default_style (gpointer data)
{
  g_set_object (&default_style, NULL);
}

BobguiCssStyle *
bobgui_css_static_style_get_default (void)
{
  /* FIXME: This really depends on the screen, but we don't have
   * a screen at hand when we call this function, and in practice,
   * the default style is always replaced by something else
   * before we use it.
   */
  if (default_style == NULL)
    {
      BobguiCountingBloomFilter filter = BOBGUI_COUNTING_BLOOM_FILTER_INIT;
      BobguiSettings *settings;

      settings = bobgui_settings_get_default ();
      default_style = bobgui_css_static_style_new_compute (BOBGUI_STYLE_PROVIDER (settings),
                                                        &filter,
                                                        NULL,
                                                        0);
      g_object_set_data_full (G_OBJECT (settings), I_("bobgui-default-style"),
                              default_style, clear_default_style);
    }

  return default_style;
}

static BobguiCssValues *
bobgui_css_core_create_initial_values (void)
{
  return NULL;
}

static BobguiCssValues *
bobgui_css_background_create_initial_values (void)
{
  BobguiCssBackgroundValues *values;
  BobguiCssComputeContext context = { NULL, };

  values = (BobguiCssBackgroundValues *)bobgui_css_values_new (BOBGUI_CSS_BACKGROUND_INITIAL_VALUES);

  values->background_color = _bobgui_css_initial_value_new_compute (BOBGUI_CSS_PROPERTY_BACKGROUND_COLOR, &context);
  values->box_shadow = _bobgui_css_initial_value_new_compute (BOBGUI_CSS_PROPERTY_BOX_SHADOW, &context);
  values->background_clip = _bobgui_css_initial_value_new_compute (BOBGUI_CSS_PROPERTY_BACKGROUND_CLIP, &context);
  values->background_origin = _bobgui_css_initial_value_new_compute (BOBGUI_CSS_PROPERTY_BACKGROUND_ORIGIN, &context);
  values->background_size = _bobgui_css_initial_value_new_compute (BOBGUI_CSS_PROPERTY_BACKGROUND_SIZE, &context);
  values->background_position = _bobgui_css_initial_value_new_compute (BOBGUI_CSS_PROPERTY_BACKGROUND_POSITION, &context);
  values->background_repeat = _bobgui_css_initial_value_new_compute (BOBGUI_CSS_PROPERTY_BACKGROUND_REPEAT, &context);
  values->background_image = _bobgui_css_initial_value_new_compute (BOBGUI_CSS_PROPERTY_BACKGROUND_IMAGE, &context);
  values->background_blend_mode = _bobgui_css_initial_value_new_compute (BOBGUI_CSS_PROPERTY_BACKGROUND_BLEND_MODE, &context);

  return (BobguiCssValues *)values;
}

static BobguiCssValues *
bobgui_css_border_create_initial_values (void)
{
  BobguiCssBorderValues *values;
  BobguiCssComputeContext context = { NULL, };

  values = (BobguiCssBorderValues *)bobgui_css_values_new (BOBGUI_CSS_BORDER_INITIAL_VALUES);

  values->border_top_style = _bobgui_css_initial_value_new_compute (BOBGUI_CSS_PROPERTY_BORDER_TOP_STYLE, &context);
  values->border_top_width = _bobgui_css_initial_value_new_compute (BOBGUI_CSS_PROPERTY_BORDER_TOP_WIDTH, &context);
  values->border_left_style = _bobgui_css_initial_value_new_compute (BOBGUI_CSS_PROPERTY_BORDER_LEFT_STYLE, &context);
  values->border_left_width = _bobgui_css_initial_value_new_compute (BOBGUI_CSS_PROPERTY_BORDER_LEFT_WIDTH, &context);
  values->border_bottom_style = _bobgui_css_initial_value_new_compute (BOBGUI_CSS_PROPERTY_BORDER_BOTTOM_STYLE, &context);
  values->border_bottom_width = _bobgui_css_initial_value_new_compute (BOBGUI_CSS_PROPERTY_BORDER_BOTTOM_WIDTH, &context);
  values->border_right_style = _bobgui_css_initial_value_new_compute (BOBGUI_CSS_PROPERTY_BORDER_RIGHT_STYLE, &context);
  values->border_right_width = _bobgui_css_initial_value_new_compute (BOBGUI_CSS_PROPERTY_BORDER_RIGHT_WIDTH, &context);
  values->border_top_left_radius = _bobgui_css_initial_value_new_compute (BOBGUI_CSS_PROPERTY_BORDER_TOP_LEFT_RADIUS, &context);
  values->border_top_right_radius = _bobgui_css_initial_value_new_compute (BOBGUI_CSS_PROPERTY_BORDER_TOP_RIGHT_RADIUS, &context);
  values->border_bottom_left_radius = _bobgui_css_initial_value_new_compute (BOBGUI_CSS_PROPERTY_BORDER_BOTTOM_LEFT_RADIUS, &context);
  values->border_bottom_right_radius = _bobgui_css_initial_value_new_compute (BOBGUI_CSS_PROPERTY_BORDER_BOTTOM_RIGHT_RADIUS, &context);
  values->border_top_color = _bobgui_css_initial_value_new_compute (BOBGUI_CSS_PROPERTY_BORDER_TOP_COLOR, &context);
  values->border_right_color = _bobgui_css_initial_value_new_compute (BOBGUI_CSS_PROPERTY_BORDER_RIGHT_COLOR, &context);
  values->border_bottom_color = _bobgui_css_initial_value_new_compute (BOBGUI_CSS_PROPERTY_BORDER_BOTTOM_COLOR, &context);
  values->border_left_color = _bobgui_css_initial_value_new_compute (BOBGUI_CSS_PROPERTY_BORDER_LEFT_COLOR, &context);
  values->border_image_source = _bobgui_css_initial_value_new_compute (BOBGUI_CSS_PROPERTY_BORDER_IMAGE_SOURCE, &context);
  values->border_image_repeat = _bobgui_css_initial_value_new_compute (BOBGUI_CSS_PROPERTY_BORDER_IMAGE_REPEAT, &context);
  values->border_image_slice = _bobgui_css_initial_value_new_compute (BOBGUI_CSS_PROPERTY_BORDER_IMAGE_SLICE, &context);
  values->border_image_width = _bobgui_css_initial_value_new_compute (BOBGUI_CSS_PROPERTY_BORDER_IMAGE_WIDTH, &context);

  return (BobguiCssValues *)values;
}

static BobguiCssValues *
bobgui_css_outline_create_initial_values (void)
{
  BobguiCssOutlineValues *values;
  BobguiCssComputeContext context = { NULL, };

  values = (BobguiCssOutlineValues *)bobgui_css_values_new (BOBGUI_CSS_OUTLINE_INITIAL_VALUES);

  values->outline_style = _bobgui_css_initial_value_new_compute (BOBGUI_CSS_PROPERTY_OUTLINE_STYLE, &context);
  values->outline_width = _bobgui_css_initial_value_new_compute (BOBGUI_CSS_PROPERTY_OUTLINE_WIDTH, &context);
  values->outline_offset = _bobgui_css_initial_value_new_compute (BOBGUI_CSS_PROPERTY_OUTLINE_OFFSET, &context);
  values->outline_color = _bobgui_css_initial_value_new_compute (BOBGUI_CSS_PROPERTY_OUTLINE_COLOR, &context);

  return (BobguiCssValues *)values;
}

static BobguiCssValues *
bobgui_css_icon_create_initial_values (void)
{
  return NULL;
}

static BobguiCssValues *
bobgui_css_font_create_initial_values (void)
{
  return NULL;
}

static BobguiCssValues *
bobgui_css_text_decoration_create_initial_values (void)
{
  BobguiCssTextDecorationValues *values;
  BobguiCssComputeContext context = { NULL, };

  values = (BobguiCssTextDecorationValues *)bobgui_css_values_new (BOBGUI_CSS_TEXT_DECORATION_INITIAL_VALUES);

  values->text_decoration_line = _bobgui_css_initial_value_new_compute (BOBGUI_CSS_PROPERTY_TEXT_DECORATION_LINE, &context);
  values->text_decoration_color = _bobgui_css_initial_value_new_compute (BOBGUI_CSS_PROPERTY_TEXT_DECORATION_COLOR, &context);
  values->text_decoration_style = _bobgui_css_initial_value_new_compute (BOBGUI_CSS_PROPERTY_TEXT_DECORATION_STYLE, &context);

  return (BobguiCssValues *)values;
}

static BobguiCssValues *
bobgui_css_font_variant_create_initial_values (void)
{
  return NULL;
}

static BobguiCssValues *
bobgui_css_animation_create_initial_values (void)
{
  BobguiCssAnimationValues *values;
  BobguiCssComputeContext context = { NULL, };

  values = (BobguiCssAnimationValues *)bobgui_css_values_new (BOBGUI_CSS_ANIMATION_INITIAL_VALUES);

  values->animation_name = _bobgui_css_initial_value_new_compute (BOBGUI_CSS_PROPERTY_ANIMATION_NAME, &context);
  values->animation_duration = _bobgui_css_initial_value_new_compute (BOBGUI_CSS_PROPERTY_ANIMATION_DURATION, &context);
  values->animation_timing_function = _bobgui_css_initial_value_new_compute (BOBGUI_CSS_PROPERTY_ANIMATION_TIMING_FUNCTION, &context);
  values->animation_iteration_count = _bobgui_css_initial_value_new_compute (BOBGUI_CSS_PROPERTY_ANIMATION_ITERATION_COUNT, &context);
  values->animation_direction = _bobgui_css_initial_value_new_compute (BOBGUI_CSS_PROPERTY_ANIMATION_DIRECTION, &context);
  values->animation_play_state = _bobgui_css_initial_value_new_compute (BOBGUI_CSS_PROPERTY_ANIMATION_PLAY_STATE, &context);
  values->animation_delay = _bobgui_css_initial_value_new_compute (BOBGUI_CSS_PROPERTY_ANIMATION_DELAY, &context);
  values->animation_fill_mode = _bobgui_css_initial_value_new_compute (BOBGUI_CSS_PROPERTY_ANIMATION_FILL_MODE, &context);

  return (BobguiCssValues *)values;
}

static BobguiCssValues *
bobgui_css_transition_create_initial_values (void)
{
  BobguiCssTransitionValues *values;
  BobguiCssComputeContext context = { NULL, };

  values = (BobguiCssTransitionValues *)bobgui_css_values_new (BOBGUI_CSS_TRANSITION_INITIAL_VALUES);

  values->transition_property = _bobgui_css_initial_value_new_compute (BOBGUI_CSS_PROPERTY_TRANSITION_PROPERTY, &context);
  values->transition_duration = _bobgui_css_initial_value_new_compute (BOBGUI_CSS_PROPERTY_TRANSITION_DURATION, &context);
  values->transition_timing_function = _bobgui_css_initial_value_new_compute (BOBGUI_CSS_PROPERTY_TRANSITION_TIMING_FUNCTION, &context);
  values->transition_delay = _bobgui_css_initial_value_new_compute (BOBGUI_CSS_PROPERTY_TRANSITION_DELAY, &context);

  return (BobguiCssValues *)values;
}

static BobguiCssValues *
bobgui_css_size_create_initial_values (void)
{
  BobguiCssSizeValues *values;
  BobguiCssComputeContext context = { NULL, };

  values = (BobguiCssSizeValues *)bobgui_css_values_new (BOBGUI_CSS_SIZE_INITIAL_VALUES);

  values->margin_top = _bobgui_css_initial_value_new_compute (BOBGUI_CSS_PROPERTY_MARGIN_TOP, &context);
  values->margin_left = _bobgui_css_initial_value_new_compute (BOBGUI_CSS_PROPERTY_MARGIN_LEFT, &context);
  values->margin_bottom = _bobgui_css_initial_value_new_compute (BOBGUI_CSS_PROPERTY_MARGIN_BOTTOM, &context);
  values->margin_right = _bobgui_css_initial_value_new_compute (BOBGUI_CSS_PROPERTY_MARGIN_RIGHT, &context);
  values->padding_top = _bobgui_css_initial_value_new_compute (BOBGUI_CSS_PROPERTY_PADDING_TOP, &context);
  values->padding_left = _bobgui_css_initial_value_new_compute (BOBGUI_CSS_PROPERTY_PADDING_LEFT, &context);
  values->padding_bottom = _bobgui_css_initial_value_new_compute (BOBGUI_CSS_PROPERTY_PADDING_BOTTOM, &context);
  values->padding_right = _bobgui_css_initial_value_new_compute (BOBGUI_CSS_PROPERTY_PADDING_RIGHT, &context);
  values->border_spacing = _bobgui_css_initial_value_new_compute (BOBGUI_CSS_PROPERTY_BORDER_SPACING, &context);
  values->min_width = _bobgui_css_initial_value_new_compute (BOBGUI_CSS_PROPERTY_MIN_WIDTH, &context);
  values->min_height = _bobgui_css_initial_value_new_compute (BOBGUI_CSS_PROPERTY_MIN_HEIGHT, &context);

  return (BobguiCssValues *)values;
}

static BobguiCssValues *
bobgui_css_other_create_initial_values (void)
{
  BobguiCssOtherValues *values;
  BobguiCssComputeContext context = { NULL, };

  values = (BobguiCssOtherValues *)bobgui_css_values_new (BOBGUI_CSS_OTHER_INITIAL_VALUES);

  values->icon_source = _bobgui_css_initial_value_new_compute (BOBGUI_CSS_PROPERTY_ICON_SOURCE, &context);
  values->icon_transform = _bobgui_css_initial_value_new_compute (BOBGUI_CSS_PROPERTY_ICON_TRANSFORM, &context);
  values->icon_filter = _bobgui_css_initial_value_new_compute (BOBGUI_CSS_PROPERTY_ICON_FILTER, &context);
  values->transform = _bobgui_css_initial_value_new_compute (BOBGUI_CSS_PROPERTY_TRANSFORM, &context);
  values->transform_origin = _bobgui_css_initial_value_new_compute (BOBGUI_CSS_PROPERTY_TRANSFORM_ORIGIN, &context);
  values->opacity = _bobgui_css_initial_value_new_compute (BOBGUI_CSS_PROPERTY_OPACITY, &context);
  values->backdrop_filter = _bobgui_css_initial_value_new_compute (BOBGUI_CSS_PROPERTY_BACKDROP_FILTER, &context);
  values->filter = _bobgui_css_initial_value_new_compute (BOBGUI_CSS_PROPERTY_FILTER, &context);

  return (BobguiCssValues *)values;
}

static void
bobgui_css_lookup_resolve (BobguiCssLookup      *lookup,
                        BobguiStyleProvider  *provider,
                        BobguiCssStaticStyle *sstyle,
                        BobguiCssStyle       *parent_style)
{
  BobguiCssStyle *style = (BobguiCssStyle *)sstyle;
  BobguiCssValue *shorthands[BOBGUI_CSS_SHORTHAND_PROPERTY_N_PROPERTIES] = { NULL, };
  BobguiCssComputeContext context = { NULL, };

  bobgui_internal_return_if_fail (lookup != NULL);
  bobgui_internal_return_if_fail (BOBGUI_IS_STYLE_PROVIDER (provider));
  bobgui_internal_return_if_fail (BOBGUI_IS_CSS_STATIC_STYLE (style));
  bobgui_internal_return_if_fail (parent_style == NULL || BOBGUI_IS_CSS_STYLE (parent_style));

  if (lookup->custom_values)
    {
      GHashTableIter iter;
      gpointer id;
      BobguiCssVariableValue *value;

      g_clear_pointer (&style->variables, bobgui_css_variable_set_unref);
      style->variables = bobgui_css_variable_set_new ();

      g_hash_table_iter_init (&iter, lookup->custom_values);

      while (g_hash_table_iter_next (&iter, &id, (gpointer) &value))
        bobgui_css_variable_set_add (style->variables, GPOINTER_TO_INT (id), value);

      bobgui_css_variable_set_resolve_cycles (style->variables);

      if (parent_style)
        {
          bobgui_css_variable_set_set_parent (style->variables,
                                           parent_style->variables);
        }
    }
  else if (parent_style && parent_style->variables)
    {
      g_clear_pointer (&style->variables, bobgui_css_variable_set_unref);
      style->variables = bobgui_css_variable_set_ref (parent_style->variables);
    }

  context.provider = provider;
  context.style = (BobguiCssStyle *) sstyle;
  context.parent_style = parent_style;
  context.variables = NULL;
  context.shorthands = shorthands;

  if (_bobgui_bitmask_is_empty (_bobgui_css_lookup_get_set_values (lookup)))
    {
      style->background = (BobguiCssBackgroundValues *)bobgui_css_values_ref (bobgui_css_background_initial_values);
      style->border = (BobguiCssBorderValues *)bobgui_css_values_ref (bobgui_css_border_initial_values);
      style->outline = (BobguiCssOutlineValues *)bobgui_css_values_ref (bobgui_css_outline_initial_values);
      style->text_decoration = (BobguiCssTextDecorationValues *)bobgui_css_values_ref (bobgui_css_text_decoration_initial_values);
      style->animation = (BobguiCssAnimationValues *)bobgui_css_values_ref (bobgui_css_animation_initial_values);
      style->transition = (BobguiCssTransitionValues *)bobgui_css_values_ref (bobgui_css_transition_initial_values);
      style->size = (BobguiCssSizeValues *)bobgui_css_values_ref (bobgui_css_size_initial_values);
      style->other = (BobguiCssOtherValues *)bobgui_css_values_ref (bobgui_css_other_initial_values);

      if (parent_style)
        {
          style->core = (BobguiCssCoreValues *)bobgui_css_values_ref ((BobguiCssValues *)parent_style->core);
          style->icon = (BobguiCssIconValues *)bobgui_css_values_ref ((BobguiCssValues *)parent_style->icon);
          style->font = (BobguiCssFontValues *)bobgui_css_values_ref ((BobguiCssValues *)parent_style->font);
          style->font_variant = (BobguiCssFontVariantValues *)bobgui_css_values_ref ((BobguiCssValues *)parent_style->font_variant);
        }
      else
        {
          bobgui_css_core_values_new_compute (sstyle, lookup, &context);
          bobgui_css_icon_values_new_compute (sstyle, lookup, &context);
          bobgui_css_font_values_new_compute (sstyle, lookup, &context);
          bobgui_css_font_variant_values_new_compute (sstyle, lookup, &context);
        }

      goto resolve;
    }

  if (parent_style && bobgui_css_core_values_unset (lookup))
    style->core = (BobguiCssCoreValues *)bobgui_css_values_ref ((BobguiCssValues *)parent_style->core);
  else
    bobgui_css_core_values_new_compute (sstyle, lookup, &context);

  if (bobgui_css_background_values_unset (lookup))
    style->background = (BobguiCssBackgroundValues *)bobgui_css_values_ref (bobgui_css_background_initial_values);
  else
    bobgui_css_background_values_new_compute (sstyle, lookup, &context);

  if (bobgui_css_border_values_unset (lookup))
    style->border = (BobguiCssBorderValues *)bobgui_css_values_ref (bobgui_css_border_initial_values);
  else
    bobgui_css_border_values_new_compute (sstyle, lookup, &context);

  if (parent_style && bobgui_css_icon_values_unset (lookup))
    style->icon = (BobguiCssIconValues *)bobgui_css_values_ref ((BobguiCssValues *)parent_style->icon);
  else
    bobgui_css_icon_values_new_compute (sstyle, lookup, &context);

  if (bobgui_css_outline_values_unset (lookup))
    style->outline = (BobguiCssOutlineValues *)bobgui_css_values_ref (bobgui_css_outline_initial_values);
  else
    bobgui_css_outline_values_new_compute (sstyle, lookup, &context);

  if (parent_style && bobgui_css_font_values_unset (lookup))
    style->font = (BobguiCssFontValues *)bobgui_css_values_ref ((BobguiCssValues *)parent_style->font);
  else
    bobgui_css_font_values_new_compute (sstyle, lookup, &context);

  if (bobgui_css_text_decoration_values_unset (lookup))
    style->text_decoration = (BobguiCssTextDecorationValues *)bobgui_css_values_ref (bobgui_css_text_decoration_initial_values);
  else
    bobgui_css_text_decoration_values_new_compute (sstyle, lookup, &context);

  if (parent_style && bobgui_css_font_variant_values_unset (lookup))
    style->font_variant = (BobguiCssFontVariantValues *)bobgui_css_values_ref ((BobguiCssValues *)parent_style->font_variant);
  else
    bobgui_css_font_variant_values_new_compute (sstyle, lookup, &context);

  if (bobgui_css_animation_values_unset (lookup))
    style->animation = (BobguiCssAnimationValues *)bobgui_css_values_ref (bobgui_css_animation_initial_values);
  else
    bobgui_css_animation_values_new_compute (sstyle, lookup, &context);

  if (bobgui_css_transition_values_unset (lookup))
    style->transition = (BobguiCssTransitionValues *)bobgui_css_values_ref (bobgui_css_transition_initial_values);
  else
    bobgui_css_transition_values_new_compute (sstyle, lookup, &context);

  if (bobgui_css_size_values_unset (lookup))
    style->size = (BobguiCssSizeValues *)bobgui_css_values_ref (bobgui_css_size_initial_values);
  else
    bobgui_css_size_values_new_compute (sstyle, lookup, &context);

  if (bobgui_css_other_values_unset (lookup))
    style->other = (BobguiCssOtherValues *)bobgui_css_values_ref (bobgui_css_other_initial_values);
  else
    bobgui_css_other_values_new_compute (sstyle, lookup, &context);

resolve:
  bobgui_css_style_resolve_used_values (style, &context);

  for (unsigned int i = 0; i < BOBGUI_CSS_SHORTHAND_PROPERTY_N_PROPERTIES; i++)
    {
      if (shorthands[i])
        bobgui_css_value_unref (shorthands[i]);
    }
}

BobguiCssStyle *
bobgui_css_static_style_new_compute (BobguiStyleProvider             *provider,
                                  const BobguiCountingBloomFilter *filter,
                                  BobguiCssNode                   *node,
                                  BobguiCssChange                  change)
{
  BobguiCssStaticStyle *result;
  BobguiCssLookup lookup;
  BobguiCssNode *parent;

  _bobgui_css_lookup_init (&lookup);

  if (node)
    bobgui_style_provider_lookup (provider,
                               filter,
                               node,
                               &lookup,
                               change == 0 ? &change : NULL);

  result = g_object_new (BOBGUI_TYPE_CSS_STATIC_STYLE, NULL);

  result->change = change;

  if (node)
    parent = bobgui_css_node_get_parent (node);
  else
    parent = NULL;

  bobgui_css_lookup_resolve (&lookup,
                          provider,
                          result,
                          parent ? bobgui_css_node_get_style (parent) : NULL);

  _bobgui_css_lookup_destroy (&lookup);

  return BOBGUI_CSS_STYLE (result);
}

G_STATIC_ASSERT (BOBGUI_CSS_PROPERTY_BORDER_TOP_STYLE == BOBGUI_CSS_PROPERTY_BORDER_TOP_WIDTH - 1);
G_STATIC_ASSERT (BOBGUI_CSS_PROPERTY_BORDER_RIGHT_STYLE == BOBGUI_CSS_PROPERTY_BORDER_RIGHT_WIDTH - 1);
G_STATIC_ASSERT (BOBGUI_CSS_PROPERTY_BORDER_BOTTOM_STYLE == BOBGUI_CSS_PROPERTY_BORDER_BOTTOM_WIDTH - 1);
G_STATIC_ASSERT (BOBGUI_CSS_PROPERTY_BORDER_LEFT_STYLE == BOBGUI_CSS_PROPERTY_BORDER_LEFT_WIDTH - 1);
G_STATIC_ASSERT (BOBGUI_CSS_PROPERTY_OUTLINE_STYLE == BOBGUI_CSS_PROPERTY_OUTLINE_WIDTH - 1);

static void
bobgui_css_static_style_compute_value (BobguiCssStaticStyle    *style,
                                    guint                 id,
                                    BobguiCssValue          *specified,
                                    BobguiCssSection        *section,
                                    BobguiCssComputeContext *context)
{
  BobguiCssValue *value, *original_value;
  BobguiBorderStyle border_style;

  bobgui_internal_return_if_fail (id < BOBGUI_CSS_PROPERTY_N_PROPERTIES);

  /* special case according to http://dev.w3.org/csswg/css-backgrounds/#the-border-width */
  switch (id)
    {
      /* We have them ordered in bobguicssstylepropertyimpl.c accordingly, so the
       * border styles are already computed when we compute the border widths.
       *
       * Note that we rely on ..._STYLE == ..._WIDTH - 1 here.
       */
      case BOBGUI_CSS_PROPERTY_BORDER_TOP_WIDTH:
      case BOBGUI_CSS_PROPERTY_BORDER_RIGHT_WIDTH:
      case BOBGUI_CSS_PROPERTY_BORDER_BOTTOM_WIDTH:
      case BOBGUI_CSS_PROPERTY_BORDER_LEFT_WIDTH:
      case BOBGUI_CSS_PROPERTY_OUTLINE_WIDTH:
        border_style = _bobgui_css_border_style_value_get (bobgui_css_style_get_value ((BobguiCssStyle *)style, id - 1));
        if (border_style == BOBGUI_BORDER_STYLE_NONE || border_style == BOBGUI_BORDER_STYLE_HIDDEN)
          {
            bobgui_css_static_style_set_value (style, id, bobgui_css_dimension_value_new (0, BOBGUI_CSS_NUMBER), NULL, section);
            return;
          }
        break;

      default:
        /* Go ahead */
        break;
    }

  /* http://www.w3.org/TR/css3-cascade/#cascade
   * Then, for every element, the value for each property can be found
   * by following this pseudo-algorithm:
   * 1) Identify all declarations that apply to the element
   */
  if (specified)
    {
      value = bobgui_css_value_compute (specified, id, context);

      if (bobgui_css_value_contains_variables (specified))
        original_value = specified;
      else
        original_value = NULL;
    }
  else if (context->parent_style && _bobgui_css_style_property_is_inherit (_bobgui_css_style_property_lookup_by_id (id)))
    {
      BobguiCssValue *parent_original_value;

      /* Just take the style from the parent */
      value = bobgui_css_value_ref (bobgui_css_style_get_computed_value (context->parent_style, id));

      parent_original_value = bobgui_css_style_get_original_value (context->parent_style, id);

      if (parent_original_value)
        original_value = parent_original_value;
      else
        original_value = NULL;
    }
  else
    {
      value = _bobgui_css_initial_value_new_compute (id, context);
      original_value = NULL;
    }

  bobgui_css_static_style_set_value (style, id, value, original_value, section);
}

BobguiCssChange
bobgui_css_static_style_get_change (BobguiCssStaticStyle *style)
{
  g_return_val_if_fail (BOBGUI_IS_CSS_STATIC_STYLE (style), BOBGUI_CSS_CHANGE_ANY);

  return style->change;
}

void
bobgui_css_custom_values_compute_changes_and_affects (BobguiCssStyle    *style1,
                                                   BobguiCssStyle    *style2,
                                                   BobguiBitmask    **changes,
                                                   BobguiCssAffects  *affects)
{
  if (bobgui_css_variable_set_equal (style1->variables, style2->variables))
    return;

  *changes = _bobgui_bitmask_set (*changes, BOBGUI_CSS_PROPERTY_CUSTOM, TRUE);
}
