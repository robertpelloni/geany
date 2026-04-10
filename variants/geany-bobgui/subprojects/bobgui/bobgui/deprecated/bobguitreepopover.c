/*
 * Copyright © 2019 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the licence, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Matthias Clasen
 */

#include "config.h"

#include "bobguitreepopoverprivate.h"

#include "bobguitreemodel.h"
#include "bobguicellarea.h"
#include "bobguicelllayout.h"
#include "bobguicellview.h"
#include "bobguiprivate.h"
#include "bobguigizmoprivate.h"
#include "bobguiwidgetprivate.h"
#include "bobguibuiltiniconprivate.h"
#include "bobguiscrolledwindow.h"
#include "bobguiviewport.h"

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

// TODO
// positioning + sizing

struct _BobguiTreePopover
{
  BobguiPopover parent_instance;

  BobguiTreeModel *model;

  BobguiCellArea *area;
  BobguiCellAreaContext *context;

  gulong size_changed_id;
  gulong row_inserted_id;
  gulong row_deleted_id;
  gulong row_changed_id;
  gulong row_reordered_id;
  gulong apply_attributes_id;

  BobguiTreeViewRowSeparatorFunc row_separator_func;
  gpointer                    row_separator_data;
  GDestroyNotify              row_separator_destroy;

  BobguiWidget *active_item;
};

enum {
  PROP_0,
  PROP_MODEL,
  PROP_CELL_AREA,

  NUM_PROPERTIES
};

enum {
  MENU_ACTIVATE,
  NUM_SIGNALS
};

static guint signals[NUM_SIGNALS];

static void bobgui_tree_popover_cell_layout_init (BobguiCellLayoutIface  *iface);
static void bobgui_tree_popover_set_area (BobguiTreePopover *popover,
                                       BobguiCellArea    *area);
static void rebuild_menu (BobguiTreePopover *popover);
static void context_size_changed_cb (BobguiCellAreaContext *context,
                                     GParamSpec         *pspec,
                                     BobguiWidget          *popover);
static BobguiWidget * bobgui_tree_popover_create_item (BobguiTreePopover *popover,
                                                 BobguiTreePath    *path,
                                                 BobguiTreeIter    *iter,
                                                 gboolean        header_item);
static BobguiWidget * bobgui_tree_popover_get_path_item (BobguiTreePopover *popover,
                                                   BobguiTreePath    *search);
static void bobgui_tree_popover_set_active_item (BobguiTreePopover *popover,
                                              BobguiWidget      *item);

G_DEFINE_TYPE_WITH_CODE (BobguiTreePopover, bobgui_tree_popover, BOBGUI_TYPE_POPOVER,
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_CELL_LAYOUT,
                                                bobgui_tree_popover_cell_layout_init));

static void
bobgui_tree_popover_constructed (GObject *object)
{
  BobguiTreePopover *popover = BOBGUI_TREE_POPOVER (object);

  G_OBJECT_CLASS (bobgui_tree_popover_parent_class)->constructed (object);

  if (!popover->area)
    {
      BobguiCellArea *area = bobgui_cell_area_box_new ();
      bobgui_tree_popover_set_area (popover, area);
    }

  popover->context = bobgui_cell_area_create_context (popover->area);

  popover->size_changed_id = g_signal_connect (popover->context, "notify",
                                               G_CALLBACK (context_size_changed_cb), popover);
}

static void
bobgui_tree_popover_dispose (GObject *object)
{
  BobguiTreePopover *popover = BOBGUI_TREE_POPOVER (object);

  bobgui_tree_popover_set_model (popover, NULL);
  bobgui_tree_popover_set_area (popover, NULL);

  if (popover->context)
    {
      g_signal_handler_disconnect (popover->context, popover->size_changed_id);
      popover->size_changed_id = 0;

      g_clear_object (&popover->context);
    }

  G_OBJECT_CLASS (bobgui_tree_popover_parent_class)->dispose (object);
}

