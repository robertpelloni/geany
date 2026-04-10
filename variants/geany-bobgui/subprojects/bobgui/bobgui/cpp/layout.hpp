#ifndef BOBGUI_CPP_LAYOUT_HPP
#define BOBGUI_CPP_LAYOUT_HPP

#include <bobgui/modules/core/layout/bobguilayout.h>
#include "object_handle.hpp"
#include <string>

namespace bobgui {
namespace cpp {

/**
 * LayoutManager: A high-level C++ wrapper for modern Flex/Grid layout engine.
 * Parity: QLayout (Qt6), FlexBox (JUCE), Automatic Layout (U++).
 */
class LayoutManager
{
public:
  explicit LayoutManager(BobguiLayoutType type)
  : manager_(bobgui_layout_manager_new(type))
  {
  }

  BobguiLayoutManager* native() const { return manager_.get(); }

  LayoutManager& flex_direction(const std::string& dir)
  {
    bobgui_layout_set_flex_direction(manager_.get(), dir.c_str());
    return *this;
  }

  LayoutManager& justify_content(const std::string& justify)
  {
    bobgui_layout_set_justify_content(manager_.get(), justify.c_str());
    return *this;
  }

  LayoutManager& align_items(const std::string& align)
  {
    bobgui_layout_set_align_items(manager_.get(), align.c_str());
    return *this;
  }

  LayoutManager& grid_columns(const std::string& cols)
  {
    bobgui_layout_set_grid_template_columns(manager_.get(), cols.c_str());
    return *this;
  }

  LayoutManager& grid_rows(const std::string& rows)
  {
    bobgui_layout_set_grid_template_rows(manager_.get(), rows.c_str());
    return *this;
  }

private:
  ObjectHandle<BobguiLayoutManager> manager_;
};

class FlexLayout : public LayoutManager
{
public:
  FlexLayout() : LayoutManager(BOBGUI_LAYOUT_TYPE_FLEX) {}
};

class GridLayout : public LayoutManager
{
public:
  GridLayout() : LayoutManager(BOBGUI_LAYOUT_TYPE_GRID) {}
};

} /* namespace cpp */
} /* namespace bobgui */

#endif
