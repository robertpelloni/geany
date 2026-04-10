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

#include "bobguiprivate.h"
#include "bobguicssstyleprivate.h"

#include "bobguicssanimationprivate.h"
#include "bobguicssarrayvalueprivate.h"
#include "bobguicsscustompropertypoolprivate.h"
#include "bobguicssenumvalueprivate.h"
#include "bobguicssimagevalueprivate.h"
#include "bobguicssinheritvalueprivate.h"
#include "bobguicssinitialvalueprivate.h"
#include "bobguicssnumbervalueprivate.h"
#include "bobguicsscolorvalueprivate.h"
#include "bobguicsspalettevalueprivate.h"
#include "bobguicssshadowvalueprivate.h"
#include "bobguicssshorthandpropertyprivate.h"
#include "bobguicssstringvalueprivate.h"
#include "bobguicssfontvariationsvalueprivate.h"
#include "bobguicssfontfeaturesvalueprivate.h"
#include "bobguicsslineheightvalueprivate.h"
#include "bobguicssstylepropertyprivate.h"
#include "bobguicsstransitionprivate.h"
#include "bobguistyleanimationprivate.h"
#include "bobguistylepropertyprivate.h"
#include "bobguistyleproviderprivate.h"
#include "bobguicssvaluesprivate.h"

G_DEFINE_ABSTRACT_TYPE (BobguiCssStyle, bobgui_css_style, G_TYPE_OBJECT)

static BobguiCssSection *
bobgui_css_style_real_get_section (BobguiCssStyle *style,
                                guint        id)
{
  return NULL;
}

static gboolean
bobgui_css_style_real_is_static (BobguiCssStyle *style)
{
  return TRUE;
}


static void
bobgui_css_style_finalize (GObject *object)
{
  BobguiCssStyle *style = BOBGUI_CSS_STYLE (object);

  bobgui_css_values_unref ((BobguiCssValues *)style->core);
  bobgui_css_values_unref ((BobguiCssValues *)style->background);
  bobgui_css_values_unref ((BobguiCssValues *)style->border);
  bobgui_css_values_unref ((BobguiCssValues *)style->icon);
  bobgui_css_values_unref ((BobguiCssValues *)style->outline);
  bobgui_css_values_unref ((BobguiCssValues *)style->font);
  bobgui_css_values_unref ((BobguiCssValues *)style->text_decoration);
  bobgui_css_values_unref ((BobguiCssValues *)style->font_variant);
  bobgui_css_values_unref ((BobguiCssValues *)style->animation);
  bobgui_css_values_unref ((BobguiCssValues *)style->transition);
  bobgui_css_values_unref ((BobguiCssValues *)style->size);
  bobgui_css_values_unref ((BobguiCssValues *)style->other);
  bobgui_css_values_unref ((BobguiCssValues *)style->used);

  if (style->variables)
    bobgui_css_variable_set_unref (style->variables);

  G_OBJECT_CLASS (bobgui_css_style_parent_class)->finalize (object);
}

static void
bobgui_css_style_class_init (BobguiCssStyleClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = bobgui_css_style_finalize;

  klass->get_section = bobgui_css_style_real_get_section;
  klass->is_static = bobgui_css_style_real_is_static;
}

static void
bobgui_css_style_init (BobguiCssStyle *style)
{
}

BobguiCssValue *
bobgui_css_style_get_value (BobguiCssStyle *style,
                         guint        id)
{
  return bobgui_css_style_get_used_value (style, id);
}

BobguiCssValue *
bobgui_css_style_get_used_value (BobguiCssStyle *style,
                              guint        id)
{
  switch (id)
    {
    case BOBGUI_CSS_PROPERTY_COLOR:
      return style->used->color;
    case BOBGUI_CSS_PROPERTY_ICON_PALETTE:
      return style->used->icon_palette;
    case BOBGUI_CSS_PROPERTY_BACKGROUND_COLOR:
      return style->used->background_color;
    case BOBGUI_CSS_PROPERTY_BOX_SHADOW:
      return style->used->box_shadow;
    case BOBGUI_CSS_PROPERTY_BACKGROUND_IMAGE:
      return style->used->background_image;
    case BOBGUI_CSS_PROPERTY_BORDER_TOP_COLOR:
      return style->used->border_top_color;
    case BOBGUI_CSS_PROPERTY_BORDER_RIGHT_COLOR:
      return style->used->border_right_color;
    case BOBGUI_CSS_PROPERTY_BORDER_BOTTOM_COLOR:
      return style->used->border_bottom_color;
    case BOBGUI_CSS_PROPERTY_BORDER_LEFT_COLOR:
      return style->used->border_left_color;
    case BOBGUI_CSS_PROPERTY_BORDER_IMAGE_SOURCE:
      return style->used->border_image_source;
    case BOBGUI_CSS_PROPERTY_ICON_SHADOW:
      return style->used->icon_shadow;
    case BOBGUI_CSS_PROPERTY_OUTLINE_COLOR:
      return style->used->outline_color;
    case BOBGUI_CSS_PROPERTY_CARET_COLOR:
      return style->used->caret_color;
    case BOBGUI_CSS_PROPERTY_SECONDARY_CARET_COLOR:
      return style->used->secondary_caret_color;
    case BOBGUI_CSS_PROPERTY_TEXT_SHADOW:
      return style->used->text_shadow;
    case BOBGUI_CSS_PROPERTY_TEXT_DECORATION_COLOR:
      return style->used->text_decoration_color;
    case BOBGUI_CSS_PROPERTY_ICON_SOURCE:
      return style->used->icon_source;

    default:
      return bobgui_css_style_get_computed_value (style, id);
    }
}

