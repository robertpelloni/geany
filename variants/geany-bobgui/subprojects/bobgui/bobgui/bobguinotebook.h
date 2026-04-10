/* -*- Mode: C; c-file-style: "gnu"; tab-width: 8 -*- */
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
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

#include <bobgui/bobguiwidget.h>


G_BEGIN_DECLS

#define BOBGUI_TYPE_NOTEBOOK                  (bobgui_notebook_get_type ())
#define BOBGUI_NOTEBOOK(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_NOTEBOOK, BobguiNotebook))
#define BOBGUI_IS_NOTEBOOK(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_NOTEBOOK))

#define BOBGUI_TYPE_NOTEBOOK_PAGE (bobgui_notebook_page_get_type ())
#define BOBGUI_NOTEBOOK_PAGE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_NOTEBOOK_PAGE, BobguiNotebookPage))
#define BOBGUI_IS_NOTEBOOK_PAGE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_NOTEBOOK_PAGE))

typedef struct _BobguiNotebookPage BobguiNotebookPage;

/**
 * BobguiNotebookTab:
 * @BOBGUI_NOTEBOOK_TAB_FIRST: the first tab in the notebook
 * @BOBGUI_NOTEBOOK_TAB_LAST: the last tab in the notebook
 *
 * The parameter used in the action signals of `BobguiNotebook`.
 */
typedef enum
{
  BOBGUI_NOTEBOOK_TAB_FIRST,
  BOBGUI_NOTEBOOK_TAB_LAST
} BobguiNotebookTab;

typedef struct _BobguiNotebook BobguiNotebook;

/***********************************************************
 *           Creation, insertion, deletion                 *
 ***********************************************************/

