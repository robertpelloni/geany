#include "bobguiworkbench.h"
#include "bobguiactionregistry.h"
#include "bobguicommandpalette.h"
#include <gdk/gdk.h>
#include <gio/gio.h>

struct _BobguiWorkbench
{
  GObject parent_instance;
  BobguiApplication *application;
  BobguiApplicationWindow *window;
  BobguiHeaderBar *header_bar;
  BobguiBox *root_box;
  BobguiBox *toolbar_box;
  BobguiPaned *outer_paned;
  BobguiPaned *inner_paned;
  BobguiBox *header_actions;
  BobguiLabel *title_label;
  BobguiLabel *status_label;
  BobguiWidget *central;
  BobguiWidget *left_sidebar;
  BobguiWidget *right_sidebar;
  BobguiActionRegistry *action_registry;
  BobguiCommandPalette *command_palette;
};

G_DEFINE_TYPE (BobguiWorkbench, bobgui_workbench, G_TYPE_OBJECT)

static void
bobgui_workbench_dispose (GObject *object)
{
  BobguiWorkbench *self = BOBGUI_WORKBENCH (object);

  g_clear_object (&self->window);
  g_clear_object (&self->action_registry);
  g_clear_object (&self->command_palette);

  G_OBJECT_CLASS (bobgui_workbench_parent_class)->dispose (object);
}

static void
bobgui_workbench_class_init (BobguiWorkbenchClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  object_class->dispose = bobgui_workbench_dispose;
}

static void
bobgui_workbench_init (BobguiWorkbench *self)
{
}

static char *
bobgui_workbench_to_action_name (const char *command_id)
{
  char *name = g_strdup (command_id);
  for (char *p = name; p && *p; p++)
    if (*p == '.')
      *p = '-';
  return name;
}

typedef struct
{
  BobguiWorkbenchCommandCallback callback;
  gpointer user_data;
  char *command_id;
} BobguiWorkbenchRegisteredAction;

static void
bobgui_workbench_registered_action_free (BobguiWorkbenchRegisteredAction *registered)
{
  g_free (registered->command_id);
  g_free (registered);
}

static void
bobgui_workbench_application_action_activate (GSimpleAction *action,
                                              GVariant      *parameter,
                                              gpointer       user_data)
{
  BobguiWorkbenchRegisteredAction *registered = user_data;

  (void) action;
  (void) parameter;

  if (registered->callback)
    registered->callback (registered->command_id, registered->user_data);
}

static gboolean
bobgui_workbench_key_pressed (BobguiEventControllerKey *controller,
                              guint                     keyval,
                              guint                     keycode,
                              GdkModifierType           state,
                              gpointer                  user_data)
{
  BobguiWorkbench *self = BOBGUI_WORKBENCH (user_data);

  (void) controller;
  (void) keycode;

  if ((state & GDK_CONTROL_MASK) != 0 &&
      (state & GDK_SHIFT_MASK) != 0 &&
      (keyval == GDK_KEY_P || keyval == GDK_KEY_p))
    {
      if (self->command_palette)
        bobgui_command_palette_present (self->command_palette);
      return TRUE;
    }

  return FALSE;
}

