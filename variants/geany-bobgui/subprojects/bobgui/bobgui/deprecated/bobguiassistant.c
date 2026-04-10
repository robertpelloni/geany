/*
 * BOBGUI - The Bobgui Framework
 * Copyright (C) 1999  Red Hat, Inc.
 * Copyright (C) 2002  Anders Carlsson <andersca@gnu.org>
 * Copyright (C) 2003  Matthias Clasen <mclasen@redhat.com>
 * Copyright (C) 2005  Carlos Garnacho Parro <carlosg@gnome.org>
 *
 * All rights reserved.
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

/**
 * BobguiAssistant:
 *
 * `BobguiAssistant` is used to represent a complex as a series of steps.
 *
 * <picture>
 *   <source srcset="assistant-dark.png" media="(prefers-color-scheme: dark)">
 *   <img alt="An example BobguiAssistant" src="assistant.png">
 * </picture>
 *
 * Each step consists of one or more pages. `BobguiAssistant` guides the user
 * through the pages, and controls the page flow to collect the data needed
 * for the operation.
 *
 * `BobguiAssistant` handles which buttons to show and to make sensitive based
 * on page sequence knowledge and the [enum@Bobgui.AssistantPageType] of each
 * page in addition to state information like the *completed* and *committed*
 * page statuses.
 *
 * If you have a case that doesn’t quite fit in `BobguiAssistant`s way of
 * handling buttons, you can use the %BOBGUI_ASSISTANT_PAGE_CUSTOM page
 * type and handle buttons yourself.
 *
 * `BobguiAssistant` maintains a `BobguiAssistantPage` object for each added
 * child, which holds additional per-child properties. You
 * obtain the `BobguiAssistantPage` for a child with [method@Bobgui.Assistant.get_page].
 *
 * # BobguiAssistant as BobguiBuildable
 *
 * The `BobguiAssistant` implementation of the `BobguiBuildable` interface
 * exposes the @action_area as internal children with the name
 * “action_area”.
 *
 * To add pages to an assistant in `BobguiBuilder`, simply add it as a
 * child to the `BobguiAssistant` object. If you need to set per-object
 * properties, create a `BobguiAssistantPage` object explicitly, and
 * set the child widget as a property on it.
 *
 * # CSS nodes
 *
 * `BobguiAssistant` has a single CSS node with the name window and style
 * class .assistant.
 *
 * Deprecated: 4.10: This widget will be removed in BOBGUI 5
 */

/**
 * BobguiAssistantPage:
 *
 * `BobguiAssistantPage` is an auxiliary object used by `BobguiAssistant`.
 *
 * Deprecated: 4.10: This object will be removed in BOBGUI 5
 */

#include "config.h"

#include "bobguiassistant.h"

#include "bobguibox.h"
#include "bobguibuildable.h"
#include "bobguibutton.h"
#include "bobguiframe.h"
#include "bobguiheaderbar.h"
#include "bobguiimage.h"
#include "bobguilabel.h"
#include "bobguilistlistmodelprivate.h"
#include "bobguimaplistmodel.h"
#include "bobguiprivate.h"
#include "bobguisettings.h"
#include "bobguisizegroup.h"
#include "bobguisizerequest.h"
#include "bobguistack.h"
#include "bobguitypebuiltins.h"

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

typedef struct _BobguiAssistantPageClass   BobguiAssistantPageClass;

struct _BobguiAssistantPage
{
  GObject instance;
  BobguiAssistantPageType type;
  guint      complete     : 1;
  guint      complete_set : 1;

  char *title;

  BobguiWidget *page;
  BobguiWidget *regular_title;
  BobguiWidget *current_title;
};

struct _BobguiAssistantPageClass
{
  GObjectClass parent_class;
};

typedef struct _BobguiAssistantClass   BobguiAssistantClass;

struct _BobguiAssistant
{
  BobguiWindow  parent;

  BobguiWidget *cancel;
  BobguiWidget *forward;
  BobguiWidget *back;
  BobguiWidget *apply;
  BobguiWidget *close;
  BobguiWidget *last;

  BobguiWidget *sidebar;
  BobguiWidget *content;
  BobguiWidget *action_area;
  BobguiWidget *headerbar;
  int use_header_bar;
  gboolean constructed;

  GList     *pages;
  GSList    *visited_pages;
  BobguiAssistantPage *current_page;

  BobguiSizeGroup *button_size_group;
  BobguiSizeGroup *title_size_group;

  BobguiAssistantPageFunc forward_function;
  gpointer forward_function_data;
  GDestroyNotify forward_data_destroy;

  GListModel *model;

  int extra_buttons;

  guint committed : 1;
};

struct _BobguiAssistantClass
{
  BobguiWindowClass parent_class;

  void (* prepare) (BobguiAssistant *assistant, BobguiWidget *page);
  void (* apply)   (BobguiAssistant *assistant);
  void (* close)   (BobguiAssistant *assistant);
  void (* cancel)  (BobguiAssistant *assistant);
};

#define BOBGUI_TYPE_ASSISTANT_PAGES (bobgui_assistant_pages_get_type ())
G_DECLARE_FINAL_TYPE (BobguiAssistantPages, bobgui_assistant_pages, BOBGUI, ASSISTANT_PAGES, GObject)

struct _BobguiAssistantPages
{
  GObject parent_instance;
  BobguiAssistant *assistant;
};

struct _BobguiAssistantPagesClass
{
  GObjectClass parent_class;
};

enum {
  PAGES_PROP_0,
  PAGES_PROP_ITEM_TYPE,
  PAGES_PROP_N_ITEMS,

  PAGES_N_PROPS
};

static GParamSpec *pages_properties[PAGES_N_PROPS] = { NULL, };

static void     bobgui_assistant_dispose            (GObject           *object);
static void     bobgui_assistant_map                (BobguiWidget         *widget);
static void     bobgui_assistant_unmap              (BobguiWidget         *widget);
static gboolean bobgui_assistant_close_request      (BobguiWindow         *window);

static void     bobgui_assistant_page_set_property  (GObject           *object,
                                                  guint              property_id,
                                                  const GValue      *value,
                                                  GParamSpec        *pspec);
static void     bobgui_assistant_page_get_property  (GObject           *object,
                                                  guint              property_id,
                                                  GValue            *value,
                                                  GParamSpec        *pspec);

static void     bobgui_assistant_buildable_interface_init   (BobguiBuildableIface  *iface);
static void     bobgui_assistant_buildable_add_child        (BobguiBuildable       *buildable,
                                                          BobguiBuilder         *builder,
                                                          GObject            *child,
                                                          const char         *type);
static gboolean bobgui_assistant_buildable_custom_tag_start (BobguiBuildable       *buildable,
                                                          BobguiBuilder         *builder,
                                                          GObject            *child,
                                                          const char         *tagname,
                                                          BobguiBuildableParser *parser,
                                                          gpointer           *data);
static void     bobgui_assistant_buildable_custom_finished  (BobguiBuildable       *buildable,
                                                          BobguiBuilder         *builder,
                                                          GObject            *child,
                                                          const char         *tagname,
                                                          gpointer            user_data);

static GList*   find_page                                (BobguiAssistant       *assistant,
                                                          BobguiWidget          *page);
static void     on_assistant_close                       (BobguiWidget          *widget,
                                                          BobguiAssistant       *assistant);
static void     on_assistant_apply                       (BobguiWidget          *widget,
                                                          BobguiAssistant       *assistant);
static void     on_assistant_forward                     (BobguiWidget          *widget,
                                                          BobguiAssistant       *assistant);
static void     on_assistant_back                        (BobguiWidget          *widget,
                                                          BobguiAssistant       *assistant);
static void     on_assistant_cancel                      (BobguiWidget          *widget,
                                                          BobguiAssistant       *assistant);
static void     on_assistant_last                        (BobguiWidget          *widget,
                                                          BobguiAssistant       *assistant);

static int        bobgui_assistant_add_page                 (BobguiAssistant     *assistant,
                                                          BobguiAssistantPage *page_info,
                                                          int               position);

enum
{
  CHILD_PROP_0,
  CHILD_PROP_CHILD,
  CHILD_PROP_PAGE_TYPE,
  CHILD_PROP_PAGE_TITLE,
  CHILD_PROP_PAGE_COMPLETE,
  CHILD_PROP_HAS_PADDING
};

G_DEFINE_TYPE (BobguiAssistantPage, bobgui_assistant_page, G_TYPE_OBJECT)

static void
bobgui_assistant_page_init (BobguiAssistantPage *page)
{
  page->type = BOBGUI_ASSISTANT_PAGE_CONTENT;
}

static void
bobgui_assistant_page_finalize (GObject *object)
{
  BobguiAssistantPage *page = BOBGUI_ASSISTANT_PAGE (object);

  g_clear_object (&page->page);
  g_free (page->title);

  G_OBJECT_CLASS (bobgui_assistant_page_parent_class)->finalize (object);
}

