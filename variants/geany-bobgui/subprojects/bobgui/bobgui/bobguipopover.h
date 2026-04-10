/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2019 Red Hat, Inc.
 *
 * Authors:
 * - Matthias Clasen <mclasen@redhat.com>
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

#pragma once

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <bobgui/bobguiwidget.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_POPOVER                 (bobgui_popover_get_type ())
#define BOBGUI_POPOVER(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_POPOVER, BobguiPopover))
#define BOBGUI_POPOVER_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), BOBGUI_TYPE_POPOVER, BobguiPopoverClass))
#define BOBGUI_IS_POPOVER(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_POPOVER))
#define BOBGUI_IS_POPOVER_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), BOBGUI_TYPE_POPOVER))
#define BOBGUI_POPOVER_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_POPOVER, BobguiPopoverClass))

typedef struct _BobguiPopover       BobguiPopover;
typedef struct _BobguiPopoverClass  BobguiPopoverClass;

struct _BobguiPopover
{
  BobguiWidget parent;
};

struct _BobguiPopoverClass
{
  BobguiWidgetClass parent_class;

  void (* closed)           (BobguiPopover *popover);
  void (* activate_default) (BobguiPopover *popover);

  /*< private >*/

  gpointer reserved[8];
};

GDK_AVAILABLE_IN_ALL
GType           bobgui_popover_get_type (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
BobguiWidget *     bobgui_popover_new             (void);

GDK_AVAILABLE_IN_ALL
void            bobgui_popover_set_child       (BobguiPopover         *popover,
                                             BobguiWidget          *child);
GDK_AVAILABLE_IN_ALL
BobguiWidget *     bobgui_popover_get_child       (BobguiPopover         *popover);

GDK_AVAILABLE_IN_ALL
void            bobgui_popover_set_pointing_to (BobguiPopover         *popover,
                                             const GdkRectangle *rect);
GDK_AVAILABLE_IN_ALL
gboolean        bobgui_popover_get_pointing_to (BobguiPopover         *popover,
                                             GdkRectangle       *rect);
GDK_AVAILABLE_IN_ALL
void            bobgui_popover_set_position    (BobguiPopover         *popover,
                                             BobguiPositionType     position);
GDK_AVAILABLE_IN_ALL
BobguiPositionType bobgui_popover_get_position    (BobguiPopover         *popover);

GDK_AVAILABLE_IN_ALL
void            bobgui_popover_set_autohide    (BobguiPopover         *popover,
                                             gboolean            autohide);
GDK_AVAILABLE_IN_ALL
gboolean        bobgui_popover_get_autohide    (BobguiPopover         *popover);

GDK_AVAILABLE_IN_ALL
void            bobgui_popover_set_has_arrow   (BobguiPopover         *popover,
                                             gboolean            has_arrow);
GDK_AVAILABLE_IN_ALL
gboolean        bobgui_popover_get_has_arrow   (BobguiPopover         *popover);

GDK_AVAILABLE_IN_ALL
void            bobgui_popover_set_mnemonics_visible (BobguiPopover   *popover,
                                                   gboolean      mnemonics_visible);
GDK_AVAILABLE_IN_ALL
gboolean        bobgui_popover_get_mnemonics_visible (BobguiPopover   *popover);

GDK_AVAILABLE_IN_ALL
void            bobgui_popover_popup (BobguiPopover *popover);
GDK_AVAILABLE_IN_ALL
void            bobgui_popover_popdown (BobguiPopover *popover);

GDK_AVAILABLE_IN_ALL
void            bobgui_popover_set_offset (BobguiPopover *popover,
                                        int         x_offset,
                                        int         y_offset);
GDK_AVAILABLE_IN_ALL
void            bobgui_popover_get_offset (BobguiPopover *popover,
                                        int        *x_offset,
                                        int        *y_offset);
GDK_AVAILABLE_IN_ALL
void            bobgui_popover_set_cascade_popdown (BobguiPopover *popover,
                                                 gboolean    cascade_popdown);
GDK_AVAILABLE_IN_ALL
gboolean        bobgui_popover_get_cascade_popdown (BobguiPopover *popover);

GDK_AVAILABLE_IN_ALL
void bobgui_popover_set_default_widget (BobguiPopover *popover,
                                     BobguiWidget  *widget);

GDK_AVAILABLE_IN_ALL
void bobgui_popover_present (BobguiPopover *popover);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiPopover, g_object_unref)

G_END_DECLS