BobguiWorkbench *
bobgui_workbench_new (BobguiApplication *application)
{
  BobguiWorkbench *self = g_object_new (BOBGUI_TYPE_WORKBENCH, NULL);
  BobguiEventController *key_controller;
  BobguiWidget *title_box;

  self->application = application;
  self->window = g_object_new (BOBGUI_TYPE_APPLICATION_WINDOW,
                               "application", application,
                               NULL);

  self->header_bar = BOBGUI_HEADER_BAR (bobgui_header_bar_new ());
  self->header_actions = BOBGUI_BOX (bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 6));
  self->title_label = BOBGUI_LABEL (bobgui_label_new ("Bobgui Workbench"));
  self->status_label = BOBGUI_LABEL (bobgui_label_new ("Ready"));
  self->root_box = BOBGUI_BOX (bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0));
  self->toolbar_box = BOBGUI_BOX (bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 6));
  self->outer_paned = BOBGUI_PANED (bobgui_paned_new (BOBGUI_ORIENTATION_HORIZONTAL));
  self->inner_paned = BOBGUI_PANED (bobgui_paned_new (BOBGUI_ORIENTATION_HORIZONTAL));

  title_box = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
  bobgui_box_append (BOBGUI_BOX (title_box), BOBGUI_WIDGET (self->title_label));
  bobgui_header_bar_set_title_widget (self->header_bar, title_box);
  bobgui_header_bar_pack_end (self->header_bar, BOBGUI_WIDGET (self->header_actions));

  bobgui_widget_set_visible (BOBGUI_WIDGET (self->toolbar_box), FALSE);
  bobgui_box_append (self->root_box, BOBGUI_WIDGET (self->toolbar_box));
  bobgui_box_append (self->root_box, BOBGUI_WIDGET (self->outer_paned));
  bobgui_box_append (self->root_box, BOBGUI_WIDGET (self->status_label));

  bobgui_paned_set_end_child (self->outer_paned, BOBGUI_WIDGET (self->inner_paned));
  bobgui_window_set_titlebar (BOBGUI_WINDOW (self->window), BOBGUI_WIDGET (self->header_bar));
  bobgui_window_set_child (BOBGUI_WINDOW (self->window), BOBGUI_WIDGET (self->root_box));

  key_controller = bobgui_event_controller_key_new ();
  g_signal_connect (key_controller, "key-pressed",
                    G_CALLBACK (bobgui_workbench_key_pressed), self);
  bobgui_widget_add_controller (BOBGUI_WIDGET (self->window), key_controller);

  return self;
}

BobguiWindow *
bobgui_workbench_get_window (BobguiWorkbench *self)
{
  g_return_val_if_fail (BOBGUI_IS_WORKBENCH (self), NULL);
  return BOBGUI_WINDOW (self->window);
}

BobguiWidget *
bobgui_workbench_get_content (BobguiWorkbench *self)
{
  g_return_val_if_fail (BOBGUI_IS_WORKBENCH (self), NULL);
  return BOBGUI_WIDGET (self->inner_paned);
}

BobguiWidget *
bobgui_workbench_get_left_sidebar (BobguiWorkbench *self)
{
  g_return_val_if_fail (BOBGUI_IS_WORKBENCH (self), NULL);
  return self->left_sidebar;
}

BobguiWidget *
bobgui_workbench_get_right_sidebar (BobguiWorkbench *self)
{
  g_return_val_if_fail (BOBGUI_IS_WORKBENCH (self), NULL);
  return self->right_sidebar;
}

BobguiWidget *
bobgui_workbench_get_central (BobguiWorkbench *self)
{
  g_return_val_if_fail (BOBGUI_IS_WORKBENCH (self), NULL);
  return self->central;
}

void
bobgui_workbench_set_title (BobguiWorkbench *self,
                            const char      *title)
{
  g_return_if_fail (BOBGUI_IS_WORKBENCH (self));

  bobgui_window_set_title (BOBGUI_WINDOW (self->window), title);
  bobgui_label_set_text (self->title_label, title);
}

void
bobgui_workbench_set_central (BobguiWorkbench *self,
                              BobguiWidget    *child)
{
  g_return_if_fail (BOBGUI_IS_WORKBENCH (self));

  self->central = child;
  bobgui_paned_set_start_child (self->inner_paned, child);
}

void
bobgui_workbench_set_left_sidebar (BobguiWorkbench *self,
                                   BobguiWidget    *child)
{
  g_return_if_fail (BOBGUI_IS_WORKBENCH (self));

  self->left_sidebar = child;
  bobgui_paned_set_start_child (self->outer_paned, child);
}

void
bobgui_workbench_set_right_sidebar (BobguiWorkbench *self,
                                    BobguiWidget    *child)
{
  g_return_if_fail (BOBGUI_IS_WORKBENCH (self));

  self->right_sidebar = child;
  bobgui_paned_set_end_child (self->inner_paned, child);
}

void
bobgui_workbench_set_status (BobguiWorkbench *self,
                             const char      *message)
{
  g_return_if_fail (BOBGUI_IS_WORKBENCH (self));
  bobgui_label_set_text (self->status_label, message);
}

void
bobgui_workbench_add_header_action (BobguiWorkbench                 *self,
                                    const char                      *label,
                                    BobguiWorkbenchActionCallback    callback,
                                    gpointer                         user_data)
{
  BobguiWidget *button;

  g_return_if_fail (BOBGUI_IS_WORKBENCH (self));

  button = bobgui_button_new_with_label (label);
  if (callback)
    g_signal_connect (button, "clicked", G_CALLBACK (callback), user_data);
  bobgui_box_append (self->header_actions, button);
}

