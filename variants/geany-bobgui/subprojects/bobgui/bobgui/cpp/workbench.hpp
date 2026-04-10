#ifndef BOBGUI_CPP_WORKBENCH_HPP
#define BOBGUI_CPP_WORKBENCH_HPP

#include <bobgui/modules/visual/workbench/bobguiworkbench.h>

#include "action_registry.hpp"
#include "command_palette.hpp"
#include "object_handle.hpp"

#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace bobgui {
namespace cpp {

class Workbench
{
public:
  typedef std::function<void(const std::string &)> CommandHandler;
  typedef ActionRegistry::ActionOptions CommandOptions;

  explicit Workbench (Application &application)
  : workbench_ (bobgui_workbench_new (application.native ()))
  {
  }

  BobguiWorkbench *native () const
  {
    return workbench_.get ();
  }

  BobguiWindow *window () const
  {
    return bobgui_workbench_get_window (workbench_.get ());
  }

  void set_title (const char *title)
  {
    bobgui_workbench_set_title (workbench_.get (), title);
  }

  BobguiWidget *get_central () const
  {
    return bobgui_workbench_get_central (workbench_.get ());
  }

  void set_central (BobguiWidget *widget)
  {
    bobgui_workbench_set_central (workbench_.get (), widget);
  }

  BobguiWidget *get_left_sidebar () const
  {
    return bobgui_workbench_get_left_sidebar (workbench_.get ());
  }

  void set_left_sidebar (BobguiWidget *widget)
  {
    bobgui_workbench_set_left_sidebar (workbench_.get (), widget);
  }

  BobguiWidget *get_right_sidebar () const
  {
    return bobgui_workbench_get_right_sidebar (workbench_.get ());
  }

  void set_right_sidebar (BobguiWidget *widget)
  {
    bobgui_workbench_set_right_sidebar (workbench_.get (), widget);
  }

  void set_status (const char *message)
  {
    bobgui_workbench_set_status (workbench_.get (), message);
  }

  void set_action_registry (ActionRegistry &registry)
  {
    bobgui_workbench_set_action_registry (workbench_.get (), registry.native ());
  }

  void set_command_palette (CommandPalette &palette)
  {
    bobgui_workbench_set_command_palette (workbench_.get (), palette.native ());
  }

  void add_header_action_for_command (const char *label,
                                      const char *command_id)
  {
    bobgui_workbench_add_header_action_for_command (workbench_.get (), label, command_id);
  }

  void enable_menubar (bool enabled)
  {
    bobgui_workbench_enable_menubar (workbench_.get (), enabled);
  }

  void enable_toolbar (bool enabled)
  {
    bobgui_workbench_enable_toolbar (workbench_.get (), enabled);
  }

  void add_command (const char         *command_id,
                    const char         *title,
                    const char         *subtitle,
                    const CommandOptions &options,
                    CommandHandler      handler)
  {
    CommandBinding *binding = add_binding (std::move (handler));

    bobgui_workbench_add_command_sectioned_visual (workbench_.get (),
                                                   command_id,
                                                   title,
                                                   subtitle,
                                                   options.section,
                                                   options.category,
                                                   options.shortcut,
                                                   options.icon_name,
                                                   &Workbench::command_trampoline,
                                                   binding);
  }

  void add_sectioned_command (const char *command_id,
                              const char *title,
                              const char *subtitle,
                              const char *section,
                              const char *category,
                              const char *shortcut,
                              const char *icon_name,
                              CommandHandler handler)
  {
    CommandOptions options;

    options.section = section;
    options.category = category;
    options.shortcut = shortcut;
    options.icon_name = icon_name;

    add_command (command_id, title, subtitle, options, std::move (handler));
  }

  void add_command (const char *command_id,
                    const char *title,
                    const char *subtitle,
                    const char *category,
                    const char *shortcut,
                    const char *icon_name,
                    CommandHandler handler)
  {
    add_sectioned_command (command_id,
                           title,
                           subtitle,
                           NULL,
                           category,
                           shortcut,
                           icon_name,
                           std::move (handler));
  }

  void add_toggle_command (const char          *command_id,
                           const char          *title,
                           const char          *subtitle,
                           const CommandOptions &options,
                           bool                 checked,
                           CommandHandler       handler)
  {
    CommandBinding *binding = add_binding (std::move (handler));

    bobgui_workbench_add_toggle_command_sectioned_visual (workbench_.get (),
                                                          command_id,
                                                          title,
                                                          subtitle,
                                                          options.section,
                                                          options.category,
                                                          options.shortcut,
                                                          options.icon_name,
                                                          checked,
                                                          &Workbench::command_trampoline,
                                                          binding);
  }

  void add_toggle_sectioned_command (const char *command_id,
                                     const char *title,
                                     const char *subtitle,
                                     const char *section,
                                     const char *category,
                                     const char *shortcut,
                                     const char *icon_name,
                                     bool        checked,
                                     CommandHandler handler)
  {
    CommandOptions options;

    options.section = section;
    options.category = category;
    options.shortcut = shortcut;
    options.icon_name = icon_name;

    add_toggle_command (command_id, title, subtitle, options, checked, std::move (handler));
  }

  void add_toggle_command (const char *command_id,
                           const char *title,
                           const char *subtitle,
                           const char *category,
                           const char *shortcut,
                           const char *icon_name,
                           bool        checked,
                           CommandHandler handler)
  {
    add_toggle_sectioned_command (command_id,
                                  title,
                                  subtitle,
                                  NULL,
                                  category,
                                  shortcut,
                                  icon_name,
                                  checked,
                                  std::move (handler));
  }

  void present ()
  {
    bobgui_workbench_present (workbench_.get ());
  }

private:
  struct CommandBinding
  {
    CommandHandler handler;
  };

  static void command_trampoline (const char *command_id,
                                  gpointer    user_data)
  {
    CommandBinding *binding = static_cast<CommandBinding *> (user_data);

    if (binding->handler)
      binding->handler (command_id != nullptr ? std::string (command_id) : std::string ());
  }

  CommandBinding *add_binding (CommandHandler handler)
  {
    command_bindings_.push_back (std::unique_ptr<CommandBinding> (new CommandBinding));
    command_bindings_.back ()->handler = std::move (handler);
    return command_bindings_.back ().get ();
  }

  ObjectHandle<BobguiWorkbench> workbench_;
  std::vector<std::unique_ptr<CommandBinding> > command_bindings_;
};

} /* namespace cpp */
} /* namespace bobgui */

#endif
