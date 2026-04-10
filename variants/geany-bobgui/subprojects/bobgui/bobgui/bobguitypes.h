/* GDK - The GIMP Drawing Kit
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

#include <gdk/gdk.h>

G_BEGIN_DECLS

typedef struct _BobguiAdjustment          BobguiAdjustment;
typedef struct _BobguiATContext           BobguiATContext;
typedef struct _BobguiBitset              BobguiBitset;
typedef struct _BobguiBuilder             BobguiBuilder;
typedef struct _BobguiBuilderScope        BobguiBuilderScope;

/**
 * BobguiCssStyleChange:
 *
 * A CSS style change.
 */
typedef struct _BobguiCssStyleChange      BobguiCssStyleChange;
typedef struct _BobguiEventController     BobguiEventController;
typedef struct _BobguiGesture             BobguiGesture;
typedef struct _BobguiLayoutManager       BobguiLayoutManager;
typedef struct _BobguiListItem            BobguiListItem;
typedef struct _BobguiListItemFactory     BobguiListItemFactory;
typedef struct _BobguiNative              BobguiNative;
typedef struct _BobguiRequisition	       BobguiRequisition;
typedef struct _BobguiRoot  	       BobguiRoot;
typedef struct _BobguiScrollInfo  	       BobguiScrollInfo;
typedef struct _BobguiSettings            BobguiSettings;
typedef struct _BobguiShortcut            BobguiShortcut;
typedef struct _BobguiShortcutAction      BobguiShortcutAction;
typedef struct _BobguiShortcutTrigger     BobguiShortcutTrigger;
typedef GdkSnapshot                    BobguiSnapshot;
typedef struct _BobguiStyleContext        BobguiStyleContext;
typedef struct _BobguiTooltip             BobguiTooltip;
typedef struct _BobguiWidget              BobguiWidget;
typedef struct _BobguiWindow              BobguiWindow;

/**
 * BOBGUI_INVALID_LIST_POSITION:
 *
 * The value used to refer to a guaranteed invalid position
 * in a `GListModel`.
 *
 * This value may be returned from some functions, others may
 * accept it as input. Its interpretation may differ for different
 * functions.
 *
 * Refer to each function's documentation for if this value is
 * allowed and what it does.
 */
#define BOBGUI_INVALID_LIST_POSITION ((guint) 0xffffffff)

G_END_DECLS

