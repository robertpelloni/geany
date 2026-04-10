/* bobguishortcutsshortcutprivate.h
 *
 * Copyright (C) 2015 Christian Hergert <christian@hergert.me>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <gdk/gdk.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_SHORTCUTS_SHORTCUT (bobgui_shortcuts_shortcut_get_type())
#define BOBGUI_SHORTCUTS_SHORTCUT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_SHORTCUTS_SHORTCUT, BobguiShortcutsShortcut))
#define BOBGUI_IS_SHORTCUTS_SHORTCUT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_SHORTCUTS_SHORTCUT))


typedef struct _BobguiShortcutsShortcut      BobguiShortcutsShortcut;
typedef struct _BobguiShortcutsShortcutClass BobguiShortcutsShortcutClass;

/**
 * BobguiShortcutType:
 * @BOBGUI_SHORTCUT_ACCELERATOR:
 *   The shortcut is a keyboard accelerator. The BobguiShortcutsShortcut:accelerator
 *   property will be used.
 * @BOBGUI_SHORTCUT_GESTURE_PINCH:
 *   The shortcut is a pinch gesture. BOBGUI provides an icon and subtitle.
 * @BOBGUI_SHORTCUT_GESTURE_STRETCH:
 *   The shortcut is a stretch gesture. BOBGUI provides an icon and subtitle.
 * @BOBGUI_SHORTCUT_GESTURE_ROTATE_CLOCKWISE:
 *   The shortcut is a clockwise rotation gesture. BOBGUI provides an icon and subtitle.
 * @BOBGUI_SHORTCUT_GESTURE_ROTATE_COUNTERCLOCKWISE:
 *   The shortcut is a counterclockwise rotation gesture. BOBGUI provides an icon and subtitle.
 * @BOBGUI_SHORTCUT_GESTURE_TWO_FINGER_SWIPE_LEFT:
 *   The shortcut is a two-finger swipe gesture. BOBGUI provides an icon and subtitle.
 * @BOBGUI_SHORTCUT_GESTURE_TWO_FINGER_SWIPE_RIGHT:
 *   The shortcut is a two-finger swipe gesture. BOBGUI provides an icon and subtitle.
 * @BOBGUI_SHORTCUT_GESTURE:
 *   The shortcut is a gesture. The BobguiShortcutsShortcut:icon property will be
 *   used.
 * @BOBGUI_SHORTCUT_GESTURE_SWIPE_LEFT:
 *   The shortcut is a swipe gesture. BOBGUI provides an icon and subtitle.
 * @BOBGUI_SHORTCUT_GESTURE_SWIPE_RIGHT:
 *   The shortcut is a swipe gesture. BOBGUI provides an icon and subtitle.
 *
 * BobguiShortcutType specifies the kind of shortcut that is being described.
 *
 * More values may be added to this enumeration over time.
 */
typedef enum {
  BOBGUI_SHORTCUT_ACCELERATOR,
  BOBGUI_SHORTCUT_GESTURE_PINCH,
  BOBGUI_SHORTCUT_GESTURE_STRETCH,
  BOBGUI_SHORTCUT_GESTURE_ROTATE_CLOCKWISE,
  BOBGUI_SHORTCUT_GESTURE_ROTATE_COUNTERCLOCKWISE,
  BOBGUI_SHORTCUT_GESTURE_TWO_FINGER_SWIPE_LEFT,
  BOBGUI_SHORTCUT_GESTURE_TWO_FINGER_SWIPE_RIGHT,
  BOBGUI_SHORTCUT_GESTURE,
  BOBGUI_SHORTCUT_GESTURE_SWIPE_LEFT,
  BOBGUI_SHORTCUT_GESTURE_SWIPE_RIGHT
} BobguiShortcutType;

GDK_AVAILABLE_IN_ALL
GType        bobgui_shortcuts_shortcut_get_type (void) G_GNUC_CONST;

G_END_DECLS

