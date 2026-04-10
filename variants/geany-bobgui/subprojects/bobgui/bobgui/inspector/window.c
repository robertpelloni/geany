/*
 * Copyright (c) 2008-2009  Christian Hammond
 * Copyright (c) 2008-2009  David Trowbridge
 * Copyright (c) 2013 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#include <stdlib.h>

#include "init.h"
#include "window.h"
#include "prop-list.h"
#include "clipboard.h"
#include "controllers.h"
#include "css-editor.h"
#include "css-node-tree.h"
#include "object-tree.h"
#include "size-groups.h"
#include "a11y.h"
#include "actions.h"
#include "shortcuts.h"
#include "list-data.h"
#include "menu.h"
#include "misc-info.h"
#include "magnifier.h"
#include "recorder.h"
#include "svg.h"
#include "tree-data.h"
#include "visual.h"
#include "general.h"
#include "logs.h"

#include "gdkdebugprivate.h"
#include "gdkmarshalers.h"
#include "gskrendererprivate.h"
#include "bobguibutton.h"
#include "bobguicsswidgetnodeprivate.h"
#include "bobguilabel.h"
#include "bobguimodulesprivate.h"
#include "bobguiprivate.h"
#include "bobguinative.h"
#include "bobguistack.h"
#include "bobguiwindowgroup.h"
#include "bobguirevealer.h"
#include "bobguilayoutmanager.h"
#include "bobguicssprovider.h"
#include "bobguiwidgetprivate.h"


enum {
  PROP_INSPECTED_DISPLAY = 1,
  NUM_PROPERTIES
};

static GParamSpec *properties[NUM_PROPERTIES];

enum {
  EVENT,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];


G_DEFINE_TYPE (BobguiInspectorWindow, bobgui_inspector_window, BOBGUI_TYPE_WINDOW)


/* Fast way of knowing that further checks are necessary because at least
 * one inspector window has been constructed. */
static gboolean any_inspector_window_constructed = FALSE;


static gboolean
set_selected_object (BobguiInspectorWindow *iw,
                     GObject            *selected)
{
  GList *l;
  char *title;

  if (!bobgui_inspector_prop_list_set_object (BOBGUI_INSPECTOR_PROP_LIST (iw->prop_list), selected))
    return FALSE;

  title = bobgui_inspector_get_object_title (selected);
  bobgui_label_set_label (BOBGUI_LABEL (iw->object_title), title);
  g_free (title);

  bobgui_inspector_prop_list_set_layout_child (BOBGUI_INSPECTOR_PROP_LIST (iw->layout_prop_list), selected);
  bobgui_inspector_misc_info_set_object (BOBGUI_INSPECTOR_MISC_INFO (iw->misc_info), selected);
  bobgui_inspector_css_node_tree_set_object (BOBGUI_INSPECTOR_CSS_NODE_TREE (iw->widget_css_node_tree), selected);
  bobgui_inspector_size_groups_set_object (BOBGUI_INSPECTOR_SIZE_GROUPS (iw->size_groups), selected);
  bobgui_inspector_tree_data_set_object (BOBGUI_INSPECTOR_TREE_DATA (iw->tree_data), selected);
  bobgui_inspector_list_data_set_object (BOBGUI_INSPECTOR_LIST_DATA (iw->list_data), selected);
  bobgui_inspector_actions_set_object (BOBGUI_INSPECTOR_ACTIONS (iw->actions), selected);
  bobgui_inspector_shortcuts_set_object (BOBGUI_INSPECTOR_SHORTCUTS (iw->shortcuts), selected);
  bobgui_inspector_menu_set_object (BOBGUI_INSPECTOR_MENU (iw->menu), selected);
  bobgui_inspector_controllers_set_object (BOBGUI_INSPECTOR_CONTROLLERS (iw->controllers), selected);
  bobgui_inspector_magnifier_set_object (BOBGUI_INSPECTOR_MAGNIFIER (iw->magnifier), selected);
  bobgui_inspector_a11y_set_object (BOBGUI_INSPECTOR_A11Y (iw->a11y), selected);
  bobgui_inspector_svg_set_object (BOBGUI_INSPECTOR_SVG (iw->svg), selected);

  for (l = iw->extra_pages; l != NULL; l = l->next)
    g_object_set (l->data, "object", selected, NULL);

  return TRUE;
}

static void
on_object_activated (BobguiInspectorObjectTree *wt,
                     GObject                *selected,
                     BobguiInspectorWindow     *iw)
{
  if (BOBGUI_IS_WIDGET (selected))
    bobgui_inspector_window_set_object (iw, selected, CHILD_KIND_WIDGET, 0);
  else
    bobgui_inspector_window_set_object (iw, selected, CHILD_KIND_OTHER, 0);

  bobgui_stack_set_visible_child_name (BOBGUI_STACK (iw->object_stack), "object-details");
  bobgui_stack_set_visible_child_name (BOBGUI_STACK (iw->object_buttons), "details");
}

