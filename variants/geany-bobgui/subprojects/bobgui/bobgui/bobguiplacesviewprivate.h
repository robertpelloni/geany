/* bobguiplacesview.h
 *
 * Copyright (C) 2015 Georges Basile Stavracas Neto <georges.stavracas@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <bobgui/bobguibox.h>
#include <bobgui/bobguiplacessidebarprivate.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_PLACES_VIEW        (bobgui_places_view_get_type ())
#define BOBGUI_PLACES_VIEW(obj)        (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_PLACES_VIEW, BobguiPlacesView))
#define BOBGUI_PLACES_VIEW_CLASS(klass)(G_TYPE_CHECK_CLASS_CAST ((klass), BOBGUI_TYPE_PLACES_VIEW, BobguiPlacesViewClass))
#define BOBGUI_IS_PLACES_VIEW(obj)     (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_PLACES_VIEW))
#define BOBGUI_IS_PLACES_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BOBGUI_TYPE_PLACES_VIEW))
#define BOBGUI_PLACES_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_PLACES_VIEW, BobguiPlacesViewClass))

typedef struct _BobguiPlacesView BobguiPlacesView;
typedef struct _BobguiPlacesViewClass BobguiPlacesViewClass;

GType              bobgui_places_view_get_type                      (void) G_GNUC_CONST;

BobguiPlacesOpenFlags bobgui_places_view_get_open_flags                (BobguiPlacesView      *view);
void               bobgui_places_view_set_open_flags                (BobguiPlacesView      *view,
                                                                  BobguiPlacesOpenFlags  flags);

const char *       bobgui_places_view_get_search_query              (BobguiPlacesView      *view);
void               bobgui_places_view_set_search_query              (BobguiPlacesView      *view,
                                                                  const char         *query_text);

gboolean           bobgui_places_view_get_loading                   (BobguiPlacesView         *view);

BobguiWidget *        bobgui_places_view_new                           (void);

G_END_DECLS

