#include "bobguicommandpalette.h"

typedef struct
{
  char *id;
  char *title;
  char *subtitle;
  char *section;
  char *icon_name;
  BobguiCommandPaletteFunc callback;
  gpointer user_data;
} BobguiCommandPaletteItem;

typedef struct
{
  BobguiCommandPaletteItem *item;
  int score;
} BobguiCommandPaletteMatch;

struct _BobguiCommandPalette
{
  GObject parent_instance;
  BobguiWindow *window;
  BobguiSearchEntry *search_entry;
  BobguiListBox *list_box;
  GPtrArray *items;
  GHashTable *recent_counts;
  GHashTable *pinned_ids;
};

G_DEFINE_TYPE (BobguiCommandPalette, bobgui_command_palette, G_TYPE_OBJECT)

static void
bobgui_command_palette_item_free (BobguiCommandPaletteItem *item)
{
  g_free (item->id);
  g_free (item->title);
  g_free (item->subtitle);
  g_free (item->section);
  g_free (item->icon_name);
  g_free (item);
}

static BobguiCommandPaletteItem *
bobgui_command_palette_item_from_row (BobguiListBoxRow *row)
{
  return g_object_get_data (G_OBJECT (row), "bobgui-command-palette-item");
}

static void
bobgui_command_palette_on_row_activated (BobguiListBox *box,
                                         BobguiListBoxRow *row,
                                         gpointer user_data)
{
  BobguiCommandPalette *self = BOBGUI_COMMAND_PALETTE (user_data);
  BobguiCommandPaletteItem *item = bobgui_command_palette_item_from_row (row);

  (void) box;

  if (item)
    {
      bobgui_command_palette_mark_used (self, item->id);

      if (item->callback)
        item->callback (item->id, item->user_data);
    }

  bobgui_window_close (self->window);
}

static int
bobgui_command_palette_get_recent_internal (BobguiCommandPalette *self,
                                            const char           *command_id)
{
  return GPOINTER_TO_INT (g_hash_table_lookup (self->recent_counts, command_id));
}

static BobguiWidget *
bobgui_command_palette_build_section_row (const char *title)
{
  BobguiWidget *row;
  BobguiWidget *label;

  row = bobgui_list_box_row_new ();
  label = bobgui_label_new (title);
  bobgui_label_set_xalign (BOBGUI_LABEL (label), 0.0f);
  bobgui_list_box_row_set_child (BOBGUI_LIST_BOX_ROW (row), label);
  bobgui_list_box_row_set_activatable (BOBGUI_LIST_BOX_ROW (row), FALSE);
  bobgui_list_box_row_set_selectable (BOBGUI_LIST_BOX_ROW (row), FALSE);

  return row;
}

static BobguiWidget *
bobgui_command_palette_build_row (BobguiCommandPaletteItem *item)
{
  BobguiWidget *row;
  BobguiWidget *box;
  BobguiWidget *content_box;
  BobguiWidget *icon;
  BobguiWidget *title;
  BobguiWidget *subtitle;

  row = bobgui_list_box_row_new ();
  box = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 8);
  content_box = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 2);
  title = bobgui_label_new (item->title ? item->title : item->id);
  subtitle = bobgui_label_new (item->subtitle ? item->subtitle : "");

  bobgui_label_set_xalign (BOBGUI_LABEL (title), 0.0f);
  bobgui_label_set_xalign (BOBGUI_LABEL (subtitle), 0.0f);

  if (item->icon_name && *item->icon_name)
    {
      icon = bobgui_image_new_from_icon_name (item->icon_name);
      bobgui_box_append (BOBGUI_BOX (box), icon);
    }

  bobgui_box_append (BOBGUI_BOX (content_box), title);
  bobgui_box_append (BOBGUI_BOX (content_box), subtitle);
  bobgui_box_append (BOBGUI_BOX (box), content_box);
  bobgui_list_box_row_set_child (BOBGUI_LIST_BOX_ROW (row), box);
  g_object_set_data (G_OBJECT (row), "bobgui-command-palette-item", item);

  return row;
}

static int
bobgui_command_palette_score_text (const char *haystack,
                                   const char *needle)
{
  const char *match;

  if (haystack == NULL || needle == NULL || *needle == '\0')
    return 0;

  match = strstr (haystack, needle);
  if (match == NULL)
    return -1;

  if (match == haystack)
    return 100;

  return 50 - (int) (match - haystack);
}

static int
bobgui_command_palette_score_item (BobguiCommandPalette     *self,
                                   BobguiCommandPaletteItem *item,
                                   const char               *query)
{
  char *id_lower;
  char *title_lower;
  char *subtitle_lower;
  char *query_lower;
  int score = -1;
  int current;

  if (query == NULL || *query == '\0')
    {
      score = 0;
      if (g_hash_table_contains (self->pinned_ids, item->id))
        score += 1000;
      score += bobgui_command_palette_get_recent_internal (self, item->id) * 10;
      return score;
    }

  query_lower = g_utf8_strdown (query, -1);
  id_lower = g_utf8_strdown (item->id ? item->id : "", -1);
  title_lower = g_utf8_strdown (item->title ? item->title : "", -1);
  subtitle_lower = g_utf8_strdown (item->subtitle ? item->subtitle : "", -1);

  current = bobgui_command_palette_score_text (id_lower, query_lower);
  if (current > score)
    score = current;

  current = bobgui_command_palette_score_text (title_lower, query_lower);
  if (current > score)
    score = current + 25;

  current = bobgui_command_palette_score_text (subtitle_lower, query_lower);
  if (current > score)
    score = current;

  g_free (query_lower);
  g_free (id_lower);
  g_free (title_lower);
  g_free (subtitle_lower);

  if (g_hash_table_contains (self->pinned_ids, item->id))
    score += 1000;

  score += bobgui_command_palette_get_recent_internal (self, item->id) * 10;

  return score;
}

static gint
bobgui_command_palette_compare_match (gconstpointer a,
                                      gconstpointer b)
{
  const BobguiCommandPaletteMatch *ma = a;
  const BobguiCommandPaletteMatch *mb = b;

  return mb->score - ma->score;
}

static void
bobgui_command_palette_rebuild (BobguiCommandPalette *self)
{
  const char *query;
  GArray *matches;
  char *last_category = NULL;
  guint i;

  bobgui_list_box_remove_all (self->list_box);
  query = bobgui_editable_get_text (BOBGUI_EDITABLE (self->search_entry));
  matches = g_array_new (FALSE, FALSE, sizeof (BobguiCommandPaletteMatch));

  for (i = 0; i < self->items->len; i++)
    {
      BobguiCommandPaletteItem *item = g_ptr_array_index (self->items, i);
      BobguiCommandPaletteMatch match = { 0, };

      match.item = item;
      match.score = bobgui_command_palette_score_item (self, item, query);

      if (query == NULL || *query == '\0' || match.score >= 0)
        g_array_append_val (matches, match);
    }

  g_array_sort (matches, bobgui_command_palette_compare_match);

  for (i = 0; i < matches->len; i++)
    {
      BobguiCommandPaletteMatch *match = &g_array_index (matches, BobguiCommandPaletteMatch, i);
      const char *section_name = NULL;

      if (g_hash_table_contains (self->pinned_ids, match->item->id))
        section_name = "Pinned";
      else if (bobgui_command_palette_get_recent_internal (self, match->item->id) > 0)
        section_name = "Recent";
      else if (match->item->section)
        section_name = match->item->section;

      if (section_name && g_strcmp0 (last_category, section_name) != 0)
        {
          bobgui_list_box_append (self->list_box,
                                  bobgui_command_palette_build_section_row (section_name));
          g_free (last_category);
          last_category = g_strdup (section_name);
        }

      bobgui_list_box_append (self->list_box, bobgui_command_palette_build_row (match->item));
    }

  g_clear_pointer (&last_category, g_free);

  if (matches->len > 0)
    {
      BobguiListBoxRow *row = NULL;
      int index = 0;

      while ((row = bobgui_list_box_get_row_at_index (self->list_box, index++)) != NULL)
        {
          if (bobgui_list_box_row_get_activatable (row))
            {
              bobgui_list_box_select_row (self->list_box, row);
              break;
            }
        }
    }

  g_array_unref (matches);
}

static void
bobgui_command_palette_on_search_changed (BobguiSearchEntry     *entry,
                                          BobguiCommandPalette  *self)
{
  (void) entry;
  bobgui_command_palette_rebuild (self);
}

static BobguiListBoxRow *
bobgui_command_palette_find_selectable_row (BobguiCommandPalette *self,
                                            int                   start_index,
                                            int                   step)
{
  BobguiListBoxRow *row;
  int index = start_index;

  while (index >= 0 && (row = bobgui_list_box_get_row_at_index (self->list_box, index)) != NULL)
    {
      if (bobgui_list_box_row_get_activatable (row))
        return row;
      index += step;
    }

  return NULL;
}

static gboolean
bobgui_command_palette_on_search_key_pressed (BobguiEventControllerKey *controller,
                                              guint                     keyval,
                                              guint                     keycode,
                                              GdkModifierType           state,
                                              gpointer                  user_data)
{
  BobguiCommandPalette *self = BOBGUI_COMMAND_PALETTE (user_data);
  BobguiListBoxRow *row;
  int index;

  (void) controller;
  (void) keycode;
  (void) state;

  row = bobgui_list_box_get_selected_row (self->list_box);
  index = row ? bobgui_list_box_row_get_index (row) : -1;

  if (keyval == GDK_KEY_Down)
    {
      BobguiListBoxRow *next = bobgui_command_palette_find_selectable_row (self, index + 1, 1);
      if (next)
        bobgui_list_box_select_row (self->list_box, next);
      return TRUE;
    }
  else if (keyval == GDK_KEY_Up)
    {
      BobguiListBoxRow *prev = bobgui_command_palette_find_selectable_row (self, index > 0 ? index - 1 : 0, -1);
      if (prev)
        bobgui_list_box_select_row (self->list_box, prev);
      return TRUE;
    }
  else if (keyval == GDK_KEY_Return || keyval == GDK_KEY_KP_Enter)
    {
      if (row && bobgui_list_box_row_get_activatable (row))
        g_signal_emit_by_name (self->list_box, "row-activated", row);
      return TRUE;
    }

  return FALSE;
}

static void
bobgui_command_palette_dispose (GObject *object)
{
  BobguiCommandPalette *self = BOBGUI_COMMAND_PALETTE (object);

  g_clear_object (&self->window);
  if (self->items)
    {
      g_ptr_array_unref (self->items);
      self->items = NULL;
    }
  g_clear_pointer (&self->recent_counts, g_hash_table_unref);
  g_clear_pointer (&self->pinned_ids, g_hash_table_unref);

  G_OBJECT_CLASS (bobgui_command_palette_parent_class)->dispose (object);
}

static void
bobgui_command_palette_class_init (BobguiCommandPaletteClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  object_class->dispose = bobgui_command_palette_dispose;
}

static void
bobgui_command_palette_init (BobguiCommandPalette *self)
{
  self->items = g_ptr_array_new_with_free_func ((GDestroyNotify) bobgui_command_palette_item_free);
  self->recent_counts = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
  self->pinned_ids = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
}

BobguiCommandPalette *
bobgui_command_palette_new (BobguiApplication *application)
{
  BobguiCommandPalette *self = g_object_new (BOBGUI_TYPE_COMMAND_PALETTE, NULL);
  BobguiEventController *key_controller;
  BobguiWidget *root;
  BobguiWidget *scroller;

  self->window = BOBGUI_WINDOW (g_object_new (BOBGUI_TYPE_APPLICATION_WINDOW,
                                              "application", application,
                                              NULL));
  bobgui_window_set_title (self->window, "Command Palette");
  bobgui_window_set_default_size (self->window, 560, 420);

  root = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 8);
  self->search_entry = BOBGUI_SEARCH_ENTRY (bobgui_search_entry_new ());
  self->list_box = BOBGUI_LIST_BOX (bobgui_list_box_new ());
  scroller = bobgui_scrolled_window_new ();

  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (scroller), BOBGUI_WIDGET (self->list_box));
  bobgui_box_append (BOBGUI_BOX (root), BOBGUI_WIDGET (self->search_entry));
  bobgui_box_append (BOBGUI_BOX (root), scroller);
  bobgui_window_set_child (self->window, root);

  g_signal_connect (self->list_box, "row-activated",
                    G_CALLBACK (bobgui_command_palette_on_row_activated), self);
  g_signal_connect (self->search_entry, "search-changed",
                    G_CALLBACK (bobgui_command_palette_on_search_changed), self);

  key_controller = bobgui_event_controller_key_new ();
  g_signal_connect (key_controller, "key-pressed",
                    G_CALLBACK (bobgui_command_palette_on_search_key_pressed), self);
  bobgui_widget_add_controller (BOBGUI_WIDGET (self->search_entry), key_controller);

  return self;
}

void
bobgui_command_palette_set_pinned (BobguiCommandPalette *self,
                                   const char           *command_id,
                                   gboolean              pinned)
{
  g_return_if_fail (BOBGUI_IS_COMMAND_PALETTE (self));
  g_return_if_fail (command_id != NULL);

  if (pinned)
    g_hash_table_replace (self->pinned_ids, g_strdup (command_id), GINT_TO_POINTER (1));
  else
    g_hash_table_remove (self->pinned_ids, command_id);

  bobgui_command_palette_rebuild (self);
}

gboolean
bobgui_command_palette_get_pinned (BobguiCommandPalette *self,
                                   const char           *command_id)
{
  g_return_val_if_fail (BOBGUI_IS_COMMAND_PALETTE (self), FALSE);
  g_return_val_if_fail (command_id != NULL, FALSE);

  return g_hash_table_contains (self->pinned_ids, command_id);
}

int
bobgui_command_palette_get_recent_count (BobguiCommandPalette *self,
                                         const char           *command_id)
{
  g_return_val_if_fail (BOBGUI_IS_COMMAND_PALETTE (self), 0);
  g_return_val_if_fail (command_id != NULL, 0);

  return bobgui_command_palette_get_recent_internal (self, command_id);
}

void
bobgui_command_palette_mark_used (BobguiCommandPalette *self,
                                  const char           *command_id)
{
  int recent;

  g_return_if_fail (BOBGUI_IS_COMMAND_PALETTE (self));
  g_return_if_fail (command_id != NULL);

  recent = bobgui_command_palette_get_recent_internal (self, command_id);
  g_hash_table_replace (self->recent_counts,
                        g_strdup (command_id),
                        GINT_TO_POINTER (recent + 1));
  bobgui_command_palette_rebuild (self);
}

void
bobgui_command_palette_clear_history (BobguiCommandPalette *self)
{
  g_return_if_fail (BOBGUI_IS_COMMAND_PALETTE (self));

  g_hash_table_remove_all (self->recent_counts);
  bobgui_command_palette_rebuild (self);
}

void
bobgui_command_palette_clear (BobguiCommandPalette *self)
{
  g_return_if_fail (BOBGUI_IS_COMMAND_PALETTE (self));

  g_ptr_array_set_size (self->items, 0);
  bobgui_command_palette_rebuild (self);
}

void
bobgui_command_palette_add_command_visual (BobguiCommandPalette     *self,
                                           const char               *command_id,
                                           const char               *title,
                                           const char               *subtitle,
                                           const char               *section,
                                           const char               *icon_name,
                                           BobguiCommandPaletteFunc  callback,
                                           gpointer                  user_data)
{
  BobguiCommandPaletteItem *item;

  g_return_if_fail (BOBGUI_IS_COMMAND_PALETTE (self));

  item = g_new0 (BobguiCommandPaletteItem, 1);
  item->id = g_strdup (command_id);
  item->title = g_strdup (title);
  item->subtitle = g_strdup (subtitle);
  item->section = g_strdup (section);
  item->icon_name = g_strdup (icon_name);
  item->callback = callback;
  item->user_data = user_data;

  g_ptr_array_add (self->items, item);
  bobgui_command_palette_rebuild (self);
}

void
bobgui_command_palette_add_command (BobguiCommandPalette     *self,
                                    const char               *command_id,
                                    const char               *title,
                                    const char               *subtitle,
                                    BobguiCommandPaletteFunc  callback,
                                    gpointer                  user_data)
{
  bobgui_command_palette_add_command_visual (self,
                                             command_id,
                                             title,
                                             subtitle,
                                             NULL,
                                             NULL,
                                             callback,
                                             user_data);
}

void
bobgui_command_palette_attach_to_window (BobguiCommandPalette *self,
                                         BobguiWindow         *window)
{
  g_return_if_fail (BOBGUI_IS_COMMAND_PALETTE (self));
  g_return_if_fail (BOBGUI_IS_WINDOW (window));

  bobgui_window_set_transient_for (self->window, window);
  bobgui_window_set_modal (self->window, TRUE);
}

void
bobgui_command_palette_present (BobguiCommandPalette *self)
{
  g_return_if_fail (BOBGUI_IS_COMMAND_PALETTE (self));
  bobgui_window_present (self->window);
}
