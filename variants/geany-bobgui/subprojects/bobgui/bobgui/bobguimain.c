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

#include "config.h"

#include "gdk/gdk.h"
#include "gdk/gdkdisplayprivate.h"
#include "gdk/gdkprofilerprivate.h"
#include "gdk/gdkdebugprivate.h"
#include "gsk/gskprivate.h"
#include "gsk/gskrendernodeprivate.h"
#include "bobguinative.h"

#include <locale.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <sys/types.h>          /* For uid_t, gid_t */

#ifdef G_OS_WIN32
#include <windows.h>
#endif

#include <hb-glib.h>

#include <glib/gi18n-lib.h>

#include "bobguidebug.h"
#include "bobguidropprivate.h"
#include "bobguimain.h"
#include "bobguimediafileprivate.h"
#include "bobguimodulesprivate.h"
#include "bobguiprivate.h"
#include "bobguirecentmanager.h"
#include "bobguitooltipprivate.h"
#include "bobguiwidgetprivate.h"
#include "bobguiwindowprivate.h"
#include "bobguiwindowgroup.h"
#include "print/bobguiprintbackendprivate.h"
#include "bobguiimmoduleprivate.h"
#include "bobguiroot.h"
#include "bobguinative.h"
#include "bobguipopcountprivate.h"

#include "inspector/init.h"
#include "inspector/window.h"

#include "gdk/gdkeventsprivate.h"
#include "gdk/gdksurfaceprivate.h"

#ifdef GDK_WINDOWING_WAYLAND
#include "gdk/wayland/gdkwaylandprivate.h"
#endif

#define GDK_ARRAY_ELEMENT_TYPE BobguiWidget *
#define GDK_ARRAY_TYPE_NAME BobguiWidgetStack
#define GDK_ARRAY_NAME bobgui_widget_stack
#define GDK_ARRAY_FREE_FUNC g_object_unref
#define GDK_ARRAY_PREALLOC 16
#include "gdk/gdkarrayimpl.c"

static BobguiWindowGroup *bobgui_main_get_window_group (BobguiWidget   *widget);

static int pre_initialized = FALSE;
static int bobgui_initialized = FALSE;
static GList *current_events = NULL;

typedef struct {
  GdkDisplay *display;
  guint flags;
} DisplayDebugFlags;

#define N_DEBUG_DISPLAYS 4

DisplayDebugFlags debug_flags[N_DEBUG_DISPLAYS];

/* This is a flag to speed up development builds. We set it to TRUE when
 * any of the debug displays has debug flags >0, but we never set it back
 * to FALSE. This way we don't need to call bobgui_widget_get_display() in
 * hot paths.
 */
gboolean any_display_debug_flags_set = FALSE;

BobguiDebugFlags
bobgui_get_display_debug_flags (GdkDisplay *display)
{
  int i;

  if (display == NULL)
    display = gdk_display_get_default ();

  for (i = 0; i < N_DEBUG_DISPLAYS; i++)
    {
      if (debug_flags[i].display == display)
        return (BobguiDebugFlags)debug_flags[i].flags;
    }

  return 0;
}

gboolean
bobgui_get_any_display_debug_flag_set (void)
{
  return any_display_debug_flags_set;
}

void
bobgui_set_display_debug_flags (GdkDisplay    *display,
                             BobguiDebugFlags  flags)
{
  int i;

  for (i = 0; i < N_DEBUG_DISPLAYS; i++)
    {
      if (debug_flags[i].display == NULL)
        debug_flags[i].display = display;

      if (debug_flags[i].display == display)
        {
          debug_flags[i].flags = flags;
          if (flags > 0)
            any_display_debug_flags_set = TRUE;

          return;
        }
    }
}

/**
 * bobgui_get_debug_flags:
 *
 * Returns the BOBGUI debug flags that are currently active.
 *
 * This function is intended for BOBGUI modules that want
 * to adjust their debug output based on BOBGUI debug flags.
 *
 * Returns: the BOBGUI debug flags.
 */
BobguiDebugFlags
bobgui_get_debug_flags (void)
{
  if (bobgui_get_any_display_debug_flag_set ())
    return bobgui_get_display_debug_flags (gdk_display_get_default ());

  return 0;
}

/**
 * bobgui_set_debug_flags:
 * @flags: the debug flags to set
 *
 * Sets the BOBGUI debug flags.
 */
void
bobgui_set_debug_flags (BobguiDebugFlags flags)
{
  bobgui_set_display_debug_flags (gdk_display_get_default (), flags);
}

static const GdkDebugKey bobgui_debug_keys[] = {
  { "keybindings", BOBGUI_DEBUG_KEYBINDINGS, "Information about keyboard shortcuts" },
  { "modules", BOBGUI_DEBUG_MODULES, "Information about modules and extensions" },
  { "icontheme", BOBGUI_DEBUG_ICONTHEME, "Information about icon themes" },
  { "printing", BOBGUI_DEBUG_PRINTING, "Information about printing" },
  { "geometry", BOBGUI_DEBUG_GEOMETRY, "Information about size allocation" },
  { "size-request", BOBGUI_DEBUG_SIZE_REQUEST, "Information about size requests" },
  { "actions", BOBGUI_DEBUG_ACTIONS, "Information about actions and menu models" },
  { "constraints", BOBGUI_DEBUG_CONSTRAINTS, "Information from the constraints solver" },
  { "text", BOBGUI_DEBUG_TEXT, "Information about BobguiTextView" },
  { "tree", BOBGUI_DEBUG_TREE, "Information about BobguiTreeView" },
  { "layout", BOBGUI_DEBUG_LAYOUT, "Information from layout managers" },
  { "builder-trace", BOBGUI_DEBUG_BUILDER_TRACE, "Trace BobguiBuilder operation" },
  { "builder-objects", BOBGUI_DEBUG_BUILDER_OBJECTS, "Log unused BobguiBuilder objects" },
  { "no-css-cache", BOBGUI_DEBUG_NO_CSS_CACHE, "Disable style property cache" },
  { "interactive", BOBGUI_DEBUG_INTERACTIVE, "Enable the BOBGUI inspector" },
  { "touch-ui", BOBGUI_DEBUG_TOUCHSCREEN, "Show touch ui elements for pointer events" },
  { "snapshot", BOBGUI_DEBUG_SNAPSHOT, "Generate debug render nodes" },
  { "accessibility", BOBGUI_DEBUG_A11Y, "Information about accessibility state changes" },
  { "iconfallback", BOBGUI_DEBUG_ICONFALLBACK, "Information about icon fallback" },
  { "invert-text-dir", BOBGUI_DEBUG_INVERT_TEXT_DIR, "Invert the default text direction" },
  { "css", BOBGUI_DEBUG_CSS, "Information about deprecated CSS features" },
  { "builder", BOBGUI_DEBUG_BUILDER, "Information about deprecated BobguiBuilder features" },
  { "session-mgmt", BOBGUI_DEBUG_SESSION, "Information about session saving" },
};

/* This checks to see if the process is running suid or sgid
 * at the current time. If so, we don’t allow BOBGUI to be initialized.
 * This is meant to be a mild check - we only error out if we
 * can prove the programmer is doing something wrong, not if
 * they could be doing something wrong. For this reason, we
 * don’t use issetugid() on BSD or prctl (PR_GET_DUMPABLE).
 */
static gboolean
check_setugid (void)
{
/* this isn't at all relevant on MS Windows and doesn't compile ... --hb */
#ifndef G_OS_WIN32
  uid_t ruid, euid, suid; /* Real, effective and saved user ID's */
  gid_t rgid, egid, sgid; /* Real, effective and saved group ID's */

#ifdef HAVE_GETRESUID
  if (getresuid (&ruid, &euid, &suid) != 0 ||
      getresgid (&rgid, &egid, &sgid) != 0)
#endif /* HAVE_GETRESUID */
    {
      suid = ruid = getuid ();
      sgid = rgid = getgid ();
      euid = geteuid ();
      egid = getegid ();
    }

  if (ruid != euid || ruid != suid ||
      rgid != egid || rgid != sgid)
    {
      g_warning ("This process is currently running setuid or setgid.\n"
                 "This is not a supported use of BOBGUI. You must create a helper\n"
                 "program instead. For further details, see:\n\n"
                 "    http://www.bobgui.org/setuid.html\n\n"
                 "Refusing to initialize BOBGUI.");
      exit (1);
    }
#endif
  return TRUE;
}

static gboolean do_setlocale = TRUE;

/**
 * bobgui_disable_setlocale:
 *
 * Prevents [func@Bobgui.init] and [func@Bobgui.init_check] from calling `setlocale()`.
 *
 * You would want to use this function if you wanted to set the locale for
 * your program to something other than the user’s locale, or if you wanted
 * to set different values for different locale categories.
 *
 * Most programs should not need to call this function.
 **/
void
bobgui_disable_setlocale (void)
{
  if (pre_initialized)
    g_warning ("bobgui_disable_setlocale() must be called before bobgui_init()");

  do_setlocale = FALSE;
}

/**
 * bobgui_disable_portals:
 *
 * Prevents BOBGUI from using portals.
 *
 * This is equivalent to setting `GDK_DEBUG=no-portals` in the environment.
 *
 * This should only be used in portal implementations, apps must not call it.
 *
 * Since: 4.18
 */
void
bobgui_disable_portals (void)
{
  if (pre_initialized)
    g_warning ("bobgui_disable_portals() must be called before bobgui_init()");

  gdk_disable_all_portals ();
}


/**
 * bobgui_disable_portal_interfaces:
 * @portal_interfaces: (array zero-terminated=1) (not nullable):
 *     a %NULL-terminated array of portal interface names to disable
 *
 * Prevents BOBGUI from using the specified portals.
 *
 * This should only be used in portal implementations, apps must not call it.
 *
 * Since: 4.22
 */
void
bobgui_disable_portal_interfaces (const char **portal_interfaces)
{
  if (pre_initialized)
    g_warning ("bobgui_disable_portal_interfaces() must be called before bobgui_init()");

  gdk_disable_portals (portal_interfaces);
}

#ifdef G_PLATFORM_WIN32
#undef bobgui_init_check
#endif

#ifdef G_OS_WIN32

static char *iso639_to_check = NULL;
static char *iso3166_to_check = NULL;
static char *script_to_check = NULL;
static gboolean setlocale_called = FALSE;

static BOOL CALLBACK
enum_locale_proc (LPSTR locale)
{
  LCID lcid;
  char iso639[10];
  char iso3166[10];
  char *endptr;


  lcid = strtoul (locale, &endptr, 16);
  if (*endptr == '\0' &&
      GetLocaleInfoA (lcid, LOCALE_SISO639LANGNAME, iso639, sizeof (iso639)) &&
      GetLocaleInfoA (lcid, LOCALE_SISO3166CTRYNAME, iso3166, sizeof (iso3166)))
    {
      if (strcmp (iso639, iso639_to_check) == 0 &&
          ((iso3166_to_check != NULL &&
            strcmp (iso3166, iso3166_to_check) == 0) ||
           (iso3166_to_check == NULL &&
            SUBLANGID (LANGIDFROMLCID (lcid)) == SUBLANG_DEFAULT)))
        {
          char language[100], country[100];

          if (script_to_check != NULL)
            {
              /* If lcid is the "other" script for this language,
               * return TRUE, i.e. continue looking.
               */
              if (strcmp (script_to_check, "Latn") == 0)
                {
                  switch (LANGIDFROMLCID (lcid))
                    {
                    case MAKELANGID (LANG_AZERI, SUBLANG_AZERI_CYRILLIC):
                      return TRUE;
                    case MAKELANGID (LANG_UZBEK, SUBLANG_UZBEK_CYRILLIC):
                      return TRUE;
                    case MAKELANGID (LANG_SERBIAN, SUBLANG_SERBIAN_CYRILLIC):
                      return TRUE;
                    case MAKELANGID (LANG_SERBIAN, 0x07):
                      /* Serbian in Bosnia and Herzegovina, Cyrillic */
                      return TRUE;
                    default:
                      break;
                    }
                }
              else if (strcmp (script_to_check, "Cyrl") == 0)
                {
                  switch (LANGIDFROMLCID (lcid))
                    {
                    case MAKELANGID (LANG_AZERI, SUBLANG_AZERI_LATIN):
                      return TRUE;
                    case MAKELANGID (LANG_UZBEK, SUBLANG_UZBEK_LATIN):
                      return TRUE;
                    case MAKELANGID (LANG_SERBIAN, SUBLANG_SERBIAN_LATIN):
                      return TRUE;
                    case MAKELANGID (LANG_SERBIAN, 0x06):
                      /* Serbian in Bosnia and Herzegovina, Latin */
                      return TRUE;
                    default:
                      break;
                    }
                }
            }

          SetThreadLocale (lcid);

          if (GetLocaleInfoA (lcid, LOCALE_SENGLANGUAGE, language, sizeof (language)) &&
              GetLocaleInfoA (lcid, LOCALE_SENGCOUNTRY, country, sizeof (country)))
            {
              char str[300];

              strcpy (str, language);
              strcat (str, "_");
              strcat (str, country);

              if (setlocale (LC_ALL, str) != NULL)
                setlocale_called = TRUE;
            }

          return FALSE;
        }
    }

  return TRUE;
}

#endif

void
setlocale_initialization (void)
{
  static gboolean initialized = FALSE;

  if (initialized)
    return;
  initialized = TRUE;

  if (do_setlocale)
    {
#ifdef G_OS_WIN32
      /* If some of the POSIXish environment variables are set, set
       * the Win32 thread locale correspondingly.
       */
      char *p = getenv ("LC_ALL");
      if (p == NULL)
        p = getenv ("LANG");

      if (p != NULL)
        {
          p = g_strdup (p);
          if (strcmp (p, "C") == 0)
            SetThreadLocale (LOCALE_SYSTEM_DEFAULT);
          else
            {
              /* Check if one of the supported locales match the
               * environment variable. If so, use that locale.
               */
              iso639_to_check = p;
              iso3166_to_check = strchr (iso639_to_check, '_');
              if (iso3166_to_check != NULL)
                {
                  *iso3166_to_check++ = '\0';

                  script_to_check = strchr (iso3166_to_check, '@');
                  if (script_to_check != NULL)
                    *script_to_check++ = '\0';

                  /* Handle special cases. */

                  /* The standard code for Serbia and Montenegro was
                   * "CS", but MSFT uses for some reason "SP". By now
                   * (October 2006), SP has split into two, "RS" and
                   * "ME", but don't bother trying to handle those
                   * yet. Do handle the even older "YU", though.
                   */
                  if (strcmp (iso3166_to_check, "CS") == 0 ||
                      strcmp (iso3166_to_check, "YU") == 0)
                    iso3166_to_check = (char *) "SP";
                }
              else
                {
                  script_to_check = strchr (iso639_to_check, '@');
                  if (script_to_check != NULL)
                    *script_to_check++ = '\0';
                  /* LANG_SERBIAN == LANG_CROATIAN, recognize just "sr" */
                  if (strcmp (iso639_to_check, "sr") == 0)
                    iso3166_to_check = (char *) "SP";
                }

              EnumSystemLocalesA (enum_locale_proc, LCID_SUPPORTED);
            }
          g_free (p);
        }
      if (!setlocale_called)
        setlocale (LC_ALL, "");
#else
      if (!setlocale (LC_ALL, ""))
        g_warning ("Locale not supported by C library.\n\tUsing the fallback 'C' locale.");
#endif
    }
}

/* Return TRUE if module_to_check causes version conflicts.
 * If module_to_check is NULL, check the main module.
 */
static gboolean
_bobgui_module_has_mixed_deps (GModule *module_to_check)
{
  GModule *module;
  gpointer func;
  gboolean result;

  if (!module_to_check)
    module = g_module_open (NULL, 0);
  else
    module = module_to_check;

  if (g_module_symbol (module, "bobgui_progress_get_type", &func))
    result = TRUE;
  else if (g_module_symbol (module, "bobgui_misc_get_type", &func))
    result = TRUE;
  else
    result = FALSE;

  if (!module_to_check)
    g_module_close (module);

  return result;
}

#ifdef GDK_WINDOWING_WAYLAND
static void
prepare_wayland_socket (void)
{
  char *wayland_socket;

  wayland_socket = getenv ("WAYLAND_SOCKET");
  if (wayland_socket && *wayland_socket)
    {
      char *end;
      int wayland_socket_fd;

      errno = 0;
      wayland_socket_fd = strtol (wayland_socket, &end, 10);
      if (errno != 0 || wayland_socket == end || *end != '\0')
        {
          g_warning ("Invalid WAYLAND_SOCKET");
          return;
        }

      unsetenv ("WAYLAND_SOCKET");
      gdk_wayland_set_wayland_socket (wayland_socket_fd);
    }
}
#endif

static void
do_pre_parse_initialization (void)
{
  const char *env_string;
  double slowdown;

  if (pre_initialized)
    return;

  pre_initialized = TRUE;

  if (_bobgui_module_has_mixed_deps (NULL))
    g_error ("BOBGUI 2/3 symbols detected. Using BOBGUI 2/3 and BOBGUI 4 in the same process is not supported");

#ifdef GDK_WINDOWING_WAYLAND
  prepare_wayland_socket ();
#endif

  gdk_pre_parse ();

  debug_flags[0].flags = gdk_parse_debug_var ("BOBGUI_DEBUG",
      "BOBGUI_DEBUG can be set to values that make BOBGUI print out different\n"
      "types of debugging information or change the behavior of BOBGUI for\n"
      "debugging purposes.\n",
      bobgui_debug_keys,
      G_N_ELEMENTS (bobgui_debug_keys));
  any_display_debug_flags_set = debug_flags[0].flags > 0;

  env_string = g_getenv ("BOBGUI_SLOWDOWN");
  if (env_string)
    {
      slowdown = g_ascii_strtod (env_string, NULL);
      _bobgui_set_slowdown (slowdown);
    }

  /* Trigger fontconfig initialization early */
  pango_cairo_font_map_get_default ();
}

static void
gettext_initialization (void)
{
  setlocale_initialization ();

  bindtextdomain (GETTEXT_PACKAGE, _bobgui_get_localedir ());
#ifdef HAVE_BIND_TEXTDOMAIN_CODESET
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
#endif
}

static void
default_display_notify_cb (GdkDisplayManager *dm)
{
  debug_flags[0].display = gdk_display_get_default ();
}

static void
do_post_parse_initialization (void)
{
  GdkDisplayManager *display_manager;
  BobguiTextDirection text_dir;
  gint64 before G_GNUC_UNUSED;

  if (bobgui_initialized)
    return;

  before = GDK_PROFILER_CURRENT_TIME;

  gettext_initialization ();

#ifdef SIGPIPE
  signal (SIGPIPE, SIG_IGN);
#endif

  text_dir = bobgui_get_locale_direction ();

  /* We always allow inverting the text direction, even in
   * production builds, as SDK/IDE tooling may provide the
   * ability for developers to test rtl/ltr inverted.
   */
  if (bobgui_get_debug_flags () & BOBGUI_DEBUG_INVERT_TEXT_DIR)
    text_dir = (text_dir == BOBGUI_TEXT_DIR_LTR) ? BOBGUI_TEXT_DIR_RTL : BOBGUI_TEXT_DIR_LTR;

  bobgui_widget_set_default_direction (text_dir);

  gdk_event_init_types ();

  gsk_ensure_resources ();
  gsk_render_node_init_types ();
  _bobgui_ensure_resources ();

  gdk_profiler_end_mark (before, "Basic initialization", NULL);

  bobgui_initialized = TRUE;

  before = GDK_PROFILER_CURRENT_TIME;
#ifdef G_OS_UNIX
  bobgui_print_backends_init ();
#endif
  bobgui_im_modules_init ();
  bobgui_media_file_extension_init ();
  gdk_profiler_end_mark (before, "Init modules", NULL);

  before = GDK_PROFILER_CURRENT_TIME;
  display_manager = gdk_display_manager_get ();
  if (gdk_display_manager_get_default_display (display_manager) != NULL)
    default_display_notify_cb (display_manager);
  gdk_profiler_end_mark (before, "Create display", NULL);

  g_signal_connect (display_manager, "notify::default-display",
                    G_CALLBACK (default_display_notify_cb),
                    NULL);

  bobgui_inspector_register_extension ();
}

#ifdef G_PLATFORM_WIN32
#undef bobgui_init_check
#endif

/**
 * bobgui_init_check:
 *
 * Initializes BOBGUI.
 *
 * This function does the same work as [func@Bobgui.init] with only a
 * single change: It does not terminate the program if the windowing
 * system can’t be initialized. Instead it returns false on failure.
 *
 * This way the application can fall back to some other means of
 * communication with the user - for example a curses or command line
 * interface.
 *
 * Returns: true if the windowing system has been successfully
 *   initialized, false otherwise
 */
gboolean
bobgui_init_check (void)
{
  gboolean ret;

  if (bobgui_initialized)
    return TRUE;

  if (gdk_profiler_is_running ())
    g_info ("Profiling is active");

  gettext_initialization ();

  if (!check_setugid ())
    return FALSE;

  do_pre_parse_initialization ();
  do_post_parse_initialization ();

  ret = gdk_display_open_default () != NULL;

  if (ret && (bobgui_get_debug_flags () & BOBGUI_DEBUG_INTERACTIVE))
    bobgui_window_set_interactive_debugging (TRUE);

  return ret;
}

#ifdef G_PLATFORM_WIN32
#undef bobgui_init
#endif

/**
 * bobgui_init:
 *
 * Initializes BOBGUI.
 *
 * This function must be called before using any other BOBGUI functions
 * in your GUI applications.
 *
 * It will initialize everything needed to operate the toolkit. In particular,
 * it will open the default display (see [func@Gdk.Display.get_default]).
 *
 * If you are using [class@Bobgui.Application], you usually don't have to call this
 * function; the [vfunc@Gio.Application.startup] handler does it for you. Though,
 * if you are using `GApplication` methods that will be invoked before `startup`,
 * such as `local_command_line`, you may need to initialize BOBGUI explicitly.
 *
 * This function will terminate your program if it was unable to initialize
 * the windowing system for some reason. If you want your program to fall back
 * to a textual interface, call [func@Bobgui.init_check] instead.
 *
 * BOBGUI calls `signal (SIGPIPE, SIG_IGN)` during initialization, to ignore
 * SIGPIPE signals, since these are almost never wanted in graphical
 * applications. If you do need to handle SIGPIPE for some reason, reset
 * the handler after bobgui_init(), but notice that other libraries (e.g.
 * libdbus or gvfs) might do similar things.
 */
void
bobgui_init (void)
{
  if (!bobgui_init_check ())
    {
      g_warning ("Failed to open display");
      exit (1);
    }
}

#ifdef G_OS_WIN32

/* This is relevant when building with gcc for Windows (MinGW),
 * where we want to be struct packing compatible with MSVC,
 * i.e. use the -mms-bitfields switch.
 * For Cygwin there should be no need to be compatible with MSVC,
 * so no need to use G_PLATFORM_WIN32.
 */

static void
check_sizeof_BobguiWindow (size_t sizeof_BobguiWindow)
{
  if (sizeof_BobguiWindow != sizeof (BobguiWindow))
    g_error ("Incompatible build!\n"
             "The code using BOBGUI thinks BobguiWindow is of different\n"
             "size than it actually is in this build of BOBGUI.\n"
             "On Windows, this probably means that you have compiled\n"
             "your code with gcc without the -mms-bitfields switch,\n"
             "or that you are using an unsupported compiler.");
}

/* In BOBGUI 2.0 the BobguiWindow struct actually is the same size in
 * gcc-compiled code on Win32 whether compiled with -fnative-struct or
 * not. Unfortunately this wasn’t noticed until after BOBGUI 2.0.1. So,
 * from BOBGUI 2.0.2 on, check some other struct, too, where the use of
 * -fnative-struct still matters. BobguiBox is one such.
 */
static void
check_sizeof_BobguiBox (size_t sizeof_BobguiBox)
{
  if (sizeof_BobguiBox != sizeof (BobguiBox))
    g_error ("Incompatible build!\n"
             "The code using BOBGUI thinks BobguiBox is of different\n"
             "size than it actually is in this build of BOBGUI.\n"
             "On Windows, this probably means that you have compiled\n"
             "your code with gcc without the -mms-bitfields switch,\n"
             "or that you are using an unsupported compiler.");
}

/* These two functions might get more checks added later, thus pass
 * in the number of extra args.
 */
void
bobgui_init_abi_check (int num_checks, size_t sizeof_BobguiWindow, size_t sizeof_BobguiBox)
{
  check_sizeof_BobguiWindow (sizeof_BobguiWindow);
  if (num_checks >= 2)
    check_sizeof_BobguiBox (sizeof_BobguiBox);
  bobgui_init ();
}

gboolean
bobgui_init_check_abi_check (int num_checks, size_t sizeof_BobguiWindow, size_t sizeof_BobguiBox)
{
  check_sizeof_BobguiWindow (sizeof_BobguiWindow);
  if (num_checks >= 2)
    check_sizeof_BobguiBox (sizeof_BobguiBox);
  return bobgui_init_check ();
}

#endif

/**
 * bobgui_is_initialized:
 *
 * Returns whether BOBGUI has been initialized.
 *
 * See [func@Bobgui.init].
 *
 * Returns: the initialization status
 */
gboolean
bobgui_is_initialized (void)
{
  return bobgui_initialized;
}


/**
 * bobgui_get_locale_direction:
 *
 * Gets the direction of the current locale.
 *
 * This is the expected reading direction for text and UI.
 *
 * This function depends on the current locale being set with
 * `setlocale()` and will default to setting the `BOBGUI_TEXT_DIR_LTR`
 * direction otherwise. `BOBGUI_TEXT_DIR_NONE` will never be returned.
 *
 * BOBGUI sets the default text direction according to the locale during
 * [func@Bobgui.init], and you should normally use [method@Bobgui.Widget.get_direction]
 * or [func@Bobgui.Widget.get_default_direction] to obtain the current direction.
 *
 * This function is only needed rare cases when the locale is
 * changed after BOBGUI has already been initialized. In this case,
 * you can use it to update the default text direction as follows:
 *
 * ```c
 * #include <locale.h>
 *
 * static void
 * update_locale (const char *new_locale)
 * {
 *   setlocale (LC_ALL, new_locale);
 *   bobgui_widget_set_default_direction (bobgui_get_locale_direction ());
 * }
 * ```
 *
 * Returns: the direction of the current locale
 */
BobguiTextDirection
bobgui_get_locale_direction (void)
{
  PangoLanguage *language;
  const PangoScript *scripts;
  int n_scripts;

  language = bobgui_get_default_language ();
  scripts = pango_language_get_scripts (language, &n_scripts);

  if (n_scripts > 0)
    {
      for (int i = 0; i < n_scripts; i++)
        {
          hb_script_t script;

          script = hb_glib_script_to_script ((GUnicodeScript) scripts[i]);

          switch (hb_script_get_horizontal_direction (script))
            {
            case HB_DIRECTION_LTR:
              return BOBGUI_TEXT_DIR_LTR;
            case HB_DIRECTION_RTL:
              return BOBGUI_TEXT_DIR_RTL;
            case HB_DIRECTION_TTB:
            case HB_DIRECTION_BTT:
            case HB_DIRECTION_INVALID:
            default:
              break;
            }
        }
    }

  return BOBGUI_TEXT_DIR_LTR;
}

/**
 * bobgui_get_default_language:
 *
 * Returns the `PangoLanguage` for the default language
 * currently in effect.
 *
 * Note that this can change over the life of an
 * application.
 *
 * The default language is derived from the current
 * locale. It determines, for example, whether BOBGUI uses
 * the right-to-left or left-to-right text direction.
 *
 * This function is equivalent to [func@Pango.Language.get_default].
 * See that function for details.
 *
 * Returns: (transfer none): the default language
 */
PangoLanguage *
bobgui_get_default_language (void)
{
  return pango_language_get_default ();
}

typedef struct {
  GMainLoop *store_loop;
  guint n_clipboards;
  guint timeout_id;
} ClipboardStore;

static void
clipboard_store_finished (GObject      *source,
                          GAsyncResult *result,
                          gpointer      data)
{
  ClipboardStore *store;
  GError *error = NULL;

  if (!gdk_clipboard_store_finish (GDK_CLIPBOARD (source), result, &error))
    {
      if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
        {
          g_error_free (error);
          return;
        }

      g_error_free (error);
    }

  store = data;
  store->n_clipboards--;
  if (store->n_clipboards == 0)
    g_main_loop_quit (store->store_loop);
}

static gboolean
sync_timed_out_cb (ClipboardStore *store)
{
  store->timeout_id = 0;
  g_main_loop_quit (store->store_loop);
  return G_SOURCE_REMOVE;
}

void
bobgui_main_sync (void)
{
  ClipboardStore store = { NULL, };
  GSList *displays, *l;
  GCancellable *cancel;

  /* Try storing all clipboard data we have */
  displays = gdk_display_manager_list_displays (gdk_display_manager_get ());
  if (displays == NULL)
    return;

  cancel = g_cancellable_new ();

  for (l = displays; l; l = l->next)
    {
      GdkDisplay *display = l->data;
      GdkClipboard *clipboard = gdk_display_get_clipboard (display);

      gdk_clipboard_store_async (clipboard,
                                 G_PRIORITY_HIGH,
                                 cancel,
                                 clipboard_store_finished,
                                 &store);
      store.n_clipboards++;
    }
  g_slist_free (displays);

  store.store_loop = g_main_loop_new (NULL, TRUE);
  store.timeout_id = g_timeout_add_seconds (10, (GSourceFunc) sync_timed_out_cb, &store);
  gdk_source_set_static_name_by_id (store.timeout_id, "[bobgui] bobgui_main_sync clipboard store timeout");

  if (g_main_loop_is_running (store.store_loop))
    g_main_loop_run (store.store_loop);

  g_cancellable_cancel (cancel);
  g_object_unref (cancel);
  g_clear_handle_id (&store.timeout_id, g_source_remove);
  g_clear_pointer (&store.store_loop, g_main_loop_unref);

  /* Synchronize the recent manager singleton */
  _bobgui_recent_manager_sync ();
}

static GdkEvent *
rewrite_event_for_surface (GdkEvent  *event,
			   GdkSurface *new_surface)
{
  GdkEventType type;
  double x = -G_MAXDOUBLE, y = -G_MAXDOUBLE;
  double dx, dy;

  type = gdk_event_get_event_type (event);

  switch ((guint) type)
    {
    case GDK_BUTTON_PRESS:
    case GDK_BUTTON_RELEASE:
      return gdk_button_event_new (type,
                                   new_surface,
                                   gdk_event_get_device (event),
                                   gdk_event_get_device_tool (event),
                                   gdk_event_get_time (event),
                                   gdk_event_get_modifier_state (event),
                                   gdk_button_event_get_button (event),
                                   x, y,
                                   gdk_event_dup_axes (event));
    case GDK_MOTION_NOTIFY:
      return gdk_motion_event_new (new_surface,
                                   gdk_event_get_device (event),
                                   gdk_event_get_device_tool (event),
                                   gdk_event_get_time (event),
                                   gdk_event_get_modifier_state (event),
                                   x, y,
                                   gdk_event_dup_axes (event));
    case GDK_TOUCH_BEGIN:
    case GDK_TOUCH_UPDATE:
    case GDK_TOUCH_END:
    case GDK_TOUCH_CANCEL:
      return gdk_touch_event_new (type,
                                  gdk_event_get_event_sequence (event),
                                  new_surface,
                                  gdk_event_get_device (event),
                                  gdk_event_get_time (event),
                                  gdk_event_get_modifier_state (event),
                                  x, y,
                                  gdk_event_dup_axes (event),
                                  gdk_touch_event_get_emulating_pointer (event));
    case GDK_TOUCHPAD_SWIPE:
      gdk_touchpad_event_get_deltas (event, &dx, &dy);
      return gdk_touchpad_event_new_swipe (new_surface,
                                           gdk_event_get_event_sequence (event),
                                           gdk_event_get_device (event),
                                           gdk_event_get_time (event),
                                           gdk_event_get_modifier_state (event),
                                           gdk_touchpad_event_get_gesture_phase (event),
                                           x, y,
                                           gdk_touchpad_event_get_n_fingers (event),
                                           dx, dy);
    case GDK_TOUCHPAD_PINCH:
      gdk_touchpad_event_get_deltas (event, &dx, &dy);
      return gdk_touchpad_event_new_pinch (new_surface,
                                           gdk_event_get_event_sequence (event),
                                           gdk_event_get_device (event),
                                           gdk_event_get_time (event),
                                           gdk_event_get_modifier_state (event),
                                           gdk_touchpad_event_get_gesture_phase (event),
                                           x, y,
                                           gdk_touchpad_event_get_n_fingers (event),
                                           dx, dy,
                                           gdk_touchpad_event_get_pinch_scale (event),
                                           gdk_touchpad_event_get_pinch_angle_delta (event));
    case GDK_TOUCHPAD_HOLD:
      return gdk_touchpad_event_new_hold (new_surface,
                                          gdk_event_get_event_sequence (event),
                                          gdk_event_get_device (event),
                                          gdk_event_get_time (event),
                                          gdk_event_get_modifier_state (event),
                                          gdk_touchpad_event_get_gesture_phase (event),
                                          x, y,
                                          gdk_touchpad_event_get_n_fingers (event));
    default:
      break;
    }

  return NULL;
}

