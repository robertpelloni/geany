/*
 * Copyright © 2014 Codethink Limited
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the licence, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Ryan Lortie <desrt@desrt.ca>
 */

#pragma once

#include <bobgui/bobguimenutrackeritemprivate.h>
#include <bobgui/bobguibox.h>
#include <bobgui/bobguipopovermenu.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_MENU_SECTION_BOX                           (bobgui_menu_section_box_get_type ())
#define BOBGUI_MENU_SECTION_BOX(inst)                          (G_TYPE_CHECK_INSTANCE_CAST ((inst),                     \
                                                             BOBGUI_TYPE_MENU_SECTION_BOX, BobguiMenuSectionBox))
#define BOBGUI_MENU_SECTION_BOX_CLASS(class)                   (G_TYPE_CHECK_CLASS_CAST ((class),                       \
                                                             BOBGUI_TYPE_MENU_SECTION_BOX, BobguiMenuSectionBoxClass))
#define BOBGUI_IS_MENU_SECTION_BOX(inst)                       (G_TYPE_CHECK_INSTANCE_TYPE ((inst),                     \
                                                             BOBGUI_TYPE_MENU_SECTION_BOX))
#define BOBGUI_IS_MENU_SECTION_BOX_CLASS(class)                (G_TYPE_CHECK_CLASS_TYPE ((class),                       \
                                                             BOBGUI_TYPE_MENU_SECTION_BOX))
#define BOBGUI_MENU_SECTION_BOX_GET_CLASS(inst)                (G_TYPE_INSTANCE_GET_CLASS ((inst),                      \
                                                             BOBGUI_TYPE_MENU_SECTION_BOX, BobguiMenuSectionBoxClass))

typedef struct _BobguiMenuSectionBox                           BobguiMenuSectionBox;

GType                   bobgui_menu_section_box_get_type                   (void) G_GNUC_CONST;
void                    bobgui_menu_section_box_new_toplevel               (BobguiPopoverMenu      *popover,
                                                                         GMenuModel          *model,
                                                                         BobguiPopoverMenuFlags  flags);

gboolean                bobgui_menu_section_box_add_custom                 (BobguiPopoverMenu *popover,
                                                                         BobguiWidget      *child,
                                                                         const char     *id);

gboolean                bobgui_menu_section_box_remove_custom              (BobguiPopoverMenu *popover,
                                                                         BobguiWidget      *child);

G_END_DECLS

