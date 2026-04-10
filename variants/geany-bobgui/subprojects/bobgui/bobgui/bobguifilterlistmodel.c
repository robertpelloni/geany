/*
 * Copyright © 2018 Benjamin Otte
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Benjamin Otte <otte@gnome.org>
 */

#include "config.h"

#include "bobguifilterlistmodel.h"

#include "bobguibitset.h"
#include "bobguifilterprivate.h"
#include "bobguiprivate.h"
#include "bobguisectionmodelprivate.h"

/**
 * BobguiFilterListModel:
 *
 * A list model that filters the elements of another model.
 *
 * It hides some elements from the underlying model according to
 * criteria given by a `BobguiFilter`.
 *
 * The model can be set up to do incremental filtering, so that
 * filtering long lists doesn't block the UI. See
 * [method@Bobgui.FilterListModel.set_incremental] for details.
 *
 * `BobguiFilterListModel` passes through sections from the underlying model.
 */

enum {
  PROP_0,
  PROP_FILTER,
  PROP_INCREMENTAL,
  PROP_ITEM_TYPE,
  PROP_MODEL,
  PROP_N_ITEMS,
  PROP_PENDING,
  PROP_WATCH_ITEMS,
  NUM_PROPERTIES
};

typedef struct _WatchData
{
  BobguiFilter *filter;
  gpointer watch;
} WatchData;

struct _BobguiFilterListModel
{
  GObject parent_instance;

  GListModel *model;
  BobguiFilter *filter;
  BobguiFilterMatch strictness;
  gboolean incremental;
  gboolean watch_items;

  GSequence *watches; /* NULL if watch_items == FALSE */
  BobguiBitset *watched_items; /* NULL if watch_items == FALSE */

  BobguiBitset *matches; /* NULL if strictness != BOBGUI_FILTER_MATCH_SOME */
  BobguiBitset *pending; /* not yet filtered items or NULL if all filtered */
  guint pending_cb; /* idle callback handle */
};

struct _BobguiFilterListModelClass
{
  GObjectClass parent_class;
};

static GParamSpec *properties[NUM_PROPERTIES] = { NULL, };

static void
watch_data_free (gpointer data)
{
  WatchData *self = (WatchData *) data;

  if (self->watch)
    bobgui_filter_unwatch (self->filter, g_steal_pointer (&self->watch));

  g_free (self);
}

static WatchData *
watch_data_new (BobguiFilter *filter,
                gpointer   watch)
{
  WatchData *self = g_new0 (WatchData, 1);
  self->filter = filter;
  self->watch = watch;
  return g_steal_pointer (&self);
}

static GType
bobgui_filter_list_model_get_item_type (GListModel *list)
{
  return G_TYPE_OBJECT;
}

static guint
bobgui_filter_list_model_get_n_items (GListModel *list)
{
  BobguiFilterListModel *self = BOBGUI_FILTER_LIST_MODEL (list);

  switch (self->strictness)
    {
    case BOBGUI_FILTER_MATCH_NONE:
      return 0;

    case BOBGUI_FILTER_MATCH_ALL:
      return g_list_model_get_n_items (self->model);

    case BOBGUI_FILTER_MATCH_SOME:
      return bobgui_bitset_get_size (self->matches);

    default:
      g_assert_not_reached ();
      return 0;
    }
}

static gpointer
bobgui_filter_list_model_get_item (GListModel *list,
                                guint       position)
{
  BobguiFilterListModel *self = BOBGUI_FILTER_LIST_MODEL (list);
  guint unfiltered;

  switch (self->strictness)
    {
    case BOBGUI_FILTER_MATCH_NONE:
      return NULL;

    case BOBGUI_FILTER_MATCH_ALL:
      unfiltered = position;
      break;

    case BOBGUI_FILTER_MATCH_SOME:
      unfiltered = bobgui_bitset_get_nth (self->matches, position);
      if (unfiltered == 0 && position >= bobgui_bitset_get_size (self->matches))
        return NULL;
      break;

    default:
      g_assert_not_reached ();
    }

  return g_list_model_get_item (self->model, unfiltered);
}

static void
bobgui_filter_list_model_model_init (GListModelInterface *iface)
{
  iface->get_item_type = bobgui_filter_list_model_get_item_type;
  iface->get_n_items = bobgui_filter_list_model_get_n_items;
  iface->get_item = bobgui_filter_list_model_get_item;
}

static void
bobgui_filter_list_model_get_section (BobguiSectionModel *model,
                                   guint            position,
                                   guint           *out_start,
                                   guint           *out_end)
{
  BobguiFilterListModel *self = BOBGUI_FILTER_LIST_MODEL (model);
  guint n_items;
  guint pos, start, end;

  switch (self->strictness)
    {
    case BOBGUI_FILTER_MATCH_NONE:
      *out_start = 0;
      *out_end = G_MAXUINT;
      return;

    case BOBGUI_FILTER_MATCH_ALL:
      bobgui_list_model_get_section (self->model, position, out_start, out_end);
      return;

    case BOBGUI_FILTER_MATCH_SOME:
      n_items = bobgui_bitset_get_size (self->matches);
      if (position >= n_items)
        {
          *out_start = n_items;
          *out_end = G_MAXUINT;
          return;
        }
      if (!BOBGUI_IS_SECTION_MODEL (self->model))
        {
          *out_start = 0;
          *out_end = n_items;
          return;
        }
      break;

    default:
      g_assert_not_reached ();
    }

  /* if we get here, we have a section model, and are MATCH_SOME */

  pos = bobgui_bitset_get_nth (self->matches, position);
  bobgui_section_model_get_section (BOBGUI_SECTION_MODEL (self->model), pos, &start, &end);
  if (start == 0)
    *out_start = 0;
  else
    *out_start = bobgui_bitset_get_size_in_range (self->matches, 0, start - 1);
  *out_end = *out_start + bobgui_bitset_get_size_in_range (self->matches, start, end - 1);
}

static void
bobgui_filter_list_model_sections_changed_cb (BobguiSectionModel *model,
                                           unsigned int     position,
                                           unsigned int     n_items,
                                           gpointer         user_data)
{
  BobguiFilterListModel *self = BOBGUI_FILTER_LIST_MODEL (user_data);
  unsigned int start, end;

  switch (self->strictness)
    {
    case BOBGUI_FILTER_MATCH_NONE:
      return;

    case BOBGUI_FILTER_MATCH_ALL:
      bobgui_section_model_sections_changed (BOBGUI_SECTION_MODEL (self), position, n_items);
      break;

    case BOBGUI_FILTER_MATCH_SOME:
      if (position > 0)
        start = bobgui_bitset_get_size_in_range (self->matches, 0, position - 1);
      else
        start = 0;
      end = bobgui_bitset_get_size_in_range (self->matches, 0, position + n_items - 1);
      if (end - start > 0)
        bobgui_section_model_sections_changed (BOBGUI_SECTION_MODEL (self), start, end - start);
      break;

    default:
      g_assert_not_reached ();
    }
}

static void
bobgui_filter_list_model_section_model_init (BobguiSectionModelInterface *iface)
{
  iface->get_section = bobgui_filter_list_model_get_section;
}

G_DEFINE_TYPE_WITH_CODE (BobguiFilterListModel, bobgui_filter_list_model, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (G_TYPE_LIST_MODEL, bobgui_filter_list_model_model_init)
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_SECTION_MODEL, bobgui_filter_list_model_section_model_init))

static gboolean
bobgui_filter_list_model_run_filter_on_item (BobguiFilterListModel *self,
                                          gpointer            item)
{
  /* all other cases should have been optimized away */
  g_assert (self->strictness == BOBGUI_FILTER_MATCH_SOME);

  return bobgui_filter_match (self->filter, item);
}

static void
bobgui_filter_list_model_emit_items_changed_for_changes (BobguiFilterListModel *self,
                                                      BobguiBitset          *old)
{
  BobguiBitset *changes;

  changes = bobgui_bitset_copy (self->matches);
  bobgui_bitset_difference (changes, old);
  if (!bobgui_bitset_is_empty (changes))
    {
      guint min, max, removed, added;

      min = bobgui_bitset_get_minimum (changes);
      max = bobgui_bitset_get_maximum (changes);
      removed = bobgui_bitset_get_size_in_range (old, min, max);
      added = bobgui_bitset_get_size_in_range (self->matches, min, max);
      g_list_model_items_changed (G_LIST_MODEL (self),
                                  min > 0 ? bobgui_bitset_get_size_in_range (self->matches, 0, min - 1) : 0,
                                  removed,
                                  added);
      if (removed != added)
        g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_N_ITEMS]);
    }
  bobgui_bitset_unref (changes);
  bobgui_bitset_unref (old);
}

static void bobgui_filter_list_model_start_filtering (BobguiFilterListModel *self,
                                                   BobguiBitset          *items);

