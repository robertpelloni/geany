/*
 * Copyright © 2025 Red Hat, Inc
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
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * Authors: Matthias Clasen <mclasen@redhat.com>
 */

#include "transform-editor.h"
#include "bobgui/svg/bobguisvgvalueprivate.h"
#include "bobgui/svg/bobguisvgtransformprivate.h"

struct _TransformEditor
{
  BobguiWidget parent_instance;

  SvgValue *value;

  BobguiWidget *box;
  BobguiDropDown *primitive_transform;
  BobguiSpinButton *transform_angle;
  BobguiWidget *center_row;
  BobguiSpinButton *transform_x;
  BobguiSpinButton *transform_y;
  BobguiWidget *transform_params;
  BobguiSpinButton *transform_param0;
  BobguiSpinButton *transform_param1;
  BobguiSpinButton *transform_param2;
  BobguiSpinButton *transform_param3;
  BobguiSpinButton *transform_param4;
  BobguiSpinButton *transform_param5;
};

enum
{
  PROP_TRANSFORM = 1,
  NUM_PROPERTIES,
};

static GParamSpec *properties[NUM_PROPERTIES];

/* {{{ Callbacks */

static GdkPaintable *
transform_get_icon (GObject      *object,
                    unsigned int  position)
{
  g_autofree char *path = NULL;
  const char *names[] = {
    "identity-symbolic",
    "translate-symbolic",
    "scale-symbolic",
    "rotate-symbolic",
    "shear-x-symbolic",
    "shear-y-symbolic",
    "transform-symbolic",
  };

  if (position == BOBGUI_INVALID_LIST_POSITION)
    return NULL;

  path = g_strdup_printf ("/org/bobgui/Shaper/%s.svg", names[position]);

  return g_object_new (BOBGUI_TYPE_SVG,
                       "resource", path,
                       "playing", 1,
                       NULL);
}

static BobguiText *
get_text (BobguiSpinButton *spin)
{
  for (BobguiWidget *child = bobgui_widget_get_first_child (BOBGUI_WIDGET (spin));
       child != NULL;
       child = bobgui_widget_get_next_sibling (child))
    {
      if (BOBGUI_IS_TEXT (child))
        return BOBGUI_TEXT (child);
    }

  return NULL;
}

