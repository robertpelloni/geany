/*
 * Copyright © 2014 Benjamin Otte <otte@gnome.org>
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

#include "bobguicssnodedeclarationprivate.h"

#include "bobguiprivate.h"

#include <string.h>

struct _BobguiCssNodeDeclaration {
  guint refcount;
  GQuark name;
  GQuark id;
  BobguiStateFlags state;
  guint n_classes;
  GQuark classes[0];
};

static inline gsize
sizeof_node (guint n_classes)
{
  return sizeof (BobguiCssNodeDeclaration)
       + sizeof (GQuark) * n_classes;
}

static inline gsize
sizeof_this_node (BobguiCssNodeDeclaration *decl)
{
  return sizeof_node (decl->n_classes);
}

static void
bobgui_css_node_declaration_make_writable (BobguiCssNodeDeclaration **decl)
{
  if ((*decl)->refcount == 1)
    return;

  (*decl)->refcount--;

  *decl = g_memdup2 (*decl, sizeof_this_node (*decl));
  (*decl)->refcount = 1;
}

static void
bobgui_css_node_declaration_make_writable_resize (BobguiCssNodeDeclaration **decl,
                                               gsize                   offset,
                                               gsize                   bytes_added,
                                               gsize                   bytes_removed)
{
  gsize old_size = sizeof_this_node (*decl);
  gsize new_size = old_size + bytes_added - bytes_removed;

  if ((*decl)->refcount == 1)
    {
      if (bytes_removed > 0 && old_size - offset - bytes_removed > 0)
        memmove (((char *) *decl) + offset, ((char *) *decl) + offset + bytes_removed, old_size - offset - bytes_removed);
      *decl = g_realloc (*decl, new_size);
      if (bytes_added > 0 && old_size - offset > 0)
        memmove (((char *) *decl) + offset + bytes_added, ((char *) *decl) + offset, old_size - offset);
    }
  else
    {
      BobguiCssNodeDeclaration *old = *decl;

      old->refcount--;
  
      *decl = g_malloc (new_size);
      memcpy (*decl, old, offset);
      if (old_size - offset - bytes_removed > 0)
        memcpy (((char *) *decl) + offset + bytes_added, ((char *) old) + offset + bytes_removed, old_size - offset - bytes_removed);
      (*decl)->refcount = 1;
    }
}

BobguiCssNodeDeclaration *
bobgui_css_node_declaration_new (void)
{
  static BobguiCssNodeDeclaration empty = {
    1,
    0,
    0,
    0,
    0
  };

  return g_memdup2 (&empty, sizeof_this_node (&empty));
}

BobguiCssNodeDeclaration *
bobgui_css_node_declaration_ref (BobguiCssNodeDeclaration *decl)
{
  decl->refcount++;

  return decl;
}

void
bobgui_css_node_declaration_unref (BobguiCssNodeDeclaration *decl)
{
  decl->refcount--;
  if (decl->refcount > 0)
    return;

  g_free (decl);
}

gboolean
bobgui_css_node_declaration_set_name (BobguiCssNodeDeclaration **decl,
                                   GQuark                  name)
{
  if ((*decl)->name == name)
    return FALSE;

  bobgui_css_node_declaration_make_writable (decl);
  (*decl)->name = name;

  return TRUE;
}

GQuark
bobgui_css_node_declaration_get_name (const BobguiCssNodeDeclaration *decl)
{
  return decl->name;
}

gboolean
bobgui_css_node_declaration_set_id (BobguiCssNodeDeclaration **decl,
                                 GQuark                  id)
{
  if ((*decl)->id == id)
    return FALSE;

  bobgui_css_node_declaration_make_writable (decl);
  (*decl)->id = id;

  return TRUE;
}

GQuark
bobgui_css_node_declaration_get_id (const BobguiCssNodeDeclaration *decl)
{
  return decl->id;
}

gboolean
bobgui_css_node_declaration_set_state (BobguiCssNodeDeclaration **decl,
                                    BobguiStateFlags           state)
{
  if ((*decl)->state == state)
    return FALSE;
  
  bobgui_css_node_declaration_make_writable (decl);
  (*decl)->state = state;

  return TRUE;
}

BobguiStateFlags
bobgui_css_node_declaration_get_state (const BobguiCssNodeDeclaration *decl)
{
  return decl->state;
}

static gboolean
find_class (const BobguiCssNodeDeclaration *decl,
            GQuark                       class_quark,
            guint                       *position)
{
  int min, max, mid;
  gboolean found = FALSE;
  guint pos;

  *position = 0;

  if (decl->n_classes == 0)
    return FALSE;

  min = 0;
  max = decl->n_classes - 1;

  do
    {
      GQuark item;

      mid = (min + max) / 2;
      item = decl->classes[mid];

      if (class_quark == item)
        {
          found = TRUE;
          pos = mid;
          break;
        }
      else if (class_quark > item)
        min = pos = mid + 1;
      else
        {
          max = mid - 1;
          pos = mid;
        }
    }
  while (min <= max);

  *position = pos;

  return found;
}

gboolean
bobgui_css_node_declaration_add_class (BobguiCssNodeDeclaration **decl,
                                    GQuark                  class_quark)
{
  guint pos;

  if (find_class (*decl, class_quark, &pos))
    return FALSE;

  bobgui_css_node_declaration_make_writable_resize (decl,
                                                 (char *) &(*decl)->classes[pos] - (char *) *decl,
                                                 sizeof (GQuark),
                                                 0);
  (*decl)->n_classes++;
  (*decl)->classes[pos] = class_quark;

  return TRUE;
}

gboolean
bobgui_css_node_declaration_remove_class (BobguiCssNodeDeclaration **decl,
                                       GQuark                  class_quark)
{
  guint pos;

  if (!find_class (*decl, class_quark, &pos))
    return FALSE;

  bobgui_css_node_declaration_make_writable_resize (decl,
                                                 (char *) &(*decl)->classes[pos] - (char *) *decl,
                                                 0,
                                                 sizeof (GQuark));
  (*decl)->n_classes--;

  return TRUE;
}

gboolean
bobgui_css_node_declaration_clear_classes (BobguiCssNodeDeclaration **decl)
{
  if ((*decl)->n_classes == 0)
    return FALSE;

  bobgui_css_node_declaration_make_writable_resize (decl,
                                                 (char *) (*decl)->classes - (char *) *decl,
                                                 0,
                                                 sizeof (GQuark) * (*decl)->n_classes);
  (*decl)->n_classes = 0;

  return TRUE;
}

gboolean
bobgui_css_node_declaration_has_class (const BobguiCssNodeDeclaration *decl,
                                    GQuark                       class_quark)
{
  guint pos;

  switch (decl->n_classes)
    {
    case 3:
      if (decl->classes[2] == class_quark)
        return TRUE;
      G_GNUC_FALLTHROUGH;

    case 2:
      if (decl->classes[1] == class_quark)
        return TRUE;
      G_GNUC_FALLTHROUGH;

    case 1:
      if (decl->classes[0] == class_quark)
        return TRUE;
      G_GNUC_FALLTHROUGH;

    case 0:
      return FALSE;

    default:
      return find_class (decl, class_quark, &pos);
    }
}

const GQuark *
bobgui_css_node_declaration_get_classes (const BobguiCssNodeDeclaration *decl,
                                      guint                       *n_classes)
{
  *n_classes = decl->n_classes;

  return decl->classes;
}

void
bobgui_css_node_declaration_add_bloom_hashes (const BobguiCssNodeDeclaration *decl,
                                           BobguiCountingBloomFilter      *filter)
{
  guint i;

  if (decl->name)
    bobgui_counting_bloom_filter_add (filter, bobgui_css_hash_name (decl->name));
  if (decl->id)
    bobgui_counting_bloom_filter_add (filter, bobgui_css_hash_id (decl->id));

  for (i = 0; i < decl->n_classes; i++)
    {
      bobgui_counting_bloom_filter_add (filter, bobgui_css_hash_class (decl->classes[i]));
    }
}

void
bobgui_css_node_declaration_remove_bloom_hashes (const BobguiCssNodeDeclaration *decl,
                                              BobguiCountingBloomFilter      *filter)
{
  guint i;

  if (decl->name)
    bobgui_counting_bloom_filter_remove (filter, bobgui_css_hash_name (decl->name));
  if (decl->id)
    bobgui_counting_bloom_filter_remove (filter, bobgui_css_hash_id (decl->id));

  for (i = 0; i < decl->n_classes; i++)
    {
      bobgui_counting_bloom_filter_remove (filter, bobgui_css_hash_class (decl->classes[i]));
    }
}

guint
bobgui_css_node_declaration_hash (gconstpointer elem)
{
  const BobguiCssNodeDeclaration *decl = elem;
  guint hash, i;
  
  hash = GPOINTER_TO_UINT (decl->name);
  hash <<= 5;
  hash ^= GPOINTER_TO_UINT (decl->id);

  for (i = 0; i < decl->n_classes; i++)
    {
      hash <<= 5;
      hash += decl->classes[i];
    }

  hash ^= decl->state;

  return hash;
}

gboolean
bobgui_css_node_declaration_equal (gconstpointer elem1,
                                gconstpointer elem2)
{
  const BobguiCssNodeDeclaration *decl1 = elem1;
  const BobguiCssNodeDeclaration *decl2 = elem2;
  guint i;

  if (decl1 == decl2)
    return TRUE;

  if (decl1->name != decl2->name)
    return FALSE;

  if (decl1->state != decl2->state)
    return FALSE;

  if (decl1->id != decl2->id)
    return FALSE;

  if (decl1->n_classes != decl2->n_classes)
    return FALSE;

  for (i = 0; i < decl1->n_classes; i++)
    {
      if (decl1->classes[i] != decl2->classes[i])
        return FALSE;
    }

  return TRUE;
}

static int
cmpstr (gconstpointer a,
        gconstpointer b,
        gpointer      data)
{
  char **ap = (char **) a;
  char **bp = (char **) b;

  return g_ascii_strcasecmp (*ap, *bp);
}

/* Append the declaration to the string, in selector format */
void
bobgui_css_node_declaration_print (const BobguiCssNodeDeclaration *decl,
                                GString                     *string)
{
  guint i;
  char **classnames;

  if (decl->name)
    g_string_append (string, g_quark_to_string (decl->name));
  else
    g_string_append (string, "*");

  if (decl->id)
    {
      g_string_append_c (string, '#');
      g_string_append (string, g_quark_to_string (decl->id));
    }

  classnames = g_new (char *, decl->n_classes);
  for (i = 0; i < decl->n_classes; i++)
    classnames[i] = (char *)g_quark_to_string (decl->classes[i]);

  g_sort_array (classnames, decl->n_classes, sizeof (char *), cmpstr, NULL);

  for (i = 0; i < decl->n_classes; i++)
    {
      g_string_append_c (string, '.');
      g_string_append (string, classnames[i]);
    }
  g_free (classnames);

  for (i = 0; i < sizeof (BobguiStateFlags) * 8; i++)
    {
      if (decl->state & (1u << i))
        {
          const char *name = bobgui_css_pseudoclass_name (1u << i);
          g_assert (name);
          g_string_append_c (string, ':');
          g_string_append (string, name);
        }
    }
}

char *
bobgui_css_node_declaration_to_string (const BobguiCssNodeDeclaration *decl)
{
  GString *s = g_string_new (NULL);

  bobgui_css_node_declaration_print (decl, s);

  return g_string_free (s, FALSE);
}