BobguiCssValue *
bobgui_css_style_get_computed_value (BobguiCssStyle *style,
                                  guint        id)
{
  switch (id)
    {
    case BOBGUI_CSS_PROPERTY_COLOR:
      return style->core->color;
    case BOBGUI_CSS_PROPERTY_DPI:
      return style->core->dpi;
    case BOBGUI_CSS_PROPERTY_FONT_SIZE:
      return style->core->font_size;
    case BOBGUI_CSS_PROPERTY_ICON_PALETTE:
      return style->core->icon_palette;
    case BOBGUI_CSS_PROPERTY_BACKGROUND_COLOR:
      return style->background->background_color;
    case BOBGUI_CSS_PROPERTY_FONT_FAMILY:
      return style->font->font_family;
    case BOBGUI_CSS_PROPERTY_FONT_STYLE:
      return style->font->font_style;
    case BOBGUI_CSS_PROPERTY_FONT_WEIGHT:
      return style->font->font_weight;
    case BOBGUI_CSS_PROPERTY_FONT_STRETCH:
      return style->font->font_stretch;
    case BOBGUI_CSS_PROPERTY_LETTER_SPACING:
      return style->font->letter_spacing;
    case BOBGUI_CSS_PROPERTY_LINE_HEIGHT:
      return style->font->line_height;
    case BOBGUI_CSS_PROPERTY_TEXT_DECORATION_LINE:
      return style->text_decoration->text_decoration_line;
    case BOBGUI_CSS_PROPERTY_TEXT_DECORATION_COLOR:
      return style->text_decoration->text_decoration_color;
    case BOBGUI_CSS_PROPERTY_TEXT_DECORATION_STYLE:
      return style->text_decoration->text_decoration_style;
    case BOBGUI_CSS_PROPERTY_TEXT_TRANSFORM:
      return style->font_variant->text_transform;
    case BOBGUI_CSS_PROPERTY_FONT_KERNING:
      return style->font_variant->font_kerning;
    case BOBGUI_CSS_PROPERTY_FONT_VARIANT_LIGATURES:
      return style->font_variant->font_variant_ligatures;
    case BOBGUI_CSS_PROPERTY_FONT_VARIANT_POSITION:
      return style->font_variant->font_variant_position;
    case BOBGUI_CSS_PROPERTY_FONT_VARIANT_CAPS:
      return style->font_variant->font_variant_caps;
    case BOBGUI_CSS_PROPERTY_FONT_VARIANT_NUMERIC:
      return style->font_variant->font_variant_numeric;
    case BOBGUI_CSS_PROPERTY_FONT_VARIANT_ALTERNATES:
      return style->font_variant->font_variant_alternates;
    case BOBGUI_CSS_PROPERTY_FONT_VARIANT_EAST_ASIAN:
      return style->font_variant->font_variant_east_asian;
    case BOBGUI_CSS_PROPERTY_TEXT_SHADOW:
      return style->font->text_shadow;
    case BOBGUI_CSS_PROPERTY_BOX_SHADOW:
      return style->background->box_shadow;
    case BOBGUI_CSS_PROPERTY_MARGIN_TOP:
      return style->size->margin_top;
    case BOBGUI_CSS_PROPERTY_MARGIN_LEFT:
      return style->size->margin_left;
    case BOBGUI_CSS_PROPERTY_MARGIN_BOTTOM:
      return style->size->margin_bottom;
    case BOBGUI_CSS_PROPERTY_MARGIN_RIGHT:
      return style->size->margin_right;
    case BOBGUI_CSS_PROPERTY_PADDING_TOP:
      return style->size->padding_top;
    case BOBGUI_CSS_PROPERTY_PADDING_LEFT:
      return style->size->padding_left;
    case BOBGUI_CSS_PROPERTY_PADDING_BOTTOM:
      return style->size->padding_bottom;
    case BOBGUI_CSS_PROPERTY_PADDING_RIGHT:
      return style->size->padding_right;
    case BOBGUI_CSS_PROPERTY_BORDER_TOP_STYLE:
      return style->border->border_top_style;
    case BOBGUI_CSS_PROPERTY_BORDER_TOP_WIDTH:
      return style->border->border_top_width;
    case BOBGUI_CSS_PROPERTY_BORDER_LEFT_STYLE:
      return style->border->border_left_style;
    case BOBGUI_CSS_PROPERTY_BORDER_LEFT_WIDTH:
      return style->border->border_left_width;
    case BOBGUI_CSS_PROPERTY_BORDER_BOTTOM_STYLE:
      return style->border->border_bottom_style;
    case BOBGUI_CSS_PROPERTY_BORDER_BOTTOM_WIDTH:
      return style->border->border_bottom_width;
    case BOBGUI_CSS_PROPERTY_BORDER_RIGHT_STYLE:
      return style->border->border_right_style;
    case BOBGUI_CSS_PROPERTY_BORDER_RIGHT_WIDTH:
      return style->border->border_right_width;
    case BOBGUI_CSS_PROPERTY_BORDER_TOP_LEFT_RADIUS:
      return style->border->border_top_left_radius;
    case BOBGUI_CSS_PROPERTY_BORDER_TOP_RIGHT_RADIUS:
      return style->border->border_top_right_radius;
    case BOBGUI_CSS_PROPERTY_BORDER_BOTTOM_RIGHT_RADIUS:
      return style->border->border_bottom_right_radius;
    case BOBGUI_CSS_PROPERTY_BORDER_BOTTOM_LEFT_RADIUS:
      return style->border->border_bottom_left_radius;
    case BOBGUI_CSS_PROPERTY_OUTLINE_STYLE:
      return style->outline->outline_style;
    case BOBGUI_CSS_PROPERTY_OUTLINE_WIDTH:
      return style->outline->outline_width;
    case BOBGUI_CSS_PROPERTY_OUTLINE_OFFSET:
      return style->outline->outline_offset;
    case BOBGUI_CSS_PROPERTY_BACKGROUND_CLIP:
      return style->background->background_clip;
    case BOBGUI_CSS_PROPERTY_BACKGROUND_ORIGIN:
      return style->background->background_origin;
    case BOBGUI_CSS_PROPERTY_BACKGROUND_SIZE:
      return style->background->background_size;
    case BOBGUI_CSS_PROPERTY_BACKGROUND_POSITION:
      return style->background->background_position;
    case BOBGUI_CSS_PROPERTY_BORDER_TOP_COLOR:
      return style->border->border_top_color;
    case BOBGUI_CSS_PROPERTY_BORDER_RIGHT_COLOR:
      return style->border->border_right_color;
    case BOBGUI_CSS_PROPERTY_BORDER_BOTTOM_COLOR:
      return style->border->border_bottom_color;
    case BOBGUI_CSS_PROPERTY_BORDER_LEFT_COLOR:
      return style->border->border_left_color;
    case BOBGUI_CSS_PROPERTY_OUTLINE_COLOR:
      return style->outline->outline_color;
    case BOBGUI_CSS_PROPERTY_BACKGROUND_REPEAT:
      return style->background->background_repeat;
    case BOBGUI_CSS_PROPERTY_BACKGROUND_IMAGE:
      return style->background->background_image;
    case BOBGUI_CSS_PROPERTY_BACKGROUND_BLEND_MODE:
      return style->background->background_blend_mode;
    case BOBGUI_CSS_PROPERTY_BORDER_IMAGE_SOURCE:
      return style->border->border_image_source;
    case BOBGUI_CSS_PROPERTY_BORDER_IMAGE_REPEAT:
      return style->border->border_image_repeat;
    case BOBGUI_CSS_PROPERTY_BORDER_IMAGE_SLICE:
      return style->border->border_image_slice;
    case BOBGUI_CSS_PROPERTY_BORDER_IMAGE_WIDTH:
      return style->border->border_image_width;
    case BOBGUI_CSS_PROPERTY_ICON_SOURCE:
      return style->other->icon_source;
    case BOBGUI_CSS_PROPERTY_ICON_SIZE:
      return style->icon->icon_size;
    case BOBGUI_CSS_PROPERTY_ICON_SHADOW:
      return style->icon->icon_shadow;
    case BOBGUI_CSS_PROPERTY_ICON_STYLE:
      return style->icon->icon_style;
    case BOBGUI_CSS_PROPERTY_ICON_TRANSFORM:
      return style->other->icon_transform;
    case BOBGUI_CSS_PROPERTY_ICON_FILTER:
      return style->other->icon_filter;
    case BOBGUI_CSS_PROPERTY_ICON_WEIGHT:
      return style->icon->icon_weight;
    case BOBGUI_CSS_PROPERTY_BORDER_SPACING:
      return style->size->border_spacing;
    case BOBGUI_CSS_PROPERTY_TRANSFORM:
      return style->other->transform;
    case BOBGUI_CSS_PROPERTY_TRANSFORM_ORIGIN:
      return style->other->transform_origin;
    case BOBGUI_CSS_PROPERTY_MIN_WIDTH:
      return style->size->min_width;
    case BOBGUI_CSS_PROPERTY_MIN_HEIGHT:
      return style->size->min_height;
    case BOBGUI_CSS_PROPERTY_TRANSITION_PROPERTY:
      return style->transition->transition_property;
    case BOBGUI_CSS_PROPERTY_TRANSITION_DURATION:
      return style->transition->transition_duration;
    case BOBGUI_CSS_PROPERTY_TRANSITION_TIMING_FUNCTION:
      return style->transition->transition_timing_function;
    case BOBGUI_CSS_PROPERTY_TRANSITION_DELAY:
      return style->transition->transition_delay;
    case BOBGUI_CSS_PROPERTY_ANIMATION_NAME:
      return style->animation->animation_name;
    case BOBGUI_CSS_PROPERTY_ANIMATION_DURATION:
      return style->animation->animation_duration;
    case BOBGUI_CSS_PROPERTY_ANIMATION_TIMING_FUNCTION:
      return style->animation->animation_timing_function;
    case BOBGUI_CSS_PROPERTY_ANIMATION_ITERATION_COUNT:
      return style->animation->animation_iteration_count;
    case BOBGUI_CSS_PROPERTY_ANIMATION_DIRECTION:
      return style->animation->animation_direction;
    case BOBGUI_CSS_PROPERTY_ANIMATION_PLAY_STATE:
      return style->animation->animation_play_state;
    case BOBGUI_CSS_PROPERTY_ANIMATION_DELAY:
      return style->animation->animation_delay;
    case BOBGUI_CSS_PROPERTY_ANIMATION_FILL_MODE:
      return style->animation->animation_fill_mode;
    case BOBGUI_CSS_PROPERTY_OPACITY:
      return style->other->opacity;
    case BOBGUI_CSS_PROPERTY_BACKDROP_FILTER:
      return style->other->backdrop_filter;
    case BOBGUI_CSS_PROPERTY_FILTER:
      return style->other->filter;
    case BOBGUI_CSS_PROPERTY_CARET_COLOR:
      return style->font->caret_color;
    case BOBGUI_CSS_PROPERTY_SECONDARY_CARET_COLOR:
      return style->font->secondary_caret_color;
    case BOBGUI_CSS_PROPERTY_FONT_FEATURE_SETTINGS:
      return style->font->font_feature_settings;
    case BOBGUI_CSS_PROPERTY_FONT_VARIATION_SETTINGS:
      return style->font->font_variation_settings;

    default:
      g_assert_not_reached ();
    }
}

BobguiCssSection *
bobgui_css_style_get_section (BobguiCssStyle *style,
                           guint        id)
{
  bobgui_internal_return_val_if_fail (BOBGUI_IS_CSS_STYLE (style), NULL);

  return BOBGUI_CSS_STYLE_GET_CLASS (style)->get_section (style, id);
}

gboolean
bobgui_css_style_is_static (BobguiCssStyle *style)
{
  return BOBGUI_CSS_STYLE_GET_CLASS (style)->is_static (style);
}

BobguiCssStaticStyle *
bobgui_css_style_get_static_style (BobguiCssStyle *style)
{
  bobgui_internal_return_val_if_fail (BOBGUI_IS_CSS_STYLE (style), NULL);

  return BOBGUI_CSS_STYLE_GET_CLASS (style)->get_static_style (style);
}

BobguiCssValue *
bobgui_css_style_get_original_value (BobguiCssStyle *style,
                                  guint        id)
{
  bobgui_internal_return_val_if_fail (BOBGUI_IS_CSS_STYLE (style), NULL);

  return BOBGUI_CSS_STYLE_GET_CLASS (style)->get_original_value (style, id);
}

/*
 * bobgui_css_style_print:
 * @style: a `BobguiCssStyle`
 * @string: the `GString` to print to
 * @indent: level of indentation to use
 * @skip_initial: %TRUE to skip properties that have their initial value
 *
 * Print the @style to @string, in CSS format. Every property is printed
 * on a line by itself, indented by @indent spaces. If @skip_initial is
 * %TRUE, properties are only printed if their value in @style is different
 * from the initial value of the property.
 *
 * Returns: %TRUE is any properties have been printed
 */
