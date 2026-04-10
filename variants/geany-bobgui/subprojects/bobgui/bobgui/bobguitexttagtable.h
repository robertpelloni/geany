/* BOBGUI - The Bobgui Framework
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Modified by the BOBGUI Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the BOBGUI Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BOBGUI at ftp://ftp.bobgui.org/pub/bobgui/.
 */

#pragma once

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <bobgui/bobguitexttag.h>

G_BEGIN_DECLS

/**
 * BobguiTextTagTableForeach:
 * @tag: the `BobguiTextTag`
 * @data: (closure): data passed to bobgui_text_tag_table_foreach()
 *
 * A function used with bobgui_text_tag_table_foreach(),
 * to iterate over every `BobguiTextTag` inside a `BobguiTextTagTable`.
 */
typedef void (* BobguiTextTagTableForeach) (BobguiTextTag *tag, gpointer data);

#define BOBGUI_TYPE_TEXT_TAG_TABLE            (bobgui_text_tag_table_get_type ())
#define BOBGUI_TEXT_TAG_TABLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_TEXT_TAG_TABLE, BobguiTextTagTable))
#define BOBGUI_IS_TEXT_TAG_TABLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_TEXT_TAG_TABLE))

GDK_AVAILABLE_IN_ALL
GType          bobgui_text_tag_table_get_type (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
BobguiTextTagTable *bobgui_text_tag_table_new      (void);
GDK_AVAILABLE_IN_ALL
gboolean         bobgui_text_tag_table_add      (BobguiTextTagTable        *table,
                                              BobguiTextTag             *tag);
GDK_AVAILABLE_IN_ALL
void             bobgui_text_tag_table_remove   (BobguiTextTagTable        *table,
                                              BobguiTextTag             *tag);
GDK_AVAILABLE_IN_ALL
BobguiTextTag      *bobgui_text_tag_table_lookup   (BobguiTextTagTable        *table,
                                              const char             *name);
GDK_AVAILABLE_IN_ALL
void             bobgui_text_tag_table_foreach  (BobguiTextTagTable        *table,
                                              BobguiTextTagTableForeach  func,
                                              gpointer                data);
GDK_AVAILABLE_IN_ALL
int              bobgui_text_tag_table_get_size (BobguiTextTagTable        *table);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiTextTagTable, g_object_unref)

G_END_DECLS

