/*
 * Copyright © 2011 Canonical Limited
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
 * Author: Ryan Lortie <desrt@desrt.ca>
 */

#include "config.h"

#include "bobguiapplicationwindowprivate.h"

#include "bobguiapplicationprivate.h"
#include "bobguiwidgetprivate.h"
#include "bobguiwindowprivate.h"
#include "bobguipopovermenubar.h"
#include "bobguisettings.h"
#include "deprecated/bobguishortcutswindowprivate.h"
#include "bobguitooltipprivate.h"
#include "bobguiprivate.h"
#include "bobguitypebuiltins.h"

#include <glib/gi18n-lib.h>

/**
 * BobguiApplicationWindow:
 *
 * A `BobguiWindow` subclass that integrates with `BobguiApplication`.
 *
 * Notably, `BobguiApplicationWindow` can handle an application menubar.
 *
 * This class implements the [iface@Gio.ActionGroup] and [iface@Gio.ActionMap]
 * interfaces, to let you add window-specific actions that will be exported
 * by the associated [class@Bobgui.Application], together with its application-wide
 * actions. Window-specific actions are prefixed with the “win.”
 * prefix and application-wide actions are prefixed with the “app.”
 * prefix. Actions must be addressed with the prefixed name when
 * referring to them from a menu model.
 *
 * Note that widgets that are placed inside a `BobguiApplicationWindow`
 * can also activate these actions, if they implement the
 * [iface@Bobgui.Actionable] interface.
 *
 * The settings [property@Bobgui.Settings:bobgui-shell-shows-app-menu] and
 * [property@Bobgui.Settings:bobgui-shell-shows-menubar] tell BOBGUI whether the
 * desktop environment is showing the application menu and menubar
 * models outside the application as part of the desktop shell.
 * For instance, on OS X, both menus will be displayed remotely;
 * on Windows neither will be.
 *
 * If the desktop environment does not display the menubar, it can be shown in
 * the `BobguiApplicationWindow` by setting the
 * [property@Bobgui.ApplicationWindow:show-menubar] property to true. If the
 * desktop environment does not display the application menu, then it will
 * automatically be included in the menubar or in the window’s client-side
 * decorations.
 *
 * See [class@Bobgui.PopoverMenu] for information about the XML language
 * used by `BobguiBuilder` for menu models.
 *
 * See also: [method@Bobgui.Application.set_menubar].
 *
 * ## A BobguiApplicationWindow with a menubar
 *
 * The code sample below shows how to set up a `BobguiApplicationWindow`
 * with a menu bar defined on the [class@Bobgui.Application]:
 *
 * ```c
 * BobguiApplication *app = bobgui_application_new ("org.bobgui.test", 0);
 *
 * BobguiBuilder *builder = bobgui_builder_new_from_string (
 *     "<interface>"
 *     "  <menu id='menubar'>"
 *     "    <submenu>"
 *     "      <attribute name='label' translatable='yes'>_Edit</attribute>"
 *     "      <item>"
 *     "        <attribute name='label' translatable='yes'>_Copy</attribute>"
 *     "        <attribute name='action'>win.copy</attribute>"
 *     "      </item>"
 *     "      <item>"
 *     "        <attribute name='label' translatable='yes'>_Paste</attribute>"
 *     "        <attribute name='action'>win.paste</attribute>"
 *     "      </item>"
 *     "    </submenu>"
 *     "  </menu>"
 *     "</interface>",
 *     -1);
 *
 * GMenuModel *menubar = G_MENU_MODEL (bobgui_builder_get_object (builder, "menubar"));
 * bobgui_application_set_menubar (BOBGUI_APPLICATION (app), menubar);
 * g_object_unref (builder);
 *
 * // ...
 *
 * BobguiWidget *window = bobgui_application_window_new (app);
 * ```
 */

typedef GSimpleActionGroupClass BobguiApplicationWindowActionsClass;
typedef struct
{
  GSimpleActionGroup parent_instance;
  BobguiWindow *window;
} BobguiApplicationWindowActions;