typedef struct
{
  BobguiWorkbench *workbench;
  char *command_id;
} BobguiWorkbenchHeaderCommand;

static void
bobgui_workbench_header_command_free (BobguiWorkbenchHeaderCommand *header_command)
{
  g_free (header_command->command_id);
  g_free (header_command);
}

static void
bobgui_workbench_header_command_clicked (BobguiButton *button,
                                         gpointer      user_data)
{
  BobguiWorkbenchHeaderCommand *header_command = user_data;

  (void) button;

  if (header_command->workbench->action_registry)
    bobgui_action_registry_activate (header_command->workbench->action_registry,
                                     header_command->command_id);
}

static void
bobgui_workbench_append_command_button (BobguiWorkbench *self,
                                        BobguiBox       *box,
                                        const char      *label,
                                        const char      *command_id,
                                        const char      *icon_name)
{
  BobguiWorkbenchHeaderCommand *header_command;
  BobguiWidget *button;

  button = bobgui_button_new_with_label (label);
  if (icon_name && *icon_name)
    bobgui_button_set_icon_name (BOBGUI_BUTTON (button), icon_name);
  header_command = g_new0 (BobguiWorkbenchHeaderCommand, 1);
  header_command->workbench = self;
  header_command->command_id = g_strdup (command_id);

  g_signal_connect_data (button,
                         "clicked",
                         G_CALLBACK (bobgui_workbench_header_command_clicked),
                         header_command,
                         (GClosureNotify) bobgui_workbench_header_command_free,
                         0);
  bobgui_box_append (box, button);
}

void
bobgui_workbench_add_header_action_for_command (BobguiWorkbench *self,
                                                const char      *label,
                                                const char      *command_id)
{
  g_return_if_fail (BOBGUI_IS_WORKBENCH (self));
  g_return_if_fail (command_id != NULL);

  bobgui_workbench_append_command_button (self,
                                          self->header_actions,
                                          label,
                                          command_id,
                                          NULL);
}

static void
bobgui_workbench_open_command_palette (BobguiButton *button,
                                       gpointer      user_data)
{
  BobguiWorkbench *self = BOBGUI_WORKBENCH (user_data);

  (void) button;

  if (self->command_palette)
    bobgui_command_palette_present (self->command_palette);
}

void
bobgui_workbench_set_command_palette (BobguiWorkbench      *self,
                                      BobguiCommandPalette *palette)
{
  g_return_if_fail (BOBGUI_IS_WORKBENCH (self));
  g_return_if_fail (BOBGUI_IS_COMMAND_PALETTE (palette));

  g_set_object (&self->command_palette, palette);
  bobgui_command_palette_attach_to_window (palette, BOBGUI_WINDOW (self->window));

  if (self->action_registry)
    bobgui_action_registry_populate_palette (self->action_registry, palette);

  bobgui_workbench_add_header_action (self,
                                      "Commands",
                                      bobgui_workbench_open_command_palette,
                                      self);
}

static void
bobgui_workbench_rebuild_toolbar (BobguiWorkbench *self)
{
  BobguiWidget *child;
  BobguiWidget *next;

  child = bobgui_widget_get_first_child (BOBGUI_WIDGET (self->toolbar_box));
  while (child)
    {
      next = bobgui_widget_get_next_sibling (child);
      bobgui_box_remove (self->toolbar_box, child);
      child = next;
    }

  if (self->action_registry)
    {
      typedef struct
      {
        BobguiWorkbench *self;
        char *last_group;
      } ToolbarBuildData;
      ToolbarBuildData data = { self, NULL };

      void build_button (const char *action_id,
                         const char *title,
                         const char *subtitle,
                         const char *section,
                         const char *category,
                         const char *shortcut,
                         const char *icon_name,
                         gboolean    checkable,
                         gboolean    checked,
                         gpointer    user_data)
      {
        ToolbarBuildData *d = user_data;
        const char *group_title = section ? section : category;
        BobguiWidget *label;
        g_autofree char *button_title = NULL;

        (void) subtitle;
        (void) shortcut;

        if (group_title && g_strcmp0 (d->last_group, group_title) != 0)
          {
            label = bobgui_label_new (group_title);
            bobgui_label_set_xalign (BOBGUI_LABEL (label), 0.0f);
            bobgui_box_append (d->self->toolbar_box, label);
            g_free (d->last_group);
            d->last_group = g_strdup (group_title);
          }

        if (checkable && checked)
          button_title = g_strdup_printf ("✓ %s", title ? title : action_id);
        else
          button_title = g_strdup (title ? title : action_id);

        bobgui_workbench_append_command_button (d->self,
                                                d->self->toolbar_box,
                                                button_title,
                                                action_id,
                                                icon_name);
      }

      bobgui_action_registry_visit (self->action_registry, build_button, &data);
      g_free (data.last_group);
    }
}

