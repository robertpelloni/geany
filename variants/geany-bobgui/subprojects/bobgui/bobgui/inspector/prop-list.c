/*
 * Copyright (c) 2008-2009  Christian Hammond
 * Copyright (c) 2008-2009  David Trowbridge
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

#include "prop-list.h"

#include "prop-editor.h"

#include "bobguipopover.h"
#include "bobguisearchentry.h"
#include "bobguilabel.h"
#include "bobguimain.h"
#include "bobguistack.h"
#include "bobguieventcontrollerkey.h"
#include "bobguilayoutmanager.h"
#include "bobguisizegroup.h"
#include "bobguiroot.h"
#include "prop-holder.h"
#include "window.h"

enum
{
  PROP_0,
  PROP_SEARCH_ENTRY
};

struct _BobguiInspectorPropListPrivate
{
  GObject *object;
  gulong notify_handler_id;
  BobguiWidget *search_entry;
  BobguiWidget *search_stack;
  BobguiWidget *list;
  BobguiStringFilter *filter;
  BobguiColumnViewColumn *name;
  BobguiColumnViewColumn *type;
  BobguiColumnViewColumn *origin;
};

G_DEFINE_TYPE_WITH_PRIVATE (BobguiInspectorPropList, bobgui_inspector_prop_list, BOBGUI_TYPE_BOX)

static void
search_close_clicked (BobguiWidget            *button,
                      BobguiInspectorPropList *pl)
{
  bobgui_editable_set_text (BOBGUI_EDITABLE (pl->priv->search_entry), "");
  bobgui_stack_set_visible_child_name (BOBGUI_STACK (pl->priv->search_stack), "title");
}

static void
show_search_entry (BobguiInspectorPropList *pl)
{
  bobgui_stack_set_visible_child (BOBGUI_STACK (pl->priv->search_stack),
                               pl->priv->search_entry);
}

static char *
holder_prop (gpointer item)
{
  GParamSpec *prop = prop_holder_get_pspec (PROP_HOLDER (item));

  return g_strdup (prop->name);
}

static char *
holder_type (gpointer item)
{
  GParamSpec *prop = prop_holder_get_pspec (PROP_HOLDER (item));

  return g_strdup (g_type_name (prop->value_type));
}

static char *
holder_origin (gpointer item)
{
  GParamSpec *prop = prop_holder_get_pspec (PROP_HOLDER (item));

  return g_strdup (g_type_name (prop->owner_type));
}

static void
bobgui_inspector_prop_list_init (BobguiInspectorPropList *pl)
{
  BobguiSorter *sorter;

  pl->priv = bobgui_inspector_prop_list_get_instance_private (pl);
  bobgui_widget_init_template (BOBGUI_WIDGET (pl));
  pl->priv->filter = bobgui_string_filter_new (NULL);
  bobgui_string_filter_set_match_mode (pl->priv->filter, BOBGUI_STRING_FILTER_MATCH_MODE_SUBSTRING);

  sorter = BOBGUI_SORTER (bobgui_string_sorter_new (bobgui_cclosure_expression_new (G_TYPE_STRING, NULL,
                                                               0, NULL,
                                                               (GCallback)holder_prop,
                                                               NULL, NULL)));
 
  bobgui_string_filter_set_expression (pl->priv->filter,
                                    bobgui_string_sorter_get_expression (BOBGUI_STRING_SORTER (sorter)));

  bobgui_column_view_column_set_sorter (pl->priv->name, sorter);
  g_object_unref (sorter);

  sorter = BOBGUI_SORTER (bobgui_string_sorter_new (bobgui_cclosure_expression_new (G_TYPE_STRING, NULL,
                                                               0, NULL,
                                                               (GCallback)holder_type,
                                                               NULL, NULL)));

  bobgui_column_view_column_set_sorter (pl->priv->type, sorter);
  g_object_unref (sorter);

  sorter = BOBGUI_SORTER (bobgui_string_sorter_new (bobgui_cclosure_expression_new (G_TYPE_STRING, NULL,
                                                               0, NULL,
                                                               (GCallback)holder_origin,
                                                               NULL, NULL)));

  bobgui_column_view_column_set_sorter (pl->priv->origin, sorter);
  g_object_unref (sorter);
}

static void
get_property (GObject    *object,
              guint       param_id,
              GValue     *value,
              GParamSpec *pspec)
{
  BobguiInspectorPropList *pl = BOBGUI_INSPECTOR_PROP_LIST (object);

  switch (param_id)
    {
      case PROP_SEARCH_ENTRY:
        g_value_take_object (value, pl->priv->search_entry);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
        break;
    }
}

static void
set_property (GObject      *object,
              guint         param_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  BobguiInspectorPropList *pl = BOBGUI_INSPECTOR_PROP_LIST (object);

  switch (param_id)
    {
      case PROP_SEARCH_ENTRY:
        pl->priv->search_entry = g_value_get_object (value);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
        break;
    }
}

static void
show_object (BobguiInspectorPropEditor *editor,
             GObject                *object,
             const char             *name,
             const char             *tab,
             BobguiInspectorPropList   *pl)
{
  BobguiInspectorWindow *iw;

  iw = BOBGUI_INSPECTOR_WINDOW (bobgui_widget_get_ancestor (BOBGUI_WIDGET (pl), BOBGUI_TYPE_INSPECTOR_WINDOW));
  bobgui_inspector_window_push_object (iw, object, CHILD_KIND_PROPERTY, 0);
}


static void cleanup_object (BobguiInspectorPropList *pl);

static void
finalize (GObject *object)
{
  BobguiInspectorPropList *pl = BOBGUI_INSPECTOR_PROP_LIST (object);

  cleanup_object (pl);

  G_OBJECT_CLASS (bobgui_inspector_prop_list_parent_class)->finalize (object);
}

static void
update_filter (BobguiInspectorPropList *pl,
               BobguiSearchEntry *entry)
{
  const char *text;

  text = bobgui_editable_get_text (BOBGUI_EDITABLE (entry));
  bobgui_string_filter_set_search (pl->priv->filter, text);
}

static void
constructed (GObject *object)
{
  BobguiInspectorPropList *pl = BOBGUI_INSPECTOR_PROP_LIST (object);

  pl->priv->search_stack = bobgui_widget_get_parent (pl->priv->search_entry);

  g_signal_connect (pl->priv->search_entry, "stop-search",
                    G_CALLBACK (search_close_clicked), pl);

  g_signal_connect_swapped (pl->priv->search_entry, "search-started",
                            G_CALLBACK (show_search_entry), pl);
  g_signal_connect_swapped (pl->priv->search_entry, "search-changed",
                            G_CALLBACK (update_filter), pl);
}

static void
update_key_capture (BobguiInspectorPropList *pl)
{
  BobguiWidget *capture_widget;

  if (bobgui_widget_get_mapped (BOBGUI_WIDGET (pl)))
    {
      BobguiWidget *toplevel;
      BobguiWidget *focus;

      toplevel = BOBGUI_WIDGET (bobgui_widget_get_root (BOBGUI_WIDGET (pl)));
      focus = bobgui_root_get_focus (BOBGUI_ROOT (toplevel));

      if (BOBGUI_IS_EDITABLE (focus) &&
          bobgui_widget_is_ancestor (focus, pl->priv->list))
        capture_widget = NULL;
      else
        capture_widget = toplevel;
    }
  else
    capture_widget = NULL;

  bobgui_search_entry_set_key_capture_widget (BOBGUI_SEARCH_ENTRY (pl->priv->search_entry),
                                           capture_widget);
}

static void
map (BobguiWidget *widget)
{
  BOBGUI_WIDGET_CLASS (bobgui_inspector_prop_list_parent_class)->map (widget);

  update_key_capture (BOBGUI_INSPECTOR_PROP_LIST (widget));
}

static void
unmap (BobguiWidget *widget)
{
  BOBGUI_WIDGET_CLASS (bobgui_inspector_prop_list_parent_class)->unmap (widget);

  update_key_capture (BOBGUI_INSPECTOR_PROP_LIST (widget));
}

static void
root (BobguiWidget *widget)
{
  BOBGUI_WIDGET_CLASS (bobgui_inspector_prop_list_parent_class)->root (widget);

  g_signal_connect_swapped (bobgui_widget_get_root (widget), "notify::focus-widget",
                            G_CALLBACK (update_key_capture), widget);
}

static void
unroot (BobguiWidget *widget)
{
  g_signal_handlers_disconnect_by_func (bobgui_widget_get_root (widget),
                                        update_key_capture, widget);

  BOBGUI_WIDGET_CLASS (bobgui_inspector_prop_list_parent_class)->unroot (widget);
}

static void
setup_name_cb (BobguiSignalListItemFactory *factory,
               BobguiListItem              *list_item)
{
  BobguiWidget *label;

  label = bobgui_label_new (NULL);
  bobgui_label_set_xalign (BOBGUI_LABEL (label), 0.0);
  bobgui_list_item_set_child (list_item, label);
  bobgui_widget_add_css_class (label, "cell");
}

static void
bind_name_cb (BobguiSignalListItemFactory *factory,
              BobguiListItem              *list_item)
{
  GObject *item;
  BobguiWidget *label;

  item = bobgui_list_item_get_item (list_item);
  label = bobgui_list_item_get_child (list_item);

  bobgui_label_set_label (BOBGUI_LABEL (label), prop_holder_get_name (PROP_HOLDER (item)));
}

static void
setup_type_cb (BobguiSignalListItemFactory *factory,
               BobguiListItem              *list_item)
{
  BobguiWidget *label;

  label = bobgui_label_new (NULL);
  bobgui_label_set_xalign (BOBGUI_LABEL (label), 0.0);
  bobgui_list_item_set_child (list_item, label);
  bobgui_widget_add_css_class (label, "cell");
}

static void
bind_type_cb (BobguiSignalListItemFactory *factory,
              BobguiListItem              *list_item)
{
  GObject *item;
  BobguiWidget *label;
  GParamSpec *prop;
  const char *type;

  item = bobgui_list_item_get_item (list_item);
  label = bobgui_list_item_get_child (list_item);

  prop = prop_holder_get_pspec (PROP_HOLDER (item));
  type = g_type_name (G_PARAM_SPEC_VALUE_TYPE (prop));

  bobgui_label_set_label (BOBGUI_LABEL (label), type);
}

static void
setup_origin_cb (BobguiSignalListItemFactory *factory,
                 BobguiListItem              *list_item)
{
  BobguiWidget *label;

  label = bobgui_label_new (NULL);
  bobgui_label_set_xalign (BOBGUI_LABEL (label), 0.0);
  bobgui_list_item_set_child (list_item, label);
  bobgui_widget_add_css_class (label, "cell");
}

static void
bind_origin_cb (BobguiSignalListItemFactory *factory,
                BobguiListItem              *list_item)
{
  GObject *item;
  BobguiWidget *label;
  GParamSpec *prop;
  const char *origin;

  item = bobgui_list_item_get_item (list_item);
  label = bobgui_list_item_get_child (list_item);

  prop = prop_holder_get_pspec (PROP_HOLDER (item));
  origin = g_type_name (prop->owner_type);

  bobgui_label_set_label (BOBGUI_LABEL (label), origin);
}

static void
setup_value_cb (BobguiSignalListItemFactory *factory,
                BobguiListItem              *list_item,
                gpointer                  data)
{
  BobguiWidget *widget;

  widget = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
  bobgui_widget_add_css_class (widget, "cell");
  bobgui_list_item_set_child (list_item, widget);
}

static void
bind_value_cb (BobguiSignalListItemFactory *factory,
               BobguiListItem              *list_item,
               gpointer                  data)
{
  GObject *item;
  BobguiWidget *editor;
  BobguiWidget *widget;
  GObject *object;
  const char *name;

  item = bobgui_list_item_get_item (list_item);

  object = prop_holder_get_object (PROP_HOLDER (item));
  name = prop_holder_get_name (PROP_HOLDER (item));

  editor = bobgui_inspector_prop_editor_new (object, name, NULL);
  g_signal_connect (editor, "show-object", G_CALLBACK (show_object), data);
  widget = bobgui_list_item_get_child (list_item);
  bobgui_box_append (BOBGUI_BOX (widget), editor);
}

static void
unbind_value_cb (BobguiSignalListItemFactory *factory,
                 BobguiListItem              *list_item,
                 gpointer                  data)
{
  BobguiWidget *widget;

  widget = bobgui_list_item_get_child (list_item);
  bobgui_box_remove (BOBGUI_BOX (widget), bobgui_widget_get_first_child (widget));
}

static void
bobgui_inspector_prop_list_class_init (BobguiInspectorPropListClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);

  object_class->finalize = finalize;
  object_class->get_property = get_property;
  object_class->set_property = set_property;
  object_class->constructed = constructed;

  widget_class->map = map;
  widget_class->unmap = unmap;
  widget_class->root = root;
  widget_class->unroot = unroot;

  g_object_class_install_property (object_class, PROP_SEARCH_ENTRY,
      g_param_spec_object ("search-entry", NULL, NULL,
                           BOBGUI_TYPE_WIDGET, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

  bobgui_widget_class_set_template_from_resource (widget_class, "/org/bobgui/libbobgui/inspector/prop-list.ui");
  bobgui_widget_class_bind_template_child_private (widget_class, BobguiInspectorPropList, list);
  bobgui_widget_class_bind_template_child_private (widget_class, BobguiInspectorPropList, name);
  bobgui_widget_class_bind_template_child_private (widget_class, BobguiInspectorPropList, type);
  bobgui_widget_class_bind_template_child_private (widget_class, BobguiInspectorPropList, origin);
  bobgui_widget_class_bind_template_callback (widget_class, setup_name_cb);
  bobgui_widget_class_bind_template_callback (widget_class, bind_name_cb);
  bobgui_widget_class_bind_template_callback (widget_class, setup_type_cb);
  bobgui_widget_class_bind_template_callback (widget_class, bind_type_cb);
  bobgui_widget_class_bind_template_callback (widget_class, setup_origin_cb);
  bobgui_widget_class_bind_template_callback (widget_class, bind_origin_cb);
  bobgui_widget_class_bind_template_callback (widget_class, setup_value_cb);
  bobgui_widget_class_bind_template_callback (widget_class, bind_value_cb);
  bobgui_widget_class_bind_template_callback (widget_class, unbind_value_cb);
}

/* Like g_strdup_value_contents, but keeps the type name separate */
void
strdup_value_contents (const GValue  *value,
                       char         **contents,
                       char         **type)
{
  const char *src;

  if (G_VALUE_HOLDS_STRING (value))
    {
      src = g_value_get_string (value);

      *type = g_strdup ("char*");

      if (!src)
        {
          *contents = g_strdup ("NULL");
        }
      else
        {
          char *s = g_strescape (src, NULL);
          *contents = g_strdup_printf ("\"%s\"", s);
          g_free (s);
        }
    }
  else if (g_value_type_transformable (G_VALUE_TYPE (value), G_TYPE_STRING))
    {
      GValue tmp_value = G_VALUE_INIT;

      *type = g_strdup (g_type_name (G_VALUE_TYPE (value)));

      g_value_init (&tmp_value, G_TYPE_STRING);
      g_value_transform (value, &tmp_value);
      src = g_value_get_string (&tmp_value);
      if (!src)
        *contents = g_strdup ("NULL");
      else
        *contents = g_strescape (src, NULL);
      g_value_unset (&tmp_value);
    }
  else if (g_value_fits_pointer (value))
    {
      gpointer p = g_value_peek_pointer (value);

      if (!p)
        {
          *type = g_strdup (g_type_name (G_VALUE_TYPE (value)));
          *contents = g_strdup ("NULL");
        }
      else if (G_VALUE_HOLDS_OBJECT (value))
        {
          *type = g_strdup (G_OBJECT_TYPE_NAME (p));
          *contents = g_strdup_printf ("%p", p);
        }
      else if (G_VALUE_HOLDS_PARAM (value))
        {
          *type = g_strdup (G_PARAM_SPEC_TYPE_NAME (p));
          *contents = g_strdup_printf ("%p", p);
        }
      else if (G_VALUE_HOLDS (value, G_TYPE_STRV))
        {
          GStrv strv = g_value_get_boxed (value);
          GString *tmp = g_string_new ("[");

          while (*strv != NULL)
            {
              char *escaped = g_strescape (*strv, NULL);

              g_string_append_printf (tmp, "\"%s\"", escaped);
              g_free (escaped);

              if (*++strv != NULL)
                g_string_append (tmp, ", ");
            }

          g_string_append (tmp, "]");
          *type = g_strdup ("char**");
          *contents = g_string_free (tmp, FALSE);
        }
      else if (G_VALUE_HOLDS_BOXED (value))
        {
          *type = g_strdup (g_type_name (G_VALUE_TYPE (value)));
          *contents = g_strdup_printf ("%p", p);
        }
      else if (G_VALUE_HOLDS_POINTER (value))
        {
          *type = g_strdup ("gpointer");
          *contents = g_strdup_printf ("%p", p);
        }
      else
        {
          *type = g_strdup ("???");
          *contents = g_strdup ("???");
        }
    }
  else
    {
      *type = g_strdup ("???");
      *contents = g_strdup ("???");
    }
}