static void
bobgui_assistant_page_class_init (BobguiAssistantPageClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->finalize = bobgui_assistant_page_finalize;
  object_class->get_property = bobgui_assistant_page_get_property;
  object_class->set_property = bobgui_assistant_page_set_property;

  /**
   * BobguiAssistantPage:page-type:
   *
   * The type of the assistant page.
   *
   * Deprecated: 4.10: This object will be removed in BOBGUI 5
   */
  g_object_class_install_property (object_class,
                                   CHILD_PROP_PAGE_TYPE,
                                   g_param_spec_enum ("page-type", NULL, NULL,
                                                      BOBGUI_TYPE_ASSISTANT_PAGE_TYPE,
                                                      BOBGUI_ASSISTANT_PAGE_CONTENT,
                                                      BOBGUI_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY));

  /**
   * BobguiAssistantPage:title:
   *
   * The title of the page.
   *
   * Deprecated: 4.10: This object will be removed in BOBGUI 5
   */
  g_object_class_install_property (object_class,
                                   CHILD_PROP_PAGE_TITLE,
                                   g_param_spec_string ("title", NULL, NULL,
                                                        NULL,
                                                        BOBGUI_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY));

  /**
   * BobguiAssistantPage:complete:
   *
   * Whether all required fields are filled in.
   *
   * BOBGUI uses this information to control the sensitivity
   * of the navigation buttons.
   *
   * Deprecated: 4.10: This object will be removed in BOBGUI 5
   */
  g_object_class_install_property (object_class,
                                   CHILD_PROP_PAGE_COMPLETE,
                                   g_param_spec_boolean ("complete", NULL, NULL,
                                                         FALSE,
                                                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY));

  /**
   * BobguiAssistantPage:child:
   *
   * The child widget.
   *
   * Deprecated: 4.10: This object will be removed in BOBGUI 5
   */
  g_object_class_install_property (object_class,
                                   CHILD_PROP_CHILD,
                                   g_param_spec_object ("child", NULL, NULL,
                                                        BOBGUI_TYPE_WIDGET,
                                                        BOBGUI_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

enum
{
  CANCEL,
  PREPARE,
  APPLY,
  CLOSE,
  ESCAPE,
  LAST_SIGNAL
};

enum {
  PROP_0,
  PROP_USE_HEADER_BAR,
  PROP_PAGES
};

static guint signals [LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE_WITH_CODE (BobguiAssistant, bobgui_assistant, BOBGUI_TYPE_WINDOW,
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_BUILDABLE,
                                                bobgui_assistant_buildable_interface_init))

static void
set_use_header_bar (BobguiAssistant *assistant,
                    int           use_header_bar)
{
  if (use_header_bar == -1)
    return;

  assistant->use_header_bar = use_header_bar;
}

static void
bobgui_assistant_set_property (GObject      *object,
                            guint         prop_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  BobguiAssistant *assistant = BOBGUI_ASSISTANT (object);

  switch (prop_id)
    {
    case PROP_USE_HEADER_BAR:
      set_use_header_bar (assistant, g_value_get_int (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_assistant_get_property (GObject      *object,
                            guint         prop_id,
                            GValue       *value,
                            GParamSpec   *pspec)
{
  BobguiAssistant *assistant = BOBGUI_ASSISTANT (object);

  switch (prop_id)
    {
    case PROP_USE_HEADER_BAR:
      g_value_set_int (value, assistant->use_header_bar);
      break;

    case PROP_PAGES:
      g_value_set_object (value, bobgui_assistant_get_pages (assistant));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
apply_use_header_bar (BobguiAssistant *assistant)
{
  bobgui_widget_set_visible (assistant->action_area, !assistant->use_header_bar);
  bobgui_widget_set_visible (assistant->headerbar, assistant->use_header_bar);
  if (!assistant->use_header_bar)
    bobgui_window_set_titlebar (BOBGUI_WINDOW (assistant), NULL);
}

static void
add_to_header_bar (BobguiAssistant *assistant,
                   BobguiWidget    *child)
{
  bobgui_widget_set_valign (child, BOBGUI_ALIGN_CENTER);

  if (child == assistant->back || child == assistant->cancel)
    bobgui_header_bar_pack_start (BOBGUI_HEADER_BAR (assistant->headerbar), child);
  else
    bobgui_header_bar_pack_end (BOBGUI_HEADER_BAR (assistant->headerbar), child);
}

static void
add_action_widgets (BobguiAssistant *assistant)
{
  GList *children, *l;
  BobguiWidget *child;

  if (assistant->use_header_bar)
    {
      children = NULL;
      for (child = bobgui_widget_get_last_child (assistant->action_area);
           child != NULL;
           child = bobgui_widget_get_prev_sibling (child))
        children = g_list_prepend (children, child);
      for (l = children; l != NULL; l = l->next)
        {
          gboolean has_default;

          child = l->data;
          has_default = bobgui_widget_has_default (child);

          g_object_ref (child);
          bobgui_box_remove (BOBGUI_BOX (assistant->action_area), child);
          add_to_header_bar (assistant, child);
          g_object_unref (child);

          if (has_default)
            {
              bobgui_window_set_default_widget (BOBGUI_WINDOW (assistant), child);
              bobgui_widget_add_css_class (child, "suggested-action");
            }
        }
      g_list_free (children);
    }
}

static void
bobgui_assistant_constructed (GObject *object)
{
  BobguiAssistant *assistant = BOBGUI_ASSISTANT (object);

  G_OBJECT_CLASS (bobgui_assistant_parent_class)->constructed (object);

  assistant->constructed = TRUE;
  if (assistant->use_header_bar == -1)
    assistant->use_header_bar = FALSE;

  add_action_widgets (assistant);
  apply_use_header_bar (assistant);
}

static void
escape_cb (BobguiAssistant *assistant)
{
  /* Do not allow cancelling in the middle of a progress page */
  if (assistant->current_page &&
      (assistant->current_page->type != BOBGUI_ASSISTANT_PAGE_PROGRESS ||
       assistant->current_page->complete))
    g_signal_emit (assistant, signals [CANCEL], 0, NULL);

  /* don't run any user handlers - this is not a public signal */
  g_signal_stop_emission (assistant, signals[ESCAPE], 0);
}

static void
bobgui_assistant_finalize (GObject *object)
{
  BobguiAssistant *assistant = BOBGUI_ASSISTANT (object);

  if (assistant->model)
    g_object_remove_weak_pointer (G_OBJECT (assistant->model), (gpointer *)&assistant->model);

  G_OBJECT_CLASS (bobgui_assistant_parent_class)->finalize (object);
}

static void
bobgui_assistant_class_init (BobguiAssistantClass *class)
{
  GObjectClass *gobject_class;
  BobguiWidgetClass *widget_class;
  BobguiWindowClass *window_class;

  gobject_class   = (GObjectClass *) class;
  widget_class    = (BobguiWidgetClass *) class;
  window_class    = (BobguiWindowClass *) class;

  gobject_class->dispose = bobgui_assistant_dispose;
  gobject_class->finalize = bobgui_assistant_finalize;
  gobject_class->constructed  = bobgui_assistant_constructed;
  gobject_class->set_property = bobgui_assistant_set_property;
  gobject_class->get_property = bobgui_assistant_get_property;

  widget_class->map = bobgui_assistant_map;
  widget_class->unmap = bobgui_assistant_unmap;

  window_class->close_request = bobgui_assistant_close_request;

  /**
   * BobguiAssistant::cancel:
   * @assistant: the `BobguiAssistant`
   *
   * Emitted when then the cancel button is clicked.
   *
   * Deprecated: 4.10: This widget will be removed in BOBGUI 5
   */
  signals[CANCEL] =
    g_signal_new (I_("cancel"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BobguiAssistantClass, cancel),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);

  /**
   * BobguiAssistant::prepare:
   * @assistant: the `BobguiAssistant`
   * @page: the current page
   *
   * Emitted when a new page is set as the assistant's current page,
   * before making the new page visible.
   *
   * A handler for this signal can do any preparations which are
   * necessary before showing @page.
   *
   * Deprecated: 4.10: This widget will be removed in BOBGUI 5
   */
  signals[PREPARE] =
    g_signal_new (I_("prepare"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BobguiAssistantClass, prepare),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 1, BOBGUI_TYPE_WIDGET);

  /**
   * BobguiAssistant::apply:
   * @assistant: the `BobguiAssistant`
   *
   * Emitted when the apply button is clicked.
   *
   * The default behavior of the `BobguiAssistant` is to switch to the page
   * after the current page, unless the current page is the last one.
   *
   * A handler for the ::apply signal should carry out the actions for
   * which the wizard has collected data. If the action takes a long time
   * to complete, you might consider putting a page of type
   * %BOBGUI_ASSISTANT_PAGE_PROGRESS after the confirmation page and handle
   * this operation within the [signal@Bobgui.Assistant::prepare] signal of
   * the progress page.
   *
   * Deprecated: 4.10: This widget will be removed in BOBGUI 5
   */
  signals[APPLY] =
    g_signal_new (I_("apply"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BobguiAssistantClass, apply),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);

  /**
   * BobguiAssistant::close:
   * @assistant: the `BobguiAssistant`
   *
   * Emitted either when the close button of a summary page is clicked,
   * or when the apply button in the last page in the flow (of type
   * %BOBGUI_ASSISTANT_PAGE_CONFIRM) is clicked.
   *
   * Deprecated: 4.10: This widget will be removed in BOBGUI 5
   */
  signals[CLOSE] =
    g_signal_new (I_("close"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BobguiAssistantClass, close),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);

  /**
   * BobguiAssistant::escape
   * @assistant: the `BobguiAssistant`
   *
   * The action signal for the Escape binding.
   *
   * Deprecated: 4.10: This widget will be removed in BOBGUI 5
   */
  signals[ESCAPE] =
    g_signal_new_class_handler (I_("escape"),
                                G_TYPE_FROM_CLASS (gobject_class),
                                G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                                G_CALLBACK (escape_cb),
                                NULL, NULL,
                                NULL,
                                G_TYPE_NONE, 0);

  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_Escape, 0,
                                       "escape",
                                       NULL);

  /**
   * BobguiAssistant:use-header-bar:
   *
   * %TRUE if the assistant uses a `BobguiHeaderBar` for action buttons
   * instead of the action-area.
   *
   * For technical reasons, this property is declared as an integer
   * property, but you should only set it to %TRUE or %FALSE.
   *
   * Deprecated: 4.10: This widget will be removed in BOBGUI 5
   */
  g_object_class_install_property (gobject_class,
                                   PROP_USE_HEADER_BAR,
                                   g_param_spec_int ("use-header-bar", NULL, NULL,
                                                     -1, 1, -1,
                                                     BOBGUI_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY));

  /**
   * BobguiAssistant:pages:
   *
   * `GListModel` containing the pages.
   */
  g_object_class_install_property (gobject_class,
                                   PROP_PAGES,
                                   g_param_spec_object ("pages", NULL, NULL,
                                                        G_TYPE_LIST_MODEL,
                                                        BOBGUI_PARAM_READABLE));

  /* Bind class to template
   */
  bobgui_widget_class_set_template_from_resource (widget_class,
                                               "/org/bobgui/libbobgui/ui/bobguiassistant.ui");

  bobgui_widget_class_bind_template_child_internal (widget_class, BobguiAssistant, action_area);
  bobgui_widget_class_bind_template_child_internal (widget_class, BobguiAssistant, headerbar);
  bobgui_widget_class_bind_template_child (widget_class, BobguiAssistant, content);
  bobgui_widget_class_bind_template_child (widget_class, BobguiAssistant, cancel);
  bobgui_widget_class_bind_template_child (widget_class, BobguiAssistant, forward);
  bobgui_widget_class_bind_template_child (widget_class, BobguiAssistant, back);
  bobgui_widget_class_bind_template_child (widget_class, BobguiAssistant, apply);
  bobgui_widget_class_bind_template_child (widget_class, BobguiAssistant, close);
  bobgui_widget_class_bind_template_child (widget_class, BobguiAssistant, last);
  bobgui_widget_class_bind_template_child (widget_class, BobguiAssistant, sidebar);
  bobgui_widget_class_bind_template_child (widget_class, BobguiAssistant, button_size_group);
  bobgui_widget_class_bind_template_child (widget_class, BobguiAssistant, title_size_group);

  bobgui_widget_class_bind_template_callback (widget_class, on_assistant_close);
  bobgui_widget_class_bind_template_callback (widget_class, on_assistant_apply);
  bobgui_widget_class_bind_template_callback (widget_class, on_assistant_forward);
  bobgui_widget_class_bind_template_callback (widget_class, on_assistant_back);
  bobgui_widget_class_bind_template_callback (widget_class, on_assistant_cancel);
  bobgui_widget_class_bind_template_callback (widget_class, on_assistant_last);
}

static int
default_forward_function (int current_page, gpointer data)
{
  BobguiAssistant *assistant = BOBGUI_ASSISTANT (data);
  BobguiAssistantPage *page_info;
  GList *page_node;

  page_node = g_list_nth (assistant->pages, ++current_page);

  if (!page_node)
    return -1;

  page_info = (BobguiAssistantPage *) page_node->data;

  while (page_node && !bobgui_widget_get_visible (page_info->page))
    {
      page_node = page_node->next;
      current_page++;

      if (page_node)
        page_info = (BobguiAssistantPage *) page_node->data;
    }

  return current_page;
}

static gboolean
last_button_visible (BobguiAssistant *assistant, BobguiAssistantPage *page)
{
  BobguiAssistantPage *page_info;
  int count, page_num, n_pages;

  if (page == NULL)
    return FALSE;

  if (page->type != BOBGUI_ASSISTANT_PAGE_CONTENT)
    return FALSE;

  count = 0;
  page_num = g_list_index (assistant->pages, page);
  n_pages  = g_list_length (assistant->pages);
  page_info = page;

  while (page_num >= 0 && page_num < n_pages &&
         page_info->type == BOBGUI_ASSISTANT_PAGE_CONTENT &&
         (count == 0 || page_info->complete) &&
         count < n_pages)
    {
      page_num = (assistant->forward_function) (page_num, assistant->forward_function_data);
      page_info = g_list_nth_data (assistant->pages, page_num);

      count++;
    }

  /* Make the last button visible if we can skip multiple
   * pages and end on a confirmation or summary page
   */
  if (count > 1 && page_info &&
      (page_info->type == BOBGUI_ASSISTANT_PAGE_CONFIRM ||
       page_info->type == BOBGUI_ASSISTANT_PAGE_SUMMARY))
    return TRUE;
  else
    return FALSE;
}

static void
update_actions_size (BobguiAssistant *assistant)
{
  GList *l;
  BobguiAssistantPage *page;
  int buttons, page_buttons;

  if (!assistant->current_page)
    return;

  /* Some heuristics to find out how many buttons we should
   * reserve space for. It is possible to trick this code
   * with page forward functions and invisible pages, etc.
   */
  buttons = 0;
  for (l = assistant->pages; l; l = l->next)
    {
      page = l->data;

      if (!bobgui_widget_get_visible (page->page))
        continue;

      page_buttons = 2; /* cancel, forward/apply/close */
      if (l != assistant->pages)
        page_buttons += 1; /* back */
      if (last_button_visible (assistant, page))
        page_buttons += 1; /* last */

      buttons = MAX (buttons, page_buttons);
    }

  buttons += assistant->extra_buttons;

  bobgui_widget_set_size_request (assistant->action_area,
                               buttons * bobgui_widget_get_allocated_width (assistant->cancel) + (buttons - 1) * 6,
                               -1);
}

static void
compute_last_button_state (BobguiAssistant *assistant)
{
  bobgui_widget_set_sensitive (assistant->last, assistant->current_page->complete);
  bobgui_widget_set_visible (assistant->last,
                          last_button_visible (assistant, assistant->current_page));
}

static void
compute_progress_state (BobguiAssistant *assistant)
{
  int page_num, n_pages;

  n_pages = bobgui_assistant_get_n_pages (assistant);
  page_num = bobgui_assistant_get_current_page (assistant);

  page_num = (assistant->forward_function) (page_num, assistant->forward_function_data);
  bobgui_widget_set_visible (assistant->forward, page_num >= 0 && page_num < n_pages);
}

static void
update_buttons_state (BobguiAssistant *assistant)
{
  if (!assistant->current_page)
    return;

  switch (assistant->current_page->type)
    {
    case BOBGUI_ASSISTANT_PAGE_INTRO:
      bobgui_widget_set_sensitive (assistant->cancel, TRUE);
      bobgui_widget_set_sensitive (assistant->forward, assistant->current_page->complete);
      bobgui_window_set_default_widget (BOBGUI_WINDOW (assistant), assistant->forward);
      bobgui_widget_set_visible (assistant->forward, TRUE);
      bobgui_widget_set_visible (assistant->back, FALSE);
      bobgui_widget_set_visible (assistant->apply, FALSE);
      bobgui_widget_set_visible (assistant->close, FALSE);
      compute_last_button_state (assistant);
      break;
    case BOBGUI_ASSISTANT_PAGE_CONFIRM:
      bobgui_widget_set_sensitive (assistant->cancel, TRUE);
      bobgui_widget_set_sensitive (assistant->back, TRUE);
      bobgui_widget_set_sensitive (assistant->apply, assistant->current_page->complete);
      bobgui_window_set_default_widget (BOBGUI_WINDOW (assistant), assistant->apply);
      bobgui_widget_set_visible (assistant->back, TRUE);
      bobgui_widget_set_visible (assistant->apply, TRUE);
      bobgui_widget_set_visible (assistant->forward, FALSE);
      bobgui_widget_set_visible (assistant->close, FALSE);
      bobgui_widget_set_visible (assistant->last, FALSE);
      break;
    case BOBGUI_ASSISTANT_PAGE_CONTENT:
      bobgui_widget_set_sensitive (assistant->cancel, TRUE);
      bobgui_widget_set_sensitive (assistant->back, TRUE);
      bobgui_widget_set_sensitive (assistant->forward, assistant->current_page->complete);
      bobgui_window_set_default_widget (BOBGUI_WINDOW (assistant), assistant->forward);
      bobgui_widget_set_visible (assistant->back, TRUE);
      bobgui_widget_set_visible (assistant->forward, TRUE);
      bobgui_widget_set_visible (assistant->apply, FALSE);
      bobgui_widget_set_visible (assistant->close, FALSE);
      compute_last_button_state (assistant);
      break;
    case BOBGUI_ASSISTANT_PAGE_SUMMARY:
      bobgui_widget_set_sensitive (assistant->close, assistant->current_page->complete);
      bobgui_window_set_default_widget (BOBGUI_WINDOW (assistant), assistant->close);
      bobgui_widget_set_visible (assistant->close, TRUE);
      bobgui_widget_set_visible (assistant->back, FALSE);
      bobgui_widget_set_visible (assistant->forward, FALSE);
      bobgui_widget_set_visible (assistant->apply, FALSE);
      bobgui_widget_set_visible (assistant->last, FALSE);
      break;
    case BOBGUI_ASSISTANT_PAGE_PROGRESS:
      bobgui_widget_set_sensitive (assistant->cancel, assistant->current_page->complete);
      bobgui_widget_set_sensitive (assistant->back, assistant->current_page->complete);
      bobgui_widget_set_sensitive (assistant->forward, assistant->current_page->complete);
      bobgui_window_set_default_widget (BOBGUI_WINDOW (assistant), assistant->forward);
      bobgui_widget_set_visible (assistant->back, TRUE);
      bobgui_widget_set_visible (assistant->apply, FALSE);
      bobgui_widget_set_visible (assistant->close, FALSE);
      bobgui_widget_set_visible (assistant->last, FALSE);
      compute_progress_state (assistant);
      break;
    case BOBGUI_ASSISTANT_PAGE_CUSTOM:
      bobgui_widget_set_visible (assistant->cancel, FALSE);
      bobgui_widget_set_visible (assistant->back, FALSE);
      bobgui_widget_set_visible (assistant->forward, FALSE);
      bobgui_widget_set_visible (assistant->apply, FALSE);
      bobgui_widget_set_visible (assistant->last, FALSE);
      bobgui_widget_set_visible (assistant->close, FALSE);
      break;
    default:
      g_assert_not_reached ();
    }

  if (assistant->committed)
    bobgui_widget_set_visible (assistant->cancel, FALSE);
  else if (assistant->current_page->type == BOBGUI_ASSISTANT_PAGE_SUMMARY ||
           assistant->current_page->type == BOBGUI_ASSISTANT_PAGE_CUSTOM)
    bobgui_widget_set_visible (assistant->cancel, FALSE);
  else
    bobgui_widget_set_visible (assistant->cancel, TRUE);

  /* this is quite general, we don't want to
   * go back if it's the first page
   */
  if (!assistant->visited_pages)
    bobgui_widget_set_visible (assistant->back, FALSE);
}

static gboolean
update_page_title_state (BobguiAssistant *assistant, GList *list)
{
  BobguiAssistantPage *page, *other;
  gboolean visible;
  GList *l;

  page = list->data;

  if (page->title == NULL || page->title[0] == 0)
    visible = FALSE;
  else
    visible = bobgui_widget_get_visible (page->page);

  if (page == assistant->current_page)
    {
      bobgui_widget_set_visible (page->regular_title, FALSE);
      bobgui_widget_set_visible (page->current_title, visible);
    }
  else
    {
      /* If multiple consecutive pages have the same title,
       * we only show it once, since it would otherwise look
       * silly. We have to be a little careful, since we
       * _always_ show the title of the current page.
       */
      if (list->prev)
        {
          other = list->prev->data;
          if (g_strcmp0 (page->title, other->title) == 0)
            visible = FALSE;
        }
      for (l = list->next; l; l = l->next)
        {
          other = l->data;
          if (g_strcmp0 (page->title, other->title) != 0)
            break;

          if (other == assistant->current_page)
            {
              visible = FALSE;
              break;
            }
        }

      bobgui_widget_set_visible (page->regular_title, visible);
      bobgui_widget_set_visible (page->current_title, FALSE);
    }

  return visible;
}

static void
update_title_state (BobguiAssistant *assistant)
{
  GList *l;
  gboolean show_titles;

  show_titles = FALSE;
  for (l = assistant->pages; l != NULL; l = l->next)
    {
      if (update_page_title_state (assistant, l))
        show_titles = TRUE;
    }

  bobgui_widget_set_visible (assistant->sidebar, show_titles);
}

static void
set_current_page (BobguiAssistant *assistant,
                  int           page_num)
{
  assistant->current_page = (BobguiAssistantPage *)g_list_nth_data (assistant->pages, page_num);

  g_signal_emit (assistant, signals [PREPARE], 0, assistant->current_page->page);
  /* do not continue if the prepare signal handler has already changed the
   * current page */
  if (assistant->current_page != (BobguiAssistantPage *)g_list_nth_data (assistant->pages, page_num))
    return;

  update_title_state (assistant);

  bobgui_window_set_title (BOBGUI_WINDOW (assistant), assistant->current_page->title);

  bobgui_stack_set_visible_child (BOBGUI_STACK (assistant->content), assistant->current_page->page);

  /* update buttons state, flow may have changed */
  if (bobgui_widget_get_mapped (BOBGUI_WIDGET (assistant)))
    update_buttons_state (assistant);

  if (!bobgui_widget_child_focus (assistant->current_page->page, BOBGUI_DIR_TAB_FORWARD))
    {
      BobguiWidget *button[6];
      int i;

      /* find the best button to focus */
      button[0] = assistant->apply;
      button[1] = assistant->close;
      button[2] = assistant->forward;
      button[3] = assistant->back;
      button[4] = assistant->cancel;
      button[5] = assistant->last;
      for (i = 0; i < 6; i++)
        {
          if (bobgui_widget_get_visible (button[i]) &&
              bobgui_widget_get_sensitive (button[i]))
            {
              bobgui_widget_grab_focus (button[i]);
              break;
            }
        }
    }
}

static int
compute_next_step (BobguiAssistant *assistant)
{
  BobguiAssistantPage *page_info;
  int current_page, n_pages, next_page;

  current_page = bobgui_assistant_get_current_page (assistant);
  page_info = assistant->current_page;
  n_pages = bobgui_assistant_get_n_pages (assistant);

  next_page = (assistant->forward_function) (current_page,
                                        assistant->forward_function_data);

  if (next_page >= 0 && next_page < n_pages)
    {
      assistant->visited_pages = g_slist_prepend (assistant->visited_pages, page_info);
      set_current_page (assistant, next_page);

      return TRUE;
    }

  return FALSE;
}

static void
on_assistant_close (BobguiWidget    *widget,
                    BobguiAssistant *assistant)
{
  g_signal_emit (assistant, signals [CLOSE], 0, NULL);
}

static void
on_assistant_apply (BobguiWidget    *widget,
                    BobguiAssistant *assistant)
{
  gboolean success;

  g_signal_emit (assistant, signals [APPLY], 0);

  success = compute_next_step (assistant);

  /* if the assistant hasn't switched to another page, just emit
   * the CLOSE signal, it't the last page in the assistant flow
   */
  if (!success)
    g_signal_emit (assistant, signals [CLOSE], 0);
}

static void
on_assistant_forward (BobguiWidget    *widget,
                      BobguiAssistant *assistant)
{
  bobgui_assistant_next_page (assistant);
}

static void
on_assistant_back (BobguiWidget    *widget,
                   BobguiAssistant *assistant)
{
  bobgui_assistant_previous_page (assistant);
}

static void
on_assistant_cancel (BobguiWidget    *widget,
                     BobguiAssistant *assistant)
{
  g_signal_emit (assistant, signals [CANCEL], 0, NULL);
}

static void
on_assistant_last (BobguiWidget    *widget,
                   BobguiAssistant *assistant)
{
  while (assistant->current_page->type == BOBGUI_ASSISTANT_PAGE_CONTENT &&
         assistant->current_page->complete)
    compute_next_step (assistant);
}

static gboolean
alternative_button_order (BobguiAssistant *assistant)
{
  gboolean result;

  g_object_get (bobgui_widget_get_settings (BOBGUI_WIDGET (assistant)),
                "bobgui-alternative-button-order", &result,
                NULL);
  return result;
}

static void
on_page_page_notify (BobguiWidget  *widget,
                     GParamSpec *arg,
                     gpointer    data)
{
  BobguiAssistant *assistant = BOBGUI_ASSISTANT (data);

  if (bobgui_widget_get_mapped (BOBGUI_WIDGET (assistant)))
    {
      update_buttons_state (assistant);
      update_title_state (assistant);
    }
}

static void
on_page_notify (BobguiAssistantPage *page,
                GParamSpec *arg,
                gpointer    data)
{
  if (page->page)
    on_page_page_notify (page->page, arg, data);
}

static void
assistant_remove_page (BobguiAssistant *assistant,
                       BobguiWidget    *page)
{
  BobguiAssistantPage *page_info;
  GList *page_node;
  GList *element;

  element = find_page (assistant, page);
  if (!element)
    return;

  page_info = element->data;

  /* If this is the current page, we need to switch away. */
  if (page_info == assistant->current_page)
    {
      if (!compute_next_step (assistant))
        {
          /* The best we can do at this point is probably to pick
           * the first visible page.
           */
          page_node = assistant->pages;

          while (page_node &&
                 !bobgui_widget_get_visible (((BobguiAssistantPage *) page_node->data)->page))
            page_node = page_node->next;

          if (page_node == element)
            page_node = page_node->next;

          if (page_node)
            assistant->current_page = page_node->data;
          else
            assistant->current_page = NULL;
        }
    }

  g_signal_handlers_disconnect_by_func (page_info->page, on_page_page_notify, assistant);
  g_signal_handlers_disconnect_by_func (page_info, on_page_notify, assistant);

  bobgui_size_group_remove_widget (assistant->title_size_group, page_info->regular_title);
  bobgui_size_group_remove_widget (assistant->title_size_group, page_info->current_title);

  bobgui_box_remove (BOBGUI_BOX (assistant->sidebar), page_info->regular_title);
  bobgui_box_remove (BOBGUI_BOX (assistant->sidebar), page_info->current_title);

  assistant->pages = g_list_remove_link (assistant->pages, element);
  assistant->visited_pages = g_slist_remove_all (assistant->visited_pages, page_info);

  g_object_unref (page_info);

  g_list_free_1 (element);

  if (bobgui_widget_get_mapped (BOBGUI_WIDGET (assistant)))
    {
      update_buttons_state (assistant);
      update_actions_size (assistant);
    }
}

static void
bobgui_assistant_init (BobguiAssistant *assistant)
{
  bobgui_widget_add_css_class (BOBGUI_WIDGET (assistant), "assistant");

  assistant->pages = NULL;
  assistant->current_page = NULL;
  assistant->visited_pages = NULL;

  assistant->forward_function = default_forward_function;
  assistant->forward_function_data = assistant;
  assistant->forward_data_destroy = NULL;

  g_object_get (bobgui_widget_get_settings (BOBGUI_WIDGET (assistant)),
                "bobgui-dialogs-use-header", &assistant->use_header_bar,
                NULL);

  bobgui_widget_init_template (BOBGUI_WIDGET (assistant));

  if (alternative_button_order (assistant))
    {
      GList *buttons, *l;
      BobguiWidget *child;

      buttons = NULL;
      for (child = bobgui_widget_get_last_child (assistant->action_area);
           child != NULL;
           child = bobgui_widget_get_prev_sibling (child))
        buttons = g_list_prepend (buttons, child);

      for (l = buttons; l; l = l->next)
        bobgui_box_reorder_child_after (BOBGUI_BOX (assistant->action_area), BOBGUI_WIDGET (l->data), NULL);

      g_list_free (buttons);
    }
}

static void
bobgui_assistant_page_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  BobguiAssistantPage *page = BOBGUI_ASSISTANT_PAGE (object);
  BobguiWidget *assistant = NULL;

  if (page->page)
    assistant = bobgui_widget_get_ancestor (page->page, BOBGUI_TYPE_ASSISTANT);

  switch (property_id)
    {
    case CHILD_PROP_CHILD:
      g_set_object (&page->page, g_value_get_object (value));
      break;

    case CHILD_PROP_PAGE_TYPE:
      if (page->type != g_value_get_enum (value))
        {
          page->type = g_value_get_enum (value);

          /* backwards compatibility to the era before fixing bug 604289 */
          if (page->type == BOBGUI_ASSISTANT_PAGE_SUMMARY && !page->complete_set)
            {
              page->complete = TRUE;
              page->complete_set = FALSE;
            }

          /* Always set buttons state, a change in a future page
           * might change current page buttons
           */
          if (assistant)
            update_buttons_state (BOBGUI_ASSISTANT (assistant));
          g_object_notify (G_OBJECT (page), "page-type");
        }
      break;

    case CHILD_PROP_PAGE_TITLE:
      g_free (page->title);
      page->title = g_value_dup_string (value);

      if (assistant)
        {
          bobgui_label_set_text ((BobguiLabel*) page->regular_title, page->title);
          bobgui_label_set_text ((BobguiLabel*) page->current_title, page->title);
          update_title_state (BOBGUI_ASSISTANT (assistant));
        }

      g_object_notify (G_OBJECT (page), "title");

      break;

    case CHILD_PROP_PAGE_COMPLETE:
      if (page->complete != g_value_get_boolean (value))
        {
          page->complete = g_value_get_boolean (value);
          page->complete_set = TRUE;

          /* Always set buttons state, a change in a future page
           * might change current page buttons
           */
          if (assistant)
            update_buttons_state (BOBGUI_ASSISTANT (assistant));
          g_object_notify (G_OBJECT (page), "complete");
        }
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
bobgui_assistant_page_get_property (GObject      *object,
                                 guint         property_id,
                                 GValue       *value,
                                 GParamSpec   *pspec)
{
  BobguiAssistantPage *page = BOBGUI_ASSISTANT_PAGE (object);

  switch (property_id)
    {
    case CHILD_PROP_CHILD:
      g_value_set_object (value, page->page);
      break;

    case CHILD_PROP_PAGE_TYPE:
      g_value_set_enum (value, page->type);
      break;

    case CHILD_PROP_PAGE_TITLE:
      g_value_set_string (value, page->title);
      break;

    case CHILD_PROP_PAGE_COMPLETE:
      g_value_set_boolean (value, page->complete);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
bobgui_assistant_dispose (GObject *object)
{
  BobguiAssistant *assistant = BOBGUI_ASSISTANT (object);

  if (assistant->model && g_list_length (assistant->pages))
    {
      g_list_model_items_changed (G_LIST_MODEL (assistant->model), 0, g_list_length (assistant->pages), 0);
      g_object_notify_by_pspec (G_OBJECT (assistant->model), pages_properties[PAGES_PROP_N_ITEMS]);
    }  

  /* We set current to NULL so that the remove code doesn't try
   * to do anything funny
   */
  assistant->current_page = NULL;

  if (assistant->content)
    {
      while (assistant->pages)
        bobgui_assistant_remove_page (assistant, 0);

      assistant->content = NULL;
    }

  assistant->sidebar = NULL;
  assistant->action_area = NULL;

  if (assistant->forward_function)
    {
      if (assistant->forward_function_data &&
          assistant->forward_data_destroy)
        assistant->forward_data_destroy (assistant->forward_function_data);

      assistant->forward_function = NULL;
      assistant->forward_function_data = NULL;
      assistant->forward_data_destroy = NULL;
    }

  if (assistant->visited_pages)
    {
      g_slist_free (assistant->visited_pages);
      assistant->visited_pages = NULL;
    }

  G_OBJECT_CLASS (bobgui_assistant_parent_class)->dispose (object);
}

static GList*
find_page (BobguiAssistant  *assistant,
           BobguiWidget     *page)
{
  GList *child = assistant->pages;

  while (child)
    {
      BobguiAssistantPage *page_info = child->data;
      if (page_info->page == page)
        return child;

      child = child->next;
    }

  return NULL;
}

static void
bobgui_assistant_map (BobguiWidget *widget)
{
  BobguiAssistant *assistant = BOBGUI_ASSISTANT (widget);
  GList *page_node;
  BobguiAssistantPage *page;
  int page_num;

  /* if there's no default page, pick the first one */
  page = NULL;
  page_num = 0;
  if (!assistant->current_page)
    {
      page_node = assistant->pages;

      while (page_node && !bobgui_widget_get_visible (((BobguiAssistantPage *) page_node->data)->page))
        {
          page_node = page_node->next;
          page_num++;
        }

      if (page_node)
        page = page_node->data;
    }

  if (page && bobgui_widget_get_visible (page->page))
    set_current_page (assistant, page_num);

  update_buttons_state (assistant);
  update_actions_size (assistant);
  update_title_state (assistant);

  BOBGUI_WIDGET_CLASS (bobgui_assistant_parent_class)->map (widget);
}

static void
bobgui_assistant_unmap (BobguiWidget *widget)
{
  BobguiAssistant *assistant = BOBGUI_ASSISTANT (widget);

  g_slist_free (assistant->visited_pages);
  assistant->visited_pages = NULL;
  assistant->current_page  = NULL;

  BOBGUI_WIDGET_CLASS (bobgui_assistant_parent_class)->unmap (widget);
}

static gboolean
bobgui_assistant_close_request (BobguiWindow *window)
{
  BobguiAssistant *assistant = BOBGUI_ASSISTANT (window);

  /* Do not allow cancelling in the middle of a progress page */
  if (assistant->current_page &&
      (assistant->current_page->type != BOBGUI_ASSISTANT_PAGE_PROGRESS ||
       assistant->current_page->complete))
    g_signal_emit (assistant, signals [CANCEL], 0, NULL);

  return TRUE;
}

/**
 * bobgui_assistant_new:
 *
 * Creates a new `BobguiAssistant`.
 *
 * Returns: a newly created `BobguiAssistant`
 *
 * Deprecated: 4.10: This widget will be removed in BOBGUI 5
 */
BobguiWidget*
bobgui_assistant_new (void)
{
  BobguiWidget *assistant;

  assistant = g_object_new (BOBGUI_TYPE_ASSISTANT, NULL);

  return assistant;
}

/**
 * bobgui_assistant_get_current_page:
 * @assistant: a `BobguiAssistant`
 *
 * Returns the page number of the current page.
 *
 * Returns: The index (starting from 0) of the current
 *   page in the @assistant, or -1 if the @assistant has no pages,
 *   or no current page
 *
 * Deprecated: 4.10: This widget will be removed in BOBGUI 5
 */
int
bobgui_assistant_get_current_page (BobguiAssistant *assistant)
{
  g_return_val_if_fail (BOBGUI_IS_ASSISTANT (assistant), -1);

  if (!assistant->pages || !assistant->current_page)
    return -1;

  return g_list_index (assistant->pages, assistant->current_page);
}

/**
 * bobgui_assistant_set_current_page:
 * @assistant: a `BobguiAssistant`
 * @page_num: index of the page to switch to, starting from 0.
 *   If negative, the last page will be used. If greater
 *   than the number of pages in the @assistant, nothing
 *   will be done.
 *
 * Switches the page to @page_num.
 *
 * Note that this will only be necessary in custom buttons,
 * as the @assistant flow can be set with
 * bobgui_assistant_set_forward_page_func().
 *
 * Deprecated: 4.10: This widget will be removed in BOBGUI 5
 */
void
bobgui_assistant_set_current_page (BobguiAssistant *assistant,
                                int           page_num)
{
  BobguiAssistantPage *page;

  g_return_if_fail (BOBGUI_IS_ASSISTANT (assistant));
  g_return_if_fail (assistant->pages != NULL);

  if (page_num >= 0)
    page = (BobguiAssistantPage *) g_list_nth_data (assistant->pages, page_num);
  else
    {
      page = (BobguiAssistantPage *) g_list_last (assistant->pages)->data;
      page_num = g_list_length (assistant->pages);
    }

  g_return_if_fail (page != NULL);

  if (assistant->current_page == page)
    return;

  /* only add the page to the visited list if the assistant is mapped,
   * if not, just use it as an initial page setting, for the cases where
   * the initial page is != to 0
   */
  if (bobgui_widget_get_mapped (BOBGUI_WIDGET (assistant)))
    assistant->visited_pages = g_slist_prepend (assistant->visited_pages,
                                           assistant->current_page);

  set_current_page (assistant, page_num);
}

/**
 * bobgui_assistant_next_page:
 * @assistant: a `BobguiAssistant`
 *
 * Navigate to the next page.
 *
 * It is a programming error to call this function when
 * there is no next page.
 *
 * This function is for use when creating pages of the
 * %BOBGUI_ASSISTANT_PAGE_CUSTOM type.
 *
 * Deprecated: 4.10: This widget will be removed in BOBGUI 5
 */
void
bobgui_assistant_next_page (BobguiAssistant *assistant)
{
  g_return_if_fail (BOBGUI_IS_ASSISTANT (assistant));

  if (!compute_next_step (assistant))
    g_critical ("Page flow is broken.\n"
                "You may want to end it with a page of type\n"
                "BOBGUI_ASSISTANT_PAGE_CONFIRM or BOBGUI_ASSISTANT_PAGE_SUMMARY");
}

/**
 * bobgui_assistant_previous_page:
 * @assistant: a `BobguiAssistant`
 *
 * Navigate to the previous visited page.
 *
 * It is a programming error to call this function when
 * no previous page is available.
 *
 * This function is for use when creating pages of the
 * %BOBGUI_ASSISTANT_PAGE_CUSTOM type.
 *
 * Deprecated: 4.10: This widget will be removed in BOBGUI 5
 */
void
bobgui_assistant_previous_page (BobguiAssistant *assistant)
{
  BobguiAssistantPage *page_info;
  GSList *page_node;

  g_return_if_fail (BOBGUI_IS_ASSISTANT (assistant));

  /* skip the progress pages when going back */
  do
    {
      page_node = assistant->visited_pages;

      g_return_if_fail (page_node != NULL);

      assistant->visited_pages = assistant->visited_pages->next;
      page_info = (BobguiAssistantPage *) page_node->data;
      g_slist_free_1 (page_node);
    }
  while (page_info->type == BOBGUI_ASSISTANT_PAGE_PROGRESS ||
         !bobgui_widget_get_visible (page_info->page));

  set_current_page (assistant, g_list_index (assistant->pages, page_info));
}

/**
 * bobgui_assistant_get_n_pages:
 * @assistant: a `BobguiAssistant`
 *
 * Returns the number of pages in the @assistant
 *
 * Returns: the number of pages in the @assistant
 *
 * Deprecated: 4.10: This widget will be removed in BOBGUI 5
 */
int
bobgui_assistant_get_n_pages (BobguiAssistant *assistant)
{
  g_return_val_if_fail (BOBGUI_IS_ASSISTANT (assistant), 0);

  return g_list_length (assistant->pages);
}

/**
 * bobgui_assistant_get_nth_page:
 * @assistant: a `BobguiAssistant`
 * @page_num: the index of a page in the @assistant,
 *   or -1 to get the last page
 *
 * Returns the child widget contained in page number @page_num.
 *
 * Returns: (nullable) (transfer none): the child widget, or %NULL
 *   if @page_num is out of bounds
 *
 * Deprecated: 4.10: This widget will be removed in BOBGUI 5
 */
BobguiWidget*
bobgui_assistant_get_nth_page (BobguiAssistant *assistant,
                            int           page_num)
{
  BobguiAssistantPage *page;
  GList *elem;

  g_return_val_if_fail (BOBGUI_IS_ASSISTANT (assistant), NULL);
  g_return_val_if_fail (page_num >= -1, NULL);

  if (page_num == -1)
    elem = g_list_last (assistant->pages);
  else
    elem = g_list_nth (assistant->pages, page_num);

  if (!elem)
    return NULL;

  page = (BobguiAssistantPage *) elem->data;

  return page->page;
}

/**
 * bobgui_assistant_prepend_page:
 * @assistant: a `BobguiAssistant`
 * @page: a `BobguiWidget`
 *
 * Prepends a page to the @assistant.
 *
 * Returns: the index (starting at 0) of the inserted page
 *
 * Deprecated: 4.10: This widget will be removed in BOBGUI 5
 */
int
bobgui_assistant_prepend_page (BobguiAssistant *assistant,
                            BobguiWidget    *page)
{
  g_return_val_if_fail (BOBGUI_IS_ASSISTANT (assistant), 0);
  g_return_val_if_fail (BOBGUI_IS_WIDGET (page), 0);

  return bobgui_assistant_insert_page (assistant, page, 0);
}

/**
 * bobgui_assistant_append_page:
 * @assistant: a `BobguiAssistant`
 * @page: a `BobguiWidget`
 *
 * Appends a page to the @assistant.
 *
 * Returns: the index (starting at 0) of the inserted page
 *
 * Deprecated: 4.10: This widget will be removed in BOBGUI 5
 */
int
bobgui_assistant_append_page (BobguiAssistant *assistant,
                           BobguiWidget    *page)
{
  g_return_val_if_fail (BOBGUI_IS_ASSISTANT (assistant), 0);
  g_return_val_if_fail (BOBGUI_IS_WIDGET (page), 0);

  return bobgui_assistant_insert_page (assistant, page, -1);
}

/**
 * bobgui_assistant_insert_page:
 * @assistant: a `BobguiAssistant`
 * @page: a `BobguiWidget`
 * @position: the index (starting at 0) at which to insert the page,
 *   or -1 to append the page to the @assistant
 *
 * Inserts a page in the @assistant at a given position.
 *
 * Returns: the index (starting from 0) of the inserted page
 *
 * Deprecated: 4.10: This widget will be removed in BOBGUI 5
 */
int
bobgui_assistant_insert_page (BobguiAssistant *assistant,
                           BobguiWidget    *page,
                           int           position)
{
  BobguiAssistantPage *page_info;

  g_return_val_if_fail (BOBGUI_IS_ASSISTANT (assistant), 0);
  g_return_val_if_fail (BOBGUI_IS_WIDGET (page), 0);
  g_return_val_if_fail (bobgui_widget_get_parent (page) == NULL, 0);

  page_info = g_object_new (BOBGUI_TYPE_ASSISTANT_PAGE, NULL);
  page_info->page = g_object_ref (page);

  return bobgui_assistant_add_page (assistant, page_info, position);

  g_object_unref (page_info);
}

static int
bobgui_assistant_add_page (BobguiAssistant *assistant,
                        BobguiAssistantPage *page_info,
                        int position)
{
  int n_pages;
  BobguiWidget *sibling;
  char *name;

  page_info->regular_title = bobgui_label_new (page_info->title);
  page_info->current_title = bobgui_label_new (page_info->title);

  bobgui_label_set_xalign (BOBGUI_LABEL (page_info->regular_title), 0.0);
  bobgui_label_set_xalign (BOBGUI_LABEL (page_info->current_title), 0.0);

  bobgui_widget_set_visible (page_info->regular_title, TRUE);
  bobgui_widget_set_visible (page_info->current_title, FALSE);

  bobgui_widget_add_css_class (page_info->current_title, "highlight");

  bobgui_size_group_add_widget (assistant->title_size_group, page_info->regular_title);
  bobgui_size_group_add_widget (assistant->title_size_group, page_info->current_title);

  g_signal_connect (G_OBJECT (page_info->page), "notify::visible",
                    G_CALLBACK (on_page_page_notify), assistant);

  g_signal_connect (G_OBJECT (page_info), "notify::page-title",
                    G_CALLBACK (on_page_notify), assistant);

  g_signal_connect (G_OBJECT (page_info), "notify::page-type",
                    G_CALLBACK (on_page_notify), assistant);

  n_pages = g_list_length (assistant->pages);

  if (position < 0 || position > n_pages)
    position = n_pages;

  assistant->pages = g_list_insert (assistant->pages, g_object_ref (page_info), position);

  if (position == 0)
    sibling = NULL;
  else
    {
      int i;
      sibling = bobgui_widget_get_first_child (assistant->sidebar);
      for (i = 1; i < 2 * position; i++)
        sibling = bobgui_widget_get_next_sibling (sibling);
    }

  bobgui_box_insert_child_after (BOBGUI_BOX (assistant->sidebar), page_info->current_title, sibling);
  bobgui_box_insert_child_after (BOBGUI_BOX (assistant->sidebar), page_info->regular_title, sibling);

  name = g_strdup_printf ("%p", page_info->page);
  bobgui_stack_add_named (BOBGUI_STACK (assistant->content), page_info->page, name);
  g_free (name);

  if (bobgui_widget_get_mapped (BOBGUI_WIDGET (assistant)))
    {
      update_buttons_state (assistant);
      update_actions_size (assistant);
    }

  if (assistant->model)
    {
      g_list_model_items_changed (assistant->model, position, 0, 1);
      g_object_notify_by_pspec (G_OBJECT (assistant->model), pages_properties[PAGES_PROP_N_ITEMS]);
    }

  return position;
}

/**
 * bobgui_assistant_remove_page:
 * @assistant: a `BobguiAssistant`
 * @page_num: the index of a page in the @assistant,
 *   or -1 to remove the last page
 *
 * Removes the @page_num’s page from @assistant.
 *
 * Deprecated: 4.10: This widget will be removed in BOBGUI 5
 */
void
bobgui_assistant_remove_page (BobguiAssistant *assistant,
                           int           page_num)
{
  BobguiWidget *page;

  g_return_if_fail (BOBGUI_IS_ASSISTANT (assistant));

  page = bobgui_assistant_get_nth_page (assistant, page_num);
  if (page)
    assistant_remove_page (assistant, page);

  if (assistant->model)
    {
      g_list_model_items_changed (assistant->model, page_num, 1, 0);
      g_object_notify_by_pspec (G_OBJECT (assistant->model), pages_properties[PAGES_PROP_N_ITEMS]);
    }
}

/**
 * bobgui_assistant_set_forward_page_func:
 * @assistant: a `BobguiAssistant`
 * @page_func: (nullable): the `BobguiAssistantPageFunc`, or %NULL
 *   to use the default one
 * @data: user data for @page_func
 * @destroy: destroy notifier for @data
 *
 * Sets the page forwarding function to be @page_func.
 *
 * This function will be used to determine what will be
 * the next page when the user presses the forward button.
 * Setting @page_func to %NULL will make the assistant to
 * use the default forward function, which just goes to the
 * next visible page.
 *
 * Deprecated: 4.10: This widget will be removed in BOBGUI 5
 */
void
bobgui_assistant_set_forward_page_func (BobguiAssistant         *assistant,
                                     BobguiAssistantPageFunc  page_func,
                                     gpointer              data,
                                     GDestroyNotify        destroy)
{
  g_return_if_fail (BOBGUI_IS_ASSISTANT (assistant));

  if (assistant->forward_data_destroy &&
      assistant->forward_function_data)
    (*assistant->forward_data_destroy) (assistant->forward_function_data);

  if (page_func)
    {
      assistant->forward_function = page_func;
      assistant->forward_function_data = data;
      assistant->forward_data_destroy = destroy;
    }
  else
    {
      assistant->forward_function = default_forward_function;
      assistant->forward_function_data = assistant;
      assistant->forward_data_destroy = NULL;
    }

  /* Page flow has possibly changed, so the
   * buttons state might need to change too
   */
  if (bobgui_widget_get_mapped (BOBGUI_WIDGET (assistant)))
    update_buttons_state (assistant);
}

static void
add_to_action_area (BobguiAssistant *assistant,
                    BobguiWidget    *child)
{
  bobgui_widget_set_valign (child, BOBGUI_ALIGN_BASELINE);

  bobgui_box_append (BOBGUI_BOX (assistant->action_area), child);
}

/**
 * bobgui_assistant_add_action_widget:
 * @assistant: a `BobguiAssistant`
 * @child: a `BobguiWidget`
 *
 * Adds a widget to the action area of a `BobguiAssistant`.
 *
 * Deprecated: 4.10: This widget will be removed in BOBGUI 5
 */
void
bobgui_assistant_add_action_widget (BobguiAssistant *assistant,
                                 BobguiWidget    *child)
{
  g_return_if_fail (BOBGUI_IS_ASSISTANT (assistant));
  g_return_if_fail (BOBGUI_IS_WIDGET (child));

  if (BOBGUI_IS_BUTTON (child))
    {
      bobgui_size_group_add_widget (assistant->button_size_group, child);
      assistant->extra_buttons += 1;
      if (bobgui_widget_get_mapped (BOBGUI_WIDGET (assistant)))
        update_actions_size (assistant);
    }

  if (assistant->constructed && assistant->use_header_bar)
    add_to_header_bar (assistant, child);
  else
    add_to_action_area (assistant, child);
}

/**
 * bobgui_assistant_remove_action_widget:
 * @assistant: a `BobguiAssistant`
 * @child: a `BobguiWidget`
 *
 * Removes a widget from the action area of a `BobguiAssistant`.
 *
 * Deprecated: 4.10: This widget will be removed in BOBGUI 5
 */
void
bobgui_assistant_remove_action_widget (BobguiAssistant *assistant,
                                    BobguiWidget    *child)
{
  g_return_if_fail (BOBGUI_IS_ASSISTANT (assistant));
  g_return_if_fail (BOBGUI_IS_WIDGET (child));

  if (BOBGUI_IS_BUTTON (child))
    {
      bobgui_size_group_remove_widget (assistant->button_size_group, child);
      assistant->extra_buttons -= 1;
      if (bobgui_widget_get_mapped (BOBGUI_WIDGET (assistant)))
        update_actions_size (assistant);
    }

  bobgui_box_remove (BOBGUI_BOX (assistant->action_area), child);
}

/**
 * bobgui_assistant_set_page_title:
 * @assistant: a `BobguiAssistant`
 * @page: a page of @assistant
 * @title: the new title for @page
 *
 * Sets a title for @page.
 *
 * The title is displayed in the header area of the assistant
 * when @page is the current page.
 *
 * Deprecated: 4.10: This widget will be removed in BOBGUI 5
 */
void
bobgui_assistant_set_page_title (BobguiAssistant *assistant,
                              BobguiWidget    *page,
                              const char   *title)
{
  BobguiAssistantPage *page_info;
  GList *child;

  g_return_if_fail (BOBGUI_IS_ASSISTANT (assistant));
  g_return_if_fail (BOBGUI_IS_WIDGET (page));

  child = find_page (assistant, page);

  g_return_if_fail (child != NULL);

  page_info = (BobguiAssistantPage*) child->data;

  g_object_set (page_info, "title", title, NULL);
}

/**
 * bobgui_assistant_get_page_title:
 * @assistant: a `BobguiAssistant`
 * @page: a page of @assistant
 *
 * Gets the title for @page.
 *
 * Returns: the title for @page
 *
 * Deprecated: 4.10: This widget will be removed in BOBGUI 5
 */
const char *
bobgui_assistant_get_page_title (BobguiAssistant *assistant,
                              BobguiWidget    *page)
{
  BobguiAssistantPage *page_info;
  GList *child;

  g_return_val_if_fail (BOBGUI_IS_ASSISTANT (assistant), NULL);
  g_return_val_if_fail (BOBGUI_IS_WIDGET (page), NULL);

  child = find_page (assistant, page);

  g_return_val_if_fail (child != NULL, NULL);

  page_info = (BobguiAssistantPage*) child->data;

  return page_info->title;
}

/**
 * bobgui_assistant_set_page_type:
 * @assistant: a `BobguiAssistant`
 * @page: a page of @assistant
 * @type: the new type for @page
 *
 * Sets the page type for @page.
 *
 * The page type determines the page behavior in the @assistant.
 *
 * Deprecated: 4.10: This widget will be removed in BOBGUI 5
 */
void
bobgui_assistant_set_page_type (BobguiAssistant         *assistant,
                             BobguiWidget            *page,
                             BobguiAssistantPageType  type)
{
  BobguiAssistantPage *page_info;
  GList *child;

  g_return_if_fail (BOBGUI_IS_ASSISTANT (assistant));
  g_return_if_fail (BOBGUI_IS_WIDGET (page));

  child = find_page (assistant, page);

  g_return_if_fail (child != NULL);

  page_info = (BobguiAssistantPage*) child->data;

  g_object_set (page_info, "page-type", type, NULL);
}

/**
 * bobgui_assistant_get_page_type:
 * @assistant: a `BobguiAssistant`
 * @page: a page of @assistant
 *
 * Gets the page type of @page.
 *
 * Returns: the page type of @page
 *
 * Deprecated: 4.10: This widget will be removed in BOBGUI 5
 */
BobguiAssistantPageType
bobgui_assistant_get_page_type (BobguiAssistant *assistant,
                             BobguiWidget    *page)
{
  BobguiAssistantPage *page_info;
  GList *child;

  g_return_val_if_fail (BOBGUI_IS_ASSISTANT (assistant), BOBGUI_ASSISTANT_PAGE_CONTENT);
  g_return_val_if_fail (BOBGUI_IS_WIDGET (page), BOBGUI_ASSISTANT_PAGE_CONTENT);

  child = find_page (assistant, page);

  g_return_val_if_fail (child != NULL, BOBGUI_ASSISTANT_PAGE_CONTENT);

  page_info = (BobguiAssistantPage*) child->data;

  return page_info->type;
}

/**
 * bobgui_assistant_set_page_complete:
 * @assistant: a `BobguiAssistant`
 * @page: a page of @assistant
 * @complete: the completeness status of the page
 *
 * Sets whether @page contents are complete.
 *
 * This will make @assistant update the buttons state
 * to be able to continue the task.
 *
 * Deprecated: 4.10: This widget will be removed in BOBGUI 5
 */
void
bobgui_assistant_set_page_complete (BobguiAssistant *assistant,
                                 BobguiWidget    *page,
                                 gboolean      complete)
{
  BobguiAssistantPage *page_info;
  GList *child;

  g_return_if_fail (BOBGUI_IS_ASSISTANT (assistant));
  g_return_if_fail (BOBGUI_IS_WIDGET (page));

  child = find_page (assistant, page);

  g_return_if_fail (child != NULL);

  page_info = (BobguiAssistantPage*) child->data;

  g_object_set (page_info, "complete", complete, NULL);
}

/**
 * bobgui_assistant_get_page_complete:
 * @assistant: a `BobguiAssistant`
 * @page: a page of @assistant
 *
 * Gets whether @page is complete.
 *
 * Returns: %TRUE if @page is complete.
 *
 * Deprecated: 4.10: This widget will be removed in BOBGUI 5
 */
gboolean
bobgui_assistant_get_page_complete (BobguiAssistant *assistant,
                                 BobguiWidget    *page)
{
  BobguiAssistantPage *page_info;
  GList *child;

  g_return_val_if_fail (BOBGUI_IS_ASSISTANT (assistant), FALSE);
  g_return_val_if_fail (BOBGUI_IS_WIDGET (page), FALSE);

  child = find_page (assistant, page);

  g_return_val_if_fail (child != NULL, FALSE);

  page_info = (BobguiAssistantPage*) child->data;

  return page_info->complete;
}

/**
 * bobgui_assistant_update_buttons_state:
 * @assistant: a `BobguiAssistant`
 *
 * Forces @assistant to recompute the buttons state.
 *
 * BOBGUI automatically takes care of this in most situations,
 * e.g. when the user goes to a different page, or when the
 * visibility or completeness of a page changes.
 *
 * One situation where it can be necessary to call this
 * function is when changing a value on the current page
 * affects the future page flow of the assistant.
 *
 * Deprecated: 4.10: This widget will be removed in BOBGUI 5
 */
void
bobgui_assistant_update_buttons_state (BobguiAssistant *assistant)
{
  g_return_if_fail (BOBGUI_IS_ASSISTANT (assistant));

  update_buttons_state (assistant);
}

/**
 * bobgui_assistant_commit:
 * @assistant: a `BobguiAssistant`
 *
 * Erases the visited page history.
 *
 * BOBGUI will then hide the back button on the current page,
 * and removes the cancel button from subsequent pages.
 *
 * Use this when the information provided up to the current
 * page is hereafter deemed permanent and cannot be modified
 * or undone. For example, showing a progress page to track
 * a long-running, unreversible operation after the user has
 * clicked apply on a confirmation page.
 *
 * Deprecated: 4.10: This widget will be removed in BOBGUI 5
 */
void
bobgui_assistant_commit (BobguiAssistant *assistant)
{
  g_return_if_fail (BOBGUI_IS_ASSISTANT (assistant));

  g_slist_free (assistant->visited_pages);
  assistant->visited_pages = NULL;

  assistant->committed = TRUE;

  update_buttons_state (assistant);
}

/* buildable implementation */

static BobguiBuildableIface *parent_buildable_iface;

static void
bobgui_assistant_buildable_interface_init (BobguiBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);

  iface->custom_tag_start = bobgui_assistant_buildable_custom_tag_start;
  iface->custom_finished = bobgui_assistant_buildable_custom_finished;
  iface->add_child = bobgui_assistant_buildable_add_child;
}

static void
bobgui_assistant_buildable_add_child (BobguiBuildable *buildable,
                                   BobguiBuilder   *builder,
                                   GObject      *child,
                                   const char   *type)
{
  if (BOBGUI_IS_ASSISTANT_PAGE (child))
    bobgui_assistant_add_page (BOBGUI_ASSISTANT (buildable), BOBGUI_ASSISTANT_PAGE (child), -1);
  else if (type && g_str_equal (type, "titlebar"))
    {
      BobguiAssistant *assistant = BOBGUI_ASSISTANT (buildable);
      assistant->headerbar = BOBGUI_WIDGET (child);
      bobgui_window_set_titlebar (BOBGUI_WINDOW (buildable), assistant->headerbar);
    }
  else
    parent_buildable_iface->add_child (buildable, builder, child, type);
}

gboolean
bobgui_assistant_buildable_custom_tag_start (BobguiBuildable  *buildable,
                                          BobguiBuilder    *builder,
                                          GObject       *child,
                                          const char    *tagname,
                                          BobguiBuildableParser *parser,
                                          gpointer      *data)
{
  return parent_buildable_iface->custom_tag_start (buildable, builder, child,
                                                   tagname, parser, data);
}

static void
bobgui_assistant_buildable_custom_finished (BobguiBuildable *buildable,
                                         BobguiBuilder   *builder,
                                         GObject      *child,
                                         const char   *tagname,
                                         gpointer      user_data)
{
  parent_buildable_iface->custom_finished (buildable, builder, child,
                                           tagname, user_data);
}

/**
 * bobgui_assistant_get_page:
 * @assistant: a `BobguiAssistant`
 * @child: a child of @assistant
 *
 * Returns the `BobguiAssistantPage` object for @child.
 *
 * Returns: (transfer none): the `BobguiAssistantPage` for @child
 *
 * Deprecated: 4.10: This widget will be removed in BOBGUI 5
 */
BobguiAssistantPage *
bobgui_assistant_get_page (BobguiAssistant *assistant,
                        BobguiWidget    *child)
{
  GList *page_info = find_page (assistant, child);
  return (BobguiAssistantPage *) (page_info ? page_info->data : NULL);
}

/**
 * bobgui_assistant_page_get_child:
 * @page: a `BobguiAssistantPage`
 *
 * Returns the child to which @page belongs.
 *
 * Returns: (transfer none): the child to which @page belongs
 *
 * Deprecated: 4.10: This widget will be removed in BOBGUI 5
 */
BobguiWidget *
bobgui_assistant_page_get_child (BobguiAssistantPage *page)
{
  return page->page;
}

static GType
bobgui_assistant_pages_get_item_type (GListModel *model)
{
  return BOBGUI_TYPE_ASSISTANT_PAGE;
}

static guint
bobgui_assistant_pages_get_n_items (GListModel *model)
{
  BobguiAssistantPages *pages = BOBGUI_ASSISTANT_PAGES (model);

  return g_list_length (pages->assistant->pages);
}

static gpointer
bobgui_assistant_pages_get_item (GListModel *model,
                              guint       position)
{
  BobguiAssistantPages *pages = BOBGUI_ASSISTANT_PAGES (model);
  BobguiAssistantPage *page;

  page = g_list_nth_data (pages->assistant->pages, position);

  return g_object_ref (page);
}

static void
bobgui_assistant_pages_list_model_init (GListModelInterface *iface)
{
  iface->get_item_type = bobgui_assistant_pages_get_item_type;
  iface->get_n_items = bobgui_assistant_pages_get_n_items;
  iface->get_item = bobgui_assistant_pages_get_item;
}

G_DEFINE_TYPE_WITH_CODE (BobguiAssistantPages, bobgui_assistant_pages, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (G_TYPE_LIST_MODEL, bobgui_assistant_pages_list_model_init))

static void
bobgui_assistant_pages_get_property (GObject    *object,
                                  guint       prop_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  BobguiAssistantPages *self = BOBGUI_ASSISTANT_PAGES (object);

  switch (prop_id)
    {
    case PAGES_PROP_ITEM_TYPE:
      g_value_set_gtype (value, BOBGUI_TYPE_ASSISTANT_PAGE);
      break;

    case PAGES_PROP_N_ITEMS:
      g_value_set_uint (value, bobgui_assistant_pages_get_n_items (G_LIST_MODEL (self)));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_assistant_pages_init (BobguiAssistantPages *pages)
{
}

static void
bobgui_assistant_pages_class_init (BobguiAssistantPagesClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = bobgui_assistant_pages_get_property;

  pages_properties[PAGES_PROP_ITEM_TYPE] =
    g_param_spec_gtype ("item-type", NULL, NULL,
                        BOBGUI_TYPE_ASSISTANT_PAGE,
                        G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  pages_properties[PAGES_PROP_N_ITEMS] =
    g_param_spec_uint ("n-items", NULL, NULL,
                       0, G_MAXUINT, 0,
                       G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, PAGES_N_PROPS, pages_properties);
}

static BobguiAssistantPages *
bobgui_assistant_pages_new (BobguiAssistant *assistant)
{
  BobguiAssistantPages *pages;

  pages = g_object_new (BOBGUI_TYPE_ASSISTANT_PAGES, NULL);
  pages->assistant = assistant;

  return pages;
}

/**
 * bobgui_assistant_get_pages:
 * @assistant: a `BobguiAssistant`
 * 
 * Gets a list model of the assistant pages.
 *
 * Returns: (transfer full) (attributes element-type=BobguiAssistantPage): A list model of the pages.
 *
 * Deprecated: 4.10: This widget will be removed in BOBGUI 5
 */
GListModel *
bobgui_assistant_get_pages (BobguiAssistant *assistant)
{
  g_return_val_if_fail (BOBGUI_IS_ASSISTANT (assistant), NULL);

  if (assistant->model)
    return g_object_ref (assistant->model);

  assistant->model = G_LIST_MODEL (bobgui_assistant_pages_new (assistant));

  g_object_add_weak_pointer (G_OBJECT (assistant->model), (gpointer *)&assistant->model);

  return assistant->model;
}
