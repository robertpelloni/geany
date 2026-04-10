#ifndef BOBGUI_CPP_MODULE_NETWORK_HPP
#define BOBGUI_CPP_MODULE_NETWORK_HPP

// Network module: HTTP, WebSocket, remote services, etc.
// This maps to bobgui/modules/network/

namespace bobgui {
namespace network {
  // Re-export the network C++ utilities
  using bobgui::cpp::Network;
  using bobgui::cpp::WebSocket;
  
  // Network submodules will be mapped here as needed
  // e.g., namespace web { /* bobgui::modules::network::web::* */ }
}
}

#endif // BOBGUI_CPP_MODULE_NETWORK_HPP