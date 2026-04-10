#ifndef BOBGUI_VFS_H
#define BOBGUI_VFS_H

#include <gio/gio.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_VFS (bobgui_vfs_get_type ())
G_DECLARE_FINAL_TYPE (BobguiVFS, bobgui_vfs, BOBGUI, VFS, GObject)

BobguiVFS * bobgui_vfs_get_default (void);

G_END_DECLS

#endif