gboolean
bobgui_css_style_print (BobguiCssStyle *style,
                     GString     *string,
                     guint        indent,
                     gboolean     skip_initial)
{
  guint i;
  gboolean retval = FALSE;

  g_return_val_if_fail (BOBGUI_IS_CSS_STYLE (style), FALSE);
  g_return_val_if_fail (string != NULL, FALSE);

  for (i = 0; i < _bobgui_css_style_property_get_n_properties (); i++)
    {
      BobguiCssSection *section;
      BobguiCssStyleProperty *prop;
      BobguiCssValue *value, *computed, *initial;
      const char *name;

      prop = _bobgui_css_style_property_lookup_by_id (i);
      name = _bobgui_style_property_get_name (BOBGUI_STYLE_PROPERTY (prop));
      computed = bobgui_css_style_get_computed_value (style, i);
      value = bobgui_css_style_get_used_value (style, i);
      initial = _bobgui_css_style_property_get_initial_value (prop);

      section = bobgui_css_style_get_section (style, i);
      if (skip_initial)
        {
          if (!section &&
              (computed == initial ||
               !bobgui_css_value_contains_current_color (computed)))
            continue;
        }

      g_string_append_printf (string, "%*s%s: ", indent, "", name);
      bobgui_css_value_print (value, string);
      g_string_append_c (string, ';');

      if (section)
        {
          g_string_append (string, " /* ");
          bobgui_css_section_print (section, string);
          g_string_append (string, " */");
        }

      g_string_append_c (string, '\n');

      retval = TRUE;
    }

    if (style->variables)
      {
        BobguiCssCustomPropertyPool *pool = bobgui_css_custom_property_pool_get ();
        GArray *ids = bobgui_css_variable_set_list_ids (style->variables);

        for (i = 0; i < ids->len; i++)
          {
            int id = g_array_index (ids, int, i);
            const char *name = bobgui_css_custom_property_pool_get_name (pool, id);
            BobguiCssVariableSet *source;
            BobguiCssVariableValue *value = bobgui_css_variable_set_lookup (style->variables, id, &source);

            if (!value)
              continue;

            if (source != style->variables && skip_initial)
              continue;

            g_string_append_printf (string, "%*s%s: ", indent, "", name);
            bobgui_css_variable_value_print (value, string);
            g_string_append_c (string, ';');

            if (value->section)
              {
                g_string_append (string, " /* ");
                bobgui_css_section_print (value->section, string);
                g_string_append (string, " */");
              }

            g_string_append_c (string, '\n');
          }

        retval = TRUE;

        g_array_unref (ids);
      }

  return retval;
}

char *
bobgui_css_style_to_string (BobguiCssStyle *style)
{
  GString *string;

  g_return_val_if_fail (BOBGUI_IS_CSS_STYLE (style), NULL);

  string = g_string_new ("");

  bobgui_css_style_print (style, string, 0, FALSE);

  return g_string_free (string, FALSE);
}

static PangoUnderline
get_pango_underline_from_style (BobguiTextDecorationStyle style)
{
  switch (style)
    {
    case BOBGUI_CSS_TEXT_DECORATION_STYLE_DOUBLE:
      return PANGO_UNDERLINE_DOUBLE_LINE;
    case BOBGUI_CSS_TEXT_DECORATION_STYLE_WAVY:
      return PANGO_UNDERLINE_ERROR_LINE;
    case BOBGUI_CSS_TEXT_DECORATION_STYLE_SOLID:
    default:
      return PANGO_UNDERLINE_SINGLE_LINE;
    }

  g_return_val_if_reached (PANGO_UNDERLINE_SINGLE);
}