static GType bobgui_application_window_actions_get_type   (void);
static void  bobgui_application_window_actions_iface_init (GRemoteActionGroupInterface *iface);
G_DEFINE_TYPE_WITH_CODE (BobguiApplicationWindowActions, bobgui_application_window_actions, G_TYPE_SIMPLE_ACTION_GROUP,
                         G_IMPLEMENT_INTERFACE (G_TYPE_REMOTE_ACTION_GROUP, bobgui_application_window_actions_iface_init))

static void
bobgui_application_window_actions_activate_action_full (GRemoteActionGroup *remote,
                                                     const char         *action_name,
                                                     GVariant           *parameter,
                                                     GVariant           *platform_data)
{
  BobguiApplicationWindowActions *actions = (BobguiApplicationWindowActions *) remote;
  GApplication *application;
  GApplicationClass *class;

  application = G_APPLICATION (bobgui_window_get_application (actions->window));
  class = G_APPLICATION_GET_CLASS (application);

  class->before_emit (application, platform_data);
  g_action_group_activate_action (G_ACTION_GROUP (actions), action_name, parameter);
  class->after_emit (application, platform_data);
}

static void
bobgui_application_window_actions_change_action_state_full (GRemoteActionGroup *remote,
                                                         const char         *action_name,
                                                         GVariant           *value,
                                                         GVariant           *platform_data)
{
  BobguiApplicationWindowActions *actions = (BobguiApplicationWindowActions *) remote;
  GApplication *application;
  GApplicationClass *class;

  application = G_APPLICATION (bobgui_window_get_application (actions->window));
  class = G_APPLICATION_GET_CLASS (application);

  class->before_emit (application, platform_data);
  g_action_group_change_action_state (G_ACTION_GROUP (actions), action_name, value);
  class->after_emit (application, platform_data);
}

static void
bobgui_application_window_actions_init (BobguiApplicationWindowActions *actions)
{
}

static void
bobgui_application_window_actions_iface_init (GRemoteActionGroupInterface *iface)
{
  iface->activate_action_full = bobgui_application_window_actions_activate_action_full;
  iface->change_action_state_full = bobgui_application_window_actions_change_action_state_full;
}

static void
bobgui_application_window_actions_class_init (BobguiApplicationWindowActionsClass *class)
{
}

static GSimpleActionGroup *
bobgui_application_window_actions_new (BobguiApplicationWindow *window)
{
  BobguiApplicationWindowActions *actions;

  actions = g_object_new (bobgui_application_window_actions_get_type (), NULL);
  actions->window = BOBGUI_WINDOW (window);

  return G_SIMPLE_ACTION_GROUP (actions);
}

/* Now onto BobguiApplicationWindow... */

typedef struct _BobguiApplicationWindowPrivate BobguiApplicationWindowPrivate;
struct _BobguiApplicationWindowPrivate
{
  GSimpleActionGroup *actions;
  BobguiWidget *menubar;

  gboolean show_menubar;
  guint id;
  GMenu *menubar_section;

  BobguiShortcutsWindow *help_overlay;
};

static void bobgui_application_window_group_iface_init (GActionGroupInterface *iface);
static void bobgui_application_window_map_iface_init (GActionMapInterface *iface);

G_DEFINE_TYPE_WITH_CODE (BobguiApplicationWindow, bobgui_application_window, BOBGUI_TYPE_WINDOW,
                         G_ADD_PRIVATE (BobguiApplicationWindow)
                         G_IMPLEMENT_INTERFACE (G_TYPE_ACTION_GROUP, bobgui_application_window_group_iface_init)
                         G_IMPLEMENT_INTERFACE (G_TYPE_ACTION_MAP, bobgui_application_window_map_iface_init))

static void
bobgui_application_window_update_menubar (BobguiApplicationWindow *window)
{
  BobguiApplicationWindowPrivate *priv = bobgui_application_window_get_instance_private (window);
  gboolean should_have_menubar;
  gboolean have_menubar;

  have_menubar = priv->menubar != NULL;

  should_have_menubar = priv->show_menubar &&
                        g_menu_model_get_n_items (G_MENU_MODEL (priv->menubar_section));

  if (have_menubar && !should_have_menubar)
    {
      bobgui_widget_unparent (priv->menubar);
      priv->menubar = NULL;
    }

  if (!have_menubar && should_have_menubar)
    {
      GMenu *combined;

      combined = g_menu_new ();
      g_menu_append_section (combined, NULL, G_MENU_MODEL (priv->menubar_section));

      priv->menubar = bobgui_popover_menu_bar_new_from_model (G_MENU_MODEL (combined));
      bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (priv->menubar),
                                      BOBGUI_ACCESSIBLE_PROPERTY_LABEL, _("Menu bar"),
                                      -1);
      bobgui_widget_set_parent (priv->menubar, BOBGUI_WIDGET (window));
      g_object_unref (combined);
    }
}

