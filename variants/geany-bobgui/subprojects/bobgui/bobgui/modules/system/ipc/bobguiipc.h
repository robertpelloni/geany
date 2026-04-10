/* bobgui/modules/system/ipc/bobguiipc.h */
#ifndef BOBGUI_IPC_H
#define BOBGUI_IPC_H

#include <glib-object.h>

G_BEGIN_DECLS

/* High-performance IPC Hub (Better than Qt Remote Objects / DBus) */
#define BOBGUI_TYPE_IPC_MANAGER (bobgui_ipc_manager_get_type ())
G_DECLARE_FINAL_TYPE (BobguiIpcManager, bobgui_ipc_manager, BOBGUI, PACKAGE_MANAGER, GObject)

BobguiIpcManager * bobgui_ipc_manager_new (const char *namespace_id);

/* Shared Memory API (Lock-free / Zero-copy) */
typedef struct _BobguiSharedBuffer BobguiSharedBuffer;
BobguiSharedBuffer * bobgui_ipc_create_shared_buffer (BobguiIpcManager *self, 
                                                    const char *name, 
                                                    size_t size);

/* Ring Buffer for Command Streams (Real-time safe) */
void bobgui_ipc_send_command (BobguiIpcManager *self, 
                             const char *target_process, 
                             uint32_t cmd_id, 
                             GBytes *payload);

/* Synchronized UI State (Unmatched: Share widgets across processes) */
void bobgui_ipc_share_widget_state (BobguiIpcManager *self, 
                                   BobguiWidget *widget, 
                                   const char *global_id);

G_END_DECLS

#endif /* BOBGUI_IPC_H */