PangoTextTransform
bobgui_css_style_get_pango_text_transform (BobguiCssStyle *style)
{
  switch (_bobgui_css_text_transform_value_get (style->font_variant->text_transform))
    {
    case BOBGUI_CSS_TEXT_TRANSFORM_NONE:
      return PANGO_TEXT_TRANSFORM_NONE;
    case BOBGUI_CSS_TEXT_TRANSFORM_LOWERCASE:
      return PANGO_TEXT_TRANSFORM_LOWERCASE;
    case BOBGUI_CSS_TEXT_TRANSFORM_UPPERCASE:
      return PANGO_TEXT_TRANSFORM_UPPERCASE;
    case BOBGUI_CSS_TEXT_TRANSFORM_CAPITALIZE:
      return PANGO_TEXT_TRANSFORM_CAPITALIZE;
    default:
      return PANGO_TEXT_TRANSFORM_NONE;
    }
}

static PangoOverline
get_pango_overline_from_style (BobguiTextDecorationStyle style)
{
  return PANGO_OVERLINE_SINGLE;
}

static PangoAttrList *
add_pango_attr (PangoAttrList  *attrs,
                PangoAttribute *attr)
{
  if (attrs == NULL)
    attrs = pango_attr_list_new ();

  pango_attr_list_insert (attrs, attr);

  return attrs;
}

static void
append_separated (GString    **s,
                  const char  *text)
{
  if (G_UNLIKELY (!*s))
    *s = g_string_new (NULL);

  if ((*s)->len > 0)
    g_string_append (*s, ", ");

  g_string_append (*s, text);
}

char *
bobgui_css_style_compute_font_features (BobguiCssStyle *style)
{
  BobguiCssFontVariantLigature ligatures;
  BobguiCssFontVariantNumeric numeric;
  BobguiCssFontVariantEastAsian east_asian;
  char *settings;
  GString *s = NULL;

  switch (_bobgui_css_font_kerning_value_get (style->font_variant->font_kerning))
    {
    case BOBGUI_CSS_FONT_KERNING_NORMAL:
      append_separated (&s, "kern 1");
      break;
    case BOBGUI_CSS_FONT_KERNING_NONE:
      append_separated (&s, "kern 0");
      break;
    case BOBGUI_CSS_FONT_KERNING_AUTO:
    default:
      break;
    }

  ligatures = _bobgui_css_font_variant_ligature_value_get (style->font_variant->font_variant_ligatures);
  if (ligatures == BOBGUI_CSS_FONT_VARIANT_LIGATURE_NORMAL)
    {
      /* all defaults */
    }
  else if (ligatures == BOBGUI_CSS_FONT_VARIANT_LIGATURE_NONE)
    append_separated (&s, "liga 0, clig 0, dlig 0, hlig 0, calt 0");
  else
    {
      if (ligatures & BOBGUI_CSS_FONT_VARIANT_LIGATURE_COMMON_LIGATURES)
        append_separated (&s, "liga 1, clig 1");
      if (ligatures & BOBGUI_CSS_FONT_VARIANT_LIGATURE_NO_COMMON_LIGATURES)
        append_separated (&s, "liga 0, clig 0");
      if (ligatures & BOBGUI_CSS_FONT_VARIANT_LIGATURE_DISCRETIONARY_LIGATURES)
        append_separated (&s, "dlig 1");
      if (ligatures & BOBGUI_CSS_FONT_VARIANT_LIGATURE_NO_DISCRETIONARY_LIGATURES)
        append_separated (&s, "dlig 0");
      if (ligatures & BOBGUI_CSS_FONT_VARIANT_LIGATURE_HISTORICAL_LIGATURES)
        append_separated (&s, "hlig 1");
      if (ligatures & BOBGUI_CSS_FONT_VARIANT_LIGATURE_NO_HISTORICAL_LIGATURES)
        append_separated (&s, "hlig 0");
      if (ligatures & BOBGUI_CSS_FONT_VARIANT_LIGATURE_CONTEXTUAL)
        append_separated (&s, "calt 1");
      if (ligatures & BOBGUI_CSS_FONT_VARIANT_LIGATURE_NO_CONTEXTUAL)
        append_separated (&s, "calt 0");
    }

  switch (_bobgui_css_font_variant_position_value_get (style->font_variant->font_variant_position))
    {
    case BOBGUI_CSS_FONT_VARIANT_POSITION_SUB:
      append_separated (&s, "subs 1");
      break;
    case BOBGUI_CSS_FONT_VARIANT_POSITION_SUPER:
      append_separated (&s, "sups 1");
      break;
    case BOBGUI_CSS_FONT_VARIANT_POSITION_NORMAL:
    default:
      break;
    }

  numeric = _bobgui_css_font_variant_numeric_value_get (style->font_variant->font_variant_numeric);
  if (numeric == BOBGUI_CSS_FONT_VARIANT_NUMERIC_NORMAL)
    {
      /* all defaults */
    }
  else
    {
      if (numeric & BOBGUI_CSS_FONT_VARIANT_NUMERIC_LINING_NUMS)
        append_separated (&s, "lnum 1");
      if (numeric & BOBGUI_CSS_FONT_VARIANT_NUMERIC_OLDSTYLE_NUMS)
        append_separated (&s, "onum 1");
      if (numeric & BOBGUI_CSS_FONT_VARIANT_NUMERIC_PROPORTIONAL_NUMS)
        append_separated (&s, "pnum 1");
      if (numeric & BOBGUI_CSS_FONT_VARIANT_NUMERIC_TABULAR_NUMS)
        append_separated (&s, "tnum 1");
      if (numeric & BOBGUI_CSS_FONT_VARIANT_NUMERIC_DIAGONAL_FRACTIONS)
        append_separated (&s, "frac 1");
      if (numeric & BOBGUI_CSS_FONT_VARIANT_NUMERIC_STACKED_FRACTIONS)
        append_separated (&s, "afrc 1");
      if (numeric & BOBGUI_CSS_FONT_VARIANT_NUMERIC_ORDINAL)
        append_separated (&s, "ordn 1");
      if (numeric & BOBGUI_CSS_FONT_VARIANT_NUMERIC_SLASHED_ZERO)
        append_separated (&s, "zero 1");
    }

  switch (_bobgui_css_font_variant_alternate_value_get (style->font_variant->font_variant_alternates))
    {
    case BOBGUI_CSS_FONT_VARIANT_ALTERNATE_HISTORICAL_FORMS:
      append_separated (&s, "hist 1");
      break;
    case BOBGUI_CSS_FONT_VARIANT_ALTERNATE_NORMAL:
    default:
      break;
    }

  east_asian = _bobgui_css_font_variant_east_asian_value_get (style->font_variant->font_variant_east_asian);
  if (east_asian == BOBGUI_CSS_FONT_VARIANT_EAST_ASIAN_NORMAL)
    {
      /* all defaults */
    }
  else
    {
      if (east_asian & BOBGUI_CSS_FONT_VARIANT_EAST_ASIAN_JIS78)
        append_separated (&s, "jp78 1");
      if (east_asian & BOBGUI_CSS_FONT_VARIANT_EAST_ASIAN_JIS83)
        append_separated (&s, "jp83 1");
      if (east_asian & BOBGUI_CSS_FONT_VARIANT_EAST_ASIAN_JIS90)
        append_separated (&s, "jp90 1");
      if (east_asian & BOBGUI_CSS_FONT_VARIANT_EAST_ASIAN_JIS04)
        append_separated (&s, "jp04 1");
      if (east_asian & BOBGUI_CSS_FONT_VARIANT_EAST_ASIAN_SIMPLIFIED)
        append_separated (&s, "smpl 1");
      if (east_asian & BOBGUI_CSS_FONT_VARIANT_EAST_ASIAN_TRADITIONAL)
        append_separated (&s, "trad 1");
      if (east_asian & BOBGUI_CSS_FONT_VARIANT_EAST_ASIAN_FULL_WIDTH)
        append_separated (&s, "fwid 1");
      if (east_asian & BOBGUI_CSS_FONT_VARIANT_EAST_ASIAN_PROPORTIONAL)
        append_separated (&s, "pwid 1");
      if (east_asian & BOBGUI_CSS_FONT_VARIANT_EAST_ASIAN_RUBY)
        append_separated (&s, "ruby 1");
    }

  settings = bobgui_css_font_features_value_get_features (style->font->font_feature_settings);
  if (settings)
    {
      append_separated (&s, settings);
      g_free (settings);
    }

  if (s)
    return g_string_free (s, FALSE);
  else
    return NULL;
}

