/*
 * Copyright (C) 2023 GNOME Foundation Inc.
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
 * Authors: Alice Mikhaylenko <alicem@gnome.org>
 */

#pragma once

#include <glib-object.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_CSS_CUSTOM_PROPERTY_POOL (bobgui_css_custom_property_pool_get_type())

G_DECLARE_FINAL_TYPE (BobguiCssCustomPropertyPool, bobgui_css_custom_property_pool, BOBGUI, CSS_CUSTOM_PROPERTY_POOL, GObject)

BobguiCssCustomPropertyPool *bobgui_css_custom_property_pool_get      (void);

int                       bobgui_css_custom_property_pool_add      (BobguiCssCustomPropertyPool *self,
                                                                 const char               *str);
int                       bobgui_css_custom_property_pool_lookup   (BobguiCssCustomPropertyPool *self,
                                                                 const char               *str);
const char *              bobgui_css_custom_property_pool_get_name (BobguiCssCustomPropertyPool *self,
                                                                 int                       id);
int                       bobgui_css_custom_property_pool_ref      (BobguiCssCustomPropertyPool *self,
                                                                 int                       id);
void                      bobgui_css_custom_property_pool_unref    (BobguiCssCustomPropertyPool *self,
                                                                 int                       id);

G_END_DECLS
