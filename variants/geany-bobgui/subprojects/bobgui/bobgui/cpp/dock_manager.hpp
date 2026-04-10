#ifndef BOBGUI_CPP_DOCK_MANAGER_HPP
#define BOBGUI_CPP_DOCK_MANAGER_HPP

#include <bobgui/modules/visual/dock/bobguidock.h>

#include "object_handle.hpp"

namespace bobgui {
namespace cpp {

class DockManager
{
public:
  explicit DockManager (BobguiWindow *window)
  : manager_ (bobgui_dock_manager_new (window))
  {
  }

  BobguiDockManager *native () const
  {
    return manager_.get ();
  }

  void add_widget (const Widget& widget, BobguiDockPosition pos, const std::string& title)
  {
    bobgui_dock_manager_add_widget (manager_.get (), widget.native (), pos, title.c_str ());
  }

  void save_layout (const std::string& path)
  {
    bobgui_dock_manager_save_layout (manager_.get (), path.c_str ());
  }

  void load_layout (const std::string& path)
  {
    bobgui_dock_manager_load_layout (manager_.get (), path.c_str ());
  }

private:
  ObjectHandle<BobguiDockManager> manager_;
};

} /* namespace cpp */
} /* namespace bobgui */

#endif
