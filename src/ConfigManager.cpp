/**
 * ConfigManager.cpp
 *
 * Safe C++ implementation of Geany's keyfile configuration operations.
 */

#include "ConfigManager.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

namespace geany {

ConfigManager::ConfigManager() {}

ConfigManager::~ConfigManager() {
    Clear();
}

std::string ConfigManager::TrimWhitespace(const std::string& str) const {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

void ConfigManager::Clear() {
    m_config.clear();
}

bool ConfigManager::LoadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "[ConfigManager] Warning: Failed to open config file for reading: " << filename << std::endl;
        return false;
    }

    std::string line;
    std::string currentSection = "";

    while (std::getline(file, line)) {
        line = TrimWhitespace(line);
        if (line.empty() || line[0] == ';' || line[0] == '#') continue;

        if (line[0] == '[' && line.back() == ']') {
            currentSection = TrimWhitespace(line.substr(1, line.size() - 2));
        } else {
            size_t delimiterPos = line.find('=');
            if (delimiterPos != std::string::npos) {
                std::string key = TrimWhitespace(line.substr(0, delimiterPos));
                std::string value = TrimWhitespace(line.substr(delimiterPos + 1));
                if (!key.empty()) {
                    m_config[currentSection][key] = value;
                }
            }
        }
    }

    return true;
}

bool ConfigManager::SaveToFile(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "[ConfigManager] Error: Failed to open config file for writing: " << filename << std::endl;
        return false;
    }

    // Write global section first if it exists
    if (m_config.find("") != m_config.end()) {
        for (const auto& pair : m_config.at("")) {
            file << pair.first << "=" << pair.second << "\n";
        }
        file << "\n";
    }

    for (const auto& section : m_config) {
        if (section.first.empty()) continue; // Skip global

        file << "[" << section.first << "]\n";
        for (const auto& pair : section.second) {
            file << pair.first << "=" << pair.second << "\n";
        }
        file << "\n";
    }

    return true;
}

std::string ConfigManager::GetString(const std::string& section, const std::string& key, const std::string& fallback) const {
    auto secIt = m_config.find(section);
    if (secIt != m_config.end()) {
        auto keyIt = secIt->second.find(key);
        if (keyIt != secIt->second.end()) {
            return keyIt->second;
        }
    }
    return fallback;
}

void ConfigManager::SetString(const std::string& section, const std::string& key, const std::string& value) {
    m_config[section][key] = value;
}

int ConfigManager::GetInt(const std::string& section, const std::string& key, int fallback) const {
    std::string val = GetString(section, key, "");
    if (val.empty()) return fallback;
    try {
        return std::stoi(val);
    } catch (...) {
        return fallback;
    }
}

void ConfigManager::SetInt(const std::string& section, const std::string& key, int value) {
    SetString(section, key, std::to_string(value));
}

bool ConfigManager::GetBool(const std::string& section, const std::string& key, bool fallback) const {
    std::string val = GetString(section, key, "");
    if (val.empty()) return fallback;
    std::transform(val.begin(), val.end(), val.begin(), ::tolower);
    return val == "true" || val == "1" || val == "yes";
}

void ConfigManager::SetBool(const std::string& section, const std::string& key, bool value) {
    SetString(section, key, value ? "true" : "false");
}

} // namespace geany