GDK_AVAILABLE_IN_ALL
GType   bobgui_notebook_get_type       (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
BobguiWidget * bobgui_notebook_new        (void);
GDK_AVAILABLE_IN_ALL
int bobgui_notebook_append_page        (BobguiNotebook *notebook,
				     BobguiWidget   *child,
				     BobguiWidget   *tab_label);
GDK_AVAILABLE_IN_ALL
int bobgui_notebook_append_page_menu   (BobguiNotebook *notebook,
				     BobguiWidget   *child,
				     BobguiWidget   *tab_label,
				     BobguiWidget   *menu_label);
GDK_AVAILABLE_IN_ALL
int bobgui_notebook_prepend_page       (BobguiNotebook *notebook,
				     BobguiWidget   *child,
				     BobguiWidget   *tab_label);
GDK_AVAILABLE_IN_ALL
int bobgui_notebook_prepend_page_menu  (BobguiNotebook *notebook,
				     BobguiWidget   *child,
				     BobguiWidget   *tab_label,
				     BobguiWidget   *menu_label);
GDK_AVAILABLE_IN_ALL
int bobgui_notebook_insert_page        (BobguiNotebook *notebook,
				     BobguiWidget   *child,
				     BobguiWidget   *tab_label,
				     int          position);
GDK_AVAILABLE_IN_ALL
int bobgui_notebook_insert_page_menu   (BobguiNotebook *notebook,
				     BobguiWidget   *child,
				     BobguiWidget   *tab_label,
				     BobguiWidget   *menu_label,
				     int          position);
GDK_AVAILABLE_IN_ALL
void bobgui_notebook_remove_page       (BobguiNotebook *notebook,
				     int          page_num);

/***********************************************************
 *           Tabs drag and drop                            *
 ***********************************************************/

GDK_AVAILABLE_IN_ALL
void         bobgui_notebook_set_group_name (BobguiNotebook *notebook,
                                          const char  *group_name);
GDK_AVAILABLE_IN_ALL
const char *bobgui_notebook_get_group_name  (BobguiNotebook *notebook);



/***********************************************************
 *            query, set current NotebookPage              *
 ***********************************************************/

GDK_AVAILABLE_IN_ALL
int        bobgui_notebook_get_current_page (BobguiNotebook *notebook);
GDK_AVAILABLE_IN_ALL
BobguiWidget* bobgui_notebook_get_nth_page     (BobguiNotebook *notebook,
					  int          page_num);
GDK_AVAILABLE_IN_ALL
int        bobgui_notebook_get_n_pages      (BobguiNotebook *notebook);
GDK_AVAILABLE_IN_ALL
int        bobgui_notebook_page_num         (BobguiNotebook *notebook,
					  BobguiWidget   *child);
GDK_AVAILABLE_IN_ALL
void       bobgui_notebook_set_current_page (BobguiNotebook *notebook,
					  int          page_num);
GDK_AVAILABLE_IN_ALL
void       bobgui_notebook_next_page        (BobguiNotebook *notebook);
GDK_AVAILABLE_IN_ALL
void       bobgui_notebook_prev_page        (BobguiNotebook *notebook);

/***********************************************************
 *            set Notebook, NotebookTab style              *
 ***********************************************************/

GDK_AVAILABLE_IN_ALL
void     bobgui_notebook_set_show_border      (BobguiNotebook     *notebook,
					    gboolean         show_border);
GDK_AVAILABLE_IN_ALL
gboolean bobgui_notebook_get_show_border      (BobguiNotebook     *notebook);
GDK_AVAILABLE_IN_ALL
void     bobgui_notebook_set_show_tabs        (BobguiNotebook     *notebook,
					    gboolean         show_tabs);
GDK_AVAILABLE_IN_ALL
gboolean bobgui_notebook_get_show_tabs        (BobguiNotebook     *notebook);
GDK_AVAILABLE_IN_ALL
void     bobgui_notebook_set_tab_pos          (BobguiNotebook     *notebook,
				            BobguiPositionType  pos);
GDK_AVAILABLE_IN_ALL
BobguiPositionType bobgui_notebook_get_tab_pos   (BobguiNotebook     *notebook);
GDK_AVAILABLE_IN_ALL
void     bobgui_notebook_set_scrollable       (BobguiNotebook     *notebook,
					    gboolean         scrollable);
GDK_AVAILABLE_IN_ALL
gboolean bobgui_notebook_get_scrollable       (BobguiNotebook     *notebook);

/***********************************************************
 *               enable/disable PopupMenu                  *
 ***********************************************************/

GDK_AVAILABLE_IN_ALL
void bobgui_notebook_popup_enable  (BobguiNotebook *notebook);
GDK_AVAILABLE_IN_ALL
void bobgui_notebook_popup_disable (BobguiNotebook *notebook);

/***********************************************************
 *             query/set NotebookPage Properties           *
 ***********************************************************/

GDK_AVAILABLE_IN_ALL
BobguiWidget * bobgui_notebook_get_tab_label    (BobguiNotebook *notebook,
					   BobguiWidget   *child);
GDK_AVAILABLE_IN_ALL
void bobgui_notebook_set_tab_label           (BobguiNotebook *notebook,
					   BobguiWidget   *child,
					   BobguiWidget   *tab_label);
GDK_AVAILABLE_IN_ALL
void          bobgui_notebook_set_tab_label_text (BobguiNotebook *notebook,
                                               BobguiWidget   *child,
                                               const char  *tab_text);
GDK_AVAILABLE_IN_ALL
const char * bobgui_notebook_get_tab_label_text  (BobguiNotebook *notebook,
                                               BobguiWidget   *child);
GDK_AVAILABLE_IN_ALL
BobguiWidget * bobgui_notebook_get_menu_label   (BobguiNotebook *notebook,
					   BobguiWidget   *child);
GDK_AVAILABLE_IN_ALL
void bobgui_notebook_set_menu_label          (BobguiNotebook *notebook,
					   BobguiWidget   *child,
					   BobguiWidget   *menu_label);
GDK_AVAILABLE_IN_ALL
void          bobgui_notebook_set_menu_label_text (BobguiNotebook *notebook,
                                                BobguiWidget   *child,
                                                const char  *menu_text);
GDK_AVAILABLE_IN_ALL
const char * bobgui_notebook_get_menu_label_text  (BobguiNotebook *notebook,
						BobguiWidget   *child);
GDK_AVAILABLE_IN_ALL
void bobgui_notebook_reorder_child           (BobguiNotebook *notebook,
					   BobguiWidget   *child,
					   int          position);
GDK_AVAILABLE_IN_ALL
gboolean bobgui_notebook_get_tab_reorderable (BobguiNotebook *notebook,
					   BobguiWidget   *child);
GDK_AVAILABLE_IN_ALL
void bobgui_notebook_set_tab_reorderable     (BobguiNotebook *notebook,
					   BobguiWidget   *child,
					   gboolean     reorderable);
GDK_AVAILABLE_IN_ALL
gboolean bobgui_notebook_get_tab_detachable  (BobguiNotebook *notebook,
					   BobguiWidget   *child);
GDK_AVAILABLE_IN_ALL
void bobgui_notebook_set_tab_detachable      (BobguiNotebook *notebook,
					   BobguiWidget   *child,
					   gboolean     detachable);
GDK_AVAILABLE_IN_ALL
void bobgui_notebook_detach_tab              (BobguiNotebook *notebook,
                                           BobguiWidget   *child);

GDK_AVAILABLE_IN_ALL
BobguiWidget* bobgui_notebook_get_action_widget (BobguiNotebook *notebook,
                                           BobguiPackType  pack_type);
GDK_AVAILABLE_IN_ALL
void       bobgui_notebook_set_action_widget (BobguiNotebook *notebook,
                                           BobguiWidget   *widget,
                                           BobguiPackType  pack_type);

GDK_AVAILABLE_IN_ALL
GType   bobgui_notebook_page_get_type  (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
BobguiNotebookPage *bobgui_notebook_get_page (BobguiNotebook *notebook,
                                        BobguiWidget   *child);
GDK_AVAILABLE_IN_ALL
BobguiWidget *bobgui_notebook_page_get_child (BobguiNotebookPage *page);
GDK_AVAILABLE_IN_ALL
GListModel *bobgui_notebook_get_pages (BobguiNotebook *notebook);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiNotebook, g_object_unref)

G_END_DECLS

