#include "bobguiactionregistry.h"
#include "bobguicommandpalette.h"

typedef struct
{
  char *id;
  char *title;
  char *subtitle;
  char *section;
  char *category;
  char *shortcut;
  char *icon_name;
  char *tags;
  gboolean checkable;
  gboolean checked;
  BobguiActionRegistryFunc callback;
  gpointer user_data;
} BobguiActionRegistryItem;

struct _BobguiActionRegistry
{
  GObject parent_instance;
  GPtrArray *items;
};

G_DEFINE_TYPE (BobguiActionRegistry, bobgui_action_registry, G_TYPE_OBJECT)

static void
bobgui_action_registry_item_free (BobguiActionRegistryItem *item)
{
  g_free (item->id);
  g_free (item->title);
  g_free (item->subtitle);
  g_free (item->section);
  g_free (item->category);
  g_free (item->shortcut);
  g_free (item->icon_name);
  g_free (item->tags);
  g_free (item);
}

static char *
bobgui_action_registry_to_action_name (const char *action_id)
{
  char *name = g_strdup (action_id);
  for (char *p = name; p && *p; p++)
    if (*p == '.')
      *p = '-';
  return name;
}

static void
bobgui_action_registry_dispose (GObject *object)
{
  BobguiActionRegistry *self = BOBGUI_ACTION_REGISTRY (object);

  g_clear_pointer (&self->items, g_ptr_array_unref);

  G_OBJECT_CLASS (bobgui_action_registry_parent_class)->dispose (object);
}

static void
bobgui_action_registry_class_init (BobguiActionRegistryClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  object_class->dispose = bobgui_action_registry_dispose;
}

static void
bobgui_action_registry_init (BobguiActionRegistry *self)
{
  self->items = g_ptr_array_new_with_free_func ((GDestroyNotify) bobgui_action_registry_item_free);
}

BobguiActionRegistry *
bobgui_action_registry_new (void)
{
  return g_object_new (BOBGUI_TYPE_ACTION_REGISTRY, NULL);
}

void
bobgui_action_registry_add_tagged (BobguiActionRegistry      *self,
                                   const char                *action_id,
                                   const char                *title,
                                   const char                *subtitle,
                                   const char                *section,
                                   const char                *category,
                                   const char                *shortcut,
                                   const char                *icon_name,
                                   const char                *tags,
                                   BobguiActionRegistryFunc   callback,
                                   gpointer                   user_data)
{
  BobguiActionRegistryItem *item;

  g_return_if_fail (BOBGUI_IS_ACTION_REGISTRY (self));
  g_return_if_fail (action_id != NULL);

  item = g_new0 (BobguiActionRegistryItem, 1);
  item->id = g_strdup (action_id);
  item->title = g_strdup (title);
  item->subtitle = g_strdup (subtitle);
  item->section = g_strdup (section);
  item->category = g_strdup (category);
  item->shortcut = g_strdup (shortcut);
  item->icon_name = g_strdup (icon_name);
  item->tags = g_strdup (tags);
  item->callback = callback;
  item->user_data = user_data;
  g_ptr_array_add (self->items, item);
}

void
bobgui_action_registry_add_sectioned (BobguiActionRegistry      *self,
                                      const char                *action_id,
                                      const char                *title,
                                      const char                *subtitle,
                                      const char                *section,
                                      const char                *category,
                                      const char                *shortcut,
                                      const char                *icon_name,
                                      BobguiActionRegistryFunc   callback,
                                      gpointer                   user_data)
{
  bobgui_action_registry_add_tagged (self, action_id, title, subtitle, section, category, shortcut, icon_name, NULL, callback, user_data);
}

void
bobgui_action_registry_add_detailed (BobguiActionRegistry      *self,
                                     const char                *action_id,
                                     const char                *title,
                                     const char                *subtitle,
                                     const char                *category,
                                     const char                *shortcut,
                                     const char                *icon_name,
                                     BobguiActionRegistryFunc   callback,
                                     gpointer                   user_data)
{
  bobgui_action_registry_add_sectioned (self,
                                        action_id,
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
bobgui_action_registry_add_toggle (BobguiActionRegistry      *self,
                                   const char                *action_id,
                                   const char                *title,
                                   const char                *subtitle,
                                   const char                *section,
                                   const char                *category,
                                   const char                *shortcut,
                                   const char                *icon_name,
                                   gboolean                   checked,
                                   BobguiActionRegistryFunc   callback,
                                   gpointer                   user_data)
{
  BobguiActionRegistryItem *item;

  g_return_if_fail (BOBGUI_IS_ACTION_REGISTRY (self));
  g_return_if_fail (action_id != NULL);

  item = g_new0 (BobguiActionRegistryItem, 1);
  item->id = g_strdup (action_id);
  item->title = g_strdup (title);
  item->subtitle = g_strdup (subtitle);
  item->section = g_strdup (section);
  item->category = g_strdup (category);
  item->shortcut = g_strdup (shortcut);
  item->icon_name = g_strdup (icon_name);
  item->checkable = TRUE;
  item->checked = checked;
  item->callback = callback;
  item->user_data = user_data;
  g_ptr_array_add (self->items, item);
}

void
bobgui_action_registry_add (BobguiActionRegistry      *self,
                            const char                *action_id,
                            const char                *title,
                            const char                *subtitle,
                            BobguiActionRegistryFunc   callback,
                            gpointer                   user_data)
{
  bobgui_action_registry_add_detailed (self,
                                       action_id,
                                       title,
                                       subtitle,
                                       NULL,
                                       NULL,
                                       NULL,
                                       callback,
                                       user_data);
}

void
bobgui_action_registry_set_checked (BobguiActionRegistry *self,
                                    const char           *action_id,
                                    gboolean              checked)
{
  guint i;

  g_return_if_fail (BOBGUI_IS_ACTION_REGISTRY (self));
  g_return_if_fail (action_id != NULL);

  for (i = 0; i < self->items->len; i++)
    {
      BobguiActionRegistryItem *item = g_ptr_array_index (self->items, i);
      if (g_strcmp0 (item->id, action_id) == 0)
        {
          item->checked = checked;
          return;
        }
    }
}

gboolean
bobgui_action_registry_get_checked (BobguiActionRegistry *self,
                                    const char           *action_id)
{
  guint i;

  g_return_val_if_fail (BOBGUI_IS_ACTION_REGISTRY (self), FALSE);
  g_return_val_if_fail (action_id != NULL, FALSE);

  for (i = 0; i < self->items->len; i++)
    {
      BobguiActionRegistryItem *item = g_ptr_array_index (self->items, i);
      if (g_strcmp0 (item->id, action_id) == 0)
        return item->checked;
    }

  return FALSE;
}

void
bobgui_action_registry_activate (BobguiActionRegistry *self,
                                 const char           *action_id)
{
  guint i;

  g_return_if_fail (BOBGUI_IS_ACTION_REGISTRY (self));
  g_return_if_fail (action_id != NULL);

  for (i = 0; i < self->items->len; i++)
    {
      BobguiActionRegistryItem *item = g_ptr_array_index (self->items, i);

      if (g_strcmp0 (item->id, action_id) == 0)
        {
          if (item->callback)
            item->callback (item->id, item->user_data);
          return;
        }
    }
}

GMenuModel *
bobgui_action_registry_create_menu_model (BobguiActionRegistry *self)
{
  GMenu *menu;
  GHashTable *sections;
  guint i;

  g_return_val_if_fail (BOBGUI_IS_ACTION_REGISTRY (self), NULL);

  menu = g_menu_new ();
  sections = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_object_unref);

  for (i = 0; i < self->items->len; i++)
    {
      BobguiActionRegistryItem *item = g_ptr_array_index (self->items, i);
      const char *section_title = item->section ? item->section : (item->category ? item->category : "General");
      GMenu *section = g_hash_table_lookup (sections, section_title);
      g_autofree char *action_name = bobgui_action_registry_to_action_name (item->id);
      g_autofree char *detailed = g_strdup_printf ("app.%s", action_name);

      if (section == NULL)
        {
          section = g_menu_new ();
          g_hash_table_insert (sections, g_strdup (section_title), section);
          g_menu_append_section (menu, section_title, G_MENU_MODEL (section));
        }

      g_autofree char *label = NULL;

      if (item->checkable && item->checked)
        label = g_strdup_printf ("✓ %s", item->title ? item->title : item->id);
      else
        label = g_strdup (item->title ? item->title : item->id);

      g_menu_append (section,
                     label,
                     detailed);
    }

  g_hash_table_unref (sections);
  return G_MENU_MODEL (menu);
}

void
bobgui_action_registry_visit (BobguiActionRegistry          *self,
                              BobguiActionRegistryVisitFunc  func,
                              gpointer                       user_data)
{
  guint i;

  g_return_if_fail (BOBGUI_IS_ACTION_REGISTRY (self));
  g_return_if_fail (func != NULL);

  for (i = 0; i < self->items->len; i++)
    {
      BobguiActionRegistryItem *item = g_ptr_array_index (self->items, i);
      func (item->id,
            item->title,
            item->subtitle,
            item->section,
            item->category,
            item->shortcut,
            item->icon_name,
            item->tags,
            item->checkable,
            item->checked,
            user_data);
    }
}

void
bobgui_action_registry_populate_palette (BobguiActionRegistry *self,
                                         BobguiCommandPalette *palette)
{
  guint i;

  g_return_if_fail (BOBGUI_IS_ACTION_REGISTRY (self));
  g_return_if_fail (BOBGUI_IS_COMMAND_PALETTE (palette));

  bobgui_command_palette_clear (palette);

  for (i = 0; i < self->items->len; i++)
    {
      BobguiActionRegistryItem *item = g_ptr_array_index (self->items, i);
      g_autofree char *subtitle = NULL;

      if (item->icon_name && item->checkable && item->checked && item->category && item->shortcut && item->subtitle)
        subtitle = g_strdup_printf ("★ %s [%s] %s — %s", item->icon_name, item->category, item->shortcut, item->subtitle);
      else if (item->icon_name && item->category && item->shortcut && item->subtitle)
        subtitle = g_strdup_printf ("★ %s [%s] %s — %s", item->icon_name, item->category, item->shortcut, item->subtitle);
      else if (item->checkable && item->checked && item->category && item->shortcut && item->subtitle)
        subtitle = g_strdup_printf ("✓ [%s] %s — %s", item->category, item->shortcut, item->subtitle);
      else if (item->checkable && item->checked && item->category && item->shortcut)
        subtitle = g_strdup_printf ("✓ [%s] %s", item->category, item->shortcut);
      else if (item->checkable && item->checked && item->subtitle)
        subtitle = g_strdup_printf ("✓ %s", item->subtitle);
      else if (item->checkable && item->checked)
        subtitle = g_strdup ("✓ Enabled");
      else if (item->category && item->shortcut && item->subtitle)
        subtitle = g_strdup_printf ("[%s] %s — %s", item->category, item->shortcut, item->subtitle);
      else if (item->category && item->shortcut)
        subtitle = g_strdup_printf ("[%s] %s", item->category, item->shortcut);
      else if (item->category && item->subtitle)
        subtitle = g_strdup_printf ("[%s] %s", item->category, item->subtitle);
      else if (item->shortcut && item->subtitle)
        subtitle = g_strdup_printf ("%s — %s", item->shortcut, item->subtitle);
      else if (item->category)
        subtitle = g_strdup_printf ("[%s]", item->category);
      else if (item->shortcut)
        subtitle = g_strdup (item->shortcut);
      else if (item->subtitle)
        subtitle = g_strdup (item->subtitle);

      bobgui_command_palette_add_command_visual (palette,
                                                 item->id,
                                                 item->title,
                                                 subtitle,
                                                 item->section,
                                                 item->icon_name,
                                                 item->callback,
                                                 item->user_data);
    }
}
