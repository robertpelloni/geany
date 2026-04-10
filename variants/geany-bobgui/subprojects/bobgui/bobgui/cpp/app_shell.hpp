#ifndef BOBGUI_CPP_APP_SHELL_HPP
#define BOBGUI_CPP_APP_SHELL_HPP

#include "dock_manager.hpp"
#include "tool_surface.hpp"
#include "tool_surface_builder.hpp"
#include "toolbar_builder.hpp"
#include "workbench.hpp"

#include <memory>
#include <utility>
#include <vector>

namespace bobgui {
namespace cpp {

class AppShell
{
public:
  explicit AppShell (Application &application)
  : workbench_ (application),
    action_registry_ (),
    command_palette_ (application)
  {
    workbench_.set_action_registry (action_registry_);
    workbench_.set_command_palette (command_palette_);
  }

  Workbench &workbench ()
  {
    return workbench_;
  }

  ActionRegistry &actions ()
  {
    return action_registry_;
  }

  CommandPalette &palette ()
  {
    return command_palette_;
  }

  bool has_dock_manager () const
  {
    return dock_manager_.get () != NULL;
  }

  void ensure_dock_manager ()
  {
    dock_manager ();
  }

  DockManager &dock_manager ()
  {
    if (!dock_manager_)
      dock_manager_.reset (new DockManager (workbench_.window ()));

    return *dock_manager_;
  }

  void set_title (const char *title)
  {
    workbench_.set_title (title);
  }

  void set_central (BobguiWidget *widget)
  {
    workbench_.set_central (widget);
  }

  void set_left_sidebar (BobguiWidget *widget)
  {
    workbench_.set_left_sidebar (widget);
  }

  void set_right_sidebar (BobguiWidget *widget)
  {
    workbench_.set_right_sidebar (widget);
  }

  void set_status (const char *message)
  {
    workbench_.set_status (message);
  }

  void pin_command (const char *command_id,
                    bool        pinned)
  {
    command_palette_.set_pinned (command_id, pinned);
  }

  ObjectHandle<GMenuModel> menu_model () const
  {
    return action_registry_.create_menu_model ();
  }

  void visit_actions (ActionRegistry::ActionVisitor visitor) const
  {
    action_registry_.visit (std::move (visitor));
  }

  std::vector<ActionRegistry::ActionInfo> list_actions () const
  {
    return action_registry_.list_actions ();
  }

  std::vector<ActionRegistry::ActionSection> list_action_sections () const
  {
    return action_registry_.list_sections ();
  }

  ToolSurfaceModel tool_surface_model () const
  {
    return ToolSurfaceModel::from_action_sections (list_action_sections ());
  }

  ToolSurfaceModel filtered_tool_surface_model (const std::vector<std::string> &titles) const
  {
    return tool_surface_model ().filter_sections (titles);
  }

  ToolSurfaceModel workspace_tool_surface_model () const
  {
    std::vector<std::string> titles;

    titles.push_back ("Workspace");
    return filtered_tool_surface_model (titles);
  }

  ToolSurfaceModel panel_tool_surface_model () const
  {
    std::vector<std::string> titles;

    titles.push_back ("Panels");
    titles.push_back ("View");
    return filtered_tool_surface_model (titles);
  }

  std::size_t workspace_action_count () const
  {
    return workspace_tool_surface_model ().item_count ();
  }

  std::size_t panel_action_count () const
  {
    return panel_tool_surface_model ().item_count ();
  }

  bool has_workspace_actions () const
  {
    return workspace_action_count () > 0;
  }

  bool has_panel_actions () const
  {
    return panel_action_count () > 0;
  }

  BobguiWidget *build_tool_surface_widget (const ToolSurfaceBuilder::Options &options = ToolSurfaceBuilder::Options ())
  {
    ToolSurfaceBuilder builder (action_registry_);
    return builder.build_widget (tool_surface_model (), options);
  }

  BobguiWidget *build_filtered_tool_surface_widget (const std::vector<std::string> &titles,
                                                    const ToolSurfaceBuilder::Options &options = ToolSurfaceBuilder::Options ())
  {
    ToolSurfaceBuilder builder (action_registry_);
    return builder.build_widget (filtered_tool_surface_model (titles), options);
  }

  BobguiWidget *build_workspace_tool_surface_widget (const ToolSurfaceBuilder::Options &options = ToolSurfaceBuilder::Options ())
  {
    ToolSurfaceBuilder builder (action_registry_);
    return builder.build_widget (workspace_tool_surface_model (), options);
  }

  BobguiWidget *build_panel_tool_surface_widget (const ToolSurfaceBuilder::Options &options = ToolSurfaceBuilder::Options ())
  {
    ToolSurfaceBuilder builder (action_registry_);
    return builder.build_widget (panel_tool_surface_model (), options);
  }

  BobguiWidget *build_descriptive_tool_surface_widget ()
  {
    return build_tool_surface_widget (ToolSurfaceBuilder::Options::detailed ());
  }

  BobguiWidget *build_workspace_tool_surface_preset ()
  {
    return build_workspace_tool_surface_widget (ToolSurfaceBuilder::Options::detailed ());
  }

  BobguiWidget *build_panel_tool_surface_preset ()
  {
    return build_panel_tool_surface_widget (ToolSurfaceBuilder::Options::detailed ());
  }

  BobguiWidget *build_toolbar_widget (const ToolbarBuilder::Options &options = ToolbarBuilder::Options ())
  {
    ToolbarBuilder builder (action_registry_);
    return builder.build_widget (tool_surface_model (), options);
  }

  BobguiWidget *build_filtered_toolbar_widget (const std::vector<std::string> &titles,
                                               const ToolbarBuilder::Options &options = ToolbarBuilder::Options ())
  {
    ToolbarBuilder builder (action_registry_);
    return builder.build_widget (filtered_tool_surface_model (titles), options);
  }

  BobguiWidget *build_workspace_toolbar_widget (const ToolbarBuilder::Options &options = ToolbarBuilder::Options ())
  {
    ToolbarBuilder builder (action_registry_);
    return builder.build_widget (workspace_tool_surface_model (), options);
  }

  BobguiWidget *build_panel_toolbar_widget (const ToolbarBuilder::Options &options = ToolbarBuilder::Options ())
  {
    ToolbarBuilder builder (action_registry_);
    return builder.build_widget (panel_tool_surface_model (), options);
  }

  BobguiWidget *build_labeled_toolbar_widget ()
  {
    return build_toolbar_widget (ToolbarBuilder::Options::labeled ());
  }

  BobguiWidget *build_compact_toolbar_widget ()
  {
    return build_toolbar_widget (ToolbarBuilder::Options::compact ());
  }

  BobguiWidget *build_workspace_toolbar_preset ()
  {
    return build_workspace_toolbar_widget (ToolbarBuilder::Options::labeled ());
  }

  BobguiWidget *build_panel_toolbar_preset ()
  {
    return build_panel_toolbar_widget (ToolbarBuilder::Options::compact ());
  }

  BobguiWidget *build_tagged_toolbar_widget (const std::string& tag, 
                                             const ToolbarBuilder::Options &options = ToolbarBuilder::Options ())
  {
    auto predicate = [tag] (const ActionRegistry::ActionInfo &info) {
        return info.tags.find(tag) != std::string::npos;
    };
    
    ToolbarBuilder builder (action_registry_);
    ToolSurfaceModel model = ToolSurfaceModel::from_action_sections (
        action_registry_.list_sections_filtered (predicate)
    );
    return builder.build_widget (model, options);
  }

  void add_header_action_for_command (const char *label,
                                      const char *command_id)
  {
    workbench_.add_header_action_for_command (label, command_id);
  }

  void enable_menubar (bool enabled)
  {
    workbench_.enable_menubar (enabled);
  }

  void enable_toolbar (bool enabled)
  {
    workbench_.enable_toolbar (enabled);
  }

  void add_command (const char               *command_id,
                    const char               *title,
                    const char               *subtitle,
                    const Workbench::CommandOptions &options,
                    Workbench::CommandHandler handler)
  {
    workbench_.add_command (command_id,
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
    Workbench::CommandOptions options;

    options.section = "Workspace";
    options.category = "Workspace";
    options.shortcut = shortcut;
    options.icon_name = icon_name;

    add_command (command_id,
                 title,
                 subtitle,
                 options,
                 std::move (handler));
  }

  void add_toggle_command (const char               *command_id,
                           const char               *title,
                           const char               *subtitle,
                           const Workbench::CommandOptions &options,
                           bool                      checked,
                           Workbench::CommandHandler handler)
  {
    workbench_.add_toggle_command (command_id,
                                   title,
                                   subtitle,
                                   options,
                                   checked,
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
    Workbench::CommandOptions options;

    dock_manager ();
    options.section = "Panels";
    options.category = "View";
    options.shortcut = shortcut;
    options.icon_name = icon_name;

    add_toggle_command (command_id,
                        title,
                        subtitle,
                        options,
                        checked,
                        std::move (handler));
  }

  void present ()
  {
    workbench_.present ();
  }

  void set_left_sidebar_visible (bool visible)
  {
    BobguiWidget *sidebar = workbench_.get_left_sidebar ();
    if (sidebar != NULL)
      bobgui_widget_set_visible (sidebar, visible);
  }

  void set_right_sidebar_visible (bool visible)
  {
    BobguiWidget *sidebar = workbench_.get_right_sidebar ();
    if (sidebar != NULL)
      bobgui_widget_set_visible (sidebar, visible);
  }

  void set_toolbar_visible (bool visible)
  {
    workbench_.enable_toolbar (visible);
  }

  void set_menubar_visible (bool visible)
  {
    workbench_.enable_menubar (visible);
  }

private:
  Workbench workbench_;
  ActionRegistry action_registry_;
  CommandPalette command_palette_;
  std::unique_ptr<DockManager> dock_manager_;
};

} /* namespace cpp */
} /* namespace bobgui */

#endif