PangoAttrList *
bobgui_css_style_get_pango_attributes (BobguiCssStyle *style)
{
  PangoAttrList *attrs = NULL;
  BobguiTextDecorationLine decoration_line;
  BobguiTextDecorationStyle decoration_style;
  const GdkRGBA *color;
  const GdkRGBA *decoration_color;
  gboolean has_decoration_color;
  double letter_spacing;

  /* text-decoration */
  decoration_line = _bobgui_css_text_decoration_line_value_get (style->text_decoration->text_decoration_line);
  decoration_style = _bobgui_css_text_decoration_style_value_get (style->text_decoration->text_decoration_style);

  color = bobgui_css_color_value_get_rgba (style->used->color);
  decoration_color = bobgui_css_color_value_get_rgba (style->used->text_decoration_color);

  has_decoration_color = !gdk_rgba_equal (color, decoration_color);

  if (decoration_line & BOBGUI_CSS_TEXT_DECORATION_LINE_UNDERLINE)
    {
      attrs = add_pango_attr (attrs, pango_attr_underline_new (get_pango_underline_from_style (decoration_style)));
      if (has_decoration_color)
        attrs = add_pango_attr (attrs, pango_attr_underline_color_new (decoration_color->red * 65535. + 0.5,
                                                                       decoration_color->green * 65535. + 0.5,
                                                                       decoration_color->blue * 65535. + 0.5));
    }
  if (decoration_line & BOBGUI_CSS_TEXT_DECORATION_LINE_OVERLINE)
    {
      attrs = add_pango_attr (attrs, pango_attr_overline_new (get_pango_overline_from_style (decoration_style)));
      if (has_decoration_color)
        attrs = add_pango_attr (attrs, pango_attr_overline_color_new (decoration_color->red * 65535. + 0.5,
                                                                      decoration_color->green * 65535. + 0.5,
                                                                      decoration_color->blue * 65535. + 0.5));
    }
  if (decoration_line & BOBGUI_CSS_TEXT_DECORATION_LINE_LINE_THROUGH)
    {
      attrs = add_pango_attr (attrs, pango_attr_strikethrough_new (TRUE));
      if (has_decoration_color)
        attrs = add_pango_attr (attrs, pango_attr_strikethrough_color_new (decoration_color->red * 65535. + 0.5,
                                                                           decoration_color->green * 65535. + 0.5,
                                                                           decoration_color->blue * 65535. + 0.5));
    }

  /* letter-spacing */
  letter_spacing = bobgui_css_number_value_get (style->font->letter_spacing, 100);
  if (letter_spacing != 0)
    {
      attrs = add_pango_attr (attrs, pango_attr_letter_spacing_new (letter_spacing * PANGO_SCALE));
    }

  /* line-height */
  {
    double height = bobgui_css_line_height_value_get (style->font->line_height);
    if (height != 0.0)
      {
        if (bobgui_css_number_value_get_dimension (style->font->line_height) == BOBGUI_CSS_DIMENSION_LENGTH)
          attrs = add_pango_attr (attrs, pango_attr_line_height_new_absolute (height * PANGO_SCALE));
        else
          attrs = add_pango_attr (attrs, pango_attr_line_height_new (height));
      }
   }

  /* casing variants */
  switch (_bobgui_css_font_variant_caps_value_get (style->font_variant->font_variant_caps))
    {
    case BOBGUI_CSS_FONT_VARIANT_CAPS_SMALL_CAPS:
      attrs = add_pango_attr (attrs, pango_attr_variant_new (PANGO_VARIANT_SMALL_CAPS));
      break;
    case BOBGUI_CSS_FONT_VARIANT_CAPS_ALL_SMALL_CAPS:
      attrs = add_pango_attr (attrs, pango_attr_variant_new (PANGO_VARIANT_ALL_SMALL_CAPS));
      break;
    case BOBGUI_CSS_FONT_VARIANT_CAPS_PETITE_CAPS:
      attrs = add_pango_attr (attrs, pango_attr_variant_new (PANGO_VARIANT_PETITE_CAPS));
      break;
    case BOBGUI_CSS_FONT_VARIANT_CAPS_ALL_PETITE_CAPS:
      attrs = add_pango_attr (attrs, pango_attr_variant_new (PANGO_VARIANT_ALL_PETITE_CAPS));
      break;
    case BOBGUI_CSS_FONT_VARIANT_CAPS_UNICASE:
      attrs = add_pango_attr (attrs, pango_attr_variant_new (PANGO_VARIANT_UNICASE));
      break;
    case BOBGUI_CSS_FONT_VARIANT_CAPS_TITLING_CAPS:
      attrs = add_pango_attr (attrs, pango_attr_variant_new (PANGO_VARIANT_TITLE_CAPS));
      break;
    case BOBGUI_CSS_FONT_VARIANT_CAPS_NORMAL:
    default:
      break;
    }

  /* OpenType features */
  {
    char *font_features = bobgui_css_style_compute_font_features (style);

    if (font_features)
      {
        attrs = add_pango_attr (attrs, pango_attr_font_features_new (font_features));
        g_free (font_features);
      }
  }

  /* text-transform */
  {
    PangoTextTransform transform = bobgui_css_style_get_pango_text_transform (style);

    if (transform != PANGO_TEXT_TRANSFORM_NONE)
      attrs = add_pango_attr (attrs, pango_attr_text_transform_new (transform));
  }

  return attrs;
}