static void
on_object_selected (BobguiInspectorObjectTree *wt,
                    GObject                *selected,
                    BobguiInspectorWindow     *iw)
{
  bobgui_widget_set_sensitive (iw->object_details_button, selected != NULL);
  if (BOBGUI_IS_WIDGET (selected))
    bobgui_inspector_flash_widget (iw, BOBGUI_WIDGET (selected));
}

static void
notify_node (BobguiInspectorCssNodeTree *cnt,
             GParamSpec              *pspec,
             BobguiInspectorWindow      *iw)
{
  BobguiCssNode *node;
  BobguiWidget *widget = NULL;

  for (node = bobgui_inspector_css_node_tree_get_node (cnt);
       node != NULL;
       node = bobgui_css_node_get_parent (node))
    {
      if (!BOBGUI_IS_CSS_WIDGET_NODE (node))
        continue;

      widget = bobgui_css_widget_node_get_widget (BOBGUI_CSS_WIDGET_NODE (node));
      if (widget != NULL)
        break;
    }
  if (widget)
    bobgui_inspector_flash_widget (iw, widget);
}

static void
close_object_details (BobguiWidget *button, BobguiInspectorWindow *iw)
{
  bobgui_stack_set_visible_child_name (BOBGUI_STACK (iw->object_stack), "object-tree");
  bobgui_stack_set_visible_child_name (BOBGUI_STACK (iw->object_buttons), "list");
}

static void
open_object_details (BobguiWidget *button, BobguiInspectorWindow *iw)
{
  GObject *selected;

  selected = bobgui_inspector_object_tree_get_selected (BOBGUI_INSPECTOR_OBJECT_TREE (iw->object_tree));

  bobgui_inspector_window_set_object (iw, selected, CHILD_KIND_WIDGET, 0);

  bobgui_stack_set_visible_child_name (BOBGUI_STACK (iw->object_stack), "object-details");
  bobgui_stack_set_visible_child_name (BOBGUI_STACK (iw->object_buttons), "details");
}

static gboolean
translate_visible_child_name (GBinding     *binding,
                              const GValue *from,
                              GValue       *to,
                              gpointer      user_data)
{
  BobguiInspectorWindow *iw = user_data;
  const char *name;

  name = g_value_get_string (from);

  if (bobgui_stack_get_child_by_name (BOBGUI_STACK (iw->object_start_stack), name))
    g_value_set_string (to, name);
  else
    g_value_set_string (to, "empty");

  return TRUE;
}

typedef struct
{
  GObject *object;
  ChildKind kind;
  guint position;
} ChildData;

static void
bobgui_inspector_window_init (BobguiInspectorWindow *iw)
{
  GIOExtensionPoint *extension_point;
  GList *l, *extensions;

  iw->objects = g_array_new (FALSE, FALSE, sizeof (ChildData));

  bobgui_widget_init_template (BOBGUI_WIDGET (iw));

  g_object_bind_property_full (iw->object_details, "visible-child-name",
                               iw->object_start_stack, "visible-child-name",
                               G_BINDING_SYNC_CREATE,
                               translate_visible_child_name,
                               NULL,
                               iw,
                               NULL);

  bobgui_window_group_add_window (bobgui_window_group_new (), BOBGUI_WINDOW (iw));

  extension_point = g_io_extension_point_lookup ("bobgui-inspector-page");
  extensions = g_io_extension_point_get_extensions (extension_point);

  for (l = extensions; l != NULL; l = l->next)
    {
      GIOExtension *extension = l->data;
      GType type;
      BobguiWidget *widget;
      const char *name;
      char *title;
      BobguiWidget *button;
      gboolean use_picker;

      type = g_io_extension_get_type (extension);

      widget = g_object_new (type, NULL);

      iw->extra_pages = g_list_prepend (iw->extra_pages, widget);

      name = g_io_extension_get_name (extension);
      g_object_get (widget, "title", &title, NULL);

      if (g_object_class_find_property (G_OBJECT_GET_CLASS (widget), "use-picker"))
        g_object_get (widget, "use-picker", &use_picker, NULL);
      else
        use_picker = FALSE;

      if (use_picker)
        {
          button = bobgui_button_new_from_icon_name ("find-location-symbolic");
          bobgui_widget_set_focus_on_click (button, FALSE);
          bobgui_widget_set_halign (button, BOBGUI_ALIGN_START);
          bobgui_widget_set_valign (button, BOBGUI_ALIGN_CENTER);
          g_signal_connect (button, "clicked",
                            G_CALLBACK (bobgui_inspector_on_inspect), iw);
        }
      else
        button = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);

      bobgui_stack_add_titled (BOBGUI_STACK (iw->top_stack), widget, name, title);
      bobgui_stack_add_named (BOBGUI_STACK (iw->button_stack), button, name);

      g_free (title);
    }
}