static void
cleanup_object (BobguiInspectorPropList *pl)
{
  if (pl->priv->object &&
      g_signal_handler_is_connected (pl->priv->object, pl->priv->notify_handler_id))
    g_signal_handler_disconnect (pl->priv->object, pl->priv->notify_handler_id);

  pl->priv->object = NULL;
  pl->priv->notify_handler_id = 0;
}

gboolean
bobgui_inspector_prop_list_set_object (BobguiInspectorPropList *pl,
                                    GObject              *object)
{
  GParamSpec **props;
  guint num_properties;
  guint i;
  GListStore *store;
  GListModel *list;
  GListModel *filtered;
  BobguiSortListModel *sorted;

  if (!object)
    return FALSE;

  if (pl->priv->object == object)
    return TRUE;

  cleanup_object (pl);

  bobgui_editable_set_text (BOBGUI_EDITABLE (pl->priv->search_entry), "");
  bobgui_stack_set_visible_child_name (BOBGUI_STACK (pl->priv->search_stack), "title");

  props = g_object_class_list_properties (G_OBJECT_GET_CLASS (object), &num_properties);

  pl->priv->object = object;

  store = g_list_store_new (PROP_TYPE_HOLDER);

  for (i = 0; i < num_properties; i++)
    {
      GParamSpec *prop = props[i];
      PropHolder *holder;

      if (! (prop->flags & G_PARAM_READABLE))
        continue;

      holder = prop_holder_new (object, prop);
      g_list_store_append (store, holder);
      g_object_unref (holder);
    }

  g_free (props);

  if (BOBGUI_IS_WIDGET (object))
    g_signal_connect_object (object, "destroy", G_CALLBACK (cleanup_object), pl, G_CONNECT_SWAPPED);

  filtered = G_LIST_MODEL (bobgui_filter_list_model_new (G_LIST_MODEL (store), g_object_ref (BOBGUI_FILTER (pl->priv->filter))));
  sorted = bobgui_sort_list_model_new (filtered, NULL);
  list = G_LIST_MODEL (bobgui_no_selection_new (G_LIST_MODEL (sorted)));

  bobgui_column_view_set_model (BOBGUI_COLUMN_VIEW (pl->priv->list), BOBGUI_SELECTION_MODEL (list));
  bobgui_sort_list_model_set_sorter (sorted, bobgui_column_view_get_sorter (BOBGUI_COLUMN_VIEW (pl->priv->list)));
  bobgui_column_view_sort_by_column (BOBGUI_COLUMN_VIEW (pl->priv->list), pl->priv->name, BOBGUI_SORT_ASCENDING);

  bobgui_widget_set_visible (BOBGUI_WIDGET (pl), TRUE);

  g_object_unref (list);

  return TRUE;
}

