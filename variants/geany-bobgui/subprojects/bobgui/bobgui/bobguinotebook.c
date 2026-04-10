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
 * BOBGUI at ftp:ftp.bobgui.org/pub/bobgui/.
 */

#include "config.h"

#include "bobguinotebook.h"

#include "bobguibox.h"
#include "bobguiboxlayout.h"
#include "bobguibuildable.h"
#include "bobguibutton.h"
#include "bobguidroptarget.h"
#include "bobguidragicon.h"
#include "bobguidropcontrollermotion.h"
#include "bobguieventcontrollermotion.h"
#include "bobguigestureclick.h"
#include "bobguigizmoprivate.h"
#include "bobguibuiltiniconprivate.h"
#include <glib/gi18n-lib.h>
#include "bobguilabel.h"
#include "bobguimain.h"
#include "bobguimarshalers.h"
#include "bobguipopovermenuprivate.h"
#include "bobguiorientable.h"
#include "bobguisizerequest.h"
#include "bobguiprivate.h"
#include "bobguiselectionmodel.h"
#include "bobguistack.h"
#include "bobguitypebuiltins.h"
#include "bobguiwidgetprivate.h"
#include "bobguidragsourceprivate.h"
#include "bobguiwidgetpaintable.h"
#include "bobguinative.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

/**
 * BobguiNotebook:
 *
 * Switches between children using tabs.
 *
 * <picture>
 *   <source srcset="notebook-dark.png" media="(prefers-color-scheme: dark)">
 *   <img alt="An example BobguiNotebook" src="notebook.png">
 * </picture>
 *
 * There are many configuration options for `BobguiNotebook`. Among
 * other things, you can choose on which edge the tabs appear
 * (see [method@Bobgui.Notebook.set_tab_pos]), whether, if there are
 * too many tabs to fit the notebook should be made bigger or scrolling
 * arrows added (see [method@Bobgui.Notebook.set_scrollable]), and whether
 * there will be a popup menu allowing the users to switch pages.
 * (see [method@Bobgui.Notebook.popup_enable]).
 *
 * # BobguiNotebook as BobguiBuildable
 *
 * The `BobguiNotebook` implementation of the `BobguiBuildable` interface
 * supports placing children into tabs by specifying “tab” as the
 * “type” attribute of a `<child>` element. Note that the content
 * of the tab must be created before the tab can be filled.
 * A tab child can be specified without specifying a `<child>`
 * type attribute.
 *
 * To add a child widget in the notebooks action area, specify
 * "action-start" or “action-end” as the “type” attribute of the
 * `<child>` element.
 *
 * An example of a UI definition fragment with `BobguiNotebook`:
 *
 * ```xml
 * <object class="BobguiNotebook">
 *   <child>
 *     <object class="BobguiLabel" id="notebook-content">
 *       <property name="label">Content</property>
 *     </object>
 *   </child>
 *   <child type="tab">
 *     <object class="BobguiLabel" id="notebook-tab">
 *       <property name="label">Tab</property>
 *     </object>
 *   </child>
 * </object>
 * ```
 *
 * # Shortcuts and Gestures
 *
 * `BobguiNotebook` supports the following keyboard shortcuts:
 *
 * - <kbd>Shift</kbd>+<kbd>F10</kbd> or <kbd>Menu</kbd> opens the context menu.
 * - <kbd>Home</kbd> moves the focus to the first tab.
 * - <kbd>End</kbd> moves the focus to the last tab.
 *
 * Additionally, the following signals have default keybindings:
 *
 * - [signal@Bobgui.Notebook::change-current-page]
 * - [signal@Bobgui.Notebook::focus-tab]
 * - [signal@Bobgui.Notebook::move-focus-out]
 * - [signal@Bobgui.Notebook::reorder-tab]
 * - [signal@Bobgui.Notebook::select-page]
 *
 * Tabs support drag-and-drop between notebooks sharing the same `group-name`,
 * or to new windows by handling the `::create-window` signal.
 *
 * # Actions
 *
 * `BobguiNotebook` defines a set of built-in actions:
 *
 * - `menu.popup` opens the tabs context menu.
 *
 * # CSS nodes
 *
 * ```
 * notebook
 * ├── header.top
 * │   ├── [<action widget>]
 * │   ├── tabs
 * │   │   ├── [arrow]
 * │   │   ├── tab
 * │   │   │   ╰── <tab label>
 * ┊   ┊   ┊
 * │   │   ├── tab[.reorderable-page]
 * │   │   │   ╰── <tab label>
 * │   │   ╰── [arrow]
 * │   ╰── [<action widget>]
 * │
 * ╰── stack
 *     ├── <child>
 *     ┊
 *     ╰── <child>
 * ```
 *
 * `BobguiNotebook` has a main CSS node with name `notebook`, a subnode
 * with name `header` and below that a subnode with name `tabs` which
 * contains one subnode per tab with name `tab`.
 *
 * If action widgets are present, their CSS nodes are placed next
 * to the `tabs` node. If the notebook is scrollable, CSS nodes with
 * name `arrow` are placed as first and last child of the `tabs` node.
 *
 * The main node gets the `.frame` style class when the notebook
 * has a border (see [method@Bobgui.Notebook.set_show_border]).
 *
 * The header node gets one of the style class `.top`, `.bottom`,
 * `.left` or `.right`, depending on where the tabs are placed. For
 * reorderable pages, the tab node gets the `.reorderable-page` class.
 *
 * A `tab` node gets the `.dnd` style class while it is moved with drag-and-drop.
 *
 * The nodes are always arranged from left-to-right, regardless of text direction.
 *
 * # Accessibility
 *
 * `BobguiNotebook` uses the following roles:
 *
 *  - [enum@Bobgui.AccessibleRole.group] for the notebook widget
 *  - [enum@Bobgui.AccessibleRole.tab_list] for the list of tabs
 *  - [enum@Bobgui.AccessibleRole.tab] role for each tab
 *  - [enum@Bobgui.AccessibleRole.tab_panel] for each page
 */

/**
 * BobguiNotebookPage:
 *
 * An auxiliary object used by `BobguiNotebook`.
 */

#define SCROLL_DELAY_FACTOR   5
#define SCROLL_THRESHOLD      12
#define DND_THRESHOLD_MULTIPLIER 4

#define TIMEOUT_INITIAL  500
#define TIMEOUT_REPEAT    50
#define TIMEOUT_EXPAND   500

typedef struct _BobguiNotebookPage BobguiNotebookPage;

typedef enum
{
  DRAG_OPERATION_NONE,
  DRAG_OPERATION_REORDER,
  DRAG_OPERATION_DETACH
} BobguiNotebookDragOperation;

enum {
  ACTION_WIDGET_START,
  ACTION_WIDGET_END,
  N_ACTION_WIDGETS
};

#define BOBGUI_NOTEBOOK_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), BOBGUI_TYPE_NOTEBOOK, BobguiNotebookClass))
#define BOBGUI_NOTEBOOK_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_NOTEBOOK, BobguiNotebookClass))

typedef struct _BobguiNotebookClass         BobguiNotebookClass;

struct _BobguiNotebookClass
{
  BobguiWidgetClass parent_class;

  void (* switch_page)       (BobguiNotebook     *notebook,
                              BobguiWidget       *page,
                              guint            page_num);

  /* Action signals for keybindings */
  gboolean (* select_page)     (BobguiNotebook       *notebook,
                                gboolean           move_focus);
  gboolean (* focus_tab)       (BobguiNotebook       *notebook,
                                BobguiNotebookTab     type);
  gboolean (* change_current_page) (BobguiNotebook   *notebook,
                                int                offset);
  void (* move_focus_out)      (BobguiNotebook       *notebook,
                                BobguiDirectionType   direction);
  gboolean (* reorder_tab)     (BobguiNotebook       *notebook,
                                BobguiDirectionType   direction,
                                gboolean           move_to_last);
  /* More vfuncs */
  int (* insert_page)          (BobguiNotebook       *notebook,
                                BobguiWidget         *child,
                                BobguiWidget         *tab_label,
                                BobguiWidget         *menu_label,
                                int                position);

  BobguiNotebook * (* create_window) (BobguiNotebook    *notebook,
                                   BobguiWidget      *page);

  void (* page_reordered)      (BobguiNotebook     *notebook,
                                BobguiWidget       *child,
                                guint            page_num);

  void (* page_removed)        (BobguiNotebook     *notebook,
                                BobguiWidget       *child,
                                guint            page_num);

  void (* page_added)          (BobguiNotebook     *notebook,
                                BobguiWidget       *child,
                                guint            page_num);
};

struct _BobguiNotebook
{
  BobguiWidget container;

  BobguiNotebookDragOperation   operation;
  BobguiNotebookPage           *cur_page;
  BobguiNotebookPage           *detached_tab;
  BobguiWidget                 *action_widget[N_ACTION_WIDGETS];
  BobguiWidget                 *menu;
  BobguiWidget                 *menu_box;

  BobguiWidget                 *stack_widget;
  BobguiWidget                 *header_widget;
  BobguiWidget                 *tabs_widget;
  BobguiWidget                 *arrow_widget[4];

  GListModel    *pages;

  GList         *children;
  GList         *first_tab;             /* The first tab visible (for scrolling notebooks) */
  GList         *focus_tab;

  double         drag_begin_x;
  double         drag_begin_y;
  int            drag_offset_x;
  int            drag_offset_y;
  int            drag_surface_x;
  int            drag_surface_y;
  double         mouse_x;
  double         mouse_y;
  int            pressed_button;

  GQuark         group;

  guint          dnd_timer;
  guint                      switch_page_timer;
  BobguiNotebookPage           *switch_page;

  guint32        timer;

  guint          child_has_focus    : 1;
  guint          click_child        : 3;
  guint          remove_in_detach   : 1;
  guint          focus_out          : 1; /* Flag used by ::move-focus-out implementation */
  guint          has_scrolled       : 1;
  guint          need_timer         : 1;
  guint          show_border        : 1;
  guint          show_tabs          : 1;
  guint          scrollable         : 1;
  guint          tab_pos            : 2;
  guint          rootwindow_drop    : 1;
};

enum {
  SWITCH_PAGE,
  FOCUS_TAB,
  SELECT_PAGE,
  CHANGE_CURRENT_PAGE,
  MOVE_FOCUS_OUT,
  REORDER_TAB,
  PAGE_REORDERED,
  PAGE_REMOVED,
  PAGE_ADDED,
  CREATE_WINDOW,
  LAST_SIGNAL
};

enum {
  STEP_PREV,
  STEP_NEXT
};

typedef enum
{
  ARROW_LEFT_BEFORE,
  ARROW_RIGHT_BEFORE,
  ARROW_LEFT_AFTER,
  ARROW_RIGHT_AFTER,
  ARROW_NONE
} BobguiNotebookArrow;

typedef enum
{
  POINTER_BEFORE,
  POINTER_AFTER,
  POINTER_BETWEEN
} BobguiNotebookPointerPosition;

#define ARROW_IS_LEFT(arrow)  ((arrow) == ARROW_LEFT_BEFORE || (arrow) == ARROW_LEFT_AFTER)
#define ARROW_IS_BEFORE(arrow) ((arrow) == ARROW_LEFT_BEFORE || (arrow) == ARROW_RIGHT_BEFORE)

enum {
  PROP_0,
  PROP_TAB_POS,
  PROP_SHOW_TABS,
  PROP_SHOW_BORDER,
  PROP_SCROLLABLE,
  PROP_PAGE,
  PROP_ENABLE_POPUP,
  PROP_GROUP_NAME,
  PROP_PAGES,
  LAST_PROP
};

static GParamSpec *properties[LAST_PROP];

enum {
  CHILD_PROP_0,
  CHILD_PROP_TAB_LABEL,
  CHILD_PROP_MENU_LABEL,
  CHILD_PROP_POSITION,
  CHILD_PROP_TAB_EXPAND,
  CHILD_PROP_TAB_FILL,
  CHILD_PROP_REORDERABLE,
  CHILD_PROP_DETACHABLE,
  CHILD_PROP_CHILD,
  CHILD_PROP_TAB,
  CHILD_PROP_MENU,
};

#define BOBGUI_NOTEBOOK_PAGE_FROM_LIST(_glist_)         ((BobguiNotebookPage *)(_glist_)->data)

#define NOTEBOOK_IS_TAB_LABEL_PARENT(_notebook_,_page_) \
  (g_object_get_data (G_OBJECT ((_page_)->tab_label), "notebook") == _notebook_)

struct _BobguiNotebookPage
{
  GObject instance;

  BobguiWidget *child;
  BobguiWidget *tab_label;
  BobguiWidget *menu_label;
  BobguiWidget *last_focus_child;  /* Last descendant of the page that had focus */

  BobguiWidget *tab_widget;        /* widget used for the tab itself */

  char *tab_text;
  char *menu_text;

  guint default_menu : 1;       /* If true, we create the menu label ourself */
  guint default_tab  : 1;       /* If true, we create the tab label ourself */
  guint expand       : 1;
  guint fill         : 1;
  guint reorderable  : 1;
  guint detachable   : 1;

  BobguiRequisition requisition;

  gulong mnemonic_activate_signal;
  gulong notify_visible_handler;
};

typedef struct _BobguiNotebookPageClass BobguiNotebookPageClass;
struct _BobguiNotebookPageClass
{
  GObjectClass parent_class;
};

G_DEFINE_TYPE (BobguiNotebookPage, bobgui_notebook_page, G_TYPE_OBJECT)

static void
bobgui_notebook_page_init (BobguiNotebookPage *page)
{
  page->default_tab = TRUE;
  page->default_menu = TRUE;
  page->fill = TRUE;
}

static void
bobgui_notebook_page_finalize (GObject *object)
{
  BobguiNotebookPage *page = BOBGUI_NOTEBOOK_PAGE (object);

  g_clear_object (&page->child);
  g_clear_object (&page->tab_label);
  g_clear_object (&page->menu_label);

  g_free (page->tab_text);
  g_free (page->menu_text);

  G_OBJECT_CLASS (bobgui_notebook_page_parent_class)->finalize (object);
}


static void
bobgui_notebook_page_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  BobguiNotebookPage *page = BOBGUI_NOTEBOOK_PAGE (object);

  switch (property_id)
    {
    case CHILD_PROP_CHILD:
      g_set_object (&page->child, g_value_get_object (value));
      break;

    case CHILD_PROP_TAB:
      g_set_object (&page->tab_label, g_value_get_object (value));
      page->default_tab = page->tab_label == NULL;
      break;

    case CHILD_PROP_MENU:
      g_set_object (&page->menu_label, g_value_get_object (value));
      page->default_menu = page->menu_label == NULL;
      break;

    case CHILD_PROP_TAB_LABEL:
      g_free (page->tab_text);
      page->tab_text = g_value_dup_string (value);
      if (page->default_tab && BOBGUI_IS_LABEL (page->tab_label))
        bobgui_label_set_label (BOBGUI_LABEL (page->tab_label), page->tab_text);
      break;

    case CHILD_PROP_MENU_LABEL:
      g_free (page->menu_text);
      page->menu_text = g_value_dup_string (value);
      if (page->default_menu && BOBGUI_IS_LABEL (page->menu_label))
        bobgui_label_set_label (BOBGUI_LABEL (page->menu_label), page->menu_text);
      break;

    case CHILD_PROP_POSITION:
      {
        BobguiNotebook *notebook = NULL;
        if (page->tab_widget)
          notebook = BOBGUI_NOTEBOOK (g_object_get_data (G_OBJECT (page->tab_widget), "notebook"));

        if (notebook)
          bobgui_notebook_reorder_child (notebook, page->child, g_value_get_int (value));
      }
      break;

    case CHILD_PROP_TAB_EXPAND:
      if (page->expand != g_value_get_boolean (value))
        {
          page->expand = g_value_get_boolean (value);
          g_object_notify_by_pspec (object, pspec);
        }
      break;

    case CHILD_PROP_TAB_FILL:
      if (page->fill != g_value_get_boolean (value))
        {
          page->fill = g_value_get_boolean (value);
          g_object_notify_by_pspec (object, pspec);
        }
      break;

    case CHILD_PROP_REORDERABLE:
      if (page->reorderable != g_value_get_boolean (value))
        {
          page->reorderable = g_value_get_boolean (value);
          g_object_notify_by_pspec (object, pspec);
        }
      break;

    case CHILD_PROP_DETACHABLE:
      if (page->detachable != g_value_get_boolean (value))
        {
          page->detachable = g_value_get_boolean (value);
          g_object_notify_by_pspec (object, pspec);
        }
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
bobgui_notebook_page_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  BobguiNotebookPage *page = BOBGUI_NOTEBOOK_PAGE (object);

  switch (property_id)
    {
    case CHILD_PROP_CHILD:
      g_value_set_object (value, page->child);
      break;

    case CHILD_PROP_TAB:
      g_value_set_object (value, page->tab_label);
      break;

    case CHILD_PROP_MENU:
      g_value_set_object (value, page->menu_label);
      break;

    case CHILD_PROP_TAB_LABEL:
      g_value_set_string (value, page->tab_text);
      break;

    case CHILD_PROP_MENU_LABEL:
      g_value_set_string (value, page->menu_text);
      break;

    case CHILD_PROP_POSITION:
      {
        BobguiNotebook *notebook = NULL;
        if (page->tab_widget)
          notebook = BOBGUI_NOTEBOOK (g_object_get_data (G_OBJECT (page->tab_widget), "notebook"));

        if (notebook)
          g_value_set_int (value, g_list_index (notebook->children, page));
      }
      break;

    case CHILD_PROP_TAB_EXPAND:
        g_value_set_boolean (value, page->expand);
      break;

    case CHILD_PROP_TAB_FILL:
        g_value_set_boolean (value, page->fill);
      break;

    case CHILD_PROP_REORDERABLE:
      g_value_set_boolean (value, page->reorderable);
      break;

    case CHILD_PROP_DETACHABLE:
      g_value_set_boolean (value, page->detachable);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
bobgui_notebook_page_class_init (BobguiNotebookPageClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->finalize = bobgui_notebook_page_finalize;
  object_class->get_property = bobgui_notebook_page_get_property;
  object_class->set_property = bobgui_notebook_page_set_property;

  /**
   * BobguiNotebookPage:child:
   *
   * The child for this page.
   */
  g_object_class_install_property (object_class,
                                   CHILD_PROP_CHILD,
                                   g_param_spec_object ("child", NULL, NULL,
                                                        BOBGUI_TYPE_WIDGET,
                                                        BOBGUI_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY));

  /**
   * BobguiNotebookPage:tab:
   *
   * The tab widget for this page.
   */
  g_object_class_install_property (object_class,
                                   CHILD_PROP_TAB,
                                   g_param_spec_object ("tab", NULL, NULL,
                                                        BOBGUI_TYPE_WIDGET,
                                                        BOBGUI_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY));

  /**
   * BobguiNotebookPage:menu:
   *
   * The label widget displayed in the child's menu entry.
   */
  g_object_class_install_property (object_class,
                                   CHILD_PROP_MENU,
                                   g_param_spec_object ("menu", NULL, NULL,
                                                        BOBGUI_TYPE_WIDGET,
                                                        BOBGUI_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY));

  /**
   * BobguiNotebookPage:tab-label:
   *
   * The text of the tab widget.
   */
  g_object_class_install_property (object_class,
                                   CHILD_PROP_TAB_LABEL,
                                   g_param_spec_string ("tab-label", NULL, NULL,
                                                        NULL,
                                                         BOBGUI_PARAM_READWRITE));

  /**
   * BobguiNotebookPage:menu-label:
   *
   * The text of the menu widget.
   */
  g_object_class_install_property (object_class,
                                   CHILD_PROP_MENU_LABEL,
                                   g_param_spec_string ("menu-label", NULL, NULL,
                                                        NULL,
                                                         BOBGUI_PARAM_READWRITE));

  /**
   * BobguiNotebookPage:position:
   *
   * The index of the child in the parent.
   */
  g_object_class_install_property (object_class,
                                   CHILD_PROP_POSITION,
                                   g_param_spec_int ("position", NULL, NULL,
                                                     -1, G_MAXINT, 0,
                                                     BOBGUI_PARAM_READWRITE));

  /**
   * BobguiNotebookPage:tab-expand:
   *
   * Whether to expand the child's tab.
   */
  g_object_class_install_property (object_class,
                                   CHILD_PROP_TAB_EXPAND,
                                   g_param_spec_boolean ("tab-expand", NULL, NULL,
                                                         FALSE,
                                                         BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * BobguiNotebookPage:tab-fill:
   *
   * Whether the child's tab should fill the allocated area.
   */
  g_object_class_install_property (object_class,
                                   CHILD_PROP_TAB_FILL,
                                   g_param_spec_boolean ("tab-fill", NULL, NULL,
                                                         TRUE,
                                                         BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * BobguiNotebookPage:reorderable:
   *
   * Whether the tab is reorderable by user action.
   */
  g_object_class_install_property (object_class,
                                   CHILD_PROP_REORDERABLE,
                                   g_param_spec_boolean ("reorderable", NULL, NULL,
                                                         FALSE,
                                                         BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * BobguiNotebookPage:detachable:
   *
   * Whether the tab is detachable.
   */
  g_object_class_install_property (object_class,
                                   CHILD_PROP_DETACHABLE,
                                   g_param_spec_boolean ("detachable", NULL, NULL,
                                                         FALSE,
                                                         BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

}

#define BOBGUI_TYPE_NOTEBOOK_ROOT_CONTENT (bobgui_notebook_root_content_get_type ())
#define BOBGUI_NOTEBOOK_ROOT_CONTENT(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_NOTEBOOK_ROOT_CONTENT, BobguiNotebookRootContent))
#define BOBGUI_IS_NOTEBOOK_ROOT_CONTENT(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_NOTEBOOK_ROOT_CONTENT))

typedef struct _BobguiNotebookRootContent BobguiNotebookRootContent;
typedef struct _BobguiNotebookRootContentClass BobguiNotebookRootContentClass;

struct _BobguiNotebookRootContent
{
  GdkContentProvider parent_instance;

  BobguiNotebook *notebook;
};

struct _BobguiNotebookRootContentClass
{
  GdkContentProviderClass parent_class;
};

static GdkContentFormats *
bobgui_notebook_root_content_ref_formats (GdkContentProvider *provider)
{
  return gdk_content_formats_new ((const char *[1]) { "application/x-rootwindow-drop" }, 1);
}

GType bobgui_notebook_root_content_get_type (void);

G_DEFINE_TYPE (BobguiNotebookRootContent, bobgui_notebook_root_content, GDK_TYPE_CONTENT_PROVIDER)

static void
bobgui_notebook_root_content_write_mime_type_async (GdkContentProvider     *provider,
                                                 const char             *mime_type,
                                                 GOutputStream          *stream,
                                                 int                     io_priority,
                                                 GCancellable           *cancellable,
                                                 GAsyncReadyCallback     callback,
                                                 gpointer                user_data)
{
  BobguiNotebookRootContent *self = BOBGUI_NOTEBOOK_ROOT_CONTENT (provider);
  GTask *task;

  self->notebook->rootwindow_drop = TRUE;

  task = g_task_new (self, cancellable, callback, user_data);
  g_task_set_priority (task, io_priority);
  g_task_set_source_tag (task, bobgui_notebook_root_content_write_mime_type_async);
  g_task_return_boolean (task, TRUE);
  g_object_unref (task);
}

static gboolean
bobgui_notebook_root_content_write_mime_type_finish (GdkContentProvider *provider,
                                                   GAsyncResult       *result,
                                                   GError            **error)
{
  return g_task_propagate_boolean (G_TASK (result), error);
}

static void
bobgui_notebook_root_content_finalize (GObject *object)
{
  BobguiNotebookRootContent *self = BOBGUI_NOTEBOOK_ROOT_CONTENT (object);

  g_object_unref (self->notebook);

  G_OBJECT_CLASS (bobgui_notebook_root_content_parent_class)->finalize (object);
}

static void
bobgui_notebook_root_content_class_init (BobguiNotebookRootContentClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  GdkContentProviderClass *provider_class = GDK_CONTENT_PROVIDER_CLASS (class);

  object_class->finalize = bobgui_notebook_root_content_finalize;

  provider_class->ref_formats = bobgui_notebook_root_content_ref_formats;
  provider_class->write_mime_type_async = bobgui_notebook_root_content_write_mime_type_async;
  provider_class->write_mime_type_finish = bobgui_notebook_root_content_write_mime_type_finish;
}

static void
bobgui_notebook_root_content_init (BobguiNotebookRootContent *self)
{
}

static GdkContentProvider *
bobgui_notebook_root_content_new (BobguiNotebook *notebook)
{
  BobguiNotebookRootContent *result;

  result = g_object_new (BOBGUI_TYPE_NOTEBOOK_ROOT_CONTENT, NULL);

  result->notebook = g_object_ref (notebook);

  return GDK_CONTENT_PROVIDER (result);
}

/*** BobguiNotebook Methods ***/
static gboolean bobgui_notebook_select_page         (BobguiNotebook      *notebook,
                                                  gboolean          move_focus);
static gboolean bobgui_notebook_focus_tab           (BobguiNotebook      *notebook,
                                                  BobguiNotebookTab    type);
static gboolean bobgui_notebook_change_current_page (BobguiNotebook      *notebook,
                                                  int               offset);
static void     bobgui_notebook_move_focus_out      (BobguiNotebook      *notebook,
                                                  BobguiDirectionType  direction_type);
static gboolean bobgui_notebook_reorder_tab         (BobguiNotebook      *notebook,
                                                  BobguiDirectionType  direction_type,
                                                  gboolean          move_to_last);
static void     bobgui_notebook_remove_tab_label    (BobguiNotebook      *notebook,
                                                  BobguiNotebookPage  *page);

/*** GObject Methods ***/
static void bobgui_notebook_set_property        (GObject         *object,
                                              guint            prop_id,
                                              const GValue    *value,
                                              GParamSpec      *pspec);
static void bobgui_notebook_get_property        (GObject         *object,
                                              guint            prop_id,
                                              GValue          *value,
                                              GParamSpec      *pspec);
static void bobgui_notebook_finalize            (GObject         *object);
static void bobgui_notebook_dispose             (GObject         *object);

/*** BobguiWidget Methods ***/
static void bobgui_notebook_unmap               (BobguiWidget        *widget);
static void bobgui_notebook_popup_menu          (BobguiWidget        *widget,
                                              const char       *action_name,
                                              GVariant         *parameters);
static void bobgui_notebook_motion              (BobguiEventController *controller,
                                             double              x,
                                              double              y,
                                              gpointer            user_data);
static void bobgui_notebook_state_flags_changed (BobguiWidget          *widget,
                                              BobguiStateFlags       previous_state);
static void bobgui_notebook_direction_changed   (BobguiWidget        *widget,
                                              BobguiTextDirection  previous_direction);
static gboolean bobgui_notebook_focus           (BobguiWidget        *widget,
                                              BobguiDirectionType  direction);
static gboolean bobgui_notebook_grab_focus      (BobguiWidget        *widget);
static void     bobgui_notebook_set_focus_child (BobguiWidget        *widget,
                                              BobguiWidget        *child);

/*** Drag and drop Methods ***/
static void bobgui_notebook_dnd_finished_cb     (GdkDrag          *drag,
                                              BobguiWidget        *widget);
static void bobgui_notebook_drag_cancel_cb      (GdkDrag          *drag,
                                              GdkDragCancelReason reason,
                                              BobguiWidget        *widget);
static GdkDragAction bobgui_notebook_drag_motion(BobguiDropTarget    *dest,
                                              double            x,
                                              double            y,
                                              BobguiNotebook      *notebook);
static gboolean bobgui_notebook_drag_drop       (BobguiDropTarget    *dest,
                                              const GValue     *value,
                                              double            x,
                                              double            y,
                                              BobguiNotebook      *notebook);

static void bobgui_notebook_remove              (BobguiNotebook      *notebook,
                                              BobguiWidget        *widget);

/*** BobguiNotebook Methods ***/
static int bobgui_notebook_real_insert_page     (BobguiNotebook      *notebook,
                                              BobguiWidget        *child,
                                              BobguiWidget        *tab_label,
                                              BobguiWidget        *menu_label,
                                              int               position);

static BobguiNotebook *bobgui_notebook_create_window (BobguiNotebook    *notebook,
                                                BobguiWidget      *page);

static void bobgui_notebook_measure_tabs        (BobguiGizmo         *gizmo,
                                              BobguiOrientation    orientation,
                                              int               for_size,
                                              int              *minimum,
                                              int              *natural,
                                              int              *minimum_baseline,
                                              int              *natural_baseline);
static void bobgui_notebook_allocate_tabs       (BobguiGizmo         *gizmo,
                                              int               width,
                                              int               height,
                                              int               baseline);
static void     bobgui_notebook_snapshot_tabs   (BobguiGizmo         *gizmo,
                                              BobguiSnapshot      *snapshot);

/*** BobguiNotebook Private Functions ***/
static void bobgui_notebook_real_remove         (BobguiNotebook      *notebook,
                                              GList            *list);
static void bobgui_notebook_update_labels       (BobguiNotebook      *notebook);
static int bobgui_notebook_timer                (BobguiNotebook      *notebook);
static void bobgui_notebook_set_scroll_timer    (BobguiNotebook *notebook);
static int bobgui_notebook_page_compare         (gconstpointer     a,
                                              gconstpointer     b);
static GList* bobgui_notebook_find_child        (BobguiNotebook      *notebook,
                                              BobguiWidget        *child);
static GList * bobgui_notebook_search_page      (BobguiNotebook      *notebook,
                                              GList            *list,
                                              int               direction,
                                              gboolean          find_visible);
static void  bobgui_notebook_child_reordered    (BobguiNotebook      *notebook,
                                              BobguiNotebookPage  *page);
static int bobgui_notebook_insert_notebook_page (BobguiNotebook     *notebook,
                                              BobguiNotebookPage *page,
                                              int              position);

/*** BobguiNotebook Size Allocate Functions ***/
static void bobgui_notebook_pages_allocate      (BobguiNotebook      *notebook,
                                              int               width,
                                              int               height);
static void bobgui_notebook_calc_tabs           (BobguiNotebook      *notebook,
                                              GList            *start,
                                              GList           **end,
                                              int              *tab_space,
                                              guint             direction);

/*** BobguiNotebook Page Switch Methods ***/
static void bobgui_notebook_real_switch_page    (BobguiNotebook      *notebook,
                                              BobguiWidget        *child,
                                              guint             page_num);

/*** BobguiNotebook Page Switch Functions ***/
static void bobgui_notebook_switch_page         (BobguiNotebook      *notebook,
                                              BobguiNotebookPage  *page);
static int bobgui_notebook_page_select          (BobguiNotebook      *notebook,
                                              gboolean          move_focus);
static void bobgui_notebook_switch_focus_tab    (BobguiNotebook      *notebook,
                                              GList            *new_child);
static void bobgui_notebook_menu_switch_page    (BobguiWidget        *widget,
                                              BobguiNotebookPage  *page);

/*** BobguiNotebook Menu Functions ***/
static void bobgui_notebook_menu_item_create    (BobguiNotebook      *notebook,
                                              BobguiNotebookPage  *page);
static void bobgui_notebook_menu_item_recreate  (BobguiNotebook      *notebook,
                                              GList            *list);
static void bobgui_notebook_menu_label_unparent (BobguiWidget        *widget);

static void bobgui_notebook_update_tab_pos      (BobguiNotebook      *notebook);

/*** BobguiNotebook Private Setters ***/
static gboolean bobgui_notebook_mnemonic_activate_switch_page (BobguiWidget *child,
                                                            gboolean overload,
                                                            gpointer data);

static gboolean focus_tabs_in  (BobguiNotebook      *notebook);
static gboolean focus_child_in (BobguiNotebook      *notebook,
                                BobguiDirectionType  direction);

static void stop_scrolling (BobguiNotebook *notebook);
static void do_detach_tab  (BobguiNotebook *from,
                            BobguiNotebook *to,
                            BobguiWidget   *child);

/* BobguiBuildable */
static void bobgui_notebook_buildable_init           (BobguiBuildableIface *iface);
static void bobgui_notebook_buildable_add_child      (BobguiBuildable *buildable,
                                                   BobguiBuilder   *builder,
                                                   GObject      *child,
                                                   const char   *type);

static void bobgui_notebook_gesture_pressed (BobguiGestureClick *gesture,
                                          int                   n_press,
                                          double                x,
                                          double                y,
                                          gpointer              user_data);
static void bobgui_notebook_gesture_released (BobguiGestureClick *gesture,
                                           int                   n_press,
                                           double                x,
                                           double                y,
                                           gpointer              user_data);
static void bobgui_notebook_gesture_cancel   (BobguiGestureClick  *gesture,
                                           GdkEventSequence *sequence,
                                           BobguiNotebook      *notebook);

static guint notebook_signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE_WITH_CODE (BobguiNotebook, bobgui_notebook, BOBGUI_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_BUILDABLE,
                                                bobgui_notebook_buildable_init))

static void
add_tab_bindings (BobguiWidgetClass   *widget_class,
                  GdkModifierType   modifiers,
                  BobguiDirectionType  direction)
{
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_Tab, modifiers,
                                       "move-focus-out",
                                       "(i)", direction);
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_KP_Tab, modifiers,
                                       "move-focus-out",
                                       "(i)", direction);
}

static void
add_arrow_bindings (BobguiWidgetClass   *widget_class,
                    guint             keysym,
                    BobguiDirectionType  direction)
{
  guint keypad_keysym = keysym - GDK_KEY_Left + GDK_KEY_KP_Left;

  bobgui_widget_class_add_binding_signal (widget_class,
                                       keysym, GDK_CONTROL_MASK,
                                       "move-focus-out",
                                       "(i)", direction);
  bobgui_widget_class_add_binding_signal (widget_class,
                                       keypad_keysym, GDK_CONTROL_MASK,
                                       "move-focus-out",
                                       "(i)", direction);
}

static void
add_reorder_bindings (BobguiWidgetClass   *widget_class,
                      guint             keysym,
                      BobguiDirectionType  direction,
                      gboolean          move_to_last)
{
  guint keypad_keysym = keysym - GDK_KEY_Left + GDK_KEY_KP_Left;

  bobgui_widget_class_add_binding_signal (widget_class,
                                       keysym, GDK_ALT_MASK,
                                       "reorder-tab",
                                       "(ib)", direction, move_to_last);
  bobgui_widget_class_add_binding_signal (widget_class,
                                       keypad_keysym, GDK_ALT_MASK,
                                       "reorder-tab",
                                       "(ib)", direction, move_to_last);
}

static gboolean
bobgui_object_handled_accumulator (GSignalInvocationHint *ihint,
                                GValue                *return_accu,
                                const GValue          *handler_return,
                                gpointer               dummy)
{
  gboolean continue_emission;
  GObject *object;

  object = g_value_get_object (handler_return);
  g_value_set_object (return_accu, object);
  continue_emission = !object;

  return continue_emission;
}

static void
bobgui_notebook_compute_expand (BobguiWidget *widget,
                             gboolean  *hexpand_p,
                             gboolean  *vexpand_p)
{
  BobguiNotebook *notebook = BOBGUI_NOTEBOOK (widget);
  gboolean hexpand;
  gboolean vexpand;
  GList *list;
  BobguiNotebookPage *page;

  hexpand = FALSE;
  vexpand = FALSE;

  for (list = notebook->children; list; list = list->next)
    {
      page = list->data;

      hexpand = hexpand ||
        bobgui_widget_compute_expand (page->child, BOBGUI_ORIENTATION_HORIZONTAL);

      vexpand = vexpand ||
        bobgui_widget_compute_expand (page->child, BOBGUI_ORIENTATION_VERTICAL);

      if (hexpand & vexpand)
        break;
    }

  *hexpand_p = hexpand;
  *vexpand_p = vexpand;
}

static void
bobgui_notebook_class_init (BobguiNotebookClass *class)
{
  GObjectClass   *gobject_class = G_OBJECT_CLASS (class);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (class);

  gobject_class->set_property = bobgui_notebook_set_property;
  gobject_class->get_property = bobgui_notebook_get_property;
  gobject_class->finalize = bobgui_notebook_finalize;
  gobject_class->dispose = bobgui_notebook_dispose;

  widget_class->unmap = bobgui_notebook_unmap;
  widget_class->state_flags_changed = bobgui_notebook_state_flags_changed;
  widget_class->direction_changed = bobgui_notebook_direction_changed;
  widget_class->focus = bobgui_notebook_focus;
  widget_class->grab_focus = bobgui_notebook_grab_focus;
  widget_class->set_focus_child = bobgui_notebook_set_focus_child;
  widget_class->compute_expand = bobgui_notebook_compute_expand;

  class->switch_page = bobgui_notebook_real_switch_page;
  class->insert_page = bobgui_notebook_real_insert_page;

  class->focus_tab = bobgui_notebook_focus_tab;
  class->select_page = bobgui_notebook_select_page;
  class->change_current_page = bobgui_notebook_change_current_page;
  class->move_focus_out = bobgui_notebook_move_focus_out;
  class->reorder_tab = bobgui_notebook_reorder_tab;
  class->create_window = bobgui_notebook_create_window;

  /**
   * BobguiNotebook:page: (getter get_current_page) (setter set_current_page)
   *
   * The index of the current page.
   */
  properties[PROP_PAGE] =
      g_param_spec_int ("page", NULL, NULL,
                        -1, G_MAXINT,
                        -1,
                        BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiNotebook:tab-pos:
   *
   * Which side of the notebook holds the tabs.
   */
  properties[PROP_TAB_POS] =
      g_param_spec_enum ("tab-pos", NULL, NULL,
                         BOBGUI_TYPE_POSITION_TYPE,
                         BOBGUI_POS_TOP,
                         BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiNotebook:show-tabs:
   *
   * Whether tabs should be shown.
   */
  properties[PROP_SHOW_TABS] =
      g_param_spec_boolean ("show-tabs", NULL, NULL,
                            TRUE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiNotebook:show-border:
   *
   * Whether the border should be shown.
   */
  properties[PROP_SHOW_BORDER] =
      g_param_spec_boolean ("show-border", NULL, NULL,
                            TRUE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiNotebook:scrollable:
   *
   * If %TRUE, scroll arrows are added if there are too many pages to fit.
   */
  properties[PROP_SCROLLABLE] =
      g_param_spec_boolean ("scrollable", NULL, NULL,
                            FALSE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiNotebook:enable-popup:
   *
   * If %TRUE, pressing the right mouse button on the notebook shows a page switching menu.
   */
  properties[PROP_ENABLE_POPUP] =
      g_param_spec_boolean ("enable-popup", NULL, NULL,
                            FALSE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiNotebook:group-name:
   *
   * Group name for tab drag and drop.
   */
  properties[PROP_GROUP_NAME] =
      g_param_spec_string ("group-name", NULL, NULL,
                           NULL,
                           BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiNotebook:pages:
   *
   * A selection model with the pages.
   */
  properties[PROP_PAGES] =
      g_param_spec_object ("pages", NULL, NULL,
                           G_TYPE_LIST_MODEL,
                           BOBGUI_PARAM_READABLE);

  g_object_class_install_properties (gobject_class, LAST_PROP, properties);

  /**
   * BobguiNotebook::switch-page:
   * @notebook: the object which received the signal.
   * @page: the new current page
   * @page_num: the index of the page
   *
   * Emitted when the user or a function changes the current page.
   */
  notebook_signals[SWITCH_PAGE] =
    g_signal_new (I_("switch-page"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BobguiNotebookClass, switch_page),
                  NULL, NULL,
                  _bobgui_marshal_VOID__OBJECT_UINT,
                  G_TYPE_NONE, 2,
                  BOBGUI_TYPE_WIDGET,
                  G_TYPE_UINT);
  g_signal_set_va_marshaller (notebook_signals[SWITCH_PAGE],
                              G_TYPE_FROM_CLASS (gobject_class),
                              _bobgui_marshal_VOID__OBJECT_UINTv);

  /**
   * BobguiNotebook::focus-tab:
   * @notebook: the notebook
   * @tab: the notebook tab
   *
   * Emitted when a tab should be focused.
   *
   * Returns: whether the tab has been focused
   */
  notebook_signals[FOCUS_TAB] =
    g_signal_new (I_("focus-tab"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (BobguiNotebookClass, focus_tab),
                  NULL, NULL,
                  _bobgui_marshal_BOOLEAN__ENUM,
                  G_TYPE_BOOLEAN, 1,
                  BOBGUI_TYPE_NOTEBOOK_TAB);
  g_signal_set_va_marshaller (notebook_signals[FOCUS_TAB],
                              G_TYPE_FROM_CLASS (gobject_class),
                              _bobgui_marshal_BOOLEAN__ENUMv);

  /**
   * BobguiNotebook::select-page:
   * @notebook: the notebook
   * @move_focus: whether to move focus
   *
   * Emitted when a page should be selected.
   *
   * The default binding for this signal is <kbd>␣</kbd>.
   *
   * Returns: whether the page was selected
   */
  notebook_signals[SELECT_PAGE] =
    g_signal_new (I_("select-page"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (BobguiNotebookClass, select_page),
                  NULL, NULL,
                  _bobgui_marshal_BOOLEAN__BOOLEAN,
                  G_TYPE_BOOLEAN, 1,
                  G_TYPE_BOOLEAN);
  g_signal_set_va_marshaller (notebook_signals[SELECT_PAGE],
                              G_TYPE_FROM_CLASS (gobject_class),
                              _bobgui_marshal_BOOLEAN__BOOLEANv);

  /**
   * BobguiNotebook::change-current-page:
   * @notebook: the notebook
   * @page: the page index
   *
   * Emitted when the current page should be changed.
   *
   * The default bindings for this signal are
   * <kbd>Ctrl</kbd>+<kbd>Alt</kbd>+<kbd>PgUp</kbd>,
   * <kbd>Ctrl</kbd>+<kbd>Alt</kbd>+<kbd>PgDn</kbd>,
   * <kbd>Ctrl</kbd>+<kbd>PgUp</kbd> and <kbd>Ctrl</kbd>+<kbd>PgDn</kbd>.
   *
   * Returns: whether the page was changed
   */
  notebook_signals[CHANGE_CURRENT_PAGE] =
    g_signal_new (I_("change-current-page"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (BobguiNotebookClass, change_current_page),
                  NULL, NULL,
                  _bobgui_marshal_BOOLEAN__INT,
                  G_TYPE_BOOLEAN, 1,
                  G_TYPE_INT);
  g_signal_set_va_marshaller (notebook_signals[CHANGE_CURRENT_PAGE],
                              G_TYPE_FROM_CLASS (gobject_class),
                              _bobgui_marshal_BOOLEAN__INTv);

  /**
   * BobguiNotebook::move-focus-out:
   * @notebook: the notebook
   * @direction: the direction to move the focus
   *
   * Emitted when focus was moved out.
   *
   * The default bindings for this signal are
   * <kbd>Ctrl</kbd>+<kbd>Tab</kbd>,
   * <kbd>Ctrl</kbd>+<kbd>Shift</kbd>+<kbd>Tab</kbd>,
   * <kbd>Ctrl</kbd>+<kbd>←</kbd>, <kbd>Ctrl</kbd>+<kbd>→</kbd>,
   * <kbd>Ctrl</kbd>+<kbd>↑</kbd> and <kbd>Ctrl</kbd>+<kbd>↓</kbd>.
   */
  notebook_signals[MOVE_FOCUS_OUT] =
    g_signal_new (I_("move-focus-out"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (BobguiNotebookClass, move_focus_out),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 1,
                  BOBGUI_TYPE_DIRECTION_TYPE);

  /**
   * BobguiNotebook::reorder-tab:
   * @notebook: the notebook
   * @direction: the direction to move the tab
   * @move_to_last: whether to move to the last position
   *
   * Emitted when the tab should be reordered.
   *
   * The default bindings for this signal are
   * <kbd>Alt</kbd>+<kbd>Home</kbd>, <kbd>Alt</kbd>+<kbd>End</kbd>,
   * <kbd>Alt</kbd>+<kbd>PgUp</kbd>, <kbd>Alt</kbd>+<kbd>PgDn</kbd>,
   * <kbd>Alt</kbd>+<kbd>←</kbd>, <kbd>Alt</kbd>+<kbd>→</kbd>,
   * <kbd>Alt</kbd>+<kbd>↑</kbd> and <kbd>Alt</kbd>+<kbd>↓</kbd>.
   *
   * Returns: whether the tab was moved.
   */
  notebook_signals[REORDER_TAB] =
    g_signal_new (I_("reorder-tab"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (BobguiNotebookClass, reorder_tab),
                  NULL, NULL,
                  _bobgui_marshal_BOOLEAN__ENUM_BOOLEAN,
                  G_TYPE_BOOLEAN, 2,
                  BOBGUI_TYPE_DIRECTION_TYPE,
                  G_TYPE_BOOLEAN);
  g_signal_set_va_marshaller (notebook_signals[REORDER_TAB],
                              G_TYPE_FROM_CLASS (gobject_class),
                              _bobgui_marshal_BOOLEAN__ENUM_BOOLEANv);
  /**
   * BobguiNotebook::page-reordered:
   * @notebook: the `BobguiNotebook`
   * @child: the child `BobguiWidget` affected
   * @page_num: the new page number for @child
   *
   * the ::page-reordered signal is emitted in the notebook
   * right after a page has been reordered.
   */
  notebook_signals[PAGE_REORDERED] =
    g_signal_new (I_("page-reordered"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BobguiNotebookClass, page_reordered),
                  NULL, NULL,
                  _bobgui_marshal_VOID__OBJECT_UINT,
                  G_TYPE_NONE, 2,
                  BOBGUI_TYPE_WIDGET,
                  G_TYPE_UINT);
  g_signal_set_va_marshaller (notebook_signals[PAGE_REORDERED],
                              G_TYPE_FROM_CLASS (gobject_class),
                              _bobgui_marshal_VOID__OBJECT_UINTv);
  /**
   * BobguiNotebook::page-removed:
   * @notebook: the `BobguiNotebook`
   * @child: the child `BobguiWidget` affected
   * @page_num: the @child page number
   *
   * the ::page-removed signal is emitted in the notebook
   * right after a page is removed from the notebook.
   */
  notebook_signals[PAGE_REMOVED] =
    g_signal_new (I_("page-removed"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BobguiNotebookClass, page_removed),
                  NULL, NULL,
                  _bobgui_marshal_VOID__OBJECT_UINT,
                  G_TYPE_NONE, 2,
                  BOBGUI_TYPE_WIDGET,
                  G_TYPE_UINT);
  g_signal_set_va_marshaller (notebook_signals[PAGE_REMOVED],
                              G_TYPE_FROM_CLASS (gobject_class),
                              _bobgui_marshal_VOID__OBJECT_UINTv);
  /**
   * BobguiNotebook::page-added:
   * @notebook: the `BobguiNotebook`
   * @child: the child `BobguiWidget` affected
   * @page_num: the new page number for @child
   *
   * the ::page-added signal is emitted in the notebook
   * right after a page is added to the notebook.
   */
  notebook_signals[PAGE_ADDED] =
    g_signal_new (I_("page-added"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BobguiNotebookClass, page_added),
                  NULL, NULL,
                  _bobgui_marshal_VOID__OBJECT_UINT,
                  G_TYPE_NONE, 2,
                  BOBGUI_TYPE_WIDGET,
                  G_TYPE_UINT);
  g_signal_set_va_marshaller (notebook_signals[PAGE_ADDED],
                              G_TYPE_FROM_CLASS (gobject_class),
                              _bobgui_marshal_VOID__OBJECT_UINTv);

  /**
   * BobguiNotebook::create-window:
   * @notebook: the `BobguiNotebook` emitting the signal
   * @page: the tab of @notebook that is being detached
   *
   * The ::create-window signal is emitted when a detachable
   * tab is dropped on the root window.
   *
   * A handler for this signal can create a window containing
   * a notebook where the tab will be attached. It is also
   * responsible for moving/resizing the window and adding the
   * necessary properties to the notebook (e.g. the
   * `BobguiNotebook`:group-name ).
   *
   * Returns: (nullable) (transfer none): a `BobguiNotebook` that
   *   @page should be added to
   */
  notebook_signals[CREATE_WINDOW] =
    g_signal_new (I_("create-window"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BobguiNotebookClass, create_window),
                  bobgui_object_handled_accumulator, NULL,
                  _bobgui_marshal_OBJECT__OBJECT,
                  BOBGUI_TYPE_NOTEBOOK, 1,
                  BOBGUI_TYPE_WIDGET);
  g_signal_set_va_marshaller (notebook_signals[CREATE_WINDOW],
                              G_TYPE_FROM_CLASS (gobject_class),
                              _bobgui_marshal_OBJECT__OBJECTv);

  /**
   * BobguiNotebook|menu.popup:
   *
   * Opens the context menu.
   */
  bobgui_widget_class_install_action (widget_class, "menu.popup", NULL, bobgui_notebook_popup_menu);

  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_space, 0,
                                       "select-page",
                                       "(b)", FALSE);
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_KP_Space, 0,
                                       "select-page",
                                       "(b)", FALSE);

  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_Home, 0,
                                       "focus-tab",
                                       "(i)", BOBGUI_NOTEBOOK_TAB_FIRST);
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_KP_Home, 0,
                                       "focus-tab",
                                       "(i)", BOBGUI_NOTEBOOK_TAB_FIRST);
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_End, 0,
                                       "focus-tab",
                                       "(i)", BOBGUI_NOTEBOOK_TAB_LAST);
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_KP_End, 0,
                                       "focus-tab",
                                       "(i)", BOBGUI_NOTEBOOK_TAB_LAST);

  bobgui_widget_class_add_binding_action (widget_class,
                                       GDK_KEY_F10, GDK_SHIFT_MASK,
                                       "menu.popup",
                                       NULL);
  bobgui_widget_class_add_binding_action (widget_class,
                                       GDK_KEY_Menu, 0,
                                       "menu.popup",
                                       NULL);

  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_Page_Up, GDK_CONTROL_MASK,
                                       "change-current-page",
                                       "(i)", -1);
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_Page_Down, GDK_CONTROL_MASK,
                                       "change-current-page",
                                       "(i)", 1);

  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_Page_Up, GDK_CONTROL_MASK | GDK_ALT_MASK,
                                       "change-current-page",
                                       "(i)", -1);
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_Page_Down, GDK_CONTROL_MASK | GDK_ALT_MASK,
                                       "change-current-page",
                                       "(i)", 1);

  add_arrow_bindings (widget_class, GDK_KEY_Up, BOBGUI_DIR_UP);
  add_arrow_bindings (widget_class, GDK_KEY_Down, BOBGUI_DIR_DOWN);
  add_arrow_bindings (widget_class, GDK_KEY_Left, BOBGUI_DIR_LEFT);
  add_arrow_bindings (widget_class, GDK_KEY_Right, BOBGUI_DIR_RIGHT);

  add_reorder_bindings (widget_class, GDK_KEY_Up, BOBGUI_DIR_UP, FALSE);
  add_reorder_bindings (widget_class, GDK_KEY_Down, BOBGUI_DIR_DOWN, FALSE);
  add_reorder_bindings (widget_class, GDK_KEY_Left, BOBGUI_DIR_LEFT, FALSE);
  add_reorder_bindings (widget_class, GDK_KEY_Right, BOBGUI_DIR_RIGHT, FALSE);
  add_reorder_bindings (widget_class, GDK_KEY_Home, BOBGUI_DIR_LEFT, TRUE);
  add_reorder_bindings (widget_class, GDK_KEY_Home, BOBGUI_DIR_UP, TRUE);
  add_reorder_bindings (widget_class, GDK_KEY_End, BOBGUI_DIR_RIGHT, TRUE);
  add_reorder_bindings (widget_class, GDK_KEY_End, BOBGUI_DIR_DOWN, TRUE);

  add_tab_bindings (widget_class, GDK_CONTROL_MASK, BOBGUI_DIR_TAB_FORWARD);
  add_tab_bindings (widget_class, GDK_CONTROL_MASK | GDK_SHIFT_MASK, BOBGUI_DIR_TAB_BACKWARD);

  bobgui_widget_class_set_layout_manager_type (widget_class, BOBGUI_TYPE_BOX_LAYOUT);
  bobgui_widget_class_set_css_name (widget_class, I_("notebook"));
  bobgui_widget_class_set_accessible_role (widget_class, BOBGUI_ACCESSIBLE_ROLE_GROUP);
}

static void
bobgui_notebook_init (BobguiNotebook *notebook)
{
  BobguiEventController *controller;
  BobguiGesture *gesture;
  BobguiLayoutManager *layout;
  BobguiDropTarget *dest;

  notebook->cur_page = NULL;
  notebook->children = NULL;
  notebook->first_tab = NULL;
  notebook->focus_tab = NULL;
  notebook->menu = NULL;

  notebook->show_tabs = TRUE;
  notebook->show_border = TRUE;
  notebook->tab_pos = BOBGUI_POS_TOP;
  notebook->scrollable = FALSE;
  notebook->click_child = ARROW_NONE;
  notebook->need_timer = 0;
  notebook->child_has_focus = FALSE;
  notebook->focus_out = FALSE;

  notebook->group = 0;
  notebook->pressed_button = 0;
  notebook->dnd_timer = 0;
  notebook->operation = DRAG_OPERATION_NONE;
  notebook->detached_tab = NULL;
  notebook->has_scrolled = FALSE;

  bobgui_widget_set_focusable (BOBGUI_WIDGET (notebook), TRUE);

  notebook->header_widget = g_object_new (BOBGUI_TYPE_BOX,
                                          "css-name", "header",
                                          NULL);
  bobgui_widget_add_css_class (notebook->header_widget, "top");
  bobgui_widget_set_visible (notebook->header_widget, FALSE);
  bobgui_widget_set_parent (notebook->header_widget, BOBGUI_WIDGET (notebook));

  notebook->tabs_widget = bobgui_gizmo_new_with_role ("tabs",
                                                   BOBGUI_ACCESSIBLE_ROLE_TAB_LIST,
                                                   bobgui_notebook_measure_tabs,
                                                   bobgui_notebook_allocate_tabs,
                                                   bobgui_notebook_snapshot_tabs,
                                                   NULL,
                                                   (BobguiGizmoFocusFunc)bobgui_widget_focus_self,
                                                   (BobguiGizmoGrabFocusFunc)bobgui_widget_grab_focus_self);
  bobgui_widget_set_hexpand (notebook->tabs_widget, TRUE);
  bobgui_box_append (BOBGUI_BOX (notebook->header_widget), notebook->tabs_widget);

  notebook->stack_widget = bobgui_stack_new ();
  bobgui_widget_set_hexpand (notebook->stack_widget, TRUE);
  bobgui_widget_set_vexpand (notebook->stack_widget, TRUE);
  bobgui_widget_set_parent (notebook->stack_widget, BOBGUI_WIDGET (notebook));

  dest = bobgui_drop_target_new (BOBGUI_TYPE_NOTEBOOK_PAGE, GDK_ACTION_MOVE);
  bobgui_drop_target_set_preload (dest, TRUE);
  g_signal_connect (dest, "motion", G_CALLBACK (bobgui_notebook_drag_motion), notebook);
  g_signal_connect (dest, "drop", G_CALLBACK (bobgui_notebook_drag_drop), notebook);
  bobgui_widget_add_controller (BOBGUI_WIDGET (notebook->tabs_widget), BOBGUI_EVENT_CONTROLLER (dest));

  gesture = bobgui_gesture_click_new ();
  bobgui_gesture_single_set_button (BOBGUI_GESTURE_SINGLE (gesture), 0);
  bobgui_event_controller_set_propagation_phase (BOBGUI_EVENT_CONTROLLER (gesture), BOBGUI_PHASE_CAPTURE);
  g_signal_connect (gesture, "pressed", G_CALLBACK (bobgui_notebook_gesture_pressed), notebook);
  g_signal_connect (gesture, "released", G_CALLBACK (bobgui_notebook_gesture_released), notebook);
  g_signal_connect (gesture, "cancel", G_CALLBACK (bobgui_notebook_gesture_cancel), notebook);
  bobgui_widget_add_controller (BOBGUI_WIDGET (notebook), BOBGUI_EVENT_CONTROLLER (gesture));

  controller = bobgui_event_controller_motion_new ();
  g_signal_connect (controller, "motion", G_CALLBACK (bobgui_notebook_motion), notebook);
  bobgui_widget_add_controller (BOBGUI_WIDGET (notebook), controller);

  bobgui_widget_add_css_class (BOBGUI_WIDGET (notebook), "frame");

  layout = bobgui_widget_get_layout_manager (BOBGUI_WIDGET (notebook));
  bobgui_orientable_set_orientation (BOBGUI_ORIENTABLE (layout), BOBGUI_ORIENTATION_VERTICAL);
}

static BobguiBuildableIface *parent_buildable_iface;

static void
bobgui_notebook_buildable_init (BobguiBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);

  iface->add_child = bobgui_notebook_buildable_add_child;
}

static void
bobgui_notebook_buildable_add_child (BobguiBuildable  *buildable,
                                  BobguiBuilder    *builder,
                                  GObject       *child,
                                  const char    *type)
{
  BobguiNotebook *notebook = BOBGUI_NOTEBOOK (buildable);

  if (BOBGUI_IS_NOTEBOOK_PAGE (child))
    {
      bobgui_notebook_insert_notebook_page (notebook, BOBGUI_NOTEBOOK_PAGE (child), -1);
    }
  else if (BOBGUI_IS_WIDGET (child))
    {
      if (type && strcmp (type, "tab") == 0)
        {
          BobguiWidget * page;

          page = bobgui_notebook_get_nth_page (notebook, -1);
          /* To set the tab label widget, we must have already a child
           * inside the tab container. */
          g_assert (page != NULL);
          /* warn when Glade tries to overwrite label */
          if (bobgui_notebook_get_tab_label (notebook, page))
            g_warning ("Overriding tab label for notebook");
          bobgui_notebook_set_tab_label (notebook, page, BOBGUI_WIDGET (child));
        }
      else if (type && strcmp (type, "action-start") == 0)
        {
          bobgui_notebook_set_action_widget (notebook, BOBGUI_WIDGET (child), BOBGUI_PACK_START);
        }
      else if (type && strcmp (type, "action-end") == 0)
        {
          bobgui_notebook_set_action_widget (notebook, BOBGUI_WIDGET (child), BOBGUI_PACK_END);
        }
      else if (!type)
        bobgui_notebook_append_page (notebook, BOBGUI_WIDGET (child), NULL);
      else
        BOBGUI_BUILDER_WARN_INVALID_CHILD_TYPE (notebook, type);
    }
  else
    {
      parent_buildable_iface->add_child (buildable, builder, child, type);
    }
}

static gboolean
bobgui_notebook_has_current_page (BobguiNotebook *notebook)
{
  return notebook->cur_page &&
         bobgui_widget_get_visible (notebook->cur_page->child);
}

static gboolean
bobgui_notebook_select_page (BobguiNotebook *notebook,
                          gboolean     move_focus)
{
  if (bobgui_widget_is_focus (BOBGUI_WIDGET (notebook)) && notebook->show_tabs)
    {
      bobgui_notebook_page_select (notebook, move_focus);
      return TRUE;
    }
  else
    return FALSE;
}

static gboolean
bobgui_notebook_focus_tab (BobguiNotebook       *notebook,
                        BobguiNotebookTab     type)
{
  GList *list;

  if (bobgui_widget_is_focus (BOBGUI_WIDGET (notebook)) && notebook->show_tabs)
    {
      switch (type)
        {
        case BOBGUI_NOTEBOOK_TAB_FIRST:
          list = bobgui_notebook_search_page (notebook, NULL, STEP_NEXT, TRUE);
          if (list)
            bobgui_notebook_switch_focus_tab (notebook, list);
          break;
        case BOBGUI_NOTEBOOK_TAB_LAST:
          list = bobgui_notebook_search_page (notebook, NULL, STEP_PREV, TRUE);
          if (list)
            bobgui_notebook_switch_focus_tab (notebook, list);
          break;

        default:
          break;
        }

      return TRUE;
    }
  else
    return FALSE;
}

static gboolean
bobgui_notebook_change_current_page (BobguiNotebook *notebook,
                                  int          offset)
{
  GList *current = NULL;

  if (!notebook->show_tabs)
    return FALSE;

  if (notebook->cur_page)
    current = g_list_find (notebook->children, notebook->cur_page);

  while (offset != 0)
    {
      current = bobgui_notebook_search_page (notebook, current,
                                          offset < 0 ? STEP_PREV : STEP_NEXT,
                                          TRUE);

      if (!current)
        {
          current = bobgui_notebook_search_page (notebook, NULL,
                                              offset < 0 ? STEP_PREV : STEP_NEXT,
                                              TRUE);
        }

      offset += offset < 0 ? 1 : -1;
    }

  if (current)
    bobgui_notebook_switch_page (notebook, current->data);
  else
    bobgui_widget_error_bell (BOBGUI_WIDGET (notebook));

  return TRUE;
}

static BobguiDirectionType
get_effective_direction (BobguiNotebook      *notebook,
                         BobguiDirectionType  direction)
{
  /* Remap the directions into the effective direction it would be for a
   * BOBGUI_POS_TOP notebook
   */

#define D(rest) BOBGUI_DIR_##rest

  static const BobguiDirectionType translate_direction[2][4][6] = {
    /* LEFT */   {{ D(TAB_FORWARD),  D(TAB_BACKWARD), D(LEFT), D(RIGHT), D(UP),   D(DOWN) },
    /* RIGHT */  { D(TAB_BACKWARD), D(TAB_FORWARD),  D(LEFT), D(RIGHT), D(DOWN), D(UP)   },
    /* TOP */    { D(TAB_FORWARD),  D(TAB_BACKWARD), D(UP),   D(DOWN),  D(LEFT), D(RIGHT) },
    /* BOTTOM */ { D(TAB_BACKWARD), D(TAB_FORWARD),  D(DOWN), D(UP),    D(LEFT), D(RIGHT) }},
    /* LEFT */  {{ D(TAB_BACKWARD), D(TAB_FORWARD),  D(LEFT), D(RIGHT), D(DOWN), D(UP)   },
    /* RIGHT */  { D(TAB_FORWARD),  D(TAB_BACKWARD), D(LEFT), D(RIGHT), D(UP),   D(DOWN) },
    /* TOP */    { D(TAB_FORWARD),  D(TAB_BACKWARD), D(UP),   D(DOWN),  D(RIGHT), D(LEFT) },
    /* BOTTOM */ { D(TAB_BACKWARD), D(TAB_FORWARD),  D(DOWN), D(UP),    D(RIGHT), D(LEFT) }},
  };

#undef D

  int text_dir = bobgui_widget_get_direction (BOBGUI_WIDGET (notebook)) == BOBGUI_TEXT_DIR_RTL ? 1 : 0;

  return translate_direction[text_dir][notebook->tab_pos][direction];
}

static BobguiPositionType
get_effective_tab_pos (BobguiNotebook *notebook)
{
  if (bobgui_widget_get_direction (BOBGUI_WIDGET (notebook)) == BOBGUI_TEXT_DIR_RTL)
    {
      switch (notebook->tab_pos)
        {
        case BOBGUI_POS_LEFT:
          return BOBGUI_POS_RIGHT;
        case BOBGUI_POS_RIGHT:
          return BOBGUI_POS_LEFT;
        default: ;
        }
    }

  return notebook->tab_pos;
}

static void
bobgui_notebook_move_focus_out (BobguiNotebook      *notebook,
                             BobguiDirectionType  direction_type)
{
  BobguiDirectionType effective_direction = get_effective_direction (notebook, direction_type);
  BobguiWidget *toplevel;

  if (bobgui_widget_get_focus_child (BOBGUI_WIDGET (notebook)) && effective_direction == BOBGUI_DIR_UP)
    if (focus_tabs_in (notebook))
      return;
  if (bobgui_widget_is_focus (BOBGUI_WIDGET (notebook)) && effective_direction == BOBGUI_DIR_DOWN)
    if (focus_child_in (notebook, BOBGUI_DIR_TAB_FORWARD))
      return;

  /* At this point, we know we should be focusing out of the notebook entirely. We
   * do this by setting a flag, then propagating the focus motion to the notebook.
   */
  toplevel = BOBGUI_WIDGET (bobgui_widget_get_root (BOBGUI_WIDGET (notebook)));
  if (!BOBGUI_IS_ROOT (toplevel))
    return;

  g_object_ref (notebook);

  notebook->focus_out = TRUE;
  g_signal_emit_by_name (toplevel, "move-focus", direction_type);
  notebook->focus_out = FALSE;

  g_object_unref (notebook);
}

static int
reorder_tab (BobguiNotebook *notebook, GList *position, GList *tab)
{
  GList *elem;

  if (position == tab)
    return g_list_position (notebook->children, tab);

  /* check that we aren't inserting the tab in the
   * same relative position, taking packing into account */
  elem = (position) ? position->prev : g_list_last (notebook->children);

  if (elem == tab)
    return g_list_position (notebook->children, tab);

  /* now actually reorder the tab */
  if (notebook->first_tab == tab)
    notebook->first_tab = bobgui_notebook_search_page (notebook, notebook->first_tab,
                                                    STEP_NEXT, TRUE);

  notebook->children = g_list_remove_link (notebook->children, tab);

  if (!position)
    elem = g_list_last (notebook->children);
  else
    {
      elem = position->prev;
      position->prev = tab;
    }

  if (elem)
    elem->next = tab;
  else
    notebook->children = tab;

  tab->prev = elem;
  tab->next = position;

  return g_list_position (notebook->children, tab);
}

static gboolean
bobgui_notebook_reorder_tab (BobguiNotebook      *notebook,
                          BobguiDirectionType  direction_type,
                          gboolean          move_to_last)
{
  BobguiDirectionType effective_direction = get_effective_direction (notebook, direction_type);
  GList *last, *child, *element;
  int page_num, old_page_num, i;

  if (!bobgui_widget_is_focus (BOBGUI_WIDGET (notebook)) || !notebook->show_tabs)
    return FALSE;

  if (!bobgui_notebook_has_current_page (notebook) ||
      !notebook->cur_page->reorderable)
    return FALSE;

  if (effective_direction != BOBGUI_DIR_LEFT &&
      effective_direction != BOBGUI_DIR_RIGHT)
    return FALSE;

  if (move_to_last)
    {
      child = notebook->focus_tab;

      do
        {
          last = child;
          child = bobgui_notebook_search_page (notebook, last,
                                            (effective_direction == BOBGUI_DIR_RIGHT) ? STEP_NEXT : STEP_PREV,
                                            TRUE);
        }
      while (child);

      child = last;
    }
  else
    child = bobgui_notebook_search_page (notebook, notebook->focus_tab,
                                      (effective_direction == BOBGUI_DIR_RIGHT) ? STEP_NEXT : STEP_PREV,
                                      TRUE);

  if (!child || child->data == notebook->cur_page)
    return FALSE;

  old_page_num = g_list_position (notebook->children, notebook->focus_tab);
  if (effective_direction == BOBGUI_DIR_RIGHT)
    page_num = reorder_tab (notebook, child->next, notebook->focus_tab);
  else
    page_num = reorder_tab (notebook, child, notebook->focus_tab);

  bobgui_notebook_child_reordered (notebook, notebook->focus_tab->data);
  for (element = notebook->children, i = 0; element; element = element->next, i++)
    {
      if (MIN (old_page_num, page_num) <= i && i <= MAX (old_page_num, page_num))
        g_object_notify (G_OBJECT (element->data), "position");
    }
  g_signal_emit (notebook,
                 notebook_signals[PAGE_REORDERED],
                 0,
                 ((BobguiNotebookPage *) notebook->focus_tab->data)->child,
                 page_num);

  return TRUE;
}

/**
 * bobgui_notebook_new:
 *
 * Creates a new `BobguiNotebook` widget with no pages.

 * Returns: the newly created `BobguiNotebook`
 */
BobguiWidget*
bobgui_notebook_new (void)
{
  return g_object_new (BOBGUI_TYPE_NOTEBOOK, NULL);
}

/* Private GObject Methods :
 *
 * bobgui_notebook_set_property
 * bobgui_notebook_get_property
 */
static void
bobgui_notebook_set_property (GObject         *object,
                           guint            prop_id,
                           const GValue    *value,
                           GParamSpec      *pspec)
{
  BobguiNotebook *notebook;

  notebook = BOBGUI_NOTEBOOK (object);

  switch (prop_id)
    {
    case PROP_SHOW_TABS:
      bobgui_notebook_set_show_tabs (notebook, g_value_get_boolean (value));
      break;
    case PROP_SHOW_BORDER:
      bobgui_notebook_set_show_border (notebook, g_value_get_boolean (value));
      break;
    case PROP_SCROLLABLE:
      bobgui_notebook_set_scrollable (notebook, g_value_get_boolean (value));
      break;
    case PROP_ENABLE_POPUP:
      if (g_value_get_boolean (value))
        bobgui_notebook_popup_enable (notebook);
      else
        bobgui_notebook_popup_disable (notebook);
      break;
    case PROP_PAGE:
      bobgui_notebook_set_current_page (notebook, g_value_get_int (value));
      break;
    case PROP_TAB_POS:
      bobgui_notebook_set_tab_pos (notebook, g_value_get_enum (value));
      break;
    case PROP_GROUP_NAME:
      bobgui_notebook_set_group_name (notebook, g_value_get_string (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_notebook_get_property (GObject         *object,
                           guint            prop_id,
                           GValue          *value,
                           GParamSpec      *pspec)
{
  BobguiNotebook *notebook = BOBGUI_NOTEBOOK (object);

  switch (prop_id)
    {
    case PROP_SHOW_TABS:
      g_value_set_boolean (value, notebook->show_tabs);
      break;
    case PROP_SHOW_BORDER:
      g_value_set_boolean (value, notebook->show_border);
      break;
    case PROP_SCROLLABLE:
      g_value_set_boolean (value, notebook->scrollable);
      break;
    case PROP_ENABLE_POPUP:
      g_value_set_boolean (value, notebook->menu != NULL);
      break;
    case PROP_PAGE:
      g_value_set_int (value, bobgui_notebook_get_current_page (notebook));
      break;
    case PROP_TAB_POS:
      g_value_set_enum (value, notebook->tab_pos);
      break;
    case PROP_GROUP_NAME:
      g_value_set_string (value, bobgui_notebook_get_group_name (notebook));
      break;
    case PROP_PAGES:
      g_value_take_object (value, bobgui_notebook_get_pages (notebook));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

/* Private BobguiWidget Methods :
 *
 * bobgui_notebook_map
 * bobgui_notebook_unmap
 * bobgui_notebook_snapshot
 * bobgui_notebook_popup_menu
 * bobgui_notebook_drag_begin
 * bobgui_notebook_drag_end
 * bobgui_notebook_drag_failed
 * bobgui_notebook_drag_motion
 * bobgui_notebook_drag_drop
 * bobgui_notebook_drag_data_get
 */

static void
bobgui_notebook_finalize (GObject *object)
{
  BobguiNotebook *notebook = BOBGUI_NOTEBOOK (object);

  bobgui_widget_unparent (notebook->header_widget);
  bobgui_widget_unparent (notebook->stack_widget);

  G_OBJECT_CLASS (bobgui_notebook_parent_class)->finalize (object);
}

static void
bobgui_notebook_dispose (GObject *object)
{
  BobguiNotebook *notebook = BOBGUI_NOTEBOOK (object);
  GList *l = notebook->children;

  if (notebook->pages)
    g_list_model_items_changed (G_LIST_MODEL (notebook->pages), 0, g_list_length (notebook->children), 0);

  while (l != NULL)
    {
      BobguiNotebookPage *page = l->data;
      l = l->next;

      bobgui_notebook_remove (notebook, page->child);
    }

  G_OBJECT_CLASS (bobgui_notebook_parent_class)->dispose (object);
}

static gboolean
bobgui_notebook_get_tab_area_position (BobguiNotebook     *notebook,
                                    graphene_rect_t *rectangle)
{
  if (notebook->show_tabs && bobgui_notebook_has_current_page (notebook))
    {
      return bobgui_widget_compute_bounds (notebook->header_widget,
                                        BOBGUI_WIDGET (notebook),
                                        rectangle);
    }
  else
    {
      graphene_rect_init_from_rect (rectangle, graphene_rect_zero ());
    }

  return FALSE;
}

static void
bobgui_notebook_unmap (BobguiWidget *widget)
{
  BobguiNotebook *notebook = BOBGUI_NOTEBOOK (widget);

  stop_scrolling (notebook);

  BOBGUI_WIDGET_CLASS (bobgui_notebook_parent_class)->unmap (widget);
}

static void
bobgui_notebook_distribute_arrow_width (BobguiNotebook *notebook,
                                     BobguiPackType  type,
                                     int          size,
                                     int         *out_left,
                                     int         *out_right)
{
  BobguiRequestedSize sizes[2];

  if (notebook->arrow_widget[2 * type + 1] == NULL)
    {
      if (notebook->arrow_widget[2 * type] == NULL)
        *out_left = 0;
      else
        *out_left = size;
      *out_right = 0;
    }
  else if (notebook->arrow_widget[2 * type] == NULL)
    {
      *out_left = 0;
      *out_right = size;
    }
  else
    {
      bobgui_widget_measure (notebook->arrow_widget[2 * type],
                          BOBGUI_ORIENTATION_HORIZONTAL,
                          -1,
                          &sizes[0].minimum_size, &sizes[0].natural_size,
                          NULL, NULL);
      bobgui_widget_measure (notebook->arrow_widget[2 * type + 1],
                          BOBGUI_ORIENTATION_HORIZONTAL,
                          -1,
                          &sizes[1].minimum_size, &sizes[1].natural_size,
                          NULL, NULL);

      size -= sizes[0].minimum_size + sizes[1].minimum_size;
      size = bobgui_distribute_natural_allocation (size, G_N_ELEMENTS (sizes), sizes);

      *out_left = sizes[0].minimum_size + size / 2;
      *out_right = sizes[1].minimum_size + (size + 1) / 2;
    }
}

static void
bobgui_notebook_measure_arrows (BobguiNotebook    *notebook,
                             BobguiPackType     type,
                             BobguiOrientation  orientation,
                             int             for_size,
                             int            *minimum,
                             int            *natural,
                             int            *minimum_baseline,
                             int            *natural_baseline)
{
  int child1_min, child1_nat;
  int child2_min, child2_nat;

  if (orientation == BOBGUI_ORIENTATION_HORIZONTAL)
    {
      if (notebook->arrow_widget[2 * type])
        {
          bobgui_widget_measure (notebook->arrow_widget[2 * type],
                              orientation,
                              for_size,
                              &child1_min, &child1_nat,
                              NULL, NULL);
        }
      else
        {
          child1_min = child1_nat = 0;
        }
      if (notebook->arrow_widget[2 * type + 1])
        {
          bobgui_widget_measure (notebook->arrow_widget[2 * type + 1],
                              orientation,
                              for_size,
                              &child2_min, &child2_nat,
                              NULL, NULL);
        }
      else
        {
          child2_min = child2_nat = 0;
        }
      *minimum = child1_min + child2_min;
      *natural = child1_nat + child2_nat;
      if (minimum_baseline)
        *minimum_baseline = -1;
      if (natural_baseline)
        *natural_baseline = -1;
    }
  else
    {
      int child1_size, child2_size;

      if (for_size > -1)
        bobgui_notebook_distribute_arrow_width (notebook, type, for_size, &child1_size, &child2_size);
      else
        child1_size = child2_size = for_size;

      if (notebook->arrow_widget[2 * type])
        {
          bobgui_widget_measure (notebook->arrow_widget[2 * type],
                              orientation,
                              child1_size,
                              &child1_min, &child1_nat,
                              NULL, NULL);
        }
      else
        {
          child1_min = child1_nat = 0;
        }
      if (notebook->arrow_widget[2 * type + 1])
        {
          bobgui_widget_measure (notebook->arrow_widget[2 * type + 1],
                              orientation,
                              child2_size,
                              &child2_min, &child2_nat,
                              NULL, NULL);
        }
      else
        {
          child2_min = child2_nat = 0;
        }
      *minimum = MAX (child1_min, child2_min);
      *natural = MAX (child1_nat, child2_nat);
      if (minimum_baseline)
        *minimum_baseline = -1;
      if (natural_baseline)
        *natural_baseline = -1;
    }
}

static void
bobgui_notebook_get_preferred_tabs_size (BobguiNotebook    *notebook,
                                      BobguiRequisition *requisition)
{
  int tab_width = 0;
  int tab_height = 0;
  int tab_max = 0;
  guint vis_pages = 0;
  GList *children;
  BobguiNotebookPage *page;


  for (children = notebook->children; children;
       children = children->next)
    {
      page = children->data;

      if (bobgui_widget_get_visible (page->child))
        {
          vis_pages++;

          if (!bobgui_widget_get_visible (page->tab_label))
            bobgui_widget_set_visible (page->tab_label, TRUE);

          bobgui_widget_measure (page->tab_widget,
                              BOBGUI_ORIENTATION_HORIZONTAL,
                              -1,
                              &page->requisition.width, NULL,
                              NULL, NULL);
          bobgui_widget_measure (page->tab_widget,
                              BOBGUI_ORIENTATION_VERTICAL,
                              page->requisition.width,
                              &page->requisition.height, NULL,
                              NULL, NULL);

          switch (notebook->tab_pos)
            {
            case BOBGUI_POS_TOP:
            case BOBGUI_POS_BOTTOM:
              tab_height = MAX (tab_height, page->requisition.height);
              tab_max = MAX (tab_max, page->requisition.width);
              break;
            case BOBGUI_POS_LEFT:
            case BOBGUI_POS_RIGHT:
              tab_width = MAX (tab_width, page->requisition.width);
              tab_max = MAX (tab_max, page->requisition.height);
              break;
            default:
              g_assert_not_reached ();
              break;
            }
        }
      else if (bobgui_widget_get_visible (page->tab_label))
        bobgui_widget_set_visible (page->tab_label, FALSE);
    }

  children = notebook->children;

  if (vis_pages)
    {
      switch (notebook->tab_pos)
        {
        case BOBGUI_POS_TOP:
        case BOBGUI_POS_BOTTOM:
          if (tab_height == 0)
            break;

          if (notebook->scrollable)
            {
              int arrow_height, unused;
              bobgui_notebook_measure_arrows (notebook,
                                           BOBGUI_PACK_START,
                                           BOBGUI_ORIENTATION_VERTICAL,
                                           -1,
                                           &arrow_height, &unused,
                                           NULL, NULL);
              tab_height = MAX (tab_height, arrow_height);
              bobgui_notebook_measure_arrows (notebook,
                                           BOBGUI_PACK_END,
                                           BOBGUI_ORIENTATION_VERTICAL,
                                           -1,
                                           &arrow_height, &unused,
                                           NULL, NULL);
              tab_height = MAX (tab_height, arrow_height);
            }

          while (children)
            {
              page = children->data;
              children = children->next;

              if (!bobgui_widget_get_visible (page->child))
                continue;

              tab_width += page->requisition.width;
              page->requisition.height = tab_height;
            }

          if (notebook->scrollable)
            {
              int start_arrow_width, end_arrow_width, unused;

              bobgui_notebook_measure_arrows (notebook,
                                           BOBGUI_PACK_START,
                                           BOBGUI_ORIENTATION_HORIZONTAL,
                                           tab_height,
                                           &start_arrow_width, &unused,
                                           NULL, NULL);
              bobgui_notebook_measure_arrows (notebook,
                                           BOBGUI_PACK_END,
                                           BOBGUI_ORIENTATION_HORIZONTAL,
                                           tab_height,
                                           &end_arrow_width, &unused,
                                           NULL, NULL);
              tab_width = MIN (tab_width,
                               tab_max + start_arrow_width + end_arrow_width);
            }

          requisition->width = tab_width;

          requisition->height = tab_height;
          break;
        case BOBGUI_POS_LEFT:
        case BOBGUI_POS_RIGHT:
          if (tab_width == 0)
            break;

          if (notebook->scrollable)
            {
              int arrow_width, unused;
              bobgui_notebook_measure_arrows (notebook,
                                           BOBGUI_PACK_START,
                                           BOBGUI_ORIENTATION_HORIZONTAL,
                                           -1,
                                           &arrow_width, &unused,
                                           NULL, NULL);
              tab_width = MAX (tab_width, arrow_width);
              bobgui_notebook_measure_arrows (notebook,
                                           BOBGUI_PACK_END,
                                           BOBGUI_ORIENTATION_HORIZONTAL,
                                           -1,
                                           &arrow_width, &unused,
                                           NULL, NULL);
              tab_width = MAX (tab_width, arrow_width);
            }

          while (children)
            {
              page = children->data;
              children = children->next;

              if (!bobgui_widget_get_visible (page->child))
                continue;

              page->requisition.width = tab_width;

              tab_height += page->requisition.height;
            }

          if (notebook->scrollable)
            {
              int start_arrow_height, end_arrow_height, unused;

              bobgui_notebook_measure_arrows (notebook,
                                           BOBGUI_PACK_START,
                                           BOBGUI_ORIENTATION_VERTICAL,
                                           tab_width,
                                           &start_arrow_height, &unused,
                                           NULL, NULL);
              bobgui_notebook_measure_arrows (notebook,
                                           BOBGUI_PACK_END,
                                           BOBGUI_ORIENTATION_VERTICAL,
                                           tab_width,
                                           &end_arrow_height, &unused,
                                           NULL, NULL);
              tab_height = MIN (tab_height, tab_max + start_arrow_height + end_arrow_height);
            }

          requisition->height = tab_height;
          requisition->height = MAX (requisition->height, tab_max);

          requisition->width = tab_width;
          break;
        default:
          g_assert_not_reached ();
          requisition->width = 0;
          requisition->height = 0;
        }
    }
  else
    {
      requisition->width = 0;
      requisition->height = 0;
    }
}

static void
bobgui_notebook_measure_tabs (BobguiGizmo       *gizmo,
                           BobguiOrientation  orientation,
                           int             size,
                           int            *minimum,
                           int            *natural,
                           int            *minimum_baseline,
                           int            *natural_baseline)
{
  BobguiWidget *widget = bobgui_widget_get_parent (BOBGUI_WIDGET (gizmo));
  BobguiNotebook *notebook = BOBGUI_NOTEBOOK (bobgui_widget_get_parent (widget));
  BobguiRequisition tabs_requisition = { 0 };

  bobgui_notebook_get_preferred_tabs_size (notebook, &tabs_requisition);
  if (orientation == BOBGUI_ORIENTATION_HORIZONTAL)
    {
      *minimum = tabs_requisition.width;
      *natural = tabs_requisition.width;
    }
  else
    {
      *minimum = tabs_requisition.height;
      *natural = tabs_requisition.height;
    }
}

static void
bobgui_notebook_allocate_tabs (BobguiGizmo *gizmo,
                            int       width,
                            int       height,
                            int       baseline)
{
  BobguiWidget *widget = bobgui_widget_get_parent (BOBGUI_WIDGET (gizmo));
  BobguiNotebook *notebook = BOBGUI_NOTEBOOK (bobgui_widget_get_parent (widget));

  bobgui_notebook_pages_allocate (notebook, width, height);
}

static gboolean
bobgui_notebook_show_arrows (BobguiNotebook *notebook)
{
  GList *children;

  if (!notebook->scrollable)
    return FALSE;

  children = notebook->children;
  while (children)
    {
      BobguiNotebookPage *page = children->data;

      if (!bobgui_widget_get_child_visible (page->tab_widget))
        return TRUE;

      children = children->next;
    }

  return FALSE;
}

static BobguiNotebookArrow
bobgui_notebook_get_arrow (BobguiNotebook *notebook,
                        int          x,
                        int          y)
{
  int i;

  if (bobgui_notebook_show_arrows (notebook))
    {
      for (i = 0; i < 4; i++)
        {
          graphene_rect_t arrow_bounds;

          if (notebook->arrow_widget[i] == NULL)
            continue;

          if (!bobgui_widget_compute_bounds (notebook->arrow_widget[i],
                                          BOBGUI_WIDGET (notebook),
                                          &arrow_bounds))
            continue;

          if (graphene_rect_contains_point (&arrow_bounds,
                                            &(graphene_point_t){x, y}))
            return i;
        }
    }

  return ARROW_NONE;
}

static void
bobgui_notebook_do_arrow (BobguiNotebook     *notebook,
                       BobguiNotebookArrow arrow)
{
  BobguiWidget *widget = BOBGUI_WIDGET (notebook);
  gboolean is_rtl, left;

  is_rtl = bobgui_widget_get_direction (widget) == BOBGUI_TEXT_DIR_RTL;
  left = (ARROW_IS_LEFT (arrow) && !is_rtl) ||
         (!ARROW_IS_LEFT (arrow) && is_rtl);

  if (!notebook->focus_tab ||
      bobgui_notebook_search_page (notebook, notebook->focus_tab,
                                left ? STEP_PREV : STEP_NEXT,
                                TRUE))
    {
      bobgui_notebook_change_current_page (notebook, left ? -1 : 1);
      bobgui_widget_grab_focus (widget);
    }
}

static gboolean
bobgui_notebook_arrow_button_press (BobguiNotebook      *notebook,
                                 BobguiNotebookArrow  arrow,
                                 int               button)
{
  BobguiWidget *widget = BOBGUI_WIDGET (notebook);
  gboolean is_rtl = bobgui_widget_get_direction (widget) == BOBGUI_TEXT_DIR_RTL;
  gboolean left = (ARROW_IS_LEFT (arrow) && !is_rtl) ||
                  (!ARROW_IS_LEFT (arrow) && is_rtl);

  if (notebook->pressed_button)
    return FALSE;

  if (!bobgui_widget_has_focus (widget))
    bobgui_widget_grab_focus (widget);

  notebook->pressed_button = button;
  notebook->click_child = arrow;

  if (button == GDK_BUTTON_PRIMARY)
    {
      bobgui_notebook_do_arrow (notebook, arrow);
      bobgui_notebook_set_scroll_timer (notebook);
    }
  else if (button == GDK_BUTTON_MIDDLE)
    bobgui_notebook_page_select (notebook, TRUE);
  else if (button == GDK_BUTTON_SECONDARY)
    bobgui_notebook_switch_focus_tab (notebook,
                                   bobgui_notebook_search_page (notebook,
                                                             NULL,
                                                             left ? STEP_NEXT : STEP_PREV,
                                                             TRUE));

  return TRUE;
}

static gboolean
bobgui_notebook_page_tab_label_is_visible (BobguiNotebookPage *page)
{
  return page->tab_label &&
         bobgui_widget_get_visible (page->tab_widget) &&
         bobgui_widget_get_child_visible (page->tab_widget) &&
         bobgui_widget_get_visible (page->tab_label) &&
         bobgui_widget_get_child_visible (page->tab_label);
}

static gboolean
in_tabs (BobguiNotebook *notebook,
         double       x,
         double       y)
{
  graphene_rect_t tabs_bounds;

  if (!bobgui_widget_compute_bounds (notebook->tabs_widget, BOBGUI_WIDGET (notebook), &tabs_bounds))
    return FALSE;

  return graphene_rect_contains_point (&tabs_bounds,
                                       &(graphene_point_t){x, y});
}

static GList*
get_tab_at_pos (BobguiNotebook *notebook,
                double       x,
                double       y)
{
  BobguiNotebookPage *page;
  GList *children;

  for (children = notebook->children; children; children = children->next)
    {
      graphene_rect_t bounds;

      page = children->data;

      if (!bobgui_notebook_page_tab_label_is_visible (page))
        continue;

      if (!bobgui_widget_compute_bounds (page->tab_widget, BOBGUI_WIDGET (notebook), &bounds))
        continue;

      if (graphene_rect_contains_point (&bounds, &(graphene_point_t){x, y}))
        return children;
    }

  return NULL;
}

static void
bobgui_notebook_gesture_pressed (BobguiGestureClick *gesture,
                              int                   n_press,
                              double                x,
                              double                y,
                              gpointer              user_data)
{
  BobguiNotebook *notebook = user_data;
  BobguiWidget *widget = user_data;
  GdkEventSequence *sequence;
  BobguiNotebookArrow arrow;
  BobguiNotebookPage *page;
  GdkEvent *event;
  guint button;
  GList *tab;

  sequence = bobgui_gesture_single_get_current_sequence (BOBGUI_GESTURE_SINGLE (gesture));
  button = bobgui_gesture_single_get_current_button (BOBGUI_GESTURE_SINGLE (gesture));
  event = bobgui_gesture_get_last_event (BOBGUI_GESTURE (gesture), sequence);

  if (!notebook->children)
    return;

  arrow = bobgui_notebook_get_arrow (notebook, x, y);
  if (arrow != ARROW_NONE)
    {
      bobgui_notebook_arrow_button_press (notebook, arrow, button);
      bobgui_gesture_set_state (BOBGUI_GESTURE (gesture), BOBGUI_EVENT_SEQUENCE_CLAIMED);
      return;
    }

  if (in_tabs (notebook, x, y) && notebook->menu && gdk_event_triggers_context_menu (event))
    {
      GdkRectangle rect;

      rect.x = x;
      rect.y = y;
      rect.width = 1;
      rect.height = 1;
      bobgui_popover_set_pointing_to (BOBGUI_POPOVER (notebook->menu), &rect);
      bobgui_popover_popup (BOBGUI_POPOVER (notebook->menu));
      return;
    }

  if (button != GDK_BUTTON_PRIMARY)
    return;

  if ((tab = get_tab_at_pos (notebook, x, y)) != NULL)
    {
      gboolean page_changed, was_focus;

      page = tab->data;
      page_changed = page != notebook->cur_page;
      was_focus = bobgui_widget_is_focus (widget);

      bobgui_notebook_switch_focus_tab (notebook, tab);
      bobgui_widget_grab_focus (widget);

      if (page_changed && !was_focus)
        bobgui_widget_child_focus (page->child, BOBGUI_DIR_TAB_FORWARD);

      /* save press to possibly begin a drag */
      if (page->reorderable || page->detachable)
        {
          graphene_rect_t tab_bounds;

          notebook->pressed_button = button;

          notebook->mouse_x = x;
          notebook->mouse_y = y;

          notebook->drag_begin_x = notebook->mouse_x;
          notebook->drag_begin_y = notebook->mouse_y;

          /* tab bounds get set to empty, which is fine */
          notebook->drag_offset_x = notebook->drag_begin_x;
          notebook->drag_offset_y = notebook->drag_begin_y;
          if (bobgui_widget_compute_bounds (page->tab_widget, BOBGUI_WIDGET (notebook), &tab_bounds))
            {
              notebook->drag_offset_x -= tab_bounds.origin.x;
              notebook->drag_offset_y -= tab_bounds.origin.y;
            }
        }
    }
}

static void
bobgui_notebook_popup_menu (BobguiWidget  *widget,
                         const char *action_name,
                         GVariant   *parameters)
{
  BobguiNotebook *notebook = BOBGUI_NOTEBOOK (widget);

  if (notebook->menu)
    bobgui_popover_popup (BOBGUI_POPOVER (notebook->menu));
}

static void
stop_scrolling (BobguiNotebook *notebook)
{

  if (notebook->timer)
    {
      g_source_remove (notebook->timer);
      notebook->timer = 0;
      notebook->need_timer = FALSE;
    }
  notebook->click_child = ARROW_NONE;
  notebook->pressed_button = 0;
}

static GList*
get_drop_position (BobguiNotebook *notebook)
{
  GList *children, *last_child;
  BobguiNotebookPage *page;
  gboolean is_rtl;
  int x, y;

  x = notebook->mouse_x;
  y = notebook->mouse_y;

  is_rtl = bobgui_widget_get_direction ((BobguiWidget *) notebook) == BOBGUI_TEXT_DIR_RTL;
  last_child = NULL;

  for (children = notebook->children; children; children = children->next)
    {
      page = children->data;

      if ((notebook->operation != DRAG_OPERATION_REORDER || page != notebook->cur_page) &&
          bobgui_widget_get_visible (page->child) &&
          page->tab_label &&
          bobgui_widget_get_mapped (page->tab_label))
        {
          graphene_rect_t tab_bounds;

          if (!bobgui_widget_compute_bounds (page->tab_widget, BOBGUI_WIDGET (notebook), &tab_bounds))
            continue;

          switch (notebook->tab_pos)
            {
            case BOBGUI_POS_TOP:
            case BOBGUI_POS_BOTTOM:
              if (!is_rtl)
                {
                  if (tab_bounds.origin.x + tab_bounds.size.width / 2 > x)
                    return children;
                }
              else
                {
                  if (tab_bounds.origin.x + tab_bounds.size.width / 2 < x)
                    return children;
                }
              break;

            case BOBGUI_POS_LEFT:
            case BOBGUI_POS_RIGHT:
              if (tab_bounds.origin.y + tab_bounds.size.height / 2 > y)
                return children;
              break;

            default:
              g_assert_not_reached ();
              break;
            }

          last_child = children->next;
        }
    }

  return last_child;
}

static void
tab_drag_begin (BobguiNotebook     *notebook,
                BobguiNotebookPage *page)
{
  bobgui_widget_add_css_class (page->tab_widget, "dnd");
}

/* This function undoes the reparenting that happens both when drag_surface
 * is shown for reordering and when the DnD icon is shown for detaching
 */
static void
tab_drag_end (BobguiNotebook     *notebook,
              BobguiNotebookPage *page)
{
  if (!NOTEBOOK_IS_TAB_LABEL_PARENT (notebook, page))
    {
      g_object_ref (page->tab_label);
      bobgui_box_remove (BOBGUI_BOX (bobgui_widget_get_parent (page->tab_label)), page->tab_label);
      bobgui_widget_set_parent (page->tab_label, page->tab_widget);
      g_object_unref (page->tab_label);
    }

  bobgui_widget_remove_css_class (page->tab_widget, "dnd");
}

static void
bobgui_notebook_stop_reorder (BobguiNotebook *notebook)
{
  BobguiNotebookPage *page;

  if (notebook->operation == DRAG_OPERATION_DETACH)
    page = notebook->detached_tab;
  else
    page = notebook->cur_page;

  if (!page || !page->tab_label)
    return;

  notebook->pressed_button = 0;

  if (page->reorderable || page->detachable)
    {
      if (notebook->operation == DRAG_OPERATION_REORDER)
        {
          int old_page_num, page_num, i;
          GList *element;

          element = get_drop_position (notebook);
          old_page_num = g_list_position (notebook->children, notebook->focus_tab);
          page_num = reorder_tab (notebook, element, notebook->focus_tab);
          bobgui_notebook_child_reordered (notebook, page);

          if (notebook->has_scrolled || old_page_num != page_num)
            {
              for (element = notebook->children, i = 0; element; element = element->next, i++)
                {
                  if (MIN (old_page_num, page_num) <= i && i <= MAX (old_page_num, page_num))
                    g_object_notify (G_OBJECT (element->data), "position");
                }
              g_signal_emit (notebook,
                             notebook_signals[PAGE_REORDERED], 0,
                             page->child, page_num);
            }
        }

      notebook->has_scrolled = FALSE;

      tab_drag_end (notebook, page);

      notebook->operation = DRAG_OPERATION_NONE;

      if (notebook->dnd_timer)
        {
          g_source_remove (notebook->dnd_timer);
          notebook->dnd_timer = 0;
        }

      bobgui_widget_queue_allocate (BOBGUI_WIDGET (notebook));
    }
}

static void
bobgui_notebook_gesture_released (BobguiGestureClick *gesture,
                               int              n_press,
                               double           x,
                               double           y,
                               gpointer         user_data)
{
  BobguiNotebook *notebook = user_data;
  GdkEventSequence *sequence;
  GdkEvent *event;
  guint button;

  sequence = bobgui_gesture_single_get_current_sequence (BOBGUI_GESTURE_SINGLE (gesture));
  button = bobgui_gesture_single_get_current_button (BOBGUI_GESTURE_SINGLE (gesture));
  event = bobgui_gesture_get_last_event (BOBGUI_GESTURE (gesture), sequence);

  if (!event)
    return;

  if (notebook->pressed_button != button)
    return;

  if (notebook->operation == DRAG_OPERATION_REORDER &&
      notebook->cur_page &&
      notebook->cur_page->reorderable)
    bobgui_notebook_stop_reorder (notebook);

  stop_scrolling (notebook);
}

static void
bobgui_notebook_gesture_cancel (BobguiGestureClick  *gesture,
                             GdkEventSequence *sequence,
                             BobguiNotebook      *notebook)
{
  bobgui_notebook_stop_reorder (notebook);
  stop_scrolling (notebook);
}

static BobguiNotebookPointerPosition
get_pointer_position (BobguiNotebook *notebook)
{
  BobguiWidget *widget = BOBGUI_WIDGET (notebook);
  graphene_rect_t area;
  int width, height;
  gboolean is_rtl;

  if (!notebook->scrollable)
    return POINTER_BETWEEN;

  bobgui_notebook_get_tab_area_position (notebook, &area);
  width = area.size.width;
  height = area.size.height;

  if (notebook->tab_pos == BOBGUI_POS_TOP ||
      notebook->tab_pos == BOBGUI_POS_BOTTOM)
    {
      int x = notebook->mouse_x;

      is_rtl = bobgui_widget_get_direction (widget) == BOBGUI_TEXT_DIR_RTL;

      if (x > width - SCROLL_THRESHOLD)
        return (is_rtl) ? POINTER_BEFORE : POINTER_AFTER;
      else if (x < SCROLL_THRESHOLD)
        return (is_rtl) ? POINTER_AFTER : POINTER_BEFORE;
      else
        return POINTER_BETWEEN;
    }
  else
    {
      int y = notebook->mouse_y;

      if (y > height - SCROLL_THRESHOLD)
        return POINTER_AFTER;
      else if (y < SCROLL_THRESHOLD)
        return POINTER_BEFORE;
      else
        return POINTER_BETWEEN;
    }
}

static gboolean
scroll_notebook_timer (gpointer data)
{
  BobguiNotebook *notebook = BOBGUI_NOTEBOOK (data);
  BobguiNotebookPointerPosition pointer_position;
  GList *element, *first_tab;

  pointer_position = get_pointer_position (notebook);

  element = get_drop_position (notebook);
  reorder_tab (notebook, element, notebook->focus_tab);
  first_tab = bobgui_notebook_search_page (notebook, notebook->first_tab,
                                        (pointer_position == POINTER_BEFORE) ? STEP_PREV : STEP_NEXT,
                                        TRUE);
  if (first_tab && notebook->cur_page)
    {
      notebook->first_tab = first_tab;

      bobgui_widget_queue_allocate (notebook->tabs_widget);
    }

  return TRUE;
}

static gboolean
check_threshold (BobguiNotebook *notebook,
                 int          current_x,
                 int          current_y)
{
  int dnd_threshold;
  graphene_rect_t rectangle;
  BobguiSettings *settings;

  settings = bobgui_widget_get_settings (BOBGUI_WIDGET (notebook));
  g_object_get (G_OBJECT (settings), "bobgui-dnd-drag-threshold", &dnd_threshold, NULL);

  /* we want a large threshold */
  dnd_threshold *= DND_THRESHOLD_MULTIPLIER;

  bobgui_notebook_get_tab_area_position (notebook, &rectangle);
  graphene_rect_inset (&rectangle, -dnd_threshold, -dnd_threshold);

  /* The negation here is important! */
  return !graphene_rect_contains_point (&rectangle, &(graphene_point_t){current_x, current_y});
}

static void
bobgui_notebook_motion (BobguiEventController *controller,
                     double              x,
                     double              y,
                     gpointer            user_data)
{
  BobguiWidget *widget = BOBGUI_WIDGET (user_data);
  BobguiNotebook *notebook = BOBGUI_NOTEBOOK (widget);
  BobguiNotebookPage *page;
  guint state;

  page = notebook->cur_page;
  if (!page)
    return;

  state = bobgui_event_controller_get_current_event_state (controller);

  if (!(state & GDK_BUTTON1_MASK) &&
      notebook->pressed_button != 0)
    {
      bobgui_notebook_stop_reorder (notebook);
      stop_scrolling (notebook);
    }

  notebook->mouse_x = x;
  notebook->mouse_y = y;

  if (notebook->pressed_button == 0)
    return;

  if (page->detachable &&
      check_threshold (notebook, notebook->mouse_x, notebook->mouse_y))
    {
      GdkSurface *surface;
      GdkDevice *device;
      GdkContentProvider *content;
      GdkDrag *drag;
      GdkPaintable *paintable;

      notebook->detached_tab = notebook->cur_page;

      surface = bobgui_native_get_surface (bobgui_widget_get_native (BOBGUI_WIDGET (notebook)));
      device = bobgui_event_controller_get_current_event_device (controller);

      content = gdk_content_provider_new_union ((GdkContentProvider *[2]) {
                                                  bobgui_notebook_root_content_new (notebook),
                                                  gdk_content_provider_new_typed (BOBGUI_TYPE_NOTEBOOK_PAGE, notebook->cur_page)
                                                }, 2);
      drag = gdk_drag_begin (surface, device, content, GDK_ACTION_MOVE, notebook->drag_begin_x, notebook->drag_begin_y);
      g_object_unref (content);

      g_signal_connect (drag, "dnd-finished", G_CALLBACK (bobgui_notebook_dnd_finished_cb), notebook);
      g_signal_connect (drag, "cancel", G_CALLBACK (bobgui_notebook_drag_cancel_cb), notebook);

      paintable = bobgui_widget_paintable_new (notebook->detached_tab->tab_widget);
      bobgui_drag_icon_set_from_paintable (drag, paintable, -2, -2);
      g_object_unref (paintable);

      if (notebook->dnd_timer)
        {
          g_source_remove (notebook->dnd_timer);
          notebook->dnd_timer = 0;
        }

      notebook->operation = DRAG_OPERATION_DETACH;
      tab_drag_end (notebook, notebook->cur_page);

      g_object_set_data (G_OBJECT (drag), "bobgui-notebook-drag-origin", notebook);

      g_object_unref (drag);

      return;
    }

  if (page->reorderable &&
      (notebook->operation == DRAG_OPERATION_REORDER ||
       bobgui_drag_check_threshold_double (widget,
                                        notebook->drag_begin_x,
                                        notebook->drag_begin_y,
                                        notebook->mouse_x,
                                        notebook->mouse_y)))
    {
      BobguiNotebookPointerPosition pointer_position = get_pointer_position (notebook);

      if (pointer_position != POINTER_BETWEEN &&
          bobgui_notebook_show_arrows (notebook))
        {
          /* scroll tabs */
          if (!notebook->dnd_timer)
            {
              notebook->has_scrolled = TRUE;
              notebook->dnd_timer = g_timeout_add (TIMEOUT_REPEAT * SCROLL_DELAY_FACTOR,
                                               scroll_notebook_timer,
                                               notebook);
              gdk_source_set_static_name_by_id (notebook->dnd_timer, "[bobgui] scroll_notebook_timer");
            }
        }
      else
        {
          if (notebook->dnd_timer)
            {
              g_source_remove (notebook->dnd_timer);
              notebook->dnd_timer = 0;
            }
        }

      if (notebook->operation != DRAG_OPERATION_REORDER)
        {
          notebook->operation = DRAG_OPERATION_REORDER;
          tab_drag_begin (notebook, page);
        }
    }

  if (notebook->operation == DRAG_OPERATION_REORDER)
    bobgui_widget_queue_allocate (notebook->tabs_widget);
}

static void
update_arrow_state (BobguiNotebook *notebook)
{
  int i;
  gboolean is_rtl, left;

  is_rtl = bobgui_widget_get_direction (BOBGUI_WIDGET (notebook)) == BOBGUI_TEXT_DIR_RTL;

  for (i = 0; i < 4; i++)
    {
      gboolean sensitive = TRUE;

      if (notebook->arrow_widget[i] == NULL)
        continue;

      left = (ARROW_IS_LEFT (i) && !is_rtl) ||
             (!ARROW_IS_LEFT (i) && is_rtl);

      if (notebook->focus_tab &&
          !bobgui_notebook_search_page (notebook, notebook->focus_tab,
                                     left ? STEP_PREV : STEP_NEXT, TRUE))
        {
          sensitive = FALSE;
        }

      bobgui_widget_set_sensitive (notebook->arrow_widget[i], sensitive);
    }
}

static void
bobgui_notebook_state_flags_changed (BobguiWidget     *widget,
                                  BobguiStateFlags  previous_state)
{
  if (!bobgui_widget_is_sensitive (widget))
    stop_scrolling (BOBGUI_NOTEBOOK (widget));
}

static void
bobgui_notebook_arrow_drag_enter (BobguiDropControllerMotion *motion,
                               double                   x,
                               double                   y,
                               BobguiNotebook             *notebook)
{
  BobguiWidget *arrow_widget = bobgui_event_controller_get_widget (BOBGUI_EVENT_CONTROLLER (motion));
  guint arrow;

  for (arrow = 0; arrow < 4; arrow++)
    {
      if (notebook->arrow_widget[arrow] == arrow_widget)
        break;
    }

  g_assert (arrow != ARROW_NONE);

  notebook->click_child = arrow;
  bobgui_notebook_set_scroll_timer (notebook);
}

static void
bobgui_notebook_arrow_drag_leave (BobguiDropTarget *target,
                               GdkDrop       *drop,
                               BobguiNotebook   *notebook)
{
  stop_scrolling (notebook);
}

static void
update_arrow_nodes (BobguiNotebook *notebook)
{
  gboolean arrow[4];
  const char *up_icon_name;
  const char *down_icon_name;
  int i;

  if (notebook->tab_pos == BOBGUI_POS_LEFT ||
      notebook->tab_pos == BOBGUI_POS_RIGHT)
    {
      up_icon_name = "pan-down-symbolic";
      down_icon_name = "pan-up-symbolic";
    }
  else if (bobgui_widget_get_direction (BOBGUI_WIDGET (notebook)) == BOBGUI_TEXT_DIR_LTR)
    {
      up_icon_name = "pan-end-symbolic";
      down_icon_name = "pan-start-symbolic";
    }
  else
    {
      up_icon_name = "pan-start-symbolic";
      down_icon_name = "pan-end-symbolic";
    }

  arrow[0] = TRUE;
  arrow[1] = FALSE;
  arrow[2] = FALSE;
  arrow[3] = TRUE;

  for (i = 0; i < 4; i++)
    {
      if (notebook->scrollable && arrow[i])
        {
          if (notebook->arrow_widget[i] == NULL)
            {
              BobguiWidget *next_widget;
              BobguiEventController *controller;

              switch (i)
                {
                case 0:
                  if (notebook->arrow_widget[1])
                    {
                      next_widget = notebook->arrow_widget[1];
                      break;
                    }
                  G_GNUC_FALLTHROUGH;
                case 1:
                  if (notebook->children)
                    {
                      BobguiNotebookPage *page = notebook->children->data;
                      next_widget = page->tab_widget;
                      break;
                    }
                  if (notebook->arrow_widget[2])
                    {
                      next_widget = notebook->arrow_widget[2];
                      break;
                    }
                  G_GNUC_FALLTHROUGH;
                case 2:
                  if (notebook->arrow_widget[3])
                    {
                      next_widget = notebook->arrow_widget[3];
                      break;
                    }
                  G_GNUC_FALLTHROUGH;
                case 3:
                  next_widget = NULL;
                  break;

                default:
                  g_assert_not_reached ();
                  next_widget = NULL;
                  break;
                }

              notebook->arrow_widget[i] = g_object_new (BOBGUI_TYPE_BUTTON,
                                                        "css-name", "arrow",
                                                        NULL);
              controller = bobgui_drop_controller_motion_new ();
              g_signal_connect (controller, "enter", G_CALLBACK (bobgui_notebook_arrow_drag_enter), notebook);
              g_signal_connect (controller, "leave", G_CALLBACK (bobgui_notebook_arrow_drag_leave), notebook);
              bobgui_widget_add_controller (notebook->arrow_widget[i], controller);

              if (i == ARROW_LEFT_BEFORE || i == ARROW_LEFT_AFTER)
                {
                  bobgui_widget_add_css_class (notebook->arrow_widget[i], "down");
                  bobgui_widget_insert_after (notebook->arrow_widget[i], notebook->tabs_widget, next_widget);
                }
              else
                {
                  bobgui_widget_add_css_class (notebook->arrow_widget[i], "up");
                  bobgui_widget_insert_before (notebook->arrow_widget[i], notebook->tabs_widget, next_widget);
                }
           }

          if (i == ARROW_LEFT_BEFORE || i == ARROW_LEFT_AFTER)
            bobgui_button_set_icon_name (BOBGUI_BUTTON (notebook->arrow_widget[i]), down_icon_name);
          else
            bobgui_button_set_icon_name (BOBGUI_BUTTON (notebook->arrow_widget[i]), up_icon_name);

          if (i == ARROW_LEFT_BEFORE || i == ARROW_LEFT_AFTER)
            bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (notebook->arrow_widget[i]),
                                            BOBGUI_ACCESSIBLE_PROPERTY_LABEL, _("Previous tab"),
                                            -1);
          else
            bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (notebook->arrow_widget[i]),
                                            BOBGUI_ACCESSIBLE_PROPERTY_LABEL, _("Next tab"),
                                            -1);
        }
      else
        {
          g_clear_pointer (&notebook->arrow_widget[i], bobgui_widget_unparent);
        }
    }
}

static void
bobgui_notebook_direction_changed (BobguiWidget        *widget,
                                BobguiTextDirection  previous_direction)
{
  update_arrow_nodes (BOBGUI_NOTEBOOK (widget));
}

static void
bobgui_notebook_dnd_finished_cb (GdkDrag   *drag,
                              BobguiWidget *widget)
{
  BobguiNotebook *notebook = BOBGUI_NOTEBOOK (widget);

  bobgui_notebook_stop_reorder (notebook);

  if (notebook->rootwindow_drop)
    {
      BobguiNotebook *dest_notebook = NULL;

      g_signal_emit (notebook, notebook_signals[CREATE_WINDOW], 0,
                     notebook->detached_tab->child, &dest_notebook);

      if (dest_notebook)
        do_detach_tab (notebook, dest_notebook, notebook->detached_tab->child);

      notebook->rootwindow_drop = FALSE;
    }
  else if (notebook->detached_tab)
    {
      bobgui_notebook_switch_page (notebook, notebook->detached_tab);
    }

  notebook->operation = DRAG_OPERATION_NONE;
}

static BobguiNotebook *
bobgui_notebook_create_window (BobguiNotebook *notebook,
                            BobguiWidget   *page)
{
  return NULL;
}

static void
bobgui_notebook_drag_cancel_cb (GdkDrag             *drag,
                             GdkDragCancelReason  reason,
                             BobguiWidget           *widget)
{
  BobguiNotebook *notebook = BOBGUI_NOTEBOOK (widget);

  notebook->rootwindow_drop = FALSE;

  if (reason == GDK_DRAG_CANCEL_NO_TARGET)
    {
      BobguiNotebook *dest_notebook = NULL;

      g_signal_emit (notebook, notebook_signals[CREATE_WINDOW], 0,
                     notebook->detached_tab->child, &dest_notebook);

      if (dest_notebook)
        do_detach_tab (notebook, dest_notebook, notebook->detached_tab->child);
    }
}

static gboolean
bobgui_notebook_switch_page_timeout (gpointer data)
{
  BobguiNotebook *notebook = BOBGUI_NOTEBOOK (data);
  BobguiNotebookPage *switch_page;

  notebook->switch_page_timer = 0;

  switch_page = notebook->switch_page;
  notebook->switch_page = NULL;

  if (switch_page)
    {
      /* FIXME: hack, we don't want the
       * focus to move from the source widget
       */
      notebook->child_has_focus = FALSE;
      bobgui_notebook_switch_focus_tab (notebook,
                                     g_list_find (notebook->children,
                                                  switch_page));
    }

  return FALSE;
}

static gboolean
bobgui_notebook_can_drag_from (BobguiNotebook     *self,
                            BobguiNotebook     *other,
                            BobguiNotebookPage *page)
{
  /* always allow dragging inside self */
  if (self == other)
    return TRUE;

  /* if the groups don't match, fail */
  if (self->group == 0 ||
      self->group != other->group)
    return FALSE;

  /* Check that the dragged page is not a parent of the notebook
   * being dragged into */
  if (BOBGUI_WIDGET (self) == page->child ||
      bobgui_widget_is_ancestor (BOBGUI_WIDGET (self), BOBGUI_WIDGET (page->child)) ||
      BOBGUI_WIDGET (self) == page->tab_label ||
      bobgui_widget_is_ancestor (BOBGUI_WIDGET (self), BOBGUI_WIDGET (page->tab_label)))
    return FALSE;

  return TRUE;
}

static GdkDragAction
bobgui_notebook_drag_motion (BobguiDropTarget *dest,
                          double         x,
                          double         y,
                          BobguiNotebook   *notebook)
{
  GdkDrag *drag = gdk_drop_get_drag (bobgui_drop_target_get_current_drop (dest));
  BobguiNotebook *source;

  notebook->mouse_x = x;
  notebook->mouse_y = y;

  if (!drag)
    return 0;

  source = BOBGUI_NOTEBOOK (g_object_get_data (G_OBJECT (drag), "bobgui-notebook-drag-origin"));
  g_assert (source->cur_page != NULL);

  if (!bobgui_notebook_can_drag_from (notebook, source, source->cur_page))
    return 0;

  return GDK_ACTION_MOVE;
}

static gboolean
bobgui_notebook_drag_drop (BobguiDropTarget *dest,
                        const GValue  *value,
                        double         x,
                        double         y,
                        BobguiNotebook   *self)
{
  GdkDrag *drag = gdk_drop_get_drag (bobgui_drop_target_get_current_drop (dest));
  BobguiNotebook *source;
  BobguiNotebookPage *page = g_value_get_object (value);

  source = drag ? g_object_get_data (G_OBJECT (drag), "bobgui-notebook-drag-origin") : NULL;

  if (!source || !bobgui_notebook_can_drag_from (self, source, source->cur_page))
    return FALSE;

  self->mouse_x = x;
  self->mouse_y = y;

  do_detach_tab (source, self, page->child);

  return TRUE;
}

/**
 * bobgui_notebook_detach_tab:
 * @notebook: a `BobguiNotebook`
 * @child: a child
 *
 * Removes the child from the notebook.
 *
 * This function is very similar to [method@Bobgui.Notebook.remove_page],
 * but additionally informs the notebook that the removal
 * is happening as part of a tab DND operation, which should
 * not be cancelled.
 */
void
bobgui_notebook_detach_tab (BobguiNotebook *notebook,
                         BobguiWidget   *child)
{
  notebook->remove_in_detach = TRUE;
  bobgui_notebook_remove (notebook, child);
  notebook->remove_in_detach = FALSE;
}

static void
do_detach_tab (BobguiNotebook *from,
               BobguiNotebook *to,
               BobguiWidget   *child)
{
  BobguiWidget *tab_label, *menu_label;
  gboolean tab_expand, tab_fill, reorderable, detachable;
  GList *element;
  int page_num;
  BobguiNotebookPage *page;

  menu_label = bobgui_notebook_get_menu_label (from, child);

  if (menu_label)
    g_object_ref (menu_label);

  tab_label = bobgui_notebook_get_tab_label (from, child);

  if (tab_label)
    g_object_ref (tab_label);

  g_object_ref (child);

  page = bobgui_notebook_get_page (from, child);
  g_object_get (page,
                "tab-expand", &tab_expand,
                "tab-fill", &tab_fill,
                "reorderable", &reorderable,
                "detachable", &detachable,
                NULL);

  bobgui_notebook_detach_tab (from, child);

  element = get_drop_position (to);
  page_num = g_list_position (to->children, element);
  bobgui_notebook_insert_page_menu (to, child, tab_label, menu_label, page_num);

  page = bobgui_notebook_get_page (to, child);
  g_object_set (page,
                "tab-expand", tab_expand,
                "tab-fill", tab_fill,
                "reorderable", reorderable,
                "detachable", detachable,
                NULL);

  if (child)
    g_object_unref (child);

  if (tab_label)
    g_object_unref (tab_label);

  if (menu_label)
    g_object_unref (menu_label);

  bobgui_notebook_set_current_page (to, page_num);
}

/* Private methods:
 *
 * bobgui_notebook_remove
 * bobgui_notebook_focus
 * bobgui_notebook_set_focus_child
 */
static void
bobgui_notebook_remove (BobguiNotebook *notebook,
                     BobguiWidget   *widget)
{
  BobguiNotebookPage *page;
  GList *children, *list;
  int page_num = 0;

  children = notebook->children;
  while (children)
    {
      page = children->data;

      if (page->child == widget)
        break;

      page_num++;
      children = children->next;
    }

  if (children == NULL)
    return;

  g_object_ref (widget);

  list = children->next;
  bobgui_notebook_real_remove (notebook, children);

  while (list)
    {
      g_object_notify (G_OBJECT (list->data), "position");
      list = list->next;
    }

  g_signal_emit (notebook,
                 notebook_signals[PAGE_REMOVED],
                 0,
                 widget,
                 page_num);

  g_object_unref (widget);
}

static gboolean
focus_tabs_in (BobguiNotebook *notebook)
{
  if (notebook->show_tabs && bobgui_notebook_has_current_page (notebook))
    {
      bobgui_widget_grab_focus (BOBGUI_WIDGET (notebook));
      bobgui_notebook_set_focus_child (BOBGUI_WIDGET (notebook), NULL);
      bobgui_notebook_switch_focus_tab (notebook,
                                     g_list_find (notebook->children,
                                                  notebook->cur_page));

      return TRUE;
    }
  else
    return FALSE;
}

static gboolean
focus_tabs_move (BobguiNotebook     *notebook,
                 BobguiDirectionType direction,
                 int              search_direction)
{
  GList *new_page;

  new_page = bobgui_notebook_search_page (notebook, notebook->focus_tab,
                                       search_direction, TRUE);
  if (!new_page)
    {
      new_page = bobgui_notebook_search_page (notebook, NULL,
                                           search_direction, TRUE);
    }

  if (new_page)
    bobgui_notebook_switch_focus_tab (notebook, new_page);
  else
    bobgui_widget_error_bell (BOBGUI_WIDGET (notebook));

  return TRUE;
}

static gboolean
focus_child_in (BobguiNotebook      *notebook,
                BobguiDirectionType  direction)
{
  if (notebook->cur_page)
    return bobgui_widget_child_focus (notebook->cur_page->child, direction);
  else
    return FALSE;
}

static gboolean
focus_action_in (BobguiNotebook      *notebook,
                 int               action,
                 BobguiDirectionType  direction)
{
  if (notebook->action_widget[action] &&
      bobgui_widget_get_visible (notebook->action_widget[action]))
    return bobgui_widget_child_focus (notebook->action_widget[action], direction);
  else
    return FALSE;
}

/* Focus in the notebook can either be on the pages, or on
 * the tabs or on the action_widgets.
 */
static gboolean
bobgui_notebook_focus (BobguiWidget        *widget,
                    BobguiDirectionType  direction)
{
  BobguiNotebook *notebook = BOBGUI_NOTEBOOK (widget);
  BobguiWidget *old_focus_child;
  BobguiDirectionType effective_direction;
  int first_action;
  int last_action;

  gboolean widget_is_focus;

  if (notebook->tab_pos == BOBGUI_POS_TOP ||
      notebook->tab_pos == BOBGUI_POS_LEFT)
    {
      first_action = ACTION_WIDGET_START;
      last_action = ACTION_WIDGET_END;
    }
  else
    {
      first_action = ACTION_WIDGET_END;
      last_action = ACTION_WIDGET_START;
    }

  if (notebook->focus_out)
    {
      notebook->focus_out = FALSE; /* Clear this to catch the wrap-around case */
      return FALSE;
    }

  widget_is_focus = bobgui_widget_is_focus (widget);
  old_focus_child = bobgui_widget_get_focus_child (widget);
  if (old_focus_child)
    old_focus_child = bobgui_widget_get_focus_child (old_focus_child);

  effective_direction = get_effective_direction (notebook, direction);

  if (old_focus_child)          /* Focus on page child or action widget */
    {
      if (bobgui_widget_child_focus (old_focus_child, direction))
        return TRUE;

      if (old_focus_child == notebook->action_widget[ACTION_WIDGET_START])
        {
          switch ((guint) effective_direction)
            {
            case BOBGUI_DIR_DOWN:
              return focus_child_in (notebook, BOBGUI_DIR_TAB_FORWARD);
            case BOBGUI_DIR_RIGHT:
              return focus_tabs_in (notebook);
            case BOBGUI_DIR_LEFT:
              return FALSE;
            case BOBGUI_DIR_UP:
              return FALSE;
            default:
              switch ((guint) direction)
                {
                case BOBGUI_DIR_TAB_FORWARD:
                  if ((notebook->tab_pos == BOBGUI_POS_RIGHT || notebook->tab_pos == BOBGUI_POS_BOTTOM) &&
                      focus_child_in (notebook, direction))
                    return TRUE;
                  return focus_tabs_in (notebook);
                case BOBGUI_DIR_TAB_BACKWARD:
                  return FALSE;
                default:
                  g_assert_not_reached ();
                  break;
                }
            }
        }
      else if (old_focus_child == notebook->action_widget[ACTION_WIDGET_END])
        {
          switch ((guint) effective_direction)
            {
            case BOBGUI_DIR_DOWN:
              return focus_child_in (notebook, BOBGUI_DIR_TAB_FORWARD);
            case BOBGUI_DIR_RIGHT:
              return FALSE;
            case BOBGUI_DIR_LEFT:
              return focus_tabs_in (notebook);
            case BOBGUI_DIR_UP:
              return FALSE;
            default:
              switch ((guint) direction)
                {
                case BOBGUI_DIR_TAB_FORWARD:
                  return FALSE;
                case BOBGUI_DIR_TAB_BACKWARD:
                  if ((notebook->tab_pos == BOBGUI_POS_TOP || notebook->tab_pos == BOBGUI_POS_LEFT) &&
                      focus_child_in (notebook, direction))
                    return TRUE;
                  return focus_tabs_in (notebook);
                default:
                  g_assert_not_reached ();
                  break;
                }
            }
        }
      else
        {
          switch ((guint) effective_direction)
            {
            case BOBGUI_DIR_TAB_BACKWARD:
            case BOBGUI_DIR_UP:
              /* Focus onto the tabs */
              return focus_tabs_in (notebook);
            case BOBGUI_DIR_DOWN:
            case BOBGUI_DIR_LEFT:
            case BOBGUI_DIR_RIGHT:
              return FALSE;
            case BOBGUI_DIR_TAB_FORWARD:
              return focus_action_in (notebook, last_action, direction);
            default:
              break;
            }
        }
    }
  else if (widget_is_focus)     /* Focus was on tabs */
    {
      switch ((guint) effective_direction)
        {
        case BOBGUI_DIR_TAB_BACKWARD:
              return focus_action_in (notebook, first_action, direction);
        case BOBGUI_DIR_UP:
          return FALSE;
        case BOBGUI_DIR_TAB_FORWARD:
          if (focus_child_in (notebook, BOBGUI_DIR_TAB_FORWARD))
            return TRUE;
          return focus_action_in (notebook, last_action, direction);
        case BOBGUI_DIR_DOWN:
          /* We use TAB_FORWARD rather than direction so that we focus a more
           * predictable widget for the user; users may be using arrow focusing
           * in this situation even if they don't usually use arrow focusing.
           */
          return focus_child_in (notebook, BOBGUI_DIR_TAB_FORWARD);
        case BOBGUI_DIR_LEFT:
          return focus_tabs_move (notebook, direction, STEP_PREV);
        case BOBGUI_DIR_RIGHT:
          return focus_tabs_move (notebook, direction, STEP_NEXT);
        default:
          break;
        }
    }
  else /* Focus was not on widget */
    {
      switch ((guint) effective_direction)
        {
        case BOBGUI_DIR_TAB_FORWARD:
        case BOBGUI_DIR_DOWN:
          if (focus_action_in (notebook, first_action, direction))
            return TRUE;
          if (focus_tabs_in (notebook))
            return TRUE;
          if (focus_action_in (notebook, last_action, direction))
            return TRUE;
          if (focus_child_in (notebook, direction))
            return TRUE;
          return FALSE;
        case BOBGUI_DIR_TAB_BACKWARD:
          if (focus_action_in (notebook, last_action, direction))
            return TRUE;
          if (focus_child_in (notebook, direction))
            return TRUE;
          if (focus_tabs_in (notebook))
            return TRUE;
          if (focus_action_in (notebook, first_action, direction))
            return TRUE;
          return FALSE;
        case BOBGUI_DIR_UP:
        case BOBGUI_DIR_LEFT:
        case BOBGUI_DIR_RIGHT:
          return focus_child_in (notebook, direction);
        default:
          break;
        }
    }

  g_assert_not_reached ();
  return FALSE;
}

static gboolean
bobgui_notebook_grab_focus (BobguiWidget *widget)
{
  BobguiNotebook *notebook = BOBGUI_NOTEBOOK (widget);

  if (notebook->show_tabs)
    return bobgui_widget_grab_focus_self (widget);
  else
    return bobgui_widget_grab_focus_child (widget);
}

static void
bobgui_notebook_set_focus_child (BobguiWidget *widget,
                              BobguiWidget *child)
{
  BobguiNotebook *notebook = BOBGUI_NOTEBOOK (widget);
  BobguiWidget *page_child;
  BobguiWidget *toplevel;

  /* If the old focus widget was within a page of the notebook,
   * (child may either be NULL or not in this case), record it
   * for future use if we switch to the page with a mnemonic.
   */

  toplevel = BOBGUI_WIDGET (bobgui_widget_get_root (widget));
  if (BOBGUI_IS_WINDOW (toplevel))
    {
      page_child = bobgui_window_get_focus (BOBGUI_WINDOW (toplevel));
      while (page_child)
        {
          if (bobgui_widget_get_parent (page_child) == widget)
            {
              GList *list = bobgui_notebook_find_child (notebook, page_child);
              if (list != NULL)
                {
                  BobguiNotebookPage *page = list->data;

                  if (page->last_focus_child)
                    g_object_remove_weak_pointer (G_OBJECT (page->last_focus_child), (gpointer *)&page->last_focus_child);

                  page->last_focus_child = bobgui_window_get_focus (BOBGUI_WINDOW (toplevel));
                  g_object_add_weak_pointer (G_OBJECT (page->last_focus_child), (gpointer *)&page->last_focus_child);

                  break;
                }
            }

          page_child = bobgui_widget_get_parent (page_child);
        }
    }

  if (child)
    {
      g_return_if_fail (BOBGUI_IS_WIDGET (child));

      notebook->child_has_focus = TRUE;
      if (!notebook->focus_tab)
        {
          GList *children;
          BobguiNotebookPage *page;

          children = notebook->children;
          while (children)
            {
              page = children->data;
              if (page->child == child || page->tab_label == child)
                bobgui_notebook_switch_focus_tab (notebook, children);
              children = children->next;
            }
        }
    }
  else
    notebook->child_has_focus = FALSE;

  BOBGUI_WIDGET_CLASS (bobgui_notebook_parent_class)->set_focus_child (widget, child);
}

/* Private BobguiNotebook Methods:
 *
 * bobgui_notebook_real_insert_page
 */
static void
page_visible_cb (BobguiWidget  *child,
                 GParamSpec *arg,
                 gpointer    data)
{
  BobguiNotebook *notebook = BOBGUI_NOTEBOOK (data);
  GList *list = bobgui_notebook_find_child (notebook, BOBGUI_WIDGET (child));
  BobguiNotebookPage *page = list->data;
  GList *next = NULL;

  if (notebook->menu && page->menu_label)
    {
      BobguiWidget *parent = bobgui_widget_get_parent (page->menu_label);
      if (parent)
        bobgui_widget_set_visible (parent, bobgui_widget_get_visible (child));
    }

  bobgui_widget_set_visible (page->tab_widget, bobgui_widget_get_visible (child));

  if (notebook->cur_page == page)
    {
      if (!bobgui_widget_get_visible (child))
        {
          list = g_list_find (notebook->children, notebook->cur_page);
          if (list)
            {
              next = bobgui_notebook_search_page (notebook, list, STEP_NEXT, TRUE);
              if (!next)
                next = bobgui_notebook_search_page (notebook, list, STEP_PREV, TRUE);
            }

          if (next)
            bobgui_notebook_switch_page (notebook, BOBGUI_NOTEBOOK_PAGE_FROM_LIST (next));
        }
      bobgui_widget_set_visible (notebook->header_widget, notebook->show_tabs && bobgui_notebook_has_current_page (notebook));
    }

  if (!bobgui_notebook_has_current_page (notebook) && bobgui_widget_get_visible (child))
    {
      bobgui_notebook_switch_page (notebook, page);
      /* focus_tab is set in the switch_page method */
      bobgui_notebook_switch_focus_tab (notebook, notebook->focus_tab);
    }
}

static void
measure_tab (BobguiGizmo       *gizmo,
             BobguiOrientation  orientation,
             int             for_size,
             int            *minimum,
             int            *natural,
             int            *minimum_baseline,
             int            *natural_baseline)
{
  BobguiNotebook *notebook = g_object_get_data (G_OBJECT (gizmo), "notebook");
  GList *l;
  BobguiNotebookPage *page = NULL;

  for (l = notebook->children; l; l = l->next)
    {
      BobguiNotebookPage *p = BOBGUI_NOTEBOOK_PAGE_FROM_LIST (l);
      if (p->tab_widget == BOBGUI_WIDGET (gizmo))
        {
          page = p;
          break;
        }
    }

  g_assert (page != NULL);

  bobgui_widget_measure (page->tab_label,
                      orientation,
                      for_size,
                      minimum, natural,
                      minimum_baseline, natural_baseline);
}

static void
allocate_tab (BobguiGizmo *gizmo,
              int       width,
              int       height,
              int       baseline)
{
  BobguiNotebook *notebook = g_object_get_data (G_OBJECT (gizmo), "notebook");
  GList *l;
  BobguiNotebookPage *page = NULL;
  BobguiAllocation child_allocation;

  for (l = notebook->children; l; l = l->next)
    {
      BobguiNotebookPage *p = BOBGUI_NOTEBOOK_PAGE_FROM_LIST (l);
      if (p->tab_widget == BOBGUI_WIDGET (gizmo))
        {
          page = p;
          break;
        }
    }

  g_assert (page != NULL);

  child_allocation = (BobguiAllocation) {0, 0, width, height};

  if (!page->fill)
    {
      if (notebook->tab_pos == BOBGUI_POS_TOP || notebook->tab_pos == BOBGUI_POS_BOTTOM)
        {
          bobgui_widget_measure (page->tab_label, BOBGUI_ORIENTATION_HORIZONTAL, height,
                              NULL, &child_allocation.width, NULL, NULL);
          if (child_allocation.width > width)
            child_allocation.width = width;
          else
            child_allocation.x += (width - child_allocation.width) / 2;

        }
      else
        {
          bobgui_widget_measure (page->tab_label, BOBGUI_ORIENTATION_VERTICAL, width,
                              NULL, &child_allocation.height, NULL, NULL);

          if (child_allocation.height > height)
            child_allocation.height = height;
          else
            child_allocation.y += (height - child_allocation.height) / 2;
        }
    }

  bobgui_widget_size_allocate (page->tab_label, &child_allocation, baseline);
}

static void
bobgui_notebook_tab_drop_enter (BobguiEventController *controller,
                             double              x,
                             double              y,
                             BobguiNotebookPage    *page)
{
  BobguiWidget *widget = bobgui_event_controller_get_widget (controller);
  BobguiNotebook *notebook = g_object_get_data (G_OBJECT (widget), "notebook");

  g_assert (!notebook->switch_page_timer);

  notebook->switch_page = page;

  notebook->switch_page_timer = g_timeout_add (TIMEOUT_EXPAND, bobgui_notebook_switch_page_timeout, notebook);
  gdk_source_set_static_name_by_id (notebook->switch_page_timer, "[bobgui] bobgui_notebook_switch_page_timeout");
}

static void
bobgui_notebook_tab_drop_leave (BobguiEventController *controller,
                             BobguiNotebookPage    *page)
{
  BobguiWidget *widget = bobgui_event_controller_get_widget (controller);
  BobguiNotebook *notebook = g_object_get_data (G_OBJECT (widget), "notebook");

  g_clear_handle_id (&notebook->switch_page_timer, g_source_remove);
}

static int
bobgui_notebook_insert_notebook_page (BobguiNotebook *notebook,
                                   BobguiNotebookPage *page,
                                   int position)
{
  int nchildren;
  GList *list;
  BobguiWidget *sibling;
  BobguiEventController *controller;
  BobguiStackPage *stack_page;

  nchildren = g_list_length (notebook->children);
  if ((position < 0) || (position > nchildren))
    position = nchildren;

  notebook->children = g_list_insert (notebook->children, g_object_ref (page), position);

  if (position < nchildren)
    sibling = BOBGUI_NOTEBOOK_PAGE_FROM_LIST (g_list_nth (notebook->children, position))->tab_widget;
  else if (notebook->arrow_widget[ARROW_LEFT_AFTER])
    sibling = notebook->arrow_widget[ARROW_LEFT_AFTER];
  else
  sibling = notebook->arrow_widget[ARROW_RIGHT_AFTER];

  page->tab_widget = bobgui_gizmo_new_with_role ("tab",
                                              BOBGUI_ACCESSIBLE_ROLE_TAB,
                                              measure_tab,
                                              allocate_tab,
                                              NULL,
                                              NULL,
                                              NULL,
                                              NULL);
  g_object_set_data (G_OBJECT (page->tab_widget), "notebook", notebook);
  bobgui_widget_insert_before (page->tab_widget, notebook->tabs_widget, sibling);
  controller = bobgui_drop_controller_motion_new ();
  g_signal_connect (controller, "enter", G_CALLBACK (bobgui_notebook_tab_drop_enter), page);
  g_signal_connect (controller, "leave", G_CALLBACK (bobgui_notebook_tab_drop_leave), page);
  bobgui_widget_add_controller (page->tab_widget, controller);

  page->expand = FALSE;
  page->fill = TRUE;

  if (notebook->menu)
    bobgui_notebook_menu_item_create (notebook, page);

  bobgui_stack_add_named (BOBGUI_STACK (notebook->stack_widget), page->child, NULL);

  if (page->tab_label)
    {
      bobgui_widget_set_parent (page->tab_label, page->tab_widget);
      bobgui_accessible_update_relation (BOBGUI_ACCESSIBLE (page->tab_widget),
                                      BOBGUI_ACCESSIBLE_RELATION_LABELLED_BY, page->tab_label, NULL,
                                      -1);
      g_object_set_data (G_OBJECT (page->tab_label), "notebook", notebook);
    }

  stack_page = bobgui_stack_get_page (BOBGUI_STACK (notebook->stack_widget), page->child);
  bobgui_accessible_update_relation (BOBGUI_ACCESSIBLE (page->tab_widget),
                                  BOBGUI_ACCESSIBLE_RELATION_CONTROLS, stack_page, NULL,
                                  -1);
  bobgui_accessible_update_relation (BOBGUI_ACCESSIBLE (stack_page),
                                  BOBGUI_ACCESSIBLE_RELATION_LABELLED_BY, page->tab_widget, NULL,
                                  -1);

  bobgui_accessible_update_state (BOBGUI_ACCESSIBLE (page->tab_widget),
                               BOBGUI_ACCESSIBLE_STATE_SELECTED, FALSE,
                               -1);

  bobgui_notebook_update_labels (notebook);

  if (!notebook->first_tab)
    notebook->first_tab = notebook->children;

  if (page->tab_label)
    {
      bobgui_widget_set_visible (page->tab_label,
                              notebook->show_tabs && bobgui_widget_get_visible (page->child));

    page->mnemonic_activate_signal =
      g_signal_connect (page->tab_label,
                        "mnemonic-activate",
                        G_CALLBACK (bobgui_notebook_mnemonic_activate_switch_page),
                        notebook);
    }

  page->notify_visible_handler = g_signal_connect (page->child, "notify::visible",
                                                   G_CALLBACK (page_visible_cb), notebook);

  g_signal_emit (notebook, notebook_signals[PAGE_ADDED], 0, page->child, position);

  if (!bobgui_notebook_has_current_page (notebook))
    {
      bobgui_notebook_switch_page (notebook, page);
      /* focus_tab is set in the switch_page method */
      bobgui_notebook_switch_focus_tab (notebook, notebook->focus_tab);
    }

  g_object_notify (G_OBJECT (page), "tab-expand");
  g_object_notify (G_OBJECT (page), "tab-fill");
  g_object_notify (G_OBJECT (page), "tab-label");
  g_object_notify (G_OBJECT (page), "menu-label");

  list = g_list_nth (notebook->children, position);
  while (list)
    {
      g_object_notify (G_OBJECT (list->data), "position");
      list = list->next;
    }

  update_arrow_state (notebook);

  if (notebook->pages)
    g_list_model_items_changed (notebook->pages, position, 0, 1);

  /* The page-added handler might have reordered the pages, re-get the position */
  return bobgui_notebook_page_num (notebook, page->child);
}

static int
bobgui_notebook_real_insert_page (BobguiNotebook *notebook,
                               BobguiWidget   *child,
                               BobguiWidget   *tab_label,
                               BobguiWidget   *menu_label,
                               int          position)
{
  BobguiNotebookPage *page;
  int ret;

  page = g_object_new (BOBGUI_TYPE_NOTEBOOK_PAGE,
                       "child", child,
                       "tab", tab_label,
                       "menu", menu_label,
                       NULL);

  ret = bobgui_notebook_insert_notebook_page (notebook, page, position);

  g_object_unref (page);

  return ret;
}

static gboolean
bobgui_notebook_timer (BobguiNotebook *notebook)
{
  gboolean retval = FALSE;

  if (notebook->timer)
    {
      bobgui_notebook_do_arrow (notebook, notebook->click_child);

      if (notebook->need_timer)
        {
          notebook->need_timer = FALSE;
          notebook->timer = g_timeout_add (TIMEOUT_REPEAT * SCROLL_DELAY_FACTOR,
                                       (GSourceFunc) bobgui_notebook_timer,
                                       notebook);
          gdk_source_set_static_name_by_id (notebook->timer, "[bobgui] bobgui_notebook_timer");
        }
      else
        retval = TRUE;
    }

  return retval;
}

static void
bobgui_notebook_set_scroll_timer (BobguiNotebook *notebook)
{
  if (!notebook->timer)
    {
      notebook->timer = g_timeout_add (TIMEOUT_INITIAL,
                                   (GSourceFunc) bobgui_notebook_timer,
                                   notebook);
      gdk_source_set_static_name_by_id (notebook->timer, "[bobgui] bobgui_notebook_timer");
      notebook->need_timer = TRUE;
    }
}

static int
bobgui_notebook_page_compare (gconstpointer a,
                           gconstpointer b)
{
  return (((BobguiNotebookPage *) a)->child != b);
}

static GList*
bobgui_notebook_find_child (BobguiNotebook *notebook,
                         BobguiWidget   *child)
{
  return g_list_find_custom (notebook->children,
                             child,
                             bobgui_notebook_page_compare);
}

static void
bobgui_notebook_remove_tab_label (BobguiNotebook     *notebook,
                               BobguiNotebookPage *page)
{
  if (page->tab_label)
    {
      if (page->mnemonic_activate_signal)
        g_signal_handler_disconnect (page->tab_label,
                                     page->mnemonic_activate_signal);
      page->mnemonic_activate_signal = 0;

      if (bobgui_widget_get_native (page->tab_label) != bobgui_widget_get_native (BOBGUI_WIDGET (notebook)) ||
          !NOTEBOOK_IS_TAB_LABEL_PARENT (notebook, page))
        {
          BobguiWidget *parent;

          /* we hit this condition during dnd of a detached tab */
          parent = bobgui_widget_get_parent (page->tab_label);
          if (BOBGUI_IS_WINDOW (parent))
            bobgui_box_remove (BOBGUI_BOX (parent), page->tab_label);
          else
            bobgui_widget_unparent (page->tab_label);
        }
      else
        {
          bobgui_widget_unparent (page->tab_label);
        }

      page->tab_label = NULL;
    }
}

static void
bobgui_notebook_real_remove (BobguiNotebook *notebook,
                          GList       *list)
{
  BobguiNotebookPage *page;
  GList * next_list;
  int need_resize = FALSE;
  BobguiWidget *tab_label;
  gboolean destroying;
  int position;

  page = list->data;

  destroying = bobgui_widget_in_destruction (BOBGUI_WIDGET (notebook));

  next_list = bobgui_notebook_search_page (notebook, list, STEP_NEXT, TRUE);
  if (!next_list)
    next_list = bobgui_notebook_search_page (notebook, list, STEP_PREV, TRUE);

  notebook->children = g_list_remove_link (notebook->children, list);

  if (notebook->cur_page == list->data)
    {
      notebook->cur_page = NULL;
      if (next_list && !destroying)
        bobgui_notebook_switch_page (notebook, BOBGUI_NOTEBOOK_PAGE_FROM_LIST (next_list));
      if (notebook->operation == DRAG_OPERATION_REORDER && !notebook->remove_in_detach)
        bobgui_notebook_stop_reorder (notebook);
    }

  if (notebook->detached_tab == list->data)
    notebook->detached_tab = NULL;

  if (notebook->switch_page == page)
    notebook->switch_page = NULL;

  if (list == notebook->first_tab)
    notebook->first_tab = next_list;
  if (list == notebook->focus_tab && !destroying)
    bobgui_notebook_switch_focus_tab (notebook, next_list);

  position = g_list_index (notebook->children, page);

  g_signal_handler_disconnect (page->child, page->notify_visible_handler);

  if (bobgui_widget_get_visible (page->child) &&
      bobgui_widget_get_visible (BOBGUI_WIDGET (notebook)))
    need_resize = TRUE;

  bobgui_stack_remove (BOBGUI_STACK (notebook->stack_widget), page->child);

  tab_label = page->tab_label;
  if (tab_label)
    {
      g_object_ref (tab_label);
      bobgui_notebook_remove_tab_label (notebook, page);
      if (destroying)
        bobgui_widget_unparent (tab_label);
      g_object_unref (tab_label);
    }

  if (notebook->menu)
    {
      BobguiWidget *parent = bobgui_widget_get_parent (page->menu_label);

      if (parent)
        bobgui_notebook_menu_label_unparent (parent);
      bobgui_popover_set_child (BOBGUI_POPOVER (notebook->menu), NULL);

      bobgui_widget_queue_resize (notebook->menu);
    }

  g_list_free (list);

  if (page->last_focus_child)
    {
      g_object_remove_weak_pointer (G_OBJECT (page->last_focus_child), (gpointer *)&page->last_focus_child);
      page->last_focus_child = NULL;
    }

  bobgui_widget_unparent (page->tab_widget);

  g_object_unref (page);

  bobgui_notebook_update_labels (notebook);
  if (need_resize)
    bobgui_widget_queue_resize (BOBGUI_WIDGET (notebook));

  if (notebook->pages)
    g_list_model_items_changed (notebook->pages, position, 1, 0);
}

static void
bobgui_notebook_update_labels (BobguiNotebook *notebook)
{
  BobguiNotebookPage *page;
  GList *list;
  char string[32];
  int page_num = 1;

  if (!notebook->show_tabs && !notebook->menu)
    return;

  for (list = bobgui_notebook_search_page (notebook, NULL, STEP_NEXT, FALSE);
       list;
       list = bobgui_notebook_search_page (notebook, list, STEP_NEXT, FALSE))
    {
      const char *text;
      page = list->data;
      g_snprintf (string, sizeof (string), _("Page %u"), page_num++);
      if (page->tab_text)
        text = page->tab_text;
      else
        text = string;

      bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (page->tab_widget),
                                BOBGUI_ACCESSIBLE_PROPERTY_LABEL, text,
                                -1);

      if (notebook->show_tabs)
        {
          if (page->default_tab)
            {
              if (!page->tab_label)
                {
                  page->tab_label = bobgui_label_new ("");
                  g_object_ref_sink (page->tab_label);
                  g_object_set_data (G_OBJECT (page->tab_label), "notebook", notebook);
                  bobgui_widget_set_parent (page->tab_label, page->tab_widget);
                }
              bobgui_label_set_text (BOBGUI_LABEL (page->tab_label), text);
            }

          if (page->child && page->tab_label)
            bobgui_widget_set_visible (page->tab_label, bobgui_widget_get_visible (page->child));
        }

      if (notebook->menu && page->default_menu)
        {
          if (page->menu_text)
            text = page->menu_text;
          else if (BOBGUI_IS_LABEL (page->tab_label))
            text = bobgui_label_get_text (BOBGUI_LABEL (page->tab_label));
          else
            text = string;
          bobgui_label_set_text (BOBGUI_LABEL (page->menu_label), text);
        }
    }
}

static GList *
bobgui_notebook_search_page (BobguiNotebook *notebook,
                          GList       *list,
                          int          direction,
                          gboolean     find_visible)
{
  BobguiNotebookPage *page = NULL;
  GList *old_list = NULL;

  if (list)
    page = list->data;

  if (!page || direction == STEP_NEXT)
    {
      if (list)
        {
          old_list = list;
          list = list->next;
        }
      else
        list = notebook->children;

      while (list)
        {
          page = list->data;
          if (direction == STEP_NEXT &&
              (!find_visible ||
               (bobgui_widget_get_visible (page->child) &&
                (!page->tab_label || NOTEBOOK_IS_TAB_LABEL_PARENT (notebook, page)))))
            return list;
          old_list = list;
          list = list->next;
        }
      list = old_list;
    }
  else
    {
      list = list->prev;
    }
  while (list)
    {
      page = list->data;
      if (direction == STEP_PREV &&
          (!find_visible ||
           (bobgui_widget_get_visible (page->child) &&
            (!page->tab_label || NOTEBOOK_IS_TAB_LABEL_PARENT (notebook, page)))))
        return list;
      list = list->prev;
    }
  return NULL;
}

static void
bobgui_notebook_snapshot_tabs (BobguiGizmo    *gizmo,
                            BobguiSnapshot *snapshot)
{
  BobguiWidget *widget = bobgui_widget_get_parent (BOBGUI_WIDGET (gizmo));
  BobguiNotebook *notebook = BOBGUI_NOTEBOOK (bobgui_widget_get_parent (widget));
  BobguiNotebookPage *page;
  GList *children;
  gboolean showarrow;
  int step = STEP_PREV;
  gboolean is_rtl;
  BobguiPositionType tab_pos;
  guint i;

  is_rtl = bobgui_widget_get_direction (widget) == BOBGUI_TEXT_DIR_RTL;
  tab_pos = get_effective_tab_pos (notebook);
  showarrow = FALSE;

  if (!bobgui_notebook_has_current_page (notebook))
    return;

  if (!notebook->first_tab)
    notebook->first_tab = notebook->children;

  if (!NOTEBOOK_IS_TAB_LABEL_PARENT (notebook, notebook->cur_page) ||
      !bobgui_widget_get_mapped (notebook->cur_page->tab_label))
    {
      step = STEP_PREV;
    }
  else
    {
      switch (tab_pos)
        {
        case BOBGUI_POS_TOP:
        case BOBGUI_POS_BOTTOM:
          step = is_rtl ? STEP_PREV : STEP_NEXT;
          break;
        case BOBGUI_POS_LEFT:
        case BOBGUI_POS_RIGHT:
          step = STEP_PREV;
          break;
        default:
          g_assert_not_reached ();
          break;
        }
    }

  for (children = notebook->children; children; children = children->next)
    {
      page = children->data;

      if (!bobgui_widget_get_visible (page->child) ||
          page == notebook->detached_tab)
        continue;

      if (!bobgui_widget_get_mapped (page->tab_label))
        showarrow = TRUE;

      /* No point in keeping searching */
      if (showarrow)
        break;
    }

  for (children = bobgui_notebook_search_page (notebook, NULL, step, TRUE);
       children;
       children = bobgui_notebook_search_page (notebook, children, step, TRUE))
    {
      page = children->data;

      if (page == notebook->cur_page)
        break;

      if (!bobgui_notebook_page_tab_label_is_visible (page))
        continue;

      bobgui_widget_snapshot_child (BOBGUI_WIDGET (gizmo), page->tab_widget, snapshot);
    }

  if (children != NULL)
    {
      GList *other_order = NULL;

      for (children = bobgui_notebook_search_page (notebook, children, step, TRUE);
           children;
           children = bobgui_notebook_search_page (notebook, children, step, TRUE))
        {
          page = children->data;

          if (!bobgui_notebook_page_tab_label_is_visible (page))
            continue;

          other_order = g_list_prepend (other_order, page);
        }

      /* draw them with the opposite order */
      for (children = other_order; children; children = children->next)
        {
          page = children->data;
          bobgui_widget_snapshot_child (BOBGUI_WIDGET (gizmo), page->tab_widget, snapshot);
        }

      g_list_free (other_order);
    }

  if (showarrow && notebook->scrollable)
    {
      for (i = 0; i < 4; i++)
        {
          if (notebook->arrow_widget[i] == NULL)
            continue;

          bobgui_widget_snapshot_child (BOBGUI_WIDGET (gizmo), notebook->arrow_widget[i], snapshot);
        }
    }

  if (notebook->operation != DRAG_OPERATION_DETACH)
    bobgui_widget_snapshot_child (BOBGUI_WIDGET (gizmo), notebook->cur_page->tab_widget, snapshot);
}

/* Private BobguiNotebook Size Allocate Functions:
 *
 * bobgui_notebook_calculate_shown_tabs
 * bobgui_notebook_calculate_tabs_allocation
 * bobgui_notebook_calc_tabs
 */
static void
bobgui_notebook_allocate_arrows (BobguiNotebook   *notebook,
                              BobguiAllocation *allocation)
{
  BobguiAllocation arrow_allocation;
  int size1, size2, min, nat;
  guint i, ii;

  switch (notebook->tab_pos)
    {
    case BOBGUI_POS_TOP:
    case BOBGUI_POS_BOTTOM:
      arrow_allocation.y = allocation->y;
      arrow_allocation.height = allocation->height;
      for (i = 0; i < 4; i++)
        {
          ii = i < 2 ? i : i ^ 1;

          if (notebook->arrow_widget[ii] == NULL)
            continue;

          bobgui_widget_measure (notebook->arrow_widget[ii],
                              BOBGUI_ORIENTATION_HORIZONTAL,
                              allocation->height,
                              &min, &nat,
                              NULL, NULL);
          if (i < 2)
            {
              arrow_allocation.x = allocation->x;
              arrow_allocation.width = min;
              bobgui_widget_size_allocate (notebook->arrow_widget[ii],
                                        &arrow_allocation,
                                        -1);
              allocation->x += min;
              allocation->width -= min;
            }
          else
            {
              arrow_allocation.x = allocation->x + allocation->width - min;
              arrow_allocation.width = min;
              bobgui_widget_size_allocate (notebook->arrow_widget[ii],
                                        &arrow_allocation,
                                        -1);
              allocation->width -= min;
            }
        }
      break;

    case BOBGUI_POS_LEFT:
    case BOBGUI_POS_RIGHT:
      if (notebook->arrow_widget[0] || notebook->arrow_widget[1])
        {
          bobgui_notebook_measure_arrows (notebook,
                                       BOBGUI_PACK_START,
                                       BOBGUI_ORIENTATION_VERTICAL,
                                       allocation->width,
                                       &min, &nat,
                                       NULL, NULL);
          bobgui_notebook_distribute_arrow_width (notebook, BOBGUI_PACK_START, allocation->width, &size1, &size2);
          arrow_allocation.x = allocation->x;
          arrow_allocation.y = allocation->y;
          arrow_allocation.width = size1;
          arrow_allocation.height = min;
          if (notebook->arrow_widget[0])
            bobgui_widget_size_allocate (notebook->arrow_widget[0], &arrow_allocation, -1);
          arrow_allocation.x += size1;
          arrow_allocation.width = size2;
          if (notebook->arrow_widget[1])
            bobgui_widget_size_allocate (notebook->arrow_widget[1], &arrow_allocation, -1);
          allocation->y += min;
          allocation->height -= min;
        }
      if (notebook->arrow_widget[2] || notebook->arrow_widget[3])
        {
          bobgui_notebook_measure_arrows (notebook,
                                       BOBGUI_PACK_END,
                                       BOBGUI_ORIENTATION_VERTICAL,
                                       allocation->width,
                                       &min, &nat,
                                       NULL, NULL);
          bobgui_notebook_distribute_arrow_width (notebook, BOBGUI_PACK_END, allocation->width, &size1, &size2);
          arrow_allocation.x = allocation->x;
          arrow_allocation.y = allocation->y + allocation->height - min;
          arrow_allocation.width = size1;
          arrow_allocation.height = min;
          if (notebook->arrow_widget[2])
            bobgui_widget_size_allocate (notebook->arrow_widget[2], &arrow_allocation, -1);
          arrow_allocation.x += size1;
          arrow_allocation.width = size2;
          if (notebook->arrow_widget[3])
            bobgui_widget_size_allocate (notebook->arrow_widget[3], &arrow_allocation, -1);
          allocation->height -= min;
        }
      break;

    default:
      g_assert_not_reached ();
      break;
    }
}


static void
bobgui_notebook_tab_space (BobguiNotebook   *notebook,
                        int            notebook_width,
                        int            notebook_height,
                        gboolean      *show_arrows,
                        BobguiAllocation *tabs_allocation,
                        int           *tab_space)
{
  GList *children;
  BobguiPositionType tab_pos = get_effective_tab_pos (notebook);

  children = notebook->children;

  *tabs_allocation = (BobguiAllocation) {0, 0, notebook_width, notebook_height};

  switch (tab_pos)
    {
    case BOBGUI_POS_TOP:
    case BOBGUI_POS_BOTTOM:
      while (children)
        {
          BobguiNotebookPage *page;

          page = children->data;
          children = children->next;

          if (NOTEBOOK_IS_TAB_LABEL_PARENT (notebook, page) &&
              bobgui_widget_get_visible (page->child))
            *tab_space += page->requisition.width;
        }
      break;
    case BOBGUI_POS_RIGHT:
    case BOBGUI_POS_LEFT:
      while (children)
        {
          BobguiNotebookPage *page;

          page = children->data;
          children = children->next;

          if (NOTEBOOK_IS_TAB_LABEL_PARENT (notebook, page) &&
              bobgui_widget_get_visible (page->child))
            *tab_space += page->requisition.height;
        }
      break;

    default:
      g_assert_not_reached ();
      break;
    }

  if (!notebook->scrollable)
    *show_arrows = FALSE;
  else
    {
      switch (tab_pos)
        {
        case BOBGUI_POS_TOP:
        case BOBGUI_POS_BOTTOM:
          if (*tab_space > tabs_allocation->width)
            {
              *show_arrows = TRUE;

              bobgui_notebook_allocate_arrows (notebook, tabs_allocation);

              *tab_space = tabs_allocation->width;
            }
          break;
        case BOBGUI_POS_LEFT:
        case BOBGUI_POS_RIGHT:
          if (*tab_space > tabs_allocation->height)
            {
              *show_arrows = TRUE;

              bobgui_notebook_allocate_arrows (notebook, tabs_allocation);

              *tab_space = tabs_allocation->height;
            }
          break;

        default:
          g_assert_not_reached ();
          break;
        }
    }
}

static void
bobgui_notebook_calculate_shown_tabs (BobguiNotebook          *notebook,
                                   gboolean              show_arrows,
                                   const BobguiAllocation  *tabs_allocation,
                                   int                   tab_space,
                                   GList               **last_child,
                                   int                  *n,
                                   int                  *remaining_space)
{
  GList *children;
  BobguiNotebookPage *page;

  if (show_arrows) /* first_tab <- focus_tab */
    {
      *remaining_space = tab_space;

      if (NOTEBOOK_IS_TAB_LABEL_PARENT (notebook, notebook->cur_page) &&
          bobgui_widget_get_visible (notebook->cur_page->child))
        {
          bobgui_notebook_calc_tabs (notebook,
                                  notebook->focus_tab,
                                  &(notebook->focus_tab),
                                  remaining_space, STEP_NEXT);
        }

      if (tab_space <= 0 || *remaining_space <= 0)
        {
          /* show 1 tab */
          notebook->first_tab = notebook->focus_tab;
          *last_child = bobgui_notebook_search_page (notebook, notebook->focus_tab,
                                                  STEP_NEXT, TRUE);
          *n = 1;
        }
      else
        {
          children = NULL;

          if (notebook->first_tab && notebook->first_tab != notebook->focus_tab)
            {
              /* Is first_tab really predecessor of focus_tab? */
              page = notebook->first_tab->data;
              if (NOTEBOOK_IS_TAB_LABEL_PARENT (notebook, page) &&
                  bobgui_widget_get_visible (page->child))
                for (children = notebook->focus_tab;
                     children && children != notebook->first_tab;
                     children = bobgui_notebook_search_page (notebook,
                                                          children,
                                                          STEP_PREV,
                                                          TRUE));
            }

          if (!children)
            {
              if (NOTEBOOK_IS_TAB_LABEL_PARENT (notebook, notebook->cur_page))
                notebook->first_tab = notebook->focus_tab;
              else
                notebook->first_tab = bobgui_notebook_search_page (notebook, notebook->focus_tab,
                                                            STEP_NEXT, TRUE);
            }
          else
            /* calculate shown tabs counting backwards from the focus tab */
            bobgui_notebook_calc_tabs (notebook,
                                    bobgui_notebook_search_page (notebook,
                                                              notebook->focus_tab,
                                                              STEP_PREV,
                                                              TRUE),
                                    &(notebook->first_tab),
                                    remaining_space,
                                    STEP_PREV);

          if (*remaining_space < 0)
            {
              notebook->first_tab =
                bobgui_notebook_search_page (notebook, notebook->first_tab,
                                          STEP_NEXT, TRUE);
              if (!notebook->first_tab)
                notebook->first_tab = notebook->focus_tab;

              *last_child = bobgui_notebook_search_page (notebook, notebook->focus_tab,
                                                      STEP_NEXT, TRUE);
            }
          else /* focus_tab -> end */
            {
              if (!notebook->first_tab)
                notebook->first_tab = bobgui_notebook_search_page (notebook,
                                                            NULL,
                                                            STEP_NEXT,
                                                            TRUE);
              children = NULL;
              bobgui_notebook_calc_tabs (notebook,
                                      bobgui_notebook_search_page (notebook,
                                                                notebook->focus_tab,
                                                                STEP_NEXT,
                                                                TRUE),
                                      &children,
                                      remaining_space,
                                      STEP_NEXT);

              if (*remaining_space <= 0)
                *last_child = children;
              else /* start <- first_tab */
                {
                  *last_child = NULL;
                  children = NULL;

                  bobgui_notebook_calc_tabs (notebook,
                                          bobgui_notebook_search_page (notebook,
                                                                    notebook->first_tab,
                                                                    STEP_PREV,
                                                                    TRUE),
                                          &children,
                                          remaining_space,
                                          STEP_PREV);

                  if (*remaining_space == 0)
                    notebook->first_tab = children;
                  else
                    notebook->first_tab = bobgui_notebook_search_page(notebook,
                                                               children,
                                                               STEP_NEXT,
                                                               TRUE);
                }
            }

          if (*remaining_space < 0)
            {
              /* calculate number of tabs */
              *remaining_space = - (*remaining_space);
              *n = 0;

              for (children = notebook->first_tab;
                   children && children != *last_child;
                   children = bobgui_notebook_search_page (notebook, children,
                                                        STEP_NEXT, TRUE))
                (*n)++;
            }
          else
            *remaining_space = 0;
        }

      /* unmap all non-visible tabs */
      for (children = bobgui_notebook_search_page (notebook, NULL,
                                                STEP_NEXT, TRUE);
           children && children != notebook->first_tab;
           children = bobgui_notebook_search_page (notebook, children,
                                                STEP_NEXT, TRUE))
        {
          page = children->data;

          if (page->tab_label &&
              NOTEBOOK_IS_TAB_LABEL_PARENT (notebook, page))
            bobgui_widget_set_child_visible (page->tab_widget, FALSE);
        }

      for (children = *last_child; children;
           children = bobgui_notebook_search_page (notebook, children,
                                                STEP_NEXT, TRUE))
        {
          page = children->data;

          if (page->tab_label &&
              NOTEBOOK_IS_TAB_LABEL_PARENT (notebook, page))
            bobgui_widget_set_child_visible (page->tab_widget, FALSE);
        }
    }
  else /* !show_arrows */
    {
      BobguiOrientation tab_expand_orientation;
      *n = 0;

      if (notebook->tab_pos == BOBGUI_POS_TOP || notebook->tab_pos == BOBGUI_POS_BOTTOM)
        {
          tab_expand_orientation = BOBGUI_ORIENTATION_HORIZONTAL;
          *remaining_space = tabs_allocation->width - tab_space;
        }
      else
        {
          tab_expand_orientation = BOBGUI_ORIENTATION_VERTICAL;
          *remaining_space = tabs_allocation->height - tab_space;
        }
      children = notebook->children;
      notebook->first_tab = bobgui_notebook_search_page (notebook, NULL,
                                                  STEP_NEXT, TRUE);
      while (children)
        {
          page = children->data;
          children = children->next;

          if (!NOTEBOOK_IS_TAB_LABEL_PARENT (notebook, page) ||
              !bobgui_widget_get_visible (page->child))
            continue;

          if (page->expand ||
              (bobgui_widget_compute_expand (page->tab_label, tab_expand_orientation)))
            (*n)++;
        }
    }
}

static gboolean
get_allocate_at_bottom (BobguiWidget *widget,
                        int        search_direction)
{
  gboolean is_rtl = (bobgui_widget_get_direction (widget) == BOBGUI_TEXT_DIR_RTL);
  BobguiPositionType tab_pos = get_effective_tab_pos (BOBGUI_NOTEBOOK (widget));

  switch (tab_pos)
    {
    case BOBGUI_POS_TOP:
    case BOBGUI_POS_BOTTOM:
      if (!is_rtl)
        return (search_direction == STEP_PREV);
      else
        return (search_direction == STEP_NEXT);

    case BOBGUI_POS_RIGHT:
    case BOBGUI_POS_LEFT:
      return (search_direction == STEP_PREV);

    default:
      g_assert_not_reached ();
      return FALSE;
    }
}

static void
bobgui_notebook_calculate_tabs_allocation (BobguiNotebook          *notebook,
                                        GList               **children,
                                        GList                *last_child,
                                        gboolean              showarrow,
                                        int                   direction,
                                        int                  *remaining_space,
                                        int                  *expanded_tabs,
                                        const BobguiAllocation  *allocation)
{
  BobguiWidget *widget;
  BobguiNotebookPage *page;
  gboolean allocate_at_bottom;
  int tab_extra_space;
  BobguiPositionType tab_pos;
  int left_x, right_x, top_y, bottom_y, anchor;
  gboolean gap_left, packing_changed;
  BobguiAllocation child_allocation;
  BobguiOrientation tab_expand_orientation;
  graphene_rect_t drag_bounds;

  g_assert (notebook->cur_page != NULL);

  widget = BOBGUI_WIDGET (notebook);
  tab_pos = get_effective_tab_pos (notebook);
  allocate_at_bottom = get_allocate_at_bottom (widget, direction);

  child_allocation = *allocation;

  switch (tab_pos)
    {
    case BOBGUI_POS_BOTTOM:
    case BOBGUI_POS_TOP:
      if (allocate_at_bottom)
        child_allocation.x += allocation->width;
      anchor = child_allocation.x;
      break;

    case BOBGUI_POS_RIGHT:
    case BOBGUI_POS_LEFT:
      if (allocate_at_bottom)
        child_allocation.y += allocation->height;
      anchor = child_allocation.y;
      break;

    default:
      g_assert_not_reached ();
      anchor = 0;
      break;
    }

  if (!bobgui_widget_compute_bounds (notebook->cur_page->tab_widget, notebook->cur_page->tab_widget, &drag_bounds))
    graphene_rect_init_from_rect (&drag_bounds, graphene_rect_zero ());

  left_x   = CLAMP (notebook->mouse_x - notebook->drag_offset_x,
                    allocation->x, allocation->x + allocation->width - drag_bounds.size.width);
  top_y    = CLAMP (notebook->mouse_y - notebook->drag_offset_y,
                    allocation->y, allocation->y + allocation->height - drag_bounds.size.height);
  right_x  = left_x + drag_bounds.size.width;
  bottom_y = top_y + drag_bounds.size.height;
  gap_left = packing_changed = FALSE;

  if (notebook->tab_pos == BOBGUI_POS_TOP || notebook->tab_pos == BOBGUI_POS_BOTTOM)
    tab_expand_orientation = BOBGUI_ORIENTATION_HORIZONTAL;
  else
    tab_expand_orientation = BOBGUI_ORIENTATION_VERTICAL;

  while (*children && *children != last_child)
    {
      page = (*children)->data;

      if (direction == STEP_NEXT)
        *children = bobgui_notebook_search_page (notebook, *children, direction, TRUE);
      else
        {
          *children = (*children)->next;
          continue;
        }

      if (!NOTEBOOK_IS_TAB_LABEL_PARENT (notebook, page))
        continue;

      tab_extra_space = 0;
      if (*expanded_tabs && (showarrow || page->expand || bobgui_widget_compute_expand (page->tab_label, tab_expand_orientation)))
        {
          tab_extra_space = *remaining_space / *expanded_tabs;
          *remaining_space -= tab_extra_space;
          (*expanded_tabs)--;
        }

      switch (tab_pos)
        {
        case BOBGUI_POS_TOP:
        case BOBGUI_POS_BOTTOM:
          child_allocation.width = MAX (1, page->requisition.width + tab_extra_space);

          /* make sure that the reordered tab doesn't go past the last position */
          if (notebook->operation == DRAG_OPERATION_REORDER &&
              !gap_left && packing_changed)
            {
              if (!allocate_at_bottom)
                {
                  if (left_x >= anchor)
                    {
                      left_x = notebook->drag_surface_x = anchor;
                      anchor += drag_bounds.size.width;
                    }
                }
              else
                {
                  if (right_x <= anchor)
                    {
                      anchor -= drag_bounds.size.width;
                      left_x = notebook->drag_surface_x = anchor;
                    }
                }

              gap_left = TRUE;
            }

          if (notebook->operation == DRAG_OPERATION_REORDER && page == notebook->cur_page)
            {
              notebook->drag_surface_x = left_x;
              notebook->drag_surface_y = child_allocation.y;
            }
          else
            {
              if (allocate_at_bottom)
                anchor -= child_allocation.width;

              if (notebook->operation == DRAG_OPERATION_REORDER)
                {
                  if (!allocate_at_bottom &&
                      left_x >= anchor &&
                      left_x <= anchor + child_allocation.width / 2)
                    anchor += drag_bounds.size.width;
                  else if (allocate_at_bottom &&
                           right_x >= anchor + child_allocation.width / 2 &&
                           right_x <= anchor + child_allocation.width)
                    anchor -= drag_bounds.size.width;
                }

              child_allocation.x = anchor;
            }

          break;
        case BOBGUI_POS_LEFT:
        case BOBGUI_POS_RIGHT:
          child_allocation.height = MAX (1, page->requisition.height + tab_extra_space);

          /* make sure that the reordered tab doesn't go past the last position */
          if (notebook->operation == DRAG_OPERATION_REORDER &&
              !gap_left && packing_changed)
            {
              if (!allocate_at_bottom && top_y >= anchor)
                {
                  top_y = notebook->drag_surface_y = anchor;
                  anchor += drag_bounds.size.height;
                }

              gap_left = TRUE;
            }

          if (notebook->operation == DRAG_OPERATION_REORDER && page == notebook->cur_page)
            {
              notebook->drag_surface_x = child_allocation.x;
              notebook->drag_surface_y = top_y;
            }
          else
            {
              if (allocate_at_bottom)
                anchor -= child_allocation.height;

              if (notebook->operation == DRAG_OPERATION_REORDER)
                {
                  if (!allocate_at_bottom &&
                      top_y >= anchor &&
                      top_y <= anchor + child_allocation.height / 2)
                    anchor += drag_bounds.size.height;
                  else if (allocate_at_bottom &&
                           bottom_y >= anchor + child_allocation.height / 2 &&
                           bottom_y <= anchor + child_allocation.height)
                    anchor -= drag_bounds.size.height;
                }

              child_allocation.y = anchor;
            }
          break;

        default:
          g_assert_not_reached ();
          break;
        }

      /* set child visible */
      if (page->tab_label)
        bobgui_widget_set_child_visible (page->tab_widget, TRUE);

      if (page == notebook->cur_page && notebook->operation == DRAG_OPERATION_REORDER)
        {
          BobguiAllocation fixed_allocation = { notebook->drag_surface_x, notebook->drag_surface_y,
                                             child_allocation.width, child_allocation.height };
          bobgui_widget_size_allocate (page->tab_widget, &fixed_allocation, -1);
        }
      else if (page == notebook->detached_tab && notebook->operation == DRAG_OPERATION_DETACH)
        {
          /* needs to be allocated at 0,0
           * to be shown in the drag window */
          BobguiAllocation fixed_allocation = { 0, 0, child_allocation.width, child_allocation.height };
          bobgui_widget_size_allocate (page->tab_widget, &fixed_allocation, -1);
        }
      else if (bobgui_notebook_page_tab_label_is_visible (page))
        {
          bobgui_widget_size_allocate (page->tab_widget, &child_allocation, -1);
        }

      /* calculate whether to leave a gap based on reorder operation or not */
      switch (tab_pos)
        {
        case BOBGUI_POS_TOP:
        case BOBGUI_POS_BOTTOM:
          if (notebook->operation != DRAG_OPERATION_REORDER || page != notebook->cur_page)
            {
              if (notebook->operation == DRAG_OPERATION_REORDER)
                {
                  if (!allocate_at_bottom &&
                      left_x >  anchor + child_allocation.width / 2 &&
                      left_x <= anchor + child_allocation.width)
                    anchor += drag_bounds.size.width;
                  else if (allocate_at_bottom &&
                           right_x >= anchor &&
                           right_x <= anchor + child_allocation.width / 2)
                    anchor -= drag_bounds.size.width;
                }

              if (!allocate_at_bottom)
                anchor += child_allocation.width;
            }

          break;
        case BOBGUI_POS_LEFT:
        case BOBGUI_POS_RIGHT:
          if (notebook->operation != DRAG_OPERATION_REORDER || page != notebook->cur_page)
            {
              if (notebook->operation == DRAG_OPERATION_REORDER)
                {
                  if (!allocate_at_bottom &&
                      top_y >= anchor + child_allocation.height / 2 &&
                      top_y <= anchor + child_allocation.height)
                    anchor += drag_bounds.size.height;
                  else if (allocate_at_bottom &&
                           bottom_y >= anchor &&
                           bottom_y <= anchor + child_allocation.height / 2)
                    anchor -= drag_bounds.size.height;
                }

              if (!allocate_at_bottom)
                anchor += child_allocation.height;
            }

          break;
        default:
          g_assert_not_reached ();
      break;
        }
    }

  /* Don't move the current tab past the last position during tabs reordering */
  if (notebook->operation == DRAG_OPERATION_REORDER &&
      direction == STEP_NEXT)
    {
      switch (tab_pos)
        {
        case BOBGUI_POS_TOP:
        case BOBGUI_POS_BOTTOM:
          if (allocate_at_bottom)
            anchor -= drag_bounds.size.width;

          if ((!allocate_at_bottom && notebook->drag_surface_x > anchor) ||
              (allocate_at_bottom && notebook->drag_surface_x < anchor))
            notebook->drag_surface_x = anchor;
          break;
        case BOBGUI_POS_LEFT:
        case BOBGUI_POS_RIGHT:
          if (allocate_at_bottom)
            anchor -= drag_bounds.size.height;

          if ((!allocate_at_bottom && notebook->drag_surface_y > anchor) ||
              (allocate_at_bottom && notebook->drag_surface_y < anchor))
            notebook->drag_surface_y = anchor;
          break;
        default:
          g_assert_not_reached ();
          break;
        }
    }
}

static void
bobgui_notebook_pages_allocate (BobguiNotebook *notebook,
                             int          width,
                             int          height)
{
  GList *children = NULL;
  GList *last_child = NULL;
  gboolean showarrow = FALSE;
  BobguiAllocation tabs_allocation;
  int tab_space, remaining_space;
  int expanded_tabs;

  if (!notebook->show_tabs || !bobgui_notebook_has_current_page (notebook))
    return;

  tab_space = remaining_space = 0;
  expanded_tabs = 1;

  bobgui_notebook_tab_space (notebook, width, height,
                          &showarrow, &tabs_allocation, &tab_space);

  bobgui_notebook_calculate_shown_tabs (notebook, showarrow,
                                     &tabs_allocation, tab_space, &last_child,
                                     &expanded_tabs, &remaining_space);

  children = notebook->first_tab;
  bobgui_notebook_calculate_tabs_allocation (notebook, &children, last_child,
                                          showarrow, STEP_NEXT,
                                          &remaining_space, &expanded_tabs, &tabs_allocation);
  if (children && children != last_child)
    {
      children = notebook->children;
      bobgui_notebook_calculate_tabs_allocation (notebook, &children, last_child,
                                              showarrow, STEP_PREV,
                                              &remaining_space, &expanded_tabs, &tabs_allocation);
    }

  if (!notebook->first_tab)
    notebook->first_tab = notebook->children;
}

static void
bobgui_notebook_calc_tabs (BobguiNotebook  *notebook,
                        GList        *start,
                        GList       **end,
                        int          *tab_space,
                        guint         direction)
{
  BobguiNotebookPage *page = NULL;
  GList *children;
  GList *last_calculated_child = NULL;
  BobguiPositionType tab_pos = get_effective_tab_pos (notebook);

  if (!start)
    return;

  children = start;

  switch (tab_pos)
    {
    case BOBGUI_POS_TOP:
    case BOBGUI_POS_BOTTOM:
      while (children)
        {
          page = children->data;
          if (NOTEBOOK_IS_TAB_LABEL_PARENT (notebook, page) &&
              bobgui_widget_get_visible (page->child))
            {
              *tab_space -= page->requisition.width;
              if (*tab_space < 0 || children == *end)
                {
                  if (*tab_space < 0)
                    {
                      *tab_space = - (*tab_space +
                                      page->requisition.width);

                      if (*tab_space == 0 && direction == STEP_PREV)
                        children = last_calculated_child;

                      *end = children;
                    }
                  return;
                }

              last_calculated_child = children;
            }
          if (direction == STEP_NEXT)
            children = children->next;
          else
            children = children->prev;
        }
      break;
    case BOBGUI_POS_LEFT:
    case BOBGUI_POS_RIGHT:
      while (children)
        {
          page = children->data;
          if (NOTEBOOK_IS_TAB_LABEL_PARENT (notebook, page) &&
              bobgui_widget_get_visible (page->child))
            {
              *tab_space -= page->requisition.height;
              if (*tab_space < 0 || children == *end)
                {
                  if (*tab_space < 0)
                    {
                      *tab_space = - (*tab_space + page->requisition.height);

                      if (*tab_space == 0 && direction == STEP_PREV)
                        children = last_calculated_child;

                      *end = children;
                    }
                  return;
                }

              last_calculated_child = children;
            }
          if (direction == STEP_NEXT)
            children = children->next;
          else
            children = children->prev;
        }
      break;
    default:
      g_assert_not_reached ();
      break;
    }
}

/* Private BobguiNotebook Page Switch Methods:
 *
 * bobgui_notebook_real_switch_page
 */
static void
bobgui_notebook_real_switch_page (BobguiNotebook     *notebook,
                               BobguiWidget*       child,
                               guint            page_num)
{
  GList *list = bobgui_notebook_find_child (notebook, BOBGUI_WIDGET (child));
  BobguiNotebookPage *page = BOBGUI_NOTEBOOK_PAGE_FROM_LIST (list);
  gboolean child_has_focus;

  if (notebook->cur_page == page || !bobgui_widget_get_visible (BOBGUI_WIDGET (child)))
    return;

  /* save the value here, changing visibility changes focus */
  child_has_focus = notebook->child_has_focus;

  if (notebook->cur_page)
    {
      BobguiRoot *root = bobgui_widget_get_root (BOBGUI_WIDGET (notebook));
      BobguiWidget *focus = NULL;
      if (root)
        focus = bobgui_root_get_focus (root);
      if (focus)
        child_has_focus = bobgui_widget_is_ancestor (focus, notebook->cur_page->child);
      bobgui_widget_unset_state_flags (notebook->cur_page->tab_widget, BOBGUI_STATE_FLAG_CHECKED);

      bobgui_accessible_update_state (BOBGUI_ACCESSIBLE (notebook->cur_page->tab_widget),
                                   BOBGUI_ACCESSIBLE_STATE_SELECTED, FALSE,
                                   -1);
    }

  notebook->cur_page = page;
  bobgui_widget_set_state_flags (page->tab_widget, BOBGUI_STATE_FLAG_CHECKED, FALSE);
  bobgui_widget_set_visible (notebook->header_widget, notebook->show_tabs);

  if (bobgui_widget_get_realized (BOBGUI_WIDGET (notebook)))
    bobgui_widget_realize_at_context (notebook->cur_page->tab_widget);

  bobgui_accessible_update_state (BOBGUI_ACCESSIBLE (notebook->cur_page->tab_widget),
                               BOBGUI_ACCESSIBLE_STATE_SELECTED, TRUE,
                               -1);

  if (!notebook->focus_tab ||
      notebook->focus_tab->data != (gpointer) notebook->cur_page)
    notebook->focus_tab =
      g_list_find (notebook->children, notebook->cur_page);

  bobgui_stack_set_visible_child (BOBGUI_STACK (notebook->stack_widget), notebook->cur_page->child);
  bobgui_widget_set_child_visible (notebook->cur_page->tab_widget, TRUE);

  /* If the focus was on the previous page, move it to the first
   * element on the new page, if possible, or if not, to the
   * notebook itself.
   */
  if (child_has_focus)
    {
      if (notebook->cur_page->last_focus_child &&
          bobgui_widget_is_ancestor (notebook->cur_page->last_focus_child, notebook->cur_page->child))
        bobgui_widget_grab_focus (notebook->cur_page->last_focus_child);
      else
        if (!bobgui_widget_child_focus (notebook->cur_page->child, BOBGUI_DIR_TAB_FORWARD))
          bobgui_widget_grab_focus (BOBGUI_WIDGET (notebook));
    }

  update_arrow_state (notebook);

  bobgui_widget_queue_resize (BOBGUI_WIDGET (notebook));
  bobgui_widget_queue_resize (notebook->tabs_widget);
  g_object_notify_by_pspec (G_OBJECT (notebook), properties[PROP_PAGE]);
}

/* Private BobguiNotebook Page Switch Functions:
 *
 * bobgui_notebook_switch_page
 * bobgui_notebook_page_select
 * bobgui_notebook_switch_focus_tab
 * bobgui_notebook_menu_switch_page
 */
static void
bobgui_notebook_switch_page (BobguiNotebook     *notebook,
                          BobguiNotebookPage *page)
{
  guint page_num;

  if (notebook->cur_page == page)
    return;

  page_num = g_list_index (notebook->children, page);

  g_signal_emit (notebook,
                 notebook_signals[SWITCH_PAGE],
                 0,
                 page->child,
                 page_num);
}

static int
bobgui_notebook_page_select (BobguiNotebook *notebook,
                          gboolean     move_focus)
{
  BobguiNotebookPage *page;
  BobguiDirectionType dir;
  BobguiPositionType tab_pos = get_effective_tab_pos (notebook);

  if (!notebook->focus_tab)
    return FALSE;

  page = notebook->focus_tab->data;
  bobgui_notebook_switch_page (notebook, page);

  if (move_focus)
    {
      switch (tab_pos)
        {
        case BOBGUI_POS_TOP:
          dir = BOBGUI_DIR_DOWN;
          break;
        case BOBGUI_POS_BOTTOM:
          dir = BOBGUI_DIR_UP;
          break;
        case BOBGUI_POS_LEFT:
          dir = BOBGUI_DIR_RIGHT;
          break;
        case BOBGUI_POS_RIGHT:
          dir = BOBGUI_DIR_LEFT;
          break;
        default:
          g_assert_not_reached ();
          dir = BOBGUI_DIR_DOWN;
          break;
        }

      if (bobgui_widget_child_focus (page->child, dir))
        return TRUE;
    }
  return FALSE;
}

static void
bobgui_notebook_switch_focus_tab (BobguiNotebook *notebook,
                               GList       *new_child)
{
  BobguiNotebookPage *page;

  if (notebook->focus_tab == new_child)
    return;

  notebook->focus_tab = new_child;

  if (!notebook->show_tabs || !notebook->focus_tab)
    return;

  page = notebook->focus_tab->data;
  bobgui_notebook_switch_page (notebook, page);
}

static void
bobgui_notebook_menu_switch_page (BobguiWidget       *widget,
                               BobguiNotebookPage *page)
{
  BobguiNotebook *notebook;
  GList *children;
  guint page_num;

  notebook = BOBGUI_NOTEBOOK (bobgui_widget_get_ancestor (widget, BOBGUI_TYPE_NOTEBOOK));

  bobgui_popover_popdown (BOBGUI_POPOVER (notebook->menu));

  if (notebook->cur_page == page)
    return;

  page_num = 0;
  children = notebook->children;
  while (children && children->data != page)
    {
      children = children->next;
      page_num++;
    }

  g_signal_emit (notebook,
                 notebook_signals[SWITCH_PAGE],
                 0,
                 page->child,
                 page_num);
}

/* Private BobguiNotebook Menu Functions:
 *
 * bobgui_notebook_menu_item_create
 * bobgui_notebook_menu_item_recreate
 * bobgui_notebook_menu_label_unparent
 */
static void
bobgui_notebook_menu_item_create (BobguiNotebook *notebook,
                               BobguiNotebookPage *page)
{
  BobguiWidget *menu_item;

  if (page->default_menu)
    {
      if (BOBGUI_IS_LABEL (page->tab_label))
        page->menu_label = bobgui_label_new (bobgui_label_get_text (BOBGUI_LABEL (page->tab_label)));
      else
        page->menu_label = bobgui_label_new ("");
      g_object_ref_sink (page->menu_label);
      bobgui_widget_set_halign (page->menu_label, BOBGUI_ALIGN_START);
      bobgui_widget_set_valign (page->menu_label, BOBGUI_ALIGN_CENTER);
    }

  menu_item = bobgui_button_new ();
  bobgui_button_set_has_frame (BOBGUI_BUTTON (menu_item), FALSE);
  bobgui_button_set_child (BOBGUI_BUTTON (menu_item), page->menu_label);
  bobgui_box_append (BOBGUI_BOX (notebook->menu_box), menu_item);
  g_signal_connect (menu_item, "clicked",
                    G_CALLBACK (bobgui_notebook_menu_switch_page), page);
  if (!bobgui_widget_get_visible (page->child))
    bobgui_widget_set_visible (menu_item, FALSE);
}

static void
bobgui_notebook_menu_item_recreate (BobguiNotebook *notebook,
                                 GList       *list)
{
  BobguiNotebookPage *page = list->data;
  BobguiWidget *menu_item = bobgui_widget_get_parent (page->menu_label);

  bobgui_button_set_child (BOBGUI_BUTTON (menu_item), NULL);
  bobgui_box_remove (BOBGUI_BOX (notebook->menu_box), menu_item);
  bobgui_notebook_menu_item_create (notebook, page);
}

static void
bobgui_notebook_menu_label_unparent (BobguiWidget *widget)
{
  bobgui_button_set_child (BOBGUI_BUTTON (widget), NULL);
}

/* Public BobguiNotebook Page Insert/Remove Methods :
 *
 * bobgui_notebook_append_page
 * bobgui_notebook_append_page_menu
 * bobgui_notebook_prepend_page
 * bobgui_notebook_prepend_page_menu
 * bobgui_notebook_insert_page
 * bobgui_notebook_insert_page_menu
 * bobgui_notebook_remove_page
 */
/**
 * bobgui_notebook_append_page:
 * @notebook: a `BobguiNotebook`
 * @child: the `BobguiWidget` to use as the contents of the page
 * @tab_label: (nullable): the `BobguiWidget` to be used as the label
 *   for the page, or %NULL to use the default label, “page N”
 *
 * Appends a page to @notebook.
 *
 * Returns: the index (starting from 0) of the appended
 *   page in the notebook, or -1 if function fails
 */
int
bobgui_notebook_append_page (BobguiNotebook *notebook,
                          BobguiWidget   *child,
                          BobguiWidget   *tab_label)
{
  g_return_val_if_fail (BOBGUI_IS_NOTEBOOK (notebook), -1);
  g_return_val_if_fail (BOBGUI_IS_WIDGET (child), -1);
  g_return_val_if_fail (tab_label == NULL || BOBGUI_IS_WIDGET (tab_label), -1);

  return bobgui_notebook_insert_page_menu (notebook, child, tab_label, NULL, -1);
}

/**
 * bobgui_notebook_append_page_menu:
 * @notebook: a `BobguiNotebook`
 * @child: the `BobguiWidget` to use as the contents of the page
 * @tab_label: (nullable): the `BobguiWidget` to be used as the label
 *   for the page, or %NULL to use the default label, “page N”
 * @menu_label: (nullable): the widget to use as a label for the
 *   page-switch menu, if that is enabled. If %NULL, and @tab_label
 *   is a `BobguiLabel` or %NULL, then the menu label will be a newly
 *   created label with the same text as @tab_label; if @tab_label
 *   is not a `BobguiLabel`, @menu_label must be specified if the
 *   page-switch menu is to be used.
 *
 * Appends a page to @notebook, specifying the widget to use as the
 * label in the popup menu.
 *
 * Returns: the index (starting from 0) of the appended
 *   page in the notebook, or -1 if function fails
 */
int
bobgui_notebook_append_page_menu (BobguiNotebook *notebook,
                               BobguiWidget   *child,
                               BobguiWidget   *tab_label,
                               BobguiWidget   *menu_label)
{
  g_return_val_if_fail (BOBGUI_IS_NOTEBOOK (notebook), -1);
  g_return_val_if_fail (BOBGUI_IS_WIDGET (child), -1);
  g_return_val_if_fail (tab_label == NULL || BOBGUI_IS_WIDGET (tab_label), -1);
  g_return_val_if_fail (menu_label == NULL || BOBGUI_IS_WIDGET (menu_label), -1);

  return bobgui_notebook_insert_page_menu (notebook, child, tab_label, menu_label, -1);
}

/**
 * bobgui_notebook_prepend_page:
 * @notebook: a `BobguiNotebook`
 * @child: the `BobguiWidget` to use as the contents of the page
 * @tab_label: (nullable): the `BobguiWidget` to be used as the label
 *   for the page, or %NULL to use the default label, “page N”
 *
 * Prepends a page to @notebook.
 *
 * Returns: the index (starting from 0) of the prepended
 *   page in the notebook, or -1 if function fails
 */
int
bobgui_notebook_prepend_page (BobguiNotebook *notebook,
                           BobguiWidget   *child,
                           BobguiWidget   *tab_label)
{
  g_return_val_if_fail (BOBGUI_IS_NOTEBOOK (notebook), -1);
  g_return_val_if_fail (BOBGUI_IS_WIDGET (child), -1);
  g_return_val_if_fail (tab_label == NULL || BOBGUI_IS_WIDGET (tab_label), -1);

  return bobgui_notebook_insert_page_menu (notebook, child, tab_label, NULL, 0);
}

/**
 * bobgui_notebook_prepend_page_menu:
 * @notebook: a `BobguiNotebook`
 * @child: the `BobguiWidget` to use as the contents of the page
 * @tab_label: (nullable): the `BobguiWidget` to be used as the label
 *   for the page, or %NULL to use the default label, “page N”
 * @menu_label: (nullable): the widget to use as a label for the
 *   page-switch menu, if that is enabled. If %NULL, and @tab_label
 *   is a `BobguiLabel` or %NULL, then the menu label will be a newly
 *   created label with the same text as @tab_label; if @tab_label
 *   is not a `BobguiLabel`, @menu_label must be specified if the
 *   page-switch menu is to be used.
 *
 * Prepends a page to @notebook, specifying the widget to use as the
 * label in the popup menu.
 *
 * Returns: the index (starting from 0) of the prepended
 *   page in the notebook, or -1 if function fails
 */
int
bobgui_notebook_prepend_page_menu (BobguiNotebook *notebook,
                                BobguiWidget   *child,
                                BobguiWidget   *tab_label,
                                BobguiWidget   *menu_label)
{
  g_return_val_if_fail (BOBGUI_IS_NOTEBOOK (notebook), -1);
  g_return_val_if_fail (BOBGUI_IS_WIDGET (child), -1);
  g_return_val_if_fail (tab_label == NULL || BOBGUI_IS_WIDGET (tab_label), -1);
  g_return_val_if_fail (menu_label == NULL || BOBGUI_IS_WIDGET (menu_label), -1);

  return bobgui_notebook_insert_page_menu (notebook, child, tab_label, menu_label, 0);
}

/**
 * bobgui_notebook_insert_page:
 * @notebook: a `BobguiNotebook`
 * @child: the `BobguiWidget` to use as the contents of the page
 * @tab_label: (nullable): the `BobguiWidget` to be used as the label
 *   for the page, or %NULL to use the default label, “page N”
 * @position: the index (starting at 0) at which to insert the page,
 *   or -1 to append the page after all other pages
 *
 * Insert a page into @notebook at the given position.
 *
 * Returns: the index (starting from 0) of the inserted
 *   page in the notebook, or -1 if function fails
 */
int
bobgui_notebook_insert_page (BobguiNotebook *notebook,
                          BobguiWidget   *child,
                          BobguiWidget   *tab_label,
                          int          position)
{
  g_return_val_if_fail (BOBGUI_IS_NOTEBOOK (notebook), -1);
  g_return_val_if_fail (BOBGUI_IS_WIDGET (child), -1);
  g_return_val_if_fail (tab_label == NULL || BOBGUI_IS_WIDGET (tab_label), -1);

  return bobgui_notebook_insert_page_menu (notebook, child, tab_label, NULL, position);
}


static int
bobgui_notebook_page_compare_tab (gconstpointer a,
                               gconstpointer b)
{
  return (((BobguiNotebookPage *) a)->tab_label != b);
}

static gboolean
bobgui_notebook_mnemonic_activate_switch_page (BobguiWidget *child,
                                            gboolean overload,
                                            gpointer data)
{
  BobguiNotebook *notebook = BOBGUI_NOTEBOOK (data);
  GList *list;

  list = g_list_find_custom (notebook->children, child,
                             bobgui_notebook_page_compare_tab);
  if (list)
    {
      BobguiNotebookPage *page = list->data;

      bobgui_widget_grab_focus (BOBGUI_WIDGET (notebook));    /* Do this first to avoid focusing new page */
      bobgui_notebook_switch_page (notebook, page);
      focus_tabs_in (notebook);
    }

  return TRUE;
}

/**
 * bobgui_notebook_insert_page_menu:
 * @notebook: a `BobguiNotebook`
 * @child: the `BobguiWidget` to use as the contents of the page
 * @tab_label: (nullable): the `BobguiWidget` to be used as the label
 *   for the page, or %NULL to use the default label, “page N”
 * @menu_label: (nullable): the widget to use as a label for the
 *   page-switch menu, if that is enabled. If %NULL, and @tab_label
 *   is a `BobguiLabel` or %NULL, then the menu label will be a newly
 *   created label with the same text as @tab_label; if @tab_label
 *   is not a `BobguiLabel`, @menu_label must be specified if the
 *   page-switch menu is to be used.
 * @position: the index (starting at 0) at which to insert the page,
 *   or -1 to append the page after all other pages.
 *
 * Insert a page into @notebook at the given position, specifying
 * the widget to use as the label in the popup menu.
 *
 * Returns: the index (starting from 0) of the inserted
 *   page in the notebook
 */
int
bobgui_notebook_insert_page_menu (BobguiNotebook *notebook,
                               BobguiWidget   *child,
                               BobguiWidget   *tab_label,
                               BobguiWidget   *menu_label,
                               int          position)
{
  BobguiNotebookClass *class;

  g_return_val_if_fail (BOBGUI_IS_NOTEBOOK (notebook), -1);
  g_return_val_if_fail (BOBGUI_IS_WIDGET (child), -1);
  g_return_val_if_fail (tab_label == NULL || BOBGUI_IS_WIDGET (tab_label), -1);
  g_return_val_if_fail (menu_label == NULL || BOBGUI_IS_WIDGET (menu_label), -1);

  class = BOBGUI_NOTEBOOK_GET_CLASS (notebook);

  return (class->insert_page) (notebook, child, tab_label, menu_label, position);
}

/**
 * bobgui_notebook_remove_page:
 * @notebook: a `BobguiNotebook`
 * @page_num: the index of a notebook page, starting
 *   from 0. If -1, the last page will be removed.
 *
 * Removes a page from the notebook given its index
 * in the notebook.
 */
void
bobgui_notebook_remove_page (BobguiNotebook *notebook,
                          int          page_num)
{
  GList *list = NULL;

  g_return_if_fail (BOBGUI_IS_NOTEBOOK (notebook));

  if (page_num >= 0)
    list = g_list_nth (notebook->children, page_num);
  else
    list = g_list_last (notebook->children);

  if (list)
    bobgui_notebook_remove (notebook, ((BobguiNotebookPage *) list->data)->child);
}

/* Public BobguiNotebook Page Switch Methods :
 * bobgui_notebook_get_current_page
 * bobgui_notebook_page_num
 * bobgui_notebook_set_current_page
 * bobgui_notebook_next_page
 * bobgui_notebook_prev_page
 */

/**
 * bobgui_notebook_get_current_page: (get-property page)
 * @notebook: a `BobguiNotebook`
 *
 * Returns the page number of the current page.
 *
 * Returns: the index (starting from 0) of the current
 *   page in the notebook. If the notebook has no pages,
 *   then -1 will be returned.
 */
int
bobgui_notebook_get_current_page (BobguiNotebook *notebook)
{
  g_return_val_if_fail (BOBGUI_IS_NOTEBOOK (notebook), -1);

  if (!notebook->cur_page)
    return -1;

  return g_list_index (notebook->children, notebook->cur_page);
}

/**
 * bobgui_notebook_get_nth_page:
 * @notebook: a `BobguiNotebook`
 * @page_num: the index of a page in the notebook, or -1
 *   to get the last page
 *
 * Returns the child widget contained in page number @page_num.
 *
 * Returns: (nullable) (transfer none): the child widget, or %NULL if @page_num
 * is out of bounds
 */
BobguiWidget*
bobgui_notebook_get_nth_page (BobguiNotebook *notebook,
                           int          page_num)
{
  BobguiNotebookPage *page;
  GList *list;

  g_return_val_if_fail (BOBGUI_IS_NOTEBOOK (notebook), NULL);

  if (page_num >= 0)
    list = g_list_nth (notebook->children, page_num);
  else
    list = g_list_last (notebook->children);

  if (list)
    {
      page = list->data;
      return page->child;
    }

  return NULL;
}

/**
 * bobgui_notebook_get_n_pages:
 * @notebook: a `BobguiNotebook`
 *
 * Gets the number of pages in a notebook.
 *
 * Returns: the number of pages in the notebook
 */
int
bobgui_notebook_get_n_pages (BobguiNotebook *notebook)
{
  g_return_val_if_fail (BOBGUI_IS_NOTEBOOK (notebook), 0);

  return g_list_length (notebook->children);
}

/**
 * bobgui_notebook_page_num:
 * @notebook: a `BobguiNotebook`
 * @child: a `BobguiWidget`
 *
 * Finds the index of the page which contains the given child
 * widget.
 *
 * Returns: the index of the page containing @child, or
 *   -1 if @child is not in the notebook
 */
int
bobgui_notebook_page_num (BobguiNotebook      *notebook,
                       BobguiWidget        *child)
{
  GList *children;
  int num;

  g_return_val_if_fail (BOBGUI_IS_NOTEBOOK (notebook), -1);

  num = 0;
  children = notebook->children;
  while (children)
    {
      BobguiNotebookPage *page =  children->data;

      if (page->child == child)
        return num;

      children = children->next;
      num++;
    }

  return -1;
}

/**
 * bobgui_notebook_set_current_page: (set-property page)
 * @notebook: a `BobguiNotebook`
 * @page_num: index of the page to switch to, starting from 0.
 *   If negative, the last page will be used. If greater
 *   than the number of pages in the notebook, nothing
 *   will be done.
 *
 * Switches to the page number @page_num.
 *
 * Note that due to historical reasons, BobguiNotebook refuses
 * to switch to a page unless the child widget is visible.
 * Therefore, it is recommended to show child widgets before
 * adding them to a notebook.
 */
void
bobgui_notebook_set_current_page (BobguiNotebook *notebook,
                               int          page_num)
{
  GList *list;

  g_return_if_fail (BOBGUI_IS_NOTEBOOK (notebook));

  if (page_num < 0)
    page_num = g_list_length (notebook->children) - 1;

  list = g_list_nth (notebook->children, page_num);
  if (list)
    bobgui_notebook_switch_page (notebook, BOBGUI_NOTEBOOK_PAGE_FROM_LIST (list));
}

/**
 * bobgui_notebook_next_page:
 * @notebook: a `BobguiNotebook`
 *
 * Switches to the next page.
 *
 * Nothing happens if the current page is the last page.
 */
void
bobgui_notebook_next_page (BobguiNotebook *notebook)
{
  GList *list;

  g_return_if_fail (BOBGUI_IS_NOTEBOOK (notebook));

  list = g_list_find (notebook->children, notebook->cur_page);
  if (!list)
    return;

  list = bobgui_notebook_search_page (notebook, list, STEP_NEXT, TRUE);
  if (!list)
    return;

  bobgui_notebook_switch_page (notebook, BOBGUI_NOTEBOOK_PAGE_FROM_LIST (list));
}

/**
 * bobgui_notebook_prev_page:
 * @notebook: a `BobguiNotebook`
 *
 * Switches to the previous page.
 *
 * Nothing happens if the current page is the first page.
 */
void
bobgui_notebook_prev_page (BobguiNotebook *notebook)
{
  GList *list;

  g_return_if_fail (BOBGUI_IS_NOTEBOOK (notebook));

  list = g_list_find (notebook->children, notebook->cur_page);
  if (!list)
    return;

  list = bobgui_notebook_search_page (notebook, list, STEP_PREV, TRUE);
  if (!list)
    return;

  bobgui_notebook_switch_page (notebook, BOBGUI_NOTEBOOK_PAGE_FROM_LIST (list));
}

/* Public BobguiNotebook/Tab Style Functions
 *
 * bobgui_notebook_set_show_border
 * bobgui_notebook_get_show_border
 * bobgui_notebook_set_show_tabs
 * bobgui_notebook_get_show_tabs
 * bobgui_notebook_set_tab_pos
 * bobgui_notebook_get_tab_pos
 * bobgui_notebook_set_scrollable
 * bobgui_notebook_get_scrollable
 */
/**
 * bobgui_notebook_set_show_border:
 * @notebook: a `BobguiNotebook`
 * @show_border: %TRUE if a bevel should be drawn around the notebook
 *
 * Sets whether a bevel will be drawn around the notebook pages.
 *
 * This only has a visual effect when the tabs are not shown.
 */
void
bobgui_notebook_set_show_border (BobguiNotebook *notebook,
                              gboolean     show_border)
{
  g_return_if_fail (BOBGUI_IS_NOTEBOOK (notebook));

  if (notebook->show_border != show_border)
    {
      notebook->show_border = show_border;

      if (show_border)
        bobgui_widget_add_css_class (BOBGUI_WIDGET (notebook), "frame");
      else
        bobgui_widget_remove_css_class (BOBGUI_WIDGET (notebook), "frame");

      g_object_notify_by_pspec (G_OBJECT (notebook), properties[PROP_SHOW_BORDER]);
    }
}

/**
 * bobgui_notebook_get_show_border:
 * @notebook: a `BobguiNotebook`
 *
 * Returns whether a bevel will be drawn around the notebook pages.
 *
 * Returns: %TRUE if the bevel is drawn
 */
gboolean
bobgui_notebook_get_show_border (BobguiNotebook *notebook)
{
  g_return_val_if_fail (BOBGUI_IS_NOTEBOOK (notebook), FALSE);

  return notebook->show_border;
}

/**
 * bobgui_notebook_set_show_tabs:
 * @notebook: a `BobguiNotebook`
 * @show_tabs: %TRUE if the tabs should be shown
 *
 * Sets whether to show the tabs for the notebook or not.
 */
void
bobgui_notebook_set_show_tabs (BobguiNotebook *notebook,
                            gboolean     show_tabs)
{
  BobguiNotebookPage *page;
  GList *children;
  int i;

  g_return_if_fail (BOBGUI_IS_NOTEBOOK (notebook));

  show_tabs = show_tabs != FALSE;

  if (notebook->show_tabs == show_tabs)
    return;

  notebook->show_tabs = show_tabs;
  children = notebook->children;

  if (!show_tabs)
    {
      while (children)
        {
          page = children->data;
          children = children->next;
          if (page->default_tab)
            {
              bobgui_widget_unparent (page->tab_label);
              page->tab_label = NULL;
            }
          else
            bobgui_widget_set_visible (page->tab_label, FALSE);
        }
    }
  else
    {
      bobgui_notebook_update_labels (notebook);
    }
  bobgui_widget_set_visible (notebook->header_widget, show_tabs);

  for (i = 0; i < N_ACTION_WIDGETS; i++)
    {
      if (notebook->action_widget[i])
        bobgui_widget_set_child_visible (notebook->action_widget[i], show_tabs);
    }

  bobgui_notebook_update_tab_pos (notebook);
  bobgui_widget_queue_resize (BOBGUI_WIDGET (notebook));

  g_object_notify_by_pspec (G_OBJECT (notebook), properties[PROP_SHOW_TABS]);
}

/**
 * bobgui_notebook_get_show_tabs:
 * @notebook: a `BobguiNotebook`
 *
 * Returns whether the tabs of the notebook are shown.
 *
 * Returns: %TRUE if the tabs are shown
 */
gboolean
bobgui_notebook_get_show_tabs (BobguiNotebook *notebook)
{
  g_return_val_if_fail (BOBGUI_IS_NOTEBOOK (notebook), FALSE);

  return notebook->show_tabs;
}

static void
bobgui_notebook_update_tab_pos (BobguiNotebook *notebook)
{
  BobguiLayoutManager *layout;
  BobguiPositionType tab_pos;
  const char *tab_pos_names[] = {
    "left", "right", "top", "bottom",
  };
  int i;

  tab_pos = get_effective_tab_pos (notebook);

  for (i = 0; i < G_N_ELEMENTS (tab_pos_names); i++)
    {
      if (tab_pos == i)
        bobgui_widget_add_css_class (notebook->header_widget, tab_pos_names[i]);
      else
        bobgui_widget_remove_css_class (notebook->header_widget, tab_pos_names[i]);
    }

  layout = bobgui_widget_get_layout_manager (BOBGUI_WIDGET (notebook));

  switch (tab_pos)
    {
    case BOBGUI_POS_TOP:
      bobgui_widget_set_hexpand (notebook->tabs_widget, TRUE);
      bobgui_widget_set_vexpand (notebook->tabs_widget, FALSE);
      bobgui_widget_set_hexpand (notebook->header_widget, TRUE);
      bobgui_widget_set_vexpand (notebook->header_widget, FALSE);
      if (notebook->show_tabs)
        {
          bobgui_widget_insert_before (notebook->header_widget, BOBGUI_WIDGET (notebook), notebook->stack_widget);
        }

      bobgui_orientable_set_orientation (BOBGUI_ORIENTABLE (layout), BOBGUI_ORIENTATION_VERTICAL);
      bobgui_orientable_set_orientation (BOBGUI_ORIENTABLE (notebook->header_widget), BOBGUI_ORIENTATION_HORIZONTAL);
      break;

    case BOBGUI_POS_BOTTOM:
      bobgui_widget_set_hexpand (notebook->tabs_widget, TRUE);
      bobgui_widget_set_vexpand (notebook->tabs_widget, FALSE);
      bobgui_widget_set_hexpand (notebook->header_widget, TRUE);
      bobgui_widget_set_vexpand (notebook->header_widget, FALSE);
      if (notebook->show_tabs)
        {
          bobgui_widget_insert_after (notebook->header_widget, BOBGUI_WIDGET (notebook), notebook->stack_widget);
        }

      bobgui_orientable_set_orientation (BOBGUI_ORIENTABLE (layout), BOBGUI_ORIENTATION_VERTICAL);
      bobgui_orientable_set_orientation (BOBGUI_ORIENTABLE (notebook->header_widget), BOBGUI_ORIENTATION_HORIZONTAL);
      break;

    case BOBGUI_POS_LEFT:
      bobgui_widget_set_hexpand (notebook->tabs_widget, FALSE);
      bobgui_widget_set_vexpand (notebook->tabs_widget, TRUE);
      bobgui_widget_set_hexpand (notebook->header_widget, FALSE);
      bobgui_widget_set_vexpand (notebook->header_widget, TRUE);
      if (notebook->show_tabs)
        {
          bobgui_widget_insert_before (notebook->header_widget, BOBGUI_WIDGET (notebook), notebook->stack_widget);
        }

      bobgui_orientable_set_orientation (BOBGUI_ORIENTABLE (layout), BOBGUI_ORIENTATION_HORIZONTAL);
      bobgui_orientable_set_orientation (BOBGUI_ORIENTABLE (notebook->header_widget), BOBGUI_ORIENTATION_VERTICAL);
      break;

    case BOBGUI_POS_RIGHT:
      bobgui_widget_set_hexpand (notebook->tabs_widget, FALSE);
      bobgui_widget_set_vexpand (notebook->tabs_widget, TRUE);
      bobgui_widget_set_hexpand (notebook->header_widget, FALSE);
      bobgui_widget_set_vexpand (notebook->header_widget, TRUE);
      if (notebook->show_tabs)
        {
          bobgui_widget_insert_after (notebook->header_widget, BOBGUI_WIDGET (notebook), notebook->stack_widget);
        }

      bobgui_orientable_set_orientation (BOBGUI_ORIENTABLE (layout), BOBGUI_ORIENTATION_HORIZONTAL);
      bobgui_orientable_set_orientation (BOBGUI_ORIENTABLE (notebook->header_widget), BOBGUI_ORIENTATION_VERTICAL);
      break;
    default:
      g_assert_not_reached ();
      break;
    }
}

/**
 * bobgui_notebook_set_tab_pos:
 * @notebook: a `BobguiNotebook`.
 * @pos: the edge to draw the tabs at
 *
 * Sets the edge at which the tabs are drawn.
 */
void
bobgui_notebook_set_tab_pos (BobguiNotebook     *notebook,
                          BobguiPositionType  pos)
{
  g_return_if_fail (BOBGUI_IS_NOTEBOOK (notebook));

  if (notebook->tab_pos != pos)
    {
      notebook->tab_pos = pos;
      bobgui_widget_queue_resize (BOBGUI_WIDGET (notebook));

      bobgui_notebook_update_tab_pos (notebook);

      g_object_notify_by_pspec (G_OBJECT (notebook), properties[PROP_TAB_POS]);
    }
}

/**
 * bobgui_notebook_get_tab_pos:
 * @notebook: a `BobguiNotebook`
 *
 * Gets the edge at which the tabs are drawn.
 *
 * Returns: the edge at which the tabs are drawn
 */
BobguiPositionType
bobgui_notebook_get_tab_pos (BobguiNotebook *notebook)
{
  g_return_val_if_fail (BOBGUI_IS_NOTEBOOK (notebook), BOBGUI_POS_TOP);

  return notebook->tab_pos;
}

/**
 * bobgui_notebook_set_scrollable:
 * @notebook: a `BobguiNotebook`
 * @scrollable: %TRUE if scroll arrows should be added
 *
 * Sets whether the tab label area will have arrows for
 * scrolling if there are too many tabs to fit in the area.
 */
void
bobgui_notebook_set_scrollable (BobguiNotebook *notebook,
                             gboolean     scrollable)
{
  g_return_if_fail (BOBGUI_IS_NOTEBOOK (notebook));

  scrollable = (scrollable != FALSE);

  if (notebook->scrollable == scrollable)
    return;

  notebook->scrollable = scrollable;

  update_arrow_nodes (notebook);
  update_arrow_state (notebook);

  bobgui_widget_queue_resize (BOBGUI_WIDGET (notebook));

  g_object_notify_by_pspec (G_OBJECT (notebook), properties[PROP_SCROLLABLE]);
}

/**
 * bobgui_notebook_get_scrollable:
 * @notebook: a `BobguiNotebook`
 *
 * Returns whether the tab label area has arrows for scrolling.
 *
 * Returns: %TRUE if arrows for scrolling are present
 */
gboolean
bobgui_notebook_get_scrollable (BobguiNotebook *notebook)
{
  g_return_val_if_fail (BOBGUI_IS_NOTEBOOK (notebook), FALSE);

  return notebook->scrollable;
}


/* Public BobguiNotebook Popup Menu Methods:
 *
 * bobgui_notebook_popup_enable
 * bobgui_notebook_popup_disable
 */


/**
 * bobgui_notebook_popup_enable:
 * @notebook: a `BobguiNotebook`
 *
 * Enables the popup menu.
 *
 * If the user clicks with the right mouse button on the tab labels,
 * a menu with all the pages will be popped up.
 */
void
bobgui_notebook_popup_enable (BobguiNotebook *notebook)
{
  GList *list;

  g_return_if_fail (BOBGUI_IS_NOTEBOOK (notebook));

  if (notebook->menu)
    return;

  notebook->menu = bobgui_popover_menu_new ();
  bobgui_widget_set_parent (notebook->menu, notebook->tabs_widget);

  notebook->menu_box = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
  g_object_ref_sink (notebook->menu_box);
  bobgui_popover_menu_add_submenu (BOBGUI_POPOVER_MENU (notebook->menu), notebook->menu_box, "main");

  for (list = bobgui_notebook_search_page (notebook, NULL, STEP_NEXT, FALSE);
       list;
       list = bobgui_notebook_search_page (notebook, list, STEP_NEXT, FALSE))
    bobgui_notebook_menu_item_create (notebook, list->data);

  bobgui_notebook_update_labels (notebook);

  g_object_notify_by_pspec (G_OBJECT (notebook), properties[PROP_ENABLE_POPUP]);
}

/**
 * bobgui_notebook_popup_disable:
 * @notebook: a `BobguiNotebook`
 *
 * Disables the popup menu.
 */
void
bobgui_notebook_popup_disable (BobguiNotebook *notebook)
{
  BobguiWidget *child;

  g_return_if_fail (BOBGUI_IS_NOTEBOOK (notebook));

  if (!notebook->menu)
    return;

  for (child = bobgui_widget_get_first_child (notebook->menu_box);
       child != NULL;
       child = bobgui_widget_get_next_sibling (child))
    bobgui_notebook_menu_label_unparent (child);
  notebook->menu = NULL;
  notebook->menu_box = NULL;

  g_object_notify_by_pspec (G_OBJECT (notebook), properties[PROP_ENABLE_POPUP]);
}

/* Public BobguiNotebook Page Properties Functions:
 *
 * bobgui_notebook_get_tab_label
 * bobgui_notebook_set_tab_label
 * bobgui_notebook_set_tab_label_text
 * bobgui_notebook_get_menu_label
 * bobgui_notebook_set_menu_label
 * bobgui_notebook_set_menu_label_text
 * bobgui_notebook_get_tab_reorderable
 * bobgui_notebook_set_tab_reorderable
 * bobgui_notebook_get_tab_detachable
 * bobgui_notebook_set_tab_detachable
 */

/**
 * bobgui_notebook_get_tab_label:
 * @notebook: a `BobguiNotebook`
 * @child: the page
 *
 * Returns the tab label widget for the page @child.
 *
 * %NULL is returned if @child is not in @notebook or
 * if no tab label has specifically been set for @child.
 *
 * Returns: (transfer none) (nullable): the tab label
 */
BobguiWidget *
bobgui_notebook_get_tab_label (BobguiNotebook *notebook,
                            BobguiWidget   *child)
{
  GList *list;

  g_return_val_if_fail (BOBGUI_IS_NOTEBOOK (notebook), NULL);
  g_return_val_if_fail (BOBGUI_IS_WIDGET (child), NULL);

  list = bobgui_notebook_find_child (notebook, child);
  if (list == NULL)
    return NULL;

  if (BOBGUI_NOTEBOOK_PAGE_FROM_LIST (list)->default_tab)
    return NULL;

  return BOBGUI_NOTEBOOK_PAGE_FROM_LIST (list)->tab_label;
}

/**
 * bobgui_notebook_set_tab_label:
 * @notebook: a `BobguiNotebook`
 * @child: the page
 * @tab_label: (nullable): the tab label widget to use, or %NULL
 *   for default tab label
 *
 * Changes the tab label for @child.
 *
 * If %NULL is specified for @tab_label, then the page will
 * have the label “page N”.
 */
void
bobgui_notebook_set_tab_label (BobguiNotebook *notebook,
                            BobguiWidget   *child,
                            BobguiWidget   *tab_label)
{
  BobguiNotebookPage *page;
  GList *list;

  g_return_if_fail (BOBGUI_IS_NOTEBOOK (notebook));
  g_return_if_fail (BOBGUI_IS_WIDGET (child));

  list = bobgui_notebook_find_child (notebook, child);
  g_return_if_fail (list != NULL);

  /* a NULL pointer indicates a default_tab setting, otherwise
   * we need to set the associated label
   */
  page = list->data;

  if (page->tab_label == tab_label)
    return;

  bobgui_notebook_remove_tab_label (notebook, page);

  if (tab_label)
    {
      page->default_tab = FALSE;
      page->tab_label = tab_label;
      g_object_set_data (G_OBJECT (page->tab_label), "notebook", notebook);
      bobgui_widget_set_parent (page->tab_label, page->tab_widget);
    }
  else
    {
      page->default_tab = TRUE;
      page->tab_label = NULL;

      if (notebook->show_tabs)
        {
          char string[32];

          g_snprintf (string, sizeof(string), _("Page %u"),
                      g_list_position (notebook->children, list));
          page->tab_label = bobgui_label_new (string);
          bobgui_widget_set_parent (page->tab_label, page->tab_widget);
          g_object_set_data (G_OBJECT (page->tab_label), "notebook", notebook);
        }
    }

  if (page->tab_label)
    page->mnemonic_activate_signal =
      g_signal_connect (page->tab_label,
                        "mnemonic-activate",
                        G_CALLBACK (bobgui_notebook_mnemonic_activate_switch_page),
                        notebook);

  if (notebook->show_tabs && bobgui_widget_get_visible (child))
    {
      bobgui_widget_set_visible (page->tab_label, TRUE);
      bobgui_widget_queue_resize (BOBGUI_WIDGET (notebook));
    }

  if (notebook->menu)
    bobgui_notebook_menu_item_recreate (notebook, list);

  g_object_notify (G_OBJECT (page), "tab-label");
}

/**
 * bobgui_notebook_set_tab_label_text:
 * @notebook: a `BobguiNotebook`
 * @child: the page
 * @tab_text: the label text
 *
 * Creates a new label and sets it as the tab label for the page
 * containing @child.
 */
void
bobgui_notebook_set_tab_label_text (BobguiNotebook *notebook,
                                 BobguiWidget   *child,
                                 const char *tab_text)
{
  BobguiWidget *tab_label = NULL;

  g_return_if_fail (BOBGUI_IS_NOTEBOOK (notebook));

  if (tab_text)
    tab_label = bobgui_label_new (tab_text);
  bobgui_notebook_set_tab_label (notebook, child, tab_label);
}

/**
 * bobgui_notebook_get_tab_label_text:
 * @notebook: a `BobguiNotebook`
 * @child: a widget contained in a page of @notebook
 *
 * Retrieves the text of the tab label for the page containing
 * @child.
 *
 * Returns: (nullable): the text of the tab label, or %NULL if
 *   the tab label widget is not a `BobguiLabel`. The string is owned
 *   by the widget and must not be freed.
 */
const char *
bobgui_notebook_get_tab_label_text (BobguiNotebook *notebook,
                                 BobguiWidget   *child)
{
  BobguiWidget *tab_label;

  g_return_val_if_fail (BOBGUI_IS_NOTEBOOK (notebook), NULL);
  g_return_val_if_fail (BOBGUI_IS_WIDGET (child), NULL);

  tab_label = bobgui_notebook_get_tab_label (notebook, child);

  if (BOBGUI_IS_LABEL (tab_label))
    return bobgui_label_get_text (BOBGUI_LABEL (tab_label));
  else
    return NULL;
}

/**
 * bobgui_notebook_get_menu_label:
 * @notebook: a `BobguiNotebook`
 * @child: a widget contained in a page of @notebook
 *
 * Retrieves the menu label widget of the page containing @child.
 *
 * Returns: (nullable) (transfer none): the menu label, or %NULL
 *   if the notebook page does not have a menu label other than
 *   the default (the tab label).
 */
BobguiWidget*
bobgui_notebook_get_menu_label (BobguiNotebook *notebook,
                             BobguiWidget   *child)
{
  GList *list;

  g_return_val_if_fail (BOBGUI_IS_NOTEBOOK (notebook), NULL);
  g_return_val_if_fail (BOBGUI_IS_WIDGET (child), NULL);

  list = bobgui_notebook_find_child (notebook, child);
  g_return_val_if_fail (list != NULL, NULL);

  if (BOBGUI_NOTEBOOK_PAGE_FROM_LIST (list)->default_menu)
    return NULL;

  return BOBGUI_NOTEBOOK_PAGE_FROM_LIST (list)->menu_label;
}

/**
 * bobgui_notebook_set_menu_label:
 * @notebook: a `BobguiNotebook`
 * @child: the child widget
 * @menu_label: (nullable): the menu label, or %NULL for default
 *
 * Changes the menu label for the page containing @child.
 */
void
bobgui_notebook_set_menu_label (BobguiNotebook *notebook,
                             BobguiWidget   *child,
                             BobguiWidget   *menu_label)
{
  BobguiNotebookPage *page;
  GList *list;

  g_return_if_fail (BOBGUI_IS_NOTEBOOK (notebook));
  g_return_if_fail (BOBGUI_IS_WIDGET (child));

  list = bobgui_notebook_find_child (notebook, child);
  g_return_if_fail (list != NULL);

  page = list->data;
  if (page->menu_label)
    {
      if (notebook->menu)
        bobgui_widget_unparent (bobgui_widget_get_parent (page->menu_label));

      g_clear_object (&page->menu_label);
    }

  if (menu_label)
    {
      page->menu_label = menu_label;
      g_object_ref_sink (page->menu_label);
      page->default_menu = FALSE;
    }
  else
    page->default_menu = TRUE;

  if (notebook->menu)
    bobgui_notebook_menu_item_create (notebook, page);
  g_object_notify (G_OBJECT (page), "menu-label");
}

/**
 * bobgui_notebook_set_menu_label_text:
 * @notebook: a `BobguiNotebook`
 * @child: the child widget
 * @menu_text: the label text
 *
 * Creates a new label and sets it as the menu label of @child.
 */
void
bobgui_notebook_set_menu_label_text (BobguiNotebook *notebook,
                                  BobguiWidget   *child,
                                  const char *menu_text)
{
  BobguiWidget *menu_label = NULL;

  g_return_if_fail (BOBGUI_IS_NOTEBOOK (notebook));

  if (menu_text)
    {
      menu_label = bobgui_label_new (menu_text);
      bobgui_widget_set_halign (menu_label, BOBGUI_ALIGN_START);
      bobgui_widget_set_valign (menu_label, BOBGUI_ALIGN_CENTER);
    }
  bobgui_notebook_set_menu_label (notebook, child, menu_label);
}

/**
 * bobgui_notebook_get_menu_label_text:
 * @notebook: a `BobguiNotebook`
 * @child: the child widget of a page of the notebook.
 *
 * Retrieves the text of the menu label for the page containing
 * @child.
 *
 * Returns: (nullable): the text of the tab label, or %NULL if
 *   the widget does not have a menu label other than the default
 *   menu label, or the menu label widget is not a `BobguiLabel`.
 *   The string is owned by the widget and must not be freed.
 */
const char *
bobgui_notebook_get_menu_label_text (BobguiNotebook *notebook,
                                  BobguiWidget *child)
{
  BobguiWidget *menu_label;

  g_return_val_if_fail (BOBGUI_IS_NOTEBOOK (notebook), NULL);
  g_return_val_if_fail (BOBGUI_IS_WIDGET (child), NULL);

  menu_label = bobgui_notebook_get_menu_label (notebook, child);

  if (BOBGUI_IS_LABEL (menu_label))
    return bobgui_label_get_text (BOBGUI_LABEL (menu_label));
  else
    return NULL;
}

/* Helper function called when pages are reordered
 */
static void
bobgui_notebook_child_reordered (BobguiNotebook     *notebook,
                              BobguiNotebookPage *page)
{
  GList *list;
  BobguiWidget *sibling;

  list = g_list_find (notebook->children, page);

  if (notebook->menu)
    bobgui_notebook_menu_item_recreate (notebook, list);

  if (list->prev)
    sibling = BOBGUI_NOTEBOOK_PAGE_FROM_LIST (list->prev)->tab_widget;
  else if (notebook->arrow_widget[ARROW_RIGHT_BEFORE])
    sibling = notebook->arrow_widget[ARROW_RIGHT_BEFORE];
  else if (notebook->arrow_widget[ARROW_LEFT_BEFORE])
    sibling = notebook->arrow_widget[ARROW_LEFT_BEFORE];
  else
    sibling = NULL;

  bobgui_widget_insert_after (page->tab_widget, notebook->tabs_widget, sibling);

  update_arrow_state (notebook);
  bobgui_notebook_update_labels (notebook);
  bobgui_widget_queue_allocate (notebook->tabs_widget);
}

/**
 * bobgui_notebook_reorder_child:
 * @notebook: a `BobguiNotebook`
 * @child: the child to move
 * @position: the new position, or -1 to move to the end
 *
 * Reorders the page containing @child, so that it appears in position
 * @position.
 *
 * If @position is greater than or equal to the number of children in
 * the list or negative, @child will be moved to the end of the list.
 */
void
bobgui_notebook_reorder_child (BobguiNotebook *notebook,
                            BobguiWidget   *child,
                            int          position)
{
  GList *list, *new_list;
  BobguiNotebookPage *page;
  int old_pos;
  int max_pos;
  int i;

  g_return_if_fail (BOBGUI_IS_NOTEBOOK (notebook));
  g_return_if_fail (BOBGUI_IS_WIDGET (child));

  list = bobgui_notebook_find_child (notebook, child);
  g_return_if_fail (list != NULL);

  max_pos = g_list_length (notebook->children) - 1;
  if (position < 0 || position > max_pos)
    position = max_pos;

  old_pos = g_list_position (notebook->children, list);

  if (old_pos == position)
    return;

  page = list->data;
  notebook->children = g_list_delete_link (notebook->children, list);

  notebook->children = g_list_insert (notebook->children, page, position);
  new_list = g_list_nth (notebook->children, position);

  /* Fix up GList references in BobguiNotebook structure */
  if (notebook->first_tab == list)
    notebook->first_tab = new_list;
  if (notebook->focus_tab == list)
    notebook->focus_tab = new_list;

  /* Move around the menu items if necessary */
  bobgui_notebook_child_reordered (notebook, page);

  for (list = notebook->children, i = 0; list; list = list->next, i++)
    {
      if (MIN (old_pos, position) <= i && i <= MAX (old_pos, position))
        g_object_notify (G_OBJECT (list->data), "position");
    }

  g_signal_emit (notebook,
                 notebook_signals[PAGE_REORDERED],
                 0,
                 child,
                 position);
}

/**
 * bobgui_notebook_set_group_name:
 * @notebook: a `BobguiNotebook`
 * @group_name: (nullable): the name of the notebook group,
 *   or %NULL to unset it
 *
 * Sets a group name for @notebook.
 *
 * Notebooks with the same name will be able to exchange tabs
 * via drag and drop. A notebook with a %NULL group name will
 * not be able to exchange tabs with any other notebook.
 */
void
bobgui_notebook_set_group_name (BobguiNotebook *notebook,
                             const char *group_name)
{
  GQuark group;

  g_return_if_fail (BOBGUI_IS_NOTEBOOK (notebook));

  group = g_quark_from_string (group_name);

  if (notebook->group != group)
    {
      notebook->group = group;

      g_object_notify_by_pspec (G_OBJECT (notebook), properties[PROP_GROUP_NAME]);
    }
}

/**
 * bobgui_notebook_get_group_name:
 * @notebook: a `BobguiNotebook`
 *
 * Gets the current group name for @notebook.
 *
 * Returns: (nullable) (transfer none): the group name,
 *   or %NULL if none is set
 */
const char *
bobgui_notebook_get_group_name (BobguiNotebook *notebook)
{
  g_return_val_if_fail (BOBGUI_IS_NOTEBOOK (notebook), NULL);

  return g_quark_to_string (notebook->group);
}

/**
 * bobgui_notebook_get_tab_reorderable:
 * @notebook: a `BobguiNotebook`
 * @child: a child `BobguiWidget`
 *
 * Gets whether the tab can be reordered via drag and drop or not.
 *
 * Returns: %TRUE if the tab is reorderable.
 */
gboolean
bobgui_notebook_get_tab_reorderable (BobguiNotebook *notebook,
                                  BobguiWidget   *child)
{
  GList *list;

  g_return_val_if_fail (BOBGUI_IS_NOTEBOOK (notebook), FALSE);
  g_return_val_if_fail (BOBGUI_IS_WIDGET (child), FALSE);

  list = bobgui_notebook_find_child (notebook, child);
  g_return_val_if_fail (list != NULL, FALSE);

  return BOBGUI_NOTEBOOK_PAGE_FROM_LIST (list)->reorderable;
}

/**
 * bobgui_notebook_set_tab_reorderable:
 * @notebook: a `BobguiNotebook`
 * @child: a child `BobguiWidget`
 * @reorderable: whether the tab is reorderable or not
 *
 * Sets whether the notebook tab can be reordered
 * via drag and drop or not.
 */
void
bobgui_notebook_set_tab_reorderable (BobguiNotebook *notebook,
                                  BobguiWidget   *child,
                                  gboolean     reorderable)
{
  BobguiNotebookPage *page;
  GList *list;

  g_return_if_fail (BOBGUI_IS_NOTEBOOK (notebook));
  g_return_if_fail (BOBGUI_IS_WIDGET (child));

  list = bobgui_notebook_find_child (notebook, child);
  g_return_if_fail (list != NULL);

  page = BOBGUI_NOTEBOOK_PAGE_FROM_LIST (list);
  reorderable = reorderable != FALSE;

  if (page->reorderable != reorderable)
    {
      page->reorderable = reorderable;
      if (reorderable)
        bobgui_widget_add_css_class (page->tab_widget, "reorderable-page");
      else
        bobgui_widget_remove_css_class (page->tab_widget, "reorderable-page");

      g_object_notify (G_OBJECT (page), "reorderable");
    }
}

/**
 * bobgui_notebook_get_tab_detachable:
 * @notebook: a `BobguiNotebook`
 * @child: a child `BobguiWidget`
 *
 * Returns whether the tab contents can be detached from @notebook.
 *
 * Returns: %TRUE if the tab is detachable.
 */
gboolean
bobgui_notebook_get_tab_detachable (BobguiNotebook *notebook,
                                 BobguiWidget   *child)
{
  GList *list;

  g_return_val_if_fail (BOBGUI_IS_NOTEBOOK (notebook), FALSE);
  g_return_val_if_fail (BOBGUI_IS_WIDGET (child), FALSE);

  list = bobgui_notebook_find_child (notebook, child);
  g_return_val_if_fail (list != NULL, FALSE);

  return BOBGUI_NOTEBOOK_PAGE_FROM_LIST (list)->detachable;
}

/**
 * bobgui_notebook_set_tab_detachable:
 * @notebook: a `BobguiNotebook`
 * @child: a child `BobguiWidget`
 * @detachable: whether the tab is detachable or not
 *
 * Sets whether the tab can be detached from @notebook to another
 * notebook or widget.
 *
 * Note that two notebooks must share a common group identifier
 * (see [method@Bobgui.Notebook.set_group_name]) to allow automatic tabs
 * interchange between them.
 *
 * If you want a widget to interact with a notebook through DnD
 * (i.e.: accept dragged tabs from it) it must be set as a drop
 * destination by adding to it a [class@Bobgui.DropTarget] controller that accepts
 * the GType `BOBGUI_TYPE_NOTEBOOK_PAGE`. The `:value` of said drop target will be
 * preloaded with a [class@Bobgui.NotebookPage] object that corresponds to the
 * dropped tab, so you can process the value via `::accept` or `::drop` signals.
 *
 * Note that you should use [method@Bobgui.Notebook.detach_tab] instead
 * of [method@Bobgui.Notebook.remove_page] if you want to remove the tab
 * from the source notebook as part of accepting a drop. Otherwise,
 * the source notebook will think that the dragged tab was removed
 * from underneath the ongoing drag operation, and will initiate a
 * drag cancel animation.
 *
 * ```c
 * static void
 * on_drag_data_received (BobguiWidget        *widget,
 *                        GdkDrop          *drop,
 *                        BobguiSelectionData *data,
 *                        guint             time,
 *                        gpointer          user_data)
 * {
 *   BobguiDrag *drag;
 *   BobguiWidget *notebook;
 *   BobguiWidget **child;
 *
 *   drag = bobgui_drop_get_drag (drop);
 *   notebook = g_object_get_data (drag, "bobgui-notebook-drag-origin");
 *   child = (void*) bobgui_selection_data_get_data (data);
 *
 *   // process_widget (*child);
 *
 *   bobgui_notebook_detach_tab (BOBGUI_NOTEBOOK (notebook), *child);
 * }
 * ```
 *
 * If you want a notebook to accept drags from other widgets,
 * you will have to set your own DnD code to do it.
 */
void
bobgui_notebook_set_tab_detachable (BobguiNotebook *notebook,
                                 BobguiWidget  *child,
                                 gboolean    detachable)
{
  GList *list;
  BobguiNotebookPage *page;

  g_return_if_fail (BOBGUI_IS_NOTEBOOK (notebook));
  g_return_if_fail (BOBGUI_IS_WIDGET (child));

  list = bobgui_notebook_find_child (notebook, child);
  g_return_if_fail (list != NULL);

  page = BOBGUI_NOTEBOOK_PAGE_FROM_LIST (list);
  detachable = detachable != FALSE;

  if (page->detachable != detachable)
    {
      page->detachable = detachable;
      g_object_notify (G_OBJECT (page), "detachable");
    }
}

/**
 * bobgui_notebook_get_action_widget:
 * @notebook: a `BobguiNotebook`
 * @pack_type: pack type of the action widget to receive
 *
 * Gets one of the action widgets.
 *
 * See [method@Bobgui.Notebook.set_action_widget].
 *
 * Returns: (nullable) (transfer none): The action widget
 *   with the given @pack_type or %NULL when this action
 *   widget has not been set
 */
BobguiWidget*
bobgui_notebook_get_action_widget (BobguiNotebook *notebook,
                                BobguiPackType  pack_type)
{
  g_return_val_if_fail (BOBGUI_IS_NOTEBOOK (notebook), NULL);

  return notebook->action_widget[pack_type];
}

/**
 * bobgui_notebook_set_action_widget:
 * @notebook: a `BobguiNotebook`
 * @widget: a `BobguiWidget`
 * @pack_type: pack type of the action widget
 *
 * Sets @widget as one of the action widgets.
 *
 * Depending on the pack type the widget will be placed before
 * or after the tabs. You can use a `BobguiBox` if you need to pack
 * more than one widget on the same side.
 */
void
bobgui_notebook_set_action_widget (BobguiNotebook *notebook,
                                BobguiWidget   *widget,
                                BobguiPackType  pack_type)
{
  g_return_if_fail (BOBGUI_IS_NOTEBOOK (notebook));
  g_return_if_fail (!widget || BOBGUI_IS_WIDGET (widget));
  g_return_if_fail (!widget || bobgui_widget_get_parent (widget) == NULL);

  if (notebook->action_widget[pack_type])
    bobgui_box_remove (BOBGUI_BOX (notebook->header_widget), notebook->action_widget[pack_type]);

  notebook->action_widget[pack_type] = widget;

  if (widget)
    {
      bobgui_box_append (BOBGUI_BOX (notebook->header_widget), widget);
      if (pack_type == BOBGUI_PACK_START)
        bobgui_box_reorder_child_after (BOBGUI_BOX (notebook->header_widget), widget, NULL);
      else
        bobgui_box_reorder_child_after (BOBGUI_BOX (notebook->header_widget), widget, bobgui_widget_get_last_child (notebook->header_widget));
      bobgui_widget_set_child_visible (widget, notebook->show_tabs);
    }

  bobgui_widget_queue_resize (BOBGUI_WIDGET (notebook));
}

/**
 * bobgui_notebook_get_page:
 * @notebook: a `BobguiNotebook`
 * @child: a child of @notebook
 *
 * Returns the `BobguiNotebookPage` for @child.
 *
 * Returns: (transfer none): the `BobguiNotebookPage` for @child
 */
BobguiNotebookPage *
bobgui_notebook_get_page (BobguiNotebook *notebook,
                       BobguiWidget   *child)
{
  GList *list;
  BobguiNotebookPage *page = NULL;

  g_return_val_if_fail (BOBGUI_IS_NOTEBOOK (notebook), NULL);
  g_return_val_if_fail (BOBGUI_IS_WIDGET (child), NULL);

  list = bobgui_notebook_find_child (notebook, child);
  if (list != NULL)
    page = list->data;

  return page;
}

/**
 * bobgui_notebook_page_get_child:
 * @page: a `BobguiNotebookPage`
 *
 * Returns the notebook child to which @page belongs.
 *
 * Returns: (transfer none): the child to which @page belongs
 */
BobguiWidget *
bobgui_notebook_page_get_child (BobguiNotebookPage *page)
{
  return page->child;
}

#define BOBGUI_TYPE_NOTEBOOK_PAGES (bobgui_notebook_pages_get_type ())
G_DECLARE_FINAL_TYPE (BobguiNotebookPages, bobgui_notebook_pages, BOBGUI, NOTEBOOK_PAGES, GObject)

struct _BobguiNotebookPages
{
  GObject parent_instance;
  BobguiNotebook *notebook;
};

struct _BobguiNotebookPagesClass
{
  GObjectClass parent_class;
};

static GType
bobgui_notebook_pages_get_item_type (GListModel *model)
{
  return BOBGUI_TYPE_NOTEBOOK_PAGE;
}

static guint
bobgui_notebook_pages_get_n_items (GListModel *model)
{
  BobguiNotebookPages *pages = BOBGUI_NOTEBOOK_PAGES (model);

  return g_list_length (pages->notebook->children);
}


static gpointer
bobgui_notebook_pages_get_item (GListModel *model,
                             guint       position)
{
  BobguiNotebookPages *pages = BOBGUI_NOTEBOOK_PAGES (model);
  BobguiNotebookPage *page;

  page = g_list_nth_data (pages->notebook->children, position);

  return g_object_ref (page);
}

static void
bobgui_notebook_pages_list_model_init (GListModelInterface *iface)
{
  iface->get_item_type = bobgui_notebook_pages_get_item_type;
  iface->get_n_items = bobgui_notebook_pages_get_n_items;
  iface->get_item = bobgui_notebook_pages_get_item;
}

static gboolean
bobgui_notebook_pages_is_selected (BobguiSelectionModel *model,
                                guint              position)
{
  BobguiNotebookPages *pages = BOBGUI_NOTEBOOK_PAGES (model);
  BobguiNotebookPage *page;

  page = g_list_nth_data (pages->notebook->children, position);
  if (page == NULL)
    return FALSE;

  return page == pages->notebook->cur_page;
}

static gboolean
bobgui_notebook_pages_select_item (BobguiSelectionModel *model,
                                guint              position,
                                gboolean           exclusive)
{
  BobguiNotebookPages *pages = BOBGUI_NOTEBOOK_PAGES (model);
  BobguiNotebookPage *page;

  page = g_list_nth_data (pages->notebook->children, position);
  if (page == NULL)
    return FALSE;

  if (page == pages->notebook->cur_page)
    return FALSE;

  bobgui_notebook_switch_page (pages->notebook, page);

  return TRUE;
}

static void
bobgui_notebook_pages_selection_model_init (BobguiSelectionModelInterface *iface)
{
  iface->is_selected = bobgui_notebook_pages_is_selected;
  iface->select_item = bobgui_notebook_pages_select_item;
}

G_DEFINE_TYPE_WITH_CODE (BobguiNotebookPages, bobgui_notebook_pages, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (G_TYPE_LIST_MODEL, bobgui_notebook_pages_list_model_init)
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_SELECTION_MODEL, bobgui_notebook_pages_selection_model_init))

static void
bobgui_notebook_pages_init (BobguiNotebookPages *pages)
{
}

static void
bobgui_notebook_pages_class_init (BobguiNotebookPagesClass *class)
{
}

static BobguiNotebookPages *
bobgui_notebook_pages_new (BobguiNotebook *notebook)
{
  BobguiNotebookPages *pages;

  pages = g_object_new (BOBGUI_TYPE_NOTEBOOK_PAGES, NULL);
  pages->notebook = notebook;

  return pages;
}

/**
 * bobgui_notebook_get_pages:
 * @notebook: a `BobguiNotebook`
 *
 * Returns a `GListModel` that contains the pages of the notebook.
 *
 * This can be used to keep an up-to-date view. The model also
 * implements [iface@Bobgui.SelectionModel] and can be used to track
 * and modify the visible page.

 * Returns: (transfer full) (attributes element-type=BobguiNotebookPage): a
 *   `GListModel` for the notebook's children
 */
GListModel *
bobgui_notebook_get_pages (BobguiNotebook *notebook)
{
  g_return_val_if_fail (BOBGUI_IS_NOTEBOOK (notebook), NULL);

  if (notebook->pages)
    return g_object_ref (notebook->pages);

  notebook->pages = G_LIST_MODEL (bobgui_notebook_pages_new (notebook));

  g_object_add_weak_pointer (G_OBJECT (notebook->pages), (gpointer *)&notebook->pages);

  return notebook->pages;
}

