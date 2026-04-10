/* BOBGUI - The Bobgui Framework
 * bobguiprintoperationpreview.h: Abstract print preview interface
 * Copyright (C) 2006, Red Hat, Inc.
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

#pragma once

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <cairo.h>
#include <bobgui/print/bobguiprintcontext.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_PRINT_OPERATION_PREVIEW                  (bobgui_print_operation_preview_get_type ())
#define BOBGUI_PRINT_OPERATION_PREVIEW(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_PRINT_OPERATION_PREVIEW, BobguiPrintOperationPreview))
#define BOBGUI_IS_PRINT_OPERATION_PREVIEW(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_PRINT_OPERATION_PREVIEW))
#define BOBGUI_PRINT_OPERATION_PREVIEW_GET_IFACE(obj)        (G_TYPE_INSTANCE_GET_INTERFACE ((obj), BOBGUI_TYPE_PRINT_OPERATION_PREVIEW, BobguiPrintOperationPreviewIface))

typedef struct _BobguiPrintOperationPreview      BobguiPrintOperationPreview;      /*dummy typedef */
typedef struct _BobguiPrintOperationPreviewIface BobguiPrintOperationPreviewIface;


struct _BobguiPrintOperationPreviewIface
{
  GTypeInterface g_iface;

  /* signals */
  void              (*ready)          (BobguiPrintOperationPreview *preview,
				       BobguiPrintContext          *context);
  void              (*got_page_size)  (BobguiPrintOperationPreview *preview,
				       BobguiPrintContext          *context,
				       BobguiPageSetup             *page_setup);

  /* methods */
  void              (*render_page)    (BobguiPrintOperationPreview *preview,
				       int                       page_nr);
  gboolean          (*is_selected)    (BobguiPrintOperationPreview *preview,
				       int                       page_nr);
  void              (*end_preview)    (BobguiPrintOperationPreview *preview);

  /* Padding for future expansion */
  void (*_bobgui_reserved1) (void);
  void (*_bobgui_reserved2) (void);
  void (*_bobgui_reserved3) (void);
  void (*_bobgui_reserved4) (void);
  void (*_bobgui_reserved5) (void);
  void (*_bobgui_reserved6) (void);
  void (*_bobgui_reserved7) (void);
  void (*_bobgui_reserved8) (void);
};

GDK_AVAILABLE_IN_ALL
GType   bobgui_print_operation_preview_get_type       (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
void     bobgui_print_operation_preview_render_page (BobguiPrintOperationPreview *preview,
						  int                       page_nr);
GDK_AVAILABLE_IN_ALL
void     bobgui_print_operation_preview_end_preview (BobguiPrintOperationPreview *preview);
GDK_AVAILABLE_IN_ALL
gboolean bobgui_print_operation_preview_is_selected (BobguiPrintOperationPreview *preview,
						  int                       page_nr);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiPrintOperationPreview, g_object_unref)

G_END_DECLS

