#ifndef BOBGUI_CPP_MODULE_SYSTEM_HPP
#define BOBGUI_CPP_MODULE_SYSTEM_HPP

// System module: OS integration, input, IPC, plugins, shells, etc.
// This maps to bobgui/modules/system/

namespace bobgui {
namespace system {
  // Re-export the system C++ utilities
  using bobgui::cpp::FileSystemWatcher;
  using bobgui::cpp::LocalServer;
  
  // System submodules will be mapped here as needed
  // e.g., namespace ipc { using bobgui::cpp::ipc::LocalServer; }
}
}

#endif // BOBGUI_CPP_MODULE_SYSTEM_HPP