static void
bobgui_tree_popover_finalize (GObject *object)
{
  BobguiTreePopover *popover = BOBGUI_TREE_POPOVER (object);

  if (popover->row_separator_destroy)
    popover->row_separator_destroy (popover->row_separator_data);

  G_OBJECT_CLASS (bobgui_tree_popover_parent_class)->finalize (object);
}

static void
bobgui_tree_popover_set_property (GObject      *object,
                               guint         prop_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  BobguiTreePopover *popover = BOBGUI_TREE_POPOVER (object);

  switch (prop_id)
    {
    case PROP_MODEL:
      bobgui_tree_popover_set_model (popover, g_value_get_object (value));
      break;

    case PROP_CELL_AREA:
      bobgui_tree_popover_set_area (popover, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_tree_popover_get_property (GObject    *object,
                               guint       prop_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  BobguiTreePopover *popover = BOBGUI_TREE_POPOVER (object);

  switch (prop_id)
    {
    case PROP_MODEL:
      g_value_set_object (value, popover->model);
      break;

    case PROP_CELL_AREA:
      g_value_set_object (value, popover->area);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_tree_popover_class_init (BobguiTreePopoverClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->constructed  = bobgui_tree_popover_constructed;
  object_class->dispose = bobgui_tree_popover_dispose;
  object_class->finalize = bobgui_tree_popover_finalize;
  object_class->set_property = bobgui_tree_popover_set_property;
  object_class->get_property = bobgui_tree_popover_get_property;

  g_object_class_install_property (object_class,
                                   PROP_MODEL,
                                   g_param_spec_object ("model", NULL, NULL,
                                                        BOBGUI_TYPE_TREE_MODEL,
                                                        BOBGUI_PARAM_READWRITE));

  g_object_class_install_property (object_class,
                                   PROP_CELL_AREA,
                                   g_param_spec_object ("cell-area", NULL, NULL,
                                                        BOBGUI_TYPE_CELL_AREA,
                                                        BOBGUI_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

  signals[MENU_ACTIVATE] =
    g_signal_new (I_("menu-activate"),
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_FIRST,
                  0,
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 1, G_TYPE_STRING);
}

static BobguiWidget *
bobgui_tree_popover_get_stack (BobguiTreePopover *popover)
{
  BobguiWidget *sw = bobgui_popover_get_child (BOBGUI_POPOVER (popover));
  BobguiWidget *vp = bobgui_scrolled_window_get_child (BOBGUI_SCROLLED_WINDOW (sw));
  BobguiWidget *stack = bobgui_viewport_get_child (BOBGUI_VIEWPORT (vp));

  return stack;
}

static void
bobgui_tree_popover_add_submenu (BobguiTreePopover *popover,
                              BobguiWidget      *submenu,
                              const char     *name)
{
  BobguiWidget *stack = bobgui_tree_popover_get_stack (popover);
  bobgui_stack_add_named (BOBGUI_STACK (stack), submenu, name);
}

static BobguiWidget *
bobgui_tree_popover_get_submenu (BobguiTreePopover *popover,
                              const char     *name)
{
  BobguiWidget *stack = bobgui_tree_popover_get_stack (popover);
  return bobgui_stack_get_child_by_name (BOBGUI_STACK (stack), name);
}

void
bobgui_tree_popover_open_submenu (BobguiTreePopover *popover,
                               const char     *name)
{
  BobguiWidget *stack = bobgui_tree_popover_get_stack (popover);
  bobgui_stack_set_visible_child_name (BOBGUI_STACK (stack), name);
}

static void
bobgui_tree_popover_init (BobguiTreePopover *popover)
{
  BobguiWidget *sw;
  BobguiWidget *stack;

  sw = bobgui_scrolled_window_new ();
  bobgui_scrolled_window_set_policy (BOBGUI_SCROLLED_WINDOW (sw), BOBGUI_POLICY_NEVER, BOBGUI_POLICY_AUTOMATIC);
  bobgui_scrolled_window_set_propagate_natural_width (BOBGUI_SCROLLED_WINDOW (sw), TRUE);
  bobgui_scrolled_window_set_propagate_natural_height (BOBGUI_SCROLLED_WINDOW (sw), TRUE);
  bobgui_popover_set_child (BOBGUI_POPOVER (popover), sw);

  stack = bobgui_stack_new ();
  bobgui_stack_set_vhomogeneous (BOBGUI_STACK (stack), FALSE);
  bobgui_stack_set_transition_type (BOBGUI_STACK (stack), BOBGUI_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT);
  bobgui_stack_set_interpolate_size (BOBGUI_STACK (stack), TRUE);
  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (sw), stack);

  bobgui_widget_add_css_class (BOBGUI_WIDGET (popover), "menu");
}

static BobguiCellArea *
bobgui_tree_popover_cell_layout_get_area (BobguiCellLayout *layout)
{
  return BOBGUI_TREE_POPOVER (layout)->area;
}

static void
bobgui_tree_popover_cell_layout_init (BobguiCellLayoutIface  *iface)
{
  iface->get_area = bobgui_tree_popover_cell_layout_get_area;
}

static void
insert_at_position (BobguiBox    *box,
                    BobguiWidget *child,
                    int        position)
{
  BobguiWidget *sibling = NULL;

  if (position > 0)
    {
      int i;

      sibling = bobgui_widget_get_first_child (BOBGUI_WIDGET (box));
      for (i = 1; i < position; i++)
        sibling = bobgui_widget_get_next_sibling (sibling);
    }

  bobgui_box_insert_child_after (box, child, sibling);
}

static BobguiWidget *
ensure_submenu (BobguiTreePopover *popover,
                BobguiTreePath    *path)
{
  BobguiWidget *box;
  char *name;

  if (path)
    name = bobgui_tree_path_to_string (path);
  else
    name = NULL;

  box = bobgui_tree_popover_get_submenu (popover, name ? name : "main");
  if (!box)
    {
      box = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
      bobgui_tree_popover_add_submenu (popover, box, name ? name : "main");
      if (path)
        {
          BobguiTreeIter iter;
          BobguiWidget *item;
          bobgui_tree_model_get_iter (popover->model, &iter, path);
          item = bobgui_tree_popover_create_item (popover, path, &iter, TRUE);
          bobgui_box_append (BOBGUI_BOX (box), item);
          bobgui_box_append (BOBGUI_BOX (box), bobgui_separator_new (BOBGUI_ORIENTATION_HORIZONTAL));
        }

    }

  g_free (name);

  return box;
}

static void
row_inserted_cb (BobguiTreeModel   *model,
                 BobguiTreePath    *path,
                 BobguiTreeIter    *iter,
                 BobguiTreePopover *popover)
{
  int *indices, depth, index;
  BobguiWidget *item;
  BobguiWidget *box;

  indices = bobgui_tree_path_get_indices (path);
  depth = bobgui_tree_path_get_depth (path);
  index = indices[depth - 1];

  item = bobgui_tree_popover_create_item (popover, path, iter, FALSE);
  if (depth == 1)
    {
      box = ensure_submenu (popover, NULL);
      insert_at_position (BOBGUI_BOX (box), item, index);
    }
  else
    {
      BobguiTreePath *ppath;

      ppath = bobgui_tree_path_copy (path);
      bobgui_tree_path_up (ppath);

      box = ensure_submenu (popover, ppath);
      insert_at_position (BOBGUI_BOX (box), item, index + 2);

      bobgui_tree_path_free (ppath);
    }

  bobgui_cell_area_context_reset (popover->context);
}

static void
row_deleted_cb (BobguiTreeModel   *model,
                BobguiTreePath    *path,
                BobguiTreePopover *popover)
{
  BobguiWidget *item;

  item = bobgui_tree_popover_get_path_item (popover, path);

  if (item)
    {
      bobgui_widget_unparent (item);
      bobgui_cell_area_context_reset (popover->context);
    }
}

static void
row_changed_cb (BobguiTreeModel   *model,
                BobguiTreePath    *path,
                BobguiTreeIter    *iter,
                BobguiTreePopover *popover)
{
  gboolean is_separator = FALSE;
  BobguiWidget *item;
  int *indices, depth, index;

  item = bobgui_tree_popover_get_path_item (popover, path);

  if (!item)
    return;

  indices = bobgui_tree_path_get_indices (path);
  depth = bobgui_tree_path_get_depth (path);
  index = indices[depth - 1];

  if (popover->row_separator_func)
    is_separator = popover->row_separator_func (model, iter, popover->row_separator_data);

  if (is_separator != BOBGUI_IS_SEPARATOR (item))
    {
      BobguiWidget *box = bobgui_widget_get_parent (item);

      bobgui_box_remove (BOBGUI_BOX (box), item);

      item = bobgui_tree_popover_create_item (popover, path, iter, FALSE);

      if (depth == 1)
        insert_at_position (BOBGUI_BOX (box), item, index);
      else
        insert_at_position (BOBGUI_BOX (box), item, index + 2);
    }
}

static void
row_reordered_cb (BobguiTreeModel   *model,
                  BobguiTreePath    *path,
                  BobguiTreeIter    *iter,
                  int            *new_order,
                  BobguiTreePopover *popover)
{
  rebuild_menu (popover);
}

static void
context_size_changed_cb (BobguiCellAreaContext *context,
                         GParamSpec         *pspec,
                         BobguiWidget          *popover)
{
  if (!strcmp (pspec->name, "minimum-width") ||
      !strcmp (pspec->name, "natural-width") ||
      !strcmp (pspec->name, "minimum-height") ||
      !strcmp (pspec->name, "natural-height"))
    bobgui_widget_queue_resize (popover);
}

static gboolean
area_is_sensitive (BobguiCellArea *area)
{
  GList    *cells, *list;
  gboolean  sensitive = FALSE;

  cells = bobgui_cell_layout_get_cells (BOBGUI_CELL_LAYOUT (area));

  for (list = cells; list; list = list->next)
    {
      g_object_get (list->data, "sensitive", &sensitive, NULL);

      if (sensitive)
        break;
    }
  g_list_free (cells);

  return sensitive;
}

static BobguiWidget *
bobgui_tree_popover_get_path_item (BobguiTreePopover *popover,
                                BobguiTreePath    *search)
{
  BobguiWidget *stack = bobgui_tree_popover_get_stack (popover);
  BobguiWidget *item = NULL;
  BobguiWidget *stackchild;
  BobguiWidget *child;

  for (stackchild = bobgui_widget_get_first_child (stack);
       stackchild != NULL;
       stackchild = bobgui_widget_get_next_sibling (stackchild))
    {
      for (child = bobgui_widget_get_first_child (stackchild);
           !item && child;
           child = bobgui_widget_get_next_sibling (child))
        {
          BobguiTreePath *path  = NULL;

          if (BOBGUI_IS_SEPARATOR (child))
            {
              BobguiTreeRowReference *row = g_object_get_data (G_OBJECT (child), "bobgui-tree-path");

              if (row)
                {
                  path = bobgui_tree_row_reference_get_path (row);
                  if (!path)
                    item = child;
                }
            }
          else
            {
              BobguiWidget *view = BOBGUI_WIDGET (g_object_get_data (G_OBJECT (child), "view"));

              path = bobgui_cell_view_get_displayed_row (BOBGUI_CELL_VIEW (view));

              if (!path)
                item = child;
             }

           if (path)
             {
               if (bobgui_tree_path_compare (search, path) == 0)
                 item = child;
               bobgui_tree_path_free (path);
             }
        }
    }

  return item;
}

static void
area_apply_attributes_cb (BobguiCellArea    *area,
                          BobguiTreeModel   *tree_model,
                          BobguiTreeIter    *iter,
                          gboolean       is_expander,
                          gboolean       is_expanded,
                          BobguiTreePopover *popover)
{
  BobguiTreePath*path;
  BobguiWidget *item;
  gboolean sensitive;
  BobguiTreeIter dummy;
  gboolean has_submenu = FALSE;

  if (bobgui_tree_model_iter_children (popover->model, &dummy, iter))
    has_submenu = TRUE;

  path = bobgui_tree_model_get_path (tree_model, iter);
  item = bobgui_tree_popover_get_path_item (popover, path);

  if (item)
    {
      sensitive = area_is_sensitive (popover->area);
      bobgui_widget_set_sensitive (item, sensitive || has_submenu);
    }

  bobgui_tree_path_free (path);
}

static void
bobgui_tree_popover_set_area (BobguiTreePopover *popover,
                           BobguiCellArea    *area)
{
  if (popover->area)
    {
      g_signal_handler_disconnect (popover->area, popover->apply_attributes_id);
      popover->apply_attributes_id = 0;
      g_clear_object (&popover->area);
    }

  popover->area = area;

  if (popover->area)
    {
      g_object_ref_sink (popover->area);
      popover->apply_attributes_id = g_signal_connect (popover->area, "apply-attributes",
                                                       G_CALLBACK (area_apply_attributes_cb), popover);
    }
}

static void
activate_item (BobguiWidget      *item,
               BobguiTreePopover *popover)
{
  BobguiCellView *view;
  BobguiTreePath *path;
  char *path_str;
  gboolean is_header = FALSE;
  gboolean has_submenu = FALSE;

  is_header = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (item), "is-header"));

  view = BOBGUI_CELL_VIEW (g_object_get_data (G_OBJECT (item), "view"));

  path = bobgui_cell_view_get_displayed_row (view);

  if (is_header)
    {
      bobgui_tree_path_up (path);
    }
  else
    {
      BobguiTreeIter iter;
      BobguiTreeIter dummy;

      bobgui_tree_model_get_iter (popover->model, &iter, path);
      if (bobgui_tree_model_iter_children (popover->model, &dummy, &iter))
        has_submenu = TRUE;
    }

  path_str = bobgui_tree_path_to_string (path);

  if (is_header || has_submenu)
    {
      bobgui_tree_popover_open_submenu (popover, path_str ? path_str : "main");
    }
  else
    {
      g_signal_emit (popover, signals[MENU_ACTIVATE], 0, path_str);
      bobgui_popover_popdown (BOBGUI_POPOVER (popover));
    }

  g_free (path_str);
  bobgui_tree_path_free (path);
}

static void
item_activated_cb (BobguiGesture     *gesture,
                   guint           n_press,
                   double          x,
                   double          y,
                   BobguiTreePopover *popover)
{
  BobguiWidget *item = bobgui_event_controller_get_widget (BOBGUI_EVENT_CONTROLLER (gesture));
  activate_item (item, popover);
}

static void
enter_cb (BobguiEventController   *controller,
          double                x,
          double                y,
          BobguiTreePopover       *popover)
{
  BobguiWidget *item;
  item = bobgui_event_controller_get_widget (controller);

  bobgui_tree_popover_set_active_item (popover, item);
}

static void
enter_focus_cb (BobguiEventController   *controller,
                BobguiTreePopover       *popover)
{
  BobguiWidget *item = bobgui_event_controller_get_widget (controller);

  bobgui_tree_popover_set_active_item (popover, item);
}

static gboolean
activate_shortcut (BobguiWidget *widget,
                   GVariant  *args,
                   gpointer   user_data)
{
  activate_item (widget, user_data);
  return TRUE;
}

static BobguiWidget *
bobgui_tree_popover_create_item (BobguiTreePopover *popover,
                              BobguiTreePath    *path,
                              BobguiTreeIter    *iter,
                              gboolean        header_item)
{
  BobguiWidget *item, *view;
  gboolean is_separator = FALSE;

  if (popover->row_separator_func)
    is_separator = popover->row_separator_func (popover->model, iter, popover->row_separator_data);

  if (is_separator)
    {
      item = bobgui_separator_new (BOBGUI_ORIENTATION_HORIZONTAL);
      g_object_set_data_full (G_OBJECT (item), "bobgui-tree-path",
                                               bobgui_tree_row_reference_new (popover->model, path),
                                               (GDestroyNotify)bobgui_tree_row_reference_free);
    }
  else
    {
      BobguiEventController *controller;
      BobguiTreeIter dummy;
      gboolean has_submenu = FALSE;
      BobguiWidget *indicator;

      if (!header_item &&
          bobgui_tree_model_iter_children (popover->model, &dummy, iter))
        has_submenu = TRUE;

      view = bobgui_cell_view_new_with_context (popover->area, popover->context);
      bobgui_cell_view_set_model (BOBGUI_CELL_VIEW (view), popover->model);
      bobgui_cell_view_set_displayed_row (BOBGUI_CELL_VIEW (view), path);
      bobgui_widget_set_hexpand (view, TRUE);

      item = bobgui_gizmo_new ("modelbutton", NULL, NULL, NULL, NULL,
                            (BobguiGizmoFocusFunc)bobgui_widget_focus_self,
                            (BobguiGizmoGrabFocusFunc)bobgui_widget_grab_focus_self);
      bobgui_widget_set_layout_manager (item, bobgui_box_layout_new (BOBGUI_ORIENTATION_HORIZONTAL));
      bobgui_widget_set_focusable (item, TRUE);
      bobgui_widget_add_css_class (item, "flat");

      if (header_item)
        {
          indicator = bobgui_builtin_icon_new ("arrow");
          bobgui_widget_add_css_class (indicator, "left");
          bobgui_widget_set_parent (indicator, item);
        }

      bobgui_widget_set_parent (view, item);

      indicator = bobgui_builtin_icon_new (has_submenu ? "arrow" : "none");
      bobgui_widget_add_css_class (indicator, "right");
      bobgui_widget_set_parent (indicator, item);

      controller = BOBGUI_EVENT_CONTROLLER (bobgui_gesture_click_new ());
      g_signal_connect (controller, "pressed", G_CALLBACK (item_activated_cb), popover);
      bobgui_widget_add_controller (item, BOBGUI_EVENT_CONTROLLER (controller));

      controller = bobgui_event_controller_motion_new ();
      g_signal_connect (controller, "enter", G_CALLBACK (enter_cb), popover);
      bobgui_widget_add_controller (item, controller);

      controller = bobgui_event_controller_focus_new ();
      g_signal_connect (controller, "enter", G_CALLBACK (enter_focus_cb), popover);
      bobgui_widget_add_controller (item, controller);

      {
        const guint activate_keyvals[] = { GDK_KEY_space, GDK_KEY_KP_Space,
                                           GDK_KEY_Return, GDK_KEY_ISO_Enter,
                                           GDK_KEY_KP_Enter };
        BobguiShortcutTrigger *trigger;
        BobguiShortcut *shortcut;

        trigger = g_object_ref (bobgui_never_trigger_get ());
        for (int i = 0; i < G_N_ELEMENTS (activate_keyvals); i++)
          trigger = bobgui_alternative_trigger_new (bobgui_keyval_trigger_new (activate_keyvals[i], 0), trigger);

        shortcut = bobgui_shortcut_new (trigger, bobgui_callback_action_new (activate_shortcut, popover, NULL));
        controller = bobgui_shortcut_controller_new ();
        bobgui_shortcut_controller_add_shortcut (BOBGUI_SHORTCUT_CONTROLLER (controller), shortcut);
        bobgui_widget_add_controller (item, controller);
      }

      g_object_set_data (G_OBJECT (item), "is-header", GINT_TO_POINTER (header_item));
      g_object_set_data (G_OBJECT (item), "view", view);
    }

  return item;
}

static void
populate (BobguiTreePopover *popover,
          BobguiTreeIter    *parent)
{
  BobguiTreeIter iter;
  gboolean valid = FALSE;

  if (!popover->model)
    return;

  valid = bobgui_tree_model_iter_children (popover->model, &iter, parent);

  while (valid)
    {
      BobguiTreePath *path;

      path = bobgui_tree_model_get_path (popover->model, &iter);
      row_inserted_cb (popover->model, path, &iter, popover);

      populate (popover, &iter);

      valid = bobgui_tree_model_iter_next (popover->model, &iter);
      bobgui_tree_path_free (path);
    }
}

static void
bobgui_tree_popover_populate (BobguiTreePopover *popover)
{
  populate (popover, NULL);
}

static void
rebuild_menu (BobguiTreePopover *popover)
{
  BobguiWidget *stack;
  BobguiWidget *child;

  stack = bobgui_tree_popover_get_stack (popover);
  while ((child = bobgui_widget_get_first_child (stack)))
    bobgui_stack_remove (BOBGUI_STACK (stack), child);

  if (popover->model)
    bobgui_tree_popover_populate (popover);
}

void
bobgui_tree_popover_set_model (BobguiTreePopover *popover,
                            BobguiTreeModel   *model)
{
  if (popover->model == model)
    return;

  if (popover->model)
    {
      g_signal_handler_disconnect (popover->model, popover->row_inserted_id);
      g_signal_handler_disconnect (popover->model, popover->row_deleted_id);
      g_signal_handler_disconnect (popover->model, popover->row_changed_id);
      g_signal_handler_disconnect (popover->model, popover->row_reordered_id);
      popover->row_inserted_id  = 0;
      popover->row_deleted_id = 0;
      popover->row_changed_id = 0;
      popover->row_reordered_id = 0;

      g_object_unref (popover->model);
    }

  popover->model = model;

  if (popover->model)
    {
      g_object_ref (popover->model);

      popover->row_inserted_id = g_signal_connect (popover->model, "row-inserted",
                                                   G_CALLBACK (row_inserted_cb), popover);
      popover->row_deleted_id = g_signal_connect (popover->model, "row-deleted",
                                                  G_CALLBACK (row_deleted_cb), popover);
      popover->row_changed_id = g_signal_connect (popover->model, "row-changed",
                                                  G_CALLBACK (row_changed_cb), popover);
      popover->row_reordered_id = g_signal_connect (popover->model, "rows-reordered",
                                                    G_CALLBACK (row_reordered_cb), popover);
    }

  rebuild_menu (popover);
}

void
bobgui_tree_popover_set_row_separator_func (BobguiTreePopover              *popover,
                                         BobguiTreeViewRowSeparatorFunc  func,
                                         gpointer                     data,
                                         GDestroyNotify               destroy)
{
  if (popover->row_separator_destroy)
    popover->row_separator_destroy (popover->row_separator_data);

  popover->row_separator_func = func;
  popover->row_separator_data = data;
  popover->row_separator_destroy = destroy;

  rebuild_menu (popover);
}

static void
bobgui_tree_popover_set_active_item (BobguiTreePopover *popover,
                                  BobguiWidget      *item)
{
  if (popover->active_item == item)
    return;

  if (popover->active_item)
    {
      bobgui_widget_unset_state_flags (popover->active_item, BOBGUI_STATE_FLAG_SELECTED);
      g_object_remove_weak_pointer (G_OBJECT (popover->active_item), (gpointer *)&popover->active_item);
    }

  popover->active_item = item;

  if (popover->active_item)
    {
      g_object_add_weak_pointer (G_OBJECT (popover->active_item), (gpointer *)&popover->active_item);
      bobgui_widget_set_state_flags (popover->active_item, BOBGUI_STATE_FLAG_SELECTED, FALSE);
    }
}

void
bobgui_tree_popover_set_active (BobguiTreePopover *popover,
                             int             item)
{
  BobguiWidget *box;
  BobguiWidget *child;
  int pos;

  if (item == -1)
    {
      bobgui_tree_popover_set_active_item (popover, NULL);
      return;
    }

  box = bobgui_tree_popover_get_submenu (popover, "main");
  if (!box)
    return;

  for (child = bobgui_widget_get_first_child (box), pos = 0;
       child;
       child = bobgui_widget_get_next_sibling (child), pos++)
    {
      if (pos == item)
        {
          bobgui_tree_popover_set_active_item (popover, child);
          break;
        }
    }
}

