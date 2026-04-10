/* BOBGUI - The Bobgui Framework
 * bobguifontfilter.c:
 * Copyright (C) 2024 Niels De Graef <nielsdegraef@gmail.com>
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
#include <glib/gprintf.h>
#include <string.h>

#include "bobguifontfilterprivate.h"
#include "bobguiprivate.h"

struct _BobguiFontFilter
{
  BobguiFilter parent_instance;

  PangoContext *pango_context;

  gboolean monospace;
  PangoLanguage *language;
};

enum {
  PROP_0,
  PROP_PANGO_CONTEXT,
  PROP_MONOSPACE,
  PROP_LANGUAGE,
  N_PROPS
};
static GParamSpec *properties[N_PROPS] = { NULL, };

G_DEFINE_TYPE (BobguiFontFilter, bobgui_font_filter, BOBGUI_TYPE_FILTER)

static void
bobgui_font_filter_set_property (GObject         *object,
                              guint            prop_id,
                              const GValue    *value,
                              GParamSpec      *pspec)
{
  BobguiFontFilter *self = BOBGUI_FONT_FILTER (object);

  switch (prop_id)
    {
    case PROP_PANGO_CONTEXT:
      _bobgui_font_filter_set_pango_context (self, g_value_get_object (value));
      break;
    case PROP_MONOSPACE:
      _bobgui_font_filter_set_monospace (self, g_value_get_boolean (value));
      break;
    case PROP_LANGUAGE:
      _bobgui_font_filter_set_language (self, g_value_get_boxed (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_font_filter_get_property (GObject         *object,
                              guint            prop_id,
                              GValue          *value,
                              GParamSpec      *pspec)
{
  BobguiFontFilter *self = BOBGUI_FONT_FILTER (object);

  switch (prop_id)
    {
    case PROP_PANGO_CONTEXT:
      g_value_set_object (value, self->pango_context);
      break;
    case PROP_MONOSPACE:
      g_value_set_boolean (value, _bobgui_font_filter_get_monospace (self));
      break;
    case PROP_LANGUAGE:
      g_value_set_boxed (value, _bobgui_font_filter_get_language (self));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static gboolean
bobgui_font_filter_match (BobguiFilter *filter,
                       gpointer   item)
{
  BobguiFontFilter *self = BOBGUI_FONT_FILTER (filter);
  PangoFontFamily *family;
  PangoFontFace *face;

  if (PANGO_IS_FONT_FAMILY (item))
    {
      family = item;
      face = pango_font_family_get_face (family, NULL);
    }
  else
    {
      face = PANGO_FONT_FACE (item);
      family = pango_font_face_get_family (face);
    }

  if (self->monospace &&
      !pango_font_family_is_monospace (family))
    return FALSE;

  if (self->language)
    {
      PangoFontDescription *desc;
      PangoFont *font;
      PangoLanguage **langs;
      gboolean ret = FALSE;

      desc = pango_font_face_describe (face);
      pango_font_description_set_size (desc, 20);

      font = pango_context_load_font (self->pango_context, desc);

      langs = pango_font_get_languages (font);
      if (langs)
        {
          for (int i = 0; langs[i]; i++)
            {
              if (langs[i] == self->language)
                {
                  ret = TRUE;
                  break;
                }
            }
        }

      g_object_unref (font);
      pango_font_description_free (desc);

      return ret;
    }

  return TRUE;
}

static BobguiFilterMatch
bobgui_font_filter_get_strictness (BobguiFilter *filter)
{
  BobguiFontFilter *self = BOBGUI_FONT_FILTER (filter);

  if (!self->monospace && self->language == NULL)
    return BOBGUI_FILTER_MATCH_ALL;

  return BOBGUI_FILTER_MATCH_SOME;
}

static void
bobgui_font_filter_class_init (BobguiFontFilterClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  BobguiFilterClass *filter_class = BOBGUI_FILTER_CLASS (klass);

  filter_class->match = bobgui_font_filter_match;
  filter_class->get_strictness = bobgui_font_filter_get_strictness;

  gobject_class->set_property = bobgui_font_filter_set_property;
  gobject_class->get_property = bobgui_font_filter_get_property;

  properties[PROP_PANGO_CONTEXT] =
    g_param_spec_object ("pango-context", NULL, NULL,
                         PANGO_TYPE_CONTEXT,
                         BOBGUI_PARAM_READWRITE);
  properties[PROP_MONOSPACE] =
    g_param_spec_boolean ("monospace", NULL, NULL,
                          FALSE,
                          BOBGUI_PARAM_READWRITE);
  properties[PROP_LANGUAGE] =
    g_param_spec_boxed ("language", NULL, NULL,
                        PANGO_TYPE_LANGUAGE,
                        BOBGUI_PARAM_READWRITE);

  g_object_class_install_properties (gobject_class, N_PROPS, properties);
}

static void
bobgui_font_filter_init (BobguiFontFilter *self)
{
}

void
_bobgui_font_filter_set_pango_context (BobguiFontFilter *self,
                                    PangoContext  *context)
{
  g_return_if_fail (BOBGUI_IS_FONT_FILTER (self));
  g_return_if_fail (PANGO_IS_CONTEXT (context));

  if (self->pango_context == context)
    return;

  self->pango_context = context;
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_PANGO_CONTEXT]);
}

gboolean
_bobgui_font_filter_get_monospace (BobguiFontFilter *self)
{
  g_return_val_if_fail (BOBGUI_IS_FONT_FILTER (self), FALSE);

  return self->monospace;
}

void
_bobgui_font_filter_set_monospace (BobguiFontFilter *self,
                                gboolean       monospace)
{
  g_return_if_fail (BOBGUI_IS_FONT_FILTER (self));

  if (self->monospace == monospace)
    return;

  self->monospace = monospace;
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_MONOSPACE]);
  bobgui_filter_changed (BOBGUI_FILTER (self),
                      monospace ? BOBGUI_FILTER_CHANGE_MORE_STRICT
                                : BOBGUI_FILTER_CHANGE_LESS_STRICT);
}

PangoLanguage *
_bobgui_font_filter_get_language (BobguiFontFilter *self)
{
  g_return_val_if_fail (BOBGUI_IS_FONT_FILTER (self), NULL);

  return self->language;
}

void
_bobgui_font_filter_set_language (BobguiFontFilter *self,
                               PangoLanguage *lang)
{
  BobguiFilterChange filter_change = BOBGUI_FILTER_CHANGE_DIFFERENT;

  g_return_if_fail (BOBGUI_IS_FONT_FILTER (self));

  if (self->language == lang)
    return;

  if (lang == NULL || self->language == NULL)
    {
      filter_change = (lang != NULL) ? BOBGUI_FILTER_CHANGE_MORE_STRICT
                                     : BOBGUI_FILTER_CHANGE_LESS_STRICT;
    }

  self->language = lang;
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_LANGUAGE]);
  bobgui_filter_changed (BOBGUI_FILTER (self), filter_change);
}

BobguiFilter *
_bobgui_font_filter_new (void)
{
  return g_object_new (BOBGUI_TYPE_FONT_FILTER, NULL);
}
