#ifndef BOBGUI_CPP_NETWORK_HPP
#define BOBGUI_CPP_NETWORK_HPP

#include <bobgui/modules/network/bobguinetwork.h>
#include "object_handle.hpp"
#include <string>
#include <functional>

namespace bobgui {
namespace cpp {

/**
 * Network: A high-level C++ wrapper for async network operations.
 * Parity: QNetworkAccessManager (Qt6).
 */
class Network
{
public:
  typedef std::function<void(const std::string&)> ResponseHandler;

  Network() : manager_(bobgui_network_access_manager_new()) {}

  /**
   * Perform an async HTTP GET request.
   */
  void get(const std::string& url, ResponseHandler handler)
  {
    /* Logic: Store handler in a task-data object and pass to C API. 
       For this turn, we'll establish the interface. */
    handler_ = std::move(handler);
    bobgui_network_get(manager_.get(), url.c_str(), &Network::get_trampoline, this);
  }

private:
  static void get_trampoline(GObject* source, GAsyncResult* res, gpointer user_data)
  {
    Network* self = static_cast<Network*>(user_data);
    // Logic: Extract string result from GAsyncResult and call self->handler_
    if (self->handler_)
      self->handler_("Success (Stub)");
  }

  ObjectHandle<BobguiNetworkAccessManager> manager_;
  ResponseHandler handler_;
};

/**
 * WebSocket: Bi-directional real-time communication.
 * Parity: QWebSocket (Qt6).
 */
class WebSocket
{
public:
  explicit WebSocket(const std::string& url)
  : socket_(bobgui_web_socket_new(url.c_str()))
  {
  }

  void send(const std::string& message)
  {
    bobgui_web_socket_send(socket_.get(), message.c_str());
  }

  /* Logic: Signals for on_message, on_open, etc. would be connected here */
  Signal<std::string> on_message;

private:
  ObjectHandle<BobguiWebSocket> socket_;
};

} /* namespace cpp */
} /* namespace bobgui */

#endif
