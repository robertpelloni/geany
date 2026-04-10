/* BOBGUI - The Bobgui Framework
 * Copyright (C) Christian Kellner <gicmo@gnome.org>
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

G_BEGIN_DECLS

#define BOBGUI_TYPE_MOUNT_OPERATION         (bobgui_mount_operation_get_type ())
#define BOBGUI_MOUNT_OPERATION(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), BOBGUI_TYPE_MOUNT_OPERATION, BobguiMountOperation))
#define BOBGUI_MOUNT_OPERATION_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), BOBGUI_TYPE_MOUNT_OPERATION, BobguiMountOperationClass))
#define BOBGUI_IS_MOUNT_OPERATION(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), BOBGUI_TYPE_MOUNT_OPERATION))
#define BOBGUI_IS_MOUNT_OPERATION_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), BOBGUI_TYPE_MOUNT_OPERATION))
#define BOBGUI_MOUNT_OPERATION_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), BOBGUI_TYPE_MOUNT_OPERATION, BobguiMountOperationClass))

typedef struct _BobguiMountOperation         BobguiMountOperation;
typedef struct _BobguiMountOperationClass    BobguiMountOperationClass;
typedef struct _BobguiMountOperationPrivate  BobguiMountOperationPrivate;

struct _BobguiMountOperation
{
  GMountOperation parent_instance;

  BobguiMountOperationPrivate *priv;
};

/**
 * BobguiMountOperationClass:
 * @parent_class: The parent class.
 */
struct _BobguiMountOperationClass
{
  GMountOperationClass parent_class;

  /*< private >*/

  /* Padding for future expansion */
  void (*_bobgui_reserved1) (void);
  void (*_bobgui_reserved2) (void);
  void (*_bobgui_reserved3) (void);
  void (*_bobgui_reserved4) (void);
};


GDK_AVAILABLE_IN_ALL
GType            bobgui_mount_operation_get_type   (void);
GDK_AVAILABLE_IN_ALL
GMountOperation *bobgui_mount_operation_new        (BobguiWindow         *parent);
GDK_AVAILABLE_IN_ALL
gboolean         bobgui_mount_operation_is_showing (BobguiMountOperation *op);
GDK_AVAILABLE_IN_ALL
void             bobgui_mount_operation_set_parent (BobguiMountOperation *op,
                                                 BobguiWindow         *parent);
GDK_AVAILABLE_IN_ALL
BobguiWindow *      bobgui_mount_operation_get_parent (BobguiMountOperation *op);
GDK_AVAILABLE_IN_ALL
void             bobgui_mount_operation_set_display(BobguiMountOperation *op,
                                                 GdkDisplay        *display);
GDK_AVAILABLE_IN_ALL
GdkDisplay *     bobgui_mount_operation_get_display(BobguiMountOperation *op);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiMountOperation, g_object_unref)

G_END_DECLS