static void
item_changed_cb (gpointer item,
                 gpointer user_data)
{
  BobguiFilterListModel *self = (BobguiFilterListModel *) user_data;
  BobguiBitset *item_to_refilter = NULL;
  unsigned int position = BOBGUI_INVALID_LIST_POSITION;
  unsigned int n_items;
  gboolean was_filtered;
  gboolean is_filtered;

  g_assert (BOBGUI_IS_FILTER_LIST_MODEL (self));
  g_assert (G_IS_LIST_MODEL (self->model));
  g_assert (BOBGUI_IS_FILTER (self->filter));

  item_to_refilter = bobgui_bitset_new_empty ();
  n_items = g_list_model_get_n_items (self->model);

  for (position = 0; position < n_items; position++)
    {
      gpointer aux = g_list_model_get_item (self->model, position);

      if (aux == item)
        {
          bobgui_bitset_add (item_to_refilter, position);
          g_clear_object (&aux);
          break;
        }

      g_clear_object (&aux);
    }

  g_assert (!bobgui_bitset_is_empty (item_to_refilter));
  g_assert (position != BOBGUI_INVALID_LIST_POSITION);

  was_filtered = bobgui_bitset_contains (self->matches, position);

  bobgui_filter_list_model_start_filtering (self, g_steal_pointer (&item_to_refilter));

  is_filtered = bobgui_bitset_contains (self->matches, position);
  if (was_filtered != is_filtered)
    {
      g_list_model_items_changed (G_LIST_MODEL (self),
                                  position > 0 ? bobgui_bitset_get_size_in_range (self->matches, 0, position - 1) : 0,
                                  is_filtered ? 0 : 1,
                                  is_filtered ? 1 : 0);

      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_N_ITEMS]);
    }
}

static void
bobgui_filter_list_model_run_filter (BobguiFilterListModel *self,
                                  guint               n_steps)
{
  BobguiBitsetIter iter;
  guint i, pos;
  gboolean more;

  g_return_if_fail (BOBGUI_IS_FILTER_LIST_MODEL (self));

  if (self->pending == NULL)
    return;

  for (i = 0, more = bobgui_bitset_iter_init_first (&iter, self->pending, &pos);
       i < n_steps && more;
       i++, more = bobgui_bitset_iter_next (&iter, &pos))
    {
      gpointer item = g_list_model_get_item (self->model, pos);

      if (bobgui_filter_list_model_run_filter_on_item (self, item))
        bobgui_bitset_add (self->matches, pos);
      else
        bobgui_bitset_remove (self->matches, pos);

      if (self->watch_items && !bobgui_bitset_contains (self->watched_items, pos))
        {
          gpointer watch;

          watch = bobgui_filter_watch (self->filter, item, item_changed_cb, self, NULL);
          g_sequence_insert_before (g_sequence_get_iter_at_pos (self->watches, pos),
                                    watch_data_new (self->filter, watch));

          bobgui_bitset_add (self->watched_items, pos);
        }

      g_clear_object (&item);
    }

  if (more)
    bobgui_bitset_remove_range_closed (self->pending, 0, pos - 1);
  else
    g_clear_pointer (&self->pending, bobgui_bitset_unref);
}

static void
bobgui_filter_list_model_stop_filtering (BobguiFilterListModel *self)
{
  gboolean notify_pending = self->pending != NULL;

  g_clear_pointer (&self->pending, bobgui_bitset_unref);
  g_clear_handle_id (&self->pending_cb, g_source_remove);

  if (notify_pending)
    g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_PENDING]);
}

static void
setup_all_watches (BobguiFilterListModel *self)
{
  unsigned int n_items;

  if (self->filter == NULL || self->model == NULL)
    return;

  n_items = g_list_model_get_n_items (self->model);

  for (size_t i = 0; i < n_items; i++)
    {
      gpointer item = g_list_model_get_item (self->model, i);
      gpointer watch = bobgui_filter_watch (self->filter, item, item_changed_cb, self, NULL);

      g_sequence_append (self->watches, watch_data_new (self->filter, watch));

      g_clear_object (&item);
    }

  bobgui_bitset_add_range (self->watched_items, 0, n_items);
}

static gboolean
bobgui_filter_list_model_run_filter_cb (gpointer data)
{
  BobguiFilterListModel *self = data;
  BobguiBitset *old;

  old = bobgui_bitset_copy (self->matches);
  bobgui_filter_list_model_run_filter (self, 512);

  if (self->pending == NULL)
    bobgui_filter_list_model_stop_filtering (self);

  bobgui_filter_list_model_emit_items_changed_for_changes (self, old);
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_PENDING]);

  return G_SOURCE_CONTINUE;
}

/* NB: bitset is (transfer full) */
static void
bobgui_filter_list_model_start_filtering (BobguiFilterListModel *self,
                                       BobguiBitset          *items)
{
  if (self->pending)
    {
      bobgui_bitset_union (self->pending, items);
      bobgui_bitset_unref (items);
      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_PENDING]);
      return;
    }

  if (bobgui_bitset_is_empty (items))
    {
      bobgui_bitset_unref (items);
      return;
    }

  self->pending = items;

  if (!self->incremental)
    {
      bobgui_filter_list_model_run_filter (self, G_MAXUINT);
      g_assert (self->pending == NULL);
      return;
    }

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_PENDING]);
  g_assert (self->pending_cb == 0);
  self->pending_cb = g_idle_add (bobgui_filter_list_model_run_filter_cb, self);
  gdk_source_set_static_name_by_id (self->pending_cb, "[bobgui] bobgui_filter_list_model_run_filter_cb");
}

