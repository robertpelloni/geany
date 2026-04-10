/* BOBGUI - The Bobgui Framework
 * bobguiprintoperation.h: Print Operation
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

#include "bobguiprintoperation.h"

G_BEGIN_DECLS

/* Page drawing states */
typedef enum
{
  BOBGUI_PAGE_DRAWING_STATE_READY,
  BOBGUI_PAGE_DRAWING_STATE_DRAWING,
  BOBGUI_PAGE_DRAWING_STATE_DEFERRED_DRAWING
} BobguiPageDrawingState;

struct _BobguiPrintOperationPrivate
{
  BobguiPrintOperationAction action;
  BobguiPrintStatus status;
  GError *error;
  char *status_string;
  BobguiPageSetup *default_page_setup;
  BobguiPrintSettings *print_settings;
  char *job_name;
  int nr_of_pages;
  int nr_of_pages_to_print;
  int page_position;
  int current_page;
  BobguiUnit unit;
  char *export_filename;
  guint use_full_page      : 1;
  guint track_print_status : 1;
  guint show_progress      : 1;
  guint cancelled          : 1;
  guint allow_async        : 1;
  guint is_sync            : 1;
  guint support_selection  : 1;
  guint has_selection      : 1;
  guint embed_page_setup   : 1;

  BobguiPageDrawingState      page_drawing_state;

  guint print_pages_idle_id;
  guint show_progress_timeout_id;

  BobguiPrintContext *print_context;
  
  BobguiPrintPages print_pages;
  BobguiPageRange *page_ranges;
  int num_page_ranges;
  
  int manual_num_copies;
  guint manual_collation   : 1;
  guint manual_reverse     : 1;
  guint manual_orientation : 1;
  double manual_scale;
  BobguiPageSet manual_page_set;
  guint manual_number_up;
  BobguiNumberUpLayout manual_number_up_layout;

  BobguiWidget *custom_widget;
  char *custom_tab_label;
  
  gpointer platform_data;
  GDestroyNotify free_platform_data;

  GMainLoop *rloop; /* recursive mainloop */

  void (*start_page) (BobguiPrintOperation *operation,
		      BobguiPrintContext   *print_context,
		      BobguiPageSetup      *page_setup);
  void (*end_page)   (BobguiPrintOperation *operation,
		      BobguiPrintContext   *print_context);
  void (*end_run)    (BobguiPrintOperation *operation,
		      gboolean           wait,
		      gboolean           cancelled);
};


typedef void (* BobguiPrintOperationPrintFunc) (BobguiPrintOperation      *op,
					     BobguiWindow              *parent,
					     gboolean                do_print,
					     BobguiPrintOperationResult result);

BobguiPrintOperationResult _bobgui_print_operation_platform_backend_run_dialog             (BobguiPrintOperation           *operation,
										      gboolean                     show_dialog,
										      BobguiWindow                   *parent,
										      gboolean                    *do_print);
void                    _bobgui_print_operation_platform_backend_run_dialog_async       (BobguiPrintOperation           *op,
										      gboolean                     show_dialog,
										      BobguiWindow                   *parent,
										      BobguiPrintOperationPrintFunc   print_cb);
void                    _bobgui_print_operation_platform_backend_launch_preview         (BobguiPrintOperation           *op,
										      cairo_surface_t             *surface,
										      BobguiWindow                   *parent,
										      const char                  *filename);
cairo_surface_t *       _bobgui_print_operation_platform_backend_create_preview_surface (BobguiPrintOperation           *op,
										      BobguiPageSetup                *page_setup,
										      double                      *dpi_x,
										      double                      *dpi_y,
										      char                        **target);
void                    _bobgui_print_operation_platform_backend_resize_preview_surface (BobguiPrintOperation           *op,
										      BobguiPageSetup                *page_setup,
										      cairo_surface_t             *surface);
void                    _bobgui_print_operation_platform_backend_preview_start_page     (BobguiPrintOperation *op,
										      cairo_surface_t *surface,
										      cairo_t *cr);
void                    _bobgui_print_operation_platform_backend_preview_end_page       (BobguiPrintOperation *op,
										      cairo_surface_t *surface,
										      cairo_t *cr);

void _bobgui_print_operation_set_status (BobguiPrintOperation *op,
				      BobguiPrintStatus     status,
				      const char        *string);

/* BobguiPrintContext private functions: */

BobguiPrintContext *_bobgui_print_context_new                             (BobguiPrintOperation *op);
void             _bobgui_print_context_set_page_setup                  (BobguiPrintContext   *context,
								     BobguiPageSetup      *page_setup);
void             _bobgui_print_context_translate_into_margin           (BobguiPrintContext   *context);
void             _bobgui_print_context_rotate_according_to_orientation (BobguiPrintContext   *context);
void             _bobgui_print_context_reverse_according_to_orientation (BobguiPrintContext *context);
void             _bobgui_print_context_set_hard_margins                (BobguiPrintContext   *context,
								     double             top,
								     double             bottom,
								     double             left,
								     double             right);

G_END_DECLS

