/* bobgui/modules/remote/bobguiremote.h */
#ifndef BOBGUI_REMOTE_H
#define BOBGUI_REMOTE_H

#include <bobgui/bobgui.h>

G_BEGIN_DECLS

/* Remote UI Protocol (Better than Broadway / Qt Remote) */
#define BOBGUI_TYPE_REMOTE_SERVER (bobgui_remote_server_get_type ())
G_DECLARE_FINAL_TYPE (BobguiRemoteServer, bobgui_remote_server, BOBGUI, REMOTE_SERVER, GObject)

/* Accelerated Stream (H.264/H.265 encoding via GPU) */
BobguiRemoteServer * bobgui_remote_server_new (int port, const char *codec);
void                  bobgui_remote_server_start (BobguiRemoteServer *self, BobguiWidget *ui_root);

/* Zero-Latency Interaction Protocol */
void bobgui_remote_server_set_quality (BobguiRemoteServer *self, int bit_rate, int frame_rate);

/* Remote Client API */
typedef struct _BobguiRemoteClient BobguiRemoteClient;
BobguiRemoteClient * bobgui_remote_client_new (const char *host, int port);
void                  bobgui_remote_client_connect (BobguiRemoteClient *self, BobguiWidget *local_container);

G_END_DECLS

#endif /* BOBGUI_REMOTE_H */