static void
bobgui_filter_list_model_items_changed_cb (GListModel         *model,
                                        guint               position,
                                        guint               removed,
                                        guint               added,
                                        BobguiFilterListModel *self)
{
  guint filter_removed, filter_added;

  switch (self->strictness)
    {
    case BOBGUI_FILTER_MATCH_NONE:
      return;

    case BOBGUI_FILTER_MATCH_ALL:
      g_list_model_items_changed (G_LIST_MODEL (self), position, removed, added);
      if (removed != added)
        g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_N_ITEMS]);
      return;

    case BOBGUI_FILTER_MATCH_SOME:
      break;

    default:
      g_assert_not_reached ();
    }

  if (removed > 0)
    filter_removed = bobgui_bitset_get_size_in_range (self->matches, position, position + removed - 1);
  else
    filter_removed = 0;

  bobgui_bitset_splice (self->matches, position, removed, added);
  if (self->pending)
    bobgui_bitset_splice (self->pending, position, removed, added);

  if (self->watch_items)
    {
      GSequenceIter *start = g_sequence_get_iter_at_pos (self->watches, position);
      g_sequence_remove_range (start, g_sequence_iter_move (start, removed));
      bobgui_bitset_splice (self->watched_items, position, removed, added);
    }

  if (added > 0)
    {
      bobgui_filter_list_model_start_filtering (self, bobgui_bitset_new_range (position, added));
      filter_added = bobgui_bitset_get_size_in_range (self->matches, position, position + added - 1);
    }
  else
    filter_added = 0;

  if (filter_removed > 0 || filter_added > 0)
    g_list_model_items_changed (G_LIST_MODEL (self),
                                position > 0 ? bobgui_bitset_get_size_in_range (self->matches, 0, position - 1) : 0,
                                filter_removed, filter_added);
  if (filter_removed != filter_added)
    g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_N_ITEMS]);
}