void
bobgui_workbench_set_action_registry (BobguiWorkbench      *self,
                                      BobguiActionRegistry *registry)
{
  g_return_if_fail (BOBGUI_IS_WORKBENCH (self));
  g_return_if_fail (BOBGUI_IS_ACTION_REGISTRY (registry));

  g_set_object (&self->action_registry, registry);

  if (self->command_palette)
    bobgui_action_registry_populate_palette (registry, self->command_palette);

  bobgui_workbench_rebuild_toolbar (self);
}

void
bobgui_workbench_enable_menubar (BobguiWorkbench *self,
                                 gboolean         enabled)
{
  g_autoptr(GMenuModel) menu_model = NULL;

  g_return_if_fail (BOBGUI_IS_WORKBENCH (self));

  if (!enabled || self->action_registry == NULL)
    {
      bobgui_application_set_menubar (self->application, NULL);
      bobgui_application_window_set_show_menubar (self->window, FALSE);
      return;
    }

  menu_model = bobgui_action_registry_create_menu_model (self->action_registry);
  bobgui_application_set_menubar (self->application, menu_model);
  bobgui_application_window_set_show_menubar (self->window, TRUE);
}

void
bobgui_workbench_enable_toolbar (BobguiWorkbench *self,
                                 gboolean         enabled)
{
  g_return_if_fail (BOBGUI_IS_WORKBENCH (self));

  bobgui_widget_set_visible (BOBGUI_WIDGET (self->toolbar_box), enabled);
  if (enabled)
    bobgui_workbench_rebuild_toolbar (self);
}

void
bobgui_workbench_add_command_sectioned_visual (BobguiWorkbench                *self,
                                               const char                     *command_id,
                                               const char                     *title,
                                               const char                     *subtitle,
                                               const char                     *section,
                                               const char                     *category,
                                               const char                     *shortcut,
                                               const char                     *icon_name,
                                               BobguiWorkbenchCommandCallback  callback,
                                               gpointer                        user_data)
{
  g_return_if_fail (BOBGUI_IS_WORKBENCH (self));

  if (self->action_registry)
    {
      g_autofree char *action_name = bobgui_workbench_to_action_name (command_id);
      BobguiWorkbenchRegisteredAction *registered = g_new0 (BobguiWorkbenchRegisteredAction, 1);
      GSimpleAction *action;

      registered->callback = callback;
      registered->user_data = user_data;
      registered->command_id = g_strdup (command_id);

      action = g_simple_action_new (action_name, NULL);
      g_signal_connect_data (action,
                             "activate",
                             G_CALLBACK (bobgui_workbench_application_action_activate),
                             registered,
                             (GClosureNotify) bobgui_workbench_registered_action_free,
                             0);
      g_action_map_add_action (G_ACTION_MAP (self->application), G_ACTION (action));
      g_object_unref (action);

      bobgui_action_registry_add_sectioned (self->action_registry,
                                            command_id,
                                            title,
                                            subtitle,
                                            section,
                                            category,
                                            shortcut,
                                            icon_name,
                                            callback,
                                            user_data);

      if (self->command_palette)
        bobgui_action_registry_populate_palette (self->action_registry,
                                                 self->command_palette);
    }
  else if (self->command_palette)
    {
      bobgui_command_palette_add_command_visual (self->command_palette,
                                                 command_id,
                                                 title,
                                                 subtitle,
                                                 section,
                                                 icon_name,
                                                 callback,
                                                 user_data);
    }
}

void
bobgui_workbench_add_command_visual (BobguiWorkbench                *self,
                                     const char                     *command_id,
                                     const char                     *title,
                                     const char                     *subtitle,
                                     const char                     *category,
                                     const char                     *shortcut,
                                     const char                     *icon_name,
                                     BobguiWorkbenchCommandCallback  callback,
                                     gpointer                        user_data)
{
  bobgui_workbench_add_command_sectioned_visual (self,
                                                 command_id,
                                                 title,
                                                 subtitle,
                                                 NULL,
                                                 category,
                                                 shortcut,
                                                 icon_name,
                                                 callback,
                                                 user_data);
}