void
bobgui_inspector_prop_list_set_layout_child (BobguiInspectorPropList *pl,
                                          GObject              *object)
{
  BobguiWidget *stack;
  BobguiStackPage *page;
  BobguiWidget *parent;
  BobguiLayoutManager *layout_manager;
  BobguiLayoutChild *layout_child;

  stack = bobgui_widget_get_parent (BOBGUI_WIDGET (pl));
  page = bobgui_stack_get_page (BOBGUI_STACK (stack), BOBGUI_WIDGET (pl));
  g_object_set (page, "visible", FALSE, NULL);

  if (!BOBGUI_IS_WIDGET (object))
    return;

  parent = bobgui_widget_get_parent (BOBGUI_WIDGET (object));
  if (!parent)
    return;

  layout_manager = bobgui_widget_get_layout_manager (parent);
  if (!layout_manager)
    return;

  if (BOBGUI_LAYOUT_MANAGER_GET_CLASS (layout_manager)->layout_child_type == G_TYPE_INVALID)
    return;

  layout_child = bobgui_layout_manager_get_layout_child (layout_manager, BOBGUI_WIDGET (object));
  if (!layout_child)
    return;

  if (!bobgui_inspector_prop_list_set_object (pl, G_OBJECT (layout_child)))
    return;
  
  g_object_set (page, "visible", TRUE, NULL);
}

// vim: set et sw=2 ts=2:
