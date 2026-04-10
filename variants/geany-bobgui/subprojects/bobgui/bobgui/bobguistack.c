/*
 * Copyright (c) 2013 Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Author: Alexander Larsson <alexl@redhat.com>
 *
 */

#include "config.h"

#include <bobgui/bobgui.h>
#include "bobguistack.h"
#include "bobguienums.h"
#include "bobguiaccessibleprivate.h"
#include "bobguiatcontextprivate.h"
#include "bobguiprivate.h"
#include "bobguiprogresstrackerprivate.h"
#include "bobguisettingsprivate.h"
#include "bobguisnapshot.h"
#include "bobguiwidgetprivate.h"
#include "bobguisingleselection.h"
#include "bobguilistlistmodelprivate.h"
#include <math.h>
#include <string.h>

/**
 * BobguiStack:
 *
 * Shows one of its children at a time.
 *
 * <picture>
 *   <source srcset="stack-dark.png" media="(prefers-color-scheme: dark)">
 *   <img alt="An example BobguiStack" src="stack.png">
 * </picture>
 *
 * In contrast to `BobguiNotebook`, `BobguiStack` does not provide a means
 * for users to change the visible child. Instead, a separate widget
 * such as [class@Bobgui.StackSwitcher] or [class@Bobgui.StackSidebar] can
 * be used with `BobguiStack` to provide this functionality.
 *
 * Transitions between pages can be animated as slides or fades. This
 * can be controlled with [method@Bobgui.Stack.set_transition_type].
 * These animations respect the [property@Bobgui.Settings:bobgui-enable-animations]
 * setting.
 *
 * `BobguiStack` maintains a [class@Bobgui.StackPage] object for each added
 * child, which holds additional per-child properties. You
 * obtain the `BobguiStackPage` for a child with [method@Bobgui.Stack.get_page]
 * and you can obtain a `BobguiSelectionModel` containing all the pages
 * with [method@Bobgui.Stack.get_pages].
 *
 * # BobguiStack as BobguiBuildable
 *
 * To set child-specific properties in a .ui file, create `BobguiStackPage`
 * objects explicitly, and set the child widget as a property on it:
 *
 * ```xml
 *   <object class="BobguiStack" id="stack">
 *     <child>
 *       <object class="BobguiStackPage">
 *         <property name="name">page1</property>
 *         <property name="title">In the beginning…</property>
 *         <property name="child">
 *           <object class="BobguiLabel">
 *             <property name="label">It was dark</property>
 *           </object>
 *         </property>
 *       </object>
 *     </child>
 * ```
 *
 * # CSS nodes
 *
 * `BobguiStack` has a single CSS node named stack.
 *
 * # Accessibility
 *
 * `BobguiStack` uses the [enum@Bobgui.AccessibleRole.tab_panel] role for the stack
 * pages, which are the accessible parent objects of the child widgets.
 */

/**
 * BobguiStackTransitionType:
 * @BOBGUI_STACK_TRANSITION_TYPE_NONE: No transition
 * @BOBGUI_STACK_TRANSITION_TYPE_CROSSFADE: A cross-fade
 * @BOBGUI_STACK_TRANSITION_TYPE_SLIDE_RIGHT: Slide from left to right
 * @BOBGUI_STACK_TRANSITION_TYPE_SLIDE_LEFT: Slide from right to left
 * @BOBGUI_STACK_TRANSITION_TYPE_SLIDE_UP: Slide from bottom up
 * @BOBGUI_STACK_TRANSITION_TYPE_SLIDE_DOWN: Slide from top down
 * @BOBGUI_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT: Slide from left or right according to the children order
 * @BOBGUI_STACK_TRANSITION_TYPE_SLIDE_UP_DOWN: Slide from top down or bottom up according to the order
 * @BOBGUI_STACK_TRANSITION_TYPE_OVER_UP: Cover the old page by sliding up
 * @BOBGUI_STACK_TRANSITION_TYPE_OVER_DOWN: Cover the old page by sliding down
 * @BOBGUI_STACK_TRANSITION_TYPE_OVER_LEFT: Cover the old page by sliding to the left
 * @BOBGUI_STACK_TRANSITION_TYPE_OVER_RIGHT: Cover the old page by sliding to the right
 * @BOBGUI_STACK_TRANSITION_TYPE_UNDER_UP: Uncover the new page by sliding up
 * @BOBGUI_STACK_TRANSITION_TYPE_UNDER_DOWN: Uncover the new page by sliding down
 * @BOBGUI_STACK_TRANSITION_TYPE_UNDER_LEFT: Uncover the new page by sliding to the left
 * @BOBGUI_STACK_TRANSITION_TYPE_UNDER_RIGHT: Uncover the new page by sliding to the right
 * @BOBGUI_STACK_TRANSITION_TYPE_OVER_UP_DOWN: Cover the old page sliding up or uncover the new page sliding down, according to order
 * @BOBGUI_STACK_TRANSITION_TYPE_OVER_DOWN_UP: Cover the old page sliding down or uncover the new page sliding up, according to order
 * @BOBGUI_STACK_TRANSITION_TYPE_OVER_LEFT_RIGHT: Cover the old page sliding left or uncover the new page sliding right, according to order
 * @BOBGUI_STACK_TRANSITION_TYPE_OVER_RIGHT_LEFT: Cover the old page sliding right or uncover the new page sliding left, according to order
 * @BOBGUI_STACK_TRANSITION_TYPE_ROTATE_LEFT: Pretend the pages are sides of a cube and rotate that cube to the left
 * @BOBGUI_STACK_TRANSITION_TYPE_ROTATE_RIGHT: Pretend the pages are sides of a cube and rotate that cube to the right
 * @BOBGUI_STACK_TRANSITION_TYPE_ROTATE_LEFT_RIGHT: Pretend the pages are sides of a cube and rotate that cube to the left or right according to the children order
 *
 * Possible transitions between pages in a `BobguiStack` widget.
 *
 * New values may be added to this enumeration over time.
 */

/**
 * BobguiStackPage:
 *
 * An auxiliary class used by `BobguiStack`.
 */

/* TODO:
 *  filter events out events to the last_child widget during transitions
 */

struct _BobguiStack {
  BobguiWidget parent_instance;
};

typedef struct _BobguiStackClass BobguiStackClass;
struct _BobguiStackClass {
  BobguiWidgetClass parent_class;
};

typedef struct {
  GPtrArray *children;

  BobguiStackPage *visible_child;

  gboolean homogeneous[2];

  BobguiStackTransitionType transition_type;
  guint transition_duration;

  BobguiStackPage *last_visible_child;
  guint tick_id;
  BobguiProgressTracker tracker;
  gboolean first_frame_skipped;

  int last_visible_widget_width;
  int last_visible_widget_height;

  gboolean interpolate_size;

  BobguiStackTransitionType active_transition_type;

  BobguiSelectionModel *pages;

} BobguiStackPrivate;

static void bobgui_stack_buildable_interface_init (BobguiBuildableIface *iface);
static void bobgui_stack_accessible_init (BobguiAccessibleInterface *iface);

G_DEFINE_TYPE_WITH_CODE (BobguiStack, bobgui_stack, BOBGUI_TYPE_WIDGET,
                         G_ADD_PRIVATE (BobguiStack)
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_ACCESSIBLE,
                                                bobgui_stack_accessible_init)
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_BUILDABLE,
                                                bobgui_stack_buildable_interface_init))
enum  {
  PROP_0,
  PROP_HHOMOGENEOUS,
  PROP_VHOMOGENEOUS,
  PROP_VISIBLE_CHILD,
  PROP_VISIBLE_CHILD_NAME,
  PROP_TRANSITION_DURATION,
  PROP_TRANSITION_TYPE,
  PROP_TRANSITION_RUNNING,
  PROP_INTERPOLATE_SIZE,
  PROP_PAGES,
  LAST_PROP
};

enum
{
  CHILD_PROP_0,
  CHILD_PROP_CHILD,
  CHILD_PROP_NAME,
  CHILD_PROP_TITLE,
  CHILD_PROP_ICON_NAME,
  CHILD_PROP_NEEDS_ATTENTION,
  CHILD_PROP_VISIBLE,
  CHILD_PROP_USE_UNDERLINE,
  LAST_CHILD_PROP,

  PROP_ACCESSIBLE_ROLE
};

struct _BobguiStackPage
{
  GObject parent_instance;

  BobguiWidget *widget;
  char *name;
  char *title;
  char *icon_name;
  BobguiWidget *last_focus;

  BobguiStackPage *next_page;

  BobguiATContext *at_context;

  guint needs_attention : 1;
  guint visible         : 1;
  guint use_underline   : 1;
  guint in_destruction  : 1;
};

typedef struct _BobguiStackPageClass BobguiStackPageClass;
struct _BobguiStackPageClass
{
  GObjectClass parent_class;
};

static GParamSpec *stack_props[LAST_PROP] = { NULL, };
static GParamSpec *stack_page_props[LAST_CHILD_PROP] = { NULL, };

static BobguiATContext *
bobgui_stack_page_accessible_get_at_context (BobguiAccessible *accessible)
{
  BobguiStackPage *page = BOBGUI_STACK_PAGE (accessible);

  if (page->in_destruction)
    {
      BOBGUI_DEBUG (A11Y, "ATContext for “%s” [%p] accessed during destruction",
                       G_OBJECT_TYPE_NAME (accessible),
                       accessible);
      return NULL;
    }

  if (page->at_context == NULL)
    {
      BobguiAccessibleRole role = BOBGUI_ACCESSIBLE_ROLE_TAB_PANEL;
      GdkDisplay *display;

      if (page->widget != NULL)
        display = bobgui_widget_get_display (page->widget);
      else
        display = gdk_display_get_default ();

      page->at_context = bobgui_at_context_create (role, accessible, display);
      if (page->at_context == NULL)
        return NULL;
    }

  return g_object_ref (page->at_context);
}

static gboolean
bobgui_stack_page_accessible_get_platform_state (BobguiAccessible              *self,
                                              BobguiAccessiblePlatformState  state)
{
  return FALSE;
}

static BobguiAccessible *
bobgui_stack_page_accessible_get_accessible_parent (BobguiAccessible *accessible)
{
  BobguiStackPage *page = BOBGUI_STACK_PAGE (accessible);
  BobguiWidget *parent;

  if (page->widget == NULL)
    return NULL;

  parent = _bobgui_widget_get_parent (page->widget);

  return BOBGUI_ACCESSIBLE (g_object_ref (parent));
}

static BobguiAccessible *
bobgui_stack_page_accessible_get_first_accessible_child (BobguiAccessible *accessible)
{
  BobguiStackPage *page = BOBGUI_STACK_PAGE (accessible);

  if (page->widget == NULL)
    return NULL;

  return BOBGUI_ACCESSIBLE (g_object_ref (page->widget));
}

static BobguiAccessible *
bobgui_stack_page_accessible_get_next_accessible_sibling (BobguiAccessible *accessible)
{
  BobguiStackPage *page = BOBGUI_STACK_PAGE (accessible);

  if (page->next_page == NULL)
    return NULL;

  return BOBGUI_ACCESSIBLE (g_object_ref (page->next_page));
}

static gboolean
bobgui_stack_page_accessible_get_bounds (BobguiAccessible *accessible,
                                      int           *x,
                                      int           *y,
                                      int           *width,
                                      int           *height)
{
  BobguiStackPage *page = BOBGUI_STACK_PAGE (accessible);
  if (page->widget != NULL)
    return bobgui_accessible_get_bounds (BOBGUI_ACCESSIBLE (page->widget), x, y, width, height);
  else
    return FALSE;
}

static void
bobgui_stack_page_accessible_init (BobguiAccessibleInterface *iface)
{
  iface->get_at_context = bobgui_stack_page_accessible_get_at_context;
  iface->get_platform_state = bobgui_stack_page_accessible_get_platform_state;
  iface->get_accessible_parent = bobgui_stack_page_accessible_get_accessible_parent;
  iface->get_first_accessible_child = bobgui_stack_page_accessible_get_first_accessible_child;
  iface->get_next_accessible_sibling = bobgui_stack_page_accessible_get_next_accessible_sibling;
  iface->get_bounds = bobgui_stack_page_accessible_get_bounds;
}

G_DEFINE_TYPE_WITH_CODE (BobguiStackPage, bobgui_stack_page, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_ACCESSIBLE,
                                                bobgui_stack_page_accessible_init))

static void
bobgui_stack_page_init (BobguiStackPage *page)
{
  page->visible = TRUE;
}

static void
bobgui_stack_page_finalize (GObject *object)
{
  BobguiStackPage *page = BOBGUI_STACK_PAGE (object);

  g_clear_object (&page->widget);
  g_free (page->name);
  g_free (page->title);
  g_free (page->icon_name);

  if (page->last_focus)
    g_object_remove_weak_pointer (G_OBJECT (page->last_focus),
                                  (gpointer *)&page->last_focus);

  G_OBJECT_CLASS (bobgui_stack_page_parent_class)->finalize (object);
}

static void
bobgui_stack_page_dispose (GObject *object)
{
  BobguiStackPage *page = BOBGUI_STACK_PAGE (object);

  page->in_destruction = TRUE;

  g_clear_object (&page->at_context);

  G_OBJECT_CLASS (bobgui_stack_page_parent_class)->dispose (object);
}

