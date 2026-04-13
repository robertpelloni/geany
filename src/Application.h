/**
 * Application.h
 *
 * Modern C++ refactor of the core startup sequence in `src/main.c` and `src/libmain.c`.
 * This class acts as the central object-oriented root of the IDE,
 * instantiating and owning all major C++ managers.
 */

#ifndef GEANY_APPLICATION_H
#define GEANY_APPLICATION_H

#include <memory>
#include <string>
#include <vector>

namespace geany {

// Forward declarations of all managers
class DocumentManager;
class ProjectManager;
class PluginManager;
class KeybindingManager;
class FileTypeManager;
class ToolsManager;

class Application {
public:
    Application();
    ~Application();

    // Prevent copying
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    // Initializes the application state, configuration directories, and managers.
    // Replaces the sprawling `main_init()` in libmain.c.
    bool Initialize(int argc, char** argv);

    // Enters the main execution loop (typically handing off to GTK or Qt).
    int Run();

    // Cleanly shuts down the application, saving states and tearing down plugins.
    // Replaces `main_finalize()`.
    void Quit();

    // --- Accessors for Managers ---
    DocumentManager* GetDocumentManager() const { return m_docManager.get(); }
    ProjectManager* GetProjectManager() const { return m_projManager.get(); }
    PluginManager* GetPluginManager() const { return m_pluginManager.get(); }
    KeybindingManager* GetKeybindingManager() const { return m_keyManager.get(); }
    FileTypeManager* GetFileTypeManager() const { return m_fileTypeManager.get(); }
    ToolsManager* GetToolsManager() const { return m_toolsManager.get(); }

private:
    bool m_initialized;

    // Core IDE Managers
    std::unique_ptr<DocumentManager> m_docManager;
    std::unique_ptr<ProjectManager> m_projManager;
    std::unique_ptr<PluginManager> m_pluginManager;
    std::unique_ptr<KeybindingManager> m_keyManager;
    std::unique_ptr<FileTypeManager> m_fileTypeManager;
    std::unique_ptr<ToolsManager> m_toolsManager;

    // Parses raw command line arguments into an internal vector.
    void ParseArgs(int argc, char** argv);
    std::vector<std::string> m_args;
};

} // namespace geany

#endif // GEANY_APPLICATION_H