static void
bobgui_application_window_update_shell_shows_menubar (BobguiApplicationWindow *window,
                                                   BobguiSettings          *settings)
{
  BobguiApplicationWindowPrivate *priv = bobgui_application_window_get_instance_private (window);
  gboolean shown_by_shell;

  g_object_get (settings, "bobgui-shell-shows-menubar", &shown_by_shell, NULL);

  if (shown_by_shell)
    {
      /* the shell shows it, so don't show it locally */
      if (g_menu_model_get_n_items (G_MENU_MODEL (priv->menubar_section)) != 0)
        g_menu_remove (priv->menubar_section, 0);
    }
  else
    {
      /* the shell does not show it, so make sure we show it */
      if (g_menu_model_get_n_items (G_MENU_MODEL (priv->menubar_section)) == 0)
        {
          GMenuModel *menubar = NULL;

          if (bobgui_window_get_application (BOBGUI_WINDOW (window)) != NULL)
            menubar = bobgui_application_get_menubar (bobgui_window_get_application (BOBGUI_WINDOW (window)));

          if (menubar != NULL)
            g_menu_append_section (priv->menubar_section, NULL, menubar);
        }
    }
}

static void
bobgui_application_window_shell_shows_menubar_changed (GObject    *object,
                                                    GParamSpec *pspec,
                                                    gpointer    user_data)
{
  BobguiApplicationWindow *window = user_data;

  bobgui_application_window_update_shell_shows_menubar (window, BOBGUI_SETTINGS (object));
  bobgui_application_window_update_menubar (window);
}

static char **
bobgui_application_window_list_actions (GActionGroup *group)
{
  BobguiApplicationWindow *window = BOBGUI_APPLICATION_WINDOW (group);
  BobguiApplicationWindowPrivate *priv = bobgui_application_window_get_instance_private (window);

  /* may be NULL after dispose has run */
  if (!priv->actions)
    return g_new0 (char *, 0 + 1);

  return g_action_group_list_actions (G_ACTION_GROUP (priv->actions));
}

static gboolean
bobgui_application_window_query_action (GActionGroup        *group,
                                     const char          *action_name,
                                     gboolean            *enabled,
                                     const GVariantType **parameter_type,
                                     const GVariantType **state_type,
                                     GVariant           **state_hint,
                                     GVariant           **state)
{
  BobguiApplicationWindow *window = BOBGUI_APPLICATION_WINDOW (group);
  BobguiApplicationWindowPrivate *priv = bobgui_application_window_get_instance_private (window);

  if (!priv->actions)
    return FALSE;

  return g_action_group_query_action (G_ACTION_GROUP (priv->actions),
                                      action_name, enabled, parameter_type, state_type, state_hint, state);
}

static void
bobgui_application_window_activate_action (GActionGroup *group,
                                        const char   *action_name,
                                        GVariant     *parameter)
{
  BobguiApplicationWindow *window = BOBGUI_APPLICATION_WINDOW (group);
  BobguiApplicationWindowPrivate *priv = bobgui_application_window_get_instance_private (window);

  if (!priv->actions)
    return;

  g_action_group_activate_action (G_ACTION_GROUP (priv->actions), action_name, parameter);
}

static void
bobgui_application_window_change_action_state (GActionGroup *group,
                                            const char   *action_name,
                                            GVariant     *state)
{
  BobguiApplicationWindow *window = BOBGUI_APPLICATION_WINDOW (group);
  BobguiApplicationWindowPrivate *priv = bobgui_application_window_get_instance_private (window);

  if (!priv->actions)
    return;

  g_action_group_change_action_state (G_ACTION_GROUP (priv->actions), action_name, state);
}

static GAction *
bobgui_application_window_lookup_action (GActionMap  *action_map,
                                      const char *action_name)
{
  BobguiApplicationWindow *window = BOBGUI_APPLICATION_WINDOW (action_map);
  BobguiApplicationWindowPrivate *priv = bobgui_application_window_get_instance_private (window);

  if (!priv->actions)
    return NULL;

  return g_action_map_lookup_action (G_ACTION_MAP (priv->actions), action_name);
}

static void
bobgui_application_window_add_action (GActionMap *action_map,
                                   GAction    *action)
{
  BobguiApplicationWindow *window = BOBGUI_APPLICATION_WINDOW (action_map);
  BobguiApplicationWindowPrivate *priv = bobgui_application_window_get_instance_private (window);

  if (!priv->actions)
    return;

  g_action_map_add_action (G_ACTION_MAP (priv->actions), action);
}

static void
bobgui_application_window_remove_action (GActionMap  *action_map,
                                      const char *action_name)
{
  BobguiApplicationWindow *window = BOBGUI_APPLICATION_WINDOW (action_map);
  BobguiApplicationWindowPrivate *priv = bobgui_application_window_get_instance_private (window);

  if (!priv->actions)
    return;

  g_action_map_remove_action (G_ACTION_MAP (priv->actions), action_name);
}

static void
bobgui_application_window_group_iface_init (GActionGroupInterface *iface)
{
  iface->list_actions = bobgui_application_window_list_actions;
  iface->query_action = bobgui_application_window_query_action;
  iface->activate_action = bobgui_application_window_activate_action;
  iface->change_action_state = bobgui_application_window_change_action_state;
}

static void
bobgui_application_window_map_iface_init (GActionMapInterface *iface)
{
  iface->lookup_action = bobgui_application_window_lookup_action;
  iface->add_action = bobgui_application_window_add_action;
  iface->remove_action = bobgui_application_window_remove_action;
}

enum {
  PROP_0,
  PROP_SHOW_MENUBAR,
  N_PROPS
};
static GParamSpec *bobgui_application_window_properties[N_PROPS];

enum {
  SAVE_STATE,
  LAST_SIGNAL
};
static guint bobgui_application_window_signals[LAST_SIGNAL] = { 0 };

static void
bobgui_application_window_measure (BobguiWidget      *widget,
                                BobguiOrientation  orientation,
                                int             for_size,
                                int            *minimum,
                                int            *natural,
                                int            *minimum_baseline,
                                int            *natural_baseline)
{
  BobguiApplicationWindow *window = BOBGUI_APPLICATION_WINDOW (widget);
  BobguiApplicationWindowPrivate *priv = bobgui_application_window_get_instance_private (window);

  BOBGUI_WIDGET_CLASS (bobgui_application_window_parent_class)->measure (widget,
                                                                   orientation,
                                                                   for_size,
                                                                   minimum, natural,
                                                                   minimum_baseline, natural_baseline);

  if (priv->menubar != NULL)
    {
      int menubar_min, menubar_nat;

      if (orientation == BOBGUI_ORIENTATION_HORIZONTAL)
        {
          int menubar_height = 0;

          bobgui_widget_measure (priv->menubar, BOBGUI_ORIENTATION_VERTICAL,
                              -1, &menubar_height, NULL, NULL, NULL);


          BOBGUI_WIDGET_CLASS (bobgui_application_window_parent_class)->measure (widget,
                                                                           orientation,
                                                                           for_size > -1 ?
                                                                             for_size - menubar_height : -1,
                                                                           minimum, natural,
                                                                           minimum_baseline, natural_baseline);


          bobgui_widget_measure (priv->menubar, orientation, menubar_height, &menubar_min, &menubar_nat, NULL, NULL);

          *minimum = MAX (*minimum, menubar_min);
          *natural = MAX (*natural, menubar_nat);

        }
      else /* VERTICAL */
        {
          bobgui_widget_measure (priv->menubar, orientation, for_size, &menubar_min, &menubar_nat, NULL, NULL);
          *minimum += menubar_min;
          *natural += menubar_nat;
        }
    }
}