static void
bobgui_filter_list_model_set_property (GObject      *object,
                                    guint         prop_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  BobguiFilterListModel *self = BOBGUI_FILTER_LIST_MODEL (object);

  switch (prop_id)
    {
    case PROP_FILTER:
      bobgui_filter_list_model_set_filter (self, g_value_get_object (value));
      break;

    case PROP_INCREMENTAL:
      bobgui_filter_list_model_set_incremental (self, g_value_get_boolean (value));
      break;

    case PROP_MODEL:
      bobgui_filter_list_model_set_model (self, g_value_get_object (value));
      break;

    case PROP_WATCH_ITEMS:
      bobgui_filter_list_model_set_watch_items (self, g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_filter_list_model_get_property (GObject     *object,
                                    guint        prop_id,
                                    GValue      *value,
                                    GParamSpec  *pspec)
{
  BobguiFilterListModel *self = BOBGUI_FILTER_LIST_MODEL (object);

  switch (prop_id)
    {
    case PROP_FILTER:
      g_value_set_object (value, self->filter);
      break;

    case PROP_INCREMENTAL:
      g_value_set_boolean (value, self->incremental);
      break;

    case PROP_ITEM_TYPE:
      g_value_set_gtype (value, bobgui_filter_list_model_get_item_type (G_LIST_MODEL (self)));
      break;

    case PROP_MODEL:
      g_value_set_object (value, self->model);
      break;

    case PROP_N_ITEMS:
      g_value_set_uint (value, bobgui_filter_list_model_get_n_items (G_LIST_MODEL (self)));
      break;

    case PROP_PENDING:
      g_value_set_uint (value, bobgui_filter_list_model_get_pending (self));
      break;

    case PROP_WATCH_ITEMS:
      g_value_set_boolean (value, bobgui_filter_list_model_get_watch_items (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static inline void
remove_all_watches (BobguiFilterListModel *self)
{
  if (self->watches && !g_sequence_is_empty (self->watches))
    g_sequence_remove_range (g_sequence_get_begin_iter (self->watches),
                             g_sequence_get_end_iter (self->watches));
  if (self->watched_items)
    bobgui_bitset_remove_all (self->watched_items);
}

static void
bobgui_filter_list_model_clear_model (BobguiFilterListModel *self)
{
  if (self->model == NULL)
    return;

  remove_all_watches (self);

  bobgui_filter_list_model_stop_filtering (self);
  g_signal_handlers_disconnect_by_func (self->model, bobgui_filter_list_model_items_changed_cb, self);
  g_signal_handlers_disconnect_by_func (self->model, bobgui_filter_list_model_sections_changed_cb, self);
  g_clear_object (&self->model);
  if (self->matches)
    bobgui_bitset_remove_all (self->matches);
}

static void
bobgui_filter_list_model_refilter (BobguiFilterListModel *self,
                                BobguiFilterChange     change)
{
  BobguiFilterMatch new_strictness;

  if (self->model == NULL)
    new_strictness = BOBGUI_FILTER_MATCH_NONE;
  else if (self->filter == NULL)
    new_strictness = BOBGUI_FILTER_MATCH_ALL;
  else
    new_strictness = bobgui_filter_get_strictness (self->filter);

  /* Item watches only make sense with BOBGUI_FILTER_MATCH_SOME; drop
   * them for every other situation.
   */
  if (new_strictness != self->strictness && new_strictness != BOBGUI_FILTER_MATCH_SOME)
    remove_all_watches (self);

  /* don't set self->strictness yet so get_n_items() and friends return old values */

  switch (new_strictness)
    {
    case BOBGUI_FILTER_MATCH_NONE:
      {
        guint n_before = g_list_model_get_n_items (G_LIST_MODEL (self));
        g_clear_pointer (&self->matches, bobgui_bitset_unref);
        self->strictness = new_strictness;
        bobgui_filter_list_model_stop_filtering (self);
        if (n_before > 0)
          {
            g_list_model_items_changed (G_LIST_MODEL (self), 0, n_before, 0);
            g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_N_ITEMS]);
          }
      }
      break;

    case BOBGUI_FILTER_MATCH_ALL:
      switch (self->strictness)
        {
        case BOBGUI_FILTER_MATCH_NONE:
          {
            guint n_items;

            self->strictness = new_strictness;
            n_items = g_list_model_get_n_items (self->model);
            if (n_items > 0)
              {
                g_list_model_items_changed (G_LIST_MODEL (self), 0, 0, n_items);
                g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_N_ITEMS]);
              }
          }
          break;
        case BOBGUI_FILTER_MATCH_ALL:
          self->strictness = new_strictness;
          break;
        default:
        case BOBGUI_FILTER_MATCH_SOME:
          {
            guint n_before, n_after;

            bobgui_filter_list_model_stop_filtering (self);
            self->strictness = new_strictness;
            n_after = g_list_model_get_n_items (G_LIST_MODEL (self));
            n_before = bobgui_bitset_get_size (self->matches);
            if (n_before == n_after)
              {
                g_clear_pointer (&self->matches, bobgui_bitset_unref);
              }
            else
              {
                BobguiBitset *inverse;
                guint start, end;

                inverse = bobgui_bitset_new_range (0, n_after);
                bobgui_bitset_subtract (inverse, self->matches);
                /* otherwise all items would be visible */
                g_assert (!bobgui_bitset_is_empty (inverse));

                /* find first filtered */
                start = bobgui_bitset_get_minimum (inverse);
                end = n_after - bobgui_bitset_get_maximum (inverse) - 1;

                bobgui_bitset_unref (inverse);

                g_clear_pointer (&self->matches, bobgui_bitset_unref);
                g_list_model_items_changed (G_LIST_MODEL (self), start, n_before - end - start, n_after - end - start);
                g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_N_ITEMS]);
              }
          }
          break;
        }
      break;

    default:
      g_assert_not_reached ();
      break;

    case BOBGUI_FILTER_MATCH_SOME:
      {
        BobguiBitset *old, *pending;

        if (self->matches == NULL)
          {
            if (self->strictness == BOBGUI_FILTER_MATCH_ALL)
              old = bobgui_bitset_new_range (0, g_list_model_get_n_items (self->model));
            else
              old = bobgui_bitset_new_empty ();
          }
        else
          {
            old = self->matches;
          }
        self->strictness = new_strictness;
        switch (change)
          {
          default:
            g_assert_not_reached ();
            /* fall thru */
          case BOBGUI_FILTER_CHANGE_DIFFERENT_REWATCH:
            remove_all_watches (self);
            G_GNUC_FALLTHROUGH;
          case BOBGUI_FILTER_CHANGE_DIFFERENT:
            self->matches = bobgui_bitset_new_empty ();
            pending = bobgui_bitset_new_range (0, g_list_model_get_n_items (self->model));
            break;

          case BOBGUI_FILTER_CHANGE_LESS_STRICT_REWATCH:
            remove_all_watches (self);
            G_GNUC_FALLTHROUGH;
          case BOBGUI_FILTER_CHANGE_LESS_STRICT:
            self->matches = bobgui_bitset_copy (old);
            pending = bobgui_bitset_new_range (0, g_list_model_get_n_items (self->model));
            bobgui_bitset_subtract (pending, self->matches);
            break;

          case BOBGUI_FILTER_CHANGE_MORE_STRICT_REWATCH:
            remove_all_watches (self);
            G_GNUC_FALLTHROUGH;
          case BOBGUI_FILTER_CHANGE_MORE_STRICT:
            self->matches = bobgui_bitset_new_empty ();
            pending = bobgui_bitset_copy (old);
            break;
          }
        bobgui_filter_list_model_start_filtering (self, pending);

        bobgui_filter_list_model_emit_items_changed_for_changes (self, old);
      }
    }
}

static void
bobgui_filter_list_model_filter_changed_cb (BobguiFilter          *filter,
                                         BobguiFilterChange     change,
                                         BobguiFilterListModel *self)
{
  bobgui_filter_list_model_refilter (self, change);
}

static void
bobgui_filter_list_model_clear_filter (BobguiFilterListModel *self)
{
  if (self->filter == NULL)
    return;

  remove_all_watches (self);

  g_signal_handlers_disconnect_by_func (self->filter, bobgui_filter_list_model_filter_changed_cb, self);
  g_clear_object (&self->filter);
}

static void
bobgui_filter_list_model_dispose (GObject *object)
{
  BobguiFilterListModel *self = BOBGUI_FILTER_LIST_MODEL (object);

  bobgui_filter_list_model_clear_model (self);
  bobgui_filter_list_model_clear_filter (self);
  g_clear_pointer (&self->matches, bobgui_bitset_unref);
  g_clear_pointer (&self->watched_items, bobgui_bitset_unref);
  g_clear_pointer (&self->watches, g_sequence_free);

  G_OBJECT_CLASS (bobgui_filter_list_model_parent_class)->dispose (object);
}

static void
bobgui_filter_list_model_class_init (BobguiFilterListModelClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);

  gobject_class->set_property = bobgui_filter_list_model_set_property;
  gobject_class->get_property = bobgui_filter_list_model_get_property;
  gobject_class->dispose = bobgui_filter_list_model_dispose;

  /**
   * BobguiFilterListModel:filter:
   *
   * The filter for this model.
   */
  properties[PROP_FILTER] =
      g_param_spec_object ("filter", NULL, NULL,
                           BOBGUI_TYPE_FILTER,
                           BOBGUI_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiFilterListModel:incremental:
   *
   * If the model should filter items incrementally.
   */
  properties[PROP_INCREMENTAL] =
      g_param_spec_boolean ("incremental", NULL, NULL,
                            FALSE,
                            BOBGUI_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiFilterListModel:item-type:
   *
   * The type of items. See [method@Gio.ListModel.get_item_type].
   *
   * Since: 4.8
   **/
  properties[PROP_ITEM_TYPE] =
    g_param_spec_gtype ("item-type", NULL, NULL,
                        G_TYPE_OBJECT,
                        G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  /**
   * BobguiFilterListModel:model:
   *
   * The model being filtered.
   */
  properties[PROP_MODEL] =
      g_param_spec_object ("model", NULL, NULL,
                           G_TYPE_LIST_MODEL,
                           BOBGUI_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiFilterListModel:n-items:
   *
   * The number of items. See [method@Gio.ListModel.get_n_items].
   *
   * Since: 4.8
   **/
  properties[PROP_N_ITEMS] =
    g_param_spec_uint ("n-items", NULL, NULL,
                       0, G_MAXUINT, 0,
                       G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  /**
   * BobguiFilterListModel:pending:
   *
   * Number of items not yet filtered.
   */
  properties[PROP_PENDING] =
      g_param_spec_uint ("pending", NULL, NULL,
                         0, G_MAXUINT, 0,
                         BOBGUI_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY);


  /**
   * BobguiFilterListModel:watch-items:
   *
   * Monitor the list items for changes. It may impact performance.
   *
   * Since: 4.20
   */
  properties[PROP_WATCH_ITEMS] =
      g_param_spec_boolean ("watch-items", NULL, NULL,
                            FALSE,
                            BOBGUI_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (gobject_class, NUM_PROPERTIES, properties);
}

static void
bobgui_filter_list_model_init (BobguiFilterListModel *self)
{
  self->strictness = BOBGUI_FILTER_MATCH_NONE;
}

/**
 * bobgui_filter_list_model_new:
 * @model: (nullable) (transfer full): the model to sort
 * @filter: (nullable) (transfer full): filter
 *
 * Creates a new `BobguiFilterListModel` that will filter @model using the given
 * @filter.
 *
 * Returns: a new `BobguiFilterListModel`
 **/
BobguiFilterListModel *
bobgui_filter_list_model_new (GListModel *model,
                           BobguiFilter  *filter)
{
  BobguiFilterListModel *result;

  g_return_val_if_fail (model == NULL || G_IS_LIST_MODEL (model), NULL);
  g_return_val_if_fail (filter == NULL || BOBGUI_IS_FILTER (filter), NULL);

  result = g_object_new (BOBGUI_TYPE_FILTER_LIST_MODEL,
                         "model", model,
                         "filter", filter,
                         NULL);

  /* consume the references */
  g_clear_object (&model);
  g_clear_object (&filter);

  return result;
}

/**
 * bobgui_filter_list_model_set_filter:
 * @self: a `BobguiFilterListModel`
 * @filter: (nullable) (transfer none): filter to use
 *
 * Sets the filter used to filter items.
 **/
void
bobgui_filter_list_model_set_filter (BobguiFilterListModel *self,
                                  BobguiFilter          *filter)
{
  g_return_if_fail (BOBGUI_IS_FILTER_LIST_MODEL (self));
  g_return_if_fail (filter == NULL || BOBGUI_IS_FILTER (filter));

  if (self->filter == filter)
    return;

  bobgui_filter_list_model_clear_filter (self);

  if (filter)
    {
      self->filter = g_object_ref (filter);
      g_signal_connect (filter, "changed", G_CALLBACK (bobgui_filter_list_model_filter_changed_cb), self);
      bobgui_filter_list_model_filter_changed_cb (filter, BOBGUI_FILTER_CHANGE_DIFFERENT, self);
    }
  else
    {
      bobgui_filter_list_model_refilter (self, BOBGUI_FILTER_CHANGE_LESS_STRICT);
    }

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_FILTER]);
}

/**
 * bobgui_filter_list_model_get_filter:
 * @self: a `BobguiFilterListModel`
 *
 * Gets the `BobguiFilter` currently set on @self.
 *
 * Returns: (nullable) (transfer none): The filter currently in use
 */
BobguiFilter *
bobgui_filter_list_model_get_filter (BobguiFilterListModel *self)
{
  g_return_val_if_fail (BOBGUI_IS_FILTER_LIST_MODEL (self), FALSE);

  return self->filter;
}

/**
 * bobgui_filter_list_model_set_model:
 * @self: a `BobguiFilterListModel`
 * @model: (nullable): The model to be filtered
 *
 * Sets the model to be filtered.
 *
 * Note that BOBGUI makes no effort to ensure that @model conforms to
 * the item type of @self. It assumes that the caller knows what they
 * are doing and have set up an appropriate filter to ensure that item
 * types match.
 */
void
bobgui_filter_list_model_set_model (BobguiFilterListModel *self,
                                 GListModel         *model)
{
  guint removed, added;

  g_return_if_fail (BOBGUI_IS_FILTER_LIST_MODEL (self));
  g_return_if_fail (model == NULL || G_IS_LIST_MODEL (model));
  /* Note: We don't check for matching item type here, we just assume the
   * filter func takes care of filtering wrong items. */

  if (self->model == model)
    return;

  removed = g_list_model_get_n_items (G_LIST_MODEL (self));
  bobgui_filter_list_model_clear_model (self);

  if (model)
    {
      self->model = g_object_ref (model);
      g_signal_connect (model, "items-changed", G_CALLBACK (bobgui_filter_list_model_items_changed_cb), self);
      if (BOBGUI_IS_SECTION_MODEL (model))
        g_signal_connect (model, "sections-changed", G_CALLBACK (bobgui_filter_list_model_sections_changed_cb), self);
      if (removed == 0)
        {
          self->strictness = BOBGUI_FILTER_MATCH_NONE;
          bobgui_filter_list_model_refilter (self, BOBGUI_FILTER_CHANGE_LESS_STRICT);
          added = 0;
        }
      else if (self->matches)
        {
          bobgui_filter_list_model_start_filtering (self, bobgui_bitset_new_range (0, g_list_model_get_n_items (model)));
          added = bobgui_bitset_get_size (self->matches);
        }
      else
        {
          added = g_list_model_get_n_items (model);
        }
    }
  else
    {
      self->strictness = BOBGUI_FILTER_MATCH_NONE;
      added = 0;
    }

  if (removed > 0 || added > 0)
    g_list_model_items_changed (G_LIST_MODEL (self), 0, removed, added);
  if (removed != added)
    g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_N_ITEMS]);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_MODEL]);
}

