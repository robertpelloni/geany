/* bobgui/network/bobguinetwork.h */
#ifndef BOBGUI_NETWORK_H
#define BOBGUI_NETWORK_H

#include <gio/gio.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_NETWORK_ACCESS_MANAGER (bobgui_network_access_manager_get_type ())
G_DECLARE_FINAL_TYPE (BobguiNetworkAccessManager, bobgui_network_access_manager, BOBGUI, NETWORK_ACCESS_MANAGER, GObject)

BobguiNetworkAccessManager * bobgui_network_access_manager_new (void);

/* Advanced Networking API (Qt6 Parity) */
void bobgui_network_get (BobguiNetworkAccessManager *self, const char *url, GAsyncReadyCallback callback, gpointer user_data);
void bobgui_network_post (BobguiNetworkAccessManager *self, const char *url, const char *data, GAsyncReadyCallback callback, gpointer user_data);

/* WebSocket Support (Qt6 Parity) */
typedef struct _BobguiWebSocket BobguiWebSocket;
BobguiWebSocket * bobgui_web_socket_new (const char *url);
void              bobgui_web_socket_send (BobguiWebSocket *self, const char *message);

G_END_DECLS

#endif /* BOBGUI_NETWORK_H */