static void
transform_type_changed (TransformEditor *self)
{
  bobgui_widget_set_visible (BOBGUI_WIDGET (self->center_row), FALSE);
  bobgui_widget_set_visible (BOBGUI_WIDGET (self->transform_angle), FALSE);
  bobgui_widget_set_visible (BOBGUI_WIDGET (self->transform_params), FALSE);
  bobgui_spin_button_set_value (self->transform_x, 0);
  bobgui_spin_button_set_value (self->transform_y, 0);
  bobgui_spin_button_set_value (self->transform_angle, 0);
  bobgui_spin_button_set_value (self->transform_param0, 0);
  bobgui_spin_button_set_value (self->transform_param1, 1);
  bobgui_spin_button_set_value (self->transform_param2, 1);
  bobgui_spin_button_set_value (self->transform_param3, 0);
  bobgui_spin_button_set_value (self->transform_param4, 0);
  bobgui_spin_button_set_value (self->transform_param5, 0);

  switch (bobgui_drop_down_get_selected (self->primitive_transform))
    {
    case TRANSFORM_NONE:
      break;
    case TRANSFORM_SKEW_X:
    case TRANSFORM_SKEW_Y:
      bobgui_widget_set_visible (BOBGUI_WIDGET (self->transform_angle), TRUE);
      bobgui_text_set_placeholder_text (get_text (self->transform_angle), "Angle…");
      bobgui_widget_set_tooltip_text (BOBGUI_WIDGET (self->transform_angle), "Angle");
      break;
    case TRANSFORM_ROTATE:
      bobgui_widget_set_visible (BOBGUI_WIDGET (self->transform_angle), TRUE);
      bobgui_widget_set_visible (BOBGUI_WIDGET (self->center_row), TRUE);
      bobgui_text_set_placeholder_text (get_text (self->transform_angle), "Angle…");
      bobgui_text_set_placeholder_text (get_text (self->transform_x), "Center X…");
      bobgui_text_set_placeholder_text (get_text (self->transform_y), "Center Y‥");
      bobgui_widget_set_tooltip_text (BOBGUI_WIDGET (self->transform_angle), "Angle");
      bobgui_widget_set_tooltip_text (BOBGUI_WIDGET (self->transform_x), "Center X");
      bobgui_widget_set_tooltip_text (BOBGUI_WIDGET (self->transform_y), "Center X");
      break;
    case TRANSFORM_TRANSLATE:
      bobgui_widget_set_visible (BOBGUI_WIDGET (self->center_row), TRUE);
      bobgui_text_set_placeholder_text (get_text (self->transform_x), "X Shift…");
      bobgui_text_set_placeholder_text (get_text (self->transform_y), "Y Shift‥");
      bobgui_widget_set_tooltip_text (BOBGUI_WIDGET (self->transform_x), "X Shift");
      bobgui_widget_set_tooltip_text (BOBGUI_WIDGET (self->transform_y), "Y Shift");
      break;
    case TRANSFORM_SCALE:
      bobgui_spin_button_set_value (self->transform_x, 1);
      bobgui_spin_button_set_value (self->transform_y, 1);
      bobgui_widget_set_visible (BOBGUI_WIDGET (self->center_row), TRUE);
      bobgui_text_set_placeholder_text (get_text (self->transform_x), "X Scale‥");
      bobgui_text_set_placeholder_text (get_text (self->transform_y), "Y Scale‥");
      bobgui_widget_set_tooltip_text (BOBGUI_WIDGET (self->transform_x), "X Scale");
      bobgui_widget_set_tooltip_text (BOBGUI_WIDGET (self->transform_y), "Y Scale");
      break;
    case TRANSFORM_MATRIX:
      bobgui_widget_set_visible (BOBGUI_WIDGET (self->transform_params), TRUE);
      break;
    default:
      g_assert_not_reached ();
    }
}

static void
transform_changed (TransformEditor *self)
{
  SvgValue *value;
  double angle, x, y;
  double params[6];

  angle = bobgui_spin_button_get_value (self->transform_angle);
  x = bobgui_spin_button_get_value (self->transform_x);
  y = bobgui_spin_button_get_value (self->transform_y);
  params[0] = bobgui_spin_button_get_value (self->transform_param0);
  params[1] = bobgui_spin_button_get_value (self->transform_param1);
  params[2] = bobgui_spin_button_get_value (self->transform_param2);
  params[3] = bobgui_spin_button_get_value (self->transform_param3);
  params[4] = bobgui_spin_button_get_value (self->transform_param4);
  params[5] = bobgui_spin_button_get_value (self->transform_param5);

  switch (bobgui_drop_down_get_selected (self->primitive_transform))
    {
    case TRANSFORM_NONE:
      value = svg_transform_new_none ();
      break;
    case TRANSFORM_TRANSLATE:
      value = svg_transform_new_translate (x, y);
      break;
    case TRANSFORM_ROTATE:
      value = svg_transform_new_rotate (angle, x, y);
      break;
    case TRANSFORM_SCALE:
      value = svg_transform_new_scale (x, y);
      break;
    case TRANSFORM_SKEW_X:
      value = svg_transform_new_skew_x (angle);
      break;
    case TRANSFORM_SKEW_Y:
      value = svg_transform_new_skew_y (angle);
      break;
    case TRANSFORM_MATRIX:
      value = svg_transform_new_matrix (params);
      break;
    default:
      g_assert_not_reached ();
    }

  transform_editor_set_transform (self, value);

  svg_value_unref (value);
}

/* }}} */
/* {{{ GObject boilerplate */

struct _TransformEditorClass
{
  BobguiWidgetClass parent_class;
};

