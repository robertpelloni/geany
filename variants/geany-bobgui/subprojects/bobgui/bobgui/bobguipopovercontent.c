/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2021 Red Hat, Inc.
 *
 * Authors:
 * - Matthias Clasen <mclasen@redhat.com>
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

#include "bobguipopovercontentprivate.h"

#include "bobguicssstylechangeprivate.h"
#include "bobguiwidgetprivate.h"
#include "bobguiprivate.h"

/* A private class used as the child of BobguiPopover. The only thing
 * special here is that we need to queue a resize on the popover when
 * our shadow changes.
 */

G_DEFINE_TYPE (BobguiPopoverContent, bobgui_popover_content, BOBGUI_TYPE_WIDGET);

static void
bobgui_popover_content_css_changed (BobguiWidget         *widget,
                                 BobguiCssStyleChange *change)
{
  BOBGUI_WIDGET_CLASS (bobgui_popover_content_parent_class)->css_changed (widget, change);

  if (change == NULL ||
      bobgui_css_style_change_changes_property (change, BOBGUI_CSS_PROPERTY_BOX_SHADOW))
    bobgui_widget_queue_resize (bobgui_widget_get_parent (widget));
}

static void
bobgui_popover_content_finalize (GObject *object)
{
  BobguiPopoverContent *self = BOBGUI_POPOVER_CONTENT (object);
  BobguiWidget *widget;

  widget = _bobgui_widget_get_first_child (BOBGUI_WIDGET (self));
  while (widget != NULL)
    {
      BobguiWidget *next = _bobgui_widget_get_next_sibling (widget);

      bobgui_widget_unparent (widget);

      widget = next;
    }

  G_OBJECT_CLASS (bobgui_popover_content_parent_class)->finalize (object);
}

static void
bobgui_popover_content_class_init (BobguiPopoverContentClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);

  object_class->finalize = bobgui_popover_content_finalize;

  widget_class->focus = bobgui_widget_focus_child;
  widget_class->grab_focus = bobgui_widget_grab_focus_child;
  widget_class->css_changed = bobgui_popover_content_css_changed;

  bobgui_widget_class_set_css_name (widget_class, I_("contents"));
  bobgui_widget_class_set_accessible_role (widget_class, BOBGUI_ACCESSIBLE_ROLE_GENERIC);
}

static void
bobgui_popover_content_init (BobguiPopoverContent *self)
{
}

BobguiWidget *
bobgui_popover_content_new (void)
{
  return g_object_new (BOBGUI_TYPE_POPOVER_CONTENT, NULL);
}