static void
bobgui_inspector_window_constructed (GObject *object)
{
  BobguiInspectorWindow *iw = BOBGUI_INSPECTOR_WINDOW (object);

  G_OBJECT_CLASS (bobgui_inspector_window_parent_class)->constructed (object);

  g_object_set_data (G_OBJECT (iw->inspected_display), "-bobgui-inspector", iw);
  any_inspector_window_constructed = TRUE;

  bobgui_inspector_object_tree_set_display (BOBGUI_INSPECTOR_OBJECT_TREE (iw->object_tree), iw->inspected_display);
  bobgui_inspector_css_editor_set_display (BOBGUI_INSPECTOR_CSS_EDITOR (iw->css_editor), iw->inspected_display);
  bobgui_inspector_visual_set_display (BOBGUI_INSPECTOR_VISUAL (iw->visual), iw->inspected_display);
  bobgui_inspector_general_set_display (BOBGUI_INSPECTOR_GENERAL (iw->general), iw->inspected_display);
  bobgui_inspector_clipboard_set_display (BOBGUI_INSPECTOR_CLIPBOARD (iw->clipboard), iw->inspected_display);
  bobgui_inspector_logs_set_display (BOBGUI_INSPECTOR_LOGS (iw->logs), iw->inspected_display);
  bobgui_inspector_css_node_tree_set_display (BOBGUI_INSPECTOR_CSS_NODE_TREE (iw->widget_css_node_tree), iw->inspected_display);
}

static void
bobgui_inspector_window_dispose (GObject *object)
{
  BobguiInspectorWindow *iw = BOBGUI_INSPECTOR_WINDOW (object);

  g_object_set_data (G_OBJECT (iw->inspected_display), "-bobgui-inspector", NULL);

  g_clear_object (&iw->flash_overlay);
  g_clear_pointer (&iw->objects, g_array_unref);

  bobgui_widget_dispose_template (BOBGUI_WIDGET (iw), BOBGUI_TYPE_INSPECTOR_WINDOW);

  G_OBJECT_CLASS (bobgui_inspector_window_parent_class)->dispose (object);
}

static void
object_details_changed (BobguiWidget          *combo,
                        GParamSpec         *pspec,
                        BobguiInspectorWindow *iw)
{
  bobgui_stack_set_visible_child_name (BOBGUI_STACK (iw->object_center_stack), "title");
}

static void
go_up_cb (BobguiWidget          *button,
          BobguiInspectorWindow *iw)
{
  if (iw->objects->len > 1)
    {
      bobgui_inspector_window_pop_object (iw);
      return;
    }
  else if (iw->objects->len > 0)
    {
      ChildData *data = &g_array_index (iw->objects, ChildData, 0);
      BobguiWidget *widget = (BobguiWidget *)data->object;
      if (BOBGUI_IS_WIDGET (widget) && bobgui_widget_get_parent (widget))
        {
          GObject *obj = G_OBJECT (bobgui_widget_get_parent (widget));
          bobgui_inspector_window_replace_object (iw, obj, CHILD_KIND_WIDGET, 0);
          return;
        }
    }

  bobgui_widget_error_bell (BOBGUI_WIDGET (iw));
}

static void
go_down_cb (BobguiWidget          *button,
            BobguiInspectorWindow *iw)
{
  ChildData *data;
  GObject *object;

  if (iw->objects->len < 1)
    {
      bobgui_widget_error_bell (BOBGUI_WIDGET (iw));
      return;
    }

  data = &g_array_index (iw->objects, ChildData, iw->objects->len - 1);
  object = data->object;

  if (BOBGUI_IS_WIDGET (object))
    {
      BobguiWidget *child = bobgui_widget_get_first_child (BOBGUI_WIDGET (object));

      if (child)
        {
          bobgui_inspector_window_push_object (iw, G_OBJECT (child), CHILD_KIND_WIDGET, 0);
          return;
        }
    }
  else if (G_IS_LIST_MODEL (object))
    {
      GObject *item = g_list_model_get_item (G_LIST_MODEL (object), 0);
      if (item)
        {
          bobgui_inspector_window_push_object (iw, item, CHILD_KIND_LISTITEM, 0);
          g_object_unref (item);
          return;
        }
    }

  bobgui_widget_error_bell (BOBGUI_WIDGET (iw));
}