G_DEFINE_TYPE (TransformEditor, transform_editor, BOBGUI_TYPE_WIDGET)

static void
unbutton_spin (BobguiSpinButton *spin)
{
  for (BobguiWidget *child = bobgui_widget_get_first_child (BOBGUI_WIDGET (spin));
       child != NULL;
       child = bobgui_widget_get_next_sibling (child))
    {
      if (BOBGUI_IS_BUTTON (child))
        bobgui_widget_set_visible (child, FALSE);
    }
}

static void
transform_editor_init (TransformEditor *self)
{
  self->value = svg_transform_parse ("none");

  bobgui_widget_init_template (BOBGUI_WIDGET (self));

  /* We want numeric entries, but there's no space
   * for buttons, so...
   */
  unbutton_spin (self->transform_x);
  unbutton_spin (self->transform_y);
  unbutton_spin (self->transform_angle);
  unbutton_spin (self->transform_param0);
  unbutton_spin (self->transform_param1);
  unbutton_spin (self->transform_param2);
  unbutton_spin (self->transform_param3);
  unbutton_spin (self->transform_param4);
  unbutton_spin (self->transform_param5);

  for (BobguiWidget *child = bobgui_widget_get_first_child (BOBGUI_WIDGET (self->primitive_transform));
       child != NULL;
       child = bobgui_widget_get_next_sibling (child))
    {
      if (BOBGUI_IS_TOGGLE_BUTTON (child))
        {
          for (BobguiWidget *grand_child = bobgui_widget_get_first_child (child);
               grand_child != NULL;
               grand_child = bobgui_widget_get_next_sibling (grand_child))
            {
              if (BOBGUI_IS_BOX (grand_child))
                {
                  for (BobguiWidget *gg_child = bobgui_widget_get_first_child (grand_child);
                       gg_child != NULL;
                       gg_child = bobgui_widget_get_next_sibling (gg_child))
                    {
                      if (strcmp (G_OBJECT_TYPE_NAME (gg_child), "BobguiBuiltinIcon") == 0)
                        {
                          bobgui_widget_set_visible (gg_child, FALSE);
                          break;
                        }
                    }
                  break;
                }
            }
          break;
        }
    }

  transform_type_changed (self);
}

static void
transform_editor_set_property (GObject      *object,
                               unsigned int  prop_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  TransformEditor *self = TRANSFORM_EDITOR (object);

  switch (prop_id)
    {
    case PROP_TRANSFORM:
      transform_editor_set_transform (self, g_value_get_boxed (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
transform_editor_get_property (GObject      *object,
                               unsigned int  prop_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  TransformEditor *self = TRANSFORM_EDITOR (object);

  switch (prop_id)
    {
    case PROP_TRANSFORM:
      g_value_set_boxed (value, self->value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
transform_editor_dispose (GObject *object)
{
  //TransformEditor *self = TRANSFORM_EDITOR (object);

  bobgui_widget_dispose_template (BOBGUI_WIDGET (object), transform_editor_get_type ());

  G_OBJECT_CLASS (transform_editor_parent_class)->dispose (object);
}

static void
transform_editor_finalize (GObject *object)
{
  TransformEditor *self = TRANSFORM_EDITOR (object);

  svg_value_unref (self->value);

  G_OBJECT_CLASS (transform_editor_parent_class)->finalize (object);
}

static void
transform_editor_class_init (TransformEditorClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (class);

  object_class->set_property = transform_editor_set_property;
  object_class->get_property = transform_editor_get_property;
  object_class->dispose = transform_editor_dispose;
  object_class->finalize = transform_editor_finalize;

  properties[PROP_TRANSFORM] =
    g_param_spec_boxed ("transform", NULL, NULL,
                        svg_value_get_type (),
                        G_PARAM_READWRITE | G_PARAM_STATIC_NAME);

  g_object_class_install_properties (object_class, NUM_PROPERTIES, properties);

  bobgui_widget_class_set_template_from_resource (widget_class, "/org/bobgui/Shaper/transform-editor.ui");

  bobgui_widget_class_bind_template_child (widget_class, TransformEditor, box);
  bobgui_widget_class_bind_template_child (widget_class, TransformEditor, primitive_transform);
  bobgui_widget_class_bind_template_child (widget_class, TransformEditor, center_row);
  bobgui_widget_class_bind_template_child (widget_class, TransformEditor, transform_x);
  bobgui_widget_class_bind_template_child (widget_class, TransformEditor, transform_y);
  bobgui_widget_class_bind_template_child (widget_class, TransformEditor, transform_angle);
  bobgui_widget_class_bind_template_child (widget_class, TransformEditor, transform_params);
  bobgui_widget_class_bind_template_child (widget_class, TransformEditor, transform_param0);
  bobgui_widget_class_bind_template_child (widget_class, TransformEditor, transform_param1);
  bobgui_widget_class_bind_template_child (widget_class, TransformEditor, transform_param2);
  bobgui_widget_class_bind_template_child (widget_class, TransformEditor, transform_param3);
  bobgui_widget_class_bind_template_child (widget_class, TransformEditor, transform_param4);
  bobgui_widget_class_bind_template_child (widget_class, TransformEditor, transform_param5);

  bobgui_widget_class_bind_template_callback (widget_class, transform_changed);
  bobgui_widget_class_bind_template_callback (widget_class, transform_type_changed);
  bobgui_widget_class_bind_template_callback (widget_class, transform_get_icon);

  bobgui_widget_class_set_css_name (widget_class, "TransformEditor");
  bobgui_widget_class_set_layout_manager_type (widget_class, BOBGUI_TYPE_BIN_LAYOUT);
}

/*  }}} */
/* {{{ Public API */

TransformEditor *
transform_editor_new (void)
{
  return g_object_new (transform_editor_get_type (), NULL);
}

void
transform_editor_set_transform (TransformEditor *self,
                                SvgValue        *value)
{
  TransformType type;
  double params[6] = { 0, };

  g_return_if_fail (TRANSFORM_IS_EDITOR (self));

  if (svg_value_equal (self->value, value))
    return;

  svg_value_unref (self->value);
  self->value = svg_value_ref (value);

  type = svg_transform_get_primitive (value, 0, params);

  bobgui_drop_down_set_selected (self->primitive_transform, type);
  switch (type)
    {
    case TRANSFORM_NONE:
      break;
    case TRANSFORM_TRANSLATE:
    case TRANSFORM_SCALE:
      bobgui_spin_button_set_value (self->transform_x, params[0]);
      bobgui_spin_button_set_value (self->transform_y, params[1]);
      break;
    case TRANSFORM_ROTATE:
      bobgui_spin_button_set_value (self->transform_angle, params[0]);
      bobgui_spin_button_set_value (self->transform_x, params[1]);
      bobgui_spin_button_set_value (self->transform_y, params[2]);
      break;
    case TRANSFORM_SKEW_X:
    case TRANSFORM_SKEW_Y:
      bobgui_spin_button_set_value (self->transform_angle, params[0]);
      break;
    case TRANSFORM_MATRIX:
      bobgui_spin_button_set_value (self->transform_param0, params[0]);
      bobgui_spin_button_set_value (self->transform_param1, params[1]);
      bobgui_spin_button_set_value (self->transform_param2, params[2]);
      bobgui_spin_button_set_value (self->transform_param3, params[3]);
      bobgui_spin_button_set_value (self->transform_param4, params[4]);
      bobgui_spin_button_set_value (self->transform_param5, params[5]);
      break;
    default:
      g_assert_not_reached ();
    }

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_TRANSFORM]);
}

SvgValue *
transform_editor_get_transform (TransformEditor *self)
{
  g_return_val_if_fail (TRANSFORM_IS_EDITOR (self), NULL);

  return self->value;
}

/* }}} */

/* vim:set foldmethod=marker: */
