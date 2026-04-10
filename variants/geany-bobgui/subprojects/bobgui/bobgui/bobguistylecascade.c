/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2012 Benjamin Otte <otte@gnome.org>
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

#include "bobguistylecascadeprivate.h"

#include "bobguistyleprovider.h"
#include "bobguistyleproviderprivate.h"
#include "bobguiprivate.h"

typedef struct _BobguiStyleCascadeIter BobguiStyleCascadeIter;
typedef struct _BobguiStyleProviderData BobguiStyleProviderData;

struct _BobguiStyleCascadeIter {
  int  n_cascades;
  int *cascade_index;  /* each one points at last index that was returned, */
                       /* not next one that should be returned */
  int index_[20];
};

struct _BobguiStyleProviderData
{
  BobguiStyleProvider *provider;
  guint priority;
  guint changed_signal_id;
};

static BobguiStyleProvider *
bobgui_style_cascade_iter_next (BobguiStyleCascade     *cascade,
                             BobguiStyleCascadeIter *iter)
{
  BobguiStyleCascade *cas;
  int ix, highest_priority_index = 0;
  BobguiStyleProviderData *highest_priority_data = NULL;

  for (cas = cascade, ix = 0; ix < iter->n_cascades; cas = cas->parent, ix++)
    {
      BobguiStyleProviderData *data;

      if (iter->cascade_index[ix] <= 0)
        continue;

      data = &g_array_index (cas->providers,
                             BobguiStyleProviderData,
                             iter->cascade_index[ix] - 1);
      if (highest_priority_data == NULL || data->priority > highest_priority_data->priority)
        {
          highest_priority_index = ix;
          highest_priority_data = data;
        }
    }

  if (highest_priority_data != NULL)
    {
      iter->cascade_index[highest_priority_index]--;
      return highest_priority_data->provider;
    }
  return NULL;
}

static BobguiStyleProvider *
bobgui_style_cascade_iter_init (BobguiStyleCascade     *cascade,
                             BobguiStyleCascadeIter *iter)
{
  BobguiStyleCascade *cas = cascade;
  int ix;

  iter->n_cascades = 1;
  while ((cas = cas->parent) != NULL)
    iter->n_cascades++;

  if (iter->n_cascades < 20)
    iter->cascade_index = iter->index_;
  else
    iter->cascade_index = g_new (int, iter->n_cascades);

  for (cas = cascade, ix = 0; ix < iter->n_cascades; cas = cas->parent, ix++)
    iter->cascade_index[ix] = cas->providers->len;

  return bobgui_style_cascade_iter_next (cascade, iter);
}

static void
bobgui_style_cascade_iter_clear (BobguiStyleCascadeIter *iter)
{
  if (iter->cascade_index != iter->index_)
    g_free (iter->cascade_index);
}

static BobguiSettings *
bobgui_style_cascade_get_settings (BobguiStyleProvider *provider)
{
  BobguiStyleCascade *cascade = BOBGUI_STYLE_CASCADE (provider);
  BobguiStyleCascadeIter iter;
  BobguiSettings *settings;
  BobguiStyleProvider *item;

  for (item = bobgui_style_cascade_iter_init (cascade, &iter);
       item;
       item = bobgui_style_cascade_iter_next (cascade, &iter))
    {
      settings = bobgui_style_provider_get_settings (item);
      if (settings)
        {
          bobgui_style_cascade_iter_clear (&iter);
          return settings;
        }
    }

  bobgui_style_cascade_iter_clear (&iter);
  return NULL;
}

static BobguiCssValue *
bobgui_style_cascade_get_color (BobguiStyleProvider *provider,
                             const char              *name)
{
  BobguiStyleCascade *cascade = BOBGUI_STYLE_CASCADE (provider);
  BobguiStyleCascadeIter iter;
  BobguiCssValue *color;
  BobguiStyleProvider *item;

  for (item = bobgui_style_cascade_iter_init (cascade, &iter);
       item;
       item = bobgui_style_cascade_iter_next (cascade, &iter))
    {
      color = bobgui_style_provider_get_color (item, name);
      if (color)
        {
          bobgui_style_cascade_iter_clear (&iter);
          return color;
        }
    }

  bobgui_style_cascade_iter_clear (&iter);
  return NULL;
}

