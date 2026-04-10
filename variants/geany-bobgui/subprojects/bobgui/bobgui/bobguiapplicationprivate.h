/*
 * Copyright © 2011, 2013 Canonical Limited
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the licence, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Ryan Lortie <desrt@desrt.ca>
 */


#pragma once

#include "bobguiapplicationwindow.h"
#include "bobguiwindowprivate.h"
#include "bobguiapplicationwindow.h"

#include "bobguiactionmuxerprivate.h"
#include "bobguiapplicationaccelsprivate.h"

G_BEGIN_DECLS

void                    bobgui_application_window_set_id                   (BobguiApplicationWindow     *window,
                                                                         guint                     id);
GActionGroup *          bobgui_application_window_get_action_group         (BobguiApplicationWindow     *window);
void                    bobgui_application_handle_window_realize           (BobguiApplication           *application,
                                                                         BobguiWindow                *window);
void                    bobgui_application_handle_window_map               (BobguiApplication           *application,
                                                                         BobguiWindow                *window);
BobguiActionMuxer *        bobgui_application_get_parent_muxer_for_window     (BobguiWindow                *window);

BobguiActionMuxer *        bobgui_application_get_action_muxer                (BobguiApplication           *application);
void                    bobgui_application_insert_action_group             (BobguiApplication           *application,
                                                                         const char               *name,
                                                                         GActionGroup             *action_group);

BobguiApplicationAccels *  bobgui_application_get_application_accels          (BobguiApplication           *application);

void                    bobgui_application_set_screensaver_active          (BobguiApplication           *application,
                                                                         gboolean                  active);


#define BOBGUI_TYPE_APPLICATION_IMPL                           (bobgui_application_impl_get_type ())
#define BOBGUI_APPLICATION_IMPL_CLASS(class)                   (G_TYPE_CHECK_CLASS_CAST ((class),                     \
                                                             BOBGUI_TYPE_APPLICATION_IMPL,                            \
                                                             BobguiApplicationImplClass))
#define BOBGUI_APPLICATION_IMPL_GET_CLASS(obj)                 (G_TYPE_INSTANCE_GET_CLASS ((obj),                     \
                                                             BOBGUI_TYPE_APPLICATION_IMPL,                            \
                                                             BobguiApplicationImplClass))

typedef struct
{
  GObject parent_instance;
  BobguiApplication *application;
  GdkDisplay *display;
} BobguiApplicationImpl;

typedef struct
{
  GObjectClass parent_class;

  void        (* startup)                   (BobguiApplicationImpl          *impl,
                                             gboolean                     support_save);
  void        (* shutdown)                  (BobguiApplicationImpl          *impl);

  void        (* before_emit)               (BobguiApplicationImpl          *impl,
                                             GVariant                    *platform_data);

  void        (* window_added)              (BobguiApplicationImpl          *impl,
                                             BobguiWindow                   *window,
                                             GVariant                    *state);
  void        (* window_removed)            (BobguiApplicationImpl          *impl,
                                             BobguiWindow                   *window);
  void        (* window_forget)             (BobguiApplicationImpl          *impl,
                                             BobguiWindow                   *window);
  void        (* active_window_changed)     (BobguiApplicationImpl          *impl,
                                             BobguiWindow                   *window);
  void        (* handle_window_realize)     (BobguiApplicationImpl          *impl,
                                             BobguiWindow                   *window);
  void        (* handle_window_map)         (BobguiApplicationImpl          *impl,
                                             BobguiWindow                   *window);

  void        (* set_app_menu)              (BobguiApplicationImpl          *impl,
                                             GMenuModel                  *app_menu);
  void        (* set_menubar)               (BobguiApplicationImpl          *impl,
                                             GMenuModel                  *menubar);

  guint       (* inhibit)                   (BobguiApplicationImpl          *impl,
                                             BobguiWindow                   *window,
                                             BobguiApplicationInhibitFlags   flags,
                                             const char                  *reason);
  void        (* uninhibit)                 (BobguiApplicationImpl          *impl,
                                             guint                        cookie);
  gboolean    (* is_inhibited)              (BobguiApplicationImpl          *impl,
                                             BobguiApplicationInhibitFlags   flags);

  BobguiRestoreReason
               (* get_restore_reason)       (BobguiApplicationImpl          *impl);

  void         (* clear_restore_reason)     (BobguiApplicationImpl          *impl);

  void         (* collect_global_state)     (BobguiApplicationImpl          *impl,
                                             GVariantBuilder             *state);
  void         (* restore_global_state)     (BobguiApplicationImpl          *impl,
                                             GVariant                    *state);
  void         (* collect_window_state)     (BobguiApplicationImpl          *impl,
                                             BobguiWindow                   *window,
                                             GVariantBuilder             *state);
  void         (* store_state)              (BobguiApplicationImpl          *impl,
                                             GVariant                    *state);
  void         (* forget_state)             (BobguiApplicationImpl          *impl);
  GVariant *   (* retrieve_state)           (BobguiApplicationImpl          *impl);
} BobguiApplicationImplClass;

#define BOBGUI_TYPE_APPLICATION_IMPL_DBUS                      (bobgui_application_impl_dbus_get_type ())
#define BOBGUI_APPLICATION_IMPL_DBUS_CLASS(class)              (G_TYPE_CHECK_CLASS_CAST ((class),                     \
                                                             BOBGUI_TYPE_APPLICATION_IMPL_DBUS,                       \
                                                             BobguiApplicationImplDBusClass))
#define BOBGUI_APPLICATION_IMPL_DBUS_GET_CLASS(obj)            (G_TYPE_INSTANCE_GET_CLASS ((obj),                     \
                                                             BOBGUI_TYPE_APPLICATION_IMPL_DBUS,                       \
                                                             BobguiApplicationImplDBusClass))

typedef struct
{
  BobguiApplicationImpl impl;

  GDBusConnection *session;
  GCancellable    *cancellable;

  const char      *application_id;
  const char      *unique_name;
  const char      *object_path;

  char            *app_menu_path;
  guint            app_menu_id;

  char            *menubar_path;
  guint            menubar_id;

  char            *instance_id;
  BobguiRestoreReason reason;

  /* Portal support */
  GDBusProxy      *inhibit_proxy;
  GSList          *inhibit_handles;
  guint            state_changed_handler;
  char            *session_path;
  guint            session_state;
} BobguiApplicationImplDBus;

typedef struct
{
  BobguiApplicationImplClass parent_class;

} BobguiApplicationImplDBusClass;

GType                   bobgui_application_impl_get_type                   (void);
GType                   bobgui_application_impl_dbus_get_type              (void);
GType                   bobgui_application_impl_x11_get_type               (void);
GType                   bobgui_application_impl_wayland_get_type           (void);
GType                   bobgui_application_impl_quartz_get_type            (void);
GType                   bobgui_application_impl_android_get_type           (void);
GType                   bobgui_application_impl_win32_get_type             (void);

BobguiApplicationImpl *    bobgui_application_impl_new                        (BobguiApplication              *application,
                                                                         GdkDisplay                  *display);
void                    bobgui_application_impl_startup                    (BobguiApplicationImpl          *impl,
                                                                         gboolean                     support_save);
void                    bobgui_application_impl_shutdown                   (BobguiApplicationImpl          *impl);
void                    bobgui_application_impl_before_emit                (BobguiApplicationImpl          *impl,
                                                                         GVariant                    *platform_data);
void                    bobgui_application_impl_window_added               (BobguiApplicationImpl          *impl,
                                                                         BobguiWindow                   *window,
                                                                         GVariant                    *state);
void                    bobgui_application_impl_window_removed             (BobguiApplicationImpl          *impl,
                                                                         BobguiWindow                   *window);
void                    bobgui_application_impl_window_forget              (BobguiApplicationImpl          *impl,
                                                                         BobguiWindow                   *window);
void                    bobgui_application_impl_active_window_changed      (BobguiApplicationImpl          *impl,
                                                                         BobguiWindow                   *window);
void                    bobgui_application_impl_handle_window_realize      (BobguiApplicationImpl          *impl,
                                                                         BobguiWindow                   *window);
void                    bobgui_application_impl_handle_window_map          (BobguiApplicationImpl          *impl,
                                                                         BobguiWindow                   *window);
void                    bobgui_application_impl_set_app_menu               (BobguiApplicationImpl          *impl,
                                                                         GMenuModel                  *app_menu);
void                    bobgui_application_impl_set_menubar                (BobguiApplicationImpl          *impl,
                                                                         GMenuModel                  *menubar);
guint                   bobgui_application_impl_inhibit                    (BobguiApplicationImpl          *impl,
                                                                         BobguiWindow                   *window,
                                                                         BobguiApplicationInhibitFlags   flags,
                                                                         const char                  *reason);
void                    bobgui_application_impl_uninhibit                  (BobguiApplicationImpl          *impl,
                                                                         guint                        cookie);
gboolean                bobgui_application_impl_is_inhibited               (BobguiApplicationImpl          *impl,
                                                                         BobguiApplicationInhibitFlags   flags);

char *                 bobgui_application_impl_dbus_get_window_path       (BobguiApplicationImplDBus      *dbus,
                                                                         BobguiWindow                   *window);

void                    bobgui_application_impl_quartz_setup_menu          (GMenuModel                  *model,
                                                                         BobguiActionMuxer              *muxer);

BobguiRestoreReason        bobgui_application_impl_get_restore_reason         (BobguiApplicationImpl          *impl);

void                    bobgui_application_impl_clear_restore_reason       (BobguiApplicationImpl          *impl);

void                    bobgui_application_impl_collect_global_state       (BobguiApplicationImpl          *impl,
                                                                         GVariantBuilder             *builder);
void                    bobgui_application_impl_restore_global_state       (BobguiApplicationImpl          *impl,
                                                                         GVariant                    *state);

void                    bobgui_application_impl_collect_window_state       (BobguiApplicationImpl          *impl,
                                                                         BobguiWindow                   *window,
                                                                         GVariantBuilder             *builder);

void                    bobgui_application_impl_store_state                (BobguiApplicationImpl          *impl,
                                                                         GVariant                    *state);
void                    bobgui_application_impl_forget_state               (BobguiApplicationImpl          *impl);
GVariant *              bobgui_application_impl_retrieve_state             (BobguiApplicationImpl          *impl);

GVariant *              bobgui_application_impl_dbus_get_window_state      (BobguiApplicationImplDBus *dbus,
                                                                         BobguiWindow              *window);

G_END_DECLS

