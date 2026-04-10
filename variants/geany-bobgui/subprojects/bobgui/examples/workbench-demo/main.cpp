#include <bobgui/cpp/bobgui.hpp>

#include <iostream>
#include <memory>
#include <string>

using bobgui::cpp::Application;
using bobgui::cpp::StudioShell;

int
main (int argc, char **argv)
{
  Application app ("org.bobgui.WorkbenchDemoCpp");
  std::unique_ptr<StudioShell> shell;

  app.on_startup ([&] (Application &) {
    std::cout << "Application starting up..." << std::endl;
  });

  app.on_activate ([&] (Application &application) {
    shell.reset (new StudioShell (application));

    BobguiWidget *editor = bobgui_text_view_new ();
    BobguiWidget *sidebar = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 6);
    BobguiWidget *inspector = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 6);

    bobgui_box_append (BOBGUI_BOX (sidebar), bobgui_label_new ("Project"));
    bobgui_box_append (BOBGUI_BOX (sidebar), bobgui_label_new ("Files"));
    bobgui_box_append (BOBGUI_BOX (inspector), bobgui_label_new ("Inspector"));
    bobgui_box_append (BOBGUI_BOX (inspector), bobgui_label_new ("Properties"));

    shell->pin_command ("app.about", true);

    shell->set_title ("Bobgui Workbench Demo (C++)");
    shell->set_navigation_panel (sidebar);
    shell->set_document_view (editor);
    shell->set_inspector_panel (inspector);

    // Demonstrate Ultimate++ style layout operators
    bobgui::cpp::Box custom_container (BOBGUI_ORIENTATION_VERTICAL, 10);
    
    // Apply Flexbox Layout (Qt6/JUCE parity)
    custom_container.set_layout(bobgui::cpp::FlexLayout()
        .flex_direction("column")
        .justify_content("center")
        .align_items("stretch"));

    custom_container << bobgui::cpp::Label ("Ultrasonic Layout (U++ Style)")
                     << bobgui::cpp::Button ("Click Me");
    
    bobgui_box_append (BOBGUI_BOX (inspector), custom_container.native ());

    bobgui::cpp::ActionRegistry::ActionOptions about_options;

    about_options.section = "Help";
    about_options.category = "Application";
    about_options.shortcut = "Ctrl+Shift+A";
    about_options.icon_name = "help-about-symbolic";
    about_options.tags = "help";

    shell->add_command ("app.about",
                        "About",
                        "Show application information",
                        about_options,
                        [&] (const std::string &) {
                          shell->set_status ("About action triggered");
                        });

    shell->add_panel_toggle_command ("view.toggle-left-sidebar",
                                     "Toggle Left Sidebar",
                                     "Show or hide the project sidebar",
                                     "Ctrl+B",
                                     "sidebar-show-right-symbolic",
                                     true,
                                     [&] (const std::string &) {
                                       bool is_visible = !shell->actions ().is_checked ("view.toggle-left-sidebar");
                                       shell->actions ().set_checked ("view.toggle-left-sidebar", is_visible);
                                       shell->set_navigation_panel_visible (is_visible);
                                       shell->set_status (is_visible ? "Sidebar visible" : "Sidebar hidden");
                                     });

    shell->add_workspace_command ("workspace.focus-editor",
                                  "Focus Editor",
                                  "Move attention to the main document view",
                                  "Ctrl+1",
                                  "document-edit-symbolic",
                                  [&] (const std::string &) {
                                    shell->set_status ("Editor workspace action triggered");
                                  });

    // Persistent Settings (Qt/JUCE Parity)
    bobgui::cpp::Settings settings ("Demo");
    int launch_count = settings.get_int ("launch_count", 0);
    settings.set_int ("launch_count", launch_count + 1);
    std::cout << "Application launch count: " << launch_count + 1 << std::endl;

    // Reactive Properties (Qt6 Parity)
    static bobgui::cpp::Property<std::string> app_status;
    app_status.on_changed.connect([&] (const std::string& new_status) {
        shell->set_status(new_status.c_str());
    });
    app_status = "Ultrasonic Framework Active";

    // Type-Safe Signals (JUCE/Qt Parity)
    static bobgui::cpp::Signal<int> counter_signal;
    static int internal_counter = 0;
    counter_signal.connect([&] (int count) {
        app_status = "Counter reached: " + std::to_string(count);
    });

    // Periodic Timer (Qt/JUCE Parity)
    static bobgui::cpp::Timer status_timer;
    status_timer.start (5000, [&] () {
      internal_counter++;
      counter_signal(internal_counter);
      return true; // repeat
    });

    // Property Animation (Qt6 Parity)
    static bobgui::cpp::Property<float> anim_val(0.0f);
    static bobgui::cpp::PropertyAnimation animation(anim_val);
    anim_val.on_changed.connect([&] (float val) {
        // In a real app, this would update a widget opacity or position
        if (internal_counter % 2 == 0) // avoid flooding console
            std::cout << "Animation Progress: " << val << std::endl;
    });
    animation.set_range(0.0f, 100.0f, 2000);
    animation.start();

    bobgui_box_append (BOBGUI_BOX (sidebar), bobgui_label_new ("Workspace"));
    bobgui_box_append (BOBGUI_BOX (sidebar), shell->build_workspace_toolbar_preset ());

    bobgui_box_append (BOBGUI_BOX (inspector), bobgui_label_new ("Panels"));
    bobgui_box_append (BOBGUI_BOX (inspector), shell->build_panel_toolbar_preset ());

    bobgui_box_append (BOBGUI_BOX (inspector), bobgui_label_new ("Panel Tools"));
    bobgui_box_append (BOBGUI_BOX (inspector), shell->build_panel_tool_surface_preset ());

    bobgui_box_append (BOBGUI_BOX (inspector), bobgui_label_new ("All Tools"));
    bobgui_box_append (BOBGUI_BOX (inspector), shell->build_descriptive_tool_surface_widget ());

    // Demonstrate Tagged Toolbars (Qt6 Parity)
    bobgui_box_append (BOBGUI_BOX (sidebar), bobgui_label_new ("Help (Tagged)"));
    bobgui_box_append (BOBGUI_BOX (sidebar), shell->build_tagged_toolbar_widget ("help"));

    {
      bobgui::cpp::ToolSurfaceModel tool_surface = shell->tool_surface_model ();
      std::string status = "Ready: " + std::to_string (tool_surface.section_count ()) +
                           " sections / " + std::to_string (tool_surface.item_count ()) +
                           " tools / " + std::to_string (shell->workspace_action_count ()) +
                           " workspace / " + std::to_string (shell->panel_action_count ()) +
                           " panel";
      shell->set_status (status.c_str ());
    }

    shell->add_header_action_for_command ("About", "app.about");
    shell->add_header_action_for_command ("Sidebar", "view.toggle-left-sidebar");
    shell->enable_menubar (true);
    shell->enable_toolbar (true);
    shell->present ();
  });

  app.on_shutdown ([&] (Application &) {
    std::cout << "Application shutting down..." << std::endl;
    shell.reset ();
  });

  return app.run (argc, argv);
}