static void
bobgui_application_window_real_size_allocate (BobguiWidget *widget,
                                           int        width,
                                           int        height,
                                           int        baseline)
{
  BobguiApplicationWindow *window = BOBGUI_APPLICATION_WINDOW (widget);
  BobguiApplicationWindowPrivate *priv = bobgui_application_window_get_instance_private (window);

  if (priv->menubar != NULL)
    {
      BobguiAllocation menubar_allocation;
      BobguiAllocation child_allocation;
      int menubar_height;
      BobguiWidget *child;

      _bobgui_window_set_allocation (BOBGUI_WINDOW (widget), width, height, &child_allocation);
      menubar_allocation = child_allocation;

      bobgui_widget_measure (priv->menubar, BOBGUI_ORIENTATION_VERTICAL,
                          menubar_allocation.width,
                          &menubar_height, NULL, NULL, NULL);

      menubar_allocation.height = menubar_height;
      bobgui_widget_size_allocate  (priv->menubar, &menubar_allocation, -1);

      child_allocation.y += menubar_height;
      child_allocation.height -= menubar_height;
      if (baseline != -1)
        baseline -= child_allocation.y;
      child = bobgui_window_get_child (BOBGUI_WINDOW (window));
      if (child != NULL && bobgui_widget_get_visible (child))
        bobgui_widget_size_allocate (child, &child_allocation, baseline);

      bobgui_tooltip_maybe_allocate (BOBGUI_NATIVE (widget));
    }
  else
    BOBGUI_WIDGET_CLASS (bobgui_application_window_parent_class)->size_allocate (widget,
                                                                           width,
                                                                           height,
                                                                           baseline);
}

static void
bobgui_application_window_real_realize (BobguiWidget *widget)
{
  BobguiApplicationWindow *window = BOBGUI_APPLICATION_WINDOW (widget);
  BobguiSettings *settings;

  settings = bobgui_widget_get_settings (widget);

  g_signal_connect (settings, "notify::bobgui-shell-shows-menubar",
                    G_CALLBACK (bobgui_application_window_shell_shows_menubar_changed), window);

  BOBGUI_WIDGET_CLASS (bobgui_application_window_parent_class)->realize (widget);

  bobgui_application_window_update_shell_shows_menubar (window, settings);
  bobgui_application_window_update_menubar (window);
}

static void
bobgui_application_window_real_unrealize (BobguiWidget *widget)
{
  BobguiSettings *settings;

  settings = bobgui_widget_get_settings (widget);

  g_signal_handlers_disconnect_by_func (settings, bobgui_application_window_shell_shows_menubar_changed, widget);

  BOBGUI_WIDGET_CLASS (bobgui_application_window_parent_class)->unrealize (widget);
}

GActionGroup *
bobgui_application_window_get_action_group (BobguiApplicationWindow *window)
{
  BobguiApplicationWindowPrivate *priv = bobgui_application_window_get_instance_private (window);
  return G_ACTION_GROUP (priv->actions);
}

static void
bobgui_application_window_real_map (BobguiWidget *widget)
{
  BobguiApplicationWindow *window = BOBGUI_APPLICATION_WINDOW (widget);
  BobguiApplicationWindowPrivate *priv = bobgui_application_window_get_instance_private (window);

  /* XXX could eliminate this by tweaking bobgui_window_map */
  if (priv->menubar)
    bobgui_widget_map (priv->menubar);

  BOBGUI_WIDGET_CLASS (bobgui_application_window_parent_class)->map (widget);
}

static void
bobgui_application_window_real_unmap (BobguiWidget *widget)
{
  BobguiApplicationWindow *window = BOBGUI_APPLICATION_WINDOW (widget);
  BobguiApplicationWindowPrivate *priv = bobgui_application_window_get_instance_private (window);

  /* XXX could eliminate this by tweaking bobgui_window_unmap */
  if (priv->menubar)
    bobgui_widget_unmap (priv->menubar);

  BOBGUI_WIDGET_CLASS (bobgui_application_window_parent_class)->unmap (widget);
}

