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

#include "bobguiversion.h"

/**
 * bobgui_get_major_version:
 *
 * Returns the major version number of the BOBGUI library.
 *
 * For example, in BOBGUI version 3.1.5 this is 3.
 *
 * This function is in the library, so it represents the BOBGUI library
 * your code is running against. Contrast with the %BOBGUI_MAJOR_VERSION
 * macro, which represents the major version of the BOBGUI headers you
 * have included when compiling your code.
 *
 * Returns: the major version number of the BOBGUI library
 */
guint
bobgui_get_major_version (void)
{
  return BOBGUI_MAJOR_VERSION;
}

/**
 * bobgui_get_minor_version:
 *
 * Returns the minor version number of the BOBGUI library.
 *
 * For example, in BOBGUI version 3.1.5 this is 1.
 *
 * This function is in the library, so it represents the BOBGUI library
 * your code is are running against. Contrast with the
 * %BOBGUI_MINOR_VERSION macro, which represents the minor version of the
 * BOBGUI headers you have included when compiling your code.
 *
 * Returns: the minor version number of the BOBGUI library
 */
guint
bobgui_get_minor_version (void)
{
  return BOBGUI_MINOR_VERSION;
}

/**
 * bobgui_get_micro_version:
 *
 * Returns the micro version number of the BOBGUI library.
 *
 * For example, in BOBGUI version 3.1.5 this is 5.
 *
 * This function is in the library, so it represents the BOBGUI library
 * your code is are running against. Contrast with the
 * %BOBGUI_MICRO_VERSION macro, which represents the micro version of the
 * BOBGUI headers you have included when compiling your code.
 *
 * Returns: the micro version number of the BOBGUI library
 */
guint
bobgui_get_micro_version (void)
{
  return BOBGUI_MICRO_VERSION;
}

/**
 * bobgui_get_binary_age:
 *
 * Returns the binary age as passed to `libtool`.
 *
 * If `libtool` means nothing to you, don't worry about it.
 *
 * Returns: the binary age of the BOBGUI library
 */
guint
bobgui_get_binary_age (void)
{
  return BOBGUI_BINARY_AGE;
}

/**
 * bobgui_get_interface_age:
 *
 * Returns the interface age as passed to `libtool`.
 *
 * If `libtool` means nothing to you, don't worry about it.
 *
 * Returns: the interface age of the BOBGUI library
 */
guint
bobgui_get_interface_age (void)
{
  return BOBGUI_INTERFACE_AGE;
}

/**
 * bobgui_check_version:
 * @required_major: the required major version
 * @required_minor: the required minor version
 * @required_micro: the required micro version
 *
 * Checks that the BOBGUI library in use is compatible with the
 * given version.
 *
 * Generally you would pass in the constants %BOBGUI_MAJOR_VERSION,
 * %BOBGUI_MINOR_VERSION, %BOBGUI_MICRO_VERSION as the three arguments
 * to this function; that produces a check that the library in
 * use is compatible with the version of BOBGUI the application or
 * module was compiled against.
 *
 * Compatibility is defined by two things: first the version
 * of the running library is newer than the version
 * @required_major.required_minor.@required_micro. Second
 * the running library must be binary compatible with the
 * version @required_major.required_minor.@required_micro
 * (same major version.)
 *
 * This function is primarily for BOBGUI modules; the module
 * can call this function to check that it wasn’t loaded
 * into an incompatible version of BOBGUI. However, such a
 * check isn’t completely reliable, since the module may be
 * linked against an old version of BOBGUI and calling the
 * old version of bobgui_check_version(), but still get loaded
 * into an application using a newer version of BOBGUI.
 *
 * Returns: (nullable): %NULL if the BOBGUI library is compatible with the
 *   given version, or a string describing the version mismatch.
 *   The returned string is owned by BOBGUI and should not be modified
 *   or freed.
 */
const char *
bobgui_check_version (guint required_major,
                   guint required_minor,
                   guint required_micro)
{
  int bobgui_effective_micro = 100 * BOBGUI_MINOR_VERSION + BOBGUI_MICRO_VERSION;
  int required_effective_micro = 100 * required_minor + required_micro;

  if (required_major > BOBGUI_MAJOR_VERSION)
    return "BOBGUI version too old (major mismatch)";
  if (required_major < BOBGUI_MAJOR_VERSION)
    return "BOBGUI version too new (major mismatch)";
  if (required_effective_micro < bobgui_effective_micro - BOBGUI_BINARY_AGE)
    return "BOBGUI version too new (micro mismatch)";
  if (required_effective_micro > bobgui_effective_micro)
    return "BOBGUI version too old (micro mismatch)";
  return NULL;
}
