#ifndef BOBGUI_CPP_DOCUMENT_SHELL_HPP
#define BOBGUI_CPP_DOCUMENT_SHELL_HPP

#include "app_shell.hpp"

#include <utility>
#include <vector>

namespace bobgui {
namespace cpp {

class DocumentShell
{
public:
  explicit DocumentShell (Application &application)
  : shell_ (application)
  {
  }

  AppShell &shell ()
  {
    return shell_;
  }

  Workbench &workbench ()
  {
    return shell_.workbench ();
  }

  ActionRegistry &actions ()
  {
    return shell_.actions ();
  }

  CommandPalette &palette ()
  {
    return shell_.palette ();
  }

  void set_title (const char *title)
  {
    shell_.set_title (title);
  }

  void set_outline_panel (BobguiWidget *widget)
  {
    shell_.set_left_sidebar (widget);
  }

  void set_content_view (BobguiWidget *widget)
  {
    shell_.set_central (widget);
  }

  void set_details_panel (BobguiWidget *widget)
  {
    shell_.set_right_sidebar (widget);
  }

  void set_outline_panel_visible (bool visible)
  {
    shell_.set_left_sidebar_visible (visible);
  }

  void set_details_panel_visible (bool visible)
  {
    shell_.set_right_sidebar_visible (visible);
  }

  void set_status (const char *message)
  {
    shell_.set_status (message);
  }

  void pin_command (const char *command_id,
                    bool        pinned)
  {
    shell_.pin_command (command_id, pinned);
  }

  void ensure_dock_manager ()
  {
    shell_.ensure_dock_manager ();
  }

  void add_header_action_for_command (const char *label,
                                      const char *command_id)
  {
    shell_.add_header_action_for_command (label, command_id);
  }

  void enable_menubar (bool enabled)
  {
    shell_.enable_menubar (enabled);
  }

  void enable_toolbar (bool enabled)
  {
    shell_.enable_toolbar (enabled);
  }

  void add_document_command (const char               *command_id,
                             const char               *title,
                             const char               *subtitle,
                             const char               *shortcut,
                             const char               *icon_name,
                             Workbench::CommandHandler handler)
  {
    Workbench::CommandOptions options;

    options.section = "Document";
    options.category = "Document";
    options.shortcut = shortcut;
    options.icon_name = icon_name;

    shell_.add_command (command_id,
                        title,
                        subtitle,
                        options,
                        std::move (handler));
  }

  void add_workspace_command (const char               *command_id,
                              const char               *title,
                              const char               *subtitle,
                              const char               *shortcut,
                              const char               *icon_name,
                              Workbench::CommandHandler handler)
  {
    shell_.add_workspace_command (command_id,
                                  title,
                                  subtitle,
                                  shortcut,
                                  icon_name,
                                  std::move (handler));
  }

  void add_panel_toggle_command (const char               *command_id,
                                 const char               *title,
                                 const char               *subtitle,
                                 const char               *shortcut,
                                 const char               *icon_name,
                                 bool                      checked,
                                 Workbench::CommandHandler handler)
  {
    shell_.add_panel_toggle_command (command_id,
                                     title,
                                     subtitle,
                                     shortcut,
                                     icon_name,
                                     checked,
                                     std::move (handler));
  }

  ToolSurfaceModel document_tool_surface_model () const
  {
    std::vector<std::string> titles;

    titles.push_back ("Document");
    return shell_.filtered_tool_surface_model (titles);
  }

  BobguiWidget *build_document_toolbar_widget ()
  {
    return shell_.build_filtered_toolbar_widget (std::vector<std::string> (1, "Document"),
                                                 ToolbarBuilder::Options::labeled ());
  }

  BobguiWidget *build_document_panel_toolbar_widget ()
  {
    return shell_.build_panel_toolbar_preset ();
  }

  BobguiWidget *build_document_tools_widget ()
  {
    return shell_.build_filtered_tool_surface_widget (std::vector<std::string> (1, "Document"),
                                                      ToolSurfaceBuilder::Options::detailed ());
  }

  std::size_t document_action_count () const
  {
    return document_tool_surface_model ().item_count ();
  }

  BobguiWidget *build_document_panel_tools_widget ()
  {
    return shell_.build_panel_tool_surface_preset ();
  }

  void present ()
  {
    shell_.present ();
  }

private:
  AppShell shell_;
};

} /* namespace cpp */
} /* namespace bobgui */

#endif