PangoFontDescription *
bobgui_css_style_get_pango_font (BobguiCssStyle *style)
{
  PangoFontDescription *description;
  BobguiCssValue *v;
  char *str;

  description = pango_font_description_new ();

  v = style->font->font_family;
  if (_bobgui_css_array_value_get_n_values (v) > 1)
    {
      int i;
      GString *s = g_string_new ("");

      for (i = 0; i < _bobgui_css_array_value_get_n_values (v); i++)
        {
          if (i > 0)
            g_string_append (s, ",");
          g_string_append (s, _bobgui_css_string_value_get (_bobgui_css_array_value_get_nth (v, i)));
        }

      pango_font_description_set_family (description, s->str);
      g_string_free (s, TRUE);
    }
  else
    {
      pango_font_description_set_family (description,
                                         _bobgui_css_string_value_get (_bobgui_css_array_value_get_nth (v, 0)));
    }

  v = style->core->font_size;
  pango_font_description_set_absolute_size (description, round (bobgui_css_number_value_get (v, 100) * PANGO_SCALE));

  v = style->font->font_style;
  pango_font_description_set_style (description, _bobgui_css_font_style_value_get (v));

  v = style->font->font_weight;
  pango_font_description_set_weight (description, bobgui_css_number_value_get (v, 100));

  v = style->font->font_stretch;
  pango_font_description_set_stretch (description, _bobgui_css_font_stretch_value_get (v));

  v = style->font->font_variation_settings;
  str = bobgui_css_font_variations_value_get_variations (v);
  if (str)
    pango_font_description_set_variations (description, str);
  g_free (str);

  return description;
}

void
bobgui_css_style_lookup_symbolic_colors (BobguiCssStyle *style,
                                      GdkRGBA      color_out[5])
{
  const char *names[5] = {
    [BOBGUI_SYMBOLIC_COLOR_ERROR] = "error",
    [BOBGUI_SYMBOLIC_COLOR_WARNING] = "warning",
    [BOBGUI_SYMBOLIC_COLOR_SUCCESS] = "success",
    [BOBGUI_SYMBOLIC_COLOR_ACCENT] = "accent",
  };

  color_out[BOBGUI_SYMBOLIC_COLOR_FOREGROUND] = *bobgui_css_color_value_get_rgba (style->used->color);

  for (gsize i = 1; i < G_N_ELEMENTS (names); i++)
    {
      BobguiCssValue *lookup;

      lookup = bobgui_css_palette_value_get_color (style->used->icon_palette, names[i]);

      if (lookup)
        color_out[i] = *bobgui_css_color_value_get_rgba (lookup);
      else
        color_out[i] = color_out[BOBGUI_SYMBOLIC_COLOR_FOREGROUND];
    }
}

/* Refcounted value structs */

static const int values_size[] = {
  sizeof (BobguiCssCoreValues),
  sizeof (BobguiCssBackgroundValues),
  sizeof (BobguiCssBorderValues),
  sizeof (BobguiCssIconValues),
  sizeof (BobguiCssOutlineValues),
  sizeof (BobguiCssFontValues),
  sizeof (BobguiCssTextDecorationValues),
  sizeof (BobguiCssFontVariantValues),
  sizeof (BobguiCssAnimationValues),
  sizeof (BobguiCssTransitionValues),
  sizeof (BobguiCssSizeValues),
  sizeof (BobguiCssOtherValues),
  sizeof (BobguiCssUsedValues)
};

