/**
 * ConfigManager.h
 *
 * Modern C++ replacement for src/keyfile.c INI configuration parsing.
 * Utilizes std::map and std::string for safe, memory-managed key-value storage.
 * Interfaces smoothly with the Go config package for ultra-project parity.
 */

#ifndef GEANY_CONFIG_MANAGER_H
#define GEANY_CONFIG_MANAGER_H

#include <string>
#include <map>
#include <vector>

namespace geany {

// Represents a standard INI-style configuration file
class ConfigManager {
public:
    ConfigManager();
    ~ConfigManager();

    // Prevent copying
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    // Load and Save operations
    bool LoadFromFile(const std::string& filename);
    bool SaveToFile(const std::string& filename) const;

    // Accessors
    std::string GetString(const std::string& section, const std::string& key, const std::string& fallback = "") const;
    void SetString(const std::string& section, const std::string& key, const std::string& value);

    int GetInt(const std::string& section, const std::string& key, int fallback = 0) const;
    void SetInt(const std::string& section, const std::string& key, int value);

    bool GetBool(const std::string& section, const std::string& key, bool fallback = false) const;
    void SetBool(const std::string& section, const std::string& key, bool value);

    // Clears all configuration data from memory
    void Clear();

private:
    // Memory-safe map of Section -> Key -> Value
    std::map<std::string, std::map<std::string, std::string>> m_config;

    // Helper utilities for string parsing
    std::string TrimWhitespace(const std::string& str) const;
};

} // namespace geany

#endif // GEANY_CONFIG_MANAGER_H
