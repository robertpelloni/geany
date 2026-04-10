/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2015 Benjamin Otte <otte@gnome.org>
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

#include "bobguicssstylechangeprivate.h"

#include "bobguicssstylepropertyprivate.h"

static void
compute_change (BobguiCssStyleChange *change)
{
  gboolean color_changed = FALSE;

  if (change->old_style->core != change->new_style->core ||
      bobgui_css_value_contains_current_color (change->old_style->core->color))
    {
      bobgui_css_core_values_compute_changes_and_affects (change->old_style,
                                                       change->new_style,
                                                       &change->changes,
                                                       &change->affects);
      color_changed = _bobgui_bitmask_get (change->changes, BOBGUI_CSS_PROPERTY_COLOR);
    }

  if (change->old_style->background != change->new_style->background ||
      (color_changed && (bobgui_css_value_contains_current_color (change->old_style->background->background_color) ||
                         bobgui_css_value_contains_current_color (change->old_style->background->box_shadow) ||
                         bobgui_css_value_contains_current_color (change->old_style->background->background_image))))
    bobgui_css_background_values_compute_changes_and_affects (change->old_style,
                                                           change->new_style,
                                                           &change->changes,
                                                           &change->affects);

  if (change->old_style->border != change->new_style->border ||
      (color_changed && (bobgui_css_value_contains_current_color (change->old_style->border->border_top_color) ||
                         bobgui_css_value_contains_current_color (change->old_style->border->border_right_color) ||
                         bobgui_css_value_contains_current_color (change->old_style->border->border_bottom_color) ||
                         bobgui_css_value_contains_current_color (change->old_style->border->border_left_color) ||
                         bobgui_css_value_contains_current_color (change->old_style->border->border_image_source))))
    bobgui_css_border_values_compute_changes_and_affects (change->old_style,
                                                       change->new_style,
                                                       &change->changes,
                                                       &change->affects);

  if (change->old_style->icon != change->new_style->icon ||
      (color_changed && (bobgui_css_value_contains_current_color (change->old_style->icon->icon_shadow))))
    bobgui_css_icon_values_compute_changes_and_affects (change->old_style,
                                                     change->new_style,
                                                     &change->changes,
                                                     &change->affects);

  if (change->old_style->outline != change->new_style->outline ||
      (color_changed && bobgui_css_value_contains_current_color (change->old_style->outline->outline_color)))
    bobgui_css_outline_values_compute_changes_and_affects (change->old_style,
                                                        change->new_style,
                                                        &change->changes,
                                                        &change->affects);

  if (change->old_style->font != change->new_style->font ||
      (color_changed && (bobgui_css_value_contains_current_color (change->old_style->font->caret_color) ||
                         bobgui_css_value_contains_current_color (change->old_style->font->secondary_caret_color) ||
                         bobgui_css_value_contains_current_color (change->old_style->font->text_shadow))))
    bobgui_css_font_values_compute_changes_and_affects (change->old_style,
                                                     change->new_style,
                                                     &change->changes,
                                                     &change->affects);

  if (change->old_style->font_variant != change->new_style->font_variant ||
      (color_changed && bobgui_css_value_contains_current_color (change->old_style->text_decoration->text_decoration_color)))
    bobgui_css_font_variant_values_compute_changes_and_affects (change->old_style,
                                                             change->new_style,
                                                             &change->changes,
                                                             &change->affects);

  if (change->old_style->animation != change->new_style->animation)
    bobgui_css_animation_values_compute_changes_and_affects (change->old_style,
                                                          change->new_style,
                                                          &change->changes,
                                                          &change->affects);

  if (change->old_style->transition != change->new_style->transition)
    bobgui_css_transition_values_compute_changes_and_affects (change->old_style,
                                                           change->new_style,
                                                           &change->changes,
                                                           &change->affects);

  if (change->old_style->size != change->new_style->size)
    bobgui_css_size_values_compute_changes_and_affects (change->old_style,
                                                     change->new_style,
                                                     &change->changes,
                                                     &change->affects);

  if (change->old_style->other != change->new_style->other ||
      (color_changed && bobgui_css_value_contains_current_color (change->old_style->other->icon_source)))
    bobgui_css_other_values_compute_changes_and_affects (change->old_style,
                                                      change->new_style,
                                                      &change->changes,
                                                      &change->affects);

  if (change->old_style->variables != change->new_style->variables)
    bobgui_css_custom_values_compute_changes_and_affects (change->old_style,
                                                       change->new_style,
                                                       &change->changes,
                                                       &change->affects);
}

void
bobgui_css_style_change_init (BobguiCssStyleChange *change,
                           BobguiCssStyle       *old_style,
                           BobguiCssStyle       *new_style)
{
  change->old_style = g_object_ref (old_style);
  change->new_style = g_object_ref (new_style);

  change->affects = 0;
  change->changes = _bobgui_bitmask_new ();
  
  if (old_style != new_style)
    compute_change (change);
}

void
bobgui_css_style_change_finish (BobguiCssStyleChange *change)
{
  g_object_unref (change->old_style);
  g_object_unref (change->new_style);
  _bobgui_bitmask_free (change->changes);
}

BobguiCssStyle *
bobgui_css_style_change_get_old_style (BobguiCssStyleChange *change)
{
  return change->old_style;
}

BobguiCssStyle *
bobgui_css_style_change_get_new_style (BobguiCssStyleChange *change)
{
  return change->new_style;
}

gboolean
bobgui_css_style_change_has_change (BobguiCssStyleChange *change)
{
  return !_bobgui_bitmask_is_empty (change->changes);
}

gboolean
bobgui_css_style_change_affects (BobguiCssStyleChange *change,
                              BobguiCssAffects      affects)
{
  return (change->affects & affects) != 0;
}

gboolean
bobgui_css_style_change_changes_property (BobguiCssStyleChange *change,
                                       guint              id)
{
  return _bobgui_bitmask_get (change->changes, id);
}

void
bobgui_css_style_change_print (BobguiCssStyleChange *change,
                            GString           *string)
{
  int i;
  BobguiCssStyle *old = bobgui_css_style_change_get_old_style (change);
  BobguiCssStyle *new = bobgui_css_style_change_get_new_style (change);

  for (i = 0; i < BOBGUI_CSS_PROPERTY_N_PROPERTIES; i ++)
    {
      if (bobgui_css_style_change_changes_property (change, i))
        {
          BobguiCssStyleProperty *prop;
          BobguiCssValue *value;
          const char *name;

          prop = _bobgui_css_style_property_lookup_by_id (i);
          name = _bobgui_style_property_get_name (BOBGUI_STYLE_PROPERTY (prop));

          g_string_append_printf (string, "%s: ", name);
          value = bobgui_css_style_get_value (old, i);
          bobgui_css_value_print (value, string);
          g_string_append (string, "\n");

          g_string_append_printf (string, "%s: ", name);
          value = bobgui_css_style_get_value (new, i);
          bobgui_css_value_print (value, string);
          g_string_append (string, "\n");
        }
    }

}

char *
bobgui_css_style_change_to_string (BobguiCssStyleChange *change)
{
  GString *string = g_string_new ("");

  bobgui_css_style_change_print (change, string);

  return g_string_free (string, FALSE);
}