static int
bobgui_style_cascade_get_scale (BobguiStyleProvider *provider)
{
  BobguiStyleCascade *cascade = BOBGUI_STYLE_CASCADE (provider);

  return cascade->scale;
}

static BobguiCssKeyframes *
bobgui_style_cascade_get_keyframes (BobguiStyleProvider *provider,
                                 const char       *name)
{
  BobguiStyleCascade *cascade = BOBGUI_STYLE_CASCADE (provider);
  BobguiStyleCascadeIter iter;
  BobguiCssKeyframes *keyframes;
  BobguiStyleProvider *item;

  for (item = bobgui_style_cascade_iter_init (cascade, &iter);
       item;
       item = bobgui_style_cascade_iter_next (cascade, &iter))
    {
      keyframes = bobgui_style_provider_get_keyframes (item, name);
      if (keyframes)
        {
          bobgui_style_cascade_iter_clear (&iter);
          return keyframes;
        }
    }

  bobgui_style_cascade_iter_clear (&iter);
  return NULL;
}

static void
bobgui_style_cascade_lookup (BobguiStyleProvider             *provider,
                          const BobguiCountingBloomFilter *filter,
                          BobguiCssNode                   *node,
                          BobguiCssLookup                 *lookup,
                          BobguiCssChange                 *change)
{
  BobguiStyleCascade *cascade = BOBGUI_STYLE_CASCADE (provider);
  BobguiStyleCascadeIter iter;
  BobguiStyleProvider *item;
  BobguiCssChange iter_change;

  for (item = bobgui_style_cascade_iter_init (cascade, &iter);
       item;
       item = bobgui_style_cascade_iter_next (cascade, &iter))
    {
      bobgui_style_provider_lookup (item, filter, node, lookup,
                                 change ? &iter_change : NULL);
      if (change)
        *change |= iter_change;
    }
  bobgui_style_cascade_iter_clear (&iter);
}

static void
bobgui_style_cascade_emit_error (BobguiStyleProvider *provider,
                              BobguiCssSection    *section,
                              const GError     *error)
{
  BobguiStyleCascade *cascade = BOBGUI_STYLE_CASCADE (provider);
  BobguiStyleCascadeIter iter;
  BobguiStyleProvider *item;

  for (item = bobgui_style_cascade_iter_init (cascade, &iter);
       item;
       item = bobgui_style_cascade_iter_next (cascade, &iter))
    {
      if (bobgui_style_provider_has_section (item, section))
        {
          bobgui_style_provider_emit_error (item, section, (GError *) error);
          break;
        }
    }

  bobgui_style_cascade_iter_clear (&iter);
}

static void
bobgui_style_cascade_provider_iface_init (BobguiStyleProviderInterface *iface)
{
  iface->get_color = bobgui_style_cascade_get_color;
  iface->get_settings = bobgui_style_cascade_get_settings;
  iface->get_scale = bobgui_style_cascade_get_scale;
  iface->get_keyframes = bobgui_style_cascade_get_keyframes;
  iface->lookup = bobgui_style_cascade_lookup;
  iface->emit_error = bobgui_style_cascade_emit_error;
}

G_DEFINE_TYPE_EXTENDED (BobguiStyleCascade, _bobgui_style_cascade, G_TYPE_OBJECT, 0,
                        G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_STYLE_PROVIDER,
                                               bobgui_style_cascade_provider_iface_init));

static void
bobgui_style_cascade_dispose (GObject *object)
{
  BobguiStyleCascade *cascade = BOBGUI_STYLE_CASCADE (object);

  _bobgui_style_cascade_set_parent (cascade, NULL);
  g_array_unref (cascade->providers);

  G_OBJECT_CLASS (_bobgui_style_cascade_parent_class)->dispose (object);
}

static void
_bobgui_style_cascade_class_init (BobguiStyleCascadeClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = bobgui_style_cascade_dispose;
}

static void
style_provider_data_clear (gpointer data_)
{
  BobguiStyleProviderData *data = data_;

  g_signal_handler_disconnect (data->provider, data->changed_signal_id);
  g_object_unref (data->provider);
}

static void
_bobgui_style_cascade_init (BobguiStyleCascade *cascade)
{
  cascade->scale = 1;

  cascade->providers = g_array_new (FALSE, FALSE, sizeof (BobguiStyleProviderData));
  g_array_set_clear_func (cascade->providers, style_provider_data_clear);
}

BobguiStyleCascade *
_bobgui_style_cascade_new (void)
{
  return g_object_new (BOBGUI_TYPE_STYLE_CASCADE, NULL);
}

void
_bobgui_style_cascade_set_parent (BobguiStyleCascade *cascade,
                               BobguiStyleCascade *parent)
{
  bobgui_internal_return_if_fail (BOBGUI_IS_STYLE_CASCADE (cascade));
  bobgui_internal_return_if_fail (parent == NULL || BOBGUI_IS_STYLE_CASCADE (parent));

  if (cascade->parent == parent)
    return;

  if (parent)
    {
      g_object_ref (parent);
      g_signal_connect_swapped (parent,
                                "bobgui-private-changed",
                                G_CALLBACK (bobgui_style_provider_changed),
                                cascade);
    }

  if (cascade->parent)
    {
      g_signal_handlers_disconnect_by_func (cascade->parent, 
                                            bobgui_style_provider_changed,
                                            cascade);
      g_object_unref (cascade->parent);
    }

  cascade->parent = parent;
}

void
_bobgui_style_cascade_add_provider (BobguiStyleCascade  *cascade,
                                 BobguiStyleProvider *provider,
                                 guint             priority)
{
  BobguiStyleProviderData data;
  guint i;

  bobgui_internal_return_if_fail (BOBGUI_IS_STYLE_CASCADE (cascade));
  bobgui_internal_return_if_fail (BOBGUI_IS_STYLE_PROVIDER (provider));
  bobgui_internal_return_if_fail (BOBGUI_STYLE_PROVIDER (cascade) != provider);

  data.provider = g_object_ref (provider);
  data.priority = priority;
  data.changed_signal_id = g_signal_connect_swapped (provider,
                                                     "bobgui-private-changed",
                                                     G_CALLBACK (bobgui_style_provider_changed),
                                                     cascade);

  /* ensure it gets removed first */
  _bobgui_style_cascade_remove_provider (cascade, provider);

  for (i = 0; i < cascade->providers->len; i++)
    {
      if (g_array_index (cascade->providers, BobguiStyleProviderData, i).priority > priority)
        break;
    }
  g_array_insert_val (cascade->providers, i, data);

  bobgui_style_provider_changed (BOBGUI_STYLE_PROVIDER (cascade));
}

void
_bobgui_style_cascade_remove_provider (BobguiStyleCascade  *cascade,
                                    BobguiStyleProvider *provider)
{
  guint i;

  bobgui_internal_return_if_fail (BOBGUI_IS_STYLE_CASCADE (cascade));
  bobgui_internal_return_if_fail (BOBGUI_IS_STYLE_PROVIDER (provider));

  for (i = 0; i < cascade->providers->len; i++)
    {
      BobguiStyleProviderData *data = &g_array_index (cascade->providers, BobguiStyleProviderData, i);

      if (data->provider == provider)
        {
          g_array_remove_index (cascade->providers, i);
  
          bobgui_style_provider_changed (BOBGUI_STYLE_PROVIDER (cascade));
          break;
        }
    }
}

void
_bobgui_style_cascade_set_scale (BobguiStyleCascade *cascade,
                              int              scale)
{
  bobgui_internal_return_if_fail (BOBGUI_IS_STYLE_CASCADE (cascade));

  if (cascade->scale == scale)
    return;

  cascade->scale = scale;

  bobgui_style_provider_changed (BOBGUI_STYLE_PROVIDER (cascade));
}

int
_bobgui_style_cascade_get_scale (BobguiStyleCascade *cascade)
{
  bobgui_internal_return_val_if_fail (BOBGUI_IS_STYLE_CASCADE (cascade), 1);

  return cascade->scale;
}
