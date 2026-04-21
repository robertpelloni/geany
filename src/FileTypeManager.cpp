/**
 * FileTypeManager.cpp
 *
 * Implementation of the C++ FileTypeManager using STL maps.
 */

#include "FileTypeManager.h"
#include <iostream>
#include <algorithm>

namespace geany {

FileTypeManager::FileTypeManager() {
    InitDefaults();
}

FileTypeManager::~FileTypeManager() {
    m_fileTypes.clear();
}

void FileTypeManager::Register(const FileType& ft) {
    if (m_fileTypes.find(ft.id) != m_fileTypes.end()) {
        std::cerr << "[FileTypeManager] Warning: Overwriting existing filetype: " << ft.id << std::endl;
    }

    auto newFt = std::make_unique<FileType>(ft);
    m_fileTypes[ft.id] = std::move(newFt);
    std::cout << "[FileTypeManager] Registered: " << ft.title << " (" << ft.id << ")" << std::endl;
}

const FileType* FileTypeManager::GetById(const std::string& id) const {
    auto it = m_fileTypes.find(id);
    if (it != m_fileTypes.end()) {
        return it->second.get();
    }
    // Fallback to plain text
    auto fallback = m_fileTypes.find("none");
    return fallback != m_fileTypes.end() ? fallback->second.get() : nullptr;
}

const FileType* FileTypeManager::Detect(const std::string& filename) const {
    if (filename.empty()) {
        return GetById("none");
    }

    size_t lastSlash = filename.find_last_of("/\\");
    std::string base = (lastSlash == std::string::npos) ? filename : filename.substr(lastSlash + 1);

    // 1. Try exact filename match (e.g., "Makefile")
    for (const auto& pair : m_fileTypes) {
        for (const auto& pattern : pair.second->patterns) {
            if (pattern == base) {
                return pair.second.get();
            }
        }
    }

    // 2. Try extension match (e.g., "*.cpp")
    size_t lastDot = base.find_last_of('.');
    if (lastDot != std::string::npos) {
        std::string ext = "*" + base.substr(lastDot); // Transform ".cpp" to "*.cpp"
        // In a full implementation we'd lowercase the extension for case-insensitive matching
        for (const auto& pair : m_fileTypes) {
            for (const auto& pattern : pair.second->patterns) {
                if (pattern == ext) {
                    return pair.second.get();
                }
            }
        }
    }

    // 3. Fallback
    return GetById("none");
}

std::vector<const FileType*> FileTypeManager::GetAllTypes() const {
    std::vector<const FileType*> list;
    list.reserve(m_fileTypes.size());
    for (const auto& pair : m_fileTypes) {
        list.push_back(pair.second.get());
    }
    return list;
}

void FileTypeManager::InitDefaults() {
    FileType noneFT = {"none", "None", ".txt", {"*.txt"}, 0};
    Register(noneFT);

    FileType cFT = {"c", "C/C++", ".c", {"*.c", "*.cpp", "*.cxx", "*.h", "*.hpp"}, 3}; // SCLEX_CPP=3
    Register(cFT);

    FileType makeFT = {"make", "Make", "Makefile", {"Makefile", "makefile", "*.mak"}, 34}; // SCLEX_MAKEFILE=34
    Register(makeFT);
}

} // namespace geany
