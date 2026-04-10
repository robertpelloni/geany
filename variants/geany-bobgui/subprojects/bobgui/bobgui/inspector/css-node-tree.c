/*
 * Copyright (c) 2014 Benjamin Otte <otte@gnome.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicntnse,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright noticnt and this permission noticnt shall be included
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

#include "css-node-tree.h"
#include "prop-editor.h"
#include "window.h"

#include "bobguilabel.h"
#include "bobgui/bobguiwidgetprivate.h"
#include "bobguicsscustompropertypoolprivate.h"
#include "bobguicssproviderprivate.h"
#include "bobguicssstylepropertyprivate.h"
#include "bobguicssstyleprivate.h"
#include "bobguicssvalueprivate.h"
#include "bobguicssselectorprivate.h"
#include "bobguisettings.h"
#include "bobguitypebuiltins.h"
#include "bobguistack.h"
#include "bobguinoselection.h"
#include "bobguisingleselection.h"
#include "bobguicolumnview.h"
#include "bobguicolumnviewcolumn.h"

#include <glib/gi18n-lib.h>
#include <bobgui/css/bobguicss.h>

/* {{{ CssProperty object */

typedef struct _CssProperty CssProperty;

G_DECLARE_FINAL_TYPE (CssProperty, css_property, CSS, PROPERTY, GObject);

struct _CssProperty
{
  GObject parent;

  char *name;
  char *value;
  char *location;
};

enum {
  CSS_PROPERTY_PROP_NAME = 1,
  CSS_PROPERTY_PROP_VALUE,
  CSS_PROPERTY_PROP_LOCATION,
  Css_PROPERTY_NUM_PROPERTIES
};

G_DEFINE_TYPE (CssProperty, css_property, G_TYPE_OBJECT);

static void
css_property_init (CssProperty *self)
{
}

static void
css_property_finalize (GObject *object)
{
  CssProperty *self = CSS_PROPERTY (object);

  g_free (self->name);
  g_free (self->value);
  g_free (self->location);

  G_OBJECT_CLASS (css_property_parent_class)->finalize (object);
}

