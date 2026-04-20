/**
 * Application.cpp
 *
 * Implementation of the C++ Application root class.
 */

#include "Application.h"
#include "DocumentManager.h"
#include "ProjectManager.h"
#include "PluginManager.h"
#include "KeybindingManager.h"
#include "FileTypeManager.h"
#include "ToolsManager.h"
#include "ConfigManager.h"
#include <iostream>

namespace geany {

Application::Application() : m_initialized(false) {
    // Instantiate all core managers.
    // The order here defines the fundamental dependency graph of the IDE.

    m_docManager = std::make_unique<DocumentManager>();
    m_projManager = std::make_unique<ProjectManager>();
    m_pluginManager = std::make_unique<PluginManager>();
    m_keyManager = std::make_unique<KeybindingManager>();
    m_fileTypeManager = std::make_unique<FileTypeManager>();

    // ToolsManager requires a pointer to the DocumentManager
    m_toolsManager = std::make_unique<ToolsManager>(m_docManager.get());
    m_configManager = std::make_unique<ConfigManager>();
}

Application::~Application() {
    if (m_initialized) {
        Quit();
    }
}

void Application::ParseArgs(int argc, char** argv) {
    for (int i = 0; i < argc; ++i) {
        if (argv[i]) {
            m_args.push_back(std::string(argv[i]));
        }
    }
}

bool Application::Initialize(int argc, char** argv) {
    if (m_initialized) {
        std::cerr << "[Application] Error: Already initialized." << std::endl;
        return false;
    }

    std::cout << "[Application] Initializing Geany Ultra-Project..." << std::endl;
    ParseArgs(argc, argv);

    // Simulate standard initialization sequences:
    // 1. Load settings (using Go config engine eventually)
    // 2. Load Filetypes
    // 3. Register Keybindings
    // 4. Scan for Plugins

    m_pluginManager->ScanDirectory("/usr/lib/geany");

    // If files were passed on CLI, open them
    for (size_t i = 1; i < m_args.size(); ++i) {
        // Skip basic flags
        if (!m_args[i].empty() && m_args[i][0] != '-') {
            m_docManager->OpenDocument(m_args[i]);
        }
    }

    m_initialized = true;
    return true;
}

int Application::Run() {
    if (!m_initialized) {
        std::cerr << "[Application] Cannot Run before Initialization." << std::endl;
        return 1;
    }

    std::cout << "[Application] Entering main event loop..." << std::endl;
    // Stub: This is where we hand off control to the UI framework (GTK, Qt, etc.)
    // For this C++ refactor phase, we return immediately.

    return 0;
}

void Application::Quit() {
    if (!m_initialized) return;

    std::cout << "[Application] Shutting down..." << std::endl;

    // Shutdown sequence:
    // 1. Save preferences
    // 2. Unload plugins gracefully
    m_pluginManager->UnloadAll();

    // 3. Prompt user for unsaved documents, then close
    // m_docManager->CloseAll();

    m_initialized = false;
}

} // namespace geany

// External Go bindings
extern "C" {
    void GeanyGo_Scintilla_Bind(int64_t fnPtr, int64_t objPtr);
}

namespace geany {
    void Application::BindGoScintilla(intptr_t fnPtr, intptr_t objPtr) {
        // Send raw Scintilla message function and object pointers across FFI to Go backend
        GeanyGo_Scintilla_Bind(static_cast<int64_t>(fnPtr), static_cast<int64_t>(objPtr));
    }
}

// C API Export mappings
#include "Application_C_Bridge.h"

extern "C" {
    GeanyApplicationHandle geany_application_new(void) {
        return new geany::Application();
    }

    void geany_application_free(GeanyApplicationHandle handle) {
        if (handle) {
            delete static_cast<geany::Application*>(handle);
        }
    }

    int geany_application_initialize(GeanyApplicationHandle handle, int argc, char** argv) {
        if (!handle) return 0;
        return static_cast<geany::Application*>(handle)->Initialize(argc, argv) ? 1 : 0;
    }

    int geany_application_run(GeanyApplicationHandle handle) {
        if (!handle) return 1;
        return static_cast<geany::Application*>(handle)->Run();
    }

    void geany_application_quit(GeanyApplicationHandle handle) {
        if (!handle) return;
        static_cast<geany::Application*>(handle)->Quit();
    }
}
