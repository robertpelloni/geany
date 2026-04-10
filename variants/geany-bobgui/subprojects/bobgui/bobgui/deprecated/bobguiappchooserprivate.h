/*
 * bobguiappchooserprivate.h: app-chooser interface
 *
 * Copyright (C) 2010 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Cosimo Cecchi <ccecchi@redhat.com>
 */

#pragma once

#include <glib.h>
#include <gio/gio.h>

#include "bobguiappchooser.h"
#include "bobguiappchooserwidget.h"
#include "bobguientry.h"

G_BEGIN_DECLS

typedef struct _BobguiAppChooserIface BobguiAppChooserIface;
typedef BobguiAppChooserIface BobguiAppChooserInterface;

#define BOBGUI_APP_CHOOSER_GET_IFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE ((inst), BOBGUI_TYPE_APP_CHOOSER, BobguiAppChooserIface))

struct _BobguiAppChooserIface {
  GTypeInterface base_iface;

  GAppInfo * (* get_app_info) (BobguiAppChooser *object);
  void       (* refresh)      (BobguiAppChooser *object);
};

void
_bobgui_app_chooser_widget_set_search_entry (BobguiAppChooserWidget *self,
                                          BobguiEditable         *editable);


G_END_DECLS

