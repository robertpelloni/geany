#ifndef BOBGUI_CPP_IPC_HPP
#define BOBGUI_CPP_IPC_HPP

#include <gio/gio.h>
#include "object_handle.hpp"
#include "signal.hpp"
#include <string>

namespace bobgui {
namespace cpp {

/**
 * LocalServer: A high-level IPC server for local communication.
 * Parity: QLocalServer (Qt6).
 */
class LocalServer
{
public:
  LocalServer() 
  : service_(g_socket_service_new())
  {
    g_signal_connect(service_.get(), "incoming", G_CALLBACK(incoming_trampoline), this);
  }

  /**
   * Listen on a named address (Unix socket or Named Pipe).
   */
  bool listen(const std::string& name)
  {
    GError* error = nullptr;
    GSocketAddress* address = g_unix_socket_address_new(name.c_str());
    bool success = g_socket_listener_add_address(G_SOCKET_LISTENER(service_.get()), 
                                                 address, 
                                                 G_SOCKET_TYPE_STREAM, 
                                                 G_SOCKET_PROTOCOL_DEFAULT, 
                                                 nullptr, nullptr, &error);
    g_object_unref(address);
    if (error)
      {
        g_error_free(error);
        return false;
      }
    return success;
  }

  /**
   * Signal emitted when a new connection is received.
   */
  Signal<GSocketConnection*> on_new_connection;

private:
  static gboolean incoming_trampoline(GSocketService* service, 
                                      GSocketConnection* connection, 
                                      GObject* source_object, 
                                      gpointer user_data)
  {
    LocalServer* self = static_cast<LocalServer*>(user_data);
    self->on_new_connection(connection);
    return TRUE;
  }

  ObjectHandle<GSocketService> service_;
};

} /* namespace cpp */
} /* namespace bobgui */

#endif
