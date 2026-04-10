/* Propertylookupmodel tests.
 *
 * Copyright (C) 2011, Red Hat, Inc.
 * Authors: Benjamin Otte <otte@gnome.org>
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

#include "bobgui/bobguipropertylookuplistmodelprivate.h"

GQuark changes_quark;

static char *
model_to_string (GListModel *model)
{
  GString *string = g_string_new (NULL);
  gpointer item;
  guint i;

  for (i = 0; i < g_list_model_get_n_items (model); i++)
    {
      if (i > 0)
        g_string_append (string, " ");
      item = g_list_model_get_item (model, i);
      g_string_append_printf (string, "%s", G_OBJECT_TYPE_NAME (item));
      g_object_unref (item);
    }

  return g_string_free (string, FALSE);
}

#define assert_model(model, expected) G_STMT_START{ \
  char *s = model_to_string (G_LIST_MODEL (model)); \
  if (!g_str_equal (s, expected)) \
     g_assertion_message_cmpstr (G_LOG_DOMAIN, __FILE__, __LINE__, G_STRFUNC, \
         #model " == " #expected, s, "==", expected); \
  g_free (s); \
}G_STMT_END

#define assert_changes(model, expected) G_STMT_START{ \
  GString *changes = g_object_get_qdata (G_OBJECT (model), changes_quark); \
  if (!g_str_equal (changes->str, expected)) \
     g_assertion_message_cmpstr (G_LOG_DOMAIN, __FILE__, __LINE__, G_STRFUNC, \
         #model " == " #expected, changes->str, "==", expected); \
  g_string_set_size (changes, 0); \
}G_STMT_END

static void
items_changed (GListModel *model,
               guint       position,
               guint       removed,
               guint       added,
               GString    *changes)
{
  g_assert_true (removed != 0 || added != 0);

  if (changes->len)
    g_string_append (changes, ", ");

  if (removed == 1 && added == 0)
    {
      g_string_append_printf (changes, "-%u", position);
    }
  else if (removed == 0 && added == 1)
    {
      g_string_append_printf (changes, "+%u", position);
    }
  else
    {
      g_string_append_printf (changes, "%u", position);
      if (removed > 0)
        g_string_append_printf (changes, "-%u", removed);
      if (added > 0)
        g_string_append_printf (changes, "+%u", added);
    }
}

static void
notify_n_items (GObject    *object,
                GParamSpec *pspec,
                GString    *changes)
{
  g_string_append_c (changes, '*');
}

static void
free_changes (gpointer data)
{
  GString *changes = data;

  /* all changes must have been checked via assert_changes() before */
  g_assert_cmpstr (changes->str, ==, "");

  g_string_free (changes, TRUE);
}

static GSList *widgets = NULL;

static BobguiWidget *
create_widget_tree (void)
{
  BobguiWidget *window, *box, *grid, *label;

  window = bobgui_window_new ();
  widgets = g_slist_prepend (widgets, window);

  box = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
  bobgui_window_set_child (BOBGUI_WINDOW (window), box);

  grid = bobgui_grid_new ();
  bobgui_box_append (BOBGUI_BOX (box), grid);

  label = bobgui_label_new ("Hello World");
  bobgui_grid_attach (BOBGUI_GRID (grid), label, 0, 0, 1, 1);

  return label;
}

static void
destroy_widgets (void)
{
  g_slist_free_full (widgets, (GDestroyNotify) bobgui_window_destroy);
  widgets = NULL;
}

static BobguiPropertyLookupListModel *
new_model (gboolean fill)
{
  BobguiPropertyLookupListModel *result;
  GString *changes;

  result = bobgui_property_lookup_list_model_new (BOBGUI_TYPE_WIDGET, "parent");
  if (fill)
    {
      BobguiWidget *widget = create_widget_tree ();
      bobgui_property_lookup_list_model_set_object (result, widget);
    }
  changes = g_string_new ("");
  g_object_set_qdata_full (G_OBJECT(result), changes_quark, changes, free_changes);
  g_signal_connect (result, "items-changed", G_CALLBACK (items_changed), changes);
  g_signal_connect (result, "notify::n-items", G_CALLBACK (notify_n_items), changes);

  return result;
}

static void
test_create_empty (void)
{
  BobguiPropertyLookupListModel *model;
  GType type;
  guint n_items;
  char *property;
  GObject *object;

  model = new_model (FALSE);
  assert_model (model, "");
  assert_changes (model, "");

  g_assert_true (g_list_model_get_item_type (G_LIST_MODEL (model)) == BOBGUI_TYPE_WIDGET);
  g_object_get (model,
                "item-type", &type,
                "object", &object,
                "n-items", &n_items,
                "property", &property,
                NULL);
  g_assert_true (type == BOBGUI_TYPE_WIDGET);
  g_assert_null (object);
  g_assert_true (n_items == 0);
  g_assert_cmpstr (property, ==, "parent");

  g_object_unref (model);
}

static void
test_create (void)
{
  BobguiPropertyLookupListModel *model;

  model = new_model (TRUE);
  assert_model (model, "BobguiLabel BobguiGrid BobguiBox BobguiWindow");
  assert_changes (model, "");

  g_object_unref (model);
  destroy_widgets ();
}

static void
test_set_object (void)
{
  BobguiPropertyLookupListModel *model;
  BobguiWidget *widget;

  widget = create_widget_tree ();

  model = new_model (FALSE);
  bobgui_property_lookup_list_model_set_object (model, widget);
  g_assert_true (bobgui_property_lookup_list_model_get_object (model) == widget);

  assert_model (model, "BobguiLabel BobguiGrid BobguiBox BobguiWindow");
  assert_changes (model, "+0*");
  g_object_unref (model);

  model = new_model (FALSE);
  assert_model (model, "");
  bobgui_property_lookup_list_model_set_object (model, widget);
  assert_model (model, "BobguiLabel BobguiGrid BobguiBox BobguiWindow");
  assert_changes (model, "0+4*");

  g_object_set (model, "object", NULL, NULL);
  assert_model (model, "");
  assert_changes (model, "0-4*");

  g_object_unref (model);
  destroy_widgets ();
}

static void
test_change_property (void)
{
  BobguiPropertyLookupListModel *model;
  BobguiWidget *widget, *parent, *grandparent;

  widget = create_widget_tree ();
  parent = bobgui_widget_get_parent (widget);
  grandparent = bobgui_widget_get_parent (parent);

  model = new_model (FALSE);
  assert_model (model, ""); /* make sure the model has a definite size */
  bobgui_property_lookup_list_model_set_object (model, widget);
  assert_model (model, "BobguiLabel BobguiGrid BobguiBox BobguiWindow");
  assert_changes (model, "0+4*");

  bobgui_grid_remove (BOBGUI_GRID (parent), widget);
  assert_model (model, "BobguiLabel");
  assert_changes (model, "1-3*");

  bobgui_box_append (BOBGUI_BOX (grandparent), widget);
  assert_model (model, "BobguiLabel BobguiBox BobguiWindow");
  assert_changes (model, "1+2*");

  g_object_unref (model);
  destroy_widgets ();
}

int
main (int argc, char *argv[])
{
  bobgui_test_init (&argc, &argv);

  changes_quark = g_quark_from_static_string ("What did I see? Can I believe what I saw?");

  g_test_add_func ("/propertylookuplistmodel/create_empty", test_create_empty);
  g_test_add_func ("/propertylookuplistmodel/create", test_create);
  g_test_add_func ("/propertylookuplistmodel/set-object", test_set_object);
  g_test_add_func ("/propertylookuplistmodel/change-property", test_change_property);

  return g_test_run ();
}
