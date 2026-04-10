#ifndef BOBGUI_CPP_TOOL_SURFACE_HPP
#define BOBGUI_CPP_TOOL_SURFACE_HPP

#include "action_registry.hpp"

#include <string>
#include <vector>

namespace bobgui {
namespace cpp {

class ToolSurfaceModel
{
public:
  struct ToolItem
  {
    std::string action_id;
    std::string title;
    std::string subtitle;
    std::string shortcut;
    std::string icon_name;
    bool checkable;
    bool checked;
  };

  struct ToolSection
  {
    std::string title;
    std::vector<ToolItem> items;
  };

  ToolSurfaceModel ()
  {
  }

  static ToolSurfaceModel from_action_sections (const std::vector<ActionRegistry::ActionSection> &sections)
  {
    ToolSurfaceModel model;

    for (std::vector<ActionRegistry::ActionSection>::const_iterator it = sections.begin (); it != sections.end (); ++it)
      {
        ToolSection section;

        section.title = it->title;

        for (std::vector<ActionRegistry::ActionInfo>::const_iterator action = it->actions.begin (); action != it->actions.end (); ++action)
          {
            ToolItem item;

            item.action_id = action->id;
            item.title = action->title;
            item.subtitle = action->subtitle;
            item.shortcut = action->shortcut;
            item.icon_name = action->icon_name;
            item.checkable = action->checkable;
            item.checked = action->checked;
            section.items.push_back (item);
          }

        model.sections_.push_back (section);
      }

    return model;
  }

  const std::vector<ToolSection> &sections () const
  {
    return sections_;
  }

  ToolSurfaceModel filter_sections (const std::vector<std::string> &titles) const
  {
    ToolSurfaceModel filtered;

    for (std::vector<ToolSection>::const_iterator section = sections_.begin (); section != sections_.end (); ++section)
      {
        for (std::vector<std::string>::const_iterator title = titles.begin (); title != titles.end (); ++title)
          {
            if (section->title == *title)
              {
                filtered.sections_.push_back (*section);
                break;
              }
          }
      }

    return filtered;
  }

  std::size_t section_count () const
  {
    return sections_.size ();
  }

  std::size_t item_count () const
  {
    std::size_t count = 0;

    for (std::vector<ToolSection>::const_iterator section = sections_.begin (); section != sections_.end (); ++section)
      count += section->items.size ();

    return count;
  }

  bool empty () const
  {
    return sections_.empty ();
  }

private:
  std::vector<ToolSection> sections_;
};

} /* namespace cpp */
} /* namespace bobgui */

#endif
