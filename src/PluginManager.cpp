/**
 * PluginManager.cpp
 *
 * Implementation of the C++ PluginManager using STL maps and strings.
 * This abstracts away platform-specific `dlopen` or `GModule` logic into a cleaner interface.
 */

#include "PluginManager.h"
#include <iostream>
#include <algorithm>

namespace geany {

PluginManager::PluginManager() {}

PluginManager::~PluginManager() {
    UnloadAll();
}

bool PluginManager::LoadPlugin(const std::string& filename) {
    // Note: C++ Stub implementation for now.
    // In a full implementation, we'd include glib.h and use g_module_open.
    auto info = std::make_unique<PluginInfo>();

    size_t lastSlash = filename.find_last_of("/\\");
    std::string base = (lastSlash == std::string::npos) ? filename : filename.substr(lastSlash + 1);

    info->name = base;
    info->filename = filename;
    info->description = "A C++ refactored generic plugin";
    info->version = "1.0";
    info->author = "Geany AI";
    info->isActive = true;
    info->handle = nullptr;

    m_plugins[base] = std::move(info);
    return true;
}

bool PluginManager::UnloadPlugin(const std::string& name) {
    auto it = m_plugins.find(name);
    if (it != m_plugins.end()) {
        m_plugins.erase(it);
        return true;
    }
    return false;
}

void PluginManager::UnloadAll() {
    while (!m_plugins.empty()) {
        UnloadPlugin(m_plugins.begin()->first);
    }
}

std::vector<PluginInfo> PluginManager::GetActivePlugins() const {
    std::vector<PluginInfo> activeList;
    for (const auto& pair : m_plugins) {
        if (pair.second && pair.second->isActive) {
            activeList.push_back(*pair.second);
        }
    }
    return activeList;
}

void PluginManager::ScanDirectory(const std::string& path) {
    // Stub
}

} // namespace geany
