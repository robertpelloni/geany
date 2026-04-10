/* BOBGUI - The Bobgui Framework
 * bobguitextchild.h Copyright (C) 2000 Red Hat, Inc.
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
#include <glib-object.h>
#include <bobgui/bobguiwidget.h>

G_BEGIN_DECLS


/**
 * BobguiTextChildAnchor:
 *
 * Marks a spot in a `BobguiTextBuffer` where child widgets can be “anchored”.
 *
 * The anchor can have multiple widgets anchored, to allow for multiple views.
 */
typedef struct _BobguiTextChildAnchor      BobguiTextChildAnchor;
typedef struct _BobguiTextChildAnchorClass BobguiTextChildAnchorClass;

#define BOBGUI_TYPE_TEXT_CHILD_ANCHOR              (bobgui_text_child_anchor_get_type ())
#define BOBGUI_TEXT_CHILD_ANCHOR(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BOBGUI_TYPE_TEXT_CHILD_ANCHOR, BobguiTextChildAnchor))
#define BOBGUI_TEXT_CHILD_ANCHOR_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), BOBGUI_TYPE_TEXT_CHILD_ANCHOR, BobguiTextChildAnchorClass))
#define BOBGUI_IS_TEXT_CHILD_ANCHOR(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BOBGUI_TYPE_TEXT_CHILD_ANCHOR))
#define BOBGUI_IS_TEXT_CHILD_ANCHOR_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), BOBGUI_TYPE_TEXT_CHILD_ANCHOR))
#define BOBGUI_TEXT_CHILD_ANCHOR_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_TEXT_CHILD_ANCHOR, BobguiTextChildAnchorClass))

struct _BobguiTextChildAnchor
{
  GObject parent_instance;

  /*< private >*/
  gpointer segment;
};

struct _BobguiTextChildAnchorClass
{
  GObjectClass parent_class;

  /* Padding for future expansion */
  void (*_bobgui_reserved1) (void);
  void (*_bobgui_reserved2) (void);
  void (*_bobgui_reserved3) (void);
  void (*_bobgui_reserved4) (void);
};

GDK_AVAILABLE_IN_ALL
GType bobgui_text_child_anchor_get_type (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
BobguiTextChildAnchor *bobgui_text_child_anchor_new (void);

GDK_AVAILABLE_IN_4_6
BobguiTextChildAnchor *bobgui_text_child_anchor_new_with_replacement (const char *character);

GDK_AVAILABLE_IN_ALL
BobguiWidget **bobgui_text_child_anchor_get_widgets (BobguiTextChildAnchor *anchor,
                                               guint *out_len);
GDK_AVAILABLE_IN_ALL
gboolean bobgui_text_child_anchor_get_deleted (BobguiTextChildAnchor *anchor);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiTextChildAnchor, g_object_unref)

G_END_DECLS

