#ifndef BOBGUI_CPP_ACTION_REGISTRY_HPP
#define BOBGUI_CPP_ACTION_REGISTRY_HPP

#include <bobgui/modules/visual/actionregistry/bobguiactionregistry.h>

#include "object_handle.hpp"

#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace bobgui {
namespace cpp {

class ActionRegistry
{
public:
  typedef std::function<void(const std::string &)> ActionHandler;

  struct ActionInfo
  {
    std::string id;
    std::string title;
    std::string subtitle;
    std::string section;
    std::string category;
    std::string shortcut;
    std::string icon_name;
    std::string tags;
    bool checkable;
    bool checked;
  };

  struct ActionSection
  {
    std::string title;
    std::vector<ActionInfo> actions;
  };

  typedef std::function<void(const ActionInfo &)> ActionVisitor;

  struct ActionOptions
  {
    const char *section;
    const char *category;
    const char *shortcut;
    const char *icon_name;
    const char *tags;

    ActionOptions ()
    : section (NULL),
      category (NULL),
      shortcut (NULL),
      icon_name (NULL),
      tags (NULL)
    {
    }
  };

  ActionRegistry ()
  : registry_ (bobgui_action_registry_new ())
  {
  }

  BobguiActionRegistry *native () const
  {
    return registry_.get ();
  }

  void add_action (const char          *action_id,
                   const char          *title,
                   const char          *subtitle,
                   const ActionOptions &options,
                   ActionHandler        handler)
  {
    ActionBinding *binding = add_binding (std::move (handler));

    bobgui_action_registry_add_tagged (registry_.get (),
                                       action_id,
                                       title,
                                       subtitle,
                                       options.section,
                                       options.category,
                                       options.shortcut,
                                       options.icon_name,
                                       options.tags,
                                       &ActionRegistry::action_trampoline,
                                       binding);
  }

  void add_toggle_action (const char          *action_id,
                          const char          *title,
                          const char          *subtitle,
                          const ActionOptions &options,
                          bool                 checked,
                          ActionHandler        handler)
  {
    ActionBinding *binding = add_binding (std::move (handler));

    bobgui_action_registry_add_toggle (registry_.get (),
                                       action_id,
                                       title,
                                       subtitle,
                                       options.section,
                                       options.category,
                                       options.shortcut,
                                       options.icon_name,
                                       checked,
                                       &ActionRegistry::action_trampoline,
                                       binding);
  }

  void activate (const char *action_id)
  {
    bobgui_action_registry_activate (registry_.get (), action_id);
  }

  void set_checked (const char *action_id,
                    bool        checked)
  {
    bobgui_action_registry_set_checked (registry_.get (), action_id, checked);
  }

  bool is_checked (const char *action_id) const
  {
    return bobgui_action_registry_get_checked (registry_.get (), action_id);
  }

  ObjectHandle<GMenuModel> create_menu_model () const
  {
    return ObjectHandle<GMenuModel> (bobgui_action_registry_create_menu_model (registry_.get ()));
  }

  void visit (ActionVisitor visitor) const
  {
    VisitState state = { &visitor };

    bobgui_action_registry_visit (registry_.get (),
                                  &ActionRegistry::visit_trampoline,
                                  &state);
  }

  std::vector<ActionInfo> list_actions () const
  {
    std::vector<ActionInfo> actions;

    visit ([&actions] (const ActionInfo &info) {
      actions.push_back (info);
    });

    return actions;
  }

  std::vector<ActionSection> list_sections () const
  {
    return list_sections_filtered ([] (const ActionInfo &) { return true; });
  }

  std::vector<ActionSection> list_sections_filtered (std::function<bool(const ActionInfo &)> predicate) const
  {
    std::vector<ActionSection> sections;
    std::vector<ActionInfo> actions = list_actions ();

    for (std::vector<ActionInfo>::const_iterator it = actions.begin (); it != actions.end (); ++it)
      {
        if (!predicate (*it))
          continue;

        std::string title = section_title (*it);
        bool found = false;

        for (std::vector<ActionSection>::iterator section = sections.begin (); section != sections.end (); ++section)
          {
            if (section->title == title)
              {
                section->actions.push_back (*it);
                found = true;
                break;
              }
          }

        if (!found)
          {
            ActionSection section;

            section.title = title;
            section.actions.push_back (*it);
            sections.push_back (section);
          }
      }

    return sections;
  }

private:
  struct ActionBinding
  {
    ActionHandler handler;
  };

  struct VisitState
  {
    ActionVisitor *visitor;
  };

  static void action_trampoline (const char *action_id,
                                 gpointer    user_data)
  {
    ActionBinding *binding = static_cast<ActionBinding *> (user_data);

    if (binding->handler)
      binding->handler (action_id != nullptr ? std::string (action_id) : std::string ());
  }

  static void visit_trampoline (const char *action_id,
                                const char *title,
                                const char *subtitle,
                                const char *section,
                                const char *category,
                                const char *shortcut,
                                const char *icon_name,
                                const char *tags,
                                gboolean    checkable,
                                gboolean    checked,
                                gpointer    user_data)
  {
    VisitState *state = static_cast<VisitState *> (user_data);
    ActionInfo info;

    info.id = action_id != NULL ? action_id : "";
    info.title = title != NULL ? title : "";
    info.subtitle = subtitle != NULL ? subtitle : "";
    info.section = section != NULL ? section : "";
    info.category = category != NULL ? category : "";
    info.shortcut = shortcut != NULL ? shortcut : "";
    info.icon_name = icon_name != NULL ? icon_name : "";
    info.tags = tags != NULL ? tags : "";
    info.checkable = checkable;
    info.checked = checked;

    if (state->visitor != NULL && *state->visitor)
      (*state->visitor) (info);
  }

  static std::string section_title (const ActionInfo &info)
  {
    if (!info.section.empty ())
      return info.section;

    if (!info.category.empty ())
      return info.category;

    return "General";
  }

  ActionBinding *add_binding (ActionHandler handler)
  {
    bindings_.push_back (std::unique_ptr<ActionBinding> (new ActionBinding));
    bindings_.back ()->handler = std::move (handler);
    return bindings_.back ().get ();
  }

  ObjectHandle<BobguiActionRegistry> registry_;
  std::vector<std::unique_ptr<ActionBinding> > bindings_;
};

} /* namespace cpp */
} /* namespace bobgui */

#endif