#define TYPE_INDEX(type) ((type) - ((type) % 2))
#define VALUES_SIZE(type) (values_size[(type) / 2])
#define N_VALUES(type) ((VALUES_SIZE(type) - sizeof (BobguiCssValues)) / sizeof (BobguiCssValue *))

#define GET_VALUES(v) (BobguiCssValue **)((guint8 *)(v) + sizeof (BobguiCssValues))

BobguiCssValues *bobgui_css_values_ref (BobguiCssValues *values)
{
  values->ref_count++;

  return values;
}

static void
bobgui_css_values_free (BobguiCssValues *values)
{
  BobguiCssValue **v = GET_VALUES (values);

  for (int i = 0; i < N_VALUES (values->type); i++)
    {
      if (v[i])
        bobgui_css_value_unref (v[i]);
    }

  g_free (values);
}

void bobgui_css_values_unref (BobguiCssValues *values)
{
  if (!values)
    return;

  values->ref_count--;

  if (values->ref_count == 0)
    bobgui_css_values_free (values);
}

BobguiCssValues *
bobgui_css_values_copy (BobguiCssValues *values)
{
  BobguiCssValues *copy;
  BobguiCssValue **v, **v2;

  copy = bobgui_css_values_new (TYPE_INDEX (values->type));

  v = GET_VALUES (values);
  v2 = GET_VALUES (copy);

  for (int i = 0; i < N_VALUES (values->type); i++)
    {
      if (v[i])
        v2[i] = bobgui_css_value_ref (v[i]);
    }

  return copy;
}

BobguiCssValues *
bobgui_css_values_new (BobguiCssValuesType type)
{
  BobguiCssValues *values;

  values = (BobguiCssValues *) g_malloc0 (VALUES_SIZE (type));
  values->ref_count = 1;
  values->type = type;

  return values;
}

BobguiCssVariableValue *
bobgui_css_style_get_custom_property (BobguiCssStyle *style,
                                   int          id)
{
  if (style->variables)
    return bobgui_css_variable_set_lookup (style->variables, id, NULL);

  return NULL;
}

GArray *
bobgui_css_style_list_custom_properties (BobguiCssStyle *style)
{
  if (style->variables)
    return bobgui_css_variable_set_list_ids (style->variables);

  return NULL;
}

BobguiCssValue *
bobgui_css_style_resolve_used_value (BobguiCssStyle          *style,
                                  BobguiCssValue          *value,
                                  guint                 id,
                                  BobguiCssComputeContext *context)
{
  BobguiCssValue *used;

  switch (id)
    {
    case BOBGUI_CSS_PROPERTY_COLOR:
      {
        BobguiCssValue *current;

        if (context->parent_style && context->parent_style->core->color == value)
          used = bobgui_css_value_ref (context->parent_style->used->color);
        else
          {
            if (context->parent_style)
              current = context->parent_style->used->color;
            else
              current = _bobgui_css_style_property_get_initial_value (_bobgui_css_style_property_lookup_by_id (BOBGUI_CSS_PROPERTY_COLOR));

            used = bobgui_css_value_resolve (value, context, current);
          }
      }
      break;

    case BOBGUI_CSS_PROPERTY_BACKGROUND_COLOR:
    case BOBGUI_CSS_PROPERTY_TEXT_DECORATION_COLOR:
    case BOBGUI_CSS_PROPERTY_BORDER_TOP_COLOR:
    case BOBGUI_CSS_PROPERTY_BORDER_RIGHT_COLOR:
    case BOBGUI_CSS_PROPERTY_BORDER_BOTTOM_COLOR:
    case BOBGUI_CSS_PROPERTY_BORDER_LEFT_COLOR:
    case BOBGUI_CSS_PROPERTY_OUTLINE_COLOR:
    case BOBGUI_CSS_PROPERTY_CARET_COLOR:
    case BOBGUI_CSS_PROPERTY_SECONDARY_CARET_COLOR:
    case BOBGUI_CSS_PROPERTY_BOX_SHADOW:
    case BOBGUI_CSS_PROPERTY_TEXT_SHADOW:
    case BOBGUI_CSS_PROPERTY_ICON_SHADOW:
    case BOBGUI_CSS_PROPERTY_ICON_PALETTE:
    case BOBGUI_CSS_PROPERTY_BACKGROUND_IMAGE:
    case BOBGUI_CSS_PROPERTY_ICON_SOURCE:
    case BOBGUI_CSS_PROPERTY_BORDER_IMAGE_SOURCE:
      used = bobgui_css_value_resolve (value, context, style->used->color);
      break;

    default:
      return NULL;
    }

  g_assert (!bobgui_css_value_contains_current_color (used));

  return used;
}

static inline void
bobgui_css_take_value (BobguiCssValue **variable,
                    BobguiCssValue  *value)
{
  if (*variable)
    bobgui_css_value_unref (*variable);
  *variable = value;
}

void
bobgui_css_style_resolve_used_values (BobguiCssStyle          *style,
                                   BobguiCssComputeContext *context)
{
  BobguiCssValue **values;

  if (style->used)
    bobgui_css_values_unref ((BobguiCssValues *) style->used);

  style->used = (BobguiCssUsedValues *) bobgui_css_values_new (BOBGUI_CSS_USED_VALUES);
  values = &style->used->color;

  for (guint i = 0; i < G_N_ELEMENTS (used_props); i++)
    {
      guint id = used_props[i];
      BobguiCssValue *value, *used;

      value = bobgui_css_style_get_computed_value (style, id);

      if (bobgui_css_value_contains_current_color (value))
        used = bobgui_css_style_resolve_used_value (style, value, id, context);
      else
        used = bobgui_css_value_ref (value);

      bobgui_css_take_value (&values[i], used);
    }
}