static void
bobgui_application_window_get_property (GObject    *object,
                                     guint       prop_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
  BobguiApplicationWindow *window = BOBGUI_APPLICATION_WINDOW (object);
  BobguiApplicationWindowPrivate *priv = bobgui_application_window_get_instance_private (window);

  switch (prop_id)
    {
    case PROP_SHOW_MENUBAR:
      g_value_set_boolean (value, priv->show_menubar);
      break;

    default:
      g_assert_not_reached ();
    }
}

static void
bobgui_application_window_set_property (GObject      *object,
                                     guint         prop_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  BobguiApplicationWindow *window = BOBGUI_APPLICATION_WINDOW (object);

  switch (prop_id)
    {
    case PROP_SHOW_MENUBAR:
      bobgui_application_window_set_show_menubar (window, g_value_get_boolean (value));
      break;

    default:
      g_assert_not_reached ();
    }
}

static void
bobgui_application_window_dispose (GObject *object)
{
  BobguiApplicationWindow *window = BOBGUI_APPLICATION_WINDOW (object);
  BobguiApplicationWindowPrivate *priv = bobgui_application_window_get_instance_private (window);

  if (priv->menubar)
    {
      bobgui_widget_unparent (priv->menubar);
      priv->menubar = NULL;
    }

  g_clear_object (&priv->menubar_section);

  if (priv->help_overlay)
    {
      bobgui_window_destroy (BOBGUI_WINDOW (priv->help_overlay));
      g_clear_object (&priv->help_overlay);
    }

  G_OBJECT_CLASS (bobgui_application_window_parent_class)->dispose (object);

  /* We do this below the chain-up above to give us a chance to be
   * removed from the BobguiApplication (which is done in the dispose
   * handler of BobguiWindow).
   *
   * That reduces our chances of being watched as a GActionGroup from a
   * muxer constructed by BobguiApplication.
   */
  g_clear_object (&priv->actions);
}

static void
bobgui_application_window_init (BobguiApplicationWindow *window)
{
  BobguiApplicationWindowPrivate *priv = bobgui_application_window_get_instance_private (window);

  priv->actions = bobgui_application_window_actions_new (window);
  priv->menubar_section = g_menu_new ();

  bobgui_widget_insert_action_group (BOBGUI_WIDGET (window), "win", G_ACTION_GROUP (priv->actions));

  /* priv->actions is the one and only ref on the group, so when
   * we dispose, the action group will die, disconnecting all signals.
   */
  g_signal_connect_swapped (priv->actions, "action-added",
                            G_CALLBACK (g_action_group_action_added), window);
  g_signal_connect_swapped (priv->actions, "action-enabled-changed",
                            G_CALLBACK (g_action_group_action_enabled_changed), window);
  g_signal_connect_swapped (priv->actions, "action-state-changed",
                            G_CALLBACK (g_action_group_action_state_changed), window);
  g_signal_connect_swapped (priv->actions, "action-removed",
                            G_CALLBACK (g_action_group_action_removed), window);
}

static void
bobgui_application_window_keys_changed (BobguiWindow *window)
{
  BobguiApplicationWindow *self = BOBGUI_APPLICATION_WINDOW (window);
  BobguiApplicationWindowPrivate *priv = bobgui_application_window_get_instance_private (self);

  BOBGUI_WINDOW_CLASS (bobgui_application_window_parent_class)->keys_changed (window);

  /* Notify key changes on the help overlay */
  if (priv->help_overlay != NULL)
    _bobgui_window_notify_keys_changed (BOBGUI_WINDOW (priv->help_overlay));
}

