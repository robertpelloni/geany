/* BOBGUI - The Bobgui Framework
 * bobguitrashmonitor.h: Monitor the trash:/// folder to see if there is trash or not
 * Copyright (C) 2011 Suse
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Federico Mena Quintero <federico@gnome.org>
 */

#pragma once

#include <gio/gio.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_TRASH_MONITOR			(_bobgui_trash_monitor_get_type ())
#define BOBGUI_TRASH_MONITOR(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_TRASH_MONITOR, BobguiTrashMonitor))
#define BOBGUI_TRASH_MONITOR_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), BOBGUI_TYPE_TRASH_MONITOR, BobguiTrashMonitorClass))
#define BOBGUI_IS_TRASH_MONITOR(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_TRASH_MONITOR))
#define BOBGUI_IS_TRASH_MONITOR_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), BOBGUI_TYPE_TRASH_MONITOR))
#define BOBGUI_TRASH_MONITOR_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_TRASH_MONITOR, BobguiTrashMonitorClass))

typedef struct _BobguiTrashMonitor BobguiTrashMonitor;
typedef struct _BobguiTrashMonitorClass BobguiTrashMonitorClass;

GType _bobgui_trash_monitor_get_type (void);
BobguiTrashMonitor *_bobgui_trash_monitor_get (void);

GIcon *_bobgui_trash_monitor_get_icon (BobguiTrashMonitor *monitor);

gboolean _bobgui_trash_monitor_get_has_trash (BobguiTrashMonitor *monitor);

G_END_DECLS