void
bobgui_workbench_add_command_detailed (BobguiWorkbench                *self,
                                       const char                     *command_id,
                                       const char                     *title,
                                       const char                     *subtitle,
                                       const char                     *category,
                                       const char                     *shortcut,
                                       BobguiWorkbenchCommandCallback  callback,
                                       gpointer                        user_data)
{
  bobgui_workbench_add_command_visual (self,
                                       command_id,
                                       title,
                                       subtitle,
                                       category,
                                       shortcut,
                                       NULL,
                                       callback,
                                       user_data);
}

void
bobgui_workbench_add_toggle_command_sectioned_visual (BobguiWorkbench                *self,
                                                      const char                     *command_id,
                                                      const char                     *title,
                                                      const char                     *subtitle,
                                                      const char                     *section,
                                                      const char                     *category,
                                                      const char                     *shortcut,
                                                      const char                     *icon_name,
                                                      gboolean                        checked,
                                                      BobguiWorkbenchCommandCallback  callback,
                                                      gpointer                        user_data)
{
  g_return_if_fail (BOBGUI_IS_WORKBENCH (self));

  if (self->action_registry)
    {
      g_autofree char *action_name = bobgui_workbench_to_action_name (command_id);
      BobguiWorkbenchRegisteredAction *registered = g_new0 (BobguiWorkbenchRegisteredAction, 1);
      GSimpleAction *action;

      registered->callback = callback;
      registered->user_data = user_data;
      registered->command_id = g_strdup (command_id);

      action = g_simple_action_new (action_name, NULL);
      g_signal_connect_data (action,
                             "activate",
                             G_CALLBACK (bobgui_workbench_application_action_activate),
                             registered,
                             (GClosureNotify) bobgui_workbench_registered_action_free,
                             0);
      g_action_map_add_action (G_ACTION_MAP (self->application), G_ACTION (action));
      g_object_unref (action);

      bobgui_action_registry_add_toggle (self->action_registry,
                                         command_id,
                                         title,
                                         subtitle,
                                         section,
                                         category,
                                         shortcut,
                                         icon_name,
                                         checked,
                                         callback,
                                         user_data);

      if (self->command_palette)
        bobgui_action_registry_populate_palette (self->action_registry,
                                                 self->command_palette);
    }
}

void
bobgui_workbench_add_toggle_command_visual (BobguiWorkbench                *self,
                                            const char                     *command_id,
                                            const char                     *title,
                                            const char                     *subtitle,
                                            const char                     *category,
                                            const char                     *shortcut,
                                            const char                     *icon_name,
                                            gboolean                        checked,
                                            BobguiWorkbenchCommandCallback  callback,
                                            gpointer                        user_data)
{
  bobgui_workbench_add_toggle_command_sectioned_visual (self,
                                                        command_id,
                                                        title,
                                                        subtitle,
                                                        NULL,
                                                        category,
                                                        shortcut,
                                                        icon_name,
                                                        checked,
                                                        callback,
                                                        user_data);
}

void
bobgui_workbench_add_toggle_command (BobguiWorkbench                *self,
                                     const char                     *command_id,
                                     const char                     *title,
                                     const char                     *subtitle,
                                     const char                     *category,
                                     const char                     *shortcut,
                                     gboolean                        checked,
                                     BobguiWorkbenchCommandCallback  callback,
                                     gpointer                        user_data)
{
  bobgui_workbench_add_toggle_command_visual (self,
                                              command_id,
                                              title,
                                              subtitle,
                                              category,
                                              shortcut,
                                              NULL,
                                              checked,
                                              callback,
                                              user_data);
}

void
bobgui_workbench_add_command (BobguiWorkbench                *self,
                              const char                     *command_id,
                              const char                     *title,
                              const char                     *subtitle,
                              BobguiWorkbenchCommandCallback  callback,
                              gpointer                        user_data)
{
  bobgui_workbench_add_command_detailed (self,
                                         command_id,
                                         title,
                                         subtitle,
                                         NULL,
                                         NULL,
                                         callback,
                                         user_data);
}

void
bobgui_workbench_present (BobguiWorkbench *self)
{
  g_return_if_fail (BOBGUI_IS_WORKBENCH (self));
  bobgui_window_present (BOBGUI_WINDOW (self->window));
}
