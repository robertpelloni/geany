/* BOBGUI - The Bobgui Framework
 * bobguirecentmanager.h: a manager for the recently used resources
 *
 * Copyright (C) 2006 Emmanuele Bassi
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <gdk/gdk.h>
#include <time.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_RECENT_INFO			(bobgui_recent_info_get_type ())

#define BOBGUI_TYPE_RECENT_MANAGER			(bobgui_recent_manager_get_type ())
#define BOBGUI_RECENT_MANAGER(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_RECENT_MANAGER, BobguiRecentManager))
#define BOBGUI_IS_RECENT_MANAGER(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_RECENT_MANAGER))
#define BOBGUI_RECENT_MANAGER_CLASS(klass) 	(G_TYPE_CHECK_CLASS_CAST ((klass), BOBGUI_TYPE_RECENT_MANAGER, BobguiRecentManagerClass))
#define BOBGUI_IS_RECENT_MANAGER_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), BOBGUI_TYPE_RECENT_MANAGER))
#define BOBGUI_RECENT_MANAGER_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_RECENT_MANAGER, BobguiRecentManagerClass))

typedef struct _BobguiRecentInfo		BobguiRecentInfo;
typedef struct _BobguiRecentData		BobguiRecentData;
typedef struct _BobguiRecentManager	BobguiRecentManager;
typedef struct _BobguiRecentManagerClass	BobguiRecentManagerClass;
typedef struct _BobguiRecentManagerPrivate BobguiRecentManagerPrivate;

/**
 * BobguiRecentData:
 * @display_name: a UTF-8 encoded string, containing the name of the recently
 *   used resource to be displayed, or %NULL;
 * @description: a UTF-8 encoded string, containing a short description of
 *   the resource, or %NULL;
 * @mime_type: the MIME type of the resource;
 * @app_name: the name of the application that is registering this recently
 *   used resource;
 * @app_exec: command line used to launch this resource; may contain the
 *   “\%f” and “\%u” escape characters which will be expanded
 *   to the resource file path and URI respectively when the command line
 *   is retrieved;
 * @groups: (array zero-terminated=1): a vector of strings containing
 *   groups names;
 * @is_private: whether this resource should be displayed only by the
 *   applications that have registered it or not.
 *
 * Meta-data to be passed to bobgui_recent_manager_add_full() when
 * registering a recently used resource.
 **/
struct _BobguiRecentData
{
  char *display_name;
  char *description;

  char *mime_type;

  char *app_name;
  char *app_exec;

  char **groups;

  gboolean is_private;
};

struct _BobguiRecentManager
{
  /*< private >*/
  GObject parent_instance;

  BobguiRecentManagerPrivate *priv;
};

/**
 * BobguiRecentManagerClass:
 *
 * `BobguiRecentManagerClass` contains only private data.
 */
struct _BobguiRecentManagerClass
{
  /*< private >*/
  GObjectClass parent_class;

  void (*changed) (BobguiRecentManager *manager);

  /* padding for future expansion */
  void (*_bobgui_recent1) (void);
  void (*_bobgui_recent2) (void);
  void (*_bobgui_recent3) (void);
  void (*_bobgui_recent4) (void);
};

/**
 * BobguiRecentManagerError:
 * @BOBGUI_RECENT_MANAGER_ERROR_NOT_FOUND: the URI specified does not exists in
 *   the recently used resources list.
 * @BOBGUI_RECENT_MANAGER_ERROR_INVALID_URI: the URI specified is not valid.
 * @BOBGUI_RECENT_MANAGER_ERROR_INVALID_ENCODING: the supplied string is not
 *   UTF-8 encoded.
 * @BOBGUI_RECENT_MANAGER_ERROR_NOT_REGISTERED: no application has registered
 *   the specified item.
 * @BOBGUI_RECENT_MANAGER_ERROR_READ: failure while reading the recently used
 *   resources file.
 * @BOBGUI_RECENT_MANAGER_ERROR_WRITE: failure while writing the recently used
 *   resources file.
 * @BOBGUI_RECENT_MANAGER_ERROR_UNKNOWN: unspecified error.
 *
 * Error codes for `BobguiRecentManager` operations
 */
typedef enum
{
  BOBGUI_RECENT_MANAGER_ERROR_NOT_FOUND,
  BOBGUI_RECENT_MANAGER_ERROR_INVALID_URI,
  BOBGUI_RECENT_MANAGER_ERROR_INVALID_ENCODING,
  BOBGUI_RECENT_MANAGER_ERROR_NOT_REGISTERED,
  BOBGUI_RECENT_MANAGER_ERROR_READ,
  BOBGUI_RECENT_MANAGER_ERROR_WRITE,
  BOBGUI_RECENT_MANAGER_ERROR_UNKNOWN
} BobguiRecentManagerError;

/**
 * BOBGUI_RECENT_MANAGER_ERROR:
 *
 * The `GError` domain for `BobguiRecentManager` errors.
 */
#define BOBGUI_RECENT_MANAGER_ERROR	(bobgui_recent_manager_error_quark ())
GDK_AVAILABLE_IN_ALL
GQuark 	bobgui_recent_manager_error_quark (void);


GDK_AVAILABLE_IN_ALL
GType 		  bobgui_recent_manager_get_type       (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
BobguiRecentManager *bobgui_recent_manager_new            (void);
GDK_AVAILABLE_IN_ALL
BobguiRecentManager *bobgui_recent_manager_get_default    (void);

GDK_AVAILABLE_IN_ALL
gboolean          bobgui_recent_manager_add_item       (BobguiRecentManager     *manager,
						     const char           *uri);
GDK_AVAILABLE_IN_ALL
gboolean          bobgui_recent_manager_add_full       (BobguiRecentManager     *manager,
						     const char           *uri,
						     const BobguiRecentData  *recent_data);
GDK_AVAILABLE_IN_ALL
gboolean          bobgui_recent_manager_remove_item    (BobguiRecentManager     *manager,
						     const char           *uri,
						     GError              **error);
GDK_AVAILABLE_IN_ALL
BobguiRecentInfo *   bobgui_recent_manager_lookup_item    (BobguiRecentManager     *manager,
						     const char           *uri,
						     GError              **error);
GDK_AVAILABLE_IN_ALL
gboolean          bobgui_recent_manager_has_item       (BobguiRecentManager     *manager,
						     const char           *uri);
GDK_AVAILABLE_IN_ALL
gboolean          bobgui_recent_manager_move_item      (BobguiRecentManager     *manager,
						     const char           *uri,
						     const char           *new_uri,
						     GError              **error);
GDK_AVAILABLE_IN_ALL
GList *           bobgui_recent_manager_get_items      (BobguiRecentManager     *manager);
GDK_AVAILABLE_IN_ALL
int               bobgui_recent_manager_purge_items    (BobguiRecentManager     *manager,
						     GError              **error);


GDK_AVAILABLE_IN_ALL
GType	              bobgui_recent_info_get_type             (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
BobguiRecentInfo *       bobgui_recent_info_ref                  (BobguiRecentInfo  *info);
GDK_AVAILABLE_IN_ALL
void                  bobgui_recent_info_unref                (BobguiRecentInfo  *info);

GDK_AVAILABLE_IN_ALL
const char *         bobgui_recent_info_get_uri              (BobguiRecentInfo  *info);
GDK_AVAILABLE_IN_ALL
const char *         bobgui_recent_info_get_display_name     (BobguiRecentInfo  *info);
GDK_AVAILABLE_IN_ALL
const char *         bobgui_recent_info_get_description      (BobguiRecentInfo  *info);
GDK_AVAILABLE_IN_ALL
const char *         bobgui_recent_info_get_mime_type        (BobguiRecentInfo  *info);
GDK_AVAILABLE_IN_ALL
GDateTime *          bobgui_recent_info_get_added            (BobguiRecentInfo  *info);
GDK_AVAILABLE_IN_ALL
GDateTime *          bobgui_recent_info_get_modified         (BobguiRecentInfo  *info);
GDK_AVAILABLE_IN_ALL
GDateTime *          bobgui_recent_info_get_visited          (BobguiRecentInfo  *info);
GDK_AVAILABLE_IN_ALL
gboolean             bobgui_recent_info_get_private_hint     (BobguiRecentInfo  *info);
GDK_AVAILABLE_IN_ALL
gboolean             bobgui_recent_info_get_application_info (BobguiRecentInfo  *info,
                                                           const char     *app_name,
                                                           const char    **app_exec,
                                                           guint          *count,
                                                           GDateTime     **stamp);
GDK_AVAILABLE_IN_ALL
GAppInfo *            bobgui_recent_info_create_app_info      (BobguiRecentInfo  *info,
                                                            const char     *app_name,
                                                            GError        **error);
GDK_AVAILABLE_IN_ALL
char **              bobgui_recent_info_get_applications     (BobguiRecentInfo  *info,
							    gsize          *length) G_GNUC_MALLOC;
GDK_AVAILABLE_IN_ALL
char *               bobgui_recent_info_last_application     (BobguiRecentInfo  *info) G_GNUC_MALLOC;
GDK_AVAILABLE_IN_ALL
gboolean              bobgui_recent_info_has_application      (BobguiRecentInfo  *info,
							    const char     *app_name);
GDK_AVAILABLE_IN_ALL
char **              bobgui_recent_info_get_groups           (BobguiRecentInfo  *info,
							    gsize          *length) G_GNUC_MALLOC;
GDK_AVAILABLE_IN_ALL
gboolean              bobgui_recent_info_has_group            (BobguiRecentInfo  *info,
							    const char     *group_name);
GDK_AVAILABLE_IN_ALL
GIcon *               bobgui_recent_info_get_gicon            (BobguiRecentInfo  *info);
GDK_AVAILABLE_IN_ALL
char *               bobgui_recent_info_get_short_name       (BobguiRecentInfo  *info) G_GNUC_MALLOC;
GDK_AVAILABLE_IN_ALL
char *               bobgui_recent_info_get_uri_display      (BobguiRecentInfo  *info) G_GNUC_MALLOC;
GDK_AVAILABLE_IN_ALL
int                   bobgui_recent_info_get_age              (BobguiRecentInfo  *info);
GDK_AVAILABLE_IN_ALL
gboolean              bobgui_recent_info_is_local             (BobguiRecentInfo  *info);
GDK_AVAILABLE_IN_ALL
gboolean              bobgui_recent_info_exists               (BobguiRecentInfo  *info);
GDK_AVAILABLE_IN_ALL
gboolean              bobgui_recent_info_match                (BobguiRecentInfo  *info_a,
							    BobguiRecentInfo  *info_b);

/* private */
void _bobgui_recent_manager_sync (void);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiRecentManager, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiRecentInfo, bobgui_recent_info_unref)

G_END_DECLS