/**
 * bobgui_filter_list_model_get_model:
 * @self: a `BobguiFilterListModel`
 *
 * Gets the model currently filtered or %NULL if none.
 *
 * Returns: (nullable) (transfer none): The model that gets filtered
 */
GListModel *
bobgui_filter_list_model_get_model (BobguiFilterListModel *self)
{
  g_return_val_if_fail (BOBGUI_IS_FILTER_LIST_MODEL (self), NULL);

  return self->model;
}

/**
 * bobgui_filter_list_model_set_incremental:
 * @self: a `BobguiFilterListModel`
 * @incremental: %TRUE to enable incremental filtering
 *
 * Sets the filter model to do an incremental sort.
 *
 * When incremental filtering is enabled, the `BobguiFilterListModel` will not
 * run filters immediately, but will instead queue an idle handler that
 * incrementally filters the items and adds them to the list. This of course
 * means that items are not instantly added to the list, but only appear
 * incrementally.
 *
 * When your filter blocks the UI while filtering, you might consider
 * turning this on. Depending on your model and filters, this may become
 * interesting around 10,000 to 100,000 items.
 *
 * By default, incremental filtering is disabled.
 *
 * See [method@Bobgui.FilterListModel.get_pending] for progress information
 * about an ongoing incremental filtering operation.
 **/
void
bobgui_filter_list_model_set_incremental (BobguiFilterListModel *self,
                                       gboolean            incremental)
{
  g_return_if_fail (BOBGUI_IS_FILTER_LIST_MODEL (self));

  if (self->incremental == incremental)
    return;

  self->incremental = incremental;

  if (!incremental)
    {
      BobguiBitset *old;
      bobgui_filter_list_model_run_filter (self, G_MAXUINT);

      old = bobgui_bitset_copy (self->matches);
      bobgui_filter_list_model_run_filter (self, 512);

      bobgui_filter_list_model_stop_filtering (self);

      bobgui_filter_list_model_emit_items_changed_for_changes (self, old);
      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_PENDING]);
    }

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_INCREMENTAL]);
}

/**
 * bobgui_filter_list_model_get_incremental:
 * @self: a `BobguiFilterListModel`
 *
 * Returns whether incremental filtering is enabled.
 *
 * See [method@Bobgui.FilterListModel.set_incremental].
 *
 * Returns: %TRUE if incremental filtering is enabled
 */
gboolean
bobgui_filter_list_model_get_incremental (BobguiFilterListModel *self)
{
  g_return_val_if_fail (BOBGUI_IS_FILTER_LIST_MODEL (self), FALSE);

  return self->incremental;
}

/**
 * bobgui_filter_list_model_get_pending:
 * @self: a `BobguiFilterListModel`
 *
 * Returns the number of items that have not been filtered yet.
 *
 * You can use this value to check if @self is busy filtering by
 * comparing the return value to 0 or you can compute the percentage
 * of the filter remaining by dividing the return value by the total
 * number of items in the underlying model:
 *
 * ```c
 * pending = bobgui_filter_list_model_get_pending (self);
 * model = bobgui_filter_list_model_get_model (self);
 * percentage = pending / (double) g_list_model_get_n_items (model);
 * ```
 *
 * If no filter operation is ongoing - in particular when
 * [property@Bobgui.FilterListModel:incremental] is %FALSE - this
 * function returns 0.
 *
 * Returns: The number of items not yet filtered
 */
guint
bobgui_filter_list_model_get_pending (BobguiFilterListModel *self)
{
  g_return_val_if_fail (BOBGUI_IS_FILTER_LIST_MODEL (self), FALSE);

  if (self->pending == NULL)
    return 0;

  return bobgui_bitset_get_size (self->pending);
}

/**
 * bobgui_filter_list_model_get_watch_items:
 * @self: a `BobguiFilterListModel`
 *
 * Returns whether watching items is enabled.
 *
 * See [method@Bobgui.FilterListModel.set_watch_items].
 *
 * Returns: %TRUE if watching items is enabled
 *
 * Since: 4.20
 */
gboolean
bobgui_filter_list_model_get_watch_items (BobguiFilterListModel *self)
{
  g_return_val_if_fail (BOBGUI_IS_FILTER_LIST_MODEL (self), FALSE);

  return self->watch_items;
}

/**
 * bobgui_filter_list_model_set_watch_items:
 * @self: a `BobguiFilterListModel`
 * @watch_items: %TRUE to watch items for property changes
 *
 * Sets the filter model to monitor properties of its items.
 *
 * This allows implementations of [class@Bobgui.Filter] that support expression
 * watching to react to property changes. This property has no effect if the
 * current filter doesn't support watching items.
 *
 * By default, watching items is disabled.
 *
 * Since: 4.20
 **/
void
bobgui_filter_list_model_set_watch_items (BobguiFilterListModel *self,
                                       gboolean            watch_items)
{
  g_return_if_fail (BOBGUI_IS_FILTER_LIST_MODEL (self));

  if (self->watch_items == watch_items)
    return;

  self->watch_items = watch_items;

  if (watch_items)
    {
      g_assert (self->watches == NULL);
      g_assert (self->watched_items == NULL);
      self->watched_items = bobgui_bitset_new_empty ();
      self->watches = g_sequence_new (watch_data_free);
      setup_all_watches (self);
    }
  else
    {
      g_assert (self->watches != NULL);
      g_assert (self->watched_items != NULL);
      g_clear_pointer (&self->watches, g_sequence_free);
      g_clear_pointer (&self->watched_items, bobgui_bitset_unref);
    }

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_WATCH_ITEMS]);
}
