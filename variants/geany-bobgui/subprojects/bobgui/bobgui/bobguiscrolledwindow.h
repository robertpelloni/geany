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

#include <bobgui/bobguiwidget.h>

G_BEGIN_DECLS


#define BOBGUI_TYPE_SCROLLED_WINDOW            (bobgui_scrolled_window_get_type ())
#define BOBGUI_SCROLLED_WINDOW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_SCROLLED_WINDOW, BobguiScrolledWindow))
#define BOBGUI_IS_SCROLLED_WINDOW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_SCROLLED_WINDOW))


typedef struct _BobguiScrolledWindow BobguiScrolledWindow;

/**
 * BobguiCornerType:
 * @BOBGUI_CORNER_TOP_LEFT: Place the scrollbars on the right and bottom of the
 *   widget (default behaviour).
 * @BOBGUI_CORNER_BOTTOM_LEFT: Place the scrollbars on the top and right of the
 *   widget.
 * @BOBGUI_CORNER_TOP_RIGHT: Place the scrollbars on the left and bottom of the
 *   widget.
 * @BOBGUI_CORNER_BOTTOM_RIGHT: Place the scrollbars on the top and left of the
 *   widget.
 *
 * Specifies which corner a child widget should be placed in when packed into
 * a `BobguiScrolledWindow.`
 *
 * This is effectively the opposite of where the scroll bars are placed.
 */
typedef enum
{
  BOBGUI_CORNER_TOP_LEFT,
  BOBGUI_CORNER_BOTTOM_LEFT,
  BOBGUI_CORNER_TOP_RIGHT,
  BOBGUI_CORNER_BOTTOM_RIGHT
} BobguiCornerType;


/**
 * BobguiPolicyType:
 * @BOBGUI_POLICY_ALWAYS: The scrollbar is always visible. The view size is
 *   independent of the content.
 * @BOBGUI_POLICY_AUTOMATIC: The scrollbar will appear and disappear as necessary.
 *   For example, when all of a `BobguiTreeView` can not be seen.
 * @BOBGUI_POLICY_NEVER: The scrollbar should never appear. In this mode the
 *   content determines the size.
 * @BOBGUI_POLICY_EXTERNAL: Don't show a scrollbar, but don't force the
 *   size to follow the content. This can be used e.g. to make multiple
 *   scrolled windows share a scrollbar.
 *
 * Determines how the size should be computed to achieve the one of the
 * visibility mode for the scrollbars.
 */
typedef enum
{
  BOBGUI_POLICY_ALWAYS,
  BOBGUI_POLICY_AUTOMATIC,
  BOBGUI_POLICY_NEVER,
  BOBGUI_POLICY_EXTERNAL
} BobguiPolicyType;


