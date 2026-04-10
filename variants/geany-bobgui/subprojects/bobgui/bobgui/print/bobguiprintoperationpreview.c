/* BOBGUI - The Bobgui Framework
 * bobguiprintoperationpreview.c: Abstract print preview interface
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

#include "config.h"

#include <bobgui/bobgui.h>

#include "bobguimarshalers.h"

#include "bobguiprintoperationpreview.h"


/**
 * BobguiPrintOperationPreview:
 *
 * The interface that is used to implement print preview.
 *
 * A `BobguiPrintOperationPreview` object is passed to the
 * [signal@Bobgui.PrintOperation::preview] signal by
 * [class@Bobgui.PrintOperation].
 */

static void bobgui_print_operation_preview_base_init (gpointer g_iface);

GType
bobgui_print_operation_preview_get_type (void)
{
  static GType print_operation_preview_type = 0;

  if (!print_operation_preview_type)
    {
      const GTypeInfo print_operation_preview_info =
      {
        sizeof (BobguiPrintOperationPreviewIface), /* class_size */
	bobgui_print_operation_preview_base_init,   /* base_init */
	NULL,		/* base_finalize */
	NULL,
	NULL,		/* class_finalize */
	NULL,		/* class_data */
	0,
	0,              /* n_preallocs */
	NULL
      };

      print_operation_preview_type =
	g_type_register_static (G_TYPE_INTERFACE, "BobguiPrintOperationPreview",
				&print_operation_preview_info, 0);

      g_type_interface_add_prerequisite (print_operation_preview_type, G_TYPE_OBJECT);
    }

  return print_operation_preview_type;
}

static void
bobgui_print_operation_preview_base_init (gpointer g_iface)
{
  static gboolean initialized = FALSE;

  if (!initialized)
    {
      unsigned int id;

      /**
       * BobguiPrintOperationPreview::ready:
       * @preview: the object on which the signal is emitted
       * @context: the current `BobguiPrintContext`
       *
       * The ::ready signal gets emitted once per preview operation,
       * before the first page is rendered.
       * 
       * A handler for this signal can be used for setup tasks.
       */
      g_signal_new ("ready",
		    BOBGUI_TYPE_PRINT_OPERATION_PREVIEW,
		    G_SIGNAL_RUN_LAST,
		    G_STRUCT_OFFSET (BobguiPrintOperationPreviewIface, ready),
		    NULL, NULL,
		    NULL,
		    G_TYPE_NONE, 1,
		    BOBGUI_TYPE_PRINT_CONTEXT);

      /**
       * BobguiPrintOperationPreview::got-page-size:
       * @preview: the object on which the signal is emitted
       * @context: the current `BobguiPrintContext`
       * @page_setup: the `BobguiPageSetup` for the current page
       *
       * Emitted once for each page that gets rendered to the preview.
       *
       * A handler for this signal should update the @context
       * according to @page_setup and set up a suitable cairo
       * context, using [method@Bobgui.PrintContext.set_cairo_context].
       */
      id = g_signal_new ("got-page-size",
                         BOBGUI_TYPE_PRINT_OPERATION_PREVIEW,
                         G_SIGNAL_RUN_LAST,
                         G_STRUCT_OFFSET (BobguiPrintOperationPreviewIface, got_page_size),
                         NULL, NULL,
                         _bobgui_marshal_VOID__OBJECT_OBJECT,
                         G_TYPE_NONE, 2,
                         BOBGUI_TYPE_PRINT_CONTEXT,
                         BOBGUI_TYPE_PAGE_SETUP);
      g_signal_set_va_marshaller (id,
                                  BOBGUI_TYPE_PRINT_OPERATION_PREVIEW,
                                  _bobgui_marshal_VOID__OBJECT_OBJECTv);

      initialized = TRUE;
    }
}

/**
 * bobgui_print_operation_preview_render_page:
 * @preview: a `BobguiPrintOperationPreview`
 * @page_nr: the page to render
 *
 * Renders a page to the preview.
 *
 * This is using the print context that was passed to the
 * [signal@Bobgui.PrintOperation::preview] handler together
 * with @preview.
 *
 * A custom print preview should use this function to render
 * the currently selected page.
 *
 * Note that this function requires a suitable cairo context to
 * be associated with the print context.
 */
void    
bobgui_print_operation_preview_render_page (BobguiPrintOperationPreview *preview,
					 int			   page_nr)
{
  g_return_if_fail (BOBGUI_IS_PRINT_OPERATION_PREVIEW (preview));

  BOBGUI_PRINT_OPERATION_PREVIEW_GET_IFACE (preview)->render_page (preview,
								page_nr);
}

/**
 * bobgui_print_operation_preview_end_preview:
 * @preview: a `BobguiPrintOperationPreview`
 *
 * Ends a preview.
 *
 * This function must be called to finish a custom print preview.
 */
void
bobgui_print_operation_preview_end_preview (BobguiPrintOperationPreview *preview)
{
  g_return_if_fail (BOBGUI_IS_PRINT_OPERATION_PREVIEW (preview));

  BOBGUI_PRINT_OPERATION_PREVIEW_GET_IFACE (preview)->end_preview (preview);
}

/**
 * bobgui_print_operation_preview_is_selected:
 * @preview: a `BobguiPrintOperationPreview`
 * @page_nr: a page number
 *
 * Returns whether the given page is included in the set of pages that
 * have been selected for printing.
 *
 * Returns: %TRUE if the page has been selected for printing
 */
gboolean
bobgui_print_operation_preview_is_selected (BobguiPrintOperationPreview *preview,
					 int                       page_nr)
{
  g_return_val_if_fail (BOBGUI_IS_PRINT_OPERATION_PREVIEW (preview), FALSE);

  return BOBGUI_PRINT_OPERATION_PREVIEW_GET_IFACE (preview)->is_selected (preview, page_nr);
}
