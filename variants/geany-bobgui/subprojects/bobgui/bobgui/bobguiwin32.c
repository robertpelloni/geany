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

#include "bobguiprivate.h"

#include <windows.h>
#include <commctrl.h>

extern IMAGE_DOS_HEADER __ImageBase;

static inline HMODULE
this_module (void)
{
  return (HMODULE) &__ImageBase;
}

/* In practice, resulting DLL will have manifest resource under index 2.
 * Fall back to that value if we can't find resource index programmatically.
 */
#define EMPIRIC_MANIFEST_RESOURCE_INDEX 2

static wchar_t *
g_wcsdup (const wchar_t *wcs)
{
  wchar_t *new_wcs = NULL;
  gsize length;

  if G_LIKELY (wcs)
    {
      length = wcslen (wcs) + 1;
      new_wcs = g_new (wchar_t, length);
      wcscpy (new_wcs, wcs);
      new_wcs[length - 1] = L'\0';
    }

  return new_wcs;
}

static BOOL CALLBACK
find_first_manifest (HMODULE  module_handle,
                     LPCWSTR  resource_type,
                     LPWSTR   resource_name,
                     LONG_PTR user_data)
{
  LPWSTR *result_name = (LPWSTR *) user_data;

  if (resource_type == RT_MANIFEST)
    {
      if (IS_INTRESOURCE (resource_name))
        *result_name = resource_name;
      else
        *result_name = g_wcsdup (resource_name);
      return FALSE;
    }
  return TRUE;
}

/*
 * Grabs the first manifest it finds in libbobgui (which is expected to be the
 * common-controls-6.0.0.0 manifest we embedded to enable visual styles),
 * uses it to create a process-default activation context, activates that
 * context, loads up the library passed in @dllname, then deactivates and
 * releases the context.
 *
 * In practice this is used to force system DLLs (like comdlg32) to be
 * loaded as if the application had the same manifest as libbobgui
 * (otherwise libbobgui3 manifest only affests libbobgui itself).
 * This way application does not need to have a manifest or to link
 * against comctl32.
 *
 * Note that loaded library handle leaks, so only use this function in
 * g_once_init_enter (leaking once is OK, Windows will clean up after us).
 */
void
_bobgui_load_dll_with_libbobgui3_manifest (const wchar_t *dll_name)
{
  HANDLE activation_ctx_handle;
  ACTCTX activation_ctx_descriptor;
  ULONG_PTR activation_cookie;
  LPWSTR resource_name;
  BOOL activated;
  DWORD error_code;

  resource_name = NULL;
  EnumResourceNames (this_module (), RT_MANIFEST, find_first_manifest,
                     (LONG_PTR) &resource_name);

  if (resource_name == NULL)
    resource_name = MAKEINTRESOURCE (EMPIRIC_MANIFEST_RESOURCE_INDEX);

  memset (&activation_ctx_descriptor, 0, sizeof (activation_ctx_descriptor));
  activation_ctx_descriptor.cbSize = sizeof (activation_ctx_descriptor);
  activation_ctx_descriptor.dwFlags = ACTCTX_FLAG_RESOURCE_NAME_VALID |
                                      ACTCTX_FLAG_HMODULE_VALID |
                                      ACTCTX_FLAG_SET_PROCESS_DEFAULT;
  activation_ctx_descriptor.hModule = this_module ();
  activation_ctx_descriptor.lpResourceName = resource_name;
  activation_ctx_handle = CreateActCtx (&activation_ctx_descriptor);
  error_code = GetLastError ();

  if (activation_ctx_handle == INVALID_HANDLE_VALUE &&
      error_code != ERROR_SXS_PROCESS_DEFAULT_ALREADY_SET)
    g_warning ("Failed to CreateActCtx for module %p, resource %p: %lu",
               this_module (), resource_name, GetLastError ());
  else if (error_code != ERROR_SXS_PROCESS_DEFAULT_ALREADY_SET)
    {
      activation_cookie = 0;
      activated = ActivateActCtx (activation_ctx_handle, &activation_cookie);

      if (!activated)
        g_warning ("Failed to ActivateActCtx: %lu", GetLastError ());

      LoadLibrary (dll_name);

      if (activated && !DeactivateActCtx (0, activation_cookie))
        g_warning ("Failed to DeactivateActCtx: %lu", GetLastError ());

      ReleaseActCtx (activation_ctx_handle);
    }

  if (!IS_INTRESOURCE (resource_name))
    g_free (resource_name);
}

const char *
_bobgui_get_libdir (void)
{
  static char *bobgui_libdir = NULL;
  if (bobgui_libdir == NULL)
    {
      char *root = g_win32_get_package_installation_directory_of_module (this_module ());
      char *slash = strrchr (root, '\\');
      if (slash != NULL &&
          g_ascii_strcasecmp (slash + 1, ".libs") == 0)
        bobgui_libdir = g_strdup (BOBGUI_LIBDIR);
      else
        bobgui_libdir = g_build_filename (root, "lib", NULL);
      g_free (root);
    }

  return bobgui_libdir;
}

const char *
_bobgui_get_localedir (void)
{
  static char *bobgui_localedir = NULL;
  if (bobgui_localedir == NULL)
    {
      const char *p;
      char *root, *temp;

      /* BOBGUI_LOCALEDIR ends in either /lib/locale or
       * /share/locale. Scan for that slash.
       */
      p = BOBGUI_LOCALEDIR;
      p += strlen (BOBGUI_LOCALEDIR);
      while (*--p != '/')
        ;
      while (*--p != '/')
        ;

      root = g_win32_get_package_installation_directory_of_module (this_module ());
      temp = g_build_filename (root, p, NULL);
      g_free (root);

      /* bobgui_localedir is passed to bindtextdomain() which isn't
       * UTF-8-aware.
       */
      bobgui_localedir = g_win32_locale_filename_from_utf8 (temp);
      g_free (temp);
    }
  return bobgui_localedir;
}

const char *
_bobgui_get_datadir (void)
{
  static char *bobgui_datadir = NULL;
  if (bobgui_datadir == NULL)
    {
      char *root = g_win32_get_package_installation_directory_of_module (this_module ());
      bobgui_datadir = g_build_filename (root, "share", NULL);
      g_free (root);
    }

  return bobgui_datadir;
}

const char *
_bobgui_get_sysconfdir (void)
{
  static char *bobgui_sysconfdir = NULL;
  if (bobgui_sysconfdir == NULL)
    {
      char *root = g_win32_get_package_installation_directory_of_module (this_module ());
      bobgui_sysconfdir = g_build_filename (root, "etc", NULL);
      g_free (root);
    }

  return bobgui_sysconfdir;
}

const char *
_bobgui_get_data_prefix (void)
{
  static char *bobgui_data_prefix = NULL;
  if (bobgui_data_prefix == NULL)
    bobgui_data_prefix = g_win32_get_package_installation_directory_of_module (this_module ());

  return bobgui_data_prefix;
}
