/* BOBGUI - The Bobgui Framework
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
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

/*
 * Modified by the BOBGUI Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the BOBGUI Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BOBGUI at ftp://ftp.bobgui.org/pub/bobgui/.
 */

#pragma once

#include <glib-object.h>
#include <gdk/gdk.h>
#include <gdk/gdkprivate.h>
#include <gdk/gdkdebugprivate.h>

#include "bobguicsstypesprivate.h"
#include "bobguitexthandleprivate.h"
#include "bobguiplacessidebarprivate.h"
#include "bobguieventcontrollerprivate.h"
#include "bobguiwindowgroup.h"
#include "bobguidebug.h"

G_BEGIN_DECLS

#define BOBGUI_PARAM_READABLE G_PARAM_READABLE|G_PARAM_STATIC_NAME|G_PARAM_STATIC_NICK|G_PARAM_STATIC_BLURB
#define BOBGUI_PARAM_WRITABLE G_PARAM_WRITABLE|G_PARAM_STATIC_NAME|G_PARAM_STATIC_NICK|G_PARAM_STATIC_BLURB
#define BOBGUI_PARAM_READWRITE G_PARAM_READWRITE|G_PARAM_STATIC_NAME|G_PARAM_STATIC_NICK|G_PARAM_STATIC_BLURB

#define OPPOSITE_ORIENTATION(_orientation) (1 - (_orientation))

#ifndef G_ENABLE_CONSISTENCY_CHECKS
/* This is true for buildtypes release and debugoptimized */
#define bobgui_internal_return_if_fail(__expr) G_STMT_START{ (void)0; }G_STMT_END
#define bobgui_internal_return_val_if_fail(__expr, __val) G_STMT_START{ (void)0; }G_STMT_END
#else
/* This is true for debug builds */
#define bobgui_internal_return_if_fail(__expr) g_return_if_fail(__expr)
#define bobgui_internal_return_val_if_fail(__expr, __val) g_return_val_if_fail(__expr, __val)
#endif

const char * _bobgui_get_datadir            (void) G_GNUC_CONST;
const char * _bobgui_get_libdir             (void) G_GNUC_CONST;
const char * _bobgui_get_sysconfdir         (void) G_GNUC_CONST;
const char * _bobgui_get_localedir          (void) G_GNUC_CONST;
const char * _bobgui_get_data_prefix        (void) G_GNUC_CONST;

gboolean      _bobgui_fnmatch                (const char *pattern,
                                           const char *string,
                                           gboolean    no_leading_period,
                                           gboolean    casefold);

char *        _bobgui_make_ci_glob_pattern   (const char *pattern);


char        * _bobgui_get_lc_ctype           (void) G_GNUC_MALLOC;

void          _bobgui_ensure_resources       (void);

void          bobgui_main_sync               (void);

BobguiWidget *   bobgui_window_group_get_current_grab (BobguiWindowGroup *window_group);
void          bobgui_grab_add                      (BobguiWidget      *widget);
void          bobgui_grab_remove                   (BobguiWidget      *widget);

gboolean _bobgui_boolean_handled_accumulator (GSignalInvocationHint *ihint,
                                           GValue                *return_accu,
                                           const GValue          *handler_return,
                                           gpointer               dummy);

gboolean _bobgui_single_string_accumulator   (GSignalInvocationHint *ihint,
                                           GValue                *return_accu,
                                           const GValue          *handler_return,
                                           gpointer               dummy);

gboolean         bobgui_propagate_event_internal  (BobguiWidget       *widget,
                                                GdkEvent        *event,
                                                BobguiWidget       *topmost);
gboolean   bobgui_propagate_event          (BobguiWidget       *widget,
                                         GdkEvent        *event);
gboolean   bobgui_main_do_event            (GdkEvent        *event);

BobguiWidget *bobgui_get_event_widget         (GdkEvent  *event);

guint32    bobgui_get_current_event_time   (void);

void check_crossing_invariants (BobguiWidget       *widget,
                                BobguiCrossingData *crossing);

double _bobgui_get_slowdown (void);
void    _bobgui_set_slowdown (double slowdown_factor);

char *bobgui_get_portal_request_path (GDBusConnection  *connection,
                                   char            **token) G_GNUC_MALLOC;
char *bobgui_get_portal_session_path (GDBusConnection  *connection,
                                   char            **token) G_GNUC_MALLOC;
guint bobgui_get_portal_interface_version (GDBusConnection *connection,
                                        const char      *interface_name);

#define PORTAL_BUS_NAME "org.freedesktop.portal.Desktop"
#define PORTAL_OBJECT_PATH "/org/freedesktop/portal/desktop"
#define PORTAL_REQUEST_INTERFACE "org.freedesktop.portal.Request"
#define PORTAL_SESSION_INTERFACE "org.freedesktop.portal.Session"
#define PORTAL_FILECHOOSER_INTERFACE "org.freedesktop.portal.FileChooser"
#define PORTAL_PRINT_INTERFACE "org.freedesktop.portal.Print"
#define PORTAL_SCREENSHOT_INTERFACE "org.freedesktop.portal.Screenshot"
#define PORTAL_INHIBIT_INTERFACE "org.freedesktop.portal.Inhibit"
#define PORTAL_OPENURI_INTERFACE "org.freedesktop.portal.OpenURI"

void            bobgui_set_display_debug_flags        (GdkDisplay    *display,
                                                    BobguiDebugFlags  flags);
BobguiDebugFlags   bobgui_get_display_debug_flags        (GdkDisplay    *display);
gboolean        bobgui_get_any_display_debug_flag_set (void);

GBytes *get_emoji_data (void);

#define BOBGUI_DISPLAY_DEBUG_CHECK(display,type)                   \
  (bobgui_get_any_display_debug_flag_set () &&                     \
   G_UNLIKELY (bobgui_get_display_debug_flags (display) & BOBGUI_DEBUG_##type))

#define BOBGUI_DEBUG(type,...)                                     \
  G_STMT_START {                                                \
    if (BOBGUI_DEBUG_CHECK (type))                                 \
      gdk_debug_message (__VA_ARGS__);                          \
  } G_STMT_END

#define BOBGUI_DISPLAY_DEBUG(display,type,...)                     \
  G_STMT_START {                                                \
    if (BOBGUI_DISPLAY_DEBUG_CHECK (display,type))                 \
      gdk_debug_message (__VA_ARGS__);                          \
  } G_STMT_END

char * _bobgui_elide_underscores (const char *original);

void setlocale_initialization (void);

void bobgui_synthesize_crossing_events (BobguiRoot         *toplevel,
                                     BobguiCrossingType  crossing_type,
                                     BobguiWidget       *old_target,
                                     BobguiWidget       *new_target,
                                     double           surface_x,
                                     double           surface_y,
                                     GdkCrossingMode  mode,
                                     GdkDrop         *drop);

#ifdef G_OS_WIN32

void _bobgui_load_dll_with_libbobgui3_manifest (const wchar_t *dllname);

#endif /* G_OS_WIN32 */

G_END_DECLS

