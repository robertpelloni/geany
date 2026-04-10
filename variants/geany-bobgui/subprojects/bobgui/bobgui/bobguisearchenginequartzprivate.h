/*
 * Copyright (C) 2007  Kristian Rietveld  <kris@bobgui.org>
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

#pragma once

#include "bobguisearchengineprivate.h"

G_BEGIN_DECLS

#define BOBGUI_TYPE_SEARCH_ENGINE_QUARTZ			(_bobgui_search_engine_quartz_get_type ())
#define BOBGUI_SEARCH_ENGINE_QUARTZ(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_SEARCH_ENGINE_QUARTZ, BobguiSearchEngineQuartz))
#define BOBGUI_SEARCH_ENGINE_QUARTZ_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), BOBGUI_TYPE_SEARCH_ENGINE_QUARTZ, BobguiSearchEngineQuartzClass))
#define BOBGUI_IS_SEARCH_ENGINE_QUARTZ(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_SEARCH_ENGINE_QUARTZ))
#define BOBGUI_IS_SEARCH_ENGINE_QUARTZ_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), BOBGUI_TYPE_SEARCH_ENGINE_QUARTZ))
#define BOBGUI_SEARCH_ENGINE_QUARTZ_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_SEARCH_ENGINE_QUARTZ, BobguiSearchEngineQuartzClass))

typedef struct _BobguiSearchEngineQuartz BobguiSearchEngineQuartz;
typedef struct _BobguiSearchEngineQuartzClass BobguiSearchEngineQuartzClass;
typedef struct _BobguiSearchEngineQuartzPrivate BobguiSearchEngineQuartzPrivate;

struct _BobguiSearchEngineQuartz
{
  BobguiSearchEngine parent;

  BobguiSearchEngineQuartzPrivate *priv;
};

struct _BobguiSearchEngineQuartzClass
{
  BobguiSearchEngineClass parent_class;
};

GType            _bobgui_search_engine_quartz_get_type (void);
BobguiSearchEngine *_bobgui_search_engine_quartz_new      (void);

G_END_DECLS

