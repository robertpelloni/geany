/* bobguicellrenderertext.h
 * Copyright (C) 2000  Red Hat, Inc.,  Jonathan Blandford <jrb@redhat.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once


#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <bobgui/deprecated/bobguicellrenderer.h>


G_BEGIN_DECLS


#define BOBGUI_TYPE_CELL_RENDERER_TEXT		(bobgui_cell_renderer_text_get_type ())
#define BOBGUI_CELL_RENDERER_TEXT(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_CELL_RENDERER_TEXT, BobguiCellRendererText))
#define BOBGUI_CELL_RENDERER_TEXT_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), BOBGUI_TYPE_CELL_RENDERER_TEXT, BobguiCellRendererTextClass))
#define BOBGUI_IS_CELL_RENDERER_TEXT(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_CELL_RENDERER_TEXT))
#define BOBGUI_IS_CELL_RENDERER_TEXT_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), BOBGUI_TYPE_CELL_RENDERER_TEXT))
#define BOBGUI_CELL_RENDERER_TEXT_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_CELL_RENDERER_TEXT, BobguiCellRendererTextClass))

typedef struct _BobguiCellRendererText              BobguiCellRendererText;
typedef struct _BobguiCellRendererTextClass         BobguiCellRendererTextClass;

struct _BobguiCellRendererText
{
  BobguiCellRenderer parent;
};

struct _BobguiCellRendererTextClass
{
  BobguiCellRendererClass parent_class;

  void (* edited) (BobguiCellRendererText *cell_renderer_text,
		   const char          *path,
		   const char          *new_text);

  /*< private >*/

  gpointer padding[8];
};

GDK_AVAILABLE_IN_ALL
GType            bobgui_cell_renderer_text_get_type (void) G_GNUC_CONST;
GDK_DEPRECATED_IN_4_10
BobguiCellRenderer *bobgui_cell_renderer_text_new      (void);

GDK_DEPRECATED_IN_4_10
void             bobgui_cell_renderer_text_set_fixed_height_from_font (BobguiCellRendererText *renderer,
								    int                  number_of_rows);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiCellRendererText, g_object_unref)

G_END_DECLS

