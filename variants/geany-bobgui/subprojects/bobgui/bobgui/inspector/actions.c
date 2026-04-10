/*
 * Copyright (c) 2014 Red Hat, Inc.
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

#include "actions.h"
#include "action-editor.h"
#include "action-holder.h"

#include "bobguiapplication.h"
#include "bobguiapplicationwindow.h"
#include "bobguiwidgetprivate.h"
#include "bobguiactionmuxerprivate.h"
#include "bobguiactionobserverprivate.h"
#include "bobguiactionobservableprivate.h"
#include "bobguipopover.h"
#include "bobguilabel.h"
#include "bobguistack.h"
#include "bobguilistbox.h"
#include "bobguisizegroup.h"
#include "bobguiboxlayout.h"


struct _BobguiInspectorActions
{
  BobguiWidget parent;

  BobguiWidget *swin;
  BobguiWidget *list;
  BobguiWidget *button;

  GObject *object;

  GListStore *actions;
  BobguiSortListModel *sorted;
  BobguiColumnViewColumn *name;
};

typedef struct _BobguiInspectorActionsClass
{
  BobguiWidgetClass parent;
} BobguiInspectorActionsClass;

enum {
  PROP_0,
  PROP_BUTTON
};

static void bobgui_inspector_actions_observer_iface_init (BobguiActionObserverInterface *iface);

G_DEFINE_TYPE_WITH_CODE (BobguiInspectorActions, bobgui_inspector_actions, BOBGUI_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_ACTION_OBSERVER, bobgui_inspector_actions_observer_iface_init))

static void
bobgui_inspector_actions_init (BobguiInspectorActions *sl)
{
 BobguiBoxLayout *layout;

  bobgui_widget_init_template (BOBGUI_WIDGET (sl));

  layout = BOBGUI_BOX_LAYOUT (bobgui_widget_get_layout_manager (BOBGUI_WIDGET (sl)));
  bobgui_orientable_set_orientation (BOBGUI_ORIENTABLE (layout), BOBGUI_ORIENTATION_VERTICAL);
}

static void
action_added (GObject             *owner,
              const char          *action_name,
              BobguiInspectorActions *sl)
{
  ActionHolder *holder = action_holder_new (owner, action_name);
  g_list_store_append (sl->actions, holder);
  g_object_unref (holder);
}

static void
setup_name_cb (BobguiSignalListItemFactory *factory,
               BobguiListItem              *list_item)
{
  BobguiWidget *label;

  label = bobgui_label_new (NULL);
  bobgui_label_set_xalign (BOBGUI_LABEL (label), 0.0);
  bobgui_widget_add_css_class (label, "cell");
  bobgui_list_item_set_child (list_item, label);
}

static void
bind_name_cb (BobguiSignalListItemFactory *factory,
              BobguiListItem              *list_item)
{
  gpointer item;
  BobguiWidget *label;

  item = bobgui_list_item_get_item (list_item);
  label = bobgui_list_item_get_child (list_item);

  bobgui_label_set_label (BOBGUI_LABEL (label), action_holder_get_name (ACTION_HOLDER (item)));
}

static void
setup_enabled_cb (BobguiSignalListItemFactory *factory,
                  BobguiListItem              *list_item)
{
  BobguiWidget *label;

  label = bobgui_label_new (NULL);
  bobgui_label_set_xalign (BOBGUI_LABEL (label), 0.5);
  bobgui_widget_add_css_class (label, "cell");
  bobgui_list_item_set_child (list_item, label);
}

static void
update_enabled (ActionHolder *holder,
                BobguiLabel     *label)
{
  GObject *owner = action_holder_get_owner (holder);
  const char *name = action_holder_get_name (holder);
  gboolean enabled = FALSE;

  if (G_IS_ACTION_GROUP (owner))
    enabled = g_action_group_get_action_enabled (G_ACTION_GROUP (owner), name);
  else if (!BOBGUI_IS_ACTION_MUXER (owner) ||
           !bobgui_action_muxer_query_action (BOBGUI_ACTION_MUXER (owner), name,
                                           &enabled, NULL, NULL, NULL, NULL))
    enabled = FALSE;

  bobgui_label_set_label (label, enabled ? "+" : "-");
}

static void
bind_enabled_cb (BobguiSignalListItemFactory *factory,
                 BobguiListItem              *list_item)
{
  gpointer item = bobgui_list_item_get_item (list_item);
  BobguiWidget *label = bobgui_list_item_get_child (list_item);

  g_signal_connect (item, "changed", G_CALLBACK (update_enabled), label);

  update_enabled (ACTION_HOLDER (item), BOBGUI_LABEL (label));
}

static void
unbind_enabled_cb (BobguiSignalListItemFactory *factory,
                   BobguiListItem              *list_item)
{
  gpointer item = bobgui_list_item_get_item (list_item);
  BobguiWidget *label = bobgui_list_item_get_child (list_item);

  g_signal_handlers_disconnect_by_func (item, update_enabled, label);
}

static void
setup_parameter_cb (BobguiSignalListItemFactory *factory,
                    BobguiListItem              *list_item)
{
  BobguiWidget *label;

  label = bobgui_label_new (NULL);
  bobgui_label_set_xalign (BOBGUI_LABEL (label), 0.5);
  bobgui_widget_add_css_class (label, "cell");
  bobgui_list_item_set_child (list_item, label);
}

static void
bind_parameter_cb (BobguiSignalListItemFactory *factory,
                   BobguiListItem              *list_item)
{
  gpointer item;
  BobguiWidget *label;
  GObject *owner;
  const char *name;
  const char *parameter;

  item = bobgui_list_item_get_item (list_item);
  label = bobgui_list_item_get_child (list_item);

  owner = action_holder_get_owner (ACTION_HOLDER (item));
  name = action_holder_get_name (ACTION_HOLDER (item));
  if (G_IS_ACTION_GROUP (owner))
    parameter = (const char *)g_action_group_get_action_parameter_type (G_ACTION_GROUP (owner), name);
  else if (!BOBGUI_IS_ACTION_MUXER (owner) ||
           !bobgui_action_muxer_query_action (BOBGUI_ACTION_MUXER (owner), name,
                                           NULL, (const GVariantType **)&parameter, NULL, NULL, NULL))
    parameter = "(Unknown)";

  bobgui_label_set_label (BOBGUI_LABEL (label), parameter);
}

static void
setup_state_cb (BobguiSignalListItemFactory *factory,
                BobguiListItem              *list_item)
{
  BobguiWidget *label;

  label = bobgui_label_new (NULL);
  bobgui_widget_set_margin_start (label, 5);
  bobgui_widget_set_margin_end (label, 5);
  bobgui_label_set_xalign (BOBGUI_LABEL (label), 0.0);
  bobgui_widget_add_css_class (label, "cell");
  bobgui_list_item_set_child (list_item, label);
}

static void
update_state (ActionHolder *h,
              BobguiLabel     *label)
{
  GObject *owner = action_holder_get_owner (h);
  const char *name = action_holder_get_name (h);
  GVariant *state;

  if (G_IS_ACTION_GROUP (owner))
    state = g_action_group_get_action_state (G_ACTION_GROUP (owner), name);
  else if (!BOBGUI_IS_ACTION_MUXER (owner) ||
           !bobgui_action_muxer_query_action (BOBGUI_ACTION_MUXER (owner), name,
                                           NULL, NULL, NULL, NULL, &state))
    state = NULL;

  if (state)
    {
      char *state_string;

      state_string = g_variant_print (state, FALSE);
      bobgui_label_set_label (label, state_string);
      g_free (state_string);
      g_variant_unref (state);
    }
  else
    bobgui_label_set_label (label, "");
}

static void
bind_state_cb (BobguiSignalListItemFactory *factory,
               BobguiListItem              *list_item)
{
  gpointer item = bobgui_list_item_get_item (list_item);
  BobguiWidget *label = bobgui_list_item_get_child (list_item);

  g_signal_connect (item, "changed", G_CALLBACK (update_state), label);

  update_state (ACTION_HOLDER (item), BOBGUI_LABEL (label));
}

static void
unbind_state_cb (BobguiSignalListItemFactory *factory,
                 BobguiListItem              *list_item)
{
  gpointer item = bobgui_list_item_get_item (list_item);
  BobguiWidget *label = bobgui_list_item_get_child (list_item);

  g_signal_handlers_disconnect_by_func (item, update_state, label);
}

static void
setup_changes_cb (BobguiSignalListItemFactory *factory,
                  BobguiListItem              *list_item)
{
  BobguiWidget *editor;

  editor = bobgui_inspector_action_editor_new ();
  bobgui_widget_add_css_class (editor, "cell");
  bobgui_list_item_set_child (list_item, editor);
}

static void
update_changes (ActionHolder             *h,
                BobguiInspectorActionEditor *editor)
{
  bobgui_inspector_action_editor_update (editor);
}

static void
bind_changes_cb (BobguiSignalListItemFactory *factory,
                 BobguiListItem              *list_item)
{
  gpointer item = bobgui_list_item_get_item (list_item);
  BobguiWidget *editor = bobgui_list_item_get_child (list_item);
  GObject *owner = action_holder_get_owner (ACTION_HOLDER (item));
  const char *name = action_holder_get_name (ACTION_HOLDER (item));

  bobgui_inspector_action_editor_set (BOBGUI_INSPECTOR_ACTION_EDITOR (editor), owner, name);

  g_signal_connect (item, "changed", G_CALLBACK (update_changes), editor);
}

static void
unbind_changes_cb (BobguiSignalListItemFactory *factory,
                   BobguiListItem              *list_item)
{
  gpointer item = bobgui_list_item_get_item (list_item);
  BobguiWidget *editor = bobgui_list_item_get_child (list_item);

  g_signal_handlers_disconnect_by_func (item, update_changes, editor);
}

static void
add_group (BobguiInspectorActions *sl,
           GActionGroup        *group)
{
  int i;
  char **names;

  names = g_action_group_list_actions (group);
  for (i = 0; names[i]; i++)
    action_added (G_OBJECT (group), names[i], sl);
  g_strfreev (names);
}

static void
add_muxer (BobguiInspectorActions *sl,
           BobguiActionMuxer      *muxer)
{
  int i;
  char **names;

  names = bobgui_action_muxer_list_actions (muxer, FALSE);
  for (i = 0; names[i]; i++)
    action_added (G_OBJECT (muxer), names[i], sl);
  g_strfreev (names);
}

static gboolean
reload (BobguiInspectorActions *sl)
{
  gboolean loaded = FALSE;

  g_object_unref (sl->actions);
  sl->actions = g_list_store_new (ACTION_TYPE_HOLDER);

  if (BOBGUI_IS_APPLICATION (sl->object))
    {
      add_group (sl, G_ACTION_GROUP (sl->object));
      loaded = TRUE;
    }
  else if (BOBGUI_IS_WIDGET (sl->object))
    {
      BobguiActionMuxer *muxer;

      muxer = _bobgui_widget_get_action_muxer (BOBGUI_WIDGET (sl->object), FALSE);
      if (muxer)
        {
          add_muxer (sl, muxer);
          loaded = TRUE;
        }
    }

  bobgui_sort_list_model_set_model (sl->sorted, G_LIST_MODEL (sl->actions));

  return loaded;
}

static void
refresh_all (BobguiInspectorActions *sl)
{
  reload (sl);
}

static void
action_changed (BobguiInspectorActions *sl,
                const char          *name)
{
  unsigned int n_actions;

  n_actions = g_list_model_get_n_items (G_LIST_MODEL (sl->actions));
  for (unsigned int i = 0; i < n_actions; i++)
    {
      ActionHolder *h = ACTION_HOLDER (g_list_model_get_item (G_LIST_MODEL (sl->actions), i));

      if (g_str_equal (action_holder_get_name (h), name))
        {
          action_holder_changed (h);
          g_object_unref (h);
          break;
        }

      g_object_unref (h);
    }
}

static void
action_enabled_changed (GActionGroup        *group,
                        const char          *action_name,
                        gboolean             enabled,
                        BobguiInspectorActions *sl)
{
  action_changed (sl, action_name);
}

static void
action_state_changed (GActionGroup        *group,
                      const char          *action_name,
                      GVariant            *state,
                      BobguiInspectorActions *sl)
{
  action_changed (sl, action_name);
}

static void
observer_action_added (BobguiActionObserver    *observer,
                       BobguiActionObservable  *observable,
                       const char           *action_name,
                       const GVariantType   *parameter_type,
                       gboolean              enabled,
                       GVariant             *state)
{
}

static void
observer_action_removed (BobguiActionObserver   *observer,
                         BobguiActionObservable *observable,
                         const char          *action_name)
{
}

static void
observer_action_enabled_changed (BobguiActionObserver   *observer,
                                 BobguiActionObservable *observable,
                                 const char          *action_name,
                                 gboolean             enabled)
{
  action_changed (BOBGUI_INSPECTOR_ACTIONS (observer), action_name);
}

static void
observer_action_state_changed (BobguiActionObserver   *observer,
                               BobguiActionObservable *observable,
                               const char          *action_name,
                               GVariant            *state)
{
  action_changed (BOBGUI_INSPECTOR_ACTIONS (observer), action_name);
}

static void
observer_primary_accel_changed (BobguiActionObserver   *observer,
                                BobguiActionObservable *observable,
                                const char          *action_name,
                                const char          *action_and_target)
{
}

static void
bobgui_inspector_actions_observer_iface_init (BobguiActionObserverInterface *iface)
{
  iface->action_added = observer_action_added;
  iface->action_removed = observer_action_removed;
  iface->action_enabled_changed = observer_action_enabled_changed;
  iface->action_state_changed = observer_action_state_changed;
  iface->primary_accel_changed = observer_primary_accel_changed;
}

static void
bobgui_inspector_actions_connect (BobguiInspectorActions *sl)
{
  if (G_IS_ACTION_GROUP (sl->object))
    {
      g_signal_connect (sl->object, "action-enabled-changed",
                        G_CALLBACK (action_enabled_changed), sl);
      g_signal_connect (sl->object, "action-state-changed",
                        G_CALLBACK (action_state_changed), sl);
    }
  else if (BOBGUI_IS_WIDGET (sl->object))
    {
      BobguiActionMuxer *muxer;

      muxer = _bobgui_widget_get_action_muxer (BOBGUI_WIDGET (sl->object), FALSE);

      if (muxer)
        {
          int i;
          char **names;
  
          names = bobgui_action_muxer_list_actions (muxer, FALSE);
          for (i = 0; names[i]; i++)
            {
              bobgui_action_observable_register_observer (BOBGUI_ACTION_OBSERVABLE (muxer), names[i], BOBGUI_ACTION_OBSERVER (sl));
            }
          g_strfreev (names);
        }
    }
}

static void
bobgui_inspector_actions_disconnect (BobguiInspectorActions *sl)
{
  if (G_IS_ACTION_GROUP (sl->object))
    {
      g_signal_handlers_disconnect_by_func (sl->object, action_enabled_changed, sl);
      g_signal_handlers_disconnect_by_func (sl->object, action_state_changed, sl);
    }
  else if (BOBGUI_IS_WIDGET (sl->object))
    {
      BobguiActionMuxer *muxer;

      muxer = _bobgui_widget_get_action_muxer (BOBGUI_WIDGET (sl->object), FALSE);

      if (muxer)
        {
          int i;
          char **names;

          names = bobgui_action_muxer_list_actions (muxer, FALSE);
          for (i = 0; names[i]; i++)
            bobgui_action_observable_unregister_observer (BOBGUI_ACTION_OBSERVABLE (muxer), names[i], BOBGUI_ACTION_OBSERVER (sl));
          g_strfreev (names);
        }
    }
}

void
bobgui_inspector_actions_set_object (BobguiInspectorActions *sl,
                                  GObject             *object)
{
  BobguiWidget *stack;
  BobguiStackPage *page;
  gboolean loaded;

  stack = bobgui_widget_get_parent (BOBGUI_WIDGET (sl));
  page = bobgui_stack_get_page (BOBGUI_STACK (stack), BOBGUI_WIDGET (sl));
  bobgui_stack_page_set_visible (page, FALSE);

  if (sl->object)
    bobgui_inspector_actions_disconnect (sl);

  g_set_object (&sl->object, object);

  bobgui_column_view_sort_by_column (BOBGUI_COLUMN_VIEW (sl->list), sl->name, BOBGUI_SORT_ASCENDING);
  loaded = reload (sl);
  bobgui_stack_page_set_visible (page, loaded);

  if (sl->object)
    bobgui_inspector_actions_connect (sl);
}

static void
get_property (GObject    *object,
              guint       param_id,
              GValue     *value,
              GParamSpec *pspec)
{
  BobguiInspectorActions *sl = BOBGUI_INSPECTOR_ACTIONS (object);

  switch (param_id)
    {
    case PROP_BUTTON:
      g_value_set_object (value, sl->button);
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
  BobguiInspectorActions *sl = BOBGUI_INSPECTOR_ACTIONS (object);

  switch (param_id)
    {
    case PROP_BUTTON:
      sl->button = g_value_get_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

static char *
holder_name (gpointer item)
{
  return g_strdup (action_holder_get_name (ACTION_HOLDER (item)));
}

static void
constructed (GObject *object)
{
  BobguiInspectorActions *sl = BOBGUI_INSPECTOR_ACTIONS (object);
  BobguiSorter *sorter;
  GListModel *model;

  g_signal_connect_swapped (sl->button, "clicked",
                            G_CALLBACK (refresh_all), sl);

  sorter = BOBGUI_SORTER (bobgui_string_sorter_new (bobgui_cclosure_expression_new (G_TYPE_STRING,
                                                               NULL,
                                                               0, NULL,
                                                               (GCallback)holder_name,
                                                               NULL, NULL)));
  bobgui_column_view_column_set_sorter (sl->name, sorter);
  g_object_unref (sorter);

  sl->actions = g_list_store_new (ACTION_TYPE_HOLDER);
  sl->sorted = bobgui_sort_list_model_new (g_object_ref (G_LIST_MODEL (sl->actions)),
                                        g_object_ref (bobgui_column_view_get_sorter (BOBGUI_COLUMN_VIEW (sl->list))));
  model = G_LIST_MODEL (bobgui_no_selection_new (g_object_ref (G_LIST_MODEL (sl->sorted))));
  bobgui_column_view_set_model (BOBGUI_COLUMN_VIEW (sl->list), BOBGUI_SELECTION_MODEL (model));
  g_object_unref (model);
}

static void
dispose (GObject *object)
{
  BobguiInspectorActions *sl = BOBGUI_INSPECTOR_ACTIONS (object);

  if (sl->object)
    bobgui_inspector_actions_disconnect (sl);

  g_clear_object (&sl->sorted);
  g_clear_object (&sl->actions);
  g_clear_object (&sl->object);

  bobgui_widget_dispose_template (BOBGUI_WIDGET (sl), BOBGUI_TYPE_INSPECTOR_ACTIONS);

  G_OBJECT_CLASS (bobgui_inspector_actions_parent_class)->dispose (object);
}

static void
bobgui_inspector_actions_class_init (BobguiInspectorActionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);

  object_class->dispose = dispose;
  object_class->get_property = get_property;
  object_class->set_property = set_property;
  object_class->constructed = constructed;

  g_object_class_install_property (object_class, PROP_BUTTON,
      g_param_spec_object ("button", NULL, NULL,
                           BOBGUI_TYPE_WIDGET, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

  bobgui_widget_class_set_template_from_resource (widget_class, "/org/bobgui/libbobgui/inspector/actions.ui");
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorActions, swin);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorActions, list);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorActions, name);
  bobgui_widget_class_bind_template_callback (widget_class, setup_name_cb);
  bobgui_widget_class_bind_template_callback (widget_class, bind_name_cb);
  bobgui_widget_class_bind_template_callback (widget_class, setup_enabled_cb);
  bobgui_widget_class_bind_template_callback (widget_class, bind_enabled_cb);
  bobgui_widget_class_bind_template_callback (widget_class, unbind_enabled_cb);
  bobgui_widget_class_bind_template_callback (widget_class, setup_parameter_cb);
  bobgui_widget_class_bind_template_callback (widget_class, bind_parameter_cb);
  bobgui_widget_class_bind_template_callback (widget_class, setup_state_cb);
  bobgui_widget_class_bind_template_callback (widget_class, bind_state_cb);
  bobgui_widget_class_bind_template_callback (widget_class, unbind_state_cb);
  bobgui_widget_class_bind_template_callback (widget_class, setup_changes_cb);
  bobgui_widget_class_bind_template_callback (widget_class, bind_changes_cb);
  bobgui_widget_class_bind_template_callback (widget_class, unbind_changes_cb);

  bobgui_widget_class_set_layout_manager_type (widget_class, BOBGUI_TYPE_BOX_LAYOUT);
}

// vim: set et sw=2 ts=2:
