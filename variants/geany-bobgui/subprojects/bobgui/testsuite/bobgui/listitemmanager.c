/*
 * Copyright © 2023 Benjamin Otte
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

#include <locale.h>

#include <bobgui/bobgui.h>
#include "bobgui/bobguilistitemmanagerprivate.h"
#include "bobgui/bobguilistbaseprivate.h"

static GListModel *
create_source_model (guint min_size, guint max_size)
{
  BobguiStringList *list;
  guint i, size;

  size = g_test_rand_int_range (min_size, max_size + 1);
  list = bobgui_string_list_new (NULL);

  for (i = 0; i < size; i++)
    bobgui_string_list_append (list, g_test_rand_bit () ? "A" : "B");

  return G_LIST_MODEL (list);
}

void
print_list_item_manager_tiles (BobguiListItemManager *items)
{
  GString *string;
  BobguiListTile *tile;

  string = g_string_new ("");

  for (tile = bobgui_list_item_manager_get_first (items);
       tile != NULL;
       tile = bobgui_rb_tree_node_get_next (tile))
    {
      switch (tile->type)
        {
          case BOBGUI_LIST_TILE_ITEM:
            if (tile->widget)
              g_string_append_c (string, 'W');
            else if (tile->n_items == 1)
              g_string_append_c (string, 'x');
            else
              g_string_append_printf (string, "%u,", tile->n_items);
            break;
          case BOBGUI_LIST_TILE_HEADER:
            g_string_append_c (string, '[');
            break;
          case BOBGUI_LIST_TILE_UNMATCHED_HEADER:
            g_string_append_c (string, '(');
            break;
          case BOBGUI_LIST_TILE_FOOTER:
            g_string_append_c (string, ']');
            break;
          case BOBGUI_LIST_TILE_UNMATCHED_FOOTER:
            g_string_append_c (string, ')');
            break;

          case BOBGUI_LIST_TILE_REMOVED:
            g_string_append_c (string, '.');
            break;
          default:
            g_assert_not_reached ();
            break;
        }
    }

  g_print ("%s\n", string->str);

  g_string_free (string, TRUE);
}

static gsize
widget_count_children (BobguiWidget *widget)
{
  BobguiWidget *child;
  gsize n_children = 0;

  for (child = bobgui_widget_get_first_child (widget);
       child;
       child = bobgui_widget_get_next_sibling (child))
    n_children++;

  return n_children;
}

static void
check_list_item_manager (BobguiListItemManager  *items,
                         BobguiWidget           *widget,
                         BobguiListItemTracker **trackers,
                         gsize                n_trackers)
{
  GListModel *model = G_LIST_MODEL (bobgui_list_item_manager_get_model (items));
  BobguiListTile *tile;
  guint n_items, n_tile_widgets;
  guint i;
  gboolean has_sections;
  enum {
    NO_SECTION,
    MATCHED_SECTION,
    UNMATCHED_SECTION
  } section_state = NO_SECTION;
  gboolean after_items = FALSE;

  has_sections = bobgui_list_item_manager_get_has_sections (items);

  n_items = 0;
  n_tile_widgets = 0;

  for (tile = bobgui_list_item_manager_get_first (items);
       tile != NULL;
       tile = bobgui_rb_tree_node_get_next (tile))
    {
      switch (tile->type)
        {
          case BOBGUI_LIST_TILE_HEADER:
            g_assert_cmpint (section_state, ==, NO_SECTION);
            g_assert_cmpint (tile->n_items, ==, 0);
            g_assert_true (has_sections);
            g_assert_true (tile->widget);
            section_state = MATCHED_SECTION;
            after_items = FALSE;
            break;

          case BOBGUI_LIST_TILE_UNMATCHED_HEADER:
            g_assert_cmpint (section_state, ==, NO_SECTION);
            g_assert_cmpint (tile->n_items, ==, 0);
            g_assert_null (tile->widget);
            section_state = UNMATCHED_SECTION;
            after_items = FALSE;
            break;

          case BOBGUI_LIST_TILE_FOOTER:
            g_assert_cmpint (section_state, ==, MATCHED_SECTION);
            g_assert_cmpint (tile->n_items, ==, 0);
            g_assert_true (has_sections);
            g_assert_null (tile->widget);
            section_state = NO_SECTION;
            after_items = FALSE;
            break;

          case BOBGUI_LIST_TILE_UNMATCHED_FOOTER:
            g_assert_cmpint (section_state, ==, UNMATCHED_SECTION);
            g_assert_cmpint (tile->n_items, ==, 0);
            g_assert_null (tile->widget);
            section_state = NO_SECTION;
            after_items = FALSE;
            break;

          case BOBGUI_LIST_TILE_ITEM:
            g_assert_cmpint (section_state, !=, NO_SECTION);
            if (tile->widget)
              {
                GObject *item = g_list_model_get_item (model, n_items);
                if (has_sections)
                  g_assert_cmpint (section_state, ==, MATCHED_SECTION);
                else
                  g_assert_cmpint (section_state, ==, UNMATCHED_SECTION);
                g_assert_cmphex (GPOINTER_TO_SIZE (item), ==, GPOINTER_TO_SIZE (bobgui_list_item_base_get_item (BOBGUI_LIST_ITEM_BASE (tile->widget))));
                g_object_unref (item);
                g_assert_cmpint (n_items, ==, bobgui_list_item_base_get_position (BOBGUI_LIST_ITEM_BASE (tile->widget)));
                g_assert_cmpint (tile->n_items, ==, 1);
                after_items = FALSE;
              }
            else
              {
                after_items = TRUE;
              }
            if (tile->n_items)
              n_items += tile->n_items;
            break;

          case BOBGUI_LIST_TILE_REMOVED:
            g_assert_cmpint (tile->n_items, ==, 0);
            g_assert_null (tile->widget);
            break;

          default:
            g_assert_not_reached ();
            break;
        }

      if (tile->widget)
        n_tile_widgets++;
    }

  g_assert_cmpint (section_state, ==, NO_SECTION);
  g_assert_cmpint (n_items, ==, g_list_model_get_n_items (model));
  g_assert_cmpint (n_tile_widgets, ==, widget_count_children (widget));

  for (i = 0; i < n_trackers; i++)
    {
      guint pos, offset;

      pos = bobgui_list_item_tracker_get_position (items, trackers[i]);
      if (pos == BOBGUI_INVALID_LIST_POSITION)
        continue;
      tile = bobgui_list_item_manager_get_nth (items, pos, &offset);
      g_assert_cmpint (tile->n_items, ==, 1);
      g_assert_cmpint (offset, ==, 0);
      g_assert_true (tile->widget);
    }

  bobgui_list_item_manager_gc_tiles (items);

  n_items = 0;
  n_tile_widgets = 0;

  for (tile = bobgui_list_item_manager_get_first (items);
       tile != NULL;
       tile = bobgui_rb_tree_node_get_next (tile))
    {
      switch (tile->type)
        {
          case BOBGUI_LIST_TILE_HEADER:
            g_assert_cmpint (section_state, ==, NO_SECTION);
            g_assert_cmpint (tile->n_items, ==, 0);
            g_assert_true (has_sections);
            g_assert_true (tile->widget);
            section_state = MATCHED_SECTION;
            after_items = FALSE;
            break;

          case BOBGUI_LIST_TILE_UNMATCHED_HEADER:
            g_assert_cmpint (section_state, ==, NO_SECTION);
            g_assert_cmpint (tile->n_items, ==, 0);
            g_assert_false (tile->widget);
            section_state = UNMATCHED_SECTION;
            after_items = FALSE;
            break;

          case BOBGUI_LIST_TILE_FOOTER:
            g_assert_cmpint (section_state, ==, MATCHED_SECTION);
            g_assert_cmpint (tile->n_items, ==, 0);
            g_assert_true (has_sections);
            g_assert_false (tile->widget);
            section_state = NO_SECTION;
            after_items = FALSE;
            break;

          case BOBGUI_LIST_TILE_UNMATCHED_FOOTER:
            g_assert_cmpint (section_state, ==, UNMATCHED_SECTION);
            g_assert_cmpint (tile->n_items, ==, 0);
            g_assert_false (tile->widget);
            section_state = NO_SECTION;
            after_items = FALSE;
            break;

          case BOBGUI_LIST_TILE_ITEM:
            g_assert_cmpint (section_state, !=, NO_SECTION);
            if (tile->widget)
              {
                g_assert_cmpint (tile->n_items, ==, 1);
                after_items = FALSE;
              }
            else
              {
                g_assert_false (after_items);
                after_items = TRUE;
              }
            if (tile->n_items)
              n_items += tile->n_items;
            break;

          case BOBGUI_LIST_TILE_REMOVED:
          default:
            g_assert_not_reached ();
            break;
        }

      if (tile->widget)
        n_tile_widgets++;
    }

  g_assert_cmpint (section_state, ==, NO_SECTION);
  g_assert_cmpint (n_items, ==, g_list_model_get_n_items (model));
  g_assert_cmpint (n_tile_widgets, ==, widget_count_children (widget));

  for (i = 0; i < n_trackers; i++)
    {
      guint pos, offset;

      pos = bobgui_list_item_tracker_get_position (items, trackers[i]);
      if (pos == BOBGUI_INVALID_LIST_POSITION)
        continue;
      tile = bobgui_list_item_manager_get_nth (items, pos, &offset);
      g_assert_cmpint (tile->n_items, ==, 1);
      g_assert_cmpint (offset, ==, 0);
      g_assert_true (tile->widget);
    }
}

static BobguiListTile *
split_simple (BobguiWidget   *widget,
              BobguiListTile *tile,
              guint        n_items)
{
  BobguiListItemManager *items = g_object_get_data (G_OBJECT (widget), "the-items");

  return bobgui_list_tile_split (items, tile, n_items);
}

static void
prepare_simple (BobguiWidget   *widget,
                BobguiListTile *tile,
                guint        n_items)
{
}

static BobguiListItemBase *
create_simple_item (BobguiWidget *widget)
{
  return g_object_new (BOBGUI_TYPE_LIST_ITEM_BASE, NULL);
}

static BobguiListHeaderBase *
create_simple_header (BobguiWidget *widget)
{
  return g_object_new (BOBGUI_TYPE_LIST_HEADER_BASE, NULL);
}

static void
test_create (void)
{
  BobguiListItemManager *items;
  BobguiWidget *widget;

  widget = bobgui_window_new ();
  items = bobgui_list_item_manager_new (widget,
                                     split_simple,
                                     create_simple_item,
                                     prepare_simple,
                                     create_simple_header);
  g_object_set_data_full (G_OBJECT (widget), "the-items", items, g_object_unref);

  bobgui_window_destroy (BOBGUI_WINDOW (widget));
}

static void
test_create_with_items (void)
{
  GListModel *source;
  BobguiNoSelection *selection;
  BobguiListItemManager *items;
  BobguiWidget *widget;

  widget = bobgui_window_new ();
  items = bobgui_list_item_manager_new (widget,
                                     split_simple,
                                     create_simple_item,
                                     prepare_simple,
                                     create_simple_header);
  g_object_set_data_full (G_OBJECT (widget), "the-items", items, g_object_unref);

  source = create_source_model (1, 50);
  selection = bobgui_no_selection_new (G_LIST_MODEL (source));
  bobgui_list_item_manager_set_model (items, BOBGUI_SELECTION_MODEL (selection));
  check_list_item_manager (items, widget, NULL, 0);
  bobgui_list_item_manager_set_model (items, BOBGUI_SELECTION_MODEL (selection));
  check_list_item_manager (items, widget, NULL, 0);

  g_object_unref (selection);
  bobgui_window_destroy (BOBGUI_WINDOW (widget));
}

#define N_TRACKERS 3
#define N_WIDGETS_PER_TRACKER 10
#define N_RUNS 500

static void
print_changes_cb (GListModel *model,
                  guint       position,
                  guint       removed,
                  guint       added,
                  gpointer    unused)
{
  if (!g_test_verbose ())
    return;

  if (removed == 0)
    g_test_message ("%u/%u: adding %u items", position, g_list_model_get_n_items (model), added);
  else if (added == 0)
    g_test_message ("%u/%u: removing %u items", position, g_list_model_get_n_items (model), removed);
  else
    g_test_message ("%u/%u: removing %u and adding %u items", position, g_list_model_get_n_items (model), removed, added);
}

static void
test_exhaustive (void)
{
  BobguiListItemTracker *trackers[N_TRACKERS];
  GListStore *store;
  BobguiFlattenListModel *flatten;
  BobguiNoSelection *selection;
  BobguiListItemManager *items;
  BobguiWidget *widget;
  gsize i;

  widget = bobgui_window_new ();
  items = bobgui_list_item_manager_new (widget,
                                     split_simple,
                                     create_simple_item,
                                     prepare_simple,
                                     create_simple_header);
  for (i = 0; i < N_TRACKERS; i++)
    trackers[i] = bobgui_list_item_tracker_new (items);

  g_object_set_data_full (G_OBJECT (widget), "the-items", items, g_object_unref);

  store = g_list_store_new (G_TYPE_OBJECT);
  flatten = bobgui_flatten_list_model_new (G_LIST_MODEL (store));
  selection = bobgui_no_selection_new (G_LIST_MODEL (flatten));
  g_signal_connect (selection, "items-changed", G_CALLBACK (print_changes_cb), NULL);
  bobgui_list_item_manager_set_model (items, BOBGUI_SELECTION_MODEL (selection));

  for (i = 0; i < N_RUNS; i++)
    {
      gboolean add = FALSE, remove = FALSE;
      guint position, n_items;

      switch (g_test_rand_int_range (0, 7))
      {
        case 0:
          if (g_test_verbose ())
            g_test_message ("GC and checking");
          check_list_item_manager (items, widget, trackers, N_TRACKERS);
          break;

        case 1:
          /* remove a model */
          remove = TRUE;
          break;

        case 2:
          /* add a model */
          add = TRUE;
          break;

        case 3:
          /* replace a model */
          remove = TRUE;
          add = TRUE;
          break;

        case 4:
          n_items = g_list_model_get_n_items (G_LIST_MODEL (selection));
          if (n_items > 0)
            {
              guint tracker_id = g_test_rand_int_range (0, N_TRACKERS);
              guint pos = g_test_rand_int_range (0, n_items);
              guint n_before = g_test_rand_int_range (0, N_WIDGETS_PER_TRACKER / 2);
              guint n_after = g_test_rand_int_range (0, N_WIDGETS_PER_TRACKER / 2);
              if (g_test_verbose ())
                g_test_message ("setting tracker %u to %u -%u + %u", tracker_id, pos, n_before, n_after);
              bobgui_list_item_tracker_set_position (items,
                                                  trackers [tracker_id],
                                                  pos,
                                                  n_before, n_after);
            }
          break;

        case 5:
          {
            gboolean has_sections = g_test_rand_bit ();
            if (g_test_verbose ())
              g_test_message ("Setting has_sections to %s", has_sections ? "true" : "false");
            bobgui_list_item_manager_set_has_sections (items, has_sections);
          }
          break;

        case 6:
          {
            n_items = g_list_model_get_n_items (G_LIST_MODEL (store));
            if (n_items > 0)
              {
                guint j = g_test_rand_int_range (0, n_items);
                GListModel *source = g_list_model_get_item (G_LIST_MODEL (store), j);
                guint source_size = g_list_model_get_n_items (G_LIST_MODEL (source));
                GStrvBuilder *builder = g_strv_builder_new ();
                guint inclusion_size = g_test_rand_int_range (1, 11);
                char **inclusion;

                for (j = 0; j < inclusion_size; j++)
                  g_strv_builder_add (builder, g_test_rand_bit () ? "A" : "B");
                inclusion = g_strv_builder_end (builder);
                g_strv_builder_unref (builder);

                j = g_test_rand_int_range (0, source_size + 1);
                bobgui_string_list_splice (BOBGUI_STRING_LIST (source), j, 0, (const char * const *) inclusion);
                g_strfreev (inclusion);

                if (g_test_verbose ())
                  g_test_message ("Adding %u items at position %u of a section which had %u items",
                                  inclusion_size, j, source_size);
              }
          }
          break;

        default:
          g_assert_not_reached ();
          break;
      }

      position = g_test_rand_int_range (0, g_list_model_get_n_items (G_LIST_MODEL (store)) + 1);
      if (g_list_model_get_n_items (G_LIST_MODEL (store)) == position)
        remove = FALSE;

      if (add)
        {
          /* We want at least one element, otherwise the filters will see no changes */
          GListModel *source = create_source_model (1, 50);
          g_list_store_splice (store,
                               position,
                               remove ? 1 : 0,
                               (gpointer *) &source, 1);
          g_object_unref (source);
        }
      else if (remove)
        {
          g_list_store_remove (store, position);
        }

      if (g_test_verbose ())
        print_list_item_manager_tiles (items);
    }

  check_list_item_manager (items, widget, trackers, N_TRACKERS);

  if (g_test_verbose ())
    g_test_message ("removing trackers");
  for (i = 0; i < N_TRACKERS; i++)
    bobgui_list_item_tracker_free (items, trackers[i]);
  g_object_unref (selection);
  if (g_test_verbose ())
    print_list_item_manager_tiles (items);
  check_list_item_manager (items, widget, NULL, 0);

  bobgui_window_destroy (BOBGUI_WINDOW (widget));
}

int
main (int argc, char *argv[])
{
  bobgui_test_init (&argc, &argv);

  g_test_add_func ("/listitemmanager/create", test_create);
  g_test_add_func ("/listitemmanager/create_with_items", test_create_with_items);
  g_test_add_func ("/listitemmanager/exhaustive", test_exhaustive);

  return g_test_run ();
}
