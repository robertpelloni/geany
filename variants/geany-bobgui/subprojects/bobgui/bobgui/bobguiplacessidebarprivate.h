/* bobguiplacessidebarprivate.h
 *
 * Copyright (C) 2015 Red Hat
 *
 * This file is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of the
 * License, or (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Carlos Soriano <csoriano@gnome.org>
 */

#pragma once

#include <glib.h>
#include <bobgui/bobguiwidget.h>
#include <bobgui/bobguienums.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_PLACES_SIDEBAR			(bobgui_places_sidebar_get_type ())
#define BOBGUI_PLACES_SIDEBAR(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_PLACES_SIDEBAR, BobguiPlacesSidebar))
#define BOBGUI_PLACES_SIDEBAR_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), BOBGUI_TYPE_PLACES_SIDEBAR, BobguiPlacesSidebarClass))
#define BOBGUI_IS_PLACES_SIDEBAR(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_PLACES_SIDEBAR))
#define BOBGUI_IS_PLACES_SIDEBAR_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), BOBGUI_TYPE_PLACES_SIDEBAR))
#define BOBGUI_PLACES_SIDEBAR_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_PLACES_SIDEBAR, BobguiPlacesSidebarClass))

typedef struct _BobguiPlacesSidebar BobguiPlacesSidebar;
typedef struct _BobguiPlacesSidebarClass BobguiPlacesSidebarClass;

/*
 * BobguiPlacesOpenFlags:
 * @BOBGUI_PLACES_OPEN_NORMAL: This is the default mode that BobguiPlacesSidebar uses if no other flags
 *  are specified.  It indicates that the calling application should open the selected location
 *  in the normal way, for example, in the folder view beside the sidebar.
 * @BOBGUI_PLACES_OPEN_NEW_TAB: When passed to bobgui_places_sidebar_set_open_flags(), this indicates
 *  that the application can open folders selected from the sidebar in new tabs.  This value
 *  will be passed to the BobguiPlacesSidebar::open-location signal when the user selects
 *  that a location be opened in a new tab instead of in the standard fashion.
 * @BOBGUI_PLACES_OPEN_NEW_WINDOW: Similar to @BOBGUI_PLACES_OPEN_NEW_TAB, but indicates that the application
 *  can open folders in new windows.
 *
 * These flags serve two purposes.  First, the application can call bobgui_places_sidebar_set_open_flags()
 * using these flags as a bitmask.  This tells the sidebar that the application is able to open
 * folders selected from the sidebar in various ways, for example, in new tabs or in new windows in
 * addition to the normal mode.
 *
 * Second, when one of these values gets passed back to the application in the
 * BobguiPlacesSidebar::open-location signal, it means that the application should
 * open the selected location in the normal way, in a new tab, or in a new
 * window.  The sidebar takes care of determining the desired way to open the location,
 * based on the modifier keys that the user is pressing at the time the selection is made.
 *
 * If the application never calls bobgui_places_sidebar_set_open_flags(), then the sidebar will only
 * use BOBGUI_PLACES_OPEN_NORMAL in the BobguiPlacesSidebar::open-location signal.  This is the
 * default mode of operation.
 */
typedef enum {
  BOBGUI_PLACES_OPEN_NORMAL     = 1 << 0,
  BOBGUI_PLACES_OPEN_NEW_TAB    = 1 << 1,
  BOBGUI_PLACES_OPEN_NEW_WINDOW = 1 << 2
} BobguiPlacesOpenFlags;

GType              bobgui_places_sidebar_get_type                   (void) G_GNUC_CONST;
BobguiWidget *        bobgui_places_sidebar_new                        (void);

BobguiPlacesOpenFlags bobgui_places_sidebar_get_open_flags             (BobguiPlacesSidebar   *sidebar);
void               bobgui_places_sidebar_set_open_flags             (BobguiPlacesSidebar   *sidebar,
                                                                  BobguiPlacesOpenFlags  flags);