static void
bobgui_application_window_class_init (BobguiApplicationWindowClass *class)
{
  BobguiWindowClass *window_class = BOBGUI_WINDOW_CLASS (class);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (class);
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  window_class->keys_changed = bobgui_application_window_keys_changed;

  widget_class->measure = bobgui_application_window_measure;
  widget_class->size_allocate = bobgui_application_window_real_size_allocate;
  widget_class->realize = bobgui_application_window_real_realize;
  widget_class->unrealize = bobgui_application_window_real_unrealize;
  widget_class->map = bobgui_application_window_real_map;
  widget_class->unmap = bobgui_application_window_real_unmap;

  object_class->get_property = bobgui_application_window_get_property;
  object_class->set_property = bobgui_application_window_set_property;
  object_class->dispose = bobgui_application_window_dispose;

  /**
   * BobguiApplicationWindow:show-menubar:
   *
   * If this property is true, the window will display a menubar
   * unless it is shown by the desktop shell.
   *
   * See [method@Bobgui.Application.set_menubar].
   *
   * If false, the window will not display a menubar, regardless
   * of whether the desktop shell is showing it or not.
   */
  bobgui_application_window_properties[PROP_SHOW_MENUBAR] =
    g_param_spec_boolean ("show-menubar", NULL, NULL,
                          FALSE, G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, N_PROPS, bobgui_application_window_properties);

  /**
   * BobguiApplicationWindow::save-state:
   * @window: the window on which the signal is emitted
   * @dict: a dictionary of type `a{sv}`
   *
   * The handler for this signal should persist any
   * application-specific state of @window into @dict.
   *
   * Note that window management state such as maximized,
   * fullscreen, or window size should not be saved as
   * part of this, they are handled by BOBGUI.
   *
   * You must be careful to be robust in the face of app upgrades and downgrades:
   * the @state might have been created by a previous or occasionally even a future
   * version of your app. Do not assume that a given key exists in the state.
   * Apps must try to restore state saved by a previous version, but are free to
   * discard state if it was written by a future version.
   *
   * See [signal@Bobgui.Application::restore-window].
   *
   * Returns: true to stop stop further handlers from running
   *
   * Since: 4.24
   */
  bobgui_application_window_signals[SAVE_STATE] =
    g_signal_new (I_("save-state"),
                  G_TYPE_FROM_CLASS (class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BobguiApplicationWindowClass, save_state),
                  _bobgui_boolean_handled_accumulator, NULL,
                  NULL,
                  G_TYPE_BOOLEAN, 1,
                  G_TYPE_VARIANT_DICT);
}

/**
 * bobgui_application_window_new:
 * @application: an application
 *
 * Creates a new `BobguiApplicationWindow`.
 *
 * Returns: a newly created `BobguiApplicationWindow`
 */
BobguiWidget *
bobgui_application_window_new (BobguiApplication *application)
{
  g_return_val_if_fail (BOBGUI_IS_APPLICATION (application), NULL);

  return g_object_new (BOBGUI_TYPE_APPLICATION_WINDOW,
                       "application", application,
                       NULL);
}

/**
 * bobgui_application_window_get_show_menubar:
 * @window: an application window
 *
 * Returns whether the window will display a menubar for the app menu
 * and menubar as needed.
 *
 * Returns: True if the window will display a menubar when needed
 */
gboolean
bobgui_application_window_get_show_menubar (BobguiApplicationWindow *window)
{
  BobguiApplicationWindowPrivate *priv = bobgui_application_window_get_instance_private (window);
  return priv->show_menubar;
}

/**
 * bobgui_application_window_set_show_menubar:
 * @window: an application window
 * @show_menubar: whether to show a menubar when needed
 *
 * Sets whether the window will display a menubar for the app menu
 * and menubar as needed.
 */
void
bobgui_application_window_set_show_menubar (BobguiApplicationWindow *window,
                                         gboolean              show_menubar)
{
  BobguiApplicationWindowPrivate *priv = bobgui_application_window_get_instance_private (window);
  g_return_if_fail (BOBGUI_IS_APPLICATION_WINDOW (window));

  show_menubar = !!show_menubar;

  if (priv->show_menubar != show_menubar)
    {
      priv->show_menubar = show_menubar;

      bobgui_application_window_update_menubar (window);

      g_object_notify_by_pspec (G_OBJECT (window), bobgui_application_window_properties[PROP_SHOW_MENUBAR]);
    }
}

/**
 * bobgui_application_window_get_id:
 * @window: an application window
 *
 * Returns the unique ID of the window.
 *
 *  If the window has not yet been added to a `BobguiApplication`, returns `0`.
 *
 * Returns: the unique ID for the window, or `0` if the window
 *   has not yet been added to an application
 */
guint
bobgui_application_window_get_id (BobguiApplicationWindow *window)
{
  BobguiApplicationWindowPrivate *priv = bobgui_application_window_get_instance_private (window);
  g_return_val_if_fail (BOBGUI_IS_APPLICATION_WINDOW (window), 0);

  return priv->id;
}

