/*
 * Copyright (C) 2015 Red Hat, Inc
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
 * Author: Matthias Clasen <mclasen@redhat.com>
 */

#include "config.h"

#include <gio/gio.h>

#include <gdk/gdk.h>

#include "bobguisearchenginemodelprivate.h"
#include "bobguifilechooserutils.h"
#include "bobguiprivate.h"

#include <string.h>

struct _BobguiSearchEngineModel
{
  BobguiSearchEngine parent;

  BobguiFileSystemModel *model;
  BobguiQuery *query;

  guint idle;
};

struct _BobguiSearchEngineModelClass
{
  BobguiSearchEngineClass parent_class;
};

G_DEFINE_TYPE (BobguiSearchEngineModel, _bobgui_search_engine_model, BOBGUI_TYPE_SEARCH_ENGINE)

static void
bobgui_search_engine_model_dispose (GObject *object)
{
  BobguiSearchEngineModel *model = BOBGUI_SEARCH_ENGINE_MODEL (object);

  g_clear_object (&model->query);
  g_clear_object (&model->model);

  G_OBJECT_CLASS (_bobgui_search_engine_model_parent_class)->dispose (object);
}

static gboolean
info_matches_query (BobguiQuery  *query,
                    GFileInfo *info)
{
  const char *display_name;

  display_name = g_file_info_get_display_name (info);
  if (display_name == NULL)
    return FALSE;

  if (g_file_info_get_is_hidden (info))
    return FALSE;

  if (!bobgui_query_matches_string (query, display_name))
    return FALSE;

  return TRUE;
}

static gboolean
do_search (gpointer data)
{
  BobguiSearchEngineModel *model = data;
  GList *hits = NULL;
  gboolean got_results = FALSE;

  for (guint i = 0; i < g_list_model_get_n_items (G_LIST_MODEL (model->model)); i++)
    {
      GFileInfo *info = g_list_model_get_item (G_LIST_MODEL (model->model), i);

      if (info_matches_query (model->query, info))
        {
          GFile *file;
          BobguiSearchHit *hit;

          file = _bobgui_file_info_get_file (info);
          hit = g_new (BobguiSearchHit, 1);
          hit->file = g_object_ref (file);
          hit->info = g_object_ref (info);
          hits = g_list_prepend (hits, hit);
        }

      g_clear_object (&info);
    }

  if (hits)
    {
      _bobgui_search_engine_hits_added (BOBGUI_SEARCH_ENGINE (model), hits);
      g_list_free_full (hits, (GDestroyNotify)_bobgui_search_hit_free);
      got_results = TRUE;
    }

  model->idle = 0;

  _bobgui_search_engine_finished (BOBGUI_SEARCH_ENGINE (model), got_results);

  return G_SOURCE_REMOVE;
}

static void
bobgui_search_engine_model_start (BobguiSearchEngine *engine)
{
  BobguiSearchEngineModel *model;

  model = BOBGUI_SEARCH_ENGINE_MODEL (engine);

  if (model->query == NULL)
    return;

  model->idle = g_idle_add (do_search, engine);
  gdk_source_set_static_name_by_id (model->idle, "[bobgui] bobgui_search_engine_model_start");
}

static void
bobgui_search_engine_model_stop (BobguiSearchEngine *engine)
{
  BobguiSearchEngineModel *model;

  model = BOBGUI_SEARCH_ENGINE_MODEL (engine);

  if (model->idle != 0)
    {
      g_source_remove (model->idle);
      model->idle = 0;
    }
}

static void
bobgui_search_engine_model_set_query (BobguiSearchEngine *engine,
                                   BobguiQuery        *query)
{
  BobguiSearchEngineModel *model = BOBGUI_SEARCH_ENGINE_MODEL (engine);

  g_set_object (&model->query, query);
}

static void
_bobgui_search_engine_model_class_init (BobguiSearchEngineModelClass *class)
{
  GObjectClass *gobject_class;
  BobguiSearchEngineClass *engine_class;

  gobject_class = G_OBJECT_CLASS (class);
  gobject_class->dispose = bobgui_search_engine_model_dispose;

  engine_class = BOBGUI_SEARCH_ENGINE_CLASS (class);
  engine_class->set_query = bobgui_search_engine_model_set_query;
  engine_class->start = bobgui_search_engine_model_start;
  engine_class->stop = bobgui_search_engine_model_stop;
}

static void
_bobgui_search_engine_model_init (BobguiSearchEngineModel *engine)
{
}

BobguiSearchEngine *
_bobgui_search_engine_model_new (BobguiFileSystemModel *model)
{
  BobguiSearchEngineModel *engine;

  engine = BOBGUI_SEARCH_ENGINE_MODEL (g_object_new (BOBGUI_TYPE_SEARCH_ENGINE_MODEL, NULL));
  engine->model = g_object_ref (model);

  return BOBGUI_SEARCH_ENGINE (engine);
}
