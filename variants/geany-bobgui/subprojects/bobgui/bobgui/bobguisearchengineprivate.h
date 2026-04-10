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
 * Based on nautilus-search-engine.h
 */

#pragma once

#include "bobguiquery.h"
#include "bobguifilesystemmodelprivate.h"
#include <gio/gio.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_SEARCH_ENGINE		(_bobgui_search_engine_get_type ())
#define BOBGUI_SEARCH_ENGINE(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_SEARCH_ENGINE, BobguiSearchEngine))
#define BOBGUI_SEARCH_ENGINE_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), BOBGUI_TYPE_SEARCH_ENGINE, BobguiSearchEngineClass))
#define BOBGUI_IS_SEARCH_ENGINE(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_SEARCH_ENGINE))
#define BOBGUI_IS_SEARCH_ENGINE_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), BOBGUI_TYPE_SEARCH_ENGINE))
#define BOBGUI_SEARCH_ENGINE_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_SEARCH_ENGINE, BobguiSearchEngineClass))

typedef struct _BobguiSearchEngine BobguiSearchEngine;
typedef struct _BobguiSearchEngineClass BobguiSearchEngineClass;
typedef struct _BobguiSearchEnginePrivate BobguiSearchEnginePrivate;
typedef struct _BobguiSearchHit BobguiSearchHit;

G_DEFINE_AUTOPTR_CLEANUP_FUNC (BobguiSearchEngine, g_object_unref)

struct _BobguiSearchHit
{
  GFile *file;
  GFileInfo *info; /* may be NULL */
};

struct _BobguiSearchEngine
{
  GObject parent;

  BobguiSearchEnginePrivate *priv;
};

struct _BobguiSearchEngineClass 
{
  GObjectClass parent_class;
  
  /* VTable */
  void     (*set_query)       (BobguiSearchEngine *engine, 
			       BobguiQuery        *query);
  void     (*start)           (BobguiSearchEngine *engine);
  void     (*stop)            (BobguiSearchEngine *engine);
  
  /* Signals */
  void     (*hits_added)      (BobguiSearchEngine *engine, 
			       GList           *hits);
  void     (*finished)        (BobguiSearchEngine *engine);
  void     (*error)           (BobguiSearchEngine *engine, 
			       const char      *error_message);
};

GType            _bobgui_search_engine_get_type        (void);

BobguiSearchEngine* _bobgui_search_engine_new             (void);

void             _bobgui_search_engine_set_query       (BobguiSearchEngine *engine, 
                                                     BobguiQuery        *query);
void	         _bobgui_search_engine_start           (BobguiSearchEngine *engine);
void	         _bobgui_search_engine_stop            (BobguiSearchEngine *engine);

void	         _bobgui_search_engine_hits_added      (BobguiSearchEngine *engine, 
						     GList           *hits);
void             _bobgui_search_engine_finished        (BobguiSearchEngine *engine,
                                                     gboolean         got_results);
void	         _bobgui_search_engine_error           (BobguiSearchEngine *engine, 
						     const char      *error_message);

void             _bobgui_search_hit_free (BobguiSearchHit *hit);
BobguiSearchHit    *_bobgui_search_hit_dup (BobguiSearchHit *hit);

void             _bobgui_search_engine_set_model       (BobguiSearchEngine    *engine,
                                                     BobguiFileSystemModel *model);

G_END_DECLS

