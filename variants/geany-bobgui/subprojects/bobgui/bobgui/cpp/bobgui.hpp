#ifndef BOBGUI_CPP_BOBGUI_HPP
#define BOBGUI_CPP_BOBGUI_HPP

// Unified entry point for the Bobgui C++ framework
// Provides access to all framework functionality through organized modules

#include "module.hpp"

// Legacy support - all functionality available in the flattened namespace
// (for backward compatibility during transition)
namespace bobgui {
namespace cpp {
  // Core module exports
  using core::ObjectHandle;
  using core::Property;
  using core::Signal;
  using core::Timer;
  using core::Settings;
  using core::Animation;
  using core::Database;
  
  // System module exports
  using system::FileSystemWatcher;
  using system::LocalServer;
  
  // Network module exports
  using network::Network;
  using network::WebSocket;
  
  // Visual module exports
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
  
  // Convenience aliases for common patterns
  namespace core   = bobgui::core;
  namespace system = bobgui::system;
  namespace network = bobgui::network;
  namespace visual = bobgui::visual;
  namespace media  = bobgui::media;
  namespace tools  = bobgui::tools;
}
}

#endif // BOBGUI_CPP_BOBGUI_HPP