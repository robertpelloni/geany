/**
 * PluginManager.h
 *
 * Modern C++ refactor of the core of `src/plugins.c`.
 * This class utilizes standard library containers to safely manage the lifecycle,
 * loading, unloading, and routing of Geany plugins.
 */

#ifndef GEANY_PLUGIN_MANAGER_H
#define GEANY_PLUGIN_MANAGER_H

#include <string>
#include <vector>
#include <map>
#include <memory>

// Forward declaration of a generic plugin handle
struct PluginHandle;

namespace geany {

// Represents metadata and state for a loaded plugin.
struct PluginInfo {
    std::string name;
    std::string description;
    std::string version;
    std::string author;
    std::string filename;
    bool isActive;
    PluginHandle* handle; // Pointer to the loaded dynamic library
};

class PluginManager {
public:
    PluginManager();
    ~PluginManager();

    // Prevent copying
    PluginManager(const PluginManager&) = delete;
    PluginManager& operator=(const PluginManager&) = delete;

    // Load a specific plugin by its absolute or relative filename.
    bool LoadPlugin(const std::string& filename);

    // Unload a specific plugin by its internally registered name.
    bool UnloadPlugin(const std::string& name);

    // Unload all currently active plugins, safely tearing down their state.
    void UnloadAll();

    // Retrieves a list of all currently known/loaded plugins.
    std::vector<PluginInfo> GetActivePlugins() const;

    // Scans a directory for valid shared objects (.so / .dll / .dylib) to load later.
    void ScanDirectory(const std::string& path);

private:
    // Safely manages the dynamic library handles (replacing bare arrays/GSLists).
    std::map<std::string, std::unique_ptr<PluginInfo>> m_plugins;
};

} // namespace geany

#endif // GEANY_PLUGIN_MANAGER_H