/* If there is a pointer or keyboard grab in effect with owner_events = TRUE,
 * then what X11 does is deliver the event normally if it was going to this
 * client, otherwise, delivers it in terms of the grab surface. This function
 * rewrites events to the effect that events going to the same window group
 * are delivered normally, otherwise, the event is delivered in terms of the
 * grab window.
 */
static GdkEvent *
rewrite_event_for_grabs (GdkEvent *event)
{
  GdkSurface *grab_surface;
  BobguiWidget *event_widget, *grab_widget;
  gboolean owner_events;
  GdkDisplay *display;
  GdkDevice *device;

  switch ((guint) gdk_event_get_event_type (event))
    {
    case GDK_SCROLL:
    case GDK_BUTTON_PRESS:
    case GDK_BUTTON_RELEASE:
    case GDK_MOTION_NOTIFY:
    case GDK_PROXIMITY_IN:
    case GDK_PROXIMITY_OUT:
    case GDK_KEY_PRESS:
    case GDK_KEY_RELEASE:
    case GDK_TOUCH_BEGIN:
    case GDK_TOUCH_UPDATE:
    case GDK_TOUCH_END:
    case GDK_TOUCH_CANCEL:
    case GDK_TOUCHPAD_SWIPE:
    case GDK_TOUCHPAD_PINCH:
    case GDK_TOUCHPAD_HOLD:
      display = gdk_event_get_display (event);
      device = gdk_event_get_device (event);

      if (!gdk_device_grab_info (display, device, &grab_surface, &owner_events))
        return NULL;
      break;
    default:
      return NULL;
    }

  event_widget = bobgui_get_event_widget (event);
  grab_widget = BOBGUI_WIDGET (bobgui_native_get_for_surface (grab_surface));

  if (!grab_widget)
    return NULL;

  /* If owner_events was set, events in client surfaces get forwarded
   * as normal, but we consider other window groups foreign surfaces.
   */
  if (owner_events &&
      bobgui_main_get_window_group (grab_widget) == bobgui_main_get_window_group (event_widget))
    return NULL;

  /* If owner_events was not set, events only get sent to the grabbing
   * surface.
   */
  if (!owner_events &&
      grab_surface == bobgui_native_get_surface (bobgui_widget_get_native (event_widget)))
    return NULL;

  return rewrite_event_for_surface (event, grab_surface);
}

static GdkEvent *
rewrite_event_for_toplevel (GdkEvent *event)
{
  GdkSurface *surface;
  GdkEventType event_type;
  GdkTranslatedKey *key, *key_no_lock;

  surface = gdk_event_get_surface (event);
  if (!surface->parent)
    return NULL;

  event_type = gdk_event_get_event_type (event);
  if (event_type != GDK_KEY_PRESS &&
      event_type != GDK_KEY_RELEASE)
    return NULL;

  while (surface->parent)
    surface = surface->parent;

  key = gdk_key_event_get_translated_key (event, FALSE);
  key_no_lock = gdk_key_event_get_translated_key (event, TRUE);

  return gdk_key_event_new (gdk_event_get_event_type (event),
                            surface,
                            gdk_event_get_device (event),
                            gdk_event_get_time (event),
                            gdk_key_event_get_keycode (event),
                            gdk_event_get_modifier_state (event),
                            gdk_key_event_is_modifier (event),
                            key, key_no_lock,
                            gdk_key_event_get_compose_sequence (event));
}

static gboolean
translate_coordinates (double     event_x,
                       double     event_y,
                       double    *x,
                       double    *y,
                       BobguiWidget *widget)
{
  BobguiNative *native;
  graphene_point_t p;

  *x = *y = 0;
  native = bobgui_widget_get_native (widget);

  if (!bobgui_widget_compute_point (BOBGUI_WIDGET (native),
                                 widget,
                                 &GRAPHENE_POINT_INIT (event_x, event_y),
                                 &p))
    return FALSE;

  *x = p.x;
  *y = p.y;

  return TRUE;
}

void
bobgui_synthesize_crossing_events (BobguiRoot         *toplevel,
                                BobguiCrossingType  crossing_type,
                                BobguiWidget       *old_target,
                                BobguiWidget       *new_target,
                                double           surface_x,
                                double           surface_y,
                                GdkCrossingMode  mode,
                                GdkDrop         *drop)
{
  BobguiCrossingData crossing;
  BobguiWidget *ancestor;
  BobguiWidget *widget;
  double x, y;
  BobguiWidget *prev;
  gboolean seen_ancestor;
  BobguiWidgetStack target_array;
  int i;

  if (old_target == new_target)
    return;

  if (old_target && new_target)
    ancestor = bobgui_widget_common_ancestor (old_target, new_target);
  else
    ancestor = NULL;

  crossing.type = crossing_type;
  crossing.mode = mode;
  crossing.old_target = old_target ? g_object_ref (old_target) : NULL;
  crossing.old_descendent = NULL;
  crossing.new_target = new_target ? g_object_ref (new_target) : NULL;
  crossing.new_descendent = NULL;
  crossing.drop = drop;

  crossing.direction = BOBGUI_CROSSING_OUT;

  prev = NULL;
  seen_ancestor = FALSE;
  widget = old_target;
  while (widget)
    {
      crossing.old_descendent = prev;
      if (seen_ancestor)
        {
          crossing.new_descendent = new_target ? prev : NULL;
        }
      else if (widget == ancestor)
        {
          BobguiWidget *w;

          crossing.new_descendent = NULL;
          for (w = new_target; w != ancestor; w = _bobgui_widget_get_parent (w))
            crossing.new_descendent = w;

          seen_ancestor = TRUE;
        }
      else
        {
          crossing.new_descendent = NULL;
        }
      check_crossing_invariants (widget, &crossing);
      translate_coordinates (surface_x, surface_y, &x, &y, widget);
      bobgui_widget_handle_crossing (widget, &crossing, x, y);
      if (crossing_type == BOBGUI_CROSSING_POINTER)
        bobgui_widget_unset_state_flags (widget, BOBGUI_STATE_FLAG_PRELIGHT);
      prev = widget;
      widget = _bobgui_widget_get_parent (widget);
    }

  bobgui_widget_stack_init (&target_array);
  for (widget = new_target; widget; widget = _bobgui_widget_get_parent (widget))
    bobgui_widget_stack_append (&target_array, g_object_ref (widget));

  crossing.direction = BOBGUI_CROSSING_IN;

  seen_ancestor = FALSE;
  for (i = bobgui_widget_stack_get_size (&target_array) - 1; i >= 0; i--)
    {
      widget = bobgui_widget_stack_get (&target_array, i);

      if (i > 0)
        crossing.new_descendent = bobgui_widget_stack_get (&target_array, i - 1);
      else
        crossing.new_descendent = NULL;

      if (seen_ancestor)
        {
          crossing.old_descendent = NULL;
        }
      else if (widget == ancestor)
        {
          BobguiWidget *w;

          crossing.old_descendent = NULL;
          for (w = old_target; w != ancestor; w = _bobgui_widget_get_parent (w))
            crossing.old_descendent = w;

          seen_ancestor = TRUE;
        }
      else
        {
          crossing.old_descendent = (old_target && ancestor) ? crossing.new_descendent : NULL;
        }

      check_crossing_invariants (widget, &crossing);
      translate_coordinates (surface_x, surface_y, &x, &y, widget);
      bobgui_widget_handle_crossing (widget, &crossing, x, y);
      if (crossing_type == BOBGUI_CROSSING_POINTER)
        bobgui_widget_set_state_flags (widget, BOBGUI_STATE_FLAG_PRELIGHT, FALSE);
    }

  g_clear_object (&crossing.old_target);
  g_clear_object (&crossing.new_target);

  bobgui_widget_stack_clear (&target_array);
}

static BobguiWidget *
update_pointer_focus_state (BobguiWindow *toplevel,
                            GdkEvent  *event,
                            BobguiWidget *new_target)
{
  BobguiWidget *old_target = NULL;
  GdkEventSequence *sequence;
  GdkDevice *device;
  BobguiWidget *event_widget;
  graphene_point_t p;
  double x, y;
  double nx, ny;

  device = gdk_event_get_device (event);
  sequence = gdk_event_get_event_sequence (event);
  old_target = bobgui_window_lookup_pointer_focus_widget (toplevel, device, sequence);
  if (old_target == new_target)
    return old_target;

  gdk_event_get_position (event, &x, &y);
  p = GRAPHENE_POINT_INIT (x, y);

  event_widget  = bobgui_get_event_widget (event);
  if (!bobgui_widget_compute_point (event_widget, BOBGUI_WIDGET (toplevel), &p, &p))
    return old_target;

  bobgui_native_get_surface_transform (BOBGUI_NATIVE (toplevel), &nx, &ny);
  p.x -= nx;
  p.y -= ny;

  bobgui_window_update_pointer_focus (toplevel, device, sequence,
                                   new_target, p.x, p.y);

  return old_target;
}

static gboolean
is_pointing_event (GdkEvent *event)
{
  switch ((guint) gdk_event_get_event_type (event))
    {
    case GDK_MOTION_NOTIFY:
    case GDK_ENTER_NOTIFY:
    case GDK_LEAVE_NOTIFY:
    case GDK_BUTTON_PRESS:
    case GDK_BUTTON_RELEASE:
    case GDK_SCROLL:
    case GDK_TOUCH_BEGIN:
    case GDK_TOUCH_UPDATE:
    case GDK_TOUCH_END:
    case GDK_TOUCH_CANCEL:
    case GDK_TOUCHPAD_PINCH:
    case GDK_TOUCHPAD_SWIPE:
    case GDK_TOUCHPAD_HOLD:
    case GDK_DRAG_ENTER:
    case GDK_DRAG_LEAVE:
    case GDK_DRAG_MOTION:
    case GDK_DROP_START:
      return TRUE;

    case GDK_GRAB_BROKEN:
      return gdk_device_get_source (gdk_event_get_device (event)) != GDK_SOURCE_KEYBOARD;

    default:
      return FALSE;
    }
}

static gboolean
is_key_event (GdkEvent *event)
{
  switch ((guint) gdk_event_get_event_type (event))
    {
    case GDK_KEY_PRESS:
    case GDK_KEY_RELEASE:
      return TRUE;
      break;
    case GDK_GRAB_BROKEN:
      return gdk_device_get_source (gdk_event_get_device (event)) == GDK_SOURCE_KEYBOARD;
    default:
      return FALSE;
    }
}

static BobguiWidget *
handle_pointing_event (GdkEvent *event)
{
  BobguiWidget *target = NULL, *old_target = NULL, *event_widget;
  BobguiWindow *toplevel;
  GdkEventSequence *sequence;
  GdkDevice *device;
  double x, y;
  double native_x, native_y;
  BobguiWidget *native;
  GdkEventType type;
  gboolean has_implicit;
  GdkModifierType modifiers;

  event_widget = bobgui_get_event_widget (event);
  device = gdk_event_get_device (event);
  gdk_event_get_position (event, &x, &y);

  toplevel = BOBGUI_WINDOW (bobgui_widget_get_root (event_widget));
  native = BOBGUI_WIDGET (bobgui_widget_get_native (event_widget));

  bobgui_native_get_surface_transform (BOBGUI_NATIVE (native), &native_x, &native_y);
  x -= native_x;
  y -= native_y;

  type = gdk_event_get_event_type (event);
  sequence = gdk_event_get_event_sequence (event);

  if (type == GDK_SCROLL && !gdk_event_get_device_tool (event))
    {
      /* A bit of a kludge, resolve target lookups for scrolling devices
       * on the seat pointer.
       */
      device = gdk_seat_get_pointer (gdk_event_get_seat (event));
    }
  else if (type == GDK_TOUCHPAD_PINCH ||
           type == GDK_TOUCHPAD_SWIPE ||
           type == GDK_TOUCHPAD_HOLD)
    {
      /* Another bit of a kludge, touchpad gesture sequences do not
       * reflect on the pointer focus lookup.
       */
      sequence = NULL;
    }

  switch ((guint) type)
    {
    case GDK_LEAVE_NOTIFY:
      if (gdk_crossing_event_get_mode (event) == GDK_CROSSING_GRAB)
        {
          BobguiWidget *grab_widget;

          grab_widget =
            bobgui_window_lookup_pointer_focus_implicit_grab (toplevel,
                                                           device,
                                                           sequence);
          if (grab_widget)
            {
              bobgui_window_set_pointer_focus_grab (toplevel, device,
                                                 sequence, NULL);
            }
        }

      old_target = update_pointer_focus_state (toplevel, event, NULL);
      bobgui_synthesize_crossing_events (BOBGUI_ROOT (toplevel), BOBGUI_CROSSING_POINTER, old_target, NULL,
                                      x, y, gdk_crossing_event_get_mode (event), NULL);
      break;
    case GDK_TOUCH_END:
    case GDK_TOUCH_CANCEL:
      old_target = update_pointer_focus_state (toplevel, event, NULL);
      bobgui_window_set_pointer_focus_grab (toplevel, device,
                                         sequence, NULL);
      break;
    case GDK_DRAG_LEAVE:
      {
        GdkDrop *drop = gdk_dnd_event_get_drop (event);
        old_target = update_pointer_focus_state (toplevel, event, NULL);
        bobgui_drop_begin_event (drop, GDK_DRAG_LEAVE);
        bobgui_synthesize_crossing_events (BOBGUI_ROOT (toplevel), BOBGUI_CROSSING_DROP, old_target, NULL,
                                        x, y, GDK_CROSSING_NORMAL, drop);
        bobgui_drop_end_event (drop);
      }
      break;
    case GDK_ENTER_NOTIFY:
    case GDK_DRAG_ENTER:
    case GDK_DRAG_MOTION:
    case GDK_DROP_START:
    case GDK_TOUCH_BEGIN:
    case GDK_TOUCH_UPDATE:
    case GDK_MOTION_NOTIFY:
      target = bobgui_window_lookup_pointer_focus_implicit_grab (toplevel, device, sequence);

      if (!target)
        target = bobgui_widget_pick (native, x, y, BOBGUI_PICK_DEFAULT);

      if (!target)
        target = BOBGUI_WIDGET (native);

      old_target = update_pointer_focus_state (toplevel, event, target);

      if (type == GDK_MOTION_NOTIFY || type == GDK_ENTER_NOTIFY)
        {
          if (!bobgui_window_lookup_pointer_focus_implicit_grab (toplevel, device,
                                                              sequence))
            {
              bobgui_synthesize_crossing_events (BOBGUI_ROOT (toplevel), BOBGUI_CROSSING_POINTER, old_target, target,
                                              x, y, GDK_CROSSING_NORMAL, NULL);
            }

          bobgui_window_maybe_update_cursor (toplevel, NULL, device);
        }
      else if ((old_target != target) &&
               (type == GDK_DRAG_ENTER || type == GDK_DRAG_MOTION || type == GDK_DROP_START))
        {
          GdkDrop *drop = gdk_dnd_event_get_drop (event);
          bobgui_drop_begin_event (drop, type);
          bobgui_synthesize_crossing_events (BOBGUI_ROOT (toplevel), BOBGUI_CROSSING_DROP, old_target, target,
                                          x, y, GDK_CROSSING_NORMAL, gdk_dnd_event_get_drop (event));
          bobgui_drop_end_event (drop);
        }
      else if (type == GDK_TOUCH_BEGIN)
        {
          bobgui_window_set_pointer_focus_grab (toplevel, device, sequence, target);
        }

      /* Let it take the effective pointer focus anyway, as it may change due
       * to implicit grabs.
       */
      target = NULL;
      break;
    case GDK_BUTTON_PRESS:
    case GDK_BUTTON_RELEASE:
      target = bobgui_window_lookup_effective_pointer_focus_widget (toplevel,
                                                                 device,
                                                                 sequence);
      has_implicit =
        bobgui_window_lookup_pointer_focus_implicit_grab (toplevel,
                                                       device,
                                                       sequence) != NULL;
      modifiers = gdk_event_get_modifier_state (event);

      if (type == GDK_BUTTON_RELEASE &&
          bobgui_popcount (modifiers & (GDK_BUTTON1_MASK |
                                     GDK_BUTTON2_MASK |
                                     GDK_BUTTON3_MASK |
                                     GDK_BUTTON4_MASK |
                                     GDK_BUTTON5_MASK)) == 1)
        {
          BobguiWidget *new_target = bobgui_widget_pick (native, x, y, BOBGUI_PICK_DEFAULT);

          bobgui_window_set_pointer_focus_grab (toplevel, device, sequence, NULL);

          if (new_target == NULL)
            new_target = BOBGUI_WIDGET (toplevel);

          bobgui_synthesize_crossing_events (BOBGUI_ROOT (toplevel), BOBGUI_CROSSING_POINTER, target, new_target,
                                          x, y, GDK_CROSSING_UNGRAB, NULL);
          bobgui_window_maybe_update_cursor (toplevel, NULL, device);
          update_pointer_focus_state (toplevel, event, new_target);
        }
      else if (type == GDK_BUTTON_PRESS &&
               !has_implicit &&
               (modifiers & (GDK_BUTTON1_MASK |
                             GDK_BUTTON2_MASK |
                             GDK_BUTTON3_MASK |
                             GDK_BUTTON4_MASK |
                             GDK_BUTTON5_MASK)) == 0)
        {
          bobgui_window_set_pointer_focus_grab (toplevel, device,
                                             sequence, target);
        }

      break;
    case GDK_SCROLL:
    case GDK_TOUCHPAD_PINCH:
    case GDK_TOUCHPAD_SWIPE:
    case GDK_TOUCHPAD_HOLD:
      break;
    case GDK_GRAB_BROKEN:
      if (gdk_grab_broken_event_get_implicit (event))
        {
          target = bobgui_window_lookup_effective_pointer_focus_widget (toplevel,
                                                                     device,
                                                                     sequence);
          if (target)
            {
              bobgui_window_set_pointer_focus_grab (toplevel, device,
                                                 sequence, NULL);
            }
        }
      break;
    default:
      g_assert_not_reached ();
    }

  if (!target)
    target = bobgui_window_lookup_effective_pointer_focus_widget (toplevel,
                                                               device,
                                                               sequence);
  return target ? target : old_target;
}

static BobguiWidget *
handle_key_event (GdkEvent *event)
{
  BobguiWidget *event_widget;
  BobguiWidget *focus_widget;

  event_widget = bobgui_get_event_widget (event);

  focus_widget = bobgui_root_get_focus (bobgui_widget_get_root (event_widget));
  return focus_widget ? focus_widget : event_widget;
}

static gboolean
is_transient_for (BobguiWindow *child,
                  BobguiWindow *parent)
{
  BobguiWindow *transient_for;

  transient_for = bobgui_window_get_transient_for (child);

  while (transient_for)
    {
      if (transient_for == parent)
        return TRUE;

      transient_for = bobgui_window_get_transient_for (transient_for);
    }

  return FALSE;
}

gboolean
bobgui_main_do_event (GdkEvent *event)
{
  BobguiWidget *event_widget;
  BobguiWidget *target_widget;
  BobguiWidget *grab_widget = NULL;
  BobguiWindowGroup *window_group;
  GdkEvent *rewritten_event = NULL;
  GList *tmp_list;
  gboolean handled_event = FALSE;

  if (bobgui_inspector_handle_event (event))
    return FALSE;

  /* Find the widget which got the event. We store the widget
   * in the user_data field of GdkSurface's. Ignore the event
   * if we don't have a widget for it.
   */
  event_widget = bobgui_get_event_widget (event);
  if (!event_widget)
    return FALSE;

  target_widget = event_widget;

  /* We propagate key events from the root, even if they are
   * delivered to a popup surface.
   *
   * If pointer or keyboard grabs are in effect, munge the events
   * so that each window group looks like a separate app.
   */
  if (is_key_event (event))
    rewritten_event = rewrite_event_for_toplevel (event);
  else
    rewritten_event = rewrite_event_for_grabs (event);
  if (rewritten_event)
    {
      event = rewritten_event;
      target_widget = bobgui_get_event_widget (event);
    }

  /* Push the event onto a stack of current events for
   * bobgui_current_event_get().
   */
  current_events = g_list_prepend (current_events, event);

  if (is_pointing_event (event))
    {
      target_widget = handle_pointing_event (event);
    }
  else if (is_key_event (event))
    {
      target_widget = handle_key_event (event);
    }

  if (!target_widget)
    goto cleanup;

  window_group = bobgui_main_get_window_group (target_widget);

  /* check whether there is a grab in effect... */
  grab_widget = bobgui_window_group_get_current_grab (window_group);

  /* If the grab widget is an ancestor of the event widget
   * then we send the event to the original event widget.
   * This is the key to implementing modality. This also applies
   * across windows that are directly or indirectly transient-for
   * the modal one.
   */
  if (!grab_widget ||
      ((bobgui_widget_is_sensitive (target_widget) || gdk_event_get_event_type (event) == GDK_SCROLL) &&
       bobgui_widget_is_ancestor (target_widget, grab_widget)) ||
      (BOBGUI_IS_WINDOW (grab_widget) &&
       BOBGUI_IS_WINDOW (event_widget) &&
       grab_widget != event_widget &&
       is_transient_for (BOBGUI_WINDOW (event_widget), BOBGUI_WINDOW (grab_widget))))
    grab_widget = target_widget;

  g_object_ref (target_widget);

  /* Not all events get sent to the grabbing widget.
   * The delete, destroy, expose, focus change and resize
   * events still get sent to the event widget because
   * 1) these events have no meaning for the grabbing widget
   * and 2) redirecting these events to the grabbing widget
   * could cause the display to be messed up.
   *
   * Drag events are also not redirected, since it isn't
   * clear what the semantics of that would be.
   */
  switch ((guint)gdk_event_get_event_type (event))
    {
    case GDK_DELETE:
      if (!bobgui_window_group_get_current_grab (window_group) ||
          BOBGUI_WIDGET (bobgui_widget_get_root (bobgui_window_group_get_current_grab (window_group))) == target_widget)
        {
          if (BOBGUI_IS_WINDOW (target_widget) &&
              !bobgui_window_emit_close_request (BOBGUI_WINDOW (target_widget)))
            bobgui_window_destroy (BOBGUI_WINDOW (target_widget));
          handled_event = TRUE;
        }
      break;

    case GDK_FOCUS_CHANGE:
      {
        BobguiWidget *root = BOBGUI_WIDGET (bobgui_widget_get_root (target_widget));
        if (!_bobgui_widget_captured_event (root, event, root))
          handled_event = bobgui_widget_event (root, event, root);
      }
      break;

    case GDK_KEY_PRESS:
    case GDK_KEY_RELEASE:
    case GDK_SCROLL:
    case GDK_BUTTON_PRESS:
    case GDK_TOUCH_BEGIN:
    case GDK_MOTION_NOTIFY:
    case GDK_BUTTON_RELEASE:
    case GDK_PROXIMITY_IN:
    case GDK_PROXIMITY_OUT:
    case GDK_TOUCH_UPDATE:
    case GDK_TOUCH_END:
    case GDK_TOUCH_CANCEL:
    case GDK_TOUCHPAD_SWIPE:
    case GDK_TOUCHPAD_PINCH:
    case GDK_TOUCHPAD_HOLD:
    case GDK_PAD_BUTTON_PRESS:
    case GDK_PAD_BUTTON_RELEASE:
    case GDK_PAD_RING:
    case GDK_PAD_STRIP:
    case GDK_PAD_DIAL:
    case GDK_PAD_GROUP_MODE:
    case GDK_GRAB_BROKEN:
      handled_event = bobgui_propagate_event (grab_widget, event);
      break;

    case GDK_ENTER_NOTIFY:
    case GDK_LEAVE_NOTIFY:
    case GDK_DRAG_ENTER:
    case GDK_DRAG_LEAVE:
      /* Crossing event propagation happens during picking */
      handled_event = TRUE;
      break;

    case GDK_DRAG_MOTION:
    case GDK_DROP_START:
      {
        GdkDrop *drop = gdk_dnd_event_get_drop (event);
        bobgui_drop_begin_event (drop, gdk_event_get_event_type (event));
        handled_event = bobgui_propagate_event (grab_widget, event);
        bobgui_drop_end_event (drop);
      }
      break;

    case GDK_EVENT_LAST:
    default:
      g_assert_not_reached ();
      break;
    }

  _bobgui_tooltip_handle_event (target_widget, event);

  g_object_unref (target_widget);

 cleanup:
  tmp_list = current_events;
  current_events = g_list_remove_link (current_events, tmp_list);
  g_list_free_1 (tmp_list);

  if (rewritten_event)
    gdk_event_unref (rewritten_event);
  return handled_event;
}

static BobguiWindowGroup *
bobgui_main_get_window_group (BobguiWidget *widget)
{
  BobguiWidget *toplevel = NULL;

  if (widget)
    toplevel = BOBGUI_WIDGET (bobgui_widget_get_root (widget));

  if (BOBGUI_IS_WINDOW (toplevel))
    return bobgui_window_get_group (BOBGUI_WINDOW (toplevel));
  else
    return bobgui_window_get_group (NULL);
}

static void
bobgui_grab_notify (BobguiWindowGroup *group,
                 BobguiWidget      *old_grab_widget,
                 BobguiWidget      *new_grab_widget,
                 gboolean        from_grab)
{
  GList *toplevels;

  if (old_grab_widget == new_grab_widget)
    return;

  g_object_ref (group);

  toplevels = bobgui_window_list_toplevels ();
  g_list_foreach (toplevels, (GFunc)g_object_ref, NULL);

  while (toplevels)
    {
      BobguiWindow *toplevel = toplevels->data;
      toplevels = g_list_delete_link (toplevels, toplevels);

      bobgui_window_grab_notify (toplevel,
                              old_grab_widget,
                              new_grab_widget,
                              from_grab);
      g_object_unref (toplevel);
    }

  g_object_unref (group);
}

/**
 * bobgui_grab_add: (method)
 * @widget: The widget that grabs keyboard and pointer events
 *
 * Makes @widget the current grabbed widget.
 *
 * This means that interaction with other widgets in the same
 * application is blocked and mouse as well as keyboard events
 * are delivered to this widget.
 *
 * If @widget is not sensitive, it is not set as the current
 * grabbed widget and this function does nothing.
 */
void
bobgui_grab_add (BobguiWidget *widget)
{
  BobguiWindowGroup *group;
  BobguiWidget *old_grab_widget;

  g_return_if_fail (widget != NULL);

  if (!bobgui_widget_has_grab (widget) && bobgui_widget_is_sensitive (widget))
    {
      _bobgui_widget_set_has_grab (widget, TRUE);

      group = bobgui_main_get_window_group (widget);

      old_grab_widget = bobgui_window_group_get_current_grab (group);

      g_object_ref (widget);
      _bobgui_window_group_add_grab (group, widget);

      bobgui_grab_notify (group, old_grab_widget, widget, TRUE);
    }
}

/**
 * bobgui_grab_remove: (method)
 * @widget: The widget which gives up the grab
 *
 * Removes the grab from the given widget.
 *
 * You have to pair calls to bobgui_grab_add() and bobgui_grab_remove().
 *
 * If @widget does not have the grab, this function does nothing.
 */
void
bobgui_grab_remove (BobguiWidget *widget)
{
  BobguiWindowGroup *group;
  BobguiWidget *new_grab_widget;

  g_return_if_fail (widget != NULL);

  if (bobgui_widget_has_grab (widget))
    {
      _bobgui_widget_set_has_grab (widget, FALSE);

      group = bobgui_main_get_window_group (widget);
      _bobgui_window_group_remove_grab (group, widget);
      new_grab_widget = bobgui_window_group_get_current_grab (group);

      bobgui_grab_notify (group, widget, new_grab_widget, FALSE);

      g_object_unref (widget);
    }
}

guint32
bobgui_get_current_event_time (void)
{
  if (current_events)
    return gdk_event_get_time (current_events->data);
  else
    return GDK_CURRENT_TIME;
}

/**
 * bobgui_get_event_widget:
 * @event: a `GdkEvent`
 *
 * If @event is %NULL or the event was not associated with any widget,
 * returns %NULL, otherwise returns the widget that received the event
 * originally.
 *
 * Returns: (transfer none) (nullable): the widget that originally
 *   received @event
 */
BobguiWidget *
bobgui_get_event_widget (GdkEvent *event)
{
  GdkSurface *surface;

  surface = gdk_event_get_surface (event);
  if (surface && !gdk_surface_is_destroyed (surface))
    return BOBGUI_WIDGET (bobgui_native_get_for_surface (surface));

  return NULL;
}

gboolean
bobgui_propagate_event_internal (BobguiWidget *widget,
                              GdkEvent  *event,
                              BobguiWidget *topmost)
{
  int handled_event = FALSE;
  BobguiWidget *target = widget;
  BobguiWidgetStack widget_array;
  int i;

  /* First, propagate event down */
  bobgui_widget_stack_init (&widget_array);
  bobgui_widget_stack_append (&widget_array, g_object_ref (widget));

  for (;;)
    {
      widget = _bobgui_widget_get_parent (widget);
      if (!widget)
        break;

      bobgui_widget_stack_append (&widget_array, g_object_ref (widget));

      if (widget == topmost)
        break;
    }

  i = bobgui_widget_stack_get_size (&widget_array) - 1;
  for (;;)
    {
      widget = bobgui_widget_stack_get (&widget_array, i);

      if (!_bobgui_widget_is_sensitive (widget))
        {
          /* stop propagating on SCROLL, but don't handle the event, so it
           * can propagate up again and reach its handling widget
           */
          if (gdk_event_get_event_type (event) == GDK_SCROLL)
            break;
          else
            handled_event = TRUE;
        }
      else if (_bobgui_widget_get_realized (widget))
        handled_event = _bobgui_widget_captured_event (widget, event, target);

      handled_event |= !_bobgui_widget_get_realized (widget);

      if (handled_event)
        break;

      if (i == 0)
        break;

      i--;
    }

  /* If not yet handled, also propagate back up */
  if (!handled_event)
    {
      /* Propagate event up the widget tree so that
       * parents can see the button and motion
       * events of the children.
       */
      for (i = 0; i < bobgui_widget_stack_get_size (&widget_array); i++)
        {
          widget = bobgui_widget_stack_get (&widget_array, i);

          /* Scroll events are special cased here because it
           * feels wrong when scrolling a BobguiViewport, say,
           * to have children of the viewport eat the scroll
           * event
           */
          if (!_bobgui_widget_is_sensitive (widget))
            handled_event = gdk_event_get_event_type (event) != GDK_SCROLL;
          else if (_bobgui_widget_get_realized (widget))
            handled_event = bobgui_widget_event (widget, event, target);

          handled_event |= !_bobgui_widget_get_realized (widget);

          if (handled_event)
            break;
        }
    }

  bobgui_widget_stack_clear (&widget_array);
  return handled_event;
}

/**
 * bobgui_propagate_event:
 * @widget: a `BobguiWidget`
 * @event: an event
 *
 * Sends an event to a widget, propagating the event to parent widgets
 * if the event remains unhandled. This function will emit the event
 * through all the hierarchy of @widget through all propagation phases.
 *
 * Events received by BOBGUI from GDK normally begin at a `BobguiRoot` widget.
 * Depending on the type of event, existence of modal dialogs, grabs, etc.,
 * the event may be propagated; if so, this function is used.
 *
 * All that said, you most likely don’t want to use any of these
 * functions; synthesizing events is rarely needed. There are almost
 * certainly better ways to achieve your goals. For example, use
 * bobgui_widget_queue_draw() instead
 * of making up expose events.
 *
 * Returns: %TRUE if the event was handled
 */
gboolean
bobgui_propagate_event (BobguiWidget *widget,
                     GdkEvent  *event)
{
  BobguiWindowGroup *window_group;
  BobguiWidget *event_widget, *topmost = NULL;

  g_return_val_if_fail (BOBGUI_IS_WIDGET (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  event_widget = bobgui_get_event_widget (event);
  window_group = bobgui_main_get_window_group (event_widget);

  /* check whether there is a grab in effect... */
  topmost = bobgui_window_group_get_current_grab (window_group);

  return bobgui_propagate_event_internal (widget, event, topmost);
}
