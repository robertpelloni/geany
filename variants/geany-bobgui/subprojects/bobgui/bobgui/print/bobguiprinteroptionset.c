/* BOBGUI - The Bobgui Framework
 * bobguiprintbackendprivate.h: Abstract printer backend interfaces
 * Copyright (C) 2006, Red Hat, Inc.
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
#include <string.h>
#include <glib.h>

#include "bobguiprinteroptionsetprivate.h"

/*****************************************
 *         BobguiPrinterOptionSet    *
 *****************************************/

enum {
  CHANGED,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

/* ugly side-effect of aliasing */
#undef bobgui_printer_option_set

G_DEFINE_TYPE (BobguiPrinterOptionSet, bobgui_printer_option_set, G_TYPE_OBJECT)

static void
bobgui_printer_option_set_finalize (GObject *object)
{
  BobguiPrinterOptionSet *set = BOBGUI_PRINTER_OPTION_SET (object);

  g_hash_table_destroy (set->hash);
  g_ptr_array_foreach (set->array, (GFunc)g_object_unref, NULL);
  g_ptr_array_free (set->array, TRUE);
  
  G_OBJECT_CLASS (bobgui_printer_option_set_parent_class)->finalize (object);
}

static void
bobgui_printer_option_set_init (BobguiPrinterOptionSet *set)
{
  set->array = g_ptr_array_new ();
  set->hash = g_hash_table_new (g_str_hash, g_str_equal);
}

static void
bobgui_printer_option_set_class_init (BobguiPrinterOptionSetClass *class)
{
  GObjectClass *gobject_class = (GObjectClass *)class;

  gobject_class->finalize = bobgui_printer_option_set_finalize;

  signals[CHANGED] =
    g_signal_new ("changed",
		  G_TYPE_FROM_CLASS (gobject_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BobguiPrinterOptionSetClass, changed),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 0);
}


static void
emit_changed (BobguiPrinterOptionSet *set)
{
  g_signal_emit (set, signals[CHANGED], 0);
}

BobguiPrinterOptionSet *
bobgui_printer_option_set_new (void)
{
  return g_object_new (BOBGUI_TYPE_PRINTER_OPTION_SET, NULL);
}

void
bobgui_printer_option_set_remove (BobguiPrinterOptionSet *set,
			       BobguiPrinterOption    *option)
{
  int i;
  
  for (i = 0; i < set->array->len; i++)
    {
      if (g_ptr_array_index (set->array, i) == option)
	{
	  g_ptr_array_remove_index (set->array, i);
	  g_hash_table_remove (set->hash, option->name);
	  g_signal_handlers_disconnect_by_func (option, emit_changed, set);

	  g_object_unref (option);
	  break;
	}
    }
}

void
bobgui_printer_option_set_add (BobguiPrinterOptionSet *set,
			    BobguiPrinterOption    *option)
{
  g_object_ref (option);
  
  if (bobgui_printer_option_set_lookup (set, option->name))
    bobgui_printer_option_set_remove (set, option);
    
  g_ptr_array_add (set->array, option);
  g_hash_table_insert (set->hash, option->name, option);
  g_signal_connect_object (option, "changed", G_CALLBACK (emit_changed), set, G_CONNECT_SWAPPED);
}

BobguiPrinterOption *
bobgui_printer_option_set_lookup (BobguiPrinterOptionSet *set,
			       const char          *name)
{
  gpointer ptr;

  ptr = g_hash_table_lookup (set->hash, name);

  return BOBGUI_PRINTER_OPTION (ptr);
}

void
bobgui_printer_option_set_clear_conflicts (BobguiPrinterOptionSet *set)
{
  bobgui_printer_option_set_foreach (set,
				  (BobguiPrinterOptionSetFunc)bobgui_printer_option_clear_has_conflict,
				  NULL);
}

/**
 * bobgui_printer_option_set_get_groups:
 * @set: a `BobguiPrinterOptionSet`
 *
 * Gets the groups in this set.
 *
 * Returns: (element-type utf8) (transfer full): a list of group names.
 */
GList *
bobgui_printer_option_set_get_groups (BobguiPrinterOptionSet *set)
{
  BobguiPrinterOption *option;
  GList *list = NULL;
  int i;

  for (i = 0; i < set->array->len; i++)
    {
      option = g_ptr_array_index (set->array, i);

      if (g_list_find_custom (list, option->group, (GCompareFunc)g_strcmp0) == NULL)
	list = g_list_prepend (list, g_strdup (option->group));
    }

  return g_list_reverse (list);
}

void
bobgui_printer_option_set_foreach_in_group (BobguiPrinterOptionSet     *set,
					 const char              *group,
					 BobguiPrinterOptionSetFunc  func,
					 gpointer                 user_data)
{
  BobguiPrinterOption *option;
  int i;

  for (i = 0; i < set->array->len; i++)
    {
      option = g_ptr_array_index (set->array, i);

      if (group == NULL || g_strcmp0 (group, option->group) == 0)
	func (option, user_data);
    }
}

void
bobgui_printer_option_set_foreach (BobguiPrinterOptionSet *set,
				BobguiPrinterOptionSetFunc func,
				gpointer user_data)
{
  bobgui_printer_option_set_foreach_in_group (set, NULL, func, user_data);
}
