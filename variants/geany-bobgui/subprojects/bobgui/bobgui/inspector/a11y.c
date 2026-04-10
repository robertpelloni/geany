/*
 * Copyright (c) 2020 Red Hat, Inc.
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
#include <glib/gi18n-lib.h>

#include "a11y.h"
#include "window.h"

#include "bobguitypebuiltins.h"
#include "bobguilabel.h"
#include "bobguistack.h"
#include "bobguibinlayout.h"
#include "bobguiaccessibleprivate.h"
#include "bobguiaccessibletext.h"
#include "bobguiaccessiblerange.h"
#include "bobguiaccessiblevalueprivate.h"
#include "bobguiatcontextprivate.h"
#include "bobguicolumnview.h"
#include "bobguisignallistitemfactory.h"
#include "bobguilistitem.h"
#include "bobguinoselection.h"
#include "bobguifilterlistmodel.h"
#include "bobguiboolfilter.h"
#ifdef G_OS_UNIX
#include "a11y/bobguiatspicontextprivate.h"
#endif

typedef enum {
  STATE,
  PROPERTY,
  RELATION
} AttributeKind;

typedef struct _AccessibleAttribute AccessibleAttribute;
typedef struct _AccessibleAttributeClass AccessibleAttributeClass;

struct _AccessibleAttribute
{
  GObject parent_instance;
  AttributeKind kind;
  int attribute;
  char *name;
  gboolean is_default;
  BobguiAccessibleValue *value;
};

struct _AccessibleAttributeClass
{
  GObjectClass parent_class;
};

enum {
  PROP_KIND = 1,
  PROP_ATTRIBUTE,
  PROP_NAME,
  PROP_IS_DEFAULT,
  PROP_VALUE
};

static GType accessible_attribute_get_type (void);

G_DEFINE_TYPE (AccessibleAttribute, accessible_attribute, G_TYPE_OBJECT);

static void
accessible_attribute_init (AccessibleAttribute *object)
{
}

static void
accessible_attribute_finalize (GObject *object)
{
  AccessibleAttribute *self = (AccessibleAttribute *)object;

  g_free (self->name);
  bobgui_accessible_value_unref (self->value);

  G_OBJECT_CLASS (accessible_attribute_parent_class)->finalize (object);
}

static void
accessible_attribute_set_property (GObject      *object,
                                   guint         prop_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  AccessibleAttribute *self = (AccessibleAttribute *)object;

  switch (prop_id)
    {
    case PROP_KIND:
      self->kind = g_value_get_uint (value);
      break;

    case PROP_ATTRIBUTE:
      self->attribute = g_value_get_uint (value);
      break;

    case PROP_NAME:
      g_clear_pointer (&self->name, g_free);
      self->name = g_value_dup_string (value);
      break;

    case PROP_IS_DEFAULT:
      self->is_default = g_value_get_boolean (value);
      break;

    case PROP_VALUE:
      g_clear_pointer (&self->value, bobgui_accessible_value_unref);
      self->value = g_value_dup_boxed (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
accessible_attribute_get_property (GObject    *object,
                                   guint       prop_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  AccessibleAttribute *self = (AccessibleAttribute *)object;

  switch (prop_id)
    {
    case PROP_KIND:
      g_value_set_uint (value, self->kind);
      break;

    case PROP_ATTRIBUTE:
      g_value_set_uint (value, self->attribute);
      break;

    case PROP_NAME:
      g_value_set_string (value, self->name);
      break;

    case PROP_IS_DEFAULT:
      g_value_set_boolean (value, self->is_default);
      break;

    case PROP_VALUE:
      g_value_set_boxed (value, self->value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
accessible_attribute_class_init (AccessibleAttributeClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->finalize = accessible_attribute_finalize;
  object_class->set_property = accessible_attribute_set_property;
  object_class->get_property = accessible_attribute_get_property;

  g_object_class_install_property (object_class, PROP_KIND,
      g_param_spec_uint ("kind", NULL, NULL,
                         0, 2, 0,
                         G_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_ATTRIBUTE,
      g_param_spec_uint ("attribute", NULL, NULL,
                         0, G_MAXUINT, 0,
                         G_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_NAME,
      g_param_spec_string ("name", NULL, NULL,
                           NULL,
                           G_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_IS_DEFAULT,
      g_param_spec_boolean ("is-default", NULL, NULL,
                            FALSE,
                            G_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_VALUE,
      g_param_spec_boxed ("value", NULL, NULL,
                          BOBGUI_TYPE_ACCESSIBLE_VALUE,
                          G_PARAM_READWRITE));
}

struct _BobguiInspectorA11y
{
  BobguiWidget parent;

  GObject *object;

  BobguiWidget *box;
  BobguiWidget *role;
  BobguiWidget *name;
  BobguiWidget *description;
  BobguiWidget *bounds;
  BobguiWidget *path_label;
  BobguiWidget *path;
  BobguiWidget *interface_label;
  BobguiWidget *interface;
  BobguiWidget *attributes;
};

typedef struct _BobguiInspectorA11yClass
{
  BobguiWidgetClass parent_class;
} BobguiInspectorA11yClass;

G_DEFINE_TYPE (BobguiInspectorA11y, bobgui_inspector_a11y, BOBGUI_TYPE_WIDGET)

static void
update_role (BobguiInspectorA11y *sl)
{
  BobguiAccessibleRole role;
  GEnumClass *eclass;
  GEnumValue *value;

  role = bobgui_accessible_get_accessible_role (BOBGUI_ACCESSIBLE (sl->object));

  eclass = g_type_class_ref (BOBGUI_TYPE_ACCESSIBLE_ROLE);
  value = g_enum_get_value (eclass, role);
  bobgui_label_set_label (BOBGUI_LABEL (sl->role), value->value_nick);
  g_type_class_unref (eclass);
}

static void
update_name (BobguiInspectorA11y *sl)
{
  BobguiATContext *context;
  char *name;

  context = bobgui_accessible_get_at_context (BOBGUI_ACCESSIBLE (sl->object));
  if (context == NULL)
    return;

  name = bobgui_at_context_get_name (context);
  bobgui_label_set_label (BOBGUI_LABEL (sl->name), name);

  g_object_unref (context);
}

static void
update_description (BobguiInspectorA11y *sl)
{
  BobguiATContext *context;
  char *description;

  context = bobgui_accessible_get_at_context (BOBGUI_ACCESSIBLE (sl->object));
  if (context == NULL)
    return;

  description = bobgui_at_context_get_description (context);
  bobgui_label_set_label (BOBGUI_LABEL (sl->description), description);

  g_object_unref (context);
}

static void
update_path (BobguiInspectorA11y *sl)
{
#ifdef G_OS_UNIX
  const char *path = NULL;
  BobguiATContext *context;

  context = bobgui_accessible_get_at_context (BOBGUI_ACCESSIBLE (sl->object));
  if (BOBGUI_IS_AT_SPI_CONTEXT (context))
    {
      if (bobgui_at_context_is_realized (context))
        path = bobgui_at_spi_context_get_context_path (BOBGUI_AT_SPI_CONTEXT (context));
      else
        path = "not realized";
    }

  if (path != NULL)
      bobgui_label_set_label (BOBGUI_LABEL (sl->path), path);

  bobgui_widget_set_visible (sl->path, path != NULL);
  bobgui_widget_set_visible (sl->path_label, path != NULL);

  g_clear_object (&context);
#endif
}

static void
update_bounds (BobguiInspectorA11y *sl)
{
  int x, y, w, h;

  if (bobgui_accessible_get_bounds (BOBGUI_ACCESSIBLE (sl->object), &x, &y, &w, &h))
    {
      char *size_label = g_strdup_printf ("%d × %d +%d +%d", w, h, x, y);
      bobgui_label_set_label (BOBGUI_LABEL (sl->bounds), size_label);
      g_free (size_label);
    }
}

static void
update_interface (BobguiInspectorA11y *sl)
{
  const char *interface = NULL;

  if (BOBGUI_IS_ACCESSIBLE_TEXT (sl->object))
    interface = "BobguiAccessibleText";
  else if (BOBGUI_IS_ACCESSIBLE_RANGE (sl->object))
    interface = "BobguiAccessibleRange";

  bobgui_label_set_label (BOBGUI_LABEL (sl->interface), interface ? interface : "");

  bobgui_widget_set_visible (sl->interface, interface != NULL);
  bobgui_widget_set_visible (sl->interface_label, interface != NULL);
}

extern GType bobgui_string_pair_get_type (void);

static void
update_attributes (BobguiInspectorA11y *sl)
{
  BobguiATContext *context;
  GListStore *store;
  BobguiBoolFilter *filter;
  BobguiFilterListModel *filter_model;
  BobguiNoSelection *selection;
  GObject *obj;
  GEnumClass *eclass;
  guint i;
  const char *name;
  BobguiAccessibleState state;
  BobguiAccessibleProperty prop;
  BobguiAccessibleRelation rel;
  BobguiAccessibleValue *value;
  gboolean has_value;

  context = bobgui_accessible_get_at_context (BOBGUI_ACCESSIBLE (sl->object));
  if (context == NULL)
    return;

  store = g_list_store_new (G_TYPE_OBJECT);

  eclass = g_type_class_ref (BOBGUI_TYPE_ACCESSIBLE_STATE);

  for (i = 0; i < eclass->n_values; i++)
    {
      state = eclass->values[i].value;
      name = eclass->values[i].value_nick;
      has_value = bobgui_at_context_has_accessible_state (context, state);
      value = bobgui_at_context_get_accessible_state (context, state);
      obj = g_object_new (accessible_attribute_get_type (),
                          "kind", STATE,
                          "attribute", state,
                          "name", name,
                          "is-default", !has_value,
                          "value", value,
                          NULL);
      g_list_store_append (store, obj);
      g_object_unref (obj);
    }

  g_type_class_unref (eclass);

  eclass = g_type_class_ref (BOBGUI_TYPE_ACCESSIBLE_PROPERTY);

  for (i = 0; i < eclass->n_values; i++)
    {
      prop = eclass->values[i].value;
      name = eclass->values[i].value_nick;
      has_value = bobgui_at_context_has_accessible_property (context, prop);
      value = bobgui_at_context_get_accessible_property (context, prop);
      obj = g_object_new (accessible_attribute_get_type (),
                          "kind", PROPERTY,
                          "attribute", prop,
                          "name", name,
                          "is-default", !has_value,
                          "value", value,
                          NULL);
      g_list_store_append (store, obj);
      g_object_unref (obj);
    }

  g_type_class_unref (eclass);

  eclass = g_type_class_ref (BOBGUI_TYPE_ACCESSIBLE_RELATION);

  for (i = 0; i < eclass->n_values; i++)
    {
      rel = eclass->values[i].value;
      name = eclass->values[i].value_nick;
      has_value = bobgui_at_context_has_accessible_relation (context, rel);
      value = bobgui_at_context_get_accessible_relation (context, rel);
      obj = g_object_new (accessible_attribute_get_type (),
                          "kind", RELATION,
                          "attribute", rel,
                          "name", name,
                          "is-default", !has_value,
                          "value", value,
                          NULL);
      g_list_store_append (store, obj);
      g_object_unref (obj);
    }

  g_type_class_unref (eclass);

  filter = bobgui_bool_filter_new (bobgui_property_expression_new (accessible_attribute_get_type (), NULL, "is-default"));
  bobgui_bool_filter_set_invert (filter, TRUE);

  filter_model = bobgui_filter_list_model_new (G_LIST_MODEL (store), BOBGUI_FILTER (filter));
  selection = bobgui_no_selection_new (G_LIST_MODEL (filter_model));
  bobgui_column_view_set_model (BOBGUI_COLUMN_VIEW (sl->attributes), BOBGUI_SELECTION_MODEL (selection));
  g_object_unref (selection);

  bobgui_widget_set_visible (sl->attributes, g_list_model_get_n_items (G_LIST_MODEL (filter_model)) > 0);

  g_object_unref (context);
}

static void
setup_cell_cb (BobguiSignalListItemFactory *factory,
               BobguiListItem              *list_item)
{
  BobguiWidget *label;

  label = bobgui_label_new (NULL);
  bobgui_label_set_xalign (BOBGUI_LABEL (label), 0.0);
  bobgui_label_set_ellipsize (BOBGUI_LABEL (label), PANGO_ELLIPSIZE_END);
  bobgui_label_set_width_chars (BOBGUI_LABEL (label), 50);
  bobgui_widget_set_margin_start (label, 6);
  bobgui_widget_set_margin_end (label, 6);
  bobgui_list_item_set_child (list_item, label);
}

static void
bind_name_cb (BobguiSignalListItemFactory *factory,
              BobguiListItem              *list_item)
{
  AccessibleAttribute *item;
  BobguiWidget *label;

  item = bobgui_list_item_get_item (list_item);
  label = bobgui_list_item_get_child (list_item);

  if (item->is_default)
    bobgui_widget_add_css_class (label, "dim-label");
  else
    bobgui_widget_remove_css_class (label, "dim-label");

  bobgui_label_set_label (BOBGUI_LABEL (label), item->name);
}

static void
bind_value_cb (BobguiSignalListItemFactory *factory,
               BobguiListItem              *list_item)
{
  AccessibleAttribute *item;
  BobguiWidget *label;
  char *string;

  item = bobgui_list_item_get_item (list_item);
  label = bobgui_list_item_get_child (list_item);

  if (item->is_default)
    bobgui_widget_add_css_class (label, "dim-label");
  else
    bobgui_widget_remove_css_class (label, "dim-label");

  string = bobgui_accessible_value_to_string (item->value);
  bobgui_label_set_label (BOBGUI_LABEL (label), string);
  g_free (string);
}

static void
refresh_all (BobguiInspectorA11y *sl)
{
  update_role (sl);
  update_name (sl);
  update_description (sl);
  update_path (sl);
  update_interface (sl);
  update_attributes (sl);
}

void
bobgui_inspector_a11y_set_object (BobguiInspectorA11y *sl,
                               GObject          *object)
{
  BobguiWidget *stack;
  BobguiStackPage *page;
  BobguiATContext *context;

  if (sl->object && BOBGUI_IS_ACCESSIBLE (sl->object))
    {
      context = bobgui_accessible_get_at_context (BOBGUI_ACCESSIBLE (sl->object));
      if (context != NULL)
        {
          g_signal_handlers_disconnect_by_func (context, refresh_all, sl);
          g_object_unref (context);
        }
    }

  g_set_object (&sl->object, object);

  stack = bobgui_widget_get_parent (BOBGUI_WIDGET (sl));
  page = bobgui_stack_get_page (BOBGUI_STACK (stack), BOBGUI_WIDGET (sl));

  if (BOBGUI_IS_ACCESSIBLE (sl->object))
    {
      context = bobgui_accessible_get_at_context (BOBGUI_ACCESSIBLE (sl->object));
      if (context != NULL)
        {
          bobgui_at_context_realize (context);
          g_signal_connect_swapped (context, "state-change", G_CALLBACK (refresh_all), sl);
          g_object_unref (context);
        }

      bobgui_stack_page_set_visible (page, TRUE);
      refresh_all (sl);
      update_bounds (sl);
      update_interface (sl);
    }
  else
    {
      bobgui_stack_page_set_visible (page, FALSE);
    }
}

static void
bobgui_inspector_a11y_init (BobguiInspectorA11y *sl)
{
  bobgui_widget_init_template (BOBGUI_WIDGET (sl));

#ifndef G_OS_UNIX
  bobgui_widget_set_visible (sl->path, FALSE);
  bobgui_widget_set_visible (sl->path_label, FALSE);
#endif

  bobgui_widget_set_visible (sl->interface, FALSE);
  bobgui_widget_set_visible (sl->interface_label, FALSE);
}

static void
dispose (GObject *o)
{
  BobguiInspectorA11y *sl = BOBGUI_INSPECTOR_A11Y (o);

  if (sl->object && BOBGUI_IS_ACCESSIBLE (sl->object))
    {
      BobguiATContext *context;

      context = bobgui_accessible_get_at_context (BOBGUI_ACCESSIBLE (sl->object));
      if (context != NULL)
        {
          g_signal_handlers_disconnect_by_func (context, refresh_all, sl);
          g_object_unref (context);
        }
    }

  g_clear_object (&sl->object);

  bobgui_widget_dispose_template (BOBGUI_WIDGET (o), BOBGUI_TYPE_INSPECTOR_A11Y);

  G_OBJECT_CLASS (bobgui_inspector_a11y_parent_class)->dispose (o);
}

static void
bobgui_inspector_a11y_class_init (BobguiInspectorA11yClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);

  object_class->dispose = dispose;

  bobgui_widget_class_set_template_from_resource (widget_class, "/org/bobgui/libbobgui/inspector/a11y.ui");
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorA11y, box);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorA11y, role);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorA11y, name);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorA11y, description);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorA11y, bounds);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorA11y, path_label);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorA11y, path);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorA11y, interface_label);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorA11y, interface);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorA11y, attributes);

  bobgui_widget_class_bind_template_callback (widget_class, setup_cell_cb);
  bobgui_widget_class_bind_template_callback (widget_class, bind_name_cb);
  bobgui_widget_class_bind_template_callback (widget_class, bind_value_cb);

  bobgui_widget_class_set_layout_manager_type (widget_class, BOBGUI_TYPE_BIN_LAYOUT);
}

// vim: set et sw=2 ts=2:
