/* bobguipopoverbin.h: A single-child container with a popover
 *
 * SPDX-FileCopyrightText: 2025  Emmanuele Bassi
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <bobgui/bobguiwidget.h>
#include <bobgui/bobguipopover.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_POPOVER_BIN (bobgui_popover_bin_get_type())

GDK_AVAILABLE_IN_4_22
G_DECLARE_FINAL_TYPE (BobguiPopoverBin, bobgui_popover_bin, BOBGUI, POPOVER_BIN, BobguiWidget)

GDK_AVAILABLE_IN_4_22
BobguiWidget *     bobgui_popover_bin_new             (void);

GDK_AVAILABLE_IN_4_22
void            bobgui_popover_bin_set_child       (BobguiPopoverBin *self,
                                                 BobguiWidget     *child);
GDK_AVAILABLE_IN_4_22
BobguiWidget *     bobgui_popover_bin_get_child       (BobguiPopoverBin *self);

GDK_AVAILABLE_IN_4_22
void            bobgui_popover_bin_set_menu_model  (BobguiPopoverBin *self,
                                                 GMenuModel    *model);
GDK_AVAILABLE_IN_4_22
GMenuModel *    bobgui_popover_bin_get_menu_model  (BobguiPopoverBin *self);
GDK_AVAILABLE_IN_4_22
void            bobgui_popover_bin_set_popover     (BobguiPopoverBin *self,
                                                 BobguiWidget     *popover);
GDK_AVAILABLE_IN_4_22
BobguiWidget *     bobgui_popover_bin_get_popover     (BobguiPopoverBin *self);
GDK_AVAILABLE_IN_4_22
void            bobgui_popover_bin_popup           (BobguiPopoverBin *self);
GDK_AVAILABLE_IN_4_22
void            bobgui_popover_bin_popdown         (BobguiPopoverBin *self);

GDK_AVAILABLE_IN_4_22
void            bobgui_popover_bin_set_handle_input (BobguiPopoverBin *self,
                                                  gboolean       handle_input);
GDK_AVAILABLE_IN_4_22
gboolean        bobgui_popover_bin_get_handle_input (BobguiPopoverBin *self);

G_END_DECLS
