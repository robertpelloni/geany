#include <bobgui/cpp/bobgui.hpp>

#include <iostream>
#include <memory>
#include <string>

using bobgui::cpp::Application;
using bobgui::cpp::DocumentShell;

int
main (int argc, char **argv)
{
  Application app ("org.bobgui.DocumentDemoCpp", G_APPLICATION_HANDLES_OPEN);
  std::unique_ptr<DocumentShell> shell;

  app.on_startup ([&] (Application &) {
    std::cout << "Document application starting up..." << std::endl;
  });

  app.on_open ([&] (Application &application, GFile **files, int n_files, const char *hint) {
    (void) application;
    (void) hint;
    std::cout << "Document application asked to open " << n_files << " files." << std::endl;
    for (int i = 0; i < n_files; i++) {
        char *uri = g_file_get_uri (files[i]);
        std::cout << "  - " << uri << std::endl;
        g_free (uri);
    }
  });

  app.on_activate ([&] (Application &application) {
    shell.reset (new DocumentShell (application));

    BobguiWidget *outline = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 6);
    BobguiWidget *content = bobgui_text_view_new ();
    BobguiWidget *details = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 6);

    bobgui_box_append (BOBGUI_BOX (outline), bobgui_label_new ("Outline"));
    bobgui_box_append (BOBGUI_BOX (outline), bobgui_label_new ("Chapter 1"));
    bobgui_box_append (BOBGUI_BOX (outline), bobgui_label_new ("Chapter 2"));
    bobgui_box_append (BOBGUI_BOX (details), bobgui_label_new ("Details"));
    bobgui_box_append (BOBGUI_BOX (details), bobgui_label_new ("Metadata"));

    shell->set_title ("Bobgui Document Demo (C++)");
    shell->set_outline_panel (outline);
    shell->set_content_view (content);
    shell->set_details_panel (details);
    shell->pin_command ("document.outline.toggle", true);

    shell->add_document_command ("document.save",
                                 "Save Document",
                                 "Write the active document to disk",
                                 "Ctrl+S",
                                 "document-save-symbolic",
                                 [&] (const std::string &) {
                                   shell->set_status ("Document save triggered");
                                 });

    shell->add_workspace_command ("document.focus-content",
                                  "Focus Content",
                                  "Jump to the main document editor",
                                  "Ctrl+1",
                                  "document-edit-symbolic",
                                  [&] (const std::string &) {
                                    shell->set_status ("Content workspace action triggered");
                                  });

    shell->add_panel_toggle_command ("document.outline.toggle",
                                     "Toggle Outline",
                                     "Show or hide the outline panel",
                                     "Ctrl+Shift+O",
                                     "sidebar-show-left-symbolic",
                                     true,
                                     [&] (const std::string &) {
                                       bool is_visible = !shell->actions ().is_checked ("document.outline.toggle");
                                       shell->actions ().set_checked ("document.outline.toggle", is_visible);
                                       shell->set_outline_panel_visible (is_visible);
                                       shell->set_status (is_visible ? "Outline visible" : "Outline hidden");
                                     });

    bobgui_box_append (BOBGUI_BOX (outline), bobgui_label_new ("Document Actions"));
    bobgui_box_append (BOBGUI_BOX (outline), shell->build_document_toolbar_widget ());

    bobgui_box_append (BOBGUI_BOX (details), bobgui_label_new ("Detail Actions"));
    bobgui_box_append (BOBGUI_BOX (details), shell->build_document_panel_toolbar_widget ());
    bobgui_box_append (BOBGUI_BOX (details), bobgui_label_new ("Document Tools"));
    bobgui_box_append (BOBGUI_BOX (details), shell->build_document_tools_widget ());
    bobgui_box_append (BOBGUI_BOX (details), bobgui_label_new ("Detail Tools"));
    bobgui_box_append (BOBGUI_BOX (details), shell->build_document_panel_tools_widget ());

    shell->add_header_action_for_command ("Save", "document.save");
    shell->add_header_action_for_command ("Content", "document.focus-content");
    shell->add_header_action_for_command ("Outline", "document.outline.toggle");
    shell->enable_menubar (true);
    shell->enable_toolbar (true);

    {
      std::string status = "Document shell ready: " + std::to_string (shell->document_action_count ()) +
                           " document actions";
      shell->set_status (status.c_str ());
    }

    shell->present ();
  });

  app.on_shutdown ([&] (Application &) {
    std::cout << "Document application shutting down..." << std::endl;
    shell.reset ();
  });

  return app.run (argc, argv);
}
