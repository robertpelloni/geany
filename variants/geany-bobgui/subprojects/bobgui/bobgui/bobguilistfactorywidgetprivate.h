/*
 * Copyright © 2023 Benjamin Otte
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

#include "bobguilistitembaseprivate.h"

#include "bobguilistitemfactory.h"

G_BEGIN_DECLS

#define BOBGUI_TYPE_LIST_FACTORY_WIDGET         (bobgui_list_factory_widget_get_type ())
#define BOBGUI_LIST_FACTORY_WIDGET(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), BOBGUI_TYPE_LIST_FACTORY_WIDGET, BobguiListFactoryWidget))
#define BOBGUI_LIST_FACTORY_WIDGET_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), BOBGUI_TYPE_LIST_FACTORY_WIDGET, BobguiListFactoryWidgetClass))
#define BOBGUI_IS_LIST_FACTORY_WIDGET(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), BOBGUI_TYPE_LIST_FACTORY_WIDGET))
#define BOBGUI_IS_LIST_FACTORY_WIDGET_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), BOBGUI_TYPE_LIST_FACTORY_WIDGET))
#define BOBGUI_LIST_FACTORY_WIDGET_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), BOBGUI_TYPE_LIST_FACTORY_WIDGET, BobguiListFactoryWidgetClass))

typedef struct _BobguiListFactoryWidget BobguiListFactoryWidget;
typedef struct _BobguiListFactoryWidgetClass BobguiListFactoryWidgetClass;

struct _BobguiListFactoryWidget
{
  BobguiListItemBase parent_instance;
};

struct _BobguiListFactoryWidgetClass
{
  BobguiListItemBaseClass parent_class;

  void          (* activate_signal)                             (BobguiListFactoryWidget         *self);

  gpointer      (* create_object)                               (BobguiListFactoryWidget         *self);
  void          (* setup_object)                                (BobguiListFactoryWidget         *self,
                                                                 gpointer                      object);
  void          (* update_object)                               (BobguiListFactoryWidget         *self,
                                                                 gpointer                      object,
                                                                 guint                         position,
                                                                 gpointer                      item,
                                                                 gboolean                      selected);
  void          (* teardown_object)                             (BobguiListFactoryWidget         *self,
                                                                 gpointer                      object);
};

GType                   bobgui_list_factory_widget_get_type        (void) G_GNUC_CONST;

gpointer                bobgui_list_factory_widget_get_object      (BobguiListFactoryWidget   *self);

void                    bobgui_list_factory_widget_set_factory     (BobguiListFactoryWidget   *self,
                                                                 BobguiListItemFactory     *factory);
BobguiListItemFactory *    bobgui_list_factory_widget_get_factory     (BobguiListFactoryWidget   *self);

void                    bobgui_list_factory_widget_set_single_click_activate
                                                                (BobguiListFactoryWidget   *self,
                                                                 gboolean                single_click_activate);
gboolean                bobgui_list_factory_widget_get_single_click_activate
                                                                (BobguiListFactoryWidget   *self);

void                    bobgui_list_factory_widget_set_activatable (BobguiListFactoryWidget   *self,
                                                                 gboolean                activatable);
gboolean                bobgui_list_factory_widget_get_activatable (BobguiListFactoryWidget   *self);

void                    bobgui_list_factory_widget_set_selectable  (BobguiListFactoryWidget   *self,
                                                                 gboolean                activatable);
gboolean                bobgui_list_factory_widget_get_selectable  (BobguiListFactoryWidget   *self);

G_END_DECLS