GDK_AVAILABLE_IN_ALL
GType          bobgui_scrolled_window_get_type          (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
BobguiWidget*     bobgui_scrolled_window_new               (void);
GDK_AVAILABLE_IN_ALL
void           bobgui_scrolled_window_set_hadjustment   (BobguiScrolledWindow *scrolled_window,
                                                      BobguiAdjustment     *hadjustment);
GDK_AVAILABLE_IN_ALL
void           bobgui_scrolled_window_set_vadjustment   (BobguiScrolledWindow *scrolled_window,
                                                      BobguiAdjustment     *vadjustment);
GDK_AVAILABLE_IN_ALL
BobguiAdjustment* bobgui_scrolled_window_get_hadjustment   (BobguiScrolledWindow *scrolled_window);
GDK_AVAILABLE_IN_ALL
BobguiAdjustment* bobgui_scrolled_window_get_vadjustment   (BobguiScrolledWindow *scrolled_window);
GDK_AVAILABLE_IN_ALL
BobguiWidget*     bobgui_scrolled_window_get_hscrollbar    (BobguiScrolledWindow *scrolled_window);
GDK_AVAILABLE_IN_ALL
BobguiWidget*     bobgui_scrolled_window_get_vscrollbar    (BobguiScrolledWindow *scrolled_window);
GDK_AVAILABLE_IN_ALL
void           bobgui_scrolled_window_set_policy        (BobguiScrolledWindow *scrolled_window,
                                                      BobguiPolicyType      hscrollbar_policy,
                                                      BobguiPolicyType      vscrollbar_policy);
GDK_AVAILABLE_IN_ALL
void           bobgui_scrolled_window_get_policy        (BobguiScrolledWindow *scrolled_window,
                                                      BobguiPolicyType     *hscrollbar_policy,
                                                      BobguiPolicyType     *vscrollbar_policy);
GDK_AVAILABLE_IN_ALL
void           bobgui_scrolled_window_set_placement     (BobguiScrolledWindow *scrolled_window,
                                                      BobguiCornerType      window_placement);
GDK_AVAILABLE_IN_ALL
void           bobgui_scrolled_window_unset_placement   (BobguiScrolledWindow *scrolled_window);

GDK_AVAILABLE_IN_ALL
BobguiCornerType  bobgui_scrolled_window_get_placement     (BobguiScrolledWindow *scrolled_window);
GDK_AVAILABLE_IN_ALL
void           bobgui_scrolled_window_set_has_frame     (BobguiScrolledWindow *scrolled_window,
                                                      gboolean           has_frame);
GDK_AVAILABLE_IN_ALL
gboolean       bobgui_scrolled_window_get_has_frame     (BobguiScrolledWindow *scrolled_window);

GDK_AVAILABLE_IN_ALL
int            bobgui_scrolled_window_get_min_content_width  (BobguiScrolledWindow *scrolled_window);
GDK_AVAILABLE_IN_ALL
void           bobgui_scrolled_window_set_min_content_width  (BobguiScrolledWindow *scrolled_window,
                                                           int                width);
GDK_AVAILABLE_IN_ALL
int            bobgui_scrolled_window_get_min_content_height (BobguiScrolledWindow *scrolled_window);
GDK_AVAILABLE_IN_ALL
void           bobgui_scrolled_window_set_min_content_height (BobguiScrolledWindow *scrolled_window,
                                                           int                height);
GDK_AVAILABLE_IN_ALL
void           bobgui_scrolled_window_set_kinetic_scrolling  (BobguiScrolledWindow *scrolled_window,
                                                           gboolean           kinetic_scrolling);
GDK_AVAILABLE_IN_ALL
gboolean       bobgui_scrolled_window_get_kinetic_scrolling  (BobguiScrolledWindow *scrolled_window);

GDK_AVAILABLE_IN_ALL
void           bobgui_scrolled_window_set_overlay_scrolling  (BobguiScrolledWindow *scrolled_window,
                                                           gboolean           overlay_scrolling);
GDK_AVAILABLE_IN_ALL
gboolean       bobgui_scrolled_window_get_overlay_scrolling (BobguiScrolledWindow   *scrolled_window);

GDK_AVAILABLE_IN_ALL
void           bobgui_scrolled_window_set_max_content_width  (BobguiScrolledWindow *scrolled_window,
                                                           int                width);
GDK_AVAILABLE_IN_ALL
int            bobgui_scrolled_window_get_max_content_width  (BobguiScrolledWindow *scrolled_window);

GDK_AVAILABLE_IN_ALL
void           bobgui_scrolled_window_set_max_content_height (BobguiScrolledWindow *scrolled_window,
                                                           int                height);
GDK_AVAILABLE_IN_ALL
int            bobgui_scrolled_window_get_max_content_height (BobguiScrolledWindow *scrolled_window);

GDK_AVAILABLE_IN_ALL
void           bobgui_scrolled_window_set_propagate_natural_width  (BobguiScrolledWindow *scrolled_window,
                                                                 gboolean           propagate);
GDK_AVAILABLE_IN_ALL
gboolean       bobgui_scrolled_window_get_propagate_natural_width  (BobguiScrolledWindow *scrolled_window);

GDK_AVAILABLE_IN_ALL
void           bobgui_scrolled_window_set_propagate_natural_height (BobguiScrolledWindow *scrolled_window,
                                                                 gboolean           propagate);
GDK_AVAILABLE_IN_ALL
gboolean       bobgui_scrolled_window_get_propagate_natural_height (BobguiScrolledWindow *scrolled_window);

GDK_AVAILABLE_IN_ALL
void           bobgui_scrolled_window_set_child        (BobguiScrolledWindow *scrolled_window,
                                                     BobguiWidget         *child);
GDK_AVAILABLE_IN_ALL
BobguiWidget     *bobgui_scrolled_window_get_child        (BobguiScrolledWindow *scrolled_window);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiScrolledWindow, g_object_unref)

G_END_DECLS