void
bobgui_application_window_set_id (BobguiApplicationWindow *window,
                               guint                 id)
{
  BobguiApplicationWindowPrivate *priv = bobgui_application_window_get_instance_private (window);
  g_return_if_fail (BOBGUI_IS_APPLICATION_WINDOW (window));
  priv->id = id;
}

static void
show_help_overlay (GSimpleAction *action,
                   GVariant      *parameter,
                   gpointer       user_data)
{
  BobguiApplicationWindow *window = user_data;
  BobguiApplicationWindowPrivate *priv = bobgui_application_window_get_instance_private (window);

  if (priv->help_overlay)
    bobgui_window_present (BOBGUI_WINDOW (priv->help_overlay));
}

/**
 * bobgui_application_window_set_help_overlay:
 * @window: an application window
 * @help_overlay: (nullable): a shortcuts window
 *
 * Associates a shortcuts window with the application window.
 *
 * Additionally, sets up an action with the name
 * `win.show-help-overlay` to present it.
 *
 * The window takes responsibility for destroying the help overlay.
 *
 * Deprecated: 4.18: `BobguiShortcutsWindow` will be removed in BOBGUI 5
 */
void
bobgui_application_window_set_help_overlay (BobguiApplicationWindow *window,
                                         BobguiShortcutsWindow   *help_overlay)
{
  BobguiApplicationWindowPrivate *priv = bobgui_application_window_get_instance_private (window);
  g_return_if_fail (BOBGUI_IS_APPLICATION_WINDOW (window));
  g_return_if_fail (help_overlay == NULL || BOBGUI_IS_SHORTCUTS_WINDOW (help_overlay));

  if (priv->help_overlay)
    bobgui_window_destroy (BOBGUI_WINDOW (priv->help_overlay));
  g_set_object (&priv->help_overlay, help_overlay);

  if (!priv->help_overlay)
    return;

  bobgui_window_set_modal (BOBGUI_WINDOW (help_overlay), TRUE);
  bobgui_window_set_hide_on_close (BOBGUI_WINDOW (help_overlay), TRUE);
  bobgui_window_set_transient_for (BOBGUI_WINDOW (help_overlay), BOBGUI_WINDOW (window));
  bobgui_shortcuts_window_set_window (help_overlay, BOBGUI_WINDOW (window));

  if (!g_action_map_lookup_action (G_ACTION_MAP (priv->actions), "show-help-overlay"))
    {
      GSimpleAction *action;

      action = g_simple_action_new ("show-help-overlay", NULL);
      g_signal_connect (action, "activate", G_CALLBACK (show_help_overlay), window);

      g_action_map_add_action (G_ACTION_MAP (priv->actions), G_ACTION (action));
      g_object_unref (G_OBJECT (action));
    }
}

/**
 * bobgui_application_window_get_help_overlay:
 * @window: an application window
 *
 * Gets the `BobguiShortcutsWindow` that is associated with @window.
 *
 * See [method@Bobgui.ApplicationWindow.set_help_overlay].
 *
 * Returns: (transfer none) (nullable): the help overlay associated
 *   with the window
 *
 * Deprecated: 4.18: `BobguiShortcutsWindow` will be removed in BOBGUI 5
 */
BobguiShortcutsWindow *
bobgui_application_window_get_help_overlay (BobguiApplicationWindow *window)
{
  BobguiApplicationWindowPrivate *priv = bobgui_application_window_get_instance_private (window);
  g_return_val_if_fail (BOBGUI_IS_APPLICATION_WINDOW (window), NULL);

  return priv->help_overlay;
}

/*< private >
 * bobgui_application_window_save:
 * @window: a `BobguiApplicationWindow`
 * @state: a `GVariantDict` to add state to
 *
 * Save the state of @window and its children to a `GVariant`.
 *
 * See [signal@Bobgui.ApplicationWindow::save-state] for how to override
 * what state is saved.
 */
void
bobgui_application_window_save (BobguiApplicationWindow *window,
                             GVariantDict         *state)
{
  gboolean ret;

  g_signal_emit (window, bobgui_application_window_signals[SAVE_STATE], 0, state, &ret);
}