static void
css_property_get_property (GObject    *object,
                           guint       property_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  CssProperty *self = CSS_PROPERTY (object);

  switch (property_id)
    {
    case CSS_PROPERTY_PROP_NAME:
      g_value_set_string (value, self->name);
      break;

    case CSS_PROPERTY_PROP_VALUE:
      g_value_set_string (value, self->value);
      break;

    case CSS_PROPERTY_PROP_LOCATION:
      g_value_set_string (value, self->location);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
css_property_class_init (CssPropertyClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  GParamSpec *pspec;

  object_class->finalize = css_property_finalize;
  object_class->get_property = css_property_get_property;

  pspec = g_param_spec_string ("name", NULL, NULL,
                               NULL,
                               G_PARAM_READABLE |
                               G_PARAM_STATIC_STRINGS);

  g_object_class_install_property (object_class, CSS_PROPERTY_PROP_NAME, pspec);

  pspec = g_param_spec_string ("value", NULL, NULL,
                               NULL,
                               G_PARAM_READABLE |
                               G_PARAM_STATIC_STRINGS);

  g_object_class_install_property (object_class, CSS_PROPERTY_PROP_VALUE, pspec);

  pspec = g_param_spec_string ("location", NULL, NULL,
                               NULL,
                               G_PARAM_READABLE |
                               G_PARAM_STATIC_STRINGS);

  g_object_class_install_property (object_class, CSS_PROPERTY_PROP_LOCATION, pspec);
}

static CssProperty *
css_property_new (const char *name,
                  const char *value,
                  const char *location)
{
  CssProperty *self;

  self = g_object_new (css_property_get_type (), NULL);

  self->name = g_strdup (name);
  self->value = g_strdup (value);
  self->location = g_strdup (location);

  return self;
}

/* }}} */

enum
{
  PROP_0,
  PROP_NODE,

  N_PROPS
};

struct _BobguiInspectorCssNodeTreePrivate
{
  GListStore *root_model;
  BobguiTreeListModel *node_model;
  BobguiSingleSelection *selection_model;
  BobguiWidget *node_tree;
  GListStore *prop_model;
  BobguiWidget *prop_tree;
  BobguiCssNode *node;
};

static GParamSpec *properties[N_PROPS] = { NULL, };

G_DEFINE_TYPE_WITH_PRIVATE (BobguiInspectorCssNodeTree, bobgui_inspector_css_node_tree, BOBGUI_TYPE_BOX)

static void
bobgui_inspector_css_node_tree_set_node (BobguiInspectorCssNodeTree *cnt,
                                      BobguiCssNode              *node);
static void
bobgui_inspector_css_node_tree_unset_node (BobguiInspectorCssNodeTree *cnt);

static void
selection_changed (BobguiSelectionModel       *model,
                   GParamSpec              *pspec,
                   BobguiInspectorCssNodeTree *cnt)
{
  if (bobgui_single_selection_get_selected (cnt->priv->selection_model) != BOBGUI_INVALID_LIST_POSITION)
    {
      BobguiTreeListRow *row;
      BobguiCssNode *node;

      row = bobgui_single_selection_get_selected_item (cnt->priv->selection_model);
      node = bobgui_tree_list_row_get_item (row);
      bobgui_inspector_css_node_tree_set_node (cnt, node);
    }
  else
    bobgui_inspector_css_node_tree_unset_node (cnt);
}

static void
bobgui_inspector_css_node_tree_unset_node (BobguiInspectorCssNodeTree *cnt)
{
  BobguiInspectorCssNodeTreePrivate *priv = cnt->priv;

  if (priv->node)
    {
      g_signal_handlers_disconnect_matched (priv->node,
                                            G_SIGNAL_MATCH_DATA,
                                            0, 0, NULL, NULL,
                                            cnt);
      g_object_unref (priv->node);
      priv->node = NULL;
    }
}

static void
bobgui_inspector_css_node_tree_get_property (GObject    *object,
                                          guint       property_id,
                                          GValue     *value,
                                          GParamSpec *pspec)
{
  BobguiInspectorCssNodeTree *cnt = BOBGUI_INSPECTOR_CSS_NODE_TREE (object);
  BobguiInspectorCssNodeTreePrivate *priv = cnt->priv;

  switch (property_id)
    {
    case PROP_NODE:
      g_value_set_object (value, priv->node);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
bobgui_inspector_css_node_tree_set_property (GObject      *object,
                                          guint         property_id,
                                          const GValue *value,
                                          GParamSpec   *pspec)
{
  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
bobgui_inspector_css_node_tree_finalize (GObject *object)
{
  BobguiInspectorCssNodeTree *cnt = BOBGUI_INSPECTOR_CSS_NODE_TREE (object);

  bobgui_inspector_css_node_tree_unset_node (cnt);

  G_OBJECT_CLASS (bobgui_inspector_css_node_tree_parent_class)->finalize (object);
}

static void
bobgui_inspector_css_node_tree_class_init (BobguiInspectorCssNodeTreeClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);

  object_class->set_property = bobgui_inspector_css_node_tree_set_property;
  object_class->get_property = bobgui_inspector_css_node_tree_get_property;
  object_class->finalize = bobgui_inspector_css_node_tree_finalize;

  properties[PROP_NODE] =
    g_param_spec_object ("node", NULL, NULL,
                         BOBGUI_TYPE_CSS_NODE,
                         G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPS, properties);

  bobgui_widget_class_set_template_from_resource (widget_class, "/org/bobgui/libbobgui/inspector/css-node-tree.ui");
  bobgui_widget_class_bind_template_child_private (widget_class, BobguiInspectorCssNodeTree, node_tree);
  bobgui_widget_class_bind_template_child_private (widget_class, BobguiInspectorCssNodeTree, prop_tree);
}

static int
sort_strv (gconstpointer a,
           gconstpointer b,
           gpointer      data)
{
  char **ap = (char **) a;
  char **bp = (char **) b;

  return g_ascii_strcasecmp (*ap, *bp);
}

static void
strv_sort (char **strv)
{
  g_sort_array (strv, g_strv_length (strv), sizeof (char *), sort_strv, NULL);
}

static char *
format_state_flags (BobguiStateFlags state)
{
  if (state)
    {
      GString *str;
      int i;
      gboolean first = TRUE;

      str = g_string_new ("");

      for (i = 0; i < 31; i++)
        {
          if (state & (1 << i))
            {
              if (!first)
                g_string_append (str, " | ");
              first = FALSE;
              g_string_append (str, bobgui_css_pseudoclass_name (1 << i));
            }
        }
      return g_string_free (str, FALSE);
    }

 return g_strdup ("");
}

static void
setup_label (BobguiSignalListItemFactory *factory,
             BobguiListItem              *list_item)
{
  BobguiWidget *label;

  label = bobgui_label_new (NULL);
  bobgui_label_set_xalign (BOBGUI_LABEL (label), 0.);
  bobgui_list_item_set_child (list_item, label);
}

static void
setup_value (BobguiSignalListItemFactory *factory,
             BobguiListItem              *list_item)
{
  BobguiWidget *label;

  label = bobgui_label_new (NULL);
  bobgui_label_set_xalign (BOBGUI_LABEL (label), 0.);
  bobgui_label_set_ellipsize (BOBGUI_LABEL (label), PANGO_ELLIPSIZE_END);
  bobgui_label_set_width_chars (BOBGUI_LABEL (label), 20);
  bobgui_list_item_set_child (list_item, label);
}

static void
bind_name (BobguiSignalListItemFactory *factory,
           BobguiListItem              *list_item)
{
  BobguiWidget *label;
  CssProperty *property;

  property = bobgui_list_item_get_item (list_item);
  label = bobgui_list_item_get_child (list_item);
  bobgui_label_set_text (BOBGUI_LABEL (label), property->name);
}

static void
bind_value (BobguiSignalListItemFactory *factory,
            BobguiListItem              *list_item)
{
  BobguiWidget *label;
  CssProperty *property;

  property = bobgui_list_item_get_item (list_item);
  label = bobgui_list_item_get_child (list_item);
  bobgui_label_set_text (BOBGUI_LABEL (label), property->value);
}

static void
bind_location (BobguiSignalListItemFactory *factory,
               BobguiListItem              *list_item)
{
  BobguiWidget *label;
  CssProperty *property;

  property = bobgui_list_item_get_item (list_item);
  label = bobgui_list_item_get_child (list_item);
  bobgui_label_set_text (BOBGUI_LABEL (label), property->location);
}

static GListModel *
create_model_for_node (gpointer object,
                       gpointer user_data)
{
  return bobgui_css_node_observe_children (BOBGUI_CSS_NODE (object));
}

static void
setup_node_label (BobguiSignalListItemFactory *factory,
                  BobguiListItem              *list_item)
{
  BobguiWidget *expander;
  BobguiWidget *label;

  expander = bobgui_tree_expander_new ();
  label = bobgui_editable_label_new ("");
  bobgui_editable_set_width_chars (BOBGUI_EDITABLE (label), 5);
  bobgui_tree_expander_set_child (BOBGUI_TREE_EXPANDER (expander), label);
  bobgui_list_item_set_child (list_item, expander);
}

static void
setup_editable_label (BobguiSignalListItemFactory *factory,
                      BobguiListItem              *list_item)
{
  BobguiWidget *label;

  label = bobgui_editable_label_new ("");
  bobgui_editable_set_width_chars (BOBGUI_EDITABLE (label), 5);
  bobgui_list_item_set_child (list_item, label);
}

static void
name_changed (BobguiEditable *editable,
              GParamSpec  *pspec,
              BobguiCssNode  *node)
{
  bobgui_css_node_set_name (node, g_quark_from_string (bobgui_editable_get_text (editable)));
}

static void
bind_node_name (BobguiSignalListItemFactory *factory,
                BobguiListItem              *list_item)
{
  BobguiWidget *expander;
  BobguiWidget *label;
  BobguiTreeListRow *row;
  BobguiCssNode *node;
  const char *text;

  row = bobgui_list_item_get_item (list_item);
  node = bobgui_tree_list_row_get_item (row);

  expander = bobgui_list_item_get_child (list_item);
  bobgui_tree_expander_set_list_row (BOBGUI_TREE_EXPANDER (expander), row);

  label = bobgui_tree_expander_get_child (BOBGUI_TREE_EXPANDER (expander));
  text = g_quark_to_string (bobgui_css_node_get_name (node));
  if (!text)
    text = "";
  bobgui_editable_set_text (BOBGUI_EDITABLE (label), text);

  g_signal_connect (label, "notify::text", G_CALLBACK (name_changed), node);
}

static void
unbind_node_name (BobguiSignalListItemFactory *factory,
                  BobguiListItem              *list_item)
{
  BobguiWidget *expander;
  BobguiWidget *label;
  BobguiTreeListRow *row;
  BobguiCssNode *node;

  row = bobgui_list_item_get_item (list_item);
  node = bobgui_tree_list_row_get_item (row);

  expander = bobgui_list_item_get_child (list_item);
  label = bobgui_tree_expander_get_child (BOBGUI_TREE_EXPANDER (expander));

  g_signal_handlers_disconnect_by_func (label, G_CALLBACK (name_changed), node);
}

static void
id_changed (BobguiEditable *editable,
              GParamSpec  *pspec,
              BobguiCssNode  *node)
{
  bobgui_css_node_set_id (node, g_quark_from_string (bobgui_editable_get_text (editable)));
}

static void
bind_node_id (BobguiSignalListItemFactory *factory,
              BobguiListItem              *list_item)
{
  BobguiWidget *label;
  BobguiTreeListRow *row;
  BobguiCssNode *node;
  const char *text;

  row = bobgui_list_item_get_item (list_item);
  node = bobgui_tree_list_row_get_item (row);

  label = bobgui_list_item_get_child (list_item);
  text = g_quark_to_string (bobgui_css_node_get_id (node));
  if (!text)
    text = "";
  bobgui_editable_set_text (BOBGUI_EDITABLE (label), text);

  g_signal_connect (label, "notify::text", G_CALLBACK (id_changed), node);
}

static void
unbind_node_id (BobguiSignalListItemFactory *factory,
                BobguiListItem              *list_item)
{
  BobguiWidget *label;
  BobguiTreeListRow *row;
  BobguiCssNode *node;

  row = bobgui_list_item_get_item (list_item);
  node = bobgui_tree_list_row_get_item (row);

  label = bobgui_list_item_get_child (list_item);

  g_signal_handlers_disconnect_by_func (label, G_CALLBACK (id_changed), node);
}

static void
classes_changed (BobguiEditable *editable,
                 GParamSpec  *pspec,
                 BobguiCssNode  *node)
{
  const char *text;
  char **classes;

  text = bobgui_editable_get_text (editable);
  classes = g_strsplit (text, " ", -1);
  bobgui_css_node_set_classes (node, (const char **)classes);
  g_strfreev (classes);
}

static void
bind_node_classes (BobguiSignalListItemFactory *factory,
                   BobguiListItem              *list_item)
{
  BobguiWidget *label;
  BobguiTreeListRow *row;
  BobguiCssNode *node;
  char **classes;
  char *text;

  row = bobgui_list_item_get_item (list_item);
  node = bobgui_tree_list_row_get_item (row);

  label = bobgui_list_item_get_child (list_item);
  classes = bobgui_css_node_get_classes (node);
  strv_sort (classes);
  text = g_strjoinv (" ", classes);
  bobgui_editable_set_text (BOBGUI_EDITABLE (label), text);
  g_strfreev (classes);
  g_free (text);

  g_signal_connect (label, "notify::text", G_CALLBACK (classes_changed), node);
}

static void
unbind_node_classes (BobguiSignalListItemFactory *factory,
                     BobguiListItem              *list_item)
{
  BobguiWidget *label;
  BobguiTreeListRow *row;
  BobguiCssNode *node;

  row = bobgui_list_item_get_item (list_item);
  node = bobgui_tree_list_row_get_item (row);

  label = bobgui_list_item_get_child (list_item);

  g_signal_handlers_disconnect_by_func (label, G_CALLBACK (classes_changed), node);
}

static void
bind_node_state (BobguiSignalListItemFactory *factory,
                 BobguiListItem              *list_item)
{
  BobguiWidget *label;
  BobguiTreeListRow *row;
  BobguiCssNode *node;
  char *text;

  row = bobgui_list_item_get_item (list_item);
  node = bobgui_tree_list_row_get_item (row);

  label = bobgui_list_item_get_child (list_item);
  text = format_state_flags (bobgui_css_node_get_state (node));
  bobgui_label_set_text (BOBGUI_LABEL (label), text);
  g_free (text);
}

static int
compare_name_cb (gconstpointer a,
                 gconstpointer b,
                 gpointer      user_data)
{
  const CssProperty *prop_a = a;
  const CssProperty *prop_b = b;
  gboolean a_var = prop_a->name[0] == '-' && prop_a->name[1] == '-';
  gboolean b_var = prop_b->name[0] == '-' && prop_b->name[1] == '-';
  gboolean a_bobgui = prop_a->name[0] == '-' && prop_a->name[1] != '-';
  gboolean b_bobgui = prop_b->name[0] == '-' && prop_b->name[1] != '-';
  int ret;

  if (a_var && !b_var)
    ret = 1;
  else if (b_var && !a_var)
    ret = -1;
  else if (a_bobgui && !b_bobgui)
    ret = 1;
  else if (b_bobgui && !a_bobgui)
    ret = -1;
  else
    ret = g_utf8_collate (prop_a->name, prop_b->name);

  return bobgui_ordering_from_cmpfunc (ret);
}

static void
bobgui_inspector_css_node_tree_init (BobguiInspectorCssNodeTree *cnt)
{
  BobguiInspectorCssNodeTreePrivate *priv;
  int i;
  BobguiListItemFactory *factory;
  BobguiColumnViewColumn *column;
  BobguiSorter *sorter;
  BobguiSortListModel *sort_model;
  BobguiSelectionModel *selection_model;

  cnt->priv = bobgui_inspector_css_node_tree_get_instance_private (cnt);
  bobgui_widget_init_template (BOBGUI_WIDGET (cnt));
  priv = cnt->priv;

  priv->root_model = g_list_store_new (bobgui_css_node_get_type ());
  priv->node_model = bobgui_tree_list_model_new (G_LIST_MODEL (priv->root_model),
                                              FALSE, FALSE,
                                              create_model_for_node,
                                              NULL, NULL);

  priv->selection_model = bobgui_single_selection_new (G_LIST_MODEL (priv->node_model));
  g_signal_connect (priv->selection_model, "notify::selected", G_CALLBACK (selection_changed), cnt);

  bobgui_column_view_set_model (BOBGUI_COLUMN_VIEW (priv->node_tree), BOBGUI_SELECTION_MODEL (priv->selection_model));
  g_object_unref (priv->selection_model);

  column = g_list_model_get_item (bobgui_column_view_get_columns (BOBGUI_COLUMN_VIEW (priv->node_tree)), 0);
  factory = bobgui_signal_list_item_factory_new ();
  g_signal_connect (factory, "setup", G_CALLBACK (setup_node_label), NULL);
  g_signal_connect (factory, "bind", G_CALLBACK (bind_node_name), NULL);
  g_signal_connect (factory, "unbind", G_CALLBACK (unbind_node_name), NULL);
  bobgui_column_view_column_set_factory (column, factory);
  g_object_unref (factory);
  g_object_unref (column);

  column = g_list_model_get_item (bobgui_column_view_get_columns (BOBGUI_COLUMN_VIEW (priv->node_tree)), 1);
  factory = bobgui_signal_list_item_factory_new ();
  g_signal_connect (factory, "setup", G_CALLBACK (setup_editable_label), NULL);
  g_signal_connect (factory, "bind", G_CALLBACK (bind_node_id), NULL);
  g_signal_connect (factory, "unbind", G_CALLBACK (unbind_node_id), NULL);
  bobgui_column_view_column_set_factory (column, factory);
  g_object_unref (factory);
  g_object_unref (column);

  column = g_list_model_get_item (bobgui_column_view_get_columns (BOBGUI_COLUMN_VIEW (priv->node_tree)), 2);
  factory = bobgui_signal_list_item_factory_new ();
  g_signal_connect (factory, "setup", G_CALLBACK (setup_editable_label), NULL);
  g_signal_connect (factory, "bind", G_CALLBACK (bind_node_classes), NULL);
  g_signal_connect (factory, "unbind", G_CALLBACK (unbind_node_classes), NULL);
  bobgui_column_view_column_set_factory (column, factory);
  g_object_unref (factory);
  g_object_unref (column);

  column = g_list_model_get_item (bobgui_column_view_get_columns (BOBGUI_COLUMN_VIEW (priv->node_tree)), 3);
  factory = bobgui_signal_list_item_factory_new ();
  g_signal_connect (factory, "setup", G_CALLBACK (setup_label), NULL);
  g_signal_connect (factory, "bind", G_CALLBACK (bind_node_state), NULL);
  bobgui_column_view_column_set_factory (column, factory);
  g_object_unref (factory);
  g_object_unref (column);

  priv->prop_model = g_list_store_new (css_property_get_type ());

  sort_model = bobgui_sort_list_model_new (G_LIST_MODEL (priv->prop_model),
                                        g_object_ref (bobgui_column_view_get_sorter (BOBGUI_COLUMN_VIEW (priv->prop_tree))));

  selection_model = BOBGUI_SELECTION_MODEL (bobgui_no_selection_new (G_LIST_MODEL (sort_model)));
  bobgui_column_view_set_model (BOBGUI_COLUMN_VIEW (priv->prop_tree), selection_model);
  g_object_unref (selection_model);

  column = g_list_model_get_item (bobgui_column_view_get_columns (BOBGUI_COLUMN_VIEW (priv->prop_tree)), 0);
  factory = bobgui_signal_list_item_factory_new ();
  g_signal_connect (factory, "setup", G_CALLBACK (setup_label), NULL);
  g_signal_connect (factory, "bind", G_CALLBACK (bind_name), NULL);
  bobgui_column_view_column_set_factory (column, factory);
  sorter = BOBGUI_SORTER (bobgui_custom_sorter_new ((GCompareDataFunc) compare_name_cb, NULL, NULL));
  bobgui_column_view_column_set_sorter (column, sorter);
  bobgui_column_view_sort_by_column (BOBGUI_COLUMN_VIEW (priv->prop_tree), column, BOBGUI_SORT_ASCENDING);
  g_object_unref (sorter);
  g_object_unref (factory);
  g_object_unref (column);

  column = g_list_model_get_item (bobgui_column_view_get_columns (BOBGUI_COLUMN_VIEW (priv->prop_tree)), 1);
  factory = bobgui_signal_list_item_factory_new ();
  g_signal_connect (factory, "setup", G_CALLBACK (setup_value), NULL);
  g_signal_connect (factory, "bind", G_CALLBACK (bind_value), NULL);
  bobgui_column_view_column_set_factory (column, factory);
  g_object_unref (factory);
  g_object_unref (column);

  column = g_list_model_get_item (bobgui_column_view_get_columns (BOBGUI_COLUMN_VIEW (priv->prop_tree)), 2);
  factory = bobgui_signal_list_item_factory_new ();
  g_signal_connect (factory, "setup", G_CALLBACK (setup_label), NULL);
  g_signal_connect (factory, "bind", G_CALLBACK (bind_location), NULL);
  bobgui_column_view_column_set_factory (column, factory);
  g_object_unref (factory);
  g_object_unref (column);

  for (i = 0; i < _bobgui_css_style_property_get_n_properties (); i++)
    {
      BobguiCssStyleProperty *prop;
      const char *name;

      prop = _bobgui_css_style_property_lookup_by_id (i);
      name = _bobgui_style_property_get_name (BOBGUI_STYLE_PROPERTY (prop));

      g_list_store_append (priv->prop_model, css_property_new (name, NULL, NULL));
    }
}

void
bobgui_inspector_css_node_tree_set_object (BobguiInspectorCssNodeTree *cnt,
                                        GObject                 *object)
{
  BobguiWidget *stack;
  BobguiStackPage *page;
  BobguiCssNode *root;
  GList *nodes = NULL;
  GList *l;
  int i;

  g_return_if_fail (BOBGUI_INSPECTOR_IS_CSS_NODE_TREE (cnt));

  stack = bobgui_widget_get_parent (BOBGUI_WIDGET (cnt));
  page = bobgui_stack_get_page (BOBGUI_STACK (stack), BOBGUI_WIDGET (cnt));

  if (!BOBGUI_IS_WIDGET (object))
    {
      g_object_set (page, "visible", FALSE, NULL);
      return;
    }

  g_object_set (page, "visible", TRUE, NULL);

  root = bobgui_widget_get_css_node (BOBGUI_WIDGET (object));
  nodes = g_list_prepend (nodes, root);
  while (bobgui_css_node_get_parent (root))
    {
      root = bobgui_css_node_get_parent (root);
      nodes = g_list_prepend (nodes, root);
    }

  g_list_store_remove_all (cnt->priv->root_model);
  g_list_store_append (cnt->priv->root_model, root);

  i = 0;
  for (l = nodes; l; l = l->next)
    {
      BobguiCssNode *node = l->data;

      for (; i < g_list_model_get_n_items (G_LIST_MODEL (cnt->priv->node_model)); i++)
        {
          BobguiTreeListRow *row = g_list_model_get_item (G_LIST_MODEL (cnt->priv->node_model), i);
          if (bobgui_tree_list_row_get_item (row) == node)
            {
              bobgui_tree_list_row_set_expanded (row, TRUE);
              g_object_unref (row);
              break;
            }
          g_object_unref (row);
        }
    }

 bobgui_single_selection_set_selected (cnt->priv->selection_model, i);

 g_list_free (nodes);
}

static void
bobgui_inspector_css_node_tree_update_style (BobguiInspectorCssNodeTree *cnt,
                                          BobguiCssStyle             *new_style)
{
  BobguiInspectorCssNodeTreePrivate *priv = cnt->priv;
  BobguiCssCustomPropertyPool *pool = bobgui_css_custom_property_pool_get ();
  GArray *custom_props;
  int i, n, n_props;

  n_props = _bobgui_css_style_property_get_n_properties ();
  n = g_list_model_get_n_items (G_LIST_MODEL (priv->prop_model));

  for (i = 0; i < n_props; i++)
    {
      BobguiCssStyleProperty *prop;
      const char *name;
      BobguiCssSection *section;
      char *location;
      char *value;
      CssProperty *property;

      prop = _bobgui_css_style_property_lookup_by_id (i);
      name = _bobgui_style_property_get_name (BOBGUI_STYLE_PROPERTY (prop));

      if (new_style)
        {
          value = bobgui_css_value_to_string (bobgui_css_style_get_value (new_style, i));

          section = bobgui_css_style_get_section (new_style, i);
          if (section)
            location = bobgui_css_section_to_string (section);
          else
            location = NULL;
        }
      else
        {
          value = NULL;
          location = NULL;
        }

      property = css_property_new (name, value, location);
      g_list_store_splice (priv->prop_model, i, 1, (gpointer *)&property, 1);

      g_free (location);
      g_free (value);
    }

  g_list_store_splice (priv->prop_model, n_props, n - n_props, NULL, 0);

  if (new_style)
    {
      custom_props = bobgui_css_style_list_custom_properties (new_style);

      if (custom_props)
        {
          for (i = 0; i < custom_props->len; i++)
            {
              BobguiCssVariableValue *var_value;
              int id;
              const char *name;
              BobguiCssSection *section;
              char *location;
              char *value;
              CssProperty *property;

              id = g_array_index (custom_props, int, i);
              name = bobgui_css_custom_property_pool_get_name (pool, id);
              var_value = bobgui_css_style_get_custom_property (new_style, id);

              value = bobgui_css_variable_value_to_string (var_value);

              section = var_value->section;
              if (section)
                location = bobgui_css_section_to_string (section);
              else
                location = NULL;

              property = css_property_new (name, value, location);
              g_list_store_append (priv->prop_model, property);

              g_free (location);
              g_free (value);
            }

          g_array_unref (custom_props);
        }
    }
}

static void
bobgui_inspector_css_node_tree_update_style_cb (BobguiCssNode              *node,
                                             BobguiCssStyleChange       *change,
                                             BobguiInspectorCssNodeTree *cnt)
{
  bobgui_inspector_css_node_tree_update_style (cnt, bobgui_css_style_change_get_new_style (change));
}

static void
bobgui_inspector_css_node_tree_set_node (BobguiInspectorCssNodeTree *cnt,
                                      BobguiCssNode              *node)
{
  BobguiInspectorCssNodeTreePrivate *priv = cnt->priv;

  if (priv->node == node)
    return;

  if (node)
    g_object_ref (node);

  bobgui_inspector_css_node_tree_update_style (cnt, node ? bobgui_css_node_get_style (node) : NULL);

  bobgui_inspector_css_node_tree_unset_node (cnt);

  priv->node = node;

  if (node)
    g_signal_connect (node, "style-changed", G_CALLBACK (bobgui_inspector_css_node_tree_update_style_cb), cnt);

  g_object_notify_by_pspec (G_OBJECT (cnt), properties[PROP_NODE]);
}

BobguiCssNode *
bobgui_inspector_css_node_tree_get_node (BobguiInspectorCssNodeTree *cnt)
{
  BobguiInspectorCssNodeTreePrivate *priv = cnt->priv;

  return priv->node;
}

void
bobgui_inspector_css_node_tree_set_display (BobguiInspectorCssNodeTree *cnt,
                                         GdkDisplay *display)
{
  BobguiSettings *settings;
  char *theme_name;

  settings = bobgui_settings_get_for_display (display);
  g_object_get (settings, "bobgui-theme-name", &theme_name, NULL);
  g_object_set (settings, "bobgui-theme-name", theme_name, NULL);
  g_free (theme_name);
}

/* vim:set foldmethod=marker: */
