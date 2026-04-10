/*
 * Copyright (c) 2014 Red Hat, Inc.
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
#include <glib/gi18n-lib.h>

#include "magnifier.h"

#include "bobguimagnifierprivate.h"

#include "bobguilabel.h"
#include "bobguiadjustment.h"
#include "bobguistack.h"

enum
{
  PROP_0,
  PROP_ADJUSTMENT
};

struct _BobguiInspectorMagnifierPrivate
{
  BobguiWidget *object;
  BobguiWidget *magnifier;
  BobguiAdjustment *adjustment;
};

G_DEFINE_TYPE_WITH_PRIVATE (BobguiInspectorMagnifier, bobgui_inspector_magnifier, BOBGUI_TYPE_BOX)

static void
bobgui_inspector_magnifier_init (BobguiInspectorMagnifier *sl)
{
  sl->priv = bobgui_inspector_magnifier_get_instance_private (sl);
  bobgui_widget_init_template (BOBGUI_WIDGET (sl));
}

void
bobgui_inspector_magnifier_set_object (BobguiInspectorMagnifier *sl,
                                    GObject              *object)
{
  BobguiWidget *stack;
  BobguiStackPage *page;

  stack = bobgui_widget_get_parent (BOBGUI_WIDGET (sl));
  page = bobgui_stack_get_page (BOBGUI_STACK (stack), BOBGUI_WIDGET (sl));

  sl->priv->object = NULL;

  if (!BOBGUI_IS_WIDGET (object) || !bobgui_widget_is_visible (BOBGUI_WIDGET (object)))
    {
      g_object_set (page, "visible", FALSE, NULL);
      _bobgui_magnifier_set_inspected (BOBGUI_MAGNIFIER (sl->priv->magnifier), NULL);
      return;
    }

  g_object_set (page, "visible", TRUE, NULL);

  sl->priv->object = BOBGUI_WIDGET (object);

  _bobgui_magnifier_set_inspected (BOBGUI_MAGNIFIER (sl->priv->magnifier), BOBGUI_WIDGET (object));
  _bobgui_magnifier_set_coords (BOBGUI_MAGNIFIER (sl->priv->magnifier), 0, 0);
}

static void
get_property (GObject    *object,
              guint       param_id,
              GValue     *value,
              GParamSpec *pspec)
{
  BobguiInspectorMagnifier *sl = BOBGUI_INSPECTOR_MAGNIFIER (object);

  switch (param_id)
    {
    case PROP_ADJUSTMENT:
      g_value_take_object (value, sl->priv->adjustment);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

static void
set_property (GObject      *object,
              guint         param_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  BobguiInspectorMagnifier *sl = BOBGUI_INSPECTOR_MAGNIFIER (object);

  switch (param_id)
    {
    case PROP_ADJUSTMENT:
      sl->priv->adjustment = g_value_get_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

static void
constructed (GObject *object)
{
  BobguiInspectorMagnifier *sl = BOBGUI_INSPECTOR_MAGNIFIER (object);

  g_object_bind_property (sl->priv->adjustment, "value",
                          sl->priv->magnifier, "magnification",
                          G_BINDING_SYNC_CREATE);
}

static void
bobgui_inspector_magnifier_class_init (BobguiInspectorMagnifierClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);

  object_class->get_property = get_property;
  object_class->set_property = set_property;
  object_class->constructed = constructed;

  g_object_class_install_property (object_class, PROP_ADJUSTMENT,
      g_param_spec_object ("adjustment", NULL, NULL,
                           BOBGUI_TYPE_ADJUSTMENT, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

  bobgui_widget_class_set_template_from_resource (widget_class, "/org/bobgui/libbobgui/inspector/magnifier.ui");
  bobgui_widget_class_bind_template_child_private (widget_class, BobguiInspectorMagnifier, magnifier);
}

// vim: set et sw=2 ts=2:
