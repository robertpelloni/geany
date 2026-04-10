/*
 * Copyright (C) 2006 Alexander Larsson  <alexl@redhat.com>
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
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include <glib/gi18n-lib.h>

#include "bobguistringpairprivate.h"


struct _BobguiStringPair {
  GObject parent_instance;
  char *id;
  char *string;
};

enum {
  PROP_ID = 1,
  PROP_STRING,
  PROP_NUM_PROPERTIES
};

G_DEFINE_TYPE (BobguiStringPair, bobgui_string_pair, G_TYPE_OBJECT);

static void
bobgui_string_pair_init (BobguiStringPair *pair)
{
}

static void
bobgui_string_pair_finalize (GObject *object)
{
  BobguiStringPair *pair = BOBGUI_STRING_PAIR (object);

  g_free (pair->id);
  g_free (pair->string);

  G_OBJECT_CLASS (bobgui_string_pair_parent_class)->finalize (object);
}

static void
bobgui_string_pair_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BobguiStringPair *pair = BOBGUI_STRING_PAIR (object);

  switch (property_id)
    {
    case PROP_STRING:
      g_free (pair->string);
      pair->string = g_value_dup_string (value);
      break;

    case PROP_ID:
      g_free (pair->id);
      pair->id = g_value_dup_string (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
bobgui_string_pair_get_property (GObject      *object,
                              guint         property_id,
                              GValue       *value,
                              GParamSpec   *pspec)
{
  BobguiStringPair *pair = BOBGUI_STRING_PAIR (object);

  switch (property_id)
    {
    case PROP_STRING:
      g_value_set_string (value, pair->string);
      break;

    case PROP_ID:
      g_value_set_string (value, pair->id);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
bobgui_string_pair_class_init (BobguiStringPairClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  GParamSpec *pspec;

  object_class->finalize = bobgui_string_pair_finalize;
  object_class->set_property = bobgui_string_pair_set_property;
  object_class->get_property = bobgui_string_pair_get_property;

  pspec = g_param_spec_string ("string", NULL, NULL,
                               NULL,
                               G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_property (object_class, PROP_STRING, pspec);

  pspec = g_param_spec_string ("id", NULL, NULL,
                               NULL,
                               G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_property (object_class, PROP_ID, pspec);
}

BobguiStringPair *
bobgui_string_pair_new (const char *id,
                     const char *string)
{
  return g_object_new (BOBGUI_TYPE_STRING_PAIR,
                       "id", id,
                       "string", string,
                       NULL);
}

const char *
bobgui_string_pair_get_string (BobguiStringPair *pair)
{
  return pair->string;
}

const char *
bobgui_string_pair_get_id (BobguiStringPair *pair)
{
  return pair->id;
}
