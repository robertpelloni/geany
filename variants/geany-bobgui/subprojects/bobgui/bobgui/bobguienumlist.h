/*
 * Copyright (C) 2020 Purism SPC
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#if !defined(__BOBGUI_H_INSIDE__) && !defined(BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <glib-object.h>
#include <bobgui/bobguitypes.h>
#include <bobgui/bobguienums.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_ENUM_LIST_ITEM (bobgui_enum_list_item_get_type ())

GDK_AVAILABLE_IN_4_24
G_DECLARE_FINAL_TYPE (BobguiEnumListItem, bobgui_enum_list_item, BOBGUI, ENUM_LIST_ITEM, GObject)

GDK_AVAILABLE_IN_4_24
int             bobgui_enum_list_item_get_value    (BobguiEnumListItem *self);

GDK_AVAILABLE_IN_4_24
const char *    bobgui_enum_list_item_get_name     (BobguiEnumListItem *self);

GDK_AVAILABLE_IN_4_24
const char *    bobgui_enum_list_item_get_nick     (BobguiEnumListItem *self);

#define BOBGUI_TYPE_ENUM_LIST (bobgui_enum_list_get_type ())

GDK_AVAILABLE_IN_4_24
G_DECLARE_FINAL_TYPE (BobguiEnumList, bobgui_enum_list, BOBGUI, ENUM_LIST, GObject)

GDK_AVAILABLE_IN_4_24
BobguiEnumList *   bobgui_enum_list_new               (GType        enum_type) G_GNUC_WARN_UNUSED_RESULT;

GDK_AVAILABLE_IN_4_24
GType           bobgui_enum_list_get_enum_type     (BobguiEnumList *self);

GDK_AVAILABLE_IN_4_24
unsigned int    bobgui_enum_list_find              (BobguiEnumList *self,
                                                 int          value);

G_END_DECLS
