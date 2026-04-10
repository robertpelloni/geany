/*
 * Copyright (C) 2005 Novell, Inc.
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
 *
 * Author: Anders Carlsson <andersca@imendio.com>
 *
 * Based on nautilus-query.c
 */

#include "config.h"
#include <string.h>

#include "bobguiquery.h"

struct _BobguiQueryPrivate
{
  char *text;
  GFile *location;
  GList *mime_types;
  char **words;
};

G_DEFINE_TYPE_WITH_PRIVATE (BobguiQuery, bobgui_query, G_TYPE_OBJECT)

static void
finalize (GObject *object)
{
  BobguiQuery *query = BOBGUI_QUERY (object);
  BobguiQueryPrivate *priv = bobgui_query_get_instance_private (query);

  g_clear_object (&priv->location);
  g_free (priv->text);
  g_strfreev (priv->words);

  G_OBJECT_CLASS (bobgui_query_parent_class)->finalize (object);
}

static void
bobgui_query_class_init (BobguiQueryClass *class)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (class);
  gobject_class->finalize = finalize;
}

static void
bobgui_query_init (BobguiQuery *query)
{
}

BobguiQuery *
bobgui_query_new (void)
{
  return g_object_new (BOBGUI_TYPE_QUERY,  NULL);
}


const char *
bobgui_query_get_text (BobguiQuery *query)
{
  BobguiQueryPrivate *priv = bobgui_query_get_instance_private (query);

  return priv->text;
}

void
bobgui_query_set_text (BobguiQuery    *query,
                    const char *text)
{
  BobguiQueryPrivate *priv = bobgui_query_get_instance_private (query);

  g_free (priv->text);
  priv->text = g_strdup (text);

  g_strfreev (priv->words);
  priv->words = NULL;
}

GFile *
bobgui_query_get_location (BobguiQuery *query)
{
  BobguiQueryPrivate *priv = bobgui_query_get_instance_private (query);

  return priv->location;
}

void
bobgui_query_set_location (BobguiQuery *query,
                        GFile    *file)
{
  BobguiQueryPrivate *priv = bobgui_query_get_instance_private (query);

  g_set_object (&priv->location, file);
}

static char *
prepare_string_for_compare (const char *string)
{
  char *normalized, *res;

  normalized = g_utf8_normalize (string, -1, G_NORMALIZE_NFD);
  res = g_utf8_strdown (normalized, -1);
  g_free (normalized);

  return res;
}

gboolean
bobgui_query_matches_string (BobguiQuery    *query,
                          const char *string)
{
  BobguiQueryPrivate *priv = bobgui_query_get_instance_private (query);
  char *prepared;
  gboolean found;
  int i;

  if (!priv->text)
    return FALSE;

  if (!priv->words)
    {
      prepared = prepare_string_for_compare (priv->text);
      priv->words = g_strsplit (prepared, " ", -1);
      g_free (prepared);
    }

  prepared = prepare_string_for_compare (string);

  found = TRUE;
  for (i = 0; priv->words[i]; i++)
    {
      if (strstr (prepared, priv->words[i]) == NULL)
        {
          found = FALSE;
          break;
        }
    }

  g_free (prepared);

  return found;
}
