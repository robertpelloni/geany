#ifndef BOBGUI_CPP_MODULE_CORE_HPP
#define BOBGUI_CPP_MODULE_CORE_HPP

// Core module: Fundamental types, utilities, data structures, reflection, etc.
// This maps to bobgui/modules/core/

namespace bobgui {
namespace core {
  // Re-export the core C++ utilities
  using bobgui::cpp::ObjectHandle;
  using bobgui::cpp::Property;
  using bobgui::cpp::Signal;
  using bobgui::cpp::Timer;
  using bobgui::cpp::Settings;
  using bobgui::cpp::Animation;
  using bobgui::cpp::Database;
  
  // Core submodules will be mapped here as needed
  // e.g., namespace data { using bobgui::cpp::database::Database; }
}
}

#endif // BOBGUI_CPP_MODULE_CORE_HPP