#ifndef BOBGUI_CPP_MODULE_VISUAL_HPP
#define BOBGUI_CPP_MODULE_VISUAL_HPP

// Visual module: Widgets, graphics, layout, workbench, shells, etc.
// This maps to bobgui/modules/visual/

namespace bobgui {
namespace visual {
  // Re-export the visual C++ utilities
  using bobgui::cpp::Widget;
  using bobgui::cpp::Canvas;
  using bobgui::cpp::Graphics;
  using bobgui::cpp::Layout;
  using bobgui::cpp::Workbench;
  using bobgui::cpp::AppShell;
  using bobgui::cpp::StudioShell;
  using bobgui::cpp::DocumentShell;
  using bobgui::cpp::DashboardShell;
  using bobgui::cpp::CommandPalette;
  using bobgui::cpp::ToolSurface;
  using bobgui::cpp::ToolSurfaceBuilder;
  using bobgui::cpp::ToolbarBuilder;
  
  // Visual submodules will be mapped here as needed
  // e.g., namespace commandpalette { /* bobgui::modules::visual::commandpalette::* */ }
}
}

#endif // BOBGUI_CPP_MODULE_VISUAL_HPP