static void
bobgui_stack_page_get_property (GObject      *object,
                             guint         property_id,
                             GValue       *value,
                             GParamSpec   *pspec)
{
  BobguiStackPage *info = BOBGUI_STACK_PAGE (object);

  switch (property_id)
    {
    case CHILD_PROP_CHILD:
      g_value_set_object (value, info->widget);
      break;

    case CHILD_PROP_NAME:
      g_value_set_string (value, bobgui_stack_page_get_name (info));
      break;

    case CHILD_PROP_TITLE:
      g_value_set_string (value, bobgui_stack_page_get_title (info));
      break;

    case CHILD_PROP_ICON_NAME:
      g_value_set_string (value, bobgui_stack_page_get_icon_name (info));
      break;

    case CHILD_PROP_NEEDS_ATTENTION:
      g_value_set_boolean (value, bobgui_stack_page_get_needs_attention (info));
      break;

    case CHILD_PROP_VISIBLE:
      g_value_set_boolean (value, bobgui_stack_page_get_visible (info));
      break;

    case CHILD_PROP_USE_UNDERLINE:
      g_value_set_boolean (value, bobgui_stack_page_get_use_underline (info));
      break;

    case PROP_ACCESSIBLE_ROLE:
      g_value_set_enum (value, BOBGUI_ACCESSIBLE_ROLE_TAB_PANEL);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
bobgui_stack_page_set_property (GObject      *object,
                             guint         property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  BobguiStackPage *info = BOBGUI_STACK_PAGE (object);

  switch (property_id)
    {
    case CHILD_PROP_CHILD:
      g_set_object (&info->widget, g_value_get_object (value));
      bobgui_accessible_set_accessible_parent (BOBGUI_ACCESSIBLE (info->widget),
                                            BOBGUI_ACCESSIBLE (info),
                                            NULL);
      break;

    case CHILD_PROP_NAME:
      bobgui_stack_page_set_name (info, g_value_get_string (value));
      break;

    case CHILD_PROP_TITLE:
      bobgui_stack_page_set_title (info, g_value_get_string (value));
      break;

    case CHILD_PROP_ICON_NAME:
      bobgui_stack_page_set_icon_name (info, g_value_get_string (value));
      break;

    case CHILD_PROP_NEEDS_ATTENTION:
      bobgui_stack_page_set_needs_attention (info, g_value_get_boolean (value));
      break;

    case CHILD_PROP_VISIBLE:
      bobgui_stack_page_set_visible (info, g_value_get_boolean (value));
      break;

    case CHILD_PROP_USE_UNDERLINE:
      bobgui_stack_page_set_use_underline (info, g_value_get_boolean (value));
      break;

    case PROP_ACCESSIBLE_ROLE:
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
bobgui_stack_page_constructed (GObject *gobject)
{
  BobguiStackPage *self = BOBGUI_STACK_PAGE (gobject);

  if (G_UNLIKELY (self->widget == NULL))
    g_error ("BobguiStackPage '%s' [%p] is missing a child widget",
             self->name != NULL ? self->name : "<unnamed>",
             self);

  G_OBJECT_CLASS (bobgui_stack_page_parent_class)->constructed (gobject);
}

static void
bobgui_stack_page_class_init (BobguiStackPageClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->finalize = bobgui_stack_page_finalize;
  object_class->dispose = bobgui_stack_page_dispose;
  object_class->get_property = bobgui_stack_page_get_property;
  object_class->set_property = bobgui_stack_page_set_property;
  object_class->constructed = bobgui_stack_page_constructed;

  /**
   * BobguiStackPage:child:
   *
   * The child that this page is for.
   */
  stack_page_props[CHILD_PROP_CHILD] =
    g_param_spec_object ("child", NULL, NULL,
                         BOBGUI_TYPE_WIDGET,
                         BOBGUI_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  /**
   * BobguiStackPage:name:
   *
   * The name of the child page.
   */
  stack_page_props[CHILD_PROP_NAME] =
    g_param_spec_string ("name", NULL, NULL,
                         NULL,
                         BOBGUI_PARAM_READWRITE);

  /**
   * BobguiStackPage:title:
   *
   * The title of the child page.
   */
  stack_page_props[CHILD_PROP_TITLE] =
    g_param_spec_string ("title", NULL, NULL,
                         NULL,
                         BOBGUI_PARAM_READWRITE);

  /**
   * BobguiStackPage:icon-name:
   *
   * The icon name of the child page.
   */
  stack_page_props[CHILD_PROP_ICON_NAME] =
    g_param_spec_string ("icon-name", NULL, NULL,
                         NULL,
                         BOBGUI_PARAM_READWRITE);

  /**
   * BobguiStackPage:needs-attention:
   *
   * Whether the page requires the user attention.
   *
   * This is used by the [class@Bobgui.StackSwitcher] to change the
   * appearance of the corresponding button when a page needs
   * attention and it is not the current one.
   */
  stack_page_props[CHILD_PROP_NEEDS_ATTENTION] =
    g_param_spec_boolean ("needs-attention", NULL, NULL,
                         FALSE,
                         BOBGUI_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiStackPage:visible:
   *
   * Whether this page is visible.
   */
  stack_page_props[CHILD_PROP_VISIBLE] =
    g_param_spec_boolean ("visible", NULL, NULL,
                         TRUE,
                         BOBGUI_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiStackPage:use-underline:
   *
   * If set, an underline in the title indicates a mnemonic.
   */
  stack_page_props[CHILD_PROP_USE_UNDERLINE] =
    g_param_spec_boolean ("use-underline", NULL, NULL,
                         FALSE,
                         BOBGUI_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, LAST_CHILD_PROP, stack_page_props);

  g_object_class_override_property (object_class, PROP_ACCESSIBLE_ROLE, "accessible-role");
}

#define BOBGUI_TYPE_STACK_PAGES (bobgui_stack_pages_get_type ())
G_DECLARE_FINAL_TYPE (BobguiStackPages, bobgui_stack_pages, BOBGUI, STACK_PAGES, GObject)

struct _BobguiStackPages
{
  GObject parent_instance;
  BobguiStack *stack;
};

struct _BobguiStackPagesClass
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

static GType
bobgui_stack_pages_get_item_type (GListModel *model)
{
  return BOBGUI_TYPE_STACK_PAGE;
}

static guint
bobgui_stack_pages_get_n_items (GListModel *model)
{
  BobguiStackPages *pages = BOBGUI_STACK_PAGES (model);
  BobguiStackPrivate *priv = bobgui_stack_get_instance_private (pages->stack);

  return priv->children->len;
}

static gpointer
bobgui_stack_pages_get_item (GListModel *model,
                          guint       position)
{
  BobguiStackPages *pages = BOBGUI_STACK_PAGES (model);
  BobguiStackPrivate *priv = bobgui_stack_get_instance_private (pages->stack);
  BobguiStackPage *page;

  if (position >= priv->children->len)
    return NULL;

  page = g_ptr_array_index (priv->children, position);

  return g_object_ref (page);
}

static void
bobgui_stack_pages_list_model_init (GListModelInterface *iface)
{
  iface->get_item_type = bobgui_stack_pages_get_item_type;
  iface->get_n_items = bobgui_stack_pages_get_n_items;
  iface->get_item = bobgui_stack_pages_get_item;
}

static gboolean
bobgui_stack_pages_is_selected (BobguiSelectionModel *model,
                             guint              position)
{
  BobguiStackPages *pages = BOBGUI_STACK_PAGES (model);
  BobguiStackPrivate *priv = bobgui_stack_get_instance_private (pages->stack);
  BobguiStackPage *page;

  if (position >= priv->children->len)
    return FALSE;

  page = g_ptr_array_index (priv->children, position);

  return page == priv->visible_child;
}

static void set_visible_child (BobguiStack               *stack,
                               BobguiStackPage           *child_info,
                               BobguiStackTransitionType  transition_type,
                               guint                   transition_duration);

static gboolean
bobgui_stack_pages_select_item (BobguiSelectionModel *model,
                             guint              position,
                             gboolean           exclusive)
{
  BobguiStackPages *pages = BOBGUI_STACK_PAGES (model);
  BobguiStackPrivate *priv = bobgui_stack_get_instance_private (pages->stack);
  BobguiStackPage *page;

  if (position >= priv->children->len)
    return FALSE;

  page = g_ptr_array_index (priv->children, position);

  set_visible_child (pages->stack, page, priv->transition_type, priv->transition_duration);

  return TRUE;
}

static void
bobgui_stack_pages_selection_model_init (BobguiSelectionModelInterface *iface)
{
  iface->is_selected = bobgui_stack_pages_is_selected;
  iface->select_item = bobgui_stack_pages_select_item;
}

G_DEFINE_TYPE_WITH_CODE (BobguiStackPages, bobgui_stack_pages, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (G_TYPE_LIST_MODEL, bobgui_stack_pages_list_model_init)
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_SELECTION_MODEL, bobgui_stack_pages_selection_model_init))

static void
bobgui_stack_pages_get_property (GObject    *object,
                              guint       prop_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  BobguiStackPages *self = BOBGUI_STACK_PAGES (object);

  switch (prop_id)
    {
    case PAGES_PROP_ITEM_TYPE:
      g_value_set_gtype (value, BOBGUI_TYPE_STACK_PAGE);
      break;

    case PAGES_PROP_N_ITEMS:
      g_value_set_uint (value, bobgui_stack_pages_get_n_items (G_LIST_MODEL (self)));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_stack_pages_init (BobguiStackPages *pages)
{
}

static void
bobgui_stack_pages_class_init (BobguiStackPagesClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = bobgui_stack_pages_get_property;

  pages_properties[PAGES_PROP_ITEM_TYPE] =
    g_param_spec_gtype ("item-type", NULL, NULL,
                        BOBGUI_TYPE_STACK_PAGE,
                        G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  pages_properties[PAGES_PROP_N_ITEMS] =
    g_param_spec_uint ("n-items", NULL, NULL,
                       0, G_MAXUINT, 0,
                       G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, PAGES_N_PROPS, pages_properties);
}

static BobguiStackPages *
bobgui_stack_pages_new (BobguiStack *stack)
{
  BobguiStackPages *pages;

  pages = g_object_new (BOBGUI_TYPE_STACK_PAGES, NULL);
  pages->stack = stack;

  return pages;
}

static BobguiStackPage *bobgui_stack_add_internal (BobguiStack *stack,
                                             BobguiWidget  *child,
                                             const char *name,
                                             const char *title);

static BobguiSizeRequestMode bobgui_stack_get_request_mode (BobguiWidget *widget);
static void     bobgui_stack_compute_expand                 (BobguiWidget     *widget,
                                                          gboolean      *hexpand,
                                                          gboolean      *vexpand);
static void     bobgui_stack_size_allocate                  (BobguiWidget     *widget,
                                                          int            width,
                                                          int            height,
                                                          int            baseline);
static void     bobgui_stack_snapshot                       (BobguiWidget     *widget,
                                                          BobguiSnapshot   *snapshot);
static void     bobgui_stack_measure                        (BobguiWidget      *widget,
                                                          BobguiOrientation  orientation,
                                                          int             for_size,
                                                          int            *minimum,
                                                          int            *natural,
                                                          int            *minimum_baseline,
                                                          int            *natural_baseline);
static void     bobgui_stack_dispose                        (GObject       *obj);
static void     bobgui_stack_finalize                       (GObject       *obj);
static void     bobgui_stack_get_property                   (GObject       *object,
                                                          guint          property_id,
                                                          GValue        *value,
                                                          GParamSpec    *pspec);
static void     bobgui_stack_set_property                   (GObject       *object,
                                                          guint          property_id,
                                                          const GValue  *value,
                                                          GParamSpec    *pspec);
static void     bobgui_stack_unschedule_ticks               (BobguiStack      *stack);


static void     bobgui_stack_add_page                       (BobguiStack     *stack,
                                                          BobguiStackPage *page);

static BobguiBuildableIface *parent_buildable_iface;

static void
bobgui_stack_buildable_add_child (BobguiBuildable *buildable,
                               BobguiBuilder   *builder,
                               GObject      *child,
                               const char   *type)
{
  if (BOBGUI_IS_STACK_PAGE (child))
    bobgui_stack_add_page (BOBGUI_STACK (buildable), BOBGUI_STACK_PAGE (child));
  else if (BOBGUI_IS_WIDGET (child))
    bobgui_stack_add_internal (BOBGUI_STACK (buildable), BOBGUI_WIDGET (child), NULL, NULL);
  else
    parent_buildable_iface->add_child (buildable, builder, child, type);
}

static void
bobgui_stack_buildable_interface_init (BobguiBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);

  iface->add_child = bobgui_stack_buildable_add_child;
}

static BobguiAccessible *
bobgui_stack_accessible_get_first_accessible_child (BobguiAccessible *accessible)
{
  BobguiStack *stack = BOBGUI_STACK (accessible);
  BobguiStackPrivate *priv = bobgui_stack_get_instance_private (stack);
  BobguiAccessible *page_accessible = NULL;

  if (priv->children->len > 0)
    page_accessible = BOBGUI_ACCESSIBLE (g_object_ref (g_ptr_array_index (priv->children, 0)));

  return page_accessible;
}

static void
bobgui_stack_accessible_init (BobguiAccessibleInterface *iface)
{
  iface->get_first_accessible_child = bobgui_stack_accessible_get_first_accessible_child;
}

static void stack_remove (BobguiStack  *stack,
                          BobguiWidget *child,
                          gboolean   in_dispose);

static void
bobgui_stack_dispose (GObject *obj)
{
  BobguiStack *stack = BOBGUI_STACK (obj);
  BobguiStackPrivate *priv = bobgui_stack_get_instance_private (stack);
  BobguiWidget *child;
  guint n_pages = priv->children->len;

  while ((child = bobgui_widget_get_first_child (BOBGUI_WIDGET (stack))))
    stack_remove (stack, child, TRUE);

  if (priv->pages && n_pages > 0)
    {
      g_list_model_items_changed (G_LIST_MODEL (priv->pages), 0, n_pages, 0);
      g_object_notify_by_pspec (G_OBJECT (priv->pages), pages_properties[PAGES_PROP_N_ITEMS]);
    }

  G_OBJECT_CLASS (bobgui_stack_parent_class)->dispose (obj);
}

static void
bobgui_stack_finalize (GObject *obj)
{
  BobguiStack *stack = BOBGUI_STACK (obj);
  BobguiStackPrivate *priv = bobgui_stack_get_instance_private (stack);

  if (priv->pages)
    g_object_remove_weak_pointer (G_OBJECT (priv->pages), (gpointer *)&priv->pages);

  bobgui_stack_unschedule_ticks (stack);

  g_ptr_array_free (priv->children, TRUE);

  G_OBJECT_CLASS (bobgui_stack_parent_class)->finalize (obj);
}

static void
bobgui_stack_get_property (GObject   *object,
                        guint       property_id,
                        GValue     *value,
                        GParamSpec *pspec)
{
  BobguiStack *stack = BOBGUI_STACK (object);

  switch (property_id)
    {
    case PROP_HHOMOGENEOUS:
      g_value_set_boolean (value, bobgui_stack_get_hhomogeneous (stack));
      break;
    case PROP_VHOMOGENEOUS:
      g_value_set_boolean (value, bobgui_stack_get_vhomogeneous (stack));
      break;
    case PROP_VISIBLE_CHILD:
      g_value_set_object (value, bobgui_stack_get_visible_child (stack));
      break;
    case PROP_VISIBLE_CHILD_NAME:
      g_value_set_string (value, bobgui_stack_get_visible_child_name (stack));
      break;
    case PROP_TRANSITION_DURATION:
      g_value_set_uint (value, bobgui_stack_get_transition_duration (stack));
      break;
    case PROP_TRANSITION_TYPE:
      g_value_set_enum (value, bobgui_stack_get_transition_type (stack));
      break;
    case PROP_TRANSITION_RUNNING:
      g_value_set_boolean (value, bobgui_stack_get_transition_running (stack));
      break;
    case PROP_INTERPOLATE_SIZE:
      g_value_set_boolean (value, bobgui_stack_get_interpolate_size (stack));
      break;
    case PROP_PAGES:
      g_value_take_object (value, bobgui_stack_get_pages (stack));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
bobgui_stack_set_property (GObject     *object,
                        guint         property_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
  BobguiStack *stack = BOBGUI_STACK (object);

  switch (property_id)
    {
    case PROP_HHOMOGENEOUS:
      bobgui_stack_set_hhomogeneous (stack, g_value_get_boolean (value));
      break;
    case PROP_VHOMOGENEOUS:
      bobgui_stack_set_vhomogeneous (stack, g_value_get_boolean (value));
      break;
    case PROP_VISIBLE_CHILD:
      bobgui_stack_set_visible_child (stack, g_value_get_object (value));
      break;
    case PROP_VISIBLE_CHILD_NAME:
      bobgui_stack_set_visible_child_name (stack, g_value_get_string (value));
      break;
    case PROP_TRANSITION_DURATION:
      bobgui_stack_set_transition_duration (stack, g_value_get_uint (value));
      break;
    case PROP_TRANSITION_TYPE:
      bobgui_stack_set_transition_type (stack, g_value_get_enum (value));
      break;
    case PROP_INTERPOLATE_SIZE:
      bobgui_stack_set_interpolate_size (stack, g_value_get_boolean (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
bobgui_stack_class_init (BobguiStackClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);

  object_class->get_property = bobgui_stack_get_property;
  object_class->set_property = bobgui_stack_set_property;
  object_class->dispose = bobgui_stack_dispose;
  object_class->finalize = bobgui_stack_finalize;

  widget_class->size_allocate = bobgui_stack_size_allocate;
  widget_class->snapshot = bobgui_stack_snapshot;
  widget_class->measure = bobgui_stack_measure;
  widget_class->compute_expand = bobgui_stack_compute_expand;
  widget_class->get_request_mode = bobgui_stack_get_request_mode;

  /**
   * BobguiStack:hhomogeneous:
   *
   * %TRUE if the stack allocates the same width for all children.
   */
  stack_props[PROP_HHOMOGENEOUS] =
      g_param_spec_boolean ("hhomogeneous", NULL, NULL,
                            TRUE,
                            BOBGUI_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiStack:vhomogeneous:
   *
   * %TRUE if the stack allocates the same height for all children.
   */
  stack_props[PROP_VHOMOGENEOUS] =
      g_param_spec_boolean ("vhomogeneous", NULL, NULL,
                            TRUE,
                            BOBGUI_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiStack:visible-child:
   *
   * The widget currently visible in the stack.
   */
  stack_props[PROP_VISIBLE_CHILD] =
      g_param_spec_object ("visible-child", NULL, NULL,
                           BOBGUI_TYPE_WIDGET,
                           BOBGUI_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiStack:visible-child-name:
   *
   * The name of the widget currently visible in the stack.
   */
  stack_props[PROP_VISIBLE_CHILD_NAME] =
      g_param_spec_string ("visible-child-name", NULL, NULL,
                           NULL,
                           BOBGUI_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiStack:transition-duration:
   *
   * The animation duration, in milliseconds.
   */
  stack_props[PROP_TRANSITION_DURATION] =
      g_param_spec_uint ("transition-duration", NULL, NULL,
                         0, G_MAXUINT, 200,
                         BOBGUI_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiStack:transition-type:
   *
   * The type of animation used to transition.
   */
  stack_props[PROP_TRANSITION_TYPE] =
      g_param_spec_enum ("transition-type", NULL, NULL,
                         BOBGUI_TYPE_STACK_TRANSITION_TYPE, BOBGUI_STACK_TRANSITION_TYPE_NONE,
                         BOBGUI_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiStack:transition-running:
   *
   * Whether or not the transition is currently running.
   */
  stack_props[PROP_TRANSITION_RUNNING] =
      g_param_spec_boolean ("transition-running", NULL, NULL,
                            FALSE,
                            BOBGUI_PARAM_READABLE);

  /**
   * BobguiStack:interpolate-size:
   *
   * Whether or not the size should smoothly change during the transition.
   */
  stack_props[PROP_INTERPOLATE_SIZE] =
      g_param_spec_boolean ("interpolate-size", NULL, NULL,
                            FALSE,
                            BOBGUI_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiStack:pages:
   *
   * A selection model with the stack pages.
   */
  stack_props[PROP_PAGES] =
      g_param_spec_object ("pages", NULL, NULL,
                           BOBGUI_TYPE_SELECTION_MODEL,
                           BOBGUI_PARAM_READABLE);

  g_object_class_install_properties (object_class, LAST_PROP, stack_props);

  bobgui_widget_class_set_css_name (widget_class, I_("stack"));
  bobgui_widget_class_set_accessible_role (widget_class, BOBGUI_ACCESSIBLE_ROLE_GROUP);
}

/**
 * bobgui_stack_new:
 *
 * Creates a new `BobguiStack`.
 *
 * Returns: a new `BobguiStack`
 */
BobguiWidget *
bobgui_stack_new (void)
{
  return g_object_new (BOBGUI_TYPE_STACK, NULL);
}

static BobguiStackPage *
find_child_info_for_widget (BobguiStack  *stack,
                            BobguiWidget *child)
{
  BobguiStackPrivate *priv = bobgui_stack_get_instance_private (stack);
  BobguiStackPage *info;
  guint idx;

  for (idx = 0; idx < priv->children->len; idx++)
    {
      info = g_ptr_array_index (priv->children, idx);
      if (info->widget == child)
        return info;
    }

  return NULL;
}

static inline gboolean
is_left_transition (BobguiStackTransitionType transition_type)
{
  return (transition_type == BOBGUI_STACK_TRANSITION_TYPE_SLIDE_LEFT ||
          transition_type == BOBGUI_STACK_TRANSITION_TYPE_OVER_LEFT);
}

static inline gboolean
is_right_transition (BobguiStackTransitionType transition_type)
{
  return (transition_type == BOBGUI_STACK_TRANSITION_TYPE_SLIDE_RIGHT ||
          transition_type == BOBGUI_STACK_TRANSITION_TYPE_OVER_RIGHT);
}

static inline gboolean
is_up_transition (BobguiStackTransitionType transition_type)
{
  return (transition_type == BOBGUI_STACK_TRANSITION_TYPE_SLIDE_UP ||
          transition_type == BOBGUI_STACK_TRANSITION_TYPE_OVER_UP);
}

static inline gboolean
is_down_transition (BobguiStackTransitionType transition_type)
{
  return (transition_type == BOBGUI_STACK_TRANSITION_TYPE_SLIDE_DOWN ||
          transition_type == BOBGUI_STACK_TRANSITION_TYPE_OVER_DOWN);
}

/* Transitions that cause the bin window to move */
static inline gboolean
is_window_moving_transition (BobguiStackTransitionType transition_type)
{
  return (transition_type == BOBGUI_STACK_TRANSITION_TYPE_SLIDE_LEFT ||
          transition_type == BOBGUI_STACK_TRANSITION_TYPE_SLIDE_RIGHT ||
          transition_type == BOBGUI_STACK_TRANSITION_TYPE_SLIDE_UP ||
          transition_type == BOBGUI_STACK_TRANSITION_TYPE_SLIDE_DOWN ||
          transition_type == BOBGUI_STACK_TRANSITION_TYPE_OVER_UP ||
          transition_type == BOBGUI_STACK_TRANSITION_TYPE_OVER_DOWN ||
          transition_type == BOBGUI_STACK_TRANSITION_TYPE_OVER_LEFT ||
          transition_type == BOBGUI_STACK_TRANSITION_TYPE_OVER_RIGHT);
}

/* Transitions that change direction depending on the relative order of the
old and new child */
static inline gboolean
is_direction_dependent_transition (BobguiStackTransitionType transition_type)
{
  return (transition_type == BOBGUI_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT ||
          transition_type == BOBGUI_STACK_TRANSITION_TYPE_SLIDE_UP_DOWN ||
          transition_type == BOBGUI_STACK_TRANSITION_TYPE_OVER_UP_DOWN ||
          transition_type == BOBGUI_STACK_TRANSITION_TYPE_OVER_DOWN_UP ||
          transition_type == BOBGUI_STACK_TRANSITION_TYPE_OVER_LEFT_RIGHT ||
          transition_type == BOBGUI_STACK_TRANSITION_TYPE_OVER_RIGHT_LEFT ||
          transition_type == BOBGUI_STACK_TRANSITION_TYPE_ROTATE_LEFT_RIGHT);
}

/* Returns simple transition type for a direction dependent transition, given
whether the new child (the one being switched to) is first in the stacking order
(added earlier). */
static inline BobguiStackTransitionType
get_simple_transition_type (gboolean               new_child_first,
                            BobguiStackTransitionType transition_type)
{
  switch (transition_type)
    {
    case BOBGUI_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT:
      return new_child_first ? BOBGUI_STACK_TRANSITION_TYPE_SLIDE_RIGHT : BOBGUI_STACK_TRANSITION_TYPE_SLIDE_LEFT;
    case BOBGUI_STACK_TRANSITION_TYPE_ROTATE_LEFT_RIGHT:
      return new_child_first ? BOBGUI_STACK_TRANSITION_TYPE_ROTATE_RIGHT : BOBGUI_STACK_TRANSITION_TYPE_ROTATE_LEFT;
    case BOBGUI_STACK_TRANSITION_TYPE_SLIDE_UP_DOWN:
      return new_child_first ? BOBGUI_STACK_TRANSITION_TYPE_SLIDE_DOWN : BOBGUI_STACK_TRANSITION_TYPE_SLIDE_UP;
    case BOBGUI_STACK_TRANSITION_TYPE_OVER_UP_DOWN:
      return new_child_first ? BOBGUI_STACK_TRANSITION_TYPE_UNDER_DOWN : BOBGUI_STACK_TRANSITION_TYPE_OVER_UP;
    case BOBGUI_STACK_TRANSITION_TYPE_OVER_DOWN_UP:
      return new_child_first ? BOBGUI_STACK_TRANSITION_TYPE_UNDER_UP : BOBGUI_STACK_TRANSITION_TYPE_OVER_DOWN;
    case BOBGUI_STACK_TRANSITION_TYPE_OVER_LEFT_RIGHT:
      return new_child_first ? BOBGUI_STACK_TRANSITION_TYPE_UNDER_RIGHT : BOBGUI_STACK_TRANSITION_TYPE_OVER_LEFT;
    case BOBGUI_STACK_TRANSITION_TYPE_OVER_RIGHT_LEFT:
      return new_child_first ? BOBGUI_STACK_TRANSITION_TYPE_UNDER_LEFT : BOBGUI_STACK_TRANSITION_TYPE_OVER_RIGHT;
    case BOBGUI_STACK_TRANSITION_TYPE_SLIDE_UP:
    case BOBGUI_STACK_TRANSITION_TYPE_SLIDE_DOWN:
    case BOBGUI_STACK_TRANSITION_TYPE_OVER_UP:
    case BOBGUI_STACK_TRANSITION_TYPE_OVER_DOWN:
    case BOBGUI_STACK_TRANSITION_TYPE_UNDER_UP:
    case BOBGUI_STACK_TRANSITION_TYPE_UNDER_DOWN:
    case BOBGUI_STACK_TRANSITION_TYPE_NONE:
    case BOBGUI_STACK_TRANSITION_TYPE_SLIDE_LEFT:
    case BOBGUI_STACK_TRANSITION_TYPE_SLIDE_RIGHT:
    case BOBGUI_STACK_TRANSITION_TYPE_OVER_LEFT:
    case BOBGUI_STACK_TRANSITION_TYPE_OVER_RIGHT:
    case BOBGUI_STACK_TRANSITION_TYPE_UNDER_LEFT:
    case BOBGUI_STACK_TRANSITION_TYPE_UNDER_RIGHT:
    case BOBGUI_STACK_TRANSITION_TYPE_CROSSFADE:
    case BOBGUI_STACK_TRANSITION_TYPE_ROTATE_LEFT:
    case BOBGUI_STACK_TRANSITION_TYPE_ROTATE_RIGHT:
    default:
      return transition_type;
    }
}

static int
get_bin_window_x (BobguiStack *stack)
{
  BobguiStackPrivate *priv = bobgui_stack_get_instance_private (stack);
  int width;
  int x = 0;

  width = bobgui_widget_get_width (BOBGUI_WIDGET (stack));

  if (bobgui_progress_tracker_get_state (&priv->tracker) != BOBGUI_PROGRESS_STATE_AFTER)
    {
      if (is_left_transition (priv->active_transition_type))
        x = width * (1 - bobgui_progress_tracker_get_ease_out_cubic (&priv->tracker, FALSE));
      if (is_right_transition (priv->active_transition_type))
        x = -width * (1 - bobgui_progress_tracker_get_ease_out_cubic (&priv->tracker, FALSE));
    }

  return x;
}

static int
get_bin_window_y (BobguiStack *stack)
{
  BobguiStackPrivate *priv = bobgui_stack_get_instance_private (stack);
  int height;
  int y = 0;

  height = bobgui_widget_get_height (BOBGUI_WIDGET (stack));

  if (bobgui_progress_tracker_get_state (&priv->tracker) != BOBGUI_PROGRESS_STATE_AFTER)
    {
      if (is_up_transition (priv->active_transition_type))
        y = height * (1 - bobgui_progress_tracker_get_ease_out_cubic (&priv->tracker, FALSE));
      if (is_down_transition(priv->active_transition_type))
        y = -height * (1 - bobgui_progress_tracker_get_ease_out_cubic (&priv->tracker, FALSE));
    }

  return y;
}

static void
bobgui_stack_progress_updated (BobguiStack *stack)
{
  BobguiStackPrivate *priv = bobgui_stack_get_instance_private (stack);

  if (!priv->homogeneous[BOBGUI_ORIENTATION_VERTICAL] || !priv->homogeneous[BOBGUI_ORIENTATION_HORIZONTAL])
    bobgui_widget_queue_resize (BOBGUI_WIDGET (stack));
  else if (is_window_moving_transition (priv->active_transition_type))
    bobgui_widget_queue_allocate (BOBGUI_WIDGET (stack));
  else
    bobgui_widget_queue_draw (BOBGUI_WIDGET (stack));

  if (bobgui_progress_tracker_get_state (&priv->tracker) == BOBGUI_PROGRESS_STATE_AFTER &&
      priv->last_visible_child != NULL)
    {
      bobgui_widget_set_child_visible (priv->last_visible_child->widget, FALSE);
      priv->last_visible_child = NULL;
    }
}

static gboolean
bobgui_stack_transition_cb (BobguiWidget     *widget,
                         GdkFrameClock *frame_clock,
                         gpointer       user_data)
{
  BobguiStack *stack = BOBGUI_STACK (widget);
  BobguiStackPrivate *priv = bobgui_stack_get_instance_private (stack);

  if (priv->first_frame_skipped)
    bobgui_progress_tracker_advance_frame (&priv->tracker,
                                        gdk_frame_clock_get_frame_time (frame_clock));
  else
    priv->first_frame_skipped = TRUE;

  /* Finish animation early if not mapped anymore */
  if (!bobgui_widget_get_mapped (widget))
    bobgui_progress_tracker_finish (&priv->tracker);

  bobgui_stack_progress_updated (BOBGUI_STACK (widget));

  if (bobgui_progress_tracker_get_state (&priv->tracker) == BOBGUI_PROGRESS_STATE_AFTER)
    {
      priv->tick_id = 0;
      g_object_notify_by_pspec (G_OBJECT (stack), stack_props[PROP_TRANSITION_RUNNING]);

      return FALSE;
    }

  return TRUE;
}

static void
bobgui_stack_schedule_ticks (BobguiStack *stack)
{
  BobguiStackPrivate *priv = bobgui_stack_get_instance_private (stack);

  if (priv->tick_id == 0)
    {
      priv->tick_id =
        bobgui_widget_add_tick_callback (BOBGUI_WIDGET (stack), bobgui_stack_transition_cb, stack, NULL);
      g_object_notify_by_pspec (G_OBJECT (stack), stack_props[PROP_TRANSITION_RUNNING]);
    }
}

static void
bobgui_stack_unschedule_ticks (BobguiStack *stack)
{
  BobguiStackPrivate *priv = bobgui_stack_get_instance_private (stack);

  if (priv->tick_id != 0)
    {
      bobgui_widget_remove_tick_callback (BOBGUI_WIDGET (stack), priv->tick_id);
      priv->tick_id = 0;
      g_object_notify_by_pspec (G_OBJECT (stack), stack_props[PROP_TRANSITION_RUNNING]);
      bobgui_widget_set_overflow (BOBGUI_WIDGET (stack), BOBGUI_OVERFLOW_VISIBLE);
    }
}

static BobguiStackTransitionType
effective_transition_type (BobguiStack               *stack,
                           BobguiStackTransitionType  transition_type)
{
  if (_bobgui_widget_get_direction (BOBGUI_WIDGET (stack)) == BOBGUI_TEXT_DIR_RTL)
    {
      switch (transition_type)
        {
        case BOBGUI_STACK_TRANSITION_TYPE_SLIDE_LEFT:
          return BOBGUI_STACK_TRANSITION_TYPE_SLIDE_RIGHT;
        case BOBGUI_STACK_TRANSITION_TYPE_SLIDE_RIGHT:
          return BOBGUI_STACK_TRANSITION_TYPE_SLIDE_LEFT;
        case BOBGUI_STACK_TRANSITION_TYPE_ROTATE_LEFT:
          return BOBGUI_STACK_TRANSITION_TYPE_ROTATE_RIGHT;
        case BOBGUI_STACK_TRANSITION_TYPE_ROTATE_RIGHT:
          return BOBGUI_STACK_TRANSITION_TYPE_ROTATE_LEFT;
        case BOBGUI_STACK_TRANSITION_TYPE_OVER_LEFT:
          return BOBGUI_STACK_TRANSITION_TYPE_OVER_RIGHT;
        case BOBGUI_STACK_TRANSITION_TYPE_OVER_RIGHT:
          return BOBGUI_STACK_TRANSITION_TYPE_OVER_LEFT;
        case BOBGUI_STACK_TRANSITION_TYPE_UNDER_LEFT:
          return BOBGUI_STACK_TRANSITION_TYPE_UNDER_RIGHT;
        case BOBGUI_STACK_TRANSITION_TYPE_UNDER_RIGHT:
          return BOBGUI_STACK_TRANSITION_TYPE_UNDER_LEFT;
        case BOBGUI_STACK_TRANSITION_TYPE_SLIDE_UP:
        case BOBGUI_STACK_TRANSITION_TYPE_SLIDE_DOWN:
        case BOBGUI_STACK_TRANSITION_TYPE_OVER_UP:
        case BOBGUI_STACK_TRANSITION_TYPE_OVER_DOWN:
        case BOBGUI_STACK_TRANSITION_TYPE_UNDER_UP:
        case BOBGUI_STACK_TRANSITION_TYPE_UNDER_DOWN:
        case BOBGUI_STACK_TRANSITION_TYPE_NONE:
        case BOBGUI_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT:
        case BOBGUI_STACK_TRANSITION_TYPE_SLIDE_UP_DOWN:
        case BOBGUI_STACK_TRANSITION_TYPE_OVER_UP_DOWN:
        case BOBGUI_STACK_TRANSITION_TYPE_OVER_DOWN_UP:
        case BOBGUI_STACK_TRANSITION_TYPE_OVER_LEFT_RIGHT:
        case BOBGUI_STACK_TRANSITION_TYPE_OVER_RIGHT_LEFT:
        case BOBGUI_STACK_TRANSITION_TYPE_CROSSFADE:
        case BOBGUI_STACK_TRANSITION_TYPE_ROTATE_LEFT_RIGHT:
        default:
          return transition_type;
        }
    }
  else
    {
      return transition_type;
    }
}

static void
bobgui_stack_start_transition (BobguiStack               *stack,
                            BobguiStackTransitionType  transition_type,
                            guint                   transition_duration)
{
  BobguiStackPrivate *priv = bobgui_stack_get_instance_private (stack);
  BobguiWidget *widget = BOBGUI_WIDGET (stack);

  if (bobgui_widget_get_mapped (widget) &&
      bobgui_settings_get_enable_animations (bobgui_widget_get_settings (widget)) &&
      transition_type != BOBGUI_STACK_TRANSITION_TYPE_NONE &&
      transition_duration != 0 &&
      priv->last_visible_child != NULL)
    {
      priv->active_transition_type = effective_transition_type (stack, transition_type);
      priv->first_frame_skipped = FALSE;
      bobgui_stack_schedule_ticks (stack);
      bobgui_progress_tracker_start (&priv->tracker,
                                  priv->transition_duration * 1000,
                                  0,
                                  1.0);
      /* We set overflow to hidden during transitions to avoid
       * input problems.
       */
      bobgui_widget_set_overflow (BOBGUI_WIDGET (stack), BOBGUI_OVERFLOW_HIDDEN);
    }
  else
    {
      bobgui_stack_unschedule_ticks (stack);
      priv->active_transition_type = BOBGUI_STACK_TRANSITION_TYPE_NONE;
      bobgui_progress_tracker_finish (&priv->tracker);
    }

  bobgui_stack_progress_updated (BOBGUI_STACK (widget));
}

static void
set_visible_child (BobguiStack               *stack,
                   BobguiStackPage      *child_info,
                   BobguiStackTransitionType  transition_type,
                   guint                   transition_duration)
{
  BobguiStackPrivate *priv = bobgui_stack_get_instance_private (stack);
  BobguiStackPage *info;
  BobguiWidget *widget = BOBGUI_WIDGET (stack);
  guint idx;
  BobguiWidget *focus;
  gboolean contains_focus = FALSE;
  guint old_pos = BOBGUI_INVALID_LIST_POSITION;
  guint new_pos = BOBGUI_INVALID_LIST_POSITION;

  /* if we are being destroyed, do not bother with transitions
   * and notifications
   */
  if (bobgui_widget_in_destruction (widget))
    return;

  /* If none, pick first visible */
  if (child_info == NULL)
    {
      for (idx = 0; idx < priv->children->len; idx++)
        {
          info = g_ptr_array_index (priv->children, idx);
          if (bobgui_widget_get_visible (info->widget))
            {
              child_info = info;
              break;
            }
        }
    }

  if (child_info == priv->visible_child)
    return;

  if (priv->pages)
    {
      guint position;
      for (idx = 0, position = 0; idx < priv->children->len; idx++, position++)
        {
          info = g_ptr_array_index (priv->children, idx);
          if (info == priv->visible_child)
            old_pos = position;
          else if (info == child_info)
            new_pos = position;
        }
    }

  if (bobgui_widget_get_root (widget))
    focus = bobgui_root_get_focus (bobgui_widget_get_root (widget));
  else
    focus = NULL;
  if (focus &&
      priv->visible_child &&
      priv->visible_child->widget &&
      bobgui_widget_is_ancestor (focus, priv->visible_child->widget))
    {
      contains_focus = TRUE;

      if (priv->visible_child->last_focus)
        g_object_remove_weak_pointer (G_OBJECT (priv->visible_child->last_focus),
                                      (gpointer *)&priv->visible_child->last_focus);
      priv->visible_child->last_focus = focus;
      g_object_add_weak_pointer (G_OBJECT (priv->visible_child->last_focus),
                                 (gpointer *)&priv->visible_child->last_focus);
    }

  if (priv->last_visible_child)
    {
      bobgui_widget_set_child_visible (priv->last_visible_child->widget, FALSE);
      bobgui_accessible_update_state (BOBGUI_ACCESSIBLE (priv->last_visible_child),
                                   BOBGUI_ACCESSIBLE_STATE_HIDDEN, TRUE,
                                   -1);
    }
      priv->last_visible_child = NULL;

  if (priv->visible_child && priv->visible_child->widget)
    {
      if (bobgui_widget_is_visible (widget))
        {
          priv->last_visible_child = priv->visible_child;
          priv->last_visible_widget_width = bobgui_widget_get_width (widget);
          priv->last_visible_widget_height = bobgui_widget_get_height (widget);
        }
      else
        {
          bobgui_widget_set_child_visible (priv->visible_child->widget, FALSE);
        }
      bobgui_accessible_update_state (BOBGUI_ACCESSIBLE (priv->visible_child),
                                   BOBGUI_ACCESSIBLE_STATE_HIDDEN, TRUE,
                                   -1);
    }

  priv->visible_child = child_info;

  if (child_info)
    {
      bobgui_widget_set_child_visible (child_info->widget, TRUE);
      bobgui_accessible_update_state (BOBGUI_ACCESSIBLE (child_info),
                                   BOBGUI_ACCESSIBLE_STATE_HIDDEN, FALSE,
                                   -1);

      if (contains_focus)
        {
          if (child_info->last_focus)
            bobgui_widget_grab_focus (child_info->last_focus);
          else
            bobgui_widget_child_focus (child_info->widget, BOBGUI_DIR_TAB_FORWARD);
        }
    }

  if ((child_info == NULL || priv->last_visible_child == NULL) &&
      is_direction_dependent_transition (transition_type))
    {
      transition_type = BOBGUI_STACK_TRANSITION_TYPE_NONE;
    }
  else if (is_direction_dependent_transition (transition_type))
    {
      gboolean i_first = FALSE;
      for (idx = 0; idx < priv->children->len; idx++)
        {
	  if (child_info == g_ptr_array_index (priv->children, idx))
	    {
	      i_first = TRUE;
	      break;
	    }
	  if (priv->last_visible_child == g_ptr_array_index (priv->children, idx))
	    break;
        }

      transition_type = get_simple_transition_type (i_first, transition_type);
    }

  if (priv->homogeneous[BOBGUI_ORIENTATION_HORIZONTAL] && priv->homogeneous[BOBGUI_ORIENTATION_VERTICAL])
    bobgui_widget_queue_allocate (widget);
  else
    bobgui_widget_queue_resize (widget);

  g_object_notify_by_pspec (G_OBJECT (stack), stack_props[PROP_VISIBLE_CHILD]);
  g_object_notify_by_pspec (G_OBJECT (stack),
                            stack_props[PROP_VISIBLE_CHILD_NAME]);

  if (priv->pages)
    {
      if (old_pos == BOBGUI_INVALID_LIST_POSITION && new_pos == BOBGUI_INVALID_LIST_POSITION)
        ; /* nothing to do */
      else if (old_pos == BOBGUI_INVALID_LIST_POSITION)
        bobgui_selection_model_selection_changed (priv->pages, new_pos, 1);
      else if (new_pos == BOBGUI_INVALID_LIST_POSITION)
        bobgui_selection_model_selection_changed (priv->pages, old_pos, 1);
      else
        bobgui_selection_model_selection_changed (priv->pages,
                                               MIN (old_pos, new_pos),
                                               MAX (old_pos, new_pos) - MIN (old_pos, new_pos) + 1);
    }

  bobgui_stack_start_transition (stack, transition_type, transition_duration);
}

static void
update_child_visible (BobguiStack     *stack,
                      BobguiStackPage *child_info)
{
  BobguiStackPrivate *priv = bobgui_stack_get_instance_private (stack);
  gboolean visible;

  visible = child_info->visible && bobgui_widget_get_visible (child_info->widget);

  if (priv->visible_child == NULL && visible)
    set_visible_child (stack, child_info, priv->transition_type, priv->transition_duration);
  else if (priv->visible_child == child_info && !visible)
    set_visible_child (stack, NULL, priv->transition_type, priv->transition_duration);

  if (child_info == priv->last_visible_child)
    {
      bobgui_widget_set_child_visible (priv->last_visible_child->widget, FALSE);
      priv->last_visible_child = NULL;
    }

  bobgui_accessible_update_state (BOBGUI_ACCESSIBLE (child_info),
                               BOBGUI_ACCESSIBLE_STATE_HIDDEN, !visible,
                               -1);
}

static void
stack_child_visibility_notify_cb (GObject    *obj,
                                  GParamSpec *pspec,
                                  gpointer    user_data)
{
  BobguiStack *stack = BOBGUI_STACK (user_data);
  BobguiStackPage *child_info;

  child_info = find_child_info_for_widget (stack, BOBGUI_WIDGET (obj));
  g_return_if_fail (child_info != NULL);

  update_child_visible (stack, child_info);
}

/**
 * bobgui_stack_add_titled:
 * @stack: a `BobguiStack`
 * @child: the widget to add
 * @name: (nullable): the name for @child
 * @title: a human-readable title for @child
 *
 * Adds a child to @stack.
 *
 * The child is identified by the @name. The @title
 * will be used by `BobguiStackSwitcher` to represent
 * @child in a tab bar, so it should be short.
 *
 * Returns: (transfer none): the `BobguiStackPage` for @child
 */
BobguiStackPage *
bobgui_stack_add_titled (BobguiStack   *stack,
                      BobguiWidget  *child,
                      const char *name,
                      const char *title)
{
  g_return_val_if_fail (BOBGUI_IS_STACK (stack), NULL);
  g_return_val_if_fail (BOBGUI_IS_WIDGET (child), NULL);

  return bobgui_stack_add_internal (stack, child, name, title);
}

/**
 * bobgui_stack_add_child:
 * @stack: a `BobguiStack`
 * @child: the widget to add
 *
 * Adds a child to @stack.
 *
 * Returns: (transfer none): the `BobguiStackPage` for @child
 */
BobguiStackPage *
bobgui_stack_add_child (BobguiStack   *stack,
                     BobguiWidget  *child)
{
  g_return_val_if_fail (BOBGUI_IS_STACK (stack), NULL);
  g_return_val_if_fail (BOBGUI_IS_WIDGET (child), NULL);

  return bobgui_stack_add_internal (stack, child, NULL, NULL);
}

/**
 * bobgui_stack_add_named:
 * @stack: a `BobguiStack`
 * @child: the widget to add
 * @name: (nullable): the name for @child
 *
 * Adds a child to @stack.
 *
 * The child is identified by the @name.
 *
 * Returns: (transfer none): the `BobguiStackPage` for @child
 */
BobguiStackPage *
bobgui_stack_add_named (BobguiStack   *stack,
                     BobguiWidget  *child,
                     const char *name)
{
  g_return_val_if_fail (BOBGUI_IS_STACK (stack), NULL);
  g_return_val_if_fail (BOBGUI_IS_WIDGET (child), NULL);

  return bobgui_stack_add_internal (stack, child, name, NULL);
}

static BobguiStackPage *
bobgui_stack_add_internal (BobguiStack   *stack,
                        BobguiWidget  *child,
                        const char *name,
                        const char *title)
{
  BobguiStackPage *child_info;

  g_return_val_if_fail (child != NULL, NULL);

  child_info = g_object_new (BOBGUI_TYPE_STACK_PAGE,
                             "child", child,
                             "name", name,
                             "title", title,
                             NULL);

  bobgui_stack_add_page (stack, child_info);

  g_object_unref (child_info);

  return child_info;
}

static void
bobgui_stack_add_page (BobguiStack     *stack,
                    BobguiStackPage *child_info)
{
  BobguiStackPrivate *priv = bobgui_stack_get_instance_private (stack);
  guint idx;

  g_return_if_fail (child_info->widget != NULL);

  if (child_info->name)
    {
      for (idx = 0; idx < priv->children->len; idx++)
        {
          BobguiStackPage *info = g_ptr_array_index (priv->children, idx);
          if (info->name &&
              g_strcmp0 (info->name, child_info->name) == 0)
            {
              g_warning ("While adding page: duplicate child name in BobguiStack: %s", child_info->name);
              break;
            }
        }
    }


  if (priv->children->len > 0)
    {
      BobguiStackPage *prev_last = g_ptr_array_index (priv->children, priv->children->len - 1);

      prev_last->next_page = child_info;
    }
  else
    {
      child_info->next_page = NULL;
    }

  g_ptr_array_add (priv->children, g_object_ref (child_info));

  bobgui_widget_set_child_visible (child_info->widget, FALSE);
  bobgui_widget_set_parent (child_info->widget, BOBGUI_WIDGET (stack));

  if (priv->pages)
    {
      g_list_model_items_changed (G_LIST_MODEL (priv->pages), priv->children->len - 1, 0, 1);
      g_object_notify_by_pspec (G_OBJECT (priv->pages), pages_properties[PAGES_PROP_N_ITEMS]);
    }

  g_signal_connect (child_info->widget, "notify::visible",
                    G_CALLBACK (stack_child_visibility_notify_cb), stack);

  if (priv->visible_child == NULL &&
      bobgui_widget_get_visible (child_info->widget))
    set_visible_child (stack, child_info, priv->transition_type, priv->transition_duration);

  if (priv->homogeneous[BOBGUI_ORIENTATION_HORIZONTAL] || priv->homogeneous[BOBGUI_ORIENTATION_VERTICAL] || priv->visible_child == child_info)
    bobgui_widget_queue_resize (BOBGUI_WIDGET (stack));
}

static void
stack_remove (BobguiStack  *stack,
              BobguiWidget *child,
              gboolean   in_dispose)
{
  BobguiStackPrivate *priv = bobgui_stack_get_instance_private (stack);
  BobguiStackPage *child_info;
  gboolean was_visible;

  child_info = find_child_info_for_widget (stack, child);
  if (child_info == NULL)
    return;

  g_signal_handlers_disconnect_by_func (child,
                                        stack_child_visibility_notify_cb,
                                        stack);

  was_visible = bobgui_widget_get_visible (child);

  if (priv->visible_child == child_info)
    priv->visible_child = NULL;

  if (priv->last_visible_child == child_info)
    priv->last_visible_child = NULL;

  bobgui_widget_unparent (child);

  g_clear_object (&child_info->widget);

    g_ptr_array_remove (priv->children, child_info);

  for (guint prev_idx = 0; prev_idx < priv->children->len; prev_idx++)
    {
      BobguiStackPage *prev_page = g_ptr_array_index (priv->children, prev_idx);
      if (prev_page->next_page == child_info)
        {
          prev_page->next_page = child_info->next_page;
          break;
        }
    }

  g_object_unref (child_info);

  if (!in_dispose &&
      (priv->homogeneous[BOBGUI_ORIENTATION_HORIZONTAL] || priv->homogeneous[BOBGUI_ORIENTATION_VERTICAL]) &&
      was_visible)
    bobgui_widget_queue_resize (BOBGUI_WIDGET (stack));
}

/**
 * bobgui_stack_remove:
 * @stack: a `BobguiStack`
 * @child: the child to remove
 *
 * Removes a child widget from @stack.
 */
void
bobgui_stack_remove (BobguiStack  *stack,
                  BobguiWidget *child)
{
  BobguiStackPrivate *priv = bobgui_stack_get_instance_private (stack);
  guint position;

  g_return_if_fail (BOBGUI_IS_STACK (stack));
  g_return_if_fail (BOBGUI_IS_WIDGET (child));
  g_return_if_fail (bobgui_widget_get_parent (child) == BOBGUI_WIDGET (stack));

  for (position = 0; position < priv->children->len; position++)
    {
      BobguiStackPage *page = g_ptr_array_index (priv->children, position);
      if (page->widget == child)
        break;
    }

  stack_remove (stack, child, FALSE);

  if (priv->pages)
    {
      g_list_model_items_changed (G_LIST_MODEL (priv->pages), position, 1, 0);
      g_object_notify_by_pspec (G_OBJECT (priv->pages), pages_properties[PAGES_PROP_N_ITEMS]);
    }
}

/**
 * bobgui_stack_get_page:
 * @stack: a `BobguiStack`
 * @child: a child of @stack
 *
 * Returns the `BobguiStackPage` object for @child.
 *
 * Returns: (transfer none): the `BobguiStackPage` for @child
 */
BobguiStackPage *
bobgui_stack_get_page (BobguiStack  *stack,
                    BobguiWidget *child)
{
  return find_child_info_for_widget (stack, child);
}

/**
 * bobgui_stack_get_child_by_name:
 * @stack: a `BobguiStack`
 * @name: the name of the child to find
 *
 * Finds the child with the name given as the argument.
 *
 * Returns %NULL if there is no child with this name.
 *
 * Returns: (transfer none) (nullable): the requested child
 *   of the `BobguiStack`
 */
BobguiWidget *
bobgui_stack_get_child_by_name (BobguiStack    *stack,
                             const char *name)
{
  BobguiStackPrivate *priv = bobgui_stack_get_instance_private (stack);
  BobguiStackPage *info;
  guint idx;

  g_return_val_if_fail (BOBGUI_IS_STACK (stack), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  for (idx = 0; idx < priv->children->len; idx++)
    {
      info = g_ptr_array_index (priv->children, idx);
      if (info->name && strcmp (info->name, name) == 0)
        return info->widget;
    }

  return NULL;
}

/**
 * bobgui_stack_page_get_child:
 * @self: a `BobguiStackPage`
 *
 * Returns the stack child to which @self belongs.
 *
 * Returns: (transfer none): the child to which @self belongs
 */
BobguiWidget *
bobgui_stack_page_get_child (BobguiStackPage *self)
{
  return self->widget;
}

/**
 * bobgui_stack_set_hhomogeneous:
 * @stack: a `BobguiStack`
 * @hhomogeneous: %TRUE to make @stack horizontally homogeneous
 *
 * Sets the `BobguiStack` to be horizontally homogeneous or not.
 *
 * If it is homogeneous, the `BobguiStack` will request the same
 * width for all its children. If it isn't, the stack
 * may change width when a different child becomes visible.
 */
void
bobgui_stack_set_hhomogeneous (BobguiStack *stack,
                            gboolean  hhomogeneous)
{
  BobguiStackPrivate *priv = bobgui_stack_get_instance_private (stack);

  g_return_if_fail (BOBGUI_IS_STACK (stack));

  hhomogeneous = !!hhomogeneous;

  if (priv->homogeneous[BOBGUI_ORIENTATION_HORIZONTAL] == hhomogeneous)
    return;

  priv->homogeneous[BOBGUI_ORIENTATION_HORIZONTAL] = hhomogeneous;

  if (bobgui_widget_get_visible (BOBGUI_WIDGET(stack)))
    bobgui_widget_queue_resize (BOBGUI_WIDGET (stack));

  g_object_notify_by_pspec (G_OBJECT (stack), stack_props[PROP_HHOMOGENEOUS]);
}

/**
 * bobgui_stack_get_hhomogeneous:
 * @stack: a `BobguiStack`
 *
 * Gets whether @stack is horizontally homogeneous.
 *
 * Returns: whether @stack is horizontally homogeneous.
 */
gboolean
bobgui_stack_get_hhomogeneous (BobguiStack *stack)
{
  BobguiStackPrivate *priv = bobgui_stack_get_instance_private (stack);

  g_return_val_if_fail (BOBGUI_IS_STACK (stack), FALSE);

  return priv->homogeneous[BOBGUI_ORIENTATION_HORIZONTAL];
}

/**
 * bobgui_stack_set_vhomogeneous:
 * @stack: a `BobguiStack`
 * @vhomogeneous: %TRUE to make @stack vertically homogeneous
 *
 * Sets the `BobguiStack` to be vertically homogeneous or not.
 *
 * If it is homogeneous, the `BobguiStack` will request the same
 * height for all its children. If it isn't, the stack
 * may change height when a different child becomes visible.
 */
void
bobgui_stack_set_vhomogeneous (BobguiStack *stack,
                            gboolean  vhomogeneous)
{
  BobguiStackPrivate *priv = bobgui_stack_get_instance_private (stack);

  g_return_if_fail (BOBGUI_IS_STACK (stack));

  vhomogeneous = !!vhomogeneous;

  if (priv->homogeneous[BOBGUI_ORIENTATION_VERTICAL] == vhomogeneous)
    return;

  priv->homogeneous[BOBGUI_ORIENTATION_VERTICAL] = vhomogeneous;

  if (bobgui_widget_get_visible (BOBGUI_WIDGET(stack)))
    bobgui_widget_queue_resize (BOBGUI_WIDGET (stack));

  g_object_notify_by_pspec (G_OBJECT (stack), stack_props[PROP_VHOMOGENEOUS]);
}

/**
 * bobgui_stack_get_vhomogeneous:
 * @stack: a `BobguiStack`
 *
 * Gets whether @stack is vertically homogeneous.
 *
 * Returns: whether @stack is vertically homogeneous.
 */
gboolean
bobgui_stack_get_vhomogeneous (BobguiStack *stack)
{
  BobguiStackPrivate *priv = bobgui_stack_get_instance_private (stack);

  g_return_val_if_fail (BOBGUI_IS_STACK (stack), FALSE);

  return priv->homogeneous[BOBGUI_ORIENTATION_VERTICAL];
}

/**
 * bobgui_stack_get_transition_duration:
 * @stack: a `BobguiStack`
 *
 * Returns the amount of time (in milliseconds) that
 * transitions between pages in @stack will take.
 *
 * Returns: the transition duration
 */
guint
bobgui_stack_get_transition_duration (BobguiStack *stack)
{
  BobguiStackPrivate *priv = bobgui_stack_get_instance_private (stack);

  g_return_val_if_fail (BOBGUI_IS_STACK (stack), 0);

  return priv->transition_duration;
}

/**
 * bobgui_stack_set_transition_duration:
 * @stack: a `BobguiStack`
 * @duration: the new duration, in milliseconds
 *
 * Sets the duration that transitions between pages in @stack
 * will take.
 */
void
bobgui_stack_set_transition_duration (BobguiStack *stack,
                                   guint     duration)
{
  BobguiStackPrivate *priv = bobgui_stack_get_instance_private (stack);

  g_return_if_fail (BOBGUI_IS_STACK (stack));

  if (priv->transition_duration == duration)
    return;

  priv->transition_duration = duration;
  g_object_notify_by_pspec (G_OBJECT (stack),
                            stack_props[PROP_TRANSITION_DURATION]);
}

/**
 * bobgui_stack_get_transition_type:
 * @stack: a `BobguiStack`
 *
 * Gets the type of animation that will be used
 * for transitions between pages in @stack.
 *
 * Returns: the current transition type of @stack
 */
BobguiStackTransitionType
bobgui_stack_get_transition_type (BobguiStack *stack)
{
  BobguiStackPrivate *priv = bobgui_stack_get_instance_private (stack);

  g_return_val_if_fail (BOBGUI_IS_STACK (stack), BOBGUI_STACK_TRANSITION_TYPE_NONE);

  return priv->transition_type;
}

/**
 * bobgui_stack_set_transition_type:
 * @stack: a `BobguiStack`
 * @transition: the new transition type
 *
 * Sets the type of animation that will be used for
 * transitions between pages in @stack.
 *
 * Available types include various kinds of fades and slides.
 *
 * The transition type can be changed without problems
 * at runtime, so it is possible to change the animation
 * based on the page that is about to become current.
 */
void
bobgui_stack_set_transition_type (BobguiStack              *stack,
                              BobguiStackTransitionType  transition)
{
  BobguiStackPrivate *priv = bobgui_stack_get_instance_private (stack);

  g_return_if_fail (BOBGUI_IS_STACK (stack));

  if (priv->transition_type == transition)
    return;

  priv->transition_type = transition;
  g_object_notify_by_pspec (G_OBJECT (stack),
                            stack_props[PROP_TRANSITION_TYPE]);
}

/**
 * bobgui_stack_get_transition_running:
 * @stack: a `BobguiStack`
 *
 * Returns whether the @stack is currently in a transition from one page to
 * another.
 *
 * Returns: %TRUE if the transition is currently running, %FALSE otherwise.
 */
gboolean
bobgui_stack_get_transition_running (BobguiStack *stack)
{
  BobguiStackPrivate *priv = bobgui_stack_get_instance_private (stack);

  g_return_val_if_fail (BOBGUI_IS_STACK (stack), FALSE);

  return (priv->tick_id != 0);
}

/**
 * bobgui_stack_set_interpolate_size:
 * @stack: A `BobguiStack`
 * @interpolate_size: the new value
 *
 * Sets whether or not @stack will interpolate its size when
 * changing the visible child.
 *
 * If the [property@Bobgui.Stack:interpolate-size] property is set
 * to %TRUE, @stack will interpolate its size between the current
 * one and the one it'll take after changing the visible child,
 * according to the set transition duration.
 */
void
bobgui_stack_set_interpolate_size (BobguiStack *stack,
                                gboolean  interpolate_size)
{
  BobguiStackPrivate *priv = bobgui_stack_get_instance_private (stack);
  g_return_if_fail (BOBGUI_IS_STACK (stack));

  interpolate_size = !!interpolate_size;

  if (priv->interpolate_size == interpolate_size)
    return;

  priv->interpolate_size = interpolate_size;
  g_object_notify_by_pspec (G_OBJECT (stack),
                            stack_props[PROP_INTERPOLATE_SIZE]);
}

/**
 * bobgui_stack_get_interpolate_size:
 * @stack: A `BobguiStack`
 *
 * Returns whether the `BobguiStack` is set up to interpolate between
 * the sizes of children on page switch.
 *
 * Returns: %TRUE if child sizes are interpolated
 */
gboolean
bobgui_stack_get_interpolate_size (BobguiStack *stack)
{
  BobguiStackPrivate *priv = bobgui_stack_get_instance_private (stack);
  g_return_val_if_fail (BOBGUI_IS_STACK (stack), FALSE);

  return priv->interpolate_size;
}



/**
 * bobgui_stack_get_visible_child:
 * @stack: a `BobguiStack`
 *
 * Gets the currently visible child of @stack.
 *
 * Returns %NULL if there are no visible children.
 *
 * Returns: (transfer none) (nullable): the visible child of the `BobguiStack`
 */
BobguiWidget *
bobgui_stack_get_visible_child (BobguiStack *stack)
{
  BobguiStackPrivate *priv = bobgui_stack_get_instance_private (stack);

  g_return_val_if_fail (BOBGUI_IS_STACK (stack), NULL);

  return priv->visible_child ? priv->visible_child->widget : NULL;
}

/**
 * bobgui_stack_get_visible_child_name:
 * @stack: a `BobguiStack`
 *
 * Returns the name of the currently visible child of @stack.
 *
 * Returns %NULL if there is no visible child.
 *
 * Returns: (transfer none) (nullable): the name of the visible child
 *   of the `BobguiStack`
 */
const char *
bobgui_stack_get_visible_child_name (BobguiStack *stack)
{
  BobguiStackPrivate *priv = bobgui_stack_get_instance_private (stack);

  g_return_val_if_fail (BOBGUI_IS_STACK (stack), NULL);

  if (priv->visible_child)
    return priv->visible_child->name;

  return NULL;
}

/**
 * bobgui_stack_set_visible_child:
 * @stack: a `BobguiStack`
 * @child: a child of @stack
 *
 * Makes @child the visible child of @stack.
 *
 * If @child is different from the currently visible child,
 * the transition between the two will be animated with the
 * current transition type of @stack.
 *
 * Note that the @child widget has to be visible itself
 * (see [method@Bobgui.Widget.show]) in order to become the visible
 * child of @stack.
 */
void
bobgui_stack_set_visible_child (BobguiStack  *stack,
                             BobguiWidget *child)
{
  BobguiStackPrivate *priv = bobgui_stack_get_instance_private (stack);
  BobguiStackPage *child_info;

  g_return_if_fail (BOBGUI_IS_STACK (stack));
  g_return_if_fail (BOBGUI_IS_WIDGET (child));

  child_info = find_child_info_for_widget (stack, child);
  if (child_info == NULL)
    {
      g_warning ("Given child of type '%s' not found in BobguiStack",
                 G_OBJECT_TYPE_NAME (child));
      return;
    }

  if (bobgui_widget_get_visible (child_info->widget))
    set_visible_child (stack, child_info,
                       priv->transition_type,
                       priv->transition_duration);
}

/**
 * bobgui_stack_set_visible_child_name:
 * @stack: a `BobguiStack`
 * @name: the name of the child to make visible
 *
 * Makes the child with the given name visible.
 *
 * If @child is different from the currently visible child,
 * the transition between the two will be animated with the
 * current transition type of @stack.
 *
 * Note that the child widget has to be visible itself
 * (see [method@Bobgui.Widget.show]) in order to become the visible
 * child of @stack.
 */
void
bobgui_stack_set_visible_child_name (BobguiStack   *stack,
                                 const char *name)
{
  BobguiStackPrivate *priv = bobgui_stack_get_instance_private (stack);

  g_return_if_fail (BOBGUI_IS_STACK (stack));

  bobgui_stack_set_visible_child_full (stack, name, priv->transition_type);
}

/**
 * bobgui_stack_set_visible_child_full:
 * @stack: a `BobguiStack`
 * @name: the name of the child to make visible
 * @transition: the transition type to use
 *
 * Makes the child with the given name visible.
 *
 * Note that the child widget has to be visible itself
 * (see [method@Bobgui.Widget.show]) in order to become the visible
 * child of @stack.
 */
void
bobgui_stack_set_visible_child_full (BobguiStack               *stack,
                                  const char             *name,
                                  BobguiStackTransitionType  transition)
{
  BobguiStackPrivate *priv = bobgui_stack_get_instance_private (stack);
  BobguiStackPage *child_info, *info;
  guint idx;

  g_return_if_fail (BOBGUI_IS_STACK (stack));

  if (name == NULL)
    return;

  child_info = NULL;
  for (idx = 0; idx < priv->children->len; idx++)
    {
      info = g_ptr_array_index (priv->children, idx);
      if (info->name != NULL &&
          strcmp (info->name, name) == 0)
        {
          child_info = info;
          break;
        }
    }

  if (child_info == NULL)
    {
      g_warning ("Child name '%s' not found in BobguiStack", name);
      return;
    }

  if (bobgui_widget_get_visible (child_info->widget))
    set_visible_child (stack, child_info, transition, priv->transition_duration);
}

static void
bobgui_stack_compute_expand (BobguiWidget *widget,
                          gboolean  *hexpand_p,
                          gboolean  *vexpand_p)
{
  BobguiStack *stack = BOBGUI_STACK (widget);
  BobguiStackPrivate *priv = bobgui_stack_get_instance_private (stack);
  gboolean hexpand, vexpand;
  BobguiStackPage *child_info;
  BobguiWidget *child;
  guint idx;

  hexpand = FALSE;
  vexpand = FALSE;
  for (idx = 0; idx < priv->children->len; idx++)
    {
      child_info = g_ptr_array_index (priv->children, idx);
      child = child_info->widget;

      if (!hexpand &&
          bobgui_widget_compute_expand (child, BOBGUI_ORIENTATION_HORIZONTAL))
        hexpand = TRUE;

      if (!vexpand &&
          bobgui_widget_compute_expand (child, BOBGUI_ORIENTATION_VERTICAL))
        vexpand = TRUE;

      if (hexpand && vexpand)
        break;
    }

  *hexpand_p = hexpand;
  *vexpand_p = vexpand;
}

static BobguiSizeRequestMode
bobgui_stack_get_request_mode (BobguiWidget *widget)
{
  BobguiStack *stack = BOBGUI_STACK (widget);
  BobguiStackPrivate *priv = bobgui_stack_get_instance_private (stack);
  BobguiWidget *w;
  int wfh = 0, hfw = 0;

  if (!priv->homogeneous[BOBGUI_ORIENTATION_VERTICAL] &&
      !priv->homogeneous[BOBGUI_ORIENTATION_HORIZONTAL])
    {
      BobguiSizeRequestMode lv_mode;

      /* Only the visible child, and perhaps the last visible child
       * during a transition, matter.  Attempt to return constant-size
       * when we can.  */
      if (priv->last_visible_child)
        lv_mode = bobgui_widget_get_request_mode (priv->last_visible_child->widget);
      else
        lv_mode = BOBGUI_SIZE_REQUEST_CONSTANT_SIZE;

      if (lv_mode == BOBGUI_SIZE_REQUEST_CONSTANT_SIZE && priv->visible_child)
        return bobgui_widget_get_request_mode (priv->visible_child->widget);
      else
        return lv_mode;
    }

  for (w = bobgui_widget_get_first_child (widget);
       w != NULL;
       w = bobgui_widget_get_next_sibling (w))
    {
      BobguiSizeRequestMode mode = bobgui_widget_get_request_mode (w);

      switch (mode)
        {
        case BOBGUI_SIZE_REQUEST_HEIGHT_FOR_WIDTH:
          hfw ++;
          break;
        case BOBGUI_SIZE_REQUEST_WIDTH_FOR_HEIGHT:
          wfh ++;
          break;
        case BOBGUI_SIZE_REQUEST_CONSTANT_SIZE:
        default:
          break;
        }
    }

  if (hfw == 0 && wfh == 0)
    return BOBGUI_SIZE_REQUEST_CONSTANT_SIZE;
  else
    return wfh > hfw ?
        BOBGUI_SIZE_REQUEST_WIDTH_FOR_HEIGHT :
        BOBGUI_SIZE_REQUEST_HEIGHT_FOR_WIDTH;
}

static void
bobgui_stack_snapshot_crossfade (BobguiWidget   *widget,
                              BobguiSnapshot *snapshot)
{
  BobguiStack *stack = BOBGUI_STACK (widget);
  BobguiStackPrivate *priv = bobgui_stack_get_instance_private (stack);
  double progress = bobgui_progress_tracker_get_progress (&priv->tracker, FALSE);

  bobgui_snapshot_push_cross_fade (snapshot, progress);

  if (priv->last_visible_child)
    {
      bobgui_widget_snapshot_child (widget,
                                 priv->last_visible_child->widget,
                                 snapshot);
    }
  bobgui_snapshot_pop (snapshot);

  bobgui_widget_snapshot_child (widget,
                             priv->visible_child->widget,
                             snapshot);
  bobgui_snapshot_pop (snapshot);
}

static void
bobgui_stack_snapshot_under (BobguiWidget   *widget,
                          BobguiSnapshot *snapshot)
{
  BobguiStack *stack = BOBGUI_STACK (widget);
  BobguiStackPrivate *priv = bobgui_stack_get_instance_private (stack);
  int widget_width, widget_height;
  int x, y, width, height, pos_x, pos_y;


  x = y = 0;
  width = widget_width = bobgui_widget_get_width (widget);
  height = widget_height = bobgui_widget_get_height (widget);

  pos_x = pos_y = 0;

  switch ((guint) priv->active_transition_type)
    {
    case BOBGUI_STACK_TRANSITION_TYPE_UNDER_DOWN:
      y = 0;
      height = widget_height * (bobgui_progress_tracker_get_ease_out_cubic (&priv->tracker, FALSE));
      pos_y = height;
      break;
    case BOBGUI_STACK_TRANSITION_TYPE_UNDER_UP:
      y = widget_height * (1 - bobgui_progress_tracker_get_ease_out_cubic (&priv->tracker, FALSE));
      height = widget_height - y;
      pos_y = y - widget_height;
      break;
    case BOBGUI_STACK_TRANSITION_TYPE_UNDER_LEFT:
      x = widget_width * (1 - bobgui_progress_tracker_get_ease_out_cubic (&priv->tracker, FALSE));
      width = widget_width - x;
      pos_x = x - widget_width;
      break;
    case BOBGUI_STACK_TRANSITION_TYPE_UNDER_RIGHT:
      x = 0;
      width = widget_width * (bobgui_progress_tracker_get_ease_out_cubic (&priv->tracker, FALSE));
      pos_x = width;
      break;
    default:
      g_assert_not_reached ();
    }

  bobgui_snapshot_push_clip (snapshot, &GRAPHENE_RECT_INIT(x, y, width, height));

  bobgui_widget_snapshot_child (widget,
                             priv->visible_child->widget,
                             snapshot);

  bobgui_snapshot_pop (snapshot);

  if (priv->last_visible_child)
    {
      bobgui_snapshot_save (snapshot);
      bobgui_snapshot_translate (snapshot, &GRAPHENE_POINT_INIT (pos_x, pos_y));
      bobgui_widget_snapshot_child (widget, priv->last_visible_child->widget, snapshot);
      bobgui_snapshot_restore (snapshot);
    }
}

static void
bobgui_stack_snapshot_cube (BobguiWidget   *widget,
                         BobguiSnapshot *snapshot)
{
  BobguiStack *stack = BOBGUI_STACK (widget);
  BobguiStackPrivate *priv = bobgui_stack_get_instance_private (stack);
  double progress = bobgui_progress_tracker_get_progress (&priv->tracker, FALSE);

  g_assert (priv->active_transition_type == BOBGUI_STACK_TRANSITION_TYPE_ROTATE_RIGHT ||
            priv->active_transition_type == BOBGUI_STACK_TRANSITION_TYPE_ROTATE_LEFT);

  if (priv->active_transition_type == BOBGUI_STACK_TRANSITION_TYPE_ROTATE_RIGHT)
    progress = 1 - progress;

  if (priv->last_visible_child && progress > 0.5)
    {
      bobgui_snapshot_save (snapshot);
      bobgui_snapshot_translate_3d (snapshot, &GRAPHENE_POINT3D_INIT (
                                 bobgui_widget_get_width (widget) / 2.f,
                                 bobgui_widget_get_height (widget) / 2.f,
                                 0));
      bobgui_snapshot_perspective (snapshot, 2 * bobgui_widget_get_width (widget) / 1.f);
      bobgui_snapshot_translate_3d (snapshot, &GRAPHENE_POINT3D_INIT (
                                 0, 0,
                                 - bobgui_widget_get_width (widget) / 2.f));
      bobgui_snapshot_rotate_3d (snapshot, -90 * progress, graphene_vec3_y_axis());
      bobgui_snapshot_translate_3d (snapshot, &GRAPHENE_POINT3D_INIT (
                                 - bobgui_widget_get_width (widget) / 2.f,
                                 - bobgui_widget_get_height (widget) / 2.f,
                                 bobgui_widget_get_width (widget) / 2.f));
      if (priv->active_transition_type == BOBGUI_STACK_TRANSITION_TYPE_ROTATE_LEFT)
        bobgui_widget_snapshot_child (widget, priv->last_visible_child->widget, snapshot);
      else
        bobgui_widget_snapshot_child (widget, priv->visible_child->widget, snapshot);
      bobgui_snapshot_restore (snapshot);
    }

  bobgui_snapshot_save (snapshot);
  bobgui_snapshot_translate_3d (snapshot, &GRAPHENE_POINT3D_INIT (
                             bobgui_widget_get_width (widget) / 2.f,
                             bobgui_widget_get_height (widget) / 2.f,
                             0));
  bobgui_snapshot_perspective (snapshot, 2 * bobgui_widget_get_width (widget) / 1.f);
  bobgui_snapshot_translate_3d (snapshot, &GRAPHENE_POINT3D_INIT (
                             0, 0,
                             - bobgui_widget_get_width (widget) / 2.f));
  bobgui_snapshot_rotate_3d (snapshot, 90 * (1.0 - progress), graphene_vec3_y_axis());
  bobgui_snapshot_translate_3d (snapshot, &GRAPHENE_POINT3D_INIT (
                             - bobgui_widget_get_width (widget) / 2.f,
                             - bobgui_widget_get_height (widget) / 2.f,
                             bobgui_widget_get_width (widget) / 2.f));

  if (priv->active_transition_type == BOBGUI_STACK_TRANSITION_TYPE_ROTATE_LEFT)
    bobgui_widget_snapshot_child (widget, priv->visible_child->widget, snapshot);
  else if (priv->last_visible_child)
    bobgui_widget_snapshot_child (widget, priv->last_visible_child->widget, snapshot);
  bobgui_snapshot_restore (snapshot);

  if (priv->last_visible_child && progress <= 0.5)
    {
      bobgui_snapshot_save (snapshot);
      bobgui_snapshot_translate_3d (snapshot, &GRAPHENE_POINT3D_INIT (
                                 bobgui_widget_get_width (widget) / 2.f,
                                 bobgui_widget_get_height (widget) / 2.f,
                                 0));
      bobgui_snapshot_perspective (snapshot, 2 * bobgui_widget_get_width (widget) / 1.f);
      bobgui_snapshot_translate_3d (snapshot, &GRAPHENE_POINT3D_INIT (
                                 0, 0,
                                 - bobgui_widget_get_width (widget) / 2.f));
      bobgui_snapshot_rotate_3d (snapshot, -90 * progress, graphene_vec3_y_axis());
      bobgui_snapshot_translate_3d (snapshot, &GRAPHENE_POINT3D_INIT (
                                 - bobgui_widget_get_width (widget) / 2.f,
                                 - bobgui_widget_get_height (widget) / 2.f,
                                 bobgui_widget_get_width (widget) / 2.f));
      if (priv->active_transition_type == BOBGUI_STACK_TRANSITION_TYPE_ROTATE_LEFT)
        bobgui_widget_snapshot_child (widget, priv->last_visible_child->widget, snapshot);
      else
        bobgui_widget_snapshot_child (widget, priv->visible_child->widget, snapshot);
      bobgui_snapshot_restore (snapshot);
    }
}

static void
bobgui_stack_snapshot_slide (BobguiWidget   *widget,
                          BobguiSnapshot *snapshot)
{
  BobguiStack *stack = BOBGUI_STACK (widget);
  BobguiStackPrivate *priv = bobgui_stack_get_instance_private (stack);

  if (priv->last_visible_child)
    {
      int x, y;
      int width, height;

      width = bobgui_widget_get_width (widget);
      height = bobgui_widget_get_height (widget);

      x = get_bin_window_x (stack);
      y = get_bin_window_y (stack);

      switch ((guint) priv->active_transition_type)
        {
        case BOBGUI_STACK_TRANSITION_TYPE_SLIDE_LEFT:
          x -= width;
          break;
        case BOBGUI_STACK_TRANSITION_TYPE_SLIDE_RIGHT:
          x += width;
          break;
        case BOBGUI_STACK_TRANSITION_TYPE_SLIDE_UP:
          y -= height;
          break;
        case BOBGUI_STACK_TRANSITION_TYPE_SLIDE_DOWN:
          y += height;
          break;
        case BOBGUI_STACK_TRANSITION_TYPE_OVER_UP:
        case BOBGUI_STACK_TRANSITION_TYPE_OVER_DOWN:
          y = 0;
          break;
        case BOBGUI_STACK_TRANSITION_TYPE_OVER_LEFT:
        case BOBGUI_STACK_TRANSITION_TYPE_OVER_RIGHT:
          x = 0;
          break;
        default:
          g_assert_not_reached ();
          break;
        }

      if (priv->last_visible_child != NULL)
        {
          if (bobgui_widget_get_valign (priv->last_visible_child->widget) == BOBGUI_ALIGN_END &&
              priv->last_visible_widget_height > height)
            y -= priv->last_visible_widget_height - height;
          else if (bobgui_widget_get_valign (priv->last_visible_child->widget) == BOBGUI_ALIGN_CENTER)
            y -= (priv->last_visible_widget_height - height) / 2;
        }

      bobgui_snapshot_save (snapshot);
      bobgui_snapshot_translate (snapshot, &GRAPHENE_POINT_INIT (x, y));
      bobgui_widget_snapshot_child (widget, priv->last_visible_child->widget, snapshot);
      bobgui_snapshot_restore (snapshot);
     }

  bobgui_widget_snapshot_child (widget,
                             priv->visible_child->widget,
                             snapshot);
}

static void
bobgui_stack_snapshot (BobguiWidget   *widget,
                    BobguiSnapshot *snapshot)
{
  BobguiStack *stack = BOBGUI_STACK (widget);
  BobguiStackPrivate *priv = bobgui_stack_get_instance_private (stack);

  if (priv->visible_child)
    {
      if (bobgui_progress_tracker_get_state (&priv->tracker) != BOBGUI_PROGRESS_STATE_AFTER)
        {
          bobgui_snapshot_push_clip (snapshot,
                                  &GRAPHENE_RECT_INIT(
                                      0, 0,
                                      bobgui_widget_get_width (widget),
                                      bobgui_widget_get_height (widget)
                                  ));

          switch (priv->active_transition_type)
            {
            case BOBGUI_STACK_TRANSITION_TYPE_CROSSFADE:
              bobgui_stack_snapshot_crossfade (widget, snapshot);
              break;
            case BOBGUI_STACK_TRANSITION_TYPE_SLIDE_LEFT:
            case BOBGUI_STACK_TRANSITION_TYPE_SLIDE_RIGHT:
            case BOBGUI_STACK_TRANSITION_TYPE_SLIDE_UP:
            case BOBGUI_STACK_TRANSITION_TYPE_SLIDE_DOWN:
            case BOBGUI_STACK_TRANSITION_TYPE_OVER_UP:
            case BOBGUI_STACK_TRANSITION_TYPE_OVER_DOWN:
            case BOBGUI_STACK_TRANSITION_TYPE_OVER_LEFT:
            case BOBGUI_STACK_TRANSITION_TYPE_OVER_RIGHT:
              bobgui_stack_snapshot_slide (widget, snapshot);
              break;
            case BOBGUI_STACK_TRANSITION_TYPE_UNDER_UP:
            case BOBGUI_STACK_TRANSITION_TYPE_UNDER_DOWN:
            case BOBGUI_STACK_TRANSITION_TYPE_UNDER_LEFT:
            case BOBGUI_STACK_TRANSITION_TYPE_UNDER_RIGHT:
              bobgui_stack_snapshot_under (widget, snapshot);
              break;
            case BOBGUI_STACK_TRANSITION_TYPE_ROTATE_LEFT:
            case BOBGUI_STACK_TRANSITION_TYPE_ROTATE_RIGHT:
              bobgui_stack_snapshot_cube (widget, snapshot);
              break;
            case BOBGUI_STACK_TRANSITION_TYPE_NONE:
            case BOBGUI_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT:
            case BOBGUI_STACK_TRANSITION_TYPE_SLIDE_UP_DOWN:
            case BOBGUI_STACK_TRANSITION_TYPE_OVER_UP_DOWN:
            case BOBGUI_STACK_TRANSITION_TYPE_OVER_DOWN_UP:
            case BOBGUI_STACK_TRANSITION_TYPE_OVER_LEFT_RIGHT:
            case BOBGUI_STACK_TRANSITION_TYPE_OVER_RIGHT_LEFT:
            case BOBGUI_STACK_TRANSITION_TYPE_ROTATE_LEFT_RIGHT:
            default:
              g_assert_not_reached ();
            }

          bobgui_snapshot_pop (snapshot);
        }
      else
        bobgui_widget_snapshot_child (widget,
                                   priv->visible_child->widget,
                                   snapshot);
    }
}

static void
adjust_child_allocation (BobguiWidget     *child,
                         BobguiAllocation *allocation)
{
  int min, width, height;
  BobguiAlign align;

  if (bobgui_widget_get_request_mode (child) == BOBGUI_SIZE_REQUEST_WIDTH_FOR_HEIGHT)
    {
      bobgui_widget_measure (child, BOBGUI_ORIENTATION_VERTICAL, -1,
                          &min, NULL, NULL, NULL);
      height = MAX (allocation->height, min);
      bobgui_widget_measure (child, BOBGUI_ORIENTATION_HORIZONTAL, height,
                          &min, NULL, NULL, NULL);
      width = MAX (allocation->width, min);
    }
  else /* BOBGUI_SIZE_REQUEST_HEIGHT_FOR_WIDTH or CONSTANT_SIZE */
    {
      bobgui_widget_measure (child, BOBGUI_ORIENTATION_HORIZONTAL, -1,
                          &min, NULL, NULL, NULL);
      width = MAX (allocation->width, min);
      bobgui_widget_measure (child, BOBGUI_ORIENTATION_VERTICAL, width,
                          &min, NULL, NULL, NULL);
      height = MAX (allocation->height, min);
    }

  if (width > allocation->width)
    {
      BobguiTextDirection direction = _bobgui_widget_get_direction (child);
      align = bobgui_widget_get_halign (child);

      if (align == BOBGUI_ALIGN_CENTER || align == BOBGUI_ALIGN_FILL)
        allocation->x -= (width - allocation->width) / 2;
      else if (align == (direction == BOBGUI_TEXT_DIR_RTL ? BOBGUI_ALIGN_START : BOBGUI_ALIGN_END))
        allocation->x -= (width - allocation->width);

      allocation->width = width;
    }

  if (height > allocation->height)
    {
      align = bobgui_widget_get_valign (child);

      if (align == BOBGUI_ALIGN_CENTER || align == BOBGUI_ALIGN_FILL)
        allocation->y -= (height - allocation->height) / 2;
      else if (align == BOBGUI_ALIGN_END)
        allocation->y -= (height - allocation->height);

      allocation->height = height;
    }
}

static void
bobgui_stack_size_allocate (BobguiWidget *widget,
                         int        width,
                         int        height,
                         int        baseline)
{
  BobguiStack *stack = BOBGUI_STACK (widget);
  BobguiStackPrivate *priv = bobgui_stack_get_instance_private (stack);
  BobguiAllocation child_allocation;

  if (priv->last_visible_child)
    {
      child_allocation = (BobguiAllocation) {0, 0, width, height};
      adjust_child_allocation (priv->last_visible_child->widget,
                               &child_allocation);

      bobgui_widget_size_allocate (priv->last_visible_child->widget,
                                &child_allocation, -1);
    }

  child_allocation.x = get_bin_window_x (stack);
  child_allocation.y = get_bin_window_y (stack);
  child_allocation.width = width;
  child_allocation.height = height;

  if (priv->visible_child)
    {
      adjust_child_allocation (priv->visible_child->widget,
                               &child_allocation);

      bobgui_widget_size_allocate (priv->visible_child->widget, &child_allocation, -1);
    }
}

static inline double
lerp (int    a,
      int    b,
      double progress)
{
  return a * (1.0 - progress) + b * progress;
}

/*
 * Given lerp (a, b, progress) -> r, find b. In other words: we know what size
 * we're interpolating *from*, the current size, and the current interpolation
 * progress; guess which size we're interpolating *to*.
 */
static inline double
inverse_lerp (int    a,
              int    r,
              double progress)
{
  return (r - a * (1.0 - progress)) / progress;
}

static void
bobgui_stack_measure (BobguiWidget      *widget,
                   BobguiOrientation  orientation,
                   int             for_size,
                   int            *minimum,
                   int            *natural,
                   int            *minimum_baseline,
                   int            *natural_baseline)
{
  BobguiStack *stack = BOBGUI_STACK (widget);
  BobguiStackPrivate *priv = bobgui_stack_get_instance_private (stack);
  BobguiStackPage *child_info;
  BobguiWidget *child;
  guint idx;
  int child_min, child_nat, child_for_size;
  int last_size, last_opposite_size;
  double t;

  if (priv->interpolate_size && priv->last_visible_child)
    {
      t = bobgui_progress_tracker_get_ease_out_cubic (&priv->tracker, FALSE);

      if (orientation == BOBGUI_ORIENTATION_HORIZONTAL)
        {
          last_size = priv->last_visible_widget_width;
          last_opposite_size = priv->last_visible_widget_height;
        }
      else
        {
          last_size = priv->last_visible_widget_height;
          last_opposite_size = priv->last_visible_widget_width;
        }

      /* Work out which for_size we want to pass for measuring our children */
      if (for_size == -1 || for_size == last_opposite_size ||
          priv->homogeneous[OPPOSITE_ORIENTATION(orientation)])
        child_for_size = for_size;
      else if (t <= 0.0)
        /* We're going to return last_size anyway */
        child_for_size = -1;
      else
        {
          double d = inverse_lerp (last_opposite_size, for_size, t);
          /* inverse_lerp is numerically unstable due to its use of floating-
           * point division, potentially by a very small value when progress is
           * close to zero. So we sanity check the return value.
           */
          if (isnan (d) || isinf (d) || d < 0 || d >= G_MAXINT)
            child_for_size = -1;
          else
            child_for_size = floor (d);
        }
    }
  else
    {
      t = 1.0;
      last_size = 0;
      child_for_size = for_size;
    }

  *minimum = 0;
  *natural = 0;

  for (idx = 0; idx < priv->children->len; idx++)
    {
      child_info = g_ptr_array_index (priv->children, idx);
      child = child_info->widget;

      if (!priv->homogeneous[orientation] &&
          priv->visible_child != child_info)
        continue;
      if (!bobgui_widget_get_visible (child))
        continue;

      if (!priv->homogeneous[OPPOSITE_ORIENTATION(orientation)] && priv->visible_child != child_info)
        {
          int measure_for_size;

          /* Make sure to measure at least for the minimum size */
          if (child_for_size == -1)
            measure_for_size = -1;
          else
            {
              bobgui_widget_measure (child, OPPOSITE_ORIENTATION (orientation),
                                  -1,
                                  &measure_for_size, NULL,
                                  NULL, NULL);
              measure_for_size = MAX (measure_for_size, child_for_size);
            }

          bobgui_widget_measure (child, orientation,
                              measure_for_size,
                              &child_min, &child_nat,
                              NULL, NULL);
        }
      else
        bobgui_widget_measure (child, orientation,
                            child_for_size,
                            &child_min, &child_nat,
                            NULL, NULL);

      *minimum = MAX (*minimum, child_min);
      *natural = MAX (*natural, child_nat);
  }

  if (priv->last_visible_child != NULL &&
      priv->interpolate_size &&
      !priv->homogeneous[orientation])
    {
      *minimum = ceil (lerp (last_size, *minimum, t));
      *natural = ceil (lerp (last_size, *natural, t));
    }
}

static void
bobgui_stack_init (BobguiStack *stack)
{
  BobguiStackPrivate *priv = bobgui_stack_get_instance_private (stack);

  priv->homogeneous[BOBGUI_ORIENTATION_VERTICAL] = TRUE;
  priv->homogeneous[BOBGUI_ORIENTATION_HORIZONTAL] = TRUE;
  priv->transition_duration = 200;
  priv->transition_type = BOBGUI_STACK_TRANSITION_TYPE_NONE;
  priv->children = g_ptr_array_new();
}

/**
 * bobgui_stack_get_pages:
 * @stack: a `BobguiStack`
 *
 * Returns a `GListModel` that contains the pages of the stack.
 *
 * This can be used to keep an up-to-date view. The model also
 * implements [iface@Bobgui.SelectionModel] and can be used to track
 * and modify the visible page.
 *
 * Returns: (transfer full): a `BobguiSelectionModel` for the stack's children
 */
BobguiSelectionModel *
bobgui_stack_get_pages (BobguiStack *stack)
{
  BobguiStackPrivate *priv = bobgui_stack_get_instance_private (stack);

  g_return_val_if_fail (BOBGUI_IS_STACK (stack), NULL);

  if (priv->pages)
    return g_object_ref (priv->pages);

  priv->pages = BOBGUI_SELECTION_MODEL (bobgui_stack_pages_new (stack));
  g_object_add_weak_pointer (G_OBJECT (priv->pages), (gpointer *)&priv->pages);

  return priv->pages;
}

/**
 * bobgui_stack_page_get_visible:
 * @self: a `BobguiStackPage`
 *
 * Returns whether @page is visible in its `BobguiStack`.
 *
 * This is independent from the [property@Bobgui.Widget:visible]
 * property of its widget.
 *
 * Returns: %TRUE if @page is visible
 */
gboolean
bobgui_stack_page_get_visible (BobguiStackPage *self)
{
  g_return_val_if_fail (BOBGUI_IS_STACK_PAGE (self), FALSE);

  return self->visible;
}

/**
 * bobgui_stack_page_set_visible:
 * @self: a `BobguiStackPage`
 * @visible: The new property value
 *
 * Sets whether @page is visible in its `BobguiStack`.
 */
void
bobgui_stack_page_set_visible (BobguiStackPage *self,
                            gboolean      visible)
{
  g_return_if_fail (BOBGUI_IS_STACK_PAGE (self));

  visible = !!visible;

  if (visible == self->visible)
    return;

  self->visible = visible;

  if (self->widget && bobgui_widget_get_parent (self->widget))
    update_child_visible (BOBGUI_STACK (bobgui_widget_get_parent (self->widget)), self);

  g_object_notify_by_pspec (G_OBJECT (self), stack_page_props[CHILD_PROP_VISIBLE]);
}

/**
 * bobgui_stack_page_get_needs_attention:
 * @self: a `BobguiStackPage`
 *
 * Returns whether the page is marked as “needs attention”.
 *
 * Returns: The value of the [property@Bobgui.StackPage:needs-attention]
 *   property.
 */
gboolean
bobgui_stack_page_get_needs_attention (BobguiStackPage *self)
{
  return self->needs_attention;
}

/**
 * bobgui_stack_page_set_needs_attention:
 * @self: a `BobguiStackPage`
 * @setting: the new value to set
 *
 * Sets whether the page is marked as “needs attention”.
 */
void
bobgui_stack_page_set_needs_attention (BobguiStackPage *self,
                                    gboolean      setting)
{
  setting = !!setting;

  if (setting == self->needs_attention)
    return;

  self->needs_attention = setting;
  g_object_notify_by_pspec (G_OBJECT (self), stack_page_props[CHILD_PROP_NEEDS_ATTENTION]);
}

/**
 * bobgui_stack_page_get_use_underline:
 * @self: a `BobguiStackPage`
 *
 * Gets whether underlines in the page title indicate mnemonics.
 *
 * Returns: The value of the [property@Bobgui.StackPage:use-underline] property
 */
gboolean
bobgui_stack_page_get_use_underline (BobguiStackPage *self)
{
  return self->use_underline;
}

/**
 * bobgui_stack_page_set_use_underline:
 * @self: a `BobguiStackPage`
 * @setting: the new value to set
 *
 * Sets whether underlines in the page title indicate mnemonics.
 */
void
bobgui_stack_page_set_use_underline (BobguiStackPage *self,
                                  gboolean      setting)
{
  setting = !!setting;

  if (setting == self->use_underline)
    return;

  self->use_underline = setting;
  g_object_notify_by_pspec (G_OBJECT (self), stack_page_props[CHILD_PROP_USE_UNDERLINE]);
}


/**
 * bobgui_stack_page_get_name:
 * @self: a `BobguiStackPage`
 *
 * Returns the name of the page.
 *
 * Returns: (nullable): The value of the [property@Bobgui.StackPage:name] property
 */
const char *
bobgui_stack_page_get_name (BobguiStackPage *self)
{
  g_return_val_if_fail (BOBGUI_IS_STACK_PAGE (self), NULL);

  return self->name;
}

/**
 * bobgui_stack_page_set_name:
 * @self: a `BobguiStackPage`
 * @setting: (transfer none): the new value to set
 *
 * Sets the name of the page.
 */
void
bobgui_stack_page_set_name (BobguiStackPage *self,
                         const char   *setting)
{
  BobguiStack *stack = NULL;
  BobguiStackPrivate *priv = NULL;

  g_return_if_fail (BOBGUI_IS_STACK_PAGE (self));

  if (self->widget &&
      bobgui_widget_get_parent (self->widget) &&
      BOBGUI_IS_STACK (bobgui_widget_get_parent (self->widget)))
    {
      guint idx;

      stack = BOBGUI_STACK (bobgui_widget_get_parent (self->widget));
      priv = bobgui_stack_get_instance_private (stack);

      for (idx = 0; idx < priv->children->len; idx++)
        {
          BobguiStackPage *info2 = g_ptr_array_index (priv->children, idx);
          if (self == info2)
            continue;

          if (g_strcmp0 (info2->name, setting) == 0)
            {
              g_warning ("Duplicate child name in BobguiStack: %s", setting);
              break;
            }
        }
    }

  if (setting == self->name)
    return;

  g_free (self->name);
  self->name = g_strdup (setting);
  g_object_notify_by_pspec (G_OBJECT (self), stack_page_props[CHILD_PROP_NAME]);

  if (priv && priv->visible_child == self)
    g_object_notify_by_pspec (G_OBJECT (stack),
                              stack_props[PROP_VISIBLE_CHILD_NAME]);
}

/**
 * bobgui_stack_page_get_title:
 * @self: a `BobguiStackPage`
 *
 * Gets the page title.
 *
 * Returns: (nullable): The value of the [property@Bobgui.StackPage:title] property
 */
const char *
bobgui_stack_page_get_title (BobguiStackPage *self)
{
  g_return_val_if_fail (BOBGUI_IS_STACK_PAGE (self), NULL);

  return self->title;
}

/**
 * bobgui_stack_page_set_title:
 * @self: a `BobguiStackPage`
 * @setting: (transfer none): the new value to set
 *
 * Sets the page title.
 */
void
bobgui_stack_page_set_title (BobguiStackPage *self,
                          const char   *setting)
{
  g_return_if_fail (BOBGUI_IS_STACK_PAGE (self));

  if (setting == self->title)
    return;

  g_free (self->title);
  self->title = g_strdup (setting);
  g_object_notify_by_pspec (G_OBJECT (self), stack_page_props[CHILD_PROP_TITLE]);

  bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (self),
                                  BOBGUI_ACCESSIBLE_PROPERTY_LABEL, self->title,
                                  -1);
}

/**
 * bobgui_stack_page_get_icon_name:
 * @self: a `BobguiStackPage`
 *
 * Returns the icon name of the page.
 *
 * Returns: (nullable): The value of the [property@Bobgui.StackPage:icon-name] property
 */
const char *
bobgui_stack_page_get_icon_name (BobguiStackPage *self)
{
  g_return_val_if_fail (BOBGUI_IS_STACK_PAGE (self), NULL);

  return self->icon_name;
}

/**
 * bobgui_stack_page_set_icon_name:
 * @self: a `BobguiStackPage`
 * @setting: (transfer none): the new value to set
 *
 * Sets the icon name of the page.
 */
void
bobgui_stack_page_set_icon_name (BobguiStackPage *self,
                              const char   *setting)
{
  g_return_if_fail (BOBGUI_IS_STACK_PAGE (self));

  if (setting == self->icon_name)
    return;

  g_free (self->icon_name);
  self->icon_name = g_strdup (setting);
  g_object_notify_by_pspec (G_OBJECT (self), stack_page_props[CHILD_PROP_ICON_NAME]);
}