GFile *            bobgui_places_sidebar_get_location               (BobguiPlacesSidebar   *sidebar);
void               bobgui_places_sidebar_set_location               (BobguiPlacesSidebar   *sidebar,
                                                                  GFile              *location);

gboolean           bobgui_places_sidebar_get_show_recent            (BobguiPlacesSidebar   *sidebar);
void               bobgui_places_sidebar_set_show_recent            (BobguiPlacesSidebar   *sidebar,
                                                                  gboolean            show_recent);

gboolean           bobgui_places_sidebar_get_show_desktop           (BobguiPlacesSidebar   *sidebar);
void               bobgui_places_sidebar_set_show_desktop           (BobguiPlacesSidebar   *sidebar,
                                                                  gboolean            show_desktop);

gboolean           bobgui_places_sidebar_get_show_enter_location    (BobguiPlacesSidebar   *sidebar);
void               bobgui_places_sidebar_set_show_enter_location    (BobguiPlacesSidebar   *sidebar,
                                                                  gboolean            show_enter_location);

void               bobgui_places_sidebar_add_shortcut               (BobguiPlacesSidebar   *sidebar,
                                                                  GFile              *location);
void               bobgui_places_sidebar_remove_shortcut            (BobguiPlacesSidebar   *sidebar,
                                                                  GFile              *location);
GListModel *       bobgui_places_sidebar_get_shortcuts              (BobguiPlacesSidebar   *sidebar);

GFile *            bobgui_places_sidebar_get_nth_bookmark           (BobguiPlacesSidebar   *sidebar,
                                                                  int                 n);
void               bobgui_places_sidebar_set_drop_targets_visible   (BobguiPlacesSidebar   *sidebar,
                                                                  gboolean            visible);
gboolean           bobgui_places_sidebar_get_show_trash             (BobguiPlacesSidebar   *sidebar);
void               bobgui_places_sidebar_set_show_trash             (BobguiPlacesSidebar   *sidebar,
                                                                  gboolean            show_trash);

void                 bobgui_places_sidebar_set_show_other_locations (BobguiPlacesSidebar   *sidebar,
                                                                  gboolean            show_other_locations);
gboolean             bobgui_places_sidebar_get_show_other_locations (BobguiPlacesSidebar   *sidebar);

void                 bobgui_places_sidebar_set_show_starred_location (BobguiPlacesSidebar   *sidebar,
                                                                   gboolean            show_starred_location);
gboolean             bobgui_places_sidebar_get_show_starred_location (BobguiPlacesSidebar   *sidebar);

/* Keep order, since it's used for the sort functions */
typedef enum {
  BOBGUI_PLACES_SECTION_INVALID,
  BOBGUI_PLACES_SECTION_COMPUTER,
  BOBGUI_PLACES_SECTION_MOUNTS,
  BOBGUI_PLACES_SECTION_CLOUD,
  BOBGUI_PLACES_SECTION_BOOKMARKS,
  BOBGUI_PLACES_SECTION_OTHER_LOCATIONS,
  BOBGUI_PLACES_N_SECTIONS
} BobguiPlacesSectionType;

typedef enum {
  BOBGUI_PLACES_INVALID,
  BOBGUI_PLACES_BUILT_IN,
  BOBGUI_PLACES_XDG_DIR,
  BOBGUI_PLACES_MOUNTED_VOLUME,
  BOBGUI_PLACES_BOOKMARK,
  BOBGUI_PLACES_HEADING,
  BOBGUI_PLACES_CONNECT_TO_SERVER,
  BOBGUI_PLACES_ENTER_LOCATION,
  BOBGUI_PLACES_DROP_FEEDBACK,
  BOBGUI_PLACES_BOOKMARK_PLACEHOLDER,
  BOBGUI_PLACES_OTHER_LOCATIONS,
  BOBGUI_PLACES_STARRED_LOCATION,
  BOBGUI_PLACES_N_PLACES
} BobguiPlacesPlaceType;

char *bobgui_places_sidebar_get_location_title (BobguiPlacesSidebar *sidebar);

G_END_DECLS

