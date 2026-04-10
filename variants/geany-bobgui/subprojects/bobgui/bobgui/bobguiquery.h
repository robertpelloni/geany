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
 * Based on nautilus-query.h
 */

#pragma once

#include <gio/gio.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_QUERY          (bobgui_query_get_type ())
#define BOBGUI_QUERY(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_QUERY, BobguiQuery))
#define BOBGUI_QUERY_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass), BOBGUI_TYPE_QUERY, BobguiQueryClass))
#define BOBGUI_IS_QUERY(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_QUERY))
#define BOBGUI_IS_QUERY_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), BOBGUI_TYPE_QUERY))
#define BOBGUI_QUERY_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_QUERY, BobguiQueryClass))

typedef struct _BobguiQuery BobguiQuery;
typedef struct _BobguiQueryClass BobguiQueryClass;
typedef struct _BobguiQueryPrivate BobguiQueryPrivate;

struct _BobguiQuery
{
  GObject parent;
};

struct _BobguiQueryClass
{
  GObjectClass parent_class;
};

GType        bobgui_query_get_type       (void);

BobguiQuery    *bobgui_query_new            (void);

const char *bobgui_query_get_text       (BobguiQuery    *query);
void         bobgui_query_set_text       (BobguiQuery    *query,
                                       const char *text);

GFile       *bobgui_query_get_location   (BobguiQuery    *query);
void         bobgui_query_set_location   (BobguiQuery    *query,
                                       GFile       *file);

gboolean     bobgui_query_matches_string (BobguiQuery    *query,
                                       const char *string);

G_END_DECLS

