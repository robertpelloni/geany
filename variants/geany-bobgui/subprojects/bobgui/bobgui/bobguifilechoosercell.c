/*
 * Copyright © 2022 Red Hat, Inc.
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

#include "config.h"

#include "bobguifilechoosercellprivate.h"

#include "bobguiprivate.h"
#include "bobguibinlayout.h"
#include "bobguidragsource.h"
#include "bobguigestureclick.h"
#include "bobguigesturelongpress.h"
#include "bobguiicontheme.h"
#include "bobguilistitem.h"
#include "bobguiselectionmodel.h"
#include "bobguifilechooserutils.h"
#include "bobguifilechooserwidgetprivate.h"

struct _BobguiFileChooserCell
{
  BobguiWidget parent_instance;

  GFileInfo *item;
  BobguiListItem *list_item;
};

struct _BobguiFileChooserCellClass
{
  BobguiWidgetClass parent_class;
};

G_DEFINE_TYPE (BobguiFileChooserCell, bobgui_file_chooser_cell, BOBGUI_TYPE_WIDGET)

enum
{
  PROP_POSITION = 1,
  PROP_ITEM,
  PROP_LIST_ITEM,
};

#define ICON_SIZE 16

static void
popup_menu (BobguiFileChooserCell *self,
            double              x,
            double              y)
{
  BobguiWidget *widget = BOBGUI_WIDGET (self);
  BobguiWidget *impl;
  graphene_point_t p;

  bobgui_widget_activate_action (BOBGUI_WIDGET (self), "listitem.select", "(bb)", FALSE, FALSE);

  impl = bobgui_widget_get_ancestor (widget, BOBGUI_TYPE_FILE_CHOOSER_WIDGET);

  if (!bobgui_widget_compute_point (widget, BOBGUI_WIDGET (impl),
                                 &GRAPHENE_POINT_INIT (x, y), &p))
    return;

  if (self->list_item)
    bobgui_widget_activate_action (widget, "item.popup-file-list-menu",
                                "(udd)", bobgui_list_item_get_position (self->list_item), p.x, p.y);
}

static void
file_chooser_cell_clicked (BobguiEventController *controller,
                           int                 n_press,
                           double              x,
                           double              y)
{
  BobguiWidget *widget = bobgui_event_controller_get_widget (controller);
  BobguiFileChooserCell *self = BOBGUI_FILE_CHOOSER_CELL (widget);

  bobgui_gesture_set_state (BOBGUI_GESTURE (controller), BOBGUI_EVENT_SEQUENCE_CLAIMED);
  popup_menu (self, x, y);
}

static void
file_chooser_cell_long_pressed (BobguiEventController *controller,
                                double              x,
                                double              y)
{
  BobguiWidget *widget = bobgui_event_controller_get_widget (controller);
  BobguiFileChooserCell *self = BOBGUI_FILE_CHOOSER_CELL (widget);

  bobgui_gesture_set_state (BOBGUI_GESTURE (controller), BOBGUI_EVENT_SEQUENCE_CLAIMED);
  popup_menu (self, x, y);
}

static GdkContentProvider *
drag_prepare_cb (BobguiDragSource *source,
                 double         x,
                 double         y,
                 gpointer       user_data)
{
  GdkContentProvider *provider;
  GSList *selection;
  BobguiFileChooserWidget *impl;
  BobguiIconTheme *icon_theme;
  GIcon *icon;
  int scale;
  BobguiIconPaintable *paintable;
  BobguiFileChooserCell *self = user_data;

  impl = BOBGUI_FILE_CHOOSER_WIDGET (bobgui_widget_get_ancestor (BOBGUI_WIDGET (self),
                                                           BOBGUI_TYPE_FILE_CHOOSER_WIDGET));

  if (self->list_item && !bobgui_list_item_get_selected (self->list_item))
    {
      bobgui_widget_activate_action (BOBGUI_WIDGET (self), "listitem.select", "(bb)", FALSE, FALSE);
    }

  selection = bobgui_file_chooser_widget_get_selected_files (impl);
  if (!selection)
    return NULL;

  scale = bobgui_widget_get_scale_factor (BOBGUI_WIDGET (self));
  icon_theme = bobgui_icon_theme_get_for_display (bobgui_widget_get_display (BOBGUI_WIDGET (self)));

  icon = _bobgui_file_info_get_icon (self->item, ICON_SIZE, scale, icon_theme);

  paintable = bobgui_icon_theme_lookup_by_gicon (icon_theme,icon, ICON_SIZE, scale, BOBGUI_TEXT_DIR_NONE, 0);

  bobgui_drag_source_set_icon (source, GDK_PAINTABLE (paintable), x, y);

  provider = gdk_content_provider_new_typed (GDK_TYPE_FILE_LIST, selection);
  g_slist_free_full (selection, g_object_unref);
  g_object_unref (paintable);
  g_object_unref (icon);

  return provider;
}

static void
bobgui_file_chooser_cell_init (BobguiFileChooserCell *self)
{
  BobguiGesture *gesture;
  BobguiDragSource *drag_source;

  gesture = bobgui_gesture_click_new ();
  bobgui_gesture_single_set_button (BOBGUI_GESTURE_SINGLE (gesture), GDK_BUTTON_SECONDARY);
  g_signal_connect (gesture, "pressed", G_CALLBACK (file_chooser_cell_clicked), NULL);
  bobgui_widget_add_controller (BOBGUI_WIDGET (self), BOBGUI_EVENT_CONTROLLER (gesture));

  gesture = bobgui_gesture_long_press_new ();
  bobgui_gesture_single_set_touch_only (BOBGUI_GESTURE_SINGLE (gesture), TRUE);
  g_signal_connect (gesture, "pressed", G_CALLBACK (file_chooser_cell_long_pressed), NULL);
  bobgui_widget_add_controller (BOBGUI_WIDGET (self), BOBGUI_EVENT_CONTROLLER (gesture));

  drag_source = bobgui_drag_source_new ();
  bobgui_widget_add_controller (BOBGUI_WIDGET (self), BOBGUI_EVENT_CONTROLLER (drag_source));
  g_signal_connect (drag_source, "prepare", G_CALLBACK (drag_prepare_cb), self);
}

static void
bobgui_file_chooser_cell_dispose (GObject *object)
{
  BobguiWidget *child;

  while ((child = bobgui_widget_get_first_child (BOBGUI_WIDGET (object))))
    bobgui_widget_unparent (child);

  G_OBJECT_CLASS (bobgui_file_chooser_cell_parent_class)->dispose (object);
}

static gboolean
get_selectable (BobguiFileChooserCell *self)
{
  if (self->item)
    return g_file_info_get_attribute_boolean (self->item, "filechooser::selectable");

  return TRUE;
}

static void
bobgui_file_chooser_cell_set_property (GObject      *object,
                                    guint         prop_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  BobguiFileChooserCell *self = BOBGUI_FILE_CHOOSER_CELL (object);

  switch (prop_id)
    {
    case PROP_ITEM:
      self->item = g_value_get_object (value);

      if (get_selectable (self))
        bobgui_widget_remove_css_class (BOBGUI_WIDGET (self), "dim-label");
      else
        bobgui_widget_add_css_class (BOBGUI_WIDGET (self), "dim-label");

      break;

    case PROP_LIST_ITEM:
      self->list_item = g_value_get_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_file_chooser_cell_get_property (GObject    *object,
                                    guint       prop_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  BobguiFileChooserCell *self = BOBGUI_FILE_CHOOSER_CELL (object);

  switch (prop_id)
    {
    case PROP_ITEM:
      g_value_set_object (value, self->item);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}


static void
bobgui_file_chooser_cell_class_init (BobguiFileChooserCellClass *klass)
{
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = bobgui_file_chooser_cell_dispose;
  object_class->set_property = bobgui_file_chooser_cell_set_property;
  object_class->get_property = bobgui_file_chooser_cell_get_property;

  g_object_class_install_property (object_class, PROP_ITEM,
                                   g_param_spec_object ("item", NULL, NULL,
                                                        G_TYPE_FILE_INFO,
                                                        BOBGUI_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_LIST_ITEM,
                                   g_param_spec_object ("list-item", NULL, NULL,
                                                        BOBGUI_TYPE_LIST_ITEM,
                                                        BOBGUI_PARAM_WRITABLE));

  bobgui_widget_class_set_css_name (widget_class, I_("filelistcell"));
  bobgui_widget_class_set_layout_manager_type (widget_class, BOBGUI_TYPE_BIN_LAYOUT);
}

BobguiFileChooserCell *
bobgui_file_chooser_cell_new (void)
{
  return g_object_new (BOBGUI_TYPE_FILE_CHOOSER_CELL, NULL);
}
