#ifndef BOBGUI_CPP_MODULE_HPP
#define BOBGUI_CPP_MODULE_HPP

// Unified module interface for the Bobgui framework
// Provides access to all six pillars: Core, System, Network, Visual, Media, Tools

#include "module/core.hpp"
#include "module/system.hpp"
#include "module/network.hpp"
#include "module/visual.hpp"
#include "module/media.hpp"
#include "module/tools.hpp"

namespace bobgui {
namespace cpp {
  // Module access for organized API usage
  namespace core   = bobgui::core;
  namespace system = bobgui::system;
  namespace network = bobgui::network;
  namespace visual = bobgui::visual;
  namespace media  = bobgui::media;
  namespace tools  = bobgui::tools;
  
  // Legacy flat namespace support (deprecated but maintained for compatibility)
  using core::ObjectHandle;
  using core::Property;
  using core::Signal;
  using core::Timer;
  using core::Settings;
  using core::Animation;
  using core::Database;
  
  using system::FileSystemWatcher;
  using system::LocalServer;
  
  using network::Network;
  using network::WebSocket;
  
  using visual::Widget;
  using visual::Canvas;
  using visual::Graphics;
  using visual::Layout;
  using visual::Workbench;
  using visual::AppShell;
  using visual::StudioShell;
  using visual::DocumentShell;
  using visual::DashboardShell;
  using visual::CommandPalette;
  using visual::ToolSurface;
  using visual::ToolSurfaceBuilder;
  using visual::ToolbarBuilder;
}
}

#endif // BOBGUI_CPP_MODULE_HPP