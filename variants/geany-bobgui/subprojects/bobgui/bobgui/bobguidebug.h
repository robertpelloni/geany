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

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <glib.h>
#include <gdk/gdk.h>

G_BEGIN_DECLS

/**
 * BobguiDebugFlags:
 * @BOBGUI_DEBUG_TEXT: Information about BobguiTextView
 * @BOBGUI_DEBUG_TREE: Information about BobguiTreeView
 * @BOBGUI_DEBUG_KEYBINDINGS: Information about keyboard shortcuts
 * @BOBGUI_DEBUG_MODULES: Information about modules and extensions
 * @BOBGUI_DEBUG_GEOMETRY: Information about size allocation
 * @BOBGUI_DEBUG_ICONTHEME: Information about icon themes
 * @BOBGUI_DEBUG_PRINTING: Information about printing
 * @BOBGUI_DEBUG_BUILDER_TRACE: Trace BobguiBuilder operation
 * @BOBGUI_DEBUG_SIZE_REQUEST: Information about size requests
 * @BOBGUI_DEBUG_NO_CSS_CACHE: Disable the style property cache
 * @BOBGUI_DEBUG_INTERACTIVE: Open the BOBGUI inspector
 * @BOBGUI_DEBUG_ACTIONS: Information about actions and menu models
 * @BOBGUI_DEBUG_LAYOUT: Information from layout managers
 * @BOBGUI_DEBUG_SNAPSHOT: Include debug render nodes in the generated snapshots
 * @BOBGUI_DEBUG_CONSTRAINTS: Information from the constraints solver
 * @BOBGUI_DEBUG_BUILDER_OBJECTS: Log unused BobguiBuilder objects
 * @BOBGUI_DEBUG_A11Y: Information about accessibility state changes
 *
 * Flags to use with bobgui_set_debug_flags().
 *
 * Settings these flags causes BOBGUI to print out different
 * types of debugging information. Some of these flags are
 * only available when BOBGUI has been configured with `-Ddebug=true`.
 */

/**
 * BOBGUI_DEBUG_ICONFALLBACK:
 *
 * Information about icon fallback.
 *
 * Since: 4.2
 */

/**
 * BOBGUI_DEBUG_INVERT_TEXT_DIR:
 *
 * Inverts the default text-direction.
 *
 * Since: 4.8
 */

/**
 * BOBGUI_DEBUG_CSS:
 *
 * Information about deprecated CSS features.
 *
 * Since: 4.16
 */

/**
 * BOBGUI_DEBUG_BUILDER:
 *
 * Information about deprecated BobguiBuilder features.
 *
 * Since: 4.18
 */

 /**
  * BOBGUI_DEBUG_TOUCHSCREEN:
  *
  * Show touch UI elements for pointer events.
  *
  * Since: 4.20
  */

/**
 * BOBGUI_DEBUG_SESSION:
 *
 * Information about session saving.
 *
 * Since: 4.22
 */
typedef enum {
  BOBGUI_DEBUG_TEXT            = 1 <<  0,
  BOBGUI_DEBUG_TREE            = 1 <<  1,
  BOBGUI_DEBUG_KEYBINDINGS     = 1 <<  2,
  BOBGUI_DEBUG_MODULES         = 1 <<  3,
  BOBGUI_DEBUG_GEOMETRY        = 1 <<  4,
  BOBGUI_DEBUG_ICONTHEME       = 1 <<  5,
  BOBGUI_DEBUG_PRINTING        = 1 <<  6,
  BOBGUI_DEBUG_BUILDER_TRACE   = 1 <<  7,
  BOBGUI_DEBUG_SIZE_REQUEST    = 1 <<  8,
  BOBGUI_DEBUG_NO_CSS_CACHE    = 1 <<  9,
  BOBGUI_DEBUG_INTERACTIVE     = 1 << 10,
  BOBGUI_DEBUG_TOUCHSCREEN     = 1 << 11,
  BOBGUI_DEBUG_ACTIONS         = 1 << 12,
  BOBGUI_DEBUG_LAYOUT          = 1 << 13,
  BOBGUI_DEBUG_SNAPSHOT        = 1 << 14,
  BOBGUI_DEBUG_CONSTRAINTS     = 1 << 15,
  BOBGUI_DEBUG_BUILDER_OBJECTS = 1 << 16,
  BOBGUI_DEBUG_A11Y            = 1 << 17,
  BOBGUI_DEBUG_ICONFALLBACK    = 1 << 18,
  BOBGUI_DEBUG_INVERT_TEXT_DIR = 1 << 19,
  BOBGUI_DEBUG_CSS             = 1 << 20,
  BOBGUI_DEBUG_BUILDER         = 1 << 21,
  BOBGUI_DEBUG_SESSION         = 1 << 22,
} BobguiDebugFlags;

/**
 * BOBGUI_DEBUG_CHECK:
 * @type: type to check
 *
 * Whether the `type` debug flag is set.
 **/
#define BOBGUI_DEBUG_CHECK(type) G_UNLIKELY (bobgui_get_debug_flags () & BOBGUI_DEBUG_##type)

GDK_AVAILABLE_IN_ALL
BobguiDebugFlags bobgui_get_debug_flags (void);
GDK_AVAILABLE_IN_ALL
void          bobgui_set_debug_flags (BobguiDebugFlags flags);

G_END_DECLS