static void
go_previous_cb (BobguiWidget          *button,
                BobguiInspectorWindow *iw)
{
  ChildData *data;
  GObject *object;
  GObject *parent;

  if (iw->objects->len < 1)
    {
      bobgui_widget_error_bell (BOBGUI_WIDGET (iw));
      return;
    }

  if (iw->objects->len > 1)
    {
      data = &g_array_index (iw->objects, ChildData, iw->objects->len - 2);
      parent = data->object;
    }
  else
    parent = NULL;

  data = &g_array_index (iw->objects, ChildData, iw->objects->len - 1);
  object = data->object;

  switch (data->kind)
    {
    case CHILD_KIND_WIDGET:
      {
        BobguiWidget *sibling = bobgui_widget_get_prev_sibling (BOBGUI_WIDGET (object));
        if (sibling)
          {
            bobgui_inspector_window_replace_object (iw, (GObject*)sibling, CHILD_KIND_WIDGET, 0);
            return;
          }
      }
      break;

    case CHILD_KIND_LISTITEM:
      {
        GObject *item;

        if (parent && data->position > 0)
          item = g_list_model_get_item (G_LIST_MODEL (parent), data->position - 1);
        else
          item = NULL;

        if (item)
          {
            bobgui_inspector_window_replace_object (iw, item, CHILD_KIND_LISTITEM, data->position - 1);
            g_object_unref (item);
            return;
          }
      }
      break;

    case CHILD_KIND_CONTROLLER:
    case CHILD_KIND_PROPERTY:
    case CHILD_KIND_OTHER:
    default: ;
    }

  bobgui_widget_error_bell (BOBGUI_WIDGET (iw));
}

static void
go_next_cb (BobguiWidget          *button,
            BobguiInspectorWindow *iw)
{
  ChildData *data;
  GObject *object;
  GObject *parent;

  if (iw->objects->len < 1)
    {
      bobgui_widget_error_bell (BOBGUI_WIDGET (iw));
      return;
    }

  if (iw->objects->len > 1)
    {
      data = &g_array_index (iw->objects, ChildData, iw->objects->len - 2);
      parent = data->object;
    }
  else
    parent = NULL;

  data = &g_array_index (iw->objects, ChildData, iw->objects->len - 1);
  object = data->object;

  switch (data->kind)
    {
    case CHILD_KIND_WIDGET:
      {
        BobguiWidget *sibling = bobgui_widget_get_next_sibling (BOBGUI_WIDGET (object));
        if (sibling)
          {
            bobgui_inspector_window_replace_object (iw, (GObject*)sibling, CHILD_KIND_WIDGET, 0);
            return;
          }
      }
      break;

    case CHILD_KIND_LISTITEM:
      {
        GObject *item;

        if (parent &&
            data->position + 1 < g_list_model_get_n_items (G_LIST_MODEL (parent)))
          item = g_list_model_get_item (G_LIST_MODEL (parent), data->position + 1);
        else
          item = NULL;

        if (item)
          {
            bobgui_inspector_window_replace_object (iw, item, CHILD_KIND_LISTITEM, data->position + 1);
            g_object_unref (item);
            return;
          }
      }
      break;

    case CHILD_KIND_CONTROLLER:
    case CHILD_KIND_PROPERTY:
    case CHILD_KIND_OTHER:
    default: ;
    }

  bobgui_widget_error_bell (BOBGUI_WIDGET (iw));
}

static void
bobgui_inspector_window_realize (BobguiWidget *widget)
{
  GskRenderer *renderer;
  BobguiCssProvider *provider;

  BOBGUI_WIDGET_CLASS (bobgui_inspector_window_parent_class)->realize (widget);

  renderer = bobgui_native_get_renderer (BOBGUI_NATIVE (widget));
  gsk_renderer_set_debug_flags (renderer, 0);

  provider = bobgui_css_provider_new ();
  bobgui_css_provider_load_from_resource (provider, "/org/bobgui/libbobgui/inspector/inspector.css");
  bobgui_style_context_add_provider_for_display (bobgui_widget_get_display (widget),
                                              BOBGUI_STYLE_PROVIDER (provider),
                                              800);
  g_object_unref (provider);
}

static void
bobgui_inspector_window_set_property (GObject      *object,
                                   guint         prop_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  BobguiInspectorWindow *iw = BOBGUI_INSPECTOR_WINDOW (object);

  switch (prop_id)
    {
    case PROP_INSPECTED_DISPLAY:
      iw->inspected_display = g_value_get_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_inspector_window_get_property (GObject    *object,
                                   guint       prop_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  BobguiInspectorWindow *iw = BOBGUI_INSPECTOR_WINDOW (object);

  switch (prop_id)
    {
    case PROP_INSPECTED_DISPLAY:
      g_value_set_object (value, iw->inspected_display);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static gboolean
bobgui_inspector_window_enable_debugging (BobguiWindow *window,
                                       gboolean   toggle)
{
  return FALSE;
}

static void
force_one_full_redraw (BobguiWidget *w)
{
  bobgui_widget_queue_draw (w);
  for (w = bobgui_widget_get_first_child (w); w; w = bobgui_widget_get_next_sibling (w))
    force_one_full_redraw (w);
}

static void
force_full_redraw (BobguiInspectorWindow *window)
{
  GListModel *toplevels;

  toplevels = bobgui_window_get_toplevels ();
  for (unsigned int i = 0; i < g_list_model_get_n_items (toplevels); i++)
    {
      BobguiWidget *w = BOBGUI_WIDGET (g_list_model_get_item (toplevels, i));

      if (bobgui_widget_get_display (w) == window->inspected_display)
        force_one_full_redraw (w);

      g_object_unref (w);
    }
}

static void
bobgui_inspector_window_class_init (BobguiInspectorWindowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);
  BobguiWindowClass *window_class = BOBGUI_WINDOW_CLASS (klass);

  object_class->constructed = bobgui_inspector_window_constructed;
  object_class->dispose = bobgui_inspector_window_dispose;
  object_class->set_property = bobgui_inspector_window_set_property;
  object_class->get_property = bobgui_inspector_window_get_property;

  widget_class->realize = bobgui_inspector_window_realize;

  window_class->enable_debugging = bobgui_inspector_window_enable_debugging;

  properties[PROP_INSPECTED_DISPLAY] =
      g_param_spec_object ("inspected-display", NULL, NULL,
                           GDK_TYPE_DISPLAY,
                           G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);
  g_object_class_install_properties (object_class, NUM_PROPERTIES, properties);

  signals[EVENT] = g_signal_new (g_intern_static_string ("event"),
                                 G_OBJECT_CLASS_TYPE (object_class),
                                 G_SIGNAL_RUN_LAST,
                                 0,
                                 g_signal_accumulator_true_handled,
                                 NULL,
                                 _gdk_marshal_BOOLEAN__POINTER,
                                 G_TYPE_BOOLEAN,
                                 1,
                                 GDK_TYPE_EVENT);
  g_signal_set_va_marshaller (signals[EVENT],
                              G_OBJECT_CLASS_TYPE (object_class),
                              _gdk_marshal_BOOLEAN__POINTERv);

  bobgui_widget_class_set_template_from_resource (widget_class, "/org/bobgui/libbobgui/inspector/window.ui");

  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorWindow, top_stack);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorWindow, button_stack);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorWindow, object_stack);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorWindow, object_tree);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorWindow, object_details);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorWindow, object_start_stack);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorWindow, object_center_stack);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorWindow, object_buttons);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorWindow, object_details_button);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorWindow, select_object);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorWindow, prop_list);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorWindow, layout_prop_list);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorWindow, widget_css_node_tree);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorWindow, widget_recorder);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorWindow, object_title);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorWindow, size_groups);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorWindow, tree_data);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorWindow, list_data);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorWindow, actions);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorWindow, shortcuts);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorWindow, menu);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorWindow, misc_info);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorWindow, controllers);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorWindow, magnifier);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorWindow, a11y);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorWindow, svg);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorWindow, sidebar_revealer);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorWindow, css_editor);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorWindow, visual);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorWindow, general);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorWindow, clipboard);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorWindow, logs);

  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorWindow, go_up_button);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorWindow, go_down_button);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorWindow, go_previous_button);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorWindow, list_position_label);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorWindow, go_next_button);

  bobgui_widget_class_bind_template_callback (widget_class, bobgui_inspector_on_inspect);
  bobgui_widget_class_bind_template_callback (widget_class, on_object_activated);
  bobgui_widget_class_bind_template_callback (widget_class, on_object_selected);
  bobgui_widget_class_bind_template_callback (widget_class, open_object_details);
  bobgui_widget_class_bind_template_callback (widget_class, close_object_details);
  bobgui_widget_class_bind_template_callback (widget_class, object_details_changed);
  bobgui_widget_class_bind_template_callback (widget_class, notify_node);
  bobgui_widget_class_bind_template_callback (widget_class, go_previous_cb);
  bobgui_widget_class_bind_template_callback (widget_class, go_up_cb);
  bobgui_widget_class_bind_template_callback (widget_class, go_down_cb);
  bobgui_widget_class_bind_template_callback (widget_class, go_next_cb);
  bobgui_widget_class_bind_template_callback (widget_class, force_full_redraw);
}

static GdkDisplay *
get_inspector_display (void)
{
  GdkDisplay *display;
  const char *name;

  name = g_getenv ("BOBGUI_INSPECTOR_DISPLAY");
  display = gdk_display_open (name);

  if (display)
    g_debug ("Using display %s for BobguiInspector", name);
  else
    g_message ("Failed to open display %s", name);

  if (!display)
    {
      display = gdk_display_open (NULL);
      if (display)
        g_debug ("Using default display for BobguiInspector");
      else
        g_message ("Failed to open separate connection to default display");
    }


  if (display)
    {
      name = g_getenv ("BOBGUI_INSPECTOR_RENDERER");

      g_object_set_data_full (G_OBJECT (display), "gsk-renderer",
                              g_strdup (name), g_free);
    }

  if (!display)
    display = gdk_display_get_default ();

  if (display == gdk_display_get_default ())
    g_message ("Using default display for BobguiInspector; expect some spillover");

  return display;
}

static BobguiInspectorWindow *
bobgui_inspector_window_new (GdkDisplay *display)
{
  BobguiInspectorWindow *iw;

  iw = g_object_new (BOBGUI_TYPE_INSPECTOR_WINDOW,
                     "display", get_inspector_display (),
                     "inspected-display", display,
                     NULL);

  return iw;
}

BobguiWidget *
bobgui_inspector_window_get (GdkDisplay *display)
{
  BobguiWidget *iw;

  bobgui_inspector_init ();

  iw = BOBGUI_WIDGET (g_object_get_data (G_OBJECT (display), "-bobgui-inspector"));

  if (!iw)
    iw = BOBGUI_WIDGET (bobgui_inspector_window_new (display));

  return iw;
}

void
bobgui_inspector_window_add_overlay (BobguiInspectorWindow  *iw,
                                  BobguiInspectorOverlay *overlay)
{
  iw->overlays = g_list_prepend (iw->overlays, g_object_ref (overlay));

  bobgui_inspector_overlay_queue_draw (overlay);
}

void
bobgui_inspector_window_remove_overlay (BobguiInspectorWindow  *iw,
                                     BobguiInspectorOverlay *overlay)
{
  GList *item;

  item = g_list_find (iw->overlays, overlay);
  if (item == NULL)
    return;

  bobgui_inspector_overlay_queue_draw (overlay);

  iw->overlays = g_list_delete_link (iw->overlays, item);
  g_object_unref (overlay);
}

static BobguiInspectorWindow *
bobgui_inspector_window_get_for_display (GdkDisplay *display)
{
  return g_object_get_data (G_OBJECT (display), "-bobgui-inspector");
}

GskRenderNode *
bobgui_inspector_prepare_render (BobguiWidget            *widget,
                              GskRenderer          *renderer,
                              GdkSurface           *surface,
                              const cairo_region_t *region,
                              GskRenderNode        *root,
                              GskRenderNode        *widget_node)
{
  BobguiInspectorWindow *iw;

  iw = bobgui_inspector_window_get_for_display (bobgui_widget_get_display (widget));
  if (iw == NULL)
    return root;

  /* sanity check for single-display GDK backends */
  if (BOBGUI_WIDGET (iw) == widget)
    return root;

  bobgui_inspector_recorder_record_render (BOBGUI_INSPECTOR_RECORDER (iw->widget_recorder),
                                        widget,
                                        renderer,
                                        surface,
                                        region,
                                        root);

  if (iw->overlays)
    {
      BobguiSnapshot *snapshot;
      GList *l;
      double native_x, native_y;

      snapshot = bobgui_snapshot_new ();
      bobgui_snapshot_append_node (snapshot, root);

      bobgui_native_get_surface_transform (BOBGUI_NATIVE (widget), &native_x, &native_y);

      bobgui_snapshot_save (snapshot);
      bobgui_snapshot_translate (snapshot, &(graphene_point_t) { native_x, native_y });

      for (l = iw->overlays; l; l = l->next)
        {
          bobgui_inspector_overlay_snapshot (l->data, snapshot, widget_node, widget);
        }

      bobgui_snapshot_restore (snapshot);

      gsk_render_node_unref (root);
      root = bobgui_snapshot_free_to_node (snapshot);
    }

  return root;
}

gboolean
bobgui_inspector_is_recording (BobguiWidget *widget)
{
  BobguiInspectorWindow *iw;

  if (!any_inspector_window_constructed)
    return FALSE;

  iw = bobgui_inspector_window_get_for_display (bobgui_widget_get_display (widget));
  if (iw == NULL)
    return FALSE;

  /* sanity check for single-display GDK backends */
  if (BOBGUI_WIDGET (iw) == widget)
    return FALSE;

  return bobgui_inspector_recorder_is_recording (BOBGUI_INSPECTOR_RECORDER (iw->widget_recorder));
}

gboolean
bobgui_inspector_handle_event (GdkEvent *event)
{
  BobguiInspectorWindow *iw;
  gboolean handled = FALSE;

  if (!any_inspector_window_constructed)
    return FALSE;

  iw = bobgui_inspector_window_get_for_display (gdk_event_get_display (event));
  if (iw == NULL)
    return FALSE;

  if (GDK_IS_EVENT_TYPE (event, GDK_KEY_PRESS))
    {
      BobguiInspectorRecorder *recorder = BOBGUI_INSPECTOR_RECORDER (iw->widget_recorder);

      if (gdk_key_event_matches (event, GDK_KEY_r, GDK_SUPER_MASK) == GDK_KEY_MATCH_EXACT)
        {
          gboolean recording = bobgui_inspector_recorder_is_recording (recorder);

          bobgui_inspector_recorder_set_recording (recorder, !recording);
          return TRUE;
        }
      else if (gdk_key_event_matches (event, GDK_KEY_c, GDK_SUPER_MASK) == GDK_KEY_MATCH_EXACT)
        {
          bobgui_inspector_recorder_record_single_frame (recorder, FALSE);
          return TRUE;
        }
      else if (gdk_key_event_matches (event, GDK_KEY_f, GDK_SUPER_MASK) == GDK_KEY_MATCH_EXACT)
        {
          bobgui_inspector_recorder_record_single_frame (recorder, TRUE);
          return TRUE;
        }
    }

  bobgui_inspector_recorder_record_event (BOBGUI_INSPECTOR_RECORDER (iw->widget_recorder),
                                       bobgui_get_event_widget (event),
                                       event);

  g_signal_emit (iw, signals[EVENT], 0, event, &handled);

  return handled;
}

void
bobgui_inspector_trace_event (GdkEvent            *event,
                           BobguiPropagationPhase  phase,
                           BobguiWidget           *widget,
                           BobguiEventController  *controller,
                           BobguiWidget           *target,
                           gboolean             handled)
{
  BobguiInspectorWindow *iw;

  if (!any_inspector_window_constructed)
    return;

  iw = bobgui_inspector_window_get_for_display (gdk_event_get_display (event));
  if (iw == NULL)
    return;

  bobgui_inspector_recorder_trace_event (BOBGUI_INSPECTOR_RECORDER (iw->widget_recorder),
                                      event, phase, widget, controller, target, handled);
}

GdkDisplay *
bobgui_inspector_window_get_inspected_display (BobguiInspectorWindow *iw)
{
  return iw->inspected_display;
}

static void
update_go_button (BobguiWidget  *button,
                  gboolean    enabled,
                  const char *tooltip)
{
  bobgui_widget_set_sensitive (button, enabled);
  bobgui_widget_set_tooltip_text (button, tooltip);
}

static void
update_go_buttons (BobguiInspectorWindow *iw)
{
  GObject *parent;
  GObject *object;
  ChildKind kind;
  guint position;

  if (iw->objects->len > 1)
    {
      ChildData *data = &g_array_index (iw->objects, ChildData, iw->objects->len - 2);
      parent = data->object;
    }
  else
    {
      parent = NULL;
    }

  if (iw->objects->len > 0)
    {
      ChildData *data = &g_array_index (iw->objects, ChildData, iw->objects->len - 1);
      object = data->object;
      kind = data->kind;
      position = data->position;
    }
  else
    {
      object = NULL;
      kind = CHILD_KIND_OTHER;
      position = 0;
    }

  if (parent)
    {
      char *text;
      text = g_strdup_printf ("Go to %s", G_OBJECT_TYPE_NAME (parent));
      update_go_button (iw->go_up_button, TRUE, text);
      g_free (text);
    }
  else
    {
      update_go_button (iw->go_up_button, BOBGUI_IS_WIDGET (object) && !BOBGUI_IS_ROOT (object), "Parent widget");
    }

  switch (kind)
    {
    case CHILD_KIND_WIDGET:
      update_go_button (iw->go_down_button,
                        BOBGUI_IS_WIDGET (object) &&bobgui_widget_get_first_child (BOBGUI_WIDGET (object)) != NULL,
                        "First child");
      update_go_button (iw->go_previous_button,
                        BOBGUI_IS_WIDGET (object) && bobgui_widget_get_prev_sibling (BOBGUI_WIDGET (object)) != NULL,
                        "Previous sibling");
      update_go_button (iw->go_next_button,
                        BOBGUI_IS_WIDGET (object) && bobgui_widget_get_next_sibling (BOBGUI_WIDGET (object)) != NULL,
                        "Next sibling");
      bobgui_widget_set_visible (iw->list_position_label, FALSE);
      break;
    case CHILD_KIND_LISTITEM:
      update_go_button (iw->go_down_button, FALSE, NULL);
      update_go_button (iw->go_previous_button, position > 0, "Previous list item");
      update_go_button (iw->go_next_button, position + 1 < g_list_model_get_n_items (G_LIST_MODEL (parent)), "Next list item");
      {
        char *text = g_strdup_printf ("%u", position);
        bobgui_label_set_label (BOBGUI_LABEL (iw->list_position_label), text);
        g_free (text);
        bobgui_widget_set_visible (iw->list_position_label, TRUE);
      }
      break;
    case CHILD_KIND_PROPERTY:
    case CHILD_KIND_CONTROLLER:
    case CHILD_KIND_OTHER:
      update_go_button (iw->go_down_button, FALSE, NULL);
      update_go_button (iw->go_previous_button, FALSE, NULL);
      update_go_button (iw->go_next_button, FALSE, NULL);
      bobgui_widget_set_visible (iw->list_position_label, FALSE);
      break;
    default:
      g_assert_not_reached ();
      break;
    }
}

static void
show_object_details (BobguiInspectorWindow *iw,
                     GObject            *object,
                     const char         *tab)
{
  set_selected_object (iw, object);

  if (tab)
    bobgui_stack_set_visible_child_name (BOBGUI_STACK (iw->object_details), tab);
  if (!bobgui_stack_get_visible_child_name (BOBGUI_STACK (iw->object_details)))
    bobgui_stack_set_visible_child_name (BOBGUI_STACK (iw->object_details), "properties");

  bobgui_stack_set_visible_child_name (BOBGUI_STACK (iw->object_stack), "object-details");
  bobgui_stack_set_visible_child_name (BOBGUI_STACK (iw->object_buttons), "details");
}

void
bobgui_inspector_window_push_object (BobguiInspectorWindow *iw,
                                  GObject            *object,
                                  ChildKind           kind,
                                  guint               position)
{
  ChildData data;

  data.kind = kind;
  data.object = object;
  data.position = position;
  g_array_append_val (iw->objects, data);
  show_object_details (iw, object, "properties");
  update_go_buttons (iw);
}

void
bobgui_inspector_window_pop_object (BobguiInspectorWindow *iw)
{
  ChildData *data;
  const char *tabs[] = {
    "properties",
    "controllers",
    "properties",
    "list-data",
    "misc",
  };
  const char *tab;

  if (iw->objects->len < 2)
    {
      bobgui_widget_error_bell (BOBGUI_WIDGET (iw));
      return;
    }

  data = &g_array_index (iw->objects, ChildData, iw->objects->len - 1);
  tab = tabs[data->kind];
  g_array_remove_index (iw->objects, iw->objects->len - 1);
  data = &g_array_index (iw->objects, ChildData, iw->objects->len - 1);
  show_object_details (iw, data->object, tab);
  update_go_buttons (iw);
}

void
bobgui_inspector_window_replace_object (BobguiInspectorWindow *iw,
                                     GObject            *object,
                                     ChildKind           kind,
                                     guint               position)
{
  ChildData *data;

  data = &g_array_index (iw->objects, ChildData, iw->objects->len - 1);
  g_assert (data->kind == kind);
  data->object = object;
  data->position = position;
  show_object_details (iw, object, NULL);
  update_go_buttons (iw);
}

void
bobgui_inspector_window_set_object (BobguiInspectorWindow *iw,
                                 GObject            *object,
                                 ChildKind           kind,
                                 guint               position)
{
  g_array_set_size (iw->objects, 0);
  bobgui_inspector_window_push_object (iw, object, kind, position);
  update_go_buttons (iw);
}

void
bobgui_inspector_add_profile_node (GdkDisplay    *display,
                                GskRenderNode *node,
                                GskRenderNode *profile_node)
{
  BobguiInspectorWindow *iw;

  if (!any_inspector_window_constructed)
    return;

  iw = bobgui_inspector_window_get_for_display (display);
  if (iw == NULL)
    return;

  bobgui_inspector_recorder_add_profile_node (BOBGUI_INSPECTOR_RECORDER (iw->widget_recorder),
                                           node,
                                           profile_node);
}

// vim: set et sw=2 ts=2:
